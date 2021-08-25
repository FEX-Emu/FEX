#pragma once
#include <FEXCore/Config/Config.h>

#include <cassert>
#include <list>
#include <string>
#include <string_view>

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
}
