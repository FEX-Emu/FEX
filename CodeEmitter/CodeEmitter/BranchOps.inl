// SPDX-License-Identifier: MIT
/* Branch instruction emitters.
 *
 * Most of these instructions will use `BackwardLabel`, `ForwardLabel`, or `BiDirectionLabel` to determine where a branch targets.
 */

#pragma once
#ifndef INCLUDED_BY_EMITTER
#include <CodeEmitter/Emitter.h>
namespace ARMEmitter {
struct EmitterOps : Emitter {
#endif

public:
  // Branches, Exception Generating and System instructions
public:
  // Conditional branch immediate
  ///< Branch conditional
  void b(ARMEmitter::Condition Cond, uint32_t Imm) {
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 0, Cond, Imm);
  }
  void b(ARMEmitter::Condition Cond, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 0, Cond, Imm >> 2);
  }
  void b(ARMEmitter::Condition Cond, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::BC});
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 0, Cond, 0);
  }

  void b(ARMEmitter::Condition Cond, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      b(Cond, &Label->Backward);
    } else {
      b(Cond, &Label->Forward);
    }
  }

  ///< Branch consistent conditional
  void bc(ARMEmitter::Condition Cond, uint32_t Imm) {
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 1, Cond, Imm);
  }
  void bc(ARMEmitter::Condition Cond, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 1, Cond, Imm >> 2);
  }

  void bc(ARMEmitter::Condition Cond, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::BC});
    constexpr uint32_t Op = 0b0101'010 << 25;
    Branch_Conditional(Op, 0, 1, Cond, 0);
  }

  void bc(ARMEmitter::Condition Cond, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      bc(Cond, &Label->Backward);
    } else {
      bc(Cond, &Label->Forward);
    }
  }

  // Unconditional branch register
  void br(ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1101011 << 25 | 0b0'000 << 21 | // opc
                            0b1'1111 << 16 |                  // op2
                            0b0000'00 << 10 |                 // op3
                            0b0'0000;                         // op4

    UnconditionalBranch(Op, rn);
  }
  void blr(ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1101011 << 25 | 0b0'001 << 21 | // opc
                            0b1'1111 << 16 |                  // op2
                            0b0000'00 << 10 |                 // op3
                            0b0'0000;                         // op4

    UnconditionalBranch(Op, rn);
  }
  void ret(ARMEmitter::Register rn = ARMEmitter::Reg::r30) {
    constexpr uint32_t Op = 0b1101011 << 25 | 0b0'010 << 21 | // opc
                            0b1'1111 << 16 |                  // op2
                            0b0000'00 << 10 |                 // op3
                            0b0'0000;                         // op4

    UnconditionalBranch(Op, rn);
  }

  // Unconditional branch immediate
  void b(uint32_t Imm) {
    constexpr uint32_t Op = 0b0001'01 << 26;

    UnconditionalBranch(Op, Imm);
  }
  void b(const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -134217728 && Imm <= 134217724 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'01 << 26;

    UnconditionalBranch(Op, Imm >> 2);
  }
  void b(ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::B});
    constexpr uint32_t Op = 0b0001'01 << 26;

    UnconditionalBranch(Op, 0);
  }

  void b(BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      b(&Label->Backward);
    } else {
      b(&Label->Forward);
    }
  }

  void bl(uint32_t Imm) {
    constexpr uint32_t Op = 0b1001'01 << 26;

    UnconditionalBranch(Op, Imm);
  }

  void bl(const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -134217728 && Imm <= 134217724 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'01 << 26;

    UnconditionalBranch(Op, Imm >> 2);
  }
  void bl(ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::B});
    constexpr uint32_t Op = 0b1001'01 << 26;

    UnconditionalBranch(Op, 0);
  }

  void bl(BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      bl(&Label->Backward);
    } else {
      bl(&Label->Forward);
    }
  }

  // Compare and branch
  void cbz(ARMEmitter::Size s, ARMEmitter::Register rt, uint32_t Imm) {
    constexpr uint32_t Op = 0b0011'0100 << 24;

    CompareAndBranch(Op, s, rt, Imm);
  }

  void cbz(ARMEmitter::Size s, ARMEmitter::Register rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'0100 << 24;

    CompareAndBranch(Op, s, rt, Imm >> 2);
  }

  void cbz(ARMEmitter::Size s, ARMEmitter::Register rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::BC});

    constexpr uint32_t Op = 0b0011'0100 << 24;

    CompareAndBranch(Op, s, rt, 0);
  }

  void cbz(ARMEmitter::Size s, ARMEmitter::Register rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      cbz(s, rt, &Label->Backward);
    } else {
      cbz(s, rt, &Label->Forward);
    }
  }

  void cbnz(ARMEmitter::Size s, ARMEmitter::Register rt, uint32_t Imm) {
    constexpr uint32_t Op = 0b0011'0101 << 24;

    CompareAndBranch(Op, s, rt, Imm);
  }

  void cbnz(ARMEmitter::Size s, ARMEmitter::Register rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'0101 << 24;

    CompareAndBranch(Op, s, rt, Imm >> 2);
  }

  void cbnz(ARMEmitter::Size s, ARMEmitter::Register rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::BC});

    constexpr uint32_t Op = 0b0011'0101 << 24;

    CompareAndBranch(Op, s, rt, 0);
  }

  void cbnz(ARMEmitter::Size s, ARMEmitter::Register rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      cbnz(s, rt, &Label->Backward);
    } else {
      cbnz(s, rt, &Label->Forward);
    }
  }

  // Test and branch immediate
  void tbz(ARMEmitter::Register rt, uint32_t Bit, uint32_t Imm) {
    constexpr uint32_t Op = 0b0011'0110 << 24;

    TestAndBranch(Op, rt, Bit, Imm);
  }
  void tbz(ARMEmitter::Register rt, uint32_t Bit, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -32768 && Imm <= 32764 && ((Imm & 0b11) == 0), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'0110 << 24;

    TestAndBranch(Op, rt, Bit, Imm >> 2);
  }

  void tbz(ARMEmitter::Register rt, uint32_t Bit, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::TEST_BRANCH});

    constexpr uint32_t Op = 0b0011'0110 << 24;

    TestAndBranch(Op, rt, Bit, 0);
  }

  void tbz(ARMEmitter::Register rt, uint32_t Bit, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      tbz(rt, Bit, &Label->Backward);
    } else {
      tbz(rt, Bit, &Label->Forward);
    }
  }

  void tbnz(ARMEmitter::Register rt, uint32_t Bit, uint32_t Imm) {
    constexpr uint32_t Op = 0b0011'0111 << 24;

    TestAndBranch(Op, rt, Bit, Imm);
  }
  void tbnz(ARMEmitter::Register rt, uint32_t Bit, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -32768 && Imm <= 32764 && ((Imm & 0b11) == 0), "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'0111 << 24;

    TestAndBranch(Op, rt, Bit, Imm >> 2);
  }

  void tbnz(ARMEmitter::Register rt, uint32_t Bit, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::TEST_BRANCH});
    constexpr uint32_t Op = 0b0011'0111 << 24;

    TestAndBranch(Op, rt, Bit, 0);
  }

  void tbnz(ARMEmitter::Register rt, uint32_t Bit, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      tbnz(rt, Bit, &Label->Backward);
    } else {
      tbnz(rt, Bit, &Label->Forward);
    }
  }

private:
  // Conditional branch immediate
  void Branch_Conditional(uint32_t Op, uint32_t Op1, uint32_t Op0, ARMEmitter::Condition Cond, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Op1 << 24;
    Instr |= (Imm & 0x7'FFFF) << 5;
    Instr |= Op0 << 4;
    Instr |= FEXCore::ToUnderlying(Cond);

    dc32(Instr);
  }

  // Unconditional branch register
  void UnconditionalBranch(uint32_t Op, ARMEmitter::Register rn) {
    uint32_t Instr = Op;
    Instr |= Encode_rn(rn);
    dc32(Instr);
  }

  // Unconditional branch - immediate
  void UnconditionalBranch(uint32_t Op, uint32_t Imm) {
    uint32_t Instr = Op;
    Instr |= Imm & 0x3FF'FFFF;
    dc32(Instr);
  }

  // Compare and branch
  void CompareAndBranch(uint32_t Op, ARMEmitter::Size s, ARMEmitter::Register rt, uint32_t Imm) {
    const uint32_t SF = s == ARMEmitter::Size::i64Bit ? (1U << 31) : 0;

    uint32_t Instr = Op;

    Instr |= SF;
    Instr |= (Imm & 0x7'FFFF) << 5;
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  // Test and branch - immediate
  void TestAndBranch(uint32_t Op, ARMEmitter::Register rt, uint32_t Bit, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= (Bit >> 5) << 31;
    Instr |= (Bit & 0b1'1111) << 19;
    Instr |= (Imm & 0x3FFF) << 5;
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

#ifndef INCLUDED_BY_EMITTER
}; // struct LoadstoreEmitterOps
} // namespace ARMEmitter
#endif
