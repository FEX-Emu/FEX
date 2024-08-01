// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdio>
#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>

namespace {
void (*WineDbgOut)(const char* Message);
FILE* LogFile;

static void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Output = fextl::fmt::format("[{}][{:X}] {}\n", LogMan::DebugLevelStr(Level), GetCurrentThreadId(), Message);
  if (WineDbgOut) {
    WineDbgOut(Output.c_str());
  } else {
    fwrite(Output.c_str(), 1, Output.size(), LogFile);
  }
}

static void AssertHandler(const char* Message) {
  const auto Output = fextl::fmt::format("[ASSERT] {}\n", Message);
  if (WineDbgOut) {
    WineDbgOut(Output.c_str());
  } else {
    fwrite(Output.c_str(), 1, Output.size(), LogFile);
  }
}
} // namespace

namespace FEX::Windows::Logging {
void Init() {
  WineDbgOut = reinterpret_cast<decltype(WineDbgOut)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "__wine_dbg_output"));
  if (!WineDbgOut) {
    const auto Path = fextl::fmt::format("{}\\fex-{}.log", getenv("LOCALAPPDATA"), GetCurrentProcessId());
    LogFile = fopen(Path.c_str(), "a");
  }
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
}
} // namespace FEX::Windows::Logging
