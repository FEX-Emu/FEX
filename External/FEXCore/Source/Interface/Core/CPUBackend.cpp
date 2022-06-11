#include "Interface/Core/Dispatcher/Dispatcher.h"
#include <FEXCore/Core/CPUBackend.h>

namespace FEXCore {
namespace CPU {

CPUBackend::CPUBackend(FEXCore::Core::InternalThreadState *ThreadState, size_t InitialCodeSize, size_t MaxCodeSize)
    : ThreadState(ThreadState), InitialCodeSize(InitialCodeSize), MaxCodeSize(MaxCodeSize) {}

CPUBackend::~CPUBackend() {
  for (auto CodeBuffer : CodeBuffers) {
    FreeCodeBuffer(CodeBuffer);
  }
  CodeBuffers.clear();
}

auto CPUBackend::GetEmptyCodeBuffer() -> CodeBuffer * {
  if (ThreadState->CurrentFrame->SignalHandlerRefCounter == 0) {
    if (CodeBuffers.empty()) {
      auto NewCodeBuffer = AllocateNewCodeBuffer(InitialCodeSize);
      EmplaceNewCodeBuffer(NewCodeBuffer);
    } else {
      if (CodeBuffers.size() > 1) {
        // If we have more than one code buffer we are tracking then walk them and delete
        // This is a cleanup step
        for (size_t i = 1; i < CodeBuffers.size(); i++) {
          FreeCodeBuffer(CodeBuffers[i]);
        }
        CodeBuffers.resize(1);
      }
      // Set the current code buffer to the initial
      CurrentCodeBuffer = &CodeBuffers[0];

      if (CurrentCodeBuffer->Size != MaxCodeSize) {
        FreeCodeBuffer(*CurrentCodeBuffer);

        // Resize the code buffer and reallocate our code size
        CurrentCodeBuffer->Size *= 1.5;
        CurrentCodeBuffer->Size = std::min(CurrentCodeBuffer->Size, MaxCodeSize);

        *CurrentCodeBuffer = AllocateNewCodeBuffer(CurrentCodeBuffer->Size);
      }
    }
  } else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto NewCodeBuffer = AllocateNewCodeBuffer(InitialCodeSize);
    EmplaceNewCodeBuffer(NewCodeBuffer);
  }

  return CurrentCodeBuffer;
}

auto CPUBackend::AllocateNewCodeBuffer(size_t Size) -> CodeBuffer {
  CodeBuffer Buffer;
  Buffer.Size = Size;
  Buffer.Ptr = static_cast<uint8_t *>(
      FEXCore::Allocator::mmap(nullptr, Buffer.Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  LOGMAN_THROW_A_FMT(!!Buffer.Ptr, "Couldn't allocate code buffer");
  Dispatcher->RegisterCodeBuffer(Buffer.Ptr, Buffer.Size);
  if (ThreadState->CTX->Config.GlobalJITNaming()) {
    ThreadState->CTX->Symbols.RegisterJITSpace(Buffer.Ptr, Buffer.Size);
  }
  return Buffer;
}

void CPUBackend::FreeCodeBuffer(CodeBuffer Buffer) {
  FEXCore::Allocator::munmap(Buffer.Ptr, Buffer.Size);
  Dispatcher->RemoveCodeBuffer(Buffer.Ptr);
}

}
}