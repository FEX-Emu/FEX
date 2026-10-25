/* FEX Horizon helper functions — ported from autorun/horizon-wine/fex */
/* Resolved against FEX-2610 (berlint-hub/FEX @ arena/799d045b-fex) */

#ifndef FEXCORE_UTILS_HORIZON_H
#define FEXCORE_UTILS_HORIZON_H

#include <cstdint>
#include <cstddef>

namespace FEXCore {
namespace Allocator {

/* Writable address mapping for Horizon code buffers, and the instruction
   cache flush after code emission: defined once by the Horizon CRT in
   Source/Windows/Common/CRT/HorizonHeap.cpp, like Autorun's horizon.patch. */
void* HorizonWritableAddress(void* Pointer);
void HorizonFlushICache(void* Begin, std::size_t Length);

} // namespace Allocator
} // namespace FEXCore

#endif
