#pragma once
#include <stdint.h>
#include "esp_err.h"

esp_err_t dht22_init(void);
/* temp_x10: nhiệt độ x10 (vd 253 = 25.3 °C), hum_x10: độ ẩm x10 (vd 655 = 65.5 %) */
esp_err_t dht22_read(int16_t *temp_x10, int16_t *hum_x10);