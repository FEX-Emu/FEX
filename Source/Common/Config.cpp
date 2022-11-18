#include "Common/ArgumentLoader.h"
#include "Common/Config.h"

#include <FEXCore/Config/Config.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <list>
#include <unordered_map>
#include <utility>
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

  std::pair<std::string, std::string> LoadConfig(
    bool NoFEXArguments,
    bool LoadProgramConfig,
    int argc,
    char **argv,
    char **const envp) {
    FEXCore::Config::Initialize();
    FEXCore::Config::AddLayer(FEXCore::Config::CreateGlobalMainLayer());
    FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());

    if (NoFEXArguments) {
      FEX::ArgLoader::LoadWithoutArguments(argc, argv);
    }
    else {
      FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
    }

    FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
    FEXCore::Config::Load();

    auto Args = FEX::ArgLoader::Get();

    if (LoadProgramConfig) {
      if (Args.empty()) {
        // Early exit if we weren't passed an argument
        return {};
      }

      std::string Program = Args[0];

      bool Wine = false;
      std::filesystem::path ProgramName;
      for (size_t CurrentProgramNameIndex = 0; CurrentProgramNameIndex < Args.size(); ++CurrentProgramNameIndex) {
        auto CurrentProgramName = std::filesystem::path(Args[CurrentProgramNameIndex]).filename();

        if (CurrentProgramName == "wine-preloader" ||
            CurrentProgramName == "wine64-preloader") {
          // Wine preloader is required to be in the format of `wine-preloader <wine executable>`
          // The preloader doesn't execve the executable, instead maps it directly itself
          // Skip the next argument since we know it is wine (potentially with custom wine executable name)
          ++CurrentProgramNameIndex;
          Wine = true;
        }
        else if(CurrentProgramName == "wine" ||
                CurrentProgramName == "wine64") {
          // Next argument, this isn't the program we want
          //
          // If we are running wine or wine64 then we should check the next argument for the application name instead.
          // wine will change the active program name with `setprogname` or `prctl(PR_SET_NAME`.
          // Since FEX needs this data far earlier than libraries we need a different check.
          Wine = true;
        }
        else {
          if (Wine == true) {
            // If this was path separated with '\' then we need to check that.
            auto WinSeparator = CurrentProgramName.string().find_last_of('\\');
            if (WinSeparator != CurrentProgramName.string().npos) {
              // Used windows separators
              CurrentProgramName = CurrentProgramName.string().substr(WinSeparator + 1);
            }
          }

          ProgramName = CurrentProgramName;

          // Past any wine program names
          break;
        }
      }

      FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, FEXCore::Config::LayerType::LAYER_GLOBAL_APP));
      FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, FEXCore::Config::LayerType::LAYER_LOCAL_APP));

      auto SteamID = getenv("SteamAppId");
      if (SteamID) {
        // If a SteamID exists then let's search for Steam application configs as well.
        // We want to key off both the SteamAppId number /and/ the executable since we may not want to thunk all binaries.
        auto SteamAppName = fmt::format("Steam_{}_{}", SteamID, ProgramName.string());
        FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_GLOBAL_STEAM_APP));
        FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_LOCAL_STEAM_APP));
      }

      return std::make_pair(Program, ProgramName);
    }
    return {};
  }
}
