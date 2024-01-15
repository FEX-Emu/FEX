#include <clang/Frontend/CompilerInstance.h>
#include <catch2/catch.hpp>

#include <data_layout.h>
#include <interface.h>
#include "common.h"

#include <fmt/format.h>

#include <string_view>

// run_tool will leak memory when the ToolAction throws an exception, so
// disable AddressSanitizer's leak detection
const char* __asan_default_options() {
    return "detect_leaks=0";
}

inline std::ostream& operator<<(std::ostream& os, TypeCompatibility compat) {
    if (compat == TypeCompatibility::Full) {
        os << "Compatible";
    } else if (compat == TypeCompatibility::Repackable) {
        os << "Repackable";
    } else if (compat == TypeCompatibility::None) {
        os << "Incompatible";
    } else {
        os << "(INVALID)";
    }
    return os;
}

class DataLayoutCompareActionForTest;

namespace {

struct Fixture {
    /**
     * Parses annotations from the input source and generates data layout descriptions from it.
     *
     * Input code with common definitions (types, functions, ...) should be specified in "prelude".
     * It will be prepended to "code" before processing and also to the generator output.
     */
    std::unique_ptr<DataLayoutCompareActionForTest> compute_data_layout(std::string_view prelude, std::string_view code, GuestABI);
};

}

class DataLayoutCompareActionForTest : public DataLayoutCompareAction {
    std::unordered_map<const clang::Type*, TypeCompatibility> type_compat_cache;

    // Persistent reference taken to enable accessing the ASTContext after CompilerInstance::ExecuteAction returns
    llvm::IntrusiveRefCntPtr<clang::ASTContext> ast_context;
    std::shared_ptr<clang::Preprocessor> preprocessor;

public:
    DataLayoutCompareActionForTest(std::unique_ptr<ABI> guest_layout) : DataLayoutCompareAction(*guest_layout), guest_layout(std::move(guest_layout)) {
    }

    void ExecuteAction() override {
        AnalysisAction::ExecuteAction();

        ast_context = &getCompilerInstance().getASTContext();
        preprocessor = getCompilerInstance().getPreprocessorPtr();
        host_layout = ComputeDataLayout(*ast_context, types);
    }

    std::unique_ptr<ABI> guest_layout;
    std::unordered_map<const clang::Type*, TypeInfo> host_layout;

    TypeCompatibility GetTypeCompatibility(std::string_view type_name) {
        for (const auto& [type, _] : host_layout) {
            if (clang::QualType { type, 0 }.getAsString() == type_name) {
                return DataLayoutCompareAction::GetTypeCompatibility(*ast_context, type, host_layout, type_compat_cache);
            }
        }

        throw std::runtime_error("No data layout information recorded for type \"" + std::string { type_name } + "\"");
    }
};

/**
 * Same as clang::FrontendActionFactory but takes an external FrontendAction
 * reference instead of constructing an internal one. Since the FrontendAction
 * lifetime may extend past this ToolAction, state captured by the
 * FrontendAction can be accessed after the ToolAction returns.
 */
class ThunkTestToolAction : public clang::tooling::ToolAction {
public:
    clang::FrontendAction& ScopedToolAction;

public:
    ThunkTestToolAction(clang::FrontendAction& action) : ScopedToolAction(action) {
    }
    ~ThunkTestToolAction() = default;

    // Same as FrontendActionFactory but keeps ScopedToolAction alive when returning
    bool runInvocation( std::shared_ptr<clang::CompilerInvocation> invocation, clang::FileManager *files,
                        std::shared_ptr<clang::PCHContainerOperations> pch,
                        clang::DiagnosticConsumer *diag_consumer) override {

        auto diagnostics = clang::CompilerInstance::createDiagnostics(&invocation->getDiagnosticOpts(), diag_consumer, false);

        clang::CompilerInstance Compiler(std::move(pch));
        Compiler.setInvocation(std::move(invocation));
        Compiler.setFileManager(files);
        Compiler.createDiagnostics(diag_consumer, false);
        if (!Compiler.hasDiagnostics())
          return false;
        Compiler.createSourceManager(*files);

        const bool Success = Compiler.ExecuteAction(ScopedToolAction);

        files->clearStatCache();
        return Success;
    }
};

std::unique_ptr<DataLayoutCompareActionForTest> Fixture::compute_data_layout(std::string_view prelude, std::string_view code, GuestABI guest_abi) {
    const std::string full_code = std::string { prelude } + std::string { code };

    // Compute guest data layout
    auto data_layout_analysis_factory = std::make_unique<AnalyzeDataLayoutActionFactory>();
    run_tool(*data_layout_analysis_factory, full_code, false, guest_abi);

    // Compute host data layout
    auto ScopedToolAction = std::make_unique<DataLayoutCompareActionForTest>(data_layout_analysis_factory->TakeDataLayout());
    run_tool(std::make_unique<ThunkTestToolAction>(*ScopedToolAction), full_code, false, std::nullopt);

    return ScopedToolAction;
}

static std::string FormatDataLayout(const std::unordered_map<const clang::Type*, TypeInfo>& layout) {
     std::string ret;

    for (const auto& [type, info] : layout) {
        auto basic_info = info.get_if_simple_or_struct();
        if (!basic_info) {
            continue;
        }

        ret += fmt::format("  Host entry {}: {} ({})\n", clang::QualType { type, 0 }.getAsString().c_str(), basic_info->size_bits / 8, basic_info->alignment_bits / 8);

        if (auto struct_info = info.get_if_struct()) {
            for (const auto& member : struct_info->members) {
                ret += fmt::format("    Offset {}-{}: {} {}{}\n", member.offset_bits / 8, (member.offset_bits + member.size_bits - 1) / 8, member.type_name.c_str(), member.member_name.c_str(), member.array_size ? fmt::format("[{}]", member.array_size.value()).c_str() : "");
            }
        }
    }

    return ret;
}

TEST_CASE_METHOD(Fixture, "DataLayout") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    SECTION("Trivial") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n",
            "struct A { int a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        REQUIRE(action->guest_layout->contains("A"));

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
    }

    SECTION("Builtin types") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n",
            "struct A { char a; short b; int c; float d; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        REQUIRE(action->guest_layout->contains("A"));

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("char") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("short") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("int") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("float") == TypeCompatibility::Full);
    }

    SECTION("Padding after int16_t") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int16_t a; int32_t b; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
    }

    SECTION("Array of int16_t") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int16_t a[64]; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
    }

    const auto compat_full64_repackable32 = (guest_abi == GuestABI::X86_32 ? TypeCompatibility::Repackable : TypeCompatibility::Full);

    SECTION("Type with platform-dependent size (size_t)") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdlib>\n",
            "struct A { size_t a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("int64_t has stricter alignment requirements on 64-bit platforms") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int64_t a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->alignment_bits == (guest_abi == GuestABI::X86_32 ? 32 : 64));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Array of int64_t") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int64_t a[64]; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("int64_t with explicit alignment specification") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct alignas(8) A { int64_t a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->alignment_bits == 64);
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
    }

    SECTION("int64_t alignment requirements propagate to parent struct") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int32_t a; int32_t b; int64_t c; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->alignment_bits == (guest_abi == GuestABI::X86_32 ? 32 : 64));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Padding before int64_t member") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int32_t a; int64_t b; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->members[1].offset_bits == (guest_abi == GuestABI::X86_32 ? 32 : 64));

        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Padding at end of struct due to int64_t alignment (like VkMemoryHeap)") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { int64_t a; int32_t b; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->size_bits == (guest_abi == GuestABI::X86_32 ? 96 : 128));

        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Different struct definition between guest and host; different member order") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct A { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct A { int32_t b; int32_t a; };\n"
            "#endif\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->members.at(0).member_name == "b");
        CHECK(action->guest_layout->at("A").get_if_struct()->members.at(1).member_name == "a");

        REQUIRE(!action->host_layout.empty());
        CHECK(action->host_layout.begin()->second.get_if_struct()->members.at(0).member_name == "a");
        CHECK(action->host_layout.begin()->second.get_if_struct()->members.at(1).member_name == "b");

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
    }

    SECTION("Different struct definition between guest and host; different member size") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct A { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct A { int32_t a; int64_t b; };\n"
            "#endif\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->guest_layout->at("A").get_if_struct()->members.at(0).size_bits == 32);
        CHECK(action->guest_layout->at("A").get_if_struct()->members.at(1).size_bits == 64);

        REQUIRE(!action->host_layout.empty());
        CHECK(action->host_layout.begin()->second.get_if_struct()->members.at(0).size_bits == 32);
        CHECK(action->host_layout.begin()->second.get_if_struct()->members.at(1).size_bits == 32);

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
    }

    SECTION("Different struct definition between guest and host; completely different members") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct A { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct A { int32_t c; int32_t d; };\n"
            "#endif\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::None);
    }

    SECTION("Different struct definition between guest and host; member missing from guest") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct A { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct A { int32_t a; };\n"
            "#endif\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::None);
    }

    SECTION("Different struct definition between guest and host; member missing from host") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct A { int32_t a; };\n"
            "#else\n"
            "struct A { int32_t a; int32_t b; };\n"
            "#endif\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));

        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::None);
    }

    SECTION("Nesting structs of consistent data layout") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct C { int32_t a; int16_t b; };\n"
            "struct B { C a; int16_t b; };\n"
            "struct A { int32_t a; B b; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        REQUIRE(action->guest_layout->contains("B"));
        REQUIRE(action->guest_layout->contains("C"));
        CHECK(action->guest_layout->at("A").get_if_struct()->members.at(0).size_bits == 32);

        CHECK(action->GetTypeCompatibility("struct C") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
    }

    SECTION("Nesting repackable structs by embedding") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct C { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct C { int32_t b; int32_t a; };\n"
            "#endif\n"
            "struct B { C a; int16_t b; };\n"
            "struct A { int32_t a; B b; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        REQUIRE(action->guest_layout->contains("B"));
        REQUIRE(action->guest_layout->contains("C"));
        CHECK(action->guest_layout->at("A").get_if_struct()->size_bits == 128);
        CHECK(action->guest_layout->at("A").get_if_struct()->alignment_bits == 32);

        CHECK(action->GetTypeCompatibility("struct C") == TypeCompatibility::Repackable);
        CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Repackable);
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
    }

    SECTION("Embedded union type (like VkRenderingAttachmentInfo)") {
        SECTION("without annotation") {
            CHECK_THROWS_WITH(compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "union B { int32_t a; uint32_t b; };\n"
                "struct A { B a; };\n"
                "template<> struct fex_gen_type<A> {};\n", guest_abi),
                Catch::Contains("unannotated member") && Catch::Contains("union type"));
        }

        SECTION("with annotation") {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "union B { int32_t a; uint32_t b; };\n"
                "struct A { B a; };\n"
                "template<> struct fex_gen_type<B> : fexgen::assume_compatible_data_layout {};\n"
                "template<> struct fex_gen_type<A> {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("A"));
            CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
        }
    }
}

TEST_CASE_METHOD(Fixture, "DataLayoutPointers") {
    auto guest_abi = GENERATE(GuestABI::X86_32, GuestABI::X86_64);
    INFO(guest_abi);

    const auto compat_full64_repackable32 = (guest_abi == GuestABI::X86_32 ? TypeCompatibility::Repackable : TypeCompatibility::Full);

    SECTION("Pointer to data with consistent layout") {
        std::string type = GENERATE("char", "short", "int", "float", "struct B { int a; }");
        INFO(type);
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { " + type + "* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        // The pointer itself needs repacking on 32-bit. On 64-bit, no repacking is needed at all.
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
        if (!type.starts_with("struct B")) {
            CHECK(action->GetTypeCompatibility(type) == TypeCompatibility::Full);
        }
    }

    SECTION("Pointer to struct with consistent layout") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct B { int32_t a; };\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("B"));
        CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Full);
        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Unannotated pointer to incomplete type") {
        CHECK_THROWS_WITH(compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct B;\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi),
            Catch::Contains("incomplete type"));
    }

    SECTION("Unannotated pointer to repackable type") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct B { int32_t a; int32_t b; };\n"
            "#else\n"
            "struct B { int32_t a; int64_t b; };\n"
            "#endif\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::None);
    }

    SECTION("Nesting repackable structs by pointers") {
        SECTION("Innermost struct is compatible") {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "struct C { int32_t a; int32_t b; };\n"
                "struct B { C* a; int16_t b; };\n"
                "struct A { int32_t a; B b; };\n"
                "template<> struct fex_gen_type<A> {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("A"));
            REQUIRE(action->guest_layout->contains("B"));
            REQUIRE(action->guest_layout->contains("C"));

            // 64-bit is fully compatible, but 32-bit needs to zero-extend the pointer itself
            CHECK(action->GetTypeCompatibility("struct C") == TypeCompatibility::Full);
            CHECK(action->GetTypeCompatibility("struct B") == compat_full64_repackable32);
            CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
        }

        SECTION("Innermost struct is incompatible") {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "#ifdef HOST\n"
                "struct C { int32_t a; int32_t b; };\n"
                "#else\n"
                "struct C { int32_t b; int32_t a; };\n"
                "#endif\n"
                "struct B { C* a; int16_t b; };\n"
                "struct A { int32_t a; B b; };\n"
                "template<> struct fex_gen_type<A> {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("A"));
            REQUIRE(action->guest_layout->contains("B"));
            REQUIRE(action->guest_layout->contains("C"));

            CHECK(action->GetTypeCompatibility("struct C") == TypeCompatibility::Repackable);
            CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::None);
            CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::None);
        }

        SECTION("Innermost struct is incompatible but the pointer member is annotated") {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "#ifdef HOST\n"
                "struct C { int32_t a; int32_t b; };\n"
                "#else\n"
                "struct C { int32_t b; int32_t a; };\n"
                "#endif\n"
                "struct B { C* a; int16_t b; };\n"
                "struct A { int32_t a; B b; };\n"
                "template<> struct fex_gen_config<&B::a> : fexgen::custom_repack {};\n"
                "template<> struct fex_gen_type<A> {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("A"));
            REQUIRE(action->guest_layout->contains("B"));
            REQUIRE(action->guest_layout->contains("C"));

            CHECK(action->GetTypeCompatibility("struct C") == TypeCompatibility::Repackable);
            CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Repackable);
            CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
        }
    }

    SECTION("Unannotated pointer to union type") {
        CHECK_THROWS_WITH(compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "union B { int32_t a; uint32_t b; };\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi),
            Catch::Contains("unannotated member") && Catch::Contains("union type"));
    }

    SECTION("Pointer to union type with assume_compatible_data_layout annotation") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "union B { int32_t a; uint32_t b; };\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<B> : fexgen::assume_compatible_data_layout {};\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Pointer to union type with custom_repack annotation") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "union B { int32_t a; uint32_t b; };\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_config<&A::a> : fexgen::custom_repack {};\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
    }

    SECTION("Pointer to opaque type") {
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct B;\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_type<B> : fexgen::opaque_type {};\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == compat_full64_repackable32);
    }

    SECTION("Pointer member with custom repacking code") {
        // Data layout analysis only needs to know about the custom_repack
        // annotation. The actual custom repacking code isn't needed for the
        // test.

        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "#ifdef HOST\n"
            "struct B { int32_t a; };\n"
            "#else\n"
            "struct B { int32_t b; };\n"
            "#endif\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_config<&A::a> : fexgen::custom_repack {};\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        REQUIRE(action->guest_layout->contains("B"));
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
        CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::None);
    }

    SECTION("Custom repacking induces repacking requirement") {
        // Data layout analysis only needs to know about the custom_repack
        // annotation. The actual custom repacking code isn't needed for the
        // test.

        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct B {};\n"
            "struct A { B* a; };\n"
            "template<> struct fex_gen_config<&A::a> : fexgen::custom_repack {};\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        REQUIRE(action->guest_layout->contains("B"));
        CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Full);
        CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Repackable);
    }

    SECTION("Self-referencing struct (like VkBaseOutStructure)") {
        // Without annotation
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { A* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK_THROWS_WITH(action->GetTypeCompatibility("struct A"), Catch::Contains("recursive reference"));

        // With annotation
        if (guest_abi == GuestABI::X86_64) {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "struct A { A* a; };\n"
                "template<> struct fex_gen_type<A> : fexgen::assume_compatible_data_layout {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("A"));
            CHECK(action->GetTypeCompatibility("struct A") == TypeCompatibility::Full);
        }
    }

    SECTION("Circularly referencing structs") {
        // Without annotation
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct B;\n"
            "struct A { B* a; };\n"
            "struct B { A* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        REQUIRE(action->guest_layout->contains("B"));
        CHECK_THROWS_WITH(action->GetTypeCompatibility("struct A"), Catch::Contains("recursive reference"));
        CHECK_THROWS_WITH(action->GetTypeCompatibility("struct B"), Catch::Contains("recursive reference"));

        // With annotation
        if (guest_abi == GuestABI::X86_64) {
            auto action = compute_data_layout(
                "#include <thunks_common.h>\n"
                "#include <cstdint>\n",
                "struct B;\n"
                "struct A { B* a; };\n"
                "struct B { A* a; };\n"
                "template<> struct fex_gen_type<B> : fexgen::assume_compatible_data_layout {};\n", guest_abi);

            INFO(FormatDataLayout(action->host_layout));

            REQUIRE(action->guest_layout->contains("B"));
            CHECK(action->GetTypeCompatibility("struct B") == TypeCompatibility::Full);
        }
    }

    SECTION("Pointers to void") {
        // Without annotation
        auto action = compute_data_layout(
            "#include <thunks_common.h>\n"
            "#include <cstdint>\n",
            "struct A { void* a; };\n"
            "template<> struct fex_gen_type<A> {};\n", guest_abi);

        INFO(FormatDataLayout(action->host_layout));

        REQUIRE(action->guest_layout->contains("A"));
        CHECK(action->GetTypeCompatibility("struct A") == (guest_abi == GuestABI::X86_32 ? TypeCompatibility::None : TypeCompatibility::Full));
    }

    // TODO: Double pointers to compatible data: struct B { int a ; }; struct A { B** b; };
}
