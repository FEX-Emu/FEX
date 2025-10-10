// SPDX-License-Identifier: MIT
#include "Common/JSONPool.h"

namespace FEX::JSON {
static json_t* PoolInit(jsonPool_t* Pool) {
  auto* alloc = static_cast<JsonAllocator*>(Pool);
  return &*alloc->json_objects.emplace(alloc->json_objects.end());
}

static json_t* PoolAlloc(jsonPool_t* Pool) {
  auto* alloc = static_cast<JsonAllocator*>(Pool);
  return &*alloc->json_objects.emplace(alloc->json_objects.end());
}

JsonAllocator::JsonAllocator()
  : jsonPool_t {
      .init = PoolInit,
      .alloc = PoolAlloc,
    } {}
} // namespace FEX::JSON
