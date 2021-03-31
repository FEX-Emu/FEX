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

  class OptionMapper : public FEXCore::Config::Layer {
  public:
    explicit OptionMapper(FEXCore::Config::LayerType Layer);

  protected:
    void MapNameToOption(const char *ConfigName, const char *ConfigString);
  };

  class MainLoader final : public FEX::Config::OptionMapper {
  public:
    explicit MainLoader();
    explicit MainLoader(std::string ConfigFile);
    void Load() override;

  private:
    std::string Config;
  };

  class AppLoader final : public FEX::Config::OptionMapper {
  public:
    explicit AppLoader(std::string Filename, bool Global);
    void Load();

  private:
    std::string Config;
  };

  class EnvLoader final : public FEXCore::Config::Layer {
  public:
    explicit EnvLoader(char *const _envp[]);
    void Load() override;

  private:
    char *const *envp;
  };

  void SaveLayerToJSON(std::string Filename, FEXCore::Config::Layer *const Layer);
}
