// SPDX-License-Identifier: MIT
#include "Interface/Core/Addressing.h"

#include "Interface/IR/IREmitter.h"
#include "FEXCore/Utils/MathUtils.h"
#include "Interface/IR/IR.h"

namespace FEXCore::IR {

Ref LoadEffectiveAddress(IREmitter* IREmit, const AddressMode& A, IR::OpSize GPRSize, bool AddSegmentBase, bool AllowUpperGarbage) {
  Ref Tmp = A.Base;

  if (A.Offset) {
    Tmp = Tmp ? IREmit->Add(GPRSize, Tmp, A.Offset) : IREmit->Constant(A.Offset);
  }

  if (A.Index) {
    if (A.IndexScale != 1) {
      uint32_t Log2 = FEXCore::ilog2(A.IndexScale);

      if (Tmp) {
        Tmp = IREmit->_AddShift(GPRSize, Tmp, A.Index, ShiftType::LSL, Log2);
      } else {
        Tmp = IREmit->_Lshl(GPRSize, A.Index, IREmit->Constant(Log2));
      }
    } else {
      Tmp = Tmp ? IREmit->Add(GPRSize, Tmp, A.Index) : A.Index;
    }
  }

  // For 64-bit AddrSize can be 32-bit or 64-bit
  // For 32-bit AddrSize can be 32-bit or 16-bit
  //
  // If the AddrSize is not the GPRSize then we need to clear the upper bits.
  if ((A.AddrSize < GPRSize) && !AllowUpperGarbage && Tmp) {
    uint32_t Bits = IR::OpSizeAsBits(A.AddrSize);

    if (A.Base || A.Index) {
      Tmp = IREmit->_Bfe(GPRSize, Bits, 0, Tmp);
    } else if (A.Offset) {
      uint64_t X = A.Offset;
      X &= (1ull << Bits) - 1;
      Tmp = IREmit->Constant(X);
    }
  }

  if (A.Segment && AddSegmentBase) {
    Tmp = Tmp ? IREmit->Add(GPRSize, Tmp, A.Segment) : A.Segment;
  }

  return Tmp ?: IREmit->Constant(0);
}

AddressMode SelectAddressMode(IREmitter* IREmit, const AddressMode& A, IR::OpSize GPRSize, bool HostSupportsTSOImm9, bool AtomicTSO,
                              bool Vector, IR::OpSize AccessSize) {
  const auto Is32Bit = GPRSize == OpSize::i32Bit;
  const auto GPRSizeMatchesAddrSize = A.AddrSize == GPRSize;
  const auto OffsetIndexToLargeFor32Bit = Is32Bit && (A.Offset <= -16384 || A.Offset >= 16384);
  if (!GPRSizeMatchesAddrSize || OffsetIndexToLargeFor32Bit) {
    // If address size doesn't match GPR size then no optimizations can occur.
    return {
      .Base = LoadEffectiveAddress(IREmit, A, GPRSize, true),
      .Index = IREmit->Invalid(),
    };
  }

  // Loadstore rules:
  // Non-TSO GPR:
  // * LDR/STR:   [Reg]
  // * LDR/STR:   [Reg + Reg, {Shift <AccessSize>}]
  //   * Can't use with 32-bit
  // * LDR/STR:   [Reg + [0,4095] * <AccessSize>]
  //   * Imm must be smaller than 16k with 32-bit
  // * LDUR/STUR: [Reg + [-256, 255]]
  //
  // TSO GPR:
  // * ARMv8.0:
  //  LDAR/STLR: [Reg]
  // * FEAT_LRCPC:
  //  LDAPR: [Reg]
  // * FEAT_LRCPC2:
  //  LDAPUR/STLUR: [Reg + [-256, 255]]
  //
  // Non-TSO Vector:
  // * LDR/STR: [Reg + [0,4095] * <AccessSize>]
  // * LDUR/STUR: [Reg + [-256,255]]
  //
  // TSO Vector:
  // * ARMv8.0:
  //   Just DMB + previous
  // * FEAT_LRCPC3 (Unsupported by FEXCore currently):
  //   LDAPUR/STLUR: [Reg + [-256,255]]

  const auto AccessSizeAsImm = OpSizeToSize(AccessSize);
  const bool OffsetIsSIMM9 = A.Offset && A.Offset >= -256 && A.Offset <= 255;
  const bool OffsetIsUnsignedScaled = A.Offset > 0 && (A.Offset & (AccessSizeAsImm - 1)) == 0 && (A.Offset / AccessSizeAsImm) <= 4095;

  if ((AtomicTSO && !Vector && HostSupportsTSOImm9 && OffsetIsSIMM9) || (!AtomicTSO && (OffsetIsSIMM9 || OffsetIsUnsignedScaled))) {
    // Peel off the offset
    AddressMode B = A;
    B.Offset = 0;

    return {
      .Base = LoadEffectiveAddress(IREmit, B, GPRSize, true /* AddSegmentBase */, false),
      .Index = IREmit->Constant(A.Offset),
      .IndexType = MemOffsetType::SXTX,
      .IndexScale = 1,
    };
  }

  if (AtomicTSO) {
    // TODO: LRCPC3 support for vector Imm9.
  } else if (!Is32Bit && A.Base && (A.Index || A.Segment) && !A.Offset && (A.IndexScale == 1 || A.IndexScale == AccessSizeAsImm)) {
    AddressMode B = A;

    // ScaledRegisterLoadstore
    if (B.Index && B.Segment) {
      B.Base = IREmit->Add(GPRSize, B.Base, B.Segment);
    } else if (B.Segment) {
      B.Index = B.Segment;
      B.IndexScale = 1;
    }

    return B;
  }

  if (Vector || !AtomicTSO) {
    if ((A.Base || A.Segment) && A.Offset) {
      const bool Const_16K = A.Offset > -16384 && A.Offset < 16384 && GPRSizeMatchesAddrSize && Is32Bit;

      if (!Is32Bit || Const_16K) {
        // Peel off the offset
        AddressMode B = A;
        B.Offset = 0;

        return {
          .Base = LoadEffectiveAddress(IREmit, B, GPRSize, true /* AddSegmentBase */, false),
          .Index = IREmit->Constant(A.Offset),
          .IndexType = MemOffsetType::SXTX,
          .IndexScale = 1,
        };
      }
    }
  }

  // Fallback on software address calculation
  return {
    .Base = LoadEffectiveAddress(IREmit, A, GPRSize, true),
    .Index = IREmit->Invalid(),
  };
}


}; // namespace FEXCore::IR
