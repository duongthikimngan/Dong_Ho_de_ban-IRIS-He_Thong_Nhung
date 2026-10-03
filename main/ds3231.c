#include "ds3231.h"
#include "app_config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ds3231";
static i2c_master_dev_handle_t s_dev;

static uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

/* Thứ trong tuần (Sakamoto): 0 = CN */
static uint8_t calc_dow(int y, int m, int d)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3)
        y -= 1;
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}

static esp_err_t read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, 100);
}

static esp_err_t set_from_build_time(void)
{
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {0};
    int d, y, hh, mm, ss;
    sscanf(__DATE__, "%3s %d %d", mon, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
    const char *p = strstr(months, mon);
    int m = p ? (int)((p - months) / 3) + 1 : 1;

    rtc_time_t t = {.sec = ss, .min = mm, .hour = hh, .day = d, .month = m, .year = y};
    t.dow = calc_dow(y, m, d);
    ESP_LOGW(TAG, "Set RTC = %02d/%02d/%04d %02d:%02d:%02d", d, m, y, hh, mm, ss);
    return ds3231_set_time(&t);
}

esp_err_t ds3231_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev));

    uint8_t status;
    esp_err_t ret = read_regs(0x0F, &status, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Không thấy DS3231: %s", esp_err_to_name(ret));
        return ret;
    }
    /* Bit7 = OSF: oscillator đã dừng (mất pin) -> giờ không đáng tin */
    if ((status & 0x80) || RTC_FORCE_SET_FROM_BUILD)
    {
        ret = set_from_build_time();
        uint8_t w[2] = {0x0F, (uint8_t)(status & ~0x80)};
        i2c_master_transmit(s_dev, w, 2, 100);
    }
    return ret;
}

esp_err_t ds3231_get_time(rtc_time_t *t)
{
    uint8_t b[7];
    esp_err_t ret = read_regs(0x00, b, 7);
    if (ret != ESP_OK)
        return ret;
    t->sec = bcd2dec(b[0] & 0x7F);
    t->min = bcd2dec(b[1] & 0x7F);
    t->hour = bcd2dec(b[2] & 0x3F);
    t->day = bcd2dec(b[4] & 0x3F);
    t->month = bcd2dec(b[5] & 0x1F);
    t->year = 2000 + bcd2dec(b[6]);
    t->dow = calc_dow(t->year, t->month, t->day);
    return ESP_OK;
}

esp_err_t ds3231_set_time(const rtc_time_t *t)
{
    uint8_t b[8] = {
        0x00,
        dec2bcd(t->sec),
        dec2bcd(t->min),
        dec2bcd(t->hour),
        (uint8_t)(t->dow + 1),
        dec2bcd(t->day),
        dec2bcd(t->month),
        dec2bcd(t->year % 100),
    };
    return i2c_master_transmit(s_dev, b, sizeof(b), 100);
}
