// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/InterruptableConditionVariable.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/vector.h>

#include <shared_mutex>
#include <type_traits>

namespace FEXCore {
class LookupCache;
class CompileService;
struct JITSymbolBuffer;
} // namespace FEXCore

namespace FEXCore::Context {
class Context;
}

namespace FEXCore::CPU {
class CPUBackend;
} // namespace FEXCore::CPU

namespace FEXCore::Frontend {
class Decoder;
}

namespace FEXCore::IR {
class OpDispatchBuilder;
class PassManager;
} // namespace FEXCore::IR

namespace FEXCore::Core {

// Special-purpose replacement for std::unique_ptr to allow InternalThreadState to be standard layout.
// Since a NonMovableUniquePtr is neither copyable nor movable, its only function is to own and release the contained object.
template<typename T>
struct NonMovableUniquePtr {
  NonMovableUniquePtr() noexcept = default;
  NonMovableUniquePtr(const NonMovableUniquePtr&) = delete;
  NonMovableUniquePtr& operator=(const NonMovableUniquePtr& UPtr) = delete;

  NonMovableUniquePtr& operator=(fextl::unique_ptr<T> UPtr) noexcept {
    Ptr = UPtr.release();
    return *this;
  }

  ~NonMovableUniquePtr() {
    fextl::default_delete<T> {}(Ptr);
  }

  T* operator->() const noexcept {
    return Ptr;
  }

  std::add_lvalue_reference_t<T> operator*() const noexcept {
    return *Ptr;
  }

  T* get() const noexcept {
    return Ptr;
  }

  operator bool() const noexcept {
    return Ptr != nullptr;
  }

private:
  T* Ptr = nullptr;
};
static_assert(!std::is_move_constructible_v<NonMovableUniquePtr<int>>);
static_assert(!std::is_move_assignable_v<NonMovableUniquePtr<int>>);

struct InternalThreadState : public FEXCore::Allocator::FEXAllocOperators {
  FEXCore::Core::CpuStateFrame* const CurrentFrame = &BaseFrameState;

  struct {
    std::atomic_bool Running {false};
    std::atomic_bool WaitingToStart {true};
    std::atomic_bool ThreadSleeping {false};
  } RunningEvents;

  FEXCore::Context::Context* CTX;
  std::atomic<SignalEvent> SignalReason {SignalEvent::Nothing};

  NonMovableUniquePtr<FEXCore::Threads::Thread> ExecutionThread;
  bool StartPaused {false};
  InterruptableConditionVariable StartRunning;
  Event ThreadWaiting;

  NonMovableUniquePtr<FEXCore::IR::OpDispatchBuilder> OpDispatcher;

  NonMovableUniquePtr<FEXCore::CPU::CPUBackend> CPUBackend;
  NonMovableUniquePtr<FEXCore::LookupCache> LookupCache;

  NonMovableUniquePtr<FEXCore::Frontend::Decoder> FrontendDecoder;
  NonMovableUniquePtr<FEXCore::IR::PassManager> PassManager;
  NonMovableUniquePtr<JITSymbolBuffer> SymbolBuffer;

  int StatusCode {};
  FEXCore::Context::ExitReason ExitReason {FEXCore::Context::ExitReason::EXIT_WAITING};
  std::shared_ptr<FEXCore::CompileService> CompileService;

  std::shared_mutex ObjectCacheRefCounter {};

  struct DeferredSignalState {
#ifndef _WIN32
    siginfo_t Info;
#endif
    int Signal;
  };

  // Queue of thread local signal frames that have been deferred.
  // Async signals aren't guaranteed to be delivered in any particular order, but FEX treats them as FILO.
  fextl::vector<DeferredSignalState> DeferredSignalFrames;

  ///< Data pointer for exclusive use by the frontend
  void* FrontendPtr;

  // BaseFrameState should always be at the end, directly before the interrupt fault page
  alignas(16) FEXCore::Core::CpuStateFrame BaseFrameState {};

  // Can be reprotected as RO to trigger an interrupt at generated code block entrypoints
  alignas(FEXCore::Utils::FEX_PAGE_SIZE) uint8_t InterruptFaultPage[FEXCore::Utils::FEX_PAGE_SIZE];
};
static_assert(std::is_standard_layout_v<FEXCore::Core::InternalThreadState>);
static_assert(
  (offsetof(FEXCore::Core::InternalThreadState, InterruptFaultPage) - offsetof(FEXCore::Core::InternalThreadState, BaseFrameState)) < 4096,
  "Fault page is outside of immediate range from CPU state");

} // namespace FEXCore::Core
