#include <clang/Tooling/Tooling.h>

#include <optional>
#include <string>

struct OutputFilenames {
    std::string host;
    std::string guest;
};

class AnalyzeDataLayoutActionFactory : public clang::tooling::FrontendActionFactory {
    std::unique_ptr<struct ABI> abi;

public:
    AnalyzeDataLayoutActionFactory();
    ~AnalyzeDataLayoutActionFactory();

    std::unique_ptr<clang::FrontendAction> create() override;

    const ABI& GetDataLayout() {
        return *abi;
    }

    std::unique_ptr<ABI> TakeDataLayout() {
        return std::move(abi);
    }
};

class DataLayoutCompareActionFactory : public clang::tooling::FrontendActionFactory {
    const ABI& abi;

public:
    DataLayoutCompareActionFactory(const ABI&);
    ~DataLayoutCompareActionFactory();

    std::unique_ptr<clang::FrontendAction> create() override;
};

class GenerateThunkLibsActionFactory : public clang::tooling::ToolAction {
public:
    GenerateThunkLibsActionFactory(std::string_view libname_, OutputFilenames output_filenames_, const ABI& abi_)
        : libname(std::move(libname_)), output_filenames(std::move(output_filenames_)), abi(abi_) {
    }

    bool runInvocation(
        std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files,
        std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
        clang::DiagnosticConsumer *DiagConsumer) override;

private:
    std::string libname;
    OutputFilenames output_filenames;
    const ABI& abi;
};
