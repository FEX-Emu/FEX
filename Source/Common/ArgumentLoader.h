// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

namespace FEX::ArgLoader {
class ArgLoader final : public FEXCore::Config::Layer {
public:
  explicit ArgLoader(int argc, char** argv)
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_ARGUMENTS)
    , argc {argc}
    , argv {argv} {
    PreLoad();
  }

  void Load() override {
    // Intentional no-op.
  }
  void PreLoad();
  fextl::vector<fextl::string> Get() {
    return RemainingArgs;
  }
  fextl::vector<fextl::string> GetParsedArgs() {
    return ProgramArguments;
  }

private:
  int argc {};
  char** argv {};

  fextl::vector<fextl::string> RemainingArgs {};
  fextl::vector<fextl::string> ProgramArguments {};
};

} // namespace FEX::ArgLoader
