#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <stddef.h>
#include <string_view>
#include <system_error>

namespace FEXCore::Telemetry {
#ifndef FEX_DISABLE_TELEMETRY
  static std::array<Value, FEXCore::Telemetry::TelemetryType::TYPE_LAST> TelemetryValues = {{ }};
  const std::array<std::string_view, FEXCore::Telemetry::TelemetryType::TYPE_LAST> TelemetryNames {
    "64byte Split Locks",
    "16byte Split atomics",
    "VEX instructions (AVX)",
    "EVEX instructions (AVX512)",
    "16bit CAS Tear",
    "32bit CAS Tear",
    "64bit CAS Tear",
    "128bit CAS Tear",
  };
  void Initialize() {
    auto DataDirectory = Config::GetDataDirectory();
    DataDirectory += "Telemetry/";

    // Ensure the folder structure is created for our configuration
    if (!FHU::Filesystem::Exists(DataDirectory) &&
        !FHU::Filesystem::CreateDirectories(DataDirectory)) {
      LogMan::Msg::IFmt("Couldn't create telemetry Folder");
    }
  }

  void Shutdown(fextl::string const &ApplicationName) {
    auto DataDirectory = Config::GetDataDirectory();
    DataDirectory += "Telemetry/" + ApplicationName + ".telem";

    if (FHU::Filesystem::Exists(DataDirectory)) {
      // If the file exists, retain a single backup
      auto Backup = DataDirectory + ".1";
      FHU::Filesystem::CopyFile(DataDirectory, Backup, FHU::Filesystem::CopyOptions::OVERWRITE_EXISTING);
    }

    constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
    int fd = open(DataDirectory.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, USER_PERMS);

    if (fd != -1) {
      for (size_t i = 0; i < TelemetryType::TYPE_LAST; ++i) {
        auto &Name = TelemetryNames.at(i);
        auto &Data = TelemetryValues.at(i);
        auto Output = fextl::fmt::format("{}: {}\n", Name, *Data);
        write(fd, Output.c_str(), Output.size());
      }
      fsync(fd);
      close(fd);
    }
  }

  Value &GetObject(TelemetryType Type) {
    return TelemetryValues.at(Type);
  }
#endif
}
