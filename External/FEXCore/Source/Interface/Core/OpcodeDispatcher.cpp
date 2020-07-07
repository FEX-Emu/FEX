#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/HLE/Thunks/Thunks.h"

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
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  constexpr size_t SyscallArgs = 7;
  std::array<uint64_t, SyscallArgs> *GPRIndexes {};

  if (CTX->Config.Is64BitMode) {
    static std::array<uint64_t, SyscallArgs> GPRIndexes_64 = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
    };
    GPRIndexes = &GPRIndexes_64;
  }
  else {
    static std::array<uint64_t, SyscallArgs> GPRIndexes_32 = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
    };
    GPRIndexes = &GPRIndexes_32;
  }

  auto NewRIP = _Constant(Op->PC);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);

  auto SyscallOp = _Syscall(
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(0) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(1) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(2) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(3) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(4) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(5) * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndexes->at(6) * 8, GPRClass));

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), SyscallOp);
}

void OpDispatchBuilder::ThunkOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  const char *name = (const char*)(Op->PC + 2);

  auto ThunkOp = _Thunk(
    _Constant(Op->PC + 2),
    _Constant((uintptr_t)CTX->ThunkHandler->LookupThunk(name)),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + FEXCore::X86State::REG_RDI * 8, GPRClass)
  );

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ThunkOp);

  auto Constant = _Constant(GPRSize);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewRIP = _LoadMem(GPRClass, GPRSize, OldSP, GPRSize);

  OrderedNode *NewSP;
  
  NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store the new RIP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  _ExitFunction();
  BlockSetRIP = true;
}

void OpDispatchBuilder::LEAOp(OpcodeArgs) {
  if (CTX->Config.Is64BitMode) {
    uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 :
      X86Tables::DecodeFlags::GetOpAddr(Op->Flags) == X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST ? 8 : 4;
    uint32_t SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 4 : 8;

    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, -1, false);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
  }
  else {
    uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 : 4;
    uint32_t SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 2 : 4;

    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, -1, false);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
  }
}

void OpDispatchBuilder::NOPOp(OpcodeArgs) {
}

void OpDispatchBuilder::RETOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  auto Constant = _Constant(GPRSize);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewRIP = _LoadMem(GPRClass, GPRSize, OldSP, GPRSize);

  OrderedNode *NewSP;
  if (Op->OP == 0xC2) {
    auto Offset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    NewSP = _Add(_Add(OldSP, Constant), Offset);
  }
  else {
    NewSP = _Add(OldSP, Constant);
  }

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store the new RIP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  _ExitFunction();
  BlockSetRIP = true;
}

template<uint32_t SrcIndex>
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
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _Add(Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(GPRClass, Op, ALUOp, -1);

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

template<uint32_t SrcIndex>
void OpDispatchBuilder::ADCOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto ALUOp = _Add(_Add(Dest, Src), CF);

  StoreResult(GPRClass, Op, ALUOp, -1);
  GenerateFlags_ADC(Op, ALUOp, Dest, Src, CF);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::SBBOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto ALUOp = _Sub(_Sub(Dest, Src), CF);
  StoreResult(GPRClass, Op, ALUOp, -1);
  GenerateFlags_SBB(Op, ALUOp, Dest, Src, CF);
}

void OpDispatchBuilder::PUSHOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  if (Op->OP == 0xFF && Size == 4) LogMan::Msg::A("Woops. Can't do 32bit for this PUSH op");

  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::PUSHREGOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::POPOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewGPR = _LoadMem(GPRClass, Size, OldSP, Size);

  auto NewSP = _Add(OldSP, Constant);

  if (Op->OP == 0x8F && Size == 4) LogMan::Msg::A("Woops. Can't do 32bit for this POP op");

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded from the stack
  StoreResult(GPRClass, Op, NewGPR, -1);
}

void OpDispatchBuilder::LEAVEOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  // First we move RBP in to RSP and then behave effectively like a pop
  uint8_t Size = GetSrcSize(Op);
  auto Constant = _Constant(Size);

  LogMan::Throw::A(Size == 8, "Can't handle a LEAVE op with size %d", Size);

  auto OldBP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), GPRClass);

  auto NewGPR = _LoadMem(GPRClass, Size, OldBP, Size);

  auto NewSP = _Add(OldBP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded to RBP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), NewGPR);
}

void OpDispatchBuilder::CALLOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  BlockSetRIP = true;
  auto ConstantPC = _Constant(Op->PC + Op->InstSize);

  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *NewRIP = _Add(JMPPCOffset, ConstantPC);
  auto ConstantPCReturn = _Constant(Op->PC + Op->InstSize);

  auto ConstantSize = _Constant(GPRSize);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(GPRClass, GPRSize, NewSP, ConstantPCReturn, GPRSize);

  // Store the RIP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  _ExitFunction(); // If we get here then leave the function now
}

void OpDispatchBuilder::CALLAbsoluteOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  BlockSetRIP = true;

  uint8_t Size = GetSrcSize(Op);
  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ConstantPCReturn = _Constant(Op->PC + Op->InstSize);

  auto ConstantSize = _Constant(Size);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(GPRClass, Size, NewSP, ConstantPCReturn, Size);

  // Store the RIP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), JMPPCOffset);
  _ExitFunction(); // If we get here then leave the function now
}

void OpDispatchBuilder::CondJUMPOp(OpcodeArgs) {
  BlockSetRIP = true;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  IRPair<IROp_Header> SrcCond;

  IRPair<IROp_Constant> TakeBranch;
  IRPair<IROp_Constant> DoNotTakeBranch;
  TakeBranch = _Constant(1);
  DoNotTakeBranch = _Constant(0);

  switch (Op->OP) {
    case 0x70:
    case 0x80: { // JO - Jump if OF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x71:
    case 0x81: { // JNO - Jump if OF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x72:
    case 0x82: { // JC - Jump if CF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x73:
    case 0x83: { // JNC - Jump if CF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x74:
    case 0x84: { // JE - Jump if ZF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x75:
    case 0x85: { // JNE - Jump if ZF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x76:
    case 0x86: { // JNA - Jump if CF == 1 || ZC == 1
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      auto Check = _Or(Flag1, Flag2);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, OneConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x77:
    case 0x87: { // JA - Jump if CF == 0 && ZF == 0
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      auto Check = _Or(Flag1, _Lshl(Flag2, _Constant(1)));
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x78:
    case 0x88: { // JS - Jump if SF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x79:
    case 0x89: { // JNS - Jump if SF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x7A:
    case 0x8A: { // JP - Jump if PF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_PF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x7B:
    case 0x8B: { // JNP - Jump if PF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_PF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x7C: // SF <> OF
    case 0x8C: {
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag1, Flag2, TakeBranch, DoNotTakeBranch);
    break;
    }
    case 0x7D: // SF = OF
    case 0x8D: {
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag1, Flag2, TakeBranch, DoNotTakeBranch);
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
          Check, OneConst, TakeBranch, DoNotTakeBranch);
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
          Check, OneConst, TakeBranch, DoNotTakeBranch);
    break;
    }
    default: LogMan::Msg::A("Unknown Jmp Op: 0x%x\n", Op->OP); return;
  }

  LogMan::Throw::A(Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].TypeLiteral.Literal;

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  // Fallback
  {
    uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

    auto CondJump = _CondJump(SrcCond);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

      auto NewRIP = _Add(RIPOffset, RIPTargetConst);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
      _ExitFunction();
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), RIPTargetConst);
      _ExitFunction();
    }
  }
}

void OpDispatchBuilder::CondJUMPRCXOp(OpcodeArgs) {
  BlockSetRIP = true;
  uint32_t Size = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 4 : 8;

  auto ZeroConst = _Constant(0);
  IRPair<IROp_Header> SrcCond;

  IRPair<IROp_Constant> TakeBranch;
  IRPair<IROp_Constant> DoNotTakeBranch;
  TakeBranch = _Constant(1);
  DoNotTakeBranch = _Constant(0);

  LogMan::Throw::A(Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].TypeLiteral.Literal;

  OrderedNode *CondReg = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
  SrcCond = _Select(FEXCore::IR::COND_EQ,
          CondReg, ZeroConst, TakeBranch, DoNotTakeBranch);

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  {
    uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
    auto CondJump = _CondJump(SrcCond);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto NewRIP = _Constant(Target);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
      _ExitFunction();
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), RIPTargetConst);
      _ExitFunction();
    }
  }
}

template<bool CheckZF, bool ZFTrue>
void OpDispatchBuilder::LoopOp(OpcodeArgs) {
  BlockSetRIP = true;
  auto ZeroConst = _Constant(0);
  IRPair<IROp_Header> SrcCond;

  IRPair<IROp_Constant> TakeBranch = _Constant(1);
  IRPair<IROp_Constant> DoNotTakeBranch = _Constant(0);

  uint32_t SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 4 : 8;

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[1].TypeLiteral.Literal;

  OrderedNode *CondReg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  CondReg = _Sub(CondReg, _Constant(SrcSize, 1));
  StoreResult(GPRClass, Op, Op->Src[0], CondReg, -1);

  SrcCond = _Select(FEXCore::IR::COND_NEQ,
          CondReg, ZeroConst, TakeBranch, DoNotTakeBranch);

  // If LOOPE then jumps to target if RCX != 0 && ZF == 1
  // If LOOPNE then jumps to target if RCX != 0 && ZF == 0
  if (CheckZF) {
    OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    if (!ZFTrue) {
      ZF = _Xor(ZF, _Constant(1));
    }
    SrcCond = _And(SrcCond, ZF);
  }

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  {
    uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
    auto CondJump = _CondJump(SrcCond);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto NewRIP = _Constant(Target);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
      _ExitFunction();
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

      // Store the new RIP
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), RIPTargetConst);
      _ExitFunction();
    }
  }
}

void OpDispatchBuilder::JUMPOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  BlockSetRIP = true;

  // This is just an unconditional relative literal jump
  if (Multiblock) {
    LogMan::Throw::A(Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
    uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].TypeLiteral.Literal;
    auto JumpBlock = JumpTargets.find(Target);
    if (JumpBlock != JumpTargets.end()) {
      _Jump(GetNewJumpBlock(Target));
    }
    else {
      // If the block isn't a jump target then we need to create an exit block
      auto Jump = _Jump();

      auto JumpTarget = CreateNewCodeBlock();
      SetJumpTarget(Jump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), _Constant(Target));
      _ExitFunction();
    }
    return;
  }

  // Fallback
  {
    // This source is a literal
    auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

    auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);

		auto NewRIP = _Add(RIPOffset, RIPTargetConst);

    // Store the new RIP
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
    _ExitFunction();
  }
}

void OpDispatchBuilder::JUMPAbsoluteOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  BlockSetRIP = true;
  // This is just an unconditional jump
  // This uses ModRM to determine its location
  // No way to use this effectively in multiblock
  auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  // Store the new RIP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), RIPOffset);
  _ExitFunction();
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

  StoreResult(GPRClass, Op, SrcCond, -1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::TESTOp(OpcodeArgs) {
  // TEST is an instruction that does an AND between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

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

  OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], Size, Op->Flags, -1);
  if (Size == 2) {
    // This'll make sure to insert in to the lower 16bits without modifying upper bits
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, Size, -1);
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // With REX.W then Sext
    Src = _Sext(Size * 8, Src);
    StoreResult(GPRClass, Op, Src, -1);
  }
  else {
    // Without REX.W then Zext
    Src = _Zext(Size * 8, Src);
    StoreResult(GPRClass, Op, Src, -1);
  }
}

void OpDispatchBuilder::MOVSXOp(OpcodeArgs) {
  // This will ZExt the loaded size
  // We want to Sext it
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  Src = _Sext(Size * 8, Src);
  StoreResult(GPRClass, Op, Op->Dest, Src, -1);
}

void OpDispatchBuilder::MOVZXOp(OpcodeArgs) {
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Just make sure this is zero extended
  Src = _Zext(Size * 8, Src);
  StoreResult(GPRClass, Op, Src, -1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::CMPOp(OpcodeArgs) {
  // CMP is an instruction that does a SUB between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op) * 8;
  auto ALUOp = _Sub(Dest, Src);
  GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::CQOOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto BfeOp = _Bfe(1, GetSrcSize(Op) * 8 - 1, Src);
  auto ZeroConst = _Constant(0);
  auto MaxConst = _Constant(~0ULL);
  auto SelectOp = _Select(FEXCore::IR::COND_EQ, BfeOp, ZeroConst, ZeroConst, MaxConst);

  StoreResult(GPRClass, Op, SelectOp, -1);
}

void OpDispatchBuilder::XCHGOp(OpcodeArgs) {
  // Load both the source and the destination
  if (Op->OP == 0x90 &&
      GetSrcSize(Op) >= 4 &&
      Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Src[0].TypeGPR.GPR == FEXCore::X86State::REG_RAX &&
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

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  if (DestIsLockedMem(Op)) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    auto Result = _AtomicSwap(Dest, Src, GetSrcSize(Op));
    StoreResult(GPRClass, Op, Op->Src[0], Result, -1);
  }
  else {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    // Swap the contents
    // Order matters here since we don't want to swap context contents for one that effects the other
    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
    StoreResult(GPRClass, Op, Op->Src[0], Dest, -1);
  }
}

void OpDispatchBuilder::CDQOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  uint8_t DstSize = GetDstSize(Op);
  uint8_t SrcSize = DstSize >> 1;

  Src = _Sext(SrcSize * 8, Src);
  if (SrcSize == 2) {
    Src = _Zext(DstSize * 8, Src);
    DstSize *= 2;
  }

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
}

void OpDispatchBuilder::SAHFOp(OpcodeArgs) {
  OrderedNode *Src = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, GPRClass);

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
  _StoreContext(GPRClass, 1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, RFLAG);
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


template<bool ToSeg>
void OpDispatchBuilder::MOVSegOp(OpcodeArgs) {
  // In x86-64 mode the accesses to the segment registers end up being constant zero moves
  // Aside from FS/GS
  // In x86-64 mode the accesses to segment registers can actually still touch the segments
  // These write to the selector portion of the register
  //
  // FS and GS are specially handled here though
  // AMD documentation is /wrong/ in this regard
  // AMD documentation claims that the MOV to SReg and POP SReg registers will load a 32bit
  // value in to the HIDDEN portions of the FS and GS registers /OR/ ignored if a null selector is
  // selected for the registers
  // This statement is actually untrue, the instructions will /actually/ load 16bits in to the selector portion of the register!
  // Tested on a Zen+ CPU, the selector is the portion that is modified!
  // We don't currently support FS/GS selector modifying, so this needs to be asserted out
  // The loads here also load the selector, NOT the base

  if (ToSeg) {
    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], 2, Op->Flags, -1);

    switch (Op->Dest.TypeGPR.GPR) {
      case 0: // ES
        _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, es), Src);
        break;
      case 1: // DS
        _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, ds), Src);
        break;
      case 2: // CS
        // CPL3 can't write to this
        _Break(4, 0);
        break;
      case 3: // SS
        _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, ss), Src);
        break;
      case 6: // GS
        LogMan::Throw::A(!CTX->Config.Is64BitMode, "We don't support modifying GS selector in 64bit mode!");
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, gs), Src);
        }
        break;
      case 7: // FS
        LogMan::Throw::A(!CTX->Config.Is64BitMode, "We don't support modifying FS selector in 64bit mode!");
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, fs), Src);
        }
        break;
      default: LogMan::Msg::A("Unknown segment register: %d", Op->Dest.TypeGPR.GPR);
    }
  }
  else {
    OrderedNode *Segment{};

    switch (Op->Src[0].TypeGPR.GPR) {
      case 0: // ES
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, es), GPRClass);
        break;
      case 1: // DS
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, ds), GPRClass);
        break;
      case 2: // CS
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, cs), GPRClass);
        break;
      case 3: // SS
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, ss), GPRClass);
        break;
      case 6: // GS
        if (CTX->Config.Is64BitMode) {
          Segment = _Constant(0);
        }
        else {
          Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
        }
        break;
      case 7: // FS
        if (CTX->Config.Is64BitMode) {
          Segment = _Constant(0);
        }
        else {
          Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, fs), GPRClass);
        }
        break;
      default: LogMan::Msg::A("Unknown segment register: %d", Op->Src[0].TypeGPR.GPR);
    }
    StoreResult(GPRClass, Op, Segment, -1);
  }
}

void OpDispatchBuilder::MOVOffsetOp(OpcodeArgs) {
  OrderedNode *Src;

  switch (Op->OP) {
  case 0xA0:
  case 0xA1:
    // Source is memory(literal)
    // Dest is GPR
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, true, true);
    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
    break;
  case 0xA2:
  case 0xA3:
    // Source is GPR
    // Dest is memory(literal)
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    // This one is a bit special since the destination is a literal
    // So the destination gets stored in Src[1]
    StoreResult(GPRClass, Op, Op->Src[1], Src, -1);
    break;
  }
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

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

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

  StoreResult(GPRClass, Op, SrcCond, -1);
}

void OpDispatchBuilder::CPUIDOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _CPUID(Src);

  OrderedNode *Result_Lower = _ExtractElementPair(Res, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(Res, 1);

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), _Bfe(32, 0,  Result_Lower));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), _Bfe(32, 32, Result_Lower));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), _Bfe(32, 0,  Result_Upper));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), _Bfe(32, 32, Result_Upper));
}

template<bool SHL1Bit>
void OpDispatchBuilder::SHLOp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  if (SHL1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(0x3F));
  else
    Src = _And(Src, _Constant(0x1F));

  auto ALUOp = _Lshl(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if (SHL1Bit) {
    GenerateFlags_ShiftLeftImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), 1);
  }
  else {
    GenerateFlags_ShiftLeft(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::SHLImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = _Constant(Size, Shift);

  auto ALUOp = _Lshl(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_ShiftLeftImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), Shift);
}

template<bool SHR1Bit>
void OpDispatchBuilder::SHROp(OpcodeArgs) {
  OrderedNode *Src;
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  if (SHR1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(0x3F));
  else
    Src = _And(Src, _Constant(0x1F));

  if (Size != 64) {
    Dest = _Bfe(Size, 0, Dest);
  }

  auto ALUOp = _Lshr(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if (SHR1Bit) {
    GenerateFlags_ShiftRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), 1);
  }
  else {
    GenerateFlags_ShiftRight(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::SHRImmediateOp(OpcodeArgs) {
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = _Constant(Size, Shift);

  if (Size != 64) {
    Dest = _Bfe(Size, 0, Dest);
  }

  auto ALUOp = _Lshr(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_ShiftRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), Shift);
}

void OpDispatchBuilder::SHLDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *Shift = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], 1, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = _And(Shift, _Constant(0x3F));
  else
    Shift = _And(Shift, _Constant(0x1F));

  auto ShiftRight = _Sub(_Constant(Size), Shift);

  OrderedNode *Res{};
  auto Tmp1 = _Lshl(Dest, Shift);
  auto Tmp2 = _Lshr(Src, ShiftRight);
  Res = _Or(Tmp1, Tmp2);

  StoreResult(GPRClass, Op, Res, -1);

  GenerateFlags_ShiftLeft(Op, _Bfe(Size, 0, Res), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::SHLDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *ShiftLeft = _Constant(Shift);
  auto ShiftRight = _Constant(Size - Shift);

  OrderedNode *Res{};
  auto Tmp1 = _Lshl(Dest, ShiftLeft);
  auto Tmp2 = _Lshr(Src, ShiftRight);
  Res = _Or(Tmp1, Tmp2);

  StoreResult(GPRClass, Op, Res, -1);

  GenerateFlags_ShiftLeftImmediate(Op, _Bfe(Size, 0, Res), _Bfe(Size, 0, Dest), Shift);
}

void OpDispatchBuilder::SHRDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *Shift = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = _And(Shift, _Constant(0x3F));
  else
    Shift = _And(Shift, _Constant(0x1F));

  auto ShiftLeft = _Sub(_Constant(Size), Shift);

  OrderedNode *Res{};
  auto Tmp1 = _Lshr(Dest, Shift);
  auto Tmp2 = _Lshl(Src, ShiftLeft);
  Res = _Or(Tmp1, Tmp2);
  StoreResult(GPRClass, Op, Res, -1);

  GenerateFlags_ShiftRight(Op, _Bfe(Size, 0, Res), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
}

void OpDispatchBuilder::SHRDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = Op->Src[1].TypeLiteral.Literal & 0x3F;
  else
    Shift = Op->Src[1].TypeLiteral.Literal & 0x1F;

  OrderedNode *ShiftRight = _Constant(Shift);
  auto ShiftLeft = _Constant(Size - Shift);

  OrderedNode *Res{};
  auto Tmp1 = _Lshr(Dest, ShiftRight);
  auto Tmp2 = _Lshl(Src, ShiftLeft);
  Res = _Or(Tmp1, Tmp2);

  StoreResult(GPRClass, Op, Res, -1);

  GenerateFlags_ShiftRightImmediate(Op, _Bfe(Size, 0, Res), _Bfe(Size, 0, Dest), Shift);
}

template<bool SHR1Bit>
void OpDispatchBuilder::ASHROp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  if (SHR1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }


  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Ashr(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if (SHR1Bit) {
    GenerateFlags_SignShiftRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), 1);
  }
  else {
    GenerateFlags_SignShiftRight(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::ASHRImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = _Constant(Size, Shift);
  auto ALUOp = _Ashr(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_SignShiftRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), Shift);
}

template<bool Is1Bit>
void OpDispatchBuilder::ROROp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;
  if (Is1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if (Is1Bit) {
    GenerateFlags_RotateRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), 1);
  }
  else {
    GenerateFlags_RotateRight(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::RORImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = _Constant(Size, Shift);

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_RotateRightImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), Shift);
}

template<bool Is1Bit>
void OpDispatchBuilder::ROLOp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  if (Is1Bit) {
    Src = _Constant(Size, 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, _Constant(Size, 0x3F));
  else
    Src = _And(Src, _Constant(Size, 0x1F));

  auto ALUOp = _Rol(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if (Is1Bit) {
    GenerateFlags_RotateLeftImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), 1);
  }
  else {
    GenerateFlags_RotateLeft(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::ROLImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = _Constant(Size, Shift);

  auto ALUOp = _Rol(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_RotateLeftImmediate(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), Shift);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    uint32_t Size = GetSrcSize(Op);
    uint32_t Mask = Size * 8 - 1;
    OrderedNode *SizeMask = _Constant(Mask);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _And(Src, SizeMask);

    Result = _Lshr(Dest, BitSelect);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Just shift by 3 to get the offset
    Src = _Lshr(Src, _Constant(3));

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    Result = _LoadMem(GPRClass, 1, MemoryLocation, 1);

    // Now shift in to the correct bit location
    Result = _Lshr(Result, BitSelect);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTROp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    uint32_t Size = GetSrcSize(Op);
    uint32_t Mask = Size * 8 - 1;
    OrderedNode *SizeMask = _Constant(Mask);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _And(Src, SizeMask);

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    BitMask = _Not(BitMask);
    Dest = _And(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Just shift by 3 to get the offset
    Src = _Lshr(Src, _Constant(3));

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *Value = _LoadMem(GPRClass, 1, MemoryLocation, 1);

    // Now shift in to the correct bit location
    Result = _Lshr(Value, BitSelect);
    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    BitMask = _Not(BitMask);
    Value = _And(Value, BitMask);
    _StoreMem(GPRClass, 1, MemoryLocation, Value, 1);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTSOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    uint32_t Size = GetSrcSize(Op);
    uint32_t Mask = Size * 8 - 1;
    OrderedNode *SizeMask = _Constant(Mask);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _And(Src, SizeMask);

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    Dest = _Or(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Just shift by 3 to get the offset
    Src = _Lshr(Src, _Constant(3));

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *Value = _LoadMem(GPRClass, 1, MemoryLocation, 1);

    // Now shift in to the correct bit location
    Result = _Lshr(Value, BitSelect);
    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    Value = _Or(Value, BitMask);
    _StoreMem(GPRClass, 1, MemoryLocation, Value, 1);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTCOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    uint32_t Size = GetSrcSize(Op);
    uint32_t Mask = Size * 8 - 1;
    OrderedNode *SizeMask = _Constant(Mask);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _And(Src, SizeMask);

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    Dest = _Xor(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Just shift by 3 to get the offset
    Src = _Lshr(Src, _Constant(3));

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *Value = _LoadMem(GPRClass, 1, MemoryLocation, 1);

    // Now shift in to the correct bit location
    Result = _Lshr(Value, BitSelect);
    OrderedNode *BitMask = _Lshl(_Constant(1), BitSelect);
    Value = _Xor(Value, BitMask);
    _StoreMem(GPRClass, 1, MemoryLocation, Value, 1);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

void OpDispatchBuilder::IMUL1SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Dest = _Mul(Src1, Src2);
  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, _MulH(Src1, Src2));
}

void OpDispatchBuilder::IMUL2SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);

  auto Dest = _Mul(Src1, Src2);
  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, _MulH(Src1, Src2));
}

void OpDispatchBuilder::IMULOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);

  if (Size != 8) {
    Src1 = _Sext(Size * 8, Src1);
    Src2 = _Sext(Size * 8, Src2);
  }

  OrderedNode *Result = _Mul(Src1, Src2);
  OrderedNode *ResultHigh{};
  if (Size == 1) {
    // Result is stored in AX
    _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(8, 8, Result);
    ResultHigh = _Sext(Size * 8, ResultHigh);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(16, 16, Result);
    ResultHigh = _Sext(Size * 8, ResultHigh);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    OrderedNode *ResultLow = _Bfe(32, 0, Result);
    ResultHigh = _Bfe(32, 32, Result);
    if (GPRSize == 8) {
      ResultLow = _Zext(Size * 8, ResultLow);
      ResultHigh = _Zext(Size * 8, ResultHigh);
    }
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ResultLow);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 8) {
    LogMan::Throw::A(CTX->Config.Is64BitMode, "Doesn't exist in 32bit mode");
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _MulH(Src1, Src2);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }

  GenerateFlags_MUL(Op, Result, ResultHigh);
}

void OpDispatchBuilder::MULOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
  if (Size != 8) {
    Src1 = _Zext(Size * 8, Src1);
    Src2 = _Zext(Size * 8, Src2);
  }
  OrderedNode *Result = _UMul(Src1, Src2);
  OrderedNode *ResultHigh{};

  if (Size == 1) {
   // Result is stored in AX
    _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(8, 8, Result);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Bfe(16, 16, Result);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    OrderedNode *ResultLow = Result;
    if (GPRSize == 8) {
      ResultLow = _Bfe(32, 0, Result);
      ResultHigh = _Bfe(32, 32, Result);
    }
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ResultLow);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);

  }
  else if (Size == 8) {
    LogMan::Throw::A(CTX->Config.Is64BitMode, "Doesn't exist in 32bit mode");
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _UMulH(Src1, Src2);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
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

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  Src = _Xor(Src, MaskConst);
  StoreResult(GPRClass, Op, Src, -1);
}

void OpDispatchBuilder::XADDOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // If this is a GPR then we can just do an Add and store
    auto Result = _Add(Dest, Src);
    StoreResult(GPRClass, Op, Result, -1);

    // Previous value in dest gets stored in src
    StoreResult(GPRClass, Op, Op->Src[0], Dest, -1);

    auto Size = GetSrcSize(Op) * 8;
    GenerateFlags_ADD(Op, _Bfe(Size, 0, Result), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
  }
  else {
    auto Before = _AtomicFetchAdd(Dest, Src, GetSrcSize(Op));
    StoreResult(GPRClass, Op, Op->Src[0], Before, -1);

    auto Size = GetSrcSize(Op) * 8;
    auto Result = _Add(Before, Src);
    GenerateFlags_ADD(Op, _Bfe(Size, 0, Result), _Bfe(Size, 0, Before), _Bfe(Size, 0, Src));
  }
}

void OpDispatchBuilder::PopcountOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  Src = _Popcount(Src);
  StoreResult(GPRClass, Op, Src, -1);
  // Set ZF
  auto Zero = _Constant(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      _Constant(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(Zero);
}

void OpDispatchBuilder::XLATOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  OrderedNode *Src = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), GPRClass);
  OrderedNode *Offset = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);

  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
    Src = _Add(Src, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    Src = _Add(Src, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
  }
  Src = _Add(Src, Offset);

  auto Res = _LoadMem(GPRClass, 1, Src, 1);

  _StoreContext(GPRClass, 1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Res);
}

template<OpDispatchBuilder::Segment Seg>
void OpDispatchBuilder::ReadSegmentReg(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src{};
  if (Seg == Segment_FS) {
    Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, fs), GPRClass);
  }
  else {
    Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
  }

  StoreResult(GPRClass, Op, Src, -1);
}

template<OpDispatchBuilder::Segment Seg>
void OpDispatchBuilder::WriteSegmentReg(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  if (Seg == Segment_FS) {
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, fs), Src);
  }
  else {
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gs), Src);
  }
}

void OpDispatchBuilder::EnterOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  LogMan::Throw::A(Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Value = Op->Src[0].TypeLiteral.Literal;

  uint16_t AllocSpace = Value & 0xFFFF;
  uint8_t Level = (Value >> 16) & 0x1F;

  auto PushValue = [&](uint8_t Size, OrderedNode *Src) {
    auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

    auto NewSP = _Sub(OldSP, _Constant(Size));
    _StoreMem(GPRClass, Size, NewSP, Src, Size);

    // Store the new stack pointer
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);
    return NewSP;
  };

  auto OldBP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), GPRClass);
  auto NewSP = PushValue(GPRSize, OldBP);
  auto temp_RBP = NewSP;

  if (Level > 0) {
    for (uint8_t i = 1; i < Level; ++i) {
      auto Offset = _Constant(i * GPRSize);
      auto MemLoc = _Sub(OldBP, Offset);
      auto Mem = _LoadMem(GPRClass, GPRSize, MemLoc, GPRSize);
      NewSP = PushValue(GPRSize, Mem);
    }
    NewSP = PushValue(GPRSize, temp_RBP);
  }
  NewSP = _Sub(NewSP, _Constant(AllocSpace));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), temp_RBP);
}

void OpDispatchBuilder::RDTSCOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  auto Counter = _CycleCounter();
  auto CounterLow = _Bfe(32, 0, Counter);
  auto CounterHigh = _Bfe(32, 32, Counter);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CounterLow);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), CounterHigh);
}

void OpDispatchBuilder::INCOp(OpcodeArgs) {
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX), "Can't handle REP on this\n");

  if (DestIsLockedMem(Op)) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    auto OneConst = _Constant(1);
    auto AtomicResult = _AtomicFetchAdd(Dest, OneConst, GetSrcSize(Op));
    auto ALUOp = _Add(AtomicResult, OneConst);

    StoreResult(GPRClass, Op, ALUOp, -1);

    auto Size = GetSrcSize(Op) * 8;
    GenerateFlags_ADD(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
  }
  else {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    auto OneConst = _Constant(1);
    auto ALUOp = _Add(Dest, OneConst);

    StoreResult(GPRClass, Op, ALUOp, -1);

    auto Size = GetSrcSize(Op) * 8;
    GenerateFlags_ADD(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
  }
}

void OpDispatchBuilder::DECOp(OpcodeArgs) {
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX), "Can't handle REP on this\n");

  if (DestIsLockedMem(Op)) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    auto OneConst = _Constant(1);
    auto AtomicResult = _AtomicFetchSub(Dest, OneConst, GetSrcSize(Op));
    auto ALUOp = _Sub(AtomicResult, OneConst);

    StoreResult(GPRClass, Op, ALUOp, -1);

    auto Size = GetSrcSize(Op) * 8;
    GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
  }
  else {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    auto OneConst = _Constant(1);
    auto ALUOp = _Sub(Dest, OneConst);

    StoreResult(GPRClass, Op, ALUOp, -1);

    auto Size = GetSrcSize(Op) * 8;
    GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, Dest), _Bfe(Size, 0, OneConst));
  }
}

void OpDispatchBuilder::STOSOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX), "Invalid REPNE on STOS");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE), "Can't handle adddress size\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX), "Can't handle FS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX), "Can't handle GS\n");

  auto Size = GetSrcSize(Op);

  bool Repeat = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

  if (!Repeat) {
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

    // Store to memory where RDI points
    _StoreMem(GPRClass, Size, Dest, Src, Size);

    auto SizeConst = _Constant(Size);
    auto NegSizeConst = _Constant(-Size);

    // Calculate direction.
    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF,  _Constant(0),
        SizeConst, NegSizeConst);

    // Offset the pointer
    OrderedNode *TailDest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    TailDest = _Add(TailDest, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest);

  }
  else {
    // Create all our blocks
    auto LoopHead = CreateNewCodeBlock();
    auto LoopTail = CreateNewCodeBlock();
    auto LoopEnd = CreateNewCodeBlock();

    // At the time this was written, our RA can't handle accessing nodes across blocks.
    // So we need to re-load and re-calculate essential values each iteration of the loop.

    // First thing we need to do is finish this block and jump to the start of the loop.
    _Jump(LoopHead);

      SetCurrentCodeBlock(LoopHead);
      {
        OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
        auto ZeroConst = _Constant(0);

        // Can we end the block?
        auto CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
            Counter, ZeroConst,
            _Constant(1), ZeroConst);

        _CondJump(CanLeaveCond, LoopEnd, LoopTail);
      }

      SetCurrentCodeBlock(LoopTail);
      {
        OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

        // Store to memory where RDI points
        _StoreMem(GPRClass, Size, Dest, Src, Size);

        OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
        OrderedNode *TailDest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

        // Decrement counter
        TailCounter = _Sub(TailCounter, _Constant(1));

        // Store the counter so we don't have to deal with PHI here
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

        auto SizeConst = _Constant(Size);
        auto NegSizeConst = _Constant(-Size);

        // Calculate direction.
        auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
        auto PtrDir = _Select(FEXCore::IR::COND_EQ,
            DF,  _Constant(0),
            SizeConst, NegSizeConst);

        // Offset the pointer
        TailDest = _Add(TailDest, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest);

        // Jump back to the start, we have more work to do
        _Jump(LoopHead);
      }

    // Make sure to start a new block after ending this one

    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::MOVSOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX), "Invalid REPNE on MOVS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE), "Can't handle adddress size\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX), "Can't handle FS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX), "Can't handle GS\n");

  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    auto Size = GetSrcSize(Op);

    // Create all our blocks
    auto LoopHead = CreateNewCodeBlock();
    auto LoopTail = CreateNewCodeBlock();
    auto LoopEnd = CreateNewCodeBlock();

    // At the time this was written, our RA can't handle accessing nodes across blocks.
    // So we need to re-load and re-calculate essential values each iteration of the loop.

    // First thing we need to do is finish this block and jump to the start of the loop.
    _Jump(LoopHead);

      SetCurrentCodeBlock(LoopHead);
      {
        OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
        auto ZeroConst = _Constant(0);

        // Can we end the block?
        auto CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
            Counter, ZeroConst,
            _Constant(1), ZeroConst);

        _CondJump(CanLeaveCond, LoopEnd, LoopTail);
      }

      SetCurrentCodeBlock(LoopTail);
      {
        OrderedNode *Src = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
        OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

        Src = _LoadMem(GPRClass, Size, Src, Size);

        // Store to memory where RDI points
        _StoreMem(GPRClass, Size, Dest, Src, Size);

        OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

        // Decrement counter
        TailCounter = _Sub(TailCounter, _Constant(1));

        // Store the counter so we don't have to deal with PHI here
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

        auto SizeConst = _Constant(Size);
        auto NegSizeConst = _Constant(-Size);

        // Calculate direction.
        auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
        auto PtrDir = _Select(FEXCore::IR::COND_EQ,
            DF,  _Constant(0),
            SizeConst, NegSizeConst);

        // Offset the pointer
        OrderedNode *TailSrc = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
        OrderedNode *TailDest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
        TailSrc = _Add(TailSrc, PtrDir);
        TailDest = _Add(TailDest, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), TailSrc);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest);

        // Jump back to the start, we have more work to do
        _Jump(LoopHead);
      }

    // Make sure to start a new block after ending this one

    SetCurrentCodeBlock(LoopEnd);
  }
  else {
    auto Size = GetSrcSize(Op);
    OrderedNode *RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
    OrderedNode *RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

    auto Src = _LoadMem(GPRClass, Size, RSI, Size);

    // Store to memory where RDI points
    _StoreMem(GPRClass, Size, RDI, Src, Size);

    auto SizeConst = _Constant(Size);
    auto NegSizeConst = _Constant(-Size);

    // Calculate direction.
    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF,  _Constant(0),
        SizeConst, NegSizeConst);

    RSI = _Add(RSI, PtrDir);
    RDI = _Add(RDI, PtrDir);

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), RSI);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), RDI);
  }
}

void OpDispatchBuilder::CMPSOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE), "Can't handle adddress size\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX), "Can't handle FS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX), "Can't handle GS\n");

  auto Size = GetSrcSize(Op);

  bool Repeat = Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX);
  if (!Repeat) {
    OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

    auto Src1 = _LoadMem(GPRClass, Size, Dest_RDI, Size);
    auto Src2 = _LoadMem(GPRClass, Size, Dest_RSI, Size);

    auto ALUOp = _Sub(Src1, Src2);
    GenerateFlags_SUB(Op, _Bfe(Size * 8, 0, ALUOp), _Bfe(Size * 8, 0, Src1), _Bfe(Size * 8, 0, Src2));

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, _Constant(0),
        _Constant(Size), _Constant(-Size));

    // Offset the pointer
    Dest_RDI = _Add(Dest_RDI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest_RDI);

    // Offset second pointer
    Dest_RSI = _Add(Dest_RSI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Dest_RSI);
  }
  else {
    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlock();
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);

      OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

      // Can we end the block?
      OrderedNode *CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
        Counter, _Constant(0),
        _Constant(1), _Constant(0));

      auto CondJump = _CondJump(CanLeaveCond);
      IRPair<IROp_CondJump> InternalCondJump;

      auto LoopTail = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, LoopTail);
      SetCurrentCodeBlock(LoopTail);

      // Working loop
      {
        OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
        OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

        auto Src1 = _LoadMem(GPRClass, Size, Dest_RDI, Size);
        auto Src2 = _LoadMem(GPRClass, Size, Dest_RSI, Size);

        auto ALUOp = _Sub(Src1, Src2);
        GenerateFlags_SUB(Op, _Bfe(Size * 8, 0, ALUOp), _Bfe(Size * 8, 0, Src1), _Bfe(Size * 8, 0, Src2));

        OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

        // Decrement counter
        TailCounter = _Sub(TailCounter, _Constant(1));

        // Store the counter so we don't have to deal with PHI here
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

        auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
        auto PtrDir = _Select(FEXCore::IR::COND_EQ,
            DF, _Constant(0),
            _Constant(Size), _Constant(-Size));

        // Offset the pointer
        Dest_RDI = _Add(Dest_RDI, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest_RDI);

        // Offset second pointer
        Dest_RSI = _Add(Dest_RSI, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Dest_RSI);

        OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
        InternalCondJump = _CondJump(ZF);

        if (REPE) {
          // Jump back to the start if we have more work to do
          SetTrueJumpTarget(InternalCondJump, LoopStart);
        }
        else {
          // Jump back to the start if we have more work to do
          SetFalseJumpTarget(InternalCondJump, LoopStart);
        }
      }

    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlock();
    SetTrueJumpTarget(CondJump, LoopEnd);
    if (REPE) {
      SetFalseJumpTarget(InternalCondJump, LoopEnd);
    }
    else {
      SetTrueJumpTarget(InternalCondJump, LoopEnd);
    }
    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::LODSOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE), "Can't handle adddress size\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX), "Can't handle FS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX), "Can't handle GS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX), "LODS doesn't support REPNE");

  auto Size = GetSrcSize(Op);
  bool Repeat = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

  if (!Repeat) {
    OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

    auto Src = _LoadMem(GPRClass, Size, Dest_RSI, Size);

    StoreResult(GPRClass, Op, Src, -1);

    auto SizeConst = _Constant(Size);
    auto NegSizeConst = _Constant(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, _Constant(0),
        SizeConst, NegSizeConst);

    // Offset the pointer
    OrderedNode *TailDest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
    TailDest_RSI = _Add(TailDest_RSI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), TailDest_RSI);
  }
  else {
    // XXX: Theoretically LODS could be optimized to
    // RSI += {-}(RCX * Size)
    // RAX = [RSI - Size]
    // But this might violate the case of an application scanning pages for read permission and catching the fault
    // May or may not matter
    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlock();
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);

        auto ZeroConst = _Constant(0);
        auto OneConst = _Constant(1);

        OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

        // Can we end the block?
        OrderedNode *CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
          Counter, ZeroConst,
          OneConst, ZeroConst);

      // We leave if RCX = 0
      auto CondJump = _CondJump(CanLeaveCond);

      auto LoopTail = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, LoopTail);
      SetCurrentCodeBlock(LoopTail);

      // Working loop
      {
        OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

        auto Src = _LoadMem(GPRClass, Size, Dest_RSI, Size);

        StoreResult(GPRClass, Op, Src, -1);

        OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
        OrderedNode *TailDest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

        // Decrement counter
        TailCounter = _Sub(TailCounter, _Constant(1));

        // Store the counter so we don't have to deal with PHI here
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

        auto SizeConst = _Constant(Size);
        auto NegSizeConst = _Constant(-Size);

        auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
        auto PtrDir = _Select(FEXCore::IR::COND_EQ,
            DF, _Constant(0),
            SizeConst, NegSizeConst);

        // Offset the pointer
        TailDest_RSI = _Add(TailDest_RSI, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), TailDest_RSI);

        // Jump back to the start, we have more work to do
        _Jump(LoopStart);
    }
    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlock();
    SetTrueJumpTarget(CondJump, LoopEnd);
    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::SCASOp(OpcodeArgs) {
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE), "Can't handle adddress size\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX), "Can't handle FS\n");
  LogMan::Throw::A(!(Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX), "Can't handle GS\n");
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  auto Size = GetSrcSize(Op);
  bool Repeat = Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX);

  if (!Repeat) {
    OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

    auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    auto Src2 = _LoadMem(GPRClass, Size, Dest_RDI, Size);

    auto ALUOp = _Sub(Src1, Src2);

    GenerateFlags_SUB(Op, _Bfe(Size * 8, 0, ALUOp), _Bfe(Size * 8, 0, Src1), _Bfe(Size * 8, 0, Src2));

    auto SizeConst = _Constant(Size);
    auto NegSizeConst = _Constant(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, _Constant(0),
        SizeConst, NegSizeConst);

    // Offset the pointer
    OrderedNode *TailDest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    TailDest_RDI = _Add(TailDest_RDI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest_RDI);
  }
  else {
    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlock();
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);

        auto ZeroConst = _Constant(0);
        auto OneConst = _Constant(1);

        OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

        // Can we end the block?
        OrderedNode *CanLeaveCond = _Select(FEXCore::IR::COND_EQ,
          Counter, ZeroConst,
          OneConst, ZeroConst);

      // We leave if RCX = 0
      auto CondJump = _CondJump(CanLeaveCond);
      IRPair<IROp_CondJump> InternalCondJump;

      auto LoopTail = CreateNewCodeBlock();
      SetFalseJumpTarget(CondJump, LoopTail);
      SetCurrentCodeBlock(LoopTail);

      // Working loop
      {
        OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

        auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        auto Src2 = _LoadMem(GPRClass, Size, Dest_RDI, Size);

        auto ALUOp = _Sub(Src1, Src2);
        GenerateFlags_SUB(Op, _Bfe(Size * 8, 0, ALUOp), _Bfe(Size * 8, 0, Src1), _Bfe(Size * 8, 0, Src2));

        OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
        OrderedNode *TailDest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

        // Decrement counter
        TailCounter = _Sub(TailCounter, _Constant(1));

        // Store the counter so we don't have to deal with PHI here
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

        auto SizeConst = _Constant(Size);
        auto NegSizeConst = _Constant(-Size);

        auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
        auto PtrDir = _Select(FEXCore::IR::COND_EQ,
            DF, _Constant(0),
            SizeConst, NegSizeConst);

        // Offset the pointer
        TailDest_RDI = _Add(TailDest_RDI, PtrDir);
        _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest_RDI);

        OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
        InternalCondJump = _CondJump(ZF);

        if (REPE) {
          // Jump back to the start if we have more work to do
          SetTrueJumpTarget(InternalCondJump, LoopStart);
        }
        else {
          // Jump back to the start if we have more work to do
          SetFalseJumpTarget(InternalCondJump, LoopStart);
        }
    }
    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlock();
    SetTrueJumpTarget(CondJump, LoopEnd);
    if (REPE) {
      SetFalseJumpTarget(InternalCondJump, LoopEnd);
    }
    else {
      SetTrueJumpTarget(InternalCondJump, LoopEnd);
    }
    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::BSWAPOp(OpcodeArgs) {
  OrderedNode *Dest;
  if (GetSrcSize(Op) == 2) {
    // BSWAP of 16bit is undef. ZEN+ causes the lower 16bits to get zero'd
    Dest = _Constant(0);
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Dest = _Rev(Dest);
  }
  StoreResult(GPRClass, Op, Dest, -1);
}

void OpDispatchBuilder::PUSHFOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = GetPackedRFLAG(false);
  if (Size != 8) {
    Src = _Bfe(Size * 8, 0, Src);
  }

  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::POPFOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  uint8_t Size = GetSrcSize(Op);
  auto Constant = _Constant(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto Src = _LoadMem(GPRClass, Size, OldSP, Size);

  auto NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  SetPackedRFLAG(false, Src);
}

void OpDispatchBuilder::NEGOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto ZeroConst = _Constant(0);
  auto ALUOp = _Sub(ZeroConst, Dest);

  StoreResult(GPRClass, Op, ALUOp, -1);

  auto Size = GetSrcSize(Op) * 8;

  GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, ZeroConst), _Bfe(Size, 0, Dest));
}

void OpDispatchBuilder::DIVOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);

    auto UDivOp = _UDiv(Src1, Divisor);
    auto URemOp = _URem(Src1, Divisor);

    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, URemOp);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);
    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    OrderedNode *UDivOp = _LUDiv(Src1, Src2, Divisor);
    OrderedNode *URemOp = _LURem(Src1, Src2, Divisor);

    if (GPRSize == 8) {
      UDivOp = _Zext(32, UDivOp);
      URemOp = _Zext(32, URemOp);
    }

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 8) {
    LogMan::Throw::A(CTX->Config.Is64BitMode, "Doesn't exist in 32bit mode");
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::IDIVOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);

    auto UDivOp = _Div(Src1, Divisor);
    auto URemOp = _Rem(Src1, Divisor);

    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, URemOp);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);
    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    OrderedNode *UDivOp = _LDiv(Src1, Src2, Divisor);
    OrderedNode *URemOp = _LRem(Src1, Src2, Divisor);

    if (GPRSize == 8) {
      UDivOp = _Zext(32, UDivOp);
      URemOp = _Zext(32, URemOp);
    }

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 8) {
    LogMan::Throw::A(CTX->Config.Is64BitMode, "Doesn't exist in 32bit mode");
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::BSFOp(OpcodeArgs) {
  uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : 8;
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

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

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, -1);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFSelectOp);
}

void OpDispatchBuilder::BSROp(OpcodeArgs) {
  uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : 8;
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

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

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, -1);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFSelectOp);
}

void OpDispatchBuilder::MOVAPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::MOVUPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVLHPSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, 8);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
  StoreResult(FPRClass, Op, Result, 8);
}

void OpDispatchBuilder::MOVHPDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // If the destination is a GPR then the source is memory
    // xmm1[127:64] = src
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);
    auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else {
    // In this case memory is the destination and the high bits of the XMM are source
    // Mem64 = xmm1[127:64]
    auto Result = _VExtractToGPR(16, 8, Src, 1);
    StoreResult(GPRClass, Op, Result, -1);
  }
}

void OpDispatchBuilder::MOVLPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, 8, 16);
    auto Result = _VInsScalarElement(16, 8, 0, Dest, Src);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 8, 16);
  }
  else {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, 8);
  }
}

void OpDispatchBuilder::MOVSHDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  OrderedNode *Result = _VInsElement(16, 4, 3, 3, Src, Src);
  Result = _VInsElement(16, 4, 2, 3, Result, Src);
  Result = _VInsElement(16, 4, 1, 1, Result, Src);
  Result = _VInsElement(16, 4, 0, 1, Result, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVSLDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  OrderedNode *Result = _VInsElement(16, 4, 3, 2, Src, Src);
  Result = _VInsElement(16, 4, 2, 2, Result, Src);
  Result = _VInsElement(16, 4, 1, 0, Result, Src);
  Result = _VInsElement(16, 4, 0, 0, Result, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVSSOp(OpcodeArgs) {
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // MOVSS xmm1, xmm2
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    auto Result = _VInsScalarElement(16, 4, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // MOVSS xmm1, mem32
    // xmm1[127:0] <- zext(mem32)
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    Src = _Zext(32, Src);
    Src = _Zext(64, Src);
    StoreResult(FPRClass, Op, Src, -1);
  }
  else {
    // MOVSS mem32, xmm1
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 4, -1);
  }
}

void OpDispatchBuilder::MOVSDOp(OpcodeArgs) {
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Src[0].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // xmm1[63:0] <- xmm2[63:0]
    OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    auto Result = _VInsScalarElement(16, 8, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    // xmm1[127:0] <- zext(mem64)
    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], 8, Op->Flags, -1);
    Src = _Zext(64, Src);
    StoreResult(FPRClass, Op, Src, -1);
  }
  else {
    // In this case memory is the destination and the low bits of the XMM are source
    // Mem64 = xmm2[63:0]
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, -1);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::PADDQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSUBQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VSub(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PMINUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VUMin(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PMAXUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VUMax(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

void OpDispatchBuilder::PMINSWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VSMin(Size, 2, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

void OpDispatchBuilder::PMAXSWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VSMax(Size, 2, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorALUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorScalarALUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // If OpSize == ElementSize then it only does the lower scalar op
  auto ALUOp = _VAdd(ElementSize, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  // Insert the lower bits
  auto Result = _VInsScalarElement(Size, ElementSize, 0, Dest, ALUOp);

  StoreResult(FPRClass, Op, Result, -1);
}

template<FEXCore::IR::IROps IROp, size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VectorUnaryOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  if (Scalar) {
    Size = ElementSize;
  }
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VFSqrt(Size, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  if (Scalar) {
    // Insert the lower bits
    auto Result = _VInsScalarElement(GetSrcSize(Op), ElementSize, 0, Dest, ALUOp);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else {
    StoreResult(FPRClass, Op, ALUOp, -1);
  }
}

void OpDispatchBuilder::MOVQOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    _StoreContext(FPRClass, 8, offsetof(FEXCore::Core::CPUState, xmm[Op->Dest.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][0]), Src);
    auto Const = _Constant(0);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, xmm[Op->Dest.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][1]), Const);
  }
  else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, -1);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::MOVMSKOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t NumElements = Size / ElementSize;

  OrderedNode *CurrentVal = _Constant(0);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  for (unsigned i = 0; i < NumElements; ++i) {
    // Extract the top bit of the element
    OrderedNode *Tmp = _VExtractToGPR(16, ElementSize, Src, i);
    Tmp = _Bfe(1, ElementSize * 8 - 1, Tmp);

    // Shift it to the correct location
    Tmp = _Lshl(Tmp, _Constant(i));

    // Or it with the current value
    CurrentVal = _Or(CurrentVal, Tmp);
  }
  StoreResult(GPRClass, Op, CurrentVal, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PUNPCKLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VZip(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PUNPCKHOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VZip2(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize, bool HalfSize, bool Low>
void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  LogMan::Throw::A(ElementSize != 0, "What. No element size?");
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  uint8_t Shuffle = Op->Src[1].TypeLiteral.Literal;

  uint8_t NumElements = Size / ElementSize;

  // 16bit is a bit special of a shuffle
  // It only ever operates on half the register
  // Then there is a high and low variant of the instruction to determine where the destination goes
  // and where the source comes from
  if (HalfSize) {
    NumElements /= 2;
  }

  uint8_t BaseElement = Low ? 0 : NumElements;

  auto Dest = Src;
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, BaseElement + Element, BaseElement + (Shuffle & 0b11), Dest, Src);
    Shuffle >>= 2;
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::SHUFOp(OpcodeArgs) {
  LogMan::Throw::A(ElementSize != 0, "What. No element size?");
  auto Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  uint8_t Shuffle = Op->Src[1].TypeLiteral.Literal;

  uint8_t NumElements = Size / ElementSize;

  auto Dest = Src1;
  std::array<OrderedNode*, 4> Srcs = {
  };

  for (int i = 0; i < (NumElements >> 1); ++i) {
    Srcs[i] = Src1;
  }

  for (int i = (NumElements >> 1); i < NumElements; ++i) {
    Srcs[i] = Src2;
  }

  // 32bit:
  // [31:0]   = Src1[Selection]
  // [63:32]  = Src1[Selection]
  // [95:64]  = Src2[Selection]
  // [127:96] = Src2[Selection]
  // 64bit:
  // [63:0]   = Src1[Selection]
  // [127:64] = Src2[Selection]
  uint8_t SelectionMask = NumElements - 1;
  uint8_t ShiftAmount = __builtin_popcount(SelectionMask);
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, Element, Shuffle & SelectionMask, Dest, Srcs[Element]);
    Shuffle >>= ShiftAmount;
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

void OpDispatchBuilder::ANDNOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Dest = ~Src1 & Src2

  Src1 = _VNot(Size, Size, Src1);
  auto Dest = _VAnd(Size, Size, Src1, Src2);

  StoreResult(FPRClass, Op, Dest, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PINSROp(OpcodeArgs) {
  auto Size = GetDstSize(Op);

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].TypeLiteral.Literal;

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VInsGPR(Size, ElementSize, Dest, Src, Index);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PExtrOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].TypeLiteral.Literal;

  auto Result = _VExtractToGPR(16, ElementSize, Src, Index);
  StoreResult(GPRClass, Op, Result, -1);
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PMULOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *Res{};
  if (Signed)
    Res = _VSMul(Size, ElementSize, Dest, Src);
  else
    Res = _VUMul(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PCMPEQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VCMPEQ(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PCMPGTOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VCMPGT(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
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

  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  auto Size = GetSrcSize(Op);
  // If this is a memory location then we want the pointer to it
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
    Src1 = _Add(Src1, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    Src1 = _Add(Src1, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
  }

  // This is our source register
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src3 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
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
      _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);
    }
    else {
      if (GPRSize == 8 && Size == 4) {
        RAXResult = _Zext(32, RAXResult);
      }
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);
    }

    // Store in to GPR Dest
    // Have to make sure this is after the result store in RAX for when Dest == RAX
    StoreResult(GPRClass, Op, DestResult, -1);
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
      _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CASResult);
    }
    else {
      if (GPRSize == 8 && Size == 4) {
        CASResult = _Zext(32, CASResult);
      }
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CASResult);
    }

    auto Size = GetDstSize(Op) * 8;
    auto ALUOp = _Sub(CASResult, Src3);
    GenerateFlags_SUB(Op, _Bfe(Size, 0, ALUOp), _Bfe(Size, 0, CASResult), _Bfe(Size, 0, Src3));

    // Set ZF
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
  }
}

void OpDispatchBuilder::CMPXCHGPairOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;
  // REX.W used to determine if it is 16byte or 8byte
  // Unlike CMPXCHG, the destination can only be a memory location

  auto Size = GetSrcSize(Op);
  // If this is a memory location then we want the pointer to it
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
    Src1 = _Add(Src1, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    Src1 = _Add(Src1, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
  }

  OrderedNode *Expected_Lower = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
  OrderedNode *Expected_Upper = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);
  OrderedNode *Expected = _CreateElementPair(Expected_Lower, Expected_Upper);

  // Always load 128bits from the context
  // Since we want the full RBX and RCX loaded
  OrderedNode *Desired = _LoadContextPair(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), GPRPairClass);
  if (Size == 4) {
    Desired = _TruncElementPair(Desired, 4);
  }

  // ssa0 = Expected
  // ssa1 = Desired
  // ssa2 = MemoryLocation

  // DataSrc = *MemSrc
  // if (DataSrc == Expected) { *MemSrc == Desired; } Expected = DataSrc
  // This will write to memory! Careful!
  // Third operand must be a calculated guest memory address

  OrderedNode *CASResult = _CASPair(Expected, Desired, Src1);

  OrderedNode *Result_Lower = _ExtractElementPair(CASResult, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(CASResult, 1);

  if (GPRSize == 8 && Size == 4) {
    Result_Lower = _Zext(32, Result_Lower);
    Result_Upper = _Zext(32, Result_Upper);
  }

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result_Lower);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), Result_Upper);

  // Set ZF if memory result was expected
  OrderedNode *EOR_Lower = _Xor(Result_Lower, Expected_Lower);
  OrderedNode *EOR_Upper = _Xor(Result_Upper, Expected_Upper);
  OrderedNode *Orr_Result = _Or(EOR_Lower, EOR_Upper);

  auto OneConst = _Constant(1);
  auto ZeroConst = _Constant(0);
  OrderedNode *ZFResult = _Select(FEXCore::IR::COND_EQ,
    Orr_Result, ZeroConst,
    OneConst, ZeroConst);

  // Set ZF
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
}

void OpDispatchBuilder::CreateJumpBlocks(std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks) {
  OrderedNode *PrevCodeBlock{};
  for (auto &Target : *Blocks) {
    auto CodeNode = CreateCodeNode();

    auto NewNode = _Dummy();
    SetCodeNodeBegin(CodeNode, NewNode);

    auto EndBlock = _EndBlock(0);
    SetCodeNodeLast(CodeNode, EndBlock);

    JumpTargets.try_emplace(Target.Entry, JumpTargetInfo{CodeNode, false});

    if (PrevCodeBlock) {
      LinkCodeBlocks(PrevCodeBlock, CodeNode);
    }

    PrevCodeBlock = CodeNode;
  }
}

void OpDispatchBuilder::BeginFunction(uint64_t RIP, std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks) {
  Entry = RIP;
  auto IRHeader = _IRHeader(InvalidNode, RIP, 0, false);
  Current_Header = IRHeader.first;
  CreateJumpBlocks(Blocks);

  auto Block = GetNewJumpBlock(RIP);
  SetCurrentCodeBlock(Block);
  IRHeader.first->Blocks = Block->Wrapped(ListData.Begin());
}

void OpDispatchBuilder::Finalize() {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  // Node 0 is invalid node
  OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(GetNode(1));
  FEXCore::IR::IROp_Header *IROp = RealNode->Op(Data.Begin());
  LogMan::Throw::A(IROp->Op == OP_IRHEADER, "First op in function must be our header");

  // Let's walk the jump blocks and see if we have handled every block target
  for (auto &Handler : JumpTargets) {
    if (Handler.second.HaveEmitted) continue;

    // We haven't emitted. Dump out to the dispatcher
    SetCurrentCodeBlock(Handler.second.BlockEntry);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), _Constant(Handler.first));
    _ExitFunction();
  }
}

void OpDispatchBuilder::ExitFunction() {
  _ExitFunction();
}

uint8_t OpDispatchBuilder::GetDstSize(FEXCore::X86Tables::DecodedOp Op) {
  constexpr std::array<uint8_t, 8> Sizes = {
    0, // Invalid DEF
    1,
    2,
    4,
    8,
    16,
    32,
    0, // Invalid DEF
  };

  uint32_t DstSizeFlag = FEXCore::X86Tables::DecodeFlags::GetSizeDstFlags(Op->Flags);
  uint8_t Size = Sizes[DstSizeFlag];
  LogMan::Throw::A(Size != 0, "Invalid destination size for op");
  return Size;
}

uint8_t OpDispatchBuilder::GetSrcSize(FEXCore::X86Tables::DecodedOp Op) {
  constexpr std::array<uint8_t, 8> Sizes = {
    0, // Invalid DEF
    1,
    2,
    4,
    8,
    16,
    32,
    0, // Invalid DEF
  };

  uint32_t SrcSizeFlag = FEXCore::X86Tables::DecodeFlags::GetSizeSrcFlags(Op->Flags);
  uint8_t Size = Sizes[SrcSizeFlag];
  LogMan::Throw::A(Size != 0, "Invalid destination size for op");
  return Size;
}

OrderedNode *OpDispatchBuilder::LoadSource_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, int8_t Align, bool LoadData, bool ForceLoad) {
  LogMan::Throw::A(Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE ||
          Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB
        , "Unsupported Src type");

  OrderedNode *Src {nullptr};
  bool LoadableType = false;
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL) {
    Src = _Constant(Operand.TypeLiteral.Size * 8, Operand.TypeLiteral.Literal);
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_MM_0) {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, mm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_MM_0]), FPRClass);
    }
    else if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, xmm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][Operand.TypeGPR.HighBits ? 1 : 0]), FPRClass);
    }
    else {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]) + (Operand.TypeGPR.HighBits ? 1 : 0), GPRClass);
    }
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT) {
    Src = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]), GPRClass);
    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT) {
    auto GPR = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPRIndirect.GPR]), GPRClass);
    auto Constant = _Constant(GPRSize * 8, Operand.TypeGPRIndirect.Displacement);

		Src = _Add(GPR, Constant);

    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE) {
    if (CTX->Config.Is64BitMode) {
      Src = _Constant(GPRSize * 8, Operand.TypeRIPLiteral.Literal + Op->PC + Op->InstSize);
    }
    else {
      // 32bit this isn't RIP relative but instead absolute
      Src = _Constant(GPRSize * 8, Operand.TypeRIPLiteral.Literal);
    }

    LoadableType = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB) {
    OrderedNode *Tmp {};
    if (Operand.TypeSIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Index]), GPRClass);

      if (Operand.TypeSIB.Scale != 1) {
        auto Constant = _Constant(GPRSize * 8, Operand.TypeSIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
    }

    if (Operand.TypeSIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Base]), GPRClass);

      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
    }

    if (Operand.TypeSIB.Offset) {
      if (Tmp != nullptr) {
        Src = _Add(Tmp, _Constant(GPRSize * 8, Operand.TypeSIB.Offset));
      }
      else {
        Src = _Constant(GPRSize * 8, Operand.TypeSIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        Src = Tmp;
      }
      else {
        Src = _Constant(GPRSize * 8, 0);
      }
    }

    LoadableType = true;
  }
  else {
    LogMan::Msg::A("Unknown Src Type: %d\n", Operand.TypeNone.Type);
  }

  if ((LoadableType && LoadData) || ForceLoad) {
    if (CTX->Config.Is64BitMode) {
      if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
        Src = _Add(Src, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
      }
      else if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
        Src = _Add(Src, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
      }
    }
    else {
      if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
        LogMan::Msg::A("Don't support segment prefix on 32bit yet");
      }
      else if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
        OrderedNode *Segment = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
        Segment = _Lshr(Segment, _Constant(3));
        auto data = _LoadContextIndexed(Segment, 4, offsetof(FEXCore::Core::CPUState, gdt[0]), 4, GPRClass);
        Src = _Add(Src, data);
      }
    }

    Src = _LoadMem(Class, OpSize, Src, Align == -1 ? OpSize : Align);
  }
  return Src;
}

OrderedNode *OpDispatchBuilder::LoadSource(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint32_t Flags, int8_t Align, bool LoadData, bool ForceLoad) {
  uint8_t OpSize = GetSrcSize(Op);
  return LoadSource_WithOpSize(Class, Op, Operand, OpSize, Flags, Align, LoadData, ForceLoad);
}

void OpDispatchBuilder::StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align) {
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
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL) {
    MemStoreDst = _Constant(Operand.TypeLiteral.Size * 8, Operand.TypeLiteral.Literal);
    MemStore = true; // Literals are ONLY hardcoded memory destinations
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR) {
    if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_MM_0) {
      _StoreContext(Class, OpSize, offsetof(FEXCore::Core::CPUState, mm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_MM_0]), Src);
    }
    else if (Operand.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
      _StoreContext(Class, OpSize, offsetof(FEXCore::Core::CPUState, xmm[Operand.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][Operand.TypeGPR.HighBits ? 1 : 0]), Src);
    }
    else {
      if (GPRSize == 8 && OpSize == 4) {
        LogMan::Throw::A(!Operand.TypeGPR.HighBits, "Can't handle 32bit store to high 8bit register");
        auto ZextOp = _Zext(Src, 32);

        _StoreContext(Class, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]), ZextOp);
      }
      else {
        LogMan::Throw::A(!(GPRSize == 4 && OpSize > 4), "Oops had a %d GPR load", OpSize);
        _StoreContext(Class, std::min(GPRSize, OpSize), offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]) + (Operand.TypeGPR.HighBits ? 1 : 0), Src);
      }
    }
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_DIRECT) {
    MemStoreDst = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPR.GPR]), GPRClass);
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR_INDIRECT) {
    auto GPR = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeGPRIndirect.GPR]), GPRClass);
    auto Constant = _Constant(GPRSize * 8, Operand.TypeGPRIndirect.Displacement);

    MemStoreDst = _Add(GPR, Constant);
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_RIP_RELATIVE) {
    MemStoreDst = _Constant(GPRSize * 8, Operand.TypeRIPLiteral.Literal + Op->PC + Op->InstSize);
    MemStore = true;
  }
  else if (Operand.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_SIB) {
    OrderedNode *Tmp {};
    if (Operand.TypeSIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Index]), GPRClass);

      if (Operand.TypeSIB.Scale != 1) {
        auto Constant = _Constant(GPRSize * 8, Operand.TypeSIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
    }

    if (Operand.TypeSIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.TypeSIB.Base]), GPRClass);

      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
    }

    if (Operand.TypeSIB.Offset) {
      if (Tmp != nullptr) {
        MemStoreDst = _Add(Tmp, _Constant(GPRSize * 8, Operand.TypeSIB.Offset));
      }
      else {
        MemStoreDst = _Constant(GPRSize * 8, Operand.TypeSIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        MemStoreDst = Tmp;
      }
      else {
        MemStoreDst = _Constant(GPRSize * 8, 0);
      }
    }

    MemStore = true;
  }

  if (MemStore) {
    if (CTX->Config.Is64BitMode) {
      if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
        MemStoreDst = _Add(MemStoreDst, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
      }
      else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
        MemStoreDst = _Add(MemStoreDst, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
      }
    }
    else {
      if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
        LogMan::Msg::A("Don't support segment prefix on 32bit yet");
      }
      else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
        OrderedNode *Segment = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
        Segment = _Lshr(Segment, _Constant(3));
        auto data = _LoadContextIndexed(Segment, 4, offsetof(FEXCore::Core::CPUState, gdt[0]), 4, GPRClass);
        MemStoreDst = _Add(MemStoreDst, data);
      }
    }

    if (OpSize == 10) {
      // For X87 extended doubles, split before storing
      _StoreMem(FPRClass, 8, MemStoreDst, Src, Align);
      auto Upper = _VExtractToGPR(16, 8, Src, 1);
      auto DestAddr = _Add(MemStoreDst, _Constant(8));
      _StoreMem(GPRClass, 2, DestAddr, Upper, std::min<uint8_t>(Align, 8));
    } else {
      _StoreMem(Class, OpSize, MemStoreDst, Src, Align == -1 ? OpSize : Align);
    }
  }
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, int8_t Align) {
  StoreResult_WithOpSize(Class, Op, Operand, Src, GetDstSize(Op), Align);
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src, int8_t Align) {
  StoreResult(Class, Op, Op->Dest, Src, Align);
}

OpDispatchBuilder::OpDispatchBuilder(FEXCore::Context::Context *ctx)
  : CTX {ctx} {
  ResetWorkingList();
}

void OpDispatchBuilder::ResetWorkingList() {
  IREmitter::ResetWorkingList();
  JumpTargets.clear();
  BlockSetRIP = false;
  DecodeFailure = false;
  ShouldDump = false;
  CurrentCodeBlock = nullptr;
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
  auto Size = GetSrcSize(Op) * 8;
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
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto Dst8 = _Bfe(Size, 0, Res);

    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Dst8, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto Dst8 = _Bfe(Size, 0, Res);
    auto Src8 = _Bfe(Size, 0, Src2);

    auto SelectOpLT = _Select(FEXCore::IR::COND_ULT, Dst8, Src8, _Constant(1), _Constant(0));
    auto SelectOpLE = _Select(FEXCore::IR::COND_ULE, Dst8, Src8, _Constant(1), _Constant(0));
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, _Constant(1), SelectOpLE, SelectOpLT);
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
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto Dst8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto Src8_1 = _Bfe(GetSrcSize(Op) * 8, 0, Src1);

    auto SelectOpLT = _Select(FEXCore::IR::COND_UGT, Dst8, Src8_1, _Constant(1), _Constant(0));
    auto SelectOpLE = _Select(FEXCore::IR::COND_UGE, Dst8, Src8_1, _Constant(1), _Constant(0));
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, _Constant(1), SelectOpLE, SelectOpLT);
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
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto ZeroConst = _Constant(0);
    auto OneConst = _Constant(1);
    auto Bfe8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Bfe8, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  {
    auto ZeroConst = _Constant(0);
    auto OneConst = _Constant(1);

    auto SelectOp = _Select(FEXCore::IR::COND_ULT,
        Src1, Src2, OneConst, ZeroConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
  }
  // OF
  {
    auto XorOp1 = _Xor(Src1, Src2);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *FinalAnd = _And(XorOp1, XorOp2);

    FinalAnd = _Bfe(1, GetSrcSize(Op) * 8 - 1, FinalAnd);

    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(FinalAnd);
  }
}

void OpDispatchBuilder::GenerateFlags_ADD(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
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
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }
  // CF
  {
    auto Dst8 = _Bfe(GetSrcSize(Op) * 8, 0, Res);
    auto Src8 = _Bfe(GetSrcSize(Op) * 8, 0, Src2);

    auto SelectOp = _Select(FEXCore::IR::COND_ULT, Dst8, Src8, _Constant(1), _Constant(0));

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
  auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

  // PF/AF/ZF/SF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(_Constant(0));
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // If the value can fit then the top bits will be zero

    auto SignBit = _Ashr(Res, SignBitConst);

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, SignBit, _Constant(0), _Constant(1));

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::GenerateFlags_UMUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *High) {
  // AF/SF/PF/ZF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Constant(0));
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // The result register will be all zero if it can't fit due to how multiplication behaves

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, _Constant(0), _Constant(0), _Constant(1));

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::GenerateFlags_Logical(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
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
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF/OF
  {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Constant(0));
  }
}

void OpDispatchBuilder::GenerateFlags_ShiftLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto CmpResult = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), _Constant(1), _Constant(0));
  auto CondJump = _CondJump(CmpResult);

  // Make sure to start a new block after ending this one
  auto JumpTarget = CreateNewCodeBlock();
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  // CF
  {
    // Extract the last bit shifted in to CF
    auto Size = _Constant(GetSrcSize(Op) * 8);
    auto ShiftAmt = _Sub(Size, Src2);
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(LastBit);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);

    // OF
    // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
    // When Shift > 1 then OF is undefined
    auto SourceBit = _Bfe(1, GetSrcSize(Op) * 8 - 1, Src1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(SourceBit, LshrOp));
  }

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlock();
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}

void OpDispatchBuilder::GenerateFlags_ShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto CmpResult = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), _Constant(1), _Constant(0));
  auto CondJump = _CondJump(CmpResult);

  // Make sure to start a new block after ending this one
  auto JumpTarget = CreateNewCodeBlock();
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  // CF
  {
    // Extract the last bit shifted in to CF
    auto ShiftAmt = _Sub(Src2, _Constant(1));
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(LastBit);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set to the MSB of the original value
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, GetSrcSize(Op) * 8 - 1, Src1));
  }

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlock();
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}

void OpDispatchBuilder::GenerateFlags_SignShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto CmpResult = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), _Constant(1), _Constant(0));
  auto CondJump = _CondJump(CmpResult);

  // Make sure to start a new block after ending this one
  auto JumpTarget = CreateNewCodeBlock();
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  // CF
  {
    // Extract the last bit shifted in to CF
    auto ShiftAmt = _Sub(Src2, _Constant(1));
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(LastBit);
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // OF
  {
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Constant(0));
  }

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlock();
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}

void OpDispatchBuilder::GenerateFlags_ShiftLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, GetSrcSize(Op) * 8 - Shift, Src1));
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto LshrOp = _Bfe(1, GetSrcSize(Op) * 8 - 1, Res);

    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);

    // OF
    // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
    if (Shift == 1) {
      auto SourceBit = _Bfe(1, GetSrcSize(Op) * 8 - 1, Src1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(SourceBit, LshrOp));
    }
  }
}

void OpDispatchBuilder::GenerateFlags_SignShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift, Src1));
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);

    // OF
    // Only defined when Shift is 1 else undefined
    // Only is set if the top bit was set to 1 when shifted
    // So it is set to same value as SF
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Constant(0));
    }
  }
}

void OpDispatchBuilder::GenerateFlags_ShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift, Src1));
  }

  // PF
  {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(GetSrcSize(Op) * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set to the MSB of the original value
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, GetSrcSize(Op) * 8 - 1, Src1));
    }
  }
}

void OpDispatchBuilder::GenerateFlags_RotateRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto OpSize = GetSrcSize(Op) * 8;

  // Extract the last bit shifted in to CF
  auto ShiftAmt = _Sub(Src2, _Constant(1));
  auto NewCF = _And(_Lshr(Src1, ShiftAmt), _Constant(1));

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);
  }

  // OF
  {
    // OF is set to the XOR of the new CF bit and the most significant bit of the result
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Res), NewCF));
  }
}

void OpDispatchBuilder::GenerateFlags_RotateLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto OpSize = GetSrcSize(Op) * 8;

  // Extract the last bit shifted in to CF
  auto Size = _Constant(GetSrcSize(Op) * 8);
  auto ShiftAmt = _Sub(Size, Src2);
  auto NewCF = _And(_Lshr(Src1, ShiftAmt), _Constant(1));

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);
  }

  // OF
  {
    // OF is set to the XOR of the new CF bit and the most significant bit of the result
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Res), NewCF));
  }
}

void OpDispatchBuilder::GenerateFlags_RotateRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  auto OpSize = GetSrcSize(Op) * 8;

  auto NewCF = _Bfe(1, OpSize - Shift, Src1);

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);
  }

  // OF
  {
    if (Shift == 1) {
      // OF is set to the XOR of the new CF bit and the most significant bit of the result
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Res), NewCF));
    }
  }
}

void OpDispatchBuilder::GenerateFlags_RotateLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  auto OpSize = GetSrcSize(Op) * 8;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift, Src1));
  }

  // OF
  {
    if (Shift == 1) {
      // OF is the top two MSBs XOR'd together
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Src1), _Bfe(1, OpSize - 2, Src1)));
    }
  }
}

void OpDispatchBuilder::UnhandledOp(OpcodeArgs) {
  DecodeFailure = true;
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::MOVGPROp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, 1);
  StoreResult(GPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVVectorOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  StoreResult(FPRClass, Op, Src, 1);
}

template<uint32_t SrcIndex>
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
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);

  OrderedNode *Result{};
  OrderedNode *Dest{};
  if (DestIsLockedMem(Op)) {
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    switch (IROp) {
      case FEXCore::IR::IROps::OP_ADD: {
        Dest = _AtomicFetchAdd(DestMem, Src, GetSrcSize(Op));
        Result = _Add(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_SUB: {
        Dest = _AtomicFetchSub(DestMem, Src, GetSrcSize(Op));
        Result = _Sub(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_OR: {
        Dest = _AtomicFetchOr(DestMem, Src, GetSrcSize(Op));
        Result = _Or(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_AND: {
        Dest = _AtomicFetchAnd(DestMem, Src, GetSrcSize(Op));
        Result = _And(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_XOR: {
        Dest = _AtomicFetchXor(DestMem, Src, GetSrcSize(Op));
        Result = _Xor(Dest, Src);
        break;
      }
      default: LogMan::Msg::A("Unknown Atomic IR Op: %d", IROp); break;
    }
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    auto ALUOp = _Add(Dest, Src);
    // Overwrite our IR's op type
    ALUOp.first->Header.Op = IROp;

    StoreResult(GPRClass, Op, ALUOp, -1);
    Result = ALUOp;
  }

  // Flags set
  {
    auto Size = GetSrcSize(Op) * 8;
    switch (IROp) {
    case FEXCore::IR::IROps::OP_ADD:
      GenerateFlags_ADD(Op, _Bfe(Size, 0, Result), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_SUB:
      GenerateFlags_SUB(Op, _Bfe(Size, 0, Result), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    case FEXCore::IR::IROps::OP_MUL:
      GenerateFlags_MUL(Op, _Bfe(Size, 0, Result), _MulH(Dest, Src));
    break;
    case FEXCore::IR::IROps::OP_AND:
    case FEXCore::IR::IROps::OP_XOR:
    case FEXCore::IR::IROps::OP_OR: {
      GenerateFlags_Logical(Op, _Bfe(Size, 0, Result), _Bfe(Size, 0, Dest), _Bfe(Size, 0, Src));
    break;
    }
    default: break;
    }
  }
}

void OpDispatchBuilder::INTOp(OpcodeArgs) {
  uint8_t Reason{};
  uint8_t Literal{};
  bool setRIP = false;

  switch (Op->OP) {
  case 0xCD:
    Reason = 1;
    Literal = Op->Src[0].TypeLiteral.Literal;
    if (Literal == 0x80) {
      // Syscall on linux
      SyscallOp(Op);
      return;
    }
    if (Literal == 0x7F) {
      // special thunk op
      ThunkOp(Op);
      return;
    }
  break;
  case 0xCE:
    Reason = 2;
  break;
  case 0xF1:
    Reason = 3;
  break;
  case 0xF4: {
    Reason = 4;
    setRIP = true;
  break;
  }
  case 0x0B:
    Reason = 5;
  case 0xCC:
    Reason = 6;
    setRIP = true;
  break;
  }

  if (setRIP) {
    uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

    BlockSetRIP = setRIP;

    // We want to set RIP to the next instruction after HLT/INT3
    auto NewRIP = _Constant(Op->PC + Op->InstSize);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  }

  if (Op->OP == 0xCE) { // Conditional to only break if Overflow == 1
    auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    // If condition doesn't hold then keep going
    auto CondJump = _CondJump(_Xor(Flag, _Constant(1)));
    auto FalseBlock = CreateNewCodeBlock();
    SetFalseJumpTarget(CondJump, FalseBlock);
    SetCurrentCodeBlock(FalseBlock);

    _Break(Reason, Literal);

    // Make sure to start a new block after ending this one
    auto JumpTarget = CreateNewCodeBlock();
    SetTrueJumpTarget(CondJump, JumpTarget);
    SetCurrentCodeBlock(JumpTarget);

  }
  else {
    _Break(Reason, Literal);
  }
}

template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
void OpDispatchBuilder::PSRLDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op);

  OrderedNode *Result{};

  if (Scalar) {
    Result = _VUShrS(Size, ElementSize, Dest, Src);
  }
  else {
    Result = _VUShr(Size, ElementSize, Dest, Src);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRLI(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t ShiftConstant = Op->Src[1].TypeLiteral.Literal;

  auto Size = GetSrcSize(Op);

  auto Shift = _VUShrI(Size, ElementSize, Dest, ShiftConstant);
  StoreResult(FPRClass, Op, Shift, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSLLI(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t ShiftConstant = Op->Src[1].TypeLiteral.Literal;

  auto Size = GetSrcSize(Op);

  auto Shift = _VShlI(Size, ElementSize, Dest, ShiftConstant);
  StoreResult(FPRClass, Op, Shift, -1);
}

template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
void OpDispatchBuilder::PSLL(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result{};

  if (Scalar) {
    Result = _VUShlS(Size, ElementSize, Dest, Src);
  }
  else {
    Result = _VUShl(Size, ElementSize, Dest, Src);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
void OpDispatchBuilder::PSRAOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result{};

  if (Scalar) {
    Result = _VSShrS(Size, ElementSize, Dest, Src);
  }
  else {
    Result = _VSShr(Size, ElementSize, Dest, Src);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PSRLDQ(OpcodeArgs) {
  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  auto Result = _VSRI(Size, 16, Dest, Shift);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PSLLDQ(OpcodeArgs) {
  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  auto Result = _VSLI(Size, 16, Dest, Shift);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRAIOp(OpcodeArgs) {
  LogMan::Throw::A(Op->Src[1].TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_LITERAL, "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].TypeLiteral.Literal;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  auto Result = _VSShrI(Size, ElementSize, Dest, Shift);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PAVGOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Result = _VURAvg(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVDDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res =  _SplatVector2(Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t DstElementSize, bool Signed>
void OpDispatchBuilder::CVTGPR_To_FPR(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Op->Flags & X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // Source is 64bit
    if (DstElementSize == 4) {
      Src = _Bfe(32, 0, Src);
    }
  }
  else {
    // Source is 32bit
    if (DstElementSize == 8) {
      if (Signed)
        Src = _Sext(32, Src);
      else
        Src = _Zext(32, Src);
    }
  }

  if (Signed)
    Src = _Float_FromGPR_S(Src, DstElementSize);
  else
    Src = _Float_FromGPR_U(Src, DstElementSize);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);

  Src = _VInsScalarElement(16, DstElementSize, 0, Dest, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template<size_t SrcElementSize, bool Signed>
void OpDispatchBuilder::CVTFPR_To_GPR(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Signed)
    Src = _Float_ToGPR_ZS(Src, SrcElementSize);
  else
    Src = _Float_ToGPR_ZU(Src, SrcElementSize);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, SrcElementSize, -1);
}

template<size_t SrcElementSize, bool Signed, bool Widen>
void OpDispatchBuilder::Vector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);
  if (Widen) {
    Src = _VSXTL(Src, Size, ElementSize);
    ElementSize <<= 1;
  }

  if (Signed)
    Src = _Vector_SToF(Src, Size, ElementSize);
  else
    Src = _Vector_UToF(Src, Size, ElementSize);

  StoreResult(FPRClass, Op, Src, -1);
}

template<size_t SrcElementSize, bool Signed, bool Narrow>
void OpDispatchBuilder::Vector_CVT_Float_To_Int(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);

  if (Narrow) {
    Src = _Vector_FToF(Size, SrcElementSize >> 1, SrcElementSize, Src);
    ElementSize >>= 1;
  }

  if (Signed)
    Src = _Vector_FToZS(Src, Size, ElementSize);
  else
    Src = _Vector_FToZU(Src, Size, ElementSize);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Scalar_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  Src = _Float_FToF(DstElementSize, SrcElementSize, Src);
  Src = _VInsScalarElement(16, DstElementSize, 0, Dest, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Vector_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  size_t Size = GetDstSize(Op);

  if (DstElementSize > SrcElementSize) {
    Src = _Vector_FToF(Size, SrcElementSize << 1, SrcElementSize, Src);
  }
  else {
    Src = _Vector_FToF(Size, SrcElementSize >> 1, SrcElementSize, Src);
  }

  StoreResult(FPRClass, Op, Src, -1);
}

template<size_t SrcElementSize, bool Signed, bool Widen>
void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetSrcSize(Op);
  if (Widen) {
    Src = _VSXTL(Src, Size, ElementSize);
    ElementSize <<= 1;
  }

  if (Signed)
    Src = _Vector_SToF(Src, Size, ElementSize);
  else
    Src = _Vector_UToF(Src, Size, ElementSize);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  // Insert the lower bits
  Dest = _VInsElement(GetDstSize(Op), 8, 0, 0, Dest, Src);

  StoreResult(FPRClass, Op, Dest, -1);
}

void OpDispatchBuilder::MOVBetweenGPR_FPR(OpcodeArgs) {
  if (Op->Dest.TypeNone.Type == FEXCore::X86Tables::DecodedOperand::TYPE_GPR &&
      Op->Dest.TypeGPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    // When destination is XMM then it is zext to 128bit
    uint64_t SrcSize = GetSrcSize(Op) * 8;

    while (SrcSize != 128) {
      Src = _Zext(SrcSize, Src);
      SrcSize *= 2;
    }
    StoreResult(FPRClass, Op, Op->Dest, Src, -1);
  }
  else {
    // Destination is GPR or mem
    // Extract from XMM first
    uint8_t ElementSize = 4;
    if (Op->Flags & X86Tables::DecodeFlags::FLAG_REX_WIDENING)
      ElementSize = 8;
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0],Op->Flags, -1);

    Src = _VExtractToGPR(GetSrcSize(Op), ElementSize, Src, 0);

    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
  }
}

void OpDispatchBuilder::TZCNT(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  Src = _FindTrailingZeros(Src);
  StoreResult(GPRClass, Op, Src, -1);

  auto Zero = _Constant(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      _Constant(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Bfe(1, 0, Src));
}

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VFCMPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  OrderedNode *Src2{};
  if (Scalar) {
    Src2 = _VExtractElement(GetDstSize(Op), Size, Dest, 0);
  }
  else {
    Src2 = Dest;
  }
  uint8_t CompType = Op->Src[1].TypeLiteral.Literal;

  OrderedNode *Result{};
  // This maps 1:1 to an AArch64 NEON Op
  //auto ALUOp = _VCMPGT(Size, ElementSize, Dest, Src);
  switch (CompType) {
    case 0x00: case 0x08: case 0x10: case 0x18: // EQ
      Result = _VFCMPEQ(Size, ElementSize, Src2, Src);
    break;
    case 0x01: case 0x09: case 0x11: case 0x19: // LT, GT(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Src2, Src);
    break;
    case 0x02: case 0x0A: case 0x12: case 0x1A: // LE, GE(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Src2, Src);
    break;
    case 0x03: case 0x0B: case 0x13: case 0x1B: // Unordered
      Result = _VFCMPUNO(Size, ElementSize, Src2, Src);
    break;
    case 0x04: case 0x0C: case 0x14: case 0x1C: // NEQ
      Result = _VFCMPNEQ(Size, ElementSize, Src2, Src);
    break;
    case 0x05: case 0x0D: case 0x15: case 0x1D: // NLT, NGT(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Src, Src2);
    break;
    case 0x06: case 0x0E: case 0x16: case 0x1E: // NLE, NGE(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Src, Src2);
    break;
    case 0x07: case 0x0F: case 0x17: case 0x1F: // Ordered
      Result = _VFCMPORD(Size, ElementSize, Src2, Src);
    break;
    default: LogMan::Msg::A("Unknown Comparison type: %d", CompType);
  }

  if (Scalar) {
    // Insert the lower bits
    Result = _VInsScalarElement(GetDstSize(Op), ElementSize, 0, Dest, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode *OpDispatchBuilder::GetX87Top() {
  // Yes, we are storing 3 bits in a single flag register.
  // Deal with it
  return _LoadContext(1, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC, GPRClass);
}

void OpDispatchBuilder::SetX87Top(OrderedNode *Value) {
  _StoreContext(GPRClass, 1, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC, Value);
}

template<size_t width>
void OpDispatchBuilder::FLD(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);
  auto top = _And(_Sub(orig_top, _Constant(1)), mask);
  SetX87Top(top);

  size_t read_width = (width == 80) ? 16 : width / 8;

  OrderedNode *data{};

  if (Op->Src[0].TypeNone.Type != 0) {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);
  }
  else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    data = _And(_Add(top, offset), mask);
    data = _LoadContextIndexed(data, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }
  OrderedNode *converted = data;

  // Convert to 80bit float
  if (width == 32 || width == 64) {
      converted = _F80CVTTo(data, width / 8);
  }
  else if (width == 32 || width == 64) {
    _Zext(32, data);
    if (width == 32)
      data = _Zext(32, data);

    constexpr size_t mantissa_bits = (width == 32) ? 23 : 52;
    constexpr size_t exponent_bits = width - (mantissa_bits + 1);

    uint64_t sign_mask = (width == 32) ? 0x80000000 : 0x8000000000000000;
    uint64_t exponent_mask = (width == 32) ? 0x7F800000 : 0x7FE0000000000000;
    uint64_t lower_mask = (width == 32) ? 0x007FFFFF : 0x001FFFFFFFFFFFFF;
    auto sign = _Lshr(_And(data, _Constant(sign_mask)), _Constant(width - 16));
    auto exponent = _Lshr(_And(data, _Constant(exponent_mask)), _Constant(mantissa_bits));
    auto lower = _Lshl(_And(data, _Constant(lower_mask)), _Constant(63 - mantissa_bits));

    // XXX: Need to handle NaN/Infinities
    constexpr size_t exponent_zero = (1 << (exponent_bits-1));
    auto adjusted_exponent = _Add(exponent, _Constant(0x4000 - exponent_zero));
    auto upper = _Or(sign, adjusted_exponent);

    // XXX: Need to support decoding of denormals
    auto intergerBit = _Constant(1ULL << 63);

    converted = _VCastFromGPR(16, 8, _Or(intergerBit, lower));
    converted = _VInsElement(16, 8, 1, 0, converted, _VCastFromGPR(16, 8, upper));
  }
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  //_StoreContext(converted, 16, offsetof(FEXCore::Core::CPUState, mm[7][0]));
}

template<uint64_t Lower, uint32_t Upper>
void OpDispatchBuilder::FLD_Const(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto low = _Constant(Lower);
  auto high = _Constant(Upper);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, data, high, 1);
  // Write to ST[TOP]
  _StoreContextIndexed(data, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<size_t width>
void OpDispatchBuilder::FILD(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  size_t read_width = width / 8;

  // Read from memory
  auto data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);

  auto zero = _Constant(0);

  // Sign extend to 64bits
  if (width != 64)
    data = _Sext(width, data);

  // Extract sign and make interger absolute
  auto sign = _Select(COND_SLT, data, zero, _Constant(0x8000), zero);
  auto absolute =  _Select(COND_SLT, data, zero, _Sub(zero, data), data);

  // left justify the absolute interger
  auto shift = _Sub(_Constant(63), _FindMSB(absolute));
  auto shifted = _Lshl(absolute, shift);

  auto adjusted_exponent = _Sub(_Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(sign, zeroed_exponent);


  OrderedNode *converted = _VCastFromGPR(16, 8, shifted);
  converted = _VInsElement(16, 8, 1, 0, converted, _VCastFromGPR(16, 8, upper));

  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<size_t width, bool pop>
void OpDispatchBuilder::FST(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  if (width == 80) {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, data, 10, 1);
  }
  else if (width == 32 || width == 64) {
    auto result = _F80CVT(data, width / 8);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, width / 8, 1);
  }

  if (pop) {
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template<size_t width, bool pop>
void OpDispatchBuilder::FIST(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  OrderedNode *data = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  data = _F80CVTInt(data, width / 8);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, width / 8, 1);

  if (pop) {
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template <size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FADD(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode *StackLocation = top;

  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (Op->Src[0].TypeNone.Type != 0) {
    // Memory arg
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  auto result = _F80Add(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FMUL(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (Op->Src[0].TypeNone.Type != 0) {
    // Memory arg
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80Mul(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FDIV(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (Op->Src[0].TypeNone.Type != 0) {
    // Memory arg
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  OrderedNode *result{};
  if (reverse) {
    result = _F80Div(b, a);
  }
  else {
    result = _F80Div(a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FSUB(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (Op->Src[0].TypeNone.Type != 0) {
    // Memory arg
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  OrderedNode *result{};
  if (reverse) {
    result = _F80Sub(b, a);
  }
  else {
    result = _F80Sub(a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FCHS(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto low = _Constant(0);
  auto high = _Constant(0b1'000'0000'0000'0000);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, data, high, 1);

  auto result = _VXor(a, data, 16, 1);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FABS(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0b0'111'1111'1111'1111);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, data, high, 1);

  auto result = _VAnd(a, data, 16, 1);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FTST(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto low = _Constant(0);
  OrderedNode *data = _VCastFromGPR(16, 8, low);

  OrderedNode *Res = _F80Cmp(a, data,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(HostFlag_ZF, HostFlag_Unordered);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
}

void OpDispatchBuilder::FRNDINT(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80Round(a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FXTRACT(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto exp = _F80XTRACT_EXP(a);
  auto sig = _F80XTRACT_SIG(a);

  // Write to ST[TOP]
  _StoreContextIndexed(exp, orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  _StoreContextIndexed(sig, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  SetX87Top(_Constant(0));
}

template<size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool pop>
void OpDispatchBuilder::FCOMI(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto mask = _Constant(7);

  OrderedNode *arg{};
  OrderedNode *b{};

  if (Op->Src[0].TypeNone.Type != 0) {
    // Memory arg
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  OrderedNode *Res = _F80Cmp(a, b,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(HostFlag_ZF, HostFlag_Unordered);

  if (whichflags == FCOMIFlags::FLAGS_X87) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
  }
  else {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(HostFlag_ZF);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(HostFlag_Unordered);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }
}

void OpDispatchBuilder::FXCH(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(_Add(top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  // Write to ST[TOP]
  _StoreContextIndexed(b, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  _StoreContextIndexed(a, arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<bool pop>
void OpDispatchBuilder::FST(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(_Add(top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  // Write to ST[TOP]
  _StoreContextIndexed(a, arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  if (pop) {
    top = _And(_Add(top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87UnaryOp(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80Round(a);
  // Overwrite the op
  result.first->Header.Op = IROp;

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87BinaryOp(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  auto top = GetX87Top();

  auto mask = _Constant(7);
  OrderedNode *st1 = _And(_Add(top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80Add(a, st1);
  // Overwrite the op
  result.first->Header.Op = IROp;

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<bool Inc>
void OpDispatchBuilder::X87ModifySTP(OpcodeArgs) {
  auto orig_top = GetX87Top();
  if (Inc) {
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
  else {
    auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

void OpDispatchBuilder::X87SinCos(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto sin = _F80SIN(a);
  auto cos = _F80COS(a);

  // Write to ST[TOP]
  _StoreContextIndexed(sin, orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  _StoreContextIndexed(cos, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

template<bool Plus1>
void OpDispatchBuilder::X87FYL2X(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  if (Plus1) {
    auto low = _Constant(0x8000'0000'0000'0000);
    auto high = _Constant(0b0'011'1111'1111'1111);
    OrderedNode *data = _VCastFromGPR(16, 8, low);
    data = _VInsGPR(16, 8, data, high, 1);
    st1 = _F80Add(st1, data);
  }

  auto result = _F80FYL2X(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::X87TAN(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80TAN(a);

  auto low = _Constant(0x8000'0000'0000'0000);
  auto high = _Constant(0b0'011'1111'1111'1111);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, data, high, 1);

  // Write to ST[TOP]
  _StoreContextIndexed(result, orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  _StoreContextIndexed(data, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::X87ATAN(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;

  auto orig_top = GetX87Top();
  auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

  auto result = _F80ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);

  OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);

  // Strip out the FSW information
  auto Top = _Bfe(3, 11, NewFSW);
  SetX87Top(Top);

  auto C0 = _Bfe(1, 8,  NewFSW);
  auto C1 = _Bfe(1, 9,  NewFSW);
  auto C2 = _Bfe(1, 10, NewFSW);
  auto C3 = _Bfe(1, 14, NewFSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
}

void OpDispatchBuilder::X87FNSTENV(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
	// 14 bytes for 16bit
	// 2 Bytes : FCW
	// 2 Bytes : FSW
	// 2 bytes : FTW
	// 2 bytes : Instruction offset
	// 2 bytes : Instruction CS selector
	// 2 bytes : Data offset
	// 2 bytes : Data selector

	// 28 bytes for 32bit
	// 4 bytes : FCW
	// 4 bytes : FSW
	// 4 bytes : FTW
	// 4 bytes : Instruction pointer
	// 2 bytes : instruction pointer selector
	// 2 bytes : Opcode
	// 4 bytes : data pointer offset
	// 4 bytes : data pointer selector

  auto Size = GetDstSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

	{
		// FCW store default
		_StoreMem(GPRClass, Size, Mem, _Constant(0x37F), Size);
	}

	{
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
		// We must construct the FSW from our various bits
		OrderedNode *FSW = _Constant(0);
		auto Top = GetX87Top();
		FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

		auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
		auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
		auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
		auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

		FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
		FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
		FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
		FSW = _Or(FSW, _Lshl(C3, _Constant(14)));
    _StoreMem(GPRClass, Size, MemLocation, FSW, Size);
	}

	auto ZeroConst = _Constant(0);

	{
		// FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Instruction Offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Data pointer offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Data pointer selector
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}
}

void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  StoreResult(GPRClass, Op, _Constant(0x37F), -1);
}

void OpDispatchBuilder::X87LDSW(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  OrderedNode *NewFSW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Strip out the FSW information
  auto Top = _Bfe(3, 11, NewFSW);
  SetX87Top(Top);

  auto C0 = _Bfe(1, 8,  NewFSW);
  auto C1 = _Bfe(1, 9,  NewFSW);
  auto C2 = _Bfe(1, 10, NewFSW);
  auto C3 = _Bfe(1, 14, NewFSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
}

void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  // We must construct the FSW from our various bits
  OrderedNode *FSW = _Constant(0);
  auto Top = GetX87Top();
  FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

  auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

  FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
  FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
  FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
  FSW = _Or(FSW, _Lshl(C3, _Constant(14)));

  StoreResult(GPRClass, Op, FSW, -1);
}

void OpDispatchBuilder::X87FNSAVE(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
	// 14 bytes for 16bit
	// 2 Bytes : FCW
	// 2 Bytes : FSW
	// 2 bytes : FTW
	// 2 bytes : Instruction offset
	// 2 bytes : Instruction CS selector
	// 2 bytes : Data offset
	// 2 bytes : Data selector

	// 28 bytes for 32bit
	// 4 bytes : FCW
	// 4 bytes : FSW
	// 4 bytes : FTW
	// 4 bytes : Instruction pointer
	// 2 bytes : instruction pointer selector
	// 2 bytes : Opcode
	// 4 bytes : data pointer offset
	// 4 bytes : data pointer selector

  auto Size = GetDstSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

  OrderedNode *Top = GetX87Top();
	{
		// FCW store default
		_StoreMem(GPRClass, Size, Mem, _Constant(0x37F), Size);
	}

	{
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
		// We must construct the FSW from our various bits
		OrderedNode *FSW = _Constant(0);
		FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

		auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
		auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
		auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
		auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

		FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
		FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
		FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
		FSW = _Or(FSW, _Lshl(C3, _Constant(14)));
    _StoreMem(GPRClass, Size, MemLocation, FSW, Size);
	}

	auto ZeroConst = _Constant(0);

	{
		// FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Instruction Offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Data pointer offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

	{
		// Data pointer selector
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
	}

  OrderedNode *ST0Location = _Add(Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);
  for (int i = 0; i < 7; ++i) {
    auto data = _LoadContextIndexed(Top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
    _StoreMem(FPRClass, 16, ST0Location, data, 1);
    ST0Location = _Add(ST0Location, TenConst);
    Top = _And(_Add(Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  auto data = _LoadContextIndexed(Top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, 8, ST0Location, data, 1);
  ST0Location = _Add(ST0Location, _Constant(8));
  _VStoreMemElement(16, 2, ST0Location, data, 4, 1);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);

  OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);

  // Strip out the FSW information
  OrderedNode *Top = _Bfe(3, 11, NewFSW);
  SetX87Top(Top);

  auto C0 = _Bfe(1, 8,  NewFSW);
  auto C1 = _Bfe(1, 9,  NewFSW);
  auto C2 = _Bfe(1, 10, NewFSW);
  auto C3 = _Bfe(1, 14, NewFSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);

  OrderedNode *ST0Location = _Add(Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0xFFFF);
  OrderedNode *Mask = _VCastFromGPR(16, 8, low);
  Mask = _VInsGPR(16, 8, Mask, high, 1);

  for (int i = 0; i < 7; ++i) {
    OrderedNode *Reg = _LoadMem(FPRClass, 16, ST0Location, 1);
    // Mask off the top bits
    Reg = _VAnd(16, 16, Reg, Mask);

    _StoreContextIndexed(Reg, Top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);

    ST0Location = _Add(ST0Location, TenConst);
    Top = _And(_Add(Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  OrderedNode *Reg = _LoadMem(FPRClass, 8, ST0Location, 1);
  ST0Location = _Add(ST0Location, _Constant(8));
  Reg = _VLoadMemElement(16, 2, ST0Location, Reg, 4, 1);
  _StoreContextIndexed(Reg, Top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  OrderedNode *Result = _VExtractToGPR(16, 8, a, 1);

  // Extract the sign bit
  Result = _Lshr(Result, _Constant(15));
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Result);

  // Claim this is a normal number
  // We don't support anything else
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(OneConst);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(ZeroConst);
}

void OpDispatchBuilder::X87FCMOV(OpcodeArgs) {
  Current_Header->ShouldInterpret = true;
  enum CompareType {
    COMPARE_ZERO,
    COMPARE_NOTZERO,
  };
  uint32_t FLAGMask{};
  CompareType Type = COMPARE_ZERO;
  OrderedNode *SrcCond;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  uint16_t Opcode = Op->OP & 0b1111'1111'1000;
  switch (Opcode) {
  case 0x3'C0:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x2'C0:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x2'C8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'C8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x2'D0:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'D0:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_ZERO;
  break;
  case 0x2'D8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'D8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_ZERO;
  break;
  default:
    LogMan::Msg::A("Unhandled FCMOV op: 0x%x", Opcode);
  break;
  }

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
  }

  SrcCond = _Sbfe(1, 0, SrcCond);

  OrderedNode *VecCond = _VCastFromGPR(16, 8, SrcCond);
  VecCond = _VInsGPR(16, 8, VecCond, SrcCond, 1);

  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(_Add(top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
  auto Result = _VBSL(VecCond, b, a);

  // Write to ST[TOP]
  _StoreContextIndexed(Result, top, 16, offsetof(FEXCore::Core::CPUState, mm[0][0]), 16, FPRClass);
}

void OpDispatchBuilder::FXSaveOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

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

  {
    // FCW store default
    _StoreMem(GPRClass, 2, Mem, _Constant(0x37F), 2);
  }

  {
    // We must construct the FSW from our various bits
    OrderedNode *MemLocation = _Add(Mem, _Constant(2));
    OrderedNode *FSW = _Constant(0);
    auto Top = GetX87Top();
    FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

    auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
    auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
    auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
    auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

    FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
    FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
    FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
    FSW = _Or(FSW, _Lshl(C3, _Constant(14)));
    _StoreMem(GPRClass, 2, MemLocation, FSW, 2);
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
    OrderedNode *MMReg = _LoadContext(16, offsetof(FEXCore::Core::CPUState, mm[i]), FPRClass);
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));

    _StoreMem(FPRClass, 16, MemLocation, MMReg, 16);
  }
  for (unsigned i = 0; i < 16; ++i) {
    OrderedNode *XMMReg = _LoadContext(16, offsetof(FEXCore::Core::CPUState, xmm[i]), FPRClass);
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));

    _StoreMem(FPRClass, 16, MemLocation, XMMReg, 16);
  }
}

void OpDispatchBuilder::FXRStoreOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  {
    OrderedNode *MemLocation = _Add(Mem, _Constant(2));
    auto NewFSW = _LoadMem(GPRClass, 2, MemLocation, 2);

    // Strip out the FSW information
    auto Top = _Bfe(3, 11, NewFSW);
    SetX87Top(Top);

    auto C0 = _Bfe(1, 8,  NewFSW);
    auto C1 = _Bfe(1, 9,  NewFSW);
    auto C2 = _Bfe(1, 10, NewFSW);
    auto C3 = _Bfe(1, 14, NewFSW);

    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
  }

  for (unsigned i = 0; i < 8; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));
    auto MMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    _StoreContext(FPRClass, 16, offsetof(FEXCore::Core::CPUState, mm[i]), MMReg);
  }
  for (unsigned i = 0; i < 16; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));
    auto XMMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    _StoreContext(FPRClass, 16, offsetof(FEXCore::Core::CPUState, xmm[i]), XMMReg);
  }
}

void OpDispatchBuilder::PAlignrOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  uint8_t Index = Op->Src[1].TypeLiteral.Literal;
  OrderedNode *Res = _VExtr(GetDstSize(Op), 1, Src1, Src2, Index);
  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::UCOMISxOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res = _FCmp(Src1, Src2, ElementSize,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(HostFlag_ZF, HostFlag_Unordered);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(HostFlag_ZF);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(HostFlag_Unordered);

  auto ZeroConst = _Constant(0);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
}

void OpDispatchBuilder::LDMXCSR(OpcodeArgs) {
}

void OpDispatchBuilder::STMXCSR(OpcodeArgs) {
  // Default MXCSR
  StoreResult(GPRClass, Op, _Constant(0x1F80), -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKUSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VSQXTUN(Size, ElementSize, Dest);
  Res = _VSQXTUN2(Size, ElementSize, Res, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKSSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VSQXTN(Size, ElementSize, Dest);
  Res = _VSQXTN2(Size, ElementSize, Res, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PMULLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};

  if (Size == 8) {
    if (Signed) {
      Res = _VSMull(16, ElementSize, Src1, Src2);
    }
    else {
      Res = _VUMull(16, ElementSize, Src1, Src2);
    }
  }
  else {
    OrderedNode* Srcs1[2]{};
    OrderedNode* Srcs2[2]{};

    Srcs1[0] = _VExtr(Size, ElementSize, Src1, Src1, 0);
    Srcs1[1] = _VExtr(Size, ElementSize, Src1, Src1, 2);

    Srcs2[0] = _VExtr(Size, ElementSize, Src2, Src2, 0);
    Srcs2[1] = _VExtr(Size, ElementSize, Src2, Src2, 2);

    Src1 = _VInsElement(Size, ElementSize, 1, 0, Srcs1[0], Srcs1[1]);
    Src2 = _VInsElement(Size, ElementSize, 1, 0, Srcs2[0], Srcs2[1]);

    if (Signed) {
      Res = _VSMull(Size, ElementSize, Src1, Src2);
    }
    else {
      Res = _VUMull(Size, ElementSize, Src1, Src2);
    }
  }
  StoreResult(FPRClass, Op, Res, -1);
}

template<bool ToXMM>
void OpDispatchBuilder::MOVQ2DQ(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This instruction is a bit special in that if the source is MMX then it zexts to 128bit
  if (ToXMM) {
    Src = _VMov(Src, 16);
    _StoreContext(FPRClass, 16, offsetof(FEXCore::Core::CPUState, xmm[Op->Dest.TypeGPR.GPR - FEXCore::X86State::REG_XMM_0][0]), Src);
  }
  else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, -1);
  }
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PADDSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if (Signed)
    Res = _VSQAdd(Size, ElementSize, Dest, Src);
  else
    Res = _VUQAdd(Size, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PSUBSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if (Signed)
    Res = _VSQSub(Size, ElementSize, Dest, Src);
  else
    Res = _VUQSub(Size, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::PMADDWD(OpcodeArgs) {
  // This is a pretty curious operation
  // Does two MADD operations across 4 16bit signed integers and accumulates to 32bit integers in the destination
  //
  // x86 PMADDWD: xmm1, xmm2
  //              xmm1[31:0]  = (xmm1[15:0] * xmm2[15:0]) + (xmm1[31:16] * xmm2[31:16])
  //              xmm1[63:32] = (xmm1[47:32] * xmm2[47:32]) + (xmm1[63:48] * xmm2[63:48])
  //              etc.. for larger registers

  auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Size == 8) {
    Size <<= 1;
    Src1 = _VBitcast(Size, 2, Src1);
    Src2 = _VBitcast(Size, 2, Src2);
  }

  auto Src1_L = _VSXTL(Size, 2, Src1);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src1_H = _VSXTL2(Size, 2, Src1); // [79:64], [95:80], [111:96], [127:112]

  auto Src2_L = _VSXTL(Size, 2, Src2);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src2_H = _VSXTL2(Size, 2, Src2); // [79:64], [95:80], [111:96], [127:112]

  auto Res_L = _VSMul(Size, 4, Src1_L, Src2_L); // [15:0 ], [31:16], [32:47 ], [63:48  ] : Original elements
  auto Res_H = _VSMul(Size, 4, Src1_H, Src2_H); // [79:64], [95:80], [111:96], [127:112] : Original elements

  // [15:0 ] + [31:16], [32:47 ] + [63:48  ], [79:64] + [95:80], [111:96] + [127:112]
  auto Res = _VAddP(Size, 4, Res_L, Res_H);
  StoreResult(FPRClass, Op, Res, -1);
}

template<bool Signed>
void OpDispatchBuilder::PMULHW(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if (Size == 8) {
    Dest = _VBitcast(Size * 2, 2, Dest);
    Src = _VBitcast(Size * 2, 2, Src);

    // Implementation is more efficient for 8byte registers
    if (Signed)
      Res = _VSMull(Size * 2, 2, Dest, Src);
    else
      Res = _VUMull(Size * 2, 2, Dest, Src);

    Res = _VUShrNI(Size * 2, 4, Res, 16);
  }
  else {
    // 128bit is less efficient
    OrderedNode *ResultLow;
    OrderedNode *ResultHigh;
    if (Signed) {
      ResultLow = _VSMull(Size, 2, Dest, Src);
      ResultHigh = _VSMull2(Size, 2, Dest, Src);
    }
    else {
      ResultLow = _VUMull(Size, 2, Dest, Src);
      ResultHigh = _VUMull2(Size, 2, Dest, Src);
    }

    // Combine the results
    Res = _VUShrNI(Size, 4, ResultLow, 16);
    Res = _VUShrNI2(Size, 4, Res, ResultHigh, 16);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::MOVBEOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, 1);
  Src = _Rev(Src);
  StoreResult(GPRClass, Op, Src, 1);
}

template<size_t ElementSize>
void OpDispatchBuilder::HADDP(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VFAddP(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::HSUBP(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This is a bit complicated since AArch64 doesn't support a pairwise subtract
  auto Dest_Neg = _VFNeg(Size, ElementSize, Dest);
  auto Src_Neg = _VFNeg(Size, ElementSize, Src);

  // Now we need to swizzle the values
  OrderedNode *Swizzle_Dest = Dest;
  OrderedNode *Swizzle_Src = Src;

  if (ElementSize == 4) {
    Swizzle_Dest = _VInsElement(Size, ElementSize, 0, 0, Swizzle_Dest, Dest_Neg);
    Swizzle_Dest = _VInsElement(Size, ElementSize, 2, 2, Swizzle_Dest, Dest_Neg);

    Swizzle_Src = _VInsElement(Size, ElementSize, 0, 0, Swizzle_Src, Src_Neg);
    Swizzle_Src = _VInsElement(Size, ElementSize, 2, 2, Swizzle_Src, Src_Neg);
  }
  else {
    Swizzle_Dest = _VInsElement(Size, ElementSize, 0, 0, Swizzle_Dest, Dest_Neg);
    Swizzle_Src = _VInsElement(Size, ElementSize, 0, 0, Swizzle_Src, Src_Neg);
  }

  OrderedNode *Res = _VFAddP(Size, ElementSize, Swizzle_Dest, Swizzle_Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template<uint8_t FenceType>
void OpDispatchBuilder::FenceOp(OpcodeArgs) {
  _Fence({FenceType});
}

void OpDispatchBuilder::PSADBW(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *SubResult = _VSub(Size, 1, Dest, Src);
  OrderedNode *AbsResult = _VAbs(Size, 1, SubResult);

  OrderedNode *Result{};

  if (Size == 8) {
    // First we need to extract the 8bit values and extend them to 16bits
    OrderedNode *Result_Low = _VUXTL(Size, 1, AbsResult);
    // Now extract the upper 8bits and extend to 16bits
    OrderedNode *Result_High = _VUShrI(Size, 2, AbsResult, 8);
    // Now vector-wide add the results for each
    Result_Low = _VAddV(Size, 2, Result_Low);
    Result_High = _VAddV(Size, 2, Result_High);
    // Now add the two scalar results back together
    // Easy with only 8 byte registers
    Result = _VAdd(Size, 2, Result_Low, Result_High);
  }
  else {
    // For 16byte registers we need to do a bit more work
    // First we need to extract the 8bit values and extend them to 16bits
    OrderedNode *Result_Low_Low = _VUXTL(Size, 1, AbsResult);
    //// Then we need to extract the upper 8 bits of the lower 64bit result and extend them to 16bits
    OrderedNode *Result_Low_High = _VUShrI(Size, 2, AbsResult, 8);

    // Now we need to extract the upper 64bits and do the same thing
    OrderedNode *Result_High = _VExtractElement(Size, 8, AbsResult, 1);
    OrderedNode *Result_High_Low = _VUXTL(Size, 1, Result_High);
    OrderedNode *Result_High_High = _VUShrI(Size, 2, Result_High, 8);

    // Now vector pairwise add all four of these
    Result_Low_Low  = _VAddV(8, 2, Result_Low_Low);
    Result_Low_High = _VAddV(8, 2, Result_Low_High);

    Result_High_Low  = _VAddV(8, 2, Result_High_Low);
    Result_High_High = _VAddV(8, 2, Result_High_High);

    //// Now add the low and high scalar results together
    OrderedNode * Added_Result      = _VAdd(8, 2, Result_Low_Low, Result_Low_High);
    OrderedNode * Added_Result_High = _VAdd(8, 2, Result_High_Low, Result_High_High);

    // Now generate a new 128bit value for this
    Result = _VInsElement(Size, 8, 1, 0, Added_Result, Added_Result_High);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::UnimplementedOp(OpcodeArgs) {
  uint8_t GPRSize = CTX->Config.Is64BitMode ? 8 : 4;

  // We don't actually support this instruction
  // Multiblock may hit it though
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), _Constant(Op->PC));
  _Break(0, 0);
  BlockSetRIP = true;

  auto NextBlock = CreateNewCodeBlock();
  SetCurrentCodeBlock(NextBlock);
}

#undef OpcodeArgs

void InstallOpcodeHandlers(Context::OperatingMode Mode) {
  const std::vector<std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr>> BaseOpTable = {
    // Instructions
    {0x00, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x01, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x02, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x03, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x04, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x05, 1, &OpDispatchBuilder::ALUOp<0>},

    {0x08, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x09, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x0A, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x0B, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x0C, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x0D, 1, &OpDispatchBuilder::ALUOp<0>},

    {0x10, 1, &OpDispatchBuilder::ADCOp<0>},
    {0x11, 1, &OpDispatchBuilder::ADCOp<0>},
    {0x12, 1, &OpDispatchBuilder::ADCOp<0>},
    {0x13, 1, &OpDispatchBuilder::ADCOp<0>},
    {0x14, 1, &OpDispatchBuilder::ADCOp<0>},
    {0x15, 1, &OpDispatchBuilder::ADCOp<0>},

    {0x18, 1, &OpDispatchBuilder::SBBOp<0>},
    {0x19, 1, &OpDispatchBuilder::SBBOp<0>},
    {0x1A, 1, &OpDispatchBuilder::SBBOp<0>},
    {0x1B, 1, &OpDispatchBuilder::SBBOp<0>},
    {0x1C, 1, &OpDispatchBuilder::SBBOp<0>},
    {0x1D, 1, &OpDispatchBuilder::SBBOp<0>},

    {0x20, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x21, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x22, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x23, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x24, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x25, 1, &OpDispatchBuilder::ALUOp<0>},

    {0x28, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x29, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x2A, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x2B, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x2C, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x2D, 1, &OpDispatchBuilder::ALUOp<0>},

    {0x30, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x31, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x32, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x33, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x34, 1, &OpDispatchBuilder::ALUOp<0>},
    {0x35, 1, &OpDispatchBuilder::ALUOp<0>},

    {0x38, 6, &OpDispatchBuilder::CMPOp<0>},
    {0x50, 8, &OpDispatchBuilder::PUSHREGOp},
    {0x58, 8, &OpDispatchBuilder::POPOp},
    {0x63, 1, &OpDispatchBuilder::MOVSXDOp},
    {0x68, 1, &OpDispatchBuilder::PUSHOp},
    {0x69, 1, &OpDispatchBuilder::IMUL2SrcOp},
    {0x6A, 1, &OpDispatchBuilder::PUSHOp},
    {0x6B, 1, &OpDispatchBuilder::IMUL2SrcOp},

    {0x70, 16, &OpDispatchBuilder::CondJUMPOp},
    {0x84, 2, &OpDispatchBuilder::TESTOp<0>},
    {0x86, 2, &OpDispatchBuilder::XCHGOp},
    {0x88, 4, &OpDispatchBuilder::MOVGPROp<0>},

    {0x8C, 1, &OpDispatchBuilder::MOVSegOp<false>},
    {0x8D, 1, &OpDispatchBuilder::LEAOp},
    {0x8E, 1, &OpDispatchBuilder::MOVSegOp<true>},
    {0x8F, 1, &OpDispatchBuilder::POPOp},
    {0x90, 8, &OpDispatchBuilder::XCHGOp},

    {0x98, 1, &OpDispatchBuilder::CDQOp},
    {0x99, 1, &OpDispatchBuilder::CQOOp},
    {0x9B, 1, &OpDispatchBuilder::NOPOp},
    {0x9C, 1, &OpDispatchBuilder::PUSHFOp},
    {0x9D, 1, &OpDispatchBuilder::POPFOp},
    {0x9E, 1, &OpDispatchBuilder::SAHFOp},
    {0x9F, 1, &OpDispatchBuilder::LAHFOp},
    {0xA0, 4, &OpDispatchBuilder::MOVOffsetOp},
    {0xA4, 2, &OpDispatchBuilder::MOVSOp},

    {0xA6, 2, &OpDispatchBuilder::CMPSOp},
    {0xA8, 2, &OpDispatchBuilder::TESTOp<0>},
    {0xAA, 2, &OpDispatchBuilder::STOSOp},
    {0xAC, 2, &OpDispatchBuilder::LODSOp},
    {0xAE, 2, &OpDispatchBuilder::SCASOp},
    {0xB0, 16, &OpDispatchBuilder::MOVGPROp<0>},
    {0xC2, 2, &OpDispatchBuilder::RETOp},
    {0xC8, 1, &OpDispatchBuilder::EnterOp},
    {0xC9, 1, &OpDispatchBuilder::LEAVEOp},
    {0xCC, 2, &OpDispatchBuilder::INTOp},
    {0xD7, 2, &OpDispatchBuilder::XLATOp},
    {0xE0, 1, &OpDispatchBuilder::LoopOp<true, false>},
    {0xE1, 1, &OpDispatchBuilder::LoopOp<true, true>},
    {0xE2, 1, &OpDispatchBuilder::LoopOp<false, false>},
    {0xE3, 1, &OpDispatchBuilder::CondJUMPRCXOp},
    {0xE8, 1, &OpDispatchBuilder::CALLOp},
    {0xE9, 1, &OpDispatchBuilder::JUMPOp},
    {0xEB, 1, &OpDispatchBuilder::JUMPOp},
    {0xF1, 1, &OpDispatchBuilder::INTOp},
    {0xF4, 1, &OpDispatchBuilder::INTOp},

    {0xF5, 1, &OpDispatchBuilder::FLAGControlOp},
    {0xF8, 2, &OpDispatchBuilder::FLAGControlOp},
    {0xFC, 2, &OpDispatchBuilder::FLAGControlOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr>> BaseOpTable_32 = {
    {0x40, 8, &OpDispatchBuilder::INCOp},
    {0x48, 8, &OpDispatchBuilder::DECOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> TwoByteOpTable = {
    // Instructions
    {0x00, 1, nullptr}, // GROUP 6
    {0x01, 1, nullptr}, // GROUP 7
    {0x0B, 1, &OpDispatchBuilder::INTOp},
    {0x0D, 1, nullptr}, // GROUP P
    {0x0E, 1, &OpDispatchBuilder::NOPOp},
    {0x18, 1, nullptr}, // GROUP 16

    {0x19, 7, &OpDispatchBuilder::NOPOp}, // NOP with ModRM

    {0x31, 1, &OpDispatchBuilder::RDTSCOp},

    {0x40, 16, &OpDispatchBuilder::CMOVOp},
    {0x6E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x80, 16, &OpDispatchBuilder::CondJUMPOp},
    {0x90, 16, &OpDispatchBuilder::SETccOp},
    {0xA2, 1, &OpDispatchBuilder::CPUIDOp},
    {0xA3, 1, &OpDispatchBuilder::BTOp<0>}, // BT
    {0xA4, 1, &OpDispatchBuilder::SHLDImmediateOp},
    {0xA5, 1, &OpDispatchBuilder::SHLDOp},
    {0xAB, 1, &OpDispatchBuilder::BTSOp<0>},
    {0xAC, 1, &OpDispatchBuilder::SHRDImmediateOp},
    {0xAD, 1, &OpDispatchBuilder::SHRDOp},
    {0xAF, 1, &OpDispatchBuilder::IMUL1SrcOp},
    {0xB0, 2, &OpDispatchBuilder::CMPXCHGOp}, // CMPXCHG
    {0xB3, 1, &OpDispatchBuilder::BTROp<0>},
    {0xB6, 2, &OpDispatchBuilder::MOVZXOp},
    {0xBB, 1, &OpDispatchBuilder::BTCOp<0>},
    {0xBC, 1, &OpDispatchBuilder::BSFOp}, // BSF
    {0xBD, 1, &OpDispatchBuilder::BSROp}, // BSF
    {0xBE, 2, &OpDispatchBuilder::MOVSXOp},
    {0xC0, 2, &OpDispatchBuilder::XADDOp},
    {0xC3, 1, &OpDispatchBuilder::MOVGPROp<0>},
    {0xC4, 1, &OpDispatchBuilder::PINSROp<2>},
    {0xC5, 1, &OpDispatchBuilder::PExtrOp<2>},
    {0xC8, 8, &OpDispatchBuilder::BSWAPOp},

    // SSE
    {0x10, 2, &OpDispatchBuilder::MOVUPSOp},
    {0x12, 2, &OpDispatchBuilder::MOVLPOp},
    {0x14, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x15, 1, &OpDispatchBuilder::PUNPCKHOp<4>},
    {0x16, 1, &OpDispatchBuilder::MOVLHPSOp},
    {0x17, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x28, 2, &OpDispatchBuilder::MOVUPSOp},
    {0x2A, 1, &OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float<4, true, false>},
    {0x2B, 1, &OpDispatchBuilder::MOVAPSOp},
    {0x2C, 2, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, true, false>},
    {0x2E, 2, &OpDispatchBuilder::UCOMISxOp<4>},
    {0x50, 1, &OpDispatchBuilder::MOVMSKOp<4>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4, false>},
    {0x52, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4, false>},
    {0x53, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4, false>},
    {0x54, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0x55, 1, &OpDispatchBuilder::ANDNOp},
    {0x56, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 16>},
    {0x57, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 16>},
    {0x58, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 4>},
    {0x59, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMUL, 4>},
    {0x5A, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Float<8, 4>},
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 4>},
    {0x5D, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 4>},
    {0x5E, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 4>},
    {0x5F, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 4>},
    {0x60, 1, &OpDispatchBuilder::PUNPCKLOp<1>},
    {0x61, 1, &OpDispatchBuilder::PUNPCKLOp<2>},
    {0x62, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x63, 1, &OpDispatchBuilder::PACKSSOp<2>},
    {0x64, 1, &OpDispatchBuilder::PCMPGTOp<1>},
    {0x65, 1, &OpDispatchBuilder::PCMPGTOp<2>},
    {0x66, 1, &OpDispatchBuilder::PCMPGTOp<4>},
    {0x67, 1, &OpDispatchBuilder::PACKUSOp<2>},
    {0x68, 1, &OpDispatchBuilder::PUNPCKHOp<1>},
    {0x69, 1, &OpDispatchBuilder::PUNPCKHOp<2>},
    {0x6A, 1, &OpDispatchBuilder::PUNPCKHOp<4>},
    {0x6B, 1, &OpDispatchBuilder::PACKSSOp<4>},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, false, true>},
    {0x71, 1, nullptr}, // GROUP 12
    {0x72, 1, nullptr}, // GROUP 13
    {0x73, 1, nullptr}, // GROUP 14

    {0x74, 1, &OpDispatchBuilder::PCMPEQOp<1>},
    {0x75, 1, &OpDispatchBuilder::PCMPEQOp<2>},
    {0x76, 1, &OpDispatchBuilder::PCMPEQOp<4>},
    {0x77, 1, &OpDispatchBuilder::NOPOp},

    {0xAE, 1, nullptr}, // GROUP 15
    {0xB9, 1, nullptr}, // GROUP 10
    {0xBA, 1, nullptr}, // GROUP 8

    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<4, false>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<4>},
    {0xC7, 1, nullptr}, // GROUP 9

    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2, true, 0>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4, true, 0>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8, true, 0>},
    {0xD4, 1, &OpDispatchBuilder::PADDQOp<8>},
    {0xD5, 1, &OpDispatchBuilder::PMULOp<2, true>},
    {0xD8, 1, &OpDispatchBuilder::PSUBSOp<1, false>},
    {0xD9, 1, &OpDispatchBuilder::PSUBSOp<2, false>},
    {0xDA, 1, &OpDispatchBuilder::PMINUOp<1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 8>},
    {0xDC, 1, &OpDispatchBuilder::PADDSOp<1, false>},
    {0xDD, 1, &OpDispatchBuilder::PADDSOp<2, false>},
    {0xDE, 1, &OpDispatchBuilder::PMAXUOp<1>},
    {0xDF, 1, &OpDispatchBuilder::ANDNOp},
    {0xE0, 1, &OpDispatchBuilder::PAVGOp<1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2, true, 0>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4, true, 0>},
    {0xE3, 1, &OpDispatchBuilder::PAVGOp<2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE7, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xE8, 1, &OpDispatchBuilder::PSUBSOp<1, true>},
    {0xE9, 1, &OpDispatchBuilder::PSUBSOp<2, true>},
    {0xEA, 1, &OpDispatchBuilder::PMINSWOp},
    {0xEB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 8>},
    {0xEC, 1, &OpDispatchBuilder::PADDSOp<1, true>},
    {0xED, 1, &OpDispatchBuilder::PADDSOp<2, true>},
    {0xEE, 1, &OpDispatchBuilder::PMAXSWOp},
    {0xEF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 8>},

    {0xF1, 1, &OpDispatchBuilder::PSLL<2, true, 0>},
    {0xF2, 1, &OpDispatchBuilder::PSLL<4, true, 0>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8, true, 0>},
    {0xF4, 1, &OpDispatchBuilder::PMULLOp<4, false>},
    {0xF5, 1, &OpDispatchBuilder::PMADDWD},
    {0xF6, 1, &OpDispatchBuilder::PSADBW},
    {0xF8, 1, &OpDispatchBuilder::PSUBQOp<1>},
    {0xF9, 1, &OpDispatchBuilder::PSUBQOp<2>},
    {0xFA, 1, &OpDispatchBuilder::PSUBQOp<4>},
    {0xFB, 1, &OpDispatchBuilder::PSUBQOp<8>},
    {0xFC, 1, &OpDispatchBuilder::PADDQOp<1>},
    {0xFD, 1, &OpDispatchBuilder::PADDQOp<2>},
    {0xFE, 1, &OpDispatchBuilder::PADDQOp<4>},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> TwoByteOpTable_32 = {
    {0x05, 1, &OpDispatchBuilder::NOPOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> TwoByteOpTable_64 = {
    {0x05, 1, &OpDispatchBuilder::SyscallOp},
  };

#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> PrimaryGroupOpTable = {
    // GROUP 1
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 3), 1, &OpDispatchBuilder::SBBOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 7), 1, &OpDispatchBuilder::CMPOp<1>}, // CMP

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 3), 1, &OpDispatchBuilder::SBBOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 7), 1, &OpDispatchBuilder::CMPOp<1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 3), 1, &OpDispatchBuilder::SBBOp<1>}, // Unit tests find this setting flags incorrectly
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6), 1, &OpDispatchBuilder::SecondaryALUOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 7), 1, &OpDispatchBuilder::CMPOp<1>},

    // GROUP 2
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, &OpDispatchBuilder::ROLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, &OpDispatchBuilder::RORImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, &OpDispatchBuilder::ROLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, &OpDispatchBuilder::RORImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, &OpDispatchBuilder::ROLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, &OpDispatchBuilder::ROROp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, &OpDispatchBuilder::ROLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, &OpDispatchBuilder::ROROp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, &OpDispatchBuilder::ROLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, &OpDispatchBuilder::ROROp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, &OpDispatchBuilder::SHLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, &OpDispatchBuilder::SHROp<false>}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, &OpDispatchBuilder::ASHROp<false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, &OpDispatchBuilder::ROLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, &OpDispatchBuilder::ROROp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, &OpDispatchBuilder::SHLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, &OpDispatchBuilder::SHROp<false>}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, &OpDispatchBuilder::ASHROp<false>}, // SAR

    // GROUP 3
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, &OpDispatchBuilder::TESTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, &OpDispatchBuilder::MULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, &OpDispatchBuilder::DIVOp}, // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, &OpDispatchBuilder::TESTOp<1>},
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
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC6), 0), 1, &OpDispatchBuilder::MOVGPROp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC7), 0), 1, &OpDispatchBuilder::MOVGPROp<1>},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> RepModOpTable = {
    {0x10, 2, &OpDispatchBuilder::MOVSSOp},
    {0x12, 1, &OpDispatchBuilder::MOVSLDUPOp},
    {0x16, 1, &OpDispatchBuilder::MOVSHDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x2A, 1, &OpDispatchBuilder::CVTGPR_To_FPR<4, true>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    {0x2D, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4, true>},
    {0x52, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4, true>},
    {0x53, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4, true>},
    {0x58, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFADD, 4>},
    {0x59, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMUL, 4>},
    {0x5A, 1, &OpDispatchBuilder::Scalar_CVT_Float_To_Float<8, 4>},
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, true, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFSUB, 4>},
    {0x5D, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMIN, 4>},
    {0x5E, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFDIV, 4>},
    {0x5F, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMAX, 4>},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, true, false>},
    {0x7E, 1, &OpDispatchBuilder::MOVQOp},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xB8, 1, &OpDispatchBuilder::PopcountOp},
    {0xBC, 2, &OpDispatchBuilder::TZCNT},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<4, true>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true, true>},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> RepNEModOpTable = {
    {0x10, 2, &OpDispatchBuilder::MOVSDOp},
    {0x12, 1, &OpDispatchBuilder::MOVDDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x2A, 1, &OpDispatchBuilder::CVTGPR_To_FPR<8, true>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, true>},
    {0x2D, 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, true>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8, true>},
    //x52 = Invalid
    {0x58, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFADD, 8>},
    {0x59, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMUL, 8>},
    {0x5A, 1, &OpDispatchBuilder::Scalar_CVT_Float_To_Float<4, 8>},
    {0x5C, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFSUB, 8>},
    {0x5D, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMIN, 8>},
    {0x5E, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFDIV, 8>},
    {0x5F, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMAX, 8>},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, true, true>},
    {0x7C, 1, &OpDispatchBuilder::HADDP<4>},
    {0x7D, 1, &OpDispatchBuilder::HSUBP<4>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<false>},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<8, true>},
    {0xF0, 1, &OpDispatchBuilder::MOVVectorOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> OpSizeModOpTable = {
    {0x10, 2, &OpDispatchBuilder::MOVVectorOp},
    {0x12, 2, &OpDispatchBuilder::MOVLPOp},
    {0x14, 1, &OpDispatchBuilder::PUNPCKLOp<8>},
    {0x15, 1, &OpDispatchBuilder::PUNPCKHOp<8>},
    {0x16, 2, &OpDispatchBuilder::MOVHPDOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x28, 2, &OpDispatchBuilder::MOVAPSOp},
    {0x2E, 2, &OpDispatchBuilder::UCOMISxOp<8>},

    {0x40, 16, &OpDispatchBuilder::CMOVOp},
    {0x50, 1, &OpDispatchBuilder::MOVMSKOp<8>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8, false>},
    {0x54, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0x55, 1, &OpDispatchBuilder::ANDNOp},
    {0x56, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 16>},
    {0x57, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 16>},
    {0x58, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 8>},
    {0x59, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMUL, 8>},
    {0x5A, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Float<4, 8>},
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, true, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 8>},
    {0x5D, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 8>},
    {0x5E, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 8>},
    {0x5F, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 8>},
    {0x60, 1, &OpDispatchBuilder::PUNPCKLOp<1>},
    {0x61, 1, &OpDispatchBuilder::PUNPCKLOp<2>},
    {0x62, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x63, 1, &OpDispatchBuilder::PACKSSOp<2>},
    {0x64, 1, &OpDispatchBuilder::PCMPGTOp<1>},
    {0x65, 1, &OpDispatchBuilder::PCMPGTOp<2>},
    {0x66, 1, &OpDispatchBuilder::PCMPGTOp<4>},
    {0x67, 1, &OpDispatchBuilder::PACKUSOp<2>},
    {0x68, 1, &OpDispatchBuilder::PUNPCKHOp<1>},
    {0x69, 1, &OpDispatchBuilder::PUNPCKHOp<2>},
    {0x6A, 1, &OpDispatchBuilder::PUNPCKHOp<4>},
    {0x6B, 1, &OpDispatchBuilder::PACKSSOp<4>},
    {0x6C, 1, &OpDispatchBuilder::PUNPCKLOp<8>},
    {0x6D, 1, &OpDispatchBuilder::PUNPCKHOp<8>},
    {0x6E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<4, false, true>},

    {0x74, 1, &OpDispatchBuilder::PCMPEQOp<1>},
    {0x75, 1, &OpDispatchBuilder::PCMPEQOp<2>},
    {0x76, 1, &OpDispatchBuilder::PCMPEQOp<4>},
    {0x78, 1, nullptr}, // GROUP 17
    {0x7C, 1, &OpDispatchBuilder::HADDP<8>},
    {0x7D, 1, &OpDispatchBuilder::HSUBP<8>},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<8, false>},
    {0xC4, 1, &OpDispatchBuilder::PINSROp<2>},
    {0xC5, 1, &OpDispatchBuilder::PExtrOp<2>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<8>},

    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2, true, 0>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4, true, 0>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8, true, 0>},
    {0xD4, 1, &OpDispatchBuilder::PADDQOp<8>},
    {0xD5, 1, &OpDispatchBuilder::PMULOp<2, true>},
    {0xD6, 1, &OpDispatchBuilder::MOVQOp},
    {0xD7, 1, &OpDispatchBuilder::MOVMSKOp<1>}, // PMOVMSKB
    {0xD8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>},
    {0xD9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>},
    {0xDA, 1, &OpDispatchBuilder::PMINUOp<1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0xDC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>},
    {0xDD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>},
    {0xDE, 1, &OpDispatchBuilder::PMAXUOp<1>},
    {0xDF, 1, &OpDispatchBuilder::ANDNOp},
    {0xE0, 1, &OpDispatchBuilder::PAVGOp<1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2, true, 0>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4, true, 0>},
    {0xE3, 1, &OpDispatchBuilder::PAVGOp<2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, true>},
    {0xE7, 1, &OpDispatchBuilder::MOVVectorOp},
    {0xE8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 1>},
    {0xE9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 2>},
    {0xEA, 1, &OpDispatchBuilder::PMINSWOp},
    {0xEB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 16>},
    {0xEC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 1>},
    {0xED, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 2>},
    {0xEE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 2>},
    {0xEF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 16>},

    {0xF1, 1, &OpDispatchBuilder::PSLL<2, true, 0>},
    {0xF2, 1, &OpDispatchBuilder::PSLL<4, true, 0>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8, true, 0>},
    {0xF4, 1, &OpDispatchBuilder::PMULLOp<4, false>},
    {0xF5, 1, &OpDispatchBuilder::PMADDWD},
    {0xF6, 1, &OpDispatchBuilder::PSADBW},
    {0xF8, 1, &OpDispatchBuilder::PSUBQOp<1>},
    {0xF9, 1, &OpDispatchBuilder::PSUBQOp<2>},
    {0xFA, 1, &OpDispatchBuilder::PSUBQOp<4>},
    {0xFB, 1, &OpDispatchBuilder::PSUBQOp<8>},
    {0xFC, 1, &OpDispatchBuilder::PADDQOp<1>},
    {0xFD, 1, &OpDispatchBuilder::PADDQOp<2>},
    {0xFE, 1, &OpDispatchBuilder::PADDQOp<4>},
  };

constexpr uint16_t PF_NONE = 0;
constexpr uint16_t PF_F3 = 1;
constexpr uint16_t PF_66 = 2;
constexpr uint16_t PF_F2 = 3;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> SecondaryExtensionOpTable = {
    // GROUP 8
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 4), 1, &OpDispatchBuilder::BTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 4), 1, &OpDispatchBuilder::BTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 4), 1, &OpDispatchBuilder::BTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 4), 1, &OpDispatchBuilder::BTOp<1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 5), 1, &OpDispatchBuilder::BTSOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 5), 1, &OpDispatchBuilder::BTSOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 5), 1, &OpDispatchBuilder::BTSOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 5), 1, &OpDispatchBuilder::BTSOp<1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 6), 1, &OpDispatchBuilder::BTROp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 6), 1, &OpDispatchBuilder::BTROp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 6), 1, &OpDispatchBuilder::BTROp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 6), 1, &OpDispatchBuilder::BTROp<1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 7), 1, &OpDispatchBuilder::BTCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 7), 1, &OpDispatchBuilder::BTCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 7), 1, &OpDispatchBuilder::BTCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 7), 1, &OpDispatchBuilder::BTCOp<1>},

    // GROUP 9
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_NONE, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},

    // GROUP 12
    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 2), 1, &OpDispatchBuilder::PSRLI<2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 4), 1, &OpDispatchBuilder::PSRAIOp<2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 6), 1, &OpDispatchBuilder::PSLLI<2>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 2), 1, &OpDispatchBuilder::PSRLI<2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 4), 1, &OpDispatchBuilder::PSRAIOp<2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 6), 1, &OpDispatchBuilder::PSLLI<2>},

    // GROUP 13
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 2), 1, &OpDispatchBuilder::PSRLI<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 4), 1, &OpDispatchBuilder::PSRAIOp<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 6), 1, &OpDispatchBuilder::PSLLI<4>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 2), 1, &OpDispatchBuilder::PSRLI<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 4), 1, &OpDispatchBuilder::PSRAIOp<4>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 6), 1, &OpDispatchBuilder::PSLLI<4>},

    // GROUP 14
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 2), 1, &OpDispatchBuilder::PSRLI<8>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 6), 1, &OpDispatchBuilder::PSLLI<8>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 2), 1, &OpDispatchBuilder::PSRLI<8>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 3), 1, &OpDispatchBuilder::PSRLDQ},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 6), 1, &OpDispatchBuilder::PSLLI<8>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 7), 1, &OpDispatchBuilder::PSLLDQ},

    // GROUP 15
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 0), 1, &OpDispatchBuilder::FXSaveOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 1), 1, &OpDispatchBuilder::FXRStoreOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 2), 1, &OpDispatchBuilder::LDMXCSR},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 3), 1, &OpDispatchBuilder::STMXCSR},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 5), 1, &OpDispatchBuilder::FenceOp<FEXCore::IR::Fence_Load.Val>},      //LFENCE
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 6), 1, &OpDispatchBuilder::FenceOp<FEXCore::IR::Fence_LoadStore.Val>}, //MFENCE
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 7), 1, &OpDispatchBuilder::FenceOp<FEXCore::IR::Fence_Store.Val>},     //SFENCE

    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 0), 1, &OpDispatchBuilder::ReadSegmentReg<OpDispatchBuilder::Segment_FS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 1), 1, &OpDispatchBuilder::ReadSegmentReg<OpDispatchBuilder::Segment_GS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 2), 1, &OpDispatchBuilder::WriteSegmentReg<OpDispatchBuilder::Segment_FS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 3), 1, &OpDispatchBuilder::WriteSegmentReg<OpDispatchBuilder::Segment_GS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 5), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 6), 1, &OpDispatchBuilder::UnimplementedOp},

    // GROUP 16
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 0), 8, &OpDispatchBuilder::NOPOp},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> SecondaryModRMExtensionOpTable = {
    // REG /2
    {((1 << 3) | 0), 1, &OpDispatchBuilder::UnimplementedOp},
  };
#define OPDReg(op, reg) (((op - 0xD8) << 8) | (reg << 3))
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> X87OpTable = {
    {OPDReg(0xD8, 0) | 0x00, 8, &OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 0) | 0x40, 8, &OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 0) | 0x80, 8, &OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 1) | 0x40, 8, &OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 1) | 0x80, 8, &OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xD8, 2) | 0x40, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xD8, 2) | 0x80, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xD8, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xD8, 3) | 0x40, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xD8, 3) | 0x80, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},

    {OPDReg(0xD8, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 4) | 0x40, 8, &OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 4) | 0x80, 8, &OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 5) | 0x40, 8, &OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 5) | 0x80, 8, &OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 6) | 0x40, 8, &OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 6) | 0x80, 8, &OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 7) | 0x40, 8, &OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xD8, 7) | 0x80, 8, &OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xD8, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xD0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xD8, 0xD8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      {OPD(0xD8, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD9, 0) | 0x00, 8, &OpDispatchBuilder::FLD<32>},
    {OPDReg(0xD9, 0) | 0x40, 8, &OpDispatchBuilder::FLD<32>},
    {OPDReg(0xD9, 0) | 0x80, 8, &OpDispatchBuilder::FLD<32>},

    // 1 = Invalid

    {OPDReg(0xD9, 2) | 0x00, 8, &OpDispatchBuilder::FST<32, false>},
    {OPDReg(0xD9, 2) | 0x40, 8, &OpDispatchBuilder::FST<32, false>},
    {OPDReg(0xD9, 2) | 0x80, 8, &OpDispatchBuilder::FST<32, false>},

    {OPDReg(0xD9, 3) | 0x00, 8, &OpDispatchBuilder::FST<32, true>},
    {OPDReg(0xD9, 3) | 0x40, 8, &OpDispatchBuilder::FST<32, true>},
    {OPDReg(0xD9, 3) | 0x80, 8, &OpDispatchBuilder::FST<32, true>},

    {OPDReg(0xD9, 4) | 0x00, 8, &OpDispatchBuilder::X87LDENV},
    {OPDReg(0xD9, 4) | 0x40, 8, &OpDispatchBuilder::X87LDENV},
    {OPDReg(0xD9, 4) | 0x80, 8, &OpDispatchBuilder::X87LDENV},

    {OPDReg(0xD9, 5) | 0x00, 8, &OpDispatchBuilder::NOPOp}, // XXX: stubbed FLDCW
    {OPDReg(0xD9, 5) | 0x40, 8, &OpDispatchBuilder::NOPOp}, // XXX: stubbed FLDCW
    {OPDReg(0xD9, 5) | 0x80, 8, &OpDispatchBuilder::NOPOp}, // XXX: stubbed FLDCW

    {OPDReg(0xD9, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSTENV},
    {OPDReg(0xD9, 6) | 0x40, 8, &OpDispatchBuilder::X87FNSTENV},
    {OPDReg(0xD9, 6) | 0x80, 8, &OpDispatchBuilder::X87FNSTENV},

    {OPDReg(0xD9, 7) | 0x00, 8, &OpDispatchBuilder::X87FSTCW},
    {OPDReg(0xD9, 7) | 0x40, 8, &OpDispatchBuilder::X87FSTCW},
    {OPDReg(0xD9, 7) | 0x80, 8, &OpDispatchBuilder::X87FSTCW},

      {OPD(0xD9, 0xC0), 8, &OpDispatchBuilder::FLD<80>},
      {OPD(0xD9, 0xC8), 8, &OpDispatchBuilder::FXCH},
      {OPD(0xD9, 0xD0), 1, &OpDispatchBuilder::NOPOp}, // FNOP
      // D1 = Invalid
      // D8 = Invalid
      {OPD(0xD9, 0xE0), 1, &OpDispatchBuilder::FCHS},
      {OPD(0xD9, 0xE1), 1, &OpDispatchBuilder::FABS},
      // E2 = Invalid
      {OPD(0xD9, 0xE4), 1, &OpDispatchBuilder::FTST},
      {OPD(0xD9, 0xE5), 1, &OpDispatchBuilder::X87FXAM},
      // E6 = Invalid
      {OPD(0xD9, 0xE8), 1, &OpDispatchBuilder::FLD_Const<0x8000'0000'0000'0000, 0b0'011'1111'1111'1111>}, // 1.0
      {OPD(0xD9, 0xE9), 1, &OpDispatchBuilder::FLD_Const<0xD49A'784B'CD1B'8AFE, 0x4000>}, // log2l(10)
      {OPD(0xD9, 0xEA), 1, &OpDispatchBuilder::FLD_Const<0xB8AA'3B29'5C17'F0BC, 0x3FFF>}, // log2l(e)
      {OPD(0xD9, 0xEB), 1, &OpDispatchBuilder::FLD_Const<0xC90F'DAA2'2168'C235, 0x4000>}, // pi
      {OPD(0xD9, 0xEC), 1, &OpDispatchBuilder::FLD_Const<0x9A20'9A84'FBCF'F799, 0x3FFD>}, // log10l(2)
      {OPD(0xD9, 0xED), 1, &OpDispatchBuilder::FLD_Const<0xB172'17F7'D1CF'79AC, 0x3FFE>}, // log(2)
      {OPD(0xD9, 0xEE), 1, &OpDispatchBuilder::FLD_Const<0, 0>}, // 0.0

      // EF = Invalid
      {OPD(0xD9, 0xF0), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80F2XM1>},
      {OPD(0xD9, 0xF1), 1, &OpDispatchBuilder::X87FYL2X<false>},
      {OPD(0xD9, 0xF2), 1, &OpDispatchBuilder::X87TAN},
      {OPD(0xD9, 0xF3), 1, &OpDispatchBuilder::X87ATAN},
      {OPD(0xD9, 0xF4), 1, &OpDispatchBuilder::FXTRACT},
      {OPD(0xD9, 0xF5), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM1>},
      {OPD(0xD9, 0xF6), 1, &OpDispatchBuilder::X87ModifySTP<false>},
      {OPD(0xD9, 0xF7), 1, &OpDispatchBuilder::X87ModifySTP<true>},
      {OPD(0xD9, 0xF8), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM>},
      {OPD(0xD9, 0xF9), 1, &OpDispatchBuilder::X87FYL2X<true>},
      {OPD(0xD9, 0xFA), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80SQRT>},
      {OPD(0xD9, 0xFB), 1, &OpDispatchBuilder::X87SinCos},
      {OPD(0xD9, 0xFC), 1, &OpDispatchBuilder::FRNDINT},
      {OPD(0xD9, 0xFD), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80SCALE>},
      {OPD(0xD9, 0xFE), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80SIN>},
      {OPD(0xD9, 0xFF), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80COS>},

    {OPDReg(0xDA, 0) | 0x00, 8, &OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 0) | 0x40, 8, &OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 0) | 0x80, 8, &OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 1) | 0x40, 8, &OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 1) | 0x80, 8, &OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDA, 2) | 0x40, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDA, 2) | 0x80, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDA, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDA, 3) | 0x40, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDA, 3) | 0x80, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},

    {OPDReg(0xDA, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 4) | 0x40, 8, &OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 4) | 0x80, 8, &OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 5) | 0x40, 8, &OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 5) | 0x80, 8, &OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 6) | 0x40, 8, &OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 6) | 0x80, 8, &OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 7) | 0x40, 8, &OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDA, 7) | 0x80, 8, &OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDA, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
      // E0 = Invalid
      // E8 = Invalid
      {OPD(0xDA, 0xE9), 1, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      // EA = Invalid
      // F0 = Invalid
      // F8 = Invalid

    {OPDReg(0xDB, 0) | 0x00, 8, &OpDispatchBuilder::FILD<32>},
    {OPDReg(0xDB, 0) | 0x40, 8, &OpDispatchBuilder::FILD<32>},
    {OPDReg(0xDB, 0) | 0x80, 8, &OpDispatchBuilder::FILD<32>},

    {OPDReg(0xDB, 1) | 0x00, 8, &OpDispatchBuilder::FIST<32, true>},
    {OPDReg(0xDB, 1) | 0x40, 8, &OpDispatchBuilder::FIST<32, true>},
    {OPDReg(0xDB, 1) | 0x80, 8, &OpDispatchBuilder::FIST<32, true>},

    {OPDReg(0xDB, 2) | 0x00, 8, &OpDispatchBuilder::FIST<32, false>},
    {OPDReg(0xDB, 2) | 0x40, 8, &OpDispatchBuilder::FIST<32, false>},
    {OPDReg(0xDB, 2) | 0x80, 8, &OpDispatchBuilder::FIST<32, false>},

    {OPDReg(0xDB, 3) | 0x00, 8, &OpDispatchBuilder::FIST<32, true>},
    {OPDReg(0xDB, 3) | 0x40, 8, &OpDispatchBuilder::FIST<32, true>},
    {OPDReg(0xDB, 3) | 0x80, 8, &OpDispatchBuilder::FIST<32, true>},

    // 4 = Invalid

    {OPDReg(0xDB, 5) | 0x00, 8, &OpDispatchBuilder::FLD<80>},
    {OPDReg(0xDB, 5) | 0x40, 8, &OpDispatchBuilder::FLD<80>},
    {OPDReg(0xDB, 5) | 0x80, 8, &OpDispatchBuilder::FLD<80>},

    // 6 = Invalid

    {OPDReg(0xDB, 7) | 0x00, 8, &OpDispatchBuilder::FST<80, true>},
    {OPDReg(0xDB, 7) | 0x40, 8, &OpDispatchBuilder::FST<80, true>},
    {OPDReg(0xDB, 7) | 0x80, 8, &OpDispatchBuilder::FST<80, true>},


      {OPD(0xDB, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
      // E0 = Invalid
      {OPD(0xDB, 0xE2), 1, &OpDispatchBuilder::NOPOp}, // FNCLEX
      {OPD(0xDB, 0xE3), 1, &OpDispatchBuilder::FNINIT},
      // E4 = Invalid
      {OPD(0xDB, 0xE8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
      {OPD(0xDB, 0xF0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},

      // F8 = Invalid

    {OPDReg(0xDC, 0) | 0x00, 8, &OpDispatchBuilder::FADD<64, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 0) | 0x40, 8, &OpDispatchBuilder::FADD<64, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 0) | 0x80, 8, &OpDispatchBuilder::FADD<64, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 1) | 0x40, 8, &OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 1) | 0x80, 8, &OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDC, 2) | 0x40, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDC, 2) | 0x80, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDC, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDC, 3) | 0x40, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDC, 3) | 0x80, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},

    {OPDReg(0xDC, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 4) | 0x40, 8, &OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 4) | 0x80, 8, &OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 5) | 0x40, 8, &OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 5) | 0x80, 8, &OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 6) | 0x40, 8, &OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 6) | 0x80, 8, &OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 7) | 0x40, 8, &OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDC, 7) | 0x80, 8, &OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDC, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDD, 0) | 0x00, 8, &OpDispatchBuilder::FLD<64>},
    {OPDReg(0xDD, 0) | 0x40, 8, &OpDispatchBuilder::FLD<64>},
    {OPDReg(0xDD, 0) | 0x80, 8, &OpDispatchBuilder::FLD<64>},

    {OPDReg(0xDD, 1) | 0x00, 8, &OpDispatchBuilder::FIST<64, true>},
    {OPDReg(0xDD, 1) | 0x40, 8, &OpDispatchBuilder::FIST<64, true>},
    {OPDReg(0xDD, 1) | 0x80, 8, &OpDispatchBuilder::FIST<64, true>},

    {OPDReg(0xDD, 2) | 0x00, 8, &OpDispatchBuilder::FST<64, false>},
    {OPDReg(0xDD, 2) | 0x40, 8, &OpDispatchBuilder::FST<64, false>},
    {OPDReg(0xDD, 2) | 0x80, 8, &OpDispatchBuilder::FST<64, false>},

    {OPDReg(0xDD, 3) | 0x00, 8, &OpDispatchBuilder::FST<64, true>},
    {OPDReg(0xDD, 3) | 0x40, 8, &OpDispatchBuilder::FST<64, true>},
    {OPDReg(0xDD, 3) | 0x80, 8, &OpDispatchBuilder::FST<64, true>},

    {OPDReg(0xDD, 4) | 0x00, 8, &OpDispatchBuilder::X87FRSTOR},
    {OPDReg(0xDD, 4) | 0x40, 8, &OpDispatchBuilder::X87FRSTOR},
    {OPDReg(0xDD, 4) | 0x80, 8, &OpDispatchBuilder::X87FRSTOR},

    // 5 = Invalid
    {OPDReg(0xDD, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSAVE},
    {OPDReg(0xDD, 6) | 0x40, 8, &OpDispatchBuilder::X87FNSAVE},
    {OPDReg(0xDD, 6) | 0x80, 8, &OpDispatchBuilder::X87FNSAVE},

    {OPDReg(0xDD, 7) | 0x00, 8, &OpDispatchBuilder::X87FNSTSW},
    {OPDReg(0xDD, 7) | 0x40, 8, &OpDispatchBuilder::X87FNSTSW},
    {OPDReg(0xDD, 7) | 0x80, 8, &OpDispatchBuilder::X87FNSTSW},

      {OPD(0xDD, 0xC0), 8, &OpDispatchBuilder::NOPOp}, // stubbed FFREE
      {OPD(0xDD, 0xD0), 8, &OpDispatchBuilder::FST<false>},
      {OPD(0xDD, 0xD8), 8, &OpDispatchBuilder::FST<true>},

      {OPD(0xDD, 0xE0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xDD, 0xE8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},

    {OPDReg(0xDE, 0) | 0x00, 8, &OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 0) | 0x40, 8, &OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 0) | 0x80, 8, &OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 1) | 0x40, 8, &OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 1) | 0x80, 8, &OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDE, 2) | 0x40, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
    {OPDReg(0xDE, 2) | 0x80, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDE, 3) | 0x40, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
    {OPDReg(0xDE, 3) | 0x80, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},

    {OPDReg(0xDE, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 4) | 0x40, 8, &OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 4) | 0x80, 8, &OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 5) | 0x40, 8, &OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 5) | 0x80, 8, &OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 6) | 0x40, 8, &OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 6) | 0x80, 8, &OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 7) | 0x40, 8, &OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},
    {OPDReg(0xDE, 7) | 0x80, 8, &OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDE, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xD9), 1, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      {OPD(0xDE, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDF, 0) | 0x00, 8, &OpDispatchBuilder::FILD<16>},
    {OPDReg(0xDF, 0) | 0x40, 8, &OpDispatchBuilder::FILD<16>},
    {OPDReg(0xDF, 0) | 0x80, 8, &OpDispatchBuilder::FILD<16>},

    {OPDReg(0xDF, 1) | 0x00, 8, &OpDispatchBuilder::FIST<16, true>},
    {OPDReg(0xDF, 1) | 0x40, 8, &OpDispatchBuilder::FIST<16, true>},
    {OPDReg(0xDF, 1) | 0x80, 8, &OpDispatchBuilder::FIST<16, true>},

    {OPDReg(0xDF, 2) | 0x00, 8, &OpDispatchBuilder::FIST<16, false>},
    {OPDReg(0xDF, 2) | 0x40, 8, &OpDispatchBuilder::FIST<16, false>},
    {OPDReg(0xDF, 2) | 0x80, 8, &OpDispatchBuilder::FIST<16, false>},

    {OPDReg(0xDF, 3) | 0x00, 8, &OpDispatchBuilder::FIST<16, true>},
    {OPDReg(0xDF, 3) | 0x40, 8, &OpDispatchBuilder::FIST<16, true>},
    {OPDReg(0xDF, 3) | 0x80, 8, &OpDispatchBuilder::FIST<16, true>},

    // 4 = FBLD

    {OPDReg(0xDF, 5) | 0x00, 8, &OpDispatchBuilder::FILD<64>},
    {OPDReg(0xDF, 5) | 0x40, 8, &OpDispatchBuilder::FILD<64>},
    {OPDReg(0xDF, 5) | 0x80, 8, &OpDispatchBuilder::FILD<64>},

    // 6 = FTSTB

    {OPDReg(0xDF, 7) | 0x00, 8, &OpDispatchBuilder::FIST<64, true>},
    {OPDReg(0xDF, 7) | 0x40, 8, &OpDispatchBuilder::FIST<64, true>},
    {OPDReg(0xDF, 7) | 0x80, 8, &OpDispatchBuilder::FIST<64, true>},

      // XXX: This should also set the x87 tag bits to empty
      // We don't support this currently, so just pop the stack
      {OPD(0xDF, 0xC0), 8, &OpDispatchBuilder::X87ModifySTP<true>},

      {OPD(0xDF, 0xE0), 8, &OpDispatchBuilder::X87FNSTSW},
      {OPD(0xDF, 0xE8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, true>},
      {OPD(0xDF, 0xF0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, true>},
  };
#undef OPD
#undef OPDReg

#define OPD(prefix, opcode) ((prefix << 8) | opcode)
  constexpr uint16_t PF_38_NONE = 0;
  constexpr uint16_t PF_38_66   = 1;
  constexpr uint16_t PF_38_F2   = 2;
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> H0F38Table = {
    {OPD(PF_38_66,   0x00), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(PF_38_66,   0x3B), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(PF_38_NONE, 0xF0), 1, &OpDispatchBuilder::MOVBEOp},
    {OPD(PF_38_NONE, 0xF1), 1, &OpDispatchBuilder::MOVBEOp},
    {OPD(PF_38_66, 0xF0), 1, &OpDispatchBuilder::MOVBEOp},
    {OPD(PF_38_66, 0xF1), 1, &OpDispatchBuilder::MOVBEOp},

  };
#undef OPD

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66   1
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> H0F3ATable = {
    {OPD(0, PF_3A_66,   0x0F), 1, &OpDispatchBuilder::PAlignrOp},
    {OPD(1, PF_3A_66,   0x0F), 1, &OpDispatchBuilder::PAlignrOp},
  };
#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> VEXTable = {
    {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x6F), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::UnimplementedOp},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> EVEXTable = {
    {0x10, 1, &OpDispatchBuilder::UnimplementedOp},
    {0x11, 1, &OpDispatchBuilder::UnimplementedOp},
    {0x59, 1, &OpDispatchBuilder::UnimplementedOp},
    {0x7F, 1, &OpDispatchBuilder::UnimplementedOp},
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
  if (Mode == Context::MODE_32BIT) {
    InstallToTable(FEXCore::X86Tables::BaseOps, BaseOpTable_32);
    InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable_32);
  }
  else {
    InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable_64);
  }

  InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable);
  InstallToTable(FEXCore::X86Tables::PrimaryInstGroupOps, PrimaryGroupOpTable);

  InstallToTable(FEXCore::X86Tables::RepModOps, RepModOpTable);
  InstallToTable(FEXCore::X86Tables::RepNEModOps, RepNEModOpTable);
  InstallToTable(FEXCore::X86Tables::OpSizeModOps, OpSizeModOpTable);
  InstallToTable(FEXCore::X86Tables::SecondInstGroupOps, SecondaryExtensionOpTable);

  InstallToTable(FEXCore::X86Tables::SecondModRMTableOps, SecondaryModRMExtensionOpTable);

  InstallToTable(FEXCore::X86Tables::X87Ops, X87OpTable);

  InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38Table);
  InstallToTable(FEXCore::X86Tables::H0F3ATableOps, H0F3ATable);
  InstallToTable(FEXCore::X86Tables::VEXTableOps, VEXTable);
  InstallToTable(FEXCore::X86Tables::EVEXTableOps, EVEXTable);

  // Useful for debugging
  // CheckTable(FEXCore::X86Tables::BaseOps);
  LogMan::Msg::D("We installed %ld instructions to the tables", NumInsts);
}

}
