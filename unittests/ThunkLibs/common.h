#pragma once

#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/raw_os_ostream.h>

#include <optional>

/**
 * Prints diagnostics to console like clang::TextDiagnosticPrinter.
 * A copy of the first error message is stored so that it can be queried
 * after compiling.
 */
class TestDiagnosticConsumer : public clang::TextDiagnosticPrinter {
  bool silent;

  std::optional<std::string> first_error;

public:
#if LLVM_VERSION_MAJOR >= 21
  TestDiagnosticConsumer(bool silent_, clang::DiagnosticOptions& diag_opts)
    : clang::TextDiagnosticPrinter(llvm::errs(), diag_opts)
    , silent(silent_) {}
#else
  TestDiagnosticConsumer(bool silent_)
    : clang::TextDiagnosticPrinter(llvm::errs(), new clang::DiagnosticOptions)
    , silent(silent_) {}
#endif

  void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& diag) override {
    if (level >= clang::DiagnosticsEngine::Error && !first_error) {
      llvm::SmallVector<char, 64> message;
      diag.FormatDiagnostic(message);
      first_error = std::string(message.begin(), message.end());
    }

    if (silent && level != clang::DiagnosticsEngine::Fatal) {
      return;
    }

    clang::TextDiagnosticPrinter::HandleDiagnostic(level, diag);
  }

  std::optional<std::string> GetFirstError() const {
    return first_error;
  }
};

enum class GuestABI {
  X86_32,
  X86_64,
};

inline std::ostream& operator<<(std::ostream& os, GuestABI abi) {
  if (abi == GuestABI::X86_32) {
    os << "X86_32";
  } else if (abi == GuestABI::X86_64) {
    os << "X86_64";
  }
  return os;
}

/**
 * Run the given ToolAction on the input code.
 *
 * The "silent" parameter is used to suppress non-fatal diagnostics in tests that expect failure
 */
inline void
run_tool(clang::tooling::ToolAction& action, std::string_view code, bool silent = false, std::optional<GuestABI> guest_abi = std::nullopt) {
  const char* memory_filename = "gen_input.cpp";
  auto adjuster = clang::tooling::getClangStripDependencyFileAdjuster();
  std::vector<std::string> args = {"clang-tool", "-fsyntax-only", "-std=c++20", "-Werror", "-I.", memory_filename};
  if (CLANG_RESOURCE_DIR[0] != 0) {
    args.push_back("-resource-dir");
    args.push_back(CLANG_RESOURCE_DIR);
  }
  if (guest_abi == GuestABI::X86_64) {
    args.push_back("-target");
    args.push_back("x86_64-linux-gnu");
    args.push_back("-isystem");
    args.push_back("/usr/x86_64-linux-gnu/include/");
  } else if (guest_abi == GuestABI::X86_32) {
    args.push_back("-target");
    args.push_back("i686-linux-gnu");
    args.push_back("-isystem");
    args.push_back("/usr/i686-linux-gnu/include/");
  } else {
    args.push_back("-DHOST");
  }

  // Corresponds to the content of GeneratorInterface.h
  const char* common_header_code = R"(namespace fexgen {
struct returns_guest_pointer {};
struct custom_host_impl {};
struct callback_annotation_base { bool prevent_multiple; };
struct callback_stub : callback_annotation_base {};

struct custom_repack {};
struct emit_layout_wrappers {};

struct opaque_type {};
struct assume_compatible_data_layout {};

struct ptr_passthrough {};

} // namespace fexgen

template<auto, int, typename = void> struct fex_gen_param {};

template<typename>
struct fex_gen_type;
template<auto>
struct fex_gen_config;

)";

  llvm::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> overlay_fs(new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem()));
  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> memory_fs(new llvm::vfs::InMemoryFileSystem);
  overlay_fs->pushOverlay(memory_fs);
  memory_fs->addFile(memory_filename, 0, llvm::MemoryBuffer::getMemBufferCopy(code));
  memory_fs->addFile("thunks_common.h", 0, llvm::MemoryBuffer::getMemBufferCopy(common_header_code));
  llvm::IntrusiveRefCntPtr<clang::FileManager> files(new clang::FileManager(clang::FileSystemOptions(), overlay_fs));

  auto invocation = clang::tooling::ToolInvocation(args, &action, files.get(), std::make_shared<clang::PCHContainerOperations>());

#if LLVM_VERSION_MAJOR >= 21
  clang::DiagnosticOptions diag_opts;
  TestDiagnosticConsumer consumer(silent, diag_opts);
#else
  TestDiagnosticConsumer consumer(silent);
#endif

  invocation.setDiagnosticConsumer(&consumer);

  // Process the actual ToolAction.
  // NOTE: If the ToolAction throws an exception, clang will leak memory here.
  invocation.run();

  if (auto error = consumer.GetFirstError()) {
    throw std::runtime_error(*error);
  }
}

inline void run_tool(std::unique_ptr<clang::tooling::ToolAction> action, std::string_view code, bool silent = false,
                     std::optional<GuestABI> guest_abi = std::nullopt) {
  return run_tool(*action, code, silent, guest_abi);
}
