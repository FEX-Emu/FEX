#pragma once
#include "LogManager.h"
#include <stdint.h>

namespace FEXCore::HLE {
#define INVALID_OP { LogMan::Msg::A("Tried to syscall with unknown number of registers");  return 0; }
  class SyscallVisitor {
  public:
    SyscallVisitor(uint32_t Mask) : SyscallVisitor(Mask, false) {}
    SyscallVisitor(uint32_t Mask, bool Constant) : ArgsMask { Mask }, ConstantVal { Constant } {}

    /**
     * @brief If this syscall returns a constant value regardless of state then we can just read the value at compile time
     * Won't happen often
     *
     * @return true if it is constant value
     */
    bool IsConstant() { return ConstantVal; }

    virtual uint64_t VisitSyscall0() INVALID_OP
    virtual uint64_t VisitSyscall1(uint64_t RDI) INVALID_OP
    virtual uint64_t VisitSyscall2(uint64_t RDI,
                                   uint64_t RSI) INVALID_OP
    virtual uint64_t VisitSyscall3(uint64_t RDI,
                                   uint64_t RSI,
                                   uint64_t RDX) INVALID_OP
    virtual uint64_t VisitSyscall4(uint64_t RDI,
                                   uint64_t RSI,
                                   uint64_t RDX,
                                   uint64_t R10) INVALID_OP
    virtual uint64_t VisitSyscall5(uint64_t RDI,
                                   uint64_t RSI,
                                   uint64_t RDX,
                                   uint64_t R10,
                                   uint64_t  R8) INVALID_OP
    // This one MUST be valid
    // Hard fallback if we couldn't look it up
    virtual uint64_t VisitSyscall6(uint64_t RDI,
                                   uint64_t RSI,
                                   uint64_t RDX,
                                   uint64_t R10,
                                   uint64_t  R8,
                                   uint64_t  R9) = 0;
  private:
    uint32_t ArgsMask{};
    bool ConstantVal{};
  };
#undef INVALID_OP
}
