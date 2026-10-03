#pragma once
#include "esp_err.h"
#include <stdint.h>
esp_err_t battery_init(void);
/* Trả về mV của pin và % (0..100) */
esp_err_t battery_read(uint16_t *mv, uint8_t *percent);
