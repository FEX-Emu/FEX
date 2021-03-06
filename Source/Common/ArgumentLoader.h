#pragma once

#include <FEXCore/Config/Config.h>

#include <string>
#include <vector>

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
  std::vector<std::string> Get();
  std::vector<std::string> GetParsedArgs();

}
