#include <array>
#include <cstdint>
#include <fcntl.h>
#include <limits.h>
#include <linux/magic.h>
#include <string>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>

#define BACKEND_OFF 0
#define BACKEND_GPUVIS 1

#ifdef ENABLE_FEXCORE_PROFILER
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
namespace FEXCore::Profiler {
  ProfilerBlock::ProfilerBlock(std::string_view const Format)
    : DurationBegin {GetTime()}
    , Format {Format} {
    }

  ProfilerBlock::~ProfilerBlock() {
    auto Duration = GetTime() - DurationBegin;
    TraceObject(Format, Duration);
  }
}

namespace GPUVis {
  // ftrace FD for writing trace data.
  // Needs to be a raw FD since we hold this open for the entire application execution.
  static int TraceFD {-1};

  // Need to search the paths to find the real trace path
  static std::array<char const*, 2> TraceFSDirectories {
    "/sys/kernel/tracing",
    "/sys/kernel/debug/tracing",
  };

  static bool IsTraceFS(char const* Path) {
    struct statfs stat;
    if (statfs(Path, &stat)) {
      return false;
    }
    return stat.f_type == TRACEFS_MAGIC;
  }

  void Init() {
    for (auto Path : TraceFSDirectories) {
      if (IsTraceFS(Path)) {
        std::string FilePath = fmt::format("{}/trace_marker", Path);
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

  void TraceObject(std::string_view const Format, uint64_t Duration) {
    if (TraceFD != -1) {
      // Print the duration as something that began negative duration ago
      std::string Event = fmt::format("{} (lduration=-{})\n", Format, Duration);
      write(TraceFD, Event.c_str(), Event.size());
    }
  }

  void TraceObject(std::string_view const Format) {
    if (TraceFD != -1) {
      std::string Event = fmt::format("{}\n", Format);
      write(TraceFD, Format.data(), Format.size());
    }
  }
}
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

  void TraceObject(std::string_view const Format, uint64_t Duration) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
    GPUVis::TraceObject(Format, Duration);
#endif
  }

  void TraceObject(std::string_view const Format) {
#if FEXCORE_PROFILER_BACKEND == BACKEND_GPUVIS
    GPUVis::TraceObject(Format);
#endif

  }
#endif
}
