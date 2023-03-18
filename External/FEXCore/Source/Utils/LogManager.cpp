/*
$info$
tags: glue|log-manager
$end_info$
*/

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/vector.h>

#include <alloca.h>
#include <cstdarg>
#include <cstdio>

namespace LogMan {

namespace Throw {
fextl::vector<ThrowHandler> Handlers;
void InstallHandler(ThrowHandler Handler) { Handlers.emplace_back(Handler); }
void UnInstallHandlers() { Handlers.clear(); }

void MFmt(const char *fmt, const fmt::format_args& args) {
  auto msg = fextl::fmt::vformat(fmt, args);

  for (auto& Handler : Handlers) {
    Handler(msg.c_str());
  }

  FEX_TRAP_EXECUTION;
}
} // namespace Throw

namespace Msg {
fextl::vector<MsgHandler> Handlers;
void InstallHandler(MsgHandler Handler) { Handlers.emplace_back(Handler); }
void UnInstallHandlers() { Handlers.clear(); }

static void M(DebugLevels Level, const char *fmt, va_list args) {
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

void D(const char *fmt, ...) {
  if (MSG_LEVEL < DEBUG) {
    return;
  }

  va_list args;
  va_start(args, fmt);
  M(DEBUG, fmt, args);
  va_end(args);
}

void MFmtImpl(DebugLevels level, const char* fmt, const fmt::format_args& args) {
  const auto msg = fextl::fmt::vformat(fmt, args);

  for (auto& Handler : Handlers) {
    Handler(level, msg.c_str());
  }
}

} // namespace Msg
} // namespace LogMan
