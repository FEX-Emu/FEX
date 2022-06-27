#include "Interface/Core/LookupCache.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"

#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <aarch64/assembler-aarch64.h>
#include <aarch64/constants-aarch64.h>
#include <aarch64/cpu-aarch64.h>
#include <aarch64/operands-aarch64.h>
#include <code-buffer-vixl.h>
#include <platform-vixl.h>
#include <xbyak/xbyak.h>

#include <sys/syscall.h>
#include <unistd.h>

#define STATE_PTR(STATE_TYPE, FIELD) \
  MemOperand(STATE, offsetof(FEXCore::Core::STATE_TYPE, FIELD))

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;

static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

/*
 * Global register that holds a pointer to the emulator
 * administration of the currently emulated x86 thread.
 * The code below assumes that this is a callee saved
 * register, and does not save and restore it around
 * calls to C++ functions.
 * NOTE: These definitions do not belong in this source.
 */
#define STATE       x28 
#define STATE_STR  "x28"

static constexpr bool SignalSafeCompile = true;

/* ---------------------------------------------------------------------------------- */

/*
 * These functions are called directly from inline asm
 * so declare them "C" and non-static to prevent the compiler 
 * from giving them mangled or otherwise unrecognizable assembly
 * names.
 */
extern "C" {
    __attribute__((used)) void     Arm64PopAllDynamicRegs            ( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) void     Arm64PushAllDynamicRegs           ( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) void     Arm64SpillAllStaticRegs           ( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) void     Arm64FillAllStaticRegs            ( FEXCore::Core::CpuStateFrame *Frame );

    __attribute__((used)) uint64_t Arm64CoreDispatchCode             ( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) uint64_t Arm64ExitFunctionLinkerCode       ( FEXCore::Core::CpuStateFrame *Frame, uint64_t *record );
    __attribute__((used)) void     Arm64ThreadStopHandlerCode        ( FEXCore::Core::CpuStateFrame *Frame );
    __attribute__((used)) void     Arm64ThreadPauseHandlerAddressCode( FEXCore::Core::CpuStateFrame *Frame );
}

/* ---------------------------------------------------------------------------------- */

/*
 * Register save/restore functions. Their definitions do not 
 * belong here, but are related to the dynamic functions in
 * Arm64Emitter.h. The only practical and clean way to deal
 * with them is to generate them during the build, but that
 * is not the purpose of this change.
 */
__attribute__((naked))
//static
void Arm64SpillAllStaticRegs( FEXCore::Core::CpuStateFrame *Frame )
{
    __asm__ __volatile__( "stp x4, x5,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 0] )));
    __asm__ __volatile__( "stp x6, x7,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 2] )));
    __asm__ __volatile__( "stp x8, x9,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 4] )));
    __asm__ __volatile__( "stp x10, x11, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 6] )));
    __asm__ __volatile__( "stp x12, x18, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 8] )));
    __asm__ __volatile__( "stp x17, x16, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[10] )));
    __asm__ __volatile__( "stp x15, x14, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[12] )));
    __asm__ __volatile__( "stp x13, x29, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[14] )));
    
    __asm__ __volatile__( "stp q16, q17, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 0][0] )));
    __asm__ __volatile__( "stp q18, q19, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 2][0] )));
    __asm__ __volatile__( "stp q20, q21, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 4][0] )));
    __asm__ __volatile__( "stp q22, q23, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 6][0] )));
    __asm__ __volatile__( "stp q24, q25, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 8][0] )));
    __asm__ __volatile__( "stp q26, q27, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[10][0] )));
    __asm__ __volatile__( "stp q28, q29, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[12][0] )));
    __asm__ __volatile__( "stp q30, q31, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[14][0] )));
    asm("ret");
}


__attribute__((naked))
//static
void Arm64FillAllStaticRegs( FEXCore::Core::CpuStateFrame *Frame )
{
    __asm__ __volatile__( "ldp x4, x5,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 0] )));
    __asm__ __volatile__( "ldp x6, x7,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 2] )));
    __asm__ __volatile__( "ldp x8, x9,   [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 4] )));
    __asm__ __volatile__( "ldp x10, x11, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 6] )));
    __asm__ __volatile__( "ldp x12, x18, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[ 8] )));
    __asm__ __volatile__( "ldp x17, x16, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[10] )));
    __asm__ __volatile__( "ldp x15, x14, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[12] )));
    __asm__ __volatile__( "ldp x13, x29, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.gregs[14] )));
    
    __asm__ __volatile__( "ldp q16, q17, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 0][0] )));
    __asm__ __volatile__( "ldp q18, q19, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 2][0] )));
    __asm__ __volatile__( "ldp q20, q21, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 4][0] )));
    __asm__ __volatile__( "ldp q22, q23, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 6][0] )));
    __asm__ __volatile__( "ldp q24, q25, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[ 8][0] )));
    __asm__ __volatile__( "ldp q26, q27, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[10][0] )));
    __asm__ __volatile__( "ldp q28, q29, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[12][0] )));
    __asm__ __volatile__( "ldp q30, q31, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, State.xmm[14][0] )));
    asm("ret");
}

__attribute__((naked))
//static
void Arm64PushAllDynamicRegs( FEXCore::Core::CpuStateFrame *Frame )
{
    asm("sub	sp,   sp, #0x110");
    asm("str	q4,  [sp]");
    asm("str	q5,  [sp, #16]");
    asm("str	q6,  [sp, #32]");
    asm("str	q7,  [sp, #48]");
    asm("str	q8,  [sp, #64]");
    asm("str	q9,  [sp, #80]");
    asm("str	q10, [sp, #96]");
    asm("str	q11, [sp, #112]");
    asm("str	q12, [sp, #128]");
    asm("str    q13, [sp, #144]");
    asm("str    q14, [sp, #160]");
    asm("str    q15, [sp, #176]");
    asm("ret");
}

__attribute__((naked))
//static
void Arm64PopAllDynamicRegs( FEXCore::Core::CpuStateFrame *Frame )
{
    asm("ldr    q4,  [sp]");
    asm("ldr    q5,  [sp, #16]");
    asm("ldr    q6,  [sp, #32]");
    asm("ldr    q7,  [sp, #48]");
    asm("ldr    q8,  [sp, #64]");
    asm("ldr    q9,  [sp, #80]");
    asm("ldr    q10, [sp, #96]");
    asm("ldr    q11, [sp, #112]");
    asm("ldr    q12, [sp, #128]");
    asm("ldr    q13, [sp, #144]");
    asm("ldr    q14, [sp, #160]");
    asm("ldr    q15, [sp, #176]");
    asm("add    sp, sp, #0x110");
    asm("ret");
}


/* ---------------------------------------------------------------------------------- */

__attribute__((naked))
__attribute__((noreturn))
static
void Arm64SignalHandlerReturnAddressCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    asm("hlt 0");
}

__attribute__((naked))
__attribute__((noreturn))
static
void Arm64UnimplementedInstructionAddressCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    // Guest SIGILL handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    asm("bl Arm64SpillAllStaticRegs" );
    asm("hlt 0");
}

// ---

__attribute__((naked))
__attribute__((noreturn))
static
void Arm64PauseReturnInstructionCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    // Fault to start running again
    asm("hlt 0");
}

// ---

__attribute__((naked))
__attribute__((noreturn))
static
void Arm64ThreadPauseHandlerAddressSpillSRACodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("bl Arm64SpillAllStaticRegs" );
    
    asm("adrp	x3,           Arm64ThreadPauseHandlerAddressCode");
    asm("add	x3, x3, :lo12:Arm64ThreadPauseHandlerAddressCode");
    asm("mov x0, " STATE_STR);
    asm("br x3");
}

// ---

__attribute__((naked))
__attribute__((noreturn))
static
void Arm64ThreadStopHandlerCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("bl Arm64SpillAllStaticRegs" );
    
    asm("adrp	x3,           Arm64ThreadStopHandlerCode");
    asm("add	x3, x3, :lo12:Arm64ThreadStopHandlerCode");
    asm("mov x0, " STATE_STR);
    asm("br x3");
}



/* ---------------------------------------------------------------------------------- */

/*
 * Jump back to start of Arm64CoreDispatchCode from within JIT code.
 * This sequence can be considered a *very* specialized longjmp:
 * the only casaved regs are the first function parameter register, 
 * which is hereby refilled, and the sp, which is 'sufficiently' restored
 * for a noreturn function like Arm64CoreDispatchCode. Also, cesaved 
 * regs are irrelevant for noreturn functions, so no need for any restore.
 */
__attribute__((naked))
__attribute__((noreturn))
static inline
void Arm64CoreDispatchCodeAsmFillSRA( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("mov x0," STATE_STR);
    asm("bl Arm64CoreDispatchCode");
    
    asm("bl Arm64FillAllStaticRegs");
    
    asm("br x0");
}

__attribute__((naked))
__attribute__((noreturn))
static inline
void Arm64CoreDispatchCodeAsm( FEXCore::Core::CpuStateFrame *FillMe )
{
    asm("bl Arm64SpillAllStaticRegs");
    
    asm("mov x0," STATE_STR);
    asm("bl Arm64CoreDispatchCode");
    
    asm("bl Arm64FillAllStaticRegs");
    
    asm("br x0");
}


__attribute__((naked))
__attribute__((noreturn))
static inline
void Arm64FirstCoreDispatchCodeAsm( FEXCore::Core::CpuStateFrame *Frame )
{
    asm("mov " STATE_STR ",x0");
    asm("bl Arm64CoreDispatchCode");
    
    asm("bl Arm64FillAllStaticRegs");
    
    asm("br x0");
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
void Arm64ExitFunctionLinkerCodeAsm( FEXCore::Core::CpuStateFrame *FillMe1, uint64_t *FillMe2 )
{
    asm("mov x0," STATE_STR);
    asm("mov x1, lr");
    asm("bl Arm64SpillAllStaticRegs" );
    asm("bl	Arm64ExitFunctionLinkerCode");
    asm("bl Arm64FillAllStaticRegs" );
    asm("br x0");
}

/* ---------------------------------------------------------------------------------- */

__attribute__((naked)) uint64_t LUDIVAsm(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor)
{
    asm("str lr, [sp, #-16]!");
    asm("bl Arm64SpillAllStaticRegs" );
    asm("bl Arm64PushAllDynamicRegs" );
    
    __asm__ __volatile__( "ldr x3, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUDIV)));
    asm("blr x3" );
    
    asm("bl Arm64PopAllDynamicRegs" );
    asm("bl Arm64FillAllStaticRegs" );
    asm("ldr lr, [sp], #16");
    asm("ret");
}

__attribute__((naked)) uint64_t LUREMAsm(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor)
{
    asm("str lr, [sp, #-16]!");
    asm("bl Arm64SpillAllStaticRegs" );
    asm("bl Arm64PushAllDynamicRegs" );
    
    __asm__ __volatile__( "ldr x3, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUREM)));
    asm("blr x3" );
    
    asm("bl Arm64PopAllDynamicRegs" );
    asm("bl Arm64FillAllStaticRegs" );
    asm("ldr lr, [sp], #16");
    asm("ret");
}

__attribute__((naked))  int64_t  LDIVAsm( int64_t SrcHigh,  int64_t SrcLow,  int64_t Divisor)
{
    asm("str lr, [sp, #-16]!");
    asm("bl Arm64SpillAllStaticRegs" );
    asm("bl Arm64PushAllDynamicRegs" );
    
    __asm__ __volatile__( "ldr x3, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LDIV)));
    asm("blr x3" );
    
    asm("bl Arm64PopAllDynamicRegs" );
    asm("bl Arm64FillAllStaticRegs" );
    asm("ldr lr, [sp], #16");
    asm("ret");
}

__attribute__((naked))  int64_t  LREMAsm( int64_t SrcHigh,  int64_t SrcLow,  int64_t Divisor)
{
    asm("str lr, [sp, #-16]!");
    asm("bl Arm64SpillAllStaticRegs" );
    asm("bl Arm64PushAllDynamicRegs" );
    
    __asm__ __volatile__( "ldr x3, [" STATE_STR ", %[a]]" : : [a]"i"(offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LREM)));
    asm("blr x3" );
    
    asm("bl Arm64PopAllDynamicRegs" );
    asm("bl Arm64FillAllStaticRegs" );
    asm("ldr lr, [sp], #16");
    asm("ret");
}


/* ---------------------------------------------------------------------------------- */

/*
 * Find host JIT entry for a guest RIP, and continue 
 * with executing that code. First check the caches,
 * and ultimately call the JIT compiler:
 */
//static
uint64_t Arm64CoreDispatchCode( FEXCore::Core::CpuStateFrame *Frame )
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
uint64_t Arm64ExitFunctionLinkerCode( FEXCore::Core::CpuStateFrame *Frame, uint64_t *record )
{ 
  uint64_t HostCode;

  if (!SignalSafeCompile) {
    // Just compile the code
    HostCode = Arm64JITCore::ExitFunctionLink(Frame,record);
  } else {
    // When compiling code, mask all signals to reduce the chance of reentrant allocations
    sigset_t ProcMask={(unsigned long)-1,(unsigned long)-1};
    sigprocmask( SIG_SETMASK, &ProcMask, &ProcMask);
    HostCode = Arm64JITCore::ExitFunctionLink(Frame,record);
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
void Arm64DispatchCode( FEXCore::Core::CpuStateFrame *Frame )
{
  if (!setjmp(Frame->EmuContext)) {
      uint32_t aligner MIE_ALIGN(16);
      Frame->ReturningStackLocation = reinterpret_cast<uintptr_t>(&aligner);

      Arm64FirstCoreDispatchCodeAsm(Frame);
  }
}

__attribute__((noreturn))
//static 
void Arm64ThreadStopHandlerCode( FEXCore::Core::CpuStateFrame *Frame )
{
  longjmp(Frame->EmuContext,1);
}

/* ---------------------------------------------------------------------------------- */

    __attribute__((noreturn))
    static void WaitUntilWeHitATestCase(){assert(0);}

//static 
void Arm64ThreadPauseHandlerAddressCode( FEXCore::Core::CpuStateFrame *Frame )
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
void Arm64OverflowExceptionInstructionAddressCode( FEXCore::Core::CpuStateFrame *Frame )
#if 1
// Does not seem to be used yet???
{WaitUntilWeHitATestCase();}
#else
{
    if (SRAEnabled)
      SpillStaticRegs();

    LoadConstant(x0, reinterpret_cast<uint64_t>(&SynchronousFaultData));
    LoadConstant(w1, 1);
    strb(w1, MemOperand(x0, offsetof(Dispatcher::SynchronousFaultDataStruct, FaultToTopAndGeneratedException)));
    LoadConstant(w1, X86State::X86_TRAPNO_OF);
    str(w1, MemOperand(x0, offsetof(Dispatcher::SynchronousFaultDataStruct, TrapNo)));
    LoadConstant(w1, 0x80);
    str(w1, MemOperand(x0, offsetof(Dispatcher::SynchronousFaultDataStruct, si_code)));
    LoadConstant(x1, 0);
    str(w1, MemOperand(x0, offsetof(Dispatcher::SynchronousFaultDataStruct, err_code)));

    // hlt/udf = SIGILL
    // brk = SIGTRAP
    // ??? = SIGSEGV
    // Force a SIGSEGV by loading zero
    ldr(x1, MemOperand(x1));
}
#endif
  
  
static 
void Arm64CallbackPtrCode( FEXCore::Core::CpuStateFrame *Frame, uint64_t RSI )
#if 1
{WaitUntilWeHitATestCase();}
#else
{
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
#endif
  
/* ---------------------------------------------------------------------------------- */

Arm64Dispatcher::Arm64Dispatcher(FEXCore::Context::Context *ctx, const DispatcherConfig &config)
  : FEXCore::CPU::Dispatcher(ctx), Arm64Emitter(ctx, MAX_DISPATCHER_CODE_SIZE)
  , config(config) {
  SetAllowAssembler(true);

  DispatchPtr                         = reinterpret_cast<AsmDispatch>(Arm64DispatchCode);
  AbsoluteLoopTopAddressFillSRA       = reinterpret_cast<uint64_t>(Arm64CoreDispatchCodeAsmFillSRA);
  AbsoluteLoopTopAddress              = reinterpret_cast<uint64_t>(Arm64CoreDispatchCodeAsm);
  ExitFunctionLinkerAddress           = reinterpret_cast<uint64_t>(Arm64ExitFunctionLinkerCodeAsm);
  ThreadStopHandlerAddress            = reinterpret_cast<uint64_t>(Arm64ThreadStopHandlerCodeAsm);
  ThreadStopHandlerAddressSpillSRA    = reinterpret_cast<uint64_t>(Arm64ThreadStopHandlerCodeAsm);
  SignalHandlerReturnAddress          = reinterpret_cast<uint64_t>(Arm64SignalHandlerReturnAddressCodeAsm);
  PauseReturnInstruction              = reinterpret_cast<uint64_t>(Arm64PauseReturnInstructionCodeAsm);
  UnimplementedInstructionAddress     = reinterpret_cast<uint64_t>(Arm64UnimplementedInstructionAddressCodeAsm);

  ThreadPauseHandlerAddress           = reinterpret_cast<uint64_t>(Arm64ThreadPauseHandlerAddressCode);
  ThreadPauseHandlerAddressSpillSRA   = reinterpret_cast<uint64_t>(Arm64ThreadPauseHandlerAddressSpillSRACodeAsm);
  OverflowExceptionInstructionAddress = reinterpret_cast<uint64_t>(Arm64OverflowExceptionInstructionAddressCode);
  CallbackPtr                         = reinterpret_cast<JITCallback>(Arm64CallbackPtrCode);

  // Long division helpers
  LDIVHandlerAddress	   	      = reinterpret_cast<uint64_t>(LDIVAsm);	     
  LREMHandlerAddress	   	      = reinterpret_cast<uint64_t>(LREMAsm);	     
  LUDIVHandlerAddress	   	      = reinterpret_cast<uint64_t>(LUDIVAsm);	     
  LUREMHandlerAddress	   	      = reinterpret_cast<uint64_t>(LUREMAsm);	     
}

// Used by GenerateGDBPauseCheck, GenerateInterpreterTrampoline
static thread_local vixl::aarch64::Assembler emit(nullptr, 1);

size_t Arm64Dispatcher::GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) {

  *emit.GetBuffer() = vixl::CodeBuffer(CodeBuffer, MaxGDBPauseCheckSize);
  
  vixl::CodeBufferCheckScope scope(&emit, MaxGDBPauseCheckSize, vixl::CodeBufferCheckScope::kDontReserveBufferSpace, vixl::CodeBufferCheckScope::kNoAssert);

  aarch64::Label RunBlock;

  // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
  // This happens when single stepping

  static_assert(sizeof(FEXCore::Context::Context::Config.RunningMode) == 4, "This is expected to be size of 4");
  emit.ldr(x0, STATE_PTR(CpuStateFrame, Thread)); // Get thread
  emit.ldr(x0, MemOperand(x0, offsetof(FEXCore::Core::InternalThreadState, CTX))); // Get Context
  emit.ldr(w0, MemOperand(x0, offsetof(FEXCore::Context::Context, Config.RunningMode)));

  // If the value == 0 then we don't need to stop
  emit.cbz(w0, &RunBlock);
  {
    Literal l_GuestRIP {GuestRIP};
    // Make sure RIP is syncronized to the context
    emit.ldr(x0, &l_GuestRIP);
    emit.str(x0, STATE_PTR(CpuStateFrame, State.rip));

    // Stop the thread
    emit.ldr(x0, STATE_PTR(CpuStateFrame, Pointers.Common.ThreadPauseHandlerSpillSRA));
    emit.br(x0);
    emit.place(&l_GuestRIP);
  }
  emit.bind(&RunBlock);
  emit.FinalizeCode();

  auto UsedBytes = emit.GetBuffer()->GetCursorOffset();
  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(CodeBuffer, UsedBytes);
  return UsedBytes;
}

size_t Arm64Dispatcher::GenerateInterpreterTrampoline(uint8_t *CodeBuffer) {
  LOGMAN_THROW_A_FMT(!config.StaticRegisterAllocation, "GenerateInterpreterTrampoline dispatcher does not support SRA");
  
  *emit.GetBuffer() = vixl::CodeBuffer(CodeBuffer, MaxInterpreterTrampolineSize);

  vixl::CodeBufferCheckScope scope(&emit, MaxInterpreterTrampolineSize, vixl::CodeBufferCheckScope::kDontReserveBufferSpace, vixl::CodeBufferCheckScope::kNoAssert);

  aarch64::Label InlineIRData;

  emit.mov(x0, STATE);
  emit.adr(x1, &InlineIRData);

  emit.ldr(x3, STATE_PTR(CpuStateFrame, Pointers.Interpreter.FragmentExecuter));
  emit.blr(x3);

  emit.ldr(x0, STATE_PTR(CpuStateFrame, Pointers.Common.DispatcherLoopTop));
  emit.br(x0);

  emit.bind(&InlineIRData);

  emit.FinalizeCode();

  auto UsedBytes = emit.GetBuffer()->GetCursorOffset();
  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(CodeBuffer, UsedBytes);
  return UsedBytes;
}

void Arm64Dispatcher::SpillSRA(FEXCore::Core::InternalThreadState *Thread, void *ucontext, uint32_t IgnoreMask) {
  for(int i = 0; i < SRA64.size(); i++) {
    if (IgnoreMask & (1U << SRA64[i].GetCode())) {
      // Skip this one, it's already spilled
      continue;
    }
    Thread->CurrentFrame->State.gregs[i] = ArchHelpers::Context::GetArmReg(ucontext, SRA64[i].GetCode());
  }

  for(int i = 0; i < SRAFPR.size(); i++) {
    auto FPR = ArchHelpers::Context::GetArmFPR(ucontext, SRAFPR[i].GetCode());
    memcpy(&Thread->CurrentFrame->State.xmm[i][0], &FPR, sizeof(__uint128_t));
  }
}

void Arm64Dispatcher::InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) {
// Setup dispatcher specific pointers that need to be accessed from JIT code
  {
    auto &Common = Thread->CurrentFrame->Pointers.Common;

    Common.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Common.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Common.ExitFunctionLinker = ExitFunctionLinkerAddress;
    Common.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddressSpillSRA;
    Common.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddressSpillSRA;
    Common.UnimplementedInstructionHandler = UnimplementedInstructionAddress;
    Common.OverflowExceptionHandler = OverflowExceptionInstructionAddress;
    Common.SignalReturnHandler = SignalHandlerReturnAddress;

    auto &AArch64 = Thread->CurrentFrame->Pointers.AArch64;
    AArch64.LUDIVHandler = LUDIVHandlerAddress;
    AArch64.LDIVHandler = LDIVHandlerAddress;
    AArch64.LUREMHandler = LUREMHandlerAddress;
    AArch64.LREMHandler = LREMHandlerAddress;
  }
}

std::unique_ptr<Dispatcher> Dispatcher::CreateArm64(FEXCore::Context::Context *CTX, const DispatcherConfig &Config) {
  return std::make_unique<Arm64Dispatcher>(CTX, Config);
}

}
