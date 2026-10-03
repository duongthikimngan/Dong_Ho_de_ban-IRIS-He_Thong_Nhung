/*
 * ESP32-C3 + DHT11 + DS3231 + Pin 3.7V + LCD GC9A01 240x240
 * Kiến trúc Task + Queue (FreeRTOS):
 *
 *   dht_task ──┐
 *   rtc_task ──┼──► xQueue (sys_msg_t) ──► display_task ──► LCD
 *   bat_task ──┘
 *
 * - Nhiều task GỬI (writers), 1 task NHẬN (reader) -> đúng mô hình ở PDF Queue Management.
 * - Queue truyền cấu trúc (compound type) kèm mã nguồn `source` như ví dụ xData.
 * - display_task có mức ưu tiên cao nhất, block trên queue tối đa 2 s (giống Example 10).
 * - Task gửi block tối đa 100 ms nếu queue đầy (giống Example 11).
 * - rtc_task dùng vTaskDelayUntil (Example 5) để có chu kỳ cố định.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "app_config.h"
#include "sys_msg.h"
#include "dht22.h" // thay cho "dht11.h"
#include "ds3231.h"
#include "battery.h"
#include "lcd_gc9a01.h"
#include "gfx.h"
#include "ui.h"

static const char *TAG = "main";

/* ---------------- Task đọc DHT22 (mỗi 2,5 s) ---------------- */
static void dht_task(void *pvParameters)
{
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    dht22_init();
    vTaskDelay(pdMS_TO_TICKS(2000)); /* DHT22 cần ~1–2 s sau khi cấp nguồn */

    for (;;)
    {
        sys_msg_t msg = {.source = SRC_DHT22}; /* giữ nguyên mã nguồn để không phải sửa sys_msg.h */
        int16_t t10 = 0, h10 = 0;
        esp_err_t r = dht22_read(&t10, &h10);
        msg.data.dht.valid = (r == ESP_OK);
        if (r == ESP_OK)
        {
            /* làm tròn về số nguyên để khớp kiểu int8_t/uint8_t hiện tại */
            int t = (t10 + (t10 >= 0 ? 5 : -5)) / 10;
            int h = (h10 + 5) / 10;
            if (h > 100)
                h = 100;
            msg.data.dht.temp_c = (int8_t)t;
            msg.data.dht.humidity = (uint8_t)h;
        }
        else
        {
            ESP_LOGW(TAG, "DHT22 loi: %s", esp_err_to_name(r));
        }
        if (xQueueSendToBack(q, &msg, pdMS_TO_TICKS(100)) != pdPASS)
            ESP_LOGW(TAG, "Queue day (DHT)");
        vTaskDelay(pdMS_TO_TICKS(2500)); /* DHT22: tối thiểu 2 s giữa 2 lần đọc */
    }
}

/* ---------------- Task đọc DS3231 ----------------
 * Đọc mỗi 250 ms nhưng chỉ gửi vào queue khi giây thay đổi:
 *  - vẫn đúng 1 tin/giây trong queue
 *  - bám sát lúc DS3231 đổi giây -> không bị nhảy cóc / lặp giây
 */
static void rtc_task(void *pvParameters)
{
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    TickType_t last = xTaskGetTickCount();
    uint8_t last_sec = 0xFF; /* giá trị không hợp lệ -> lần đọc đầu luôn gửi */

    for (;;)
    {
        sys_msg_t msg = {.source = SRC_RTC};
        if (ds3231_get_time(&msg.data.time) == ESP_OK)
        {
            if (msg.data.time.sec != last_sec)
            {
                if (xQueueSendToBack(q, &msg, pdMS_TO_TICKS(100)) == pdPASS)
                    last_sec = msg.data.time.sec; /* chỉ ghi nhận khi gửi thành công */
                else
                    ESP_LOGW(TAG, "Queue day (RTC)");
            }
        }
        else
        {
            ESP_LOGW(TAG, "Doc DS3231 loi");
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(250));
    }
}

/* ---------------- Task đo pin (mỗi 5 s) ---------------- */
static void battery_task(void *pvParameters)
{
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    battery_init();

    for (;;)
    {
        sys_msg_t msg = {.source = SRC_BATTERY};
        if (battery_read(&msg.data.batt.mv, &msg.data.batt.percent) == ESP_OK)
        {
            ESP_LOGI(TAG, "Pin: %u mV = %u%%", msg.data.batt.mv, msg.data.batt.percent);
            if (xQueueSendToBack(q, &msg, pdMS_TO_TICKS(100)) != pdPASS)
                ESP_LOGW(TAG, "Queue day (BAT)");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ---------------- Task hiển thị: NHẬN từ queue ---------------- */
static void apply_msg(ui_state_t *ui, const sys_msg_t *m)
{
    switch (m->source)
    {
    case SRC_DHT22:
        if (m->data.dht.valid)
        {
            ui->temp_c = m->data.dht.temp_c;
            ui->humidity = m->data.dht.humidity;
            ui->dht_valid = true;
            ui->dht_fail = 0;
        }
        else if (++ui->dht_fail >= 3)
        { /* 3 lần lỗi liên tiếp (~6 s) mới hiện "--" */
            ui->dht_valid = false;
        }
        break;
    case SRC_RTC:
        ui->time = m->data.time;
        ui->time_valid = true;
        break;
    case SRC_BATTERY:
        ui->batt_percent = m->data.batt.percent;
        ui->batt_valid = true;
        break;
    }
}

static void display_task(void *pvParameters)
{
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    uint16_t *fb = NULL;
    if (lcd_init(&fb) != ESP_OK)
    {
        ESP_LOGE(TAG, "LCD init that bai");
        vTaskDelete(NULL);
    }
    gfx_set_fb(fb);

    ui_state_t ui = {0};
    ui_draw(&ui);
    lcd_flush();

    for (;;)
    {
        sys_msg_t msg;
        /* Block tối đa 2 s chờ dữ liệu (RTC gửi mỗi 1 s) */
        if (xQueueReceive(q, &msg, pdMS_TO_TICKS(2000)) == pdPASS)
        {
            apply_msg(&ui, &msg);
            /* gom hết các tin còn trong queue rồi mới vẽ 1 lần */
            while (xQueueReceive(q, &msg, 0) == pdPASS)
                apply_msg(&ui, &msg);
            ui_draw(&ui);
            lcd_flush();
        }
        else
        {
            ESP_LOGW(TAG, "Khong co du lieu trong 2 s");
        }
    }
}

void app_main(void)
{
    /* Trong ESP-IDF scheduler đã chạy sẵn, KHÔNG gọi vTaskStartScheduler() như bản Arduino */
    QueueHandle_t q = xQueueCreate(SYS_QUEUE_LENGTH, sizeof(sys_msg_t));
    if (q == NULL)
    {
        ESP_LOGE(TAG, "Khong tao duoc queue");
        return;
    }

    if (ds3231_init() != ESP_OK)
        ESP_LOGE(TAG, "DS3231 loi - kiem tra day I2C");

    /* Receiver ưu tiên cao nhất, các sender thấp hơn.
     * Stack tính bằng BYTE trong ESP-IDF (PDF Arduino tính bằng word). */
    if (xTaskCreate(display_task, "display", 4096, q, 5, NULL) != pdPASS ||
        xTaskCreate(rtc_task, "rtc", 3072, q, 4, NULL) != pdPASS ||
        xTaskCreate(dht_task, "dht22", 3072, q, 3, NULL) != pdPASS ||
        xTaskCreate(battery_task, "battery", 3072, q, 3, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Khong tao duoc task (thieu heap?)");
    }
}