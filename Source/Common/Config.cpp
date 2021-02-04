#include "Common/Config.h"

#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <list>
#include <string_view>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <tiny-json.h>
#include <json-maker.h>

namespace FEX::Config {
  std::string GetConfigFolder(bool Global) {
    std::string ConfigDir;
    if (Global) {
      ConfigDir = GLOBAL_DATA_DIRECTORY;
    }
    else {
      char const *HomeDir = getenv("HOME");

      if (!HomeDir) {
        HomeDir = getenv("PWD");
      }

      if (!HomeDir) {
        HomeDir = ".";
      }

      char const *ConfigXDG = getenv("XDG_CONFIG_HOME");
      ConfigDir = ConfigXDG ? ConfigXDG : HomeDir;
      ConfigDir += "/.fex-emu/";

      // Ensure the folder structure is created for our configuration
      if (!std::filesystem::exists(ConfigDir) &&
          !std::filesystem::create_directories(ConfigDir)) {
        LogMan::Msg::D("Couldn't create config directory: '%s'", ConfigDir.c_str());
        // Let's go local in this case
        return "./";
      }
    }

    return ConfigDir;
  }

  std::string GetConfigFileLocation() {
    std::string ConfigFile = GetConfigFolder(false) + "Config.json";
    return ConfigFile;
  }

  std::string GetApplicationConfig(std::string &Filename, bool Global) {
    std::string ConfigFile = GetConfigFolder(Global);
    if (!Global &&
        !std::filesystem::exists(ConfigFile) &&
        !std::filesystem::create_directories(ConfigFile)) {
      LogMan::Msg::D("Couldn't create config directory: '%s'", ConfigFile.c_str());
      // Let's go local in this case
      return "./";
    }

    ConfigFile += "AppConfig/" + Filename + ".json";
    return ConfigFile;
  }

  bool LoadConfigFile(std::vector<char> &Data, std::string Config) {
    std::fstream ConfigFile;
    ConfigFile.open(Config, std::ios::in);

    if (!ConfigFile.is_open()) {
      return false;
    }

    if (!ConfigFile.seekg(0, std::fstream::end)) {
      LogMan::Msg::D("Couldn't load configuration file: Seek end");
      return false;
    }

    auto FileSize = ConfigFile.tellg();
    if (ConfigFile.fail()) {
      LogMan::Msg::D("Couldn't load configuration file: tellg");
      return false;
    }

    if (!ConfigFile.seekg(0, std::fstream::beg)) {
      LogMan::Msg::D("Couldn't load configuration file: Seek beginning");
      return false;
    }

    if (FileSize > 0) {
      Data.resize(FileSize);
      if (!ConfigFile.read(&Data.at(0), FileSize)) {
        // Probably means permissions aren't set. Just early exit
        return false;
      }
      ConfigFile.close();
    }
    else {
      return false;
    }

    return true;
  }

  struct JsonAllocator {
    jsonPool_t PoolObject;
    std::unique_ptr<std::list<json_t>> json_objects;
  };

  json_t* PoolInit(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    alloc->json_objects = std::make_unique<std::list<json_t>>();
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  json_t* PoolAlloc(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  void LoadJSonConfig(std::string &Config, std::function<void(const char *Name, const char *ConfigSring)> Func) {
    std::vector<char> Data;
    if (!LoadConfigFile(Data, Config)) {
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
      LogMan::Msg::E("Couldn't create json");
      return;
    }

    json_t const* ConfigList = json_getProperty(json, "Config");

    if (!ConfigList) {
      LogMan::Msg::E("Couldn't get config list");
      return;
    }

    for (json_t const* ConfigItem = json_getChild(ConfigList);
      ConfigItem != nullptr;
      ConfigItem = json_getSibling(ConfigItem)) {
      const char* ConfigName = json_getName(ConfigItem);
      const char* ConfigString = json_getValue(ConfigItem);

      if (!ConfigName) {
        LogMan::Msg::E("Couldn't get config name");
        return;
      }

      if (!ConfigString) {
        LogMan::Msg::E("Couldn't get ConfigString for '%s'", ConfigName);
        return;
      }

      Func(ConfigName, ConfigString);
    }
  }

  static const std::map<FEXCore::Config::ConfigOption, std::string> ConfigToNameLookup = {{
    {FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE,        "Core"},
    {FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST,       "MaxInst"},
    {FEXCore::Config::ConfigOption::CONFIG_SINGLESTEP,         "SingleStep"},
    {FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK,         "Multiblock"},
    {FEXCore::Config::ConfigOption::CONFIG_GDBSERVER,          "GdbServer"},
    {FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES, "Threads"},
    {FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH,         "RootFS"},
    {FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH,      "ThunkLibs"},
    {FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS,         "SilentLog"},
    {FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT,        "Env"},
    {FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG,          "OutputLog"},
    {FEXCore::Config::ConfigOption::CONFIG_DUMPIR,             "DumpIR"},
    {FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED,        "TSOEnabled"},
    {FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS,         "SMCChecks"},
    {FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS,    "ABILocalFlags"},
    {FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF,          "ABINoPF"},
    {FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES, "O0"},
    {FEXCore::Config::ConfigOption::CONFIG_AOTIR_GENERATE,       "AOTIRCapture"},
    {FEXCore::Config::ConfigOption::CONFIG_AOTIR_LOAD,           "AOTIRLoad"},
  }};


  void SaveLayerToJSON(std::string Filename, FEXCore::Config::Layer *const Layer) {
    char Buffer[4096];
    char *Dest{};
    Dest = json_objOpen(Buffer, nullptr);
    Dest = json_objOpen(Dest, "Config");
    for (auto &it : Layer->GetOptionMap()) {
			auto &Name = ConfigToNameLookup.find(it.first)->second;
			for (auto &var : it.second) {
				Dest = json_str(Dest, Name.c_str(), var.c_str());
			}
    }
    Dest = json_objClose(Dest);
    Dest = json_objClose(Dest);
    json_end(Dest);

		std::ofstream Output (Filename, std::ios::out | std::ios::binary);
		if (Output.is_open()) {
			Output.write(Buffer, strlen(Buffer));
			Output.close();
		}
  }

  OptionMapper::OptionMapper(FEXCore::Config::LayerType Layer)
    : FEXCore::Config::Layer(Layer) {
  }

  static const std::map<std::string, FEXCore::Config::ConfigOption> ConfigLookup = {{
    {"Core",          FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE},
    {"MaxInst",       FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST},
    {"SingleStep",    FEXCore::Config::ConfigOption::CONFIG_SINGLESTEP},
    {"Multiblock",    FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK},
    {"GdbServer",     FEXCore::Config::ConfigOption::CONFIG_GDBSERVER},
    {"Threads",       FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES},
    {"RootFS",        FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH},
    {"ThunkLibs",     FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH},
    {"SilentLog",     FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS},
    {"Env",           FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT},
    {"OutputLog",     FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG},
    {"DumpIR",        FEXCore::Config::ConfigOption::CONFIG_DUMPIR},
    {"TSOEnabled",    FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED},
    {"SMCChecks",     FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS},
    {"ABILocalFlags", FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS},
    {"AbiNoPF",       FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF},
    {"O0",            FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES},
    {"AOTIRCapture",   FEXCore::Config::ConfigOption::CONFIG_AOTIR_GENERATE},
    {"AOTIRLoad",       FEXCore::Config::ConfigOption::CONFIG_AOTIR_LOAD},
  }};

  void OptionMapper::MapNameToOption(const char *ConfigName, const char *ConfigString) {
    auto it = ConfigLookup.find(ConfigName);
    if (it != ConfigLookup.end()) {
      Set(it->second, ConfigString);
    }
  }

  MainLoader::MainLoader()
    : FEX::Config::OptionMapper(FEXCore::Config::LayerType::LAYER_MAIN) {
    Config = GetConfigFileLocation();
  }

  MainLoader::MainLoader(std::string ConfigFile)
    : FEX::Config::OptionMapper(FEXCore::Config::LayerType::LAYER_MAIN) {
    Config = ConfigFile;
  }

  void MainLoader::Load() {
    LoadJSonConfig(Config, [this](const char *Name, const char *ConfigString) {
      MapNameToOption(Name, ConfigString);
    });
  }

  AppLoader::AppLoader(std::string Filename, bool Global)
    : FEX::Config::OptionMapper(Global ? FEXCore::Config::LayerType::LAYER_GLOBAL_APP : FEXCore::Config::LayerType::LAYER_LOCAL_APP) {
    Config = GetApplicationConfig(Filename, Global);

    // Immediately load so we can reload the meta layer
    Load();
  }

  void AppLoader::Load() {
    LoadJSonConfig(Config, [this](const char *Name, const char *ConfigString) {
      MapNameToOption(Name, ConfigString);
    });
  }

  EnvLoader::EnvLoader(char *const _envp[])
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_ENVIRONMENT)
    , envp {_envp} {
  }

  void EnvLoader::Load() {
    std::unordered_map<std::string_view, std::string_view> EnvMap;

    for(const char *const *pvar=envp; pvar && *pvar; pvar++) {
      std::string_view Var(*pvar);
      size_t pos = Var.rfind('=');
      if (std::string::npos == pos)
        continue;

      std::string_view Ident = Var.substr(0,pos);
      std::string_view Value = Var.substr(pos+1);
      EnvMap[Ident]=Value;
    }

    std::function GetVar = [=](const std::string_view id)  -> std::optional<std::string_view> {
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

    static const std::array<std::pair<std::string, FEXCore::Config::ConfigOption>, 20> ConfigLookup = {{
      {"FEX_CORE",          FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE},
      {"FEX_MAXINST",       FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST},
      {"FEX_SINGLESTEP",    FEXCore::Config::ConfigOption::CONFIG_SINGLESTEP},
      {"FEX_MULTIBLOCK",    FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK},
      {"FEX_GDBSERVER",     FEXCore::Config::ConfigOption::CONFIG_GDBSERVER},
      {"FEX_THREADS",       FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES},
      {"FEX_ROOTFS",        FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH},
      {"FEX_THUNKLIBS",     FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH},
      {"FEX_SILENTLOG",     FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS},
      {"FEX_ENV",           FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT},
      {"FEX_OUTPUTLOG",     FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG},
      {"FEX_DUMPIR",        FEXCore::Config::ConfigOption::CONFIG_DUMPIR},
      {"FEX_TSOENABLED",    FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED},
      {"FEX_SMCCHECKS",     FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS},
      {"FEX_ABILOCALFLAGS", FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS},
      {"FEX_ABINOPF",       FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF},
      {"FEX_BREAK",         FEXCore::Config::ConfigOption::CONFIG_BREAK_ON_FRONTEND},
      {"FEX_DUMP_GPRS",     FEXCore::Config::ConfigOption::CONFIG_DUMP_GPRS},
      {"FEX_AOT_GENERATE",  FEXCore::Config::ConfigOption::CONFIG_AOTIR_GENERATE},
      {"FEX_AOT_LOAD",      FEXCore::Config::ConfigOption::CONFIG_AOTIR_LOAD},
    }};

    std::optional<std::string_view> Value;

    for (auto &it : ConfigLookup) {
      if ((Value = GetVar(it.first)).has_value()) {
        Set(it.second, std::string(*Value));
      }
    }
  }
}
