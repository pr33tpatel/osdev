#ifndef __OS__UTILS__DS__MAP_H
#define __OS__UTILS__DS__MAP_H

#include <common/types.h>

namespace os {
namespace utils {
namespace ds {
template <typename K, typename V, common::uint32_t MaxSize = 512>
class Map {
 private:
  struct Pair {
    K key;
    V value;
    bool active;
  };
  Pair pairs[MaxSize];
  common::uint32_t count;

 public:
  Map() {
    count = 0;
    for (common::uint32_t i = 0; i < MaxSize; i++) {
      pairs[i].active = false;
    }
  }

  ~Map();

  bool put(K key, V value) {
    for (common::uint32_t i = 0; i < MaxSize; i++) {
      if (pairs[i].active && pairs[i].key == key) {
        pairs[i].value = value;
        return true;
      }
    }
    if (count >= MaxSize) return false;

    for (common::uint32_t i = 0; i < MaxSize; i++) {
      if (!pairs[i].active) {
        pairs[i].key = key;
        pairs[i].value = value;
        pairs[i].active = true;
        count++;
        return true;
      }
    }
    return false;
  }

  V get(K key) {
    for (common::uint32_t i = 0; i < MaxSize; i++) {
      if (pairs[i].active && pairs[i].key == key) {
        return pairs[i].value;
      }
    }
    return V();
  }

  bool contains(K key) {
    for (common::uint32_t i = 0; i < MaxSize; i++) {
      if (pairs[i].active && pairs[i].key == key) return true;
    }
    return false;
  }

  Pair* getPair(common::uint32_t index) {
    if (index < 0 || index >= MaxSize) return 0;
    return &pairs[index];
  }

  common::uint32_t size() {
    return count;
  }
  common::uint32_t max_capacity() {
    return MaxSize;
  }
};
}  // namespace ds
}  // namespace utils
}  // namespace os

#endif
