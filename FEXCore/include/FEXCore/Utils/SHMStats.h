// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

#ifdef _M_X86_64
#include <x86intrin.h>
#endif

namespace FEXCore::SHMStats {
#ifdef _M_ARM_64
/**
 * @brief Get the raw cycle counter with synchronizing isb.
 *
 * `CNTVCTSS_EL0` also does the same thing, but requires the FEAT_ECV feature.
 */
static inline uint64_t GetCycleCounter() {
  uint64_t Result {};
  __asm volatile(R"(
      isb;
      mrs %[Res], CNTVCT_EL0;
    )"
                 : [Res] "=r"(Result));
  return Result;
}
#else
static inline uint64_t GetCycleCounter() {
  unsigned dummy;
  uint64_t tsc = __rdtscp(&dummy);
  return tsc;
}
#endif
// FEXCore live-stats
constexpr uint8_t STATS_VERSION = 2;
enum class AppType : uint8_t {
  LINUX_32,
  LINUX_64,
  WIN_ARM64EC,
  WIN_WOW64,
};

// Only append new members to the end of {ThreadStatsHeader, ThreadStats} to allow old tools time to support new information.
// FEX isn't guaranteeing /not/ breaking compatibility with versions, but trying to not cause too much churn.
struct ThreadStatsHeader {
  uint8_t Version;
  AppType app_type;
  uint16_t ThreadStatsSize;
  char fex_version[48];
  std::atomic<uint32_t> Head;
  std::atomic<uint32_t> Size;
  uint32_t Pad;
};

struct ThreadStats {
  std::atomic<uint32_t> Next;
  std::atomic<uint32_t> TID;

  // Accumulated time (In unscaled CPU cycles!)
  uint64_t AccumulatedJITTime;
  uint64_t AccumulatedSignalTime;

  // Accumulated event counts
  uint64_t AccumulatedSIGBUSCount;
  uint64_t AccumulatedSMCCount;
  uint64_t AccumulatedFloatFallbackCount;

  uint64_t AccumulatedCacheMissCount;
  uint64_t AccumulatedCacheReadLockTime;
  uint64_t AccumulatedCacheWriteLockTime;

  uint64_t AccumulatedJITCount;
};

// Ensure 16-byte alignment to take advantage of ARM single-copy atomicity.
static_assert(sizeof(ThreadStats) % 16 == 0, "Needs to be 16-byte aligned!");

template<typename T, size_t FlatOffset = 0>
class AccumulationBlock final {
public:
  AccumulationBlock(T* Stat)
    : Begin {Stat ? GetCycleCounter() : 0}
    , Stat {Stat} {}

  ~AccumulationBlock() {
    if (Stat) {
      const auto Duration = GetCycleCounter() - Begin + FlatOffset;
      auto ref = std::atomic_ref<T>(*Stat);
      ref.fetch_add(Duration, std::memory_order_relaxed);
    }
  }

private:
  uint64_t Begin;
  T* Stat;
};
#define UniqueScopeName2(name, line) name##line
#define UniqueScopeName(name, line) UniqueScopeName2(name, line)

#define FEXCORE_PROFILE_ACCUMULATION(ThreadState, Stat)                                                                          \
  FEXCore::SHMStats::AccumulationBlock<decltype(ThreadState->ThreadStats->Stat)> UniqueScopeName(ScopedAccumulation_, __LINE__)( \
    ThreadState->ThreadStats ? &ThreadState->ThreadStats->Stat : nullptr);
#define FEXCORE_PROFILE_INSTANT_INCREMENT(ThreadState, Stat, value) \
  do {                                                              \
    if (ThreadState->ThreadStats) {                                 \
      ThreadState->ThreadStats->Stat += value;                      \
    }                                                               \
  } while (0)

} // namespace FEXCore::SHMStats
