// SPDX-License-Identifier: MIT

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/list.h>

#include <tiny-json.h>

namespace FEX::JSON {
struct JsonAllocator {
  jsonPool_t PoolObject;
  fextl::list<json_t> json_objects;

  JsonAllocator();
};
static_assert(offsetof(JsonAllocator, PoolObject) == 0, "This needs to be at offset zero");

template<typename T>
const json_t* CreateJSON(T& Container, JsonAllocator& Allocator) {
  if (Container.empty()) {
    return nullptr;
  }

  return json_createWithPool(&Container.at(0), &Allocator.PoolObject);
}
} // namespace FEX::JSON
