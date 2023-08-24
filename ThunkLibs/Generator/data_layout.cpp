#include "analysis.h"
#include "data_layout.h"
#include "interface.h"

#include <fmt/format.h> // TODO: Drop

#include <openssl/sha.h>

std::unordered_map<const clang::Type*, TypeInfo> ComputeDataLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, AnalysisAction::RepackedType>& types) {
    std::unordered_map<const clang::Type*, TypeInfo> layout;

    // First, add all types directly used in function signatures of the library API to the meta set
    for (const auto& [type, type_repack_info] : types) {
        if (type->isIncompleteType()) {
            throw std::runtime_error("Cannot compute data layout of incomplete type \"" + clang::QualType { type, 0 }.getAsString() + "\". Did you forget any annotations?");
        }

        if (type->isStructureType()) {
            StructInfo info;
            info.size_bits = context.getTypeSize(type);
            info.alignment_bits = context.getTypeAlign(type);

            auto [_, inserted] = layout.insert(std::pair { context.getCanonicalType(type), info });
            if (!inserted) {
                throw std::runtime_error("Failed to gather type metadata: Type \"" + clang::QualType { type, 0 }.getAsString() + "\" already registered");
            }
        } else if (type->isBuiltinType() || type->isEnumeralType()) {
            SimpleTypeInfo info;
            info.size_bits = context.getTypeSize(type);
            info.alignment_bits = context.getTypeAlign(type);

            // NOTE: Non-enum types are intentionally not canonicalized since that would turn e.g. size_t into platform-specific types
            auto [_, inserted] = layout.insert(std::pair { type->isEnumeralType() ? context.getCanonicalType(type) : type, info });
            if (!inserted) {
                throw std::runtime_error("Failed to gather type metadata: Type \"" + clang::QualType { type, 0 }.getAsString() + "\" already registered");
            }
        }
    }

    // Then, add information about members
    for (const auto& [type, type_repack_info] : types) {
        if (!type->isStructureType()) {
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
                    throw std::runtime_error("Unsupported multi-dimensional array member \"" + field->getNameAsString() + "\" in type \"" + clang::QualType { type, 0 }.getAsString() + "\"");
                }
            }

            StructInfo::MemberInfo member_info {
                .size_bits = context.getTypeSize(field->getType()), // Total size even for arrays
                .offset_bits = context.getFieldOffset(field),
                .type_name = get_type_name(context, field_type),
                .member_name = field->getNameAsString(),
                .array_size = array_size,
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
                    auto [prev, inserted] = layout.insert(std::pair { inner_field_type, info });
//                    if (!inserted && prev->second != TypeInfo { info }) {
//                        // TODO: Throw error since consistency check failed
//                    }
                }
            }

            info.members.push_back(member_info);
        }
    }

    for (const auto& [type, info] : layout) {
        auto basic_info = info.get_if_simple_or_struct();
        if (!basic_info) {
            continue;
        }

        fprintf(stderr, "  Host entry %s: %lu (%lu)\n", clang::QualType { type, 0 }.getAsString().c_str(), basic_info->size_bits / 8, basic_info->alignment_bits / 8);

        if (auto struct_info = info.get_if_struct()) {
            for (const auto& member : struct_info->members) {
                fprintf(stderr, "    Offset %lu-%lu: %s %s%s\n", member.offset_bits / 8, (member.offset_bits + member.size_bits - 1) / 8, member.type_name.c_str(), member.member_name.c_str(), member.array_size ? fmt::format("[{}]", member.array_size.value()).c_str() : "");
            }
        }
    }

    return layout;
}
