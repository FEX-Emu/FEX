/* FEX Horizon helper functions — ported from autorun/horizon-wine/fex */
/* Resolved against FEX-2610 (berlint-hub/FEX @ arena/799d045b-fex) */

#ifndef FEXCORE_UTILS_HORIZON_H
#define FEXCORE_UTILS_HORIZON_H

#include <cstdint>
#include <cstddef>

namespace FEXCore {
namespace Allocator {

/* Writable address mapping for Horizon code buffers */
inline void* HorizonWritableAddress(void* Pointer) {
    return Pointer;  // Direct mapping for fallback; overridden when FEX_HORIZON is active
}

/* Flush instruction cache after code emission */
inline void HorizonFlushICache(void* Begin, std::size_t Length) {
#if defined(__linux__) || defined(__unix__)
    __builtin___clear_cache(static_cast<char*>(Begin), static_cast<char*>(Begin) + Length);
#endif
}

} // namespace Allocator
} // namespace FEXCore

#endif
