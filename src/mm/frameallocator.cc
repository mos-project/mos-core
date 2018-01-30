/*
 * frameallocator.cc
 *
 *  Created on: 30 jan. 2018
 *      Author: micke
 */

#include "frameallocator.h"
#include "basic_math.h"

#if DEBUG_LEVEL >= 2
#include <iostream>
#endif

#define NUM_LEVELS              19

static constexpr u32 pagesize = 4096;

struct FrameBlock {
  FrameBlock * nextFree;
  u32 size;
  bool free;
};
FrameBlock _blocks[262144]; // Room for 1 GB memory range. (1024*1024*1024) / pagesize
FrameBlock * _startblock;
FrameBlock * _freeblocks[NUM_LEVELS];
u8* _firstpage;
u32 _memsize;
u32 _allocatedblocks;

#define LEVEL(x)		powof2((x) / pagesize)

static void addBlock(FrameBlock* block, u32 lvl) {
  if (!block) return;
  
  // set this block as the first free block
  block->nextFree = _freeblocks[lvl];
  _freeblocks[lvl] = block;
}

static void removeBlock(FrameBlock* block, u32 lvl) {
  if (!block) return;
  
  auto tmp = _freeblocks[lvl];
  
  if (block == tmp) {
    // First (or only) block in chain
    _freeblocks[lvl] = block->nextFree;
    return;
  }

  while (tmp) {
    if (tmp->nextFree == block)
      tmp->nextFree = block->nextFree; // unlink block
    tmp = tmp->nextFree;
  }
}

// split a block in two equal sized halves
static FrameBlock* split(FrameBlock* block, int lvl) {
  u32 newsize = block->size / 2;
  
  // remove current block from the chain
  // make sure caller adds it back if not used
  removeBlock(block, lvl);
  
  block->free = true;
  block->size = newsize;
  
  auto buddy = block + (newsize / pagesize);
  buddy->free = true;
  buddy->size = newsize;
  
  addBlock(buddy, lvl - 1);

#if DEBUG_LEVEL >= 2
  auto pagePtr = _firstpage + (block - _blocks) * pagesize;
  std::cout << "Split block at 0x" << std::hex << (void*)pagePtr << "\tsize = 2x 0x" << std::hex << block->size << std::endl;
#endif

  return block;
}

static FrameBlock* findbuddy(FrameBlock* block, int lvl) {
  u32 blockIdx = (block - _blocks);
  blockIdx &= (lvl + 1);
  blockIdx >>= lvl;
  
  u32 offset = (1 << (lvl));
  if (blockIdx & 1) {
    blockIdx = (block - _blocks) - offset;
  }
  else {
    blockIdx = (block - _blocks) + offset;
  }
  
  return _blocks + blockIdx;
}

static FrameBlock* merge(FrameBlock* block) {
  u32 lvl = LEVEL(block->size);
  auto buddy = findbuddy(block, lvl);
  
  if (!buddy->free || buddy->size != block->size)
    return nullptr;
  
  if (block > buddy) {
    auto tmp = block;
    block = buddy;
    buddy = tmp;
  }
  
  removeBlock(block, lvl);
  removeBlock(buddy, lvl);
  
  block->size *= 2;
  block->free = true;
  
  addBlock(block, lvl + 1);

#if DEBUG_EVEL >= 2
  std::cout << "Merged two blocks starting at 0x" << std::hex << (void*)pagePtr << "\tnew size = 0x" << std::hex << block->size << std::endl;
  auto pagePtr = _firstpage + (block - _blocks) * pagesize;
#endif

  return block;
}


void FrameInit(void* memstart, u32 memsize) {
  memsize &= -0x1000; // round down to multiple of 4KB
  _firstpage = (u8*)(((u32)memstart + 0xFFF) & -0x1000); // round up to 4KB alignment
  
  _startblock = _blocks;
  _startblock->size = memsize;
  _startblock->free = true;
  
  _startblock = _blocks;
  _memsize = memsize;
  _allocatedblocks = 0;
  
  for (auto& f : _freeblocks) {
    f = nullptr;
  }
  
  u32 lvl = LEVEL(memsize);
  addBlock(_startblock, lvl);

#if DEBUG_LEVEL >= 2
  std::cout << "New heap created @ 0x" << std::hex << (void*)_firstpage << "\tsize = 0x" << std::hex << memsize << " bytes" << std::endl;
#endif
}

void* FrameAlloc(u32 size) {
  size = (((u32)size + 0xFFF) & -0x1000); // round up to multiple of 4KB
  u32 lvl = LEVEL(size);
  
  // find first free block that is the same size or larger
  while (!_freeblocks[lvl] && lvl < NUM_LEVELS)
    ++lvl;
  
  auto tmp = _freeblocks[lvl];
  removeBlock(tmp, lvl);
  
  // if block is to large, split it
  while (tmp->size / 2 >= size) {
    tmp = split(tmp, lvl);
    --lvl;
  }
  
  tmp->free = false;
  
  ++_allocatedblocks;
  
  u32 idx = (tmp - _blocks);
  auto ret = _firstpage + idx * pagesize;

#if DEBUG_LEVEL >= 2
  std::cout << "Alloc 0x" << std::hex << size << " bytes at 0x" << std::hex << (void*)ret << "\tg_Blocks = " << std::dec << _allocatedblocks << std::endl;
#endif

  return ret;
}

bool FrameFree(void* ptr) {
  // verify pointer inside memory region
  if (ptr < _firstpage)
    return false;
  
  // verify pointer alignment
  if (((u32)ptr & 0xFFFFF000) != (u32)ptr)
    return false;
  
  u32 blockIdx = ((u8*)ptr - _firstpage) / pagesize;
  auto tmp = _blocks + blockIdx;
  
  tmp->free = true;
  
  addBlock(tmp, LEVEL(tmp->size));
  
  while (tmp) {
    if (tmp->size == _memsize)
      break;
    else
      tmp = merge(tmp);
  }
  
  --_allocatedblocks;

#if DEBUG_LEVEL >= 2
  std::cout << "Free " << std::hex << ptr << "\tg_Blocks = " << std::dec << _allocatedblocks << std::endl;
#endif
  return true;
}

void FrameCleanup(u32& remainingAllocations) {
  remainingAllocations = _allocatedblocks;
#if DEBUG_LEVEL >= 2
  std::cerr << "*** MEMORY LEAK! " << std::dec << _allocatedblocks << " allocations unfreed. ***" << std::endl;
#endif
}
