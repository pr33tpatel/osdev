# Memory Management

## Responsibility

- Manage the kernel heap as a contiguous region starting at a given physical/virtual address.
- Provide dynamic allocation (`malloc`/`free`) on top of a simple chunk-based allocator.
- Integrate with C++ `new`/`delete` so all heap allocations go through `MemoryManager`.
- Expose a global `activeMemoryManager` singleton used by the rest of the kernel.

`utils/memory.*` provides low-level helpers (e.g., `memcpy`, `memcmp`) and is conceptually separate from the heap allocator.

---

## Heap Model

- The heap is initialized once at boot with:

  ```cpp
  MemoryManager heap(heapStart, heapSize);
  ```

  in `kernelMain`.

- `heapStart` and `heapSize` are computed based on Multiboot’s upper‑memory info:
  - Heap begins at 10 MiB (see `kernel.cc`).
- The allocator treats the heap as a linked list of variable-sized chunks:

  ```cpp
  struct MemoryChunk {
    bool        allocated;
    size_t      size;   // size of usable payload (not including metadata)
    MemoryChunk* next;
    MemoryChunk* prev;
  };
  ```

- `MemoryManager::first` points to the first chunk covering the entire heap region minus metadata.

### Global singleton

```cpp
MemoryManager* MemoryManager::activeMemoryManager = 0;
```

- The constructor sets:

  ```cpp
  MemoryManager::activeMemoryManager = this;
  ```

- All `new`/`delete` operations go through `activeMemoryManager`.

---

## MemoryManager Construction

```cpp
MemoryManager::MemoryManager(size_t start, size_t size) {
  activeMemoryManager = this;

  if (size < sizeof(MemoryChunk)) {
    first = 0;
  } else {
    first = (MemoryChunk*)start;

    first->allocated = false;
    first->next = 0;
    first->prev = 0;
    first->size = size - sizeof(MemoryChunk);
  }
}
```

- If heap size is too small to hold even one `MemoryChunk`, the allocator is disabled (`first = 0`).
- Otherwise:
  - The first chunk header is placed at `start`.
  - Its payload covers `size - sizeof(MemoryChunk)` bytes.
  - At this point, there is a single large free chunk.

---

## Allocation (`malloc`)

```cpp
void* MemoryManager::malloc(size_t size);
```

### Algorithm

1. Search for the first free chunk that is large enough:

   ```cpp
   MemoryChunk* result = 0;
   for (MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next) {
     if (size <= chunk->size && !chunk->allocated)
       result = chunk;
   }
   ```

   - Complexity: O(n) over the number of chunks.

2. If no suitable chunk is found, return `0`.

3. Otherwise, consider splitting the chunk if it is significantly larger than requested:

   ```cpp
   if (result->size >= size + sizeof(MemoryChunk) + 1) {
     MemoryChunk* remaining =
         (MemoryChunk*)((size_t)result + sizeof(MemoryChunk) + size);

     remaining->allocated = false;
     remaining->size =
         result->size - (size + sizeof(MemoryChunk));
     remaining->prev = result;
     remaining->next = result->next;
     if (remaining->next != 0)
       remaining->next->prev = remaining;

     result->size = size;
     result->next = remaining;
   }
   ```

   - This partitions the original free chunk into:
     - An allocated chunk of exactly `size` bytes.
     - A new free chunk (`remaining`) holding the leftover space.
   - The `+ 1` in the condition adds a minimal buffer between chunks; it prevents splitting when there would be no meaningful remaining space.

4. Mark the chosen chunk as allocated:

   ```cpp
   result->allocated = true;
   ```

5. Return a pointer to the usable payload, not the header:

   ```cpp
   return (void*)((size_t)result + sizeof(MemoryChunk));
   ```

### Notes

- The allocator is **first‑fit**: it picks the first free chunk big enough to satisfy the request.
- There is no alignment logic beyond whatever the initial heap alignment provides; in practice, `sizeof(MemoryChunk)` and `start` typically yield at least word alignment.

---

## Deallocation (`free`)

```cpp
void MemoryManager::free(void* ptr);
```

### Algorithm

1. Ignore `nullptr`:

   ```cpp
   if (ptr == 0) return;
   ```

2. Compute the header address from the payload pointer:

   ```cpp
   MemoryChunk* chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));
   ```

3. Mark the chunk as free:

   ```cpp
   chunk->allocated = false;
   ```

4. Coalesce with previous chunk if it is free:

   ```cpp
   if (chunk->prev != 0 && !chunk->prev->allocated) {
     chunk->prev->next = chunk->next;
     chunk->prev->size += chunk->size + sizeof(MemoryChunk);

     if (chunk->next != 0)
       chunk->next->prev = chunk->prev;

     chunk = chunk->prev;
   }
   ```

5. Coalesce with next chunk if it is free:

   ```cpp
   if (chunk->next != 0 && !chunk->next->allocated) {
     chunk->size += chunk->next->size + sizeof(MemoryChunk);
     chunk->next = chunk->next->next;
     if (chunk->next != 0)
       chunk->next->prev = chunk;
   }
   ```

### Behavior

- `free` aggressively merges adjacent free chunks to reduce fragmentation.
- No checks are performed for double‑free or invalid pointers; misuse leads to undefined behavior.

---

## C++ `new` / `delete` Integration

At the bottom of `memorymanagement.cc`, global `new`/`delete` operators are overridden so that all C++ allocations use the kernel heap.

### Allocation

```cpp
void* operator new(unsigned size) {
  if (os::MemoryManager::activeMemoryManager == 0) return 0;
  return os::MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new[](unsigned size) {
  if (os::MemoryManager::activeMemoryManager == 0) return 0;
  return os::MemoryManager::activeMemoryManager->malloc(size);
}
```

- If no `activeMemoryManager` is set, `new`/`new[]` return `0` (allocation failure).
- Otherwise they delegate to `MemoryManager::malloc`.

### Placement new

```cpp
void* operator new(unsigned size, void* ptr) { return ptr; }
void* operator new[](unsigned size, void* ptr) { return ptr; }
```

- Standard placement new: returns the provided pointer without allocation.

### Deallocation

```cpp
void operator delete(void* ptr) {
  if (os::MemoryManager::activeMemoryManager != 0)
    os::MemoryManager::activeMemoryManager->free(ptr);
}

void operator delete[](void* ptr) {
  if (os::MemoryManager::activeMemoryManager != 0)
    os::MemoryManager::activeMemoryManager->free(ptr);
}

void operator delete(void* ptr, unsigned size) {
  if (os::MemoryManager::activeMemoryManager != 0)
    os::MemoryManager::activeMemoryManager->free(ptr);
}

void operator delete[](void* ptr, unsigned size) {
  if (os::MemoryManager::activeMemoryManager != 0)
    os::MemoryManager::activeMemoryManager->free(ptr);
}
```

- All delete variants simply forward to `MemoryManager::free` if a manager exists.
- `size` parameters are currently unused.

---

## Examples of Use

- Kernel code can use `new`/`delete` directly:

  ```cpp
  Driver* nic = new amd_am79c973(&dev, interrupts);
  delete nic;
  ```

- Networking layer uses dynamic arrays:

  ```cpp
  uint8_t* buffer = new uint8_t[sizeof(InternetProtocolMessage) + payloadSize];
  // ...
  delete[] buffer;
  ```

- Legacy C-style allocations:

  ```cpp
  void* block = MemoryManager::activeMemoryManager->malloc(1024);
  MemoryManager::activeMemoryManager->free(block);
  ```

All of these end up managing memory within the same heap region.

---

## Invariants and Assumptions

- `MemoryManager` is initialized once at boot (in `kernelMain`) and remains active for the lifetime of the kernel.
- Heap region `[heapStart, heapStart + heapSize)` must be mapped and accessible in the current address space.
- All allocations and frees supplied to `MemoryManager` must originate from the same heap region; passing arbitrary pointers to `free` results in undefined behavior.
- The allocator is **not** thread-safe:
  - On a single‑CPU, non‑preemptive kernel, this is acceptable.
  - With preemption or multiple cores, additional locking or disabling interrupts would be required around `malloc`/`free`.
- No guard pages or canaries are implemented; buffer overflows or use‑after‑free bugs may silently corrupt the heap.

---

## Open Questions / TODO

- Consider adding alignment control (e.g., 8/16‑byte alignment) for structures with stricter requirements.
- Improve search strategy (e.g., best‑fit or segregated free lists) to reduce fragmentation and speed up allocation.
- Add basic heap diagnostics:
  - Walk the chunk list and print statistics and fragmentation.
- Add optional debug checks:
  - Poison freed memory.
  - Detect some classes of double‑free / out‑of‑heap pointers in debug builds.
- Clarify virtual vs physical addressing in documentation and tie this allocator more explicitly to the paging model in `memory.md`.
```
