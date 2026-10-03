#include "gfx.h"
#include "app_config.h"
#include <string.h>
#include <ctype.h>

static uint16_t *s_fb;

void gfx_set_fb(uint16_t *fb) { s_fb = fb; }

void gfx_fill(uint16_t color)
{
    for (int i = 0; i < LCD_W * LCD_H; i++) s_fb[i] = color;
}

void gfx_pixel(int x, int y, uint16_t color)
{
    if ((unsigned)x < LCD_W && (unsigned)y < LCD_H) s_fb[y * LCD_W + x] = color;
}

void gfx_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            gfx_pixel(i, j, color);
}

void gfx_disc(int cx, int cy, int r, uint16_t color)
{
    int r2 = r * r;
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r2) gfx_pixel(cx + x, cy + y, color);
}

void gfx_ring(int cx, int cy, int r_out, int r_in, uint16_t color)
{
    int ro2 = r_out * r_out, ri2 = r_in * r_in;
    for (int y = -r_out; y <= r_out; y++)
        for (int x = -r_out; x <= r_out; x++) {
            int d = x * x + y * y;
            if (d <= ro2 && d >= ri2) gfx_pixel(cx + x, cy + y, color);
        }
}

/* ---------- Font 5x7 (mỗi cột 1 byte, bit0 = hàng trên cùng) ---------- */
typedef struct { char c; uint8_t col[5]; } glyph_t;
static const glyph_t FONT[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'-', {0x08,0x08,0x08,0x08,0x08}},
    {'.', {0x00,0x60,0x60,0x00,0x00}},
    {'/', {0x20,0x10,0x08,0x04,0x02}},
    {'%', {0x23,0x13,0x08,0x64,0x62}},
    {':', {0x00,0x36,0x36,0x00,0x00}},
    {'0', {0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}},
    {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x06,0x49,0x49,0x29,0x1E}},
    {'A', {0x7E,0x11,0x11,0x11,0x7E}},
    {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'D', {0x7F,0x41,0x41,0x22,0x1C}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'G', {0x3E,0x41,0x49,0x49,0x7A}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},
    {'J', {0x20,0x40,0x41,0x3F,0x01}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'Q', {0x3E,0x41,0x51,0x21,0x5E}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}},
    {'V', {0x1F,0x20,0x40,0x20,0x1F}},
    {'W', {0x3F,0x40,0x38,0x40,0x3F}},
    {'X', {0x63,0x14,0x08,0x14,0x63}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}},
    {'Z', {0x61,0x51,0x49,0x45,0x43}},
};

static const uint8_t *find_glyph(char c)
{
    c = (char)toupper((unsigned char)c);
    for (unsigned i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++)
        if (FONT[i].c == c) return FONT[i].col;
    return FONT[0].col;
}

int gfx_text_width(const char *s, int scale)
{
    int n = (int)strlen(s);
    return n ? n * 6 * scale - scale : 0;
}

void gfx_text(int x, int y, const char *s, int scale, uint16_t color)
{
    for (; *s; s++, x += 6 * scale) {
        const uint8_t *g = find_glyph(*s);
        for (int cx = 0; cx < 5; cx++)
            for (int cy = 0; cy < 7; cy++)
                if (g[cx] & (1 << cy))
                    gfx_rect(x + cx * scale, y + cy * scale, scale, scale, color);
    }
}

void gfx_text_centered(int cx, int y, const char *s, int scale, uint16_t color)
{
    gfx_text(cx - gfx_text_width(s, scale) / 2, y, s, scale, color);
}

/* ---------- 7 đoạn ---------- */
/* bit: a=1 b=2 c=4 d=8 e=16 f=32 g=64 */
static const uint8_t SEG[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};

void gfx_digit7(int x, int y, int w, int h, int t, int digit, uint16_t color)
{
    if (digit < 0 || digit > 9) return;
    uint8_t s = SEG[digit];
    int mid = (h - t) / 2;
    if (s & 0x01) gfx_rect(x + t, y, w - 2 * t, t, color);                       /* a */
    if (s & 0x02) gfx_rect(x + w - t, y + t, t, mid - t, color);                 /* b */
    if (s & 0x04) gfx_rect(x + w - t, y + mid + t, t, h - t - (mid + t), color); /* c */
    if (s & 0x08) gfx_rect(x + t, y + h - t, w - 2 * t, t, color);               /* d */
    if (s & 0x10) gfx_rect(x, y + mid + t, t, h - t - (mid + t), color);         /* e */
    if (s & 0x20) gfx_rect(x, y + t, t, mid - t, color);                         /* f */
    if (s & 0x40) gfx_rect(x + t, y + mid, w - 2 * t, t, color);                 /* g */
    /* nối các đoạn dọc với đoạn ngang để nét liền */
    if (s & 0x02) gfx_rect(x + w - t, y, t, t, color);
    if (s & 0x20) gfx_rect(x, y, t, t, color);
    if (s & 0x04) gfx_rect(x + w - t, y + mid, t, t, color);
    if (s & 0x10) gfx_rect(x, y + mid, t, t, color);
    if (s & 0x04) gfx_rect(x + w - t, y + h - t, t, t, color);
    if (s & 0x10) gfx_rect(x, y + h - t, t, t, color);
}
