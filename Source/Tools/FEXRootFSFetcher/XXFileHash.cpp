#include "XXFileHash.h"

#include <chrono>
#include <fcntl.h>
#include <fmt/format.h>
#include <unistd.h>
#include <vector>
#include <xxhash.h>

namespace XXFileHash {
  // 32MB blocks
  constexpr static size_t BLOCK_SIZE = 32 * 1024 * 1024;
  std::pair<bool, uint64_t> HashFile(const std::string &Filepath) {
    int fd = open(Filepath.c_str(), O_RDONLY);
    if (fd == -1) {
      return {false, 0};
    }

    auto HadError = [fd]() -> std::pair<bool, uint64_t> {
      close(fd);
      return {false, 0};
    };
    // Get file size
    off_t Size = lseek(fd, 0, SEEK_END);
    double SizeD = Size;

    // Reset to beginning
    lseek(fd, 0, SEEK_SET);

    // Set up XXHash state
    XXH64_state_t* const State = XXH64_createState();
    XXH64_hash_t const Seed = 0;

    if (!State) {
      return HadError();
    }

    if (XXH64_reset(State, Seed) == XXH_ERROR) {
      return HadError();
    }

    std::vector<char> Data(BLOCK_SIZE);
    off_t DataRemaining = Size - BLOCK_SIZE;
    off_t DataTail = Size - DataRemaining;
    off_t CurrentOffset = 0;
    auto Now = std::chrono::high_resolution_clock::now();

    // Let the kernel know that we will be reading linearly
    posix_fadvise(fd, 0, Size, POSIX_FADV_SEQUENTIAL);
    while (CurrentOffset < DataRemaining) {
      ssize_t Result = pread(fd, Data.data(), BLOCK_SIZE, CurrentOffset);
      if (Result == -1) {
        return HadError();
      }

      if (XXH64_update(State, Data.data(), BLOCK_SIZE) == XXH_ERROR) {
        return HadError();
      }
      auto Cur = std::chrono::high_resolution_clock::now();
      auto Dur = Cur - Now;
      if (Dur >= std::chrono::seconds(5)) {
        fmt::print("{}% hashed\n", (double)CurrentOffset / SizeD);
        Now = Cur;
      }
      CurrentOffset += BLOCK_SIZE;
    }

    // Finish the tail
    ssize_t Result = pread(fd, Data.data(), DataTail, CurrentOffset);
    if (Result == -1) {
      return HadError();
    }

    if (XXH64_update(State, Data.data(), DataTail) == XXH_ERROR) {
      return HadError();
    }

    XXH64_hash_t const Hash = XXH64_digest(State);
    XXH64_freeState(State);

    close(fd);
    return {true, Hash};
  }
}
