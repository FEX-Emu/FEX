// SPDX-License-Identifier: MIT
#include "XXFileHash.h"

#include <chrono>
#include <fcntl.h>
#include <fmt/format.h>
#include <unistd.h>
#include <xxhash.h>
#include <functional>

namespace XXFileHash {
class Reader {
public:
  Reader(int fd, size_t Size)
    : fd {fd}
    , Size {Size} {}

  virtual ~Reader() = default;

  bool Initialized() const {
    return IsInitialized;
  }

  using Callback = std::function<bool(const void* Data, size_t Size)>;
  virtual bool Read(Callback cb) = 0;

protected:
  int fd {};
  size_t Size {};
  bool IsInitialized {};
};

class MemoryReader final : public Reader {
public:
  MemoryReader(int fd, size_t Size)
    : Reader(fd, Size) {
    Ptr = reinterpret_cast<std::byte*>(mmap(nullptr, Size, PROT_READ, MAP_SHARED, fd, 0));
    IsInitialized = Ptr != MAP_FAILED;
  }

  ~MemoryReader() {
    munmap(reinterpret_cast<void*>(Ptr), Size);
  }

  bool Read(Callback cb) override {
    auto ReadPtr = Ptr;
    const auto ReadEndPtr = Ptr + Size;
    size_t ReadSize {};

    // Claim sequential access.
    ::madvise(reinterpret_cast<void*>(ReadPtr), Size, MADV_SEQUENTIAL);

    while (ReadPtr < ReadEndPtr) {
      ReadSize = std::min<size_t>(READ_BLOCK_SIZE, ReadEndPtr - ReadPtr);

      if (!cb(ReadPtr, ReadSize)) {
        return false;
      }

      // Only allow a single block read to be resident.
      ::madvise(reinterpret_cast<void*>(ReadPtr), ReadSize, MADV_DONTNEED);

      ReadPtr += ReadSize;
    }

    return true;
  }

private:
  std::byte* Ptr {};

  // Only allow 128MB in flight.
  constexpr static size_t READ_BLOCK_SIZE = 128 * 1024 * 1024;
};

std::optional<uint64_t> HashFile(const fextl::string& Filepath) {
  int fd = open(Filepath.c_str(), O_RDONLY);
  if (fd == -1) {
    return std::nullopt;
  }

  XXH3_state_t* State {};
  auto HadError = [fd, &State]() {
    close(fd);
    if (State) {
      XXH3_freeState(State);
    }
    return std::nullopt;
  };
  // Get file size
  off_t Size = lseek(fd, 0, SEEK_END);
  if (Size == -1) {
    return HadError();
  }

  // Reset to beginning
  if (lseek(fd, 0, SEEK_SET) == -1) {
    return HadError();
  }

  // Set up XXHash state
  State = XXH3_createState();
  const XXH64_hash_t Seed = 0;

  if (!State) {
    return HadError();
  }

  if (XXH3_64bits_reset_withSeed(State, Seed) == XXH_ERROR) {
    return HadError();
  }

  MemoryReader Read(fd, Size);

  if (!Read.Initialized()) {
    return HadError();
  }

  const auto Start = std::chrono::high_resolution_clock::now();
  auto Now = Start;
  const double SizeD = Size;
  size_t CurrentOffset {};

  auto CB_XXH = [&](const void* Data, size_t BlockSize) -> bool {
    if (XXH3_64bits_update(State, Data, BlockSize) == XXH_ERROR) {
      return false;
    }

    auto Cur = std::chrono::high_resolution_clock::now();
    auto Dur = Cur - Now;
    if (Dur >= std::chrono::seconds(1)) {
      fmt::print("{:.2}% hashed\n", (double)CurrentOffset / SizeD * 100.0);
      Now = Cur;
    }

    CurrentOffset += BlockSize;

    return true;
  };

  if (!Read.Read(CB_XXH)) {
    return HadError();
  }

  const auto Hash = XXH3_64bits_digest(State);
  XXH3_freeState(State);

  close(fd);
  return Hash;
}
} // namespace XXFileHash
