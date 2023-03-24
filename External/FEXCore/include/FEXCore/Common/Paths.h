#pragma once
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/string.h>

namespace FEXCore::Paths {
  FEX_DEFAULT_VISIBILITY const char *GetHomeDirectory();

  FEX_DEFAULT_VISIBILITY fextl::string GetCachePath();

  FEX_DEFAULT_VISIBILITY fextl::string GetEntryCachePath();
}
