#pragma once
#include <FEXCore/IR/IR.h>
#include <vector>

namespace FEXCore::CPU {
  enum class RelocationTypes : uint8_t {
    // 4 instruction named symbol move
    // Aligned to struct RelocNamedSymbolMove
    RELOC_NAMED_SYMBOL_MOVE,

    // 8 byte literal in memory for symbol
    // Aligned to struct RelocNamedSymbolLiteral
    RELOC_NAMED_SYMBOL_LITERAL,

    // 4 instruction named thunk move
    // Aligned to struct RelocNamedThunkMove
    RELOC_NAMED_THUNK_MOVE,

    // 4 instruction guest RIP move
    // Aligned to struct RelocGuestRIPMove
    RELOC_GUEST_RIP_MOVE,
  };

  struct RelocationTypeHeader final {
    RelocationTypes Type;
  };

  struct RelocNamedSymbolMove final {
    enum class NamedSymbol : uint8_t {
      ///< XXX: Nothing here right now
      ///< Process specific relocations
      ///< Thread specific Relocations
    };

    RelocationTypeHeader Header{};

    // Offset to begin the relocation
    uint64_t Offset{};

    NamedSymbol Symbol;

    uint8_t RegisterIndex;
  };

  struct RelocNamedSymbolLiteral final {
    enum class NamedSymbol : uint8_t {
      ///< Thread specific relocations
      // JIT Literal pointers
      SYMBOL_LITERAL_EXITFUNCTION_LINKER,
    };

    RelocationTypeHeader Header{};

    NamedSymbol Symbol;

    // Offset to begin the relocation
    uint64_t Offset{};
  };

  struct RelocNamedThunkMove final {
    RelocationTypeHeader Header{};

    IR::SHA256Sum Symbol;
    uint8_t RegisterIndex;
  };

  struct RelocGuestRIPMove final {
    RelocationTypeHeader Header{};

    // Offset to begin the relocation
    uint64_t Offset{};

    uint64_t GuestRIP;

    uint8_t RegisterIndex;
  };

  union Relocation {
    RelocationTypeHeader Header{};

    RelocNamedSymbolMove NamedSymbolMove;
    RelocNamedSymbolLiteral NamedSymbolLiteral;
    // XXX: This makes our union of relocations at least 33 bytes
    // Make relocations variable sized rather than a union
    RelocNamedThunkMove NamedThunkMove;

    RelocGuestRIPMove GuestRIPMove;
  };
}
