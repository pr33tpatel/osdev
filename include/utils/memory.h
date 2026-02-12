#ifndef __OS__UTILS__MEMORY_H
#define __OS__UTILS__MEMORY_H

#include <common/types.h>

namespace os {
namespace utils {

// moves memory, handles overlapping memory regions, slower, returns destination memory Chunk
void* memmove(void* dest, const void* src, common::size_t n);

// copies memory, faster, but does not handle overlapping memory regions, returns destination memory Chunk
// NOTE: potential to overwrite memory
void* memcpy(void* dest, const void* src, common::size_t n);


// sets a Chunk of memory to a specific value, returns the memory Chunk
void* memset(void* ptr, int value, common::size_t n);


// compares two chunks of memory, return < 0, 0, > 0
// if > 0, ptr1 is greater, if < 0 pt2 is greater, if 0 then ptr1 == ptr2
int memcmp(const void* ptr1, const void* ptr2, common::size_t n);

}  // namespace utils
}  // namespace os

#endif
