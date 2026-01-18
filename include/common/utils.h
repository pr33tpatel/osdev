#ifndef __UTILS_H
#define __UTILS_H


namespace os{
  namespace common {


#include <common/types.h>
#include <common/VGA_COLOR_PALETTE.h>

void printf_VGA(const char* str, os::common::uint8_t fg = VGA_COLOR_LIGHT_GRAY, os::common::uint8_t bg = VGA_COLOR_BLACK);
void printf_VGA(const char* str, os::common::uint8_t fg, os::common::uint8_t bg);

void clearScreen();

  }
}
#endif
