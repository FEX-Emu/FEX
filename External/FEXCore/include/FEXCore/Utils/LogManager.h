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
__attribute__((visibility("default"))) void InstallHandler(ThrowHandler Handler);
__attribute__((visibility("default"))) void UnInstallHandlers();

[[noreturn]] void M(const char *fmt, va_list args);

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
static inline void A(bool Value, const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT && !Value) {
    va_list args;
    va_start(args, fmt);
    M(fmt, args);
    va_end(args);
  }
}
#define LOGMAN_THROW_A(pred, ...) do { LOGMAN_THROW_A(pred, __VA_ARGS__); } while (0)
#else
static inline void A(bool, const char*, ...) {}
#define LOGMAN_THROW_A(pred, ...) do {} while (0)
#endif

} // namespace Throw

namespace Msg {
using MsgHandler = void(*)(DebugLevels Level, char const *Message);
__attribute__((visibility("default"))) void InstallHandler(MsgHandler Handler);
__attribute__((visibility("default"))) void UnInstallHandlers();

__attribute__((visibility("default"))) void M(DebugLevels Level, const char *fmt, va_list args);

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
static inline void A(const char *fmt, ...) {
  if (MSG_LEVEL >= ASSERT) {
    va_list args;
    va_start(args, fmt);
    M(ASSERT, fmt, args);
    va_end(args);
  }
  __builtin_trap();
}
#define LOGMAN_MSG_A(...) do { LogMan::Msg::A(__VA_ARGS__); } while (0)
#else
static inline void A(const char*, ...) {}
#define LOGMAN_MSG_A(...) do {} while(0)
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

#define WARN_ONCE(...) \
  do { \
    static bool Warned{}; \
    if (!Warned) { \
      LogMan::Msg::D(__VA_ARGS__); \
      Warned = true; \
    } \
  } while (0);

#define ERROR_AND_DIE(...) \
  do { \
    LogMan::Msg::E(__VA_ARGS__); \
    __builtin_trap(); \
  } while(0)

} // namespace Msg
} // namespace LogMan
