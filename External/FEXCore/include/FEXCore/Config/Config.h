#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/unordered_map.h>

#include <list>
#include <memory>
#include <optional>
#include <stdint.h>

namespace FEXCore::Config {
namespace Handler {
  static inline std::string_view CoreHandler(std::string_view Value) {
    if (Value == "irint")
      return "0";
    else if (Value == "irjit")
      return "1";
#ifdef _M_X86_64
    else if (Value == "host")
      return "2";
#endif
    return "1";
  }

  static inline std::string_view SMCCheckHandler(std::string_view Value) {
    if (Value == "none")
      return "0";
    else if (Value == "mtrack")
      return "1";
    else if (Value == "full")
      return "2";
    else if (Value == "mman")
      return "3";
    return "0";
  }
  static inline std::string_view CacheObjectCodeHandler(std::string_view Value) {
    if (Value == "none")
      return "0";
    else if (Value == "read")
      return "1";
    else if (Value == "write")
      return "2";
    return "0";
  }
}

  enum ConfigOption {
#define OPT_BASE(type, group, enum, json, default) CONFIG_##enum,
#include <FEXCore/Config/ConfigValues.inl>
  };

  enum ConfigCore {
    CONFIG_INTERPRETER,
    CONFIG_IRJIT,
    CONFIG_CUSTOM,
  };

  enum ConfigSMCChecks {
    CONFIG_SMC_NONE,
    CONFIG_SMC_MTRACK,
    CONFIG_SMC_FULL,
    CONFIG_SMC_MMAN,
  };

  enum ConfigObjectCodeHandler {
    CONFIG_NONE,
    CONFIG_READ,
    CONFIG_READWRITE,
  };

  enum class LayerType {
    LAYER_GLOBAL_MAIN, ///< /usr/share/fex-emu/Config.json by default
    LAYER_MAIN,
    LAYER_ARGUMENTS,
    LAYER_GLOBAL_STEAM_APP,
    LAYER_GLOBAL_APP,
    LAYER_LOCAL_STEAM_APP,
    LAYER_LOCAL_APP,
    LAYER_ENVIRONMENT,
    LAYER_TOP,
  };

namespace DefaultValues {
#define P(x) x
#define OPT_BASE(type, group, enum, json, default) extern const P(type) P(enum);
#define OPT_STR(group, enum, json, default) extern const std::string_view P(enum);
#define OPT_STRARRAY(group, enum, json, default) OPT_STR(group, enum, json, default)
#include <FEXCore/Config/ConfigValues.inl>

namespace Type {
#define OPT_BASE(type, group, enum, json, default) using P(enum) = P(type);
#define OPT_STR(group, enum, json, default) using P(enum) = std::string;
#define OPT_STRARRAY(group, enum, json, default) OPT_STR(group, enum, json, default)
#include <FEXCore/Config/ConfigValues.inl>
}
#define FEX_CONFIG_OPT(name, enum) \
  FEXCore::Config::Value<FEXCore::Config::DefaultValues::Type::enum> name {FEXCore::Config::CONFIG_##enum, FEXCore::Config::DefaultValues::enum}

#undef P
}

  FEX_DEFAULT_VISIBILITY std::string GetDataDirectory();
  FEX_DEFAULT_VISIBILITY std::string GetConfigDirectory(bool Global);
  FEX_DEFAULT_VISIBILITY std::string GetConfigFileLocation(bool Global = false);
  FEX_DEFAULT_VISIBILITY std::string GetApplicationConfig(const std::string &Filename, bool Global);

  using LayerValue = std::list<std::string>;
  using LayerOptions = fextl::unordered_map<ConfigOption, LayerValue>;

  class FEX_DEFAULT_VISIBILITY Layer {
  public:
    explicit Layer(const LayerType _Type);
    virtual ~Layer();

    virtual void Load() = 0;

    bool OptionExists(ConfigOption Option) const {
      return OptionMap.find(Option) != OptionMap.end();
    }

    std::optional<LayerValue*>
    All(ConfigOption Option) {
      const auto it = OptionMap.find(Option);
      if (it == OptionMap.end()) {
        return std::nullopt;
      }

      return &it->second;
    }

    std::optional<std::string*>
    Get(ConfigOption Option) {
      const auto it = OptionMap.find(Option);
      if (it == OptionMap.end()) {
        return std::nullopt;
      }

      return &it->second.front();
    }

    void Set(ConfigOption Option, std::string_view Data) {
      OptionMap[Option].emplace_back(std::string(Data));
    }

    void EraseSet(ConfigOption Option, std::string_view Data) {
      Erase(Option);
      Set(Option, std::string(Data));
    }

    void Erase(ConfigOption Option) {
      OptionMap.erase(Option);
    }

    LayerType GetLayerType() const { return Type; }
    const LayerOptions &GetOptionMap() const { return OptionMap; }

  protected:
    const LayerType Type;
    LayerOptions OptionMap;
  };

  FEX_DEFAULT_VISIBILITY void Initialize();
  FEX_DEFAULT_VISIBILITY void Shutdown();

  FEX_DEFAULT_VISIBILITY void Load();
  FEX_DEFAULT_VISIBILITY void ReloadMetaLayer();
  FEX_DEFAULT_VISIBILITY std::string FindContainer();
  FEX_DEFAULT_VISIBILITY std::string FindContainerPrefix();

  FEX_DEFAULT_VISIBILITY void AddLayer(std::unique_ptr<FEXCore::Config::Layer> _Layer);

  FEX_DEFAULT_VISIBILITY bool Exists(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY std::optional<LayerValue*> All(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY std::optional<std::string*> Get(ConfigOption Option);

  FEX_DEFAULT_VISIBILITY void Set(ConfigOption Option, std::string_view Data);
  FEX_DEFAULT_VISIBILITY void Erase(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY void EraseSet(ConfigOption Option, std::string_view Data);

  template<typename T>
  class FEX_DEFAULT_VISIBILITY Value {
  public:
    template <typename TT = T> requires (!std::is_same_v<TT, std::string>)
    Value(FEXCore::Config::ConfigOption _Option, TT Default)
      : Option {_Option} {
      ValueData = GetIfExists(Option, Default);
    }

    template <typename TT = T> requires (std::is_same_v<TT, std::string>)
    Value(FEXCore::Config::ConfigOption _Option, TT Default)
      : Option {_Option} {
      ValueData = GetIfExists(Option, Default);
      GetListIfExists(Option, &AppendList);
    }

    template <typename TT = T> requires (std::is_same_v<TT, std::string>)
    Value(FEXCore::Config::ConfigOption _Option, std::string_view Default)
      : Option {_Option} {
      ValueData = GetIfExists(Option, Default);
      GetListIfExists(Option, &AppendList);
    }

    template <typename TT = T> requires (!std::is_same_v<TT, std::string>)
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE_FMT("FEXCore::Config::Value has no value");
      }

      ValueData = Get(Option);
    }

    template <typename TT = T> requires (std::is_same_v<TT, std::string>)
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE_FMT("FEXCore::Config::Value has no value");
      }

      ValueData = GetIfExists(Option);
      GetListIfExists(Option, &AppendList);
    }

    operator T() const { return ValueData; }
    T operator()() const { return ValueData; }
    Value<T>(T Value) { ValueData = std::move(Value); }
    std::list<T> &All() { return AppendList; }

  private:
    FEXCore::Config::ConfigOption Option;
    T ValueData;
    std::list<T> AppendList;

    static T Get(FEXCore::Config::ConfigOption Option);
    static T GetIfExists(FEXCore::Config::ConfigOption Option, T Default);
    static T GetIfExists(FEXCore::Config::ConfigOption Option, std::string_view Default);

    static void GetListIfExists(FEXCore::Config::ConfigOption Option, std::list<std::string> *List);
  };

  // Application loaders
  class FEX_DEFAULT_VISIBILITY OptionMapper : public FEXCore::Config::Layer {
  public:
    explicit OptionMapper(FEXCore::Config::LayerType Layer);

  protected:
    void MapNameToOption(const char *ConfigName, const char *ConfigString);
  };

  /**
   * @brief Loads the global FEX config
   *
   * @return unique_ptr for that layer
   */
  FEX_DEFAULT_VISIBILITY std::unique_ptr<FEXCore::Config::Layer> CreateGlobalMainLayer();

  /**
   * @brief Loads the main application config
   *
   * @param File Optional override to load a specific config file in to the main layer
   * Shouldn't be commonly used
   *
   * @return unique_ptr for that layer
   */
  FEX_DEFAULT_VISIBILITY std::unique_ptr<FEXCore::Config::Layer> CreateMainLayer(std::string const *File = nullptr);

  /**
   * @brief Create an application configuration loader
   *
   * @param Filename Application filename component
   * @param Global Load the global configuration or user accessible file
   *
   * @return unique_ptr for that layer
   */
  FEX_DEFAULT_VISIBILITY std::unique_ptr<FEXCore::Config::Layer> CreateAppLayer(const std::string& Filename, FEXCore::Config::LayerType Type);

  /**
   * @brief iCreate an environment configuration loader
   *
   * @param _envp[] The environment array from main
   *
   * @return unique_ptr for that layer
   */
  FEX_DEFAULT_VISIBILITY std::unique_ptr<FEXCore::Config::Layer> CreateEnvironmentLayer(char *const _envp[]);

}
