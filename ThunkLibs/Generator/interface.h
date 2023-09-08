#include <clang/Tooling/Tooling.h>

#include <optional>
#include <string>

struct OutputFilenames {
    std::string host;
    std::string guest;
};

class GenerateThunkLibsActionFactory : public clang::tooling::ToolAction {
public:
    GenerateThunkLibsActionFactory(std::string_view libname_, OutputFilenames output_filenames_)
        : libname(std::move(libname_)), output_filenames(std::move(output_filenames_)) {
    }

    bool runInvocation(
        std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files,
        std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
        clang::DiagnosticConsumer *DiagConsumer) override;

private:
    std::string libname;
    OutputFilenames output_filenames;
};
