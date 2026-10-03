#pragma once
/* ===================== CẤU HÌNH CHÂN / HẰNG SỐ ===================== */

/* --- LCD GC9A01 (SPI2) --- */
#define LCD_HOST SPI2_HOST
#define PIN_LCD_SCK 6  /* SCL trên module */
#define PIN_LCD_MOSI 7 /* SDA trên module */
#define PIN_LCD_CS 10
#define PIN_LCD_DC 4
#define PIN_LCD_RST 3
#define PIN_LCD_BLK 1 /* backlight (có thể nối thẳng 3V3) */
#define LCD_W 240
#define LCD_H 240
#define LCD_SPI_HZ (40 * 1000 * 1000)
#define LCD_STRIP_LINES 40 /* đẩy ảnh theo từng dải 40 dòng */

/* --- I2C cho DS3231 --- */
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 5
#define DS3231_ADDR 0x68

/* --- DHT22 --- */
#define PIN_DHT22 2

/* --- Đo pin: GPIO0 = ADC1_CH0, qua cầu chia áp 100k/100k --- */
#define PIN_BAT_ADC 0
#define BAT_DIVIDER_RATIO 2.0f

/* --- RTC --- */
/* 1 = ép ghi giờ lúc biên dịch vào DS3231 (nạp 1 lần rồi đổi về 0 và nạp lại) */
#define RTC_FORCE_SET_FROM_BUILD 0

/* --- Queue --- */
#define SYS_QUEUE_LENGTH 10
