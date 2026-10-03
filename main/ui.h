#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "sys_msg.h"

typedef struct
{
    rtc_time_t time;
    bool time_valid;
    bool dht_valid;
    uint8_t dht_fail; /* số lần đọc DHT lỗi liên tiếp */
    int8_t temp_c;
    uint8_t humidity;
    uint8_t batt_percent;
    bool batt_valid;
} ui_state_t;

/* Vẽ toàn bộ giao diện vào framebuffer (chưa đẩy lên LCD) */
void ui_draw(const ui_state_t *s);