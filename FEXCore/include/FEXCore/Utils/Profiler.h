// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string_view>
#include <time.h>

#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::Profiler {
#ifdef ENABLE_FEXCORE_PROFILER

FEX_DEFAULT_VISIBILITY void Init();
FEX_DEFAULT_VISIBILITY void Shutdown();
FEX_DEFAULT_VISIBILITY void TraceObject(std::string_view const Format);
FEX_DEFAULT_VISIBILITY void TraceObject(std::string_view const Format, uint64_t Duration);

static inline uint64_t GetTime() {
  // We want the time in the least amount of overhead possible
  // clock_gettime will do a VDSO call with the least amount of overhead
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec;
}

// A class that follows scoping rules to generate a profile duration block
class ProfilerBlock final {
public:
  ProfilerBlock(std::string_view const Format);

  ~ProfilerBlock();

private:
  uint64_t DurationBegin;
  std::string_view const Format;
};

#define UniqueScopeName2(name, line) name##line
#define UniqueScopeName(name, line) UniqueScopeName2(name, line)

// Declare an instantaneous profiler event.
#define FEXCORE_PROFILE_INSTANT(name) FEXCore::Profiler::TraceObject(name)

// Declare a scoped profile block variable with a fixed name.
#define FEXCORE_PROFILE_SCOPED(name) FEXCore::Profiler::ProfilerBlock UniqueScopeName(ScopedBlock_, __LINE__)(name)

#else
[[maybe_unused]]
static void Init() {}
[[maybe_unused]]
static void Shutdown() {}
[[maybe_unused]]
static void TraceObject(std::string_view const Format) {}
[[maybe_unused]]
static void TraceObject(std::string_view const, uint64_t) {}

#define FEXCORE_PROFILE_INSTANT(...) \
  do {                               \
  } while (0)
#define FEXCORE_PROFILE_SCOPED(...) \
  do {                              \
  } while (0)
#endif
} // namespace FEXCore::Profiler
