#include "Interface/Core/LookupCache.h"

#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/RISCVDispatcher.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"

#include "Interface/Context/Context.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stddef.h>

#include <biscuit/assembler.hpp>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::CPU {
#ifdef _M_RISCV_64
using namespace biscuit;
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

RISCVDispatcher::RISCVDispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, uint8_t* Buffer)
  : Dispatcher(ctx, Thread), RISCVEmitter(ctx, Buffer, MAX_DISPATCHER_CODE_SIZE) {

  DispatchPtr = reinterpret_cast<CPUBackend::AsmDispatch>(GetCursorPointer());

  // Push all the register we need to save
  PushCalleeSavedRegisters();

  // Push our memory base to the correct register
  // Move our thread pointer to the correct register
  // This is passed in to parameter 0 (x10)
  MV(STATE, a0);
  LoadConstant(FPRSTATE, reinterpret_cast<uint64_t>(&FPRWorkingSpace[0]));

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  SD(sp, offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation), STATE);

  AbsoluteLoopTopAddressFillSRA = reinterpret_cast<uint64_t>(GetCursorPointer());

  // XXX: SRA save

  biscuit::Label LoopTop;
  Label FullLookup{};

  Bind(&LoopTop);
  AbsoluteLoopTopAddress = reinterpret_cast<uint64_t>(GetCursorPointer());

  // Load in our RIP
  // Don't modify t0 since it contains our RIP once the block doesn't exist
  LD(t2, offsetof(FEXCore::Core::CpuStateFrame, State.rip), STATE);
  auto RipReg = t2;

  // L1 Cache
  LD(t0, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.L1Pointer), STATE);

  LoadConstant(t1, LookupCache::L1_ENTRIES_MASK);
  AND(t1, RipReg, t1);
  SLLI64(t1, t1, 4);
  ADD(t0, t0, t1);
  // {HostPC, GuestRIP}
  LD(t1, 8, t0); // Load cached RIP in to t1
  LD(t0, 0, t0); // Load target in to t0
  BNE(t1, RipReg, &FullLookup);

  MV(a0, STATE);
  JALR(x0, 0, t0);

  Bind(&FullLookup);

  // This is the block cache lookup routine
  // It matches what is going on it LookupCache.h::FindBlock
  LoadConstant(t0, Thread->LookupCache->GetPagePointer());

  // Mask the address by the virtual address size so we can check for aliases
  uint64_t VirtualMemorySize = Thread->LookupCache->GetVirtualMemorySize();
  LoadConstant(t1, VirtualMemorySize - 1);
  AND(t1, RipReg, t1);

  Label NoBlock;
  // AArch64: X3 = Masked address, currently in T1
  // AArch64: X2 = RIPReg = raw RIP, Currently in T2
  {
    // Offset the address and add to our page pointer
    SRLI64(t3, t1, 12);
    SLLI64(t3, t3, 3);
    ADD(t0, t0, t3);

    // Load the pointer from the offset
    LD(t0, 0, t0);

    // If page pointer is zero then we have no block
    BEQZ(t0, &NoBlock);

    // Steal the page offset
    ANDI(t3, t1, 0x0FFF);
    SLLI64(t3, t3, (int)log2(sizeof(FEXCore::LookupCache::LookupCacheEntry)));

    // Shift the offset by the size of the block cache entry
    ADD(t0, t0, t3);

    // Load the guest address first to ensure it maps to the address we are currently at
    // This fixes aliasing problems
    LD(t3, offsetof(FEXCore::LookupCache::LookupCacheEntry, GuestCode), t0);
    EBREAK();
    BNE(t3, RipReg, &NoBlock);

    // Now load the actual host block to execute if we can
    LD(t3, offsetof(FEXCore::LookupCache::LookupCacheEntry, HostCode), t0);
    EBREAK();
    BEQZ(t3, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      // XXX:
      EBREAK();
    }
  }

  {
    ThreadStopHandlerAddressSpillSRA = reinterpret_cast<uint64_t>(GetCursorPointer());
    // XXX: SRA
    ThreadStopHandlerAddress = reinterpret_cast<uint64_t>(GetCursorPointer());

    PopCalleeSavedRegisters();

    // Return from the function
    // RA is set to the correct return location now
    RET();
  }

  {
    Bind(&NoBlock);
    // XXX: Spill SRA
    // XXX: Disable signals
    LoadConstant(a0, reinterpret_cast<uintptr_t>(CTX));
    MV(a1, STATE);
    MV(a2, t2);
    LoadConstant(t2, GetCompileBlockPtr());

    JALR(t2);

    // XXX: Enable signals
    // XXX: FILL SRA

    J(&LoopTop);
  }

  {
    SignalHandlerReturnAddress = reinterpret_cast<uint64_t>(GetCursorPointer());
    // XXX:
  }
  {
    // Guest SIGILL handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    UnimplementedInstructionAddress = reinterpret_cast<uint64_t>(GetCursorPointer());
    // XXX:
  }
  {
    // Guest Overflow handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    OverflowExceptionInstructionAddress = reinterpret_cast<uint64_t>(GetCursorPointer());
    // XXX:
  }
  {
    ThreadPauseHandlerAddressSpillSRA = reinterpret_cast<uint64_t>(GetCursorPointer());
    ThreadPauseHandlerAddress = reinterpret_cast<uint64_t>(GetCursorPointer());
    // XXX:
  }
  {
    CallbackPtr = reinterpret_cast<CPUBackend::JITCallback>(GetCursorPointer());
    // XXX:
  }


  EBREAK();
  J(&LoopTop);

  // Setup dispatcher specific pointers that need to be accessed from JIT code
  {
    auto &Pointers = ThreadState->CurrentFrame->Pointers.RISCV;

    Pointers.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Pointers.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Pointers.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddressSpillSRA;
    Pointers.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddressSpillSRA;
    Pointers.ThreadStopHandler = ThreadStopHandlerAddress;
    Pointers.ThreadPauseHandler = ThreadPauseHandlerAddress;
    Pointers.UnimplementedInstructionHandler = UnimplementedInstructionAddress;
    Pointers.OverflowExceptionHandler = OverflowExceptionInstructionAddress;
    Pointers.SignalReturnHandler = SignalHandlerReturnAddress;
    Pointers.L1Pointer = Thread->LookupCache->GetL1Pointer();
  }
}

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  uint8_t *Buffer = (uint8_t*)FEXCore::Allocator::mmap(nullptr, MAX_DISPATCHER_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  Dispatcher = std::make_unique<RISCVDispatcher>(ctx, Thread, Buffer);
  DispatchPtr = Dispatcher->DispatchPtr;
  CallbackPtr = Dispatcher->CallbackPtr;

  // TODO: It feels wrong to initialize this way
  ctx->InterpreterCallbackReturn = Dispatcher->ReturnPtr;
}
#endif
}

