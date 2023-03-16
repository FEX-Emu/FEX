#include "Common/ArgumentLoader.h"
#include "Common/Config.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/SymlinkChecks.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <linux/limits.h>
#include <list>
#include <utility>
#include <json-maker.h>

namespace FEX::Config {
  static const fextl::map<FEXCore::Config::ConfigOption, fextl::string> ConfigToNameLookup = {{
#define OPT_BASE(type, group, enum, json, default) {FEXCore::Config::ConfigOption::CONFIG_##enum, #json},
#include <FEXCore/Config/ConfigValues.inl>
  }};

  void SaveLayerToJSON(const fextl::string& Filename, FEXCore::Config::Layer *const Layer) {
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

    std::ofstream Output (Filename.c_str(), std::ios::out | std::ios::binary);
    if (Output.is_open()) {
      Output.write(Buffer, strlen(Buffer));
      Output.close();
    }
  }

  fextl::string RecoverGuestProgramFilename(fextl::string Program, bool ExecFDInterp, const std::string_view ProgramFDFromEnv) {
    // If executed with a FEX FD then the Program argument might be empty.
    // In this case we need to scan the FD node to recover the application binary that exists on disk.
    // Only do this if the Program argument is empty, since we would prefer the application's expectation
    // of application name.
    if (!ProgramFDFromEnv.empty() && Program.empty()) {
      // Get the `dev` node of the execveat fd string.
      Program = "/dev/fd/";
      Program += ProgramFDFromEnv;
    }

    // If we were provided a relative path then we need to canonicalize it to become absolute.
    // If the program name isn't resolved to an absolute path then glibc breaks inside it's `_dl_get_origin` function.
    // This is because we rewrite `/proc/self/exe` to the absolute program path calculated in here.
    if (!Program.starts_with('/')) {
      Program = std::filesystem::canonical(std::move(Program)).string();
    }

    // If FEX was invoked through an FD path (either binfmt_misc or execveat) then we need to check the
    // Program to see if it is a symlink to find the real path.
    //
    // binfmt_misc: Arg[0] is actually the execve `pathname` argument or `/dev/fd/<FD>` path
    //   - `pathname` with execve (See Side Note)
    //   - FD path with execveat and FD doesn't have an existing file on the disk
    //
    // ProgramFDFromEnv: Arg[0] is Application provided data or `/dev/fd/<FD>` from above fix-up.
    //   - execveat was either passed no arguments (argv=NULL) or the first argument is an empty string (argv[0]="").
    //   - FD path with execveat and FD doesn't have an existing file on the disk
    //
    // Side Note:
    //  The `execve` syscall doesn't take an FD but binfmt_misc will give FEX an FD to execute still.
    //  Arg[0] will always contain the `pathname` argument provided to execve.
    //  It does not resolve symlinks, and it does not convert the path to absolute.
    //
    // Examples:
    //  - Regular execve. Application must exist on disk.
    //    execve binfmt_misc args layout:   `FEXInterpreter <Path provided to execve pathname> <user provided argv[0]> <user provided argv[n]>...`
    //  - Regular execveat with FD. FD is backed by application on disk.
    //    execveat binfmt_misc args layout: `FEXInterpreter <Path provided to execve pathname> <user provided argv[0]> <user provided argv[n]>...`
    //  - Regular execveat with FD. FD points to file on disk that has been deleted.
    //    execveat binfmt_misc args layout: `FEXInterpreter /dev/fd/<FD> <user provided argv[0]> <user provided argv[n]>...`
    if (ExecFDInterp || !ProgramFDFromEnv.empty()) {
      // Only in the case that FEX is executing an FD will the program argument potentially be a symlink.
      // This symlink will be in the style of `/dev/fd/<FD>`.
      //
      // If the argument /is/ a symlink then resolve its path to get the original application name.
      if (FHU::Symlinks::IsSymlink(Program.c_str())) {
        char Filename[PATH_MAX];
        auto SymlinkPath = FHU::Symlinks::ResolveSymlink(Program.c_str(), Filename);
        if (SymlinkPath.starts_with('/')) {
          // This file was executed through an FD.
          // Remove the ` (deleted)` text if the file was deleted after the fact.
          // Otherwise just get the symlink without the deleted text.
          return fextl::string{SymlinkPath.substr(0, SymlinkPath.rfind(" (deleted)"))};
        }
      }
    }

    return Program;
  }

  ApplicationNames LoadConfig(
    bool NoFEXArguments,
    bool LoadProgramConfig,
    int argc,
    char **argv,
    char **const envp,
    bool ExecFDInterp,
    const std::string_view ProgramFDFromEnv) {
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

      Args[0] = RecoverGuestProgramFilename(std::move(Args[0]), ExecFDInterp, ProgramFDFromEnv);
      fextl::string& Program = Args[0];

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

      FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName.string().c_str(), FEXCore::Config::LayerType::LAYER_GLOBAL_APP));
      FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName.string().c_str(), FEXCore::Config::LayerType::LAYER_LOCAL_APP));

      auto SteamID = getenv("SteamAppId");
      if (SteamID) {
        // If a SteamID exists then let's search for Steam application configs as well.
        // We want to key off both the SteamAppId number /and/ the executable since we may not want to thunk all binaries.
        fextl::string SteamAppName = fmt::format("Steam_{}_{}", SteamID, ProgramName.string()).c_str();
        FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_GLOBAL_STEAM_APP));
        FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_LOCAL_STEAM_APP));
      }

      // TODO: No need for conversion once Config uses fextl.
      return ApplicationNames{std::move(Program.c_str()), std::move(ProgramName.c_str())};
    }
    return {};
  }
}
