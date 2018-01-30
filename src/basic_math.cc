#include "basic_math.h"

u32 powof2(u32 val) {
  u32 i = 0;
  while (val >>= 1)
    ++i;
  return i;
}
