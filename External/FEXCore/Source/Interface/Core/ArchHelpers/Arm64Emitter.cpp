#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "FEXCore/IR/IR.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Context/Context.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include "aarch64/cpu-aarch64.h"
#include "cpu-features.h"
#include "aarch64/instructions-aarch64.h"
#include "utils-vixl.h"

#include <tuple>

namespace FEXCore::CPU {
#define STATE x28

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(FEXCore::Context::Context *ctx, size_t size)
  : vixl::aarch64::Assembler(size, vixl::aarch64::PositionDependentCode)
  , EmitterCTX {ctx} {
  CPU.SetUp();

  auto Features = vixl::CPUFeatures::InferFromOS();
  if (ctx->HostFeatures.SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }

  SetCPUFeatures(Features);
}

void Arm64Emitter::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant, bool NOPPad) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant)>> 16) == 0) {
    movn(Reg, (~Constant) & 0xFFFF);

    if (NOPPad) {
      nop(); nop(); nop();
    }
    return;
  }

  int NumMoves = 1;
  movz(Reg, (Constant) & 0xFFFF, 0);
  for (int i = 1; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part) {
      movk(Reg, Part, i * 16);
      ++NumMoves;
    }
  }

  if (NOPPad) {
    for (int i = NumMoves; i < Segments; ++i) {
      nop();
    }
  }
}

void Arm64Emitter::PushCalleeSavedRegisters() {
  // We need to save pairs of registers
  // We save r19-r30
  MemOperand PairOffset(sp, -16, PreIndex);
  const std::array<std::pair<vixl::aarch64::Register, vixl::aarch64::Register>, 6> CalleeSaved = {{
    {x19, x20},
    {x21, x22},
    {x23, x24},
    {x25, x26},
    {x27, x28},
    {x29, x30},
  }};

  for (auto &RegPair : CalleeSaved) {
    stp(RegPair.first, RegPair.second, PairOffset);
  }

  // Additionally we need to store the lower 64bits of v8-v15
  // Here's a fun thing, we can use two ST4 instructions to store everything
  // We just need a single sub to sp before that
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v8, v9, v10, v11},
    {v12, v13, v14, v15},
  }};

  uint32_t VectorSaveSize = sizeof(uint64_t) * 8;
  sub(sp, sp, VectorSaveSize);
  // SP supporting move
  // We just saved x19 so it is safe
  add(x19, sp, 0);

  MemOperand QuadOffset(x19, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    st4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }
}

void Arm64Emitter::PopCalleeSavedRegisters() {
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v12, v13, v14, v15},
    {v8, v9, v10, v11},
  }};

  MemOperand QuadOffset(sp, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    ld4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }

  MemOperand PairOffset(sp, 16, PostIndex);
  const std::array<std::pair<vixl::aarch64::Register, vixl::aarch64::Register>, 6> CalleeSaved = {{
    {x29, x30},
    {x27, x28},
    {x25, x26},
    {x23, x24},
    {x21, x22},
    {x19, x20},
  }};

  for (auto &RegPair : CalleeSaved) {
    ldp(RegPair.first, RegPair.second, PairOffset);
  }
}


void Arm64Emitter::SpillStaticRegs(bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
      auto Reg1 = SRA64[i];
      auto Reg2 = SRA64[i+1];
      if (((1U << Reg1.GetCode()) & GPRSpillMask) &&
          ((1U << Reg2.GetCode()) & GPRSpillMask)) {
        stp(Reg1, Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg1.GetCode()) & GPRSpillMask)) {
        str(Reg1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg2.GetCode()) & GPRSpillMask)) {
        str(Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1])));
      }
    }

    if (FPRs) {
      for (size_t i = 0; i < SRAFPR.size(); i+=2) {
        auto Reg1 = SRAFPR[i];
        auto Reg2 = SRAFPR[i+1];

        if (((1U << Reg1.GetCode()) & FPRSpillMask) &&
            ((1U << Reg2.GetCode()) & FPRSpillMask)) {
          stp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg1.GetCode()) & FPRSpillMask)) {
          str(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg2.GetCode()) & FPRSpillMask)) {
          str(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i+1][0])));
        }
      }
    }
  }
}

void Arm64Emitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask) {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
      auto Reg1 = SRA64[i];
      auto Reg2 = SRA64[i+1];
      if (((1U << Reg1.GetCode()) & GPRFillMask) &&
          ((1U << Reg2.GetCode()) & GPRFillMask)) {
        ldp(Reg1, Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg1.GetCode()) & GPRFillMask)) {
        ldr(Reg1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg2.GetCode()) & GPRFillMask)) {
        ldr(Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1])));
      }
    }

    if (FPRs) {
      for (size_t i = 0; i < SRAFPR.size(); i+=2) {
        auto Reg1 = SRAFPR[i];
        auto Reg2 = SRAFPR[i+1];

        if (((1U << Reg1.GetCode()) & FPRFillMask) &&
            ((1U << Reg2.GetCode()) & FPRFillMask)) {
          ldp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg1.GetCode()) & FPRFillMask)) {
          ldr(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg2.GetCode()) & FPRFillMask)) {
          ldr(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i+1][0])));
        }
      }
    }
  }
}

void Arm64Emitter::PushDynamicRegsAndLR() {
  uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8 + RAFPR.size() * 16, 16);

  sub(sp, sp, SPOffset);
  int i = 0;

  for (auto RA : RAFPR)
  {
    str(RA.Q(), MemOperand(sp, i * 8));
    i+=2;
  }

#if 0 // All GPRs should be caller saved
  for (auto RA : RA64)
  {
    str(RA, MemOperand(sp, i * 8));
    i++;
  }
#endif

  str(lr, MemOperand(sp, i * 8));
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8 + RAFPR.size() * 16, 16);
  int i = 0;

  for (auto RA : RAFPR)
  {
    ldr(RA.Q(), MemOperand(sp, i * 8));
    i+=2;
  }

#if 0 // All GPRs should be caller saved
  for (auto RA : RA64)
  {
    ldr(RA, MemOperand(sp, i * 8));
    i++;
  }
#endif

  ldr(lr, MemOperand(sp, i * 8));

  add(sp, sp, SPOffset);
}

void Arm64Emitter::ResetStack() {
  if (SpillSlots == 0)
    return;

  if (IsImmAddSub(SpillSlots * 16)) {
    add(sp, sp, SpillSlots * 16);
  } else {
   // Too big to fit in a 12bit immediate
   LoadConstant(x0, SpillSlots * 16);
   add(sp, sp, x0);
  }
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
    nop();
  }
}

uint64_t Arm64Emitter::GetNamedSymbol(FEXCore::CPU::RelocNamedSymbolMove::NamedSymbol Op) {
  // AOTLOG("NamedSymbol Op: {}", Op);
  switch (Op) {
  }
  return 0;
}

void Arm64Emitter::InsertNamedSymbolRelocation(FEXCore::CPU::RelocNamedSymbolMove::NamedSymbol Op, vixl::aarch64::Register Reg) {
  uint64_t Pointer{};
  Relocation MoveABI{};
  MoveABI.NamedSymbolMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  MoveABI.NamedSymbolMove.Offset = CurrentCursor - GuestEntry;
  MoveABI.NamedSymbolMove.Symbol = Op;
  MoveABI.NamedSymbolMove.RegisterIndex = Reg.GetCode();

  Pointer = GetNamedSymbol(Op);

  LoadConstant(Reg, Pointer, CacheCodeCompilation());
  Relocations.emplace_back(MoveABI);
}

void Arm64Emitter::InsertNamedThunkRelocation(const IR::SHA256Sum &Sum, vixl::aarch64::Register Reg) {
  uint64_t Pointer{};
  Relocation MoveABI{};
  MoveABI.NamedThunkMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE;
  MoveABI.NamedThunkMove.Symbol = Sum;
  MoveABI.NamedThunkMove.RegisterIndex = Reg.GetCode();

  Pointer = reinterpret_cast<uint64_t>(EmitterCTX->ThunkHandler->LookupThunk(Sum));

  LoadConstant(Reg, Pointer, CacheCodeCompilation());
  Relocations.emplace_back(MoveABI);
}

Arm64Emitter::NamedSymbolLiteralPair Arm64Emitter::InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op) {
  uint64_t Pointer{};
  switch (Op) {
    case FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol::SYMBOL_LITERAL_EXITFUNCTION_LINKER:
      Pointer = ThreadSharedData.Dispatcher->ExitFunctionLinkerAddress;
    break;
  }

  Arm64Emitter::NamedSymbolLiteralPair Lit {
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

void Arm64Emitter::PlaceNamedSymbolLiteral(NamedSymbolLiteralPair &Lit) {
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  Lit.MoveABI.NamedSymbolLiteral.Offset = CurrentCursor - GuestEntry;

  place(&Lit.Lit);
  Relocations.emplace_back(Lit.MoveABI);
}

void Arm64Emitter::InsertGuestRIPMove(vixl::aarch64::Register Reg, uint64_t Constant) {
  Relocation MoveABI{};
  MoveABI.GuestRIPMove.Header.Type = FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE;
  // Offset is the offset from the entrypoint of the block
  auto CurrentCursor = GetCursorAddress<uint64_t>();
  MoveABI.GuestRIPMove.Offset = CurrentCursor - GuestEntry;
  MoveABI.GuestRIPMove.GuestRIP = Constant;
  MoveABI.GuestRIPMove.RegisterIndex = Reg.GetCode();

  LoadConstant(Reg, Constant, CacheCodeCompilation());
  Relocations.emplace_back(MoveABI);
}

bool Arm64Emitter::ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations) {
  //AOTLOG("\tWe have {} relocations", EntryRelocations.size());

  size_t DataIndex{};
  for (size_t j = 0; j < NumRelocations; ++j) {
    const FEXCore::CPU::Relocation *Reloc = reinterpret_cast<const FEXCore::CPU::Relocation *>(&EntryRelocations[DataIndex]);

    switch (Reloc->Header.Type) {
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_MOVE: {
        uint64_t Pointer = GetNamedSymbol(Reloc->NamedSymbolMove.Symbol);

        // Relocation occurs at the cursorEntry + offset relative to that cursor
        GetBuffer()->SetCursorOffset(CursorEntry + Reloc->NamedSymbolMove.Offset);
        //AOTLOG("\t\tRelocation at {}", GetCursorAddress<void*>());
        LoadConstant(vixl::aarch64::XRegister(Reloc->NamedSymbolMove.RegisterIndex), Pointer, true);
        DataIndex += sizeof(Reloc->NamedSymbolMove);
        break;
      }
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
        ERROR_AND_DIE_FMT("2");
        DataIndex += sizeof(Reloc->NamedSymbolLiteral);
        break;
      case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE:
        ERROR_AND_DIE_FMT("3");
        DataIndex += sizeof(Reloc->NamedThunkMove);

        break;
      case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
        uint64_t Pointer = EmitterCTX->AOTService->FindRelocatedRIP(Reloc->GuestRIPMove.GuestRIP);
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

  return true;
}

}
