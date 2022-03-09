#include <catch2/catch.hpp>
#include "catch_helpers.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/raw_os_ostream.h>

#include <interface.h>

#include <filesystem>
#include <fstream>
#include <string_view>

enum class GuestABI {
    X86_32,
    X86_64,
};

inline std::ostream& operator<<(std::ostream& os, GuestABI abi) {
    if (abi == GuestABI::X86_32) {
        os << "X86_32";
    } else if (abi == GuestABI::X86_64) {
        os << "X86_64";
    }
    return os;
}

/**
 * This class parses its input code and stores it alongside its AST representation.
 *
 * Use this with HasASTMatching in Catch2's CHECK_THAT/REQUIRE_THAT macros.
 */
struct SourceWithAST {
    std::string code;
    std::unique_ptr<clang::ASTUnit> ast;

    SourceWithAST(std::string_view input);
};

std::ostream& operator<<(std::ostream& os, const SourceWithAST& ast) {
    os << ast.code;

    // Additionally, change this to true to print the full AST on test failures
    const bool print_ast = false;
    if (print_ast) {
        for (auto it = ast.ast->top_level_begin(); it != ast.ast->top_level_end(); ++it) {
            // Skip header declarations
            if (!ast.ast->isInMainFileID((*it)->getBeginLoc())) {
                continue;
            }

            auto llvm_os = llvm::raw_os_ostream { os };
            (*it)->dump(llvm_os);
        }
    }
    return os;
}

struct Fixture {
    Fixture() {
        tmpdir = std::tmpnam(nullptr);
        std::filesystem::create_directory(tmpdir);
        output_filenames = {
            tmpdir + "/function_unpacks",
            tmpdir + "/tab_function_unpacks",
            tmpdir + "/ldr",
            tmpdir + "/ldr_ptrs",
            tmpdir + "/thunks",
            tmpdir + "/function_packs",
            tmpdir + "/function_packs_public",
            tmpdir + "/callback_unpacks_header_init",
            tmpdir + "/callback_structs",
            tmpdir + "/callback_typedefs",
            tmpdir + "/callback_unpacks_header",
            tmpdir + "/callback_unpacks",
        };
    }

    ~Fixture() {
        std::filesystem::remove_all(tmpdir);
    }

    struct GenOutput {
        SourceWithAST guest;
        SourceWithAST host;
    };

    /**
     * Runs the given given code through the thunk generator and verifies the output compiles.
     *
     * Input code with common definitions (types, functions, ...) should be specified in "prelude".
     * It will be prepended to "code" before processing and also to the generator output.
     */
    SourceWithAST run_thunkgen_guest(std::string_view prelude, std::string_view code, bool silent = false, std::optional<std::pair<ABITable*, GuestABI>> abi = std::nullopt);
    SourceWithAST run_thunkgen_host(std::string_view prelude, std::string_view code, bool silent = false);
    GenOutput run_thunkgen(std::string_view prelude, std::string_view code, bool silent = false);

    const std::string libname = "libtest";
    std::string tmpdir;
    OutputFilenames output_filenames;
};

using namespace clang::ast_matchers;

class MatchCallback : public MatchFinder::MatchCallback {
    bool success = false;

    using CheckFn = std::function<bool(const MatchFinder::MatchResult&)>;
    std::vector<CheckFn> binding_checks;

public:
    template<typename NodeType>
    void check_binding(std::string_view binding_name, bool (*check_fn)(const NodeType*)) {
        // Decorate the given check with node extraction and wrap it in a type-erased interface
        binding_checks.push_back(
            [check_fn, binding_name = std::string(binding_name)](const MatchFinder::MatchResult& result) {
                if (auto node = result.Nodes.getNodeAs<NodeType>(binding_name.c_str())) {
                    return check_fn(node);
                }
                return false;
            });
    }

    void run(const MatchFinder::MatchResult& result) override {
        success = true; // NOTE: If there are no callbacks, this signals that the match was found at all

        for (auto& binding_check : binding_checks) {
            success = success && binding_check(result);
        }
    }

    bool matched() const noexcept {
        return success;
    }
};

/**
 * This class connects the libclang AST to Catch2 test matchers, allowing for
 * code compiled via SourceWithAST objects to be pattern-matched using the
 * libclang ASTMatcher API.
 */
template<typename ClangMatcher>
class HasASTMatching : public Catch::MatcherBase<SourceWithAST> {
    ClangMatcher matcher;
    MatchCallback callback;
    std::string description;

public:
    HasASTMatching(const ClangMatcher& matcher_, std::optional<std::string> description_)
        : matcher(matcher_), description(description_.value_or("should compile and match the given AST pattern")) {

    }

    template<typename NodeT>
    HasASTMatching& check_binding(std::string_view binding_name, bool (*check_fn)(const NodeT*)) {
        callback.check_binding(binding_name, check_fn);
        return *this;
    }

    // TODO: Should be able to add host defines...
    bool match(const SourceWithAST& code) const override {
        MatchCallback result = callback;
        clang::ast_matchers::MatchFinder finder;
        finder.addMatcher(matcher, &result);
        finder.matchAST(code.ast->getASTContext());
        return result.matched();
    }

    std::string describe() const override {
        return description;
    }
};

static HasASTMatching<DeclarationMatcher> matches(const DeclarationMatcher& matcher_, std::optional<std::string> description = std::nullopt) {
    return HasASTMatching<DeclarationMatcher>(matcher_, std::move(description));
}

static HasASTMatching<StatementMatcher> matches(const StatementMatcher& matcher_, std::optional<std::string> description = std::nullopt) {
    return HasASTMatching<StatementMatcher>(matcher_, std::move(description));
}

/**
 * Catch matcher that checks if a tested C++ source defines a function with the given name
 */
static HasASTMatching<DeclarationMatcher> DefinesPublicFunction(std::string_view name) {
    auto desc = "should define and export a function called \"" + std::string(name) + "\"";
    return HasASTMatching<DeclarationMatcher>(functionDecl(hasName(name)), desc);
}

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
 * The "silent" parameter is used to suppress non-fatal diagnostics in tests that expect failure
 */
static void run_tool(clang::tooling::ToolAction& action, std::string_view code, bool silent = false, std::optional<GuestABI> guest_abi = std::nullopt) {
    const char* memory_filename = "gen_input.cpp";
    auto adjuster = clang::tooling::getClangStripDependencyFileAdjuster();
    std::vector<std::string> args = { "clang-tool", "-fsyntax-only", "-std=c++17", "-Werror", "-I.", memory_filename };
    if (guest_abi == GuestABI::X86_64) {
        args.push_back("-target");
        args.push_back("x86_64-linux-gnu");
    } else if (guest_abi == GuestABI::X86_32) {
        args.push_back("-target");
        args.push_back("i686-linux-gnu");
    } else {
        args.push_back("-DHOST");
    }

    // Corresponds to the content of GeneratorInterface.h
    const char* common_header_code = R"(namespace fexgen {
// function annotations: fex_gen_config<MyFunc>
struct returns_guest_pointer {}; // TODO: Deprecate in favor of pointer_passthrough
struct custom_host_impl {};
struct callback_annotation_base { bool prevent_multiple; };
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};

// type annotations: fex_gen_config<MyStruct>
struct opaque_to_guest {};
struct opaque_to_host {};

// struct member annotations: fex_gen_config<&MyStruct::member>
struct is_padding_member {};

// function parameter annotations: fex_gen_config<fexgen::annotate_parameter<MyFunc, 2>>
struct ptr_in {};
struct ptr_out {};
struct ptr_inout {};
struct ptr_pointer_passthrough {};
struct ptr_is_untyped_address {};

template<auto, int, typename = decltype(nullptr)>
struct annotate_parameter {};
} // namespace fexgen

template<typename> struct fex_gen_type {};
template<auto, int, typename = void> struct fex_gen_param {};

template<typename>
struct fex_gen_type;

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

static void run_tool(std::unique_ptr<clang::tooling::ToolAction> action, std::string_view code, bool silent = false, std::optional<GuestABI> guest_abi = std::nullopt) {
    return run_tool(*action, code, silent, guest_abi);
}

SourceWithAST::SourceWithAST(std::string_view input) : code(input) {
    // Call run_tool with a ToolAction that assigns this->ast

    struct ToolAction : clang::tooling::ToolAction {
        std::unique_ptr<clang::ASTUnit>& ast;

        ToolAction(std::unique_ptr<clang::ASTUnit>& ast_) : ast(ast_) { }

        bool runInvocation(std::shared_ptr<clang::CompilerInvocation> invocation,
                           clang::FileManager* files,
                           std::shared_ptr<clang::PCHContainerOperations> pch,
                           clang::DiagnosticConsumer *diag_consumer) override {
            auto diagnostics = clang::CompilerInstance::createDiagnostics(&invocation->getDiagnosticOpts(), diag_consumer, false);
            ast = clang::ASTUnit::LoadFromCompilerInvocation(invocation, std::move(pch), std::move(diagnostics), files);
            return (ast != nullptr);
        }
    } tool_action { ast };

    run_tool(tool_action, code);
}

/**
 * Generates guest thunk library code from the given input
 */
SourceWithAST Fixture::run_thunkgen_guest(std::string_view prelude, std::string_view code, bool silent, std::optional<std::pair<ABITable*, GuestABI>> abi) {
    ABITable dummy_abi = ABITable::singleton();
    ABITable& active_abi = abi ? *abi->first : dummy_abi;

    const std::string full_code = std::string { prelude } + std::string { code };
    run_tool(std::make_unique<AnalyzeABIActionFactory>(active_abi.host()), full_code, silent);
    if (abi) {
        run_tool(std::make_unique<AnalyzeABIActionFactory>(active_abi.abis[1]), full_code, silent, abi->second); // TODO: Pick proper abi member dynamically depending on GuestABI
//        run_tool(std::make_unique<AnalyzeABIActionFactory>(active_abi.abis[2]), full_code, silent, GuestABI::X86_32); // TODO: Remove?
    } else {
        // Use host API for second entry...
        run_tool(std::make_unique<AnalyzeABIActionFactory>(active_abi.abis[1]), full_code, silent);
    }

    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames, active_abi), full_code, silent);

    std::string result =
        "#include <cstring>\n"
        "#include <cstdint>\n"
        "#include <type_traits>\n"
        "#define MAKE_THUNK(lib, name, hash) extern \"C\" int fexthunks_##lib##_##name(void*);\n"
        "#define FEX_PACKFN_LINKAGE\n";
    for (auto& filename : {
            output_filenames.thunks,
            output_filenames.function_packs_public,
            output_filenames.function_packs,
            }) {
        std::ifstream file(filename);
        const auto current_size = result.size();
        const auto new_data_size = std::filesystem::file_size(filename);
        result.resize(result.size() + new_data_size);
        file.read(result.data() + current_size, result.size());
    }
    return SourceWithAST { std::string { prelude } + result };
}

/**
 * Generates host thunk library code from the given input
 */
SourceWithAST Fixture::run_thunkgen_host(std::string_view prelude, std::string_view code, bool silent) {
    // Duplicate host ABI twice, as ABI information isn't needed for host generation
    ABITable abi_table = ABITable::singleton();
    const std::string full_code = std::string { prelude } + std::string { code };
    run_tool(std::make_unique<AnalyzeABIActionFactory>(abi_table.host()), full_code, silent);
    run_tool(std::make_unique<AnalyzeABIActionFactory>(abi_table.abis[1]), full_code, silent);
    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames, abi_table), full_code, silent);

    std::string result =
        "#include <type_traits>\n"

        "#include <cstdint>\n"
        "#include <dlfcn.h>\n"
        "template<typename Fn>\n"
        "struct function_traits;\n"
        "template<typename Result, typename Arg>\n"
        "struct function_traits<Result(*)(Arg)> {\n"
        "    using result_t = Result;\n"
        "    using arg_t = Arg;\n"
        "};\n"
        "template<auto Fn>\n"
        "static typename function_traits<decltype(Fn)>::result_t\n"
        "fexfn_type_erased_unpack(void* argsv) {\n"
        "    using args_t = typename function_traits<decltype(Fn)>::arg_t;\n"
        "    return Fn(reinterpret_cast<args_t>(argsv));\n"
        "}\n";
    for (auto& filename : {
            output_filenames.ldr_ptrs,
            output_filenames.function_unpacks,
            output_filenames.tab_function_unpacks,
            output_filenames.ldr,
            }) {
        bool tab_function_unpacks = (filename == output_filenames.tab_function_unpacks);
        if (tab_function_unpacks) {
            result += "struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };\n";
            result += "static ExportEntry exports[] = {\n";
        }

        std::ifstream file(filename);
        const auto current_size = result.size();
        const auto new_data_size = std::filesystem::file_size(filename);
        result.resize(result.size() + new_data_size);
        file.read(result.data() + current_size, result.size());

        if (tab_function_unpacks) {
            result += "  { nullptr, nullptr }\n";
            result += "};\n";
        }
    }
    return SourceWithAST { std::string { prelude } + result };
}

Fixture::GenOutput Fixture::run_thunkgen(std::string_view prelude, std::string_view code, bool silent) {
    return { run_thunkgen_guest(prelude, code, silent),
             run_thunkgen_host(prelude, code, silent) };
}

TEST_CASE_METHOD(Fixture, "Trivial") {
    const auto output = run_thunkgen("",
        "#include <thunks_common.h>\n"
        "void func();\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n");

    // Guest code
    CHECK_THAT(output.guest, DefinesPublicFunction("func"));

    CHECK_THAT(output.guest,
        matches(functionDecl(
            hasName("fexfn_pack_func"),
            returns(asString("void")),
            parameterCountIs(0)
        )));

    // Host code
    CHECK_THAT(output.host,
        matches(varDecl(
            hasName("exports"),
            hasType(constantArrayType(hasElementType(asString("struct ExportEntry")), hasSize(2))),
            hasInitializer(initListExpr(hasInit(0, expr()),
                                        hasInit(1, initListExpr(hasInit(0, implicitCastExpr()), hasInit(1, implicitCastExpr())))))
            // TODO: check null termination
            )));
}

// Unknown annotations trigger an error
TEST_CASE_METHOD(Fixture, "UnknownAnnotation") {
    REQUIRE_THROWS(run_thunkgen("void func();\n",
        "struct invalid_annotation {};\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> : invalid_annotation {};\n", true));

    REQUIRE_THROWS(run_thunkgen("void func();\n",
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> { int invalid_field_annotation; };\n", true));
}

TEST_CASE_METHOD(Fixture, "VersionedLibrary") {
    const auto output = run_thunkgen_host("",
        "template<auto> struct fex_gen_config { int version = 123; };\n");

    CHECK_THAT(output,
        matches(callExpr(
            callee(functionDecl(hasName("dlopen"))),
            hasArgument(0, stringLiteral().bind("libname"))
            ))
        .check_binding("libname", +[](const clang::StringLiteral* lit) {
            return lit->getString().endswith(".so.123");
        }));
}

// Parameter is a function pointer
TEST_CASE_METHOD(Fixture, "FunctionPointerParameter") {
    const auto output = run_thunkgen_guest("void func(int (*funcptr)(char, char));\n",
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n");

    CHECK_THAT(output,
        matches(functionDecl(
            hasName("fexfn_pack_func_internal"),
            returns(asString("void")),
            parameterCountIs(1),
            hasParameter(0, hasType(asString("int (*)(char, char)")))
        )));
}

// Parameter is a guest function pointer
TEST_CASE_METHOD(Fixture, "GuestFunctionPointerParameter") {
    const std::string prelude =
        "struct fex_guest_function_ptr { int (*x)(char,char); };\n"
        "void fexfn_impl_libtest_func(fex_guest_function_ptr);\n";
    const auto output = run_thunkgen(prelude,
        "#include <thunks_common.h>\n"
        "void func(int (*funcptr)(char, char));\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> : fexgen::callback_guest, fexgen::custom_host_impl {};\n");

    CHECK_THAT(output.guest,
        matches(functionDecl(
            hasName("fexfn_pack_func"),
            returns(asString("void")),
            parameterCountIs(1),
            hasParameter(0, hasType(asString("int (*)(char, char)")))
        )));

    // Host-side implementation only sees an opaque type that it can't call
    CHECK_THAT(output.host,
        matches(callExpr(callee(functionDecl(hasName("fexfn_impl_libtest_func"))),
                         hasArgument(0, hasType(asString("struct fex_guest_function_ptr")))
            )));
}

TEST_CASE_METHOD(Fixture, "MultipleParameters") {
    const std::string prelude =
        "struct TestStruct { int member; };\n";

    auto output = run_thunkgen(prelude,
        "void func(int arg, char, unsigned long, TestStruct);\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n");

    // Guest code
    CHECK_THAT(output.guest, DefinesPublicFunction("func"));

    CHECK_THAT(output.guest,
        matches(functionDecl(
            hasName("fexfn_pack_func"),
            returns(asString("void")),
            parameterCountIs(4),
            hasParameter(0, hasType(asString("int"))),
            hasParameter(1, hasType(asString("char"))),
            hasParameter(2, hasType(asString("unsigned long"))),
            hasParameter(3, hasType(asString("struct TestStruct")))
        )));

    // Host code
    CHECK_THAT(output.host,
        matches(varDecl(
            hasName("exports"),
            hasType(constantArrayType(hasElementType(asString("struct ExportEntry")), hasSize(2))),
            hasInitializer(initListExpr(hasInit(0, expr()),
                                        hasInit(1, initListExpr(hasInit(0, implicitCastExpr()), hasInit(1, implicitCastExpr())))))
            // TODO: check null termination
            )));

    CHECK_THAT(output.host,
        matches(functionDecl(
            hasName("fexfn_unpack_libtest_func"),
            // Packed argument struct should contain all parameters
            parameterCountIs(1),
            hasParameter(0, hasType(pointerType(pointee(
                recordType(hasDeclaration(decl(
                    has(fieldDecl(hasType(asString("int")))),
                    has(fieldDecl(hasType(asString("char")))),
                    has(fieldDecl(hasType(asString("unsigned long")))),
                    has(fieldDecl(hasType(asString("struct TestStruct"))))
                    )))))))
            )));
}

// Returning a function pointer should trigger an error unless an annotation is provided
TEST_CASE_METHOD(Fixture, "ReturnFunctionPointer") {
    const std::string prelude = "using funcptr = void (*)(char, char);\n";

    REQUIRE_THROWS(run_thunkgen_guest(prelude,
        "funcptr func(int);\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n", true));

    REQUIRE_NOTHROW(run_thunkgen_guest(prelude,
        "#include <thunks_common.h>\n"
        "funcptr func(int);\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> : fexgen::returns_guest_pointer {};\n"));
}

#if 0
TEST_CASE_METHOD(Fixture, "VariadicFunction") {
    const std::string prelude = "void func(int arg, ...);\n";

    const auto output = run_thunkgen_guest(prelude,
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {\n"
        "  using uniform_va_type = char;\n"
        "};\n");

    CHECK_THAT(output,
        matches(functionDecl(
            hasName("fexfn_pack_func_internal"),
            returns(asString("void")),
            parameterCountIs(3),
            hasParameter(0, hasType(asString("int"))),
            hasParameter(1, hasType(asString("unsigned long"))),
            hasParameter(2, hasType(pointerType(pointee(asString("char")))))
        )));
}

// Variadic functions without annotation trigger an error
TEST_CASE_METHOD(Fixture, "VariadicFunctionsWithoutAnnotation") {
    REQUIRE_THROWS(run_thunkgen_guest("void func(int arg, ...);\n",
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n", true));
}
#endif

static auto DefinesGuestToHostRepackingFunction (std::string_view type_name) {
    return matches(functionDecl(hasName("fex_repack_to_host"), hasParameter(1, hasType(asString("fex_host_type<" + std::string { type_name } + "> &")))),
                   "fex_repack_to_host defined for \"" + std::string { type_name } + "\"");
}

static auto DefinesHostToGuestRepackingFunction(std::string_view type_name) {
    return matches(functionDecl(hasName("fex_repack_from_host"), hasParameter(0, hasType(asString("const fex_host_type<" + std::string { type_name } + "> &")))),
                   "fex_repack_from_host defined for \"" + std::string { type_name } + "\"");
}

static auto FunctionRepacksStructOnEntry(std::string_view function_name, std::string_view type_name_view) {
    std::string type_name { type_name_view };
    return matches(functionDecl(
              hasName(function_name),
              hasDescendant(callExpr(callee(functionDecl(hasName("fex_repack_to_host"), hasParameter(1, hasType(asString("fex_host_type<" + type_name + "> &")))))))
              ), "fexfn_pack_func repacks \"" + type_name + "\" on entry");
}

static auto FunctionRepacksStructOnExit(std::string_view function_name, std::string_view type_name_view) {
    std::string type_name { type_name_view };
    return matches(functionDecl(
              hasName(function_name),
              hasDescendant(callExpr(callee(functionDecl(hasName("fex_repack_from_host"), hasParameter(0, hasType(asString("const fex_host_type<" + type_name + "> &")))))))
              ), "fexfn_pack_func repacks \"" + type_name + "\" on exit");
}

// Check the packing function repacks the struct on both entry and exit
static auto FunctionFullyRepacksType(std::string_view function_name, std::string_view type_name) {
    return matches_all(
            // First, check that the repacking helper functions are defined at all
            DefinesGuestToHostRepackingFunction(type_name),
            DefinesHostToGuestRepackingFunction(type_name),
            // Then, check they are called in the packing function
            FunctionRepacksStructOnEntry(function_name, type_name),
            FunctionRepacksStructOnExit(function_name, type_name));
}

// Check the packing function does *not* repack the struct on either entry or exit
static auto FunctionForwardsStructData(std::string_view function_name, std::string_view type_name) {
    return matches_all(
            // First, check the packing function is defined
            DefinesPublicFunction(function_name),
            // Then, check that it doesn't call any of the packing functions on the given type
            not_matches(FunctionRepacksStructOnEntry(function_name, type_name)),
            not_matches(FunctionRepacksStructOnExit(function_name, type_name)));
}

// Check the packing function does *not* repack the pointee data struct on either entry or exit
static auto FunctionForwardsPointer(std::string_view function_name, std::string_view variable_name, bool zero_extended = true) {
    return matches_all(
            // First, check the packing function is defined
            DefinesPublicFunction(function_name)//,
            // Then, check that args.var = var
            // Then, check that var = args.var
            // Then, check that args.var uses the guest size IFF !zero_extended, otherwise check that it uses host size
            );
}

static std::optional<TypeInfo> lookup_type(const ABI& abi, std::string_view struct_name) {
    auto info_it = abi.find(std::string { struct_name });
    if (info_it == abi.end()) {
        return std::nullopt;
    }
    return info_it->second;
}

inline std::ostream& operator<<(std::ostream& os, const StructInfo::ChildInfo& child) {
    os << child.offset_bits / 8;
    if (child.offset_bits % 8) {
        os << '.' << child.offset_bits % 8;
    }
    os << ": " << child.type_name << " ";
    for (auto& ptr : child.pointer_chain) {
        if (ptr.array_size) {
            os << '[' << *ptr.array_size << "] ";
        } else {
            os << "* ";
        }
    }
    os << child.member_name;
    if (child.is_padding_member) {
        os << " [[padding]]";
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const StructInfo& info) {
    os << "{ ";
    for (auto& child : info.children) {
        os << child << "; ";
    }
    os << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, std::optional<TypeInfo> info) {
    if (!info) {
        os << "(no ABI description)";
        return os;
    }

    if (info->is_opaque()) {
        os << "(opaque type)";
    } if (auto struct_info = info->as_struct()) {
        os << "struct " << *struct_info;
    }
    return os;
}

/**
 * Catch matcher that checks if the tested data is a struct with the given set of members
 */
struct has_layout : Catch::MatcherBase<std::optional<TypeInfo>> {
    std::vector<StructInfo::ChildInfo> expected_members;

    has_layout(std::vector<StructInfo::ChildInfo> members_)
        : expected_members(std::move(members_)) {

    }

    bool match(const std::optional<TypeInfo>& type_info) const override {
        auto struct_info = type_info ? type_info->as_struct() : nullptr;
        if (!struct_info) {
            return false;
        }

        // TODO: Move to a dedicated matcher so we can tell which child is off on failure
        const auto& members = struct_info->children;
        if (members.size() != expected_members.size()) {
            return false;
        }
        for (int child_idx = 0; child_idx < members.size(); ++child_idx) {
            if (members[child_idx] != expected_members[child_idx]) {
                return false;
            }
        }

        return true;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "should have struct layout " << StructInfo { 0, {}, expected_members };
        return std::move(ss).str();
    }
};

/**
 * Catch matcher that checks if the tested data is an opaque type
 */
struct is_opaque_type : Catch::MatcherBase<std::optional<TypeInfo>> {
    bool match(const std::optional<TypeInfo>& type_info) const override {
        return type_info->is_opaque();
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "should be an opaque type";
        return std::move(ss).str();
    }
};

TEST_CASE_METHOD(Fixture, "Forwarding builtin types", "[abi]") {
    // TODO: Test that simple types and pointers to them get forwarded trivially:
    // int, char

    // TODO: Test that pointers to simple types only need truncation/zero-extension:
    // int*, char*, float*, double*

    // TODO: Verify long long double is an exception
}

TEST_CASE_METHOD(Fixture, "Forwarding trivial struct", "[abi]") {
    const std::string prelude =
        "struct A { int x; int y; };\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();
    const auto output = run_thunkgen_guest(prelude,
        "#include \"thunks_common.h\"\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n"
        "template<> struct fex_gen_param<func, 0, A*> : fexgen::ptr_inout {};\n", // TODO: Shouldn't really be needed since A is trivally ABI compatible
        false, std::make_pair(&abi, GuestABI::X86_64));

    std::vector<StructInfo::ChildInfo> expected_layout {
       { 0, "int", "x" },
       { 32, "int", "y" }
    };

    CHECK_THAT(lookup_type(abi.abis[1], "struct A"), has_layout(expected_layout));
    CHECK_THAT(lookup_type(abi.host(), "struct A"), has_layout(expected_layout));

    CHECK_THAT(output, FunctionForwardsStructData("fexfn_pack_func", "struct A"));
}

TEST_CASE_METHOD(Fixture, "Repacking struct", "[abi]") {
    const std::string prelude =
        // Simple example struct that flips its members on the host
        "#ifndef HOST\n"
        "struct A { int x; int y; };\n"
        "#else\n"
        "struct A { int y; int x; };\n"
        "#endif\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();
    const auto output = run_thunkgen_guest(prelude,
        "#include \"thunks_common.h\"\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n"
        "template<> struct fex_gen_param<func, 0, A*> : fexgen::ptr_inout {};\n",
        false, std::make_pair(&abi, GuestABI::X86_64));

    CHECK_THAT(lookup_type(abi.abis[1], "struct A"),
                has_layout({
                   { 0, "int", "x" },
                   { 32, "int", "y" }
                }));

    CHECK_THAT(lookup_type(abi.host(), "struct A"),
                has_layout({
                   { 0, "int", "y" },
                   { 32, "int", "x" }
                }));

    CHECK_THAT(output, FunctionFullyRepacksType("fexfn_pack_func", "struct A"));
}

// TODO: Also test struct C { B; A } where B has different size across archs => Ensure A gets relocated despite being ABI-compatible
// Tests that ABI-incompatibility is transitive by considering a function taking pointers to three different struct types:
// * struct A: ABI-compatible across architectures (can be forwarded)
// * struct B: ABI-incompatible (hence requires repacking)
// * struct C: Composite struct of A and B (requires repacking since B is ABI-incompatible)
TEST_CASE_METHOD(Fixture, "Repacking nested struct", "[abi]") {
    const bool use_array_of_bs = GENERATE(false, true);
    const std::string prelude = std::string {
        "struct A { int x; };\n"
        "#ifndef HOST\n"
        "struct B { int y; int z; };\n"
        "#else\n"
        "struct B { int z; int y; };\n"
        "#endif\n"
        "struct C { A a; B b" } + (use_array_of_bs ? "[12]" : "") + "; };\n"
        "extern \"C\" void func(A*, B*, C*);\n";

    ABITable abi = ABITable::standard();
    const auto output = run_thunkgen_guest(prelude,
        "#include \"thunks_common.h\"\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n"
        "template<> struct fex_gen_param<func, 0, A*> : fexgen::ptr_inout {};\n"
        "template<> struct fex_gen_param<func, 1, B*> : fexgen::ptr_inout {};\n"
        "template<> struct fex_gen_param<func, 2, C*> : fexgen::ptr_inout {};\n",
        false, std::make_pair(&abi, GuestABI::X86_64));

    // Check ABI for A
    CHECK_THAT(lookup_type(abi.abis[1], "struct A"), has_layout({{ 0, "int", "x" }}));
    CHECK_THAT(lookup_type(abi.host(), "struct A"), has_layout({{ 0, "int", "x" }}));

    // Check ABI for B
    CHECK_THAT(lookup_type(abi.abis[1], "struct B"),
                has_layout({
                   { 0, "int", "y" },
                   { 32, "int", "z" }
                }));
    CHECK_THAT(lookup_type(abi.host(), "struct B"),
                has_layout({
                   { 0, "int", "z" },
                   { 32, "int", "y" }
                }));

    // Check ABI for C
    std::vector<StructInfo::ChildInfo> layout_c {
       { 0, "struct A", "a" },
       { 32, "struct B", "b" }
    };
    if (use_array_of_bs) {
        layout_c[1].pointer_chain.push_back(PointerInfo {12});
    }
    CHECK_THAT(lookup_type(abi.abis[1], "struct C"), has_layout({ layout_c }));
    CHECK_THAT(lookup_type(abi.host(), "struct C"), has_layout({ layout_c }));

    CHECK_THAT(output, FunctionForwardsStructData("fexfn_pack_func", "struct A"));
    CHECK_THAT(output, FunctionFullyRepacksType("fexfn_pack_func", "struct B"));
    // TODO: Test that fex_repack_from_host(struct C) repacks b recursively...
    CHECK_THAT(output, FunctionFullyRepacksType("fexfn_pack_func", "struct C"));
}

TEST_CASE_METHOD(Fixture, "Repacking padded struct", "[abi]") {
    const std::string prelude =
        "struct A {\n"
        "  int x;\n"
        "#ifndef HOST\n"
        "  int pad;\n"
        "#endif\n"
        "  int y;\n"
        "};\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();

    SECTION("Fails with unannotated padding member") {
        CHECK_THROWS(run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n",
            true, std::make_pair(&abi, GuestABI::X86_64)));
    }

    SECTION("Works with annotated padding member") {
        const auto output = run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n"
            "template<> struct fex_gen_param<func, 0, A*> : fexgen::ptr_inout {};\n"
            "#ifndef HOST\n"
            "template<> struct fex_gen_config<&A::pad> : fexgen::is_padding_member {};\n"
            "#endif\n", false, std::make_pair(&abi, GuestABI::X86_64));

        CHECK_THAT(lookup_type(abi.abis[1], "struct A"),
                    has_layout({
                       { 0, "int", "x" },
                       { 32, "int", "pad", {}, {}, true },
                       { 64, "int", "y" }
                    }));

        CHECK_THAT(lookup_type(abi.host(), "struct A"),
                    has_layout({
                       { 0, "int", "x" },
                       { 32, "int", "y" }
                    }));

        // TODO: Check that pad is just skipped?
        CHECK_THAT(output, FunctionFullyRepacksType("fexfn_pack_func", "struct A"));
    }
}

// TODO: Tested functionality not implemented. Test incomplete.
#if 0
TEST_CASE_METHOD(Fixture, "Repacking structs with pointers") {
    const std::string prelude =
        "struct A {\n"
        "  int* ptr;\n"
        "};\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();

    SECTION("Fails with unannotated pointer member") {
        CHECK_THROWS(run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n",
            true, std::make_pair(&abi, GuestABI::X86_64)));
    }

    SECTION("Works with annotated pointer member") {
        const std::string annotation = GENERATE("passthrough", "ptr_is_untyped_address");
        const auto output = run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n"
            "#ifndef HOST\n"
            "template<> struct fex_gen_config<&A::ptr> : fexgen::" + annotation + " {};\n"
            "#endif\n", false, std::make_pair(&abi, GuestABI::X86_64));

        CHECK_THAT(lookup_type(abi.abis[1], "struct A"), has_layout({{ 0, "int*", "ptr" }}));
        CHECK_THAT(lookup_type(abi.host(), "struct A"), has_layout({{ 0, "int*", "ptr" }}));

        // TODO: Check pointer gets converted appropriately depending on the given annotation
        if (annotation == "passthrough") {
            // TODO: no repacking but conversion to uintptr_t wrapper
            // TODO: A should be abi compatible
        } else if (annotation == "guest_opaque_ptr") {
            // TODO: repacking with zero-extension
        } else if (annotation == "host_opaque_ptr") {
            // TODO: repacking with zero-extension to custom type wrapping uintptr_t
        }
    }

    // TODO: struct with pointers to opaque struct types
}
#endif

TEST_CASE_METHOD(Fixture, "Pointer parameters", "[abi]") {
    const std::string prelude =
        // Simple example struct that flips its members on the host
        "#ifndef HOST\n"
        "struct A { int x; int y; };\n"
        "#else\n"
        "struct A { int y; int x; };\n"
        "#endif\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();

    // TODO: Test with int*
    // TODO: Test with B* (B being ABI-compatible)

    SECTION("Fails with unannotated pointer member") {
        CHECK_THROWS(run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n",
            true, std::make_pair(&abi, GuestABI::X86_64)));
    }

    SECTION("Works with annotated pointer member") {
        // TODO: Test ptr_in/out/inout-array
        for (const std::string annotation : { "ptr_in", "ptr_out", "ptr_inout", "ptr_pointer_passthrough", "ptr_is_untyped_address" }) {
            DYNAMIC_SECTION("With annotation " << annotation) {
                const auto output = run_thunkgen_guest(prelude,
                    "#include <thunks_common.h>\n"
                    "template<auto> struct fex_gen_config {};\n"
                    "template<> struct fex_gen_config<func> {};\n"
                    "template<> struct fex_gen_param<func, 0, A*> : fexgen::" + annotation + " {};\n",
                    false, std::make_pair(&abi, GuestABI::X86_64));

                // First, verify that ABI information is detected correctly for all annotations
                CHECK_THAT(lookup_type(abi.abis[1], "struct A"),
                            has_layout({
                               { 0, "int", "x" },
                               { 32, "int", "y" }
                            }));
                CHECK_THAT(lookup_type(abi.host(), "struct A"),
                            has_layout({
                               { 0, "int", "y" },
                               { 32, "int", "x" }
                            }));

                // Second, check that the correct repacking code gets emitted
                if (annotation == "ptr_in") {
                    CHECK_THAT(output, DefinesGuestToHostRepackingFunction("struct A"));
                    CHECK_THAT(output, FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A"));
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnExit("fexfn_pack_func", "struct A")));
                } else if (annotation == "ptr_out") {
                    CHECK_THAT(output, DefinesHostToGuestRepackingFunction("struct A"));
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A")));
                    CHECK_THAT(output, FunctionRepacksStructOnExit("fexfn_pack_func", "struct A"));
                } else if (annotation == "ptr_inout") {
                    CHECK_THAT(output, DefinesGuestToHostRepackingFunction("struct A"));
                    CHECK_THAT(output, DefinesHostToGuestRepackingFunction("struct A"));
                    CHECK_THAT(output, FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A"));
                    CHECK_THAT(output, FunctionRepacksStructOnExit("fexfn_pack_func", "struct A"));
                } else if (annotation == "ptr_pointer_passthrough") {
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnExit("fexfn_pack_func", "struct A")));
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A")));
                    // TODO: conversion to uintptr_t wrapper
                } else if (annotation == "ptr_is_untyped_address") {
                    // TODO: First member in "args" is a uint32_t/64_t
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnExit("fexfn_pack_func", "struct A")));
                    CHECK_THAT(output, not_matches(FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A")));
                    // TODO: repacking with zero-extension
                }
            }
        }
    }
}

// TODO: Equivalent tests for pointer return values
TEST_CASE_METHOD(Fixture, "Opaque type parameters", "[abi]") {
    const std::string prelude =
        // Simple example struct that's made ABI incompatible by choosing completely separate definitions
        "#ifndef HOST\n"
        "struct A { int x; };\n"
        "#else\n"
        "struct A { void* y; };\n"
        "#endif\n"
        "extern \"C\" void func(A*);\n";

    ABITable abi = ABITable::standard();

    for (const std::string annotation : { "opaque_to_guest", "opaque_to_host" }) {
        DYNAMIC_SECTION("With annotation " << annotation) {
            const auto output = run_thunkgen_guest(prelude,
                "#include <thunks_common.h>\n"
                "template<auto> struct fex_gen_config {};\n"
                "template<> struct fex_gen_config<func> {};\n"
                "template<> struct fex_gen_type<A> : fexgen::" + annotation + " {};\n",
                false, std::make_pair(&abi, GuestABI::X86_64));

            // Verify that no repacking code gets emitted
            CHECK_THAT(output, not_matches(FunctionRepacksStructOnEntry("fexfn_pack_func", "struct A")));
            CHECK_THAT(output, not_matches(FunctionRepacksStructOnExit("fexfn_pack_func", "struct A")));

            // Verify that ABI information is detected correctly for all annotations
            if (annotation == "opaque_to_guest") {
                CHECK_THAT(lookup_type(abi.abis[1], "struct A"), is_opaque_type());
                CHECK_THAT(lookup_type(abi.host(), "struct A"), has_layout({ { 0, "int", "x" } }));
            } else if (annotation == "opaque_to_host") {
                // Opaque to host
                CHECK_THAT(lookup_type(abi.abis[1], "struct A"), has_layout({ { 0, "void*", "y" } }));
                CHECK_THAT(lookup_type(abi.host(), "struct A"), is_opaque_type());
            }
        }
    }
}

TEST_CASE_METHOD(Fixture, "Factory functions", "[abi]") {
    SECTION("Works if data is ABI-compatible") {
        const std::string prelude =
            // Simple example struct that flips its members on the host
            "struct A { int x; int y; };\n"
            "extern \"C\" A* func();\n";

        ABITable abi = ABITable::standard();

        auto output = run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n",
            false, std::make_pair(&abi, GuestABI::X86_64));

        CHECK_THAT(output, FunctionForwardsStructData("fexfn_pack_func", "struct A"));
    }

    SECTION("Fails if data is ABI-incompatible") {
        const std::string prelude =
            // Simple example struct that flips its members on the host
            "#ifndef HOST\n"
            "struct A { int x; int y; };\n"
            "#else\n"
            "struct A { int y; int x; };\n"
            "#endif\n"
            "extern \"C\" A* func();\n";

        // TODO: Consider enabling automatic repacking for return values
        CHECK_THROWS(run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n",
            true));
    }

    SECTION("Works if data is ABI-incompatible but guest-opaque") {
        const std::string prelude =
            // Simple example struct that flips its members on the host
            "#ifndef HOST\n"
            "struct A { int x; int y; };\n"
            "#else\n"
            "struct A { int y; int x; };\n"
            "#endif\n"
            "extern \"C\" A* func();\n";

        ABITable abi = ABITable::standard();
        auto output = run_thunkgen_guest(prelude,
            "#include <thunks_common.h>\n"
            "template<auto> struct fex_gen_config {};\n"
            "template<> struct fex_gen_config<func> {};\n"
            "template<> struct fex_gen_type<A> : fexgen::opaque_to_guest {};\n",
            false, std::make_pair(&abi, GuestABI::X86_64));

        // TODO: Check that the raw pointer is copied
        CHECK_THAT(output, FunctionForwardsStructData("fexfn_pack_func", "struct A"));
    }
}
