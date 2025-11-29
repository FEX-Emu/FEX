// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
enum class RelocationTypes : uint32_t {
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

struct FEX_PACKED RelocationHeader final {
  // Offset to the relocated host code data
  uint64_t Offset {};

  RelocationTypes Type;
};

struct RelocNamedSymbolLiteral final {
  enum class NamedSymbol : uint32_t {
    ///< Thread specific relocations
    // JIT Literal pointers
    SYMBOL_LITERAL_EXITFUNCTION_LINKER,
  };

  RelocationHeader Header {};

  NamedSymbol Symbol;

  uint32_t Pad[8];
};

struct RelocNamedThunkMove final {
  RelocationHeader Header {};

  // GPR index the constant is being moved to
  uint32_t RegisterIndex;

  // The thunk SHA256 hash
  IR::SHA256Sum Symbol;
};

struct RelocGuestRIPMove final {
  RelocationHeader Header {};

  // GPR index the constant is being moved to
  uint8_t RegisterIndex;

  char Pad[3];

  // The unrelocated RIP that is being moved
  uint64_t GuestRIP;

  uint32_t pad2[6] {};
};

union Relocation {
  RelocationHeader Header {};

  RelocNamedSymbolLiteral NamedSymbolLiteral;
  // This makes our union of relocations at least 48 bytes
  // It might be more efficient to not use a union
  RelocNamedThunkMove NamedThunkMove;

  RelocGuestRIPMove GuestRIPMove;
};
} // namespace FEXCore::CPU
