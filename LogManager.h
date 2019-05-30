#pragma once
#include <functional>
#include <sstream>

namespace LogMan {
enum DebugLevels {
  NONE = 0,   ///< Expect zero messages
  ASSERT = 1, ///< Assert throwing
  ERROR = 2,  ///< Only Errors printed
  DEBUG = 3,  ///< Debug messages added
  INFO = 4,   ///< Info messages added
  STDOUT = 5, ///< Meant to go to STDOUT
  STDERR = 6, ///< Meant to go to STDERR
};

constexpr DebugLevels MSG_LEVEL = INFO;

namespace Throw {
using ThrowHandler = std::function<void(std::string const &Message)>;
void InstallHandler(ThrowHandler Handler);

[[noreturn]] void M(const char *fmt, va_list args);

[[maybe_unused]] static void A(bool Value, const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT && !Value) {
    va_list args;
    va_start(args, fmt);
    M(fmt, args);
    va_end(args);
  }
}

} // namespace Throw

namespace Msg {
using MsgHandler =
    std::function<void(DebugLevels Level, std::string const &Message)>;
void InstallHandler(MsgHandler Handler);

void M(DebugLevels Level, const char *fmt, va_list args);

[[maybe_unused]] static void A(const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT) {
    va_list args;
    va_start(args, fmt);
    M(ASSERT, fmt, args);
    va_end(args);
  }
  __builtin_trap();
}
[[maybe_unused]] static void E(const char *fmt, ...) {
  if (MSG_LEVEL >= ERROR) {
    va_list args;
    va_start(args, fmt);
    M(ERROR, fmt, args);
    va_end(args);
  }
}
[[maybe_unused]] static void D(const char *fmt, ...) {
  if (MSG_LEVEL >= DEBUG) {
    va_list args;
    va_start(args, fmt);
    M(DEBUG, fmt, args);
    va_end(args);
  }
}
[[maybe_unused]] static void I(const char *fmt, ...) {
  if (MSG_LEVEL >= INFO) {
    va_list args;
    va_start(args, fmt);
    M(INFO, fmt, args);
    va_end(args);
  }
}

[[maybe_unused]] static void OUT(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  M(STDOUT, fmt, args);
  va_end(args);
}

[[maybe_unused]] static void ERR(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  M(STDERR, fmt, args);
  va_end(args);
}


} // namespace Msg
} // namespace LogMan
