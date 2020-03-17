#include <SonicUtils/VM.h>
#include <stdint.h>

namespace SU::VM {

  namespace KVM {
    class NoopInstance final : public VMInstance {
      public:
        void SetPhysicalMemorySize(uint64_t Size) override {}
        void *GetPhysicalMemoryPointer() override { return nullptr; }

        void AddMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) override {}

        void SetVMRIP(uint64_t VirtualAddress) override {}
        bool Initialize() override { return false; }
        bool SetStepping(bool Step) override { return true; }
        bool Run() override { return false; }
        int ExitReason() override { return -1; }
        uint64_t GetFailEntryReason() override { return ~0U; }
        VMInstance::RegisterState GetRegisterState() override { return {}; }
        VMInstance::SpecialRegisterState GetSpecialRegisterState() override { return {}; }

        void SetRegisterState(VMInstance::RegisterState &State) override {}
        void SetSpecialRegisterState(VMInstance::SpecialRegisterState &State) override {}
        void Debug() override {}

    };
  }

  VMInstance* VMInstance::Create() {
    return new KVM::NoopInstance{};
  }
}
