#include "LogManager.h"
#include <sstream>
#include <vector>

namespace LogMan {

namespace Throw {
std::vector<ThrowHandler> Handlers;
void InstallHandler(ThrowHandler Handler) { Handlers.emplace_back(Handler); }

[[noreturn]] void M(std::string const &Message) {
  for (auto Handler : Handlers) {
    Handler(Message);
  }
  __builtin_trap();
}

} // namespace Throw

namespace Msg {
std::vector<MsgHandler> Handlers;
void InstallHandler(MsgHandler Handler) { Handlers.emplace_back(Handler); }

void M(DebugLevels Level, std::string const &Message) {
  for (auto Handler : Handlers) {
    Handler(Level, Message);
  }
}

} // namespace Msg
} // namespace LogMan
