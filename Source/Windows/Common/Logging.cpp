// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/LogManager.h>

#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>

namespace {
void (*WineDbgOut)(const char* Message);

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Output = fextl::fmt::format("[{}][{:X}] {}\n", LogMan::DebugLevelStr(Level), GetCurrentThreadId(), Message);
  WineDbgOut(Output.c_str());
}

void AssertHandler(const char* Message) {
  const auto Output = fextl::fmt::format("[ASSERT] {}\n", Message);
  WineDbgOut(Output.c_str());
}
} // namespace

namespace FEX::Windows::Logging {
void Init() {
  WineDbgOut = reinterpret_cast<decltype(WineDbgOut)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "__wine_dbg_output"));
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
}
} // namespace FEX::Windows::Logging
