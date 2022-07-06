/*
$info$
tags: backend|arm64
desc: relocation logic of the arm64 splatter backend
$end_info$
*/
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/HLE/Thunks/Thunks.h"
#include "FEXCore/Core/CPURelocations.h"
#include "Interface/Core/CodeCache/ObjCache.h"

template <typename T>
inline std::vector<uint8_t> &
operator << (std::vector<uint8_t> & out, T const && data) {
  auto size = out.size();
  out.resize(sizeof(data) + size);
  (T&)out.at(size) = data;
  return out;
}

#define AOTLOG(...)
//#define AOTLOG LogMan::Msg::DFmt

namespace FEXCore::CPU {
    
uint64_t Arm64JITCore::GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
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

void Arm64JITCore::InsertNamedThunkRelocation(vixl::aarch64::Register Reg, const IR::SHA256Sum &Sum) {
  auto CurrentCursor = GetCursorOffset();

  uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Sum));

  LoadConstant(Reg, Pointer, true /* for now*/);
  
  Relocations << RelocNamedThunkMove {
    .Offset = CurrentCursor - CursorEntry,
    .Symbol = Sum,
    .RegisterIndex = static_cast<uint8_t>(Reg.GetCode())
  };
}

Arm64JITCore::RelocatedLiteralPair Arm64JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  uint64_t Pointer = GetNamedSymbolLiteral(Op);

  Arm64JITCore::RelocatedLiteralPair Lit {
    .Lit = Literal(Pointer),
    .MoveABI = {
      .NamedSymbolLiteral = {
        .Header = {
          .Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL,
        },
        .Offset = 0,
        .Symbol = Op,
      },
    },
  };
  return Lit;
}

Arm64JITCore::RelocatedLiteralPair Arm64JITCore::InsertGuestRIPLiteral(const uint64_t GuestRIP) {
  RelocatedLiteralPair Lit {
    .Lit = Literal(GuestRIP),
    .MoveABI = {
      .GuestRIPMove = {
        .Header = {
          .Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL,
        },
        .Offset = 0,
        .GuestEntryOffset = GuestRIP - Entry,
      },
    },
  };
  return Lit;
}


void Arm64JITCore::PlaceRelocatedLiteral(RelocatedLiteralPair &Lit) {
  
  auto CurrentCursor = GetCursorOffset();

  switch (Lit.MoveABI.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
      Lit.MoveABI.NamedSymbolLiteral.Offset = CurrentCursor - CursorEntry;
      Relocations << std::move(Lit.MoveABI.NamedSymbolLiteral);
      break;

    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL:
      Lit.MoveABI.GuestRIPLiteral.Offset = CurrentCursor - CursorEntry;
      Relocations << std::move(Lit.MoveABI.GuestRIPLiteral);
      break;
    default:
      ERROR_AND_DIE_FMT("PlaceRelocatedLiteral: Invalid value in Lit.MoveABI.Header.Type");
  }

  place(&Lit.Lit);
}

void Arm64JITCore::InsertGuestRIPMove(vixl::aarch64::Register Reg, const uint64_t GuestRIP) {
  auto CurrentCursor = GetCursorOffset();

  LoadConstant(Reg, GuestRIP, CTX->Config.ObjCache());

  Relocations << RelocGuestRIPMove {
    .Offset = CurrentCursor - CursorEntry,
    .GuestEntryOffset = GuestRIP - Entry,
    .RegisterIndex = static_cast<uint8_t>(Reg.GetCode())
  };
}

void *Arm64JITCore::RelocateJITObjectCode(uint64_t Entry, const ObjCacheFragment *const HostCode, const ObjCacheRelocations *const Relocations) {
  AOTLOG("Relocating RIP 0x{:x}", Entry);


  if ((GetCursorOffset() + HostCode->Bytes) > CurrentCodeBuffer->Size) {
    ThreadState->CTX->ClearCodeCache(ThreadState);
  }

  auto CursorBegin = GetCursorOffset();
  auto HostEntry = GetCursorAddress<uint64_t>();

  AOTLOG("RIP Entry: disas 0x{:x},+{}", (uintptr_t)HostEntry, HostCode->Bytes);

  // Forward the cursor
  GetBuffer()->CursorForward(HostCode->Bytes);

  memcpy(reinterpret_cast<void*>(HostEntry), HostCode->Code, HostCode->Bytes);

  // Relocation apply messes with the cursor
  // Save the cursor and restore at the end
  auto CurrentCursor = GetCursorOffset();
  bool Result = ApplyRelocations(Entry, HostEntry, CursorBegin, Relocations);

  if (!Result) {
    // Reset cursor to the start
    GetBuffer()->SetCursorOffset(CursorBegin);
    return nullptr;
  }

  // We've moved the cursor around with relocations. Move it back to where we were before relocations
  GetBuffer()->SetCursorOffset(CurrentCursor);

  FinalizeCode();

  auto CodeEnd = GetCursorAddress<uint64_t>();
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(HostEntry), CodeEnd - reinterpret_cast<uint64_t>(HostEntry));

  this->IR = nullptr;

  //AOTLOG("\tRelocated JIT at [0x{:x}, 0x{:x}): RIP 0x{:x}", (uint64_t)HostEntry, CodeEnd, Entry);
  return reinterpret_cast<void*>(HostEntry);
}

bool Arm64JITCore::ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, const ObjCacheRelocations *const Relocations) {
  auto Begin = (const FEXCore::CPU::Relocation *)(Relocations->Relocations);
  auto End = (const FEXCore::CPU::Relocation *)(Relocations->Relocations + Relocations->Bytes);

  for (auto Reloc = Begin; Reloc < End; ) {
    switch (Reloc->Header.Type) {
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        uint64_t Pointer = GetNamedSymbolLiteral(Reloc->NamedSymbolLiteral.Symbol);
        // Relocation occurs at the cursorEntry + offset relative to that cursor
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->NamedSymbolLiteral.Offset);

        // Generate a literal so we can place it
        Literal<uint64_t> Lit(Pointer);
        place(&Lit);

        Reloc = Reloc->NamedSymbolLiteral.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
        uint64_t Data = GuestEntry + Reloc->GuestRIPLiteral.GuestEntryOffset;
        // Relocation occurs at the cursorEntry + offset relative to that cursor
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->GuestRIPLiteral.Offset);

        // Generate a literal so we can place it
        Literal<uint64_t> Lit(Data);
        place(&Lit);

        Reloc = Reloc->GuestRIPLiteral.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        uint64_t Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Reloc->NamedThunkMove.Symbol));

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->NamedThunkMove.Offset);
        LoadConstant(vixl::aarch64::XRegister(Reloc->NamedThunkMove.RegisterIndex), Pointer, true);
  
        Reloc = Reloc->NamedThunkMove.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
        uint64_t Data = GuestEntry + Reloc->GuestRIPMove.GuestEntryOffset;

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->GuestRIPMove.Offset);
        LoadConstant(vixl::aarch64::XRegister(Reloc->GuestRIPMove.RegisterIndex), Data, true);

        Reloc = Reloc->GuestRIPMove.Next();
        break;
      }
      default:
        ERROR_AND_DIE_FMT("Invalid Relocation type {}", (int)Reloc->Header.Type);
        FEX_UNREACHABLE;
    }
  }

  return true;
}
}

