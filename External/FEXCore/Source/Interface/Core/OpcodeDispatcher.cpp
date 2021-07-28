/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 ops to IR, no-pf opt, local-flags opt
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CoreState.h>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>

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
  using SyscallArray = std::array<uint64_t, SyscallArgs>;

  const SyscallArray *GPRIndexes {};
  static constexpr SyscallArray GPRIndexes_64 = {
    FEXCore::X86State::REG_RAX,
    FEXCore::X86State::REG_RDI,
    FEXCore::X86State::REG_RSI,
    FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_R10,
    FEXCore::X86State::REG_R8,
    FEXCore::X86State::REG_R9,
  };
  static constexpr SyscallArray GPRIndexes_32 = {
    FEXCore::X86State::REG_RAX,
    FEXCore::X86State::REG_RBX,
    FEXCore::X86State::REG_RCX,
    FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_RSI,
    FEXCore::X86State::REG_RDI,
    FEXCore::X86State::REG_RBP,
  };
  static_assert(GPRIndexes_64.size() == GPRIndexes_32.size());

  const auto OSABI = CTX->SyscallHandler->GetOSABI();
  if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX64) {
    GPRIndexes = &GPRIndexes_64;
  }
  else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX32) {
    GPRIndexes = &GPRIndexes_32;
  }
  else {
    LogMan::Msg::D("Unhandled OSABI syscall");
  }

  const uint8_t GPRSize = CTX->GetGPRSize();
  auto NewRIP = GetDynamicPC(Op, -Op->InstSize);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);

  const auto& GPRIndicesRef = *GPRIndexes;
  auto SyscallOp = _Syscall(
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[0] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[1] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[2] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[3] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[4] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[5] * 8, GPRClass),
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs) + GPRIndicesRef[6] * 8, GPRClass));

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), SyscallOp);
}

void OpDispatchBuilder::ThunkOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  uint8_t *sha256 = (uint8_t *)(Op->PC + 2);

  _Thunk(
    _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass),
    *reinterpret_cast<SHA256Sum*>(sha256)
  );

  auto Constant = ConstructConst(GPRSize);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);
  auto NewRIP = _LoadMem(GPRClass, GPRSize, OldSP, GPRSize);
  OrderedNode *NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store the new RIP
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::LEAOp(OpcodeArgs) {
  // LEA specifically ignores segment prefixes
  if (CTX->Config.Is64BitMode) {
    uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 :
      X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST ? 8 : 4;

    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GetSrcSize(Op), Op->Flags, -1, false);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
  }
  else {
    uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 : 4;

    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GetSrcSize(Op), Op->Flags, -1, false);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
  }
}

void OpDispatchBuilder::NOPOp(OpcodeArgs) {
}

void OpDispatchBuilder::RETOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // ABI Optimization: Flags don't survive calls or rets
  if (CTX->Config.ABILocalFlags) {
    _InvalidateFlags(~0UL); // all flags
  }

  auto Constant = ConstructConst(GPRSize);

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
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

/*
stack contains:
Size of each member is 64-bit, 32-bit, or 16-bit depending on operating size
RIP
CS
EFLAGS
RSP
SS
*/
void OpDispatchBuilder::IRETOp(OpcodeArgs) {
  // Operand Size override unsupported!
  if ((Op->Flags & X86Tables::DecodeFlags::FLAG_OPERAND_SIZE) != 0) {
    LogMan::Msg::E("IRET only implemented for 64bit and 32bit sizes");
    DecodeFailure = true;
    return;
  }

  const uint8_t GPRSize = CTX->GetGPRSize();

  auto Constant = ConstructConst(GPRSize);

  OrderedNode* SP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  // RIP (64/32/16 bits)
  auto NewRIP = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
  SP = _Add(SP, Constant);
  //CS (lower 16 used)
  _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, cs), _LoadMem(GPRClass, GPRSize, SP, GPRSize));
  SP = _Add(SP, Constant);
  //eflags (lower 16 used)
  auto eflags = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
  SetPackedRFLAG(false, eflags);

  if (CTX->Config.Is64BitMode) {
    // RSP and SS only happen in 64-bit mode or if this is a CPL mode jump!
    SP = _Add(SP, Constant);
    // RSP
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), _LoadMem(GPRClass, GPRSize, SP, GPRSize));
    SP = _Add(SP, Constant);
    //ss
    _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, ss), _LoadMem(GPRClass, GPRSize, SP, GPRSize));
    SP = _Add(SP, Constant);
  }

  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::SIGRETOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  // Store the new RIP
  _SignalReturn();
  auto NewRIP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, rip), GPRClass);
  // This ExitFunction won't actually get hit but needs to exist
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::CallbackReturnOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  // Store the new RIP
  _CallbackReturn();
  auto NewRIP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, rip), GPRClass);
  // This ExitFunction won't actually get hit but needs to exist
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::SecondaryALUOp(OpcodeArgs) {
  bool RequiresMask = false;
  FEXCore::IR::IROps IROp;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0):
    IROp = FEXCore::IR::IROps::OP_ADD;
    RequiresMask = true;
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
    RequiresMask = true;
  break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6):
    IROp = FEXCore::IR::IROps::OP_XOR;
  break;
  default:
    IROp = FEXCore::IR::IROps::OP_LAST;
    LOGMAN_MSG_A("Unknown ALU Op: 0x%x", Op->OP);
  break;
  };
#undef OPD
  // X86 basic ALU ops just do the operation between the destination and a single source
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  uint8_t Size = GetDstSize(Op);
  OrderedNode *Result{};
  OrderedNode *Dest{};

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    switch (IROp) {
      case FEXCore::IR::IROps::OP_ADD: {
        Dest = _AtomicFetchAdd(DestMem, Src, Size);
        Result = _Add(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_SUB: {
        Dest = _AtomicFetchSub(DestMem, Src, Size);
        Result = _Sub(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_OR: {
        Dest = _AtomicFetchOr(DestMem, Src, Size);
        Result = _Or(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_AND: {
        Dest = _AtomicFetchAnd(DestMem, Src, Size);
        Result = _And(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_XOR: {
        Dest = _AtomicFetchXor(DestMem, Src, Size);
        Result = _Xor(Dest, Src);
        break;
      }
      default: LOGMAN_MSG_A("Unknown Atomic IR Op: %d", IROp); break;
    }
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    auto ALUOp = _Add(Dest, Src);
    // Overwrite our IR's op type
    ALUOp.first->Header.Op = IROp;

    Result = ALUOp;

    StoreResult(GPRClass, Op, Result, -1);
  }

  // Store result masks, but we need to
  if (RequiresMask && Size < 4) {
    Result = _Bfe(Size, Size * 8, 0, Result);
  }

  // Flags set
  {
    switch (IROp) {
    case FEXCore::IR::IROps::OP_ADD:
      GenerateFlags_ADD(Op, Result, Dest, Src);
    break;
    case FEXCore::IR::IROps::OP_SUB:
      GenerateFlags_SUB(Op, Result, Dest, Src);
    break;
    case FEXCore::IR::IROps::OP_AND:
    case FEXCore::IR::IROps::OP_XOR:
    case FEXCore::IR::IROps::OP_OR: {
      GenerateFlags_Logical(Op, Result, Dest, Src);
    break;
    }
    default: break;
    }
  }
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::ADCOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  uint8_t Size = GetDstSize(Op);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
  auto ALUOp = _Add(Src, CF);

  OrderedNode *Result{};
  OrderedNode *Before{};
  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    Before = _AtomicFetchAdd(DestMem, ALUOp, Size);
    Result = _Add(Before, ALUOp);
  }
  else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Result = _Add(Before, ALUOp);
    StoreResult(GPRClass, Op, Result, -1);
  }

  if (Size < 4)
    Result = _Bfe(Size, Size * 8, 0, Result);
  GenerateFlags_ADC(Op, Result, Before, Src, CF);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::SBBOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  auto Size = GetDstSize(Op);

  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
  auto ALUOp = _Add(Src, CF);

  OrderedNode *Result{};
  OrderedNode *Before{};
  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    Before = _AtomicFetchSub(DestMem, ALUOp, Size);
    Result = _Sub(Before, ALUOp);
  }
  else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Result = _Sub(Before, ALUOp);
    StoreResult(GPRClass, Op, Result, -1);
  }

  if (Size < 4) {
    Result = _Bfe(Size, Size * 8, 0, Result);
  }
  GenerateFlags_SBB(Op, Result, Before, Src, CF);
}

void OpDispatchBuilder::PUSHOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::PUSHREGOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::PUSHAOp(OpcodeArgs) {
  // 32bit only
  uint8_t Size = GetSrcSize(Op);
  uint8_t GPRSize = 4;

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  // PUSHA order:
  // Tmp = SP
  // push EAX
  // push ECX
  // push EDX
  // push EBX
  // push Tmp
  // push EBP
  // push ESI
  // push EDI

  OrderedNode *Src{};
  OrderedNode *NewSP = OldSP;
  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  _StoreMem(GPRClass, Size, NewSP, OldSP, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  NewSP = _Sub(NewSP, Constant);
  Src = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
  _StoreMem(GPRClass, Size, NewSP, Src, Size);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);
}

template<uint32_t SegmentReg>
void OpDispatchBuilder::PUSHSegmentOp(OpcodeArgs) {
  const uint8_t SrcSize = GetSrcSize(Op);
  const uint8_t DstSize = GetDstSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();
  auto Constant = ConstructConst(DstSize);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  OrderedNode *Src{};
  switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, es), GPRClass);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, cs), GPRClass);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, ss), GPRClass);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, ds), GPRClass);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      Src = _LoadContext(SrcSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
      break;
    default: break; // Do nothing
  }

  // Store our value to the new stack location
  // AMD hardware zexts segment selector to 32bit
  // Intel hardware inserts segment selector
  _StoreMem(GPRClass, DstSize, NewSP, Src, DstSize);
}

void OpDispatchBuilder::POPOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewGPR = _LoadMem(GPRClass, Size, OldSP, Size);

  auto NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded from the stack
  StoreResult(GPRClass, Op, NewGPR, -1);
}

void OpDispatchBuilder::POPAOp(OpcodeArgs) {
  // 32bit only
  uint8_t Size = GetSrcSize(Op);
  uint8_t GPRSize = 4;

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  // POPA order:
  // pop EDI
  // pop ESI
  // pop EBP
  // ESP += 4; // Skip RSP because it'll be correct at the end
  // pop EBX
  // pop EDX
  // pop ECX
  // pop EAX

  OrderedNode *Src{};
  OrderedNode *NewSP = OldSP;
  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Src);
  NewSP = _Add(NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Src);
  NewSP = _Add(NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), Src);
  NewSP = _Add(NewSP, ConstructConst(Size * 2));

  // Skip SP loading
  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), Src);
  NewSP = _Add(NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), Src);
  NewSP = _Add(NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), Src);
  NewSP = _Add(NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Src);
  NewSP = _Add(NewSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);
}

template<uint32_t SegmentReg>
void OpDispatchBuilder::POPSegmentOp(OpcodeArgs) {
  const uint8_t SrcSize = GetSrcSize(Op);
  const uint8_t DstSize = GetDstSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();

  auto Constant = ConstructConst(SrcSize);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSegment = _LoadMem(GPRClass, SrcSize, OldSP, SrcSize);

  auto NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, es), NewSegment);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, cs), NewSegment);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, ss), NewSegment);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, ds), NewSegment);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, fs), NewSegment);
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      _StoreContext(GPRClass, DstSize, offsetof(FEXCore::Core::CPUState, gs), NewSegment);
      break;
    default: break; // Do nothing
  }
}

void OpDispatchBuilder::LEAVEOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // First we move RBP in to RSP and then behave effectively like a pop
  const uint8_t Size = GetSrcSize(Op);
  auto Constant = ConstructConst(Size);

  auto OldBP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), GPRClass);

  auto NewGPR = _LoadMem(GPRClass, Size, OldBP, Size);

  auto NewSP = _Add(OldBP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store what we loaded to RBP
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), NewGPR);
}

void OpDispatchBuilder::CALLOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  BlockSetRIP = true;

  // ABI Optimization: Flags don't survive calls or rets
  if (CTX->Config.ABILocalFlags) {
    _InvalidateFlags(~0UL); // all flags
  }

  auto ConstantPC = GetDynamicPC(Op);

  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *NewRIP = _Add(ConstantPC, JMPPCOffset);
  auto ConstantPCReturn = GetDynamicPC(Op);

  auto ConstantSize = ConstructConst(GPRSize);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(GPRClass, GPRSize, NewSP, ConstantPCReturn, GPRSize);

  // Store the RIP
  _ExitFunction(NewRIP); // If we get here then leave the function now
}

void OpDispatchBuilder::CALLAbsoluteOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  BlockSetRIP = true;

  const uint8_t Size = GetSrcSize(Op);
  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ConstantPCReturn = GetDynamicPC(Op);

  auto ConstantSize = ConstructConst(Size);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  auto NewSP = _Sub(OldSP, ConstantSize);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreMem(GPRClass, Size, NewSP, ConstantPCReturn, Size);

  // Store the RIP
  _ExitFunction(JMPPCOffset); // If we get here then leave the function now
}

OrderedNode *OpDispatchBuilder::SelectCC(uint8_t OP, OrderedNode *TrueValue, OrderedNode *FalseValue) {
  OrderedNode *SrcCond = nullptr;

  auto ZeroConst = ConstructConst(0);
  auto OneConst = ConstructConst(1);

  switch (OP) {
    case 0x0: { // JO - Jump if OF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x1:{ // JNO - Jump if OF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x2: { // JC - Jump if CF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x3: { // JNC - Jump if CF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x4: { // JE - Jump if ZF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x5: { // JNE - Jump if ZF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x6: { // JNA - Jump if CF == 1 || ZC == 1
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      auto Check = _Or(Flag1, Flag2);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, OneConst, TrueValue, FalseValue);
      break;
    }
    case 0x7: { // JA - Jump if CF == 0 && ZF == 0
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
      auto Check = _Or(Flag1, _Lshl(Flag2, ConstructConst(1)));
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x8: { // JS - Jump if SF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0x9: { // JNS - Jump if SF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0xA: { // JP - Jump if PF == 1
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_PF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0xB: { // JNP - Jump if PF == 0
      auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_PF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag, ZeroConst, TrueValue, FalseValue);
      break;
    }
    case 0xC: { // SF <> OF
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_NEQ,
          Flag1, Flag2, TrueValue, FalseValue);
      break;
    }
    case 0xD: { // SF = OF
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Flag1, Flag2, TrueValue, FalseValue);
      break;
    }
    case 0xE: {// ZF = 1 || SF <> OF
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

      auto Select1 = _Select(FEXCore::IR::COND_EQ,
          Flag1, OneConst, OneConst, ZeroConst);

      auto Select2 = _Select(FEXCore::IR::COND_NEQ,
          Flag2, Flag3, OneConst, ZeroConst);

      auto Check = _Or(Select1, Select2);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, OneConst, TrueValue, FalseValue);
      break;
    }
    case 0xF: {// ZF = 0 && SF = OF
      auto Flag1 = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      auto Flag2 = GetRFLAG(FEXCore::X86State::RFLAG_SF_LOC);
      auto Flag3 = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

      auto Select1 = _Select(FEXCore::IR::COND_EQ,
          Flag1, ZeroConst, OneConst, ZeroConst);

      auto Select2 = _Select(FEXCore::IR::COND_EQ,
          Flag2, Flag3, OneConst, ZeroConst);

      auto Check = _And(Select1, Select2);
      SrcCond = _Select(FEXCore::IR::COND_EQ,
          Check, OneConst, TrueValue, FalseValue);
      break;
    }
    default: LOGMAN_MSG_A("Unknown CC Op: 0x%x\n", OP); return nullptr;
  }

  // Try folding the flags generation in the select op
  if (flagsOp == FLAGS_OP_CMP) {
    switch(OP) {
      // SGT
      case 0xF: SrcCond = _Select(FEXCore::IR::COND_SGT, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;
      // SLE
      case 0xE: SrcCond = _Select(FEXCore::IR::COND_SLE, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;
      // SGE
      case 0xD: SrcCond = _Select(FEXCore::IR::COND_SGE, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;
      // SL
      case 0xC: SrcCond = _Select(FEXCore::IR::COND_SLT, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;

      // not sign
      //case 0x99: SrcCond = _Select(FEXCore::IR::COND_, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;
      // sign
      //case 0x98: SrcCond = _Select(FEXCore::IR::COND_, flagsOpDestSigned, flagsOpSrcSigned, TrueValue, FalseValue, flagsOpSize); break;

      // UABove
      case 0x7: SrcCond = _Select(FEXCore::IR::COND_UGT, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;
      // UBE
      case 0x6: SrcCond = _Select(FEXCore::IR::COND_ULE, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;
      // NE
      case 0x5: SrcCond = _Select(FEXCore::IR::COND_NEQ, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;
      // EQ/Zero
      case 0x4: SrcCond = _Select(FEXCore::IR::COND_EQ, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;
      // UAE
      case 0x3: SrcCond = _Select(FEXCore::IR::COND_UGE, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;
      // UBelow
      case 0x2: SrcCond = _Select(FEXCore::IR::COND_ULT, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize); break;

      //default: printf("Missed Condition %04X OP_CMP\n", OP); break;
    }
  }
  else if (flagsOp == FLAGS_OP_AND) {
    switch(OP) {
      case 0x4: SrcCond = _Select(FEXCore::IR::COND_EQ, flagsOpDest, ZeroConst, TrueValue, FalseValue, flagsOpSize); break;
      case 0x5: SrcCond = _Select(FEXCore::IR::COND_NEQ, flagsOpDest, ZeroConst, TrueValue, FalseValue, flagsOpSize); break;
      //default: printf("Missed Condition %04X OP_AND\n", OP); break;
    }
  } else if (flagsOp == FLAGS_OP_FCMP) {
    /*
      x86:ZCP
        unordered { 11 1 }
        greater   { 00 0 }
        less      { 01 0 }
        equal     { 10 0 }
      aarch64: NZCV
        unordered { 0 01 1 }
        greater   { 0 01 0 }
        less      { 1 00 0 }
        equal     { 0 11 0 }
    */

   /*
      eq = 0,   // Z set            Equal.
      ne = 1,   // Z clear          Not equal.
      cs = 2,   // C set            Carry set.
      cc = 3,   // C clear          Carry clear.
      mi = 4,   // N set            Negative.
      pl = 5,   // N clear          Positive or zero.
      vs = 6,   // V set            Overflow.
      vc = 7,   // V clear          No overflow.
      hi = 8,   // C set, Z clear   Unsigned higher.
      ls = 9,   // C clear or Z set Unsigned lower or same.
      ge = 10,  // N == V           Greater or equal.
      lt = 11,  // N != V           Less than.
      gt = 12,  // Z clear, N == V  Greater than.
      le = 13,  // Z set or N != V  Less then or equal
   */
    switch(OP) {
      case 0x2: // CF == 1 // less or unordered                      // N==1 OR V==1        // lt
        SrcCond = _Select(FEXCore::IR::COND_FLU, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      case 0x3: // CF == 0 // greater or equal (and not unordered)   // N==V                // ge
        SrcCond = _Select(FEXCore::IR::COND_FGE, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      case 0x6: // CF == 1 || ZF == 1 // less or equal or unordered  // Z==1 OR N!=V        // le
        SrcCond = _Select(FEXCore::IR::COND_FLEU, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      case 0x7: // CF == 0 && ZF == 0 // greater (and not unordered) // C==1 AND V=0        // hi
        SrcCond = _Select(FEXCore::IR::COND_FGT, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      case 0xA: // PF = 1 // unordered                               // V==1                // vs
        SrcCond = _Select(FEXCore::IR::COND_FU, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      case 0xB: // PF = 0 // not unordered                           // V==0                // vc
        SrcCond = _Select(FEXCore::IR::COND_FNU, flagsOpDest, flagsOpSrc, TrueValue, FalseValue, flagsOpSize);
        break;
      default:
        // TODO: Add more optimized cases
        break;
    }
  }

  return SrcCond;
}

void OpDispatchBuilder::SETccOp(OpcodeArgs) {
  auto ZeroConst = ConstructConst(0);
  auto OneConst = ConstructConst(1);

  auto SrcCond = SelectCC(Op->OP & 0xF, OneConst, ZeroConst);

  StoreResult(GPRClass, Op, SrcCond, -1);
}

void OpDispatchBuilder::CMOVOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto SrcCond = SelectCC(Op->OP & 0xF, Src, Dest);

  StoreResult(GPRClass, Op, SrcCond, -1);
}

void OpDispatchBuilder::CondJUMPOp(OpcodeArgs) {
  BlockSetRIP = true;

  auto TakeBranch = ConstructConst(1);
  auto DoNotTakeBranch = ConstructConst(0);

  auto SrcCond = SelectCC(Op->OP & 0xF, TakeBranch, DoNotTakeBranch);

  LOGMAN_THROW_A(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].Data.Literal.Value;

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  // Fallback
  {
    auto CondJump = _CondJump(SrcCond);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto NewRIP = GetDynamicPC(Op, Op->Src[0].Data.Literal.Value);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place it after this block for fallthrough optimization
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = GetDynamicPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::CondJUMPRCXOp(OpcodeArgs) {
  BlockSetRIP = true;
  uint8_t JcxGPRSize = CTX->GetGPRSize();
  JcxGPRSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? (JcxGPRSize >> 1) : JcxGPRSize;

  LOGMAN_THROW_A(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].Data.Literal.Value;

  OrderedNode *CondReg = _LoadContext(JcxGPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  {
    auto CondJump = _CondJump(CondReg, {COND_EQ});

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto NewRIP = GetDynamicPC(Op, Op->Src[0].Data.Literal.Value);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place it after the current block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = GetDynamicPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::LoopOp(OpcodeArgs) {
  bool CheckZF = Op->OP != 0xE2;
  bool ZFTrue = Op->OP == 0xE1;

  BlockSetRIP = true;
  auto ZeroConst = ConstructConst(0);
  IRPair<IROp_Header> SrcCond;

  auto TakeBranch = ConstructConst(1);
  auto DoNotTakeBranch = ConstructConst(0);

  uint32_t SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 4 : 8;

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[1].Data.Literal.Value;

  OrderedNode *CondReg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  CondReg = _Sub(CondReg, ConstructConst(SrcSize * 8, 1));
  StoreResult(GPRClass, Op, Op->Src[0], CondReg, -1);

  SrcCond = _Select(FEXCore::IR::COND_NEQ,
          CondReg, ZeroConst, TakeBranch, DoNotTakeBranch);

  // If LOOPE then jumps to target if RCX != 0 && ZF == 1
  // If LOOPNE then jumps to target if RCX != 0 && ZF == 0
  if (CheckZF) {
    OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
    if (!ZFTrue) {
      ZF = _Xor(ZF, ConstructConst(1));
    }
    SrcCond = _And(SrcCond, ZF);
  }

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  {
    auto CondJump = _CondJump(SrcCond);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      auto NewRIP = GetDynamicPC(Op, Op->Src[1].Data.Literal.Value);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetFalseJumpTarget(CondJump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);

      // Leave block
      auto RIPTargetConst = GetDynamicPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::JUMPOp(OpcodeArgs) {
  BlockSetRIP = true;

  // This is just an unconditional relative literal jump
  if (Multiblock) {
    LOGMAN_THROW_A(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");
    uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].Data.Literal.Value;
    auto JumpBlock = JumpTargets.find(Target);
    if (JumpBlock != JumpTargets.end()) {
      _Jump(GetNewJumpBlock(Target));
    }
    else {
      // If the block isn't a jump target then we need to create an exit block
      auto Jump = _Jump();

      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(Jump, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      _ExitFunction(GetDynamicPC(Op, Op->Src[0].Data.Literal.Value));
    }
    return;
  }

  // Fallback
  {
    // This source is a literal
    auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

    auto RIPTargetConst = GetDynamicPC(Op);
    auto NewRIP = _Add(RIPOffset, RIPTargetConst);

    // Store the new RIP
    _ExitFunction(NewRIP);
  }
}

void OpDispatchBuilder::JUMPAbsoluteOp(OpcodeArgs) {
  BlockSetRIP = true;
  // This is just an unconditional jump
  // This uses ModRM to determine its location
  // No way to use this effectively in multiblock
  auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  // Store the new RIP
  _ExitFunction(RIPOffset);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::TESTOp(OpcodeArgs) {
  // TEST is an instruction that does an AND between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _And(Dest, Src);
  GenerateFlags_Logical(Op, ALUOp, Dest, Src);

  auto Size = GetDstSize(Op);

  if (Size >=4) {
    flagsOp = FLAGS_OP_AND;
    flagsOpDest = ALUOp;
    flagsOpSize = Size;
  } else {
    flagsOp = FLAGS_OP_AND;
    flagsOpDest = ALUOp;
    flagsOpSize = 4;  // assuming ZEXT semantics here
  }
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
    // Without REX.W then Zext (store result implicitly zero extends)
    StoreResult(GPRClass, Op, Src, -1);
  }
}

void OpDispatchBuilder::MOVSXOp(OpcodeArgs) {
  // This will ZExt the loaded size
  // We want to Sext it
  uint8_t Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  Src = _Sbfe(Size * 8, 0, Src);
  StoreResult(GPRClass, Op, Op->Dest, Src, -1);
}

void OpDispatchBuilder::MOVZXOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Store result implicitly zero extends
  StoreResult(GPRClass, Op, Src, -1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::CMPOp(OpcodeArgs) {
  // CMP is an instruction that does a SUB between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  auto ALUOp = _Sub(Dest, Src);

  OrderedNode *Result = ALUOp;
  if (Size < 4) {
    Result = _Bfe(Size, Size * 8, 0, ALUOp);
  }

  GenerateFlags_SUB(Op, Result, Dest, Src);

  if (Size >= 4) {
    flagsOpSize = Size;
    flagsOp = FLAGS_OP_CMP;
    flagsOpDestSigned = flagsOpDest = Dest;
    flagsOpSrcSigned = flagsOpSrc = Src;
  } else {
    flagsOpSize = 4;
    flagsOp = FLAGS_OP_CMP;
    flagsOpDestSigned = _Sext(Size * 8, flagsOpDest = Dest);
    flagsOpSrcSigned = _Sext(Size * 8, flagsOpSrc = Src);
  }
}

void OpDispatchBuilder::CQOOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Size = GetSrcSize(Op);
  OrderedNode *Upper = _Sbfe(1, Size * 8 - 1, Src);

  StoreResult(GPRClass, Op, Upper, -1);
}

void OpDispatchBuilder::XCHGOp(OpcodeArgs) {
  // Load both the source and the destination
  if (Op->OP == 0x90 &&
      GetSrcSize(Op) >= 4 &&
      Op->Src[0].IsGPR() && Op->Src[0].Data.GPR.GPR == FEXCore::X86State::REG_RAX &&
      Op->Dest.IsGPR() && Op->Dest.Data.GPR.GPR == FEXCore::X86State::REG_RAX) {
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
  if (DestIsMem(Op)) {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

    Dest = AppendSegmentOffset(Dest, Op->Flags);

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

  Src = _Sbfe(SrcSize * 8, 0, Src);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
}

void OpDispatchBuilder::SAHFOp(OpcodeArgs) {
  OrderedNode *Src = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]) + 1, GPRClass);

  // Clear bits that aren't supposed to be set
  Src = _And(Src, ConstructConst(~0b101000));

  // Set the bit that is always set here
  Src = _Or(Src, ConstructConst(0b10));

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
    Result = ConstructConst(0);
  break;
  }
  case OP_SET: {
    Result = ConstructConst(1);
  break;
  }
  case OP_COMPLEMENT: {
    auto RFLAG = GetRFLAG(Flag);
    Result = _Xor(RFLAG, ConstructConst(1));
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

  if constexpr (ToSeg) {
    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], 2, Op->Flags, -1);

    switch (Op->Dest.Data.GPR.GPR) {
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
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, gs), Src);
        } else {
          LogMan::Msg::E("We don't support modifying GS selector in 64bit mode!");
          DecodeFailure = true;
        }
        break;
      case 7: // FS
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(GPRClass, 2, offsetof(FEXCore::Core::CPUState, fs), Src);
        } else {
          LogMan::Msg::E("We don't support modifying FS selector in 64bit mode!");
          DecodeFailure = true;
        }
        break;
      default:
        LogMan::Msg::E("Unknown segment register: %d", Op->Dest.Data.GPR.GPR);
        DecodeFailure = true;
        break;
    }
  }
  else {
    OrderedNode *Segment{};

    switch (Op->Src[0].Data.GPR.GPR) {
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
          Segment = ConstructConst(0);
        }
        else {
          Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
        }
        break;
      case 7: // FS
        if (CTX->Config.Is64BitMode) {
          Segment = ConstructConst(0);
        }
        else {
          Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, fs), GPRClass);
        }
        break;
      default:
        LogMan::Msg::E("Unknown segment register: %d", Op->Dest.Data.GPR.GPR);
        DecodeFailure = true;
        return;
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

void OpDispatchBuilder::CPUIDOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Leaf = _LoadContext(4, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

  auto Res = _CPUID(Src, Leaf);

  OrderedNode *Result_Lower = _ExtractElementPair(Res, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(Res, 1);

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), _Bfe(32, 0,  Result_Lower));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), _Bfe(32, 32, Result_Lower));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), _Bfe(32, 32, Result_Upper));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), _Bfe(32, 0,  Result_Upper));
}

template<bool SHL1Bit>
void OpDispatchBuilder::SHLOp(OpcodeArgs) {
  OrderedNode *Src{};
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  if constexpr (SHL1Bit) {
    Src = ConstructConst(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Src = _And(Src, ConstructConst(0x3F));
  }
  else {
    Src = _And(Src, ConstructConst(0x1F));
  }

  OrderedNode *Result = _Lshl(Dest, Src);
  StoreResult(GPRClass, Op, Result, -1);

  if (Size < 32) {
    Result = _Bfe(Size, 0, Result);
  }

  if constexpr (SHL1Bit) {
    GenerateFlags_ShiftLeftImmediate(Op, Result, Dest, 1);
  }
  else {
    GenerateFlags_ShiftLeft(Op, Result, Dest, Src);
  }
}

void OpDispatchBuilder::SHLImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = ConstructConst(Size, Shift);

  OrderedNode *Result = _Lshl(Dest, Src);

  StoreResult(GPRClass, Op, Result, -1);

  if (Size < 32)
    Result = _Bfe(Size, 0, Result);

  GenerateFlags_ShiftLeftImmediate(Op, Result, Dest, Shift);
}

template<bool SHR1Bit>
void OpDispatchBuilder::SHROp(OpcodeArgs) {
  OrderedNode *Src;
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  if constexpr (SHR1Bit) {
    Src = ConstructConst(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Src = _And(Src, ConstructConst(0x3F));
  }
  else {
    Src = _And(Src, ConstructConst(0x1F));
  }

  auto ALUOp = _Lshr(Dest, Src);
  StoreResult(GPRClass, Op, ALUOp, -1);

  if constexpr (SHR1Bit) {
    GenerateFlags_ShiftRightImmediate(Op, ALUOp, Dest, 1);
  }
  else {
    GenerateFlags_ShiftRight(Op, ALUOp, Dest, Src);
  }
}

void OpDispatchBuilder::SHRImmediateOp(OpcodeArgs) {
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = ConstructConst(Size, Shift);

  auto ALUOp = _Lshr(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_ShiftRightImmediate(Op, ALUOp, Dest, Shift);
}

void OpDispatchBuilder::SHLDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *Shift = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], 1, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = _And(Shift, ConstructConst(0x3F));
  else
    Shift = _And(Shift, ConstructConst(0x1F));

  auto ShiftRight = _Sub(ConstructConst(Size), Shift);

  OrderedNode *Res{};
  auto Tmp1 = _Lshl(Dest, Shift);
  Tmp1.first->Header.Size = 8;
  auto Tmp2 = _Lshr(Src, ShiftRight);

  Res = _Or(Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  Res = _Select(FEXCore::IR::COND_EQ,
    Shift, ConstructConst(0),
    Dest, Res);

  StoreResult(GPRClass, Op, Res, -1);

  auto CondJump = _CondJump(Shift, {COND_EQ});

  auto CurrentBlock = GetCurrentBlock();

  // Do nothing if shift count is zero
  auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  if (Size != 64)
    Res = _Bfe(Size, 0, Res);
  GenerateFlags_ShiftLeft(Op, Res, Dest, Shift);

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlockAfter(JumpTarget);
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}


void OpDispatchBuilder::SHLDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  if (Shift != 0) {
    OrderedNode *ShiftLeft = ConstructConst(Shift);
    auto ShiftRight = ConstructConst(Size - Shift);

    OrderedNode *Res{};
    auto Tmp1 = _Lshl(Dest, ShiftLeft);
    Tmp1.first->Header.Size = 8;
    auto Tmp2 = _Lshr(Src, ShiftRight);
    Res = _Or(Tmp1, Tmp2);

    StoreResult(GPRClass, Op, Res, -1);

    if (Size != 64)
      Res = _Bfe(Size, 0, Res);
    GenerateFlags_ShiftLeftImmediate(Op, Res, Dest, Shift);
  }
  else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, -1);
  }
}

void OpDispatchBuilder::SHRDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *Shift = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

  auto Size = GetDstSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = _And(Shift, ConstructConst(0x3F));
  else
    Shift = _And(Shift, ConstructConst(0x1F));


  OrderedNode *Res{};

  auto ShiftLeft = _Sub(ConstructConst(Size), Shift);

  auto Tmp1 = _Lshr(Dest, Shift);
  auto Tmp2 = _Lshl(Src, ShiftLeft);
  Tmp2.first->Header.Size = 8;
  Res = _Or(Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  Res = _Select(FEXCore::IR::COND_EQ,
    Shift, ConstructConst(0),
    Dest, Res);

  StoreResult(GPRClass, Op, Res, -1);

  auto CondJump = _CondJump(Shift, {COND_EQ});

  // Do not change flags if shift count is zero
  auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  if (Size != 64)
    Res = _Bfe(Size, 0, Res);

  GenerateFlags_ShiftRight(Op, Res, Dest, Shift);

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlockAfter(JumpTarget);
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}

void OpDispatchBuilder::SHRDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = Op->Src[1].Data.Literal.Value & 0x3F;
  else
    Shift = Op->Src[1].Data.Literal.Value & 0x1F;

  if (Shift != 0) {
    OrderedNode *ShiftRight = ConstructConst(Shift);
    auto ShiftLeft = ConstructConst(Size - Shift);

    OrderedNode *Res{};
    auto Tmp1 = _Lshr(Dest, ShiftRight);
    auto Tmp2 = _Lshl(Src, ShiftLeft);
    Tmp2.first->Header.Size = 8;
    Res = _Or(Tmp1, Tmp2);

    StoreResult(GPRClass, Op, Res, -1);

    if (Size != 64)
      Res = _Bfe(Size, 0, Res);
    GenerateFlags_ShiftRightImmediate(Op, Res, Dest, Shift);
  }
  else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, -1);
  }
}

template<bool SHR1Bit>
void OpDispatchBuilder::ASHROp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  if constexpr (SHR1Bit) {
    Src = ConstructConst(Size, 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Src = _And(Src, ConstructConst(Size, 0x3F));
  }
  else {
    Src = _And(Src, ConstructConst(Size, 0x1F));
  }

  if (Size < 32) {
    Dest = _Sbfe(Size, 0, Dest);
  }

  OrderedNode *Result = _Ashr(Dest, Src);
  StoreResult(GPRClass, Op, Result, -1);

  if constexpr (SHR1Bit) {
    GenerateFlags_SignShiftRightImmediate(Op, Result, Dest, 1);
  }
  else {
    GenerateFlags_SignShiftRight(Op, Result, Dest, Src);
  }
}

void OpDispatchBuilder::ASHRImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  if (Size < 32) {
    Dest = _Sbfe(Size, 0, Dest);
  }

  OrderedNode *Src = ConstructConst(Size, Shift);
  OrderedNode *Result = _Ashr(Dest, Src);

  StoreResult(GPRClass, Op, Result, -1);

  GenerateFlags_SignShiftRightImmediate(Op, Result, Dest, Shift);
}

template<bool Is1Bit>
void OpDispatchBuilder::ROROp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;
  if constexpr (Is1Bit) {
    Src = ConstructConst(std::max(32, Size), 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Src = _And(Src, ConstructConst(Size, 0x3F));
  }
  else {
    Src = _And(Src, ConstructConst(Size, 0x1F));
  }

  if (Size < 32) {
    // ARM doesn't support 8/16bit rotates. Emulate with an insert
    // StoreResult truncates back to a 8/16 bit value
    Dest = _Bfi(4, Size, Size, Dest, Dest);
    if (Size == 8 && !Is1Bit) {
      // And because the shift size isn't masked to 8 bits, we need to fill the
      // the full 32bits to get the correct result.
      Dest = _Bfi(4, 16, 16, Dest, Dest);
    }
  }

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  if constexpr (Is1Bit) {
    GenerateFlags_RotateRightImmediate(Op, ALUOp, Dest, 1);
  }
  else {
    GenerateFlags_RotateRight(Op, ALUOp, Dest, Src);
  }
}

void OpDispatchBuilder::RORImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift &= 0x3F;
  else
    Shift &= 0x1F;

  OrderedNode *Src = ConstructConst(std::max(32, Size), Shift);

  if (Size < 32) {
    // ARM doesn't support 8/16bit rotates. Emulate with an insert
    // StoreResult truncates back to a 8/16 bit value
    Dest = _Bfi(4, Size, Size, Dest, Dest);
    if (Size == 8 && Shift > 8) {
      // And because the shift size isn't masked to 8 bits, we need to fill the
      // the full 32bits to get the correct result.
      Dest = _Bfi(4, 16, 16, Dest, Dest);
    }
  }

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_RotateRightImmediate(Op, ALUOp, Dest, Shift);
}

template<bool Is1Bit>
void OpDispatchBuilder::ROLOp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op) * 8;

  // Need to negate the shift so we can use ROR instead
  if constexpr (Is1Bit) {
    Src = ConstructConst(Size, 1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  }

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Src = _And(Src, ConstructConst(Size, 0x3F));
  }
  else {
    Src = _And(Src, ConstructConst(Size, 0x1F));
  }

  if (Size < 32) {
    // ARM doesn't support 8/16bit rotates. Emulate with an insert
    // StoreResult truncates back to a 8/16 bit value
    Dest = _Bfi(4, Size, Size, Dest, Dest);
    if (Size == 8) {
      // And because the shift size isn't masked to 8 bits, we need to fill the
      // the full 32bits to get the correct result.
      Dest = _Bfi(4, 16, 16, Dest, Dest);
    }
  }

  auto ALUOp = _Ror(Dest, _Sub(ConstructConst(Size, std::max(32, Size)), Src));

  StoreResult(GPRClass, Op, ALUOp, -1);

  if constexpr (Is1Bit) {
    GenerateFlags_RotateLeftImmediate(Op, ALUOp, Dest, 1);
  }
  else {
    GenerateFlags_RotateLeft(Op, ALUOp, Dest, Src);
  }
}

void OpDispatchBuilder::ROLImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Shift = Shift & 0x3F;
  else
    Shift = Shift & 0x1F;

  // We also negate the shift so we can emulate Rol with Ror.
  auto NegatedShift = std::max(32, Size) - Shift;
  OrderedNode *Src = ConstructConst(Size, NegatedShift);

  if (Size < 32) {
    // ARM doesn't support 8/16bit rotates. Emulate with an insert
    // StoreResult truncates back to a 8/16 bit value
    Dest = _Bfi(4, Size, Size, Dest, Dest);
    if (Size == 8) {
      // And because the shift size isn't masked to 8 bits, we need to fill the
      // the full 32bits to get the correct result.
      Dest = _Bfi(4, 16, 16, Dest, Dest);
    }
  }

  auto ALUOp = _Ror(Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);

  GenerateFlags_RotateLeftImmediate(Op, ALUOp, Dest, Shift);
}

void OpDispatchBuilder::RCROp1Bit(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto Size = GetSrcSize(Op) * 8;
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  uint32_t Shift = 1;

  if (Size == 32 || Size == 64) {
    // Rotate and insert CF in the upper bit
    auto Res = _Extr(CF, Dest, Shift);

    // Our new CF will be bit (Shift - 1) of the source
    auto NewCF = _Bfe(1, Shift - 1, Dest);

    StoreResult(GPRClass, Op, Res, -1);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);

    if (Shift == 1) {
      // OF is the top two MSBs XOR'd together
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, Size - 1, Res), _Bfe(1, Size - 2, Res)));
    }
  }
  else {
    // Res = Src >> Shift
    OrderedNode *Res = _Bfe(Size - Shift, Shift, Dest);

    // inject the CF
    Res = _Or(Res, _Lshl(CF, ConstructConst(Size, Size - Shift)));

    StoreResult(GPRClass, Op, Res, -1);

    // CF only changes if we actually shifted
    // Our new CF will be bit (Shift - 1) of the source
    auto NewCF = _Bfe(1, Shift - 1, Dest);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, Size - 1, Res), _Bfe(1, Size - 2, Res)));
  }
}

void OpDispatchBuilder::RCROp8x1Bit(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto Size = GetSrcSize(Op) * 8;
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  uint32_t Shift = 1;

  // Rotate and insert CF in the upper bit
  OrderedNode *Res = _Bfe(7, 1, Dest);
  Res = _Bfi(Size/8, 1, 7, Res, CF);

  // Our new CF will be bit (Shift - 1) of the source
  auto NewCF = _Bfe(1, Shift - 1, Dest);

  StoreResult(GPRClass, Op, Res, -1);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);

  if (Shift == 1) {
    // OF is the top two MSBs XOR'd together
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, Size - 1, Res), _Bfe(1, Size - 2, Res)));
  }
}

void OpDispatchBuilder::RCROp(OpcodeArgs) {
  auto Size = GetSrcSize(Op) * 8;

  if (Size == 8 || Size == 16) {
    RCRSmallerOp(Op);
    return;
  }

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, ConstructConst(Size, 0x3F));
  else
    Src = _And(Src, ConstructConst(Size, 0x1F));

  // Res = Src >> Shift
  OrderedNode *Res = _Lshr(Dest, Src);

  // Res |= (Src << (Size - Shift + 1));
  OrderedNode *SrcShl = _Sub(ConstructConst(Size, Size + 1), Src);
  auto TmpHigher = _Lshl(Dest, SrcShl);

  auto One = ConstructConst(Size, 1);
  auto Zero = ConstructConst(Size, 0);

  auto CompareResult = _Select(FEXCore::IR::COND_UGT,
    Src, One,
    TmpHigher, Zero);

  Res = _Or(Res, CompareResult);

  // If Shift != 0 then we can inject the CF
  OrderedNode *CFShl = _Sub(ConstructConst(Size, Size), Src);
  auto TmpCF = _Lshl(CF, CFShl);
  TmpCF.first->Header.Size = 8;

  CompareResult = _Select(FEXCore::IR::COND_UGE,
    Src, One,
    TmpCF, Zero);

  Res = _Or(Res, CompareResult);

  StoreResult(GPRClass, Op, Res, -1);

  // CF only changes if we actually shifted
  // Our new CF will be bit (Shift - 1) of the source
  auto NewCF = _Lshr(Dest, _Sub(Src, One));
  CompareResult = _Select(FEXCore::IR::COND_UGE,
    Src, One,
    NewCF, CF);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CompareResult);

  // OF is the top two MSBs XOR'd together
  // Only when Shift == 1, it is undefined otherwise
  // Only changed if shift isn't zero
  auto OF = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
  auto NewOF = _Xor(_Bfe(1, Size - 1, Res), _Bfe(1, Size - 2, Res));
  CompareResult = _Select(FEXCore::IR::COND_EQ,
    Src, ConstructConst(0),
    OF, NewOF);

  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(CompareResult);
}

void OpDispatchBuilder::RCRSmallerOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  Src = _And(Src, ConstructConst(Size, 0x1F));

  OrderedNode *Tmp = ConstructConst(64, 0);

  // Insert the incoming value across the temporary 64bit source
  // Make sure to insert at <BitSize> + 1 offsets
  // We need to cover 32bits plus the amount that could rotate in
  for (size_t i = 0; i < (32 + Size + 1); i += (Size + 1)) {
    // Insert incoming value
    Tmp = _Bfi(8, Size, i, Tmp, Dest);

    // Insert CF
    Tmp = _Bfi(8, 1, i + Size, Tmp, CF);
  }

  // Entire bitfield has been setup
  // Just extract the 8 or 16bits we need
  OrderedNode *Res = _Lshr(Tmp, Src);

  StoreResult(GPRClass, Op, Res, -1);

  // CF only changes if we actually shifted
  // Our new CF will be bit (Shift - 1) of the source
  auto One = ConstructConst(Size, 1);
  auto NewCF = _Lshr(Tmp, _Sub(Src, One));
  auto CompareResult = _Select(FEXCore::IR::COND_UGE,
    Src, One,
    NewCF, CF);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CompareResult);

  // OF is the top two MSBs XOR'd together
  // Only when Shift == 1, it is undefined otherwise
  // Make it easier, just store it regardless
  auto NewOF = _Xor(_Bfe(1, Size - 1, Res), _Bfe(1, Size - 2, Res));
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(NewOF);
}

void OpDispatchBuilder::RCLOp1Bit(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto Size = GetSrcSize(Op) * 8;
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  uint32_t Shift = 1;

  // Rotate left and insert CF in to lowest bit
  OrderedNode *Res = _Lshl(Dest, ConstructConst(Size, 1));
  Res = _Or(Res, CF);

  // Our new CF will be the top bit of the source
  auto NewCF = _Bfe(1, Size - 1, Dest);

  StoreResult(GPRClass, Op, Res, -1);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);

  if (Shift == 1) {
    // OF is the top two MSBs XOR'd together
    // Top two MSBs is CF and top bit of result
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, Size - 1, Res), NewCF));
  }
}

void OpDispatchBuilder::RCLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op) * 8;

  if (Size == 8 || Size == 16) {
    RCLSmallerOp(Op);
    return;
  }

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64)
    Src = _And(Src, ConstructConst(Size, 0x3F));
  else
    Src = _And(Src, ConstructConst(Size, 0x1F));

  // Res = Src << Shift
  OrderedNode *Res = _Lshl(Dest, Src);

  // Res |= (Src << (Size - Shift + 1));
  OrderedNode *SrcShl = _Sub(ConstructConst(Size, Size + 1), Src);
  auto TmpHigher = _Lshr(Dest, SrcShl);

  auto One = ConstructConst(Size, 1);
  auto Zero = ConstructConst(Size, 0);

  auto CompareResult = _Select(FEXCore::IR::COND_UGT,
    Src, One,
    TmpHigher, Zero);

  Res = _Or(Res, CompareResult);

  // If Shift != 0 then we can inject the CF
  OrderedNode *CFShl = _Sub(Src, ConstructConst(Size, 1));
  auto TmpCF = _Lshl(CF, CFShl);
  TmpCF.first->Header.Size = 8;

  CompareResult = _Select(FEXCore::IR::COND_UGE,
    Src, One,
    TmpCF, Zero);

  Res = _Or(Res, CompareResult);

  StoreResult(GPRClass, Op, Res, -1);

  {
    // CF only changes if we actually shifted
    // Our new CF will be bit (Shift - 1) of the source
    auto NewCF = _Lshr(Dest, _Sub(ConstructConst(Size, Size), Src));
    CompareResult = _Select(FEXCore::IR::COND_UGE,
      Src, One,
      NewCF, CF);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CompareResult);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    // Only changed if shift isn't zero
    auto OF = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    auto NewOF = _Xor(_Bfe(1, Size - 1, Res), NewCF);
    CompareResult = _Select(FEXCore::IR::COND_EQ,
      Src, ConstructConst(0),
      OF, NewOF);

    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(CompareResult);
  }
}

void OpDispatchBuilder::RCLSmallerOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);

  auto Size = GetSrcSize(Op) * 8;

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  Src = _And(Src, ConstructConst(Size, 0x1F));

  OrderedNode *Tmp = ConstructConst(64, 0);

  for (size_t i = 0; i < (32 + Size + 1); i += (Size + 1)) {
    // Insert incoming value
    Tmp = _Bfi(8, Size, 63 - i - Size, Tmp, Dest);

    // Insert CF
    Tmp = _Bfi(8, 1, 63 - i, Tmp, CF);
  }

  // Insert incoming value
  Tmp = _Bfi(8, Size, 0, Tmp, Dest);

  // The data is now set up like this
  // [Data][CF]:[Data][CF]:[Data][CF]:[Data][CF]
  // Shift 1 more bit that expected to get our result
  // Shifting to the right will now behave like a rotate to the left
  // Which we emulate with a _Ror
  OrderedNode *Res = _Ror(Tmp, _Sub(ConstructConst(Size, 64), Src));

  StoreResult(GPRClass, Op, Res, -1);

  {
    // Our new CF is now at the bit position that we are shifting
    // Either 0 if CF hasn't changed (CF is living in bit 0)
    // or higher
    auto NewCF = _Ror(Tmp, _Sub(ConstructConst(63), Src));
    auto CompareResult = _Select(FEXCore::IR::COND_UGE,
      Src, ConstructConst(1),
      NewCF, CF);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CompareResult);

    // OF is only defined for 1 bit shifts
    // To make it easy, just always store a result
    // OF is the XOR of the NewCF and the MSB of the result
    // Only changed if shift isn't zero
    auto OF = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    auto NewOF = _Xor(_Bfe(1, Size - 1, Res), NewCF);
    CompareResult = _Select(FEXCore::IR::COND_EQ,
      Src, ConstructConst(0),
      OF, NewOF);

    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(CompareResult);
  }
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src{};
  bool AlreadyMasked{};

  uint32_t Size = GetDstSize(Op) * 8;
  uint32_t Mask = Size - 1;

  if (Op->Src[SrcIndex].IsGPR()) {
    Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  }
  else {
    // Can only be an immediate
    // Masked by operand size
    Src = ConstructConst(Size, Op->Src[SrcIndex].Data.Literal.Value & Mask);
    AlreadyMasked = true;
  }

  if (Op->Dest.IsGPR()) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    OrderedNode *BitSelect{};
    if (AlreadyMasked) {
      BitSelect = Src;
    }
    else {
      OrderedNode *SizeMask = ConstructConst(Mask);

      // Get the bit selection from the src
      BitSelect = _And(Src, SizeMask);
    }

    Result = _Lshr(Dest, BitSelect);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = _Sbfe(Size-3,3, Src);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    Result = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

    // Now shift in to the correct bit location
    Result = _Lshr(Result, BitSelect);
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTROp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src{};
  bool AlreadyMasked{};

  uint32_t Size = GetDstSize(Op) * 8;
  uint32_t Mask = Size - 1;

  if (Op->Src[SrcIndex].IsGPR()) {
    Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  }
  else {
    // Can only be an immediate
    // Masked by operand size
    Src = ConstructConst(Size, Op->Src[SrcIndex].Data.Literal.Value & Mask);
    AlreadyMasked = true;
  }

  if (Op->Dest.IsGPR()) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    OrderedNode *BitSelect{};
    if (AlreadyMasked) {
      BitSelect = Src;
    }
    else {
      OrderedNode *SizeMask = ConstructConst(Mask);

      // Get the bit selection from the src
      BitSelect = _And(Src, SizeMask);
    }

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);
    BitMask = _Not(BitMask);
    Dest = _And(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    Dest = AppendSegmentOffset(Dest, Op->Flags);

    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = _Sbfe(Size-3,3, Src);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);
    BitMask = _Not(BitMask);

    if (DestIsLockedMem(Op)) {
      HandledLock = true;
      // XXX: Technically this can optimize to an AArch64 ldclralb
      // We don't current support this IR op though
      Result = _AtomicFetchAnd(MemoryLocation, BitMask, 1);
      // Now shift in to the correct bit location
      Result = _Lshr(Result, BitSelect);
    }
    else {
      OrderedNode *Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

      // Now shift in to the correct bit location
      Result = _Lshr(Value, BitSelect);
      Value = _And(Value, BitMask);
      _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Value, 1);
    }
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTSOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src{};
  bool AlreadyMasked{};

  uint32_t Size = GetDstSize(Op) * 8;
  uint32_t Mask = Size - 1;

  if (Op->Src[SrcIndex].IsGPR()) {
    Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  }
  else {
    // Can only be an immediate
    // Masked by operand size
    Src = ConstructConst(Size, Op->Src[SrcIndex].Data.Literal.Value & Mask);
    AlreadyMasked = true;
  }

  if (Op->Dest.IsGPR()) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    OrderedNode *BitSelect{};
    if (AlreadyMasked) {
      BitSelect = Src;
    }
    else {
      OrderedNode *SizeMask = ConstructConst(Mask);

      // Get the bit selection from the src
      BitSelect = _And(Src, SizeMask);
    }

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);
    Dest = _Or(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = _Sbfe(Size-3,3, Src);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);

    if (DestIsLockedMem(Op)) {
      HandledLock = true;
      Result = _AtomicFetchOr(MemoryLocation, BitMask, 1);
      // Now shift in to the correct bit location
      Result = _Lshr(Result, BitSelect);
    }
    else {
      OrderedNode *Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

      // Now shift in to the correct bit location
      Result = _Lshr(Value, BitSelect);
      Value = _Or(Value, BitMask);
      _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Value, 1);
    }
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::BTCOp(OpcodeArgs) {
  OrderedNode *Result;
  OrderedNode *Src{};
  bool AlreadyMasked{};

  uint32_t Size = GetDstSize(Op) * 8;
  uint32_t Mask = Size - 1;

  if (Op->Src[SrcIndex].IsGPR()) {
    Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  }
  else {
    // Can only be an immediate
    // Masked by operand size
    Src = ConstructConst(Size, Op->Src[SrcIndex].Data.Literal.Value & Mask);
    AlreadyMasked = true;
  }

  if (Op->Dest.IsGPR()) {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    OrderedNode *BitSelect{};
    if (AlreadyMasked) {
      BitSelect = Src;
    }
    else {
      OrderedNode *SizeMask = ConstructConst(Mask);

      // Get the bit selection from the src
      BitSelect = _And(Src, SizeMask);
    }

    Result = _Lshr(Dest, BitSelect);

    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);
    Dest = _Xor(Dest, BitMask);
    StoreResult(GPRClass, Op, Dest, -1);
  }
  else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = _Sbfe(Size-3,3, Src);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(Dest, Src);
    OrderedNode *BitMask = _Lshl(ConstructConst(1), BitSelect);

    if (DestIsLockedMem(Op)) {
      HandledLock = true;
      Result = _AtomicFetchXor(MemoryLocation, BitMask, 1);
      // Now shift in to the correct bit location
      Result = _Lshr(Result, BitSelect);
    }
    else {
      OrderedNode *Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

      // Now shift in to the correct bit location
      Result = _Lshr(Value, BitSelect);
      Value = _Xor(Value, BitMask);
      _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Value, 1);
    }
  }
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Result);
}

void OpDispatchBuilder::IMUL1SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  uint8_t Size = GetSrcSize(Op);
  if (Size != 8) {
    Src1 = _Sext(Size * 8, Src1);
    Src2 = _Sext(Size * 8, Src2);
  }

  auto Dest = _Mul(Src1, Src2);
  OrderedNode *ResultHigh{};
  if (Size < 8) {
    ResultHigh = _Sbfe(Size * 8, Size * 8, Dest);
  }
  else {
    ResultHigh = _MulH(Src1, Src2);
  }
  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, ResultHigh);
}

void OpDispatchBuilder::IMUL2SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, -1);

  uint8_t Size = GetSrcSize(Op);
  if (Size != 8) {
    Src1 = _Sext(Size * 8, Src1);
    Src2 = _Sext(Size * 8, Src2);
  }

  auto Dest = _Mul(Src1, Src2);
  OrderedNode *ResultHigh{};
  if (Size < 8) {
    ResultHigh = _Sbfe(Size * 8, Size * 8, Dest);
  }
  else {
    ResultHigh = _MulH(Src1, Src2);
  }

  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, ResultHigh);
}

void OpDispatchBuilder::IMULOp(OpcodeArgs) {
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
    ResultHigh = _Sbfe(8, 8, Result);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    ResultHigh = _Sbfe(16, 16, Result);
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    auto LocalResult = _Bfe(32, 0, Result);
    auto LocalResultHigh = _Bfe(32, 32, Result);
    ResultHigh = _Sbfe(32, 32, Result);
    Result = _Sbfe(32, 0, Result);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), LocalResult);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), LocalResultHigh);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::E("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _MulH(Src1, Src2);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }

  GenerateFlags_MUL(Op, Result, ResultHigh);
}

void OpDispatchBuilder::MULOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);
  const uint8_t GPRSize = CTX->GetGPRSize();

  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
  if (Size != 8) {
    Src1 = _Bfe(8, Size * 8, 0, Src1);
    Src2 = _Bfe(8, Size * 8, 0, Src2);
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
    OrderedNode *ResultLow = _Bfe(GPRSize, 32, 0, Result);
    ResultHigh = _Bfe(GPRSize, 32, 32, Result);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), ResultLow);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), ResultHigh);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::E("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
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
    MaskConst = ConstructConst(~0ULL);
  }
  else {
    MaskConst = ConstructConst((1ULL << (Size * 8)) - 1);
  }

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    _AtomicXor(DestMem, MaskConst, Size);
  }
  else {
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Src = _Xor(Src, MaskConst);
    StoreResult(GPRClass, Op, Src, -1);
  }
}

void OpDispatchBuilder::XADDOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result;

  auto Size = GetSrcSize(Op) * 8;

  if (Op->Dest.IsGPR()) {
    // If this is a GPR then we can just do an Add
    Result = _Add(Dest, Src);

    // Previous value in dest gets stored in src
    StoreResult(GPRClass, Op, Op->Src[0], Dest, -1);

    // Calculated value gets stored in dst (order is important if dst is same as src)
    StoreResult(GPRClass, Op, Result, -1);

    if (Size < 32)
      Result = _Bfe(Size, 0, Result);

    GenerateFlags_ADD(Op, Result, Dest, Src);
  }
  else {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    auto Before = _AtomicFetchAdd(Dest, Src, GetSrcSize(Op));
    StoreResult(GPRClass, Op, Op->Src[0], Before, -1);
    Result = _Add(Before, Src); // Seperate result just for flags

    if (Size < 32)
      Result = _Bfe(Size, 0, Result);

    GenerateFlags_ADD(Op, Result, Before, Src);
  }
}

void OpDispatchBuilder::PopcountOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  Src = _Popcount(Src);
  StoreResult(GPRClass, Op, Src, -1);
  // Set ZF
  auto Zero = ConstructConst(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      ConstructConst(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(Zero);
}

void OpDispatchBuilder::XLATOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  OrderedNode *Src = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), GPRClass);
  OrderedNode *Offset = _LoadContext(1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);

  Src = AppendSegmentOffset(Src, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

  Src = _Add(Src, Offset);

  auto Res = _LoadMemAutoTSO(GPRClass, 1, Src, 1);

  _StoreContext(GPRClass, 1, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Res);
}

template<OpDispatchBuilder::Segment Seg>
void OpDispatchBuilder::ReadSegmentReg(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src{};
  if constexpr (Seg == Segment_FS) {
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
  if constexpr (Seg == Segment_FS) {
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, fs), Src);
  }
  else {
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gs), Src);
  }
}

void OpDispatchBuilder::EnterOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  LOGMAN_THROW_A(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Value = Op->Src[0].Data.Literal.Value;

  const uint16_t AllocSpace = Value & 0xFFFF;
  const uint8_t Level = (Value >> 16) & 0x1F;

  const auto PushValue = [&](uint8_t Size, OrderedNode *Src) {
    auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

    auto NewSP = _Sub(OldSP, ConstructConst(Size));
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
      auto Offset = ConstructConst(i * GPRSize);
      auto MemLoc = _Sub(OldBP, Offset);
      auto Mem = _LoadMem(GPRClass, GPRSize, MemLoc, GPRSize);
      NewSP = PushValue(GPRSize, Mem);
    }
    NewSP = PushValue(GPRSize, temp_RBP);
  }
  NewSP = _Sub(NewSP, ConstructConst(AllocSpace));
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]), temp_RBP);
}

void OpDispatchBuilder::RDTSCOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  auto Counter = _CycleCounter();
  auto CounterLow = _Bfe(32, 0, Counter);
  auto CounterHigh = _Bfe(32, 32, Counter);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CounterLow);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), CounterHigh);
}

void OpDispatchBuilder::INCOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::E("Can't handle REP on this");
    DecodeFailure = true;
    return;
  }

  OrderedNode *Dest;
  OrderedNode *Result;
  auto Size = GetSrcSize(Op) * 8;
  auto OneConst = ConstructConst(Size, 1);

  bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;
    auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestAddress = AppendSegmentOffset(DestAddress, Op->Flags);
    Dest = _AtomicFetchAdd(DestAddress, OneConst, GetSrcSize(Op));

  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  }

  Result = _Add(Dest, OneConst);
  if (!IsLocked) {
    StoreResult(GPRClass, Op, Result, -1);
  }

  if (Size < 32) {
    Result = _Bfe(Size, 0, Result);
  }
  GenerateFlags_ADD(Op, Result, Dest, OneConst, false);
}

void OpDispatchBuilder::DECOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::E("Can't handle REP on this");
    DecodeFailure = true;
    return;
  }

  OrderedNode *Dest;
  OrderedNode *Result;
  auto Size = GetSrcSize(Op) * 8;
  auto OneConst = ConstructConst(Size, 1);

  bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;
    auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestAddress = AppendSegmentOffset(DestAddress, Op->Flags);
    Dest = _AtomicFetchSub(DestAddress, OneConst, GetSrcSize(Op));

  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  }

  Result = _Sub(Dest, OneConst);
  if (!IsLocked)
    StoreResult(GPRClass, Op, Result, -1);

  if (Size < 32) {
    Result = _Bfe(Size, 0, Result);
  }
  GenerateFlags_SUB(Op, Result, Dest, OneConst, false);
}

void OpDispatchBuilder::STOSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX) {
    LogMan::Msg::E("Invalid REPNE on STOS");
    DecodeFailure = true;
    return;
  }
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::E("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) != 0;

  if (!Repeat) {
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

    // Only ES prefix
    Dest = AppendSegmentOffset(Dest, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, Dest, Src, Size);

    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    // Calculate direction.
    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF,  ConstructConst(0),
        SizeConst, NegSizeConst);

    // Offset the pointer
    OrderedNode *TailDest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    TailDest = _Add(TailDest, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest);

  }
  else {
    // Create all our blocks
    auto LoopHead = CreateNewCodeBlockAfter(GetCurrentBlock());
    auto LoopTail = CreateNewCodeBlockAfter(LoopHead);
    auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);



    // At the time this was written, our RA can't handle accessing nodes across blocks.
    // So we need to re-load and re-calculate essential values each iteration of the loop.

    // First thing we need to do is finish this block and jump to the start of the loop.

    // RA can now better allocate things, move these ops before the header, to avoid accessing
    // DF on every iteration
    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    // Calculate direction.
    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF,  ConstructConst(0),
        SizeConst, NegSizeConst);

    _Jump(LoopHead);

    SetCurrentCodeBlock(LoopHead);
    {
      OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
      // Can we end the block?
      _CondJump(Counter, LoopEnd, LoopTail, {COND_EQ});
    }

    SetCurrentCodeBlock(LoopTail);
    {
      OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

      // Only ES prefix
      Dest = AppendSegmentOffset(Dest, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

      // Store to memory where RDI points
      _StoreMemAutoTSO(GPRClass, Size, Dest, Src, Size);

      OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
      OrderedNode *TailDest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

      // Decrement counter
      TailCounter = _Sub(TailCounter, ConstructConst(1));

      // Store the counter so we don't have to deal with PHI here
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

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
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX) {
    LogMan::Msg::E("Invalid REPNE on MOVS");
    DecodeFailure = true;
    return;
  }
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::E("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  // RA now can handle these to be here, to avoid DF accesses
  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);
  auto SizeConst = ConstructConst(Size);
  auto NegSizeConst = ConstructConst(-Size);

  // Calculate direction.
  auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
  auto PtrDir = _Select(FEXCore::IR::COND_EQ, DF,  ConstructConst(0), SizeConst, NegSizeConst);

  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    // Create all our blocks
    auto LoopHead = CreateNewCodeBlockAfter(GetCurrentBlock());
    auto LoopTail = CreateNewCodeBlockAfter(LoopHead);
    auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);


    // At the time this was written, our RA can't handle accessing nodes across blocks.
    // So we need to re-load and re-calculate essential values each iteration of the loop.

    // First thing we need to do is finish this block and jump to the start of the loop.

    _Jump(LoopHead);

    SetCurrentCodeBlock(LoopHead);
    {
      OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
      _CondJump(Counter, LoopEnd, LoopTail, {COND_EQ});
    }

    SetCurrentCodeBlock(LoopTail);
    {
      OrderedNode *Src = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
      OrderedNode *Dest = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
      Dest = AppendSegmentOffset(Dest, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
      Src = AppendSegmentOffset(Src, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

      Src = _LoadMemAutoTSO(GPRClass, Size, Src, Size);

      // Store to memory where RDI points
      _StoreMemAutoTSO(GPRClass, Size, Dest, Src, Size);

      OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

      // Decrement counter
      TailCounter = _Sub(TailCounter, ConstructConst(1));

      // Store the counter so we don't have to deal with PHI here
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

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
    OrderedNode *RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
    OrderedNode *RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    RDI= AppendSegmentOffset(RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    RSI = AppendSegmentOffset(RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, RSI, Size);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, RDI, Src, Size);

    RSI = _Add(RSI, PtrDir);
    RDI = _Add(RDI, PtrDir);

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), RSI);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), RDI);
  }
}

void OpDispatchBuilder::CMPSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::E("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);

  bool Repeat = Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX);
  if (!Repeat) {
    OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

    // Only ES prefix
    Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    // Default DS prefix
    Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    OrderedNode* Result = _Sub(Src2, Src1);
    if (Size < 4)
      Result = _Bfe(Size * 8, 0, Result);

    GenerateFlags_SUB(Op, Result, Src2, Src1);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
        ConstructConst(Size), ConstructConst(-Size));

    // Offset the pointer
    Dest_RDI = _Add(Dest_RDI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest_RDI);

    // Offset second pointer
    Dest_RSI = _Add(Dest_RSI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Dest_RSI);
  }
  else {
    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    // read DF once
    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
        ConstructConst(Size), ConstructConst(-Size));

    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
    auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetJumpTarget(JumpStart, LoopStart);
    SetCurrentCodeBlock(LoopStart);

    OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

    // Can we end the block?
    auto CondJump = _CondJump(Counter, {COND_EQ});
    IRPair<IROp_CondJump> InternalCondJump;

    auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
    SetFalseJumpTarget(CondJump, LoopTail);
    SetCurrentCodeBlock(LoopTail);

    // Working loop
    {
      OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
      OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

      // Only ES prefix
      Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
      // Default DS prefix
      Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

      auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
      auto Src2 = _LoadMem(GPRClass, Size, Dest_RSI, Size);

      OrderedNode* Result = _Sub(Src2, Src1);
      if (Size < 4)
        Result = _Bfe(Size * 8, 0, Result);

      GenerateFlags_SUB(Op, Result, Src2, Src1);

      OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

      // Decrement counter
      TailCounter = _Sub(TailCounter, ConstructConst(1));

      // Store the counter so we don't have to deal with PHI here
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

      // Offset the pointer
      Dest_RDI = _Add(Dest_RDI, PtrDir);
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), Dest_RDI);

      // Offset second pointer
      Dest_RSI = _Add(Dest_RSI, PtrDir);
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), Dest_RSI);

      OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      InternalCondJump = _CondJump(ZF, {REPE ? COND_NEQ : COND_EQ});

      // Jump back to the start if we have more work to do
      SetTrueJumpTarget(InternalCondJump, LoopStart);
    }

    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);
    SetTrueJumpTarget(CondJump, LoopEnd);

    SetFalseJumpTarget(InternalCondJump, LoopEnd);

    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::LODSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX) {
    LogMan::Msg::E("Invalid REPNE on LODS");
    DecodeFailure = true;
    return;
  }
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::E("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) != 0;

  if (!Repeat) {
    OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
    Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    StoreResult(GPRClass, Op, Src, -1);

    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
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

    // Read DF once
    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
        SizeConst, NegSizeConst);

    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
    auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetJumpTarget(JumpStart, LoopStart);
    SetCurrentCodeBlock(LoopStart);

    OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

    // Can we end the block?

    // We leave if RCX = 0
    auto CondJump = _CondJump(Counter, {COND_EQ});

    auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
    SetFalseJumpTarget(CondJump, LoopTail);
    SetCurrentCodeBlock(LoopTail);

    // Working loop
    {
      OrderedNode *Dest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);
      Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

      auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

      StoreResult(GPRClass, Op, Src, -1);

      OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
      OrderedNode *TailDest_RSI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), GPRClass);

      // Decrement counter
      TailCounter = _Sub(TailCounter, ConstructConst(1));

      // Store the counter so we don't have to deal with PHI here
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

      // Offset the pointer
      TailDest_RSI = _Add(TailDest_RSI, PtrDir);
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]), TailDest_RSI);

      // Jump back to the start, we have more work to do
      _Jump(LoopStart);
    }
    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);
    SetTrueJumpTarget(CondJump, LoopEnd);
    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::SCASOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::E("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX)) != 0;

  if (!Repeat) {
    OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

    OrderedNode* Result = _Sub(Src1, Src2);
    if (Size < 4)
      Result = _Bfe(Size * 8, 0, Result);

    GenerateFlags_SUB(Op, Result, Src1, Src2);

    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
        SizeConst, NegSizeConst);

    // Offset the pointer
    OrderedNode *TailDest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
    TailDest_RDI = _Add(TailDest_RDI, PtrDir);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest_RDI);
  }
  else {
    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    // read DF once

    auto SizeConst = ConstructConst(Size);
    auto NegSizeConst = ConstructConst(-Size);

    auto DF = GetRFLAG(FEXCore::X86State::RFLAG_DF_LOC);
    auto PtrDir = _Select(FEXCore::IR::COND_EQ,
        DF, ConstructConst(0),
        SizeConst, NegSizeConst);

    auto JumpStart = _Jump();
    // Make sure to start a new block after ending this one
    auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetJumpTarget(JumpStart, LoopStart);
    SetCurrentCodeBlock(LoopStart);

    OrderedNode *Counter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);

    // Can we end the block?
    // We leave if RCX = 0
    auto CondJump = _CondJump(Counter, {COND_EQ});
    IRPair<IROp_CondJump> InternalCondJump;

    auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
    SetFalseJumpTarget(CondJump, LoopTail);
    SetCurrentCodeBlock(LoopTail);

    // Working loop
    {
      OrderedNode *Dest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);
      Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

      auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

      OrderedNode* Result = _Sub(Src1, Src2);
      if (Size < 4)
        Result = _Bfe(Size * 8, 0, Result);

      GenerateFlags_SUB(Op, Result, Src1, Src2);

      OrderedNode *TailCounter = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
      OrderedNode *TailDest_RDI = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), GPRClass);

      // Decrement counter
      TailCounter = _Sub(TailCounter, ConstructConst(1));

      // Store the counter so we don't have to deal with PHI here
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), TailCounter);

      // Offset the pointer
      TailDest_RDI = _Add(TailDest_RDI, PtrDir);
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]), TailDest_RDI);

      OrderedNode *ZF = GetRFLAG(FEXCore::X86State::RFLAG_ZF_LOC);
      InternalCondJump = _CondJump(ZF, {REPE ? COND_NEQ : COND_EQ});

      // Jump back to the start if we have more work to do
      SetTrueJumpTarget(InternalCondJump, LoopStart);
    }
    // Make sure to start a new block after ending this one
    auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);
    SetTrueJumpTarget(CondJump, LoopEnd);

    SetFalseJumpTarget(InternalCondJump, LoopEnd);

    SetCurrentCodeBlock(LoopEnd);
  }
}

void OpDispatchBuilder::BSWAPOp(OpcodeArgs) {
  OrderedNode *Dest;
  if (GetSrcSize(Op) == 2) {
    // BSWAP of 16bit is undef. ZEN+ causes the lower 16bits to get zero'd
    Dest = ConstructConst(0);
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Dest = _Rev(Dest);
  }
  StoreResult(GPRClass, Op, Dest, -1);
}

void OpDispatchBuilder::PUSHFOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src = GetPackedRFLAG(false);
  if (Size != 8) {
    Src = _Bfe(Size * 8, 0, Src);
  }

  auto Constant = ConstructConst(Size);
  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);
  auto NewSP = _Sub(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Store our value to the new stack location
  _StoreMem(GPRClass, Size, NewSP, Src, Size);
}

void OpDispatchBuilder::POPFOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t Size = GetSrcSize(Op);

  auto Constant = ConstructConst(Size);

  auto OldSP = _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), GPRClass);

  OrderedNode *Src = _LoadMem(GPRClass, Size, OldSP, Size);

  auto NewSP = _Add(OldSP, Constant);

  // Store the new stack pointer
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]), NewSP);

  // Add back our flag constants
  // Bit 1 is always 1
  // Bit 9 is always 1 because we always have interrupts enabled

  Src = _Or(Src, ConstructConst(Size * 8, 0x202));

  SetPackedRFLAG(false, Src);
}

void OpDispatchBuilder::NEGOp(OpcodeArgs) {
  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;

  auto Size = GetSrcSize(Op);
  auto ZeroConst = ConstructConst(0);

  OrderedNode *Dest{};
  OrderedNode *Result{};

  if (DestIsLockedMem(Op)) {
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);

    Dest = _AtomicFetchNeg(DestMem, Size);
    Result = _Neg(Dest);
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
    Result = _Neg(Dest);

    StoreResult(GPRClass, Op, Result, -1);
  }

  if (Size < 4)
    Result = _Bfe(Size * 8, 0, Result);

  GenerateFlags_SUB(Op, Result, ZeroConst, Dest);
}

void OpDispatchBuilder::DIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);

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

    OrderedNode *UDivOp = _Bfe(Size * 8, 0, _LUDiv(Src1, Src2, Divisor));
    OrderedNode *URemOp = _Bfe(Size * 8, 0, _LURem(Src1, Src2, Divisor));

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::E("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    auto UDivOp = _LUDiv(Src1, Src2, Divisor);
    auto URemOp = _LURem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::IDIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    Src1 = _Sbfe(Src1, 16, 0);
    Divisor = _Sbfe(Divisor, 8, 0);

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

    OrderedNode *UDivOp = _Bfe(Size * 8, 0, _LDiv(Src1, Src2, Divisor));
    OrderedNode *URemOp = _Bfe(Size * 8, 0, _LRem(Src1, Src2, Divisor));

    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::E("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    OrderedNode *Src1 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
    OrderedNode *Src2 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);

    auto UDivOp = _LDiv(Src1, Src2, Divisor);
    auto URemOp = _LRem(Src1, Src2, Divisor);

    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), UDivOp);
    _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), URemOp);
  }
}

void OpDispatchBuilder::BSFOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : GPRSize;
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  // Find the LSB of this source
  auto Result = _FindLSB(Src);

  auto ZeroConst = ConstructConst(0);
  auto OneConst = ConstructConst(1);

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
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : GPRSize;
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  // Find the MSB of this source
  auto Result = _FindMSB(Src);

  auto ZeroConst = ConstructConst(0);
  auto OneConst = ConstructConst(1);

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

  const auto GPRSize = CTX->GetGPRSize();
  auto Size = GetSrcSize(Op);

  // This is our source register
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  // 0x80014000
  // 0x80064000
  // 0x80064000

  if (Op->Dest.IsGPR()) {
    OrderedNode *Src1{};
    OrderedNode *Src1Lower{};

    OrderedNode *Src3{};
    OrderedNode *Src3Lower{};
    if (GPRSize == 8 && Size == 4) {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags, -1);
      Src1Lower = _Bfe(4, 32, 0, Src1);
      Src3 = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
      Src3Lower = _Bfe(4, 32, 0, Src3);
    }
    else {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, Size, Op->Flags, -1);
      Src1Lower = Src1;
			Src3 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
      Src3Lower = Src3;
    }

    // If our destination is a GPR then this behaves differently
    // RAX = RAX == Op1 ? RAX : Op1
    // AKA if they match then don't touch RAX value
    // Otherwise set it to the rm operand
    OrderedNode *CASResult = _Select(FEXCore::IR::COND_EQ,
      Src1Lower, Src3Lower,
      Src3Lower, Src1Lower);

    // Op1 = RAX == Op1 ? Op2 : Op1
    // If they match then set the rm operand to the input
    // else don't set the rm operand
    OrderedNode *DestResult = _Select(FEXCore::IR::COND_EQ,
        Src1Lower, Src3Lower,
        Src2, Src1);

    // Store in to GPR Dest
    // Have to make sure this is after the result store in RAX for when Dest == RAX
    if (GPRSize == 8 && Size == 4) {
      // This allows us to only hit the ZEXT case on failure
      OrderedNode *RAXResult = _Select(FEXCore::IR::COND_EQ,
        CASResult, Src3Lower,
        Src3, Src1Lower);

      // When the size is 4 we need to make sure not zext the GPR when the comparison fails
      _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);
      StoreResult_WithOpSize(GPRClass, Op, Op->Dest, DestResult, GPRSize, -1);
    }
    else {
      _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), CASResult);
      StoreResult(GPRClass, Op, DestResult, -1);
    }

    auto Size = GetDstSize(Op) * 8;

    OrderedNode *Result = _Sub(Src3Lower, CASResult);
    if (Size < 32)
      Result = _Bfe(Size, 0, Result);

    GenerateFlags_SUB(Op, Result, Src3Lower, CASResult);
  }
  else {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

    OrderedNode *Src3{};
    OrderedNode *Src3Lower{};
    if (GPRSize == 8 && Size == 4) {
      Src3 = _LoadContext(8, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
      Src3Lower = _Bfe(4, 32, 0, Src3);
    }
    else {
      Src3 = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
      Src3Lower = Src3;
    }
    // If this is a memory location then we want the pointer to it
    OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

    Src1 = AppendSegmentOffset(Src1, Op->Flags);

    // DataSrc = *Src1
    // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
    // This will write to memory! Careful!
    // Third operand must be a calculated guest memory address
    OrderedNode *CASResult = _CAS(Src3Lower, Src2, Src1);
		OrderedNode *RAXResult = CASResult;

    if (GPRSize == 8 && Size == 4) {
      // This allows us to only hit the ZEXT case on failure
      RAXResult = _Select(FEXCore::IR::COND_EQ,
        CASResult, Src3Lower,
        Src3, CASResult);
      Size = 8;
    }

    // RAX gets the result of the CAS op
    _StoreContext(GPRClass, Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), RAXResult);

    auto Size = GetDstSize(Op) * 8;

    OrderedNode *Result = _Sub(Src3Lower, CASResult);
    if (Size < 32)
      Result = _Bfe(Size, 0, Result);

    GenerateFlags_SUB(Op, Result, Src3Lower, CASResult);
  }
}

void OpDispatchBuilder::CMPXCHGPairOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  // REX.W used to determine if it is 16byte or 8byte
  // Unlike CMPXCHG, the destination can only be a memory location

  const auto Size = GetSrcSize(Op);
  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  // If this is a memory location then we want the pointer to it
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);

  Src1 = AppendSegmentOffset(Src1, Op->Flags);

  OrderedNode *Expected_Lower = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), GPRClass);
  OrderedNode *Expected_Upper = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), GPRClass);
  OrderedNode *Expected = _CreateElementPair(Expected_Lower, Expected_Upper);

  OrderedNode *Desired_Lower = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]), GPRClass);
  OrderedNode *Desired_Upper = _LoadContext(Size, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]), GPRClass);
  OrderedNode *Desired = _CreateElementPair(Desired_Lower, Desired_Upper);

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

  // Set ZF if memory result was expected
  OrderedNode *EOR_Lower = _Xor(Result_Lower, Expected_Lower);
  OrderedNode *EOR_Upper = _Xor(Result_Upper, Expected_Upper);
  OrderedNode *Orr_Result = _Or(EOR_Lower, EOR_Upper);

  auto OneConst = ConstructConst(1);
  auto ZeroConst = ConstructConst(0);
  OrderedNode *ZFResult = _Select(FEXCore::IR::COND_EQ,
    Orr_Result, ZeroConst,
    OneConst, ZeroConst);

  // Set ZF
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);

  auto CondJump = _CondJump(ZFResult);

  // Make sure to start a new block after ending this one
  auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
  SetFalseJumpTarget(CondJump, JumpTarget);
  SetCurrentCodeBlock(JumpTarget);

  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]), Result_Lower);
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]), Result_Upper);

  auto Jump = _Jump();
  auto NextJumpTarget = CreateNewCodeBlockAfter(JumpTarget);
  SetJumpTarget(Jump, NextJumpTarget);
  SetTrueJumpTarget(CondJump, NextJumpTarget);
  SetCurrentCodeBlock(NextJumpTarget);
}

void OpDispatchBuilder::CreateJumpBlocks(std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks) {
  OrderedNode *PrevCodeBlock{};
  for (auto &Target : *Blocks) {
    auto CodeNode = CreateCodeNode();

    JumpTargets.try_emplace(Target.Entry, JumpTargetInfo{CodeNode, false});

    if (PrevCodeBlock) {
      LinkCodeBlocks(PrevCodeBlock, CodeNode);
    }

    PrevCodeBlock = CodeNode;
  }
}

void OpDispatchBuilder::BeginFunction(uint64_t RIP, std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks) {
  Entry = RIP;
  auto IRHeader = _IRHeader(InvalidNode, 0);
  Current_Header = IRHeader.first;
  Current_HeaderNode = IRHeader;
  CreateJumpBlocks(Blocks);

  auto Block = GetNewJumpBlock(RIP);
  SetCurrentCodeBlock(Block);
  IRHeader.first->Blocks = Block->Wrapped(DualListData.ListBegin());
}

void OpDispatchBuilder::Finalize() {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // Node 0 is invalid node
  OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(GetNode(1));
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  FEXCore::IR::IROp_Header *IROp =
#endif
  RealNode->Op(DualListData.DataBegin());
  LOGMAN_THROW_A(IROp->Op == OP_IRHEADER, "First op in function must be our header");

  // Let's walk the jump blocks and see if we have handled every block target
  for (auto &Handler : JumpTargets) {
    if (Handler.second.HaveEmitted) continue;

    // We haven't emitted. Dump out to the dispatcher
    SetCurrentCodeBlock(Handler.second.BlockEntry);
    _ExitFunction(_EntrypointOffset(Handler.first - Entry, GPRSize));
  }
}

uint8_t OpDispatchBuilder::GetDstSize(FEXCore::X86Tables::DecodedOp Op) const {
  static constexpr std::array<uint8_t, 8> Sizes = {
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
  LOGMAN_THROW_A(Size != 0, "Invalid destination size for op");
  return Size;
}

uint8_t OpDispatchBuilder::GetSrcSize(FEXCore::X86Tables::DecodedOp Op) const {
  static constexpr std::array<uint8_t, 8> Sizes = {
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
  LOGMAN_THROW_A(Size != 0, "Invalid destination size for op");
  return Size;
}

OrderedNode *OpDispatchBuilder::AppendSegmentOffset(OrderedNode *Value, uint32_t Flags, uint32_t DefaultPrefix, bool Override) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  if (CTX->Config.Is64BitMode) {
    if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
      Value = _Add(Value, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, fs), GPRClass));
    }
    else if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
      Value = _Add(Value, _LoadContext(GPRSize, offsetof(FEXCore::Core::CPUState, gs), GPRClass));
    }
    // If there was any other segment in 64bit then it is ignored
  }
  else {
    OrderedNode *Segment{};
    uint32_t Prefix = Flags & FEXCore::X86Tables::DecodeFlags::FLAG_SEGMENTS;
    if (!Prefix || Override) {
      // If there was no prefix then use the default one if available
      // Or the argument only uses a specific prefix (with override set)
      Prefix = DefaultPrefix;
    }
    switch (Prefix) {
      case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, es), GPRClass);
        break;
      case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, cs), GPRClass);
        break;
      case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, ss), GPRClass);
        break;
      case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, ds), GPRClass);
        break;
      case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, fs), GPRClass);
        break;
      case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
        Segment = _LoadContext(2, offsetof(FEXCore::Core::CPUState, gs), GPRClass);
        break;
      default: break; // Do nothing
    }

    if (Segment) {
      Segment = _Lshr(Segment, ConstructConst(3));
      auto data = _LoadContextIndexed(Segment, 4, offsetof(FEXCore::Core::CPUState, gdt[0]), 4, GPRClass);
      Value = _Add(Value, data);
    }
  }

  return Value;
}

OrderedNode *OpDispatchBuilder::LoadSource_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, int8_t Align, bool LoadData, bool ForceLoad) {
  LOGMAN_THROW_A(Operand.IsGPR() ||
                 Operand.IsLiteral() ||
                 Operand.IsGPRDirect() ||
                 Operand.IsGPRIndirect() ||
                 Operand.IsRIPRelative() ||
                 Operand.IsSIB(),
                 "Unsupported Src type");

  OrderedNode *Src {nullptr};
  bool LoadableType = false;
  bool StackAccess = false;
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint32_t AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  if (Operand.IsLiteral()) {
    uint64_t constant = Operand.Data.Literal.Value;
    uint64_t width = Operand.Data.Literal.Size * 8;

    if (Operand.Data.Literal.Size != 8) {
      // zero extend
      constant = constant & ((1ULL << width) - 1);
    }
    Src = ConstructConst(width, constant);
  }
  else if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    if (gpr >= FEXCore::X86State::REG_MM_0) {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, mm[gpr - FEXCore::X86State::REG_MM_0]), FPRClass);
    }
    else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, xmm[gpr - FEXCore::X86State::REG_XMM_0][Operand.Data.GPR.HighBits ? 1 : 0]), FPRClass);
    }
    else {
      Src = _LoadContext(OpSize, offsetof(FEXCore::Core::CPUState, gregs[gpr]) + (Operand.Data.GPR.HighBits ? 1 : 0), GPRClass);
    }
  }
  else if (Operand.IsGPRDirect()) {
    Src = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.GPR.GPR]), GPRClass);
    LoadableType = true;
    StackAccess = Operand.Data.GPR.GPR == FEXCore::X86State::REG_RSP;
  }
  else if (Operand.IsGPRIndirect()) {
    auto GPR = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.GPRIndirect.GPR]), GPRClass);
    auto Constant = ConstructConst(GPRSize * 8, Operand.Data.GPRIndirect.Displacement);

		Src = _Add(GPR, Constant);

    LoadableType = true;
    StackAccess = Operand.Data.GPRIndirect.GPR == FEXCore::X86State::REG_RSP;
  }
  else if (Operand.IsRIPRelative()) {
    if (CTX->Config.Is64BitMode) {
      Src = GetDynamicPC(Op, Operand.Data.RIPLiteral.Value.s);
    }
    else {
      // 32bit this isn't RIP relative but instead absolute
      Src = ConstructConst(GPRSize * 8, Operand.Data.RIPLiteral.Value.u);
    }

    LoadableType = true;
  }
  else if (Operand.IsSIB()) {
    OrderedNode *Tmp {};
    if (Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(AddrSize , offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.SIB.Index]), GPRClass);

      if (Operand.Data.SIB.Scale != 1) {
        auto Constant = ConstructConst(GPRSize * 8, Operand.Data.SIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
      StackAccess |= Operand.Data.SIB.Index == FEXCore::X86State::REG_RSP;
    }

    if (Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.SIB.Base]), GPRClass);

      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
      StackAccess |= Operand.Data.SIB.Base == FEXCore::X86State::REG_RSP;
    }

    if (Operand.Data.SIB.Offset) {
      if (Tmp != nullptr) {
        Src = _Add(Tmp, ConstructConst(GPRSize * 8, Operand.Data.SIB.Offset));
      }
      else {
        Src = ConstructConst(GPRSize * 8, Operand.Data.SIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        Src = Tmp;
      }
      else {
        Src = ConstructConst(GPRSize * 8, 0);
      }
    }

    if (AddrSize < GPRSize) {
      // If AddrSize == 16 then we need to clear the upper bits
      // GPRSize will be 32 in this case
      Src = _Bfe(AddrSize, AddrSize * 8, 0, Src);
    }

    LoadableType = true;
  }
  else {
    LOGMAN_MSG_A("Unknown Src Type: %d\n", Operand.Type);
  }

  if ((LoadableType && LoadData) || ForceLoad) {
    Src = AppendSegmentOffset(Src, Flags);

    if (StackAccess) {
      Src = _LoadMem(Class, OpSize, Src, Align == -1 ? OpSize : Align);
    }
    else {
      Src = _LoadMemAutoTSO(Class, OpSize, Src, Align == -1 ? OpSize : Align);
    }
  }
  return Src;
}

OrderedNode *OpDispatchBuilder::GetDynamicPC(FEXCore::X86Tables::DecodedOp const& Op, int64_t Offset) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  return _EntrypointOffset(Op->PC + Op->InstSize + Offset - Entry, GPRSize);
}

OrderedNode *OpDispatchBuilder::LoadSource(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint32_t Flags, int8_t Align, bool LoadData, bool ForceLoad) {
  const uint8_t OpSize = GetSrcSize(Op);
  return LoadSource_WithOpSize(Class, Op, Operand, OpSize, Flags, Align, LoadData, ForceLoad);
}

void OpDispatchBuilder::StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align) {
  LOGMAN_THROW_A((Operand.IsGPR() ||
          Operand.IsLiteral() ||
          Operand.IsGPRDirect() ||
          Operand.IsGPRIndirect() ||
          Operand.IsRIPRelative() ||
          Operand.IsSIB()
        ), "Unsupported Dest type");

  // 8Bit and 16bit destination types store their result without effecting the upper bits
  // 32bit ops ZEXT the result to 64bit
  OrderedNode *MemStoreDst {nullptr};
  bool MemStore = false;
  bool StackAccess = false;
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint32_t AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  if (Operand.IsLiteral()) {
    MemStoreDst = ConstructConst(Operand.Data.Literal.Size * 8, Operand.Data.Literal.Value);
    MemStore = true; // Literals are ONLY hardcoded memory destinations
  }
  else if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    if (gpr >= FEXCore::X86State::REG_MM_0) {
      _StoreContext(Class, OpSize, offsetof(FEXCore::Core::CPUState, mm[gpr - FEXCore::X86State::REG_MM_0]), Src);
    }
    else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      _StoreContext(Class, OpSize, offsetof(FEXCore::Core::CPUState, xmm[gpr - FEXCore::X86State::REG_XMM_0][Operand.Data.GPR.HighBits ? 1 : 0]), Src);
    }
    else {
      if (GPRSize == 8 && OpSize == 4) {
        // If the Source IR op is 64 bits, we need to zext the upper bits
        // For all other sizes, the upper bits are guaranteed to already be zero
         OrderedNode *Value = GetOpSize(Src) == 8 ? _Bfe(4, 32, 0, Src) : Src;

        LOGMAN_THROW_A(!Operand.Data.GPR.HighBits, "Can't handle 32bit store to high 8bit register");
        _StoreContext(Class, GPRSize, offsetof(FEXCore::Core::CPUState, gregs[gpr]), Value);
      }
      else {
        LOGMAN_THROW_A(!(GPRSize == 4 && OpSize > 4), "Oops had a %d GPR load", OpSize);
        _StoreContext(Class, std::min(GPRSize, OpSize), offsetof(FEXCore::Core::CPUState, gregs[gpr]) + (Operand.Data.GPR.HighBits ? 1 : 0), Src);
      }
    }
  }
  else if (Operand.IsGPRDirect()) {
    MemStoreDst = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.GPR.GPR]), GPRClass);
    MemStore = true;
    StackAccess = Operand.Data.GPR.GPR == FEXCore::X86State::REG_RSP;
  }
  else if (Operand.IsGPRIndirect()) {
    auto GPR = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.GPRIndirect.GPR]), GPRClass);
    auto Constant = ConstructConst(GPRSize * 8, Operand.Data.GPRIndirect.Displacement);

    MemStoreDst = _Add(GPR, Constant);
    MemStore = true;
    StackAccess = Operand.Data.GPRIndirect.GPR == FEXCore::X86State::REG_RSP;
  }
  else if (Operand.IsRIPRelative()) {
    if (CTX->Config.Is64BitMode) {
      MemStoreDst = GetDynamicPC(Op, Operand.Data.RIPLiteral.Value.s);
    }
    else {
      // 32bit this isn't RIP relative but instead absolute
      MemStoreDst = ConstructConst(GPRSize * 8, Operand.Data.RIPLiteral.Value.u);
    }
    MemStore = true;
  }
  else if (Operand.IsSIB()) {
    OrderedNode *Tmp {};
    if (Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID) {
      Tmp = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.SIB.Index]), GPRClass);

      if (Operand.Data.SIB.Scale != 1) {
        auto Constant = ConstructConst(GPRSize * 8, Operand.Data.SIB.Scale);
        Tmp = _Mul(Tmp, Constant);
      }
    }

    if (Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
      auto GPR = _LoadContext(AddrSize, offsetof(FEXCore::Core::CPUState, gregs[Operand.Data.SIB.Base]), GPRClass);

      if (Tmp != nullptr) {
        Tmp = _Add(Tmp, GPR);
      }
      else {
        Tmp = GPR;
      }
    }

    if (Operand.Data.SIB.Offset) {
      if (Tmp != nullptr) {
        MemStoreDst = _Add(Tmp, ConstructConst(GPRSize * 8, Operand.Data.SIB.Offset));
      }
      else {
        MemStoreDst = ConstructConst(GPRSize * 8, Operand.Data.SIB.Offset);
      }
    }
    else {
      if (Tmp != nullptr) {
        MemStoreDst = Tmp;
      }
      else {
        MemStoreDst = ConstructConst(GPRSize * 8, 0);
      }
    }

    if (AddrSize < GPRSize) {
      // If AddrSize == 16 then we need to clear the upper bits
      // GPRSize will be 32 in this case
      MemStoreDst = _Bfe(AddrSize, AddrSize * 8, 0, MemStoreDst);
    }

    MemStore = true;
  }

  if (MemStore) {
    MemStoreDst = AppendSegmentOffset(MemStoreDst, Op->Flags);

    if (OpSize == 10) {
      // For X87 extended doubles, split before storing
      _StoreMem(FPRClass, 8, MemStoreDst, Src, Align);
      auto Upper = _VExtractToGPR(16, 8, Src, 1);
      auto DestAddr = _Add(MemStoreDst, ConstructConst(8));
      _StoreMem(GPRClass, 2, DestAddr, Upper, std::min<uint8_t>(Align, 8));
    } else {
      if (StackAccess) {
        _StoreMem(Class, OpSize, MemStoreDst, Src, Align == -1 ? OpSize : Align);
      }
      else {
        _StoreMemAutoTSO(Class, OpSize, MemStoreDst, Src, Align == -1 ? OpSize : Align);
      }
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

void OpDispatchBuilder::UnhandledOp(OpcodeArgs) {
  DecodeFailure = true;
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::MOVGPROp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, 1);
  StoreResult(GPRClass, Op, Src, 1);
}

void OpDispatchBuilder::ALUOp(OpcodeArgs) {
  bool RequiresMask = false;
  FEXCore::IR::IROps IROp;
  switch (Op->OP) {
  case 0x0:
  case 0x1:
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
    IROp = FEXCore::IR::IROps::OP_ADD;
    RequiresMask = true;
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
    RequiresMask = true;
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
    LOGMAN_MSG_A("Unknown ALU Op: 0x%x", Op->OP);
  break;
  }

  auto Size = GetDstSize(Op);

  // X86 basic ALU ops just do the operation between the destination and a single source
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Result{};
  OrderedNode *Dest{};

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    switch (IROp) {
      case FEXCore::IR::IROps::OP_ADD: {
        Dest = _AtomicFetchAdd(DestMem, Src, Size);
        Result = _Add(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_SUB: {
        Dest = _AtomicFetchSub(DestMem, Src, Size);
        Result = _Sub(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_OR: {
        Dest = _AtomicFetchOr(DestMem, Src, Size);
        Result = _Or(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_AND: {
        Dest = _AtomicFetchAnd(DestMem, Src, Size);
        Result = _And(Dest, Src);
        break;
      }
      case FEXCore::IR::IROps::OP_XOR: {
        Dest = _AtomicFetchXor(DestMem, Src, Size);
        Result = _Xor(Dest, Src);
        break;
      }
      default: LOGMAN_MSG_A("Unknown Atomic IR Op: %d", IROp); break;
    }
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

    auto ALUOp = _Add(Dest, Src);
    // Overwrite our IR's op type
    ALUOp.first->Header.Op = IROp;

    Result = ALUOp;
    StoreResult(GPRClass, Op, Result, -1);
  }

  if (RequiresMask && Size < 4) {
    Result = _Bfe(Size, Size * 8, 0, Result);
  }

  // Flags set
  {
    switch (IROp) {
    case FEXCore::IR::IROps::OP_ADD:
      GenerateFlags_ADD(Op, Result, Dest, Src);
    break;
    case FEXCore::IR::IROps::OP_SUB:
      GenerateFlags_SUB(Op, Result, Dest, Src);
    break;
    case FEXCore::IR::IROps::OP_AND:
    case FEXCore::IR::IROps::OP_XOR:
    case FEXCore::IR::IROps::OP_OR: {
      GenerateFlags_Logical(Op, Result, Dest, Src);
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
    Literal = Op->Src[0].Data.Literal.Value;
    if (Literal == 0x80) {
      // Syscall on linux
      SyscallOp(Op);
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
  break;
  case 0xCC:
    Reason = 6;
    setRIP = true;
  break;
  }

  if (setRIP) {
    const uint8_t GPRSize = CTX->GetGPRSize();

    BlockSetRIP = setRIP;

    // We want to set RIP to the next instruction after HLT/INT3
    auto NewRIP = GetDynamicPC(Op);
    _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), NewRIP);
  }

  if (Op->OP == 0xCE) { // Conditional to only break if Overflow == 1
    auto Flag = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    // If condition doesn't hold then keep going
    auto CondJump = _CondJump(Flag, {COND_EQ});
    auto FalseBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetFalseJumpTarget(CondJump, FalseBlock);
    SetCurrentCodeBlock(FalseBlock);

    _Break(Reason, Literal);

    // Make sure to start a new block after ending this one
    auto JumpTarget = CreateNewCodeBlockAfter(FalseBlock);
    SetTrueJumpTarget(CondJump, JumpTarget);
    SetCurrentCodeBlock(JumpTarget);
  }
  else {
    _Break(Reason, Literal);
  }
}

void OpDispatchBuilder::TZCNT(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  Src = _FindTrailingZeros(Src);
  StoreResult(GPRClass, Op, Src, -1);

  auto Zero = ConstructConst(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      ConstructConst(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Bfe(1, 0, Src));
}

void OpDispatchBuilder::LZCNT(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Res = _CountLeadingZeroes(Src);
  StoreResult(GPRClass, Op, Res, -1);

  auto Zero = ConstructConst(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      ConstructConst(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Bfe(1, GetSrcSize(Op) * 8 - 1, Src));
}

void OpDispatchBuilder::MOVBEOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, 1);
  Src = _Rev(Src);
  StoreResult(GPRClass, Op, Src, 1);
}

template<uint8_t FenceType>
void OpDispatchBuilder::FenceOp(OpcodeArgs) {
  _Fence({FenceType});
}

void OpDispatchBuilder::StoreFenceOrCLFlush(OpcodeArgs) {
  if (Op->ModRM == 0xF8) {
    // 0xF8 is SFENCE
    _Fence({FEXCore::IR::Fence_Store});
  }
  else {
    // This is a CLFlush
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    _CacheLineClear(DestMem);
  }
}

void OpDispatchBuilder::UnimplementedOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // We don't actually support this instruction
  // Multiblock may hit it though
  _StoreContext(GPRClass, GPRSize, offsetof(FEXCore::Core::CPUState, rip), GetDynamicPC(Op, -Op->InstSize));
  _Break(0, 0);
  BlockSetRIP = true;

  auto NextBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
  SetCurrentCodeBlock(NextBlock);
}

#undef OpcodeArgs

void InstallOpcodeHandlers(Context::OperatingMode Mode) {
  const std::vector<std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr>> BaseOpTable = {
    // Instructions
    {0x00, 6, &OpDispatchBuilder::ALUOp},

    {0x08, 6, &OpDispatchBuilder::ALUOp},

    {0x10, 6, &OpDispatchBuilder::ADCOp<0>},

    {0x18, 6, &OpDispatchBuilder::SBBOp<0>},

    {0x20, 6, &OpDispatchBuilder::ALUOp},

    {0x28, 6, &OpDispatchBuilder::ALUOp},

    {0x30, 6, &OpDispatchBuilder::ALUOp},

    {0x38, 6, &OpDispatchBuilder::CMPOp<0>},
    {0x50, 8, &OpDispatchBuilder::PUSHREGOp},
    {0x58, 8, &OpDispatchBuilder::POPOp},
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
    {0xCF, 1, &OpDispatchBuilder::IRETOp},
    {0xD7, 2, &OpDispatchBuilder::XLATOp},
    {0xE0, 3, &OpDispatchBuilder::LoopOp},
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
    {0x06, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX>},
    {0x07, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX>},
    {0x0E, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX>},
    {0x16, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX>},
    {0x17, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX>},
    {0x1E, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX>},
    {0x1F, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX>},
    {0x40, 8, &OpDispatchBuilder::INCOp},
    {0x48, 8, &OpDispatchBuilder::DECOp},

    {0x60, 1, &OpDispatchBuilder::PUSHAOp},
    {0x61, 1, &OpDispatchBuilder::POPAOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr>> BaseOpTable_64 = {
    {0x63, 1, &OpDispatchBuilder::MOVSXDOp},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> TwoByteOpTable = {
    // Instructions
    {0x0B, 1, &OpDispatchBuilder::INTOp},
    {0x0E, 1, &OpDispatchBuilder::X87EMMS},

    {0x19, 7, &OpDispatchBuilder::NOPOp}, // NOP with ModRM

    {0x31, 1, &OpDispatchBuilder::RDTSCOp},

    {0x3F, 1, &OpDispatchBuilder::ThunkOp},
    {0x40, 16, &OpDispatchBuilder::CMOVOp},
    {0x6E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x80, 16, &OpDispatchBuilder::CondJUMPOp},
    {0x90, 16, &OpDispatchBuilder::SETccOp},
    {0xA0, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX>},
    {0xA1, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX>},
    {0xA2, 1, &OpDispatchBuilder::CPUIDOp},
    {0xA3, 1, &OpDispatchBuilder::BTOp<0>}, // BT
    {0xA4, 1, &OpDispatchBuilder::SHLDImmediateOp},
    {0xA5, 1, &OpDispatchBuilder::SHLDOp},
    {0xA8, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX>},
    {0xA9, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX>},
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
    {0x2A, 1, &OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float<4, false>},
    {0x2B, 1, &OpDispatchBuilder::MOVAPSOp},
    {0x2C, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>},
    {0x2D, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, true>},
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
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 4>},
    {0x5D, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 4>},
    {0x5E, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 4>},
    {0x5F, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 4>},
    {0x60, 1, &OpDispatchBuilder::PUNPCKLOp<1>},
    {0x61, 1, &OpDispatchBuilder::PUNPCKLOp<2>},
    {0x62, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x63, 1, &OpDispatchBuilder::PACKSSOp<2>},
    {0x64, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 1>},
    {0x65, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 2>},
    {0x66, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 4>},
    {0x67, 1, &OpDispatchBuilder::PACKUSOp<2>},
    {0x68, 1, &OpDispatchBuilder::PUNPCKHOp<1>},
    {0x69, 1, &OpDispatchBuilder::PUNPCKHOp<2>},
    {0x6A, 1, &OpDispatchBuilder::PUNPCKHOp<4>},
    {0x6B, 1, &OpDispatchBuilder::PACKSSOp<4>},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, false, true>},

    {0x74, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 1>},
    {0x75, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 2>},
    {0x76, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 4>},
    {0x77, 1, &OpDispatchBuilder::X87EMMS},

    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<4, false>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<4>},

    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8>},
    {0xD4, 1, &OpDispatchBuilder::PADDQOp<8>},
    {0xD5, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMUL, 2>},
    {0xD7, 1, &OpDispatchBuilder::MOVMSKOpOne}, // PMOVMSKB
    {0xD8, 1, &OpDispatchBuilder::PSUBSOp<1, false>},
    {0xD9, 1, &OpDispatchBuilder::PSUBSOp<2, false>},
    {0xDA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 8>},
    {0xDC, 1, &OpDispatchBuilder::PADDSOp<1, false>},
    {0xDD, 1, &OpDispatchBuilder::PADDSOp<2, false>},
    {0xDE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 1>},
    {0xDF, 1, &OpDispatchBuilder::ANDNOp},
    {0xE0, 1, &OpDispatchBuilder::PAVGOp<1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4>},
    {0xE3, 1, &OpDispatchBuilder::PAVGOp<2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE7, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xE8, 1, &OpDispatchBuilder::PSUBSOp<1, true>},
    {0xE9, 1, &OpDispatchBuilder::PSUBSOp<2, true>},
    {0xEA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 2>},
    {0xEB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 8>},
    {0xEC, 1, &OpDispatchBuilder::PADDSOp<1, true>},
    {0xED, 1, &OpDispatchBuilder::PADDSOp<2, true>},
    {0xEE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 2>},
    {0xEF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 8>},

    {0xF1, 1, &OpDispatchBuilder::PSLL<2>},
    {0xF2, 1, &OpDispatchBuilder::PSLL<4>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8>},
    {0xF4, 1, &OpDispatchBuilder::PMULLOp<4, false>},
    {0xF5, 1, &OpDispatchBuilder::PMADDWD},
    {0xF6, 1, &OpDispatchBuilder::PSADBW},
    {0xF7, 1, &OpDispatchBuilder::MASKMOVOp},
    {0xF8, 1, &OpDispatchBuilder::PSUBQOp<1>},
    {0xF9, 1, &OpDispatchBuilder::PSUBQOp<2>},
    {0xFA, 1, &OpDispatchBuilder::PSUBQOp<4>},
    {0xFB, 1, &OpDispatchBuilder::PSUBQOp<8>},
    {0xFC, 1, &OpDispatchBuilder::PADDQOp<1>},
    {0xFD, 1, &OpDispatchBuilder::PADDQOp<2>},
    {0xFE, 1, &OpDispatchBuilder::PADDQOp<4>},

    // FEX reserved instructions
    {0x36, 1, &OpDispatchBuilder::SIGRETOp},
    {0x37, 1, &OpDispatchBuilder::CallbackReturnOp},
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
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 3), 1, &OpDispatchBuilder::SBBOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 7), 1, &OpDispatchBuilder::CMPOp<1>}, // CMP

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 3), 1, &OpDispatchBuilder::SBBOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 7), 1, &OpDispatchBuilder::CMPOp<1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 2), 1, &OpDispatchBuilder::ADCOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 3), 1, &OpDispatchBuilder::SBBOp<1>}, // Unit tests find this setting flags incorrectly
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 7), 1, &OpDispatchBuilder::CMPOp<1>},

    // GROUP 2
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, &OpDispatchBuilder::ROLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, &OpDispatchBuilder::RORImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 6), 1, &OpDispatchBuilder::SHLImmediateOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, &OpDispatchBuilder::ROLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, &OpDispatchBuilder::RORImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 6), 1, &OpDispatchBuilder::SHLImmediateOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, &OpDispatchBuilder::ROLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, &OpDispatchBuilder::ROROp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 3), 1, &OpDispatchBuilder::RCROp8x1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 6), 1, &OpDispatchBuilder::SHLOp<true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, &OpDispatchBuilder::ROLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, &OpDispatchBuilder::ROROp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 3), 1, &OpDispatchBuilder::RCROp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 6), 1, &OpDispatchBuilder::SHLOp<true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, &OpDispatchBuilder::ROLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, &OpDispatchBuilder::ROROp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 2), 1, &OpDispatchBuilder::RCLSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 3), 1, &OpDispatchBuilder::RCRSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, &OpDispatchBuilder::SHLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, &OpDispatchBuilder::SHROp<false>}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 6), 1, &OpDispatchBuilder::SHLOp<false>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, &OpDispatchBuilder::ASHROp<false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, &OpDispatchBuilder::ROLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, &OpDispatchBuilder::ROROp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, &OpDispatchBuilder::SHLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, &OpDispatchBuilder::SHROp<false>}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 6), 1, &OpDispatchBuilder::SHLOp<false>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, &OpDispatchBuilder::ASHROp<false>}, // SAR

    // GROUP 3
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, &OpDispatchBuilder::TESTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 1), 1, &OpDispatchBuilder::TESTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, &OpDispatchBuilder::MULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, &OpDispatchBuilder::DIVOp}, // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, &OpDispatchBuilder::TESTOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 1), 1, &OpDispatchBuilder::TESTOp<1>},
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
    {0x2A, 1, &OpDispatchBuilder::CVTGPR_To_FPR<4>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, false>},
    {0x2D, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4, true>},
    {0x52, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4, true>},
    {0x53, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4, true>},
    {0x58, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFADD, 4>},
    {0x59, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMUL, 4>},
    {0x5A, 1, &OpDispatchBuilder::Scalar_CVT_Float_To_Float<8, 4>},
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFSUB, 4>},
    {0x5D, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMIN, 4>},
    {0x5E, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFDIV, 4>},
    {0x5F, 1, &OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMAX, 4>},
    {0x6F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp<2, true, false>},
    {0x7E, 1, &OpDispatchBuilder::MOVQOp},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xB8, 1, &OpDispatchBuilder::PopcountOp},
    {0xBC, 1, &OpDispatchBuilder::TZCNT},
    {0xBD, 1, &OpDispatchBuilder::LZCNT},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<4, true>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true>},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> RepNEModOpTable = {
    {0x10, 2, &OpDispatchBuilder::MOVSDOp},
    {0x12, 1, &OpDispatchBuilder::MOVDDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x2A, 1, &OpDispatchBuilder::CVTGPR_To_FPR<8>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, false>},
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
    {0xD0, 1, &OpDispatchBuilder::ADDSUBPOp<4>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<false>},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<8, true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, true>},
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
    {0x2A, 1, &OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float<4, true>},
    {0x2B, 1, &OpDispatchBuilder::MOVAPSOp},
    {0x2C, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, false>},
    {0x2D, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true>},
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
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, true>},
    {0x5C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 8>},
    {0x5D, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 8>},
    {0x5E, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 8>},
    {0x5F, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 8>},
    {0x60, 1, &OpDispatchBuilder::PUNPCKLOp<1>},
    {0x61, 1, &OpDispatchBuilder::PUNPCKLOp<2>},
    {0x62, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x63, 1, &OpDispatchBuilder::PACKSSOp<2>},
    {0x64, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 1>},
    {0x65, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 2>},
    {0x66, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 4>},
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

    {0x74, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 1>},
    {0x75, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 2>},
    {0x76, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 4>},
    {0x78, 1, nullptr}, // GROUP 17
    {0x7C, 1, &OpDispatchBuilder::HADDP<8>},
    {0x7D, 1, &OpDispatchBuilder::HSUBP<8>},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVUPSOp},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<8, false>},
    {0xC4, 1, &OpDispatchBuilder::PINSROp<2>},
    {0xC5, 1, &OpDispatchBuilder::PExtrOp<2>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<8>},

    {0xD0, 1, &OpDispatchBuilder::ADDSUBPOp<8>},
    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8>},
    {0xD4, 1, &OpDispatchBuilder::PADDQOp<8>},
    {0xD5, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMUL, 2>},
    {0xD6, 1, &OpDispatchBuilder::MOVQOp},
    {0xD7, 1, &OpDispatchBuilder::MOVMSKOpOne}, // PMOVMSKB
    {0xD8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>},
    {0xD9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>},
    {0xDA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0xDC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>},
    {0xDD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>},
    {0xDE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 1>},
    {0xDF, 1, &OpDispatchBuilder::ANDNOp},
    {0xE0, 1, &OpDispatchBuilder::PAVGOp<1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4>},
    {0xE3, 1, &OpDispatchBuilder::PAVGOp<2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, false>},
    {0xE7, 1, &OpDispatchBuilder::MOVVectorOp},
    {0xE8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 1>},
    {0xE9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 2>},
    {0xEA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 2>},
    {0xEB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 16>},
    {0xEC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 1>},
    {0xED, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 2>},
    {0xEE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 2>},
    {0xEF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 16>},

    {0xF1, 1, &OpDispatchBuilder::PSLL<2>},
    {0xF2, 1, &OpDispatchBuilder::PSLL<4>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8>},
    {0xF4, 1, &OpDispatchBuilder::PMULLOp<4, false>},
    {0xF5, 1, &OpDispatchBuilder::PMADDWD},
    {0xF6, 1, &OpDispatchBuilder::PSADBW},
    {0xF7, 1, &OpDispatchBuilder::MASKMOVOp},
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
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 7), 1, &OpDispatchBuilder::StoreFenceOrCLFlush},     //SFENCE

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

    // GROUP P
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F3, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_66, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F2, 0), 8, &OpDispatchBuilder::NOPOp},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> SecondaryModRMExtensionOpTable = {
    // REG /2
    {((1 << 3) | 0), 1, &OpDispatchBuilder::UnimplementedOp},
  };
// Top bit indicating if it needs to be repeated with {0x40, 0x80} or'd in
// All OPDReg versions need it
#define OPDReg(op, reg) ((1 << 15) | ((op - 0xD8) << 8) | (reg << 3))
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> X87OpTable = {
    {OPDReg(0xD8, 0) | 0x00, 8, &OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xD8, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xD8, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xD8, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xD0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xD8, 0xD8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xD8, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD9, 0) | 0x00, 8, &OpDispatchBuilder::FLD<32>},

    // 1 = Invalid

    {OPDReg(0xD9, 2) | 0x00, 8, &OpDispatchBuilder::FST<32>},

    {OPDReg(0xD9, 3) | 0x00, 8, &OpDispatchBuilder::FST<32>},

    {OPDReg(0xD9, 4) | 0x00, 8, &OpDispatchBuilder::X87LDENV},

    {OPDReg(0xD9, 5) | 0x00, 8, &OpDispatchBuilder::X87FLDCW}, // XXX: stubbed FLDCW

    {OPDReg(0xD9, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSTENV},

    {OPDReg(0xD9, 7) | 0x00, 8, &OpDispatchBuilder::X87FSTCW},

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
      {OPD(0xD9, 0xF1), 1, &OpDispatchBuilder::X87FYL2X},
      {OPD(0xD9, 0xF2), 1, &OpDispatchBuilder::X87TAN},
      {OPD(0xD9, 0xF3), 1, &OpDispatchBuilder::X87ATAN},
      {OPD(0xD9, 0xF4), 1, &OpDispatchBuilder::FXTRACT},
      {OPD(0xD9, 0xF5), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM1>},
      {OPD(0xD9, 0xF6), 1, &OpDispatchBuilder::X87ModifySTP<false>},
      {OPD(0xD9, 0xF7), 1, &OpDispatchBuilder::X87ModifySTP<true>},
      {OPD(0xD9, 0xF8), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM>},
      {OPD(0xD9, 0xF9), 1, &OpDispatchBuilder::X87FYL2X},
      {OPD(0xD9, 0xFA), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80SQRT>},
      {OPD(0xD9, 0xFB), 1, &OpDispatchBuilder::X87SinCos},
      {OPD(0xD9, 0xFC), 1, &OpDispatchBuilder::FRNDINT},
      {OPD(0xD9, 0xFD), 1, &OpDispatchBuilder::X87BinaryOp<IR::OP_F80SCALE>},
      {OPD(0xD9, 0xFE), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80SIN>},
      {OPD(0xD9, 0xFF), 1, &OpDispatchBuilder::X87UnaryOp<IR::OP_F80COS>},

    {OPDReg(0xDA, 0) | 0x00, 8, &OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDA, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDA, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

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

    {OPDReg(0xDB, 0) | 0x00, 8, &OpDispatchBuilder::FILD},

    {OPDReg(0xDB, 1) | 0x00, 8, &OpDispatchBuilder::FIST<true>},

    {OPDReg(0xDB, 2) | 0x00, 8, &OpDispatchBuilder::FIST<false>},

    {OPDReg(0xDB, 3) | 0x00, 8, &OpDispatchBuilder::FIST<false>},

    // 4 = Invalid

    {OPDReg(0xDB, 5) | 0x00, 8, &OpDispatchBuilder::FLD<80>},

    // 6 = Invalid

    {OPDReg(0xDB, 7) | 0x00, 8, &OpDispatchBuilder::FST<80>},


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

    {OPDReg(0xDC, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDC, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDC, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDC, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDD, 0) | 0x00, 8, &OpDispatchBuilder::FLD<64>},

    {OPDReg(0xDD, 1) | 0x00, 8, &OpDispatchBuilder::FIST<true>},

    {OPDReg(0xDD, 2) | 0x00, 8, &OpDispatchBuilder::FST<64>},

    {OPDReg(0xDD, 3) | 0x00, 8, &OpDispatchBuilder::FST<64>},

    {OPDReg(0xDD, 4) | 0x00, 8, &OpDispatchBuilder::X87FRSTOR},

    // 5 = Invalid
    {OPDReg(0xDD, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSAVE},

    {OPDReg(0xDD, 7) | 0x00, 8, &OpDispatchBuilder::X87FNSTSW},

      {OPD(0xDD, 0xC0), 8, &OpDispatchBuilder::X87FFREE},
      {OPD(0xDD, 0xD0), 8, &OpDispatchBuilder::FST},
      {OPD(0xDD, 0xD8), 8, &OpDispatchBuilder::FST},

      {OPD(0xDD, 0xE0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xDD, 0xE8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 0) | 0x00, 8, &OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 1) | 0x00, 8, &OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 2) | 0x00, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 3) | 0x00, 8, &OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 4) | 0x00, 8, &OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 5) | 0x00, 8, &OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 6) | 0x00, 8, &OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 7) | 0x00, 8, &OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDE, 0xC0), 8, &OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xC8), 8, &OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xD9), 1, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      {OPD(0xDE, 0xE0), 8, &OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xE8), 8, &OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF0), 8, &OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF8), 8, &OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDF, 0) | 0x00, 8, &OpDispatchBuilder::FILD},

    {OPDReg(0xDF, 1) | 0x00, 8, &OpDispatchBuilder::FIST<true>},

    {OPDReg(0xDF, 2) | 0x00, 8, &OpDispatchBuilder::FIST<false>},

    {OPDReg(0xDF, 3) | 0x00, 8, &OpDispatchBuilder::FIST<false>},

    {OPDReg(0xDF, 4) | 0x00, 8, &OpDispatchBuilder::FBLD},

    {OPDReg(0xDF, 5) | 0x00, 8, &OpDispatchBuilder::FILD},

    {OPDReg(0xDF, 6) | 0x00, 8, &OpDispatchBuilder::FBSTP},

    {OPDReg(0xDF, 7) | 0x00, 8, &OpDispatchBuilder::FIST<false>},

      // XXX: This should also set the x87 tag bits to empty
      // We don't support this currently, so just pop the stack
      {OPD(0xDF, 0xC0), 8, &OpDispatchBuilder::X87ModifySTP<true>},

      {OPD(0xDF, 0xE0), 8, &OpDispatchBuilder::X87FNSTSW},
      {OPD(0xDF, 0xE8), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
      {OPD(0xDF, 0xF0), 8, &OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  };
#undef OPD
#undef OPDReg

#define OPD(prefix, opcode) ((prefix << 8) | opcode)
  constexpr uint16_t PF_38_NONE = 0;
  constexpr uint16_t PF_38_66   = 1;

  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> H0F38Table = {
    {OPD(PF_38_NONE, 0x00), 1, &OpDispatchBuilder::PSHUFBOp},
    {OPD(PF_38_66,   0x00), 1, &OpDispatchBuilder::PSHUFBOp},
    {OPD(PF_38_NONE, 0x01), 1, &OpDispatchBuilder::PHADD<2>},
    {OPD(PF_38_66,   0x01), 1, &OpDispatchBuilder::PHADD<2>},
    {OPD(PF_38_NONE, 0x02), 1, &OpDispatchBuilder::PHADD<4>},
    {OPD(PF_38_66,   0x02), 1, &OpDispatchBuilder::PHADD<4>},
    {OPD(PF_38_NONE, 0x03), 1, &OpDispatchBuilder::PHADDS},
    {OPD(PF_38_66,   0x03), 1, &OpDispatchBuilder::PHADDS},
    {OPD(PF_38_NONE, 0x04), 1, &OpDispatchBuilder::PMADDUBSW},
    {OPD(PF_38_66,   0x04), 1, &OpDispatchBuilder::PMADDUBSW},
    {OPD(PF_38_NONE, 0x05), 1, &OpDispatchBuilder::PHSUB<2>},
    {OPD(PF_38_66,   0x05), 1, &OpDispatchBuilder::PHSUB<2>},
    {OPD(PF_38_NONE, 0x06), 1, &OpDispatchBuilder::PHSUB<4>},
    {OPD(PF_38_66,   0x06), 1, &OpDispatchBuilder::PHSUB<4>},
    {OPD(PF_38_NONE, 0x07), 1, &OpDispatchBuilder::PHSUBS},
    {OPD(PF_38_66,   0x07), 1, &OpDispatchBuilder::PHSUBS},
    {OPD(PF_38_NONE, 0x08), 1, &OpDispatchBuilder::PSIGN<1>},
    {OPD(PF_38_66,   0x08), 1, &OpDispatchBuilder::PSIGN<1>},
    {OPD(PF_38_NONE, 0x09), 1, &OpDispatchBuilder::PSIGN<2>},
    {OPD(PF_38_66,   0x09), 1, &OpDispatchBuilder::PSIGN<2>},
    {OPD(PF_38_NONE, 0x0A), 1, &OpDispatchBuilder::PSIGN<4>},
    {OPD(PF_38_66,   0x0A), 1, &OpDispatchBuilder::PSIGN<4>},
    {OPD(PF_38_NONE, 0x0B), 1, &OpDispatchBuilder::PMULHRSW},
    {OPD(PF_38_66,   0x0B), 1, &OpDispatchBuilder::PMULHRSW},
    {OPD(PF_38_66,   0x10), 1, &OpDispatchBuilder::VectorVariableBlend<1>},
    {OPD(PF_38_66,   0x14), 1, &OpDispatchBuilder::VectorVariableBlend<4>},
    {OPD(PF_38_66,   0x15), 1, &OpDispatchBuilder::VectorVariableBlend<8>},
    {OPD(PF_38_66,   0x17), 1, &OpDispatchBuilder::PTestOp},
    {OPD(PF_38_NONE, 0x1C), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1, false>},
    {OPD(PF_38_66,   0x1C), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1, false>},
    {OPD(PF_38_NONE, 0x1D), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2, false>},
    {OPD(PF_38_66,   0x1D), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2, false>},
    {OPD(PF_38_NONE, 0x1E), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4, false>},
    {OPD(PF_38_66,   0x1E), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4, false>},
    {OPD(PF_38_66,   0x20), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, true>},
    {OPD(PF_38_66,   0x21), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, true>},
    {OPD(PF_38_66,   0x22), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, true>},
    {OPD(PF_38_66,   0x23), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, true>},
    {OPD(PF_38_66,   0x24), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, true>},
    {OPD(PF_38_66,   0x25), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, true>},
    {OPD(PF_38_66,   0x28), 1, &OpDispatchBuilder::PMULLOp<4, true>},
    {OPD(PF_38_66,   0x29), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 8>},
    {OPD(PF_38_66,   0x2A), 1, &OpDispatchBuilder::MOVAPSOp},
    {OPD(PF_38_66,   0x2B), 1, &OpDispatchBuilder::PACKUSOp<4>},
    {OPD(PF_38_66,   0x30), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, false>},
    {OPD(PF_38_66,   0x31), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, false>},
    {OPD(PF_38_66,   0x32), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, false>},
    {OPD(PF_38_66,   0x33), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, false>},
    {OPD(PF_38_66,   0x34), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, false>},
    {OPD(PF_38_66,   0x35), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, false>},
    {OPD(PF_38_66,   0x38), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 1>},
    {OPD(PF_38_66,   0x39), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 4>},
    {OPD(PF_38_66,   0x3A), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 2>},
    {OPD(PF_38_66,   0x3B), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 4>},
    {OPD(PF_38_66,   0x3C), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 1>},
    {OPD(PF_38_66,   0x3D), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 4>},
    {OPD(PF_38_66,   0x3E), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 2>},
    {OPD(PF_38_66,   0x3F), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 4>},
    {OPD(PF_38_66,   0x40), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMUL, 4>},
    {OPD(PF_38_66,   0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},

    {OPD(PF_38_66, 0xDB), 1, &OpDispatchBuilder::AESImcOp},
    {OPD(PF_38_66, 0xDC), 1, &OpDispatchBuilder::AESEncOp},
    {OPD(PF_38_66, 0xDD), 1, &OpDispatchBuilder::AESEncLastOp},
    {OPD(PF_38_66, 0xDE), 1, &OpDispatchBuilder::AESDecOp},
    {OPD(PF_38_66, 0xDF), 1, &OpDispatchBuilder::AESDecLastOp},

    {OPD(PF_38_NONE, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},
    {OPD(PF_38_66, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},

  };
#undef OPD

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66   1
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> H0F3ATable = {
    {OPD(0, PF_3A_66,   0x08), 1, &OpDispatchBuilder::VectorRound<4, false>},
    {OPD(0, PF_3A_66,   0x09), 1, &OpDispatchBuilder::VectorRound<8, false>},
    {OPD(0, PF_3A_66,   0x0A), 1, &OpDispatchBuilder::VectorRound<4, true>},
    {OPD(0, PF_3A_66,   0x0B), 1, &OpDispatchBuilder::VectorRound<8, true>},
    {OPD(0, PF_3A_66,   0x0C), 1, &OpDispatchBuilder::VectorBlend<4>},
    {OPD(0, PF_3A_66,   0x0D), 1, &OpDispatchBuilder::VectorBlend<8>},
    {OPD(0, PF_3A_66,   0x0E), 1, &OpDispatchBuilder::VectorBlend<2>},

    {OPD(0, PF_3A_NONE, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},
    {OPD(0, PF_3A_66,   0x0F), 1, &OpDispatchBuilder::PAlignrOp},
    {OPD(1, PF_3A_66,   0x0F), 1, &OpDispatchBuilder::PAlignrOp},

    {OPD(0, PF_3A_66,   0x14), 1, &OpDispatchBuilder::PExtrOp<1>},
    {OPD(0, PF_3A_66,   0x15), 1, &OpDispatchBuilder::PExtrOp<2>},
    {OPD(0, PF_3A_66,   0x16), 1, &OpDispatchBuilder::PExtrOp<4>},
    {OPD(1, PF_3A_66,   0x16), 1, &OpDispatchBuilder::PExtrOp<8>},
    {OPD(0, PF_3A_66,   0x17), 1, &OpDispatchBuilder::PExtrOp<4>},

    {OPD(0, PF_3A_66,   0x20), 1, &OpDispatchBuilder::PINSROp<1>},
    {OPD(0, PF_3A_66,   0x21), 1, &OpDispatchBuilder::InsertPSOp},
    {OPD(0, PF_3A_66,   0x22), 1, &OpDispatchBuilder::PINSROp<4>},
    {OPD(1, PF_3A_66,   0x22), 1, &OpDispatchBuilder::PINSROp<8>},
    {OPD(0, PF_3A_66,   0x40), 1, &OpDispatchBuilder::DPPOp<4>},
    {OPD(0, PF_3A_66,   0x41), 1, &OpDispatchBuilder::DPPOp<8>},
    {OPD(0, PF_3A_66,   0x42), 1, &OpDispatchBuilder::MPSADBWOp},

    {OPD(0, PF_3A_66,   0xDF), 1, &OpDispatchBuilder::AESKeyGenAssist},
  };
#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  const std::vector<std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> VEXTable = {
    {OPD(1, 0b01, 0x6E), 2, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x74), 3, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x58), 3, &OpDispatchBuilder::UnimplementedOp},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::UnimplementedOp},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr>> EVEXTable = {
    {0x10, 2, &OpDispatchBuilder::UnimplementedOp},
    {0x59, 1, &OpDispatchBuilder::UnimplementedOp},
    {0x7F, 1, &OpDispatchBuilder::UnimplementedOp},
  };

  auto InstallToTable = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LOGMAN_THROW_A(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
        FinalTable[OpNum + i].OpcodeDispatcher = Dispatcher;
      }
    }
  };
  auto InstallToX87Table = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      bool Repeat = (OpNum & 0x8000) != 0;
      OpNum = OpNum & 0x7FF;
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LOGMAN_THROW_A(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
        FinalTable[OpNum + i].OpcodeDispatcher = Dispatcher;

        // Flag to indicate if we need to repeat this op in {0x40, 0x80} ranges
        if (Repeat) {
          FinalTable[(OpNum | 0x40) + i].OpcodeDispatcher = Dispatcher;
          FinalTable[(OpNum | 0x80) + i].OpcodeDispatcher = Dispatcher;
        }
      }
    }
  };

  InstallToTable(FEXCore::X86Tables::BaseOps, BaseOpTable);
  if (Mode == Context::MODE_32BIT) {
    InstallToTable(FEXCore::X86Tables::BaseOps, BaseOpTable_32);
    InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable_32);
  }
  else {
    InstallToTable(FEXCore::X86Tables::BaseOps, BaseOpTable_64);
    InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable_64);
  }

  InstallToTable(FEXCore::X86Tables::SecondBaseOps, TwoByteOpTable);
  InstallToTable(FEXCore::X86Tables::PrimaryInstGroupOps, PrimaryGroupOpTable);

  InstallToTable(FEXCore::X86Tables::RepModOps, RepModOpTable);
  InstallToTable(FEXCore::X86Tables::RepNEModOps, RepNEModOpTable);
  InstallToTable(FEXCore::X86Tables::OpSizeModOps, OpSizeModOpTable);
  InstallToTable(FEXCore::X86Tables::SecondInstGroupOps, SecondaryExtensionOpTable);

  InstallToTable(FEXCore::X86Tables::SecondModRMTableOps, SecondaryModRMExtensionOpTable);

  InstallToX87Table(FEXCore::X86Tables::X87Ops, X87OpTable);

  InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38Table);
  InstallToTable(FEXCore::X86Tables::H0F3ATableOps, H0F3ATable);
  InstallToTable(FEXCore::X86Tables::VEXTableOps, VEXTable);
  InstallToTable(FEXCore::X86Tables::EVEXTableOps, EVEXTable);
}

}
