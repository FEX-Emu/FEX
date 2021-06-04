#pragma once
#include <FEXCore/Core/Context.h>
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

  __attribute__((visibility("default"))) std::string GetDataDirectory();
  __attribute__((visibility("default"))) std::string GetConfigDirectory(bool Global);
  __attribute__((visibility("default"))) std::string GetConfigFileLocation();
  __attribute__((visibility("default"))) std::string GetApplicationConfig(std::string &Filename, bool Global);

  using LayerValue = std::list<std::string>;
  using LayerOptions = std::unordered_map<ConfigOption, LayerValue>;

  class __attribute__((visibility("default"))) Layer {
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

  __attribute__((visibility("default"))) void Initialize();
  __attribute__((visibility("default"))) void Shutdown();

  __attribute__((visibility("default"))) void Load();
  __attribute__((visibility("default"))) void ReloadMetaLayer();

  __attribute__((visibility("default"))) void AddLayer(std::unique_ptr<FEXCore::Config::Layer> _Layer);

  __attribute__((visibility("default"))) bool Exists(ConfigOption Option);
  __attribute__((visibility("default"))) std::optional<LayerValue*> All(ConfigOption Option);
  __attribute__((visibility("default"))) std::optional<std::string*> Get(ConfigOption Option);

  __attribute__((visibility("default"))) void Set(ConfigOption Option, std::string Data);
  __attribute__((visibility("default"))) void Erase(ConfigOption Option);
  __attribute__((visibility("default"))) void EraseSet(ConfigOption Option, std::string Data);

  template<typename T>
  class __attribute__((visibility("default"))) Value {
  public:
    template <typename TT = T,
      typename std::enable_if<!std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option, T Default)
      : Option {_Option} {
      ValueData = FEXCore::Config::Value<T>::GetIfExists(Option, Default);
    }

    template <typename TT = T,
      typename std::enable_if<std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option, T Default)
      : Option {_Option} {
      ValueData = FEXCore::Config::Value<T>::GetIfExists(Option, Default);
      GetListIfExists(Option, &AppendList);
    }

    template <typename TT = T,
      typename std::enable_if<!std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE("FEXCore::Config::Value has no value");
      }

      ValueData = FEXCore::Config::Value<T>::Get(Option);
    }

    template <typename TT = T,
      typename std::enable_if<std::is_same<TT, std::string>::value, int>::type = 0>
    Value(FEXCore::Config::ConfigOption _Option)
      : Option {_Option} {
      if (!FEXCore::Config::Exists(Option)) {
        ERROR_AND_DIE("FEXCore::Config::Value has no value");
      }

      ValueData = FEXCore::Config::Value<T>::GetIfExists(Option);
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
