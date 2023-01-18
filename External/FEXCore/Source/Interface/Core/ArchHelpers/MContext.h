#pragma once

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/UContext.h>
#include <FEXCore/Core/X86Enums.h>

#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <stdint.h>
#include <type_traits>

namespace FEXCore::ArchHelpers::Context {

enum ContextFlags : uint32_t {
  CONTEXT_FLAG_INJIT = (1U << 0),
};

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
constexpr uint64_t STACK_COOKIE_MAGIC = 0x4142434445464748ULL;
#endif

struct X86ContextBackup {
  // Host State
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  // During debug builds, insert a cookie on the stack.
  // This is useful for validation that the stack is trying to be restored from the correct location.
  // During stack restore, we ensure this is set to the value we expect.
  // If given an incorrect stack location, or corrupted stack then this cookie will be wrong.
  uint64_t StackCookie;
#endif
  // RIP and RSP is stored in GPRs here
  uint64_t GPRs[23];
  FEXCore::x86_64::_libc_fpstate FPRState;
  uint64_t sa_mask;
  bool FaultToTopAndGeneratedException;

  // Guest state
  int Signal;
  uint32_t Flags;
  uint64_t OriginalRIP;
  uint64_t FPStateLocation;
  uint64_t UContextLocation;
  uint64_t SigInfoLocation;
  FEXCore::Core::CPUState GuestState;
  static constexpr int RedZoneSize = 128;
};

struct ArmContextBackup {
  // Host State
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  uint64_t StackCookie;
#endif
  uint64_t GPRs[31];
  uint64_t PrevSP;
  uint64_t PrevPC;
  uint64_t PState;
  uint32_t FPSR;
  uint32_t FPCR;
  __uint128_t FPRs[32];
  uint64_t sa_mask;
  bool FaultToTopAndGeneratedException;

  // Guest state
  int Signal;
  uint32_t Flags;
  uint64_t OriginalRIP;
  uint64_t FPStateLocation;
  uint64_t UContextLocation;
  uint64_t SigInfoLocation;
  FEXCore::Core::CPUState GuestState;

  // Arm64 doesn't have a red zone
  static constexpr int RedZoneSize = 0;
};

static inline ucontext_t* GetUContext(void* ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  return _context;
}

static inline mcontext_t* GetMContext(void* ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  return &_context->uc_mcontext;
}


#ifdef _M_ARM_64

constexpr uint32_t FPR_MAGIC = 0x46508001U;
constexpr uint32_t ESR1_MAGIC = 0x45535201U;

struct HostCTXHeader {
  uint32_t Magic;
  uint32_t Size;
};

struct HostFPRState {
  HostCTXHeader Head;
  uint32_t FPSR;
  uint32_t FPCR;
  __uint128_t FPRs[32];
};

struct HostESRState {
  HostCTXHeader Head;
  uint64_t ESR;
};

static inline uint64_t GetSp(void* ucontext) {
  return GetMContext(ucontext)->sp;
}

static inline uint64_t GetPc(void* ucontext) {
  return GetMContext(ucontext)->pc;
}

static inline void SetSp(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->sp = val;
}

static inline void SetPc(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->pc = val;
}

static inline uint64_t GetState(void* ucontext) {
  return GetMContext(ucontext)->regs[28];
}

static inline void SetState(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->regs[28] = val;
}

static inline uint64_t GetArmReg(void* ucontext, uint32_t id) {
  return GetMContext(ucontext)->regs[id];
}

static inline void SetArmReg(void* ucontext, uint32_t id, uint64_t val) {
  GetMContext(ucontext)->regs[id] = val;
}

static inline __uint128_t GetArmFPR(void* ucontext, uint32_t id) {
  auto MContext = GetMContext(ucontext);
  HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&MContext->__reserved[0]);
  LOGMAN_THROW_AA_FMT(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x{:08x}", HostState->Head.Magic);

  return HostState->FPRs[id];
}

static inline uint64_t GetArmESR(void* ucontext) {
  auto MContext = GetMContext(ucontext);

  size_t i = 0;
  auto HostState = reinterpret_cast<HostCTXHeader*>(&MContext->__reserved[i]);
  do {
    if (HostState->Magic == ESR1_MAGIC) {
      auto ESR = reinterpret_cast<HostESRState*>(HostState);
      return ESR->ESR;
    }
    i += HostState->Size;
    HostState = reinterpret_cast<HostCTXHeader*>(&MContext->__reserved[i]);
  } while (HostState->Size != 0);

  return 0;
}

constexpr static uint64_t ESR1_EC = 0b111111U << 26;
constexpr static uint64_t ESR1_EC_DataAbort = 0b100100U << 26;

// Write-Not-Read flag
// When set - Abort is due to a write
constexpr static uint64_t ESR1_WNR = 1 << 6;

// DFSC - Default Status Code
// Translation fault - No page mapped
// Permissions fault - Page mapped but with incorrect permission from access.
constexpr static uint64_t ESR1_DataAbort_DFSC = 0b111111;
constexpr static uint64_t ESR1_DataAbort_TranslationFault_EL0 = 0b000111;
constexpr static uint64_t ESR1_DataAbort_PermissionFault_EL0 = 0b001111;
constexpr static uint64_t ESR1_DataAbort_Level = 0b11;
constexpr static uint64_t ESR1_DataAbort_Level_EL3 = 0b00;
constexpr static uint64_t ESR1_DataAbort_Level_EL2 = 0b01;
constexpr static uint64_t ESR1_DataAbort_Level_EL1 = 0b10;
constexpr static uint64_t ESR1_DataAbort_Level_EL0 = 0b11;

static inline uint32_t GetProtectFlags(void* ucontext) {
  uint64_t ESR = GetArmESR(ucontext);
  LOGMAN_THROW_A_FMT((ESR & ESR1_EC) == ESR1_EC_DataAbort, "Unknown ESR1 EC type: 0x{:x} != 0x{:x}", ESR & ESR1_EC, ESR1_EC_DataAbort);

  uint32_t ProtectFlags{};
  if ((ESR & ESR1_DataAbort_Level) == ESR1_DataAbort_Level_EL0) {
    // Always a user error for us.
    ProtectFlags |= X86State::X86_PF_USER;
  }

  if (ESR & ESR1_WNR) {
    // Fault was due to a write
    ProtectFlags |= X86State::X86_PF_WRITE;
  }

  // PF_PROT is not returned to user on x86, so don't return the difference between permission fault and translation fault.
  return ProtectFlags;
}

using ContextBackup = ArmContextBackup;
template <typename T>
static inline void BackupContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, ArmContextBackup>::value) {
    auto _ucontext = GetUContext(ucontext);
    auto _mcontext = GetMContext(ucontext);

    memcpy(&Backup->GPRs[0], &_mcontext->regs[0], 31 * sizeof(uint64_t));
    Backup->PrevSP = ArchHelpers::Context::GetSp(ucontext);
    Backup->PrevPC = ArchHelpers::Context::GetPc(ucontext);
    Backup->PState = _mcontext->pstate;

    // Host FPR state starts at _mcontext->reserved[0];
    HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
    LOGMAN_THROW_AA_FMT(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x{:08x}", HostState->Head.Magic);
    Backup->FPSR = HostState->FPSR;
    Backup->FPCR = HostState->FPCR;
    memcpy(&Backup->FPRs[0], &HostState->FPRs[0], 32 * sizeof(__uint128_t));

    // Save the signal mask so we can restore it
    memcpy(&Backup->sa_mask, &_ucontext->uc_sigmask, sizeof(uint64_t));

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    Backup->StackCookie = STACK_COOKIE_MAGIC;
#endif
  } else {
    // This must be a runtime error
    ERROR_AND_DIE_FMT("Wrong context type");
  }
}

template <typename T>
static inline void RestoreContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, ArmContextBackup>::value) {
    auto _ucontext = GetUContext(ucontext);
   auto _mcontext = GetMContext(ucontext);

    HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
    LOGMAN_THROW_AA_FMT(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x{:08x}", HostState->Head.Magic);
    memcpy(&HostState->FPRs[0], &Backup->FPRs[0], 32 * sizeof(__uint128_t));
    HostState->FPCR = Backup->FPCR;
    HostState->FPSR = Backup->FPSR;

    // Restore GPRs and other state
    _mcontext->pstate = Backup->PState;
    ArchHelpers::Context::SetPc(ucontext, Backup->PrevPC);
    ArchHelpers::Context::SetSp(ucontext, Backup->PrevSP);
    memcpy(&_mcontext->regs[0], &Backup->GPRs[0], 31 * sizeof(uint64_t));

    // Restore the signal mask now
    memcpy(&_ucontext->uc_sigmask, &Backup->sa_mask, sizeof(uint64_t));

    LOGMAN_THROW_A_FMT(Backup->StackCookie == STACK_COOKIE_MAGIC, "Stack cookie didn't match! 0x{:x}", Backup->StackCookie);
  } else {
    // This must be a runtime error
    ERROR_AND_DIE_FMT("Wrong context type");
  }
}

#endif

#ifdef _M_X86_64

static inline uint64_t GetSp(void* ucontext) {
  return GetMContext(ucontext)->gregs[REG_RSP];
}

static inline uint64_t GetPc(void* ucontext) {
  return GetMContext(ucontext)->gregs[REG_RIP];
  }

static inline void SetSp(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->gregs[REG_RSP] = val;
}

static inline void SetPc(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->gregs[REG_RIP] = val;
}

static inline uint64_t GetState(void* ucontext) {
  return GetMContext(ucontext)->gregs[REG_R14];
}

static inline void SetState(void* ucontext, uint64_t val) {
  GetMContext(ucontext)->gregs[REG_R14] = val;
}

static inline uint64_t GetArmReg(void* ucontext, uint32_t id) {
  ERROR_AND_DIE_FMT("Not impelented for x86 host");
}

static inline void SetArmReg(void* ucontext, uint32_t id, uint64_t val) {
  ERROR_AND_DIE_FMT("Not impelented for x86 host");
}

static inline __uint128_t GetArmFPR(void* ucontext, uint32_t id) {
  ERROR_AND_DIE_FMT("Not implemented for x86 host");
}

static inline uint32_t GetProtectFlags(void* ucontext) {
  return GetMContext(ucontext)->gregs[REG_ERR];
}

using ContextBackup = X86ContextBackup;
template <typename T>
static inline void BackupContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, X86ContextBackup>::value) {
    auto _ucontext = GetUContext(ucontext);
    auto _mcontext = GetMContext(ucontext);

    // Copy the GPRs
    memcpy(&Backup->GPRs[0], &_mcontext->gregs[0], sizeof(X86ContextBackup::GPRs));
    // Copy the FPRState
    memcpy(&Backup->FPRState, _mcontext->fpregs, sizeof(X86ContextBackup::FPRState));
    // XXX: Save 256bit and 512bit AVX register state

    // Save the signal mask so we can restore it
    memcpy(&Backup->sa_mask, &_ucontext->uc_sigmask, sizeof(uint64_t));

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    Backup->StackCookie = STACK_COOKIE_MAGIC;
#endif
  } else {
    // This must be a runtime error
    ERROR_AND_DIE_FMT("Wrong context type");
  }
}

template <typename T>
static inline void RestoreContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, X86ContextBackup>::value) {
    auto _ucontext = GetUContext(ucontext);
    auto _mcontext = GetMContext(ucontext);

    // Copy the GPRs
    memcpy(&_mcontext->gregs[0], &Backup->GPRs[0], sizeof(X86ContextBackup::GPRs));
    // Copy the FPRState
    memcpy(_mcontext->fpregs, &Backup->FPRState, sizeof(X86ContextBackup::FPRState));

    // Restore the signal mask now
    memcpy(&_ucontext->uc_sigmask, &Backup->sa_mask, sizeof(uint64_t));

    LOGMAN_THROW_A_FMT(Backup->StackCookie == STACK_COOKIE_MAGIC, "Stack cookie didn't match! 0x{:x}", Backup->StackCookie);
  } else {
    // This must be a runtime error
    ERROR_AND_DIE_FMT("Wrong context type");
  }
}

#endif

} // namespace FEXCore::ArchHelpers::Context
