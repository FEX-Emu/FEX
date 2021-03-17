#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/ArchHelpers/StateReg.h"
#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Core/X86Enums.h>
#include <cmath>

#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;

static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;
#define STATE vixl::aarch64::Register::GetXRegFromCode(STATE_arm64)

Arm64Dispatcher::Arm64Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config)
  : Dispatcher(ctx, Thread), Arm64Emitter(MAX_DISPATCHER_CODE_SIZE) {
  SRAEnabled = config.StaticRegisterAssignment;
  SetAllowAssembler(true);

  auto Buffer = GetBuffer();
  DispatchPtr = Buffer->GetOffsetAddress<CPUBackend::AsmDispatch>(GetCursorOffset());

  // while (true) {
  //    Ptr = FindBlock(RIP)
  //    if (!Ptr)
  //      Ptr = CTX->CompileBlock(RIP);
  //
  //    Ptr();
  // }

  uint64_t VirtualMemorySize = Thread->LookupCache->GetVirtualMemorySize();
  Literal l_VirtualMemory {VirtualMemorySize};
  Literal l_PagePtr {Thread->LookupCache->GetPagePointer()};
  Literal l_L1Ptr {Thread->LookupCache->GetL1Pointer()};
  Literal l_CTX {reinterpret_cast<uintptr_t>(CTX)};
  Literal l_Sleep {reinterpret_cast<uint64_t>(SleepThread)};
  Literal l_CompileBlock {GetCompileBlockPtr()};
  Literal l_ExitFunctionLink {config.ExitFunctionLink};
  Literal l_ExitFunctionLinkThis {config.ExitFunctionLinkThis};

  // Push all the register we need to save
  PushCalleeSavedRegisters();

  // Push our memory base to the correct register
  // Move our thread pointer to the correct register
  // This is passed in to parameter 0 (x0)
  mov(STATE, x0);

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  add(x0, sp, 0);
  str(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)));

  AbsoluteLoopTopAddressFillSRA = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

  if (SRAEnabled) {
    FillStaticRegs();
  }

  // We want to ensure that we are 16 byte aligned at the top of this loop
  Align16B();
  aarch64::Label FullLookup{};
  aarch64::Label LoopTop{};
  aarch64::Label ExitSpillSRA{};
  aarch64::Label ThreadPauseHandler{};

  bind(&LoopTop);
  AbsoluteLoopTopAddress = GetLabelAddress<uint64_t>(&LoopTop);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip)));
  auto RipReg = x2;

  if (!config.ExecuteBlocksWithCall) {
    // L1 Cache
    ldr(x0, &l_L1Ptr);

    and_(x3, RipReg, LookupCache::L1_ENTRIES_MASK);
    add(x0, x0, Operand(x3, Shift::LSL, 4));
    ldp(x1, x0, MemOperand(x0));
    cmp(x0, RipReg);
    b(&FullLookup, Condition::ne);
    br(x1);
  }

  // L1C check failed, do a full lookup
  bind(&FullLookup);

  // This is the block cache lookup routine
  // It matches what is going on it LookupCache.h::FindBlock
  ldr(x0, &l_PagePtr);

  // Mask the address by the virtual address size so we can check for aliases
  if (__builtin_popcountl(VirtualMemorySize) == 1) {
    and_(x3, RipReg, Thread->LookupCache->GetVirtualMemorySize() - 1);
  }
  else {
    ldr(x3, &l_VirtualMemory);
    and_(x3, RipReg, x3);
  }

  aarch64::Label NoBlock;
  {
    // Offset the address and add to our page pointer
    lsr(x1, x3, 12);

    // Load the pointer from the offset
    ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));

    // If page pointer is zero then we have no block
    cbz(x0, &NoBlock);

    // Steal the page offset
    and_(x1, x3, 0x0FFF);

    // Shift the offset by the size of the block cache entry
    add(x0, x0, Operand(x1, Shift::LSL, (int)log2(sizeof(FEXCore::LookupCache::LookupCacheEntry))));

    // Load the guest address first to ensure it maps to the address we are currently at
    // This fixes aliasing problems
    ldr(x1, MemOperand(x0, offsetof(FEXCore::LookupCache::LookupCacheEntry, GuestCode)));
    cmp(x1, RipReg);
    b(&NoBlock, Condition::ne);

    // Now load the actual host block to execute if we can
    ldr(x3, MemOperand(x0, offsetof(FEXCore::LookupCache::LookupCacheEntry, HostCode)));
    cbz(x3, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      // Jump to the block
      if (!config.ExecuteBlocksWithCall) {
        // update L1 cache
        ldr(x0, &l_L1Ptr);

        and_(x1, RipReg, LookupCache::L1_ENTRIES_MASK);
        add(x0, x0, Operand(x1, Shift::LSL, 4));
        stp(x3, x2, MemOperand(x0));

        br(x3);
      } else {
        mov(x0, STATE);
        blr(x3);
      }
    }

    if (config.ExecuteBlocksWithCall) {
      // Interpreter continues execution here
      if (CTX->GetGdbServerStatus()) {
        // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
        // This happens when single stepping

        static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
        ldr(x0, &l_CTX);
        ldr(w0, MemOperand(x0, offsetof(FEXCore::Context::Context, Config.RunningMode)));
        // If the value == 0 then branch to the top
        cbz(x0, &LoopTop);
        // Else we need to pause now
        b(&ThreadPauseHandler);
      }
      else {
        // Unconditionally loop to the top
        // We will only stop on error when compiling a block or signal
        b(&LoopTop);
      }
    }
  }

  {
    bind(&ExitSpillSRA);
    ThreadStopHandlerAddressSpillSRA = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    if (SRAEnabled)
      SpillStaticRegs();

    ThreadStopHandlerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

    PopCalleeSavedRegisters();

    // Return from the function
    // LR is set to the correct return location now
    ret();
  }

  {
    ExitFunctionLinkerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    if (SRAEnabled)
      SpillStaticRegs();

    ldr(x0, &l_ExitFunctionLinkThis);
    mov(x1, STATE);
    mov(x2, lr);

    ldr(x3, &l_ExitFunctionLink);
    blr(x3);

    if (SRAEnabled)
      FillStaticRegs();
    br(x0);
  }

  // Need to create the block
  {
    bind(&NoBlock);

    ldr(x0, &l_CTX);
    mov(x1, STATE);
    ldr(x3, &l_CompileBlock);

    if (SRAEnabled)
      SpillStaticRegs();

    // X2 contains our guest RIP
    blr(x3); // { CTX, Frame, RIP}

    if (SRAEnabled)
      FillStaticRegs();

    b(&LoopTop);
  }

  {
    SignalHandlerReturnAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    hlt(0);
  }

  {
    ThreadPauseHandlerAddressSpillSRA = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    if (SRAEnabled)
      SpillStaticRegs();

    bind(&ThreadPauseHandler);
    ThreadPauseHandlerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    // We are pausing, this means the frontend should be waiting for this thread to idle
    // We will have faulted and jumped to this location at this point

    // Call our sleep handler
    ldr(x0, &l_CTX);
    mov(x1, STATE);
    ldr(x2, &l_Sleep);
    blr(x2);

    PauseReturnInstruction = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    // Fault to start running again
    hlt(0);
  }

  {
    // The expectation here is that a thunked function needs to call back in to the JIT in a reentrant safe way
    // To do this safely we need to do some state tracking and register saving
    //
    // eg:
    // JIT Call->
    //  Thunk->
    //    Thunk callback->
    //
    // The thunk callback needs to execute JIT code and when it returns, it needs to safely return to the thunk rather than JIT space
    // This is handled by pushing a return address trampoline to the stack so when the guest address returns it hits our custom thunk return
    //  - This will safely return us to the thunk
    //
    // On return to the thunk, the thunk can get whatever its return value is from the thread context depending on ABI handling on its end
    // When the thunk itself returns, it'll do its regular return logic there
    // void ReentrantCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    CallbackPtr = Buffer->GetOffsetAddress<CPUBackend::JITCallback>(GetCursorOffset());

    // We expect the thunk to have previously pushed the registers it was using
    PushCalleeSavedRegisters();

    // First thing we need to move the thread state pointer back in to our register
    mov(STATE, x0);

    // Make sure to adjust the refcounter so we don't clear the cache now
    LoadConstant(x0, reinterpret_cast<uint64_t>(&SignalHandlerRefCounter));
    ldr(w2, MemOperand(x0));
    add(w2, w2, 1);
    str(w2, MemOperand(x0));

    // Now push the callback return trampoline to the guest stack
    // Guest will be misaligned because calling a thunk won't correct the guest's stack once we call the callback from the host
    LoadConstant(x0, CTX->X86CodeGen.CallbackReturn);

    ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])));
    sub(x2, x2, 16);
    str(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])));

    // Store the trampoline to the guest stack
    // Guest stack is now correctly misaligned after a regular call instruction
    str(x0, MemOperand(x2));

    // Store RIP to the context state
    str(x1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip)));

    // load static regs
    if (SRAEnabled)
      FillStaticRegs();

    // Now go back to the regular dispatcher loop
    b(&LoopTop);
  }

  place(&l_VirtualMemory);
  place(&l_PagePtr);
  place(&l_L1Ptr);
  place(&l_CTX);
  place(&l_Sleep);
  place(&l_CompileBlock);
  place(&l_ExitFunctionLink);
  place(&l_ExitFunctionLinkThis);


  FinalizeCode();
  Start = reinterpret_cast<uint64_t>(DispatchPtr);
  End = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr));
  GetBuffer()->SetExecutable();

#if ENABLE_JITSYMBOLS
  std::string Name = "Dispatch_" + std::to_string(::gettid());
  CTX->Symbols.Register(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr), Name);
#endif
}

void Arm64Dispatcher::SpillSRA(void *ucontext) {
  for(int i = 0; i < SRA64.size(); i++) {
    ThreadState->CurrentFrame->State.gregs[i] = ArchHelpers::Context::GetArmReg(ucontext, SRA64[i].GetCode());
  }
  // TODO: Also recover FPRs, not sure where the neon context is
  // This is usually not needed
  /*
  for(int i = 0; i < SRAFPR.size(); i++) {
      State->State.State.xmm[i][0] = _mcontext.neon[SRAFPR[i].GetCode()];
      State->State.State.xmm[i][0] = _mcontext.neon[SRAFPR[i].GetCode()];
  }
  */
}

#ifdef _M_ARM_64

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  DispatcherConfig config;
  config.ExecuteBlocksWithCall = true;

  Dispatcher = new Arm64Dispatcher(ctx, Thread, config);
  DispatchPtr = Dispatcher->DispatchPtr;
  CallbackPtr = Dispatcher->CallbackPtr;

  // TODO: It feels wrong to initialize this way
  ctx->InterpreterCallbackReturn = Dispatcher->ReturnPtr;
}

#endif

}
