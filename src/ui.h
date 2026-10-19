#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include <stdint.h>
void ui_begin(void);
void ui_page(const char *title);
uint8_t ui_wait_key(void);
int ui_digit_key(uint8_t key);
bool ui_parse_number(const char *text,int32_t *value);
void ui_wrap(const char *text,int y,int last_y);
#endif
