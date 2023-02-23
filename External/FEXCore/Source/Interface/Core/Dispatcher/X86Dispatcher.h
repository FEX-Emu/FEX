#pragma once

#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/unordered_set.h>

#include "Interface/Core/Dispatcher/Dispatcher.h"

#define XBYAK64
#define XBYAK_CUSTOM_ALLOC
#define XBYAK_CUSTOM_MALLOC FEXCore::Allocator::malloc
#define XBYAK_CUSTOM_FREE FEXCore::Allocator::free
#define XBYAK_CUSTOM_SETS
#define XBYAK_STD_UNORDERED_SET fextl::unordered_set
#define XBYAK_STD_UNORDERED_MAP fextl::unordered_map
#define XBYAK_STD_UNORDERED_MULTIMAP fextl::unordered_multimap
#define XBYAK_STD_LIST fextl::list
#define XBYAK_NO_EXCEPTION

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {

class X86Dispatcher final : public Dispatcher, public Xbyak::CodeGenerator {
  public:
    X86Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &config);
    void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) override;
    size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) override;
    size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) override;

    virtual ~X86Dispatcher() override;

    FEXCore::Context::Context::JITRegionPairs GetDispatcherRegion() const override;
};

}
