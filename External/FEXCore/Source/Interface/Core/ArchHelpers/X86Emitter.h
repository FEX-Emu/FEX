#pragma once

#include "Interface/Core/CodeBuffer.h"

#define XBYAK64
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {

// Ok, I admit this is a bit of a hack....
// But we don't want XByak to allocate a memory buffer, and passing it any pointer will stop that.
// We will set a memory buffer later
// Also, XByak already uses fake pointers like XByak::AutoGrow to control it's internal allocator
#define XByakNoAllocate reinterpret_cast<void*>(~0llu)

class X86Emitter : public Xbyak::CodeGenerator {
protected:
  X86Emitter() : CodeGenerator(4096, XByakNoAllocate) {}
  X86Emitter(size_t Size) : CodeGenerator(4096, XByakNoAllocate), InternalCodeBuffer(Size) {
    SetCodeBuffer(InternalCodeBuffer);
  }

  void SetCodeBuffer(CodeBuffer &Buffer) {
    setNewBuffer(Buffer.Ptr, Buffer.Size);
    CurrentCodeBuffer = &Buffer;
  }

  CodeBuffer *CurrentCodeBuffer{};

private:
  // Internal codebuffer used when X86Emitter is initialized with a size
  CodeBuffer InternalCodeBuffer{};
};



}