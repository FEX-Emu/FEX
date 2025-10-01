// SPDX-License-Identifier: MIT
/* SVE instruction emitters
 * These contain instruction emitters for AArch64 SVE and SVE2 operations.
 *
 * All of these SVE emitters have a `SubRegSize` as their first argument to set the element size on the instruction.
 * Since nearly every SVE instruction is unsized they don't need more than `ZRegister` and `PRegister` arguments.
 *
 * Most predicated instructions take a `PRegister` argument, not explicitly stating if it is merging or zeroing behaviour.
 * This is because the instruction only supports one style.
 * For instructions that take an explicit `PRegisterMerge` or `PRegisterZero`, then this instruction likely
 * supports both so we support both implementations depending on predicate register type.
 *
 * Some instructions take a templated `OpType` to choose between a destructive or constructive version of the instruction.
 *
 * Some instructions support the `i128Bit` SubRegSize, mostly around data movement.
 *
 * There are some SVE load-store helper functions which take a `SVEMemOperand` argument.
 * This helper will select the viable SVE load-store that can work with the provided encapsulated arguments.
 */

#pragma once
#ifndef INCLUDED_BY_EMITTER
#include <CodeEmitter/Emitter.h>
namespace ARMEmitter {
struct EmitterOps : Emitter {
#endif

public:
  // SVE encodings
  void dup(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Index) {
    SVEDupIndexed(size, zn, zd, Index);
  }

  void sel(SubRegSize size, ZRegister zd, PRegister pv, ZRegister zn, ZRegister zm) {
    SVESel(size, zm, pv, zn, zd);
  }
  void mov(SubRegSize size, ZRegister zd, PRegisterMerge pv, ZRegister zn) {
    sel(size, zd, pv, zn, zd);
  }

  void histcnt(SubRegSize size, ZRegister zd, PRegisterZero pv, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "SubRegSize must be 32-bit or 64-bit");
    LOGMAN_THROW_A_FMT(pv <= PReg::p7.Zeroing(), "histcnt can only use p0 to p7");

    uint32_t Op = 0b0100'0101'0010'0000'1100'0000'0000'0000;
    Op |= FEXCore::ToUnderlying(size) << 22;
    Op |= zm.Idx() << 16;
    Op |= pv.Idx() << 10;
    Op |= zn.Idx() << 5;
    Op |= zd.Idx();
    dc32(Op);
  }

  void histseg(ZRegister zd, ZRegister zn, ZRegister zm) {
    uint32_t Op = 0b0100'0101'0010'0000'1010'0000'0000'0000;
    Op |= zm.Idx() << 16;
    Op |= zn.Idx() << 5;
    Op |= zd.Idx();
    dc32(Op);
  }

  void fcmla(SubRegSize size, ZRegister zda, PRegisterMerge pv, ZRegister zn, ZRegister zm, Rotation rot) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT(pv <= PReg::p7.Merging(), "fcmla can only use p0 to p7");

    uint32_t Op = 0b0110'0100'0000'0000'0000'0000'0000'0000;
    Op |= FEXCore::ToUnderlying(size) << 22;
    Op |= zm.Idx() << 16;
    Op |= FEXCore::ToUnderlying(rot) << 13;
    Op |= pv.Idx() << 10;
    Op |= zn.Idx() << 5;
    Op |= zda.Idx();

    dc32(Op);
  }

  void fcadd(SubRegSize size, ZRegister zd, PRegisterMerge pv, ZRegister zn, ZRegister zm, Rotation rot) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT(pv <= PReg::p7.Merging(), "fcadd can only use p0 to p7");
    LOGMAN_THROW_A_FMT(rot == Rotation::ROTATE_90 || rot == Rotation::ROTATE_270, "fcadd rotation may only be 90 or 270 degrees");
    LOGMAN_THROW_A_FMT(zd == zn, "fcadd zd and zn must be the same register");

    const uint32_t ConvertedRotation = rot == Rotation::ROTATE_90 ? 0 : 1;

    uint32_t Op = 0b0110'0100'0000'0000'1000'0000'0000'0000;
    Op |= FEXCore::ToUnderlying(size) << 22;
    Op |= ConvertedRotation << 16;
    Op |= pv.Idx() << 10;
    Op |= zm.Idx() << 5;
    Op |= zd.Idx();

    dc32(Op);
  }

  // SVE integer add/subtract vectors (unpredicated)
  void add(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b000, size, zm, zn, zd);
  }
  void sub(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b001, size, zm, zn, zd);
  }
  void sqadd(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b100, size, zm, zn, zd);
  }
  void uqadd(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b101, size, zm, zn, zd);
  }
  void sqsub(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b110, size, zm, zn, zd);
  }
  void uqsub(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEIntegerAddSubUnpredicated(0b111, size, zm, zn, zd);
  }

  // SVE address generation
  void adr(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, SVEModType mod = SVEModType::MOD_NONE, uint32_t scale = 0) {
    SVEAddressGeneration(size, zd, zn, zm, mod, scale);
  }

  // SVE table lookup (three sources)
  void tbl(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVETableLookup(0b100, size, zm, zn, zd);
  }
  void tbl(SubRegSize size, ZRegister zd, ZRegister zn1, ZRegister zn2, ZRegister zm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zn1, zn2), "TBL zn1 and zn2 must be sequential");
    SVETableLookup(0b010, size, zm, zn1, zd);
  }
  void tbx(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVETableLookup(0b011, size, zm, zn, zd);
  }

  // SVE permute vector elements
  void zip1(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b000, size, zm, zn, zd);
  }
  void zip2(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b001, size, zm, zn, zd);
  }
  void uzp1(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b010, size, zm, zn, zd);
  }
  void uzp2(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b011, size, zm, zn, zd);
  }
  void trn1(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b100, size, zm, zn, zd);
  }
  void trn2(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEPermute(0b101, size, zm, zn, zd);
  }

  // SVE integer compare with unsigned immediate
  void cmphi(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, uint32_t imm) {
    SVEIntegerCompareImm(0, 1, imm, size, pg, zn, pd);
  }
  void cmphs(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, uint32_t imm) {
    SVEIntegerCompareImm(0, 0, imm, size, pg, zn, pd);
  }
  void cmplo(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, uint32_t imm) {
    SVEIntegerCompareImm(1, 0, imm, size, pg, zn, pd);
  }
  void cmpls(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, uint32_t imm) {
    SVEIntegerCompareImm(1, 1, imm, size, pg, zn, pd);
  }

  // SVE integer compare with signed immediate
  void cmpeq(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(1, 0, 0, imm, size, pg, zn, pd);
  }
  void cmpgt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(0, 0, 1, imm, size, pg, zn, pd);
  }
  void cmpge(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(0, 0, 0, imm, size, pg, zn, pd);
  }
  void cmplt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(0, 1, 0, imm, size, pg, zn, pd);
  }
  void cmple(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(0, 1, 1, imm, size, pg, zn, pd);
  }
  void cmpne(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, int32_t imm) {
    SVEIntegerCompareSignedImm(1, 0, 1, imm, size, pg, zn, pd);
  }

  // SVE predicate logical operations
  void and_(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 0, 0, 0, pm, pg, pn, pd);
  }
  void ands(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 1, 0, 0, pm, pg, pn, pd);
  }

  void mov(PRegister pd, PRegisterMerge pg, PRegister pn) {
    SVEPredicateLogical(0, 0, 1, 1, pd, pg, pn, pd);
  }
  void mov(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPredicateLogical(0, 0, 0, 0, pn, pg, pn, pd);
  }

  void movs(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPredicateLogical(0, 1, 0, 0, pn, pg, pn, pd);
  }
  void bic(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 0, 0, 1, pm, pg, pn, pd);
  }
  void bics(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 1, 0, 1, pm, pg, pn, pd);
  }

  void eor(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 0, 1, 0, pm, pg, pn, pd);
  }
  void eors(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 1, 1, 0, pm, pg, pn, pd);
  }

  void not_(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPredicateLogical(0, 0, 1, 0, pg, pg, pn, pd);
  }
  void sel(PRegister pd, PRegister pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(0, 0, 1, 1, pm, pg, pn, pd);
  }
  void orr(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 0, 0, 0, pm, pg, pn, pd);
  }
  void mov(PRegister pd, PRegister pn) {
    SVEPredicateLogical(1, 0, 0, 0, pn, pn, pn, pd);
  }
  void orn(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 0, 0, 1, pm, pg, pn, pd);
  }
  void nor(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 0, 1, 0, pm, pg, pn, pd);
  }
  void nand(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 0, 1, 1, pm, pg, pn, pd);
  }
  void orrs(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 1, 0, 0, pm, pg, pn, pd);
  }
  void movs(PRegister pd, PRegister pn) {
    SVEPredicateLogical(1, 1, 0, 0, pn, pn, pn, pd);
  }
  void orns(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 1, 0, 1, pm, pg, pn, pd);
  }
  void nors(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 1, 1, 0, pm, pg, pn, pd);
  }
  void nands(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPredicateLogical(1, 1, 1, 1, pm, pg, pn, pd);
  }

  // SVE broadcast predicate element
  // XXX:

  // SVE integer clamp
  // XXX:

  // SVE2 character match
  void match(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVECharacterMatch(0, size, pd, pg, zn, zm);
  }
  void nmatch(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVECharacterMatch(1, size, pd, pg, zn, zm);
  }

  // SVE floating-point convert precision odd elements
  void fcvtxnt(ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatConvertOdd(0b00, 0b10, pg, zn, zd);
  }
  ///< Size is destination size
  void fcvtnt(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    const auto ConvertedDestSize = size == SubRegSize::i16Bit ? 0b00 : size == SubRegSize::i32Bit ? 0b10 : 0b00;

    const auto ConvertedSrcSize = size == SubRegSize::i16Bit ? 0b10 : size == SubRegSize::i32Bit ? 0b11 : 0b00;

    SVEFloatConvertOdd(ConvertedSrcSize, ConvertedDestSize, pg, zn, zd);
  }

  ///< Size is destination size
  void fcvtlt(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit || size == SubRegSize::i32Bit, "Unsupported size in {}", __func__);

    const auto ConvertedDestSize = size == SubRegSize::i32Bit ? 0b01 : size == SubRegSize::i64Bit ? 0b11 : 0b00;

    const auto ConvertedSrcSize = size == SubRegSize::i32Bit ? 0b10 : size == SubRegSize::i64Bit ? 0b11 : 0b00;

    SVEFloatConvertOdd(ConvertedSrcSize, ConvertedDestSize, pg, zn, zd);
  }

  // XXX: BFCVTNT

  // SVE2 floating-point pairwise operations
  void faddp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatPairwiseArithmetic(0b000, size, pg, zd, zn, zm);
  }
  void fmaxnmp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatPairwiseArithmetic(0b100, size, pg, zd, zn, zm);
  }
  void fminnmp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatPairwiseArithmetic(0b101, size, pg, zd, zn, zm);
  }
  void fmaxp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatPairwiseArithmetic(0b110, size, pg, zd, zn, zm);
  }
  void fminp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatPairwiseArithmetic(0b111, size, pg, zd, zn, zm);
  }

  // SVE floating-point multiply-add (indexed)
  void fmla(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddIndexed(0, size, zda, zn, zm, index);
  }
  void fmls(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddIndexed(1, size, zda, zn, zm, index);
  }

  // SVE floating-point complex multiply-add (indexed)
  void fcmla(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index, Rotation rot) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit, "SubRegSize must be 16-bit or 32-bit");

    // 16 -> 32, 32 -> 64, since fcmla (indexed)'s restrictions and encodings
    // are essentially as if 16-bit were 32-bit and 32-bit were 64-bit.
    const auto DoubledSize = static_cast<SubRegSize>(FEXCore::ToUnderlying(size) + 1);

    SVEFPMultiplyAddIndexed(0b100 | FEXCore::ToUnderlying(rot), DoubledSize, zda, zn, zm, index);
  }

  // SVE floating-point multiply (indexed)
  void fmul(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddIndexed(0b1000, size, zd, zn, zm, index);
  }

  // SVE floating point matrix multiply accumulate
  // XXX: BFMMLA
  void fmmla(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMatrixMultiplyAccumulate(size, zda, zn, zm);
  }

  // SVE floating-point compare vectors
  void fcmeq(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(0, 1, 0, size, zm, pg, zn, pd);
  }
  void fcmgt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(0, 0, 1, size, zm, pg, zn, pd);
  }
  void fcmge(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(0, 0, 0, size, zm, pg, zn, pd);
  }
  void fcmne(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(0, 1, 1, size, zm, pg, zn, pd);
  }
  void fcmuo(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(1, 0, 0, size, zm, pg, zn, pd);
  }
  void facge(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(1, 0, 1, size, zm, pg, zn, pd);
  }
  void facgt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEFloatCompareVector(1, 1, 1, size, zm, pg, zn, pd);
  }
  void facle(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zm, ZRegister zn) {
    facge(size, pd, pg, zn, zm);
  }
  void faclt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zm, ZRegister zn) {
    facgt(size, pd, pg, zn, zm);
  }

  // SVE floating-point arithmetic (unpredicated)
  void fadd(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b000, size, zm, zn, zd);
  }
  void fsub(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b001, size, zm, zn, zd);
  }
  void fmul(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b010, size, zm, zn, zd);
  }
  void ftsmul(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b011, size, zm, zn, zd);
  }
  void frecps(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b110, size, zm, zn, zd);
  }
  void frsqrts(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticUnpredicated(0b111, size, zm, zn, zd);
  }

  // SVE floating-point recursive reduction
  void faddv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEFPRecursiveReduction(0b000, size, vd, pg, zn);
  }
  void fmaxnmv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEFPRecursiveReduction(0b100, size, vd, pg, zn);
  }
  void fminnmv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEFPRecursiveReduction(0b101, size, vd, pg, zn);
  }
  void fmaxv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEFPRecursiveReduction(0b110, size, vd, pg, zn);
  }
  void fminv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEFPRecursiveReduction(0b111, size, vd, pg, zn);
  }

  // SVE integer Multiply-Add - Predicated
  // SVE integer multiply-accumulate writing addend (predicated)
  void mla(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMultiplyAddSubPredicated(0b0, 0b0, size, zda, pg, zn, zm);
  }
  void mls(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMultiplyAddSubPredicated(0b0, 0b1, size, zda, pg, zn, zm);
  }

  // SVE integer multiply-add writing multiplicand (predicated)
  void mad(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEIntegerMultiplyAddSubPredicated(0b1, 0b0, size, zdn, pg, za, zm);
  }
  void msb(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEIntegerMultiplyAddSubPredicated(0b1, 0b1, size, zdn, pg, za, zm);
  }

  // SVE Integer Binary Arithmetic - Predicated
  // SVE integer add/subtract vectors (predicated)
  void add(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEAddSubVectorsPredicated(0b000, size, zd, pg, zn, zm);
  }
  void sub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEAddSubVectorsPredicated(0b001, size, zd, pg, zn, zm);
  }
  void subr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEAddSubVectorsPredicated(0b011, size, zd, pg, zn, zm);
  }

  // SVE integer min/max/difference (predicated)
  void smax(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b00, 0, size, pg, zdn, zm, zd);
  }
  void umax(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b00, 1, size, pg, zdn, zm, zd);
  }
  void smin(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b01, 0, size, pg, zdn, zm, zd);
  }
  void umin(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b01, 1, size, pg, zdn, zm, zd);
  }
  void sabd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b10, 0, size, pg, zdn, zm, zd);
  }
  void uabd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEIntegerMinMaxDifferencePredicated(0b10, 1, size, pg, zdn, zm, zd);
  }

  // SVE integer multiply vectors (predicated)
  void mul(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b0, 0b00, size, zd, pg, zn, zm);
  }
  void smulh(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b0, 0b10, size, zd, pg, zn, zm);
  }
  void umulh(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b0, 0b11, size, zd, pg, zn, zm);
  }

  // SVE integer divide vectors (predicated)
  void sdiv(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b1, 0b00, size, zd, pg, zn, zm);
  }
  void udiv(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b1, 0b01, size, zd, pg, zn, zm);
  }
  void sdivr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b1, 0b10, size, zd, pg, zn, zm);
  }
  void udivr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerMulDivVectorsPredicated(0b1, 0b11, size, zd, pg, zn, zm);
  }

  // SVE bitwise logical operations (predicated)
  void orr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEBitwiseLogicalPredicated(0b000, size, pg, zdn, zm, zd);
  }
  void eor(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEBitwiseLogicalPredicated(0b001, size, pg, zdn, zm, zd);
  }
  void and_(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEBitwiseLogicalPredicated(0b010, size, pg, zdn, zm, zd);
  }
  void bic(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, ZRegister zm) {
    SVEBitwiseLogicalPredicated(0b011, size, pg, zdn, zm, zd);
  }

  // SVE Integer Reduction
  // SVE integer add reduction (predicated)
  void saddv(SubRegSize size, DRegister vd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit, "saddv may only use 8-bit, "
                                                                                                              "16-bit, or 32-bit "
                                                                                                              "elements.");
    constexpr uint32_t Op = 0b0000'0100'0000'0000'0010'0000'0000'0000;
    SVEIntegerReductionOperation(Op, 0b00, size, vd, pg, zn);
  }
  void uaddv(SubRegSize size, DRegister vd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit, "uaddv may only use 8-bit, "
                                                                                                              "16-bit, or 32-bit "
                                                                                                              "elements.");
    constexpr uint32_t Op = 0b0000'0100'0000'0000'0010'0000'0000'0000;
    SVEIntegerReductionOperation(Op, 0b01, size, vd, pg, zn);
  }

  // SVE integer min/max reduction (predicated)
  void smaxv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerReductionOperation(Op, 0b00, size, vd, pg, zn);
  }
  void umaxv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerReductionOperation(Op, 0b01, size, vd, pg, zn);
  }
  void sminv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerReductionOperation(Op, 0b10, size, vd, pg, zn);
  }
  void uminv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerReductionOperation(Op, 0b11, size, vd, pg, zn);
  }

  // SVE constructive prefix (predicated)
  template<typename T>
  requires (std::is_same_v<PRegisterZero, T> || std::is_same_v<PRegisterMerge, T>)
  void movprfx(SubRegSize size, ZRegister zd, T pg, ZRegister zn) {
    constexpr uint32_t M = std::is_same_v<PRegisterMerge, T> ? 1 : 0;
    SVEConstructivePrefixPredicated(0b00, M, size, pg, zn, zd);
  }

  // SVE bitwise logical reduction (predicated)
  void orv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0001'1000'0010'0000'0000'0000;
    SVEIntegerReductionOperation(Op, 0b00, size, vd, pg, zn);
  }
  void eorv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0001'1000'0010'0000'0000'0000;
    SVEIntegerReductionOperation(Op, 0b01, size, vd, pg, zn);
  }
  void andv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0001'1000'0010'0000'0000'0000;
    SVEIntegerReductionOperation(Op, 0b10, size, vd, pg, zn);
  }

  // SVE Bitwise Shift - Predicated
  // SVE bitwise shift by immediate (predicated)
  void asr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b00, 0, 0, pg, zd, zdn, Shift);
  }
  void lsr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b00, 0, 1, pg, zd, zdn, Shift);
  }
  void lsl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b00, 1, 1, pg, zd, zdn, Shift);
  }
  void asrd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b01, 0, 0, pg, zd, zdn, Shift);
  }
  void sqshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b01, 1, 0, pg, zd, zdn, Shift);
  }
  void uqshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b01, 1, 1, pg, zd, zdn, Shift);
  }
  void srshr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b11, 0, 0, pg, zd, zdn, Shift);
  }
  void urshr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b11, 0, 1, pg, zd, zdn, Shift);
  }
  void sqshlu(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zdn, uint32_t Shift) {
    SVEBitWiseShiftImmediatePred(size, 0b11, 1, 1, pg, zd, zdn, Shift);
  }

  // SVE bitwise shift by vector (predicated)
  void asr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(0, 0, 0, size, pg, zd, zn, zm);
  }
  void lsr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(0, 0, 1, size, pg, zd, zn, zm);
  }
  void lsl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(0, 1, 1, size, pg, zd, zn, zm);
  }
  void asrr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(1, 0, 0, size, pg, zd, zn, zm);
  }
  void lsrr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(1, 0, 1, size, pg, zd, zn, zm);
  }
  void lslr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftbyVector(1, 1, 1, size, pg, zd, zn, zm);
  }

  // SVE bitwise shift by wide elements (predicated)
  void asr_wide(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementPredicated(size, 0b000, zd, pg, zn, zm);
  }
  void lsr_wide(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementPredicated(size, 0b001, zd, pg, zn, zm);
  }
  void lsl_wide(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementPredicated(size, 0b011, zd, pg, zn, zm);
  }

  // SVE Integer Unary Arithmetic - Predicated
  // SVE integer unary operations (predicated)
  void sxtb(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b000, size, pg, zn, zd);
  }
  void uxtb(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b001, size, pg, zn, zd);
  }
  void sxth(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b010, size, pg, zn, zd);
  }
  void uxth(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b011, size, pg, zn, zd);
  }
  void sxtw(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b100, size, pg, zn, zd);
  }
  void uxtw(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEIntegerUnaryPredicated(0b10, 0b101, size, pg, zn, zd);
  }
  void abs(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b10, 0b110, size, pg, zn, zd);
  }
  void neg(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b10, 0b111, size, pg, zn, zd);
  }

  // SVE bitwise unary operations (predicated)
  void cls(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b11, 0b000, size, pg, zn, zd);
  }
  void clz(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b11, 0b001, size, pg, zn, zd);
  }
  void cnt(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b11, 0b010, size, pg, zn, zd);
  }
  void cnot(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b11, 0b011, size, pg, zn, zd);
  }
  void fabs(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Invalid size");
    SVEIntegerUnaryPredicated(0b11, 0b100, size, pg, zn, zd);
  }
  void fneg(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Invalid size");
    SVEIntegerUnaryPredicated(0b11, 0b101, size, pg, zn, zd);
  }
  void not_(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEIntegerUnaryPredicated(0b11, 0b110, size, pg, zn, zd);
  }

  // SVE Bitwise Logical - Unpredicated
  // SVE bitwise logical operations (unpredicated)
  void and_(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseLogicalUnpredicated(0b00, zm, zn, zd);
  }
  void orr(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseLogicalUnpredicated(0b01, zm, zn, zd);
  }
  void mov(ZRegister zd, ZRegister zn) {
    SVEBitwiseLogicalUnpredicated(0b01, zn, zn, zd);
  }
  void eor(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseLogicalUnpredicated(0b10, zm, zn, zd);
  }
  void bic(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseLogicalUnpredicated(0b11, zm, zn, zd);
  }

  void xar(SubRegSize size, ZRegister zd, ZRegister zm, uint32_t rotate) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Element size cannot be 128-bit.");

    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, rotate);

    uint32_t Inst = 0b0000'0100'0010'0000'0011'0100'0000'0000;
    Inst |= tszh << 22;
    Inst |= tszl_imm3 << 16;
    Inst |= zm.Idx() << 5;
    Inst |= zd.Idx();
    dc32(Inst);
  }

  // SVE2 bitwise ternary operations
  void eor3(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b00, 0, zm, zk, zd, zdn);
  }
  void bsl(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b00, 1, zm, zk, zd, zdn);
  }
  void bcax(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b01, 0, zm, zk, zd, zdn);
  }
  void bsl1n(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b01, 1, zm, zk, zd, zdn);
  }
  void bsl2n(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b10, 1, zm, zk, zd, zdn);
  }
  void nbsl(ZRegister zd, ZRegister zdn, ZRegister zm, ZRegister zk) {
    SVE2BitwiseTernary(0b11, 1, zm, zk, zd, zdn);
  }

  // SVE Index Generation
  void index(SubRegSize size, ZRegister zd, int32_t initial, int32_t increment) {
    LOGMAN_THROW_A_FMT(initial >= -16 && initial <= 15, "initial value must be within -16-15. initial: {}", initial);
    LOGMAN_THROW_A_FMT(increment >= -16 && increment <= 15, "increment value must be within -16-15. increment: {}", increment);
    SVEIndexGeneration(0b00, size, zd, initial, increment);
  }
  void index(SubRegSize size, ZRegister zd, Register initial, int32_t increment) {
    LOGMAN_THROW_A_FMT(increment >= -16 && increment <= 15, "increment value must be within -16-15. increment: {}", increment);
    SVEIndexGeneration(0b01, size, zd, static_cast<int32_t>(initial.Idx()), increment);
  }
  void index(SubRegSize size, ZRegister zd, int32_t initial, Register increment) {
    LOGMAN_THROW_A_FMT(initial >= -16 && initial <= 15, "initial value must be within -16-15. initial: {}", initial);
    SVEIndexGeneration(0b10, size, zd, initial, static_cast<int32_t>(increment.Idx()));
  }
  void index(SubRegSize size, ZRegister zd, Register initial, Register increment) {
    SVEIndexGeneration(0b11, size, zd, static_cast<int32_t>(initial.Idx()), static_cast<int32_t>(increment.Idx()));
  }

  // SVE Stack Allocation
  // SVE stack frame adjustment
  void addvl(XRegister rd, XRegister rn, int32_t imm) {
    SVEStackFrameOperation(0b00, rd, rn, imm);
  }
  void addpl(XRegister rd, XRegister rn, int32_t imm) {
    SVEStackFrameOperation(0b01, rd, rn, imm);
  }

  // Streaming SVE stack frame adjustment (SME)
  // XXX:

  // SVE stack frame size
  void rdvl(XRegister rd, int32_t imm) {
    // Would-be Rn field is just set to all 1's, which is the same
    // as writing the encoding for the SP into it.
    SVEStackFrameOperation(0b10, rd, XReg::rsp, imm);
  }

  // Streaming SVE stack frame size (SME)
  // XXX:

  // SVE2 Integer Multiply - Unpredicated
  // SVE2 integer multiply vectors (unpredicated)
  void mul(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b00, size, zm, zn, zd);
  }
  void smulh(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b10, size, zm, zn, zd);
  }

  void umulh(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b11, size, zm, zn, zd);
  }

  void pmul(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b01, SubRegSize::i8Bit, zm, zn, zd);
  }

  // SVE2 signed saturating doubling multiply high (unpredicated)
  void sqdmulh(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b100, size, zm, zn, zd);
  }
  void sqrdmulh(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b101, size, zm, zn, zd);
  }

  // SVE Bitwise Shift - Unpredicated
  // SVE bitwise shift by wide elements (unpredicated)
  void asr_wide(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementsUnpredicated(size, 0b00, zd, zn, zm);
  }
  void lsr_wide(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementsUnpredicated(size, 0b01, zd, zn, zm);
  }
  void lsl_wide(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVEBitwiseShiftByWideElementsUnpredicated(size, 0b11, zd, zn, zm);
  }

  // SVE bitwise shift by immediate (unpredicated)
  void asr(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVEBitWiseShiftImmediateUnpred(size, 0b00, zd, zn, shift);
  }
  void lsr(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVEBitWiseShiftImmediateUnpred(size, 0b01, zd, zn, shift);
  }
  void lsl(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVEBitWiseShiftImmediateUnpred(size, 0b11, zd, zn, shift);
  }

  // SVE Integer Misc - Unpredicated
  // SVE floating-point trig select coefficient
  void ftssel(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "ftssel may only use 16/32/64-bit element sizes");
    SVEIntegerMiscUnpredicated(0b00, zm.Idx(), FEXCore::ToUnderlying(size), zd, zn);
  }
  // SVE floating-point exponential accelerator
  void fexpa(SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "fexpa may only use 16/32/64-bit element sizes");
    SVEIntegerMiscUnpredicated(0b10, 0b00000, FEXCore::ToUnderlying(size), zd, zn);
  }
  // SVE constructive prefix (unpredicated)
  void movprfx(ZRegister zd, ZRegister zn) {
    SVEIntegerMiscUnpredicated(0b11, 0b00000, 0b00, zd, zn);
  }

  // SVE Element Count
  // SVE saturating inc/dec vector by element count
  void sqinch(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0000, SubRegSize::i16Bit, zdn, pattern, imm4);
  }
  void uqinch(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0001, SubRegSize::i16Bit, zdn, pattern, imm4);
  }
  void sqdech(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0010, SubRegSize::i16Bit, zdn, pattern, imm4);
  }
  void uqdech(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0011, SubRegSize::i16Bit, zdn, pattern, imm4);
  }
  void sqincw(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0000, SubRegSize::i32Bit, zdn, pattern, imm4);
  }
  void uqincw(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0001, SubRegSize::i32Bit, zdn, pattern, imm4);
  }
  void sqdecw(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0010, SubRegSize::i32Bit, zdn, pattern, imm4);
  }
  void uqdecw(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0011, SubRegSize::i32Bit, zdn, pattern, imm4);
  }
  void sqincd(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0000, SubRegSize::i64Bit, zdn, pattern, imm4);
  }
  void uqincd(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0001, SubRegSize::i64Bit, zdn, pattern, imm4);
  }
  void sqdecd(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0010, SubRegSize::i64Bit, zdn, pattern, imm4);
  }
  void uqdecd(ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    SVEElementCount(0, 0b0011, SubRegSize::i64Bit, zdn, pattern, imm4);
  }

  // SVE element count
  void cntb(XRegister rd, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1000, SubRegSize::i8Bit, ZRegister {rd.Idx()}, pattern, imm);
  }
  void cnth(XRegister rd, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1000, SubRegSize::i16Bit, ZRegister {rd.Idx()}, pattern, imm);
  }
  void cntw(XRegister rd, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1000, SubRegSize::i32Bit, ZRegister {rd.Idx()}, pattern, imm);
  }
  void cntd(XRegister rd, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1000, SubRegSize::i64Bit, ZRegister {rd.Idx()}, pattern, imm);
  }

  // SVE inc/dec vector by element count
  void inch(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0000, SubRegSize::i16Bit, zdn, pattern, imm);
  }
  void dech(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0001, SubRegSize::i16Bit, zdn, pattern, imm);
  }
  void incw(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0000, SubRegSize::i32Bit, zdn, pattern, imm);
  }
  void decw(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0001, SubRegSize::i32Bit, zdn, pattern, imm);
  }
  void incd(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0000, SubRegSize::i64Bit, zdn, pattern, imm);
  }
  void decd(ZRegister zdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b0001, SubRegSize::i64Bit, zdn, pattern, imm);
  }

  // SVE inc/dec register by element count
  void incb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1000, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void decb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1001, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void inch(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1000, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void dech(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1001, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void incw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1000, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void decw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1001, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void incd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1000, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void decd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1001, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }

  // SVE saturating inc/dec register by element count
  void sqincb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1100, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqincb(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1100, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1101, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincb(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1101, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1110, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecb(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1110, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecb(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1111, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecb(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1111, SubRegSize::i8Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }

  void sqinch(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1100, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqinch(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1100, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqinch(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1101, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqinch(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1101, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdech(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1110, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdech(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1110, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdech(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1111, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdech(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1111, SubRegSize::i16Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }

  void sqincw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1100, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqincw(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1100, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1101, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincw(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1101, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1110, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecw(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1110, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecw(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1111, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecw(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1111, SubRegSize::i32Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }

  void sqincd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1100, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqincd(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1100, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1101, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqincd(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1101, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1110, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void sqdecd(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1110, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecd(XRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(1, 0b1111, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }
  void uqdecd(WRegister rdn, PredicatePattern pattern, uint32_t imm) {
    SVEElementCount(0, 0b1111, SubRegSize::i64Bit, ZRegister {rdn.Idx()}, pattern, imm);
  }

  // SVE Bitwise Immediate
  // XXX: DUPM
  // SVE bitwise logical with immediate (unpredicated)
  // XXX:

  // SVE Integer Wide Immediate - Predicated
  void fcpy(SubRegSize size, ZRegister zd, PRegisterMerge pg, float value) {
    SVEBroadcastFloatImmPredicated(size, zd, pg, value);
  }
  void fmov(SubRegSize size, ZRegister zd, PRegisterMerge pg, float value) {
    fcpy(size, zd, pg, value);
  }

  // SVE copy integer immediate (predicated)
  void cpy(SubRegSize size, ZRegister zd, PRegisterZero pg, int32_t imm) {
    SVEBroadcastIntegerImmPredicated(0, size, zd, pg, imm);
  }
  void cpy(SubRegSize size, ZRegister zd, PRegisterMerge pg, int32_t imm) {
    SVEBroadcastIntegerImmPredicated(1, size, zd, pg, imm);
  }
  void mov_imm(SubRegSize size, ZRegister zd, PRegisterZero pg, int32_t imm) {
    cpy(size, zd, pg, imm);
  }
  void mov_imm(SubRegSize size, ZRegister zd, PRegisterMerge pg, int32_t imm) {
    cpy(size, zd, pg, imm);
  }

  // SVE Permute Vector - Unpredicated
  void dup(SubRegSize size, ZRegister zd, Register rn) {
    SVEPermuteUnpredicated(size, 0b00000, zd, ZRegister {rn.Idx()});
  }
  void mov(SubRegSize size, ZRegister zd, Register rn) {
    dup(size, zd, rn);
  }
  void insr(SubRegSize size, ZRegister zdn, Register rm) {
    SVEPermuteUnpredicated(size, 0b00100, zdn, ZRegister {rm.Idx()});
  }
  void insr(SubRegSize size, ZRegister zdn, VRegister vm) {
    SVEPermuteUnpredicated(size, 0b10100, zdn, vm.Z());
  }
  void rev(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVEPermuteUnpredicated(size, 0b11000, zd, zn);
  }

  // SVE unpack vector elements
  void sunpklo(SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEPermuteUnpredicated(size, 0b10000, zd, zn);
  }
  void sunpkhi(SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEPermuteUnpredicated(size, 0b10001, zd, zn);
  }
  void uunpklo(SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEPermuteUnpredicated(size, 0b10010, zd, zn);
  }
  void uunpkhi(SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid subregsize size");
    SVEPermuteUnpredicated(size, 0b10011, zd, zn);
  }

  // SVE Permute Predicate
  void rev(SubRegSize size, PRegister pd, PRegister pn) {
    SVEPermutePredicate(size, 0b10100, 0b0000, 0b0, pd, pn);
  }

  // SVE unpack predicate elements
  void punpklo(PRegister pd, PRegister pn) {
    SVEPermutePredicate(SubRegSize::i8Bit, 0b10000, 0b0000, 0b0, pd, pn);
  }
  void punpkhi(PRegister pd, PRegister pn) {
    SVEPermutePredicate(SubRegSize::i8Bit, 0b10001, 0b0000, 0b0, pd, pn);
  }

  // SVE permute predicate elements
  void zip1(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b0000, 0b0, pd, pn);
  }
  void zip2(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b0010, 0b0, pd, pn);
  }
  void uzp1(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b0100, 0b0, pd, pn);
  }
  void uzp2(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b0110, 0b0, pd, pn);
  }
  void trn1(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b1000, 0b0, pd, pn);
  }
  void trn2(SubRegSize size, PRegister pd, PRegister pn, PRegister pm) {
    SVEPermutePredicate(size, pm.Idx(), 0b1010, 0b0, pd, pn);
  }

  // SVE Permute Vector - Predicated - Base
  // CPY (SIMD&FP scalar)
  void cpy(SubRegSize size, ZRegister zd, PRegisterMerge pg, VRegister vn) {
    SVEPermuteVectorPredicated(0b00000, 0b0, size, zd, pg, ZRegister {vn.Idx()});
  }

  void compact(SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit || size == SubRegSize::i32Bit, "Invalid element size");
    SVEPermuteVectorPredicated(0b00001, 0b0, size, zd, pg, zn);
  }

  // CPY (scalar)
  void cpy(SubRegSize size, ZRegister zd, PRegisterMerge pg, Register rn) {
    SVEPermuteVectorPredicated(0b01000, 0b1, size, zd, pg, ZRegister {rn.Idx()});
  }

  template<OpType optype>
  requires (optype == OpType::Constructive)
  void splice(SubRegSize size, ZRegister zd, PRegister pv, ZRegister zn, ZRegister zn2) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zn, zn2), "zn and zn2 must be sequential registers");
    SVEPermuteVectorPredicated(0b01101, 0b0, size, zd, pv, zn);
  }

  template<OpType optype>
  requires (optype == OpType::Destructive)
  void splice(SubRegSize size, ZRegister zd, PRegister pv, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd needs to equal zn");
    SVEPermuteVectorPredicated(0b01100, 0b0, size, zd, pv, zm);
  }

  // SVE Permute Vector - Predicated
  // SVE extract element to general register
  void lasta(SubRegSize size, Register rd, PRegister pg, ZRegister zn) {
    SVEPermuteVectorPredicated(0b00000, 0b1, size, ZRegister {rd.Idx()}, pg, zn);
  }
  void lastb(SubRegSize size, Register rd, PRegister pg, ZRegister zn) {
    SVEPermuteVectorPredicated(0b00001, 0b1, size, ZRegister {rd.Idx()}, pg, zn);
  }

  // SVE extract element to SIMD&FP scalar register
  void lasta(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEPermuteVectorPredicated(0b00010, 0b0, size, ZRegister {vd.Idx()}, pg, zn);
  }
  void lastb(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    SVEPermuteVectorPredicated(0b00011, 0b0, size, ZRegister {vd.Idx()}, pg, zn);
  }

  // SVE reverse within elements
  void revb(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Can't use 8-bit element size");
    SVEPermuteVectorPredicated(0b00100, 0b0, size, zd, pg, zn);
  }
  void revh(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "Can't use 8/16-bit element sizes");
    SVEPermuteVectorPredicated(0b00101, 0b0, size, zd, pg, zn);
  }
  void revw(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit, "Can't use 8/16/32-bit element sizes");
    SVEPermuteVectorPredicated(0b00110, 0b0, size, zd, pg, zn);
  }
  void rbit(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEPermuteVectorPredicated(0b00111, 0b0, size, zd, pg, zn);
  }

  // SVE conditionally broadcast element to vector
  void clasta(SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd must be the same as zn");
    SVEPermuteVectorPredicated(0b01000, 0b0, size, zd, pg, zm);
  }
  void clastb(SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd must be the same as zn");
    SVEPermuteVectorPredicated(0b01001, 0b0, size, zd, pg, zm);
  }

  // SVE conditionally extract element to SIMD&FP scalar
  void clasta(SubRegSize size, VRegister vd, PRegister pg, VRegister vn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(vd == vn, "vd must be the same as vn");
    SVEPermuteVectorPredicated(0b01010, 0b0, size, ZRegister {vd.Idx()}, pg, zm);
  }
  void clastb(SubRegSize size, VRegister vd, PRegister pg, VRegister vn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(vd == vn, "vd must be the same as vn");
    SVEPermuteVectorPredicated(0b01011, 0b0, size, ZRegister {vd.Idx()}, pg, zm);
  }

  // SVE reverse doublewords (SME)
  // XXX:

  // SVE conditionally extract element to general register
  void clasta(SubRegSize size, Register rd, PRegister pg, Register rn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(rd == rn, "rd must be the same as rn");
    SVEPermuteVectorPredicated(0b10000, 0b1, size, ZRegister {rd.Idx()}, pg, zm);
  }
  void clastb(SubRegSize size, Register rd, PRegister pg, Register rn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(rd == rn, "rd must be the same as rn");
    SVEPermuteVectorPredicated(0b10001, 0b1, size, ZRegister {rd.Idx()}, pg, zm);
  }

  // SVE Permute Vector - Extract
  // Constructive
  template<OpType optype>
  requires (optype == OpType::Constructive)
  void ext(ZRegister zd, ZRegister zn, ZRegister zn2, uint8_t Imm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zn, zn2), "zn and zn2 must be sequential registers");
    SVEPermuteVector(1, zd, zn, Imm);
  }

  // Destructive
  template<OpType optype>
  requires (optype == OpType::Destructive)
  void ext(ZRegister zd, ZRegister zdn, ZRegister zm, uint8_t Imm) {
    LOGMAN_THROW_A_FMT(zd == zdn, "Dest needs to equal zdn");
    SVEPermuteVector(0, zd, zm, Imm);
  }

  // SVE Permute Vector - Segments
  // SVE permute vector segments
  // XXX:

  // SVE Integer Compare - Vectors
  // SVE integer compare vectors
  void cmpeq(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(1, 1, 0, size, zm, pg, zn, pd);
  }
  void cmpge(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(1, 0, 0, size, zm, pg, zn, pd);
  }
  void cmpgt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(1, 0, 1, size, zm, pg, zn, pd);
  }
  void cmphi(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(0, 0, 1, size, zm, pg, zn, pd);
  }
  void cmphs(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(0, 0, 0, size, zm, pg, zn, pd);
  }
  void cmpne(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVector(1, 1, 1, size, zm, pg, zn, pd);
  }

  // SVE integer compare with wide elements
  void cmpeq_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b01, 0, size, pd, pg, zn, zm);
  }
  void cmpgt_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b10, 1, size, pd, pg, zn, zm);
  }
  void cmpge_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b10, 0, size, pd, pg, zn, zm);
  }
  void cmphi_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(1, 0b10, 1, size, pd, pg, zn, zm);
  }
  void cmphs_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(1, 0b10, 0, size, pd, pg, zn, zm);
  }
  void cmplt_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b11, 0, size, pd, pg, zn, zm);
  }
  void cmple_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b11, 1, size, pd, pg, zn, zm);
  }
  void cmplo_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(1, 0b11, 0, size, pd, pg, zn, zm);
  }
  void cmpls_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(1, 0b11, 1, size, pd, pg, zn, zm);
  }
  void cmpne_wide(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    SVEIntegerCompareVectorWide(0, 0b01, 1, size, pd, pg, zn, zm);
  }

  // SVE Propagate Break
  // SVE propagate break from previous partition
  void brkpa(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPropagateBreak(0b0000, 0b11, 0, pd, pg, pn, pm);
  }
  void brkpb(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPropagateBreak(0b0000, 0b11, 1, pd, pg, pn, pm);
  }
  void brkpas(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPropagateBreak(0b0100, 0b11, 0, pd, pg, pn, pm);
  }
  void brkpbs(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    SVEPropagateBreak(0b0100, 0b11, 1, pd, pg, pn, pm);
  }

  // SVE Partition Break
  // SVE propagate break to next partition
  void brkn(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    LOGMAN_THROW_A_FMT(pd == pm, "pd and pm need to be the same");
    SVEPropagateBreak(0b0001, 0b01, 0, pd, pg, pn, PReg::p8);
  }
  void brkns(PRegister pd, PRegisterZero pg, PRegister pn, PRegister pm) {
    LOGMAN_THROW_A_FMT(pd == pm, "pd and pm need to be the same");
    SVEPropagateBreak(0b0101, 0b01, 0, pd, pg, pn, PReg::p8);
  }

  // SVE partition break condition
  void brka(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPropagateBreak(0b0001, 0b01, 0, pd, pg, pn, PReg::p0);
  }
  void brka(PRegister pd, PRegisterMerge pg, PRegister pn) {
    SVEPropagateBreak(0b0001, 0b01, 1, pd, pg, pn, PReg::p0);
  }
  void brkas(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPropagateBreak(0b0101, 0b01, 0, pd, pg, pn, PReg::p0);
  }
  void brkb(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPropagateBreak(0b1001, 0b01, 0, pd, pg, pn, PReg::p0);
  }
  void brkb(PRegister pd, PRegisterMerge pg, PRegister pn) {
    SVEPropagateBreak(0b1001, 0b01, 1, pd, pg, pn, PReg::p0);
  }
  void brkbs(PRegister pd, PRegisterZero pg, PRegister pn) {
    SVEPropagateBreak(0b1101, 0b01, 0, pd, pg, pn, PReg::p0);
  }

  // SVE Predicate Misc
  void pnext(SubRegSize size, PRegister pd, PRegister pv, PRegister pn) {
    LOGMAN_THROW_A_FMT(pd == pn, "pd and pn need to be the same");
    SVEPredicateMisc(0b1001, 0b00010, pv.Idx(), size, pd);
  }

  // SVE predicate test
  void ptest(PRegister pg, PRegister pn) {
    SVEPredicateMisc(0b0000, pg.Idx() << 1, pn.Idx(), SubRegSize::i16Bit, PReg::p0);
  }

  // SVE predicate first active
  void pfirst(PRegister pd, PRegister pg, PRegister pn) {
    LOGMAN_THROW_A_FMT(pd == pn, "pd and pn need to be the same");
    SVEPredicateMisc(0b1000, 0b00000, pg.Idx(), SubRegSize::i16Bit, pd);
  }

  // SVE predicate zero
  void pfalse(PRegister pd) {
    SVEPredicateMisc(0b1000, 0b10010, 0b0000, SubRegSize::i8Bit, pd);
  }

  // SVE predicate read from FFR (predicated)
  void rdffr(PRegister pd, PRegisterZero pg) {
    SVEPredicateMisc(0b1000, 0b11000, pg.Idx(), SubRegSize::i8Bit, pd);
  }

  void rdffrs(PRegister pd, PRegisterZero pg) {
    SVEPredicateMisc(0b1000, 0b11000, pg.Idx(), SubRegSize::i16Bit, pd);
  }

  // SVE predicate read from FFR (unpredicated)
  void rdffr(PRegister pd) {
    SVEPredicateMisc(0b1001, 0b11000, 0b0000, SubRegSize::i8Bit, pd);
  }

  // SVE predicate initialize
  void ptrue(SubRegSize size, PRegister pd, PredicatePattern pattern) {
    SVEPredicateMisc(0b1000, 0b10000, FEXCore::ToUnderlying(pattern), size, pd);
  }
  void ptrues(SubRegSize size, PRegister pd, PredicatePattern pattern) {
    SVEPredicateMisc(0b1001, 0b10000, FEXCore::ToUnderlying(pattern), size, pd);
  }

  // SVE Integer Compare - Scalars
  // SVE integer compare scalar count and limit
  template<IsXOrWRegister T>
  void whilege(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar(IsXRegister << 2, 0, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilegt(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar(IsXRegister << 2, 1, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilelt(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b001, 0, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilele(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b001, 1, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilehs(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b010, 0, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilehi(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b010, 1, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilelo(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b011, 0, pd.Idx(), size, rn, rm);
  }
  template<IsXOrWRegister T>
  void whilels(SubRegSize size, PRegister pd, T rn, T rm) {
    constexpr auto IsXRegister = static_cast<uint32_t>(std::is_same_v<T, XRegister>);
    SVEIntCompareScalar((IsXRegister << 2) | 0b011, 1, pd.Idx(), size, rn, rm);
  }

  // SVE conditionally terminate scalars
  template<IsXOrWRegister T>
  void ctermeq(T rn, T rm) {
    constexpr auto size = std::is_same_v<T, XRegister> ? SubRegSize::i64Bit : SubRegSize::i32Bit;
    SVEIntCompareScalar(0b1000, 0, 0b0000, size, rn, rm);
  }
  template<IsXOrWRegister T>
  void ctermne(T rn, T rm) {
    constexpr auto size = std::is_same_v<T, XRegister> ? SubRegSize::i64Bit : SubRegSize::i32Bit;
    SVEIntCompareScalar(0b1000, 1, 0b0000, size, rn, rm);
  }

  // SVE pointer conflict compare
  void whilewr(SubRegSize size, PRegister pd, XRegister rn, XRegister rm) {
    SVEIntCompareScalar(0b1100, 0, pd.Idx(), size, rn, rm);
  }
  void whilerw(SubRegSize size, PRegister pd, XRegister rn, XRegister rm) {
    SVEIntCompareScalar(0b1100, 1, pd.Idx(), size, rn, rm);
  }

  // SVE Integer Wide Immediate - Unpredicated
  // SVE integer add/subtract immediate (unpredicated)
  void add(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b000, size, zd, zn, imm);
  }
  void sub(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b001, size, zd, zn, imm);
  }
  void subr(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b011, size, zd, zn, imm);
  }
  void sqadd(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b100, size, zd, zn, imm);
  }
  void uqadd(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b101, size, zd, zn, imm);
  }
  void sqsub(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b110, size, zd, zn, imm);
  }
  void uqsub(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    SVEAddSubImmediateUnpred(0b111, size, zd, zn, imm);
  }

  // SVE integer min/max immediate (unpredicated)
  void smax(SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    SVEMinMaxImmediateUnpred(0b000, size, zd, zn, imm);
  }
  void umax(SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    SVEMinMaxImmediateUnpred(0b001, size, zd, zn, imm);
  }
  void smin(SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    SVEMinMaxImmediateUnpred(0b010, size, zd, zn, imm);
  }
  void umin(SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    SVEMinMaxImmediateUnpred(0b011, size, zd, zn, imm);
  }

  // SVE integer multiply immediate (unpredicated)
  void mul(SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    SVEMultiplyImmediateUnpred(0b000, size, zd, zn, imm);
  }

  // SVE broadcast integer immediate (unpredicated)
  void dup_imm(SubRegSize size, ZRegister zd, int32_t Value) {
    SVEBroadcastImm(0b00, Value, size, zd);
  }
  void mov_imm(SubRegSize size, ZRegister zd, int32_t Value) {
    dup_imm(size, zd, Value);
  }

  // SVE broadcast floating-point immediate (unpredicated)
  void fdup(SubRegSize size, ZRegister zd, float Value) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Unsupported fmov size");

    uint32_t Imm {};
    if (size == SubRegSize::i16Bit) {
      LOGMAN_MSG_A_FMT("Unsupported");
      FEX_UNREACHABLE;
    } else if (size == SubRegSize::i32Bit) {
      Imm = FP32ToImm8(Value);
    } else if (size == SubRegSize::i64Bit) {
      Imm = FP64ToImm8(Value);
    }

    SVEBroadcastFloatImmUnpredicated(0b00, 0, Imm, size, zd);
  }
  void fmov(SubRegSize size, ZRegister zd, float Value) {
    fdup(size, zd, Value);
  }

  // SVE Predicate Count
  // SVE predicate count
  void cntp(SubRegSize size, XRegister rd, PRegister pg, PRegister pn) {
    SVEPredicateCount(0b000, size, rd, pg, pn);
  }

  // SVE Inc/Dec by Predicate Count
  // SVE saturating inc/dec vector by predicate count
  void sqincp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(0, 0, 0b00, 0b00, size, zdn, pm);
  }
  void uqincp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(0, 0, 0b00, 0b01, size, zdn, pm);
  }
  void sqdecp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(0, 0, 0b00, 0b10, size, zdn, pm);
  }
  void uqdecp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(0, 0, 0b00, 0b11, size, zdn, pm);
  }

  // SVE saturating inc/dec register by predicate count
  void sqincp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b10, 0b00, size, rdn, pm);
  }
  void sqincp(SubRegSize size, XRegister rdn, PRegister pm, WRegister wn) {
    LOGMAN_THROW_A_FMT(rdn.Idx() == wn.Idx(), "rdn and wn must be the same");
    SVEIncDecPredicateCountScalar(0, 1, 0b00, 0b00, size, rdn, pm);
  }
  void uqincp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b10, 0b01, size, rdn, pm);
  }
  void uqincp(SubRegSize size, WRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b00, 0b01, size, rdn, pm);
  }
  void sqdecp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b10, 0b10, size, rdn, pm);
  }
  void sqdecp(SubRegSize size, XRegister rdn, PRegister pm, WRegister wn) {
    LOGMAN_THROW_A_FMT(rdn.Idx() == wn.Idx(), "rdn and wn must be the same");
    SVEIncDecPredicateCountScalar(0, 1, 0b00, 0b10, size, rdn, pm);
  }
  void uqdecp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b10, 0b11, size, rdn, pm);
  }
  void uqdecp(SubRegSize size, WRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(0, 1, 0b00, 0b11, size, rdn, pm);
  }

  // SVE inc/dec vector by predicate count
  void incp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(1, 0, 0b00, 0b00, size, zdn, pm);
  }
  void decp(SubRegSize size, ZRegister zdn, PRegister pm) {
    SVEIncDecPredicateCountVector(1, 0, 0b00, 0b01, size, zdn, pm);
  }

  // SVE inc/dec register by predicate count
  void incp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(1, 1, 0b00, 0b00, size, rdn, pm);
  }
  void decp(SubRegSize size, XRegister rdn, PRegister pm) {
    SVEIncDecPredicateCountScalar(1, 1, 0b00, 0b01, size, rdn, pm);
  }

  // SVE Write FFR
  // SVE FFR write from predicate
  void wrffr(PRegister pn) {
    SVEWriteFFR(0, 0b00, 0b000, pn.Idx(), 0b00000);
  }
  // SVE FFR initialise
  void setffr() {
    SVEWriteFFR(1, 0b00, 0b000, 0b0000, 0b00000);
  }

  // SVE Integer Multiply-Add - Unpredicated
  void cdot(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, Rotation rot) {
    SVEIntegerDotProduct(0b0001, size, zda, zn, zm, rot);
  }

  // SVE integer dot product (unpredicated)
  void sdot(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerDotProduct(0b0000, size, zda, zn, zm, Rotation::ROTATE_0);
  }
  void udot(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerDotProduct(0b0000, size, zda, zn, zm, Rotation::ROTATE_90);
  }

  // SVE2 saturating multiply-add interleaved long
  void sqdmlalbt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2SaturatingMulAddInterleaved(0b000010, size, zda, zn, zm);
  }
  void sqdmlslbt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2SaturatingMulAddInterleaved(0b000011, size, zda, zn, zm);
  }

  // SVE2 complex integer multiply-add
  void cmla(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, Rotation rot) {
    SVEIntegerComplexMulAdd(0b0010, size, zda, zn, zm, rot);
  }
  void sqrdcmlah(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, Rotation rot) {
    SVEIntegerComplexMulAdd(0b0011, size, zda, zn, zm, rot);
  }

  // SVE2 integer multiply-add long
  void smlalb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'000, size, zda, zn, zm);
  }
  void smlalt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'001, size, zda, zn, zm);
  }
  void umlalb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'010, size, zda, zn, zm);
  }
  void umlalt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'011, size, zda, zn, zm);
  }
  void smlslb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'100, size, zda, zn, zm);
  }
  void smlslt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'101, size, zda, zn, zm);
  }
  void umlslb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'110, size, zda, zn, zm);
  }
  void umlslt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b010'111, size, zda, zn, zm);
  }

  // SVE2 saturating multiply-add long
  void sqdmlalb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b0110'00, size, zda, zn, zm);
  }
  void sqdmlalt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b0110'01, size, zda, zn, zm);
  }
  void sqdmlslb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b0110'10, size, zda, zn, zm);
  }
  void sqdmlslt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerMulAddLong(0b0110'11, size, zda, zn, zm);
  }

  // SVE2 saturating multiply-add high
  void sqrdmlah(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerMultiplyAddUnpredicated(0b011'100, size, zda, zn, zm);
  }
  void sqrdmlsh(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerMultiplyAddUnpredicated(0b011'101, size, zda, zn, zm);
  }

  // SVE mixed sign dot product
  void usdot(ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerDotProduct(0b0111, SubRegSize::i32Bit, zda, zn, zm, Rotation::ROTATE_180);
  }

  // SVE2 Integer - Predicated
  // SVE2 integer pairwise add and accumulate long
  void sadalp(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerPairwiseAddAccumulateLong(0, size, zda, pg, zn);
  }
  void uadalp(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerPairwiseAddAccumulateLong(1, size, zda, pg, zn);
  }

  // SVE2 integer unary operations (predicated)
  void urecpe(ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerUnaryOpsPredicated(0b00000, SubRegSize::i32Bit, zd, pg, zn);
  }
  void ursqrte(ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerUnaryOpsPredicated(0b00001, SubRegSize::i32Bit, zd, pg, zn);
  }
  void sqabs(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerUnaryOpsPredicated(0b01000, size, zd, pg, zn);
  }
  void sqneg(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerUnaryOpsPredicated(0b01001, size, zd, pg, zn);
  }

  // SVE2 saturating/rounding bitwise shift left (predicated)
  void srshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b00010, size, zd, pg, zn, zm);
  }
  void urshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b00011, size, zd, pg, zn, zm);
  }
  void srshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b00110, size, zd, pg, zn, zm);
  }
  void urshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b00111, size, zd, pg, zn, zm);
  }
  void sqshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01000, size, zd, pg, zn, zm);
  }
  void uqshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01001, size, zd, pg, zn, zm);
  }
  void sqrshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01010, size, zd, pg, zn, zm);
  }
  void uqrshl(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01011, size, zd, pg, zn, zm);
  }
  void sqshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01100, size, zd, pg, zn, zm);
  }
  void uqshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01101, size, zd, pg, zn, zm);
  }
  void sqrshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01110, size, zd, pg, zn, zm);
  }
  void uqrshlr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2SaturatingRoundingBitwiseShiftLeft(0b01111, size, zd, pg, zn, zm);
  }

  // SVE2 integer halving add/subtract (predicated)
  void shadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b000, size, pg, zd, zn, zm);
  }
  void uhadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b001, size, pg, zd, zn, zm);
  }
  void shsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b010, size, pg, zd, zn, zm);
  }
  void uhsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b011, size, pg, zd, zn, zm);
  }
  void srhadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b100, size, pg, zd, zn, zm);
  }
  void urhadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b101, size, pg, zd, zn, zm);
  }
  void shsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b110, size, pg, zd, zn, zm);
  }
  void uhsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerHalvingPredicated(0b111, size, pg, zd, zn, zm);
  }

  // SVE2 integer pairwise arithmetic
  void addp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerPairwiseArithmetic(0b00, 1, size, pg, zd, zn, zm);
  }
  void smaxp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerPairwiseArithmetic(0b10, 0, size, pg, zd, zn, zm);
  }
  void umaxp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerPairwiseArithmetic(0b10, 1, size, pg, zd, zn, zm);
  }
  void sminp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerPairwiseArithmetic(0b11, 0, size, pg, zd, zn, zm);
  }
  void uminp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEIntegerPairwiseArithmetic(0b11, 1, size, pg, zd, zn, zm);
  }

  // SVE2 saturating add/subtract
  void sqadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b000, size, zd, pg, zn, zm);
  }
  void uqadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b001, size, zd, pg, zn, zm);
  }
  void sqsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b010, size, zd, pg, zn, zm);
  }
  void uqsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b011, size, zd, pg, zn, zm);
  }
  void suqadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b100, size, zd, pg, zn, zm);
  }
  void usqadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b101, size, zd, pg, zn, zm);
  }
  void sqsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b110, size, zd, pg, zn, zm);
  }
  void uqsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVE2IntegerSaturatingAddSub(0b111, size, zd, pg, zn, zm);
  }

  // SVE2 Widening Integer Arithmetic
  // SVE2 integer add/subtract long
  void saddlb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b000, size, zd, zn, zm);
  }

  void saddlt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b001, size, zd, zn, zm);
  }

  void uaddlb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b010, size, zd, zn, zm);
  }

  void uaddlt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b011, size, zd, zn, zm);
  }

  void ssublb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b100, size, zd, zn, zm);
  }

  void ssublt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b101, size, zd, zn, zm);
  }

  void usublb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b110, size, zd, zn, zm);
  }

  void usublt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(0, 0b111, size, zd, zn, zm);
  }

  void sabdlb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(1, 0b100, size, zd, zn, zm);
  }

  void sabdlt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(1, 0b101, size, zd, zn, zm);
  }

  void uabdlb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(1, 0b110, size, zd, zn, zm);
  }

  void uabdlt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLong(1, 0b111, size, zd, zn, zm);
  }

  // SVE2 integer add/subtract wide
  void saddwb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b000, size, zd, zn, zm);
  }
  void saddwt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b001, size, zd, zn, zm);
  }
  void uaddwb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b010, size, zd, zn, zm);
  }
  void uaddwt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b011, size, zd, zn, zm);
  }
  void ssubwb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b100, size, zd, zn, zm);
  }
  void ssubwt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b101, size, zd, zn, zm);
  }
  void usubwb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b110, size, zd, zn, zm);
  }
  void usubwt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubWide(0b111, size, zd, zn, zm);
  }

  // SVE2 integer multiply long
  void sqdmullb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b000, size, zd, zn, zm);
  }
  void sqdmullt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b001, size, zd, zn, zm);
  }
  void pmullb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b010, size, zd, zn, zm);
  }
  void pmullt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b011, size, zd, zn, zm);
  }
  void smullb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b100, size, zd, zn, zm);
  }
  void smullt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b101, size, zd, zn, zm);
  }
  void umullb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b110, size, zd, zn, zm);
  }
  void umullt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerMultiplyLong(0b111, size, zd, zn, zm);
  }

  //
  // SVE Misc
  // SVE2 bitwise shift left long
  void sshllb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftLeftLong(size, 0b00, zd, zn, shift);
  }
  void sshllt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftLeftLong(size, 0b01, zd, zn, shift);
  }
  void ushllb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftLeftLong(size, 0b10, zd, zn, shift);
  }
  void ushllt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftLeftLong(size, 0b11, zd, zn, shift);
  }

  // SVE2 integer add/subtract interleaved long
  void saddlbt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b00, zd, zn, zm);
  }
  void ssublbt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b10, zd, zn, zm);
  }
  void ssubltb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b11, zd, zn, zm);
  }

  // SVE2 bitwise exclusive-or interleaved
  void eorbt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2BitwiseXorInterleaved(size, 0b0, zd, zn, zm);
  }
  void eortb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2BitwiseXorInterleaved(size, 0b1, zd, zn, zm);
  }

  // SVE integer matrix multiply accumulate
  void smmla(ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerMatrixMulAccumulate(0b00, zda, zn, zm);
  }
  void usmmla(ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerMatrixMulAccumulate(0b10, zda, zn, zm);
  }
  void ummla(ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEIntegerMatrixMulAccumulate(0b11, zda, zn, zm);
  }

  // SVE2 bitwise permute
  void bext(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2BitwisePermute(size, 0b00, zd, zn, zm);
  }
  void bdep(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2BitwisePermute(size, 0b01, zd, zn, zm);
  }
  void bgrp(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2BitwisePermute(size, 0b10, zd, zn, zm);
  }

  // SVE2 Accumulate
  // SVE2 complex integer add
  void cadd(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, Rotation rot) {
    SVE2ComplexIntAdd(size, 0b0, rot, zd, zn, zm);
  }
  void sqcadd(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, Rotation rot) {
    SVE2ComplexIntAdd(size, 0b1, rot, zd, zn, zm);
  }

  // SVE2 integer absolute difference and accumulate long
  void sabalb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b10000, zda, zn, zm);
  }
  void sabalt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b10001, zda, zn, zm);
  }
  void uabalb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b10010, zda, zn, zm);
  }
  void uabalt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubInterleavedLong(size, 0b10011, zda, zn, zm);
  }

  // SVE2 integer add/subtract long with carry
  void adclb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLongWithCarry(size, 0, 0, zda, zn, zm);
  }
  void adclt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLongWithCarry(size, 0, 1, zda, zn, zm);
  }
  void sbclb(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLongWithCarry(size, 1, 0, zda, zn, zm);
  }
  void sbclt(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubLongWithCarry(size, 1, 1, zda, zn, zm);
  }

  // SVE2 bitwise shift right and accumulate
  void ssra(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftRightAndAccumulate(size, 0b00, zda, zn, shift);
  }
  void usra(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftRightAndAccumulate(size, 0b01, zda, zn, shift);
  }
  void srsra(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftRightAndAccumulate(size, 0b10, zda, zn, shift);
  }
  void ursra(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftRightAndAccumulate(size, 0b11, zda, zn, shift);
  }

  // SVE2 bitwise shift and insert
  void sri(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftAndInsert(size, 0b0, zda, zn, shift);
  }
  void sli(SubRegSize size, ZRegister zda, ZRegister zn, uint32_t shift) {
    SVE2BitwiseShiftAndInsert(size, 0b1, zda, zn, shift);
  }

  // SVE2 integer absolute difference and accumulate
  void saba(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAbsDiffAndAccumulate(size, 0b0, zda, zn, zm);
  }
  void uaba(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2IntegerAbsDiffAndAccumulate(size, 0b1, zda, zn, zm);
  }

  // SVE2 Narrowing
  // SVE2 saturating extract narrow
  void sqxtnb(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b00, 0, zn, zd);
  }
  void sqxtnt(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b00, 1, zn, zd);
  }
  void uqxtnb(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b01, 0, zn, zd);
  }
  void uqxtnt(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b01, 1, zn, zd);
  }
  void sqxtunb(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b10, 0, zn, zd);
  }
  void sqxtunt(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVE2SaturatingExtractNarrow(size, 0b10, 1, zn, zd);
  }

  // SVE2 bitwise shift right narrow
  void sqshrunb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 0, 0, 0, zn, zd);
  }
  void sqshrunt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 0, 0, 1, zn, zd);
  }
  void sqrshrunb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 0, 1, 0, zn, zd);
  }
  void sqrshrunt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 0, 1, 1, zn, zd);
  }
  void shrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 1, 0, 0, zn, zd);
  }
  void shrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 1, 0, 1, zn, zd);
  }
  void rshrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 1, 1, 0, zn, zd);
  }
  void rshrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 0, 1, 1, 1, zn, zd);
  }
  void sqshrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 0, 0, 0, zn, zd);
  }
  void sqshrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 0, 0, 1, zn, zd);
  }
  void sqrshrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 0, 1, 0, zn, zd);
  }
  void sqrshrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 0, 1, 1, zn, zd);
  }
  void uqshrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 1, 0, 0, zn, zd);
  }
  void uqshrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 1, 0, 1, zn, zd);
  }
  void uqrshrnb(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 1, 1, 0, zn, zd);
  }
  void uqrshrnt(SubRegSize size, ZRegister zd, ZRegister zn, uint32_t Shift) {
    SVE2BitwiseShiftRightNarrow(size, Shift, 1, 1, 1, 1, zn, zd);
  }

  // SVE2 integer add/subtract narrow high part
  void addhnb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b000, zd, zn, zm);
  }
  void addhnt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b001, zd, zn, zm);
  }
  void raddhnb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b010, zd, zn, zm);
  }
  void raddhnt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b011, zd, zn, zm);
  }
  void subhnb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b100, zd, zn, zm);
  }
  void subhnt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b101, zd, zn, zm);
  }
  void rsubhnb(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b110, zd, zn, zm);
  }
  void rsubhnt(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2IntegerAddSubNarrowHighPart(size, 0b111, zd, zn, zm);
  }

  // SVE2 Crypto Extensions
  // SVE2 crypto unary operations
  void aesimc(ZRegister zdn, ZRegister zn) {
    SVE2CryptoUnaryOperation(1, zdn, zn);
  }
  void aesmc(ZRegister zdn, ZRegister zn) {
    SVE2CryptoUnaryOperation(0, zdn, zn);
  }

  // SVE2 crypto destructive binary operations
  void aese(ZRegister zdn, ZRegister zn, ZRegister zm) {
    SVE2CryptoDestructiveBinaryOperation(0, 0, zdn, zn, zm);
  }
  void aesd(ZRegister zdn, ZRegister zn, ZRegister zm) {
    SVE2CryptoDestructiveBinaryOperation(0, 1, zdn, zn, zm);
  }
  void sm4e(ZRegister zdn, ZRegister zn, ZRegister zm) {
    SVE2CryptoDestructiveBinaryOperation(1, 0, zdn, zn, zm);
  }

  // SVE2 crypto constructive binary operations
  void sm4ekey(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2CryptoConstructiveBinaryOperation(0, zd, zn, zm);
  }
  void rax1(ZRegister zd, ZRegister zn, ZRegister zm) {
    SVE2CryptoConstructiveBinaryOperation(1, zd, zn, zm);
  }

  // SVE Floating Point Widening Multiply-Add - Indexed
  // SVE BFloat16 floating-point dot product (indexed)
  // XXX:

  // SVE floating-point multiply-add long (indexed)
  void fmlalb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(0, 0, 0, dstsize, zda, zn, zm, index);
  }
  void fmlalt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(0, 0, 1, dstsize, zda, zn, zm, index);
  }
  void fmlslb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(0, 1, 0, dstsize, zda, zn, zm, index);
  }
  void fmlslt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(0, 1, 1, dstsize, zda, zn, zm, index);
  }
  void bfmlalb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(1, 0, 0, dstsize, zda, zn, zm, index);
  }
  void bfmlalt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(1, 0, 1, dstsize, zda, zn, zm, index);
  }
  void bfmlslb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(1, 1, 0, dstsize, zda, zn, zm, index);
  }
  void bfmlslt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    SVEFPMultiplyAddLongIndexed(1, 1, 1, dstsize, zda, zn, zm, index);
  }

  // SVE Floating Point Widening Multiply-Add
  // SVE BFloat16 floating-point dot product
  // XXX:

  // SVE floating-point multiply-add long
  void fmlalb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(0, 0, 0, dstsize, zda, zn, zm);
  }
  void fmlalt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(0, 0, 1, dstsize, zda, zn, zm);
  }
  void fmlslb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(0, 1, 0, dstsize, zda, zn, zm);
  }
  void fmlslt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(0, 1, 1, dstsize, zda, zn, zm);
  }
  void bfmlalb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(1, 0, 0, dstsize, zda, zn, zm);
  }
  void bfmlalt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(1, 0, 1, dstsize, zda, zn, zm);
  }
  void bfmlslb(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(1, 1, 0, dstsize, zda, zn, zm);
  }
  void bfmlslt(SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAddLong(1, 1, 1, dstsize, zda, zn, zm);
  }

  // SVE Floating Point Arithmetic - Predicated
  void ftmad(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, uint32_t imm) {
    LOGMAN_THROW_A_FMT(imm <= 7, "ftmad immediate must be within 0-7");
    SVEFloatArithmeticPredicated(0b10000 | imm, size, PReg::p0, zd, zn, zm);
  }
  // SVE floating-point arithmetic (predicated)
  void fadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0000, size, pg, zd, zn, zm);
  }
  void fsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0001, size, pg, zd, zn, zm);
  }
  void fmul(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0010, size, pg, zd, zn, zm);
  }
  void fsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0011, size, pg, zd, zn, zm);
  }
  void fmaxnm(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0100, size, pg, zd, zn, zm);
  }
  void fminnm(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0101, size, pg, zd, zn, zm);
  }
  void fmax(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0110, size, pg, zd, zn, zm);
  }
  void fmin(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b0111, size, pg, zd, zn, zm);
  }
  void fabd(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b1000, size, pg, zd, zn, zm);
  }
  void fscale(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b1001, size, pg, zd, zn, zm);
  }
  void fmulx(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b1010, size, pg, zd, zn, zm);
  }
  void fdivr(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b1100, size, pg, zd, zn, zm);
  }
  void fdiv(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFloatArithmeticPredicated(0b1101, size, pg, zd, zn, zm);
  }

  // SVE floating-point arithmetic with immediate (predicated)
  void fadd(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFAddSubImm imm) {
    SVEFPArithWithImmediate(0b000, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fsub(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFAddSubImm imm) {
    SVEFPArithWithImmediate(0b001, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fmul(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFMulImm imm) {
    SVEFPArithWithImmediate(0b010, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fsubr(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFAddSubImm imm) {
    SVEFPArithWithImmediate(0b011, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fmaxnm(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFMaxMinImm imm) {
    SVEFPArithWithImmediate(0b100, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fminnm(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFMaxMinImm imm) {
    SVEFPArithWithImmediate(0b101, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fmax(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFMaxMinImm imm) {
    SVEFPArithWithImmediate(0b110, size, zd, pg, FEXCore::ToUnderlying(imm));
  }
  void fmin(SubRegSize size, ZRegister zd, PRegisterMerge pg, SVEFMaxMinImm imm) {
    SVEFPArithWithImmediate(0b111, size, zd, pg, FEXCore::ToUnderlying(imm));
  }

  // SVE Floating Point Unary Operations - Predicated
  // SVE floating-point round to integral value
  void frinti(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b111, size, zd, pg, zn);
  }
  void frintx(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b110, size, zd, pg, zn);
  }
  void frinta(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b100, size, zd, pg, zn);
  }
  void frintn(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b000, size, zd, pg, zn);
  }
  void frintz(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b011, size, zd, pg, zn);
  }
  void frintm(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b010, size, zd, pg, zn);
  }
  void frintp(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatRoundIntegral(0b001, size, zd, pg, zn);
  }

  // SVE floating-point convert precision
  void fcvt(SubRegSize to, SubRegSize from, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFPConvertPrecision(to, from, zd, pg, zn);
  }
  void fcvtx(ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    uint32_t Instr = 0b0110'0101'0000'1010'1010'0000'0000'0000;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point unary operations
  void frecpx(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatUnary(0b00, size, pg, zn, zd);
  }
  void fsqrt(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVEFloatUnary(0b01, size, pg, zn, zd);
  }

  // SVE integer convert to floating-point
  void scvtf(ZRegister zd, SubRegSize dstsize, PRegisterMerge pg, ZRegister zn, SubRegSize srcsize) {
    uint32_t opc1, opc2;
    if (srcsize == SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      LOGMAN_THROW_A_FMT(dstsize == SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = 0b01;
      opc2 = 0b01;
    } else if (srcsize == SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;

      opc2 = dstsize == SubRegSize::i64Bit ? 0b00 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b10 : 0b00;
    } else if (srcsize == SubRegSize::i64Bit) {
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b11 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b11 : 0b00;
    } else {
      FEX_UNREACHABLE;
    }
    SVEIntegerConvertToFloat(dstsize, srcsize, opc1, opc2, 0, pg, zn, zd);
  }
  void ucvtf(ZRegister zd, SubRegSize dstsize, PRegisterMerge pg, ZRegister zn, SubRegSize srcsize) {
    uint32_t opc1, opc2;
    if (srcsize == SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      LOGMAN_THROW_A_FMT(dstsize == SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = 0b01;
      opc2 = 0b01;
    } else if (srcsize == SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;

      opc2 = dstsize == SubRegSize::i64Bit ? 0b00 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b10 : 0b00;
    } else if (srcsize == SubRegSize::i64Bit) {
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b11 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b11 : 0b00;
    } else {
      FEX_UNREACHABLE;
    }
    SVEIntegerConvertToFloat(dstsize, srcsize, opc1, opc2, 1, pg, zn, zd);
  }

  // SVE floating-point convert to integer
  void flogb(SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    const auto ConvertedSize = size == SubRegSize::i64Bit ? 0b11 :
                               size == SubRegSize::i32Bit ? 0b10 :
                               size == SubRegSize::i16Bit ? 0b01 :
                                                            0b00;

    SVEFloatConvertToInt(size, size, 1, 0b00, ConvertedSize, 0, pg, zn, zd);
  }
  void fcvtzs(ZRegister zd, SubRegSize dstsize, PRegisterMerge pg, ZRegister zn, SubRegSize srcsize) {
    uint32_t opc1, opc2;
    if (srcsize == SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      opc1 = 0b01;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;
    } else if (srcsize == SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      LOGMAN_THROW_A_FMT(dstsize != SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : 0b10;
      opc2 = 0b10;
    } else if (srcsize == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(dstsize != SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = 0b11;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : 0b00;
    } else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(dstsize, srcsize, 1, opc1, opc2, 0, pg, zn, zd);
  }
  void fcvtzu(ZRegister zd, SubRegSize dstsize, PRegisterMerge pg, ZRegister zn, SubRegSize srcsize) {
    uint32_t opc1, opc2;
    if (srcsize == SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      opc1 = 0b01;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : dstsize == SubRegSize::i32Bit ? 0b10 : dstsize == SubRegSize::i16Bit ? 0b01 : 0b00;
    } else if (srcsize == SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      LOGMAN_THROW_A_FMT(dstsize != SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = dstsize == SubRegSize::i64Bit ? 0b11 : 0b10;
      opc2 = 0b10;
    } else if (srcsize == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(dstsize != SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = 0b11;
      opc2 = dstsize == SubRegSize::i64Bit ? 0b11 : 0b00;
    } else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(dstsize, srcsize, 1, opc1, opc2, 1, pg, zn, zd);
  }

  // SVE Floating Point Unary Operations - Unpredicated
  // SVE floating-point reciprocal estimate (unpredicated)
  void frecpe(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVEFPUnaryOpsUnpredicated(0b110, size, zd, zn);
  }
  void frsqrte(SubRegSize size, ZRegister zd, ZRegister zn) {
    SVEFPUnaryOpsUnpredicated(0b111, size, zd, zn);
  }

  // SVE Floating Point Compare - with Zero
  // SVE floating-point compare with zero
  void fcmge(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b00, 0, size, pd, pg, zn);
  }
  void fcmgt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b00, 1, size, pd, pg, zn);
  }
  void fcmlt(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b01, 0, size, pd, pg, zn);
  }
  void fcmle(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b01, 1, size, pd, pg, zn);
  }
  void fcmeq(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b10, 0, size, pd, pg, zn);
  }
  void fcmne(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn) {
    SVEFPCompareWithZero(0b11, 0, size, pd, pg, zn);
  }

  // SVE Floating Point Accumulating Reduction
  // SVE floating-point serial reduction (predicated)
  void fadda(SubRegSize size, VRegister vd, PRegister pg, VRegister vn, ZRegister zm) {
    SVEFPSerialReductionPredicated(0b00, size, vd, pg, vn, zm);
  }

  // SVE Floating Point Multiply-Add
  // SVE floating-point multiply-accumulate writing addend
  void fmla(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAdd(0b000, size, zda, pg, zn, zm);
  }
  void fmls(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAdd(0b001, size, zda, pg, zn, zm);
  }
  void fnmla(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAdd(0b010, size, zda, pg, zn, zm);
  }
  void fnmls(SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    SVEFPMultiplyAdd(0b011, size, zda, pg, zn, zm);
  }

  // SVE floating-point multiply-accumulate writing multiplicand
  void fmad(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEFPMultiplyAdd(0b100, size, zdn, pg, zm, za);
  }
  void fmsb(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEFPMultiplyAdd(0b101, size, zdn, pg, zm, za);
  }
  void fnmad(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEFPMultiplyAdd(0b110, size, zdn, pg, zm, za);
  }
  void fnmsb(SubRegSize size, ZRegister zdn, PRegisterMerge pg, ZRegister zm, ZRegister za) {
    SVEFPMultiplyAdd(0b111, size, zdn, pg, zm, za);
  }

  // SVE Memory - 32-bit Gather and Unsized Contiguous
  void ldr(PRegister pt, XRegister rn, int32_t imm = 0) {
    SVEUnsizedLoadStoreContiguous(0b0, imm, ZRegister {pt.Idx()}, rn, false);
  }
  void ldr(ZRegister zt, XRegister rn, int32_t imm = 0) {
    SVEUnsizedLoadStoreContiguous(0b1, imm, zt, rn, false);
  }

  // SVE 32-bit gather prefetch (scalar plus 32-bit scaled offsets)
  // XXX:
  // SVE contiguous prefetch (scalar plus immediate)
  // XXX:
  // SVE2 32-bit gather non-temporal load (vector plus scalar)
  // XXX:
  // SVE contiguous prefetch (scalar plus scalar)
  // XXX:
  // SVE 32-bit gather prefetch (vector plus immediate)
  // XXX:

  // SVE load and broadcast element
  void ld1rb(SubRegSize esize, ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(false, esize, SubRegSize::i8Bit, zt, pg, rn, imm);
  }
  void ld1rsb(SubRegSize esize, ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(true, esize, SubRegSize::i8Bit, zt, pg, rn, imm);
  }
  void ld1rh(SubRegSize esize, ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(false, esize, SubRegSize::i16Bit, zt, pg, rn, imm);
  }
  void ld1rsh(SubRegSize esize, ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(true, esize, SubRegSize::i16Bit, zt, pg, rn, imm);
  }
  void ld1rw(SubRegSize esize, ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(false, esize, SubRegSize::i32Bit, zt, pg, rn, imm);
  }
  void ld1rsw(ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(true, SubRegSize::i64Bit, SubRegSize::i32Bit, zt, pg, rn, imm);
  }
  void ld1rd(ZRegister zt, PRegisterZero pg, Register rn, uint32_t imm = 0) {
    SVELoadAndBroadcastElement(false, SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, rn, imm);
  }

  // SVE contiguous non-temporal load (scalar plus immediate)
  void ldnt1b(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalLoad(0b00, zt, pg, rn, Imm);
  }
  void ldnt1h(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalLoad(0b01, zt, pg, rn, Imm);
  }
  void ldnt1w(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalLoad(0b10, zt, pg, rn, Imm);
  }
  void ldnt1d(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalLoad(0b11, zt, pg, rn, Imm);
  }

  // SVE contiguous non-temporal load (scalar plus scalar)
  // XXX:
  // SVE load multiple structures (scalar plus immediate)
  void ld2b(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, false, 0b00, Imm, zt1, pg, rn);
  }
  void ld3b(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, false, 0b00, Imm, zt1, pg, rn);
  }
  void ld4b(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, false, 0b00, Imm, zt1, pg, rn);
  }
  void ld2h(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, false, 0b01, Imm, zt1, pg, rn);
  }
  void ld3h(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, false, 0b01, Imm, zt1, pg, rn);
  }
  void ld4h(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, false, 0b01, Imm, zt1, pg, rn);
  }
  void ld2w(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, false, 0b10, Imm, zt1, pg, rn);
  }
  void ld3w(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, false, 0b10, Imm, zt1, pg, rn);
  }
  void ld4w(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, false, 0b10, Imm, zt1, pg, rn);
  }
  void ld2d(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, false, 0b11, Imm, zt1, pg, rn);
  }
  void ld3d(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, false, 0b11, Imm, zt1, pg, rn);
  }
  void ld4d(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, false, 0b11, Imm, zt1, pg, rn);
  }

  // SVE helper implementations
  template<SubRegSize size>
  void ld1b(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i8Bit, zt, pg, Src, true, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i8Bit, zt, pg, Src, true, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ldff1b(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1b doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i8Bit, zt, pg, Src, true, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i8Bit, zt, pg, Src, true, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  void ld1sw(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1sw(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1sw(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(SubRegSize::i64Bit, SubRegSize::i32Bit, zt, pg, Src, false, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(SubRegSize::i64Bit, SubRegSize::i32Bit, zt, pg, Src, false, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ld1h(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i16Bit, zt, pg, Src, true, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i16Bit, zt, pg, Src, true, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ld1sh(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1sh<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1sh<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i16Bit, zt, pg, Src, false, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i16Bit, zt, pg, Src, false, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ldff1h(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1h doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i16Bit, zt, pg, Src, true, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i16Bit, zt, pg, Src, true, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ldff1sh(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1sh<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1sh doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i16Bit, zt, pg, Src, false, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i16Bit, zt, pg, Src, false, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ld1w(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i32Bit, zt, pg, Src, true, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i32Bit, zt, pg, Src, true, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ldff1w(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1w doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i32Bit, zt, pg, Src, true, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i32Bit, zt, pg, Src, true, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  void ldff1sw(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1sw(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1sw doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(SubRegSize::i64Bit, SubRegSize::i32Bit, zt, pg, Src, false, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(SubRegSize::i64Bit, SubRegSize::i32Bit, zt, pg, Src, false, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ld1sb(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1sb<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1sb<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i8Bit, zt, pg, Src, false, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i8Bit, zt, pg, Src, false, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void ldff1sb(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1sb<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1sb doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(size, SubRegSize::i8Bit, zt, pg, Src, false, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(size, SubRegSize::i8Bit, zt, pg, Src, false, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  void ld1d(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ld1d(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      ld1d(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src, true, false);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src, true, false);
    } else {
      FEX_UNREACHABLE;
    }
  }

  void ldff1d(ZRegister zt, PRegisterZero pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      ldff1d(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      LOGMAN_THROW_A_FMT(false, "ldff1d doesn't have a scalar plus immediate variant");
    } else if (Src.IsScalarPlusVector()) {
      SVEGatherLoadScalarPlusVector(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src, true, true);
    } else if (Src.IsVectorPlusImm()) {
      SVEGatherLoadVectorPlusImm(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src, true, true);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void st1b(ZRegister zt, PRegister pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      st1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      st1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEScatterStoreScalarPlusVector(size, SubRegSize::i8Bit, zt, pg, Src);
    } else if (Src.IsVectorPlusImm()) {
      SVEScatterStoreVectorPlusImm(size, SubRegSize::i8Bit, zt, pg, Src);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void st1h(ZRegister zt, PRegister pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      st1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      st1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEScatterStoreScalarPlusVector(size, SubRegSize::i16Bit, zt, pg, Src);
    } else if (Src.IsVectorPlusImm()) {
      SVEScatterStoreVectorPlusImm(size, SubRegSize::i16Bit, zt, pg, Src);
    } else {
      FEX_UNREACHABLE;
    }
  }

  template<SubRegSize size>
  void st1w(ZRegister zt, PRegister pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      st1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      st1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEScatterStoreScalarPlusVector(size, SubRegSize::i32Bit, zt, pg, Src);
    } else if (Src.IsVectorPlusImm()) {
      SVEScatterStoreVectorPlusImm(size, SubRegSize::i32Bit, zt, pg, Src);
    } else {
      FEX_UNREACHABLE;
    }
  }

  void st1d(ZRegister zt, PRegister pg, SVEMemOperand Src) {
    if (Src.IsScalarPlusScalar()) {
      st1d(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    } else if (Src.IsScalarPlusImm()) {
      st1d(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    } else if (Src.IsScalarPlusVector()) {
      SVEScatterStoreScalarPlusVector(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src);
    } else if (Src.IsVectorPlusImm()) {
      SVEScatterStoreVectorPlusImm(SubRegSize::i64Bit, SubRegSize::i64Bit, zt, pg, Src);
    } else {
      FEX_UNREACHABLE;
    }
  }

  // SVE load multiple structures (scalar plus scalar)
  void ld2b(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i8Bit, 0b01, zt1, pg, rn, rm);
  }
  void ld3b(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i8Bit, 0b10, zt1, pg, rn, rm);
  }
  void ld4b(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i8Bit, 0b11, zt1, pg, rn, rm);
  }
  void ld2h(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i16Bit, 0b01, zt1, pg, rn, rm);
  }
  void ld3h(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i16Bit, 0b10, zt1, pg, rn, rm);
  }
  void ld4h(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i16Bit, 0b11, zt1, pg, rn, rm);
  }
  void ld2w(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i32Bit, 0b01, zt1, pg, rn, rm);
  }
  void ld3w(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i32Bit, 0b10, zt1, pg, rn, rm);
  }
  void ld4w(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i32Bit, 0b11, zt1, pg, rn, rm);
  }
  void ld2d(ZRegister zt1, ZRegister zt2, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i64Bit, 0b01, zt1, pg, rn, rm);
  }
  void ld3d(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i64Bit, 0b10, zt1, pg, rn, rm);
  }
  void ld4d(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegisterZero pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(false, SubRegSize::i64Bit, 0b11, zt1, pg, rn, rm);
  }

  // SVE load and broadcast quadword (scalar plus immediate)
  void ld1rqb(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b00, 0b00, zt, pg, rn, imm);
  }
  void ld1rob(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b00, 0b01, zt, pg, rn, imm);
  }
  void ld1rqh(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b01, 0b00, zt, pg, rn, imm);
  }
  void ld1roh(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b01, 0b01, zt, pg, rn, imm);
  }
  void ld1rqw(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b10, 0b00, zt, pg, rn, imm);
  }
  void ld1row(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b10, 0b01, zt, pg, rn, imm);
  }
  void ld1rqd(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b11, 0b00, zt, pg, rn, imm);
  }
  void ld1rod(ZRegister zt, PRegisterZero pg, Register rn, int imm = 0) {
    SVELoadBroadcastQuadScalarPlusImm(0b11, 0b01, zt, pg, rn, imm);
  }

  // SVE contiguous load (scalar plus immediate)
  template<SubRegSize size>
  void ld1b(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    SVEContiguousLoadImm(false, 0b0000 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  void ld1sw(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    SVEContiguousLoadImm(false, 0b0100, Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1h(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    static_assert(size != SubRegSize::i8Bit, "Invalid size");
    SVEContiguousLoadImm(false, 0b0100 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1sh(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");

    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 1 : size == SubRegSize::i64Bit ? 0 : -1;

    SVEContiguousLoadImm(false, 0b1000 | ConvertedSize, Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1w(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");

    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 0 : size == SubRegSize::i64Bit ? 1 : -1;

    SVEContiguousLoadImm(false, 0b1010 | ConvertedSize, Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1sb(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    static_assert(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");

    constexpr uint32_t ConvertedSize = size == SubRegSize::i16Bit ? 0b10 :
                                       size == SubRegSize::i32Bit ? 0b01 :
                                       size == SubRegSize::i64Bit ? 0b00 :
                                                                    -1;

    SVEContiguousLoadImm(false, 0b1100 | ConvertedSize, Imm, pg, rn, zt);
  }
  void ld1d(ZRegister zt, PRegisterZero pg, Register rn, int32_t Imm = 0) {
    SVEContiguousLoadImm(false, 0b1111, Imm, pg, rn, zt);
  }

  // SVE contiguous non-fault load (scalar plus immediate)
  // XXX:

  // SVE load and broadcast quadword (scalar plus scalar)
  void ld1rqb(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b00, 0b00, zt, pg, rn, rm);
  }
  void ld1rob(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b00, 0b01, zt, pg, rn, rm);
  }
  void ld1rqh(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b01, 0b00, zt, pg, rn, rm);
  }
  void ld1roh(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b01, 0b01, zt, pg, rn, rm);
  }
  void ld1rqw(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b10, 0b00, zt, pg, rn, rm);
  }
  void ld1row(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b10, 0b01, zt, pg, rn, rm);
  }
  void ld1rqd(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b11, 0b00, zt, pg, rn, rm);
  }
  void ld1rod(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVELoadBroadcastQuadScalarPlusScalar(0b11, 0b01, zt, pg, rn, rm);
  }

  // SVE contiguous load (scalar plus scalar)
  template<SubRegSize size>
  void ld1b(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 0, 0b0000 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  void ld1sw(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 0, 0b0100, rm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1h(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size != SubRegSize::i8Bit, "Invalid size");
    SVEContiguousLoadStore(0, 0, 0b0100 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1sh(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 1 : size == SubRegSize::i64Bit ? 0 : -1;
    SVEContiguousLoadStore(0, 0, 0b1000 | ConvertedSize, rm, pg, rn, zt);
  }

  template<SubRegSize size>
  void ld1w(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 0 : size == SubRegSize::i64Bit ? 1 : -1;
    SVEContiguousLoadStore(0, 0, 0b1010 | ConvertedSize, rm, pg, rn, zt);
  }
  template<SubRegSize size>
  void ld1sb(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i16Bit ? 0b10 :
                                       size == SubRegSize::i32Bit ? 0b01 :
                                       size == SubRegSize::i64Bit ? 0b00 :
                                                                    -1;
    SVEContiguousLoadStore(0, 0, 0b1100 | ConvertedSize, rm, pg, rn, zt);
  }

  void ld1d(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 0, 0b1111, rm, pg, rn, zt);
  }

  // SVE contiguous first-fault load (scalar plus scalar)
  template<SubRegSize size>
  void ldff1b(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 1, 0b0000 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }
  template<SubRegSize size>
  void ldff1sb(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i16Bit ? 0b10 :
                                       size == SubRegSize::i32Bit ? 0b01 :
                                       size == SubRegSize::i64Bit ? 0b00 :
                                                                    -1;
    SVEContiguousLoadStore(0, 1, 0b1100 | ConvertedSize, rm, pg, rn, zt);
  }
  template<SubRegSize size>
  void ldff1h(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size != SubRegSize::i8Bit, "Invalid size");
    SVEContiguousLoadStore(0, 1, 0b0100 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }
  template<SubRegSize size>
  void ldff1sh(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 1 : size == SubRegSize::i64Bit ? 0 : -1;
    SVEContiguousLoadStore(0, 1, 0b1000 | ConvertedSize, rm, pg, rn, zt);
  }
  template<SubRegSize size>
  void ldff1w(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 0 : size == SubRegSize::i64Bit ? 1 : -1;
    SVEContiguousLoadStore(0, 1, 0b1010 | ConvertedSize, rm, pg, rn, zt);
  }
  void ldff1sw(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 1, 0b0100, rm, pg, rn, zt);
  }
  void ldff1d(ZRegister zt, PRegisterZero pg, Register rn, Register rm) {
    SVEContiguousLoadStore(0, 1, 0b1111, rm, pg, rn, zt);
  }

  // SVE Memory - 64-bit Gather
  // SVE 64-bit gather prefetch (scalar plus 64-bit scaled offsets)
  // XXX:
  // SVE 64-bit gather prefetch (scalar plus unpacked 32-bit scaled offsets)
  // XXX:
  // SVE 64-bit gather prefetch (vector plus immediate)
  // XXX:
  // SVE2 64-bit gather non-temporal load (vector plus scalar)
  // XXX:

  // SVE Memory - Contiguous Store and Unsized Contiguous
  void str(PRegister pt, XRegister rn, int32_t imm = 0) {
    SVEUnsizedLoadStoreContiguous(0b0, imm, ZRegister {pt.Idx()}, rn, true);
  }
  void str(ZRegister zt, XRegister rn, int32_t imm = 0) {
    SVEUnsizedLoadStoreContiguous(0b1, imm, zt, rn, true);
  }

  // SVE contiguous store (scalar plus scalar)
  template<SubRegSize size>
  void st1b(ZRegister zt, PRegister pg, Register rn, Register rm) {
    SVEContiguousLoadStore(1, 0, 0b0000 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<SubRegSize size>
  void st1h(ZRegister zt, PRegister pg, Register rn, Register rm) {
    static_assert(size != SubRegSize::i8Bit, "Invalid size");
    SVEContiguousLoadStore(1, 0, 0b0100 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<SubRegSize size>
  void st1w(ZRegister zt, PRegister pg, Register rn, Register rm) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 0 : size == SubRegSize::i64Bit ? 1 : -1;

    SVEContiguousLoadStore(1, 0, 0b1010 | ConvertedSize, rm, pg, rn, zt);
  }
  void st1d(ZRegister zt, PRegister pg, Register rn, Register rm) {
    SVEContiguousLoadStore(1, 0, 0b1111, rm, pg, rn, zt);
  }

  // SVE Memory - Non-temporal and Multi-register Store
  // SVE2 64-bit scatter non-temporal store (vector plus scalar)
  // XXX:
  // SVE contiguous non-temporal store (scalar plus scalar)
  // XXX:
  // SVE2 32-bit scatter non-temporal store (vector plus scalar)
  // XXX:

  // SVE store multiple structures (scalar plus scalar)
  void st2b(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i8Bit, 0b01, zt1, pg, rn, rm);
  }
  void st3b(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i8Bit, 0b10, zt1, pg, rn, rm);
  }
  void st4b(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i8Bit, 0b11, zt1, pg, rn, rm);
  }
  void st2h(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i16Bit, 0b01, zt1, pg, rn, rm);
  }
  void st3h(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i16Bit, 0b10, zt1, pg, rn, rm);
  }
  void st4h(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i16Bit, 0b11, zt1, pg, rn, rm);
  }
  void st2w(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i32Bit, 0b01, zt1, pg, rn, rm);
  }
  void st3w(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i32Bit, 0b10, zt1, pg, rn, rm);
  }
  void st4w(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i32Bit, 0b11, zt1, pg, rn, rm);
  }
  void st2d(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i64Bit, 0b01, zt1, pg, rn, rm);
  }
  void st3d(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i64Bit, 0b10, zt1, pg, rn, rm);
  }
  void st4d(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousLoadStoreMultipleScalar(true, SubRegSize::i64Bit, 0b11, zt1, pg, rn, rm);
  }

  // SVE Memory - Contiguous Store with Immediate Offset
  // SVE contiguous non-temporal store (scalar plus immediate)
  void stnt1b(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalStore(0b00, zt, pg, rn, Imm);
  }
  void stnt1h(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalStore(0b01, zt, pg, rn, Imm);
  }
  void stnt1w(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalStore(0b10, zt, pg, rn, Imm);
  }
  void stnt1d(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousNontemporalStore(0b11, zt, pg, rn, Imm);
  }

  // SVE store multiple structures (scalar plus immediate)
  void st2b(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, true, 0b00, Imm, zt1, pg, rn);
  }
  void st3b(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, true, 0b00, Imm, zt1, pg, rn);
  }
  void st4b(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, true, 0b00, Imm, zt1, pg, rn);
  }
  void st2h(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, true, 0b01, Imm, zt1, pg, rn);
  }
  void st3h(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, true, 0b01, Imm, zt1, pg, rn);
  }
  void st4h(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, true, 0b01, Imm, zt1, pg, rn);
  }
  void st2w(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, true, 0b10, Imm, zt1, pg, rn);
  }
  void st3w(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, true, 0b10, Imm, zt1, pg, rn);
  }
  void st4w(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, true, 0b10, Imm, zt1, pg, rn);
  }
  void st2d(ZRegister zt1, ZRegister zt2, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(2, true, 0b11, Imm, zt1, pg, rn);
  }
  void st3d(ZRegister zt1, ZRegister zt2, ZRegister zt3, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(3, true, 0b11, Imm, zt1, pg, rn);
  }
  void st4d(ZRegister zt1, ZRegister zt2, ZRegister zt3, ZRegister zt4, PRegister pg, Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(zt1, zt2, zt3, zt4), "Registers need to be contiguous");
    SVEContiguousMultipleStructures(4, true, 0b11, Imm, zt1, pg, rn);
  }

  // SVE contiguous store (scalar plus immediate)
  template<SubRegSize size>
  void st1b(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousLoadImm(true, 0b0000 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void st1h(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    static_assert(size != SubRegSize::i8Bit, "Invalid size");
    SVEContiguousLoadImm(true, 0b0100 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<SubRegSize size>
  void st1w(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    static_assert(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Invalid size");

    constexpr uint32_t ConvertedSize = size == SubRegSize::i32Bit ? 0 : size == SubRegSize::i64Bit ? 1 : -1;

    SVEContiguousLoadImm(true, 0b1010 | ConvertedSize, Imm, pg, rn, zt);
  }

  void st1d(ZRegister zt, PRegister pg, Register rn, int32_t Imm = 0) {
    SVEContiguousLoadImm(true, 0b1111, Imm, pg, rn, zt);
  }
private:
  // SVE encodings
  void SVEDupIndexed(SubRegSize size, ZRegister zn, ZRegister zd, uint32_t Index) {
    const auto size_bytes = 1U << FEXCore::ToUnderlying(size);
    const auto log2_size_bytes = FEXCore::ilog2(size_bytes);

    // We can index up to 512-bit registers with dup
    const auto max_index = (64U >> log2_size_bytes) - 1;
    LOGMAN_THROW_A_FMT(Index <= max_index, "dup index ({}) too large. Must be within [0, {}].", Index, max_index);

    // imm2:tsz make up a 7 bit wide field, with each increasing element size
    // restricting the range of those 7 bits (e.g. B: tsz=xxxx1, H: tsz=xxx10,
    // S: tsz=xx100. etc). So we can just use the log2 of the element size
    // to construct the overall immediate and form both imm2 and tsz.
    const auto imm7 = (Index << (log2_size_bytes + 1)) | (1U << log2_size_bytes);
    const auto imm2 = imm7 >> 5;
    const auto tsz = imm7 & 0b11111;

    uint32_t Instr = 0b0000'0101'0010'0000'0010'0000'0000'0000;
    Instr |= imm2 << 22;
    Instr |= tsz << 16;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  void SVEAddSubImmediateUnpred(uint32_t opc, SubRegSize size, ZRegister zd, ZRegister zn, uint32_t imm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zn, "zd needs to equal zn");

    const bool is_uint8_imm = (imm >> 8) == 0;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(is_uint8_imm, "Can't perform LSL #8 shift on 8-bit elements.");
    }

    uint32_t shift = 0;
    if (!is_uint8_imm) {
      const bool is_uint16_imm = (imm >> 16) == 0;

      LOGMAN_THROW_A_FMT(is_uint16_imm, "Immediate ({}) must be a 16-bit value within [256, 65280]", imm);
      LOGMAN_THROW_A_FMT((imm % 256) == 0, "Immediate ({}) must be a multiple of 256", imm);

      imm /= 256;
      shift = 1;
    }

    uint32_t Instr = 0b0010'0101'0010'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= shift << 13;
    Instr |= imm << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEMinMaxImmediateUnpred(uint32_t opc, SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zn, "zd needs to equal zn");

    const bool is_signed = (opc & 1) == 0;
    if (is_signed) {
      LOGMAN_THROW_A_FMT(imm >= -128 && imm <= 127, "Invalid immediate ({}). Must be within [-127, 128]", imm);
    } else {
      LOGMAN_THROW_A_FMT(imm >= 0 && imm <= 255, "Invalid immediate ({}). Must be within [0, 255]", imm);
    }

    const auto imm8 = static_cast<uint32_t>(imm) & 0xFF;

    uint32_t Instr = 0b0010'0101'0010'1000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= imm8 << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEMultiplyImmediateUnpred(uint32_t opc, SubRegSize size, ZRegister zd, ZRegister zn, int32_t imm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zn, "zd needs to equal zn");
    LOGMAN_THROW_A_FMT(imm >= -128 && imm <= 127, "Invalid immediate ({}). Must be within [-127, 128]", imm);

    const auto imm8 = static_cast<uint32_t>(imm) & 0xFF;

    uint32_t Instr = 0b0010'0101'0011'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= imm8 << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBroadcastImm(uint32_t opc, int32_t imm, SubRegSize size, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    const auto [new_imm, is_shift] = HandleSVESImm8Shift(size, imm);

    uint32_t Instr = 0b0010'0101'0011'1000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= is_shift << 13;
    Instr |= (static_cast<uint32_t>(new_imm) & 0xFF) << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBroadcastFloatImmPredicated(SubRegSize size, ZRegister zd, PRegister pg, float value) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Unsupported fcpy/fmov size");

    uint32_t imm {};
    if (size == SubRegSize::i16Bit) {
      LOGMAN_MSG_A_FMT("Unsupported");
      FEX_UNREACHABLE;
    } else if (size == SubRegSize::i32Bit) {
      imm = FP32ToImm8(value);
    } else if (size == SubRegSize::i64Bit) {
      imm = FP64ToImm8(value);
    }

    uint32_t Instr = 0b0000'0101'0001'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= pg.Idx() << 16;
    Instr |= imm << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBroadcastFloatImmUnpredicated(uint32_t opc, uint32_t o2, uint32_t imm, SubRegSize size, ZRegister zd) {
    uint32_t Instr = 0b0010'0101'0011'1001'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= o2 << 13;
    Instr |= imm << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBroadcastIntegerImmPredicated(uint32_t m, SubRegSize size, ZRegister zd, PRegister pg, int32_t imm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    const auto [new_imm, is_shift] = HandleSVESImm8Shift(size, imm);

    uint32_t Instr = 0b0000'0101'0001'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= pg.Idx() << 16;
    Instr |= m << 14;
    Instr |= is_shift << 13;
    Instr |= (static_cast<uint32_t>(new_imm) & 0xFF) << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEAddressGeneration(SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm, SVEModType mod, uint32_t scale) {
    LOGMAN_THROW_A_FMT(scale <= 3, "Scale ({}) must be within [0, 3]", scale);

    uint32_t Instr = 0b0000'0100'0010'0000'1010'0000'0000'0000;

    switch (mod) {
    case SVEModType::MOD_UXTW:
    case SVEModType::MOD_SXTW: {
      LOGMAN_THROW_A_FMT(size == SubRegSize::i64Bit, "Unpacked ADR must be using 64-bit elements");

      const auto is_unsigned = mod == SVEModType::MOD_UXTW;
      if (is_unsigned) {
        Instr |= 1U << 22;
      }
      break;
    }
    case SVEModType::MOD_NONE:
    case SVEModType::MOD_LSL: {
      if (mod == SVEModType::MOD_NONE) {
        LOGMAN_THROW_A_FMT(scale == 0, "Cannot scale packed ADR without a modifier");
      }
      LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Packed ADR must be using 32-bit or 64-bit elements");
      Instr |= FEXCore::ToUnderlying(size) << 22;
      break;
    }
    }

    Instr |= zm.Idx() << 16;
    Instr |= scale << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVESel(SubRegSize size, ZRegister zm, PRegister pv, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    uint32_t Instr = 0b0000'0101'0010'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= pv.Idx() << 10;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  void SVEBitwiseShiftbyVector(uint32_t R, uint32_t L, uint32_t U, SubRegSize size, PRegister pg, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd == zn, "Dest needs to equal zn");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0001'0000'1000'0000'0000'0000;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= R << 18;
    Instr |= L << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE integer add/subtract vectors (unpredicated)
  void SVEIntegerAddSubUnpredicated(uint32_t opc, SubRegSize size, ZRegister zm, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    uint32_t Instr = 0b0000'0100'0010'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE table lookup (three sources)
  void SVETableLookup(uint32_t op, SubRegSize size, ZRegister zm, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    uint32_t Instr = 0b0000'0101'0010'0000'0010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE permute vector elements
  void SVEPermute(uint32_t opc, SubRegSize size, ZRegister zm, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    uint32_t Instr = 0b0000'0101'0010'0000'0110'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE predicate logical operations
  void SVEPredicateLogical(uint32_t op, uint32_t S, uint32_t o2, uint32_t o3, PRegister pm, PRegister pg, PRegister pn, PRegister pd) {
    uint32_t Instr = 0b0010'0101'0000'0000'0100'0000'0000'0000;
    Instr |= op << 23;
    Instr |= S << 22;
    Instr |= pm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= o2 << 9;
    Instr |= pn.Idx() << 5;
    Instr |= o3 << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  // SVE floating-point convert precision odd elements
  void SVEFloatConvertOdd(uint32_t opc, uint32_t opc2, PRegister pg, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0100'0000'1000'1010'0000'0000'0000;
    Instr |= opc << 22;
    Instr |= opc2 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE2 floating-point pairwise operations
  void SVEFloatPairwiseArithmetic(uint32_t opc, SubRegSize size, PRegister pg, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd needs to equal zn");
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Invalid float size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0100'0001'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point arithmetic (unpredicated)
  void SVEFloatArithmeticUnpredicated(uint32_t opc, SubRegSize size, ZRegister zm, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Invalid float size");

    uint32_t Instr = 0b0110'0101'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE bitwise logical operations (predicated)
  void SVEBitwiseLogicalPredicated(uint32_t opc, SubRegSize size, PRegister pg, ZRegister zdn, ZRegister zm, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd == zdn, "zd needs to equal zdn");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0001'1000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE constructive prefix (predicated)
  void SVEConstructivePrefixPredicated(uint32_t opc, uint32_t M, SubRegSize size, PRegister pg, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0001'0000'0010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= M << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE bitwise unary operations (predicated)
  void SVEIntegerUnaryPredicated(uint32_t op0, uint32_t opc, SubRegSize size, PRegister pg, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0000'0000'1010'0000'0000'0000;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op0 << 19;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE bitwise logical operations (unpredicated)
  void SVEBitwiseLogicalUnpredicated(uint32_t opc, ZRegister zm, ZRegister zn, ZRegister zd) {
    uint32_t Instr = 0b0000'0100'0010'0000'0011'0000'0000'0000;

    Instr |= opc << 22;
    Instr |= zm.Idx() << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE Permute Vector - Unpredicated
  void SVEPermuteUnpredicated(SubRegSize size, uint32_t opc, ZRegister zdn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0000'0101'0010'0000'0011'1000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= zm.Idx() << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  // SVE Permute Predicate
  void SVEPermutePredicate(SubRegSize size, uint32_t op1, uint32_t op2, uint32_t op3, PRegister pd, PRegister pn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0000'0101'0010'0000'0100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op1 << 16;
    Instr |= op2 << 9;
    Instr |= op3 << 4;
    Instr |= pn.Idx() << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  // SVE Integer Misc - Unpredicated
  void SVEIntegerMiscUnpredicated(uint32_t op0, uint32_t opc, uint32_t opc2, ZRegister zd, ZRegister zn) {
    uint32_t Instr = 0b0000'0100'0010'0000'1011'0000'0000'0000;
    Instr |= opc2 << 22;
    Instr |= opc << 16;
    Instr |= op0 << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point arithmetic (predicated)
  void SVEFloatArithmeticPredicated(uint32_t opc, SubRegSize size, PRegister pg, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zn needs to equal zd");
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Invalid float size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0101'0000'0000'1000'0000'0000'0000;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVECharacterMatch(uint32_t opc, SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit, "match/nmatch can only use 8-bit or 16-bit element sizes");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7.Zeroing(), "match/nmatch can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0100'0101'0010'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 4;
    Instr |= zm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEFPRecursiveReduction(uint32_t opc, SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "FP reduction operation can only use 16/32/64-bit element sizes");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "FP reduction operation can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0101'0000'0000'0010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= vd.Idx();
    dc32(Instr);
  }

  void SVEAddSubVectorsPredicated(uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd and zn must be the same register");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Add/Sub operation can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerMulDivVectorsPredicated(uint32_t b18, uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zd and zn must be the same register");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Mul/Div operation can only use p0-p7 as a governing predicate");

    // Division instruction
    if (b18 != 0) {
      LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Predicated divide only handles 32-bit or 64-bit "
                                                                                   "elements");
    }

    uint32_t Instr = 0b0000'0100'0001'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= b18 << 18;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerReductionOperation(uint32_t op, uint32_t opc, SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size for reduction operation");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Integer reduction operation can only use p0-p7 as a governing predicate");

    uint32_t Instr = op;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= vd.Idx();
    dc32(Instr);
  }

  void SVEIntegerMultiplyAddSubPredicated(uint32_t op0, uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0000'0000'0100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op0 << 15;
    Instr |= opc << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEStackFrameOperation(uint32_t opc, XRegister rd, XRegister rn, int32_t imm) {
    LOGMAN_THROW_A_FMT(imm >= -32 && imm <= 31, "Stack frame operation immediate must be within -32 to 31");

    uint32_t Instr = 0b0000'0100'0010'0000'0101'0000'0000'0000;
    Instr |= opc << 22;
    Instr |= rn.Idx() << 16;
    Instr |= (static_cast<uint32_t>(imm) & 0b111111) << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  void SVEBitwiseShiftByWideElementPredicated(SubRegSize size, uint32_t opc, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit && size != SubRegSize::i128Bit, "Can't use 64-bit or 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zn, "zd and zn must be the same register");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7.Merging(), "Wide shift can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0001'1000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBitwiseShiftByWideElementsUnpredicated(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit && size != SubRegSize::i128Bit, "Can't use 64-bit or 128-bit element size");

    uint32_t Instr = 0b0000'0100'0010'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 10;
    Instr |= zm.Idx() << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEFPArithWithImmediate(uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, uint32_t i1) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");

    uint32_t Instr = 0b0110'0101'0001'1000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= i1 << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEFPConvertPrecision(SubRegSize to, SubRegSize from, ZRegister zd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(to != from, "to and from sizes cannot be the same.");
    LOGMAN_THROW_A_FMT(to != SubRegSize::i8Bit && to != SubRegSize::i128Bit && from != SubRegSize::i8Bit && from != SubRegSize::i128Bit,
                       "Can't use 8-bit or 128-bit element size");

    // Encodings for the to and from sizes can get a little funky
    // depending on what is being converted to/from.
    const uint32_t op = [&] {
      switch (from) {
      case SubRegSize::i16Bit: {
        switch (to) {
        case SubRegSize::i32Bit: return 0x00810000U;
        case SubRegSize::i64Bit: return 0x00C10000U;
        default: return UINT32_MAX;
        }
      }

      case SubRegSize::i32Bit: {
        switch (to) {
        case SubRegSize::i16Bit: return 0x00800000U;
        case SubRegSize::i64Bit: return 0x00C30000U;
        default: return UINT32_MAX;
        }
      }

      case SubRegSize::i64Bit: {
        switch (to) {
        case SubRegSize::i16Bit: return 0x00C00000U;
        case SubRegSize::i32Bit: return 0x00C20000U;
        default: return UINT32_MAX;
        }
      }

      default: return UINT32_MAX;
      }
    }();
    LOGMAN_THROW_A_FMT(op != UINT32_MAX, "Invalid conversion op value: {}", op);

    uint32_t Instr = 0b0110'0101'0000'1000'1010'0000'0000'0000;
    Instr |= op;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2IntegerAddSubNarrowHighPart(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit && size != SubRegSize::i128Bit, "Can't use 64-bit or 128-bit element size");

    uint32_t Instr = 0b0100'0101'0010'0000'0110'0000'0000'0000;
    Instr |= (FEXCore::ToUnderlying(size) + 1) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2CryptoUnaryOperation(uint32_t op, ZRegister zdn, ZRegister zn) {
    LOGMAN_THROW_A_FMT(zdn == zn, "zdn and zn must be the same register");

    uint32_t Instr = 0b0100'0101'0010'0000'1110'0000'0000'0000;
    Instr |= op << 10;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  void SVE2CryptoDestructiveBinaryOperation(uint32_t op, uint32_t o2, ZRegister zdn, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zdn == zn, "zdn and zn must be the same register");

    uint32_t Instr = 0b0100'0101'0010'0010'1110'0000'0000'0000;
    Instr |= op << 16;
    Instr |= o2 << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  void SVE2CryptoConstructiveBinaryOperation(uint32_t op, ZRegister zd, ZRegister zn, ZRegister zm) {
    uint32_t Instr = 0b0100'0101'0010'0000'1111'0000'0000'0000;
    Instr |= zm.Idx() << 16;
    Instr |= op << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2BitwisePermute(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    uint32_t Instr = 0b0100'0101'0000'0000'1011'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseXorInterleaved(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    uint32_t Instr = 0b0100'0101'0000'0000'1001'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerMatrixMulAccumulate(uint32_t opc, ZRegister zda, ZRegister zn, ZRegister zm) {
    uint32_t Instr = 0b0100'0101'0000'0000'1001'1000'0000'0000;
    Instr |= opc << 22;
    Instr |= zm.Idx() << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVE2IntegerAddSubInterleavedLong(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");

    uint32_t Instr = 0b0100'0101'0000'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2IntegerAbsDiffAndAccumulate(SubRegSize size, uint32_t opc, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    uint32_t Instr = 0b0100'0101'0000'0000'1111'1000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVE2IntegerAddSubLongWithCarry(SubRegSize size, uint32_t sizep1, uint32_t T, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Element size must be 32-bit or 64-bit");

    const uint32_t NewSize = size == SubRegSize::i32Bit ? 0 : 1;

    uint32_t Instr = 0b0100'0101'0000'0000'1101'0000'0000'0000;
    Instr |= sizep1 << 23;
    Instr |= NewSize << 22;
    Instr |= zm.Idx() << 16;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseShiftRightAndAccumulate(SubRegSize size, uint32_t opc, ZRegister zda, ZRegister zn, uint32_t shift) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Element size cannot be 128-bit");

    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, shift);

    uint32_t Instr = 0b0100'0101'0000'0000'1110'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseShiftAndInsert(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, uint32_t shift) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Element size cannot be 128-bit");

    const bool IsLeftShift = opc != 0;
    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, shift, IsLeftShift);

    uint32_t Instr = 0b0100'0101'0000'0000'1111'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseShiftLeftLong(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, uint32_t shift) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");

    // The size provided in is the size to expand to (e.g. 16-bit means a long shift
    // expanding from 8-bit) so we just need to subtract the size by 1 so that our
    // encoding helper will perform the proper encoding.
    const auto size_minus_1 = SubRegSize {FEXCore::ToUnderlying(size) - 1};
    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size_minus_1, shift, true);

    uint32_t Instr = 0b0100'0101'0000'0000'1010'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2ComplexIntAdd(SubRegSize size, uint32_t opc, Rotation rot, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Complex add cannot use 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zn, "zd and zn must be the same register");
    LOGMAN_THROW_A_FMT(rot == Rotation::ROTATE_90 || rot == Rotation::ROTATE_270, "Rotation must be 90 or 270 degrees");

    const uint32_t SanitizedRot = rot == Rotation::ROTATE_90 ? 0 : 1;

    uint32_t Instr = 0b0100'0101'0000'0000'1101'1000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= SanitizedRot << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2AbsDiffAccLong(SubRegSize size, uint32_t opc, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Cannot use 8-bit or 128-bit element size");

    uint32_t Instr = 0b0100'0101'0000'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVEPermuteVectorUnpredicated(SubRegSize size, uint32_t opc, ZRegister zdn, VRegister vm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0000'0101'0010'0000'0011'1000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= vm.Idx() << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  // SVE floating-point round to integral value
  void SVEFloatRoundIntegral(uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn) {
    // opc = round mode
    // 0b000 - N - Neaest ties to even
    // 0b001 - P - Towards +inf
    // 0b010 - M - Towards -inf
    // 0b011 - Z - Towards zero
    // 0b100 - A - Nearest away from zero
    // 0b101 - Unallocated
    // 0b110 - X - Current signalling inexact
    // 0b111 - I - Current

    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Unsupported size in {}", __func__);

    uint32_t Instr = 0b0110'0101'0000'0000'1010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point convert to integer
  void SVEFloatConvertToInt(SubRegSize dstsize, SubRegSize srcsize, uint32_t b19, uint32_t opc, uint32_t opc2, uint32_t U, PRegister pg,
                            ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(srcsize == SubRegSize::i16Bit || srcsize == SubRegSize::i32Bit || srcsize == SubRegSize::i64Bit,
                       "Unsupported src size in {}", __func__);
    LOGMAN_THROW_A_FMT(dstsize == SubRegSize::i16Bit || dstsize == SubRegSize::i32Bit || dstsize == SubRegSize::i64Bit,
                       "Unsupported dst size in {}", __func__);

    uint32_t Instr = 0b0110'0101'0001'0000'1010'0000'0000'0000;
    Instr |= opc << 22;
    Instr |= b19 << 19;
    Instr |= opc2 << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // SVE integer convert to floating-point
  // We can implement this in terms of the floating-point to int version above,
  // since the only difference in encoding is setting bit 19 to 0.
  void SVEIntegerConvertToFloat(SubRegSize dstsize, SubRegSize srcsize, uint32_t opc, uint32_t opc2, uint32_t U, PRegister pg, ZRegister zn,
                                ZRegister zd) {
    SVEFloatConvertToInt(dstsize, srcsize, 0, opc, opc2, U, pg, zn, zd);
  }

  // SVE Memory - 32-bit Gather and Unsized Contiguous
  // Note: This also handles 64-bit variants to keep overall handling code
  //       compact and in the same place.
  void SVEGatherLoadScalarPlusVector(SubRegSize esize, SubRegSize msize, ZRegister zt, PRegisterZero pg, SVEMemOperand mem_op,
                                     bool is_unsigned, bool is_fault_first) {
    LOGMAN_THROW_A_FMT(esize == SubRegSize::i32Bit || esize == SubRegSize::i64Bit, "Gather load element size must be 32-bit or 64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    const auto& op_data = mem_op.MetaType.ScalarVectorType;
    const bool is_scaled = op_data.scale != 0;
    const auto msize_value = FEXCore::ToUnderlying(msize);

    LOGMAN_THROW_A_FMT(op_data.scale == 0 || op_data.scale == msize_value, "scale may only be 0 or {}", msize_value);

    uint32_t mod_value = FEXCore::ToUnderlying(op_data.mod);
    uint32_t Instr = 0b1000'0100'0000'0000'0000'0000'0000'0000;

    if (esize == SubRegSize::i64Bit) {
      Instr |= 1U << 30;

      const auto mod = op_data.mod;
      const bool is_lsl = mod == SVEModType::MOD_LSL;
      const bool is_none = mod == SVEModType::MOD_NONE;

      // LSL and no modifier encodings should be setting bit 22 to 1.
      if (is_lsl || is_none) {
        if (is_lsl) {
          LOGMAN_THROW_A_FMT(op_data.scale == msize_value, "mod type of LSL must have a scale of {}", msize_value);
        } else {
          LOGMAN_THROW_A_FMT(op_data.scale == 0, "mod type of none must have a scale of 0");
        }

        Instr |= 1U << 15;
        mod_value = 1;
      }
    } else {
      LOGMAN_THROW_A_FMT(op_data.mod == SVEModType::MOD_UXTW || op_data.mod == SVEModType::MOD_SXTW, "mod type for 32-bit lane size may "
                                                                                                     "only be UXTW or SXTW");
    }

    Instr |= FEXCore::ToUnderlying(msize) << 23;
    Instr |= static_cast<uint32_t>(mod_value) << 22;
    Instr |= static_cast<uint32_t>(is_scaled) << 21;
    Instr |= op_data.zm.Idx() << 16;
    Instr |= static_cast<uint32_t>(is_unsigned) << 14;
    Instr |= static_cast<uint32_t>(is_fault_first) << 13;
    Instr |= pg.Idx() << 10;
    Instr |= mem_op.rn.Idx() << 5;
    Instr |= zt.Idx();

    dc32(Instr);
  }

  void SVEScatterStoreScalarPlusVector(SubRegSize esize, SubRegSize msize, ZRegister zt, PRegister pg, SVEMemOperand mem_op) {
    LOGMAN_THROW_A_FMT(esize == SubRegSize::i32Bit || esize == SubRegSize::i64Bit, "Gather load element size must be 32-bit or 64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    const auto& op_data = mem_op.MetaType.ScalarVectorType;
    const bool is_scaled = op_data.scale != 0;

    const auto msize_value = FEXCore::ToUnderlying(msize);
    uint32_t mod_value = FEXCore::ToUnderlying(op_data.mod);

    LOGMAN_THROW_A_FMT(op_data.scale == 0 || op_data.scale == msize_value, "scale may only be 0 or {}", msize_value);

    uint32_t Instr = 0b1110'0100'0000'0000'1000'0000'0000'0000;

    if (esize == SubRegSize::i64Bit) {
      const auto mod = op_data.mod;
      const bool is_lsl = mod == SVEModType::MOD_LSL;
      const bool is_none = mod == SVEModType::MOD_NONE;

      if (is_lsl || is_none) {
        if (is_lsl) {
          LOGMAN_THROW_A_FMT(op_data.scale == msize_value, "mod type of LSL must have a scale of {}", msize_value);
        } else {
          LOGMAN_THROW_A_FMT(op_data.scale == 0, "mod type of none must have a scale of 0");
        }
        if (is_lsl || is_scaled) {
          LOGMAN_THROW_A_FMT(msize != SubRegSize::i8Bit, "Cannot use 8-bit store elements with unpacked 32-bit scaled offset and "
                                                         "64-bit scaled offset variants. Instructions not allocated.");
        }

        // 64-bit scaled/unscaled scatters need to set bit 13
        Instr |= 1U << 13;
        mod_value = 0;
      }
    } else {
      if (is_scaled) {
        LOGMAN_THROW_A_FMT(msize != SubRegSize::i8Bit && msize != SubRegSize::i64Bit, "Cannot use 8-bit or 64-bit store elements with "
                                                                                      "32-bit scaled offset variant. "
                                                                                      "Instructions not allocated");
      } else {
        LOGMAN_THROW_A_FMT(msize != SubRegSize::i64Bit, "Cannot use 64-bit store elements with 32-bit unscaled offset variant. "
                                                        "Instruction not allocated.");
      }

      LOGMAN_THROW_A_FMT(op_data.mod == SVEModType::MOD_UXTW || op_data.mod == SVEModType::MOD_SXTW, "mod type for 32-bit lane size may "
                                                                                                     "only be UXTW or SXTW");

      // 32-bit scatters need to set bit 22.
      Instr |= 1U << 22;
    }

    Instr |= msize_value << 23;
    Instr |= static_cast<uint32_t>(is_scaled) << 21;
    Instr |= op_data.zm.Idx() << 16;
    Instr |= static_cast<uint32_t>(mod_value) << 14;
    Instr |= pg.Idx() << 10;
    Instr |= mem_op.rn.Idx() << 5;
    Instr |= zt.Idx();

    dc32(Instr);
  }

  void SVEGatherScatterVectorPlusImm(SubRegSize esize, SubRegSize msize, ZRegister zt, PRegister pg, SVEMemOperand mem_op, bool is_store,
                                     bool is_unsigned, bool is_fault_first) {
    LOGMAN_THROW_A_FMT(esize == SubRegSize::i32Bit || esize == SubRegSize::i64Bit, "Gather load/store element size must be 32-bit or "
                                                                                   "64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    const auto msize_value = FEXCore::ToUnderlying(msize);
    const auto msize_bytes = 1U << msize_value;

    const auto imm_limit = (32U << msize_value) - msize_bytes;
    const auto imm = mem_op.MetaType.VectorImmType.Imm;
    const auto imm_to_encode = imm >> msize_value;

    LOGMAN_THROW_A_FMT(imm <= imm_limit, "Immediate must be within [0, {}]", imm_limit);
    LOGMAN_THROW_A_FMT(imm == 0 || (imm % msize_bytes) == 0, "Immediate must be cleanly divisible by {}", msize_bytes);

    uint32_t Instr = 0b1000'0100'0000'0000'1000'0000'0000'0000;

    if (is_store) {
      Instr |= 0x60402000U;
      if (esize == SubRegSize::i32Bit) {
        Instr |= 1U << 21;
      }
    } else {
      Instr |= 0x00200000U;
      if (esize == SubRegSize::i64Bit) {
        Instr |= 1U << 30;
      }
    }

    Instr |= msize_value << 23;
    Instr |= imm_to_encode << 16;
    Instr |= static_cast<uint32_t>(is_unsigned) << 14;
    Instr |= static_cast<uint32_t>(is_fault_first) << 13;
    Instr |= pg.Idx() << 10;
    Instr |= mem_op.rn.Idx() << 5;
    Instr |= zt.Idx();

    dc32(Instr);
  }

  void SVEGatherLoadVectorPlusImm(SubRegSize esize, SubRegSize msize, ZRegister zt, PRegisterZero pg, SVEMemOperand mem_op,
                                  bool is_unsigned, bool is_fault_first) {
    SVEGatherScatterVectorPlusImm(esize, msize, zt, pg, mem_op, false, is_unsigned, is_fault_first);
  }

  void SVEScatterStoreVectorPlusImm(SubRegSize esize, SubRegSize msize, ZRegister zt, PRegister pg, SVEMemOperand mem_op) {
    SVEGatherScatterVectorPlusImm(esize, msize, zt, pg, mem_op, true, false, true);
  }

  void SVEUnsizedLoadStoreContiguous(uint32_t op2, int32_t imm, ZRegister zt, Register rn, bool is_store) {
    LOGMAN_THROW_A_FMT(imm >= -256 && imm <= 255, "Immediate offset ({}) too large. Must be within [-256, 255].", imm);

    const auto imm9 = static_cast<uint32_t>(imm) & 0b1'1111'1111;

    uint32_t Instr = 0b1000'0101'1000'0000'0000'0000'0000'0000;

    if (is_store) {
      Instr |= 0x60000000U;
    }

    Instr |= (imm9 >> 3) << 16;
    Instr |= op2 << 14;
    Instr |= (imm9 & 0b111) << 10;
    Instr |= rn.Idx() << 5;
    Instr |= zt.Idx();

    dc32(Instr);
  }

  // SVE load/store multiple structures (scalar plus immediate)
  void SVEContiguousMultipleStructures(int32_t num_regs, bool is_store, uint32_t msz, int32_t imm, ZRegister zt, PRegister pg, Register rn) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT((imm % num_regs) == 0, "Offset must be a multiple of {}", num_regs);

    const auto min_offset = -8 * num_regs;
    const auto max_offset = 7 * num_regs;
    LOGMAN_THROW_A_FMT(imm >= min_offset && imm <= max_offset,
                       "Invalid load/store offset ({}). Offset must be a multiple of {} and be within [{}, {}]", imm, num_regs, min_offset,
                       max_offset);

    const auto imm4 = static_cast<uint32_t>(imm / num_regs) & 0xF;
    const auto opc = static_cast<uint32_t>(num_regs - 1);

    uint32_t Instr = 0b1010'0100'0000'0000'1110'0000'0000'0000;
    Instr |= msz << 23;
    Instr |= opc << 21;
    Instr |= imm4 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    if (is_store) {
      Instr |= 0x40100000U;
    }
    dc32(Instr);
  }

  // SVE contiguous non-temporal load (scalar plus immediate)
  void SVEContiguousNontemporalLoad(uint32_t msz, ZRegister zt, PRegister pg, Register rn, int32_t imm) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(imm >= -8 && imm <= 7, "Invalid loadstore offset ({}). Must be between [-8, 7]", imm);

    const auto imm4 = static_cast<uint32_t>(imm) & 0xF;
    uint32_t Instr = 0b1010'0100'0000'0000'1110'0000'0000'0000;
    Instr |= msz << 23;
    Instr |= imm4 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }

  // SVE contiguous non-temporal store (scalar plus immediate)
  void SVEContiguousNontemporalStore(uint32_t msz, ZRegister zt, PRegister pg, Register rn, int32_t imm) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(imm >= -8 && imm <= 7, "Invalid loadstore offset ({}). Must be between [-8, 7]", imm);

    const auto imm4 = static_cast<uint32_t>(imm) & 0xF;
    uint32_t Instr = 0b1110'0100'0001'0000'1110'0000'0000'0000;
    Instr |= msz << 23;
    Instr |= imm4 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVEContiguousLoadImm(bool is_store, uint32_t dtype, int32_t imm, PRegister pg, Register rn, ZRegister zt) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(imm >= -8 && imm <= 7, "Invalid loadstore offset ({}). Must be between [-8, 7]", imm);

    const auto imm4 = static_cast<uint32_t>(imm) & 0xF;

    uint32_t Instr = 0b1010'0100'0000'0000'1010'0000'0000'0000;
    Instr |= dtype << 21;
    Instr |= imm4 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    if (is_store) {
      Instr |= 0x40004000U;
    }
    dc32(Instr);
  }

  // zt.b, pg/z, xn, xm
  void SVEContiguousLoadStore(uint32_t b30, uint32_t b13, uint32_t dtype, Register rm, PRegister pg, Register rn, ZRegister zt) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b1010'0100'0000'0000'0100'0000'0000'0000;
    Instr |= b30 << 30;
    Instr |= dtype << 21;
    Instr |= Encode_rm(rm);
    Instr |= b13 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVEContiguousLoadStoreMultipleScalar(bool is_store, SubRegSize msz, uint32_t opc, ZRegister zt, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(rm != Reg::rsp, "rm cannot be the stack pointer");

    uint32_t Instr = 0b1010'0100'0000'0000'0000'0000'0000'0000;
    if (is_store) {
      Instr |= 0x40006000U;
    } else {
      Instr |= 0x0000C000U;
    }
    Instr |= FEXCore::ToUnderlying(msz) << 23;
    Instr |= opc << 21;
    Instr |= rm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= rn.Idx() << 5;
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVELoadBroadcastQuadScalarPlusImm(uint32_t msz, uint32_t ssz, ZRegister zt, PRegister pg, Register rn, int imm) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    const auto esize = static_cast<int>(16 << ssz);
    const auto max_imm = (esize << 3) - esize;
    const auto min_imm = -(max_imm + esize);

    LOGMAN_THROW_A_FMT((imm % esize) == 0, "imm ({}) must be a multiple of {}", imm, esize);
    LOGMAN_THROW_A_FMT(imm >= min_imm && imm <= max_imm, "imm ({}) must be within [{}, {}]", imm, min_imm, max_imm);

    const auto sanitized_imm = static_cast<uint32_t>(imm / esize) & 0b1111;

    uint32_t Instr = 0b1010'0100'0000'0000'0010'0000'0000'0000;
    Instr |= msz << 23;
    Instr |= ssz << 21;
    Instr |= sanitized_imm << 16;
    Instr |= pg.Idx() << 10;
    Instr |= rn.Idx() << 5;
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVELoadBroadcastQuadScalarPlusScalar(uint32_t msz, uint32_t ssz, ZRegister zt, PRegister pg, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(rm != Reg::rsp, "rm may not be the stack pointer");

    uint32_t Instr = 0b1010'0100'0000'0000'0000'0000'0000'0000;
    Instr |= msz << 23;
    Instr |= ssz << 21;
    Instr |= rm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= rn.Idx() << 5;
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVELoadAndBroadcastElement(bool is_signed, SubRegSize esize, SubRegSize msize, ZRegister zt, PRegister pg, Register rn, uint32_t imm) {
    LOGMAN_THROW_A_FMT(esize != SubRegSize::i128Bit, "Cannot use 128-bit elements.");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    if (is_signed) {
      // The element size needs to be larger than memory size, otherwise you tell
      // me how we're gonna sign extend this bad boy in memory.
      LOGMAN_THROW_A_FMT(esize > msize, "Signed broadcast element size must be greater than memory size.");
    }

    const auto esize_value = FEXCore::ToUnderlying(esize);
    const auto msize_value = FEXCore::ToUnderlying(msize);

    const auto data_size_bytes = 1U << msize_value;
    const auto max_imm = (64U << msize_value) - data_size_bytes;
    LOGMAN_THROW_A_FMT((imm % data_size_bytes) == 0 && imm <= max_imm, "imm must be a multiple of {} and be within [0, {}]",
                       data_size_bytes, max_imm);

    const auto sanitized_imm = imm / data_size_bytes;

    auto dtypeh = msize_value;
    auto dtypel = esize_value;
    if (is_signed) {
      // Signed forms of the broadcast instructions are encoded in such a way
      // that msize will always be greater than esize, which, conveniently,
      // works out by just XORing the would-be unsigned dtype values by 3.
      dtypeh ^= 0b11;
      dtypel ^= 0b11;
    }
    // Guards against bogus combinations of element size and memory size values
    // being passed in. Unsigned variants will always have dtypeh be less than
    // or equal to dtypel. The only time this isn't the case is with signed variants.
    LOGMAN_THROW_A_FMT(is_signed == (dtypeh > dtypel),
                       "Invalid element size used with load broadcast instruction "
                       "(esize: {}, msize: {})",
                       esize_value, msize_value);

    uint32_t Instr = 0b1000'0100'0100'0000'1000'0000'0000'0000;
    Instr |= dtypeh << 23;
    Instr |= sanitized_imm << 16;
    Instr |= dtypel << 13;
    Instr |= pg.Idx() << 10;
    Instr |= rn.Idx() << 5;
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVEIndexGeneration(uint32_t op, SubRegSize size, ZRegister zd, int32_t imm5, int32_t imm5b) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "INDEX cannot use 128-bit element sizes");

    uint32_t Instr = 0b0000'0100'0010'0000'0100'0000'0000'0000;
    Instr |= op << 10;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= (static_cast<uint32_t>(imm5b) & 0b11111) << 16;
    Instr |= (static_cast<uint32_t>(imm5) & 0b11111) << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerCompareImm(uint32_t lt, uint32_t ne, uint32_t imm7, SubRegSize size, PRegister pg, ZRegister zn, PRegister pd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(imm7 < 128, "Invalid imm ({}). Must be within [0, 128]", imm7);
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0010'0100'0010'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= imm7 << 14;
    Instr |= lt << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEIntegerCompareSignedImm(uint32_t op, uint32_t o2, uint32_t ne, int32_t imm5, SubRegSize size, PRegister pg, ZRegister zn, PRegister pd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(imm5 >= -16 && imm5 <= 15, "Invalid imm ({}). Must be within [-16, 15].", imm5);
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0010'0101'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= (static_cast<uint32_t>(imm5) & 0b1'1111) << 16;
    Instr |= op << 15;
    Instr |= o2 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEFloatCompareVector(uint32_t op, uint32_t o2, uint32_t o3, SubRegSize size, ZRegister zm, PRegister pg, ZRegister zn, PRegister pd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Can't use 8-bit size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0101'0000'0000'0100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 15;
    Instr |= o2 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= o3 << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEIntegerMinMaxDifferencePredicated(uint32_t opc, uint32_t U, SubRegSize size, PRegister pg, ZRegister zdn, ZRegister zm, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd == zdn, "zd needs to equal zdn");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0100'0000'1000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBitWiseShiftImmediatePred(SubRegSize size, uint32_t opc, uint32_t L, uint32_t U, PRegister pg, ZRegister zd, ZRegister zdn,
                                    uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(zd == zdn, "zd needs to equal zdn");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    const bool IsLeftShift = L != 0;
    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, Shift, IsLeftShift);

    uint32_t Instr = 0b0000'0100'0000'0000'1000'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= opc << 18;
    Instr |= L << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= tszl_imm3 << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEBitWiseShiftImmediateUnpred(SubRegSize size, uint32_t opc, ZRegister zd, ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");

    const bool IsLeftShift = opc == 0b11;
    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, Shift, IsLeftShift);

    uint32_t Instr = 0b0000'0100'0010'0000'1001'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseTernary(uint32_t opc, uint32_t o2, ZRegister zm, ZRegister zk, ZRegister zd, ZRegister zdn) {
    LOGMAN_THROW_A_FMT(zd == zdn, "zd needs to equal zdn");

    uint32_t Instr = 0b0000'0100'0010'0000'0011'1000'0000'0000;
    Instr |= opc << 22;
    Instr |= zm.Idx() << 16;
    Instr |= o2 << 10;
    Instr |= zk.Idx() << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  void SVEPermuteVector(uint32_t op0, ARMEmitter::ZRegister zd, ARMEmitter::ZRegister zm, uint32_t Imm) {
    constexpr uint32_t Op = 0b0000'0101'0010'0000'000 << 13;
    uint32_t Instr = Op;

    Instr |= op0 << 22;
    Instr |= (Imm >> 3) << 16;
    Instr |= (Imm & 0b111) << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerCompareVector(uint32_t op, uint32_t o2, uint32_t ne, SubRegSize size, ZRegister zm, PRegister pg, ZRegister zn, PRegister pd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit element size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    constexpr uint32_t Op = 0b0010'0100'0000'0000'000 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 15;
    Instr |= o2 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEIntegerCompareVectorWide(uint32_t op, uint32_t o2, uint32_t ne, SubRegSize size, PRegister pd, PRegister pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "Can't use 64-bit element size");
    SVEIntegerCompareVector(op, o2, ne, size, zm, pg, zn, pd);
  }

  void SVE2SaturatingExtractNarrow(SubRegSize size, uint32_t opc, uint32_t T, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit && size != SubRegSize::i64Bit, "Can't use 64/128-bit size");

    // While not necessarily a left shift, we can piggyback off its
    // encoding behavior to encode the tszh and tszl bits.
    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, 0, true);

    uint32_t Instr = 0b0100'0101'0010'0000'0100'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2BitwiseShiftRightNarrow(SubRegSize size, uint32_t shift, uint32_t opc, uint32_t U, uint32_t R, uint32_t T, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit && size != SubRegSize::i64Bit, "Can't use 64/128-bit element size");

    const auto [tszh, tszl_imm3] = EncodeSVEShiftImmediate(size, shift);

    uint32_t Instr = 0b0100'0101'0010'0000'0000'0000'0000'0000;
    Instr |= tszh << 22;
    Instr |= tszl_imm3 << 16;
    Instr |= opc << 13;
    Instr |= U << 12;
    Instr |= R << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEFloatUnary(uint32_t opc, SubRegSize size, PRegister pg, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "Unsupported size in {}", __func__);

    uint32_t Instr = 0b0110'0101'0000'1100'1010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2IntegerMultiplyVectors(uint32_t opc, SubRegSize size, ZRegister zm, ZRegister zn, ZRegister zd) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    constexpr uint32_t Op = 0b0000'0100'0010'0000'0110 << 12;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEPermuteVectorPredicated(uint32_t opc1, uint32_t opc2, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0000'0101'0010'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc1 << 16;
    Instr |= opc2 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEPropagateBreak(uint32_t opc, uint32_t op2, uint32_t op3, PRegister pd, PRegister pg, PRegister pn, PRegister pm) {
    uint32_t Instr = 0b0010'0101'0000'0000'0000'0000'0000'0000;
    Instr |= opc << 20;
    Instr |= op2 << 14;
    Instr |= op3 << 4;
    Instr |= pm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= pn.Idx() << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEPredicateMisc(uint32_t op0, uint32_t op2, uint32_t op3, SubRegSize size, PRegister pd) {
    // Note: op2 combines op1 like [op1:op2], since they're adjacent.
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    uint32_t Instr = 0b0010'0101'0001'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op0 << 16;
    Instr |= op2 << 9;
    Instr |= op3 << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEIntCompareScalar(uint32_t op1, uint32_t b4, uint32_t op2, SubRegSize size, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");

    uint32_t Instr = 0b0010'0101'0010'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= rm.Idx() << 16;
    Instr |= op1 << 10;
    Instr |= rn.Idx() << 5;
    Instr |= b4 << 4;
    Instr |= op2;
    dc32(Instr);
  }

  void SVEWriteFFR(uint32_t op0, uint32_t op1, uint32_t op2, uint32_t op3, uint32_t op4) {
    uint32_t Instr = 0b0010'0101'0010'1000'1001'0000'0000'0000;
    Instr |= op0 << 18;
    Instr |= op1 << 16;
    Instr |= op2 << 9;
    Instr |= op3 << 5;
    Instr |= op4;
    dc32(Instr);
  }

  void SVEFPUnaryOpsUnpredicated(uint32_t opc, SubRegSize size, ZRegister zd, ZRegister zn) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");

    uint32_t Instr = 0b0110'0101'0000'1000'0011'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEFPSerialReductionPredicated(uint32_t opc, SubRegSize size, VRegister vd, PRegister pg, VRegister vn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");
    LOGMAN_THROW_A_FMT(vd == vn, "vn must be the same as vd");

    uint32_t Instr = 0b0110'0101'0001'1000'0010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= vd.Idx();
    dc32(Instr);
  }

  void SVEFPCompareWithZero(uint32_t eqlt, uint32_t ne, SubRegSize size, PRegister pd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0101'0001'0000'0010'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= eqlt << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEFPMultiplyAdd(uint32_t opc, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn, ZRegister zm) {
    // NOTE: opc also includes the op0 bit (bit 15) like op0:opc, since the fields are adjacent
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0110'0101'0010'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEFPMultiplyAddIndexed(uint32_t op, SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index) {
    LOGMAN_THROW_A_FMT(IsStandardFloatSize(size), "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_A_FMT((size <= SubRegSize::i32Bit && zm <= ZReg::z7) || (size == SubRegSize::i64Bit && zm <= ZReg::z15),
                       "16-bit and 32-bit indexed variants may only use Zm between z0-z7\n"
                       "64-bit variants may only use Zm between z0-z15");

    const auto Underlying = FEXCore::ToUnderlying(size);
    const uint32_t IndexMax = (16 / (1U << Underlying)) - 1;
    LOGMAN_THROW_A_FMT(index <= IndexMax, "Index must be within 0-{}", IndexMax);

    // Can be bit 20 or 19 depending on whether or not the element size is 64-bit.
    const auto IndexShift = 19 + static_cast<uint32_t>(size == SubRegSize::i64Bit);

    uint32_t Instr = 0b0110'0100'0010'0000'0000'0000'0000'0000;
    Instr |= Underlying << 22;
    Instr |= (index & 0b1000) << 19;
    Instr |= (index & 0b0111) << IndexShift;
    Instr |= zm.Idx() << 16;
    Instr |= op << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVEFPMultiplyAddLongIndexed(uint32_t o2, uint32_t op, uint32_t T, SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm,
                                   uint32_t index) {
    LOGMAN_THROW_A_FMT(dstsize == SubRegSize::i32Bit, "Destination size must be 32-bit.");
    LOGMAN_THROW_A_FMT(index <= 7, "Index ({}) must be within [0, 7]", index);
    LOGMAN_THROW_A_FMT(zm <= ZReg::z7, "zm (z{}) must be within [z0, z7]", zm.Idx());

    uint32_t Inst = 0b0110'0100'1010'0000'0100'0000'0000'0000;
    Inst |= o2 << 22;
    Inst |= (index & 0b110) << 18;
    Inst |= zm.Idx() << 16;
    Inst |= op << 13;
    Inst |= (index & 0b001) << 11;
    Inst |= T << 10;
    Inst |= zn.Idx() << 5;
    Inst |= zda.Idx();
    dc32(Inst);
  }

  void SVEFPMultiplyAddLong(uint32_t o2, uint32_t op, uint32_t T, SubRegSize dstsize, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(dstsize == SubRegSize::i32Bit, "Destination size must be 32-bit.");

    uint32_t Instr = 0b0110'0100'1010'0000'1000'0000'0000'0000;
    Instr |= o2 << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 13;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVEFPMatrixMultiplyAccumulate(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "SubRegSize must be 32-bit or 64-bit");

    uint32_t Instr = 0b0110'0100'0010'0000'1110'0100'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zda.Idx();
    dc32(Instr);
  }

  void SVEPredicateCount(uint32_t opc, SubRegSize size, XRegister rd, PRegister pg, PRegister pn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0010'0101'0010'0000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= pn.Idx() << 5;
    Instr |= rd.Idx();

    dc32(Instr);
  }

  void SVEElementCount(uint32_t b20, uint32_t op1, SubRegSize size, ZRegister zdn, PredicatePattern pattern, uint32_t imm4) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");
    LOGMAN_THROW_A_FMT(imm4 >= 1 && imm4 <= 16, "Immediate must be between 1-16 inclusive");

    uint32_t Instr = 0b0000'0100'0010'0000'1100'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= b20 << 20;
    Instr |= (imm4 - 1) << 16;
    Instr |= op1 << 10;
    Instr |= FEXCore::ToUnderlying(pattern) << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }

  void SVEIncDecPredicateCountScalar(uint32_t op0, uint32_t op1, uint32_t opc, uint32_t b16, SubRegSize size, Register rdn, PRegister pm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0010'0101'0010'1000'1000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op0 << 18;
    Instr |= b16 << 16;
    Instr |= op1 << 11;
    Instr |= opc << 9;
    Instr |= pm.Idx() << 5;
    Instr |= rdn.Idx();
    dc32(Instr);
  }
  void SVEIncDecPredicateCountVector(uint32_t op0, uint32_t op1, uint32_t opc, uint32_t b16, SubRegSize size, ZRegister zdn, PRegister pm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Cannot use 8-bit element size");
    SVEIncDecPredicateCountScalar(op0, op1, opc, b16, size, Register {zdn.Idx()}, pm);
  }

  void SVE2IntegerPredicated(uint32_t op0, uint32_t op1, SubRegSize size, ZRegister zd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit size");
    LOGMAN_THROW_A_FMT(pg <= PReg::p7, "Can only use p0-p7 as a governing predicate");

    uint32_t Instr = 0b0100'0100'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op0 << 16; // Intentionally 16 instead of 17 to handle bit range nicer
    Instr |= op1 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2IntegerPairwiseAddAccumulateLong(uint32_t U, SubRegSize size, ZRegister zda, PRegisterMerge pg, ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "SubRegSize must be 16-bit, "
                                                                                                               "32-bit, or 64-bit");
    SVE2IntegerPredicated((0b0010 << 1) | U, 0b101, size, zda, pg, zn);
  }

  void SVE2IntegerUnaryOpsPredicated(uint32_t op0, SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn) {
    SVE2IntegerPredicated(op0, 0b101, size, zd, pg, zn);
  }

  void SVE2SaturatingRoundingBitwiseShiftLeft(uint32_t op0, SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zn needs to equal zd");
    SVE2IntegerPredicated(op0, 0b100, size, zd, pg, zm);
  }

  void SVE2IntegerHalvingPredicated(uint32_t RSU, SubRegSize size, PRegister pg, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zn needs to equal zd");
    SVE2IntegerPredicated((0b10 << 3) | RSU, 0b100, size, zd, pg, zm);
  }

  void SVEIntegerPairwiseArithmetic(uint32_t opc, uint32_t U, SubRegSize size, PRegister pg, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zn needs to equal zd");
    SVE2IntegerPredicated((0b10 << 3) | (opc << 1) | U, 0b101, size, zd, pg, zm);
  }

  void SVE2IntegerSaturatingAddSub(uint32_t opc, SubRegSize size, ZRegister zd, PRegisterMerge pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd == zn, "zn needs to equal zd");
    SVE2IntegerPredicated((0b11 << 3) | opc, 0b100, size, zd, pg, zm);
  }

  void SVEIntegerMultiplyAddUnpredicated(uint32_t op0, SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Cannot use 128-bit element size");

    uint32_t Instr = 0b0100'0100'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op0 << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVEIntegerDotProduct(uint32_t op, SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, Rotation rot) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Dot product must only use 32-bit or 64-bit element "
                                                                                 "sizes");
    SVEIntegerComplexMulAdd(op, size, zda, zn, zm, rot);
  }

  void SVEIntegerComplexMulAdd(uint32_t op, SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, Rotation rot) {
    const auto op0 = op << 2 | FEXCore::ToUnderlying(rot);
    SVEIntegerMultiplyAddUnpredicated(op0, size, zda, zn, zm);
  }

  void SVE2SaturatingMulAddInterleaved(uint32_t op0, SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "Element size may only be 16-bit, 32-bit, or 64-bit");
    SVEIntegerMultiplyAddUnpredicated(op0, size, zda, zn, zm);
  }

  void SVE2IntegerMulAddLong(uint32_t op0, SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm) {
    SVE2SaturatingMulAddInterleaved(op0, size, zda, zn, zm);
  }

  void SVE2WideningIntegerArithmetic(uint32_t op, uint32_t SUT, SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    uint32_t Instr = 0b0100'0101'0000'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 13;
    Instr |= SUT << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVE2IntegerAddSubLong(uint32_t op, uint32_t SUT, SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");
    SVE2WideningIntegerArithmetic(op, SUT, size, zd, zn, zm);
  }

  void SVE2IntegerAddSubWide(uint32_t SUT, SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");
    SVE2WideningIntegerArithmetic(0b10, SUT, size, zd, zn, zm);
  }

  void SVE2IntegerMultiplyLong(uint32_t SUT, SubRegSize size, ZRegister zd, ZRegister zn, ZRegister zm) {
    // PMULLB and PMULLT support the use of 128-bit element sizes (with the SVE2PMULL128 extension)
    if (SUT == 0b010 || SUT == 0b011) {
      LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i32Bit, "Can't use 8-bit or 32-bit element size");

      // 128-bit variant is encoded as if it were 8-bit (0b00)
      if (size == SubRegSize::i128Bit) {
        size = SubRegSize::i8Bit;
      }
    } else {
      LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i128Bit, "Can't use 8-bit or 128-bit element size");
    }

    SVE2WideningIntegerArithmetic(0b11, SUT, size, zd, zn, zm);
  }

  struct SVEEncodedImmShift {
    uint32_t tszh;
    uint32_t tszl_imm3;
  };
  // Helper for encoding shift immediates that make use of the tszh:tszl and imm3 field.
  static constexpr SVEEncodedImmShift EncodeSVEShiftImmediate(SubRegSize size, uint32_t shift, bool is_left_shift = false) {
    const uint32_t element_size = SubRegSizeInBits(size);

    if (is_left_shift) {
      LOGMAN_THROW_A_FMT(shift < element_size, "Invalid left shift value ({}). Must be within [0, {}]", shift, element_size - 1);
    } else {
      LOGMAN_THROW_A_FMT(shift > 0 && shift <= element_size, "Invalid right shift value ({}). Must be within [1, {}]", shift, element_size);
    }

    // Both left and right shifts encodes their shift as if it were
    // expanding the tszh:tszl (tsize) bits to the the left in order to accomodate
    // larger shift values. e.g. (B: tsize=0b0001, H: tsize=0b001x, etc)
    //
    // The difference is in how they're encoded. Left shifts are trivial and
    // encode as element_size_in_bits + shift, which works nicely since
    // the size will just occupy the next bit in tsize leaving the previous
    // one for encoding larger shifts.
    //
    // Right shifts instead encode it like a subtraction. e.g. A shift of 1
    // would encode like (S: tsize=0b0111 imm3=0b111, where 64 - 1 = 63, etc).
    // so the more lower in value the bits are set, the larger the shift.
    const uint32_t encoded_shift = is_left_shift ? element_size + shift : (2 * element_size) - shift;

    return {
      .tszh = encoded_shift >> 5,
      .tszl_imm3 = encoded_shift & 0b11111,
    };
  }

  // Alias that returns the equivalently sized unsigned type for a floating-point type T.
  template<typename T>
  requires (std::is_same_v<T, float> || std::is_same_v<T, double>)
  using FloatToEquivalentUInt = std::conditional_t<std::is_same_v<T, float>, uint32_t, uint64_t>;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  // Determines if a floating-point value is capable of being converted
  // into an 8-bit immediate. See pseudocode definition of VFPExpandImm
  // in ARM A-profile reference manual for a general overview of how this was derived.
  template<typename T>
  requires (std::is_same_v<T, float> || std::is_same_v<T, double>)
  [[nodiscard]]
  static bool IsValidFPValueForImm8(T value) {
    const uint64_t bits = std::bit_cast<FloatToEquivalentUInt<T>>(value);
    const uint64_t datasize_idx = FEXCore::ilog2(sizeof(T)) - 1;

    static constexpr std::array mantissa_masks {
      0x00000000'0000003FULL, // half (bits [5:0])
      0x00000000'0007FFFFULL, // single (bits [18:0])
      0x0000FFFF'FFFFFFFFULL, // double (bits [47:0])
    };
    const auto mantissa_mask = mantissa_masks[datasize_idx];

    // Relevant mantissa bits must be set to zero
    if ((bits & mantissa_mask) != 0) {
      return false;
    }

    static constexpr std::array exponent_masks {
      0x00000000'00003000ULL, // half (bits [13:12])
      0x00000000'3E000000ULL, // single (bits [29:25])
      0x3FC00000'00000000ULL, // double (bits [61:54])
    };
    const auto exponent_mask = exponent_masks[datasize_idx];
    const auto masked_exponent = bits & exponent_mask;

    // Relevant exponent bits must either be all set or all cleared.
    if (masked_exponent != 0 && masked_exponent != exponent_mask) {
      return false;
    }

    // The two bits before the sign bit must be inverses of each other.
    const auto datasize = 8ULL * sizeof(T);
    const auto inverse = bits ^ (bits << 1);
    const auto inverse_mask = 1ULL << (datasize - 2);
    if ((inverse & inverse_mask) == 0) {
      return false;
    }

    return true;
  }
#endif

protected:
  static uint32_t FP32ToImm8(float value) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_THROW_A_FMT(IsValidFPValueForImm8(value), "Value ({}) cannot be encoded into an 8-bit immediate", value);
#endif

    const auto bits = std::bit_cast<uint32_t>(value);
    const auto sign = (bits & 0x80000000) >> 24;
    const auto expb2 = (bits & 0x20000000) >> 23;
    const auto b5_to_0 = (bits >> 19) & 0x3F;

    return sign | expb2 | b5_to_0;
  }

  static uint32_t FP64ToImm8(double value) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_THROW_A_FMT(IsValidFPValueForImm8(value), "Value ({}) cannot be encoded into an 8-bit immediate", value);
#endif

    const auto bits = std::bit_cast<uint64_t>(value);
    const auto sign = (bits & 0x80000000'00000000) >> 56;
    const auto expb2 = (bits & 0x20000000'00000000) >> 55;
    const auto b5_to_0 = (bits >> 48) & 0x3F;

    return static_cast<uint32_t>(sign | expb2 | b5_to_0);
  }

private:
  // Handling for signed 8-bit immediate shifts (e.g. in cpy/dup)
  struct HandledSImm8Shift {
    int32_t imm;
    uint32_t is_shift;
  };
  static constexpr HandledSImm8Shift HandleSVESImm8Shift(SubRegSize size, int32_t imm) {
    const int32_t imm8_limit = 128;
    const bool is_int8_imm = -imm8_limit <= imm && imm < imm8_limit;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(is_int8_imm, "Can't perform LSL #8 shift on 8-bit elements.");
    }

    uint32_t shift = 0;
    if (!is_int8_imm) {
      const int32_t imm16_limit = 32768;
      const bool is_int16_imm = -imm16_limit <= imm && imm < imm16_limit;

      LOGMAN_THROW_A_FMT(is_int16_imm, "Immediate ({}) must be a 16-bit value within [-32768, 32512]", imm);
      LOGMAN_THROW_A_FMT((imm % 256) == 0, "Immediate ({}) must be a multiple of 256", imm);

      imm /= 256;
      shift = 1;
    }

    return {
      .imm = imm,
      .is_shift = shift,
    };
  }

#ifndef INCLUDED_BY_EMITTER
}; // struct LoadstoreEmitterOps
} // namespace ARMEmitter
#endif
