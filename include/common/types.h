#ifndef __OS__COMMON__TYPES_H
#define __OS__COMMON__TYPES_H

namespace os {

namespace common {
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int int64_t;
typedef unsigned long long int uint64_t;

typedef const char* string;
typedef uint32_t size_t;  // NOTE: 32-bit OS so memory addresses are 32-bit (uint32_t)
}  // namespace common
}  // namespace os
#endif
