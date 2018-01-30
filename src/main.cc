/*
 * Empty C++ Application
 */

#include "basic_types.h"
#include "mm/frameallocator.h"

extern char* _vector_table;

void kmain(void) __attribute__((section(".boot")));
void kmain(void)
{
  const u32 memstart = 0x100000;
  const u32 memend = (u32)&_vector_table;
  const u32 memsize = memend - memstart;
  FrameInit((void*)memstart, memsize);

  __asm("svc #10");
  while(true) {
    __asm("wfe");
  }

  u32 numLeaks;
  FrameCleanup(numLeaks);
}
