/* Fallback Horizon integration — FEX-2610 */
#include <FEXCore/Utils/Horizon.h>
#include <windows.h>

namespace WineNX {
namespace FEX {

uint64_t LookupAddressSpace(bool Is64Bit) {
  return Is64Bit ? (static_cast<uint64_t>(1) << 36) : (static_cast<uint32_t>(1) << 32);
}

uint64_t CodeCacheLimit(bool Is64Bit) {
  return Is64Bit ? (128ULL << 20) : (64ULL << 20);
}

} // namespace FEX
} // namespace WineNX
