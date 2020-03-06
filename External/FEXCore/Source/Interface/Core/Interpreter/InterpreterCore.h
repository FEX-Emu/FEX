#pragma once

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::CPU {
class CPUBackend;

FEXCore::CPU::CPUBackend *CreateInterpreterCore(FEXCore::Context::Context *ctx);

}
