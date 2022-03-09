#include <catch2/catch.hpp>

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
    SourceWithAST run_thunkgen_guest(std::string_view prelude, std::string_view code, bool silent = false);
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

public:
    HasASTMatching(const ClangMatcher& matcher_) : matcher(matcher_) {

    }

    template<typename NodeT>
    HasASTMatching& check_binding(std::string_view binding_name, bool (*check_fn)(const NodeT*)) {
        callback.check_binding(binding_name, check_fn);
        return *this;
    }

    bool match(const SourceWithAST& code) const override {
        MatchCallback result = callback;
        clang::ast_matchers::MatchFinder finder;
        finder.addMatcher(matcher, &result);
        finder.matchAST(code.ast->getASTContext());
        return result.matched();
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "should compile and match the given AST pattern";
        return ss.str();
    }
};

HasASTMatching<DeclarationMatcher> matches(const DeclarationMatcher& matcher_) {
    return HasASTMatching<DeclarationMatcher>(matcher_);
}

HasASTMatching<StatementMatcher> matches(const StatementMatcher& matcher_) {
    return HasASTMatching<StatementMatcher>(matcher_);
}

/**
 * Catch matcher that checks if a tested C++ source defines a function with the given name
 */
class DefinesPublicFunction : public HasASTMatching<DeclarationMatcher> {
    std::string function_name;

public:
    DefinesPublicFunction(std::string_view name) : HasASTMatching(functionDecl(hasName(name))), function_name(name) {
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "should define and export a function called \"" + function_name + "\"";
        return ss.str();
    }
};

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
static void run_tool(clang::tooling::ToolAction& action, std::string_view code, bool silent = false) {
    const char* memory_filename = "gen_input.cpp";
    auto adjuster = clang::tooling::getClangStripDependencyFileAdjuster();
    std::vector<std::string> args = { "clang-tool", "-fsyntax-only", "-std=c++17", "-Werror", "-I.", memory_filename };

    const char* common_header_code = R"(namespace fexgen {
struct returns_guest_pointer {};
struct custom_host_impl {};
struct callback_annotation_base { bool prevent_multiple; };
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};
} // namespace fexgen
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

static void run_tool(std::unique_ptr<clang::tooling::ToolAction> action, std::string_view code, bool silent = false) {
    return run_tool(*action, code, silent);
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
SourceWithAST Fixture::run_thunkgen_guest(std::string_view prelude, std::string_view code, bool silent) {
    const std::string full_code = std::string { prelude } + std::string { code };
    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames), full_code, silent);

    std::string result =
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
    const std::string full_code = std::string { prelude } + std::string { code };
    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames), full_code, silent);

    std::string result =
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
