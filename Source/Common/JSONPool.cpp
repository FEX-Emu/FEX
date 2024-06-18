// SPDX-License-Identifier: MIT
#include "Common/JSONPool.h"

namespace FEX::JSON {
json_t* PoolInit(jsonPool_t* Pool);
json_t* PoolAlloc(jsonPool_t* Pool);

JsonAllocator::JsonAllocator()
  : PoolObject {
      .init = FEX::JSON::PoolInit,
      .alloc = FEX::JSON::PoolAlloc,
    } {}

json_t* PoolInit(jsonPool_t* Pool) {
  JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
  return &*alloc->json_objects.emplace(alloc->json_objects.end());
}

json_t* PoolAlloc(jsonPool_t* Pool) {
  JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
  return &*alloc->json_objects.emplace(alloc->json_objects.end());
}
} // namespace FEX::JSON
