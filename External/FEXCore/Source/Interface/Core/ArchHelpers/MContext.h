#pragma once

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/UContext.h>

#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <stdint.h>
#include <type_traits>


namespace FEXCore::CPU {


struct X86ContextBackup {
  // Host State
  // RIP and RSP is stored in GPRs here
  uint64_t GPRs[23];
  FEXCore::x86_64::_libc_fpstate FPRState;

  // Guest state
  int Signal;
  FEXCore::Core::CPUState GuestState;
};

struct ArmContextBackup {
  // Host State
  uint64_t GPRs[31];
  uint64_t PrevSP;
  uint64_t PrevPC;
  uint64_t PState;
  uint32_t FPSR;
  uint32_t FPCR;
  __uint128_t FPRs[32];

  // Guest state
  int Signal;
  FEXCore::Core::CPUState GuestState;
};

}

namespace FEXCore::ArchHelpers::Context {

static inline mcontext_t* GetMContext(void* ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  return &_context->uc_mcontext;
}


#ifdef _M_ARM_64

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

constexpr uint32_t FPR_MAGIC = 0x46508001U;

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

template <typename T>
static inline void BackupContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, FEXCore::CPU::ArmContextBackup>::value) {
    auto _mcontext = GetMContext(ucontext);

    memcpy(&Backup->GPRs[0], &_mcontext->regs[0], 31 * sizeof(uint64_t));
    Backup->PrevSP = ArchHelpers::Context::GetSp(ucontext);
    Backup->PrevPC = ArchHelpers::Context::GetPc(ucontext);
    Backup->PState = _mcontext->pstate;

    // Host FPR state starts at _mcontext->reserved[0];
    HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
    LogMan::Throw::A(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x%08x", HostState->Head.Magic);
    Backup->FPSR = HostState->FPSR;
    Backup->FPCR = HostState->FPCR;
    memcpy(&Backup->FPRs[0], &HostState->FPRs[0], 32 * sizeof(__uint128_t));
  } else {
      ERROR_AND_DIE("Wrong context type"); // This must be a runtime error
  }
}

template <typename T>
static inline void RestoreContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, FEXCore::CPU::ArmContextBackup>::value) {
   auto _mcontext = GetMContext(ucontext);

    HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
    LogMan::Throw::A(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x%08x", HostState->Head.Magic);
    memcpy(&HostState->FPRs[0], &Backup->FPRs[0], 32 * sizeof(__uint128_t));
    HostState->FPCR = Backup->FPCR;
    HostState->FPSR = Backup->FPSR;

    // Restore GPRs and other state
    _mcontext->pstate = Backup->PState;
    ArchHelpers::Context::SetPc(ucontext, Backup->PrevPC);
    ArchHelpers::Context::SetSp(ucontext, Backup->PrevSP);
    memcpy(&_mcontext->regs[0], &Backup->GPRs[0], 31 * sizeof(uint64_t));
  } else {
      ERROR_AND_DIE("Wrong context type"); // This must be a runtime error
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
  ERROR_AND_DIE("Not impelented for x86 host");
}

static inline void SetArmReg(void* ucontext, uint32_t id, uint64_t val) {
  ERROR_AND_DIE("Not impelented for x86 host");
}

template <typename T>
static inline void BackupContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, FEXCore::CPU::X86ContextBackup>::value) {
    auto _mcontext = GetMContext(ucontext);

    // Copy the GPRs
    memcpy(&Backup->GPRs[0], &_mcontext->gregs[0], sizeof(FEXCore::CPU::X86ContextBackup::GPRs));
    // Copy the FPRState
    memcpy(&Backup->FPRState, _mcontext->fpregs, sizeof(FEXCore::CPU::X86ContextBackup::FPRState));
    // XXX: Save 256bit and 512bit AVX register state
  } else {
    ERROR_AND_DIE("Wrong context type"); // This must be a runtime error
  }
}

template <typename T>
static inline void RestoreContext(void* ucontext, T *Backup) {
  if constexpr (std::is_same<T, FEXCore::CPU::X86ContextBackup>::value) {
    auto _mcontext = GetMContext(ucontext);

    // Copy the GPRs
    memcpy(&_mcontext->gregs[0], &Backup->GPRs[0], sizeof(FEXCore::CPU::X86ContextBackup::GPRs));
    // Copy the FPRState
    memcpy(_mcontext->fpregs, &Backup->FPRState, sizeof(FEXCore::CPU::X86ContextBackup::FPRState));
  } else {
    ERROR_AND_DIE("Wrong context type"); // This must be a runtime error
  }
}

#endif

} // namespace FEXCore::ArchHelpers::Context