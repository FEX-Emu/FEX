#include "Interface/Core/ArchHelpers/Arm64.h"

#include <FEXCore/Utils/LogManager.h>

#include <atomic>
#include <stdint.h>

#include <signal.h>

namespace FEXCore::ArchHelpers::Arm64 {
static __uint128_t LoadAcquire128(uint64_t Addr) {
  __uint128_t Result{};
  uint64_t Lower;
  uint64_t Upper;
  // This specifically avoids using std::atomic<__uint128_t>
  // std::atomic helper does a ldaxp + stxp pair that crashes when the page is only mapped readable
  __asm volatile(
R"(
  ldaxp %[ResultLower], %[ResultUpper], [%[Addr]];
  clrex;
)"
  : [ResultLower] "=r" (Lower)
  , [ResultUpper] "=r" (Upper)
  : [Addr] "r" (Addr)
  : "memory");
  Result = Upper;
  Result <<= 64;
  Result |= Lower;
  return Result;
}

static uint64_t LoadAcquire64(uint64_t Addr) {
  std::atomic<uint64_t> *Atom = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
  return Atom->load(std::memory_order_acquire);
}

static bool StoreCAS64(uint64_t &Expected, uint64_t Val, uint64_t Addr) {
  std::atomic<uint64_t> *Atom = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
  return Atom->compare_exchange_strong(Expected, Val);
}

static uint32_t LoadAcquire32(uint64_t Addr) {
  std::atomic<uint32_t> *Atom = reinterpret_cast<std::atomic<uint32_t>*>(Addr);
  return Atom->load(std::memory_order_acquire);
}

static bool StoreCAS32(uint32_t &Expected, uint32_t Val, uint64_t Addr) {
  std::atomic<uint32_t> *Atom = reinterpret_cast<std::atomic<uint32_t>*>(Addr);
  return Atom->compare_exchange_strong(Expected, Val);
}

static uint8_t LoadAcquire8(uint64_t Addr) {
  std::atomic<uint8_t> *Atom = reinterpret_cast<std::atomic<uint8_t>*>(Addr);
  return Atom->load(std::memory_order_acquire);
}

static bool StoreCAS8(uint8_t &Expected, uint8_t Val, uint64_t Addr) {
  std::atomic<uint8_t> *Atom = reinterpret_cast<std::atomic<uint8_t>*>(Addr);
  return Atom->compare_exchange_strong(Expected, Val);
}

bool HandleCASPAL(void *_ucontext, void *_info, uint32_t Instr) {
  mcontext_t* mcontext = &reinterpret_cast<ucontext_t*>(_ucontext)->uc_mcontext;
  siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

  if (info->si_code != BUS_ADRALN) {
    // This only handles alignment problems
    return false;
  }

  uint32_t Size = (Instr >> 30) & 1;

  uint32_t DesiredReg1 = Instr & 0b11111;
  uint32_t DesiredReg2 = DesiredReg1 + 1;
  uint32_t ExpectedReg1 = (Instr >> 16) & 0b11111;
  uint32_t ExpectedReg2 = ExpectedReg1 + 1;
  uint32_t AddressReg = (Instr >> 5) & 0b11111;

  if (Size == 0) {
    // 32bit
    uint64_t Addr = mcontext->regs[AddressReg];

    uint32_t DesiredLower = mcontext->regs[DesiredReg1];
    uint32_t DesiredUpper = mcontext->regs[DesiredReg2];

    uint32_t ExpectedLower = mcontext->regs[ExpectedReg1];
    uint32_t ExpectedUpper = mcontext->regs[ExpectedReg2];

    // Cross-cacheline CAS doesn't work on ARM
    // It isn't even guaranteed to work on x86
    // Intel will do a "split lock" which locks the full bus
    // AMD will tear instead
    // Both cross-cacheline and cross 16byte both need dual CAS loops that can tear
    // ARMv8.4 LSE2 solves all atomic issues except cross-cacheline

    uint64_t AlignmentMask = 0b1111;
    if ((Addr & AlignmentMask) > 8) {
      uint64_t Alignment = Addr & 0b111;
      Addr &= ~0b111ULL;
      uint64_t AddrUpper = Addr + 8;

      // Crosses a 16byte boundary
      // Need to do 256bit atomic, but since that doesn't exist we need to do a dual CAS loop
      __uint128_t Mask = ~0ULL;
      Mask <<= Alignment * 8;
      __uint128_t NegMask = ~Mask;
      __uint128_t TmpExpected{};
      __uint128_t TmpDesired{};

      __uint128_t Desired = DesiredUpper;
      Desired <<= 32;
      Desired |= DesiredLower;
      Desired <<= Alignment * 8;

      __uint128_t Expected = ExpectedUpper;
      Expected <<= 32;
      Expected |= ExpectedLower;
      Expected <<= Alignment * 8;

      while (1) {
        __uint128_t LoadOrderUpper = LoadAcquire64(AddrUpper);
        LoadOrderUpper <<= 64;
        __uint128_t TmpActual = LoadOrderUpper | LoadAcquire64(Addr);

        // Set up expected
        TmpExpected = TmpActual;
        TmpExpected &= NegMask;
        TmpExpected |= Expected;

        // Set up desired
        TmpDesired = TmpExpected;
        TmpDesired &= NegMask;
        TmpDesired |= Desired;

        uint64_t TmpExpectedLower = TmpExpected;
        uint64_t TmpExpectedUpper = TmpExpected >> 64;

        uint64_t TmpDesiredLower = TmpDesired;
        uint64_t TmpDesiredUpper = TmpDesired >> 64;

        if (TmpExpected == TmpActual) {
          if (StoreCAS64(TmpExpectedUpper, TmpDesiredUpper, AddrUpper)) {
            if (StoreCAS64(TmpExpectedLower, TmpDesiredLower, Addr)) {
              // Stored successfully
              return true;
            }
            else {
              // CAS managed to tear, we can't really solve this
              // Continue down the path to let the guest know values weren't expected
            }
          }

          TmpExpected = TmpExpectedUpper;
          TmpExpected <<= 64;
          TmpExpected |= TmpExpectedLower;
        }
        else {
          // Mismatch up front
          TmpExpected = TmpActual;
        }

        // Not successful
        // Now we need to check the results to see if we need to try again
        __uint128_t FailedResultOurBits = TmpExpected & Mask;
        __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

        __uint128_t FailedDesiredOurBits = TmpDesired & Mask;
        __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
        if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
          // If the bits changed that weren't part of our regular CAS then we need to try again
          continue;
        }
        if ((FailedResultOurBits ^ FailedDesiredOurBits) != 0) {
          // If the bits changed that we were wanting to change then we have failed and can return
          // We need to extract the bits and return them in EXPECTED
          uint64_t FailedResult = FailedResultOurBits >> (Alignment * 8);
          mcontext->regs[ExpectedReg1] = FailedResult & ~0U;
          mcontext->regs[ExpectedReg2] = FailedResult >> 32;
          return true;
        }

        // This happens in the case that between Load and CAS that something has store our desired in to the memory location
        // This means our CAS fails because what we wanted to store was already stored
        uint64_t FailedResult = FailedResultOurBits >> (Alignment * 8);
        mcontext->regs[ExpectedReg1] = FailedResult & ~0U;
        mcontext->regs[ExpectedReg2] = FailedResult >> 32;
        return true;
      }
    }
    else {
      // Fits within a 16byte region
      uint64_t Alignment = Addr & 0b1111;
      Addr &= ~0b1111ULL;
      std::atomic<__uint128_t> *Atomic128 = reinterpret_cast<std::atomic<__uint128_t>*>(Addr);

      __uint128_t Mask = ~0ULL;
      Mask <<= Alignment * 8;
      __uint128_t NegMask = ~Mask;
      __uint128_t TmpExpected{};
      __uint128_t TmpDesired{};

      __uint128_t Desired = (uint64_t)DesiredUpper << 32 | DesiredLower;
      Desired <<= Alignment * 8;

      __uint128_t Expected = (uint64_t)ExpectedUpper << 32 | ExpectedLower;
      Expected <<= Alignment * 8;

      while (1) {
        TmpExpected = Atomic128->load();

        // Set up expected
        TmpExpected &= NegMask;
        TmpExpected |= Expected;

        // Set up desired
        TmpDesired = TmpExpected;
        TmpDesired &= NegMask;
        TmpDesired |= Desired;

        bool CASResult = Atomic128->compare_exchange_strong(TmpExpected, TmpDesired);
        if (CASResult) {
          // Successful, so we are done
          return true;
        }
        else {
          // Not successful
          // Now we need to check the results to see if we need to try again
          __uint128_t FailedResultOurBits = TmpExpected & Mask;
          __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

          __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
          if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
            // If the bits changed that weren't part of our regular CAS then we need to try again
            continue;
          }

          // This happens in the case that between Load and CAS that something has store our desired in to the memory location
          // This means our CAS fails because what we wanted to store was already stored
          uint64_t FailedResult = FailedResultOurBits >> (Alignment * 8);
          mcontext->regs[ExpectedReg1] = FailedResult & ~0U;
          mcontext->regs[ExpectedReg2] = FailedResult >> 32;
          return true;
        }
      }
    }
  }
  return false;
}

uint16_t DoLoad16(uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) == 15) {
    // Address crosses over 16byte or 64byte threshold
    // Needs two loads
    uint64_t AddrUpper = Addr + 1;
    uint8_t ActualUpper{};
    uint8_t ActualLower{};
    // Careful ordering here
    ActualUpper = LoadAcquire8(AddrUpper);
    ActualLower = LoadAcquire8(Addr);

    uint16_t Result = ActualUpper;
    Result <<= 8;
    Result |= ActualLower;
    return Result;
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) == 7) {
      // Crosses 8byte boundary
      // Needs 128bit load
      // Fits within a 16byte region
      uint64_t Alignment = Addr & 0b1111;
      Addr &= ~0b1111ULL;

      __uint128_t TmpResult = LoadAcquire128(Addr);

      // Zexts the result
      uint16_t Result = TmpResult >> (Alignment * 8);
      return Result;
    }
    else {
      AlignmentMask = 0b11;
      if ((Addr & AlignmentMask) == 3) {
        // Crosses 4byte boundary
        // Needs 64bit Load
        uint64_t Alignment = Addr & AlignmentMask;
        Addr &= ~AlignmentMask;

        std::atomic<uint64_t> *Atomic = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
        uint64_t TmpResult = Atomic->load();

        // Zexts the result
        uint16_t Result = TmpResult >> (Alignment * 8);
        return Result;
      }
      else {
        // Fits within 4byte boundary
        // Only needs 32bit Load
        // Only alignment offset will be 1 here
        uint64_t Alignment = Addr & AlignmentMask;
        Addr &= ~AlignmentMask;

        std::atomic<uint32_t> *Atomic = reinterpret_cast<std::atomic<uint32_t>*>(Addr);
        uint32_t TmpResult = Atomic->load();

        // Zexts the result
        uint16_t Result = TmpResult >> (Alignment * 8);
        return Result;
      }
    }
  }
}

uint32_t DoLoad32(uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 12) {
    // Address crosses over 16byte threshold
    // Needs dual 32bit load
    uint64_t Alignment = Addr & 0b11;
    Addr &= ~0b11ULL;

    uint64_t AddrUpper = Addr + 4;

    // Careful ordering here
    uint32_t ActualUpper = LoadAcquire32(AddrUpper);
    uint32_t ActualLower = LoadAcquire32(Addr);

    uint64_t Result = ActualUpper;
    Result <<= 32;
    Result |= ActualLower;
    return Result >> (Alignment * 8);
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) >= 5) {
      // Crosses 8byte boundary
      // Needs 128bit load
      // Fits within a 16byte region
      uint64_t Alignment = Addr & 0b1111;
      Addr &= ~0b1111ULL;

      __uint128_t TmpResult = LoadAcquire128(Addr);

      return TmpResult >> (Alignment * 8);
    }
    else {
      // Fits within 8byte boundary
      // Only needs 64bit CAS
      // Alignments can be [1,5)
      uint64_t Alignment = Addr & AlignmentMask;
      Addr &= ~AlignmentMask;

      std::atomic<uint64_t> *Atomic = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
      uint64_t TmpResult = Atomic->load();

      return TmpResult >> (Alignment * 8);
    }
  }
}

uint64_t DoLoad64(uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 8) {
    uint64_t Alignment = Addr & 0b111;
    Addr &= ~0b111ULL;
    uint64_t AddrUpper = Addr + 8;

    // Crosses a 16byte boundary
    // Needs two 8 byte loads
    uint64_t ActualUpper{};
    uint64_t ActualLower{};
    // Careful ordering here
    ActualUpper = LoadAcquire64(AddrUpper);
    ActualLower = LoadAcquire64(Addr);

    __uint128_t Result = ActualUpper;
    Result <<= 64;
    Result |= ActualLower;
    return Result >> (Alignment * 8);
  }
  else {
    // Fits within a 16byte region
    uint64_t Alignment = Addr & AlignmentMask;
    Addr &= ~AlignmentMask;
    __uint128_t TmpResult = LoadAcquire128(Addr);
    uint64_t Result = TmpResult >> (Alignment * 8);
    return Result;
  }
}

std::pair<uint64_t, uint64_t> DoLoad128(uint64_t Addr) {
  // Any misalignment here means we cross a 16byte boundary
  // So we need two 128bit loads
  uint64_t Alignment = Addr & 0b1111;
  Addr &= ~0b1111ULL;
  uint64_t AddrUpper = Addr + 16;

  union AlignedData {
    struct {
      __uint128_t Lower;
      __uint128_t Upper;
    } Large;
    struct {
      uint8_t Data[32];
    } Bytes;
  };

  AlignedData *Data = reinterpret_cast<AlignedData*>(alloca(sizeof(AlignedData)));
  Data->Large.Upper = LoadAcquire128(AddrUpper);
  Data->Large.Lower = LoadAcquire128(Addr);

  uint64_t ResultLower{}, ResultUpper{};
  memcpy(&ResultLower, &Data->Bytes.Data[Alignment], sizeof(uint64_t));
  memcpy(&ResultUpper, &Data->Bytes.Data[Alignment + sizeof(uint64_t)], sizeof(uint64_t));
  return {ResultLower, ResultUpper};
}

template <typename T>
using CASExpectedFn = T (*)(T Src, T Expected);
template <typename T>
using CASDesiredFn = T (*)(T Src, T Desired);

template<bool Retry>
static
uint16_t DoCAS16(
  uint16_t DesiredSrc,
  uint16_t ExpectedSrc,
  uint64_t Addr,
  CASExpectedFn<uint16_t> ExpectedFunction,
  CASDesiredFn<uint16_t> DesiredFunction) {
  // 16 bit
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) == 15) {
    // Address crosses over 16byte or 64byte threshold
    // Need a dual 8bit CAS loop
    uint64_t AddrUpper = Addr + 1;

    while (1) {
      uint8_t ActualUpper{};
      uint8_t ActualLower{};
      // Careful ordering here
      ActualUpper = LoadAcquire8(AddrUpper);
      ActualLower = LoadAcquire8(Addr);

      uint16_t Actual = ActualUpper;
      Actual <<= 8;
      Actual |= ActualLower;

      uint16_t Desired = DesiredFunction(Actual, DesiredSrc);
      uint8_t DesiredLower = Desired;
      uint8_t DesiredUpper = Desired >> 8;

      uint16_t Expected = ExpectedFunction(Actual, ExpectedSrc);
      uint8_t ExpectedLower = Expected;
      uint8_t ExpectedUpper = Expected >> 8;

      bool Tear = false;
      if (ActualUpper == ExpectedUpper &&
          ActualLower == ExpectedLower) {
        if (StoreCAS8(ExpectedUpper, DesiredUpper, AddrUpper)) {
          if (StoreCAS8(ExpectedLower, DesiredLower, Addr)) {
            // Stored successfully
            return Expected;
          }
          else {
            // CAS managed to tear, we can't really solve this
            // Continue down the path to let the guest know values weren't expected
            Tear = true;
          }
        }

        ActualLower = ExpectedLower;
        ActualUpper = ExpectedUpper;
      }

      // If the bits changed that we were wanting to change then we have failed and can return
      // We need to extract the bits and return them in EXPECTED
      uint16_t FailedResult = ActualUpper;
      FailedResult <<= 8;
      FailedResult |= ActualLower;

      if constexpr (Retry) {
        if (Tear) {
          // If we are retrying and tearing then we can't do anything here
          // XXX: Resolve with TME
          return FailedResult;
        }
        else {
          // We can retry safely
        }
      }
      else {
        // Without Retry (CAS) then we have failed regardless of tear
        // CAS failed but handled successfully
        return FailedResult;
      }
    }
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) == 7) {
      // Crosses 8byte boundary
      // Needs 128bit CAS
      // Fits within a 16byte region
      uint64_t Alignment = Addr & 0b1111;
      Addr &= ~0b1111ULL;
      std::atomic<__uint128_t> *Atomic128 = reinterpret_cast<std::atomic<__uint128_t>*>(Addr);

      __uint128_t Mask = 0xFFFF;
      Mask <<= Alignment * 8;
      __uint128_t NegMask = ~Mask;
      __uint128_t TmpExpected{};
      __uint128_t TmpDesired{};

      while (1) {
        TmpExpected = Atomic128->load();

        __uint128_t Desired = DesiredFunction(TmpExpected >> (Alignment * 8), DesiredSrc);
        Desired <<= Alignment * 8;

        __uint128_t Expected = ExpectedFunction(TmpExpected >> (Alignment * 8), ExpectedSrc);
        Expected <<= Alignment * 8;

        // Set up expected
        TmpExpected &= NegMask;
        TmpExpected |= Expected;

        // Set up desired
        TmpDesired = TmpExpected;
        TmpDesired &= NegMask;
        TmpDesired |= Desired;

        bool CASResult = Atomic128->compare_exchange_strong(TmpExpected, TmpDesired);
        if (CASResult) {
          // Successful, so we are done
          return Expected >> (Alignment * 8);
        }
        else {
          if constexpr (Retry) {
            // If we failed but we have enabled retry then just retry without checking results
            // CAS can't retry but atomic memory ops need to retry until passing
            continue;
          }
          // Not successful
          // Now we need to check the results to see if we need to try again
          __uint128_t FailedResultOurBits = TmpExpected & Mask;
          __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

          __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
          if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
            // If the bits changed that weren't part of our regular CAS then we need to try again
            continue;
          }

          // This happens in the case that between Load and CAS that something has store our desired in to the memory location
          // This means our CAS fails because what we wanted to store was already stored
          uint16_t FailedResult = FailedResultOurBits >> (Alignment * 8);
          // CAS failed but handled successfully
          return FailedResult;
        }
      }
    }
    else {
      AlignmentMask = 0b11;
      if ((Addr & AlignmentMask) == 3) {
        // Crosses 4byte boundary
        // Needs 64bit CAS
        uint64_t Alignment = Addr & AlignmentMask;
        Addr &= ~AlignmentMask;

        uint64_t Mask = 0xFFFF;
        Mask <<= Alignment * 8;

        uint64_t NegMask = ~Mask;

        uint64_t TmpExpected{};
        uint64_t TmpDesired{};

        std::atomic<uint64_t> *Atomic = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
        while (1) {
          TmpExpected = Atomic->load();

          uint64_t Desired = DesiredFunction(TmpExpected >> (Alignment * 8), DesiredSrc);
          Desired <<= Alignment * 8;

          uint64_t Expected = ExpectedFunction(TmpExpected >> (Alignment * 8), ExpectedSrc);
          Expected <<= Alignment * 8;

          // Set up expected
          TmpExpected &= NegMask;
          TmpExpected |= Expected;

          // Set up desired
          TmpDesired = TmpExpected;
          TmpDesired &= NegMask;
          TmpDesired |= Desired;

          bool CASResult = Atomic->compare_exchange_strong(TmpExpected, TmpDesired);
          if (CASResult) {
            // Successful, so we are done
            return Expected >> (Alignment * 8);
          }
          else {
            if constexpr (Retry) {
              // If we failed but we have enabled retry then just retry without checking results
              // CAS can't retry but atomic memory ops need to retry until passing
              continue;
            }
            // Not successful
            // Now we need to check the results to see if we can try again
            uint64_t FailedResultOurBits = TmpExpected & Mask;
            uint64_t FailedResultNotOurBits = TmpExpected & NegMask;

            uint64_t FailedDesiredNotOurBits = TmpDesired & NegMask;

            if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
              // If the bits changed that weren't part of our regular CAS then we need to try again
              continue;
            }

            // This happens in the case that between Load and CAS that something has store our desired in to the memory location
            // This means our CAS fails because what we wanted to store was already stored
            uint16_t FailedResult = FailedResultOurBits >> (Alignment * 8);
            // CAS failed but handled successfully
            return FailedResult;
          }
        }
      }
      else {
        // Fits within 4byte boundary
        // Only needs 32bit CAS
        // Only alignment offset will be 1 here
        uint64_t Alignment = Addr & AlignmentMask;
        Addr &= ~AlignmentMask;

        uint32_t Mask = 0xFFFF;
        Mask <<= Alignment * 8;

        uint32_t NegMask = ~Mask;

        uint32_t TmpExpected{};
        uint32_t TmpDesired{};

        std::atomic<uint32_t> *Atomic = reinterpret_cast<std::atomic<uint32_t>*>(Addr);
        while (1) {
          TmpExpected = Atomic->load();


          uint32_t Desired = DesiredFunction(TmpExpected >> (Alignment * 8), DesiredSrc);
          Desired <<= Alignment * 8;

          uint32_t Expected = ExpectedFunction(TmpExpected >> (Alignment * 8), ExpectedSrc);
          Expected <<= Alignment * 8;

          // Set up expected
          TmpExpected &= NegMask;
          TmpExpected |= Expected;

          // Set up desired
          TmpDesired = TmpExpected;
          TmpDesired &= NegMask;
          TmpDesired |= Desired;

          bool CASResult = Atomic->compare_exchange_strong(TmpExpected, TmpDesired);
          if (CASResult) {
            // Successful, so we are done
            return Expected >> (Alignment * 8);
          }
          else {
            if constexpr (Retry) {
              // If we failed but we have enabled retry then just retry without checking results
              // CAS can't retry but atomic memory ops need to retry until passing
              continue;
            }
            // Not successful
            // Now we need to check the results to see if we can try again
            uint32_t FailedResultOurBits = TmpExpected & Mask;
            uint32_t FailedResultNotOurBits = TmpExpected & NegMask;

            uint32_t FailedDesiredNotOurBits = TmpDesired & NegMask;

            if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
              // If the bits changed that weren't part of our regular CAS then we need to try again
              continue;
            }

            // This happens in the case that between Load and CAS that something has store our desired in to the memory location
            // This means our CAS fails because what we wanted to store was already stored
            uint16_t FailedResult = FailedResultOurBits >> (Alignment * 8);
            // CAS failed but handled successfully
            return FailedResult;
          }
        }
      }
    }
  }
}

template<bool Retry>
static
uint32_t DoCAS32(
  uint32_t DesiredSrc,
  uint32_t ExpectedSrc,
  uint64_t Addr,
  CASExpectedFn<uint32_t> ExpectedFunction,
  CASDesiredFn<uint32_t> DesiredFunction) {
  // 32 bit
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 12) {
    // Address crosses over 16byte threshold
    // Needs dual 4 byte CAS loop
    uint64_t Alignment = Addr & 0b11;
    Addr &= ~0b11;

    uint64_t AddrUpper = Addr + 4;

    uint64_t Mask = ~0U;
    Mask <<= Alignment * 8;
    uint64_t NegMask = ~Mask;

    // Careful ordering here
    while (1) {
      uint64_t LoadOrderUpper = LoadAcquire32(AddrUpper);
      LoadOrderUpper <<= 32;
      uint64_t TmpActual = LoadOrderUpper | LoadAcquire32(Addr);

      uint64_t Desired = DesiredFunction(TmpActual >> (Alignment * 8), DesiredSrc);
      uint64_t Expected = ExpectedFunction(TmpActual >> (Alignment * 8), ExpectedSrc);

      uint64_t TmpExpected = TmpActual;
      TmpExpected &= NegMask;
      TmpExpected |= Expected << (Alignment * 8);

      uint64_t TmpDesired = TmpExpected;
      TmpDesired &= NegMask;
      TmpDesired |= Desired << (Alignment * 8);

      bool Tear = false;
      if (TmpExpected == TmpActual) {
        uint32_t TmpExpectedLower = TmpExpected;
        uint32_t TmpExpectedUpper = TmpExpected >> 32;

        uint32_t TmpDesiredLower = TmpDesired;
        uint32_t TmpDesiredUpper = TmpDesired >> 32;

        if (StoreCAS32(TmpExpectedUpper, TmpDesiredUpper, AddrUpper)) {
          if (StoreCAS32(TmpExpectedLower, TmpDesiredLower, Addr)) {
            // Stored successfully
            return Expected;
          }
          else {
            // CAS managed to tear, we can't really solve this
            // Continue down the path to let the guest know values weren't expected
            Tear = true;
          }
        }

        TmpExpected = TmpExpectedUpper;
        TmpExpected <<= 32;
        TmpExpected |= TmpExpectedLower;
      }
      else {
        // Mismatch up front
        TmpExpected = TmpActual;
      }

      // Not successful
      // Now we need to check the results to see if we need to try again
      uint64_t FailedResultOurBits = TmpExpected & Mask;
      uint64_t FailedResultNotOurBits = TmpExpected & NegMask;

      uint64_t FailedDesiredNotOurBits = TmpDesired & NegMask;
      if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
        // If the bits changed that weren't part of our regular CAS then we need to try again
        continue;
      }

      // This happens in the case that between Load and CAS that something has store our desired in to the memory location
      // This means our CAS fails because what we wanted to store was already stored
      uint32_t FailedResult = FailedResultOurBits >> (Alignment * 8);

      if constexpr (Retry) {
        if (Tear) {
          // If we are retrying and tearing then we can't do anything here
          // XXX: Resolve with TME
          return FailedResult;
        }
        else {
          // We can retry safely
        }
      }
      else {
        // Without Retry (CAS) then we have failed regardless of tear
        // CAS failed but handled successfully
        return FailedResult;
      }
    }
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) >= 5) {
      // Crosses 8byte boundary
      // Needs 128bit CAS
      // Fits within a 16byte region
      uint64_t Alignment = Addr & 0b1111;
      Addr &= ~0b1111ULL;
      std::atomic<__uint128_t> *Atomic128 = reinterpret_cast<std::atomic<__uint128_t>*>(Addr);

      __uint128_t Mask = ~0U;
      Mask <<= Alignment * 8;
      __uint128_t NegMask = ~Mask;
      __uint128_t TmpExpected{};
      __uint128_t TmpDesired{};

      while (1) {
        __uint128_t TmpActual = Atomic128->load();

        __uint128_t Desired = DesiredFunction(TmpActual >> (Alignment * 8), DesiredSrc);
        __uint128_t Expected = ExpectedFunction(TmpActual >> (Alignment * 8), ExpectedSrc);

        // Set up expected
        TmpExpected = TmpActual;
        TmpExpected &= NegMask;
        TmpExpected |= Expected << (Alignment * 8);

        // Set up desired
        TmpDesired = TmpExpected;
        TmpDesired &= NegMask;
        TmpDesired |= Desired << (Alignment * 8);

        bool CASResult = Atomic128->compare_exchange_strong(TmpExpected, TmpDesired);
        if (CASResult) {
          // Stored successfully
          return Expected;
        }
        else {
          if constexpr (Retry) {
            // If we failed but we have enabled retry then just retry without checking results
            // CAS can't retry but atomic memory ops need to retry until passing
            continue;
          }

          // Not successful
          // Now we need to check the results to see if we need to try again
          __uint128_t FailedResultOurBits = TmpExpected & Mask;
          __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

          __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
          if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
            // If the bits changed that weren't part of our regular CAS then we need to try again
            continue;
          }

          // This happens in the case that between Load and CAS that something has store our desired in to the memory location
          // This means our CAS fails because what we wanted to store was already stored
          uint32_t FailedResult = FailedResultOurBits >> (Alignment * 8);
          // CAS failed but handled successfully
          return FailedResult;
        }
      }
    }
    else {
      // Fits within 8byte boundary
      // Only needs 64bit CAS
      // Alignments can be [1,5)
      uint64_t Alignment = Addr & AlignmentMask;
      Addr &= ~AlignmentMask;

      uint64_t Mask = ~0U;
      Mask <<= Alignment * 8;

      uint64_t NegMask = ~Mask;

      uint64_t TmpExpected{};
      uint64_t TmpDesired{};

      std::atomic<uint64_t> *Atomic = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
      while (1) {
        uint64_t TmpActual = Atomic->load();

        uint64_t Desired = DesiredFunction(TmpActual >> (Alignment * 8), DesiredSrc);
        uint64_t Expected = ExpectedFunction(TmpActual >> (Alignment * 8), ExpectedSrc);

        // Set up expected
        TmpExpected = TmpActual;
        TmpExpected &= NegMask;
        TmpExpected |= Expected << (Alignment * 8);

        // Set up desired
        TmpDesired = TmpExpected;
        TmpDesired &= NegMask;
        TmpDesired |= Desired << (Alignment * 8);

        bool CASResult = Atomic->compare_exchange_strong(TmpExpected, TmpDesired);
        if (CASResult) {
          // Stored successfully
          return Expected;
        }
        else {
          if constexpr (Retry) {
            // If we failed but we have enabled retry then just retry without checking results
            // CAS can't retry but atomic memory ops need to retry until passing
            continue;
          }

          // Not successful
          // Now we need to check the results to see if we can try again
          uint64_t FailedResultOurBits = TmpExpected & Mask;
          uint64_t FailedResultNotOurBits = TmpExpected & NegMask;

          uint64_t FailedDesiredNotOurBits = TmpDesired & NegMask;

          if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
            // If the bits changed that weren't part of our regular CAS then we need to try again
            continue;
          }

          // This happens in the case that between Load and CAS that something has store our desired in to the memory location
          // This means our CAS fails because what we wanted to store was already stored
          uint32_t FailedResult = FailedResultOurBits >> (Alignment * 8);
          // CAS failed but handled successfully
          return FailedResult;
        }
      }
    }
  }
}

template<bool Retry>
static
uint64_t DoCAS64(
  uint64_t DesiredSrc,
  uint64_t ExpectedSrc,
  uint64_t Addr,
  CASExpectedFn<uint64_t> ExpectedFunction,
  CASDesiredFn<uint64_t> DesiredFunction) {
  // 64bit
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 8) {
    uint64_t Alignment = Addr & 0b111;
    Addr &= ~0b111ULL;
    uint64_t AddrUpper = Addr + 8;

    // Crosses a 16byte boundary
    // Need to do 256bit atomic, but since that doesn't exist we need to do a dual CAS loop
    __uint128_t Mask = ~0ULL;
    Mask <<= Alignment * 8;
    __uint128_t NegMask = ~Mask;
    __uint128_t TmpExpected{};
    __uint128_t TmpDesired{};

    while (1) {
      __uint128_t LoadOrderUpper = LoadAcquire64(AddrUpper);
      LoadOrderUpper <<= 64;
      __uint128_t TmpActual = LoadOrderUpper | LoadAcquire64(Addr);

      __uint128_t Desired = DesiredFunction(TmpActual >> (Alignment * 8), DesiredSrc);
      __uint128_t Expected = ExpectedFunction(TmpActual >> (Alignment * 8), ExpectedSrc);

      // Set up expected
      TmpExpected = TmpActual;
      TmpExpected &= NegMask;
      TmpExpected |= Expected << (Alignment * 8);

      // Set up desired
      TmpDesired = TmpExpected;
      TmpDesired &= NegMask;
      TmpDesired |= Desired << (Alignment * 8);

      uint64_t TmpExpectedLower = TmpExpected;
      uint64_t TmpExpectedUpper = TmpExpected >> 64;

      uint64_t TmpDesiredLower = TmpDesired;
      uint64_t TmpDesiredUpper = TmpDesired >> 64;

      bool Tear = false;
      if (TmpExpected == TmpActual) {
        if (StoreCAS64(TmpExpectedUpper, TmpDesiredUpper, AddrUpper)) {
          if (StoreCAS64(TmpExpectedLower, TmpDesiredLower, Addr)) {
            // Stored successfully
            return Expected;
          }
          else {
            // CAS managed to tear, we can't really solve this
            // Continue down the path to let the guest know values weren't expected
            Tear = true;
          }
        }

        TmpExpected = TmpExpectedUpper;
        TmpExpected <<= 64;
        TmpExpected |= TmpExpectedLower;
      }
      else {
        // Mismatch up front
        TmpExpected = TmpActual;
      }

      // Not successful
      // Now we need to check the results to see if we need to try again
      __uint128_t FailedResultOurBits = TmpExpected & Mask;
      __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

      __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
      if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
        // If the bits changed that weren't part of our regular CAS then we need to try again
        continue;
      }

      // This happens in the case that between Load and CAS that something has store our desired in to the memory location
      // This means our CAS fails because what we wanted to store was already stored
      uint64_t FailedResult = FailedResultOurBits >> (Alignment * 8);

      if constexpr (Retry) {
        if (Tear) {
          // If we are retrying and tearing then we can't do anything here
          // XXX: Resolve with TME
          return FailedResult;
        }
        else {
          // We can retry safely
        }
      }
      else {
        // Without Retry (CAS) then we have failed regardless of tear
        // CAS failed but handled successfully
        return FailedResult;
      }
    }
  }
  else {
    // Fits within a 16byte region
    uint64_t Alignment = Addr & AlignmentMask;
    Addr &= ~AlignmentMask;
    std::atomic<__uint128_t> *Atomic128 = reinterpret_cast<std::atomic<__uint128_t>*>(Addr);

    __uint128_t Mask = ~0ULL;
    Mask <<= Alignment * 8;
    __uint128_t NegMask = ~Mask;
    __uint128_t TmpExpected{};
    __uint128_t TmpDesired{};

    while (1) {
      __uint128_t TmpActual = Atomic128->load();

      __uint128_t Desired = DesiredFunction(TmpActual >> (Alignment * 8), DesiredSrc);
      __uint128_t Expected = ExpectedFunction(TmpActual >> (Alignment * 8), ExpectedSrc);

      // Set up expected
      TmpExpected = TmpActual;
      TmpExpected &= NegMask;
      TmpExpected |= Expected << (Alignment * 8);

      // Set up desired
      TmpDesired = TmpExpected;
      TmpDesired &= NegMask;
      TmpDesired |= Desired << (Alignment * 8);

      bool CASResult = Atomic128->compare_exchange_strong(TmpExpected, TmpDesired);
      if (CASResult) {
        // Stored successfully
        return Expected;
      }
      else {
        if constexpr (Retry) {
          // If we failed but we have enabled retry then just retry without checking results
          // CAS can't retry but atomic memory ops need to retry until passing
          continue;
        }

        // Not successful
        // Now we need to check the results to see if we need to try again
        __uint128_t FailedResultOurBits = TmpExpected & Mask;
        __uint128_t FailedResultNotOurBits = TmpExpected & NegMask;

        __uint128_t FailedDesiredNotOurBits = TmpDesired & NegMask;
        if ((FailedResultNotOurBits ^ FailedDesiredNotOurBits) != 0) {
          // If the bits changed that weren't part of our regular CAS then we need to try again
          continue;
        }

        // This happens in the case that between Load and CAS that something has store our desired in to the memory location
        // This means our CAS fails because what we wanted to store was already stored
        uint64_t FailedResult = FailedResultOurBits >> (Alignment * 8);
        // CAS failed but handled successfully
        return FailedResult;
      }
    }
  }
}

bool HandleCASAL(void *_ucontext, void *_info, uint32_t Instr) {
  mcontext_t* mcontext = &reinterpret_cast<ucontext_t*>(_ucontext)->uc_mcontext;
  siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

  if (info->si_code != BUS_ADRALN) {
    // This only handles alignment problems
    return false;
  }

  uint32_t Size = 1 << (Instr >> 30);

  uint32_t DesiredReg = Instr & 0b11111;
  uint32_t ExpectedReg = (Instr >> 16) & 0b11111;
  uint32_t AddressReg = (Instr >> 5) & 0b11111;

  uint64_t Addr = mcontext->regs[AddressReg];

  // Cross-cacheline CAS doesn't work on ARM
  // It isn't even guaranteed to work on x86
  // Intel will do a "split lock" which locks the full bus
  // AMD will tear instead
  // Both cross-cacheline and cross 16byte both need dual CAS loops that can tear
  // ARMv8.4 LSE2 solves all atomic issues except cross-cacheline
  // ARM's TME extension solves the cross-cacheline problem

  // 8bit can't be unaligned
  // Only need to handle 16, 32, 64
  if (Size == 2) {
    auto Res = DoCAS16<false>(
      mcontext->regs[DesiredReg],
      mcontext->regs[ExpectedReg],
      Addr,
      [](uint16_t, uint16_t Expected) -> uint16_t {
        // Expected is just Expected
        return Expected;
      },
      [](uint16_t, uint16_t Desired) -> uint16_t {
        // Desired is just Desired
        return Desired;
      });

    // Regardless of pass or fail
    // We set the result register if it isn't a zero register
    if (ExpectedReg != 31) {
      mcontext->regs[ExpectedReg] = Res;
    }
    return true;
  }
  else if (Size == 4) {
    auto Res = DoCAS32<false>(
      mcontext->regs[DesiredReg],
      mcontext->regs[ExpectedReg],
      Addr,
      [](uint32_t, uint32_t Expected) -> uint32_t {
        // Expected is just Expected
        return Expected;
      },
      [](uint32_t, uint32_t Desired) -> uint32_t {
        // Desired is just Desired
        return Desired;
      });

    // Regardless of pass or fail
    // We set the result register if it isn't a zero register
    if (ExpectedReg != 31) {
      mcontext->regs[ExpectedReg] = Res;
    }
    return true;
  }
  else if (Size == 8) {
    auto Res = DoCAS64<false>(
      mcontext->regs[DesiredReg],
      mcontext->regs[ExpectedReg],
      Addr,
      [](uint64_t, uint64_t Expected) -> uint64_t {
        // Expected is just Expected
        return Expected;
      },
      [](uint64_t, uint64_t Desired) -> uint64_t {
        // Desired is just Desired
        return Desired;
      });

    // Regardless of pass or fail
    // We set the result register if it isn't a zero register
    if (ExpectedReg != 31) {
      mcontext->regs[ExpectedReg] = Res;
    }
    return true;
  }

  return false;
}

bool HandleAtomicMemOp(void *_ucontext, void *_info, uint32_t Instr) {
  mcontext_t* mcontext = &reinterpret_cast<ucontext_t*>(_ucontext)->uc_mcontext;
  siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

  if (info->si_code != BUS_ADRALN) {
    // This only handles alignment problems
    return false;
  }

  uint32_t Size = 1 << (Instr >> 30);
  uint32_t ResultReg = Instr & 0b11111;
  uint32_t SourceReg = (Instr >> 16) & 0b11111;
  uint32_t AddressReg = (Instr >> 5) & 0b11111;

  uint64_t Addr = mcontext->regs[AddressReg];

  uint8_t Op = (Instr >> 12) & 0xF;

  if (Size == 2) {
    auto NOPExpected = [](uint16_t SrcVal, uint16_t) -> uint16_t {
      return SrcVal;
    };

    auto ADDDesired = [](uint16_t SrcVal, uint16_t Desired) -> uint16_t {
      return SrcVal + Desired;
    };

    auto CLRDesired = [](uint16_t SrcVal, uint16_t Desired) -> uint16_t {
      return SrcVal & ~Desired;
    };

    auto EORDesired = [](uint16_t SrcVal, uint16_t Desired) -> uint16_t {
      return SrcVal ^ Desired;
    };

    auto SETDesired = [](uint16_t SrcVal, uint16_t Desired) -> uint16_t {
      return SrcVal | Desired;
    };

    auto SWAPDesired = [](uint16_t SrcVal, uint16_t Desired) -> uint16_t {
      return Desired;
    };

    CASDesiredFn<uint16_t> DesiredFunction{};

    switch (Op) {
      case ATOMIC_ADD_OP:
        DesiredFunction = ADDDesired;
        break;
      case ATOMIC_CLR_OP:
        DesiredFunction = CLRDesired;
        break;
      case ATOMIC_EOR_OP:
        DesiredFunction = EORDesired;
        break;
      case ATOMIC_SET_OP:
        DesiredFunction = SETDesired;
        break;
      case ATOMIC_SWAP_OP:
        DesiredFunction = SWAPDesired;
        break;
      default:
        LogMan::Msg::E("Unhandled JIT SIGBUS Atomic mem op 0x%02x", Op);
        return false;
        break;
    }

    auto Res = DoCAS16<true>(
      mcontext->regs[SourceReg],
      0, // Unused
      Addr,
      NOPExpected,
      DesiredFunction);
    // If we passed and our destination register is not zero
    // Then we need to update the result register with what was in memory
    if (ResultReg != 31) {
      mcontext->regs[ResultReg] = Res;
    }
    return true;
  }
  else if (Size == 4) {
    auto NOPExpected = [](uint32_t SrcVal, uint32_t) -> uint32_t {
      return SrcVal;
    };

    auto ADDDesired = [](uint32_t SrcVal, uint32_t Desired) -> uint32_t {
      return SrcVal + Desired;
    };

    auto CLRDesired = [](uint32_t SrcVal, uint32_t Desired) -> uint32_t {
      return SrcVal & ~Desired;
    };

    auto EORDesired = [](uint32_t SrcVal, uint32_t Desired) -> uint32_t {
      return SrcVal ^ Desired;
    };

    auto SETDesired = [](uint32_t SrcVal, uint32_t Desired) -> uint32_t {
      return SrcVal | Desired;
    };

    auto SWAPDesired = [](uint32_t SrcVal, uint32_t Desired) -> uint32_t {
      return Desired;
    };

    CASDesiredFn<uint32_t> DesiredFunction{};

    switch (Op) {
      case ATOMIC_ADD_OP:
        DesiredFunction = ADDDesired;
        break;
      case ATOMIC_CLR_OP:
        DesiredFunction = CLRDesired;
        break;
      case ATOMIC_EOR_OP:
        DesiredFunction = EORDesired;
        break;
      case ATOMIC_SET_OP:
        DesiredFunction = SETDesired;
        break;
      case ATOMIC_SWAP_OP:
        DesiredFunction = SWAPDesired;
        break;
      default:
        LogMan::Msg::E("Unhandled JIT SIGBUS Atomic mem op 0x%02x", Op);
        return false;
        break;
    }

    auto Res = DoCAS32<true>(
      mcontext->regs[SourceReg],
      0, // Unused
      Addr,
      NOPExpected,
      DesiredFunction);
    // If we passed and our destination register is not zero
    // Then we need to update the result register with what was in memory
    if (ResultReg != 31) {
      mcontext->regs[ResultReg] = Res;
    }
    return true;
  }
  else if (Size == 8) {
    auto NOPExpected = [](uint64_t SrcVal, uint64_t) -> uint64_t {
      return SrcVal;
    };

    auto ADDDesired = [](uint64_t SrcVal, uint64_t Desired) -> uint64_t {
      return SrcVal + Desired;
    };

    auto CLRDesired = [](uint64_t SrcVal, uint64_t Desired) -> uint64_t {
      return SrcVal & ~Desired;
    };

    auto EORDesired = [](uint64_t SrcVal, uint64_t Desired) -> uint64_t {
      return SrcVal ^ Desired;
    };

    auto SETDesired = [](uint64_t SrcVal, uint64_t Desired) -> uint64_t {
      return SrcVal | Desired;
    };

    auto SWAPDesired = [](uint64_t SrcVal, uint64_t Desired) -> uint64_t {
      return Desired;
    };

    CASDesiredFn<uint64_t> DesiredFunction{};

    switch (Op) {
      case ATOMIC_ADD_OP:
        DesiredFunction = ADDDesired;
        break;
      case ATOMIC_CLR_OP:
        DesiredFunction = CLRDesired;
        break;
      case ATOMIC_EOR_OP:
        DesiredFunction = EORDesired;
        break;
      case ATOMIC_SET_OP:
        DesiredFunction = SETDesired;
        break;
      case ATOMIC_SWAP_OP:
        DesiredFunction = SWAPDesired;
        break;
      default:
        LogMan::Msg::E("Unhandled JIT SIGBUS Atomic mem op 0x%02x", Op);
        return false;
        break;
    }

    auto Res = DoCAS64<true>(
      mcontext->regs[SourceReg],
      0, // Unused
      Addr,
      NOPExpected,
      DesiredFunction);
    // If we passed and our destination register is not zero
    // Then we need to update the result register with what was in memory
    if (ResultReg != 31) {
      mcontext->regs[ResultReg] = Res;
    }
    return true;
  }

  return false;
}

}
