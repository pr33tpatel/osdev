#ifndef __OS__MEMORYMANAGEMENT_H
#define __OS__MEMORYMANAGEMENT_H

#include <common/types.h>

namespace os {

struct MemoryChunk {  // NOTE: MemoryChunk stores metadata and the actual size allocated
  // NOTE: sizeof(MemoryChunk) := metadata + size_t size
  MemoryChunk* next;
  MemoryChunk* prev;
  bool allocated;
  common::size_t size;  // NOTE: memory addresses in a 32-bit OS are 32 bits, hence, size_t = uint32_t
  /* NOTE:
      - the metadata is not a part of the size of the MemoryChunk
      - for example: malloc(size_t: 10); // allocate uint32_t * 10 = 320 bytes
      - this will make the "size_t size = 10 (320 bytes)", but the actual sizeof(MemoryChunk) = metadata + size_t size
  */

  /* DIAGRAM:
                         DIAGRAM OF MEMORYCHUNK:

     chunk := [| *next | *prev | bool allocated |     ...  size_t size  ...    |]
     bytes := [|   4   |   4   |       4        |     bytes to be allocated    |]
  */
};

class MemoryManager {
 protected:
  MemoryChunk* first;  // pointer to first MemoryChunk (entire memory space initially)

 public:
  static MemoryManager* activeMemoryManager;

  MemoryManager(common::size_t start, common::size_t size);
  ~MemoryManager();

  // NOTE: void* (void ptr) is a pointer to an object of an unknown size or unspecified data type
  // NOTE: void* is used for malloc() because malloc should work for any data type of any size. the size is given to
  // malloc as an argument
  void* malloc(common::size_t size
  );  // NOTE: returns pointer to an object of *unkown* size (size is passed in as the argument)
  void free(void* ptr);
};

}  // namespace os

void* operator new(unsigned size);
void* operator new[](unsigned size);

// placement new
void* operator new(unsigned size, void* ptr);
void* operator new[](unsigned size, void* ptr);

void operator delete(void* ptr);
void operator delete[](void* ptr);


#endif
