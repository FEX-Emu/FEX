// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdarg>

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

static inline const char* DebugLevelStr(uint32_t Level) {
  switch (Level) {
  case NONE: return "NONE";
  case ASSERT: return "ASSERT";
  case ERROR: return "ERROR";
  case DEBUG: return "DEBUG";
  case INFO: return "INFO";
  case STDOUT: return "STDOUT";
  case STDERR: return "STDERR";
  default: return "???"; break;
  }
}

constexpr DebugLevels MSG_LEVEL = INFO;

// Note that all logging functions with the Fmt or _FMT suffix on them expect
// format strings as used by fmtlib (or C++ std::format).

namespace Throw {
  using ThrowHandler = void (*)(const char* Message);
  FEX_DEFAULT_VISIBILITY void InstallHandler(ThrowHandler Handler);
  FEX_DEFAULT_VISIBILITY void UnInstallHandler();

  [[noreturn]]
  void MFmt(const char* fmt, const fmt::format_args& args);

// AA_FMT and AAFmt are assume versions of {AA_FMT, AFmt} which will assert in debug builds if the assumption is incorrect.
// In a release build these use __builtin_assume so compilers can optimize around the case that these cases always hold true.
// The assume version should be preferred unless what is being checked has side effects.
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  template<typename... Args>
  static inline void AFmt(bool Value, const char* fmt, const Args&... args) {
    if (MSG_LEVEL < ASSERT || Value) {
      return;
    }
    MFmt(fmt, fmt::make_format_args(args...));
  }
  template<typename... Args>
  static inline void AAFmt(bool Value, const char* fmt, const Args&... args) {
    if (MSG_LEVEL < ASSERT || Value) {
      return;
    }
    MFmt(fmt, fmt::make_format_args(args...));
  }

#define LOGMAN_THROW_A_FMT(pred, ...)       \
  do {                                      \
    LogMan::Throw::AFmt(pred, __VA_ARGS__); \
  } while (0)
#define LOGMAN_THROW_AA_FMT(pred, ...)      \
  do {                                      \
    LogMan::Throw::AFmt(pred, __VA_ARGS__); \
  } while (0)
#else
  static inline void AFmt(bool, const char*, ...) {}
#define LOGMAN_THROW_A_FMT(pred, ...) \
  do {                                \
  } while (0)
  static inline void AAFmt(bool pred, const char*, ...) {
    __builtin_assume(pred);
  }
#define LOGMAN_THROW_AA_FMT(pred, ...) \
  do {                                 \
    __builtin_assume(pred);            \
  } while (0)
#endif

} // namespace Throw

namespace Msg {
  using MsgHandler = void (*)(DebugLevels Level, const char* Message);
  FEX_DEFAULT_VISIBILITY void InstallHandler(MsgHandler Handler);
  FEX_DEFAULT_VISIBILITY void UnInstallHandler();

  // Fmt-capable interface.

  FEX_DEFAULT_VISIBILITY void MFmtImpl(DebugLevels level, const char* fmt, const fmt::format_args& args);

  template<typename... Args>
  static inline void MFmt(DebugLevels level, const char* fmt, const Args&... args) {
    MFmtImpl(level, fmt, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static inline void EFmt(const char* fmt, const Args&... args) {
    if (MSG_LEVEL < ERROR) {
      return;
    }
    MFmtImpl(ERROR, fmt, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static inline void DFmt(const char* fmt, const Args&... args) {
    if (MSG_LEVEL < DEBUG) {
      return;
    }
    MFmtImpl(DEBUG, fmt, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static inline void IFmt(const char* fmt, const Args&... args) {
    if (MSG_LEVEL < INFO) {
      return;
    }
    MFmtImpl(INFO, fmt, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static inline void OutFmt(const char* fmt, const Args&... args) {
    MFmtImpl(STDOUT, fmt, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static inline void ErrFmt(const char* fmt, const Args&... args) {
    MFmtImpl(STDERR, fmt, fmt::make_format_args(args...));
  }

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  template<typename... Args>
  static inline void AFmt(const char* fmt, const Args&... args) {
    if (MSG_LEVEL < ASSERT) {
      return;
    }
    MFmtImpl(ASSERT, fmt, fmt::make_format_args(args...));
    FEX_TRAP_EXECUTION;
  }
#define LOGMAN_MSG_A_FMT(...)       \
  do {                              \
    LogMan::Msg::AFmt(__VA_ARGS__); \
  } while (0)
#else
  template<typename... Args>
  static inline void AFmt(const char*, const Args&...) {}
#define LOGMAN_MSG_A_FMT(...) \
  do {                        \
  } while (0)
#endif

#define WARN_ONCE_FMT(...)            \
  do {                                \
    static bool Warned {};            \
    if (!Warned) {                    \
      LogMan::Msg::DFmt(__VA_ARGS__); \
      Warned = true;                  \
    }                                 \
  } while (0);

#define ERROR_AND_DIE_FMT(...)      \
  do {                              \
    LogMan::Msg::EFmt(__VA_ARGS__); \
    FEX_TRAP_EXECUTION;             \
  } while (0)

} // namespace Msg
} // namespace LogMan
