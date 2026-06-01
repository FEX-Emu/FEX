// SPDX-License-Identifier: MIT
#ifndef _WIN32
#include "../FEXInterpreter/ELFCodeLoader.h"
#endif
#include <DummyHandlers.h>
#ifndef _WIN32
#include <PortabilityInfo.h>
#include <Thunks.h>
#endif

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SourcecodeResolver.h>

#include <Common/ArgumentLoader.h>
#include <Common/Config.h>
#include <Common/FEXServerClient.h>
#include <Common/HostFeatures.h>

#include <OptionParser.h>

#ifndef _WIN32
#include <elf.h>
#endif
#include <fmt/printf.h>
#include <libgen.h>

#include <fcntl.h>
#include <fstream>
#include <optional>
#include <ranges>

#ifndef _WIN32
#include <sys/mman.h>
#else
#include <Common/CPUFeatures.h>
#include <Common/Handle.h>
#include <Common/ImageTracker.h>
#include <Common/InvalidationTracker.h>
#include <Common/JITGuardPage.h>
#include <Common/Logging.h>
#include <Common/Module.h>
#include <Common/OvercommitTracker.h>
#include <Common/PortabilityInfo.h>

static std::unique_ptr<FEX::Windows::OvercommitTracker> OvercommitTracker;
#endif
static FEXCore::Core::InternalThreadState* Thread = nullptr;

#ifdef _WIN32
class AOTSyscallHandler : public FEXCore::HLE::SyscallHandler {
#else
class AOTSyscallHandler : public FEXCore::HLE::SyscallHandler, public FEX::HLE::SyscallMmapInterface {
#endif
public:
  AOTSyscallHandler(FEXCore::Context::Context& CTX, FEXCore::HLE::SyscallOSABI SyscallOSABI)
    : CTX(CTX) {
    OSABI = SyscallOSABI;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    // Don't do anything
    return 0;
  }

  FEXCore::Context::Context& CTX;
#ifdef _WIN32
  FEX::Windows::ImageTracker ImageTracker {CTX, true};
  const std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*> ThreadsUnused;
  FEX::Windows::InvalidationTracker InvalidationTracker {CTX, ThreadsUnused};
#else
  FEXCore::ExecutableFileInfo FileInfo;
  std::map<uint64_t, uint64_t> FileRanges;
#endif
  uintptr_t VAFileStart = 0;

  // These are no-ops implementations of the SyscallHandler API
  std::optional<FEXCore::ExecutableFileSectionInfo> LookupExecutableFileSection(FEXCore::Core::InternalThreadState*, uint64_t Address) override {
#ifndef _WIN32
    auto It = FileRanges.upper_bound(Address - VAFileStart);
    LOGMAN_THROW_A_FMT(It != FileRanges.begin(), "Could not find associated file mapping");
    --It;
    LOGMAN_THROW_A_FMT(VAFileStart + It->first + It->second > Address, "Could not find associated file mapping for {:#x}", Address);
    return FEXCore::ExecutableFileSectionInfo {FileInfo, VAFileStart, VAFileStart + It->first, VAFileStart + It->first + It->second};
#else
    return ImageTracker.LookupExecutableFileSection(Address);
#endif
  }

  FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override {
#ifndef _WIN32
    return {0, UINT64_MAX, true};
#else
    return InvalidationTracker.QueryExecutableRange(Address);
#endif
  }

#ifndef _WIN32
  void* GuestMmap(FEXCore::Core::InternalThreadState*, void* addr, size_t Size, int prot, int Flags, int fd, off_t offset) override {
    // Force writeable to allow applying relocations
    auto Ret = mmap(addr, Size, prot | PROT_WRITE, Flags, fd, offset);
    if (Ret != MAP_FAILED && VAFileStart == 0) {
      VAFileStart = reinterpret_cast<uintptr_t>(Ret);
    }
    FileRanges[reinterpret_cast<uintptr_t>(Ret) - VAFileStart] = Size;
    return Ret;
  }

  uint64_t GuestMunmap(FEXCore::Core::InternalThreadState*, void* addr, uint64_t length) override {
    return munmap(addr, length);
  }
#else
  virtual void MarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->MarkRange(Start, Length);
  }
  virtual void UnmarkOvercommitRange(uint64_t Start, uint64_t Length) override {
    OvercommitTracker->UnmarkRange(Start, Length);
  }
#endif
};

static void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  fmt::print("[{}] {}\n", LogMan::DebugLevelStr(Level), Message);
}

static void AssertHandler(const char* Message) {
  fmt::print("[A] {}\n", Message);
}

namespace FEXCore {
inline bool operator<(const ExecutableFileInfo& a, const ExecutableFileInfo& b) noexcept {
  return a.FileId < b.FileId;
}
} // namespace FEXCore

template<>
struct std::hash<FEXCore::ExecutableFileInfo> {
  std::size_t operator()(const FEXCore::ExecutableFileInfo& Val) const noexcept {
    return Val.FileId;
  }
};

// Windows requires O_BINARY, whereas on Linux it's implicit
#ifndef O_BINARY
#define O_BINARY 0
#endif

// Placeholder data to ensure the compile thread doesn't de-reference nullptr data
static FEXCore::Core::CPUState::gdt_segment gdt[32] {};
#if !defined(_WIN32) || defined(_M_ARM64EC)
static constexpr size_t DefaultCS {FEXCore::Core::CPUState::DEFAULT_USER_CS};
#else
static constexpr size_t DefaultCS {4};
#endif

static FEXCore::Core::InternalThreadState* SetupCompileThread(FEXCore::Context::Context& CTX, bool Is64Bit) {
  auto Thread = CTX.CreateThread(0, 0);

  auto Frame = Thread->CurrentFrame;
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT] = &gdt[0];
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = &gdt[0];
  Frame->State.cs_idx = DefaultCS << 3;
  auto GDT = FEXCore::Core::CPUState::GetSegmentFromIndex(Frame->State, Frame->State.cs_idx);
  FEXCore::Core::CPUState::SetGDTBase(GDT, 0);
  FEXCore::Core::CPUState::SetGDTLimit(GDT, 0xFFFFFU);
  Frame->State.cs_cached = FEXCore::Core::CPUState::CalculateGDTBase(*GDT);

  if (Is64Bit) {
    GDT->L = 1; // L = Long Mode = 64-bit
    GDT->D = 0; // D = Default Operand SIze = Reserved
  } else {
    GDT->L = 0; // L = Long Mode = 32-bit
    GDT->D = 1; // D = Default Operand Size = 32-bit
  }

  return Thread;
}

#ifdef _WIN32
static bool RelocateMappedImage(HMODULE Module) {
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
static void* MapView(HANDLE SectionHandle) {
  return MapViewOfFile(SectionHandle, FILE_MAP_EXECUTE | FILE_MAP_READ, 0, 0, 0);
}
#else
static void* MapView(HANDLE SectionHandle) {
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

// Returns the base address of the mapped image
static std::optional<uint64_t> TryMapImage(FEX::Windows::InvalidationTracker& InvalidationTracker, FEX::Windows::ImageTracker& ImageTracker,
                                           const FEXCore::CodeMapFileId& ID, FEXCore::ExecutableFileInfo& Info) {
  {
    FEX::Windows::ScopedHandle File {CreateFileA(Info.Filename.c_str(), GENERIC_READ | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_DELETE,
                                                 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (!File) {
      LogMan::Msg::EFmt("Couldn't find image: {}", Info.Filename);
      return std::nullopt;
    }

    FEX::Windows::ScopedHandle Section {CreateFileMappingA(*File, nullptr, SEC_IMAGE | PAGE_EXECUTE_READ, 0, 0, nullptr)};
    if (!Section) {
      LogMan::Msg::EFmt("Couldn't create section for image: {}", Info.Filename);
      return std::nullopt;
    }

    void* Mapping = MapView(*Section);
    if (!Mapping) {
      LogMan::Msg::EFmt("Couldn't map section for image: {}", Info.Filename);
      return std::nullopt;
    }

    if (!RelocateMappedImage(reinterpret_cast<HMODULE>(Mapping))) {
      LogMan::Msg::EFmt("Failed to apply image relocations");
      UnmapViewOfFile(Mapping);
      return std::nullopt;
    }

    uint64_t BaseAddress = reinterpret_cast<uint64_t>(Mapping);
    LogMan::Msg::IFmt("Mapped image: {} @ {:X}", Info.Filename, BaseAddress);

    InvalidationTracker.HandleImageMap(FEX::Windows::BaseName(Info.Filename), BaseAddress);
    ImageTracker.HandleImageMap(Info.Filename, BaseAddress, false /* unused during cache generation */);

    return BaseAddress;
  }
}

static LONG ExceptionHandler(_EXCEPTION_POINTERS* ExceptionInfo) {
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
    if (FEX::Windows::JITGuardPage::HandleJITGuardPage(Thread, reinterpret_cast<void*>(FaultAddress), Context->X,
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

struct winsize {
  int ws_col;
};
#endif

// Returns filename of generated cache on success
static std::optional<std::string> GenerateSingleCache(FEXCore::ExecutableFileInfo& Binary, uint64_t CodeCacheConfigId,
                                                      fextl::set<uintptr_t> BlockList, std::string_view OutDir) {
#ifndef _WIN32
  ELFCodeLoader Loader(Binary.Filename.c_str(), -1, "", fextl::vector<fextl::string> {Binary.Filename.c_str()},
                       fextl::vector<fextl::string> {}, nullptr, nullptr, true /* skip interpreter */);
  if (!Loader.ELFWasLoaded()) {
    fmt::print("Invalid or unsupported ELF file.\n");
    return std::nullopt;
  }
  const bool Is64Bit = Loader.Is64BitMode();
#elif defined(_M_ARM64EC)
  const bool Is64Bit = true;
#else
  const bool Is64Bit = false;
#endif
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Is64Bit ? "1" : "0");

  // Load HostFeatures
  auto HostFeatures = FEX::FetchHostFeatures();

  auto CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);
  CTX->GetCodeCache().InitiateCacheGeneration();

#ifdef _WIN32
  const auto NtDll = GetModuleHandle("ntdll.dll");
  const bool IsWine = !!GetProcAddress(NtDll, "wine_get_version");
  OvercommitTracker = std::make_unique<FEX::Windows::OvercommitTracker>(IsWine);

  auto SyscallOSABI = Is64Bit ? FEXCore::HLE::SyscallOSABI::OS_LINUX64 : FEXCore::HLE::SyscallOSABI::OS_LINUX32;
  auto SyscallHandler = std::make_unique<AOTSyscallHandler>(*CTX, SyscallOSABI);

  SyscallHandler->VAFileStart =
    TryMapImage(SyscallHandler->InvalidationTracker, SyscallHandler->ImageTracker, Binary.FileId, Binary).value_or(0);
  if (!SyscallHandler->VAFileStart) {
    return std::nullopt;
  }

  // Register exception handler for OvercommitTracker
  AddVectoredExceptionHandler(1, ExceptionHandler);
#else
  Loader.CalculateHWCaps(CTX.get());

  auto SyscallOSABI = Is64Bit ? FEXCore::HLE::SyscallOSABI::OS_LINUX64 : FEXCore::HLE::SyscallOSABI::OS_LINUX32;
  auto SyscallHandler = std::make_unique<AOTSyscallHandler>(*CTX, SyscallOSABI);

  // Populate relocations from ELF file
  {
    ELFParser RelocParser;
    RelocParser.ReadElf(Binary.Filename);
    Binary.Relocations = RelocParser.PopulateRelocations();
    SyscallHandler->FileInfo.Relocations = Binary.Relocations;
  }

  if (!Is64Bit) {
    const auto PageSize = sysconf(_SC_PAGESIZE);
    // Block upper address space
    FEXCore::Allocator::SetupHooks(PageSize > 0 ? PageSize : FEXCore::Utils::FEX_PAGE_SIZE);
  }
#endif

  if (!std::filesystem::exists(Binary.Filename)) {
    fmt::print("File {} does not exist\n", Binary.Filename);
    // TODO: Pressure vessel hits this
    return /*EXIT_FAILURE*/ std::nullopt;
  }

  auto SignalDelegation = std::make_unique<FEX::DummyHandlers::DummySignalDelegator>();
  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
#ifndef _WIN32
  auto ThunkHandler = FEX::HLE::CreateThunkHandler();
  CTX->SetThunkHandler(ThunkHandler.get());
#endif

  if (!CTX->InitCore()) {
    return std::nullopt;
  }

  Thread = SetupCompileThread(*CTX, Is64Bit);

#ifndef _WIN32
  {
    auto ElfBase = Loader.LoadMainElfFile(nullptr, SyscallHandler.get(), Thread);
    if (!ElfBase.has_value()) {
      ERROR_AND_DIE_FMT("Failed to load ELF file {} ({})", Binary.Filename, Binary.FileId);
    }

    {
      ELFParser RelocParser;
      RelocParser.ReadElf(Binary.Filename);
      auto relocs32 = RelocParser.ReadRawRelocations32();

      for (auto& reloc : relocs32) {
        if (ELF32_R_TYPE(reloc.r_info) == R_386_RELATIVE) {
          // The FEX-relocation is applied on top of this during cache serialization, so this must be countered
          uint32_t val = *reinterpret_cast<uint32_t*>(SyscallHandler->VAFileStart + reloc.r_offset) + SyscallHandler->VAFileStart;
          memcpy(reinterpret_cast<uint32_t*>(SyscallHandler->VAFileStart + reloc.r_offset), &val, sizeof(val));
        } else if (ELF32_R_TYPE(reloc.r_info) == R_386_32) {
          // The FEX-relocation is applied on top of this during cache serialization, so this must be countered
          uint32_t* orig = reinterpret_cast<uint32_t*>(SyscallHandler->VAFileStart + reloc.r_offset);
          uint32_t val = *orig + reloc.r_addend + SyscallHandler->VAFileStart;
          memcpy(orig, &val, sizeof(val));
        }
      }
    }
  }
#endif

  {
    // Refuse to continue if the block list contains any out-of-bounds blocks.
    // This often indicates a corrupted code map.
    {
      auto [min_val, max_val] = std::ranges::minmax_element(BlockList, std::less {});
      auto MinBound = SyscallHandler->LookupExecutableFileSection(Thread, *min_val + SyscallHandler->VAFileStart);
      auto MaxBound = SyscallHandler->LookupExecutableFileSection(Thread, *max_val + SyscallHandler->VAFileStart);
      LOGMAN_THROW_A_FMT(MinBound && MaxBound, "Cached blocks offsets {:#x}-{:#x} out of bounds for library {} ({:016x} @ {:#x})!",
                         *min_val, *max_val, Binary.Filename, Binary.FileId, SyscallHandler->VAFileStart);
    }

    fmt::print(stderr, "Compiling code...\n");

    FEX_CONFIG_OPT(MaxInst, MAXINST);
    for (auto Addr : BlockList) {
      if (!CTX->CheckIfBlockIsCacheable(*Thread, Addr + SyscallHandler->VAFileStart, MaxInst)) {
        continue;
      }

      CTX->CompileRIP(Thread, Addr + SyscallHandler->VAFileStart);
    }

    auto Filename = fmt::format("{}{}-{:016x}", OutDir, FEXCore::CodeMap::GetBaseFilename(Binary, false), CodeCacheConfigId);
    auto FilenameNew = Filename + ".new";
    int fd = open(FilenameNew.c_str(), O_CREAT | O_WRONLY | O_BINARY, 0644);
    {
      auto Entry = SyscallHandler->LookupExecutableFileSection(Thread, SyscallHandler->VAFileStart).value();
#ifndef _WIN32
      CTX->GetCodeCache().SaveData(*Thread, fd, Entry, 0 /* TODO: Use static base address information if available */);
#else
      CTX->GetCodeCache().SaveData(*Thread, fd, Entry, SyscallHandler->VAFileStart);
#endif
    }
    close(fd);
    std::filesystem::rename(FilenameNew.c_str(), Filename.c_str());
    return Filename;
  }
}

// Command handler that parses the given code map and generates a code cache for the selected x86 binary.
// If no binary is selected explicitly, it is inferred from the code map ExecutableFileId block.
static int GenerateCache(int argc, const char** argv) {
  optparse::OptionParser Parser {};
  Parser.add_option("--outdir").set_default(FEX::Config::GetCacheDirectory() + "cache").help("Output directory for generated cache files");
  Parser.add_option("--fileid").help("Select binary to generate cache for");

  optparse::Values Options = Parser.parse_args(argc, argv);
  if (Parser.args().size() != 1) {
    Parser.print_usage();
    return 1;
  }
  const fextl::string CodeMapPath = Parser.args()[0];

  std::ifstream Codemap(CodeMapPath.c_str(), std::ios_base::binary);
  if (!Codemap) {
    fmt::print("Could not open {}\n", CodeMapPath);
    return 1;
  }

  FEXCore::ExecutableFileInfo ProgramName;
  std::map<FEXCore::ExecutableFileInfo, fextl::set<uintptr_t>> Data;
  {
    auto Parsed = FEXCore::CodeMap::ParseCodeMap(Codemap);

    // If an explicit file id is selected, use it.
    // Otherwise, fall back to an IsExecutable marker (or pick the first entry if there's only one)
    auto ExplicitFileId = strtoull(((fextl::string)Options.get("fileid")).data(), nullptr, 16);
    if (ExplicitFileId) {
      ProgramName.FileId = ExplicitFileId;
      ProgramName.Filename = Parsed.at(ExplicitFileId).Filename;
    }

    for (auto& [FileId, Contents] : Parsed) {
      if (!ExplicitFileId && (Contents.IsExecutable || Parsed.size() == 1)) {
        ProgramName.FileId = FileId;
        ProgramName.Filename = Contents.Filename;
      }
      Data.emplace(std::piecewise_construct, std::forward_as_tuple(nullptr, FileId, std::move(Contents.Filename)),
                   std::forward_as_tuple(std::move(Contents.Blocks)));
    }
  }
  if (!ProgramName.FileId) {
    fmt::print("Cannot generate cache from unsanitized code map {}", CodeMapPath);
    return 1;
  }

  for (auto& [File, Blocks] : Data) {
    if (!Blocks.empty()) {
      fmt::print("Parsed {} codemap entries for {} ({:016x})\n", Blocks.size(), File.Filename, File.FileId);
    } else {
      fmt::print("Found dependency {} ({:016x})\n", File.Filename, File.FileId);
    }
  }

  if (!Data.contains(ProgramName)) {
    throw std::runtime_error(fmt::format("Input code map {} did not contain {} ({:016x})", CodeMapPath, ProgramName.Filename, ProgramName.FileId));
  }

  fextl::string OutDir(Options.get("outdir"));
  if (!OutDir.ends_with('/')) {
    OutDir.push_back('/');
  }
  std::filesystem::create_directories(OutDir);

  const auto PortableInfo = FEX::ReadPortabilityInformation();
  char* envp[] = {nullptr};
  FEXCore::Config::Shutdown();
  FEX::Config::LoadConfig("", envp, PortableInfo);

  auto NumBlocks = Data.at(ProgramName).size();
  auto GeneratedCache = GenerateSingleCache(ProgramName, 0 /* TODO: Config id */, Data.at(ProgramName), OutDir);
  if (GeneratedCache) {
    fmt::print("Successfully populated cache {} ({} blocks) via {}\n\n", GeneratedCache.value(), NumBlocks,
               std::filesystem::path {CodeMapPath}.filename().string());
  }
  return GeneratedCache ? 0 : 1;
}

int main(int argc, char** argv) {
#ifndef _WIN32
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
#else
  FEX::Windows::Logging::Init();
#endif

  std::vector<const char*> Args {argv + 1, argv + argc};
  auto CommandName = std::string {basename(argv[0])} + " " + (argc > 1 ? argv[1] : "");
  if (!Args.empty()) {
    Args[0] = CommandName.c_str();
  }

  if (argc >= 2 && argv[1] == std::string_view {"generate"}) {
    return GenerateCache(argc - 1, Args.data());
  } else {
    fmt::print("Usage: {} <command>\n\n", basename(argv[0]));
    fmt::print("Commands:\n");
    fmt::print("  generate\tTrigger cache generation from combined code map\n");
    return EXIT_FAILURE;
  }
}
