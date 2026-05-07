#include <FEXCore/Config/Config.h>

#include <cstddef>
#include <ntstatus.h>
#include <shlwapi.h>
#include <windef.h>
#include <winternl.h>

namespace FEX::Windows {
std::wstring GetCurrentPath() {
  std::wstring PathStr(4096, L'\0');
  DWORD Result {};

  while (true) {
    Result = GetCurrentDirectoryW(PathStr.size(), PathStr.data());

    // Failure for whatever reason.
    if (Result == 0) {
      break;
    }

    // String fit.
    if (Result < PathStr.size()) {
      PathStr.resize(Result);
      break;
    }

    // String too large.
    if (Result >= PathStr.size()) {
      PathStr.resize(Result);
    }
  }

  return PathStr;
}

void HandleMonoTSOHeuristicConfig(std::wstring_view ExecutableName) {
  // Only use mono heuristic for Steam games.
  bool ShouldDoMonoDetect = getenv("SteamAppId") != nullptr;

  const auto SteamCompat = getenv("STEAM_COMPAT_FEX_CONFIG");
  if (getenv("STEAM_FEX_TSOENABLED")) {
    // If TSO is explicitly controlled by user, then disable heuristic.
    ShouldDoMonoDetect = false;
    LogMan::Msg::DFmt("FEX TSO controlled by user.");
  }

  if (SteamCompat && strstr(SteamCompat, "TSOEnabled:") != nullptr) {
    // If TSO is explicitly controlled by compat config, then disable heuristic.
    ShouldDoMonoDetect = false;
    LogMan::Msg::DFmt("FEX TSO controlled by compat.");
  }

  if (ShouldDoMonoDetect) {
    const auto PathStr = GetCurrentPath();

    // Paths to find mono relative to the current working directory.
    // If the file exists, then this will be a mono+unity game.
    // TODO: This could be extended to Linux native games if the Linux side could detect the file mapping.
    const std::array<const wchar_t*, 2> PathAdders = {
      L"\\Mono\\EmbedRuntime\\mono.dll",
      L"\\MonoBleedingEdge\\EmbedRuntime\\mono-2.0-bdwgc.dll",
    };

    bool IsMono {};
    if (!PathStr.empty()) {
      for (auto adder : PathAdders) {
        auto NewPath = PathStr + adder;
        if (PathFileExistsW(NewPath.c_str())) {
          IsMono = true;
          break;
        }
      }
    }

    if (!IsMono && !ExecutableName.empty()) {
      ExecutableName.remove_suffix(ExecutableName.ends_with(L".exe") ? strlen(".exe") : 0);
      // Older Unity titles stuck the mono.dll in to a sub-directory.
      auto NewPath = PathStr;
      NewPath += L"\\";
      NewPath += ExecutableName;
      NewPath += L"\\";
      NewPath += ExecutableName;
      NewPath += L"_Data\\Mono\\mono.dll";
      if (PathFileExistsW(NewPath.c_str())) {
        LogMan::Msg::DFmt("Found Mono in legacy path.");
        IsMono = true;
      }
    }

    FEX_CONFIG_OPT(MaxInst, MAXINST);
    FEX_CONFIG_OPT(Multiblock, MULTIBLOCK);
    if (IsMono && Multiblock && MaxInst() >= 500) {
      LogMan::Msg::DFmt("Mono has been detected in expected game directory. Turning on Mono TSO heuristic.");
      FEXCore::Config::Set(FEXCore::Config::CONFIG_MONOHACKS, "1");
      FEXCore::Config::Set(FEXCore::Config::CONFIG_TSOENABLED, "0");
    }
  }
}
} // namespace FEX::Windows
