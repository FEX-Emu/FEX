// SPDX-License-Identifier: MIT
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/File.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Filesystem.h>

#include "Utils/Config.h"

#include <array>
#include <stddef.h>
#include <string_view>
#include <system_error>

namespace FEXCore::Telemetry {
#ifndef FEX_DISABLE_TELEMETRY
std::array<Value, FEXCore::Telemetry::TelemetryType::TYPE_LAST> TelemetryValues = {{}};
const std::array<std::string_view, FEXCore::Telemetry::TelemetryType::TYPE_LAST> TelemetryNames {
  "64byte Split Locks",
  "16byte Split atomics",
  "VEX instructions (AVX)",
  "EVEX instructions (AVX512)",
  "16bit CAS Tear",
  "32bit CAS Tear",
  "64bit CAS Tear",
  "128bit CAS Tear",
  "Crash mask",
  "Write 32-bit Segment ES",
  "Write 32-bit Segment SS",
  "Write 32-bit Segment CS",
  "Write 32-bit Segment DS",
  "Uses 32-bit Segment ES",
  "Uses 32-bit Segment SS",
  "Uses 32-bit Segment CS",
  "Uses 32-bit Segment DS",
  "Non-Canonical 64-bit address access",
};

static bool Enabled {true};
void Initialize() {
  FEX_CONFIG_OPT(DisableTelemetry, DISABLETELEMETRY);
  if (DisableTelemetry) {
    Enabled = false;
    return;
  }

  auto DataDirectory = Config::GetTelemetryDirectory();

  // Ensure the folder structure is created for our configuration
  if (!FHU::Filesystem::Exists(DataDirectory) && !FHU::Filesystem::CreateDirectories(DataDirectory)) {
    LogMan::Msg::IFmt("Couldn't create telemetry Folder");
  }
}

void Shutdown(const fextl::string& ApplicationName) {
  if (!Enabled) {
    return;
  }

  auto DataDirectory = Config::GetTelemetryDirectory() + ApplicationName + ".telem";

  // Retain a single backup if the telemetry already existed.
  auto Backup = DataDirectory + ".bck";

  // Failure on rename is okay.
  (void)FHU::Filesystem::RenameFile(DataDirectory, Backup);

  auto File = FEXCore::File::File(DataDirectory.c_str(),
                                  FEXCore::File::FileModes::WRITE | FEXCore::File::FileModes::CREATE | FEXCore::File::FileModes::TRUNCATE);

  if (File.IsValid()) {
    for (size_t i = 0; i < TelemetryType::TYPE_LAST; ++i) {
      auto& Name = TelemetryNames.at(i);
      auto& Data = TelemetryValues.at(i);
      fextl::fmt::print(File, "{}: {}\n", Name, *Data);
    }
    File.Flush();
  }
}

#endif
} // namespace FEXCore::Telemetry
