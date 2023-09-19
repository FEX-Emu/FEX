// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|log-manager
$end_info$
*/

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/vector.h>

#include <cstdarg>
#include <cstdio>
#include <malloc.h>

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

void MFmtImpl(DebugLevels level, const char* fmt, const fmt::format_args& args) {
  const auto msg = fextl::fmt::vformat(fmt, args);

  for (auto& Handler : Handlers) {
    Handler(level, msg.c_str());
  }
}

} // namespace Msg
} // namespace LogMan
