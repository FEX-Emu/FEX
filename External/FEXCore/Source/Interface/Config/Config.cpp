#include "Common/StringConv.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <map>

namespace FEXCore::Config {
  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config) {
  }

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, std::string const &Config) {
  }

  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option) {
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

  template bool     Value<bool>::GetIfExists(FEXCore::Config::ConfigOption Option, bool Default);
  template int8_t   Value<int8_t>::GetIfExists(FEXCore::Config::ConfigOption Option, int8_t Default);
  template uint8_t  Value<uint8_t>::GetIfExists(FEXCore::Config::ConfigOption Option, uint8_t Default);
  template int16_t   Value<int16_t>::GetIfExists(FEXCore::Config::ConfigOption Option, int16_t Default);
  template uint16_t  Value<uint16_t>::GetIfExists(FEXCore::Config::ConfigOption Option, uint16_t Default);
  template int32_t  Value<int32_t>::GetIfExists(FEXCore::Config::ConfigOption Option, int32_t Default);
  template uint32_t Value<uint32_t>::GetIfExists(FEXCore::Config::ConfigOption Option, uint32_t Default);
  template int64_t  Value<int64_t>::GetIfExists(FEXCore::Config::ConfigOption Option, int64_t Default);
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

