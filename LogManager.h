#pragma once
#include <functional>
#include <sstream>
#include <stdarg.h>

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
using ThrowHandler = void(*)(char const *Message);
void InstallHandler(ThrowHandler Handler);

[[noreturn]] void M(const char *fmt, va_list args);

#ifdef NDEBUG
static inline void A(bool, const char*, ...) {}
#else
static inline void A(bool Value, const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT && !Value) {
    va_list args;
    va_start(args, fmt);
    M(fmt, args);
    va_end(args);
  }
}
#endif

} // namespace Throw

namespace Msg {
using MsgHandler = void(*)(DebugLevels Level, char const *Message);
void InstallHandler(MsgHandler Handler);

void M(DebugLevels Level, const char *fmt, va_list args);

#ifdef NDEBUG
static inline void A(const char*, ...) {}
#else
static inline void A(const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT) {
    va_list args;
    va_start(args, fmt);
    M(ASSERT, fmt, args);
    va_end(args);
  }
  __builtin_trap();
}
#endif

static inline void E(const char *fmt, ...) {
  if (MSG_LEVEL >= ERROR) {
    va_list args;
    va_start(args, fmt);
    M(ERROR, fmt, args);
    va_end(args);
  }
}
static inline void D(const char *fmt, ...) {
  if (MSG_LEVEL >= DEBUG) {
    va_list args;
    va_start(args, fmt);
    M(DEBUG, fmt, args);
    va_end(args);
  }
}
static inline void I(const char *fmt, ...) {
  if (MSG_LEVEL >= INFO) {
    va_list args;
    va_start(args, fmt);
    M(INFO, fmt, args);
    va_end(args);
  }
}

static inline void OUT(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  M(STDOUT, fmt, args);
  va_end(args);
}

static inline void ERR(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  M(STDERR, fmt, args);
  va_end(args);
}


} // namespace Msg
} // namespace LogMan
