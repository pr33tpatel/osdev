#include <utils/math.h>

using namespace os;
using namespace os::common;

namespace os {
namespace utils {

uint32_t convertToBigEndian(uint32_t _4, uint32_t _3, uint32_t _2, uint32_t _1) {
  uint32_t result_BE = (_4 << 24) | (_3 << 16) | (_2 << 8) | (_1);

  return result_BE;
}

}  // namespace utils
}  // namespace os
