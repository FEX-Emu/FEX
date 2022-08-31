/*
$info$
tags: backend|x86-64
desc: relocation logic of the x86-64 splatter backend
$end_info$
*/
#include "FEXCore/Utils/CompilerDefs.h"
#include "FEXCore/Utils/LogManager.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/HLE/Thunks/Thunks.h"
#include "FEXCore/Core/CPURelocations.h"
#include "Interface/Core/CodeCache/ObjCache.h"
#include "xbyak/xbyak.h"

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

void X86JITCore::InsertNamedThunkRelocation(Xbyak::Reg Reg, const IR::SHA256Sum &Sum) {
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = getSize();
  
  uint64_t Pointer = reinterpret_cast<uint64_t>(CTX->ThunkHandler->LookupThunk(Sum));

  LoadConstantWithPadding(Reg, Pointer);

  Relocations << RelocNamedThunkMove {
    .Offset = CurrentCursor - CursorEntry,
    .Symbol = Sum,
    .RegisterIndex = static_cast<uint8_t>(Reg.getIdx())
  };
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

X86JITCore::RelocatedLiteralPair X86JITCore::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  RelocatedLiteralPair Lit {
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

X86JITCore::RelocatedLiteralPair X86JITCore::InsertGuestRIPLiteral(const uint64_t GuestRIP) {
  AOTLOG("InsertGuestRIPLiteral: GuestRIP: {:x}, Entry: {:x}", GuestRIP, Entry);

  RelocatedLiteralPair Lit {
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

void X86JITCore::PlaceRelocatedLiteral(RelocatedLiteralPair &Lit) {
  uint64_t Value;

  L(Lit.Offset);

  switch (Lit.MoveABI.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
      Value = GetNamedSymbolLiteral(Lit.MoveABI.NamedSymbolLiteral.Symbol);
      Lit.MoveABI.NamedSymbolLiteral.Offset = getSize() - CursorEntry;
      Relocations << std::move(Lit.MoveABI.NamedSymbolLiteral);
      break;

    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL:
      Value = Lit.MoveABI.GuestRIPLiteral.GuestEntryOffset + Entry;
      Lit.MoveABI.GuestRIPLiteral.Offset = getSize() - CursorEntry;

      AOTLOG("PlaceRelocatedLiteral: GuestEntryOffset: {:x}, Offset: {:x}", Lit.MoveABI.GuestRIPLiteral.GuestEntryOffset, Lit.MoveABI.GuestRIPLiteral.Offset);
      Relocations << std::move(Lit.MoveABI.GuestRIPLiteral);
      break;
    default:
      ERROR_AND_DIE_FMT("PlaceRelocatedLiteral: Invalid value in Lit.MoveABI.Header.Type");
  }

  dq(Value);
}


void X86JITCore::InsertGuestRIPMove(Xbyak::Reg Reg, const uint64_t GuestRIP) {
  auto CurrentCursor = getSize();

  if (CTX->Config.ObjCache()) {
    LoadConstantWithPadding(Reg, GuestRIP);
  } else {
    mov(Reg, GuestRIP);
  }

  Relocations << RelocGuestRIPMove {
    .Offset = CurrentCursor - CursorEntry,
    .GuestEntryOffset = GuestRIP - Entry,
    .RegisterIndex = static_cast<uint8_t>(Reg.getIdx())
  };
}

void *X86JITCore::RelocateJITObjectCode(uint64_t Entry, const ObjCacheFragment *const HostCode, const ObjCacheRelocations *const Relocations) {
  AOTLOG("Relocating RIP 0x{:x}", Entry);

  if ((getSize() + HostCode->Bytes) > CurrentCodeBuffer->Size) {
    ThreadState->CTX->ClearCodeCache(ThreadState);
  }

  auto CursorBegin = getSize();
	auto HostEntry = getCurr<uint64_t>();
  AOTLOG("RIP Entry: disas 0x{:x},+{}", (uintptr_t)HostEntry, HostCode->Bytes);

  // Forward the cursor
  setSize(CursorBegin + HostCode->Bytes);

  memcpy(reinterpret_cast<void*>(HostEntry), HostCode->Code, HostCode->Bytes);

  // Relocation apply messes with the cursor
  // Save the cursor and restore at the end
  auto NewCursor = getSize();
  bool Result = ApplyRelocations(Entry, HostEntry, CursorBegin, Relocations);

  if (!Result) {
    // Reset cursor to the start
    setSize(CursorBegin);
    return nullptr;
  }

  // We've moved the cursor around with relocations. Move it back to where we were before relocations
  setSize(NewCursor);

  ready();

  this->IR = nullptr;

  //AOTLOG("\tRelocated JIT at [0x{:x}, 0x{:x}): RIP 0x{:x}", (uint64_t)HostEntry, CodeEnd, Entry);
  return reinterpret_cast<void*>(HostEntry);
}

bool X86JITCore::ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, const ObjCacheRelocations *const Relocations) {
  auto Begin = (const FEXCore::CPU::Relocation *)(Relocations->Relocations);
  auto End = (const FEXCore::CPU::Relocation *)(Relocations->Relocations + Relocations->Bytes);

  for (auto Reloc = Begin; Reloc < End; ) {
    switch (Reloc->Header.Type) {
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        uint64_t Pointer = GetNamedSymbolLiteral(Reloc->NamedSymbolLiteral.Symbol);
        
        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->NamedSymbolLiteral.Offset);

        // Place the pointer
        dq(Pointer);

        Reloc = Reloc->NamedSymbolLiteral.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
        uint64_t Data = GuestEntry + Reloc->GuestRIPLiteral.GuestEntryOffset;

        setSize(CursorEntry + Reloc->GuestRIPLiteral.Offset);
        dq(Data);
        AOTLOG("RELOC_GUEST_RIP_LITERAL: Offset: {:x} GuestEntry: {:x}, GuestEntryOffset: {:x}, Data: {:x}", Reloc->GuestRIPLiteral.Offset, GuestEntry, Reloc->GuestRIPLiteral.GuestEntryOffset, Data);

        Reloc = Reloc->GuestRIPLiteral.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        uint64_t Pointer = reinterpret_cast<uint64_t>(CTX->ThunkHandler->LookupThunk(Reloc->NamedThunkMove.Symbol));

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->NamedThunkMove.Offset);
        LoadConstantWithPadding(Xbyak::Reg64(Reloc->NamedThunkMove.RegisterIndex), Pointer);
        
        
        Reloc = Reloc->NamedThunkMove.Next();
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
        uint64_t Data = GuestEntry + Reloc->GuestRIPMove.GuestEntryOffset;

        // Relocation occurs at the cursorEntry + offset relative to that cursor.
        setSize(CursorEntry + Reloc->GuestRIPMove.Offset);
        LoadConstantWithPadding(Xbyak::Reg64(Reloc->GuestRIPMove.RegisterIndex), Data);
        
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

