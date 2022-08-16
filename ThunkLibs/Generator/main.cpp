#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include <iostream>
#include <string>

#include "interface.h"

using namespace clang::tooling;

static llvm::cl::OptionCategory ThunkgenCategory("thunkgen options");

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << "<libname> <gen_target> <output_filename> -- <filename> -- <clang_flags>\n";
}

int main(int argc, const char* argv[]) {
    if (argc < 6) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    auto const last_arg = argv + argc;

    auto arg = argv + 1;
    const std::string libname = *arg++;
    argc--;

    // Parse Commands before "--" (this updates arg to the "--" separator, and replaces it with argv[0])
    // Iterate over generator targets (remaining arguments up to "--" separator)
    OutputFilenames output_filenames;
    while (arg < last_arg) {
        auto target = std::string { *arg++ };
        argc--;
        if (target == "--")
        {
            *--arg = argv[0];
            break;
        }

        auto out_filename = *arg++;
        argc--;

        if (target == "-function_unpacks") {
            output_filenames.function_unpacks = out_filename;
        } else if (target == "-tab_function_unpacks") {
            output_filenames.tab_function_unpacks = out_filename;
        } else if (target == "-ldr") {
            output_filenames.ldr = out_filename;
        } else if (target == "-ldr_ptrs") {
            output_filenames.ldr_ptrs = out_filename;
        } else if (target == "-thunks") {
            output_filenames.thunks = out_filename;
        } else if (target == "-function_packs") {
            output_filenames.function_packs = out_filename;
        } else if (target == "-function_packs_public") {
            output_filenames.function_packs_public = out_filename;
        } else if (target == "-symbol_list") {
            output_filenames.symbol_list = out_filename;
        } else {
            std::cerr << "Unrecognized generator target \"" << target << "\"\n";
            return EXIT_FAILURE;
        }
    }

    // Parse clang tool arguments after the first "--"

    auto OptionsParser = CommonOptionsParser::create(argc, arg, ThunkgenCategory);
    if (!OptionsParser) {
        print_usage(argv[0]);
        llvm::errs() <<  OptionsParser.takeError();
        return EXIT_FAILURE;
    }

    ClangTool Tool(OptionsParser->getCompilations(), OptionsParser->getSourcePathList());

    if (CLANG_RESOURCE_DIR[0] != 0) {
        auto set_resource_directory = [](const clang::tooling::CommandLineArguments &Args, clang::StringRef) {
            clang::tooling::CommandLineArguments AdjustedArgs = Args;
            AdjustedArgs.push_back(std::string { "-resource-dir=" } + CLANG_RESOURCE_DIR);
            return AdjustedArgs;
        };
        Tool.appendArgumentsAdjuster(set_resource_directory);
    }
    return Tool.run(std::make_unique<GenerateThunkLibsActionFactory>(std::move(libname), std::move(output_filenames)).get());
}
