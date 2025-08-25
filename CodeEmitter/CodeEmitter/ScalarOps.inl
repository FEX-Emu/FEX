// SPDX-License-Identifier: MIT
/* Scalar instruction emitters.
 *
 * These contain instruction emitters for scalar ASIMD operations explicitly.
 * Some of these emitter arguments might seem a bit strange at first glance,
 * but is because ARM's instruction encodings for these instructions are a hot mess.
 *
 * Specifically FP16 was an afterthought for these scalar operations, using a `ScalarRegSize` with
 * 16-bit wouldn't encode an FP16 instruction because they are a different instruction class instead.
 *
 * Most FP16 operations instead have their own freestanding implementation using `HRegister` arguments.
 *
 * Meanwhile other FP32 and FP64 instructions will use `ScalarRegSize`, supporting both those sizes.
 *
 * For Scalar integer operations, these instructions will mostly support all `ScalarRegSize` operations.
 * Exceptions to this rule will have asserts in the emitter implementation when misused.
 *
 */

#pragma once
#ifndef INCLUDED_BY_EMITTER
#include <CodeEmitter/Emitter.h>
namespace ARMEmitter {
struct EmitterOps : Emitter {
#endif

public:
  // Advanced SIMD scalar copy
  void dup(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Index) {
    const uint32_t SizeImm = FEXCore::ToUnderlying(size);
    const uint32_t IndexShift = SizeImm + 1;
    const uint32_t ElementSize = 1U << SizeImm;
    const uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(1, 1, imm5, 0b0000, rd, rn);
  }

  void mov(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Index) {
    dup(size, rd, rn, Index);
  }

  // Advanced SIMD scalar three same FP16
  void fmulx(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(0, 0, 0b011, rm, rn, rd);
  }
  void fcmeq(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(0, 0, 0b100, rm, rn, rd);
  }
  void frecps(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(0, 0, 0b111, rm, rn, rd);
  }
  void frsqrts(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(0, 1, 0b111, rm, rn, rd);
  }
  void fcmge(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(1, 0, 0b100, rm, rn, rd);
  }
  void facge(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(1, 0, 0b101, rm, rn, rd);
  }
  void fabd(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(1, 1, 0b010, rm, rn, rd);
  }
  void fcmgt(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(1, 1, 0b100, rm, rn, rd);
  }
  void facgt(HRegister rd, HRegister rn, HRegister rm) {
    ASIMDScalarThreeSameFP16(1, 1, 0b101, rm, rn, rd);
  }

  // Advanced SIMD scalar two-register miscellaneous FP16
  void fcvtns(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 0, 0b11010, rn, rd);
  }
  void fcvtms(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 0, 0b11011, rn, rd);
  }
  void fcvtas(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 0, 0b11100, rn, rd);
  }
  void scvtf(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 0, 0b11101, rn, rd);
  }
  void fcmgt(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b01100, rn, rd);
  }
  void fcmeq(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b01101, rn, rd);
  }
  void fcmlt(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b01110, rn, rd);
  }
  void fcvtps(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b11010, rn, rd);
  }
  void fcvtzs(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b11011, rn, rd);
  }
  void frecpe(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b11101, rn, rd);
  }
  void frecpx(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(0, 1, 0b11111, rn, rd);
  }
  void fcvtnu(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 0, 0b11010, rn, rd);
  }
  void fcvtmu(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 0, 0b11011, rn, rd);
  }
  void fcvtau(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 0, 0b11100, rn, rd);
  }
  void ucvtf(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 0, 0b11101, rn, rd);
  }
  void fcmge(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 1, 0b01100, rn, rd);
  }
  void fcmle(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 1, 0b01101, rn, rd);
  }
  void fcvtpu(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 1, 0b11010, rn, rd);
  }
  void fcvtzu(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 1, 0b11011, rn, rd);
  }
  void frsqrte(HRegister rd, HRegister rn) {
    ASIMDScalarTwoRegMiscFP16(1, 1, 0b11101, rn, rd);
  }

  // Advanced SIMD scalar three same extra
  void sqrdmlah(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i16Bit || size == ScalarRegSize::i32Bit, "Only supports 16/32-bit");
    ASIMDScalarThreeSameExtra(1, size, 0b0000, rm, rn, rd);
  }
  void sqrdmlsh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i16Bit || size == ScalarRegSize::i32Bit, "Only supports 16/32-bit");
    ASIMDScalarThreeSameExtra(1, size, 0b0001, rm, rn, rd);
  }

  // Advanced SIMD scalar two-register miscellaneous
  void suqadd(ScalarRegSize size, VRegister rd, VRegister rn) {
    ASIMDScalar2RegMisc(0, 0, size, 0b00011, rd, rn);
  }
  void sqabs(ScalarRegSize size, VRegister rd, VRegister rn) {
    ASIMDScalar2RegMisc(0, 0, size, 0b00111, rd, rn);
  }

  ///< Comparison against 0.0
  void cmgt(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 0, size, 0b01000, rd, rn);
  }
  ///< Comparison against 0.0
  void cmeq(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 0, size, 0b01001, rd, rn);
  }

  ///< Comparison against 0.0
  void cmlt(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 0, size, 0b01010, rd, rn);
  }
  void abs(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 0, size, 0b01011, rd, rn);
  }
  ///< size is destination size.
  void sqxtn(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "64-bit destination not supported");
    ASIMDScalar2RegMisc(0, 0, size, 0b10100, rd, rn);
  }

  void fcvtns(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 0, ConvertedSize, 0b11010, rd, rn);
  }
  void fcvtms(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 0, ConvertedSize, 0b11011, rd, rn);
  }
  void fcvtas(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 0, ConvertedSize, 0b11100, rd, rn);
  }
  void scvtf(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 0, ConvertedSize, 0b11101, rd, rn);
  }

  ///< Comparison against 0.0
  void fcmgt(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float compare");
    ASIMDScalar2RegMisc(0, 0, size, 0b01100, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmeq(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float compare");
    ASIMDScalar2RegMisc(0, 0, size, 0b01101, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmlt(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float compare");

    ASIMDScalar2RegMisc(0, 0, size, 0b01110, rd, rn);
  }
  void fcvtps(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 0, size, 0b11010, rd, rn);
  }
  void fcvtzs(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 0, size, 0b11011, rd, rn);
  }
  void frecpe(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 0, size, 0b11101, rd, rn);
  }
  void frecpx(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 0, size, 0b11111, rd, rn);
  }
  void usqadd(ScalarRegSize size, VRegister rd, VRegister rn) {
    ASIMDScalar2RegMisc(0, 1, size, 0b00011, rd, rn);
  }
  void sqneg(ScalarRegSize size, VRegister rd, VRegister rn) {
    ASIMDScalar2RegMisc(0, 1, size, 0b00111, rd, rn);
  }
  ///< Comparison against 0.0
  void cmge(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 1, size, 0b01000, rd, rn);
  }
  ///< Comparison against 0.0
  void cmle(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 1, size, 0b01001, rd, rn);
  }
  void neg(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMDScalar2RegMisc(0, 1, size, 0b01011, rd, rn);
  }
  ///< size is destination.
  void sqxtun(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "64-bit destination not supported");
    ASIMDScalar2RegMisc(0, 1, size, 0b10010, rd, rn);
  }
  ///< size is destination.
  void uqxtn(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "64-bit destination not supported");
    ASIMDScalar2RegMisc(0, 1, size, 0b10100, rd, rn);
  }
  ///< size is destination.
  void fcvtxn(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMDScalar2RegMisc(0, 1, ScalarRegSize::i16Bit, 0b10110, rd, rn);
  }
  void fcvtnu(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 1, ConvertedSize, 0b11010, rd, rn);
  }
  void fcvtmu(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 1, ConvertedSize, 0b11011, rd, rn);
  }
  void fcvtau(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 1, ConvertedSize, 0b11100, rd, rn);
  }
  void ucvtf(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(0, 1, ConvertedSize, 0b11101, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmge(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 1, size, 0b01100, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmle(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 1, size, 0b01101, rd, rn);
  }
  void fcvtpu(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 1, size, 0b11010, rd, rn);
  }
  void fcvtzu(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 1, size, 0b11011, rd, rn);
  }
  void frsqrte(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    ASIMDScalar2RegMisc(0, 1, size, 0b11101, rd, rn);
  }
  // Advanced SIMD scalar pairwise
  void addp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Invalid size selected for addp");
    ASIMDScalar2RegMisc(1, 0, size, 0b11011, rd, rn);
  }

  void fmaxnmp(HRegister rd, HRegister rn) {
    ASIMDScalar2RegMisc(1, 0, ScalarRegSize::i8Bit, 0b01100, rd.V(), rn.V());
  }
  void faddp(HRegister rd, HRegister rn) {
    ASIMDScalar2RegMisc(1, 0, ScalarRegSize::i8Bit, 0b01101, rd.V(), rn.V());
  }
  void fmaxp(HRegister rd, HRegister rn) {
    ASIMDScalar2RegMisc(1, 0, ScalarRegSize::i8Bit, 0b01111, rd.V(), rn.V());
  }
  void fminnmp(HRegister rd, HRegister rn) {
    ASIMDScalar2RegMisc(1, 0, ScalarRegSize::i32Bit, 0b01100, rd.V(), rn.V());
  }
  void fminp(HRegister rd, HRegister rn) {
    ASIMDScalar2RegMisc(1, 0, ScalarRegSize::i32Bit, 0b01111, rd.V(), rn.V());
  }

  void fmaxnmp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(1, 1, ConvertedSize, 0b01100, rd, rn);
  }
  void faddp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(1, 1, ConvertedSize, 0b01101, rd, rn);
  }
  void fmaxp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(1, 1, ConvertedSize, 0b01111, rd, rn);
  }
  void fminnmp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMDScalar2RegMisc(1, 1, size, 0b01100, rd, rn);
  }
  void fminp(ScalarRegSize size, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMDScalar2RegMisc(1, 1, size, 0b01111, rd, rn);
  }
  // Advanced SIMD scalar three different
  ///< size is destination.
  void sqdmlal(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i32Bit : ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(0, ConvertedSize, 0b1001, rd, rn, rm);
  }
  ///< size is destination.
  void sqdmlsl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i32Bit : ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(0, ConvertedSize, 0b1011, rd, rn, rm);
  }

  ///< size is destination.
  void sqdmull(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i32Bit : ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(0, ConvertedSize, 0b1101, rd, rn, rm);
  }
  // Advanced SIMD scalar three same
  void sqadd(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(0, size, 0b00001, rd, rn, rm);
  }
  void sqsub(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(0, size, 0b00101, rd, rn, rm);
  }
  void cmgt(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b00110, rd, rn, rm);
  }
  void cmge(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b00111, rd, rn, rm);
  }
  void sshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b01000, rd, rn, rm);
  }
  void sqshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(0, size, 0b01001, rd, rn, rm);
  }
  void srshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b01010, rd, rn, rm);
  }
  void sqrshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(0, size, 0b01011, rd, rn, rm);
  }
  void add(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b10000, rd, rn, rm);
  }
  void cmtst(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(0, size, 0b10001, rd, rn, rm);
  }
  void sqdmulh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i32Bit || size == ScalarRegSize::i16Bit, "Invalid size");
    ASIMD3RegSame(0, size, 0b10110, rd, rn, rm);
  }
  void fmulx(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMD3RegSame(0, ConvertedSize, 0b11011, rd, rn, rm);
  }
  void fcmeq(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMD3RegSame(0, ConvertedSize, 0b11100, rd, rn, rm);
  }
  void frecps(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMD3RegSame(0, ConvertedSize, 0b11111, rd, rn, rm);
  }
  void frsqrts(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMD3RegSame(0, size, 0b11111, rd, rn, rm);
  }
  void uqadd(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(1, size, 0b00001, rd, rn, rm);
  }
  void uqsub(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(1, size, 0b00101, rd, rn, rm);
  }
  void cmhi(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b00110, rd, rn, rm);
  }
  void cmhs(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b00111, rd, rn, rm);
  }
  void ushl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b01000, rd, rn, rm);
  }
  void uqshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(1, size, 0b01001, rd, rn, rm);
  }
  void urshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b01010, rd, rn, rm);
  }
  void uqrshl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    ASIMD3RegSame(1, size, 0b01011, rd, rn, rm);
  }
  void sub(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b10000, rd, rn, rm);
  }
  void cmeq(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit, "Only supports 64-bit");
    ASIMD3RegSame(1, size, 0b10001, rd, rn, rm);
  }
  void sqrdmulh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i32Bit || size == ScalarRegSize::i16Bit, "Invalid size");
    ASIMD3RegSame(1, size, 0b10110, rd, rn, rm);
  }
  void fcmge(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMD3RegSame(1, ConvertedSize, 0b11100, rd, rn, rm);
  }
  void facge(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");

    const ScalarRegSize ConvertedSize = size == ScalarRegSize::i64Bit ? ScalarRegSize::i16Bit : ScalarRegSize::i8Bit;

    ASIMD3RegSame(1, ConvertedSize, 0b11101, rd, rn, rm);
  }
  void fabd(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMD3RegSame(1, size, 0b11010, rd, rn, rm);
  }
  void fcmgt(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMD3RegSame(1, size, 0b11100, rd, rn, rm);
  }
  void facgt(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    ASIMD3RegSame(1, size, 0b11101, rd, rn, rm);
  }
  // Advanced SIMD scalar shift by immediate
  void sshr(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b00000, rd, rn);
  }
  void ssra(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b00010, rd, rn);
  }
  void srshr(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b00100, rd, rn);
  }
  void srsra(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b00110, rd, rn);
  }
  void shl(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b01010, rd, rn);
  }
  void sqshl(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b01110, rd, rn);
  }
  ///< size is destination
  void sqshrn(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrn");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b10010, rd, rn);
  }
  void sqrshrn(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrn");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(0, immh, immb, 0b10011, rd, rn);
  }
  // TODO: SCVTF, FCVTZS
  void ushr(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b00000, rd, rn);
  }
  void usra(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b00010, rd, rn);
  }
  void urshr(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b00100, rd, rn);
  }
  void ursra(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b00110, rd, rn);
  }
  void sri(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b01000, rd, rn);
  }
  void sli(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b01010, rd, rn);
  }
  void sqshlu(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b01100, rd, rn);
  }
  void uqshl(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b01110, rd, rn);
  }
  ///< size is destination.
  void sqshrun(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrun");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b10000, rd, rn);
  }
  ///< size is destination.
  void sqrshrun(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b10001, rd, rn);
  }
  ///< size is destination.
  void uqshrn(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b10010, rd, rn);
  }
  ///< size is destination.
  void uqrshrn(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(1, immh, immb, 0b10011, rd, rn);
  }

  // TODO: UCVTF, FCVTZU

  // Advanced SIMD scalar x indexed element
  void sqdmlal(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(0, size, 0b0011, rm, rn, rd, index);
  }
  void sqdmlsl(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(0, size, 0b0111, rm, rn, rd, index);
  }
  void sqdmull(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(0, size, 0b1011, rm, rn, rd, index);
  }
  void sqdmulh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(0, size, 0b1100, rm, rn, rd, index);
  }
  void sqrdmulh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(0, size, 0b1101, rm, rn, rd, index);
  }
  void fmla(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    ASIMDScalarXIndexedElement(0, size, 0b0001, rm, rn, rd, index);
  }
  void fmls(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    ASIMDScalarXIndexedElement(0, size, 0b0101, rm, rn, rd, index);
  }
  void fmul(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    ASIMDScalarXIndexedElement(0, size, 0b1001, rm, rn, rd, index);
  }
  void sqrdmlah(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(1, size, 0b1101, rm, rn, rd, index);
  }
  void sqrdmlsh(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i64Bit, "Scalar size must not be 64-bit");
    ASIMDScalarXIndexedElement(1, size, 0b1111, rm, rn, rd, index);
  }
  void fmulx(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, uint32_t index) {
    ASIMDScalarXIndexedElement(1, size, 0b1001, rm, rn, rd, index);
  }

  // Floating-point data-processing (1 source)
  void fmov(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b000000, rd, rn);
  }
  void fabs(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b000001, rd, rn);
  }
  void fneg(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b000010, rd, rn);
  }
  void fsqrt(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b000011, rd, rn);
  }
  void frintn(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001000, rd, rn);
  }
  void frintp(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001001, rd, rn);
  }
  void frintm(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001010, rd, rn);
  }
  void frintz(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001011, rd, rn);
  }
  void frinta(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001100, rd, rn);
  }
  void frintx(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001110, rd, rn);
  }
  void frinti(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b001111, rd, rn);
  }
  void frint32z(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b010000, rd, rn);
  }
  void frint32x(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b010001, rd, rn);
  }
  void frint64z(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b010010, rd, rn);
  }
  void frint64x(ScalarRegSize size, VRegister rd, VRegister rn) {
    Float1Source(size, 0, 0, 0b010011, rd, rn);
  }

  void fmov(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000000, rd.V(), rn.V());
  }
  void fabs(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000001, rd.V(), rn.V());
  }
  void fneg(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000011, rd.V(), rn.V());
  }
  void fcvt(DRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000101, rd.V(), rn.V());
  }
  void fcvt(HRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b000111, rd.V(), rn.V());
  }
  void frintn(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001000, rd.V(), rn.V());
  }
  void frintp(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001001, rd.V(), rn.V());
  }
  void frintm(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001010, rd.V(), rn.V());
  }
  void frintz(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001011, rd.V(), rn.V());
  }
  void frinta(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001100, rd.V(), rn.V());
  }
  void frintx(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001110, rd.V(), rn.V());
  }
  void frinti(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b001111, rd.V(), rn.V());
  }
  void frint32z(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b010000, rd.V(), rn.V());
  }
  void frint32x(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b010001, rd.V(), rn.V());
  }
  void frint64z(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b010010, rd.V(), rn.V());
  }
  void frint64x(SRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b00, 0b010011, rd.V(), rn.V());
  }

  void fmov(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000000, rd.V(), rn.V());
  }
  void fabs(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000001, rd.V(), rn.V());
  }
  void fneg(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000011, rd.V(), rn.V());
  }
  void fcvt(SRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000100, rd.V(), rn.V());
  }
  void bfcvt(HRegister rd, SRegister rn) {
    Float1Source(0, 0, 0b01, 0b000110, rd.V(), rn.V());
  }
  void fcvt(HRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b000111, rd.V(), rn.V());
  }
  void frintn(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001000, rd.V(), rn.V());
  }
  void frintp(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001001, rd.V(), rn.V());
  }
  void frintm(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001010, rd.V(), rn.V());
  }
  void frintz(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001011, rd.V(), rn.V());
  }
  void frinta(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001100, rd.V(), rn.V());
  }
  void frintx(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001110, rd.V(), rn.V());
  }
  void frinti(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b001111, rd.V(), rn.V());
  }
  void frint32z(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b010000, rd.V(), rn.V());
  }
  void frint32x(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b010001, rd.V(), rn.V());
  }
  void frint64z(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b010010, rd.V(), rn.V());
  }
  void frint64x(DRegister rd, DRegister rn) {
    Float1Source(0, 0, 0b01, 0b010011, rd.V(), rn.V());
  }

  void fmov(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000000, rd.V(), rn.V());
  }
  void fabs(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000001, rd.V(), rn.V());
  }
  void fneg(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000011, rd.V(), rn.V());
  }
  void fcvt(SRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000100, rd.V(), rn.V());
  }
  void fcvt(DRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b000101, rd.V(), rn.V());
  }
  void frintn(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001000, rd.V(), rn.V());
  }
  void frintp(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001001, rd.V(), rn.V());
  }
  void frintm(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001010, rd.V(), rn.V());
  }
  void frintz(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001011, rd.V(), rn.V());
  }
  void frinta(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001100, rd.V(), rn.V());
  }
  void frintx(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001110, rd.V(), rn.V());
  }
  void frinti(HRegister rd, HRegister rn) {
    Float1Source(0, 0, 0b11, 0b001111, rd.V(), rn.V());
  }

  // Floating-point compare
  void fcmp(ScalarRegSize Size, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(Size != ScalarRegSize::i8Bit, "8-bit destination not supported");

    const auto ConvertedSize = Size == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
                               Size == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
                               Size == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 :
                                                                           0;

    FloatCompare(0, 0, ConvertedSize, 0b00, 0b00000, rn, rm);
  }

  void fcmp(SRegister rn, SRegister rm) {
    FloatCompare(0, 0, 0b00, 0b00, 0b00000, rn.V(), rm.V());
  }
  ///< Compare to #0.0
  void fcmp(SRegister rn) {
    FloatCompare(0, 0, 0b00, 0b00, 0b01000, rn.V(), VReg::v0);
  }
  void fcmpe(SRegister rn, SRegister rm) {
    FloatCompare(0, 0, 0b00, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(SRegister rn) {
    FloatCompare(0, 0, 0b00, 0b00, 0b11000, rn.V(), VReg::v0);
  }
  void fcmp(DRegister rn, DRegister rm) {
    FloatCompare(0, 0, 0b01, 0b00, 0b00000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmp(DRegister rn) {
    FloatCompare(0, 0, 0b01, 0b00, 0b01000, rn.V(), VReg::v0);
  }
  void fcmpe(DRegister rn, DRegister rm) {
    FloatCompare(0, 0, 0b01, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(DRegister rn) {
    FloatCompare(0, 0, 0b01, 0b00, 0b11000, rn.V(), VReg::v0);
  }
  void fcmp(HRegister rn, HRegister rm) {
    FloatCompare(0, 0, 0b11, 0b00, 0b00000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmp(HRegister rn) {
    FloatCompare(0, 0, 0b11, 0b00, 0b01000, rn.V(), VReg::v0);
  }
  void fcmpe(HRegister rn, HRegister rm) {
    FloatCompare(0, 0, 0b11, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(HRegister rn) {
    FloatCompare(0, 0, 0b11, 0b00, 0b11000, rn.V(), VReg::v0);
  }

  // Floating-point immediate
  void fmov(ARMEmitter::ScalarRegSize size, ARMEmitter::VRegister rd, float Value) {
    uint32_t M = 0;
    uint32_t S = 0;
    uint32_t ptype;
    uint32_t imm8;
    uint32_t imm5 = 0b0'0000;
    if (size == ARMEmitter::ScalarRegSize::i16Bit) {
      LOGMAN_MSG_A_FMT("Unsupported");
      FEX_UNREACHABLE;
    } else if (size == ARMEmitter::ScalarRegSize::i32Bit) {
      ptype = 0b00;
      imm8 = FP32ToImm8(Value);
    } else if (size == ARMEmitter::ScalarRegSize::i64Bit) {
      ptype = 0b01;
      imm8 = FP64ToImm8(Value);
    } else {
      FEX_UNREACHABLE;
    }

    FloatScalarImmediate(M, S, ptype, imm8, imm5, rd);
  }

  void FloatScalarImmediate(uint32_t M, uint32_t S, uint32_t ptype, uint32_t imm8, uint32_t imm5, ARMEmitter::VRegister rd) {
    constexpr uint32_t Op = 0b0001'1110'0010'0000'0001'00 << 10;
    uint32_t Instr = Op;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= imm8 << 13;
    Instr |= imm5 << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Floating-point conditional compare
  void fccmp(SRegister rn, SRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b00, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(SRegister rn, SRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b00, 0b1, rn.V(), rm.V(), flags, Cond);
  }
  void fccmp(DRegister rn, DRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b01, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(DRegister rn, DRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b01, 0b1, rn.V(), rm.V(), flags, Cond);
  }
  void fccmp(HRegister rn, HRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b11, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(HRegister rn, HRegister rm, StatusFlags flags, Condition Cond) {
    FloatConditionalCompare(0, 0, 0b11, 0b1, rn.V(), rm.V(), flags, Cond);
  }

  // Floating-point data-processing (2 source)
  void fmul(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0000, rd, rn, rm);
  }
  void fdiv(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0001, rd, rn, rm);
  }
  void fadd(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0010, rd, rn, rm);
  }
  void fsub(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0011, rd, rn, rm);
  }
  void fmax(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0100, rd, rn, rm);
  }
  void fmin(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0101, rd, rn, rm);
  }
  void fmaxnm(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0110, rd, rn, rm);
  }
  void fminnm(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b0111, rd, rn, rm);
  }
  void fnmul(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm) {
    Float2Source(size, 0, 0, 0b1000, rd, rn, rm);
  }

  void fmul(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(SRegister rd, SRegister rn, SRegister rm) {
    Float2Source(0, 0, 0b00, 0b1000, rd.V(), rn.V(), rm.V());
  }

  void fmul(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(DRegister rd, DRegister rn, DRegister rm) {
    Float2Source(0, 0, 0b01, 0b1000, rd.V(), rn.V(), rm.V());
  }

  void fmul(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(HRegister rd, HRegister rn, HRegister rm) {
    Float2Source(0, 0, 0b11, 0b1000, rd.V(), rn.V(), rm.V());
  }

  // Floating-point conditional select
  void fcsel(ScalarRegSize size, VRegister rd, VRegister rn, VRegister rm, Condition Cond) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i16Bit || size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit,
                       "Invalid size selected for {}", __func__);

    const uint32_t ConvertedSize = size == ScalarRegSize::i64Bit ? 0b01 : size == ScalarRegSize::i32Bit ? 0b00 : 0b11;

    FloatConditionalSelect(0, 0, ConvertedSize, rd, rn, rm, Cond);
  }

  void fcsel(SRegister rd, SRegister rn, SRegister rm, Condition Cond) {
    FloatConditionalSelect(0, 0, 0b00, rd.V(), rn.V(), rm.V(), Cond);
  }
  void fcsel(DRegister rd, DRegister rn, DRegister rm, Condition Cond) {
    FloatConditionalSelect(0, 0, 0b01, rd.V(), rn.V(), rm.V(), Cond);
  }
  void fcsel(HRegister rd, HRegister rn, HRegister rm, Condition Cond) {
    FloatConditionalSelect(0, 0, 0b11, rd.V(), rn.V(), rm.V(), Cond);
  }

  // Floating-point data-processing (3 source)
  void fmadd(SRegister rd, SRegister rn, SRegister rm, SRegister ra) {
    Float3Source(0, 0, 0b00, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(SRegister rd, SRegister rn, SRegister rm, SRegister ra) {
    Float3Source(0, 0, 0b00, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(SRegister rd, SRegister rn, SRegister rm, SRegister ra) {
    Float3Source(0, 0, 0b00, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(SRegister rd, SRegister rn, SRegister rm, SRegister ra) {
    Float3Source(0, 0, 0b00, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

  void fmadd(DRegister rd, DRegister rn, DRegister rm, DRegister ra) {
    Float3Source(0, 0, 0b01, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(DRegister rd, DRegister rn, DRegister rm, DRegister ra) {
    Float3Source(0, 0, 0b01, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(DRegister rd, DRegister rn, DRegister rm, DRegister ra) {
    Float3Source(0, 0, 0b01, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(DRegister rd, DRegister rn, DRegister rm, DRegister ra) {
    Float3Source(0, 0, 0b01, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

  void fmadd(HRegister rd, HRegister rn, HRegister rm, HRegister ra) {
    Float3Source(0, 0, 0b11, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(HRegister rd, HRegister rn, HRegister rm, HRegister ra) {
    Float3Source(0, 0, 0b11, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(HRegister rd, HRegister rn, HRegister rm, HRegister ra) {
    Float3Source(0, 0, 0b11, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(HRegister rd, HRegister rn, HRegister rm, HRegister ra) {
    Float3Source(0, 0, 0b11, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

private:
  // Advanced SIMD scalar copy
  void ASIMDScalarCopy(uint32_t Q, uint32_t b28, uint32_t imm5, uint32_t imm4, VRegister rd, VRegister rn) {
    uint32_t Instr = 0b0000'1110'0000'0000'0000'01U << 10;
    Instr |= Q << 30;
    Instr |= b28 << 28;
    Instr |= imm5 << 16;
    Instr |= imm4 << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD scalar three same FP16
  void ASIMDScalarThreeSameFP16(uint32_t U, uint32_t a, uint32_t opcode, HRegister rm, HRegister rn, HRegister rd) {
    uint32_t Instr = 0b0101'1110'0100'0000'0000'0100'0000'0000;

    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }
  // Advanced SIMD scalar two-register miscellaneous FP16
  void ASIMDScalarTwoRegMiscFP16(uint32_t U, uint32_t a, uint32_t opcode, HRegister rn, HRegister rd) {
    uint32_t Instr = 0b0101'1110'0111'1000'0000'1000'0000'0000;

    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= opcode << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Advanced SIMD scalar three same extra
  void ASIMDScalarThreeSameExtra(uint32_t U, ScalarRegSize size, uint32_t opcode, VRegister rm, VRegister rn, VRegister rd) {
    uint32_t Instr = 0b0101'1110'0000'0000'1000'0100'0000'0000;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Advanced SIMD scalar two-register miscellaneous
  void ASIMDScalar2RegMisc(uint32_t b20, uint32_t U, ScalarRegSize size, uint32_t opcode, VRegister rd, VRegister rn) {
    uint32_t Instr = 0b0101'1110'0010'0000'0000'1000'0000'0000;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= b20 << 20;
    Instr |= opcode << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Advanced SIMD scalar three different
  void ASIMD3RegDifferent(uint32_t U, ScalarRegSize size, uint32_t opcode, VRegister rd, VRegister rn, VRegister rm) {
    uint32_t Instr = 0b0101'1110'0010'0000'0000'0000'0000'0000;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }
  // Advanced SIMD scalar three same
  void ASIMD3RegSame(uint32_t U, ScalarRegSize size, uint32_t opcode, VRegister rd, VRegister rn, VRegister rm) {
    uint32_t Instr = 0b0101'1110'0010'0000'0000'0100'0000'0000;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }
  // Advanced SIMD scalar shift by immediate
  void ASIMDScalarShiftByImm(uint32_t U, uint32_t immh, uint32_t immb, uint32_t opcode, VRegister rd, VRegister rn) {
    uint32_t Instr = 0b0101'1111'0000'0000'0000'0100'0000'0000;

    Instr |= U << 29;
    Instr |= immh << 19;
    Instr |= immb << 16;
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD scalar x indexed element
  void ASIMDScalarXIndexedElement(uint32_t U, ScalarRegSize size, uint32_t opcode, VRegister rm, VRegister rn, VRegister rd, uint32_t index) {
    LOGMAN_THROW_A_FMT(size != ScalarRegSize::i8Bit, "Scalar size must not be 8-bit");

    const auto invalid_bound = 16U >> FEXCore::ToUnderlying(size);
    LOGMAN_THROW_A_FMT(index < invalid_bound, "Index ({}) must be within [0-{}]", index, invalid_bound - 1);

    uint32_t Instr = 0b0101'1111'0000'0000'0000'0000'0000'0000;

    // FMUL/FMLA/FMLS indexed variants deal with size differently.
    if (opcode == 0b0001 || opcode == 0b0101 || opcode == 0b1001) {
      // Unlike other instructions in the group, 16-bit is encoded as zero
      // and 32/64-bit are encoded with the top bit always set to one.
      if (size != ScalarRegSize::i16Bit) {
        Instr |= (0b10 | (FEXCore::ToUnderlying(size) & 1)) << 22;
      }
    } else {
      Instr |= FEXCore::ToUnderlying(size) << 22;
    }

    uint32_t H = 0;
    uint32_t LM = 0;
    if (size == ScalarRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm <= VReg::v15, "rm ({}) must be within [v0-v15]", rm.Idx());
      H = (index >> 2) & 1;
      LM = index & 0b11;
    } else if (size == ScalarRegSize::i32Bit) {
      H = (index >> 1) & 1;
      LM = (index & 0b01) << 1;
    } else {
      H = index & 1;
    }

    Instr |= U << 29;
    Instr |= LM << 20;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 12;
    Instr |= H << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Floating-point data-processing (1 source)
  void Float1Source(uint32_t M, uint32_t S, uint32_t ptype, uint32_t opcode, VRegister rd, VRegister rn) {
    uint32_t Instr = 0b0001'1110'0010'0000'0100'0000'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= opcode << 15;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }
  void Float1Source(ScalarRegSize size, uint32_t M, uint32_t S, uint32_t opcode, VRegister rd, VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i16Bit || size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit,
                       "Invalid size selected for {}", __func__);

    const uint32_t ConvertedSize = size == ScalarRegSize::i64Bit ? 0b01 : size == ScalarRegSize::i32Bit ? 0b00 : 0b11;

    Float1Source(M, S, ConvertedSize, opcode, rd, rn);
  }

  // Floating-point compare
  void FloatCompare(uint32_t M, uint32_t S, uint32_t ftype, uint32_t op, uint32_t opcode2, VRegister rn, VRegister rm) {
    uint32_t Instr = 0b0001'1110'0010'0000'0010'0000'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ftype << 22;
    Instr |= Encode_rm(rm);
    Instr |= op << 14;
    Instr |= Encode_rn(rn);
    Instr |= opcode2;

    dc32(Instr);
  }
  // Floating-point immediate
  // XXX:
  // Floating-point conditional compare
  void FloatConditionalCompare(uint32_t M, uint32_t S, uint32_t ptype, uint32_t op, VRegister rn, VRegister rm, StatusFlags flags, Condition Cond) {
    uint32_t Instr = 0b0001'1110'0010'0000'0000'0100'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= Encode_rm(rm);
    Instr |= FEXCore::ToUnderlying(Cond) << 12;
    Instr |= Encode_rn(rn);
    Instr |= op << 4;
    Instr |= FEXCore::ToUnderlying(flags);

    dc32(Instr);
  }
  // Floating-point data-processing (2 source)

  void Float2Source(uint32_t M, uint32_t S, uint32_t ptype, uint32_t opcode, VRegister rd, VRegister rn, VRegister rm) {
    uint32_t Instr = 0b0001'1110'0010'0000'0000'1000'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void Float2Source(ScalarRegSize size, uint32_t M, uint32_t S, uint32_t opcode, VRegister rd, VRegister rn, VRegister rm) {
    LOGMAN_THROW_A_FMT(size == ScalarRegSize::i16Bit || size == ScalarRegSize::i64Bit || size == ScalarRegSize::i32Bit,
                       "Invalid size selected for {}", __func__);

    const uint32_t ConvertedSize = size == ScalarRegSize::i64Bit ? 0b01 : size == ScalarRegSize::i32Bit ? 0b00 : 0b11;

    Float2Source(M, S, ConvertedSize, opcode, rd, rn, rm);
  }

  // Floating-point conditional select
  void FloatConditionalSelect(uint32_t M, uint32_t S, uint32_t ptype, VRegister rd, VRegister rn, VRegister rm, Condition Cond) {
    uint32_t Instr = 0b0001'1110'0010'0000'0000'1100'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= rm.Idx() << 16;
    Instr |= FEXCore::ToUnderlying(Cond) << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Floating-point data-processing (3 source)
  void Float3Source(uint32_t M, uint32_t S, uint32_t ptype, uint32_t o1, uint32_t o0, VRegister rd, VRegister rn, VRegister rm, VRegister ra) {
    uint32_t Instr = 0b0001'1111'0000'0000'0000'0000'0000'0000;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= o1 << 21;
    Instr |= Encode_rm(rm);
    Instr |= o0 << 15;
    Instr |= Encode_ra(ra);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

#ifndef INCLUDED_BY_EMITTER
}; // struct LoadstoreEmitterOps
} // namespace ARMEmitter
#endif
