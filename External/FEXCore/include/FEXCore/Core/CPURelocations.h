#pragma once
#include <FEXCore/IR/IR.h>

namespace FEXCore::CPU {
  enum class RelocationTypes : uint8_t {
    // 8 byte literal in memory for symbol
    // Aligned to struct RelocNamedSymbolLiteral
    RELOC_NAMED_SYMBOL_LITERAL,
    RELOC_GUEST_RIP_LITERAL,

    // Fixed size named thunk move
    // 4 instruction constant generation on AArch64
    // 64-bit mov on x86-64
    // Aligned to struct RelocNamedThunkMove
    RELOC_NAMED_THUNK_MOVE,

    // Fixed size guest RIP move
    // 4 instruction constant generation on AArch64
    // 64-bit mov on x86-64
    // Aligned to struct RelocGuestRIPMove
    RELOC_GUEST_RIP_MOVE,
  };

  union Relocation;

  struct FEX_PACKED RelocationTypeHeader final {
    RelocationTypes Type;
  };

  struct FEX_PACKED RelocNamedSymbolLiteral final {
    enum class NamedSymbol : uint8_t {
      ///< Thread specific relocations
      // JIT Literal pointers
      SYMBOL_LITERAL_EXITFUNCTION_LINKER,
    };

    RelocationTypeHeader Header { .Type = RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL };

    // Offset in to the code section to begin the relocation
    uint64_t Offset;

    NamedSymbol Symbol;

    auto Next() const { return (const Relocation *)(this + 1); }
  };

  struct FEX_PACKED RelocGuestRIPLiteral final {
    RelocationTypeHeader Header { .Type = RelocationTypes::RELOC_GUEST_RIP_LITERAL };

    // Offset in to the code section to begin the relocation
    uint64_t Offset;

    // The offset relative to the fragment entry point
    uint64_t GuestEntryOffset;

    auto Next() const { return (const Relocation *)(this + 1); }
  };

  struct FEX_PACKED RelocNamedThunkMove final {
    RelocationTypeHeader Header { .Type = RelocationTypes::RELOC_NAMED_THUNK_MOVE };

    // Offset in to the code section to begin the relocation
    uint64_t Offset;

    // The thunk SHA256 hash
    IR::SHA256Sum Symbol;

    // GPR index the constant is being moved to
    uint8_t RegisterIndex;

    auto Next() const { return (const Relocation *)(this + 1); }
  };

  struct FEX_PACKED RelocGuestRIPMove final {
    RelocationTypeHeader Header { .Type = RelocationTypes::RELOC_GUEST_RIP_MOVE };

    // Offset in to the code section to begin the relocation
    uint64_t Offset;

    // The offset relative to the fragment entry point
    uint64_t GuestEntryOffset;

    // GPR index the constant is being moved to
    uint8_t RegisterIndex;

    auto Next() const { return (const Relocation *)(this + 1); }
  };

  union Relocation {
    RelocationTypeHeader Header{};

    RelocNamedSymbolLiteral NamedSymbolLiteral;
    RelocGuestRIPLiteral  GuestRIPLiteral;
    // This makes our union of relocations at least 48 bytes
    // It might be more efficient to not use a union
    RelocNamedThunkMove NamedThunkMove;

    RelocGuestRIPMove GuestRIPMove;
  };
}
