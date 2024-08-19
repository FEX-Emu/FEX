#include <atomic>
#include <chrono>
#include <mutex>
#include <type_traits>

namespace FEXCore::Utils::SpinWaitLock {
/**
 * @brief This provides routines to implement implement an "efficient spin-loop" using ARM's WFE and exclusive monitor interfaces.
 *
 * Spin-loops on mobile devices with a battery can be a bad idea as they burn a bunch of power. This attempts to mitigate some of the impact
 * by putting the CPU in to a lower-power state using WFE.
 * On platforms tested, WFE will put the CPU in to a lower power state for upwards of 0.11ms(!) per WFE. Which isn't a significant amount of
 * time but should still have power savings. Ideally WFE would be able to keep the CPU in a lower power state for longer. This also has the
 * added benefit that atomics aren't abusing the caches when spinning on a cacheline, which has knock-on powersaving benefits.
 *
 * This short timeout is because the Linux kernel has a 100 microsecond architecture timer which wakes up WFE and WFI. Nothing can be
 * improved beyond that period.
 *
 * FEAT_WFxT adds a new instruction with a timeout, but since the spurious wake-up is so aggressive it isn't worth using.
 *
 * It should be noted that this implementation has a few dozen cycles of start-up time. Which means the overhead for invoking this
 * implementation is slightly higher than a true spin-loop. The hot loop body itself is only three instructions so it is quite efficient.
 *
 * On non-ARM platforms it is truly a spin-loop, which is okay for debugging only.
 */
#ifdef _M_ARM_64

#define LOADEXCLUSIVE(LoadExclusiveOp, RegSize)                 \
  /* Prime the exclusive monitor with the passed in address. */ \
  #LoadExclusiveOp " %" #RegSize "[Result], [%[Futex]];\n"

#define SPINLOOP_BODY(LoadAtomicOp, RegSize)                               \
  /* WFE will wait for either the memory to change or spurious wake-up. */ \
  "wfe;\n" /* Load with acquire to get the result of memory. */            \
    #LoadAtomicOp " %" #RegSize "[Result], [%[Futex]];\n"

#define SPINLOOP_WFE_LDX_8BIT LOADEXCLUSIVE(ldaxrb, w)
#define SPINLOOP_WFE_LDX_16BIT LOADEXCLUSIVE(ldaxrh, w)
#define SPINLOOP_WFE_LDX_32BIT LOADEXCLUSIVE(ldaxr, w)
#define SPINLOOP_WFE_LDX_64BIT LOADEXCLUSIVE(ldaxr, x)

#define SPINLOOP_8BIT SPINLOOP_BODY(ldarb, w)
#define SPINLOOP_16BIT SPINLOOP_BODY(ldarh, w)
#define SPINLOOP_32BIT SPINLOOP_BODY(ldar, w)
#define SPINLOOP_64BIT SPINLOOP_BODY(ldar, x)

extern uint32_t CycleCounterFrequency;
extern uint64_t CyclesPerNanosecond;

///< Get the raw cycle counter which is synchronizing.
/// `CNTVCTSS_EL0` also does the same thing, but requires the FEAT_ECV feature.
static inline uint64_t GetCycleCounter() {
  uint64_t Result {};
  __asm volatile(R"(
      isb;
      mrs %[Res], CNTVCT_EL0;
    )"
                 : [Res] "=r"(Result));
  return Result;
}

///< Converts nanoseconds to number of cycles.
/// If the cycle counter is 1Ghz then this is a direct 1:1 map.
static inline uint64_t ConvertNanosecondsToCycles(const std::chrono::nanoseconds& Nanoseconds) {
  const auto NanosecondCount = Nanoseconds.count();
  return NanosecondCount / CyclesPerNanosecond;
}

static inline uint8_t LoadExclusive(uint8_t* Futex) {
  uint8_t Result {};
  __asm volatile(SPINLOOP_WFE_LDX_8BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint16_t LoadExclusive(uint16_t* Futex) {
  uint16_t Result {};
  __asm volatile(SPINLOOP_WFE_LDX_16BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint32_t LoadExclusive(uint32_t* Futex) {
  uint32_t Result {};
  __asm volatile(SPINLOOP_WFE_LDX_32BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint64_t LoadExclusive(uint64_t* Futex) {
  uint64_t Result {};
  __asm volatile(SPINLOOP_WFE_LDX_64BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint8_t WFELoadAtomic(uint8_t* Futex) {
  uint8_t Result {};
  __asm volatile(SPINLOOP_8BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint16_t WFELoadAtomic(uint16_t* Futex) {
  uint16_t Result {};
  __asm volatile(SPINLOOP_16BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint32_t WFELoadAtomic(uint32_t* Futex) {
  uint32_t Result {};
  __asm volatile(SPINLOOP_32BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

static inline uint64_t WFELoadAtomic(uint64_t* Futex) {
  uint64_t Result {};
  __asm volatile(SPINLOOP_64BIT : [Result] "=r"(Result), [Futex] "+r"(Futex)::"memory");

  return Result;
}

template<typename T, typename TT = T>
static inline void Wait(T* Futex, TT ExpectedValue) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
  T Result = AtomicFutex->load();

  // Early exit if possible.
  if (Result == ExpectedValue) {
    return;
  }

  do {
    Result = LoadExclusive(Futex);
    if (Result == ExpectedValue) {
      return;
    }
    Result = WFELoadAtomic(Futex);
  } while (Result != ExpectedValue);
}

template void Wait<uint8_t>(uint8_t*, uint8_t);
template void Wait<uint16_t>(uint16_t*, uint16_t);
template void Wait<uint32_t>(uint32_t*, uint32_t);
template void Wait<uint64_t>(uint64_t*, uint64_t);

template<typename T, typename TT>
static inline bool Wait(T* Futex, TT ExpectedValue, const std::chrono::nanoseconds& Timeout) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);

  T Result = AtomicFutex->load();

  // Early exit if possible.
  if (Result == ExpectedValue) {
    return true;
  }

  const auto TimeoutCycles = ConvertNanosecondsToCycles(Timeout);
  const auto Begin = GetCycleCounter();

  do {
    Result = LoadExclusive(Futex);
    if (Result == ExpectedValue) {
      return true;
    }
    Result = WFELoadAtomic(Futex);

    const auto CurrentCycleCounter = GetCycleCounter();
    if ((CurrentCycleCounter - Begin) >= TimeoutCycles) {
      // Couldn't get value before timeout.
      return false;
    }
  } while (Result != ExpectedValue);

  // We got our result.
  return true;
}

template bool Wait<uint8_t>(uint8_t*, uint8_t, const std::chrono::nanoseconds&);
template bool Wait<uint16_t>(uint16_t*, uint16_t, const std::chrono::nanoseconds&);
template bool Wait<uint32_t>(uint32_t*, uint32_t, const std::chrono::nanoseconds&);
template bool Wait<uint64_t>(uint64_t*, uint64_t, const std::chrono::nanoseconds&);

#else
template<typename T, typename TT>
static inline void Wait(T* Futex, TT ExpectedValue) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
  T Result = AtomicFutex->load();

  // Early exit if possible.
  if (Result == ExpectedValue) {
    return;
  }

  do {
    Result = AtomicFutex->load();
  } while (Result != ExpectedValue);
}

template<typename T, typename TT>
static inline bool Wait(T* Futex, TT ExpectedValue, const std::chrono::nanoseconds& Timeout) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);

  T Result = AtomicFutex->load();

  // Early exit if possible.
  if (Result == ExpectedValue) {
    return true;
  }

  const auto Begin = std::chrono::high_resolution_clock::now();

  do {
    Result = AtomicFutex->load();

    const auto CurrentCycleCounter = std::chrono::high_resolution_clock::now();
    if ((CurrentCycleCounter - Begin) >= Timeout) {
      // Couldn't get value before timeout.
      return false;
    }
  } while (Result != ExpectedValue);

  // We got our result.
  return true;
}
#endif

template<typename T>
static inline void lock(T* Futex) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
  T Expected {};
  T Desired {1};

  // Try to CAS immediately.
  if (AtomicFutex->compare_exchange_strong(Expected, Desired)) {
    return;
  }

  do {
    // Wait until the futex is unlocked.
    Wait(Futex, 0);
    Expected = 0;
  } while (!AtomicFutex->compare_exchange_strong(Expected, Desired));
}

template<typename T>
static inline bool try_lock(T* Futex) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
  T Expected {};
  T Desired {1};

  // Try to CAS immediately.
  if (AtomicFutex->compare_exchange_strong(Expected, Desired)) {
    return true;
  }

  return false;
}

template<typename T>
static inline void unlock(T* Futex) {
  std::atomic<T>* AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
  AtomicFutex->store(0);
}

#undef SPINLOOP_8BIT
#undef SPINLOOP_16BIT
#undef SPINLOOP_32BIT
#undef SPINLOOP_64BIT
template<typename T>
class UniqueSpinMutex final {
public:
  UniqueSpinMutex(T* Futex)
    : Futex {Futex} {
    FEXCore::Utils::SpinWaitLock::lock(Futex);
  }

  ~UniqueSpinMutex() {
    FEXCore::Utils::SpinWaitLock::unlock(Futex);
  }
private:
  T* Futex;
};
} // namespace FEXCore::Utils::SpinWaitLock
