// SPDX-License-Identifier: MIT
#pragma once

#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Debug/InternalThreadState.h>

namespace FEX::Windows {
template<typename TReg>
static inline EXCEPTION_RECORD
HandleGuestException(FEXCore::Core::CpuStateFrame::SynchronousFaultDataStruct& Fault, const EXCEPTION_RECORD& Src, TReg& Rip, TReg Rax) {
  EXCEPTION_RECORD Dst = Src;
  Dst.ExceptionAddress = reinterpret_cast<void*>(Rip);

  if (!Fault.FaultToTopAndGeneratedException) {
    return Dst;
  }
  Fault.FaultToTopAndGeneratedException = false;

  Dst.ExceptionFlags = 0;
  Dst.NumberParameters = 0;

  switch (Fault.Signal) {
  case FEXCore::Core::FAULT_SIGILL: Dst.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION; return Dst;
  case FEXCore::Core::FAULT_SIGTRAP:
    switch (Fault.TrapNo) {
    case FEXCore::X86State::X86_TRAPNO_DB: Dst.ExceptionCode = EXCEPTION_SINGLE_STEP; return Dst;
    case FEXCore::X86State::X86_TRAPNO_BP:
      Rip -= 1;
      Dst.ExceptionAddress = reinterpret_cast<void*>(Rip);
      Dst.ExceptionCode = EXCEPTION_BREAKPOINT;
      Dst.NumberParameters = 1;
      Dst.ExceptionInformation[0] = 0;
      return Dst;
    default: LogMan::Msg::EFmt("Unknown SIGTRAP trap: {}", Fault.TrapNo); break;
    }
    break;
  case FEXCore::Core::FAULT_SIGSEGV:
    switch (Fault.TrapNo) {
    case FEXCore::X86State::X86_TRAPNO_GP:
      if ((Fault.err_code & 0b111) == 0b010) {
        switch (Fault.err_code >> 3) {
        case 0x2d:
          Rip += 2;
          Dst.ExceptionCode = EXCEPTION_BREAKPOINT;
          Dst.ExceptionAddress = reinterpret_cast<void*>(Rip + 1);
          Dst.NumberParameters = 1;
          Dst.ExceptionInformation[0] = Rax; // RAX
          // Note that ExceptionAddress doesn't equal the reported context RIP here, this discrepancy expected and not having it can trigger anti-debug logic.
          return Dst;
        default: LogMan::Msg::EFmt("Unknown interrupt: 0x{:X}", Fault.err_code >> 3); break;
        }
      } else {
        Dst.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        return Dst;
      }
      break;
    case FEXCore::X86State::X86_TRAPNO_OF: Dst.ExceptionCode = EXCEPTION_INT_OVERFLOW; return Dst;
    default: LogMan::Msg::EFmt("Unknown SIGSEGV trap: {}", Fault.TrapNo); break;
    }
    break;
  default: LogMan::Msg::EFmt("Unknown signal type: {}", Fault.Signal); break;
  }

  // Default to SIGILL
  Dst.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
  return Dst;
}
} // namespace FEX::Windows
