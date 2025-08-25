// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string_view>

#include <FEXCore/Utils/CompilerDefs.h>

#define FEXCORE_PROFILER_BACKEND_OFF 0
#define FEXCORE_PROFILER_BACKEND_GPUVIS 1
#define FEXCORE_PROFILER_BACKEND_TRACY 2

#if defined(ENABLE_FEXCORE_PROFILER) && FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
#include "tracy/Tracy.hpp"
#endif

namespace FEXCore::Profiler {
#define UniqueScopeName2(name, line) name##line
#define UniqueScopeName(name, line) UniqueScopeName2(name, line)

#ifdef ENABLE_FEXCORE_PROFILER

FEX_DEFAULT_VISIBILITY void Init(std::string_view ProgramName, std::string_view ProgramPath);
FEX_DEFAULT_VISIBILITY void PostForkAction(bool IsChild);
FEX_DEFAULT_VISIBILITY bool IsActive();
FEX_DEFAULT_VISIBILITY void Shutdown();
FEX_DEFAULT_VISIBILITY void TraceObject(const std::string_view Format);
FEX_DEFAULT_VISIBILITY void TraceObject(const std::string_view Format, uint64_t Duration);

// Declare an instantaneous profiler event.
#define FEXCORE_PROFILE_INSTANT(name) FEXCore::Profiler::TraceObject(name)

#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
// Declare a scoped profile block variable with a fixed name.
#define FEXCORE_PROFILE_SCOPED(name) ZoneNamedN(___tracy_scoped_zone, name, ::FEXCore::Profiler::IsActive())
#else
// A class that follows scoping rules to generate a profile duration block
class ProfilerBlock final {
public:
  ProfilerBlock(const std::string_view Format);

  ~ProfilerBlock();

private:
  uint64_t DurationBegin;
  const std::string_view Format;
};

// Declare a scoped profile block variable with a fixed name.
#define FEXCORE_PROFILE_SCOPED(name) FEXCore::Profiler::ProfilerBlock UniqueScopeName(ScopedBlock_, __LINE__)(name)
#endif

#else
inline void Init(std::string_view ProgramName, std::string_view ProgramPath) {}
inline void PostForkAction(bool IsChild) {}
inline void Shutdown() {}
inline void TraceObject(const std::string_view Format) {}
inline void TraceObject(const std::string_view, uint64_t) {}

#define FEXCORE_PROFILE_INSTANT(...) \
  do {                               \
  } while (0)
#define FEXCORE_PROFILE_SCOPED(...) \
  do {                              \
  } while (0)

#endif
} // namespace FEXCore::Profiler
