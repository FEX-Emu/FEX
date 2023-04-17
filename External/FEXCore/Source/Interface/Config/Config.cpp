#include "Common/StringConv.h"
#include "Common/StringUtils.h"
#include "Common/Paths.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CPUInfo.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <array>
#include <assert.h>
#include <cstdlib>
#include <functional>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <tiny-json.h>

namespace FEXCore::Context {
  class Context;
}

namespace FEXCore::Config {
namespace DefaultValues {
#define P(x) x
#define OPT_BASE(type, group, enum, json, default) const P(type) P(enum) = P(default);
#define OPT_STR(group, enum, json, default) const std::string_view P(enum) = P(default);
#define OPT_STRARRAY(group, enum, json, default) OPT_STR(group, enum, json, default)
#include <FEXCore/Config/ConfigValues.inl>
}

namespace JSON {
  struct JsonAllocator {
    jsonPool_t PoolObject;
    fextl::unique_ptr<fextl::list<json_t>> json_objects;
  };
  static_assert(offsetof(JsonAllocator, PoolObject) == 0, "This needs to be at offset zero");

  json_t* PoolInit(jsonPool_t* Pool) {
    JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
    alloc->json_objects = fextl::make_unique<fextl::list<json_t>>();
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  json_t* PoolAlloc(jsonPool_t* Pool) {
    JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  static void LoadJSonConfig(const fextl::string &Config, std::function<void(const char *Name, const char *ConfigSring)> Func) {
    fextl::vector<char> Data;
    if (!FEXCore::FileLoading::LoadFile(Data, Config)) {
      return;
    }

    JsonAllocator Pool {
      .PoolObject = {
        .init = PoolInit,
        .alloc = PoolAlloc,
      },
    };

    json_t const *json = json_createWithPool(&Data.at(0), &Pool.PoolObject);
    if (!json) {
      LogMan::Msg::EFmt("Couldn't create json");
      return;
    }

    json_t const* ConfigList = json_getProperty(json, "Config");

    if (!ConfigList) {
      // This is a non-error if the configuration file exists but no Config section
      return;
    }

    for (json_t const* ConfigItem = json_getChild(ConfigList);
      ConfigItem != nullptr;
      ConfigItem = json_getSibling(ConfigItem)) {
      const char* ConfigName = json_getName(ConfigItem);
      const char* ConfigString = json_getValue(ConfigItem);

      if (!ConfigName) {
        LogMan::Msg::EFmt("Couldn't get config name");
        return;
      }

      if (!ConfigString) {
        LogMan::Msg::EFmt("Couldn't get ConfigString for '{}'", ConfigName);
        return;
      }

      Func(ConfigName, ConfigString);
    }
  }
}

  fextl::string GetDataDirectory() {
    fextl::string DataDir{};

    char const *HomeDir = Paths::GetHomeDirectory();
    char const *DataXDG = getenv("XDG_DATA_HOME");
    char const *DataOverride = getenv("FEX_APP_DATA_LOCATION");
    if (DataOverride) {
      // Data override will override the complete directory
      DataDir = DataOverride;
    }
    else {
      DataDir = DataXDG ?: HomeDir;
      DataDir += "/.fex-emu/";
    }
    return DataDir;
  }

  fextl::string GetConfigDirectory(bool Global) {
    fextl::string ConfigDir;
    if (Global) {
      ConfigDir = GLOBAL_DATA_DIRECTORY;
    }
    else {
      char const *HomeDir = Paths::GetHomeDirectory();
      char const *ConfigXDG = getenv("XDG_CONFIG_HOME");
      char const *ConfigOverride = getenv("FEX_APP_CONFIG_LOCATION");
      if (ConfigOverride) {
        // Config override completely overrides the config directory
        ConfigDir = ConfigOverride;
      }
      else {
        ConfigDir = ConfigXDG ? ConfigXDG : HomeDir;
        ConfigDir += "/.fex-emu/";
      }

      // Ensure the folder structure is created for our configuration
      if (!FHU::Filesystem::Exists(ConfigDir) &&
          !FHU::Filesystem::CreateDirectories(ConfigDir)) {
        // Let's go local in this case
        return "./";
      }
    }

    return ConfigDir;
  }

  fextl::string GetConfigFileLocation(bool Global) {
    fextl::string ConfigFile{};
    if (Global) {
      ConfigFile = GetConfigDirectory(true) + "Config.json";
    }
    else {
      const char *AppConfig = getenv("FEX_APP_CONFIG");
      if (AppConfig) {
        // App config environment variable overwrites only the config file
        ConfigFile = AppConfig;
      }
      else {
        ConfigFile = GetConfigDirectory(false) + "Config.json";
      }
    }
    return ConfigFile;
  }

  fextl::string GetApplicationConfig(const fextl::string &Filename, bool Global) {
    fextl::string ConfigFile = GetConfigDirectory(Global);

    if (!Global &&
        !FHU::Filesystem::Exists(ConfigFile) &&
        !FHU::Filesystem::CreateDirectories(ConfigFile)) {
      LogMan::Msg::DFmt("Couldn't create config directory: '{}'", ConfigFile);
      // Let's go local in this case
      return "./" + Filename + ".json";
    }

    ConfigFile += "AppConfig/";

    // Attempt to create the local folder if it doesn't exist
    if (!Global &&
        !FHU::Filesystem::Exists(ConfigFile) &&
        !FHU::Filesystem::CreateDirectories(ConfigFile)) {
      // Let's go local in this case
      return "./" + Filename + ".json";
    }

    ConfigFile += Filename + ".json";
    return ConfigFile;
  }

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config) {
  }

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, fextl::string const &Config) {
  }

  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option) {
    return 0;
  }

  static fextl::map<FEXCore::Config::LayerType, fextl::unique_ptr<FEXCore::Config::Layer>> ConfigLayers;
  static FEXCore::Config::Layer *Meta{};

  constexpr std::array<FEXCore::Config::LayerType, 9> LoadOrder = {
    FEXCore::Config::LayerType::LAYER_GLOBAL_MAIN,
    FEXCore::Config::LayerType::LAYER_MAIN,
    FEXCore::Config::LayerType::LAYER_GLOBAL_STEAM_APP,
    FEXCore::Config::LayerType::LAYER_GLOBAL_APP,
    FEXCore::Config::LayerType::LAYER_LOCAL_STEAM_APP,
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
    fextl::unordered_map<fextl::string, fextl::string> LookupMap;
    const auto AddToMap = [&LookupMap](FEXCore::Config::LayerValue const &Value) {
      for (const auto &EnvVar : Value) {
        const auto ItEq = EnvVar.find_first_of('=');
        if (ItEq == fextl::string::npos) {
          // Broken environment variable
          // Skip
          continue;
        }
        auto Key = fextl::string(EnvVar.begin(), EnvVar.begin() + ItEq);
        auto Value = fextl::string(EnvVar.begin() + ItEq + 1, EnvVar.end());

        // Add the key to the map, overwriting whatever previous value was there
        LookupMap.insert_or_assign(std::move(Key), std::move(Value));
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
      if (it.first == FEXCore::Config::ConfigOption::CONFIG_ENV ||
          it.first == FEXCore::Config::ConfigOption::CONFIG_HOSTENV) {
        MergeEnvironmentVariables(it.first, it.second);
      }
      else {
        OptionMap.insert_or_assign(it.first, it.second);
      }
    }
  }

  void Initialize() {
    AddLayer(fextl::make_unique<MetaLayer>(FEXCore::Config::LayerType::LAYER_TOP));
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

  fextl::string ExpandPath(fextl::string const &ContainerPrefix, fextl::string PathName) {
    if (PathName.empty()) {
      return {};
    }


    // Expand home if it exists
    if (FHU::Filesystem::IsRelative(PathName)) {
      fextl::string Home = getenv("HOME") ?: "";
      // Home expansion only works if it is the first character
      // This matches bash behaviour
      if (PathName.at(0) == '~') {
        PathName.replace(0, 1, Home);
        return PathName;
      }

      // Expand relative path to absolute
      char ExistsTempPath[PATH_MAX];
      char *RealPath = FHU::Filesystem::Absolute(PathName.c_str(), ExistsTempPath);
      if (RealPath) {
        PathName = RealPath;
      }

      // Only return if it exists
      if (FHU::Filesystem::Exists(PathName)) {
        return PathName;
      }
    }
    else {
      // If the containerprefix and pathname isn't empty
      // Then we check if the pathname exists in our current namespace
      // If the path DOESN'T exist but DOES exist with the prefix applied
      // then redirect to the prefix
      //
      // This might not be expected behaviour for some edge cases but since
      // all paths aren't mounted inside the container, then it'll be fine
      //
      // Main catch case for this is the default thunk install folders
      // HostThunks: $CMAKE_INSTALL_PREFIX/lib/fex-emu/HostThunks/
      // GuestThunks: $CMAKE_INSTALL_PREFIX/share/fex-emu/GuestThunks/
      if (!ContainerPrefix.empty() && !PathName.empty()) {
        if (!FHU::Filesystem::Exists(PathName)) {
          auto ContainerPath = ContainerPrefix + PathName;
          if (FHU::Filesystem::Exists(ContainerPath)) {
            return ContainerPath;
          }
        }
      }
    }
    return {};
  }

  constexpr char ContainerManager[] = "/run/host/container-manager";

  fextl::string FindContainer() {
    // We only support pressure-vessel at the moment
    if (FHU::Filesystem::Exists(ContainerManager)) {
      fextl::vector<char> Manager{};
      if (FEXCore::FileLoading::LoadFile(Manager, ContainerManager)) {
        // Trim the whitespace, may contain a newline
        fextl::string ManagerStr = Manager.data();
        ManagerStr = FEXCore::StringUtils::Trim(ManagerStr);
        return ManagerStr;
      }
    }
    return {};
  }

  fextl::string FindContainerPrefix() {
    // We only support pressure-vessel at the moment
    if (FHU::Filesystem::Exists(ContainerManager)) {
      fextl::vector<char> Manager{};
      if (FEXCore::FileLoading::LoadFile(Manager, ContainerManager)) {
        // Trim the whitespace, may contain a newline
        fextl::string ManagerStr = Manager.data();
        ManagerStr = FEXCore::StringUtils::Trim(ManagerStr);
        if (strncmp(ManagerStr.data(), "pressure-vessel", Manager.size()) == 0) {
          // We are running inside of pressure vessel
          // Our $CMAKE_INSTALL_PREFIX paths are now inside of /run/host/$CMAKE_INSTALL_PREFIX
          return "/run/host/";
        }
      }
    }
    return {};
  }

  void ReloadMetaLayer() {
    Meta->Load();

    // Do configuration option fix ups after everything is reloaded
    {
      // Always fix up the number of threads and create the configuration
      // Otherwise the application could receive zero as the number of threads
      FEX_CONFIG_OPT(Cores, THREADS);
      if (Cores == 0) {
        // When the number of emulated CPU cores is zero then auto detect
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_THREADS, fextl::fmt::format("{}", FEXCore::CPUInfo::CalculateNumberOfCPUs()));
      }
    }

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_CORE)) {
      // Sanitize Core option
      FEX_CONFIG_OPT(Core, CORE);
#if (_M_X86_64)
      constexpr uint32_t MaxCoreNumber = 2;
#else
      constexpr uint32_t MaxCoreNumber = 1;
#endif
#ifdef INTERPRETER_ENABLED
      constexpr uint32_t MinCoreNumber = 0;
#else
      constexpr uint32_t MinCoreNumber = 1;
#endif
      if (Core > MaxCoreNumber || Core < MinCoreNumber) {
        // Sanitize the core option by setting the core to the JIT if invalid
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_CORE, fextl::fmt::format("{}", static_cast<uint32_t>(FEXCore::Config::CONFIG_IRJIT)));
      }
    }

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_CACHEOBJECTCODECOMPILATION)) {
      FEX_CONFIG_OPT(CacheObjectCodeCompilation, CACHEOBJECTCODECOMPILATION);
      FEX_CONFIG_OPT(Core, CORE);

      if (CacheObjectCodeCompilation() && Core() == FEXCore::Config::CONFIG_INTERPRETER) {
        // If running the interpreter then disable cache code compilation
        FEXCore::Config::Erase(FEXCore::Config::CONFIG_CACHEOBJECTCODECOMPILATION);
      }
    }

    fextl::string ContainerPrefix { FindContainerPrefix() };
    auto ExpandPathIfExists = [&ContainerPrefix](FEXCore::Config::ConfigOption Config, fextl::string PathName) {
      auto NewPath = ExpandPath(ContainerPrefix, PathName);
      if (!NewPath.empty()) {
        FEXCore::Config::EraseSet(Config, NewPath);
      }
    };

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_ROOTFS)) {
      FEX_CONFIG_OPT(PathName, ROOTFS);
      auto ExpandedString = ExpandPath(ContainerPrefix,PathName());
      if (!ExpandedString.empty()) {
        // Adjust the path if it ended up being relative
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_ROOTFS, ExpandedString);
      }
      else if (!PathName().empty()) {
        // If the filesystem doesn't exist then let's see if it exists in the fex-emu folder
        fextl::string NamedRootFS = GetDataDirectory() + "RootFS/" + PathName();
        if (FHU::Filesystem::Exists(NamedRootFS)) {
          FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_ROOTFS, NamedRootFS);
        }
      }
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
      auto ExpandedString = ExpandPath(ContainerPrefix, PathName());
      if (!ExpandedString.empty()) {
        // Adjust the path if it ended up being relative
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_THUNKCONFIG, ExpandedString);
      }
      else if (!PathName().empty()) {
        // If the filesystem doesn't exist then let's see if it exists in the fex-emu folder
        fextl::string NamedConfig = GetDataDirectory() + "ThunkConfigs/" + PathName();
        if (FHU::Filesystem::Exists(NamedConfig)) {
          FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_THUNKCONFIG, NamedConfig);
        }
      }
    }
    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_OUTPUTLOG)) {
      FEX_CONFIG_OPT(PathName, OUTPUTLOG);
      if (PathName() != "stdout" && PathName() != "stderr") {
        ExpandPathIfExists(FEXCore::Config::CONFIG_OUTPUTLOG, PathName());
      }
    }

    if (FEXCore::Config::Exists(FEXCore::Config::CONFIG_SINGLESTEP)) {
      // Single stepping also enforces single instruction size blocks
      Set(FEXCore::Config::ConfigOption::CONFIG_MAXINST, "1");
    }
  }

  void AddLayer(fextl::unique_ptr<FEXCore::Config::Layer> _Layer) {
    ConfigLayers.emplace(_Layer->GetLayerType(), std::move(_Layer));
  }

  bool Exists(ConfigOption Option) {
    return Meta->OptionExists(Option);
  }

  std::optional<LayerValue*> All(ConfigOption Option) {
    return Meta->All(Option);
  }

  std::optional<fextl::string*> Get(ConfigOption Option) {
    return Meta->Get(Option);
  }

  void Set(ConfigOption Option, std::string_view Data) {
    Meta->Set(Option, Data);
  }

  void Erase(ConfigOption Option) {
    Meta->Erase(Option);
  }

  void EraseSet(ConfigOption Option, std::string_view Data) {
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
  fextl::string Value<fextl::string>::GetIfExists(FEXCore::Config::ConfigOption Option, fextl::string Default) {
    auto Value = FEXCore::Config::Get(Option);
    if (Value) {
      return **Value;
    }
    else {
      return Default;
    }
  }

  template<>
  fextl::string Value<fextl::string>::GetIfExists(FEXCore::Config::ConfigOption Option, std::string_view Default) {
    auto Value = FEXCore::Config::Get(Option);
    if (Value) {
      return **Value;
    }
    else {
      return fextl::string(Default);
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
  template Value<fextl::string>::Value(FEXCore::Config::ConfigOption _Option, fextl::string Default);
  template Value<bool>::Value(FEXCore::Config::ConfigOption _Option, bool Default);
  template Value<uint8_t>::Value(FEXCore::Config::ConfigOption _Option, uint8_t Default);
  template Value<uint64_t>::Value(FEXCore::Config::ConfigOption _Option, uint64_t Default);

  template<typename T>
  void Value<T>::GetListIfExists(FEXCore::Config::ConfigOption Option, fextl::list<fextl::string> *List) {
    auto Value = FEXCore::Config::All(Option);
    List->clear();
    if (Value) {
      *List = **Value;
    }
  }
  template void Value<fextl::string>::GetListIfExists(FEXCore::Config::ConfigOption Option, fextl::list<fextl::string> *List);

  // Application loaders
  class MainLoader final : public FEXCore::Config::OptionMapper {
  public:
    explicit MainLoader(FEXCore::Config::LayerType Type);
    explicit MainLoader(fextl::string ConfigFile);
    void Load() override;

  private:
    fextl::string Config;
  };

  class AppLoader final : public FEXCore::Config::OptionMapper {
  public:
    explicit AppLoader(const fextl::string& Filename, FEXCore::Config::LayerType Type);
    void Load();

  private:
    fextl::string Config;
  };

  class EnvLoader final : public FEXCore::Config::Layer {
  public:
    explicit EnvLoader(char *const _envp[]);
    void Load() override;

  private:
    char *const *envp;
  };

  static const fextl::map<fextl::string, FEXCore::Config::ConfigOption, std::less<>> ConfigLookup = {{
#define OPT_BASE(type, group, enum, json, default) {#json, FEXCore::Config::ConfigOption::CONFIG_##enum},
#include <FEXCore/Config/ConfigValues.inl>
  }};
  static const fextl::vector<std::pair<const char*, FEXCore::Config::ConfigOption>> EnvConfigLookup = {{
#define OPT_BASE(type, group, enum, json, default) {"FEX_" #enum, FEXCore::Config::ConfigOption::CONFIG_##enum},
#include <FEXCore/Config/ConfigValues.inl>
  }};

  OptionMapper::OptionMapper(FEXCore::Config::LayerType Layer)
    : FEXCore::Config::Layer(Layer) {
  }

  void OptionMapper::MapNameToOption(const char *ConfigName, const char *ConfigString) {
    auto it = ConfigLookup.find(ConfigName);
    if (it != ConfigLookup.end()) {
      Set(it->second, ConfigString);
    }
  }

  MainLoader::MainLoader(FEXCore::Config::LayerType Type)
    : FEXCore::Config::OptionMapper(Type)
    , Config{FEXCore::Config::GetConfigFileLocation(Type == FEXCore::Config::LayerType::LAYER_GLOBAL_MAIN)} {
  }

  MainLoader::MainLoader(fextl::string ConfigFile)
    : FEXCore::Config::OptionMapper(FEXCore::Config::LayerType::LAYER_MAIN)
    , Config{std::move(ConfigFile)} {
  }

  void MainLoader::Load() {
    JSON::LoadJSonConfig(Config, [this](const char *Name, const char *ConfigString) {
      MapNameToOption(Name, ConfigString);
    });
  }

  AppLoader::AppLoader(const fextl::string& Filename, FEXCore::Config::LayerType Type)
    : FEXCore::Config::OptionMapper(Type) {
    const bool Global = Type == FEXCore::Config::LayerType::LAYER_GLOBAL_STEAM_APP ||
                        Type == FEXCore::Config::LayerType::LAYER_GLOBAL_APP;
    Config = FEXCore::Config::GetApplicationConfig(Filename, Global);

    // Immediately load so we can reload the meta layer
    Load();
  }

  void AppLoader::Load() {
    JSON::LoadJSonConfig(Config, [this](const char *Name, const char *ConfigString) {
      MapNameToOption(Name, ConfigString);
    });
  }

  EnvLoader::EnvLoader(char *const _envp[])
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_ENVIRONMENT)
    , envp {_envp} {
  }

  void EnvLoader::Load() {
    using EnvMapType = fextl::unordered_map<std::string_view, std::string_view>;
    EnvMapType EnvMap;

    for(const char *const *pvar=envp; pvar && *pvar; pvar++) {
      std::string_view Var(*pvar);
      size_t pos = Var.rfind('=');
      if (fextl::string::npos == pos)
        continue;

      std::string_view Key = Var.substr(0,pos);
      std::string_view Value {Var.substr(pos+1)};

#define ENVLOADER
#include <FEXCore/Config/ConfigOptions.inl>

      EnvMap[Key] = Value;
    }

    auto GetVar = [](EnvMapType &EnvMap, const std::string_view id)  -> std::optional<std::string_view> {
      if (EnvMap.find(id) != EnvMap.end())
        return EnvMap.at(id);

      // If envp[] was empty, search using std::getenv()
      const char* vs = std::getenv(id.data());
      if (vs) {
        return vs;
      }
      else {
        return std::nullopt;
      }
    };

    std::optional<std::string_view> Value;

    for (auto &it : EnvConfigLookup) {
      if ((Value = GetVar(EnvMap, it.first)).has_value()) {
        Set(it.second, fextl::string(*Value));
      }
    }
  }

  fextl::unique_ptr<FEXCore::Config::Layer> CreateGlobalMainLayer() {
    return fextl::make_unique<FEXCore::Config::MainLoader>(FEXCore::Config::LayerType::LAYER_GLOBAL_MAIN);
  }

  fextl::unique_ptr<FEXCore::Config::Layer> CreateMainLayer(fextl::string const *File) {
    if (File) {
      return fextl::make_unique<FEXCore::Config::MainLoader>(*File);
    }
    else {
      return fextl::make_unique<FEXCore::Config::MainLoader>(FEXCore::Config::LayerType::LAYER_MAIN);
    }
  }

  fextl::unique_ptr<FEXCore::Config::Layer> CreateAppLayer(const fextl::string& Filename, FEXCore::Config::LayerType Type) {
    return fextl::make_unique<FEXCore::Config::AppLoader>(Filename, Type);
  }

  fextl::unique_ptr<FEXCore::Config::Layer> CreateEnvironmentLayer(char *const _envp[]) {
    return fextl::make_unique<FEXCore::Config::EnvLoader>(_envp);
  }
}

