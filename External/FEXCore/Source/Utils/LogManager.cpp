/*
$info$
tags: glue|log-manager
$end_info$
*/

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <alloca.h>
#include <fmt/format.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace LogMan {

namespace Throw {
std::vector<ThrowHandler> Handlers;
void InstallHandler(ThrowHandler Handler) { Handlers.emplace_back(Handler); }
void UnInstallHandlers() { Handlers.clear(); }

[[noreturn]] void M(const char *fmt, va_list args) {
  size_t MsgSize = 1024;
  char *Buffer = reinterpret_cast<char*>(alloca(MsgSize));
  va_list argsCopy;
  va_copy(argsCopy, args);
  size_t Return = vsnprintf(Buffer, MsgSize, fmt, argsCopy);
  va_end(argsCopy);
  if (Return >= MsgSize) {
    // Allocate a bigger size on failure
    MsgSize = Return;
    Buffer = reinterpret_cast<char*>(alloca(MsgSize));
    va_end(argsCopy);
    va_copy(argsCopy, args);
    vsnprintf(Buffer, MsgSize, fmt, argsCopy);
    va_end(argsCopy);
  }

  for (auto &Handler : Handlers) {
    Handler(Buffer);
  }

  FEX_TRAP_EXECUTION;
}

void MFmt(const char *fmt, const fmt::format_args& args) {
  auto msg = fmt::vformat(fmt, args);

  for (auto& Handler : Handlers) {
    Handler(msg.c_str());
  }

  FEX_TRAP_EXECUTION;
}
} // namespace Throw

namespace Msg {
std::vector<MsgHandler> Handlers;
void InstallHandler(MsgHandler Handler) { Handlers.emplace_back(Handler); }
void UnInstallHandlers() { Handlers.clear(); }

void M(DebugLevels Level, const char *fmt, va_list args) {
  size_t MsgSize = 1024;
  char *Buffer = reinterpret_cast<char*>(alloca(MsgSize));
  va_list argsCopy;
  va_copy(argsCopy, args);
  size_t Return = vsnprintf(Buffer, MsgSize, fmt, argsCopy);
  va_end(argsCopy);
  if (Return >= MsgSize) {
    // Allocate a bigger size on failure
    MsgSize = Return;
    Buffer = reinterpret_cast<char*>(alloca(MsgSize));
    va_end(argsCopy);
    va_copy(argsCopy, args);
    vsnprintf(Buffer, MsgSize, fmt, argsCopy);
    va_end(argsCopy);
  }
  for (auto &Handler : Handlers) {
    Handler(Level, Buffer);
  }
}

void MFmtImpl(DebugLevels level, const char* fmt, const fmt::format_args& args) {
  const auto msg = fmt::vformat(fmt, args);

  for (auto& Handler : Handlers) {
    Handler(level, msg.c_str());
  }
}

} // namespace Msg
} // namespace LogMan
