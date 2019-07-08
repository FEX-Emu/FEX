#pragma once
#include <stdint.h>

namespace SU::VM {
  class VMInstance {
  public:
    struct RegisterState {
      uint64_t rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp;
      uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
      uint64_t rip, rflags;
    };

    struct SpecialRegisterState {
      struct {
        uint64_t base;
      } fs, gs;
    };

    static VMInstance* Create();
    virtual ~VMInstance() {}

    virtual void SetPhysicalMemorySize(uint64_t Size) = 0;
    virtual void AddMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) = 0;
    virtual void *GetPhysicalMemoryPointer() = 0;
    virtual void SetVMRIP(uint64_t VirtualAddress) = 0;
    virtual bool SetStepping(bool Step) = 0;
    virtual bool Run() = 0;
    virtual bool Initialize() = 0;
    virtual int ExitReason() = 0;
    virtual uint64_t GetFailEntryReason() = 0;
    virtual RegisterState GetRegisterState() = 0;
    virtual SpecialRegisterState GetSpecialRegisterState() = 0;

    virtual void SetRegisterState(RegisterState &State) = 0;
    virtual void SetSpecialRegisterState(SpecialRegisterState &State) = 0;

    virtual void Debug() = 0;

  };
}
