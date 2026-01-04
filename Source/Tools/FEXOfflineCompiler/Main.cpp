// SPDX-License-Identifier: MIT
#include "../FEXInterpreter/ELFCodeLoader.h"
#include <DummyHandlers.h>
#include <PortabilityInfo.h>
#include <Thunks.h>

#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/HostFeatures.h>

#include <Common/ArgumentLoader.h>
#include <Common/Config.h>
#include <Common/FEXServerClient.h>
#include <Common/HostFeatures.h>

#include <OptionParser.h>

#include <fmt/printf.h>

#include <fstream>
#include <optional>

class AOTSyscallHandler : public FEXCore::HLE::SyscallHandler, public FEX::HLE::SyscallMmapInterface {
public:
  AOTSyscallHandler(FEXCore::HLE::SyscallOSABI SyscallOSABI) {
    OSABI = SyscallOSABI;
  }

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    // Don't do anything
    return 0;
  }

  FEXCore::ExecutableFileInfo FileInfo;
  std::map<uint64_t, uint64_t> FileRanges;

  uintptr_t VAFileStart = 0;

  // These are no-ops implementations of the SyscallHandler API
  std::optional<FEXCore::ExecutableFileSectionInfo> LookupExecutableFileSection(FEXCore::Core::InternalThreadState*, uint64_t Address) override {
    auto It = FileRanges.upper_bound(Address - VAFileStart);
    LOGMAN_THROW_A_FMT(It != FileRanges.begin(), "Could not find associated file mapping");
    --It;
    LOGMAN_THROW_A_FMT(VAFileStart + It->first + It->second > Address, "Could not find associated file mapping for {:#x}", Address);
    return FEXCore::ExecutableFileSectionInfo {FileInfo, VAFileStart, VAFileStart + It->first, VAFileStart + It->first + It->second};
  }

  FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override {
    return {0, UINT64_MAX, true};
  }

  void* GuestMmap(FEXCore::Core::InternalThreadState*, void* addr, size_t Size, int prot, int Flags, int fd, off_t offset) override {
    auto Ret = mmap(addr, Size, prot, Flags, fd, offset);
    if (Ret != MAP_FAILED && VAFileStart == 0) {
      VAFileStart = reinterpret_cast<uintptr_t>(Ret);
    }
    FileRanges[reinterpret_cast<uintptr_t>(Ret) - VAFileStart] = Size;
    return Ret;
  }

  uint64_t GuestMunmap(FEXCore::Core::InternalThreadState*, void* addr, uint64_t length) override {
    return munmap(addr, length);
  }
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

// Placeholder data to ensure the compile thread doesn't de-reference nullptr data
static FEXCore::Core::CPUState::gdt_segment gdt[32] {};

static FEXCore::Core::InternalThreadState* SetupCompileThread(FEXCore::Context::Context& CTX, bool Is64Bit) {
  auto Thread = CTX.CreateThread(0, 0);

  auto Frame = Thread->CurrentFrame;
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT] = &gdt[0];
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = &gdt[0];

  Frame->State.cs_idx = FEXCore::Core::CPUState::DEFAULT_USER_CS << 3;
  auto GDT = FEXCore::Core::CPUState::GetSegmentFromIndex(Frame->State, Frame->State.cs_idx);
  FEXCore::Core::CPUState::SetGDTBase(GDT, 0);
  FEXCore::Core::CPUState::SetGDTLimit(GDT, 0xFFFFFU);
  Frame->State.cs_cached =
    FEXCore::Core::CPUState::CalculateGDTBase(*FEXCore::Core::CPUState::GetSegmentFromIndex(Frame->State, Frame->State.cs_idx));

  if (Is64Bit) {
    GDT->L = 1; // L = Long Mode = 64-bit
    GDT->D = 0; // D = Default Operand SIze = Reserved
  } else {
    GDT->L = 0; // L = Long Mode = 32-bit
    GDT->D = 1; // D = Default Operand Size = 32-bit
  }

  return Thread;
}

// Returns filename of generated cache on success
static std::optional<std::string>
GenerateSingleCache(const FEXCore::ExecutableFileInfo& Binary, fextl::set<uintptr_t> BlockList, std::string_view OutDir) {
  uint64_t CodeCacheConfigId = 0; // TODO: Make unique to active configuration

  ELFCodeLoader Loader(Binary.Filename.c_str(), -1, "", fextl::vector<fextl::string> {Binary.Filename.c_str()},
                       fextl::vector<fextl::string> {}, nullptr, nullptr, true /* skip interpreter */);
  if (!Loader.ELFWasLoaded()) {
    fmt::print("Invalid or unsupported ELF file.\n");
    return std::nullopt;
  }
  const bool Is64Bit = Loader.Is64BitMode();
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Is64Bit ? "1" : "0");

  // Load HostFeatures
  auto HostFeatures = FEX::FetchHostFeatures();

  if (!std::filesystem::exists(Binary.Filename)) {
    fmt::print("File {} does not exist\n", Binary.Filename);
    // TODO: Pressure vessel hits this
    return /*EXIT_FAILURE*/ std::nullopt;
  }

  auto CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);

  auto SignalDelegation = std::make_unique<FEX::DummyHandlers::DummySignalDelegator>();

  auto SyscallOSABI = Is64Bit ? FEXCore::HLE::SyscallOSABI::OS_LINUX64 : FEXCore::HLE::SyscallOSABI::OS_LINUX32;
  auto SyscallHandler = std::make_unique<AOTSyscallHandler>(SyscallOSABI);

  Loader.CalculateHWCaps(CTX.get());

  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  auto ThunkHandler = FEX::HLE::CreateThunkHandler();
  CTX->SetThunkHandler(ThunkHandler.get());

  if (!CTX->InitCore()) {
    return std::nullopt;
  }

  if (!Is64Bit) {
    // Block upper address space
    FEXCore::Allocator::SetupHooks();
  }

  auto Thread = SetupCompileThread(*CTX, Is64Bit);

  {
    auto ElfBase = Loader.LoadMainElfFile(nullptr, SyscallHandler.get(), Thread);
    if (!ElfBase.has_value()) {
      ERROR_AND_DIE_FMT("Failed to load ELF file {} ({})", Binary.Filename, Binary.FileId);
    }
  }

  CTX->GetCodeCache().InitiateCacheGeneration();

  {
    std::vector<std::unique_ptr<ELFCodeLoader>> LoaderMem;

    fmt::print(stderr, "Compiling code...\n");
    for (auto Addr : BlockList) {
      CTX->CompileRIP(Thread, Addr + SyscallHandler->VAFileStart);
    }

    auto Filename = fmt::format("{}{}-{:016x}", OutDir, FEXCore::CodeMap::GetBaseFilename(Binary, false), CodeCacheConfigId);
    auto FilenameNew = Filename + ".new";
    int fd = open(FilenameNew.c_str(), O_CREAT | O_WRONLY, 0644);
    {
      auto Entry = SyscallHandler->LookupExecutableFileSection(Thread, SyscallHandler->VAFileStart).value();
      CTX->GetCodeCache().SaveData(*Thread, fd, Entry, 0 /* TODO: Use static base address information if available */);
    }
    std::filesystem::rename(FilenameNew.c_str(), Filename.c_str());
    close(fd);
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
  FEX::Config::LoadConfig("", envp, PortableInfo);

  auto NumBlocks = Data.at(ProgramName).size();
  auto GeneratedCache = GenerateSingleCache(ProgramName, Data.at(ProgramName), OutDir);
  if (GeneratedCache) {
    fmt::print("Successfully populated cache {} ({} blocks) via {}\n\n", GeneratedCache.value(), NumBlocks,
               std::filesystem::path {CodeMapPath}.filename().string());
  }
  return GeneratedCache ? 0 : 1;
}

int main(int argc, char** argv) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  std::vector<const char*> Args {argv + 1, argv + argc};
  auto CommandName = std::string {basename(argv[0])} + " " + (argc > 1 ? argv[1] : "");
  Args[0] = CommandName.c_str();

  if (argc >= 2 && argv[1] == std::string_view {"generate"}) {
    return GenerateCache(argc - 1, Args.data());
  } else {
    fmt::print("Usage: {} <command>\n\n", basename(argv[0]));
    fmt::print("Commands:\n");
    fmt::print("  generate\tTrigger cache generation from combined code map\n");
    return EXIT_FAILURE;
  }
}
