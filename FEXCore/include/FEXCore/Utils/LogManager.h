// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdarg>

#include <fmt/format.h>
#include <fmt/color.h>

namespace LogMan {
enum DebugLevels {
  NONE = 0,   ///< Expect zero messages
  ASSERT = 1, ///< Assert throwing
  ERROR = 2,  ///< Only Errors printed
  DEBUG = 3,  ///< Debug messages added
  INFO = 4,   ///< Info messages added
};

static inline const char* DebugLevelStr(uint32_t Level) {
  switch (Level) {
  case NONE: return "NONE";
  case ASSERT: return "A";
  case ERROR: return "E";
  case DEBUG: return "D";
  case INFO: return "I";
  default: return "???"; break;
  }
}

static inline fmt::text_style DebugLevelStyle(uint32_t Level) {
  switch (Level) {
  case LogMan::ASSERT: return fmt::bg(fmt::color::red) | fmt::emphasis::bold | fmt::fg(fmt::color::white);
  case LogMan::ERROR: return fmt::fg(fmt::color::red);
  case LogMan::DEBUG: return fmt::fg(fmt::color::gray);
  case LogMan::INFO: return fmt::fg(fmt::color::green);
  default: return {}; break;
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

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  template<typename... Args>
  static inline void AFmt(bool Value, const char* fmt, const Args&... args) {
    if (MSG_LEVEL < ASSERT || Value) {
      return;
    }
    MFmt(fmt, fmt::make_format_args(args...));
  }

#define LOGMAN_THROW_A_FMT(pred, format, ...)                                                                             \
  do {                                                                                                                    \
    if (!(pred)) {                                                                                                        \
      LogMan::Throw::AFmt(false, "{}:{}, {}: " format, __FILE_NAME__, __LINE__, __FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
    }                                                                                                                     \
  } while (0)
#else
  static inline void AFmt(bool, const char*, ...) {}
#define LOGMAN_THROW_A_FMT(pred, ...) \
  do {                                \
    (void)(pred);                     \
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

#define ERROR_AND_DIE_FMT(...)                      \
  do {                                              \
    LogMan::Msg::MFmt(LogMan::ASSERT, __VA_ARGS__); \
    FEX_TRAP_EXECUTION;                             \
  } while (0)

} // namespace Msg
} // namespace LogMan
