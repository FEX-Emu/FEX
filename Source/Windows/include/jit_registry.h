/* Fallback Horizon ABI — JIT registry definitions */
#ifndef WINE_NX_FEX_JIT_REGISTRY_H
#define WINE_NX_FEX_JIT_REGISTRY_H

#include <cstdint>

namespace WineNX {
namespace FEX {

/* JIT registry handles for code buffer tracking */
using JITRegistryHandle = uint64_t;

inline JITRegistryHandle RegisterJITBuffer(void* Base, size_t Size) {
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Base));
}

inline void UnregisterJITBuffer(JITRegistryHandle Handle) {
    (void)Handle;
}

} // namespace FEX
} // namespace WineNX

#endif
