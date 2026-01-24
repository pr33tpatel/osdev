#ifndef __OS__MEMORYMANAGEMENT_H
#define __OS__MEMORYMANAGEMENT_H

#include <common/types.h>

namespace os {

  struct MemoryChunk {
    MemoryChunk* next;
    MemoryChunk* prev;
    bool allocated;
    common::size_t size; // memory

  };

  class MemoryManager {

  };

}

#endif
