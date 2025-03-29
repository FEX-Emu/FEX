// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/string.h>

#include <array>
#include <atomic>
#include <stdint.h>

namespace FEXCore::Telemetry {
enum TelemetryType {
  TYPE_HAS_SPLIT_LOCKS,
  TYPE_16BYTE_SPLIT,
  TYPE_USES_EVEX_OPS,
  TYPE_CAS_16BIT_TEAR,
  TYPE_CAS_32BIT_TEAR,
  TYPE_CAS_64BIT_TEAR,
  TYPE_CAS_128BIT_TEAR,
  TYPE_CRASH_MASK,
  // If a 32-bit application is writing a non-zero value to segments.
  TYPE_WRITES_32BIT_SEGMENT_ES,
  TYPE_WRITES_32BIT_SEGMENT_SS,
  TYPE_WRITES_32BIT_SEGMENT_CS,
  TYPE_WRITES_32BIT_SEGMENT_DS,
  // If a 32-bit application is prefix/using a non-zero segment on memory access.
  TYPE_USES_32BIT_SEGMENT_ES,
  TYPE_USES_32BIT_SEGMENT_SS,
  TYPE_USES_32BIT_SEGMENT_CS,
  TYPE_USES_32BIT_SEGMENT_DS,
  TYPE_UNHANDLED_NONCANONICAL_ADDRESS,
  TYPE_LAST,
};

#ifndef FEX_DISABLE_TELEMETRY
using Value = std::atomic<uint64_t>;

FEX_DEFAULT_VISIBILITY extern std::array<Value, FEXCore::Telemetry::TelemetryType::TYPE_LAST> TelemetryValues;
// This returns the internal structure to the telemetry data structures
// One must be careful with placing these in the hot path of code execution
// It can be fairly costly, especially in the static version where it puts barriers in the code
inline Value& GetTelemetryValue(TelemetryType Type) {
  return FEXCore::Telemetry::TelemetryValues[Type];
}

FEX_DEFAULT_VISIBILITY void Initialize();
FEX_DEFAULT_VISIBILITY void Shutdown(const fextl::string& ApplicationName);

// Telemetry object declaration
// Telemetry ALU operations
// These are typically 3-4 instructions depending on what you're doing
#define FEXCORE_TELEMETRY_SET(Type, Value)                                      \
  do {                                                                          \
    auto& Name = FEXCore::Telemetry::TelemetryValues[FEXCore::Telemetry::Type]; \
    Name = Value;                                                               \
  } while (0)
#define FEXCORE_TELEMETRY_OR(Type, Value)                                       \
  do {                                                                          \
    auto& Name = FEXCore::Telemetry::TelemetryValues[FEXCore::Telemetry::Type]; \
    Name |= Value;                                                              \
  } while (0)
#define FEXCORE_TELEMETRY_INC(Type, Value)                                      \
  do {                                                                          \
    auto& Name = FEXCore::Telemetry::TelemetryValues[FEXCore::Telemetry::Type]; \
    Name++;                                                                     \
  } while (0)

#else
static inline void Initialize() {}
static inline void Shutdown(const fextl::string& ApplicationName) {}

#define FEXCORE_TELEMETRY_INIT(Name, Type)
#define FEXCORE_TELEMETRY(Name, Value) \
  do {                                 \
  } while (0)
#define FEXCORE_TELEMETRY_SET(Name, Value) \
  do {                                     \
  } while (0)
#define FEXCORE_TELEMETRY_OR(Name, Value) \
  do {                                    \
  } while (0)
#define FEXCORE_TELEMETRY_INC(Name) \
  do {                              \
  } while (0)
#endif
} // namespace FEXCore::Telemetry
