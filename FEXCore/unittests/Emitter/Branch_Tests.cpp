// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Conditional branch immediate") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)b(Condition::CC_PL, &Label);

    CHECK(DisassembleEncoding(1) == 0x54ffffe5);
  }

  {
    ForwardLabel Label;
    (void)b(Condition::CC_PL, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x54000025);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)b(Condition::CC_PL, &Label);

    CHECK(DisassembleEncoding(1) == 0x54ffffe5);
  }

  {
    BiDirectionalLabel Label;
    (void)b(Condition::CC_PL, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x54000025);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Branch consistent conditional") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)bc(Condition::CC_PL, &Label);

    CHECK(DisassembleEncoding(1) == 0x54fffff5);
  }

  {
    ForwardLabel Label;
    (void)bc(Condition::CC_PL, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x54000035);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)bc(Condition::CC_PL, &Label);

    CHECK(DisassembleEncoding(1) == 0x54fffff5);
  }

  {
    BiDirectionalLabel Label;
    (void)bc(Condition::CC_PL, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x54000035);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Unconditional branch register") {
  TEST_SINGLE(br(Reg::r29), "br x29");
  TEST_SINGLE(blr(Reg::r29), "blr x29");
  TEST_SINGLE(ret(), "ret");
  TEST_SINGLE(ret(Reg::r29), "ret x29");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Unconditional branch immediate") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)b(&Label);

    CHECK(DisassembleEncoding(1) == 0x17ffffff);
  }

  {
    ForwardLabel Label;
    (void)b(&Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x14000001);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)b(&Label);

    CHECK(DisassembleEncoding(1) == 0x17ffffff);
  }

  {
    BiDirectionalLabel Label;
    (void)b(&Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x14000001);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)bl(&Label);

    CHECK(DisassembleEncoding(1) == 0x97ffffff);
  }

  {
    ForwardLabel Label;
    (void)bl(&Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x94000001);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)bl(&Label);

    CHECK(DisassembleEncoding(1) == 0x97ffffff);
  }

  {
    BiDirectionalLabel Label;
    (void)bl(&Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x94000001);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Compare and branch") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbz(Size::i32Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0x34fffffd);
  }

  {
    ForwardLabel Label;
    (void)cbz(Size::i32Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3400003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbz(Size::i32Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0x34fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)cbz(Size::i32Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3400003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbz(Size::i64Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0xb4fffffd);
  }

  {
    ForwardLabel Label;
    (void)cbz(Size::i64Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb400003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbz(Size::i64Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0xb4fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)cbz(Size::i64Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb400003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbnz(Size::i32Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0x35fffffd);
  }

  {
    ForwardLabel Label;
    (void)cbnz(Size::i32Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3500003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbnz(Size::i32Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0x35fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)cbnz(Size::i32Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3500003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbnz(Size::i64Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0xb5fffffd);
  }

  {
    ForwardLabel Label;
    (void)cbnz(Size::i64Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb500003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)cbnz(Size::i64Bit, Reg::r29, &Label);

    CHECK(DisassembleEncoding(1) == 0xb5fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)cbnz(Size::i64Bit, Reg::r29, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb500003d);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Branch: Test and branch immediate") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbz(Reg::r29, 0, &Label);

    CHECK(DisassembleEncoding(1) == 0x3607fffd);
  }

  {
    ForwardLabel Label;
    (void)tbz(Reg::r29, 0, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3600003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbz(Reg::r29, 0, &Label);

    CHECK(DisassembleEncoding(1) == 0x3607fffd);
  }

  {
    BiDirectionalLabel Label;
    (void)tbz(Reg::r29, 0, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3600003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbz(Reg::r29, 63, &Label);

    CHECK(DisassembleEncoding(1) == 0xb6fffffd);
  }

  {
    ForwardLabel Label;
    (void)tbz(Reg::r29, 63, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb6f8003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbz(Reg::r29, 63, &Label);

    CHECK(DisassembleEncoding(1) == 0xb6fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)tbz(Reg::r29, 63, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb6f8003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbnz(Reg::r29, 0, &Label);

    CHECK(DisassembleEncoding(1) == 0x3707fffd);
  }

  {
    ForwardLabel Label;
    (void)tbnz(Reg::r29, 0, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3700003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbnz(Reg::r29, 0, &Label);

    CHECK(DisassembleEncoding(1) == 0x3707fffd);
  }

  {
    BiDirectionalLabel Label;
    (void)tbnz(Reg::r29, 0, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x3700003d);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbnz(Reg::r29, 63, &Label);

    CHECK(DisassembleEncoding(1) == 0xb7fffffd);
  }

  {
    ForwardLabel Label;
    (void)tbnz(Reg::r29, 63, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb7f8003d);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)tbnz(Reg::r29, 63, &Label);

    CHECK(DisassembleEncoding(1) == 0xb7fffffd);
  }

  {
    BiDirectionalLabel Label;
    (void)tbnz(Reg::r29, 63, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb7f8003d);
  }
}
