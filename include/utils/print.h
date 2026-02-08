#ifndef __OS__UTILS__PRINT_H
#define __OS__UTILS__PRINT_H

#include <common/types.h>
#include <hardwarecommunication/port.h>
#include <stdarg.h>

namespace os {
  namespace utils {
    
    // VGA colors
    enum VGAColor {
      BLACK_COLOR           =   0,
      BLUE_COLOR            =   1,
      GREEN_COLOR           =   2,
      CYAN_COLOR            =   3,
      RED_COLOR             =   4,
      MAGENTA_COLOR         =   5,
      BROWN_COLOR           =   6,
      LIGHT_GRAY_COLOR      =   7,
      DARK_GRAY_COLOR       =   8,
      LIGHT_BLUE_COLOR      =   9,
      LIGHT_GREEN_COLOR     =  10,
      LIGHT_CYAN_COLOR      =  11,
      LIGHT_RED_COLOR       =  12,
      LIGHT_MAGENTA_COLOR   =  13,
      YELLOW_COLOR          =  14,
      WHITE_COLOR           =  15
    };

    /* screen dimensions */
    static const common::uint16_t VGA_WIDTH  = 80;
    static const common::uint16_t VGA_HEIGHT = 25;

    /* helper to combine foreground (bottom 4 bits) and background (top 4 bits) color */
    static inline common::uint8_t vga_color_entry(enum VGAColor fg, enum VGAColor bg) {
      return (os::common::uint8_t) fg | ((os::common::uint8_t)bg << 4);
    }

    /* helper to combine character byte and color attribute byte */
    static inline common::uint16_t vga_entry(common::uint8_t char_, common::uint8_t color = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR)) {
      return (os::common::uint16_t) char_ | ((os::common::uint8_t) color << 8);
    }


    
    /* standard print functions */
    // void putChar(char c, common::uint8_t color = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR));
    void putChar(char c);
    void putChar(char c, VGAColor fg, VGAColor bg = BLACK_COLOR);

    void printf(const char* fmt, ...);
    void printf(VGAColor fg , VGAColor bg, const char* fmt, ...);
    void printfInternal(VGAColor fg, VGAColor bg, const char* fmt, va_list args);

    void printNumber(int number, int base, VGAColor fg, VGAColor bg, int width, char paddingChar);

    void printByte(common::uint8_t byte);
    void printByte(common::uint8_t byte, VGAColor fg, VGAColor bg = BLACK_COLOR);

    void print4Bytes(common::uint32_t byte);
    void print4Bytes(common::uint32_t byte, VGAColor fg, VGAColor bg = BLACK_COLOR);

    void printNBytes(common::uint8_t byte, common::uint8_t N);
    void printNBytes(common::uint8_t byte, common::uint8_t N, VGAColor fg, VGAColor bg = BLACK_COLOR);

    /* miscellaneous functions */
    // static void scrollConsole();
    void clearScreen();
    void enableCursor(common::uint8_t cursorStart, common::uint8_t cursorEnd);
    void setCursorPos(common::uint8_t row, common::uint8_t col);
  }
}

#endif
