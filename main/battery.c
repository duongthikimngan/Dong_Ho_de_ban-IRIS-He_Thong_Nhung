#include "battery.h"
#include "app_config.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "battery";
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static adc_channel_t s_chan;
static bool s_cali_ok;

/* Đường cong xả Li-ion 3.7V (mV -> %) */
static const struct { uint16_t mv; uint8_t pct; } LUT[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3900, 65}, {3850, 55},
    {3800, 40},  {3750, 25}, {3700, 12}, {3600, 5},  {3300, 0},
};

static uint8_t mv_to_percent(int mv)
{
    const int n = sizeof(LUT) / sizeof(LUT[0]);
    if (mv >= LUT[0].mv) return 100;
    if (mv <= LUT[n - 1].mv) return 0;
    for (int i = 0; i < n - 1; i++) {
        if (mv <= LUT[i].mv && mv > LUT[i + 1].mv) {
            int dv = LUT[i].mv - LUT[i + 1].mv;
            int dp = LUT[i].pct - LUT[i + 1].pct;
            return LUT[i + 1].pct + (mv - LUT[i + 1].mv) * dp / dv;
        }
    }
    return 0;
}

esp_err_t battery_init(void)
{
    adc_unit_t unit;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(PIN_BAT_ADC, &unit, &s_chan));

    adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = unit };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&ucfg, &s_adc));

    adc_oneshot_chan_cfg_t ccfg = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, s_chan, &ccfg));

    adc_cali_curve_fitting_config_t cal = {
        .unit_id = unit, .chan = s_chan,
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    s_cali_ok = (adc_cali_create_scheme_curve_fitting(&cal, &s_cali) == ESP_OK);
    if (!s_cali_ok) ESP_LOGW(TAG, "Không có calibration, dùng công thức gần đúng");
    return ESP_OK;
}

esp_err_t battery_read(uint16_t *mv, uint8_t *percent)
{
    int sum = 0, raw = 0;
    for (int i = 0; i < 16; i++) {
        esp_err_t r = adc_oneshot_read(s_adc, s_chan, &raw);
        if (r != ESP_OK) return r;
        sum += raw;
    }
    raw = sum / 16;
    int pin_mv = 0;
    if (s_cali_ok) adc_cali_raw_to_voltage(s_cali, raw, &pin_mv);
    else           pin_mv = raw * 3100 / 4095;

    int vbat = (int)(pin_mv * BAT_DIVIDER_RATIO);
    *mv = (uint16_t)vbat;
    *percent = mv_to_percent(vbat);
    return ESP_OK;
}
