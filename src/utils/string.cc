#include <utils/string.h>

using namespace os;
using namespace os::common;
using namespace os::utils;

namespace os {
namespace utils {


uint32_t strlen(const char* str) {
  uint32_t len = 0;
  while (str[len] != 0) len++;
  return len;
}

// return < 0, 0, or > 0, based on how str1 compares to str2
int strcmp(const char* str1, const char* str2) {
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const uint8_t*)str1 - *(const uint8_t*)str2;
}


int strncmp(const char* str1, const char* str2, uint32_t n) {
  while (n > 0 && *str1 && (*str1 == *str2)) {
    str1++;
    str2++;
    n--;
  }
  if (n == 0) return 0;
  return *(const uint8_t*)str1 - *(const uint8_t*)str2;
}


// copy data from src to destination
char* strcpy(char* dest, const char* src) {
  char* og_dest = dest;
  while ((*dest++ = *src++));  // copy all elements of src to dest
  return og_dest;
}


char* strncpy(char* dest, const char* src, uint32_t n) {
  char* og_dest = dest;
  while (n > 0 && *src != '\0') {
    *dest++ = *src++;
    n--;
  }
  while (n > 0) {
    *dest++ = '\0';  // fill remaining with zeros
    n--;
  }

  return og_dest;
}


// append additional data to destination
char* strcat(char* dest, const char* src) {
  char* ptr = dest + strlen(dest);
  while (*src != '\0') *ptr++ = *src++;
  *ptr = '\0';
  return dest;
}

char* strncat(char* dest, const char* src, uint32_t n) {
  char* ptr = dest + strlen(dest);
  while (n > 0 && *src != '\0') {
    *ptr++ = *src++;
    n--;
  }
  *ptr = '\0';
  return dest;
}

// find and return first occurence specific character in string
char* strchr(const char* str, int character) {
  while (*str != (char)character) {
    if (!*str++)
      return 0;  // (character not found) => if you parse the entire str and request character does not
                 // appear, return 0,
  }
  return (char*)str;
}


// return pointer to the first token found string, 0 if no tokens
static char* sp = 0;  // tokenizer state

char* strtok(char* str, const char* delimiters) {
  if (str) sp = str;
  if (!sp) return 0;

  while (*sp && strchr(delimiters, *sp)) sp++;  // skip leading delimiters

  if (!*sp) {
    sp = 0;
    return 0;
  }

  char* ret = sp;
  while (*sp) {
    if (strchr(delimiters, *sp)) {
      *sp = 0;
      sp++;
      return ret;
    }
    sp++;
  }

  sp = 0;
  return ret;
}


}  // namespace utils
}  // namespace os
