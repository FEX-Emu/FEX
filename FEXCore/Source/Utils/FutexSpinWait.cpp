#include "Utils/FutexSpinWait.h"

namespace FEXCore::Utils::FutexSpinWait {
#ifdef _M_ARM_64
  constexpr uint64_t NanosecondsInSecond = 1'000'000'000ULL;

  static uint32_t GetCycleCounterFrequency() {
    uint64_t Result{};
    __asm("mrs %[Res], CNTFRQ_EL0"
        : [Res] "=r" (Result));
    return Result;
  }

  static uint64_t CalculateCyclesPerNanosecond() {
    // Snapdragon devices historically use a 19.2Mhz cycle counter frequency
    // This means that the number of cycles per nanosecond ends up being 52.0833...
    //
    // ARMv8.6 and ARMv9.1 requires the cycle counter frequency to be 1Ghz.
    // This means the number of cycles per nanosecond ends up being 1.
    uint64_t CounterFrequency = GetCycleCounterFrequency();
    return NanosecondsInSecond / CounterFrequency;
  }

  uint32_t CycleCounterFrequency = GetCycleCounterFrequency();
  uint64_t CyclesPerNanosecond = CalculateCyclesPerNanosecond();
#endif
}
