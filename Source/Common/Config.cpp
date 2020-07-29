#include "Common/Config.h"
#include "LogManager.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <list>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <tiny-json.h>
#include <json-maker.h>

namespace FEX::Config {
  std::unordered_map<std::string, std::string> ConfigMap;

  struct JsonAllocator {
    jsonPool_t PoolObject;
    std::unique_ptr<std::list<json_t>> json_objects;
  };

  std::string GetConfigFileLocation() {
    char const *HomeDir = getenv("HOME");

    if (!HomeDir) {
      HomeDir = getenv("PWD");
    }

    if (!HomeDir) {
      HomeDir = "";
    }

    char const *ConfigXDG = getenv("XDG_CONFIG_HOME");
    std::string ConfigDir = ConfigXDG ? ConfigXDG : HomeDir;
    ConfigDir += "/.fex-emu/";

    // Ensure the folder structure is created for our configuration
    if (!std::filesystem::exists(ConfigDir) &&
        !std::filesystem::create_directories(ConfigDir)) {
      LogMan::Msg::D("Couldn't create config directory: '%s'", ConfigDir.c_str());
      // Let's go local in this case
      return "Config.json";
    }

    std::string ConfigFile = ConfigDir;
    ConfigFile += "Config.json";

    return ConfigFile;
  }

  json_t* PoolInit(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    alloc->json_objects = std::make_unique<std::list<json_t>>();
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  json_t* PoolAlloc(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  bool LoadConfigFile(std::vector<char> &Data) {
    std::fstream ConfigFile;
    ConfigFile.open(GetConfigFileLocation(), std::fstream::in);

    if (!ConfigFile.is_open()) {
      return false;
    }

    if (!ConfigFile.seekg(0, std::fstream::end)) {
      LogMan::Msg::D("Couldn't load configuration file: Seek end");
      return false;
    }
    size_t FileSize = ConfigFile.tellg();
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
      if (ConfigFile.read(&Data.at(0), FileSize)) {
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

  void Init() {
    std::vector<char> Data;
    if (!LoadConfigFile(Data)) {
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

      FEX::Config::Add(ConfigName, ConfigString);
    }
  }

  void Shutdown() {
    char Buffer[4096];
    char *Dest;
    Dest = json_objOpen(Buffer, nullptr);
    Dest = json_objOpen(Dest, "Config");
    for (auto &it : ConfigMap) {
      Dest = json_str(Dest, it.first.data(), it.second.c_str());
    }
    Dest = json_objClose(Dest);
    Dest = json_objClose(Dest);
    json_end(Dest);

    // Write the config to a temporary file first then move it to the correct location
    // This is to solve threading issues when multiple applications are loading and storing the configuration
    std::string Config = GetConfigFileLocation();
    std::string ConfigTmp{Config};
    ConfigTmp.resize(Config.size() + 6);

    auto it = ConfigTmp.rbegin();
    for (int i = 6; i > 0; --i) {
      *it = 'X';
      ++it;
    }

    int tmp = mkstemp(&ConfigTmp.at(0));
    write(tmp, Buffer, strlen(Buffer));
    close(tmp);
    // Rename only works if ther files are in the same filesystem
    rename(ConfigTmp.c_str(), GetConfigFileLocation().c_str());

    ConfigMap.clear();
  }

  void Add(std::string const &Key, std::string_view const Value) {
    ConfigMap[Key] = Value;
  }

  bool Exists(std::string const &Key) {
    return ConfigMap.find(Key) != ConfigMap.end();
  }

  std::string_view Get(std::string const &Key) {
    auto Value = ConfigMap.find(Key);
    if (Value == ConfigMap.end())
      assert(0 && "Not a real config value");
    std::string_view ValueView = Value->second;
    return ValueView;
  }

  std::string_view GetIfExists(std::string const &Key, std::string_view const Default) {
    auto Value = ConfigMap.find(Key);
    if (Value == ConfigMap.end())
      return Default;

    std::string_view ValueView = Value->second;
    return ValueView;
  }
}
