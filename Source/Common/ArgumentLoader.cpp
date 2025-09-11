// SPDX-License-Identifier: MIT
#include "Common/ArgumentLoader.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <stdint.h>

namespace FEX::ArgLoader {
void FEX::ArgLoader::ArgLoader::PreLoad() {
  RemainingArgs.clear();
  ProgramArguments.clear();

  // Skip argument 0, which will be the interpreter
  for (int i = 1; i < argc; ++i) {
    RemainingArgs.emplace_back(argv[i]);
  }

  // Put the interpreter in ProgramArguments
  ProgramArguments.emplace_back(argv[0]);
}

} // namespace FEX::ArgLoader
