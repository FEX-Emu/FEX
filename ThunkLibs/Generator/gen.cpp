#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string_view>
#include <variant>

#include <openssl/sha.h>

#include "interface.h"

/**
 * Describes what action must be taken when passing data from guest to host or vice-versa.
 */
enum class ABICompatibility {
    Trivial,         // data can be exchanged without issues
    Repack,          // apply struct repacking
    RepackNestedPtr, // Pointer-to-pointer type where the innermost (i.e. pointee-pointee) type has trivial ABICompatibility
    GuestPtrAsInt,   // host-pointer is stored as a uintptr_t with size of the guest pointer. No repacking is needed
    PackedPtr,       // pointer value is the same for guest and host, converting via zero-extension or truncation where needed. Repacking is only needed if guest pointer size is different from the host
    Unknown,         // ABI compatibility can't be detected
};

static ABICompatibility CheckABICompatibility(const ABITable& abi, const StructInfo::ChildInfo& type);

/* type_name must be a non-pointer type that is present in ABITable */
static ABICompatibility CheckABICompatibility(const ABITable& abi, std::string_view type_name) {
    // TODO: Assert there's no star or array brackets in type_name

    std::cerr << "Checking ABI compatibility of type " << type_name << "\n";

    std::vector<const TypeInfo*> types_per_abi;
    for (int i = 0; i < abi.abis.size(); ++i) {
        auto type_it = abi.abis[i].find(std::string { type_name });
        if (type_it == abi.abis[i].end()) {
            std::cerr << "TYPE NOT PRESENT " << type_name << "\n";
            throw std::runtime_error("No ABI description for type \"" + std::string { type_name } + "\" required by interface definition");
        }

        types_per_abi.push_back(&type_it->second);

        if (types_per_abi[i]->is_opaque()) {
            // TODO: Requires zero-extension on x86-32
            // TODO: Should only return trivial ABI compatibility for pointers, since the data itself can't be handled
            return ABICompatibility::Trivial;
        }

        // For non-opaque types, make sure the general kind of type is consistent (struct, builtin, ...)
        if (types_per_abi[i]->index() != types_per_abi[0]->index()) {
            // TODO: Probably should assert here?
            assert(false && "Type has inconsistent kind across ABIs");
            return ABICompatibility::Unknown;
        }
    }

    if (auto* struct_info = std::get_if<StructInfo>(types_per_abi[0])) {
        // First, recursively check ABI compatibility of children:
        // The struct *can* be trivial if all of them are trivial, otherwise
        // the struct *can* be repackable if all of them are repackable.
        auto compat =
            std::accumulate(struct_info->children.begin(), struct_info->children.end(),
                            ABICompatibility::Trivial,
                            [&](ABICompatibility compat, const StructInfo::ChildInfo& next) {
                                if (compat != ABICompatibility::Trivial && compat != ABICompatibility::Repack) {
                                    return ABICompatibility::Unknown;
                                }

                                auto next_compat = CheckABICompatibility(abi, next);
                                switch (compat) {
                                case ABICompatibility::Trivial:
                                    return next_compat;

                                case ABICompatibility::Repack:
                                    if (next_compat == ABICompatibility::Trivial) {
                                        return ABICompatibility::Repack;
                                    } else {
                                        return next_compat;
                                    }

                                // TODO: Member with GuestPtrAsInt allows for Trivial compatibility

                                default:
                                    return ABICompatibility::Unknown;
                                }
                            });
        std::cerr << "  ABICompatibility of children is " << static_cast<int>(compat) << "\n";

        // Second, check if the member offsets are consistent
        //
        // The struct as a whole has ABICompatibility::Trivial only if all
        // members are trivially ABI compatible *and* the member offsets
        // within the struct are consistent across ABIs.
        //
        // If all members are ABI compatible trivially or through repacking,
        // but the member offsets differ between ABIs, then the struct has
        // ABICompatibility::Repack.
        //
        // Otherwise, the struct has ABICompatibility::Unknown.
        std::vector<StructInfo::ChildInfo> base_layout;
        for (int i = 0; i < types_per_abi.size(); ++i) {
            const auto& children = std::get<StructInfo>(*types_per_abi[i]).children;

            int non_padding_members = 0;

            for (auto& child : children) {
                if (child.is_padding_member) {
                    continue;
                }

                ++non_padding_members;

                if (i == 0) {
                    // Build reference layout
                    base_layout.push_back(child);
                } else {
                    // Ensure the layout on this ABI is compatible with the reference layout
                    auto other_child = std::find_if(base_layout.begin(), base_layout.end(),
                                                    [&child](const StructInfo::ChildInfo& other) {
                                                        return other.member_name == child.member_name;
                                                    });
                    if (other_child == base_layout.end()) {
                        // Member not present in the reference ABI
                        // TODO: Fail loudly?
                        std::cerr << "Member not present in the reference ABI\n";
                        compat = ABICompatibility::Unknown;
                        break;
                    }

                    if (child.offset_bits == other_child->offset_bits) {
                        // Trivial. Leave compatibility unchanged
                    } else {
                        // Child offsets break triviality, but we can restore ABI compatibility by reordering them
                        if (compat == ABICompatibility::Trivial) {
                            compat = ABICompatibility::Repack;
                        }
                    }
                }
            }

            if (non_padding_members != base_layout.size()) {
                // Some member of the reference ABI is not present here
                // TODO: Fail loudly?
                std::cerr << "Some member of the reference ABI is not present in all ABIs\n";
                compat = ABICompatibility::Unknown;
                break;
            }
        }

        std::cerr << "  ABICompatibility after layout analysis is " << static_cast<int>(compat) << "\n";
        return compat;
    } else if (auto* simple_info = std::get_if<SimpleTypeInfo>(types_per_abi[0])) {
        // TODO: Unless long double
        std::cerr << "  is of simple kind, returning ABICompatibility::Trivial\n";
        return ABICompatibility::Trivial;
    } else {
        // TODO: Handle opaque types. Probably shouldn't ever get here, but instead special-case the pointer path?
        std::cerr << "  is of unknown kind, returning ABICompatibility::Unknown\n";
        return ABICompatibility::Unknown;
    }
}

struct FunctionAnnotations {
    bool in = false;
    bool out = false;
    bool ptr_passthrough = false;
    bool untyped_address = false;

    bool HasAny() const noexcept {
        return in || out || ptr_passthrough || untyped_address;
    }
};

static ABICompatibility CheckABICompatibility(const ABITable& abi, const clang::Type* type, FunctionAnnotations annotations) {
    if (auto* typedef_type = llvm::dyn_cast_or_null<clang::TypedefType>(type)) {
        auto aliased_type = typedef_type->getDecl()->getUnderlyingType().getTypePtr();
        if (aliased_type->isRecordType()) {
            // TODO: Needed by xcb_randr_mode_end to reconstruct the "struct" for xcb_randr_mode_iterator_t. Is this the right approach, and should we just strip off the typedef elsewhere?
            type = aliased_type;
        }

        // Handle simple, built-in types.
        // TODO: These should be part of the ABI instead!
        if (aliased_type->isBuiltinType()) {
            return ABICompatibility::Trivial;
        }
    }

    if (!type->isPointerType()) {
        return CheckABICompatibility(abi, clang::QualType(type, 0).getAsString());
    } else if (type->isFunctionPointerType()) {
        // TODO
        return ABICompatibility::Trivial;
    } else {
        auto* const pointee_type = type->getPointeeType()->getUnqualifiedDesugaredType();

        if (annotations.ptr_passthrough) {
            return ABICompatibility::GuestPtrAsInt;
        } else if (annotations.untyped_address) {
            return ABICompatibility::PackedPtr;
        } else if (pointee_type->isPointerType()) {
            // Pointer-to-pointer: Either RepackNestedPtr or Unknown
            // Notably, this applies for create()-like functions that initialize an opaque type via an output parameter
            type = pointee_type->getPointeeType()->getUnqualifiedDesugaredType();
            auto inner_compat = CheckABICompatibility(abi, clang::QualType { type, 0 }.getAsString());
            return (inner_compat == ABICompatibility::Trivial) ? ABICompatibility::RepackNestedPtr : ABICompatibility::Unknown;
        } else if (type->isVoidPointerType()) {
            throw std::runtime_error("Unannotated void pointer type '" + clang::QualType(type, 0).getAsString() + "'");
        } else {
            type = pointee_type;
            std::cerr << "Checking ABI of " << clang::QualType { type, 0 }.getAsString() << "\n";
            auto pointee_compat = CheckABICompatibility(abi, clang::QualType { type, 0 }.getAsString());
            if (pointee_compat == ABICompatibility::Repack) {
                if (!annotations.in && !annotations.out) {
                    throw std::runtime_error("Struct requires repacking, but no input/output annotations were given"); // TODO: Proper error
                }
                return pointee_compat;
            }

            return pointee_compat == ABICompatibility::Trivial ? ABICompatibility::Trivial : ABICompatibility::Unknown;
        }
    }
}

static ABICompatibility CheckABICompatibility(const ABITable& abi, const StructInfo::ChildInfo& type) {
    static int callstack_depth = 0;
    struct AvoidInfiniteRecursion {
        AvoidInfiniteRecursion() { ++callstack_depth; }
        ~AvoidInfiniteRecursion() { --callstack_depth; }
    } avoid_infinite_recursion;
    if (callstack_depth > 100) {
        // TODO: Properly detect linked lists, e.g. XExtData or snd_devname
        // TODO: Add a test to ensure we handle this gracefully
        return ABICompatibility::Unknown;
    }

    if (type.pointer_chain.empty() || (type.pointer_chain.size() == 1/* && type.pointer_chain[0].array_size*/)) {
        return CheckABICompatibility(abi, type.type_name);
    } else {
        // TODO: Check member annotations (e.g. passthrough pointer)
        return ABICompatibility::Unknown;
    }
}

#if 0
static bool TypeHasConsistentABI(const ABITable& abi, const clang::Type* type) {
    if (!type->isPointerType()) {
        // TODO: Handle non-pointer types...
        return true;
    }
    type = type->getPointeeType()->getUnqualifiedDesugaredType();

    std::array<const TypeInfo*, ABITable::num_abis> types;
    for (int i = 0; i < types.size(); ++i) {
        auto type_it = abi.abis[i].find(clang::QualType(type, 0).getAsString());
        if (type_it == abi.abis[i].end()) {
            return false;
        }

        types[i] = &type_it->second;

        // Make sure the general kind of type is consistent (struct, builtin, ...)
        if (types[i]->index() != types[0]->index()) {
            return false;
        }
    }

    if (auto* struct_info = std::get_if<StructInfo>(types[0])) {
        return std::all_of(types.begin(), types.end(),
                           [&](const TypeInfo* a) {
                                // TODO: Recursively test ABI compatibility
                               return std::get<StructInfo>(*a) == *struct_info;
                           });
    } else {
        // TODO
        return true;
    }
}
#endif

struct FunctionParams {
    std::vector<clang::QualType> param_types;
};

struct ThunkedCallback : FunctionParams {
    clang::QualType return_type;

    std::size_t callback_index;
    std::size_t user_arg_index;

    bool is_stub = false;  // Callback will be replaced by a stub that calls std::abort
    bool is_guest = false; // Callback will never be called on the host
    bool is_variadic = false;
};

enum class VariadicStrategy {
    Unneeded,
    UniformList, // Convert va_list into a pair of element count and pointer to array of arguments with type specified by an annotation

    // Convert va_list into a triple of element count, pointer to array of element types (e.g. PA_INT), and pointer to array of value slots.
    // Each formatted argument is written to a 128-bit value slot, which is enough to fit the largest type printable by printf (long double)
    LikePrintf,
};

/**
 * Guest<->Host transition point.
 *
 * These are normally used to translate the public API of the guest to host
 * function calls (ThunkedAPIFunction), but a thunk library may also define
 * internal thunks that don't correspond to any function in the implemented
 * API.
 */
struct ThunkedFunction : FunctionParams {
    std::string function_name;
    clang::QualType return_type;

    // If true, param_types contains an extra size_t and the valist for marshalling through an internal function
    VariadicStrategy variadic_strategy = VariadicStrategy::Unneeded;

    // For s(n)printf-like functions, this field stores the index of the output buffer parameter
    std::optional<int> sprintf_buffer;

    // For snprintf-like functions, this field stores the index of the output buffer size parameter
    std::optional<int> snprintf_buffer_size;

    // If true, the unpacking function will call a custom fexfn_impl function
    // to be provided manually instead of calling the host library function
    // directly.
    // This is implied e.g. for thunks generated for variadic functions
    bool custom_host_impl = false;

    std::string GetOriginalFunctionName() const {
        const std::string suffix = "_internal";
        assert(function_name.length() > suffix.size());
        assert((std::string_view { &*function_name.end() - suffix.size(), suffix.size() } == suffix));
        return function_name.substr(0, function_name.size() - suffix.size());
    }

    // Maps parameter index to ThunkedCallback
    std::unordered_map<unsigned, ThunkedCallback> callbacks;

    clang::FunctionDecl* decl;

    // Maps parameter index to FunctionAnnotations
    // TODO: Map -1 to return value annotations?
    // TODO: Is this a good place to put this?
    std::unordered_map<int, FunctionAnnotations> parameter_annotations;
};

/**
 * Function that is part of the API of the thunked library.
 *
 * For each of these, there is:
 * - A publicly visible guest entrypoint (usually auto-generated but may be manually defined)
 * - A pointer to the native host library function loaded through dlsym (or a user-provided function specified via host_loader)
 * - A ThunkedFunction with the same function_name (possibly suffixed with _internal)
 */
struct ThunkedAPIFunction : FunctionParams {
    std::string function_name;

    clang::QualType return_type;

    // name of the function to load the native host symbol with
    std::string host_loader;

    // If true, no guest-side implementation of this function will be autogenerated
    bool custom_guest_impl;

    bool is_variadic;

    // Index of the symbol table to store this export in (see guest_symtables).
    // If empty, a library export is created, otherwise the function is entered into a function pointer array
    std::optional<std::size_t> symtable_namespace;
};

static std::vector<ThunkedFunction> thunks;
static std::vector<ThunkedAPIFunction> thunked_api;
static std::optional<unsigned> lib_version;

static std::vector<std::string> data_symbols;

struct NamespaceInfo {
    std::string name;

    // Function to load native host library functions with.
    // This function must be defined manually with the signature "void* func(void*, const char*)"
    std::string host_loader;

    bool generate_guest_symtable;
};

// List of namespaces with a non-specialized fex_gen_config definition (including the global namespace, represented with an empty name)
static std::vector<NamespaceInfo> namespaces;

struct ClangDiagnosticAsException : std::pair<clang::SourceLocation, unsigned> {
    // List of callbacks that add an argument to a clang::DiagnosticBuilder
    std::vector<std::function<void(clang::DiagnosticBuilder&)>> args;

    ClangDiagnosticAsException& AddString(std::string str) {
        args.push_back([arg=std::move(str)](clang::DiagnosticBuilder& db) {
            db.AddString(arg);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddSourceRange(clang::CharSourceRange range) {
        args.push_back([range](clang::DiagnosticBuilder& db) {
            db.AddSourceRange(range);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddTaggedVal(clang::QualType type) {
        args.push_back([val=reinterpret_cast<uint64_t&>(type)](clang::DiagnosticBuilder& db) {
            db.AddTaggedVal(val, clang::DiagnosticsEngine::ak_qualtype);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddTaggedVal(clang::NamedDecl* decl) {
        args.push_back([val=reinterpret_cast<uint64_t&>(decl)](clang::DiagnosticBuilder& db) {
            db.AddTaggedVal(val, clang::DiagnosticsEngine::ak_nameddecl);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddTaggedValUnsigned(unsigned number) {
        args.push_back([val=number](clang::DiagnosticBuilder& db) {
            db.AddTaggedVal(val, clang::DiagnosticsEngine::ak_uint);
        });
        return *this;
    }

    ClangDiagnosticAsException& AddTaggedValSigned(int number) {
        args.push_back([val=number](clang::DiagnosticBuilder& db) {
            db.AddTaggedVal(val, clang::DiagnosticsEngine::ak_sint);
        });
        return *this;
    }

    void Report(clang::DiagnosticsEngine& diagnostics) const {
        auto builder = diagnostics.Report(first, second);
        for (auto& arg_appender : args) {
            arg_appender(builder);
        }
    }
};

class ASTVisitor : public clang::RecursiveASTVisitor<ASTVisitor> {
    clang::ASTContext& context;

    enum class CallbackStrategy {
        Default,
        Stub,
        Guest,
    };

    struct NamespaceAnnotations {
        std::optional<unsigned> version;
        std::optional<std::string> load_host_endpoint_via;
        bool generate_guest_symtable = false;
    };

    struct Annotations {
        bool custom_host_impl = false;
        bool custom_guest_entrypoint = false;

        bool returns_guest_pointer = false;

        bool like_printf = false;
        std::optional<int> sprintf_buffer = std::nullopt;
        std::optional<int> snprintf_buffer_size = std::nullopt;

        std::optional<clang::QualType> uniform_va_type;

        CallbackStrategy callback_strategy = CallbackStrategy::Default;
    };

    NamespaceAnnotations GetNamespaceAnnotations(clang::CXXRecordDecl* decl) {
        if (!decl->hasDefinition()) {
            return {};
        }

        NamespaceAnnotations ret;

        for (const clang::CXXBaseSpecifier& base : decl->bases()) {
            auto annotation = base.getType().getAsString();
            if (annotation == "fexgen::generate_guest_symtable") {
                ret.generate_guest_symtable = true;
            } else {
                throw Error(base.getSourceRange().getBegin(), "Unknown namespace annotation");
            }
        }

        for (const clang::FieldDecl* field : decl->fields()) {
            auto name = field->getNameAsString();
            if (name == "load_host_endpoint_via") {
                auto loader_function_expr = field->getInClassInitializer()->IgnoreCasts();
                auto loader_function_str = llvm::dyn_cast_or_null<clang::StringLiteral>(loader_function_expr);
                if (loader_function_expr && !loader_function_str) {
                    throw Error(loader_function_expr->getBeginLoc(),
                                "Must initialize load_host_endpoint_via with a string");
                }
                if (loader_function_str) {
                    ret.load_host_endpoint_via = loader_function_str->getString();
                }
            } else if (name == "version") {
                auto initializer = field->getInClassInitializer()->IgnoreCasts();
                auto version_literal = llvm::dyn_cast_or_null<clang::IntegerLiteral>(initializer);
                if (!initializer || !version_literal) {
                    throw Error(field->getBeginLoc(), "No version given (expected integral typed member, e.g. \"int version = 5;\")");
                }
                ret.version = version_literal->getValue().getZExtValue();
            } else {
                throw Error(field->getBeginLoc(), "Unknown namespace annotation");
            }
        }

        return ret;
    }

    Annotations GetAnnotations(clang::CXXRecordDecl* decl) {
        Annotations ret;

        for (const auto& base : decl->bases()) {
            auto annotation = base.getType().getAsString();

            auto read_int_arg = [&base](int arg_idx) -> int64_t {
                auto template_arg_expr = base.getType()->getAs<clang::TemplateSpecializationType>()->getArg(arg_idx).getAsExpr();
                return llvm::dyn_cast<clang::ConstantExpr>(template_arg_expr)->getAPValueResult().getInt().getExtValue();
            };

            if (annotation == "fexgen::returns_guest_pointer") {
                ret.returns_guest_pointer = true;
            } else if (annotation == "fexgen::custom_host_impl") {
                ret.custom_host_impl = true;
            } else if (annotation == "fexgen::callback_stub") {
                ret.callback_strategy = CallbackStrategy::Stub;
            } else if (annotation == "fexgen::callback_guest") {
                ret.callback_strategy = CallbackStrategy::Guest;
            } else if (annotation == "fexgen::custom_guest_entrypoint") {
                ret.custom_guest_entrypoint = true;
            } else if (annotation == "fexgen::like_printf") {
                ret.like_printf = true;
            } else if (annotation.starts_with("fexgen::like_sprintf<")) {
                ret.like_printf = true;
                ret.sprintf_buffer = read_int_arg(0);
            } else if (annotation.starts_with("fexgen::like_snprintf<")) {
                ret.like_printf = true;
                ret.sprintf_buffer = read_int_arg(0);
                ret.snprintf_buffer_size = read_int_arg(1);
            } else {
                throw Error(base.getSourceRange().getBegin(), "Unknown annotation");
            }
        }

        for (const auto& child_decl : decl->getPrimaryContext()->decls()) {
            if (auto field = llvm::dyn_cast_or_null<clang::FieldDecl>(child_decl)) {
                throw Error(field->getBeginLoc(), "Unknown field annotation");
            } else if (auto type_alias = llvm::dyn_cast_or_null<clang::TypedefNameDecl>(child_decl)) {
                auto name = type_alias->getNameAsString();
                if (name == "uniform_va_type") {
                    ret.uniform_va_type = type_alias->getUnderlyingType();
                } else {
                    throw Error(type_alias->getBeginLoc(), "Unknown type alias annotation");
                }
            }
        }

        return ret;
    }

    template<std::size_t N>
    [[nodiscard]] ClangDiagnosticAsException Error(clang::SourceLocation loc, const char (&message)[N]) {
        auto id = context.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error, message);
        return { std::pair(loc, id) };
    }

public:
    ASTVisitor(clang::ASTContext& context_) : context(context_) {
    }

    /**
     * Matches "template<auto> struct fex_gen_config { ... }"
     */
    bool VisitClassTemplateDecl(clang::ClassTemplateDecl* decl) try {
        if (decl->getName() != "fex_gen_config") {
            return true;
        }

        auto annotations = GetNamespaceAnnotations(decl->getTemplatedDecl());

        auto namespace_decl = llvm::dyn_cast<clang::NamespaceDecl>(decl->getDeclContext());
        namespaces.push_back({  namespace_decl ? namespace_decl->getNameAsString() : "",
                                annotations.load_host_endpoint_via.value_or(""),
                                annotations.generate_guest_symtable });

        if (annotations.version) {
            if (namespace_decl) {
                throw Error(decl->getBeginLoc(), "Library version must be defined in the global namespace");
            }
            lib_version = annotations.version;
        }

        return true;
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
        return false;
    }

    /**
     * Matches "template<> struct fex_gen_param<LibraryFunc, ParamIdx[, ParamType]> { ... }"
     */
    bool VisitClassTemplateSpecializationDecl(clang::ClassTemplateSpecializationDecl* decl) try {
        if (decl->getName() == "fex_gen_param") {
            // Function parameter annotation
            const auto& template_args = decl->getTemplateArgs();
            auto function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl());
            auto param_idx = template_args[1].getAsIntegral().getExtValue();
            auto param_type = template_args[2].getAsType();

            if (param_idx >= function->getNumParams()) {
                throw Error(decl->getBeginLoc(), "Parameter index exceeds total parameter count %0 function %1")
                      .AddTaggedValUnsigned(function->getNumParams()).AddTaggedVal(function);
            }

            // Check that the explicitly provided parameter type matches the one from the function signature
            if (param_type->isVoidType() && !param_type->isPointerType()) {
                // Type wasn't specified, auto-detect it and skip consistency check
                param_type = function->getParamDecl(param_idx)->getType();
            } else if (!context.hasSameType(param_type, function->getParamDecl(param_idx)->getType())) {
                throw Error(decl->getBeginLoc(), "Given parameter type %0 doesn't match %1 expected from function signature")
                    .AddTaggedVal(param_type)
                    .AddTaggedVal(function->getParamDecl(param_idx)->getType());
            }

            FunctionAnnotations annotations;
            for (const clang::CXXBaseSpecifier& base : decl->bases()) {
                auto annotation = base.getType().getAsString();
                if (annotation == "fexgen::ptr_in") {
                    annotations.in = true;
                } else if (annotation == "fexgen::ptr_out") {
                    annotations.out = true;
                } else if (annotation == "fexgen::ptr_inout") {
                    annotations.in = true;
                    annotations.out = true;
                } else if (annotation == "fexgen::ptr_pointer_passthrough") {
                    annotations.ptr_passthrough = true;
                } else if (annotation == "fexgen::ptr_is_untyped_address") {
                    annotations.untyped_address = true;
                } else if (annotation == "fexgen::ptr_todo_only64") {
                    // Treat as passthrough pointer
                    annotations.ptr_passthrough = true;
                } else {
                    throw Error(base.getSourceRange().getBegin(), "Unknown function parameter annotation");
                }
            }
            auto thunk_it = std::find_if(thunks.begin(), thunks.end(), [function](auto& thunk) { return thunk.decl == function; });
            if (thunk_it == thunks.end()) {
                throw Error(decl->getBeginLoc(), "Before annotating function parameters, fex_gen_config must be specialized for the function itself");
            }
            thunk_it->parameter_annotations[param_idx] = annotations;
            return true;
        }


        if (decl->getName() != "fex_gen_config") {
            return true;
        }

        if (decl->getSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition) {
            throw Error(decl->getBeginLoc(), "fex_gen_config may not be partially specialized\n");
        }

        std::string namespace_name;
        if (auto namespace_decl = llvm::dyn_cast<clang::NamespaceDecl>(decl->getDeclContext())) {
            namespace_name = namespace_decl->getNameAsString();
        }
        const auto namespace_idx = std::distance(   namespaces.begin(),
                                                    std::find_if(   namespaces.begin(), namespaces.end(),
                                                                    [&](auto& info) { return info.name == namespace_name; }));
        const NamespaceInfo& namespace_info = namespaces[namespace_idx];

        const auto& template_args = decl->getTemplateArgs();
        assert(template_args.size() == 1);

        if (auto emitted_function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl())) {
          auto return_type = emitted_function->getReturnType();

          const auto annotations = GetAnnotations(decl); // TODO: Rename to GetFunctionAnnotations
          if (return_type->isFunctionPointerType() && !annotations.returns_guest_pointer) {
              // TODO: Should regular pointers require annotation, too?
              throw Error(decl->getBeginLoc(),
                          "Function pointer return types require explicit annotation\n");
          }

          // TODO: Use the types as written in the signature instead?
          ThunkedFunction data;
          data.function_name = emitted_function->getName().str();
          data.return_type = return_type;
          data.decl = emitted_function;

          data.custom_host_impl = annotations.custom_host_impl;

          const bool has_explicit_va_list = std::any_of(emitted_function->param_begin(), emitted_function->param_end(),
                       [](clang::ParmVarDecl* param) { return param->getType().getAsString() == "__gnuc_va_list"; });

          if (emitted_function->isVariadic()) {
              if (annotations.uniform_va_type) {
                  assert(!has_explicit_va_list); // Not implemented yet
                  data.variadic_strategy = VariadicStrategy::UniformList;
              } else if (annotations.like_printf) {
                  data.variadic_strategy = VariadicStrategy::LikePrintf;
                  data.sprintf_buffer = annotations.sprintf_buffer;
                  data.snprintf_buffer_size = annotations.snprintf_buffer_size;
              } else {
                  throw Error(decl->getBeginLoc(), "Variadic functions must be annotated with parameter type using uniform_va_type");
              }
          } else if (has_explicit_va_list) {
              if (annotations.like_printf) {
                  // Assume this is a vprintf-like function that takes an explicit va_list parameter
                  auto num_params = emitted_function->getNumParams();
                  if (num_params == 0 ||
                      emitted_function->getParamDecl(num_params - 1)->getType().getAsString() != "__gnuc_va_list") {
                      throw Error(decl->getBeginLoc(), "printf annotations may only be used if the function is variadic or has a va_list parameter at the end");
                  }

                  data.variadic_strategy = VariadicStrategy::LikePrintf;
                  data.sprintf_buffer = annotations.sprintf_buffer;
                  data.snprintf_buffer_size = annotations.snprintf_buffer_size;
              } else {
                  throw Error(decl->getBeginLoc(), "Functions with explicit va_list parameter must be annotated (consider like_printf)");
              }
          } else if (annotations.uniform_va_type || annotations.like_printf) {
              throw Error(decl->getBeginLoc(), "Invalid annotation for non-variadic function");
          }

          for (std::size_t param_idx = 0; param_idx < emitted_function->param_size(); ++param_idx) {
              auto* param = emitted_function->getParamDecl(param_idx);
              data.param_types.push_back(param->getType());

              if (param->getType()->isFunctionPointerType()) {
                  auto funcptr = param->getFunctionType()->getAs<clang::FunctionProtoType>();
                  ThunkedCallback callback;
                  callback.return_type = funcptr->getReturnType();
                  for (auto& cb_param : funcptr->getParamTypes()) {
                      callback.param_types.push_back(cb_param);
                  }
                  callback.is_stub = annotations.callback_strategy == CallbackStrategy::Stub;
                  callback.is_guest = annotations.callback_strategy == CallbackStrategy::Guest;
                  callback.is_variadic = funcptr->isVariadic();

                  if (callback.is_guest && !data.custom_host_impl) {
                      throw Error(decl->getBeginLoc(), "callback_guest can only be used with custom_host_impl");
                  }

                  if (!annotations.custom_guest_entrypoint || !annotations.custom_host_impl) {
                      data.callbacks.emplace(param_idx, callback);
                      if (data.callbacks.size() != 1) {
                          throw Error(decl->getBeginLoc(), "Support for more than one callback is not implemented");
                      }
                  }
                  if (funcptr->isVariadic() && !callback.is_stub) {
                      throw Error(decl->getBeginLoc(), "Variadic callbacks are not supported");
                  }
              }
          }

          // TODO: Rename to something like "needs_modified_callback"
          const bool has_nonstub_callbacks = std::any_of(data.callbacks.begin(), data.callbacks.end(),
                                                         [](auto& cb) { return !cb.second.is_stub && !cb.second.is_guest; });
          thunked_api.push_back(ThunkedAPIFunction { (const FunctionParams&)data, data.function_name, data.return_type,
                                                      namespace_info.host_loader.empty() ? "dlsym" : namespace_info.host_loader,
                                                      has_nonstub_callbacks || data.variadic_strategy == VariadicStrategy::UniformList || annotations.custom_guest_entrypoint,
                                                      emitted_function->isVariadic(),
                                                      std::nullopt });
          if (namespace_info.generate_guest_symtable) {
              thunked_api.back().symtable_namespace = namespace_idx;
          }

          if (data.variadic_strategy == VariadicStrategy::UniformList) {
              // Convert variadic argument list into a count + pointer pair
              data.param_types.push_back(context.getSizeType());
              data.param_types.push_back(context.getPointerType(*annotations.uniform_va_type));
          } else if (data.variadic_strategy == VariadicStrategy::LikePrintf) {
              // Pop explicit va_list argument (if any)
              if (!data.decl->isVariadic()) {
                  data.param_types.pop_back();
              }

              // Convert variadic argument list into a count + type[] + value[] triple
              data.param_types.push_back(context.getSizeType());
              data.param_types.push_back(context.getPointerType(context.IntTy));
              data.param_types.push_back(context.VoidPtrTy);
          }

          if (has_nonstub_callbacks || data.variadic_strategy != VariadicStrategy::Unneeded) {
              // This function is thunked through an "_internal" symbol since its signature
              // is different from the one in the native host/guest libraries.
              data.function_name = data.function_name + "_internal";
              assert(!data.custom_host_impl && "Custom host impl requested but this is implied by the function signature already");
              data.custom_host_impl = true;
          }

          thunks.push_back(std::move(data));
        } else {
          const auto data_symbol = template_args[0].getAsDecl();
          // FieldDecls are used to annotated struct members
          if (!llvm::isa<clang::FieldDecl>(data_symbol)) {
              data_symbols.push_back(data_symbol->getName().str());
          }

//          ThunkedFunction data;
//          data.function_name = "fex_get_datasymbol_" + data_symbol->getName().str();
//          data.return_type = context.VoidPtrTy;

//          data.decl = nullptr;

//          thunks.push_back(std::move(data));
        }

        return true;
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
        return false;
    }
};

class ASTConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        ASTVisitor{context}.TraverseDecl(context.getTranslationUnitDecl());
    }
};

GenerateThunkLibsAction::GenerateThunkLibsAction(const std::string& libname_, const OutputFilenames& output_filenames_, const ABITable& abi_)
    : libfilename(libname_), libname(libname_), output_filenames(output_filenames_), abi(abi_) {
    for (auto& c : libname) {
        if (c == '-') {
            c = '_';
        }
    }

    thunks.clear();
    thunked_api.clear();
    namespaces.clear();
    lib_version = std::nullopt;
}

template<typename Fn>
static std::string format_function_args(const FunctionParams& params, Fn&& format_arg) {
    std::string ret;
    for (std::size_t idx = 0; idx < params.param_types.size(); ++idx) {
        ret += std::forward<Fn>(format_arg)(idx) + ", ";
    }
    // drop trailing ", "
    ret.resize(ret.size() > 2 ? ret.size() - 2 : 0);
    return ret;
};

static std::string format_function_args(const FunctionParams& params) {
    return format_function_args(params,
                                [](std::size_t idx) -> std::string { return "args->a_" + std::to_string(idx); });
}

static StructInfo::ChildInfo FromClangType(clang::ASTContext& context, const clang::Type* field_type, std::string_view field_name, std::optional<uint64_t> field_offset, std::optional<unsigned> bitfield_width) {
    const auto field_size = context.getTypeSize(field_type);

    std::vector<PointerInfo> pointer_chain;
    while (true) {
        if (auto pointer_type = field_type->getAs<clang::PointerType>()) {
            field_type = pointer_type->getPointeeType()->getUnqualifiedDesugaredType();
            pointer_chain.push_back(PointerInfo {});
        } else if (auto array_type = llvm::dyn_cast<clang::ConstantArrayType>(field_type)) {
            field_type = array_type->getElementType()->getUnqualifiedDesugaredType();
            pointer_chain.push_back(PointerInfo { array_type->getSize().getZExtValue() });
        } else {
            break;
        }
    }

    return {
        field_size,
        field_offset.value_or(std::numeric_limits<uint64_t>::max()),
        clang::QualType(field_type, 0).getAsString(),
        std::string { field_name },
        std::move(pointer_chain),
        bitfield_width,
        false /* is_padding_member (set by caller if applicable) */
    };
}

struct MemberAnnotations {
    std::optional<PointerInfo::Type> pointer_type;
    bool is_padding_member;
};

static TypeInfo GetTypeInfoFromClang( clang::ASTContext& context, ABIPlatform platform, const clang::Type* type,
                                      std::unordered_map<const clang::FieldDecl*, MemberAnnotations>& member_annotations) {
    if (type->isIncompleteType()) {
        throw std::runtime_error("Type is incomplete. Did you forget to include its defining library header? If not, consider annotating the type as opaque.");
    }

    TypeInfo info;
    auto size_bits = context.getTypeSize(type);

    if (auto* record_type = type->getAs<clang::RecordType>()) {
        StructInfo struct_info { size_bits };
        if (record_type->isUnionType()) {
            struct_info.is_union = true;
        } else {
            // This is a proper struct
            for (auto* field : record_type->getAsCXXRecordDecl()->fields()) {
                auto field_type = field->getType().getCanonicalType()->getUnqualifiedDesugaredType();
                auto child_info = FromClangType(context, field_type, field->getNameAsString(), context.getFieldOffset(field), field->isBitField() ? std::optional { field->getBitWidthValue(context) } : std::nullopt);
                auto member_annotations_it = member_annotations.find(field);
                if (member_annotations_it != member_annotations.end()) {
                    const auto& annotations = member_annotations_it->second;
                    if (annotations.pointer_type) {
                        if (child_info.pointer_chain.empty()) {
                            // TODO: Error properly
                            throw std::runtime_error("Specified pointer annotations to non-pointer member");
                        }
                        child_info.pointer_chain[0].type = *annotations.pointer_type;

                        if (annotations.pointer_type == PointerInfo::Type::Passthrough ||
                            annotations.pointer_type == PointerInfo::Type::TODOONLY64) {
                            if (platform == ABIPlatform::Host) {
                                child_info.pointer_chain.clear();
                                // TODO: Pick uint64_t for 32-bit guests
                                child_info.type_name = "unsigned long";
                            } else {
                                // TODO: Handle function pointers properly
                                if (field_type->isFunctionPointerType()) {
                                    child_info.pointer_chain.clear();
                                    // TODO: Use uint64_t for 32-bit guests
                                    child_info.type_name = "unsigned long";
                                }
                            }
                        }
                    }

                    child_info.is_padding_member = annotations.is_padding_member;
                }
                struct_info.children.push_back(child_info);
            }
        }

        info = struct_info;
    } else {
        // TODO: Size?
        // TODO: Pointer chain
        info = SimpleTypeInfo {};
    }

    return info;
}

enum class RepackTo {
    Host,
    Guest
};

static void emit_repacking_code_by_passthrough(std::ostream& file, std::string source_parent, const StructInfo::ChildInfo& source_child, std::string target_parent, const StructInfo::ChildInfo& target_child, RepackTo target_abi) {
    // Host child is just a uint value: reinterpret_cast pointer accordingly
    if (target_abi == RepackTo::Host) {
        // Generates: uint_val = reinterpret_cast<uint>(pointer_val)
        assert(target_child.pointer_chain.empty());
    } else {
        // Generates: pointer_val = reinterpret_cast<pointer>(uint)
        assert(source_child.pointer_chain.empty());
    }
    auto target_type = target_child.type_name + std::string(target_child.pointer_chain.size(), '*');
    file << "  " << target_parent << target_child.member_name << " = reinterpret_cast<" << target_type << ">(" << source_parent << source_child.member_name << ");\n";
}

static void emit_repacking_code_by_zero_ext(std::ostream& file, std::string source_parent, const StructInfo::ChildInfo& source_child, std::string target_parent, const StructInfo::ChildInfo& target_child) {
    assert(source_child.pointer_chain == target_child.pointer_chain);
    // TODO: Using memcpy to copy const-pointers into non-const pointers... which is not ideal
    // TODO: For 32-bit guests, the input pointer needs to be zero-extended/truncated
    file << "  memcpy(&" << target_parent << target_child.member_name << ", &" << source_parent << source_child.member_name << ", sizeof(" << source_parent << source_child.member_name << "));\n";
}

static void emit_repacking_code_child(std::ostream& file, const TypeInfo& child_type_info, std::string source_parent, const StructInfo::ChildInfo& source_child, std::string target_parent, const StructInfo::ChildInfo& target_child, RepackTo target_abi, bool is_fex_wrapped, bool is_trivially_compatible) {
    // TODO: If ABI compatible, just do a straight assignment

    const bool is_struct = std::holds_alternative<StructInfo>(child_type_info);

    source_parent = source_parent.empty() ? "" : (source_parent + '.');
    target_parent = target_parent.empty() ? "" : (target_parent + '.');

    // TODO: Clean up these conditionals...
    if ((target_child.pointer_chain.size() == 1 && !target_child.pointer_chain.back().array_size) ||
        (source_child.pointer_chain.size() == 1 && !source_child.pointer_chain.back().array_size)) {
        // TODO: Also handle PointerInfo::Type::Untyped
        // TODO: Passthrough should convert to guest-sized uintptr_t
        if ((!target_child.pointer_chain.empty() && (target_child.pointer_chain.back().type == PointerInfo::Type::Passthrough ||
            target_child.pointer_chain.back().type == PointerInfo::Type::TODOONLY64)) ||
            (!source_child.pointer_chain.empty() && (source_child.pointer_chain.back().type == PointerInfo::Type::Passthrough ||
            source_child.pointer_chain.back().type == PointerInfo::Type::TODOONLY64))) {
            emit_repacking_code_by_passthrough(file, std::move(source_parent), source_child, std::move(target_parent), target_child, target_abi);
            return;
        } else if (target_child.pointer_chain.back().type == PointerInfo::Type::Untyped || is_trivially_compatible) {
            // Opaque types can simply be assigned at pointer-level
            emit_repacking_code_by_zero_ext(file, std::move(source_parent), source_child, std::move(target_parent), target_child);
            return;
        } else if (false /* TODO: Deprecated: nested structs are still recursively repacked, but only for non-pointers */) {
            // TODO: This is not needed for trivial ABI compatibility...
            assert(source_child.pointer_chain == target_child.pointer_chain);

            // Peel off one pointer layer and recurse
            auto new_source_child = source_child;
            new_source_child.member_name = '*' + source_parent + new_source_child.member_name;
            new_source_child.pointer_chain.pop_back();

            auto new_target_child = target_child;
            new_target_child.member_name = '*' + target_parent + new_target_child.member_name;
            new_target_child.pointer_chain.pop_back();

            if (target_abi == RepackTo::Host) {
                // Repoint the pointer to a stack-allocated struct instance with host layout
                // TODO: Only do this if the types require repacking!
                // TODO: This can't be done in repacking functions, but only at the top-level of packing functions... otherwise, the temp_ variable goes out of scope!
                new_target_child.member_name = "temp_" + target_child.member_name;
                file << "  " << (is_struct ? "fex_host_type<" : "") << target_child.type_name << (is_struct ? ">" : "") << " " << new_target_child.member_name << ";\n";
                file << "  " << target_parent << target_child.member_name << " = &" << new_target_child.member_name << ";\n";
            }

            return emit_repacking_code_child(file, child_type_info, "", new_source_child, "", new_target_child, target_abi, is_fex_wrapped, is_trivially_compatible);
        } else {
            throw std::runtime_error("Cannot repack pointer member. Are you missing any annotations?");
        }
    } else if (target_child.pointer_chain.size() > 1) {
        //     TODO: Disabled since unannotated nested pointers are not supported
        throw std::runtime_error("Cannot repack unannotated nested pointer");

        if (target_parent == "args.") {
            // TODO: Remove this hacky workaround... Needed e.g. for getopt_long, which has a const char* argument and a const struct option* argument
//            file << "  args." << target_child.member_name << " = " << source_parent << source_child.member_name << ";\n";
            file << "  memcpy(&args." << target_child.member_name << ", (const void*)&" << source_parent << source_child.member_name << ", sizeof(" << source_parent << source_child.member_name << "));\n";
        } else {
            file << "  // TODO: Skipping " << source_child.member_name << " with too many pointer/array decorators\n";
        }
        return;
    }
    std::string pointer_prefix;
    for (auto& pointer : source_child.pointer_chain) {
        if (!pointer.array_size) {
            pointer_prefix += '*';
        }
    }
    // TODO: Support multi-dimensional arrays
    const auto array_size = target_child.pointer_chain.size() == 1 ? target_child.pointer_chain.back().array_size : std::nullopt;
    const auto array_subscript = array_size ? "[elem_idx]" : "";
    if (array_size) {
        file << "  for (int elem_idx = 0; elem_idx < " << *array_size << "; ++elem_idx)\n  ";
    }
    if (is_struct) {
        // TODO: Actually, avoid dereferencing and make fex_repack_to_host accept pointer arguments
        file << "  " << pointer_prefix << target_parent << target_child.member_name << array_subscript << " = " << (target_abi == RepackTo::Host ? "fex_repack_to_host" : "fex_repack_from_host") << "(" << pointer_prefix << source_parent << source_child.member_name << array_subscript << ")";
    } else {
        // TODO: Simple types like long-double still need repacking
        file << "  " << pointer_prefix << target_parent << target_child.member_name << array_subscript << " = " << pointer_prefix << source_parent << source_child.member_name << array_subscript;
    }

    file << ';';
    if (source_child.offset_bits != std::numeric_limits<uint64_t>::max()) {
        file << " // Offset " << source_child.offset_bits / 8 << " -> " << target_child.offset_bits / 8;
    }
    file << '\n';
};

// Custom sort algorithm that works with partial orders.
//
// In contrast, std::sort requires that any two different elements A and B of
// the input range compare either A<B or B<A. This requirement is violated e.g.
// for dependency relations: Elements A and B might not depend on each other,
// but they both might depend on some third element C. BubbleSort then ensures
// C preceeds both A and B in the sorted range, while leaving the relative
// order of A and B undetermined. In effect when iterating over the sorted
// range, each dependency is visited before any of its dependees.
template<std::forward_iterator It>
void BubbleSort(It begin, It end,
                std::relation<std::iter_value_t<It>, std::iter_value_t<It>> auto compare) {
    while (true) {
        bool fixpoint = true;
        for (auto it = begin; it != end; ++it) {
            for (auto it2 = std::next(it); it2 != end; ++it2) {
                if (compare(*it2, *it)) {
                    std::swap(*it, *it2);
                    fixpoint = false;
                }
            }
        }

        if (fixpoint) {
            return;
        }
    }
}

void GenerateThunkLibsAction::ExecuteAction() {
    clang::ASTFrontendAction::ExecuteAction();

    // Post-processing happens here rather than in an overridden EndSourceFileAction implementation.
    // We can't move the logic to the latter since this code might still raise errors, but
    // clang's diagnostics engine is already shut down by the time EndSourceFileAction is called.
    auto& context = getCompilerInstance().getASTContext();
    if (context.getDiagnostics().hasErrorOccurred()) {
        return;
    }

    // TODO: Move elsewhere
    auto Error = [&context]<std::size_t N>(clang::SourceLocation loc, const char (&message)[N]) -> ClangDiagnosticAsException {
        auto id = context.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error, message);
        return ClangDiagnosticAsException {{ loc, id }};
    };

try {
    const ABI& guest_abi = abi.abis[1]; // TODO: Select properly

    static auto format_decl = [](clang::QualType type, const std::string_view& name) {
        if (type->isFunctionPointerType()) {
            auto signature = type.getAsString();
            const char needle[] = { '(', '*', ')' };
            auto it = std::search(signature.begin(), signature.end(), std::begin(needle), std::end(needle));
            if (it == signature.end()) {
                // It's *probably* a typedef, so this should be safe after all
                return signature + " " + std::string(name);
            } else {
                signature.insert(it + 2, name.begin(), name.end());
                return signature;
            }
        } else {
            auto type_name = type.getAsString();
            if (type_name == "__gnuc_va_list") {
                type_name = "va_list";
            }
            return type_name + " " + std::string(name);
        }
    };

    auto format_struct_members = [](const FunctionParams& params, const char* indent) {
        std::string ret;
        for (std::size_t idx = 0; idx < params.param_types.size(); ++idx) {
            ret += indent + format_decl(params.param_types[idx].getUnqualifiedType(), "a_" + std::to_string(idx)) + ";\n";
        }
        return ret;
    };

    auto format_function_params = [](const FunctionParams& params) {
        std::string ret;
        for (std::size_t idx = 0; idx < params.param_types.size(); ++idx) {
            auto& type = params.param_types[idx];
            ret += format_decl(type, "a_" + std::to_string(idx)) + ", ";
        }
        // drop trailing ", "
        ret.resize(ret.size() > 2 ? ret.size() - 2 : 0);
        return ret;
    };

    auto get_sha256 = [this](const std::string& function_name) {
        std::string sha256_message = libname + ":" + function_name;
        std::vector<unsigned char> sha256(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const unsigned char*>(sha256_message.data()),
               sha256_message.size(),
               sha256.data());
        return sha256;
    };

    auto get_callback_name = [](std::string_view function_name, unsigned param_index, bool is_first_cb) -> std::string {
        return std::string { function_name } + "CBFN" + (is_first_cb ? "" : std::to_string(param_index));
    };

    if (!output_filenames.thunks.empty()) {
        std::ofstream file(output_filenames.thunks);

        file << "extern \"C\" {\n";
        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;
            auto sha256 = get_sha256(function_name);
            file << "MAKE_THUNK(" << libname << ", " << function_name << ", \"";
            bool first = true;
            for (auto c : sha256) {
                file << (first ? "" : ", ") << "0x" << std::hex << std::setw(2) << std::setfill('0') << +c;
                first = false;
            }
            file << "\")\n";
        }
        for (auto& data_symbol : data_symbols) {
            const auto& function_name = "fetchsym_" + data_symbol;
            auto sha256 = get_sha256(function_name);
            file << "MAKE_THUNK(" << libname << ", " << function_name << ", \"";
            bool first = true;
            for (auto c : sha256) {
                file << (first ? "" : ", ") << "0x" << std::hex << std::setw(2) << std::setfill('0') << +c;
                first = false;
            }
            file << "\")\n";
        }
        file << "}\n";
    }

    if (!output_filenames.function_packs_public.empty()) {
        std::ofstream file(output_filenames.function_packs_public);

        file << "extern \"C\" {\n";
        for (auto& data : thunked_api) {
            if (data.custom_guest_impl) {
                continue;
            }

            const auto& function_name = data.function_name;
            const char* variadic_ellipsis = data.is_variadic ? ", ..." : "";

            const bool has_explicit_va_list = (!data.param_types.empty() && data.param_types.back().getAsString() == "__gnuc_va_list");

            if (!data.is_variadic && !has_explicit_va_list) {
                file << "__attribute__((alias(\"fexfn_pack_" << function_name << "\"))) auto " << function_name << "(";
                file << format_function_params(data);
                file << variadic_ellipsis << ") -> " << data.return_type.getAsString() << ";\n";
            } else {
                // Assume this is a printf-like function
                const bool is_vprintf = has_explicit_va_list;
                const int format_arg_idx = data.param_types.size() - 1 - is_vprintf;

                file << "auto " << function_name << "(";
                file << format_function_params(data);
                if (!is_vprintf) {
                    file << ", ...";
                }
                file << ") -> " << data.return_type.getAsString() << " {\n";
                file << "  int num_args = fexfn_pack_parse_printf_format(a_" << format_arg_idx << ", 0, nullptr);\n";
                file << "  auto* arg_types = (int*)alloca(num_args * sizeof(int));\n";
                file << "  auto* args = (printf_packed_arg_t*)alloca(num_args * sizeof(printf_packed_arg_t));\n";

                if (!is_vprintf) {
                    file << "  va_list va_args;\n";
                    file << "  va_start(va_args, a_" << format_arg_idx << ");\n";
                }
                const auto va_list_name = !is_vprintf ? "va_args" : "a_" + std::to_string(data.param_types.size() - 1);
                file << "  pack_printf_va_list(a_" << format_arg_idx << ", " << va_list_name << ", num_args, arg_types, args);\n";
                if (!is_vprintf) {
                    file << "  va_end(va_args);\n";
                }
                file << "  return fexfn_pack_" << function_name << "_internal(";
                for (std::size_t idx = 0; idx < data.param_types.size() - is_vprintf; ++idx) {
                    file << "a_" << idx << ", ";
                }
                file << "num_args, arg_types, args);\n";
                file << "}\n";
            }
        }

        for (std::size_t namespace_idx = 0; namespace_idx < namespaces.size(); ++namespace_idx) {
            bool empty = true;
            for (auto& symbol : thunked_api) {
                if (symbol.symtable_namespace == namespace_idx) {
                    if (empty) {
                        file << "static struct { const char* name; void (*fn)(); } " << namespaces[namespace_idx].name << "_symtable[] = {\n";
                        empty = false;
                    }
                    file << "  { \"" << symbol.function_name << "\", (void(*)())&" << symbol.function_name << " },\n";
                }
            }
            if (!empty) {
                file << "  { nullptr, nullptr }\n";
                file << "};\n";
            }
        }

        file << "}\n";
    }

    auto get_struct_member_annotations = [](std::string_view parent_name, const StructInfo::ChildInfo& member) {
        // TODO: Return proper annotations
        return (/* TODO: cleanup */ parent_name == "struct sigstack" && member.member_name == "ss_sp");
    };

    if (!output_filenames.function_packs.empty()) {
        std::ofstream file(output_filenames.function_packs);

        file << "#include <cstring>\n";
        file << "#include <type_traits>\n";

        auto emit_repacking_code = [this, &file, &guest_abi, &get_struct_member_annotations, &Error](std::string_view parent_type, const StructInfo& source_struct_info, const ABI& target_abi, const StructInfo& target_struct_info) {
            // TODO: memset "into" to zero
            for (auto& target_child : target_struct_info.children) try {
                auto source_child = std::find_if(source_struct_info.children.begin(), source_struct_info.children.end(),
                                                [&target_child](const auto& source_child) { return source_child.member_name == target_child.member_name; });
                if (source_child == source_struct_info.children.end()) {
                    // TODO: Some structs have named padding members (e.g. stat::__pad0), but skipping them should require an explicit annotation
                    file << "  // Omitted " << (&target_abi == &guest_abi ? "guest" : "host") << "-only member \"" << target_child.member_name << "\"\n";
                    continue;
                }

                if (!target_abi.contains(target_child.type_name)) {
                    throw Error(clang::SourceLocation {}, "Don't know how to emit repacking code for %0: Member %1 %2 has no ABI description")
                          .AddString(std::string { parent_type }).AddString(target_child.type_name).AddString(target_child.member_name);
                }

                const auto& child_type_info = target_abi.at(target_child.type_name);
                auto child_annotations = get_struct_member_annotations(parent_type, target_child);
                const bool is_trivially_compatible = (CheckABICompatibility(abi, target_child) == ABICompatibility::Trivial);
                emit_repacking_code_child(file, child_type_info, "from", *source_child, "into", target_child, (&target_abi == &guest_abi ? RepackTo::Guest : RepackTo::Host), child_annotations, is_trivially_compatible);
            } catch (std::exception& err) {
                throw Error(clang::SourceLocation {}, "Can't auto-repack member '%0' of '%1': %2")
                        .AddString(target_child.member_name)
                        .AddString(std::string { parent_type })
                        .AddString(err.what());
            }
        };

        file << "template<typename> struct fex_host_type;\n";

        // Specialize for pointers
        file << "template<typename T> struct fex_host_type<T*> {\n";
        file << "  fex_host_type<T>* data;\n";
        file << "  fex_host_type<T>& operator*() { return *data; }\n";
        file << "  fex_host_type& operator=(fex_host_type<T>* ptr) { data = ptr; return *this; }\n";
        // TODO: Shouldn't enable these operators by default, instead enable them only for ABI compatible types
        file << "  fex_host_type& operator=(T* ptr) { memcpy(&this->data, &ptr, sizeof(ptr)); return *this; }\n";
        file << "  operator T*() const { T* ptr; memcpy(&ptr, &this->data, sizeof(ptr)); return ptr; }\n";
        file << "};\n";
        file << "template<typename T> struct fex_host_type<const T*> : fex_host_type<T*> {\n";
        file << "  fex_host_type& operator=(fex_host_type<std::remove_const_t<T>>* ptr) { memcpy(&this->data, &ptr, sizeof(ptr)); return *this; }\n";
        file << "};\n";

        // Used for pointers to untyped data (e.g. free), which are compatible with the host ABI (e.g. x86_64 on aarch64) or can be zero-extended (e.g. x86_32 on aarch64)
        file << "template<typename T> struct fex_untyped_host_pointer { uint64_t host_pointer; };\n";

        // Compares such that A < B if B contains A as a member and requires A to be completely defined (i.e. non-pointer/non-reference).
        // This applies recursively to structs contained by B.
        struct compare_by_struct_dependency {
            const ABI& abi;

            bool operator()(const std::pair<std::string, TypeInfo>& a, const std::pair<std::string, TypeInfo>& b) /*const*/ {
                auto* b_as_struct = std::get_if<StructInfo>(&b.second);
                if (!b_as_struct) {
                    // Not a struct => no dependency
                    return false;
                }

                for (auto& child : b_as_struct->children) {
                    if (child.type_name == a.first) {
                        return true;
                    }

                    if (child.type_name == b.first) {
                        // Pointer to the struct itself, no need to recurse
                        continue;
                    }

//                    // If this is a pointer (and not a fixed-size array), we don't need a complete definition
//                    if (!child.pointer_chain.empty() && std::none_of(child.pointer_chain.begin(), child.pointer_chain.end(),
//                                                                      [](const PointerInfo& ptr) { return ptr.array_size.has_value(); })) {
//                         continue;
//                    }

                    auto child_info = abi.find(child.type_name);
                    if (child_info != abi.end() && (*this)(a, *child_info)) {
                        // Child depends on A => transitive dependency
                        return true;
                    }
                }

                // No dependency found
                return false;
            }
        };

        std::vector<std::pair<std::string, TypeInfo>> types(abi.host().size());
        std::copy(abi.host().begin(), abi.host().end(), types.begin());
        auto comparer = compare_by_struct_dependency{abi.host()};
        BubbleSort(types.begin(), types.end(), comparer);

        for (auto& type : types) {
            if (auto* host_struct_info = std::get_if<StructInfo>(&type.second)) {
                // TODO: Assert they have common members

                auto struct_name = type.first;
                if (!guest_abi.count(type.first)) {
                    continue;
                }
                const auto& guest_struct_info = std::get<StructInfo>(guest_abi.at(type.first));

                // TODO: Only use __attribute__((__packed__)) if the guest struct uses it? (needed for e.g. xcb_present_redirect_notify_event_t)
                file << "template<> struct __attribute__((packed)) fex_host_type<" << struct_name << "> {\n";
                // TODO: Needs fex_host_type declared for all children
                uint64_t padded_member_offset = 0;
                for (auto& child : host_struct_info->children) {
                    // Since this structure is always packed, ensure the child starts at the right target offset
                    if (padded_member_offset != child.offset_bits) {
                        file << "  char padding_for_" << child.member_name << "[" << (child.offset_bits - padded_member_offset) / 8 << "];\n";
                        padded_member_offset = child.offset_bits;
                    }

                    if (!abi.host().contains(child.type_name)) {
                        throw Error(clang::SourceLocation{}, "Can't build fex_host_type<%2>: No ABI description for member \"%0 %1\"")
                              .AddString(child.type_name).AddString(child.member_name).AddString(struct_name);
                    }
                    const auto array_size = child.pointer_chain.size() == 1 ? child.pointer_chain.back().array_size : std::nullopt;
                    std::string pointer_stars;
                    if (!array_size) {
                        for ([[maybe_unused]] auto& pointer_info : child.pointer_chain) {
                            pointer_stars += "*";
                        }
                    }
                    if (std::holds_alternative<StructInfo>(abi.host().at(child.type_name))) {
                        file << "  fex_host_type<" << child.type_name << pointer_stars << ">";
                    } else {
                        file << "  " << child.type_name << pointer_stars;
                    }
                    file << " " << child.member_name;
                    if (array_size) {
                        file << '[' << *array_size << ']';
                    }
                    file << "; // " << child.offset_bits / 8 << "\n";

                    // TODO: The size of the *guest* type should be used here instead since that's the context the struct will be used in. This will matter for 32-bit libraries
                    padded_member_offset += child.size_bits;
                }
                if (padded_member_offset < host_struct_info->size_bits) {
                    file << "  char padding_for_struct_alignment[" << (host_struct_info->size_bits - padded_member_offset) / 8 << "];\n";
                }
                file << "};\n";
                // TODO: Remove if-check once unions are supported
                if (!host_struct_info->is_union) {
                    file << "static_assert(sizeof(fex_host_type<" << struct_name << ">) == " << host_struct_info->size_bits / 8 << ");\n\n";
                }

                if (CheckABICompatibility(abi, struct_name) == ABICompatibility::Repack) {
                    // Repacker for guest->host layout
                    file << "inline fex_host_type<" << struct_name << "> fex_repack_to_host(std::add_lvalue_reference_t<std::add_const_t<" << struct_name << ">> from) {\n";
                    file << "  fex_host_type<" << struct_name << "> into {};\n"; // TODO: Doesn't work with const members
                    emit_repacking_code(struct_name, guest_struct_info, abi.host(), *host_struct_info);
                    file << "  return into;\n";
                    file << "}\n\n";

                    // Repacker for host->guest layout
                    file << "inline " << struct_name << " fex_repack_from_host(const fex_host_type<" << struct_name << ">& from) {\n";
                    file << "  " << struct_name << " into {};\n"; // TODO: Doesn't work with const members
                    emit_repacking_code(struct_name, *host_struct_info, guest_abi, guest_struct_info);
                    file << "  return into;\n";
                    file << "}\n\n";
                }
            }
        }

        file << "extern \"C\" {\n";
        for (auto& data : thunks) {
            const auto& function_name = data.function_name;
            bool is_void = data.return_type->isVoidType();
            file << "FEX_PACKFN_LINKAGE auto fexfn_pack_" << function_name << "(" << format_function_params(data);
            // Using trailing return type as it makes handling function pointer returns much easier
            file << ") -> " << data.return_type.getAsString() << " {\n";
            file << "  struct {\n";
            std::vector<ABICompatibility> param_abi_compat;
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto& type = data.param_types[idx];
                auto& annotations = data.parameter_annotations[idx];
                try {
                    auto compat = CheckABICompatibility(abi, type->getUnqualifiedDesugaredType(), annotations);
                    if (type->isPointerType() && compat == ABICompatibility::Unknown) {
                        throw Error(data.decl->getNameInfo().getBeginLoc(),
                                    "Could not determine ABI compatibility for parameter of type %0. Are you missing any annotations?")
                                .AddTaggedVal(type)
                                .AddSourceRange({data.decl->getSourceRange(), false});
                    }

                    if (!annotations.in && !annotations.out) {
                        if (compat != ABICompatibility::Unknown && compat != ABICompatibility::Repack) {
                            // Copying arguments is cheap (or not needed in the first place), hence we can just enable it by default
                            annotations.in = true;
                            annotations.out = true;
                        } else {
                            // If the argument needs repacking, we need to know whether to apply it before or after invoking the thunk (or both)
                            throw Error(data.decl->getNameInfo().getBeginLoc(),
                                        "Parameter %0 of type %1 lacks input/output annotations required due to non-trivial argument conversion")
                                    .AddTaggedVal(data.decl->parameters()[idx])
                                    .AddTaggedVal(type)
                                    .AddSourceRange({data.decl->getSourceRange(), false});
                        }
                    }

                    param_abi_compat.push_back(compat);
                } catch (std::runtime_error& err) {
                    throw Error(data.decl->getNameInfo().getBeginLoc(), "Can't auto-pack '%0': %1")
                            .AddString(function_name)
                            .AddString(err.what())
                            .AddSourceRange({data.decl->getSourceRange(), false});
                }
            }

            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto& type = data.param_types[idx];
                if (param_abi_compat[idx] == ABICompatibility::Repack) { // && type.getUnqualifiedType().getAsString() != "void **" /* TODO: Drop workaround for _internal variadic functions */) {
                    // TODO: Define fex_host_type for pointers?
                    file << "    " << "fex_host_type<" << type.getUnqualifiedType().getAsString() << "> a_" + std::to_string(idx) << ";\n";
                } else if (param_abi_compat[idx] == ABICompatibility::RepackNestedPtr) {
                    // TODO: Distinguish guest/host pointer types for 32-bit support
                    file << "    " << type.getUnqualifiedType().getAsString() << " a_" + std::to_string(idx) << ";\n";
                } else if (param_abi_compat[idx] == ABICompatibility::Trivial) {
                    file << "    " << format_decl(type.getUnqualifiedType(), "a_" + std::to_string(idx)) << ";\n";
                } else if (param_abi_compat[idx] == ABICompatibility::GuestPtrAsInt) {
                    // TODO: 32-bit support
                    file << "    " << format_decl(type.getUnqualifiedType(), "a_" + std::to_string(idx)) << ";\n";
                } else if (param_abi_compat[idx] == ABICompatibility::PackedPtr) {
                    // TODO: 32-bit support
                    file << "    " << format_decl(type.getUnqualifiedType(), "a_" + std::to_string(idx)) << ";\n";
                } else {
                  throw std::runtime_error("Unknown ABI compatibility for function parameter of type " + type.getAsString()); // TODO: Proper error
                }
            }
            if (!is_void) {
                file << "    " << format_decl(data.return_type, "rv") << ";\n";
            } else if (data.param_types.size() == 0) {
                // Avoid "empty struct has size 0 in C, size 1 in C++" warning
                file << "    char force_nonempty;\n";
            }
            file << "  } args;\n";

            // Initialize host argument structure (repacking guest->host structs if needed)
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                if (!data.parameter_annotations[idx].in) {
                    // TODO: Ideally, avoid declaring the corresponding member in the "args" structure altogether
                    continue;
                }
                // TODO: Converting to ChildInfo may be too specific. Use GetTypeInfoFromClang instead?
                auto& type = data.param_types[idx];
                auto field_type = data.param_types.at(idx).getCanonicalType()->getUnqualifiedDesugaredType();
                auto guest_args = FromClangType(context, field_type, "a_" + std::to_string(idx), std::nullopt, std::nullopt);
                auto host_args = FromClangType(context, field_type, "a_" + std::to_string(idx), std::nullopt, std::nullopt);
                if (abi.host().count(host_args.type_name)) {
                    if (param_abi_compat[idx] == ABICompatibility::Trivial ||
                        param_abi_compat[idx] == ABICompatibility::GuestPtrAsInt ||
                        param_abi_compat[idx] == ABICompatibility::PackedPtr) { // && type.getUnqualifiedType().getAsString() != "void **" /* TODO: Drop workaround for _internal variadic functions */) {
                        file << "  args.a_" << idx << " = a_" << idx << ";\n";
                    } else if (type->isPointerType() && type->getPointeeType()->isPointerType()) {
                        // TODO: Check for ABICompatibility::RepackNestedPointer instead!

                        // Double pointer arguments must either be annotated (handled above) or point to trivially ABI compatible data.
                        // This commonly affects creator-functions that operate on opaque types (e.g. snd_input_stdio_open(snd_input_t **inputp, ...)).
                        // Struct repacking isn't attempted here, as too many guesses about the pointer semantics would have to be made.

                        // TODO: Check that the parameter is a pointer to ABICompatibility::Trivial/GuestPtrAsInt/PackedPtr
                        // Allocate temp_ on stack, assign args.a_ to it, then zext pointer into temp_
                        auto temp_type = "decltype(*args.a_" + std::to_string(idx) + ")";
                        file << "  uint64_t raw_temp_a" << idx << " = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(a_" << idx << "))" << ";\n";
                        file << "  " << temp_type << " temp_a" << idx << " = reinterpret_cast<" << temp_type << ">(raw_temp_a" << idx << ")" << ";\n";
                        file << "  args.a_" << idx << " = &temp_a" << idx << ";\n";
                    } else if (param_abi_compat[idx] == ABICompatibility::Repack) {
                        const auto& type_info = abi.host().at(host_args.type_name);
                        // TODO: Include function parameter annotations
                        // TODO: Should be fex_wrapped for struct types

                        auto new_source_child = guest_args;
                        auto new_target_child = host_args;

                        if (guest_args.pointer_chain.empty() && host_args.pointer_chain.empty()) {
                            // Nothing special to do to prepare repacking
                        } else if (guest_args.pointer_chain.size() == host_args.pointer_chain.size()) {
                            // TODO: This can (and should) be skipped for trivial ABI compatibility...
                            assert(guest_args.pointer_chain == host_args.pointer_chain);

                            // Peel off one pointer layer and recurse
                            new_source_child.member_name = '*' + new_source_child.member_name;
                            new_source_child.pointer_chain.pop_back();

                            new_target_child.member_name = "*args." + new_target_child.member_name;
                            new_target_child.pointer_chain.pop_back();

                            // Repoint the pointer to a stack-allocated struct instance with host layout
                            const bool is_struct = true;
                            new_target_child.member_name = "temp_" + host_args.member_name;
                            file << "  " << (is_struct ? "fex_host_type<" : "") << host_args.type_name << (is_struct ? ">" : "") << " " << new_target_child.member_name << ";\n";
                            file << "  " << "args." << host_args.member_name << " = &" << new_target_child.member_name << ";\n";
                        } else {
                            throw std::runtime_error("Can't repack nested pointers");
                        }

                        emit_repacking_code_child(file, type_info, "", new_source_child, "", new_target_child, RepackTo::Host, false, false);
                    } else {
                        assert(false);
                    }
                } else {
                    // TODO: Error. Can't decide if repacking is needed without ABI description
                    file << "  // TODO: Can't repack " << host_args.type_name << " without ABI info\n";
                    // TODO: Error out instead of trying anyway...
                    file << "  memcpy(&args.a_" << idx << ", (const void*)&a_" << idx << ", sizeof(a_" << idx << "));\n";
                }
            }

            file << "\n  fexthunks_" << libname << "_" << function_name << "(&args);\n\n";

            // Repack host->guest structs for output parameters
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                // NOTE: Even for by-value parameters, structs may contain pointers that may need to be updated!
                const auto qual_field_type = data.param_types.at(idx).getCanonicalType();
                auto field_type = qual_field_type->getUnqualifiedDesugaredType();
                if (!data.parameter_annotations[idx].out) {
                    continue;
                }

                if (field_type->isPointerType() || field_type->isRecordType()) {
                    // TODO: Use GetTypeInfoFromClang instead?
                    // TODO: For plain pointer types (e.g. char*), this currently does an incorrect pointer reassignment instead of a data reassignment!!
                    auto guest_args = FromClangType(context, field_type, "a_" + std::to_string(idx), std::nullopt, std::nullopt);
                    auto host_args = FromClangType(context, field_type, "a_" + std::to_string(idx), std::nullopt, std::nullopt);
                    if (abi.host().count(host_args.type_name)) {
                        // TODO: Handle ABICompatibility::RepackNestedPointer
                        if (param_abi_compat[idx] == ABICompatibility::Repack) {
                            const auto& type_info = abi.host().at(host_args.type_name);
                            // TODO: Include function parameter annotations
                            // TODO: Should be fex_wrapped for struct types
                            emit_repacking_code_child(file, type_info, "args", host_args, "", guest_args, RepackTo::Guest, false, false);
                        } else {
                            file << "  // NO NEED TO REPACK OUTGOING DATA\n";
                        }
                    } else {
                        // TODO: Error. Can't decide if repacking is needed without ABI description
                        file << "  // TODO: Can't repack " << host_args.type_name << " without ABI info\n";
                    }
                }
            }

            if (!is_void) {
                file << "  return args.rv;\n";
            }
            file << "}\n";
        }
        for (auto& data_symbol : data_symbols) {
            const auto& function_name = "fetchsym_" + data_symbol;
            file << "static void* fexfn_pack_" << function_name << "() {\n";
            file << "  struct {\n";
            file << "    void* rv;\n";
            file << "  } args;\n";
            file << "  fexthunks_" << libname << "_" << function_name << "(&args);\n";
            file << "  return args.rv;\n";
            file << "}\n";
        }

        file << "}\n";
    }

    if (!output_filenames.function_unpacks.empty()) {
        std::ofstream file(output_filenames.function_unpacks);

        file << "extern \"C\" {\n";
        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;
            bool is_void = thunk.return_type->isVoidType();

            file << "struct fexfn_packed_args_" << libname << "_" << function_name << " {\n";
            file << format_struct_members(thunk, "  ");
            if (!is_void) {
                file << "  " << format_decl(thunk.return_type, "rv") << ";\n";
            } else if (thunk.param_types.size() == 0) {
                // Avoid "empty struct has size 0 in C, size 1 in C++" warning
                file << "    char force_nonempty;\n";
            }
            file << "};\n";

            /* Generate stub callbacks */
            for (auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub) {
                    bool is_first_cb = (cb_idx == thunk.callbacks.begin()->first);
                    const char* variadic_ellipsis = cb.is_variadic ? ", ..." : "";
                    auto cb_function_name = "fexfn_unpack_" + get_callback_name(function_name, cb_idx, is_first_cb) + "_stub";
                    file << "[[noreturn]] static " << cb.return_type.getAsString() << " "
                         << cb_function_name << "("
                         << format_function_params(cb) << variadic_ellipsis << ") {\n";
                    file << "  fprintf(stderr, \"FATAL: Attempted to invoke callback stub for " << function_name << "\\n\");\n";
                    file << "  std::abort();\n";
                    file << "}\n";
                }
            }

            file << "/*static */void fexfn_unpack_" << libname << "_" << function_name << "(fexfn_packed_args_" << libname << "_" << function_name << "* args) {\n";

            const auto impl_function_name = (thunk.custom_host_impl ? "fexfn_impl_" : "fexldr_ptr_") + libname + "_" + function_name;
            auto format_param = [&](std::size_t idx) {
                auto cb = thunk.callbacks.find(idx);
                if (cb != thunk.callbacks.end() && cb->second.is_stub) {
                    bool is_first_cb = (cb->first == thunk.callbacks.begin()->first);
                    return "fexfn_unpack_" + get_callback_name(function_name, cb->first, is_first_cb) + "_stub";
                } else if (cb != thunk.callbacks.end() && cb->second.is_guest) {
                    return "fex_guest_function_ptr { args->a_" + std::to_string(idx) + " }";
                } else {
                    return "args->a_" + std::to_string(idx);
                }
            };

            if(thunk.variadic_strategy == VariadicStrategy::LikePrintf) {
                const bool is_vprintf = !thunk.decl->isVariadic();
                const int format_arg_idx = thunk.param_types.size() - 4;
                assert(thunk.return_type.getAsString() == "int");
                file << "  int chars_written = 0;\n";
                file << "  auto do_printf = wrap_format_function<"
                     << thunk.sprintf_buffer.value_or(-1) << ", "
                     << thunk.snprintf_buffer_size.value_or(-1) << ", "
                     << (is_vprintf ? "true" : "false")
                     << ">(" << "fexldr_ptr_" + libname + "_" + thunk.GetOriginalFunctionName() << ", chars_written";
                for (std::size_t idx = 0; idx < format_arg_idx; ++idx) {
                    file << ", " << format_param(idx);
                }
                file << ");\n";
                file << "  unpack_printf_params(args->a_" << format_arg_idx << ", args->a_" << format_arg_idx + 1
                     << ", args->a_" << format_arg_idx + 2 << ", args->a_" << format_arg_idx + 3 << ", do_printf);\n";
                file << "  args->rv = chars_written;\n";
            } else {
                file << (is_void ? "  " : "  args->rv = ") << impl_function_name << "(";
                file << format_function_args(thunk, format_param);
                file << ");\n";
            }
            file << "}\n";
        }

        file << "}\n";
    }

    if (!output_filenames.tab_function_unpacks.empty()) {
        std::ofstream file(output_filenames.tab_function_unpacks);

        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;
            auto sha256 = get_sha256(function_name);

            file << "{(uint8_t*)\"";
            for (auto c : sha256) {
                file << "\\x" << std::hex << std::setw(2) << std::setfill('0') << +c;
            }
            file << "\", &fexfn_type_erased_unpack<fexfn_unpack_" << libname << "_" << function_name << ">}, // " << libname << ":" << function_name << "\n";
        }
        for (auto& data_symbol : data_symbols) {
            const auto& function_name = "fetchsym_" + data_symbol;
            auto sha256 = get_sha256(function_name);
            file << "{(uint8_t*)\"";
            for (auto c : sha256) {
                file << "\\x" << std::hex << std::setw(2) << std::setfill('0') << +c;
            }
            file << "\", &fexfn_fetch_symbol<&" << data_symbol << ">}, // " << libname << ":" << function_name << "\n";
        }
    }

    if (!output_filenames.ldr.empty()) {
        std::ofstream file(output_filenames.ldr);

        file << "static void* fexldr_ptr_" << libname << "_so;\n";
        file << "extern \"C\" bool fexldr_init_" << libname << "() {\n";

        std::string version_suffix;
        if (lib_version) {
          version_suffix = '.' + std::to_string(*lib_version);
        }
        const std::string library_filename = libfilename + ".so" + version_suffix;
        file << "  fexldr_ptr_" << libname << "_so = dlopen(\"" << library_filename << "\", RTLD_LOCAL | RTLD_LAZY);\n";

        file << "  if (!fexldr_ptr_" << libname << "_so) { return false; }\n\n";
        for (auto& import : thunked_api) {
            file << "  (void*&)fexldr_ptr_" << libname << "_" << import.function_name << " = " << import.host_loader << "(fexldr_ptr_" << libname << "_so, \"" << import.function_name << "\");\n";
        }
        for (auto& data_symbol : data_symbols) {
            file << "  fexldr_dataptr_" << data_symbol << " = " << "dlsym" << "(fexldr_ptr_" << libname << "_so, \"" << data_symbol << "\");\n";
        }
        file << "  return true;\n";
        file << "}\n";
    }

    if (!output_filenames.ldr_ptrs.empty()) {
        std::ofstream file(output_filenames.ldr_ptrs);

        for (auto& import : thunked_api) {
            const auto& function_name = import.function_name;
            const char* variadic_ellipsis = import.is_variadic ? ", ..." : "";
            file << "using fexldr_type_" << libname << "_" << function_name << " = auto " << "(" << format_function_params(import) << variadic_ellipsis << ") -> " << import.return_type.getAsString() << ";\n";
            file << "static fexldr_type_" << libname << "_" << function_name << " *fexldr_ptr_" << libname << "_" << function_name << ";\n";
        }
        for (auto& data_symbol : data_symbols) {
            file << "void* fexldr_dataptr_" << data_symbol << ";\n";
        }
    }

    if (!output_filenames.callback_structs.empty()) {
        std::ofstream file(output_filenames.callback_structs);

        for (auto& thunk : thunks) {
            for (const auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub || cb.is_guest) {
                    continue;
                }

                file << "struct " << thunk.GetOriginalFunctionName() << "CB_Args {\n";
                file << format_struct_members(cb, "  ");
                if (!cb.return_type->isVoidType()) {
                    file << "  " << format_decl(cb.return_type, "rv") << ";\n";
                }
                file << "};\n";
            }
        }
    }

    if (!output_filenames.callback_typedefs.empty()) {
        std::ofstream file(output_filenames.callback_typedefs);

        for (auto& thunk : thunks) {
            for (const auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub || cb.is_guest) {
                    continue;
                }

                bool is_first_cb = (cb_idx == thunk.callbacks.begin()->first);
                auto cb_function_name = get_callback_name(thunk.GetOriginalFunctionName(), cb_idx, is_first_cb);
                file << "typedef " << cb.return_type.getAsString() << " "
                     << cb_function_name << "("
                     << format_function_params(cb) << ");\n";
            }
        }
    }

    if (!output_filenames.callback_unpacks.empty()) {
        std::ofstream file(output_filenames.callback_unpacks);

        for (auto& thunk : thunks) {
            for (const auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub || cb.is_guest) {
                    continue;
                }

                bool is_void = cb.return_type->isVoidType();
                bool is_first_cb = (cb_idx == thunk.callbacks.begin()->first);
                auto cb_function_name = thunk.function_name + "CB" + (is_first_cb ? "" : std::to_string(cb_idx));
                file << "static void fexfn_unpack_" << libname << "_" << cb_function_name << "(uintptr_t cb, void* argsv) {\n";
                file << "  typedef " << cb.return_type.getAsString() << " fn_t (" << format_function_params(cb) << ");\n";
                file << "  auto callback = reinterpret_cast<fn_t*>(cb);\n";
                file << "  struct arg_t {\n";
                file << format_struct_members(cb, "    ");
                if (!is_void) {
                    file << "    " << format_decl(cb.return_type, "rv") << ";\n";
                }
                file << "  };\n";
                file << "  auto args = (arg_t*)argsv;\n";
                file << (is_void ? "  " : "  args->rv = ") << "callback(" << format_function_args(cb) << ");\n";
                file << "}\n";
            }
        }
    }

    if (!output_filenames.callback_unpacks_header.empty()) {
        std::ofstream file(output_filenames.callback_unpacks_header);

        for (auto& thunk : thunks) {
            for (const auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub || cb.is_guest) {
                    continue;
                }

                bool is_first_cb = (cb_idx == thunk.callbacks.begin()->first);
                auto cb_function_name = thunk.GetOriginalFunctionName() + "CB" + (is_first_cb ? "" : std::to_string(cb_idx));
                file << "uintptr_t " << libname << "_" << cb_function_name << ";\n";
            }
        }
    }

    if (!output_filenames.callback_unpacks_header_init.empty()) {
        std::ofstream file(output_filenames.callback_unpacks_header_init);

        for (auto& thunk : thunks) {
            for (const auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub || cb.is_guest) {
                    continue;
                }

                bool is_first_cb = (cb_idx == thunk.callbacks.begin()->first);
                auto cb_function_name = thunk.function_name + "CB" + (is_first_cb ? "" : std::to_string(cb_idx));
                file << "(uintptr_t)&fexfn_unpack_" << libname << "_" << cb_function_name << ",\n";
            }
        }
    }

    if (!output_filenames.symbol_list.empty()) {
        std::ofstream file(output_filenames.symbol_list);

        for (std::size_t namespace_idx = 0; namespace_idx < namespaces.size(); ++namespace_idx) {
            const auto& ns = namespaces[namespace_idx];
            file << "#define FOREACH_" << ns.name << (ns.name.empty() ? "" : "_") << "SYMBOL(EXPAND) \\\n";
            for (auto& symbol : thunked_api) {
                if (symbol.symtable_namespace.value_or(0) == namespace_idx) {
                    file << "  EXPAND(" << symbol.function_name << ", \"TODO\") \\\n";
                }
            }
            file << "\n";
        }
    }
    } catch (std::exception& exception) {
        auto error = Error(clang::SourceLocation{}, "Exception thrown while generating functions: %0");
        error.AddString(exception.what());
        error.Report(context.getDiagnostics());
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
    }
}

std::unique_ptr<clang::ASTConsumer> GenerateThunkLibsAction::CreateASTConsumer(clang::CompilerInstance&, clang::StringRef) {
    return std::make_unique<ASTConsumer>();
}

#include <unordered_set>

#include "abi.h"

class AnalyzeABIVisitor : public clang::RecursiveASTVisitor<AnalyzeABIVisitor> {
    clang::ASTContext& context;
    ABI& type_abi;

    template<std::size_t N>
    [[nodiscard]] ClangDiagnosticAsException Error(clang::SourceLocation loc, const char (&message)[N]) {
        auto id = context.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error, message);
        return { std::pair(loc, id) };
    }

    // Type data from annotations
    struct TypeData {
        // If true, objects of this type are never accessed directly on this ABI
        bool is_opaque = false;

        // For pointer members, a pointer type can be specified here
        std::unordered_map<const clang::FieldDecl*, MemberAnnotations> members;
    };

    std::unordered_map<const clang::Type*, TypeData> types;

public:
    AnalyzeABIVisitor(clang::ASTContext& context_, ABI& abi_) : context(context_), type_abi(abi_) {
        // Types used internally
        // uint64_t
        HandleType(clang::SourceLocation{}, context.getIntTypeForBitwidth(64, false).getTypePtr());
    }

    bool TraverseDecl(clang::Decl* decl) {
        const bool ret = clang::RecursiveASTVisitor<AnalyzeABIVisitor>::TraverseDecl(decl);

        // TranslationUnitDecl is the final declaration to be processed
        if (auto* unit = llvm::dyn_cast_or_null<clang::TranslationUnitDecl>(decl)) try {
            // TODO: Check that there is exactly one of each!
            auto fex_gen_configs1 = llvm::dyn_cast<clang::ClassTemplateDecl>(unit->lookup(&context.Idents.get("fex_gen_config"))[0])->specializations();
            auto fex_gen_params = llvm::dyn_cast<clang::ClassTemplateDecl>(unit->lookup(&context.Idents.get("fex_gen_param"))[0])->specializations();
            auto fex_gen_types = llvm::dyn_cast<clang::ClassTemplateDecl>(unit->lookup(&context.Idents.get("fex_gen_type"))[0])->specializations();

            std::vector<clang::ClassTemplateSpecializationDecl*> fex_gen_configs { fex_gen_configs1.begin(), fex_gen_configs1.end() };
            // TODO: Don't hardcode namespace name. Instead, look through *all* namespaces and aggregate their annotations
            auto internal_ns = unit->lookup(&context.Idents.get("internal"));
            if (!internal_ns.empty()) {
                auto fex_gen_configs2 = llvm::dyn_cast<clang::ClassTemplateDecl>(llvm::dyn_cast<clang::NamespaceDecl>(internal_ns[0])->lookup(&context.Idents.get("fex_gen_config"))[0])->specializations();
                fex_gen_configs.insert(fex_gen_configs.end(), fex_gen_configs2.begin(), fex_gen_configs2.end());
            }

            // Map from function name to map from parameter index to annotation decl
            std::unordered_map<std::string_view, std::unordered_map<unsigned, clang::ClassTemplateSpecializationDecl*>> function_parameter_annotations;
            for (auto* annotation : fex_gen_params) {
                const auto& template_args = annotation->getTemplateArgs();
                auto function = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl());
                auto param_idx = template_args[1].getAsIntegral().getExtValue();
                function_parameter_annotations[function->getName()][param_idx] = annotation;
            }

            HandleType(clang::SourceLocation{}, context.getIntTypeForBitwidth(64, false).getTypePtr());
            for (auto* decl : fex_gen_types) {
                HandleTypeAnnotations(decl);
            }

            for (auto* decl : fex_gen_configs) {
                HandleMiscAnnotations(decl);

                const auto& template_args = decl->getTemplateArgs();
                if (auto function_decl = llvm::dyn_cast<clang::FunctionDecl>(template_args[0].getAsDecl())) {
                    // Register types of pointer parameters, skipping those that are annnotated
                    for (unsigned param_idx = 0; param_idx < function_decl->getNumParams(); ++param_idx) {
                        clang::ParmVarDecl* param = function_decl->parameters()[param_idx];
                        auto annotations_it = function_parameter_annotations.find(function_decl->getNameAsString());
                        if (annotations_it != function_parameter_annotations.end()) {
                            if (annotations_it->second.count(param_idx)) {
                                // TODO: Check annotation type. Not all should cause this to be skipped
                                continue;
                            }
                        }
                        HandleType(param->getBeginLoc(), param->getType().getTypePtr());
                    }
                    // TODO: handle returned types too
                }
            }

            // For all struct types, also pull in the types of their members (unless the struct is an opaque type)
            for (auto type = types.begin(); type != types.end();) {
                if (auto* record_type = type->first->getAs<clang::RecordType>()) {
                    if (!type->second.is_opaque) {
                        auto num_types = types.size();

                        for (auto* field : record_type->getDecl()->fields()) {
                            if (field->getType()->isFunctionPointerType()) {
                                throw Error(field->getBeginLoc(), "Could not build ABI description for type %0: Member %1 is a function pointer")
                                      .AddTaggedVal(record_type->getDecl()).AddTaggedVal(field);
                            }
                            HandleType(field->getBeginLoc(), field->getType().getTypePtr());
                        }

                        if (num_types != types.size()) {
                            // If new elements have been inserted, repeat
                            type = types.begin();
                            continue;
                        }
                    }
                }
                ++type;
            }

            while (!types.empty()) {
                auto [type, type_data]  = *types.begin();
                types.erase(type);

                auto type_name = static_cast<clang::QualType>(type->getCanonicalTypeUnqualified()).getAsString();

                if (std::any_of(type_abi.begin(), type_abi.end(), [&](auto& info) { return info.first == type_name; })) {
                    continue;
                }

                try {
                    type_abi.emplace( type_name,
                                      type_data.is_opaque ? TypeInfo {} : GetTypeInfoFromClang(context, type_abi.platform, type, type_data.members));
                } catch (std::runtime_error& exc) {
                    throw Error(clang::SourceLocation{}, "Could not build ABI description for type %0: %1")
                          .AddTaggedVal(type->getCanonicalTypeUnqualified()).AddString(exc.what());
                }
            }

            // Completeness check: Remove types that can't be checked for ABI incompatibility, and any types those are contained in
            bool changed = true;
            while (changed) {
                changed = false;

                for (const auto& [type_name, type_info] : type_abi) {
                    if (auto struct_info = type_info.as_struct()) {
                        bool remove = false;
                        std::string removal_reason;
                        if (struct_info->is_union && !type_abi.at(type_name).is_opaque()) {
                            // Non-opaque unions are not supported yet
                            // TODO: Any union of trivially compatible data should itself be detected as trivially compatible
                            removal_reason = "is a union";
                            remove = true;
                        }

                        for (auto& child : struct_info->children) {
                            std::string_view anon_indicator = "(anonymous at";
                            if (std::search(child.type_name.begin(), child.type_name.end(),
                                            anon_indicator.begin(), anon_indicator.end()) != child.type_name.end()) {
                                remove = true;
                            }
                            if (!type_abi.contains(child.type_name)) {
                                // No ABI description of the member available, so we can't reliably the describe parent ABI either
                                removal_reason = "has member \"" + child.type_name + " " + child.member_name + "\" without ABI description";
                                // TODO: Don't remove if this member has a pointer annotation. For now, we just avoid removing for all pointers...
                                if (child.pointer_chain.empty() || child.pointer_chain[0].array_size) {
                                    remove = true;
                                }
                            } else if (child.member_name == "") {
                                // Some structs have unnamed bit fields, e.g. struct timex
                                removal_reason = "has unnamed member";
                                remove = true;
                            } else if (child.pointer_chain.size() != 1 &&
                                       std::any_of(child.pointer_chain.begin(), child.pointer_chain.end(),
                                                   [](auto& info) -> bool { return info.array_size.has_value(); })) {
                                // Nesting pointers and array is not supported yet
                                // TODO: See below. Pointers aren't supported at all
                                removal_reason = "has member \"" + child.type_name + " " + child.member_name + "\" that is a nested pointer/array";
                                remove = true;
                            } else if (child.pointer_chain.size() == 1 &&
                                       !child.pointer_chain[0].array_size) {
                                // TODO: Actually, we don't support these at all. Structs like ftsent make this really difficult to do safely!
                                // TODO: For pointers to trivially compatible data types, this should be fine...
                                removal_reason = "has pointer member \"" + child.type_name + " " + child.member_name + "\"";
//                                remove = true;
                            } else if (child.type_name.starts_with("union ")) {
                                // Unions are not supported yet
                                removal_reason = "has member \"" + child.type_name + " " + child.member_name + "\" that is a union";
                                remove = true;
                            } else if (child.bitfield_size) {
                                // Bit fields are not supported yet
                                removal_reason = "has bit field member \"" + child.type_name + " " + child.member_name + "\"";
                                remove = true;
                            }
                        }

                        if (remove) {
                            // Remove parent type since we can't accurately describe its ABI
                            std::cerr << "WARNING: Dropping ABI description of \"" << type_name << "\" (" + removal_reason + ")\n";
                            type_abi.erase(type_name);
                            changed = true;
                            goto repeat;
                        }
                    }
                }
            repeat:;
            }

            // Mark pointers to opaque types as untyped
            for (auto& type : type_abi) {
                auto type_as_struct = type.second.as_struct();
                if (!type_as_struct) {
                    continue;
                }
                for (auto& child : type_as_struct->children) {
                    if (type_abi.contains(child.type_name) && type_abi.at(child.type_name).is_opaque()) {
                        assert(child.pointer_chain.size() == 1 && !child.pointer_chain.back().array_size);
                        child.pointer_chain.back().type = PointerInfo::Type::Untyped;
                    }
                }
            }
        } catch (ClangDiagnosticAsException& exception) {
            exception.Report(context.getDiagnostics());
        }

        return ret;
    }

    /**
     * Called on instances of "template<> struct fex_gen_type<LibraryType> { ... }"
     */
    void HandleTypeAnnotations(clang::ClassTemplateSpecializationDecl* decl) try {
        assert(decl->getName() == "fex_gen_type");

        if (decl->getSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition) {
            throw Error(decl->getBeginLoc(), "fex_gen_config may not be partially specialized\n");
        }

        const auto& template_args = decl->getTemplateArgs();
        assert(template_args.size() == 1);

        auto type = template_args[0].getAsType();
        HandleType(clang::SourceLocation {} /* TODO */, type.getTypePtr());
        for (const clang::CXXBaseSpecifier& base : decl->bases()) {
            auto annotation = base.getType().getAsString();
            if (annotation == "fexgen::opaque_to_guest" ||
                annotation == "fexgen::opaque_to_host") {
              types.at(type.getTypePtr()).is_opaque = true;
            } else {
                throw Error(base.getSourceRange().getBegin(), "Unknown type annotation");
            }
        }
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
    } catch (std::exception& exception) {
        auto error = Error(decl->getBeginLoc(), "Exception thrown while processing annotations: %0");
        error.AddSourceRange({decl->getSourceRange(),false});
        error.AddString(exception.what());
        error.Report(context.getDiagnostics());
    }

    /**
     * Called on instances of "template<> struct fex_gen_config<LibraryFunction> { ... }"
     * and "template<> struct fex_gen_config<&LibraryStruct::member> { ... }"
     */
    void HandleMiscAnnotations(clang::ClassTemplateSpecializationDecl* decl) try {
        assert (decl->getName() == "fex_gen_config");

        if (decl->getSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition) {
            throw Error(decl->getBeginLoc(), "fex_gen_config may not be partially specialized\n");
        }

        const auto& template_args = decl->getTemplateArgs();
        assert(template_args.size() == 1);

        if (auto member_decl = llvm::dyn_cast<clang::FieldDecl>(template_args[0].getAsDecl())) {
            // Struct member annotation
            auto parent_type = member_decl->getParent()->getTypeForDecl();
            HandleType(clang::SourceLocation {}, parent_type);
            auto& annotated_members = types.at(parent_type).members;
            HandleType(member_decl->getBeginLoc(), parent_type);
            for (const clang::CXXBaseSpecifier& base : decl->bases()) {
                auto annotation = base.getType().getAsString();
                if (annotation == "fexgen::ptr_pointer_passthrough") {
                    annotated_members[member_decl].pointer_type = PointerInfo::Type::Passthrough;
                } else if (annotation == "fexgen::ptr_is_untyped_address") {
                    annotated_members[member_decl].pointer_type = PointerInfo::Type::Untyped;
                } else if (annotation == "fexgen::ptr_todo_only64") {
                    annotated_members[member_decl].pointer_type = PointerInfo::Type::TODOONLY64;
                } else if (annotation == "fexgen::is_padding_member") {
                    annotated_members[member_decl].is_padding_member = true;
                } else {
                    throw Error(base.getSourceRange().getBegin(), "Unknown struct member annotation");
                }
            }
        } else {
            // Function annotations are processed at the end, and other annotations are ignored in this ABI pass
        }
    } catch (ClangDiagnosticAsException& exception) {
        exception.Report(context.getDiagnostics());
    } catch (std::exception& exception) {
        auto error = Error(decl->getBeginLoc(), "Exception thrown while processing annotations: %0");
        error.AddSourceRange({decl->getSourceRange(),false});
        error.AddString(exception.what());
        error.Report(context.getDiagnostics());
    }

private:
    void HandleType(clang::SourceLocation loc, const clang::Type* type) {
        // Strip any typedefs or "struct" prefixes from the type
        type = type->getUnqualifiedDesugaredType();

        if (types.contains(type)) {
            return;
        }

        if (auto* builtin_type = type->getAs<clang::BuiltinType>()) {
            clang::PrintingPolicy pol {{}};
//            types.insert(builtin_type);
            auto [it, inserted] = types.emplace(type, TypeData {});
            if (!inserted) {
                abort();
            }
        } else if (type->getAs<clang::FunctionProtoType>()) {
            // Nothing to do
        } else if (type->getAs<clang::TemplateSpecializationType>()) {
            // Not handled currently
        } else if (type->getAs<clang::TemplateTypeParmType>()) {
            // Not handled currently
        } else if (type->getAs<clang::DecltypeType>()) {
            // Not handled currently
        } else if (type->getAs<clang::ReferenceType>()) {
            // Not handled currently
        } else if (auto* array_type = llvm::dyn_cast<clang::ArrayType>(type)) {
            HandleType(loc, array_type->getElementType().getTypePtr());
        } else if (auto* pointer_type = type->getAs<clang::PointerType>()) {
            if (!pointer_type->isVoidPointerType()) {
                HandleType(loc, pointer_type->getPointeeType().getTypePtr());
            }
        } else if (auto* enum_type = type->getAs<clang::EnumType>()) {
            types.emplace(enum_type, TypeData {});
        } else if (auto* record_type = type->getAs<clang::RecordType>()) {
            // TODO: Handle anonymous unions

            types.emplace(record_type, TypeData {});
            // Member types will be added once all annotations have been parsed. This is necessary so we avoid recursion into opaque struct types
        } else {
            // TODO: Emit warning about unknown type
            std::cerr << "UNKNOWN TYPE: ";
            type->dump();
//            throw Error(loc, "Unknown type");
        }
    }
};

class AnalyzeABIConsumer : public clang::ASTConsumer {
    ABI& abi;

public:
    AnalyzeABIConsumer(ABI& output) : abi(output) {

    }

    void HandleTranslationUnit(clang::ASTContext& context) override {
        AnalyzeABIVisitor{context, abi}.TraverseDecl(context.getTranslationUnitDecl());
    }
};

std::unique_ptr<clang::ASTConsumer> AnalyzeABIAction::CreateASTConsumer(clang::CompilerInstance&, clang::StringRef) {
    return std::make_unique<AnalyzeABIConsumer>(abi);
}
