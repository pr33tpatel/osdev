#include <common/types.h>
#include <drivers/terminal.h>
#include <utils/print.h>


using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::hardwarecommunication;


namespace os {
namespace utils {

/* => print functions, color variable */

// void putChar(char c, VGAColor fg, VGAColor bg = BLACK_COLOR);
void putChar(char c, VGAColor fg, VGAColor bg) {
  if (drivers::Terminal::activeTerminal != 0) {
    drivers::Terminal::activeTerminal->PutChar(c, fg, bg);
  }
}


void printNumber(int number, int base, VGAColor fg, VGAColor bg, int width, char paddingChar) {
  char buffer[32];
  int pos = 0;

  if (number == 0) {
    putChar('0', fg, bg);
    return;
  }

  if (number < 0 &&
      base == 10) {  // NOTE: negative numbers only for base10, base16 will only be positive
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
  for (int i = pos - 1; i >= 0; i--) {
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

    switch (fmt[i]) {
      case 'c': {  // character
        char c = (char)va_arg(args, int);
        putChar(c, fg, bg);
        break;
      }

      case 's':  // string
      {
        const char* str = va_arg(args, const char*);
        for (size_t j = 0; str[j] != '\0'; j++) putChar(str[j], fg, bg);

        break;
      }

      case 'i':
      case 'd':  // decimal integer
      {
        int x = va_arg(args, int);
        printNumber(x, 10, fg, bg, width, paddingChar);
        break;
      }

      case 'X':
      case 'x': {  // hexadecimal

        int x = va_arg(args, int);
        // putChar('0', fg, bg); putChar('x', fg, bg); // print "0x" prefix
        // now, with the paddingChar, "%08x" will produce "00001234"
        printNumber(x, 16, fg, bg, width, paddingChar);
        break;
      }

      case '%':  // escaped sequence for %
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

void printf(VGAColor fg, VGAColor bg, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printfInternal(fg, bg, fmt, args);
  va_end(args);
}

void printByte(uint8_t byte, VGAColor fg, VGAColor bg) {
  char* hexSet = "0123456789ABCDEF";

  putChar(hexSet[(byte >> 4) & 0xF], fg, bg);  // print high nibble
  putChar(hexSet[byte & 0xF], fg, bg);         // print low nibble
}

void print4Bytes(uint32_t byte, VGAColor fg, VGAColor bg) {
  // N = 4
  printf("0x");
  printByte(((byte >> 3 * 8) & 0xFF), fg, bg);  // print byte 3
  printByte(((byte >> 2 * 8) & 0xFF), fg, bg);  // print byte 2
  printByte(((byte >> 1 * 8) & 0xFF), fg, bg);  // print byte 1
  printByte(((byte >> 0 * 8) & 0xFF), fg, bg);  // print byte 0
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


/* => conversion functions */

int strToInt(char* str, common::uint16_t base) {
  int num = 0;
  char c = 0;
  uint8_t value = 0;

  for (uint32_t i = 0; str[i] != '\0'; i++) {  // loop through string
    c = str[i];
    // if (c == '-')
    //   value = -value;
    if (c >= '0' && c <= '9')
      value = c - '0';
    else if (c >= 'A' && c <= 'F')
      value = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
      value = c - 'a' + 10;
    else
      continue;  // skip non-numeric chars

    if (value >= base) continue;  // digit cannot exceed the base
    num = num * base + value;
  }

  if (str[0] == '-') num = -num;
  return num;

  /* ALGORITHM: Conversion of string to integer
   * [1] convert the char to an int via offsetting the ascii value by 48 or '0', see ascii table
   * [2] mulitply current result by base (10) and add new int to the result (step 1)
   *
   * example: str = "123"
   * -> 0 * 10 + ('1' - '0') => 0 + (49 - 48) = 1
   * -> 1 * 10 + ('2' - '0') => 10 + (50 - 48) = 12
   * -> 12 * 10 + ('3' - '0') => 120 + (51 - 48) = 123
   *  return result == 123;
   *
   */
}

char* intToStr(int value, char* str, uint32_t base) {
  char* rc = str;
  char* ptr = str;
  char* low;

  // Set constants for digits
  if (base < 2 || base > 36) {
    *str = '\0';
    return str;
  }

  // Convert to string (in reverse)
  do {
    *ptr++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[value % base];
    value /= base;
  } while (value);

  *ptr-- = '\0';  // Null terminate

  // Reverse the string
  low = rc;
  while (low < ptr) {
    char tmp = *low;
    *low++ = *ptr;
    *ptr-- = tmp;
  }
  return rc;
}


int strToInt(char* str) {
  return strToInt(str, 10);
}


/* => miscellaneous functions */

void clearScreen() {
  if (drivers::Terminal::activeTerminal != 0) {
    drivers::Terminal::activeTerminal->Clear();
  }
}

}  // namespace utils
}  // namespace os
