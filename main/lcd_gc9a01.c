#include "lcd_gc9a01.h"
#include "app_config.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_gc9a01.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "lcd";
static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_done;
static uint16_t *s_fb;

#define STRIPS (LCD_H / LCD_STRIP_LINES)

static bool on_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &woken);
    return woken == pdTRUE;
}

esp_err_t lcd_init(uint16_t **framebuffer)
{
    s_done = xSemaphoreCreateCounting(STRIPS, 0);

    gpio_config_t bl = {.pin_bit_mask = 1ULL << PIN_LCD_BLK, .mode = GPIO_MODE_OUTPUT};
    gpio_config(&bl);
    gpio_set_level(PIN_LCD_BLK, 0);

    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_W * LCD_STRIP_LINES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_SPI_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = STRIPS + 2,
        .on_color_trans_done = on_trans_done,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io));

    esp_lcd_panel_dev_config_t pcfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, /* nếu đỏ/xanh bị ngược -> đổi thành _RGB */
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io, &pcfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, true, false)); /* lật trục X */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));

    s_fb = heap_caps_malloc(LCD_W * LCD_H * 2, MALLOC_CAP_DMA);
    if (!s_fb)
    {
        ESP_LOGE(TAG, "Không đủ RAM cho framebuffer");
        return ESP_ERR_NO_MEM;
    }
    for (int i = 0; i < LCD_W * LCD_H; i++)
        s_fb[i] = 0;
    lcd_flush();
    gpio_set_level(PIN_LCD_BLK, 1);

    *framebuffer = s_fb;
    return ESP_OK;
}

void lcd_flush(void)
{
    for (int y = 0; y < LCD_H; y += LCD_STRIP_LINES)
    {
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, LCD_W, y + LCD_STRIP_LINES, s_fb + y * LCD_W);
    }
    /* Chờ DMA xong từng dải; có timeout để task không bị treo nếu SPI lỗi */
    for (int i = 0; i < STRIPS; i++)
    {
        if (xSemaphoreTake(s_done, pdMS_TO_TICKS(200)) != pdTRUE)
        {
            ESP_LOGW(TAG, "flush timeout");
            break;
        }
    }
}