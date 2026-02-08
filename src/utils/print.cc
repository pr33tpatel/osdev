#include <common/types.h>
#include <utils/print.h>

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::hardwarecommunication;


namespace os    {
namespace utils {

/* static variable to track cursor position */
static uint8_t cursorCol = 0;
static uint8_t cursorRow = 0;
static uint16_t* VideoMemory = (uint16_t*)0xB8000;

static Port8Bit vgaIndexPort(0x3D4);
static Port8Bit vgaDataPort(0x3D5);

static void updateCursor() {
  uint16_t position = cursorRow * VGA_WIDTH + cursorCol;
  
  // set high byte for cursor position 
  vgaIndexPort.Write(0x0E);
  vgaDataPort.Write((uint8_t)(position >> 8) & 0xFF);

  // set low byte
  vgaIndexPort.Write(0x0F);
  vgaDataPort.Write((uint8_t)(position & 0xFF));
}

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


/* => print functions, color variable */

// void putChar(char c, VGAColor fg, VGAColor bg = BLACK_COLOR);
void putChar(char c, VGAColor fg, VGAColor bg) {
  uint8_t color = vga_color_entry(fg, bg);
  /* default color:= vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR) */  
  if (c == '\n') { // new line
    cursorCol = 0;
    cursorRow++;
  } 

  else if (c == '\b') { // backspace
    if (cursorCol > 0) {
      cursorCol--;
    }

    else if (cursorCol <= 0) {
      cursorCol = VGA_WIDTH - 1;
      cursorRow--; 
    }

      // overwrite the character with blank
      size_t index = cursorRow * VGA_WIDTH + cursorCol;
      VideoMemory[index] = vga_entry(' ', color);
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

  updateCursor();
}


void printNumber(int number, int base, VGAColor fg, VGAColor bg, int width, char paddingChar) {
  char buffer[32];
  int pos = 0;

  if (number == 0) {
    putChar('0', fg, bg);
    return;
  }

  if (number < 0 && base == 10) { // NOTE: negative numbers only for base10, base16 will only be positive
    putChar('-', fg, bg);
    number = -number;
  }

  // convert number to string
  unsigned int uNumber = (unsigned int)number;

  const char* digits = "0123456789ABCDEF";

  while (uNumber > 0) {
    buffer[pos++] = digits[uNumber % base];
    uNumber /= base;
  }

  while (pos < width) {
    putChar(paddingChar, fg, bg);
    width--;
  }

  
  // print the buffer in reverse
  for (int i = pos-1; i >= 0; i--) {
    putChar(buffer[i], fg, bg);
  }
}

void printfInternal(VGAColor fg, VGAColor bg, const char* fmt, va_list args) {
  for (size_t i = 0; fmt[i] != '\0'; i++) {
    
    // if not % specifier, print the value
    if (fmt[i] != '%') {
      putChar(fmt[i], fg, bg);
      continue;
    }

    // if % has been found, look at the next character
    i++;
    
    int width = 0;
    char paddingChar = ' ';

    if (fmt[i] == '0') {
      paddingChar = '0';
      i++;
    }

    while (fmt[i] >= '0' && fmt[i] <= '9') {
      width = width * 10 + (fmt[i] - '0');
      i++;
    }

    switch (fmt[i]) 
    {
      case 'c':  // character
        {
          char c = (char) va_arg(args, int);
          putChar(c, fg, bg);
          break;
        }

      case 's':  // string
        {
          const char* str = va_arg(args, const char*);
          for (size_t j = 0; str[j] != '\0'; j++)
            putChar(str[j], fg, bg);

          break;
        }

      case 'i':
      case 'd': // decimal integer 
        {
          int x = va_arg(args, int);
          printNumber(x, 10, fg, bg, width, paddingChar);
          break;
        }

      case 'X':
      case 'x': // hexadecimal
        {
          int x = va_arg(args,int);
          // putChar('0', fg, bg); putChar('x', fg, bg); // print "0x" prefix
          // now, with the paddingChar, "%08x" will produce "00001234"
          printNumber(x, 16, fg, bg, width, paddingChar);
          break;
        }

      case '%': // escaped sequence for %
        {
          putChar('%', fg, bg);
          break;
        }

      default:  // unknown, so just print specifier (e.g."... %q ...")
        {
          putChar('%', fg, bg);
          putChar(fmt[i], fg, bg);
          break;
        }
    }
  }
}

void printf(VGAColor fg , VGAColor bg, const char* fmt, ...){
  va_list args;
  va_start(args, fmt);
  printfInternal(fg, bg, fmt, args);
  va_end(args);
}

void printByte(uint8_t byte, VGAColor fg, VGAColor bg) {
  char* hexSet = "0123456789ABCDEF";

  putChar(hexSet[(byte >> 4) & 0xF], fg, bg); // print high nibble
  putChar(hexSet[byte & 0xF], fg, bg); // print low nibble
}

void print4Bytes(uint32_t byte, VGAColor fg, VGAColor bg) {
  // N = 4
  printf("0x");
  printByte(((byte >> 3*8) & 0xFF), fg, bg); // print byte 3
  printByte(((byte >> 2*8) & 0xFF), fg, bg); // print byte 2
  printByte(((byte >> 1*8) & 0xFF), fg, bg); // print byte 1
  printByte(((byte >> 0*8) & 0xFF), fg, bg); // print byte 0
}

// FIXME: fix this, was casuing infintie loop i think
void printNBytes(uint8_t byte, uint8_t N) {
  // printf("0x");
  // for (uint8_t i = N-1 ; i >= 0; i--)  {
  //   printByte((byte >> (i*8)) & 0xFF);
  //   N--;
  // }
}


/* => default print functions, no color */

// NOTE: the color in the these functions is the default color for the functions 
void putChar(char c) {
  putChar(c, LIGHT_GRAY_COLOR, BLACK_COLOR); 
}

void printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printfInternal(LIGHT_GRAY_COLOR, BLACK_COLOR, fmt, args);
  va_end(args);
}

void printByte(uint8_t byte) {
  printByte(byte, LIGHT_GRAY_COLOR, BLACK_COLOR);
}

void print4Bytes(uint32_t byte) {
  print4Bytes(byte, LIGHT_GRAY_COLOR, BLACK_COLOR);
}


/* => miscellaneous functions */

void clearScreen() {
  uint8_t colorByte = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR);
  uint16_t space = vga_entry(' ',colorByte);
  
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) 
    VideoMemory[i] = space;

  cursorCol = 0; cursorRow = 0;

  enableCursor(1,15);

  updateCursor();
}

void enableCursor(uint8_t cursorStart, uint8_t cursorEnd) {

  vgaIndexPort.Write(0x0A);
  uint8_t start_val = vgaDataPort.Read();
  vgaIndexPort.Write(0x0A);
  vgaDataPort.Write((start_val & 0xC0) | cursorStart);

  vgaIndexPort.Write(0x0B);
  uint8_t end_val = vgaDataPort.Read();
  vgaIndexPort.Write(0x0B);
  vgaDataPort.Write((end_val & 0xE0) | cursorEnd);
}

void setCursorPos(uint8_t row, uint8_t col) {
  if ((row < VGA_HEIGHT) && (col < VGA_WIDTH)) {
    cursorRow = row; 
    cursorCol = col; 
    updateCursor();
  }
  else {
    printf("setCursorPos got invalid coordinates\n");
  }
}


} // namespace utils
} // namespace os
