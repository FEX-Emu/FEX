#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <sys/resource.h>

namespace FDCountWatch {
  // FD count watching
  constexpr size_t static MAX_FD_DISTANCE = 32;
  rlimit MaxFDs{};
  std::atomic<ssize_t> NumFilesOpened{};
  std::mutex FDLimitMutex;

  size_t GetNumFilesOpen() {
    // Walk /proc/self/fd/ to see how many open files we currently have
    const std::filesystem::path self{"/proc/self/fd/"};

    return std::distance(std::filesystem::directory_iterator{self}, std::filesystem::directory_iterator{});
  }

  void GetMaxFDs() {
    std::unique_lock lk{FDLimitMutex};

    // Get our kernel limit for the number of open files
    if (getrlimit(RLIMIT_NOFILE, &MaxFDs) != 0) {
      fprintf(stderr, "[FEXMountDaemon] getrlimit(RLIMIT_NOFILE) returned error %d %s\n", errno, strerror(errno));
    }

    // Walk /proc/self/fd/ to see how many open files we currently have
    NumFilesOpened = GetNumFilesOpen();
  }

  void CheckRaiseFDLimit() {
    if (NumFilesOpened < (MaxFDs.rlim_cur - MAX_FD_DISTANCE)) {
      // No need to raise the limit.
      return;
    }

    if (FDLimitMutex.try_lock()) {
      if (MaxFDs.rlim_cur == MaxFDs.rlim_max) {
        fprintf(stderr, "[FEXMountDaemon] Our open FD limit is already set to max and we are wanting to increase it\n");
        fprintf(stderr, "[FEXMountDaemon] FEXMountDaemon will now no longer be able to track new instances of FEX\n");
        fprintf(stderr, "[FEXMountDaemon] Current limit is %zd(hard %zd) FDs and we are at %zd\n", MaxFDs.rlim_cur, MaxFDs.rlim_max, GetNumFilesOpen());
        fprintf(stderr, "[FEXMountDaemon] Ask your administrator to raise your kernel's hard limit on open FDs\n");
        return;
      }

      rlimit NewLimit = MaxFDs;

      // Just multiply by two
      NewLimit.rlim_cur <<= 1;

      // Now limit to the hard max
      NewLimit.rlim_cur = std::min(NewLimit.rlim_cur, NewLimit.rlim_max);

      if (setrlimit(RLIMIT_NOFILE, &NewLimit) != 0) {
        fprintf(stderr, "[FEXMountDaemon] Couldn't raise FD limit to %zd even though our hard limit is %zd\n", NewLimit.rlim_cur, NewLimit.rlim_max);
      }
      else {
        // Set the new limit
        MaxFDs = NewLimit;
      }

      FDLimitMutex.unlock();
    }
  }

  void IncrementFDCountAndCheckLimits(ssize_t Num) {
    // Increment number of FDs.
    NumFilesOpened += Num;

    CheckRaiseFDLimit();
  }
}
