#pragma once

#include "ELFCodeLoader2.h"

namespace FEX::AOT {
  void AOTGenSection(FEXCore::Context::Context *CTX, ELFCodeLoader2::LoadedSection &Section);
}
