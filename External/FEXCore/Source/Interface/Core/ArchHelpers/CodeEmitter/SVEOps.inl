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
public:
  // SVE encodings
  void dup(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Index) {
    constexpr uint32_t Op = 0b0000'0101'0010'0000'0010'00 << 10;
    uint32_t imm2{};
    uint32_t tsz{};

    // We can index up to 512-bit registers with dup
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 64, "Index too large");
      tsz = 0b00001 | ((Index & 0b1111) << 1);
      imm2 = Index >> 4;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 32, "Index too large");
      tsz = 0b00010 | ((Index & 0b111) << 2);
      imm2 = Index >> 3;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      tsz = 0b00100 | ((Index & 0b11) << 3);
      imm2 = Index >> 2;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      tsz = 0b01000 | ((Index & 0b1) << 4);
      imm2 = Index >> 1;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i128Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      tsz = 0b10000;
      imm2 = Index;
    }

    SVEDup(Op, imm2, tsz, zn, zd);
  }
  // TODO: TBL

  void sel(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pv, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'11 << 14;
    SVESel(Op, size, zm, pv, zn, zd);
  }

  void mov(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pv, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'11 << 14;
    SVESel(Op, size, zd, pv, zn, zd);
  }

  void histcnt(SubRegSize size, ZRegister zd, PRegisterZero pv, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "SubRegSize must be 32-bit or 64-bit");
    LOGMAN_THROW_AA_FMT(pv <= PReg::p7, "histcnt can only use p0 to p7");

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

  void fcmla(SubRegSize size, ZRegister zda, ZRegister zn, ZRegister zm, uint32_t index, Rotation rot) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit,
                        "SubRegSize must be 16-bit or 32-bit");

    const auto IsHalfPrecision = size == SubRegSize::i16Bit;

    if (IsHalfPrecision) {
      LOGMAN_THROW_AA_FMT(index <= 3, "Index for half-precision fcmla must be within 0-3. Index={}", index);
      LOGMAN_THROW_AA_FMT(zm.Idx() <= 7, "zm must be within z0-z7. zm=z{}", zm.Idx());
    } else {
      LOGMAN_THROW_AA_FMT(index <= 1, "Index for single-precision fcmla must be within 0-1. Index={}", index);
      LOGMAN_THROW_AA_FMT(zm.Idx() <= 15, "zm must be within z0-z15. zm=z{}", zm.Idx());
    }

    uint32_t Op = 0b0110'0100'1010'0000'0001'0000'0000'0000;
    Op |= (IsHalfPrecision ? 0 : 1) << 22;
    Op |= index << (19 + int(!IsHalfPrecision));
    Op |= zm.Idx() << 16;
    Op |= FEXCore::ToUnderlying(rot) << 10;
    Op |= zn.Idx() << 5;
    Op |= zda.Idx();

    dc32(Op);
  }

  void fcmla(SubRegSize size, ZRegister zda, PRegisterMerge pv, ZRegister zn, ZRegister zm, Rotation rot) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                        "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_AA_FMT(pv <= PReg::p7, "fcmla can only use p0 to p7");

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
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                        "SubRegSize must be 16-bit, 32-bit, or 64-bit");
    LOGMAN_THROW_AA_FMT(pv <= PReg::p7, "fcadd can only use p0 to p7");
    LOGMAN_THROW_AA_FMT(rot == Rotation::ROTATE_90 || rot == Rotation::ROTATE_270,
                        "fcadd rotation may only be 90 or 270 degrees");
    LOGMAN_THROW_AA_FMT(zd.Idx() == zn.Idx(), "fcadd zd and zn must be the same register");

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
  void add(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b000, size, zm, zn, zd);
  }
  void sub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b001, size, zm, zn, zd);
  }
  void sqadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b100, size, zm, zn, zd);
  }
  void uqadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b101, size, zm, zn, zd);
  }
  void sqsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b110, size, zm, zn, zd);
  }
  void uqsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'000 << 13;
    SVEIntegerAddSubUnpredicated(Op, 0b111, size, zm, zn, zd);
  }

  // SVE address generation
  // XXX:
  // SVE table lookup (three sources)
  void tbl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0101'0010'0000'0011'0 << 11;
    SVETableLookup(Op, 0, size, zm, zn, zd);
  }
  void tbx(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0101'0010'0000'0010'1 << 11;
    SVETableLookup(Op, 1, size, zm, zn, zd);
  }
  // SVE permute vector elements
  void zip1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b000, size, zm, zn, zd);
  }
  void zip2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b001, size, zm, zn, zd);
  }
  void uzp1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b010, size, zm, zn, zd);
  }
  void uzp2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b011, size, zm, zn, zd);
  }
  void trn1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b100, size, zm, zn, zd);
  }
  void trn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'011 << 13;
    SVEPermute(Op, 0b101, size, zm, zn, zd);
  }

  // SVE integer compare with unsigned immediate
  void cmphi(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, uint32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm < 128, "Invalid imm");
    SVEIntegerCompareImm(0, 1, imm, size, pg, zn, pd);
  }

  void cmphs(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, uint32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm < 128, "Invalid imm");
    SVEIntegerCompareImm(0, 0, imm, size, pg, zn, pd);
  }

  void cmplo(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, uint32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm < 128, "Invalid imm");
    SVEIntegerCompareImm(1, 0, imm, size, pg, zn, pd);
  }

  void cmpls(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, uint32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm < 128, "Invalid imm");
    SVEIntegerCompareImm(1, 1, imm, size, pg, zn, pd);
  }

  void SVEIntegerCompareImm(uint32_t lt, uint32_t ne, uint32_t imm7, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::PRegister pd) {
    constexpr uint32_t Op = 0b0010'0100'0010'0000'0000 << 12;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= imm7 << 14;
    Instr |= lt << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }
  // SVE integer compare with signed immediate
  void cmpeq(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, int32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm >= -16 && imm <= 15, "Invalid imm");
    SVEIntegerCompareSignedImm(1, 0, 0, imm, size, pg, zn, pd);
  }

  void cmpgt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, int32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm >= -16 && imm <= 15, "Invalid imm");
    SVEIntegerCompareSignedImm(0, 0, 1, imm, size, pg, zn, pd);
  }

  void cmpge(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, int32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm >= -16 && imm <= 15, "Invalid imm");
    SVEIntegerCompareSignedImm(0, 0, 0, imm, size, pg, zn, pd);
  }

  void cmplt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, int32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm >= -16 && imm <= 15, "Invalid imm");
    SVEIntegerCompareSignedImm(0, 1, 0, imm, size, pg, zn, pd);
  }

  void cmple(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, int32_t imm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(imm >= -16 && imm <= 15, "Invalid imm");
    SVEIntegerCompareSignedImm(0, 1, 1, imm, size, pg, zn, pd);
  }

  void SVEIntegerCompareSignedImm(uint32_t op, uint32_t o2, uint32_t ne, uint32_t imm5, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::PRegister pd) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'000 << 13;;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= (imm5 & 0b1'1111) << 16;
    Instr |= op << 15;
    Instr |= o2 << 13;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= ne << 4;
    Instr |= pd.Idx();
    dc32(Instr);
  }
  // SVE predicate logical operations
  void and_(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 0, 0, pm, pg, pn, pd);
  }

  void ands(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 1, 0, 0, pm, pg, pn, pd);
  }

  void mov(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 1, 1, pd, pg, pn, pd);
  }

  void mov(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 0, 0, pn, pg, pn, pd);
  }

  void movs(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 1, 0, 0, pn, pg, pn, pd);
  }
  void bic(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 0, 1, pm, pg, pn, pd);
  }
  void bics(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 1, 0, 1, pm, pg, pn, pd);
  }

  void eor(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 1, 0, pm, pg, pn, pd);
  }
  void eors(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 1, 1, 0, pm, pg, pn, pd);
  }

  void not_(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 1, 0, pg, pg, pn, pd);
  }
  void sel(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 0, 0, 1, 1, pm, pg, pn, pd);
  }
  void orr(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 0, 0, 0, pm, pg, pn, pd);
  }
  void mov(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 0, 0, 0, pn, pn, pn, pd);
  }
  void orn(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 0, 0, 1, pm, pg, pn, pd);
  }
  void nor(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 0, 1, 0, pm, pg, pn, pd);
  }
  void nand(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 0, 1, 1, pm, pg, pn, pd);
  }
  void orrs(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 1, 0, 0, pm, pg, pn, pd);
  }
  void movs(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 1, 0, 0, pn, pn, pn, pd);
  }
  void orns(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 1, 0, 1, pm, pg, pn, pd);
  }
  void nors(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 1, 1, 0, pm, pg, pn, pd);
  }
  void nands(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pm) {
    constexpr uint32_t Op = 0b0010'0101'0000'0000'01 << 14;
    SVEPredicateLogical(Op, 1, 1, 1, 1, pm, pg, pn, pd);
  }

  // XXX:
  // SVE broadcast predicate element
  // XXX:
  // SVE integer clamp

  // SVE2 character match
  void match(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    constexpr uint32_t Op = 0b0100'0101'0010'0000'1000'0000'0000'0000;
    SVECharacterMatch(Op, 0, size, pd, pg, zn, zm);
  }
  void nmatch(SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    constexpr uint32_t Op = 0b0100'0101'0010'0000'1000'0000'0000'0000;
    SVECharacterMatch(Op, 1, size, pd, pg, zn, zm);
  }

  // SVE floating-point convert precision odd elements
  void fcvtxnt(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    constexpr uint32_t Op = 0b0110'0100'0000'1000'101 << 13;
    SVEFloatConvertOdd(Op, 0b00, 0b10, pg, zn, zd);
  }
  ///< Size is destination size
  void fcvtnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0100'0000'1000'101 << 13;

    const auto ConvertedDestSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b00 :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 : 0b00;

    const auto ConvertedSrcSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b11 : 0b00;

    SVEFloatConvertOdd(Op, ConvertedSrcSize, ConvertedDestSize, pg, zn, zd);
  }

  ///< Size is destination size
  void fcvtlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0100'0000'1000'101 << 13;

    const auto ConvertedDestSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b01 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 : 0b00;

    const auto ConvertedSrcSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 : 0b00;

    SVEFloatConvertOdd(Op, ConvertedSrcSize, ConvertedDestSize, pg, zn, zd);
  }

  // XXX: BFCVTNT
  // SVE2 floating-point pairwise operations
  void faddp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");

    constexpr uint32_t Op = 0b0110'0100'0001'0000'100 << 13;;
    SVEFloatPairwiseArithmetic(Op, 0b000, size, pg, zm, zd);
  }

  void fmaxnmp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");

    constexpr uint32_t Op = 0b0110'0100'0001'0000'100 << 13;;
    SVEFloatPairwiseArithmetic(Op, 0b100, size, pg, zm, zd);
  }
  void fminnmp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");

    constexpr uint32_t Op = 0b0110'0100'0001'0000'100 << 13;;
    SVEFloatPairwiseArithmetic(Op, 0b101, size, pg, zm, zd);
  }
  void fmaxp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");

    constexpr uint32_t Op = 0b0110'0100'0001'0000'100 << 13;;
    SVEFloatPairwiseArithmetic(Op, 0b110, size, pg, zm, zd);
  }
  void fminp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");

    constexpr uint32_t Op = 0b0110'0100'0001'0000'100 << 13;;
    SVEFloatPairwiseArithmetic(Op, 0b111, size, pg, zm, zd);
  }

  // SVE floating-point multiply-add (indexed)
  // XXX:
  // SVE floating-point complex multiply-add (indexed)
  // XXX:
  // SVE floating-point multiply (indexed)
  // XXX:
  // SVE floating point matrix multiply accumulate
  // XXX:
  // SVE floating-point compare vectors
  void fcmeq(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEFloatCompareVector(0, 1, 0, size, zm, pg, zn, pd);
  }

  void fcmgt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEFloatCompareVector(0, 0, 1, size, zm, pg, zn, pd);
  }

  void fcmge(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEFloatCompareVector(0, 0, 0, size, zm, pg, zn, pd);
  }
  void fcmne(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEFloatCompareVector(0, 1, 1, size, zm, pg, zn, pd);
  }

  void fcmuo(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEFloatCompareVector(1, 0, 0, size, zm, pg, zn, pd);
  }

  void SVEFloatCompareVector(uint32_t op, uint32_t o2, uint32_t o3, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::PRegister pd) {
    constexpr uint32_t Op = 0b0110'0101'0000'0000'010 << 13;
    uint32_t Instr = Op;

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

  // SVE floating-point arithmetic (unpredicated)
  void fadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b000, size, zm, zn, zd);
  }
  void fsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b001, size, zm, zn, zd);
  }
  void fmul(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b010, size, zm, zn, zd);
  }
  void ftsmul(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b011, size, zm, zn, zd);
  }
  void frecps(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b110, size, zm, zn, zd);
  }
  void frsqrts(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'000 << 13;
    SVEFloatArithmeticUnpredicated(Op, 0b111, size, zm, zn, zd);
  }

  // SVE floating-point recursive reduction
  void faddv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0110'0101'0000'0000'0010'0000'0000'0000;
    SVEFPRecursiveReduction(Op, 0b000, size, vd, pg, zn);
  }
  void fmaxnmv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0110'0101'0000'0000'0010'0000'0000'0000;
    SVEFPRecursiveReduction(Op, 0b100, size, vd, pg, zn);
  }
  void fminnmv(SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    constexpr uint32_t Op = 0b0110'0101'0000'0000'0010'0000'0000'0000;
    SVEFPRecursiveReduction(Op, 0b101, size, vd, pg, zn);
  }

  // SVE integer Multiply-Add - Predicated
  // SVE integer multiply-accumulate writing addend (predicated)
  // XXX:
  // SVE integer multiply-add writing multiplicand (predicated)
  // XXX:

  // SVE Integer Binary Arithmetic - Predicated
  // SVE integer add/subtract vectors (predicated)
  // XXX:
  // SVE integer min/max/difference (predicated)
  void smax(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b00, 0, size, pg, zm, zd);
  }
  void umax(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b00, 1, size, pg, zm, zd);
  }
  void smin(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b01, 0, size, pg, zm, zd);
  }
  void umin(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b01, 1, size, pg, zm, zd);
  }
  void sabd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b10, 0, size, pg, zm, zd);
  }
  void uabd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEIntegerMinMaxDifferencePredicated(0b10, 1, size, pg, zm, zd);
  }

  void SVEIntegerMinMaxDifferencePredicated(uint32_t opc, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0000'0100'0000'1000'000 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE integer multiply vectors (predicated)
  // XXX:
  // SVE integer divide vectors (predicated)
  // XXX:
  // SVE bitwise logical operations (predicated)
  void orr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'000 << 13;
    SVEBitwiseLogicalPredicated(Op, 0b000, size, pg, zm, zd);
  }
  void eor(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'000 << 13;
    SVEBitwiseLogicalPredicated(Op, 0b001, size, pg, zm, zd);
  }
  void and_(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'000 << 13;
    SVEBitwiseLogicalPredicated(Op, 0b010, size, pg, zm, zd);
  }
  void bic(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'000 << 13;
    SVEBitwiseLogicalPredicated(Op, 0b011, size, pg, zm, zd);
  }

  // SVE Integer Reduction
  // SVE integer add reduction (predicated)
  // XXX:
  // SVE integer min/max reduction (predicated)
  void smaxv(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerMinMaxReduction(Op, 0, 0, size, pg, zn, zd);
  }
  void umaxv(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerMinMaxReduction(Op, 0, 1, size, pg, zn, zd);
  }
  void sminv(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerMinMaxReduction(Op, 1, 0, size, pg, zn, zd);
  }
  void uminv(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0000'1000'001 << 13;
    SVEIntegerMinMaxReduction(Op, 1, 1, size, pg, zn, zd);
  }

  // SVE constructive prefix (predicated)
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::PRegisterZero, T> || std::is_same_v<FEXCore::ARMEmitter::PRegisterMerge, T>)
  void movprfx(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, T pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t M = std::is_same_v<FEXCore::ARMEmitter::PRegisterMerge, T> ? 1 : 0;
    constexpr uint32_t Op = 0b0000'0100'0001'0000'001 << 13;
    SVEConstructivePrefixPredicated(Op, 0b00, M, size, pg, zn, zd);
  }

  // SVE bitwise logical reduction (predicated)
  // XXX

  // SVE Bitwise Shift - Predicated
  // SVE bitwise shift by immediate (predicated)
  void asr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b00, 0, 0, pg, tszl, imm3, zd);
  }
  void lsr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b00, 0, 1, pg, tszl, imm3, zd);
  }
  void lsl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b00, 1, 1, pg, tszl, imm3, zd);
  }
  void asrd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b01, 0, 0, pg, tszl, imm3, zd);
  }
  void sqshl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b01, 1, 0, pg, tszl, imm3, zd);
  }
  void uqshl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b01, 1, 1, pg, tszl, imm3, zd);
  }
  void srshr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b11, 0, 0, pg, tszl, imm3, zd);
  }
  void urshr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b11, 0, 1, pg, tszl, imm3, zd);
  }
  void sqshlu(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 8, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 16, "Incorrect shift");
      tszh = 0b00;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 32, "Incorrect shift");
      tszh = 0b01;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift >= 0 && Shift < 64, "Incorrect shift");
      tszh = 0b10 | ((InverseShift >> 5) & 1);
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVEBitWiseShiftImmediatePred(tszh, 0b11, 1, 1, pg, tszl, imm3, zd);
  }

  void SVEBitWiseShiftImmediatePred(uint32_t tszh, uint32_t opc, uint32_t L, uint32_t U, FEXCore::ARMEmitter::PRegister pg, uint32_t tszl, uint32_t imm3, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0000'0100'0000'0000'100 << 13;
    uint32_t Instr = Op;

    Instr |= tszh << 22;
    Instr |= opc << 18;
    Instr |= L << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= tszl << 8;
    Instr |= imm3 << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // SVE bitwise shift by vector (predicated)
  void asr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(0, 0, 0, size, pg, zm, zd);
  }

  void lsr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(0, 0, 1, size, pg, zm, zd);
  }
  void lsl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(0, 1, 1, size, pg, zm, zd);
  }

  void asrr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(1, 0, 0, size, pg, zm, zd);
  }
  void lsrr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(1, 0, 1, size, pg, zm, zd);
  }

  void lslr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEBitwiseShiftbyVector(1, 1, 1, size, pg, zm, zd);
  }

  void SVEBitwiseShiftbyVector(uint32_t R, uint32_t L, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0000'0100'0001'0000'100 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= R << 18;
    Instr |= L << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE bitwise shift by wide elements (predicated)
  // XXX:

  // SVE Integer Unary Arithmetic - Predicated
  // SVE integer unary operations (predicated)
  void sxtb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b000, size, pg, zn, zd);
  }
  void uxtb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b001, size, pg, zn, zd);
  }
  void sxth(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b010, size, pg, zn, zd);
  }
  void uxth(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b011, size, pg, zn, zd);
  }
  void sxtw(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b100, size, pg, zn, zd);
  }
  void uxtw(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b101, size, pg, zn, zd);
  }
  void abs(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b110, size, pg, zn, zd);
  }
  void neg(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'0000'101 << 13;;
    SVEIntegerUnaryPredicated(Op, 0b111, size, pg, zn, zd);
  }

  // SVE bitwise unary operations (predicated)
  void cls(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b000, size, pg, zn, zd);
  }
  void clz(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b001, size, pg, zn, zd);
  }
  void cnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b010, size, pg, zn, zd);
  }
  void cnot(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b011, size, pg, zn, zd);
  }
  void fabs(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b100, size, pg, zn, zd);
  }
  void fneg(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b101, size, pg, zn, zd);
  }
  void not_(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0100'0001'1000'101 << 13;
    SVEIntegerUnaryPredicated(Op, 0b110, size, pg, zn, zd);
  }

  // SVE Bitwise Logical - Unpredicated
  // SVE bitwise logical operations (unpredicated)
  void and_(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'00 << 10;
    SVEBitwiseLogicalUnpredicated(Op, 0b00, zm, zn, zd);
  }
  void orr(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'00 << 10;
    SVEBitwiseLogicalUnpredicated(Op, 0b01, zm, zn, zd);
  }
  void mov(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'00 << 10;
    SVEBitwiseLogicalUnpredicated(Op, 0b01, zn, zn, zd);
  }
  void eor(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'00 << 10;
    SVEBitwiseLogicalUnpredicated(Op, 0b10, zm, zn, zd);
  }
  void bic(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'00 << 10;
    SVEBitwiseLogicalUnpredicated(Op, 0b11, zm, zn, zd);
  }

  // SVE2 bitwise ternary operations
  void eor3(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b00, 0, zm, zk, zd);
  }
  void bsl(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b00, 1, zm, zk, zd);
  }
  void bcax(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b01, 0, zm, zk, zd);
  }
  void bsl1n(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b01, 1, zm, zk, zd);
  }
  void bsl2n(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b10, 1, zm, zk, zd);
  }
  void nbsl(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVE2BitwiseTernary(0b11, 1, zm, zk, zd);
  }

  void SVE2BitwiseTernary(uint32_t opc, uint32_t o2, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zk, FEXCore::ARMEmitter::ZRegister zdn) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0011'1 << 11;
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= zm.Idx() << 16;
    Instr |= o2 << 10;
    Instr |= zk.Idx() << 5;
    Instr |= zdn.Idx();
    dc32(Instr);
  }
  // SVE Index Generation
  // XXX:

  // SVE Stack Allocation
  // SVE stack frame adjustment
  // XXX:
  // Streaming SVE stack frame adjustment
  // XXX:
  // SVE stack frame size
  // XXX:
  // Streaming SVE stack frame size
  // XXX:

  // SVE2 Integer Multiply - Unpredicated
  // SVE2 integer multiply vectors (unpredicated)
  void mul(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVE2IntegerMultiplyVectors(0b00, size, zm, zn, zd);
  }
  void smulh(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVE2IntegerMultiplyVectors(0b10, size, zm, zn, zd);
  }

  void umulh(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVE2IntegerMultiplyVectors(0b11, size, zm, zn, zd);
  }

  void pmul(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    SVE2IntegerMultiplyVectors(0b01, FEXCore::ARMEmitter::SubRegSize::i8Bit, zm, zn, zd);
  }

  void SVE2IntegerMultiplyVectors(uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0000'0100'0010'0000'0110 << 12;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // XXX:
  // SVE2 signed saturating doubling multiply high (unpredicated)
  // XXX:

  // SVE Bitwise Shift - Unpredicated
  // SVE bitwise shift by wide elements (unpredicated)
  // XXX:
  // SVE bitwise shift by immediate (unpredicated)
  // XXX:

  // SVE Integer Misc - Unpredicated
  // SVE floating-point trig select coefficient
  // XXX:
  // SVE floating-point exponential accelerator
  // XXX:
  // SVE constructive prefix (unpredicated)
  void movprfx(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    const uint32_t Op = 0b0000'0100'0010'0000'1011'11 << 10;
    SVEConstructivePrefixUnpredicated(Op, 0b00, 0b00000, zn, zd);
  }

  // SVE Element Count
  // SVE saturating inc/dec vector by element count
  // XXX:
  // SVE element count
  // XXX:
  // SVE inc/dec vector by element count
  // XXX:
  // SVE inc/dec register by element count
  // XXX:
  // SVE saturating inc/dec register by element count
  // XXX:

  // SVE Bitwise Immediate
  // XXX: DUPM
  // SVE bitwise logical with immediate (unpredicated)

  // SVE Integer Wide Immediate - Predicated
  // XXX: FCPY
  // SVE copy integer immediate (predicated)
  // XXX:
  // SVE Permute Vector - Unpredicated
  void dup(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    constexpr uint32_t Op = 0b0000'0101'0010'0000'0011'10 << 10;
    SVEPermuteUnpredicated(Op, 0b00, 0b000, size, rn, zd);
  }
  void mov(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::Register rn) {
    dup(size, zd, rn);
  }

  // XXX: INSR
  // XXX: INSR SIMD
  // XXX: REV

  // SVE unpack vector elements
  void sunpklo(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0101'0011'0000'0011'10 << 10;
    SVEUnpackVectorElements(Op, 0, 0, size, zn, zd);
  }
  void sunpkhi(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0101'0011'0000'0011'10 << 10;
    SVEUnpackVectorElements(Op, 0, 1, size, zn, zd);
  }
  void uunpklo(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0101'0011'0000'0011'10 << 10;
    SVEUnpackVectorElements(Op, 1, 0, size, zn, zd);
  }
  void uunpkhi(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid subregsize size");
    constexpr uint32_t Op = 0b0000'0101'0011'0000'0011'10 << 10;
    SVEUnpackVectorElements(Op, 1, 1, size, zn, zd);
  }

  // SVE Permute Predicate
  // XXX: REV (predicate)
  // sve unpack predicate elements
  // XXX:
  // SVE permute predicate elements
  // XXX:

  // SVE Permute Vector - Predicated - Base
  // XXX: CPY (SIMD&FP scalar)
  void compact(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid size");
    constexpr uint32_t Op = 0b0000'0101'0010'0001'100 << 13;

    const uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 : 0b00;

    uint32_t Instr = Op;

    Instr |= ConvertedSize << 22;
    Instr |= 0 << 20; // op0
    Instr |= 0b000 << 17; // op1
    Instr |= 0 << 16; // op2
    Instr |= 0 << 13; // op3
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // XXX: CPY (scalar)

  template<FEXCore::ARMEmitter::OpType optype>
  requires(optype == FEXCore::ARMEmitter::OpType::Constructive)
  void splice(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pv, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zn2) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");

    constexpr uint32_t Op = 0b0000'0101'0010'1101'100 << 13;

    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= FEXCore::ToUnderlying(optype) << 16;
    Instr |= pv.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  template<FEXCore::ARMEmitter::OpType optype>
  requires(optype == FEXCore::ARMEmitter::OpType::Destructive)
  void splice(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pv, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");

    constexpr uint32_t Op = 0b0000'0101'0010'1100'100 << 13;

    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= FEXCore::ToUnderlying(optype) << 16;
    Instr |= pv.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE Permute Vector - Predicated
  // XXX:
  // XXX: LASTA
  // XXX: LASTB
  // SVE extract element to SIMD&FP scalar register
  // XXX:
  // SVE reverse within elements
  void revb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8-bit size");
    SVEReverseWithinElements(0b00, size, pg, zn, zd);
  }

  void revh(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit &&
                        size != FEXCore::ARMEmitter::SubRegSize::i8Bit &&
                        size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "Can't use 8/16/128-bit size");

    SVEReverseWithinElements(0b01, size, pg, zn, zd);
  }

  void revw(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/16/32/128-bit size");
    SVEReverseWithinElements(0b10, size, pg, zn, zd);
  }

  void rbit(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEReverseWithinElements(0b11, size, pg, zn, zd);
  }

  // SVE conditionally broadcast element to vector
  // XXX:
  // SVE conditionally extract element to SIMD&FP scalar
  // XXX:
  // SVE reverse doublewords
  // XXX:
  // SVE conditionally extract element to general register
  // XXX:

  // SVE Permute Vector - Extract
  // Constructive
  template<FEXCore::ARMEmitter::OpType optype>
  requires(optype == FEXCore::ARMEmitter::OpType::Constructive)
  void ext(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zn2, uint8_t Imm) {
    LOGMAN_THROW_A_FMT((zn.Idx() + 1) == zn2.Idx(), "zn needs to be consecutive");
    SVEPermuteVector(1, zd, zn, Imm);
  }

  // Destructive
  template<FEXCore::ARMEmitter::OpType optype>
  requires(optype == FEXCore::ARMEmitter::OpType::Destructive)
  void ext(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm, uint8_t Imm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    SVEPermuteVector(0, zd, zm, Imm);
  }

  void SVEPermuteVector(uint32_t op0, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zm, uint32_t Imm) {
    constexpr uint32_t Op = 0b0000'0101'0010'0000'000 << 13;
    uint32_t Instr = Op;

    Instr |= op0 << 22;
    Instr |= (Imm >> 3) << 16;
    Instr |= (Imm & 0b111) << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE Permute Vector - Segments
  // SVE permute vector segments
  // XXX:

  // SVE Integer Compare - Vectors
  // SVE integer compare vectors
  void cmpeq(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(1, 1, 0, size, zm, pg, zn, pd);
  }
  void cmpge(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(1, 0, 0, size, zm, pg, zn, pd);
  }
  void cmpgt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(1, 0, 1, size, zm, pg, zn, pd);
  }
  void cmphi(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(0, 0, 1, size, zm, pg, zn, pd);
  }
  void cmphs(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(0, 0, 0, size, zm, pg, zn, pd);
  }
  void cmpne(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    SVEIntegerCompareVector(1, 1, 1, size, zm, pg, zn, pd);
  }

  void SVEIntegerCompareVector(uint32_t op, uint32_t o2, uint32_t ne, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::PRegister pd) {
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
  // SVE integer compare with wide elements
  // XXX:

  // SVE Propagate Break
  // SVE propagate break from previous partition
  // XXX:

  // SVE Predicate Misc
  // XXX:
  // XXX: PNEXT
  // SVE predicate test
  void ptest(FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pn) {
    constexpr uint32_t Op = 0b0010'0101'0001'0000'11 << 14;
    SVEPredicateTest(Op, 0, 1, 0b0000, pg, pn);
  }

  // SVE predicate first active
  void pfirst(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pn) {
    LOGMAN_THROW_A_FMT(pd == pn, "pd and pn need to be the same");
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1100 << 12;
    SVEPredicateReadFFRPredicated(Op, 0, 1, pg, pd);
  }

  // SVE predicate zero
  void pfalse(FEXCore::ARMEmitter::PRegister pd) {
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1110'01 << 10;
    SVEPredicateReadFFR(Op, 0, 0, pd);
  }

  // SVE predicate read from FFR (predicated)
  void rdffr(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pg) {
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1111 << 12;
    SVEPredicateReadFFRPredicated(Op, 0, 0, pg, pd);
  }

  void rdffrs(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PRegister pg) {
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1111 << 12;
    SVEPredicateReadFFRPredicated(Op, 0, 1, pg, pd);
  }

  // SVE predicate read from FFR (unpredicated)
  void rdffr(FEXCore::ARMEmitter::PRegister pd) {
    constexpr uint32_t Op = 0b0010'0101'0001'1001'1111 << 12;
    SVEPredicateReadFFR(Op, 0, 0, pd);
  }

  // SVE predicate initialize
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ptrue(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PredicatePattern pattern) {
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1110 << 12;
    SVEPredicateInit(Op, size, 0, pattern, pd);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ptrues(FEXCore::ARMEmitter::PRegister pd, FEXCore::ARMEmitter::PredicatePattern pattern) {
    constexpr uint32_t Op = 0b0010'0101'0001'1000'1110 << 12;
    SVEPredicateInit(Op, size, 1, pattern, pd);
  }

  // SVE Integer Compare - Scalars
  // SVE integer compare scalar count and limit
  // XXX:
  // SVE conditionally terminate scalars
  // XXX:
  // SVE pointer conflict compare
  // XXX:

  // SVE Integer Wide Immediate - Unpredicated
  // SVE integer add/subtract immediate (unpredicated)
  // XXX:
  // SVE integer min/max immediate (unpredicated)
  // XXX:
  // SVE integer multiply immediate (unpredicated)
  // XXX:
  // SVE broadcast integer immediate (unpredicated)
  void dup_imm(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, int32_t Value, bool LSL8 = false) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_AA_FMT(Value >= -128 && Value <= 127, "Immediate out of range");
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(LSL8 == false, "Can't shift immediate with 8-bit elements");
    }
    SVEBroadcastImm(0b00, LSL8, Value, size, zd);
  }
  void mov_imm(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, int32_t Value, bool LSL8 = false) {
    dup_imm(size, zd, Value, LSL8);
  }

  void SVEBroadcastImm(uint32_t opc, uint32_t sh, uint32_t imm, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0010'0101'0011'1000'110 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= sh << 13;
    Instr |= (imm & 0xFF) << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE broadcast floating-point immediate (unpredicated)
  void fdup(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, float Value) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Unsupported fmov size");
    uint32_t Imm;
    if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(vixl::aarch64::Assembler::IsImmFP16(vixl::Float16(Value)), "Invalid float");
      Imm = vixl::VFP::FP16ToImm8(vixl::Float16(Value));
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP32(Value), "Invalid float");
      Imm = vixl::VFP::FP32ToImm8(Value);

    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP64(Value), "Invalid float");
      Imm = vixl::VFP::FP64ToImm8(Value);
    }
    else {
      LOGMAN_MSG_A_FMT("Invalid subregsize");
      FEX_UNREACHABLE;
    }

    SVEBroadcastFloatImm(0b00, 0, Imm, size, zd);
  }
  void fmov(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, float Value) {
    fdup(size, zd, Value);
  }

  void SVEBroadcastFloatImm(uint32_t opc, uint32_t o2, uint32_t imm, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0010'0101'0011'1001'110 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= o2 << 13;
    Instr |= imm << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // XXX:

  // SVE Predicate Count
  // SVE predicate count
  // XXX:

  // SVE Inc/Dec by Predicate Count
  // SVE saturating inc/dec vector by predicate count
  // XXX:
  // SVE saturating inc/dec register by predicate count
  // XXX:
  // SVE inc/dec vector by predicate count
  // XXX:
  // SVE inc/dec register by predicate count
  // XXX:

  // SVE Write FFR
  // SVE FFR write from predicate
  // XXX:
  // SVE FFR initialise
  // XXX:

  // SVE Integer Multiply-Add - Unpredicated
  // XXX: CDOT
  // SVE integer dot product (unpredicated)
  // XXX:
  // SVE2 saturating multiply-add interleaved long
  // XXX:
  // SVE2 complex integer multiply-add
  // XXX:
  // SVE2 integer multiply-add long
  // XXX:
  // SVE2 saturating multiply-add long
  // XXX:
  // SVE2 saturating multiply-add high
  // XXX:
  // SVE mixed sign dot product
  // XXX:

  // SVE2 Integer - Predicated
  // SVE2 integer pairwise add and accumulate long
  // XXX:
  // SVE2 integer unary operations (predicated)
  // XXX:
  // SVE2 saturating/rounding bitwise shift left (predicated)
  // XXX
  // SVE2 integer halving add/subtract (predicated)
  void shadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 0, 0, 0, size, pg, zm, zd);
  }
  void uhadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 0, 0, 1, size, pg, zm, zd);
  }
  void shsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 0, 1, 0, size, pg, zm, zd);
  }
  void uhsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 0, 1, 1, size, pg, zm, zd);
  }
  void srhadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 1, 0, 0, size, pg, zm, zd);
  }
  void urhadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 1, 0, 1, size, pg, zm, zd);
  }
  void shsubr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 1, 1, 0, size, pg, zm, zd);
  }
  void uhsubr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'100 << 13;
    SVE2IntegerHalvingPredicated(Op, 1, 1, 1, size, pg, zm, zd);
  }

  // SVE2 integer pairwise arithmetic
  void addp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'101 << 13;
    SVEIntegerPairwiseArithmetic(Op, 0b00, 1, size, pg, zm, zd);
  }
  void smaxp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'101 << 13;
    SVEIntegerPairwiseArithmetic(Op, 0b10, 0, size, pg, zm, zd);
  }
  void umaxp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'101 << 13;
    SVEIntegerPairwiseArithmetic(Op, 0b10, 1, size, pg, zm, zd);
  }
  void sminp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'101 << 13;
    SVEIntegerPairwiseArithmetic(Op, 0b11, 0, size, pg, zm, zd);
  }
  void uminp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    constexpr uint32_t Op = 0b0100'0100'0001'0000'101 << 13;
    SVEIntegerPairwiseArithmetic(Op, 0b11, 1, size, pg, zm, zd);
  }

  // SVE2 saturating add/subtract
  // XXX:
  //
  // SVE2 Widening Integer Arithmetic
  // SVE2 integer add/subtract long
  void saddlb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 0, 0, 0, size, zd, zn, zm);
  }

  void saddlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 0, 0, 1, size, zd, zn, zm);
  }

  void uaddlb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 0, 1, 0, size, zd, zn, zm);
  }

  void uaddlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 0, 1, 1, size, zd, zn, zm);
  }

  void ssublb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 1, 0, 0, size, zd, zn, zm);
  }

  void ssublt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 1, 0, 1, size, zd, zn, zm);
  }

  void usublb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 1, 1, 0, size, zd, zn, zm);
  }

  void usublt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(0, 1, 1, 1, size, zd, zn, zm);
  }

  void sabdlb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(1, 1, 0, 0, size, zd, zn, zm);
  }

  void sabdlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(1, 1, 0, 1, size, zd, zn, zm);
  }

  void uabdlb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(1, 1, 1, 0, size, zd, zn, zm);
  }

  void uabdlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerAddSubLong(1, 1, 1, 1, size, zd, zn, zm);
  }

  void SVE2IntegerAddSubLong(uint32_t op, uint32_t S, uint32_t U, uint32_t T, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0100'0101'0000'0000'00 << 14;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 13;
    Instr |= S << 12;
    Instr |= U << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // SVE2 integer add/subtract wide
  // XXX:
  // SVE2 integer multiply long
  void sqdmullb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(0, 0, 0, size, zd, zn, zm);
  }
  void sqdmullt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(0, 0, 1, size, zd, zn, zm);
  }
  void pmullb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(0, 1, 0, size, zd, zn, zm);
  }
  void pmullt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(0, 1, 1, size, zd, zn, zm);
  }
  void smullb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(1, 0, 0, size, zd, zn, zm);
  }
  void smullt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(1, 0, 1, size, zd, zn, zm);
  }
  void umullb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(1, 1, 0, size, zd, zn, zm);
  }
  void umullt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Can't use 8/128-bit size");
    SVE2IntegerMultiplyLong(1, 1, 1, size, zd, zn, zm);
  }

  void SVE2IntegerMultiplyLong(uint32_t op, uint32_t U, uint32_t T, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zm) {
    constexpr uint32_t Op = 0b0100'0101'0000'0000'011 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 12;
    Instr |= U << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  //
  // SVE Misc
  // SVE2 bitwise shift left long
  // XXX:
  // SVE2 integer add/subtract interleaved long
  // XXX:
  // SVE2 bitwise exclusive-or interleaved
  // XXX:
  // SVE integer matrix multiply accumulate
  // XXX:
  // SVE2 bitwise permute
  // XXX:
  //
  // SVE2 Accumulate
  // SVE2 complex integer add
  // XXX:
  // SVE2 integer absolute difference and accumulate long
  // XXX:
  // SVE2 integer add/subtract long with carry
  // XXX:
  // SVE2 bitwise shift right and accumulate
  // XXX:
  // SVE2 bitwise shift and insert
  // XXX:
  // SVE2 integer absolute difference and accumulate
  // XXX:
  //
  // SVE2 Narrowing
  // SVE2 saturating extract narrow
  void sqxtnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b00, 0, zn, zd);
  }
  void sqxtnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b00, 1, zn, zd);
  }
  void uqxtnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b01, 0, zn, zd);
  }
  void uqxtnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b01, 1, zn, zd);
  }
  void sqxtunb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b10, 0, zn, zd);
  }
  void sqxtunt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 64/128-bit size");
    uint32_t tszh, tszl;

    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      tszh = 0;
      tszl = 0b01;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      tszh = 0;
      tszl = 0b10;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      tszh = 1;
      tszl = 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2SaturatingExtractNarrow(tszh, tszl, 0b10, 1, zn, zd);
  }

  void SVE2SaturatingExtractNarrow(uint32_t tszh, uint32_t tszl, uint32_t opc, uint32_t T, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0100'0101'0010'0000'010 << 13;
    uint32_t Instr = Op;

    Instr |= tszh << 22;
    Instr |= tszl << 19;
    Instr |= opc << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE2 bitwise shift right narrow
  void sqshrunb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 0, 0, 0, zn, zd);
  }
  void sqshrunt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 0, 0, 1, zn, zd);
  }
  void sqrshrunb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 0, 1, 0, zn, zd);
  }
  void sqrshrunt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 0, 1, 1, zn, zd);
  }
  void shrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 1, 0, 0, zn, zd);
  }
  void shrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 1, 0, 1, zn, zd);
  }
  void rshrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 1, 1, 0, zn, zd);
  }
  void rshrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 0, 1, 1, 1, zn, zd);
  }
  void sqshrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 0, 0, 0, zn, zd);
  }
  void sqshrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 0, 0, 1, zn, zd);
  }
  void sqrshrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 0, 1, 0, zn, zd);
  }
  void sqrshrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 0, 1, 1, zn, zd);
  }
  void uqshrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 1, 0, 0, zn, zd);
  }
  void uqshrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 1, 0, 1, zn, zd);
  }
  void uqrshrnb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 1, 1, 0, zn, zd);
  }
  void uqrshrnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::ZRegister zn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Can't use 8/128-bit size");

    uint32_t tszh, tszl;
    uint32_t imm3;
    const uint32_t InverseShift = (2 * SubRegSizeInBits(size)) - Shift;
    if (size == FEXCore::ARMEmitter::SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 8, "Incorrect shift");
      tszh = 0;
      tszl = 0b01;
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 16, "Incorrect shift");
      tszh = 0;
      tszl = 0b10 | ((InverseShift >> 3) & 0b1);
      imm3 = InverseShift & 0b111;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift > 0 && Shift <= 32, "Incorrect shift");
      tszh = 1;
      tszl = (InverseShift >> 3) & 0b11;
      imm3 = InverseShift & 0b111;
    }
    else {
      FEX_UNREACHABLE;
    }

    SVE2BitwiseShiftRightNarrow(tszh, tszl, imm3, 1, 1, 1, 1, zn, zd);
  }

  void SVE2BitwiseShiftRightNarrow(uint32_t tszh, uint32_t tszl, uint32_t imm3, uint32_t op, uint32_t U, uint32_t R, uint32_t T, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0100'0101'0010'0000'00 << 14;
    uint32_t Instr = Op;

    Instr |= tszh << 22;
    Instr |= tszl << 19;
    Instr |= imm3 << 16;
    Instr |= op << 13;
    Instr |= U << 12;
    Instr |= R << 11;
    Instr |= T << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }
  // SVE2 integer add/subtract narrow high part
  // XXX:
  //
  // SVE2 Histogram Computation - Segment
  // XXX:
  //
  // SVE2 Crypto Extensions
  // SVE2 crypto unary operations
  // XXX:
  // SVE2 crypto destructive binary operations
  // XXX:
  // SVE2 crypto constructive binary operations
  // XXX:
  //
  // SVE Floating Point Widening Multiply-Add - Indexed
  // SVE BFloat16 floating-point dot product (indexed)
  // XXX:
  // SVE floating-point multiply-add long (indexed)
  // XXX:
  //
  // SVE Floating Point Widening Multiply-Add
  // SVE BFloat16 floating-point dot product
  // XXX:
  // SVE floating-point multiply-add long
  // XXX:
  //
  // SVE Floating Point Arithmetic - Predicated
  // XXX: FTMAD
  // SVE floating-point arithmetic (predicated)
  void fadd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0000, size, pg, zm, zd);
  }
  void fsub(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0001, size, pg, zm, zd);
  }
  void fmul(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0010, size, pg, zm, zd);
  }
  void fsubr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0011, size, pg, zm, zd);
  }
  void fmaxnm(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0100, size, pg, zm, zd);
  }
  void fminnm(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0101, size, pg, zm, zd);
  }
  void fmax(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0110, size, pg, zm, zd);
  }
  void fmin(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b0111, size, pg, zm, zd);
  }
  void fabd(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b1000, size, pg, zm, zd);
  }
  void fscale(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b1001, size, pg, zm, zd);
  }
  void fmulx(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b1010, size, pg, zm, zd);
  }
  void fdivr(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b1100, size, pg, zm, zd);
  }
  void fdiv(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zdn, FEXCore::ARMEmitter::ZRegister zm) {
    LOGMAN_THROW_A_FMT(zd.Idx() == zdn.Idx(), "Dest needs to equal zdn");
    LOGMAN_THROW_A_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid float size");
    constexpr uint32_t Op = 0b0110'0101'0000'0000'100 << 13;
    SVEFloatArithmeticPredicated(Op, 0b1101, size, pg, zm, zd);
  }
  // SVE floating-point arithmetic with immediate (predicated)
  // XXX:

  // SVE Floating Point Unary Operations - Predicated
  // SVE floating-point round to integral value
  void frinti(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b111, size, zd, pg, zn);
  }
  void frintx(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b110, size, zd, pg, zn);
  }
  void frinta(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b100, size, zd, pg, zn);
  }
  void frintn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b000, size, zd, pg, zn);
  }
  void frintz(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b011, size, zd, pg, zn);
  }
  void frintm(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b010, size, zd, pg, zn);
  }
  void frintp(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    frintX(0b001, size, zd, pg, zn);
  }

  // SVE floating-point convert precision
  // XXX:
  // SVE floating-point unary operations
  void frecpx(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
    SVEFloatUnary(0b00, size, pg, zn, zd);
  }

  void fsqrt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
    SVEFloatUnary(0b01, size, pg, zn, zd);
  }

  void SVEFloatUnary(uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0110'0101'0000'1100'101 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE integer convert to floating-point
  // XXX:
  // SVE floating-point convert to integer
  void flogb(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0101'0001'1000'101 << 13;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 : 0b00;

    SVEFloatConvertToInt(Op, 0b00, ConvertedSize, 0, pg, zn, zd);
  }

  void scvtf(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::SubRegSize dstsize, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::SubRegSize srcsize) {
    LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    LOGMAN_THROW_AA_FMT(srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0101'0001'0000'101 << 13;
    uint32_t opc1, opc2;
    if (srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = 0b01;
      opc2 = 0b01;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      opc1 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 :0b00;

      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b00 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b10 :0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      // SrcSize = fp64, opc2 encodes dst size
      opc1 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 :0b00;
      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b11 :0b00;
    }
    else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(Op, opc1, opc2, 0, pg, zn, zd);
  }
  void ucvtf(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::SubRegSize dstsize, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::SubRegSize srcsize) {
    LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    LOGMAN_THROW_AA_FMT(srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0101'0001'0000'101 << 13;
    uint32_t opc1, opc2;
    if (srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc1 = 0b01;
      opc2 = 0b01;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      opc1 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 :0b00;

      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b00 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b10 :0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      // SrcSize = fp64, opc2 encodes dst size
      opc1 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 :0b00;
      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b11 :0b00;
    }
    else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(Op, opc1, opc2, 1, pg, zn, zd);
  }
  void fcvtzs(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::SubRegSize dstsize, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::SubRegSize srcsize) {
    LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    LOGMAN_THROW_AA_FMT(srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0101'0001'1000'101 << 13;
    uint32_t opc1, opc2;
    if (srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      opc1 = 0b01;
      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 : 0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      LOGMAN_THROW_AA_FMT(dstsize != FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc2 = 0b10;
      opc1 = dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 : 0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(dstsize != FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = 0b11;
      opc2 = dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b00 : 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(Op, opc1, opc2, 0, pg, zn, zd);
  }
  void fcvtzu(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::SubRegSize dstsize, FEXCore::ARMEmitter::PRegisterMerge pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::SubRegSize srcsize) {
    LOGMAN_THROW_AA_FMT(dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    LOGMAN_THROW_AA_FMT(srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    constexpr uint32_t Op = 0b0110'0101'0001'1000'101 << 13;
    uint32_t opc1, opc2;
    if (srcsize == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Srcsize = fp16, opc2 encodes dst size
      opc1 = 0b01;
      opc2 =
        dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b01 : 0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Srcsize = fp32, opc1 encodes dst size
      LOGMAN_THROW_AA_FMT(dstsize != FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      opc2 = 0b10;
      opc1 = dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b10 : 0b00;
    }
    else if (srcsize == FEXCore::ARMEmitter::SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(dstsize != FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);
      // SrcSize = fp64, opc2 encodes dst size
      opc1 = 0b11;
      opc2 = dstsize == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b11 :
        dstsize == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b00 : 0b00;
    }
    else {
      FEX_UNREACHABLE;
    }
    SVEFloatConvertToInt(Op, opc1, opc2, 1, pg, zn, zd);
  }

  // SVE Floating Point Unary Operations - Unpredicated
  // SVE floating-point reciprocal estimate (unpredicated)
  // XXX:

  // SVE Floating Point Compare - with Zero
  // SVE floating-point compare with zero
  // XXX:

  // SVE Floating Point Accumulating Reduction
  // SVE floating-point serial reduction (predicated)
  // XXX:

  // SVE Floating Point Multiply-Add
  // SVE floating-point multiply-accumulate writing addend
  // XXX:
  // SVE floating-point multiply-accumulate writing multiplicand
  // XXX:

  // SVE Memory - 32-bit Gather and Unsized Contiguous
  void ldr(FEXCore::ARMEmitter::PRegister pt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -256 && Imm <= 255, "Immediate offset too large");
    constexpr uint32_t Op = 0b1000'0101'10 << 22;
    SVEGatherAndUnsizedContiguous(Op, Imm & 0b1'1111'1111, pt, rn);
  }
  // XXX: LDR (vector)

  // SVE 32-bit gather prefetch (scalar plus 32-bit scaled offsets)
  // XXX:
  // SVE 32-bit gather load halfwords (scalar plus 32-bit scaled offsets)
  // XXX:
  // SVE 32-bit gather load words (scalar plus 32-bit scaled offsets)
  // XXX:
  // SVE contiguous prefetch (scalar plus immediate)
  // XXX:
  // SVE 32-bit gather load (scalar plus 32-bit unscaled offsets)
  // XXX:
  // SVE2 32-bit gather non-temporal load (vector plus scalar)
  // XXX:
  // SVE contiguous prefetch (scalar plus scalar)
  // XXX:
  // SVE 32-bit gather prefetch (vector plus immediate)
  // XXX:
  // SVE 32-bit gather load (vector plus immediate)
  // XXX:
  // SVE load and broadcast element
  // XXX:

  // SVE contiguous non-temporal load (scalar plus immediate)
  // XXX:
  // SVE contiguous non-temporal load (scalar plus scalar)
  // XXX:
  // SVE load multiple structures (scalar plus immediate)
  void ld2b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b01, Imm / 2, zt1, pg, rn);
  }
  void ld3b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b10, Imm / 3, zt1, pg, rn);
  }
  void ld4b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b11, Imm / 4, zt1, pg, rn);
  }
  void ld2h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b01, Imm / 2, zt1, pg, rn);
  }
  void ld3h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b10, Imm / 3, zt1, pg, rn);
  }
  void ld4h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b11, Imm / 4, zt1, pg, rn);
  }
  void ld2w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b01, Imm / 2, zt1, pg, rn);
  }
  void ld3w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b10, Imm / 3, zt1, pg, rn);
  }
  void ld4w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b11, Imm / 4, zt1, pg, rn);
  }
  void ld2d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b01, Imm / 2, zt1, pg, rn);
  }
  void ld3d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b10, Imm / 3, zt1, pg, rn);
  }
  void ld4d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1010'0100'0000'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b11, Imm / 4, zt1, pg, rn);
  }

  // SVE helper implementations
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  void ld1sw(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1sw(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1sw(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sh(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1sh<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1sh<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sb(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1sb<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1sb<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  void ld1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      ld1d(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      ld1d(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      st1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      st1b<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      st1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      st1h<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      st1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      st1w<size>(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  void st1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::SVEMemOperand Src) {
    if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_SCALAR) {
      st1d(zt, pg, Src.rn, Src.MetaType.ScalarScalarType.rm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_IMM) {
      st1d(zt, pg, Src.rn, Src.MetaType.ScalarImmType.Imm);
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_SCALAR_VECTOR) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else if (Src.MetaType.Header.MemType == FEXCore::ARMEmitter::SVEMemOperand::Type::TYPE_VECTOR_IMM) {
      LOGMAN_THROW_A_FMT(false, "Not yet implemented");
    }
    else {
      FEX_UNREACHABLE;
    }
  }

  // SVE load multiple structures (scalar plus scalar)
  // XXX:
  // SVE load and broadcast quadword (scalar plus immediate)
  // XXX:
  // SVE contiguous load (scalar plus immediate)
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    SVEContiguousLoadImm(Op, 0b0000 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  void ld1sw(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    SVEContiguousLoadImm(Op, 0b0100, Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    SVEContiguousLoadImm(Op, 0b0100 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sh(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 1 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0 : -1;

    SVEContiguousLoadImm(Op, 0b1000 | ConvertedSize, Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 1 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0 : -1;

    SVEContiguousLoadImm(Op, 0b1010 | ConvertedSize, Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sb(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b01 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b00 : -1;
    SVEContiguousLoadImm(Op, 0b1100 | ConvertedSize, Imm, pg, rn, zt);
  }
  void ld1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'101 << 13;
    SVEContiguousLoadImm(Op, 0b1111, Imm, pg, rn, zt);
  }

  // SVE contiguous non-fault load (scalar plus immediate)
  // XXX:
  // SVE load and broadcast quadword (scalar plus scalar)
  // XXX:
  // SVE contiguous load (scalar plus scalar)
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b0000 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  void ld1sw(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b0100, rm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b0100 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sh(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 1 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0 : -1;
    SVEContiguousLoadStore(Op, 0b1000 | ConvertedSize, rm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 1 : -1;
    SVEContiguousLoadStore(Op, 0b1010 | ConvertedSize, rm, pg, rn, zt);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1sb(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? 0b10 :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0b01 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 0b00 : -1;
    SVEContiguousLoadStore(Op, 0b1100 | ConvertedSize, rm, pg, rn, zt);
  }

  void ld1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegisterZero pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b1010'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b1111, rm, pg, rn, zt);
  }

  // SVE contiguous first-fault load (scalar plus scalar)
  // XXX:

  // SVE Memory - 64-bit Gather
  // SVE 64-bit gather prefetch (scalar plus 64-bit scaled offsets)
  // XXX:
  // SVE 64-bit gather prefetch (scalar plus unpacked 32-bit scaled offsets)
  // XXX:
  // SVE 64-bit gather load (scalar plus 64-bit scaled offsets)
  // XXX:
  // SVE 64-bit gather load (scalar plus 32-bit unpacked scaled offsets)
  // XXX:
  // SVE 64-bit gather prefetch (vector plus immediate)
  // XXX:
  // SVE2 64-bit gather non-temporal load (vector plus scalar)
  // XXX:
  // SVE 64-bit gather load (vector plus immediate)
  // XXX:
  // SVE 64-bit gather load (scalar plus 64-bit unscaled offsets)
  // XXX:
  // SVE 64-bit gather load (scalar plus unpacked 32-bit unscaled offsets)
  // XXX:

  // SVE Memory - Contiguous Store and Unsized Contiguous
  // XXX: STR (predicate)
  // XXX: STR (vector)
  //
  // SVE contiguous store (scalar plus scalar)
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b1110'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b0000 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'010 << 13;
    SVEContiguousLoadStore(Op, 0b0100 | FEXCore::ToUnderlying(size), rm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'010 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 1 : -1;

    SVEContiguousLoadStore(Op, 0b1010 | ConvertedSize, rm, pg, rn, zt);
  }
  void st1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b1110'0100'0000'0000'010 << 13;

    SVEContiguousLoadStore(Op, 0b1111, rm, pg, rn, zt);
  }

  // SVE Memory - Non-temporal and Multi-register Store
  // SVE2 64-bit scatter non-temporal store (vector plus scalar)
  // XXX:
  // SVE contiguous non-temporal store (scalar plus scalar)
  // XXX:
  // SVE2 32-bit scatter non-temporal store (vector plus scalar)
  // XXX:
  // SVE store multiple structures (scalar plus scalar)
  // XXX:

  // SVE Memory - Scatter with Optional Sign Extend
  // SVE 64-bit scatter store (scalar plus unpacked 32-bit unscaled offsets)
  // XXX:
  // SVE 64-bit scatter store (scalar plus unpacked 32-bit scaled offsets)
  // XXX:
  // SVE 32-bit scatter store (scalar plus 32-bit unscaled offsets)
  // XXX:
  // SVE 32-bit scatter store (scalar plus 32-bit scaled offsets)
  // XXX:

  // SVE Memory - Scatter
  // SVE 64-bit scatter store (scalar plus 64-bit unscaled offsets)
  // XXX:
  // SVE 64-bit scatter store (scalar plus 64-bit scaled offsets)
  // XXX:
  // SVE 64-bit scatter store (vector plus immediate)
  // XXX:
  // SVE 32-bit scatter store (vector plus immediate)
  // XXX:

  // SVE Memory - Contiguous Store with Immediate Offset
  // SVE contiguous non-temporal store (scalar plus immediate)
  // XXX:
  // SVE store multiple structures (scalar plus immediate)
  void st2b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b01, Imm / 2, zt1, pg, rn);
  }
  void st3b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b10, Imm / 3, zt1, pg, rn);
  }
  void st4b(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b00, 0b11, Imm / 4, zt1, pg, rn);
  }
  void st2h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b01, Imm / 2, zt1, pg, rn);
  }
  void st3h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b10, Imm / 3, zt1, pg, rn);
  }
  void st4h(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b01, 0b11, Imm / 4, zt1, pg, rn);
  }
  void st2w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b01, Imm / 2, zt1, pg, rn);
  }
  void st3w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b10, Imm / 3, zt1, pg, rn);
  }
  void st4w(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b10, 0b11, Imm / 4, zt1, pg, rn);
  }
  void st2d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -16 && Imm <= 14 && ((Imm % 2) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b01, Imm / 2, zt1, pg, rn);
  }
  void st3d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -24 && Imm <= 21 && ((Imm % 3) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b10, Imm / 3, zt1, pg, rn);
  }
  void st4d(FEXCore::ARMEmitter::ZRegister zt1, FEXCore::ARMEmitter::ZRegister zt2, FEXCore::ARMEmitter::ZRegister zt3, FEXCore::ARMEmitter::ZRegister zt4, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -32 && Imm <= 28 && ((Imm % 4) == 0), "Invalid sized loadstore offset size");
    LOGMAN_THROW_A_FMT((zt1.Idx() + 1) == zt2.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt2.Idx() + 1) == zt3.Idx(), "Registers need to be contiguous");
    LOGMAN_THROW_A_FMT((zt3.Idx() + 1) == zt4.Idx(), "Registers need to be contiguous");

    constexpr uint32_t Op = 0b1110'0100'0001'0000'111 << 13;
    SVEContiguousMultipleStructures(Op, 0b11, 0b11, Imm / 4, zt1, pg, rn);
  }

  // SVE contiguous store (scalar plus immediate)
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1b(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'111 << 13;
    SVEContiguousLoadImm(Op, 0b0000 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1h(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'111 << 13;
    SVEContiguousLoadImm(Op, 0b0100 | FEXCore::ToUnderlying(size), Imm, pg, rn, zt);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1w(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'111 << 13;
    constexpr uint32_t ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? 0 :
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? 1 : -1;

    SVEContiguousLoadImm(Op, 0b1010 | ConvertedSize, Imm, pg, rn, zt);
  }

  void st1d(FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(Imm >= -8 && Imm <= 7, "Invalid sized loadstore offset size");
    constexpr uint32_t Op = 0b1110'0100'0000'0000'111 << 13;
    SVEContiguousLoadImm(Op, 0b1111, Imm, pg, rn, zt);
  }
private:
  // SVE encodings
  void SVEDup(uint32_t Op, uint32_t imm2, uint32_t tsz, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= imm2 << 22;
    Instr |= tsz << 16;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  void SVESel(uint32_t Op, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::PRegister pv, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= pv.Idx() << 10;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE integer add/subtract vectors (unpredicated)
  void SVEIntegerAddSubUnpredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE table lookup (three sources)
  void SVETableLookup(uint32_t Op, uint32_t op, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i128Bit, "Can't use 128-bit size");
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= op << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE permute vector elements
  void SVEPermute(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE predicate logical operations
  void SVEPredicateLogical(uint32_t Op, uint32_t op, uint32_t S, uint32_t o2, uint32_t o3, FEXCore::ARMEmitter::PRegister pm, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pn, FEXCore::ARMEmitter::PRegister pd) {
    uint32_t Instr = Op;

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
  void SVEFloatConvertOdd(uint32_t Op, uint32_t opc, uint32_t opc2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= opc2 << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE2 floating-point pairwise operations
  void SVEFloatPairwiseArithmetic(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point arithmetic (unpredicated)
  void SVEFloatArithmeticUnpredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= zm.Idx() << 16;
    Instr |= opc << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE bitwise logical operations (predicated)
  void SVEBitwiseLogicalPredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE integer min/max reduction (predicated)
  void SVEIntegerMinMaxReduction(uint32_t Op, uint32_t op, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE constructive prefix (predicated)
  void SVEConstructivePrefixPredicated(uint32_t Op, uint32_t opc, uint32_t M, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= M << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE bitwise unary operations (predicated)
  void SVEIntegerUnaryPredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE bitwise logical operations (unpredicated)
  void SVEBitwiseLogicalUnpredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= zm.Idx() << 16;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE Permute Vector - Unpredicated
  void SVEPermuteUnpredicated(uint32_t Op, uint32_t op0, uint32_t op1, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= op0 << 19;
    Instr |= op1 << 16;
    Instr |= Encode_rn(rn);
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE unpack vector elements
  void SVEUnpackVectorElements(uint32_t Op, uint32_t U, uint32_t H, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= U << 17;
    Instr |= H << 16;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE reverse within elements
  void SVEReverseWithinElements(uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    constexpr uint32_t Op = 0b0000'0101'0010'0100'100 << 13;
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE constructive prefix (unpredicated)
  void SVEConstructivePrefixUnpredicated(uint32_t Op, uint32_t opc, uint32_t opc2, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= opc2 << 16;
    Instr |= Encode_rn(zn);
    Instr |= Encode_rd(zd);
    dc32(Instr);
  }

  // SVE2 integer halving add/subtract (predicated)
  void SVE2IntegerHalvingPredicated(uint32_t Op, uint32_t R, uint32_t S, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= R << 18;
    Instr |= S << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE2 integer pairwise arithmetic
  void SVEIntegerPairwiseArithmetic(uint32_t Op, uint32_t opc, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point arithmetic (predicated)
  void SVEFloatArithmeticPredicated(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zm, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zm.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  void SVECharacterMatch(uint32_t op, uint32_t opc, SubRegSize size, PRegister pd, PRegisterZero pg, ZRegister zn, ZRegister zm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit,
                        "match/nmatch can only use 8-bit or 16-bit element sizes");
    LOGMAN_THROW_AA_FMT(pg <= PReg::p7, "match/nmatch can only use p0-p7 as a governing predicate");

    uint32_t Instr = op;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 4;
    Instr |= zm.Idx() << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEFPRecursiveReduction(uint32_t op, uint32_t opc, SubRegSize size, VRegister vd, PRegister pg, ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                        "FP reduction operation can only use 16-bit, 32-bit, or 64-bit element sizes");
    LOGMAN_THROW_AA_FMT(pg <= PReg::p7, "FP reduction operation can only use p0-p7 as a governing predicate");

    uint32_t Instr = op;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= vd.Idx();
    dc32(Instr);
  }

  // SVE floating-point round to integral value
  void frintX(uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    // opc = round mode
    // 0b000 - N - Neaest ties to even
    // 0b001 - P - Towards +inf
    // 0b010 - M - Towards -inf
    // 0b011 - Z - Towards zero
    // 0b100 - A - Nearest away from zero
    // 0b101 - Unallocated
    // 0b110 - X - Current signalling inexact
    // 0b111 - I - Current
    constexpr uint32_t Op = 0b0110'0101'0000'0000'101 << 13;
    SVEFloatRoundIntegral(Op, opc, size, zd, pg, zn);
  }

  void SVEFloatRoundIntegral(uint32_t Op, uint32_t opc, FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported size in {}", __func__);

    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opc << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE floating-point convert to integer
  void SVEFloatConvertToInt(uint32_t Op, uint32_t opc, uint32_t opc2, uint32_t U, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn, FEXCore::ARMEmitter::ZRegister zd) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= opc2 << 17;
    Instr |= U << 16;
    Instr |= pg.Idx() << 10;
    Instr |= zn.Idx() << 5;
    Instr |= zd.Idx();
    dc32(Instr);
  }

  // SVE Memory - 32-bit Gather and Unsized Contiguous
  void SVEGatherAndUnsizedContiguous(uint32_t Op, uint32_t imm9, FEXCore::ARMEmitter::PRegister pt, FEXCore::ARMEmitter::Register rn) {
    uint32_t Instr = Op;

    Instr |= (imm9 >> 3) << 16;
    Instr |= (imm9 & 0b111) << 10;
    Instr |= rn.Idx() << 5;
    Instr |= pt.Idx();

    dc32(Instr);
  }

  // SVE store multiple structures (scalar plus immediate)
  void SVEContiguousMultipleStructures(uint32_t Op, uint32_t msz, uint32_t opc, uint32_t imm4, FEXCore::ARMEmitter::ZRegister zt, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn) {
    uint32_t Instr = Op;

    Instr |= msz << 23;
    Instr |= opc << 21;
    Instr |= (imm4 & 0xF) << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }

  void SVEContiguousLoadImm(uint32_t Op, uint32_t dtype, uint32_t Imm, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::ZRegister zt) {
    uint32_t Instr = Op;

    Instr |= dtype << 21;
    Instr |= (Imm & 0xF) << 16;
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }

  // zt.b, pg/z, xn, xm
  void SVEContiguousLoadStore(uint32_t Op, uint32_t dtype, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::ZRegister zt) {
    uint32_t Instr = Op;

    Instr |= dtype << 21;
    Instr |= Encode_rm(rm);
    Instr |= pg.Idx() << 10;
    Instr |= Encode_rn(rn);
    Instr |= zt.Idx();
    dc32(Instr);
  }
  void SVEPredicateTest(uint32_t Op, uint32_t op, uint32_t S, uint32_t opc2, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pn) {
    uint32_t Instr = Op;

    Instr |= op << 23;
    Instr |= S << 22;
    Instr |= pg.Idx() << 10;
    Instr |= pn.Idx() << 5;
    Instr |= opc2;
    dc32(Instr);
  }

  void SVEPredicateReadFFRPredicated(uint32_t Op, uint32_t op, uint32_t S, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::PRegister pd) {
    uint32_t Instr = Op;

    Instr |= op << 23;
    Instr |= S << 22;
    Instr |= pg.Idx() << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEPredicateReadFFR(uint32_t Op, uint32_t op, uint32_t S, FEXCore::ARMEmitter::PRegister pd) {
    uint32_t Instr = Op;

    Instr |= op << 23;
    Instr |= S << 16;
    Instr |= pd.Idx();
    dc32(Instr);
  }

  void SVEPredicateInit(uint32_t Op, FEXCore::ARMEmitter::SubRegSize size, uint32_t S, FEXCore::ARMEmitter::PredicatePattern pattern, FEXCore::ARMEmitter::PRegister pd) {
    uint32_t Instr = Op;

    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= S << 16;
    Instr |= FEXCore::ToUnderlying(pattern) << 5;
    Instr |= pd.Idx();
    dc32(Instr);
  }

