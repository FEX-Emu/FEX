// SPDX-License-Identifier: MIT
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <limits.h>
#ifndef _WIN32
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <time.h>
#endif

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#define BACKEND_OFF 0
#define BACKEND_GPUVIS 1
#define BACKEND_TRACY 2

#ifdef ENABLE_FEXCORE_PROFILER
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

#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
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
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
#include "tracy/TracyC.h"
namespace Tracy {
void Init() {}

void Shutdown() {}

void TraceObject(std::string_view const Format, uint64_t Duration) {}

void TraceObject(std::string_view const Format) {}
} // namespace Tracy
#else
#error Unknown profiler backend
#endif
#endif

namespace FEXCore::Profiler {

#ifdef ENABLE_FEXCORE_PROFILER
static int g_EnableAfterFork = 0;
static bool g_Enable = false;

void Init(const fextl::string& ProgramName, const fextl::string& ProgramPath) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::Init();
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  const char* ProfileTargetName = getenv("FEX_PROFILE_TARGET_NAME"); // Match by application name
  const char* ProfileTargetPath = getenv("FEX_PROFILE_TARGET_PATH"); // Match by path suffix
  const char* WaitForFork = getenv("FEX_PROFILE_WAIT_FOR_FORK");     // Don't enable profiling until the process forks N times
  bool Enable = (ProfileTargetName && ProgramName == ProfileTargetName) || (ProfileTargetPath && ProgramPath.ends_with(ProfileTargetPath));
  if (Enable && WaitForFork) {
    g_EnableAfterFork = std::atoi(WaitForFork);
  }
  g_Enable = Enable && !g_EnableAfterFork;
  if (g_Enable) {
    Tracy::Init();
    tracy::StartupProfiler();
    LogMan::Msg::IFmt("Tracy profiling started");
  } else if (g_EnableAfterFork) {
    LogMan::Msg::IFmt("Tracy profiling will start after {} forks", g_EnableAfterFork);
  }
#endif
}

void PostForkAction(bool IsChild) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  if (g_Enable) {
    // Tracy does not support multiprocess profiling
    LogMan::Msg::EFmt("Warning: Profiling a process with forks is not supported. Set the environment variable "
                      "FEX_PROFILE_WAIT_FOR_FORK=<n> to start profiling after the n-th fork.");
  }

  if (IsChild) {
    g_Enable = false;
    return;
  }

  if (g_EnableAfterFork > 1) {
    --g_EnableAfterFork;
    LogMan::Msg::IFmt("Tracy profiling will start after {} forks", g_EnableAfterFork);
  } else if (g_EnableAfterFork == 1) {
    g_Enable = true;
    g_EnableAfterFork = 0;
    Tracy::Init();
    tracy::StartupProfiler();
    LogMan::Msg::IFmt("Tracy profiling started");
  }
#endif
}

bool IsActive() {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  // Always active
  return true;
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  // Active if previously enabled
  return g_Enable;
#endif
}

void Shutdown() {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::Shutdown();
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  if (g_Enable) {
    LogMan::Msg::IFmt("Stopping Tracy profiling");
    tracy::ShutdownProfiler();
    Tracy::Shutdown();
  }
#endif
}

void TraceObject(std::string_view const Format, uint64_t Duration) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::TraceObject(Format, Duration);
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  Tracy::TraceObject(Format, Duration);
#endif
}

void TraceObject(std::string_view const Format) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::TraceObject(Format);
#elif FEXCORE_PROFILER_BACKEND == BACKEND_TRACY
  Tracy::TraceObject(Format);
#endif
}

#endif
} // namespace FEXCore::Profiler
