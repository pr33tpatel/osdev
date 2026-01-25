#ifndef __OS__UTILS_H
#define __OS__UTILS_H

#include <common/types.h>
#include <common/vga_color_palette.h>

namespace os{
  namespace common {

    void printf_VGA(const char* str, os::common::uint8_t fg = VGA_COLOR_LIGHT_GRAY, os::common::uint8_t bg = VGA_COLOR_BLACK);
    void clearScreen();
    
  }
}
#endif
