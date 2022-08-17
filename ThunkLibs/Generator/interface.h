#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include "clang/Frontend/CompilerInstance.h"

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

    // Guest + Host
    std::string symbol_list;
};

static bool ParseArg(const std::string &Name, const std::string &Arg, std::string &out) {
  if (!Arg.starts_with(Name + "="))
    return false;
  out = Arg.substr(Name.length() + 1);
  return true;
}

class GenerateThunkLibsAction : public clang::PluginASTAction {
public:
    GenerateThunkLibsAction() { }

    GenerateThunkLibsAction(const std::string& libname, const OutputFilenames&);

    void EndSourceFileAction() override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

protected:
  bool ParseArgs(const clang::CompilerInstance &CI, const std::vector<std::string> &args) override;

  PluginASTAction::ActionType getActionType() override {
    return ReplaceAction;
  }
private:
    std::string libfilename;
    std::string libname; // sanitized filename, usable as part of emitted function names
    std::string outfile;
    OutputFilenames output_filenames;
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
