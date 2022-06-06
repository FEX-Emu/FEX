/*
$info$
tags: backend|arm64
desc: relocation logic of the arm64 splatter backend
$end_info$
*/
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/HLE/Thunks/Thunks.h"

namespace FEXCore::CPU {
    
uint64_t Arm64JITCore::GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  switch (Op) {
    case FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol::SYMBOL_LITERAL_EXITFUNCTION_LINKER:
      return Dispatcher->ExitFunctionLinkerAddress;
    break;
    default:
      ERROR_AND_DIE_FMT("Unknown named symbol literal: {}", static_cast<uint32_t>(Op));
    break;
  }
  return ~0ULL;
}

void Arm64JITCore::InsertNamedThunkRelocation(vixl::aarch64::Register Reg, const IR::SHA256Sum &Sum) {
  Relocation MoveABI{};
  MoveABI.NamedThunkMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  MoveABI.NamedThunkMove.Offset = CurrentCursor - GuestEntry;
  MoveABI.NamedThunkMove.Symbol = Sum;
  MoveABI.NamedThunkMove.RegisterIndex = Reg.GetCode();

  uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Sum));

  LoadConstant(Reg, Pointer, EmitterCTX->Config.CacheObjectCodeCompilation());
  Relocations.emplace_back(MoveABI);
}

Arm64JITCore::NamedSymbolLiteralPair Arm64JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  uint64_t Pointer = GetNamedSymbolLiteral(Op);

  Arm64JITCore::NamedSymbolLiteralPair Lit {
    .Lit = Literal(Pointer),
    .MoveABI = {
      .NamedSymbolLiteral = {
        .Header = {
          .Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL,
        },
        .Symbol = Op,
        .Offset = 0,
      },
    },
  };
  return Lit;
}

void Arm64JITCore::PlaceNamedSymbolLiteral(NamedSymbolLiteralPair &Lit) {
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  Lit.MoveABI.NamedSymbolLiteral.Offset = CurrentCursor - GuestEntry;

  place(&Lit.Lit);
  Relocations.emplace_back(Lit.MoveABI);
}

void Arm64JITCore::InsertGuestRIPMove(vixl::aarch64::Register Reg, uint64_t Constant) {
  Relocation MoveABI{};
  MoveABI.GuestRIPMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  MoveABI.GuestRIPMove.Offset = CurrentCursor - GuestEntry;
  MoveABI.GuestRIPMove.GuestRIP = Constant;
  MoveABI.GuestRIPMove.RegisterIndex = Reg.GetCode();

  LoadConstant(Reg, Constant, EmitterCTX->Config.CacheObjectCodeCompilation());
  Relocations.emplace_back(MoveABI);
}

bool Arm64JITCore::ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations) {
  size_t DataIndex{};
  for (size_t j = 0; j < NumRelocations; ++j) {
    const FEXCore::CPU::Relocation *Reloc = reinterpret_cast<const FEXCore::CPU::Relocation *>(&EntryRelocations[DataIndex]);
    LOGMAN_THROW_A_FMT((DataIndex % alignof(Relocation)) == 0, "Alignment of relocation wasn't adhered to");

    switch (Reloc->Header.Type) {
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        uint64_t Pointer = GetNamedSymbolLiteral(Reloc->NamedSymbolLiteral.Symbol);
        // Relocation occurs at the cursorEntry + offset relative to that cursor
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->NamedSymbolLiteral.Offset);

        // Generate a literal so we can place it
        Literal<uint64_t> Lit(Pointer);
        place(&Lit);

        DataIndex += sizeof(Reloc->NamedSymbolLiteral);
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Reloc->NamedThunkMove.Symbol));
        if (Pointer == ~0ULL) {
          return false;
        }

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->NamedThunkMove.Offset);
        LoadConstant(vixl::aarch64::XRegister(Reloc->NamedThunkMove.RegisterIndex), Pointer, true);
        DataIndex += sizeof(Reloc->NamedThunkMove);
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
        // XXX: Reenable once the JIT Object Cache is upstream
        // XXX: Should spin the relocation list, create a list of guest RIP moves, and ask for them all once, reduces lock contention.
        uint64_t Pointer = ~0ULL; // EmitterCTX->JITObjectCache->FindRelocatedRIP(Reloc->GuestRIPMove.GuestRIP);
        if (Pointer == ~0ULL) {
          return false;
        }

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->GuestRIPMove.Offset);
        LoadConstant(vixl::aarch64::XRegister(Reloc->GuestRIPMove.RegisterIndex), Pointer, true);
        DataIndex += sizeof(Reloc->GuestRIPMove);
        break;
      }
    }
  }

  return true;
}
}

