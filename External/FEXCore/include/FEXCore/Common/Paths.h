#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#include <string>

namespace FEXCore::Paths {
  FEX_DEFAULT_VISIBILITY const char *GetHomeDirectory();

  FEX_DEFAULT_VISIBILITY std::string GetCachePath();

  FEX_DEFAULT_VISIBILITY std::string GetEntryCachePath();
}
