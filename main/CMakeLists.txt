idf_component_register(
    SRCS "main.c" "dht22.c" "ds3231.c" "battery.c" "lcd_gc9a01.c" "gfx.c" "ui.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc esp_lcd esp_rom freertos log esp_timer
)