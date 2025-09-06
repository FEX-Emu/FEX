// SPDX-License-Identifier: MIT

#include <FEXCore/fextl/list.h>

#include <iterator>
#include <tiny-json.h>

namespace FEX::JSON {
struct JsonAllocator : jsonPool_t {
  fextl::list<json_t> json_objects;

  JsonAllocator();
};

template<typename T>
const json_t* CreateJSON(T& Container, JsonAllocator& Allocator) {
  if (std::empty(Container)) {
    return nullptr;
  }

  return json_createWithPool(std::data(Container), &Allocator);
}
} // namespace FEX::JSON
