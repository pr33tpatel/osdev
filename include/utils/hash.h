#ifndef __OS__UTILS__HASH_H
#define __OS__UTILS__HASH_H

#include <common/types.h>
#include <utils/string.h>

namespace os {
namespace utils {

// default used for scalar types (int, long, void*)
template <typename T>
struct Hasher {
  // ALGORITHM: Thomas Wang's 32-bit mix hashing algorithm
  static common::uint32_t Hash(const T& key) {
    key = ~key + (key << 15);
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057;
    key = key ^ (key >> 16);
    return key;
  }

  static bool isEqual(const T& a, const T& b) {
    return a == b;
  };
};

// hasher for const strings
template <>
struct Hasher<const char*> {
  // ALGORITHM: djb2 hashing algorithm
  static common::uint32_t Hash(const char* key) {
    common::uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {              // iterate until the end of the string
      hash = ((hash << 5) + hash) + c;  // ALGORITHM: (hash * 32) + hash + c => hash * 33 + c
    }
    return hash;
  }

  static bool isEqual(const char* a, const char* b) {
    return strcmp(a, b) == 0;
  }
};


template <>
struct Hasher<char*> {
  // ALGORITHM: djb2 hashing algorithm
  static common::uint32_t Hash(char* key) {
    return Hasher<const char*>::Hash((const char*)key);
  }
  static bool isEqual(char* a, char* b) {
    return Hasher<const char*>::isEqual((const char*)a, (const char*)b);
  }
};
}  // namespace utils
}  // namespace os

#endif
