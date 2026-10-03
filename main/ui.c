#include "ui.h"
#include "gfx.h"
#include "app_config.h"
#include <math.h>
#include <stdio.h>

#define CX 120

static const char *DOW_NAME[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void info_circle(int cx, int cy, uint16_t ring, const char *txt)
{
    gfx_ring(cx, cy, 28, 25, ring);
    gfx_text_centered(cx, cy - 7, txt, 2, gfx_rgb(255, 255, 255));
}

/* Cung chấm vàng góc trên-phải: chấm to dần, sáng theo số giây */
static void draw_seconds_arc(int sec)
{
    const int N = 16;
    uint16_t on  = gfx_rgb(240, 220, 0);
    uint16_t off = gfx_rgb(50, 50, 20);
    int lit = ((sec + 1) * N) / 60;
    for (int i = 0; i < N; i++) {
        float ang = (15.0f + 75.0f * i / (N - 1)) * (float)M_PI / 180.0f; /* từ 12h theo chiều kim đồng hồ */
        int x = 120 + (int)lroundf(112.0f * sinf(ang));
        int y = 120 - (int)lroundf(112.0f * cosf(ang));
        int r = 2 + (i * 3) / (N - 1);
        gfx_disc(x, y, r, i < lit ? on : off);
    }
}

void ui_draw(const ui_state_t *s)
{
    char buf[24];
    const uint16_t C_BG    = gfx_rgb(0, 0, 0);
    const uint16_t C_TIME  = gfx_rgb(170, 215, 255);
    const uint16_t C_DATE  = gfx_rgb(240, 220, 0);
    const uint16_t C_GREEN = gfx_rgb(40, 200, 60);
    const uint16_t C_BLUE  = gfx_rgb(40, 140, 255);
    const uint16_t C_WHITE = gfx_rgb(235, 235, 235);
    const uint16_t C_RED   = gfx_rgb(255, 60, 60);

    gfx_fill(C_BG);

    /* --- 3 vòng tròn: nhiệt độ / độ ẩm / pin --- */
    if (s->dht_valid) snprintf(buf, sizeof(buf), "%dC", s->temp_c);
    else              snprintf(buf, sizeof(buf), "--C");
    info_circle(60, 70, C_GREEN, buf);

    if (s->dht_valid) snprintf(buf, sizeof(buf), "%u%%", s->humidity);
    else              snprintf(buf, sizeof(buf), "--%%");
    info_circle(120, 70, C_BLUE, buf);

    if (s->batt_valid) snprintf(buf, sizeof(buf), "%u%%", s->batt_percent);
    else               snprintf(buf, sizeof(buf), "--%%");
    info_circle(180, 70, (s->batt_valid && s->batt_percent <= 20) ? C_RED : C_WHITE, buf);

    /* --- Cung giây --- */
    draw_seconds_arc(s->time_valid ? s->time.sec : 0);

    /* --- Giờ:phút bằng chữ số 7 đoạn --- */
    const int DW = 34, DH = 60, DT = 7, Y0 = 108, X0 = 31;
    int hh = s->time_valid ? s->time.hour : 0;
    int mm = s->time_valid ? s->time.min : 0;
    if (s->time_valid) {
        gfx_digit7(X0,       Y0, DW, DH, DT, hh / 10, C_TIME);
        gfx_digit7(X0 + 42,  Y0, DW, DH, DT, hh % 10, C_TIME);
        gfx_rect(X0 + 85, Y0 + DH / 3 - 4,     8, 8, C_TIME);
        gfx_rect(X0 + 85, Y0 + 2 * DH / 3 - 4, 8, 8, C_TIME);
        gfx_digit7(X0 + 102, Y0, DW, DH, DT, mm / 10, C_TIME);
        gfx_digit7(X0 + 144, Y0, DW, DH, DT, mm % 10, C_TIME);
    } else {
        gfx_text_centered(CX, Y0 + 20, "--:--", 6, C_TIME);
    }

    /* --- Thứ + ngày/tháng/năm --- */
    if (s->time_valid) {
        snprintf(buf, sizeof(buf), "%s %02u/%02u/%04u",
                 DOW_NAME[s->time.dow % 7], s->time.day, s->time.month, s->time.year);
    } else {
        snprintf(buf, sizeof(buf), "--- --/--/----");
    }
    gfx_text_centered(CX, 184, buf, 2, C_DATE);
}
