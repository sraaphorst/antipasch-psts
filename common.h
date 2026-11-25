#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

/* One bit per point; unsigned shifts are defined even for the top bit. */
typedef uint64_t point_bits;
enum { POINT_BITS = 64 };

static inline int gbit(point_bits value, int pos)
{
  return (int)((value >> (POINT_BITS - 1 - pos)) & UINT64_C(1));
}

static inline void set_bit(point_bits *value, int pos, int enabled)
{
  point_bits mask = UINT64_C(1) << (POINT_BITS - 1 - pos);
  if (enabled)
    *value |= mask;
  else
    *value &= ~mask;
}
#define sbit(value, pos, enabled) set_bit(&(value), (pos), (enabled))

#endif
