/*
$info$
tags: backend|x86-64
desc: relocation logic of the x86-64 splatter backend
$end_info$
*/
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/HLE/Thunks/Thunks.h"

namespace FEXCore::CPU {
uint64_t X86JITCore::GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  switch (Op) {
    case FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol::SYMBOL_LITERAL_EXITFUNCTION_LINKER:
      return ThreadState->CurrentFrame->Pointers.Common.ExitFunctionLinker;
    break;
    default:
      ERROR_AND_DIE_FMT("Unknown named symbol literal: {}", static_cast<uint32_t>(Op));
    break;
  }
  return ~0ULL;

}

void X86JITCore::LoadConstantWithPadding(Xbyak::Reg Reg, uint64_t Constant) {
  // The maximum size a move constant can be in bytes
  // Need to NOP pad to this size to ensure backpatching is always the same size
  // Calculated as:
  // [Rex]
  // [Mov op]
  // [8 byte constant]
  //
  // All other move types are smaller than this. xbyak will use a NOP slide which is quite quick
  constexpr static size_t MAX_MOVE_SIZE = 10;
  auto StartingOffset = getSize();
  mov(Reg, Constant);
  auto MoveSize = getSize() - StartingOffset;
  auto NOPPadSize = MAX_MOVE_SIZE - MoveSize;
  nop(NOPPadSize);
}

X86JITCore::NamedSymbolLiteralPair X86JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  NamedSymbolLiteralPair Lit {
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

void X86JITCore::PlaceNamedSymbolLiteral(NamedSymbolLiteralPair &Lit) {
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = getSize();
  Lit.MoveABI.NamedSymbolLiteral.Offset = CurrentCursor - CursorEntry;

  uint64_t Pointer = GetNamedSymbolLiteral(Lit.MoveABI.NamedSymbolLiteral.Symbol);

  L(Lit.Offset);
  dq(Pointer);
  Relocations.emplace_back(Lit.MoveABI);
}


void X86JITCore::InsertGuestRIPMove(Xbyak::Reg Reg, uint64_t Constant) {
  Relocation MoveABI{};
  MoveABI.GuestRIPMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE;

  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = getSize();
  MoveABI.GuestRIPMove.Offset = CurrentCursor - CursorEntry;
  MoveABI.GuestRIPMove.GuestRIP = Constant;
  MoveABI.GuestRIPMove.RegisterIndex = Reg.getIdx();

  if (CTX->Config.CacheObjectCodeCompilation()) {
    LoadConstantWithPadding(Reg, Constant);
  }
  else {
    mov(Reg, Constant);
  }

  Relocations.emplace_back(MoveABI);
}

bool X86JITCore::ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations) {
  size_t DataIndex{};
  for (size_t j = 0; j < NumRelocations; ++j) {
    const FEXCore::CPU::Relocation *Reloc = reinterpret_cast<const FEXCore::CPU::Relocation *>(&EntryRelocations[DataIndex]);
    LOGMAN_THROW_AA_FMT((DataIndex % alignof(Relocation)) == 0, "Alignment of relocation wasn't adhered to");

    switch (Reloc->Header.Type) {
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        uint64_t Pointer = GetNamedSymbolLiteral(Reloc->NamedSymbolLiteral.Symbol);
        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->NamedSymbolLiteral.Offset);

        // Place the pointer
        dq(Pointer);

        DataIndex += sizeof(Reloc->NamedSymbolLiteral);
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        uint64_t Pointer = reinterpret_cast<uint64_t>(CTX->ThunkHandler->LookupThunk(Reloc->NamedThunkMove.Symbol));
        if (Pointer == ~0ULL) {
          return false;
        }

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->NamedThunkMove.Offset);
        LoadConstantWithPadding(Xbyak::Reg64(Reloc->NamedThunkMove.RegisterIndex), Pointer);
        DataIndex += sizeof(Reloc->NamedThunkMove);
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
        // XXX: Reenable once the JIT Object Cache is upstream
        // XXX: Should spin the relocation list, create a list of guest RIP moves, and ask for them all once, reduces lock contention.
        uint64_t Pointer = ~0ULL; // EmitterCTX->JITObjectCache->FindRelocatedRIP(Reloc->GuestRIPMove.GuestRIP);
        if (Pointer == ~0ULL) {
          return false;
        }

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->GuestRIPMove.Offset);
        LoadConstantWithPadding(Xbyak::Reg64(Reloc->GuestRIPMove.RegisterIndex), Pointer);
        DataIndex += sizeof(Reloc->GuestRIPMove);
        break;
    }
  }

  return true;
}
}

