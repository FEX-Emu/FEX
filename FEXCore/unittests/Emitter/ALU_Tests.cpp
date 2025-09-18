// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: PC relative") {
  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)adr(Reg::r30, &Label);

    CHECK(DisassembleEncoding(1) == 0x10fffffe);
  }

  {
    ForwardLabel Label;
    (void)adr(Reg::r30, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x1000003e);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)adr(Reg::r30, &Label);

    CHECK(DisassembleEncoding(1) == 0x10fffffe);
  }

  {
    BiDirectionalLabel Label;
    (void)adr(Reg::r30, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x1000003e);
  }

  {
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)adrp(Reg::r30, &Label);

    CHECK(DisassembleEncoding(1) == 0x9000001e);
  }

  {
    ForwardLabel Label;
    (void)adrp(Reg::r30, &Label);
    // Move label a page away
    for (size_t i = 0; i < 1023; ++i) {
      nop();
    }
    (void)Bind(&Label);

    CHECK(DisassembleEncoding(0) == 0xb000001e);
  }

  {
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)adrp(Reg::r30, &Label);

    CHECK(DisassembleEncoding(1) == 0x9000001e);
  }

  {
    BiDirectionalLabel Label;
    (void)adrp(Reg::r30, &Label);
    // Move label a page away
    for (size_t i = 0; i < 1023; ++i) {
      nop();
    }
    (void)Bind(&Label);

    CHECK(DisassembleEncoding(0) == 0xb000001e);
  }

  {
    // Will generate adr.
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);

    (void)LongAddressGen(Reg::r30, &Label);
    CHECK(DisassembleEncoding(1) == 0x10fffffe);
  }
  {
    // Will generate nop + adr.
    ForwardLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xd503201f);
    CHECK(DisassembleEncoding(1) == 0x1000003e);
  }
  {
    // Will generate adr.
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);
    (void)LongAddressGen(Reg::r30, &Label);

    CHECK(DisassembleEncoding(1) == 0x10fffffe);
  }

  {
    // Will generate nop + adr.
    BiDirectionalLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);
    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xd503201f);
    CHECK(DisassembleEncoding(1) == 0x1000003e);
  }

  {
    // Will generate adrp.
    BackwardLabel Label;
    (void)Bind(&Label);
    dc32(0);

    // Move adrp 1MB away.
    for (size_t i = 0; i < (1 * 1024 * 1024 / 4); ++i) {
      nop();
    }

    (void)LongAddressGen(Reg::r30, &Label);
    nop();
    CHECK(DisassembleEncoding(262145) == 0x90fff81e);
    CHECK(DisassembleEncoding(262146) == 0xd503201f);
  }

  {
    // Will generate nop + adrp.
    ForwardLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);

    // Move label 1MB away, plus a page, and then aligned to a page.
    for (size_t i = 0; i < ((1 * 1024 * 1024 + 4096) / 4 - 2); ++i) {
      nop();
    }

    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xd503201f);
    CHECK(DisassembleEncoding(1) == 0x9000081e);
  }

  {
    // Will generate adrp + add.
    ForwardLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);

    // Move label 1MB away, plus a page, plus one instruction.
    for (size_t i = 0; i < ((1 * 1024 * 1024 + 4096) / 4 - 1); ++i) {
      nop();
    }

    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb000081e);
    CHECK(DisassembleEncoding(1) == 0x910013de);
  }


  {
    // Will generate adrp.
    BiDirectionalLabel Label;
    (void)Bind(&Label);
    dc32(0);

    // Move adrp 1MB away.
    for (size_t i = 0; i < (1 * 1024 * 1024 / 4); ++i) {
      nop();
    }

    (void)LongAddressGen(Reg::r30, &Label);
    nop();
    CHECK(DisassembleEncoding(262145) == 0x90fff81e);
    CHECK(DisassembleEncoding(262146) == 0xd503201f);
  }

  {
    // Will generate nop + adrp.
    BiDirectionalLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);

    // Move label 1MB away, plus a page, and then aligned to a page.
    for (size_t i = 0; i < ((1 * 1024 * 1024 + 4096) / 4 - 2); ++i) {
      nop();
    }

    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xd503201f);
    CHECK(DisassembleEncoding(1) == 0x9000081e);
  }

  {
    // Will generate adrp + add.
    BiDirectionalLabel Label;
    (void)LongAddressGen(Reg::r30, &Label);

    // Move label 1MB away, plus a page, plus one instruction.
    for (size_t i = 0; i < ((1 * 1024 * 1024 + 4096) / 4 - 1); ++i) {
      nop();
    }

    (void)Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xb000081e);
    CHECK(DisassembleEncoding(1) == 0x910013de);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Add/subtract immediate") {
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, 0, false), "add w29, w28, #0x0 (0)");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, 4095, false), "add w29, w28, #0xfff (4095)");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, 0, true), "add w29, w28, #0x0 (0)");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, 4095, true), "add w29, w28, #0xfff000 (16773120)");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, 16773120), "add w29, w28, #0xfff000 (16773120)");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, 0, false), "add x29, x28, #0x0 (0)");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, 4095, false), "add x29, x28, #0xfff (4095)");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, 0, true), "add x29, x28, #0x0 (0)");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, 4095, true), "add x29, x28, #0xfff000 (16773120)");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, 16773120), "add x29, x28, #0xfff000 (16773120)");

  TEST_SINGLE(add(Size::i64Bit, Reg::rsp, Reg::rsp, 0, false), "mov sp, sp");
  TEST_SINGLE(add(Size::i64Bit, Reg::rsp, Reg::rsp, 4095, false), "add sp, sp, #0xfff (4095)");
  TEST_SINGLE(add(Size::i64Bit, Reg::rsp, Reg::rsp, 0, true), "mov sp, sp");
  TEST_SINGLE(add(Size::i64Bit, Reg::rsp, Reg::rsp, 4095, true), "add sp, sp, #0xfff000 (16773120)");
  TEST_SINGLE(add(Size::i64Bit, Reg::rsp, Reg::rsp, 16773120), "add sp, sp, #0xfff000 (16773120)");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, 0, false), "adds w29, w28, #0x0 (0)");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, 4095, false), "adds w29, w28, #0xfff (4095)");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, 0, true), "adds w29, w28, #0x0 (0)");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, 4095, true), "adds w29, w28, #0xfff000 (16773120)");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, 16773120), "adds w29, w28, #0xfff000 (16773120)");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, 0, false), "adds x29, x28, #0x0 (0)");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, 4095, false), "adds x29, x28, #0xfff (4095)");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, 0, true), "adds x29, x28, #0x0 (0)");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, 4095, true), "adds x29, x28, #0xfff000 (16773120)");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, 16773120), "adds x29, x28, #0xfff000 (16773120)");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, 0, false), "cmn w28, #0x0 (0)");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, 4095, false), "cmn w28, #0xfff (4095)");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, 0, true), "cmn w28, #0x0 (0)");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, 4095, true), "cmn w28, #0xfff000 (16773120)");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, 16773120), "cmn w28, #0xfff000 (16773120)");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, 0, false), "cmn x28, #0x0 (0)");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, 4095, false), "cmn x28, #0xfff (4095)");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, 0, true), "cmn x28, #0x0 (0)");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, 4095, true), "cmn x28, #0xfff000 (16773120)");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, 16773120), "cmn x28, #0xfff000 (16773120)");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, 0, false), "sub w29, w28, #0x0 (0)");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, 4095, false), "sub w29, w28, #0xfff (4095)");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, 0, true), "sub w29, w28, #0x0 (0)");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, 4095, true), "sub w29, w28, #0xfff000 (16773120)");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, 16773120), "sub w29, w28, #0xfff000 (16773120)");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, 0, false), "sub x29, x28, #0x0 (0)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, 4095, false), "sub x29, x28, #0xfff (4095)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, 0, true), "sub x29, x28, #0x0 (0)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, 4095, true), "sub x29, x28, #0xfff000 (16773120)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, 16773120), "sub x29, x28, #0xfff000 (16773120)");

  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, 0, false), "sub sp, sp, #0x0 (0)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, 4095, false), "sub sp, sp, #0xfff (4095)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, 0, true), "sub sp, sp, #0x0 (0)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, 4095, true), "sub sp, sp, #0xfff000 (16773120)");
  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, 16773120), "sub sp, sp, #0xfff000 (16773120)");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, 0, false), "subs w29, w28, #0x0 (0)");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, 4095, false), "subs w29, w28, #0xfff (4095)");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, 0, true), "subs w29, w28, #0x0 (0)");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, 4095, true), "subs w29, w28, #0xfff000 (16773120)");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, 16773120), "subs w29, w28, #0xfff000 (16773120)");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, 0, false), "subs x29, x28, #0x0 (0)");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, 4095, false), "subs x29, x28, #0xfff (4095)");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, 0, true), "subs x29, x28, #0x0 (0)");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, 4095, true), "subs x29, x28, #0xfff000 (16773120)");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, 16773120), "subs x29, x28, #0xfff000 (16773120)");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r28, 0, false), "cmp w28, #0x0 (0)");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r28, 4095, false), "cmp w28, #0xfff (4095)");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r28, 0, true), "cmp w28, #0x0 (0)");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r28, 4095, true), "cmp w28, #0xfff000 (16773120)");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r28, 16773120), "cmp w28, #0xfff000 (16773120)");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r28, 0, false), "cmp x28, #0x0 (0)");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r28, 4095, false), "cmp x28, #0xfff (4095)");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r28, 0, true), "cmp x28, #0x0 (0)");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r28, 4095, true), "cmp x28, #0xfff000 (16773120)");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r28, 16773120), "cmp x28, #0xfff000 (16773120)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Min/max immediate") {
  TEST_SINGLE(smax(Size::i32Bit, Reg::r29, Reg::r28, 1), "smax w29, w28, #1");
  TEST_SINGLE(smax(Size::i32Bit, Reg::r29, Reg::r28, 127), "smax w29, w28, #127");
  TEST_SINGLE(smax(Size::i32Bit, Reg::r29, Reg::r28, -128), "smax w29, w28, #-128");
  TEST_SINGLE(smax(Size::i64Bit, Reg::r29, Reg::r28, 1), "smax x29, x28, #1");
  TEST_SINGLE(smax(Size::i64Bit, Reg::r29, Reg::r28, 127), "smax x29, x28, #127");
  TEST_SINGLE(smax(Size::i64Bit, Reg::r29, Reg::r28, -128), "smax x29, x28, #-128");

  TEST_SINGLE(umax(Size::i32Bit, Reg::r29, Reg::r28, 0), "umax w29, w28, #0");
  TEST_SINGLE(umax(Size::i32Bit, Reg::r29, Reg::r28, 255), "umax w29, w28, #255");
  TEST_SINGLE(umax(Size::i64Bit, Reg::r29, Reg::r28, 0), "umax x29, x28, #0");
  TEST_SINGLE(umax(Size::i64Bit, Reg::r29, Reg::r28, 255), "umax x29, x28, #255");

  TEST_SINGLE(smin(Size::i32Bit, Reg::r29, Reg::r28, 1), "smin w29, w28, #1");
  TEST_SINGLE(smin(Size::i32Bit, Reg::r29, Reg::r28, 127), "smin w29, w28, #127");
  TEST_SINGLE(smin(Size::i32Bit, Reg::r29, Reg::r28, -128), "smin w29, w28, #-128");
  TEST_SINGLE(smin(Size::i64Bit, Reg::r29, Reg::r28, 1), "smin x29, x28, #1");
  TEST_SINGLE(smin(Size::i64Bit, Reg::r29, Reg::r28, 127), "smin x29, x28, #127");
  TEST_SINGLE(smin(Size::i64Bit, Reg::r29, Reg::r28, -128), "smin x29, x28, #-128");

  TEST_SINGLE(umin(Size::i32Bit, Reg::r29, Reg::r28, 0), "umin w29, w28, #0");
  TEST_SINGLE(umin(Size::i32Bit, Reg::r29, Reg::r28, 255), "umin w29, w28, #255");
  TEST_SINGLE(umin(Size::i64Bit, Reg::r29, Reg::r28, 0), "umin x29, x28, #0");
  TEST_SINGLE(umin(Size::i64Bit, Reg::r29, Reg::r28, 255), "umin x29, x28, #255");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Logical immediate") {
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, 1), "and w29, w28, #0x1");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, -2), "and w29, w28, #0xfffffffe");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, 1), "and x29, x28, #0x1");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, -2), "and x29, x28, #0xfffffffffffffffe");

  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, 1), "and w29, w28, #0xfffffffe");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, -2), "and w29, w28, #0x1");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, 1), "and x29, x28, #0xfffffffffffffffe");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, -2), "and x29, x28, #0x1");

  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, 1), "ands w29, w28, #0x1");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, -2), "ands w29, w28, #0xfffffffe");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, 1), "ands x29, x28, #0x1");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, -2), "ands x29, x28, #0xfffffffffffffffe");

  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, 1), "ands w29, w28, #0xfffffffe");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, -2), "ands w29, w28, #0x1");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, 1), "ands x29, x28, #0xfffffffffffffffe");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, -2), "ands x29, x28, #0x1");

  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, 1), "orr w29, w28, #0x1");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, -2), "orr w29, w28, #0xfffffffe");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, 1), "orr x29, x28, #0x1");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, -2), "orr x29, x28, #0xfffffffffffffffe");

  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, 1), "eor w29, w28, #0x1");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, -2), "eor w29, w28, #0xfffffffe");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, 1), "eor x29, x28, #0x1");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, -2), "eor x29, x28, #0xfffffffffffffffe");

  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, 1), "tst w28, #0x1");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, -2), "tst w28, #0xfffffffe");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, 1), "tst x28, #0x1");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, -2), "tst x28, #0xfffffffffffffffe");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Move wide immediate") {
  TEST_SINGLE(movn(Size::i32Bit, Reg::r29, 0x4243, 0), "mov w29, #0xffffbdbc");
  TEST_SINGLE(movn(Size::i32Bit, Reg::r29, 0x4243, 16), "mov w29, #0xbdbcffff");

  TEST_SINGLE(movn(Size::i64Bit, Reg::r29, 0x4243, 0), "mov x29, #0xffffffffffffbdbc");
  TEST_SINGLE(movn(Size::i64Bit, Reg::r29, 0x4243, 16), "mov x29, #0xffffffffbdbcffff");
  TEST_SINGLE(movn(Size::i64Bit, Reg::r29, 0x4243, 32), "mov x29, #0xffffbdbcffffffff");
  TEST_SINGLE(movn(Size::i64Bit, Reg::r29, 0x4243, 48), "mov x29, #0xbdbcffffffffffff");

  TEST_SINGLE(mov(Size::i32Bit, Reg::r29, 0x4243), "mov w29, #0x4243");
  TEST_SINGLE(mov(Size::i64Bit, Reg::r29, 0x4243), "mov x29, #0x4243");

  TEST_SINGLE(mov(WReg::w29, 0x4243), "mov w29, #0x4243");
  TEST_SINGLE(mov(XReg::x29, 0x4243), "mov x29, #0x4243");

  TEST_SINGLE(movz(Size::i32Bit, Reg::r29, 0x4243, 0), "mov w29, #0x4243");
  TEST_SINGLE(movz(Size::i32Bit, Reg::r29, 0x4243, 16), "mov w29, #0x42430000");

  TEST_SINGLE(movz(Size::i64Bit, Reg::r29, 0x4243, 0), "mov x29, #0x4243");
  TEST_SINGLE(movz(Size::i64Bit, Reg::r29, 0x4243, 16), "mov x29, #0x42430000");
  TEST_SINGLE(movz(Size::i64Bit, Reg::r29, 0x4243, 32), "mov x29, #0x424300000000");
  TEST_SINGLE(movz(Size::i64Bit, Reg::r29, 0x4243, 48), "mov x29, #0x4243000000000000");

  TEST_SINGLE(movk(Size::i32Bit, Reg::r29, 0x4243, 0), "movk w29, #0x4243");
  TEST_SINGLE(movk(Size::i32Bit, Reg::r29, 0x4243, 16), "movk w29, #0x4243, lsl #16");

  TEST_SINGLE(movk(Size::i64Bit, Reg::r29, 0x4243, 0), "movk x29, #0x4243");
  TEST_SINGLE(movk(Size::i64Bit, Reg::r29, 0x4243, 16), "movk x29, #0x4243, lsl #16");
  TEST_SINGLE(movk(Size::i64Bit, Reg::r29, 0x4243, 32), "movk x29, #0x4243, lsl #32");
  TEST_SINGLE(movk(Size::i64Bit, Reg::r29, 0x4243, 48), "movk x29, #0x4243, lsl #48");

  TEST_SINGLE(movn(WReg::w29, 0x4243, 0), "mov w29, #0xffffbdbc");
  TEST_SINGLE(movn(WReg::w29, 0x4243, 16), "mov w29, #0xbdbcffff");
  TEST_SINGLE(movz(WReg::w29, 0x4243, 0), "mov w29, #0x4243");
  TEST_SINGLE(movz(WReg::w29, 0x4243, 16), "mov w29, #0x42430000");
  TEST_SINGLE(movk(WReg::w29, 0x4243, 0), "movk w29, #0x4243");
  TEST_SINGLE(movk(WReg::w29, 0x4243, 16), "movk w29, #0x4243, lsl #16");

  TEST_SINGLE(movn(XReg::x29, 0x4243, 0), "mov x29, #0xffffffffffffbdbc");
  TEST_SINGLE(movn(XReg::x29, 0x4243, 16), "mov x29, #0xffffffffbdbcffff");
  TEST_SINGLE(movn(XReg::x29, 0x4243, 32), "mov x29, #0xffffbdbcffffffff");
  TEST_SINGLE(movn(XReg::x29, 0x4243, 48), "mov x29, #0xbdbcffffffffffff");
  TEST_SINGLE(movz(XReg::x29, 0x4243, 0), "mov x29, #0x4243");
  TEST_SINGLE(movz(XReg::x29, 0x4243, 16), "mov x29, #0x42430000");
  TEST_SINGLE(movz(XReg::x29, 0x4243, 32), "mov x29, #0x424300000000");
  TEST_SINGLE(movz(XReg::x29, 0x4243, 48), "mov x29, #0x4243000000000000");
  TEST_SINGLE(movk(XReg::x29, 0x4243, 0), "movk x29, #0x4243");
  TEST_SINGLE(movk(XReg::x29, 0x4243, 16), "movk x29, #0x4243, lsl #16");
  TEST_SINGLE(movk(XReg::x29, 0x4243, 32), "movk x29, #0x4243, lsl #32");
  TEST_SINGLE(movk(XReg::x29, 0x4243, 48), "movk x29, #0x4243, lsl #48");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Bitfield") {
  TEST_SINGLE(sxtb(Size::i32Bit, Reg::r29, Reg::r28), "sxtb w29, w28");
  TEST_SINGLE(sxtb(Size::i64Bit, Reg::r29, Reg::r28), "sxtb x29, w28");

  TEST_SINGLE(sxth(Size::i32Bit, Reg::r29, Reg::r28), "sxth w29, w28");
  TEST_SINGLE(sxth(Size::i64Bit, Reg::r29, Reg::r28), "sxth x29, w28");

  TEST_SINGLE(sxtw(XReg::x29, WReg::w28), "sxtw x29, w28");

  TEST_SINGLE(sbfx(Size::i32Bit, Reg::r29, Reg::r28, 4, 16), "sbfx w29, w28, #4, #16");
  TEST_SINGLE(sbfx(Size::i64Bit, Reg::r29, Reg::r28, 4, 16), "sbfx x29, x28, #4, #16");

  TEST_SINGLE(asr(Size::i32Bit, Reg::r29, Reg::r28, 17), "asr w29, w28, #17");
  TEST_SINGLE(asr(Size::i64Bit, Reg::r29, Reg::r28, 17), "asr x29, x28, #17");

  TEST_SINGLE(bfc(Size::i32Bit, Reg::r29, 4, 3), "bfc w29, #4, #3");
  TEST_SINGLE(bfc(Size::i32Bit, Reg::r29, 27, 3), "bfc w29, #27, #3");

  TEST_SINGLE(bfc(Size::i64Bit, Reg::r29, 4, 3), "bfc x29, #4, #3");
  TEST_SINGLE(bfc(Size::i64Bit, Reg::r29, 57, 3), "bfc x29, #57, #3");

  TEST_SINGLE(bfxil(Size::i32Bit, Reg::r29, Reg::r28, 4, 3), "bfxil w29, w28, #4, #3");
  TEST_SINGLE(bfxil(Size::i32Bit, Reg::r29, Reg::r28, 27, 3), "bfxil w29, w28, #27, #3");

  TEST_SINGLE(bfxil(Size::i64Bit, Reg::r29, Reg::r28, 4, 3), "bfxil x29, x28, #4, #3");
  TEST_SINGLE(bfxil(Size::i64Bit, Reg::r29, Reg::r28, 57, 3), "bfxil x29, x28, #57, #3");

  TEST_SINGLE(sbfiz(Size::i32Bit, Reg::r29, Reg::r28, 5, 3), "sbfiz w29, w28, #5, #3");
  TEST_SINGLE(sbfiz(Size::i32Bit, Reg::r29, Reg::r28, 27, 3), "sbfiz w29, w28, #27, #3");

  TEST_SINGLE(sbfiz(Size::i64Bit, Reg::r29, Reg::r28, 5, 3), "sbfiz x29, x28, #5, #3");
  TEST_SINGLE(sbfiz(Size::i64Bit, Reg::r29, Reg::r28, 54, 3), "sbfiz x29, x28, #54, #3");

  TEST_SINGLE(ubfiz(Size::i32Bit, Reg::r29, Reg::r28, 5, 3), "ubfiz w29, w28, #5, #3");
  TEST_SINGLE(ubfiz(Size::i32Bit, Reg::r29, Reg::r28, 27, 3), "ubfiz w29, w28, #27, #3");

  TEST_SINGLE(ubfiz(Size::i64Bit, Reg::r29, Reg::r28, 5, 3), "ubfiz x29, x28, #5, #3");
  TEST_SINGLE(ubfiz(Size::i64Bit, Reg::r29, Reg::r28, 54, 3), "ubfiz x29, x28, #54, #3");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Extract") {
  TEST_SINGLE(extr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, 0), "extr w29, w28, w27, #0");
  TEST_SINGLE(extr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, 16), "extr w29, w28, w27, #16");

  TEST_SINGLE(extr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, 0), "extr x29, x28, x27, #0");
  TEST_SINGLE(extr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, 16), "extr x29, x28, x27, #16");
  TEST_SINGLE(extr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, 32), "extr x29, x28, x27, #32");
  TEST_SINGLE(extr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, 48), "extr x29, x28, x27, #48");

  TEST_SINGLE(ror(Size::i32Bit, Reg::r29, Reg::r28, 0), "ror w29, w28, #0");
  TEST_SINGLE(ror(Size::i32Bit, Reg::r29, Reg::r28, 16), "ror w29, w28, #16");

  TEST_SINGLE(ror(Size::i64Bit, Reg::r29, Reg::r28, 0), "ror x29, x28, #0");
  TEST_SINGLE(ror(Size::i64Bit, Reg::r29, Reg::r28, 16), "ror x29, x28, #16");
  TEST_SINGLE(ror(Size::i64Bit, Reg::r29, Reg::r28, 32), "ror x29, x28, #32");
  TEST_SINGLE(ror(Size::i64Bit, Reg::r29, Reg::r28, 48), "ror x29, x28, #48");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Data processing - 2 source") {
  TEST_SINGLE(udiv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "udiv w29, w28, w27");
  TEST_SINGLE(sdiv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "sdiv w29, w28, w27");
  TEST_SINGLE(lslv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "lsl w29, w28, w27");
  TEST_SINGLE(lsrv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "lsr w29, w28, w27");
  TEST_SINGLE(asrv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "asr w29, w28, w27");
  TEST_SINGLE(rorv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "ror w29, w28, w27");
  TEST_SINGLE(crc32b(WReg::w29, WReg::w28, WReg::w27), "crc32b w29, w28, w27");
  TEST_SINGLE(crc32h(WReg::w29, WReg::w28, WReg::w27), "crc32h w29, w28, w27");
  TEST_SINGLE(crc32w(WReg::w29, WReg::w28, WReg::w27), "crc32w w29, w28, w27");
  TEST_SINGLE(crc32cb(WReg::w29, WReg::w28, WReg::w27), "crc32cb w29, w28, w27");
  TEST_SINGLE(crc32ch(WReg::w29, WReg::w28, WReg::w27), "crc32ch w29, w28, w27");
  TEST_SINGLE(crc32cw(WReg::w29, WReg::w28, WReg::w27), "crc32cw w29, w28, w27");
  TEST_SINGLE(smax(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "smax w29, w28, w27");
  TEST_SINGLE(umax(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "umax w29, w28, w27");
  TEST_SINGLE(smin(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "smin w29, w28, w27");
  TEST_SINGLE(umin(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "umin w29, w28, w27");

  TEST_SINGLE(udiv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "udiv x29, x28, x27");
  TEST_SINGLE(sdiv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "sdiv x29, x28, x27");
  TEST_SINGLE(lslv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "lsl x29, x28, x27");
  TEST_SINGLE(lsrv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "lsr x29, x28, x27");
  TEST_SINGLE(asrv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "asr x29, x28, x27");
  TEST_SINGLE(rorv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "ror x29, x28, x27");
  TEST_SINGLE(smax(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "smax x29, x28, x27");
  TEST_SINGLE(umax(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "umax x29, x28, x27");
  TEST_SINGLE(smin(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "smin x29, x28, x27");
  TEST_SINGLE(umin(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "umin x29, x28, x27");

  if (false) {
    // vixl doesn't support this instruction.
    TEST_SINGLE(subp(XReg::x29, XReg::x28, XReg::x27), "subp x29, x28, x27");
    TEST_SINGLE(irg(XReg::x29, XReg::x28, XReg::x27), "irg x29, x28, x27");
    TEST_SINGLE(gmi(XReg::x29, XReg::x28, XReg::x27), "gmi x29, x28, x27");
  }

  TEST_SINGLE(pacga(XReg::x29, XReg::x28, XReg::x27), "pacga x29, x28, x27");
  TEST_SINGLE(crc32x(XReg::x29, XReg::x28, XReg::x27), "crc32x w29, w28, x27");
  TEST_SINGLE(crc32cx(XReg::x29, XReg::x28, XReg::x27), "crc32cx w29, w28, x27");

  if (false) {
    // vixl doesn't support this instruction.
    TEST_SINGLE(subps(XReg::x29, XReg::x28, XReg::x27), "subps x29, x28, x27");
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Data processing - 1 source") {
  TEST_SINGLE(rbit(Size::i32Bit, Reg::r29, Reg::r28), "rbit w29, w28");
  TEST_SINGLE(rbit(Size::i64Bit, Reg::r29, Reg::r28), "rbit x29, x28");

  TEST_SINGLE(rev16(Size::i32Bit, Reg::r29, Reg::r28), "rev16 w29, w28");
  TEST_SINGLE(rev16(Size::i64Bit, Reg::r29, Reg::r28), "rev16 x29, x28");

  TEST_SINGLE(rev(WReg::w29, WReg::w28), "rev w29, w28");
  TEST_SINGLE(rev32(XReg::x29, XReg::x28), "rev32 x29, x28");

  TEST_SINGLE(clz(Size::i32Bit, Reg::r29, Reg::r28), "clz w29, w28");
  TEST_SINGLE(clz(Size::i64Bit, Reg::r29, Reg::r28), "clz x29, x28");

  TEST_SINGLE(cls(Size::i32Bit, Reg::r29, Reg::r28), "cls w29, w28");
  TEST_SINGLE(cls(Size::i64Bit, Reg::r29, Reg::r28), "cls x29, x28");

  TEST_SINGLE(rev(XReg::x29, XReg::x28), "rev x29, x28");
  TEST_SINGLE(rev(Size::i32Bit, Reg::r29, Reg::r28), "rev w29, w28");
  TEST_SINGLE(rev(Size::i64Bit, Reg::r29, Reg::r28), "rev x29, x28");

  TEST_SINGLE(ctz(Size::i32Bit, Reg::r29, Reg::r28), "ctz w29, w28");
  TEST_SINGLE(ctz(Size::i64Bit, Reg::r29, Reg::r28), "ctz x29, x28");

  TEST_SINGLE(cnt(Size::i32Bit, Reg::r29, Reg::r28), "cnt w29, w28");
  TEST_SINGLE(cnt(Size::i64Bit, Reg::r29, Reg::r28), "cnt x29, x28");

  TEST_SINGLE(abs(Size::i32Bit, Reg::r29, Reg::r28), "abs w29, w28");
  TEST_SINGLE(abs(Size::i64Bit, Reg::r29, Reg::r28), "abs x29, x28");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: PAUTH") {
  // TODO: Implement in the emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Logical - shifted register") {
  TEST_SINGLE(mov(Size::i32Bit, Reg::r29, Reg::r28), "mov w29, w28");
  TEST_SINGLE(mov(Size::i64Bit, Reg::r29, Reg::r28), "mov x29, x28");

  TEST_SINGLE(mov(WReg::w29, WReg::w28), "mov w29, w28");
  TEST_SINGLE(mov(XReg::x29, XReg::x28), "mov x29, x28");

  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSL, 0), "mvn w29, w28");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSL, 1), "mvn w29, w28, lsl #1");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSL, 31), "mvn w29, w28, lsl #31");

  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSR, 0), "mvn w29, w28");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSR, 1), "mvn w29, w28, lsr #1");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSR, 31), "mvn w29, w28, lsr #31");

  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ASR, 0), "mvn w29, w28");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ASR, 1), "mvn w29, w28, asr #1");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ASR, 31), "mvn w29, w28, asr #31");

  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ROR, 0), "mvn w29, w28");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ROR, 1), "mvn w29, w28, ror #1");
  TEST_SINGLE(mvn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ROR, 31), "mvn w29, w28, ror #31");

  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSL, 0), "mvn x29, x28");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSL, 1), "mvn x29, x28, lsl #1");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSL, 63), "mvn x29, x28, lsl #63");

  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSR, 0), "mvn x29, x28");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSR, 1), "mvn x29, x28, lsr #1");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSR, 63), "mvn x29, x28, lsr #63");

  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ASR, 0), "mvn x29, x28");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ASR, 1), "mvn x29, x28, asr #1");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ASR, 63), "mvn x29, x28, asr #63");

  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ROR, 0), "mvn x29, x28");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ROR, 1), "mvn x29, x28, ror #1");
  TEST_SINGLE(mvn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ROR, 63), "mvn x29, x28, ror #63");

  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "and w29, w28, w27");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "and w29, w28, w27, lsl #1");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "and w29, w28, w27, lsl #31");

  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "and w29, w28, w27");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "and w29, w28, w27, lsr #1");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "and w29, w28, w27, lsr #31");

  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "and w29, w28, w27");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "and w29, w28, w27, asr #1");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "and w29, w28, w27, asr #31");

  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "and w29, w28, w27");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "and w29, w28, w27, ror #1");
  TEST_SINGLE(and_(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "and w29, w28, w27, ror #31");

  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "and x29, x28, x27");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "and x29, x28, x27, lsl #1");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "and x29, x28, x27, lsl #63");

  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "and x29, x28, x27");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "and x29, x28, x27, lsr #1");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "and x29, x28, x27, lsr #63");

  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "and x29, x28, x27");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "and x29, x28, x27, asr #1");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "and x29, x28, x27, asr #63");

  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "and x29, x28, x27");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "and x29, x28, x27, ror #1");
  TEST_SINGLE(and_(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "and x29, x28, x27, ror #63");

  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "ands w29, w28, w27");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "ands w29, w28, w27, lsl #1");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "ands w29, w28, w27, lsl #31");

  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "ands w29, w28, w27");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "ands w29, w28, w27, lsr #1");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "ands w29, w28, w27, lsr #31");

  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "ands w29, w28, w27");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "ands w29, w28, w27, asr #1");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "ands w29, w28, w27, asr #31");

  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "ands w29, w28, w27");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "ands w29, w28, w27, ror #1");
  TEST_SINGLE(ands(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "ands w29, w28, w27, ror #31");

  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "ands x29, x28, x27");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "ands x29, x28, x27, lsl #1");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "ands x29, x28, x27, lsl #63");

  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "ands x29, x28, x27");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "ands x29, x28, x27, lsr #1");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "ands x29, x28, x27, lsr #63");

  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "ands x29, x28, x27");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "ands x29, x28, x27, asr #1");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "ands x29, x28, x27, asr #63");

  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "ands x29, x28, x27");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "ands x29, x28, x27, ror #1");
  TEST_SINGLE(ands(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "ands x29, x28, x27, ror #63");

  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "bic w29, w28, w27");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "bic w29, w28, w27, lsl #1");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "bic w29, w28, w27, lsl #31");

  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "bic w29, w28, w27");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "bic w29, w28, w27, lsr #1");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "bic w29, w28, w27, lsr #31");

  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "bic w29, w28, w27");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "bic w29, w28, w27, asr #1");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "bic w29, w28, w27, asr #31");

  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "bic w29, w28, w27");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "bic w29, w28, w27, ror #1");
  TEST_SINGLE(bic(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "bic w29, w28, w27, ror #31");

  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "bic x29, x28, x27");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "bic x29, x28, x27, lsl #1");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "bic x29, x28, x27, lsl #63");

  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "bic x29, x28, x27");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "bic x29, x28, x27, lsr #1");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "bic x29, x28, x27, lsr #63");

  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "bic x29, x28, x27");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "bic x29, x28, x27, asr #1");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "bic x29, x28, x27, asr #63");

  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "bic x29, x28, x27");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "bic x29, x28, x27, ror #1");
  TEST_SINGLE(bic(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "bic x29, x28, x27, ror #63");

  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "bics w29, w28, w27");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "bics w29, w28, w27, lsl #1");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "bics w29, w28, w27, lsl #31");

  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "bics w29, w28, w27");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "bics w29, w28, w27, lsr #1");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "bics w29, w28, w27, lsr #31");

  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "bics w29, w28, w27");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "bics w29, w28, w27, asr #1");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "bics w29, w28, w27, asr #31");

  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "bics w29, w28, w27");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "bics w29, w28, w27, ror #1");
  TEST_SINGLE(bics(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "bics w29, w28, w27, ror #31");

  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "bics x29, x28, x27");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "bics x29, x28, x27, lsl #1");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "bics x29, x28, x27, lsl #63");

  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "bics x29, x28, x27");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "bics x29, x28, x27, lsr #1");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "bics x29, x28, x27, lsr #63");

  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "bics x29, x28, x27");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "bics x29, x28, x27, asr #1");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "bics x29, x28, x27, asr #63");

  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "bics x29, x28, x27");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "bics x29, x28, x27, ror #1");
  TEST_SINGLE(bics(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "bics x29, x28, x27, ror #63");

  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "orr w29, w28, w27");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "orr w29, w28, w27, lsl #1");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "orr w29, w28, w27, lsl #31");

  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "orr w29, w28, w27");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "orr w29, w28, w27, lsr #1");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "orr w29, w28, w27, lsr #31");

  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "orr w29, w28, w27");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "orr w29, w28, w27, asr #1");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "orr w29, w28, w27, asr #31");

  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "orr w29, w28, w27");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "orr w29, w28, w27, ror #1");
  TEST_SINGLE(orr(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "orr w29, w28, w27, ror #31");

  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "orr x29, x28, x27");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "orr x29, x28, x27, lsl #1");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "orr x29, x28, x27, lsl #63");

  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "orr x29, x28, x27");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "orr x29, x28, x27, lsr #1");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "orr x29, x28, x27, lsr #63");

  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "orr x29, x28, x27");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "orr x29, x28, x27, asr #1");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "orr x29, x28, x27, asr #63");

  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "orr x29, x28, x27");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "orr x29, x28, x27, ror #1");
  TEST_SINGLE(orr(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "orr x29, x28, x27, ror #63");

  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "orn w29, w28, w27");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "orn w29, w28, w27, lsl #1");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "orn w29, w28, w27, lsl #31");

  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "orn w29, w28, w27");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "orn w29, w28, w27, lsr #1");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "orn w29, w28, w27, lsr #31");

  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "orn w29, w28, w27");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "orn w29, w28, w27, asr #1");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "orn w29, w28, w27, asr #31");

  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "orn w29, w28, w27");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "orn w29, w28, w27, ror #1");
  TEST_SINGLE(orn(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "orn w29, w28, w27, ror #31");

  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "orn x29, x28, x27");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "orn x29, x28, x27, lsl #1");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "orn x29, x28, x27, lsl #63");

  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "orn x29, x28, x27");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "orn x29, x28, x27, lsr #1");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "orn x29, x28, x27, lsr #63");

  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "orn x29, x28, x27");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "orn x29, x28, x27, asr #1");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "orn x29, x28, x27, asr #63");

  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "orn x29, x28, x27");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "orn x29, x28, x27, ror #1");
  TEST_SINGLE(orn(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "orn x29, x28, x27, ror #63");

  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "eor w29, w28, w27");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "eor w29, w28, w27, lsl #1");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "eor w29, w28, w27, lsl #31");

  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "eor w29, w28, w27");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "eor w29, w28, w27, lsr #1");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "eor w29, w28, w27, lsr #31");

  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "eor w29, w28, w27");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "eor w29, w28, w27, asr #1");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "eor w29, w28, w27, asr #31");

  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "eor w29, w28, w27");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "eor w29, w28, w27, ror #1");
  TEST_SINGLE(eor(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "eor w29, w28, w27, ror #31");

  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "eor x29, x28, x27");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "eor x29, x28, x27, lsl #1");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "eor x29, x28, x27, lsl #63");

  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "eor x29, x28, x27");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "eor x29, x28, x27, lsr #1");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "eor x29, x28, x27, lsr #63");

  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "eor x29, x28, x27");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "eor x29, x28, x27, asr #1");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "eor x29, x28, x27, asr #63");

  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "eor x29, x28, x27");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "eor x29, x28, x27, ror #1");
  TEST_SINGLE(eor(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "eor x29, x28, x27, ror #63");

  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "eon w29, w28, w27");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "eon w29, w28, w27, lsl #1");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 31), "eon w29, w28, w27, lsl #31");

  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "eon w29, w28, w27");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "eon w29, w28, w27, lsr #1");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 31), "eon w29, w28, w27, lsr #31");

  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "eon w29, w28, w27");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "eon w29, w28, w27, asr #1");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 31), "eon w29, w28, w27, asr #31");

  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "eon w29, w28, w27");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "eon w29, w28, w27, ror #1");
  TEST_SINGLE(eon(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 31), "eon w29, w28, w27, ror #31");

  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 0), "eon x29, x28, x27");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 1), "eon x29, x28, x27, lsl #1");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSL, 63), "eon x29, x28, x27, lsl #63");

  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 0), "eon x29, x28, x27");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 1), "eon x29, x28, x27, lsr #1");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::LSR, 63), "eon x29, x28, x27, lsr #63");

  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 0), "eon x29, x28, x27");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 1), "eon x29, x28, x27, asr #1");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ASR, 63), "eon x29, x28, x27, asr #63");

  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 0), "eon x29, x28, x27");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 1), "eon x29, x28, x27, ror #1");
  TEST_SINGLE(eon(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ShiftType::ROR, 63), "eon x29, x28, x27, ror #63");

  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSL, 0), "tst w28, w27");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSL, 1), "tst w28, w27, lsl #1");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSL, 31), "tst w28, w27, lsl #31");

  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSR, 0), "tst w28, w27");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSR, 1), "tst w28, w27, lsr #1");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::LSR, 31), "tst w28, w27, lsr #31");

  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ASR, 0), "tst w28, w27");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ASR, 1), "tst w28, w27, asr #1");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ASR, 31), "tst w28, w27, asr #31");

  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ROR, 0), "tst w28, w27");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ROR, 1), "tst w28, w27, ror #1");
  TEST_SINGLE(tst(Size::i32Bit, Reg::r28, Reg::r27, ShiftType::ROR, 31), "tst w28, w27, ror #31");

  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSL, 0), "tst x28, x27");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSL, 1), "tst x28, x27, lsl #1");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSL, 63), "tst x28, x27, lsl #63");

  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSR, 0), "tst x28, x27");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSR, 1), "tst x28, x27, lsr #1");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::LSR, 63), "tst x28, x27, lsr #63");

  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ASR, 0), "tst x28, x27");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ASR, 1), "tst x28, x27, asr #1");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ASR, 63), "tst x28, x27, asr #63");

  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ROR, 0), "tst x28, x27");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ROR, 1), "tst x28, x27, ror #1");
  TEST_SINGLE(tst(Size::i64Bit, Reg::r28, Reg::r27, ShiftType::ROR, 63), "tst x28, x27, ror #63");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: AddSub - shifted register") {
  {
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28), "add x30, x29, x28");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28), "add w30, w29, w28");

    // LSL
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "add x30, x29, x28, lsl #1");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "add w30, w29, w28, lsl #1");
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 63), "add x30, x29, x28, lsl #63");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 31), "add w30, w29, w28, lsl #31");

    // LSR
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "add x30, x29, x28, lsr #1");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "add w30, w29, w28, lsr #1");
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 63), "add x30, x29, x28, lsr #63");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 31), "add w30, w29, w28, lsr #31");

    // ASR
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "add x30, x29, x28, asr #1");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "add w30, w29, w28, asr #1");
    TEST_SINGLE(add(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 63), "add x30, x29, x28, asr #63");
    TEST_SINGLE(add(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 31), "add w30, w29, w28, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28), "adds x30, x29, x28");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28), "adds w30, w29, w28");

    // LSL
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "adds x30, x29, x28, lsl #1");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "adds w30, w29, w28, lsl #1");
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 63), "adds x30, x29, x28, lsl #63");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 31), "adds w30, w29, w28, lsl #31");

    // LSR
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "adds x30, x29, x28, lsr #1");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "adds w30, w29, w28, lsr #1");
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 63), "adds x30, x29, x28, lsr #63");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 31), "adds w30, w29, w28, lsr #31");

    // ASR
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "adds x30, x29, x28, asr #1");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "adds w30, w29, w28, asr #1");
    TEST_SINGLE(adds(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 63), "adds x30, x29, x28, asr #63");
    TEST_SINGLE(adds(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 31), "adds w30, w29, w28, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28), "cmn x29, x28");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28), "cmn w29, w28");

    // LSL
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSL, 1), "cmn x29, x28, lsl #1");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSL, 1), "cmn w29, w28, lsl #1");
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSL, 63), "cmn x29, x28, lsl #63");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSL, 31), "cmn w29, w28, lsl #31");

    // LSR
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSR, 1), "cmn x29, x28, lsr #1");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSR, 1), "cmn w29, w28, lsr #1");
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::LSR, 63), "cmn x29, x28, lsr #63");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::LSR, 31), "cmn w29, w28, lsr #31");

    // ASR
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ASR, 1), "cmn x29, x28, asr #1");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ASR, 1), "cmn w29, w28, asr #1");
    TEST_SINGLE(cmn(Size::i64Bit, Reg::r29, Reg::r28, ShiftType::ASR, 63), "cmn x29, x28, asr #63");
    TEST_SINGLE(cmn(Size::i32Bit, Reg::r29, Reg::r28, ShiftType::ASR, 31), "cmn w29, w28, asr #31");

    // ROR
    // Unsupported
  }

  // FEX had a bug with this
  TEST_SINGLE(sub(Size::i64Bit, Reg::rsp, Reg::rsp, Reg::r0, ShiftType::LSL, 0), "neg xzr, x0");

  {
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28), "sub x30, x29, x28");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28), "sub w30, w29, w28");

    // LSL
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "sub x30, x29, x28, lsl #1");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "sub w30, w29, w28, lsl #1");
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 63), "sub x30, x29, x28, lsl #63");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 31), "sub w30, w29, w28, lsl #31");

    // LSR
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "sub x30, x29, x28, lsr #1");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "sub w30, w29, w28, lsr #1");
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 63), "sub x30, x29, x28, lsr #63");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 31), "sub w30, w29, w28, lsr #31");

    // ASR
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "sub x30, x29, x28, asr #1");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "sub w30, w29, w28, asr #1");
    TEST_SINGLE(sub(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 63), "sub x30, x29, x28, asr #63");
    TEST_SINGLE(sub(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 31), "sub w30, w29, w28, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28), "subs x30, x29, x28");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28), "subs w30, w29, w28");

    // LSL
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "subs x30, x29, x28, lsl #1");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 1), "subs w30, w29, w28, lsl #1");
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 63), "subs x30, x29, x28, lsl #63");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSL, 31), "subs w30, w29, w28, lsl #31");

    // LSR
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "subs x30, x29, x28, lsr #1");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 1), "subs w30, w29, w28, lsr #1");
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 63), "subs x30, x29, x28, lsr #63");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::LSR, 31), "subs w30, w29, w28, lsr #31");

    // ASR
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "subs x30, x29, x28, asr #1");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 1), "subs w30, w29, w28, asr #1");
    TEST_SINGLE(subs(Size::i64Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 63), "subs x30, x29, x28, asr #63");
    TEST_SINGLE(subs(Size::i32Bit, Reg::r30, Reg::r29, Reg::r28, ShiftType::ASR, 31), "subs w30, w29, w28, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29), "neg x30, x29");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29), "neg w30, w29");

    // LSL
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "neg x30, x29, lsl #1");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "neg w30, w29, lsl #1");
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 63), "neg x30, x29, lsl #63");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 31), "neg w30, w29, lsl #31");

    // LSR
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "neg x30, x29, lsr #1");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "neg w30, w29, lsr #1");
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 63), "neg x30, x29, lsr #63");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 31), "neg w30, w29, lsr #31");

    // ASR
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "neg x30, x29, asr #1");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "neg w30, w29, asr #1");
    TEST_SINGLE(neg(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 63), "neg x30, x29, asr #63");
    TEST_SINGLE(neg(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 31), "neg w30, w29, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29), "cmp x30, x29");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29), "cmp w30, w29");

    // LSL
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "cmp x30, x29, lsl #1");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "cmp w30, w29, lsl #1");
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 63), "cmp x30, x29, lsl #63");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 31), "cmp w30, w29, lsl #31");

    // LSR
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "cmp x30, x29, lsr #1");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "cmp w30, w29, lsr #1");
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 63), "cmp x30, x29, lsr #63");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 31), "cmp w30, w29, lsr #31");

    // ASR
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "cmp x30, x29, asr #1");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "cmp w30, w29, asr #1");
    TEST_SINGLE(cmp(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 63), "cmp x30, x29, asr #63");
    TEST_SINGLE(cmp(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 31), "cmp w30, w29, asr #31");

    // ROR
    // Unsupported
  }

  {
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29), "negs x30, x29");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29), "negs w30, w29");

    // LSL
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "negs x30, x29, lsl #1");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 1), "negs w30, w29, lsl #1");
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSL, 63), "negs x30, x29, lsl #63");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSL, 31), "negs w30, w29, lsl #31");

    // LSR
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "negs x30, x29, lsr #1");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 1), "negs w30, w29, lsr #1");
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::LSR, 63), "negs x30, x29, lsr #63");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::LSR, 31), "negs w30, w29, lsr #31");

    // ASR
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "negs x30, x29, asr #1");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 1), "negs w30, w29, asr #1");
    TEST_SINGLE(negs(Size::i64Bit, Reg::r30, Reg::r29, ShiftType::ASR, 63), "negs x30, x29, asr #63");
    TEST_SINGLE(negs(Size::i32Bit, Reg::r30, Reg::r29, ShiftType::ASR, 31), "negs w30, w29, asr #31");

    // ROR
    // Unsupported
  }
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: AddSub - extended register") {
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "add w29, w28, w27, uxtb");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "add w29, w28, w27, uxtb #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "add w29, w28, w27, uxtb #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "add w29, w28, w27, uxtb #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "add w29, w28, w27, uxtb #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "add w29, w28, w27, uxth");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "add w29, w28, w27, uxth #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "add w29, w28, w27, uxth #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "add w29, w28, w27, uxth #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "add w29, w28, w27, uxth #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "add w29, w28, w27, uxtw");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "add w29, w28, w27, uxtw #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "add w29, w28, w27, uxtw #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "add w29, w28, w27, uxtw #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "add w29, w28, w27, uxtw #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "add w29, w28, x27, uxtx");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "add w29, w28, x27, uxtx #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "add w29, w28, x27, uxtx #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "add w29, w28, x27, uxtx #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "add w29, w28, x27, uxtx #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "add w29, w28, w27, sxtb");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "add w29, w28, w27, sxtb #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "add w29, w28, w27, sxtb #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "add w29, w28, w27, sxtb #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "add w29, w28, w27, sxtb #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "add w29, w28, w27, sxth");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "add w29, w28, w27, sxth #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "add w29, w28, w27, sxth #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "add w29, w28, w27, sxth #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "add w29, w28, w27, sxth #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "add w29, w28, w27, sxtw");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "add w29, w28, w27, sxtw #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "add w29, w28, w27, sxtw #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "add w29, w28, w27, sxtw #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "add w29, w28, w27, sxtw #4");

  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "add w29, w28, x27, sxtx");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "add w29, w28, x27, sxtx #1");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "add w29, w28, x27, sxtx #2");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "add w29, w28, x27, sxtx #3");
  TEST_SINGLE(add(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "add w29, w28, x27, sxtx #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "add x29, x28, w27, uxtb");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "add x29, x28, w27, uxtb #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "add x29, x28, w27, uxtb #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "add x29, x28, w27, uxtb #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "add x29, x28, w27, uxtb #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "add x29, x28, w27, uxth");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "add x29, x28, w27, uxth #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "add x29, x28, w27, uxth #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "add x29, x28, w27, uxth #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "add x29, x28, w27, uxth #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "add x29, x28, w27, uxtw");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "add x29, x28, w27, uxtw #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "add x29, x28, w27, uxtw #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "add x29, x28, w27, uxtw #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "add x29, x28, w27, uxtw #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "add x29, x28, x27, uxtx");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "add x29, x28, x27, uxtx #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "add x29, x28, x27, uxtx #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "add x29, x28, x27, uxtx #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "add x29, x28, x27, uxtx #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "add x29, x28, w27, sxtb");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "add x29, x28, w27, sxtb #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "add x29, x28, w27, sxtb #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "add x29, x28, w27, sxtb #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "add x29, x28, w27, sxtb #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "add x29, x28, w27, sxth");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "add x29, x28, w27, sxth #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "add x29, x28, w27, sxth #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "add x29, x28, w27, sxth #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "add x29, x28, w27, sxth #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "add x29, x28, w27, sxtw");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "add x29, x28, w27, sxtw #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "add x29, x28, w27, sxtw #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "add x29, x28, w27, sxtw #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "add x29, x28, w27, sxtw #4");

  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "add x29, x28, x27, sxtx");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "add x29, x28, x27, sxtx #1");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "add x29, x28, x27, sxtx #2");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "add x29, x28, x27, sxtx #3");
  TEST_SINGLE(add(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "add x29, x28, x27, sxtx #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "adds w29, w28, w27, uxtb");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "adds w29, w28, w27, uxtb #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "adds w29, w28, w27, uxtb #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "adds w29, w28, w27, uxtb #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "adds w29, w28, w27, uxtb #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "adds w29, w28, w27, uxth");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "adds w29, w28, w27, uxth #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "adds w29, w28, w27, uxth #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "adds w29, w28, w27, uxth #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "adds w29, w28, w27, uxth #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "adds w29, w28, w27, uxtw");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "adds w29, w28, w27, uxtw #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "adds w29, w28, w27, uxtw #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "adds w29, w28, w27, uxtw #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "adds w29, w28, w27, uxtw #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "adds w29, w28, x27, uxtx");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "adds w29, w28, x27, uxtx #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "adds w29, w28, x27, uxtx #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "adds w29, w28, x27, uxtx #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "adds w29, w28, x27, uxtx #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "adds w29, w28, w27, sxtb");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "adds w29, w28, w27, sxtb #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "adds w29, w28, w27, sxtb #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "adds w29, w28, w27, sxtb #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "adds w29, w28, w27, sxtb #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "adds w29, w28, w27, sxth");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "adds w29, w28, w27, sxth #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "adds w29, w28, w27, sxth #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "adds w29, w28, w27, sxth #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "adds w29, w28, w27, sxth #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "adds w29, w28, w27, sxtw");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "adds w29, w28, w27, sxtw #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "adds w29, w28, w27, sxtw #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "adds w29, w28, w27, sxtw #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "adds w29, w28, w27, sxtw #4");

  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "adds w29, w28, x27, sxtx");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "adds w29, w28, x27, sxtx #1");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "adds w29, w28, x27, sxtx #2");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "adds w29, w28, x27, sxtx #3");
  TEST_SINGLE(adds(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "adds w29, w28, x27, sxtx #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "adds x29, x28, w27, uxtb");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "adds x29, x28, w27, uxtb #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "adds x29, x28, w27, uxtb #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "adds x29, x28, w27, uxtb #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "adds x29, x28, w27, uxtb #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "adds x29, x28, w27, uxth");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "adds x29, x28, w27, uxth #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "adds x29, x28, w27, uxth #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "adds x29, x28, w27, uxth #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "adds x29, x28, w27, uxth #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "adds x29, x28, w27, uxtw");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "adds x29, x28, w27, uxtw #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "adds x29, x28, w27, uxtw #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "adds x29, x28, w27, uxtw #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "adds x29, x28, w27, uxtw #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "adds x29, x28, x27, uxtx");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "adds x29, x28, x27, uxtx #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "adds x29, x28, x27, uxtx #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "adds x29, x28, x27, uxtx #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "adds x29, x28, x27, uxtx #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "adds x29, x28, w27, sxtb");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "adds x29, x28, w27, sxtb #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "adds x29, x28, w27, sxtb #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "adds x29, x28, w27, sxtb #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "adds x29, x28, w27, sxtb #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "adds x29, x28, w27, sxth");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "adds x29, x28, w27, sxth #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "adds x29, x28, w27, sxth #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "adds x29, x28, w27, sxth #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "adds x29, x28, w27, sxth #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "adds x29, x28, w27, sxtw");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "adds x29, x28, w27, sxtw #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "adds x29, x28, w27, sxtw #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "adds x29, x28, w27, sxtw #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "adds x29, x28, w27, sxtw #4");

  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "adds x29, x28, x27, sxtx");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "adds x29, x28, x27, sxtx #1");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "adds x29, x28, x27, sxtx #2");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "adds x29, x28, x27, sxtx #3");
  TEST_SINGLE(adds(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "adds x29, x28, x27, sxtx #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "cmn w28, w27, uxtb");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "cmn w28, w27, uxtb #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "cmn w28, w27, uxtb #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "cmn w28, w27, uxtb #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "cmn w28, w27, uxtb #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "cmn w28, w27, uxth");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "cmn w28, w27, uxth #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "cmn w28, w27, uxth #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "cmn w28, w27, uxth #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "cmn w28, w27, uxth #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_32, 0), "cmn w28, w27");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_32, 1), "cmn w28, w27, lsl #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_32, 2), "cmn w28, w27, lsl #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_32, 3), "cmn w28, w27, lsl #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_32, 4), "cmn w28, w27, lsl #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 0), "cmn w28, x27");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 1), "cmn w28, x27, lsl #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 2), "cmn w28, x27, lsl #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 3), "cmn w28, x27, lsl #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 4), "cmn w28, x27, lsl #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "cmn w28, w27, sxtb");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "cmn w28, w27, sxtb #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "cmn w28, w27, sxtb #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "cmn w28, w27, sxtb #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "cmn w28, w27, sxtb #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "cmn w28, w27, sxth");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "cmn w28, w27, sxth #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "cmn w28, w27, sxth #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "cmn w28, w27, sxth #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "cmn w28, w27, sxth #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "cmn w28, w27, sxtw");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "cmn w28, w27, sxtw #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "cmn w28, w27, sxtw #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "cmn w28, w27, sxtw #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "cmn w28, w27, sxtw #4");

  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "cmn w28, x27, sxtx");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "cmn w28, x27, sxtx #1");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "cmn w28, x27, sxtx #2");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "cmn w28, x27, sxtx #3");
  TEST_SINGLE(cmn(Size::i32Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "cmn w28, x27, sxtx #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "cmn x28, w27, uxtb");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "cmn x28, w27, uxtb #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "cmn x28, w27, uxtb #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "cmn x28, w27, uxtb #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "cmn x28, w27, uxtb #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "cmn x28, w27, uxth");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "cmn x28, w27, uxth #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "cmn x28, w27, uxth #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "cmn x28, w27, uxth #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "cmn x28, w27, uxth #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "cmn x28, w27, uxtw");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "cmn x28, w27, uxtw #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "cmn x28, w27, uxtw #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "cmn x28, w27, uxtw #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "cmn x28, w27, uxtw #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 0), "cmn x28, x27");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 1), "cmn x28, x27, lsl #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 2), "cmn x28, x27, lsl #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 3), "cmn x28, x27, lsl #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::LSL_64, 4), "cmn x28, x27, lsl #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "cmn x28, w27, sxtb");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "cmn x28, w27, sxtb #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "cmn x28, w27, sxtb #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "cmn x28, w27, sxtb #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "cmn x28, w27, sxtb #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "cmn x28, w27, sxth");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "cmn x28, w27, sxth #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "cmn x28, w27, sxth #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "cmn x28, w27, sxth #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "cmn x28, w27, sxth #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "cmn x28, w27, sxtw");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "cmn x28, w27, sxtw #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "cmn x28, w27, sxtw #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "cmn x28, w27, sxtw #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "cmn x28, w27, sxtw #4");

  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "cmn x28, x27, sxtx");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "cmn x28, x27, sxtx #1");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "cmn x28, x27, sxtx #2");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "cmn x28, x27, sxtx #3");
  TEST_SINGLE(cmn(Size::i64Bit, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "cmn x28, x27, sxtx #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "sub w29, w28, w27, uxtb");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "sub w29, w28, w27, uxtb #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "sub w29, w28, w27, uxtb #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "sub w29, w28, w27, uxtb #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "sub w29, w28, w27, uxtb #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "sub w29, w28, w27, uxth");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "sub w29, w28, w27, uxth #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "sub w29, w28, w27, uxth #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "sub w29, w28, w27, uxth #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "sub w29, w28, w27, uxth #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "sub w29, w28, w27, uxtw");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "sub w29, w28, w27, uxtw #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "sub w29, w28, w27, uxtw #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "sub w29, w28, w27, uxtw #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "sub w29, w28, w27, uxtw #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "sub w29, w28, x27, uxtx");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "sub w29, w28, x27, uxtx #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "sub w29, w28, x27, uxtx #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "sub w29, w28, x27, uxtx #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "sub w29, w28, x27, uxtx #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "sub w29, w28, w27, sxtb");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "sub w29, w28, w27, sxtb #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "sub w29, w28, w27, sxtb #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "sub w29, w28, w27, sxtb #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "sub w29, w28, w27, sxtb #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "sub w29, w28, w27, sxth");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "sub w29, w28, w27, sxth #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "sub w29, w28, w27, sxth #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "sub w29, w28, w27, sxth #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "sub w29, w28, w27, sxth #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "sub w29, w28, w27, sxtw");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "sub w29, w28, w27, sxtw #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "sub w29, w28, w27, sxtw #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "sub w29, w28, w27, sxtw #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "sub w29, w28, w27, sxtw #4");

  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "sub w29, w28, x27, sxtx");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "sub w29, w28, x27, sxtx #1");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "sub w29, w28, x27, sxtx #2");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "sub w29, w28, x27, sxtx #3");
  TEST_SINGLE(sub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "sub w29, w28, x27, sxtx #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "sub x29, x28, w27, uxtb");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "sub x29, x28, w27, uxtb #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "sub x29, x28, w27, uxtb #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "sub x29, x28, w27, uxtb #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "sub x29, x28, w27, uxtb #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "sub x29, x28, w27, uxth");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "sub x29, x28, w27, uxth #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "sub x29, x28, w27, uxth #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "sub x29, x28, w27, uxth #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "sub x29, x28, w27, uxth #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "sub x29, x28, w27, uxtw");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "sub x29, x28, w27, uxtw #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "sub x29, x28, w27, uxtw #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "sub x29, x28, w27, uxtw #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "sub x29, x28, w27, uxtw #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "sub x29, x28, x27, uxtx");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "sub x29, x28, x27, uxtx #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "sub x29, x28, x27, uxtx #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "sub x29, x28, x27, uxtx #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "sub x29, x28, x27, uxtx #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "sub x29, x28, w27, sxtb");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "sub x29, x28, w27, sxtb #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "sub x29, x28, w27, sxtb #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "sub x29, x28, w27, sxtb #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "sub x29, x28, w27, sxtb #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "sub x29, x28, w27, sxth");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "sub x29, x28, w27, sxth #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "sub x29, x28, w27, sxth #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "sub x29, x28, w27, sxth #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "sub x29, x28, w27, sxth #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "sub x29, x28, w27, sxtw");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "sub x29, x28, w27, sxtw #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "sub x29, x28, w27, sxtw #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "sub x29, x28, w27, sxtw #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "sub x29, x28, w27, sxtw #4");

  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "sub x29, x28, x27, sxtx");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "sub x29, x28, x27, sxtx #1");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "sub x29, x28, x27, sxtx #2");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "sub x29, x28, x27, sxtx #3");
  TEST_SINGLE(sub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "sub x29, x28, x27, sxtx #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "subs w29, w28, w27, uxtb");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "subs w29, w28, w27, uxtb #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "subs w29, w28, w27, uxtb #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "subs w29, w28, w27, uxtb #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "subs w29, w28, w27, uxtb #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "subs w29, w28, w27, uxth");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "subs w29, w28, w27, uxth #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "subs w29, w28, w27, uxth #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "subs w29, w28, w27, uxth #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "subs w29, w28, w27, uxth #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "subs w29, w28, w27, uxtw");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "subs w29, w28, w27, uxtw #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "subs w29, w28, w27, uxtw #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "subs w29, w28, w27, uxtw #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "subs w29, w28, w27, uxtw #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "subs w29, w28, x27, uxtx");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "subs w29, w28, x27, uxtx #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "subs w29, w28, x27, uxtx #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "subs w29, w28, x27, uxtx #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "subs w29, w28, x27, uxtx #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "subs w29, w28, w27, sxtb");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "subs w29, w28, w27, sxtb #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "subs w29, w28, w27, sxtb #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "subs w29, w28, w27, sxtb #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "subs w29, w28, w27, sxtb #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "subs w29, w28, w27, sxth");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "subs w29, w28, w27, sxth #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "subs w29, w28, w27, sxth #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "subs w29, w28, w27, sxth #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "subs w29, w28, w27, sxth #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "subs w29, w28, w27, sxtw");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "subs w29, w28, w27, sxtw #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "subs w29, w28, w27, sxtw #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "subs w29, w28, w27, sxtw #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "subs w29, w28, w27, sxtw #4");

  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "subs w29, w28, x27, sxtx");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "subs w29, w28, x27, sxtx #1");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "subs w29, w28, x27, sxtx #2");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "subs w29, w28, x27, sxtx #3");
  TEST_SINGLE(subs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "subs w29, w28, x27, sxtx #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 0), "subs x29, x28, w27, uxtb");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 1), "subs x29, x28, w27, uxtb #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 2), "subs x29, x28, w27, uxtb #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 3), "subs x29, x28, w27, uxtb #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTB, 4), "subs x29, x28, w27, uxtb #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 0), "subs x29, x28, w27, uxth");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 1), "subs x29, x28, w27, uxth #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 2), "subs x29, x28, w27, uxth #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 3), "subs x29, x28, w27, uxth #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTH, 4), "subs x29, x28, w27, uxth #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 0), "subs x29, x28, w27, uxtw");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 1), "subs x29, x28, w27, uxtw #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 2), "subs x29, x28, w27, uxtw #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 3), "subs x29, x28, w27, uxtw #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTW, 4), "subs x29, x28, w27, uxtw #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 0), "subs x29, x28, x27, uxtx");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 1), "subs x29, x28, x27, uxtx #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 2), "subs x29, x28, x27, uxtx #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 3), "subs x29, x28, x27, uxtx #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::UXTX, 4), "subs x29, x28, x27, uxtx #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 0), "subs x29, x28, w27, sxtb");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 1), "subs x29, x28, w27, sxtb #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 2), "subs x29, x28, w27, sxtb #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 3), "subs x29, x28, w27, sxtb #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTB, 4), "subs x29, x28, w27, sxtb #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 0), "subs x29, x28, w27, sxth");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 1), "subs x29, x28, w27, sxth #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 2), "subs x29, x28, w27, sxth #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 3), "subs x29, x28, w27, sxth #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTH, 4), "subs x29, x28, w27, sxth #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 0), "subs x29, x28, w27, sxtw");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 1), "subs x29, x28, w27, sxtw #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 2), "subs x29, x28, w27, sxtw #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 3), "subs x29, x28, w27, sxtw #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTW, 4), "subs x29, x28, w27, sxtw #4");

  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 0), "subs x29, x28, x27, sxtx");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 1), "subs x29, x28, x27, sxtx #1");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 2), "subs x29, x28, x27, sxtx #2");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 3), "subs x29, x28, x27, sxtx #3");
  TEST_SINGLE(subs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, ExtendedType::SXTX, 4), "subs x29, x28, x27, sxtx #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 0), "cmp w29, w28, uxtb");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 1), "cmp w29, w28, uxtb #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 2), "cmp w29, w28, uxtb #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 3), "cmp w29, w28, uxtb #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 4), "cmp w29, w28, uxtb #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 0), "cmp w29, w28, uxth");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 1), "cmp w29, w28, uxth #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 2), "cmp w29, w28, uxth #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 3), "cmp w29, w28, uxth #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 4), "cmp w29, w28, uxth #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_32, 0), "cmp w29, w28");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_32, 1), "cmp w29, w28, lsl #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_32, 2), "cmp w29, w28, lsl #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_32, 3), "cmp w29, w28, lsl #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_32, 4), "cmp w29, w28, lsl #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 0), "cmp w29, x28");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 1), "cmp w29, x28, lsl #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 2), "cmp w29, x28, lsl #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 3), "cmp w29, x28, lsl #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 4), "cmp w29, x28, lsl #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 0), "cmp w29, w28, sxtb");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 1), "cmp w29, w28, sxtb #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 2), "cmp w29, w28, sxtb #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 3), "cmp w29, w28, sxtb #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 4), "cmp w29, w28, sxtb #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 0), "cmp w29, w28, sxth");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 1), "cmp w29, w28, sxth #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 2), "cmp w29, w28, sxth #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 3), "cmp w29, w28, sxth #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 4), "cmp w29, w28, sxth #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 0), "cmp w29, w28, sxtw");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 1), "cmp w29, w28, sxtw #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 2), "cmp w29, w28, sxtw #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 3), "cmp w29, w28, sxtw #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 4), "cmp w29, w28, sxtw #4");

  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 0), "cmp w29, x28, sxtx");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 1), "cmp w29, x28, sxtx #1");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 2), "cmp w29, x28, sxtx #2");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 3), "cmp w29, x28, sxtx #3");
  TEST_SINGLE(cmp(Size::i32Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 4), "cmp w29, x28, sxtx #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 0), "cmp x29, w28, uxtb");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 1), "cmp x29, w28, uxtb #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 2), "cmp x29, w28, uxtb #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 3), "cmp x29, w28, uxtb #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTB, 4), "cmp x29, w28, uxtb #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 0), "cmp x29, w28, uxth");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 1), "cmp x29, w28, uxth #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 2), "cmp x29, w28, uxth #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 3), "cmp x29, w28, uxth #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTH, 4), "cmp x29, w28, uxth #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTW, 0), "cmp x29, w28, uxtw");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTW, 1), "cmp x29, w28, uxtw #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTW, 2), "cmp x29, w28, uxtw #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTW, 3), "cmp x29, w28, uxtw #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::UXTW, 4), "cmp x29, w28, uxtw #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 0), "cmp x29, x28");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 1), "cmp x29, x28, lsl #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 2), "cmp x29, x28, lsl #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 3), "cmp x29, x28, lsl #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::LSL_64, 4), "cmp x29, x28, lsl #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 0), "cmp x29, w28, sxtb");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 1), "cmp x29, w28, sxtb #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 2), "cmp x29, w28, sxtb #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 3), "cmp x29, w28, sxtb #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTB, 4), "cmp x29, w28, sxtb #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 0), "cmp x29, w28, sxth");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 1), "cmp x29, w28, sxth #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 2), "cmp x29, w28, sxth #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 3), "cmp x29, w28, sxth #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTH, 4), "cmp x29, w28, sxth #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 0), "cmp x29, w28, sxtw");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 1), "cmp x29, w28, sxtw #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 2), "cmp x29, w28, sxtw #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 3), "cmp x29, w28, sxtw #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTW, 4), "cmp x29, w28, sxtw #4");

  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 0), "cmp x29, x28, sxtx");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 1), "cmp x29, x28, sxtx #1");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 2), "cmp x29, x28, sxtx #2");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 3), "cmp x29, x28, sxtx #3");
  TEST_SINGLE(cmp(Size::i64Bit, Reg::r29, Reg::r28, ExtendedType::SXTX, 4), "cmp x29, x28, sxtx #4");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: AddSub - with carry") {
  TEST_SINGLE(adc(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "adc w29, w28, w27");
  TEST_SINGLE(adc(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "adc x29, x28, x27");

  TEST_SINGLE(adcs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "adcs w29, w28, w27");
  TEST_SINGLE(adcs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "adcs x29, x28, x27");

  TEST_SINGLE(sbc(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "sbc w29, w28, w27");
  TEST_SINGLE(sbc(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "sbc x29, x28, x27");

  TEST_SINGLE(sbcs(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "sbcs w29, w28, w27");
  TEST_SINGLE(sbcs(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "sbcs x29, x28, x27");

  TEST_SINGLE(ngc(Size::i32Bit, Reg::r29, Reg::r27), "ngc w29, w27");
  TEST_SINGLE(ngc(Size::i64Bit, Reg::r29, Reg::r27), "ngc x29, x27");

  TEST_SINGLE(ngcs(Size::i32Bit, Reg::r29, Reg::r27), "ngcs w29, w27");
  TEST_SINGLE(ngcs(Size::i64Bit, Reg::r29, Reg::r27), "ngcs x29, x27");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Rotate right into flags") {
  TEST_SINGLE(rmif(XReg::x30, 63, 0b0000), "rmif x30, #63, #nzcv");
  TEST_SINGLE(rmif(XReg::x30, 63, 0b0001), "rmif x30, #63, #nzcV");
  TEST_SINGLE(rmif(XReg::x30, 63, 0b0010), "rmif x30, #63, #nzCv");
  TEST_SINGLE(rmif(XReg::x30, 63, 0b0100), "rmif x30, #63, #nZcv");
  TEST_SINGLE(rmif(XReg::x30, 63, 0b1000), "rmif x30, #63, #Nzcv");
  TEST_SINGLE(rmif(XReg::x30, 63, 0b1111), "rmif x30, #63, #NZCV");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Evaluate into flags") {
  TEST_SINGLE(setf8(WReg::w30), "setf8 w30");
  TEST_SINGLE(setf16(WReg::w30), "setf16 w30");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Carry flag invert") {
  TEST_SINGLE(cfinv(), "cfinv");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Arm to eXternal FLAG") {
  TEST_SINGLE(axflag(), "axflag");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: eXternal to Arm FLAG") {
  TEST_SINGLE(xaflag(), "xaflag");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Conditional compare - register") {
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_AL), "ccmn w29, w28, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_AL), "ccmn w29, w28, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn w29, w28, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_AL), "ccmn w29, w28, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_AL), "ccmn w29, w28, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn w29, w28, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_EQ), "ccmn w29, w28, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn w29, w28, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn w29, w28, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn w29, w28, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn w29, w28, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn w29, w28, #NZCV, eq");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_AL), "ccmn x29, x28, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_AL), "ccmn x29, x28, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn x29, x28, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_AL), "ccmn x29, x28, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_AL), "ccmn x29, x28, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn x29, x28, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_EQ), "ccmn x29, x28, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn x29, x28, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn x29, x28, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn x29, x28, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn x29, x28, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn x29, x28, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_AL), "ccmp w29, w28, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_AL), "ccmp w29, w28, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp w29, w28, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_AL), "ccmp w29, w28, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_AL), "ccmp w29, w28, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp w29, w28, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_EQ), "ccmp w29, w28, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp w29, w28, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp w29, w28, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp w29, w28, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp w29, w28, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp w29, w28, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_AL), "ccmp x29, x28, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_AL), "ccmp x29, x28, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp x29, x28, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_AL), "ccmp x29, x28, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_AL), "ccmp x29, x28, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp x29, x28, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::None, Condition::CC_EQ), "ccmp x29, x28, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp x29, x28, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp x29, x28, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp x29, x28, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp x29, x28, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, Reg::r28, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp x29, x28, #NZCV, eq");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Conditional compare - immediate") {
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_AL), "ccmn w29, #0, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_AL), "ccmn w29, #0, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn w29, #0, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_AL), "ccmn w29, #0, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_AL), "ccmn w29, #0, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn w29, #0, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_EQ), "ccmn w29, #0, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn w29, #0, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn w29, #0, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn w29, #0, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn w29, #0, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn w29, #0, #NZCV, eq");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_AL), "ccmn x29, #0, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_AL), "ccmn x29, #0, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn x29, #0, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_AL), "ccmn x29, #0, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_AL), "ccmn x29, #0, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn x29, #0, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_EQ), "ccmn x29, #0, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn x29, #0, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn x29, #0, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn x29, #0, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn x29, #0, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn x29, #0, #NZCV, eq");

  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_AL), "ccmn w29, #31, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_AL), "ccmn w29, #31, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn w29, #31, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_AL), "ccmn w29, #31, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_AL), "ccmn w29, #31, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn w29, #31, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_EQ), "ccmn w29, #31, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn w29, #31, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn w29, #31, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn w29, #31, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn w29, #31, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn w29, #31, #NZCV, eq");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_AL), "ccmn x29, #31, #nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_AL), "ccmn x29, #31, #Nzcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_AL), "ccmn x29, #31, #nZcv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_AL), "ccmn x29, #31, #nzCv, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_AL), "ccmn x29, #31, #nzcV, al");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmn x29, #31, #NZCV, al");

  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_EQ), "ccmn x29, #31, #nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_EQ), "ccmn x29, #31, #Nzcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmn x29, #31, #nZcv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_EQ), "ccmn x29, #31, #nzCv, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_EQ), "ccmn x29, #31, #nzcV, eq");
  TEST_SINGLE(ccmn(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmn x29, #31, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_AL), "ccmp w29, #0, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_AL), "ccmp w29, #0, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp w29, #0, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_AL), "ccmp w29, #0, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_AL), "ccmp w29, #0, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp w29, #0, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_EQ), "ccmp w29, #0, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp w29, #0, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp w29, #0, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp w29, #0, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp w29, #0, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp w29, #0, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_AL), "ccmp x29, #0, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_AL), "ccmp x29, #0, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp x29, #0, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_AL), "ccmp x29, #0, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_AL), "ccmp x29, #0, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp x29, #0, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::None, Condition::CC_EQ), "ccmp x29, #0, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp x29, #0, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp x29, #0, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp x29, #0, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp x29, #0, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 0, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp x29, #0, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_AL), "ccmp w29, #31, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_AL), "ccmp w29, #31, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp w29, #31, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_AL), "ccmp w29, #31, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_AL), "ccmp w29, #31, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp w29, #31, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_EQ), "ccmp w29, #31, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp w29, #31, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp w29, #31, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp w29, #31, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp w29, #31, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i32Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp w29, #31, #NZCV, eq");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_AL), "ccmp x29, #31, #nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_AL), "ccmp x29, #31, #Nzcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_AL), "ccmp x29, #31, #nZcv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_AL), "ccmp x29, #31, #nzCv, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_AL), "ccmp x29, #31, #nzcV, al");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_AL), "ccmp x29, #31, #NZCV, al");

  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::None, Condition::CC_EQ), "ccmp x29, #31, #nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_N, Condition::CC_EQ), "ccmp x29, #31, #Nzcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_Z, Condition::CC_EQ), "ccmp x29, #31, #nZcv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_C, Condition::CC_EQ), "ccmp x29, #31, #nzCv, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_V, Condition::CC_EQ), "ccmp x29, #31, #nzcV, eq");
  TEST_SINGLE(ccmp(Size::i64Bit, Reg::r29, 31, StatusFlags::Flag_NZCV, Condition::CC_EQ), "ccmp x29, #31, #NZCV, eq");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Conditional select") {
  TEST_SINGLE(csel(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csel w29, w28, w27, eq");
  TEST_SINGLE(csel(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csel x29, x28, x27, eq");
  TEST_SINGLE(cset(Size::i32Bit, Reg::r29, Condition::CC_EQ), "cset w29, eq");
  TEST_SINGLE(cset(Size::i64Bit, Reg::r29, Condition::CC_EQ), "cset x29, eq");
  TEST_SINGLE(csinc(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csinc w29, w28, w27, eq");
  TEST_SINGLE(csinc(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csinc x29, x28, x27, eq");
  TEST_SINGLE(csinv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csinv w29, w28, w27, eq");
  TEST_SINGLE(csinv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csinv x29, x28, x27, eq");
  TEST_SINGLE(csneg(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csneg w29, w28, w27, eq");
  TEST_SINGLE(csneg(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_EQ), "csneg x29, x28, x27, eq");
  TEST_SINGLE(cneg(Size::i32Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cneg w29, w28, eq");
  TEST_SINGLE(cneg(Size::i64Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cneg x29, x28, eq");

  TEST_SINGLE(cinc(Size::i32Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cinc w29, w28, eq");
  TEST_SINGLE(cinc(Size::i64Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cinc x29, x28, eq");
  TEST_SINGLE(cinv(Size::i32Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cinv w29, w28, eq");
  TEST_SINGLE(cinv(Size::i64Bit, Reg::r29, Reg::r28, Condition::CC_EQ), "cinv x29, x28, eq");
  TEST_SINGLE(csetm(Size::i32Bit, Reg::r29, Condition::CC_EQ), "csetm w29, eq");
  TEST_SINGLE(csetm(Size::i64Bit, Reg::r29, Condition::CC_EQ), "csetm x29, eq");

  TEST_SINGLE(csel(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csel w29, w28, w27, al");
  TEST_SINGLE(csel(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csel x29, x28, x27, al");
  TEST_SINGLE(cset(Size::i32Bit, Reg::r29, Condition::CC_AL), "csinc w29, wzr, wzr, nv");
  TEST_SINGLE(cset(Size::i64Bit, Reg::r29, Condition::CC_AL), "csinc x29, xzr, xzr, nv");
  TEST_SINGLE(csinc(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csinc w29, w28, w27, al");
  TEST_SINGLE(csinc(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csinc x29, x28, x27, al");
  TEST_SINGLE(csinv(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csinv w29, w28, w27, al");
  TEST_SINGLE(csinv(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csinv x29, x28, x27, al");
  TEST_SINGLE(csneg(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csneg w29, w28, w27, al");
  TEST_SINGLE(csneg(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Condition::CC_AL), "csneg x29, x28, x27, al");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ALU: Data processing - 3 source") {
  TEST_SINGLE(madd(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Reg::r26), "madd w29, w28, w27, w26");
  TEST_SINGLE(madd(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Reg::r26), "madd x29, x28, x27, x26");
  TEST_SINGLE(mul(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "mul w29, w28, w27");
  TEST_SINGLE(mul(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "mul x29, x28, x27");
  TEST_SINGLE(msub(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27, Reg::r26), "msub w29, w28, w27, w26");
  TEST_SINGLE(msub(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27, Reg::r26), "msub x29, x28, x27, x26");
  TEST_SINGLE(mneg(Size::i32Bit, Reg::r29, Reg::r28, Reg::r27), "mneg w29, w28, w27");
  TEST_SINGLE(mneg(Size::i64Bit, Reg::r29, Reg::r28, Reg::r27), "mneg x29, x28, x27");

  TEST_SINGLE(smaddl(XReg::x29, WReg::w28, WReg::w27, XReg::x26), "smaddl x29, w28, w27, x26");
  TEST_SINGLE(smull(XReg::x29, WReg::w28, WReg::w27), "smull x29, w28, w27");
  TEST_SINGLE(smsubl(XReg::x29, WReg::w28, WReg::w27, XReg::x26), "smsubl x29, w28, w27, x26");
  TEST_SINGLE(smnegl(XReg::x29, WReg::w28, WReg::w27), "smnegl x29, w28, w27");
  TEST_SINGLE(smulh(XReg::x29, XReg::x28, XReg::x27), "smulh x29, x28, x27");

  TEST_SINGLE(umaddl(XReg::x29, WReg::w28, WReg::w27, XReg::x26), "umaddl x29, w28, w27, x26");
  TEST_SINGLE(umull(XReg::x29, WReg::w28, WReg::w27), "umull x29, w28, w27");
  TEST_SINGLE(umsubl(XReg::x29, WReg::w28, WReg::w27, XReg::x26), "umsubl x29, w28, w27, x26");
  TEST_SINGLE(umnegl(XReg::x29, WReg::w28, WReg::w27), "umnegl x29, w28, w27");
  TEST_SINGLE(umulh(XReg::x29, XReg::x28, XReg::x27), "umulh x29, x28, x27");
}
