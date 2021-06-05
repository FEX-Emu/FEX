#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <list>
#include <memory>
#include <optional>
#include <stdint.h>
#include <unordered_map>

namespace FEXCore::Config {
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
    CONFIG_SMC_MMAN,
    CONFIG_SMC_FULL,
  };

  enum class LayerType {
    LAYER_MAIN,
    LAYER_ARGUMENTS,
    LAYER_GLOBAL_APP,
    LAYER_LOCAL_APP,
    LAYER_ENVIRONMENT,
    LAYER_TOP,
  };

namespace DefaultValues {
#define P(x) x
#define OPT_BASE(type, group, enum, json, default) constexpr P(type) P(enum) = P(default);
#define OPT_STR(group, enum, json, default) const std::string P(enum) = P(default);
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
  FEX_DEFAULT_VISIBILITY std::string GetConfigFileLocation();
  FEX_DEFAULT_VISIBILITY std::string GetApplicationConfig(const std::string &Filename, bool Global);

  using LayerValue = std::list<std::string>;
  using LayerOptions = std::unordered_map<ConfigOption, LayerValue>;

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

    void Set(ConfigOption Option, std::string Data) {
      OptionMap[Option].emplace_back(std::move(Data));
    }

    void EraseSet(ConfigOption Option, std::string Data) {
      Erase(Option);
      Set(Option, std::move(Data));
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

  FEX_DEFAULT_VISIBILITY void AddLayer(std::unique_ptr<FEXCore::Config::Layer> _Layer);

  FEX_DEFAULT_VISIBILITY bool Exists(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY std::optional<LayerValue*> All(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY std::optional<std::string*> Get(ConfigOption Option);

  FEX_DEFAULT_VISIBILITY void Set(ConfigOption Option, std::string Data);
  FEX_DEFAULT_VISIBILITY void Erase(ConfigOption Option);
  FEX_DEFAULT_VISIBILITY void EraseSet(ConfigOption Option, std::string Data);

  template<typename T>
  class FEX_DEFAULT_VISIBILITY Value {
  public:
    template <typename TT = T,
      typename std::enable_if<!std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option, T Default)
      : Option {_Option} {
      ValueData = GetIfExists(Option, Default);
    }

    template <typename TT = T,
      typename std::enable_if<std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option, T Default)
      : Option {_Option} {
      ValueData = GetIfExists(Option, Default);
      GetListIfExists(Option, &AppendList);
    }

    template <typename TT = T,
      typename std::enable_if<!std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE("FEXCore::Config::Value has no value");
      }

      ValueData = Get(Option);
    }

    template <typename TT = T,
      typename std::enable_if<std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE("FEXCore::Config::Value has no value");
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
    static void GetListIfExists(FEXCore::Config::ConfigOption Option, std::list<std::string> *List);
  };
}
