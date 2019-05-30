#pragma once
#include <stdint.h>

namespace SU::VM {
  class VMInstance {
  public:
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
  };
}
