#ifndef __OS__UTILS__STRING_H
#define __OS__UTILS__STRING_H

#include <common/types.h>

namespace os {
namespace utils {

common::uint32_t strlen(const char* str);

int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, common::uint32_t n);

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, common::uint32_t n);

char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, common::uint32_t n);

char* strchr(const char* str, int character);

char* strtok(char* str, const char* delimiters);

char* test(int x, int y, int z, int a);

}  // namespace utils
}  // namespace os

#endif
