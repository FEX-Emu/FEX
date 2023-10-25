#include "analysis.h"
#include "data_layout.h"
#include "diagnostics.h"
#include "interface.h"
#include <clang/Frontend/CompilerInstance.h>

#include <fstream>
#include <numeric>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <openssl/sha.h>

class GenerateThunkLibsAction : public DataLayoutCompareAction {
public:
    GenerateThunkLibsAction(const std::string& libname, const OutputFilenames&, const ABI& abi);

private:
    // Generate helper code for thunk libraries and write them to the output file
    void OnAnalysisComplete(clang::ASTContext&) override;

    const std::string& libfilename;
    std::string libname; // sanitized filename, usable as part of emitted function names
    const OutputFilenames& output_filenames;
};

GenerateThunkLibsAction::GenerateThunkLibsAction(const std::string& libname_, const OutputFilenames& output_filenames_, const ABI& abi)
    : DataLayoutCompareAction(abi), libfilename(libname_), libname(libname_), output_filenames(output_filenames_) {
    for (auto& c : libname) {
        if (c == '-') {
            c = '_';
        }
    }
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

void GenerateThunkLibsAction::OnAnalysisComplete(clang::ASTContext& context) {
    ErrorReporter report_error { context };

    // Compute data layout differences between host and guest
    auto type_compat = [&]() {
        std::unordered_map<const clang::Type*, TypeCompatibility> ret;
        const auto host_abi = ComputeDataLayout(context, types);
        for (const auto& [type, type_repack_info] : types) {
            if (!type_repack_info.pointers_only) {
                GetTypeCompatibility(context, type, host_abi, ret);
            }
        }
        return ret;
    }();

    static auto format_decl = [](clang::QualType type, const std::string_view& name) {
        clang::QualType innermostPointee = type;
        while (innermostPointee->isPointerType()) {
          innermostPointee = innermostPointee->getPointeeType();
        }
        if (innermostPointee->isFunctionType()) {
            // Function pointer declarations (e.g. void (**callback)()) require
            // the variable name to be prefixed *and* suffixed.

            auto signature = type.getAsString();

            // Search for strings like (*), (**), or (*****). Insert the
            // variable name before the closing parenthesis
            auto needle = signature.begin();
            for (; needle != signature.end(); ++needle) {
                if (signature.end() - needle < 3 ||
                    std::string_view { &*needle, 2 } != "(*") {
                    continue;
                }
                while (*++needle == '*') {
                }
                if (*needle == ')') {
                    break;
                }
            }
            if (needle == signature.end()) {
                // It's *probably* a typedef, so this should be safe after all
                return fmt::format("{} {}", signature, name);
            } else {
                signature.insert(needle, name.begin(), name.end());
                return signature;
            }
        } else {
            return type.getAsString() + " " + std::string(name);
        }
    };

    auto format_function_params = [](const FunctionParams& params) {
        std::string ret;
        for (std::size_t idx = 0; idx < params.param_types.size(); ++idx) {
            auto& type = params.param_types[idx];
            ret += format_decl(type, fmt::format("a_{}", idx)) + ", ";
        }
        // drop trailing ", "
        ret.resize(ret.size() > 2 ? ret.size() - 2 : 0);
        return ret;
    };

    auto get_sha256 = [this](const std::string& function_name, bool include_libname) {
        std::string sha256_message = (include_libname ? libname + ":" : "") + function_name;
        std::vector<unsigned char> sha256(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const unsigned char*>(sha256_message.data()),
               sha256_message.size(),
               sha256.data());
        return sha256;
    };

    auto get_callback_name = [](std::string_view function_name, unsigned param_index) -> std::string {
        return fmt::format("{}CBFN{}", function_name, param_index);
    };

    // Files used guest-side
    if (!output_filenames.guest.empty()) {
        std::ofstream file(output_filenames.guest);

        // Guest->Host transition points for API functions
        file << "extern \"C\" {\n";
        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;
            auto sha256 = get_sha256(function_name, true);
            fmt::print( file, "MAKE_THUNK({}, {}, \"{:#02x}\")\n",
                        libname, function_name, fmt::join(sha256, ", "));
        }
        file << "}\n";

        // Guest->Host transition points for invoking runtime host-function pointers based on their signature
        std::vector<std::vector<unsigned char>> sha256s;
        for (auto type_it = thunked_funcptrs.begin(); type_it != thunked_funcptrs.end(); ++type_it) {
            auto* type = type_it->second.first;
            std::string funcptr_signature = clang::QualType { type, 0 }.getAsString();

            auto cb_sha256 = get_sha256("fexcallback_" + funcptr_signature, false);
            auto it = std::find(sha256s.begin(), sha256s.end(), cb_sha256);
            if (it != sha256s.end()) {
                // TODO: Avoid this ugly way of avoiding duplicates
                continue;
            } else {
                sha256s.push_back(cb_sha256);
            }

            // Thunk used for guest-side calls to host function pointers
            file << "  // " << funcptr_signature << "\n";
            auto funcptr_idx = std::distance(thunked_funcptrs.begin(), type_it);
            fmt::print( file, "  MAKE_CALLBACK_THUNK(callback_{}, {}, \"{:#02x}\");\n",
                        funcptr_idx, funcptr_signature, fmt::join(cb_sha256, ", "));
        }

        // Thunks-internal packing functions
        file << "extern \"C\" {\n";
        for (auto& data : thunks) {
            const auto& function_name = data.function_name;
            bool is_void = data.return_type->isVoidType();
            file << "FEX_PACKFN_LINKAGE auto fexfn_pack_" << function_name << "(";
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto& type = data.param_types[idx];
                file << (idx == 0 ? "" : ", ") << format_decl(type, fmt::format("a_{}", idx));
            }
            // Using trailing return type as it makes handling function pointer returns much easier
            file << ") -> " << data.return_type.getAsString() << " {\n";
            file << "  struct {\n";
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto& type = data.param_types[idx];
                file << "    " << format_decl(type.getUnqualifiedType(), fmt::format("a_{}", idx)) << ";\n";
            }
            if (!is_void) {
                file << "    " << format_decl(data.return_type, "rv") << ";\n";
            } else if (data.param_types.size() == 0) {
                // Avoid "empty struct has size 0 in C, size 1 in C++" warning
                file << "    char force_nonempty;\n";
            }
            file << "  } args;\n";

            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto cb = data.callbacks.find(idx);

                file << "  args.a_" << idx << " = ";
                if (cb == data.callbacks.end() || cb->second.is_stub || cb->second.is_guest) {
                    file << "a_" << idx << ";\n";
                } else {
                    // Before passing guest function pointers to the host, wrap them in a host-callable trampoline
                    fmt::print(file, "AllocateHostTrampolineForGuestFunction(a_{});\n", idx);
                }
            }
            file << "  fexthunks_" << libname << "_" << function_name << "(&args);\n";
            if (!is_void) {
                file << "  return args.rv;\n";
            }
            file << "}\n";
        }
        file << "}\n";

        // Publicly exports equivalent to symbols exported from the native guest library
        file << "extern \"C\" {\n";
        for (auto& data : thunked_api) {
            if (data.custom_guest_impl) {
                continue;
            }

            const auto& function_name = data.function_name;

            file << "__attribute__((alias(\"fexfn_pack_" << function_name << "\"))) auto " << function_name << "(";
            for (std::size_t idx = 0; idx < data.param_types.size(); ++idx) {
                auto& type = data.param_types[idx];
                file << (idx == 0 ? "" : ", ") << format_decl(type, "a_" + std::to_string(idx));
            }
            file << ") -> " << data.return_type.getAsString() << ";\n";
        }
        file << "}\n";

        // Symbol enumerators
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

    // Files used host-side
    if (!output_filenames.host.empty()) {
        std::ofstream file(output_filenames.host);

        // Forward declarations for symbols loaded from the native host library
        for (auto& import : thunked_api) {
            const auto& function_name = import.function_name;
            const char* variadic_ellipsis = import.is_variadic ? ", ..." : "";
            file << "using fexldr_type_" << libname << "_" << function_name << " = auto " << "(" << format_function_params(import) << variadic_ellipsis << ") -> " << import.return_type.getAsString() << ";\n";
            file << "static fexldr_type_" << libname << "_" << function_name << " *fexldr_ptr_" << libname << "_" << function_name << ";\n";
        }

        file << "extern \"C\" {\n";
        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;

            // Generate stub callbacks
            for (auto& [cb_idx, cb] : thunk.callbacks) {
                if (cb.is_stub) {
                    const char* variadic_ellipsis = cb.is_variadic ? ", ..." : "";
                    auto cb_function_name = "fexfn_unpack_" + get_callback_name(function_name, cb_idx) + "_stub";
                    file << "[[noreturn]] static " << cb.return_type.getAsString() << " "
                         << cb_function_name << "("
                         << format_function_params(cb) << variadic_ellipsis << ") {\n";
                    file << "  fprintf(stderr, \"FATAL: Attempted to invoke callback stub for " << function_name << "\\n\");\n";
                    file << "  std::abort();\n";
                    file << "}\n";
                }
            }

            // Forward declarations for user-provided implementations
            if (thunk.custom_host_impl) {
                file << "static auto fexfn_impl_" << libname << "_" << function_name << "(";
                for (std::size_t idx = 0; idx < thunk.param_types.size(); ++idx) {
                    // TODO: fex_guest_function_ptr for guest callbacks?
                    auto& type = thunk.param_types[idx];

                    file << (idx == 0 ? "" : ", ");

                    auto cb = thunk.callbacks.find(idx);
                    if (cb != thunk.callbacks.end() && cb->second.is_guest) {
                        file << "fex_guest_function_ptr a_" << idx;
                    } else if (thunk.param_annotations[idx].is_passthrough) {
                        fmt::print(file, "guest_layout<{}> a_{}", type.getAsString(), idx);
                    } else {
                        file << format_decl(type, fmt::format("a_{}", idx));
                    }
                }
                // Using trailing return type as it makes handling function pointer returns much easier
                file << ") -> " << thunk.return_type.getAsString() << ";\n";
            }

            // Check data layout compatibility of parameter types
            // TODO: Also check non-struct/non-pointer types
            // TODO: Also check return type
            for (size_t param_idx = 0; param_idx != thunk.param_types.size(); ++param_idx) {
                const auto& param_type = thunk.param_types[param_idx];
                if (!param_type->isPointerType() || !param_type->getPointeeType()->isStructureType()) {
                    continue;
                }
                auto type = param_type->getPointeeType();
                if (!types.at(context.getCanonicalType(type.getTypePtr())).assumed_compatible && type_compat.at(context.getCanonicalType(type.getTypePtr())) == TypeCompatibility::None) {
                    // TODO: Factor in "assume_compatible_layout" annotations here
                    //       That annotation should cause the type to be treated as TypeCompatibility::Full
                    if (!thunk.param_annotations[param_idx].is_passthrough) {
                        throw report_error(thunk.decl->getLocation(), "Unsupported parameter type %0").AddTaggedVal(param_type);
                    }
                }
            }

            // Packed argument structs used in fexfn_unpack_*
            auto GeneratePackedArgs = [&](const auto &function_name, const ThunkedFunction &thunk) -> std::string {
                std::string struct_name = "fexfn_packed_args_" + libname + "_" + function_name;
                file << "struct " << struct_name << " {\n";

                for (std::size_t idx = 0; idx < thunk.param_types.size(); ++idx) {
                    fmt::print(file, "  guest_layout<{}> a_{};\n", get_type_name(context, thunk.param_types[idx].getTypePtr()), idx);
                }
                if (!thunk.return_type->isVoidType()) {
                    file << "  " << format_decl(thunk.return_type, "rv") << ";\n";
                } else if (thunk.param_types.size() == 0) {
                    // Avoid "empty struct has size 0 in C, size 1 in C++" warning
                    file << "    char force_nonempty;\n";
                }
                file << "};\n";
                return struct_name;
            };
            auto struct_name = GeneratePackedArgs(function_name, thunk);

            // Unpacking functions
            auto function_to_call = "fexldr_ptr_" + libname + "_" + function_name;
            if (thunk.custom_host_impl) {
                function_to_call = "fexfn_impl_" + libname + "_" + function_name;
            }

            file << "static void fexfn_unpack_" << libname << "_" << function_name << "(" << struct_name << "* args) {\n";
            file << (thunk.return_type->isVoidType() ? "  " : "  args->rv = ") << function_to_call << "(";

            for (unsigned param_idx = 0; param_idx != thunk.param_types.size(); ++param_idx) {
                if (thunk.callbacks.contains(param_idx) && thunk.callbacks.at(param_idx).is_stub) {
                    continue;
                }

                auto& param_type = thunk.param_types[param_idx];
                const bool is_assumed_compatible = param_type->isPointerType() &&
                          (thunk.param_annotations[param_idx].assume_compatible || ((param_type->getPointeeType()->isStructureType() || (param_type->getPointeeType()->isPointerType() && param_type->getPointeeType()->getPointeeType()->isStructureType())) &&
                          (types.contains(context.getCanonicalType(param_type->getPointeeType()->getLocallyUnqualifiedSingleStepDesugaredType().getTypePtr())) && LookupType(context, context.getCanonicalType(param_type->getPointeeType()->getLocallyUnqualifiedSingleStepDesugaredType().getTypePtr())).assumed_compatible)));

                std::optional<TypeCompatibility> pointee_compat;
                if (param_type->isPointerType()) {
                    // Get TypeCompatibility from existing entry, or register TypeCompatibility::None if no entry exists
                    // TODO: Currently needs TypeCompatibility::Full workaround...
                    pointee_compat = type_compat.emplace(context.getCanonicalType(param_type->getPointeeType().getTypePtr()), TypeCompatibility::Full).first->second;
                }

                if (thunk.param_annotations[param_idx].is_passthrough) {
                    // args are passed directly to function, no need to use `unpacked` wrappers
                    continue;
                }

                if (!param_type->isPointerType() || (is_assumed_compatible || pointee_compat == TypeCompatibility::Full) ||
                    param_type->getPointeeType()->isBuiltinType() /* TODO: handle size_t. Actually, properly check for data layout compatibility */) {
                    // Fully compatible
                } else if (pointee_compat == TypeCompatibility::Repackable) {
                    throw report_error(thunk.decl->getLocation(), "Pointer parameter %1 of function %0 requires automatic repacking, which is not implemented yet").AddString(function_name).AddTaggedVal(param_type);
                } else {
                    throw report_error(thunk.decl->getLocation(), "Cannot generate unpacking function for function %0 with unannotated pointer parameter %1").AddString(function_name).AddTaggedVal(param_type);
                }
            }

            {
                auto format_param = [&](std::size_t idx) {
                    std::string raw_arg = fmt::format("args->a_{}.data", idx);

                    auto cb = thunk.callbacks.find(idx);
                    if (cb != thunk.callbacks.end() && cb->second.is_stub) {
                        return "fexfn_unpack_" + get_callback_name(function_name, cb->first) + "_stub";
                    } else if (cb != thunk.callbacks.end() && cb->second.is_guest) {
                        return fmt::format("fex_guest_function_ptr {{ {} }}", raw_arg);
                    } else if (cb != thunk.callbacks.end()) {
                        auto arg_name = fmt::format("args->a_{}.data", idx);
                        // Use comma operator to inject a function call before returning the argument
                        return "(FinalizeHostTrampolineForGuestFunction(" + arg_name + "), " + arg_name + ")";
                    } else if (thunk.param_annotations[idx].is_passthrough) {
                        // Pass raw guest_layout<T*>
                        return fmt::format("args->a_{}", idx);
                    } else {
                        return raw_arg;
                    }
                };

                file << format_function_args(thunk, format_param);
            }
            file << ");\n";
            file << "}\n";
        }
        file << "}\n";

        // Endpoints for Guest->Host invocation of API functions
        file << "static ExportEntry exports[] = {\n";
        for (auto& thunk : thunks) {
            const auto& function_name = thunk.function_name;
            auto sha256 = get_sha256(function_name, true);
            fmt::print( file, "  {{(uint8_t*)\"\\x{:02x}\", (void(*)(void *))&fexfn_unpack_{}_{}}}, // {}:{}\n",
                        fmt::join(sha256, "\\x"), libname, function_name, libname, function_name);
        }

        // Endpoints for Guest->Host invocation of runtime host-function pointers
        for (auto& host_funcptr_entry : thunked_funcptrs) {
            auto& [type, param_annotations] = host_funcptr_entry.second;
            std::string mangled_name = clang::QualType { type, 0 }.getAsString();
            auto info = LookupGuestFuncPtrInfo(host_funcptr_entry.first.c_str());

            std::string annotations;
            for (int param_idx = 0; param_idx < info.args.size(); ++param_idx) {
                if (param_idx != 0) {
                    annotations += ", ";
                }

                annotations += "ParameterAnnotations {";
                if (param_annotations.contains(param_idx) && param_annotations.at(param_idx).is_passthrough) {
                    annotations += ".is_passthrough=true,";
                }
                if (param_annotations.contains(param_idx) && param_annotations.at(param_idx).assume_compatible) {
                    annotations += ".assume_compatible=true,";
                }
                annotations += "}";
            }
            fmt::print( file, "  {{(uint8_t*)\"\\x{:02x}\", (void(*)(void *))&GuestWrapperForHostFunction<{}({})>::Call<{}>}}, // {}\n",
                        fmt::join(info.sha256, "\\x"), info.result, fmt::join(info.args, ", "), annotations, host_funcptr_entry.first);
        }

        file << "  { nullptr, nullptr }\n";
        file << "};\n";

        // Symbol lookup from native host library
        file << "static void* fexldr_ptr_" << libname << "_so;\n";
        file << "extern \"C\" bool fexldr_init_" << libname << "() {\n";

        std::string version_suffix;
        if (lib_version) {
          version_suffix = '.' + std::to_string(*lib_version);
        }
        const std::string library_filename = libfilename + ".so" + version_suffix;

        // Load the host library in the global symbol namespace.
        // This follows how these libraries get loaded in a non-emulated environment,
        // Either by directly linking to the library or a loader (In OpenGL or Vulkan) putting everything in the global namespace.
        file << "  fexldr_ptr_" << libname << "_so = dlopen(\"" << library_filename << "\", RTLD_GLOBAL | RTLD_LAZY);\n";

        file << "  if (!fexldr_ptr_" << libname << "_so) { return false; }\n\n";
        for (auto& import : thunked_api) {
            fmt::print( file, "  (void*&)fexldr_ptr_{}_{} = {}(fexldr_ptr_{}_so, \"{}\");\n",
                        libname, import.function_name, import.host_loader, libname, import.function_name);
        }
        file << "  return true;\n";
        file << "}\n";
    }
}

bool GenerateThunkLibsActionFactory::runInvocation(
    std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files,
    std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
    clang::DiagnosticConsumer *DiagConsumer) {
  clang::CompilerInstance Compiler(std::move(PCHContainerOps));
  Compiler.setInvocation(std::move(Invocation));
  Compiler.setFileManager(Files);

  GenerateThunkLibsAction Action(libname, output_filenames, abi);

  Compiler.createDiagnostics(DiagConsumer, false);
  if (!Compiler.hasDiagnostics())
    return false;

  Compiler.createSourceManager(*Files);

  const bool Success = Compiler.ExecuteAction(Action);

  Files->clearStatCache();
  return Success;
}
