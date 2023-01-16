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
  void aese(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00100, rd, rn);
  }
  void aesd(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00101, rd, rn);
  }
  void aesmc(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00110, rd, rn);
  }
  void aesimc(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0100'1110'0010'1000'0000'10 << 10;
    CryptoAES(Op, 0b00111, rd, rn);
  }

  // Cryptographic three-register SHA
  void sha1c(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b000, rd, rn.V(), rm);
  }
  void sha1p(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b001, rd, rn.V(), rm);
  }
  void sha1m(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::SRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b010, rd, rn.V(), rm);
  }
  void sha1su0(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b011, rd, rn, rm);
  }
  void sha256h(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b100, rd, rn, rm);
  }
  void sha256h2(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b100, rd, rn, rm);
  }
  void sha256su1(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    constexpr uint32_t Op = 0b0101'1110'0000'0000'0000'00 << 10;
    Crypto3RegSHA(Op, 0b100, rd, rn, rm);
  }

  // Cryptographic two-register SHA
  void sha1h(FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00000, rd.V(), rn.V());
  }
  void sha1su1(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00001, rd, rn);
  }
  void sha256su0(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0101'1110'0010'1000'0000'10 << 10;
    Crypto2RegSHA(Op, 0b00010, rd, rn);
  }
  // Advanced SIMD table lookup
  void tbl(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b00, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b00, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b00, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b00, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b01, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b01, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b01, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b01, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b10, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b10, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b10, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b10, 0b1, rd.V(), rn.V(), rm.V());
  }

  void tbl(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rn4, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn3.Idx() + 1) == rn4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b11, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbl(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rn4, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn3.Idx() + 1) == rn4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b11, 0b0, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rn4, FEXCore::ARMEmitter::QRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn3.Idx() + 1) == rn4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 1, 0b00, 0b11, 0b1, rd.V(), rn.V(), rm.V());
  }
  void tbx(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rn2, FEXCore::ARMEmitter::QRegister rn3, FEXCore::ARMEmitter::QRegister rn4, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_A_FMT((rn.Idx() + 1) == rn2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn2.Idx() + 1) == rn3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rn3.Idx() + 1) == rn4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1110'000 << 21;
    ASIMDTable(Op, 0, 0b00, 0b11, 0b1, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD permute
  template<FEXCore::ARMEmitter::SubRegSize size>
  void uzp1(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void uzp1(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void trn1(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void trn1(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void zip1(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void zip1(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void uzp2(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void uzp2(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void trn2(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void trn2(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void zip2(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b111, rd.V(), rn.V(), rm.V());
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires (size != FEXCore::ARMEmitter::SubRegSize::i64Bit)
  void zip2(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b111, rd.V(), rn.V(), rm.V());
  }


  void uzp1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  void uzp1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b001, rd.V(), rn.V(), rm.V());
  }
  void trn1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  void trn1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b010, rd.V(), rn.V(), rm.V());
  }
  void zip1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  void zip1(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b011, rd.V(), rn.V(), rm.V());
  }
  void uzp2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  void uzp2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b101, rd.V(), rn.V(), rm.V());
  }
  void trn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  void trn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b110, rd.V(), rn.V(), rm.V());
  }
  void zip2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 1, size, 0b111, rd.V(), rn.V(), rm.V());
  }
  void zip2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid 64-bit size on 64-bit permute");
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'10 << 10;
    ASIMDPermute(Op, 0, size, 0b111, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD extract
  void ext(FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, FEXCore::ARMEmitter::QRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(Index < 16, "Index can't be more than 15");
    constexpr uint32_t Op = 0b0010'1110'000 << 21;
    ASIMDExtract(Op, 1, 0b00, Index, rd.V(), rn.V(), rm.V());
  }
  void ext(FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, FEXCore::ARMEmitter::DRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(Index < 8, "Index can't be more than 7");
    constexpr uint32_t Op = 0b0010'1110'000 << 21;
    ASIMDExtract(Op, 0, 0b00, Index, rd.V(), rn.V(), rm.V());
  }

  // Advanced SIMD copy
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void dup(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Index) {
    if constexpr(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm5 = (Index << 4) | 0b1000;
    }

    ASIMDScalarCopy(Op, Q, imm5, 0b0000, rd.V(), rn.V());
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void dup(FEXCore::ARMEmitter::SubRegSize size, T rd, FEXCore::ARMEmitter::Register rn) {
    if constexpr(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;

    // Upper bits of imm5 are ignored for GPR dup
    uint32_t imm5 = 0b00000;
    if (size == SubRegSize::i8Bit) {
      imm5 = 1;
    }
    else if (size == SubRegSize::i16Bit) {
      imm5 = 0b10;
    }
    else if (size == SubRegSize::i32Bit) {
      imm5 = 0b100;
    }
    else if (size == SubRegSize::i64Bit) {
      imm5 = 0b1000;
    }

    ASIMDScalarCopy(Op, Q, imm5, 0b0001, rd, ToVReg(rn));
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  requires(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit)
  void smov(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Index) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i8Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if constexpr (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if constexpr (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if constexpr (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }

    ASIMDScalarCopy(Op, 1, imm5, 0b0101, ToVReg(rd), rn);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  requires(size == FEXCore::ARMEmitter::SubRegSize::i8Bit || size == FEXCore::ARMEmitter::SubRegSize::i16Bit)
  void smov(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Index) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i8Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if constexpr (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if constexpr (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }

    ASIMDScalarCopy(Op, 0, imm5, 0b0101, ToVReg(rd), rn);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void umov(FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::VRegister rn, uint32_t Index) {
    static_assert(size == FEXCore::ARMEmitter::SubRegSize::i8Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                  size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Unsupported smov size");


    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    uint32_t Q = 0;
    if constexpr (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if constexpr (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if constexpr (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }
    else if constexpr (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm5 = (Index << 4) | 0b1000;
      Q = 1;
    }

    ASIMDScalarCopy(Op, Q, imm5, 0b0111, ToVReg(rd), rn);
  }

  template<FEXCore::ARMEmitter::SubRegSize size>
  void ins(FEXCore::ARMEmitter::VRegister rd, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if constexpr (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if constexpr (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if constexpr (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }
    else if constexpr (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm5 = (Index << 4) | 0b1000;
    }

    ASIMDScalarCopy(Op, 1, imm5, 0b0011, rd, ToVReg(rn));
  }

  void ins(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm5 = (Index << 4) | 0b1000;
    }

    ASIMDScalarCopy(Op, 1, imm5, 0b0011, rd, ToVReg(rn));
  }

  void ins(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, uint32_t Index, FEXCore::ARMEmitter::VRegister rn, uint32_t Index2) {
    constexpr uint32_t Op = 0b0110'1110'0000'0000'0000'01 << 10;
    uint32_t imm5 = 0b00000;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm5 = (Index << 1) | 1;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm5 = (Index << 2) | 0b10;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm5 = (Index << 3) | 0b100;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm5 = (Index << 4) | 0b1000;
    }

    uint32_t imm4 = 0b0000;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      imm4 = Index2;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      imm4 = Index2 << 1;
      // bit 0 ignored
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      imm4 = Index2 << 2;
      // bits [1:0] ignored
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      imm4 = Index2 << 3;
      // bits [2:0] ignored
    }

    ASIMDScalarCopy(Op, 1, imm5, imm4, rd, rn);
  }


  // Advanced SIMD three same (FP16)
  // TODO
  // Advanced SIMD two-register miscellaneous (FP16)
  // TODO
  // Advanced SIMD three-register extension
  // TODO
  // Advanced SIMD two-register miscellaneous
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void rev64(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00000, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void rev16(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00001, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void saddlp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b00010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void suqadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cls(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cnt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00101, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sadalp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b00110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqabs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b00111, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cmgt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01000, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cmeq(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01001, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cmlt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void abs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01011, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void xtn(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10010, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void xtn2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10010, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtn(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10100, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtn2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 0, size, 0b10100, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtn(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Only 16-bit & 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10110, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtn2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Only 16-bit & 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10110, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtl(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10111, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtl2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 0, ConvertedSize, 0b10111, rd.Q(), rn.Q());
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frintn(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11000, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frintm(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtns(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtms(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtas(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void scvtf(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frint32z(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11110, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frint64z(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 0, ConvertedSize, 0b11111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcmgt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01100, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcmeq(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcmlt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fabs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b01111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frintp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11000, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frintz(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtps(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtzs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void urecpe(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11100, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frecpe(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 0, size, 0b11101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void rev32(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i16Bit, "Only 8-bit & 16-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00000, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uaddlp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b00010, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void usqadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void clz(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00100, rd, rn);
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uadalp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b00110, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqneg(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b00111, rd, rn);
  }

  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cmge(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01000, rd, rn);
  }
  // Comparison against zero
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void cmle(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01001, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void neg(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtun(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10010, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void sqxtun2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit destination subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10010, rd.Q(), rn.Q());
  }

  ///< size is the destination size.
  ///< source size is the next size up.
  void shll(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::DRegister rd, ARMEmitter::DRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void shll2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::QRegister rd, ARMEmitter::QRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10011, rd, rn);
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void uqxtn(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10100, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void uqxtn2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc(Op, 1, size, 0b10100, rd.Q(), rn.Q());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtxn(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10110, rd.D(), rn.D());
  }
  ///< size is the destination size.
  ///< source size is the next size up.
  void fcvtxn2(FEXCore::ARMEmitter::SubRegSize size, ARMEmitter::VRegister rd, ARMEmitter::VRegister rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc(Op, 1, ConvertedSize, 0b10110, rd.Q(), rn.Q());
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frinta(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11000, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frintx(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtnu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtmu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtau(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ucvtf(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frint32x(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11110, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frint64x(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMD2RegMisc<T>(Op, 1, ConvertedSize, 0b11111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void not_(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b00101, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void mvn(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b00101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void rbit(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit, "Only 8-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0b00101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcmge(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcmle(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fneg(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b01111, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frinti(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11001, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtpu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fcvtzu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ursqrte(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void frsqrte(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11101, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fsqrt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                       size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit & 64-bit subregsize supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'10 << 10;
    ASIMD2RegMisc<T>(Op, 1, size, 0b11111, rd, rn);
  }

  // Advanced SIMD across lanes
  ///< size is the destination size.
  ///< source size is the next size up.
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void saddlv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Destination 8-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMDAcrossLanes<T>(Op, 0, ConvertedSize, 0b00011, rd, rn);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void smaxv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sminv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void addv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 0, size, 0b11011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uaddlv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    }
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    ASIMDAcrossLanes<T>(Op, 1, ConvertedSize, 0b00011, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void umaxv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 1, size, 0b01010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uminv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "32/64-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    ASIMDAcrossLanes<T>(Op, 1, size, 0b11010, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmaxnmv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmaxv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i16Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i8Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01111, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fminnmv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
    constexpr uint32_t Op = 0b0000'1110'0011'0000'0000'10 << 10;
    const auto ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ? ARMEmitter::SubRegSize::i64Bit :
      size == ARMEmitter::SubRegSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
      size == ARMEmitter::SubRegSize::i16Bit ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i32Bit;

    const auto U = size == ARMEmitter::SubRegSize::i16Bit ? 0 : 1;

    ASIMDAcrossLanes<T>(Op, U, ConvertedSize, 0b01100, rd, rn);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fminv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i32Bit, "32-bit subregsize not supported");
    }
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Destination 8/64-bit subregsize unsupported");
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
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void saddl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void saddl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void saddw(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void saddw2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ssubl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void ssubl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ssubw(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void ssubw2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0011, ConvertedSize, rd, rn, rm);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void addhn(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b0100, size, rd, rn, rm);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void addhn2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b0100, size, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sabal(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void sabal2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0101, ConvertedSize, rd, rn, rm);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void subhn(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b0110, size, rd, rn, rm);
  }
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void subhn2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "No 64-bit dest support.");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b0110, size, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sabdl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void sabdl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void smlal(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void smlal2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqdmlal(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void sqdmlal2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void smlsl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void smlsl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqdmlsl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void sqdmlsl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void smull(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void smull2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqdmull(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void sqdmull2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit && size != FEXCore::ARMEmitter::SubRegSize::i16Bit, "No 8/16-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 0, 0b1101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void pmull(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i128Bit, "Only 16-bit and 128-bit destination supported");
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i64Bit;

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b1110, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void pmull2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i128Bit, "Only 16-bit and 128-bit destination supported");
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i64Bit;

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    ASIMD3Different<T>(Op, 0, 0b1110, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uaddl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void uaddl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uaddw(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void uaddw2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0001, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void usubl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void usubl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void usubw(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0011, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void usubw2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0011, ConvertedSize, rd, rn, rm);
  }
  // XXX: RADDHN/2
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uabal(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0101, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void uabal2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b0101, ConvertedSize, rd, rn, rm);
  }
  // XXX: RSUBHN/2
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void uabdl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different(Op, 1, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void uabdl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different(Op, 1, 0b0111, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void umlal(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void umlal2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1000, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void umlsl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void umlsl2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1010, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void umull(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1100, ConvertedSize, rd, rn, rm);
  }
  ///< Size is dest size
  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>)
  void umull2(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'00 << 10;
    const auto ConvertedSize =
      size == FEXCore::ARMEmitter::SubRegSize::i64Bit ? FEXCore::ARMEmitter::SubRegSize::i32Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i32Bit ? FEXCore::ARMEmitter::SubRegSize::i16Bit :
      size == FEXCore::ARMEmitter::SubRegSize::i16Bit ? FEXCore::ARMEmitter::SubRegSize::i8Bit : FEXCore::ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Different<T>(Op, 1, 0b1100, ConvertedSize, rd, rn, rm);
  }

  // Advanced SIMD three same
  template<typename T>
  void shadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00000, rd, rn, rm);
  }

  template<typename T>
  void sqadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00001, rd, rn, rm);
  }

  template<typename T>
  void srhadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00010, rd, rn, rm);
  }
  template<typename T>
  void shsub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00100, rd, rn, rm);
  }
  template<typename T>
  void sqsub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00101, rd, rn, rm);
  }
  template<typename T>
  void cmgt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00110, rd, rn, rm);
  }
  template<typename T>
  void cmge(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b00111, rd, rn, rm);
  }
  template<typename T>
  void sshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01000, rd, rn, rm);
  }
  template<typename T>
  void sqshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01001, rd, rn, rm);
  }
  template<typename T>
  void srshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01010, rd, rn, rm);
  }
  template<typename T>
  void sqrshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01011, rd, rn, rm);
  }
  template<typename T>
  void smax(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01100, rd, rn, rm);
  }
  template<typename T>
  void smin(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01101, rd, rn, rm);
  }
  template<typename T>
  void sabd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01110, rd, rn, rm);
  }
  template<typename T>
  void saba(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b01111, rd, rn, rm);
  }
  template<typename T>
  void add(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10000, rd, rn, rm);
  }
  template<typename T>
  void cmtst(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit dup");
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10001, rd, rn, rm);
  }
  template<typename T>
  void mla(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10010, rd, rn, rm);
  }
  template<typename T>
  void mul(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10011, rd, rn, rm);
  }
  template<typename T>
  void smaxp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10100, rd, rn, rm);
  }
  template<typename T>
  void sminp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10101, rd, rn, rm);
  }
  template<typename T>
  void sqdmulh(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "No 8-bit dest support.");
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10110, rd, rn, rm);
  }
  template<typename T>
  void addp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b10111, rd, rn, rm);
  }
  template<typename T>
  void fmaxnm(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fmla(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void fadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmulx(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11011, rd, rn, rm);
  }
  template<typename T>
  void fcmeq(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void fmax(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 0, ConvertedSize, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void frecps(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
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
  void fminnm(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fmls(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11001, rd, rn, rm);
  }
  template<typename T>
  void fsub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmin(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 0, size, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void frsqrts(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

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
  void uhadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00000, rd, rn, rm);
  }
  template<typename T>
  void uqadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00001, rd, rn, rm);
  }
  template<typename T>
  void urhadd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00010, rd, rn, rm);
  }
  template<typename T>
  void uhsub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00100, rd, rn, rm);
  }
  template<typename T>
  void uqsub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00101, rd, rn, rm);
  }
  template<typename T>
  void cmhi(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00110, rd, rn, rm);
  }
  template<typename T>
  void cmhs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b00111, rd, rn, rm);
  }
  template<typename T>
  void ushl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01000, rd, rn, rm);
  }
  template<typename T>
  void uqshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01001, rd, rn, rm);
  }
  template<typename T>
  void urshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01010, rd, rn, rm);
  }
  template<typename T>
  void uqrshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01011, rd, rn, rm);
  }
  template<typename T>
  void umax(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01100, rd, rn, rm);
  }
  template<typename T>
  void umin(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01101, rd, rn, rm);
  }
  template<typename T>
  void uabd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01110, rd, rn, rm);
  }
  template<typename T>
  void uaba(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b01111, rd, rn, rm);
  }
  template<typename T>
  void sub(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10000, rd, rn, rm);
  }
  template<typename T>
  void cmeq(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10001, rd, rn, rm);
  }
  template<typename T>
  void mls(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10010, rd, rn, rm);
  }
  template<typename T>
  void pmul(T rd, T rn, T rm) {
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, ARMEmitter::SubRegSize::i8Bit, 0b10011, rd, rn, rm);
  }
  template<typename T>
  void umaxp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10100, rd, rn, rm);
  }
  template<typename T>
  void uminp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10101, rd, rn, rm);
  }
  template<typename T>
  void sqrdmulh(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit && size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "8/64-bit subregsize not supported");
    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b10110, rd, rn, rm);
  }
  template<typename T>
  void fmaxnmp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void faddp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fmul(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11011, rd, rn, rm);
  }
  template<typename T>
  void fcmge(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void facge(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fmaxp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
      size == ARMEmitter::SubRegSize::i64Bit ?
        ARMEmitter::SubRegSize::i16Bit :
        ARMEmitter::SubRegSize::i8Bit;

    ASIMD3Same<T>(Op, 1, ConvertedSize, 0b11110, rd, rn, rm);
  }
  template<typename T>
  void fdiv(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    const FEXCore::ARMEmitter::SubRegSize ConvertedSize =
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
  void fminnmp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11000, rd, rn, rm);
  }
  template<typename T>
  void fabd(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11010, rd, rn, rm);
  }
  template<typename T>
  void fcmgt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11100, rd, rn, rm);
  }
  template<typename T>
  void facgt(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

    constexpr uint32_t Op = 0b0000'1110'0010'0000'0000'01 << 10;
    ASIMD3Same<T>(Op, 1, size, 0b11101, rd, rn, rm);
  }
  template<typename T>
  void fminp(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i64Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Only 32-bit and 64-bit subregsize supported");

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
  void fmov(FEXCore::ARMEmitter::SubRegSize size, T rd, float Value) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Unsupported fmov size");

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    uint32_t op;
    uint32_t cmode = 0b1111;
    uint32_t o2;
    uint32_t Imm;
    if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(vixl::aarch64::Assembler::IsImmFP16(vixl::Float16(Value)), "Invalid float");
      op = 0;
      o2 = 1;
      Imm = vixl::VFP::FP16ToImm8(vixl::Float16(Value));
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP32(Value), "Invalid float");
      op = 0;
      o2 = 0;
      Imm = vixl::VFP::FP32ToImm8(Value);
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(vixl::VFP::IsImmFP64(Value), "Invalid float");
      op = 1;
      o2 = 0;
      Imm = vixl::VFP::FP64ToImm8(Value);
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
  void movi(FEXCore::ARMEmitter::SubRegSize size, T rd, uint64_t Imm, uint16_t Shift = 0) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i8Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Unsupported smov size");

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    uint32_t cmode;
    uint32_t op;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Shift == 0, "8-bit can't have shift");
      LOGMAN_THROW_AA_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b1110;
      op = 0;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Shift == 0 || Shift == 8, "Shift by invalid amount");
      LOGMAN_THROW_AA_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b1000 | (Shift ? 0b10 : 0b00);
      op = 0;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Shift == 0 || Shift == 8 || Shift == 16 || Shift == 24, "Shift by invalid amount");
      LOGMAN_THROW_AA_FMT((Imm & ~0xFF) == 0, "Larger than 8-bit Imm not supported");
      cmode = 0b0000 | ((Shift >> 3) << 1);
      op = 0;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Shift == 0, "64-bit can't have shift");
      cmode = 0b1110;
      op = 1;

      // 64-bit movi doesn't behave like the smaller types
      // Each bit of the 8-bit imm encoding is expanded to a full 8-bits.
      // This gives us a full 64-bits for the final result but needs special handling.
      uint8_t NewImm{};
      for (size_t i = 0; i < 8; ++i) {
        const size_t BitOffset = i * 8;
        uint8_t Section = (Imm >> BitOffset) & 0xFF;
        LOGMAN_THROW_AA_FMT(Section == 0 || Section == 0xFF, "Invalid 64-bit constant encoding");
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
  void sshr(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void ssra(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void srshr(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void srsra(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void shl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void shrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void shrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void rshrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void rshrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqrshrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqrshrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sshll(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    size = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(Shift < SubregSizeInBits, "Shift must not be larger than incoming element size");

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10100, rn, rd);
  }

  ///< size is destination size
  void sshll2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    size = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(Shift < SubregSizeInBits, "Shift must not be larger than incoming element size");

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b10100, rn, rd);
  }
  ///< size is destination size
  void sxtl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    sshll(size, rd, rn, 0);
  }
  ///< size is destination size
  void sxtl2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    sshll2(size, rd, rn, 0);
  }

  template<typename T>
  void scvtf(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b11100, rn, rd);
  }

  template<typename T>
  void fcvtzs(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 0, immh, immb, 0b11111, rn, rd);
  }

  template<typename T>
  void ushr(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void usra(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void urshr(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void ursra(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sri(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sli(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshlu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void uqshl(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t Shift) {
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshrun(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqshrun2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqrshrun(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void sqrshrun2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
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
  void uqshrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
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
  void uqshrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
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
  void uqrshrn(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
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
  void uqrshrn2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
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
  void ushll(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    size = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b10100, rn, rd);
  }
  ///< size is destination size
  void ushll2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn, uint32_t Shift) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    size = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    const size_t SubregSizeInBits = SubRegSizeInBits(size);

    // Shift encoded a bit weirdly.
    // shift = immh:immb - esize but immh is /also/ used for element size.
    const uint32_t InvertedShift = SubregSizeInBits + Shift;
    const uint32_t immh = InvertedShift >> 3;
    const uint32_t immb = InvertedShift & 0b111;


    ASIMDShiftByImm(Op, 1, immh, immb, 0b10100, rn, rd);
  }
  void uxtl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::DRegister rn) {
    ushll(size, rd, rn, 0);
  }
  void uxtl2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::QRegister rd, FEXCore::ARMEmitter::QRegister rn) {
    ushll2(size, rd, rn, 0);
  }
  template<typename T>
  void ucvtf(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b11100, rn, rd);
  }

  template<typename T>
  void fcvtzu(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i8Bit, "Invalid size");
    if constexpr (std::is_same_v<FEXCore::ARMEmitter::DRegister, T>) {
      LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid element size with 64-bit {}", __func__);
    }

    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'01 << 10;
    const size_t SubregSizeInBits = SubRegSizeInBits(size);
    LOGMAN_THROW_AA_FMT(FractionalBits < SubregSizeInBits, "FractionalBits must not be larger than incoming element size");

    // fbits encoded a bit weirdly.
    // fbits = (esize * 2) - immh:immb but immh is /also/ used for element size.
    const uint32_t InvertedFractionalBits = (SubregSizeInBits * 2) - FractionalBits;
    const uint32_t immh = InvertedFractionalBits >> 3;
    const uint32_t immb = InvertedFractionalBits & 0b111;

    ASIMDShiftByImm(Op, 1, immh, immb, 0b11111, rn, rd);
  }

  // Advanced SIMD vector x indexed element
  ///< size is destination size
  void smlal(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void smlal2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmlal(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmlal2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void smlsl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void smlsl2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmlsl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmlsl2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void mul(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  void smull(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void smull2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmull(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void sqdmull2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqdmulh(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqrdmulh(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1110, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sudot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, FEXCore::ARMEmitter::SubRegSize::i8Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void bfdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, FEXCore::ARMEmitter::SubRegSize::i16Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmla(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0001, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmls(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0101, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmul(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i32Bit ||
                        size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    auto EncodedSubRegSize = size;

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      // Index encoded in H:L:M
      H = (Index >> 2) & 1;
      L = (Index >> 1) & 1;
      M = (Index >> 0) & 1;
      // ARM in their infinite wisdom decided to encode 16-bit as an 8-bit operation even though 16-bit was unallocated.
      EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize::i8Bit;
    }
    else if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      // Index encoded in H:L
      H = (Index >> 1) & 1;
      L = (Index >> 0) & 1;
      M = 0;
    }
    else {
      LOGMAN_THROW_A_FMT(std::is_same_v<FEXCore::ARMEmitter::QRegister, T>, "Can't encode DRegister with i64Bit");
      // Index encoded in H
      H = Index;
      L = 0;
      M = 0;
    }
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1001, H, EncodedSubRegSize, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmlal(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0000, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmlal2(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1000, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmlsl(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b0100, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void fmlsl2(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1100, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void usdot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  void bfmlalb(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, FEXCore::ARMEmitter::SubRegSize::i64Bit, rm.D(), rn.D(), rd.D());
  }
  void bfmlalt(FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    LOGMAN_THROW_A_FMT(Index < 8, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 2) & 1;
    L = (Index >> 1) & 1;
    M = (Index >> 0) & 1;
    ASIMDVectorXIndexedElement(0b0, L, M, 0b1111, H, FEXCore::ARMEmitter::SubRegSize::i64Bit, rm.Q(), rn.Q(), rd.Q());
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void mla(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  void umlal(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void umlal2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void mls(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  void umlsl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void umlsl2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void umull(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  void umull2(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i32Bit || size == FEXCore::ARMEmitter::SubRegSize::i64Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    const auto EncodedSubRegSize = FEXCore::ARMEmitter::SubRegSize(FEXCore::ToUnderlying(size) - 1);
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(EncodedSubRegSize), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i32Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqrdmlah(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void udot(T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_A_FMT(Index < 4, "Index must be less than the source register size");

    uint32_t H, L, M;
    // Index encoded in H:L
    // M overlaps rm register.
    H = (Index >> 1) & 1;
    L = (Index >> 0) & 1;
    M = 0;
    ASIMDVectorXIndexedElement(0b1, L, M, 0b1110, H, FEXCore::ARMEmitter::SubRegSize::i32Bit, rm, rn, rd);
  }

  template<typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void sqrdmlsh(FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm, uint32_t Index) {
    LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::SubRegSize::i16Bit || size == FEXCore::ARMEmitter::SubRegSize::i32Bit, "Invalid destination size");

    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(rm.Idx() < 16, "Rm can't be v16-v31 with half source size");
    }
    LOGMAN_THROW_A_FMT(Index < SubRegSizeInBits(size), "Index must be less than the source register size");

    uint32_t H, L, M;
    if (size == FEXCore::ARMEmitter::SubRegSize::i16Bit) {
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
  void scvtf(FEXCore::ARMEmitter::ScalarRegSize ScalarSize, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::Size GPRSize, FEXCore::ARMEmitter::Register rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(FractionalBits >= 1 && FractionalBits <= FEXCore::ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b00, 0b010, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void ucvtf(FEXCore::ARMEmitter::ScalarRegSize ScalarSize, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::Size GPRSize, FEXCore::ARMEmitter::Register rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(FractionalBits >= 1 && FractionalBits <= FEXCore::ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b00, 0b011, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void fcvtzs(FEXCore::ARMEmitter::Size GPRSize, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::ScalarRegSize ScalarSize, FEXCore::ARMEmitter::VRegister rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(FractionalBits >= 1 && FractionalBits <= FEXCore::ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b11, 0b000, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  void fcvtzu(FEXCore::ARMEmitter::Size GPRSize, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::ScalarRegSize ScalarSize, FEXCore::ARMEmitter::VRegister rn, uint32_t FractionalBits) {
    LOGMAN_THROW_AA_FMT(FractionalBits >= 1 && FractionalBits <= FEXCore::ARMEmitter::RegSizeInBits(GPRSize), "Fractional bits out of range");

    uint32_t Scale = 64 - FractionalBits;
    const auto ConvertedSize =
      ScalarSize == ARMEmitter::ScalarRegSize::i64Bit ? 0b01 :
      ScalarSize == ARMEmitter::ScalarRegSize::i32Bit ? 0b00 :
      ScalarSize == ARMEmitter::ScalarRegSize::i16Bit ? 0b11 : 0;

    ScalarConvertBetweenFPAndFixed(0, 0b11, 0b001, Scale, GPRSize, ConvertedSize, rn, rd);
  }

  // Conversion between floating-point and integer
  void fcvtns(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtns(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtns(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtnu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtnu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtnu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void scvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b010, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void scvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b010, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void scvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b010, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b011, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b011, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void ucvtf(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b011, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void fcvtas(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b100, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtas(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b100, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtas(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b100, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtau(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b101, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtau(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b101, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtau(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b101, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b110, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::Size::i64Bit, "Can't move SReg to 64-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b110, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::Size::i32Bit, "Can't move DReg to 32-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b110, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::VRegister rn, bool Upper) {
    if (Upper) {
      LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::Size::i64Bit, "Can only move upper with 64-bit elements");
    }
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, Upper ? 0b10 : 0b01, Upper ? 0b01 : 0b00, 0b110, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::HRegister rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b00, 0b111, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::SRegister rd, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::Size::i64Bit, "Can't move SReg to 64-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b00, 0b111, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::DRegister rd, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_AA_FMT(size != FEXCore::ARMEmitter::Size::i32Bit, "Can't move DReg to 32-bit");
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b00, 0b111, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void fmov(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::Register rn, bool Upper) {
    if (Upper) {
      LOGMAN_THROW_AA_FMT(size == FEXCore::ARMEmitter::Size::i64Bit, "Can only move upper with 64-bit elements");
    }
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, Upper ? 0b10 : 0b01, Upper ? 0b01 : 0b00, 0b111, FEXCore::ARMEmitter::ToReg(rd), rn);
  }
  void fcvtps(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b01, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtps(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b01, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtps(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b01, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtpu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b01, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtpu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b01, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtpu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b01, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtms(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b10, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtms(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b10, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtms(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b10, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtmu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b10, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtmu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b10, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtmu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b10, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzs(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b11, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzs(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b11, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzs(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzs(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::VRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b000, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::HRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b11, 0b11, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b00, 0b11, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }
  void fcvtzu(FEXCore::ARMEmitter::Size size, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::DRegister rn) {
    constexpr uint32_t Op = 0b0001'1110'001 << 21;
    ASIMDFloatConvBetweenInt(Op, size, 0, 0b01, 0b11, 0b001, rd, FEXCore::ARMEmitter::ToReg(rn));
  }

private:
  // Cryptographic AES
  void CryptoAES(uint32_t Op, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Cryptographic three-register SHA
  void Crypto3RegSHA(uint32_t Op, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
    uint32_t Instr = Op;

    Instr |= Encode_rm(rm);
    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Cryptographic two-register SHA
  void Crypto2RegSHA(uint32_t Op, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn) {
    uint32_t Instr = Op;

    Instr |= opcode << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);
    dc32(Instr);
  }

  // Advanced SIMD table lookup
  void ASIMDTable(uint32_t Op, uint32_t Q, uint32_t op2, uint32_t len, uint32_t op, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
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
  void ASIMDPermute(uint32_t Op, uint32_t Q, FEXCore::ARMEmitter::SubRegSize size, uint32_t opcode, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
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
  void ASIMDExtract(uint32_t Op, uint32_t Q, uint32_t op2, uint32_t imm4, FEXCore::ARMEmitter::VRegister rd, FEXCore::ARMEmitter::VRegister rn, FEXCore::ARMEmitter::VRegister rm) {
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMD2RegMisc(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMDAcrossLanes(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMD3Different(uint32_t Op, uint32_t U, uint32_t opcode, FEXCore::ARMEmitter::SubRegSize size, T rd, T rn, T rm) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMD3Same(uint32_t Op, uint32_t U, FEXCore::ARMEmitter::SubRegSize size, uint32_t opcode, T rd, T rn, T rm) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMDModifiedImm(uint32_t Op, uint32_t op, uint32_t cmode, uint32_t o2, uint32_t imm, T rd) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMDShiftByImm(uint32_t Op, uint32_t U, uint32_t immh, uint32_t immb, uint32_t opcode, T rn, T rd) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;
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
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMDVectorXIndexedElement(uint32_t U, uint32_t L, uint32_t M, uint32_t opcode, uint32_t H, FEXCore::ARMEmitter::SubRegSize size, T rm, T rn, T rd) {
    constexpr uint32_t Op = 0b0000'1111'0000'0000'0000'00 << 10;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
                                      FEXCore::ARMEmitter::Size GPRSize, uint32_t ScalarSize,
                                      T rn, T2 rd) {
    constexpr uint32_t Op = 0b0001'1110'000 << 21;
    const uint32_t SF = GPRSize == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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
  void ASIMDFloatConvBetweenInt(uint32_t Op, FEXCore::ARMEmitter::Size s, uint32_t S, uint32_t ptype, uint32_t rmode, uint32_t opcode, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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

  template<FEXCore::ARMEmitter::SubRegSize size, bool Load, typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ASIMDLoadStoreMultipleStructure(uint32_t Op, uint32_t opcode, T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;

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
  template<FEXCore::ARMEmitter::SubRegSize size, bool Load, uint32_t Count>
  void ASIMDSTLD(uint32_t Op, uint32_t Opcode, FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
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

  template<FEXCore::ARMEmitter::SubRegSize size, bool Load, uint32_t Count, typename T>
  void ASIMDSTLD(uint32_t Op, uint32_t Opcode, T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1U << 30 : 0;
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
  void ASIMDLoadStore(uint32_t Op, uint32_t L, uint32_t R, uint32_t opcode, uint32_t S, uint32_t size, FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
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


