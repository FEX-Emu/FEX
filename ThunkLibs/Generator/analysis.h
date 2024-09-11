#pragma once

#include <clang/Basic/FileEntry.h>
#include <clang/Frontend/FrontendAction.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

struct SimpleTypeInfo {
  uint64_t size_bits;
  uint64_t alignment_bits;

  bool operator==(const SimpleTypeInfo& other) const {
    return size_bits == other.size_bits && alignment_bits == other.alignment_bits;
  }
};

struct StructInfo : SimpleTypeInfo {
  struct MemberInfo {
    uint64_t size_bits; // size of this member. For arrays, total size of all elements
    uint64_t offset_bits;
    std::string type_name;
    std::string member_name;
    std::optional<uint64_t> array_size;
    bool is_function_pointer;
    bool is_integral;
    bool is_signed_integer;

    bool operator==(const MemberInfo& other) const {
      return size_bits == other.size_bits && offset_bits == other.offset_bits &&
             // The type name may differ for integral types if all other parameters are equal
             (type_name == other.type_name || (is_integral && other.is_integral)) && member_name == other.member_name &&
             array_size == other.array_size && is_function_pointer == other.is_function_pointer && is_integral == other.is_integral;
    }
  };

  std::vector<MemberInfo> members;

  bool operator==(const StructInfo& other) const {
    return (const SimpleTypeInfo&)*this == (const SimpleTypeInfo&)other &&
           std::equal(members.begin(), members.end(), other.members.begin(), other.members.end());
  }
};

struct TypeInfo : std::variant<std::monostate, SimpleTypeInfo, StructInfo> {
  using Parent = std::variant<std::monostate, SimpleTypeInfo, StructInfo>;

  TypeInfo() = default;
  TypeInfo(const SimpleTypeInfo& info)
    : Parent(info) {}
  TypeInfo(const StructInfo& info)
    : Parent(info) {}

  // Opaque declaration with no full definition.
  // Pointers to these can still be passed along ABI boundaries assuming
  // implementation details are only ever accessed on one side.
  bool is_opaque() const {
    return std::holds_alternative<std::monostate>(*this);
  }

  const StructInfo* get_if_struct() const {
    return std::get_if<StructInfo>(this);
  }

  StructInfo* get_if_struct() {
    return std::get_if<StructInfo>(this);
  }

  const SimpleTypeInfo* get_if_simple_or_struct() const {
    auto as_struct = std::get_if<StructInfo>(this);
    if (as_struct) {
      return as_struct;
    }
    return std::get_if<SimpleTypeInfo>(this);
  }
};

struct FuncPtrInfo {
  std::array<uint8_t, 32> sha256;
  std::string result;
  std::vector<std::string> args;
};

struct ABI : std::unordered_map<std::string, TypeInfo> {
  std::unordered_map<std::string, FuncPtrInfo> thunked_funcptrs;
  int pointer_size; // in bytes
};

struct FunctionParams {
  std::vector<clang::QualType> param_types;
};

struct ThunkedCallback : FunctionParams {
  clang::QualType return_type;

  bool is_stub = false; // Callback will be replaced by a stub that calls std::abort
  bool is_variadic = false;
};

struct ParameterAnnotations {
  bool is_passthrough = false;
  bool assume_compatible = false;

  bool operator==(const ParameterAnnotations&) const = default;
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
  bool is_variadic = false;

  // If true, the unpacking function will call a custom fexfn_impl function
  // to be provided manually instead of calling the host library function
  // directly.
  // This is implied e.g. for thunks generated for variadic functions
  bool custom_host_impl = false;

  bool inregister_abi = false;

  std::string GetOriginalFunctionName() const {
    const std::string suffix = "_internal";
    assert(function_name.length() > suffix.size());
    assert((std::string_view {&*function_name.end() - suffix.size(), suffix.size()} == suffix));
    return function_name.substr(0, function_name.size() - suffix.size());
  }

  // Maps parameter index to ThunkedCallback
  std::unordered_map<unsigned, ThunkedCallback> callbacks;

  // Maps parameter index to ParameterAnnotations
  // TODO: Use index -1 for the return value?
  std::unordered_map<unsigned, ParameterAnnotations> param_annotations;

  clang::FunctionDecl* decl;
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

  bool inregister_abi;

  bool is_variadic;

  // Index of the symbol table to store this export in (see guest_symtables).
  // If empty, a library export is created, otherwise the function is entered into a function pointer array
  std::optional<std::size_t> symtable_namespace;
};

struct NamespaceInfo {
  clang::DeclContext* context;

  std::string name;

  // Function to load native host library functions with.
  // This function must be defined manually with the signature "void* func(void*, const char*)"
  std::string host_loader;

  bool generate_guest_symtable;

  bool indirect_guest_calls;
};

class AnalysisAction : public clang::ASTFrontendAction {
public:
  AnalysisAction(const ABI& guest_abi)
    : guest_abi(guest_abi) {
    decl_contexts.push_back(nullptr); // global namespace (replaced by getTranslationUnitDecl later)
  }

  void ExecuteAction() override;

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef /*file*/) override;

  struct RepackedType {
    bool assumed_compatible = false;         // opaque_type or assume_compatible_data_layout
    bool pointers_only = assumed_compatible; // if true, only pointers to this type may be used

    // If true, emit guest_layout/host_layout definitions even if the type is non-repackable
    bool emit_layout_wrappers = false;

    // Set of members (identified by their field name) with custom repacking
    std::unordered_set<std::string> custom_repacked_members;

    bool UsesCustomRepackFor(const clang::FieldDecl* member) const {
      return custom_repacked_members.contains(member->getNameAsString());
    }
    bool UsesCustomRepackFor(const std::string& member_name) const {
      return custom_repacked_members.contains(member_name);
    }
  };

protected:
  // Build the internal API representation by processing fex_gen_config and other annotated entities
  void ParseInterface(clang::ASTContext&);

  // Recursively extend the type set to include types of struct members
  void CoverReferencedTypes(clang::ASTContext&);

  // Called from ExecuteAction() after parsing is complete
  virtual void OnAnalysisComplete(clang::ASTContext&) {};

  std::vector<clang::DeclContext*> decl_contexts;

  std::vector<ThunkedFunction> thunks;
  std::vector<ThunkedAPIFunction> thunked_api;

  // Set of function types for which to generate Guest->Host thunking trampolines.
  // The map key is a unique identifier that must be consistent between guest/host processing passes.
  // The map value is a pair of the function pointer's clang::Type and the mapping of parameter annotations
  std::unordered_map<std::string, std::pair<const clang::Type*, std::unordered_map<unsigned, ParameterAnnotations>>> thunked_funcptrs;

  std::unordered_map<const clang::Type*, RepackedType> types;
  std::optional<unsigned> lib_version;
  std::vector<NamespaceInfo> namespaces;

  RepackedType& LookupType(clang::ASTContext& context, const clang::Type* type) {
    return types.at(context.getCanonicalType(type));
  }

  const ABI& guest_abi;
};

inline std::string get_type_name(const clang::ASTContext& context, const clang::Type* type) {
  if (type->isBuiltinType()) {
    // Skip canonicalization
    return clang::QualType {type, 0}.getAsString();
  }

  if (auto decl = type->getAsTagDecl()) {
    // Replace unnamed types with a placeholder. This will fail to compile if referenced
    // anywhere in generated code, but at least it will point to a useful location.
    //
    // A notable exception are C-style struct declarations like "typedef struct (unnamed) { ... } MyStruct;".
    // A typedef name is associated with these for linking purposes, so
    // getAsString() will produce a usable identifier.
    // TODO: Consider turning this into a hard error instead of replacing the name
    if (!decl->getDeclName() && !decl->getTypedefNameForAnonDecl()) {
      auto loc = context.getSourceManager().getPresumedLoc(decl->getLocation());
      std::string filename = loc.getFilename();
      filename = std::move(filename).substr(filename.rfind("/"));
      filename = std::move(filename).substr(1);
      std::replace(filename.begin(), filename.end(), '.', '_');
      return "unnamed_type_" + filename + "_" + std::to_string(loc.getLine());
    }
  }

  auto type_name = clang::QualType {context.getCanonicalType(type), 0}.getAsString();
  if (type_name.starts_with("struct ")) {
    type_name = type_name.substr(7);
  }
  if (type_name.starts_with("class ") || type_name.starts_with("union ")) {
    type_name = type_name.substr(6);
  }
  if (type_name.starts_with("enum ")) {
    type_name = type_name.substr(5);
  }
  return type_name;
}

inline std::string get_fixed_size_int_name(bool is_signed, int size) {
  return (!is_signed ? "u" : "") + std::string {"int"} + std::to_string(size) + "_t";
}

inline std::string get_fixed_size_int_name(const clang::Type* type, int size) {
  return get_fixed_size_int_name(type->isSignedIntegerType(), size);
}

inline std::string get_fixed_size_int_name(const clang::Type* type, const clang::ASTContext& context) {
  return get_fixed_size_int_name(type, context.getTypeSize(type));
}
