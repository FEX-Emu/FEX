#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <FEXCore/Utils/Allocator.h>

#define XBYAK64
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {

class X86Dispatcher final : public Dispatcher, public Xbyak::CodeGenerator, public Xbyak::Allocator {
  public:
    X86Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config);

    virtual ~X86Dispatcher() override;

    // Xbyak::Allocator
    Xbyak::uint8 *alloc(size_t size) override { Size = size; return reinterpret_cast<uint8_t*>(FEXCore::Allocator::mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)); }
    void free(Xbyak::uint8 *p) override { FEXCore::Allocator::munmap(p, Size); }
    bool useProtect() const override { return false; }

  private:
    size_t Size{};
};

}
