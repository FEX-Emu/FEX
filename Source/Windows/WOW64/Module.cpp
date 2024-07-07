// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|WOW64
desc: Implements the WOW64 BT module API using FEXCore
$end_info$
*/

// Thanks to André Zwing, whose ideas from https://github.com/AndreRH/hangover this code is based upon

#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/Utils/EnumOperators.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/TypeDefines.h>

#include "Common/Config.h"
#include "Common/TSOHandlerConfig.h"
#include "Common/InvalidationTracker.h"
#include "Common/CPUFeatures.h"
#include "DummyHandlers.h"
#include "BTInterface.h"

#include <cstdint>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <utility>
#include <unordered_map>
#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <wine/debug.h>
#include <wine/unixlib.h>

namespace ControlBits {
// When this is unset, a thread can be safely interrupted and have its context recovered
// IMPORTANT: This can only safely be written by the owning thread
static constexpr uint32_t IN_JIT {1U << 0};

// JIT entry polls this bit until it is unset, at which point CONTROL_IN_JIT will be set
static constexpr uint32_t PAUSED {1U << 1};

// When this is set, the CPU context stored in the CPU area has not yet been flushed to the FEX TLS
static constexpr uint32_t WOW_CPU_AREA_DIRTY {1U << 2};
}; // namespace ControlBits

struct TLS {
  enum class Slot : size_t {
    ENTRY_CONTEXT = WOW64_TLS_MAX_NUMBER,
    CONTROL_WORD = WOW64_TLS_MAX_NUMBER - 1,
    THREAD_STATE = WOW64_TLS_MAX_NUMBER - 2,
  };

  _TEB* TEB;

  explicit TLS(_TEB* TEB)
    : TEB(TEB) {}

  std::atomic<uint32_t>& ControlWord() const {
    // TODO: Change this when libc++ gains std::atomic_ref support
    return reinterpret_cast<std::atomic<uint32_t>&>(TEB->TlsSlots[FEXCore::ToUnderlying(Slot::CONTROL_WORD)]);
  }

  CONTEXT*& EntryContext() const {
    return reinterpret_cast<CONTEXT*&>(TEB->TlsSlots[FEXCore::ToUnderlying(Slot::ENTRY_CONTEXT)]);
  }

  FEXCore::Core::InternalThreadState*& ThreadState() const {
    return reinterpret_cast<FEXCore::Core::InternalThreadState*&>(TEB->TlsSlots[FEXCore::ToUnderlying(Slot::THREAD_STATE)]);
  }
};

class WowSyscallHandler;

namespace {
namespace BridgeInstrs {
  // These directly jumped to by the guest to make system calls
  void* Syscall {};
  void* UnixCall {};
} // namespace BridgeInstrs

fextl::unique_ptr<FEXCore::Context::Context> CTX;
fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> SignalDelegator;
fextl::unique_ptr<WowSyscallHandler> SyscallHandler;

std::optional<FEX::Windows::InvalidationTracker> InvalidationTracker;
std::optional<FEX::Windows::CPUFeatures> CPUFeatures;

std::mutex ThreadCreationMutex;
// Map of TIDs to their FEX thread state, `ThreadCreationMutex` must be locked when accessing
std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*> Threads;

std::pair<NTSTATUS, TLS> GetThreadTLS(HANDLE Thread) {
  THREAD_BASIC_INFORMATION Info;
  const NTSTATUS Err = NtQueryInformationThread(Thread, ThreadBasicInformation, &Info, sizeof(Info), nullptr);
  return {Err, TLS {reinterpret_cast<_TEB*>(Info.TebBaseAddress)}};
}

TLS GetTLS() {
  return TLS {NtCurrentTeb()};
}

uint64_t GetWowTEB(void* TEB) {
  static constexpr size_t WowTEBOffsetMemberOffset {0x180c};
  return static_cast<uint64_t>(
    *reinterpret_cast<LONG*>(reinterpret_cast<uintptr_t>(TEB) + WowTEBOffsetMemberOffset) + reinterpret_cast<uint64_t>(TEB));
}

bool IsAddressInJit(uint64_t Address) {
  auto Thread = GetTLS().ThreadState();
  return Thread->CTX->IsAddressInCodeBuffer(Thread, Address);
}
} // namespace

namespace Context {
void LoadStateFromWowContext(FEXCore::Core::InternalThreadState* Thread, uint64_t WowTEB, WOW64_CONTEXT* Context) {
  auto& State = Thread->CurrentFrame->State;

  // General register state

  State.gregs[FEXCore::X86State::REG_RAX] = Context->Eax;
  State.gregs[FEXCore::X86State::REG_RBX] = Context->Ebx;
  State.gregs[FEXCore::X86State::REG_RCX] = Context->Ecx;
  State.gregs[FEXCore::X86State::REG_RDX] = Context->Edx;
  State.gregs[FEXCore::X86State::REG_RSI] = Context->Esi;
  State.gregs[FEXCore::X86State::REG_RDI] = Context->Edi;
  State.gregs[FEXCore::X86State::REG_RBP] = Context->Ebp;
  State.gregs[FEXCore::X86State::REG_RSP] = Context->Esp;

  State.rip = Context->Eip;
  CTX->SetFlagsFromCompactedEFLAGS(Thread, Context->EFlags);

  State.es_idx = Context->SegEs & 0xffff;
  State.cs_idx = Context->SegCs & 0xffff;
  State.ss_idx = Context->SegSs & 0xffff;
  State.ds_idx = Context->SegDs & 0xffff;
  State.fs_idx = Context->SegFs & 0xffff;
  State.gs_idx = Context->SegGs & 0xffff;

  // The TEB is the only populated GDT entry by default
  State.gdt[(Context->SegFs & 0xffff) >> 3].base = WowTEB;
  State.fs_cached = WowTEB;
  State.es_cached = 0;
  State.cs_cached = 0;
  State.ss_cached = 0;
  State.ds_cached = 0;

  // Floating-point register state
  const auto* XSave = reinterpret_cast<XSAVE_FORMAT*>(Context->ExtendedRegisters);

  CTX->SetXMMRegistersFromState(Thread, reinterpret_cast<const __uint128_t*>(XSave->XmmRegisters), nullptr);
  memcpy(State.mm, XSave->FloatRegisters, sizeof(State.mm));

  State.FCW = XSave->ControlWord;
  State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (XSave->StatusWord >> 8) & 1;
  State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (XSave->StatusWord >> 9) & 1;
  State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (XSave->StatusWord >> 10) & 1;
  State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (XSave->StatusWord >> 14) & 1;
  State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (XSave->StatusWord >> 11) & 0b111;
  State.AbridgedFTW = XSave->TagWord;
}

void StoreWowContextFromState(FEXCore::Core::InternalThreadState* Thread, WOW64_CONTEXT* Context) {
  auto& State = Thread->CurrentFrame->State;

  // General register state

  Context->Eax = State.gregs[FEXCore::X86State::REG_RAX];
  Context->Ebx = State.gregs[FEXCore::X86State::REG_RBX];
  Context->Ecx = State.gregs[FEXCore::X86State::REG_RCX];
  Context->Edx = State.gregs[FEXCore::X86State::REG_RDX];
  Context->Esi = State.gregs[FEXCore::X86State::REG_RSI];
  Context->Edi = State.gregs[FEXCore::X86State::REG_RDI];
  Context->Ebp = State.gregs[FEXCore::X86State::REG_RBP];
  Context->Esp = State.gregs[FEXCore::X86State::REG_RSP];

  Context->Eip = State.rip;
  Context->EFlags = CTX->ReconstructCompactedEFLAGS(Thread, false, nullptr, 0);

  Context->SegEs = State.es_idx;
  Context->SegCs = State.cs_idx;
  Context->SegSs = State.ss_idx;
  Context->SegDs = State.ds_idx;
  Context->SegFs = State.fs_idx;
  Context->SegGs = State.gs_idx;

  // Floating-point register state

  auto* XSave = reinterpret_cast<XSAVE_FORMAT*>(Context->ExtendedRegisters);

  CTX->ReconstructXMMRegisters(Thread, reinterpret_cast<__uint128_t*>(XSave->XmmRegisters), nullptr);
  memcpy(XSave->FloatRegisters, State.mm, sizeof(State.mm));

  XSave->ControlWord = State.FCW;
  XSave->StatusWord = (State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) | (State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) |
                      (State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) | (State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) |
                      (State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);
  XSave->TagWord = State.AbridgedFTW;

  Context->FloatSave.ControlWord = XSave->ControlWord;
  Context->FloatSave.StatusWord = XSave->StatusWord;
  Context->FloatSave.TagWord = FEXCore::FPState::ConvertFromAbridgedFTW(XSave->StatusWord, State.mm, XSave->TagWord);
  Context->FloatSave.ErrorOffset = XSave->ErrorOffset;
  Context->FloatSave.ErrorSelector = XSave->ErrorSelector | (XSave->ErrorOpcode << 16);
  Context->FloatSave.DataOffset = XSave->DataOffset;
  Context->FloatSave.DataSelector = XSave->DataSelector;
  Context->FloatSave.Cr0NpxState = XSave->StatusWord | 0xffff0000;
}

NTSTATUS FlushThreadStateContext(HANDLE Thread) {
  const auto [Err, TLS] = GetThreadTLS(Thread);
  if (Err) {
    return Err;
  }

  WOW64_CONTEXT TmpWowContext {.ContextFlags = WOW64_CONTEXT_FULL | WOW64_CONTEXT_EXTENDED_REGISTERS};

  Context::StoreWowContextFromState(TLS.ThreadState(), &TmpWowContext);
  return RtlWow64SetThreadContext(Thread, &TmpWowContext);
}

void ReconstructThreadState(CONTEXT* Context) {
  const auto& Config = SignalDelegator->GetConfig();
  auto* Thread = GetTLS().ThreadState();
  auto& State = Thread->CurrentFrame->State;

  State.rip = CTX->RestoreRIPFromHostPC(Thread, Context->Pc);

  // Spill all SRA GPRs
  for (size_t i = 0; i < Config.SRAGPRCount; i++) {
    State.gregs[i] = Context->X[Config.SRAGPRMapping[i]];
  }

  // Spill all SRA FPRs
  for (size_t i = 0; i < Config.SRAFPRCount; i++) {
    memcpy(State.xmm.sse.data[i], &Context->V[Config.SRAFPRMapping[i]], sizeof(__uint128_t));
  }
}

WOW64_CONTEXT ReconstructWowContext(CONTEXT* Context) {
  ReconstructThreadState(Context);

  WOW64_CONTEXT WowContext {
    .ContextFlags = WOW64_CONTEXT_ALL,
  };

  auto* XSave = reinterpret_cast<XSAVE_FORMAT*>(WowContext.ExtendedRegisters);
  XSave->ControlWord = 0x27f;
  XSave->MxCsr = 0x1f80;

  Context::StoreWowContextFromState(GetTLS().ThreadState(), &WowContext);
  return WowContext;
}

static std::optional<FEX::Windows::TSOHandlerConfig> HandlerConfig;

bool HandleUnalignedAccess(CONTEXT* Context) {
  auto Thread = GetTLS().ThreadState();
  if (!Thread->CTX->IsAddressInCodeBuffer(Thread, Context->Pc)) {
    return false;
  }

  const auto Result =
    FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(Thread, HandlerConfig->GetUnalignedHandlerType(), Context->Pc, &Context->X0);
  if (!Result.first) {
    return false;
  }

  Context->Pc += Result.second;
  return true;
}

void LockJITContext() {
  uint32_t Expected = GetTLS().ControlWord().load(), New;

  // Spin until PAUSED is unset, setting IN_JIT when that occurs
  do {
    Expected = Expected & ~ControlBits::PAUSED;
    New = (Expected | ControlBits::IN_JIT) & ~ControlBits::WOW_CPU_AREA_DIRTY;
  } while (!GetTLS().ControlWord().compare_exchange_weak(Expected, New, std::memory_order::relaxed));
  std::atomic_signal_fence(std::memory_order::seq_cst);

  // If the CPU area is dirty, flush it to the JIT context before reentry
  if (Expected & ControlBits::WOW_CPU_AREA_DIRTY) {
    WOW64_CONTEXT* WowContext;
    RtlWow64GetCurrentCpuArea(nullptr, reinterpret_cast<void**>(&WowContext), nullptr);
    Context::LoadStateFromWowContext(GetTLS().ThreadState(), GetWowTEB(NtCurrentTeb()), WowContext);
  }
}

void UnlockJITContext() {
  std::atomic_signal_fence(std::memory_order::seq_cst);
  GetTLS().ControlWord().fetch_and(~ControlBits::IN_JIT, std::memory_order::relaxed);
}

bool HandleSuspendInterrupt(CONTEXT* Context, uint64_t FaultAddress) {
  if (FaultAddress != reinterpret_cast<uint64_t>(&GetTLS().ThreadState()->InterruptFaultPage)) {
    return false;
  }

  void* TmpAddress = reinterpret_cast<void*>(FaultAddress);
  SIZE_T TmpSize = FEXCore::Utils::FEX_PAGE_SIZE;
  ULONG TmpProt;
  NtProtectVirtualMemory(NtCurrentProcess(), &TmpAddress, &TmpSize, PAGE_READWRITE, &TmpProt);

  // Since interrupts only happen at the start of blocks, the reconstructed state should be entirely accurate
  ReconstructThreadState(Context);

  // Yield to the suspender
  UnlockJITContext();
  LockJITContext();

  // Adjust context to return to the dispatcher, reloading SRA from thread state
  const auto& Config = SignalDelegator->GetConfig();
  Context->Pc = Config.AbsoluteLoopTopAddressFillSRA;
  return true;
}
} // namespace Context

namespace Logging {
void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Output = fextl::fmt::format("[{}][{:X}] {}\n", LogMan::DebugLevelStr(Level), GetCurrentThreadId(), Message);
  __wine_dbg_output(Output.c_str());
}

void AssertHandler(const char* Message) {
  const auto Output = fextl::fmt::format("[ASSERT] {}\n", Message);
  __wine_dbg_output(Output.c_str());
}

void Init() {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
}
} // namespace Logging

// Calls a 2-argument function `Func` setting the parent unwind frame information to the given SP and PC
__attribute__((naked)) extern "C" uint64_t SEHFrameTrampoline2Args(void* Arg0, void* Arg1, void* Func, uint64_t Sp, uint64_t Pc) {
  asm(".seh_proc SEHFrameTrampoline2Args;"
      "stp x3, x4, [sp, #-0x10]!;"
      ".seh_pushframe;"
      "stp x29, x30, [sp, #-0x10]!;"
      ".seh_save_fplr_x 16;"
      ".seh_endprologue;"
      "blr x2;"
      "ldp x29, x30, [sp], 0x20;"
      "ret;"
      ".seh_endproc;");
}

class WowSyscallHandler : public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
public:
  WowSyscallHandler() {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_WIN32;
  }

  static uint64_t HandleSyscallImpl(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) {
    const uint64_t ReturnRIP = *(uint32_t*)(Frame->State.gregs[FEXCore::X86State::REG_RSP]); // Return address from the stack
    uint64_t ReturnRSP = Frame->State.gregs[FEXCore::X86State::REG_RSP] + 4;                 // Stack pointer after popping return address
    uint64_t ReturnRAX = 0;

    if (Frame->State.rip == (uint64_t)BridgeInstrs::UnixCall) {
      struct StackLayout {
        unixlib_handle_t Handle;
        UINT32 ID;
        ULONG32 Args;
      }* StackArgs = reinterpret_cast<StackLayout*>(ReturnRSP);

      ReturnRSP += sizeof(StackLayout);

      Context::UnlockJITContext();
      ReturnRAX = static_cast<uint64_t>(__wine_unix_call(StackArgs->Handle, StackArgs->ID, ULongToPtr(StackArgs->Args)));
      Context::LockJITContext();
    } else if (Frame->State.rip == (uint64_t)BridgeInstrs::Syscall) {
      const uint64_t EntryRAX = Frame->State.gregs[FEXCore::X86State::REG_RAX];

      Context::UnlockJITContext();
      Wow64ProcessPendingCrossProcessItems();
      ReturnRAX = static_cast<uint64_t>(Wow64SystemServiceEx(static_cast<UINT>(EntryRAX), reinterpret_cast<UINT*>(ReturnRSP + 4)));
      Context::LockJITContext();
    }
    // If a new context has been set, use it directly and don't return to the syscall caller
    if (Frame->State.rip == (uint64_t)BridgeInstrs::Syscall || Frame->State.rip == (uint64_t)BridgeInstrs::UnixCall) {
      Frame->State.gregs[FEXCore::X86State::REG_RAX] = ReturnRAX;
      Frame->State.gregs[FEXCore::X86State::REG_RSP] = ReturnRSP;
      Frame->State.rip = ReturnRIP;
    }

    // NORETURNEDRESULT causes this result to be ignored since we restore all registers back from memory after a syscall anyway
    return 0;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    // Stash the the context pointer on the stack, as Simulate can be called from this syscall handler which would overwrite it
    CONTEXT* EntryContext = GetTLS().EntryContext();
    // Call the syscall handler with unwind information pointing to Simulate as its caller
    uint64_t Ret = SEHFrameTrampoline2Args(reinterpret_cast<void*>(Frame), reinterpret_cast<void*>(Args),
                                           reinterpret_cast<void*>(&HandleSyscallImpl), EntryContext->Sp, EntryContext->Pc);
    GetTLS().EntryContext() = EntryContext;
    return Ret;
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    return {.NumArgs = 0, .HasReturn = false, .HostSyscallNumber = -1};
  }

  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) override {
    return {0, 0};
  }

  void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override {
    InvalidationTracker->ReprotectRWXIntervals(Start, Length);
  }
};

void BTCpuProcessInit() {
  Logging::Init();
  FEX::Config::InitializeConfigs();
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEX::Config::CreateGlobalMainLayer());
  FEXCore::Config::AddLayer(FEX::Config::CreateMainLayer());
  FEXCore::Config::Load();
  FEXCore::Config::ReloadMetaLayer();

  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS_INTERPRETER, "0");
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_INTERPRETER_INSTALLED, "0");
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS64BIT_MODE, "0");

  // Not applicable to Windows
  FEXCore::Config::EraseSet(FEXCore::Config::ConfigOption::CONFIG_TSOAUTOMIGRATION, "0");

  FEXCore::Context::InitializeStaticTables(FEXCore::Context::MODE_32BIT);

  SignalDelegator = fextl::make_unique<FEX::DummyHandlers::DummySignalDelegator>();
  SyscallHandler = fextl::make_unique<WowSyscallHandler>();
  Context::HandlerConfig.emplace();

  CTX = FEXCore::Context::Context::CreateNewContext();
  CTX->SetSignalDelegator(SignalDelegator.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  CTX->InitCore();
  InvalidationTracker.emplace(*CTX, Threads);
  CPUFeatures.emplace(*CTX);

  // Allocate the syscall/unixcall trampolines in the lower 2GB of the address space
  SIZE_T Size = 4;
  void* Addr = nullptr;
  NtAllocateVirtualMemory(NtCurrentProcess(), &Addr, (1U << 31) - 1, &Size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  *reinterpret_cast<uint32_t*>(Addr) = 0x2ecd2ecd;
  BridgeInstrs::Syscall = Addr;
  BridgeInstrs::UnixCall = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Addr) + 2);
}

NTSTATUS BTCpuThreadInit() {
  auto* Thread = CTX->CreateThread(0, 0);
  GetTLS().ThreadState() = Thread;

  std::scoped_lock Lock(ThreadCreationMutex);
  Threads.emplace(GetCurrentThreadId(), Thread);
  return STATUS_SUCCESS;
}

NTSTATUS BTCpuThreadTerm(HANDLE Thread) {
  const auto [Err, TLS] = GetThreadTLS(Thread);
  if (Err) {
    return Err;
  }

  {
    THREAD_BASIC_INFORMATION Info;
    if (NTSTATUS Err = NtQueryInformationThread(Thread, ThreadBasicInformation, &Info, sizeof(Info), nullptr); Err) {
      return Err;
    }

    const auto ThreadTID = reinterpret_cast<uint64_t>(Info.ClientId.UniqueThread);
    std::scoped_lock Lock(ThreadCreationMutex);
    Threads.erase(ThreadTID);
  }

  CTX->DestroyThread(TLS.ThreadState());
  return STATUS_SUCCESS;
}

void* BTCpuGetBopCode() {
  return BridgeInstrs::Syscall;
}

void* __wine_get_unix_opcode() {
  return BridgeInstrs::UnixCall;
}

NTSTATUS BTCpuGetContext(HANDLE Thread, HANDLE Process, void* Unknown, WOW64_CONTEXT* Context) {
  auto [Err, TLS] = GetThreadTLS(Thread);
  if (Err) {
    return Err;
  }

  if (!(TLS.ControlWord().load(std::memory_order::relaxed) & ControlBits::WOW_CPU_AREA_DIRTY)) {
    if (Err = Context::FlushThreadStateContext(Thread); Err) {
      return Err;
    }
  }

  return RtlWow64GetThreadContext(Thread, Context);
}

NTSTATUS BTCpuSetContext(HANDLE Thread, HANDLE Process, void* Unknown, WOW64_CONTEXT* Context) {
  auto [Err, TLS] = GetThreadTLS(Thread);
  if (Err) {
    return Err;
  }


  // Back-up the input context incase we've been passed the CPU area (the flush below would wipe it out otherwise)
  WOW64_CONTEXT TmpContext = *Context;

  if (!(TLS.ControlWord().load(std::memory_order::relaxed) & ControlBits::WOW_CPU_AREA_DIRTY)) {
    if (Err = Context::FlushThreadStateContext(Thread); Err) {
      return Err;
    }
  }

  // Merge the input context into the CPU area then pass the full context into the JIT
  if (Err = RtlWow64SetThreadContext(Thread, &TmpContext); Err) {
    return Err;
  }

  TmpContext.ContextFlags = WOW64_CONTEXT_FULL | WOW64_CONTEXT_EXTENDED_REGISTERS;

  if (Err = RtlWow64GetThreadContext(Thread, &TmpContext); Err) {
    return Err;
  }

  Context::LoadStateFromWowContext(TLS.ThreadState(), GetWowTEB(TLS.TEB), &TmpContext);
  return STATUS_SUCCESS;
}

void BTCpuSimulate() {
  CONTEXT entry_context;
  RtlCaptureContext(&entry_context);
  GetTLS().EntryContext() = &entry_context;

  Context::LockJITContext();
  CTX->ExecuteThread(GetTLS().ThreadState());
  Context::UnlockJITContext();
}

NTSTATUS BTCpuSuspendLocalThread(HANDLE Thread, ULONG* Count) {
  THREAD_BASIC_INFORMATION Info;
  if (NTSTATUS Err = NtQueryInformationThread(Thread, ThreadBasicInformation, &Info, sizeof(Info), nullptr); Err) {
    return Err;
  }

  const auto ThreadTID = reinterpret_cast<uint64_t>(Info.ClientId.UniqueThread);
  if (ThreadTID == GetCurrentThreadId()) {
    LogMan::Msg::DFmt("Suspending self");
    // Mark the CPU area as dirty, to force the JIT context to be restored from it on entry as it may be changed using
    // SetThreadContext (which doesn't use the BTCpu API)
    if (!(GetTLS().ControlWord().fetch_or(ControlBits::WOW_CPU_AREA_DIRTY, std::memory_order::relaxed) & ControlBits::WOW_CPU_AREA_DIRTY)) {
      if (NTSTATUS Err = Context::FlushThreadStateContext(Thread); Err) {
        return Err;
      }
    }

    return NtSuspendThread(Thread, Count);
  }

  LogMan::Msg::DFmt("Suspending thread: {:X}", ThreadTID);

  auto [Err, TLS] = GetThreadTLS(Thread);
  if (Err) {
    return Err;
  }

  std::scoped_lock Lock(ThreadCreationMutex);

  // If the thread hasn't yet been initialized, suspend it without special handling as it wont yet have entered the JIT
  if (!Threads.contains(ThreadTID)) {
    return NtSuspendThread(Thread, Count);
  }

  // If CONTROL_IN_JIT is unset at this point, then it can never be set (and thus the JIT cannot be reentered) as
  // CONTROL_PAUSED has been set, as such, while this may redundantly request interrupts in rare cases it will never
  // miss them
  if (TLS.ControlWord().fetch_or(ControlBits::PAUSED, std::memory_order::relaxed) & ControlBits::IN_JIT) {
    LogMan::Msg::DFmt("Thread {:X} is in JIT, polling for interrupt", ThreadTID);

    ULONG TmpProt;
    void* TmpAddress = &TLS.ThreadState()->InterruptFaultPage;
    SIZE_T TmpSize = FEXCore::Utils::FEX_PAGE_SIZE;
    NtProtectVirtualMemory(NtCurrentProcess(), &TmpAddress, &TmpSize, PAGE_READONLY, &TmpProt);
  }

  // Spin until the JIT is interrupted
  while (TLS.ControlWord().load() & ControlBits::IN_JIT)
    ;

  // The JIT has now been interrupted and the context stored in the thread's CPU area is up-to-date
  if (Err = NtSuspendThread(Thread, Count); Err) {
    TLS.ControlWord().fetch_and(~ControlBits::PAUSED, std::memory_order::relaxed);
    return Err;
  }

  CONTEXT TmpContext {
    .ContextFlags = CONTEXT_INTEGER,
  };

  // NtSuspendThread may return before the thread is actually suspended, so a sync operation like NtGetContextThread
  // needs to be called to ensure it is before we unset CONTROL_PAUSED
  std::ignore = NtGetContextThread(Thread, &TmpContext);

  // Mark the CPU area as dirty, to force the JIT context to be restored from it on entry as it may be changed using
  // SetThreadContext (which doesn't use the BTCpu API)
  if (!(TLS.ControlWord().fetch_or(ControlBits::WOW_CPU_AREA_DIRTY, std::memory_order::relaxed) & ControlBits::WOW_CPU_AREA_DIRTY)) {
    if (Err = Context::FlushThreadStateContext(Thread); Err) {
      return Err;
    }
  }

  LogMan::Msg::DFmt("Thread suspended: {:X}", ThreadTID);

  // Now the thread is suspended on the host, unset CONTROL_PAUSED so that NtResumeThread will
  // continue execution in the JIT
  TLS.ControlWord().fetch_and(~ControlBits::PAUSED, std::memory_order::relaxed);

  return Err;
}

NTSTATUS BTCpuResetToConsistentState(EXCEPTION_POINTERS* Ptrs) {
  auto* Context = Ptrs->ContextRecord;
  const auto* Exception = Ptrs->ExceptionRecord;
  if (Exception->ExceptionCode == EXCEPTION_DATATYPE_MISALIGNMENT && Context::HandleUnalignedAccess(Context)) {
    LogMan::Msg::DFmt("Handled unaligned atomic: new pc: {:X}", Context->Pc);
    NtContinue(Context, FALSE);
  }

  if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    const auto FaultAddress = static_cast<uint64_t>(Exception->ExceptionInformation[1]);


    if (Context::HandleSuspendInterrupt(Context, FaultAddress)) {
      LogMan::Msg::DFmt("Resumed from suspend");
      NtContinue(Context, FALSE);
    }

    bool HandledRWX = false;
    if (GetTLS().ThreadState()) {
      std::scoped_lock Lock(ThreadCreationMutex);
      HandledRWX = InvalidationTracker->HandleRWXAccessViolation(FaultAddress);
    }

    if (HandledRWX) {
      LogMan::Msg::DFmt("Handled self-modifying code: pc: {:X} fault: {:X}", Context->Pc, FaultAddress);
      NtContinue(Context, FALSE);
    }
  }

  if (!IsAddressInJit(Context->Pc)) {
    return STATUS_SUCCESS;
  }

  LogMan::Msg::DFmt("Reconstructing context");

  WOW64_CONTEXT WowContext = Context::ReconstructWowContext(Context);
  LogMan::Msg::DFmt("pc: {:X} eip: {:X}", Context->Pc, WowContext.Eip);

  BTCpuSetContext(GetCurrentThread(), GetCurrentProcess(), nullptr, &WowContext);
  Context::UnlockJITContext();

  // Replace the host context with one captured before JIT entry so host code can unwind
  memcpy(Context, GetTLS().EntryContext(), sizeof(*Context));

  return STATUS_SUCCESS;
}

void BTCpuFlushInstructionCache2(const void* Address, SIZE_T Size) {
  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), false);
}

void BTCpuNotifyMemoryAlloc(void* Address, SIZE_T Size, ULONG Type, ULONG Prot) {
  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), Prot);
}

void BTCpuNotifyMemoryProtect(void* Address, SIZE_T Size, ULONG NewProt) {
  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), NewProt);
}

void BTCpuNotifyMemoryFree(void* Address, SIZE_T Size, ULONG FreeType) {
  std::scoped_lock Lock(ThreadCreationMutex);
  if (!Size) {
    InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
  } else if (FreeType & MEM_DECOMMIT) {
    InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), true);
  }
}

void BTCpuNotifyUnmapViewOfSection(void* Address, ULONG Flags) {
  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
}

BOOLEAN WINAPI BTCpuIsProcessorFeaturePresent(UINT Feature) {
  return CPUFeatures->IsFeaturePresent(Feature) ? TRUE : FALSE;
}

BOOLEAN BTCpuUpdateProcessorInformation(SYSTEM_CPU_INFORMATION* Info) {
  CPUFeatures->UpdateInformation(Info);
  return TRUE;
}
