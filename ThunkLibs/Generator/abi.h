#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct PointerInfo {
    enum class Type {
        Pointer,     // Pointer to an object with separate ABI info
        Array,       // Array with fixed size specified in array_size

        Passthrough, // Pass-through as uint of guest-size
        Untyped,     // Pass-through with zero-extension (if needed). Also used for pointers to opaque types, and pointers to trivially ABI-compatible types. TODO: This clearly needs a rename
        TODOONLY64 [[deprecated]], // Transitionary type. Will only work on 64-bit ABIs
    };

    std::optional<unsigned> array_size;
    // TODO: CV-qualifiers could be useful

    Type type = array_size ? Type::Array : Type::Pointer;

    constexpr bool operator==(const PointerInfo& oth) const noexcept {
        return array_size == oth.array_size && type == oth.type;
    }
};

// Simple type without members (e.g. int, enums, ...)
struct SimpleTypeInfo {
    std::vector<PointerInfo> pointer_chain;

    bool operator==(const SimpleTypeInfo& oth) const noexcept {
        return pointer_chain == oth.pointer_chain;
    }
};

// Struct or class
struct StructInfo {
    uint64_t size_bits = std::numeric_limits<uint64_t>::max();

    bool is_union = false;

    struct ChildInfo {
        uint64_t size_bits;
        uint64_t offset_bits;
        std::string type_name;
        std::string member_name;
        // TODO: We don't support nested pointers, so this should be an std::optional<PointerInfo> instead
        std::vector<PointerInfo> pointer_chain;
        std::optional<unsigned> bitfield_size;

        // If true, it is assumed that neither host nor guest read this member
        bool is_padding_member;

        constexpr bool operator==(const ChildInfo& oth) const noexcept {
            return offset_bits == oth.offset_bits &&
                   type_name == oth.type_name &&
                   member_name == oth.member_name &&
                   pointer_chain == oth.pointer_chain &&
                   bitfield_size == oth.bitfield_size &&
                   is_padding_member == oth.is_padding_member;
        }
    };

    std::vector<ChildInfo> children;

    constexpr bool operator==(const StructInfo& oth) const noexcept {
        return size_bits == oth.size_bits && is_union == oth.is_union && children == oth.children;
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

    const StructInfo* as_struct() const {
        return std::get_if<StructInfo>(this);
    }

    StructInfo* as_struct() {
        return std::get_if<StructInfo>(this);
    }
};

enum class ABIPlatform {
    Host,
//    Guest32,
    Guest64,
};

// Indexed by type names
struct ABI : std::unordered_map<std::string, TypeInfo> {
    ABIPlatform platform;
};

struct ABITable {
    static ABITable singleton() {
        // Just create one entry for the host ABI
        // TODO: Changed to storing the host ABI twice, since it makes the implementation easier...
        ABITable table;
        table.abis.resize(2);
        table.abis[0].platform = ABIPlatform::Host;
        table.abis[1].platform = ABIPlatform::Host;
        return table;
    }

    static ABITable standard() {
        // Create two entries: Host ABI and x86-64
        ABITable table;
        table.abis.resize(2);
        table.abis[0].platform = ABIPlatform::Host;
        table.abis[1].platform = ABIPlatform::Guest64;
        return table;
    }

    std::vector<ABI> abis;

    ABI& host() noexcept { return abis[0]; }
    const ABI& host() const noexcept { return abis[0]; }

private:
    ABITable() = default;
};
