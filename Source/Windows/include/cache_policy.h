/* Fallback Horizon ABI — cache policy for FEX on Tegra X1 (Nintendo Switch) */
#ifndef WINE_NX_FEX_CACHE_POLICY_H
#define WINE_NX_FEX_CACHE_POLICY_H

#include <cstdint>
#include <cstddef>

namespace WineNX {
namespace FEX {

inline uint64_t LookupAddressSpace(bool Is64Bit) {
    // Tegra X1 (Cortex-A57) Horizon OS address space
    return Is64Bit ? (static_cast<uint64_t>(1) << 36) : (static_cast<uint32_t>(1) << 32);
}

inline uint64_t CodeCacheLimit(bool Is64Bit) {
    return Is64Bit ? (128ULL << 20) : (64ULL << 20);  // 128 MB / 64 MB
}

} // namespace FEX
} // namespace WineNX

#endif
