#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef uint16_t eadk_color_t;
typedef struct { uint16_t x, y; } eadk_point_t;
typedef struct { uint16_t x, y, width, height; } eadk_rect_t;
typedef uint64_t eadk_keyboard_state_t;
void eadk_display_push_rect_uniform(eadk_rect_t rect, eadk_color_t color);
void eadk_display_draw_string(const char * text, eadk_point_t point, bool large_font, eadk_color_t text_color, eadk_color_t background_color);
eadk_keyboard_state_t eadk_keyboard_scan(void);
uint64_t eadk_timing_millis(void);
void eadk_timing_msleep(uint32_t ms);
uint32_t eadk_random(void);
void eadk_backlight_set_brightness(uint8_t b);
