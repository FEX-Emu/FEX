#pragma once
#include <FEXCore/Config/Config.h>

#include <string>

/**
 * @brief This is a singleton for storing global configuration state
 */
namespace FEX::Config {
  class EmptyMapper : public FEXCore::Config::Layer {
  public:
    explicit EmptyMapper()
      : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_MAIN) {
    }
    void Load() override {}

  protected:
  };

  void SaveLayerToJSON(const std::string& Filename, FEXCore::Config::Layer *const Layer);

  std::pair<std::string, std::string> LoadConfig(
    bool NoFEXArguments,
    bool LoadProgramConfig,
    int argc,
    char **argv,
    char **const envp
  );
}
