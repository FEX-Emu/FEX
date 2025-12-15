// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <mutex>
#include <map>
#include <shared_mutex>
#include <string_view>

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Config/Config.h>

#include "Common/VolatileMetadata.h"
#include "Module.h"

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::Context {
class Context;
}

namespace FEX::Windows {
#ifdef _M_ARM_64EC
using ArchImageNtHeaders = IMAGE_NT_HEADERS64;
using ArchImageLoadConfigDirectory = _IMAGE_LOAD_CONFIG_DIRECTORY64;
#else
using ArchImageNtHeaders = IMAGE_NT_HEADERS32;
using ArchImageLoadConfigDirectory = _IMAGE_LOAD_CONFIG_DIRECTORY32;
#endif

/**
 * @brief Tracks mapped PE code images and handles their volatile metadata
 */
class ImageTracker {
public:
  ImageTracker(FEXCore::Context::Context& CTX);
  void HandleImageMap(std::string_view Path, uint64_t Address, bool MainImage);
  void HandleImageUnmap(uint64_t Address, uint64_t Size);

private:
  FEXCore::Context::Context& CTX;

  FEX_CONFIG_OPT(ExtendedVolatileMetadataConfig, EXTENDEDVOLATILEMETADATA);
  fextl::unordered_map<fextl::string, FEX::VolatileMetadata::ExtendedVolatileMetadata> ExtendedMetaData;
};

} // namespace FEX::Windows
