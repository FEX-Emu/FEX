#pragma once

#include <cstddef>

using MallocPtr = void* (*)(size_t);
using FreePtr = void (*)(void*);
using CallocPtr = void* (*)(size_t, size_t);
using MemalignPtr = void* (*)(size_t, size_t);
using ReallocPtr = void* (*)(void*, size_t);
using VallocPtr = void* (*)(size_t);
using PosixMemalignPtr = int (*)(void**, size_t, size_t);
using AlignedAllocPtr = void* (*)(size_t, size_t);
using MallocUsablePtr = size_t (*)(void*);

struct AllocationPtrs {
  MallocPtr Malloc;
  FreePtr Free;
  CallocPtr Calloc;
  MemalignPtr Memalign;
  ReallocPtr Realloc;
  VallocPtr Valloc;
  PosixMemalignPtr PosixMemalign;
  AlignedAllocPtr AlignedAlloc;
  MallocUsablePtr MallocUsable;
};
