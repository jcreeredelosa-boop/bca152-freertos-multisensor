#pragma once
#include <stdint.h>

void display_init(void);
void display_clear(void);
void display_draw_string(uint8_t page, uint8_t col, const char *str);