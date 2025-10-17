// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|ARM64EC
desc: Implements the ARM64EC BT module API using FEXCore
$end_info$
*/

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
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/Utils/SHMStats.h>
#include <FEXCore/Utils/EnumOperators.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include "Common/CallRetStack.h"
#include "Common/Config.h"
#include "Common/Exception.h"
#include "Common/InvalidationTracker.h"
#include "Common/OvercommitTracker.h"
#include "Common/TSOHandlerConfig.h"
#include "Common/CPUFeatures.h"
#include "Common/Logging.h"
#include "Common/Module.h"
#include "Common/CRT/CRT.h"
#include "Common/PortabilityInfo.h"
#include "Common/Handle.h"
#include "DummyHandlers.h"
#include "BTInterface.h"
#include "Windows/Common/SHMStats.h"
#include "Windows/Common/VolatileMetadata.h"

#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>
#include <wine/debug.h>

namespace Exception {
class ECSyscallHandler;
}

extern "C" {
extern IMAGE_DOS_HEADER __ImageBase; // Provided by the linker

extern void* ExitFunctionEC;
extern void* CheckCall;
extern void* ExitFunctionSuspendPoint;

void* X64ReturnInstr; // See Module.S
uintptr_t NtDllBase;

// Exports on ARM64EC point to x64 fast forward sequences to allow for redirecting to the JIT if functions are hotpatched. This LUT is from their addresses to the relative addresses of the native code exports.
uint32_t* NtDllRedirectionLUT;
uint32_t NtDllRedirectionLUTSize;

// Wine doesn't support issuing direct system calls with SVC, and unlike Windows it doesn't have a 'stable' syscall number for NtContinue
void* WineSyscallDispatcher;
uint64_t WineNtContinueSyscallId;
uint64_t WineNtAllocateVirtualMemorySyscallId;
uint64_t WineNtProtectVirtualMemorySyscallId;
static fextl::unordered_map<fextl::string, FEX::VolatileMetadata::ExtendedVolatileMetadata> ExtendedMetaData {};

NTSTATUS NtContinueNative(ARM64_NT_CONTEXT* NativeContext, BOOLEAN Alert);
NTSTATUS NtAllocateVirtualMemoryNative(HANDLE, PVOID*, ULONG_PTR, SIZE_T*, ULONG, ULONG);
NTSTATUS NtProtectVirtualMemoryNative(HANDLE, PVOID*, SIZE_T*, ULONG, ULONG*);

[[noreturn]]
void JumpSetStack(uintptr_t PC, uintptr_t SP);
}

struct ThreadCPUArea {
  static constexpr size_t TEBCPUAreaOffset = 0x1788;
  CHPE_V2_CPU_AREA_INFO* Area;

  explicit ThreadCPUArea(_TEB* TEB)
    : Area(*reinterpret_cast<CHPE_V2_CPU_AREA_INFO**>(reinterpret_cast<uintptr_t>(TEB) + TEBCPUAreaOffset)) {}

  uint64_t& EmulatorStackLimit() const {
    return Area->EmulatorStackLimit;
  }

  uint64_t& EmulatorStackBase() const {
    return Area->EmulatorStackBase;
  }

  ARM64EC_NT_CONTEXT& ContextAmd64() const {
    return *Area->ContextAmd64;
  }

  FEXCore::Core::CpuStateFrame*& StateFrame() const {
    return reinterpret_cast<FEXCore::Core::CpuStateFrame*&>(Area->EmulatorData[0]);
  }

  FEXCore::Core::InternalThreadState*& ThreadState() const {
    return reinterpret_cast<FEXCore::Core::InternalThreadState*&>(Area->EmulatorData[1]);
  }

  uint64_t& DispatcherLoopTopEnterEC() const {
    return reinterpret_cast<uint64_t&>(Area->EmulatorData[2]);
  }

  uint64_t& DispatcherLoopTopEnterECFillSRA() const {
    return reinterpret_cast<uint64_t&>(Area->EmulatorData[3]);
  }
};

namespace {
fextl::unique_ptr<FEXCore::Context::Context> CTX;
fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> SignalDelegator;
fextl::unique_ptr<Exception::ECSyscallHandler> SyscallHandler;
fextl::unique_ptr<FEX::Windows::StatAlloc> StatAllocHandler;
std::optional<FEX::Windows::InvalidationTracker> InvalidationTracker;
std::optional<FEX::Windows::CPUFeatures> CPUFeatures;
std::optional<FEX::Windows::OvercommitTracker> OvercommitTracker;

std::recursive_mutex ThreadCreationMutex;
// Map of TIDs to their FEX thread state, `ThreadCreationMutex` must be locked when accessing
std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*> Threads;

std::pair<NTSTATUS, ThreadCPUArea> GetThreadCPUArea(HANDLE Thread) {
  THREAD_BASIC_INFORMATION Info;
  const NTSTATUS Err = NtQueryInformationThread(Thread, ThreadBasicInformation, &Info, sizeof(Info), nullptr);
  return {Err, ThreadCPUArea(reinterpret_cast<_TEB*>(Info.TebBaseAddress))};
}

ThreadCPUArea GetCPUArea() {
  return ThreadCPUArea(NtCurrentTeb());
}

bool IsEmulatorStackAddress(uint64_t Address) {
  return Address <= GetCPUArea().EmulatorStackBase() && Address >= GetCPUArea().EmulatorStackLimit();
}

bool IsDispatcherAddress(uint64_t Address) {
  const auto& Config = SignalDelegator->GetConfig();
  return Address >= Config.DispatcherBegin && Address < Config.DispatcherEnd;
}


void FillNtDllLUTs(HMODULE NtDll) {
  ULONG Size;
  const auto* LoadConfig =
    reinterpret_cast<_IMAGE_LOAD_CONFIG_DIRECTORY64*>(RtlImageDirectoryEntryToData(NtDll, true, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &Size));
  const auto* CHPEMetadata = reinterpret_cast<IMAGE_ARM64EC_METADATA*>(LoadConfig->CHPEMetadataPointer);
  const auto* RedirectionTableBegin = reinterpret_cast<IMAGE_ARM64EC_REDIRECTION_ENTRY*>(NtDllBase + CHPEMetadata->RedirectionMetadata);
  const auto* RedirectionTableEnd = RedirectionTableBegin + CHPEMetadata->RedirectionMetadataCount;

  NtDllRedirectionLUTSize = std::prev(RedirectionTableEnd)->Source + 1;

  SIZE_T AllocSize = NtDllRedirectionLUTSize * sizeof(uint32_t);
  NtAllocateVirtualMemoryNative(NtCurrentProcess(), reinterpret_cast<void**>(&NtDllRedirectionLUT), 0, &AllocSize, MEM_COMMIT | MEM_RESERVE,
                                PAGE_READWRITE);
  for (auto It = RedirectionTableBegin; It != RedirectionTableEnd; It++) {
    NtDllRedirectionLUT[It->Source] = It->Destination;
  }
}

template<typename T>
void WriteModuleRVA(HMODULE Module, LONG RVA, T Data) {
  if (!RVA) {
    return;
  }

  void* Address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Module) + RVA);
  void* ProtAddress = Address;
  SIZE_T ProtSize = sizeof(T);
  ULONG Prot;
  NtProtectVirtualMemoryNative(NtCurrentProcess(), &ProtAddress, &ProtSize, PAGE_READWRITE, &Prot);
  *reinterpret_cast<T*>(Address) = Data;
  NtProtectVirtualMemoryNative(NtCurrentProcess(), &ProtAddress, &ProtSize, Prot, nullptr);
}

void PatchCallChecker() {
  // See the comment for CheckCall in Module.S for why this is necessary
  const auto Module = reinterpret_cast<HMODULE>(&__ImageBase);
  ULONG Size;
  const auto* LoadConfig =
    reinterpret_cast<_IMAGE_LOAD_CONFIG_DIRECTORY64*>(RtlImageDirectoryEntryToData(Module, true, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &Size));
  const auto* CHPEMetadata = reinterpret_cast<IMAGE_ARM64EC_METADATA*>(LoadConfig->CHPEMetadataPointer);
  WriteModuleRVA(Module, CHPEMetadata->__os_arm64x_dispatch_call, &CheckCall);
  WriteModuleRVA(Module, CHPEMetadata->__os_arm64x_dispatch_icall, &CheckCall);
  WriteModuleRVA(Module, CHPEMetadata->__os_arm64x_dispatch_icall_cfg, &CheckCall);
}

// Fills in the syscall numbers necessary to call *Native variants of syscalls from FEX under wine.
void ParseWineSyscallNumbers(HMODULE NtDll) {
  ULONG Size;
  const auto* Exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(RtlImageDirectoryEntryToData(NtDll, true, IMAGE_DIRECTORY_ENTRY_EXPORT, &Size));
  const auto* NameTable = reinterpret_cast<uint32_t*>(NtDllBase + Exports->AddressOfNames);
  const auto* FunctionTable = reinterpret_cast<uint32_t*>(NtDllBase + Exports->AddressOfFunctions);
  const auto* OrdinalTable = reinterpret_cast<uint16_t*>(NtDllBase + Exports->AddressOfNameOrdinals);
  struct SyscallEntry {
    const char* Name;
    uint32_t RVA;

    bool operator<(const SyscallEntry& Other) const {
      return RVA < Other.RVA;
    }
  };

  // Cannot use any syscalls at this stage, so rely on a stack-allocated array
  std::array<SyscallEntry, 0x200> SyscallTable;
  auto SyscallTableEnd = SyscallTable.begin();

  // Windows/Wine orders syscalls in memory by their ID, take advantage of that to find the syscall indices for those
  // which we need to manually issue. Note that all functions starting with Nt besides NtGetTickCount are syscalls.
  for (uint32_t Idx = 0; Idx < Exports->NumberOfNames; Idx++) {
    const char* Name = reinterpret_cast<const char*>(NtDllBase + NameTable[Idx]);
    if (Name[0] == 'N' && Name[1] == 't' && strcmp(Name, "NtGetTickCount") != 0) {
      *SyscallTableEnd++ = {Name, FunctionTable[OrdinalTable[Idx]]};
    }
  }

  // Sort such that index 0 is now syscall 0, etc
  std::sort(SyscallTable.begin(), SyscallTableEnd);

  for (auto it = SyscallTable.begin(); it != SyscallTableEnd; it++) {
    uint32_t CurSyscallId = static_cast<uint32_t>(std::distance(SyscallTable.begin(), it));
    if (strcmp(it->Name, "NtContinue") == 0) {
      WineNtContinueSyscallId = CurSyscallId;
    } else if (strcmp(it->Name, "NtAllocateVirtualMemory") == 0) {
      WineNtAllocateVirtualMemorySyscallId = CurSyscallId;
    } else if (strcmp(it->Name, "NtProtectVirtualMemory") == 0) {
      WineNtProtectVirtualMemorySyscallId = CurSyscallId;
    }
  }
}

// Syscall thunks may have been patched before FEX has loaded, the default call checker installed by ntdll into FEX will
// try to invoke the JIT when calling such patched syscalls but this obviously doesn't work before FEX is initalised.
// This function parses ntdll and sets up a custom call checker to prevent this, as such it must avoid using any syscall
// thunks itself.
void InitSyscalls() {
  // The ntdll exports called by GetModuleHandle/GetProcAddress aren't known to be patched before JIT init by any current
  // software so are safe to call, but if that changes the loader structures in the PEB could be parsed manually.
  const auto NtDll = GetModuleHandle("ntdll.dll");
  NtDllBase = reinterpret_cast<uintptr_t>(NtDll);

  const auto WineSyscallDispatcherPtr = reinterpret_cast<void**>(GetProcAddress(NtDll, "__wine_syscall_dispatcher"));
  if (WineSyscallDispatcherPtr) {
    WineSyscallDispatcher = *WineSyscallDispatcherPtr;
    ParseWineSyscallNumbers(NtDll);
  }

  FillNtDllLUTs(NtDll);
  PatchCallChecker();
}

void LoadImageVolatileMetadata(fextl::set<uint64_t>& VolatileInstructions, FEXCore::IntervalList<uint64_t>& VolatileValidRanges,
                               HMODULE Module, IMAGE_NT_HEADERS* Nt, uint64_t Address, uint64_t EndAddress) {
  ULONG Size;

  const auto* LoadConfig =
    reinterpret_cast<_IMAGE_LOAD_CONFIG_DIRECTORY64*>(RtlImageDirectoryEntryToData(Module, true, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &Size));
  if (!LoadConfig || LoadConfig->Size <= offsetof(_IMAGE_LOAD_CONFIG_DIRECTORY64, VolatileMetadataPointer)) {
    return;
  }

  if (LoadConfig->VolatileMetadataPointer < Address || LoadConfig->VolatileMetadataPointer + sizeof(IMAGE_VOLATILE_METADATA) >= EndAddress) {
    return;
  }

  const auto* VolatileMetadata = reinterpret_cast<IMAGE_VOLATILE_METADATA*>(LoadConfig->VolatileMetadataPointer);
  if (!VolatileMetadata || Address + VolatileMetadata->VolatileAccessTable + VolatileMetadata->VolatileAccessTableSize >= EndAddress ||
      Address + VolatileMetadata->VolatileInfoRangeTable + VolatileMetadata->VolatileInfoRangeTableSize >= EndAddress) {
    return;
  }

  const auto* VolatileAccessTableBegin = reinterpret_cast<IMAGE_VOLATILE_RVA_METADATA*>(Address + VolatileMetadata->VolatileAccessTable);
  const auto* VolatileAccessTableEnd =
    VolatileAccessTableBegin + (VolatileMetadata->VolatileAccessTableSize / sizeof(IMAGE_VOLATILE_RVA_METADATA));
  for (auto It = VolatileAccessTableBegin; It != VolatileAccessTableEnd; It++) {
    VolatileInstructions.emplace(Address + It->Rva);
  }

  const auto* VolatileInfoRangeTableBegin = reinterpret_cast<IMAGE_VOLATILE_RANGE_METADATA*>(Address + VolatileMetadata->VolatileInfoRangeTable);
  const auto* VolatileInfoRangeTableEnd =
    VolatileInfoRangeTableBegin + (VolatileMetadata->VolatileInfoRangeTableSize / sizeof(IMAGE_VOLATILE_RANGE_METADATA));
  for (auto It = VolatileInfoRangeTableBegin; It != VolatileInfoRangeTableEnd; It++) {
    VolatileValidRanges.Insert({Address + It->Rva, Address + It->Rva + It->Size});
  }
}

void LoadImageVolatileMetadata(const fextl::string& ModuleName, uint64_t Address) {
  const auto Module = reinterpret_cast<HMODULE>(Address);
  IMAGE_NT_HEADERS* Nt = RtlImageNtHeader(Module);
  uint64_t EndAddress = Address + Nt->OptionalHeader.SizeOfImage;

  fextl::set<uint64_t> VolatileInstructions {};
  FEXCore::IntervalList<uint64_t> VolatileValidRanges {};
  LoadImageVolatileMetadata(VolatileInstructions, VolatileValidRanges, Module, Nt, Address, EndAddress);

  auto it = ExtendedMetaData.find(ModuleName);
  if (it != ExtendedMetaData.end()) {
    FEX::Windows::ApplyFEXExtendedVolatileMetadata(it->second, VolatileInstructions, VolatileValidRanges, Address, EndAddress);
  }

  if (VolatileInstructions.empty() && VolatileValidRanges.Empty()) {
    return;
  }

  LogMan::Msg::DFmt("Loaded volatile metadata for {:X}: {} entries", Address, VolatileInstructions.size());
  std::scoped_lock Lock(CTX->GetCodeInvalidationMutex());
  CTX->AddForceTSOInformation(VolatileValidRanges, std::move(VolatileInstructions));
}

void HandleImageMap(uint64_t Address) {
  fextl::string ModuleName = FEX::Windows::GetSectionFilePath(Address);
  LogMan::Msg::DFmt("Load module {}: {:X}", ModuleName, Address);
  FEX_CONFIG_OPT(VolatileMetadata, VOLATILEMETADATA);
  if (VolatileMetadata) {
    LoadImageVolatileMetadata(ModuleName, Address);
  }
  InvalidationTracker->HandleImageMap(ModuleName, Address);
}
} // namespace

namespace Exception {
static std::optional<FEX::Windows::TSOHandlerConfig> HandlerConfig;
static uintptr_t KiUserExceptionDispatcher;

struct alignas(16) KiUserExceptionDispatcherStackLayout {
  ARM64_NT_CONTEXT Context;
  uint64_t Pad[4]; // Only present on newer Windows versions, likely for SVE.
  EXCEPTION_RECORD Rec;
  uint64_t Align;
  uint64_t Redzone[2];
};

static bool HandleUnalignedAccess(ARM64_NT_CONTEXT& Context) {
  auto Thread = GetCPUArea().ThreadState();
  if (!CTX->IsAddressInCodeBuffer(Thread, Context.Pc)) {
    return false;
  }

  FEXCORE_PROFILE_INSTANT_INCREMENT(Thread, AccumulatedSIGBUSCount, 1);
  const auto Result =
    FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(Thread, HandlerConfig->GetUnalignedHandlerType(), Context.Pc, &Context.X0);
  Context.Pc += Result.value_or(0);
  return Result.has_value();
}

static void LoadStateFromECContext(FEXCore::Core::InternalThreadState* Thread, CONTEXT& Context) {
  auto& State = Thread->CurrentFrame->State;

  if ((Context.ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
    // General register state
    State.gregs[FEXCore::X86State::REG_RAX] = Context.Rax;
    State.gregs[FEXCore::X86State::REG_RCX] = Context.Rcx;
    State.gregs[FEXCore::X86State::REG_RDX] = Context.Rdx;
    State.gregs[FEXCore::X86State::REG_RBX] = Context.Rbx;

    State.gregs[FEXCore::X86State::REG_RSI] = Context.Rsi;
    State.gregs[FEXCore::X86State::REG_RDI] = Context.Rdi;
    State.gregs[FEXCore::X86State::REG_R8] = Context.R8;
    State.gregs[FEXCore::X86State::REG_R9] = Context.R9;
    State.gregs[FEXCore::X86State::REG_R10] = Context.R10;
    State.gregs[FEXCore::X86State::REG_R11] = Context.R11;
    State.gregs[FEXCore::X86State::REG_R12] = Context.R12;
    State.gregs[FEXCore::X86State::REG_R13] = Context.R13;
    State.gregs[FEXCore::X86State::REG_R14] = Context.R14;
    State.gregs[FEXCore::X86State::REG_R15] = Context.R15;
  }

  if ((Context.ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
    State.rip = Context.Rip;
    State.gregs[FEXCore::X86State::REG_RSP] = Context.Rsp;
    State.gregs[FEXCore::X86State::REG_RBP] = Context.Rbp;
    CTX->SetFlagsFromCompactedEFLAGS(Thread, Context.EFlags);
  }

  if ((Context.ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {
    State.es_idx = Context.SegEs & 0xffff;
    State.cs_idx = Context.SegCs & 0xffff;
    State.ss_idx = Context.SegSs & 0xffff;
    State.ds_idx = Context.SegDs & 0xffff;
    State.fs_idx = Context.SegFs & 0xffff;
    State.gs_idx = Context.SegGs & 0xffff;

    // The TEB is the only populated GDT entry by default
    const auto TEB = reinterpret_cast<uint64_t>(NtCurrentTeb());
    auto GDT = State.GetSegmentFromIndex(State, (Context.SegGs & 0xffff));
    State.SetGDTBase(GDT, TEB);
    State.SetGDTLimit(GDT, 0xF'FFFFU);
    State.gs_cached = TEB;
    State.fs_cached = 0;
    State.es_cached = 0;
    State.cs_cached = 0;
    State.ss_cached = 0;
    State.ds_cached = 0;
  }

  if ((Context.ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {
    // Floating-point register state
    if ((Context.ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE) {
      const auto* Ymm = RtlLocateExtendedFeature(reinterpret_cast<CONTEXT_EX*>(&Context + 1), XSTATE_AVX, nullptr);
      CTX->SetXMMRegistersFromState(Thread, reinterpret_cast<const __uint128_t*>(Context.FltSave.XmmRegisters),
                                    reinterpret_cast<const __uint128_t*>(Ymm));
    } else {
      CTX->SetXMMRegistersFromState(Thread, reinterpret_cast<const __uint128_t*>(Context.FltSave.XmmRegisters), nullptr);
    }
    memcpy(State.mm, Context.FltSave.FloatRegisters, sizeof(State.mm));

    State.FCW = Context.FltSave.ControlWord;
    State.flags[FEXCore::X86State::X87FLAG_IE_LOC] = Context.FltSave.StatusWord & 1;
    State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (Context.FltSave.StatusWord >> 8) & 1;
    State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (Context.FltSave.StatusWord >> 9) & 1;
    State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (Context.FltSave.StatusWord >> 10) & 1;
    State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (Context.FltSave.StatusWord >> 14) & 1;
    State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (Context.FltSave.StatusWord >> 11) & 0b111;
    State.AbridgedFTW = Context.FltSave.TagWord;
  }
}

static void ReconstructThreadState(FEXCore::Core::InternalThreadState* Thread, ARM64_NT_CONTEXT& Context) {
  const auto& Config = SignalDelegator->GetConfig();
  auto& State = Thread->CurrentFrame->State;

  State.rip = CTX->RestoreRIPFromHostPC(Thread, Context.Pc);

  // Spill all SRA GPRs
  for (size_t i = 0; i < Config.SRAGPRCount; i++) {
    State.gregs[i] = Context.X[Config.SRAGPRMapping[i]];
  }

  // Spill all SRA FPRs
  for (size_t i = 0; i < Config.SRAFPRCount; i++) {
    memcpy(State.xmm.sse.data[i], &Context.V[Config.SRAFPRMapping[i]], sizeof(__uint128_t));
  }

  // Spill EFlags
  uint32_t EFlags = CTX->ReconstructCompactedEFLAGS(Thread, true, Context.X, Context.Cpsr);
  CTX->SetFlagsFromCompactedEFLAGS(Thread, EFlags);
}

// Reconstructs an x64 context from the input thread's state, packed into a regular ARM64 context following the ARM64EC register mapping
static ARM64_NT_CONTEXT StoreStateToPackedECContext(FEXCore::Core::InternalThreadState* Thread, uint32_t FPCR, uint32_t FPSR) {
  ARM64_NT_CONTEXT ECContext {};

  ECContext.ContextFlags = CONTEXT_ARM64_FULL;
  if (CPUFeatures->IsFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE)) {
    // This is a FEX extension and requires corresponding wine-side patches to be of use, however it is harmless to set
    // even if those patches are not used.
    ECContext.ContextFlags |= CONTEXT_ARM64_FEX_YMMSTATE;
  }

  auto& State = Thread->CurrentFrame->State;

  ECContext.X8 = State.gregs[FEXCore::X86State::REG_RAX];
  ECContext.X0 = State.gregs[FEXCore::X86State::REG_RCX];
  ECContext.X1 = State.gregs[FEXCore::X86State::REG_RDX];
  ECContext.X27 = State.gregs[FEXCore::X86State::REG_RBX];
  ECContext.Sp = State.gregs[FEXCore::X86State::REG_RSP];
  ECContext.Fp = State.gregs[FEXCore::X86State::REG_RBP];
  ECContext.X25 = State.gregs[FEXCore::X86State::REG_RSI];
  ECContext.X26 = State.gregs[FEXCore::X86State::REG_RDI];
  ECContext.X2 = State.gregs[FEXCore::X86State::REG_R8];
  ECContext.X3 = State.gregs[FEXCore::X86State::REG_R9];
  ECContext.X4 = State.gregs[FEXCore::X86State::REG_R10];
  ECContext.X5 = State.gregs[FEXCore::X86State::REG_R11];
  ECContext.X19 = State.gregs[FEXCore::X86State::REG_R12];
  ECContext.X20 = State.gregs[FEXCore::X86State::REG_R13];
  ECContext.X21 = State.gregs[FEXCore::X86State::REG_R14];
  ECContext.X22 = State.gregs[FEXCore::X86State::REG_R15];

  ECContext.Pc = State.rip;

  CTX->ReconstructXMMRegisters(Thread, reinterpret_cast<__uint128_t*>(&ECContext.V[0]), reinterpret_cast<__uint128_t*>(&ECContext.V[16]));

  ECContext.Lr = State.mm[0][0];
  ECContext.X6 = State.mm[1][0];
  ECContext.X7 = State.mm[2][0];
  ECContext.X9 = State.mm[3][0];
  ECContext.X16 = (State.mm[3][1] & 0xffff) << 48 | (State.mm[2][1] & 0xffff) << 32 | (State.mm[1][1] & 0xffff) << 16 | (State.mm[0][1] & 0xffff);
  ECContext.X10 = State.mm[4][0];
  ECContext.X11 = State.mm[5][0];
  ECContext.X12 = State.mm[6][0];
  ECContext.X15 = State.mm[7][0];
  ECContext.X17 = (State.mm[7][1] & 0xffff) << 48 | (State.mm[6][1] & 0xffff) << 32 | (State.mm[5][1] & 0xffff) << 16 | (State.mm[4][1] & 0xffff);

  // Zero all disallowed registers
  ECContext.X13 = 0;
  ECContext.X14 = 0;
  ECContext.X18 = 0;
  ECContext.X23 = 0;
  ECContext.X24 = 0;
  ECContext.X28 = 0;

  // NZCV+SS will be converted into EFlags by ntdll, the rest are lost during exception handling.
  // See HandleGuestException
  uint32_t EFlags = CTX->ReconstructCompactedEFLAGS(Thread, false, nullptr, 0);
  ECContext.Cpsr = 0;
  ECContext.Cpsr |= (EFlags & (1U << FEXCore::X86State::RFLAG_TF_RAW_LOC)) ? (1U << 21) : 0;
  ECContext.Cpsr |= (EFlags & (1U << FEXCore::X86State::RFLAG_OF_RAW_LOC)) ? (1U << 28) : 0;
  ECContext.Cpsr |= (EFlags & (1U << FEXCore::X86State::RFLAG_CF_RAW_LOC)) ? (1U << 29) : 0;
  ECContext.Cpsr |= (EFlags & (1U << FEXCore::X86State::RFLAG_ZF_RAW_LOC)) ? (1U << 30) : 0;
  ECContext.Cpsr |= (EFlags & (1U << FEXCore::X86State::RFLAG_SF_RAW_LOC)) ? (1U << 31) : 0;

  ECContext.Fpcr = FPCR;
  ECContext.Fpsr = FPSR;

  return ECContext;
}

static void RethrowGuestException(const EXCEPTION_RECORD& Rec, ARM64_NT_CONTEXT& Context) {
  const auto& Config = SignalDelegator->GetConfig();
  auto* Thread = GetCPUArea().ThreadState();
  auto& Fault = Thread->CurrentFrame->SynchronousFaultData;
  uint64_t GuestSp = Context.X[Config.SRAGPRMapping[static_cast<size_t>(FEXCore::X86State::REG_RSP)]];
  auto* Args = reinterpret_cast<KiUserExceptionDispatcherStackLayout*>(FEXCore::AlignDown(GuestSp, 64)) - 1;

  LogMan::Msg::DFmt("Reconstructing context");
  if (!IsDispatcherAddress(Context.Pc)) {
    ReconstructThreadState(Thread, Context);
  }
  Args->Context = StoreStateToPackedECContext(Thread, Context.Fpcr, Context.Fpsr);
  LogMan::Msg::DFmt("pc: {:X} rip: {:X}", Context.Pc, Args->Context.Pc);

  // X64 Windows always clears TF, DF and AF when handling an exception, restoring after.
  // Current ARM64EC windows can only restore NZCV+SS when returning from an exception and other flags are left untouched from the handler context.
  // TODO: Can extend wine to support this by mapping the remaining EFlags into reserved cpsr members.
  uint32_t EFlags = CTX->ReconstructCompactedEFLAGS(Thread, false, nullptr, 0);
  EFlags &= ~(1 << FEXCore::X86State::RFLAG_TF_RAW_LOC);
  CTX->SetFlagsFromCompactedEFLAGS(Thread, EFlags);

  Args->Rec = FEX::Windows::HandleGuestException(Fault, Rec, Args->Context.Pc, Args->Context.X8);
  if (Args->Rec.ExceptionCode == EXCEPTION_SINGLE_STEP) {
    Args->Context.Cpsr &= ~(1 << 21); // PSTATE.SS
  } else if (Args->Rec.ExceptionCode == EXCEPTION_BREAKPOINT) {
    // INT3 will set RIP to the instruction following it, undo this (any edge cases with multibyte instructions that trigger breakpoints are bugs present in Windows also)
    Args->Context.Pc -= 1;
  }

  Context.Sp = reinterpret_cast<uint64_t>(Args);
  Context.Pc = KiUserExceptionDispatcher;
}

class ECSyscallHandler : public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
public:
  ECSyscallHandler() {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_GENERIC;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    ProcessPendingCrossProcessEmulatorWork();

    // Manually raise an exeption with the current JIT state packed into a native context, ntdll handles this and
    // reenters the JIT (see dlls/ntdll/signal_arm64ec.c in wine).
    uint64_t FPCR, FPSR;
    __asm volatile("mrs %[fpcr], fpcr" : [fpcr] "=r"(FPCR));
    __asm volatile("mrs %[fpsr], fpsr" : [fpsr] "=r"(FPSR));

    auto* Thread = GetCPUArea().ThreadState();
    KiUserExceptionDispatcherStackLayout DispatchArgs {
      .Context = StoreStateToPackedECContext(Thread, static_cast<uint32_t>(FPCR), static_cast<uint32_t>(FPSR)),
      .Rec = {.ExceptionCode = STATUS_EMULATION_SYSCALL}};
    // PC is expected to hold the return address after the thunk, so skip over the INT 2E/SYSCALL instruction.
    DispatchArgs.Context.Pc += 2;
    JumpSetStack(KiUserExceptionDispatcher, reinterpret_cast<uintptr_t>(&DispatchArgs));
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    return {.NumArgs = 0, .HasReturn = false, .HostSyscallNumber = -1};
  }

  std::optional<FEXCore::ExecutableFileSectionInfo> LookupExecutableFileSection(FEXCore::Core::InternalThreadState&, uint64_t) override {
    return std::nullopt;
  }

  void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override {
    InvalidationTracker->ReprotectRWXIntervals(Start, Length);
  }

  void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override {
    InvalidationTracker->InvalidateAlignedInterval(Start, Length, false);
  }

  void MarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->MarkRange(Start, Length);
  }

  void UnmarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->UnmarkRange(Start, Length);
  }

  FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override {
    return InvalidationTracker->QueryExecutableRange(Address);
  }

  void PreCompile() override {
    ProcessPendingCrossProcessEmulatorWork();
  }
};
} // namespace Exception

extern "C" void SyncThreadContext(CONTEXT* Context) {
  ProcessPendingCrossProcessEmulatorWork();
  auto* Thread = GetCPUArea().ThreadState();
  // All other EFlags bits are lost when converting to/from an ARM64EC context, so merge them in from the current JIT state.
  // This is advisable over dropping their values as thread suspend/resume uses this function, and that can happen at any point in guest code.
  static constexpr uint32_t ECValidEFlagsMask {(1U << FEXCore::X86State::RFLAG_OF_RAW_LOC) | (1U << FEXCore::X86State::RFLAG_CF_RAW_LOC) |
                                               (1U << FEXCore::X86State::RFLAG_ZF_RAW_LOC) | (1U << FEXCore::X86State::RFLAG_SF_RAW_LOC) |
                                               (1U << FEXCore::X86State::RFLAG_TF_RAW_LOC)};

  uint32_t StateEFlags = CTX->ReconstructCompactedEFLAGS(Thread, false, nullptr, 0);
  Context->EFlags = (Context->EFlags & ECValidEFlagsMask) | (StateEFlags & ~ECValidEFlagsMask);
  Exception::LoadStateFromECContext(Thread, *Context);
}

NTSTATUS ProcessInit() {
  InitSyscalls();

  FEX::Windows::InitCRTProcess();
  const auto ExecutablePath = FEX::Windows::GetExecutableFilePath();
  FEX::Config::LoadConfig(ExecutablePath, _environ, FEX::ReadPortabilityInformation());
  FEXCore::Config::ReloadMetaLayer();
  FEX::Windows::Logging::Init();

  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, "1");

  {
    FEX_CONFIG_OPT(TraceProfiler, TRACEPROFILER);
    if (TraceProfiler()) {
      FEXCore::Profiler::Init("", "");
    }
  }

  FEX_CONFIG_OPT(ExtendedVolatileMetadataConfig, EXTENDEDVOLATILEMETADATA);
  ExtendedMetaData = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(ExtendedVolatileMetadataConfig());

  SignalDelegator = fextl::make_unique<FEX::DummyHandlers::DummySignalDelegator>();
  SyscallHandler = fextl::make_unique<Exception::ECSyscallHandler>();
  Exception::HandlerConfig.emplace();

  const auto NtDll = GetModuleHandle("ntdll.dll");
  const bool IsWine = !!GetProcAddress(NtDll, "wine_get_version");
  OvercommitTracker.emplace(IsWine);

  {
    auto HostFeatures = FEX::Windows::CPUFeatures::FetchHostFeatures(IsWine);
    CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);
  }

  CTX->SetSignalDelegator(SignalDelegator.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  CTX->InitCore();

  InvalidationTracker.emplace(*CTX, Threads);

  HandleImageMap(NtDllBase);

  auto MainModule = reinterpret_cast<__TEB*>(NtCurrentTeb())->Peb->ImageBaseAddress;
  HandleImageMap(reinterpret_cast<uint64_t>(MainModule));

  CPUFeatures.emplace(*CTX);

  X64ReturnInstr = ::VirtualAlloc(nullptr, FEXCore::Utils::FEX_PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(X64ReturnInstr), FEXCore::Utils::FEX_PAGE_SIZE,
                                                          PAGE_EXECUTE_READ);
  *reinterpret_cast<uint8_t*>(X64ReturnInstr) = 0xc3;

  const uintptr_t KiUserExceptionDispatcherFFS = reinterpret_cast<uintptr_t>(GetProcAddress(NtDll, "KiUserExceptionDispatcher"));
  Exception::KiUserExceptionDispatcher = NtDllRedirectionLUT[KiUserExceptionDispatcherFFS - NtDllBase] + NtDllBase;

  FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
  if (TSOEnabled()) {
    BOOL Enable = TRUE;
    NTSTATUS Status = NtSetInformationProcess(NtCurrentProcess(), ProcessFexHardwareTso, &Enable, sizeof(Enable));
    if (Status == STATUS_SUCCESS) {
      CTX->SetHardwareTSOSupport(true);
    }
  }

  FEX_CONFIG_OPT(ProfileStats, PROFILESTATS);
  FEX_CONFIG_OPT(StartupSleep, STARTUPSLEEP);
  FEX_CONFIG_OPT(StartupSleepProcName, STARTUPSLEEPPROCNAME);

  if (IsWine && ProfileStats()) {
    StatAllocHandler = fextl::make_unique<FEX::Windows::StatAlloc>(FEXCore::SHMStats::AppType::WIN_ARM64EC);
  }

  if (StartupSleep() && (StartupSleepProcName().empty() || ExecutablePath == StartupSleepProcName())) {
    LogMan::Msg::IFmt("[{}][{}] Sleeping for {} seconds", GetCurrentProcessId(), ExecutablePath, StartupSleep());
    std::this_thread::sleep_for(std::chrono::seconds(StartupSleep()));
  }

  return STATUS_SUCCESS;
}

void ProcessTerm(HANDLE Handle, BOOL After, NTSTATUS Status) {}

class ScopedCallbackDisable {
private:
  bool Prev;

public:
  ScopedCallbackDisable() {
    Prev = GetCPUArea().Area->InSyscallCallback;
    GetCPUArea().Area->InSyscallCallback = true;
  }

  ~ScopedCallbackDisable() {
    GetCPUArea().Area->InSyscallCallback = Prev;
  }
};

// Returns true if exception dispatch should be halted and the execution context restored to NativeContext
bool ResetToConsistentStateImpl(EXCEPTION_RECORD* Exception, CONTEXT* GuestContext, ARM64_NT_CONTEXT* NativeContext) {
  const auto CPUArea = GetCPUArea();
  auto Thread = CPUArea.ThreadState();
  FEXCORE_PROFILE_ACCUMULATION(Thread, AccumulatedSignalTime);
  LogMan::Msg::DFmt("Exception: Code: {:X} Address: {:X}", Exception->ExceptionCode, reinterpret_cast<uintptr_t>(Exception->ExceptionAddress));

  if (NativeContext->Pc == reinterpret_cast<uint64_t>(&ExitFunctionSuspendPoint)) {
    // A suspend interrupt can occur in ExitFunctionEC before InSimulation is unset and set SuspendDoorbell. If this
    // occurs then it is still our duty to cooperatively suspend with an appropriate context. To support this, after
    // unsetting InSimulation a brk #0xCAFE instruction will be raised that we can handle here.
    NativeContext->Pc += 4; // Skip over the brk instruction when we resume
    *CPUArea.Area->SuspendDoorbell = 0;
    return true;
  }

  if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    const auto FaultAddress = static_cast<uint64_t>(Exception->ExceptionInformation[1]);

    if (FEX::Windows::CallRetStack::HandleAccessViolation(Thread, FaultAddress, NativeContext->X17)) {
      return true;
    }

    std::scoped_lock Lock(ThreadCreationMutex);
    if (InvalidationTracker && InvalidationTracker->HandleRWXAccessViolation(Thread, NativeContext->Pc, FaultAddress)) {
      FEXCORE_PROFILE_INSTANT_INCREMENT(Thread, AccumulatedSMCCount, 1);
      if (CTX->IsAddressInCodeBuffer(Thread, NativeContext->Pc) && !CTX->IsCurrentBlockSingleInst(CPUArea.ThreadState()) &&
          CTX->IsAddressInCurrentBlock(Thread, FaultAddress & FEXCore::Utils::FEX_PAGE_MASK, FEXCore::Utils::FEX_PAGE_SIZE)) {
        // If we are not patching ourself (single inst block case) and potentially patching the current block, this is inline SMC. Reconstruct the current context (before the SMC write) then single step the write to reduce it to regular SMC.
        Exception::ReconstructThreadState(Thread, *NativeContext);
        LogMan::Msg::DFmt("Handled inline self-modifying code: pc: {:X} rip: {:X} fault: {:X}", NativeContext->Pc,
                          Thread->CurrentFrame->State.rip, FaultAddress);
        NativeContext->Pc = CPUArea.DispatcherLoopTopEnterECFillSRA();
        NativeContext->Sp = CPUArea.EmulatorStackBase();
        NativeContext->X11 = 1;                                        // Set ENTRY_FILL_SRA_SINGLE_INST_REG to force a single step
        NativeContext->X17 = reinterpret_cast<uint64_t>(CPUArea.Area); // Set EC_ENTRY_CPUAREA_REG
      } else {
        LogMan::Msg::DFmt("Handled self-modifying code: pc: {:X} fault: {:X}", NativeContext->Pc, FaultAddress);
      }

      return true;
    }
  }

  if (!CTX->IsAddressInCodeBuffer(Thread, NativeContext->Pc) && !IsDispatcherAddress(NativeContext->Pc)) {
    LogMan::Msg::DFmt("Passing through exception");
    return false;
  }

  if (Exception->ExceptionCode == EXCEPTION_DATATYPE_MISALIGNMENT && Exception::HandleUnalignedAccess(*NativeContext)) {
    LogMan::Msg::DFmt("Handled unaligned atomic: new pc: {:X}", NativeContext->Pc);
    return true;
  }

  // The JIT (in CompileBlock) emits code to check the suspend doorbell at the start of every block, and run the following instruction if it is set:
  static constexpr uint32_t SuspendTrapMagic {0xD4395FC0}; // brk #0xCAFE
  if (Exception->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION && *reinterpret_cast<uint32_t*>(NativeContext->Pc) == SuspendTrapMagic) {
    Exception::ReconstructThreadState(Thread, *NativeContext);
    *NativeContext = Exception::StoreStateToPackedECContext(Thread, NativeContext->Fpcr, NativeContext->Fpsr);
    LogMan::Msg::DFmt("Suspending: RIP: {:X} SP: {:X}", NativeContext->Pc, NativeContext->Sp);
    CPUArea.Area->InSimulation = 0;
    *CPUArea.Area->SuspendDoorbell = 0;
    return true;
  }

  if (IsEmulatorStackAddress(reinterpret_cast<uint64_t>(__builtin_frame_address(0)))) {
    Exception::RethrowGuestException(*Exception, *NativeContext);
    LogMan::Msg::DFmt("Rethrowing onto guest stack: {:X}", NativeContext->Sp);
    return true;
  } else {
    LogMan::Msg::EFmt("Unexpected exception in JIT code on guest stack");
    return false;
  }
}

NTSTATUS ResetToConsistentState(EXCEPTION_RECORD* Exception, CONTEXT* GuestContext, ARM64_NT_CONTEXT* NativeContext) {
  bool Cont {};
  if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    const auto FaultAddress = static_cast<uint64_t>(Exception->ExceptionInformation[1]);

    if (OvercommitTracker) {
      {
        ScopedCallbackDisable guard;
        Cont = OvercommitTracker->HandleAccessViolation(FaultAddress);
      }
      if (Cont) {
        NtContinueNative(NativeContext, false);
      }
    }
  }

  if (!GetCPUArea().ThreadState()) {
    return STATUS_SUCCESS;
  }

  {

    ScopedCallbackDisable guard;
    Cont = ResetToConsistentStateImpl(Exception, GuestContext, NativeContext);
  }

  if (Cont) {
    NtContinueNative(NativeContext, false);
  }

  GetCPUArea().Area->InSimulation = false;
  GetCPUArea().Area->InSyscallCallback = false;
  return STATUS_SUCCESS;
}

void NotifyMemoryAlloc(void* Address, SIZE_T Size, ULONG Type, ULONG Prot, BOOL After, NTSTATUS Status) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  if (!After) {
    ThreadCreationMutex.lock();
  } else {
    // MEM_RESET(_UNDO) ignores the passed permissions
    if (!Status && !(Type & (MEM_RESET | MEM_RESET_UNDO))) {
      InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), Prot);
    }
    ThreadCreationMutex.unlock();
  }
}

void NotifyMemoryFree(void* Address, SIZE_T Size, ULONG FreeType, BOOL After, NTSTATUS Status) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  if (!After) {
    ThreadCreationMutex.lock();
    if (FreeType & MEM_DECOMMIT) {
      InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), true);
    } else if (FreeType & MEM_RELEASE) {
      InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
    }
  } else {
    ThreadCreationMutex.unlock();
  }
}

void NotifyMemoryProtect(void* Address, SIZE_T Size, ULONG NewProt, BOOL After, NTSTATUS Status) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  if (!After) {
    ThreadCreationMutex.lock();
  } else {
    if (!Status) {
      InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), NewProt);
    }
    ThreadCreationMutex.unlock();
  }
}

NTSTATUS NotifyMapViewOfSection(void* Unk1, void* Address, void* Unk2, SIZE_T Size, ULONG AllocType, ULONG Prot) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return STATUS_SUCCESS;
  }

  {
    std::scoped_lock Lock(ThreadCreationMutex);
    HandleImageMap(reinterpret_cast<uint64_t>(Address));
  }


  return STATUS_SUCCESS;
}

void NotifyUnmapViewOfSection(void* Address, BOOL After, NTSTATUS Status) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  if (!After) {
    ThreadCreationMutex.lock();
    auto [Start, Size] = InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
    if (Size) {
      std::scoped_lock Lock(CTX->GetCodeInvalidationMutex());
      CTX->RemoveForceTSOInformation(Start, Size);
    }
  } else {
    ThreadCreationMutex.unlock();
  }
}

void FlushInstructionCacheHeavy(const void* Address, SIZE_T Size) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), false);
}

void BTCpu64FlushInstructionCache(const void* Address, SIZE_T Size) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), false);
}

void BTCpu64NotifyMemoryDirty(void* Address, SIZE_T Size) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), false);
}

void BTCpu64NotifyReadFile(HANDLE Handle, void* Address, SIZE_T Size, BOOL After, NTSTATUS Status) {}

NTSTATUS ThreadInit() {
  std::scoped_lock Lock(ThreadCreationMutex);
  FEX::Windows::InitCRTThread();
  static constexpr size_t EmulatorStackSize = 0x40000;
  const uint64_t EmulatorStack = reinterpret_cast<uint64_t>(::VirtualAlloc(nullptr, EmulatorStackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
  GetCPUArea().EmulatorStackLimit() = EmulatorStack;
  GetCPUArea().EmulatorStackBase() = EmulatorStack + EmulatorStackSize;

  const auto CPUArea = GetCPUArea();

  auto* Thread = CTX->CreateThread(0, 0);

  // Default segment setup.
  auto Frame = Thread->CurrentFrame;
  auto NewSegments = new FEXCore::Core::CPUState::gdt_segment[32];

  // Setup initial code-segment GDT
  auto& GDT = NewSegments[FEXCore::Core::CPUState::DEFAULT_USER_CS];
  FEXCore::Core::CPUState::SetGDTBase(&GDT, 0);
  FEXCore::Core::CPUState::SetGDTLimit(&GDT, 0xF'FFFFU);
  GDT.L = 1; // L = Long Mode = 64-bit
  GDT.D = 0; // D = Default Operand SIze = Reserved

  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT] = &NewSegments[0];
  // TODO: LDTs are currently unsupported, mirror them to GDT.
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = &NewSegments[0];

  Frame->State.cs_idx = FEXCore::Core::CPUState::DEFAULT_USER_CS << 3;
  Frame->State.cs_cached = FEXCore::Core::CPUState::CalculateGDTBase(GDT);

  FEX::Windows::CallRetStack::InitializeThread(Thread);
  Thread->CurrentFrame->Pointers.Common.ExitFunctionEC = reinterpret_cast<uintptr_t>(&ExitFunctionEC);
  CPUArea.StateFrame() = Thread->CurrentFrame;

  uint64_t EnterEC = Thread->CurrentFrame->Pointers.Common.DispatcherLoopTopEnterEC;
  CPUArea.DispatcherLoopTopEnterEC() = EnterEC;

  uint64_t EnterECFillSRA = Thread->CurrentFrame->Pointers.Common.DispatcherLoopTopEnterECFillSRA;
  CPUArea.DispatcherLoopTopEnterECFillSRA() = EnterECFillSRA;

  CPUArea.ContextAmd64() = {.ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT,
                            .AMD64_SegCs = (FEXCore::Core::CPUState::DEFAULT_USER_CS << 3) | 3,
                            .AMD64_SegDs = 0x2b,
                            .AMD64_SegEs = 0x2b,
                            .AMD64_SegFs = 0x53,
                            .AMD64_SegGs = 0x2b,
                            .AMD64_SegSs = 0x2b,
                            .AMD64_EFlags = 0x202,
                            .AMD64_MxCsr = 0x1f80,
                            .AMD64_MxCsr_copy = 0x1f80,
                            .AMD64_ControlWord = 0x27f};
  Exception::LoadStateFromECContext(Thread, CPUArea.ContextAmd64().AMD64_Context);

  {
    auto ThreadTID = GetCurrentThreadId();
    Threads.emplace(ThreadTID, Thread);
    if (StatAllocHandler) {
      Thread->ThreadStats = StatAllocHandler->AllocateSlot(ThreadTID);
    }
  }

  CPUArea.ThreadState() = Thread;
  CPUArea.Area->SuspendDoorbell = reinterpret_cast<ULONG*>(&Thread->CurrentFrame->SuspendDoorbell);
  return STATUS_SUCCESS;
}

NTSTATUS ThreadTerm(HANDLE Thread, LONG ExitCode) {
  if (!FEX::Windows::ValidateHandleAccess(Thread, THREAD_TERMINATE)) {
    return STATUS_ACCESS_DENIED;
  }

  auto ThreadDup = FEX::Windows::DupHandle(Thread, THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME);

  THREAD_BASIC_INFORMATION Info;
  if (auto Err = NtQueryInformationThread(*ThreadDup, ThreadBasicInformation, &Info, sizeof(Info), nullptr); Err) {
    return Err;
  }

  const auto ThreadTID = reinterpret_cast<uint64_t>(Info.ClientId.UniqueThread);
  bool Self = ThreadTID == GetCurrentThreadId();
  if (!Self) {
    CONTEXT TmpContext;
    // If we are suspending a thread that isn't ourselves, try to suspend it first so we know internal JIT locks aren't being held.
    NtSuspendThread(*ThreadDup, NULL);
    // This will wait for the thread to be suspended
    NtGetContextThread(*ThreadDup, &TmpContext);
  }

  const auto [Err, CPUArea] = GetThreadCPUArea(*ThreadDup);
  if (Err) {
    return Err;
  }

  {
    std::scoped_lock Lock(ThreadCreationMutex);
    auto it = Threads.find(ThreadTID);
    if (it == Threads.end()) {
      // Thread already terminated
      return STATUS_SUCCESS;
    }

    Threads.erase(it);
    if (StatAllocHandler) {
      StatAllocHandler->DeallocateSlot(CPUArea.ThreadState()->ThreadStats);
    }
  }
  auto ThreadState = CPUArea.ThreadState();

  // GDT and LDT are mirrored, only free one.
  delete[] ThreadState->CurrentFrame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT];

  FEX::Windows::CallRetStack::DestroyThread(ThreadState);
  CTX->DestroyThread(ThreadState);
  ::VirtualFree(reinterpret_cast<void*>(CPUArea.EmulatorStackLimit()), 0, MEM_RELEASE);
  if (ThreadTID == GetCurrentThreadId()) {
    FEX::Windows::DeinitCRTThread();
  }
  return STATUS_SUCCESS;
}

BOOLEAN BTCpu64IsProcessorFeaturePresent(UINT Feature) {
  return CPUFeatures->IsFeaturePresent(Feature) ? TRUE : FALSE;
}

void UpdateProcessorInformation(SYSTEM_CPU_INFORMATION* Info) {
  CPUFeatures->UpdateInformation(Info);
}
