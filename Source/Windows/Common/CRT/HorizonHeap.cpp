/* Fallback Horizon CRT heap helper — FEX-2610 */
#include <windows.h>
#include <FEXCore/Utils/Horizon.h>

namespace FEXCore {
namespace Allocator {

void* HorizonWritableAddress(void* Pointer) {
  return Pointer;
}

void HorizonFlushICache(void* Begin, std::size_t Length) {
  __builtin___clear_cache(static_cast<char*>(Begin), static_cast<char*>(Begin) + Length);
}

} // namespace Allocator
} // namespace FEXCore
