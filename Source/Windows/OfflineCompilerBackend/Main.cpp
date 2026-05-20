// SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

#include <fcntl.h>
#include <io.h>
#include <fmt/printf.h>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>

#include "Common/ArgumentLoader.h"
#include "Common/Config.h"
#include "Common/JITGuardPage.h"
#include "Common/CPUFeatures.h"
#include "Common/Handle.h"
#include "Common/ImageTracker.h"
#include "Common/InvalidationTracker.h"
#include "Common/Logging.h"
#include "Common/Module.h"
#include "Common/OvercommitTracker.h"
#include "Common/PortabilityInfo.h"

#include "DummyHandlers.h"


namespace {
std::optional<FEX::Windows::InvalidationTracker> InvalidationTracker;
std::optional<FEX::Windows::ImageTracker> ImageTracker;
std::optional<FEX::Windows::OvercommitTracker> OvercommitTracker;
thread_local FEXCore::Core::InternalThreadState* ThisThread {};

struct ImageInfo {
  FEXCore::ExecutableFileInfo Info;
  FEXCore::CodeMap::ParsedContents Contents;
  bool RecompileCode {};

  bool CheckNeedsRecompile(const std::filesystem::path& CodeMapPath, const std::filesystem::path& CachePath) {
    std::error_code ec;
    const auto CodeMapTime = std::filesystem::last_write_time(CodeMapPath, ec);
    if (ec) {
      return true;
    }

    const auto CacheTime = std::filesystem::last_write_time(CachePath, ec);
    if (ec) {
      return true;
    }

    return CodeMapTime > CacheTime;
  }
};

#ifdef _M_ARM64EC
static constexpr size_t DefaultCS {FEXCore::Core::CPUState::DEFAULT_USER_CS};
#else
static constexpr size_t DefaultCS {4};
#endif

static FEXCore::Core::CPUState::gdt_segment GDTSegments[32] {};

static void InitializeGDT() {
  auto& GDT = GDTSegments[DefaultCS];
  FEXCore::Core::CPUState::SetGDTBase(&GDT, 0);
  FEXCore::Core::CPUState::SetGDTLimit(&GDT, 0xF'FFFFU);
#ifdef _M_ARM64EC
  GDT.L = 1;
  GDT.D = 0;
#else
  GDT.L = 0;
  GDT.D = 1;
#endif
}

void InitializeThreadContext(FEXCore::Core::InternalThreadState* ThreadState) {
  auto Frame = ThreadState->CurrentFrame;
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT] = &GDTSegments[0];
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = &GDTSegments[0];
  Frame->State.cs_idx = DefaultCS << 3;
  Frame->State.cs_cached = FEXCore::Core::CPUState::CalculateGDTBase(GDTSegments[DefaultCS]);
}

bool RelocateMappedImage(HMODULE Module) {
  const auto* NtHeaders = reinterpret_cast<FEX::Windows::ArchImageNtHeaders*>(RtlImageNtHeader(Module));
  if (!NtHeaders) {
    return false;
  }

  const auto BaseAddress = reinterpret_cast<uintptr_t>(Module);
  const auto PreferredBase = NtHeaders->OptionalHeader.ImageBase;
  const auto Delta = static_cast<intptr_t>(BaseAddress - PreferredBase);

  // Wine will automatically relocate all DLLs to their mapped address, but PE relocations must still be applied so
  // FEXCore can correctly transform them into FEX relocations
  if (Delta == 0) {
    return true;
  }

  ULONG RelocSize = 0;
  auto* RelocBlock =
    reinterpret_cast<IMAGE_BASE_RELOCATION*>(RtlImageDirectoryEntryToData(Module, true, IMAGE_DIRECTORY_ENTRY_BASERELOC, &RelocSize));

  if (!RelocBlock || RelocSize == 0) {
    return true;
  }

  // Reprotect all sections as RW to apply relocations, saving their prior protections
  struct SectionPatchState {
    void* Address;
    SIZE_T Size;
    DWORD PreviousProtection;
  };
  std::vector<SectionPatchState> SectionStates;
  SectionStates.reserve(NtHeaders->FileHeader.NumberOfSections);

  auto* SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);
  const auto* SectionHeaderEnd = SectionHeader + NtHeaders->FileHeader.NumberOfSections;
  for (; SectionHeader != SectionHeaderEnd; ++SectionHeader) {
    if (SectionHeader->SizeOfRawData == 0) {
      continue;
    }

    const auto SecAddr = reinterpret_cast<void*>(BaseAddress + SectionHeader->VirtualAddress);
    const SIZE_T SecSize = SectionHeader->Misc.VirtualSize;

    DWORD OldProt = 0;
    if (!VirtualProtect(SecAddr, SecSize, PAGE_READWRITE, &OldProt)) {
      for (const auto& State : SectionStates) {
        DWORD Ignored;
        VirtualProtect(State.Address, State.Size, State.PreviousProtection, &Ignored);
      }
      return false;
    }

    SectionStates.push_back({SecAddr, SecSize, OldProt});
  }

  // Apply relocations to all sections
  bool RelocSuccess = true;
  const uintptr_t RelocEnd = reinterpret_cast<uintptr_t>(RelocBlock) + RelocSize;
  const uint32_t ImageSize = NtHeaders->OptionalHeader.SizeOfImage;

  while (reinterpret_cast<uintptr_t>(RelocBlock) < RelocEnd && RelocBlock->SizeOfBlock) {
    if (RelocBlock->VirtualAddress >= ImageSize) {
      RelocSuccess = false;
      break;
    }

    const auto Count = (RelocBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
    const auto PageAddress = BaseAddress + RelocBlock->VirtualAddress;

    RelocBlock = LdrProcessRelocationBlock(PageAddress, Count, reinterpret_cast<USHORT*>(RelocBlock + 1), Delta);

    if (!RelocBlock) {
      RelocSuccess = false;
      break;
    }
  }

  // Restore sections to previous protection states
  for (const auto& State : SectionStates) {
    DWORD Ignored;
    VirtualProtect(State.Address, State.Size, State.PreviousProtection, &Ignored);
  }

  if (!RelocSuccess) {
    return false;
  }

  LogMan::Msg::IFmt("Relocated image {:X} -> {:X}", PreferredBase, BaseAddress);
  return true;
}

#ifdef ARCHITECTURE_arm64ec
void* MapView(HANDLE SectionHandle) {
  return MapViewOfFile(SectionHandle, FILE_MAP_EXECUTE | FILE_MAP_READ, 0, 0, 0);
}
#else
void* MapView(HANDLE SectionHandle) {
  void* BaseAddress = nullptr;
  SIZE_T ViewSize = 0;
  LARGE_INTEGER Offset {};

  // Map images in the lower 32-bits for WOW64 so relocations can be correctly applied
  const ULONG_PTR ZeroBits = 0x7fffffff;

  NTSTATUS Status =
    NtMapViewOfSection(SectionHandle, GetCurrentProcess(), &BaseAddress, ZeroBits, 0, &Offset, &ViewSize, ViewShare, 0, PAGE_EXECUTE_READ);

  if (Status < 0) {
    return nullptr;
  }

  return BaseAddress;
}
#endif

struct MappedImage {
  uint64_t BaseAddress;
  FEXCore::ExecutableFileSectionInfo SectionInfo;
  ImageInfo Info;
};

std::vector<MappedImage> TryMapImages(std::unordered_map<FEXCore::CodeMapFileId, ImageInfo>&& Images) {
  std::vector<MappedImage> Result;

  for (auto& [ID, Info] : Images) {
    if (!Info.RecompileCode || Info.Contents.Blocks.empty()) {
      continue;
    }

    FEX::Windows::ScopedHandle File {CreateFileA(Info.Contents.Filename.c_str(), GENERIC_READ | SYNCHRONIZE,
                                                 FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (!File) {
      LogMan::Msg::EFmt("Couldn't find image: {}", Info.Contents.Filename);
      continue;
    }

    FEX::Windows::ScopedHandle Section {CreateFileMappingA(*File, nullptr, SEC_IMAGE | PAGE_EXECUTE_READ, 0, 0, nullptr)};
    if (!Section) {
      LogMan::Msg::EFmt("Couldn't create section for image: {}", Info.Contents.Filename);
      continue;
    }

    void* Mapping = MapView(*Section);
    if (!Mapping) {
      LogMan::Msg::EFmt("Couldn't map section for image: {}", Info.Contents.Filename);
      continue;
    }

    if (!RelocateMappedImage(reinterpret_cast<HMODULE>(Mapping))) {
      LogMan::Msg::EFmt("Failed to apply image relocations");
      UnmapViewOfFile(Mapping);
      continue;
    }

    uint64_t BaseAddress = reinterpret_cast<uint64_t>(Mapping);
    LogMan::Msg::IFmt("Mapped image: {} @ {:X}", Info.Contents.Filename, BaseAddress);

    InvalidationTracker->HandleImageMap(FEX::Windows::BaseName(Info.Contents.Filename), BaseAddress);
    auto SectionInfo = ImageTracker->HandleImageMap(Info.Contents.Filename, BaseAddress, Info.Contents.IsExecutable);

    Result.push_back(MappedImage {.BaseAddress = BaseAddress, .SectionInfo = SectionInfo, .Info = std::move(Info)});
  }

  return Result;
}

} // namespace

class AOTSyscallHandler : public FEXCore::HLE::SyscallHandler {
public:
  AOTSyscallHandler() {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_GENERIC;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    return 0;
  }

  std::optional<FEXCore::ExecutableFileSectionInfo> LookupExecutableFileSection(FEXCore::Core::InternalThreadState*, uint64_t Address) override {
    return ImageTracker->LookupExecutableFileSection(Address);
  }

  void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override {}

  void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override {}

  void MarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->MarkRange(Start, Length);
  }

  void UnmarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->UnmarkRange(Start, Length);
  }

  FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override {
    return InvalidationTracker->QueryExecutableRange(Address);
  }

  void PreCompile() override {}
};

LONG ExceptionHandler(_EXCEPTION_POINTERS* ExceptionInfo) {
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    const auto FaultAddress = static_cast<uint64_t>(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
    if (OvercommitTracker->HandleAccessViolation(FaultAddress)) {
      return EXCEPTION_CONTINUE_EXECUTION;
    }

#ifdef ARCHITECTURE_arm64ec
    ARM64_NT_CONTEXT ArmContext {};
    auto* Context = &ArmContext;
#else
    auto* Context = ExceptionInfo->ContextRecord;
#endif
    if (FEX::Windows::JITGuardPage::HandleJITGuardPage(ThisThread, reinterpret_cast<void*>(FaultAddress), Context->X,
                                                       reinterpret_cast<__uint128_t*>(Context->V), &Context->Pc)) {
#ifdef ARCHITECTURE_arm64ec
      auto* ECContext = reinterpret_cast<ARM64EC_NT_CONTEXT*>(ExceptionInfo->ContextRecord);
      ECContext->X0 = Context->X0;
      ECContext->X19 = Context->X19;
      ECContext->X20 = Context->X20;
      ECContext->X21 = Context->X21;
      ECContext->X22 = Context->X22;
      ECContext->X25 = Context->X25;
      ECContext->X26 = Context->X26;
      ECContext->X27 = Context->X27;
      ECContext->Fp = Context->Fp;
      ECContext->Lr = Context->Lr;
      ECContext->Sp = Context->Sp;
      ECContext->Pc = Context->Pc;

      for (size_t i = 0; i < 8; ++i) {
        memcpy(&reinterpret_cast<__uint128_t*>(ECContext->V)[8 + i], &reinterpret_cast<__uint128_t*>(Context->V)[8 + i], sizeof(uint64_t));
      }
#endif
      return EXCEPTION_CONTINUE_EXECUTION;
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fmt::print("Usage: {} <main_image_name> <image_cache_dir> <codemap_file>\n", argv[0]);
    return 1;
  }

  fextl::string MainImageName {argv[1]};
  std::filesystem::path ImageCacheDir {argv[2]};
  std::ifstream CodeMap(argv[3], std::ios_base::binary);
  if (!CodeMap) {
    fmt::print("Could not open {}\n", argv[3]);
    return 1;
  }

  FEX::Config::LoadConfig(MainImageName, _environ, FEX::ReadPortabilityInformation());
  FEXCore::Config::ReloadMetaLayer();

  FEX::Windows::Logging::Init();
#ifdef _M_ARM64EC
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, "1");
#else
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, "0");
#endif

  const auto CacheDir = std::filesystem::path(FEX::Config::GetCacheDirectory());
  const auto ReadyDir = CacheDir / "codemap" / "ready";
  const auto MetadataPath = ImageCacheDir / "metadata";

  const auto NtDll = GetModuleHandle("ntdll.dll");
  const bool IsWine = !!GetProcAddress(NtDll, "wine_get_version");

  auto HostFeatures = FEX::Windows::CPUFeatures::FetchHostFeatures(IsWine);

  std::unordered_map<FEXCore::CodeMapFileId, ImageInfo> Images;
  std::unordered_set<std::string> ToPreserve;

  auto Parsed = FEXCore::CodeMap::ParseCodeMap(CodeMap);

  for (auto& [ID, Contents] : Parsed) {
    FEXCore::ExecutableFileInfo FEXInfo {.FileId = ID, .Filename = Contents.Filename};

    const auto BaseFilename = FEXCore::CodeMap::GetBaseFilename(FEXInfo, false);
    const auto LibCodeMapPath = ReadyDir / BaseFilename;
    const auto LibCachePath = ImageCacheDir / BaseFilename;

    // Handle dependencies by loading their specific codemap
    if (!Contents.IsExecutable && Contents.Blocks.empty()) {
      std::ifstream DepCodeMap(LibCodeMapPath, std::ios_base::binary);
      if (!DepCodeMap) {
        fmt::print("Could not open dependency codemap: {}\n", LibCodeMapPath.string());
      } else {
        auto DepParsed = FEXCore::CodeMap::ParseCodeMap(DepCodeMap);
        if (auto DepIt = DepParsed.find(ID); DepIt != DepParsed.end()) {
          Contents = std::move(DepIt->second);
        }
      }
    }

    LogMan::Msg::IFmt("Parsed {} codemap entries for {} ({})", Contents.Blocks.size(), Contents.Filename, ID);

    auto [It, _] = Images.emplace(ID, ImageInfo {.Info = std::move(FEXInfo), .Contents = std::move(Contents), .RecompileCode = false});

    auto& CurrentImage = It->second;
    CurrentImage.RecompileCode = CurrentImage.CheckNeedsRecompile(LibCodeMapPath, LibCachePath);
    ToPreserve.emplace(BaseFilename);
  }

  OvercommitTracker.emplace(IsWine);

  fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> SignalDelegator = fextl::make_unique<FEX::DummyHandlers::DummySignalDelegator>();
  fextl::unique_ptr<AOTSyscallHandler> SyscallHandler = fextl::make_unique<AOTSyscallHandler>();
  fextl::unique_ptr<FEXCore::Context::Context> CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);
  CTX->SetSignalDelegator(SignalDelegator.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  CTX->InitCore();
  CTX->GetCodeCache().InitiateCacheGeneration();

  AddVectoredExceptionHandler(1, ExceptionHandler);

  InitializeGDT();

  std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*> Threads;
  InvalidationTracker.emplace(*CTX, Threads);
  ImageTracker.emplace(*CTX, true);

  ThisThread = CTX->CreateThread(0, 0);
  InitializeThreadContext(ThisThread);

  auto MappedImages = TryMapImages(std::move(Images));

  if (!std::filesystem::exists(ImageCacheDir)) {
    if (!std::filesystem::create_directories(ImageCacheDir)) {
      LogMan::Msg::EFmt("Error creating directory {}", ImageCacheDir.string());
      return 1;
    }
  }

  for (auto& Image : MappedImages) {
    LogMan::Msg::IFmt("Compiling module {}: {} entrypoints", Image.Info.Contents.Filename, Image.Info.Contents.Blocks.size());
    CTX->ClearCodeCache(ThisThread, true);

    for (const auto Block : Image.Info.Contents.Blocks) {
      CTX->CompileRIP(ThisThread, Image.BaseAddress + Block);
    }

    auto Filename = ImageCacheDir / FEXCore::CodeMap::GetBaseFilename(Image.SectionInfo.FileInfo, false);
    auto StagingFilename = Filename;
    StagingFilename += ".new";

    FEX::Windows::ScopedHandle FileHandle {
      CreateFileW(StagingFilename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (FileHandle) {
      FEX::Windows::ScopedHandle MapHandle {};
      void* MappedPtr {};

      FEXCore::Core::InternalThreadState* CompilerThreads[] = {ThisThread};
      bool Success = CTX->GetCodeCache().SaveData(CompilerThreads, Image.SectionInfo, Image.BaseAddress, [&](size_t TotalSize) -> void* {
        LARGE_INTEGER LiSize;
        LiSize.QuadPart = TotalSize;
        if (!SetFilePointerEx(*FileHandle, LiSize, nullptr, FILE_BEGIN)) {
          return nullptr;
        }
        if (!SetEndOfFile(*FileHandle)) {
          return nullptr;
        }
        MapHandle.reset(CreateFileMappingW(*FileHandle, nullptr, PAGE_READWRITE, 0, 0, nullptr));
        if (!MapHandle) {
          return nullptr;
        }
        MappedPtr = MapViewOfFile(*MapHandle, FILE_MAP_WRITE, 0, 0, 0);
        return MappedPtr;
      });

      if (MappedPtr) {
        UnmapViewOfFile(MappedPtr);
      }

      MapHandle.reset();
      FileHandle.reset();
      if (Success) {
        if (!MoveFileExW(StagingFilename.c_str(), Filename.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
          LogMan::Msg::EFmt("Failed to replace cache file {}: {}", Filename.string(), GetLastError());
          std::filesystem::remove(StagingFilename);
        }
      } else {
        std::filesystem::remove(StagingFilename);
        LogMan::Msg::EFmt("Failed to save code cache data");
      }
    } else {
      LogMan::Msg::EFmt("Failed to open output file: {}", StagingFilename.string());
    }
  }

  for (const auto& Entry : std::filesystem::directory_iterator(ImageCacheDir)) {
    if (Entry.is_regular_file()) {
      const auto FileName = Entry.path().filename().string();
      if (ToPreserve.find(FileName) == ToPreserve.end()) {
        std::error_code ec;
        if (std::filesystem::remove(Entry.path(), ec)) {
          LogMan::Msg::IFmt("Deleted stale file: {}", FileName);
        }
      }
    }
  }

  LogMan::Msg::IFmt("Done Compiling");
  return 0;
}
