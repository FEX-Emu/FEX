#include "analysis.h"
#include "data_layout.h"
#include "interface.h"

#include <fmt/format.h>

#include <openssl/sha.h>

constexpr bool enable_debug_output = false;

// Visitor for gathering data layout information that can be passed across libclang invocations
class AnalyzeDataLayoutAction : public AnalysisAction {
  ABI& type_abi;

  void OnAnalysisComplete(clang::ASTContext&) override;

public:
  AnalyzeDataLayoutAction(ABI&);
};

AnalyzeDataLayoutAction::AnalyzeDataLayoutAction(ABI& abi_)
  : type_abi(abi_) {}

std::unordered_map<const clang::Type*, TypeInfo>
ComputeDataLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, AnalysisAction::RepackedType>& types) {
  std::unordered_map<const clang::Type*, TypeInfo> layout;

  // First, add all types directly used in function signatures of the library API to the meta set
  for (const auto& [type, type_repack_info] : types) {
    if (type_repack_info.assumed_compatible) {
      auto [_, inserted] = layout.insert(std::pair {context.getCanonicalType(type), TypeInfo {}});
      if (!inserted) {
        throw std::runtime_error(
          "Failed to gather type metadata: Opaque type \"" + clang::QualType {type, 0}.getAsString() + "\" already registered");
      }
      continue;
    }

    if (type->isIncompleteType()) {
      throw std::runtime_error(
        "Cannot compute data layout of incomplete type \"" + clang::QualType {type, 0}.getAsString() + "\". Did you forget any annotations?");
    }

    if (type->isStructureType()) {
      StructInfo info;
      info.size_bits = context.getTypeSize(type);
      info.alignment_bits = context.getTypeAlign(type);

      auto [_, inserted] = layout.insert(std::pair {context.getCanonicalType(type), info});
      if (!inserted) {
        throw std::runtime_error("Failed to gather type metadata: Type \"" + clang::QualType {type, 0}.getAsString() + "\" already registered");
      }
    } else if (type->isBuiltinType() || type->isEnumeralType()) {
      SimpleTypeInfo info;
      info.size_bits = context.getTypeSize(type);
      info.alignment_bits = context.getTypeAlign(type);

      // NOTE: Non-enum types are intentionally not canonicalized since that would turn e.g. size_t into platform-specific types
      auto [_, inserted] = layout.insert(std::pair {type->isEnumeralType() ? context.getCanonicalType(type) : type, info});
      if (!inserted) {
        throw std::runtime_error("Failed to gather type metadata: Type \"" + clang::QualType {type, 0}.getAsString() + "\" already registered");
      }
    }
  }

  // Then, add information about members
  for (const auto& [type, type_repack_info] : types) {
    if (!type->isStructureType() || type_repack_info.assumed_compatible) {
      continue;
    }

    auto& info = *layout.at(context.getCanonicalType(type)).get_if_struct();

    for (auto* field : type->getAsStructureType()->getDecl()->fields()) {
      auto field_type = field->getType().getTypePtr();
      std::optional<uint64_t> array_size;
      if (auto array_type = llvm::dyn_cast<clang::ConstantArrayType>(field->getType())) {
        array_size = array_type->getSize().getZExtValue();
        field_type = array_type->getElementType().getTypePtr();
        if (llvm::isa<clang::ConstantArrayType>(field_type)) {
          throw std::runtime_error("Unsupported multi-dimensional array member \"" + field->getNameAsString() + "\" in type \"" +
                                   clang::QualType {type, 0}.getAsString() + "\"");
        }
      }

      StructInfo::MemberInfo member_info {
        .size_bits = context.getTypeSize(field->getType()), // Total size even for arrays
        .offset_bits = context.getFieldOffset(field),
        .type_name = get_type_name(context, field_type),
        .member_name = field->getNameAsString(),
        .array_size = array_size,
        .is_function_pointer = field_type->isFunctionPointerType(),
        .is_integral = field->getType()->isIntegerType(),
        .is_signed_integer = field->getType()->isSignedIntegerType(),
      };

      // TODO: Process types in dependency-order. Currently we skip this
      //       check if we haven't processed the member type already,
      //       which is only safe since this is a consistency check
      if (field_type->isStructureType() && layout.contains(context.getCanonicalType(field_type))) {
        // Assert for self-consistency
        auto field_meta = layout.at(context.getCanonicalType(field_type));
        (void)types.at(context.getCanonicalType(field_type));
        if (auto field_info = field_meta.get_if_simple_or_struct()) {
          if (field_info->size_bits != member_info.size_bits / member_info.array_size.value_or(1)) {
            throw std::runtime_error("Inconsistent type size detected");
          }
        }
      }

      // Add built-in types, even if referenced through a pointer
      for (auto* inner_field_type = field_type; inner_field_type; inner_field_type = inner_field_type->getPointeeType().getTypePtrOrNull()) {
        if (inner_field_type->isBuiltinType() || inner_field_type->isEnumeralType()) {
          // The analysis pass doesn't explicitly register built-in types, so add them manually here
          SimpleTypeInfo info {
            .size_bits = context.getTypeSize(inner_field_type),
            .alignment_bits = context.getTypeAlign(inner_field_type),
          };
          if (!inner_field_type->isBuiltinType()) {
            inner_field_type = context.getCanonicalType(inner_field_type);
          }
          [[maybe_unused]] auto [prev, inserted] = layout.insert(std::pair {inner_field_type, info});
          //                    if (!inserted && prev->second != TypeInfo { info }) {
          //                        // TODO: Throw error since consistency check failed
          //                    }
        }
      }

      info.members.push_back(std::move(member_info));
    }
  }

  if (enable_debug_output) {
    for (const auto& [type, info] : layout) {
      auto basic_info = info.get_if_simple_or_struct();
      if (!basic_info) {
        continue;
      }

      fprintf(stderr, "  Host entry %s: %lu (%lu)\n", clang::QualType {type, 0}.getAsString().c_str(), basic_info->size_bits / 8,
              basic_info->alignment_bits / 8);

      if (auto struct_info = info.get_if_struct()) {
        for (const auto& member : struct_info->members) {
          fprintf(stderr, "    Offset %lu-%lu: %s %s%s\n", member.offset_bits / 8, (member.offset_bits + member.size_bits - 1) / 8,
                  member.type_name.c_str(), member.member_name.c_str(),
                  member.array_size ? fmt::format("[{}]", member.array_size.value()).c_str() : "");
        }
      }
    }
  }

  return layout;
}

ABI GetStableLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, TypeInfo>& data_layout) {
  ABI stable_layout;

  for (auto [type, type_info] : data_layout) {
    auto type_name = get_type_name(context, type);
    if (auto struct_info = type_info.get_if_struct()) {
      for (auto& member : struct_info->members) {
        if (member.is_integral) {
          // Map member types to fixed-size integers
          auto alt_type_name = get_fixed_size_int_name(member.is_signed_integer, member.size_bits);
          auto alt_type_info = SimpleTypeInfo {
            .size_bits = member.size_bits,
            .alignment_bits = context.getTypeAlign(context.getIntTypeForBitwidth(member.size_bits, member.is_signed_integer)),
          };
          stable_layout.insert(std::pair {alt_type_name, alt_type_info});
          member.type_name = std::move(alt_type_name);
        }
      }
    }

    auto [it, inserted] = stable_layout.insert(std::pair {type_name, std::move(type_info)});
    if (type->isIntegerType()) {
      auto alt_type_name = get_fixed_size_int_name(type, context);
      stable_layout.insert(std::pair {std::move(alt_type_name), type_info});
    }

    if (!inserted && it->second != type_info && !type->isIntegerType()) {
      throw std::runtime_error("Duplicate type information: Tried to re-register type \"" + type_name + "\"");
    }
  }

  stable_layout.pointer_size = context.getTypeSize(context.getUIntPtrType()) / 8;

  return stable_layout;
}

static std::array<uint8_t, 32> GetSha256(const std::string& function_name) {
  std::array<uint8_t, 32> sha256;
  SHA256(reinterpret_cast<const unsigned char*>(function_name.data()), function_name.size(), sha256.data());
  return sha256;
};

std::string GetTypeNameWithFixedSizeIntegers(clang::ASTContext& context, clang::QualType type) {
  if (type->isBuiltinType() && type->isIntegerType()) {
    auto size = context.getTypeSize(type);
    return fmt::format("uint{}_t", size);
  } else if (type->isPointerType() && type->getPointeeType()->isBuiltinType() && type->getPointeeType()->isIntegerType() &&
             context.getTypeSize(type->getPointeeType()) > 8) {
    // TODO: Also apply this path to char-like types
    auto size = context.getTypeSize(type->getPointeeType());
    return fmt::format("uint{}_t*", size);
  } else {
    return type.getAsString();
  }
}

void AnalyzeDataLayoutAction::OnAnalysisComplete(clang::ASTContext& context) {
  type_abi = GetStableLayout(context, ComputeDataLayout(context, types));

  // Register functions that must be guest-callable through host function pointers
  for (auto funcptr_type_it = thunked_funcptrs.begin(); funcptr_type_it != thunked_funcptrs.end(); ++funcptr_type_it) {
    auto& funcptr_id = funcptr_type_it->first;
    auto& [type, param_annotations] = funcptr_type_it->second;
    auto func_type = type->getAs<clang::FunctionProtoType>();
    std::string mangled_name = clang::QualType {type, 0}.getAsString();
    auto cb_sha256 = GetSha256("fexcallback_" + mangled_name);
    FuncPtrInfo info = {cb_sha256};

    // TODO: Also apply GetTypeNameWithFixedSizeIntegers here
    info.result = func_type->getReturnType().getAsString();

    for (auto arg : func_type->getParamTypes()) {
      info.args.push_back(GetTypeNameWithFixedSizeIntegers(context, arg));
    }
    type_abi.thunked_funcptrs[funcptr_id] = std::move(info);
  }
}

TypeCompatibility DataLayoutCompareAction::GetTypeCompatibility(const clang::ASTContext& context, const clang::Type* type,
                                                                const std::unordered_map<const clang::Type*, TypeInfo> host_abi,
                                                                std::unordered_map<const clang::Type*, TypeCompatibility>& type_compat) {
  assert(type->isCanonicalUnqualified() || type->isBuiltinType() || type->isEnumeralType());

  {
    // Reserve a slot to be filled later. The placeholder value is used
    // to detect infinite recursions.
    constexpr auto placeholder_compat = TypeCompatibility {100};
    auto [existing_compat_it, is_new_type] = type_compat.emplace(type, placeholder_compat);
    if (!is_new_type) {
      if (existing_compat_it->second == placeholder_compat) {
        throw std::runtime_error("Found recursive reference to type \"" + clang::QualType {type, 0}.getAsString() + "\"");
      }

      return existing_compat_it->second;
    }
  }

  if (types.contains(type) && types.at(type).assumed_compatible) {
    if (types.at(type).pointers_only && !type->isPointerType()) {
      throw std::runtime_error(
        "Tried to dereference opaque type \"" + clang::QualType {type, 0}.getAsString() + "\" when querying data layout compatibility");
    }
    type_compat.at(type) = TypeCompatibility::Full;
    return TypeCompatibility::Full;
  }

  auto type_name = get_type_name(context, type);
  // Look up the same type name in the guest map,
  // unless it's an integer (which is mapped to fixed-size uintX_t types)
  auto guest_info = guest_abi.at(!type->isIntegerType() ? std::move(type_name) : get_fixed_size_int_name(type, context));
  auto& host_info = host_abi.at(type->isBuiltinType() ? type : context.getCanonicalType(type));

  const bool is_32bit = (guest_abi.pointer_size == 4);

  // Assume full compatibility, then downgrade as needed
  auto compat = TypeCompatibility::Full;

  if (guest_info != host_info) {
    // Non-matching data layout... downgrade to Repackable
    // TODO: Even for non-structs, this only works if the types are reasonably similar (e.g. uint32_t -> uint64_t)
    compat = TypeCompatibility::Repackable;
  }

  auto guest_struct_info = guest_info.get_if_struct();
  if (guest_struct_info && guest_struct_info->members.size() != host_info.get_if_struct()->members.size()) {
    // Members are missing from either the guest or host layout
    // NOTE: If the members are merely named differently, this will be caught in the else-if below
    compat = TypeCompatibility::None;
  } else if (guest_struct_info) {
    std::vector<TypeCompatibility> member_compat;
    for (std::size_t member_idx = 0; member_idx < guest_struct_info->members.size(); ++member_idx) {
      // Look up the corresponding member in the host struct definition.
      // The members may be listed in a different order, so we can't
      // directly use member_idx for this
      auto* host_member_field = [&]() -> clang::FieldDecl* {
        auto struct_decl = type->getAsStructureType()->getDecl();
        auto it = std::find_if(struct_decl->field_begin(), struct_decl->field_end(),
                               [&](auto* field) { return field->getName() == guest_struct_info->members.at(member_idx).member_name; });
        if (it == struct_decl->field_end()) {
          return nullptr;
        }
        return *it;
      }();
      if (!host_member_field) {
        // No corresponding host struct member
        // TODO: Also detect host members that are missing from the guest struct
        member_compat.push_back(TypeCompatibility::None);
        break;
      }

      auto host_member_type = context.getCanonicalType(host_member_field->getType().getTypePtr());
      if (auto array_type = llvm::dyn_cast<clang::ConstantArrayType>(host_member_type)) {
        // Compare array element type only. The array size is already considered by the layout information of the containing struct.
        host_member_type = context.getCanonicalType(array_type->getElementType().getTypePtr());
      }

      if (types.at(type).UsesCustomRepackFor(host_member_field)) {
        member_compat.push_back(TypeCompatibility::Repackable);
        continue;
      } else if (host_member_type->isPointerType()) {
        // Automatic repacking of pointers to non-compatible types is only possible if:
        // * Pointee is fully compatible, or
        // * Pointer member is annotated
        auto host_member_pointee_type = context.getCanonicalType(host_member_type->getPointeeType().getTypePtr());
        if (types.contains(host_member_pointee_type) && types.at(host_member_pointee_type).assumed_compatible) {
          // Pointee doesn't need repacking, but pointer needs extending on 32-bit
          member_compat.push_back(is_32bit ? TypeCompatibility::Repackable : TypeCompatibility::Full);
        } else if (host_member_pointee_type->isPointerType()) {
          // This is a nested pointer, e.g. void**

          if (is_32bit) {
            // Nested pointers can't be repacked on 32-bit
            member_compat.push_back(TypeCompatibility::None);
          } else if (types.contains(host_member_pointee_type->getPointeeType().getTypePtr()) &&
                     types.at(host_member_pointee_type->getPointeeType().getTypePtr()).assumed_compatible) {
            // Pointers to opaque types are fine
            member_compat.push_back(TypeCompatibility::Full);
          } else {
            // Check the innermost type's compatibility on 64-bit
            auto pointee_pointee_type = host_member_pointee_type->getPointeeType().getTypePtr();
            // TODO: Not sure how to handle void here. Probably should require an annotation instead of "just working"
            auto pointee_pointee_compat = pointee_pointee_type->isVoidType() ?
                                            TypeCompatibility::Full :
                                            GetTypeCompatibility(context, pointee_pointee_type, host_abi, type_compat);
            if (pointee_pointee_compat == TypeCompatibility::Full) {
              member_compat.push_back(TypeCompatibility::Full);
            } else {
              member_compat.push_back(TypeCompatibility::None);
            }
          }
        } else if (!host_member_pointee_type->isVoidType() &&
                   (host_member_pointee_type->isBuiltinType() || host_member_pointee_type->isEnumeralType())) {
          // TODO: What are good heuristics for this?
          // size_t should yield TypeCompatibility::Repackable
          // inconsistent types should probably default to TypeCompatibility::None
          // For now, just always assume compatible... (will degrade to Repackable below)
          member_compat.push_back(TypeCompatibility::Full);
        } else if (!host_member_pointee_type->isVoidType() &&
                   (host_member_pointee_type->isStructureType() || types.contains(host_member_pointee_type))) {
          auto pointee_compat = GetTypeCompatibility(context, host_member_pointee_type, host_abi, type_compat);
          if (pointee_compat == TypeCompatibility::Full) {
            // Pointee is fully compatible, so automatic repacking only requires converting the pointers themselves
            member_compat.push_back(is_32bit ? TypeCompatibility::Repackable : TypeCompatibility::Full);
          } else {
            // If the pointee is incompatible (even if repackable), automatic repacking isn't possible
            member_compat.push_back(TypeCompatibility::None);
          }
        } else if (!is_32bit && host_member_pointee_type->isVoidType()) {
          // TODO: Not sure how to handle void here. Probably should require an annotation instead of "just working"
          member_compat.push_back(TypeCompatibility::Full);
        } else {
          member_compat.push_back(TypeCompatibility::None);
        }
        continue;
      }

      if (guest_abi.at(guest_struct_info->members[member_idx].type_name).get_if_struct()) {
        auto host_type_info = host_abi.at(host_member_type);
        member_compat.push_back(GetTypeCompatibility(context, host_member_type, host_abi, type_compat));
      } else {
        // Member was checked for size/alignment above already
      }
    }

    if (std::all_of(member_compat.begin(), member_compat.end(), [](auto compat) { return compat == TypeCompatibility::Full; })) {
      // TypeCompatibility::Full or ::Repackable
    } else if (std::none_of(member_compat.begin(), member_compat.end(), [](auto compat) { return compat == TypeCompatibility::None; })) {
      // Downgrade to Repackable
      compat = TypeCompatibility::Repackable;
    } else {
      // Downgrade to None
      compat = TypeCompatibility::None;
    }
  }

  type_compat.at(type) = compat;
  return compat;
}

FuncPtrInfo DataLayoutCompareAction::LookupGuestFuncPtrInfo(const char* funcptr_id) {
  return guest_abi.thunked_funcptrs.at(funcptr_id);
}

DataLayoutCompareActionFactory::DataLayoutCompareActionFactory(const ABI& abi)
  : abi(abi) {}

DataLayoutCompareActionFactory::~DataLayoutCompareActionFactory() = default;

std::unique_ptr<clang::FrontendAction> DataLayoutCompareActionFactory::create() {
  return std::make_unique<DataLayoutCompareAction>(abi);
}

AnalyzeDataLayoutActionFactory::AnalyzeDataLayoutActionFactory()
  : abi(std::make_unique<ABI>()) {}

AnalyzeDataLayoutActionFactory::~AnalyzeDataLayoutActionFactory() = default;

std::unique_ptr<clang::FrontendAction> AnalyzeDataLayoutActionFactory::create() {
  return std::make_unique<AnalyzeDataLayoutAction>(*abi);
}
