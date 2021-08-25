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
#include <unordered_map>
#include <vector>
#include <json-maker.h>

namespace FEX::Config {
  static const std::map<FEXCore::Config::ConfigOption, std::string> ConfigToNameLookup = {{
#define OPT_BASE(type, group, enum, json, default) {FEXCore::Config::ConfigOption::CONFIG_##enum, #json},
#include <FEXCore/Config/ConfigValues.inl>
  }};

  void SaveLayerToJSON(const std::string& Filename, FEXCore::Config::Layer *const Layer) {
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

}
