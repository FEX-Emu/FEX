#include "Common/StringConv.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <filesystem>
#include <map>
#include <sys/sysinfo.h>

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
    void MergeEnvironmentVariables(ConfigOption const &Option, LayerValue const &Value);
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


  void MetaLayer::MergeEnvironmentVariables(ConfigOption const &Option, LayerValue const &Value) {
    // Environment variables need a bit of additional work
    // We want to merge the arrays rather than overwrite entirely
    auto MetaEnvironment = OptionMap.find(Option);
    if (MetaEnvironment == OptionMap.end()) {
      // Doesn't exist, just insert
      OptionMap.insert_or_assign(Option, Value);
      return;
    }

    // If an environment variable exists in both current meta and in the incoming layer then the meta layer value is overwritten
    std::unordered_map<std::string, std::string> LookupMap;
    const auto AddToMap = [&LookupMap](FEXCore::Config::LayerValue const &Value) {
      for (const auto &EnvVar : Value) {
        const auto ItEq = EnvVar.find_first_of('=');
        if (ItEq == std::string::npos) {
          // Broken environment variable
          // Skip
          continue;
        }
        auto Key = std::string(EnvVar.begin(), EnvVar.begin() + ItEq);
        auto Value = std::string(EnvVar.begin() + ItEq + 1, EnvVar.end());

        // Add the key to the map, overwriting whatever previous value was there
        LookupMap.emplace(std::move(Key), std::move(Value));
      }
    };

    AddToMap(MetaEnvironment->second);
    AddToMap(Value);

    // Now with the two layers merged in the map
    // Add all the values to the option
    Erase(Option);
    for (auto &Val : LookupMap) {
      // Set will emplace multiple options in to its list
      Set(Option, Val.first + "=" + Val.second);
    }
  }

  void MetaLayer::MergeConfigMap(const LayerOptions &Options) {
    // Insert this layer's options, overlaying previous options that exist here
    for (auto &it : Options) {
      if (it.first == FEXCore::Config::ConfigOption::CONFIG_ENV) {
        MergeEnvironmentVariables(it.first, it.second);
      }
      else {
        OptionMap.insert_or_assign(it.first, it.second);
      }
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

  std::string ExpandPath(std::string PathName) {
    if (PathName.empty()) {
      return {};
    }

    std::filesystem::path Path{PathName};

    // Expand home if it exists
    if (Path.is_relative()) {
      std::string Home = getenv("HOME") ?: "";
      // Home expansion only works if it is the first character
      // This matches bash behaviour
      if (PathName.at(0) == '~') {
        PathName.replace(0, 1, Home);
        return PathName;
      }

      // Expand relative path to absolute
      return std::filesystem::absolute(Path);
    }
    return {};
  }

  void ReloadMetaLayer() {
    Meta->Load();

    // Do configuration option fix ups after everything is reloaded
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_THREADS)) {
      FEX_CONFIG_OPT(Cores, THREADS);
      if (Cores == 0) {
        // When the number of emulated CPU cores is zero then auto detect
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_THREADS, std::to_string(get_nprocs_conf()));
      }
    }

    auto ExpandPathIfExists = [](FEXCore::Config::ConfigOption Config, std::string PathName) {
      auto NewPath = ExpandPath(PathName);
      if (!NewPath.empty()) {
        FEXCore::Config::EraseSet(Config, NewPath);
      }
    };

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_ROOTFS)) {
      FEX_CONFIG_OPT(PathName, ROOTFS);
      ExpandPathIfExists(FEXCore::Config::CONFIG_ROOTFS, PathName());
    }
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_THUNKHOSTLIBS)) {
      FEX_CONFIG_OPT(PathName, THUNKHOSTLIBS);
      ExpandPathIfExists(FEXCore::Config::CONFIG_THUNKHOSTLIBS, PathName());
    }
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_THUNKGUESTLIBS)) {
      FEX_CONFIG_OPT(PathName, THUNKGUESTLIBS);
      ExpandPathIfExists(FEXCore::Config::CONFIG_THUNKGUESTLIBS, PathName());
    }
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_THUNKCONFIG)) {
      FEX_CONFIG_OPT(PathName, THUNKCONFIG);
      ExpandPathIfExists(FEXCore::Config::CONFIG_THUNKCONFIG, PathName());
    }
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_OUTPUTLOG)) {
      FEX_CONFIG_OPT(PathName, OUTPUTLOG);
      if (PathName() != "stdout" && PathName() != "stderr") {
        ExpandPathIfExists(FEXCore::Config::CONFIG_OUTPUTLOG, PathName());
      }
    }

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_SINGLESTEP)) {
      // Single stepping also enforces single instruction size blocks
      Set(FEXCore::Config::ConfigOption::CONFIG_MAXINST, std::to_string(1u));
    }
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

