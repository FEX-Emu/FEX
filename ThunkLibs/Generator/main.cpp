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

    const int total_argc = argc;

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
        } else if (target == "-callback_unpacks_header") {
            output_filenames.callback_unpacks_header = out_filename;
        } else if (target == "-callback_unpacks_header_init") {
            output_filenames.callback_unpacks_header_init = out_filename;
        } else if (target == "-symbol_list") {
            output_filenames.symbol_list = out_filename;
        } else {
            std::cerr << "Unrecognized generator target \"" << target << "\"\n";
            return EXIT_FAILURE;
        }
    }

    // TODO: Add prepass to gather functions, then only extract type ABIs for types that are actually used

    ABITable abi = ABITable::standard();
    ClangTool Tool(*compile_db, { filename });
    auto force_enable_color_output = [](const clang::tooling::CommandLineArguments &Args, clang::StringRef) {
        clang::tooling::CommandLineArguments AdjustedArgs = Args;
        AdjustedArgs.push_back("-fdiagnostics-color");
        return AdjustedArgs;
    };
    Tool.appendArgumentsAdjuster(force_enable_color_output);
    // TODO: Error check
    auto ret = Tool.run(std::make_unique<AnalyzeABIActionFactory>(abi.host()).get());
    if (ret != 0) {
        std::cerr << "Aborting because ABI analysis failed for host\n";
        return ret;
    }

    std::pair<ABI&, std::vector<std::string>> abi_commands[] = {
        // TODO: Attempt to find header locations automatically...
//        { abi.x86_32, { "-target", "i686-linux-gnu", "-isystem/usr/i686-linux-gnu/include", "-isystem/usr/i686-linux-gnu/include/c++/11/i686-linux-gnu/" } },
        { abi.abis[1], { "-target", "x86_64-linux-gnu", "-isystem/usr/x86_64-linux-gnu/include", "-isystem/usr/x86_64-linux-gnu/include/c++/11/x86_64-linux-gnu/" } }
    };
    for (auto& [abi, extra_commands] : abi_commands) {
        // Gather arguments past "--" and add target-specific commands
        std::vector<std::string> commands { argv + argc + 1, argv + total_argc };
        commands.insert(commands.end(), extra_commands.begin(), extra_commands.end());

        auto compile_db = FixedCompilationDatabase(".", commands);
        ClangTool ToolGuest(compile_db, { filename });
        ToolGuest.appendArgumentsAdjuster(force_enable_color_output);
        auto ret = ToolGuest.run(std::make_unique<AnalyzeABIActionFactory>(abi).get());
        if (ret != 0) {
            std::cerr << "Aborting because ABI analysis failed for " << abi_commands->second[1] << "\n";
            return ret;
        }
    }

    return Tool.run(std::make_unique<GenerateThunkLibsActionFactory>(std::move(libname), std::move(output_filenames), abi).get());
}
