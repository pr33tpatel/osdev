#ifndef __OS__UTILS__DS__PAIR_H
#define __OS__UTILS__DS__PAIR_H

namespace os {
namespace utils {
namespace ds {
template <typename K, typename V>
struct Pair {
  K key;
  V value;
};
}  // namespace ds
}  // namespace utils
}  // namespace os

#endif
