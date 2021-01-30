#pragma once
#include <FEXCore/Core/Context.h>

#include <list>
#include <memory>
#include <optional>
#include <stdint.h>
#include <unordered_map>

namespace FEXCore::Config {
  enum ConfigOption {
    CONFIG_MULTIBLOCK,
    CONFIG_MAXBLOCKINST,
    CONFIG_DEFAULTCORE,
    CONFIG_VIRTUALMEMSIZE,
    CONFIG_SINGLESTEP,
    CONFIG_GDBSERVER,
    CONFIG_ROOTFSPATH,
    CONFIG_THUNKLIBSPATH,
    CONFIG_IS64BIT_MODE,
    CONFIG_EMULATED_CPU_CORES,
    CONFIG_TSO_ENABLED,
    CONFIG_SMC_CHECKS,
    CONFIG_ABI_LOCAL_FLAGS,
    CONFIG_ABI_NO_PF,
    CONFIG_DUMPIR,
    CONFIG_VALIDATE_IR_PARSER,
    CONFIG_SILENTLOGS,
    CONFIG_ENVIRONMENT,
    CONFIG_OUTPUTLOG,
    CONFIG_BREAK_ON_FRONTEND,
    CONFIG_DUMP_GPRS,
    CONFIG_IS_INTERPRETER,
    CONFIG_INTERPRETER_INSTALLED,
    CONFIG_APP_FILENAME,
    CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES,
    CONFIG_AOTIR_GENERATE,
    CONFIG_AOTIR_LOAD
  };

  enum ConfigCore {
    CONFIG_INTERPRETER,
    CONFIG_IRJIT,
    CONFIG_CUSTOM,
  };

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config);
  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, std::string const &Config);
  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option);

  enum class LayerType {
    LAYER_MAIN,
    LAYER_ARGUMENTS,
    LAYER_GLOBAL_APP,
    LAYER_LOCAL_APP,
    LAYER_ENVIRONMENT,
    LAYER_TOP,
  };

  using LayerValue = std::list<std::string>;
  using LayerOptions = std::unordered_map<ConfigOption, LayerValue>;

  class Layer {
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
      OptionMap[Option].emplace_back(Data);
    }

    void EraseSet(ConfigOption Option, std::string Data) {
      OptionMap.erase(Option);
      OptionMap[Option].emplace_back(Data);
    }

    const LayerType GetLayerType() const { return Type; }
    const LayerOptions &GetOptionMap() { return OptionMap; }

  protected:
    const LayerType Type;
    LayerOptions OptionMap;
  };

  void Initialize();
  void Shutdown();

  void Load();
  void ReloadMetaLayer();

  void AddLayer(std::unique_ptr<FEXCore::Config::Layer> _Layer);

  bool Exists(ConfigOption Option);
  std::optional<LayerValue*> All(ConfigOption Option);
  std::optional<std::string*> Get(ConfigOption Option);

  void Set(ConfigOption Option, std::string Data);
  void EraseSet(ConfigOption Option, std::string Data);

  template<typename T>
  class Value {
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

    T operator()() { return ValueData; }
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
