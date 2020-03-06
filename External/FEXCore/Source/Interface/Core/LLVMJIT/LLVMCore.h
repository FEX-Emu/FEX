#pragma once

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;

FEXCore::CPU::CPUBackend *CreateLLVMCore(FEXCore::Core::InternalThreadState *Thread);
}
