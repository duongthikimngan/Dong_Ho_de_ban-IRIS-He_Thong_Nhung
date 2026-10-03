#pragma once
#include "esp_err.h"
#include "sys_msg.h"
esp_err_t ds3231_init(void);
esp_err_t ds3231_get_time(rtc_time_t *t);
esp_err_t ds3231_set_time(const rtc_time_t *t);
