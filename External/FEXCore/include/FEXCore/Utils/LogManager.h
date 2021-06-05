#pragma once

#include <functional>
#include <cstdarg>
#include <sstream>

#include <fmt/format.h>

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
#define LOGMAN_THROW_A(pred, ...) do { LogMan::Throw::A(pred, __VA_ARGS__); } while (0)
#else
static inline void A(bool, const char*, ...) {}
#define LOGMAN_THROW_A(pred, ...) do {} while (0)
#endif

// Fmt interface

[[noreturn]] void MFmt(const char *fmt, const fmt::format_args& args);

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
template <typename... Args>
static inline void AFmt(bool Value, const char *fmt, const Args&... args) {
  if (MSG_LEVEL < ASSERT || Value) {
    return;
  }
  MFmt(fmt, fmt::make_format_args(args...));
}
#define LOGMAN_THROW_A_FMT(pred, ...) do { LogMan::Throw::AFmt(pred, __VA_ARGS__); } while (0)
#else
static inline void AFmt(bool, const char*, ...) {}
#define LOGMAN_THROW_A_FMT(pred, ...) do {} while (0)
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

// Fmt-capable interface.

__attribute__((visibility("default"))) void MFmtImpl(DebugLevels level, const char* fmt, const fmt::format_args& args);

template <typename... Args>
static inline void MFmt(DebugLevels level, const char* fmt, const Args&... args) {
    MFmtImpl(level, fmt, fmt::make_format_args(args...));
}

template <typename... Args>
static inline void EFmt(const char* fmt, const Args&... args) {
  if (MSG_LEVEL < ERROR) {
    return;
  }
  MFmtImpl(ERROR, fmt, fmt::make_format_args(args...));
}

template <typename... Args>
static inline void DFmt(const char* fmt, const Args&... args) {
  if (MSG_LEVEL < DEBUG) {
    return;
  }
  MFmtImpl(DEBUG, fmt, fmt::make_format_args(args...));
}

template <typename... Args>
static inline void IFmt(const char* fmt, const Args&... args) {
  if (MSG_LEVEL < INFO) {
    return;
  }
  MFmtImpl(INFO, fmt, fmt::make_format_args(args...));
}

template <typename... Args>
static inline void OutFmt(const char* fmt, const Args&... args) {
  MFmtImpl(STDOUT, fmt, fmt::make_format_args(args...));
}

template <typename... Args>
static inline void ErrFmt(const char* fmt, const Args&... args) {
  MFmtImpl(STDERR, fmt, fmt::make_format_args(args...));
}

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
template <typename... Args>
static inline void AFmt(const char *fmt, const Args&... args) {
  if (MSG_LEVEL < ASSERT) {
    return;
  }
  MFmtImpl(ASSERT, fmt, fmt::make_format_args(args...));
  __builtin_trap();
}
#define LOGMAN_MSG_A_FMT(...) do { LogMan::Msg::AFmt(__VA_ARGS__); } while (0)
#else
template <typename... Args>
static inline void AFmt(const char*, const Args&...) {}
#define LOGMAN_MSG_A_FMT(...) do {} while(0)
#endif

#define WARN_ONCE_FMT(...) \
  do { \
    static bool Warned{}; \
    if (!Warned) { \
      LogMan::Msg::DFmt(__VA_ARGS__); \
      Warned = true; \
    } \
  } while (0);

#define ERROR_AND_DIE_FMT(...) \
  do { \
    LogMan::Msg::EFmt(__VA_ARGS__); \
    __builtin_trap(); \
  } while(0)

} // namespace Msg
} // namespace LogMan
