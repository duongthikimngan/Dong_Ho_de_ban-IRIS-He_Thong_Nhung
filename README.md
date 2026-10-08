# ESP32-C3 Clock Display (ESP-IDF, FreeRTOS Task + Queue)

DHT11 + DS3231 + pin Li-ion 3.7V + LCD GC9A01 1.28" 240x240.

## Yêu cầu
- ESP-IDF **v5.2 trở lên** (dùng driver i2c_master, adc_oneshot mới), VSCode + extension *Espressif IDF*
- Lần build đầu cần Internet để tải component `espressif/esp_lcd_gc9a01`

## Đấu nối (sửa trong `main/app_config.h`)
| Module | Chân module | ESP32-C3 |
|---|---|---|
| LCD GC9A01 | SCL | GPIO6 |
| | SDA | GPIO7 |
| | CS | GPIO10 |
| | DC | GPIO4 |
| | RST | GPIO3 |
| | BLK | GPIO1 (hoặc nối 3V3) |
| | VCC/GND | 3V3 / GND |
| DS3231 | SDA / SCL | GPIO5 / GPIO9 |
| | VCC/GND | 3V3 / GND |
| DHT11 | DATA | GPIO2 (pull-up 10k lên 3V3 nếu module chưa có) |
| | VCC/GND | 3V3 / GND |
| Pin đo | Cầu chia áp 100k+100k: B+ -> 100k -> GPIO0 -> 100k -> GND | GPIO0 |
| Nguồn | Pin 3.7V -> mạch sạc TP4056 (OUT+/OUT-) -> chân 5V/VIN (hoặc 3V3 qua LDO) & GND của ESP32-C3 |

Lưu ý: nối GND chung tất cả module. Không đưa 4.2V trực tiếp vào GPIO0.

## Build & nạp
1. Mở thư mục `esp32c3_clock` trong VSCode
2. `ESP-IDF: Set Espressif Device Target` -> **esp32c3**
3. `ESP-IDF: Build, Flash and Monitor` (biểu tượng ngọn lửa)

## Đặt giờ DS3231
Lần đầu (hoặc khi mất pin CR2032), code tự ghi giờ lúc biên dịch. Muốn ép ghi lại: đặt
`RTC_FORCE_SET_FROM_BUILD 1`, nạp, rồi đổi về `0` và nạp lại (tránh reset giờ mỗi lần khởi động).

## Cấu trúc Task/Queue
| Task | Ưu tiên | Vai trò | Chu kỳ |
|---|---|---|---|
| display_task | 5 | Receiver: block 1 s trên queue, vẽ + đẩy LCD | theo tin nhắn |
| rtc_task | 4 | Sender: giờ/ngày | 1 s |
| dht_task | 3 | Sender: nhiệt độ, độ ẩm | 2 s |
| battery_task | 3 | Sender: %pin | 5 s |

Queue chứa `sys_msg_t` (có trường `source` như ví dụ `xData` trong PDF).

## Chỉnh khi màn hình lỗi màu
Trong `lcd_gc9a01.c`: đổi `LCD_RGB_ELEMENT_ORDER_BGR` <-> `_RGB` nếu đỏ/xanh bị đảo; đổi `invert_color(true)` <-> `false` nếu màu âm bản.
