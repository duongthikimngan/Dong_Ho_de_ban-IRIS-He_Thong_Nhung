#pragma once
#include <stdint.h>

/* Màu RGB565 đã đảo byte (GC9A01 nhận big-endian) */
static inline uint16_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return __builtin_bswap16(c);
}

void gfx_set_fb(uint16_t *fb);
void gfx_fill(uint16_t color);
void gfx_pixel(int x, int y, uint16_t color);
void gfx_rect(int x, int y, int w, int h, uint16_t color);
void gfx_disc(int cx, int cy, int r, uint16_t color);
void gfx_ring(int cx, int cy, int r_out, int r_in, uint16_t color);
int  gfx_text_width(const char *s, int scale);
void gfx_text(int x, int y, const char *s, int scale, uint16_t color);
void gfx_text_centered(int cx, int y, const char *s, int scale, uint16_t color);
/* Chữ số 7 đoạn, w x h, độ dày t */
void gfx_digit7(int x, int y, int w, int h, int t, int digit, uint16_t color);
