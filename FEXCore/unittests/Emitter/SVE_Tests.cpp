// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: Base Encodings") {
  TEST_SINGLE(dup(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 0), "mov z30.b, b29");
  TEST_SINGLE(dup(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "mov z30.b, z29.b[1]");
  TEST_SINGLE(dup(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 63), "mov z30.b, z29.b[63]");

  TEST_SINGLE(dup(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "mov z30.h, h29");
  TEST_SINGLE(dup(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "mov z30.h, z29.h[1]");
  TEST_SINGLE(dup(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 31), "mov z30.h, z29.h[31]");

  TEST_SINGLE(dup(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "mov z30.s, s29");
  TEST_SINGLE(dup(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "mov z30.s, z29.s[1]");
  TEST_SINGLE(dup(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "mov z30.s, z29.s[15]");

  TEST_SINGLE(dup(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "mov z30.d, d29");
  TEST_SINGLE(dup(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "mov z30.d, z29.d[1]");
  TEST_SINGLE(dup(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 7), "mov z30.d, z29.d[7]");

  TEST_SINGLE(dup(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, 0), "mov z30.q, q29");
  TEST_SINGLE(dup(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, 1), "mov z30.q, z29.q[1]");
  TEST_SINGLE(dup(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, 3), "mov z30.q, z29.q[3]");

  TEST_SINGLE(sel(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.b, p6, z29.b, z28.b");
  TEST_SINGLE(sel(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.h, p6, z29.h, z28.h");
  TEST_SINGLE(sel(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.s, p6, z29.s, z28.s");
  TEST_SINGLE(sel(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.d, p6, z29.d, z28.d");
  // TEST_SINGLE(sel(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.q, p6, z29.q, z28.q");

  TEST_SINGLE(mov(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.b, p6/m, z29.b");
  TEST_SINGLE(mov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.h, p6/m, z29.h");
  TEST_SINGLE(mov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.s, p6/m, z29.s");
  TEST_SINGLE(mov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.d, p6/m, z29.d");
  // TEST_SINGLE(mov(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract vectors (unpredicated)") {
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.b, z29.b, z28.b");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.h, z29.h, z28.h");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.s, z29.s, z28.s");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.d, z29.d, z28.d");
  // TEST_SINGLE(add(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.q, z29.q, z28.q");

  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.b, z29.b, z28.b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.h, z29.h, z28.h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.s, z29.s, z28.s");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.d, z29.d, z28.d");
  // TEST_SINGLE(sub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.q, z29.q, z28.q");

  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.b, z29.b, z28.b");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.h, z29.h, z28.h");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.s, z29.s, z28.s");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.d, z29.d, z28.d");
  // TEST_SINGLE(sqadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.q, z29.q, z28.q");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.b, z29.b, z28.b");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.h, z29.h, z28.h");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.s, z29.s, z28.s");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.d, z29.d, z28.d");
  // TEST_SINGLE(uqadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.q, z29.q, z28.q");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.b, z29.b, z28.b");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.h, z29.h, z28.h");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.s, z29.s, z28.s");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.d, z29.d, z28.d");
  // TEST_SINGLE(sqsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.q, z29.q, z28.q");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.b, z29.b, z28.b");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.h, z29.h, z28.h");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.s, z29.s, z28.s");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.d, z29.d, z28.d");
  // TEST_SINGLE(uqsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.q, z29.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE address generation") {
  TEST_SINGLE(adr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z31), "adr z30.s, [z29.s, z31.s]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31), "adr z30.d, [z29.d, z31.d]");

  TEST_SINGLE(adr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 1), "adr z30.s, [z29.s, z31.s, lsl #1]");
  TEST_SINGLE(adr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 2), "adr z30.s, [z29.s, z31.s, lsl #2]");
  TEST_SINGLE(adr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 3), "adr z30.s, [z29.s, z31.s, lsl #3]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 1), "adr z30.d, [z29.d, z31.d, lsl #1]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 2), "adr z30.d, [z29.d, z31.d, lsl #2]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_LSL, 3), "adr z30.d, [z29.d, z31.d, lsl #3]");

  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_UXTW, 0), "adr z30.d, [z29.d, z31.d, uxtw]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_UXTW, 1), "adr z30.d, [z29.d, z31.d, uxtw #1]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_UXTW, 2), "adr z30.d, [z29.d, z31.d, uxtw #2]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_UXTW, 3), "adr z30.d, [z29.d, z31.d, uxtw #3]");

  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_SXTW, 0), "adr z30.d, [z29.d, z31.d, sxtw]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_SXTW, 1), "adr z30.d, [z29.d, z31.d, sxtw #1]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_SXTW, 2), "adr z30.d, [z29.d, z31.d, sxtw #2]");
  TEST_SINGLE(adr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z31, SVEModType::MOD_SXTW, 3), "adr z30.d, [z29.d, z31.d, sxtw #3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE table lookup (three sources)") {
  TEST_SINGLE(tbl(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.b, {z29.b}, z28.b");
  TEST_SINGLE(tbl(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.h, {z29.h}, z28.h");
  TEST_SINGLE(tbl(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.s, {z29.s}, z28.s");
  TEST_SINGLE(tbl(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.d, {z29.d}, z28.d");
  // TEST_SINGLE(tbl(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.q, {z29.q}, z28.q");

  TEST_SINGLE(tbl(SubRegSize::i8Bit, ZReg::z31, ZReg::z29, ZReg::z30, ZReg::z28), "tbl z31.b, {z29.b, z30.b}, z28.b");
  TEST_SINGLE(tbl(SubRegSize::i16Bit, ZReg::z31, ZReg::z29, ZReg::z30, ZReg::z28), "tbl z31.h, {z29.h, z30.h}, z28.h");
  TEST_SINGLE(tbl(SubRegSize::i32Bit, ZReg::z31, ZReg::z29, ZReg::z30, ZReg::z28), "tbl z31.s, {z29.s, z30.s}, z28.s");
  TEST_SINGLE(tbl(SubRegSize::i64Bit, ZReg::z31, ZReg::z29, ZReg::z30, ZReg::z28), "tbl z31.d, {z29.d, z30.d}, z28.d");

  TEST_SINGLE(tbx(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.b, z29.b, z28.b");
  TEST_SINGLE(tbx(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.h, z29.h, z28.h");
  TEST_SINGLE(tbx(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.s, z29.s, z28.s");
  TEST_SINGLE(tbx(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.d, z29.d, z28.d");
  // TEST_SINGLE(tbx(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.q, z29.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE permute vector elements") {
  TEST_SINGLE(zip1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.b, z29.b, z28.b");
  TEST_SINGLE(zip1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.h, z29.h, z28.h");
  TEST_SINGLE(zip1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.s, z29.s, z28.s");
  TEST_SINGLE(zip1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.d, z29.d, z28.d");

  TEST_SINGLE(zip2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.b, z29.b, z28.b");
  TEST_SINGLE(zip2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.h, z29.h, z28.h");
  TEST_SINGLE(zip2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.s, z29.s, z28.s");
  TEST_SINGLE(zip2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.d, z29.d, z28.d");

  TEST_SINGLE(uzp1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.b, z29.b, z28.b");
  TEST_SINGLE(uzp1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.h, z29.h, z28.h");
  TEST_SINGLE(uzp1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.s, z29.s, z28.s");
  TEST_SINGLE(uzp1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.d, z29.d, z28.d");

  TEST_SINGLE(uzp2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.b, z29.b, z28.b");
  TEST_SINGLE(uzp2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.h, z29.h, z28.h");
  TEST_SINGLE(uzp2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.s, z29.s, z28.s");
  TEST_SINGLE(uzp2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.d, z29.d, z28.d");

  TEST_SINGLE(trn1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.b, z29.b, z28.b");
  TEST_SINGLE(trn1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.h, z29.h, z28.h");
  TEST_SINGLE(trn1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.s, z29.s, z28.s");
  TEST_SINGLE(trn1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.d, z29.d, z28.d");

  TEST_SINGLE(trn2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.b, z29.b, z28.b");
  TEST_SINGLE(trn2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.h, z29.h, z28.h");
  TEST_SINGLE(trn2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.s, z29.s, z28.s");
  TEST_SINGLE(trn2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with unsigned immediate") {
  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmplo(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmplo(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmplo(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmplo(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmplo(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmplo(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmplo(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmplo(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmpls(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmpls(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmpls(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmpls(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmpls(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmpls(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmpls(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmpls(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.d, p5/z, z30.d, #127");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with signed immediate") {
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpeq p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpeq p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpeq p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpeq p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpgt p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpgt p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpgt p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpgt p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpge p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpge p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpge p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpge p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmplt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmplt p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmplt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmplt p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmplt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmplt p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmplt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmplt p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmple(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmple(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmple(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmple(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmple(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmple p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmple(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmple p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmple(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmple p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmple(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmple p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmpne(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpne p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpne(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpne p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpne(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpne p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpne(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpne p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpne(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpne p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpne(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpne p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpne(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpne p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpne(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15), "cmpne p6.d, p5/z, z30.d, #15");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate logical operations") {
  TEST_SINGLE(and_(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "and p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(ands(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "ands p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(mov(PReg::p6, PReg::p5.Merging(), PReg::p4), "mov p6.b, p5/m, p4.b");
  TEST_SINGLE(mov(PReg::p6, PReg::p5.Zeroing(), PReg::p4), "mov p6.b, p5/z, p4.b");
  TEST_SINGLE(movs(PReg::p6, PReg::p5.Zeroing(), PReg::p4), "movs p6.b, p5/z, p4.b");

  TEST_SINGLE(bic(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "bic p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(bics(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "bics p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(eor(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "eor p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(eors(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "eors p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(not_(PReg::p6, PReg::p5.Zeroing(), PReg::p4), "not p6.b, p5/z, p4.b");

  TEST_SINGLE(sel(PReg::p6, PReg::p5, PReg::p4, PReg::p3), "sel p6.b, p5, p4.b, p3.b");
  TEST_SINGLE(orr(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "orr p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(mov(PReg::p6, PReg::p5), "mov p6.b, p5.b");
  TEST_SINGLE(orn(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "orn p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(nor(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "nor p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(nand(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "nand p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(orrs(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "orrs p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(movs(PReg::p6, PReg::p5), "movs p6.b, p5.b");

  TEST_SINGLE(orns(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "orns p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(nors(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "nors p6.b, p5/z, p4.b, p3.b");
  TEST_SINGLE(nands(PReg::p6, PReg::p5.Zeroing(), PReg::p4, PReg::p3), "nands p6.b, p5/z, p4.b, p3.b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast predicate element") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer clamp") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 character match") {
  TEST_SINGLE(match(SubRegSize::i8Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29), "match p8.b, p6/z, z30.b, z29.b");
  TEST_SINGLE(match(SubRegSize::i16Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29), "match p8.h, p6/z, z30.h, z29.h");

  TEST_SINGLE(nmatch(SubRegSize::i8Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29), "nmatch p8.b, p6/z, z30.b, z29.b");
  TEST_SINGLE(nmatch(SubRegSize::i16Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29), "nmatch p8.h, p6/z, z30.h, z29.h");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point convert precision odd elements") {
  TEST_SINGLE(fcvtxnt(ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtxnt z30.s, p6/m, z29.d");
  TEST_SINGLE(fcvtnt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.h, p6/m, z29.s");
  TEST_SINGLE(fcvtnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.s, p6/m, z29.d");
  // TEST_SINGLE(fcvtnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.d, p6/m, z29.d");

  // TEST_SINGLE(fcvtlt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.h, p6/m, z29.b");
  TEST_SINGLE(fcvtlt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtlt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.d, p6/m, z29.s");


  // void fcvtxnt(ARMEmitter::ZRegister zd, ARMEmitter::PRegister pg, ARMEmitter::ZRegister zn) {
  /////< Size is destination size
  // void fcvtnt(ARMEmitter::SubRegSize size, ARMEmitter::ZRegister zd, ARMEmitter::PRegister pg, ARMEmitter::ZRegister zn) {
  /////< Size is destination size
  // void fcvtlt(ARMEmitter::SubRegSize size, ARMEmitter::ZRegister zd, ARMEmitter::PRegister pg, ARMEmitter::ZRegister zn) {

  // XXX: BFCVTNT
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 floating-point pairwise operations") {
  // TEST_SINGLE(faddp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "faddp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(faddp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "faddp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(faddp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "faddp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(faddp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "faddp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(faddp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "faddp z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmaxnmp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmaxnmp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmaxnmp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnmp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmaxnmp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnmp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmaxnmp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnmp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmaxnmp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnmp z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fminnmp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fminnmp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fminnmp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnmp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fminnmp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnmp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fminnmp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnmp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fminnmp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnmp z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmax z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmax(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmin z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmin(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point complex add") {
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90), "fcadd z30.h, p6/m, "
                                                                                                                   "z30.h, z28.h, #90");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90), "fcadd z30.s, p6/m, "
                                                                                                                   "z30.s, z28.s, #90");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90), "fcadd z30.d, p6/m, "
                                                                                                                   "z30.d, z28.d, #90");

  TEST_SINGLE(fcadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270), "fcadd z30.h, p6/m, "
                                                                                                                    "z30.h, z28.h, #270");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270), "fcadd z30.s, p6/m, "
                                                                                                                    "z30.s, z28.s, #270");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270), "fcadd z30.d, p6/m, "
                                                                                                                    "z30.d, z28.d, #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add (vector)") {
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0), "fcmla z30.h, p6/m, "
                                                                                                                  "z10.h, z28.h, #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0), "fcmla z30.s, p6/m, "
                                                                                                                  "z10.s, z28.s, #0");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0), "fcmla z30.d, p6/m, "
                                                                                                                  "z10.d, z28.d, #0");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90), "fcmla z30.h, p6/m, "
                                                                                                                   "z10.h, z28.h, #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90), "fcmla z30.s, p6/m, "
                                                                                                                   "z10.s, z28.s, #90");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90), "fcmla z30.d, p6/m, "
                                                                                                                   "z10.d, z28.d, #90");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180), "fcmla z30.h, p6/m, "
                                                                                                                    "z10.h, z28.h, #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180), "fcmla z30.s, p6/m, "
                                                                                                                    "z10.s, z28.s, #180");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180), "fcmla z30.d, p6/m, "
                                                                                                                    "z10.d, z28.d, #180");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270), "fcmla z30.h, p6/m, "
                                                                                                                    "z10.h, z28.h, #270");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270), "fcmla z30.s, p6/m, "
                                                                                                                    "z10.s, z28.s, #270");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270), "fcmla z30.d, p6/m, "
                                                                                                                    "z10.d, z28.d, #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add (indexed)") {
  TEST_SINGLE(fmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmla z30.h, z29.h, z7.h[7]");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmla z30.s, z29.s, z7.s[3]");
  TEST_SINGLE(fmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z15, 1), "fmla z30.d, z29.d, z15.d[1]");

  TEST_SINGLE(fmls(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmls z30.h, z29.h, z7.h[7]");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmls z30.s, z29.s, z7.s[3]");
  TEST_SINGLE(fmls(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z15, 1), "fmls z30.d, z29.d, z15.d[1]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point complex multiply-add (indexed)") {
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 0, Rotation::ROTATE_0), "fcmla z30.h, z10.h, z7.h[0], #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 0, Rotation::ROTATE_0), "fcmla z30.s, z10.s, z15.s[0], #0");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 1, Rotation::ROTATE_90), "fcmla z30.h, z10.h, z7.h[1], #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_90), "fcmla z30.s, z10.s, z15.s[1], #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_180), "fcmla z30.s, z10.s, z15.s[1], #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_270), "fcmla z30.s, z10.s, z15.s[1], #270");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 2, Rotation::ROTATE_180), "fcmla z30.h, z10.h, z7.h[2], #180");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 3, Rotation::ROTATE_270), "fcmla z30.h, z10.h, z7.h[3], #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply (indexed)") {
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmul z30.h, z29.h, z7.h[7]");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmul z30.s, z29.s, z7.s[3]");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z15, 1), "fmul z30.d, z29.d, z15.d[1]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating point matrix multiply accumulate") {
  TEST_SINGLE(fmmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmmla z30.s, z29.s, z28.s");
  TEST_SINGLE(fmmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmmla z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point compare vectors") {
  TEST_SINGLE(fcmeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmeq p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmeq p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(fcmeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmeq p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(fcmgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmgt p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmgt p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(fcmgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmgt p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(fcmge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmge p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(fcmge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmge p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(fcmge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmge p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(fcmne(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmne p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(fcmne(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmne p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(fcmne(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmne p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(fcmuo(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmuo p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(fcmuo(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmuo p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(fcmuo(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "fcmuo p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(facge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(facge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(facge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(facgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(facgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(facgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(facle(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.h, p5/z, z29.h, z30.h");
  TEST_SINGLE(facle(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.s, p5/z, z29.s, z30.s");
  TEST_SINGLE(facle(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facge p6.d, p5/z, z29.d, z30.d");

  TEST_SINGLE(faclt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.h, p5/z, z29.h, z30.h");
  TEST_SINGLE(faclt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.s, p5/z, z29.s, z30.s");
  TEST_SINGLE(faclt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "facgt p6.d, p5/z, z29.d, z30.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic (unpredicated)") {
  // TEST_SINGLE(fadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fadd z30.b, z29.b, z28.b");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fadd z30.h, z29.h, z28.h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fadd z30.s, z29.s, z28.s");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fadd z30.d, z29.d, z28.d");
  // TEST_SINGLE(fadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fadd z30.q, z29.q, z28.q");

  // TEST_SINGLE(fsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fsub z30.b, z29.b, z28.b");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fsub z30.h, z29.h, z28.h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fsub z30.s, z29.s, z28.s");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fsub z30.d, z29.d, z28.d");
  // TEST_SINGLE(fsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fsub z30.q, z29.q, z28.q");

  // TEST_SINGLE(fmul(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fmul z30.b, z29.b, z28.b");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmul z30.h, z29.h, z28.h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmul z30.s, z29.s, z28.s");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmul z30.d, z29.d, z28.d");
  // TEST_SINGLE(fmul(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmul z30.q, z29.q, z28.q");

  // TEST_SINGLE(ftsmul(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "ftsmul z30.b, z29.b, z28.b");
  TEST_SINGLE(ftsmul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftsmul z30.h, z29.h, z28.h");
  TEST_SINGLE(ftsmul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftsmul z30.s, z29.s, z28.s");
  TEST_SINGLE(ftsmul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftsmul z30.d, z29.d, z28.d");
  // TEST_SINGLE(ftsmul(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftsmul z30.q, z29.q, z28.q");

  // TEST_SINGLE(frecps(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "frecps z30.b, z29.b, z28.b");
  TEST_SINGLE(frecps(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frecps z30.h, z29.h, z28.h");
  TEST_SINGLE(frecps(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frecps z30.s, z29.s, z28.s");
  TEST_SINGLE(frecps(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frecps z30.d, z29.d, z28.d");
  // TEST_SINGLE(frecps(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frecps z30.q, z29.q, z28.q");

  // TEST_SINGLE(frsqrts(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "frsqrts z30.b, z29.b, z28.b");
  TEST_SINGLE(frsqrts(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frsqrts z30.h, z29.h, z28.h");
  TEST_SINGLE(frsqrts(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frsqrts z30.s, z29.s, z28.s");
  TEST_SINGLE(frsqrts(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frsqrts z30.d, z29.d, z28.d");
  // TEST_SINGLE(frsqrts(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frsqrts z30.q, z29.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point recursive reduction") {
  TEST_SINGLE(faddv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z28), "faddv h30, p7, z28.h");
  TEST_SINGLE(faddv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z28), "faddv s30, p7, z28.s");
  TEST_SINGLE(faddv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z28), "faddv d30, p7, z28.d");

  TEST_SINGLE(fmaxnmv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxnmv h30, p7, z28.h");
  TEST_SINGLE(fmaxnmv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxnmv s30, p7, z28.s");
  TEST_SINGLE(fmaxnmv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxnmv d30, p7, z28.d");

  TEST_SINGLE(fminnmv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z28), "fminnmv h30, p7, z28.h");
  TEST_SINGLE(fminnmv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z28), "fminnmv s30, p7, z28.s");
  TEST_SINGLE(fminnmv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z28), "fminnmv d30, p7, z28.d");

  TEST_SINGLE(fmaxv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxv h30, p7, z28.h");
  TEST_SINGLE(fmaxv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxv s30, p7, z28.s");
  TEST_SINGLE(fmaxv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z28), "fmaxv d30, p7, z28.d");

  TEST_SINGLE(fminv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z28), "fminv h30, p7, z28.h");
  TEST_SINGLE(fminv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z28), "fminv s30, p7, z28.s");
  TEST_SINGLE(fminv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z28), "fminv d30, p7, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply-accumulate writing addend (predicated)") {
  TEST_SINGLE(mla(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(mla(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(mla(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(mla(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.d, p7/m, z28.d, z29.d");

  TEST_SINGLE(mls(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(mls(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(mls(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(mls(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.d, p7/m, z28.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply-add writing multiplicand (predicated)") {
  TEST_SINGLE(mad(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mad z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(mad(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mad z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(mad(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mad z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(mad(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mad z30.d, p7/m, z28.d, z29.d");

  TEST_SINGLE(msb(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "msb z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(msb(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "msb z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(msb(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "msb z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(msb(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "msb z30.d, p7/m, z28.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract vectors (predicated)") {
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "add z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "add z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "add z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "add z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sub z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sub z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sub z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sub z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(subr(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "subr z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "subr z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "subr z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "subr z30.d, p7/m, z30.d, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer min/max/difference (predicated)") {
  TEST_SINGLE(smax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smax z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(smax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smax z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(smax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smax z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(smax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smax z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(umax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umax z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(umax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umax z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(umax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umax z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(umax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umax z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(smin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smin z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(smin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smin z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(smin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smin z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(smin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "smin z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(umin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umin z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(umin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umin z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(umin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umin z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(umin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "umin z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(sabd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sabd z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(sabd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sabd z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(sabd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sabd z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(sabd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sabd z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(uabd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uabd z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(uabd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uabd z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(uabd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uabd z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(uabd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uabd z30.d, p6/m, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply vectors (predicated)") {
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "mul z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "mul z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "mul z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "mul z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(smulh(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "smulh z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(smulh(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "smulh z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(smulh(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "smulh z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(smulh(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "smulh z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(umulh(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "umulh z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(umulh(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "umulh z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(umulh(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "umulh z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(umulh(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "umulh z30.d, p7/m, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer divide vectors (predicated)") {
  TEST_SINGLE(sdiv(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "sdiv z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(sdiv(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "sdiv z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(udiv(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "udiv z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(udiv(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "udiv z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(sdivr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "sdivr z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(sdivr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "sdivr z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(udivr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "udivr z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(udivr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "udivr z30.d, p7/m, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical operations (predicated)") {
  TEST_SINGLE(orr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(orr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(orr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(orr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.d, p6/m, z30.d, z29.d");
  // TEST_SINGLE(orr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(eor(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(eor(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(eor(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(eor(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.d, p6/m, z30.d, z29.d");
  // TEST_SINGLE(eor(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(and_(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(and_(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(and_(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(and_(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.d, p6/m, z30.d, z29.d");
  // TEST_SINGLE(and_(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(bic(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(bic(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(bic(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(bic(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.d, p6/m, z30.d, z29.d");
  // TEST_SINGLE(bic(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.q, p6/m, z30.q, z29.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add reduction (predicated)") {
  TEST_SINGLE(saddv(SubRegSize::i8Bit, DReg::d30, PReg::p7, ZReg::z29), "saddv d30, p7, z29.b");
  TEST_SINGLE(saddv(SubRegSize::i16Bit, DReg::d30, PReg::p7, ZReg::z29), "saddv d30, p7, z29.h");
  TEST_SINGLE(saddv(SubRegSize::i32Bit, DReg::d30, PReg::p7, ZReg::z29), "saddv d30, p7, z29.s");

  TEST_SINGLE(uaddv(SubRegSize::i8Bit, DReg::d30, PReg::p7, ZReg::z29), "uaddv d30, p7, z29.b");
  TEST_SINGLE(uaddv(SubRegSize::i16Bit, DReg::d30, PReg::p7, ZReg::z29), "uaddv d30, p7, z29.h");
  TEST_SINGLE(uaddv(SubRegSize::i32Bit, DReg::d30, PReg::p7, ZReg::z29), "uaddv d30, p7, z29.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer min/max reduction (predicated)") {
  TEST_SINGLE(smaxv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29), "smaxv b30, p6, z29.b");
  TEST_SINGLE(smaxv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29), "smaxv h30, p6, z29.h");
  TEST_SINGLE(smaxv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29), "smaxv s30, p6, z29.s");
  TEST_SINGLE(smaxv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29), "smaxv d30, p6, z29.d");

  TEST_SINGLE(umaxv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29), "umaxv b30, p6, z29.b");
  TEST_SINGLE(umaxv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29), "umaxv h30, p6, z29.h");
  TEST_SINGLE(umaxv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29), "umaxv s30, p6, z29.s");
  TEST_SINGLE(umaxv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29), "umaxv d30, p6, z29.d");

  TEST_SINGLE(sminv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29), "sminv b30, p6, z29.b");
  TEST_SINGLE(sminv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29), "sminv h30, p6, z29.h");
  TEST_SINGLE(sminv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29), "sminv s30, p6, z29.s");
  TEST_SINGLE(sminv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29), "sminv d30, p6, z29.d");

  TEST_SINGLE(uminv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29), "uminv b30, p6, z29.b");
  TEST_SINGLE(uminv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29), "uminv h30, p6, z29.h");
  TEST_SINGLE(uminv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29), "uminv s30, p6, z29.s");
  TEST_SINGLE(uminv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29), "uminv d30, p6, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE constructive prefix (predicated)") {
  TEST_SINGLE(movprfx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.b, p6/m, z29.b");
  TEST_SINGLE(movprfx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.h, p6/m, z29.h");
  TEST_SINGLE(movprfx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.s, p6/m, z29.s");
  TEST_SINGLE(movprfx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.d, p6/m, z29.d");
  // TEST_SINGLE(movprfx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.q, p6/m, z29.q");
  TEST_SINGLE(movprfx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.b, p6/z, z29.b");
  TEST_SINGLE(movprfx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.h, p6/z, z29.h");
  TEST_SINGLE(movprfx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.s, p6/z, z29.s");
  TEST_SINGLE(movprfx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.d, p6/z, z29.d");
  // TEST_SINGLE(movprfx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.q, p6/z, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical reduction (predicated)") {
  TEST_SINGLE(orv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29), "orv b30, p7, z29.b");
  TEST_SINGLE(orv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z29), "orv h30, p7, z29.h");
  TEST_SINGLE(orv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z29), "orv s30, p7, z29.s");
  TEST_SINGLE(orv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z29), "orv d30, p7, z29.d");

  TEST_SINGLE(eorv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv b30, p7, z29.b");
  TEST_SINGLE(eorv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv h30, p7, z29.h");
  TEST_SINGLE(eorv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv s30, p7, z29.s");
  TEST_SINGLE(eorv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv d30, p7, z29.d");

  TEST_SINGLE(andv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29), "andv b30, p7, z29.b");
  TEST_SINGLE(andv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z29), "andv h30, p7, z29.h");
  TEST_SINGLE(andv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z29), "andv s30, p7, z29.s");
  TEST_SINGLE(andv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z29), "andv d30, p7, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by immediate (predicated)") {
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asr z30.b, p6/m, z30.b, #1");
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 8), "asr z30.b, p6/m, z30.b, #8");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asr z30.h, p6/m, z30.h, #1");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 16), "asr z30.h, p6/m, z30.h, #16");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asr z30.s, p6/m, z30.s, #1");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 32), "asr z30.s, p6/m, z30.s, #32");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asr z30.d, p6/m, z30.d, #1");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 64), "asr z30.d, p6/m, z30.d, #64");

  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "lsr z30.b, p6/m, z30.b, #1");
  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 8), "lsr z30.b, p6/m, z30.b, #8");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "lsr z30.h, p6/m, z30.h, #1");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 16), "lsr z30.h, p6/m, z30.h, #16");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "lsr z30.s, p6/m, z30.s, #1");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 32), "lsr z30.s, p6/m, z30.s, #32");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "lsr z30.d, p6/m, z30.d, #1");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 64), "lsr z30.d, p6/m, z30.d, #64");

  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "lsl z30.b, p6/m, z30.b, #0");
  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 7), "lsl z30.b, p6/m, z30.b, #7");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "lsl z30.h, p6/m, z30.h, #0");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 15), "lsl z30.h, p6/m, z30.h, #15");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "lsl z30.s, p6/m, z30.s, #0");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 31), "lsl z30.s, p6/m, z30.s, #31");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "lsl z30.d, p6/m, z30.d, #0");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 63), "lsl z30.d, p6/m, z30.d, #63");

  TEST_SINGLE(asrd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asrd z30.b, p6/m, z30.b, #1");
  TEST_SINGLE(asrd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 8), "asrd z30.b, p6/m, z30.b, #8");
  TEST_SINGLE(asrd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asrd z30.h, p6/m, z30.h, #1");
  TEST_SINGLE(asrd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 16), "asrd z30.h, p6/m, z30.h, #16");
  TEST_SINGLE(asrd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asrd z30.s, p6/m, z30.s, #1");
  TEST_SINGLE(asrd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 32), "asrd z30.s, p6/m, z30.s, #32");
  TEST_SINGLE(asrd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "asrd z30.d, p6/m, z30.d, #1");
  TEST_SINGLE(asrd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 64), "asrd z30.d, p6/m, z30.d, #64");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshl z30.b, p6/m, z30.b, #0");
  TEST_SINGLE(sqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 7), "sqshl z30.b, p6/m, z30.b, #7");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshl z30.h, p6/m, z30.h, #0");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 15), "sqshl z30.h, p6/m, z30.h, #15");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshl z30.s, p6/m, z30.s, #0");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 31), "sqshl z30.s, p6/m, z30.s, #31");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshl z30.d, p6/m, z30.d, #0");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 63), "sqshl z30.d, p6/m, z30.d, #63");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "uqshl z30.b, p6/m, z30.b, #0");
  TEST_SINGLE(uqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 7), "uqshl z30.b, p6/m, z30.b, #7");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "uqshl z30.h, p6/m, z30.h, #0");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 15), "uqshl z30.h, p6/m, z30.h, #15");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "uqshl z30.s, p6/m, z30.s, #0");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 31), "uqshl z30.s, p6/m, z30.s, #31");
  TEST_SINGLE(uqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "uqshl z30.d, p6/m, z30.d, #0");
  TEST_SINGLE(uqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 63), "uqshl z30.d, p6/m, z30.d, #63");

  TEST_SINGLE(srshr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "srshr z30.b, p6/m, z30.b, #1");
  TEST_SINGLE(srshr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 8), "srshr z30.b, p6/m, z30.b, #8");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "srshr z30.h, p6/m, z30.h, #1");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 16), "srshr z30.h, p6/m, z30.h, #16");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "srshr z30.s, p6/m, z30.s, #1");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 32), "srshr z30.s, p6/m, z30.s, #32");
  TEST_SINGLE(srshr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "srshr z30.d, p6/m, z30.d, #1");
  TEST_SINGLE(srshr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 64), "srshr z30.d, p6/m, z30.d, #64");

  TEST_SINGLE(urshr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "urshr z30.b, p6/m, z30.b, #1");
  TEST_SINGLE(urshr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 8), "urshr z30.b, p6/m, z30.b, #8");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "urshr z30.h, p6/m, z30.h, #1");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 16), "urshr z30.h, p6/m, z30.h, #16");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "urshr z30.s, p6/m, z30.s, #1");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 32), "urshr z30.s, p6/m, z30.s, #32");
  TEST_SINGLE(urshr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 1), "urshr z30.d, p6/m, z30.d, #1");
  TEST_SINGLE(urshr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 64), "urshr z30.d, p6/m, z30.d, #64");

  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshlu z30.b, p6/m, z30.b, #0");
  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 7), "sqshlu z30.b, p6/m, z30.b, #7");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshlu z30.h, p6/m, z30.h, #0");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 15), "sqshlu z30.h, p6/m, z30.h, #15");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshlu z30.s, p6/m, z30.s, #0");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 31), "sqshlu z30.s, p6/m, z30.s, #31");
  TEST_SINGLE(sqshlu(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 0), "sqshlu z30.d, p6/m, z30.d, #0");
  TEST_SINGLE(sqshlu(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, 63), "sqshlu z30.d, p6/m, z30.d, #63");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by vector (predicated)") {
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(asrr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(asrr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(asrr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(asrr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsrr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsrr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsrr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsrr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lslr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lslr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lslr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lslr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.d, p6/m, z30.d, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by wide elements (predicated)") {
  TEST_SINGLE(asr_wide(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "asr z30.b, p7/m, z30.b, z29.d");
  TEST_SINGLE(asr_wide(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "asr z30.h, p7/m, z30.h, z29.d");
  TEST_SINGLE(asr_wide(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "asr z30.s, p7/m, z30.s, z29.d");

  TEST_SINGLE(lsr_wide(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsr z30.b, p7/m, z30.b, z29.d");
  TEST_SINGLE(lsr_wide(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsr z30.h, p7/m, z30.h, z29.d");
  TEST_SINGLE(lsr_wide(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsr z30.s, p7/m, z30.s, z29.d");

  TEST_SINGLE(lsl_wide(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsl z30.b, p7/m, z30.b, z29.d");
  TEST_SINGLE(lsl_wide(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsl z30.h, p7/m, z30.h, z29.d");
  TEST_SINGLE(lsl_wide(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29), "lsl z30.s, p7/m, z30.s, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer unary operations (predicated)") {
  // TEST_SINGLE(sxtb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxtb z30.b, p6/m, z29.b");
  TEST_SINGLE(sxtb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtb z30.h, p6/m, z29.h");
  TEST_SINGLE(sxtb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtb z30.s, p6/m, z29.s");
  TEST_SINGLE(sxtb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtb z30.d, p6/m, z29.d");
  // TEST_SINGLE(sxtb(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtb z30.q, p6/m, z29.q");

  // TEST_SINGLE(uxtb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxtb z30.b, p6/m, z29.b");
  TEST_SINGLE(uxtb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtb z30.h, p6/m, z29.h");
  TEST_SINGLE(uxtb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtb z30.s, p6/m, z29.s");
  TEST_SINGLE(uxtb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtb z30.d, p6/m, z29.d");
  // TEST_SINGLE(uxtb(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtb z30.q, p6/m, z29.q");

  // TEST_SINGLE(sxth(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxth z30.b, p6/m, z29.b");
  // TEST_SINGLE(sxth(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxth z30.h, p6/m, z29.h");
  TEST_SINGLE(sxth(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxth z30.s, p6/m, z29.s");
  TEST_SINGLE(sxth(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxth z30.d, p6/m, z29.d");
  // TEST_SINGLE(sxth(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxth z30.q, p6/m, z29.q");

  // TEST_SINGLE(uxth(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxth z30.b, p6/m, z29.b");
  // TEST_SINGLE(uxth(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxth z30.h, p6/m, z29.h");
  TEST_SINGLE(uxth(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxth z30.s, p6/m, z29.s");
  TEST_SINGLE(uxth(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxth z30.d, p6/m, z29.d");
  // TEST_SINGLE(uxth(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxth z30.q, p6/m, z29.q");

  // TEST_SINGLE(sxtw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxtw z30.b, p6/m, z29.b");
  // TEST_SINGLE(sxtw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtw z30.h, p6/m, z29.h");
  // TEST_SINGLE(sxtw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtw z30.s, p6/m, z29.s");
  TEST_SINGLE(sxtw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtw z30.d, p6/m, z29.d");
  // TEST_SINGLE(sxtw(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtw z30.q, p6/m, z29.q");

  // TEST_SINGLE(uxtw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxtw z30.b, p6/m, z29.b");
  // TEST_SINGLE(uxtw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtw z30.h, p6/m, z29.h");
  // TEST_SINGLE(uxtw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtw z30.s, p6/m, z29.s");
  TEST_SINGLE(uxtw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtw z30.d, p6/m, z29.d");
  // TEST_SINGLE(uxtw(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtw z30.q, p6/m, z29.q");

  TEST_SINGLE(abs(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.b, p6/m, z29.b");
  TEST_SINGLE(abs(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.h, p6/m, z29.h");
  TEST_SINGLE(abs(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.s, p6/m, z29.s");
  TEST_SINGLE(abs(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.d, p6/m, z29.d");
  // TEST_SINGLE(abs(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.q, p6/m, z29.q");

  TEST_SINGLE(neg(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.b, p6/m, z29.b");
  TEST_SINGLE(neg(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.h, p6/m, z29.h");
  TEST_SINGLE(neg(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.s, p6/m, z29.s");
  TEST_SINGLE(neg(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.d, p6/m, z29.d");
  // TEST_SINGLE(neg(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise unary operations (predicated)") {
  TEST_SINGLE(cls(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.b, p6/m, z29.b");
  TEST_SINGLE(cls(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.h, p6/m, z29.h");
  TEST_SINGLE(cls(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.s, p6/m, z29.s");
  TEST_SINGLE(cls(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.d, p6/m, z29.d");
  // TEST_SINGLE(cls(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.q, p6/m, z29.q");

  TEST_SINGLE(clz(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.b, p6/m, z29.b");
  TEST_SINGLE(clz(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.h, p6/m, z29.h");
  TEST_SINGLE(clz(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.s, p6/m, z29.s");
  TEST_SINGLE(clz(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.d, p6/m, z29.d");
  // TEST_SINGLE(clz(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.q, p6/m, z29.q");

  TEST_SINGLE(cnt(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.b, p6/m, z29.b");
  TEST_SINGLE(cnt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.h, p6/m, z29.h");
  TEST_SINGLE(cnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.s, p6/m, z29.s");
  TEST_SINGLE(cnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.d, p6/m, z29.d");
  // TEST_SINGLE(cnt(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.q, p6/m, z29.q");

  TEST_SINGLE(cnot(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.b, p6/m, z29.b");
  TEST_SINGLE(cnot(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.h, p6/m, z29.h");
  TEST_SINGLE(cnot(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.s, p6/m, z29.s");
  TEST_SINGLE(cnot(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.d, p6/m, z29.d");
  // TEST_SINGLE(cnot(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.q, p6/m, z29.q");

  // TEST_SINGLE(fabs(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "fabs z30.b, p6/m, z29.b");
  TEST_SINGLE(fabs(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fabs z30.h, p6/m, z29.h");
  TEST_SINGLE(fabs(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fabs z30.s, p6/m, z29.s");
  TEST_SINGLE(fabs(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fabs z30.d, p6/m, z29.d");
  // TEST_SINGLE(fabs(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fabs z30.q, p6/m, z29.q");

  // TEST_SINGLE(fneg(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "fneg z30.b, p6/m, z29.b");
  TEST_SINGLE(fneg(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fneg z30.h, p6/m, z29.h");
  TEST_SINGLE(fneg(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fneg z30.s, p6/m, z29.s");
  TEST_SINGLE(fneg(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fneg z30.d, p6/m, z29.d");
  // TEST_SINGLE(fneg(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fneg z30.q, p6/m, z29.q");

  TEST_SINGLE(not_(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.b, p6/m, z29.b");
  TEST_SINGLE(not_(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.h, p6/m, z29.h");
  TEST_SINGLE(not_(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.s, p6/m, z29.s");
  TEST_SINGLE(not_(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.d, p6/m, z29.d");
  // TEST_SINGLE(not_(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical operations (unpredicated)") {
  TEST_SINGLE(and_(ZReg::z30, ZReg::z29, ZReg::z28), "and z30.d, z29.d, z28.d");
  TEST_SINGLE(orr(ZReg::z30, ZReg::z29, ZReg::z28), "orr z30.d, z29.d, z28.d");
  TEST_SINGLE(mov(ZReg::z30, ZReg::z29), "mov z30.d, z29.d");
  TEST_SINGLE(eor(ZReg::z30, ZReg::z29, ZReg::z28), "eor z30.d, z29.d, z28.d");
  TEST_SINGLE(bic(ZReg::z30, ZReg::z29, ZReg::z28), "bic z30.d, z29.d, z28.d");

  TEST_SINGLE(xar(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "xar z30.b, z30.b, z29.b, #1");
  TEST_SINGLE(xar(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "xar z30.b, z30.b, z29.b, #8");
  TEST_SINGLE(xar(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "xar z30.h, z30.h, z29.h, #1");
  TEST_SINGLE(xar(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "xar z30.h, z30.h, z29.h, #16");
  TEST_SINGLE(xar(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "xar z30.s, z30.s, z29.s, #1");
  TEST_SINGLE(xar(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "xar z30.s, z30.s, z29.s, #32");
  TEST_SINGLE(xar(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "xar z30.d, z30.d, z29.d, #1");
  TEST_SINGLE(xar(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "xar z30.d, z30.d, z29.d, #64");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise ternary operations") {
  TEST_SINGLE(eor3(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "eor3 z30.d, z30.d, z28.d, z29.d");
  TEST_SINGLE(bsl(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "bsl z30.d, z30.d, z28.d, z29.d");
  TEST_SINGLE(bcax(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "bcax z30.d, z30.d, z28.d, z29.d");
  TEST_SINGLE(bsl1n(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "bsl1n z30.d, z30.d, z28.d, z29.d");
  TEST_SINGLE(bsl2n(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "bsl2n z30.d, z30.d, z28.d, z29.d");
  TEST_SINGLE(nbsl(ZReg::z30, ZReg::z30, ZReg::z28, ZReg::z29), "nbsl z30.d, z30.d, z28.d, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Index Generation") {
  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, -16, -16), "index z30.b, #-16, #-16");
  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, -16, 15), "index z30.b, #-16, #15");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, -16, -16), "index z30.h, #-16, #-16");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, -16, 15), "index z30.h, #-16, #15");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, -16, -16), "index z30.s, #-16, #-16");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, -16, 15), "index z30.s, #-16, #15");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, -16, -16), "index z30.d, #-16, #-16");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, -16, 15), "index z30.d, #-16, #15");

  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, WReg::w29, -16), "index z30.b, w29, #-16");
  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, WReg::w29, 15), "index z30.b, w29, #15");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, WReg::w29, -16), "index z30.h, w29, #-16");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, WReg::w29, 15), "index z30.h, w29, #15");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, WReg::w29, -16), "index z30.s, w29, #-16");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, WReg::w29, 15), "index z30.s, w29, #15");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, XReg::x29, -16), "index z30.d, x29, #-16");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, XReg::x29, 15), "index z30.d, x29, #15");

  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, -16, WReg::w29), "index z30.b, #-16, w29");
  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, 15, WReg::w29), "index z30.b, #15, w29");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, -16, WReg::w29), "index z30.h, #-16, w29");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, 15, WReg::w29), "index z30.h, #15, w29");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, -16, WReg::w29), "index z30.s, #-16, w29");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, 15, WReg::w29), "index z30.s, #15, w29");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, -16, XReg::x29), "index z30.d, #-16, x29");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, 15, XReg::x29), "index z30.d, #15, x29");

  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.b, w29, w28");
  TEST_SINGLE(index(SubRegSize::i8Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.b, w29, w28");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.h, w29, w28");
  TEST_SINGLE(index(SubRegSize::i16Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.h, w29, w28");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.s, w29, w28");
  TEST_SINGLE(index(SubRegSize::i32Bit, ZReg::z30, WReg::w29, WReg::w28), "index z30.s, w29, w28");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, XReg::x29, XReg::x28), "index z30.d, x29, x28");
  TEST_SINGLE(index(SubRegSize::i64Bit, ZReg::z30, XReg::x29, XReg::x28), "index z30.d, x29, x28");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE stack frame adjustment") {
  TEST_SINGLE(addvl(XReg::rsp, XReg::rsp, -32), "addvl sp, sp, #-32");
  TEST_SINGLE(addvl(XReg::rsp, XReg::rsp, 31), "addvl sp, sp, #31");
  TEST_SINGLE(addvl(XReg::x30, XReg::x29, 15), "addvl x30, x29, #15");

  TEST_SINGLE(addpl(XReg::rsp, XReg::rsp, -32), "addpl sp, sp, #-32");
  TEST_SINGLE(addpl(XReg::rsp, XReg::rsp, 31), "addpl sp, sp, #31");
  TEST_SINGLE(addpl(XReg::x30, XReg::x29, 15), "addpl x30, x29, #15");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: Streaming SVE stack frame adjustment") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE stack frame size") {
  TEST_SINGLE(rdvl(XReg::x30, -32), "rdvl x30, #-32");
  TEST_SINGLE(rdvl(XReg::x30, 31), "rdvl x30, #31");
  TEST_SINGLE(rdvl(XReg::x30, 15), "rdvl x30, #15");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: Streaming SVE stack frame size") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer multiply vectors (unpredicated)") {
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "mul z30.b, z29.b, z28.b");
  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "mul z30.h, z29.h, z28.h");
  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "mul z30.s, z29.s, z28.s");
  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "mul z30.d, z29.d, z28.d");

  TEST_SINGLE(smulh(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smulh z30.b, z29.b, z28.b");
  TEST_SINGLE(smulh(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smulh z30.h, z29.h, z28.h");
  TEST_SINGLE(smulh(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smulh z30.s, z29.s, z28.s");
  TEST_SINGLE(smulh(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smulh z30.d, z29.d, z28.d");

  TEST_SINGLE(umulh(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umulh z30.b, z29.b, z28.b");
  TEST_SINGLE(umulh(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umulh z30.h, z29.h, z28.h");
  TEST_SINGLE(umulh(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umulh z30.s, z29.s, z28.s");
  TEST_SINGLE(umulh(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umulh z30.d, z29.d, z28.d");

  TEST_SINGLE(pmul(ZReg::z30, ZReg::z29, ZReg::z28), "pmul z30.b, z29.b, z28.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 signed saturating doubling multiply high (unpredicated)") {
  TEST_SINGLE(sqdmulh(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmulh z30.b, z29.b, z28.b");
  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmulh z30.h, z29.h, z28.h");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmulh z30.s, z29.s, z28.s");
  TEST_SINGLE(sqdmulh(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmulh z30.d, z29.d, z28.d");

  TEST_SINGLE(sqrdmulh(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmulh z30.b, z29.b, z28.b");
  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmulh z30.h, z29.h, z28.h");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmulh z30.s, z29.s, z28.s");
  TEST_SINGLE(sqrdmulh(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmulh z30.d, z29.d, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by wide elements (unpredicated)") {
  TEST_SINGLE(asr_wide(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "asr z30.b, z29.b, z28.d");
  TEST_SINGLE(asr_wide(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "asr z30.h, z29.h, z28.d");
  TEST_SINGLE(asr_wide(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "asr z30.s, z29.s, z28.d");

  TEST_SINGLE(lsr_wide(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsr z30.b, z29.b, z28.d");
  TEST_SINGLE(lsr_wide(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsr z30.h, z29.h, z28.d");
  TEST_SINGLE(lsr_wide(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsr z30.s, z29.s, z28.d");

  TEST_SINGLE(lsl_wide(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsl z30.b, z29.b, z28.d");
  TEST_SINGLE(lsl_wide(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsl z30.h, z29.h, z28.d");
  TEST_SINGLE(lsl_wide(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "lsl z30.s, z29.s, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by immediate (unpredicated)") {
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "asr z30.b, z29.b, #1");
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "asr z30.b, z29.b, #8");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "asr z30.h, z29.h, #1");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "asr z30.h, z29.h, #16");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "asr z30.s, z29.s, #1");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "asr z30.s, z29.s, #32");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "asr z30.d, z29.d, #1");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "asr z30.d, z29.d, #64");

  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "lsr z30.b, z29.b, #1");
  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "lsr z30.b, z29.b, #8");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "lsr z30.h, z29.h, #1");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "lsr z30.h, z29.h, #16");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "lsr z30.s, z29.s, #1");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "lsr z30.s, z29.s, #32");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "lsr z30.d, z29.d, #1");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "lsr z30.d, z29.d, #64");

  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 0), "lsl z30.b, z29.b, #0");
  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 7), "lsl z30.b, z29.b, #7");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "lsl z30.h, z29.h, #0");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 15), "lsl z30.h, z29.h, #15");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "lsl z30.s, z29.s, #0");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 31), "lsl z30.s, z29.s, #31");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "lsl z30.d, z29.d, #0");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 63), "lsl z30.d, z29.d, #63");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point trig select coefficient") {
  TEST_SINGLE(ftssel(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftssel z30.h, z29.h, z28.h");
  TEST_SINGLE(ftssel(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftssel z30.s, z29.s, z28.s");
  TEST_SINGLE(ftssel(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftssel z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point exponential accelerator") {
  TEST_SINGLE(fexpa(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "fexpa z30.h, z29.h");
  TEST_SINGLE(fexpa(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "fexpa z30.s, z29.s");
  TEST_SINGLE(fexpa(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "fexpa z30.d, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE constructive prefix (unpredicated)") {
  TEST_SINGLE(movprfx(ZReg::z30, ZReg::z29), "movprfx z30, z29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec vector by element count") {
  TEST_SINGLE(sqinch(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqinch z30.h, pow2");
  TEST_SINGLE(sqinch(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqinch z30.h, vl256, mul #7");
  TEST_SINGLE(sqinch(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqinch z30.h, all, mul #16");

  TEST_SINGLE(uqinch(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqinch z30.h, pow2");
  TEST_SINGLE(uqinch(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqinch z30.h, vl256, mul #7");
  TEST_SINGLE(uqinch(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqinch z30.h, all, mul #16");

  TEST_SINGLE(sqdech(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqdech z30.h, pow2");
  TEST_SINGLE(sqdech(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqdech z30.h, vl256, mul #7");
  TEST_SINGLE(sqdech(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqdech z30.h, all, mul #16");

  TEST_SINGLE(uqdech(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqdech z30.h, pow2");
  TEST_SINGLE(uqdech(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqdech z30.h, vl256, mul #7");
  TEST_SINGLE(uqdech(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqdech z30.h, all, mul #16");

  TEST_SINGLE(sqincw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqincw z30.s, pow2");
  TEST_SINGLE(sqincw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqincw z30.s, vl256, mul #7");
  TEST_SINGLE(sqincw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqincw z30.s, all, mul #16");

  TEST_SINGLE(uqincw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqincw z30.s, pow2");
  TEST_SINGLE(uqincw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqincw z30.s, vl256, mul #7");
  TEST_SINGLE(uqincw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqincw z30.s, all, mul #16");

  TEST_SINGLE(sqdecw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqdecw z30.s, pow2");
  TEST_SINGLE(sqdecw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqdecw z30.s, vl256, mul #7");
  TEST_SINGLE(sqdecw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqdecw z30.s, all, mul #16");

  TEST_SINGLE(uqdecw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqdecw z30.s, pow2");
  TEST_SINGLE(uqdecw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqdecw z30.s, vl256, mul #7");
  TEST_SINGLE(uqdecw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqdecw z30.s, all, mul #16");

  TEST_SINGLE(sqincd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqincd z30.d, pow2");
  TEST_SINGLE(sqincd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqincd z30.d, vl256, mul #7");
  TEST_SINGLE(sqincd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqincd z30.d, all, mul #16");

  TEST_SINGLE(uqincd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqincd z30.d, pow2");
  TEST_SINGLE(uqincd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqincd z30.d, vl256, mul #7");
  TEST_SINGLE(uqincd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqincd z30.d, all, mul #16");

  TEST_SINGLE(sqdecd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "sqdecd z30.d, pow2");
  TEST_SINGLE(sqdecd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "sqdecd z30.d, vl256, mul #7");
  TEST_SINGLE(sqdecd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "sqdecd z30.d, all, mul #16");

  TEST_SINGLE(uqdecd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "uqdecd z30.d, pow2");
  TEST_SINGLE(uqdecd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "uqdecd z30.d, vl256, mul #7");
  TEST_SINGLE(uqdecd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "uqdecd z30.d, all, mul #16");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE element count") {
  TEST_SINGLE(cntb(XReg::x30, PredicatePattern::SVE_POW2, 1), "cntb x30, pow2");
  TEST_SINGLE(cntb(XReg::x30, PredicatePattern::SVE_VL256, 7), "cntb x30, vl256, mul #7");
  TEST_SINGLE(cntb(XReg::x30, PredicatePattern::SVE_ALL, 16), "cntb x30, all, mul #16");

  TEST_SINGLE(cnth(XReg::x30, PredicatePattern::SVE_POW2, 1), "cnth x30, pow2");
  TEST_SINGLE(cnth(XReg::x30, PredicatePattern::SVE_VL256, 7), "cnth x30, vl256, mul #7");
  TEST_SINGLE(cnth(XReg::x30, PredicatePattern::SVE_ALL, 16), "cnth x30, all, mul #16");

  TEST_SINGLE(cntw(XReg::x30, PredicatePattern::SVE_POW2, 1), "cntw x30, pow2");
  TEST_SINGLE(cntw(XReg::x30, PredicatePattern::SVE_VL256, 7), "cntw x30, vl256, mul #7");
  TEST_SINGLE(cntw(XReg::x30, PredicatePattern::SVE_ALL, 16), "cntw x30, all, mul #16");

  TEST_SINGLE(cntd(XReg::x30, PredicatePattern::SVE_POW2, 1), "cntd x30, pow2");
  TEST_SINGLE(cntd(XReg::x30, PredicatePattern::SVE_VL256, 7), "cntd x30, vl256, mul #7");
  TEST_SINGLE(cntd(XReg::x30, PredicatePattern::SVE_ALL, 16), "cntd x30, all, mul #16");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec vector by element count") {
  TEST_SINGLE(inch(ZReg::z30, PredicatePattern::SVE_POW2, 1), "inch z30.h, pow2");
  TEST_SINGLE(inch(ZReg::z30, PredicatePattern::SVE_VL256, 7), "inch z30.h, vl256, mul #7");
  TEST_SINGLE(inch(ZReg::z30, PredicatePattern::SVE_ALL, 16), "inch z30.h, all, mul #16");

  TEST_SINGLE(dech(ZReg::z30, PredicatePattern::SVE_POW2, 1), "dech z30.h, pow2");
  TEST_SINGLE(dech(ZReg::z30, PredicatePattern::SVE_VL256, 7), "dech z30.h, vl256, mul #7");
  TEST_SINGLE(dech(ZReg::z30, PredicatePattern::SVE_ALL, 16), "dech z30.h, all, mul #16");

  TEST_SINGLE(incw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "incw z30.s, pow2");
  TEST_SINGLE(incw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "incw z30.s, vl256, mul #7");
  TEST_SINGLE(incw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "incw z30.s, all, mul #16");

  TEST_SINGLE(decw(ZReg::z30, PredicatePattern::SVE_POW2, 1), "decw z30.s, pow2");
  TEST_SINGLE(decw(ZReg::z30, PredicatePattern::SVE_VL256, 7), "decw z30.s, vl256, mul #7");
  TEST_SINGLE(decw(ZReg::z30, PredicatePattern::SVE_ALL, 16), "decw z30.s, all, mul #16");

  TEST_SINGLE(incd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "incd z30.d, pow2");
  TEST_SINGLE(incd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "incd z30.d, vl256, mul #7");
  TEST_SINGLE(incd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "incd z30.d, all, mul #16");

  TEST_SINGLE(decd(ZReg::z30, PredicatePattern::SVE_POW2, 1), "decd z30.d, pow2");
  TEST_SINGLE(decd(ZReg::z30, PredicatePattern::SVE_VL256, 7), "decd z30.d, vl256, mul #7");
  TEST_SINGLE(decd(ZReg::z30, PredicatePattern::SVE_ALL, 16), "decd z30.d, all, mul #16");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec register by element count") {
  TEST_SINGLE(incb(XReg::x30, PredicatePattern::SVE_POW2, 1), "incb x30, pow2");
  TEST_SINGLE(incb(XReg::x30, PredicatePattern::SVE_VL256, 7), "incb x30, vl256, mul #7");
  TEST_SINGLE(incb(XReg::x30, PredicatePattern::SVE_ALL, 16), "incb x30, all, mul #16");

  TEST_SINGLE(decb(XReg::x30, PredicatePattern::SVE_POW2, 1), "decb x30, pow2");
  TEST_SINGLE(decb(XReg::x30, PredicatePattern::SVE_VL256, 7), "decb x30, vl256, mul #7");
  TEST_SINGLE(decb(XReg::x30, PredicatePattern::SVE_ALL, 16), "decb x30, all, mul #16");

  TEST_SINGLE(inch(XReg::x30, PredicatePattern::SVE_POW2, 1), "inch x30, pow2");
  TEST_SINGLE(inch(XReg::x30, PredicatePattern::SVE_VL256, 7), "inch x30, vl256, mul #7");
  TEST_SINGLE(inch(XReg::x30, PredicatePattern::SVE_ALL, 16), "inch x30, all, mul #16");

  TEST_SINGLE(dech(XReg::x30, PredicatePattern::SVE_POW2, 1), "dech x30, pow2");
  TEST_SINGLE(dech(XReg::x30, PredicatePattern::SVE_VL256, 7), "dech x30, vl256, mul #7");
  TEST_SINGLE(dech(XReg::x30, PredicatePattern::SVE_ALL, 16), "dech x30, all, mul #16");

  TEST_SINGLE(incw(XReg::x30, PredicatePattern::SVE_POW2, 1), "incw x30, pow2");
  TEST_SINGLE(incw(XReg::x30, PredicatePattern::SVE_VL256, 7), "incw x30, vl256, mul #7");
  TEST_SINGLE(incw(XReg::x30, PredicatePattern::SVE_ALL, 16), "incw x30, all, mul #16");

  TEST_SINGLE(decw(XReg::x30, PredicatePattern::SVE_POW2, 1), "decw x30, pow2");
  TEST_SINGLE(decw(XReg::x30, PredicatePattern::SVE_VL256, 7), "decw x30, vl256, mul #7");
  TEST_SINGLE(decw(XReg::x30, PredicatePattern::SVE_ALL, 16), "decw x30, all, mul #16");

  TEST_SINGLE(incd(XReg::x30, PredicatePattern::SVE_POW2, 1), "incd x30, pow2");
  TEST_SINGLE(incd(XReg::x30, PredicatePattern::SVE_VL256, 7), "incd x30, vl256, mul #7");
  TEST_SINGLE(incd(XReg::x30, PredicatePattern::SVE_ALL, 16), "incd x30, all, mul #16");

  TEST_SINGLE(decd(XReg::x30, PredicatePattern::SVE_POW2, 1), "decd x30, pow2");
  TEST_SINGLE(decd(XReg::x30, PredicatePattern::SVE_VL256, 7), "decd x30, vl256, mul #7");
  TEST_SINGLE(decd(XReg::x30, PredicatePattern::SVE_ALL, 16), "decd x30, all, mul #16");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec register by element count") {
  TEST_SINGLE(sqincb(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqincb x30, pow2");
  TEST_SINGLE(sqincb(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqincb x30, vl256, mul #7");
  TEST_SINGLE(sqincb(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqincb x30, all, mul #16");

  TEST_SINGLE(sqincb(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqincb x30, w30, pow2");
  TEST_SINGLE(sqincb(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqincb x30, w30, vl256, mul #7");
  TEST_SINGLE(sqincb(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqincb x30, w30, all, mul #16");

  TEST_SINGLE(uqincb(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqincb x30, pow2");
  TEST_SINGLE(uqincb(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqincb x30, vl256, mul #7");
  TEST_SINGLE(uqincb(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqincb x30, all, mul #16");

  TEST_SINGLE(uqincb(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqincb w30, pow2");
  TEST_SINGLE(uqincb(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqincb w30, vl256, mul #7");
  TEST_SINGLE(uqincb(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqincb w30, all, mul #16");

  TEST_SINGLE(sqdecb(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqdecb x30, pow2");
  TEST_SINGLE(sqdecb(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqdecb x30, vl256, mul #7");
  TEST_SINGLE(sqdecb(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqdecb x30, all, mul #16");

  TEST_SINGLE(sqdecb(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqdecb x30, w30, pow2");
  TEST_SINGLE(sqdecb(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqdecb x30, w30, vl256, mul #7");
  TEST_SINGLE(sqdecb(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqdecb x30, w30, all, mul #16");

  TEST_SINGLE(uqdecb(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqdecb x30, pow2");
  TEST_SINGLE(uqdecb(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqdecb x30, vl256, mul #7");
  TEST_SINGLE(uqdecb(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqdecb x30, all, mul #16");

  TEST_SINGLE(uqdecb(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqdecb w30, pow2");
  TEST_SINGLE(uqdecb(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqdecb w30, vl256, mul #7");
  TEST_SINGLE(uqdecb(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqdecb w30, all, mul #16");

  TEST_SINGLE(sqinch(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqinch x30, pow2");
  TEST_SINGLE(sqinch(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqinch x30, vl256, mul #7");
  TEST_SINGLE(sqinch(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqinch x30, all, mul #16");

  TEST_SINGLE(sqinch(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqinch x30, w30, pow2");
  TEST_SINGLE(sqinch(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqinch x30, w30, vl256, mul #7");
  TEST_SINGLE(sqinch(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqinch x30, w30, all, mul #16");

  TEST_SINGLE(uqinch(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqinch x30, pow2");
  TEST_SINGLE(uqinch(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqinch x30, vl256, mul #7");
  TEST_SINGLE(uqinch(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqinch x30, all, mul #16");

  TEST_SINGLE(uqinch(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqinch w30, pow2");
  TEST_SINGLE(uqinch(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqinch w30, vl256, mul #7");
  TEST_SINGLE(uqinch(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqinch w30, all, mul #16");

  TEST_SINGLE(sqdech(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqdech x30, pow2");
  TEST_SINGLE(sqdech(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqdech x30, vl256, mul #7");
  TEST_SINGLE(sqdech(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqdech x30, all, mul #16");

  TEST_SINGLE(sqdech(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqdech x30, w30, pow2");
  TEST_SINGLE(sqdech(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqdech x30, w30, vl256, mul #7");
  TEST_SINGLE(sqdech(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqdech x30, w30, all, mul #16");

  TEST_SINGLE(uqdech(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqdech x30, pow2");
  TEST_SINGLE(uqdech(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqdech x30, vl256, mul #7");
  TEST_SINGLE(uqdech(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqdech x30, all, mul #16");

  TEST_SINGLE(uqdech(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqdech w30, pow2");
  TEST_SINGLE(uqdech(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqdech w30, vl256, mul #7");
  TEST_SINGLE(uqdech(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqdech w30, all, mul #16");

  TEST_SINGLE(sqincw(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqincw x30, pow2");
  TEST_SINGLE(sqincw(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqincw x30, vl256, mul #7");
  TEST_SINGLE(sqincw(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqincw x30, all, mul #16");

  TEST_SINGLE(sqincw(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqincw x30, w30, pow2");
  TEST_SINGLE(sqincw(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqincw x30, w30, vl256, mul #7");
  TEST_SINGLE(sqincw(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqincw x30, w30, all, mul #16");

  TEST_SINGLE(uqincw(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqincw x30, pow2");
  TEST_SINGLE(uqincw(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqincw x30, vl256, mul #7");
  TEST_SINGLE(uqincw(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqincw x30, all, mul #16");

  TEST_SINGLE(uqincw(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqincw w30, pow2");
  TEST_SINGLE(uqincw(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqincw w30, vl256, mul #7");
  TEST_SINGLE(uqincw(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqincw w30, all, mul #16");

  TEST_SINGLE(sqdecw(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqdecw x30, pow2");
  TEST_SINGLE(sqdecw(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqdecw x30, vl256, mul #7");
  TEST_SINGLE(sqdecw(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqdecw x30, all, mul #16");

  TEST_SINGLE(sqdecw(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqdecw x30, w30, pow2");
  TEST_SINGLE(sqdecw(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqdecw x30, w30, vl256, mul #7");
  TEST_SINGLE(sqdecw(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqdecw x30, w30, all, mul #16");

  TEST_SINGLE(uqdecw(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqdecw x30, pow2");
  TEST_SINGLE(uqdecw(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqdecw x30, vl256, mul #7");
  TEST_SINGLE(uqdecw(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqdecw x30, all, mul #16");

  TEST_SINGLE(uqdecw(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqdecw w30, pow2");
  TEST_SINGLE(uqdecw(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqdecw w30, vl256, mul #7");
  TEST_SINGLE(uqdecw(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqdecw w30, all, mul #16");

  TEST_SINGLE(sqincd(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqincd x30, pow2");
  TEST_SINGLE(sqincd(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqincd x30, vl256, mul #7");
  TEST_SINGLE(sqincd(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqincd x30, all, mul #16");

  TEST_SINGLE(sqincd(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqincd x30, w30, pow2");
  TEST_SINGLE(sqincd(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqincd x30, w30, vl256, mul #7");
  TEST_SINGLE(sqincd(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqincd x30, w30, all, mul #16");

  TEST_SINGLE(uqincd(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqincd x30, pow2");
  TEST_SINGLE(uqincd(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqincd x30, vl256, mul #7");
  TEST_SINGLE(uqincd(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqincd x30, all, mul #16");

  TEST_SINGLE(uqincd(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqincd w30, pow2");
  TEST_SINGLE(uqincd(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqincd w30, vl256, mul #7");
  TEST_SINGLE(uqincd(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqincd w30, all, mul #16");

  TEST_SINGLE(sqdecd(XReg::x30, PredicatePattern::SVE_POW2, 1), "sqdecd x30, pow2");
  TEST_SINGLE(sqdecd(XReg::x30, PredicatePattern::SVE_VL256, 7), "sqdecd x30, vl256, mul #7");
  TEST_SINGLE(sqdecd(XReg::x30, PredicatePattern::SVE_ALL, 16), "sqdecd x30, all, mul #16");

  TEST_SINGLE(sqdecd(WReg::w30, PredicatePattern::SVE_POW2, 1), "sqdecd x30, w30, pow2");
  TEST_SINGLE(sqdecd(WReg::w30, PredicatePattern::SVE_VL256, 7), "sqdecd x30, w30, vl256, mul #7");
  TEST_SINGLE(sqdecd(WReg::w30, PredicatePattern::SVE_ALL, 16), "sqdecd x30, w30, all, mul #16");

  TEST_SINGLE(uqdecd(XReg::x30, PredicatePattern::SVE_POW2, 1), "uqdecd x30, pow2");
  TEST_SINGLE(uqdecd(XReg::x30, PredicatePattern::SVE_VL256, 7), "uqdecd x30, vl256, mul #7");
  TEST_SINGLE(uqdecd(XReg::x30, PredicatePattern::SVE_ALL, 16), "uqdecd x30, all, mul #16");

  TEST_SINGLE(uqdecd(WReg::w30, PredicatePattern::SVE_POW2, 1), "uqdecd w30, pow2");
  TEST_SINGLE(uqdecd(WReg::w30, PredicatePattern::SVE_VL256, 7), "uqdecd w30, vl256, mul #7");
  TEST_SINGLE(uqdecd(WReg::w30, PredicatePattern::SVE_ALL, 16), "uqdecd w30, all, mul #16");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Bitwise Immediate") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical with immediate (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Integer Wide Immediate - Predicated") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE copy integer immediate (predicated)") {
  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.b, p6/m, #-128")
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.h, p6/m, #-128");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.s, p6/m, #-128");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.d, p6/m, #-128");

  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.b, p6/m, #127");
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.h, p6/m, #127");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.s, p6/m, #127");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.d, p6/m, #127");

  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.h, p6/m, #-128, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.s, p6/m, #-128, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.d, p6/m, #-128, lsl #8");

  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.h, p6/m, #127, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.s, p6/m, #127, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.d, p6/m, #127, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.b, p6/m, #-128")
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.h, p6/m, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.s, p6/m, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -128), "mov z30.d, p6/m, #-128");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.b, p6/m, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.h, p6/m, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.s, p6/m, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 127), "mov z30.d, p6/m, #127");

  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.h, p6/m, #-128, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.s, p6/m, #-128, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -32768), "mov z30.d, p6/m, #-128, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.h, p6/m, #127, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.s, p6/m, #127, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 32512), "mov z30.d, p6/m, #127, lsl #8");

  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.b, p6/z, #-128")
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.h, p6/z, #-128");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.s, p6/z, #-128");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.d, p6/z, #-128");

  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.b, p6/z, #127");
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.h, p6/z, #127");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.s, p6/z, #127");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.d, p6/z, #127");

  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.h, p6/z, #-128, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.s, p6/z, #-128, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.d, p6/z, #-128, lsl #8");

  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.h, p6/z, #127, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.s, p6/z, #127, lsl #8");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.d, p6/z, #127, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.b, p6/z, #-128")
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.h, p6/z, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.s, p6/z, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), -128), "mov z30.d, p6/z, #-128");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.b, p6/z, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.h, p6/z, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.s, p6/z, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), 127), "mov z30.d, p6/z, #127");

  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.h, p6/z, #-128, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.s, p6/z, #-128, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), -32768), "mov z30.d, p6/z, #-128, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.h, p6/z, #127, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.s, p6/z, #127, lsl #8");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), 32512), "mov z30.d, p6/z, #127, lsl #8");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Unpredicated") {
  TEST_SINGLE(dup(SubRegSize::i8Bit, ZReg::z30, Reg::r29), "mov z30.b, w29");
  TEST_SINGLE(dup(SubRegSize::i16Bit, ZReg::z30, Reg::r29), "mov z30.h, w29");
  TEST_SINGLE(dup(SubRegSize::i32Bit, ZReg::z30, Reg::r29), "mov z30.s, w29");
  TEST_SINGLE(dup(SubRegSize::i64Bit, ZReg::z30, Reg::r29), "mov z30.d, x29");

  TEST_SINGLE(mov(SubRegSize::i8Bit, ZReg::z30, Reg::r29), "mov z30.b, w29");
  TEST_SINGLE(mov(SubRegSize::i16Bit, ZReg::z30, Reg::r29), "mov z30.h, w29");
  TEST_SINGLE(mov(SubRegSize::i32Bit, ZReg::z30, Reg::r29), "mov z30.s, w29");
  TEST_SINGLE(mov(SubRegSize::i64Bit, ZReg::z30, Reg::r29), "mov z30.d, x29");

  TEST_SINGLE(insr(SubRegSize::i8Bit, ZReg::z30, Reg::r29), "insr z30.b, w29");
  TEST_SINGLE(insr(SubRegSize::i16Bit, ZReg::z30, Reg::r29), "insr z30.h, w29");
  TEST_SINGLE(insr(SubRegSize::i32Bit, ZReg::z30, Reg::r29), "insr z30.s, w29");
  TEST_SINGLE(insr(SubRegSize::i64Bit, ZReg::z30, Reg::r29), "insr z30.d, x29");

  TEST_SINGLE(insr(SubRegSize::i8Bit, ZReg::z30, VReg::v29), "insr z30.b, b29");
  TEST_SINGLE(insr(SubRegSize::i16Bit, ZReg::z30, VReg::v29), "insr z30.h, h29");
  TEST_SINGLE(insr(SubRegSize::i32Bit, ZReg::z30, VReg::v29), "insr z30.s, s29");
  TEST_SINGLE(insr(SubRegSize::i64Bit, ZReg::z30, VReg::v29), "insr z30.d, d29");

  TEST_SINGLE(rev(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "rev z30.b, z29.b");
  TEST_SINGLE(rev(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "rev z30.h, z29.h");
  TEST_SINGLE(rev(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "rev z30.s, z29.s");
  TEST_SINGLE(rev(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "rev z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE unpack vector elements") {
  // TEST_SINGLE(sunpklo(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "sunpklo z30.b, z29.b");
  TEST_SINGLE(sunpklo(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sunpklo z30.h, z29.b");
  TEST_SINGLE(sunpklo(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sunpklo z30.s, z29.h");
  TEST_SINGLE(sunpklo(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sunpklo z30.d, z29.s");
  // TEST_SINGLE(sunpklo(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "sunpklo z30.q, z29.q");

  // TEST_SINGLE(sunpkhi(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "sunpkhi z30.b, z29.b");
  TEST_SINGLE(sunpkhi(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sunpkhi z30.h, z29.b");
  TEST_SINGLE(sunpkhi(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sunpkhi z30.s, z29.h");
  TEST_SINGLE(sunpkhi(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sunpkhi z30.d, z29.s");
  // TEST_SINGLE(sunpkhi(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "sunpkhi z30.q, z29.q");

  // TEST_SINGLE(uunpklo(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "uunpklo z30.b, z29.b");
  TEST_SINGLE(uunpklo(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uunpklo z30.h, z29.b");
  TEST_SINGLE(uunpklo(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uunpklo z30.s, z29.h");
  TEST_SINGLE(uunpklo(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uunpklo z30.d, z29.s");
  // TEST_SINGLE(uunpklo(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "uunpklo z30.q, z29.q");

  // TEST_SINGLE(uunpkhi(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "uunpkhi z30.b, z29.b");
  TEST_SINGLE(uunpkhi(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uunpkhi z30.h, z29.b");
  TEST_SINGLE(uunpkhi(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uunpkhi z30.s, z29.h");
  TEST_SINGLE(uunpkhi(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uunpkhi z30.d, z29.s");
  // TEST_SINGLE(uunpkhi(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "uunpkhi z30.q, z29.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Predicate") {
  TEST_SINGLE(rev(SubRegSize::i8Bit, PReg::p15, PReg::p14), "rev p15.b, p14.b");
  TEST_SINGLE(rev(SubRegSize::i16Bit, PReg::p15, PReg::p14), "rev p15.h, p14.h");
  TEST_SINGLE(rev(SubRegSize::i32Bit, PReg::p15, PReg::p14), "rev p15.s, p14.s");
  TEST_SINGLE(rev(SubRegSize::i64Bit, PReg::p15, PReg::p14), "rev p15.d, p14.d");

  TEST_SINGLE(punpklo(PReg::p15, PReg::p14), "punpklo p15.h, p14.b");
  TEST_SINGLE(punpkhi(PReg::p15, PReg::p14), "punpkhi p15.h, p14.b");

  TEST_SINGLE(zip1(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "zip1 p15.b, p14.b, p13.b");
  TEST_SINGLE(zip1(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "zip1 p15.h, p14.h, p13.h");
  TEST_SINGLE(zip1(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "zip1 p15.s, p14.s, p13.s");
  TEST_SINGLE(zip1(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "zip1 p15.d, p14.d, p13.d");

  TEST_SINGLE(zip2(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "zip2 p15.b, p14.b, p13.b");
  TEST_SINGLE(zip2(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "zip2 p15.h, p14.h, p13.h");
  TEST_SINGLE(zip2(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "zip2 p15.s, p14.s, p13.s");
  TEST_SINGLE(zip2(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "zip2 p15.d, p14.d, p13.d");

  TEST_SINGLE(uzp1(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "uzp1 p15.b, p14.b, p13.b");
  TEST_SINGLE(uzp1(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "uzp1 p15.h, p14.h, p13.h");
  TEST_SINGLE(uzp1(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "uzp1 p15.s, p14.s, p13.s");
  TEST_SINGLE(uzp1(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "uzp1 p15.d, p14.d, p13.d");

  TEST_SINGLE(uzp2(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "uzp2 p15.b, p14.b, p13.b");
  TEST_SINGLE(uzp2(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "uzp2 p15.h, p14.h, p13.h");
  TEST_SINGLE(uzp2(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "uzp2 p15.s, p14.s, p13.s");
  TEST_SINGLE(uzp2(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "uzp2 p15.d, p14.d, p13.d");

  TEST_SINGLE(trn1(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "trn1 p15.b, p14.b, p13.b");
  TEST_SINGLE(trn1(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "trn1 p15.h, p14.h, p13.h");
  TEST_SINGLE(trn1(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "trn1 p15.s, p14.s, p13.s");
  TEST_SINGLE(trn1(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "trn1 p15.d, p14.d, p13.d");

  TEST_SINGLE(trn2(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p13), "trn2 p15.b, p14.b, p13.b");
  TEST_SINGLE(trn2(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p13), "trn2 p15.h, p14.h, p13.h");
  TEST_SINGLE(trn2(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p13), "trn2 p15.s, p14.s, p13.s");
  TEST_SINGLE(trn2(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p13), "trn2 p15.d, p14.d, p13.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Predicated - Base") {
  // CPY (SIMD&FP scalar)
  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), VReg::v30), "mov z30.b, p7/m, b30");
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), VReg::v30), "mov z30.h, p7/m, h30");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), VReg::v30), "mov z30.s, p7/m, s30");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), VReg::v30), "mov z30.d, p7/m, d30");

  // TEST_SINGLE(compact(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z29),   "compact z30.b, p6, z29.b");
  // TEST_SINGLE(compact(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z29),  "compact z30.h, p6, z29.h");
  TEST_SINGLE(compact(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z29), "compact z30.s, p6, z29.s");
  TEST_SINGLE(compact(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z29), "compact z30.d, p6, z29.d");
  // TEST_SINGLE(compact(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z29), "compact z30.q, p6, z29.q");

  // CPY (scalar)
  TEST_SINGLE(cpy(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), WReg::rsp), "mov z30.b, p7/m, wsp");
  TEST_SINGLE(cpy(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), WReg::rsp), "mov z30.h, p7/m, wsp");
  TEST_SINGLE(cpy(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), WReg::rsp), "mov z30.s, p7/m, wsp");
  TEST_SINGLE(cpy(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), XReg::rsp), "mov z30.d, p7/m, sp");

  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.b, p6, {z28.b, "
                                                                                                          "z29.b}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.h, p6, {z28.h, "
                                                                                                           "z29.h}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.s, p6, {z28.s, "
                                                                                                           "z29.s}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.d, p6, {z28.d, "
                                                                                                           "z29.d}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z31, ZReg::z0), "splice z30.d, p6, {z31.d, "
                                                                                                          "z0.d}");
  // TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.q, p6, {z28.q, z29.q}");

  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.b, p6, z30.b, z28.b");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.h, p6, z30.h, z28.h");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.s, p6, z30.s, z28.s");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.d, p6, z30.d, z28.d");
  // TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.q, p6, z30.q, z28.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE extract element to general register") {
  TEST_SINGLE(lasta(SubRegSize::i8Bit, WReg::w30, PReg::p7, ZReg::z30), "lasta w30, p7, z30.b");
  TEST_SINGLE(lasta(SubRegSize::i16Bit, WReg::w30, PReg::p7, ZReg::z30), "lasta w30, p7, z30.h");
  TEST_SINGLE(lasta(SubRegSize::i32Bit, WReg::w30, PReg::p7, ZReg::z30), "lasta w30, p7, z30.s");
  TEST_SINGLE(lasta(SubRegSize::i64Bit, XReg::x30, PReg::p7, ZReg::z30), "lasta x30, p7, z30.d");

  TEST_SINGLE(lastb(SubRegSize::i8Bit, WReg::w30, PReg::p7, ZReg::z30), "lastb w30, p7, z30.b");
  TEST_SINGLE(lastb(SubRegSize::i16Bit, WReg::w30, PReg::p7, ZReg::z30), "lastb w30, p7, z30.h");
  TEST_SINGLE(lastb(SubRegSize::i32Bit, WReg::w30, PReg::p7, ZReg::z30), "lastb w30, p7, z30.s");
  TEST_SINGLE(lastb(SubRegSize::i64Bit, XReg::x30, PReg::p7, ZReg::z30), "lastb x30, p7, z30.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE extract element to SIMD&FP scalar register") {
  TEST_SINGLE(lasta(SubRegSize::i8Bit, BReg::b30, PReg::p7, ZReg::z29), "lasta b30, p7, z29.b");
  TEST_SINGLE(lasta(SubRegSize::i16Bit, HReg::h30, PReg::p7, ZReg::z29), "lasta h30, p7, z29.h");
  TEST_SINGLE(lasta(SubRegSize::i32Bit, SReg::s30, PReg::p7, ZReg::z29), "lasta s30, p7, z29.s");
  TEST_SINGLE(lasta(SubRegSize::i64Bit, DReg::d30, PReg::p7, ZReg::z29), "lasta d30, p7, z29.d");

  TEST_SINGLE(lastb(SubRegSize::i8Bit, BReg::b30, PReg::p7, ZReg::z29), "lastb b30, p7, z29.b");
  TEST_SINGLE(lastb(SubRegSize::i16Bit, HReg::h30, PReg::p7, ZReg::z29), "lastb h30, p7, z29.h");
  TEST_SINGLE(lastb(SubRegSize::i32Bit, SReg::s30, PReg::p7, ZReg::z29), "lastb s30, p7, z29.s");
  TEST_SINGLE(lastb(SubRegSize::i64Bit, DReg::d30, PReg::p7, ZReg::z29), "lastb d30, p7, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE reverse within elements") {
  // TEST_SINGLE(revb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.b, p6/m, z29.b");
  TEST_SINGLE(revb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.h, p6/m, z29.h");
  TEST_SINGLE(revb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.s, p6/m, z29.s");
  TEST_SINGLE(revb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.d, p6/m, z29.d");

  // TEST_SINGLE(revh(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.b, p6/m, z29.b");
  // TEST_SINGLE(revh(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.h, p6/m, z29.h");
  TEST_SINGLE(revh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.s, p6/m, z29.s");
  TEST_SINGLE(revh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.d, p6/m, z29.d");

  // TEST_SINGLE(revw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.b, p6/m, z29.b");
  // TEST_SINGLE(revw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.h, p6/m, z29.h");
  // TEST_SINGLE(revw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.s, p6/m, z29.s");
  TEST_SINGLE(revw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.d, p6/m, z29.d");

  TEST_SINGLE(rbit(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.b, p6/m, z29.b");
  TEST_SINGLE(rbit(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.h, p6/m, z29.h");
  TEST_SINGLE(rbit(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.s, p6/m, z29.s");
  TEST_SINGLE(rbit(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally broadcast element to vector") {
  TEST_SINGLE(clasta(SubRegSize::i8Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clasta z30.b, p7, z30.b, z29.b");
  TEST_SINGLE(clasta(SubRegSize::i16Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clasta z30.h, p7, z30.h, z29.h");
  TEST_SINGLE(clasta(SubRegSize::i32Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clasta z30.s, p7, z30.s, z29.s");
  TEST_SINGLE(clasta(SubRegSize::i64Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clasta z30.d, p7, z30.d, z29.d");

  TEST_SINGLE(clastb(SubRegSize::i8Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clastb z30.b, p7, z30.b, z29.b");
  TEST_SINGLE(clastb(SubRegSize::i16Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clastb z30.h, p7, z30.h, z29.h");
  TEST_SINGLE(clastb(SubRegSize::i32Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clastb z30.s, p7, z30.s, z29.s");
  TEST_SINGLE(clastb(SubRegSize::i64Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z29), "clastb z30.d, p7, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally extract element to SIMD&FP scalar") {
  TEST_SINGLE(clasta(SubRegSize::i8Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clasta b30, p7, b30, z29.b");
  TEST_SINGLE(clasta(SubRegSize::i16Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clasta h30, p7, h30, z29.h");
  TEST_SINGLE(clasta(SubRegSize::i32Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clasta s30, p7, s30, z29.s");
  TEST_SINGLE(clasta(SubRegSize::i64Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clasta d30, p7, d30, z29.d");

  TEST_SINGLE(clastb(SubRegSize::i8Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clastb b30, p7, b30, z29.b");
  TEST_SINGLE(clastb(SubRegSize::i16Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clastb h30, p7, h30, z29.h");
  TEST_SINGLE(clastb(SubRegSize::i32Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clastb s30, p7, s30, z29.s");
  TEST_SINGLE(clastb(SubRegSize::i64Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "clastb d30, p7, d30, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE reverse doublewords") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally extract element to general register") {
  TEST_SINGLE(clasta(SubRegSize::i8Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clasta w30, p7, w30, z29.b");
  TEST_SINGLE(clasta(SubRegSize::i16Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clasta w30, p7, w30, z29.h");
  TEST_SINGLE(clasta(SubRegSize::i32Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clasta w30, p7, w30, z29.s");
  TEST_SINGLE(clasta(SubRegSize::i64Bit, XReg::x30, PReg::p7, XReg::x30, ZReg::z29), "clasta x30, p7, x30, z29.d");

  TEST_SINGLE(clastb(SubRegSize::i8Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clastb w30, p7, w30, z29.b");
  TEST_SINGLE(clastb(SubRegSize::i16Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clastb w30, p7, w30, z29.h");
  TEST_SINGLE(clastb(SubRegSize::i32Bit, WReg::w30, PReg::p7, WReg::w30, ZReg::z29), "clastb w30, p7, w30, z29.s");
  TEST_SINGLE(clastb(SubRegSize::i64Bit, XReg::x30, PReg::p7, XReg::x30, ZReg::z29), "clastb x30, p7, x30, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Extract") {
  TEST_SINGLE(ext<ARMEmitter::OpType::Destructive>(ZReg::z30, ZReg::z30, ZReg::z29, 0), "ext z30.b, z30.b, z29.b, #0");
  TEST_SINGLE(ext<ARMEmitter::OpType::Destructive>(ZReg::z30, ZReg::z30, ZReg::z29, 255), "ext z30.b, z30.b, z29.b, #255");

  TEST_SINGLE(ext<ARMEmitter::OpType::Constructive>(ZReg::z30, ZReg::z28, ZReg::z29, 0), "ext z30.b, {z28.b, z29.b}, #0");
  TEST_SINGLE(ext<ARMEmitter::OpType::Constructive>(ZReg::z30, ZReg::z28, ZReg::z29, 255), "ext z30.b, {z28.b, z29.b}, #255");
  TEST_SINGLE(ext<ARMEmitter::OpType::Constructive>(ZReg::z30, ZReg::z31, ZReg::z0, 255), "ext z30.b, {z31.b, z0.b}, #255");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE permute vector segments") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare vectors") {
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpne(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpne(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpne(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpne(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.d, p5/z, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with wide elements") {
  TEST_SINGLE(cmpeq_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmpeq_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmpeq_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmpgt_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmpgt_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmpgt_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmpge_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmpge_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmpge_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmphi_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmphi_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmphi_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmphs_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmphs_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmphs_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmplt_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplt p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmplt_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplt p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmplt_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplt p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmple_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmple p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmple_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmple p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmple_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmple p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmplo_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplo p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmplo_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplo p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmplo_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmplo p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmpls_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpls p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmpls_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpls p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmpls_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpls p6.s, p5/z, z30.s, z29.d");

  TEST_SINGLE(cmpne_wide(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.b, p5/z, z30.b, z29.d");
  TEST_SINGLE(cmpne_wide(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.h, p5/z, z30.h, z29.d");
  TEST_SINGLE(cmpne_wide(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.s, p5/z, z30.s, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE propagate break from previous partition") {
  TEST_SINGLE(brkpa(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p12), "brkpa p15.b, p14/z, p13.b, p12.b");
  TEST_SINGLE(brkpas(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p12), "brkpas p15.b, p14/z, p13.b, p12.b");
  TEST_SINGLE(brkpb(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p12), "brkpb p15.b, p14/z, p13.b, p12.b");
  TEST_SINGLE(brkpbs(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p12), "brkpbs p15.b, p14/z, p13.b, p12.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE propagate break to next partition") {
  TEST_SINGLE(brkn(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p15), "brkn p15.b, p14/z, p13.b, p15.b");
  TEST_SINGLE(brkns(PReg::p15, PReg::p14.Zeroing(), PReg::p13, PReg::p15), "brkns p15.b, p14/z, p13.b, p15.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE partition break condition") {
  TEST_SINGLE(brka(PReg::p15, PReg::p14.Zeroing(), PReg::p13), "brka p15.b, p14/z, p13.b");
  TEST_SINGLE(brka(PReg::p15, PReg::p14.Merging(), PReg::p13), "brka p15.b, p14/m, p13.b");
  TEST_SINGLE(brkas(PReg::p15, PReg::p14.Zeroing(), PReg::p13), "brkas p15.b, p14/z, p13.b");

  TEST_SINGLE(brkb(PReg::p15, PReg::p14.Zeroing(), PReg::p13), "brkb p15.b, p14/z, p13.b");
  TEST_SINGLE(brkb(PReg::p15, PReg::p14.Merging(), PReg::p13), "brkb p15.b, p14/m, p13.b");
  TEST_SINGLE(brkbs(PReg::p15, PReg::p14.Zeroing(), PReg::p13), "brkbs p15.b, p14/z, p13.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Predicate Misc") {
  TEST_SINGLE(pnext(SubRegSize::i8Bit, PReg::p15, PReg::p14, PReg::p15), "pnext p15.b, p14, p15.b");
  TEST_SINGLE(pnext(SubRegSize::i16Bit, PReg::p15, PReg::p14, PReg::p15), "pnext p15.h, p14, p15.h");
  TEST_SINGLE(pnext(SubRegSize::i32Bit, PReg::p15, PReg::p14, PReg::p15), "pnext p15.s, p14, p15.s");
  TEST_SINGLE(pnext(SubRegSize::i64Bit, PReg::p15, PReg::p14, PReg::p15), "pnext p15.d, p14, p15.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate test") {
  TEST_SINGLE(ptest(PReg::p6, PReg::p5), "ptest p6, p5.b");
  TEST_SINGLE(ptest(PReg::p15, PReg::p14), "ptest p15, p14.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate first active") {
  TEST_SINGLE(pfirst(PReg::p6, PReg::p5, PReg::p6), "pfirst p6.b, p5, p6.b");
  TEST_SINGLE(pfirst(PReg::p15, PReg::p14, PReg::p15), "pfirst p15.b, p14, p15.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate zero") {
  TEST_SINGLE(pfalse(PReg::p6), "pfalse p6.b");
  TEST_SINGLE(pfalse(PReg::p15), "pfalse p15.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate read from FFR (predicated)") {
  TEST_SINGLE(rdffr(PReg::p6, PReg::p5.Zeroing()), "rdffr p6.b, p5/z");
  TEST_SINGLE(rdffr(PReg::p15, PReg::p14.Zeroing()), "rdffr p15.b, p14/z");
  TEST_SINGLE(rdffrs(PReg::p6, PReg::p5.Zeroing()), "rdffrs p6.b, p5/z");
  TEST_SINGLE(rdffrs(PReg::p15, PReg::p14.Zeroing()), "rdffrs p15.b, p14/z");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate read from FFR (unpredicated)") {
  TEST_SINGLE(rdffr(PReg::p6), "rdffr p6.b");
  TEST_SINGLE(rdffr(PReg::p15), "rdffr p15.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate initialize") {
  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.b, pow2");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.h, pow2");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.s, pow2");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.d, pow2");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.b, pow2");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.h, pow2");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.s, pow2");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.d, pow2");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.b, vl1");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.h, vl1");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.s, vl1");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.d, vl1");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.b, vl1");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.h, vl1");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.s, vl1");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.d, vl1");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.b, vl2");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.h, vl2");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.s, vl2");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.d, vl2");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.b, vl2");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.h, vl2");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.s, vl2");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.d, vl2");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.b, vl3");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.h, vl3");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.s, vl3");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.d, vl3");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.b, vl3");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.h, vl3");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.s, vl3");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.d, vl3");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.b, vl4");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.h, vl4");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.s, vl4");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.d, vl4");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.b, vl4");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.h, vl4");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.s, vl4");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.d, vl4");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.b, vl5");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.h, vl5");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.s, vl5");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.d, vl5");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.b, vl5");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.h, vl5");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.s, vl5");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.d, vl5");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.b, vl6");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.h, vl6");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.s, vl6");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.d, vl6");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.b, vl6");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.h, vl6");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.s, vl6");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.d, vl6");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.b, vl7");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.h, vl7");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.s, vl7");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.d, vl7");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.b, vl7");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.h, vl7");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.s, vl7");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.d, vl7");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.b, vl8");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.h, vl8");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.s, vl8");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.d, vl8");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.b, vl8");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.h, vl8");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.s, vl8");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.d, vl8");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.b, vl16");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.h, vl16");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.s, vl16");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.d, vl16");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.b, vl16");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.h, vl16");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.s, vl16");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.d, vl16");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.b, vl32");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.h, vl32");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.s, vl32");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.d, vl32");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.b, vl32");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.h, vl32");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.s, vl32");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.d, vl32");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.b, vl64");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.h, vl64");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.s, vl64");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.d, vl64");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.b, vl64");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.h, vl64");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.s, vl64");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.d, vl64");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.b, vl128");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.h, vl128");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.s, vl128");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.d, vl128");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.b, vl128");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.h, vl128");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.s, vl128");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.d, vl128");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.b, vl256");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.h, vl256");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.s, vl256");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.d, vl256");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.b, vl256");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.h, vl256");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.s, vl256");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.d, vl256");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.b, mul4");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.h, mul4");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.s, mul4");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.d, mul4");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.b, mul4");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.h, mul4");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.s, mul4");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.d, mul4");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.b, mul3");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.h, mul3");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.s, mul3");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.d, mul3");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.b, mul3");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.h, mul3");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.s, mul3");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.d, mul3");

  TEST_SINGLE(ptrue(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.b");
  TEST_SINGLE(ptrue(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.h");
  TEST_SINGLE(ptrue(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.s");
  TEST_SINGLE(ptrue(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.d");

  TEST_SINGLE(ptrues(SubRegSize::i8Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.b");
  TEST_SINGLE(ptrues(SubRegSize::i16Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.h");
  TEST_SINGLE(ptrues(SubRegSize::i32Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.s");
  TEST_SINGLE(ptrues(SubRegSize::i64Bit, PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare scalar count and limit") {
  TEST_SINGLE(whilege(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilege p15.b, x30, x29");
  TEST_SINGLE(whilege(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilege p15.h, x30, x29");
  TEST_SINGLE(whilege(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilege p15.s, x30, x29");
  TEST_SINGLE(whilege(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilege p15.d, x30, x29");
  TEST_SINGLE(whilege(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilege p15.b, w30, w29");
  TEST_SINGLE(whilege(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilege p15.h, w30, w29");
  TEST_SINGLE(whilege(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilege p15.s, w30, w29");
  TEST_SINGLE(whilege(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilege p15.d, w30, w29");

  TEST_SINGLE(whilegt(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilegt p15.b, x30, x29");
  TEST_SINGLE(whilegt(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilegt p15.h, x30, x29");
  TEST_SINGLE(whilegt(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilegt p15.s, x30, x29");
  TEST_SINGLE(whilegt(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilegt p15.d, x30, x29");
  TEST_SINGLE(whilegt(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilegt p15.b, w30, w29");
  TEST_SINGLE(whilegt(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilegt p15.h, w30, w29");
  TEST_SINGLE(whilegt(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilegt p15.s, w30, w29");
  TEST_SINGLE(whilegt(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilegt p15.d, w30, w29");

  TEST_SINGLE(whilelt(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilelt p15.b, x30, x29");
  TEST_SINGLE(whilelt(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilelt p15.h, x30, x29");
  TEST_SINGLE(whilelt(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilelt p15.s, x30, x29");
  TEST_SINGLE(whilelt(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilelt p15.d, x30, x29");
  TEST_SINGLE(whilelt(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilelt p15.b, w30, w29");
  TEST_SINGLE(whilelt(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilelt p15.h, w30, w29");
  TEST_SINGLE(whilelt(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilelt p15.s, w30, w29");
  TEST_SINGLE(whilelt(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilelt p15.d, w30, w29");

  TEST_SINGLE(whilele(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilele p15.b, x30, x29");
  TEST_SINGLE(whilele(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilele p15.h, x30, x29");
  TEST_SINGLE(whilele(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilele p15.s, x30, x29");
  TEST_SINGLE(whilele(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilele p15.d, x30, x29");
  TEST_SINGLE(whilele(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilele p15.b, w30, w29");
  TEST_SINGLE(whilele(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilele p15.h, w30, w29");
  TEST_SINGLE(whilele(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilele p15.s, w30, w29");
  TEST_SINGLE(whilele(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilele p15.d, w30, w29");

  TEST_SINGLE(whilehs(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilehs p15.b, x30, x29");
  TEST_SINGLE(whilehs(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilehs p15.h, x30, x29");
  TEST_SINGLE(whilehs(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilehs p15.s, x30, x29");
  TEST_SINGLE(whilehs(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilehs p15.d, x30, x29");
  TEST_SINGLE(whilehs(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilehs p15.b, w30, w29");
  TEST_SINGLE(whilehs(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilehs p15.h, w30, w29");
  TEST_SINGLE(whilehs(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilehs p15.s, w30, w29");
  TEST_SINGLE(whilehs(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilehs p15.d, w30, w29");

  TEST_SINGLE(whilehi(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilehi p15.b, x30, x29");
  TEST_SINGLE(whilehi(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilehi p15.h, x30, x29");
  TEST_SINGLE(whilehi(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilehi p15.s, x30, x29");
  TEST_SINGLE(whilehi(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilehi p15.d, x30, x29");
  TEST_SINGLE(whilehi(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilehi p15.b, w30, w29");
  TEST_SINGLE(whilehi(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilehi p15.h, w30, w29");
  TEST_SINGLE(whilehi(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilehi p15.s, w30, w29");
  TEST_SINGLE(whilehi(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilehi p15.d, w30, w29");

  TEST_SINGLE(whilelo(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilelo p15.b, x30, x29");
  TEST_SINGLE(whilelo(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilelo p15.h, x30, x29");
  TEST_SINGLE(whilelo(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilelo p15.s, x30, x29");
  TEST_SINGLE(whilelo(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilelo p15.d, x30, x29");
  TEST_SINGLE(whilelo(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilelo p15.b, w30, w29");
  TEST_SINGLE(whilelo(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilelo p15.h, w30, w29");
  TEST_SINGLE(whilelo(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilelo p15.s, w30, w29");
  TEST_SINGLE(whilelo(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilelo p15.d, w30, w29");

  TEST_SINGLE(whilels(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilels p15.b, x30, x29");
  TEST_SINGLE(whilels(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilels p15.h, x30, x29");
  TEST_SINGLE(whilels(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilels p15.s, x30, x29");
  TEST_SINGLE(whilels(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilels p15.d, x30, x29");
  TEST_SINGLE(whilels(SubRegSize::i8Bit, PReg::p15, WReg::w30, WReg::w29), "whilels p15.b, w30, w29");
  TEST_SINGLE(whilels(SubRegSize::i16Bit, PReg::p15, WReg::w30, WReg::w29), "whilels p15.h, w30, w29");
  TEST_SINGLE(whilels(SubRegSize::i32Bit, PReg::p15, WReg::w30, WReg::w29), "whilels p15.s, w30, w29");
  TEST_SINGLE(whilels(SubRegSize::i64Bit, PReg::p15, WReg::w30, WReg::w29), "whilels p15.d, w30, w29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally terminate scalars") {
  TEST_SINGLE(ctermeq(XReg::x30, XReg::x29), "ctermeq x30, x29");
  TEST_SINGLE(ctermeq(WReg::w30, WReg::w29), "ctermeq w30, w29");

  TEST_SINGLE(ctermne(XReg::x30, XReg::x29), "ctermne x30, x29");
  TEST_SINGLE(ctermne(WReg::w30, WReg::w29), "ctermne w30, w29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE pointer conflict compare") {
  TEST_SINGLE(whilewr(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilewr p15.b, x30, x29");
  TEST_SINGLE(whilewr(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilewr p15.h, x30, x29");
  TEST_SINGLE(whilewr(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilewr p15.s, x30, x29");
  TEST_SINGLE(whilewr(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilewr p15.d, x30, x29");

  TEST_SINGLE(whilerw(SubRegSize::i8Bit, PReg::p15, XReg::x30, XReg::x29), "whilerw p15.b, x30, x29");
  TEST_SINGLE(whilerw(SubRegSize::i16Bit, PReg::p15, XReg::x30, XReg::x29), "whilerw p15.h, x30, x29");
  TEST_SINGLE(whilerw(SubRegSize::i32Bit, PReg::p15, XReg::x30, XReg::x29), "whilerw p15.s, x30, x29");
  TEST_SINGLE(whilerw(SubRegSize::i64Bit, PReg::p15, XReg::x30, XReg::x29), "whilerw p15.d, x30, x29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract immediate (unpredicated)") {
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "add z30.b, z30.b, #0");
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "add z30.b, z30.b, #127");
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "add z30.b, z30.b, #255");

  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "add z30.h, z30.h, #0");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "add z30.h, z30.h, #127");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "add z30.h, z30.h, #255");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "add z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "add z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "add z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "add z30.s, z30.s, #0");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "add z30.s, z30.s, #127");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "add z30.s, z30.s, #255");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "add z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "add z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "add z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "add z30.d, z30.d, #0");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "add z30.d, z30.d, #127");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "add z30.d, z30.d, #255");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "add z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "add z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "add z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "sub z30.b, z30.b, #0");
  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "sub z30.b, z30.b, #127");
  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "sub z30.b, z30.b, #255");

  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "sub z30.h, z30.h, #0");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "sub z30.h, z30.h, #127");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "sub z30.h, z30.h, #255");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "sub z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "sub z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "sub z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "sub z30.s, z30.s, #0");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "sub z30.s, z30.s, #127");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "sub z30.s, z30.s, #255");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "sub z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "sub z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "sub z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "sub z30.d, z30.d, #0");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "sub z30.d, z30.d, #127");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "sub z30.d, z30.d, #255");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "sub z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "sub z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "sub z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(subr(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "subr z30.b, z30.b, #0");
  TEST_SINGLE(subr(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "subr z30.b, z30.b, #127");
  TEST_SINGLE(subr(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "subr z30.b, z30.b, #255");

  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "subr z30.h, z30.h, #0");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "subr z30.h, z30.h, #127");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "subr z30.h, z30.h, #255");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "subr z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "subr z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "subr z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "subr z30.s, z30.s, #0");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "subr z30.s, z30.s, #127");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "subr z30.s, z30.s, #255");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "subr z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "subr z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "subr z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "subr z30.d, z30.d, #0");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "subr z30.d, z30.d, #127");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "subr z30.d, z30.d, #255");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "subr z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "subr z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "subr z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "sqadd z30.b, z30.b, #0");
  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "sqadd z30.b, z30.b, #127");
  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "sqadd z30.b, z30.b, #255");

  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "sqadd z30.h, z30.h, #0");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "sqadd z30.h, z30.h, #127");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "sqadd z30.h, z30.h, #255");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "sqadd z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "sqadd z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "sqadd z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "sqadd z30.s, z30.s, #0");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "sqadd z30.s, z30.s, #127");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "sqadd z30.s, z30.s, #255");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "sqadd z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "sqadd z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "sqadd z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "sqadd z30.d, z30.d, #0");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "sqadd z30.d, z30.d, #127");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "sqadd z30.d, z30.d, #255");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "sqadd z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "sqadd z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "sqadd z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "uqadd z30.b, z30.b, #0");
  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "uqadd z30.b, z30.b, #127");
  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "uqadd z30.b, z30.b, #255");

  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "uqadd z30.h, z30.h, #0");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "uqadd z30.h, z30.h, #127");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "uqadd z30.h, z30.h, #255");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "uqadd z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "uqadd z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "uqadd z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "uqadd z30.s, z30.s, #0");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "uqadd z30.s, z30.s, #127");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "uqadd z30.s, z30.s, #255");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "uqadd z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "uqadd z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "uqadd z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "uqadd z30.d, z30.d, #0");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "uqadd z30.d, z30.d, #127");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "uqadd z30.d, z30.d, #255");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "uqadd z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "uqadd z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "uqadd z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "sqsub z30.b, z30.b, #0");
  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "sqsub z30.b, z30.b, #127");
  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "sqsub z30.b, z30.b, #255");

  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "sqsub z30.h, z30.h, #0");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "sqsub z30.h, z30.h, #127");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "sqsub z30.h, z30.h, #255");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "sqsub z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "sqsub z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "sqsub z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "sqsub z30.s, z30.s, #0");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "sqsub z30.s, z30.s, #127");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "sqsub z30.s, z30.s, #255");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "sqsub z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "sqsub z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "sqsub z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "sqsub z30.d, z30.d, #0");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "sqsub z30.d, z30.d, #127");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "sqsub z30.d, z30.d, #255");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "sqsub z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "sqsub z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "sqsub z30.d, z30.d, #255, lsl #8");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "uqsub z30.b, z30.b, #0");
  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "uqsub z30.b, z30.b, #127");
  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "uqsub z30.b, z30.b, #255");

  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "uqsub z30.h, z30.h, #0");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "uqsub z30.h, z30.h, #127");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "uqsub z30.h, z30.h, #255");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 256), "uqsub z30.h, z30.h, #1, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 32512), "uqsub z30.h, z30.h, #127, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 65280), "uqsub z30.h, z30.h, #255, lsl #8");

  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "uqsub z30.s, z30.s, #0");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "uqsub z30.s, z30.s, #127");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "uqsub z30.s, z30.s, #255");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 256), "uqsub z30.s, z30.s, #1, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 32512), "uqsub z30.s, z30.s, #127, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 65280), "uqsub z30.s, z30.s, #255, lsl #8");

  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "uqsub z30.d, z30.d, #0");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "uqsub z30.d, z30.d, #127");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "uqsub z30.d, z30.d, #255");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 256), "uqsub z30.d, z30.d, #1, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 32512), "uqsub z30.d, z30.d, #127, lsl #8");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 65280), "uqsub z30.d, z30.d, #255, lsl #8");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer min/max immediate (unpredicated)") {
  TEST_SINGLE(smax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "smax z30.b, z30.b, #0");
  TEST_SINGLE(smax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, -128), "smax z30.b, z30.b, #-128");
  TEST_SINGLE(smax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "smax z30.b, z30.b, #127");

  TEST_SINGLE(smax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "smax z30.h, z30.h, #0");
  TEST_SINGLE(smax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, -128), "smax z30.h, z30.h, #-128");
  TEST_SINGLE(smax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "smax z30.h, z30.h, #127");

  TEST_SINGLE(smax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "smax z30.s, z30.s, #0");
  TEST_SINGLE(smax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, -128), "smax z30.s, z30.s, #-128");
  TEST_SINGLE(smax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "smax z30.s, z30.s, #127");

  TEST_SINGLE(smax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "smax z30.d, z30.d, #0");
  TEST_SINGLE(smax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, -128), "smax z30.d, z30.d, #-128");
  TEST_SINGLE(smax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "smax z30.d, z30.d, #127");

  TEST_SINGLE(smin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "smin z30.b, z30.b, #0");
  TEST_SINGLE(smin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, -128), "smin z30.b, z30.b, #-128");
  TEST_SINGLE(smin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "smin z30.b, z30.b, #127");

  TEST_SINGLE(smin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "smin z30.h, z30.h, #0");
  TEST_SINGLE(smin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, -128), "smin z30.h, z30.h, #-128");
  TEST_SINGLE(smin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "smin z30.h, z30.h, #127");

  TEST_SINGLE(smin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "smin z30.s, z30.s, #0");
  TEST_SINGLE(smin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, -128), "smin z30.s, z30.s, #-128");
  TEST_SINGLE(smin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "smin z30.s, z30.s, #127");

  TEST_SINGLE(smin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "smin z30.d, z30.d, #0");
  TEST_SINGLE(smin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, -128), "smin z30.d, z30.d, #-128");
  TEST_SINGLE(smin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "smin z30.d, z30.d, #127");

  TEST_SINGLE(umax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "umax z30.b, z30.b, #0");
  TEST_SINGLE(umax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "umax z30.b, z30.b, #127");
  TEST_SINGLE(umax(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "umax z30.b, z30.b, #255");

  TEST_SINGLE(umax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "umax z30.h, z30.h, #0");
  TEST_SINGLE(umax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "umax z30.h, z30.h, #127");
  TEST_SINGLE(umax(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "umax z30.h, z30.h, #255");

  TEST_SINGLE(umax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "umax z30.s, z30.s, #0");
  TEST_SINGLE(umax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "umax z30.s, z30.s, #127");
  TEST_SINGLE(umax(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "umax z30.s, z30.s, #255");

  TEST_SINGLE(umax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "umax z30.d, z30.d, #0");
  TEST_SINGLE(umax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "umax z30.d, z30.d, #127");
  TEST_SINGLE(umax(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "umax z30.d, z30.d, #255");

  TEST_SINGLE(umin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "umin z30.b, z30.b, #0");
  TEST_SINGLE(umin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "umin z30.b, z30.b, #127");
  TEST_SINGLE(umin(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 255), "umin z30.b, z30.b, #255");

  TEST_SINGLE(umin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "umin z30.h, z30.h, #0");
  TEST_SINGLE(umin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "umin z30.h, z30.h, #127");
  TEST_SINGLE(umin(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 255), "umin z30.h, z30.h, #255");

  TEST_SINGLE(umin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "umin z30.s, z30.s, #0");
  TEST_SINGLE(umin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "umin z30.s, z30.s, #127");
  TEST_SINGLE(umin(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 255), "umin z30.s, z30.s, #255");

  TEST_SINGLE(umin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "umin z30.d, z30.d, #0");
  TEST_SINGLE(umin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "umin z30.d, z30.d, #127");
  TEST_SINGLE(umin(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 255), "umin z30.d, z30.d, #255");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply immediate (unpredicated)") {
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 0), "mul z30.b, z30.b, #0");
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, -128), "mul z30.b, z30.b, #-128");
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, 127), "mul z30.b, z30.b, #127");

  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 0), "mul z30.h, z30.h, #0");
  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, -128), "mul z30.h, z30.h, #-128");
  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, 127), "mul z30.h, z30.h, #127");

  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 0), "mul z30.s, z30.s, #0");
  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, -128), "mul z30.s, z30.s, #-128");
  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, 127), "mul z30.s, z30.s, #127");

  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 0), "mul z30.d, z30.d, #0");
  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, -128), "mul z30.d, z30.d, #-128");
  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, 127), "mul z30.d, z30.d, #127");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast integer immediate (unpredicated)") {
  TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, -128), "mov z30.b, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, -128), "mov z30.h, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, -128), "mov z30.s, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, -128), "mov z30.d, #-128");

  TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, 127), "mov z30.b, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, 127), "mov z30.h, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, 127), "mov z30.s, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, 127), "mov z30.d, #127");

  // TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, -32768), "mov z30.b, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, -32768), "mov z30.h, #-128, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, -32768), "mov z30.s, #-128, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, -32768), "mov z30.d, #-128, lsl #8");

  // TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, 32512), "mov z30.b, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, 32512), "mov z30.h, #127, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, 32512), "mov z30.s, #127, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, 32512), "mov z30.d, #127, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, -128), "mov z30.b, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, -128), "mov z30.h, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, -128), "mov z30.s, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, -128), "mov z30.d, #-128");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, 127), "mov z30.b, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, 127), "mov z30.h, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, 127), "mov z30.s, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, 127), "mov z30.d, #127");
}

#if TEST_FP16
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast floating-point immediate (predicated) : fp16") {
  TEST_SINGLE(fcpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.h, p6/m, #0xc0 (-0.1250)");
  TEST_SINGLE(fcpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.h, p6/m, #0x60 (0.5000)");
  TEST_SINGLE(fcpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.h, p6/m, #0x70 (1.0000)");
  TEST_SINGLE(fcpy(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.h, p6/m, #0x3f (31.0000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.h, p6/m, #0xc0 (-0.1250)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.h, p6/m, #0x60 (0.5000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.h, p6/m, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.h, p6/m, #0x3f (31.0000)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast floating-point immediate (unpredicated)") {
  TEST_SINGLE(fdup(SubRegSize::i16Bit, ZReg::z30, -0.125), "fmov z30.h, #0xc0 (-0.1250)");
  TEST_SINGLE(fdup(SubRegSize::i16Bit, ZReg::z30, 0.5), "fmov z30.h, #0x60 (0.5000)");
  TEST_SINGLE(fdup(SubRegSize::i16Bit, ZReg::z30, 1.0), "fmov z30.h, #0x70 (1.0000)");
  TEST_SINGLE(fdup(SubRegSize::i16Bit, ZReg::z30, 31.0), "fmov z30.h, #0x3f (31.0000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, -0.125), "fmov z30.h, #0xc0 (-0.1250)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, 0.5), "fmov z30.h, #0x60 (0.5000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, 1.0), "fmov z30.h, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, 31.0), "fmov z30.h, #0x3f (31.0000)");
}
#endif

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast floating-point immediate (predicated)") {
  TEST_SINGLE(fcpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.s, p6/m, #0xc0 (-0.1250)");
  TEST_SINGLE(fcpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.d, p6/m, #0xc0 (-0.1250)");

  TEST_SINGLE(fcpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.s, p6/m, #0x60 (0.5000)");
  TEST_SINGLE(fcpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.d, p6/m, #0x60 (0.5000)");

  TEST_SINGLE(fcpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.s, p6/m, #0x70 (1.0000)");
  TEST_SINGLE(fcpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.d, p6/m, #0x70 (1.0000)");

  TEST_SINGLE(fcpy(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.s, p6/m, #0x3f (31.0000)");
  TEST_SINGLE(fcpy(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.d, p6/m, #0x3f (31.0000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.s, p6/m, #0xc0 (-0.1250)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), -0.125), "fmov z30.d, p6/m, #0xc0 (-0.1250)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.s, p6/m, #0x60 (0.5000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 0.5), "fmov z30.d, p6/m, #0x60 (0.5000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.s, p6/m, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 1.0), "fmov z30.d, p6/m, #0x70 (1.0000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.s, p6/m, #0x3f (31.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), 31.0), "fmov z30.d, p6/m, #0x3f (31.0000)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast floating-point immediate (unpredicated)") {
  TEST_SINGLE(fdup(SubRegSize::i32Bit, ZReg::z30, -0.125), "fmov z30.s, #0xc0 (-0.1250)");
  TEST_SINGLE(fdup(SubRegSize::i64Bit, ZReg::z30, -0.125), "fmov z30.d, #0xc0 (-0.1250)");

  TEST_SINGLE(fdup(SubRegSize::i32Bit, ZReg::z30, 0.5), "fmov z30.s, #0x60 (0.5000)");
  TEST_SINGLE(fdup(SubRegSize::i64Bit, ZReg::z30, 0.5), "fmov z30.d, #0x60 (0.5000)");

  TEST_SINGLE(fdup(SubRegSize::i32Bit, ZReg::z30, 1.0), "fmov z30.s, #0x70 (1.0000)");
  TEST_SINGLE(fdup(SubRegSize::i64Bit, ZReg::z30, 1.0), "fmov z30.d, #0x70 (1.0000)");

  TEST_SINGLE(fdup(SubRegSize::i32Bit, ZReg::z30, 31.0), "fmov z30.s, #0x3f (31.0000)");
  TEST_SINGLE(fdup(SubRegSize::i64Bit, ZReg::z30, 31.0), "fmov z30.d, #0x3f (31.0000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, -0.125), "fmov z30.s, #0xc0 (-0.1250)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, -0.125), "fmov z30.d, #0xc0 (-0.1250)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, 0.5), "fmov z30.s, #0x60 (0.5000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, 0.5), "fmov z30.d, #0x60 (0.5000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, 1.0), "fmov z30.s, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, 1.0), "fmov z30.d, #0x70 (1.0000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, 31.0), "fmov z30.s, #0x3f (31.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, 31.0), "fmov z30.d, #0x3f (31.0000)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate count") {
  TEST_SINGLE(cntp(SubRegSize::i8Bit, XReg::x30, PReg::p15, PReg::p7), "cntp x30, p15, p7.b");
  TEST_SINGLE(cntp(SubRegSize::i16Bit, XReg::x30, PReg::p15, PReg::p7), "cntp x30, p15, p7.h");
  TEST_SINGLE(cntp(SubRegSize::i32Bit, XReg::x30, PReg::p15, PReg::p7), "cntp x30, p15, p7.s");
  TEST_SINGLE(cntp(SubRegSize::i64Bit, XReg::x30, PReg::p15, PReg::p7), "cntp x30, p15, p7.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec vector by predicate count") {
  TEST_SINGLE(sqincp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "sqincp z30.h, p15");
  TEST_SINGLE(sqincp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "sqincp z30.s, p15");
  TEST_SINGLE(sqincp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "sqincp z30.d, p15");

  TEST_SINGLE(uqincp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "uqincp z30.h, p15");
  TEST_SINGLE(uqincp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "uqincp z30.s, p15");
  TEST_SINGLE(uqincp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "uqincp z30.d, p15");

  TEST_SINGLE(sqdecp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "sqdecp z30.h, p15");
  TEST_SINGLE(sqdecp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "sqdecp z30.s, p15");
  TEST_SINGLE(sqdecp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "sqdecp z30.d, p15");

  TEST_SINGLE(uqdecp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "uqdecp z30.h, p15");
  TEST_SINGLE(uqdecp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "uqdecp z30.s, p15");
  TEST_SINGLE(uqdecp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "uqdecp z30.d, p15");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec register by predicate count") {
  TEST_SINGLE(sqincp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "sqincp x30, p15.b");
  TEST_SINGLE(sqincp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "sqincp x30, p15.h");
  TEST_SINGLE(sqincp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "sqincp x30, p15.s");
  TEST_SINGLE(sqincp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "sqincp x30, p15.d");

  TEST_SINGLE(sqincp(SubRegSize::i8Bit, XReg::x30, PReg::p15, WReg::w30), "sqincp x30, p15.b, w30");
  TEST_SINGLE(sqincp(SubRegSize::i16Bit, XReg::x30, PReg::p15, WReg::w30), "sqincp x30, p15.h, w30");
  TEST_SINGLE(sqincp(SubRegSize::i32Bit, XReg::x30, PReg::p15, WReg::w30), "sqincp x30, p15.s, w30");
  TEST_SINGLE(sqincp(SubRegSize::i64Bit, XReg::x30, PReg::p15, WReg::w30), "sqincp x30, p15.d, w30");

  TEST_SINGLE(uqincp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "uqincp x30, p15.b");
  TEST_SINGLE(uqincp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "uqincp x30, p15.h");
  TEST_SINGLE(uqincp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "uqincp x30, p15.s");
  TEST_SINGLE(uqincp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "uqincp x30, p15.d");

  TEST_SINGLE(uqincp(SubRegSize::i8Bit, WReg::w30, PReg::p15), "uqincp w30, p15.b");
  TEST_SINGLE(uqincp(SubRegSize::i16Bit, WReg::w30, PReg::p15), "uqincp w30, p15.h");
  TEST_SINGLE(uqincp(SubRegSize::i32Bit, WReg::w30, PReg::p15), "uqincp w30, p15.s");
  TEST_SINGLE(uqincp(SubRegSize::i64Bit, WReg::w30, PReg::p15), "uqincp w30, p15.d");

  TEST_SINGLE(sqdecp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "sqdecp x30, p15.b");
  TEST_SINGLE(sqdecp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "sqdecp x30, p15.h");
  TEST_SINGLE(sqdecp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "sqdecp x30, p15.s");
  TEST_SINGLE(sqdecp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "sqdecp x30, p15.d");

  TEST_SINGLE(sqdecp(SubRegSize::i8Bit, XReg::x30, PReg::p15, WReg::w30), "sqdecp x30, p15.b, w30");
  TEST_SINGLE(sqdecp(SubRegSize::i16Bit, XReg::x30, PReg::p15, WReg::w30), "sqdecp x30, p15.h, w30");
  TEST_SINGLE(sqdecp(SubRegSize::i32Bit, XReg::x30, PReg::p15, WReg::w30), "sqdecp x30, p15.s, w30");
  TEST_SINGLE(sqdecp(SubRegSize::i64Bit, XReg::x30, PReg::p15, WReg::w30), "sqdecp x30, p15.d, w30");

  TEST_SINGLE(uqdecp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "uqdecp x30, p15.b");
  TEST_SINGLE(uqdecp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "uqdecp x30, p15.h");
  TEST_SINGLE(uqdecp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "uqdecp x30, p15.s");
  TEST_SINGLE(uqdecp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "uqdecp x30, p15.d");

  TEST_SINGLE(uqdecp(SubRegSize::i8Bit, WReg::w30, PReg::p15), "uqdecp w30, p15.b");
  TEST_SINGLE(uqdecp(SubRegSize::i16Bit, WReg::w30, PReg::p15), "uqdecp w30, p15.h");
  TEST_SINGLE(uqdecp(SubRegSize::i32Bit, WReg::w30, PReg::p15), "uqdecp w30, p15.s");
  TEST_SINGLE(uqdecp(SubRegSize::i64Bit, WReg::w30, PReg::p15), "uqdecp w30, p15.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec vector by predicate count") {
  TEST_SINGLE(incp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "incp z30.h, p15");
  TEST_SINGLE(incp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "incp z30.s, p15");
  TEST_SINGLE(incp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "incp z30.d, p15");

  TEST_SINGLE(decp(SubRegSize::i16Bit, ZReg::z30, PReg::p15), "decp z30.h, p15");
  TEST_SINGLE(decp(SubRegSize::i32Bit, ZReg::z30, PReg::p15), "decp z30.s, p15");
  TEST_SINGLE(decp(SubRegSize::i64Bit, ZReg::z30, PReg::p15), "decp z30.d, p15");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec register by predicate count") {
  TEST_SINGLE(incp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "incp x30, p15.b");
  TEST_SINGLE(incp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "incp x30, p15.h");
  TEST_SINGLE(incp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "incp x30, p15.s");
  TEST_SINGLE(incp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "incp x30, p15.d");

  TEST_SINGLE(decp(SubRegSize::i8Bit, XReg::x30, PReg::p15), "decp x30, p15.b");
  TEST_SINGLE(decp(SubRegSize::i16Bit, XReg::x30, PReg::p15), "decp x30, p15.h");
  TEST_SINGLE(decp(SubRegSize::i32Bit, XReg::x30, PReg::p15), "decp x30, p15.s");
  TEST_SINGLE(decp(SubRegSize::i64Bit, XReg::x30, PReg::p15), "decp x30, p15.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE FFR write from predicate") {
  TEST_SINGLE(wrffr(PReg::p7), "wrffr p7.b");
  TEST_SINGLE(wrffr(PReg::p15), "wrffr p15.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE FFR initialise") {
  TEST_SINGLE(setffr(), "setffr");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Integer Multiply-Add - Unpredicated") {
  TEST_SINGLE(cdot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cdot z30.s, z29.b, z28.b, #0");
  TEST_SINGLE(cdot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cdot z30.s, z29.b, z28.b, #90");
  TEST_SINGLE(cdot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cdot z30.s, z29.b, z28.b, #180");
  TEST_SINGLE(cdot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cdot z30.s, z29.b, z28.b, #270");

  TEST_SINGLE(cdot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cdot z30.d, z29.h, z28.h, #0");
  TEST_SINGLE(cdot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cdot z30.d, z29.h, z28.h, #90");
  TEST_SINGLE(cdot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cdot z30.d, z29.h, z28.h, #180");
  TEST_SINGLE(cdot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cdot z30.d, z29.h, z28.h, #270");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer dot product (unpredicated)") {
  TEST_SINGLE(sdot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sdot z30.s, z29.b, z28.b");
  TEST_SINGLE(sdot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sdot z30.d, z29.h, z28.h");

  TEST_SINGLE(udot(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "udot z30.s, z29.b, z28.b");
  TEST_SINGLE(udot(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "udot z30.d, z29.h, z28.h");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add interleaved long") {
  TEST_SINGLE(sqdmlalbt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalbt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlalbt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalbt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlalbt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalbt z30.d, z29.s, z28.s");

  TEST_SINGLE(sqdmlslbt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslbt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlslbt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslbt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlslbt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslbt z30.d, z29.s, z28.s");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 complex integer multiply-add") {
  TEST_SINGLE(cmla(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cmla z30.b, z29.b, z28.b, #0");
  TEST_SINGLE(cmla(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cmla z30.b, z29.b, z28.b, #90");
  TEST_SINGLE(cmla(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cmla z30.b, z29.b, z28.b, #180");
  TEST_SINGLE(cmla(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cmla z30.b, z29.b, z28.b, #270");

  TEST_SINGLE(cmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cmla z30.h, z29.h, z28.h, #0");
  TEST_SINGLE(cmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cmla z30.h, z29.h, z28.h, #90");
  TEST_SINGLE(cmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cmla z30.h, z29.h, z28.h, #180");
  TEST_SINGLE(cmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cmla z30.h, z29.h, z28.h, #270");

  TEST_SINGLE(cmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cmla z30.s, z29.s, z28.s, #0");
  TEST_SINGLE(cmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cmla z30.s, z29.s, z28.s, #90");
  TEST_SINGLE(cmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cmla z30.s, z29.s, z28.s, #180");
  TEST_SINGLE(cmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cmla z30.s, z29.s, z28.s, #270");

  TEST_SINGLE(cmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "cmla z30.d, z29.d, z28.d, #0");
  TEST_SINGLE(cmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "cmla z30.d, z29.d, z28.d, #90");
  TEST_SINGLE(cmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "cmla z30.d, z29.d, z28.d, #180");
  TEST_SINGLE(cmla(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "cmla z30.d, z29.d, z28.d, #270");

  TEST_SINGLE(sqrdcmlah(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "sqrdcmlah z30.b, z29.b, z28.b, #0");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "sqrdcmlah z30.b, z29.b, z28.b, #90");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "sqrdcmlah z30.b, z29.b, z28.b, #180");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "sqrdcmlah z30.b, z29.b, z28.b, #270");

  TEST_SINGLE(sqrdcmlah(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "sqrdcmlah z30.h, z29.h, z28.h, #0");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "sqrdcmlah z30.h, z29.h, z28.h, #90");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "sqrdcmlah z30.h, z29.h, z28.h, #180");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "sqrdcmlah z30.h, z29.h, z28.h, #270");

  TEST_SINGLE(sqrdcmlah(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "sqrdcmlah z30.s, z29.s, z28.s, #0");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "sqrdcmlah z30.s, z29.s, z28.s, #90");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "sqrdcmlah z30.s, z29.s, z28.s, #180");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "sqrdcmlah z30.s, z29.s, z28.s, #270");

  TEST_SINGLE(sqrdcmlah(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_0), "sqrdcmlah z30.d, z29.d, z28.d, #0");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_90), "sqrdcmlah z30.d, z29.d, z28.d, #90");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_180), "sqrdcmlah z30.d, z29.d, z28.d, #180");
  TEST_SINGLE(sqrdcmlah(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28, Rotation::ROTATE_270), "sqrdcmlah z30.d, z29.d, z28.d, #270");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer multiply-add long") {
  TEST_SINGLE(smlalb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalb z30.h, z29.b, z28.b");
  TEST_SINGLE(smlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalb z30.s, z29.h, z28.h");
  TEST_SINGLE(smlalb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalb z30.d, z29.s, z28.s");

  TEST_SINGLE(smlalt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalt z30.h, z29.b, z28.b");
  TEST_SINGLE(smlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalt z30.s, z29.h, z28.h");
  TEST_SINGLE(smlalt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlalt z30.d, z29.s, z28.s");

  TEST_SINGLE(umlalb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalb z30.h, z29.b, z28.b");
  TEST_SINGLE(umlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalb z30.s, z29.h, z28.h");
  TEST_SINGLE(umlalb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalb z30.d, z29.s, z28.s");

  TEST_SINGLE(umlalt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalt z30.h, z29.b, z28.b");
  TEST_SINGLE(umlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalt z30.s, z29.h, z28.h");
  TEST_SINGLE(umlalt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlalt z30.d, z29.s, z28.s");

  TEST_SINGLE(smlslb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslb z30.h, z29.b, z28.b");
  TEST_SINGLE(smlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslb z30.s, z29.h, z28.h");
  TEST_SINGLE(smlslb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslb z30.d, z29.s, z28.s");

  TEST_SINGLE(smlslt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslt z30.h, z29.b, z28.b");
  TEST_SINGLE(smlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslt z30.s, z29.h, z28.h");
  TEST_SINGLE(smlslt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smlslt z30.d, z29.s, z28.s");

  TEST_SINGLE(umlslb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslb z30.h, z29.b, z28.b");
  TEST_SINGLE(umlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslb z30.s, z29.h, z28.h");
  TEST_SINGLE(umlslb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslb z30.d, z29.s, z28.s");

  TEST_SINGLE(umlslt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslt z30.h, z29.b, z28.b");
  TEST_SINGLE(umlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslt z30.s, z29.h, z28.h");
  TEST_SINGLE(umlslt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umlslt z30.d, z29.s, z28.s");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add long") {
  TEST_SINGLE(sqdmlalb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalb z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalb z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlalb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalb z30.d, z29.s, z28.s");

  TEST_SINGLE(sqdmlalt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlalt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlalt z30.d, z29.s, z28.s");

  TEST_SINGLE(sqdmlslb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslb z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslb z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlslb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslb z30.d, z29.s, z28.s");

  TEST_SINGLE(sqdmlslt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmlslt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmlslt z30.d, z29.s, z28.s");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add high") {
  TEST_SINGLE(sqrdmlah(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlah z30.b, z29.b, z28.b");
  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlah z30.h, z29.h, z28.h");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlah z30.s, z29.s, z28.s");
  TEST_SINGLE(sqrdmlah(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlah z30.d, z29.d, z28.d");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlsh z30.b, z29.b, z28.b");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlsh z30.h, z29.h, z28.h");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlsh z30.s, z29.s, z28.s");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqrdmlsh z30.d, z29.d, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE mixed sign dot product") {
  TEST_SINGLE(usdot(ZReg::z30, ZReg::z29, ZReg::z28), "usdot z30.s, z29.b, z28.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer pairwise add and accumulate long") {
  TEST_SINGLE(sadalp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sadalp z30.h, p6/m, z29.b");
  TEST_SINGLE(sadalp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sadalp z30.s, p6/m, z29.h");
  TEST_SINGLE(sadalp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sadalp z30.d, p6/m, z29.s");

  TEST_SINGLE(uadalp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uadalp z30.h, p6/m, z29.b");
  TEST_SINGLE(uadalp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uadalp z30.s, p6/m, z29.h");
  TEST_SINGLE(uadalp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uadalp z30.d, p6/m, z29.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer unary operations (predicated)") {
  TEST_SINGLE(urecpe(ZReg::z30, PReg::p6.Merging(), ZReg::z29), "urecpe z30.s, p6/m, z29.s");

  TEST_SINGLE(ursqrte(ZReg::z30, PReg::p6.Merging(), ZReg::z29), "ursqrte z30.s, p6/m, z29.s");

  TEST_SINGLE(sqabs(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqabs z30.b, p6/m, z29.b");
  TEST_SINGLE(sqabs(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqabs z30.h, p6/m, z29.h");
  TEST_SINGLE(sqabs(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqabs z30.s, p6/m, z29.s");
  TEST_SINGLE(sqabs(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqabs z30.d, p6/m, z29.d");

  TEST_SINGLE(sqneg(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqneg z30.b, p6/m, z29.b");
  TEST_SINGLE(sqneg(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqneg z30.h, p6/m, z29.h");
  TEST_SINGLE(sqneg(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqneg z30.s, p6/m, z29.s");
  TEST_SINGLE(sqneg(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sqneg z30.d, p6/m, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating/rounding bitwise shift left (predicated)") {
  TEST_SINGLE(srshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(srshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(srshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(srshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(urshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(urshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(urshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(urshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(srshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(srshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(srshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(srshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "srshlr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(urshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(urshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(urshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(urshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "urshlr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(uqshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(sqrshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(sqrshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(sqrshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(sqrshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(uqrshl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(uqrshl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(uqrshl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(uqrshl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(sqshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(sqshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(sqshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(sqshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqshlr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(uqshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(uqshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(uqshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(uqshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqshlr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(sqrshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(sqrshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(sqrshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(sqrshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "sqrshlr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(uqrshlr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshlr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(uqrshlr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshlr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(uqrshlr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshlr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(uqrshlr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "uqrshlr z30.d, p6/m, z30.d, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer halving add/subtract (predicated)") {
  TEST_SINGLE(shadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(shadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(uhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.q, p6/m, z30.q, z28.q");
  TEST_SINGLE(shsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(shsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(uhsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(srhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(srhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(srhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(srhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(srhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(urhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(urhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(urhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(urhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(urhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(shsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(shsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(uhsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer pairwise arithmetic") {
  TEST_SINGLE(addp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(addp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(addp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(addp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(addp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(smaxp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(smaxp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(smaxp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(smaxp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(smaxp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(umaxp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(umaxp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(umaxp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(umaxp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(umaxp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.q, p6/m, z30.q, z28.q");


  TEST_SINGLE(sminp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(sminp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(sminp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(sminp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(sminp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.q, p6/m, z30.q, z28.q");


  TEST_SINGLE(uminp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uminp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uminp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uminp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(uminp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating add/subtract") {
  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqadd z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqadd z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqadd z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqadd z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqadd z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqadd z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqadd z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqadd z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsub z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsub z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsub z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsub z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsub z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsub z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsub z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsub z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(suqadd(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "suqadd z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(suqadd(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "suqadd z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(suqadd(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "suqadd z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(suqadd(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "suqadd z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(usqadd(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "usqadd z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(usqadd(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "usqadd z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(usqadd(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "usqadd z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(usqadd(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "usqadd z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(sqsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsubr z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(sqsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsubr z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(sqsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsubr z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(sqsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "sqsubr z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(uqsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsubr z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(uqsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsubr z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(uqsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsubr z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(uqsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z28), "uqsubr z30.d, p7/m, z30.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract long") {
  // TEST_SINGLE(saddlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.b, z29.b, z28.b");
  TEST_SINGLE(saddlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.h, z29.b, z28.b");
  TEST_SINGLE(saddlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.s, z29.h, z28.h");
  TEST_SINGLE(saddlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.d, z29.s, z28.s");

  // TEST_SINGLE(saddlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.b, z29.b, z28.b");
  TEST_SINGLE(saddlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.h, z29.b, z28.b");
  TEST_SINGLE(saddlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.s, z29.h, z28.h");
  TEST_SINGLE(saddlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.d, z29.s, z28.s");

  // TEST_SINGLE(uaddlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.b, z29.b, z28.b");
  TEST_SINGLE(uaddlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.h, z29.b, z28.b");
  TEST_SINGLE(uaddlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.s, z29.h, z28.h");
  TEST_SINGLE(uaddlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.d, z29.s, z28.s");

  // TEST_SINGLE(uaddlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.b, z29.b, z28.b");
  TEST_SINGLE(uaddlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.h, z29.b, z28.b");
  TEST_SINGLE(uaddlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.s, z29.h, z28.h");
  TEST_SINGLE(uaddlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.d, z29.s, z28.s");

  // TEST_SINGLE(ssublb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.b, z29.b, z28.b");
  TEST_SINGLE(ssublb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.h, z29.b, z28.b");
  TEST_SINGLE(ssublb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.s, z29.h, z28.h");
  TEST_SINGLE(ssublb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.d, z29.s, z28.s");

  // TEST_SINGLE(ssublt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.b, z29.b, z28.b");
  TEST_SINGLE(ssublt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.h, z29.b, z28.b");
  TEST_SINGLE(ssublt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.s, z29.h, z28.h");
  TEST_SINGLE(ssublt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.d, z29.s, z28.s");

  // TEST_SINGLE(usublb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.b, z29.b, z28.b");
  TEST_SINGLE(usublb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.h, z29.b, z28.b");
  TEST_SINGLE(usublb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.s, z29.h, z28.h");
  TEST_SINGLE(usublb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.d, z29.s, z28.s");

  // TEST_SINGLE(usublt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.b, z29.b, z28.b");
  TEST_SINGLE(usublt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.h, z29.b, z28.b");
  TEST_SINGLE(usublt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.s, z29.h, z28.h");
  TEST_SINGLE(usublt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.d, z29.s, z28.s");

  // TEST_SINGLE(sabdlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.b, z29.b, z28.b");
  TEST_SINGLE(sabdlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.h, z29.b, z28.b");
  TEST_SINGLE(sabdlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.s, z29.h, z28.h");
  TEST_SINGLE(sabdlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.d, z29.s, z28.s");

  // TEST_SINGLE(sabdlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.b, z29.b, z28.b");
  TEST_SINGLE(sabdlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.h, z29.b, z28.b");
  TEST_SINGLE(sabdlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.s, z29.h, z28.h");
  TEST_SINGLE(sabdlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.d, z29.s, z28.s");

  // TEST_SINGLE(uabdlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.b, z29.b, z28.b");
  TEST_SINGLE(uabdlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.h, z29.b, z28.b");
  TEST_SINGLE(uabdlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.s, z29.h, z28.h");
  TEST_SINGLE(uabdlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.d, z29.s, z28.s");

  // TEST_SINGLE(uabdlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.b, z29.b, z28.b");
  TEST_SINGLE(uabdlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.h, z29.b, z28.b");
  TEST_SINGLE(uabdlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.s, z29.h, z28.h");
  TEST_SINGLE(uabdlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.d, z29.s, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract wide") {
  TEST_SINGLE(saddwb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwb z30.h, z29.h, z28.b");
  TEST_SINGLE(saddwb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwb z30.s, z29.s, z28.h");
  TEST_SINGLE(saddwb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwb z30.d, z29.d, z28.s");

  TEST_SINGLE(saddwt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwt z30.h, z29.h, z28.b");
  TEST_SINGLE(saddwt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwt z30.s, z29.s, z28.h");
  TEST_SINGLE(saddwt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddwt z30.d, z29.d, z28.s");

  TEST_SINGLE(uaddwb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwb z30.h, z29.h, z28.b");
  TEST_SINGLE(uaddwb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwb z30.s, z29.s, z28.h");
  TEST_SINGLE(uaddwb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwb z30.d, z29.d, z28.s");

  TEST_SINGLE(uaddwt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwt z30.h, z29.h, z28.b");
  TEST_SINGLE(uaddwt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwt z30.s, z29.s, z28.h");
  TEST_SINGLE(uaddwt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddwt z30.d, z29.d, z28.s");

  TEST_SINGLE(ssubwb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwb z30.h, z29.h, z28.b");
  TEST_SINGLE(ssubwb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwb z30.s, z29.s, z28.h");
  TEST_SINGLE(ssubwb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwb z30.d, z29.d, z28.s");

  TEST_SINGLE(ssubwt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwt z30.h, z29.h, z28.b");
  TEST_SINGLE(ssubwt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwt z30.s, z29.s, z28.h");
  TEST_SINGLE(ssubwt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubwt z30.d, z29.d, z28.s");

  TEST_SINGLE(usubwb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwb z30.h, z29.h, z28.b");
  TEST_SINGLE(usubwb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwb z30.s, z29.s, z28.h");
  TEST_SINGLE(usubwb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwb z30.d, z29.d, z28.s");

  TEST_SINGLE(usubwt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwt z30.h, z29.h, z28.b");
  TEST_SINGLE(usubwt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwt z30.s, z29.s, z28.h");
  TEST_SINGLE(usubwt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usubwt z30.d, z29.d, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer multiply long") {
  // TEST_SINGLE(sqdmullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.b, z29.b, z28.b");
  TEST_SINGLE(sqdmullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.d, z29.s, z28.s");

  // TEST_SINGLE(sqdmullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.b, z29.b, z28.b");
  TEST_SINGLE(sqdmullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.d, z29.s, z28.s");

  // TEST_SINGLE(pmullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.b, z29.b, z28.b");
  TEST_SINGLE(pmullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.h, z29.b, z28.b");
  // TEST_SINGLE(pmullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.s, z29.h, z28.h");
  TEST_SINGLE(pmullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.d, z29.s, z28.s");

  // TEST_SINGLE(pmullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.b, z29.b, z28.b");
  TEST_SINGLE(pmullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.h, z29.b, z28.b");
  // TEST_SINGLE(pmullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.s, z29.h, z28.h");
  TEST_SINGLE(pmullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.d, z29.s, z28.s");

  // TEST_SINGLE(smullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.b, z29.b, z28.b");
  TEST_SINGLE(smullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.h, z29.b, z28.b");
  TEST_SINGLE(smullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.s, z29.h, z28.h");
  TEST_SINGLE(smullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.d, z29.s, z28.s");

  // TEST_SINGLE(smullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.b, z29.b, z28.b");
  TEST_SINGLE(smullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.h, z29.b, z28.b");
  TEST_SINGLE(smullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.s, z29.h, z28.h");
  TEST_SINGLE(smullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.d, z29.s, z28.s");

  // TEST_SINGLE(umullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.b, z29.b, z28.b");
  TEST_SINGLE(umullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.h, z29.b, z28.b");
  TEST_SINGLE(umullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.s, z29.h, z28.h");
  TEST_SINGLE(umullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.d, z29.s, z28.s");

  // TEST_SINGLE(umullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.b, z29.b, z28.b");
  TEST_SINGLE(umullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.h, z29.b, z28.b");
  TEST_SINGLE(umullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.s, z29.h, z28.h");
  TEST_SINGLE(umullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.d, z29.s, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift left long") {
  TEST_SINGLE(sshllb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "sshllb z30.h, z29.b, #0");
  TEST_SINGLE(sshllb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 7), "sshllb z30.h, z29.b, #7");
  TEST_SINGLE(sshllb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "sshllb z30.s, z29.h, #0");
  TEST_SINGLE(sshllb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "sshllb z30.s, z29.h, #15");
  TEST_SINGLE(sshllb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "sshllb z30.d, z29.s, #0");
  TEST_SINGLE(sshllb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "sshllb z30.d, z29.s, #31");

  TEST_SINGLE(sshllt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "sshllt z30.h, z29.b, #0");
  TEST_SINGLE(sshllt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 7), "sshllt z30.h, z29.b, #7");
  TEST_SINGLE(sshllt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "sshllt z30.s, z29.h, #0");
  TEST_SINGLE(sshllt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "sshllt z30.s, z29.h, #15");
  TEST_SINGLE(sshllt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "sshllt z30.d, z29.s, #0");
  TEST_SINGLE(sshllt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "sshllt z30.d, z29.s, #31");

  TEST_SINGLE(ushllb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "ushllb z30.h, z29.b, #0");
  TEST_SINGLE(ushllb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 7), "ushllb z30.h, z29.b, #7");
  TEST_SINGLE(ushllb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "ushllb z30.s, z29.h, #0");
  TEST_SINGLE(ushllb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "ushllb z30.s, z29.h, #15");
  TEST_SINGLE(ushllb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "ushllb z30.d, z29.s, #0");
  TEST_SINGLE(ushllb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "ushllb z30.d, z29.s, #31");

  TEST_SINGLE(ushllt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "ushllt z30.h, z29.b, #0");
  TEST_SINGLE(ushllt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 7), "ushllt z30.h, z29.b, #7");
  TEST_SINGLE(ushllt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "ushllt z30.s, z29.h, #0");
  TEST_SINGLE(ushllt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "ushllt z30.s, z29.h, #15");
  TEST_SINGLE(ushllt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "ushllt z30.d, z29.s, #0");
  TEST_SINGLE(ushllt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "ushllt z30.d, z29.s, #31");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract interleaved long") {
  TEST_SINGLE(saddlbt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlbt z30.h, z29.b, z28.b");
  TEST_SINGLE(saddlbt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlbt z30.s, z29.h, z28.h");
  TEST_SINGLE(saddlbt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlbt z30.d, z29.s, z28.s");

  TEST_SINGLE(ssublbt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublbt z30.h, z29.b, z28.b");
  TEST_SINGLE(ssublbt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublbt z30.s, z29.h, z28.h");
  TEST_SINGLE(ssublbt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublbt z30.d, z29.s, z28.s");

  TEST_SINGLE(ssubltb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubltb z30.h, z29.b, z28.b");
  TEST_SINGLE(ssubltb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubltb z30.s, z29.h, z28.h");
  TEST_SINGLE(ssubltb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssubltb z30.d, z29.s, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise exclusive-or interleaved") {
  TEST_SINGLE(eorbt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eorbt z30.b, z29.b, z28.b");
  TEST_SINGLE(eorbt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eorbt z30.h, z29.h, z28.h");
  TEST_SINGLE(eorbt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eorbt z30.s, z29.s, z28.s");
  TEST_SINGLE(eorbt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eorbt z30.d, z29.d, z28.d");

  TEST_SINGLE(eortb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eortb z30.b, z29.b, z28.b");
  TEST_SINGLE(eortb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eortb z30.h, z29.h, z28.h");
  TEST_SINGLE(eortb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eortb z30.s, z29.s, z28.s");
  TEST_SINGLE(eortb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "eortb z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer matrix multiply accumulate") {
  TEST_SINGLE(smmla(ZReg::z30, ZReg::z29, ZReg::z28), "smmla z30.s, z29.b, z28.b");
  TEST_SINGLE(usmmla(ZReg::z30, ZReg::z29, ZReg::z28), "usmmla z30.s, z29.b, z28.b");
  TEST_SINGLE(ummla(ZReg::z30, ZReg::z29, ZReg::z28), "ummla z30.s, z29.b, z28.b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise permute") {
  TEST_SINGLE(bext(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bext z30.b, z29.b, z28.b");
  TEST_SINGLE(bext(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bext z30.h, z29.h, z28.h");
  TEST_SINGLE(bext(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bext z30.s, z29.s, z28.s");
  TEST_SINGLE(bext(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bext z30.d, z29.d, z28.d");

  TEST_SINGLE(bdep(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bdep z30.b, z29.b, z28.b");
  TEST_SINGLE(bdep(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bdep z30.h, z29.h, z28.h");
  TEST_SINGLE(bdep(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bdep z30.s, z29.s, z28.s");
  TEST_SINGLE(bdep(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bdep z30.d, z29.d, z28.d");

  TEST_SINGLE(bgrp(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bgrp z30.b, z29.b, z28.b");
  TEST_SINGLE(bgrp(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bgrp z30.h, z29.h, z28.h");
  TEST_SINGLE(bgrp(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bgrp z30.s, z29.s, z28.s");
  TEST_SINGLE(bgrp(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bgrp z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 complex integer add") {
  TEST_SINGLE(cadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "cadd z30.b, z30.b, z29.b, #90");
  TEST_SINGLE(cadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "cadd z30.h, z30.h, z29.h, #90");
  TEST_SINGLE(cadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "cadd z30.s, z30.s, z29.s, #90");
  TEST_SINGLE(cadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "cadd z30.d, z30.d, z29.d, #90");

  TEST_SINGLE(cadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "cadd z30.b, z30.b, z29.b, #270");
  TEST_SINGLE(cadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "cadd z30.h, z30.h, z29.h, #270");
  TEST_SINGLE(cadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "cadd z30.s, z30.s, z29.s, #270");
  TEST_SINGLE(cadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "cadd z30.d, z30.d, z29.d, #270");

  TEST_SINGLE(sqcadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "sqcadd z30.b, z30.b, z29.b, #90");
  TEST_SINGLE(sqcadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "sqcadd z30.h, z30.h, z29.h, #90");
  TEST_SINGLE(sqcadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "sqcadd z30.s, z30.s, z29.s, #90");
  TEST_SINGLE(sqcadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_90), "sqcadd z30.d, z30.d, z29.d, #90");

  TEST_SINGLE(sqcadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "sqcadd z30.b, z30.b, z29.b, #270");
  TEST_SINGLE(sqcadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "sqcadd z30.h, z30.h, z29.h, #270");
  TEST_SINGLE(sqcadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "sqcadd z30.s, z30.s, z29.s, #270");
  TEST_SINGLE(sqcadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, ZReg::z29, Rotation::ROTATE_270), "sqcadd z30.d, z30.d, z29.d, #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer absolute difference and accumulate long") {
  TEST_SINGLE(sabalb(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalb z28.h, z29.b, z30.b");
  TEST_SINGLE(sabalb(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalb z28.s, z29.h, z30.h");
  TEST_SINGLE(sabalb(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalb z28.d, z29.s, z30.s");

  TEST_SINGLE(sabalt(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalt z28.h, z29.b, z30.b");
  TEST_SINGLE(sabalt(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalt z28.s, z29.h, z30.h");
  TEST_SINGLE(sabalt(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sabalt z28.d, z29.s, z30.s");

  TEST_SINGLE(uabalb(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalb z28.h, z29.b, z30.b");
  TEST_SINGLE(uabalb(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalb z28.s, z29.h, z30.h");
  TEST_SINGLE(uabalb(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalb z28.d, z29.s, z30.s");

  TEST_SINGLE(uabalt(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalt z28.h, z29.b, z30.b");
  TEST_SINGLE(uabalt(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalt z28.s, z29.h, z30.h");
  TEST_SINGLE(uabalt(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uabalt z28.d, z29.s, z30.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract long with carry") {
  TEST_SINGLE(adclb(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "adclb z28.s, z29.s, z30.s");
  TEST_SINGLE(adclb(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "adclb z28.d, z29.d, z30.d");

  TEST_SINGLE(adclt(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "adclt z28.s, z29.s, z30.s");
  TEST_SINGLE(adclt(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "adclt z28.d, z29.d, z30.d");

  TEST_SINGLE(sbclb(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sbclb z28.s, z29.s, z30.s");
  TEST_SINGLE(sbclb(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sbclb z28.d, z29.d, z30.d");

  TEST_SINGLE(sbclt(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sbclt z28.s, z29.s, z30.s");
  TEST_SINGLE(sbclt(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "sbclt z28.d, z29.d, z30.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift right and accumulate") {
  TEST_SINGLE(ssra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "ssra z30.b, z29.b, #1");
  TEST_SINGLE(ssra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "ssra z30.b, z29.b, #8");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "ssra z30.h, z29.h, #1");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "ssra z30.h, z29.h, #16");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "ssra z30.s, z29.s, #1");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "ssra z30.s, z29.s, #32");
  TEST_SINGLE(ssra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "ssra z30.d, z29.d, #1");
  TEST_SINGLE(ssra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "ssra z30.d, z29.d, #64");

  TEST_SINGLE(usra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "usra z30.b, z29.b, #1");
  TEST_SINGLE(usra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "usra z30.b, z29.b, #8");
  TEST_SINGLE(usra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "usra z30.h, z29.h, #1");
  TEST_SINGLE(usra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "usra z30.h, z29.h, #16");
  TEST_SINGLE(usra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "usra z30.s, z29.s, #1");
  TEST_SINGLE(usra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "usra z30.s, z29.s, #32");
  TEST_SINGLE(usra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "usra z30.d, z29.d, #1");
  TEST_SINGLE(usra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "usra z30.d, z29.d, #64");

  TEST_SINGLE(srsra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "srsra z30.b, z29.b, #1");
  TEST_SINGLE(srsra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "srsra z30.b, z29.b, #8");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "srsra z30.h, z29.h, #1");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "srsra z30.h, z29.h, #16");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "srsra z30.s, z29.s, #1");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "srsra z30.s, z29.s, #32");
  TEST_SINGLE(srsra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "srsra z30.d, z29.d, #1");
  TEST_SINGLE(srsra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "srsra z30.d, z29.d, #64");

  TEST_SINGLE(ursra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "ursra z30.b, z29.b, #1");
  TEST_SINGLE(ursra(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "ursra z30.b, z29.b, #8");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "ursra z30.h, z29.h, #1");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "ursra z30.h, z29.h, #16");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "ursra z30.s, z29.s, #1");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "ursra z30.s, z29.s, #32");
  TEST_SINGLE(ursra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "ursra z30.d, z29.d, #1");
  TEST_SINGLE(ursra(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "ursra z30.d, z29.d, #64");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift and insert") {
  TEST_SINGLE(sri(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sri z30.b, z29.b, #1");
  TEST_SINGLE(sri(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 4), "sri z30.b, z29.b, #4");
  TEST_SINGLE(sri(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sri z30.b, z29.b, #8");
  TEST_SINGLE(sri(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sri z30.h, z29.h, #1");
  TEST_SINGLE(sri(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 15), "sri z30.h, z29.h, #15");
  TEST_SINGLE(sri(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sri z30.h, z29.h, #16");
  TEST_SINGLE(sri(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sri z30.s, z29.s, #1");
  TEST_SINGLE(sri(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "sri z30.s, z29.s, #15");
  TEST_SINGLE(sri(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sri z30.s, z29.s, #32");
  TEST_SINGLE(sri(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 1), "sri z30.d, z29.d, #1");
  TEST_SINGLE(sri(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "sri z30.d, z29.d, #31");
  TEST_SINGLE(sri(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 64), "sri z30.d, z29.d, #64");

  TEST_SINGLE(sli(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 0), "sli z30.b, z29.b, #0");
  TEST_SINGLE(sli(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 4), "sli z30.b, z29.b, #4");
  TEST_SINGLE(sli(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 7), "sli z30.b, z29.b, #7");
  TEST_SINGLE(sli(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 0), "sli z30.h, z29.h, #0");
  TEST_SINGLE(sli(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 7), "sli z30.h, z29.h, #7");
  TEST_SINGLE(sli(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 15), "sli z30.h, z29.h, #15");
  TEST_SINGLE(sli(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 0), "sli z30.s, z29.s, #0");
  TEST_SINGLE(sli(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 15), "sli z30.s, z29.s, #15");
  TEST_SINGLE(sli(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 31), "sli z30.s, z29.s, #31");
  TEST_SINGLE(sli(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 0), "sli z30.d, z29.d, #0");
  TEST_SINGLE(sli(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 31), "sli z30.d, z29.d, #31");
  TEST_SINGLE(sli(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, 63), "sli z30.d, z29.d, #63");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer absolute difference and accumulate") {
  TEST_SINGLE(saba(SubRegSize::i8Bit, ZReg::z28, ZReg::z29, ZReg::z30), "saba z28.b, z29.b, z30.b");
  TEST_SINGLE(saba(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "saba z28.h, z29.h, z30.h");
  TEST_SINGLE(saba(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "saba z28.s, z29.s, z30.s");
  TEST_SINGLE(saba(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "saba z28.d, z29.d, z30.d");

  TEST_SINGLE(uaba(SubRegSize::i8Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uaba z28.b, z29.b, z30.b");
  TEST_SINGLE(uaba(SubRegSize::i16Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uaba z28.h, z29.h, z30.h");
  TEST_SINGLE(uaba(SubRegSize::i32Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uaba z28.s, z29.s, z30.s");
  TEST_SINGLE(uaba(SubRegSize::i64Bit, ZReg::z28, ZReg::z29, ZReg::z30), "uaba z28.d, z29.d, z30.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating extract narrow") {
  TEST_SINGLE(sqxtnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.b, z29.h");
  TEST_SINGLE(sqxtnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.h, z29.s");
  TEST_SINGLE(sqxtnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.s, z29.d");
  // TEST_SINGLE(sqxtnb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.d, z29.q");

  TEST_SINGLE(sqxtnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.b, z29.h");
  TEST_SINGLE(sqxtnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.h, z29.s");
  TEST_SINGLE(sqxtnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.s, z29.d");
  // TEST_SINGLE(sqxtnt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.d, z29.q");

  TEST_SINGLE(uqxtnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.b, z29.h");
  TEST_SINGLE(uqxtnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.h, z29.s");
  TEST_SINGLE(uqxtnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.s, z29.d");
  // TEST_SINGLE(uqxtnb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.d, z29.q");

  TEST_SINGLE(uqxtnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.b, z29.h");
  TEST_SINGLE(uqxtnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.h, z29.s");
  TEST_SINGLE(uqxtnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.s, z29.d");
  // TEST_SINGLE(uqxtnt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.d, z29.q");

  TEST_SINGLE(sqxtunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.b, z29.h");
  TEST_SINGLE(sqxtunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.h, z29.s");
  TEST_SINGLE(sqxtunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.s, z29.d");
  // TEST_SINGLE(sqxtunb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.d, z29.q");

  TEST_SINGLE(sqxtunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.b, z29.h");
  TEST_SINGLE(sqxtunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.h, z29.s");
  TEST_SINGLE(sqxtunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.s, z29.d");
  // TEST_SINGLE(sqxtunt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.d, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift right narrow") {
  TEST_SINGLE(sqshrunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqshrunb z30.b, z29.h, #1");
  TEST_SINGLE(sqshrunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqshrunb z30.b, z29.h, #8");
  TEST_SINGLE(sqshrunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqshrunb z30.h, z29.s, #1");
  TEST_SINGLE(sqshrunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqshrunb z30.h, z29.s, #16");
  TEST_SINGLE(sqshrunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqshrunb z30.s, z29.d, #1");
  TEST_SINGLE(sqshrunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqshrunb z30.s, z29.d, #32");

  TEST_SINGLE(sqshrunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqshrunt z30.b, z29.h, #1");
  TEST_SINGLE(sqshrunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqshrunt z30.b, z29.h, #8");
  TEST_SINGLE(sqshrunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqshrunt z30.h, z29.s, #1");
  TEST_SINGLE(sqshrunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqshrunt z30.h, z29.s, #16");
  TEST_SINGLE(sqshrunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqshrunt z30.s, z29.d, #1");
  TEST_SINGLE(sqshrunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqshrunt z30.s, z29.d, #32");

  TEST_SINGLE(sqrshrunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunb z30.b, z29.h, #1");
  TEST_SINGLE(sqrshrunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqrshrunb z30.b, z29.h, #8");
  TEST_SINGLE(sqrshrunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunb z30.h, z29.s, #1");
  TEST_SINGLE(sqrshrunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqrshrunb z30.h, z29.s, #16");
  TEST_SINGLE(sqrshrunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunb z30.s, z29.d, #1");
  TEST_SINGLE(sqrshrunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqrshrunb z30.s, z29.d, #32");

  TEST_SINGLE(sqrshrunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunt z30.b, z29.h, #1");
  TEST_SINGLE(sqrshrunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqrshrunt z30.b, z29.h, #8");
  TEST_SINGLE(sqrshrunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunt z30.h, z29.s, #1");
  TEST_SINGLE(sqrshrunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqrshrunt z30.h, z29.s, #16");
  TEST_SINGLE(sqrshrunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqrshrunt z30.s, z29.d, #1");
  TEST_SINGLE(sqrshrunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqrshrunt z30.s, z29.d, #32");

  TEST_SINGLE(shrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "shrnb z30.b, z29.h, #1");
  TEST_SINGLE(shrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "shrnb z30.b, z29.h, #8");
  TEST_SINGLE(shrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "shrnb z30.h, z29.s, #1");
  TEST_SINGLE(shrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "shrnb z30.h, z29.s, #16");
  TEST_SINGLE(shrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "shrnb z30.s, z29.d, #1");
  TEST_SINGLE(shrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "shrnb z30.s, z29.d, #32");

  TEST_SINGLE(shrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "shrnt z30.b, z29.h, #1");
  TEST_SINGLE(shrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "shrnt z30.b, z29.h, #8");
  TEST_SINGLE(shrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "shrnt z30.h, z29.s, #1");
  TEST_SINGLE(shrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "shrnt z30.h, z29.s, #16");
  TEST_SINGLE(shrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "shrnt z30.s, z29.d, #1");
  TEST_SINGLE(shrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "shrnt z30.s, z29.d, #32");

  TEST_SINGLE(rshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "rshrnb z30.b, z29.h, #1");
  TEST_SINGLE(rshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "rshrnb z30.b, z29.h, #8");
  TEST_SINGLE(rshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "rshrnb z30.h, z29.s, #1");
  TEST_SINGLE(rshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "rshrnb z30.h, z29.s, #16");
  TEST_SINGLE(rshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "rshrnb z30.s, z29.d, #1");
  TEST_SINGLE(rshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "rshrnb z30.s, z29.d, #32");

  TEST_SINGLE(rshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "rshrnt z30.b, z29.h, #1");
  TEST_SINGLE(rshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "rshrnt z30.b, z29.h, #8");
  TEST_SINGLE(rshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "rshrnt z30.h, z29.s, #1");
  TEST_SINGLE(rshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "rshrnt z30.h, z29.s, #16");
  TEST_SINGLE(rshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "rshrnt z30.s, z29.d, #1");
  TEST_SINGLE(rshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "rshrnt z30.s, z29.d, #32");

  TEST_SINGLE(sqshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqshrnb z30.b, z29.h, #1");
  TEST_SINGLE(sqshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqshrnb z30.b, z29.h, #8");
  TEST_SINGLE(sqshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqshrnb z30.h, z29.s, #1");
  TEST_SINGLE(sqshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqshrnb z30.h, z29.s, #16");
  TEST_SINGLE(sqshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqshrnb z30.s, z29.d, #1");
  TEST_SINGLE(sqshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqshrnb z30.s, z29.d, #32");

  TEST_SINGLE(sqshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqshrnt z30.b, z29.h, #1");
  TEST_SINGLE(sqshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqshrnt z30.b, z29.h, #8");
  TEST_SINGLE(sqshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqshrnt z30.h, z29.s, #1");
  TEST_SINGLE(sqshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqshrnt z30.h, z29.s, #16");
  TEST_SINGLE(sqshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqshrnt z30.s, z29.d, #1");
  TEST_SINGLE(sqshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqshrnt z30.s, z29.d, #32");

  TEST_SINGLE(sqrshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnb z30.b, z29.h, #1");
  TEST_SINGLE(sqrshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqrshrnb z30.b, z29.h, #8");
  TEST_SINGLE(sqrshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnb z30.h, z29.s, #1");
  TEST_SINGLE(sqrshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqrshrnb z30.h, z29.s, #16");
  TEST_SINGLE(sqrshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnb z30.s, z29.d, #1");
  TEST_SINGLE(sqrshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqrshrnb z30.s, z29.d, #32");

  TEST_SINGLE(sqrshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnt z30.b, z29.h, #1");
  TEST_SINGLE(sqrshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "sqrshrnt z30.b, z29.h, #8");
  TEST_SINGLE(sqrshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnt z30.h, z29.s, #1");
  TEST_SINGLE(sqrshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "sqrshrnt z30.h, z29.s, #16");
  TEST_SINGLE(sqrshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "sqrshrnt z30.s, z29.d, #1");
  TEST_SINGLE(sqrshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "sqrshrnt z30.s, z29.d, #32");

  TEST_SINGLE(uqshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "uqshrnb z30.b, z29.h, #1");
  TEST_SINGLE(uqshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "uqshrnb z30.b, z29.h, #8");
  TEST_SINGLE(uqshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "uqshrnb z30.h, z29.s, #1");
  TEST_SINGLE(uqshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "uqshrnb z30.h, z29.s, #16");
  TEST_SINGLE(uqshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "uqshrnb z30.s, z29.d, #1");
  TEST_SINGLE(uqshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "uqshrnb z30.s, z29.d, #32");

  TEST_SINGLE(uqshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "uqshrnt z30.b, z29.h, #1");
  TEST_SINGLE(uqshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "uqshrnt z30.b, z29.h, #8");
  TEST_SINGLE(uqshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "uqshrnt z30.h, z29.s, #1");
  TEST_SINGLE(uqshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "uqshrnt z30.h, z29.s, #16");
  TEST_SINGLE(uqshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "uqshrnt z30.s, z29.d, #1");
  TEST_SINGLE(uqshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "uqshrnt z30.s, z29.d, #32");

  TEST_SINGLE(uqrshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnb z30.b, z29.h, #1");
  TEST_SINGLE(uqrshrnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "uqrshrnb z30.b, z29.h, #8");
  TEST_SINGLE(uqrshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnb z30.h, z29.s, #1");
  TEST_SINGLE(uqrshrnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "uqrshrnb z30.h, z29.s, #16");
  TEST_SINGLE(uqrshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnb z30.s, z29.d, #1");
  TEST_SINGLE(uqrshrnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "uqrshrnb z30.s, z29.d, #32");

  TEST_SINGLE(uqrshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnt z30.b, z29.h, #1");
  TEST_SINGLE(uqrshrnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, 8), "uqrshrnt z30.b, z29.h, #8");
  TEST_SINGLE(uqrshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnt z30.h, z29.s, #1");
  TEST_SINGLE(uqrshrnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, 16), "uqrshrnt z30.h, z29.s, #16");
  TEST_SINGLE(uqrshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 1), "uqrshrnt z30.s, z29.d, #1");
  TEST_SINGLE(uqrshrnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, 32), "uqrshrnt z30.s, z29.d, #32");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract narrow high part") {
  TEST_SINGLE(addhnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnb z30.b, z29.h, z28.h");
  TEST_SINGLE(addhnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnb z30.h, z29.s, z28.s");
  TEST_SINGLE(addhnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnb z30.s, z29.d, z28.d");

  TEST_SINGLE(addhnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnt z30.b, z29.h, z28.h");
  TEST_SINGLE(addhnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnt z30.h, z29.s, z28.s");
  TEST_SINGLE(addhnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "addhnt z30.s, z29.d, z28.d");

  TEST_SINGLE(raddhnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnb z30.b, z29.h, z28.h");
  TEST_SINGLE(raddhnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnb z30.h, z29.s, z28.s");
  TEST_SINGLE(raddhnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnb z30.s, z29.d, z28.d");

  TEST_SINGLE(raddhnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnt z30.b, z29.h, z28.h");
  TEST_SINGLE(raddhnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnt z30.h, z29.s, z28.s");
  TEST_SINGLE(raddhnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "raddhnt z30.s, z29.d, z28.d");

  TEST_SINGLE(subhnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnb z30.b, z29.h, z28.h");
  TEST_SINGLE(subhnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnb z30.h, z29.s, z28.s");
  TEST_SINGLE(subhnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnb z30.s, z29.d, z28.d");

  TEST_SINGLE(subhnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnt z30.b, z29.h, z28.h");
  TEST_SINGLE(subhnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnt z30.h, z29.s, z28.s");
  TEST_SINGLE(subhnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "subhnt z30.s, z29.d, z28.d");

  TEST_SINGLE(rsubhnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnb z30.b, z29.h, z28.h");
  TEST_SINGLE(rsubhnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnb z30.h, z29.s, z28.s");
  TEST_SINGLE(rsubhnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnb z30.s, z29.d, z28.d");

  TEST_SINGLE(rsubhnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnt z30.b, z29.h, z28.h");
  TEST_SINGLE(rsubhnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnt z30.h, z29.s, z28.s");
  TEST_SINGLE(rsubhnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "rsubhnt z30.s, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 Histogram Computation") {
  TEST_SINGLE(histcnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29, ZReg::z28), "histcnt z30.s, p6/z, z29.s, z28.s");
  TEST_SINGLE(histcnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29, ZReg::z28), "histcnt z30.d, p6/z, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 Histogram Computation - Segment") {
  TEST_SINGLE(histseg(ZReg::z30, ZReg::z29, ZReg::z28), "histseg z30.b, z29.b, z28.b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto unary operations") {
  TEST_SINGLE(aesimc(ZReg::z7, ZReg::z7), "aesimc z7.b, z7.b");
  TEST_SINGLE(aesimc(ZReg::z31, ZReg::z31), "aesimc z31.b, z31.b");

  TEST_SINGLE(aesmc(ZReg::z7, ZReg::z7), "aesmc z7.b, z7.b");
  TEST_SINGLE(aesmc(ZReg::z31, ZReg::z31), "aesmc z31.b, z31.b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto destructive binary operations") {
  TEST_SINGLE(aesd(ZReg::z7, ZReg::z7, ZReg::z8), "aesd z7.b, z7.b, z8.b");
  TEST_SINGLE(aesd(ZReg::z30, ZReg::z30, ZReg::z31), "aesd z30.b, z30.b, z31.b");

  TEST_SINGLE(aese(ZReg::z7, ZReg::z7, ZReg::z8), "aese z7.b, z7.b, z8.b");
  TEST_SINGLE(aese(ZReg::z30, ZReg::z30, ZReg::z31), "aese z30.b, z30.b, z31.b");

  TEST_SINGLE(sm4e(ZReg::z7, ZReg::z7, ZReg::z8), "sm4e z7.s, z7.s, z8.s");
  TEST_SINGLE(sm4e(ZReg::z30, ZReg::z30, ZReg::z31), "sm4e z30.s, z30.s, z31.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto constructive binary operations") {
  TEST_SINGLE(sm4ekey(ZReg::z0, ZReg::z1, ZReg::z2), "sm4ekey z0.s, z1.s, z2.s");
  TEST_SINGLE(sm4ekey(ZReg::z29, ZReg::z30, ZReg::z31), "sm4ekey z29.s, z30.s, z31.s");

  TEST_SINGLE(rax1(ZReg::z0, ZReg::z1, ZReg::z2), "rax1 z0.d, z1.d, z2.d");
  TEST_SINGLE(rax1(ZReg::z29, ZReg::z30, ZReg::z31), "rax1 z29.d, z30.d, z31.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE BFloat16 floating-point dot product (indexed)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add long (indexed)") {
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "fmlalb z30.s, z29.h, z7.h[0]");
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmlalb z30.s, z29.h, z7.h[3]");
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmlalb z30.s, z29.h, z7.h[7]");

  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "fmlalt z30.s, z29.h, z7.h[0]");
  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmlalt z30.s, z29.h, z7.h[3]");
  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmlalt z30.s, z29.h, z7.h[7]");

  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "fmlslb z30.s, z29.h, z7.h[0]");
  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmlslb z30.s, z29.h, z7.h[3]");
  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmlslb z30.s, z29.h, z7.h[7]");

  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "fmlslt z30.s, z29.h, z7.h[0]");
  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "fmlslt z30.s, z29.h, z7.h[3]");
  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "fmlslt z30.s, z29.h, z7.h[7]");

  // XXX: vixl's diassembler doesn't support these. Re-enable when it does
  //      or upon switching disassemblers.

  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "bfmlalb z30.s, z29.h, z7.h[0]");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "bfmlalb z30.s, z29.h, z7.h[3]");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "bfmlalb z30.s, z29.h, z7.h[7]");

  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "bfmlalt z30.s, z29.h, z7.h[0]");
  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "bfmlalt z30.s, z29.h, z7.h[3]");
  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "bfmlalt z30.s, z29.h, z7.h[7]");

  // TEST_SINGLE(bfmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "bfmlslb z30.s, z29.h, z7.h[0]");
  // TEST_SINGLE(bfmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "bfmlslb z30.s, z29.h, z7.h[3]");
  // TEST_SINGLE(bfmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "bfmlslb z30.s, z29.h, z7.h[7]");

  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 0), "bfmlslt z30.s, z29.h, z7.h[0]");
  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 3), "bfmlslt z30.s, z29.h, z7.h[3]");
  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z7, 7), "bfmlslt z30.s, z29.h, z7.h[7]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE BFloat16 floating-point dot product") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add long") {
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalb z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalb z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalb z30.s, z29.h, z28.h");

  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalt z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalt z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlalt z30.s, z29.h, z28.h");

  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslb z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslb z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlslb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslb z30.s, z29.h, z28.h");

  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslt z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslt z30.s, z29.h, z28.h");
  TEST_SINGLE(fmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmlslt z30.s, z29.h, z28.h");

  // XXX: vixl's diassembler doesn't support these. Re-enable when it does
  //      or upon switching disassemblers.

  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalb z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalb z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalb z30.s, z29.h, z28.h");

  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalt z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalt z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlalt z30.s, z29.h, z28.h");

  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslb z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslb z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlalb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslb z30.s, z29.h, z28.h");

  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslt z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslt z30.s, z29.h, z28.h");
  // TEST_SINGLE(bfmlslt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "bfmlslt z30.s, z29.h, z28.h");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic (predicated)") {
  TEST_SINGLE(ftmad(SubRegSize::i16Bit, ZReg::z30, ZReg::z30, ZReg::z28, 7), "ftmad z30.h, z30.h, z28.h, #7");
  TEST_SINGLE(ftmad(SubRegSize::i32Bit, ZReg::z30, ZReg::z30, ZReg::z28, 7), "ftmad z30.s, z30.s, z28.s, #7");
  TEST_SINGLE(ftmad(SubRegSize::i64Bit, ZReg::z30, ZReg::z30, ZReg::z28, 7), "ftmad z30.d, z30.d, z28.d, #7");

  // TEST_SINGLE(fadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fadd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fadd z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsub z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsub z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmul(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmul z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmul z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmul z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmul z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmul(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmul z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsubr z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsubr z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmaxnm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmaxnm z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnm z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnm z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnm z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmaxnm(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnm z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fminnm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fminnm z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fminnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnm z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnm z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fminnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnm z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fminnm(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnm z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmax z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmax(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmin z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmin(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fabd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fabd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fabd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fabd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fabd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fabd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fabd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fabd z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fabd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fabd z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fscale(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fscale z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fscale(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fscale z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fscale(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fscale z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fscale(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fscale z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fscale(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fscale z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fmulx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmulx z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmulx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmulx z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmulx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmulx z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmulx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmulx z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fmulx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmulx z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fdiv(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fdiv z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fdiv(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdiv z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fdiv(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdiv z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fdiv(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdiv z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fdiv(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdiv z30.q, p6/m, z30.q, z28.q");

  // TEST_SINGLE(fdivr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fdivr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fdivr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdivr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fdivr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdivr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fdivr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdivr z30.d, p6/m, z30.d, z28.d");
  // TEST_SINGLE(fdivr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdivr z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic with immediate (predicated)") {
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fadd z30.h, p6/m, z30.h, #0.5");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fadd z30.s, p6/m, z30.s, #0.5");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fadd z30.d, p6/m, z30.d, #0.5");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fadd z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fadd z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fadd z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsub z30.h, p6/m, z30.h, #0.5");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsub z30.s, p6/m, z30.s, #0.5");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsub z30.d, p6/m, z30.d, #0.5");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsub z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsub z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsub z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsubr z30.h, p6/m, z30.h, #0.5");
  TEST_SINGLE(fsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsubr z30.s, p6/m, z30.s, #0.5");
  TEST_SINGLE(fsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_0_5), "fsubr z30.d, p6/m, z30.d, #0.5");
  TEST_SINGLE(fsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsubr z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsubr z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFAddSubImm::_1_0), "fsubr z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_0_5), "fmul z30.h, p6/m, z30.h, #0.5");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_0_5), "fmul z30.s, p6/m, z30.s, #0.5");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_0_5), "fmul z30.d, p6/m, z30.d, #0.5");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_2_0), "fmul z30.h, p6/m, z30.h, #2.0");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_2_0), "fmul z30.s, p6/m, z30.s, #2.0");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMulImm::_2_0), "fmul z30.d, p6/m, z30.d, #2.0");

  TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmaxnm z30.h, p6/m, z30.h, #0.0");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmaxnm z30.s, p6/m, z30.s, #0.0");
  TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmaxnm z30.d, p6/m, z30.d, #0.0");
  TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmaxnm z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmaxnm z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmaxnm z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fminnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fminnm z30.h, p6/m, z30.h, #0.0");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fminnm z30.s, p6/m, z30.s, #0.0");
  TEST_SINGLE(fminnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fminnm z30.d, p6/m, z30.d, #0.0");
  TEST_SINGLE(fminnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fminnm z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fminnm z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fminnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fminnm z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmax z30.h, p6/m, z30.h, #0.0");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmax z30.s, p6/m, z30.s, #0.0");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmax z30.d, p6/m, z30.d, #0.0");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmax z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmax z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmax z30.d, p6/m, z30.d, #1.0");

  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmin z30.h, p6/m, z30.h, #0.0");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmin z30.s, p6/m, z30.s, #0.0");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_0_0), "fmin z30.d, p6/m, z30.d, #0.0");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmin z30.h, p6/m, z30.h, #1.0");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmin z30.s, p6/m, z30.s, #1.0");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), SVEFMaxMinImm::_1_0), "fmin z30.d, p6/m, z30.d, #1.0");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Memory - 32-bit Gather and Unsized Contiguous") {
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1b {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1b {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1b {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1b {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ld1b {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1b {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ld1b {z30.s}, p6/z, [z31.s, #31]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1b {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ld1b {z30.d}, p6/z, [z31.d, #31]");

  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1sb {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1sb {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1sb {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1sb {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ld1sb {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1sb {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ld1sb {z30.s}, p6/z, [z31.s, #31]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1sb {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ld1sb {z30.d}, p6/z, [z31.d, #31]");

  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "ld1d {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d, uxtw]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "ld1d {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d, sxtw]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 3)), "ld1d {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d, uxtw #3]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 3)), "ld1d {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d, sxtw #3]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 3)), "ld1d {z30.d}, p6/z, [x30, "
                                                                                                                "z31.d, lsl #3]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "ld1d {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d]");

  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1d {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 248)), "ld1d {z30.d}, p6/z, [z31.d, #248]");

  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ld1h {z30.s}, p6/z, [x30, z31.s, uxtw #1]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ld1h {z30.s}, p6/z, [x30, z31.s, sxtw #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ld1h {z30.d}, p6/z, [x30, z31.d, uxtw #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ld1h {z30.d}, p6/z, [x30, z31.d, sxtw #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 1)),
              "ld1h {z30.d}, p6/z, [x30, z31.d, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ld1h {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1h {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1h {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1h {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1h {z30.d}, p6/z, [x30, z31.d, sxtw]");

  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1h {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ld1h {z30.s}, p6/z, [z31.s, #62]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1h {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ld1h {z30.d}, p6/z, [z31.d, #62]");

  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ld1sh {z30.s}, p6/z, [x30, z31.s, uxtw #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ld1sh {z30.s}, p6/z, [x30, z31.s, sxtw #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d, uxtw #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d, sxtw #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 1)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d, lsl #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1sh {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1sh {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1sh {z30.d}, p6/z, [x30, z31.d, sxtw]");

  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1sh {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ld1sh {z30.s}, p6/z, [z31.s, #62]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1sh {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ld1sh {z30.d}, p6/z, [z31.d, #62]");

  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)),
              "ld1w {z30.s}, p6/z, [x30, z31.s, uxtw #2]");
  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)),
              "ld1w {z30.s}, p6/z, [x30, z31.s, sxtw #2]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)),
              "ld1w {z30.d}, p6/z, [x30, z31.d, uxtw #2]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)),
              "ld1w {z30.d}, p6/z, [x30, z31.d, sxtw #2]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 2)),
              "ld1w {z30.d}, p6/z, [x30, z31.d, lsl #2]");

  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1w {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1w {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ld1w {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ld1w {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ld1w {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1w {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ld1w {z30.s}, p6/z, [z31.s, #124]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1w {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ld1w {z30.d}, p6/z, [z31.d, #124]");

  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "ld1sw {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d, uxtw]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "ld1sw {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d, sxtw]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)), "ld1sw {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d, uxtw #2]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)), "ld1sw {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d, sxtw #2]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 2)), "ld1sw {z30.d}, p6/z, "
                                                                                                                 "[x30, z31.d, lsl #2]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "ld1sw {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d]");

  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ld1sw {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ld1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ld1sw {z30.d}, p6/z, [z31.d, #124]");

  TEST_SINGLE(ldff1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1b {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ldff1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1b {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1b {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1b {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ldff1b {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1b {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ldff1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ldff1b {z30.s}, p6/z, [z31.s, "
                                                                                                       "#31]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1b {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ldff1b {z30.d}, p6/z, [z31.d, "
                                                                                                       "#31]");

  TEST_SINGLE(ldff1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1sb {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ldff1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1sb {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1sb {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1sb {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ldff1sb {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1sb {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ldff1sb<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ldff1sb {z30.s}, p6/z, [z31.s, "
                                                                                                        "#31]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1sb {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 31)), "ldff1sb {z30.d}, p6/z, [z31.d, "
                                                                                                        "#31]");

  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "ldff1d {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "ldff1d {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d, sxtw]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 3)), "ldff1d {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d, uxtw #3]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 3)), "ldff1d {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d, sxtw #3]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 3)), "ldff1d {z30.d}, p6/z, "
                                                                                                                  "[x30, z31.d, lsl #3]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "ldff1d {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d]");

  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1d {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1d(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 248)), "ldff1d {z30.d}, p6/z, [z31.d, #248]");

  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ldff1h {z30.s}, p6/z, [x30, z31.s, uxtw #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ldff1h {z30.s}, p6/z, [x30, z31.s, sxtw #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d, uxtw #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d, sxtw #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 1)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d, lsl #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1h {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1h {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1h {z30.d}, p6/z, [x30, z31.d, sxtw]");

  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1h {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ldff1h {z30.s}, p6/z, [z31.s, "
                                                                                                       "#62]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1h {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ldff1h {z30.d}, p6/z, [z31.d, "
                                                                                                       "#62]");

  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ldff1sh {z30.s}, p6/z, [x30, z31.s, uxtw #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ldff1sh {z30.s}, p6/z, [x30, z31.s, sxtw #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d, uxtw #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d, sxtw #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 1)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d, lsl #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1sh {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1sh {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1sh {z30.d}, p6/z, [x30, z31.d, sxtw]");

  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1sh {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ldff1sh {z30.s}, p6/z, [z31.s, "
                                                                                                        "#62]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1sh {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 62)), "ldff1sh {z30.d}, p6/z, [z31.d, "
                                                                                                        "#62]");

  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)),
              "ldff1w {z30.s}, p6/z, [x30, z31.s, uxtw #2]");
  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)),
              "ldff1w {z30.s}, p6/z, [x30, z31.s, sxtw #2]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d, uxtw #2]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d, sxtw #2]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 2)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d, lsl #2]");

  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1w {z30.s}, p6/z, [x30, z31.s, uxtw]");
  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1w {z30.s}, p6/z, [x30, z31.s, sxtw]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d, uxtw]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d, sxtw]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)),
              "ldff1w {z30.d}, p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1w {z30.s}, p6/z, [z31.s]");
  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ldff1w {z30.s}, p6/z, [z31.s, "
                                                                                                        "#124]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1w {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ldff1w {z30.d}, p6/z, [z31.d, "
                                                                                                        "#124]");

  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "ldff1sw {z30.d}, "
                                                                                                                    "p6/z, [x30, z31.d, "
                                                                                                                    "uxtw]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "ldff1sw {z30.d}, "
                                                                                                                    "p6/z, [x30, z31.d, "
                                                                                                                    "sxtw]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)), "ldff1sw {z30.d}, "
                                                                                                                    "p6/z, [x30, z31.d, "
                                                                                                                    "uxtw #2]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)), "ldff1sw {z30.d}, "
                                                                                                                    "p6/z, [x30, z31.d, "
                                                                                                                    "sxtw #2]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 2)), "ldff1sw {z30.d}, p6/z, "
                                                                                                                   "[x30, z31.d, lsl #2]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "ldff1sw {z30.d}, "
                                                                                                                    "p6/z, [x30, z31.d]");

  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 0)), "ldff1sw {z30.d}, p6/z, [z31.d]");
  TEST_SINGLE(ldff1sw(ZReg::z30, PReg::p6.Zeroing(), SVEMemOperand(ZReg::z31, 124)), "ldff1sw {z30.d}, p6/z, [z31.d, #124]");

  TEST_SINGLE(ldr(PReg::p6, XReg::x29, 0), "ldr p6, [x29]");
  TEST_SINGLE(ldr(PReg::p6, XReg::x29, -256), "ldr p6, [x29, #-256, mul vl]");
  TEST_SINGLE(ldr(PReg::p6, XReg::x29, 255), "ldr p6, [x29, #255, mul vl]");

  TEST_SINGLE(ldr(ZReg::z30, XReg::x29, 0), "ldr z30, [x29]");
  TEST_SINGLE(ldr(ZReg::z30, XReg::x29, -256), "ldr z30, [x29, #-256, mul vl]");
  TEST_SINGLE(ldr(ZReg::z30, XReg::x29, 255), "ldr z30, [x29, #255, mul vl]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load and broadcast element") {
  TEST_SINGLE(ld1rb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rb {z30.b}, p6/z, [x29]");
  TEST_SINGLE(ld1rb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rb {z30.b}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rb {z30.b}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rb {z30.h}, p6/z, [x29]");
  TEST_SINGLE(ld1rb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rb {z30.h}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rb {z30.h}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rb {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rb {z30.s}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rb {z30.s}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rb {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rb {z30.d}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rb {z30.d}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rsb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsb {z30.h}, p6/z, [x29]");
  TEST_SINGLE(ld1rsb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rsb {z30.h}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rsb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rsb {z30.h}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rsb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsb {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rsb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rsb {z30.s}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rsb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rsb {z30.s}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rsb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsb {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rsb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 31), "ld1rsb {z30.d}, p6/z, [x29, #31]");
  TEST_SINGLE(ld1rsb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 63), "ld1rsb {z30.d}, p6/z, [x29, #63]");

  TEST_SINGLE(ld1rh(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rh {z30.h}, p6/z, [x29]");
  TEST_SINGLE(ld1rh(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 64), "ld1rh {z30.h}, p6/z, [x29, #64]");
  TEST_SINGLE(ld1rh(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 126), "ld1rh {z30.h}, p6/z, [x29, #126]");

  TEST_SINGLE(ld1rh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rh {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 64), "ld1rh {z30.s}, p6/z, [x29, #64]");
  TEST_SINGLE(ld1rh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 126), "ld1rh {z30.s}, p6/z, [x29, #126]");

  TEST_SINGLE(ld1rh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rh {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 64), "ld1rh {z30.d}, p6/z, [x29, #64]");
  TEST_SINGLE(ld1rh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 126), "ld1rh {z30.d}, p6/z, [x29, #126]");

  TEST_SINGLE(ld1rsh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsh {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rsh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 64), "ld1rsh {z30.s}, p6/z, [x29, #64]");
  TEST_SINGLE(ld1rsh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 126), "ld1rsh {z30.s}, p6/z, [x29, #126]");

  TEST_SINGLE(ld1rsh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsh {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rsh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 64), "ld1rsh {z30.d}, p6/z, [x29, #64]");
  TEST_SINGLE(ld1rsh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 126), "ld1rsh {z30.d}, p6/z, [x29, #126]");

  TEST_SINGLE(ld1rw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rw {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 128), "ld1rw {z30.s}, p6/z, [x29, #128]");
  TEST_SINGLE(ld1rw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 252), "ld1rw {z30.s}, p6/z, [x29, #252]");

  TEST_SINGLE(ld1rw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rw {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 128), "ld1rw {z30.d}, p6/z, [x29, #128]");
  TEST_SINGLE(ld1rw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 252), "ld1rw {z30.d}, p6/z, [x29, #252]");

  TEST_SINGLE(ld1rsw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rsw {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rsw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 128), "ld1rsw {z30.d}, p6/z, [x29, #128]");
  TEST_SINGLE(ld1rsw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 252), "ld1rsw {z30.d}, p6/z, [x29, #252]");

  TEST_SINGLE(ld1rd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rd {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 256), "ld1rd {z30.d}, p6/z, [x29, #256]");
  TEST_SINGLE(ld1rd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 504), "ld1rd {z30.d}, p6/z, [x29, #504]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous non-temporal load (scalar plus immediate)") {
  TEST_SINGLE(ldnt1b(ZReg::z31, PReg::p6, Reg::r29, 0), "ldnt1b {z31.b}, p6/z, [x29]");
  TEST_SINGLE(ldnt1b(ZReg::z31, PReg::p6, Reg::r29, -8), "ldnt1b {z31.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ldnt1b(ZReg::z31, PReg::p6, Reg::r29, 7), "ldnt1b {z31.b}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ldnt1h(ZReg::z31, PReg::p6, Reg::r29, 0), "ldnt1h {z31.h}, p6/z, [x29]");
  TEST_SINGLE(ldnt1h(ZReg::z31, PReg::p6, Reg::r29, -8), "ldnt1h {z31.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ldnt1h(ZReg::z31, PReg::p6, Reg::r29, 7), "ldnt1h {z31.h}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ldnt1w(ZReg::z31, PReg::p6, Reg::r29, 0), "ldnt1w {z31.s}, p6/z, [x29]");
  TEST_SINGLE(ldnt1w(ZReg::z31, PReg::p6, Reg::r29, -8), "ldnt1w {z31.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ldnt1w(ZReg::z31, PReg::p6, Reg::r29, 7), "ldnt1w {z31.s}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ldnt1d(ZReg::z31, PReg::p6, Reg::r29, 0), "ldnt1d {z31.d}, p6/z, [x29]");
  TEST_SINGLE(ldnt1d(ZReg::z31, PReg::p6, Reg::r29, -8), "ldnt1d {z31.d}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ldnt1d(ZReg::z31, PReg::p6, Reg::r29, 7), "ldnt1d {z31.d}, p6/z, [x29, #7, mul vl]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load multiple structures (scalar plus scalar)") {
  TEST_SINGLE(ld2b(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2b {z31.b, z0.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2b {z26.b, z27.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld3b(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3b {z31.b, z0.b, z1.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29, "
                                                                                             "x30]");
  TEST_SINGLE(ld4b(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4b {z31.b, z0.b, z1.b, z2.b}, "
                                                                                                     "p6/z, [x29, x30]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4b {z26.b, z27.b, z28.b, "
                                                                                                        "z29.b}, p6/z, [x29, x30]");

  TEST_SINGLE(ld2h(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2h {z31.h, z0.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2h {z26.h, z27.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld3h(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3h {z31.h, z0.h, z1.h}, p6/z, [x29, x30, lsl "
                                                                                           "#1]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29, x30, "
                                                                                             "lsl #1]");
  TEST_SINGLE(ld4h(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4h {z31.h, z0.h, z1.h, z2.h}, "
                                                                                                     "p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4h {z26.h, z27.h, z28.h, "
                                                                                                        "z29.h}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ld2w(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2w {z31.s, z0.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2w {z26.s, z27.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld3w(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3w {z31.s, z0.s, z1.s}, p6/z, [x29, x30, lsl "
                                                                                           "#2]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29, x30, "
                                                                                             "lsl #2]");
  TEST_SINGLE(ld4w(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4w {z31.s, z0.s, z1.s, z2.s}, "
                                                                                                     "p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4w {z26.s, z27.s, z28.s, "
                                                                                                        "z29.s}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ld2d(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2d {z31.d, z0.d}, p6/z, [x29, x30, lsl #3]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld2d {z26.d, z27.d}, p6/z, [x29, x30, lsl #3]");
  TEST_SINGLE(ld3d(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3d {z31.d, z0.d, z1.d}, p6/z, [x29, x30, lsl "
                                                                                           "#3]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29, x30, "
                                                                                             "lsl #3]");
  TEST_SINGLE(ld4d(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4d {z31.d, z0.d, z1.d, z2.d}, "
                                                                                                     "p6/z, [x29, x30, lsl #3]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld4d {z26.d, z27.d, z28.d, "
                                                                                                        "z29.d}, p6/z, [x29, x30, lsl #3]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load and broadcast quadword (scalar plus immediate)") {
  TEST_SINGLE(ld1rqb(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rqb {z30.b}, p6/z, [x29]");
  TEST_SINGLE(ld1rqb(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -128), "ld1rqb {z30.b}, p6/z, [x29, #-128]");
  TEST_SINGLE(ld1rqb(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 112), "ld1rqb {z30.b}, p6/z, [x29, #112]");

  TEST_SINGLE(ld1rob(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rob {z30.b}, p6/z, [x29]");
  TEST_SINGLE(ld1rob(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -256), "ld1rob {z30.b}, p6/z, [x29, #-256]");
  TEST_SINGLE(ld1rob(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 224), "ld1rob {z30.b}, p6/z, [x29, #224]");

  TEST_SINGLE(ld1rqh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rqh {z30.h}, p6/z, [x29]");
  TEST_SINGLE(ld1rqh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -128), "ld1rqh {z30.h}, p6/z, [x29, #-128]");
  TEST_SINGLE(ld1rqh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 112), "ld1rqh {z30.h}, p6/z, [x29, #112]");

  TEST_SINGLE(ld1roh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1roh {z30.h}, p6/z, [x29]");
  TEST_SINGLE(ld1roh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -256), "ld1roh {z30.h}, p6/z, [x29, #-256]");
  TEST_SINGLE(ld1roh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 224), "ld1roh {z30.h}, p6/z, [x29, #224]");

  TEST_SINGLE(ld1rqw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rqw {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1rqw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -128), "ld1rqw {z30.s}, p6/z, [x29, #-128]");
  TEST_SINGLE(ld1rqw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 112), "ld1rqw {z30.s}, p6/z, [x29, #112]");

  TEST_SINGLE(ld1row(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1row {z30.s}, p6/z, [x29]");
  TEST_SINGLE(ld1row(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -256), "ld1row {z30.s}, p6/z, [x29, #-256]");
  TEST_SINGLE(ld1row(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 224), "ld1row {z30.s}, p6/z, [x29, #224]");

  TEST_SINGLE(ld1rqd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rqd {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rqd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -128), "ld1rqd {z30.d}, p6/z, [x29, #-128]");
  TEST_SINGLE(ld1rqd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 112), "ld1rqd {z30.d}, p6/z, [x29, #112]");

  TEST_SINGLE(ld1rod(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 0), "ld1rod {z30.d}, p6/z, [x29]");
  TEST_SINGLE(ld1rod(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, -256), "ld1rod {z30.d}, p6/z, [x29, #-256]");
  TEST_SINGLE(ld1rod(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, 224), "ld1rod {z30.d}, p6/z, [x29, #224]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load and broadcast quadword (scalar plus scalar)") {
  TEST_SINGLE(ld1rqb(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqb {z30.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1rqb(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqb {z30.b}, p6/z, [x29, x30]");

  TEST_SINGLE(ld1rob(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rob {z30.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1rob(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rob {z30.b}, p6/z, [x29, x30]");

  TEST_SINGLE(ld1rqh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqh {z30.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1rqh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqh {z30.h}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ld1roh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1roh {z30.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1roh(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1roh {z30.h}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ld1rqw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqw {z30.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld1rqw(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqw {z30.s}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ld1row(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1row {z30.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld1row(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1row {z30.s}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ld1rqd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqd {z30.d}, p6/z, [x29, x30, lsl #3]");
  TEST_SINGLE(ld1rqd(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rqd {z30.d}, p6/z, [x29, x30, lsl #3]");

  TEST_SINGLE(ld1rod(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rod {z30.d}, p6/z, [x29, x30, lsl #3]");
  TEST_SINGLE(ld1rod(ZReg::z30, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1rod {z30.d}, p6/z, [x29, x30, lsl #3]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load multiple structures (scalar plus immediate)") {
  TEST_SINGLE(ld2b(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, 0), "ld2b {z31.b, z0.b}, p6/z, [x29]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2b {z26.b, z27.b}, p6/z, [x29]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2b {z26.b, z27.b}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2b {z26.b, z27.b}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2h(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, 0), "ld2h {z31.h, z0.h}, p6/z, [x29]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2h {z26.h, z27.h}, p6/z, [x29]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2h {z26.h, z27.h}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2h {z26.h, z27.h}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2w(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, 0), "ld2w {z31.s, z0.s}, p6/z, [x29]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2w {z26.s, z27.s}, p6/z, [x29]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2w {z26.s, z27.s}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2w {z26.s, z27.s}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2d(ZReg::z31, ZReg::z0, PReg::p6.Zeroing(), Reg::r29, 0), "ld2d {z31.d, z0.d}, p6/z, [x29]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2d {z26.d, z27.d}, p6/z, [x29]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2d {z26.d, z27.d}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2d {z26.d, z27.d}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld3b(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, 0), "ld3b {z31.b, z0.b, z1.b}, p6/z, [x29]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29, #-24, mul "
                                                                                        "vl]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29, #21, mul "
                                                                                       "vl]");

  TEST_SINGLE(ld3h(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, 0), "ld3h {z31.h, z0.h, z1.h}, p6/z, [x29]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29, #-24, mul "
                                                                                        "vl]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29, #21, mul "
                                                                                       "vl]");

  TEST_SINGLE(ld3w(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, 0), "ld3w {z31.s, z0.s, z1.s}, p6/z, [x29]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29, #-24, mul "
                                                                                        "vl]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29, #21, mul "
                                                                                       "vl]");

  TEST_SINGLE(ld3d(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6.Zeroing(), Reg::r29, 0), "ld3d {z31.d, z0.d, z1.d}, p6/z, [x29]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29, #-24, mul "
                                                                                        "vl]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29, #21, mul "
                                                                                       "vl]");

  TEST_SINGLE(ld4b(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, 0), "ld4b {z31.b, z0.b, z1.b, z2.b}, p6/z, "
                                                                                              "[x29]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4b {z26.b, z27.b, z28.b, z29.b}, p6/z, "
                                                                                                 "[x29]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4b {z26.b, z27.b, z28.b, z29.b}, "
                                                                                                   "p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4b {z26.b, z27.b, z28.b, z29.b}, "
                                                                                                  "p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4h(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, 0), "ld4h {z31.h, z0.h, z1.h, z2.h}, p6/z, "
                                                                                              "[x29]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4h {z26.h, z27.h, z28.h, z29.h}, p6/z, "
                                                                                                 "[x29]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4h {z26.h, z27.h, z28.h, z29.h}, "
                                                                                                   "p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4h {z26.h, z27.h, z28.h, z29.h}, "
                                                                                                  "p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4w(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, 0), "ld4w {z31.s, z0.s, z1.s, z2.s}, p6/z, "
                                                                                              "[x29]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4w {z26.s, z27.s, z28.s, z29.s}, p6/z, "
                                                                                                 "[x29]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4w {z26.s, z27.s, z28.s, z29.s}, "
                                                                                                   "p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4w {z26.s, z27.s, z28.s, z29.s}, "
                                                                                                  "p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4d(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6.Zeroing(), Reg::r29, 0), "ld4d {z31.d, z0.d, z1.d, z2.d}, p6/z, "
                                                                                              "[x29]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4d {z26.d, z27.d, z28.d, z29.d}, p6/z, "
                                                                                                 "[x29]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4d {z26.d, z27.d, z28.d, z29.d}, "
                                                                                                   "p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4d {z26.d, z27.d, z28.d, z29.d}, "
                                                                                                  "p6/z, [x29, #28, mul vl]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous load (scalar plus immediate)") {
  TEST_SINGLE(ld1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1b {z26.b}, p6/z, [x29]");
  TEST_SINGLE(ld1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1b {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1b {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1b {z26.d}, p6/z, [x29]");

  TEST_SINGLE(ld1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1b {z26.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1b {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1b {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1b {z26.d}, p6/z, [x29, #-8, mul vl]");

  TEST_SINGLE(ld1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1b {z26.b}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1b {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1b {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1b {z26.d}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sw {z26.d}, p6/z, [x29]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sw {z26.d}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sw {z26.d}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1w {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1w {z26.d}, p6/z, [x29]");
  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1w {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1w {z26.d}, p6/z, [x29, #-8, mul vl]");

  // TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.b}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.d}, p6/z, [x29]");

  // TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.d}, p6/z, [x29, #-8, mul vl]");

  // TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.b}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.d}, p6/z, [x29, #7, mul vl]");

  // TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.b}, p6/z, [x29]");
  // TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.d}, p6/z, [x29]");

  // TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.b}, p6/z, [x29, #-8, mul vl]");
  // TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.d}, p6/z, [x29, #-8, mul vl]");

  // TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.b}, p6/z, [x29, #7, mul vl]");
  // TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.d}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sw {z26.d}, p6/z, [x29]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sw {z26.d}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sw {z26.d}, p6/z, [x29, #7, mul vl]");

  // TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.b}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.d}, p6/z, [x29]");

  // TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.d}, p6/z, [x29, #-8, mul vl]");

  // TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sb {z26.b}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sb {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sb {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sb {z26.d}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ld1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1d {z26.d}, p6/z, [x29]");
  TEST_SINGLE(ld1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1d {z26.d}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1d {z26.d}, p6/z, [x29, #7, mul vl]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous store (scalar plus scalar)") {
  TEST_SINGLE(st1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1b {z26.b}, p6, [x29, x28]");
  TEST_SINGLE(st1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1b {z26.h}, p6, [x29, x28]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1b {z26.s}, p6, [x29, x28]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1b {z26.d}, p6, [x29, x28]");

  // TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.b}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.h}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.s}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.d}, p6, [x29, x28, lsl #1]");

  // TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.b}, p6, [x29, x28, lsl #2]");
  // TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.h}, p6, [x29, x28, lsl #2]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.s}, p6, [x29, x28, lsl #2]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.d}, p6, [x29, x28, lsl #2]");

  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1d {z26.d}, p6, [x29, x28, lsl #3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous load (scalar plus scalar)") {
  TEST_SINGLE(ld1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.d}, p6/z, [x29, x30]");

  // TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.d}, p6/z, [x29, x30]");

  // TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.b}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.d}, p6/z, [x29, x30, lsl #1]");

  // TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.b}, p6/z, [x29, x30, lsl #1]");
  // TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.d}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ld1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1w {z26.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ld1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1w {z26.d}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sw {z26.d}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ld1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1d {z26.d}, p6/z, [x29, x30, lsl #3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous first-fault load (scalar plus scalar)") {
  TEST_SINGLE(ldff1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1b {z26.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ldff1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1b {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ldff1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1b {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ldff1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1b {z26.d}, p6/z, [x29, x30]");

  TEST_SINGLE(ldff1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sb {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ldff1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sb {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ldff1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sb {z26.d}, p6/z, [x29, x30]");

  TEST_SINGLE(ldff1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1h {z26.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1h {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ldff1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1h {z26.d}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ldff1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sh {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ldff1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sh {z26.d}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ldff1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1w {z26.s}, p6/z, [x29, x30, lsl #2]");
  TEST_SINGLE(ldff1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1w {z26.d}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ldff1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1sw {z26.d}, p6/z, [x29, x30, lsl #2]");

  TEST_SINGLE(ldff1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ldff1d {z26.d}, p6/z, [x29, x30, lsl #3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point round to integral value") {
  TEST_SINGLE(frinti(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinti z30.h, p6/m, z29.h");
  TEST_SINGLE(frinti(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinti z30.s, p6/m, z29.s");
  TEST_SINGLE(frinti(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinti z30.d, p6/m, z29.d");
  TEST_SINGLE(frintx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintx z30.h, p6/m, z29.h");
  TEST_SINGLE(frintx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintx z30.s, p6/m, z29.s");
  TEST_SINGLE(frintx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintx z30.d, p6/m, z29.d");
  TEST_SINGLE(frinta(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinta z30.h, p6/m, z29.h");
  TEST_SINGLE(frinta(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinta z30.s, p6/m, z29.s");
  TEST_SINGLE(frinta(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frinta z30.d, p6/m, z29.d");
  TEST_SINGLE(frintn(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintn z30.h, p6/m, z29.h");
  TEST_SINGLE(frintn(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintn z30.s, p6/m, z29.s");
  TEST_SINGLE(frintn(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintn z30.d, p6/m, z29.d");
  TEST_SINGLE(frintz(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintz z30.h, p6/m, z29.h");
  TEST_SINGLE(frintz(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintz z30.s, p6/m, z29.s");
  TEST_SINGLE(frintz(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintz z30.d, p6/m, z29.d");
  TEST_SINGLE(frintm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintm z30.h, p6/m, z29.h");
  TEST_SINGLE(frintm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintm z30.s, p6/m, z29.s");
  TEST_SINGLE(frintm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintm z30.d, p6/m, z29.d");
  TEST_SINGLE(frintp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintp z30.h, p6/m, z29.h");
  TEST_SINGLE(frintp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintp z30.s, p6/m, z29.s");
  TEST_SINGLE(frintp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frintp z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point convert precision") {
  TEST_SINGLE(fcvt(SubRegSize::i16Bit, SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.h, p6/m, z29.s");
  TEST_SINGLE(fcvt(SubRegSize::i16Bit, SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.h, p6/m, z29.d");

  TEST_SINGLE(fcvt(SubRegSize::i32Bit, SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvt(SubRegSize::i32Bit, SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.s, p6/m, z29.d");

  TEST_SINGLE(fcvt(SubRegSize::i64Bit, SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.d, p6/m, z29.h");
  TEST_SINGLE(fcvt(SubRegSize::i64Bit, SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvt z30.d, p6/m, z29.s");

  TEST_SINGLE(fcvtx(ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtx z30.s, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point unary operations") {
  TEST_SINGLE(frecpx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frecpx z30.h, p6/m, z29.h");
  TEST_SINGLE(frecpx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frecpx z30.s, p6/m, z29.s");
  TEST_SINGLE(frecpx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "frecpx z30.d, p6/m, z29.d");

  TEST_SINGLE(fsqrt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fsqrt z30.h, p6/m, z29.h");
  TEST_SINGLE(fsqrt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fsqrt z30.s, p6/m, z29.s");
  TEST_SINGLE(fsqrt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fsqrt z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer convert to floating-point") {
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.h, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.h, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.h, p6/m, z29.d");

  // TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.s, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.s, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.s, p6/m, z29.d");

  // TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.d, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.d, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.d, p6/m, z29.d");

  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.h, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.h, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.h, p6/m, z29.d");

  // TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.s, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.s, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.s, p6/m, z29.d");

  // TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.d, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.d, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point convert to integer") {
  TEST_SINGLE(flogb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.h, p6/m, z29.h");
  TEST_SINGLE(flogb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.s, p6/m, z29.s");
  TEST_SINGLE(flogb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.d, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.h, p6/m, z29.h");
  // TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.h, p6/m, z29.s");
  // TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.h, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.s, p6/m, z29.s");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.s, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.d, p6/m, z29.h");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.d, p6/m, z29.s");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.d, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.h, p6/m, z29.h");
  // TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.h, p6/m, z29.s");
  // TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.h, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.s, p6/m, z29.s");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.s, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.d, p6/m, z29.h");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.d, p6/m, z29.s");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point reciprocal estimate (unpredicated)") {
  TEST_SINGLE(frecpe(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "frecpe z30.h, z29.h");
  TEST_SINGLE(frecpe(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "frecpe z30.s, z29.s");
  TEST_SINGLE(frecpe(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "frecpe z30.d, z29.d");

  TEST_SINGLE(frsqrte(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "frsqrte z30.h, z29.h");
  TEST_SINGLE(frsqrte(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "frsqrte z30.s, z29.s");
  TEST_SINGLE(frsqrte(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "frsqrte z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point serial reduction (predicated)") {
  TEST_SINGLE(fadda(SubRegSize::i16Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "fadda h30, p7, h30, z29.h");
  TEST_SINGLE(fadda(SubRegSize::i32Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "fadda s30, p7, s30, z29.s");
  TEST_SINGLE(fadda(SubRegSize::i64Bit, VReg::v30, PReg::p7, VReg::v30, ZReg::z29), "fadda d30, p7, d30, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point compare with zero") {
  TEST_SINGLE(fcmge(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmge p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmge(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmge p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmge(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmge p15.d, p7/z, z30.d, #0.0");

  TEST_SINGLE(fcmgt(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmgt p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmgt p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmgt(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmgt p15.d, p7/z, z30.d, #0.0");

  TEST_SINGLE(fcmlt(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmlt p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmlt(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmlt p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmlt(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmlt p15.d, p7/z, z30.d, #0.0");

  TEST_SINGLE(fcmle(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmle p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmle(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmle p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmle(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmle p15.d, p7/z, z30.d, #0.0");

  TEST_SINGLE(fcmeq(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmeq p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmeq p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmeq(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmeq p15.d, p7/z, z30.d, #0.0");

  TEST_SINGLE(fcmne(SubRegSize::i16Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmne p15.h, p7/z, z30.h, #0.0");
  TEST_SINGLE(fcmne(SubRegSize::i32Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmne p15.s, p7/z, z30.s, #0.0");
  TEST_SINGLE(fcmne(SubRegSize::i64Bit, PReg::p15, PReg::p7.Zeroing(), ZReg::z30), "fcmne p15.d, p7/z, z30.d, #0.0");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-accumulate writing addend") {
  TEST_SINGLE(fmla(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmla z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmla z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fmla(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmla z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fmls(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmls z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmls z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fmls(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmls z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fnmla(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmla z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fnmla(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmla z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fnmla(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmla z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fnmls(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmls z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fnmls(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmls z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fnmls(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmls z30.d, p7/m, z29.d, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-accumulate writing multiplicand") {
  TEST_SINGLE(fmad(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmad z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fmad(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmad z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fmad(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmad z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fmsb(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmsb z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fmsb(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmsb z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fmsb(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fmsb z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fnmad(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmad z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fnmad(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmad z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fnmad(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmad z30.d, p7/m, z29.d, z28.d");

  TEST_SINGLE(fnmsb(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmsb z30.h, p7/m, z29.h, z28.h");
  TEST_SINGLE(fnmsb(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmsb z30.s, p7/m, z29.s, z28.s");
  TEST_SINGLE(fnmsb(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z29, ZReg::z28), "fnmsb z30.d, p7/m, z29.d, z28.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE store multiple structures (scalar plus scalar)") {
  TEST_SINGLE(st2b(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, Reg::r30), "st2b {z31.b, z0.b}, p6, [x29, x30]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, Reg::r30), "st2b {z26.b, z27.b}, p6, [x29, x30]");
  TEST_SINGLE(st3b(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, Reg::r30), "st3b {z31.b, z0.b, z1.b}, p6, [x29, x30]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, Reg::r30), "st3b {z26.b, z27.b, z28.b}, p6, [x29, x30]");
  TEST_SINGLE(st4b(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, Reg::r30), "st4b {z31.b, z0.b, z1.b, z2.b}, p6, [x29, "
                                                                                           "x30]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, Reg::r30), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, "
                                                                                              "[x29, x30]");

  TEST_SINGLE(st2h(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, Reg::r30), "st2h {z31.h, z0.h}, p6, [x29, x30, lsl #1]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, Reg::r30), "st2h {z26.h, z27.h}, p6, [x29, x30, lsl #1]");
  TEST_SINGLE(st3h(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, Reg::r30), "st3h {z31.h, z0.h, z1.h}, p6, [x29, x30, lsl #1]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, Reg::r30), "st3h {z26.h, z27.h, z28.h}, p6, [x29, x30, lsl #1]");
  TEST_SINGLE(st4h(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, Reg::r30), "st4h {z31.h, z0.h, z1.h, z2.h}, p6, [x29, x30, "
                                                                                           "lsl #1]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, Reg::r30), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, "
                                                                                              "[x29, x30, lsl #1]");

  TEST_SINGLE(st2w(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, Reg::r30), "st2w {z31.s, z0.s}, p6, [x29, x30, lsl #2]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, Reg::r30), "st2w {z26.s, z27.s}, p6, [x29, x30, lsl #2]");
  TEST_SINGLE(st3w(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, Reg::r30), "st3w {z31.s, z0.s, z1.s}, p6, [x29, x30, lsl #2]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, Reg::r30), "st3w {z26.s, z27.s, z28.s}, p6, [x29, x30, lsl #2]");
  TEST_SINGLE(st4w(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, Reg::r30), "st4w {z31.s, z0.s, z1.s, z2.s}, p6, [x29, x30, "
                                                                                           "lsl #2]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, Reg::r30), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, "
                                                                                              "[x29, x30, lsl #2]");

  TEST_SINGLE(st2d(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, Reg::r30), "st2d {z31.d, z0.d}, p6, [x29, x30, lsl #3]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, Reg::r30), "st2d {z26.d, z27.d}, p6, [x29, x30, lsl #3]");
  TEST_SINGLE(st3d(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, Reg::r30), "st3d {z31.d, z0.d, z1.d}, p6, [x29, x30, lsl #3]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, Reg::r30), "st3d {z26.d, z27.d, z28.d}, p6, [x29, x30, lsl #3]");
  TEST_SINGLE(st4d(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, Reg::r30), "st4d {z31.d, z0.d, z1.d, z2.d}, p6, [x29, x30, "
                                                                                           "lsl #3]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, Reg::r30), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, "
                                                                                              "[x29, x30, lsl #3]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous non-temporal store (scalar plus immediate)") {
  TEST_SINGLE(stnt1b(ZReg::z31, PReg::p6, Reg::r29, 0), "stnt1b {z31.b}, p6, [x29]");
  TEST_SINGLE(stnt1b(ZReg::z31, PReg::p6, Reg::r29, -8), "stnt1b {z31.b}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(stnt1b(ZReg::z31, PReg::p6, Reg::r29, 7), "stnt1b {z31.b}, p6, [x29, #7, mul vl]");

  TEST_SINGLE(stnt1h(ZReg::z31, PReg::p6, Reg::r29, 0), "stnt1h {z31.h}, p6, [x29]");
  TEST_SINGLE(stnt1h(ZReg::z31, PReg::p6, Reg::r29, -8), "stnt1h {z31.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(stnt1h(ZReg::z31, PReg::p6, Reg::r29, 7), "stnt1h {z31.h}, p6, [x29, #7, mul vl]");

  TEST_SINGLE(stnt1w(ZReg::z31, PReg::p6, Reg::r29, 0), "stnt1w {z31.s}, p6, [x29]");
  TEST_SINGLE(stnt1w(ZReg::z31, PReg::p6, Reg::r29, -8), "stnt1w {z31.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(stnt1w(ZReg::z31, PReg::p6, Reg::r29, 7), "stnt1w {z31.s}, p6, [x29, #7, mul vl]");

  TEST_SINGLE(stnt1d(ZReg::z31, PReg::p6, Reg::r29, 0), "stnt1d {z31.d}, p6, [x29]");
  TEST_SINGLE(stnt1d(ZReg::z31, PReg::p6, Reg::r29, -8), "stnt1d {z31.d}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(stnt1d(ZReg::z31, PReg::p6, Reg::r29, 7), "stnt1d {z31.d}, p6, [x29, #7, mul vl]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE store multiple structures (scalar plus immediate)") {
  TEST_SINGLE(st2b(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, 0), "st2b {z31.b, z0.b}, p6, [x29]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2b {z26.b, z27.b}, p6, [x29]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2b {z26.b, z27.b}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2b {z26.b, z27.b}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2h(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, 0), "st2h {z31.h, z0.h}, p6, [x29]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2h {z26.h, z27.h}, p6, [x29]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2h {z26.h, z27.h}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2h {z26.h, z27.h}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2w(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, 0), "st2w {z31.s, z0.s}, p6, [x29]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2w {z26.s, z27.s}, p6, [x29]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2w {z26.s, z27.s}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2w {z26.s, z27.s}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2d(ZReg::z31, ZReg::z0, PReg::p6, Reg::r29, 0), "st2d {z31.d, z0.d}, p6, [x29]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2d {z26.d, z27.d}, p6, [x29]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2d {z26.d, z27.d}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2d {z26.d, z27.d}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st3b(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, 0), "st3b {z31.b, z0.b, z1.b}, p6, [x29]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3b {z26.b, z27.b, z28.b}, p6, [x29]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3b {z26.b, z27.b, z28.b}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3b {z26.b, z27.b, z28.b}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3h(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, 0), "st3h {z31.h, z0.h, z1.h}, p6, [x29]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3h {z26.h, z27.h, z28.h}, p6, [x29]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3h {z26.h, z27.h, z28.h}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3h {z26.h, z27.h, z28.h}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3w(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, 0), "st3w {z31.s, z0.s, z1.s}, p6, [x29]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3w {z26.s, z27.s, z28.s}, p6, [x29]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3w {z26.s, z27.s, z28.s}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3w {z26.s, z27.s, z28.s}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3d(ZReg::z31, ZReg::z0, ZReg::z1, PReg::p6, Reg::r29, 0), "st3d {z31.d, z0.d, z1.d}, p6, [x29]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3d {z26.d, z27.d, z28.d}, p6, [x29]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3d {z26.d, z27.d, z28.d}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3d {z26.d, z27.d, z28.d}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st4b(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, 0), "st4b {z31.b, z0.b, z1.b, z2.b}, p6, [x29]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29, "
                                                                                         "#-32, mul vl]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29, #28, "
                                                                                        "mul vl]");

  TEST_SINGLE(st4h(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, 0), "st4h {z31.h, z0.h, z1.h, z2.h}, p6, [x29]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29, "
                                                                                         "#-32, mul vl]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29, #28, "
                                                                                        "mul vl]");

  TEST_SINGLE(st4w(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, 0), "st4w {z31.s, z0.s, z1.s, z2.s}, p6, [x29]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29, "
                                                                                         "#-32, mul vl]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29, #28, "
                                                                                        "mul vl]");

  TEST_SINGLE(st4d(ZReg::z31, ZReg::z0, ZReg::z1, ZReg::z2, PReg::p6, Reg::r29, 0), "st4d {z31.d, z0.d, z1.d, z2.d}, p6, [x29]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29, "
                                                                                         "#-32, mul vl]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29, #28, "
                                                                                        "mul vl]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous store (scalar plus immediate)") {
  TEST_SINGLE(st1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1b {z26.b}, p6, [x29]");
  TEST_SINGLE(st1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1b {z26.h}, p6, [x29]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1b {z26.s}, p6, [x29]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1b {z26.d}, p6, [x29]");

  TEST_SINGLE(st1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1b {z26.b}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1b {z26.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1b {z26.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1b {z26.d}, p6, [x29, #-8, mul vl]");

  TEST_SINGLE(st1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1b {z26.b}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1b {z26.h}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1b {z26.s}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1b {z26.d}, p6, [x29, #7, mul vl]");

  // TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.b}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.h}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.s}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.d}, p6, [x29]");

  // TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.b}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.d}, p6, [x29, #-8, mul vl]");

  // TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.b}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.h}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.s}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.d}, p6, [x29, #7, mul vl]");

  // TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.b}, p6, [x29]");
  // TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.h}, p6, [x29]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.s}, p6, [x29]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.d}, p6, [x29]");

  // TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.b}, p6, [x29, #-8, mul vl]");
  // TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.d}, p6, [x29, #-8, mul vl]");

  // TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.b}, p6, [x29, #7, mul vl]");
  // TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.h}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.s}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.d}, p6, [x29, #7, mul vl]");

  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, 0), "st1d {z26.d}, p6, [x29]");
  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, -8), "st1d {z26.d}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, 7), "st1d {z26.d}, p6, [x29, #7, mul vl]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Scatters") {
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1b {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, uxtw]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1b {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, sxtw]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1b {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, uxtw]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1b {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, sxtw]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "st1b {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d]");

  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1b {z30.s}, p6, [z31.s]");
  TEST_SINGLE(st1b<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 31)), "st1b {z30.s}, p6, [z31.s, #31]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1b {z30.d}, p6, [z31.d]");
  TEST_SINGLE(st1b<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 31)), "st1b {z30.d}, p6, [z31.d, #31]");

  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)), "st1h {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, uxtw "
                                                                                                                           "#1]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)), "st1h {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, sxtw "
                                                                                                                           "#1]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 1)), "st1h {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, uxtw "
                                                                                                                           "#1]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 1)), "st1h {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, sxtw "
                                                                                                                           "#1]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 1)), "st1h {z30.d}, "
                                                                                                                          "p6, [x30, "
                                                                                                                          "z31.d, lsl #1]");

  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1h {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, uxtw]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1h {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, sxtw]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1h {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, uxtw]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1h {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, sxtw]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "st1h {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d]");

  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1h {z30.s}, p6, [z31.s]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 62)), "st1h {z30.s}, p6, [z31.s, #62]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1h {z30.d}, p6, [z31.d]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 62)), "st1h {z30.d}, p6, [z31.d, #62]");

  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)), "st1w {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, uxtw "
                                                                                                                           "#2]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)), "st1w {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, sxtw "
                                                                                                                           "#2]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 2)), "st1w {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, uxtw "
                                                                                                                           "#2]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 2)), "st1w {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, sxtw "
                                                                                                                           "#2]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 2)), "st1w {z30.d}, "
                                                                                                                          "p6, [x30, "
                                                                                                                          "z31.d, lsl #2]");

  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1w {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, uxtw]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1w {z30.s}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.s, sxtw]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1w {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, uxtw]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1w {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d, sxtw]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "st1w {z30.d}, "
                                                                                                                           "p6, [x30, "
                                                                                                                           "z31.d]");

  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1w {z30.s}, p6, [z31.s]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 124)), "st1w {z30.s}, p6, [z31.s, #124]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1w {z30.d}, p6, [z31.d]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 124)), "st1w {z30.d}, p6, [z31.d, #124]");

  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 3)), "st1d {z30.d}, p6, [x30, z31.d, "
                                                                                                       "uxtw #3]");
  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 3)), "st1d {z30.d}, p6, [x30, z31.d, "
                                                                                                       "sxtw #3]");
  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_LSL, 3)), "st1d {z30.d}, p6, [x30, z31.d, lsl "
                                                                                                      "#3]");

  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_UXTW, 0)), "st1d {z30.d}, p6, [x30, z31.d, "
                                                                                                       "uxtw]");
  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_SXTW, 0)), "st1d {z30.d}, p6, [x30, z31.d, "
                                                                                                       "sxtw]");
  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(XReg::x30, ZReg::z31, SVEModType::MOD_NONE, 0)), "st1d {z30.d}, p6, [x30, z31.d]");

  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 0)), "st1d {z30.d}, p6, [z31.d]");
  TEST_SINGLE(st1d(ZReg::z30, PReg::p6, SVEMemOperand(ZReg::z31, 248)), "st1d {z30.d}, p6, [z31.d, #248]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Unsized Stores") {
  TEST_SINGLE(str(PReg::p6, XReg::x29, 0), "str p6, [x29]");
  TEST_SINGLE(str(PReg::p6, XReg::x29, -256), "str p6, [x29, #-256, mul vl]");
  TEST_SINGLE(str(PReg::p6, XReg::x29, 255), "str p6, [x29, #255, mul vl]");

  TEST_SINGLE(str(ZReg::z30, XReg::x29, 0), "str z30, [x29]");
  TEST_SINGLE(str(ZReg::z30, XReg::x29, -256), "str z30, [x29, #-256, mul vl]");
  TEST_SINGLE(str(ZReg::z30, XReg::x29, 255), "str z30, [x29, #255, mul vl]");
}
