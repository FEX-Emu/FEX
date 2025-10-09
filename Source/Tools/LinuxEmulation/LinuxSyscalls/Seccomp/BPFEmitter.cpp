// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Seccomp/BPFEmitter.h"

#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/Utils/LogManager.h>

#include <linux/bpf_common.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

namespace FEX::HLE {

#define EMIT_INST(x)               \
  do {                             \
    if constexpr (CalculateSize) { \
      OpSize += 4;                 \
    } else {                       \
      x;                           \
    }                              \
  } while (0)

#define RETURN_ERROR(x)                                                                \
  if constexpr (CalculateSize) {                                                       \
    return ~0ULL;                                                                      \
  } else {                                                                             \
    static_assert(x == -EINVAL, "Early return error evaluation only supports EINVAL"); \
    return x;                                                                          \
  }

#define RETURN_SUCCESS()           \
  do {                             \
    if constexpr (CalculateSize) { \
      return OpSize;               \
    } else {                       \
      return 0;                    \
    }                              \
  } while (0)

#define VALIDATE(cond)      \
  do {                      \
    if (!(cond)) {          \
      RETURN_ERROR(-EINVAL) \
    }                       \
  } while (0)

using SizeErrorCheck = decltype([](uint64_t Result) -> bool { return Result == ~0ULL; });
using EmissionErrorCheck = decltype([](uint64_t Result) { return Result != 0; });

// Register selection comes from function signature.
constexpr auto REG_A = ARMEmitter::WReg::w0;
constexpr auto REG_X = ARMEmitter::WReg::w1;
constexpr auto REG_TMP = ARMEmitter::WReg::w2;
constexpr auto REG_TMP2 = ARMEmitter::WReg::w3;
constexpr auto REG_SECCOMP_DATA = ARMEmitter::XReg::x4;

template<bool CalculateSize>
uint64_t BPFEmitter::HandleLoad(uint32_t BPFIP, const sock_filter* Inst) {
  VALIDATE(BPF_SIZE(Inst->code) == BPF_W);
  [[maybe_unused]] size_t OpSize {};

  const auto DestReg = BPF_CLASS(Inst->code) == BPF_LD ? REG_A : REG_X;

  switch (BPF_MODE(Inst->code)) {
  case BPF_IMM: {
    auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
    EMIT_INST(ldr(DestReg, &Const.first->second));
    break;
  }
  case BPF_ABS: {
    // ABS has some restrictions
    // - Must be 4-byte aligned
    // - Must be less than the size of seccomp_data
    const auto Offset = Inst->k;

    // Need to be 4-byte aligned.
    VALIDATE((Offset & 0b11) == 0);
    // Ensure accessing inside of seccomp_data.
    VALIDATE(Offset < sizeof(seccomp_data));

    EMIT_INST(ldr(DestReg, REG_SECCOMP_DATA, Offset));
    break;
  }
  case BPF_MEM:
    // Must be smaller than scratch space size.
    VALIDATE(Inst->k < 16);

    EMIT_INST(ldr(DestReg, REG_SECCOMP_DATA, offsetof(WorkingBuffer, ScratchMemory[Inst->k])));
    break;
  case BPF_LEN:
    // Just returns the length of seccomp_data.
    EMIT_INST(movz(DestReg, sizeof(seccomp_data)));
    break;
  case BPF_IND:
  case BPF_MSH:
  default: RETURN_ERROR(-EINVAL); // Unsupported
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleStore(uint32_t BPFIP, const sock_filter* Inst) {
  VALIDATE(BPF_SIZE(Inst->code) == BPF_W);

  [[maybe_unused]] size_t OpSize {};

  const auto SrcReg = BPF_CLASS(Inst->code) == BPF_LD ? REG_A : REG_X;
  // Must be smaller than scratch space size.
  VALIDATE(Inst->k < 16);

  EMIT_INST(str(SrcReg, REG_SECCOMP_DATA, offsetof(WorkingBuffer, ScratchMemory[Inst->k])));

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleALU(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto SrcType = BPF_SRC(Inst->code);
  const auto Op = BPF_OP(Inst->code);

  switch (Op) {
  case BPF_ADD:
  case BPF_SUB:
  case BPF_MUL:
  case BPF_DIV:
  case BPF_OR:
  case BPF_AND:
  case BPF_LSH:
  case BPF_RSH:
  case BPF_MOD:
  case BPF_XOR: {
    auto SrcReg = REG_X;
    if (SrcType == BPF_K) {
      SrcReg = REG_TMP;
      auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
      EMIT_INST(ldr(SrcReg, &Const.first->second));
    }

    switch (Op) {
    case BPF_ADD: EMIT_INST(add(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_SUB: EMIT_INST(sub(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_MUL: EMIT_INST(mul(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_DIV:
      // Specifically unsigned.
      EMIT_INST(udiv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg));
      break;
    case BPF_OR: EMIT_INST(orr(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_AND: EMIT_INST(and_(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_LSH: EMIT_INST(lslv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_RSH: EMIT_INST(lsrv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    case BPF_MOD:
      // Specifically unsigned.
      EMIT_INST(udiv(ARMEmitter::Size::i32Bit, REG_TMP2, REG_A, SrcReg));
      EMIT_INST(msub(ARMEmitter::Size::i32Bit, REG_A, REG_TMP2, SrcReg, REG_A));
      break;
    case BPF_XOR: EMIT_INST(eor(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg)); break;
    default: RETURN_ERROR(-EINVAL);
    }

    break;
  }
  case BPF_NEG:
    // Only BPF_K supported on NEG.
    VALIDATE(SrcType == BPF_K);

    EMIT_INST(neg(ARMEmitter::Size::i32Bit, REG_A, REG_A));
    break;

  default: RETURN_ERROR(-EINVAL);
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleJmp(uint32_t BPFIP, uint32_t NumInst, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto SrcType = BPF_SRC(Inst->code);
  const auto Op = BPF_OP(Inst->code);

  switch (Op) {
  case BPF_JA: {
    // Only BPF_K supported on JA.
    VALIDATE(SrcType == BPF_K);

    // BPF IP register is effectively only 32-bit. Treat k constant like a signed integer.
    // This allows it to jump anywhere in the program.
    // But! Loops are EXPLICITLY disallowed inside of BPF programs.
    // This is to prevent DOS style attacks through BPF programs.
    uint64_t Target = BPFIP + Inst->k + 1;
    // Must not jump past the end.
    VALIDATE(Target < NumInst);

    JumpLabelIterator TargetLabel {};

    if constexpr (!CalculateSize) {
      TargetLabel = JumpLabels.try_emplace(Target, ARMEmitter::ForwardLabel {}).first;
    }

    EMIT_INST(b(&TargetLabel->second));
    break;
  }
  case BPF_JEQ:
  case BPF_JGT:
  case BPF_JGE:
  case BPF_JSET: {
    auto CompareSrcReg = REG_X;
    if (SrcType == BPF_K) {
      CompareSrcReg = REG_TMP;
      auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
      EMIT_INST(ldr(CompareSrcReg, &Const.first->second));
    }
    uint32_t TargetTrue = BPFIP + Inst->jt + 1;
    uint32_t TargetFalse = BPFIP + Inst->jf + 1;

    // Must not jump past the end.
    VALIDATE(TargetTrue < NumInst && TargetFalse < NumInst);

    ARMEmitter::Condition CompareResultOp;
    if (Op == BPF_JEQ) {
      CompareResultOp = ARMEmitter::Condition::CC_EQ;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg));
    } else if (Op == BPF_JGT) {
      CompareResultOp = ARMEmitter::Condition::CC_HI;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg));
    } else if (Op == BPF_JGE) {
      CompareResultOp = ARMEmitter::Condition::CC_HS;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg));
    } else if (Op == BPF_JSET) {
      CompareResultOp = ARMEmitter::Condition::CC_NE;
      EMIT_INST(tst(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg));
    } else {
      RETURN_ERROR(-EINVAL);
    }

    JumpLabelIterator TargetTrueLabel {};
    JumpLabelIterator TargetFalseLabel {};

    if constexpr (!CalculateSize) {
      TargetTrueLabel = JumpLabels.try_emplace(TargetTrue, ARMEmitter::ForwardLabel {}).first;
      TargetFalseLabel = JumpLabels.try_emplace(TargetFalse, ARMEmitter::ForwardLabel {}).first;
    }

    EMIT_INST(b(CompareResultOp, &TargetTrueLabel->second));
    EMIT_INST(b(&TargetFalseLabel->second));
    break;
  }
  default: RETURN_ERROR(-EINVAL); // Unknown jump type
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleRet(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto RValSrc = BPF_RVAL(Inst->code);
  switch (RValSrc) {
  case BPF_K: {
    auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
    EMIT_INST(ldr(ARMEmitter::WReg::w0, &Const.first->second));
    break;
  }
  case BPF_X: EMIT_INST(mov(ARMEmitter::WReg::w0, REG_X)); break;
  case BPF_A:
    // w0 is already REG_A
    static_assert(REG_A == ARMEmitter::WReg::w0, "This is expected to be the same");
    break;
  default: RETURN_ERROR(-EINVAL);
  }

  EMIT_INST(ret());

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleMisc(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto MiscOp = BPF_MISCOP(Inst->code);
  switch (MiscOp) {
  case BPF_TAX: EMIT_INST(mov(REG_X, REG_A)); break;
  case BPF_TXA: EMIT_INST(mov(REG_A, REG_X)); break;
  default: RETURN_ERROR(-EINVAL) // Unsupported misc operation.
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize, class Pred>
uint64_t BPFEmitter::HandleEmission(uint32_t flags, const sock_fprog* prog) {
  constexpr Pred PredFunc;
  uint64_t CalculatedSize {};

  for (uint32_t i = 0; i < prog->len; ++i) {
    if constexpr (!CalculateSize) {
      auto jump_label = JumpLabels.find(i);
      if (jump_label != JumpLabels.end()) {
        Bind(&jump_label->second);
      }
    }

    bool HadError {};
    uint64_t Result {};

    const sock_filter* Inst = &prog->filter[i];
    const uint16_t Code = Inst->code;
    const uint16_t Class = BPF_CLASS(Code);
    switch (Class) {
    case BPF_LD:
    case BPF_LDX: {
      Result = HandleLoad<CalculateSize>(i, Inst);
      break;
    }
    case BPF_ST:
    case BPF_STX: {
      Result = HandleStore<CalculateSize>(i, Inst);
      break;
    }
    case BPF_ALU: {
      Result = HandleALU<CalculateSize>(i, Inst);
      break;
    }
    case BPF_JMP: {
      Result = HandleJmp<CalculateSize>(i, prog->len, Inst);
      break;
    }
    case BPF_RET: {
      Result = HandleRet<CalculateSize>(i, Inst);
      break;
    }
    case BPF_MISC: {
      Result = HandleMisc<CalculateSize>(i, Inst);
      break;
    }
    default:
      // We handle all instruction classes.
      FEX_UNREACHABLE;
    }

    HadError = PredFunc(Result);

    if (HadError) {
      if constexpr (!CalculateSize) {
        // Had error, early return and free the memory.
        FEXCore::Allocator::munmap(GetBufferBase(), FuncSize);
      }
      return Result;
    }

    if constexpr (CalculateSize) {
      CalculatedSize += Result;
    }
  }

  if constexpr (CalculateSize) {
    // Add the constant pool size.
    CalculatedSize += ConstPool.size() * 4;

    // Size calculation could have added constants and jump labels. Erase them now.
    ConstPool.clear();
    JumpLabels.clear();

    return CalculatedSize;
  }

  return 0;
}

uint64_t BPFEmitter::JITFilter(uint32_t flags, const sock_fprog* prog) {
  FuncSize = HandleEmission<true, SizeErrorCheck>(flags, prog);

  if (FuncSize == ~0ULL) {
    // Buffer size calculation found invalid code.
    return -EINVAL;
  }

  SetBuffer((uint8_t*)FEXCore::Allocator::mmap(nullptr, FuncSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), FuncSize);

  const auto CodeBegin = GetCursorAddress<uint8_t*>();

  uint64_t Result = HandleEmission<false, EmissionErrorCheck>(flags, prog);

  if (Result != 0) {
    // Had error, early return and free the memory.
    FEXCore::Allocator::munmap(GetBufferBase(), FuncSize);
    return Result;
  }

  const uint64_t CodeOnlySize = GetCursorAddress<uint8_t*>() - CodeBegin;

  // Emit the constant pool.
  Align();
  for (auto& Const : ConstPool) {
    Bind(&Const.second);
    dc32(Const.first);
  }

  ClearICache(CodeBegin, CodeOnlySize);
  ::mprotect(CodeBegin, AllocationSize(), PROT_READ | PROT_EXEC);
  Func = CodeBegin;

  if constexpr (false) {
    // Useful for debugging seccomp filters.
    LogMan::Msg::DFmt("JITFilter: disas 0x{:x},+{}", fmt::ptr(CodeBegin), CodeOnlySize);
  }

  ConstPool.clear();
  JumpLabels.clear();
  return 0;
}


} // namespace FEX::HLE
