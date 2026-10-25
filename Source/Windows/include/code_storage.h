/* Fallback Horizon ABI — code storage definitions */
#ifndef WINE_NX_FEX_CODE_STORAGE_H
#define WINE_NX_FEX_CODE_STORAGE_H

#include <cstdint>
#include <cstddef>

namespace WineNX {
namespace FEX {

/* Code storage parameters for shared JIT buffers on Tegra X1 */
struct CodeStorageInfo {
    size_t BufferSize = 64 * 1024 * 1024;  // 64 MB default
    size_t PageSize = 4096;
    bool Writable = false;
};

inline CodeStorageInfo GetDefaultCodeStorage() {
    return CodeStorageInfo{};
}

} // namespace FEX
} // namespace WineNX

#endif
