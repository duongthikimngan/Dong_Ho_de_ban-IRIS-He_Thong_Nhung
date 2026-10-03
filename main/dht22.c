#include <stdbool.h>
#include "dht22.h"
#include "app_config.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dht22";
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

/* Đợi trong lúc chân còn ở mức `level`; trả về thời gian (us) hoặc -1 nếu quá hạn.
 * Dùng esp_timer nên đo chính xác hơn vòng lặp delay 1 us. */
static int wait_level(int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(PIN_DHT22) == level)
    {
        if (esp_timer_get_time() - start > timeout_us)
            return -1;
    }
    return (int)(esp_timer_get_time() - start);
}

esp_err_t dht22_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << PIN_DHT22,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io);
    gpio_set_level(PIN_DHT22, 1);
    return ESP_OK;
}

esp_err_t dht22_read(int16_t *temp_x10, int16_t *hum_x10)
{
    uint8_t d[5] = {0};
    bool err = false;

    /* Tín hiệu start: kéo thấp >= 1 ms (DHT22), dùng 2 ms */
    gpio_set_level(PIN_DHT22, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    /* Nếu tick = 10 ms thì vTaskDelay(2ms) vẫn >= 1 tick, đủ dài */

    portENTER_CRITICAL(&s_mux); /* đọc 40 bit ~4–5 ms, cần timing chính xác */
    gpio_set_level(PIN_DHT22, 1);
    esp_rom_delay_us(30);

    if (wait_level(1, 100) < 0)
        err = true; /* chờ DHT kéo thấp */
    if (!err && wait_level(0, 120) < 0)
        err = true; /* ~80us thấp */
    if (!err && wait_level(1, 120) < 0)
        err = true; /* ~80us cao  */

    for (int i = 0; i < 40 && !err; i++)
    {
        int low = wait_level(0, 100);  /* ~50us thấp */
        int high = wait_level(1, 150); /* ~27us = bit 0, ~70us = bit 1 */
        if (low < 0 || high < 0)
        {
            err = true;
            break;
        }
        d[i / 8] <<= 1;
        if (high > 40) /* ngưỡng cố định 40 us ổn định hơn so sánh high > low */
            d[i / 8] |= 1;
    }
    portEXIT_CRITICAL(&s_mux);
    gpio_set_level(PIN_DHT22, 1);

    if (err)
        return ESP_ERR_TIMEOUT;
    if (((d[0] + d[1] + d[2] + d[3]) & 0xFF) != d[4])
        return ESP_ERR_INVALID_CRC;

    ESP_LOGD(TAG, "raw: %02X %02X %02X %02X %02X", d[0], d[1], d[2], d[3], d[4]);

    int16_t h = (int16_t)((d[0] << 8) | d[1]);
    int16_t t = (int16_t)(((d[2] & 0x7F) << 8) | d[3]);
    if (d[2] & 0x80) /* bit dấu: nhiệt độ âm */
        t = -t;

    if (h < 0 || h > 1000 || t < -400 || t > 800) /* ngoài dải của DHT22 */
        return ESP_ERR_INVALID_RESPONSE;

    *hum_x10 = h;
    *temp_x10 = t;
    return ESP_OK;
}