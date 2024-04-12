#include <catch2/catch_all.hpp>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>

#include <interface.h>

#include <filesystem>
#include <fstream>
#include <string_view>

#include "common.h"

/**
 * This class parses its input code and stores it alongside its AST representation.
 *
 * Use this with HasASTMatching in Catch2's CHECK_THAT/REQUIRE_THAT macros.
 */
struct SourceWithAST {
    std::string code;
    std::unique_ptr<clang::ASTUnit> ast;

    SourceWithAST(std::string_view input, bool silent_compile = false);
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
        tmpdir = std::string { P_tmpdir } + "/thunkgentestXXXXXX";
        if (!mkdtemp(tmpdir.data())) {
            std::abort();
        }
        std::filesystem::create_directory(tmpdir);
        output_filenames = {
            tmpdir + "/thunkgen_guest",
            tmpdir + "/thunkgen_host",
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
    SourceWithAST run_thunkgen_host(std::string_view prelude, std::string_view code, GuestABI = GuestABI::X86_64, bool silent = false);
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
class HasASTMatching : public Catch::Matchers::MatcherBase<SourceWithAST> {
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

SourceWithAST::SourceWithAST(std::string_view input, bool silent_compile) : code(input) {
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

    run_tool(tool_action, code, silent_compile);
}

/**
 * Generates guest thunk library code from the given input
 */
SourceWithAST Fixture::run_thunkgen_guest(std::string_view prelude, std::string_view code, bool silent) {
    const std::string full_code = std::string { prelude } + std::string { code };

    // These tests don't deal with data layout differences, so just run data
    // layout analysis with host configuration
    auto data_layout_analysis_factory = std::make_unique<AnalyzeDataLayoutActionFactory>();
    run_tool(*data_layout_analysis_factory, full_code, silent);
    auto& data_layout = data_layout_analysis_factory->GetDataLayout();

    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames, data_layout), full_code, silent);

    std::string result =
        "#include <cstdint>\n"
        "#define MAKE_THUNK(lib, name, hash) extern \"C\" int fexthunks_##lib##_##name(void*);\n"
        "template<typename>\n"
        "struct callback_thunk_defined;\n"
        "#define MAKE_CALLBACK_THUNK(name, sig, hash) template<> struct callback_thunk_defined<sig> {};\n"
        "#define FEX_PACKFN_LINKAGE\n"
        "template<typename Target>\n"
        "Target *MakeHostTrampolineForGuestFunction(uint8_t HostPacker[32], void (*)(uintptr_t, void*), Target*);\n"
        "template<typename Target>\n"
        "Target *AllocateHostTrampolineForGuestFunction(Target*);\n";
    const auto& filename = output_filenames.guest;
    {
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
SourceWithAST Fixture::run_thunkgen_host(std::string_view prelude, std::string_view code, GuestABI guest_abi, bool silent) {
    const std::string full_code = std::string { prelude } + std::string { code };

    // These tests don't deal with data layout differences, so just run data
    // layout analysis with host configuration
    auto data_layout_analysis_factory = std::make_unique<AnalyzeDataLayoutActionFactory>();
    run_tool(*data_layout_analysis_factory, full_code, silent, guest_abi);
    auto& data_layout = data_layout_analysis_factory->GetDataLayout();

    run_tool(std::make_unique<GenerateThunkLibsActionFactory>(libname, output_filenames, data_layout), full_code, silent);

    std::string result =
        "#include <array>\n"
        "#include <cstdint>\n"
        "#include <cstring>\n"
        "#include <dlfcn.h>\n"
        "#include <type_traits>\n"
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
        "}\n"
        "#define LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(arg)\n"
        "struct GuestcallInfo {\n"
        "  uintptr_t HostPacker;\n"
        "  void (*CallCallback)(uintptr_t, uintptr_t, void*);\n"
        "  uintptr_t GuestUnpacker;\n"
        "  uintptr_t GuestTarget;\n"
        "};\n"
        "struct ParameterAnnotations {};\n"
        "template<typename, typename...>\n"
        "struct GuestWrapperForHostFunction {\n"
        "  template<ParameterAnnotations...> static void Call(void*);\n"
        "};\n"
        "struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };\n"
        "void *dlsym_default(void* handle, const char* symbol);\n"
        "template<typename T> inline constexpr bool has_compatible_data_layout = std::is_integral_v<T> || std::is_enum_v<T>;\n"
        "template<typename T>\n"
        "struct guest_layout {\n"
        "  T data;\n"
        "};\n"
        "\n"
        "template<typename T, std::size_t N>\n"
        "struct guest_layout<T[N]> {\n"
        "  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;\n"
        "  std::array<guest_layout<type>, N> data;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct guest_layout<T*> {\n"
        "#ifdef IS_32BIT_THUNK\n"
        "  using type = uint32_t;\n"
        "#else\n"
        "  using type = uint64_t;\n"
        "#endif\n"
        "  type data;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct host_layout {\n"
        "  T data;\n"
        "\n"
        "  explicit host_layout(const guest_layout<T>&);\n"
        "  template<typename U> explicit host_layout(const guest_layout<U>&) requires (std::is_integral_v<U> && sizeof(U) <= sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>);\n"
        "};\n"
        "\n"
        "template<typename T> constexpr bool is_long = std::is_same_v<T, long> || std::is_same_v<T, unsigned long>;\n"
        "template<typename T> constexpr bool is_longlong = std::is_same_v<T, long long> || std::is_same_v<T, unsigned long long>;\n"
        "template<typename T>\n"
        "struct host_layout<T*> {\n"
        "  T* data;\n"
        "  explicit host_layout(const guest_layout<T*>&);\n"
        "  template<typename U> explicit host_layout(const guest_layout<U*>&) requires (std::is_integral_v<U> && sizeof(U) == sizeof(long) && sizeof(long) == 8 && is_long<std::remove_cv_t<T>> && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>);\n"
        "  template<typename U> explicit host_layout(const guest_layout<U*>&) requires (std::is_integral_v<U> && sizeof(U) == sizeof(long long) && is_longlong<std::remove_cv_t<T>> && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>);\n"
        "  template<typename U> explicit host_layout(const guest_layout<U*>&) requires (std::is_same_v<std::remove_cv_t<T>, char> && std::is_integral_v<U> && std::is_convertible_v<T, U> && sizeof(U) == 1);\n"
        "  template<typename U> explicit host_layout(const guest_layout<U*>&) requires (std::is_same_v<std::remove_cv_t<T>, wchar_t> && std::is_integral_v<U> && std::is_convertible_v<T, U> && sizeof(U) == sizeof(wchar_t));\n"
        "};\n"
        "\n"
        "template<typename T> struct host_to_guest_convertible {\n"
        "  operator guest_layout<T>();\n"
        "  operator guest_layout<const unsigned long long*>() const requires(std::is_same_v<T, const unsigned long*>);\n"
        "  operator guest_layout<const uint8_t*>() const requires(std::is_same_v<T, const char*>);\n"
        "  operator guest_layout<uint8_t*>() const requires(std::is_same_v<T, char*>);\n"
        "  operator guest_layout<uint32_t*>() const requires(std::is_same_v<T, wchar_t*>);\n"
        "  template<typename U> operator guest_layout<U>() const requires (std::is_integral_v<U> && sizeof(U) == sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>);\n"
        "#if IS_32BIT_THUNK\n"
        "  operator guest_layout<uint32_t>() const requires(std::is_same_v<T, size_t>);\n"
        "#endif\n"
        "};\n"
        "\n"
        "template<typename T, size_t N>\n"
        "struct host_layout<T[N]> {\n"
        "  std::array<T, N> data;\n"
        "  host_layout(const guest_layout<T[N]>& from);\n"
        "};\n"
        "\n"
        "template<typename T, typename GuestT>\n"
        "struct repack_wrapper {};\n"
        "template<typename T, typename GuestT>\n"
        "repack_wrapper<T, GuestT> make_repack_wrapper(guest_layout<GuestT>& orig_arg);\n"
        "template<typename T> host_to_guest_convertible<T> to_guest(const host_layout<T>& from);\n"
        "template<typename F> void FinalizeHostTrampolineForGuestFunction(F*);\n"
        "template<typename F> void FinalizeHostTrampolineForGuestFunction(guest_layout<F*>);\n"
        "template<typename T> T& unwrap_host(host_layout<T>&);\n"
        "template<typename T, typename GuestT> T* unwrap_host(repack_wrapper<T*, GuestT>&);\n"
        "template<typename T> const host_layout<T>& to_host_layout(const T& t);\n";

    auto& filename = output_filenames.host;
    {
        std::ifstream file(filename);
        const auto prelude_size = result.size();
        const auto new_data_size = std::filesystem::file_size(filename);
        result.resize(result.size() + new_data_size);
        file.read(result.data() + prelude_size, result.size());

        // Force all functions to be non-static, since having to define them
        // would add a lot of noise to simple tests.
        while (true) {
            auto pos = result.find("static ", prelude_size);
            if (pos == std::string::npos) {
                break;
            }
            result.replace(pos, 6, "      "); // Replace "static" with 6 spaces (avoiding reallocation)
        }
    }
    return SourceWithAST { std::string { prelude } + result, silent };
}

Fixture::GenOutput Fixture::run_thunkgen(std::string_view prelude, std::string_view code, bool silent) {
    return { run_thunkgen_guest(prelude, code, silent),
             run_thunkgen_host(prelude, code, GuestABI::X86_64, silent) };
}

#if CLANG_VERSION_MAJOR <= 15
// Old clang versions require an explicit "struct" prefix
#define CLANG_STRUCT_PREFIX "struct "
#define asStructString(name) asString(CLANG_STRUCT_PREFIX name)
#else
#define CLANG_STRUCT_PREFIX
#define asStructString(name) asString(name)
#endif

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
            hasType(constantArrayType(hasElementType(asStructString("ExportEntry")), hasSize(2))),
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

TEST_CASE_METHOD(Fixture, "FunctionPointerViaType") {
    const auto output = run_thunkgen("",
        "template<typename> struct fex_gen_type {};\n"
        "template<> struct fex_gen_type<int(char, char)> {};\n");

    // Guest should apply MAKE_CALLBACK_THUNK to this signature
    CHECK_THAT(output.guest,
        matches(classTemplateSpecializationDecl(
            // Should have signature matching input function
            hasName("callback_thunk_defined"),
            hasTemplateArgument(0, refersToType(asString("int (char, char)")))
        )));

    // Host should export the unpacking function for callback arguments
    CHECK_THAT(output.host,
        matches(varDecl(
            hasName("exports"),
            hasType(constantArrayType(hasElementType(asStructString("ExportEntry")), hasSize(2))),
            hasInitializer(hasDescendant(declRefExpr(to(cxxMethodDecl(hasName("Call"), ofClass(hasName("GuestWrapperForHostFunction"))).bind("funcptr")))))
            )).check_binding("funcptr", +[](const clang::CXXMethodDecl* decl) {
                auto parent = llvm::cast<clang::ClassTemplateSpecializationDecl>(decl->getParent());
                return parent->getTemplateArgs().get(0).getAsType().getAsString() == "int (unsigned char, unsigned char)";
            }));
}

// Parameter is a function pointer
TEST_CASE_METHOD(Fixture, "FunctionPointerParameter") {
    const auto output = run_thunkgen("",
        "void func(int (*funcptr)(char, char));\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> {};\n");

    CHECK_THAT(output.guest,
        matches(functionDecl(
            // Should have signature matching input function
            hasName("fexfn_pack_func"),
            returns(asString("void")),
            parameterCountIs(1),
            hasParameter(0, hasType(asString("int (*)(char, char)")))
        )));

    // Host packing function should call FinalizeHostTrampolineForGuestFunction on the argument
    CHECK_THAT(output.host,
        matches(functionDecl(
            hasName("fexfn_unpack_libtest_func"),
            hasDescendant(callExpr(callee(functionDecl(hasName("FinalizeHostTrampolineForGuestFunction"))), hasArgument(0, expr().bind("funcptr"))))
        )).check_binding("funcptr", +[](const clang::Expr* funcptr) {
            // Check that the argument type matches the function pointer
            return funcptr->getType().getAsString() == "guest_layout<int (*)(char, char)>";
        }));

    // Host should export the unpacking function for function pointer arguments
    CHECK_THAT(output.host,
        matches(varDecl(
            hasName("exports"),
            hasType(constantArrayType(hasElementType(asStructString("ExportEntry")), hasSize(3))),
            hasInitializer(hasDescendant(declRefExpr(to(cxxMethodDecl(hasName("Call"), ofClass(hasName("GuestWrapperForHostFunction")))))))
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
            hasParameter(3, hasType(asStructString("TestStruct")))
        )));

    // Host code
    CHECK_THAT(output.host,
        matches(varDecl(
            hasName("exports"),
            hasType(constantArrayType(hasElementType(asStructString("ExportEntry")), hasSize(2))),
            hasInitializer(initListExpr(hasInit(0, expr()),
                                        hasInit(1, initListExpr(hasInit(0, implicitCastExpr()), hasInit(1, implicitCastExpr())))))
            // TODO: check null termination
            )));

    CHECK_THAT(output.host,
        matches(functionDecl(
            hasName("fexfn_unpack_libtest_func"),
            // Packed argument struct should contain all parameters
            parameterCountIs(1),
            hasParameter(0, hasType(pointerType(pointee(hasUnqualifiedDesugaredType(
                recordType(hasDeclaration(decl(
                    has(fieldDecl(hasType(asString("guest_layout<int32_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<uint8_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<uint64_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<" CLANG_STRUCT_PREFIX "TestStruct>"))))
                    ))))))))
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

// Tests generation of guest_layout/host_layout wrappers and related helpers
TEST_CASE_METHOD(Fixture, "LayoutWrappers") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    const auto host_layout_is_trivial =
        matches(classTemplateSpecializationDecl(
            hasName("host_layout"),
            hasAnyTemplateArgument(refersToType(asString("struct A"))),
            has(fieldDecl(hasName("data"), hasType(hasCanonicalType(asString("struct A")))))
        ));
    const auto layout_undefined = [](const char* type) {
        return matches(classTemplateSpecializationDecl(
              hasName(type),
              hasAnyTemplateArgument(refersToType(asString("struct A")))
        ).bind("layout")).check_binding("layout", +[](const clang::ClassTemplateSpecializationDecl* decl) {
            return !decl->isCompleteDefinition();
        });
    };
    const auto guest_converter_defined =
          matches(functionDecl(hasName("to_guest"),
                // Parameter is a host_layout<A> (ignoring qualifiers and references)
                hasParameter(0, hasType(references(classTemplateSpecializationDecl(hasName("host_layout"), hasAnyTemplateArgument(refersToType(asString("struct A"))))))),
                // Return value is a guest_layout<A>
                returns(asString("guest_layout<" CLANG_STRUCT_PREFIX "A>"))));
    const auto guest_converter_undefined =
          matches(functionDecl(hasName("to_guest"),
                // Parameter is a host_layout<A> (ignoring qualifiers and references)
                hasParameter(0, hasType(references(classTemplateSpecializationDecl(hasName("host_layout"), hasAnyTemplateArgument(refersToType(asString("struct A"))))))),
                isDeleted()));

    const std::string code =
        "template<typename> struct fex_gen_type {};\n"
        "template<> struct fex_gen_type<A> {};\n";

    // For fully compatible types, both guest_layout and host_layout directly
    // reference the original struct
    SECTION("Fully compatible type") {
        const char* struct_def = "struct A { int a; int b; };\n";
        const auto output = run_thunkgen_host(struct_def, code, guest_abi);
        CHECK_THAT(output,
            matches(classTemplateSpecializationDecl(
                  hasName("guest_layout"),
                  hasAnyTemplateArgument(refersToType(asString("struct A"))),
                  has(fieldDecl(hasName("data"), hasType(hasCanonicalType(asString("struct A")))))
            )));
        CHECK_THAT(output, guest_converter_defined);

        CHECK_THAT(output, host_layout_is_trivial);
    }

    // For repackable types, guest_layout explicitly lists its members
    SECTION("Repackable type") {
        const char* struct_def =
            "#ifdef HOST\n"
            "struct A { int a; int b; };\n"
            "#else\n"
            "struct A { int b; int a; };\n"
            "#endif\n";
        const auto output = run_thunkgen_host(struct_def, code, guest_abi);
        CHECK_THAT(output,
            matches(classTemplateSpecializationDecl(
                  hasName("guest_layout"),
                  hasAnyTemplateArgument(refersToType(asString("struct A"))),
                  // The member "data" exists and is defined to a struct...
                  has(fieldDecl(hasName("data"), hasType(hasCanonicalType(hasDeclaration(decl(
                      // ... the members of which also use guest_layout (with fixed-size integers)
                      has(fieldDecl(hasName("a"), hasType(asString("guest_layout<int32_t>")))),
                      has(fieldDecl(hasName("b"), hasType(asString("guest_layout<int32_t>"))))
                      ))))))
            )));
        CHECK_THAT(output, guest_converter_defined);

        CHECK_THAT(output, host_layout_is_trivial);
    }

    // For incompatible types, use of guest_layout nor host_layout should be prohibited
    SECTION("Incompatible type, unannotated") {
        const char* struct_def =
            "#ifdef HOST\n"
            "struct A { int a; int b; };\n"
            "#else\n"
            "struct A { int c; int d; };\n"
            "#endif\n";
        const auto output = run_thunkgen_host(struct_def, code, guest_abi);
        CHECK_THAT(output, layout_undefined("guest_layout"));
        CHECK_THAT(output, guest_converter_undefined);
        CHECK_THAT(output, layout_undefined("host_layout"));
    }

    // Layout wrappers can be enabled even for incompatible types using the emit_layout_wrappers annotation
    SECTION("Incompatible type, annotated") {
        // A slightly different setup is used here in order to construct a type which...
        // - has incompatible data layout (for both 32-bit and 64-bit guests)
        // - has consistently named members in struct A (which is required to emit layout wrappers)
        const char* struct_def =
            "#ifdef HOST\n"
            "struct B { int a; };\n"
            "#else\n"
            "struct B { int b; };\n"
            "#endif\n"
            "struct A { B* a; int b; };\n";
        const std::string code =
            "#include <thunks_common.h>\n"
            "template<typename> struct fex_gen_type {};\n"
            "template<> struct fex_gen_type<A> : fexgen::emit_layout_wrappers {};\n";
        const auto output = run_thunkgen_host(struct_def, code, guest_abi);
        CHECK_THAT(output,
            matches(classTemplateSpecializationDecl(
                  hasName("guest_layout"),
                  hasAnyTemplateArgument(refersToType(recordType(hasDeclaration(recordDecl(hasName("A")))))),
                  // The member "data" exists and is defined to a struct...
                  has(fieldDecl(hasName("data"), hasType(hasCanonicalType(hasDeclaration(decl(
                      // ... the members of which also use guest_layout
                      has(fieldDecl(hasName("a"), hasType(asString("guest_layout<" CLANG_STRUCT_PREFIX "B *>")))),
                      has(fieldDecl(hasName("b"), hasType(asString("guest_layout<int32_t>"))))
                      ))))))
            )));
        CHECK_THAT(output, guest_converter_defined);

        CHECK_THAT(output, host_layout_is_trivial);
    }
}

// Some integer types are differently sized on the guest than on the host.
// All integer types are mapped to fixed-size equivalents when mentioned in a
// guest context hence. This test ensures the mapping is done correctly and
// the resulting guest_layout instantiations are convertible to host_layout.
TEST_CASE_METHOD(Fixture, "Mapping guest integers to fixed-size") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    // Run each test a second time to ensure fixed-size integer mapping is
    // applied to pointees as well
    const std::string ptr = GENERATE("", " *");

    // Run each test with and without the ptr_passthrough annotation
    const bool passthrough_guest_type = GENERATE(false, true);

    // These types are differently sized on 32-bit guests
    SECTION("(u)intptr_t / size_t / long") {
        const std::string type = GENERATE("long", "unsigned long", "uintptr_t", "intptr_t", "size_t");
        INFO(type + ptr);
        const auto code =
            "#include <thunks_common.h>\n"
            "#include <cstddef>\n"
            "#include <cstdint>\n"
            "void func(" + type + ptr + ");\n"
            "template<> struct fex_gen_config<func> : fexgen::custom_host_impl {};\n" +
            (passthrough_guest_type ? "template<> struct fex_gen_param<func, 0> : fexgen::ptr_passthrough {};\n" : "");
        if (!ptr.empty() && guest_abi == GuestABI::X86_32 && !passthrough_guest_type) {
            // Guest points to a 32-bit integer, but the host to a 64-bit one.
            // This should be detected as a failure.
            CHECK_THROWS_WITH(run_thunkgen_host("", code, guest_abi, true), Catch::Matchers::ContainsSubstring("initialization of 'host_layout", Catch::CaseSensitive::No));
        } else {
            const auto output = run_thunkgen_host("", code, guest_abi);
            std::string expected_type = "guest_layout<";
            if (type == "size_t" || type.starts_with("u")) {
                expected_type += "u";
            }
            expected_type += (guest_abi == GuestABI::X86_32 ? + "int32_t" : "int64_t");
            expected_type += ptr + ">";
            CHECK_THAT(output,
                matches(functionDecl(
                    hasName("fexfn_unpack_libtest_func"),
                    // Packed argument struct should contain all parameters
                    parameterCountIs(1),
                    hasParameter(0, hasType(pointerType(pointee(hasUnqualifiedDesugaredType(
                        recordType(hasDeclaration(decl(
                            has(fieldDecl(hasType(asString(expected_type))))
                    ))))))))
                )));

            // For passthrough parameters, the target function signature should
            // match the guest_layout type
            if (passthrough_guest_type) {
                CHECK_THAT(output,
                    matches(functionDecl(
                        hasName("fexfn_impl_libtest_func"),
                        parameterCountIs(1),
                        hasParameter(0, hasType(asString(expected_type)))
                    )));
            }
        }
    }

    // Most integer types are uniquely defined by specifying their size and
    // their signedness. (w)char-types and long-types are special:
    // * "char", "signed char", and "unsigned char" are different types
    // * "wchar_t" is mapped to "guest_layout<uint32_t>", but uint32_t
    //   itself is a type alias for "int"
    // * "long long" is mapped to "guest_layout<uint64_t>", but uint64_t
    //   itself is a type alias for "long" on 64-bit (which is a different
    //   type than "long long")
    //
    // This test section ensures that the correct fixed-size integers are used
    // *and* that they can be converted to host_layout.
    SECTION("Special integer types") {
        const std::string type = GENERATE("long long", "unsigned long long", "char", "unsigned char", "signed char", "wchar_t");
        std::string fixed_size_type = ((type.starts_with("unsigned") || type == "char" || type == "wchar_t") ? "u" : "");
        if (type.ends_with("long long")) {
            fixed_size_type += "int64_t";
        } else if (type.ends_with("wchar_t")) {
            fixed_size_type += "int32_t";
        } else {
            fixed_size_type += "int8_t";
        }
        INFO(type + ptr + ", expecting " + fixed_size_type + ptr);
        const auto code =
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n"
            "void func(" + type + ptr + ");\n"
            "template<> struct fex_gen_config<func> : fexgen::custom_host_impl {};\n" +
            (passthrough_guest_type ? "template<> struct fex_gen_param<func, 0> : fexgen::ptr_passthrough {};\n" : "");
        const auto output = run_thunkgen_host("", code, guest_abi);
        CHECK_THAT(output,
            matches(functionDecl(
                hasName("fexfn_unpack_libtest_func"),
                // Packed argument struct should contain all parameters
                parameterCountIs(1),
                hasParameter(0, hasType(pointerType(pointee(hasUnqualifiedDesugaredType(
                    recordType(hasDeclaration(decl(
                        has(fieldDecl(hasType(asString("guest_layout<" + fixed_size_type + ptr + ">"))))
                ))))))))
            )));

        // For passthrough parameters, the target function signature should
        // match the guest_layout type
        if (passthrough_guest_type) {
            CHECK_THAT(output,
                matches(functionDecl(
                    hasName("fexfn_impl_libtest_func"),
                    parameterCountIs(1),
                    hasParameter(0, hasType(asString("guest_layout<" + fixed_size_type + ptr + ">")))
                )));
        }
    }
}

TEST_CASE_METHOD(Fixture, "StructRepacking") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    // All tests use the same function, but the prelude defining its parameter type "A" varies
    const std::string code =
        "#include <thunks_common.h>\n"
        "void func(A*);\n"
        "template<auto> struct fex_gen_config {};\n"
        "template<> struct fex_gen_config<func> : fexgen::custom_host_impl {};\n";

    SECTION("Pointer to struct with consistent data layout") {
        CHECK_NOTHROW(run_thunkgen_host("struct A { int a; };\n", code, guest_abi));
    }

    SECTION("Pointer to struct with unannotated pointer member with inconsistent data layout") {
        const auto prelude =
            "#ifdef HOST\n"
            "struct B { int a; };\n"
            "#else\n"
            "struct B { int b; };\n"
            "#endif\n"
            "struct A { B* a; };\n";

        SECTION("Parameter unannotated") {
            CHECK_THROWS(run_thunkgen_host(prelude, code, guest_abi, true));
        }

        SECTION("Parameter annotated as ptr_passthrough") {
            CHECK_NOTHROW(run_thunkgen_host(prelude, code + "template<> struct fex_gen_param<func, 0, A*> : fexgen::ptr_passthrough {};\n", guest_abi));
        }

        SECTION("Struct member annotated as custom_repack") {
            CHECK_NOTHROW(run_thunkgen_host("struct A { void* a; };\n",
                  code + "template<> struct fex_gen_config<&A::a> : fexgen::custom_repack {};\n", guest_abi));
        }
    }

    SECTION("Pointer to struct with pointer member of consistent data layout") {
        std::string type = GENERATE("char", "short", "int", "float");
        REQUIRE_NOTHROW(run_thunkgen_host("struct A { " + type + "* a; };\n", code, guest_abi));
    }

    SECTION("Pointer to struct with pointer member of opaque type") {
        const auto prelude =
            "struct B;\n"
            "struct A { B* a; };\n";

        // Unannotated
        REQUIRE_THROWS_WITH(run_thunkgen_host(prelude, code, guest_abi), Catch::Matchers::ContainsSubstring("incomplete type"));

        // Annotated as opaque_type
        CHECK_NOTHROW(run_thunkgen_host(prelude,
              code + "template<> struct fex_gen_type<B> : fexgen::opaque_type {};\n", guest_abi));
    }
}

TEST_CASE_METHOD(Fixture, "VoidPointerParameter") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    SECTION("Unannotated") {
        const char* code =
            "#include <thunks_common.h>\n"
            "void func(void*);\n"
            "template<> struct fex_gen_config<func> {};\n";
        if (guest_abi == GuestABI::X86_32) {
            // TODO: Currently not considered an error
//            CHECK_THROWS_WITH(run_thunkgen_host("", code, guest_abi, true), Catch::Matchers::ContainsSubstring("unsupported parameter type", Catch::CaseSensitive::No));
        } else {
            // Pointee data is assumed to be compatible on 64-bit
            CHECK_NOTHROW(run_thunkgen_host("", code, guest_abi));
        }
    }

    SECTION("Passthrough") {
        const char* code =
            "#include <thunks_common.h>\n"
            "void func(void*);\n"
            "template<> struct fex_gen_config<func> : fexgen::custom_host_impl {};\n"
            "template<> struct fex_gen_param<func, 0, void*> : fexgen::ptr_passthrough {};\n";
        CHECK_NOTHROW(run_thunkgen_host("", code, guest_abi));
    }

    SECTION("Assumed compatible") {
        const char* code =
            "#include <thunks_common.h>\n"
            "void func(void*);\n"
            "template<> struct fex_gen_config<func> {};\n"
            "template<> struct fex_gen_param<func, 0, void*> : fexgen::assume_compatible_data_layout {};\n";
        CHECK_NOTHROW(run_thunkgen_host("", code, guest_abi));
    }

    SECTION("Unannotated in struct") {
        const char* prelude =
            "struct A { void* a; };\n";
        const char* code =
            "#include <thunks_common.h>\n"
            "void func(A*);\n"
            "template<> struct fex_gen_config<func> {};\n";
        if (guest_abi == GuestABI::X86_32) {
            CHECK_THROWS_WITH(run_thunkgen_host(prelude, code, guest_abi, true), Catch::Matchers::ContainsSubstring("unsupported parameter type", Catch::CaseSensitive::No));
        } else {
            CHECK_NOTHROW(run_thunkgen_host(prelude, code, guest_abi));
        }
    }

    SECTION("Custom repack in struct") {
        const char* prelude =
            "struct A { void* a; };\n";
        const char* code =
            "#include <thunks_common.h>\n"
            "void func(A*);\n"
            "template<> struct fex_gen_config<&A::a> : fexgen::custom_repack {};\n"
            "template<> struct fex_gen_config<func> {};\n";
        CHECK_NOTHROW(run_thunkgen_host(prelude, code, guest_abi));
    }
}
