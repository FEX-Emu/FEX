// SPDX-License-Identifier: MIT
#pragma once

#include "FEXCore/Utils/CompilerDefs.h"
#include "FEXCore/Utils/File.h"

namespace FEXCore::DiskCache {
using FileMapperFunc = void* (*)(FEXCore::File::File::FileHandleType Handle, uint64_t MapSize);

FEX_DEFAULT_VISIBILITY void SetFileMapper(FileMapperFunc Func);
} // namespace FEXCore::DiskCache
