#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"

#include <iostream>
#include <string>

#include "interface.h"

using namespace clang::tooling;

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <filename> <libname> <gen_target> <output_filename> -- <clang_flags>\n";
}

int main(int argc, char* argv[]) {
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
    char** const last_internal_arg = argv + argc;

    char** arg = argv + 1;
    const auto filename = *arg++;
    const std::string libname = *arg++;

    // Iterate over generator targets (remaining arguments up to "--" separator)
    OutputFilenames output_filenames;
    while (arg < last_internal_arg) {
        auto target = std::string { *arg++ };
        auto out_filename = *arg++;
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
        } else if (target == "-callback_structs") {
            output_filenames.callback_structs = out_filename;
        } else if (target == "-callback_typedefs") {
            output_filenames.callback_typedefs = out_filename;
        } else if (target == "-callback_unpacks") {
            output_filenames.callback_unpacks = out_filename;
        } else if (target == "-symbol_list") {
            output_filenames.symbol_list = out_filename;
        } else {
            std::cerr << "Unrecognized generator target \"" << target << "\"\n";
            return EXIT_FAILURE;
        }
    }

    ClangTool Tool(*compile_db, { filename });
    return Tool.run(std::make_unique<GenerateThunkLibsActionFactory>(std::move(libname), std::move(output_filenames)).get());
}
