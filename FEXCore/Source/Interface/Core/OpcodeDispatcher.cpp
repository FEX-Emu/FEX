// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 ops to IR, no-pf opt, local-flags opt
$end_info$
*/

#include "FEXCore/Core/HostFeatures.h"
#include "FEXCore/Utils/Telemetry.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>

#include <FEXHeaderUtils/BitUtils.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>

namespace FEXCore::IR {

using X86Tables::OpToIndex;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::SyscallOp(OpcodeArgs, bool IsSyscallInst) {
  constexpr size_t SyscallArgs = 7;
  using SyscallArray = std::array<uint64_t, SyscallArgs>;

  size_t NumArguments {};
  const SyscallArray* GPRIndexes {};
  static constexpr SyscallArray GPRIndexes_64 = {
    FEXCore::X86State::REG_RAX, FEXCore::X86State::REG_RDI, FEXCore::X86State::REG_RSI, FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_R10, FEXCore::X86State::REG_R8,  FEXCore::X86State::REG_R9,
  };
  static constexpr SyscallArray GPRIndexes_32 = {
    FEXCore::X86State::REG_RAX, FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RCX, FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_RSI, FEXCore::X86State::REG_RDI, FEXCore::X86State::REG_RBP,
  };

  SyscallFlags DefaultSyscallFlags = FEXCore::IR::SyscallFlags::DEFAULT;

  const auto OSABI = CTX->SyscallHandler->GetOSABI();
  if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX64) {
    NumArguments = GPRIndexes_64.size();
    GPRIndexes = &GPRIndexes_64;
  } else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX32) {
    NumArguments = GPRIndexes_32.size();
    GPRIndexes = &GPRIndexes_32;
  } else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_GENERIC) {
    // All registers will be spilled before the syscall and filled afterwards so no JIT-side argument handling is necessary.
    NumArguments = 0;
    GPRIndexes = nullptr;
    DefaultSyscallFlags = FEXCore::IR::SyscallFlags::NORETURNEDRESULT;
  } else {
    ERROR_AND_DIE_FMT("Unhandled OSABI syscall");
  }

  // Calculate flags early.
  CalculateDeferredFlags();

  const auto GPRSize = GetGPROpSize();
  auto NewRIP = GetRelocatedPC(Op, -Op->InstSize);
  _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));

  Ref Arguments[SyscallArgs] {
    InvalidNode, InvalidNode, InvalidNode, InvalidNode, InvalidNode, InvalidNode, InvalidNode,
  };
  for (size_t i = 0; i < NumArguments; ++i) {
    Arguments[i] = LoadGPRRegister(GPRIndexes->at(i));
  }

  if (IsSyscallInst) {
    // If this is the `Syscall` instruction rather than `int 0x80` then we need to do some additional work.
    // RCX = RIP after this instruction
    // R11 = EFlags
    // Calculate flags.
    CalculateDeferredFlags();

    auto RFLAG = GetPackedRFLAG();
    StoreGPRRegister(X86State::REG_R11, RFLAG, OpSize::i64Bit);

    auto RIPAfterInst = GetRelocatedPC(Op);
    StoreGPRRegister(X86State::REG_RCX, RIPAfterInst, OpSize::i64Bit);
  }

  FlushRegisterCache();
  auto SyscallOp = _Syscall(Arguments[0], Arguments[1], Arguments[2], Arguments[3], Arguments[4], Arguments[5], Arguments[6], DefaultSyscallFlags);

  if ((DefaultSyscallFlags & FEXCore::IR::SyscallFlags::NORETURNEDRESULT) != FEXCore::IR::SyscallFlags::NORETURNEDRESULT) {
    StoreGPRRegister(X86State::REG_RAX, SyscallOp);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_BLOCK_END) {
    // RIP could have been updated after coming back from the Syscall.
    NewRIP = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, rip));
    ExitFunction(NewRIP);
  }
}

void OpDispatchBuilder::ThunkOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  uint8_t* sha256 = (uint8_t*)(Op->PC + 2);

  if (Is64BitMode) {
    // x86-64 ABI puts the function argument in RDI
    Thunk(LoadGPRRegister(X86State::REG_RDI), *reinterpret_cast<SHA256Sum*>(sha256));
  } else {
    // x86 fastcall ABI puts the function argument in ECX
    Thunk(LoadGPRRegister(X86State::REG_RCX), *reinterpret_cast<SHA256Sum*>(sha256));
  }

  auto NewRIP = Pop(GPRSize);

  // Store the new RIP
  ExitFunction(NewRIP, BranchHint::Return);
  BlockSetRIP = true;
}

void OpDispatchBuilder::LEAOp(OpcodeArgs) {
  // LEA specifically ignores segment prefixes
  const auto SrcSize = OpSizeFromSrc(Op);

  if (Is64BitMode) {
    const auto DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? OpSize::i16Bit :
                         X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST ? OpSize::i64Bit :
                                                                                                                              OpSize::i32Bit;

    auto Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, {.LoadData = false, .AllowUpperGarbage = SrcSize > DstSize});
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, OpSize::iInvalid);
  } else {
    const auto DstSize =
      X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? OpSize::i16Bit : OpSize::i32Bit;

    auto Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, {.LoadData = false, .AllowUpperGarbage = SrcSize > DstSize});
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::NOPOp(OpcodeArgs) {}

void OpDispatchBuilder::RETOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();

  // ABI Optimization: Flags don't survive calls or rets
  if (CTX->Config.ABILocalFlags) {
    _InvalidateFlags(~0UL); // all flags
    InvalidatePF_AF();
  }

  Ref SP = _RMWHandle(LoadGPRRegister(X86State::REG_RSP));
  Ref NewRIP = Pop(GPRSize, SP);

  if (Op->OP == 0xC2) {
    auto Offset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    SP = Add(GPRSize, SP, Offset);
  }

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, SP);

  // Store the new RIP
  ExitFunction(NewRIP, BranchHint::Return);
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
    LogMan::Msg::EFmt("IRET only implemented for 64bit and 32bit sizes");
    DecodeFailure = true;
    return;
  }

  const auto GPRSize = GetGPROpSize();

  Ref SP = _RMWHandle(LoadGPRRegister(X86State::REG_RSP));

  // RIP (64/32/16 bits)
  auto NewRIP = Pop(GPRSize, SP);
  // CS (lower 16 used)
  auto NewSegmentCS = Pop(GPRSize, SP);
  _StoreContext(OpSize::i16Bit, GPRClass, NewSegmentCS, offsetof(FEXCore::Core::CPUState, cs_idx));
  UpdatePrefixFromSegment(NewSegmentCS, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX);

  // eflags (lower 16 used)
  SetPackedRFLAG(false, Pop(GPRSize, SP));

  if (Is64BitMode) {
    // RSP and SS only happen in 64-bit mode or if this is a CPL mode jump!
    // FEX doesn't support a CPL mode switch, so don't need to worry about this on 32-bit
    StoreGPRRegister(X86State::REG_RSP, Pop(GPRSize, SP));

    // ss
    auto NewSegmentSS = Pop(GPRSize, SP);
    _StoreContext(OpSize::i16Bit, GPRClass, NewSegmentSS, offsetof(FEXCore::Core::CPUState, ss_idx));
    UpdatePrefixFromSegment(NewSegmentSS, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX);
  } else {
    // Store the stack in 32-bit mode
    StoreGPRRegister(X86State::REG_RSP, SP);
  }

  ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::CallbackReturnOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  // Store the new RIP
  _CallbackReturn();
  auto NewRIP = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, rip));
  // This ExitFunction won't actually get hit but needs to exist
  ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::SecondaryALUOp(OpcodeArgs) {
  FEXCore::IR::IROps IROp, AtomicIROp;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op->OP) {
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0):
    IROp = FEXCore::IR::IROps::OP_ADD;
    AtomicIROp = FEXCore::IR::IROps::OP_ATOMICFETCHADD;
    break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1):
    IROp = FEXCore::IR::IROps::OP_OR;
    AtomicIROp = FEXCore::IR::IROps::OP_ATOMICFETCHOR;
    break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4):
    IROp = FEXCore::IR::IROps::OP_ANDWITHFLAGS;
    AtomicIROp = FEXCore::IR::IROps::OP_ATOMICFETCHAND;
    break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5):
    IROp = FEXCore::IR::IROps::OP_SUB;
    AtomicIROp = FEXCore::IR::IROps::OP_ATOMICFETCHSUB;
    break;
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6):
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6):
    IROp = FEXCore::IR::IROps::OP_XOR;
    AtomicIROp = FEXCore::IR::IROps::OP_ATOMICFETCHXOR;
    break;
  default:
    IROp = FEXCore::IR::IROps::OP_LAST;
    AtomicIROp = FEXCore::IR::IROps::OP_LAST;
    LogMan::Msg::EFmt("Unknown ALU Op: 0x{:x}", Op->OP);
    DecodeFailure = true;
    return;
  };
#undef OPD

  ALUOp(Op, IROp, AtomicIROp, 1);
}

void OpDispatchBuilder::ADCOp(OpcodeArgs, uint32_t SrcIndex) {
  // Calculate flags early.
  CalculateDeferredFlags();

  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  const auto Size = OpSizeFromDst(Op);
  const auto OpSize = std::max(OpSize::i32Bit, Size);

  Ref Before {};
  if (DestIsLockedMem(Op)) {
    auto ALUOp = IncrementByCarry(OpSize, Src);
    HandledLock = true;

    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    Before = _AtomicFetchAdd(Size, ALUOp, DestMem);
  } else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  Ref Result;
  if (!DestIsLockedMem(Op) && Op->Src[SrcIndex].IsLiteral() && Op->Src[SrcIndex].Literal() == 0 && Size >= OpSize::i32Bit) {
    HandleNZCV_RMW();
    RectifyCarryInvert(true);
    Result = _AdcZeroWithFlags(OpSize, Before);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(Before);
    CalculatePF(Result);
    CFInverted = false;
  } else {
    Result = CalculateFlags_ADC(Size, Before, Src);
  }

  if (!DestIsLockedMem(Op)) {
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::SBBOp(OpcodeArgs, uint32_t SrcIndex) {
  // Calculate flags early.
  CalculateDeferredFlags();

  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  const auto Size = OpSizeFromDst(Op);
  const auto OpSize = std::max(OpSize::i32Bit, Size);

  Ref Result {};
  Ref Before {};
  if (DestIsLockedMem(Op)) {
    HandledLock = true;

    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    auto SrcPlusCF = IncrementByCarry(OpSize, Src);
    Before = _AtomicFetchSub(Size, SrcPlusCF, DestMem);
  } else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  Result = CalculateFlags_SBB(Size, Before, Src);

  if (!DestIsLockedMem(Op)) {
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::SALCOp(OpcodeArgs) {
  CalculateDeferredFlags();

  auto Result = NZCVSelect(OpSize::i32Bit, {COND_UGE} /* CF = 1 */, _InlineConstant(0xffffffff), _InlineConstant(0));

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PUSHOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);

  Push(Size, LoadSource(GPRClass, Op, Op->Src[0], Op->Flags));
}

void OpDispatchBuilder::PUSHREGOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);

  Push(Size, LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true}));
}

void OpDispatchBuilder::PUSHAOp(OpcodeArgs) {
  // 32bit only
  const auto Size = OpSizeFromSrc(Op);

  Ref OldSP = _Copy(LoadGPRRegister(X86State::REG_RSP));

  Push(Size, LoadGPRRegister(X86State::REG_RAX));
  Push(Size, LoadGPRRegister(X86State::REG_RCX));
  Push(Size, LoadGPRRegister(X86State::REG_RDX));
  Push(Size, LoadGPRRegister(X86State::REG_RBX));
  Push(Size, OldSP);
  Push(Size, LoadGPRRegister(X86State::REG_RBP));
  Push(Size, LoadGPRRegister(X86State::REG_RSI));
  Push(Size, LoadGPRRegister(X86State::REG_RDI));
}

void OpDispatchBuilder::PUSHSegmentOp(OpcodeArgs, uint32_t SegmentReg) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);

  Ref Src {};
  if (!Is64BitMode) {
    switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, es_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, cs_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, ss_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, ds_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, fs_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, gs_idx));
      break;
    default: FEX_UNREACHABLE;
    }
  } else {
    switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, es_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, cs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, ss_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, ds_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      Src = _LoadContext(SrcSize, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
      break;
    default: FEX_UNREACHABLE;
    }
  }

  // Store our value to the new stack location
  // AMD hardware zexts segment selector to 32bit
  // Intel hardware inserts segment selector
  Push(DstSize, Src);
}

void OpDispatchBuilder::POPOp(OpcodeArgs) {
  Ref Value = Pop(OpSizeFromSrc(Op));
  StoreResult(GPRClass, Op, Value, OpSize::iInvalid);
}

void OpDispatchBuilder::POPAOp(OpcodeArgs) {
  // 32bit only
  const auto Size = OpSizeFromSrc(Op);

  Ref SP = _RMWHandle(LoadGPRRegister(X86State::REG_RSP));

  StoreGPRRegister(X86State::REG_RDI, Pop(Size, SP), Size);
  StoreGPRRegister(X86State::REG_RSI, Pop(Size, SP), Size);
  StoreGPRRegister(X86State::REG_RBP, Pop(Size, SP), Size);

  // Skip loading RSP because it'll be correct at the end
  SP = _RMWHandle(Add(OpSize::i64Bit, SP, IR::OpSizeToSize(Size)));

  StoreGPRRegister(X86State::REG_RBX, Pop(Size, SP), Size);
  StoreGPRRegister(X86State::REG_RDX, Pop(Size, SP), Size);
  StoreGPRRegister(X86State::REG_RCX, Pop(Size, SP), Size);
  StoreGPRRegister(X86State::REG_RAX, Pop(Size, SP), Size);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, SP);
}

void OpDispatchBuilder::POPSegmentOp(OpcodeArgs, uint32_t SegmentReg) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);

  auto NewSegment = Pop(SrcSize);

  switch (SegmentReg) {
  case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, es_idx));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, cs_idx));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
    // Unset the 'active' bit in the packed TF, skipping the single step exception after this instruction
    SetRFLAG<FEXCore::X86State::RFLAG_TF_RAW_LOC>(_And(OpSize::i32Bit, GetRFLAG(FEXCore::X86State::RFLAG_TF_RAW_LOC), Constant(1)));
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ss_idx));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ds_idx));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, fs_idx));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
    _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, gs_idx));
    break;
  default: break; // Do nothing
  }

  UpdatePrefixFromSegment(NewSegment, SegmentReg);
}

void OpDispatchBuilder::LEAVEOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto OperandSize = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_OPERAND_SIZE) ? OpSize::i16Bit : GPRSize;

  // First we move RBP in to RSP and then behave effectively like a pop
  auto SP = _RMWHandle(LoadGPRRegister(X86State::REG_RBP));
  auto NewGPR = Pop(OperandSize, SP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, SP, OperandSize);

  // Store what we loaded to RBP
  StoreGPRRegister(X86State::REG_RBP, NewGPR, OperandSize);
}

void OpDispatchBuilder::CALLOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();

  BlockSetRIP = true;

  // ABI Optimization: Flags don't survive calls or rets
  if (CTX->Config.ABILocalFlags) {
    _InvalidateFlags(~0UL); // all flags
    InvalidatePF_AF();
  }

  // Call instruction only uses up to 32-bit signed displacement
  int64_t TargetOffset = Op->Src[0].Literal();

  auto ConstantPC = GetRelocatedPC(Op);

  // Push the return address.
  Push(GPRSize, ConstantPC);

  const uint64_t NextRIP = Op->PC + Op->InstSize;
  uint64_t TargetRIP = NextRIP + TargetOffset;

  if (NextRIP != TargetRIP) {
    // Store the RIP
    ExitRelocatedPC(Op, TargetOffset, BranchHint::Call, ConstantPC, [&]() {
      auto CallReturnJumpTarget = JumpTargets.find(NextRIP);
      if (CallReturnJumpTarget != JumpTargets.end() && CallReturnJumpTarget->second.IsEntryPoint) {
        return CallReturnJumpTarget->second.BlockEntry;
      }
      return InvalidNode;
    }());
  } else {
    NeedsBlockEnd = true;
  }
}

void OpDispatchBuilder::CALLAbsoluteOp(OpcodeArgs) {
  BlockSetRIP = true;

  const auto Size = OpSizeFromSrc(Op);
  Ref JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  // Push the return address.
  auto ConstantPC = GetRelocatedPC(Op);
  Push(Size, ConstantPC);

  // Store the RIP
  const uint64_t NextRIP = Op->PC + Op->InstSize;
  ExitFunction(JMPPCOffset, BranchHint::Call, ConstantPC, [&]() {
    auto CallReturnJumpTarget = JumpTargets.find(NextRIP);
    if (CallReturnJumpTarget != JumpTargets.end() && CallReturnJumpTarget->second.IsEntryPoint) {
      return CallReturnJumpTarget->second.BlockEntry;
    }
    return InvalidNode;
  }());
}

std::optional<CondClassType> OpDispatchBuilder::DecodeNZCVCondition(uint8_t OP) {
  switch (OP) {
  case 0x0: { // JO - Jump if OF == 1
    return CondClassType {COND_FU};
  }
  case 0x1: { // JNO - Jump if OF == 0
    return CondClassType {COND_FNU};
  }
  case 0x2: { // JC - Jump if CF == 1
    return CondClassType {CFInverted ? COND_ULT : COND_UGE};
  }
  case 0x3: { // JNC - Jump if CF == 0
    return CondClassType {CFInverted ? COND_UGE : COND_ULT};
  }
  case 0x4: { // JE - Jump if ZF == 1
    return CondClassType {COND_EQ};
  }
  case 0x5: { // JNE - Jump if ZF == 0
    return CondClassType {COND_NEQ};
  }
  case 0x6: { // JNA - Jump if CF == 1 || ZF == 1
    // With CF, we want (C == 0 || Z == 1). By De Morgan's, that's
    // equivalent to !(C == 1 && Z == 0). That's .ls
    RectifyCarryInvert(true);
    return CondClassType {COND_ULE};
  }
  case 0x7: { // JA - Jump if CF == 0 && ZF == 0
    // With CF inverted, we want (C == 1 && Z == 0). That's .hi
    RectifyCarryInvert(true);
    return CondClassType {COND_UGT};
  }
  case 0x8: { // JS - Jump if SF == 1
    return CondClassType {COND_MI};
  }
  case 0x9: { // JNS - Jump if SF == 0
    return CondClassType {COND_PL};
  }
  case 0xC: { // SF <> OF
    return CondClassType {COND_SLT};
  }
  case 0xD: { // SF = OF
    return CondClassType {COND_SGE};
  }
  case 0xE: { // ZF = 1 || SF <> OF
    return CondClassType {COND_SLE};
  }
  case 0xF: { // ZF = 0 && SF = OF
    return CondClassType {COND_SGT};
  }
  default:
    // Other conditions do not map directly, caller gets to deal with it.
    return std::nullopt;
  }
}

static bool ParityJumpIsJP(uint8_t OP) {
  LOGMAN_THROW_A_FMT(OP == 0xA || OP == 0xB, "JP or JNP");
  return OP == 0xA;
}

Ref OpDispatchBuilder::SelectCC0All1(uint8_t OP) {
  if (auto Cond = DecodeNZCVCondition(OP); Cond) {
    // Use raw select since DecodeNZCVCondition handles the carry invert
    return _NZCVSelect(OpSize::i64Bit, *Cond, _InlineConstant(~0ULL), _InlineConstant(0));
  } else {
    // Raw value contains inverted PF in bottom bit
    return _Sbfe(OpSize::i64Bit, 1, 0, LoadPFRaw(false, ParityJumpIsJP(OP)));
  }
}

void OpDispatchBuilder::SETccOp(OpcodeArgs) {
  CalculateDeferredFlags();

  Ref SrcCond;
  if (auto Cond = DecodeNZCVCondition(Op->OP & 0xf); Cond) {
    // Use raw select since DecodeNZCVCondition handles the carry invert
    SrcCond = _NZCVSelect01(*Cond);
  } else {
    SrcCond = LoadPFRaw(true, ParityJumpIsJP(Op->OP & 0xf));
  }

  StoreResult(GPRClass, Op, SrcCond, OpSize::iInvalid);
}

void OpDispatchBuilder::CMOVOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto OP = Op->OP & 0xF;
  const auto ResultSize = std::max(OpSize::i32Bit, OpSizeFromSrc(Op));

  CalculateDeferredFlags();

  // Destination is always a GPR.
  Ref Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);
  Ref Src {}, SrcCond {};
  if (Op->Src[0].IsGPR()) {
    Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  } else {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  }

  if (auto Cond = DecodeNZCVCondition(OP); Cond) {
    // Use raw select since DecodeNZCVCondition handles the carry invert
    SrcCond = _NZCVSelect(ResultSize, *Cond, Src, Dest);
  } else {
    // Raw value contains inverted PF in bottom bit
    Ref Cmp = LoadPFRaw(false, ParityJumpIsJP(OP));
    SaveNZCV();

    // Because we're only clobbering NZCV internally, we ignore all carry flag
    // shenanigans and just use the raw test and raw select.
    _TestNZ(OpSize::i32Bit, Cmp, _InlineConstant(1));
    SrcCond = _NZCVSelect(ResultSize, {COND_NEQ}, Src, Dest);
  }

  StoreResult(GPRClass, Op, SrcCond, OpSize::iInvalid);
}

void OpDispatchBuilder::CondJUMPOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  // Jump instruction only uses up to 32-bit signed displacement
  int64_t TargetOffset = Op->Src[0].Literal();
  uint64_t InstRIP = Op->PC + Op->InstSize;
  uint64_t Target = InstRIP + TargetOffset;

  if (GetGPROpSize() == OpSize::i32Bit) {
    // If the GPRSize is 4 then we need to be careful about PC wrapping
    if (TargetOffset < 0 && -TargetOffset > InstRIP) {
      // Invert the signed value if we are underflowing
      TargetOffset = 0x1'0000'0000ULL + TargetOffset;
    } else if (TargetOffset >= 0 && Target >= 0x1'0000'0000ULL) {
      // We are overflowing, wrap around
      TargetOffset = TargetOffset - 0x1'0000'0000ULL;
    }
    Target &= 0xFFFFFFFFU;
  }

  FlushRegisterCache();
  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  {
    IRPair<IR::IROp_CondJump> CondJump_;
    auto OP = Op->OP & 0xF;
    auto Cond = DecodeNZCVCondition(OP);
    if (Cond) {
      CondJump_ = CondJumpNZCV(*Cond);
    } else {
      LOGMAN_THROW_A_FMT(OP == 0xA || OP == 0xB, "only PF left");
      CondJump_ = CondJumpBit(LoadPFRaw(false, false), 0, OP == 0xB);
    }

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Store the new RIP
      ExitRelocatedPC(Op, TargetOffset);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      // Place it after this block for fallthrough optimization
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block & store the new RIP
      ExitRelocatedPC(Op);
    }
  }
}

void OpDispatchBuilder::CondJUMPRCXOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;
  auto JcxGPRSize = GetGPROpSize();
  JcxGPRSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? (JcxGPRSize >> 1) : JcxGPRSize;

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].Literal();

  Ref CondReg = LoadGPRRegister(X86State::REG_RCX, JcxGPRSize);

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  {
    auto CondJump_ = CondJump(CondReg, {COND_EQ});

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Store the new RIP
      ExitRelocatedPC(Op, Op->Src[0].Literal());
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      // Place it after the current block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block & store the new RIP
      ExitRelocatedPC(Op);
    }
  }
}

void OpDispatchBuilder::LoopOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  bool CheckZF = Op->OP != 0xE2;
  bool ZFTrue = Op->OP == 0xE1;

  BlockSetRIP = true;
  auto SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? OpSize::i32Bit : OpSize::i64Bit;
  auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;

  if (!Is64BitMode) {
    // RCX size is 32-bit or 16-bit when executing in 32-bit mode.
    SrcSize = IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) >> 1);
    OpSize = OpSize::i32Bit;
  }

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[1].Literal();

  Ref CondReg = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  CondReg = Sub(OpSize, CondReg, 1);
  StoreResult(GPRClass, Op, Op->Src[0], CondReg, OpSize::iInvalid);

  // If LOOPE then jumps to target if RCX != 0 && ZF == 1
  // If LOOPNE then jumps to target if RCX != 0 && ZF == 0
  //
  // To handle efficiently, smash RCX to zero if ZF is wrong (1 csel).
  if (CheckZF) {
    CondReg = NZCVSelect(OpSize, {ZFTrue ? COND_EQ : COND_NEQ}, CondReg, _InlineConstant(0));
  }

  CalculateDeferredFlags();
  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  {
    auto CondJump_ = CondJump(CondReg);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Store the new RIP
      ExitRelocatedPC(Op, Op->Src[1].Literal());
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    } else {
      // Make sure to start a new block after ending this one
      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block & store the new RIP
      ExitRelocatedPC(Op);
    }
  }
}

void OpDispatchBuilder::JUMPOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  // Jump instruction only uses up to 32-bit signed displacement
  int64_t TargetOffset = Op->Src[0].Literal();
  uint64_t InstRIP = Op->PC + Op->InstSize;
  uint64_t TargetRIP = InstRIP + TargetOffset;

  if (GetGPROpSize() == OpSize::i32Bit) {
    // If the GPRSize is 4 then we need to be careful about PC wrapping
    if (TargetOffset < 0 && -TargetOffset > InstRIP) {
      // Invert the signed value if we are underflowing
      TargetOffset = 0x1'0000'0000ULL + TargetOffset;
    } else if (TargetOffset >= 0 && TargetRIP >= 0x1'0000'0000ULL) {
      // We are overflowing, wrap around
      TargetOffset = TargetOffset - 0x1'0000'0000ULL;
    }

    TargetRIP &= 0xFFFFFFFFU;
  }

  CalculateDeferredFlags();
  // This is just an unconditional relative literal jump
  if (Multiblock) {
    auto JumpBlock = JumpTargets.find(TargetRIP);
    if (JumpBlock != JumpTargets.end()) {
      Jump(GetNewJumpBlock(TargetRIP));
    } else {
      // If the block isn't a jump target then we need to create an exit block
      auto Jump_ = Jump();

      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(Jump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();
      ExitRelocatedPC(Op, TargetOffset);
    }
  } else {
    ExitRelocatedPC(Op, TargetOffset);
  }
}

void OpDispatchBuilder::JUMPAbsoluteOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;
  // This is just an unconditional jump
  // This uses ModRM to determine its location
  // No way to use this effectively in multiblock
  auto RIPOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  // Store the new RIP
  ExitFunction(RIPOffset);
}

void OpDispatchBuilder::JUMPFARIndirectOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;
  // This is just an unconditional jump
  // This uses ModRM to determine its location
  // No way to use this effectively in multiblock
  Ref Src = MakeSegmentAddress(Op, Op->Dest);
  AddressMode SrcCS = {.Base = Src, .Offset = 4, .AddrSize = OpSize::i64Bit};
  auto RIPOffset = _LoadMemAutoTSO(GPRClass, OpSize::i32Bit, Src, OpSize::i8Bit);
  auto NewSegmentCS = _LoadMemAutoTSO(GPRClass, OpSize::i16Bit, SrcCS, OpSize::i8Bit);

  // Set up the new CSSegment.
  _StoreContext(OpSize::i16Bit, GPRClass, NewSegmentCS, offsetof(FEXCore::Core::CPUState, cs_idx));
  UpdatePrefixFromSegment(NewSegmentCS, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX);

  // Store the new RIP
  ExitFunction(RIPOffset);
}

void OpDispatchBuilder::CALLFARIndirectOp(OpcodeArgs) {
  const auto SrcSize = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? OpSize::i64Bit : OpSize::i32Bit;

  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  Ref Src = MakeSegmentAddress(Op, Op->Dest);
  AddressMode SrcCS = {.Base = Src, .Offset = 4, .AddrSize = OpSize::i64Bit};
  auto RIPOffset = _LoadMemAutoTSO(GPRClass, OpSize::i32Bit, Src, OpSize::i8Bit);
  auto NewSegmentCS = _LoadMemAutoTSO(GPRClass, OpSize::i16Bit, SrcCS, OpSize::i8Bit);
  auto CurrentCS = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, cs_idx));

  auto NewRIP = GetRelocatedPC(Op);

  // Push the current CS
  Push(SrcSize, CurrentCS);

  // Push the return address.
  Push(SrcSize, NewRIP);

  // Set up the new CSSegment.
  _StoreContext(OpSize::i16Bit, GPRClass, NewSegmentCS, offsetof(FEXCore::Core::CPUState, cs_idx));
  UpdatePrefixFromSegment(NewSegmentCS, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX);

  // Store the new RIP
  ExitFunction(RIPOffset);
}

void OpDispatchBuilder::RETFARIndirectOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto SrcSize = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? OpSize::i64Bit : OpSize::i32Bit;

  Ref SP = _RMWHandle(LoadGPRRegister(X86State::REG_RSP));
  Ref NewRIP = Pop(SrcSize, SP);
  Ref NewSegmentCS = Pop(SrcSize, SP);

  // Optional SP offset.
  if (Op->Src[0].IsLiteral()) {
    SP = Add(GPRSize, SP, Op->Src[0].Literal());
  }

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, SP);

  _StoreContext(OpSize::i16Bit, GPRClass, NewSegmentCS, offsetof(FEXCore::Core::CPUState, cs_idx));
  UpdatePrefixFromSegment(NewSegmentCS, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX);

  // Store the new RIP
  ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::TESTOp(OpcodeArgs, uint32_t SrcIndex) {
  // TEST is an instruction that does an AND between the sources
  // Result isn't stored in result, only writes to flags
  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = OpSizeFromDst(Op);
  LOGMAN_THROW_A_FMT(Size >= IR::OpSize::i8Bit && Size <= IR::OpSize::i64Bit, "Invalid size");

  uint64_t Const;
  bool AlwaysNonnegative = false;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    // Optimize out masking constants
    if (Const == (Size == OpSize::i64Bit ? ~0ULL : ((1ull << IR::OpSizeAsBits(Size)) - 1))) {
      Src = Dest;
    }

    // Optimize test with non-sign bits
    AlwaysNonnegative = (Const & (1ull << (IR::OpSizeAsBits(Size) - 1))) == 0;
  }

  if (Dest == Src) {
    // Optimize out the AND.
    SetNZP_ZeroCV(Size, Src);
  } else if (Size < OpSize::i32Bit && AlwaysNonnegative) {
    // If we know the result is always nonnegative, we can use a 32-bit test.
    auto Res = _And(OpSize::i32Bit, Dest, Src);
    CalculatePF(Res);
    SetNZ_ZeroCV(OpSize::i32Bit, Res);
  } else {
    HandleNZ00Write();
    CalculatePF(_AndWithFlags(Size, Dest, Src));
  }

  InvalidateAF();
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
  auto Size = std::min<IR::OpSize>(OpSize::i32Bit, OpSizeFromSrc(Op));
  bool Sext = (Size != OpSize::i16Bit) && Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING;

  Ref Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], Size, Op->Flags, {.AllowUpperGarbage = Sext});
  if (Size == OpSize::i16Bit) {
    // This'll make sure to insert in to the lower 16bits without modifying upper bits
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, Size, OpSize::iInvalid);
  } else if (Sext) {
    // With REX.W then Sext
    Src = _Sbfe(OpSize::i64Bit, IR::OpSizeAsBits(Size), 0, Src);
    StoreResult(GPRClass, Op, Src, OpSize::iInvalid);
  } else {
    // Without REX.W then Zext (store result implicitly zero extends)
    StoreResult(GPRClass, Op, Src, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::MOVSXOp(OpcodeArgs) {
  // Load garbage in upper bits, since we're sign extending anyway
  const auto Size = OpSizeFromSrc(Op);
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  // Sign-extend to DstSize and zero-extend to the register size, using a fast
  // path for 32-bit dests where the native 32-bit Sbfe zero extends the top.
  const auto DstSize = OpSizeFromDst(Op);
  Src = _Sbfe(DstSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit, IR::OpSizeAsBits(Size), 0, Src);
  StoreResult(GPRClass, Op, Op->Dest, Src, OpSize::iInvalid);
}

void OpDispatchBuilder::MOVZXOp(OpcodeArgs) {
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // Store result implicitly zero extends
  StoreResult(GPRClass, Op, Src, OpSize::iInvalid);
}

void OpDispatchBuilder::CMPOp(OpcodeArgs, uint32_t SrcIndex) {
  // CMP is an instruction that does a SUB between the sources
  // Result isn't stored in result, only writes to flags
  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  CalculateFlags_SUB(OpSizeFromSrc(Op), Dest, Src);
}

void OpDispatchBuilder::CQOOp(OpcodeArgs) {
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto Size = OpSizeFromSrc(Op);
  Ref Upper = _Sbfe(std::max(OpSize::i32Bit, Size), 1, GetSrcBitSize(Op) - 1, Src);

  StoreResult(GPRClass, Op, Upper, OpSize::iInvalid);
}

void OpDispatchBuilder::XCHGOp(OpcodeArgs) {
  // Load both the source and the destination
  if (Op->OP == 0x90 && Op->Src[0].IsGPR() && Op->Src[0].Data.GPR.GPR == FEXCore::X86State::REG_RAX && Op->Dest.IsGPR() &&
      Op->Dest.Data.GPR.GPR == FEXCore::X86State::REG_RAX) {
    // This is one heck of a sucky special case
    // If we are the 0x90 XCHG opcode (Meaning source is GPR RAX)
    // and destination register is ALSO RAX
    // and in this very specific case we are 32bit or above
    // Then this is a no-op
    // This is because 0x90 without a prefix is technically `xchg eax, eax`
    // But this would result in a zext on 64bit, which would ruin the no-op nature of the instruction
    // So x86-64 spec mandates this special case that even though it is a 32bit instruction and
    // is supposed to zext the result, it is a true no-op
    //
    // x86 spec text here:
    //
    //    XCHG (E)AX, (E)AX (encoded instruction byte is 90H) is an alias for
    //    NOP regardless of data size prefixes, including REX.W.
    //
    // Note that also includes 16-bit so we don't gate this on size. The
    // sequence (66 90) is a valid two-byte nop that we also ignore.
    if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
      // If this instruction has a REP prefix then this is architecturally
      // defined to be a `PAUSE` instruction. On older processors this ends up
      // being a true `REP NOP` which is why they stuck this here.
      _Yield();
    }
    return;
  }

  // AllowUpperGarbage: OK to allow as it will be overwritten by StoreResult.
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  if (DestIsMem(Op)) {
    HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;

    Ref Dest = MakeSegmentAddress(Op, Op->Dest);
    if (IsMonoBackpatcherBlock) {
      _MonoBackpatcherWrite(OpSizeFromSrc(Op), Src, Dest);
    } else {
      auto Result = _AtomicSwap(OpSizeFromSrc(Op), Src, Dest);
      StoreResult(GPRClass, Op, Op->Src[0], Result, OpSize::iInvalid);
    }
  } else {
    // AllowUpperGarbage: OK to allow as it will be overwritten by StoreResult.
    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    // Swap the contents
    // Order matters here since we don't want to swap context contents for one that effects the other
    StoreResult(GPRClass, Op, Op->Dest, Src, OpSize::iInvalid);
    StoreResult(GPRClass, Op, Op->Src[0], Dest, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::CDQOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto SrcSize = DstSize / 2;
  Ref Src = LoadGPRRegister(X86State::REG_RAX, SrcSize, 0, true);

  Src = _Sbfe(DstSize <= OpSize::i32Bit ? OpSize::i32Bit : OpSize::i64Bit, IR::OpSizeAsBits(SrcSize), 0, Src);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, OpSize::iInvalid);
}

void OpDispatchBuilder::SAHFOp(OpcodeArgs) {
  // Extract AH
  Ref Src = LoadGPRRegister(X86State::REG_RAX, OpSize::i8Bit, 8);

  // Clear bits that aren't supposed to be set
  Src = _Andn(OpSize::i64Bit, Src, Constant(0b101000));

  // Set the bit that is always set here
  Src = _Or(OpSize::i64Bit, Src, _InlineConstant(0b10));

  // Store the lower 8 bits in to RFLAGS
  SetPackedRFLAG(true, Src);
}
void OpDispatchBuilder::LAHFOp(OpcodeArgs) {
  // Load the lower 8 bits of the Rflags register
  auto RFLAG = GetPackedRFLAG(0xFF);

  // Store the lower 8 bits of the rflags register in to AH
  StoreGPRRegister(X86State::REG_RAX, RFLAG, OpSize::i8Bit, 8);
}

void OpDispatchBuilder::FLAGControlOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  switch (Op->OP) {
  case 0xF5: // CMC
    CarryInvert();
    break;
  case 0xF8: // CLC
    SetCFInverted(Constant(1));
    break;
  case 0xF9: // STC
    SetCFInverted(Constant(0));
    break;
  case 0xFC: // CLD
    // Transformed
    StoreDF(Constant(1));
    break;
  case 0xFD: // STD
    StoreDF(Constant(-1));
    break;
  }
}


void OpDispatchBuilder::MOVSegOp(OpcodeArgs, bool ToSeg) {
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
    Ref Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], OpSize::i16Bit, Op->Flags);

    switch (Op->Dest.Data.GPR.GPR) {
    case FEXCore::X86State::REG_RAX: // ES
    case FEXCore::X86State::REG_R8:  // ES
      _StoreContext(OpSize::i16Bit, GPRClass, Src, offsetof(FEXCore::Core::CPUState, es_idx));
      UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX);
      break;
    case FEXCore::X86State::REG_RBX: // DS
    case FEXCore::X86State::REG_R11: // DS
      _StoreContext(OpSize::i16Bit, GPRClass, Src, offsetof(FEXCore::Core::CPUState, ds_idx));
      UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);
      break;
    case FEXCore::X86State::REG_RCX: // CS
    case FEXCore::X86State::REG_R9:  // CS
      // CPL3 can't write to this
      Break(FEXCore::IR::BreakDefinition {
        .ErrorRegister = 0,
        .Signal = SIGILL,
        .TrapNumber = 0,
        .si_code = 0,
      });
      break;
    case FEXCore::X86State::REG_RDX: // SS
    case FEXCore::X86State::REG_R10: // SS
      _StoreContext(OpSize::i16Bit, GPRClass, Src, offsetof(FEXCore::Core::CPUState, ss_idx));
      UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX);
      break;
    case FEXCore::X86State::REG_RBP: // GS
    case FEXCore::X86State::REG_R13: // GS
      if (!Is64BitMode) {
        _StoreContext(OpSize::i16Bit, GPRClass, Src, offsetof(FEXCore::Core::CPUState, gs_idx));
        UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX);
      } else {
        LogMan::Msg::EFmt("We don't support modifying GS selector in 64bit mode!");
        DecodeFailure = true;
      }
      break;
    case FEXCore::X86State::REG_RSP: // FS
    case FEXCore::X86State::REG_R12: // FS
      if (!Is64BitMode) {
        _StoreContext(OpSize::i16Bit, GPRClass, Src, offsetof(FEXCore::Core::CPUState, fs_idx));
        UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX);
      } else {
        LogMan::Msg::EFmt("We don't support modifying FS selector in 64bit mode!");
        DecodeFailure = true;
      }
      break;
    default: UnimplementedOp(Op); return;
    }
  } else {
    Ref Segment {};

    switch (Op->Src[0].Data.GPR.GPR) {
    case FEXCore::X86State::REG_RAX: // ES
    case FEXCore::X86State::REG_R8:  // ES
      Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, es_idx));
      break;
    case FEXCore::X86State::REG_RBX: // DS
    case FEXCore::X86State::REG_R11: // DS
      Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, ds_idx));
      break;
    case FEXCore::X86State::REG_RCX: // CS
    case FEXCore::X86State::REG_R9:  // CS
      Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, cs_idx));
      break;
    case FEXCore::X86State::REG_RDX: // SS
    case FEXCore::X86State::REG_R10: // SS
      Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, ss_idx));
      break;
    case FEXCore::X86State::REG_RBP: // GS
    case FEXCore::X86State::REG_R13: // GS
      if (Is64BitMode) {
        Segment = Constant(0);
      } else {
        Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, gs_idx));
      }
      break;
    case FEXCore::X86State::REG_RSP: // FS
    case FEXCore::X86State::REG_R12: // FS
      if (Is64BitMode) {
        Segment = Constant(0);
      } else {
        Segment = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, fs_idx));
      }
      break;
    default: UnimplementedOp(Op); return;
    }
    if (DestIsMem(Op)) {
      // If the destination is memory then we always store 16-bits only
      StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Segment, OpSize::i16Bit, OpSize::iInvalid);
    } else {
      // If the destination is a GPR then we follow register storing rules
      StoreResult(GPRClass, Op, Segment, OpSize::iInvalid);
    }
  }
}

void OpDispatchBuilder::MOVOffsetOp(OpcodeArgs) {

  auto GenMemSrcFromOp = [&](size_t StartingSource) -> AddressMode {
    const uint64_t Lower = Op->Src[StartingSource].Literal();
    const uint64_t Upper = Op->Src[StartingSource + 1].Literal();
    const uint64_t Combined = (Upper << 32) | Lower;
    const auto GPRSize = GetGPROpSize();

    AddressMode A {
      .Segment = GetSegment(Op->Flags),
      .Offset = static_cast<int64_t>(Combined),
      .AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize,
      .NonTSO = false,
    };

    return A;
  };
  switch (Op->OP) {
  case 0xA0:
  case 0xA1: {
    // Source is memory(literal)
    // Dest is GPR
    Ref Src {};
    if (Op->Src[0].Data.Literal.Size <= 4) {
      Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.ForceLoad = true});
    } else {
      const auto OpSize = OpSizeFromSrc(Op);
      auto A = GenMemSrcFromOp(0);
      Src = _LoadMemAutoTSO(GPRClass, OpSize, A, OpSize::i8Bit);
    }
    StoreResult(GPRClass, Op, Op->Dest, Src, OpSize::iInvalid);
    break;
  }
  case 0xA2:
  case 0xA3: {
    // Source is GPR
    // Dest is memory(literal)
    Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

    // This one is a bit special since the destination is a literal
    // So the destination gets stored in Src[1]
    if (Op->Src[1].Data.Literal.Size <= 4) {
      StoreResult(GPRClass, Op, Op->Src[1], Src, OpSize::iInvalid);
    } else {
      const auto OpSize = OpSizeFromSrc(Op);
      auto A = GenMemSrcFromOp(1);
      _StoreMemAutoTSO(GPRClass, OpSize, A, Src, OpSize::i8Bit);
    }
    break;
  }
  }
}

void OpDispatchBuilder::CPUIDOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();

  Ref Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  Ref Leaf = LoadGPRRegister(X86State::REG_RCX);

  Ref RAX = _AllocateGPR(false);
  Ref RBX = _AllocateGPR(false);
  Ref RCX = _AllocateGPR(false);
  Ref RDX = _AllocateGPR(false);

  _CPUID(Src, Leaf, RAX, RBX, RCX, RDX);

  StoreGPRRegister(X86State::REG_RAX, RAX);
  StoreGPRRegister(X86State::REG_RBX, RBX);
  StoreGPRRegister(X86State::REG_RCX, RCX);
  StoreGPRRegister(X86State::REG_RDX, RDX);
}

uint32_t OpDispatchBuilder::LoadConstantShift(X86Tables::DecodedOp Op, bool Is1Bit) {
  if (Is1Bit) {
    return 1;
  } else {
    // x86 masks the shift by 0x3F or 0x1F depending on size of op
    const auto Size = OpSizeFromSrc(Op);
    uint64_t Mask = Size == OpSize::i64Bit ? 0x3F : 0x1F;

    return Op->Src[1].Literal() & Mask;
  }
}

void OpDispatchBuilder::XGetBVOp(OpcodeArgs) {
  Ref Function = LoadGPRRegister(X86State::REG_RCX);

  auto RAX = _AllocateGPR(false);
  auto RDX = _AllocateGPR(false);
  _XGetBV(Function, RAX, RDX);

  StoreGPRRegister(X86State::REG_RAX, RAX);
  StoreGPRRegister(X86State::REG_RDX, RDX);
}

void OpDispatchBuilder::SHLOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  auto Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  Ref Result = _Lshl(Size == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit, Dest, Src);
  HandleShift(Op, Result, Dest, ShiftType::LSL, Src);
}

void OpDispatchBuilder::SHLImmediateOp(OpcodeArgs, bool SHL1Bit) {
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

  uint64_t Shift = LoadConstantShift(Op, SHL1Bit);
  const auto Size = GetSrcBitSize(Op);

  Ref Result = _Lshl(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Constant(Shift));

  CalculateFlags_ShiftLeftImmediate(OpSizeFromSrc(Op), Result, Dest, Shift);
  CalculateDeferredFlags();
  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHROp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= OpSize::i32Bit});
  auto Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  auto ALUOp = _Lshr(std::max(OpSize::i32Bit, Size), Dest, Src);
  HandleShift(Op, ALUOp, Dest, ShiftType::LSR, Src);
}

void OpDispatchBuilder::SHRImmediateOp(OpcodeArgs, bool SHR1Bit) {
  const auto Size = GetSrcBitSize(Op);
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});

  uint64_t Shift = LoadConstantShift(Op, SHR1Bit);
  auto ALUOp = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Constant(Shift));

  CalculateFlags_ShiftRightImmediate(OpSizeFromSrc(Op), ALUOp, Dest, Shift);
  CalculateDeferredFlags();
  StoreResult(GPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::SHLDOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  const auto Size = GetSrcBitSize(Op);

  // Allow garbage on the Src if it will be ignored by the Lshr below
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = Size >= 32});
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  // Allow garbage on the shift, we're masking it anyway.
  Ref Shift = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  // x86 masks the shift by 0x3F or 0x1F depending on size of op.
  if (Size == 64) {
    Shift = _And(OpSize::i64Bit, Shift, _InlineConstant(0x3F));
  } else {
    Shift = _And(OpSize::i64Bit, Shift, _InlineConstant(0x1F));
  }

  // a64 masks the bottom bits, so if we're using a native 32/64-bit shift, we
  // can negate to do the subtract (it's congruent), which saves a constant.
  auto ShiftRight = Size >= 32 ? _Neg(OpSize::i64Bit, Shift) : Sub(OpSize::i64Bit, Constant(Size), Shift);

  auto Tmp1 = _Lshl(OpSize::i64Bit, Dest, Shift);
  auto Tmp2 = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Src, ShiftRight);

  Ref Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  //
  // TODO: With a backwards pass ahead-of-time, we could stick this in the
  // if(shift) used for flags.
  //
  // TODO: This whole function wants to be wrapped in the if. Maybe b/w pass is
  // a good idea after all.
  Res = _Select(FEXCore::IR::COND_EQ, Shift, Constant(0), Dest, Res);

  HandleShift(Op, Res, Dest, ShiftType::LSL, Shift);
}

void OpDispatchBuilder::SHLDImmediateOp(OpcodeArgs) {
  uint64_t Shift = LoadConstantShift(Op, false);
  const auto Size = GetSrcBitSize(Op);

  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = Size >= 32});
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});

  if (Shift != 0) {
    Ref Res {};
    if (Size < 32) {
      Ref ShiftLeft = Constant(Shift);
      auto ShiftRight = Size - Shift;

      auto Tmp1 = _Lshl(OpSize::i64Bit, Dest, ShiftLeft);
      Ref Tmp2 = ShiftRight ? _Lshr(OpSize::i32Bit, Src, Constant(ShiftRight)) : Src;

      Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);
    } else {
      // 32-bit and 64-bit SHLD behaves like an EXTR where the lower bits are filled from the source.
      Res = _Extr(OpSizeFromSrc(Op), Dest, Src, Size - Shift);
    }

    CalculateFlags_ShiftLeftImmediate(OpSizeFromSrc(Op), Res, Dest, Shift);
    CalculateDeferredFlags();
    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
  } else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::SHRDOp(OpcodeArgs) {
  // Calculate flags early.
  // This instruction conditionally generates flags so we need to insure sane state going in.
  CalculateDeferredFlags();

  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  Ref Shift = LoadGPRRegister(X86State::REG_RCX);

  const auto Size = GetDstBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift = _And(OpSize::i64Bit, Shift, _InlineConstant(0x3F));
  } else {
    Shift = _And(OpSize::i64Bit, Shift, _InlineConstant(0x1F));
  }

  auto ShiftLeft = Sub(OpSize::i64Bit, Constant(Size), Shift);

  auto Tmp1 = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Shift);
  auto Tmp2 = _Lshl(OpSize::i64Bit, Src, ShiftLeft);

  Ref Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  Res = _Select(FEXCore::IR::COND_EQ, Shift, Constant(0), Dest, Res);

  HandleShift(Op, Res, Dest, ShiftType::LSR, Shift);
}

void OpDispatchBuilder::SHRDImmediateOp(OpcodeArgs) {
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  uint64_t Shift = LoadConstantShift(Op, false);
  const auto Size = GetSrcBitSize(Op);

  if (Shift != 0) {
    Ref Res {};
    if (Size < 32) {
      Ref ShiftRight = Constant(Shift);
      auto ShiftLeft = Constant(Size - Shift);

      auto Tmp1 = _Lshr(OpSize::i32Bit, Dest, ShiftRight);
      auto Tmp2 = _Lshl(OpSize::i64Bit, Src, ShiftLeft);

      Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);
    } else {
      // 32-bit and 64-bit SHRD behaves like an EXTR where the upper bits are filled from the source.
      Res = _Extr(OpSizeFromSrc(Op), Src, Dest, Shift);
    }

    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
    CalculateFlags_ShiftRightDoubleImmediate(OpSizeFromSrc(Op), Res, Dest, Shift);
  } else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::ASHROp(OpcodeArgs, bool Immediate, bool SHR1Bit) {
  const auto Size = OpSizeFromSrc(Op);
  const auto OpSize = std::max(OpSize::i32Bit, OpSizeFromDst(Op));

  // If Size < 4, then we Sbfe the Dest so we can have garbage.
  // Otherwise, if Size = Opsize, then both are 4 or 8 and match the a64
  // semantics directly, so again we can have garbage. The only case where we
  // need zero-extension here is when the sizes mismatch.
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = (OpSize == Size) || (Size < OpSize::i32Bit)});

  if (Size < OpSize::i32Bit) {
    Dest = _Sbfe(OpSize::i64Bit, IR::OpSizeAsBits(Size), 0, Dest);
  }

  if (Immediate) {
    uint64_t Shift = LoadConstantShift(Op, SHR1Bit);
    Ref Result = _Ashr(OpSize, Dest, Constant(Shift));

    CalculateFlags_SignShiftRightImmediate(OpSizeFromSrc(Op), Result, Dest, Shift);
    CalculateDeferredFlags();
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  } else {
    auto Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
    Ref Result = _Ashr(OpSize, Dest, Src);

    HandleShift(Op, Result, Dest, ShiftType::ASR, Src);
  }
}

void OpDispatchBuilder::RotateOp(OpcodeArgs, bool Left, bool IsImmediate, bool Is1Bit) {
  CalculateDeferredFlags();

  const uint32_t Size = GetSrcBitSize(Op);
  const auto OpSize = Size == 64 ? OpSize::i64Bit : OpSize::i32Bit;
  uint64_t UnmaskedConst {};

  // x86 masks the shift by 0x3F or 0x1F depending on size of op. But it's
  // equivalent to mask to the actual size of the op, that way we can bound
  // things tighter for 8-bit later in the function.
  uint64_t Mask = Size == 8 ? 7 : (Size == 64 ? 0x3F : 0x1F);

  ArithRef UnmaskedSrc;
  if (Is1Bit || IsImmediate) {
    UnmaskedConst = LoadConstantShift(Op, Is1Bit);
    UnmaskedSrc = ARef(UnmaskedConst);
  } else {
    UnmaskedSrc = ARef(LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true}));
  }
  auto Src = UnmaskedSrc.And(Mask);

  // We fill the upper bits so we allow garbage on load.
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

  if (Size < 32) {
    // ARM doesn't support 8/16bit rotates. Emulate with an insert
    // StoreResult truncates back to a 8/16 bit value
    Dest = _Bfi(OpSize, Size, Left ? (32 - Size) : Size, Dest, Dest);
  }

  // To rotate 64-bits left, right-rotate by (64 - Shift) = -Shift mod 64.
  auto Res = _Ror(OpSize, Dest, (Left ? Src.Neg() : Src).Ref());
  StoreResult(GPRClass, Op, Res, OpSize::iInvalid);

  if (Is1Bit || IsImmediate) {
    if (UnmaskedSrc.C) {
      // Extract the last bit shifted in to CF
      SetCFDirect(Res, Left ? 0 : Size - 1, true);

      // For ROR, OF is the XOR of the new CF bit and the most significant bit of the result.
      // For ROL, OF is the LSB and MSB XOR'd together.
      // OF is architecturally only defined for 1-bit rotate.
      if (UnmaskedSrc.C == 1) {
        auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, Left ? Size - 1 : 1);
        SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Left ? 0 : Size - 2, true);
      }
    }
  } else {
    HandleNZCVWrite();
    RectifyCarryInvert(true);

    // We deferred the masking for 8-bit to the flag section, do it here.
    if (Size == 8) {
      Src = UnmaskedSrc.And(0x1F);
    }

    _RotateFlags(OpSizeFromSrc(Op), Res, Src.Ref(), Left);
  }
}

void OpDispatchBuilder::ANDNBMIOp(OpcodeArgs) {
  auto* Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  auto Dest = _Andn(OpSizeFromSrc(Op), Src2, Src1);

  StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
  CalculateFlags_Logical(OpSizeFromSrc(Op), Dest);
}

void OpDispatchBuilder::BEXTRBMIOp(OpcodeArgs) {
  // Essentially (Src1 >> Start) & ((1 << Length) - 1)
  // along with some edge-case handling and flag setting.

  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = OpSizeFromSrc(Op);
  const auto SrcSize = IR::OpSizeAsBits(Size);
  const auto MaxSrcBit = SrcSize - 1;
  auto MaxSrcBitOp = Constant(MaxSrcBit);

  // Shift the operand down to the starting bit
  auto Start = _Bfe(OpSizeFromSrc(Op), 8, 0, Src2);
  auto Shifted = _Lshr(Size, Src1, Start);

  // Shifts larger than operand size need to be set to zero.
  auto SanitizedShifted = _Select(Size, Size, CondClassType {IR::COND_ULE}, Start, MaxSrcBitOp, Shifted, Constant(0));

  // Now handle the length specifier.
  auto Length = _Bfe(Size, 8, 8, Src2);

  // Now build up the mask
  // (1 << Length) - 1 = ~(~0 << Length)
  auto AllOnes = Constant(~0ull);
  auto InvertedMask = _Lshl(Size, AllOnes, Length);

  // Now put it all together and make the result.
  auto Masked = _Andn(Size, SanitizedShifted, InvertedMask);

  // Sanitize the length. If it is above the max, we don't do the masking.
  auto Dest = _Select(Size, Size, CondClassType {IR::COND_ULE}, Length, MaxSrcBitOp, Masked, SanitizedShifted);

  // Finally store the result.
  StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);

  // ZF is set properly. CF and OF are defined as being set to zero. SF, PF, and
  // AF are undefined.
  SetNZ_ZeroCV(GetOpSize(Dest), Dest);
  InvalidatePF_AF();
}

void OpDispatchBuilder::BLSIBMIOp(OpcodeArgs) {
  // Equivalent to performing: SRC & -SRC
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  const auto Size = OpSizeFromSrc(Op);

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto NegatedSrc = _Neg(Size, Src);
  auto Result = _And(Size, Src, NegatedSrc);

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);

  // CF is cleared if Src is zero, otherwise it's set. However, Src is zero iff
  // Result is zero, so we can test the result instead. So, CF is just the
  // inverted ZF.
  //
  // ZF/SF/OF set as usual.
  SetNZ_ZeroCV(Size, Result);
  InvalidatePF_AF();
  SetCFInverted(GetRFLAG(X86State::RFLAG_ZF_RAW_LOC));
}

void OpDispatchBuilder::BLSMSKBMIOp(OpcodeArgs) {
  // Equivalent to: (Src - 1) ^ Src
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  const auto Size = OpSizeFromSrc(Op);

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _Xor(Size, Sub(Size, Src, 1), Src);

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  InvalidatePF_AF();

  // CF set according to the Src
  auto CFInv = To01(OpSize::i64Bit, Src);

  // The output of BLSMSK is always nonzero, so TST will clear Z (along with C
  // and O) while setting S.
  SetNZ_ZeroCV(Size, Result);
  SetCFInverted(CFInv);
}

void OpDispatchBuilder::BLSRBMIOp(OpcodeArgs) {
  // Equivalent to: (Src - 1) & Src
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  const auto Size = OpSizeFromSrc(Op);

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _And(Size, Sub(Size, Src, 1), Src);

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);

  auto CFInv = To01(OpSize::i64Bit, Src);

  SetNZ_ZeroCV(Size, Result);
  SetCFInverted(CFInv);
  InvalidatePF_AF();
}

// Handles SARX, SHLX, and SHRX
void OpDispatchBuilder::BMI2Shift(OpcodeArgs) {
  // In the event the source is a memory operand, use the
  // exact width instead of the GPR size.
  const auto GPRSize = GetGPROpSize();
  const auto Size = OpSizeFromSrc(Op);
  const auto SrcSize = Op->Src[0].IsGPR() ? GPRSize : Size;

  auto* Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  auto* Shift = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], GPRSize, Op->Flags, {.AllowUpperGarbage = true});

  Ref Result;
  if (Op->OP == 0x6F7) {
    // SARX
    Result = _Ashr(Size, Src, Shift);
  } else if (Op->OP == 0x5F7) {
    // SHLX
    Result = _Lshl(Size, Src, Shift);
  } else {
    // SHRX
    Result = _Lshr(Size, Src, Shift);
  }

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::BZHI(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  const auto OperandSize = IR::OpSizeAsBits(Size);

  // In 32-bit mode we only look at bottom 32-bit, no 8 or 16-bit BZHI so no
  // need to zero-extend sources
  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  auto* Index = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  // Clear the high bits specified by the index. A64 only considers bottom bits
  // of the shift, so we don't need to mask bottom 8-bits ourselves.
  // Out-of-bounds results ignored after.
  auto Mask = _Lshl(Size, Constant(-1), Index);
  auto MaskResult = _Andn(Size, Src, Mask);

  // If the index is above OperandSize, we don't clear anything. BZHI only
  // considers the bottom 8-bits, so we really want to know if the bottom 8-bits
  // have their top bits set. Test exactly that.
  //
  // Because we're clobbering flags internally we ignore all carry invert
  // shenanigans and use the raw versions here.
  _TestNZ(OpSize::i64Bit, Index, Constant(0xFF & ~(OperandSize - 1)));
  auto Result = _NZCVSelect(Size, {COND_NEQ}, Src, MaskResult);
  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);

  auto CFInv = _NZCVSelect01({COND_EQ});

  InvalidatePF_AF();
  SetNZ_ZeroCV(Size, Result);
  SetCFInverted(CFInv);
}

void OpDispatchBuilder::RORX(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto SrcSizeBits = IR::OpSizeAsBits(SrcSize);
  const auto Amount = Op->Src[1].Literal() & (SrcSizeBits - 1);
  const auto GPRSize = GetGPROpSize();

  const auto DoRotation = Amount != 0 && Amount < SrcSizeBits;
  const auto IsSameGPR = Op->Src[0].IsGPR() && Op->Dest.IsGPR() && Op->Src[0].Data.GPR.GPR == Op->Dest.Data.GPR.GPR;
  const auto SrcSizeIsGPRSize = SrcSize == GPRSize;

  // If we don't need to rotate and our source is the same as the destination
  // then we don't need to do anything at all. We still need to be careful,
  // since 32-bit operations on 64-bit mode still need to zero-extend the
  // destination register. So also compare source size and GPR size.
  //
  // Very unlikely, but hey, we can do nothing faster.
  if (!DoRotation && IsSameGPR && SrcSizeIsGPRSize) [[unlikely]] {
    return;
  }

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Result = Src;
  if (DoRotation) [[likely]] {
    Result = _Ror(OpSizeFromSrc(Op), Src, _InlineConstant(Amount));
  }

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::MULX(OpcodeArgs) {
  // RDX is the implied source operand in the instruction
  const auto OpSize = OpSizeFromSrc(Op);

  // Src1 can be a memory operand, so ensure we constrain to the
  // absolute width of the access in that scenario.
  const auto GPRSize = GetGPROpSize();
  const auto Src1Size = Op->Src[1].IsGPR() ? GPRSize : OpSize;

  Ref Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], Src1Size, Op->Flags);
  Ref Src2 = LoadGPRRegister(X86State::REG_RDX, GPRSize);

  // As per the Intel Software Development Manual, if the destination and
  // first operand correspond to the same register, then the result
  // will be the high half of the multiplication result.
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Ref ResultHi = _UMulH(OpSize, Src1, Src2);
    StoreResult(GPRClass, Op, Op->Dest, ResultHi, OpSize::iInvalid);
  } else {
    Ref ResultLo = _UMul(OpSize, Src1, Src2);
    Ref ResultHi = _UMulH(OpSize, Src1, Src2);

    StoreResult(GPRClass, Op, Op->Src[0], ResultLo, OpSize::iInvalid);
    StoreResult(GPRClass, Op, Op->Dest, ResultHi, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::PDEP(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Input = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Mask = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _PDep(OpSizeFromSrc(Op), Input, Mask);

  StoreResult(GPRClass, Op, Op->Dest, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PEXT(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Input = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Mask = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _PExt(OpSizeFromSrc(Op), Input, Mask);

  StoreResult(GPRClass, Op, Op->Dest, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::ADXOp(OpcodeArgs) {
  const auto OpSize = OpSizeFromSrc(Op);

  // Only 32/64-bit anyway so allow garbage, we use 32-bit ops.
  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

  // Handles ADCX and ADOX
  const bool IsADCX = Op->OP == 0x1F6;
  auto Zero = Constant(0);

  // Before we go trashing NZCV, save the current NZCV state.
  Ref OldNZCV = GetNZCV();

  // We want to use arm64 adc. For ADOX, copy the overflow flag into CF.  For
  // ADCX, we just rectify the carry.
  if (IsADCX) {
    RectifyCarryInvert(false);
  } else {
    // If overflow, 0 - 0 sets carry. Else, forces carry to 0.
    _CondSubNZCV(OpSize::i32Bit, Zero, Zero, {COND_FU}, 0x0 /* nzcv */);
  }

  // Do the actual add.
  HandleNZCV_RMW();
  auto Result = _AdcWithFlags(OpSize, Src, Before);
  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);

  // Now restore all flags except the one we're updating.
  if (CTX->HostFeatures.SupportsFlagM) {
    // For ADOX, we need to copy the new carry into the overflow flag. If carry is clear (ULT with uninverted
    // carry), 0 - 0 clears overflow. Else, force overflow on.
    if (!IsADCX) {
      _CondSubNZCV(OpSize::i32Bit, Zero, Zero, {COND_ULT}, 0x1 /* nzcV */);
    }

    _RmifNZCV(OldNZCV, 28, IsADCX ? 0xd /* NzcV */ : 0xe /* NZCv */);
  } else {
    // For either operation, insert the new flag into the old NZCV.
    bool SavedCFInvert = CFInverted;
    CFInverted = false;
    Ref OutputCF = GetRFLAG(X86State::RFLAG_CF_RAW_LOC, IsADCX);
    CFInverted = IsADCX ? true : SavedCFInvert;

    Ref NewNZCV = _Bfi(OpSize::i32Bit, 1, IsADCX ? 29 : 28, OldNZCV, OutputCF);
    SetNZCV(NewNZCV);
  }
}

void OpDispatchBuilder::RCROp1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  // We expliclty mask for <32-bit so allow garbage
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  const auto Size = GetSrcBitSize(Op);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  Ref Res;

  // Our new CF will be bit 0 of the source. Set upfront to avoid a move.
  SetCFDirect(Dest, 0, true);

  uint32_t Shift = 1;

  if (Size == 32 || Size == 64) {
    // Rotate and insert CF in the upper bit
    Res = _Extr(OpSizeFromSrc(Op), CF, Dest, Shift);
  } else {
    // Res = Src >> Shift
    Res = _Bfe(OpSize::i32Bit, Size - Shift, Shift, Dest);

    // inject the CF
    Res = _Orlshl(OpSize::i32Bit, Res, CF, Size - Shift);
  }

  StoreResult(GPRClass, Op, Res, OpSize::iInvalid);

  // OF is the top two MSBs XOR'd together
  // Only when Shift == 1, it is undefined otherwise
  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(_XorShift(OpSize::i64Bit, Res, Res, ShiftType::LSR, 1), Size - 2, true);
}

void OpDispatchBuilder::RCROp8x1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  const auto SizeBit = GetSrcBitSize(Op);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  // Our new CF will be bit (Shift - 1) of the source
  SetCFDirect(Dest, 0, true);

  // Rotate and insert CF in the upper bit
  Ref Res = _Bfe(OpSize::i32Bit, 7, 1, Dest);
  Res = _Bfi(OpSize::i32Bit, 1, 7, Res, CF);

  StoreResult(GPRClass, Op, Res, OpSize::iInvalid);

  // OF is the top two MSBs XOR'd together
  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(_XorShift(OpSize::i32Bit, Res, Res, ShiftType::LSR, 1), SizeBit - 2, true);
}

void OpDispatchBuilder::RCROp(OpcodeArgs) {
  const auto Size = GetSrcBitSize(Op);

  if (Size == 8 || Size == 16) {
    RCRSmallerOp(Op);
    return;
  }

  const auto Mask = (Size == 64) ? 0x3F : 0x1F;

  // Calculate flags early.
  CalculateDeferredFlags();
  const auto OpSize = OpSizeFromSrc(Op);

  Ref Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  uint64_t Const;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    Const &= Mask;
    if (!Const) {
      ZeroShiftResult(Op);
      return;
    }

    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    // Res = Src >> Shift
    Ref Res = _Lshr(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    // Constant folded version of the above, with fused shifts.
    if (Const > 1) {
      Res = _Orlshl(OpSize, Res, Dest, Size + 1 - Const);
    }

    // Our new CF will be bit (Shift - 1) of the source.
    SetCFDirect(Dest, Const - 1, true);

    // Since shift != 0 we can inject the CF
    Res = _Orlshl(OpSize, Res, CF, Size - Const);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (Const == 1) {
      auto Xor = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, Size - 2, true);
    }

    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
    return;
  }

  Ref SrcMasked = _And(OpSize, Src, _InlineConstant(Mask));
  Calculate_ShiftVariable(
    Op, SrcMasked,
    [this, Op, Size, OpSize]() {
      // Rematerialize loads to avoid crossblock liveness
      Ref Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
      Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

      // Res = Src >> Shift
      Ref Res = _Lshr(OpSize, Dest, Src);
      auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

      // Res |= (Dest << (Size - Shift + 1));
      // Expressed as Res | ((Src << (Size - Shift)) << 1) to get correct
      // behaviour for Shift without clobbering NZCV. Then observe that modulo
      // Size, Size - Shift = -Shift so we can use a simple Neg.
      //
      // The masking of Lshl means we don't need mask the source, since:
      //
      //  -(x & Mask) & Mask = (-x) & Mask
      Ref NegSrc = _Neg(OpSize, Src);
      Res = _Orlshl(OpSize, Res, _Lshl(OpSize, Dest, NegSrc), 1);

      // Our new CF will be bit (Shift - 1) of the source. this is hoisted up to
      // avoid the need to copy the source. Again, the Lshr absorbs the masking.
      auto NewCF = _Lshr(OpSize, Dest, Sub(OpSize, Src, 1));
      SetCFDirect(NewCF, 0, true);

      // Since shift != 0 we can inject the CF
      Res = _Or(OpSize, Res, _Lshl(OpSize, CF, NegSrc));

      // OF is the top two MSBs XOR'd together
      // Only when Shift == 1, it is undefined otherwise
      auto Xor = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, Size - 2, true);

      StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
    },
    OpSizeFromSrc(Op) == OpSize::i32Bit ? std::make_optional(&OpDispatchBuilder::ZeroShiftResult) : std::nullopt);
}

void OpDispatchBuilder::RCRSmallerOp(OpcodeArgs) {
  CalculateDeferredFlags();

  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  auto Src = ARef(LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true}));
  Src = Src.And(0x1F);

  // CF only changes if we actually shifted. OF undefined if we didn't shift.
  // The result is unchanged if we didn't shift. So branch over the whole thing.
  Calculate_ShiftVariable(Op, Src.Ref(), [this, Op, Size]() {
    // Rematerialized to avoid crossblock liveness
    auto Src = ARef(LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true}));
    Src = Src.And(0x1F);

    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
    Ref Tmp {};

    // Insert the incoming value across the temporary 64bit source
    // Make sure to insert at <BitSize> + 1 offsets
    // We need to cover 32bits plus the amount that could rotate in

    if (Size == 8) {
      // 8-bit optimal cascade
      // Cascade: 0
      //   Data: -> [7:0]
      //   CF:   -> [8:8]
      // Cascade: 1
      //   Data: -> [16:9]
      //   CF:   -> [17:17]
      // Cascade: 2
      //   Data: -> [25:18]
      //   CF:   -> [26:26]
      // Cascade: 3
      //   Data: -> [34:27]
      //   CF:   -> [35:35]
      // Cascade: 4
      //   Data: -> [43:36]
      //   CF:   -> [44:44]

      // Insert CF, Destination already at [7:0]
      Tmp = _Bfi(OpSize::i64Bit, 1, 8, Dest, CF);

      // First Cascade, copies 9 bits from itself.
      Tmp = _Bfi(OpSize::i64Bit, 9, 9, Tmp, Tmp);

      // Second cascade, copies 18 bits from itself.
      Tmp = _Bfi(OpSize::i64Bit, 18, 18, Tmp, Tmp);

      // Final cascade, copies 9 bits again from itself.
      Tmp = _Bfi(OpSize::i64Bit, 9, 36, Tmp, Tmp);
    } else {
      // 16-bit optimal cascade
      // Cascade: 0
      //   Data: -> [15:0]
      //   CF:   -> [16:16]
      // Cascade: 1
      //   Data: -> [32:17]
      //   CF:   -> [33:33]
      // Cascade: 2
      //   Data: -> [49:34]
      //   CF:   -> [50:50]

      // Insert CF, Destination already at [15:0]
      Tmp = _Bfi(OpSize::i64Bit, 1, 16, Dest, CF);

      // First Cascade, copies 17 bits from itself.
      Tmp = _Bfi(OpSize::i64Bit, 17, 17, Tmp, Tmp);

      // Final Cascade, copies 17 bits from itself again.
      Tmp = _Bfi(OpSize::i64Bit, 17, 34, Tmp, Tmp);
    }

    // Entire bitfield has been setup. Just extract the 8 or 16bits we need.
    // 64-bit shift used because we want to rotate in our cascaded upper bits
    // rather than zeroes.
    Ref Res = _Lshr(OpSize::i64Bit, Tmp, Src.Ref());

    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);

    // Our new CF will be bit (Shift - 1) of the source. 32-bit Lshr masks the
    // same as x86, but if we constant fold we must mask ourselves.
    if (Src.IsConstant) {
      SetCFDirect(Tmp, (Src.C & 0x1f) - 1, true);
    } else {
      auto NewCF = _Lshr(OpSize::i32Bit, Tmp, Sub(OpSize::i32Bit, Src.Ref(), 1));
      SetCFDirect(NewCF, 0, true);
    }

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (!Src.IsConstant || Src.C == 1) {
      auto NewOF = _XorShift(OpSize::i32Bit, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 2, true);
    }
  });
}

void OpDispatchBuilder::RCLOp1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  const auto Size = GetSrcBitSize(Op);
  const auto OpSize = Size == 64 ? OpSize::i64Bit : OpSize::i32Bit;
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  // Rotate left and insert CF in to lowest bit
  // TODO: Use `adc Res, xzr, Dest, lsl 1` to save an instruction
  Ref Res = _Orlshl(OpSize, CF, Dest, 1);

  // Our new CF will be the top bit of the source
  SetCFDirect(Dest, Size - 1, true);

  // OF is the top two MSBs XOR'd together
  // Top two MSBs is CF and top bit of result
  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(_Xor(OpSize, Res, Dest), Size - 1, true);

  StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
}

void OpDispatchBuilder::RCLOp(OpcodeArgs) {
  const auto Size = GetSrcBitSize(Op);

  if (Size == 8 || Size == 16) {
    RCLSmallerOp(Op);
    return;
  }

  const auto Mask = (Size == 64) ? 0x3F : 0x1F;

  // Calculate flags early.
  CalculateDeferredFlags();

  Ref Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  const auto OpSize = OpSizeFromSrc(Op);

  uint64_t Const;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    Const &= Mask;
    if (!Const) {
      ZeroShiftResult(Op);
      return;
    }

    // Res = Src << Shift
    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
    Ref Res = _Lshl(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    // Res |= (Src << (Size - Shift + 1));
    if (Const > 1) {
      Res = _Orlshr(OpSize, Res, Dest, Size + 1 - Const);
    }

    // Our new CF will be bit (Shift - 1) of the source
    SetCFDirect(Dest, Size - Const, true);

    // Since Shift != 0 we can inject the CF
    Res = _Orlshl(OpSize, Res, CF, Const - 1);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (Const == 1) {
      auto NewOF = _Xor(OpSize, Res, Dest);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 1, true);
    }

    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
    return;
  }

  Ref SrcMasked = _And(OpSize, Src, _InlineConstant(Mask));
  Calculate_ShiftVariable(
    Op, SrcMasked,
    [this, Op, Size, OpSize]() {
      // Rematerialized to avoid crossblock liveness
      Ref Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

      // Res = Src << Shift
      Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
      Ref Res = _Lshl(OpSize, Dest, Src);
      auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

      // Res |= (Dest >> (Size - Shift + 1)), expressed as
      // Res | ((Dest >> (-Shift)) >> 1), since Size - Shift = -Shift mod
      // Size. The shift aborbs the masking.
      auto NegSrc = _Neg(OpSize, Src);
      Res = _Orlshr(OpSize, Res, _Lshr(OpSize, Dest, NegSrc), 1);

      // Our new CF will be bit (Shift - 1) of the source
      auto NewCF = _Lshr(OpSize, Dest, NegSrc);
      SetCFDirect(NewCF, 0, true);

      // Since Shift != 0 we can inject the CF. Shift absorbs the masking.
      Ref CFShl = Sub(OpSize, Src, 1);
      auto TmpCF = _Lshl(OpSize, CF, CFShl);
      Res = _Or(OpSize, Res, TmpCF);

      // OF is the top two MSBs XOR'd together
      // Only when Shift == 1, it is undefined otherwise
      //
      // Note that NewCF has garbage in the upper bits, but we ignore them here
      // and mask as part of the set after.
      auto NewOF = _XorShift(OpSize, Res, NewCF, ShiftType::LSL, Size - 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 1, true);

      StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
    },
    OpSizeFromSrc(Op) == OpSize::i32Bit ? std::make_optional(&OpDispatchBuilder::ZeroShiftResult) : std::nullopt);
}

void OpDispatchBuilder::RCLSmallerOp(OpcodeArgs) {
  CalculateDeferredFlags();

  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  auto Src = ARef(LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true}));
  Src = Src.And(0x1F);

  // CF only changes if we actually shifted. OF undefined if we didn't shift.
  // The result is unchanged if we didn't shift. So branch over the whole thing.
  Calculate_ShiftVariable(Op, Src.Ref(), [this, Op, Size]() {
    // Rematerialized to avoid crossblock liveness
    auto Src = ARef(LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true}));
    Src = Src.And(0x1F);
    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    Ref Tmp = Constant(0);

    for (size_t i = 0; i < (32 + Size + 1); i += (Size + 1)) {
      // Insert incoming value
      Tmp = _Bfi(OpSize::i64Bit, Size, 63 - i - Size, Tmp, Dest);

      // Insert CF
      Tmp = _Bfi(OpSize::i64Bit, 1, 63 - i, Tmp, CF);
    }

    // Insert incoming value
    Tmp = _Bfi(OpSize::i64Bit, Size, 0, Tmp, Dest);

    // The data is now set up like this
    // [Data][CF]:[Data][CF]:[Data][CF]:[Data][CF]
    // Shift 1 more bit that expected to get our result
    // Shifting to the right will now behave like a rotate to the left
    // Which we emulate with a _Ror
    Ref Res = _Ror(OpSize::i64Bit, Tmp, Src.Neg().Ref());

    StoreResult(GPRClass, Op, Res, OpSize::iInvalid);

    // Our new CF is now at the bit position that we are shifting
    // Either 0 if CF hasn't changed (CF is living in bit 0)
    // or higher
    auto NewCF = _Ror(OpSize::i64Bit, Tmp, Src.Presub(63).Ref());
    SetCFDirect(NewCF, 0, true);

    // OF is the XOR of the NewCF and the MSB of the result
    // Only defined for 1-bit rotates.
    if (!Src.IsConstant || Src.C == 1) {
      auto NewOF = _XorShift(OpSize::i64Bit, NewCF, Res, ShiftType::LSR, Size - 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, 0, true);
    }
  });
}

void OpDispatchBuilder::BTOp(OpcodeArgs, uint32_t SrcIndex, BTAction Action) {
  Ref Value;
  ArithRef Src;
  bool IsNonconstant = Op->Src[SrcIndex].IsGPR();

  const uint32_t Size = GetDstBitSize(Op);
  const uint32_t Mask = Size - 1;

  if (IsNonconstant) {
    // Because we mask explicitly with And/Bfe/Sbfe after, we can allow garbage here.
    Src = ARef(LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true}));
  } else {
    // Can only be an immediate
    // Masked by operand size
    Src = ARef(Op->Src[SrcIndex].Literal() & Mask);
  }

  if (Op->Dest.IsGPR()) {
    // When the destination is a GPR, we don't care about garbage in the upper bits.
    // Load the full register.
    auto Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GetGPROpSize(), Op->Flags);
    Value = Dest;

    // Get the bit selection from the src. We need to mask for 8/16-bit, but
    // rely on the implicit masking of Lshr for native sizes.
    unsigned LshrSize = std::max<uint8_t>(IR::OpSizeToSize(OpSize::i32Bit), Size / 8);
    auto BitSelect = (Size == (LshrSize * 8)) ? Src : Src.And(Mask);
    auto LshrOpSize = IR::SizeToOpSize(LshrSize);

    // OF/SF/AF/PF undefined. ZF must be preserved. We choose to preserve OF/SF
    // too since we just use an rmif to insert into CF directly. We could
    // optimize perhaps.
    //
    // Set CF before the action to save a move, except for complements where we
    // can reuse the invert.
    if (Action != BTAction::BTComplement) {
      if (IsNonconstant) {
        Value = _Lshr(IR::SizeToOpSize(LshrSize), Value, BitSelect.Ref());
      }

      SetRFLAG(Value, X86State::RFLAG_CF_RAW_LOC, Src.IsConstant ? Src.C : 0, true);
      CFInverted = false;
    }

    switch (Action) {
    case BTAction::BTNone: {
      /* Nothing to do */
      break;
    }

    case BTAction::BTClear: {
      Dest = _Andn(LshrOpSize, Dest, BitSelect.MaskBit(LshrOpSize).Ref());
      StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
      break;
    }

    case BTAction::BTSet: {
      Dest = _Or(LshrOpSize, Dest, BitSelect.MaskBit(LshrOpSize).Ref());
      StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
      break;
    }

    case BTAction::BTComplement: {
      Dest = _Xor(LshrOpSize, Dest, BitSelect.MaskBit(LshrOpSize).Ref());

      if (IsNonconstant) {
        Value = _Lshr(LshrOpSize, Dest, BitSelect.Ref());
      } else {
        Value = Dest;
      }

      SetRFLAG(Value, X86State::RFLAG_CF_RAW_LOC, Src.IsConstant ? Src.C : 0, true);
      CFInverted = true;

      StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
      break;
    }
    }
  } else {
    // Load the address to the memory location
    Ref Dest = MakeSegmentAddress(Op, Op->Dest);
    // Get the bit selection from the src
    auto BitSelect = Src.Bfe(0, 3);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = Src.Sbfe(3, Size - 3);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op
    AddressMode Address = {.Base = Dest, .Index = Src.Ref(), .AddrSize = OpSize::i64Bit};

    switch (Action) {
    case BTAction::BTNone: {
      Value = _LoadMemAutoTSO(GPRClass, OpSize::i8Bit, Address, OpSize::i8Bit);
      break;
    }

    case BTAction::BTClear: {
      Ref BitMask = BitSelect.MaskBit(OpSize::i64Bit).Ref();

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchCLR(OpSize::i8Bit, BitMask, LoadEffectiveAddress(this, Address, GetGPROpSize(), true));
      } else {
        Value = _LoadMemAutoTSO(GPRClass, OpSize::i8Bit, Address, OpSize::i8Bit);

        auto Modified = _Andn(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, OpSize::i8Bit, Address, Modified, OpSize::i8Bit);
      }
      break;
    }

    case BTAction::BTSet: {
      Ref BitMask = BitSelect.MaskBit(OpSize::i64Bit).Ref();

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchOr(OpSize::i8Bit, BitMask, LoadEffectiveAddress(this, Address, GetGPROpSize(), true));
      } else {
        Value = _LoadMemAutoTSO(GPRClass, OpSize::i8Bit, Address, OpSize::i8Bit);

        auto Modified = _Or(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, OpSize::i8Bit, Address, Modified, OpSize::i8Bit);
      }
      break;
    }

    case BTAction::BTComplement: {
      Ref BitMask = BitSelect.MaskBit(OpSize::i64Bit).Ref();

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchXor(OpSize::i8Bit, BitMask, LoadEffectiveAddress(this, Address, GetGPROpSize(), true));
      } else {
        Value = _LoadMemAutoTSO(GPRClass, OpSize::i8Bit, Address, OpSize::i8Bit);

        auto Modified = _Xor(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, OpSize::i8Bit, Address, Modified, OpSize::i8Bit);
      }
      break;
    }
    }

    // Now shift in to the correct bit location
    if (!BitSelect.IsDefinitelyZero()) {
      Value = _Lshr(std::max(OpSize::i32Bit, GetOpSize(Value)), Value, BitSelect.Ref());
    }

    // OF/SF/ZF/AF/PF undefined.
    SetCFDirect(Value, 0, true);
  }
}

void OpDispatchBuilder::IMUL1SrcOp(OpcodeArgs) {
  /* We're just going to sign-extend the non-garbage anyway.. */
  Ref Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  Ref Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = OpSizeFromSrc(Op);
  const auto SizeBits = IR::OpSizeAsBits(Size);

  Ref Dest {};
  Ref ResultHigh {};
  switch (Size) {
  case OpSize::i8Bit:
  case OpSize::i16Bit: {
    Src1 = _Sbfe(OpSize::i64Bit, SizeBits, 0, Src1);
    Src2 = _Sbfe(OpSize::i64Bit, SizeBits, 0, Src2);
    Dest = _Mul(OpSize::i64Bit, Src1, Src2);
    ResultHigh = _Sbfe(OpSize::i64Bit, SizeBits, SizeBits, Dest);
    break;
  }
  case OpSize::i32Bit: {
    ResultHigh = _SMull(Src1, Src2);
    ResultHigh = _Sbfe(OpSize::i64Bit, SizeBits, SizeBits, ResultHigh);
    // Flipped order to save a move
    Dest = _Mul(OpSize::i32Bit, Src1, Src2);
    break;
  }
  case OpSize::i64Bit: {
    ResultHigh = _MulH(OpSize::i64Bit, Src1, Src2);
    // Flipped order to save a move
    Dest = _Mul(OpSize::i64Bit, Src1, Src2);
    break;
  }
  default: FEX_UNREACHABLE;
  }

  StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
  CalculateFlags_MUL(Size, Dest, ResultHigh);
}

void OpDispatchBuilder::IMUL2SrcOp(OpcodeArgs) {
  Ref Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  Ref Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = OpSizeFromSrc(Op);
  const auto SizeBits = IR::OpSizeAsBits(Size);

  Ref Dest {};
  Ref ResultHigh {};

  switch (Size) {
  case OpSize::i8Bit:
  case OpSize::i16Bit: {
    Src1 = _Sbfe(OpSize::i64Bit, SizeBits, 0, Src1);
    Src2 = ARef(Src2).Sbfe(0, SizeBits).Ref();
    Dest = _Mul(OpSize::i64Bit, Src1, Src2);
    ResultHigh = _Sbfe(OpSize::i64Bit, SizeBits, SizeBits, Dest);
    break;
  }
  case OpSize::i32Bit: {
    ResultHigh = _SMull(Src1, Src2);
    ResultHigh = _Sbfe(OpSize::i64Bit, SizeBits, SizeBits, ResultHigh);
    // Flipped order to save a move
    Dest = _Mul(OpSize::i32Bit, Src1, Src2);
    break;
  }
  case OpSize::i64Bit: {
    ResultHigh = _MulH(OpSize::i64Bit, Src1, Src2);
    // Flipped order to save a move
    Dest = _Mul(OpSize::i64Bit, Src1, Src2);
    break;
  }
  default: FEX_UNREACHABLE;
  }

  StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
  CalculateFlags_MUL(Size, Dest, ResultHigh);
}

void OpDispatchBuilder::IMULOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  const auto SizeBits = IR::OpSizeAsBits(Size);

  Ref Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  Ref Src2 = LoadGPRRegister(X86State::REG_RAX);

  if (Size != OpSize::i64Bit) {
    Src1 = _Sbfe(OpSize::i64Bit, SizeBits, 0, Src1);
    Src2 = _Sbfe(OpSize::i64Bit, SizeBits, 0, Src2);
  }

  // 64-bit special cased to save a move
  Ref Result = Size < OpSize::i64Bit ? _Mul(OpSize::i64Bit, Src1, Src2) : nullptr;
  Ref ResultHigh {};
  if (Size == OpSize::i8Bit) {
    // Result is stored in AX
    StoreGPRRegister(X86State::REG_RAX, Result, OpSize::i16Bit);
    ResultHigh = _Sbfe(OpSize::i64Bit, 8, 8, Result);
  } else if (Size == OpSize::i16Bit) {
    // 16bits stored in AX
    // 16bits stored in DX
    StoreGPRRegister(X86State::REG_RAX, Result, Size);
    ResultHigh = _Sbfe(OpSize::i64Bit, 16, 16, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh, Size);
  } else if (Size == OpSize::i32Bit) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    auto LocalResult = _Bfe(OpSize::i64Bit, 32, 0, Result);
    auto LocalResultHigh = _Bfe(OpSize::i64Bit, 32, 32, Result);
    ResultHigh = _Sbfe(OpSize::i64Bit, 32, 32, Result);
    Result = _Sbfe(OpSize::i64Bit, 32, 0, Result);
    StoreGPRRegister(X86State::REG_RAX, LocalResult);
    StoreGPRRegister(X86State::REG_RDX, LocalResultHigh);
  } else if (Size == OpSize::i64Bit) {
    if (!Is64BitMode) {
      LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _MulH(OpSize::i64Bit, Src1, Src2);
    Result = _Mul(OpSize::i64Bit, Src1, Src2);
    StoreGPRRegister(X86State::REG_RAX, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh);
  }

  CalculateFlags_MUL(Size, Result, ResultHigh);
}

void OpDispatchBuilder::MULOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  const auto SizeBits = IR::OpSizeAsBits(Size);

  Ref Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  Ref Src2 = LoadGPRRegister(X86State::REG_RAX);
  Ref Result {};

  if (Size != OpSize::i64Bit) {
    Src1 = _Bfe(OpSize::i64Bit, SizeBits, 0, Src1);
    Src2 = _Bfe(OpSize::i64Bit, SizeBits, 0, Src2);
    Result = _UMul(OpSize::i64Bit, Src1, Src2);
  }
  Ref ResultHigh {};

  if (Size == OpSize::i8Bit) {
    // Result is stored in AX
    StoreGPRRegister(X86State::REG_RAX, Result, OpSize::i16Bit);
    ResultHigh = _Bfe(OpSize::i64Bit, 8, 8, Result);
  } else if (Size == OpSize::i16Bit) {
    // 16bits stored in AX
    // 16bits stored in DX
    StoreGPRRegister(X86State::REG_RAX, Result, Size);
    ResultHigh = _Bfe(OpSize::i64Bit, 16, 16, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh, Size);
  } else if (Size == OpSize::i32Bit) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    Ref ResultLow = _Bfe(OpSize::i64Bit, 32, 0, Result);
    ResultHigh = _Bfe(OpSize::i64Bit, 32, 32, Result);
    StoreGPRRegister(X86State::REG_RAX, ResultLow);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh);
  } else if (Size == OpSize::i64Bit) {
    if (!Is64BitMode) {
      LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    // 64bits stored in RAX
    // 64bits stored in RDX
    //
    // Calculate high first to allow better RA.
    ResultHigh = _UMulH(OpSize::i64Bit, Src1, Src2);
    Result = _UMul(OpSize::i64Bit, Src1, Src2);
    StoreGPRRegister(X86State::REG_RAX, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh);
  }

  CalculateFlags_UMUL(ResultHigh);
}

void OpDispatchBuilder::NOTOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  const auto SizeBits = IR::OpSizeAsBits(Size);
  LOGMAN_THROW_A_FMT(Size >= IR::OpSize::i8Bit && Size <= IR::OpSize::i64Bit, "Invalid size");

  Ref MaskConst {};
  if (Size == OpSize::i64Bit) {
    MaskConst = Constant(~0ULL);
  } else {
    MaskConst = Constant((1ULL << SizeBits) - 1);
  }

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    _AtomicXor(Size, MaskConst, DestMem);
  } else if (!Op->Dest.IsGPR()) {
    // GPR version plays fast and loose with sizes, be safe for memory tho.
    Ref Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
    Src = _Xor(OpSize::i64Bit, Src, MaskConst);
    StoreResult(GPRClass, Op, Src, OpSize::iInvalid);
  } else {
    // Specially handle high bits so we can invert in place with the correct
    // mask and a larger type.
    auto Dest = Op->Dest;
    if (Dest.Data.GPR.HighBits) {
      LOGMAN_THROW_A_FMT(Size == OpSize::i8Bit, "Only 8-bit GPRs get high bits");
      MaskConst = Constant(0xFF00);
      Dest.Data.GPR.HighBits = false;
    }

    // Always load full size, we explicitly want the upper bits to get the
    // insert behaviour for free/implicitly.
    const auto GPRSize = GetGPROpSize();
    Ref Src = LoadSource_WithOpSize(GPRClass, Op, Dest, GPRSize, Op->Flags);

    // For 8/16-bit, use 64-bit invert so we invert in place, while getting
    // insert behaviour. For 32-bit, use 32-bit invert to zero the upper bits.
    const auto EffectiveSize = Size == OpSize::i32Bit ? OpSize::i32Bit : GPRSize;

    // If we're inverting the whole thing, use Not instead of Xor to save a constant.
    if (Size >= OpSize::i32Bit) {
      Src = _Not(EffectiveSize, Src);
    } else {
      Src = _Xor(EffectiveSize, Src, MaskConst);
    }

    // Always store 64-bit, the Not/Xor correctly handle the upper bits and this
    // way we can delete the store.
    StoreResult_WithOpSize(GPRClass, Op, Dest, Src, GPRSize, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::XADDOp(OpcodeArgs) {
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result;

  if (Op->Dest.IsGPR()) {
    // If this is a GPR then we can just do an Add
    Result = CalculateFlags_ADD(OpSizeFromSrc(Op), Dest, Src);

    // Previous value in dest gets stored in src
    StoreResult(GPRClass, Op, Op->Src[0], Dest, OpSize::iInvalid);

    // Calculated value gets stored in dst (order is important if dst is same as src)
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  } else {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    auto Before = _AtomicFetchAdd(OpSizeFromSrc(Op), Src, Dest);
    CalculateFlags_ADD(OpSizeFromSrc(Op), Before, Src);
    StoreResult(GPRClass, Op, Op->Src[0], Before, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::PopcountOp(OpcodeArgs) {
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = CTX->HostFeatures.SupportsCSSC || GetSrcSize(Op) >= 4});
  Src = _Popcount(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Src, OpSize::iInvalid);

  // We need to set ZF while clearing the rest of NZCV. The result of a popcount
  // is in the range [0, 63]. In particular, it is always positive. So a
  // combined NZ test will correctly zero SF/CF/OF while setting ZF.
  SetNZ_ZeroCV(OpSize::i32Bit, Src);
  ZeroPF_AF();
}

Ref OpDispatchBuilder::CalculateAFForDecimal(Ref A) {
  auto Nibble = _And(OpSize::i64Bit, A, Constant(0xF));
  auto Greater = Select01(OpSize::i64Bit, CondClassType {COND_UGT}, Nibble, Constant(9));

  return _Or(OpSize::i64Bit, LoadAF(), Greater);
}

void OpDispatchBuilder::DAAOp(OpcodeArgs) {
  CalculateDeferredFlags();
  auto AL = LoadGPRRegister(X86State::REG_RAX, OpSize::i8Bit);
  auto CFInv = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC, true);
  auto AF = CalculateAFForDecimal(AL);

  // CF |= (AL > 0x99);
  CFInv = _And(OpSize::i64Bit, CFInv, Select01(OpSize::i64Bit, CondClassType {COND_ULE}, AL, Constant(0x99)));

  // AL = AF ? (AL + 0x6) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, AF, Constant(0), Add(OpSize::i64Bit, AL, 0x6), AL);

  // AL = CF ? (AL + 0x60) : AL;
  AL = _Select(FEXCore::IR::COND_EQ, CFInv, Constant(0), Add(OpSize::i64Bit, AL, 0x60), AL);

  // SF, ZF, PF set according to result. CF set per above. OF undefined.
  StoreGPRRegister(X86State::REG_RAX, AL, OpSize::i8Bit);
  SetNZ_ZeroCV(OpSize::i8Bit, AL);
  SetCFInverted(CFInv);
  CalculatePF(AL);
  SetAFAndFixup(AF);
}

void OpDispatchBuilder::DASOp(OpcodeArgs) {
  CalculateDeferredFlags();
  auto AL = LoadGPRRegister(X86State::REG_RAX, OpSize::i8Bit);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  auto AF = CalculateAFForDecimal(AL);

  // CF |= (AL > 0x99);
  CF = _Or(OpSize::i64Bit, CF, Select01(OpSize::i64Bit, CondClassType {COND_UGT}, AL, Constant(0x99)));

  // NewCF = CF | (AF && (Borrow from AL - 6))
  auto NewCF = _Or(OpSize::i32Bit, CF, _Select(FEXCore::IR::COND_ULT, AL, Constant(6), AF, CF));

  // AL = AF ? (AL - 0x6) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, AF, Constant(0), Sub(OpSize::i64Bit, AL, 0x6), AL);

  // AL = CF ? (AL - 0x60) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, CF, Constant(0), Sub(OpSize::i64Bit, AL, 0x60), AL);

  // SF, ZF, PF set according to result. CF set per above. OF undefined.
  StoreGPRRegister(X86State::REG_RAX, AL, OpSize::i8Bit);
  SetNZ_ZeroCV(OpSize::i8Bit, AL);
  SetCFDirect(NewCF);
  CalculatePF(AL);
  SetAFAndFixup(AF);
}

void OpDispatchBuilder::AAAOp(OpcodeArgs) {
  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AF = CalculateAFForDecimal(A);

  // CF = AF, OF/SF/ZF/PF undefined
  SetCFDirect_InvalidateNZV(AF);
  SetAFAndFixup(AF);
  CalculateDeferredFlags();

  // AX = CF ? (AX + 0x106) : 0
  A = NZCVSelect(OpSize::i32Bit, {COND_UGE} /* CF = 1 */, Add(OpSize::i32Bit, A, 0x106), A);

  // AL = AL & 0x0F
  A = _And(OpSize::i32Bit, A, Constant(0xFF0F));
  StoreGPRRegister(X86State::REG_RAX, A, OpSize::i16Bit);
}

void OpDispatchBuilder::AASOp(OpcodeArgs) {
  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AF = CalculateAFForDecimal(A);

  // CF = AF, OF/SF/ZF/PF undefined
  SetCFDirect_InvalidateNZV(AF);
  SetAFAndFixup(AF);
  CalculateDeferredFlags();

  // AX = CF ? (AX - 0x106) : 0
  A = NZCVSelect(OpSize::i32Bit, {COND_UGE} /* CF = 1 */, Sub(OpSize::i32Bit, A, 0x106), A);

  // AL = AL & 0x0F
  A = _And(OpSize::i32Bit, A, Constant(0xFF0F));
  StoreGPRRegister(X86State::REG_RAX, A, OpSize::i16Bit);
}

void OpDispatchBuilder::AAMOp(OpcodeArgs) {
  auto AL = LoadGPRRegister(X86State::REG_RAX, OpSize::i8Bit);
  auto Imm8 = Constant(Op->Src[0].Literal() & 0xFF);
  Ref Quotient = _AllocateGPR(true);
  Ref Remainder = _AllocateGPR(true);
  _UDiv(OpSize::i64Bit, AL, Invalid(), Imm8, Quotient, Remainder);
  auto Res = _AddShift(OpSize::i64Bit, Remainder, Quotient, ShiftType::LSL, 8);
  StoreGPRRegister(X86State::REG_RAX, Res, OpSize::i16Bit);

  SetNZ_ZeroCV(OpSize::i8Bit, Res);
  CalculatePF(Res);
  InvalidateAF();
}

void OpDispatchBuilder::AADOp(OpcodeArgs) {
  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AH = _Lshr(OpSize::i32Bit, A, Constant(8));
  auto Imm8 = Constant(Op->Src[0].Literal() & 0xFF);
  auto NewAL = Add(OpSize::i64Bit, A, _Mul(OpSize::i64Bit, AH, Imm8));
  auto Result = _And(OpSize::i64Bit, NewAL, Constant(0xFF));
  StoreGPRRegister(X86State::REG_RAX, Result, OpSize::i16Bit);

  SetNZ_ZeroCV(OpSize::i8Bit, Result);
  CalculatePF(Result);
  InvalidateAF();
}

void OpDispatchBuilder::XLATOp(OpcodeArgs) {
  Ref Src = MakeSegmentAddress(X86State::REG_RBX, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);
  Ref Offset = LoadGPRRegister(X86State::REG_RAX, OpSize::i8Bit);

  AddressMode A = {.Base = Src, .Index = Offset, .AddrSize = OpSize::i64Bit};
  auto Res = _LoadMemAutoTSO(GPRClass, OpSize::i8Bit, A, OpSize::i8Bit);

  StoreGPRRegister(X86State::REG_RAX, Res, OpSize::i8Bit);
}

void OpDispatchBuilder::ReadSegmentReg(OpcodeArgs, OpDispatchBuilder::Segment Seg) {
  // 64-bit only
  // Doesn't hit the segment register optimization
  const auto Size = OpSizeFromSrc(Op);
  Ref Src {};
  if (Seg == Segment::FS) {
    Src = _LoadContext(Size, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
  } else {
    Src = _LoadContext(Size, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
  }

  StoreResult(GPRClass, Op, Src, OpSize::iInvalid);
}

void OpDispatchBuilder::WriteSegmentReg(OpcodeArgs, OpDispatchBuilder::Segment Seg) {
  // Documentation claims that the 32-bit version of this instruction inserts in to the lower 32-bits of the segment
  // This is incorrect and it instead zero extends the 32-bit value to 64-bit
  const auto Size = OpSizeFromDst(Op);
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  if (Seg == Segment::FS) {
    _StoreContext(Size, GPRClass, Src, offsetof(FEXCore::Core::CPUState, fs_cached));
  } else {
    _StoreContext(Size, GPRClass, Src, offsetof(FEXCore::Core::CPUState, gs_cached));
  }
}

void OpDispatchBuilder::EnterOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto OperandSize = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_OPERAND_SIZE) ? OpSize::i16Bit : GPRSize;
  const uint64_t Value = Op->Src[0].Literal();

  const uint16_t AllocSpace = Value & 0xFFFF;
  const uint8_t Level = (Value >> 16) & 0x1F;

  const auto PushValue = [&](IR::OpSize Size, Ref Src) -> Ref {
    auto OldSP = LoadGPRRegister(X86State::REG_RSP);
    auto NewSP = _Push(GPRSize, Size, Src, OldSP);

    // Store the new stack pointer
    StoreGPRRegister(X86State::REG_RSP, NewSP);
    return NewSP;
  };

  auto OldBP = LoadGPRRegister(X86State::REG_RBP);
  auto NewSP = PushValue(OperandSize, OldBP);
  auto temp_RBP = NewSP;

  if (Level > 0) {
    for (uint8_t i = 1; i < Level; ++i) {
      auto MemLoc = Sub(GPRSize, OldBP, i * IR::OpSizeToSize(OperandSize));
      auto Mem = _LoadMem(GPRClass, OperandSize, MemLoc, OperandSize);
      NewSP = PushValue(OperandSize, Mem);
    }
    NewSP = PushValue(OperandSize, temp_RBP);
  }
  NewSP = Sub(GPRSize, NewSP, AllocSpace);
  StoreGPRRegister(X86State::REG_RSP, NewSP);
  StoreGPRRegister(X86State::REG_RBP, temp_RBP);
}

void OpDispatchBuilder::SGDTOp(OpcodeArgs) {
  auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

  // Store an emulated value in the format of:
  // uint16_t Limit;
  // {uint32_t,uint64_t} Base;
  //
  // Limit is always 0
  // Base is always in kernel space at: 0xFFFFFFFFFFFE0000ULL
  //
  // Operand size prefix is ignored on this instruction, size purely depends on operating mode.
  uint64_t GDTAddress = 0xFFFFFFFFFFFE0000ULL;
  auto GDTStoreSize = OpSize::i64Bit;
  if (!Is64BitMode) {
    // Mask off upper bits if 32-bit result.
    GDTAddress &= ~0U;
    GDTStoreSize = OpSize::i32Bit;
  }

  _StoreMemAutoTSO(GPRClass, OpSize::i16Bit, DestAddress, Constant(0));
  _StoreMemAutoTSO(GPRClass, GDTStoreSize, AddressMode {.Base = DestAddress, .Offset = 2, .AddrSize = OpSize::i64Bit}, Constant(GDTAddress));
}

void OpDispatchBuilder::SIDTOp(OpcodeArgs) {
  auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

  // See SGDTOp, matches Linux in reported values
  uint64_t IDTAddress = 0xFFFFFE0000000000ULL;
  auto IDTStoreSize = OpSize::i64Bit;
  if (!Is64BitMode) {
    // Mask off upper bits if 32-bit result.
    IDTAddress &= ~0U;
    IDTStoreSize = OpSize::i32Bit;
  }

  _StoreMemAutoTSO(GPRClass, OpSize::i16Bit, DestAddress, Constant(0xfff));
  _StoreMemAutoTSO(GPRClass, IDTStoreSize, AddressMode {.Base = DestAddress, .Offset = 2, .AddrSize = OpSize::i64Bit}, Constant(IDTAddress));
}

void OpDispatchBuilder::SMSWOp(OpcodeArgs) {
  const bool IsMemDst = DestIsMem(Op);

  IR::OpSize DstSize {OpSize::iInvalid};
  Ref Const = Constant((1U << 31) | ///< PG - Paging
                       (0U << 30) | ///< CD - Cache Disable
                       (0U << 29) | ///< NW - Not Writethrough (Legacy, now ignored)
                       ///< [28:19] - Reserved
                       (1U << 18) | ///< AM - Alignment Mask
                       ///< 17 - Reserved
                       (1U << 16) | ///< WP - Write Protect
                       ///< [15:6] - Reserved
                       (1U << 5) | ///< NE - Numeric Error
                       (1U << 4) | ///< ET - Extension Type (Legacy, now reserved and 1)
                       (0U << 3) | ///< TS - Task Switched
                       (0U << 2) | ///< EM - Emulation
                       (1U << 1) | ///< MP - Monitor Coprocessor
                       (1U << 0)); ///< PE - Protection Enabled

  if (Is64BitMode) {
    DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST  ? OpSize::i16Bit :
              X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST ? OpSize::i64Bit :
                                                                                                                   OpSize::i32Bit;

    if (!IsMemDst && DstSize == OpSize::i32Bit) {
      // Special-case version of `smsw ebx`. This instruction does an insert in to the lower 32-bits on 64-bit hosts.
      // Override and insert.
      auto Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GetGPROpSize(), Op->Flags);
      Const = _Bfi(OpSize::i64Bit, 32, 0, Dest, Const);
      DstSize = OpSize::i64Bit;
    }
  } else {
    DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? OpSize::i16Bit : OpSize::i32Bit;
  }

  if (IsMemDst) {
    // Memory destinatino always writes only 16-bits.
    DstSize = OpSize::i16Bit;
  }

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Const, DstSize, OpSize::iInvalid);
}

OpDispatchBuilder::CycleCounterPair OpDispatchBuilder::CycleCounter(bool SelfSynchronizingLoads) {
  Ref CounterLow {};
  Ref CounterHigh {};
  auto Counter = _CycleCounter(SelfSynchronizingLoads);
  if (CTX->Config.TSCScale) {
    CounterLow = _Lshl(OpSize::i32Bit, Counter, Constant(CTX->Config.TSCScale));
    CounterHigh = _Lshr(OpSize::i64Bit, Counter, Constant(32 - CTX->Config.TSCScale));
  } else {
    CounterLow = _Bfe(OpSize::i64Bit, 32, 0, Counter);
    CounterHigh = _Bfe(OpSize::i64Bit, 32, 32, Counter);
  }

  return {
    .CounterLow = CounterLow,
    .CounterHigh = CounterHigh,
  };
}

void OpDispatchBuilder::RDTSCOp(OpcodeArgs) {
  auto Counter = CycleCounter(false);
  StoreGPRRegister(X86State::REG_RAX, Counter.CounterLow);
  StoreGPRRegister(X86State::REG_RDX, Counter.CounterHigh);
}

void OpDispatchBuilder::INCOp(OpcodeArgs) {
  Ref Dest;
  Ref Result;
  const auto Size = GetSrcBitSize(Op);
  const bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;

    Ref DestAddress = MakeSegmentAddress(Op, Op->Dest);
    Dest = _AtomicFetchAdd(OpSizeFromSrc(Op), Constant(1), DestAddress);
  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});
  }

  CalculateDeferredFlags();

  if (Size < 32 && CTX->HostFeatures.SupportsFlagM) {
    // Addition producing upper garbage
    Result = Add(OpSize::i32Bit, Dest, 1);
    CalculatePF(Result);
    CalculateAF(Dest, Constant(1));

    // Correctly set NZ flags, preserving C
    HandleNZCV_RMW();
    _SetSmallNZV(OpSizeFromSrc(Op), Result);

    // Fix up V flag. INC overflows only when incrementing a positive and
    // getting a negative. So compare the sign bits to calculate V.
    _RmifNZCV(_Andn(OpSize::i32Bit, Result, Dest), Size - 1, 1);
  } else {
    Result = CalculateFlags_ADD(OpSizeFromSrc(Op), Dest, Constant(1), false);
  }

  if (!IsLocked) {
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::DECOp(OpcodeArgs) {
  Ref Dest;
  Ref Result;
  const auto Size = GetSrcBitSize(Op);
  const bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;

    Ref DestAddress = MakeSegmentAddress(Op, Op->Dest);

    // Use Add instead of Sub to avoid a NEG
    Dest = _AtomicFetchAdd(OpSizeFromSrc(Op), Constant(Size == 64 ? -1 : ((1ULL << Size) - 1)), DestAddress);
  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});
  }

  CalculateDeferredFlags();

  if (Size < 32 && CTX->HostFeatures.SupportsFlagM) {
    // Subtraction producing upper garbage
    Result = Sub(OpSize::i32Bit, Dest, 1);
    CalculatePF(Result);
    CalculateAF(Dest, Constant(1));

    // Correctly set NZ flags, preserving C
    HandleNZCV_RMW();
    _SetSmallNZV(OpSizeFromSrc(Op), Result);

    // Fix up V flag. DEC overflows only when decrementing a negative and
    // getting a positive. So compare the sign bits to calculate V.
    _RmifNZCV(_Andn(OpSize::i32Bit, Dest, Result), Size - 1, 1);
  } else {
    Result = CalculateFlags_SUB(OpSizeFromSrc(Op), Dest, Constant(1), false);
  }

  if (!IsLocked) {
    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::STOSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto Size = OpSizeFromSrc(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) != 0;

  if (!Repeat) {
    // Src is used only for a store of the same size so allow garbage
    Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

    // Only ES prefix
    Ref Dest = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, Dest, Src, Size);

    // Offset the pointer
    Ref TailDest = LoadGPRRegister(X86State::REG_RDI);
    StoreGPRRegister(X86State::REG_RDI, OffsetByDir(TailDest, IR::OpSizeToSize(Size)));
  } else {
    // FEX doesn't support partial faulting REP instructions.
    // Converting this to a `MemSet` IR op optimizes this quite significantly in our codegen.
    // If FEX is to gain support for faulting REP instructions, then this implementation needs to change significantly.
    Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    Ref Dest = LoadGPRRegister(X86State::REG_RDI);

    // Only ES prefix
    auto Segment = GetSegment(0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    Ref Counter = LoadGPRRegister(X86State::REG_RCX);

    auto Result = _MemSet(CTX->IsAtomicTSOEnabled(), Size, Segment ?: InvalidNode, Dest, Src, Counter, LoadDir(1));
    StoreGPRRegister(X86State::REG_RCX, Constant(0));
    StoreGPRRegister(X86State::REG_RDI, Result);
  }
}

void OpDispatchBuilder::MOVSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  // RA now can handle these to be here, to avoid DF accesses
  const auto Size = OpSizeFromSrc(Op);

  if (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) {
    auto SrcAddr = LoadGPRRegister(X86State::REG_RSI);
    auto DstAddr = LoadGPRRegister(X86State::REG_RDI);
    auto Counter = LoadGPRRegister(X86State::REG_RCX);

    auto DstSegment = GetSegment(0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    auto SrcSegment = GetSegment(Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    if (DstSegment) {
      DstAddr = Add(OpSize::i64Bit, DstAddr, DstSegment);
    }

    if (SrcSegment) {
      SrcAddr = Add(OpSize::i64Bit, SrcAddr, SrcSegment);
    }

    Ref Result_Src = _AllocateGPR(false);
    Ref Result_Dst = _AllocateGPR(false);
    _MemCpy(CTX->IsAtomicTSOEnabled(), Size, DstAddr, SrcAddr, Counter, LoadDir(1), Result_Dst, Result_Src);

    if (DstSegment) {
      Result_Dst = Sub(OpSize::i64Bit, Result_Dst, DstSegment);
    }

    if (SrcSegment) {
      Result_Src = Sub(OpSize::i64Bit, Result_Src, SrcSegment);
    }

    StoreGPRRegister(X86State::REG_RCX, Constant(0));
    StoreGPRRegister(X86State::REG_RDI, Result_Dst);
    StoreGPRRegister(X86State::REG_RSI, Result_Src);
  } else {
    Ref RSI = MakeSegmentAddress(X86State::REG_RSI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);
    Ref RDI = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, RSI, Size);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, RDI, Src, Size);

    auto PtrDir = LoadDir(IR::OpSizeToSize(Size));
    RSI = Add(OpSize::i64Bit, RSI, PtrDir);
    RDI = Add(OpSize::i64Bit, RDI, PtrDir);

    StoreGPRRegister(X86State::REG_RSI, RSI);
    StoreGPRRegister(X86State::REG_RDI, RDI);
  }
}

void OpDispatchBuilder::CMPSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto Size = OpSizeFromSrc(Op);

  bool Repeat = Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX);
  if (!Repeat) {
    // Default DS prefix
    Ref Dest_RSI = MakeSegmentAddress(X86State::REG_RSI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);
    // Only ES prefix
    Ref Dest_RDI = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    CalculateFlags_SUB(OpSizeFromSrc(Op), Src2, Src1);

    auto PtrDir = LoadDir(IR::OpSizeToSize(Size));

    // Offset the pointer
    Dest_RDI = Add(OpSize::i64Bit, Dest_RDI, PtrDir);
    StoreGPRRegister(X86State::REG_RDI, Dest_RDI);

    // Offset second pointer
    Dest_RSI = Add(OpSize::i64Bit, Dest_RSI, PtrDir);
    StoreGPRRegister(X86State::REG_RSI, Dest_RSI);
  } else {
    // Calculate flags early.
    CalculateDeferredFlags();

    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    // If rcx = 0, skip the whole loop.
    Ref Counter = LoadGPRRegister(X86State::REG_RCX);
    auto OuterJump = CondJump(Counter, {COND_EQ});

    auto BeforeLoop = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetFalseJumpTarget(OuterJump, BeforeLoop);
    SetCurrentCodeBlock(BeforeLoop);
    StartNewBlock();

    ForeachDirection([this, Op, Size, REPE](int32_t PtrDir) {
      IRPair<IROp_CondJump> InnerJump;
      auto JumpIntoLoop = Jump();

      // Setup for the loop
      auto LoopHeader = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetCurrentCodeBlock(LoopHeader);
      StartNewBlock();
      SetJumpTarget(JumpIntoLoop, LoopHeader);

      // Working loop
      {
        // Default DS prefix
        Ref Dest_RSI = MakeSegmentAddress(X86State::REG_RSI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);
        // Only ES prefix
        Ref Dest_RDI = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

        auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
        auto Src2 = _LoadMem(GPRClass, Size, Dest_RSI, Size);

        // We'll calculate PF/AF after the loop, so use them as temporaries here.
        StoreRegister(Core::CPUState::PF_AS_GREG, false, Src1);
        StoreRegister(Core::CPUState::AF_AS_GREG, false, Src2);

        Ref TailCounter = LoadGPRRegister(X86State::REG_RCX);

        // Decrement counter
        TailCounter = SubWithFlags(OpSize::i64Bit, TailCounter, 1);

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        Dest_RDI = Add(OpSize::i64Bit, Dest_RDI, PtrDir * static_cast<int32_t>(IR::OpSizeToSize(Size)));
        StoreGPRRegister(X86State::REG_RDI, Dest_RDI);

        // Offset second pointer
        Dest_RSI = Add(OpSize::i64Bit, Dest_RSI, PtrDir * static_cast<int32_t>(IR::OpSizeToSize(Size)));
        StoreGPRRegister(X86State::REG_RSI, Dest_RSI);

        // If TailCounter != 0, compare sources.
        // If TailCounter == 0, set ZF iff that would break.
        _CondSubNZCV(OpSize::i64Bit, Src2, Src1, {COND_NEQ}, REPE ? 0 : (1 << 2) /* Z */);
        CachedNZCV = nullptr;
        NZCVDirty = false;
        InnerJump = CondJumpNZCV({REPE ? COND_EQ : COND_NEQ});

        // Jump back to the start if we have more work to do
        SetTrueJumpTarget(InnerJump, LoopHeader);
      }

      // Make sure to start a new block after ending this one
      auto LoopEnd = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetFalseJumpTarget(InnerJump, LoopEnd);
      SetCurrentCodeBlock(LoopEnd);
      StartNewBlock();
    });

    // Make sure to start a new block after ending this one
    {
      // Grab the sources from the last iteration so we can set flags.
      auto Src1 = LoadGPR(Core::CPUState::PF_AS_GREG);
      auto Src2 = LoadGPR(Core::CPUState::AF_AS_GREG);
      CalculateFlags_SUB(OpSizeFromSrc(Op), Src2, Src1);
    }
    auto Jump_ = Jump();

    auto Exit = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetJumpTarget(Jump_, Exit);
    SetTrueJumpTarget(OuterJump, Exit);
    SetCurrentCodeBlock(Exit);
    StartNewBlock();
  }
}

void OpDispatchBuilder::LODSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto Size = OpSizeFromSrc(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) != 0;

  if (!Repeat) {
    Ref Dest_RSI = MakeSegmentAddress(X86State::REG_RSI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    StoreResult(GPRClass, Op, Src, OpSize::iInvalid);

    // Offset the pointer
    Ref TailDest_RSI = LoadGPRRegister(X86State::REG_RSI);
    StoreGPRRegister(X86State::REG_RSI, OffsetByDir(TailDest_RSI, IR::OpSizeToSize(Size)));
  } else {
    // Calculate flags early. because end of block
    CalculateDeferredFlags();

    ForeachDirection([this, Op, Size](int32_t PtrDir) {
      // XXX: Theoretically LODS could be optimized to
      // RSI += {-}(RCX * Size)
      // RAX = [RSI - Size]
      // But this might violate the case of an application scanning pages for read permission and catching the fault
      // May or may not matter

      auto JumpStart = Jump();
      // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);
      StartNewBlock();

      Ref Counter = LoadGPRRegister(X86State::REG_RCX);

      // Can we end the block?

      // We leave if RCX = 0
      auto CondJump_ = CondJump(Counter, {COND_EQ});

      auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
      SetFalseJumpTarget(CondJump_, LoopTail);
      SetCurrentCodeBlock(LoopTail);
      StartNewBlock();

      // Working loop
      {
        Ref Dest_RSI = MakeSegmentAddress(X86State::REG_RSI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);

        auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

        StoreResult(GPRClass, Op, Src, OpSize::iInvalid);

        Ref TailCounter = LoadGPRRegister(X86State::REG_RCX);
        Ref TailDest_RSI = LoadGPRRegister(X86State::REG_RSI);

        // Decrement counter
        TailCounter = Sub(OpSize::i64Bit, TailCounter, 1);

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        TailDest_RSI = Add(OpSize::i64Bit, TailDest_RSI, PtrDir * static_cast<int32_t>(IR::OpSizeToSize(Size)));
        StoreGPRRegister(X86State::REG_RSI, TailDest_RSI);

        // Jump back to the start, we have more work to do
        Jump(LoopStart);
      }
      // Make sure to start a new block after ending this one
      auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);
      SetTrueJumpTarget(CondJump_, LoopEnd);
      SetCurrentCodeBlock(LoopEnd);
      StartNewBlock();
    });
  }
}

void OpDispatchBuilder::SCASOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto Size = OpSizeFromSrc(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX)) != 0;

  if (!Repeat) {
    Ref Dest_RDI = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

    CalculateFlags_SUB(OpSizeFromSrc(Op), Src1, Src2);

    // Offset the pointer
    Ref TailDest_RDI = LoadGPRRegister(X86State::REG_RDI);
    StoreGPRRegister(X86State::REG_RDI, OffsetByDir(TailDest_RDI, IR::OpSizeToSize(Size)));
  } else {
    // Calculate flags early. because end of block
    CalculateDeferredFlags();

    ForeachDirection([this, Op, Size](int32_t Dir) {
      bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

      auto JumpStart = Jump();
      // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);
      StartNewBlock();

      Ref Counter = LoadGPRRegister(X86State::REG_RCX);

      // Can we end the block?
      // We leave if RCX = 0
      auto CondJump_ = CondJump(Counter, {COND_EQ});
      IRPair<IROp_CondJump> InternalCondJump;

      auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
      SetFalseJumpTarget(CondJump_, LoopTail);
      SetCurrentCodeBlock(LoopTail);
      StartNewBlock();

      // Working loop
      {
        Ref Dest_RDI = MakeSegmentAddress(X86State::REG_RDI, 0, X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

        auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
        auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

        CalculateFlags_SUB(OpSizeFromSrc(Op), Src1, Src2);

        // Calculate flags early.
        CalculateDeferredFlags();

        Ref TailCounter = LoadGPRRegister(X86State::REG_RCX);
        Ref TailDest_RDI = LoadGPRRegister(X86State::REG_RDI);

        // Decrement counter
        TailCounter = Sub(OpSize::i64Bit, TailCounter, 1);

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        TailDest_RDI = Add(OpSize::i64Bit, TailDest_RDI, Dir * static_cast<int32_t>(IR::OpSizeToSize(Size)));
        StoreGPRRegister(X86State::REG_RDI, TailDest_RDI);

        CalculateDeferredFlags();
        InternalCondJump = CondJumpNZCV({REPE ? COND_EQ : COND_NEQ});

        // Jump back to the start if we have more work to do
        SetTrueJumpTarget(InternalCondJump, LoopStart);
      }
      // Make sure to start a new block after ending this one
      auto LoopEnd = CreateNewCodeBlockAfter(LoopTail);
      SetTrueJumpTarget(CondJump_, LoopEnd);

      SetFalseJumpTarget(InternalCondJump, LoopEnd);

      SetCurrentCodeBlock(LoopEnd);
      StartNewBlock();
    });
  }
}

void OpDispatchBuilder::BSWAPOp(OpcodeArgs) {
  Ref Dest;
  const auto Size = OpSizeFromSrc(Op);
  if (Size == OpSize::i16Bit) {
    // BSWAP of 16bit is undef. ZEN+ causes the lower 16bits to get zero'd
    Dest = Constant(0);
  } else {
    Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GetGPROpSize(), Op->Flags);
    Dest = _Rev(Size, Dest);
  }
  StoreResult(GPRClass, Op, Dest, OpSize::iInvalid);
}

void OpDispatchBuilder::PUSHFOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);

  Push(Size, GetPackedRFLAG());
}

void OpDispatchBuilder::POPFOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);
  Ref Src = Pop(Size);

  // Add back our flag constants
  // Bit 1 is always 1
  // Bit 9 is always 1 because we always have interrupts enabled

  Src = _Or(OpSize::i64Bit, Src, Constant(0x202));

  SetPackedRFLAG(false, Src);

  auto NewRIP = GetRelocatedPC(Op);
  ExitFunction(NewRIP, BranchHint::CheckTF);
  BlockSetRIP = true;
}

void OpDispatchBuilder::NEGOp(OpcodeArgs) {
  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;

  const auto Size = OpSizeFromSrc(Op);
  auto ZeroConst = Constant(0);

  if (DestIsLockedMem(Op)) {
    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    Ref Dest = _AtomicFetchNeg(Size, DestMem);
    CalculateFlags_SUB(Size, ZeroConst, Dest);
  } else {
    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
    Ref Result = CalculateFlags_SUB(Size, ZeroConst, Dest);

    StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::DIVOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  auto Size = OpSizeFromSrc(Op);

  // This loads the divisor. 32-bit/64-bit paths mask inside the JIT, 8/16 do not.
  Ref Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= OpSize::i32Bit});

  if (Size == OpSize::i64Bit && !Is64BitMode) {
    LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
    DecodeFailure = true;
    return;
  }

  Ref Quotient = _AllocateGPR(true);
  Ref Remainder = _AllocateGPR(true);

  if (Size == OpSize::i8Bit) {
    Ref Src1 = LoadGPRRegister(X86State::REG_RAX, OpSize::i16Bit);

    _UDiv(OpSize::i16Bit, Src1, Invalid(), Divisor, Quotient, Remainder);

    // AX[15:0] = concat<URem[7:0]:UDiv[7:0]>
    auto ResultAX = _Bfi(GPRSize, 8, 8, Quotient, Remainder);
    StoreGPRRegister(X86State::REG_RAX, ResultAX, OpSize::i16Bit);
  } else {
    Ref Src1 = LoadGPRRegister(X86State::REG_RAX);
    Ref Src2 = LoadGPRRegister(X86State::REG_RDX);

    _UDiv(Size, Src1, Src2, Divisor, Quotient, Remainder);

    if (Size == OpSize::i32Bit) {
      Quotient = _Bfe(OpSize::i32Bit, IR::OpSizeAsBits(Size), 0, Quotient);
      Remainder = _Bfe(OpSize::i32Bit, IR::OpSizeAsBits(Size), 0, Remainder);
      Size = OpSize::iInvalid;
    }

    StoreGPRRegister(X86State::REG_RAX, Quotient, Size);
    StoreGPRRegister(X86State::REG_RDX, Remainder, Size);
  }
}

void OpDispatchBuilder::IDIVOp(OpcodeArgs) {
  // This loads the divisor
  Ref Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  const auto GPRSize = GetGPROpSize();
  auto Size = OpSizeFromSrc(Op);

  if (Size == OpSize::i64Bit && !Is64BitMode) {
    LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
    DecodeFailure = true;
    return;
  }

  Ref Quotient = _AllocateGPR(true);
  Ref Remainder = _AllocateGPR(true);

  if (Size == OpSize::i8Bit) {
    Ref Src1 = LoadGPRRegister(X86State::REG_RAX);
    Src1 = _Sbfe(OpSize::i64Bit, 16, 0, Src1);
    Divisor = _Sbfe(OpSize::i64Bit, 8, 0, Divisor);

    _Div(OpSize::i64Bit, Src1, Invalid(), Divisor, Quotient, Remainder);

    // AX[15:0] = concat<URem[7:0]:UDiv[7:0]>
    auto ResultAX = _Bfi(GPRSize, 8, 8, Quotient, Remainder);
    StoreGPRRegister(X86State::REG_RAX, ResultAX, OpSize::i16Bit);
  } else {
    Ref Src1 = LoadGPRRegister(X86State::REG_RAX);
    Ref Src2 = LoadGPRRegister(X86State::REG_RDX);

    _Div(Size, Src1, Src2, Divisor, Quotient, Remainder);

    if (Size == OpSize::i32Bit) {
      Quotient = _Bfe(OpSize::i32Bit, IR::OpSizeAsBits(Size), 0, Quotient);
      Remainder = _Bfe(OpSize::i32Bit, IR::OpSizeAsBits(Size), 0, Remainder);
      Size = OpSize::iInvalid;
    }

    StoreGPRRegister(X86State::REG_RAX, Quotient, Size);
    StoreGPRRegister(X86State::REG_RDX, Remainder, Size);
  }
}

void OpDispatchBuilder::BSFOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto DstSize = OpSizeFromDst(Op) == OpSize::i16Bit ? OpSize::i16Bit : GPRSize;
  Ref Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags, {.AllowUpperGarbage = true});
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  // Find the LSB of this source
  auto Result = _FindLSB(OpSizeFromSrc(Op), Src);

  // OF, SF, AF, PF, CF all undefined
  // ZF is set to 1 if the source was zero
  SetZ_InvalidateNCV(OpSizeFromSrc(Op), Src);

  // If Src was zero then the destination doesn't get modified.
  //
  // Although Intel does not guarantee that semantic, AMD does and Intel
  // hardware satisfies it. We provide the stronger AMD behaviour as
  // applications might rely on that in the wild.
  auto SelectOp = NZCVSelect(GPRSize, {COND_EQ}, Dest, Result);
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, OpSize::iInvalid);
}

void OpDispatchBuilder::BSROp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto DstSize = OpSizeFromDst(Op) == OpSize::i16Bit ? OpSize::i16Bit : GPRSize;
  Ref Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags, {.AllowUpperGarbage = true});
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  // Find the MSB of this source
  auto Result = _FindMSB(OpSizeFromSrc(Op), Src);

  // OF, SF, AF, PF, CF all undefined
  // ZF is set to 1 if the source was zero
  SetZ_InvalidateNCV(OpSizeFromSrc(Op), Src);

  // If Src was zero then the destination doesn't get modified
  auto SelectOp = NZCVSelect(GPRSize, {COND_EQ}, Dest, Result);
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, OpSize::iInvalid);
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

  const auto GPRSize = GetGPROpSize();
  auto Size = OpSizeFromSrc(Op);

  if (Op->Dest.IsGPR()) {
    // This is our source register
    Ref Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
    Ref Src3 = LoadGPRRegister(X86State::REG_RAX);

    // If the destination is also the accumulator, we get some algebraic
    // simplifications. Not sure if this is actually hit but it's in
    // InstCountCI.
    bool Trivial = Op->Dest.Data.GPR.GPR == X86State::REG_RAX && !Op->Dest.IsGPRDirect() && !Op->Dest.Data.GPR.HighBits;

    Ref Src1 {};
    Ref Src1Lower {};

    if (GPRSize == OpSize::i64Bit && Size == OpSize::i32Bit) {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags, {.AllowUpperGarbage = true});
      Src1Lower = Trivial ? Src1 : _Bfe(GPRSize, IR::OpSizeAsBits(Size), 0, Src1);
    } else {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, Size, Op->Flags, {.AllowUpperGarbage = true});
      Src1Lower = Src1;
    }

    // Compare RAX with the destination, setting flags accordingly.
    CalculateFlags_SUB(OpSizeFromSrc(Op), Src3, Src1Lower);
    CalculateDeferredFlags();

    if (!Trivial) {
      if (GPRSize == OpSize::i64Bit && Size == OpSize::i32Bit) {
        // This allows us to only hit the ZEXT case on failure
        Ref RAXResult = NZCVSelect(OpSize::i64Bit, {COND_EQ}, Src3, Src1Lower);

        // When the size is 4 we need to make sure not zext the GPR when the comparison fails
        StoreGPRRegister(X86State::REG_RAX, RAXResult);
      } else {
        StoreGPRRegister(X86State::REG_RAX, Src1Lower, Size);
      }
    }

    // Op1 = RAX == Op1 ? Op2 : Op1
    // If they match then set the rm operand to the input
    // else don't set the rm operand
    Ref Src2Lower = Src2;
    if (GPRSize == OpSize::i64Bit && Size == OpSize::i32Bit) {
      Src2Lower = _Bfe(GPRSize, IR::OpSizeAsBits(Size), 0, Src2);
    }
    Ref DestResult = Trivial ? Src2 : NZCVSelect(OpSize::i64Bit, CondClassType {COND_EQ}, Src2Lower, Src1);

    // Store in to GPR Dest
    if (GPRSize == OpSize::i64Bit && Size == OpSize::i32Bit) {
      StoreResult_WithOpSize(GPRClass, Op, Op->Dest, DestResult, GPRSize, OpSize::iInvalid);
    } else {
      StoreResult(GPRClass, Op, DestResult, OpSize::iInvalid);
    }
  } else {
    Ref Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

    auto Src3 = LoadGPRRegister(X86State::REG_RAX);
    auto Src3Lower = _Bfe(OpSize::i64Bit, OpSizeAsBits(Size), 0, Src3);

    // If this is a memory location then we want the pointer to it
    Ref Src1 = MakeSegmentAddress(Op, Op->Dest);

    // DataSrc = *Src1
    // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
    // This will write to memory! Careful!
    // Third operand must be a calculated guest memory address
    Ref CASResult = _CAS(Size, Src3, Src2, Src1);
    Ref RAXResult = CASResult;

    CalculateFlags_SUB(OpSizeFromSrc(Op), Src3Lower, CASResult);
    CalculateDeferredFlags();

    if (GPRSize == OpSize::i64Bit && Size == OpSize::i32Bit) {
      // This allows us to only hit the ZEXT case on failure
      RAXResult = _NZCVSelect(OpSize::i64Bit, {COND_EQ}, Src3, CASResult);
      Size = OpSize::i64Bit;
    }

    // RAX gets the result of the CAS op
    StoreGPRRegister(X86State::REG_RAX, RAXResult, Size);
  }
}

void OpDispatchBuilder::CMPXCHGPairOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  // REX.W used to determine if it is 16byte or 8byte
  // Unlike CMPXCHG, the destination can only be a memory location
  const auto Size = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? OpSize::i64Bit : OpSize::i32Bit;

  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;

  // If this is a memory location then we want the pointer to it
  Ref Src1 = MakeSegmentAddress(Op, Op->Dest);

  // Load the full 64-bit registers, all the users ignore the upper 32-bits for
  // 32-bit only cmpxchg. This saves some zero extension.
  Ref Expected_Lower = LoadGPRRegister(X86State::REG_RAX);
  Ref Expected_Upper = LoadGPRRegister(X86State::REG_RDX);
  Ref Desired_Lower = LoadGPRRegister(X86State::REG_RBX);
  Ref Desired_Upper = LoadGPRRegister(X86State::REG_RCX);

  // ssa0 = Expected
  // ssa1 = Desired
  // ssa2 = MemoryLocation

  // DataSrc = *MemSrc
  // if (DataSrc == Expected) { *MemSrc == Desired; } Expected = DataSrc
  // This will write to memory! Careful!
  // Third operand must be a calculated guest memory address

  Ref Result_Lower = _AllocateGPR(true);
  Ref Result_Upper = _AllocateGPRAfter(Result_Lower);
  _CASPair(Size, Expected_Lower, Expected_Upper, Desired_Lower, Desired_Upper, Src1, Result_Lower, Result_Upper);

  HandleNZCV_RMW();
  _CmpPairZ(Size, Result_Lower, Result_Upper, Expected_Lower, Expected_Upper);
  CalculateDeferredFlags();

  auto UpdateIfNotZF = [this](auto Reg, auto Value) {
    // Always use 64-bit csel to preserve existing upper bits. If we have a
    // 32-bit cmpxchg in a 64-bit context, Value will be zeroed in upper bits.
    StoreGPRRegister(Reg, NZCVSelect(OpSize::i64Bit, {COND_NEQ}, Value, LoadGPRRegister(Reg)));
  };

  UpdateIfNotZF(X86State::REG_RAX, Result_Lower);
  UpdateIfNotZF(X86State::REG_RDX, Result_Upper);
}

void OpDispatchBuilder::CreateJumpBlocks(const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks) {
  Ref PrevCodeBlock {};
  for (auto& Target : *Blocks) {
    auto CodeNode = CreateCodeNode(Target.IsEntryPoint, Target.Entry - Entry);

    JumpTargets.try_emplace(Target.Entry, JumpTargetInfo {CodeNode, false, Target.IsEntryPoint});

    if (PrevCodeBlock) {
      LinkCodeBlocks(PrevCodeBlock, CodeNode);
    }

    PrevCodeBlock = CodeNode;
  }
}

void OpDispatchBuilder::BeginFunction(uint64_t RIP, const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks,
                                      uint32_t NumInstructions, bool _Is64BitMode, bool MonoBackpatcherBlock) {
  Entry = RIP;
  Is64BitMode = _Is64BitMode;
  LOGMAN_THROW_A_FMT(Is64BitMode == CTX->Config.Is64BitMode, "Expected operating mode to not change at runtime!");
  IsMonoBackpatcherBlock = MonoBackpatcherBlock;
  auto IRHeader = _IRHeader(InvalidNode, RIP, 0, NumInstructions, 0, 0);
  CreateJumpBlocks(Blocks);

  auto Block = GetNewJumpBlock(RIP);
  SetCurrentCodeBlock(Block);
  IRHeader.first->Blocks = Block->Wrapped(DualListData.ListBegin());
  CurrentHeader = IRHeader.first;
}

void OpDispatchBuilder::Finalize() {
  // This usually doesn't emit any IR but in the case of hitting the block instruction limit it will
  FlushRegisterCache();
  const auto GPRSize = GetGPROpSize();

  // Node 0 is invalid node
  Ref RealNode = reinterpret_cast<Ref>(GetNode(1));

  const FEXCore::IR::IROp_Header* IROp = RealNode->Op(DualListData.DataBegin());
  LOGMAN_THROW_A_FMT(IROp->Op == OP_IRHEADER, "First op in function must be our header");

  // Let's walk the jump blocks and see if we have handled every block target
  for (auto& Handler : JumpTargets) {
    if (Handler.second.HaveEmitted) {
      continue;
    }

    // We haven't emitted. Dump out to the dispatcher
    SetCurrentCodeBlock(Handler.second.BlockEntry);
    ExitFunction(_InlineEntrypointOffset(GPRSize, Handler.first - Entry));
  }
}

uint8_t OpDispatchBuilder::GetDstSize(X86Tables::DecodedOp Op) const {
  const uint32_t DstSizeFlag = X86Tables::DecodeFlags::GetSizeDstFlags(Op->Flags);
  LOGMAN_THROW_A_FMT(DstSizeFlag != 0 && DstSizeFlag != X86Tables::DecodeFlags::SIZE_MASK, "Invalid destination size for op");
  return 1u << (DstSizeFlag - 1);
}

uint8_t OpDispatchBuilder::GetSrcSize(X86Tables::DecodedOp Op) const {
  const uint32_t SrcSizeFlag = X86Tables::DecodeFlags::GetSizeSrcFlags(Op->Flags);
  LOGMAN_THROW_A_FMT(SrcSizeFlag != 0 && SrcSizeFlag != X86Tables::DecodeFlags::SIZE_MASK, "Invalid destination size for op");
  return 1u << (SrcSizeFlag - 1);
}

uint32_t OpDispatchBuilder::GetSrcBitSize(X86Tables::DecodedOp Op) const {
  return GetSrcSize(Op) * 8;
}

uint32_t OpDispatchBuilder::GetDstBitSize(X86Tables::DecodedOp Op) const {
  return GetDstSize(Op) * 8;
}

Ref OpDispatchBuilder::GetSegment(uint32_t Flags, uint32_t DefaultPrefix, bool Override) {
  const auto GPRSize = GetGPROpSize();
  uint32_t Prefix = Flags & FEXCore::X86Tables::DecodeFlags::FLAG_SEGMENTS;

  if (Is64BitMode) {
    if (Prefix == FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
      return _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
    } else if (Prefix == FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
      return _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
    }
    // If there was any other segment in 64bit then it is ignored
  } else {
    if (Prefix == FEXCore::X86Tables::DecodeFlags::FLAG_NO_PREFIX || Override) {
      // If there was no prefix then use the default one if available
      // Or the argument only uses a specific prefix (with override set)
      Prefix = DefaultPrefix;
    }
    // With the segment register optimization we store the GDT bases directly in the segment register to remove indexed loads
    Ref SegmentResult {};
    switch (Prefix) {
    [[likely]] case FEXCore::X86Tables::DecodeFlags::FLAG_NO_PREFIX:
      return nullptr;
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, es_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, cs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, ss_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, ds_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      SegmentResult = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
      break;
    default: FEX_UNREACHABLE;
    }

    CheckLegacySegmentRead(SegmentResult, Prefix);
    return SegmentResult;
  }
  return nullptr;
}

Ref OpDispatchBuilder::AppendSegmentOffset(Ref Value, uint32_t Flags, uint32_t DefaultPrefix, bool Override) {
  auto Segment = GetSegment(Flags, DefaultPrefix, Override);
  if (Segment) {
    Value = Add(std::max(OpSize::i32Bit, std::max(GetOpSize(Value), GetOpSize(Segment))), Value, Segment);
  }

  return Value;
}


void OpDispatchBuilder::CheckLegacySegmentRead(Ref NewNode, uint32_t SegmentReg) {
#ifndef FEX_DISABLE_TELEMETRY
  if (SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX || SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    // FS and GS segments aren't considered legacy.
    return;
  }

  if (!(SegmentsNeedReadCheck & SegmentReg)) {
    // If the block has done multiple reads of a segment register then skip redundant read checks.
    // Segment write will cause another read check.
    return;
  }

  if (CTX->Config.DisableTelemetry()) {
    // Telemetry disabled at runtime.
    return;
  }

  FEXCore::Telemetry::TelemetryType TelemIndex {};
  switch (SegmentReg) {
  case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_ES;
    SegmentsNeedReadCheck &= ~FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_CS;
    SegmentsNeedReadCheck &= ~FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_SS;
    SegmentsNeedReadCheck &= ~FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_DS;
    SegmentsNeedReadCheck &= ~FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX;
    break;
  default: FEX_UNREACHABLE;
  }

  // Will set the telemetry value if NewNode is != 0
  _TelemetrySetValue(NewNode, TelemIndex);
#endif
}

void OpDispatchBuilder::CheckLegacySegmentWrite(Ref NewNode, uint32_t SegmentReg) {
#ifndef FEX_DISABLE_TELEMETRY
  if (SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX || SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    // FS and GS segments aren't considered legacy.
    return;
  }

  if (CTX->Config.DisableTelemetry()) {
    // Telemetry disabled at runtime.
    return;
  }

  FEXCore::Telemetry::TelemetryType TelemIndex {};
  switch (SegmentReg) {
  case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_ES;
    SegmentsNeedReadCheck |= FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_CS;
    SegmentsNeedReadCheck |= FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_SS;
    SegmentsNeedReadCheck |= FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX;
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
    TelemIndex = FEXCore::Telemetry::TelemetryType::TYPE_WRITES_32BIT_SEGMENT_DS;
    SegmentsNeedReadCheck |= FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX;
    break;
  default: FEX_UNREACHABLE;
  }

  // Will set the telemetry value if NewNode is != 0
  _TelemetrySetValue(NewNode, TelemIndex);
#endif
}

void OpDispatchBuilder::UpdatePrefixFromSegment(Ref Segment, uint32_t SegmentReg) {
  // Use BFE to extract the selector index in bits [15,3] of the segment register.
  // In some cases the upper 16-bits of the 32-bit GPR contain garbage to ignore.
  auto GDT = _Bfe(OpSize::i32Bit, 1, 2, Segment);
  // Fun quirk, if we mask the selector then it is premultiplied by 8 which we need to do for accessing anyway.
  auto SegmentOffset = _And(OpSize::i32Bit, Segment, _Constant(0xfff8));
  Ref SegmentBase = _LoadContextIndexed(GDT, OpSize::i64Bit, offsetof(FEXCore::Core::CPUState, segment_arrays[0]), 8, GPRClass);
  Ref NewSegment = _LoadMem(GPRClass, OpSize::i64Bit, SegmentBase, SegmentOffset, OpSize::i8Bit, MEM_OFFSET_UXTW, 1);
  CheckLegacySegmentWrite(NewSegment, SegmentReg);

  // Extract the 32-bit base from the GDT segment.
  auto Upper32 = _Lshr(OpSize::i64Bit, NewSegment, _Constant(32));
  auto Masked = _And(OpSize::i32Bit, Upper32, _Constant(0xFF00'0000));
  Ref Merged = _Orlshr(OpSize::i32Bit, Masked, NewSegment, 16);
  NewSegment = _Bfi(OpSize::i32Bit, 8, 16, Merged, Upper32);

  switch (SegmentReg) {
  case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, es_cached));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, cs_cached));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ss_cached));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ds_cached));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, fs_cached));
    break;
  case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
    _StoreContext(OpSize::i32Bit, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, gs_cached));
    break;
  default: break; // Do nothing
  }
}

AddressMode OpDispatchBuilder::DecodeAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand,
                                             MemoryAccessType AccessType, bool IsLoad) {
  const auto GPRSize = GetGPROpSize();

  AddressMode A {};
  A.Segment = GetSegment(Op->Flags);
  A.AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;
  A.NonTSO = AccessType == MemoryAccessType::NONTSO || AccessType == MemoryAccessType::STREAM;

  if (Operand.IsLiteral()) {
    A.Offset = Operand.Literal();

    if (Operand.Data.Literal.Size != 8 && IsLoad) {
      // zero extend
      uint64_t width = Operand.Data.Literal.Size * 8;
      A.Offset &= ((1ULL << width) - 1);
    }
  } else if (Operand.IsGPR()) {
    // Not an address, let the caller deal with it
    A.AddrSize = GPRSize;
  } else if (Operand.IsGPRDirect()) {
    A.Base = LoadGPRRegister(Operand.Data.GPR.GPR, GPRSize);
    A.NonTSO |= IsNonTSOReg(AccessType, Operand.Data.GPR.GPR);
  } else if (Operand.IsGPRIndirect()) {
    A.Base = LoadGPRRegister(Operand.Data.GPRIndirect.GPR, GPRSize);
    A.Offset = Operand.Data.GPRIndirect.Displacement;
    A.NonTSO |= IsNonTSOReg(AccessType, Operand.Data.GPRIndirect.GPR);
  } else if (Operand.IsRIPRelative()) {
    if (Is64BitMode) {
      A.Base = GetRelocatedPC(Op, Operand.Data.RIPLiteral.Value.s);
    } else {
      // 32bit this isn't RIP relative but instead absolute
      A.Offset = Operand.Data.RIPLiteral.Value.u;
    }
  } else if (Operand.IsSIB()) {
    const bool IsVSIB = IsLoad && ((Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0);

    if (Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
      A.Base = LoadGPRRegister(Operand.Data.SIB.Base, GPRSize);
    }

    // NOTE: VSIB cannot have the index * scale portion calculated ahead of time,
    //       since the index in this case is a vector. So, we can't just apply the scale
    //       to it, since this needs to be applied to each element in the index register
    //       after said element has been sign extended. So, we pass this through for the
    //       instruction implementation to handle.
    //
    //       What we do handle though, is the applying the displacement value to
    //       the base register (if a base register is provided), since this is a
    //       part of the address calculation that can be done ahead of time.
    if (!IsVSIB && Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID) {
      A.Index = LoadGPRRegister(Operand.Data.SIB.Index, GPRSize);
      A.IndexScale = Operand.Data.SIB.Scale;
    }

    A.Offset = Operand.Data.SIB.Offset;
    A.NonTSO |= IsNonTSOReg(AccessType, Operand.Data.SIB.Base) || IsNonTSOReg(AccessType, Operand.Data.SIB.Index);
  } else {
    LOGMAN_MSG_A_FMT("Unknown Src Type: {}\n", Operand.Type);
  }

  return A;
}


Ref OpDispatchBuilder::LoadSource_WithOpSize(RegisterClassType Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand,
                                             IR::OpSize OpSize, uint32_t Flags, const LoadSourceOptions& Options) {
  auto [Align, LoadData, ForceLoad, AccessType, AllowUpperGarbage] = Options;
  AddressMode A = DecodeAddress(Op, Operand, AccessType, true /* IsLoad */);

  if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    const auto highIndex = Operand.Data.GPR.HighBits ? 1 : 0;

    if (gpr >= FEXCore::X86State::REG_MM_0) {
      LOGMAN_THROW_A_FMT(OpSize == OpSize::i64Bit, "full");

      if (MMXState != MMXState_MMX) {
        ChgStateX87_MMX();
      }

      A.Base = LoadContext(OpSize::i64Bit, MM0Index + gpr - FEXCore::X86State::REG_MM_0);
    } else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      const auto gprIndex = gpr - X86State::REG_XMM_0;

      // Load the full register size if it is a XMM register source.
      A.Base = LoadXMMRegister(gprIndex);

      // Now extract the subregister if it was a partial load /smaller/ than SSE size
      // TODO: Instead of doing the VMov implicitly on load, hunt down all use cases that require partial loads and do it after load.
      // We don't have information here to know if the operation needs zero upper bits or can contain data.
      if (!AllowUpperGarbage && OpSize < OpSize::i128Bit) {
        A.Base = _VMov(OpSize, A.Base);
      }
    } else {
      A.Base = LoadGPRRegister(gpr, OpSize, highIndex ? 8 : 0, AllowUpperGarbage);
    }
  }

  if ((IsOperandMem(Operand, true) && LoadData) || ForceLoad) {
    if (OpSize == OpSize::f80Bit) {
      Ref MemSrc = LoadEffectiveAddress(this, A, GetGPROpSize(), true);
      if (CTX->HostFeatures.SupportsSVE128 || CTX->HostFeatures.SupportsSVE256) {
        return _LoadMemX87SVEOptPredicate(OpSize::i128Bit, OpSize::i16Bit, MemSrc);
      } else {
        // For X87 extended doubles, Split the load.
        auto Res = _LoadMem(Class, OpSize::i64Bit, MemSrc, Align == OpSize::iInvalid ? OpSize : Align);
        return _VLoadVectorElement(OpSize::i128Bit, OpSize::i16Bit, Res, 4, Add(OpSize::i64Bit, MemSrc, 8));
      }
    }

    return _LoadMemAutoTSO(Class, OpSize, A, Align == OpSize::iInvalid ? OpSize : Align);
  } else {
    return LoadEffectiveAddress(this, A, GetGPROpSize(), false, AllowUpperGarbage);
  }
}

Ref OpDispatchBuilder::LoadGPRRegister(uint32_t GPR, IR::OpSize Size, uint8_t Offset, bool AllowUpperGarbage) {
  const auto GPRSize = GetGPROpSize();
  if (Size == OpSize::iInvalid) {
    Size = GPRSize;
  }
  Ref Reg = LoadGPR(GPR);

  if ((!AllowUpperGarbage && (Size != GPRSize)) || Offset != 0) {
    // Extract the subregister if requested.
    const auto OpSize = std::max(OpSize::i32Bit, Size);
    if (AllowUpperGarbage) {
      Reg = _Lshr(OpSize, Reg, Constant(Offset));
    } else {
      Reg = _Bfe(OpSize, IR::OpSizeAsBits(Size), Offset, Reg);
    }
  }
  return Reg;
}

void OpDispatchBuilder::StoreGPRRegister(uint32_t GPR, const Ref Src, IR::OpSize Size, uint8_t Offset) {
  const auto GPRSize = GetGPROpSize();
  if (Size == OpSize::iInvalid) {
    Size = GPRSize;
  }

  Ref Reg = Src;
  if (Size != GPRSize || Offset != 0) {
    // Need to do an insert if not automatic size or zero offset.
    Reg = ARef(Reg).BfiInto(LoadGPRRegister(GPR), Offset, IR::OpSizeAsBits(Size));
  }

  StoreRegister(GPR, false, Reg);
}

void OpDispatchBuilder::StoreXMMRegister(uint32_t XMM, const Ref Src) {
  StoreRegister(XMM, true, Src);
}

Ref OpDispatchBuilder::LoadSource(RegisterClassType Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand,
                                  uint32_t Flags, const LoadSourceOptions& Options) {
  const auto OpSize = OpSizeFromSrc(Op);
  return LoadSource_WithOpSize(Class, Op, Operand, OpSize, Flags, Options);
}

void OpDispatchBuilder::StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op,
                                               const FEXCore::X86Tables::DecodedOperand& Operand, const Ref Src, IR::OpSize OpSize,
                                               IR::OpSize Align, MemoryAccessType AccessType) {
  if (Operand.IsGPR()) {
    // 8Bit and 16bit destination types store their result without effecting the upper bits
    // 32bit ops ZEXT the result to 64bit
    const auto GPRSize = GetGPROpSize();

    const auto gpr = Operand.Data.GPR.GPR;
    if (gpr >= FEXCore::X86State::REG_MM_0) {
      LOGMAN_THROW_A_FMT(OpSize == OpSize::i64Bit, "full");
      LOGMAN_THROW_A_FMT(Class == FPRClass, "MMX is floaty");

      if (MMXState != MMXState_MMX) {
        ChgStateX87_MMX();
      }

      uint8_t Index = MM0Index + gpr - FEXCore::X86State::REG_MM_0;
      StoreContext(Index, Src);
      RegCache.Partial |= (1ull << (uint64_t)Index);
    } else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      const auto gprIndex = gpr - X86State::REG_XMM_0;
      const auto VectorSize = GetGuestVectorLength();

      auto Result = Src;
      if (OpSize != VectorSize) {
        // Partial writes can come from FPRs.
        // TODO: Fix the instructions doing partial writes rather than dealing with it here.

        LOGMAN_THROW_A_FMT(Class != IR::GPRClass, "Partial writes from GPR not allowed. Instruction: {}", Op->TableInfo->Name);

        // XMM-size is handled in implementations.
        if (VectorSize != OpSize::i256Bit || OpSize != OpSize::i128Bit) {
          auto SrcVector = LoadXMMRegister(gprIndex);
          Result = _VInsElement(VectorSize, OpSize, 0, 0, SrcVector, Src);
        }
      }

      StoreXMMRegister(gprIndex, Result);
    } else {
      if (GPRSize == OpSize::i64Bit && OpSize == OpSize::i32Bit) {
        // If the Source IR op is 64 bits, we need to zext the upper bits
        // For all other sizes, the upper bits are guaranteed to already be zero
        Ref Value = GetOpSize(Src) == OpSize::i64Bit ? ARef(Src).Bfe(0, 32).Ref() : Src;
        StoreGPRRegister(gpr, Value, GPRSize);

        LOGMAN_THROW_A_FMT(!Operand.Data.GPR.HighBits, "Can't handle 32bit store to high 8bit register");
      } else {
        LOGMAN_THROW_A_FMT(!(GPRSize == OpSize::i32Bit && OpSize > OpSize::i32Bit), "Oops had a {} GPR load", OpSize);

        if (GPRSize != OpSize) {
          // if the GPR isn't the full size then we need to insert.
          // eg:
          // mov al, 2 ; Move in to lower 8-bits.
          // mov ah, 2 ; Move in to upper 8-bits of 16-bit reg.
          // mov ax, 2 ; Move in to lower 16-bits of reg.
          StoreGPRRegister(gpr, Src, OpSize, Operand.Data.GPR.HighBits * 8);
        } else {
          StoreGPRRegister(gpr, Src, std::min(GPRSize, OpSize));
        }
      }
    }
    return;
  }

  AddressMode A = DecodeAddress(Op, Operand, AccessType, false /* IsLoad */);

  if (OpSize == OpSize::f80Bit) {
    Ref MemStoreDst = LoadEffectiveAddress(this, A, GetGPROpSize(), true);
    if (CTX->HostFeatures.SupportsSVE128 || CTX->HostFeatures.SupportsSVE256) {
      _StoreMemX87SVEOptPredicate(OpSize::i128Bit, OpSize::i16Bit, Src, MemStoreDst);
    } else {
      // For X87 extended doubles, split before storing
      _StoreMem(FPRClass, OpSize::i64Bit, MemStoreDst, Src, Align);
      auto Upper = _VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, Src, 1);
      _StoreMem(GPRClass, OpSize::i16Bit, Upper, MemStoreDst, Constant(8), std::min(Align, OpSize::i64Bit), MEM_OFFSET_SXTX, 1);
    }
  } else {
    _StoreMemAutoTSO(Class, OpSize, A, Src, Align == OpSize::iInvalid ? OpSize : Align);
  }
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op,
                                    const FEXCore::X86Tables::DecodedOperand& Operand, const Ref Src, IR::OpSize Align,
                                    MemoryAccessType AccessType) {
  StoreResult_WithOpSize(Class, Op, Operand, Src, OpSizeFromDst(Op), Align, AccessType);
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, const Ref Src, IR::OpSize Align,
                                    MemoryAccessType AccessType) {
  StoreResult(Class, Op, Op->Dest, Src, Align, AccessType);
}

OpDispatchBuilder::OpDispatchBuilder(FEXCore::Context::ContextImpl* ctx)
  : IREmitter {ctx->OpDispatcherAllocator, ctx->HostFeatures.SupportsTSOImm9}
  , CTX {ctx} {
  ResetWorkingList();

  if (CTX->HostFeatures.SupportsAVX && CTX->HostFeatures.SupportsSVE256) {
    SaveAVXStateFunc = &OpDispatchBuilder::SaveAVXState;
    RestoreAVXStateFunc = &OpDispatchBuilder::RestoreAVXState;
    DefaultAVXStateFunc = &OpDispatchBuilder::DefaultAVXState;
  } else if (CTX->HostFeatures.SupportsAVX) {
    SaveAVXStateFunc = &OpDispatchBuilder::AVX128_SaveAVXState;
    RestoreAVXStateFunc = &OpDispatchBuilder::AVX128_RestoreAVXState;
    DefaultAVXStateFunc = &OpDispatchBuilder::AVX128_DefaultAVXState;
  }
}

void OpDispatchBuilder::ResetWorkingList() {
  IREmitter::ResetWorkingList();
  JumpTargets.clear();
  BlockSetRIP = false;
  DecodeFailure = false;
  ShouldDump = false;
  CurrentCodeBlock = nullptr;
  RegCache.Written = 0;
  RegCache.Cached = 0;
}

void OpDispatchBuilder::UnhandledOp(OpcodeArgs) {
  DecodeFailure = true;
}

void OpDispatchBuilder::MOVGPROp(OpcodeArgs, uint32_t SrcIndex) {
  // StoreResult will store with the same size as the input, so we allow upper
  // garbage on the input. The zero extension would be pointless.
  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.Align = OpSize::i8Bit, .AllowUpperGarbage = true});
  StoreResult(GPRClass, Op, Src, OpSize::i8Bit);
}

void OpDispatchBuilder::MOVGPRImmediate(OpcodeArgs) {
  Ref Src {};
  if (Op->Src[0].Data.Literal.Size <= 4) {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit, .AllowUpperGarbage = true});
  } else {
    // 8-byte literal is special cased.
    const uint64_t Lower = Op->Src[0].Literal();
    const uint64_t Upper = Op->Src[1].Literal();
    const uint64_t Combined = (Upper << 32) | Lower;
    Src = _Constant(Combined);
  }
  StoreResult(GPRClass, Op, Src, OpSize::i8Bit);
}

void OpDispatchBuilder::MOVGPRNTOp(OpcodeArgs) {
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});
  StoreResult(GPRClass, Op, Src, OpSize::i8Bit, MemoryAccessType::STREAM);
}

void OpDispatchBuilder::ALUOp(OpcodeArgs, FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp, unsigned SrcIdx) {
  // On x86, the canonical way to zero a register is XOR with itself. Detect and
  // emit optimal arm64 assembly.
  if (!DestIsLockedMem(Op) && ALUIROp == FEXCore::IR::IROps::OP_XOR && Op->Dest.IsGPR() && Op->Src[SrcIdx].IsGPR() &&
      Op->Dest.Data.GPR == Op->Src[SrcIdx].Data.GPR) {

    // Set flags for zero result with inverted carry. We subtract an arbitrary
    // register from itself to get the zero, since `subs wzr, #0` is not
    // encodable. This is optimal and works regardless of the opsize.
    auto Zero = LoadGPR(Op->Dest.Data.GPR.GPR);
    HandleNZ00Write();
    InvalidateAF();
    CalculatePF(SubWithFlags(OpSize::i32Bit, Zero, Zero));
    CFInverted = true;
    FlushRegisterCache();

    // Move 0 into the register
    StoreResult(GPRClass, Op, Constant(0), OpSize::iInvalid);
    return;
  }

  auto Size = OpSizeFromDst(Op);
  auto ResultSize = Size;

  auto RoundedSize = Size;
  if (ALUIROp != FEXCore::IR::IROps::OP_ANDWITHFLAGS) {
    RoundedSize = std::max(OpSize::i32Bit, RoundedSize);
  }

  // X86 basic ALU ops just do the operation between the destination and a single source
  Ref Src = LoadSource(GPRClass, Op, Op->Src[SrcIdx], Op->Flags, {.AllowUpperGarbage = true});

  // Try to eliminate the masking after 8/16-bit operations with constants, by
  // promoting to a full size operation that preserves the upper bits.
  uint64_t Const;
  bool IsConst = IsValueConstant(WrapNode(Src), &Const);
  if (Size < OpSize::i32Bit && !DestIsLockedMem(Op) && Op->Dest.IsGPR() && !Op->Dest.Data.GPR.HighBits && IsConst &&
      (ALUIROp == IR::IROps::OP_XOR || ALUIROp == IR::IROps::OP_OR || ALUIROp == IR::IROps::OP_ANDWITHFLAGS)) {

    RoundedSize = ResultSize = GetGPROpSize();
    LOGMAN_THROW_A_FMT(Const < (1ull << IR::OpSizeAsBits(Size)), "does not clobber");

    // For AND, we can play the same trick but we instead need the upper bits of
    // the constant to be all-1s instead of all-0s to preserve. We also can't
    // use andwithflags in this case, since we've promoted to 64-bit so the
    // negate flag would be wrong, but using the regular logical operation path
    // instead still ends up a net win for uops.
    //
    // In the common case where the constant is of the form (1 << x) - 1, the
    // adjusted constant here will inline into the arm64 and instruction, so if
    // flags are not needed, we save an instruction overall.
    if (ALUIROp == IR::IROps::OP_ANDWITHFLAGS) {
      Src = Constant(Const | ~((1ull << IR::OpSizeAsBits(Size)) - 1));
      ALUIROp = IR::IROps::OP_AND;
    }
  }

  Ref Result {};
  Ref Dest {};

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    DeriveOp(FetchOp, AtomicFetchOp, _AtomicFetchAdd(Size, Src, DestMem));
    Dest = FetchOp;
  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  const auto OpSize = RoundedSize;
  uint64_t Mask = Size == OpSize::i64Bit ? ~0ull : ((1ull << IR::OpSizeAsBits(Size)) - 1);
  if (IsConst && Const == Mask && !DestIsLockedMem(Op) && ALUIROp == IR::IROps::OP_XOR && Size >= OpSize::i32Bit) {
    Result = _Not(OpSize, Dest);
  } else if (IsConst && Const == Mask && !DestIsLockedMem(Op) && ALUIROp == IR::IROps::OP_AND) {
    Result = Dest;
  } else {
    DeriveOp(ALUOp, ALUIROp, _AndWithFlags(OpSize, Dest, Src));
    Result = ALUOp;
  }

  // Flags set
  switch (ALUIROp) {
  case FEXCore::IR::IROps::OP_ADD: Result = CalculateFlags_ADD(Size, Dest, Src); break;
  case FEXCore::IR::IROps::OP_SUB: Result = CalculateFlags_SUB(Size, Dest, Src); break;
  case FEXCore::IR::IROps::OP_XOR:
  case FEXCore::IR::IROps::OP_AND:
  case FEXCore::IR::IROps::OP_OR: {
    CalculateFlags_Logical(Size, Result);
    break;
  }
  case FEXCore::IR::IROps::OP_ANDWITHFLAGS: {
    HandleNZ00Write();
    CalculatePF(Result);
    InvalidateAF();
    break;
  }
  default: break;
  }

  if (!DestIsLockedMem(Op)) {
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, ResultSize, OpSize::iInvalid, MemoryAccessType::DEFAULT);
  }
}

void OpDispatchBuilder::LSLOp(OpcodeArgs) {
  // Emulate by always returning failure, this deviates from both Linux and Windows but
  // shouldn't be depended on by anything.
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_RAW_LOC>(Constant(0));
}

void OpDispatchBuilder::INTOp(OpcodeArgs) {
  IR::BreakDefinition Reason;
  bool SetRIPToNext = false;

  switch (Op->OP) {
  case 0xCD: { // INT imm8
    uint8_t Literal = Op->Src[0].Literal();

#ifndef _WIN32
    constexpr uint8_t SYSCALL_LITERAL = 0x80;
    if (Literal == SYSCALL_LITERAL) {
      if (Is64BitMode) [[unlikely]] {
        LogMan::Msg::EFmt("[Unsupported] Trying to execute 32-bit syscall from a 64-bit process.");
        UnhandledOp(Op);
        return;
      }
      // Syscall on linux
      SyscallOp(Op, false);
      return;
    }
#else
    constexpr uint8_t SYSCALL_LITERAL = 0x2E;
    if (Literal == SYSCALL_LITERAL) {
      // Can be used for both 64-bit and 32-bit syscalls on windows
      SyscallOp(Op, false);
      return;
    }
#endif

#ifdef _M_ARM_64EC
    // This is used when QueryPerformanceCounter is called on recent Windows versions, it causes CNTVCT to be written into RAX.
    constexpr uint8_t GET_CNTVCT_LITERAL = 0x81;
    if (Literal == GET_CNTVCT_LITERAL) {
      StoreGPRRegister(X86State::REG_RAX, _CycleCounter(false));
      return;
    }
#endif

    Reason.ErrorRegister = Literal << 3 | (0b010);
    Reason.Signal = Core::FAULT_SIGSEGV;
    // GP is raised when task-gate isn't setup to be valid
    Reason.TrapNumber = X86State::X86_TRAPNO_GP;
    Reason.si_code = 0x80;
    break;
  }
  case 0xCE: // INTO
    Reason.ErrorRegister = 0;
    Reason.Signal = Core::FAULT_SIGSEGV;
    Reason.TrapNumber = X86State::X86_TRAPNO_OF;
    Reason.si_code = 0x80;
    break;
  case 0xF1: // INT1
    Reason.ErrorRegister = 0;
    Reason.Signal = Core::FAULT_SIGTRAP;
    Reason.TrapNumber = X86State::X86_TRAPNO_DB;
    Reason.si_code = 1;
    SetRIPToNext = true;
    break;
  case 0xF4: { // HLT
    Reason.ErrorRegister = 0;
    Reason.Signal = Core::FAULT_SIGSEGV;
    Reason.TrapNumber = X86State::X86_TRAPNO_GP;
    Reason.si_code = 0x80;
    break;
  }
  case 0x0B: // UD2
    Reason.ErrorRegister = 0;
    Reason.Signal = Core::FAULT_SIGILL;
    Reason.TrapNumber = X86State::X86_TRAPNO_UD;
    Reason.si_code = 2;
    break;
  case 0xCC: // INT3
    Reason.ErrorRegister = 0;
    Reason.Signal = Core::FAULT_SIGTRAP;
    Reason.TrapNumber = X86State::X86_TRAPNO_BP;
    Reason.si_code = 0x80;
    SetRIPToNext = true;
    break;
  default: FEX_UNREACHABLE;
  }

  // Calculate flags early.
  FlushRegisterCache();

  const auto GPRSize = GetGPROpSize();

  if (SetRIPToNext) {
    BlockSetRIP = SetRIPToNext;

    // We want to set RIP to the next instruction after INT3/INT1
    auto NewRIP = GetRelocatedPC(Op);
    _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));
  } else if (Op->OP != 0xCE) {
    auto NewRIP = GetRelocatedPC(Op, -Op->InstSize);
    _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));
  }

  if (Op->OP == 0xCE) { // Conditional to only break if Overflow == 1
    CalculateDeferredFlags();

    // If condition doesn't hold then keep going
    // COND_FNU means OF == 0
    auto CondJump_ = CondJumpNZCV({COND_FNU});
    auto FalseBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetFalseJumpTarget(CondJump_, FalseBlock);
    SetCurrentCodeBlock(FalseBlock);
    StartNewBlock();

    auto NewRIP = GetRelocatedPC(Op);
    _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));
    Break(Reason);

    // Make sure to start a new block after ending this one
    auto JumpTarget = CreateNewCodeBlockAfter(FalseBlock);
    SetTrueJumpTarget(CondJump_, JumpTarget);
    SetCurrentCodeBlock(JumpTarget);
    StartNewBlock();
  } else {
    BlockSetRIP = true;
    Break(Reason);
  }
}

void OpDispatchBuilder::TZCNT(OpcodeArgs) {
  // _FindTrailingZeroes ignores upper garbage so we don't need to mask
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  Src = _FindTrailingZeroes(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Src, OpSize::iInvalid);

  CalculateFlags_ZCNT(OpSizeFromSrc(Op), Src);
}

void OpDispatchBuilder::LZCNT(OpcodeArgs) {
  // _CountLeadingZeroes clears upper garbage so we don't need to mask
  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  auto Res = _CountLeadingZeroes(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Res, OpSize::iInvalid);
  CalculateFlags_ZCNT(OpSizeFromSrc(Op), Res);
}

void OpDispatchBuilder::MOVBEOp(OpcodeArgs) {
  const auto GPRSize = GetGPROpSize();
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});

  if (DestIsMem(Op) || SrcSize != OpSize::i16Bit) {
    Src = _Rev(SrcSize, Src);
    StoreResult(GPRClass, Op, Op->Dest, Src, OpSize::iInvalid);
  } else {
    Src = _Rev(std::max(OpSize::i32Bit, SrcSize), Src);
    // 16-bit does an insert.
    // Rev of 16-bit value as 32-bit replaces the result in the upper 16-bits of the result.
    // bfxil the 16-bit result in to the GPR.
    Ref Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);
    auto Result = _Bfxil(GPRSize, 16, 16, Dest, Src);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::CLWBOrTPause(OpcodeArgs) {
  if (DestIsMem(Op)) {
    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    _CacheLineClean(DestMem);
  } else {
    if (!CTX->HostFeatures.SupportsWFXT) {
      UnimplementedOp(Op);
    } else {
      auto RAX = LoadGPRRegister(X86State::REG_RAX);
      auto RDX = LoadGPRRegister(X86State::REG_RDX);

      // Incoming source register is unused.
      _WFET(RDX, RAX);

      // OF, SF, ZF, AF, PF, CF all zero.
      // CF is used if the OS deadline is set, which we don't do anything with.
      ZeroPF_AF();
      ZeroNZCV();
    }
  }
}

void OpDispatchBuilder::CLFLUSHOPT(OpcodeArgs) {
  Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
  _CacheLineClear(DestMem, false);
}

void OpDispatchBuilder::LoadFenceOrXRSTOR(OpcodeArgs) {
  // 0xE8 signifies LFENCE
  if (Op->ModRM == 0xE8) {
    _Fence(IR::Fence_Load);
  } else {
    XRstorOpImpl(Op);
  }
}

void OpDispatchBuilder::MemFenceOrXSAVEOPT(OpcodeArgs) {
  if (Op->ModRM == 0xF0) {
    // 0xF0 is MFENCE
    _Fence(FEXCore::IR::Fence_LoadStore);
  } else {
    XSaveOp(Op);
  }
}

void OpDispatchBuilder::StoreFenceOrCLFlush(OpcodeArgs) {
  if (Op->ModRM == 0xF8) {
    // 0xF8 is SFENCE
    _Fence({FEXCore::IR::Fence_Store});
  } else {
    // This is a CLFlush
    Ref DestMem = MakeSegmentAddress(Op, Op->Dest);
    _CacheLineClear(DestMem, true);
  }
}

void OpDispatchBuilder::UMonitorOrCLRSSBSY(OpcodeArgs) {
  if (DestIsMem(Op) || !CTX->HostFeatures.SupportsWFXT) {
    // CLRSSBSY
    UnimplementedOp(Op);
  } else {
    // Explicit NOP implementation of umonitor.
  }
}

void OpDispatchBuilder::UMWaitOp(OpcodeArgs) {
  if (DestIsMem(Op) || !CTX->HostFeatures.SupportsWFXT) {
    UnimplementedOp(Op);
  } else {
    // Explicit NOP implementation of umwait.
    // Still zero flags.
    //
    // OF, SF, ZF, AF, PF, CF all zero.
    ZeroPF_AF();
    ZeroNZCV();
  }
}

void OpDispatchBuilder::CLZeroOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsCLZERO) {
    UnimplementedOp(Op);
    return;
  }
  Ref DestMem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  _CacheLineZero(DestMem);
}

void OpDispatchBuilder::Prefetch(OpcodeArgs, bool ForStore, bool Stream, uint8_t Level) {
  Ref DestMem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  _Prefetch(ForStore, Stream, Level, DestMem, Invalid(), MEM_OFFSET_SXTX, 1);
}

void OpDispatchBuilder::RDTSCPOp(OpcodeArgs) {
  // RDTSCP is slightly different than RDTSC
  // IA32_TSC_AUX is returned in RCX
  // All previous loads are globally visible
  //  - Explicitly does not wait for stores to be globally visible
  //  - Explicitly use an MFENCE before this instruction if you want this behaviour
  // This instruction is not an execution fence, so subsequent instructions can execute after this
  //  - Explicitly use an LFENCE after RDTSCP if you want to block this behaviour

  auto Counter = CycleCounter(true);

  auto ID = _ProcessorID();
  StoreGPRRegister(X86State::REG_RAX, Counter.CounterLow);
  StoreGPRRegister(X86State::REG_RCX, ID);
  StoreGPRRegister(X86State::REG_RDX, Counter.CounterHigh);
}

void OpDispatchBuilder::RDPIDOp(OpcodeArgs) {
  StoreResult(GPRClass, Op, _ProcessorID(), OpSize::iInvalid);
}

void OpDispatchBuilder::CRC32(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsCRC) {
    UnimplementedOp(Op);
    return;
  }
  const auto GPRSize = GetGPROpSize();

  // Destination GPR size is always 4 or 8 bytes depending on widening
  const auto DstSize = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? OpSize::i64Bit : OpSize::i32Bit;
  Ref Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);

  // Incoming memory is 8, 16, 32, or 64
  Ref Src {};
  if (Op->Src[0].IsGPR()) {
    Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  } else {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});
  }
  auto Result = _CRC32(Dest, Src, OpSizeFromSrc(Op));
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template<bool Reseed>
void OpDispatchBuilder::RDRANDOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsRAND) {
    UnimplementedOp(Op);
    return;
  }

  StoreResult(GPRClass, Op, _RDRAND(Reseed), OpSize::iInvalid);

  // If the rng number is valid then NZCV is 0b0000, otherwise NZCV is 0b0100
  auto CF_inv = GetRFLAG(X86State::RFLAG_ZF_RAW_LOC);

  // OF, SF, ZF, AF, PF all zero. CF indicates if valid.
  ZeroPF_AF();

  if (!CTX->HostFeatures.SupportsFlagM) {
    ZeroNZCV();
    SetCFInverted(CF_inv);
  } else {
    // Accelerated path. Invalid is 0 or 1, so set NZCV with a single rmif.
    HandleNZCVWrite();
    _RmifNZCV(CF_inv, (64 - 1) /* rotate bit 0 into bit 1 = C */, 0xf);
    CFInverted = true;
  }
}

template void OpDispatchBuilder::RDRANDOp<true>(OpcodeArgs);
template void OpDispatchBuilder::RDRANDOp<false>(OpcodeArgs);

void OpDispatchBuilder::BreakOp(OpcodeArgs, FEXCore::IR::BreakDefinition BreakDefinition) {
  const auto GPRSize = GetGPROpSize();

  // We don't actually support this instruction
  // Multiblock may hit it though
  _StoreContext(GPRSize, GPRClass, GetRelocatedPC(Op, -Op->InstSize), offsetof(FEXCore::Core::CPUState, rip));
  Break(BreakDefinition);


  if (Multiblock) {
    auto NextBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetCurrentCodeBlock(NextBlock);
    StartNewBlock();
  } else {
    BlockSetRIP = true;
  }
}

void OpDispatchBuilder::UnimplementedOp(OpcodeArgs) {
  BreakOp(Op, FEXCore::IR::BreakDefinition {
                .ErrorRegister = 0,
                .Signal = SIGILL,
                .TrapNumber = X86State::X86_TRAPNO_UD,
                .si_code = 2, ///< ILL_ILLOPN
              });
}

void OpDispatchBuilder::PermissionRestrictedOp(OpcodeArgs) {
  BreakOp(Op, FEXCore::IR::BreakDefinition {
                .ErrorRegister = 0,
                .Signal = SIGSEGV,
                .TrapNumber = X86State::X86_TRAPNO_GP,
                .si_code = 0x80,
              });
}

void OpDispatchBuilder::InvalidOp(OpcodeArgs) {
  BreakOp(Op, FEXCore::IR::BreakDefinition {
                .ErrorRegister = 0,
                .Signal = SIGILL,
                .TrapNumber = 0,
                .si_code = 0,
              });
}

void OpDispatchBuilder::NoExecOp(OpcodeArgs) {
  BreakOp(Op, FEXCore::IR::BreakDefinition {
                .ErrorRegister = X86State::X86_PF_PROT | X86State::X86_PF_USER | X86State::X86_PF_INSTR,
                .Signal = Core::FAULT_SIGSEGV,
                .TrapNumber = X86State::X86_TRAPNO_PF,
                .si_code = 2, // SEGV_ACCERR
              });
}

#undef OpcodeArgs
} // namespace FEXCore::IR
