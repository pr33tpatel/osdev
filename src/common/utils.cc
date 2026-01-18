#include <common/types.h>
#include <common/utils.h>
#include <common/VGA_COLOR_PALETTE.h>


using namespace os::common;

void printf_VGA(const char* str, uint8_t fg, uint8_t bg){
  uint16_t* VideoMemory = (unsigned short*) 0xb8000;
  static uint8_t x = 0;
  static uint8_t y = 0;

  uint8_t attribute_byte = VGA_MAKE_COLOR(fg, bg); 
 
  for (int i = 0; str[i] != '\0'; i++) {
    // handle \n escape sequence for new lines
    if (str[i] == '\n') {
      x++;
      y = 0;
      continue;
    }

    uint16_t index = x * 80 + y; // set the cursor

    /*
     * so, VideoMemory[index] = (0x07 << 8) | str[i] is kinda weird syntax, but its makes sense
     * so, in standard VGA text, a character cell is represented by two bytes
     * the lower byte is the ASCII code (eg. 'A', 'B', 'Z', ...)
     * the upper byte is for the attributes (color, intensity, etc.)
     *
     *
     * 0x07 (0000 0111) is the value for a light gray color on a black background
     * performing a left shift (<<8) moves the value of 0x07 8 positions to the left, making it the upper byte,
     * so 0x07 << 8 results in: 
     *     upper: (0000 0111)
     *     lower: (0000 0000)
     *
     * now, the lower byte is used to represent the ASCII character
     * 
     * VGA color palette can be seen here: https://www.fountainware.com/EXPL/vga_color_palettes.htm
     *
     */

    VideoMemory[index] = (attribute_byte << 8) | str[i];
    y++;
    
    // line wrap 
    if (y >= 80) {
      y = 0;
      x++;
    }
  }
}

void clearScreen(){
  uint16_t* VideoMemory = (unsigned short*) 0xb8000;
  for (uint8_t row = 0; row < 25; row++){
    for (uint8_t col = 0; col < 80; col++){
      uint16_t index = row * 80 + col;
      VideoMemory[index] = (0x07 << 8) | ' ';
    }
  }
}
