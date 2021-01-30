#include "Common/StringConv.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <map>

namespace FEXCore::Config {
  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config) {
    switch (Option) {
    case FEXCore::Config::CONFIG_MULTIBLOCK:
      CTX->Config.Multiblock = Config != 0;
    break;
    case FEXCore::Config::CONFIG_MAXBLOCKINST:
      CTX->Config.MaxInstPerBlock = Config;
    break;
    case FEXCore::Config::CONFIG_DEFAULTCORE:
      CTX->Config.Core = static_cast<FEXCore::Config::ConfigCore>(Config);
    break;
    case FEXCore::Config::CONFIG_VIRTUALMEMSIZE:
      CTX->Config.VirtualMemSize = Config;
    break;
    case FEXCore::Config::CONFIG_SINGLESTEP:
      CTX->Config.RunningMode = Config != 0 ? FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP : FEXCore::Context::CoreRunningMode::MODE_RUN;
    break;
    case FEXCore::Config::CONFIG_GDBSERVER:
      Config != 0 ? CTX->StartGdbServer() : CTX->StopGdbServer();
    break;
    case FEXCore::Config::CONFIG_IS64BIT_MODE:
      CTX->Config.Is64BitMode = Config != 0;
    break;
    case FEXCore::Config::CONFIG_TSO_ENABLED:
      CTX->Config.TSOEnabled = Config != 0;
    break;
    case FEXCore::Config::CONFIG_SMC_CHECKS:
      CTX->Config.SMCChecks = Config != 0;
    break;
    case FEXCore::Config::CONFIG_ABI_LOCAL_FLAGS:
      CTX->Config.ABILocalFlags = Config != 0;
    break;
    case FEXCore::Config::CONFIG_ABI_NO_PF:
      CTX->Config.ABINoPF = Config != 0;
    break;
    case FEXCore::Config::CONFIG_VALIDATE_IR_PARSER:
      CTX->Config.ValidateIRarser = Config != 0;
    break;
    case FEXCore::Config::CONFIG_AOTIR_GENERATE:
      CTX->Config.AOTIRGenerate = Config != 0;
    break;
    case FEXCore::Config::CONFIG_AOTIR_LOAD:
      CTX->Config.AOTIRLoad = Config != 0;
    break;
    default: LogMan::Msg::A("Unknown configuration option");
    }
  }

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, std::string const &Config) {
    switch (Option) {
    case CONFIG_ROOTFSPATH:
      CTX->Config.RootFSPath = Config;
      break;
    case CONFIG_THUNKLIBSPATH:
      CTX->Config.ThunkLibsPath = Config;
      break;
    case FEXCore::Config::CONFIG_DUMPIR:
      CTX->Config.DumpIR = Config;
      break;
    default: LogMan::Msg::A("Unknown configuration option");
    }
  }

  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option) {
    switch (Option) {
    case FEXCore::Config::CONFIG_MULTIBLOCK:
      return CTX->Config.Multiblock;
    break;
    case FEXCore::Config::CONFIG_MAXBLOCKINST:
      return CTX->Config.MaxInstPerBlock;
    break;
    case FEXCore::Config::CONFIG_DEFAULTCORE:
      return CTX->Config.Core;
    break;
    case FEXCore::Config::CONFIG_VIRTUALMEMSIZE:
      return CTX->Config.VirtualMemSize;
    break;
    case FEXCore::Config::CONFIG_SINGLESTEP:
      return CTX->Config.RunningMode == FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP ? 1 : 0;
    case FEXCore::Config::CONFIG_GDBSERVER:
      return CTX->GetGdbServerStatus();
    break;
    case FEXCore::Config::CONFIG_IS64BIT_MODE:
      return CTX->Config.Is64BitMode;
    break;
    case FEXCore::Config::CONFIG_TSO_ENABLED:
      return CTX->Config.TSOEnabled;
    break;
    case FEXCore::Config::CONFIG_SMC_CHECKS:
      return CTX->Config.SMCChecks;
    break;
    case FEXCore::Config::CONFIG_ABI_LOCAL_FLAGS:
      return CTX->Config.ABILocalFlags;
    break;
    case FEXCore::Config::CONFIG_ABI_NO_PF:
      return CTX->Config.ABINoPF;
    break;
    case FEXCore::Config::CONFIG_VALIDATE_IR_PARSER:
      return CTX->Config.ValidateIRarser;
    break;
    case FEXCore::Config::CONFIG_AOTIR_GENERATE:
      return CTX->Config.AOTIRGenerate;
    break;
    case FEXCore::Config::CONFIG_AOTIR_LOAD:
      return CTX->Config.AOTIRLoad;
    break;
    default: LogMan::Msg::A("Unknown configuration option");
    }

    return 0;
  }

  static std::map<FEXCore::Config::LayerType, std::unique_ptr<FEXCore::Config::Layer>> ConfigLayers;
  static FEXCore::Config::Layer *Meta{};

  constexpr std::array<FEXCore::Config::LayerType, 6> LoadOrder = {
    FEXCore::Config::LayerType::LAYER_MAIN,
    FEXCore::Config::LayerType::LAYER_GLOBAL_APP,
    FEXCore::Config::LayerType::LAYER_LOCAL_APP,
    FEXCore::Config::LayerType::LAYER_ARGUMENTS,
    FEXCore::Config::LayerType::LAYER_ENVIRONMENT,
    FEXCore::Config::LayerType::LAYER_TOP
  };

  Layer::Layer(const LayerType _Type)
    : Type {_Type} {
  }

  Layer::~Layer() {
  }

  class MetaLayer final : public FEXCore::Config::Layer {
  public:
    MetaLayer(const LayerType _Type)
      : FEXCore::Config::Layer (_Type) {
    }
    ~MetaLayer() {
    }
    void Load();

  private:
    void MergeConfigMap(const LayerOptions &Options);
  };

  void MetaLayer::Load() {
    OptionMap.clear();

    for (auto CurrentLayer = LoadOrder.begin(); CurrentLayer != LoadOrder.end(); ++CurrentLayer) {
      auto it = ConfigLayers.find(*CurrentLayer);
      if (it != ConfigLayers.end() && *CurrentLayer != Type) {
        // Merge this layer's options to this layer
        MergeConfigMap(it->second->GetOptionMap());
      }
    }
  }

  void MetaLayer::MergeConfigMap(const LayerOptions &Options) {
    // Insert this layer's options, overlaying previous options that exist here
    for (auto &it : Options) {
      OptionMap.insert_or_assign(it.first, it.second);
    }
  }

  void Initialize() {
    AddLayer(std::make_unique<MetaLayer>(FEXCore::Config::LayerType::LAYER_TOP));
    Meta = ConfigLayers.begin()->second.get();
  }

  void Shutdown() {
    ConfigLayers.clear();
    Meta = nullptr;
  }

  void Load() {
    for (auto CurrentLayer = LoadOrder.begin(); CurrentLayer != LoadOrder.end(); ++CurrentLayer) {
      auto it = ConfigLayers.find(*CurrentLayer);
      if (it != ConfigLayers.end()) {
        it->second->Load();
      }
    }
  }

  void ReloadMetaLayer() {
    Meta->Load();
  }

  void AddLayer(std::unique_ptr<FEXCore::Config::Layer> _Layer) {
    ConfigLayers.emplace(_Layer->GetLayerType(), std::move(_Layer));
  }

  bool Exists(ConfigOption Option) {
    return Meta->OptionExists(Option);
  }

  std::optional<LayerValue*> All(ConfigOption Option) {
    return Meta->All(Option);
  }

  std::optional<std::string*> Get(ConfigOption Option) {
    return Meta->Get(Option);
  }

  void Set(ConfigOption Option, std::string Data) {
    Meta->Set(Option, Data);
  }

  void EraseSet(ConfigOption Option, std::string Data) {
    Meta->EraseSet(Option, Data);
  }

  template<typename T>
  T Value<T>::Get(FEXCore::Config::ConfigOption Option) {
    T Result;
    auto Value = FEXCore::Config::Get(Option);

    if (!FEXCore::StrConv::Conv(**Value, &Result)) {
      assert(0 && "Attempted to convert invalid value");
    }
    return Result;
  }

  template<typename T>
  T Value<T>::GetIfExists(FEXCore::Config::ConfigOption Option, T Default) {
    T Result;
    auto Value = FEXCore::Config::Get(Option);

    if (Value && FEXCore::StrConv::Conv(**Value, &Result)) {
      return Result;
    }
    else {
      return Default;
    }
  }

  template<>
  std::string Value<std::string>::GetIfExists(FEXCore::Config::ConfigOption Option, std::string Default) {
    auto Value = FEXCore::Config::Get(Option);
    if (Value) {
      return **Value;
    }
    else {
      return Default;
    }
  }

  template bool Value<bool>::GetIfExists(FEXCore::Config::ConfigOption Option, bool Default);
  template uint8_t Value<uint8_t>::GetIfExists(FEXCore::Config::ConfigOption Option, uint8_t Default);
  template uint64_t Value<uint64_t>::GetIfExists(FEXCore::Config::ConfigOption Option, uint64_t Default);

  // Constructor
  template Value<std::string>::Value(FEXCore::Config::ConfigOption _Option, std::string Default);
  template Value<bool>::Value(FEXCore::Config::ConfigOption _Option, bool Default);
  template Value<uint8_t>::Value(FEXCore::Config::ConfigOption _Option, uint8_t Default);
  template Value<uint64_t>::Value(FEXCore::Config::ConfigOption _Option, uint64_t Default);

  template<typename T>
  void Value<T>::GetListIfExists(FEXCore::Config::ConfigOption Option, std::list<std::string> *List) {
    auto Value = FEXCore::Config::All(Option);
    List->clear();
    if (Value) {
      *List = **Value;
    }
  }
}

