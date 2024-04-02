// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 ops to IR, no-pf opt, local-flags opt
$end_info$
*/

#include "FEXCore/Utils/Telemetry.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"
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
#include <bit>
#include <cstdint>
#include <tuple>

namespace FEXCore::IR {

using X86Tables::OpToIndex;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

template<bool IsSyscallInst>
void OpDispatchBuilder::SyscallOp(OpcodeArgs) {
  constexpr size_t SyscallArgs = 7;
  using SyscallArray = std::array<uint64_t, SyscallArgs>;

  size_t NumArguments{};
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

  static constexpr SyscallArray GPRIndexes_Hangover = {
    FEXCore::X86State::REG_RCX,
  };

  static constexpr SyscallArray GPRIndexes_Win64 = {
    FEXCore::X86State::REG_RAX,
    FEXCore::X86State::REG_R10,
    FEXCore::X86State::REG_RDX,
    FEXCore::X86State::REG_R8,
    FEXCore::X86State::REG_R9,
    FEXCore::X86State::REG_RSP,
  };

  SyscallFlags DefaultSyscallFlags = FEXCore::IR::SyscallFlags::DEFAULT;

  const auto OSABI = CTX->SyscallHandler->GetOSABI();
  if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX64) {
    NumArguments = GPRIndexes_64.size();
    GPRIndexes = &GPRIndexes_64;
  }
  else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX32) {
    NumArguments = GPRIndexes_32.size();
    GPRIndexes = &GPRIndexes_32;
  }
  else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_WIN64) {
    NumArguments = 6;
    GPRIndexes = &GPRIndexes_Win64;
    DefaultSyscallFlags = FEXCore::IR::SyscallFlags::NORETURNEDRESULT;
  }
  else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_WIN32) {
    // Since the whole context is going to be saved at entry anyway, theres no need to do additional work to pass in args
    NumArguments = 0;
    GPRIndexes = nullptr;
    DefaultSyscallFlags = FEXCore::IR::SyscallFlags::NORETURNEDRESULT;
  }
  else if (OSABI == FEXCore::HLE::SyscallOSABI::OS_HANGOVER) {
    NumArguments = 1;
    GPRIndexes = &GPRIndexes_Hangover;
  }
  else {
    LogMan::Msg::DFmt("Unhandled OSABI syscall");
  }

  // Calculate flags early.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();
  auto NewRIP = GetRelocatedPC(Op, -Op->InstSize);
  _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));

  OrderedNode *Arguments[SyscallArgs] {
    InvalidNode,
    InvalidNode,
    InvalidNode,
    InvalidNode,
    InvalidNode,
    InvalidNode,
    InvalidNode,
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
    StoreGPRRegister(X86State::REG_R11, RFLAG, 8);

    auto RIPAfterInst = GetRelocatedPC(Op);
    StoreGPRRegister(X86State::REG_RCX, RIPAfterInst, 8);
  }

  auto SyscallOp = _Syscall(
    Arguments[0],
    Arguments[1],
    Arguments[2],
    Arguments[3],
    Arguments[4],
    Arguments[5],
    Arguments[6],
    DefaultSyscallFlags);

  if (OSABI != FEXCore::HLE::SyscallOSABI::OS_HANGOVER &&
      (DefaultSyscallFlags & FEXCore::IR::SyscallFlags::NORETURNEDRESULT) != FEXCore::IR::SyscallFlags::NORETURNEDRESULT) {
    // Hangover doesn't want us returning a result here
    // syscall is being abused as a thunk for now.
    StoreGPRRegister(X86State::REG_RAX, SyscallOp);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_BLOCK_END) {
    // RIP could have been updated after coming back from the Syscall.
    NewRIP = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, rip));
    CalculateDeferredFlags();
    _ExitFunction(NewRIP);
  }
}

void OpDispatchBuilder::ThunkOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();
  uint8_t *sha256 = (uint8_t *)(Op->PC + 2);

  if (CTX->Config.Is64BitMode) {
    // x86-64 ABI puts the function argument in RDI
    _Thunk(
      LoadGPRRegister(X86State::REG_RDI),
      *reinterpret_cast<SHA256Sum*>(sha256)
    );
  }
  else {
    // x86 fastcall ABI puts the function argument in ECX
    _Thunk(
      LoadGPRRegister(X86State::REG_RCX),
      *reinterpret_cast<SHA256Sum*>(sha256)
    );
  }

  auto Constant = _Constant(GPRSize);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewRIP = _LoadMem(GPRClass, GPRSize, OldSP, GPRSize);
  OrderedNode *NewSP = _Add(IR::SizeToOpSize(GPRSize), OldSP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
  CalculateDeferredFlags();

  // Store the new RIP
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::LEAOp(OpcodeArgs) {
  // LEA specifically ignores segment prefixes
  const auto SrcSize = GetSrcSize(Op);

  if (CTX->Config.Is64BitMode) {
    const uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 :
      X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST ? 8 : 4;

    auto Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, {.LoadData = false});
    if (DstSize != SrcSize) {
      // If the SrcSize isn't the DstSize then we need to zero extend.
      const uint8_t GPRSize = CTX->GetGPRSize();
      Src = _Bfe(IR::SizeToOpSize(GPRSize), SrcSize * 8, 0, Src);
    }
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
  }
  else {
    uint32_t DstSize = X86Tables::DecodeFlags::GetOpAddr(Op->Flags, 0) == X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST ? 2 : 4;

    auto Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags, {.LoadData = false});
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
    // Deferred flags are invalidated now
    InvalidateDeferredFlags();
  }
  else {
    // Calculate flags early.
    CalculateDeferredFlags();
  }

  auto Constant = _Constant(GPRSize);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewRIP = _LoadMem(GPRClass, GPRSize, OldSP, GPRSize);

  OrderedNode *NewSP;
  if (Op->OP == 0xC2) {
    auto Offset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    NewSP = _Add(IR::SizeToOpSize(GPRSize), _Add(IR::SizeToOpSize(GPRSize), OldSP, Constant), Offset);
  }
  else {
    NewSP = _Add(IR::SizeToOpSize(GPRSize), OldSP, Constant);
  }

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
  CalculateDeferredFlags();

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
    LogMan::Msg::EFmt("IRET only implemented for 64bit and 32bit sizes");
    DecodeFailure = true;
    return;
  }

  // Calculate flags early.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();

  auto Constant = _Constant(GPRSize);

  auto SP = LoadGPRRegister(X86State::REG_RSP);

  // RIP (64/32/16 bits)
  auto NewRIP = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
  SP = _Add(IR::SizeToOpSize(GPRSize), SP, Constant);
  // CS (lower 16 used)
  auto NewSegmentCS = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
  _StoreContext(2, GPRClass, NewSegmentCS, offsetof(FEXCore::Core::CPUState, cs_idx));
  UpdatePrefixFromSegment(NewSegmentCS, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX);

  SP = _Add(IR::SizeToOpSize(GPRSize), SP, Constant);
  //eflags (lower 16 used)
  auto eflags = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
  SetPackedRFLAG(false, eflags);
  SP = _Add(IR::SizeToOpSize(GPRSize), SP, Constant);

  if (CTX->Config.Is64BitMode) {
    // RSP and SS only happen in 64-bit mode or if this is a CPL mode jump!
    // FEX doesn't support a CPL mode switch, so don't need to worry about this on 32-bit
    StoreGPRRegister(X86State::REG_RSP, _LoadMem(GPRClass, GPRSize, SP, GPRSize));

    SP = _Add(IR::SizeToOpSize(GPRSize), SP, Constant);
    // ss
    auto NewSegmentSS = _LoadMem(GPRClass, GPRSize, SP, GPRSize);
    _StoreContext(2, GPRClass, NewSegmentSS, offsetof(FEXCore::Core::CPUState, ss_idx));
    UpdatePrefixFromSegment(NewSegmentSS, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX);

    _Add(IR::SizeToOpSize(GPRSize), SP, Constant);
  }
  else {
    // Store the stack in 32-bit mode
    StoreGPRRegister(X86State::REG_RSP, SP);
  }

  CalculateDeferredFlags();
  _ExitFunction(NewRIP);
  BlockSetRIP = true;
}

void OpDispatchBuilder::CallbackReturnOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  // Store the new RIP
  _CallbackReturn();
  auto NewRIP = _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, rip));
  CalculateDeferredFlags();
  // This ExitFunction won't actually get hit but needs to exist
  _ExitFunction(NewRIP);
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
    LOGMAN_MSG_A_FMT("Unknown ALU Op: 0x{:x}", Op->OP);
  break;
  };
#undef OPD

  ALUOpImpl(Op, IROp, AtomicIROp, 1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::ADCOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  uint8_t Size = GetDstSize(Op);
  const auto OpSize = IR::SizeToOpSize(std::max<uint8_t>(4u, Size));

  OrderedNode *Before{};
  if (DestIsLockedMem(Op)) {
    auto ALUOp = _Adc(OpSize, _Constant(0), Src);

    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    Before = _AtomicFetchAdd(IR::SizeToOpSize(Size), ALUOp, DestMem);
  }
  else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  OrderedNode *Result = CalculateFlags_ADC(Size, Before, Src);
  if (!DestIsLockedMem(Op))
    StoreResult(GPRClass, Op, Result, -1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::SBBOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  auto Size = GetDstSize(Op);
  const auto OpSize = IR::SizeToOpSize(std::max<uint8_t>(4u, Size));

  OrderedNode *Result{};
  OrderedNode *Before{};
  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);

    auto SrcPlusCF = _Adc(OpSize, _Constant(0), Src);
    Before = _AtomicFetchSub(IR::SizeToOpSize(Size), SrcPlusCF, DestMem);
  }
  else {
    Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  Result = CalculateFlags_SBB(Size, Before, Src);

  if (!DestIsLockedMem(Op))
    StoreResult(GPRClass, Op, Result, -1);
}

void OpDispatchBuilder::SALCOp(OpcodeArgs) {
  CalculateDeferredFlags();

  auto Result = _NZCVSelect(OpSize::i32Bit, CondClassType{COND_UGE} /* CF = 1 */,
                            _Constant(0xffffffff), _Constant(0));

  StoreResult(GPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PUSHOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  auto OldSP = LoadGPRRegister(X86State::REG_RSP);

  const uint8_t GPRSize = CTX->GetGPRSize();
  auto NewSP = _Push(GPRSize, Size, Src, OldSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
}

void OpDispatchBuilder::PUSHREGOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest , Op->Flags,
                                {.AllowUpperGarbage = true});

  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  const uint8_t GPRSize = CTX->GetGPRSize();
  auto NewSP = _Push(GPRSize, Size, Src, OldSP);
  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
}

void OpDispatchBuilder::PUSHAOp(OpcodeArgs) {
  // 32bit only
  const uint8_t Size = GetSrcSize(Op);

  auto OldSP = LoadGPRRegister(X86State::REG_RSP);

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
  const uint8_t GPRSize = CTX->GetGPRSize();

  Src = LoadGPRRegister(X86State::REG_RAX);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  Src = LoadGPRRegister(X86State::REG_RCX);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  Src = LoadGPRRegister(X86State::REG_RDX);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  Src = LoadGPRRegister(X86State::REG_RBX);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  // Push old-sp
  NewSP = _Push(GPRSize, Size, OldSP, NewSP);

  Src = LoadGPRRegister(X86State::REG_RBP);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  Src = LoadGPRRegister(X86State::REG_RSI);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  Src = LoadGPRRegister(X86State::REG_RDI);
  NewSP = _Push(GPRSize, Size, Src, NewSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP, 4);
}

template<uint32_t SegmentReg>
void OpDispatchBuilder::PUSHSegmentOp(OpcodeArgs) {
  const uint8_t SrcSize = GetSrcSize(Op);
  const uint8_t DstSize = GetDstSize(Op);

  auto OldSP = LoadGPRRegister(X86State::REG_RSP);

  OrderedNode *Src{};
  if (!CTX->Config.Is64BitMode()) {
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
      default: break; // Do nothing
    }
  }
  else {
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
      default: break; // Do nothing
    }
  }

  const uint8_t GPRSize = CTX->GetGPRSize();
  // Store our value to the new stack location
  // AMD hardware zexts segment selector to 32bit
  // Intel hardware inserts segment selector
  auto NewSP = _Push(GPRSize, DstSize, Src, OldSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
}

void OpDispatchBuilder::POPOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  auto Constant = _Constant(Size);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewGPR = _LoadMem(GPRClass, Size, OldSP, Size);
  auto NewSP = _Add(OpSize::i64Bit, OldSP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  // Store what we loaded from the stack
  StoreResult(GPRClass, Op, NewGPR, -1);
}

void OpDispatchBuilder::POPAOp(OpcodeArgs) {
  // 32bit only
  const uint8_t Size = GetSrcSize(Op);

  auto Constant = _Constant(Size);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);

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
  StoreGPRRegister(X86State::REG_RDI, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RSI, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RBP, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, _Constant(Size * 2));

  // Skip SP loading
  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RBX, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RDX, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RCX, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  Src = _LoadMem(GPRClass, Size, NewSP, Size);
  StoreGPRRegister(X86State::REG_RAX, Src, Size);
  NewSP = _Add(OpSize::i64Bit, NewSP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
}

template<uint32_t SegmentReg>
void OpDispatchBuilder::POPSegmentOp(OpcodeArgs) {
  const uint8_t SrcSize = GetSrcSize(Op);
  const uint8_t DstSize = GetDstSize(Op);

  auto Constant = _Constant(SrcSize);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewSegment = _LoadMem(GPRClass, SrcSize, OldSP, SrcSize);
  auto NewSP = _Add(OpSize::i64Bit, OldSP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, es_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      _StoreContext(DstSize, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, cs_idx));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
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
  // First we move RBP in to RSP and then behave effectively like a pop
  const uint8_t Size = GetSrcSize(Op);

  auto Constant = _Constant(Size);
  auto OldBP = LoadGPRRegister(X86State::REG_RBP);

  auto NewGPR = _LoadMem(GPRClass, Size, OldBP, Size);
  auto NewSP = _Add(IR::SizeToOpSize(Size), OldBP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  // Store what we loaded to RBP
  StoreGPRRegister(X86State::REG_RBP, NewGPR);
}

void OpDispatchBuilder::CALLOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  BlockSetRIP = true;

  // ABI Optimization: Flags don't survive calls or rets
  if (CTX->Config.ABILocalFlags) {
    _InvalidateFlags(~0UL); // all flags
    // Deferred flags are invalidated now
    InvalidateDeferredFlags();
  }
  else {
    // Calculate flags early.
    CalculateDeferredFlags();
  }

  auto ConstantPC = GetRelocatedPC(Op);

  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *NewRIP = _Add(IR::SizeToOpSize(GPRSize), ConstantPC, JMPPCOffset);

  // Push the return address.
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewSP = _Push(GPRSize, GPRSize, ConstantPC, OldSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  const uint64_t NextRIP = Op->PC + Op->InstSize;
  LOGMAN_THROW_A_FMT(Op->Src[0].IsLiteral(), "Had wrong operand type");
  const uint64_t TargetRIP = Op->PC + Op->InstSize + Op->Src[0].Data.Literal.Value;

  CalculateDeferredFlags();
  if (NextRIP != TargetRIP) {
    // Store the RIP
    _ExitFunction(NewRIP); // If we get here then leave the function now
  }
  else {
    NeedsBlockEnd = true;
  }
}

void OpDispatchBuilder::CALLAbsoluteOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  const uint8_t Size = GetSrcSize(Op);
  OrderedNode *JMPPCOffset = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  auto ConstantPCReturn = GetRelocatedPC(Op);

  // Push the return address.
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  auto NewSP = _Push(Size, Size, ConstantPCReturn, OldSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  // Store the RIP
  CalculateDeferredFlags();
  _ExitFunction(JMPPCOffset); // If we get here then leave the function now
}

OrderedNode *OpDispatchBuilder::SelectBit(OrderedNode *Cmp, IR::OpSize ResultSize, OrderedNode *TrueValue, OrderedNode *FalseValue) {
  uint64_t TrueConst, FalseConst;
  if (IsValueConstant(WrapNode(TrueValue), &TrueConst) &&
      IsValueConstant(WrapNode(FalseValue), &FalseConst) &&
      TrueConst == 1 &&
      FalseConst == 0) {

      return _And(ResultSize, Cmp, _Constant(1));
  }

  SaveNZCV();
  _TestNZ(OpSize::i32Bit, Cmp, _Constant(1));
  return _NZCVSelect(ResultSize, CondClassType{COND_NEQ},
                     TrueValue, FalseValue);
}

std::pair<bool, CondClassType> OpDispatchBuilder::DecodeNZCVCondition(uint8_t OP) const {
  switch (OP) {
    case 0x0: { // JO - Jump if OF == 1
      return {false, CondClassType{COND_FU}};
    }
    case 0x1:{ // JNO - Jump if OF == 0
      return {false, CondClassType{COND_FNU}};
    }
    case 0x2: { // JC - Jump if CF == 1
      return {false, CondClassType{COND_UGE}};
    }
    case 0x3: { // JNC - Jump if CF == 0
      return {false, CondClassType{COND_ULT}};
    }
    case 0x4: { // JE - Jump if ZF == 1
      return {false, CondClassType{COND_EQ}};
    }
    case 0x5: { // JNE - Jump if ZF == 0
      return {false, CondClassType{COND_NEQ}};
    }
    case 0x8: { // JS - Jump if SF == 1
      return {false, CondClassType{COND_MI}};
    }
    case 0x9: { // JNS - Jump if SF == 0
      return {false, CondClassType{COND_PL}};
    }
    case 0xC: { // SF <> OF
      return {false, CondClassType{COND_SLT}};
    }
    case 0xD: { // SF = OF
      return {false, CondClassType{COND_SGE}};
    }
    case 0xE: {// ZF = 1 || SF <> OF
      return {false, CondClassType{COND_SLE}};
    }
    case 0xF: {// ZF = 0 && SF = OF
      return {false, CondClassType{COND_SGT}};
    }
    default:
      // Other conditions do not map directly, caller gets to deal with it.
      return {true, CondClassType{0}};
  }
}

OrderedNode *OpDispatchBuilder::SelectCC(uint8_t OP, IR::OpSize ResultSize, OrderedNode *TrueValue, OrderedNode *FalseValue) {
  auto [Complex, Cond] = DecodeNZCVCondition(OP);
  if (!Complex)
      return _NZCVSelect(ResultSize, Cond, TrueValue, FalseValue);

  switch (OP) {
    case 0x6: { // JNA - Jump if CF == 1 || ZC == 1
      // (A || B) ? C : D is equivalent to B ? C : (A ? C : D)
      auto TMP = _NZCVSelect(ResultSize, CondClassType{COND_UGE}, TrueValue, FalseValue);
      return _NZCVSelect(ResultSize, CondClassType{COND_EQ}, TrueValue, TMP);
    }
    case 0x7: { // JA - Jump if CF == 0 && ZF == 0
      // (A && B) ? C : D is equivalent to B ? (A ? C : D) : D
      auto TMP = _NZCVSelect(ResultSize, CondClassType{COND_ULT}, TrueValue, FalseValue);
      return _NZCVSelect(ResultSize, CondClassType{COND_NEQ}, TMP, FalseValue);
    }
    case 0xA: { // JP - Jump if PF == 1
      // Raw value contains inverted PF in bottom bit
      return SelectBit(LoadPFRaw(true), ResultSize, TrueValue, FalseValue);
    }
    case 0xB: { // JNP - Jump if PF == 0
      return SelectBit(LoadPFRaw(false), ResultSize, TrueValue, FalseValue);
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown CC Op: 0x{:x}\n", OP);
      return nullptr;
  }
}

void OpDispatchBuilder::SETccOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  auto SrcCond = SelectCC(Op->OP & 0xF, OpSize::i64Bit, OneConst, ZeroConst);

  StoreResult(GPRClass, Op, SrcCond, -1);
}

void OpDispatchBuilder::CMOVOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // Calculate flags early.
  CalculateDeferredFlags();

  // Destination is always a GPR.
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);
  OrderedNode *Src{};
  if (Op->Src[0].IsGPR()) {
    Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  }

  auto SrcCond = SelectCC(Op->OP & 0xF, IR::SizeToOpSize(std::max<uint8_t>(4u, GetSrcSize(Op))), Src, Dest);

  StoreResult(GPRClass, Op, SrcCond, -1);
}

void OpDispatchBuilder::CondJUMPOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  // Jump instruction only uses up to 32-bit signed displacement
  LOGMAN_THROW_A_FMT(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");
  int64_t TargetOffset = Op->Src[0].Data.Literal.Value;
  uint64_t InstRIP = Op->PC + Op->InstSize;
  uint64_t Target = InstRIP + TargetOffset;

  if (CTX->GetGPRSize() == 4) {
    // If the GPRSize is 4 then we need to be careful about PC wrapping
    if (TargetOffset < 0 && -TargetOffset > InstRIP) {
      // Invert the signed value if we are underflowing
      TargetOffset = 0x1'0000'0000ULL + TargetOffset;
    }
    else if (TargetOffset >= 0 && Target >= 0x1'0000'0000ULL) {
      // We are overflowing, wrap around
      TargetOffset = TargetOffset - 0x1'0000'0000ULL;
    }
    Target &= 0xFFFFFFFFU;
  }

  CalculateDeferredFlags();
  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  // Fallback
  {
    IRPair<IR::IROp_CondJump> CondJump_;
    auto [Complex, SimpleCond] = DecodeNZCVCondition(Op->OP & 0xF);
    if (Complex) {
      auto TakeBranch = _Constant(1);
      auto DoNotTakeBranch = _Constant(0);
      auto SrcCond = SelectCC(Op->OP & 0xF, OpSize::i64Bit, TakeBranch, DoNotTakeBranch);
      CondJump_ = CondJump(SrcCond);
    } else {
      CondJump_ = CondJumpNZCV(SimpleCond);
    }

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      auto NewRIP = GetRelocatedPC(Op, TargetOffset);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place it after this block for fallthrough optimization
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block
      auto RIPTargetConst = GetRelocatedPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::CondJUMPRCXOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;
  uint8_t JcxGPRSize = CTX->GetGPRSize();
  JcxGPRSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? (JcxGPRSize >> 1) : JcxGPRSize;

  IRPair<IROp_Constant> TakeBranch;
  IRPair<IROp_Constant> DoNotTakeBranch;
  TakeBranch = _Constant(1);
  DoNotTakeBranch = _Constant(0);

  LOGMAN_THROW_A_FMT(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[0].Data.Literal.Value;

  OrderedNode *CondReg = LoadGPRRegister(X86State::REG_RCX, JcxGPRSize);

  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  auto CurrentBlock = GetCurrentBlock();

  {
    auto CondJump_ = CondJump(CondReg, {COND_EQ});

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      auto NewRIP = GetRelocatedPC(Op, Op->Src[0].Data.Literal.Value);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place it after the current block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(CurrentBlock);
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block
      auto RIPTargetConst = GetRelocatedPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::LoopOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  bool CheckZF = Op->OP != 0xE2;
  bool ZFTrue = Op->OP == 0xE1;

  BlockSetRIP = true;
  uint32_t SrcSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) ? 4 : 8;
  auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;

  if (!CTX->Config.Is64BitMode) {
    // RCX size is 32-bit or 16-bit when executing in 32-bit mode.
    SrcSize >>= 1;
    OpSize = OpSize::i32Bit;
  }

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Target = Op->PC + Op->InstSize + Op->Src[1].Data.Literal.Value;

  OrderedNode *CondReg = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  CondReg = _Sub(OpSize, CondReg, _Constant(SrcSize * 8, 1));
  StoreResult(GPRClass, Op, Op->Src[0], CondReg, -1);

  // If LOOPE then jumps to target if RCX != 0 && ZF == 1
  // If LOOPNE then jumps to target if RCX != 0 && ZF == 0
  //
  // To handle efficiently, smash RCX to zero if ZF is wrong (1 csel).
  if (CheckZF) {
    CondReg = _NZCVSelect(OpSize, {ZFTrue ? COND_EQ : COND_NEQ}, CondReg, _Constant(0));
  }

  CalculateDeferredFlags();
  auto TrueBlock = JumpTargets.find(Target);
  auto FalseBlock = JumpTargets.find(Op->PC + Op->InstSize);

  {
    auto CondJump_ = CondJump(CondReg);

    // Taking branch block
    if (TrueBlock != JumpTargets.end()) {
      SetTrueJumpTarget(CondJump_, TrueBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      auto JumpTarget = CreateNewCodeBlockAtEnd();
      SetTrueJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      auto NewRIP = GetRelocatedPC(Op, Op->Src[1].Data.Literal.Value);

      // Store the new RIP
      _ExitFunction(NewRIP);
    }

    // Failure to take branch
    if (FalseBlock != JumpTargets.end()) {
      SetFalseJumpTarget(CondJump_, FalseBlock->second.BlockEntry);
    }
    else {
      // Make sure to start a new block after ending this one
      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetFalseJumpTarget(CondJump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();

      // Leave block
      auto RIPTargetConst = GetRelocatedPC(Op);

      // Store the new RIP
      _ExitFunction(RIPTargetConst);
    }
  }
}

void OpDispatchBuilder::JUMPOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  BlockSetRIP = true;

  // Jump instruction only uses up to 32-bit signed displacement
  LOGMAN_THROW_A_FMT(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");
  int64_t TargetOffset = Op->Src[0].Data.Literal.Value;
  uint64_t InstRIP = Op->PC + Op->InstSize;
  uint64_t TargetRIP = InstRIP + TargetOffset;

  if (CTX->GetGPRSize() == 4) {
    // If the GPRSize is 4 then we need to be careful about PC wrapping
    if (TargetOffset < 0 && -TargetOffset > InstRIP) {
      // Invert the signed value if we are underflowing
      TargetOffset = 0x1'0000'0000ULL + TargetOffset;
    }
    else if (TargetOffset >= 0 && TargetRIP >= 0x1'0000'0000ULL) {
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
    }
    else {
      // If the block isn't a jump target then we need to create an exit block
      auto Jump_ = Jump();

      // Place after this block for fallthrough behavior
      auto JumpTarget = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(Jump_, JumpTarget);
      SetCurrentCodeBlock(JumpTarget);
      StartNewBlock();
      _ExitFunction(GetRelocatedPC(Op, TargetOffset));
    }
    return;
  }

  // Fallback
  {
    auto RIPTargetConst = GetRelocatedPC(Op);
    auto NewRIP = _Add(OpSize::i64Bit, _Constant(TargetOffset), RIPTargetConst);

    // Store the new RIP
    _ExitFunction(NewRIP);
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
  CalculateDeferredFlags();

  // Store the new RIP
  _ExitFunction(RIPOffset);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::TESTOp(OpcodeArgs) {
  // TEST is an instruction that does an AND between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, 
                                {.AllowUpperGarbage = true});
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags,
                                 {.AllowUpperGarbage = true});

  auto Size = GetDstSize(Op);

  // Optimize out masking constants
  uint64_t Const;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    if (Const == (Size == 8 ? ~0ULL : ((1ull << Size * 8) - 1)))
      Src = Dest;
  }

  HandleNZ00Write();
  CalculatePF(_AndWithFlags(IR::SizeToOpSize(Size), Dest, Src));
  _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
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

  OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], Size, Op->Flags);
  if (Size == 2) {
    // This'll make sure to insert in to the lower 16bits without modifying upper bits
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, Size, -1);
  }
  else if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // With REX.W then Sext
    Src = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src);
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
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  Src = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src);
  StoreResult(GPRClass, Op, Op->Dest, Src, -1);
}

void OpDispatchBuilder::MOVZXOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // Store result implicitly zero extends
  StoreResult(GPRClass, Op, Src, -1);
}

template<uint32_t SrcIndex>
void OpDispatchBuilder::CMPOp(OpcodeArgs) {
  // CMP is an instruction that does a SUB between the sources
  // Result isn't stored in result, only writes to flags
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.AllowUpperGarbage = true});
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  GenerateFlags_SUB(Op, Dest, Src);
}

void OpDispatchBuilder::CQOOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  auto Size = GetSrcSize(Op);
  OrderedNode *Upper = _Sbfe(OpSize::i64Bit, 1, Size * 8 - 1, Src);

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
    if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
      // If this instruction has a REP prefix then this is architecturally
      // defined to be a `PAUSE` instruction. On older processors this ends up
      // being a true `REP NOP` which is why they stuck this here.
      _Yield();
    }
    return;
  }

  // AllowUpperGarbage: OK to allow as it will be overwritten by StoreResult.
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags,
                                {.AllowUpperGarbage = true});
  if (DestIsMem(Op)) {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

    Dest = AppendSegmentOffset(Dest, Op->Flags);

    auto Result = _AtomicSwap(OpSizeFromSrc(Op), Src, Dest);
    StoreResult(GPRClass, Op, Op->Src[0], Result, -1);
  }
  else {
    // AllowUpperGarbage: OK to allow as it will be overwritten by StoreResult.
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    // Swap the contents
    // Order matters here since we don't want to swap context contents for one that effects the other
    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
    StoreResult(GPRClass, Op, Op->Src[0], Dest, -1);
  }
}

void OpDispatchBuilder::CDQOp(OpcodeArgs) {
  uint8_t DstSize = GetDstSize(Op);
  uint8_t SrcSize = DstSize >> 1;
  OrderedNode *Src = LoadGPRRegister(X86State::REG_RAX, SrcSize, 0, true);

  Src = _Sbfe(DstSize <= 4 ? OpSize::i32Bit : OpSize::i64Bit, SrcSize * 8, 0,
              Src);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, DstSize, -1);
}

void OpDispatchBuilder::SAHFOp(OpcodeArgs) {
  // Extract AH
  OrderedNode *Src = LoadGPRRegister(X86State::REG_RAX, 1, 8);

  // Clear bits that aren't supposed to be set
  Src = _Andn(OpSize::i64Bit, Src, _Constant(0b101000));

  // Set the bit that is always set here
  Src = _Or(OpSize::i64Bit, Src, _Constant(0b10));

  // Store the lower 8 bits in to RFLAGS
  SetPackedRFLAG(true, Src);
}
void OpDispatchBuilder::LAHFOp(OpcodeArgs) {
  // Load the lower 8 bits of the Rflags register
  auto RFLAG = GetPackedRFLAG(0xFF);

  // Store the lower 8 bits of the rflags register in to AH
  StoreGPRRegister(X86State::REG_RAX, RFLAG, 1, 8);
}

void OpDispatchBuilder::FLAGControlOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  switch (Op->OP) {
  case 0xF5: // CMC
    CarryInvert();
  break;
  case 0xF8: // CLC
    SetRFLAG(_Constant(0), FEXCore::X86State::RFLAG_CF_RAW_LOC);
  break;
  case 0xF9: // STC
    SetRFLAG(_Constant(1), FEXCore::X86State::RFLAG_CF_RAW_LOC);
  break;
  case 0xFC: // CLD
    SetRFLAG(_Constant(0), FEXCore::X86State::RFLAG_DF_RAW_LOC);
  break;
  case 0xFD: // STD
    SetRFLAG(_Constant(1), FEXCore::X86State::RFLAG_DF_RAW_LOC);
  break;
  }
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
    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], 2, Op->Flags);

    switch (Op->Dest.Data.GPR.GPR) {
      case FEXCore::X86State::REG_RAX: // ES
      case FEXCore::X86State::REG_R8: // ES
        _StoreContext(2, GPRClass, Src, offsetof(FEXCore::Core::CPUState, es_idx));
        UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX);
        break;
      case FEXCore::X86State::REG_RBX: // DS
      case FEXCore::X86State::REG_R11: // DS
        _StoreContext(2, GPRClass, Src, offsetof(FEXCore::Core::CPUState, ds_idx));
        UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);
        break;
      case FEXCore::X86State::REG_RCX: // CS
      case FEXCore::X86State::REG_R9: // CS
        // CPL3 can't write to this
        _Break(FEXCore::IR::BreakDefinition {
            .ErrorRegister = 0,
            .Signal = SIGILL,
            .TrapNumber = 0,
            .si_code = 0,
        });
        break;
      case FEXCore::X86State::REG_RDX: // SS
      case FEXCore::X86State::REG_R10: // SS
        _StoreContext(2, GPRClass, Src, offsetof(FEXCore::Core::CPUState, ss_idx));
        UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX);
        break;
      case FEXCore::X86State::REG_RBP: // GS
      case FEXCore::X86State::REG_R13: // GS
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(2, GPRClass, Src, offsetof(FEXCore::Core::CPUState, gs_idx));
          UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX);
        } else {
          LogMan::Msg::EFmt("We don't support modifying GS selector in 64bit mode!");
          DecodeFailure = true;
        }
        break;
      case FEXCore::X86State::REG_RSP: // FS
      case FEXCore::X86State::REG_R12: // FS
        if (!CTX->Config.Is64BitMode) {
          _StoreContext(2, GPRClass, Src, offsetof(FEXCore::Core::CPUState, fs_idx));
          UpdatePrefixFromSegment(Src, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX);
        } else {
          LogMan::Msg::EFmt("We don't support modifying FS selector in 64bit mode!");
          DecodeFailure = true;
        }
        break;
      default:
        LogMan::Msg::EFmt("Unknown segment register: {}", Op->Dest.Data.GPR.GPR);
        DecodeFailure = true;
        break;
    }
  }
  else {
    OrderedNode *Segment{};

    switch (Op->Src[0].Data.GPR.GPR) {
      case FEXCore::X86State::REG_RAX: // ES
      case FEXCore::X86State::REG_R8: // ES
        Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, es_idx));
        break;
      case FEXCore::X86State::REG_RBX: // DS
      case FEXCore::X86State::REG_R11: // DS
        Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, ds_idx));
        break;
      case FEXCore::X86State::REG_RCX: // CS
      case FEXCore::X86State::REG_R9: // CS
        Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, cs_idx));
        break;
      case FEXCore::X86State::REG_RDX: // SS
      case FEXCore::X86State::REG_R10: // SS
        Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, ss_idx));
        break;
      case FEXCore::X86State::REG_RBP: // GS
      case FEXCore::X86State::REG_R13: // GS
        if (CTX->Config.Is64BitMode) {
          Segment = _Constant(0);
        }
        else {
          Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, gs_idx));
        }
        break;
      case FEXCore::X86State::REG_RSP: // FS
      case FEXCore::X86State::REG_R12: // FS
        if (CTX->Config.Is64BitMode) {
          Segment = _Constant(0);
        }
        else {
          Segment = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, fs_idx));
        }
        break;
      default:
        LogMan::Msg::EFmt("Unknown segment register: {}", Op->Dest.Data.GPR.GPR);
        DecodeFailure = true;
        return;
    }
    if (DestIsMem(Op)) {
      // If the destination is memory then we always store 16-bits only
      StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Segment, 2, -1);
    }
    else {
      // If the destination is a GPR then we follow register storing rules
      StoreResult(GPRClass, Op, Segment, -1);
    }
  }
}

void OpDispatchBuilder::MOVOffsetOp(OpcodeArgs) {
  OrderedNode *Src;

  switch (Op->OP) {
  case 0xA0:
  case 0xA1:
    // Source is memory(literal)
    // Dest is GPR
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.ForceLoad = true});
    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
    break;
  case 0xA2:
  case 0xA3:
    // Source is GPR
    // Dest is memory(literal)
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    // This one is a bit special since the destination is a literal
    // So the destination gets stored in Src[1]
    StoreResult(GPRClass, Op, Op->Src[1], Src, -1);
    break;
  }
}

void OpDispatchBuilder::CPUIDOp(OpcodeArgs) {
  const auto GPRSize = CTX->GetGPRSize();

  OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  OrderedNode *Leaf = LoadGPRRegister(X86State::REG_RCX);

  auto Res = _CPUID(Src, Leaf);

  OrderedNode *Result_Lower = _ExtractElementPair(OpSize::i64Bit, Res, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(OpSize::i64Bit, Res, 1);

  StoreGPRRegister(X86State::REG_RAX, _Bfe(OpSize::i64Bit, 32, 0,  Result_Lower));
  StoreGPRRegister(X86State::REG_RBX, _Bfe(OpSize::i64Bit, 32, 32, Result_Lower));
  StoreGPRRegister(X86State::REG_RDX, _Bfe(OpSize::i64Bit, 32, 32, Result_Upper));
  StoreGPRRegister(X86State::REG_RCX, _Bfe(OpSize::i64Bit, 32, 0,  Result_Upper));
}

void OpDispatchBuilder::XGetBVOp(OpcodeArgs) {
  OrderedNode *Function = LoadGPRRegister(X86State::REG_RCX);

  auto Res = _XGetBV(Function);

  OrderedNode *Result_Lower = _ExtractElementPair(OpSize::i32Bit, Res, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(OpSize::i32Bit, Res, 1);

  StoreGPRRegister(X86State::REG_RAX, Result_Lower);
  StoreGPRRegister(X86State::REG_RDX, Result_Upper);
}

template<bool SHL1Bit>
void OpDispatchBuilder::SHLOp(OpcodeArgs) {
  OrderedNode *Src{};
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  if constexpr (SHL1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags);
  }
  const auto Size = GetSrcBitSize(Op);

  OrderedNode *Result = _Lshl(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Src);
  StoreResult(GPRClass, Op, Result, -1);

  if (Size < 32) {
    Result = _Bfe(OpSize::i32Bit, Size, 0, Result);
  }

  if constexpr (SHL1Bit) {
    GenerateFlags_ShiftLeftImmediate(Op, Result, Dest, 1);
  }
  else {
    GenerateFlags_ShiftLeft(Op, Result, Dest, Src);
  }
}

void OpDispatchBuilder::SHLImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift &= 0x3F;
  } else {
    Shift &= 0x1F;
  }

  OrderedNode *Src = _Constant(Size, Shift);
  OrderedNode *Result = _Lshl(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Src);

  StoreResult(GPRClass, Op, Result, -1);

  GenerateFlags_ShiftLeftImmediate(Op, Result, Dest, Shift);
}

template<bool SHR1Bit>
void OpDispatchBuilder::SHROp(OpcodeArgs) {
  OrderedNode *Src;
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  if constexpr (SHR1Bit) {
    Src = _Constant(1);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags);
  }

  auto ALUOp = _Lshr(IR::SizeToOpSize(std::max<uint8_t>(4, GetSrcSize(Op))), Dest, Src);
  StoreResult(GPRClass, Op, ALUOp, -1);

  if constexpr (SHR1Bit) {
    GenerateFlags_ShiftRightImmediate(Op, ALUOp, Dest, 1);
  }
  else {
    GenerateFlags_ShiftRight(Op, ALUOp, Dest, Src);
  }
}

void OpDispatchBuilder::SHRImmediateOp(OpcodeArgs) {
  auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift &= 0x3F;
  } else {
    Shift &= 0x1F;
  }

  OrderedNode *Src = _Constant(Size, Shift);
  auto ALUOp = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Src);

  StoreResult(GPRClass, Op, ALUOp, -1);
  GenerateFlags_ShiftRightImmediate(Op, ALUOp, Dest, Shift);
}

void OpDispatchBuilder::SHLDOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  // Allow garbage on the shift, we're masking it anyway.
  OrderedNode *Shift = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op. Arm will mask
  // by 0x3F when we do 64-bit shifts so we don't need to mask in that case,
  // since the modulo is preserved even after presubtracting Size=64 for
  // ShiftRight.
  //
  // TODO: Implement this optimization, it requires turning the shift=0 cases
  // into (shift&0xc0) bit tests which is a bit complicated for now.
  if (Size == 64) {
    Shift = _And(OpSize::i64Bit, Shift, _Constant(0x3F));
  } else {
    Shift = _And(OpSize::i64Bit, Shift, _Constant(0x1F));
  }

  // a64 masks the bottom bits, so if we're using a native 32/64-bit shift, we
  // can negate to do the subtract (it's congruent), which saves a constant.
  auto ShiftRight = Size >= 32 ? _Neg(OpSize::i64Bit, Shift) :
                                 _Sub(OpSize::i64Bit, _Constant(Size), Shift);

  auto Tmp1 = _Lshl(OpSize::i64Bit, Dest, Shift);
  auto Tmp2 = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Src, ShiftRight);

  OrderedNode *Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  //
  // TODO: With a backwards pass ahead-of-time, we could stick this in the
  // if(shift) used for flags.
  //
  // TODO: This whole function wants to be wrapped in the if. Maybe b/w pass is
  // a good idea after all.
  Res = _Select(FEXCore::IR::COND_EQ,
    Shift, _Constant(0),
    Dest, Res);

  StoreResult(GPRClass, Op, Res, -1);

  // No need to mask result, upper garbage is ignored in the flag calc
  GenerateFlags_ShiftLeft(Op, Res, Dest, Shift);
}

void OpDispatchBuilder::SHLDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift &= 0x3F;
  } else {
    Shift &= 0x1F;
  }

  if (Shift != 0) {
    OrderedNode *Res{};
    if (Size < 32) {
      OrderedNode *ShiftLeft = _Constant(Shift);
      auto ShiftRight = _Constant(Size - Shift);

      auto Tmp1 = _Lshl(OpSize::i64Bit, Dest, ShiftLeft);
      auto Tmp2 = _Lshr(OpSize::i32Bit, Src, ShiftRight);

      Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);
    }
    else {
      // 32-bit and 64-bit SHLD behaves like an EXTR where the lower bits are filled from the source.
      Res = _Extr(OpSizeFromSrc(Op), Dest, Src, Size - Shift);
    }

    StoreResult(GPRClass, Op, Res, -1);
    GenerateFlags_ShiftLeftImmediate(Op, Res, Dest, Shift);
  }
  else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, -1);
  }
}

void OpDispatchBuilder::SHRDOp(OpcodeArgs) {
  // Calculate flags early.
  // This instruction conditionally generates flags so we need to insure sane state going in.
  CalculateDeferredFlags();

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  OrderedNode *Shift = LoadGPRRegister(X86State::REG_RCX);

  const auto Size = GetDstBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift = _And(OpSize::i64Bit, Shift, _Constant(0x3F));
  } else {
    Shift = _And(OpSize::i64Bit, Shift, _Constant(0x1F));
  }

  auto ShiftLeft = _Sub(OpSize::i64Bit, _Constant(Size), Shift);

  auto Tmp1 = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, Shift);
  auto Tmp2 = _Lshl(OpSize::i64Bit, Src, ShiftLeft);

  OrderedNode *Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);

  // If shift count was zero then output doesn't change
  // Needs to be checked for the 32bit operand case
  // where shift = 0 and the source register still gets Zext
  Res = _Select(FEXCore::IR::COND_EQ,
    Shift, _Constant(0),
    Dest, Res);

  StoreResult(GPRClass, Op, Res, -1);

  if (Size != 64) {
    Res = _Bfe(OpSize::i64Bit, Size, 0, Res);
  }
  GenerateFlags_ShiftRight(Op, Res, Dest, Shift);
}

void OpDispatchBuilder::SHRDImmediateOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift &= 0x3F;
  } else {
    Shift &= 0x1F;
  }

  if (Shift != 0) {

    OrderedNode *Res{};
    if (Size < 32) {
      OrderedNode *ShiftRight = _Constant(Shift);
      auto ShiftLeft = _Constant(Size - Shift);

      auto Tmp1 = _Lshr(Size == 64 ? OpSize::i64Bit : OpSize::i32Bit, Dest, ShiftRight);
      auto Tmp2 = _Lshl(OpSize::i64Bit, Src, ShiftLeft);

      Res = _Or(OpSize::i64Bit, Tmp1, Tmp2);
    }
    else {
      // 32-bit and 64-bit SHRD behaves like an EXTR where the upper bits are filled from the source.
      Res = _Extr(OpSizeFromSrc(Op), Src, Dest, Shift);
    }

    StoreResult(GPRClass, Op, Res, -1);
    GenerateFlags_ShiftRightDoubleImmediate(Op, Res, Dest, Shift);
  }
  else if (Shift == 0 && Size == 32) {
    // Ensure Zext still occurs
    StoreResult(GPRClass, Op, Dest, -1);
  }
}

template<bool SHR1Bit>
void OpDispatchBuilder::ASHROp(OpcodeArgs) {
  OrderedNode *Src;
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  const auto Size = GetSrcBitSize(Op);

  if constexpr (SHR1Bit) {
    Src = _Constant(Size, 1);
  } else {
    Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags);
  }

  if (Size < 32) {
    Dest = _Sbfe(OpSize::i64Bit, Size, 0, Dest);
  }

  OrderedNode *Result = _Ashr(IR::SizeToOpSize(std::max<uint8_t>(4, GetSrcSize(Op))), Dest, Src);
  StoreResult(GPRClass, Op, Result, -1);

  if constexpr (SHR1Bit) {
    GenerateFlags_SignShiftRightImmediate(Op, Result, Dest, 1);
  } else {
    GenerateFlags_SignShiftRight(Op, Result, Dest, Src);
  }
}

void OpDispatchBuilder::ASHRImmediateOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");

  uint64_t Shift = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  if (Size == 64) {
    Shift &= 0x3F;
  } else {
    Shift &= 0x1F;
  }

  if (Size < 32) {
    Dest = _Sbfe(OpSize::i64Bit, Size, 0, Dest);
  }

  OrderedNode *Src = _Constant(Size, Shift);
  OrderedNode *Result = _Ashr(IR::SizeToOpSize(std::max<uint8_t>(4, GetOpSize(Dest))), Dest, Src);

  StoreResult(GPRClass, Op, Result, -1);

  GenerateFlags_SignShiftRightImmediate(Op, Result, Dest, Shift);
}

template<bool Left, bool IsImmediate, bool Is1Bit>
void OpDispatchBuilder::RotateOp(OpcodeArgs) {
  CalculateDeferredFlags();

  auto LoadShift = [this, Op](bool MustMask) -> OrderedNode * {
    // x86 masks the shift by 0x3F or 0x1F depending on size of op
    const uint32_t Size = GetSrcBitSize(Op);
    uint64_t Mask = Size == 64 ? 0x3F : 0x1F;

    if (Is1Bit) {
      return _Constant(1);
    } else if (IsImmediate) {
      LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
      return _Constant(Op->Src[1].Data.Literal.Value & Mask);
    } else {
      auto Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
      return MustMask ? _And(OpSize::i64Bit, Src, _Constant(Mask)) : Src;
    }
  };

  Calculate_ShiftVariable(LoadShift(true), [this, LoadShift, Op](){
    const uint32_t Size = GetSrcBitSize(Op);
    const auto OpSize = Size == 64 ? OpSize::i64Bit : OpSize::i32Bit;

    // We don't need to mask when we rematerialize since the Ror aborbs.
    auto Src = LoadShift(false);

    uint64_t Const;
    bool IsConst = IsValueConstant(WrapNode(Src), &Const);

    // We fill the upper bits so we allow garbage on load.
    auto Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    if (Size < 32) {
      // ARM doesn't support 8/16bit rotates. Emulate with an insert
      // StoreResult truncates back to a 8/16 bit value
      Dest = _Bfi(OpSize, Size, Size, Dest, Dest);

      if (Size == 8 && !(IsConst && Const < 8 && !Left)) {
        // And because the shift size isn't masked to 8 bits, we need to fill the
        // the full 32bits to get the correct result.
        Dest = _Bfi(OpSize, 16, 16, Dest, Dest);
      }
    }

    // To rotate 64-bits left, right-rotate by (64 - Shift) = -Shift mod 64.
    auto Res = _Ror(OpSize, Dest, Left ? _Neg(OpSize, Src) : Src);
    StoreResult(GPRClass, Op, Res, -1);

    // Ends up faster overall if we don't have FlagM, slower if we do...
    // If Shift != 1, OF is undefined so we choose to zero here.
    if (!CTX->HostFeatures.SupportsFlagM)
      ZeroCV();

    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Res, Left ? 0 : Size - 1, true);

    // For ROR, OF is the XOR of the new CF bit and the most significant bit of the result.
    // For ROL, OF is the LSB and MSB XOR'd together.
    // OF is architecturally only defined for 1-bit rotate.
    if (!IsConst || Const == 1) {
      auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, Left ? Size - 1 : 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Left ? 0 : Size - 2, true);
    }
  });
}

void OpDispatchBuilder::ANDNBMIOp(OpcodeArgs) {
  auto* Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  auto Dest = _Andn(OpSizeFromSrc(Op), Src2, Src1);

  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_Logical(Op, Dest, Src1, Src2);
}

void OpDispatchBuilder::BEXTRBMIOp(OpcodeArgs) {
  // Essentially (Src1 >> Start) & ((1 << Length) - 1)
  // along with some edge-case handling and flag setting.

  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  const auto Size = GetSrcSize(Op);
  const auto SrcSize = Size * 8;
  const auto MaxSrcBit = SrcSize - 1;
  auto MaxSrcBitOp = _Constant(SrcSize, MaxSrcBit);

  // Shift the operand down to the starting bit
  auto Start = _Bfe(OpSizeFromSrc(Op), 8, 0, Src2);
  auto Shifted = _Lshr(IR::SizeToOpSize(Size), Src1, Start);

  // Shifts larger than operand size need to be set to zero.
  auto SanitizedShifted = _Select(IR::COND_ULE,
                                  Start, MaxSrcBitOp,
                                  Shifted, _Constant(SrcSize, 0));

  // Now handle the length specifier.
  auto Length = _Bfe(OpSizeFromSrc(Op), 8, 8, Src2);

  // Now build up the mask
  // (1 << Length) - 1 = ~(~0 << Length)
  auto AllOnes = _Constant(~0ull);
  auto InvertedMask = _Lshl(IR::SizeToOpSize(Size), AllOnes, Length);

  // Now put it all together and make the result.
  auto Masked = _Andn(IR::SizeToOpSize(Size), SanitizedShifted, InvertedMask);

  // Sanitize the length. If it is above the max, we don't do the masking.
  auto Dest = _Select(IR::COND_ULE, Length, MaxSrcBitOp, Masked, SanitizedShifted);

  // Finally store the result.
  StoreResult(GPRClass, Op, Dest, -1);

  GenerateFlags_BEXTR(Op, Dest);
}

void OpDispatchBuilder::BLSIBMIOp(OpcodeArgs) {
  // Equivalent to performing: SRC & -SRC
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto Size = OpSizeFromSrc(Op);

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto NegatedSrc = _Neg(Size, Src);
  auto Result = _And(Size, Src, NegatedSrc);

  // ...and we're done. Painless!
  StoreResult(GPRClass, Op, Result, -1);

  GenerateFlags_BLSI(Op, Result);
}

void OpDispatchBuilder::BLSMSKBMIOp(OpcodeArgs) {
  // Equivalent to: (Src - 1) ^ Src
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto Size = OpSizeFromSrc(Op);

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _Xor(Size, _Sub(Size, Src, _Constant(1)), Src);

  StoreResult(GPRClass, Op, Result, -1);
  GenerateFlags_BLSMSK(Op, Result, Src);
}

void OpDispatchBuilder::BLSRBMIOp(OpcodeArgs) {
  // Equivalent to: (Src - 1) & Src
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto Size = OpSizeFromSrc(Op);

  auto Result = _And(Size, _Sub(Size, Src, _Constant(1)), Src);
  StoreResult(GPRClass, Op, Result, -1);

  GenerateFlags_BLSR(Op, Result, Src);
}

// Handles SARX, SHLX, and SHRX
void OpDispatchBuilder::BMI2Shift(OpcodeArgs) {
  // In the event the source is a memory operand, use the
  // exact width instead of the GPR size.
  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);
  const auto SrcSize = Op->Src[0].IsGPR() ? GPRSize : Size;

  auto* Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  auto* Shift = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], GPRSize, Op->Flags);

  auto* Result = [&]() -> OrderedNode* {
    // SARX
    if (Op->OP == 0x6F7) {
      return _Ashr(IR::SizeToOpSize(Size), Src, Shift);
    }
    // SHLX
    if (Op->OP == 0x5F7) {
      return _Lshl(IR::SizeToOpSize(Size), Src, Shift);
    }

    // SHRX
    return _Lshr(IR::SizeToOpSize(Size), Src, Shift);
  }();

  StoreResult(GPRClass, Op, Result, -1);
}

void OpDispatchBuilder::BZHI(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto OperandSize = Size * 8;

  // In 32-bit mode we only look at bottom 32-bit, no 8 or 16-bit BZHI so no
  // need to zero-extend sources
  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags,
                         {.AllowUpperGarbage = true});

  auto* Index = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags,
                           {.AllowUpperGarbage = true});

  // Clear the high bits specified by the index. A64 only considers bottom bits
  // of the shift, so we don't need to mask bottom 8-bits ourselves.
  // Out-of-bounds results ignored after.
  auto NegOne = _Constant(OperandSize, -1);
  auto Mask = _Lshl(IR::SizeToOpSize(Size), NegOne, Index);
  auto MaskResult = _Andn(IR::SizeToOpSize(Size), Src, Mask);

  // If the index is above OperandSize, we don't clear anything. BZHI only
  // considers the bottom 8-bits, so we really want to know if the bottom 8-bits
  // have their top bits set. Test exactly that.
  _TestNZ(OpSize::i64Bit, Index, _Constant(0xFF & ~(OperandSize - 1)));
  auto Result = _NZCVSelect(IR::SizeToOpSize(Size), CondClassType{COND_NEQ},
                            Src, MaskResult);
  StoreResult(GPRClass, Op, Result, -1);

  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto CF = _NZCVSelect(OpSize::i32Bit, CondClassType{COND_NEQ}, One, Zero);
  GenerateFlags_BZHI(Op, Result, CF);
}

void OpDispatchBuilder::RORX(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const auto Amount = Op->Src[1].Data.Literal.Value;
  const auto SrcSize = GetSrcSize(Op);
  const auto SrcSizeBits = SrcSize * 8;
  const auto GPRSize = CTX->GetGPRSize();

  const auto DoRotation = Amount != 0 && Amount < SrcSizeBits;
  const auto IsSameGPR = Op->Src[0].IsGPR() && Op->Dest.IsGPR() &&
                         Op->Src[0].Data.GPR.GPR == Op->Dest.Data.GPR.GPR;
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
    Result = _Ror(OpSizeFromSrc(Op), Src, _Constant(Amount));
  }

  StoreResult(GPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MULX(OpcodeArgs) {
  // RDX is the implied source operand in the instruction
  const auto OperandSize = GetSrcSize(Op);
  const auto OpSize = IR::SizeToOpSize(OperandSize);

  // Src1 can be a memory operand, so ensure we constrain to the
  // absolute width of the access in that scenario.
  const auto GPRSize = CTX->GetGPRSize();
  const auto Src1Size = Op->Src[1].IsGPR() ? GPRSize : OperandSize;

  OrderedNode* Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], Src1Size, Op->Flags);
  OrderedNode* Src2 = LoadGPRRegister(X86State::REG_RDX, GPRSize);

  // As per the Intel Software Development Manual, if the destination and
  // first operand correspond to the same register, then the result
  // will be the high half of the multiplication result.
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    OrderedNode* ResultHi = _UMulH(OpSize, Src1, Src2);
    StoreResult(GPRClass, Op, Op->Dest, ResultHi, -1);
  } else {
    OrderedNode* ResultLo = _UMul(OpSize, Src1, Src2);
    OrderedNode* ResultHi = _UMulH(OpSize, Src1, Src2);

    StoreResult(GPRClass, Op, Op->Src[0], ResultLo, -1);
    StoreResult(GPRClass, Op, Op->Dest, ResultHi, -1);
  }
}

void OpDispatchBuilder::PDEP(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Input = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Mask = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _PDep(OpSizeFromSrc(Op), Input, Mask);

  StoreResult(GPRClass, Op, Op->Dest, Result, -1);
}

void OpDispatchBuilder::PEXT(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->InstSize >= 4, "No masking needed");
  auto* Input = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  auto* Mask = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  auto Result = _PExt(OpSizeFromSrc(Op), Input, Mask);

  StoreResult(GPRClass, Op, Op->Dest, Result, -1);
}

void OpDispatchBuilder::ADXOp(OpcodeArgs) {
  const auto OpSize = OpSizeFromSrc(Op);

  // Calculate flags early.
  CalculateDeferredFlags();

  // Handles ADCX and ADOX

  const bool IsADCX = Op->OP == 0x1F6;

  auto* Flag = [&]() -> OrderedNode* {
    if (IsADCX) {
      return GetRFLAG(X86State::RFLAG_CF_RAW_LOC);
    } else {
      return GetRFLAG(X86State::RFLAG_OF_RAW_LOC);
    }
  }();

  auto* Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  auto* Before = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  auto ALUOp = _Add(OpSize, Src, Flag);
  auto Result = _Add(OpSize, Before, ALUOp);

  StoreResult(GPRClass, Op, Result, -1);

  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto SelectOpLT = _Select(IR::COND_ULT, Result, Src, One, Zero);
  auto SelectOpLE = _Select(IR::COND_ULE, Result, Src, One, Zero);
  auto SelectFlag = _Select(IR::COND_EQ, Flag, One, SelectOpLE, SelectOpLT);

  if (IsADCX) {
    SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(SelectFlag);
  } else {
    SetRFLAG<X86State::RFLAG_OF_RAW_LOC>(SelectFlag);
  }
}

void OpDispatchBuilder::RCROp1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  // We expliclty mask for <32-bit so allow garbage
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  const auto Size = GetSrcBitSize(Op);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  OrderedNode *Res;

  // Our new CF will be bit 0 of the source. Set upfront to avoid a move.
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Dest, 0, true);

  uint32_t Shift = 1;

  if (Size == 32 || Size == 64) {
    // Rotate and insert CF in the upper bit
    Res = _Extr(OpSizeFromSrc(Op), CF, Dest, Shift);
  }
  else {
    // Res = Src >> Shift
    Res = _Bfe(OpSize::i32Bit, Size - Shift, Shift, Dest);

    // inject the CF
    Res = _Orlshl(OpSize::i32Bit, Res, CF, Size - Shift);
  }

  StoreResult(GPRClass, Op, Res, -1);

  // OF is the top two MSBs XOR'd together
  // Only when Shift == 1, it is undefined otherwise
  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(_XorShift(OpSize::i64Bit, Res, Res, ShiftType::LSR, 1), Size - 2, true);
}

void OpDispatchBuilder::RCROp8x1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  const auto SizeBit = GetSrcBitSize(Op);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  // Our new CF will be bit (Shift - 1) of the source
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Dest, 0, true);

  // Rotate and insert CF in the upper bit
  OrderedNode *Res = _Bfe(OpSize::i32Bit, 7, 1, Dest);
  Res = _Bfi(OpSize::i32Bit, 1, 7, Res, CF);

  StoreResult(GPRClass, Op, Res, -1);

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

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  uint64_t Const;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    Const &= Mask;
    if (!Const)
      return;

    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    // Res = Src >> Shift
    OrderedNode *Res = _Lshr(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    InvalidateDeferredFlags();

    // Constant folded version of the above, with fused shifts.
    if (Const > 1)
      Res = _Orlshl(OpSize, Res, Dest, Size + 1 - Const);

    // Our new CF will be bit (Shift - 1) of the source.
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Dest, Const - 1, true);

    // Since shift != 0 we can inject the CF
    Res = _Orlshl(OpSize, Res, CF, Size - Const);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (Const == 1) {
      auto Xor = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, Size - 2, true);
    }

    StoreResult(GPRClass, Op, Res, -1);
    return;
  }

  OrderedNode *SrcMasked = _And(OpSize, Src, _Constant(Size, Mask));
  Calculate_ShiftVariable(SrcMasked, [this, Op, Size, OpSize](){
    // Rematerialize loads to avoid crossblock liveness
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});

    // Res = Src >> Shift
    OrderedNode *Res = _Lshr(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    auto One = _Constant(Size, 1);

    // Res |= (Dest << (Size - Shift + 1));
    // Expressed as Res | ((Src << (Size - Shift)) << 1) to get correct
    // behaviour for Shift without clobbering NZCV. Then observe that modulo
    // Size, Size - Shift = -Shift so we can use a simple Neg.
    //
    // The masking of Lshl means we don't need mask the source, since:
    //
    //  -(x & Mask) & Mask = (-x) & Mask
    OrderedNode *NegSrc = _Neg(OpSize, Src);
    Res = _Orlshl(OpSize, Res, _Lshl(OpSize, Dest, NegSrc), 1);

    // Our new CF will be bit (Shift - 1) of the source. this is hoisted up to
    // avoid the need to copy the source. Again, the Lshr absorbs the masking.
    auto NewCF = _Lshr(OpSize, Dest, _Sub(OpSize, Src, One));
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(NewCF, 0, true);

    // Since shift != 0 we can inject the CF
    Res = _Or(OpSize, Res, _Lshl(OpSize, CF, NegSrc));

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    auto Xor = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, Size - 2, true);

    StoreResult(GPRClass, Op, Res, -1);
  });
}

void OpDispatchBuilder::RCRSmallerOp(OpcodeArgs) {
  CalculateDeferredFlags();

  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  Src = AndConst(OpSize::i32Bit, Src, 0x1F);

  // CF only changes if we actually shifted. OF undefined if we didn't shift.
  // The result is unchanged if we didn't shift. So branch over the whole thing.
  Calculate_ShiftVariable(Src, [this, Op, Size](){
    // Rematerialized to avoid crossblock liveness
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
    OrderedNode *Tmp{};

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
    }
    else {
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

    // Entire bitfield has been setup
    // Just extract the 8 or 16bits we need
    OrderedNode *Res = _Lshr(OpSize::i32Bit, Tmp, Src);

    StoreResult(GPRClass, Op, Res, -1);

    uint64_t SrcConst;
    bool IsSrcConst = IsValueConstant(WrapNode(Src), &SrcConst);
    SrcConst &= 0x1f;

    // Our new CF will be bit (Shift - 1) of the source. 32-bit Lshr masks the
    // same as x86, but if we constant fold we must mask ourselves.
    if (IsSrcConst) {
      SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Tmp, SrcConst - 1, true);
    } else {
      auto One = _Constant(Size, 1);
      auto NewCF = _Lshr(OpSize::i32Bit, Tmp, _Sub(OpSize::i32Bit, Src, One));
      SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(NewCF, 0, true);
    }

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (!IsSrcConst || SrcConst == 1) {
      auto NewOF = _XorShift(OpSize::i32Bit, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 2, true);
    }
  });
}

void OpDispatchBuilder::RCLOp1Bit(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  const auto Size = GetSrcBitSize(Op);
  const auto OpSize = Size == 64 ? OpSize::i64Bit : OpSize::i32Bit;
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  // Rotate left and insert CF in to lowest bit
  // TODO: Use `adc Res, xzr, Dest, lsl 1` to save an instruction
  OrderedNode *Res = _Orlshl(OpSize, CF, Dest, 1);

  StoreResult(GPRClass, Op, Res, -1);

  // Our new CF will be the top bit of the source
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Dest, Size - 1, true);

  // OF is the top two MSBs XOR'd together
  // Top two MSBs is CF and top bit of result
  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(_Xor(OpSize, Res, Dest), Size - 1, true);
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

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  const auto OpSize = OpSizeFromSrc(Op);

  uint64_t Const;
  if (IsValueConstant(WrapNode(Src), &Const)) {
    Const &= Mask;
    if (!Const)
      return;

    // Res = Src << Shift
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
    OrderedNode *Res = _Lshl(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    InvalidateDeferredFlags();

    // Res |= (Src << (Size - Shift + 1));
    if (Const > 1)
      Res = _Orlshr(OpSize, Res, Dest, Size + 1 - Const);

    // Our new CF will be bit (Shift - 1) of the source
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Dest, Size - Const, true);

    // Since Shift != 0 we can inject the CF
    Res = _Orlshl(OpSize, Res, CF, Const - 1);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    if (Const == 1) {
      auto NewOF = _Xor(OpSize, Res, Dest);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 1, true);
    }

    StoreResult(GPRClass, Op, Res, -1);
    return;
  }

  OrderedNode *SrcMasked = _And(OpSize, Src, _Constant(Size, Mask));
  Calculate_ShiftVariable(SrcMasked, [this, Op, Size, OpSize](){
    // Rematerialized to avoid crossblock liveness
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

    // Res = Src << Shift
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
    OrderedNode *Res = _Lshl(OpSize, Dest, Src);
    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    // Res |= (Dest >> (Size - Shift + 1)), expressed as
    // Res | ((Dest >> (-Shift)) >> 1), since Size - Shift = -Shift mod
    // Size. The shift aborbs the masking.
    auto NegSrc = _Neg(OpSize, Src);
    Res = _Orlshr(OpSize, Res, _Lshr(OpSize, Dest, NegSrc), 1);

    // Our new CF will be bit (Shift - 1) of the source
    auto NewCF = _Lshr(OpSize, Dest, NegSrc);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(NewCF, 0, true);

    // Since Shift != 0 we can inject the CF. Shift absorbs the masking.
    OrderedNode *CFShl = _Sub(OpSize, Src, _Constant(Size, 1));
    auto TmpCF = _Lshl(OpSize, CF, CFShl);
    Res = _Or(OpSize, Res, TmpCF);

    // OF is the top two MSBs XOR'd together
    // Only when Shift == 1, it is undefined otherwise
    //
    // Note that NewCF has garbage in the upper bits, but we ignore them here
    // and mask as part of the set after.
    auto NewOF = _XorShift(OpSize, Res, NewCF, ShiftType::LSL, Size - 1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, Size - 1, true);

    StoreResult(GPRClass, Op, Res, -1);
  });
}

void OpDispatchBuilder::RCLSmallerOp(OpcodeArgs) {
  CalculateDeferredFlags();

  const auto Size = GetSrcBitSize(Op);

  // x86 masks the shift by 0x3F or 0x1F depending on size of op
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
  Src = AndConst(OpSize::i32Bit, Src, 0x1F);

  // CF only changes if we actually shifted. OF undefined if we didn't shift.
  // The result is unchanged if we didn't shift. So branch over the whole thing.
  Calculate_ShiftVariable(Src, [this, Op, Size](){
    // Rematerialized to avoid crossblock liveness
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});
    Src = AndConst(OpSize::i32Bit, Src, 0x1F);
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

    auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    OrderedNode *Tmp = _Constant(64, 0);

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
    OrderedNode *Res = _Ror(OpSize::i64Bit, Tmp, _Neg(OpSize::i32Bit, Src));

    StoreResult(GPRClass, Op, Res, -1);

    // Our new CF is now at the bit position that we are shifting
    // Either 0 if CF hasn't changed (CF is living in bit 0)
    // or higher
    auto NewCF = _Ror(OpSize::i64Bit, Tmp, _Sub(OpSize::i64Bit, _Constant(63), Src));
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(NewCF, 0, true);

    // OF is the XOR of the NewCF and the MSB of the result
    // Only defined for 1-bit rotates.
    uint64_t SrcConst;
    if (!IsValueConstant(WrapNode(Src), &SrcConst) || SrcConst == 1) {
      auto NewOF = _XorShift(OpSize::i64Bit, NewCF, Res, ShiftType::LSR, Size - 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, 0, true);
    }
  });
}

template<uint32_t SrcIndex, BTAction Action>
void OpDispatchBuilder::BTOp(OpcodeArgs) {
  OrderedNode *Value;
  OrderedNode *Src{};
  bool IsNonconstant = Op->Src[SrcIndex].IsGPR();
  uint8_t ConstantShift = 0;

  const uint32_t Size = GetDstBitSize(Op);
  const uint32_t Mask = Size - 1;

  // Deferred flags are invalidated now
  InvalidateDeferredFlags();

  if (IsNonconstant) {
    // Because we mask explicitly with And/Bfe/Sbfe after, we can allow garbage here.
    Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags,
                     {.AllowUpperGarbage = true });
  } else {
    // Can only be an immediate
    // Masked by operand size
    ConstantShift = Op->Src[SrcIndex].Data.Literal.Value & Mask;
    Src = _Constant(ConstantShift);
  }

  if (Op->Dest.IsGPR()) {
    // When the destination is a GPR, we don't care about garbage in the upper bits.
    // Load the full register.
    auto Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, CTX->GetGPRSize(), Op->Flags);
    Value = Dest;

    // Get the bit selection from the src. We need to mask for 8/16-bit, but
    // rely on the implicit masking of Lshr for native sizes.
    unsigned LshrSize = std::max<uint8_t>(4u, Size / 8);
    auto BitSelect = (Size == (LshrSize * 8)) ? Src : _And(OpSize::i64Bit, Src, _Constant(Mask));

    if (IsNonconstant) {
      Value = _Lshr(IR::SizeToOpSize(LshrSize), Value, BitSelect);
    }

    // OF/SF/ZF/AF/PF undefined.
    // Set CF before the action to save a move.
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Value, ConstantShift, true);

    switch (Action) {
    case BTAction::BTNone: {
      /* Nothing to do */
      break;
    }

    case BTAction::BTClear: {
      OrderedNode *BitMask = _Lshl(IR::SizeToOpSize(LshrSize), _Constant(1), BitSelect);
      Dest = _Andn(IR::SizeToOpSize(LshrSize), Dest, BitMask);
      StoreResult(GPRClass, Op, Dest, -1);
      break;
    }

    case BTAction::BTSet: {
      OrderedNode *BitMask = _Lshl(IR::SizeToOpSize(LshrSize), _Constant(1), BitSelect);
      Dest = _Or(IR::SizeToOpSize(LshrSize), Dest, BitMask);
      StoreResult(GPRClass, Op, Dest, -1);
      break;
    }

    case BTAction::BTComplement: {
      OrderedNode *BitMask = _Lshl(IR::SizeToOpSize(LshrSize), _Constant(1), BitSelect);
      Dest = _Xor(IR::SizeToOpSize(LshrSize), Dest, BitMask);
      StoreResult(GPRClass, Op, Dest, -1);
      break;
    }
    }
  } else {
    // Load the address to the memory location
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    // Get the bit selection from the src
    OrderedNode *BitSelect = _Bfe(IR::SizeToOpSize(std::max<uint8_t>(4u, GetOpSize(Src))), 3, 0, Src);

    // Address is provided as bits we want BYTE offsets
    // Extract Signed offset
    Src = _Sbfe(OpSize::i64Bit, Size - 3, 3, Src);

    // Get the address offset by shifting out the size of the op (To shift out the bit selection)
    // Then use that to index in to the memory location by size of op

    // Now add the addresses together and load the memory
    OrderedNode *MemoryLocation = _Add(OpSize::i64Bit, Dest, Src);

    ConstantShift = 0;

    switch (Action) {
    case BTAction::BTNone: {
      Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);
      break;
    }

    case BTAction::BTClear: {
      OrderedNode *BitMask = _Lshl(OpSize::i64Bit, _Constant(1), BitSelect);

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchCLR(OpSize::i8Bit, BitMask, MemoryLocation);
      } else {
        Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

        auto Modified = _Andn(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Modified, 1);
      }
      break;
    }

    case BTAction::BTSet: {
      OrderedNode *BitMask = _Lshl(OpSize::i64Bit, _Constant(1), BitSelect);

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchOr(OpSize::i8Bit, BitMask, MemoryLocation);
      } else {
        Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

        auto Modified = _Or(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Modified, 1);
      }
      break;
    }

    case BTAction::BTComplement: {
      OrderedNode *BitMask = _Lshl(OpSize::i64Bit, _Constant(1), BitSelect);

      if (DestIsLockedMem(Op)) {
        HandledLock = true;
        Value = _AtomicFetchXor(OpSize::i8Bit, BitMask, MemoryLocation);
      } else {
        Value = _LoadMemAutoTSO(GPRClass, 1, MemoryLocation, 1);

        auto Modified = _Xor(OpSize::i64Bit, Value, BitMask);
        _StoreMemAutoTSO(GPRClass, 1, MemoryLocation, Modified, 1);
      }
      break;
    }
    }

    // Now shift in to the correct bit location
    Value = _Lshr(IR::SizeToOpSize(std::max<uint8_t>(4u, GetOpSize(Value))), Value, BitSelect);

    // OF/SF/ZF/AF/PF undefined.
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Value, ConstantShift, true);
  }
}

void OpDispatchBuilder::IMUL1SrcOp(OpcodeArgs) {
  /* We're just going to sign-extend the non-garbage anyway.. */
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  uint8_t Size = GetSrcSize(Op);

  OrderedNode *Dest{};
  OrderedNode *ResultHigh{};
  switch (Size) {
    case 1:
    case 2: {
      Src1 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src1);
      Src2 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src2);
      Dest = _Mul(OpSize::i64Bit, Src1, Src2);
      ResultHigh = _Sbfe(OpSize::i64Bit, Size * 8, Size * 8, Dest);
      break;
    }
    case 4: {
      ResultHigh = _SMull(Src1, Src2);
      ResultHigh = _Sbfe(OpSize::i64Bit, Size * 8, Size * 8, ResultHigh);
      // Flipped order to save a move
      Dest = _Mul(OpSize::i32Bit, Src1, Src2);
      break;
    }
    case 8: {
      ResultHigh = _MulH(OpSize::i64Bit, Src1, Src2);
      // Flipped order to save a move
      Dest = _Mul(OpSize::i64Bit, Src1, Src2);
      break;
    }
    default: FEX_UNREACHABLE;
  }

  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, ResultHigh);
}

void OpDispatchBuilder::IMUL2SrcOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags, {.AllowUpperGarbage = true});

  uint8_t Size = GetSrcSize(Op);

  OrderedNode *Dest{};
  OrderedNode *ResultHigh{};

  switch (Size) {
    case 1:
    case 2: {
      Src1 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src1);
      Src2 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src2);
      Dest = _Mul(OpSize::i64Bit, Src1, Src2);
      ResultHigh = _Sbfe(OpSize::i64Bit, Size * 8, Size * 8, Dest);
      break;
    }
    case 4: {
      ResultHigh = _SMull(Src1, Src2);
      ResultHigh = _Sbfe(OpSize::i64Bit, Size * 8, Size * 8, ResultHigh);
      // Flipped order to save a move
      Dest = _Mul(OpSize::i32Bit, Src1, Src2);
      break;
    }
    case 8: {
      ResultHigh = _MulH(OpSize::i64Bit, Src1, Src2);
      // Flipped order to save a move
      Dest = _Mul(OpSize::i64Bit, Src1, Src2);
      break;
    }
    default: FEX_UNREACHABLE;
  }

  StoreResult(GPRClass, Op, Dest, -1);
  GenerateFlags_MUL(Op, Dest, ResultHigh);
}

void OpDispatchBuilder::IMULOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  OrderedNode* Src2 = LoadGPRRegister(X86State::REG_RAX);

  if (Size != 8) {
    Src1 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src1);
    Src2 = _Sbfe(OpSize::i64Bit, Size * 8, 0, Src2);
  }

  // 64-bit special cased to save a move
  OrderedNode *Result = Size < 8 ? _Mul(OpSize::i64Bit, Src1, Src2) : nullptr;
  OrderedNode *ResultHigh{};
  if (Size == 1) {
    // Result is stored in AX
    StoreGPRRegister(X86State::REG_RAX, Result, 2);
    ResultHigh = _Sbfe(OpSize::i64Bit, 8, 8, Result);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    StoreGPRRegister(X86State::REG_RAX, Result, Size);
    ResultHigh = _Sbfe(OpSize::i64Bit, 16, 16, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh, Size);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    // Make sure they get Zext correctly
    auto LocalResult = _Bfe(OpSize::i64Bit, 32, 0, Result);
    auto LocalResultHigh = _Bfe(OpSize::i64Bit, 32, 32, Result);
    ResultHigh = _Sbfe(OpSize::i64Bit, 32, 32, Result);
    Result = _Sbfe(OpSize::i64Bit, 32, 0, Result);
    StoreGPRRegister(X86State::REG_RAX, LocalResult);
    StoreGPRRegister(X86State::REG_RDX, LocalResultHigh);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
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

  GenerateFlags_MUL(Op, Result, ResultHigh);
}

void OpDispatchBuilder::MULOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  OrderedNode* Src2 = LoadGPRRegister(X86State::REG_RAX);

  if (Size != 8) {
    Src1 = _Bfe(OpSize::i64Bit, Size * 8, 0, Src1);
    Src2 = _Bfe(OpSize::i64Bit, Size * 8, 0, Src2);
  }
  OrderedNode *Result = _UMul(OpSize::i64Bit, Src1, Src2);
  OrderedNode *ResultHigh{};

  if (Size == 1) {
    // Result is stored in AX
    StoreGPRRegister(X86State::REG_RAX, Result, 2);
    ResultHigh = _Bfe(OpSize::i64Bit, 8, 8, Result);
  }
  else if (Size == 2) {
    // 16bits stored in AX
    // 16bits stored in DX
    StoreGPRRegister(X86State::REG_RAX, Result, Size);
    ResultHigh = _Bfe(OpSize::i64Bit, 16, 16, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh, Size);
  }
  else if (Size == 4) {
    // 32bits stored in EAX
    // 32bits stored in EDX
    OrderedNode *ResultLow = _Bfe(OpSize::i64Bit, 32, 0, Result);
    ResultHigh = _Bfe(OpSize::i64Bit, 32, 32, Result);
    StoreGPRRegister(X86State::REG_RAX, ResultLow);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    // 64bits stored in RAX
    // 64bits stored in RDX
    ResultHigh = _UMulH(OpSize::i64Bit, Src1, Src2);
    StoreGPRRegister(X86State::REG_RAX, Result);
    StoreGPRRegister(X86State::REG_RDX, ResultHigh);
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

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    _AtomicXor(IR::SizeToOpSize(Size), MaskConst, DestMem);
  } else if (!Op->Dest.IsGPR()) {
    // GPR version plays fast and loose with sizes, be safe for memory tho.
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
    Src = _Xor(OpSize::i64Bit, Src, MaskConst);
    StoreResult(GPRClass, Op, Src, -1);
  } else {
    // Specially handle high bits so we can invert in place with the correct
    // mask and a larger type.
    auto Dest = Op->Dest;
    if (Dest.Data.GPR.HighBits) {
      LOGMAN_THROW_A_FMT(Size == 1, "Only 8-bit GPRs get high bits");
      MaskConst = _Constant(0xFF00);
      Dest.Data.GPR.HighBits = false;
    }

    // Always load full size, we explicitly want the upper bits to get the
    // insert behaviour for free/implicitly.
    const uint8_t GPRSize = CTX->GetGPRSize();
    OrderedNode *Src = LoadSource_WithOpSize(GPRClass, Op, Dest, GPRSize, Op->Flags);

    // For 8/16-bit, use 64-bit invert so we invert in place, while getting
    // insert behaviour. For 32-bit, use 32-bit invert to zero the upper bits.
    unsigned EffectiveSize = Size == 4 ? 4 : GPRSize;

    // If we're inverting the whole thing, use Not instead of Xor to save a constant.
    if (Size >= 4)
      Src = _Not(IR::SizeToOpSize(EffectiveSize), Src);
    else
      Src = _Xor(IR::SizeToOpSize(EffectiveSize), Src, MaskConst);

    // Always store 64-bit, the Not/Xor correctly handle the upper bits and this
    // way we can delete the store.
    StoreResult_WithOpSize(GPRClass, Op, Dest, Src, GPRSize, -1);
  }
}

void OpDispatchBuilder::XADDOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result;

  if (Op->Dest.IsGPR()) {
    // If this is a GPR then we can just do an Add
    Result = CalculateFlags_ADD(GetSrcSize(Op), Dest, Src);

    // Previous value in dest gets stored in src
    StoreResult(GPRClass, Op, Op->Src[0], Dest, -1);

    // Calculated value gets stored in dst (order is important if dst is same as src)
    StoreResult(GPRClass, Op, Result, -1);
  }
  else {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;
    Dest = AppendSegmentOffset(Dest, Op->Flags);
    auto Before = _AtomicFetchAdd(OpSizeFromSrc(Op), Src, Dest);
    StoreResult(GPRClass, Op, Op->Src[0], Before, -1);

    CalculateFlags_ADD(GetSrcSize(Op), Before, Src);
  }
}

void OpDispatchBuilder::PopcountOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = GetSrcSize(Op) >= 4});
  Src = _Popcount(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Src, -1);

  GenerateFlags_POPCOUNT(Op, Src);
}

OrderedNode *OpDispatchBuilder::CalculateAFForDecimal(OrderedNode *A) {
  auto Nibble = _And(OpSize::i64Bit, A, _Constant(0xF));
  auto Greater = _Select(FEXCore::IR::COND_UGT, Nibble, _Constant(9),
                         _Constant(1), _Constant(0));

  return _Or(OpSize::i64Bit, LoadAF(), Greater);
}

void OpDispatchBuilder::DAAOp(OpcodeArgs) {
  CalculateDeferredFlags();
  auto AL = LoadGPRRegister(X86State::REG_RAX, 1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  auto AF = CalculateAFForDecimal(AL);

  // CF |= (AL > 0x99);
  CF = _Or(OpSize::i64Bit, CF, _Select(FEXCore::IR::COND_UGT, AL, _Constant(0x99), _Constant(1), _Constant(0)));

  // AL = AF ? (AL + 0x6) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, AF, _Constant(0),
               _Add(OpSize::i64Bit, AL, _Constant(0x6)), AL);

  // AL = CF ? (AL + 0x60) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, CF, _Constant(0),
               _Add(OpSize::i64Bit, AL, _Constant(0x60)), AL);

  // SF, ZF, PF set according to result. CF set per above. OF undefined.
  StoreGPRRegister(X86State::REG_RAX, AL, 1);
  SetNZ_ZeroCV(1, AL);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(CF);
  CalculatePF(AL);
  SetAFAndFixup(AF);
}

void OpDispatchBuilder::DASOp(OpcodeArgs) {
  CalculateDeferredFlags();
  auto AL = LoadGPRRegister(X86State::REG_RAX, 1);
  auto CF = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  auto AF = CalculateAFForDecimal(AL);

  // CF |= (AL > 0x99);
  CF = _Or(OpSize::i64Bit, CF, _Select(FEXCore::IR::COND_UGT, AL, _Constant(0x99), _Constant(1), _Constant(0)));

  // NewCF = CF | (AF && (Borrow from AL - 6))
  auto NewCF = _Or(OpSize::i32Bit, CF, _Select(FEXCore::IR::COND_ULT, AL, _Constant(6), AF, CF));

  // AL = AF ? (AL - 0x6) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, AF, _Constant(0),
               _Sub(OpSize::i64Bit, AL, _Constant(0x6)), AL);

  // AL = CF ? (AL - 0x60) : AL;
  AL = _Select(FEXCore::IR::COND_NEQ, CF, _Constant(0),
               _Sub(OpSize::i64Bit, AL, _Constant(0x60)), AL);

  // SF, ZF, PF set according to result. CF set per above. OF undefined.
  StoreGPRRegister(X86State::REG_RAX, AL, 1);
  SetNZ_ZeroCV(1, AL);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(NewCF);
  CalculatePF(AL);
  SetAFAndFixup(AF);
}

void OpDispatchBuilder::AAAOp(OpcodeArgs) {
  InvalidateDeferredFlags();

  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AF = CalculateAFForDecimal(A);

  // CF = AF, OF/SF/ZF/PF undefined
  ZeroNZCV();
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(AF);
  SetAFAndFixup(AF);
  CalculateDeferredFlags();

  // AX = CF ? (AX + 0x106) : 0
  A = _NZCVSelect(OpSize::i32Bit, CondClassType{COND_UGE} /* CF = 1 */,
                  _Add(OpSize::i32Bit, A, _Constant(0x106)), A);

  // AL = AL & 0x0F
  A = _And(OpSize::i32Bit, A, _Constant(0xFF0F));
  StoreGPRRegister(X86State::REG_RAX, A, 2);
}

void OpDispatchBuilder::AASOp(OpcodeArgs) {
  InvalidateDeferredFlags();

  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AF = CalculateAFForDecimal(A);

  // CF = AF, OF/SF/ZF/PF undefined
  ZeroNZCV();
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(AF);
  SetAFAndFixup(AF);
  CalculateDeferredFlags();

  // AX = CF ? (AX - 0x106) : 0
  A = _NZCVSelect(OpSize::i32Bit, CondClassType{COND_UGE} /* CF = 1 */,
                  _Sub(OpSize::i32Bit, A, _Constant(0x106)), A);

  // AL = AL & 0x0F
  A = _And(OpSize::i32Bit, A, _Constant(0xFF0F));
  StoreGPRRegister(X86State::REG_RAX, A, 2);
}

void OpDispatchBuilder::AAMOp(OpcodeArgs) {
  InvalidateDeferredFlags();

  auto AL = LoadGPRRegister(X86State::REG_RAX, 1);
  auto Imm8 = _Constant(Op->Src[0].Data.Literal.Value & 0xFF);
  auto UDivOp = _UDiv(OpSize::i64Bit, AL, Imm8);
  auto URemOp = _URem(OpSize::i64Bit, AL, Imm8);
  auto Res = _AddShift(OpSize::i64Bit, URemOp, UDivOp, ShiftType::LSL, 8);
  StoreGPRRegister(X86State::REG_RAX, Res, 2);

  SetNZ_ZeroCV(1, Res);
  CalculatePF(Res);
  _InvalidateFlags(1u << X86State::RFLAG_AF_RAW_LOC);
}

void OpDispatchBuilder::AADOp(OpcodeArgs) {
  InvalidateDeferredFlags();

  auto A = LoadGPRRegister(X86State::REG_RAX);
  auto AH = _Lshr(OpSize::i32Bit, A, _Constant(8));
  auto Imm8 = _Constant(Op->Src[0].Data.Literal.Value & 0xFF);
  auto NewAL = _Add(OpSize::i64Bit, A, _Mul(OpSize::i64Bit, AH, Imm8));
  auto Result = _And(OpSize::i64Bit, NewAL, _Constant(0xFF));
  StoreGPRRegister(X86State::REG_RAX, Result, 2);

  SetNZ_ZeroCV(1, Result);
  CalculatePF(Result);
  _InvalidateFlags(1u << X86State::RFLAG_AF_RAW_LOC);
}

void OpDispatchBuilder::XLATOp(OpcodeArgs) {
  OrderedNode *Src = LoadGPRRegister(X86State::REG_RBX);
  OrderedNode *Offset = LoadGPRRegister(X86State::REG_RAX, 1);

  Src = AppendSegmentOffset(Src, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);
  Src = _Add(OpSize::i64Bit, Src, Offset);

  auto Res = _LoadMemAutoTSO(GPRClass, 1, Src, 1);

  StoreGPRRegister(X86State::REG_RAX, Res, 1);
}

template<OpDispatchBuilder::Segment Seg>
void OpDispatchBuilder::ReadSegmentReg(OpcodeArgs) {
  // 64-bit only
  // Doesn't hit the segment register optimization
  auto Size = GetSrcSize(Op);
  OrderedNode *Src{};
  if constexpr (Seg == Segment::FS) {
    Src = _LoadContext(Size, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
  }
  else {
    Src = _LoadContext(Size, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
  }

  StoreResult(GPRClass, Op, Src, -1);
}

template<OpDispatchBuilder::Segment Seg>
void OpDispatchBuilder::WriteSegmentReg(OpcodeArgs) {
  // Documentation claims that the 32-bit version of this instruction inserts in to the lower 32-bits of the segment
  // This is incorrect and it instead zero extends the 32-bit value to 64-bit
  auto Size = GetDstSize(Op);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  if constexpr (Seg == Segment::FS) {
    _StoreContext(Size, GPRClass, Src, offsetof(FEXCore::Core::CPUState, fs_cached));
  }
  else {
    _StoreContext(Size, GPRClass, Src, offsetof(FEXCore::Core::CPUState, gs_cached));
  }
}

void OpDispatchBuilder::EnterOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  LOGMAN_THROW_A_FMT(Op->Src[0].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Value = Op->Src[0].Data.Literal.Value;

  const uint16_t AllocSpace = Value & 0xFFFF;
  const uint8_t Level = (Value >> 16) & 0x1F;

  const auto PushValue = [&](uint8_t Size, OrderedNode *Src) -> OrderedNode* {
    const uint8_t GPRSize = CTX->GetGPRSize();

    auto OldSP = LoadGPRRegister(X86State::REG_RSP);
    auto NewSP = _Push(GPRSize, Size, Src, OldSP);

    // Store the new stack pointer
    StoreGPRRegister(X86State::REG_RSP, NewSP);
    return NewSP;
  };

  auto OldBP = LoadGPRRegister(X86State::REG_RBP);
  auto NewSP = PushValue(GPRSize, OldBP);
  auto temp_RBP = NewSP;

  if (Level > 0) {
    for (uint8_t i = 1; i < Level; ++i) {
      auto Offset = _Constant(i * GPRSize);
      auto MemLoc = _Sub(IR::SizeToOpSize(GPRSize), OldBP, Offset);
      auto Mem = _LoadMem(GPRClass, GPRSize, MemLoc, GPRSize);
      NewSP = PushValue(GPRSize, Mem);
    }
    NewSP = PushValue(GPRSize, temp_RBP);
  }
  NewSP = _Sub(IR::SizeToOpSize(GPRSize), NewSP, _Constant(AllocSpace));
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
  size_t GDTStoreSize = 8;
  if (!CTX->Config.Is64BitMode) {
    // Mask off upper bits if 32-bit result.
    GDTAddress &= ~0U;
    GDTStoreSize = 4;
  }

  _StoreMemAutoTSO(GPRClass, 2, DestAddress, _Constant(0));
  _StoreMemAutoTSO(GPRClass, GDTStoreSize, _Add(OpSize::i64Bit, DestAddress, _Constant(2)), _Constant(GDTAddress));
}


OpDispatchBuilder::CycleCounterPair OpDispatchBuilder::CycleCounter() {
  OrderedNode *CounterLow{};
  OrderedNode *CounterHigh{};
  auto Counter = _CycleCounter();
  if (CTX->Config.SmallTSCScale()) {
    const auto ShiftAmount = FEXCore::ilog2(FEXCore::Context::TSC_SCALE);
    CounterLow = _Lshl(OpSize::i32Bit, Counter, _Constant(ShiftAmount));
    CounterHigh = _Lshr(OpSize::i64Bit, Counter, _Constant(32 - ShiftAmount));
  }
  else {
    CounterLow = _Bfe(OpSize::i64Bit, 32, 0, Counter);
    CounterHigh = _Bfe(OpSize::i64Bit, 32, 32, Counter);
  }

  return {
    .CounterLow = CounterLow,
    .CounterHigh = CounterHigh,
  };
}

void OpDispatchBuilder::RDTSCOp(OpcodeArgs) {
  auto Counter = CycleCounter();
  StoreGPRRegister(X86State::REG_RAX, Counter.CounterLow);
  StoreGPRRegister(X86State::REG_RDX, Counter.CounterHigh);
}

void OpDispatchBuilder::INCOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::EFmt("Can't handle REP on this");
    DecodeFailure = true;
    return;
  }

  OrderedNode *Dest;
  OrderedNode *Result;
  const auto Size = GetSrcBitSize(Op);
  auto OneConst = _Constant(Size, 1);

  const bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;
    auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestAddress = AppendSegmentOffset(DestAddress, Op->Flags);
    Dest = _AtomicFetchAdd(OpSizeFromSrc(Op), OneConst, DestAddress);

  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});
  }

  CalculateDeferredFlags();

  if (Size < 32 && CTX->HostFeatures.SupportsFlagM) {
    // Addition producing upper garbage
    Result = _Add(OpSize::i32Bit, Dest, OneConst);
    CalculatePF(Result);
    CalculateAF(Dest, OneConst);

    // Correctly set NZ flags, preserving C
    HandleNZCV_RMW();
    _SetSmallNZV(OpSizeFromSrc(Op), Result);

    // Fix up V flag. INC overflows only when incrementing a positive and
    // getting a negative. So compare the sign bits to calculate V.
    _RmifNZCV(_Andn(OpSize::i32Bit, Result, Dest), Size - 1, 1);
  } else {
    Result = CalculateFlags_ADD(OpSizeFromSrc(Op), Dest, OneConst, false);
  }

  if (!IsLocked) {
    StoreResult(GPRClass, Op, Result, -1);
  }
}

void OpDispatchBuilder::DECOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX) {
    LogMan::Msg::EFmt("Can't handle REP on this");
    DecodeFailure = true;
    return;
  }

  OrderedNode *Dest;
  OrderedNode *Result;
  const auto Size = GetSrcBitSize(Op);
  auto OneConst = _Constant(Size, 1);

  const bool IsLocked = DestIsLockedMem(Op);

  if (IsLocked) {
    HandledLock = true;
    auto DestAddress = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestAddress = AppendSegmentOffset(DestAddress, Op->Flags);

    // Use Add instead of Sub to avoid a NEG
    Dest = _AtomicFetchAdd(OpSizeFromSrc(Op), _Constant(Size, -1), DestAddress);
  } else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = Size >= 32});
  }

  CalculateDeferredFlags();

  if (Size < 32 && CTX->HostFeatures.SupportsFlagM) {
    // Subtraction producing upper garbage
    Result = _Sub(OpSize::i32Bit, Dest, OneConst);
    CalculatePF(Result);
    CalculateAF(Dest, OneConst);

    // Correctly set NZ flags, preserving C
    HandleNZCV_RMW();
    _SetSmallNZV(OpSizeFromSrc(Op), Result);

    // Fix up V flag. DEC overflows only when decrementing a negative and
    // getting a positive. So compare the sign bits to calculate V.
    _RmifNZCV(_Andn(OpSize::i32Bit, Dest, Result), Size - 1, 1);
  } else {
    Result = CalculateFlags_SUB(OpSizeFromSrc(Op), Dest, OneConst, false);
  }

  if (!IsLocked) {
    StoreResult(GPRClass, Op, Result, -1);
  }
}

void OpDispatchBuilder::STOSOp(OpcodeArgs) {
  if (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) {
    LogMan::Msg::EFmt("Can't handle adddress size");
    DecodeFailure = true;
    return;
  }

  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) != 0;

  if (!Repeat) {
    // Src is used only for a store of the same size so allow garbage
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
    OrderedNode *Dest = LoadGPRRegister(X86State::REG_RDI);

    // Only ES prefix
    Dest = AppendSegmentOffset(Dest, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, Dest, Src, Size);

    // Offset the pointer
    OrderedNode *TailDest = LoadGPRRegister(X86State::REG_RDI);
    StoreGPRRegister(X86State::REG_RDI, OffsetByDir(TailDest, Size));
  }
  else {
    // FEX doesn't support partial faulting REP instructions.
    // Converting this to a `MemSet` IR op optimizes this quite significantly in our codegen.
    // If FEX is to gain support for faulting REP instructions, then this implementation needs to change significantly.
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    OrderedNode *Dest = LoadGPRRegister(X86State::REG_RDI);

    // Only ES prefix
    auto Segment = GetSegment(0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    OrderedNode *Counter = LoadGPRRegister(X86State::REG_RCX);

    auto Result = _MemSet(CTX->IsAtomicTSOEnabled(), Size, Segment ?: InvalidNode, Dest, Src, Counter, LoadDir(1));
    StoreGPRRegister(X86State::REG_RCX, _Constant(0));
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
  const auto Size = GetSrcSize(Op);

  if (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) {
    auto SrcAddr = LoadGPRRegister(X86State::REG_RSI);
    auto DstAddr = LoadGPRRegister(X86State::REG_RDI);
    auto Counter = LoadGPRRegister(X86State::REG_RCX);

    auto DstSegment = GetSegment(0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    auto SrcSegment = GetSegment(Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Result = _MemCpy(CTX->IsAtomicTSOEnabled(), Size,
        DstSegment ?: InvalidNode,
        SrcSegment ?: InvalidNode,
        DstAddr, SrcAddr, Counter,
        LoadDir(1));

    OrderedNode *Result_Dst = _ExtractElementPair(OpSize::i64Bit, Result, 0);
    OrderedNode *Result_Src = _ExtractElementPair(OpSize::i64Bit, Result, 1);

    StoreGPRRegister(X86State::REG_RCX, _Constant(0));
    StoreGPRRegister(X86State::REG_RDI, Result_Dst);
    StoreGPRRegister(X86State::REG_RSI, Result_Src);
  }
  else {
    OrderedNode *RSI = LoadGPRRegister(X86State::REG_RSI);
    OrderedNode *RDI = LoadGPRRegister(X86State::REG_RDI);
    RDI= AppendSegmentOffset(RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    RSI = AppendSegmentOffset(RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, RSI, Size);

    // Store to memory where RDI points
    _StoreMemAutoTSO(GPRClass, Size, RDI, Src, Size);

    auto PtrDir = LoadDir(Size);
    RSI = _Add(OpSize::i64Bit, RSI, PtrDir);
    RDI = _Add(OpSize::i64Bit, RDI, PtrDir);

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

  const auto Size = GetSrcSize(Op);

  bool Repeat = Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX);
  if (!Repeat) {
    OrderedNode *Dest_RSI = LoadGPRRegister(X86State::REG_RSI);
    OrderedNode *Dest_RDI = LoadGPRRegister(X86State::REG_RDI);

    // Only ES prefix
    Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
    // Default DS prefix
    Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    GenerateFlags_SUB(Op, Src2, Src1);

    auto PtrDir = LoadDir(Size);

    // Offset the pointer
    Dest_RDI = _Add(OpSize::i64Bit, Dest_RDI, PtrDir);
    StoreGPRRegister(X86State::REG_RDI, Dest_RDI);

    // Offset second pointer
    Dest_RSI = _Add(OpSize::i64Bit, Dest_RSI, PtrDir);
    StoreGPRRegister(X86State::REG_RSI, Dest_RSI);
  }
  else {
    // Calculate flags early.
    CalculateDeferredFlags();

    bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

    // If rcx = 0, skip the whole loop.
    OrderedNode *Counter = LoadGPRRegister(X86State::REG_RCX);
    auto OuterJump = CondJump(Counter, {COND_EQ});

    auto BeforeLoop = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetFalseJumpTarget(OuterJump, BeforeLoop);
    SetCurrentCodeBlock(BeforeLoop);
    StartNewBlock();

    ForeachDirection([this, Op, Size, REPE](int PtrDir) {
      IRPair<IROp_CondJump> InnerJump;
      auto JumpIntoLoop = Jump();

      // Setup for the loop
      auto LoopHeader = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetCurrentCodeBlock(LoopHeader);
      StartNewBlock();
      SetJumpTarget(JumpIntoLoop, LoopHeader);

      // Working loop
      {
        OrderedNode *Dest_RSI = LoadGPRRegister(X86State::REG_RSI);
        OrderedNode *Dest_RDI = LoadGPRRegister(X86State::REG_RDI);

        // Only ES prefix
        Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);
        // Default DS prefix
        Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

        auto Src1 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);
        auto Src2 = _LoadMem(GPRClass, Size, Dest_RSI, Size);

        // We'll calculate PF/AF after the loop, so use them as temporaries here.
        _StoreRegister(Src1, false, offsetof(FEXCore::Core::CPUState, pf_raw), GPRClass, GPRFixedClass, CTX->GetGPRSize());
        _StoreRegister(Src2, false, offsetof(FEXCore::Core::CPUState, af_raw), GPRClass, GPRFixedClass, CTX->GetGPRSize());

        OrderedNode *TailCounter = LoadGPRRegister(X86State::REG_RCX);

        // Decrement counter
        TailCounter = _SubWithFlags(OpSize::i64Bit, TailCounter, _Constant(1));

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        Dest_RDI = _Add(OpSize::i64Bit, Dest_RDI, _Constant(PtrDir * Size));
        StoreGPRRegister(X86State::REG_RDI, Dest_RDI);

        // Offset second pointer
        Dest_RSI = _Add(OpSize::i64Bit, Dest_RSI, _Constant(PtrDir * Size));
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
      auto Src1 = _LoadRegister(false, offsetof(FEXCore::Core::CPUState, pf_raw), GPRClass, GPRFixedClass, CTX->GetGPRSize());
      auto Src2 = _LoadRegister(false, offsetof(FEXCore::Core::CPUState, af_raw), GPRClass, GPRFixedClass, CTX->GetGPRSize());
      GenerateFlags_SUB(Op, Src2, Src1);
      CalculateDeferredFlags();
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

  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX)) != 0;

  if (!Repeat) {
    OrderedNode *Dest_RSI = LoadGPRRegister(X86State::REG_RSI);
    Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

    auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

    StoreResult(GPRClass, Op, Src, -1);

    // Offset the pointer
    OrderedNode *TailDest_RSI = LoadGPRRegister(X86State::REG_RSI);
    StoreGPRRegister(X86State::REG_RSI, OffsetByDir(TailDest_RSI, Size));
  }
  else {
    // Calculate flags early. because end of block
    CalculateDeferredFlags();

    ForeachDirection([this, Op, Size](int PtrDir) {
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

      OrderedNode *Counter = LoadGPRRegister(X86State::REG_RCX);

      // Can we end the block?

      // We leave if RCX = 0
      auto CondJump_ = CondJump(Counter, {COND_EQ});

      auto LoopTail = CreateNewCodeBlockAfter(LoopStart);
      SetFalseJumpTarget(CondJump_, LoopTail);
      SetCurrentCodeBlock(LoopTail);
      StartNewBlock();

      // Working loop
      {
        OrderedNode *Dest_RSI = LoadGPRRegister(X86State::REG_RSI);

        Dest_RSI = AppendSegmentOffset(Dest_RSI, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

        auto Src = _LoadMemAutoTSO(GPRClass, Size, Dest_RSI, Size);

        StoreResult(GPRClass, Op, Src, -1);

        OrderedNode *TailCounter = LoadGPRRegister(X86State::REG_RCX);
        OrderedNode *TailDest_RSI = LoadGPRRegister(X86State::REG_RSI);

        // Decrement counter
        TailCounter = _Sub(OpSize::i64Bit, TailCounter, _Constant(1));

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        TailDest_RSI = _Add(OpSize::i64Bit, TailDest_RSI, _Constant(PtrDir * Size));
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

  const auto Size = GetSrcSize(Op);
  const bool Repeat = (Op->Flags & (FEXCore::X86Tables::DecodeFlags::FLAG_REPNE_PREFIX | FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX)) != 0;

  if (!Repeat) {
    OrderedNode *Dest_RDI = LoadGPRRegister(X86State::REG_RDI);
    Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

    auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
    auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

    GenerateFlags_SUB(Op, Src1, Src2);

    // Offset the pointer
    OrderedNode *TailDest_RDI = LoadGPRRegister(X86State::REG_RDI);
    StoreGPRRegister(X86State::REG_RDI, OffsetByDir(TailDest_RDI, Size));
  }
  else {
    // Calculate flags early. because end of block
    CalculateDeferredFlags();

    ForeachDirection([this, Op, Size](int Dir){
      bool REPE = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REP_PREFIX;

      auto JumpStart = Jump();
      // Make sure to start a new block after ending this one
      auto LoopStart = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetJumpTarget(JumpStart, LoopStart);
      SetCurrentCodeBlock(LoopStart);
      StartNewBlock();

      OrderedNode *Counter = LoadGPRRegister(X86State::REG_RCX);

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
        OrderedNode *Dest_RDI = LoadGPRRegister(X86State::REG_RDI);

        Dest_RDI = AppendSegmentOffset(Dest_RDI, 0, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX, true);

        auto Src1 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});
        auto Src2 = _LoadMemAutoTSO(GPRClass, Size, Dest_RDI, Size);

        GenerateFlags_SUB(Op, Src1, Src2);

        // Calculate flags early.
        CalculateDeferredFlags();

        OrderedNode *TailCounter = LoadGPRRegister(X86State::REG_RCX);
        OrderedNode *TailDest_RDI = LoadGPRRegister(X86State::REG_RDI);

        // Decrement counter
        TailCounter = _Sub(OpSize::i64Bit, TailCounter, _Constant(1));

        // Store the counter since we don't have phis
        StoreGPRRegister(X86State::REG_RCX, TailCounter);

        // Offset the pointer
        TailDest_RDI = _Add(OpSize::i64Bit, TailDest_RDI, _Constant(Dir * Size));
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
  OrderedNode *Dest;
  const auto Size = GetSrcSize(Op);
  if (Size == 2) {
    // BSWAP of 16bit is undef. ZEN+ causes the lower 16bits to get zero'd
    Dest = _Constant(0);
  }
  else {
    Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, CTX->GetGPRSize(), Op->Flags);
    Dest = _Rev(IR::SizeToOpSize(Size), Dest);
  }
  StoreResult(GPRClass, Op, Dest, -1);
}

void OpDispatchBuilder::PUSHFOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  OrderedNode *Src = GetPackedRFLAG();
  if (Size != 8) {
    Src = _Bfe(OpSize::i32Bit, Size * 8, 0, Src);
  }

  auto OldSP = LoadGPRRegister(X86State::REG_RSP);

  const uint8_t GPRSize = CTX->GetGPRSize();
  auto NewSP = _Push(GPRSize, Size, Src, OldSP);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);
}

void OpDispatchBuilder::POPFOp(OpcodeArgs) {
  const uint8_t Size = GetSrcSize(Op);

  auto Constant = _Constant(Size);
  auto OldSP = LoadGPRRegister(X86State::REG_RSP);
  OrderedNode *Src = _LoadMem(GPRClass, Size, OldSP, Size);
  auto NewSP = _Add(OpSize::i64Bit, OldSP, Constant);

  // Store the new stack pointer
  StoreGPRRegister(X86State::REG_RSP, NewSP);

  // Add back our flag constants
  // Bit 1 is always 1
  // Bit 9 is always 1 because we always have interrupts enabled

  Src = _Or(OpSize::i64Bit, Src, _Constant(Size * 8, 0x202));

  SetPackedRFLAG(false, Src);
}

void OpDispatchBuilder::NEGOp(OpcodeArgs) {
  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;

  auto Size = GetSrcSize(Op);
  auto ZeroConst = _Constant(0);

  if (DestIsLockedMem(Op)) {
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);

    OrderedNode *Dest = _AtomicFetchNeg(IR::SizeToOpSize(Size), DestMem);
    CalculateFlags_SUB(Size, ZeroConst, Dest);
  }
  else {
    OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
    OrderedNode *Result = CalculateFlags_SUB(Size, ZeroConst, Dest);

    StoreResult(GPRClass, Op, Result, -1);
  }
}

void OpDispatchBuilder::DIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, 2);

    auto UDivOp = _UDiv(OpSize::i16Bit, Src1, Divisor);
    auto URemOp = _URem(OpSize::i16Bit, Src1, Divisor);

    // AX[15:0] = concat<URem[7:0]:UDiv[7:0]>
    auto ResultAX = _Bfi(IR::SizeToOpSize(GPRSize), 8, 8, UDivOp, URemOp);
    StoreGPRRegister(X86State::REG_RAX, ResultAX, 2);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, Size);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX, Size);
    auto UDivOp = _LUDiv(OpSize::i16Bit, Src1, Src2, Divisor);
    auto URemOp = _LURem(OpSize::i16Bit, Src1, Src2, Divisor);

    StoreGPRRegister(X86State::REG_RAX, UDivOp, Size);
    StoreGPRRegister(X86State::REG_RDX, URemOp, Size);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, Size);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX, Size);

    OrderedNode *UDivOp = _Bfe(OpSize::i32Bit, Size * 8, 0, _LUDiv(OpSize::i32Bit, Src1, Src2, Divisor));
    OrderedNode *URemOp = _Bfe(OpSize::i32Bit, Size * 8, 0, _LURem(OpSize::i32Bit, Src1, Src2, Divisor));

    StoreGPRRegister(X86State::REG_RAX, UDivOp);
    StoreGPRRegister(X86State::REG_RDX, URemOp);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX);

    auto UDivOp = _LUDiv(OpSize::i64Bit, Src1, Src2, Divisor);
    auto URemOp = _LURem(OpSize::i64Bit, Src1, Src2, Divisor);

    StoreGPRRegister(X86State::REG_RAX, UDivOp);
    StoreGPRRegister(X86State::REG_RDX, URemOp);
  }
}

void OpDispatchBuilder::IDIVOp(OpcodeArgs) {
  // This loads the divisor
  OrderedNode *Divisor = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = GetSrcSize(Op);

  if (Size == 1) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, 2);
    Src1 = _Sbfe(OpSize::i64Bit, 16, 0, Src1);
    Divisor = _Sbfe(OpSize::i64Bit, 8, 0, Divisor);

    auto UDivOp = _Div(OpSize::i64Bit, Src1, Divisor);
    auto URemOp = _Rem(OpSize::i64Bit, Src1, Divisor);

    // AX[15:0] = concat<URem[7:0]:UDiv[7:0]>
    auto ResultAX = _Bfi(IR::SizeToOpSize(GPRSize), 8, 8, UDivOp, URemOp);
    StoreGPRRegister(X86State::REG_RAX, ResultAX, 2);
  }
  else if (Size == 2) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, Size);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX, Size);
    auto UDivOp = _LDiv(OpSize::i16Bit, Src1, Src2, Divisor);
    auto URemOp = _LRem(OpSize::i16Bit, Src1, Src2, Divisor);

    StoreGPRRegister(X86State::REG_RAX, UDivOp, Size);
    StoreGPRRegister(X86State::REG_RDX, URemOp, Size);
  }
  else if (Size == 4) {
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX, Size);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX, Size);

    OrderedNode *UDivOp = _Bfe(OpSize::i32Bit, Size * 8, 0, _LDiv(OpSize::i32Bit, Src1, Src2, Divisor));
    OrderedNode *URemOp = _Bfe(OpSize::i32Bit, Size * 8, 0, _LRem(OpSize::i32Bit, Src1, Src2, Divisor));

    StoreGPRRegister(X86State::REG_RAX, UDivOp);
    StoreGPRRegister(X86State::REG_RDX, URemOp);
  }
  else if (Size == 8) {
    if (!CTX->Config.Is64BitMode) {
      LogMan::Msg::EFmt("Doesn't exist in 32bit mode");
      DecodeFailure = true;
      return;
    }
    OrderedNode *Src1 = LoadGPRRegister(X86State::REG_RAX);
    OrderedNode *Src2 = LoadGPRRegister(X86State::REG_RDX);

    auto UDivOp = _LDiv(OpSize::i64Bit, Src1, Src2, Divisor);
    auto URemOp = _LRem(OpSize::i64Bit, Src1, Src2, Divisor);

    StoreGPRRegister(X86State::REG_RAX, UDivOp);
    StoreGPRRegister(X86State::REG_RDX, URemOp);
  }
}

void OpDispatchBuilder::BSFOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : GPRSize;
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  InvalidateDeferredFlags();
  CachedNZCV = nullptr;

  // Find the LSB of this source
  auto Result = _FindLSB(OpSizeFromSrc(Op), Src);

  // OF, SF, AF, PF, CF all undefined
  // ZF is set to 1 if the source was zero
  SetNZ_ZeroCV(GetSrcSize(Op), Src);

  // If Src was zero then the destination doesn't get modified
  auto SelectOp = _NZCVSelect(IR::SizeToOpSize(GPRSize), CondClassType{COND_EQ}, Dest, Result);
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, -1);
}

void OpDispatchBuilder::BSROp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint8_t DstSize = GetDstSize(Op) == 2 ? 2 : GPRSize;
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, DstSize, Op->Flags);
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);

  InvalidateDeferredFlags();
  CachedNZCV = nullptr;

  // Find the MSB of this source
  auto Result = _FindMSB(OpSizeFromSrc(Op), Src);

  // OF, SF, AF, PF, CF all undefined
  // ZF is set to 1 if the source was zero
  SetNZ_ZeroCV(GetSrcSize(Op), Src);

  // If Src was zero then the destination doesn't get modified
  auto SelectOp = _NZCVSelect(IR::SizeToOpSize(GPRSize), CondClassType{COND_EQ}, Dest, Result);
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, SelectOp, DstSize, -1);
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
  OrderedNode *Src2 = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // 0x80014000
  // 0x80064000
  // 0x80064000

  if (Op->Dest.IsGPR()) {
    // If the destination is also the accumulator, we get some algebraic
    // simplifications. Not sure if this is actually hit but it's in
    // InstCountCI.
    bool Trivial = Op->Dest.Data.GPR.GPR == X86State::REG_RAX &&
                   !Op->Dest.IsGPRDirect() &&
                   !Op->Dest.Data.GPR.HighBits;

    OrderedNode *Src1{};
    OrderedNode *Src1Lower{};

    OrderedNode *Src3{};
    OrderedNode *Src3Lower{};
    if (GPRSize == 8 && Size == 4) {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);
      Src3 = LoadGPRRegister(X86State::REG_RAX);
    }
    else {
      Src1 = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, Size, Op->Flags);
      Src3 = LoadGPRRegister(X86State::REG_RAX);
    }

    if (Size != GPRSize) {
      Src1Lower = _Bfe(IR::SizeToOpSize(GPRSize), Size * 8, 0, Src1);
      Src3Lower = _Bfe(IR::SizeToOpSize(GPRSize), Size * 8, 0, Src3);
    }
    else {
      Src1Lower = Src1;
      Src3Lower = Src3;
    }

    // Compare RAX with the destination, setting flags accordingly.
    GenerateFlags_SUB(Op, Src3Lower, Src1Lower);
    CalculateDeferredFlags();

    if (!Trivial) {
      if (GPRSize == 8 && Size == 4) {
        // This allows us to only hit the ZEXT case on failure
        OrderedNode *RAXResult = _NZCVSelect(IR::i64Bit, CondClassType{COND_EQ},
                                             Src3, Src1Lower);

        // When the size is 4 we need to make sure not zext the GPR when the comparison fails
        StoreGPRRegister(X86State::REG_RAX, RAXResult);
      }
      else {
        StoreGPRRegister(X86State::REG_RAX, Src1Lower, Size);
      }
    }

    // Op1 = RAX == Op1 ? Op2 : Op1
    // If they match then set the rm operand to the input
    // else don't set the rm operand
    OrderedNode *DestResult =
      Trivial ? Src2 : _NZCVSelect(IR::i64Bit, CondClassType{COND_EQ}, Src2, Src1);

    // Store in to GPR Dest
    if (GPRSize == 8 && Size == 4) {
      StoreResult_WithOpSize(GPRClass, Op, Op->Dest, DestResult, GPRSize, -1);
    } else {
      StoreResult(GPRClass, Op, DestResult, -1);
    }
  }
  else {
    HandledLock = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

    OrderedNode *Src3{};
    OrderedNode *Src3Lower{};
    if (GPRSize == 8 && Size == 4) {
      Src3 = LoadGPRRegister(X86State::REG_RAX);
      Src3Lower = _Bfe(OpSize::i32Bit, 32, 0, Src3);
    }
    else {
      Src3 = LoadGPRRegister(X86State::REG_RAX, Size);
      Src3Lower = Src3;
    }
    // If this is a memory location then we want the pointer to it
    OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

    Src1 = AppendSegmentOffset(Src1, Op->Flags);

    // DataSrc = *Src1
    // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
    // This will write to memory! Careful!
    // Third operand must be a calculated guest memory address
    OrderedNode *CASResult = _CAS(IR::SizeToOpSize(Size), Src3Lower, Src2, Src1);
    OrderedNode *RAXResult = CASResult;

    if (GPRSize == 8 && Size == 4) {
      // This allows us to only hit the ZEXT case on failure
      RAXResult = _Select(FEXCore::IR::COND_EQ,
        CASResult, Src3Lower,
        Src3, CASResult);
      Size = 8;
    }

    // RAX gets the result of the CAS op
    StoreGPRRegister(X86State::REG_RAX, RAXResult, Size);

    GenerateFlags_SUB(Op, Src3Lower, CASResult);
  }
}

void OpDispatchBuilder::CMPXCHGPairOp(OpcodeArgs) {
  // Calculate flags early.
  CalculateDeferredFlags();

  // REX.W used to determine if it is 16byte or 8byte
  // Unlike CMPXCHG, the destination can only be a memory location
  uint8_t Size = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? 8 : 4;

  HandledLock = (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  // If this is a memory location then we want the pointer to it
  OrderedNode *Src1 = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

  Src1 = AppendSegmentOffset(Src1, Op->Flags);

  OrderedNode *Expected_Lower = LoadGPRRegister(X86State::REG_RAX, Size);
  OrderedNode *Expected_Upper = LoadGPRRegister(X86State::REG_RDX, Size);
  OrderedNode *Expected = _CreateElementPair(IR::SizeToOpSize(Size * 2), Expected_Lower, Expected_Upper);

  OrderedNode *Desired_Lower = LoadGPRRegister(X86State::REG_RBX, Size);
  OrderedNode *Desired_Upper = LoadGPRRegister(X86State::REG_RCX, Size);
  OrderedNode *Desired = _CreateElementPair(IR::SizeToOpSize(Size * 2), Desired_Lower, Desired_Upper);

  // ssa0 = Expected
  // ssa1 = Desired
  // ssa2 = MemoryLocation

  // DataSrc = *MemSrc
  // if (DataSrc == Expected) { *MemSrc == Desired; } Expected = DataSrc
  // This will write to memory! Careful!
  // Third operand must be a calculated guest memory address

  OrderedNode *CASResult = _CASPair(IR::SizeToOpSize(Size * 2), Expected, Desired, Src1);

  OrderedNode *Result_Lower = _ExtractElementPair(IR::SizeToOpSize(Size), CASResult, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(IR::SizeToOpSize(Size), CASResult, 1);

  HandleNZCV_RMW();
  _CmpPairZ(IR::SizeToOpSize(Size), CASResult, Expected);
  CalculateDeferredFlags();

  auto UpdateIfNotZF = [this](auto Reg, auto Value) {
    // Always use 64-bit csel to preserve existing upper bits. If we have a
    // 32-bit cmpxchg in a 64-bit context, Value will be zeroed in upper bits.
    StoreGPRRegister(Reg, _NZCVSelect(OpSize::i64Bit, CondClassType{COND_NEQ},
                                      Value, LoadGPRRegister(Reg)));
  };

  UpdateIfNotZF(X86State::REG_RAX, Result_Lower);
  UpdateIfNotZF(X86State::REG_RDX, Result_Upper);
}

void OpDispatchBuilder::CreateJumpBlocks(fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks) {
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

void OpDispatchBuilder::BeginFunction(uint64_t RIP, fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks, uint32_t NumInstructions) {
  Entry = RIP;
  auto IRHeader = _IRHeader(InvalidNode, RIP, 0, NumInstructions);
  CreateJumpBlocks(Blocks);

  auto Block = GetNewJumpBlock(RIP);
  SetCurrentCodeBlock(Block);
  IRHeader.first->Blocks = Block->Wrapped(DualListData.ListBegin());

  LOGMAN_THROW_A_FMT(IsDeferredFlagsStored(), "Something failed to calculate flags and now we began with invalid state");
}

void OpDispatchBuilder::Finalize() {
  // Calculate flags early.
  // This usually doesn't emit any IR but in the case of hitting the block instruction limit it will
  CalculateDeferredFlags();
  const uint8_t GPRSize = CTX->GetGPRSize();

  // Node 0 is invalid node
  OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(GetNode(1));

  [[maybe_unused]] const FEXCore::IR::IROp_Header *IROp =
  RealNode->Op(DualListData.DataBegin());
  LOGMAN_THROW_AA_FMT(IROp->Op == OP_IRHEADER, "First op in function must be our header");

  // Let's walk the jump blocks and see if we have handled every block target
  for (auto &Handler : JumpTargets) {
    if (Handler.second.HaveEmitted) continue;

    // We haven't emitted. Dump out to the dispatcher
    SetCurrentCodeBlock(Handler.second.BlockEntry);
    CalculateDeferredFlags();
    _ExitFunction(_EntrypointOffset(IR::SizeToOpSize(GPRSize), Handler.first - Entry));
  }
}

uint8_t OpDispatchBuilder::GetDstSize(X86Tables::DecodedOp Op) const {
  const uint32_t DstSizeFlag = X86Tables::DecodeFlags::GetSizeDstFlags(Op->Flags);
  LOGMAN_THROW_AA_FMT(DstSizeFlag != 0 && DstSizeFlag != X86Tables::DecodeFlags::SIZE_MASK, "Invalid destination size for op");
  return 1u << (DstSizeFlag - 1);
}

uint8_t OpDispatchBuilder::GetSrcSize(X86Tables::DecodedOp Op) const {
  const uint32_t SrcSizeFlag = X86Tables::DecodeFlags::GetSizeSrcFlags(Op->Flags);
  LOGMAN_THROW_AA_FMT(SrcSizeFlag != 0 && SrcSizeFlag != X86Tables::DecodeFlags::SIZE_MASK, "Invalid destination size for op");
  return 1u << (SrcSizeFlag - 1);
}

uint32_t OpDispatchBuilder::GetSrcBitSize(X86Tables::DecodedOp Op) const {
  return GetSrcSize(Op) * 8;
}

uint32_t OpDispatchBuilder::GetDstBitSize(X86Tables::DecodedOp Op) const {
  return GetDstSize(Op) * 8;
}

OrderedNode *OpDispatchBuilder::GetSegment(uint32_t Flags, uint32_t DefaultPrefix, bool Override) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  if (CTX->Config.Is64BitMode) {
    if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX) {
      return _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, fs_cached));
    }
    else if (Flags & FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
      return _LoadContext(GPRSize, GPRClass, offsetof(FEXCore::Core::CPUState, gs_cached));
    }
    // If there was any other segment in 64bit then it is ignored
  }
  else {
    uint32_t Prefix = Flags & FEXCore::X86Tables::DecodeFlags::FLAG_SEGMENTS;
    if (!Prefix || Override) {
      // If there was no prefix then use the default one if available
      // Or the argument only uses a specific prefix (with override set)
      Prefix = DefaultPrefix;
    }
    // With the segment register optimization we store the GDT bases directly in the segment register to remove indexed loads
    OrderedNode *SegmentResult{};
    switch (Prefix) {
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
      default:
        break; // Do nothing
    }

    CheckLegacySegmentRead(SegmentResult, Prefix);
    return SegmentResult;
  }
  return nullptr;
}

OrderedNode *OpDispatchBuilder::AppendSegmentOffset(OrderedNode *Value, uint32_t Flags, uint32_t DefaultPrefix, bool Override) {
  auto Segment = GetSegment(Flags, DefaultPrefix, Override);
  if (Segment) {
    Value = _Add(IR::SizeToOpSize(std::max<uint8_t>(4, std::max(GetOpSize(Value), GetOpSize(Segment)))), Value, Segment);
  }

  return Value;
}


void OpDispatchBuilder::CheckLegacySegmentRead(OrderedNode *NewNode, uint32_t SegmentReg) {
#ifndef FEX_DISABLE_TELEMETRY
  if (SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX ||
      SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
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

  FEXCore::Telemetry::TelemetryType TelemIndex{};
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

void OpDispatchBuilder::CheckLegacySegmentWrite(OrderedNode *NewNode, uint32_t SegmentReg) {
#ifndef FEX_DISABLE_TELEMETRY
  if (SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX ||
      SegmentReg == FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX) {
    // FS and GS segments aren't considered legacy.
    return;
  }

  if (CTX->Config.DisableTelemetry()) {
    // Telemetry disabled at runtime.
    return;
  }

  FEXCore::Telemetry::TelemetryType TelemIndex{};
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

void OpDispatchBuilder::UpdatePrefixFromSegment(OrderedNode *Segment, uint32_t SegmentReg) {
  // Use BFE to extract the selector index in bits [15,3] of the segment register.
  // In some cases the upper 16-bits of the 32-bit GPR contain garbage to ignore.
  Segment = _Bfe(OpSize::i32Bit, 16 - 3, 3, Segment);
  auto NewSegment = _LoadContextIndexed(Segment, 4, offsetof(FEXCore::Core::CPUState, gdt[0]), 4, GPRClass);
  CheckLegacySegmentWrite(NewSegment, SegmentReg);
  switch (SegmentReg) {
    case FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, es_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, cs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ss_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, ds_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, fs_cached));
      break;
    case FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX:
      _StoreContext(4, GPRClass, NewSegment, offsetof(FEXCore::Core::CPUState, gs_cached));
      break;
    default: break; // Do nothing
  }
}

OrderedNode *OpDispatchBuilder::LoadSource_WithOpSize(RegisterClassType Class, X86Tables::DecodedOp const& Op, X86Tables::DecodedOperand const& Operand,
                                                      uint8_t OpSize, uint32_t Flags, const LoadSourceOptions& Options) {
  LOGMAN_THROW_A_FMT(Operand.IsGPR() ||
                     Operand.IsLiteral() ||
                     Operand.IsGPRDirect() ||
                     Operand.IsGPRIndirect() ||
                     Operand.IsRIPRelative() ||
                     Operand.IsSIB(),
                     "Unsupported Src type");

  auto [Align, LoadData, ForceLoad, AccessType, AllowUpperGarbage] = Options;

  OrderedNode *Src {nullptr};
  bool LoadableType = false;
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint32_t AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  if (Operand.IsLiteral()) {
    uint64_t constant = Operand.Data.Literal.Value;
    uint64_t width = Operand.Data.Literal.Size * 8;

    if (Operand.Data.Literal.Size != 8) {
      // zero extend
      constant = constant & ((1ULL << width) - 1);
    }
    Src = _Constant(width, constant);
  }
  else if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    const auto highIndex = Operand.Data.GPR.HighBits ? 1 : 0;

    if (gpr >= FEXCore::X86State::REG_MM_0) {
      Src = _LoadContext(OpSize, FPRClass, offsetof(FEXCore::Core::CPUState, mm[gpr - FEXCore::X86State::REG_MM_0]));
    }
    else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      const auto gprIndex = gpr - X86State::REG_XMM_0;

      // Load the full register size if it is a XMM register source.
      Src = LoadXMMRegister(gprIndex);

      // Now extract the subregister if it was a partial load /smaller/ than SSE size
      // TODO: Instead of doing the VMov implicitly on load, hunt down all use cases that require partial loads and do it after load.
      // We don't have information here to know if the operation needs zero upper bits or can contain data.
      if (!AllowUpperGarbage && OpSize < Core::CPUState::XMM_SSE_REG_SIZE) {
        Src = _VMov(OpSize, Src);
      }
    }
    else {
      Src = LoadGPRRegister(gpr, OpSize, highIndex ? 8 : 0, AllowUpperGarbage);
    }
  }
  else if (Operand.IsGPRDirect()) {
    Src = LoadGPRRegister(Operand.Data.GPR.GPR, GPRSize);

    LoadableType = true;
    if (Operand.Data.GPR.GPR == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
      AccessType = MemoryAccessType::NONTSO;
    }
  }
  else if (Operand.IsGPRIndirect()) {
    auto GPR = LoadGPRRegister(Operand.Data.GPRIndirect.GPR, GPRSize);

    auto Constant = _Constant(GPRSize * 8, Operand.Data.GPRIndirect.Displacement);

    Src = _Add(IR::SizeToOpSize(GPRSize), GPR, Constant);

    LoadableType = true;
    if (Operand.Data.GPRIndirect.GPR == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
      AccessType = MemoryAccessType::NONTSO;
    }
  }
  else if (Operand.IsRIPRelative()) {
    if (CTX->Config.Is64BitMode) {
      Src = GetRelocatedPC(Op, Operand.Data.RIPLiteral.Value.s);
    }
    else {
      // 32bit this isn't RIP relative but instead absolute
      Src = _Constant(GPRSize * 8, Operand.Data.RIPLiteral.Value.u);
    }

    LoadableType = true;
  }
  else if (Operand.IsSIB()) {
    const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
    OrderedNode *Tmp{};

    if (!IsVSIB && Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID && Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
      auto Base = LoadGPRRegister(Operand.Data.SIB.Base, GPRSize);
      auto Index = LoadGPRRegister(Operand.Data.SIB.Index, GPRSize);
      if (Operand.Data.SIB.Scale == 1) {
        Tmp = _Add(IR::SizeToOpSize(GPRSize), Base, Index);
      }
      else {
        Tmp = _AddShift(IR::SizeToOpSize(GPRSize), Base, Index, ShiftType::LSL, FEXCore::ilog2(Operand.Data.SIB.Scale));
      }
    }
    else {
      // NOTE: VSIB cannot have the index * scale portion calculated ahead of time,
      //       since the index in this case is a vector. So, we can't just apply the scale
      //       to it, since this needs to be applied to each element in the index register
      //       after said element has been sign extended. So, we pass this through for the
      //       instruction implementation to handle.
      //
      //       What we do handle though, is the applying the displacement value to
      //       the base register (if a base register is provided), since this is a
      //       part of the address calculation that can be done ahead of time.
      if (Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID && !IsVSIB) {
        Tmp = LoadGPRRegister(Operand.Data.SIB.Index, GPRSize);

        if (Operand.Data.SIB.Scale != 1) {
          auto Constant = _Constant(GPRSize * 8, Operand.Data.SIB.Scale);
          Tmp = _Mul(IR::SizeToOpSize(GPRSize), Tmp, Constant);
        }
        if (Operand.Data.SIB.Index == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
          AccessType = MemoryAccessType::NONTSO;
        }
      }

      if (Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
        auto GPR = LoadGPRRegister(Operand.Data.SIB.Base, GPRSize);

        if (Tmp != nullptr) {
          Tmp = _Add(IR::SizeToOpSize(GPRSize), Tmp, GPR);
        }
        else {
          Tmp = GPR;
        }

        if (Operand.Data.SIB.Base == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
          AccessType = MemoryAccessType::NONTSO;
        }
      }
    }

    if (Operand.Data.SIB.Offset) {
      if (Tmp != nullptr) {
        Src = _Add(IR::SizeToOpSize(GPRSize), Tmp, _Constant(GPRSize * 8, Operand.Data.SIB.Offset));
      }
      else {
        Src = _Constant(GPRSize * 8, Operand.Data.SIB.Offset);
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
    LOGMAN_MSG_A_FMT("Unknown Src Type: {}\n", Operand.Type);
  }

  if (LoadableType && AddrSize < GPRSize) {
    // For 64-bit AddrSize can be 32-bit or 64-bit
    // For 32-bit AddrSize can be 32-bit or 16-bit
    //
    // If the AddrSize is not the GPRSize then we need to clear the upper bits.
    Src = _Bfe(IR::SizeToOpSize(GPRSize), AddrSize * 8, 0, Src);
  }

  if ((LoadableType && LoadData) || ForceLoad) {
    Src = AppendSegmentOffset(Src, Flags);

    if (AccessType == MemoryAccessType::NONTSO || AccessType == MemoryAccessType::STREAM) {
      Src = _LoadMem(Class, OpSize, Src, Align == -1 ? OpSize : Align);
    }
    else {
      Src = _LoadMemAutoTSO(Class, OpSize, Src, Align == -1 ? OpSize : Align);
    }
  }
  return Src;
}

OrderedNode *OpDispatchBuilder::GetRelocatedPC(FEXCore::X86Tables::DecodedOp const& Op, int64_t Offset) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  return _EntrypointOffset(IR::SizeToOpSize(GPRSize), Op->PC + Op->InstSize + Offset - Entry);
}

OrderedNode *OpDispatchBuilder::LoadGPRRegister(uint32_t GPR, int8_t Size, uint8_t Offset, bool AllowUpperGarbage) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  if (Size == -1) {
    Size = GPRSize;
  }
  OrderedNode *Reg = _LoadRegister(false, offsetof(FEXCore::Core::CPUState, gregs[GPR]), GPRClass, GPRFixedClass, GPRSize);

  if ((!AllowUpperGarbage && (Size != GPRSize)) || Offset != 0) {
    // Extract the subregister if requested.
    const auto OpSize = IR::SizeToOpSize(std::max<uint8_t>(4u, Size));
    if (AllowUpperGarbage)
      Reg = _Lshr(OpSize, Reg, _Constant(Offset));
    else
      Reg = _Bfe(OpSize, Size * 8, Offset, Reg);
  }
  return Reg;
}

OrderedNode *OpDispatchBuilder::LoadXMMRegister(uint32_t XMM) {
  const auto VectorSize = CTX->HostFeatures.SupportsAVX ? 32 : 16;
  const auto VectorOffset = CTX->HostFeatures.SupportsAVX ?
    offsetof(Core::CPUState, xmm.avx.data[XMM][0]) :
    offsetof(Core::CPUState, xmm.sse.data[XMM][0]);

  OrderedNode *Reg = _LoadRegister(false, VectorOffset, FPRClass, FPRFixedClass, VectorSize);
  return Reg;
}

void OpDispatchBuilder::StoreGPRRegister(uint32_t GPR, OrderedNode *const Src, int8_t Size, uint8_t Offset) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  if (Size == -1) {
    Size = GPRSize;
  }

  OrderedNode *Reg = Src;
  if (Size != GPRSize || Offset != 0) {
    // Need to do an insert if not automatic size or zero offset.
    Reg = LoadGPRRegister(GPR);
    Reg = _Bfi(IR::SizeToOpSize(GPRSize), Size * 8, Offset, Reg, Src);
  }

  _StoreRegister(Reg, false, offsetof(FEXCore::Core::CPUState, gregs[GPR]), GPRClass, GPRFixedClass, GPRSize);
}

void OpDispatchBuilder::StoreXMMRegister(uint32_t XMM, OrderedNode *const Src) {
  const auto VectorSize = CTX->HostFeatures.SupportsAVX ? 32 : 16;
  const auto VectorOffset = CTX->HostFeatures.SupportsAVX ?
    offsetof(Core::CPUState, xmm.avx.data[XMM][0]) :
    offsetof(Core::CPUState, xmm.sse.data[XMM][0]);

  _StoreRegister(Src, false, VectorOffset, FPRClass, FPRFixedClass, VectorSize);
}

OrderedNode *OpDispatchBuilder::LoadSource(RegisterClassType Class, X86Tables::DecodedOp const& Op, X86Tables::DecodedOperand const& Operand,
                                           uint32_t Flags, const LoadSourceOptions& Options) {
  const uint8_t OpSize = GetSrcSize(Op);
  return LoadSource_WithOpSize(Class, Op, Operand, OpSize, Flags, Options);
}

void OpDispatchBuilder::StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align, MemoryAccessType AccessType) {
  LOGMAN_THROW_A_FMT(Operand.IsGPR() ||
                     Operand.IsLiteral() ||
                     Operand.IsGPRDirect() ||
                     Operand.IsGPRIndirect() ||
                     Operand.IsRIPRelative() ||
                     Operand.IsSIB(),
                     "Unsupported Dest type");

  // 8Bit and 16bit destination types store their result without effecting the upper bits
  // 32bit ops ZEXT the result to 64bit
  OrderedNode *MemStoreDst {nullptr};
  bool MemStore = false;
  const uint8_t GPRSize = CTX->GetGPRSize();
  const uint32_t AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  if (Operand.IsLiteral()) {
    MemStoreDst = _Constant(Operand.Data.Literal.Size * 8, Operand.Data.Literal.Value);
    MemStore = true; // Literals are ONLY hardcoded memory destinations
  }
  else if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    if (gpr >= FEXCore::X86State::REG_MM_0) {
      _StoreContext(OpSize, Class, Src, offsetof(FEXCore::Core::CPUState, mm[gpr - FEXCore::X86State::REG_MM_0]));
    }
    else if (gpr >= FEXCore::X86State::REG_XMM_0) {
      const auto gprIndex = gpr - X86State::REG_XMM_0;
      const auto VectorSize = CTX->HostFeatures.SupportsAVX ? 32 : 16;

      auto Result = Src;
      if (OpSize != VectorSize) {
        // Partial writes can come from FPRs.
        // TODO: Fix the instructions doing partial writes rather than dealing with it here.

        LOGMAN_THROW_A_FMT(Class != IR::GPRClass, "Partial writes from GPR not allowed. Instruction: {}",
                           Op->TableInfo->Name);

        // XMM-size is handled in implementations.
        if (VectorSize != Core::CPUState::XMM_AVX_REG_SIZE || OpSize != Core::CPUState::XMM_SSE_REG_SIZE) {
          auto SrcVector = LoadXMMRegister(gprIndex);
          Result = _VInsElement(VectorSize, OpSize, 0, 0, SrcVector, Src);
        }
      }

      StoreXMMRegister(gprIndex, Result);
    }
    else {
      if (GPRSize == 8 && OpSize == 4) {
        // If the Source IR op is 64 bits, we need to zext the upper bits
        // For all other sizes, the upper bits are guaranteed to already be zero
        OrderedNode *Value = GetOpSize(Src) == 8 ? _Bfe(OpSize::i32Bit, 32, 0, Src) : Src;
        StoreGPRRegister(gpr, Value, GPRSize);

        LOGMAN_THROW_AA_FMT(!Operand.Data.GPR.HighBits, "Can't handle 32bit store to high 8bit register");
      }
      else {
        LOGMAN_THROW_AA_FMT(!(GPRSize == 4 && OpSize > 4), "Oops had a {} GPR load", OpSize);

        if (GPRSize != OpSize) {
          // if the GPR isn't the full size then we need to insert.
          // eg:
          // mov al, 2 ; Move in to lower 8-bits.
          // mov ah, 2 ; Move in to upper 8-bits of 16-bit reg.
          // mov ax, 2 ; Move in to lower 16-bits of reg.
          StoreGPRRegister(gpr, Src, OpSize, Operand.Data.GPR.HighBits * 8);
        }
        else {
          StoreGPRRegister(gpr, Src, std::min(GPRSize, OpSize));
        }
      }
    }
  }
  else if (Operand.IsGPRDirect()) {
    MemStoreDst = LoadGPRRegister(Operand.Data.GPR.GPR, GPRSize);

    MemStore = true;
    if (Operand.Data.GPR.GPR == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
      AccessType = MemoryAccessType::NONTSO;
    }
  }
  else if (Operand.IsGPRIndirect()) {
    auto GPR = LoadGPRRegister(Operand.Data.GPRIndirect.GPR, GPRSize);
    auto Constant = _Constant(GPRSize * 8, Operand.Data.GPRIndirect.Displacement);

    MemStoreDst = _Add(IR::SizeToOpSize(GPRSize), GPR, Constant);
    MemStore = true;
    if (Operand.Data.GPRIndirect.GPR == FEXCore::X86State::REG_RSP && AccessType == MemoryAccessType::DEFAULT) {
      AccessType = MemoryAccessType::NONTSO;
    }
  }
  else if (Operand.IsRIPRelative()) {
    if (CTX->Config.Is64BitMode) {
      MemStoreDst = GetRelocatedPC(Op, Operand.Data.RIPLiteral.Value.s);
    }
    else {
      // 32bit this isn't RIP relative but instead absolute
      MemStoreDst = _Constant(GPRSize * 8, Operand.Data.RIPLiteral.Value.u);
    }
    MemStore = true;
  }
  else if (Operand.IsSIB()) {
    OrderedNode *Tmp {};

    if (Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID && Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
      auto Base = LoadGPRRegister(Operand.Data.SIB.Base, GPRSize);
      auto Index = LoadGPRRegister(Operand.Data.SIB.Index, GPRSize);
      if (Operand.Data.SIB.Scale == 1) {
        Tmp = _Add(IR::SizeToOpSize(GPRSize), Base, Index);
      }
      else {
        Tmp = _AddShift(IR::SizeToOpSize(GPRSize), Base, Index, ShiftType::LSL, FEXCore::ilog2(Operand.Data.SIB.Scale));
      }
    }
    else {
      if (Operand.Data.SIB.Index != FEXCore::X86State::REG_INVALID) {
        Tmp = LoadGPRRegister(Operand.Data.SIB.Index, GPRSize);

        if (Operand.Data.SIB.Scale != 1) {
          auto Constant = _Constant(GPRSize * 8, Operand.Data.SIB.Scale);
          Tmp = _Mul(IR::SizeToOpSize(GPRSize), Tmp, Constant);
        }
      }

      if (Operand.Data.SIB.Base != FEXCore::X86State::REG_INVALID) {
        auto GPR = LoadGPRRegister(Operand.Data.SIB.Base, GPRSize);

        if (Tmp != nullptr) {
          Tmp = _Add(IR::SizeToOpSize(GPRSize), Tmp, GPR);
        }
        else {
          Tmp = GPR;
        }
      }
    }

    if (Operand.Data.SIB.Offset) {
      if (Tmp != nullptr) {
        MemStoreDst = _Add(IR::SizeToOpSize(GPRSize), Tmp, _Constant(GPRSize * 8, Operand.Data.SIB.Offset));
      }
      else {
        MemStoreDst = _Constant(GPRSize * 8, Operand.Data.SIB.Offset);
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

    if (AddrSize < GPRSize) {
      // If AddrSize == 16 then we need to clear the upper bits
      // GPRSize will be 32 in this case
      MemStoreDst = _Bfe(IR::SizeToOpSize(std::max<uint8_t>(4u, AddrSize)), AddrSize * 8, 0, MemStoreDst);
    }

    MemStore = true;
  }

  if (MemStore) {
    MemStoreDst = AppendSegmentOffset(MemStoreDst, Op->Flags);

    if (OpSize == 10) {
      // For X87 extended doubles, split before storing
      _StoreMem(FPRClass, 8, MemStoreDst, Src, Align);
      auto Upper = _VExtractToGPR(16, 8, Src, 1);
      auto DestAddr = _Add(OpSize::i64Bit, MemStoreDst, _Constant(8));
      _StoreMem(GPRClass, 2, DestAddr, Upper, std::min<uint8_t>(Align, 8));
    } else {
      if (AccessType == MemoryAccessType::NONTSO || AccessType == MemoryAccessType::STREAM) {
        _StoreMem(Class, OpSize, MemStoreDst, Src, Align == -1 ? OpSize : Align);
      }
      else {
        _StoreMemAutoTSO(Class, OpSize, MemStoreDst, Src, Align == -1 ? OpSize : Align);
      }
    }
  }
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, int8_t Align, MemoryAccessType AccessType) {
  StoreResult_WithOpSize(Class, Op, Operand, Src, GetDstSize(Op), Align, AccessType);
}

void OpDispatchBuilder::StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src, int8_t Align, MemoryAccessType AccessType) {
  StoreResult(Class, Op, Op->Dest, Src, Align, AccessType);
}

OpDispatchBuilder::OpDispatchBuilder(FEXCore::Context::ContextImpl *ctx)
  : IREmitter {ctx->OpDispatcherAllocator}
  , CTX {ctx} {
  ResetWorkingList();
  InstallHostSpecificOpcodeHandlers();
}
OpDispatchBuilder::OpDispatchBuilder(FEXCore::Utils::IntrusivePooledAllocator &Allocator)
  : IREmitter {Allocator}
  , CTX {nullptr} {
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
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIndex], Op->Flags, {.Align = 1});
  StoreResult(GPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVGPRNTOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});
  StoreResult(GPRClass, Op, Src, 1, MemoryAccessType::STREAM);
}

void OpDispatchBuilder::ALUOpImpl(OpcodeArgs, FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp, unsigned SrcIdx) {
  /* On x86, the canonical way to zero a register is XOR with itself...  because
   * modern x86 detects this pattern in hardware. arm64 does not detect this
   * pattern, we should do it like the x86 hardware would. On arm64, "mov x0,
   * #0" is faster than "eor x0, x0, x0". Additionally this lets more constant
   * folding kick in for flags.
   */
  if (!DestIsLockedMem(Op) &&
    ALUIROp == FEXCore::IR::IROps::OP_XOR &&
    Op->Dest.IsGPR() && Op->Src[SrcIdx].IsGPR() &&
    Op->Dest.Data.GPR == Op->Src[SrcIdx].Data.GPR) {

    auto Result = _Constant(0);
    StoreResult(GPRClass, Op, Result, -1);
    GenerateFlags_Logical(Op, Result, Result, Result);
    return;
  }

  auto Size = GetDstSize(Op);

  auto RoundedSize = Size;
  if (ALUIROp != FEXCore::IR::IROps::OP_ANDWITHFLAGS)
    RoundedSize = std::max<uint8_t>(4u, RoundedSize);

  const auto OpSize = IR::SizeToOpSize(RoundedSize);

  // X86 basic ALU ops just do the operation between the destination and a single source
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[SrcIdx], Op->Flags, {.AllowUpperGarbage = true});

  OrderedNode *Result{};
  OrderedNode *Dest{};

  if (DestIsLockedMem(Op)) {
    HandledLock = true;
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);

    DeriveOp(FetchOp, AtomicFetchOp, _AtomicFetchAdd(IR::SizeToOpSize(Size), Src, DestMem));
    Dest = FetchOp;
  }
  else {
    Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.AllowUpperGarbage = true});
  }

  DeriveOp(ALUOp, ALUIROp, _AndWithFlags(OpSize, Dest, Src));
  Result = ALUOp;

  // Flags set
  switch (ALUIROp) {
  case FEXCore::IR::IROps::OP_ADD:
    Result = CalculateFlags_ADD(Size, Dest, Src);
  break;
  case FEXCore::IR::IROps::OP_SUB:
    Result = CalculateFlags_SUB(Size, Dest, Src);
  break;
  case FEXCore::IR::IROps::OP_XOR:
  case FEXCore::IR::IROps::OP_OR: {
    GenerateFlags_Logical(Op, Result, Dest, Src);
  break;
  }
  case FEXCore::IR::IROps::OP_ANDWITHFLAGS: {
    HandleNZ00Write();
    CalculatePF(Result);
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
  break;
  }
  default: break;
  }

  if (!DestIsLockedMem(Op))
    StoreResult(GPRClass, Op, Result, -1);
}

template<FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp>
void OpDispatchBuilder::ALUOp(OpcodeArgs) {
  ALUOpImpl(Op, ALUIROp, AtomicFetchOp, 0);
}

void OpDispatchBuilder::INTOp(OpcodeArgs) {
  IR::BreakDefinition Reason;
  bool SetRIPToNext = false;

  switch (Op->OP) {
  case 0xCD: { // INT imm8
    uint8_t Literal = Op->Src[0].Data.Literal.Value;

#ifndef _WIN32
    constexpr uint8_t SYSCALL_LITERAL = 0x80;
#else
    constexpr uint8_t SYSCALL_LITERAL = 0x2E;
#endif
    if (Literal == SYSCALL_LITERAL) {
      if (CTX->Config.Is64BitMode()) [[unlikely]] {
        ERROR_AND_DIE_FMT("[Unsupported] Trying to execute 32-bit syscall from a 64-bit process.");
      }
      // Syscall on linux
      SyscallOp<false>(Op);
      return;
    }

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
  }

  // Calculate flags early.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();

  if (SetRIPToNext) {
    BlockSetRIP = SetRIPToNext;

    // We want to set RIP to the next instruction after INT3/INT1
    auto NewRIP = GetRelocatedPC(Op);
    _StoreContext(GPRSize, GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));
  }
  else if (Op->OP != 0xCE) {
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
    _Break(Reason);

    // Make sure to start a new block after ending this one
    auto JumpTarget = CreateNewCodeBlockAfter(FalseBlock);
    SetTrueJumpTarget(CondJump_, JumpTarget);
    SetCurrentCodeBlock(JumpTarget);
    StartNewBlock();
  }
  else {
    BlockSetRIP = true;
    _Break(Reason);
  }
}

void OpDispatchBuilder::TZCNT(OpcodeArgs) {
  // _FindTrailingZeroes ignores upper garbage so we don't need to mask
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  Src = _FindTrailingZeroes(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Src, -1);

  GenerateFlags_ZCNT(Op, Src);
}

void OpDispatchBuilder::LZCNT(OpcodeArgs) {
  // _CountLeadingZeroes clears upper garbage so we don't need to mask
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.AllowUpperGarbage = true});

  auto Res = _CountLeadingZeroes(OpSizeFromSrc(Op), Src);
  StoreResult(GPRClass, Op, Res, -1);
  GenerateFlags_ZCNT(Op, Res);
}

void OpDispatchBuilder::MOVBEOp(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});
  Src = _Rev(IR::SizeToOpSize(std::max<uint8_t>(4u, SrcSize)), Src);

  if (SrcSize == 2) {
    // 16-bit does an insert.
    // Rev of 16-bit value as 32-bit replaces the result in the upper 16-bits of the result.
    // bfxil the 16-bit result in to the GPR.
    OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);
    auto Result = _Bfxil(IR::SizeToOpSize(GPRSize), 16, 16, Dest, Src);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
  }
  else {
    // 32-bit does regular zext
    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
  }
}

void OpDispatchBuilder::CLWB(OpcodeArgs) {
  OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  DestMem = AppendSegmentOffset(DestMem, Op->Flags);
  _CacheLineClean(DestMem);
}

void OpDispatchBuilder::CLFLUSHOPT(OpcodeArgs) {
  OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  DestMem = AppendSegmentOffset(DestMem, Op->Flags);
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
  }
  else {
    LogMan::Msg::EFmt("Application tried using XSAVEOPT");
    UnimplementedOp(Op);
  }
}

void OpDispatchBuilder::StoreFenceOrCLFlush(OpcodeArgs) {
  if (Op->ModRM == 0xF8) {
    // 0xF8 is SFENCE
    _Fence({FEXCore::IR::Fence_Store});
  }
  else {
    // This is a CLFlush
    OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    DestMem = AppendSegmentOffset(DestMem, Op->Flags);
    _CacheLineClear(DestMem, true);
  }
}

void OpDispatchBuilder::CLZeroOp(OpcodeArgs) {
  OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  _CacheLineZero(DestMem);
}

template<bool ForStore, bool Stream, uint8_t Level>
void OpDispatchBuilder::Prefetch(OpcodeArgs) {
  OrderedNode *DestMem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Prefetch(ForStore, Stream, Level, DestMem);
}

void OpDispatchBuilder::RDTSCPOp(OpcodeArgs) {
  // RDTSCP is slightly different than RDTSC
  // IA32_TSC_AUX is returned in RCX
  // All previous loads are globally visible
  //  - Explicitly does not wait for stores to be globally visible
  //  - Explicitly use an MFENCE before this instruction if you want this behaviour
  // This instruction is not an execution fence, so subsequent instructions can execute after this
  //  - Explicitly use an LFENCE after RDTSCP if you want to block this behaviour

  _Fence({FEXCore::IR::Fence_Load});
  auto Counter = CycleCounter();

  auto ID = _ProcessorID();
  StoreGPRRegister(X86State::REG_RAX, Counter.CounterLow);
  StoreGPRRegister(X86State::REG_RCX, ID);
  StoreGPRRegister(X86State::REG_RDX, Counter.CounterHigh);
}

void OpDispatchBuilder::RDPIDOp(OpcodeArgs) {
  StoreResult(GPRClass, Op, _ProcessorID(), -1);
}

void OpDispatchBuilder::CRC32(OpcodeArgs) {
  const uint8_t GPRSize = CTX->GetGPRSize();

  // Destination GPR size is always 4 or 8 bytes depending on widening
  uint8_t DstSize = Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_REX_WIDENING ? 8 : 4;
  OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, GPRSize, Op->Flags);

  // Incoming memory is 8, 16, 32, or 64
  OrderedNode *Src{};
  if (Op->Src[0].IsGPR()) {
    Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], GPRSize, Op->Flags);
  }
  else {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});
  }
  auto Result = _CRC32(Dest, Src, GetSrcSize(Op));
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template<bool Reseed>
void OpDispatchBuilder::RDRANDOp(OpcodeArgs) {
  auto Res = _RDRAND(Reseed);

  OrderedNode *Result_Lower = _ExtractElementPair(OpSize::i64Bit, Res, 0);
  OrderedNode *Result_Upper = _ExtractElementPair(OpSize::i64Bit, Res, 1);

  StoreResult(GPRClass, Op, Result_Lower, -1);
  GenerateFlags_RDRAND(Op, Result_Upper);
}

void OpDispatchBuilder::UnimplementedOp(OpcodeArgs) {
  // Ensure flags are calculated on invalid op.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();

  // We don't actually support this instruction
  // Multiblock may hit it though
  _StoreContext(GPRSize, GPRClass, GetRelocatedPC(Op, -Op->InstSize), offsetof(FEXCore::Core::CPUState, rip));
  _Break(FEXCore::IR::BreakDefinition {
    .ErrorRegister = 0,
    .Signal = SIGILL,
    .TrapNumber = 0,
    .si_code = 0,
  });

  BlockSetRIP = true;

  if (Multiblock) {
    auto NextBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetCurrentCodeBlock(NextBlock);
    StartNewBlock();
  }
}

void OpDispatchBuilder::InvalidOp(OpcodeArgs) {
  // Ensure flags are calculated on invalid op.
  CalculateDeferredFlags();

  const uint8_t GPRSize = CTX->GetGPRSize();

  // We don't actually support this instruction
  // Multiblock may hit it though
  _StoreContext(GPRSize, GPRClass, GetRelocatedPC(Op, -Op->InstSize), offsetof(FEXCore::Core::CPUState, rip));
  _Break(FEXCore::IR::BreakDefinition {
    .ErrorRegister = 0,
    .Signal = SIGILL,
    .TrapNumber = 0,
    .si_code = 0,
  });
  BlockSetRIP = true;
}

#undef OpcodeArgs


void OpDispatchBuilder::InstallHostSpecificOpcodeHandlers() {
  static bool Initialized = false;
  if (!CTX || Initialized) {
    // IRCompaction doesn't set a CTX and doesn't need this anyway
    return;
  }
#define OPD(prefix, opcode) (((prefix) << 8) | opcode)
  constexpr uint16_t PF_38_NONE = 0;
  constexpr uint16_t PF_38_66   = (1U << 0);
  constexpr uint16_t PF_38_F2   = (1U << 1);

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F38_SHA[] = {
    {OPD(PF_38_NONE, 0xC8), 1, &OpDispatchBuilder::SHA1NEXTEOp},
    {OPD(PF_38_NONE, 0xC9), 1, &OpDispatchBuilder::SHA1MSG1Op},
    {OPD(PF_38_NONE, 0xCA), 1, &OpDispatchBuilder::SHA1MSG2Op},
    {OPD(PF_38_NONE, 0xCB), 1, &OpDispatchBuilder::SHA256RNDS2Op},
    {OPD(PF_38_NONE, 0xCC), 1, &OpDispatchBuilder::SHA256MSG1Op},
    {OPD(PF_38_NONE, 0xCD), 1, &OpDispatchBuilder::SHA256MSG2Op},
  };

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F38_AES[] = {
    {OPD(PF_38_66, 0xDB), 1, &OpDispatchBuilder::AESImcOp},
    {OPD(PF_38_66, 0xDC), 1, &OpDispatchBuilder::AESEncOp},
    {OPD(PF_38_66, 0xDD), 1, &OpDispatchBuilder::AESEncLastOp},
    {OPD(PF_38_66, 0xDE), 1, &OpDispatchBuilder::AESDecOp},
    {OPD(PF_38_66, 0xDF), 1, &OpDispatchBuilder::AESDecLastOp},
  };
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F38_CRC[] = {
    {OPD(PF_38_F2, 0xF0), 1, &OpDispatchBuilder::CRC32},
    {OPD(PF_38_F2, 0xF1), 1, &OpDispatchBuilder::CRC32},

    {OPD(PF_38_66 | PF_38_F2, 0xF0), 1, &OpDispatchBuilder::CRC32},
    {OPD(PF_38_66 | PF_38_F2, 0xF1), 1, &OpDispatchBuilder::CRC32},
  };
#undef OPD

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66   1
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F3A_AES[] = {
    {OPD(0, PF_3A_66,   0xDF), 1, &OpDispatchBuilder::AESKeyGenAssist},
  };
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F3A_PCLMUL[] = {
    {OPD(0, PF_3A_66,   0x44), 1, &OpDispatchBuilder::PCLMULQDQOp},
  };

#undef PF_3A_NONE
#undef PF_3A_66
#undef OPD

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> VEX_PCLMUL[] = {
    {OPD(3, 0b01, 0x44), 1, &OpDispatchBuilder::VPCLMULQDQOp},
  };
#undef OPD

#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  constexpr uint16_t PF_NONE = 0;
  constexpr uint16_t PF_66 = 2;

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> SecondaryExtensionOp_RDRAND[] = {
    // GROUP 9
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_NONE, 6), 1, &OpDispatchBuilder::RDRANDOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_NONE, 7), 1, &OpDispatchBuilder::RDRANDOp<true>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_66, 6), 1, &OpDispatchBuilder::RDRANDOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_66, 7), 1, &OpDispatchBuilder::RDRANDOp<true>},
  };
#undef OPD

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> SecondaryModRMExtensionOp_CLZero[] = {
    {((3 << 3) | 4), 1, &OpDispatchBuilder::CLZeroOp},
  };

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  static constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> AVXTable[] = {
    {OPD(1, 0b00, 0x10), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b01, 0x10), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b10, 0x10), 1, &OpDispatchBuilder::VMOVSSOp},
    {OPD(1, 0b11, 0x10), 1, &OpDispatchBuilder::VMOVSDOp},
    {OPD(1, 0b00, 0x11), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b01, 0x11), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b10, 0x11), 1, &OpDispatchBuilder::VMOVSSOp},
    {OPD(1, 0b11, 0x11), 1, &OpDispatchBuilder::VMOVSDOp},

    {OPD(1, 0b00, 0x12), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b01, 0x12), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b10, 0x12), 1, &OpDispatchBuilder::VMOVSLDUPOp},
    {OPD(1, 0b11, 0x12), 1, &OpDispatchBuilder::VMOVDDUPOp},
    {OPD(1, 0b00, 0x13), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b01, 0x13), 1, &OpDispatchBuilder::VMOVLPOp},

    {OPD(1, 0b00, 0x14), 1, &OpDispatchBuilder::VPUNPCKLOp<4>},
    {OPD(1, 0b01, 0x14), 1, &OpDispatchBuilder::VPUNPCKLOp<8>},

    {OPD(1, 0b00, 0x15), 1, &OpDispatchBuilder::VPUNPCKHOp<4>},
    {OPD(1, 0b01, 0x15), 1, &OpDispatchBuilder::VPUNPCKHOp<8>},

    {OPD(1, 0b00, 0x16), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b01, 0x16), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b10, 0x16), 1, &OpDispatchBuilder::VMOVSHDUPOp},
    {OPD(1, 0b00, 0x17), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b01, 0x17), 1, &OpDispatchBuilder::VMOVHPOp},

    {OPD(1, 0b00, 0x28), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b01, 0x28), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b00, 0x29), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b01, 0x29), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},

    {OPD(1, 0b10, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<4>},
    {OPD(1, 0b11, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<8>},

    {OPD(1, 0b00, 0x2B), 1, &OpDispatchBuilder::MOVVectorNTOp},
    {OPD(1, 0b01, 0x2B), 1, &OpDispatchBuilder::MOVVectorNTOp},

    {OPD(1, 0b10, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, false>},
    {OPD(1, 0b11, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, false>},

    {OPD(1, 0b10, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    {OPD(1, 0b11, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, true>},

    {OPD(1, 0b00, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<4>},
    {OPD(1, 0b01, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<8>},
    {OPD(1, 0b00, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<4>},
    {OPD(1, 0b01, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<8>},

    {OPD(1, 0b00, 0x50), 1, &OpDispatchBuilder::MOVMSKOp<4>},
    {OPD(1, 0b01, 0x50), 1, &OpDispatchBuilder::MOVMSKOp<8>},

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 4>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 8>},
    {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRSQRT, 4>},
    {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRECP, 4>},
    {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VAND, 16>},

    {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::VANDNOp},
    {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::VANDNOp},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VOR, 16>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VXOR, 16>},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VXOR, 16>},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFADD, 4>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFADD, 8>},
    {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMUL, 4>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMUL, 8>},
    {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<4, 8>},
    {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<4, 8>},

    {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, false>},
    {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, true>},
    {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFSUB, 4>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFSUB, 8>},
    {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMIN, 4>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMIN, 8>},
    {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFDIV, 4>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFDIV, 8>},
    {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMAX, 4>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMAX, 8>},
    {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 8>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::VPUNPCKLOp<1>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::VPUNPCKLOp<2>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::VPUNPCKLOp<4>},
    {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::VPACKSSOp<2>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 1>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 2>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 4>},
    {OPD(1, 0b01, 0x67), 1, &OpDispatchBuilder::VPACKUSOp<2>},
    {OPD(1, 0b01, 0x68), 1, &OpDispatchBuilder::VPUNPCKHOp<1>},
    {OPD(1, 0b01, 0x69), 1, &OpDispatchBuilder::VPUNPCKHOp<2>},
    {OPD(1, 0b01, 0x6A), 1, &OpDispatchBuilder::VPUNPCKHOp<4>},
    {OPD(1, 0b01, 0x6B), 1, &OpDispatchBuilder::VPACKSSOp<4>},
    {OPD(1, 0b01, 0x6C), 1, &OpDispatchBuilder::VPUNPCKLOp<8>},
    {OPD(1, 0b01, 0x6D), 1, &OpDispatchBuilder::VPUNPCKHOp<8>},
    {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},

    {OPD(1, 0b01, 0x6F), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},

    {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<4, true>},
    {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<2, false>},
    {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<2, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 1>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 2>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 4>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::VZEROOp},

    {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 8>},
    {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 4>},
    {OPD(1, 0b01, 0x7D), 1, &OpDispatchBuilder::VHSUBPOp<8>},
    {OPD(1, 0b11, 0x7D), 1, &OpDispatchBuilder::VHSUBPOp<4>},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {OPD(1, 0b10, 0x7E), 1, &OpDispatchBuilder::MOVQOp},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},

    {OPD(1, 0b00, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<4>},
    {OPD(1, 0b01, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<8>},
    {OPD(1, 0b10, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<4>},
    {OPD(1, 0b11, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<8>},

    {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::VPINSRWOp},
    {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::PExtrOp<2>},

    {OPD(1, 0b00, 0xC6), 1, &OpDispatchBuilder::VSHUFOp<4>},
    {OPD(1, 0b01, 0xC6), 1, &OpDispatchBuilder::VSHUFOp<8>},

    {OPD(1, 0b01, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<8>},
    {OPD(1, 0b11, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<4>},

    {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::VPSRLDOp<2>},
    {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::VPSRLDOp<4>},
    {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::VPSRLDOp<8>},
    {OPD(1, 0b01, 0xD4), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 8>},
    {OPD(1, 0b01, 0xD5), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VMUL, 2>},
    {OPD(1, 0b01, 0xD6), 1, &OpDispatchBuilder::MOVQOp},
    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::MOVMSKOpOne},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQSUB, 1>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQSUB, 2>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 1>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQADD, 1>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQADD, 2>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 1>},
    {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::VANDNOp},

    {OPD(1, 0b01, 0xE0), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VURAVG, 1>},
    {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::VPSRAOp<2>},
    {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::VPSRAOp<4>},
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VURAVG, 2>},
    {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::VPMULHWOp<false>},
    {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::VPMULHWOp<true>},

    {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, false>},
    {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, true>},
    {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, true>},

    {OPD(1, 0b01, 0xE7), 1, &OpDispatchBuilder::MOVVectorNTOp},

    {OPD(1, 0b01, 0xE8), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQSUB, 1>},
    {OPD(1, 0b01, 0xE9), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQSUB, 2>},
    {OPD(1, 0b01, 0xEA), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 2>},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0xEC), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQADD, 1>},
    {OPD(1, 0b01, 0xED), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQADD, 2>},
    {OPD(1, 0b01, 0xEE), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 2>},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VXOR, 16>},

    {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::VPSLLOp<2>},
    {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::VPSLLOp<4>},
    {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::VPSLLOp<8>},
    {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::VPMULLOp<4, false>},
    {OPD(1, 0b01, 0xF5), 1, &OpDispatchBuilder::VPMADDWDOp},
    {OPD(1, 0b01, 0xF6), 1, &OpDispatchBuilder::VPSADBWOp},
    {OPD(1, 0b01, 0xF7), 1, &OpDispatchBuilder::MASKMOVOp},

    {OPD(1, 0b01, 0xF8), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 1>},
    {OPD(1, 0b01, 0xF9), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 2>},
    {OPD(1, 0b01, 0xFA), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 4>},
    {OPD(1, 0b01, 0xFB), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 8>},
    {OPD(1, 0b01, 0xFC), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 1>},
    {OPD(1, 0b01, 0xFD), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 2>},
    {OPD(1, 0b01, 0xFE), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 4>},

    {OPD(2, 0b01, 0x00), 1, &OpDispatchBuilder::VPSHUFBOp},
    {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 2>},
    {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 4>},
    {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::VPHADDSWOp},
    {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::VPMADDUBSWOp},

    {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::VPHSUBOp<2>},
    {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::VPHSUBOp<4>},
    {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::VPHSUBSWOp},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::VPSIGN<1>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::VPSIGN<2>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::VPSIGN<4>},
    {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::VPMULHRSWOp},
    {OPD(2, 0b01, 0x0C), 1, &OpDispatchBuilder::VPERMILRegOp<4>},
    {OPD(2, 0b01, 0x0D), 1, &OpDispatchBuilder::VPERMILRegOp<8>},
    {OPD(2, 0b01, 0x0E), 1, &OpDispatchBuilder::VTESTPOp<4>},
    {OPD(2, 0b01, 0x0F), 1, &OpDispatchBuilder::VTESTPOp<8>},

    {OPD(2, 0b01, 0x16), 1, &OpDispatchBuilder::VPERMDOp},
    {OPD(2, 0b01, 0x17), 1, &OpDispatchBuilder::PTestOp},
    {OPD(2, 0b01, 0x18), 1, &OpDispatchBuilder::VBROADCASTOp<4>},
    {OPD(2, 0b01, 0x19), 1, &OpDispatchBuilder::VBROADCASTOp<8>},
    {OPD(2, 0b01, 0x1A), 1, &OpDispatchBuilder::VBROADCASTOp<16>},
    {OPD(2, 0b01, 0x1C), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 1>},
    {OPD(2, 0b01, 0x1D), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 2>},
    {OPD(2, 0b01, 0x1E), 1, &OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 4>},

    {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, true>},
    {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, true>},
    {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, true>},
    {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, true>},
    {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, true>},
    {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, true>},

    {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::VPMULLOp<4, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 8>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::MOVVectorNTOp},
    {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::VPACKUSOp<4>},
    {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::VMASKMOVOp<4, false>},
    {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::VMASKMOVOp<8, false>},
    {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::VMASKMOVOp<4, true>},
    {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::VMASKMOVOp<8, true>},

    {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, false>},
    {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, false>},
    {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, false>},
    {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, false>},
    {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, false>},
    {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, false>},
    {OPD(2, 0b01, 0x36), 1, &OpDispatchBuilder::VPERMDOp},

    {OPD(2, 0b01, 0x37), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 8>},
    {OPD(2, 0b01, 0x38), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 1>},
    {OPD(2, 0b01, 0x39), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 4>},
    {OPD(2, 0b01, 0x3A), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 2>},
    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 4>},
    {OPD(2, 0b01, 0x3C), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 1>},
    {OPD(2, 0b01, 0x3D), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 4>},
    {OPD(2, 0b01, 0x3E), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 2>},
    {OPD(2, 0b01, 0x3F), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 4>},

    {OPD(2, 0b01, 0x40), 1, &OpDispatchBuilder::AVXVectorALUOp<IR::OP_VMUL, 4>},
    {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},
    {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::VPSRLVOp},
    {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::VPSRAVDOp},
    {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::VPSLLVOp},

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::VBROADCASTOp<4>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::VBROADCASTOp<8>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::VBROADCASTOp<16>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::VBROADCASTOp<1>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::VBROADCASTOp<2>},

    {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::VPMASKMOVOp<false>},
    {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::VPMASKMOVOp<true>},

    {OPD(2, 0b01, 0xDB), 1, &OpDispatchBuilder::AESImcOp},
    {OPD(2, 0b01, 0xDC), 1, &OpDispatchBuilder::VAESEncOp},
    {OPD(2, 0b01, 0xDD), 1, &OpDispatchBuilder::VAESEncLastOp},
    {OPD(2, 0b01, 0xDE), 1, &OpDispatchBuilder::VAESDecOp},
    {OPD(2, 0b01, 0xDF), 1, &OpDispatchBuilder::VAESDecLastOp},

    {OPD(3, 0b01, 0x00), 1, &OpDispatchBuilder::VPERMQOp},
    {OPD(3, 0b01, 0x01), 1, &OpDispatchBuilder::VPERMQOp},
    {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::VPBLENDDOp},
    {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::VPERMILImmOp<4>},
    {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::VPERMILImmOp<8>},
    {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::VPERM2Op},
    {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::AVXVectorRound<4>},
    {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::AVXVectorRound<8>},
    {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::AVXInsertScalarRound<4>},
    {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::AVXInsertScalarRound<8>},
    {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::VPBLENDDOp},
    {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::VBLENDPDOp},
    {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::VPBLENDWOp},
    {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::VPALIGNROp},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::PExtrOp<1>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::PExtrOp<2>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::PExtrOp<4>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::PExtrOp<4>},

    {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::VINSERTOp},
    {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::VEXTRACT128Op},
    {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::VPINSRBOp},
    {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::VINSERTPSOp},
    {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::VPINSRDQOp},

    {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::VINSERTOp},
    {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::VEXTRACT128Op},

    {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::VDPPOp<4>},
    {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::VDPPOp<8>},
    {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::VMPSADBWOp},

    {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::VPERM2Op},

    {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::AVXVectorVariableBlend<4>},
    {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::AVXVectorVariableBlend<8>},
    {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::AVXVectorVariableBlend<1>},

    {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
    {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
    {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
    {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

    {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  static constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> VEXTableGroupOps[] {
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<2>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<2>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1, &OpDispatchBuilder::VPSRAIOp<2>},

    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<4>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<4>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1, &OpDispatchBuilder::VPSRAIOp<4>},

    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<8>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::VPSRLDQOp},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<8>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::VPSLLDQOp},

    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b010), 1, &OpDispatchBuilder::LDMXCSR},
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b011), 1, &OpDispatchBuilder::STMXCSR},
  };
#undef OPD

  auto InstallToTable = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LOGMAN_THROW_A_FMT(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
        FinalTable[OpNum + i].OpcodeDispatcher = Dispatcher;
      }
    }
  };

  if (CTX->HostFeatures.SupportsCRC) {
    InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38_CRC);
  }

  InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38_SHA);

  if (CTX->HostFeatures.SupportsAES) {
    InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38_AES);
    InstallToTable(FEXCore::X86Tables::H0F3ATableOps, H0F3A_AES);
  }

  if (CTX->HostFeatures.SupportsCLZERO) {
    InstallToTable(FEXCore::X86Tables::SecondModRMTableOps, SecondaryModRMExtensionOp_CLZero);
  }

  if (CTX->HostFeatures.SupportsRAND) {
    InstallToTable(FEXCore::X86Tables::SecondInstGroupOps, SecondaryExtensionOp_RDRAND);
  }

  if (CTX->HostFeatures.SupportsAVX) {
    InstallToTable(FEXCore::X86Tables::VEXTableOps, AVXTable);
    InstallToTable(FEXCore::X86Tables::VEXTableGroupOps, VEXTableGroupOps);
  }

  if (CTX->HostFeatures.SupportsPMULL_128Bit) {
    InstallToTable(FEXCore::X86Tables::H0F3ATableOps, H0F3A_PCLMUL);
    InstallToTable(FEXCore::X86Tables::VEXTableOps, VEX_PCLMUL);
  }
  Initialized = true;
}

void InstallOpcodeHandlers(Context::OperatingMode Mode) {
  constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> BaseOpTable[] = {
    // Instructions
    {0x00, 6, &OpDispatchBuilder::ALUOp<FEXCore::IR::IROps::OP_ADD, FEXCore::IR::IROps::OP_ATOMICFETCHADD>},

    {0x08, 6, &OpDispatchBuilder::ALUOp<FEXCore::IR::IROps::OP_OR, FEXCore::IR::IROps::OP_ATOMICFETCHOR>},

    {0x10, 6, &OpDispatchBuilder::ADCOp<0>},

    {0x18, 6, &OpDispatchBuilder::SBBOp<0>},

    {0x20, 6, &OpDispatchBuilder::ALUOp<FEXCore::IR::IROps::OP_ANDWITHFLAGS, FEXCore::IR::IROps::OP_ATOMICFETCHAND>},

    {0x28, 6, &OpDispatchBuilder::ALUOp<FEXCore::IR::IROps::OP_SUB, FEXCore::IR::IROps::OP_ATOMICFETCHSUB>},

    {0x30, 6, &OpDispatchBuilder::ALUOp<FEXCore::IR::IROps::OP_XOR, FEXCore::IR::IROps::OP_ATOMICFETCHXOR>},

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

  constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> BaseOpTable_32[] = {
    {0x06, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX>},
    {0x07, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX>},
    {0x0E, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX>},
    {0x16, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX>},
    {0x17, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX>},
    {0x1E, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX>},
    {0x1F, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX>},
    {0x27, 1, &OpDispatchBuilder::DAAOp},
    {0x2F, 1, &OpDispatchBuilder::DASOp},
    {0x37, 1, &OpDispatchBuilder::AAAOp},
    {0x3F, 1, &OpDispatchBuilder::AASOp},
    {0x40, 8, &OpDispatchBuilder::INCOp},
    {0x48, 8, &OpDispatchBuilder::DECOp},

    {0x60, 1, &OpDispatchBuilder::PUSHAOp},
    {0x61, 1, &OpDispatchBuilder::POPAOp},
    {0xCE, 1, &OpDispatchBuilder::INTOp},
    {0xD4, 1, &OpDispatchBuilder::AAMOp},
    {0xD5, 1, &OpDispatchBuilder::AADOp},
    {0xD6, 1, &OpDispatchBuilder::SALCOp},
  };

  constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> BaseOpTable_64[] = {
    {0x63, 1, &OpDispatchBuilder::MOVSXDOp},
  };

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> TwoByteOpTable[] = {
    // Instructions
    {0x0B, 1, &OpDispatchBuilder::INTOp},
    {0x0E, 1, &OpDispatchBuilder::X87EMMS},

    {0x19, 7, &OpDispatchBuilder::NOPOp}, // NOP with ModRM

    {0x31, 1, &OpDispatchBuilder::RDTSCOp},

    {0x3F, 1, &OpDispatchBuilder::ThunkOp},
    {0x40, 16, &OpDispatchBuilder::CMOVOp},
    {0x6E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x6F, 1, &OpDispatchBuilder::MOVQMMXOp},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVQMMXOp},
    {0x80, 16, &OpDispatchBuilder::CondJUMPOp},
    {0x90, 16, &OpDispatchBuilder::SETccOp},
    {0xA0, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX>},
    {0xA1, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX>},
    {0xA2, 1, &OpDispatchBuilder::CPUIDOp},
    {0xA3, 1, &OpDispatchBuilder::BTOp<0, BTAction::BTNone>}, // BT
    {0xA4, 1, &OpDispatchBuilder::SHLDImmediateOp},
    {0xA5, 1, &OpDispatchBuilder::SHLDOp},
    {0xA8, 1, &OpDispatchBuilder::PUSHSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX>},
    {0xA9, 1, &OpDispatchBuilder::POPSegmentOp<FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX>},
    {0xAB, 1, &OpDispatchBuilder::BTOp<0, BTAction::BTSet>}, // BTS
    {0xAC, 1, &OpDispatchBuilder::SHRDImmediateOp},
    {0xAD, 1, &OpDispatchBuilder::SHRDOp},
    {0xAF, 1, &OpDispatchBuilder::IMUL1SrcOp},
    {0xB0, 2, &OpDispatchBuilder::CMPXCHGOp}, // CMPXCHG
    {0xB3, 1, &OpDispatchBuilder::BTOp<0, BTAction::BTClear>}, // BTR
    {0xB6, 2, &OpDispatchBuilder::MOVZXOp},
    {0xBB, 1, &OpDispatchBuilder::BTOp<0, BTAction::BTComplement>}, // BTC
    {0xBC, 1, &OpDispatchBuilder::BSFOp}, // BSF
    {0xBD, 1, &OpDispatchBuilder::BSROp}, // BSF
    {0xBE, 2, &OpDispatchBuilder::MOVSXOp},
    {0xC0, 2, &OpDispatchBuilder::XADDOp},
    {0xC3, 1, &OpDispatchBuilder::MOVGPRNTOp},
    {0xC4, 1, &OpDispatchBuilder::PINSROp<2>},
    {0xC5, 1, &OpDispatchBuilder::PExtrOp<2>},
    {0xC8, 8, &OpDispatchBuilder::BSWAPOp},

    // SSE
    {0x10, 2, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0x12, 2, &OpDispatchBuilder::MOVLPOp},
    {0x14, 1, &OpDispatchBuilder::PUNPCKLOp<4>},
    {0x15, 1, &OpDispatchBuilder::PUNPCKHOp<4>},
    {0x16, 2, &OpDispatchBuilder::MOVHPDOp},
    {0x28, 2, &OpDispatchBuilder::MOVVectorAlignedOp},
    {0x2A, 1, &OpDispatchBuilder::InsertMMX_To_XMM_Vector_CVT_Int_To_Float},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorNTOp},
    {0x2C, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<4, false, false>},
    {0x2D, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<4, false, true>},
    {0x2E, 2, &OpDispatchBuilder::UCOMISxOp<4>},
    {0x50, 1, &OpDispatchBuilder::MOVMSKOp<4>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4>},
    {0x52, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4>},
    {0x53, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4>},
    {0x54, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0x55, 1, &OpDispatchBuilder::VectorALUROp<IR::OP_VBIC, 8>},
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
    {0x70, 1, &OpDispatchBuilder::PSHUFW8ByteOp},

    {0x74, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 1>},
    {0x75, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 2>},
    {0x76, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 4>},
    {0x77, 1, &OpDispatchBuilder::X87EMMS},

    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<4>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<4>},

    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8>},
    {0xD4, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 8>},
    {0xD5, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VMUL, 2>},
    {0xD7, 1, &OpDispatchBuilder::MOVMSKOpOne}, // PMOVMSKB
    {0xD8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>},
    {0xD9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>},
    {0xDA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 8>},
    {0xDC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>},
    {0xDD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>},
    {0xDE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 1>},
    {0xDF, 1, &OpDispatchBuilder::VectorALUROp<IR::OP_VBIC, 8>},
    {0xE0, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4>},
    {0xE3, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE7, 1, &OpDispatchBuilder::MOVVectorNTOp},
    {0xE8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 1>},
    {0xE9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 2>},
    {0xEA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 2>},
    {0xEB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 8>},
    {0xEC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 1>},
    {0xED, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 2>},
    {0xEE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 2>},
    {0xEF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 8>},

    {0xF1, 1, &OpDispatchBuilder::PSLL<2>},
    {0xF2, 1, &OpDispatchBuilder::PSLL<4>},
    {0xF3, 1, &OpDispatchBuilder::PSLL<8>},
    {0xF4, 1, &OpDispatchBuilder::PMULLOp<4, false>},
    {0xF5, 1, &OpDispatchBuilder::PMADDWD},
    {0xF6, 1, &OpDispatchBuilder::PSADBW},
    {0xF7, 1, &OpDispatchBuilder::MASKMOVOp},
    {0xF8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 1>},
    {0xF9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 2>},
    {0xFA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 4>},
    {0xFB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 8>},
    {0xFC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 1>},
    {0xFD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 2>},
    {0xFE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 4>},

    // FEX reserved instructions
    {0x37, 1, &OpDispatchBuilder::CallbackReturnOp},
  };

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> TwoByteOpTable_32[] = {
    {0x05, 1, &OpDispatchBuilder::NOPOp},
  };

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> TwoByteOpTable_64[] = {
    {0x05, 1, &OpDispatchBuilder::SyscallOp<true>},
  };

#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> PrimaryGroupOpTable[] = {
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
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 3), 1, &OpDispatchBuilder::SBBOp<1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 7), 1, &OpDispatchBuilder::CMPOp<1>},

    // GROUP 2
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, &OpDispatchBuilder::RotateOp<true, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, &OpDispatchBuilder::RotateOp<false, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 6), 1, &OpDispatchBuilder::SHLImmediateOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, &OpDispatchBuilder::RotateOp<true, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, &OpDispatchBuilder::RotateOp<false, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, &OpDispatchBuilder::SHLImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, &OpDispatchBuilder::SHRImmediateOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 6), 1, &OpDispatchBuilder::SHLImmediateOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, &OpDispatchBuilder::ASHRImmediateOp}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, &OpDispatchBuilder::RotateOp<true, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, &OpDispatchBuilder::RotateOp<false, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 3), 1, &OpDispatchBuilder::RCROp8x1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 6), 1, &OpDispatchBuilder::SHLOp<true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, &OpDispatchBuilder::RotateOp<true, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, &OpDispatchBuilder::RotateOp<false, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 3), 1, &OpDispatchBuilder::RCROp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, &OpDispatchBuilder::SHLOp<true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, &OpDispatchBuilder::SHROp<true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 6), 1, &OpDispatchBuilder::SHLOp<true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, &OpDispatchBuilder::ASHROp<true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, &OpDispatchBuilder::RotateOp<true, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, &OpDispatchBuilder::RotateOp<false, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 2), 1, &OpDispatchBuilder::RCLSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 3), 1, &OpDispatchBuilder::RCRSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, &OpDispatchBuilder::SHLOp<false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, &OpDispatchBuilder::SHROp<false>}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 6), 1, &OpDispatchBuilder::SHLOp<false>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, &OpDispatchBuilder::ASHROp<false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, &OpDispatchBuilder::RotateOp<true, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, &OpDispatchBuilder::RotateOp<false, false, false>},
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

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> RepModOpTable[] = {
    {0x10, 2, &OpDispatchBuilder::MOVSSOp},
    {0x12, 1, &OpDispatchBuilder::VMOVSLDUPOp},
    {0x16, 1, &OpDispatchBuilder::VMOVSHDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x2A, 1, &OpDispatchBuilder::InsertCVTGPR_To_FPR<4>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorNTOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, false>},
    {0x2D, 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    {0x51, 1, &OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 4>},
    {0x52, 1, &OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 4>},
    {0x53, 1, &OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 4>},
    {0x58, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 4>},
    {0x59, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 4>},
    {0x5A, 1, &OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<8, 4>},
    {0x5B, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>},
    {0x5C, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 4>},
    {0x5D, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 4>},
    {0x5E, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 4>},
    {0x5F, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 4>},
    {0x6F, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFWOp<false>},
    {0x7E, 1, &OpDispatchBuilder::MOVQOp},
    {0x7F, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0xB8, 1, &OpDispatchBuilder::PopcountOp},
    {0xBC, 1, &OpDispatchBuilder::TZCNT},
    {0xBD, 1, &OpDispatchBuilder::LZCNT},
    {0xC2, 1, &OpDispatchBuilder::InsertScalarFCMPOp<4>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true>},
  };

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> RepNEModOpTable[] = {
    {0x10, 2, &OpDispatchBuilder::MOVSDOp},
    {0x12, 1, &OpDispatchBuilder::MOVDDUPOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x2A, 1, &OpDispatchBuilder::InsertCVTGPR_To_FPR<8>},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorNTOp},
    {0x2C, 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, false>},
    {0x2D, 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, true>},
    {0x51, 1, &OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 8>},
    //x52 = Invalid
    {0x58, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 8>},
    {0x59, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 8>},
    {0x5A, 1, &OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<4, 8>},
    {0x5C, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 8>},
    {0x5D, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 8>},
    {0x5E, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 8>},
    {0x5F, 1, &OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 8>},
    {0x70, 1, &OpDispatchBuilder::PSHUFWOp<true>},
    {0x7C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADDP, 4>},
    {0x7D, 1, &OpDispatchBuilder::HSUBP<4>},
    {0xD0, 1, &OpDispatchBuilder::ADDSUBPOp<4>},
    {0xD6, 1, &OpDispatchBuilder::MOVQ2DQ<false>},
    {0xC2, 1, &OpDispatchBuilder::InsertScalarFCMPOp<8>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, true>},
    {0xF0, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
  };

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpSizeModOpTable[] = {
    {0x10, 2, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0x12, 2, &OpDispatchBuilder::MOVLPOp},
    {0x14, 1, &OpDispatchBuilder::PUNPCKLOp<8>},
    {0x15, 1, &OpDispatchBuilder::PUNPCKHOp<8>},
    {0x16, 2, &OpDispatchBuilder::MOVHPDOp},
    {0x19, 7, &OpDispatchBuilder::NOPOp},
    {0x28, 2, &OpDispatchBuilder::MOVVectorAlignedOp},
    {0x2A, 1, &OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float},
    {0x2B, 1, &OpDispatchBuilder::MOVVectorNTOp},
    {0x2C, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true, false>},
    {0x2D, 1, &OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true, true>},
    {0x2E, 2, &OpDispatchBuilder::UCOMISxOp<8>},

    {0x40, 16, &OpDispatchBuilder::CMOVOp},
    {0x50, 1, &OpDispatchBuilder::MOVMSKOp<8>},
    {0x51, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8>},
    {0x54, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0x55, 1, &OpDispatchBuilder::VectorALUROp<IR::OP_VBIC, 8>},
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
    {0x6F, 1, &OpDispatchBuilder::MOVVectorAlignedOp},
    {0x70, 1, &OpDispatchBuilder::PSHUFDOp},

    {0x74, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 1>},
    {0x75, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 2>},
    {0x76, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 4>},
    {0x78, 1, nullptr}, // GROUP 17
    {0x7C, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADDP, 8>},
    {0x7D, 1, &OpDispatchBuilder::HSUBP<8>},
    {0x7E, 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {0x7F, 1, &OpDispatchBuilder::MOVVectorAlignedOp},
    {0xC2, 1, &OpDispatchBuilder::VFCMPOp<8>},
    {0xC4, 1, &OpDispatchBuilder::PINSROp<2>},
    {0xC5, 1, &OpDispatchBuilder::PExtrOp<2>},
    {0xC6, 1, &OpDispatchBuilder::SHUFOp<8>},

    {0xD0, 1, &OpDispatchBuilder::ADDSUBPOp<8>},
    {0xD1, 1, &OpDispatchBuilder::PSRLDOp<2>},
    {0xD2, 1, &OpDispatchBuilder::PSRLDOp<4>},
    {0xD3, 1, &OpDispatchBuilder::PSRLDOp<8>},
    {0xD4, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 8>},
    {0xD5, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VMUL, 2>},
    {0xD6, 1, &OpDispatchBuilder::MOVQOp},
    {0xD7, 1, &OpDispatchBuilder::MOVMSKOpOne}, // PMOVMSKB
    {0xD8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>},
    {0xD9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>},
    {0xDA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 1>},
    {0xDB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>},
    {0xDC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>},
    {0xDD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>},
    {0xDE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 1>},
    {0xDF, 1, &OpDispatchBuilder::VectorALUROp<IR::OP_VBIC, 8>},
    {0xE0, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 1>},
    {0xE1, 1, &OpDispatchBuilder::PSRAOp<2>},
    {0xE2, 1, &OpDispatchBuilder::PSRAOp<4>},
    {0xE3, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 2>},
    {0xE4, 1, &OpDispatchBuilder::PMULHW<false>},
    {0xE5, 1, &OpDispatchBuilder::PMULHW<true>},
    {0xE6, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, false>},
    {0xE7, 1, &OpDispatchBuilder::MOVVectorNTOp},
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
    {0xF8, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 1>},
    {0xF9, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 2>},
    {0xFA, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 4>},
    {0xFB, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 8>},
    {0xFC, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 1>},
    {0xFD, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 2>},
    {0xFE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 4>},
  };

constexpr uint16_t PF_NONE = 0;
constexpr uint16_t PF_F3 = 1;
constexpr uint16_t PF_66 = 2;
constexpr uint16_t PF_F2 = 3;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> SecondaryExtensionOpTable[] = {
    // GROUP 7
    {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 0), 1, &OpDispatchBuilder::SGDTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 0), 1, &OpDispatchBuilder::SGDTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 0), 1, &OpDispatchBuilder::SGDTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 0), 1, &OpDispatchBuilder::SGDTOp},

    // GROUP 8
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 4), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTNone>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 4), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTNone>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 4), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTNone>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 4), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTNone>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 5), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTSet>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 5), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTSet>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 5), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTSet>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 5), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTSet>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 6), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTClear>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 6), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTClear>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 6), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTClear>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 6), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTClear>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 7), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTComplement>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 7), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTComplement>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 7), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTComplement>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 7), 1, &OpDispatchBuilder::BTOp<1, BTAction::BTComplement>},

    // GROUP 9
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_NONE, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F3, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_66, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F2, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F3, 7), 1, &OpDispatchBuilder::RDPIDOp},

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
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 4), 1, &OpDispatchBuilder::XSaveOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 5), 1, &OpDispatchBuilder::LoadFenceOrXRSTOR},   // LFENCE (or XRSTOR)
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 6), 1, &OpDispatchBuilder::MemFenceOrXSAVEOPT},  // MFENCE (or XSAVEOPT)
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 7), 1, &OpDispatchBuilder::StoreFenceOrCLFlush}, // SFENCE (or CLFLUSH)

    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 5), 1, &OpDispatchBuilder::UnimplementedOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 6), 1, &OpDispatchBuilder::UnimplementedOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_66, 6), 1, &OpDispatchBuilder::CLWB},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_66, 7), 1, &OpDispatchBuilder::CLFLUSHOPT},

    // GROUP 16
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 0), 1, &OpDispatchBuilder::Prefetch<false, true, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 1), 1, &OpDispatchBuilder::Prefetch<false, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 2), 1, &OpDispatchBuilder::Prefetch<false, false, 2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 3), 1, &OpDispatchBuilder::Prefetch<false, false, 3>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 4), 4, &OpDispatchBuilder::NOPOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 0), 1, &OpDispatchBuilder::Prefetch<false, true, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 1), 1, &OpDispatchBuilder::Prefetch<false, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 2), 1, &OpDispatchBuilder::Prefetch<false, false, 2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 3), 1, &OpDispatchBuilder::Prefetch<false, false, 3>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 4), 4, &OpDispatchBuilder::NOPOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 0), 1, &OpDispatchBuilder::Prefetch<false, true, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 1), 1, &OpDispatchBuilder::Prefetch<false, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 2), 1, &OpDispatchBuilder::Prefetch<false, false, 2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 3), 1, &OpDispatchBuilder::Prefetch<false, false, 3>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 4), 4, &OpDispatchBuilder::NOPOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 0), 1, &OpDispatchBuilder::Prefetch<false, true, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 1), 1, &OpDispatchBuilder::Prefetch<false, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 2), 1, &OpDispatchBuilder::Prefetch<false, false, 2>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 3), 1, &OpDispatchBuilder::Prefetch<false, false, 3>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 4), 4, &OpDispatchBuilder::NOPOp},

    // GROUP P
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 0), 1, &OpDispatchBuilder::Prefetch<false, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 1), 1, &OpDispatchBuilder::Prefetch<true, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 2), 1, &OpDispatchBuilder::Prefetch<true, false, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 3), 5, &OpDispatchBuilder::NOPOp},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F3, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_66, 0), 8, &OpDispatchBuilder::NOPOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F2, 0), 8, &OpDispatchBuilder::NOPOp},
  };

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> SecondaryExtensionOpTable_64[] = {
    // GROUP 15
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 0), 1, &OpDispatchBuilder::ReadSegmentReg<OpDispatchBuilder::Segment::FS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 1), 1, &OpDispatchBuilder::ReadSegmentReg<OpDispatchBuilder::Segment::GS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 2), 1, &OpDispatchBuilder::WriteSegmentReg<OpDispatchBuilder::Segment::FS>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 3), 1, &OpDispatchBuilder::WriteSegmentReg<OpDispatchBuilder::Segment::GS>},
  };

#undef OPD

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> SecondaryModRMExtensionOpTable[] = {
    // REG /2
    {((1 << 3) | 0), 1, &OpDispatchBuilder::XGetBVOp},

    // REG /7
    {((3 << 3) | 1), 1, &OpDispatchBuilder::RDTSCPOp},

  };
// Top bit indicating if it needs to be repeated with {0x40, 0x80} or'd in
// All OPDReg versions need it
#define OPDReg(op, reg) ((1 << 15) | ((op - 0xD8) << 8) | (reg << 3))
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> X87F64OpTable[] = {
    {OPDReg(0xD8, 0) | 0x00, 8, &OpDispatchBuilder::FADDF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 1) | 0x00, 8, &OpDispatchBuilder::FMULF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 2) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xD8, 3) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xD8, 4) | 0x00, 8, &OpDispatchBuilder::FSUBF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 5) | 0x00, 8, &OpDispatchBuilder::FSUBF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 6) | 0x00, 8, &OpDispatchBuilder::FDIVF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD8, 7) | 0x00, 8, &OpDispatchBuilder::FDIVF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xD8, 0xC0), 8, &OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xC8), 8, &OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xD0), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xD8, 0xD8), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xD8, 0xE0), 8, &OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xE8), 8, &OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF0), 8, &OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
      {OPD(0xD8, 0xF8), 8, &OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xD9, 0) | 0x00, 8, &OpDispatchBuilder::FLDF64<32>},

    // 1 = Invalid

    {OPDReg(0xD9, 2) | 0x00, 8, &OpDispatchBuilder::FSTF64<32>},

    {OPDReg(0xD9, 3) | 0x00, 8, &OpDispatchBuilder::FSTF64<32>},

    {OPDReg(0xD9, 4) | 0x00, 8, &OpDispatchBuilder::X87LDENVF64},

    {OPDReg(0xD9, 5) | 0x00, 8, &OpDispatchBuilder::X87FLDCWF64},

    {OPDReg(0xD9, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSTENV},

    {OPDReg(0xD9, 7) | 0x00, 8, &OpDispatchBuilder::X87FSTCW},

      {OPD(0xD9, 0xC0), 8, &OpDispatchBuilder::FLDF64<80>},
      {OPD(0xD9, 0xC8), 8, &OpDispatchBuilder::FXCH},
      {OPD(0xD9, 0xD0), 1, &OpDispatchBuilder::NOPOp}, // FNOP
      // D1 = Invalid
      // D8 = Invalid
      {OPD(0xD9, 0xE0), 1, &OpDispatchBuilder::FCHSF64},
      {OPD(0xD9, 0xE1), 1, &OpDispatchBuilder::FABSF64},
      // E2 = Invalid
      {OPD(0xD9, 0xE4), 1, &OpDispatchBuilder::FTSTF64},
      {OPD(0xD9, 0xE5), 1, &OpDispatchBuilder::X87FXAMF64},
      // E6 = Invalid
      {OPD(0xD9, 0xE8), 1, &OpDispatchBuilder::FLDF64_Const<0x3FF0000000000000>}, // 1.0
      {OPD(0xD9, 0xE9), 1, &OpDispatchBuilder::FLDF64_Const<0x400A934F0979A372>}, // log2l(10)
      {OPD(0xD9, 0xEA), 1, &OpDispatchBuilder::FLDF64_Const<0x3FF71547652B82FE>}, // log2l(e)
      {OPD(0xD9, 0xEB), 1, &OpDispatchBuilder::FLDF64_Const<0x400921FB54442D18>}, // pi
      {OPD(0xD9, 0xEC), 1, &OpDispatchBuilder::FLDF64_Const<0x3FD34413509F79FF>}, // log10l(2)
      {OPD(0xD9, 0xED), 1, &OpDispatchBuilder::FLDF64_Const<0x3FE62E42FEFA39EF>}, // log(2)
      {OPD(0xD9, 0xEE), 1, &OpDispatchBuilder::FLDF64_Const<0>}, // 0.0

      // EF = Invalid
      {OPD(0xD9, 0xF0), 1, &OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64F2XM1>},
      {OPD(0xD9, 0xF1), 1, &OpDispatchBuilder::X87FYL2XF64},
      {OPD(0xD9, 0xF2), 1, &OpDispatchBuilder::X87TANF64},
      {OPD(0xD9, 0xF3), 1, &OpDispatchBuilder::X87ATANF64},
      {OPD(0xD9, 0xF4), 1, &OpDispatchBuilder::FXTRACTF64},
      {OPD(0xD9, 0xF5), 1, &OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64FPREM1>},
      {OPD(0xD9, 0xF6), 1, &OpDispatchBuilder::X87ModifySTP<false>},
      {OPD(0xD9, 0xF7), 1, &OpDispatchBuilder::X87ModifySTP<true>},
      {OPD(0xD9, 0xF8), 1, &OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64FPREM>},
      {OPD(0xD9, 0xF9), 1, &OpDispatchBuilder::X87FYL2XF64},
      {OPD(0xD9, 0xFA), 1, &OpDispatchBuilder::FSQRTF64},
      {OPD(0xD9, 0xFB), 1, &OpDispatchBuilder::X87SinCosF64},
      {OPD(0xD9, 0xFC), 1, &OpDispatchBuilder::FRNDINTF64},
      {OPD(0xD9, 0xFD), 1, &OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64SCALE>},
      {OPD(0xD9, 0xFE), 1, &OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64SIN>},
      {OPD(0xD9, 0xFF), 1, &OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64COS>},

    {OPDReg(0xDA, 0) | 0x00, 8, &OpDispatchBuilder::FADDF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 1) | 0x00, 8, &OpDispatchBuilder::FMULF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 2) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDA, 3) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDA, 4) | 0x00, 8, &OpDispatchBuilder::FSUBF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 5) | 0x00, 8, &OpDispatchBuilder::FSUBF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 6) | 0x00, 8, &OpDispatchBuilder::FDIVF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDA, 7) | 0x00, 8, &OpDispatchBuilder::FDIVF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDA, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDA, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
      // E0 = Invalid
      // E8 = Invalid
      {OPD(0xDA, 0xE9), 1, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      // EA = Invalid
      // F0 = Invalid
      // F8 = Invalid

    {OPDReg(0xDB, 0) | 0x00, 8, &OpDispatchBuilder::FILDF64},

    {OPDReg(0xDB, 1) | 0x00, 8, &OpDispatchBuilder::FISTF64<true>},

    {OPDReg(0xDB, 2) | 0x00, 8, &OpDispatchBuilder::FISTF64<false>},

    {OPDReg(0xDB, 3) | 0x00, 8, &OpDispatchBuilder::FISTF64<false>},

    // 4 = Invalid

    {OPDReg(0xDB, 5) | 0x00, 8, &OpDispatchBuilder::FLDF64<80>},

    // 6 = Invalid

    {OPDReg(0xDB, 7) | 0x00, 8, &OpDispatchBuilder::FSTF64<80>},


      {OPD(0xDB, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
      {OPD(0xDB, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
      // E0 = Invalid
      {OPD(0xDB, 0xE2), 1, &OpDispatchBuilder::NOPOp}, // FNCLEX
      {OPD(0xDB, 0xE3), 1, &OpDispatchBuilder::FNINITF64},
      // E4 = Invalid
      {OPD(0xDB, 0xE8), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
      {OPD(0xDB, 0xF0), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},

      // F8 = Invalid

    {OPDReg(0xDC, 0) | 0x00, 8, &OpDispatchBuilder::FADDF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 1) | 0x00, 8, &OpDispatchBuilder::FMULF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 2) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDC, 3) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDC, 4) | 0x00, 8, &OpDispatchBuilder::FSUBF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 5) | 0x00, 8, &OpDispatchBuilder::FSUBF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 6) | 0x00, 8, &OpDispatchBuilder::FDIVF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDC, 7) | 0x00, 8, &OpDispatchBuilder::FDIVF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDC, 0xC0), 8, &OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xC8), 8, &OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE0), 8, &OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xE8), 8, &OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF0), 8, &OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDC, 0xF8), 8, &OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDD, 0) | 0x00, 8, &OpDispatchBuilder::FLDF64<64>},

    {OPDReg(0xDD, 1) | 0x00, 8, &OpDispatchBuilder::FISTF64<true>},

    {OPDReg(0xDD, 2) | 0x00, 8, &OpDispatchBuilder::FSTF64<64>},

    {OPDReg(0xDD, 3) | 0x00, 8, &OpDispatchBuilder::FSTF64<64>},

    {OPDReg(0xDD, 4) | 0x00, 8, &OpDispatchBuilder::X87FRSTORF64},

    // 5 = Invalid
    {OPDReg(0xDD, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSAVEF64},

    {OPDReg(0xDD, 7) | 0x00, 8, &OpDispatchBuilder::X87FNSTSW},

      {OPD(0xDD, 0xC0), 8, &OpDispatchBuilder::X87FFREE},
      {OPD(0xDD, 0xD0), 8, &OpDispatchBuilder::FST}, //register-register from regular X87
      {OPD(0xDD, 0xD8), 8, &OpDispatchBuilder::FST}, //^

      {OPD(0xDD, 0xE0), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
      {OPD(0xDD, 0xE8), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 0) | 0x00, 8, &OpDispatchBuilder::FADDF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 1) | 0x00, 8, &OpDispatchBuilder::FMULF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 2) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 3) | 0x00, 8, &OpDispatchBuilder::FCOMIF64<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

    {OPDReg(0xDE, 4) | 0x00, 8, &OpDispatchBuilder::FSUBF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 5) | 0x00, 8, &OpDispatchBuilder::FSUBF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 6) | 0x00, 8, &OpDispatchBuilder::FDIVF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

    {OPDReg(0xDE, 7) | 0x00, 8, &OpDispatchBuilder::FDIVF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

      {OPD(0xDE, 0xC0), 8, &OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xC8), 8, &OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xD9), 1, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
      {OPD(0xDE, 0xE0), 8, &OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xE8), 8, &OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF0), 8, &OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>},
      {OPD(0xDE, 0xF8), 8, &OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>},

    {OPDReg(0xDF, 0) | 0x00, 8, &OpDispatchBuilder::FILDF64},

    {OPDReg(0xDF, 1) | 0x00, 8, &OpDispatchBuilder::FISTF64<true>},

    {OPDReg(0xDF, 2) | 0x00, 8, &OpDispatchBuilder::FISTF64<false>},

    {OPDReg(0xDF, 3) | 0x00, 8, &OpDispatchBuilder::FISTF64<false>},

    {OPDReg(0xDF, 4) | 0x00, 8, &OpDispatchBuilder::FBLDF64},

    {OPDReg(0xDF, 5) | 0x00, 8, &OpDispatchBuilder::FILDF64},

    {OPDReg(0xDF, 6) | 0x00, 8, &OpDispatchBuilder::FBSTPF64},

    {OPDReg(0xDF, 7) | 0x00, 8, &OpDispatchBuilder::FISTF64<false>},

      // XXX: This should also set the x87 tag bits to empty
      // We don't support this currently, so just pop the stack
      {OPD(0xDF, 0xC0), 8, &OpDispatchBuilder::X87ModifySTP<true>},

      {OPD(0xDF, 0xE0), 8, &OpDispatchBuilder::X87FNSTSW},
      {OPD(0xDF, 0xE8), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
      {OPD(0xDF, 0xF0), 8, &OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  };

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> X87OpTable[] = {
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

#define OPD(prefix, opcode) (((prefix) << 8) | opcode)
  constexpr uint16_t PF_38_NONE = 0;
  constexpr uint16_t PF_38_66   = (1U << 0);
  constexpr uint16_t PF_38_F3   = (1U << 2);

  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F38Table[] = {
    {OPD(PF_38_NONE, 0x00), 1, &OpDispatchBuilder::PSHUFBOp},
    {OPD(PF_38_66,   0x00), 1, &OpDispatchBuilder::PSHUFBOp},
    {OPD(PF_38_NONE, 0x01), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 2>},
    {OPD(PF_38_66,   0x01), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 2>},
    {OPD(PF_38_NONE, 0x02), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 4>},
    {OPD(PF_38_66,   0x02), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 4>},
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
    {OPD(PF_38_NONE, 0x1C), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1>},
    {OPD(PF_38_66,   0x1C), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1>},
    {OPD(PF_38_NONE, 0x1D), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2>},
    {OPD(PF_38_66,   0x1D), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2>},
    {OPD(PF_38_NONE, 0x1E), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4>},
    {OPD(PF_38_66,   0x1E), 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4>},
    {OPD(PF_38_66,   0x20), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, true>},
    {OPD(PF_38_66,   0x21), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, true>},
    {OPD(PF_38_66,   0x22), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, true>},
    {OPD(PF_38_66,   0x23), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, true>},
    {OPD(PF_38_66,   0x24), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, true>},
    {OPD(PF_38_66,   0x25), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, true>},
    {OPD(PF_38_66,   0x28), 1, &OpDispatchBuilder::PMULLOp<4, true>},
    {OPD(PF_38_66,   0x29), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 8>},
    {OPD(PF_38_66,   0x2A), 1, &OpDispatchBuilder::MOVVectorNTOp},
    {OPD(PF_38_66,   0x2B), 1, &OpDispatchBuilder::PACKUSOp<4>},
    {OPD(PF_38_66,   0x30), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, false>},
    {OPD(PF_38_66,   0x31), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, false>},
    {OPD(PF_38_66,   0x32), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, false>},
    {OPD(PF_38_66,   0x33), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, false>},
    {OPD(PF_38_66,   0x34), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, false>},
    {OPD(PF_38_66,   0x35), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, false>},
    {OPD(PF_38_66,   0x37), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 8>},
    {OPD(PF_38_66,   0x38), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 1>},
    {OPD(PF_38_66,   0x39), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 4>},
    {OPD(PF_38_66,   0x3A), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 2>},
    {OPD(PF_38_66,   0x3B), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 4>},
    {OPD(PF_38_66,   0x3C), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 1>},
    {OPD(PF_38_66,   0x3D), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 4>},
    {OPD(PF_38_66,   0x3E), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 2>},
    {OPD(PF_38_66,   0x3F), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 4>},
    {OPD(PF_38_66,   0x40), 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VMUL, 4>},
    {OPD(PF_38_66,   0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},

    {OPD(PF_38_NONE, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},
    {OPD(PF_38_66, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},

    {OPD(PF_38_66, 0xF6), 1, &OpDispatchBuilder::ADXOp},
    {OPD(PF_38_F3, 0xF6), 1, &OpDispatchBuilder::ADXOp},
  };

#undef OPD

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66   1
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> H0F3ATable[] = {
    {OPD(0, PF_3A_66,   0x08), 1, &OpDispatchBuilder::VectorRound<4>},
    {OPD(0, PF_3A_66,   0x09), 1, &OpDispatchBuilder::VectorRound<8>},
    {OPD(0, PF_3A_66,   0x0A), 1, &OpDispatchBuilder::InsertScalarRound<4>},
    {OPD(0, PF_3A_66,   0x0B), 1, &OpDispatchBuilder::InsertScalarRound<8>},
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

    {OPD(0, PF_3A_66,   0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
    {OPD(0, PF_3A_66,   0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
    {OPD(0, PF_3A_66,   0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
    {OPD(0, PF_3A_66,   0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

    {OPD(0, PF_3A_NONE, 0xCC), 1, &OpDispatchBuilder::SHA1RNDS4Op},
  };
#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD

  static constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> DDDNowTable[] = {
    {0x0C, 1, &OpDispatchBuilder::PI2FWOp},
    {0x0D, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>},
    {0x1C, 1, &OpDispatchBuilder::PF2IWOp},
    {0x1D, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>},

    {0x86, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4>},
    {0x87, 1, &OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4>},

    {0x8A, 1, &OpDispatchBuilder::PFNACCOp},
    {0x8E, 1, &OpDispatchBuilder::PFPNACCOp},

    {0x90, 1, &OpDispatchBuilder::VPFCMPOp<1>},
    {0x94, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 4>},
    {0x96, 1, &OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRECP, 4>},
    {0x97, 1, &OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRSQRT, 4>},

    {0x9A, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 4>},
    {0x9E, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 4>},

    {0xA0, 1, &OpDispatchBuilder::VPFCMPOp<2>},
    {0xA4, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 4>},
    // Can be treated as a move
    {0xA6, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0xA7, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},

    {0xAA, 1, &OpDispatchBuilder::VectorALUROp<IR::OP_VFSUB, 4>},
    {0xAE, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFADDP, 4>},

    {0xB0, 1, &OpDispatchBuilder::VPFCMPOp<0>},
    {0xB4, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VFMUL, 4>},
    // Can be treated as a move
    {0xB6, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0xB7, 1, &OpDispatchBuilder::PMULHRWOp},

    {0xBB, 1, &OpDispatchBuilder::PSWAPDOp},
    {0xBF, 1, &OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 1>},
  };

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  static constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> BMITable[] = {
    {OPD(2, 0b00, 0xF2), 1, &OpDispatchBuilder::ANDNBMIOp},
    {OPD(2, 0b00, 0xF5), 1, &OpDispatchBuilder::BZHI},
    {OPD(2, 0b10, 0xF5), 1, &OpDispatchBuilder::PEXT},
    {OPD(2, 0b11, 0xF5), 1, &OpDispatchBuilder::PDEP},
    {OPD(2, 0b11, 0xF6), 1, &OpDispatchBuilder::MULX},
    {OPD(2, 0b00, 0xF7), 1, &OpDispatchBuilder::BEXTRBMIOp},
    {OPD(2, 0b01, 0xF7), 1, &OpDispatchBuilder::BMI2Shift},
    {OPD(2, 0b10, 0xF7), 1, &OpDispatchBuilder::BMI2Shift},
    {OPD(2, 0b11, 0xF7), 1, &OpDispatchBuilder::BMI2Shift},

    {OPD(3, 0b11, 0xF0), 1, &OpDispatchBuilder::RORX},
  };
#undef OPD

  #define OPD(group, pp, opcode) (((group - X86Tables::InstType::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
    constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> VEXGroupTable[] = {
      {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b001), 1, &OpDispatchBuilder::BLSRBMIOp},
      {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b010), 1, &OpDispatchBuilder::BLSMSKBMIOp},
      {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b011), 1, &OpDispatchBuilder::BLSIBMIOp},
    };
  #undef OPD

  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> EVEXTable[] = {
    {0x10, 2, &OpDispatchBuilder::UnimplementedOp},
    {0x59, 1, &OpDispatchBuilder::UnimplementedOp},
    {0x7F, 1, &OpDispatchBuilder::UnimplementedOp},
  };

  auto InstallToTable = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LOGMAN_THROW_A_FMT(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
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
        LOGMAN_THROW_A_FMT(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
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
  if (Mode == Context::MODE_64BIT) {
    InstallToTable(FEXCore::X86Tables::SecondInstGroupOps, SecondaryExtensionOpTable_64);
  }

  InstallToTable(FEXCore::X86Tables::SecondModRMTableOps, SecondaryModRMExtensionOpTable);

  FEX_CONFIG_OPT(ReducedPrecision, X87REDUCEDPRECISION);
  if(ReducedPrecision) {
    InstallToX87Table(FEXCore::X86Tables::X87Ops, X87F64OpTable);
  } else {
    InstallToX87Table(FEXCore::X86Tables::X87Ops, X87OpTable);
  }

  InstallToTable(FEXCore::X86Tables::H0F38TableOps, H0F38Table);
  InstallToTable(FEXCore::X86Tables::H0F3ATableOps, H0F3ATable);
  InstallToTable(FEXCore::X86Tables::DDDNowOps, DDDNowTable);
  InstallToTable(FEXCore::X86Tables::VEXTableOps, BMITable);
  InstallToTable(FEXCore::X86Tables::VEXTableGroupOps, VEXGroupTable);
  InstallToTable(FEXCore::X86Tables::EVEXTableOps, EVEXTable);
}

}
