#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include <optional>
#include <string>

#include "abi.h"

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

    std::string symbol_list;
};

class AnalyzeABIAction : public clang::ASTFrontendAction {
    ABI& abi;

public:
    AnalyzeABIAction(ABI& abi_) : abi(abi_) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

};

class AnalyzeABIActionFactory : public clang::tooling::FrontendActionFactory {
    ABI& abi;

public:
    AnalyzeABIActionFactory(ABI& abi_) : abi(abi_) {
    }

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<AnalyzeABIAction>(abi);
    }
};

class GenerateThunkLibsAction : public clang::ASTFrontendAction {
public:
    GenerateThunkLibsAction(const std::string& libname, const OutputFilenames&, const ABITable&);

    void ExecuteAction() override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

private:
    const std::string& libfilename;
    std::string libname; // sanitized filename, usable as part of emitted function names
    const OutputFilenames& output_filenames;
    const ABITable& abi;
};

class GenerateThunkLibsActionFactory : public clang::tooling::FrontendActionFactory {
public:
    GenerateThunkLibsActionFactory(std::string_view libname_, OutputFilenames output_filenames_, const ABITable& abi_)
        : libname(std::move(libname_)), output_filenames(std::move(output_filenames_)), abi(abi_) {
    }

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<GenerateThunkLibsAction>(libname, output_filenames, abi);
    }

private:
    std::string libname;
    OutputFilenames output_filenames;
    const ABITable& abi;
};
