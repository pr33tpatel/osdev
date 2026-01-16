#ifndef __UTILS_H
#define __UTILS_H


#include "types.h"
#include "VGA_COLOR_PALETTE.h"

void printf_VGA(const char* str, uint8_t fg = VGA_COLOR_LIGHT_GRAY, uint8_t bg = VGA_COLOR_BLACK);
void printf_VGA(const char* str, uint8_t fg, uint8_t bg);

void clearScreen();

#endif
