// SPDX-License-Identifier: MIT
#include "ArchHelpers/UContext.h"
#include "LinuxSyscalls/SignalDelegator.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/unordered_set.h>
#include <FEXCore/Utils/LogManager.h>

#ifdef _M_X86_64
#include "Common/X86Features.h"
#include <asm/ldt.h>
#include <sys/syscall.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ucontext.h>

#include <signal.h>

#ifdef _M_X86_64
static inline int modify_ldt(int func, void* ldt) {
  return ::syscall(SYS_modify_ldt, func, ldt, sizeof(struct user_desc));
}

__attribute__((naked)) void Dispatcher(uintptr_t BranchTarget, void* ReturningStackLocation, int CodeSegment, int SupportsFSGSBase) {
  // BranchTarget: rdi
  // ReturningStackLocation: rsi
  // CodeSegment: rdx
  // SupportsFSGSBase: rcx
  __asm volatile(R"(
  .intel_syntax noprefix;
    // x86-64 ABI has the stack aligned when /call/ happens
    // Which means the destination has a misaligned stack at that point
    push rbx;
    push rbp;
    push r12;
    push r13;
    push r14;
    push r15;

    test ecx, ecx;
    je 1f;
    rdfsbase rbx;
    push rbx;
    rdgsbase rbx;
    push rbx;
    1:

    push rcx;

    // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
    // regardless of where we were in the stack
    mov [rsi], rsp;

    // Clear all state going in to the branch target.
    // Only remaining state, rdi, rdx, rsp
    mov rax, 0;
    mov rbx, 0;
    mov rcx, 0;
    mov rbp, 0;
    mov rsi, 0;
    mov r8, 0;
    mov r9, 0;
    mov r10, 0;
    mov r11, 0;
    mov r12, 0;
    mov r13, 0;
    mov r14, 0;
    mov r15, 0;
    finit;

    cmp rdx, 0;
    jnz .32_bit;

    .64_bit:
      // Clear rdx and also set flags to a sane state.
      xor rdx, rdx;
      mov rsp, 0;

      // Tail-call
      jmp rdi;

    .32_bit:
      // Far call needs to go through a gate
      // This is setup just like the following packing
      // {
      //  uint32_t RIP;
      //  uint16_t CodeSegment;
      // }
      sub rsp, 16
      mov [rsp], edi;
      mov [rsp+4], dx

      // Clear rdx and also set flags to a sane state.
      xor rdx, rdx;

      GetCodeSegmentEntryLocation:
      hlt;

      jmp fword ptr [rsp];

    ThreadStopHandlerAddress:

    pop rcx
    test ecx, ecx;
    je 1f;
    pop rbx;
    wrgsbase rbx;
    pop rbx;
    wrfsbase rbx;
    1:

    pop r15;
    pop r14;
    pop r13;
    pop r12;
    pop rbp;
    pop rbx;

    ret;

  .att_syntax prefix;
  )" ::
                   : "memory", "cc");
}

extern "C" void* GetCodeSegmentEntryLocation;
uintptr_t GetCodeSegmentEntryLocationPtr = (uintptr_t)&GetCodeSegmentEntryLocation;
extern "C" void* ThreadStopHandlerAddress;
uintptr_t ThreadStopHandlerAddressPtr = (uintptr_t)&ThreadStopHandlerAddress;

class x86HostRunner final {
public:
  x86HostRunner() {
    Setup32BitCodeSegment();
  }

  bool HandleSIGSEGV(FEXCore::Core::CPUState* OutState, int Signal, void* info, void* ucontext) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Check our current instruction that we just executed to ensure it was an HLT
    uint8_t* Inst {};

    Inst = reinterpret_cast<uint8_t*>(_mcontext->gregs[REG_RIP]);
    if (!Is64BitMode()) {
      if (_mcontext->gregs[REG_RIP] == ::GetCodeSegmentEntryLocationPtr) {
        // Backup the CSGSFS register
        GlobalCodeSegmentEntry = _mcontext->gregs[REG_CSGSFS];
        // Skip past this hlt and keep running
        _mcontext->gregs[REG_RIP] += 1;
        return true;
      }
    }
    constexpr uint8_t HLT = 0xF4;
    if (Inst[0] != HLT) {
      return false;
    }

    // Store our host state in to the guest for testing against
    OutState->gregs[FEXCore::X86State::REG_RAX] = _mcontext->gregs[REG_RAX];
    OutState->gregs[FEXCore::X86State::REG_RBX] = _mcontext->gregs[REG_RBX];
    OutState->gregs[FEXCore::X86State::REG_RCX] = _mcontext->gregs[REG_RCX];
    OutState->gregs[FEXCore::X86State::REG_RDX] = _mcontext->gregs[REG_RDX];
    OutState->gregs[FEXCore::X86State::REG_RBP] = _mcontext->gregs[REG_RBP];
    OutState->gregs[FEXCore::X86State::REG_RSI] = _mcontext->gregs[REG_RSI];
    OutState->gregs[FEXCore::X86State::REG_RDI] = _mcontext->gregs[REG_RDI];
    OutState->gregs[FEXCore::X86State::REG_RSP] = _mcontext->gregs[REG_RSP];
    OutState->gregs[FEXCore::X86State::REG_R8] = _mcontext->gregs[REG_R8];
    OutState->gregs[FEXCore::X86State::REG_R9] = _mcontext->gregs[REG_R9];
    OutState->gregs[FEXCore::X86State::REG_R10] = _mcontext->gregs[REG_R10];
    OutState->gregs[FEXCore::X86State::REG_R11] = _mcontext->gregs[REG_R11];
    OutState->gregs[FEXCore::X86State::REG_R12] = _mcontext->gregs[REG_R12];
    OutState->gregs[FEXCore::X86State::REG_R13] = _mcontext->gregs[REG_R13];
    OutState->gregs[FEXCore::X86State::REG_R14] = _mcontext->gregs[REG_R14];
    OutState->gregs[FEXCore::X86State::REG_R15] = _mcontext->gregs[REG_R15];
    OutState->rip = _mcontext->gregs[REG_RIP];

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
      memcpy(&OutState->xmm.avx.data[i], &_mcontext->fpregs->_xmm[i], sizeof(_mcontext->fpregs->_xmm[0]));
    }
    const auto* xstate = reinterpret_cast<FEXCore::x86_64::xstate*>(_mcontext->fpregs);
    const auto* reserved = &xstate->fpstate.sw_reserved;
    if (reserved->HasExtendedContext() && reserved->HasYMMH()) {
      for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; i++) {
        memcpy(&OutState->xmm.avx.data[i][2], &xstate->ymmh.ymmh_space[i], sizeof(xstate->ymmh.ymmh_space[0]));
      }
    }

    const uint16_t CurrentOffset = (_mcontext->fpregs->swd >> 11) & 7;
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
      memcpy(&OutState->mm[(i + CurrentOffset) % 8], &_mcontext->fpregs->_st[i], sizeof(_mcontext->fpregs->_st[0]));
    }

    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the JIT and get out safely
    _mcontext->gregs[REG_RSP] = ReturningStackLocation;

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ::ThreadStopHandlerAddressPtr;

    if (!Is64BitMode()) {
      // Unset code segment so we can jump back in to 64-bit mode
      _mcontext->gregs[REG_CSGSFS] = GlobalCodeSegmentEntry;
    }

    return true;
  }

  void Dispatch(uint64_t InitialRip) {
    FEX::X86::Features Feature {};
    Dispatcher(InitialRip, &ReturningStackLocation, CodeSegmentEntry, Feature.Feat_fsgsbase);
  }

private:
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  uint64_t GlobalCodeSegmentEntry {};
  int CodeSegmentEntry {};
  uint64_t ReturningStackLocation;

  uint32_t MakeSelector(int Segment, bool LDT) const {
    // Selector Index, Table Indicator (1 = LDT, 0 = GDT), CPL (3 = userland)
    return (Segment << 3) | ((uint32_t)LDT << 2) | 3;
  };

  void Setup32BitCodeSegment() {
    if (Is64BitMode()) {
      return;
    }

    struct user_desc ldt {};
    ldt.entry_number = 1;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U;   // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_CODE;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    int Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }

    CodeSegmentEntry = MakeSelector(ldt.entry_number, 1);

    // Make the data segment follow directly after the code segment
    // Overlapping region makes it read/write
    ldt.entry_number = 2;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U;   // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_DATA;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }

    // Stack entry overlapping data
    ldt.entry_number = 3;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U;   // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_STACK;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }
  }
};

void RunAsHost(fextl::unique_ptr<FEX::HLE::SignalDelegator>& SignalDelegation, uintptr_t InitialRip, FEXCore::Core::CPUState* OutputState) {
  x86HostRunner runner;
  SignalDelegation->RegisterHostSignalHandler(
    SIGSEGV,
    [&runner, OutputState](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
      return runner.HandleSIGSEGV(OutputState, Signal, info, ucontext);
    },
    true);

  runner.Dispatch(InitialRip);
}
#else
void RunAsHost(fextl::unique_ptr<FEX::HLE::SignalDelegator>& SignalDelegation, uintptr_t InitialRip, FEXCore::Core::CPUState* OutputState) {
  LOGMAN_MSG_A_FMT("RunAsHost doesn't exist for this host");
}
#endif
