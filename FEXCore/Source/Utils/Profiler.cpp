// SPDX-License-Identifier: MIT
#include <cstdint>
#include <fcntl.h>
#ifndef _WIN32
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#endif

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#ifdef ENABLE_FEXCORE_PROFILER
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
#include <array>
#include <limits.h>
#include <time.h>
#ifndef _WIN32
static inline uint64_t GetTime() {
  // We want the time in the least amount of overhead possible
  // clock_gettime will do a VDSO call with the least amount of overhead
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec;
}
#else

static inline uint64_t GetTime() {
  // GetTime needs to return nanoseconds, query the interface.
  static uint64_t FrequencyScale = {};
  if (!FrequencyScale) [[unlikely]] {
    LARGE_INTEGER Frequency {};
    while (!QueryPerformanceFrequency(&Frequency))
      ;
    constexpr uint64_t NanosecondsInSecond = 1'000'000'000ULL;

    // On WINE this will always result in a scale of 100.
    FrequencyScale = NanosecondsInSecond / Frequency.QuadPart;
  }
  LARGE_INTEGER ticks;
  while (!QueryPerformanceCounter(&ticks))
    ;
  return ticks.QuadPart * FrequencyScale;
}

#endif

namespace FEXCore::Profiler {
ProfilerBlock::ProfilerBlock(std::string_view const Format)
  : DurationBegin {GetTime()}
  , Format {Format} {}

ProfilerBlock::~ProfilerBlock() {
  auto Duration = GetTime() - DurationBegin;
  TraceObject(Format, Duration);
}
} // namespace FEXCore::Profiler

namespace GPUVis {
// ftrace FD for writing trace data.
// Needs to be a raw FD since we hold this open for the entire application execution.
static int TraceFD {-1};

// Need to search the paths to find the real trace path
static std::array<const char*, 2> TraceFSDirectories {
  "/sys/kernel/tracing",
  "/sys/kernel/debug/tracing",
};

void Init() {
  for (auto Path : TraceFSDirectories) {
#ifdef _WIN32
    constexpr auto flags = O_WRONLY;
#else
    constexpr auto flags = O_WRONLY | O_CLOEXEC;
#endif
    fextl::string FilePath = fextl::fmt::format("{}/trace_marker", Path);
    TraceFD = open(FilePath.c_str(), flags);
    if (TraceFD != -1) {
      // Opened TraceFD, early exit
      break;
    }
  }
}

void Shutdown() {
  if (TraceFD != -1) {
    close(TraceFD);
    TraceFD = -1;
  }
}

void TraceObject(std::string_view const Format, uint64_t Duration) {
  if (TraceFD != -1) {
    // Print the duration as something that began negative duration ago
    const auto StringSize = Format.size() + strlen(" (lduration=-)\n") + 22;
    auto Event = reinterpret_cast<char*>(alloca(StringSize));
    auto Res = ::fmt::format_to_n(Event, StringSize, "{} (lduration=-{})\n", Format, Duration);
    write(TraceFD, Event, Res.size);
  }
}

void TraceObject(std::string_view const Format) {
  if (TraceFD != -1) {
    const auto StringSize = Format.size() + 1;
    auto Event = reinterpret_cast<char*>(alloca(StringSize));
    auto Res = ::fmt::format_to_n(Event, StringSize, "{}\n", Format);
    write(TraceFD, Event, Res.size);
  }
}
} // namespace GPUVis
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
#include "tracy/Tracy.hpp"
namespace Tracy {
static int EnableAfterFork = 0;
static bool Enable = false;

void Init(std::string_view ProgramName, std::string_view ProgramPath) {
  const char* ProfileTargetName = getenv("FEX_PROFILE_TARGET_NAME"); // Match by application name
  const char* ProfileTargetPath = getenv("FEX_PROFILE_TARGET_PATH"); // Match by path suffix
  const char* WaitForFork = getenv("FEX_PROFILE_WAIT_FOR_FORK");     // Don't enable profiling until the process forks N times
  bool Matched = (ProfileTargetName && ProgramName == ProfileTargetName) || (ProfileTargetPath && ProgramPath.ends_with(ProfileTargetPath));
  if (Matched && WaitForFork) {
    EnableAfterFork = std::atoi(WaitForFork);
  }
  Enable = Matched && !EnableAfterFork;
  if (Enable) {
    tracy::StartupProfiler();
    LogMan::Msg::IFmt("Tracy profiling started");
  } else if (EnableAfterFork) {
    LogMan::Msg::IFmt("Tracy profiling will start after fork");
  }
}

void PostForkAction(bool IsChild) {
  if (Enable) {
    // Tracy does not support multiprocess profiling
    LogMan::Msg::EFmt("Warning: Profiling a process with forks is not supported. Set the environment variable "
                      "FEX_PROFILE_WAIT_FOR_FORK=<n> to start profiling after the n-th fork.");
  }

  if (IsChild) {
    Enable = false;
    return;
  }

  if (EnableAfterFork > 1) {
    --EnableAfterFork;
    LogMan::Msg::IFmt("Tracy profiling will start after {} forks", EnableAfterFork);
  } else if (EnableAfterFork == 1) {
    Enable = true;
    EnableAfterFork = 0;
    tracy::StartupProfiler();
    LogMan::Msg::IFmt("Tracy profiling started");
  }
}

void Shutdown() {
  if (Tracy::Enable) {
    LogMan::Msg::IFmt("Stopping Tracy profiling");
    tracy::ShutdownProfiler();
  }
}

void TraceObject(std::string_view const Format, uint64_t Duration) {}

void TraceObject(std::string_view const Format) {
  if (Tracy::Enable) {
    TracyMessage(Format.data(), Format.size());
  }
}
} // namespace Tracy
#else
#error Unknown profiler backend
#endif
#endif

namespace FEXCore::Profiler {

#ifdef ENABLE_FEXCORE_PROFILER
void Init(std::string_view ProgramName, std::string_view ProgramPath) {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
  GPUVis::Init();
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  Tracy::Init(ProgramName, ProgramPath);
#endif
}

void PostForkAction(bool IsChild) {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  Tracy::PostForkAction(IsChild);
#endif
}

bool IsActive() {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
  // Always active
  return true;
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  // Active if previously enabled
  return Tracy::Enable;
#endif
}

void Shutdown() {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
  GPUVis::Shutdown();
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  Tracy::Shutdown();
#endif
}

void TraceObject(std::string_view const Format, uint64_t Duration) {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
  GPUVis::TraceObject(Format, Duration);
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  Tracy::TraceObject(Format, Duration);
#endif
}

void TraceObject(std::string_view const Format) {
#if FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_GPUVIS
  GPUVis::TraceObject(Format);
#elif FEXCORE_PROFILER_BACKEND == FEXCORE_PROFILER_BACKEND_TRACY
  Tracy::TraceObject(Format);
#endif
}

#endif
} // namespace FEXCore::Profiler
