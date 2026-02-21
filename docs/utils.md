# Utils Library

The utils library provides common helper functions and types used across DracOS: strings, memory, printing, math, hashing, and data structures.

- Core modules:
  - [`print`](#print)
  - [`string`](#string)
  - [`memory`](#memory)
  - [`math`](#math)
  - [`hash`](#hash)
- Data structures:
  - See [`ds.md`](ds.md) for `LinkedList`, `HashMap`, `Map`, `Pair`.

---

## Directory layout

- Headers: [`include/utils/`](../include/utils/)
  - [`print.h`](../include/utils/print.h)
  - [`string.h`](../include/utils/string.h)
  - [`memory.h`](../include/utils/memory.h)
  - [`math.h`](../include/utils/math.h)
  - [`hash.h`](../include/utils/hash.h)
  - Data structures in [`include/utils/ds/`](../include/utils/ds/) (documented in [`ds.md`](ds.md)).
- Sources: `src/utils/`
  - `print.cc`, `string.cc`, `memory.cc`, `math.cc` (implementations; layout mirrors headers).

All functions live in the `os::utils` namespace (and `os::utils::ds` for data structures).

---

## print

Header: [`include/utils/print.h`](../include/utils/print.h)  
Implementation: `src/utils/print.cc`

### Purpose

- Provide formatted output (`printf`) and character printing for the text-mode console.
- Handle VGA color attributes and basic integer formatting.
- Offer generic `printType` helpers used by diagnostics and containers.
- Provide simple string–integer conversions and colored ASCII art rendering.

### Types and constants

```cpp
namespace os {
namespace utils {

enum VGAColor {
  BLACK_COLOR = 0,
  BLUE_COLOR = 1,
  GREEN_COLOR = 2,
  CYAN_COLOR = 3,
  RED_COLOR = 4,
  MAGENTA_COLOR = 5,
  BROWN_COLOR = 6,
  LIGHT_GRAY_COLOR = 7,
  DARK_GRAY_COLOR = 8,
  LIGHT_BLUE_COLOR = 9,
  LIGHT_GREEN_COLOR = 10,
  LIGHT_CYAN_COLOR = 11,
  LIGHT_RED_COLOR = 12,
  LIGHT_MAGENTA_COLOR = 13,
  YELLOW_COLOR = 14,
  WHITE_COLOR = 15
};

static const common::uint16_t VGA_WIDTH  = 80;
static const common::uint16_t VGA_HEIGHT = 25;

static inline common::uint8_t vga_color_entry(VGAColor fg, VGAColor bg);
static inline common::uint16_t vga_entry(common::uint8_t ch,
                                         common::uint8_t color = vga_color_entry(LIGHT_GRAY_COLOR, BLACK_COLOR));
```

### Core API

- Character output:

  ```cpp
  void putChar(char c);
  void putChar(char c, VGAColor fg, VGAColor bg = BLACK_COLOR);
  ```

  **Implementation notes:**

  - `putChar(char c, VGAColor fg, VGAColor bg)` forwards to `drivers::Terminal::activeTerminal->PutChar(...)` if an active terminal exists.
  - The no-color `putChar(char c)` uses `LIGHT_GRAY_COLOR` on `BLACK_COLOR` by default.

- Formatted printing:

  ```cpp
  void printf(const char* fmt, ...);
  void printf(VGAColor fg, VGAColor bg, const char* fmt, ...);
  void printfInternal(VGAColor fg, VGAColor bg, const char* fmt, va_list args);
  ```

  **Supported format specifiers (in `printfInternal`)**:

  - `%c` – character.
  - `%s` – C-string.
  - `%d` / `%i` – signed decimal integer.
  - `%x` / `%X` – hexadecimal integer (uppercase hex digits).
  - `%%` – literal `%`.

  **Width and padding:**

  - Optional zero padding: e.g. `%08x`:
    - If the format has a leading `0`, padding is with `'0'`; otherwise `' '`.
    - A numeric width is parsed before the specifier and passed to `printNumber`.

  Unknown specifiers are printed literally as `%` followed by the character.

- Integer formatting:

  ```cpp
  void printNumber(int number, int base,
                   VGAColor fg, VGAColor bg,
                   int width, char paddingChar);
  ```

  **Implementation notes:**

  - Supports bases up to 16.
  - Negative numbers are only handled for base 10; for hex, values are treated as unsigned.
  - Conversion is done into a local buffer in reverse, then printed backwards.

- Byte and word helpers:

  ```cpp
  void printByte(common::uint8_t byte);
  void printByte(common::uint8_t byte, VGAColor fg, VGAColor bg = BLACK_COLOR);

  void print4Bytes(common::uint32_t value);
  void print4Bytes(common::uint32_t value, VGAColor fg, VGAColor bg = BLACK_COLOR);

  void printNBytes(common::uint8_t byte, common::uint8_t N);
  void printNBytes(common::uint8_t byte, common::uint8_t N, VGAColor fg, VGAColor bg = BLACK_COLOR);
  ```

  **Implementation notes:**

  - `printByte` prints two hex digits using `0123456789ABCDEF`.
  - `print4Bytes` prints a `0x` prefix, followed by four bytes (MSB first) via `printByte`.
  - `printNBytes` is currently commented out / incomplete; treat it as experimental.

- ASCII art with inline color codes:

  ```cpp
  void printArt(char* rawTextAscii);
  ```

  **Implementation notes:**

  - Uses `^_` as a prefix to color codes inside the string.
  - After `^_`, it calls `strtok(rawTextAscii + i + 1, " ")` to extract a hex color code (e.g., `"7F"`).
  - Interprets the color code as:
    - High nibble → foreground.
    - Low nibble → background.
  - Updates `fgValue` and `bgValue` and resumes printing with new colors.
  - Because it uses `strtok` and modifies the input buffer, callers must provide a mutable string; do not pass string literals.

- String/integer conversion:

  ```cpp
  int strToInt(char* str);
  int strToInt(char* str, common::uint16_t base);
  char* intToStr(int value, char* str, common::uint32_t base);
  ```

  **Implementation notes:**

  - `strToInt(str, base)`:
    - Accepts digits `0–9`, letters `A–F` / `a–f`.
    - Ignores non-numeric characters.
    - Skips digits that are not valid in the chosen base.
    - Handles a leading `'-'` by negating the final result.
  - `intToStr`:
    - Supports bases 2–36.
    - Converts into the buffer in reverse, then swaps in-place to correct order.
    - Caller must ensure `str` has enough space.

- Generic `printType`:

  ```cpp
  template <typename T>
  void printType(T val) { printf("?unkowntype"); }

  template <> inline void printType<int>(int val) { printf("%d", val); }

  template <> inline void printType<const char*>(const char* val) { printf("%s", val); }

  template <> inline void printType<char*>(char* val) { printf("%s", val); }
  ```

  Extendable by adding specializations as needed.

- Screen clearing:

  ```cpp
  void clearScreen();
  ```

  **Implementation notes:**

  - Delegates to `drivers::Terminal::activeTerminal->Clear()` if available.

**Constraints / invariants**

- All printing ultimately goes through the active `Terminal` driver.
- Functions are synchronous and may be called from interrupt context; avoid adding heap allocations or long-running loops.
- `printArt` modifies its input buffer and uses global `strtok` state; it is not reentrant.

---

## string

Header: [`include/utils/string.h`](../include/utils/string.h)  
Implementation: `src/utils/string.cc`

### Purpose

- Provide minimal, kernel-safe C-style string operations.
- Used by CLI, hashing, printing, and general parsing.

### API

```cpp
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

// temporary/test function
char* test(int x, int y, int z, int a);

}  // namespace utils
}  // namespace os
```

### Implementation notes

- `strlen`:
  - Linear scan until `'\0'`.
- `strcmp` / `strncmp`:
  - Compare byte-by-byte, using `uint8_t` casts for consistent ordering.
  - `strncmp` stops after `n` characters or at the first difference.
- `strcpy` / `strncpy`:
  - `strcpy` copies until `'\0'`, including terminator.
  - `strncpy` copies at most `n` characters from `src` and fills any remaining space with `'\0'`.
- `strcat` / `strncat`:
  - Append to the end of `dest` (found using `strlen`).
  - `strncat` copies at most `n` characters and then terminates with `'\0'`.
- `strchr`:
  - Returns pointer to first occurrence of `character` or `0` if not found.
- `strtok`:
  - Maintains a static `char* sp` as state.
  - If `str` is non-null, sets `sp = str`.
  - Skips leading delimiters.
  - Replaces the delimiter at the end of the token with `'\0'` and updates `sp`.
  - Returns `0` when no more tokens.
  - Not reentrant or thread-safe; stateful like standard `strtok`.

**Constraints / invariants**

- All functions assume valid null-terminated strings.
- Callers must ensure output buffers are large enough.
- `strtok` must not be used concurrently in multiple parsing operations without careful coordination.

---

## memory

Header: [`include/utils/memory.h`](../include/utils/memory.h)  
Implementation: `src/utils/memory.cc`

### Purpose

- Provide raw memory operations similar to C’s `memcpy`/`memmove`/`memset`/`memcmp`.
- Independent of the heap allocator; safe for low-level use.

### API

```cpp
namespace os {
namespace utils {

void* memmove(void* dest, const void* src, common::size_t n);
void* memcpy(void* dest, const void* src, common::size_t n);
void* memset(void* ptr, int value, common::size_t n);
int   memcmp(const void* ptr1, const void* ptr2, common::size_t n);

}  // namespace utils
}  // namespace os
```

### Implementation notes

- `memmove`:
  - Casts to `uint8_t*` and checks `dest < src`.
  - If `dest < src`, copies forward.
  - Else, sets pointers to the end and copies backward.
  - Guarantees correct behavior for overlapping regions.
- `memcpy`:
  - Simple forward copy; does not handle overlap safely.
  - Slightly simpler/faster than `memmove`.
- `memset`:
  - Writes `n` bytes of `(uint8_t)value` into the region.
- `memcmp`:
  - Compares bytes sequentially.
  - Returns:
    - `< 0` if `*p1 < *p2` at first difference.
    - `> 0` if `*p1 > *p2`.
    - `0` if all compared bytes are equal.

**Constraints / invariants**

- None of these functions allocate memory or depend on global state.
- Suitable for use in interrupt handlers and early boot.
- Callers must ensure valid, mapped memory regions.

---

## math

Header: [`include/utils/math.h`](../include/utils/math.h)  
Implementation: `src/utils/math.cc`

### Purpose

- Provide simple math helpers used across subsystems.
- Currently: construct big-endian 32-bit values from 4 octets.

### API

```cpp
namespace os {
namespace utils {

common::uint32_t convertToBigEndian(common::uint32_t _4,
                                    common::uint32_t _3,
                                    common::uint32_t _2,
                                    common::uint32_t _1);

}  // namespace utils
}  // namespace os
```

### Implementation notes

- `convertToBigEndian`:

  ```cpp
  uint32_t convertToBigEndian(uint32_t _4, uint32_t _3,
                              uint32_t _2, uint32_t _1) {
    return (_4 << 24) | (_3 << 16) | (_2 << 8) | _1;
  }
  ```

  - Intended for IPv4 addresses:
    - `_1` = lowest-order octet (e.g. first in dot notation).
    - `_4` = highest-order octet.

**Constraints / invariants**

- No side effects; pure function.
- Can be used anywhere (including early boot and interrupt handlers).

---

## hash

Header: [`include/utils/hash.h`](../include/utils/hash.h)

### Purpose

- Provide generic hashing and equality functions for keys used by `HashMap` and other containers.
- Centralize hash algorithms per type.

### API

```cpp
namespace os {
namespace utils {

template <typename T>
struct Hasher {
  static common::uint32_t Hash(const T& key);
  static inline bool isEqual(const T& a, const T& b);
};

template <>
struct Hasher<const char*> {
  static common::uint32_t Hash(const char* key);
  static inline bool isEqual(const char* a, const char* b);
};

template <>
struct Hasher<char*> {
  static common::uint32_t Hash(char* key);
  static bool isEqual(char* a, char* b);
};

}  // namespace utils
}  // namespace os
```

### Implementation notes

- Default `Hasher<T>` (for scalar types):

  ```cpp
  static common::uint32_t Hash(const T& key) {
    T k = key;
    // Thomas Wang's 32-bit mix
    k = ~k + (k << 15);
    k = k ^ (k >> 12);
    k = k + (k << 2);
    k = k ^ (k >> 4);
    k = k * 2057;
    k = k ^ (k >> 16);
    return k;
  }

  static inline bool isEqual(const T& a, const T& b) { return a == b; }
  ```

  - Intended for integer-like types (e.g. `uint32_t`, `uint64_t`, pointers).
  - Treats the key as a 32-bit value; for wider types, consider adding a specialized hash.

- `Hasher<const char*>`:

  ```cpp
  static common::uint32_t Hash(const char* key) {
    common::uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {
      hash = ((hash << 5) + hash) + c; // djb2
    }
    return hash;
  }

  static inline bool isEqual(const char* a, const char* b) {
    return strcmp(a, b) == 0;
  }
  ```

  - Uses `djb2` string hashing.
  - Equality is via `os::utils::strcmp`.

- `Hasher<char*>`:

  ```cpp
  static common::uint32_t Hash(char* key) {
    return Hasher<const char*>::Hash((const char*)key);
  }
  static bool isEqual(char* a, char* b) {
    return Hasher<const char*>::isEqual((const char*)a, (const char*)b);
  }
  ```

**Constraints / invariants**

- Must be kept in sync with the expectations of containers like `HashMap`:
  - `Hash` and `isEqual` must be consistent (equal keys → equal hashes).
- For new key types:
  - Either rely on default `Hasher<T>` if appropriate.
  - Or add a specialization in `hash.h` that understands the type’s layout.

---

## Data structures

Data structures are defined in `include/utils/ds/` and documented in detail in [`ds.md`](ds.md):

- [`LinkedList<T>`](ds.md#linkedlistt)
- [`HashMap<K, V, MaxSize>`](ds.md#hashmapk-v-maxsize)
- [`Map<K, V, MaxSize>`](ds.md#mapk-v-maxsize)
- [`Pair<K, V>`](ds.md#pairk-v)

They depend on the modules above:

- `HashMap` uses `Hasher<K>` and `LinkedList<HashNode>`.
- Printing functions (`printType`, `printf`) are used by debug helpers like `printList` / `printPairList`.

---

## Usage guidelines

- Put **generic, reusable** helpers here:
  - String, memory, formatting, math, hashing, containers.
- Do **not** put subsystem-specific logic in `utils`:
  - Networking-only helpers belong in `net/`.
  - CLI-only parsing belongs in `cli/`.
- When adding a new helper/module:
  - Declare it in the appropriate `include/utils/*.h`.
  - Implement it in `src/utils/*.cc`.
  - Add a short subsection or bullet here describing:
    - What it does.
    - Important invariants (thread-safety, allocation, side effects).

---

## Future extensions / TODO

- Extend `printType` with more built-in types (e.g. `uint32_t`, `Pair<K,V>`).
- Document supported `printf` format flags more formally (e.g., signed vs unsigned, hex prefixes).
- Add more math helpers (`min`, `max`, `clamp`, etc.) if needed.
- Consider adding reentrant variants of `strtok` or higher-level parsing helpers in a future `utils/parse` module.
```
