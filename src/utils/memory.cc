#include <system_error>
#include <utils/memory.h>

using namespace os;
using namespace os::common;
using namespace os::utils;

namespace os    {
namespace utils {
// moves memory, handles overlapping memory regions, slower, returns destination memory Chunk
void* memmove(void* dest, const void* src, common::size_t n) {
  uint8_t* d = (uint8_t*) dest;
  const uint8_t* s = (const uint8_t*) src;
  if (d < s) {
    while (n--) 
      *d++ = *s++;
  } else {

    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }
  return dest;
  
}


// copies memory, faster, but does not handle overlapping memory regions, returns destination memory Chunk
// NOTE: potential to overwrite memory
void* memcpy(void* dest, const void* src, common::size_t n) {
  uint8_t* d = (uint8_t*) dest;
  const uint8_t* s = (const uint8_t*) src;
  while (n--)
    *d++ = *s++;
  return dest;
}

/**
 * @brief [memset]
 * @param ptr [voidptr ptr]
 * @param value [value]
 */
void* memset(void* ptr, int value, common::size_t n) {
  uint8_t* p = (uint8_t*)ptr;
  while (n--)
    *p++ = (uint8_t) value;
  return ptr;
}





/**
 * @brief [compares two chunks of memory]
 * @param ptr1 [voidptr: memChunk 1]
 * @param ptr2 [voidptr: memChunk 2]
 * @param n [size_t: how many bytes to compare]
 * @return [return int: < 0, 0, > 0]
 */
int memcmp(const void* ptr1, const void* ptr2, common::size_t n) {
  const uint8_t* p1 = (const uint8_t*) ptr1;
  const uint8_t* p2 = (const uint8_t*) ptr2;
  while (n--) {
    if (*p1 != *p2)
      return *p1 - *p2; 
    p1++;
    p2++;
  }
  return 0;
}




} // namespace utils
} // namespace os
