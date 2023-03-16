#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/vector.h>

#include <string>

namespace FEX::ArgLoader {
  class ArgLoader final : public FEXCore::Config::Layer {
  public:
    explicit ArgLoader(int _argc, char **_argv)
      : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_ARGUMENTS)
      , argc {_argc}
      , argv {_argv} {}

    void Load();

  private:
    int argc{};
    char **argv;
  };

  void LoadWithoutArguments(int _argc, char **_argv);
  fextl::vector<fextl::string> Get();
  fextl::vector<fextl::string> GetParsedArgs();
}
