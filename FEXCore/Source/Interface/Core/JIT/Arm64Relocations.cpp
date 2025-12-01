// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
desc: relocation logic of the arm64 splatter backend
$end_info$
*/
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/JITClass.h"

#include <FEXCore/Core/Thunks.h>

namespace FEXCore::Context {
static uint64_t GetNamedSymbolLiteral(ContextImpl& CTX, FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  switch (Op) {
  case FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol::SYMBOL_LITERAL_EXITFUNCTION_LINKER:
    return CTX.Dispatcher->GetExitFunctionLinkerAddress();

  default: ERROR_AND_DIE_FMT("Unknown named symbol literal: {}", static_cast<uint32_t>(Op));
  }
}
} // namespace FEXCore::Context

namespace FEXCore::CPU {
void Arm64JITCore::InsertNamedThunkRelocation(ARMEmitter::Register Reg, const IR::SHA256Sum& Sum) {
  Relocation MoveABI {};
  MoveABI.NamedThunkMove.Header = {.Offset = GetCursorOffset(), .Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE};
  MoveABI.NamedThunkMove.Symbol = Sum;
  MoveABI.NamedThunkMove.RegisterIndex = Reg.Idx();

  uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Sum));

  LoadConstant(ARMEmitter::Size::i64Bit, Reg, Pointer, false);
  Relocations.emplace_back(MoveABI);
}

Arm64JITCore::NamedSymbolLiteralPair Arm64JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  uint64_t Pointer = GetNamedSymbolLiteral(*CTX, Op);

  NamedSymbolLiteralPair Lit {
    .Lit = Pointer,
    .MoveABI =
      {
        .NamedSymbolLiteral =
          {
            .Header =
              {
                .Offset = 0, // Set by PlaceNamedSymbolLiteral
                .Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL,
              },
            .Symbol = Op,
          },
      },
  };
  return Lit;
}

void Arm64JITCore::PlaceNamedSymbolLiteral(NamedSymbolLiteralPair Lit) {
  switch (Lit.MoveABI.Header.Type) {
  case RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
  case RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
    Lit.MoveABI.Header.Offset = GetCursorOffset();
    break;
  }

  default: ERROR_AND_DIE_FMT("Unknown relocation type for {}", __FUNCTION__);
  }

  BindOrRestart(&Lit.Loc);
  dc64(Lit.Lit);
  Relocations.emplace_back(Lit.MoveABI);
}

auto Arm64JITCore::InsertGuestRIPLiteral(uint64_t GuestRIP) -> NamedSymbolLiteralPair {
  return {
    .Lit = GuestRIP,
    .MoveABI =
      {
        .GuestRIP = {.Header =
                       {
                         .Offset = 0, // Set by PlaceNamedSymbolLiteral
                         .Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL,
                       },
                     // NOTE: Cache serialization will subtract the guest binary base address later to produce consistency results
                     .GuestRIP = GuestRIP},
      },
  };
}

void Arm64JITCore::InsertGuestRIPMove(ARMEmitter::Register Reg, uint64_t Constant) {
  Relocation MoveABI {};
  MoveABI.GuestRIP.Header = {.Offset = GetCursorOffset(), .Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE};
  // NOTE: Cache serialization will subtract the guest binary base address later to produce consistency results
  MoveABI.GuestRIP.GuestRIP = Constant;
  MoveABI.GuestRIP.RegisterIndex = Reg.Idx();

  LoadConstant(ARMEmitter::Size::i64Bit, Reg, Constant, false);
  Relocations.emplace_back(MoveABI);
}
} // namespace FEXCore::CPU

namespace FEXCore::Context {
bool CodeCache::ApplyCodeRelocations(uint64_t GuestEntry, std::span<std::byte> Code,
                                     std::span<const FEXCore::CPU::Relocation> EntryRelocations) {
  CPU::Arm64Emitter Emitter(&CTX, Code.data(), Code.size_bytes());
  for (size_t j = 0; j < EntryRelocations.size(); ++j) {
    const FEXCore::CPU::Relocation& Reloc = EntryRelocations[j];
    Emitter.SetCursorOffset(Reloc.Header.Offset);

    switch (Reloc.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
      // Generate a literal so we can place it
      uint64_t Pointer = GetNamedSymbolLiteral(CTX, Reloc.NamedSymbolLiteral.Symbol);
      Emitter.dc64(Pointer);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
      uint64_t Pointer = reinterpret_cast<uint64_t>(CTX.ThunkHandler->LookupThunk(Reloc.NamedThunkMove.Symbol));
      if (Pointer == ~0ULL) {
        return false;
      }

      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.NamedThunkMove.RegisterIndex), Pointer, true);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
      Emitter.dc64(GuestEntry + Reloc.GuestRIP.GuestRIP);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
      uint64_t Pointer = Reloc.GuestRIP.GuestRIP + GuestEntry;
      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.GuestRIP.RegisterIndex), Pointer, true);
      break;
    }

    default: ERROR_AND_DIE_FMT("Unknown relocation type {}", ToUnderlying(Reloc.Header.Type));
    }
  }

  return true;
}
} // namespace FEXCore::Context

namespace FEXCore::CPU {

fextl::vector<FEXCore::CPU::Relocation> Arm64JITCore::TakeRelocations(uint64_t GuestBaseAddress) {
  // Rebase relocations to library base address
  for (auto& Relocation : Relocations) {
    switch (Relocation.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
      Relocation.GuestRIP.GuestRIP -= GuestBaseAddress;
      break;
    }
    default:;
    }
  }

  return std::move(Relocations);
}

} // namespace FEXCore::CPU
