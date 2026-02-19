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
    T k = key;  // create a copy for modification
    k = ~k + (k << 15);
    k = k ^ (k >> 12);
    k = k + (k << 2);
    k = k ^ (k >> 4);
    k = k * 2057;
    k = k ^ (k >> 16);
    return k;
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
