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

namespace FEXCore::CPU {

uint64_t Arm64JITCore::GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  switch (Op) {
  case FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol::SYMBOL_LITERAL_EXITFUNCTION_LINKER:
    return ThreadState->CurrentFrame->Pointers.Common.ExitFunctionLinker;
    break;
  default: ERROR_AND_DIE_FMT("Unknown named symbol literal: {}", static_cast<uint32_t>(Op)); break;
  }
  return ~0ULL;
}

void Arm64JITCore::InsertNamedThunkRelocation(ARMEmitter::Register Reg, const IR::SHA256Sum& Sum) {
  Relocation MoveABI {};
  MoveABI.NamedThunkMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint8_t*>();
  MoveABI.NamedThunkMove.Offset = CurrentCursor - CodeData.BlockBegin;
  MoveABI.NamedThunkMove.Symbol = Sum;
  MoveABI.NamedThunkMove.RegisterIndex = Reg.Idx();

  uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Sum));

  LoadConstant(ARMEmitter::Size::i64Bit, Reg, Pointer, false);
  Relocations.emplace_back(MoveABI);
}

Arm64JITCore::NamedSymbolLiteralPair Arm64JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  uint64_t Pointer = GetNamedSymbolLiteral(Op);

  Arm64JITCore::NamedSymbolLiteralPair Lit {
    .Lit = Pointer,
    .MoveABI =
      {
        .NamedSymbolLiteral =
          {
            .Header =
              {
                .Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL,
              },
            .Symbol = Op,
            .Offset = 0,
          },
      },
  };
  return Lit;
}

void Arm64JITCore::PlaceNamedSymbolLiteral(NamedSymbolLiteralPair& Lit) {
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint8_t*>();
  Lit.MoveABI.NamedSymbolLiteral.Offset = CurrentCursor - CodeData.BlockBegin;

  BindOrRestart(&Lit.Loc);
  dc64(Lit.Lit);
  Relocations.emplace_back(Lit.MoveABI);
}

void Arm64JITCore::InsertGuestRIPMove(ARMEmitter::Register Reg, uint64_t Constant) {
  Relocation MoveABI {};
  MoveABI.GuestRIPMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint8_t*>();
  MoveABI.GuestRIPMove.Offset = CurrentCursor - CodeData.BlockBegin;
  MoveABI.GuestRIPMove.GuestRIP = Constant;
  MoveABI.GuestRIPMove.RegisterIndex = Reg.Idx();

  LoadConstant(ARMEmitter::Size::i64Bit, Reg, Constant, false);
  Relocations.emplace_back(MoveABI);
}

bool Arm64JITCore::ApplyRelocations(uint64_t GuestEntry, std::span<std::byte> Code, std::span<const FEXCore::CPU::Relocation> Relocations) {
  const auto OrigBase = GetBufferBase();
  const auto OrigSize = GetBufferSize();
  const auto OrigOffset = GetCursorOffset();

  SetBuffer(reinterpret_cast<std::uint8_t*>(Code.data()), Code.size_bytes());
  for (auto& Reloc : Relocations) {
    switch (Reloc.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
      uint64_t Pointer = GetNamedSymbolLiteral(Reloc.NamedSymbolLiteral.Symbol);
      // Relocation occurs at the cursorEntry + offset relative to that cursor
      SetCursorOffset(Reloc.NamedSymbolLiteral.Offset);

      // Generate a literal so we can place it
      dc64(Pointer);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
      uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Reloc.NamedThunkMove.Symbol));
      if (Pointer == ~0ULL) {
        return false;
      }

      // Relocation occurs at the cursorEntry + offset relative to that cursor.
      SetCursorOffset(Reloc.NamedThunkMove.Offset);
      LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.NamedThunkMove.RegisterIndex), Pointer, true);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
      // XXX: Reenable once the JIT Object Cache is upstream
      // XXX: Should spin the relocation list, create a list of guest RIP moves, and ask for them all once, reduces lock contention.
      uint64_t Pointer = ~0ULL; // EmitterCTX->JITObjectCache->FindRelocatedRIP(Reloc->GuestRIPMove.GuestRIP);
      if (Pointer == ~0ULL) {
        SetBuffer(OrigBase, OrigSize);
        SetCursorOffset(OrigOffset);
        return false;
      }

      // Relocation occurs at the cursorEntry + offset relative to that cursor.
      SetCursorOffset(Reloc.GuestRIPMove.Offset);
      LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.GuestRIPMove.RegisterIndex), Pointer, true);
      break;
    }
    }
  }

  SetBuffer(OrigBase, OrigSize);
  SetCursorOffset(OrigOffset);

  return true;
}

fextl::vector<FEXCore::CPU::Relocation> Arm64JITCore::TakeRelocations() {
  return std::move(Relocations);
}

} // namespace FEXCore::CPU
