/* Fallback Horizon CRT heap header — FEX-2610 */
#ifndef SOURCE_WINDOWS_COMMON_CRT_HORIZONHEAP_H
#define SOURCE_WINDOWS_COMMON_CRT_HORIZONHEAP_H

#include <cstddef>

namespace FEXCore {
namespace Allocator {
void* HorizonWritableAddress(void* Pointer);
void HorizonFlushICache(void* Begin, std::size_t Length);
} // namespace Allocator
} // namespace FEXCore

#endif
