#include "Common/Config.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <memory>
#include <list>
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

  json_t* PoolInit(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    alloc->json_objects = std::make_unique<std::list<json_t>>();
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  json_t* PoolAlloc(jsonPool_t* Pool) {
    JsonAllocator* alloc = json_containerOf(Pool, JsonAllocator, PoolObject);
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  void Init() {
    JsonAllocator Pool {
      .PoolObject = {
        .init = PoolInit,
        .alloc = PoolAlloc,
      },
    };

    std::fstream ConfigFile;
    std::vector<char> Data;
    ConfigFile.open("Config.json", std::fstream::in);

    if (ConfigFile.is_open()) {
      ConfigFile.seekg(0, std::fstream::end);
      size_t FileSize = ConfigFile.tellg();
      ConfigFile.seekg(0, std::fstream::beg);

      if (FileSize > 0) {
        Data.resize(FileSize);
        ConfigFile.read(&Data.at(0), FileSize);
        ConfigFile.close();

        json_t const *json = json_createWithPool(&Data.at(0), &Pool.PoolObject);
        json_t const* ConfigList = json_getProperty(json, "Config");

        for (json_t const* ConfigItem = json_getChild(ConfigList);
          ConfigItem != nullptr;
          ConfigItem = json_getSibling(ConfigItem)) {
          const char* ConfigName = json_getName(ConfigItem);
          const char* ConfigString = json_getValue(ConfigItem);

          FEX::Config::Add(ConfigName, ConfigString);
        }
      }
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

    std::fstream ConfigFile;
    ConfigFile.open("Config.json", std::fstream::out);

    if (ConfigFile.is_open()) {
      ConfigFile.write(Buffer, strlen(Buffer));
      ConfigFile.close();
    }

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
