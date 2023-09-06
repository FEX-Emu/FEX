#include <catch2/catch.hpp>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
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
SourceWithAST Fixture::run_thunkgen_host(std::string_view prelude, std::string_view code, bool silent) {
    const std::string full_code = std::string { prelude } + std::string { code };

    // These tests don't deal with data layout differences, so just run data
    // layout analysis with host configuration
    auto data_layout_analysis_factory = std::make_unique<AnalyzeDataLayoutActionFactory>();
    run_tool(*data_layout_analysis_factory, full_code, silent);
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
        "struct ParameterAnnotations {\n"
        "    bool is_passthrough = false;\n"
        "    bool is_opaque = false;\n"
        "};\n"
        "template<typename, typename...>\n"
        "struct GuestWrapperForHostFunction {\n"
        "  template<ParameterAnnotations...> static void Call(void*);\n"
        "};\n"
        "struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };\n"
        "void *dlsym_default(void* handle, const char* symbol);\n"
        "template<typename>\n"
        "struct pmd_traits;\n"
        "template<typename Parent, typename Data>\n"
        "struct pmd_traits<Data Parent::*> {\n"
        "    using parent_t = Parent;\n"
        "    using member_t = Data;\n"
        "};\n"
        "template<typename T> inline constexpr bool has_compatible_data_layout = std::is_integral_v<T> || std::is_enum_v<T>;\n"
        "template<typename T>\n"
        "struct guest_layout {\n"
        "  static_assert(!std::is_class_v<T>, \"No guest layout defined for this non-opaque struct type. This may be a bug in the thunk generator.\");\n"
        "  static_assert(!std::is_union_v<T>, \"No guest layout defined for this non-opaque union type. This may be a bug in the thunk generator.\");\n"
        "\n"
        "  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;\n"
        "  type data;\n"
        "\n"
        "  guest_layout& operator=(const T from);\n"
        "};\n"
        "\n"
        "template<typename T, std::size_t N>\n"
        "struct guest_layout<T[N]> {\n"
        "  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;\n"
        "  std::array<guest_layout<type>, N> data;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct host_layout;\n"
        "\n"
        "template<typename T>\n"
        "struct guest_layout<T*> {\n"
        "#ifdef IS_32BIT_THUNK\n"
        "  using type = uint32_t;\n"
        "#else\n"
        "  using type = uint64_t;\n"
        "#endif\n"
        "  type data;\n"
        "\n"
        "  guest_layout& operator=(const T* from);\n"
        "  guest_layout<T>* get_pointer();\n"
        "  const guest_layout<T>* get_pointer() const;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct guest_layout<T* const> {\n"
        "#ifdef IS_32BIT_THUNK\n"
        "  using type = uint32_t;\n"
        "#else\n"
        "  using type = uint64_t;\n"
        "#endif\n"
        "  type data;\n"
        "\n"
        "  guest_layout& operator=(const T* from);\n"
        "\n"
        "  guest_layout<T>* get_pointer();\n"
        "  const guest_layout<T>* get_pointer() const;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct host_layout {\n"
        "  T data;\n"
        "\n"
        "  template<typename U>\n"
        "  host_layout(const guest_layout<U>& from) requires(!std::is_integral_v<T> || sizeof(T) == sizeof(U));\n"
        "};\n"
        "\n"
        "// Specialization for size_t, which is 64-bit on 64-bit but 32-bit on 32-bit\n"
        "template<>\n"
        "struct host_layout<size_t> {\n"
        "  size_t data;\n"
        "\n"
        "  host_layout(const guest_layout<uint32_t>& from);\n"
        "  host_layout(const guest_layout<size_t>& from);\n"
        "};\n"
        "\n"
        "template<typename T, size_t N>\n"
        "struct host_layout<T[N]> {\n"
        "  std::array<T, N> data;\n"
        "  host_layout(const guest_layout<T[N]>& from);\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct host_layout<T*> {\n"
        "  T* data;\n"
        "\n"
        "  static_assert(!std::is_function_v<T>, \"Function types must be handled separately\");\n"
        "\n"
        "  host_layout(const guest_layout<T*>& from);\n"
        "\n"
        "  host_layout() = default;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct host_layout<T* const> {\n"
        "  T* data;\n"
        "\n"
        "  static_assert(!std::is_function_v<T>, \"Function types must be handled separately\");\n"
        "\n"
        "  // Assume underlying data is compatible and just convert the guest-sized pointer to 64-bit\n"
        "  host_layout(const guest_layout<T* const>& from);\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct unpacked_arg {\n"
        "  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;\n"
        "  host_layout<type> data;\n"
        "  type get();\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct unpacked_arg<T*> {\n"
        "  unpacked_arg(const guest_layout<T*>&);\n"
        "  unpacked_arg(const guest_layout<const T*>&);\n"
        "\n"
        "  T* get();\n"
        "  host_layout<T*> data;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct unpacked_arg_base;\n"
        "template<typename T>\n"
        "struct unpacked_arg_base<T*> {\n"
        "  unpacked_arg_base(host_layout<T>*);\n"
        "  unpacked_arg_base(host_layout<const T>*);\n"
        "\n"
        "  T* get();\n"
        "  host_layout<T>* data;\n"
        "};\n"
        "\n"
        "template<typename T>\n"
        "struct unpacked_arg_with_storage;\n"
        "template<typename T>\n"
        "struct unpacked_arg_with_storage<T*> : unpacked_arg_base<T*> {\n"
        "  unpacked_arg_with_storage(const guest_layout<T*>&);\n"
        "  unpacked_arg_with_storage(const guest_layout<const T*>&);\n"
        "};\n"
        "template<>\n"
        "struct unpacked_arg_with_storage<char*> {\n"
        "  using type = char*;\n"
        "\n"
        "  unpacked_arg_with_storage(guest_layout<const char*>& data);\n"
        "  unpacked_arg_with_storage(guest_layout<char*>& data);\n"
        "\n"
        "  type get();\n"
        "  uint64_t data;\n"
        "};\n"
        "\n"
        "template<typename T> guest_layout<T> to_guest(const host_layout<T>& from) requires(!std::is_pointer_v<T>);\n"
        "template<typename T> guest_layout<T*> to_guest(const host_layout<T*>& from);\n"
        "template<typename F> void FinalizeHostTrampolineForGuestFunction(F*);\n"
        "template<typename F> void FinalizeHostTrampolineForGuestFunction(const guest_layout<F*>&);\n"
        "template<typename T> T& unwrap_host(host_layout<T>&);\n"
        "template<typename T> T* unwrap_host(unpacked_arg_base<T*>&);\n";

    auto& filename = output_filenames.host;
    {
        std::ifstream file(filename);
        const auto current_size = result.size();
        const auto new_data_size = std::filesystem::file_size(filename);
        result.resize(result.size() + new_data_size);
        file.read(result.data() + current_size, result.size());
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
            hasType(constantArrayType(hasElementType(asString("struct ExportEntry")), hasSize(2))),
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
            hasType(constantArrayType(hasElementType(asString("struct ExportEntry")), hasSize(3))),
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
                    has(fieldDecl(hasType(asString("guest_layout<int32_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<uint8_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<uint64_t>")))),
                    has(fieldDecl(hasType(asString("guest_layout<struct TestStruct>"))))
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
