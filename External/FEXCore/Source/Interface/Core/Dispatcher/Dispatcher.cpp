
#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/UContext.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <signal.h>

namespace FEXCore::CPU {

void Dispatcher::SleepThread(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::CpuStateFrame *Frame) {
  auto Thread = Frame->Thread;

  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  Thread->RunningEvents.ThreadSleeping = true;

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  Thread->RunningEvents.ThreadSleeping = false;

  ctx->IdleWaitCV.notify_all();
}

uint64_t Dispatcher::GetCompileBlockPtr() {
  using ClassPtrType = void (FEXCore::Context::ContextImpl::*)(FEXCore::Core::CpuStateFrame *, uint64_t);
  union PtrCast {
    ClassPtrType ClassPtr;
    uintptr_t Data;
  };

  PtrCast CompileBlockPtr;
  CompileBlockPtr.ClassPtr = &FEXCore::Context::ContextImpl::CompileBlockJit;
  return CompileBlockPtr.Data;
}

}
