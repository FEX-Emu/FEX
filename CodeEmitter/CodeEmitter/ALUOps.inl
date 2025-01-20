// SPDX-License-Identifier: MIT
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

#pragma once
#ifndef INCLUDED_BY_EMITTER
#include <CodeEmitter/Emitter.h>
namespace ARMEmitter {
struct EmitterOps : Emitter {
#endif

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
  void adr(ARMEmitter::Register rd, uint32_t Imm) {
    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }

  void adr(ARMEmitter::Register rd, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(IsADRRange(Imm), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }
  template<typename LabelType>
  requires (std::is_same_v<LabelType, ForwardLabel> || std::is_same_v<LabelType, SingleUseForwardLabel>)
  void adr(ARMEmitter::Register rd, LabelType *Label) {
    AddLocationToLabel(Label, SingleUseForwardLabel{ .Location = GetCursorAddress<uint8_t*>(), .Type = SingleUseForwardLabel::InstType::ADR });
    constexpr uint32_t Op = 0b0001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, 0);
  }

  void adr(ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      adr(rd, &Label->Backward);
    }
    else {
      adr(rd, &Label->Forward);
    }
  }

  void adrp(ARMEmitter::Register rd, uint32_t Imm) {
    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }

  void adrp(ARMEmitter::Register rd, BackwardLabel const* Label) {
    int64_t Imm = reinterpret_cast<int64_t>(Label->Location) - (GetCursorAddress<int64_t>() & ~0xFFFLL);
    LOGMAN_THROW_A_FMT(IsADRPRange(Imm) && IsADRPAligned(Imm), "Unscaled offset too large");

    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, Imm);
  }
  template<typename LabelType>
  requires (std::is_same_v<LabelType, ForwardLabel> || std::is_same_v<LabelType, SingleUseForwardLabel>)
  void adrp(ARMEmitter::Register rd, LabelType *Label) {
    AddLocationToLabel(Label, SingleUseForwardLabel{ .Location = GetCursorAddress<uint8_t*>(), .Type = SingleUseForwardLabel::InstType::ADRP });
    constexpr uint32_t Op = 0b1001'0000 << 24;
    DataProcessing_PCRel_Imm(Op, rd, 0);
  }

  void adrp(ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      adrp(rd, &Label->Backward);
    }
    else {
      adrp(rd, &Label->Forward);
    }
  }

  void LongAddressGen(ARMEmitter::Register rd, BackwardLabel const* Label) {
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
  void LongAddressGen(ARMEmitter::Register rd, ForwardLabel* Label) {
    Label->Insts.emplace_back(SingleUseForwardLabel{ .Location = GetCursorAddress<uint8_t*>(), .Type = SingleUseForwardLabel::InstType::LONG_ADDRESS_GEN });
    // Emit a register index and a nop. These will be backpatched.
    dc32(rd.Idx());
    nop();
  }

  void LongAddressGen(ARMEmitter::Register rd, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      LongAddressGen(rd, &Label->Backward);
    }
    else {
      LongAddressGen(rd, &Label->Forward);
    }
  }

  // Add/subtract immediate
  void add(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0001'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  void adds(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0011'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }
  void cmn(ARMEmitter::Size s, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    adds(s, ARMEmitter::Reg::zr, rn, Imm, LSL12);
  }
  void sub(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0101'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  void cmp(ARMEmitter::Size s, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0111'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, ARMEmitter::Reg::rsp, rn, Imm, LSL12);
  }

  void subs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm, bool LSL12 = false) {
    constexpr uint32_t Op = 0b0111'0001'0 << 23;
    DataProcessing_AddSub_Imm(Op, s, rd, rn, Imm, LSL12);
  }

  // Min/max immediate
  void smax(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, int64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -128 && Imm <= 127, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0000, s, rd, rn, Imm);
  }

  void umax(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm <= 255, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0001, s, rd, rn, Imm);
  }

  void smin(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, int64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -128 && Imm <= 127, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0010, s, rd, rn, Imm);
  }

  void umin(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    LOGMAN_THROW_A_FMT(Imm <= 255, "{} Immediate too large", __func__);
    MinMaxImmediate(0b0011, s, rd, rn, Imm);
  }

  // Logical immediate
  void and_(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    and_(s, rd, rn, n, immr, imms);
  }

  void bic(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    and_(s, rd, rn, ~Imm);
  }

  void ands(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    ands(s, rd, rn, n, immr, imms);
  }

  void bics(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    ands(s, rd, rn, ~Imm);
  }

  void orr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    orr(s, rd, rn, n, immr, imms);
  }

  void eor(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    uint32_t n, immr, imms;
    [[maybe_unused]] const auto IsImm = IsImmLogical(Imm,
                           RegSizeInBits(s),
                           &n,
                           &imms,
                           &immr);
    LOGMAN_THROW_A_FMT(IsImm, "Couldn't encode immediate to logical op");
    eor(s, rd, rn, n, immr, imms);
  }

  void tst(ARMEmitter::Size s, Register rn, uint64_t imm) {
    ands(s, Reg::zr, rn, imm);
  }

  // Move wide immediate
  void movn(ARMEmitter::Size s, ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b001'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }
  void mov(ARMEmitter::Size s, ARMEmitter::Register rd, uint32_t Imm) {
    movz(s, rd, Imm, 0);
  }
  void mov(ARMEmitter::XRegister rd, uint32_t Imm) {
    movz(ARMEmitter::Size::i64Bit, rd.R(), Imm, 0);
  }
  void mov(ARMEmitter::WRegister rd, uint32_t Imm) {
    movz(ARMEmitter::Size::i32Bit, rd.R(), Imm, 0);
  }

  void movz(ARMEmitter::Size s, ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b101'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }
  void movk(ARMEmitter::Size s, ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset = 0) {
    LOGMAN_THROW_A_FMT((Imm & 0xFFFF0000U) == 0, "Upper bits of move wide not valid");
    LOGMAN_THROW_A_FMT((Offset % 16) == 0, "Offset must be 16bit aligned");

    constexpr uint32_t Op = 0b111'0010'100 << 21;
    DataProcessing_MoveWide(Op, s, rd, Imm, Offset >> 4);
  }

  void movn(ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movn(ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movz(ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movz(ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movk(ARMEmitter::XRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movk(ARMEmitter::Size::i64Bit, rd.R(), Imm, Offset);
  }
  void movn(ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movn(ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }
  void movz(ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movz(ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }
  void movk(ARMEmitter::WRegister rd, uint32_t Imm, uint32_t Offset = 0) {
    movk(ARMEmitter::Size::i32Bit, rd.R(), Imm, Offset);
  }

  // Bitfield
  void sxtb(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    sbfm(s, rd, rn, 0, 7);
  }
  void sxth(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    sbfm(s, rd, rn, 0, 15);
  }
  void sxtw(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn) {
    sbfm(ARMEmitter::Size::i64Bit, rd, rn.X(), 0, 31);
  }
  void sbfx(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "sbfx needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSizeInBits(s), "Tried to sbfx a region larger than the register");
    sbfm(s, rd, rn, lsb, lsb + width - 1);
  }
  void sbfiz(ARMEmitter::Size s, Register rd, Register rn, uint32_t lsb, uint32_t width) {
    xbfiz_helper(true, s, rd, rn, lsb, width);
  }
  void asr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t shift) {
    const auto RegSize_m1 = RegSizeInBits(s) - 1;
    shift &= RegSize_m1;
    sbfm(s, rd, rn, shift, RegSize_m1);
  }

  void uxtb(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 7);
  }
  void uxth(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 15);
  }
  void uxtw(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    ubfm(s, rd, rn, 0, 31);
  }

  void ubfm(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0101'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }

  void ubfiz(ARMEmitter::Size s, Register rd, Register rn, uint32_t lsb, uint32_t width) {
    xbfiz_helper(false, s, rd, rn, lsb, width);
  }

  void lsl(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t shift) {
    const auto RegSize_m1 = RegSizeInBits(s) - 1;
    shift &= RegSize_m1;
    ubfm(s, rd, rn, (RegSizeInBits(s) - shift) & RegSize_m1, RegSize_m1 - shift);
  }
  void lsr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t shift) {
    const auto RegSize_m1 = RegSizeInBits(s) - 1;
    shift &= RegSize_m1;
    ubfm(s, rd, rn, shift, RegSize_m1);
  }
  void ubfx(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "ubfx needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSizeInBits(s), "Tried to ubfx a region larger than the register");
    ubfm(s, rd, rn, lsb, lsb + width - 1);
  }

  void bfi(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t lsb, uint32_t width) {
    const auto RegSize = RegSizeInBits(s);
    LOGMAN_THROW_A_FMT(width > 0, "bfc/bfi needs width > 0");
    LOGMAN_THROW_A_FMT((lsb + width) <= RegSize, "Tried to bfc/bfi a region larger than the register");
    bfm(s, rd, rn, (RegSize - lsb) & (RegSize - 1), width - 1);
  }
  void bfc(ARMEmitter::Size s, Register rd, uint32_t lsb, uint32_t width) {
    bfi(s, rd, Reg::zr, lsb, width);
  }
  void bfxil(ARMEmitter::Size s, Register rd, Register rn, uint32_t lsb, uint32_t width) {
    [[maybe_unused]] const auto reg_size_bits = RegSizeInBits(s);
    const auto lsb_p_width = lsb + width;

    LOGMAN_THROW_A_FMT(width >= 1, "bfxil needs width >= 1");
    LOGMAN_THROW_A_FMT(lsb_p_width <= reg_size_bits, "bfxil lsb + width ({}) must be <= {}. lsb={}, width={}",
                       lsb_p_width, reg_size_bits, lsb, width);

    bfm(s, rd, rn, lsb, lsb_p_width - 1);
  }

  // Extract
  void extr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, uint32_t Imm) {
    constexpr uint32_t Op = 0b001'0011'100 << 21;
    LOGMAN_THROW_A_FMT(Imm < RegSizeInBits(s), "Tried to extr a region larger than the register");
    DataProcessing_Extract(Op, s, rd, rn, rm, Imm);
  }

  void ror(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm) {
    Imm &= RegSizeInBits(s) - 1;
    extr(s, rd, rn, rn, Imm);
  }

  // Data processing - 2 source
  void udiv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void sdiv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }

  void lslv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'00U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void lsrv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'01U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void asrv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void rorv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0010'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void crc32b(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32h(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'01U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32w(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'10U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32cb(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32ch(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'01U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void crc32cw(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'10U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i32Bit, rd, rn, rm);
  }
  void smax(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'00U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void umax(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'01U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void smin(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'10U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void umin(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0110'11U << 10);
    DataProcessing_2Source(Op, s, rd, rn, rm);
  }
  void subp(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0000'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void irg(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0001'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void gmi(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0001'01U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void pacga(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0011'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void crc32x(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0100'11U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void crc32cx(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b001'1010'110U << 21) |
                        (0b0101'11U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }
  void subps(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = (0b011'1010'110U << 21) |
                        (0b0000'00U << 10);
    DataProcessing_2Source(Op, ARMEmitter::Size::i64Bit, rd, rn, rm);
  }

  // Data processing - 1 source
  void rbit(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev16(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'01U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10);
    DataProcessing_1Source(Op, ARMEmitter::Size::i32Bit, rd, rn);
  }
  void rev32(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10);
    DataProcessing_1Source(Op, ARMEmitter::Size::i64Bit, rd, rn);
  }
  void clz(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void cls(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'01U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void rev(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'11U << 10);
    DataProcessing_1Source(Op, ARMEmitter::Size::i64Bit, rd, rn);
  }
  void rev(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0000'10U << 10) |
                        (s == ARMEmitter::Size::i64Bit ? (1U << 10) : 0);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void ctz(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'10U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void cnt(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0001'11U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }
  void abs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    constexpr uint32_t Op = (0b101'1010'110U << 21) |
                        (0b0'0000U << 16) |
                        (0b0010'00U << 10);
    DataProcessing_1Source(Op, s, rd, rn);
  }

  // TODO: PAUTH

  // Logical - shifted register
  void mov(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn) {
    orr(s, rd, ARMEmitter::Reg::zr, rn, ARMEmitter::ShiftType::LSL, 0);
  }
  void mov(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn) {
    orr(ARMEmitter::Size::i64Bit, rd.R(), ARMEmitter::Reg::zr, rn.R(), ARMEmitter::ShiftType::LSL, 0);
  }
  void mov(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn) {
    orr(ARMEmitter::Size::i32Bit, rd.R(), ARMEmitter::Reg::zr, rn.R(), ARMEmitter::ShiftType::LSL, 0);
  }

  void mvn(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    orn(s, rd, ARMEmitter::Reg::zr, rn, Shift, amt);
  }

  void and_(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b000'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void ands(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b110'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void bic(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b000'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void bics(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b110'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void orr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b010'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void tst(ARMEmitter::Size s, Register rn, Register rm, ShiftType shift = ShiftType::LSL, uint32_t amt = 0) {
    ands(s, Reg::zr, rn, rm, shift, amt);
  }

  void orn(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b010'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void eor(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b100'1010'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void eon(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    constexpr uint32_t Op = 0b100'1010'001U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }

  // AddSub - shifted register
  void add(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    add(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void adds(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void cmn(ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::zr, rn.R(), rm.R(), Shift, amt);
  }
  void sub(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void neg(ARMEmitter::XRegister rd, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(rd, ARMEmitter::XReg::zr, rm, Shift, amt);
  }
  void cmp(ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, rn.R(), rm.R(), Shift, amt);
  }
  void subs(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i64Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void negs(ARMEmitter::XRegister rd, ARMEmitter::XRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(rd, ARMEmitter::XReg::zr, rm, Shift, amt);
  }

  void add(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    add(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void adds(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void cmn(ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(ARMEmitter::Size::i32Bit, ARMEmitter::WReg::zr, rn.R(), rm.R(), Shift, amt);
  }
  void sub(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void neg(ARMEmitter::WRegister rd, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(rd, ARMEmitter::WReg::zr, rm, Shift, amt);
  }
  void cmp(ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::rsp, rn.R(), rm.R(), Shift, amt);
  }
  void subs(ARMEmitter::WRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(ARMEmitter::Size::i32Bit, rd.R(), rn.R(), rm.R(), Shift, amt);
  }
  void negs(ARMEmitter::WRegister rd, ARMEmitter::WRegister rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(rd, ARMEmitter::WReg::zr, rm, Shift, amt);
  }

  void add(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_A_FMT(Shift != ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b000'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void adds(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_A_FMT(Shift != ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b010'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void cmn(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    adds(s, ARMEmitter::Reg::zr, rn, rm, Shift, amt);
  }
  void sub(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_A_FMT(Shift != ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b100'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void neg(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    sub(s, rd, ARMEmitter::Reg::zr, rm, Shift, amt);
  }
  void cmp(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(s, ARMEmitter::Reg::zr, rn, rm, Shift, amt);
  }

  void subs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    LOGMAN_THROW_A_FMT(Shift != ARMEmitter::ShiftType::ROR, "Doesn't support ROR");
    constexpr uint32_t Op = 0b110'1011'000U << 21;
    DataProcessing_Shifted_Reg(Op, s, rd, rn, rm, Shift, amt);
  }
  void negs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift = ARMEmitter::ShiftType::LSL, uint32_t amt = 0) {
    subs(s, rd, ARMEmitter::Reg::zr, rm, Shift, amt);
  }

  // AddSub - extended register
  void add(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    LOGMAN_THROW_A_FMT(Shift <= 4, "Shift amount is too large");
    constexpr uint32_t Op = 0b000'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void adds(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b010'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void cmn(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    adds(s, ARMEmitter::Reg::zr, rn, rm, Option, Shift);
  }
  void sub(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b100'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void subs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b110'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, Option, Shift);
  }
  void cmp(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift = 0) {
    constexpr uint32_t Op = 0b110'1011'001U << 21;
    DataProcessing_Extended_Reg(Op, s, ARMEmitter::Reg::zr, rn, rm, Option, Shift);
  }

  // AddSub - with carry
  void adc(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0001'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, ARMEmitter::ExtendedType::UXTB, 0);
  }
  void adcs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0011'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, ARMEmitter::ExtendedType::UXTB, 0);
  }
  void sbc(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0101'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, ARMEmitter::ExtendedType::UXTB, 0);
  }
  void sbcs(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0111'1010'000U << 21;
    DataProcessing_Extended_Reg(Op, s, rd, rn, rm, ARMEmitter::ExtendedType::UXTB, 0);
  }
  void ngc(ARMEmitter::Size s, Register rd, Register rm) {
    sbc(s, rd, Reg::zr, rm);
  }
  void ngcs(ARMEmitter::Size s, Register rd, Register rm) {
    sbcs(s, rd, Reg::zr, rm);
  }

  // Rotate right into flags
  void rmif(XRegister rn, uint32_t shift, uint32_t mask) {
    LOGMAN_THROW_A_FMT(shift <= 63, "Shift must be within 0-63. Shift: {}", shift);
    LOGMAN_THROW_A_FMT(mask <= 15, "Mask must be within 0-15. Mask: {}", mask);

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

  void cfinv() {
    constexpr uint32_t Op = 0b1101'0101'0000'0000'0100'0000'0001'1111;
    dc32(Op);
  }

  void axflag() {
    constexpr uint32_t Op = 0b1101'0101'0000'0000'0100'0000'0101'1111;
    dc32(Op);
  }

  void xaflag() {
    constexpr uint32_t Op = 0b1101'0101'0000'0000'0100'0000'0011'1111;
    dc32(Op);
  }

  // Conditional compare - register
  void ccmn(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::StatusFlags flags, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 0, 0b00, 0, s, rn, rm, flags, Cond);
  }
  void ccmp(ARMEmitter::Size s, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::StatusFlags flags, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 1, 0b00, 0, s, rn, rm, flags, Cond);
  }

  // Conditional compare - immediate
  void ccmn(ARMEmitter::Size s, ARMEmitter::Register rn, uint32_t rm, ARMEmitter::StatusFlags flags, ARMEmitter::Condition Cond) {
    LOGMAN_THROW_A_FMT((rm & ~0b1'1111) == 0, "Comparison imm too large");
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 0, 0b10, 0, s, rn, rm, flags, Cond);
  }
  void ccmp(ARMEmitter::Size s, ARMEmitter::Register rn, uint32_t rm, ARMEmitter::StatusFlags flags, ARMEmitter::Condition Cond) {
    LOGMAN_THROW_A_FMT((rm & ~0b1'1111) == 0, "Comparison imm too large");
    constexpr uint32_t Op = 0b0011'1010'010 << 21;
    ConditionalCompare(Op, 1, 0b10, 0, s, rn, rm, flags, Cond);
  }

  // Conditional select
  void csel(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b00, s, rd, rn, rm, Cond);
  }
  void cset(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b01, s, rd, ARMEmitter::Reg::zr, ARMEmitter::Reg::zr, static_cast<ARMEmitter::Condition>(FEXCore::ToUnderlying(Cond) ^ FEXCore::ToUnderlying(ARMEmitter::Condition::CC_NE)));
  }
  void csinc(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 0, 0b01, s, rd, rn, rm, Cond);
  }
  void csinv(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 1, 0b00, s, rd, rn, rm, Cond);
  }
  void csneg(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Condition Cond) {
    constexpr uint32_t Op = 0b0001'1010'100 << 21;
    ConditionalCompare(Op, 1, 0b01, s, rd, rn, rm, Cond);
  }
  void cneg(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Condition Cond) {
    csneg(s, rd, rn, rn, InvertCondition(Cond));
  }
  void cinc(ARMEmitter::Size s, Register rd, Register rn, Condition cond) {
    csinc(s, rd, rn, rn, InvertCondition(cond));
  }
  void cinv(ARMEmitter::Size s, Register rd, Register rn, Condition cond) {
    csinv(s, rd, rn, rn, InvertCondition(cond));
  }
  void csetm(ARMEmitter::Size s, Register rd, Condition cond) {
    csinv(s, rd, Reg::zr, Reg::zr, InvertCondition(cond));
  }

  // Data processing - 3 source
  void madd(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Register ra) {
    constexpr uint32_t Op = 0b001'1011'000U << 21;
    DataProcessing_3Source(Op, 0, s, rd, rn, rm, ra);
  }
  void mul(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    madd(s, rd, rn, rm, XReg::zr);
  }
  void msub(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Register ra) {
    constexpr uint32_t Op = 0b001'1011'000U << 21;
    DataProcessing_3Source(Op, 1, s, rd, rn, rm, ra);
  }
  void mneg(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    msub(s, rd, rn, rm, XReg::zr);
  }
  void smaddl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'001U << 21;
    DataProcessing_3Source(Op, 0, ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void smull(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    smaddl(rd, rn, rm, XReg::zr);
  }
  void smsubl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'001U << 21;
    DataProcessing_3Source(Op, 1, ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void smnegl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    smsubl(rd, rn, rm, XReg::zr);
  }
  void smulh(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = 0b001'1011'010U << 21;
    DataProcessing_3Source(Op, 0, ARMEmitter::Size::i64Bit, rd, rn, rm, ARMEmitter::Reg::zr);
  }
  void umaddl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'101U << 21;
    DataProcessing_3Source(Op, 0, ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void umull(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    umaddl(rd, rn, rm, XReg::zr);
  }
  void umsubl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm, ARMEmitter::XRegister ra) {
    constexpr uint32_t Op = 0b001'1011'101U << 21;
    DataProcessing_3Source(Op, 1, ARMEmitter::Size::i64Bit, rd, rn, rm, ra);
  }
  void umnegl(ARMEmitter::XRegister rd, ARMEmitter::WRegister rn, ARMEmitter::WRegister rm) {
    umsubl(rd, rn, rm, XReg::zr);
  }
  void umulh(ARMEmitter::XRegister rd, ARMEmitter::XRegister rn, ARMEmitter::XRegister rm) {
    constexpr uint32_t Op = 0b001'1011'110U << 21;
    DataProcessing_3Source(Op, 0, ARMEmitter::Size::i64Bit, rd, rn, rm, ARMEmitter::Reg::zr);
  }

private:
  static constexpr Condition InvertCondition(Condition cond) {
    // These behave as always, so it makes no sense to allow inverting these.
    LOGMAN_THROW_A_FMT(cond != Condition::CC_AL && cond != Condition::CC_NV,
                        "Cannot invert CC_AL or CC_NV");
    return static_cast<Condition>(FEXCore::ToUnderlying(cond) ^ 1);
  }

  void and_(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b001'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void ands(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b111'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void orr(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b011'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }
  void eor(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b101'0010'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, n, immr, imms);
  }

  void sbfm(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0001'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }
  void bfm(ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t immr, uint32_t imms) {
    constexpr uint32_t Op = 0b0011'0011'00 << 22;
    DataProcessing_Logical_Imm(Op, s, rd, rn, s == ARMEmitter::Size::i64Bit, immr, imms);
  }
  // 4.1.64 - Data processing - Immediate
  void DataProcessing_PCRel_Imm(uint32_t Op, ARMEmitter::Register rd, uint32_t Imm) {
    // Ensure the immediate is masked.
    Imm &= 0b1'1111'1111'1111'1111'1111U;

    uint32_t Instr = Op;

    Instr |= (Imm & 0b11) << 29;
    Instr |= (Imm >> 2) << 5;
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void DataProcessing_AddSub_Imm(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t Imm, bool LSL12) {
    bool TooLarge = (Imm & ~0b1111'1111'1111U) != 0;
    if (TooLarge && !LSL12 && ((Imm >> 12) & ~0b1111'1111'1111U) == 0) {
      // We can convert an immediate
      TooLarge = false;
      LSL12 = true;
      Imm >>= 12;
    }
    LOGMAN_THROW_A_FMT(TooLarge == false, "Imm amount too large: 0x{:x}", Imm);

    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= LSL12 << 22;
    Instr |= Imm << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Min/max immediate
  void MinMaxImmediate(uint32_t opc, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint64_t Imm) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = 0b1'0001'11U << 22;

    Instr |= SF;
    Instr |= opc << 18;
    Instr |= (Imm & 0xFF) << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Move Wide
  void DataProcessing_MoveWide(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, uint32_t Imm, uint32_t Offset) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Imm << 5;
    Instr |= Offset << 21;
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Logical immediate
  void DataProcessing_Logical_Imm(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, uint32_t n, uint32_t immr, uint32_t imms) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= n << 22;
    Instr |= immr << 16;
    Instr |= imms << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  void xbfiz_helper(bool is_signed, ARMEmitter::Size s, Register rd, Register rn, uint32_t lsb, uint32_t width) {
    [[maybe_unused]] const auto lsb_p_width = lsb + width;
    const auto reg_size_bits = RegSizeInBits(s);

    LOGMAN_THROW_A_FMT(lsb_p_width <= reg_size_bits, "lsb + width ({}) must be <= {}. lsb={}, width={}",
                        lsb_p_width, reg_size_bits, lsb, width);
    LOGMAN_THROW_A_FMT(width >= 1, "xbfiz width must be >= 1");

    const auto immr = (reg_size_bits - lsb) & (reg_size_bits - 1);
    const auto imms = width - 1;

    if (is_signed) {
      sbfm(s, rd, rn, immr, imms);
    } else {
      ubfm(s, rd, rn, immr, imms);
    }
  }

  void DataProcessing_Extract(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, uint32_t Imm) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    // Current ARMv8 spec hardcodes SF == N for this class of instructions.
    // Anythign else is undefined behaviour.
    const uint32_t N  = s == ARMEmitter::Size::i64Bit ? (1U << 22) : 0;

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
  void DataProcessing_2Source(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rm(rm);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // Data processing - 1 source
  template<typename T>
  void DataProcessing_1Source(uint32_t Op, ARMEmitter::Size s, T rd, T rn) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rd(rd);

    dc32(Instr);
  }

  // AddSub - shifted register
  void DataProcessing_Shifted_Reg(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ShiftType Shift, uint32_t amt) {
    LOGMAN_THROW_A_FMT((amt & ~0b11'1111U) == 0, "Shift amount too large");
    if (s == ARMEmitter::Size::i32Bit) {
      LOGMAN_THROW_A_FMT(amt < 32, "Shift amount for 32-bit must be below 32");
    }

    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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
  void DataProcessing_Extended_Reg(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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
  void ConditionalCompare(uint32_t Op, uint32_t o1, uint32_t o2, uint32_t o3, ARMEmitter::Size s, ARMEmitter::Register rn, T rm, ARMEmitter::StatusFlags flags, ARMEmitter::Condition Cond) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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
  void ConditionalCompare(uint32_t Op, uint32_t o1, uint32_t o2, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, T rm, ARMEmitter::Condition Cond) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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
  void DataProcessing_3Source(uint32_t Op, uint32_t Op0, ARMEmitter::Size s, ARMEmitter::Register rd, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::Register ra) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

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

#ifndef INCLUDED_BY_EMITTER
}; // struct LoadstoreEmitterOps
} // namespace ARMEmitter
#endif
