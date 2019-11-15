#include <SonicUtils/LogManager.h>
#include <sstream>
#include <vector>

namespace LogMan {

namespace Throw {
std::vector<ThrowHandler> Handlers;
void InstallHandler(ThrowHandler Handler) { Handlers.emplace_back(Handler); }

[[noreturn]] void M(const char *fmt, va_list args) {
  char Buffer[1024];
  vsnprintf(Buffer, 1024, fmt, args);
  for (auto &Handler : Handlers) {
    Handler(Buffer);
  }

  __builtin_trap();
}
} // namespace Throw

namespace Msg {
std::vector<MsgHandler> Handlers;
void InstallHandler(MsgHandler Handler) { Handlers.emplace_back(Handler); }

void M(DebugLevels Level, const char *fmt, va_list args) {
  char Buffer[1024];
  vsnprintf(Buffer, 1024, fmt, args);
  for (auto &Handler : Handlers) {
    Handler(Level, Buffer);
  }
}

} // namespace Msg
} // namespace LogMan
