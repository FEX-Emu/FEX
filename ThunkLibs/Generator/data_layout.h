#pragma once

#include "analysis.h"

#include <clang/Frontend/FrontendAction.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct SimpleTypeInfo {
    uint64_t size_bits;
    uint64_t alignment_bits;

    bool operator==(const SimpleTypeInfo& other) const {
        return  size_bits == other.size_bits &&
                alignment_bits == other.alignment_bits;
    }
};

struct StructInfo : SimpleTypeInfo {
    struct MemberInfo {
        uint64_t size_bits; // size of this member. For arrays, total size of all elements
        uint64_t offset_bits;
        std::string type_name;
        std::string member_name;
        std::optional<uint64_t> array_size;

        bool operator==(const MemberInfo& other) const {
            return  size_bits == other.size_bits &&
                    offset_bits == other.offset_bits &&
                    type_name == other.type_name &&
                    member_name == other.member_name &&
                    array_size == other.array_size;
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
    TypeInfo(const SimpleTypeInfo& info) : Parent(info) {}
    TypeInfo(const StructInfo& info) : Parent(info) {}

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

std::unordered_map<const clang::Type*, TypeInfo>
ComputeDataLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, AnalysisAction::RepackedType>& types);

// Convert the output of ComputeDataLayout to a format that isn't tied to a libclang session.
// As a consequence, type information is indexed by type name instead of clang::Type.
ABI GetStableLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, TypeInfo>& data_layout);

enum class TypeCompatibility {
    Full,       // Type has matching data layout across architectures
    Repackable, // Type has different data layout but can be repacked automatically
    None,       // Type has different data layout and cannot be repacked automatically
};

class DataLayoutCompareAction : public AnalysisAction {
public:
    DataLayoutCompareAction(const ABI& guest_abi) : guest_abi(guest_abi) {
    }

    TypeCompatibility GetTypeCompatibility(
            const clang::ASTContext&,
            const clang::Type*,
            const std::unordered_map<const clang::Type*, TypeInfo> host_abi,
            std::unordered_map<const clang::Type*, TypeCompatibility>& type_compat);

    FuncPtrInfo LookupGuestFuncPtrInfo(const char* funcptr_id);

protected:
    const ABI& guest_abi;
};
