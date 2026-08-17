// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEXCompatTool
desc: Used for launching games from Steam
$end_info$
*/

#include "PortabilityInfo.h"
#include "Common/Config.h"
#include "FEXCore/Utils/FileLoading.h"
#include "FEXCore/Utils/StringUtils.h"
#include "FEXHeaderUtils/Filesystem.h"

#include <optional>
#include <stdlib.h>
#include <tiny-json.h>

fextl::string GenerateSteamConfigTemplate(const FEX::Config::PortableInformation& PortableInfo) {
  const auto ConfigTemplatePath = PortableInfo.InterpreterPath + "ConfigTemplate.json";
  if (!FHU::Filesystem::Exists(ConfigTemplatePath)) {
    return {};
  }

  fextl::string Data;
  if (!FEXCore::FileLoading::LoadFile(Data, ConfigTemplatePath)) {
    return {};
  }

  fextl::string MountPoint {};

  // If the graphics provider was provided through an environment variable, then use that.
  // Otherwise, try to find a mount point.
  const char* GraphicsProvider = getenv("STEAM_COMPAT_GRAPHICS_PROVIDER");
  const char* RuntimeDir = getenv("XDG_RUNTIME_DIR");
  const char* CacheDir = getenv("XDG_CACHE_HOME");
  const auto UserDirectory = fextl::fmt::format("/run/user/{}", geteuid());
  if (GraphicsProvider) {
    MountPoint = FHU::Filesystem::ParentPath(GraphicsProvider);
  } else if (RuntimeDir) {
    MountPoint = fextl::fmt::format("{}/fexrootfs/", RuntimeDir);
  } else if (FHU::Filesystem::Exists(UserDirectory)) {
    MountPoint = fextl::fmt::format("{}/fexrootfs/", UserDirectory);
  } else if (CacheDir) {
    MountPoint = fextl::fmt::format("{}/fexrootfs/", CacheDir);
  } else {
    // We tried really hard to find a mount path.
    MountPoint = "~/.cache/fexrootfs/";
  }

  // Update the @FEX_COMPAT_TOOL@ config to point to the root of the depot.
  FEXCore::StringUtils::ReplaceAllInPlace(Data, "@FEX_COMPAT_TOOL@", PortableInfo.InterpreterPath);

  // TODO: This path is getting phased out.
  FEXCore::StringUtils::ReplaceAllInPlace(Data, "@FEX_ROOTFS_PATH@", MountPoint);

  // Save the json.
  const auto ConfigPath = FEX::Config::GetConfigDirectory(false, PortableInfo);
  const auto ConfigLocation = ConfigPath + "Config.json";
  if (!FHU::Filesystem::CreateDirectories(ConfigPath)) {
    return {};
  }

  auto File = FEXCore::File::File(ConfigLocation.c_str(),
                                  FEXCore::File::FileModes::WRITE | FEXCore::File::FileModes::CREATE | FEXCore::File::FileModes::TRUNCATE);

  if (!File.IsValid()) {
    return {};
  }

  File.Write(Data.data(), Data.size());
  return ConfigPath;
}

fextl::string GenerateSteamAppConfig(const FEX::Config::PortableInformation& PortableInfo) {
  const auto user_config = getenv("FEX_APP_CONFIG");
  if (user_config) {
    // If user supplied config then don't use Steam config.
    return {};
  }

  // Current supported Steam options.
  struct SteamOptions {
    std::optional<bool> TSO {};
    std::optional<bool> Multiblock {};
    std::optional<bool> Thunks_GL {};
    std::optional<bool> Thunks_Vulkan {};
    std::optional<bool> EnableLogging {};
  };
  SteamOptions Options {};

  // Game overrides.
  const auto steam_fex_tso = getenv("STEAM_FEX_TSOENABLED");
  if (steam_fex_tso) {
    Options.TSO = std::strtoull(steam_fex_tso, nullptr, 0) != 0;
  }

  const auto steam_fex_multiblock = getenv("STEAM_FEX_MULTIBLOCK");
  if (steam_fex_multiblock) {
    Options.Multiblock = std::strtoull(steam_fex_multiblock, nullptr, 0) != 0;
  }

  const auto steam_fex_logging = getenv("STEAM_FEX_LOG");
  if (steam_fex_logging) {
    Options.EnableLogging = std::strtoull(steam_fex_logging, nullptr, 0) != 0;
  }

  // UI overrides.
  const auto steam_fex_compat = getenv("STEAM_COMPAT_FEX_CONFIG");
  if (steam_fex_compat) {
    const auto steam_fex_compat_view = std::string_view(steam_fex_compat);

    // Return a tristate for Steam options.
    // - Exists: Value must be 1 or 0 to set the boolean.
    // - Doesn't exist: std::optional is unset and config option isn't emitted.
    auto get_bool_flag = [steam_fex_compat_view](std::string_view option) -> std::optional<bool> {
      const auto pos = steam_fex_compat_view.find(option);
      const auto value_pos = pos + option.size();

      // If the key isn't found, or the value position would be beyond the end of the view, then it's not set.
      if (pos == steam_fex_compat_view.npos || value_pos >= steam_fex_compat_view.size()) {
        return std::nullopt;
      }

      const auto value = steam_fex_compat_view[value_pos];
      if (value == '1') {
        return true;
      } else if (value == '0') {
        return false;
      }

      // Give an error on invalid boolean key.
      fextl::fmt::print(stderr, "Invalid option for bool config {}. Got {}\n", option, value);
      return std::nullopt;
    };

    Options.TSO = get_bool_flag("TSOEnabled:");
    Options.Multiblock = get_bool_flag("Multiblock:");
    Options.Thunks_GL = get_bool_flag("ThunksDB_GL:");
    Options.Thunks_Vulkan = get_bool_flag("ThunksDB_Vulkan:");
  }

  // Create the json.
  char Buffer[4096];
  char* Dest {};
  Dest = json_objOpen(Buffer, nullptr);
  {
    Dest = json_objOpen(Dest, "Config");
    if (Options.TSO) {
      Dest = json_str(Dest, "TSOEnabled", Options.TSO.value() ? "1" : "0");
    }
    if (Options.Multiblock) {
      Dest = json_str(Dest, "Multiblock", Options.Multiblock.value() ? "1" : "0");
    }

    if (Options.EnableLogging) {
      Dest = json_str(Dest, "SilentLog", Options.EnableLogging.value() ? "0" : "1");
      if (Options.EnableLogging.value()) {
        Dest = json_str(Dest, "OutputLog", "server");
      }
    }
    Dest = json_objClose(Dest);
  }

  if (Options.Thunks_GL || Options.Thunks_Vulkan) {
    Dest = json_objOpen(Dest, "ThunksDB");
    if (Options.Thunks_GL) {
      Dest = json_str(Dest, "GL", Options.Thunks_GL.value() ? "1" : "0");
    }
    if (Options.Thunks_Vulkan) {
      Dest = json_str(Dest, "Vulkan", Options.Thunks_Vulkan.value() ? "1" : "0");
    }
    Dest = json_objClose(Dest);
  }

  Dest = json_objClose(Dest);
  json_end(Dest);

  // Save the json.
  const auto ConfigPath = FEX::Config::GetConfigDirectory(false, PortableInfo);
  const auto ConfigLocation = ConfigPath + "app_config.json";
  if (!FHU::Filesystem::CreateDirectories(ConfigPath)) {
    return {};
  }

  auto File = FEXCore::File::File(ConfigLocation.c_str(),
                                  FEXCore::File::FileModes::WRITE | FEXCore::File::FileModes::CREATE | FEXCore::File::FileModes::TRUNCATE);

  if (!File.IsValid()) {
    return {};
  }

  File.Write(Buffer, strlen(Buffer));
  return ConfigLocation;
}

int main(int argc, const char** argv) {
  const auto PortableInfo = FEX::ReadPortabilityInformation();

  const auto TemplateConfigPath = GenerateSteamConfigTemplate(PortableInfo);
  const auto AppConfigPath = GenerateSteamAppConfig(PortableInfo);

  if (!TemplateConfigPath.empty()) {
    setenv("FEX_APP_CONFIG_LOCATION", TemplateConfigPath.c_str(), true);
  }

  if (!AppConfigPath.empty()) {
    setenv("FEX_APP_CONFIG", AppConfigPath.c_str(), true);
  }

  const auto FEXInterpreterPath = PortableInfo.InterpreterPath + "usr/bin/FEX";

  // Due to no arguments for this application, just replace argv[0] and execve again.
  argv[0] = FEXInterpreterPath.c_str();
  execv(FEXInterpreterPath.c_str(), const_cast<char* const*>(argv));

  // Save errno as it can change after calling `perror`.
  const auto saved_errno = errno;

  perror(argv[0]);

  if (saved_errno == ENOENT) {
    return 127;
  }

  return 126;
}
