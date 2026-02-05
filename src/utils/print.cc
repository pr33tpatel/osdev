#include <common/types.h>
#include <utils/print.h>

using namespace os;
using namespace os::common;
using namespace os::utils;


namespace os    {
namespace utils {

/* static variable to track cursor position */
static uint8_t cursorCol = 0;
static uint8_t cursorRow = 0;
static uint16_t* VideoMemory = (uint16_t*)0xB8000;

/* scrolling function */
static void scrollConsole() {
  /* move all data up by 1 row*/
  for (int y = 0; y < VGA_HEIGHT - 1; y++) 
    for (int x = 0; x < VGA_WIDTH; x++)
      VideoMemory[y * VGA_WIDTH + x] =  VideoMemory[(y+1) * VGA_WIDTH + x];

  /* clear row 24 (the last line) */
  uint16_t space = vga_entry(' ');
  for (int x = 0; x < VGA_WIDTH; x++) 
    VideoMemory[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = space;

  /* reset the cursor back 1 row to the first col */
  cursorRow = VGA_HEIGHT - 1; 
  cursorCol = 0; 
}


void clearScreen() {
  uint8_t colorByte = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR);
  uint16_t space = vga_entry(' ',colorByte);
  
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) 
    VideoMemory[i] = space;

  cursorCol = 0; cursorRow = 0;
}


void putChar(char c, uint8_t color) {
  /* default color:= vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR) */  
  if (c == '\n') {
    cursorCol = 0;
    cursorRow++;
  } 
  else {
    size_t index = cursorRow * VGA_WIDTH + cursorCol;
    VideoMemory[index] = vga_entry(c, color);
    cursorCol++;
  }
  
  /* line wrapping */
  if (cursorCol >= VGA_WIDTH) {
    cursorCol = 0;
    cursorRow++;
  }

  /* scrolling */
  if (cursorRow >= VGA_HEIGHT) {
    scrollConsole();
  }
}

// TODO: add variable arguments to printf such as %d
void printf(const char* str, uint8_t color) {
  for (size_t i = 0; i != '\0'; i++) {
    putChar(str[i], color);
  }
}


void setCursorPos(uint8_t row, uint8_t col) {
  if ((row < VGA_HEIGHT) && (col < VGA_WIDTH)) {
    cursorRow = row; 
    cursorCol = col; 
  }
  else {
    printf("setCursorPos got invalid coordinates\n");
  }
}


} // namespace utils
} // namespace os
