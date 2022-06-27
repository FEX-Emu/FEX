#include "Interface/Core/LookupCache.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"

#include "Interface/Core/Dispatcher/X86Dispatcher.h"

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/X86HelperGen.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <cmath>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <xbyak/xbyak.h>

#define STATE_PTR(STATE_TYPE, FIELD) \
  [STATE + offsetof(FEXCore::Core::STATE_TYPE, FIELD)]

namespace FEXCore::CPU {
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

/*
 * Global register that holds a pointer to the emulator
 * administration of the currently emulated x86 thread.
 * The code below assumes that this is a callee saved
 * register, and does not save and restore it around
 * calls to C++ functions.
 * NOTE: This definition does not belong in this source.
 */
#define STATE r14
#define STATE_STR  "r14"

static constexpr bool SignalSafeCompile = true;

/* ---------------------------------------------------------------------------------- */

/*
 * These functions are called directly from inline asm
 * so declare them "C" and non-static to prevent the compiler 
 * from giving them mangled or otherwise unrecognizable assembly
 * names.
 */
extern "C" {
    __attribute__((used)) uint64_t X86CoreDispatchCode( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) uint64_t X86ExitFunctionLinkerCode( FEXCore::Core::CpuStateFrame *Frame, uint64_t *record );
    __attribute__((used)) void X86ThreadStopHandlerCode( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) void X86IntCallbackReturnCode( FEXCore::Core::CpuStateFrame *Frame );
}


/* ---------------------------------------------------------------------------------- */

__attribute__((naked))
__attribute__((noreturn))
static
void X86SignalHandlerReturnAddressCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    asm("ud2");
}

__attribute__((naked))
__attribute__((noreturn))
static
void X86UnimplementedInstructionAddressCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    // Guest SIGILL handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    asm("ud2");
}

// ---

__attribute__((naked))
__attribute__((noreturn))
static
void X86PauseReturnInstructionCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("ud2");
}

__attribute__((naked))
__attribute__((noreturn))
static
void X86IntCallbackReturnCodeAsm( FEXCore::Core::CpuStateFrame *Frame )
{
    asm("sub $8,%rsp");  // Misalign SP
    asm("mov %" STATE_STR ",%rdi");
    asm("jmp X86IntCallbackReturnCode");
}

// ---

__attribute__((naked))
__attribute__((noreturn))
static
void X86ThreadStopHandlerCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("sub $8,%rsp");  // Misalign SP
    asm("mov %" STATE_STR ",%rdi");
    asm("jmp X86ThreadStopHandlerCode");
}


/*
 * Jump back to start of X86CoreDispatchCode from within JIT code.
 * This sequence can be considered a *very* specialized longjmp:
 * the only casaved regs are the first function parameter register, 
 * which is hereby refilled, and the sp, which is 'sufficiently' restored
 * for a noreturn function like X86CoreDispatchCode. Also, cesaved 
 * regs are irrelevant for noreturn functions, so no need for any restore.
 */
__attribute__((naked))
__attribute__((noreturn))
static inline
void X86CoreDispatchCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("mov %" STATE_STR ",%rdi");
    asm("callq X86CoreDispatchCode");
    asm("jmpq *%rax");
}

__attribute__((naked))
__attribute__((noreturn))
static inline
void X86CallCoreDispatchCodeAsm( FEXCore::Core::CpuStateFrame *Frame )
{
    asm("push %rdi;");                 // Align SP
    asm("callq X86CoreDispatchCode");
    asm("movq (%rsp), %" STATE_STR);   // Keep SP aligned
    asm("jmpq *%rax");
}


/*
 * JIT to C++ calling interface. Copy STATE register
 * into the FillMe function parameter register,
 * and forward to specified code. 
 *
 * JIT code ensures that the following functions
 * are entered with an aligned SP. Because
 * the C++ functions that they forward to expect
 * them misaligned this is corrected here.
 * 
 * Very likely the repetion below can be defined 
 * in a bit more compact way in C++:
 */
__attribute__((naked))
__attribute__((noreturn))
static
void X86ExitFunctionLinkerCodeAsm( FEXCore::Core::CpuStateFrame *FillMe, uint64_t *record )
{
    asm("mov %" STATE_STR ",%rdi");
    asm("callq X86ExitFunctionLinkerCode");
    asm("jmpq *%rax");
}

/* ---------------------------------------------------------------------------------- */

/*
 * Find host JIT entry for a guest RIP, and continue 
 * with executing that code. First check the caches,
 * and ultimately call the JIT compiler:
 */
//static
uint64_t X86CoreDispatchCode( FEXCore::Core::CpuStateFrame *Frame )
{ 
  auto  Thread  = Frame->Thread;
  auto  CTX     = Thread->CTX;
  auto  Address = Frame->State.rip;
  auto &L1Entry = reinterpret_cast<LookupCache::LookupCacheEntry*>(Frame->Pointers.Common.L1Pointer)[Address & LookupCache::L1_ENTRIES_MASK];

  if (L1Entry.GuestCode != Address) {
    uintptr_t  HostCode= Thread->LookupCache->FindBlock(Address);
  
    if ( !HostCode ) {
      // When compiling code, mask all signals to reduce the chance of reentrant allocations
      sigset_t ProcMask={(unsigned long)-1,(unsigned long)-1};
      if (SignalSafeCompile) {sigprocmask( SIG_SETMASK, &ProcMask, &ProcMask); }
      CTX->CompileBlockJit(Thread->CurrentFrame,Address);
      if (SignalSafeCompile) {sigprocmask( SIG_SETMASK, &ProcMask, NULL); }

      HostCode= Thread->LookupCache->FindBlock(Address);
    }

    L1Entry.HostCode  = HostCode;
    L1Entry.GuestCode = Address;
  }

  return L1Entry.HostCode;
}

/* ---------------------------------------------------------------------------------- */

/*
 * Call Core::ExitFunctionLink to obtain a continuation
 * address (which can be either LoopTop or a JIT entry), 
 * and continue with executing that code. 
 * TODO: Core::ExitFunctionLink has a potential 
 * short return path, for which we may want to omit the 
 * two calls to sigprocmask:
 */
//static
uint64_t X86ExitFunctionLinkerCode( FEXCore::Core::CpuStateFrame *Frame, uint64_t *record )
{ 
  uint64_t HostCode;

  if (!SignalSafeCompile) {
    // Just compile the code
    HostCode = X86JITCore::ExitFunctionLink(Frame,record);
  } else {
    // When compiling code, mask all signals to reduce the chance of reentrant allocations
    sigset_t ProcMask={(unsigned long)-1,(unsigned long)-1};
    sigprocmask( SIG_SETMASK, &ProcMask, &ProcMask);
    HostCode = X86JITCore::ExitFunctionLink(Frame,record);
    sigprocmask( SIG_SETMASK, &ProcMask, NULL);
  }
  
  return HostCode;
}

/* ---------------------------------------------------------------------------------- */

/*
 * Call the dispatcher animation framework defined above, 
 * while providing a quick termination via a longjmp:
 */
static 
void X86DispatchCode( FEXCore::Core::CpuStateFrame *Frame )
{
  if (!setjmp(Frame->EmuContext)) {
      uint32_t aligner MIE_ALIGN(16);
      Frame->ReturningStackLocation = reinterpret_cast<uintptr_t>(&aligner);

      X86CallCoreDispatchCodeAsm(Frame);
  }
}

__attribute__((noreturn))
//static 
void X86ThreadStopHandlerCode( FEXCore::Core::CpuStateFrame *Frame )
{
  longjmp(Frame->EmuContext,1);
}

/* ---------------------------------------------------------------------------------- */

    __attribute__((noreturn))
    static void WaitUntilWeHitATestCase(){assert(0);}



static 
void X86ThreadPauseHandlerAddressCode( FEXCore::Core::CpuStateFrame *Frame )
#if 1
{WaitUntilWeHitATestCase();}
#else
{
  // Pause handler
  SleepThread(Frame->Thread->CTX,Frame);
  assert(false);
}
#endif
  
  
static 
void X86OverflowExceptionInstructionAddressCode( FEXCore::Core::CpuStateFrame *Frame )
#if 1
// Does not seem to be used yet???
{WaitUntilWeHitATestCase();}
#else
#endif
 /*
    // ud2 = SIGILL
    // int3 = SIGTRAP
    // hlt = SIGSEGV

    mov(rax, reinterpret_cast<uint64_t>(&SynchronousFaultData));
    add(byte [rax + offsetof(Dispatcher::SynchronousFaultDataStruct, FaultToTopAndGeneratedException)], 1);
    mov(dword [rax + offsetof(Dispatcher::SynchronousFaultDataStruct, TrapNo)], X86State::X86_TRAPNO_OF);
    mov(dword [rax + offsetof(Dispatcher::SynchronousFaultDataStruct, err_code)], 0);
    mov(dword [rax + offsetof(Dispatcher::SynchronousFaultDataStruct, si_code)], 0x80);

    hlt();
  */
  
  
static 
void X86CallbackPtrCode( FEXCore::Core::CpuStateFrame *Frame, uint64_t RSI )
#if 1
{WaitUntilWeHitATestCase();}
#else
{
  if (!setjmp(Frame->CallbackContext)) {
    // Make sure to adjust the refcounter so we don't clear the cache now
    Frame->Pointers.Common.SignalHandlerRefCountPointer++);

    // Now push the callback return trampoline to the guest stack
    // Guest will be misaligned because calling a thunk won't correct the guest's stack once we call the callback from the host
    mov(rax, CTX->X86CodeGen.CallbackReturn);

    // Store the trampoline to the guest stack
    // Guest stack is now correctly misaligned after a regular call instruction
    Frame->State.gregs[X86State::REG_RSP] -= 16;
    *reinterpret_cast<FEXCore::Context::Context::IntCallbackReturn*>(Frame->State.gregs[X86State::REG_RSP]) = 
        reinterpret_cast<FEXCore::Context::Context::IntCallbackReturn>(X86IntCallbackReturnCodeAsm);

    // Store RIP to the context state
    Frame->State.rip = RSI;

    // Back to the loop top now
    X86CoreDispatchCode(Frame);
  }
}
#endif
  
  
//static 
void X86IntCallbackReturnCode( FEXCore::Core::CpuStateFrame *Frame )
#if 1
{WaitUntilWeHitATestCase();}
#else
{
  longjmp(Frame->CallbackContext,1);
}
#endif  
  
  
/* ---------------------------------------------------------------------------------- */

X86Dispatcher::X86Dispatcher(FEXCore::Context::Context *ctx, const DispatcherConfig &config)
  : Dispatcher(ctx)
  , Xbyak::CodeGenerator(MAX_DISPATCHER_CODE_SIZE,
      FEXCore::Allocator::mmap(nullptr, MAX_DISPATCHER_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
      nullptr) {

  LOGMAN_THROW_A_FMT(!config.StaticRegisterAllocation, "X86 dispatcher does not support SRA");

  DispatchPtr                         = reinterpret_cast<AsmDispatch>(X86DispatchCode);
  AbsoluteLoopTopAddressFillSRA       = reinterpret_cast<uint64_t>(X86CoreDispatchCodeAsm);
  AbsoluteLoopTopAddress              = reinterpret_cast<uint64_t>(X86CoreDispatchCodeAsm);
  ExitFunctionLinkerAddress           = reinterpret_cast<uint64_t>(X86ExitFunctionLinkerCodeAsm);
  ThreadStopHandlerAddress            = reinterpret_cast<uint64_t>(X86ThreadStopHandlerCodeAsm);
  SignalHandlerReturnAddress          = reinterpret_cast<uint64_t>(X86SignalHandlerReturnAddressCodeAsm);
  PauseReturnInstruction              = reinterpret_cast<uint64_t>(X86PauseReturnInstructionCodeAsm);
  UnimplementedInstructionAddress     = reinterpret_cast<uint64_t>(X86UnimplementedInstructionAddressCodeAsm);

  ThreadPauseHandlerAddress           = reinterpret_cast<uint64_t>(X86ThreadPauseHandlerAddressCode);
  OverflowExceptionInstructionAddress = reinterpret_cast<uint64_t>(X86OverflowExceptionInstructionAddressCode);
  CallbackPtr                         = reinterpret_cast<JITCallback>(X86CallbackPtrCode);
  IntCallbackReturnAddress            = reinterpret_cast<uint64_t>(X86IntCallbackReturnCodeAsm);
}

// Used by GenerateGDBPauseCheck, GenerateInterpreterTrampoline
static thread_local Xbyak::CodeGenerator emit(1, &emit); // actual emit target set with setNewBuffer

size_t X86Dispatcher::GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) {
  using namespace Xbyak;
  using namespace Xbyak::util;

  emit.setNewBuffer(CodeBuffer, MaxGDBPauseCheckSize);
  
  Label RunBlock;

  // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
  // This happens when single stepping
  static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
  emit.mov(rax, reinterpret_cast<uint64_t>(CTX));

  // If the value == 0 then we don't need to stop
  emit.cmp(dword [rax + (offsetof(FEXCore::Context::Context, Config.RunningMode))], 0);
  emit.je(RunBlock);
  {
    // Make sure RIP is syncronized to the context
    emit.mov(rax, GuestRIP);
    emit.mov(qword STATE_PTR(CpuStateFrame, State.rip), rax);

    // Stop the thread
    emit.mov(rax, qword STATE_PTR(CpuStateFrame, Pointers.Common.ThreadPauseHandlerSpillSRA));
    emit.jmp(rax);
  }

  emit.L(RunBlock);

  emit.ready();

  return emit.getSize();
}


size_t X86Dispatcher::GenerateInterpreterTrampoline(uint8_t *CodeBuffer) {
  using namespace Xbyak;
  using namespace Xbyak::util;

  emit.setNewBuffer(CodeBuffer, MaxInterpreterTrampolineSize);

  Label InlineIRData;
  
  emit.mov(rdi, STATE);
  emit.lea(rsi, ptr[rip + InlineIRData]);
  emit.call(qword STATE_PTR(CpuStateFrame, Pointers.Interpreter.FragmentExecuter));

  emit.jmp(qword STATE_PTR(CpuStateFrame, Pointers.Common.DispatcherLoopTop));

  emit.L(InlineIRData);

  emit.ready();

  return emit.getSize();
}

X86Dispatcher::~X86Dispatcher() {
  FEXCore::Allocator::munmap(top_, MAX_DISPATCHER_CODE_SIZE);
}

void X86Dispatcher::InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) {
  // Setup dispatcher specific pointers that need to be accessed from JIT code
  {
    auto &Common = Thread->CurrentFrame->Pointers.Common;

    Common.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Common.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Common.ExitFunctionLinker = ExitFunctionLinkerAddress;
    Common.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddress;
    Common.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddress;
    Common.UnimplementedInstructionHandler = UnimplementedInstructionAddress;
    Common.OverflowExceptionHandler = OverflowExceptionInstructionAddress;
    Common.SignalReturnHandler = SignalHandlerReturnAddress;

    auto &Interpreter = Thread->CurrentFrame->Pointers.Interpreter;
    (uintptr_t&)Interpreter.CallbackReturn = IntCallbackReturnAddress;
  }
}

std::unique_ptr<Dispatcher> Dispatcher::CreateX86(FEXCore::Context::Context *CTX, const DispatcherConfig &Config) {
  return std::make_unique<X86Dispatcher>(CTX, Config);
}

}
