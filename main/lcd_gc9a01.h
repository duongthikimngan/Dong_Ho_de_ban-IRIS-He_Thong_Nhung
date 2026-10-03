#pragma once
#include "esp_err.h"
#include <stdint.h>
/* Khởi tạo LCD, trả về framebuffer RGB565 240x240 (DMA-capable) */
esp_err_t lcd_init(uint16_t **framebuffer);
/* Đẩy toàn bộ framebuffer lên màn hình (chờ DMA xong mới return) */
void lcd_flush(void);
