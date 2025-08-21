// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/IR/IR.h>

namespace FEXCore::CPU {
enum class RelocationTypes : uint8_t {
  // 8 byte literal in memory for symbol
  // Aligned to struct RelocNamedSymbolLiteral
  RELOC_NAMED_SYMBOL_LITERAL,

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

struct RelocationTypeHeader final {
  RelocationTypes Type;
};

struct RelocNamedSymbolLiteral final {
  enum class NamedSymbol : uint8_t {
    ///< Thread specific relocations
    // JIT Literal pointers
    SYMBOL_LITERAL_EXITFUNCTION_LINKER,
  };

  RelocationTypeHeader Header {};

  NamedSymbol Symbol;

  // Offset in to the code section to begin the relocation
  uint64_t Offset {};
};

struct RelocNamedThunkMove final {
  RelocationTypeHeader Header {};

  // GPR index the constant is being moved to
  uint8_t RegisterIndex;

  // The thunk SHA256 hash
  IR::SHA256Sum Symbol;

  // Offset in to the code section to begin the relocation
  uint64_t Offset {};
};

struct RelocGuestRIPMove final {
  RelocationTypeHeader Header {};

  // GPR index the constant is being moved to
  uint8_t RegisterIndex;

  // Offset in to the code section to begin the relocation
  uint64_t Offset {};

  // The unrelocated RIP that is being moved
  uint64_t GuestRIP;
};

union Relocation {
  RelocationTypeHeader Header {};

  RelocNamedSymbolLiteral NamedSymbolLiteral;
  // This makes our union of relocations at least 48 bytes
  // It might be more efficient to not use a union
  RelocNamedThunkMove NamedThunkMove;

  RelocGuestRIPMove GuestRIPMove;
};
} // namespace FEXCore::CPU
