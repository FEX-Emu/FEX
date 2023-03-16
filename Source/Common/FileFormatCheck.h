#pragma once

#include <FEXCore/fextl/string.h>

namespace FEX::FormatCheck {
  bool IsSquashFS(fextl::string const &Filename);
  bool IsEroFS(fextl::string const &Filename);
}
