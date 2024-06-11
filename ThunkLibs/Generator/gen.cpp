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

  // Emit guest_layout/host_layout wrappers for types passed across architecture boundaries
  void EmitLayoutWrappers(clang::ASTContext&, std::ofstream&, std::unordered_map<const clang::Type*, TypeCompatibility>& type_compat);

  const std::string& libfilename;
  std::string libname; // sanitized filename, usable as part of emitted function names
  const OutputFilenames& output_filenames;
};

GenerateThunkLibsAction::GenerateThunkLibsAction(const std::string& libname_, const OutputFilenames& output_filenames_, const ABI& abi)
  : DataLayoutCompareAction(abi)
  , libfilename(libname_)
  , libname(libname_)
  , output_filenames(output_filenames_) {
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
void BubbleSort(It begin, It end, std::relation<std::iter_value_t<It>, std::iter_value_t<It>> auto compare) {
  bool fixpoint;
  do {
    fixpoint = true;
    for (auto it = begin; it != end; ++it) {
      for (auto it2 = std::next(it); it2 != end; ++it2) {
        if (compare(*it2, *it)) {
          std::swap(*it, *it2);
          fixpoint = false;
          it2 = it;
        }
      }
    }
  } while (!fixpoint);
}

// Compares such that A < B if B contains A as a member and requires A to be completely defined (i.e. non-pointer/non-reference).
// This applies recursively to structs contained by B.
struct compare_by_struct_dependency {
  clang::ASTContext& context;

  bool operator()(const std::pair<const clang::Type*, GenerateThunkLibsAction::RepackedType>& a,
                  const std::pair<const clang::Type*, GenerateThunkLibsAction::RepackedType>& b) const {
    return (*this)(a.first, b.first);
  }

  bool operator()(const clang::Type* a, const clang::Type* b) const {
    if (llvm::isa<clang::ConstantArrayType>(b)) {
      throw std::runtime_error("Cannot have \"b\" be an array");
    }

    auto* b_as_struct = b->getAsStructureType();
    if (!b_as_struct) {
      // Not a struct => no dependency
      return false;
    }

    if (a->isArrayType()) {
      throw std::runtime_error("Cannot have \"a\" be an array");
    }

    for (auto* child : b_as_struct->getDecl()->fields()) {
      auto child_type = child->getType().getTypePtr();

      if (child_type->isPointerType()) {
        // Pointers don't need the definition to be available
        continue;
      }

      // Peel off any array type layers from the member
      while (auto child_as_array = llvm::dyn_cast<clang::ConstantArrayType>(child_type)) {
        child_type = child_as_array->getArrayElementTypeNoTypeQual();
      }

      if (context.hasSameType(a, child_type)) {
        return true;
      }

      if ((*this)(a, child_type)) {
        // Child depends on A => transitive dependency
        return true;
      }
    }

    // No dependency found
    return false;
  }
};

void GenerateThunkLibsAction::EmitLayoutWrappers(clang::ASTContext& context, std::ofstream& file,
                                                 std::unordered_map<const clang::Type*, TypeCompatibility>& type_compat) {
  // Sort struct types by dependency so that repacking code is emitted in an order that compiles fine
  std::vector<std::pair<const clang::Type*, RepackedType>> types {this->types.begin(), this->types.end()};
  BubbleSort(types.begin(), types.end(), compare_by_struct_dependency {context});

  for (const auto& [type, type_repack_info] : types) {
    auto struct_name = get_type_name(context, type);

    // Opaque types don't need layout definitions
    if (type_repack_info.assumed_compatible && type_repack_info.pointers_only && struct_name != "void") {
      if (guest_abi.pointer_size != 4) {
        fmt::print(file, "template<> inline constexpr bool has_compatible_data_layout<{}*> = true;\n", struct_name);
      }
      continue;
    } else if (type_repack_info.assumed_compatible) {
      // TODO: Handle more cleanly
      type_compat[type] = TypeCompatibility::Full;
    }

    // These must be handled later since they are not canonicalized and hence must be de-duplicated first
    if (type->isBuiltinType()) {
      continue;
    }

    // TODO: Instead, map these names back to *some* type that's named?
    if (struct_name.starts_with("unnamed_")) {
      continue;
    }

    if (type->isEnumeralType()) {
      fmt::print(file, "template<>\nstruct __attribute__((packed)) guest_layout<{}> {{\n", struct_name);
      fmt::print(file, "  using type = {}int{}_t;\n", type->isUnsignedIntegerOrEnumerationType() ? "u" : "",
                 guest_abi.at(struct_name).get_if_simple_or_struct()->size_bits);
      fmt::print(file, "  type data;\n");
      fmt::print(file, "}};\n");
      continue;
    }

    if (type_compat.at(type) == TypeCompatibility::None && !type_repack_info.emit_layout_wrappers) {
      // Disallow use of layout wrappers for this type by specializing without a definition
      fmt::print(file, "template<>\nstruct guest_layout<{}>;\n", struct_name);
      fmt::print(file, "template<>\nstruct host_layout<{}>;\n", struct_name);
      fmt::print(file, "guest_layout<{}>& to_guest(const host_layout<{}>&) = delete;\n", struct_name, struct_name);
      continue;
    }

    // Guest layout definition
    // NOTE: uint64_t has lower alignment requirements on 32-bit than on 64-bit, so we require tightly packed structs
    // TODO: Now we must emit padding bytes explicitly, though!
    fmt::print(file, "template<>\nstruct __attribute__((packed)) guest_layout<{}> {{\n", struct_name);
    if (type_compat.at(type) == TypeCompatibility::Full) {
      fmt::print(file, "  using type = {};\n", struct_name);
    } else {
      fmt::print(file, "  struct type {{\n");
      for (auto& member : guest_abi.at(struct_name).get_if_struct()->members) {
        fmt::print(file, "    guest_layout<{}{}> {};\n", member.type_name,
                   member.array_size ? fmt::format("[{}]", member.array_size.value()) : "", member.member_name);
      }
      fmt::print(file, "  }};\n");
    }
    fmt::print(file, "  type data;\n");
    fmt::print(file, "}};\n");

    fmt::print(file, "template<>\nstruct guest_layout<const {}> : guest_layout<{}> {{\n", struct_name, struct_name);
    fmt::print(file, "  guest_layout& operator=(const guest_layout<{}>& other) {{ memcpy(this, &other, sizeof(other)); return *this; }}\n",
               struct_name);
    fmt::print(file, "}};\n");

    // Host layout definition
    fmt::print(file, "template<>\n");
    fmt::print(file, "struct host_layout<{}> {{\n", struct_name);
    fmt::print(file, "  using type = {};\n", struct_name);
    fmt::print(file, "  type data;\n");
    fmt::print(file, "\n");
    // Host->guest layout conversion
    fmt::print(file, "  host_layout(const guest_layout<{}>& from) :\n", struct_name);
    if (type_compat.at(type) == TypeCompatibility::Full) {
      fmt::print(file, "    data {{ from.data }} {{\n");
    } else {
      // Conversion needs struct repacking.
      // Wrapping each member in `host_layout<>` ensures this is done recursively.
      fmt::print(file, "    data {{\n");
      auto map_field = [&file](clang::FieldDecl* member, bool skip_arrays) {
        auto decl_name = member->getNameAsString();
        auto type_name = member->getType().getAsString();
        auto array_type = llvm::dyn_cast<clang::ConstantArrayType>(member->getType());
        if (!array_type && skip_arrays) {
          if (member->getType()->isFunctionPointerType()) {
            // Function pointers must be handled manually, so zero them out by default
            fmt::print(file, "      .{} {{ }},\n", decl_name);
          } else {
            fmt::print(file, "      .{} = host_layout<{}> {{ from.data.{} }}.data,\n", decl_name, type_name, decl_name);
          }
        } else if (array_type && !skip_arrays) {
          // Copy element-wise below
          fmt::print(file, "      for (size_t i = 0; i < {}; ++i) {{\n", array_type->getSize().getZExtValue());
          fmt::print(file, "        data.{}[i] = host_layout<{}> {{ from.data.{} }}.data[i];\n", decl_name, type_name, decl_name);
          fmt::print(file, "      }}\n");
        }
      };
      // Prefer initialization via the constructor's initializer list if possible (to detect unintended narrowing), otherwise initialize in the body
      for (auto* member : type->getAsStructureType()->getDecl()->fields()) {
        if (!type_repack_info.UsesCustomRepackFor(member)) {
          map_field(member, true);
        } else {
          // Leave field uninitialized
        }
      }
      fmt::print(file, "    }} {{\n");
      for (auto* member : type->getAsStructureType()->getDecl()->fields()) {
        if (!type_repack_info.UsesCustomRepackFor(member)) {
          map_field(member, false);
        } else {
          // Leave field uninitialized
        }
      }
    }
    fmt::print(file, "  }}\n");
    fmt::print(file, "}};\n\n");

    // Guest->host layout conversion
    fmt::print(file, "inline guest_layout<{}> to_guest(const host_layout<{}>& from) {{\n", struct_name, struct_name);
    if (type_compat.at(type) == TypeCompatibility::Full) {
      fmt::print(file, "  guest_layout<{}> ret;\n", struct_name);
      fmt::print(file, "  static_assert(sizeof(from) == sizeof(ret));\n");
      fmt::print(file, "  memcpy(&ret, &from, sizeof(from));\n");
    } else {
      // Conversion needs struct repacking.
      // Wrapping each member in `to_guest(to_host_layout(...))` ensures this is done recursively.
      fmt::print(file, "  guest_layout<{}> ret {{ .data {{\n", struct_name);
      auto map_field2 = [&file](const StructInfo::MemberInfo& member, bool skip_arrays) {
        auto& decl_name = member.member_name;
        auto& array_size = member.array_size;
        if (!array_size && skip_arrays) {
          if (member.is_function_pointer) {
            // Function pointers must be handled manually, so zero them out by default
            fmt::print(file, "    .{} {{ }},\n", decl_name);
          } else {
            fmt::print(file, "    .{} = to_guest(to_host_layout(from.data.{})),\n", decl_name, decl_name);
          }
        } else if (array_size && !skip_arrays) {
          // Copy element-wise below
          fmt::print(file, "    for (size_t i = 0; i < {}; ++i) {{\n", array_size.value());
          fmt::print(file, "      ret.data.{}.data[i] = to_guest(to_host_layout(from.data.{}[i]));\n", decl_name, decl_name);
          fmt::print(file, "    }}\n");
        }
      };

      // Prefer initialization via the constructor's initializer list if possible (to detect unintended narrowing), otherwise initialize in the body
      for (auto& member : guest_abi.at(struct_name).get_if_struct()->members) {
        if (!type_repack_info.UsesCustomRepackFor(member.member_name)) {
          map_field2(member, true);
        } else {
          // Leave field uninitialized
        }
      }
      fmt::print(file, "  }} }};\n");
      for (auto& member : guest_abi.at(struct_name).get_if_struct()->members) {
        if (!type_repack_info.UsesCustomRepackFor(member.member_name)) {
          map_field2(member, false);
        } else {
          // Leave field uninitialized
        }
      }
    }
    fmt::print(file, "  return ret;\n");
    fmt::print(file, "}}\n\n");

    // Forward-declare user-provided repacking functions
    if (type_repack_info.custom_repacked_members.empty()) {
      fmt::print(file, "void fex_apply_custom_repacking_entry(host_layout<{}>& source, const guest_layout<{}>& from) {{\n", struct_name, struct_name);
      fmt::print(file, "}}\n");
      fmt::print(file, "bool fex_apply_custom_repacking_exit(guest_layout<{}>& into, host_layout<{}>& from) {{\n", struct_name, struct_name);
      fmt::print(file, "  return false;\n");
      fmt::print(file, "}}\n");
    } else {
      fmt::print(file, "void fex_custom_repack_entry(host_layout<{}>& into, const guest_layout<{}>& from);\n", struct_name, struct_name);
      fmt::print(file, "bool fex_custom_repack_exit(guest_layout<{}>& into, const host_layout<{}>& from);\n\n", struct_name, struct_name);

      fmt::print(file, "void fex_apply_custom_repacking_entry(host_layout<{}>& source, const guest_layout<{}>& from) {{\n", struct_name, struct_name);
      fmt::print(file, "  fex_custom_repack_entry(source, from);\n");
      fmt::print(file, "}}\n");

      fmt::print(file, "bool fex_apply_custom_repacking_exit(guest_layout<{}>& into, host_layout<{}>& from) {{\n", struct_name, struct_name);
      fmt::print(file, "  return fex_custom_repack_exit(into, from);\n");
      fmt::print(file, "}}\n");
    }

    fmt::print(file, "template<> inline constexpr bool has_compatible_data_layout<{}> = {};\n", struct_name,
               (type_compat.at(type) == TypeCompatibility::Full));
  }
}

void GenerateThunkLibsAction::OnAnalysisComplete(clang::ASTContext& context) {
  ErrorReporter report_error {context};

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
        if (signature.end() - needle < 3 || std::string_view {&*needle, 2} != "(*") {
          continue;
        }
        while (*++needle == '*') {}
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
    SHA256(reinterpret_cast<const unsigned char*>(sha256_message.data()), sha256_message.size(), sha256.data());
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
      fmt::print(file, "MAKE_THUNK({}, {}, \"{:#02x}\")\n", libname, function_name, fmt::join(sha256, ", "));
    }
    file << "}\n";

    // Guest->Host transition points for invoking runtime host-function pointers based on their signature
    std::vector<std::vector<unsigned char>> sha256s;
    for (auto type_it = thunked_funcptrs.begin(); type_it != thunked_funcptrs.end(); ++type_it) {
      auto* type = type_it->second.first;
      std::string funcptr_signature = clang::QualType {type, 0}.getAsString();

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
      fmt::print(file, "  MAKE_CALLBACK_THUNK(callback_{}, {}, \"{:#02x}\");\n", funcptr_idx, funcptr_signature, fmt::join(cb_sha256, ", "));
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
      file << "  struct __attribute__((packed)) {\n";
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
        if (cb == data.callbacks.end() || cb->second.is_stub) {
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

    EmitLayoutWrappers(context, file, type_compat);

    // Forward declarations for symbols loaded from the native host library
    for (auto& import : thunked_api) {
      const auto& function_name = import.function_name;
      const char* variadic_ellipsis = import.is_variadic ? ", ..." : "";
      file << "using fexldr_type_" << libname << "_" << function_name << " = auto "
           << "(" << format_function_params(import) << variadic_ellipsis << ") -> " << import.return_type.getAsString() << ";\n";
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
          file << "[[noreturn]] static " << cb.return_type.getAsString() << " " << cb_function_name << "(" << format_function_params(cb)
               << variadic_ellipsis << ") {\n";
          file << "  fprintf(stderr, \"FATAL: Attempted to invoke callback stub for " << function_name << "\\n\");\n";
          file << "  std::abort();\n";
          file << "}\n";
        }
      }

      auto get_guest_type_name = [this](clang::QualType type) {
        if (type->isBuiltinType() && type->isIntegerType()) {
          auto size = guest_abi.at(type.getUnqualifiedType().getAsString()).get_if_simple_or_struct()->size_bits;
          return get_fixed_size_int_name(type.getTypePtr(), size);
        } else if (type->isPointerType() && type->getPointeeType()->isBuiltinType() && type->getPointeeType()->isIntegerType() &&
                   !type->getPointeeType()->isVoidType()) {
          auto size = guest_abi.at(type->getPointeeType().getUnqualifiedType().getAsString()).get_if_simple_or_struct()->size_bits;
          return fmt::format("{}{}*", type->getPointeeType().isConstQualified() ? "const " : "",
                             get_fixed_size_int_name(type->getPointeeType().getTypePtr(), size));
        } else {
          return type.getUnqualifiedType().getAsString();
        }
      };

      // Forward declarations for user-provided implementations
      if (thunk.custom_host_impl) {
        file << "static auto fexfn_impl_" << libname << "_" << function_name << "(";
        for (std::size_t idx = 0; idx < thunk.param_types.size(); ++idx) {
          auto& type = thunk.param_types[idx];

          file << (idx == 0 ? "" : ", ");

          if (thunk.param_annotations[idx].is_passthrough) {
            fmt::print(file, "guest_layout<{}> a_{}", get_guest_type_name(type), idx);
          } else {
            fmt::print(file, "{}", format_decl(type, fmt::format("a_{}", idx)));
          }
        }
        // Using trailing return type as it makes handling function pointer returns much easier
        bool is_passthrough_ret = thunk.param_annotations[-1].is_passthrough;
        fmt::print(file, ") -> {}{}{};\n", is_passthrough_ret ? "guest_layout<" : "", thunk.return_type.getAsString(),
                   is_passthrough_ret ? ">" : "");
      }

      // Check data layout compatibility of parameter types
      // TODO: Also check non-struct/non-pointer types
      // TODO: Also check return type
      for (size_t param_idx = 0; param_idx != thunk.param_types.size(); ++param_idx) {
        const auto& param_type = thunk.param_types[param_idx];
        if (!param_type->isPointerType() || !param_type->getPointeeType()->isStructureType()) {
          continue;
        }
        if (!thunk.param_annotations[param_idx].is_passthrough) {
          auto type = param_type->getPointeeType();
          if (!types.at(context.getCanonicalType(type.getTypePtr())).assumed_compatible &&
              type_compat.at(context.getCanonicalType(type.getTypePtr())) == TypeCompatibility::None) {
            // TODO: Factor in "assume_compatible_layout" annotations here
            //       That annotation should cause the type to be treated as TypeCompatibility::Full
            throw report_error(thunk.decl->getLocation(), "Unsupported parameter type %0").AddTaggedVal(param_type);
          }
        }
      }

      // Packed argument structs used in fexfn_unpack_*
      auto GeneratePackedArgs = [&](const auto& function_name, const ThunkedFunction& thunk) -> std::string {
        std::string struct_name = "fexfn_packed_args_" + libname + "_" + function_name;
        file << "struct __attribute__((packed)) " << struct_name << " {\n";

        for (std::size_t idx = 0; idx < thunk.param_types.size(); ++idx) {
          fmt::print(file, "  guest_layout<{}> a_{};\n", get_guest_type_name(thunk.param_types[idx]), idx);
        }
        if (!thunk.return_type->isVoidType()) {
          fmt::print(file, "  guest_layout<{}> rv;\n", get_guest_type_name(thunk.return_type));
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

      auto get_type_name_with_nonconst_pointee = [&](clang::QualType type) {
        type = type.getLocalUnqualifiedType();
        if (type->isPointerType()) {
          // Strip away "const" from pointee type
          type = context.getPointerType(type->getPointeeType().getLocalUnqualifiedType());
        }
        return get_type_name(context, type.getTypePtr());
      };


      file << "static void fexfn_unpack_" << libname << "_" << function_name << "(" << struct_name << "* args) {\n";

      for (unsigned param_idx = 0; param_idx != thunk.param_types.size(); ++param_idx) {
        if (thunk.callbacks.contains(param_idx) && thunk.callbacks.at(param_idx).is_stub) {
          continue;
        }

        auto& param_type = thunk.param_types[param_idx];
        const bool is_assumed_compatible =
          param_type->isPointerType() &&
          (thunk.param_annotations[param_idx].assume_compatible ||
           ((param_type->getPointeeType()->isStructureType() ||
             (param_type->getPointeeType()->isPointerType() && param_type->getPointeeType()->getPointeeType()->isStructureType())) &&
            (types.contains(context.getCanonicalType(param_type->getPointeeType()->getLocallyUnqualifiedSingleStepDesugaredType().getTypePtr())) &&
             LookupType(context, context.getCanonicalType(param_type->getPointeeType()->getLocallyUnqualifiedSingleStepDesugaredType().getTypePtr()))
               .assumed_compatible)));

        std::optional<TypeCompatibility> pointee_compat;
        if (param_type->isPointerType()) {
          // Get TypeCompatibility from existing entry, or register TypeCompatibility::None if no entry exists
          // TODO: Currently needs TypeCompatibility::Full workaround...
          pointee_compat =
            type_compat.emplace(context.getCanonicalType(param_type->getPointeeType().getTypePtr()), TypeCompatibility::Full).first->second;
        }

        if (thunk.param_annotations[param_idx].is_passthrough) {
          // args are passed directly to function, no need to use `unpacked` wrappers
          continue;
        }

        // Layout repacking happens here
        if (!param_type->isPointerType() || (is_assumed_compatible || pointee_compat == TypeCompatibility::Full) ||
            param_type->getPointeeType()->isBuiltinType() /* TODO: handle size_t. Actually, properly check for data layout compatibility */) {
          // Fully compatible
          fmt::print(file, "  host_layout<{}> a_{} {{ args->a_{} }};\n", get_type_name(context, param_type.getTypePtr()), param_idx, param_idx);
        } else if (pointee_compat == TypeCompatibility::Repackable) {
          // TODO: Require opt-in for this to be emitted since it's single-element only; otherwise, pointers-to-arrays arguments will cause stack trampling
          fmt::print(file, "  auto a_{} = make_repack_wrapper<{}>(args->a_{});\n", param_idx,
                     get_type_name_with_nonconst_pointee(param_type), param_idx);
        } else {
          throw report_error(thunk.decl->getLocation(), "Cannot generate unpacking function for function %0 with unannotated pointer "
                                                        "parameter %1")
            .AddString(function_name)
            .AddTaggedVal(param_type);
        }
      }

      if (!thunk.return_type->isVoidType()) {
        fmt::print(file, "  args->rv = ");
        if (!thunk.return_type->isFunctionPointerType() && !thunk.param_annotations[-1].is_passthrough) {
          fmt::print(file, "to_guest(to_host_layout<{}>(", thunk.return_type.getAsString());
        }
      }
      fmt::print(file, "{}(", function_to_call);
      {
        auto format_param = [&](std::size_t idx) {
          auto cb = thunk.callbacks.find(idx);
          if (cb != thunk.callbacks.end() && cb->second.is_stub) {
            return "fexfn_unpack_" + get_callback_name(function_name, cb->first) + "_stub";
          } else if (cb != thunk.callbacks.end()) {
            auto arg_name = fmt::format("args->a_{}", idx); // Use parameter directly
            // Use comma operator to inject a function call before returning the argument
            // TODO: Avoid casting away the guest_layout
            if (thunk.custom_host_impl) {
              return fmt::format("(FinalizeHostTrampolineForGuestFunction({}), {})", arg_name, arg_name);
            } else {
              return fmt::format("(FinalizeHostTrampolineForGuestFunction({}), ({})(uint64_t {{ {}.data }}))", arg_name,
                                 get_type_name(context, thunk.param_types[idx].getTypePtr()), arg_name);
            }
          } else if (thunk.param_annotations[idx].is_passthrough) {
            // Pass raw guest_layout<T*>
            return fmt::format("args->a_{}", idx);
          } else {
            // Unwrap host_layout/repack_wrapper layer
            return fmt::format("unwrap_host(a_{})", idx);
          }
        };

        fmt::print(file, "{}", format_function_args(thunk, format_param));
      }
      if (!thunk.return_type->isVoidType() && !thunk.return_type->isFunctionPointerType() && !thunk.param_annotations[-1].is_passthrough) {
        fmt::print(file, "))");
      }
      fmt::print(file, ");\n");

      file << "}\n";
    }
    file << "}\n";

    // Endpoints for Guest->Host invocation of API functions
    file << "static ExportEntry exports[] = {\n";
    for (auto& thunk : thunks) {
      const auto& function_name = thunk.function_name;
      auto sha256 = get_sha256(function_name, true);
      fmt::print(file, "  {{(uint8_t*)\"\\x{:02x}\", (void(*)(void *))&fexfn_unpack_{}_{}}}, // {}:{}\n", fmt::join(sha256, "\\x"), libname,
                 function_name, libname, function_name);
    }

    // Endpoints for Guest->Host invocation of runtime host-function pointers
    // NOTE: The function parameters may differ slightly between guest and host,
    //       e.g. due to differing sizes or due to data layout differences.
    //       Hence, two separate parameter lists are managed here.
    for (auto& host_funcptr_entry : thunked_funcptrs) {
      auto& [type, param_annotations] = host_funcptr_entry.second;
      auto func_type = type->getAs<clang::FunctionProtoType>();
      FuncPtrInfo info = {};

      // TODO: Use GetTypeNameWithFixedSizeIntegers
      info.result = func_type->getReturnType().getAsString();

      // NOTE: In guest contexts, integer types must be mapped to
      //       fixed-size equivalents. Since this is a host context, this
      //       isn't strictly necessary here, but it makes matching up
      //       guest_layout/host_layout constructors easier.
      for (auto arg : func_type->getParamTypes()) {
        info.args.push_back(GetTypeNameWithFixedSizeIntegers(context, arg));
      }

      std::string annotations;
      for (int param_idx = -1; param_idx < (int)info.args.size(); ++param_idx) {
        if (param_idx != -1) {
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
      auto guest_info = LookupGuestFuncPtrInfo(host_funcptr_entry.first.c_str());
      // TODO: Consider differences in guest/host return types
      fmt::print(file, "  {{(uint8_t*)\"\\x{:02x}\", (void(*)(void *))&GuestWrapperForHostFunction<{}({}){}{}>::Call<{}>}}, // {}\n",
                 fmt::join(guest_info.sha256, "\\x"), guest_info.result, fmt::join(info.args, ", "), guest_info.args.empty() ? "" : ", ",
                 fmt::join(guest_info.args, ", "), annotations, host_funcptr_entry.first);
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
      fmt::print(file, "  (void*&)fexldr_ptr_{}_{} = {}(fexldr_ptr_{}_so, \"{}\");\n", libname, import.function_name, import.host_loader,
                 libname, import.function_name);
    }
    file << "  return true;\n";
    file << "}\n";
  }
}

bool GenerateThunkLibsActionFactory::runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager* Files,
                                                   std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
                                                   clang::DiagnosticConsumer* DiagConsumer) {
  clang::CompilerInstance Compiler(std::move(PCHContainerOps));
  Compiler.setInvocation(std::move(Invocation));
  Compiler.setFileManager(Files);

  GenerateThunkLibsAction Action(libname, output_filenames, abi);

  Compiler.createDiagnostics(DiagConsumer, false);
  if (!Compiler.hasDiagnostics()) {
    return false;
  }

  Compiler.createSourceManager(*Files);

  const bool Success = Compiler.ExecuteAction(Action);

  Files->clearStatCache();
  return Success;
}
