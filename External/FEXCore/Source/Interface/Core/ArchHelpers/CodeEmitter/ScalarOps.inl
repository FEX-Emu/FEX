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
public:
// Advanced SIMD scalar copy
  void dup(ScalarRegSize size, VRegister rd, VRegister rn, uint32_t Index) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'01 << 10;

    const uint32_t SizeImm = FEXCore::ToUnderlying(size);
    const uint32_t IndexShift = SizeImm + 1;
    const uint32_t ElementSize = 1U << SizeImm;
    const uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_AA_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, 1, imm5, 0b0000, rd, rn);
  }

  void mov(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Index) {
    dup(size, rd, rn, Index);
  }

// Advanced SIMD scalar three same FP16
  void fmulx(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 0, 0, 0b011, rm, rn, rd);
  }
  void fcmeq(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 0, 0, 0b100, rm, rn, rd);
  }
  void frecps(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 0, 0, 0b111, rm, rn, rd);
  }
  void frsqrts(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 0, 1, 0b111, rm, rn, rd);
  }
  void fcmge(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 1, 0, 0b100, rm, rn, rd);
  }
  void facge(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 1, 0, 0b101, rm, rn, rd);
  }
  void fabd(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 1, 1, 0b010, rm, rn, rd);
  }
  void fcmgt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 1, 1, 0b100, rm, rn, rd);
  }
  void facgt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0100'0000'0000'01 << 10;
    ASIMDScalarThreeSameFP16(Op, 1, 1, 0b101, rm, rn, rd);
  }

// Advanced SIMD scalar two-register miscellaneous FP16
  void fcvtns(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 0, 0b11010, rn, rd);
  }
  void fcvtms(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 0, 0b11011, rn, rd);
  }
  void fcvtas(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 0, 0b11100, rn, rd);
  }
  void scvtf(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 0, 0b11101, rn, rd);
  }
  void fcmgt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b01100, rn, rd);
  }
  void fcmeq(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b01101, rn, rd);
  }
  void fcmlt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b01110, rn, rd);
  }
  void fcvtps(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b11010, rn, rd);
  }
  void fcvtzs(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b11011, rn, rd);
  }
  void frecpe(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b11101, rn, rd);
  }
  void frecpx(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 0, 1, 0b11111, rn, rd);
  }
  void fcvtnu(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 0, 0b11010, rn, rd);
  }
  void fcvtmu(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 0, 0b11011, rn, rd);
  }
  void fcvtau(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 0, 0b11100, rn, rd);
  }
  void ucvtf(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 0, 0b11101, rn, rd);
  }
  void fcmge(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 1, 0b01100, rn, rd);
  }
  void fcmle(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 1, 0b01101, rn, rd);
  }
  void fcvtpu(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 1, 0b11010, rn, rd);
  }
  void fcvtzu(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 1, 0b11011, rn, rd);
  }
  void frsqrte(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0111'1000'0000'10 << 10;
    ASIMDScalarTwoRegMiscFP16(Op, 1, 1, 0b11101, rn, rd);
  }

// Advanced SIMD scalar three same extra
// XXX:
// Advanced SIMD scalar two-register miscellaneous
  void suqadd(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b00011, rd, rn);
  }
  void sqabs(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b00111, rd, rn);
  }

  ///< Comparison against 0.0
  void cmgt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01000, rd, rn);
  }
  ///< Comparison against 0.0
  void cmeq(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01001, rd, rn);
  }

  ///< Comparison against 0.0
  void cmlt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01010, rd, rn);
  }
  void abs(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01011, rd, rn);
  }
  ///< size is destination size.
  void sqxtn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "64-bit destination not supported");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b10100, rd, rn);
  }

  void fcvtns(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 0, ConvertedSize, 0b11010, rd, rn);
  }
  void fcvtms(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 0, ConvertedSize, 0b11011, rd, rn);
  }
  void fcvtas(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 0, ConvertedSize, 0b11100, rd, rn);
  }
  void scvtf(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 0, ConvertedSize, 0b11101, rd, rn);
  }

  ///< Comparison against 0.0
  void fcmgt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float compare");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01100, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmeq(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float compare");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01101, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmlt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float compare");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b01110, rd, rn);
  }
  void fcvtps(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b11010, rd, rn);
  }
  void fcvtzs(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b11011, rd, rn);
  }
  void frecpe(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b11101, rd, rn);
  }
  void frecpx(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b11111, rd, rn);
  }
  void usqadd(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b00011, rd, rn);
  }
  void sqneg(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b00111, rd, rn);
  }
  ///< Comparison against 0.0
  void cmge(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b01000, rd, rn);
  }
  ///< Comparison against 0.0
  void cmle(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b01001, rd, rn);
  }
  void neg(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b01011, rd, rn);
  }
  ///< size is destination.
  void sqxtun(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "64-bit destination not supported");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b10010, rd, rn);
  }
  ///< size is destination.
  void uqxtn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "64-bit destination not supported");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b10100, rd, rn);
  }
  ///< size is destination.
  void fcvtxn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 1, ARMEmitter::ScalarRegSize::i16Bit, 0b10110, rd, rn);
  }
  void fcvtnu(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b11010, rd, rn);
  }
  void fcvtmu(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b11011, rd, rn);
  }
  void fcvtau(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b11100, rd, rn);
  }
  void ucvtf(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b11101, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmge(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b01100, rd, rn);
  }
  ///< Comparison against 0.0
  void fcmle(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b01101, rd, rn);
  }
  void fcvtpu(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b11010, rd, rn);
  }
  void fcvtzu(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b11011, rd, rn);
  }
  void frsqrte(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 1, size, 0b11101, rd, rn);
  }
  // Advanced SIMD scalar pairwise
  void addp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for addp");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;

    ASIMDScalar2RegMisc(Op, 0, size, 0b11011, rd, rn);
  }

  void fmaxnmp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 0, ARMEmitter::ScalarRegSize::i8Bit, 0b01100, rd.V(), rn.V());
  }
  void faddp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 0, ARMEmitter::ScalarRegSize::i8Bit, 0b01101, rd.V(), rn.V());
  }
  void fmaxp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 0, ARMEmitter::ScalarRegSize::i8Bit, 0b01111, rd.V(), rn.V());
  }
  void fminnmp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 0, ARMEmitter::ScalarRegSize::i32Bit, 0b01100, rd.V(), rn.V());
  }
  void fminp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 0, ARMEmitter::ScalarRegSize::i32Bit, 0b01111, rd.V(), rn.V());
  }

  void fmaxnmp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b01100, rd, rn);
  }
  void faddp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b01101, rd, rn);
  }
  void fmaxp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMDScalar2RegMisc(Op, 1, ConvertedSize, 0b01111, rd, rn);
  }
  void fminnmp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 1, size, 0b01100, rd, rn);
  }
  void fminp(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0011'0000'0000'10 << 10;
    ASIMDScalar2RegMisc(Op, 1, size, 0b01111, rd, rn);
  }
// Advanced SIMD scalar three different
  ///< size is destination.
  void sqdmlal(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'00 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i32Bit :
        ARMEmitter::ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(Op, 0, ConvertedSize, 0b1001, rd, rn, rm);
  }
  ///< size is destination.
  void sqdmlsl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'00 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i32Bit :
        ARMEmitter::ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(Op, 0, ConvertedSize, 0b1011, rd, rn, rm);
  }

  ///< size is destination.
  void sqdmull(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'00 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i32Bit :
        ARMEmitter::ScalarRegSize::i16Bit;
    ASIMD3RegDifferent(Op, 0, ConvertedSize, 0b1101, rd, rn, rm);
  }
// Advanced SIMD scalar three same
  void sqadd(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b00001, rd, rn, rm);
  }
  void sqsub(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b00101, rd, rn, rm);
  }
  void cmgt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b00110, rd, rn, rm);
  }
  void cmge(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b00111, rd, rn, rm);
  }
  void sshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b01000, rd, rn, rm);
  }
  void sqshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b01001, rd, rn, rm);
  }
  void srshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b01010, rd, rn, rm);
  }
  void sqrshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b01011, rd, rn, rm);
  }
  void add(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b10000, rd, rn, rm);
  }
  void cmtst(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b10001, rd, rn, rm);
  }
  void sqdmulh(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i32Bit || size == ARMEmitter::ScalarRegSize::i16Bit, "Invalid size");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b10110, rd, rn, rm);
  }
  void fmulx(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMD3RegSame(Op, 0, ConvertedSize, 0b11011, rd, rn, rm);
  }
  void fcmeq(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMD3RegSame(Op, 0, ConvertedSize, 0b11100, rd, rn, rm);
  }
  void frecps(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMD3RegSame(Op, 0, ConvertedSize, 0b11111, rd, rn, rm);
  }
  void frsqrts(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 0, size, 0b11111, rd, rn, rm);
  }
  void uqadd(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b00001, rd, rn, rm);
  }
  void uqsub(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b00101, rd, rn, rm);
  }
  void cmhi(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b00110, rd, rn, rm);
  }
  void cmhs(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b00111, rd, rn, rm);
  }
  void ushl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b01000, rd, rn, rm);
  }
  void uqshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b01001, rd, rn, rm);
  }
  void urshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b01010, rd, rn, rm);
  }
  void uqrshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b01011, rd, rn, rm);
  }
  void sub(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b10000, rd, rn, rm);
  }
  void cmeq(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit, "Only supports 64-bit");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b10001, rd, rn, rm);
  }
  void sqrdmulh(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i32Bit || size == ARMEmitter::ScalarRegSize::i16Bit, "Invalid size");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b10110, rd, rn, rm);
  }
  void fcmge(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMD3RegSame(Op, 1, ConvertedSize, 0b11100, rd, rn, rm);
  }
  void facge(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::ScalarRegSize ConvertedSize =
      size == ARMEmitter::ScalarRegSize::i64Bit ?
        ARMEmitter::ScalarRegSize::i16Bit :
        ARMEmitter::ScalarRegSize::i8Bit;

    ASIMD3RegSame(Op, 1, ConvertedSize, 0b11101, rd, rn, rm);
  }
  void fabd(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b11010, rd, rn, rm);
  }
  void fcmgt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b11100, rd, rn, rm);
  }
  void facgt(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit || size == ARMEmitter::ScalarRegSize::i32Bit, "Invalid size selected for float convert");
    constexpr uint32_t Op = 0b0101'1110'0010'0000'0000'01 << 10;
    ASIMD3RegSame(Op, 1, size, 0b11101, rd, rn, rm);
  }
// Advanced SIMD scalar shift by immediate
  void sshr(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b00000, rd, rn);
  }
  void ssra(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b00010, rd, rn);
  }
  void srshr(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b00100, rd, rn);
  }
  void srsra(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b00110, rd, rn);
  }
  void shl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b01010, rd, rn);
  }
  void sqshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b01110, rd, rn);
  }
  ///< size is destination
  void sqshrn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrn");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b10010, rd, rn);
  }
  void sqrshrn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrn");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 0, immh, immb, 0b10011, rd, rn);
  }
  // TODO: SCVTF, FCVTZS
  void ushr(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b00000, rd, rn);
  }
  void usra(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b00010, rd, rn);
  }
  void urshr(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b00100, rd, rn);
  }
  void ursra(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b00110, rd, rn);
  }
  void sri(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b01000, rd, rn);
  }
  void sli(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(Shift > 0 && Shift < 64, "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size == ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b01010, rd, rn);
  }
  void sqshlu(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b01100, rd, rn);
  }
  void uqshl(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    // Shift encoded a bit weirdly.
    // shift = immh:immb - elementsize but immh is /also/ used for element size.
    const uint32_t immh = 1 << FEXCore::ToUnderlying(size) | (Shift >> 3);
    const uint32_t immb = Shift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b01110, rd, rn);
  }
  ///< size is destination.
  void sqshrun(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqshrun");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b10000, rd, rn);
  }
  ///< size is destination.
  void sqrshrun(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b10001, rd, rn);
  }
  ///< size is destination.
  void uqshrn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b10010, rd, rn);
  }
  ///< size is destination.
  void uqrshrn(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(Shift > 0 && Shift < ScalarRegSizeInBits(size), "Invalid shift for sshr");
    LOGMAN_THROW_AA_FMT(size != ARMEmitter::ScalarRegSize::i64Bit, "Invalid size selected for sqrshrun");
    constexpr uint32_t Op = 0b0101'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = ScalarRegSizeInBits(size);
    // Shift encoded in immh:immb, but inverted with 128-bit source
    // shift = (esize * 2) - immh:immb
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;
    ASIMDScalarShiftByImm(Op, 1, immh, immb, 0b10011, rd, rn);
  }
  // TODO: UCVTF, FCVTZU
// Advanced SIMD scalar x indexed element
// XXX:
//
// Floating-point data-processing (1 source)
  void fmov(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000000, rd.V(), rn.V());
  }
  void fabs(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000001, rd.V(), rn.V());
  }
  void fneg(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000011, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000101, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b000111, rd.V(), rn.V());
  }
  void frintn(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001000, rd.V(), rn.V());
  }
  void frintp(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001001, rd.V(), rn.V());
  }
  void frintm(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001010, rd.V(), rn.V());
  }
  void frintz(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001011, rd.V(), rn.V());
  }
  void frinta(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001100, rd.V(), rn.V());
  }
  void frintx(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001110, rd.V(), rn.V());
  }
  void frinti(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b001111, rd.V(), rn.V());
  }
  void frint32z(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b010000, rd.V(), rn.V());
  }
  void frint32x(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b010001, rd.V(), rn.V());
  }
  void frint64z(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b010010, rd.V(), rn.V());
  }
  void frint64x(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b00, 0b010011, rd.V(), rn.V());
  }

  void fmov(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000000, rd.V(), rn.V());
  }
  void fabs(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000001, rd.V(), rn.V());
  }
  void fneg(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000011, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000100, rd.V(), rn.V());
  }
  void bfcvt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000110, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b000111, rd.V(), rn.V());
  }
  void frintn(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001000, rd.V(), rn.V());
  }
  void frintp(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001001, rd.V(), rn.V());
  }
  void frintm(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001010, rd.V(), rn.V());
  }
  void frintz(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001011, rd.V(), rn.V());
  }
  void frinta(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001100, rd.V(), rn.V());
  }
  void frintx(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001110, rd.V(), rn.V());
  }
  void frinti(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b001111, rd.V(), rn.V());
  }
  void frint32z(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b010000, rd.V(), rn.V());
  }
  void frint32x(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b010001, rd.V(), rn.V());
  }
  void frint64z(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b010010, rd.V(), rn.V());
  }
  void frint64x(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b01, 0b010011, rd.V(), rn.V());
  }

  void fmov(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000000, rd.V(), rn.V());
  }
  void fabs(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000001, rd.V(), rn.V());
  }
  void fneg(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000010, rd.V(), rn.V());
  }
  void fsqrt(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000011, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000100, rd.V(), rn.V());
  }
  void fcvt(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b000101, rd.V(), rn.V());
  }
  void frintn(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001000, rd.V(), rn.V());
  }
  void frintp(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001001, rd.V(), rn.V());
  }
  void frintm(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001010, rd.V(), rn.V());
  }
  void frintz(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001011, rd.V(), rn.V());
  }
  void frinta(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001100, rd.V(), rn.V());
  }
  void frintx(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001110, rd.V(), rn.V());
  }
  void frinti(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b100'00 << 10;

    Float1Source(Op, 0, 0, 0b11, 0b001111, rd.V(), rn.V());
  }

// Floating-point compare
  void fcmp(FEXCore::ARMEmitter::ScalarRegSize Size, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    LOGMAN_THROW_AA_FMT(Size != FEXCore::ARMEmitter::ScalarRegSize::i8Bit, "8-bit destination not supported");

    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    const auto ConvertedSize =
      Size == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      Size == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      Size == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    FloatCompare(Op, 0, 0, ConvertedSize, 0b00, 0b00000, rn, rm);
  }

  void fcmp(FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b00, 0b00, 0b00000, rn.V(), rm.V());
  }
  ///< Compare to #0.0
  void fcmp(FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b00, 0b00, 0b01000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }
  void fcmpe(FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b00, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b00, 0b00, 0b11000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }
  void fcmp(FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b01, 0b00, 0b00000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmp(FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b01, 0b00, 0b01000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }
  void fcmpe(FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b01, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b01, 0b00, 0b11000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }
  void fcmp(FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b11, 0b00, 0b00000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmp(FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b11, 0b00, 0b01000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }
  void fcmpe(FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b11, 0b00, 0b10000, rn.V(), rm.V());
  }

  ///< Compare to #0.0
  void fcmpe(FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b1000 << 10;
    FloatCompare(Op, 0, 0, 0b11, 0b00, 0b11000, rn.V(), FEXCore::ARMEmitter::VReg::v0);
  }

// Floating-point immediate
  void fmov(FEXCore::ARMEmitter::ScalarRegSize size, FEXCore::ARMEmitter::VRegister rd, float Value) {
    uint32_t M = 0;
    uint32_t S = 0;
    uint32_t ptype;
    uint32_t imm8;
    uint32_t imm5 = 0b0'0000;
    if (size == FEXCore::ARMEmitter::ScalarRegSize::i16Bit) {
      ptype = 0b11;
      LOGMAN_THROW_A_FMT(vixl::aarch64::Assembler::IsImmFP16(vixl::Float16(Value)), "Invalid float");
      imm8 = vixl::VFP::FP16ToImm8(vixl::Float16(Value));
    }
    else if (size == FEXCore::ARMEmitter::ScalarRegSize::i32Bit) {
      ptype = 0b00;
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP32(Value), "Invalid float");
      imm8 = vixl::VFP::FP32ToImm8(Value);
    }
    else if (size == FEXCore::ARMEmitter::ScalarRegSize::i64Bit) {
      ptype = 0b01;
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP64(Value), "Invalid float");
      imm8 = vixl::VFP::FP64ToImm8(Value);
    }
    else {
      FEX_UNREACHABLE;
    }

    FloatScalarImmediate(M, S, ptype, imm8, imm5, rd);
  }

  void FloatScalarImmediate(uint32_t M, uint32_t S, uint32_t ptype, uint32_t imm8, uint32_t imm5, FEXCore::ARMEmitter::VRegister rd) {
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
  void fccmp(FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b00, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b00, 0b1, rn.V(), rm.V(), flags, Cond);
  }
  void fccmp(FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b01, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b01, 0b1, rn.V(), rm.V(), flags, Cond);
  }
  void fccmp(FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b11, 0b0, rn.V(), rm.V(), flags, Cond);
  }
  void fccmpe(FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b01 << 10;

    FloatConditionalCompare(Op, 0, 0, 0b11, 0b1, rn.V(), rm.V(), flags, Cond);
  }

// Floating-point data-processing (2 source)
  void fmul(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b00, 0b1000, rd.V(), rn.V(), rm.V());
  }

  void fmul(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b01, 0b1000, rd.V(), rn.V(), rm.V());
  }

  void fmul(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0000, rd.V(), rn.V(), rm.V());
  }
  void fdiv(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0001, rd.V(), rn.V(), rm.V());
  }
  void fadd(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0010, rd.V(), rn.V(), rm.V());
  }
  void fsub(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0011, rd.V(), rn.V(), rm.V());
  }
  void fmax(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0100, rd.V(), rn.V(), rm.V());
  }
  void fmin(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0101, rd.V(), rn.V(), rm.V());
  }
  void fmaxnm(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0110, rd.V(), rn.V(), rm.V());
  }
  void fminnm(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b0111, rd.V(), rn.V(), rm.V());
  }
  void fnmul(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b10 << 10;
    Float2Source(Op, 0, 0, 0b11, 0b1000, rd.V(), rn.V(), rm.V());
  }

  // Floating-point conditional select
  void fcsel(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b11 << 10;
    Float2Source(Op, 0, 0, 0b00, FEXCore::ToUnderlying(Cond), rd.V(), rn.V(), rm.V());
  }
  void fcsel(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b11 << 10;
    Float2Source(Op, 0, 0, 0b01, FEXCore::ToUnderlying(Cond), rd.V(), rn.V(), rm.V());
  }
  void fcsel(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21 |
                            0b11 << 10;
    Float2Source(Op, 0, 0, 0b11, FEXCore::ToUnderlying(Cond), rd.V(), rn.V(), rm.V());
  }

// Floating-point data-processing (3 source)
  void fmadd(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::SRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b00, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::SRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b00, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::SRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b00, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::SRegister rm, FEXCore::ARMEmitter::SRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b00, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

  void fmadd(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::DRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b01, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::DRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b01, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::DRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b01, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, FEXCore::ARMEmitter::DRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b01, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

  void fmadd(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::HRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b11, 0, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fmsub(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::HRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b11, 0, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmadd(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::HRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b11, 1, 0, rd.V(), rn.V(), rm.V(), ra.V());
  }
  void fnmsub(FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::HRegister ra) {
    constexpr uint32_t Op = 0b0001'1111'000 << 21;

    Float3Source(Op, 0, 0, 0b11, 1, 1, rd.V(), rn.V(), rm.V(), ra.V());
  }

private:
// Advanced SIMD scalar copy
  void ASIMDScalarCopy(uint32_t Op, uint32_t Q, uint32_t imm5, uint32_t imm4, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= Q << 30;
    Instr |= imm5 << 16;
    Instr |= imm4 << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

// Advanced SIMD scalar three same FP16
  void ASIMDScalarThreeSameFP16(uint32_t Op, uint32_t U, uint32_t a, uint32_t opcode, FEXCore::ARMEmitter::HRegister rm, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rd) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }
// Advanced SIMD scalar two-register miscellaneous FP16
  void ASIMDScalarTwoRegMiscFP16(uint32_t Op, uint32_t U, uint32_t a, uint32_t opcode, FEXCore::ARMEmitter::HRegister rn, FEXCore::ARMEmitter::HRegister rd) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= opcode << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

// Advanced SIMD scalar three same extra
// XXX:
// Advanced SIMD scalar two-register miscellaneous
  void ASIMDScalar2RegMisc(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::ScalarRegSize size, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opcode << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

// Advanced SIMD scalar pairwise
// XXX:
// Advanced SIMD scalar three different
  void ASIMD3RegDifferent(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::ScalarRegSize size, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }
// Advanced SIMD scalar three same
  void ASIMD3RegSame(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::ScalarRegSize size, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }
// Advanced SIMD scalar shift by immediate
  void ASIMDScalarShiftByImm(uint32_t Op, uint32_t U, uint32_t immh, uint32_t immb, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= U << 29;
    Instr |= immh << 19;
    Instr |= immb << 16;
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }
// Advanced SIMD scalar x indexed element
// XXX:
// Floating-point data-processing (1 source)
  void Float1Source(uint32_t Op, uint32_t M, uint32_t S, uint32_t ptype, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= opcode << 15;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }
// Floating-point compare
  void FloatCompare(uint32_t Op, uint32_t M, uint32_t S, uint32_t ftype, uint32_t op, uint32_t opcode2, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

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
  void FloatConditionalCompare(uint32_t Op, uint32_t M, uint32_t S, uint32_t ptype, uint32_t op, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    uint32_t Instr = Op;

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
  void Float2Source(uint32_t Op, uint32_t M, uint32_t S, uint32_t ptype, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= M << 31;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

// Floating-point conditional select
  void FloatConditionalSelect(uint32_t Op, uint32_t M, uint32_t S, uint32_t ptype, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, FEXCore::ARMEmitter::Condition Cond) {
    uint32_t Instr = Op;

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
  void Float3Source(uint32_t Op, uint32_t M, uint32_t S, uint32_t ptype, uint32_t o1, uint32_t o0, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, FEXCore::ARMEmitter::VRegister ra) {
    uint32_t Instr = Op;

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
