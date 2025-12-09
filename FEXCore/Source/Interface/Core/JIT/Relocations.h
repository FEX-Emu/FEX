// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::CPU {
enum class RelocationTypes : uint32_t {
  // 8 byte literal in memory for symbol
  // Aligned to struct RelocNamedSymbolLiteral
  RELOC_NAMED_SYMBOL_LITERAL,

  // Fixed size named thunk move
  // 4 instruction constant generation
  // Aligned to struct RelocNamedThunkMove
  RELOC_NAMED_THUNK_MOVE,

  // 8 byte literal (relative to binary base address)
  RELOC_GUEST_RIP_LITERAL,

  // Fixed size guest RIP move
  // 4 instruction constant generation
  // Aligned to struct RelocGuestRIP
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

struct RelocGuestRIP final {
  RelocationHeader Header {};

  // GPR index the constant is being moved to (for non-literal relocations)
  uint8_t RegisterIndex;

  char Pad[3];

  // The base RIP (to be moved by the register for non-literal relocations).
  // In a serialized code cache, this is relative to the binary base address.
  uint64_t GuestRIP;

  uint32_t pad2[6] {};
};

union Relocation {
  // Clang 16 Can't default-initialize this union
  static Relocation Default() {
#if __clang_major__ < 17
    Relocation Ret {.Header {}};
    memset(&Ret, 0, sizeof(Ret));
    return Ret;
#else
    return {};
#endif
  }

  RelocationHeader Header {};

  RelocNamedSymbolLiteral NamedSymbolLiteral;
  // This makes our union of relocations at least 48 bytes
  // It might be more efficient to not use a union
  RelocNamedThunkMove NamedThunkMove;

  RelocGuestRIP GuestRIP;
};

uint64_t GetNamedSymbolLiteral(FEXCore::Context::ContextImpl&, RelocNamedSymbolLiteral::NamedSymbol);

} // namespace FEXCore::CPU
