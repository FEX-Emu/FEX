// SPDX-License-Identifier: MIT
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <limits.h>
#ifndef _WIN32
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#endif

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#define BACKEND_OFF 0
#define BACKEND_GPUVIS 1

#ifdef ENABLE_FEXCORE_PROFILER
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
namespace FEXCore::Profiler {
ProfilerBlock::ProfilerBlock(const std::string_view Format)
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

static bool IsTraceFS(const char* Path) {
  struct statfs stat;
  if (statfs(Path, &stat)) {
    return false;
  }
  return stat.f_type == TRACEFS_MAGIC;
}

void Init() {
  for (auto Path : TraceFSDirectories) {
    if (IsTraceFS(Path)) {
      fextl::string FilePath = fextl::fmt::format("{}/trace_marker", Path);
      TraceFD = open(FilePath.c_str(), O_WRONLY | O_CLOEXEC);
      if (TraceFD != -1) {
        // Opened TraceFD, early exit
        break;
      }
    }
  }
}

void Shutdown() {
  if (TraceFD != -1) {
    close(TraceFD);
    TraceFD = -1;
  }
}

void TraceObject(const std::string_view Format, uint64_t Duration) {
  if (TraceFD != -1) {
    // Print the duration as something that began negative duration ago
    fextl::string Event = fextl::fmt::format("{} (lduration=-{})\n", Format, Duration);
    write(TraceFD, Event.c_str(), Event.size());
  }
}

void TraceObject(const std::string_view Format) {
  if (TraceFD != -1) {
    fextl::string Event = fextl::fmt::format("{}\n", Format);
    write(TraceFD, Format.data(), Format.size());
  }
}
} // namespace GPUVis
#else
#error Unknown profiler backend
#endif
#endif

namespace FEXCore::Profiler {
#ifdef ENABLE_FEXCORE_PROFILER
void Init() {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::Init();
#endif
}

void Shutdown() {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::Shutdown();
#endif
}

void TraceObject(const std::string_view Format, uint64_t Duration) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::TraceObject(Format, Duration);
#endif
}

void TraceObject(const std::string_view Format) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
  GPUVis::TraceObject(Format);
#endif
}
#endif
} // namespace FEXCore::Profiler
