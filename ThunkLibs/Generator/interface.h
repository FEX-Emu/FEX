#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include <optional>
#include <string>

struct OutputFilenames {
    std::string host;
    std::string guest;
};

class GenerateThunkLibsAction : public clang::ASTFrontendAction {
public:
    GenerateThunkLibsAction(const std::string& libname, const OutputFilenames&);

    void EndSourceFileAction() override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

private:
    const std::string& libfilename;
    std::string libname; // sanitized filename, usable as part of emitted function names
    const OutputFilenames& output_filenames;
};

class GenerateThunkLibsActionFactory : public clang::tooling::FrontendActionFactory {
public:
    GenerateThunkLibsActionFactory(std::string_view libname_, OutputFilenames output_filenames_)
        : libname(std::move(libname_)), output_filenames(std::move(output_filenames_)) {
    }

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<GenerateThunkLibsAction>(libname, output_filenames);
    }

private:
    std::string libname;
    OutputFilenames output_filenames;
};
