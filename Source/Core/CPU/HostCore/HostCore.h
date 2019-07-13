#pragma once

namespace FEX {
struct ThreadState;
}

namespace FEX::CPU {
class CPUBackend;
FEX::CPU::CPUBackend *CreateHostCore(FEX::ThreadState *Thread);
}
