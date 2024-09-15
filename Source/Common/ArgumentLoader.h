// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

namespace FEX::ArgLoader {
class ArgLoader final : public FEXCore::Config::Layer {
public:
  enum class LoadType {
    WITH_FEXLOADER_PARSER,
    WITHOUT_FEXLOADER_PARSER,
  };

  explicit ArgLoader(LoadType Type, int argc, char** argv)
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_ARGUMENTS)
    , Type {Type}
    , argc {argc}
    , argv {argv} {

    if (Type == LoadType::WITHOUT_FEXLOADER_PARSER) {
      LoadWithoutArguments();
    } else {
      Load();
    }
  }

  void Load() override;
  void LoadWithoutArguments();
  fextl::vector<fextl::string> Get() {
    return RemainingArgs;
  }

  fextl::vector<const char*> GetParsedArgs() {
    return ProgramArguments;
  }

  LoadType GetLoadType() const {
    return Type;
  }

  void SetArg(FEXCore::Config::ConfigOption Option, std::string_view Arg);

private:
  LoadType Type;
  int argc {};
  char** argv {};

  fextl::vector<fextl::string> RemainingArgs {};
  fextl::vector<const char*> ProgramArguments {};
};

} // namespace FEX::ArgLoader
