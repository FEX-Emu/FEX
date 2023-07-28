/* ALU instruction emitters.
 *
 * Almost all of these operations have `ARMEmitter::Size` as their first argument.
 * This allows both 32-bit and 64-bit selection of how that instruction is going to operate.
 *
 * Some emitter operations explicitly use `XRegister` or `WRegister`.
 * This is usually due to the instruction only supporting one operating size.
 * Although in some cases is a minor convenience without any performance implications.
 *
 * FEX-Emu ALU operations usually have a 32-bit or 64-bit operating size encoded in the IR operation,
 * This allows FEX to use a single helper function which decodes to both handlers.
 */
private:
  static bool IsADRRange(int64_t Imm) {
    return Imm >= -1048576 && Imm <= 1048575;
  }
  static bool IsADRPRange(int64_t Imm) {
    return Imm >= -4294967296 && Imm <= 4294963200;
  }
  static bool IsADRPAligned(int64_t Imm) {
    return (Imm & 0xFFF) == 0;
  }
public:
  // PC relative
  void adr(FEXCore::ARMEmitter::Register rd, uint32_t Imm) {
    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }

  void adr(FEXCore::ARMEmitter::Register rd, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(IsADRRange(Imm), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }
  void adr(FEXCore::ARMEmitter::Register rd, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::ADR });
    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, 0);
  }

  void adr(FEXCore::ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      adr(rd, &Label->Backward);
    }
    else {
      adr(rd, &Label->Forward);
    }
  }

  void adrp(FEXCore::ARMEmitter::Register rd, uint32_t Imm) {
    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }

  void adrp(FEXCore::ARMEmitter::Register rd, BackwardLabel const* Label) {
    int64_t Imm = reinterpret_cast<int64_t>(Label->Location) - (GetCursorAddress<int64_t>() & ~0xFFFLL);
    LOGMAN_THROW_A_FMT(IsADRPRange(Imm) && IsADRPAligned(Imm), "Unscaled offset too large");

    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }
  void adrp(FEXCore::ARMEmitter::Register rd, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::ADRP });
    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, 0);
  }

  void adrp(FEXCore::ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      adrp(rd, &Label->Backward);
    }
    else {
      adrp(rd, &Label->Forward);
    }
  }

  void LongAddressGen(FEXCore::ARMEmitter::Register rd, BackwardLabel const* Label) {
    int64_t Imm = reinterpret_cast<int64_t>(Label->Location) - (GetCursorAddress<int64_t>());
    if (IsADRRange(Imm)) {
      // If the range is in ADR range then we can just use ADR.
      adr(rd, Label);
    }
    else if (IsADRPRange(Imm)) {
      int64_t ADRPImm = (reinterpret_cast<int64_t>(Label->Location) & ~0xFFFLL)
        - (GetCursorAddress<int64_t>() & ~0xFFFLL);

      // If the range is in the ADRP range then we can use ADRP.
      bool NeedsOffset = !IsADRPAligned(reinterpret_cast<uint64_t>(Label->Location));
      uint64_t AlignedOffset = reinterpret_cast<uint64_t>(Label->Location) & 0xFFFULL;

      // First emit ADRP
      adrp(rd, ADRPImm >> 12);

      if (NeedsOffset) {
        // Now even an add
        add(ARMEmitter::Size::i64Bit, rd, rd, AlignedOffset);
      }
    }
    else {
      LOGMAN_MSG_A_FMT("Unscaled offset too large");
      FEX_UNREACHABLE;
    }
  }
  void LongAddressGen(FEXCore::ARMEmitter::Register rd, ForwardLabel* Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::LONG_ADDRESS_GEN });
    // Emit a register index and a nop. These will be backpatched.
    dc32(rd.Idx());
    nop();
  }

  void LongAddressGen(FEXCore::ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      LongAddressGen(rd, &Label->Backward);
    }
    else {
      LongAddressGen(rd, &Label->Forward);
    }
  }

  // Add/subtract immediate
  void add(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0001'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  void adds(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0011'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }
  void cmn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    adds(s, FEXCore::ARMEmitter::Reg::zr, rn, Imm, LSL12);
  }
  void sub(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0101'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  void cmp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0111'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, FEXCore::ARMEmitter::Reg::rsp, rn, Imm, LSL12);
  }

  void subs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0111'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  // Min/max immediate
  void smax(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, int64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -128 && Imm <= 127, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0000, s, rd, rn, Imm);
  }

  void umax(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm <= 255, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0001, s, rd, rn, Imm);
  }

  void smin(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, int64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -128 && Imm <= 127, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0010, s, rd, rn, Imm);
  }

  void umin(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm <= 255, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0011, s, rd, rn, Imm);
  }

  // Logical immediate
  void and_(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = vixl::aarch64::Assembler::IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    and_(s, rd, rn, n, immr, imms);
  }

  void bic(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    and_(s, rd, rn, ~Imm);
  }

  void ands(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = vixl::aarch64::Assembler::IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    ands(s, rd, rn, n, immr, imms);
  }

  void bics(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    ands(s, rd, rn, ~Imm);
  }

  void orr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = vixl::aarch64::Assembler::IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    orr(s, rd, rn, n, immr, imms);
  }

  void eor(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = vixl::aarch64::Assembler::IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    eor(s, rd, rn, n, immr, imms);
  }

  // Move wide immediate
  void movn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b001'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }
  void mov(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, uint32_t Imm) {
    movz(s, rd, Imm, 0);
  }
  void mov(FEXCore::ARMEmitter::XRegister rd, uint32_t Imm) {
    movz(FEXCore::ARMEmitter::Size::i64Bit, rd.R(), Imm, 0);
  }
  void mov(FEXCore::ARMEmitter::WRegister rd, uint32_t Imm) {
    movz(FEXCore::ARMEmitter::Size::i32Bit, rd.R(), Imm, 0);
  }

  void movz(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b101'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }
  void movk(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b111'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }

  void movn(FEXCore::ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movn(FEXCore::ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movz(FEXCore::ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movz(FEXCore::ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movk(FEXCore::ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movk(FEXCore::ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movn(FEXCore::ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movn(FEXCore::ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }
  void movz(FEXCore::ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movz(FEXCore::ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }
  void movk(FEXCore::ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movk(FEXCore::ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }

  // Bitfield
  void sxtb(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    sbfm(s, rd, rn, 0, 7);
  }
  void sxth(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    sbfm(s, rd, rn, 0, 15);
  }
  void sxtw(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn) {
    sbfm(ARMEmitter::Size::i64Bit, rd, rn.X(), 0, 31);
  }
  void sbfx(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "sbfx needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSizeInBits(s), "Tried to sbfx a region larger than the register");
    sbfm(s, rd, rn, lsb, lsb + width - 1);
  }
  void asr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t shift) {
    LOGMAN_THROW_A_FMT(shift <= RegSizeInBits(s), "Tried to asr a region larger than the register");
    sbfm(s, rd, rn, shift, RegSizeInBits(s) - 1);
  }

  void uxtb(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 7);
  }
  void uxth(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 15);
  }
  void uxtw(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 31);
  }

  void ubfm(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0101'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }

  void lsl(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t shift) {
    const auto RegSize = RegSizeInBits(s);
    LOGMAN_THROW_A_FMT(shift < RegSize, "Tried to lsl a region larger than the register");
    ubfm(s, rd, rn, (RegSize - shift) % RegSize, RegSize - shift - 1);
  }
  void lsr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t shift) {
    const auto RegSize = RegSizeInBits(s);
    LOGMAN_THROW_A_FMT(shift < RegSize, "Tried to lsr a region larger than the register");
    ubfm(s, rd, rn, shift, RegSize - 1);
  }
  void ubfx(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "ubfx needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSizeInBits(s), "Tried to ubfx a region larger than the register");
    ubfm(s, rd, rn, lsb, lsb + width - 1);
  }

  void bfi(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    const auto RegSize = RegSizeInBits(s);
    LOGMAN_THROW_A_FMT(width > 0, "bfi needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSize, "Tried to bfi a region larger than the register");
    bfm(s, rd, rn, (RegSize - lsb) & (RegSize - 1), width - 1);
  }

  // Extract
  void extr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, uint32_t Imm) {
    constexpr uint32_t Op = 0b001'0011'100 << 21;
    LOGMAN_THROW_A_FMT(Imm < RegSizeInBits(s), "Tried to extr a region larger than the register");
    DataProcessing_Extract(Op, s, rd, rn, rm, Imm);
  }

  void ror(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm) {
    extr(s, rd, rn, rn, Imm);
  }

  // Data processing - 2 source
  void udiv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void sdiv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }

  void lslv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'00U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void lsrv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'01U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void asrv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void rorv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void crc32b(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32h(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'01U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32w(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'10U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32cb(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32ch(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'01U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32cw(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'10U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void smax(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'00U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void umax(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'01U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void smin(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void umin(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void subp(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'00U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void irg(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0001'00U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void gmi(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0001'01U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void pacga(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0011'00U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void crc32x(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'11U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void crc32cx(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'11U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void subps(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b011'1010'110U << 21) |
                        (0b0000'00U << 10);
    DataProcessing_2Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm);
  }

  // Data processing - 1 source
  void rbit(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev16(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'01U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10);
    DataProcessing_1Source(Op, FEXCore::ARMEmitter::Size::i32Bit, rd, rn);
  }
  void rev32(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10);
    DataProcessing_1Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn);
  }
  void clz(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void cls(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'01U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'11U << 10);
    DataProcessing_1Source(Op, FEXCore::ARMEmitter::Size::i64Bit, rd, rn);
  }
  void rev(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10) |
                        (s == ARMEmitter::Size::i64Bit ? (1U << 10) : 0);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void ctz(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'10U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void cnt(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'11U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void abs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0010'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }

  // TODO: PAUTH

  // Logical - shifted register
  void mov(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn) {
    orr(s, rd, FEXCore::ARMEmitter::Reg::zr, rn, ARMEmitter::ShiftType::LSL, 0);
  }
  void mov(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn) {
    orr(FEXCore::ARMEmitter::Size::i64Bit, rd.R(), FEXCore::ARMEmitter::Reg::zr, rn.R(), ARMEmitter::ShiftType::LSL, 0);
  }
  void mov(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn) {
    orr(FEXCore::ARMEmitter::Size::i32Bit, rd.R(), FEXCore::ARMEmitter::Reg::zr, rn.R(), ARMEmitter::ShiftType::LSL, 0);
  }

  void mvn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    orn(s, rd, FEXCore::ARMEmitter::Reg::zr, rn, Shift, amt);
  }

  void and_(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b000'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void ands(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b110'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void bic(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b000'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void bics(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b110'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void orr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b010'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }

  void orn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b010'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void eor(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b100'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void eon(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b100'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }

  // AddSub - shifted register
  void add(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    add(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void adds(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void cmn(FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i64Bit, FEXCore::ARMEmitter::XReg::zr, rn.R(), rm.R(), Shift, amt);
  }
  void sub(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void neg(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(rd, FEXCore::ARMEmitter::XReg::zr, rm, Shift, amt);
  }
  void cmp(FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i64Bit, FEXCore::ARMEmitter::Reg::rsp, rn.R(), rm.R(), Shift, amt);
  }
  void subs(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void negs(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(rd, FEXCore::ARMEmitter::XReg::zr, rm, Shift, amt);
  }

  void add(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    add(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void adds(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void cmn(FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i32Bit, FEXCore::ARMEmitter::WReg::zr, rn.R(), rm.R(), Shift, amt);
  }
  void sub(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void neg(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(rd, FEXCore::ARMEmitter::WReg::zr, rm, Shift, amt);
  }
  void cmp(FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i32Bit, FEXCore::ARMEmitter::Reg::rsp, rn.R(), rm.R(), Shift, amt);
  }
  void subs(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void negs(FEXCore::ARMEmitter::WRegister rd, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(rd, FEXCore::ARMEmitter::WReg::zr, rm, Shift, amt);
  }

  void add(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_AA_FMT(Shift != FEXCore::ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b000'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void adds(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_AA_FMT(Shift != FEXCore::ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b010'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void cmn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(s, FEXCore::ARMEmitter::Reg::zr, rn, rm, Shift, amt);
  }
  void sub(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_AA_FMT(Shift != FEXCore::ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b100'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void neg(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(s, rd, FEXCore::ARMEmitter::Reg::zr, rm, Shift, amt);
  }
  void cmp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(s, FEXCore::ARMEmitter::Reg::zr, rn, rm, Shift, amt);
  }

  void subs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_AA_FMT(Shift != FEXCore::ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b110'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void negs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift = FEXCore::ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(s, rd, FEXCore::ARMEmitter::Reg::zr, rm, Shift, amt);
  }

  // AddSub - extended register
  void add(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    LOGMAN_THROW_AA_FMT(Shift <= 4, "Shift amount is too large");
    constexpr uint32_t Op = 0b000'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void adds(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b010'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void cmn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    adds(s, FEXCore::ARMEmitter::Reg::zr, rn, rm, Option, Shift);
  }
  void sub(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b100'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void subs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b110'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void cmp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b110'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, FEXCore::ARMEmitter::Reg::zr, rn, rm, Option, Shift);
  }

  // AddSub - with carry
  void adc(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0001'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, FEXCore::ARMEmitter::ExtendedType::UXTB, 0);
  }
  void adcs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0011'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, FEXCore::ARMEmitter::ExtendedType::UXTB, 0);
  }
  void sbc(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0101'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, FEXCore::ARMEmitter::ExtendedType::UXTB, 0);
  }
  void sbcs(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0111'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, FEXCore::ARMEmitter::ExtendedType::UXTB, 0);
  }

  // Rotate right into flags
  void rmif(XRegister rn, uint32_t shift, uint32_t mask) {
    LOGMAN_THROW_AA_FMT(shift <= 63, "Shift must be within 0-63. Shift: {}", shift);
    LOGMAN_THROW_AA_FMT(mask <= 15, "Mask must be within 0-15. Mask: {}", mask);

    uint32_t Op = 0b1011'1010'0000'0000'0000'0100'0000'0000;
    Op |= rn.Idx() << 5;
    Op |= shift << 15;
    Op |= mask;

    dc32(Op);
  }

  // Evaluate into flags
  void setf8(WRegister rn) {
    constexpr uint32_t Op = 0b0011'1010'0000'0000'0000'1000'0000'1101;
    EvaluateIntoFlags(Op, 0, rn);
  }
  void setf16(WRegister rn) {
    constexpr uint32_t Op = 0b0011'1010'0000'0000'0000'1000'0000'1101;
    EvaluateIntoFlags(Op, 1, rn);
  }

  // Conditional compare - register
  void ccmn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 0, 0b00, 0, s, rn, rm, flags, Cond);
  }
  void ccmp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 1, 0b00, 0, s, rn, rm, flags, Cond);
  }

  // Conditional compare - immediate
  void ccmn(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, uint32_t rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    LOGMAN_THROW_A_FMT((rm & ~0b1'1111) == 0, "Comparison imm too large");
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 0, 0b10, 0, s, rn, rm, flags, Cond);
  }
  void ccmp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, uint32_t rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    LOGMAN_THROW_A_FMT((rm & ~0b1'1111) == 0, "Comparison imm too large");
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 1, 0b10, 0, s, rn, rm, flags, Cond);
  }

  // Conditional select
  void csel(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b00, s, rd, rn, rm, Cond);
  }
  void cset(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b01, s, rd, FEXCore::ARMEmitter::Reg::zr, FEXCore::ARMEmitter::Reg::zr, static_cast<FEXCore::ARMEmitter::Condition>(FEXCore::ToUnderlying(Cond) ^ FEXCore::ToUnderlying(FEXCore::ARMEmitter::Condition::CC_NE)));
  }
  void csinc(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b01, s, rd, rn, rm, Cond);
  }
  void csinv(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 1, 0b00, s, rd, rn, rm, Cond);
  }
  void csneg(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 1, 0b01, s, rd, rn, rm, Cond);
  }
  void cneg(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Condition Cond) {
    csneg(s, rd, rn, rn, static_cast<FEXCore::ARMEmitter::Condition>(FEXCore::ToUnderlying(Cond) ^ FEXCore::ToUnderlying(FEXCore::ARMEmitter::Condition::CC_NE)));
  }

  // Data processing - 3 source
  void madd(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Register ra) {
    constexpr uint32_t Op = 0b001'1011'000U << 21;
    DataProcessing_3Source(Op, 0, s, rd, rn, rm, ra);
  }
  void mul(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    madd(s, rd, rn, rm, XReg::zr);
  }
  void msub(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Register ra) {
    constexpr uint32_t Op = 0b001'1011'000U << 21;
    DataProcessing_3Source(Op, 1, s, rd, rn, rm, ra);
  }
  void mneg(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    msub(s, rd, rn, rm, XReg::zr);
  }
  void smaddl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'001U << 21;
    DataProcessing_3Source(Op, 0, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void smull(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    smaddl(rd, rn, rm, XReg::zr);
  }
  void smsubl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'001U << 21;
    DataProcessing_3Source(Op, 1, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void smnegl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    smsubl(rd, rn, rm, XReg::zr);
  }
  void smulh(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = 0b001'1011'010U << 21;
    DataProcessing_3Source(Op, 0, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, FEXCore::ARMEmitter::Reg::zr);
  }
  void umaddl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'101U << 21;
    DataProcessing_3Source(Op, 0, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void umull(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    umaddl(rd, rn, rm, XReg::zr);
  }
  void umsubl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm, FEXCore::ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'101U << 21;
    DataProcessing_3Source(Op, 1, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void umnegl(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::WRegister rn, FEXCore::ARMEmitter::WRegister rm) {
    umsubl(rd, rn, rm, XReg::zr);
  }
  void umulh(FEXCore::ARMEmitter::XRegister rd, FEXCore::ARMEmitter::XRegister rn, FEXCore::ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = 0b001'1011'110U << 21;
    DataProcessing_3Source(Op, 0, FEXCore::ARMEmitter::Size::i64Bit, rd, rn, rm, FEXCore::ARMEmitter::Reg::zr);
  }

private:
  void and_(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b001'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void ands(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b111'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void orr(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b011'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void eor(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b101'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }

  void sbfm(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0001'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }
  void bfm(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0011'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }
  // 4.1.64 - Data processing - Immediate
  void DataProcessing_PCRel_Imm(uint32_t Op, FEXCore::ARMEmitter::Register rd, uint32_t Imm) {
    // Ensure the immediate is masked.
    Imm &= 0b1'1111'1111'1111'1111'1111U;

    uint32_t Instr = Op;

    Instr |= (Imm & 0b11) << 29;
    Instr |= (Imm >> 2) << 5;
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void DataProcessing_AddSub_Imm(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t Imm, bool LSL12) {
    bool TooLarge = (Imm & ~0b1111'1111'1111U) != 0;
    if (TooLarge && !LSL12 && ((Imm >> 12) & ~0b1111'1111'1111U) == 0) {
      // We can convert an immediate
      TooLarge = false;
      LSL12 = true;
      Imm >>= 12;
    }
    LOGMAN_THROW_AA_FMT(TooLarge == false, "Imm amount too large: 0x{:x}", Imm);

    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= LSL12 << 22;
    Instr |= Imm << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Min/max immediate
  void MinMaxImmediate(uint32_t opc, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint64_t Imm) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = 0b1'0001'11U << 22;

    Instr |= SF;
    Instr |= opc << 18;
    Instr |= (Imm & 0xFF) << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Move Wide
  void DataProcessing_MoveWide(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Imm << 5;
    Instr |= Offset << 21;
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Logical immediate
  void DataProcessing_Logical_Imm(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= n << 22;
    Instr |= immr << 16;
    Instr |= imms << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void DataProcessing_Extract(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, uint32_t Imm) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    // Current ARMv8 spec hardcodes SF == N for this class of instructions.
    // Anythign else is undefined behaviour.
    const uint32_t N  = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 22) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= N;
    Instr |= Encode_rm(rm);
    Instr |= Imm << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Data-processing - 2 source
  void DataProcessing_2Source(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rm(rm);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Data processing - 1 source
  template<typename T>
  void DataProcessing_1Source(uint32_t Op, FEXCore::ARMEmitter::Size s, T rd, T rn) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // AddSub - shifted register
  void DataProcessing_Shifted_Reg(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ShiftType Shift, uint32_t amt) {
    LOGMAN_THROW_AA_FMT((amt & ~0b11'1111U) == 0, "Shift amount too large");
    if (s == FEXCore::ARMEmitter::Size::i32Bit) {
      LOGMAN_THROW_AA_FMT(amt < 32, "Shift amount for 32-bit must be below 32");
    }

    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= FEXCore::ToUnderlying(Shift) << 22;
    Instr |= Encode_rm(rm);
    Instr |= static_cast<uint32_t>(amt) << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // AddSub - extended register
  void DataProcessing_Extended_Reg(uint32_t Op, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rm(rm);
    Instr |= FEXCore::ToUnderlying(Option) << 13;
    Instr |= static_cast<uint32_t>(Shift) << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }
  // Conditional compare - register
  template<typename T>
  void ConditionalCompare(uint32_t Op, uint32_t o1, uint32_t o2, uint32_t o3, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rn, T rm, FEXCore::ARMEmitter::StatusFlags flags, FEXCore::ARMEmitter::Condition Cond) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= o1 << 30;
    Instr |= Encode_rm(rm);
    Instr |= FEXCore::ToUnderlying(Cond) << 12;
    Instr |= o2 << 10;
    Instr |= Encode_rn(rn);
    Instr |= o3 << 4;
    Instr |= FEXCore::ToUnderlying(flags);

    dc32(Instr);
  }

  template<typename T>
  void ConditionalCompare(uint32_t Op, uint32_t o1, uint32_t o2, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, T rm, FEXCore::ARMEmitter::Condition Cond) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= o1 << 30;
    Instr |= Encode_rm(rm);
    Instr |= FEXCore::ToUnderlying(Cond) << 12;
    Instr |= o2 << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Data-processing - 3 source
  void DataProcessing_3Source(uint32_t Op, uint32_t Op0, FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Register ra) {
    const uint32_t SF = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rm(rm);
    Instr |= Op0 << 15;
    Instr |= Encode_ra(ra);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void EvaluateIntoFlags(uint32_t op, uint32_t size, WRegister rn) {
    uint32_t Instr = op;
    Instr |= size << 14;
    Instr |= rn.Idx() << 5;
    dc32(Instr);
  }


