// Threads that reach the same untranslated code together must share one translation.
//
// Worker pools run the same code at the same time, so they miss on the same entry at the same time. When each of them translated it,
// the copies filled the code buffer, and the clear that followed made them all miss together again. This is invisible to the guest
// other than in speed, so the test reads FEX's own compile count instead of timing anything.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Many translations of the same size, each long enough that the other threads arrive while the first one compiles.
// Each stays under the FEX_MAXINST=500 that CMake sets for these tests, so it is one translation.
#define FUNCTIONS 64
extern "C" {
extern const uint8_t concurrent_compile_functions[];
extern const uint8_t concurrent_compile_functions_end[];
}
__asm__(R"(
.text
.balign 4096
concurrent_compile_functions:
.rept 64
  .rept 130
  add rax, rbx
  xor rcx, rax
  imul rdx, rcx
  .endr
  ret
.endr
concurrent_compile_functions_end:
)");

// From FEXCore/Utils/SHMStats.h, which only ever appends to these. A guest can't include it, it would need FEX's arch defines.
struct StatsHeader {
  uint8_t Version;
  uint8_t AppType;
  uint16_t ThreadStatsSize;
  char FEXVersion[48];
  uint32_t Head;
};

constexpr size_t JITCountOffset = 8 + 8 * 8;

static uint64_t CompileCount(const uint8_t* Stats) {
  auto Header = reinterpret_cast<const StatsHeader*>(Stats);
  uint64_t Count {};
  for (uint32_t Offset = std::atomic_ref(const_cast<uint32_t&>(Header->Head)).load(); Offset;) {
    auto Thread = Stats + Offset;
    Count += std::atomic_ref(*const_cast<uint64_t*>(reinterpret_cast<const uint64_t*>(Thread + JITCountOffset))).load();
    Offset = std::atomic_ref(*const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(Thread))).load();
  }
  return Count;
}

TEST_CASE("Threads share one translation") {
  // CMake sets this for this test. Without it this isn't running under FEX's test harness.
  if (!getenv("FEX_PROFILESTATS")) {
    SUCCEED("Needs FEX's compile counts");
    return;
  }

  char Path[64];
  snprintf(Path, sizeof(Path), "/dev/shm/fex-%d-stats", getpid());
  int FD = open(Path, O_RDONLY);
  REQUIRE(FD != -1);

  const int Threads = std::min(8U, std::thread::hardware_concurrency());
  if (Threads < 2) {
    SUCCEED("Needs two threads running at once");
    return;
  }

  std::atomic<int> Next {-1};
  std::atomic<int> Arrived {0};
  std::atomic<bool> Counted {false};
  const size_t Size = (concurrent_compile_functions_end - concurrent_compile_functions) / FUNCTIONS;

  auto Worker = [&]() {
    for (int Function = 0; Function < FUNCTIONS; ++Function) {
      Arrived.fetch_add(1);
      while (Next.load() < Function) {
        __builtin_ia32_pause();
      }
      // Only touches caller saved registers.
      reinterpret_cast<void (*)()>(reinterpret_cast<uintptr_t>(concurrent_compile_functions) + Function * Size)();
    }
    Arrived.fetch_add(1);
    // FEX drops a thread's stats when it exits.
    while (!Counted.load()) {
      __builtin_ia32_pause();
    }
  };

  std::vector<std::thread> Pool;
  for (int i = 0; i < Threads; ++i) {
    Pool.emplace_back(Worker);
  }

  auto AllArrived = [&](int Function) {
    while (Arrived.load() < Threads * (Function + 1)) {
      __builtin_ia32_pause();
    }
  };
  AllArrived(0);

  // Stats grow as threads start, so map them once every worker exists.
  struct stat Info {};
  REQUIRE(fstat(FD, &Info) == 0);
  auto Stats = static_cast<const uint8_t*>(mmap(nullptr, Info.st_size, PROT_READ, MAP_SHARED, FD, 0));
  REQUIRE(Stats != MAP_FAILED);

  // The first function translates the worker loop itself, so it doesn't count.
  Next.store(0);
  AllArrived(1);
  const uint64_t Before = CompileCount(Stats);

  for (int Function = 1; Function < FUNCTIONS; ++Function) {
    Next.store(Function);
    AllArrived(Function + 1);
  }
  const uint64_t Compiles = CompileCount(Stats) - Before;

  Counted.store(true);
  for (auto& Thread : Pool) {
    Thread.join();
  }

  // One translation each, plus a little for the return paths.
  INFO("Compiles for " << FUNCTIONS - 1 << " functions in " << Threads << " threads");
  CHECK(Compiles < (FUNCTIONS - 1) * 3 / 2);
}
