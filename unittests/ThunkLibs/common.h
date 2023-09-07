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
    TestDiagnosticConsumer(bool silent_) : clang::TextDiagnosticPrinter(llvm::errs(), new clang::DiagnosticOptions), silent(silent_) {

    }

    void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                          const clang::Diagnostic& diag) override {
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

/**
 * Run the given ToolAction on the input code.
 *
 * The "silent" parameter is used to suppress non-fatal diagnostics in tests that expect failure
 */
inline void run_tool(clang::tooling::ToolAction& action, std::string_view code, bool silent = false) {
    const char* memory_filename = "gen_input.cpp";
    auto adjuster = clang::tooling::getClangStripDependencyFileAdjuster();
    std::vector<std::string> args = { "clang-tool", "-fsyntax-only", "-std=c++17", "-Werror", "-I.", memory_filename };

    // Corresponds to the content of GeneratorInterface.h
    const char* common_header_code = R"(namespace fexgen {
struct returns_guest_pointer {};
struct custom_host_impl {};
struct callback_annotation_base { bool prevent_multiple; };
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};
} // namespace fexgen
)";

    llvm::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> overlay_fs(new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem()));
    llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> memory_fs(new llvm::vfs::InMemoryFileSystem);
    overlay_fs->pushOverlay(memory_fs);
    memory_fs->addFile(memory_filename, 0, llvm::MemoryBuffer::getMemBufferCopy(code));
    memory_fs->addFile("thunks_common.h", 0, llvm::MemoryBuffer::getMemBufferCopy(common_header_code));
    llvm::IntrusiveRefCntPtr<clang::FileManager> files(new clang::FileManager(clang::FileSystemOptions(), overlay_fs));

    auto invocation = clang::tooling::ToolInvocation(args, &action, files.get(), std::make_shared<clang::PCHContainerOperations>());

    TestDiagnosticConsumer consumer(silent);
    invocation.setDiagnosticConsumer(&consumer);
    invocation.run();

    if (auto error = consumer.GetFirstError()) {
        throw std::runtime_error(*error);
    }
}

inline void run_tool(std::unique_ptr<clang::tooling::ToolAction> action, std::string_view code, bool silent = false) {
    return run_tool(*action, code, silent);
}
