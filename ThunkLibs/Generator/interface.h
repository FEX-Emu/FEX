#include <clang/Frontend/FrontendAction.h>

#include <optional>
#include <string>

struct OutputFilenames {
    // Host
    std::string function_unpacks;
    std::string tab_function_unpacks;
    std::string ldr;
    std::string ldr_ptrs;

    // Guest
    std::string thunks;
    std::string function_packs;
    std::string function_packs_public;
    std::string callback_unpacks_header_init;

    // Guest + Host
    std::string callback_structs;
    std::string callback_typedefs;
    std::string callback_unpacks_header;
    std::string callback_unpacks;
};

class FrontendAction : public clang::ASTFrontendAction {
public:
    FrontendAction(const std::string& libname, const OutputFilenames&);

    void EndSourceFileAction() override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

private:
    const std::string& libname;
    const OutputFilenames& output_filenames;
};
