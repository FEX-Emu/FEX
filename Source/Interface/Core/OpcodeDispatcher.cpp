

#include "Interface/Core/OpcodeDispatcher.h"
#include <FEXCore/Core/CoreState.h>
#include <climits>
#include <cstddef>
#include <cstdint>

#include <FEXCore/Core/X86Enums.h>

namespace FEXCore::IR {

auto OpToIndex = [](uint8_t Op) constexpr -> uint8_t {
  switch (Op) {
  // Group 1
  case 0x80: return 0;
  case 0x81: return 1;
  case 0x82: return 2;
  case 0x83: return 3;
  // Group 2
  case 0xC0: return 0;
  case 0xC1: return 1;
  case 0xD0: return 2;
  case 0xD1: return 3;
  case 0xD2: return 4;
  case 0xD3: return 5;
  // Group 3
  case 0xF6: return 0;
  case 0xF7: return 1;
  // Group 4
  case 0xFE: return 0;
  // Group 5
  case 0xFF: return 0;
  // Group 11
  case 0xC6: return 0;
  case 0xC7: return 1;
  }
  return 0;
};

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::SyscallOp(OpcodeArgs) {
  constexpr size_t SyscallArgs = 7;
  constexpr std::array<uint64_t, SyscallArgs> GPRIndexes = {
    FEXCore::X86State::REG_RAX,
    FEXCore::X86State::REG_RDI,
    FEXCore::X86State::REG_RSI,
    FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_R10,
    FEXCore::X86State::REG_R8,
    FEXCore::X86State::REG_R9,
  };

  auto SyscallOp = _Syscall(
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[0] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[1] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[2] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[3] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[4] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[5] * 8),
    _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes[6] * 8));

  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), SyscallOp);
}

void OpDispatchBuilder::LEAOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags, false);
  StoreResult(Op, Src);
}

void OpDispatchBuilder::NOPOp(OpcodeArgs) {
}

void OpDispatchBuilder::RETOp(OpcodeArgs) {
  auto Constant = _Constant(8);

  auto OldSP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]));

  auto NewRIP = _LoadMem(8, OldSP);

  OrderedNode *NewSP;
  if (Op->OP == 0xC2) {
    auto Offset = LoadSource(Op, Op->Src1, Op->Flags);
    NewSP = _Add(_Add(OldSP, Constant), Offset);
  }
  else {
    NewSP = _Add(OldSP, Constant);
  }

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store the new RIP
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  _EndFunction();
  Information.HadUnconditionalExit = true;
}

void OpDispatchBuilder::SecondaryALUOp(OpcodeArgs) {
  FEXCore::IR::IROps IROp;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0):
    IROp = FEXCore::IR::IROps::OP_ADD;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1):
    IROp = FEXCore::IR::IROps::OP_OR;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4):
    IROp = FEXCore::IR::IROps::OP_AND;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5):
    IROp = FEXCore::IR::IROps::OP_SUB;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 5):
    IROp = FEXCore::IR::IROps::OP_MUL;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6):
    IROp = FEXCore::IR::IROps::OP_XOR;
  break;
  default:
    IROp = FEXCore::IR::IROps::OP_LAST;
    LogMan::Msg::A("Unknown ALU Op: 0x%x", Op->OP);
  break;
  };
#undef OPD
  // X86 basic ALU ops just do the operation between the destination and a single source
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _Add(Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(Op, ALUOp);

  // Flags set
  {
    auto Size = GetSrcSize(Op) * 8;
    switch (IROp) {
    case FEXCore::IR::IROps::OP_ADD:
      GenerateFlags_ADD(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_SUB:
      GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_MUL:
      GenerateFlags_MUL(Op, _Bfe(Size, 0, ALUOp), _MulH(Dest, Src));
    break;
    case FEXCore::IR::IROps::OP_AND:
    case FEXCore::IR::IROps::OP_XOR:
    case FEXCore::IR::IROps::OP_OR: {
      GenerateFlags_Logical(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    }
    default: break;
    }
  }
}

void OpDispatchBuilder::ADCOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto ALUOp = _Add(_Add(Dest, Src), CF);

  StoreResult(Op, ALUOp);
  GenerateFlags_ADC(Op, ALUOp, Dest, Src, CF);
}

void OpDispatchBuilder::SBBOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto ALUOp = _Sub(_Sub(Dest, Src), CF);
  StoreResult(Op, ALUOp);
  GenerateFlags_SBB(Op, ALUOp, Dest, Src, CF);
}

void OpDispatchBuilder::PUSHOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]));

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  OrderedNode *Src;
  if (Op->OP == 0x68 || Op->OP == 0x6A) { // Immediate Push
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }
  else {
    if (Op->OP == 0xFF && Size == 4) LogMan::Msg::A("Woops. Can't do 32bit for this PUSH op");
    Src = LoadSource(Op, Op->Dest, Op->Flags);
  }

  // Store our value to the new stack location
  _StoreMem(Size, NewSP, Src);
}

void OpDispatchBuilder::POPOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]));

  auto NewGPR = _LoadMem(Size, OldSP);

  auto NewSP = _Add(OldSP, Constant);

  if (Op->OP == 0x8F && Size == 4) LogMan::Msg::A("Woops. Can't do 32bit for this POP op");

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded from the stack
  StoreResult(Op, NewGPR);
}

void OpDispatchBuilder::LEAVEOp(OpcodeArgs) {
  // First we move RBP in to RSP and then behave effectively like a pop
  uint8_t Size = GetSrcSize(Op);
  auto Constant = _Constant(Size);

  LogMan::Throw::A(Size == 8, "Can't handle a LEAVE op with size %d", Size);

  auto OldBP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]));

  auto NewGPR = _LoadMem(Size, OldBP);

  auto NewSP = _Add(OldBP, Constant);

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded to RBP
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), NewGPR);
}

void OpDispatchBuilder::CALLOp(OpcodeArgs) {
  auto ConstantPC = _Constant(Op->PC + Op->InstSize);

  OrderedNode *JMPPCOffset = LoadSource(Op, Op->Src1, Op->Flags);

  auto NewRIP = _Add(JMPPCOffset, ConstantPC);

  auto ConstantPCReturn = _Constant(Op->PC + Op->InstSize);

  auto ConstantSize = _Constant(8);
  auto OldSP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]));

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(8, NewSP, ConstantPCReturn);

  // Store the RIP
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  _ExitFunction(); // If we get here then leave the function now

  // Fracking RIPSetter check ending the block causes issues
  // Split the block and leave early to work around the bug
  _EndBlock(0);
  // Make sure to start a new block after ending this one
  _BeginBlock();
  Information.HadUnconditionalExit = true;
}

void OpDispatchBuilder::CALLAbsoluteOp(OpcodeArgs) {
  OrderedNode *JMPPCOffset = LoadSource(Op, Op->Src1, Op->Flags);

  auto ConstantPCReturn = _Constant(Op->PC + Op->InstSize);

  auto ConstantSize = _Constant(8);
  auto OldSP = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]));

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(8, NewSP, ConstantPCReturn);

  // Store the RIP
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), JMPPCOffset);
  _ExitFunction(); // If we get here then leave the function now

  // Fracking RIPSetter check ending the block causes issues
  // Split the block and leave early to work around the bug
  _EndBlock(0);
  // Make sure to start a new block after ending this one
  _BeginBlock();
  Information.HadUnconditionalExit = true;

}

void OpDispatchBuilder::CondJUMPOp(OpcodeArgs) {
  enum CompareType {
    COMPARE_ZERO,
    COMPARE_NOTZERO,
    COMPARE_EQUALMASK,
    COMPARE_OTHER,
  };
  uint32_t FLAGMask;
  CompareType Type = COMPARE_OTHER;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  IRPair<IROp_Header> SrcCond;

  switch (Op->OP) {
  case 0x70:
  case 0x80:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x71:
  case 0x81:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x72:
  case 0x82:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x73:
  case 0x83:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x74:
  case 0x84:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x75:
  case 0x85:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x76:
  case 0x86: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
    auto Check = _Or(Flag1, Flag2);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, OneConst, ZeroConst, OneConst);
  break;
  }
  case 0x77:
  case 0x87: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
    auto Check = _Or(Flag1, _Lshl(Flag2, _Constant(1)));
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, ZeroConst, ZeroConst, OneConst);
  break;
  }
  case 0x78:
  case 0x88:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x79:
  case 0x89:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x7A:
  case 0x8A:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x7B:
  case 0x8B:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x7C: // SF <> OF
  case 0x8C: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_NEQ,
        Flag1, Flag2, ZeroConst, OneConst);
  break;
  }
  case 0x7D: // SF = OF
  case 0x8D: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Flag1, Flag2, ZeroConst, OneConst);
  break;
  }
  case 0x7E: // ZF = 1 || SF <> OF
  case 0x8E: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, OneConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_NEQ,
        Flag2, Flag3, OneConst, ZeroConst);

    auto Check = _Or(Select1, Select2);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, OneConst, ZeroConst, OneConst);
  break;
  }
  case 0x7F: // ZF = 0 && SF = OF
  case 0x8F: {
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, ZeroConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_EQ,
        Flag2, Flag3, OneConst, ZeroConst);

    auto Check = _And(Select1, Select2);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, OneConst, ZeroConst, OneConst);
  break;
  }
  default: LogMan::Msg::A("Unknown Jmp Op: 0x%x\n", Op->OP); return;
  }

  if (Type != COMPARE_OTHER) {
    auto MaskConst = _Constant(FLAGMask);

    auto RFLAG = GetPackedRFLAG(false);

    auto AndOp = _And(RFLAG, MaskConst);

    switch (Type) {
    case COMPARE_ZERO: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, ZeroConst, ZeroConst, OneConst);
    break;
    }
    case COMPARE_NOTZERO: {
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          AndOp, ZeroConst, ZeroConst, OneConst);
    break;
    }
    case COMPARE_EQUALMASK: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, MaskConst, ZeroConst, OneConst);
    break;
    }
    case COMPARE_OTHER: break;
    }
  }

  // The conditions of the previous conditional branches are inverted from what you expect on the x86 side
  // This inversion exists because our condjump needs to jump over code that sets the RIP to the target conditionally
  // XXX: Reenable
#if 0
  if (ConfigMultiblock()) {
    auto CondJump = _CondJump();
    CondJump.first->Header.NumArgs = 1;
    CondJump.first->Cond = SrcCond;

    _EndBlock(0);
    // Make sure to start a new block after ending this one
    _BeginBlock();

    uint64_t Target = Op->PC + Op->InstSize + Op->Src1.TypeLiteral.Literal;
    if (false && Target > Op->PC) {
      // If we are forward jumping: Add the IR Op to the fixup list
      auto it = Arguments.Fixups.find(Target);
      if (it == Arguments.Fixups.end()) {
        std::vector<IRArguments::Fixup> empty;
        it = Arguments.Fixups.emplace(std::make_pair(Target, empty)).first;
      }
      it->second.emplace_back(IRArguments::Fixup{&CondJump.first->Header});
      return;
    }
    else if (false && Target <= Op->PC) {
      // If we are jumping backwards then we should have a jump target available in our jump targets list
      auto it = Arguments.JumpTargets.find(Target);
      if (it != Arguments.JumpTargets.end()) {
        CondJump.first->Location = it->second;
        return;
      }
    }
  }
#endif
  // Fallback
  {
    // XXX: Test
    GetPackedRFLAG(false);

    auto CondJump = _CondJump(SrcCond);

    auto RIPOffset = LoadSource(Op, Op->Src1, Op->Flags);
    auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

		auto NewRIP = _Add(RIPOffset, RIPTargetConst);

    // Store the new RIP
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
    _ExitFunction();

    _EndBlock(0);

    // Make sure to start a new block after ending this one
    auto JumpTarget = _BeginBlock();
    // This very explicitly avoids the isDest path for Ops. We want the actual destination here
    SetJumpTarget(CondJump, JumpTarget);
  }
}

void OpDispatchBuilder::JUMPOp(OpcodeArgs) {
  // This is just an unconditional relative literal jump
  // XXX: Reenable
#if 0
  if (ConfigMultiblock()) {
    uint64_t Target = Op->PC + Op->InstSize + Op->Src1.TypeLiteral.Literal;
    if (false && Target > Op->PC) {
      // If we are forward jumping: Add the IR Op to the fixup list
      auto it = Arguments.Fixups.find(Target);
      if (it == Arguments.Fixups.end()) {
        std::vector<IRArguments::Fixup> empty;
        it = Arguments.Fixups.emplace(std::make_pair(Target, empty)).first;
      }
      auto Jump = _Jump();
      it->second.emplace_back(IRArguments::Fixup{&Jump.first->Header});
      return;
    }
    else if (Target <= Op->PC) {
      // If we are jumping backwards then we should have a jump target available in our jump targets list
      auto it = Arguments.JumpTargets.find(Target);

      if (it != Arguments.JumpTargets.end()) {
        auto Jump = _Jump();
        Jump.first->Location = it->second;
        return;
      }
    }
  }
#endif

  // Fallback
  {
    // This source is a literal
    auto RIPOffset = LoadSource(Op, Op->Src1, Op->Flags);

    auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

		auto NewRIP = _Add(RIPOffset, RIPTargetConst);

    // Store the new RIP
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
    _ExitFunction();

    _EndBlock(0);

    // Make sure to start a new block after ending this one
    _BeginBlock();
    Information.HadUnconditionalExit = true;
  }
}

void OpDispatchBuilder::JUMPAbsoluteOp(OpcodeArgs) {
  // This is just an unconditional jump
  // This uses ModRM to determine its location
  // No way to use this effectively in multiblock
  auto RIPOffset = LoadSource(Op, Op->Src1, Op->Flags);

  // Store the new RIP
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), RIPOffset);
  _ExitFunction();

  _EndBlock(0);

  // Make sure to start a new block after ending this one
  _BeginBlock();
  Information.HadUnconditionalExit = true;

}

void OpDispatchBuilder::SETccOp(OpcodeArgs) {
  enum CompareType {
    COMPARE_ZERO,
    COMPARE_NOTZERO,
    COMPARE_EQUALMASK,
    COMPARE_OTHER,
  };
  uint32_t FLAGMask;
  CompareType Type = COMPARE_OTHER;
  OrderedNode *SrcCond;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  switch (Op->OP) {
  case 0x90:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x91:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x92:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x93:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x94:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x95:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x96:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_NOTZERO;
  break;
  case 0x97:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_ZERO;
  break;
  case 0x98:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x99:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x9A:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x9B:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x9D: { // SF = OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Flag1, Flag2, OneConst, ZeroConst);
  break;
  }
  case 0x9C: { // SF <> OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_NEQ,
        Flag1, Flag2, OneConst, ZeroConst);
  break;
  }
  case 0x9E: { // ZF = 1 || SF <> OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, OneConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_NEQ,
        Flag2, Flag3, OneConst, ZeroConst);
    SrcCond = _Or(Select1, Select2);
  break;
  }
  case 0x9F: { // ZF = 0 && SF = OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, ZeroConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_EQ,
        Flag2, Flag3, OneConst, ZeroConst);
    SrcCond = _And(Select1, Select2);
  break;
  }
  default:
    LogMan::Msg::A("Unhandled SetCC op: 0x%x", Op->OP);
  break;
  }

  if (Type != COMPARE_OTHER) {
    auto MaskConst = _Constant(FLAGMask);

    auto RFLAG = GetPackedRFLAG(false);

    auto AndOp = _And(RFLAG, MaskConst);

    switch (Type) {
    case COMPARE_ZERO: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, ZeroConst, OneConst, ZeroConst);
    break;
    }
    case COMPARE_NOTZERO: {
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          AndOp, ZeroConst, OneConst, ZeroConst);
    break;
    }
    case COMPARE_EQUALMASK: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, MaskConst, OneConst, ZeroConst);
    break;
    }
    case COMPARE_OTHER: break;
    }
  }

  StoreResult(Op, SrcCond);
}


void OpDispatchBuilder::TESTOp(OpcodeArgs) {
  // TEST is an instruction that does an AND between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _And(Dest, Src);
  auto Size = GetSrcSize(Op) * 8;
  GenerateFlags_Logical(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::MOVSXDOp(OpcodeArgs) {
  // This instruction is a bit special
  // if SrcSize == 2
  //  Then lower 16 bits of destination is written without changing the upper 48 bits
  // else /* Size == 4 */
  //  if REX_WIDENING:
  //   Sext(32, Src)
  //  else
  //   Zext(32, Src)
  //
  uint8_t Size = std::min(static_cast<uint8_t>(4), GetSrcSize(Op));

  OrderedNode *Src = LoadSource_WithOpSize(Op, Op->Src1, Size, Op->Flags);
  if (Size == 2) {
    // This'll make sure to insert in to the lower 16bits without modifying upper bits
    StoreResult_WithOpSize(Op, Op->Dest, Src, Size);
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // With REX.W then Sext
    Src = _Sext(Size * 8, Src);
    StoreResult(Op, Src);
  }
  else {
    // Without REX.W then Zext
    Src = _Zext(Size * 8, Src);
    StoreResult(Op, Src);
  }
}

void OpDispatchBuilder::MOVSXOp(OpcodeArgs) {
  // This will ZExt the loaded size
  // We want to Sext it
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  Src = _Sext(Size * 8, Src);
  StoreResult(Op, Op->Dest, Src);
}

void OpDispatchBuilder::MOVZXOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  // Just make sure this is zero extended
  Src = _Zext(Size * 8, Src);
  StoreResult(Op, Src);
}

void OpDispatchBuilder::CMPOp(OpcodeArgs) {
  // CMP is an instruction that does a SUB between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetDstSize(Op) * 8;
  auto ALUOp = _Sub(Dest, Src);
  GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::CQOOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  auto BfeOp = _Bfe(1, GetSrcSize(Op) * 8 - 1, Src);
  auto ZeroConst = _Constant(0);
  auto MaxConst = _Constant(~0ULL);
  auto SelectOp = _Select(FEXCore::IR::COND_EQ, BfeOp, ZeroConst, ZeroConst, MaxConst);

  StoreResult(Op, SelectOp);
}

void OpDispatchBuilder::XCHGOp(OpcodeArgs) {
  // Load both the source and the destination
  if (Op->OP == 0x90 &&
      GetSrcSize(Op) >= 4 &&
      Op->Src1.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Src1.TypeGPR.GPR == FEXCore::X86State::REG_RAX &&
      Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Dest.TypeGPR.GPR == FEXCore::X86State::REG_RAX) {
    // This is one heck of a sucky special case
    // If we are the 0x90 XCHG opcode (Meaning source is GPR RAX)
    // and destination register is ALSO RAX
    // and in this very specific case we are 32bit or above
    // Then this is a no-op
    // This is because 0x90 without a prefix is technically `xchg eax, eax`
    // But this would result in a zext on 64bit, which would ruin the no-op nature of the instruction
    // So x86-64 spec mandates this special case that even though it is a 32bit instruction and
    // is supposed to zext the result, it is a true no-op
    return;
  }

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  // Swap the contents
  // Order matters here since we don't want to swap context contents for one that effects the other
  StoreResult(Op, Op->Dest, Src);
  StoreResult(Op, Op->Src1, Dest);
}

void OpDispatchBuilder::CDQOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  // Op size is destination size
  // Therefore sext OpSize/2
  uint8_t SrcSize = GetSrcSize(Op) / 2;
  Src = _Sext(SrcSize * 8, Src);
  if (SrcSize == 4)
    Src = _Zext(SrcSize * 2 * 8, Src);

  StoreResult_WithOpSize(Op, Op->Dest, Src, SrcSize * 2 * 8);
}

void OpDispatchBuilder::SAHFOp(OpcodeArgs) {
  OrderedNode *Src = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1);

  // Clear bits that aren't supposed to be set
  Src = _And(Src, _Constant(~0b101000));

  // Set the bit that is always set here
  Src = _Or(Src, _Constant(0b10));

  // Store the lower 8 bits in to RFLAGS
  SetPackedRFLAG(true, Src);
}
void OpDispatchBuilder::LAHFOp(OpcodeArgs) {
  // Load the lower 8 bits of the Rflags register
    auto RFLAG = GetPackedRFLAG(true);

  // Store the lower 8 bits of the rflags register in to AH
  _StoreContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, RFLAG);
}

void OpDispatchBuilder::FLAGControlOp(OpcodeArgs) {
  enum OpType {
    OP_CLEAR,
    OP_SET,
    OP_COMPLEMENT,
  };
  OpType Type;
  uint64_t Flag;
  switch (Op->OP) {
  case 0xF5: // CMC
    Flag= FEXCore::X86State::RFLAG_CF_LOC;
    Type = OP_COMPLEMENT;
  break;
  case 0xF8: // CLC
    Flag= FEXCore::X86State::RFLAG_CF_LOC;
    Type = OP_CLEAR;
  break;
  case 0xF9: // STC
    Flag= FEXCore::X86State::RFLAG_CF_LOC;
    Type = OP_SET;
  break;
  case 0xFC: // CLD
    Flag= FEXCore::X86State::RFLAG_DF_LOC;
    Type = OP_CLEAR;
  break;
  case 0xFD: // STD
    Flag= FEXCore::X86State::RFLAG_DF_LOC;
    Type = OP_SET;
  break;
  }

  OrderedNode *Result{};
  switch (Type) {
  case OP_CLEAR: {
    Result = _Constant(0);
  break;
  }
  case OP_SET: {
    Result = _Constant(1);
  break;
  }
  case OP_COMPLEMENT: {
    auto RFLAG = GetRFLAG(Flag);
    Result = _Xor(RFLAG, _Constant(1));
  break;
  }
  }

  SetRFLAG(Result, Flag);
}

void OpDispatchBuilder::MOVSegOp(OpcodeArgs) {
  // In x86-64 mode the accesses to the segment registers end up being constant zero moves
  // Aside from FS/GS
  LogMan::Msg::A("Wanting reg: %d\n", Op->Src1.TypeGPR.GPR);
  //  StoreResult(Op, Src);
}

void OpDispatchBuilder::MOVOffsetOp(OpcodeArgs) {
  OrderedNode *Src;
  const FEXCore::X86Tables::DecodedOperand *Dest;

  switch (Op->OP) {
  case 0xA0:
  case 0xA1:
    // Source is memory(literal)
    // Dest is GPR
    Src = LoadSource(Op, Op->Src1, Op->Flags, true);
    Dest = &Op->Dest;
    break;
  case 0xA2:
  case 0xA3:
    // Source is GPR
    // Dest is memory(literal)
    Src = LoadSource(Op, Op->Src1, Op->Flags);
    Dest = &Op->Src2;
    break;
  }
  StoreResult(Op, *Dest, Src);
}

void OpDispatchBuilder::CMOVOp(OpcodeArgs) {
  enum CompareType {
    COMPARE_ZERO,
    COMPARE_NOTZERO,
    COMPARE_EQUALMASK,
    COMPARE_OTHER,
  };
  uint32_t FLAGMask;
  CompareType Type = COMPARE_OTHER;
  OrderedNode *SrcCond;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  switch (Op->OP) {
  case 0x40:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x41:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_OF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x42:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x43:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x44:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x45:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x46:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_NOTZERO;
  break;
  case 0x47:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_ZERO;
  break;
  case 0x48:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x49:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_SF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x4A:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x4B:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x4C: { // SF <> OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_NEQ,
        Flag1, Flag2, Src, Dest);
  break;
  }
  case 0x4D: { // SF = OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Flag1, Flag2, Src, Dest);
  break;
  }

  case 0x4E: { // ZF = 1 || SF <> OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, OneConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_NEQ,
        Flag2, Flag3, OneConst, ZeroConst);
    auto Check = _Or(Select1, Select2);

    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, OneConst, Src, Dest);
  break;
  }

  case 0x4F: { // ZF = 0 && SSF = OF
    auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
    auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    auto Select1 = _Select(FEXCore::IR::COND_EQ,
        Flag1, ZeroConst, OneConst, ZeroConst);

    auto Select2 = _Select(FEXCore::IR::COND_EQ,
        Flag2, Flag3, OneConst, ZeroConst);
    auto Check = _And(Select1, Select2);

    SrcCond = _Select(FEXCore::IR::COND_EQ,
        Check, OneConst, Src, Dest);
  break;
  }
  default:
    LogMan::Msg::A("Unhandled CMOV op: 0x%x", Op->OP);
  break;
  }

  if (Type != COMPARE_OTHER) {
    auto MaskConst = _Constant(FLAGMask);

    auto RFLAG = GetPackedRFLAG(false);

    auto AndOp = _And(RFLAG, MaskConst);

    switch (Type) {
    case COMPARE_ZERO: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, ZeroConst, Src, Dest);
    break;
    }
    case COMPARE_NOTZERO: {
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          AndOp, ZeroConst, Src, Dest);
    break;
    }
    case COMPARE_EQUALMASK: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          AndOp, MaskConst, Src, Dest);
    break;
    }

    case COMPARE_OTHER: break;
    }
  }

  StoreResult(Op, SrcCond);
}

void OpDispatchBuilder::CPUIDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  auto Res = _CPUID(Src);

  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), _ExtractElement(Res, 0));
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), _ExtractElement(Res, 1));
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), _ExtractElement(Res, 2));
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), _ExtractElement(Res, 3));
}
void OpDispatchBuilder::SHLOp(OpcodeArgs) {
  bool SHL1Bit = false;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4):
    SHL1Bit = true;
  break;
  }
#undef OPD
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  if (SHL1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(0x3F));
  else
    Src = _And(Src, _Constant(0x1F));

  auto ALUOp = _Lshl(Dest, Src);

  StoreResult(Op, ALUOp);

  // XXX: This isn't correct
  GenerateFlags_Shift(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::SHROp(OpcodeArgs) {
  bool SHR1Bit = false;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5):
    SHR1Bit = true;
  break;
  }
#undef OPD

  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  if (SHR1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(0x3F));
  else
    Src = _And(Src, _Constant(0x1F));

  auto ALUOp = _Lshr(Dest, Src);

  StoreResult(Op, ALUOp);

  // XXX: This isn't correct
  GenerateFlags_Logical(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));

  if (SHR1Bit) {
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, Size - 1, Src));
  }
}

void OpDispatchBuilder::ASHROp(OpcodeArgs) {
  bool SHR1Bit = false;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7):
    SHR1Bit = true;
  break;
  }
#undef OPD

  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op) * 8;

  if (SHR1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }


  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Ashr(Dest, Src);

  StoreResult(Op, ALUOp);

  // XXX: This isn't correct
  GenerateFlags_Logical(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));

  if (SHR1Bit) {
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, Size - 1, Src));
  }
}

void OpDispatchBuilder::ROROp(OpcodeArgs) {
  bool Is1Bit = false;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1):
    Is1Bit = true;
  break;
  }
#undef OPD

  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op) * 8;
  if (Is1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(Op, ALUOp);

  // XXX: This is incorrect
  GenerateFlags_Rotate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));

  if (Is1Bit) {
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, Size - 1, Src));
  }
}
void OpDispatchBuilder::ROLOp(OpcodeArgs) {
  bool Is1Bit = false;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0):
    Is1Bit = true;
  break;
  default: break;
  }
#undef OPD

  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op) * 8;

  if (Is1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(Op, Op->Src1, Op->Flags);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Rol(Dest, Src);

  StoreResult(Op, ALUOp);

  // XXX: This is incorrect
  GenerateFlags_Rotate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));

  if (Is1Bit) {
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, Size - 1, Src));
  }
}

void OpDispatchBuilder::BTOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
    Result = _Lshr(Dest, Src);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags, false);
    uint32_t Size = GetSrcSize(Op);
    uint32_t Mask = Size * 8 - 1;
    OrderedNode *SizeMask = _Constant(Mask);
    OrderedNode *AddressShift = _Constant(32 - __builtin_clz(Mask));

    // Get the bit selection from the src
    OrderedNode *BitSelect = _And(Src, SizeMask);

    // First shift out the selection bits
    Src = _Lshr(Src, AddressShift);

    // Now multiply by operand size to get correct indexing
    if (Size != 1) {
      Src = _Lshl(Src, _Constant(Size - 1));
    }

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    Result = _LoadMem(Size, MemoryLocation);

    // Now shift in to the correct bit location
    Result = _Lshr(Result, BitSelect);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

void OpDispatchBuilder::IMUL1SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = LoadSource(Op, Op->Src1, Op->Flags);

  auto Dest = _Mul(Src1, Src2);
  StoreResult(Op, Dest);
  GenerateFlags_MUL(Op, Dest, _MulH(Src1, Src2));
}

void OpDispatchBuilder::IMUL2SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Src2 = LoadSource(Op, Op->Src2, Op->Flags);

  auto Dest = _Mul(Src1, Src2);
  StoreResult(Op, Dest);
  GenerateFlags_MUL(Op, Dest, _MulH(Src1, Src2));
}

void OpDispatchBuilder::IMULOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));

  if (Size != 8) {
    Src1 = _Sext(Size * 8, Src1);
    Src2 = _Sext(Size * 8, Src2);
  }

  OrderedNode *Result = _Mul(Src1, Src2);
  OrderedNode *ResultHigh{};
  if (Size == 1) {
    // Result is stored in AX
    _StoreContext(2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(8, 8, Result);
    ResultHigh = _Sext(Size * 8, ResultHigh);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(16, 16, Result);
    ResultHigh = _Sext(Size * 8, ResultHigh);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    OrderedNode *ResultLow = _Bfe(32, 0, Result);
    ResultLow = _Zext(Size * 8, ResultLow);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ResultLow);
    ResultHigh = _Bfe(32, 32, Result);
    ResultHigh = _Zext(Size * 8, ResultHigh);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 8) {
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _MulH(Src1, Src2);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }

  GenerateFlags_MUL(Op, Result, ResultHigh);
}

void OpDispatchBuilder::MULOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
  if (Size != 8) {
    Src1 = _Zext(Size * 8, Src1);
    Src2 = _Zext(Size * 8, Src2);
  }
  OrderedNode *Result = _UMul(Src1, Src2);
  OrderedNode *ResultHigh{};

  if (Size == 1) {
   // Result is stored in AX
    _StoreContext(2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(8, 8, Result);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(16, 16, Result);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    OrderedNode *ResultLow = _Bfe(32, 0, Result);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ResultLow);
    ResultHigh = _Bfe(32, 32, Result);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);

  }
  else if (Size == 8) {
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _UMulH(Src1, Src2);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }

  GenerateFlags_UMUL(Op, ResultHigh);
}

void OpDispatchBuilder::NOTOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *MaskConst{};
  if (Size == 8) {
    MaskConst = _Constant(~0ULL);
  }
  else {
    MaskConst = _Constant((1ULL << (Size * 8)) - 1);
  }

  OrderedNode *Src = LoadSource(Op, Op->Dest, Op->Flags);
  Src = _Xor(Src, MaskConst);
  StoreResult(Op, Src);
}

void OpDispatchBuilder::RDTSCOp(OpcodeArgs) {
  auto Counter = _CycleCounter();
  auto CounterLow = _Bfe(32, 0, Counter);
  auto CounterHigh = _Bfe(32, 32, Counter);
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CounterLow);
  _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), CounterHigh);
}

void OpDispatchBuilder::INCOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::A("Can't handle REP on this\n");
  }

  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  auto OneConst = _Constant(1);
  auto ALUOp = _Add(Dest, OneConst);

  StoreResult(Op, ALUOp);

  auto Size = GetSrcSize(Op) * 8;
  GenerateFlags_ADD(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
}

void OpDispatchBuilder::DECOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::A("Can't handle REP on this\n");
  }

  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  auto OneConst = _Constant(1);
  auto ALUOp = _Sub(Dest, OneConst);

  StoreResult(Op, ALUOp);

  auto Size = GetSrcSize(Op) * 8;
  GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
}

void OpDispatchBuilder::STOSOp(OpcodeArgs) {
  if (!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX)) {
    LogMan::Msg::A("Can't handle REP not existing on STOS\n");
  }
  auto Size = GetSrcSize(Op);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  auto SizeConst = _Constant(Size);
  auto NegSizeConst = _Constant(-Size);

  auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
  auto PtrDir = _Select(FEXCore::IR::COND_EQ,
      DF, ZeroConst,
      SizeConst, NegSizeConst);

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  auto JumpStart = _Jump();
  _EndBlock(0);

    // Make sure to start a new block after ending this one
    auto LoopStart = _BeginBlock();
    SetJumpTarget(JumpStart, LoopStart);

    OrderedNode *Counter = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]));

    OrderedNode *Dest = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]));

    // Store to memory where RDI points
    _StoreMem(Size, Dest, Src);

    // Can we end the block?
    auto CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
      Counter, ZeroConst,
      OneConst, ZeroConst);
    auto CondJump = _CondJump(CanLeaveCond);

    // Decrement counter
    Counter = _Sub(Counter, OneConst);

    // Store the counter so we don't have to deal with PHI here
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), Counter);

    // Offset the pointer
    Dest = _Add(Dest, PtrDir);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest);

    // Jump back to the start, we have more work to do
    _Jump(LoopStart);
  _EndBlock(0);
  // Make sure to start a new block after ending this one
  auto LoopEnd = _BeginBlock();
  SetJumpTarget(CondJump, LoopEnd);
}

void OpDispatchBuilder::MOVSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::A("Can't handle REP\n");
  }

  _Break(0, 0);
}
void OpDispatchBuilder::CMPSOp(OpcodeArgs) {
  if (!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX)) {
    LogMan::Msg::A("Can't only handle REP\n");
  }

  auto Size = GetSrcSize(Op);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  auto SizeConst = _Constant(Size);
  auto NegSizeConst = _Constant(-Size);

  auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
  auto PtrDir = _Select(FEXCore::IR::COND_EQ,
      DF, ZeroConst,
      SizeConst, NegSizeConst);

  auto JumpStart = _Jump();
  _EndBlock(0);
  // Make sure to start a new block after ending this one
    auto LoopStart = _BeginBlock();
    SetJumpTarget(JumpStart, LoopStart);

    OrderedNode *Counter = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]));
    OrderedNode *Dest_RDI = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]));
    OrderedNode *Dest_RSI = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]));

    auto Src1 = _LoadMem(Size, Dest_RDI);
    auto Src2 = _LoadMem(Size, Dest_RSI);

    auto ALUOp = _Sub(Src1, Src2);
    GenerateFlags_SUB(Op, ALUOp, Src1, Src2);

    // Can we end the block?
    auto CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
      Counter, ZeroConst,
      OneConst, ZeroConst);
    auto CondJump = _CondJump(CanLeaveCond);

    // Decrement counter
    Counter = _Sub(Counter, OneConst);

    // Store the counter so we don't have to deal with PHI here
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), Counter);

    // Offset the pointer
    Dest_RDI = _Add(Dest_RDI, PtrDir);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest_RDI);

    // Offset second pointer
    Dest_RSI = _Add(Dest_RSI, PtrDir);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Dest_RSI);

    // Jump back to the start, we have more work to do
    _Jump(LoopStart);
  _EndBlock(0);
  // Make sure to start a new block after ending this one
  auto LoopEnd = _BeginBlock();
  SetJumpTarget(CondJump, LoopEnd);

}

void OpDispatchBuilder::BSWAPOp(OpcodeArgs) {
  OrderedNode *Dest;
  if (GetSrcSize(Op) == 2) {
    // BSWAP of 16bit is undef. ZEN+ causes the lower 16bits to get zero'd
    Dest = _Constant(0);
  }
  else {
    Dest = LoadSource(Op, Op->Dest, Op->Flags);
    Dest = _Rev(Dest);
  }
  StoreResult(Op, Dest);
}

void OpDispatchBuilder::NEGOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  auto ZeroConst = _Constant(0);
  auto ALUOp = _Sub(ZeroConst, Dest);

  StoreResult(Op, ALUOp);

  auto Size = GetSrcSize(Op) * 8;

  GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, ZeroConst), _Bfe(Size, 0, Dest));
}

void OpDispatchBuilder::DIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));

    auto UDivOp = _UDiv(Src1, Divisor);
    auto URemOp = _URem(Src1, Divisor);

    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, URemOp);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));
    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));

    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), _Zext(32, UDivOp));
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), _Zext(32, URemOp));
  }
  else if (Size == 8) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));

    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::IDIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));

    auto UDivOp = _Div(Src1, Divisor);
    auto URemOp = _Rem(Src1, Divisor);

    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, URemOp);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));
    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));

    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), _Zext(32, UDivOp));
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), _Zext(32, URemOp));
  }
  else if (Size == 8) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]));

    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::BSFOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  // Find the LSB of this source
  auto Result = _FindLSB(Src);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // If Src was zero then the destination doesn't get modified
  auto SelectOp = _Select(FEXCore::IR::COND_EQ,
      Src, ZeroConst,
      Dest, Result);

  // ZF is set to 1 if the source was zero
  auto ZFSelectOp = _Select(FEXCore::IR::COND_EQ,
      Src, ZeroConst,
      OneConst, ZeroConst);

  StoreResult(Op, SelectOp);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFSelectOp);
}

void OpDispatchBuilder::BSROp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  // Find the MSB of this source
  auto Result = _FindMSB(Src);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // If Src was zero then the destination doesn't get modified
  auto SelectOp = _Select(FEXCore::IR::COND_EQ,
      Src, ZeroConst,
      Dest, Result);

  // ZF is set to 1 if the source was zero
  auto ZFSelectOp = _Select(FEXCore::IR::COND_EQ,
      Src, ZeroConst,
      OneConst, ZeroConst);

  StoreResult(Op, SelectOp);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFSelectOp);
}

void OpDispatchBuilder::MOVUPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  StoreResult(Op, Src);
}

void OpDispatchBuilder::MOVLHPSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
  StoreResult(Op, Result);
}

void OpDispatchBuilder::MOVHPDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // If the destination is a GPR then the source is memory
    // xmm1[127:64] = src
    OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
    auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
    StoreResult(Op, Result);
  }
  else {
    // In this case memory is the destination and the high bits of the XMM are source
    // Mem64 = xmm1[127:64]
    auto Result = _VInsElement(16, 8, 0, 1, Src, Src);
    StoreResult(Op, Result);
  }
}

void OpDispatchBuilder::PADDQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 8;
  switch (Op->OP) {
  case 0xD4: ElementSize = 8; break;
  case 0xFC: ElementSize = 1; break;
  case 0xFE: ElementSize = 4; break;
  default: LogMan::Msg::A("Unknown PADD op: 0x%04x", Op->OP); break;
  }

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::PSUBQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 8;
  switch (Op->OP) {
  case 0xF8: ElementSize = 1; break;
  case 0xF9: ElementSize = 2; break;
  case 0xFA: ElementSize = 4; break;
  case 0xFB: ElementSize = 8; break;
  default: LogMan::Msg::A("Unknown PSUB op: 0x%04x", Op->OP); break;
  }

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _VSub(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

template<size_t ElementSize>
void OpDispatchBuilder::PMINUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _VUMin(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::PMINSWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _VSMin(Size, 2, Dest, Src);
  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::VectorALUOp(OpcodeArgs) {
  FEXCore::IR::IROps IROp;
  switch (Op->OP) {
  case 0xEB:
    IROp = FEXCore::IR::IROps::OP_VOR;
  break;
  case 0xEF:
    IROp = FEXCore::IR::IROps::OP_VXOR;
  break;
  default:
    IROp = FEXCore::IR::IROps::OP_LAST;
    LogMan::Msg::A("Unknown ALU Op");
  break;
  }

  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _Add(Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::MOVQOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, xmm[Op->Dest.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][0]), Src);
    auto Const = _Constant(0);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, xmm[Op->Dest.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][1]), Const);
  }
  else {
    // This is simple, just store the result
    StoreResult(Op, Src);
  }
}

void OpDispatchBuilder::PMOVMSKBOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *CurrentVal = _Constant(0);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  for (unsigned i = 0; i < Size; ++i) {
    // Extract the top bit of the element
    OrderedNode *Tmp = _Bfe(1, ((i + 1) * 8) - 1, Src);
    // Shift it to the correct location
    Tmp = _Lshl(Tmp, _Constant(i));

    // Or it with the current value
    CurrentVal = _Or(CurrentVal, Tmp);
  }
  StoreResult(Op, CurrentVal);
}

void OpDispatchBuilder::PUNPCKLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 8;
  switch (Op->OP) {
  case 0x60: ElementSize = 1; break;
  case 0x61: ElementSize = 2; break;
  case 0x62: ElementSize = 4; break;
  }

  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  auto ALUOp = _VZip(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::PUNPCKHOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 8;
  switch (Op->OP) {
  case 0x68: ElementSize = 1; break;
  case 0x69: ElementSize = 2; break;
  case 0x6A: ElementSize = 4; break;
  case 0x6D: ElementSize = 8; break;
  }
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);

  auto ALUOp = _VZip2(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

template<size_t ElementSize, bool Low>
void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  LogMan::Throw::A(ElementSize != 0, "What. No element size?");
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  uint8_t Shuffle = Op->Src2.TypeLiteral.Literal;

  uint8_t NumElements = Size / ElementSize;
  if (ElementSize == 2) {
    NumElements /= 2;
  }

  uint8_t BaseElement = Low ? 0 : NumElements;

  auto Dest = Src;
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, BaseElement + Element, BaseElement + (Shuffle & 0b11), Dest, Src);
    Shuffle >>= 2;
  }

  StoreResult(Op, Dest);
}

template<size_t ElementSize>
void OpDispatchBuilder::SHUFOp(OpcodeArgs) {
  LogMan::Throw::A(ElementSize != 0, "What. No element size?");
  auto Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = LoadSource(Op, Op->Src1, Op->Flags);
  uint8_t Shuffle = Op->Src2.TypeLiteral.Literal;

  uint8_t NumElements = Size / ElementSize;

  auto Dest = Src1;
  std::array<OrderedNode*, 2> Srcs = {
    Src1, Src2
  };

  // [63:0]   = Src1[Selection]
  // [127:64] = Src2[Selection]
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, Element, Shuffle & 0b1, Dest, Srcs[Element]);
    Shuffle >>= 1;
  }

  StoreResult(Op, Dest);
}
void OpDispatchBuilder::PCMPEQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 4;
  switch (Op->OP) {
  case 0x74: ElementSize = 1; break;
  case 0x75: ElementSize = 2; break;
  case 0x76: ElementSize = 4; break;
  default: LogMan::Msg::A("Unknown ElementSize"); break;
  }
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VCMPEQ(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

template<size_t ElementSize>
void OpDispatchBuilder::PCMPGTOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VCMPGT(Size, ElementSize, Dest, Src);
  StoreResult(Op, ALUOp);
}

void OpDispatchBuilder::MOVDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1,Op->Flags);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Dest.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    // When destination is XMM then it is zext to 128bit
    uint64_t SrcSize = GetSrcSize(Op) * 8;
    while (SrcSize != 128) {
      Src = _Zext(SrcSize, Src);
      SrcSize *= 2;
    }
  }
  StoreResult(Op, Op->Dest, Src);
}

void OpDispatchBuilder::CMPXCHGOp(OpcodeArgs) {
// CMPXCHG ModRM, reg, {RAX}
// MemData = *ModRM.dest
// if (RAX == MemData)
//    modRM.dest = reg;
//    ZF = 1
// else
//    ZF = 0
// RAX = MemData
//
// CASL Xs, Xt, Xn
// MemData = *Xn
// if (MemData == Xs)
//    *Xn = Xt
// Xs = MemData
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX ||
      Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    LogMan::Msg::A("We don't support CMPXCHG to FS/GS segment");
  }

  auto Size = GetSrcSize(Op);
  // If this is a memory location then we want the pointer to it
  OrderedNode *Src1 = LoadSource(Op, Op->Dest, Op->Flags, false);

  // This is our source register
  OrderedNode *Src2 = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Src3 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]));
  // 0x80014000
  // 0x80064000
  // 0x80064000

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // If our destination is a GPR then this behaves differently
    // RAX = RAX == Op1 ? RAX : Op1
    // AKA if they match then don't touch RAX value
    // Otherwise set it to the rm operand
    OrderedNode *RAXResult = _Select(FEXCore::IR::COND_EQ,
      Src1, Src3,
      Src3, Src1);

    // Op1 = RAX == Op1 ? Op2 : Op1
    // If they match then set the rm operand to the input
    // else don't set the rm operand
    OrderedNode *DestResult = _Select(FEXCore::IR::COND_EQ,
        Src1, Src3,
        Src2, Src1);

    // ZF = RAX == Op1 ? 1 : 0
    // Result of compare
    OrderedNode *ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src1, Src3,
      OneConst, ZeroConst);

    // Set ZF
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
    if (Size < 4) {
      _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);
    }
    else {
      if (Size == 4) {
        RAXResult = _Zext(32, RAXResult);
      }
      _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);
    }

    // Store in to GPR Dest
    // Have to make sure this is after the result store in RAX for when Dest == RAX
    StoreResult(Op, DestResult);
  }
  else {
    // DataSrc = *Src1
    // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
    // This will write to memory! Careful!
    // Third operand must be a calculated guest memory address
    OrderedNode *CASResult = _CAS(Src3, Src2, Src1);

    // If our CASResult(OldMem value) is equal to our comparison
    // Then we managed to set the memory
    OrderedNode *ZFResult = _Select(FEXCore::IR::COND_EQ,
      CASResult, Src3,
      OneConst, ZeroConst);

    // RAX gets the result of the CAS op
    if (Size < 4) {
      _StoreContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CASResult);
    }
    else {
      if (Size == 4) {
        CASResult = _Zext(32, CASResult);
      }
      _StoreContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CASResult);
    }

    // Set ZF
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
  }
}

void OpDispatchBuilder::BeginBlock() {
  _BeginBlock();
}

void OpDispatchBuilder::EndBlock(uint64_t RIPIncrement) {
  _EndBlock(RIPIncrement);
}

void OpDispatchBuilder::ExitFunction() {
  _ExitFunction();
}

uint8_t OpDispatchBuilder::GetDstSize(FEXCore::X86Tables::DecodedOp Op) {
  constexpr std::array<uint8_t, 7> Sizes = {
    0, // Invalid DEF
    1,
    2,
    4,
    8,
    16,
    32
  };

  uint32_t DstSizeFlag = FEXCore::X86Tables::DecodeFlags::GetSizeDstFlags(Op->Flags);
  uint8_t Size = Sizes[DstSizeFlag];
  LogMan::Throw::A(Size != 0, "Invalid destination size for op");
  return Size;
}

uint8_t OpDispatchBuilder::GetSrcSize(FEXCore::X86Tables::DecodedOp Op) {
  constexpr std::array<uint8_t, 7> Sizes = {
    0, // Invalid DEF
    1,
    2,
    4,
    8,
    16,
    32
  };

  uint32_t SrcSizeFlag = FEXCore::X86Tables::DecodeFlags::GetSizeSrcFlags(Op->Flags);
  uint8_t Size = Sizes[SrcSizeFlag];
  LogMan::Throw::A(Size != 0, "Invalid destination size for op");
  return Size;
}

OrderedNode *OpDispatchBuilder::LoadSource_WithOpSize(FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, bool LoadData, bool ForceLoad) {
  LogMan::Throw::A(Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB
        , "Unsupported Src type");

  OrderedNode *Src {nullptr};
  bool LoadableType = false;
  if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL) {
    Src = _Constant(Operand.TypeLiteral.Size * 8, Operand.TypeLiteral.Literal);
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, xmm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][Operand.TypeGPR.HighBits ? 1 : 0]));
    }
    else {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]) + (Operand.TypeGPR.HighBits ? 1 : 0));
    }
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT) {
    Src = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]));
    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT) {
    auto GPR = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPRIndirect.GPR]));
    auto Constant = _Constant(Operand.TypeGPRIndirect.Displacement);

		Src = _Add(GPR, Constant);

    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE) {
    Src = _Constant(Operand.TypeRIPLiteral.Literal + Op->PC + Op->InstSize);
    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB) {
    OrderedNode *Tmp {};
    if (Operand.TypeSIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Index]));

      if (Operand.TypeSIB.Scale != 1) {
        auto Constant = _Constant(Operand.TypeSIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
    }

    if (Operand.TypeSIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Base]));
      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
    }

    if (Operand.TypeSIB.Offset) {
      if (Tmp != nullptr) {
        Src = _Add(Tmp, _Constant(Operand.TypeSIB.Offset));
      }
      else {
        Src = _Constant(Operand.TypeSIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        Src = Tmp;
      }
      else {
        Src = _Constant(0);
      }
    }

    LoadableType = true;
  }
  else {
    LogMan::Msg::A("Unknown Src Type: %d\n", Operand.TypeNone.Type);
  }

  if ((LoadableType && LoadData) || ForceLoad) {
    if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
      Src = _Add(Src, _LoadContext(8, offsetof(FEXCore::Core::CPUState, fs)));
    }
    else if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
      Src = _Add(Src, _LoadContext(8, offsetof(FEXCore::Core::CPUState, gs)));
    }

    Src = _LoadMem(Src, OpSize);
  }
  return Src;
}

OrderedNode *OpDispatchBuilder::LoadSource(FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint32_t Flags, bool LoadData, bool ForceLoad) {
  uint8_t OpSize = GetSrcSize(Op);
  return LoadSource_WithOpSize(Op, Operand, OpSize, Flags, LoadData, ForceLoad);
}

void OpDispatchBuilder::StoreResult_WithOpSize(FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize) {
  LogMan::Throw::A((Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB
        ), "Unsupported Dest type");

  // 8Bit and 16bit destination types store their result without effecting the upper bits
  // 32bit ops ZEXT the result to 64bit
  OrderedNode *MemStoreDst {nullptr};
  bool MemStore = false;

  if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL) {
    MemStoreDst = _Constant(Operand.TypeLiteral.Size * 8, Operand.TypeLiteral.Literal);
    MemStore = true; // Literals are ONLY hardcoded memory destinations
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
      _StoreContext(Src, OpSize, offsetof(FEXCore::Core::CPUState, xmm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][Operand.TypeGPR.HighBits ? 1 : 0]));
    }
    else {
      if (OpSize == 4) {
        LogMan::Throw::A(!Operand.TypeGPR.HighBits, "Can't handle 32bit store to high 8bit register");
        auto ZextOp = _Zext(Src, 32);

        _StoreContext(ZextOp, 8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]));
      }
      else {
        _StoreContext(Src, std::min(static_cast<uint8_t>(8), OpSize), offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]) + (Operand.TypeGPR.HighBits ? 1 : 0));
      }
    }
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT) {
    MemStoreDst = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]));
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT) {
    auto GPR = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPRIndirect.GPR]));
    auto Constant = _Constant(Operand.TypeGPRIndirect.Displacement);

    MemStoreDst = _Add(GPR, Constant);
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE) {
    MemStoreDst = _Constant(Operand.TypeRIPLiteral.Literal + Op->PC + Op->InstSize);
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB) {
    OrderedNode *Tmp {};
    if (Operand.TypeSIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Index]));

      if (Operand.TypeSIB.Scale != 1) {
        auto Constant = _Constant(Operand.TypeSIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
    }

    if (Operand.TypeSIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Base]));

      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
    }

    if (Operand.TypeSIB.Offset) {
      if (Tmp != nullptr) {
        MemStoreDst = _Add(Tmp, _Constant(Operand.TypeSIB.Offset));
      }
      else {
        MemStoreDst = _Constant(Operand.TypeSIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        MemStoreDst = Tmp;
      }
      else {
        MemStoreDst = _Constant(0);
      }
    }

    MemStore = true;
  }

  if (MemStore) {
    if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
      MemStoreDst = _Add(MemStoreDst, _LoadContext(8, offsetof(FEXCore::Core::CPUState, fs)));
    }
    else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
      MemStoreDst = _Add(MemStoreDst, _LoadContext(8, offsetof(FEXCore::Core::CPUState, gs)));
    }

    _StoreMem(OpSize, MemStoreDst, Src);
  }
}

void OpDispatchBuilder::StoreResult(FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src) {
  return StoreResult_WithOpSize(Op, Operand, Src, GetDstSize(Op));
}

void OpDispatchBuilder::StoreResult(FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src) {
  StoreResult(Op, Op->Dest, Src);
}

void OpDispatchBuilder::TestFunction() {
  printf("Doing Test Function\n");
  _BeginBlock();
  auto Load1 = _LoadContext(8, 0);
  auto Load2  = _LoadContext(8, 0);
  //auto Res = Load1 <Add> Load2;
  auto Res = _Add(Load1, Load2);
  _StoreContext(Res, 8, 0);

  std::stringstream out;
  auto IR = ViewIR();
  FEXCore::IR::Dump(&out, &IR);

  printf("List Data Size: %ld\n", ListData.Size());
  printf("IR:\n%s\n@@@@@\n", out.str().c_str());
}

OpDispatchBuilder::OpDispatchBuilder()
  : Data {8 * 1024 * 1024}
  , ListData {8 * 1024 * 1024} {
  ResetWorkingList();
}

void OpDispatchBuilder::ResetWorkingList() {
  Data.Reset();
  ListData.Reset();
  CurrentWriteCursor = nullptr;
  // This is necessary since we do "null" pointer checks
  InvalidNode = reinterpret_cast<OrderedNode*>(ListData.Allocate(sizeof(OrderedNode)));
  DecodeFailure = false;
  Information.HadUnconditionalExit = false;
  ShouldDump = false;
}

template<unsigned BitOffset>
void OpDispatchBuilder::SetRFLAG(OrderedNode *Value) {
  _StoreFlag(Value, BitOffset);
}
void OpDispatchBuilder::SetRFLAG(OrderedNode *Value, unsigned BitOffset) {
  _StoreFlag(Value, BitOffset);
}

OrderedNode *OpDispatchBuilder::GetRFLAG(unsigned BitOffset) {
  return _LoadFlag(BitOffset);
}
constexpr std::array<uint32_t, 17> FlagOffsets = {
  FEXCore::X86State::RFLAG_CF_LOC,
  FEXCore::X86State::RFLAG_PF_LOC,
  FEXCore::X86State::RFLAG_AF_LOC,
  FEXCore::X86State::RFLAG_ZF_LOC,
  FEXCore::X86State::RFLAG_SF_LOC,
  FEXCore::X86State::RFLAG_TF_LOC,
  FEXCore::X86State::RFLAG_IF_LOC,
  FEXCore::X86State::RFLAG_DF_LOC,
  FEXCore::X86State::RFLAG_OF_LOC,
  FEXCore::X86State::RFLAG_IOPL_LOC,
  FEXCore::X86State::RFLAG_NT_LOC,
  FEXCore::X86State::RFLAG_RF_LOC,
  FEXCore::X86State::RFLAG_VM_LOC,
  FEXCore::X86State::RFLAG_AC_LOC,
  FEXCore::X86State::RFLAG_VIF_LOC,
  FEXCore::X86State::RFLAG_VIP_LOC,
  FEXCore::X86State::RFLAG_ID_LOC,
};

void OpDispatchBuilder::SetPackedRFLAG(bool Lower8, OrderedNode *Src) {
  uint8_t NumFlags = FlagOffsets.size();
  if (Lower8) {
    NumFlags = 5;
  }
  auto OneConst = _Constant(1);
  for (int i = 0; i < NumFlags; ++i) {
    auto Tmp = _And(_Lshr(Src, _Constant(FlagOffsets[i])), OneConst);
    SetRFLAG(Tmp, FlagOffsets[i]);
  }
}

OrderedNode *OpDispatchBuilder::GetPackedRFLAG(bool Lower8) {
  OrderedNode *Original = _Constant(2);
  uint8_t NumFlags = FlagOffsets.size();
  if (Lower8) {
    NumFlags = 5;
  }

  for (int i = 0; i < NumFlags; ++i) {
    OrderedNode *Flag = _LoadFlag(FlagOffsets[i]);
    Flag = _Zext(32, Flag);
    Flag = _Lshl(Flag, _Constant(FlagOffsets[i]));
    Original = _Or(Original, Flag);
  }
  return Original;
}

void OpDispatchBuilder::GenerateFlags_ADC(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  auto Size = GetSrcSize(Op) * 8;
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto ThirtyOneConst = _Constant(Size - 1);

    auto LshrOp = _Lshr(Res, ThirtyOneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto Dst8 = _Bfe(Size, 0, Res);

    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Dst8, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto Dst8 = _Bfe(Size, 0, Res);
    auto Src8 = _Bfe(Size, 0, Src2);

    auto SelectOpLT = _Select(FEXCore::IR::COND_LT, Dst8, Src8, OneConst, ZeroConst);
    auto SelectOpLE = _Select(FEXCore::IR::COND_LE, Dst8, Src8, OneConst, ZeroConst);
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, OneConst, SelectOpLE, SelectOpLT);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectCF);
  }

  // OF
  // Signed
  {
    auto NegOne = _Constant(~0ULL);
    auto XorOp1 = _Xor(_Xor(Src1, Src2), NegOne);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (Size) {
    case 8:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 16:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 32:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 64:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default: LogMan::Msg::A("Unknown BFESize: %d", Size); break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::GenerateFlags_SBB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto ThirtyOneConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, ThirtyOneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto Dst8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto Src8_1 = _Bfe(GetSrcSize(Op) * 8, 0, Src1);

    auto SelectOpLT = _Select(FEXCore::IR::COND_GT, Dst8, Src8_1, OneConst, ZeroConst);
    auto SelectOpLE = _Select(FEXCore::IR::COND_GE, Dst8, Src8_1, OneConst, ZeroConst);
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, OneConst, SelectOpLE, SelectOpLT);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectCF);
  }

  // OF
  // Signed
  {
    auto XorOp1 = _Xor(Src1, Src2);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (GetSrcSize(Op)) {
    case 1:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 2:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 4:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 8:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default: LogMan::Msg::A("Unknown BFESize: %d", GetSrcSize(Op)); break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::GenerateFlags_SUB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto Bfe8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Bfe8, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_LT,
        Src1, Src2, OneConst, ZeroConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
  }
  // OF
  {
    auto XorOp1 = _Xor(Src1, Src2);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *FinalAnd = _And(XorOp1, XorOp2);

    switch (GetSrcSize(Op)) {
    case 1:
      FinalAnd = _Bfe(1, 7, FinalAnd);
    break;
    case 2:
      FinalAnd = _Bfe(1, 15, FinalAnd);
    break;
    case 4:
      FinalAnd = _Bfe(1, 31, FinalAnd);
    break;
    case 8:
      FinalAnd = _Bfe(1, 63, FinalAnd);
    break;
    default: LogMan::Msg::A("Unknown BFESize: %d", GetSrcSize(Op)); break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(FinalAnd);
  }
}

void OpDispatchBuilder::GenerateFlags_ADD(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }
  // CF
  {
    auto Dst8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto Src8 = _Bfe(GetSrcSize(Op) * 8, 0, Src2);

    auto SelectOp = _Select(FEXCore::IR::COND_LT, Dst8, Src8, OneConst, ZeroConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
  }

  // OF
  {
    auto NegOne = _Constant(~0ULL);
    auto XorOp1 = _Xor(_Xor(Src1, Src2), NegOne);
    auto XorOp2 = _Xor(Res, Src1);

    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (GetSrcSize(Op)) {
    case 1:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 2:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 4:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 8:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default: LogMan::Msg::A("Unknown BFESize: %d", GetSrcSize(Op)); break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::GenerateFlags_MUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *High) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

  // PF/AF/ZF/SF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(ZeroConst);
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // If the value can fit then the top bits will be zero

    auto SignBit = _Ashr(Res, SignBitConst);

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, SignBit, ZeroConst, OneConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::GenerateFlags_UMUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *High) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // AF/SF/PF/ZF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZeroConst);
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // The result register will be all zero if it can't fit due to how multiplication behaves

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, ZeroConst, ZeroConst, OneConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::GenerateFlags_Logical(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF/OF
  {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
  }
}

void OpDispatchBuilder::GenerateFlags_Shift(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  auto CmpResult = _Select(FEXCore::IR::COND_EQ, Src2, ZeroConst, OneConst, ZeroConst);
  auto CondJump = _CondJump(CmpResult);
  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, OneConst);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF/OF
  {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
  }

  _EndBlock(0);
  auto NewBlock = _BeginBlock();
  SetJumpTarget(CondJump, NewBlock);
}

void OpDispatchBuilder::GenerateFlags_Rotate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto ZeroConst = _Constant(0);

  // CF/OF
  // XXX: These are wrong
  {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
  }
}

void OpDispatchBuilder::UnhandledOp(OpcodeArgs) {
  DecodeFailure = true;
}

void OpDispatchBuilder::MOVOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  StoreResult(Op, Src);
}

void OpDispatchBuilder::ALUOp(OpcodeArgs) {
  FEXCore::IR::IROps IROp;
  switch (Op->OP) {
  case 0x0:
  case 0x1:
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
    IROp = FEXCore::IR::IROps::OP_ADD;
  break;
  case 0x8:
  case 0x9:
  case 0xA:
  case 0xB:
  case 0xC:
  case 0xD:
    IROp = FEXCore::IR::IROps::OP_OR;
  break;
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x24:
  case 0x25:
    IROp = FEXCore::IR::IROps::OP_AND;
  break;
  case 0x28:
  case 0x29:
  case 0x2A:
  case 0x2B:
  case 0x2C:
  case 0x2D:
    IROp = FEXCore::IR::IROps::OP_SUB;
  break;
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
    IROp = FEXCore::IR::IROps::OP_XOR;
  break;
  default:
    IROp = FEXCore::IR::IROps::OP_LAST;
    LogMan::Msg::A("Unknown ALU Op: 0x%x", Op->OP);
  break;
  }

  // X86 basic ALU ops just do the operation between the destination and a single source
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto ALUOp = _Add(Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(Op, ALUOp);

  // Flags set
  {
    auto Size = GetSrcSize(Op) * 8;
    switch (IROp) {
    case FEXCore::IR::IROps::OP_ADD:
      GenerateFlags_ADD(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_SUB:
      GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_MUL:
      GenerateFlags_MUL(Op, _Bfe(Size, 0, ALUOp), _MulH(Dest, Src));
    break;
    case FEXCore::IR::IROps::OP_AND:
    case FEXCore::IR::IROps::OP_XOR:
    case FEXCore::IR::IROps::OP_OR: {
      GenerateFlags_Logical(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    }
    default: break;
    }
  }
}

void OpDispatchBuilder::INTOp(OpcodeArgs) {
  uint8_t Reason{};
  uint8_t Literal{};
  switch (Op->OP) {
  case 0xCC:
    Reason = 0;
  break;
  case 0xCD:
    Reason = 1;
    Literal = Op->Src1.TypeLiteral.Literal;
  break;
  case 0xCE:
    Reason = 2;
  break;
  case 0xF1:
    Reason = 3;
  break;
  case 0xF4: {
    Reason = 4;

    // We want to set RIP to the next instruction after HLT
    auto NewRIP = _Constant(Op->PC + Op->InstSize);
    _StoreContext(8, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  break;
  }
  case 0x0B:
    Reason = 5;
  break;
  }

  if (Op->OP == 0xCE) { // Conditional to only break if Overflow == 1
    auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    // If condition doesn't hold then keep going
    auto CondJump = _CondJump(_Xor(Flag, _Constant(1)));
    _Break(Reason, Literal);
    _EndBlock(0);

    // Make sure to start a new block after ending this one
    auto JumpTarget = _BeginBlock();
    SetJumpTarget(CondJump, JumpTarget);
  }
  else {
    _Break(Reason, Literal);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRLD(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetSrcSize(Op);

  auto Shift = _VUShr(Size, ElementSize, Dest, Src);
  StoreResult(Op, Shift);
}

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::PSLL(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  auto Size = GetDstSize(Op);

  OrderedNode *Result{};

  if (Scalar) {
    Result = _VUShlS(Size, ElementSize, Dest, Src);
  }
  else {
    Result = _VUShl(Size, ElementSize, Dest, Src);
  }

  StoreResult(Op, Result);
}

void OpDispatchBuilder::PSRLDQ(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Dest = LoadSource(Op, Op->Dest, Op->Flags);

  // PSRLDQ shifts by bytes
  // Adjust input value by number of bytes
  Src = _Lshl(Src, _Constant(3));

  auto Shift = _Lshr(Dest, Src);
  StoreResult(Op, Shift);
}

void OpDispatchBuilder::MOVDDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(Op, Op->Src1, Op->Flags);
  OrderedNode *Res = _CreateVector2(Src, Src);
  StoreResult(Op, Res);
}

void OpDispatchBuilder::FXSaveOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(Op, Op->Dest, Op->Flags, false);

  // Saves 512bytes to the memory location provided
  // Header changes depending on if REX.W is set or not
  if (Op->Flags & X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP                   |
    //   16 | FDP                           | MXCSR     | MXCSR_MASK|
  }
  else {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP[31:0] | FCS | <R> |
    //   16 | FDP[31:0] | FDS         | <R> | MXCSR     | MXCSR_MASK|
  }

  // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
  // ------------------------------------------
  //   32 | ST0/MM0                             | <R>
  //   48 | ST1/MM1                             | <R>
  //   64 | ST2/MM2                             | <R>
  //   80 | ST3/MM3                             | <R>
  //   96 | ST4/MM4                             | <R>
  //  112 | ST5/MM5                             | <R>
  //  128 | ST6/MM6                             | <R>
  //  144 | ST7/MM7                             | <R>
  //  160 | XMM0
  //  173 | XMM1
  //  192 | XMM2
  //  208 | XMM3
  //  224 | XMM4
  //  240 | XMM5
  //  256 | XMM6
  //  272 | XMM7
  //  288 | XMM8
  //  304 | XMM9
  //  320 | XMM10
  //  336 | XMM11
  //  352 | XMM12
  //  368 | XMM13
  //  384 | XMM14
  //  400 | XMM15
  //  416 | <R>
  //  432 | <R>
  //  448 | <R>
  //  464 | Available
  //  480 | Available
  //  496 | Available
  // FCW: x87 FPU control word
  // FSW: x87 FPU status word
  // FTW: x87 FPU Tag word (Abridged)
  // FOP: x87 FPU opcode. Lower 11 bits of the opcode
  // FIP: x87 FPU instructyion pointer offset
  // FCS: x87 FPU instruction pointer selector. If CPUID_0000_0007_0000_00000:EBX[bit 13] = 1 then this is deprecated and stores as 0
  // FDP: x87 FPU instruction operand (data) pointer offset
  // FDS: x87 FPU instruction operand (data) pointer selector. Same deprecation as FCS
  // MXCSR: If OSFXSR bit in CR4 is not set then this may not be saved
  // MXCSR_MASK: Mask for writes to the MXCSR register
  // If OSFXSR bit in CR4 is not set than FXSAVE /may/ not save the XMM registers
  // This is implementation dependent
  for (unsigned i = 0; i < 8; ++i) {
    OrderedNode *MMReg = _LoadContext(16, offsetof(FEXCore::Core::CPUState, mm[i]));
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));

    _StoreMem(16, MemLocation, MMReg);
  }
  for (unsigned i = 0; i < 16; ++i) {
    OrderedNode *XMMReg = _LoadContext(16, offsetof(FEXCore::Core::CPUState, xmm[i]));
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));

    _StoreMem(16, MemLocation, XMMReg);
  }
}

void OpDispatchBuilder::FXRStoreOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(Op, Op->Src1, Op->Flags, false);
  for (unsigned i = 0; i < 8; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));
    auto MMReg = _LoadMem(16, MemLocation);
    _StoreContext(16, offsetof(FEXCore::Core::CPUState, mm[i]), MMReg);
  }
  for (unsigned i = 0; i < 16; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));
    auto XMMReg = _LoadMem(16, MemLocation);
    _StoreContext(16, offsetof(FEXCore::Core::CPUState, xmm[i]), XMMReg);
  }
}

#undef OpcodeArgs

void InstallOpcodeHandlers() {
  const std::vector<std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr>> BaseOpTable = {
    // Instructions
    {0x00, 6, &OpDispatchBuilder::ALUOp},
    {0x08, 6, &OpDispatchBuilder::ALUOp},
    {0x10, 6, &OpDispatchBuilder::ADCOp},
    {0x18, 6, &OpDispatchBuilder::SBBOp},
    {0x20, 6, &OpDispatchBuilder::ALUOp},
    {0x28, 6, &OpDispatchBuilder::ALUOp},
    {0x30, 6, &OpDispatchBuilder::ALUOp},
    {0x38, 6, &OpDispatchBuilder::CMPOp},
    {0x50, 8, &OpDispatchBuilder::PUSHOp},
    {0x58, 8, &OpDispatchBuilder::POPOp},
    {0x68, 1, &OpDispatchBuilder::PUSHOp},
    {0x6A, 1, &OpDispatchBuilder::PUSHOp},

    {0x63, 1, &OpDispatchBuilder::MOVSXDOp},
    {0x69, 1, &OpDispatchBuilder::IMUL2SrcOp},
    {0x6B, 1, &OpDispatchBuilder::IMUL2SrcOp},
    {0x70, 16, &OpDispatchBuilder::CondJUMPOp},
    {0x84, 2, &OpDispatchBuilder::TESTOp},
    {0x86, 2, &OpDispatchBuilder::XCHGOp},
    {0x88, 1, &OpDispatchBuilder::MOVOp},
    {0x89, 1, &OpDispatchBuilder::MOVOp},
    // XXX: Causes LLVM to hang?
    {0x8A, 1, &OpDispatchBuilder::MOVOp},
    {0x8B, 1, &OpDispatchBuilder::MOVOp},

    {0x8D, 1, &OpDispatchBuilder::LEAOp},
    {0x90, 8, &OpDispatchBuilder::XCHGOp},

    {0x98, 1, &OpDispatchBuilder::CDQOp},
    {0x99, 1, &OpDispatchBuilder::CQOOp},
    {0x9E, 1, &OpDispatchBuilder::SAHFOp},
    {0x9F, 1, &OpDispatchBuilder::LAHFOp},
    {0xA0, 4, &OpDispatchBuilder::MOVOffsetOp},
    {0xA4, 2, &OpDispatchBuilder::MOVSOp},
    {0xA6, 2, &OpDispatchBuilder::CMPSOp},
    {0xA8, 2, &OpDispatchBuilder::TESTOp},
    {0xAA, 2, &OpDispatchBuilder::STOSOp},
    {0xB0, 8, &OpDispatchBuilder::MOVOp},
    {0xB8, 8, &OpDispatchBuilder::MOVOp},
    {0xC2, 2, &OpDispatchBuilder::RETOp},
    {0xC9, 1, &OpDispatchBuilder::LEAVEOp},
    {0xCC, 3, &OpDispatchBuilder::INTOp},
    {0xE8, 1, &OpDispatchBuilder::CALLOp},
    {0xE9, 1, &OpDispatchBuilder::JUMPOp},
    {0xEB, 1, &OpDispatchBuilder::JUMPOp},
    {0xF1, 1, &OpDispatchBuilder::INTOp},
    {0xF4, 1, &OpDispatchBuilder::INTOp},
    {0xF5, 1, &OpDispatchBuilder::FLAGControlOp},
    {0xF8, 2, &OpDispatchBuilder::FLAGControlOp},
    {0xFC, 2, &OpDispatchBuilder::FLAGControlOp},
  };

   const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> TwoByteOpTable = {
     // Instructions
     {0x00, 1, nullptr}, // GROUP 6
     {0x01, 1, nullptr}, // GROUP 7
     {0x05, 1, &OpDispatchBuilder::SyscallOp},
     {0x0B, 1, &OpDispatchBuilder::INTOp},
     {0x0D, 1, nullptr}, // GROUP P
     {0x18, 1, nullptr}, // GROUP 16

     {0x19, 7, &OpDispatchBuilder::NOPOp}, // NOP with ModRM

     {0x31, 1, &OpDispatchBuilder::RDTSCOp},

     {0x40, 16, &OpDispatchBuilder::CMOVOp},
     {0x6E, 1, &OpDispatchBuilder::UnhandledOp}, // MOVD
     {0x7E, 1, &OpDispatchBuilder::UnhandledOp}, // MOVD
     {0x80, 16, &OpDispatchBuilder::CondJUMPOp}, // XXX: Fails to fixup some jumps
     {0x90, 16, &OpDispatchBuilder::SETccOp}, // XXX: Causes some unit tests to fail due to flags being incorrect
     {0xA2, 1, &OpDispatchBuilder::CPUIDOp},
     {0xA3, 1, &OpDispatchBuilder::BTOp}, // BT
     {0xAF, 1, &OpDispatchBuilder::IMUL1SrcOp}, // XXX: Causes issues with LLVM JIT
     {0xB0, 2, &OpDispatchBuilder::CMPXCHGOp}, // CMPXCHG
     {0xB6, 2, &OpDispatchBuilder::MOVZXOp},
     {0xBC, 1, &OpDispatchBuilder::BSFOp}, // BSF
     {0xBD, 1, &OpDispatchBuilder::BSROp}, // BSF
     // XXX: Broken on LLVM?
     {0xBE, 2, &OpDispatchBuilder::MOVSXOp},
     {0xC8, 8, &OpDispatchBuilder::BSWAPOp},

     // SSE
     // XXX: Broken on LLVM?
     {0x10, 2, &OpDispatchBuilder::MOVUPSOp},
     {0x16, 1, &OpDispatchBuilder::MOVLHPSOp},
     {0x17, 1, &OpDispatchBuilder::MOVUPSOp},
     {0x28, 2, &OpDispatchBuilder::MOVUPSOp},
     {0xEB, 1, &OpDispatchBuilder::VectorALUOp},

     {0x60, 3, &OpDispatchBuilder::PUNPCKLOp},
     {0x64, 1, &OpDispatchBuilder::PCMPGTOp<1>},
     {0x65, 1, &OpDispatchBuilder::PCMPGTOp<2>},
     {0x66, 1, &OpDispatchBuilder::PCMPGTOp<4>},
     {0x68, 3, &OpDispatchBuilder::UnhandledOp},
     {0x6C, 1, &OpDispatchBuilder::UnhandledOp},
     {0x71, 1, nullptr}, // GROUP 12
     {0x72, 1, nullptr}, // GROUP 13
     {0x73, 1, nullptr}, // GROUP 14

     {0x74, 3, &OpDispatchBuilder::PCMPEQOp},
     {0xAE, 1, nullptr}, // GROUP 15
     {0xB9, 1, nullptr}, // GROUP 10
     {0xBA, 1, nullptr}, // GROUP 8
     {0xC7, 1, nullptr}, // GROUP 9

     {0xD4, 1, &OpDispatchBuilder::PADDQOp},
     {0xD6, 1, &OpDispatchBuilder::MOVQOp},
     {0xD7, 1, &OpDispatchBuilder::PMOVMSKBOp},
     // XXX: Untested
     {0xDA, 1, &OpDispatchBuilder::PMINUOp<1>},
     {0xEA, 1, &OpDispatchBuilder::PMINSWOp},
     {0xEF, 1, &OpDispatchBuilder::VectorALUOp},
     {0xF8, 4, &OpDispatchBuilder::PSUBQOp},
     {0xFE, 1, &OpDispatchBuilder::PADDQOp},
   };

   const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> PrimaryGroupOpTable = {
 #define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
    // GROUP 1
    // XXX: Something in this group causing bad syscall when commented out
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 2), 1, &OpDispatchBuilder::ADCOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 3), 1, &OpDispatchBuilder::SBBOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 7), 1, &OpDispatchBuilder::CMPOp}, // CMP

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 2), 1, &OpDispatchBuilder::ADCOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 3), 1, &OpDispatchBuilder::SBBOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 7), 1, &OpDispatchBuilder::CMPOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 2), 1, &OpDispatchBuilder::ADCOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 3), 1, &OpDispatchBuilder::SBBOp}, // Unit tests find this setting flags incorrectly
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 7), 1, &OpDispatchBuilder::CMPOp},

    // GROUP 2
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, &OpDispatchBuilder::SHROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, &OpDispatchBuilder::SHROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, &OpDispatchBuilder::SHROp}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, &OpDispatchBuilder::SHROp}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, &OpDispatchBuilder::SHROp}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, &OpDispatchBuilder::ROLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, &OpDispatchBuilder::ROROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, &OpDispatchBuilder::SHROp}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, &OpDispatchBuilder::ASHROp}, // SAR

    // GROUP 3
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, &OpDispatchBuilder::TESTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, &OpDispatchBuilder::MULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, &OpDispatchBuilder::DIVOp}, // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, &OpDispatchBuilder::TESTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 4), 1, &OpDispatchBuilder::MULOp}, // MUL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 6), 1, &OpDispatchBuilder::DIVOp}, // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    // GROUP 4
    {OPD(FEXCore::X86Tables::TYPE_GROUP_4, OpToIndex(0xFE), 0), 1, &OpDispatchBuilder::INCOp}, // INC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_4, OpToIndex(0xFE), 1), 1, &OpDispatchBuilder::DECOp}, // DEC

    // GROUP 5
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 0), 1, &OpDispatchBuilder::INCOp}, // INC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 1), 1, &OpDispatchBuilder::DECOp}, // DEC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 2), 1, &OpDispatchBuilder::CALLAbsoluteOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 4), 1, &OpDispatchBuilder::JUMPAbsoluteOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 6), 1, &OpDispatchBuilder::PUSHOp},

    // GROUP 11
    // XXX: LLVM hangs when commented out?
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC6), 0), 1, &OpDispatchBuilder::MOVOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC7), 0), 1, &OpDispatchBuilder::MOVOp},
 #undef OPD
   };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> RepModOpTable = {
    {0x19, 7, &OpDispatchBuilder::NOPOp},

    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    // XXX: Causes LLVM to crash if commented out?
    {0x7E, 1, &OpDispatchBuilder::MOVQOp},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> RepNEModOpTable = {
    {0x12, 1, &OpDispatchBuilder::MOVDDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, true>},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> OpSizeModOpTable = {
    {0x12, 2, &OpDispatchBuilder::MOVOp},
    {0x16, 2, &OpDispatchBuilder::MOVHPDOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},

    {0x60, 3, &OpDispatchBuilder::PUNPCKLOp},
    {0x64, 1, &OpDispatchBuilder::PCMPGTOp<1>},
    {0x65, 1, &OpDispatchBuilder::PCMPGTOp<2>},
    {0x66, 1, &OpDispatchBuilder::PCMPGTOp<4>},
    {0x68, 3, &OpDispatchBuilder::PUNPCKHOp},
    {0x6C, 1, &OpDispatchBuilder::PUNPCKLOp},
    {0x6D, 1, &OpDispatchBuilder::PUNPCKHOp},
    {0x6E, 1, &OpDispatchBuilder::MOVDOp},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<4, true>},

    //  XXX: Causing IR interpreter some problems
    {0x74, 3, &OpDispatchBuilder::PCMPEQOp},
    {0x78, 1, nullptr}, // GROUP 17
    {0x7E, 1, &OpDispatchBuilder::MOVDOp},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<8>},

    {0xD4, 1, &OpDispatchBuilder::PADDQOp},
    // XXX: Causes LLVM to crash if commented out?
    {0xD6, 1, &OpDispatchBuilder::MOVQOp},
    {0xD7, 1, &OpDispatchBuilder::PMOVMSKBOp}, // PMOVMSKB
    // XXX: Untested
    {0xDA, 1, &OpDispatchBuilder::PMINUOp<1>},
    {0xEA, 1, &OpDispatchBuilder::PMINSWOp},

    {0xEB, 1, &OpDispatchBuilder::VectorALUOp},

    {0xEF, 1, &OpDispatchBuilder::VectorALUOp}, // PXOR
    {0xF2, 1, &OpDispatchBuilder::PSLL<4, true>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8, true>},
    {0xF8, 4, &OpDispatchBuilder::PSUBQOp},
    {0xFE, 1, &OpDispatchBuilder::PADDQOp},
  };

constexpr uint16_t PF_NONE = 0;
constexpr uint16_t PF_F3 = 1;
constexpr uint16_t PF_66 = 2;
constexpr uint16_t PF_F2 = 3;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> SecondaryExtensionOpTable = {
    // GROUP 8
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 4), 1, &OpDispatchBuilder::BTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 4), 1, &OpDispatchBuilder::BTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 4), 1, &OpDispatchBuilder::BTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 4), 1, &OpDispatchBuilder::BTOp},

    // GROUP 13
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 2), 1, &OpDispatchBuilder::PSRLD<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 6), 1, &OpDispatchBuilder::PSLL<4, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 2), 1, &OpDispatchBuilder::PSRLD<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 6), 1, &OpDispatchBuilder::PSLL<4, true>},

    // GROUP 14
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 2), 1, &OpDispatchBuilder::PSRLD<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 6), 1, &OpDispatchBuilder::PSLL<8, true>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 2), 1, &OpDispatchBuilder::PSRLD<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 6), 1, &OpDispatchBuilder::PSLL<8, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 3), 1, &OpDispatchBuilder::PSRLDQ},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 7), 1, &OpDispatchBuilder::PSLL<16, true>},

    // GROUP 15
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 0), 1, &OpDispatchBuilder::FXSaveOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 1), 1, &OpDispatchBuilder::FXRStoreOp},

    // GROUP 16
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 0), 8, &OpDispatchBuilder::NOPOp},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> SecondaryModRMExtensionOpTable = {
  };
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> X87OpTable = {
  };

  uint64_t NumInsts{};
  auto InstallToTable = [&NumInsts](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LogMan::Throw::A(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
        FinalTable[OpNum + i].OpcodeDispatcher = Dispatcher;
        if (Dispatcher)
          ++NumInsts;
      }
    }
  };

  [[maybe_unused]] auto CheckTable = [](auto& FinalTable) {
    for (size_t i = 0; i < FinalTable.size(); ++i) {
      auto const &Op = FinalTable.at(i);

      if (Op.Type != X86Tables::TYPE_INST) continue; // Invalid op, we don't care
      if (Op.OpcodeDispatcher == nullptr) {
        LogMan::Msg::D("Op: 0x%lx %s didn't have an OpDispatcher", i, Op.Name);
      }
    }
  };

  InstallToTable(FEXCore::X86Tables::BaseOps, BaseOpTable);
  InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable);
  InstallToTable(FEXCore::X86Tables::PrimaryInstGroupOps, PrimaryGroupOpTable);

  InstallToTable(FEXCore::X86Tables::RepModOps, RepModOpTable);
  InstallToTable(FEXCore::X86Tables::RepNEModOps, RepNEModOpTable);
  InstallToTable(FEXCore::X86Tables::OpSizeModOps, OpSizeModOpTable);
  InstallToTable(FEXCore::X86Tables::SecondInstGroupOps, SecondaryExtensionOpTable);

  InstallToTable(FEXCore::X86Tables::X87Ops, X87OpTable);

  // Useful for debugging
  // CheckTable(FEXCore::X86Tables::BaseOps);
  printf("We installed %ld instructions to the tables\n", NumInsts);
}

}
