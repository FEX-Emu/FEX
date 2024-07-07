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
#include <FEXCore/Utils/EnumOperators.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>

#include "Common/Config.h"
#include "Common/InvalidationTracker.h"
#include "Common/CPUFeatures.h"
#include "DummyHandlers.h"
#include "BTInterface.h"

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
#include <wine/debug.h>

class ECSyscallHandler;
void* X64ReturnInstr; // See Module.S
extern void* ExitFunctionEC;

struct ThreadCPUArea {
  static constexpr size_t TEBCPUAreaOffset = 0x1788;
  CHPE_V2_CPU_AREA_INFO* Area;

  explicit ThreadCPUArea(_TEB* TEB)
    : Area(*reinterpret_cast<CHPE_V2_CPU_AREA_INFO**>(reinterpret_cast<uintptr_t>(TEB) + TEBCPUAreaOffset)) {}

  uint64_t EmulatorStackLimit() const {
    return Area->EmulatorStackLimit;
  }

  uint64_t EmulatorStackBase() const {
    return Area->EmulatorStackBase;
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
fextl::unique_ptr<ECSyscallHandler> SyscallHandler;
std::optional<FEX::Windows::InvalidationTracker> InvalidationTracker;
std::optional<FEX::Windows::CPUFeatures> CPUFeatures;

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

} // namespace

namespace Logging {
static void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Output = fextl::fmt::format("[{}][{:X}] {}\n", LogMan::DebugLevelStr(Level), GetCurrentThreadId(), Message);
  __wine_dbg_output(Output.c_str());
}

static void AssertHandler(const char* Message) {
  const auto Output = fextl::fmt::format("[ASSERT] {}\n", Message);
  __wine_dbg_output(Output.c_str());
}

static void Init() {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
}
} // namespace Logging

class ECSyscallHandler : public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
public:
  ECSyscallHandler() {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_WIN32;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    return 0;
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

void ProcessInit() {
  Logging::Init();
  FEX::Config::InitializeConfigs();
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEX::Config::CreateGlobalMainLayer());
  FEXCore::Config::AddLayer(FEX::Config::CreateMainLayer());
  FEXCore::Config::Load();
  FEXCore::Config::ReloadMetaLayer();

  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS64BIT_MODE, "1");

  // Not applicable to Windows
  FEXCore::Config::EraseSet(FEXCore::Config::ConfigOption::CONFIG_TSOAUTOMIGRATION, "0");

  FEXCore::Context::InitializeStaticTables(FEXCore::Context::MODE_64BIT);

  SignalDelegator = fextl::make_unique<FEX::DummyHandlers::DummySignalDelegator>();
  SyscallHandler = fextl::make_unique<ECSyscallHandler>();

  CTX = FEXCore::Context::Context::CreateNewContext();
  CTX->SetSignalDelegator(SignalDelegator.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  CTX->InitCore();
  InvalidationTracker.emplace(*CTX, Threads);
  CPUFeatures.emplace(*CTX);

  X64ReturnInstr = ::VirtualAlloc(nullptr, FEXCore::Utils::FEX_PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  *reinterpret_cast<uint8_t*>(X64ReturnInstr) = 0xc3;
}

void ProcessTerm() {}

void NotifyMemoryAlloc(void* Address, SIZE_T Size, ULONG Type, ULONG Prot) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), Prot);
}

void NotifyMemoryFree(void* Address, SIZE_T Size, ULONG FreeType) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  if (!Size) {
    InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
  } else if (FreeType & MEM_DECOMMIT) {
    InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), true);
  }
}

void NotifyMemoryProtect(void* Address, SIZE_T Size, ULONG NewProt) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->HandleMemoryProtectionNotification(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), NewProt);
}

void NotifyUnmapViewOfSection(void* Address) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateContainingSection(reinterpret_cast<uint64_t>(Address), true);
}

void BTCpu64FlushInstructionCache(const void* Address, SIZE_T Size) {
  if (!InvalidationTracker || !GetCPUArea().ThreadState()) {
    return;
  }

  std::scoped_lock Lock(ThreadCreationMutex);
  InvalidationTracker->InvalidateAlignedInterval(reinterpret_cast<uint64_t>(Address), static_cast<uint64_t>(Size), false);
}

NTSTATUS ThreadInit() {
  const auto CPUArea = GetCPUArea();

  auto* Thread = CTX->CreateThread(0, 0);
  Thread->CurrentFrame->Pointers.Common.ExitFunctionEC = reinterpret_cast<uintptr_t>(&ExitFunctionEC);
  CPUArea.StateFrame() = Thread->CurrentFrame;

  uint64_t EnterEC = Thread->CurrentFrame->Pointers.Common.DispatcherLoopTopEnterEC;
  CPUArea.DispatcherLoopTopEnterEC() = EnterEC;

  uint64_t EnterECFillSRA = Thread->CurrentFrame->Pointers.Common.DispatcherLoopTopEnterECFillSRA;
  CPUArea.DispatcherLoopTopEnterECFillSRA() = EnterECFillSRA;

  {
    std::scoped_lock Lock(ThreadCreationMutex);
    Threads.emplace(GetCurrentThreadId(), Thread);
  }

  CPUArea.ThreadState() = Thread;
  return STATUS_SUCCESS;
}

NTSTATUS ThreadTerm(HANDLE Thread) {
  const auto [Err, CPUArea] = GetThreadCPUArea(Thread);
  if (Err) {
    return Err;
  }
  auto* OldThreadState = CPUArea.ThreadState();
  CPUArea.ThreadState() = nullptr;

  {
    THREAD_BASIC_INFORMATION Info;
    if (NTSTATUS Err = NtQueryInformationThread(Thread, ThreadBasicInformation, &Info, sizeof(Info), nullptr); Err) {
      return Err;
    }

    const auto ThreadTID = reinterpret_cast<uint64_t>(Info.ClientId.UniqueThread);
    std::scoped_lock Lock(ThreadCreationMutex);
    Threads.erase(ThreadTID);
  }

  CTX->DestroyThread(OldThreadState);
  return STATUS_SUCCESS;
}

BOOLEAN BTCpu64IsProcessorFeaturePresent(UINT Feature) {
  return CPUFeatures->IsFeaturePresent(Feature) ? TRUE : FALSE;
}

void UpdateProcessorInformation(SYSTEM_CPU_INFORMATION* Info) {
  CPUFeatures->UpdateInformation(Info);
}
