#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t sec, min, hour;
    uint8_t dow; /* 0 = CN(SUN) ... 6 = T7(SAT) */
    uint8_t day, month;
    uint16_t year;
} rtc_time_t;

/* Giống ví dụ "xData" trong PDF: kèm mã nguồn để bên nhận biết dữ liệu từ đâu */
typedef enum
{
    SRC_DHT22 = 1,
    SRC_RTC,
    SRC_BATTERY,
} msg_source_t;

typedef struct
{
    msg_source_t source;
    union
    {
        struct
        {
            int8_t temp_c;
            uint8_t humidity;
            bool valid;
        } dht;
        rtc_time_t time;
        struct
        {
            uint8_t percent;
            uint16_t mv;
        } batt;
    } data;
} sys_msg_t;
