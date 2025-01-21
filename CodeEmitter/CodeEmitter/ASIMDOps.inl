// SPDX-License-Identifier: MIT
/* ASIMD instruction emitters.
 *
 * This contains emitters for vector operations explicitly.
 * Most instructions have a `SubRegSize` as their first argument to select element size while operating.
 * Additionally most emitters accept templated vector register arguments of both `QRegister` and `DRegister` types.
 * Based on the combination of those two arguments, it will emit an instruction operating on a 64-bit or 128-bit wide register
 * with the selected element size.
 *
 * Some vector operations are unsized and only operate at the one width. In these cases the instruction only
 * operates at one size, the width depends on the instruction.
 * The arguments for these instructions are usually `VRegister` but might be one of the other sized types as well.
 *
 * Only two instructions support the `i128Bit` ElementSize.
 */
public:
  // Data Processing -- Scalar Floating-Point and Advanced SIMD
  // Cryptographic AES
  void aese(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00100, rd, rn);
  }
  void aesd(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00101, rd, rn);
  }
  void aesmc(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00110, rd, rn);
  }
  void aesimc(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00111, rd, rn);
  }

  // Cryptographic three-register SHA
  void sha1c(ARMEmitter::VRegister rd, ARMEmitter::SRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b000, rd, rn.V(), rm);
  }
  void sha1p(ARMEmitter::VRegister rd, ARMEmitter::SRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b001, rd, rn.V(), rm);
  }
  void sha1m(ARMEmitter::VRegister rd, ARMEmitter::SRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b010, rd, rn.V(), rm);
  }
  void sha1su0(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b011, rd, rn, rm);
  }
  void sha256h(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b100, rd, rn, rm);
  }
  void sha256h2(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b100, rd, rn, rm);
  }
  void sha256su1(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b110, rd, rn, rm);
  }

  // Cryptographic two-register SHA
  void sha1h(ARMEmitter::SRegister rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00000, rd.V(), rn.V());
  }
  void sha1su1(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00001, rd, rn);
  }
  void sha256su0(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00010, rd, rn);
  }
  // Advanced SIMD table lookup
  void tbl(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b00, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(ARMEmitter::DRegister rd, ARMEmitter::QRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b00, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b00, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(ARMEmitter::DRegister rd, ARMEmitter::QRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b00, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(QRegister rd, QRegister rn, QRegister rn2, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2), "rn and rn2 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b01, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(DRegister rd, QRegister rn, QRegister rn2, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2), "rn and rn2 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b01, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(QRegister rd, QRegister rn, QRegister rn2, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2), "rn and rn2 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b01, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(DRegister rd, QRegister rn, QRegister rn2, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2), "rn and rn2 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b01, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(QRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3), "rn, rn2, and rn3 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b10, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(DRegister rd, QRegister rn, QRegister rn2, QRegister rn3, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3), "rn, rn2, and rn3 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b10, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(QRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3), "rn, rn2, and rn3 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b10, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(DRegister rd, QRegister rn, QRegister rn2, QRegister rn3, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3), "rn, rn2, and rn3 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b10, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(QRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rn4, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3, rn4), "rn, rn2, rn3, and rn4 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b11, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(DRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rn4, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3, rn4), "rn, rn2, rn3, and rn4 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b11, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(QRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rn4, QRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3, rn4), "rn, rn2, rn3, and rn4 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b11, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(DRegister rd, QRegister rn, QRegister rn2, QRegister rn3, QRegister rn4, DRegister rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rn, rn2, rn3, rn4), "rn, rn2, rn3, and rn4 must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b11, 0b1, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD permute
  template<ARMEmitter::SubRegSize size>
  void uzp1(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void uzp1(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  void trn1(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void trn1(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  void zip1(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void zip1(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  void uzp2(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void uzp2(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  void trn2(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void trn2(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  void zip2(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b111, rd.V(), rn.V(), rm.V());
  }
  template<ARMEmitter::SubRegSize size>
  requires (size != ARMEmitter::SubRegSize::i64Bit)
  void zip2(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b111, rd.V(), rn.V(), rm.V());
  }


  void uzp1(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  void uzp1(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  void trn1(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  void trn1(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  void zip1(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  void zip1(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  void uzp2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  void uzp2(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  void trn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  void trn2(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  void zip2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b111, rd.V(), rn.V(), rm.V());
  }
  void zip2(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b111, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD extract
  void ext(ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, ARMEmitter::QRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 16, "Index can't be more than 15");
    constexpr uint32_t Op = 0b0010'1110'000 << 21;
    ASIMDExtract(Op, 1, 0b00, Index, rd.V(), rn.V(), rm.V());
  }
  void ext(ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, ARMEmitter::DRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index can't be more than 7");
    constexpr uint32_t Op = 0b0010'1110'000 << 21;
    ASIMDExtract(Op, 0, 0b00, Index, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD copy
  template <typename T>
  requires(std::is_same_v<QRegister, T> || std::is_same_v<DRegister, T>)
  void dup(SubRegSize size, T rd, T rn, uint32_t Index) {
    if constexpr(std::is_same_v<DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }

    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    const uint32_t SizeImm = FEXCore::ToUnderlying(size);
    const uint32_t IndexShift = SizeImm + 1;
    const uint32_t ElementSize = 1U << SizeImm;
    const uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, Q, imm5, 0b0000, rd.V(), rn.V());
  }

  // Advanced SIMD three same (FP16)
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDThreeSameFP16(uint32_t U, uint32_t a, uint32_t opcode, T rm, T rn, T rd) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;
    constexpr uint32_t Op = 0b0000'1110'0100'0000'0000'01 << 10;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Advanced SIMD two-register miscellaneous (FP16)
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDTwoRegMiscFP16(uint32_t U, uint32_t a, uint32_t opcode, T rn, T rd) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;
    constexpr uint32_t Op = 0b0000'1110'0111'1000'0000'10 << 10;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= a << 23;
    Instr |= opcode << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Advanced SIMD three-register extension
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDThreeRegisterExt(uint32_t U, uint32_t opcode, ARMEmitter::SubRegSize size, T rm, T rn, T rd) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;
    constexpr uint32_t Op = 0b0000'1110'0000'0000'1000'01 << 10;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  template <typename T>
  requires(std::is_same_v<QRegister, T> || std::is_same_v<DRegister, T>)
  void dup(SubRegSize size, T rd, Register rn) {
    if constexpr(std::is_same_v<DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }

    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    // Upper bits of imm5 are ignored for GPR dup
    const uint32_t imm5 = 1U << FEXCore::ToUnderlying(size);

    ASIMDScalarCopy(Op, Q, imm5, 0b0001, rd, ToVReg(rn));
  }

  template <SubRegSize size>
  requires(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit)
  void smov(XRegister rd, VRegister rn, uint32_t Index) {
    static_assert(size == SubRegSize::i8Bit ||
                  size == SubRegSize::i16Bit ||
                  size == SubRegSize::i32Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    constexpr uint32_t SizeImm = FEXCore::ToUnderlying(size);
    constexpr uint32_t IndexShift = SizeImm + 1;
    constexpr uint32_t ElementSize = 1U << SizeImm;
    constexpr uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, 1, imm5, 0b0101, ToVReg(rd), rn);
  }
  template <SubRegSize size>
  requires(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit)
  void smov(WRegister rd, VRegister rn, uint32_t Index) {
    static_assert(size == SubRegSize::i8Bit ||
                  size == SubRegSize::i16Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    constexpr uint32_t SizeImm = FEXCore::ToUnderlying(size);
    constexpr uint32_t IndexShift = SizeImm + 1;
    constexpr uint32_t ElementSize = 1U << SizeImm;
    constexpr uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, 0, imm5, 0b0101, ToVReg(rd), rn);
  }

  template <SubRegSize size>
  void umov(Register rd, VRegister rn, uint32_t Index) {
    static_assert(size == SubRegSize::i8Bit ||
                  size == SubRegSize::i16Bit ||
                  size == SubRegSize::i32Bit ||
                  size == SubRegSize::i64Bit, "Unsupported umov size");


    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    constexpr uint32_t Q = size == SubRegSize::i64Bit ? 1 : 0;

    constexpr uint32_t SizeImm = FEXCore::ToUnderlying(size);
    constexpr uint32_t IndexShift = SizeImm + 1;
    constexpr uint32_t ElementSize = 1U << SizeImm;
    constexpr uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, Q, imm5, 0b0111, ToVReg(rd), rn);
  }

  template <SubRegSize size>
  void ins(VRegister rd, uint32_t Index, Register rn) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    constexpr uint32_t SizeImm = FEXCore::ToUnderlying(size);
    constexpr uint32_t IndexShift = SizeImm + 1;
    constexpr uint32_t ElementSize = 1U << SizeImm;
    constexpr uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, 1, imm5, 0b0011, rd, ToVReg(rn));
  }

  void ins(SubRegSize size, VRegister rd, uint32_t Index, Register rn) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    const uint32_t SizeImm = FEXCore::ToUnderlying(size);
    const uint32_t IndexShift = SizeImm + 1;
    const uint32_t ElementSize = 1U << SizeImm;
    const uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex, "Index too large. Index={}, Max Index: {}", Index, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;

    ASIMDScalarCopy(Op, 1, imm5, 0b0011, rd, ToVReg(rn));
  }

  void ins(SubRegSize size, VRegister rd, uint32_t Index, VRegister rn, uint32_t Index2) {
    constexpr uint32_t Op = 0b0110'1110'0000'0000'0000'01 << 10;
    
    const uint32_t SizeImm = FEXCore::ToUnderlying(size);
    const uint32_t IndexShift = SizeImm + 1;
    const uint32_t ElementSize = 1U << SizeImm;
    const uint32_t MaxIndex = 128U / (ElementSize * 8);

    LOGMAN_THROW_A_FMT(Index < MaxIndex,  "Index too large. Index={}, Max Index: {}", Index, MaxIndex);
    LOGMAN_THROW_A_FMT(Index2 < MaxIndex, "Index2 too large. Index2={}, Max Index: {}", Index2, MaxIndex);

    const uint32_t imm5 = (Index << IndexShift) | ElementSize;
    const uint32_t imm4 = Index2 << SizeImm;

    ASIMDScalarCopy(Op, 1, imm5, imm4, rd, rn);
  }

  // Advanced SIMD three same (FP16)
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmaxnm(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b000, rm, rn, rd);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmla(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b001, rm, rn, rd);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fadd(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b010, rm, rn, rd);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmulx(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b011, rm, rn, rd);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmeq(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b100, rm, rn, rd);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmax(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b110, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frecps(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 0, 0b111, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fminnm(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 1, 0b000, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmls(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 1, 0b001, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fsub(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 1, 0b010, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmin(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 1, 0b110, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frsqrts(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(0, 1, 0b111, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmaxnmp(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b000, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void faddp(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b010, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmul(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b011, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmge(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b100, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void facge(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b101, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fmaxp(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b110, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fdiv(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 0, 0b111, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fminnmp(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 1, 0b000, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fabd(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 1, 0b010, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmgt(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 1, 0b100, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void facgt(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 1, 0b101, rm, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fminp(T rd, T rn, T rm) {
    ASIMDThreeSameFP16(1, 1, 0b110, rm, rn, rd);
  }

  // Advanced SIMD two-register miscellaneous (FP16)
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frintn(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11000, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frintm(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11001, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtns(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11010, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtms(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11011, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtas(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11100, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void scvtf(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 0, 0b11101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmgt(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b01100, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmeq(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b01101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmlt(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b01110, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fabs(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b01111, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frintp(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b11000, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frintz(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b11001, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtps(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b11010, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtzs(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b11011, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frecpe(T rd, T rn) {
    ASIMDTwoRegMiscFP16(0, 1, 0b11101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frinta(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11000, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frintx(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11001, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtnu(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11010, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtmu(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11011, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtau(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11100, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void ucvtf(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 0, 0b11101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmge(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b01100, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcmle(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b01101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fneg(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b01111, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frinti(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b11001, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtpu(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b11010, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fcvtzu(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b11011, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void frsqrte(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b11101, rn, rd);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  requires(size == ARMEmitter::SubRegSize::i16Bit &&
           (std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>))
  void fsqrt(T rd, T rn) {
    ASIMDTwoRegMiscFP16(1, 1, 0b11111, rn, rd);
  }

  // Advanced SIMD three-register extension
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sdot(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    ASIMDThreeRegisterExt(0, 0b0010, size, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void usdot(T rd, T rn, T rm) {
    ASIMDThreeRegisterExt(0, 0b0011, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqrdmlah(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    ASIMDThreeRegisterExt(1, 0b0000, size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqrdmlsh(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    ASIMDThreeRegisterExt(1, 0b0001, size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void udot(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    ASIMDThreeRegisterExt(1, 0b0010, size, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmla(ARMEmitter::SubRegSize size, T rd, T rn, T rm, ARMEmitter::Rotation Rot) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "8-bit subregsize not supported");

    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    ASIMDThreeRegisterExt(1, 0b1000 | FEXCore::ToUnderlying(Rot), size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm, ARMEmitter::Rotation Rot) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "8-bit subregsize not supported");

    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(Rot == ARMEmitter::Rotation::ROTATE_90 || Rot == ARMEmitter::Rotation::ROTATE_270, "Invalid rotation");
    const uint32_t ConvertedRotation =
      Rot == ARMEmitter::Rotation::ROTATE_90 ? 0b00 : 0b10;
    ASIMDThreeRegisterExt(1, 0b1100 | ConvertedRotation, size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void bfdot(T rd, T rn, T rm) {
    ASIMDThreeRegisterExt(1, 0b1111, ARMEmitter::SubRegSize::i16Bit, rm, rn, rd);
  }
  void bfmlalb(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(1, 0b1111, ARMEmitter::SubRegSize::i64Bit, rm.D(), rn.D(), rd.D());
  }
  void bfmlalt(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(1, 0b1111, ARMEmitter::SubRegSize::i64Bit, rm.Q(), rn.Q(), rd.Q());
  }
  void smmla(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(0, 0b0100, ARMEmitter::SubRegSize::i32Bit, rm.Q(), rn.Q(), rd.Q());
  }
  void usmmla(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(0, 0b0101, ARMEmitter::SubRegSize::i32Bit, rm.Q(), rn.Q(), rd.Q());
  }
  void bfmmla(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(1, 0b1101, ARMEmitter::SubRegSize::i16Bit, rm.Q(), rn.Q(), rd.Q());
  }
  void ummla(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    ASIMDThreeRegisterExt(1, 0b0100, ARMEmitter::SubRegSize::i32Bit, rm.Q(), rn.Q(), rd.Q());
  }

  // Advanced SIMD two-register miscellaneous
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void rev64(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00000, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void rev16(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00001, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void saddlp(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b00010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void suqadd(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cls(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cnt(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00101, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sadalp(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b00110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqabs(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00111, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cmgt(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01000, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cmeq(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01001, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cmlt(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void abs(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01011, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void xtn(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10010, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void xtn2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10010, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtn(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10100, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtn2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10100, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtn(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit ||
                       size == ARMEmitter::SubRegSize::i16Bit, "Only 16-bit & 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10110, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtn2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit ||
                       size == ARMEmitter::SubRegSize::i16Bit, "Only 16-bit & 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10110, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtl(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10111, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtl2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10111, rd.Q(), rn.Q());
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frintn(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11000, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frintm(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtns(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtms(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtas(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void scvtf(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frint32z(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11110, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frint64z(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmgt(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01100, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmeq(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmlt(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fabs(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frintp(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11000, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frintz(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtps(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtzs(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void urecpe(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11100, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frecpe(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void rev32(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit ||
                       size == ARMEmitter::SubRegSize::i16Bit, "Only 8-bit & 16-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00000, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void uaddlp(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b00010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void usqadd(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void clz(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00100, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void uadalp(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b00110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqneg(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00111, rd, rn);
  }

  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cmge(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01000, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void cmle(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01001, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void neg(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtun(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10010, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtun2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10010, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void shll(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void shll2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void uqxtn(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10100, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void uqxtn2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10100, rd.Q(), rn.Q());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtxn(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10110, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtxn2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10110, rd.Q(), rn.Q());
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frinta(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11000, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frintx(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtnu(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtmu(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtau(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ucvtf(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frint32x(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11110, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frint64x(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void not_(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b00101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void mvn(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b00101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void rbit(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, ARMEmitter::SubRegSize::i16Bit, 0b00101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmge(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcmle(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fneg(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frinti(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtpu(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fcvtzu(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ursqrte(ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void frsqrte(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fsqrt(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                       size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11111, rd, rn);
  }

  // Advanced SIMD across lanes
  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void saddlv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMDAcrossLanes<T>(Op, 0, ConvertedSize, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void smaxv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit && size != ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sminv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit && size != ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void addv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit && size != ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void uaddlv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMDAcrossLanes<T>(Op, 1, ConvertedSize, 0b00011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void umaxv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit && size != ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 1, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void uminv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit && size != ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 1, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmaxnmv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit && size != ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmaxv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit && size != ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01111, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fminnmv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit && size != ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i64Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i32Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fminv(ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit && size != ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i64Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i32Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01111, rd, rn);
  }

  // Advanced SIMD three different
  // TODO: Double check narrowing op size limits.
  // TODO: Don't enforce DRegister/QRegister for Q check
  ///< Size is dest size
  void saddl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void saddl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void saddw(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void saddw2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void ssubl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void ssubl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void ssubw(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void ssubw2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0011, ConvertedSize, rd, rn, rm);
  }
  void addhn(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b0100, size, rd, rn, rm);
  }
  void addhn2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b0100, size, rd, rn, rm);
  }
  ///< Size is dest size
  void sabal(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sabal2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0101, ConvertedSize, rd, rn, rm);
  }
  void subhn(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b0110, size, rd, rn, rm);
  }
  void subhn2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b0110, size, rd, rn, rm);
  }
  ///< Size is dest size
  void sabdl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sabdl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smlal(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smlal2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmlal(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmlal2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smlsl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smlsl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmlsl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmlsl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smull(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void smull2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmull(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void sqdmull2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit && size != SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 0, 0b1101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void pmull(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i128Bit, "Only 16-bit and 128-bit destination supported");
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b1110, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void pmull2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i16Bit || size == SubRegSize::i128Bit, "Only 16-bit and 128-bit destination supported");
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different(Op, 0, 0b1110, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uaddl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uaddl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uaddw(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uaddw2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void usubl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void usubl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void usubw(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void usubw2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0011, ConvertedSize, rd, rn, rm);
  }
  // XXX: RADDHN/2
  ///< Size is dest size
  void uabal(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uabal2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0101, ConvertedSize, rd, rn, rm);
  }
  // XXX: RSUBHN/2
  ///< Size is dest size
  void uabdl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void uabdl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umlal(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umlal2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umlsl(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umlsl2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umull(SubRegSize size, DRegister rd, DRegister rn, DRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  void umull2(SubRegSize size, QRegister rd, QRegister rn, QRegister rm) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize = SubRegSize{FEXCore::ToUnderlying(size) - 1};

    ASIMD3Different(Op, 1, 0b1100, ConvertedSize, rd, rn, rm);
  }

  // Advanced SIMD three same
  template<typename T>
  void shadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00000, rd, rn, rm);
  }

  template<typename T>
  void sqadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit sqadd");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00001, rd, rn, rm);
  }

  template<typename T>
  void srhadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00010, rd, rn, rm);
  }
  template<typename T>
  void shsub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00100, rd, rn, rm);
  }
  template<typename T>
  void sqsub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit sqsub");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00101, rd, rn, rm);
  }
  template<typename T>
  void cmgt(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit cmgt");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00110, rd, rn, rm);
  }
  template<typename T>
  void cmge(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit cmge");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00111, rd, rn, rm);
  }
  template<typename T>
  void sshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit sshl");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01000, rd, rn, rm);
  }
  template<typename T>
  void sqshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit sqshl");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01001, rd, rn, rm);
  }
  template<typename T>
  void srshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit srshl");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01010, rd, rn, rm);
  }
  template<typename T>
  void sqrshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit sqrshl");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01011, rd, rn, rm);
  }
  template<typename T>
  void smax(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01100, rd, rn, rm);
  }
  template<typename T>
  void smin(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01101, rd, rn, rm);
  }
  template<typename T>
  void sabd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01110, rd, rn, rm);
  }
  template<typename T>
  void saba(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01111, rd, rn, rm);
  }
  template<typename T>
  void add(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit add");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10000, rd, rn, rm);
  }
  template<typename T>
  void cmtst(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit cmtst");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10001, rd, rn, rm);
  }
  template<typename T>
  void mla(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10010, rd, rn, rm);
  }
  template<typename T>
  void mul(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10011, rd, rn, rm);
  }
  template<typename T>
  void smaxp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10100, rd, rn, rm);
  }
  template<typename T>
  void sminp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10101, rd, rn, rm);
  }
  template<typename T>
  void sqdmulh(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10110, rd, rn, rm);
  }
  template<typename T>
  void addp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10111, rd, rn, rm);
  }
  template<typename T>
  void fmaxnm(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fmla(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void fadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmulx(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11011, rd, rn, rm);
  }
  template<typename T>
  void fcmeq(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void fmax(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void frecps(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11111, rd, rn, rm);
  }
  template<typename T>
  void and_(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i8Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void fmlal(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i8Bit, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fmlal2(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void bic(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i16Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void fminnm(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fmls(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void fsub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmin(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void frsqrts(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11111, rd, rn, rm);
  }
  template<typename T>
  void orr(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i32Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void mov(T rd, T rn) {
    orr<T>(rd, rn, rn);
  }
  template<typename T>
  void fmlsl(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i32Bit, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fmlsl2(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i32Bit, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void orn(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, ARMEmitter::SubRegSize::i64Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void uhadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00000, rd, rn, rm);
  }
  template<typename T>
  void uqadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00001, rd, rn, rm);
  }
  template<typename T>
  void urhadd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00010, rd, rn, rm);
  }
  template<typename T>
  void uhsub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00100, rd, rn, rm);
  }
  template<typename T>
  void uqsub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00101, rd, rn, rm);
  }
  template<typename T>
  void cmhi(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00110, rd, rn, rm);
  }
  template<typename T>
  void cmhs(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00111, rd, rn, rm);
  }
  template<typename T>
  void ushl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01000, rd, rn, rm);
  }
  template<typename T>
  void uqshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01001, rd, rn, rm);
  }
  template<typename T>
  void urshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01010, rd, rn, rm);
  }
  template<typename T>
  void uqrshl(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01011, rd, rn, rm);
  }
  template<typename T>
  void umax(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01100, rd, rn, rm);
  }
  template<typename T>
  void umin(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01101, rd, rn, rm);
  }
  template<typename T>
  void uabd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01110, rd, rn, rm);
  }
  template<typename T>
  void uaba(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01111, rd, rn, rm);
  }
  template<typename T>
  void sub(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10000, rd, rn, rm);
  }
  template<typename T>
  void cmeq(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10001, rd, rn, rm);
  }
  template<typename T>
  void mls(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10010, rd, rn, rm);
  }
  template<typename T>
  void pmul(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b10011, rd, rn, rm);
  }
  template<typename T>
  void umaxp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10100, rd, rn, rm);
  }
  template<typename T>
  void uminp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10101, rd, rn, rm);
  }
  template<typename T>
  void sqrdmulh(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit && size != ARMEmitter::SubRegSize::i8Bit, "8/64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10110, rd, rn, rm);
  }
  template<typename T>
  void fmaxnmp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void faddp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmul(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11011, rd, rn, rm);
  }
  template<typename T>
  void fcmge(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void facge(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fmaxp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void fdiv(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11111, rd, rn, rm);
  }
  template<typename T>
  void eor(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void bsl(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i16Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void fminnmp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fabd(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fcmgt(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void facgt(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fminp(ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i64Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void bit(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i32Bit, 0b00011, rd, rn, rm);
  }
  template<typename T>
  void bif(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i64Bit, 0b00011, rd, rn, rm);
  }

  // Advanced SIMD modified immediate
  // XXX: ORR - 32-bit/16-bit
  // XXX: MOVI - Shifting ones
  template<typename T>
  void fmov(ARMEmitter::SubRegSize size, T rd, float Value) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit ||
                        size == ARMEmitter::SubRegSize::i64Bit, "Unsupported fmov size");

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    uint32_t op;
    uint32_t cmode = 0b1111;
    uint32_t o2;
    uint32_t Imm;
    if (size == SubRegSize::i16Bit) {
      LOGMAN_MSG_A_FMT("Unsupported");
      FEX_UNREACHABLE;
    }
    else if (size == SubRegSize::i32Bit) {
      op = 0;
      o2 = 0;
      Imm = FP32ToImm8(Value);
    }
    else if (size == SubRegSize::i64Bit) {
      op = 1;
      o2 = 0;
      Imm = FP64ToImm8(Value);
    }
    else {
      LOGMAN_MSG_A_FMT("Invalid subregsize");
      FEX_UNREACHABLE;
    }

    ASIMDModifiedImm(Op, op, cmode, o2, Imm, rd);
  }
  // XXX: MVNI - Shifted immediate
  // XXX: BIC
  //void ASIMDModifiedImm(uint32_t Op, uint32_t op, uint32_t cmode, uint32_t o2, uint32_t imm, T rd) {

  template<typename T>
  void movi(ARMEmitter::SubRegSize size, T rd, uint64_t Imm, uint16_t Shift = 0) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i8Bit ||
                        size == ARMEmitter::SubRegSize::i16Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit ||
                        size == ARMEmitter::SubRegSize::i64Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    uint32_t cmode;
    uint32_t op;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Shift == 0, "8-bit can't have shift");
      LOGMAN_THROW_A_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b1110;
      op = 0;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 8, "Shift by invalid amount");
      LOGMAN_THROW_A_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b1000 | (Shift ? 0b10 : 0b00);
      op = 0;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 8 || Shift == 16 || Shift == 24, "Shift by invalid amount");
      LOGMAN_THROW_A_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b0000 | ((Shift >> 3) << 1);
      op = 0;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Shift == 0, "64-bit can't have shift");
      cmode = 0b1110;
      op = 1;

      // 64-bit movi doesn't behave like the smaller types
      // Each bit of the 8-bit imm encoding is expanded to a full 8-bits.
      // This gives us a full 64-bits for the final result but needs special handling.
      uint8_t NewImm{};
      for (size_t i = 0; i < 8; ++i) {
        const size_t BitOffset = i * 8;
        uint8_t Section = (Imm >> BitOffset) & 0xFF;
        LOGMAN_THROW_A_FMT(Section == 0 || Section == 0xFF, "Invalid 64-bit constant encoding");
        if (Section == 0xFF) {
          NewImm |= (1 << i);
        }
      }
      Imm = NewImm;
    }
    else {
      LOGMAN_MSG_A_FMT("Invalid subregsize");
      FEX_UNREACHABLE;
    }

    ASIMDModifiedImm(Op, op, cmode, 0, Imm, rd);
  }

  // Advanced SIMD shift by immediate
  template<typename T>
  void sshr(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b00000, rn, rd);
  }
  template<typename T>
  void ssra(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b00010, rn, rd);
  }
  template<typename T>
  void srshr(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b00100, rn, rd);
  }
  template<typename T>
  void srsra(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b00110, rn, rd);
  }
  template<typename T>
  void shl(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b01010, rn, rd);
  }
  template<typename T>
  void sqshl(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 0, immh, immb, 0b01110, rn, rd);
  }
  ///< size is destination size
  void shrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10000, rn, rd);
  }
  ///< size is destination size
  void shrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10000, rn, rd);
  }
  ///< size is destination size
  void rshrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10001, rn, rd);
  }
  ///< size is destination size
  void rshrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10001, rn, rd);
  }
  ///< size is destination size
  void sqshrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10010, rn, rd);
  }
  ///< size is destination size
  void sqshrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10010, rn, rd);
  }
  ///< size is destination size
  void sqrshrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10011, rn, rd);
  }
  ///< size is destination size
  void sqrshrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - (Shift);
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10011, rn, rd);
  }
  ///< size is destination size
  void sshll(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    size = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(Shift < SubregSizeInBits, "Shift must not be larger than incoming element size");

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10100, rn, rd);
  }

  ///< size is destination size
  void sshll2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    size = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(Shift < SubregSizeInBits, "Shift must not be larger than incoming element size");

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10100, rn, rd);
  }
  ///< size is destination size
  void sxtl(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    sshll(size, rd.D(), rn.D(), 0);
  }
  ///< size is destination size
  void sxtl2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    sshll2(size, rd.Q(), rn.Q(), 0);
  }

  template<typename T>
  void scvtf(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b11100, rn, rd);
  }

  template<typename T>
  void fcvtzs(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b11111, rn, rd);
  }

  template<typename T>
  void ushr(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b00000, rn, rd);
  }
  template<typename T>
  void usra(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b00010, rn, rd);
  }
  template<typename T>
  void urshr(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b00100, rn, rd);
  }
  template<typename T>
  void ursra(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b00110, rn, rd);
  }
  template<typename T>
  void sri(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b01000, rn, rd);
  }
  template<typename T>
  void sli(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b01010, rn, rd);
  }
  template<typename T>
  void sqshlu(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b01100, rn, rd);
  }
  ///< size is destination size
  template<typename T>
  void uqshl(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm<T>(Op, 1, immh, immb, 0b01110, rn, rd);
  }
  ///< size is destination size
  void sqshrun(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10000, rn, rd);
  }
  ///< size is destination size
  void sqshrun2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10000, rn, rd);
  }
  ///< size is destination size
  void sqrshrun(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10001, rn, rd);
  }
  ///< size is destination size
  void sqrshrun2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10001, rn, rd);
  }
  ///< size is destination size
  void uqshrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10010, rn, rd);
  }
  ///< size is destination size
  void uqshrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10010, rn, rd);
  }
  ///< size is destination size
  void uqrshrn(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10011, rn, rd);
  }
  ///< size is destination size
  void uqrshrn2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedShift = (SubregSizeInBits * 2) - Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10011, rn, rd);
  }
  ///< size is destination size
  void ushll(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    size = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10100, rn, rd);
  }
  ///< size is destination size
  void ushll2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    size = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;


    ASIMDShiftByImm(Op, 1, immh, immb, 0b10100, rn, rd);
  }
  void uxtl(ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn) {
    ushll(size, rd, rn, 0);
  }
  void uxtl2(ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn) {
    ushll2(size, rd, rn, 0);
  }
  template<typename T>
  void ucvtf(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b11100, rn, rd);
  }

  template<typename T>
  void fcvtzu(ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_A_FMT(size != ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_A_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b11111, rn, rd);
  }

  // Advanced SIMD vector x indexed element
  ///< size is destination size
  void smlal(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0010, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void smlal2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0010, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  ///< size is destination size
  void sqdmlal(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0011, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void sqdmlal2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0011, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  ///< size is destination size
  void smlsl(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0110, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void smlsl2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0110, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  ///< size is destination size
  void sqdmlsl(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0111, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void sqdmlsl2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0111, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void mul(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1000, H, size, rm, rn, rd);
  }
  ///< size is destination size
  void smull(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1010, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void smull2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1010, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  ///< size is destination size
  void sqdmull(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1011, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void sqdmull2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1011, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqdmulh(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1100, H, size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqrdmulh(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1101, H, size, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1110, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sudot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, ARMEmitter::SubRegSize::i8Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void bfdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, ARMEmitter::SubRegSize::i16Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmla(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit ||
                        size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0001, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmls(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit ||
                        size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0101, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmul(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit ||
                        size == ARMEmitter::SubRegSize::i32Bit ||
                        size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1001, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmlal(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0000, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmlal2(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1000, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmlsl(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0100, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void fmlsl2(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1100, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void usdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  void bfmlalb(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, ARMEmitter::SubRegSize::i64Bit, rm.D(), rn.D(), rd.D());
  }
  void bfmlalt(ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, ARMEmitter::SubRegSize::i64Bit, rm.Q(), rn.Q(), rd.Q());
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void mla(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0000, H, size, rm, rn, rd);
  }

  ///< size is destination size
  void umlal(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0010, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void umlal2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0010, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void mls(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0100, H, size, rm, rn, rd);
  }

  ///< size is destination size
  void umlsl(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0110, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void umlsl2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b0110, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }

  ///< size is destination size
  void umull(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1010, H, EncodedSubRegSize, rm.D(), rn.D(), rd.D());
  }
  ///< size is destination size
  void umull2(ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i32Bit || size == ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1010, H, EncodedSubRegSize, rm.Q(), rn.Q(), rd.Q());
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqrdmlah(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1101, H, size, rm, rn, rd);
  }
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void udot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1110, H, ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void sqrdmlsh(ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(size == ARMEmitter::SubRegSize::i16Bit || size == ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
    }
    else {
      // Index encoded in H:L
      // M overlaps rm register.
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1111, H, size, rm, rn, rd);
  }

  // Cryptographic three-register, imm2
  // TODO
  // Cryptographic three-register SHA 512
  // TODO
  // Cryptographic four-register
  // TODO
  // Cryptographic two-register SHA 512
  // TODO
  // Conversion between floating-point and fixed-point
  void scvtf(ARMEmitter::ScalarRegSize ScalarSize, ARMEmitter::VRegister rd, ARMEmitter::Size GPRSize, ARMEmitter::Register rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(FractionalBits >= 1 && FractionalBits <= ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b00, 0b010, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void ucvtf(ARMEmitter::ScalarRegSize ScalarSize, ARMEmitter::VRegister rd, ARMEmitter::Size GPRSize, ARMEmitter::Register rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(FractionalBits >= 1 && FractionalBits <= ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b00, 0b011, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void fcvtzs(ARMEmitter::Size GPRSize, ARMEmitter::Register rd, ARMEmitter::ScalarRegSize ScalarSize, ARMEmitter::VRegister rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(FractionalBits >= 1 && FractionalBits <= ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b11, 0b000, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void fcvtzu(ARMEmitter::Size GPRSize, ARMEmitter::Register rd, ARMEmitter::ScalarRegSize ScalarSize, ARMEmitter::VRegister rn, uint32_t FractionalBits) {
    LOGMAN_THROW_A_FMT(FractionalBits >= 1 && FractionalBits <= ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b11, 0b001, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  // Conversion between floating-point and integer
  void fcvtns(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtns(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtns(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtnu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtnu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtnu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void scvtf(ARMEmitter::Size size, ARMEmitter::HRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b010, ARMEmitter::ToReg(rd), rn);
  }
  void scvtf(ARMEmitter::Size size, ARMEmitter::SRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b010, ARMEmitter::ToReg(rd), rn);
  }
  void scvtf(ARMEmitter::Size size, ARMEmitter::DRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b010, ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(ARMEmitter::Size size, ARMEmitter::HRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b011, ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(ARMEmitter::Size size, ARMEmitter::SRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b011, ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(ARMEmitter::Size size, ARMEmitter::DRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b011, ARMEmitter::ToReg(rd), rn);
  }
  void fcvtas(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b100, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtas(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b100, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtas(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b100, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtau(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b101, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtau(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b101, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtau(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b101, rd, ARMEmitter::ToReg(rn));
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b110, rd, ARMEmitter::ToReg(rn));
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::Size::i64Bit, "Can't move SReg to 64-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b110, rd, ARMEmitter::ToReg(rn));
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::Size::i32Bit, "Can't move DReg to 32-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b110, rd, ARMEmitter::ToReg(rn));
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::VRegister rn, bool Upper) {
    if (Upper) {
      LOGMAN_THROW_A_FMT(size == ARMEmitter::Size::i64Bit, "Can only move upper with 64-bit elements");
    }
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, Upper ? 0b10 : 0b01, Upper ? 0b01 : 0b00, 0b110, rd, ARMEmitter::ToReg(rn));
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::HRegister rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b111, ARMEmitter::ToReg(rd), rn);
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::SRegister rd, ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::Size::i64Bit, "Can't move SReg to 64-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b111, ARMEmitter::ToReg(rd), rn);
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::DRegister rd, ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT(size != ARMEmitter::Size::i32Bit, "Can't move DReg to 32-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b111, ARMEmitter::ToReg(rd), rn);
  }
  void fmov(ARMEmitter::Size size, ARMEmitter::VRegister rd, ARMEmitter::Register rn, bool Upper) {
    if (Upper) {
      LOGMAN_THROW_A_FMT(size == ARMEmitter::Size::i64Bit, "Can only move upper with 64-bit elements");
    }
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, Upper ? 0b10 : 0b01, Upper ? 0b01 : 0b00, 0b111, ARMEmitter::ToReg(rd), rn);
  }
  void fcvtps(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b01, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtps(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b01, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtps(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b01, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtpu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b01, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtpu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b01, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtpu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b01, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtms(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b10, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtms(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b10, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtms(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b10, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtmu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b10, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtmu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b10, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtmu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b10, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzs(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b11, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzs(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b11, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzs(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzs(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b000, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b11, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b11, 0b001, rd, ARMEmitter::ToReg(rn));
  }
  void fcvtzu(ARMEmitter::Size size, ARMEmitter::Register rd, ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b001, rd, ARMEmitter::ToReg(rn));
  }

private:
  // Cryptographic AES
  void CryptoAES(uint32_t Op, uint32_t opcode, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Cryptographic three-register SHA
  void Crypto3RegSHA(uint32_t Op, uint32_t opcode, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Cryptographic two-register SHA
  void Crypto2RegSHA(uint32_t Op, uint32_t opcode, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD table lookup
  void ASIMDTable(uint32_t Op, uint32_t Q, uint32_t op2, uint32_t len, uint32_t op, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= Q << 30;
    Instr |= op2 << 22;
    Instr |= Encode_rm(rm);
    Instr |= len << 13;
    Instr |= op << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD permute
  void ASIMDPermute(uint32_t Op, uint32_t Q, ARMEmitter::SubRegSize size, uint32_t opcode, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= Q << 30;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD extract
  void ASIMDExtract(uint32_t Op, uint32_t Q, uint32_t op2, uint32_t imm4, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn, ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= Q << 30;
    Instr |= op2 << 22;
    Instr |= Encode_rm(rm);
    Instr |= imm4 << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD two-register miscellaneous
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMD2RegMisc(uint32_t Op, uint32_t U, ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD across lanes
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDAcrossLanes(uint32_t Op, uint32_t U, ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD three different
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMD3Different(uint32_t Op, uint32_t U, uint32_t opcode, ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD three same
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMD3Same(uint32_t Op, uint32_t U, ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn, T rm) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD modified immediate
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDModifiedImm(uint32_t Op, uint32_t op, uint32_t cmode, uint32_t o2, uint32_t imm, T rd) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;
    Instr |= Q;
    Instr |= op << 29;
    Instr |= ((imm >> 7) & 1) << 18;
    Instr |= ((imm >> 6) & 1) << 17;
    Instr |= ((imm >> 5) & 1) << 16;
    Instr |= cmode << 12;
    Instr |= o2 << 11;
    Instr |= ((imm >> 4) & 1) << 9;
    Instr |= ((imm >> 3) & 1) << 8;
    Instr |= ((imm >> 2) & 1) << 7;
    Instr |= ((imm >> 1) & 1) << 6;
    Instr |= ((imm >> 0) & 1) << 5;

    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD shift by immediate
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDShiftByImm(uint32_t Op, uint32_t U, uint32_t immh, uint32_t immb, uint32_t opcode, T rn, T rd) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;
    LOGMAN_THROW_A_FMT(immh != 0, "ImmH needs to not be zero");

    uint32_t Instr = Op;

    Instr |= Q;
    Instr |= U << 29;
    Instr |= immh << 19;
    Instr |= immb << 16;
    Instr |= opcode << 11;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD vector x indexed element
  template<typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDVectorXIndexedElement(uint32_t U, uint32_t L, uint32_t M, uint32_t opcode, uint32_t H, ARMEmitter::SubRegSize size, T rm, T rn, T rd) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'00 << 10;
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;

    Instr |= Q;
    Instr |= U << 29;
    Instr |= FEXCore::ToUnderlying(size) << 22;
    Instr |= L << 21;

    // M and Rm might overlap. It's up to the instruction emitter itself to ensure there is no conflict.
    Instr |= M << 20;
    Instr |= rm.Idx() << 16;
    Instr |= opcode << 12;
    Instr |= H << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Conversion between floating-point and fixed-point
  template<typename T, typename T2>
  void ScalarConvertBetweenFPAndFixed(uint32_t S, uint32_t rmode, uint32_t opcode, uint32_t scale,
                                      ARMEmitter::Size GPRSize, uint32_t ScalarSize,
                                      T rn, T2 rd) {
    constexpr uint32_t Op = 0b0001'1110'000 << 21;
    const uint32_t SF = GPRSize == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;
    Instr |= SF;
    Instr |= S << 29;
    Instr |= ScalarSize << 22;
    Instr |= rmode << 19;
    Instr |= opcode << 16;
    Instr |= scale << 10;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();
    dc32(Instr);
  }

  // Conversion between floating-point and integer
  void ASIMDFloatConvBetweenInt(uint32_t Op, ARMEmitter::Size s, uint32_t S, uint32_t ptype, uint32_t rmode, uint32_t opcode, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= S << 29;
    Instr |= ptype << 22;
    Instr |= rmode << 19;
    Instr |= opcode << 16;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  template<ARMEmitter::SubRegSize size, bool Load, typename T>
  requires(std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>)
  void ASIMDLoadStoreMultipleStructure(uint32_t Op, uint32_t opcode, T rt, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;

    uint32_t Instr = Op;

    Instr |= Q;
    Instr |= Load ? 1 << 22: 0;
    Instr |= Encode_rm(rm);
    Instr |= opcode;
    Instr |= FEXCore::ToUnderlying(size) << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }
  template<ARMEmitter::SubRegSize size, bool Load, uint32_t Count>
  void ASIMDSTLD(uint32_t Op, uint32_t Opcode, ARMEmitter::VRegister rt, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && Index < 16) ||
      (size == SubRegSize::i16Bit && Index < 8) ||
      (size == SubRegSize::i32Bit && Index < 4) ||
      (size == SubRegSize::i64Bit && Index < 2),
      "Invalid Index selected");

    uint32_t Q{};
    uint32_t S{};
    uint32_t Size{};

    // selem is for determining if we are doing 1-3 loadstore single structure operations
    // eg: ST1/2/3/4 or LD1/2/3/4
    constexpr uint32_t selem = Count - 1;
    const uint32_t opcode = Opcode | (selem >> 1);

    // Index is encoded as:
    // 8-bit:  Q:S:size
    // 16-bit  Q:S:size<1>
    // 32-bit: Q:S
    // 64-bit: Q
    if constexpr (size == SubRegSize::i8Bit) {
      Q = ((Index & 0b1000) >> 3) << 30;
      S = ((Index & 0b0100) >> 2);
      Size = Index & 0b11;
    }
    else if constexpr (size == SubRegSize::i16Bit) {
      Q = ((Index & 0b0100) >> 2) << 30;
      S = ((Index & 0b0010) >> 1);
      Size = (Index & 0b1) << 1;
    }
    else if constexpr (size == SubRegSize::i32Bit) {
      Q = ((Index & 0b0010) >> 1) << 30;
      S = Index & 0b0001;
    }
    else if constexpr (size == SubRegSize::i64Bit) {
      Q = (Index & 0b0001) << 30;
      Size = 1;
    }

    // scale = opcode<2:1>
    // selem = opcode<0>:R + 1
    //
    // scale:
    // - 0
    //   - Index = Q:S:size - aka B[0-15]
    // - 1
    //   - Index = Q:S:size<1> - aka H[0-7]
    // - 2
    //   if (size == i32)
    //     - Index = Q:S - aka S[0-3]
    //   if (size == i64)
    //     - Index = Q - aka D[0-1]
    //   if (size == i128) undefined
    // - 3
    //   Load+Replicate
    //   scale = size

    ASIMDLoadStore(Op | Q, Load, selem & 1, opcode, S, Size, rt, rn, rm);
  }

  template<ARMEmitter::SubRegSize size, bool Load, uint32_t Count, typename T>
  void ASIMDSTLD(uint32_t Op, uint32_t Opcode, T rt, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1U << 30 : 0;
    constexpr uint32_t S = 0;

    // selem is for determining if we are doing 1-3 loadstore single structure operations
    // eg: ST1/2/3/4 or LD1/2/3/4
    constexpr uint32_t selem = Count - 1;
    const uint32_t opcode = Opcode | (selem >> 1);

    // scale = opcode<2:1>
    // selem = opcode<0>:R + 1
    //
    // scale:
    // - 0
    //   - Index = Q:S:size - aka B[0-15]
    // - 1
    //   - Index = Q:S:size<1> - aka H[0-7]
    // - 2
    //   if (size == i32)
    //     - Index = Q:S - aka S[0-3]
    //   if (size == i64)
    //     - Index = Q - aka D[0-1]
    //   if (size == i128) undefined
    // - 3
    //   Load+Replicate
    //   scale = size

    ASIMDLoadStore(Op | Q, Load, selem & 1, opcode, S, FEXCore::ToUnderlying(size), rt, rn, rm);
  }
  void ASIMDLoadStore(uint32_t Op, uint32_t L, uint32_t R, uint32_t opcode, uint32_t S, uint32_t size, ARMEmitter::VRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    uint32_t Instr = Op;

    Instr |= L << 22;
    Instr |= R << 21;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 13;
    Instr |= S << 12;
    Instr |= size << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);

    dc32(Instr);
  }


