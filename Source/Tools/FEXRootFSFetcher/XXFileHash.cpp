// SPDX-License-Identifier: MIT
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
std::pair<bool, uint64_t> HashFile(const fextl::string& Filepath) {
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
  XXH3_state_t* const State = XXH3_createState();
  const XXH64_hash_t Seed = 0;

  if (!State) {
    return HadError();
  }

  if (XXH3_64bits_reset_withSeed(State, Seed) == XXH_ERROR) {
    XXH3_freeState(State);
    close(fd);
    return HadError();
  }

  std::vector<char> Data(BLOCK_SIZE);
  off_t CurrentOffset = 0;
  auto Now = std::chrono::high_resolution_clock::now();

  // Let the kernel know that we will be reading linearly
  posix_fadvise(fd, 0, Size, POSIX_FADV_SEQUENTIAL);
  while (CurrentOffset < Size) {

    ssize_t Result = pread(fd, Data.data(), BLOCK_SIZE, CurrentOffset);
    if (Result == -1) {
      XXH3_freeState(State);
      close(fd);
      return HadError();
    }

    if (XXH3_64bits_update(State, Data.data(), Result) == XXH_ERROR) {
      XXH3_freeState(State);
      close(fd);
      return HadError();
    }
    auto Cur = std::chrono::high_resolution_clock::now();
    auto Dur = Cur - Now;
    if (Dur >= std::chrono::seconds(1)) {
      fmt::print("{:.2}% hashed\n", (double)CurrentOffset / SizeD * 100.0);
      Now = Cur;
    }
    CurrentOffset += Result;
  }

  const XXH64_hash_t Hash = XXH3_64bits_digest(State);
  XXH3_freeState(State);

  close(fd);
  return {true, Hash};
}
} // namespace XXFileHash
