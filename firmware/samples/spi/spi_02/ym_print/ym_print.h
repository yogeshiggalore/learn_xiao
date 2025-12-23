#ifndef YM_PRINT_
#define YM_PRINT_

#include <stdint.h> // For standard integer types
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YM_PRINT_BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define YM_PRINT_BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

#ifdef __cplusplus
}
#endif

#endif // YM_PRINT_