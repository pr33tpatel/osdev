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


/* => print functions */

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


void printByte(uint8_t byte) {
  char* hexChar = "00"; // "00" are default placeholder values 
  char* hexSet = "0123456789ABCDEF";

  hexChar[0] = hexSet[(byte >> 4) & 0xF]; 
  hexChar[1] = hexSet[byte & 0xF];
  printf(hexChar);

  /* DIAGRAM: Printing a byte in hexademcial

     example byte: 23_10 => 0x17=> 0b 0001 0111

     hexChar[0]: (byte >> 4) & 0xF := 0000 0001 
                                    & 0000 1111 = 0000 0001 => 0x1 => '1' 

     hexChar[1]: (byte) & 0xF      := 0001 0111
                                    & 0000 1111 = 0000 0111 => 0x7 => '7'

     char* hexChar := ['1','7']
      
  */
}


void print4Bytes(uint8_t byte) {
  // N = 4
  printf("0x");
  printByte((byte >> 3*8) & 0xFF); // print byte 3
  printByte((byte >> 2*8) & 0xFF); // print byte 2
  printByte((byte >> 1*8) & 0xFF); // print byte 1
  printByte((byte >> 0*8) & 0xFF); // print byte 0
}

void printNBytes(uint8_t byte, uint8_t N) {
  printf("0x");
  for (uint8_t i = N-1 ; i >= 0; i--)  {
    printByte((byte >> (i*8)) & 0xFF);
    N--;
  }
}


/* => miscellaneous functions */

void clearScreen() {
  uint8_t colorByte = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR);
  uint16_t space = vga_entry(' ',colorByte);
  
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) 
    VideoMemory[i] = space;

  cursorCol = 0; cursorRow = 0;
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
