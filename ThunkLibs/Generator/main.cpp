#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"

#include "llvm/Support/Signals.h"

#include <iostream>
#include <optional>
#include <string>

#include "interface.h"

using namespace clang::tooling;

void print_usage(const char* program_name) {
  std::cerr << "Usage: " << program_name << " <filename> <libname> <gen_target> <output_filename> -- <clang_flags>\n";
}

int main(int argc, char* const argv[]) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

  if (argc < 6) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  // Parse compile flags after "--" (this updates argc to the index of the "--" separator)
  std::string error;
  auto compile_db = FixedCompilationDatabase::loadFromCommandLine(argc, argv, error);
  if (!compile_db) {
    print_usage(argv[0]);
    std::cerr << "\nError: " << error << "\n";
    return EXIT_FAILURE;
  }

  // Process arguments before the "--" separator
  if (argc != 6 && argc != 7) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  char* const* arg = argv + 1;
  const auto filename = *arg++;
  const std::string libname = *arg++;
  const std::string target_abi = *arg++;
  const std::string output_filename = *arg++;
  const std::string x86_rootfs = *arg++;

  OutputFilenames output_filenames;
  if (target_abi == "-host") {
    output_filenames.host = std::move(output_filename);
  } else if (target_abi == "-guest") {
    output_filenames.guest = std::move(output_filename);
  } else {
    std::cerr << "Unrecognized generator target ABI \"" << target_abi << "\"\n";
    return EXIT_FAILURE;
  }

  ClangTool Tool(*compile_db, {filename});
  if (CLANG_RESOURCE_DIR[0] != 0) {
    auto set_resource_directory = [](const clang::tooling::CommandLineArguments& Args, clang::StringRef) {
      clang::tooling::CommandLineArguments AdjustedArgs = Args;
      AdjustedArgs.push_back(std::string {"-resource-dir="} + CLANG_RESOURCE_DIR);
      return AdjustedArgs;
    };
    Tool.appendArgumentsAdjuster(set_resource_directory);
  }

  ClangTool GuestTool = Tool;

  auto append_x86_rootfs_includes = [&x86_rootfs](clang::tooling::CommandLineArguments& Args, const char* triple) {
    if (x86_rootfs == "/") {
      return;
    }

    Args.push_back("--sysroot");
    Args.push_back(x86_rootfs);

    // The dev rootfs is only really needed for the standard library.
    // Other libraries generally don't have platform specific headers.
    Args.push_back("-idirafter");
    Args.push_back("/usr/include/");
  };

  // Analyse data layout for guest ABI
  const bool is_32bit_guest = (argv[6] == std::string_view {"-for-32bit-guest"});
  GuestTool.appendArgumentsAdjuster([&](const clang::tooling::CommandLineArguments& Args, clang::StringRef) {
    clang::tooling::CommandLineArguments AdjustedArgs = Args;
    const char* platform = is_32bit_guest ? "i686-linux-gnu" : "x86_64-linux-gnu";
    if (is_32bit_guest) {
      AdjustedArgs.push_back("-m32");
      AdjustedArgs.push_back("-DIS_32BIT_THUNK");
    }
    AdjustedArgs.push_back("-DGUEST_THUNK_LIBRARY");
    AdjustedArgs.push_back(std::string {"--target="} + platform);
    AdjustedArgs.push_back("-isystem");
    AdjustedArgs.push_back(std::string {"/usr/"} + platform + "/include/");

    append_x86_rootfs_includes(AdjustedArgs, platform);

    return AdjustedArgs;
  });
  auto data_layout_analysis_factory = std::make_unique<AnalyzeDataLayoutActionFactory>();
  GuestTool.run(data_layout_analysis_factory.get());
  auto& data_layout = data_layout_analysis_factory->GetDataLayout();

  // Run generator for target ABI
  Tool.appendArgumentsAdjuster([&](const clang::tooling::CommandLineArguments& Args, clang::StringRef) {
    clang::tooling::CommandLineArguments AdjustedArgs = Args;
    AdjustedArgs.push_back("-DIS_HOST_THUNKGEN_PASS");
    if (target_abi == "-guest") {
      const char* platform = is_32bit_guest ? "i686-linux-gnu" : "x86_64-linux-gnu";
      append_x86_rootfs_includes(AdjustedArgs, platform);
    }

    return AdjustedArgs;
  });
  return Tool.run(std::make_unique<GenerateThunkLibsActionFactory>(std::move(libname), std::move(output_filenames), data_layout).get());
}
