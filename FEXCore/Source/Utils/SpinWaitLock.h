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
   * On platforms tested, WFE will put the CPU in to a lower power state for upwards of 52ns per WFE. Which isn't a significant amount of time
   * but should still have power savings. Ideally WFE would be able to keep the CPU in a lower power state for longer. This also has the added benefit
   * that atomics aren't abusing the caches when spinning on a cacheline, which has knock-on powersaving benefits.
   *
   * FEAT_WFxT adds a new instruction with a timeout, but since the spurious wake-up is so aggressive it isn't worth using.
   *
   * It should be noted that this implementation has a few dozen cycles of start-up time. Which means the overhead for invoking this implementation is
   * slightly higher than a true spin-loop. The hot loop body itself is only three instructions so it is quite efficient.
   *
   * On non-ARM platforms it is truly a spin-loop, which is okay for debugging only.
   */
#ifdef _M_ARM_64

#define SPINLOOP_BODY(LoadExclusiveOp, LoadAtomicOp, RegSize) \
  /* Prime the exclusive monitor with the passed in address. */ \
  #LoadExclusiveOp " %" #RegSize "[Tmp], [%[Futex]]; \
  /* WFE will wait for either the memory to change or spurious wake-up. */ \
  wfe; \
  /* Load with acquire to get the result of memory. */ \
  " #LoadAtomicOp " %" #RegSize "[Result], [%[Futex]]; "

#define SPINLOOP_8BIT  SPINLOOP_BODY(ldaxrb, ldarb, w)
#define SPINLOOP_16BIT SPINLOOP_BODY(ldaxrh, ldarh, w)
#define SPINLOOP_32BIT SPINLOOP_BODY(ldaxr,  ldar,  w)
#define SPINLOOP_64BIT SPINLOOP_BODY(ldaxr,  ldar,  x)

  extern uint32_t CycleCounterFrequency;
  extern uint64_t CyclesPerNanosecond;

  ///< Get the raw cycle counter which is synchronizing.
  /// `CNTVCTSS_EL0` also does the same thing, but requires the FEAT_ECV feature.
  static inline uint64_t GetCycleCounter() {
    uint64_t Result{};
    __asm volatile(R"(
      isb;
      mrs %[Res], CNTVCT_EL0;
    )"
    : [Res] "=r" (Result));
    return Result;
  }

  ///< Converts nanoseconds to number of cycles.
  /// If the cycle counter is 1Ghz then this is a direct 1:1 map.
  static inline uint64_t ConvertNanosecondsToCycles(std::chrono::nanoseconds const &Nanoseconds) {
    const auto NanosecondCount = Nanoseconds.count();
    return NanosecondCount / CyclesPerNanosecond;
  }

  template<typename T, typename TT = T>
  static inline void Wait(T *Futex, TT ExpectedValue) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
    T Tmp{};
    T Result = AtomicFutex->load();

    // Early exit if possible.
    if (Result == ExpectedValue) return;

    do {
      if constexpr (sizeof(T) == 1) {
        __asm volatile(SPINLOOP_8BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 2) {
        __asm volatile(SPINLOOP_16BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 4) {
        __asm volatile(SPINLOOP_32BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 8) {
        __asm volatile(SPINLOOP_64BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else {
        static_assert(!std::is_same_v<T, T>, "Invalid");
      }
    } while (Result != ExpectedValue);
  }

  template
  void Wait<uint8_t>(uint8_t*, uint8_t);
  template
  void Wait<uint16_t>(uint16_t*, uint16_t);
  template
  void Wait<uint32_t>(uint32_t*, uint32_t);
  template
  void Wait<uint64_t>(uint64_t*, uint64_t);

  template<typename T, typename TT>
  static inline bool Wait(T *Futex, TT ExpectedValue, std::chrono::nanoseconds const &Timeout) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);

    T Tmp{};
    T Result = AtomicFutex->load();

    // Early exit if possible.
    if (Result == ExpectedValue) return true;

    const auto TimeoutCycles = ConvertNanosecondsToCycles(Timeout);
    const auto Begin = GetCycleCounter();

    do {
      if constexpr (sizeof(T) == 1) {
        __asm volatile(SPINLOOP_8BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 2) {
        __asm volatile(SPINLOOP_16BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 4) {
        __asm volatile(SPINLOOP_32BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else if constexpr (sizeof(T) == 8) {
        __asm volatile(SPINLOOP_64BIT
          : [Result] "=r" (Result)
          , [Tmp] "=r" (Tmp)
          : [Futex] "r" (Futex)
          , [ExpectedValue] "r" (ExpectedValue)
          : "memory");
      }
      else {
        static_assert(!std::is_same_v<T, T>, "Invalid");
      }

      const auto CurrentCycleCounter = GetCycleCounter();
      if ((CurrentCycleCounter - Begin) >= TimeoutCycles) {
        // Couldn't get value before timeout.
        return false;
      }
    } while (Result != ExpectedValue);

    // We got our result.
    return true;
  }

  template
  bool Wait<uint8_t>(uint8_t*, uint8_t, std::chrono::nanoseconds const &);
  template
  bool Wait<uint16_t>(uint16_t*, uint16_t, std::chrono::nanoseconds const &);
  template
  bool Wait<uint32_t>(uint32_t*, uint32_t, std::chrono::nanoseconds const &);
  template
  bool Wait<uint64_t>(uint64_t*, uint64_t, std::chrono::nanoseconds const &);

#else
  template<typename T, typename TT>
  static inline void Wait(T *Futex, TT ExpectedValue) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
    T Tmp{};
    T Result = AtomicFutex->load();

    // Early exit if possible.
    if (Result == ExpectedValue) return;

    do {
      Result = AtomicFutex->load();
    } while (Result != ExpectedValue);
  }

  template<typename T, typename TT>
  static inline bool Wait(T *Futex, TT ExpectedValue, std::chrono::nanoseconds const &Timeout) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);

    T Tmp{};
    T Result = AtomicFutex->load();

    // Early exit if possible.
    if (Result == ExpectedValue) return true;

    const auto Begin = std::chrono::high_resolution_clock::now();

    do {
      Result = AtomicFutex->load();

      const auto CurrentCycleCounter =  std::chrono::high_resolution_clock::now();
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
  static inline void lock(T *Futex) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
    T Expected{};
    T Desired {1};

    // Try to CAS immediately.
    if (AtomicFutex->compare_exchange_strong(Expected, Desired)) return;

    do {
      // Wait until the futex is unlocked.
      Wait(Futex, 0);
    } while (!AtomicFutex->compare_exchange_strong(Expected, Desired));
  }

  template<typename T>
  static inline bool try_lock(T *Futex) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
    T Expected{};
    T Desired {1};

    // Try to CAS immediately.
    if (AtomicFutex->compare_exchange_strong(Expected, Desired)) return true;

    return false;
  }

  template<typename T>
  static inline void unlock(T *Futex) {
    std::atomic<T> *AtomicFutex = reinterpret_cast<std::atomic<T>*>(Futex);
    AtomicFutex->store(0);
  }

#undef SPINLOOP_8BIT
#undef SPINLOOP_16BIT
#undef SPINLOOP_32BIT
#undef SPINLOOP_64BIT
  template<typename T>
  class UniqueSpinMutex final {
    public:
      UniqueSpinMutex(T *Futex)
        : Futex {Futex} {
          FEXCore::Utils::SpinWaitLock::lock(Futex);
        }

      ~UniqueSpinMutex() {
        FEXCore::Utils::SpinWaitLock::unlock(Futex);
      }
    private:
      T *Futex;
  };
}
