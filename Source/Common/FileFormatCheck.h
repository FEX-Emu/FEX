#pragma once

#include <string>

namespace FEX::FormatCheck {
  bool IsSquashFS(std::string const &Filename);
  bool IsEroFS(std::string const &Filename);
}
