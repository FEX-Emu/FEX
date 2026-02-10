// SPDX-License-Identifier: MIT
#define _SECIMP
#define _CRTIMP
#include <cstdint>
#include "../Priv.h"
#include <rpmalloc/rpmalloc.h>

void* calloc(size_t NumOfElements, size_t SizeOfElements) {
  return ::rpcalloc(NumOfElements, SizeOfElements);
}

void free(void* Memory) {
  ::rpfree(Memory);
}

void* malloc(size_t Size) {
  return ::rpmalloc(Size);
}

void* realloc(void* Memory, size_t NewSize) {
  return ::rprealloc(Memory, NewSize);
}

DLLEXPORT_FUNC(void*, _aligned_malloc, (size_t Size, size_t Alignment)) {
  return ::rpaligned_alloc(Alignment, Size);
}

DLLEXPORT_FUNC(void, _aligned_free, (void* Memory)) {
  ::rpfree(Memory);
}
