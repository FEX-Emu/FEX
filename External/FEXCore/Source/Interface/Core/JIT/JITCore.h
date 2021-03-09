#pragma once

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;

FEXCore::CPU::CPUBackend *CreateX86JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread);
FEXCore::CPU::CPUBackend *CreateArm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread);
}
