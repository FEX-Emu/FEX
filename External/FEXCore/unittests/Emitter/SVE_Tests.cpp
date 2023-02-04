#include "TestDisassembler.h"

#include <catch2/catch.hpp>
#include <fcntl.h>

using namespace FEXCore::ARMEmitter;

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

  // TODO: TBL

  TEST_SINGLE(sel(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28),   "sel z30.b, p6, z29.b, z28.b");
  TEST_SINGLE(sel(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28),  "sel z30.h, p6, z29.h, z28.h");
  TEST_SINGLE(sel(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28),  "sel z30.s, p6, z29.s, z28.s");
  TEST_SINGLE(sel(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28),  "sel z30.d, p6, z29.d, z28.d");
  //TEST_SINGLE(sel(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z29, ZReg::z28), "sel z30.q, p6, z29.q, z28.q");

  TEST_SINGLE(mov(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "mov z30.b, p6/m, z29.b");
  TEST_SINGLE(mov(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "mov z30.h, p6/m, z29.h");
  TEST_SINGLE(mov(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "mov z30.s, p6/m, z29.s");
  TEST_SINGLE(mov(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "mov z30.d, p6/m, z29.d");
  //TEST_SINGLE(mov(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "mov z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract vectors (unpredicated)") {
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "add z30.b, z29.b, z28.b");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "add z30.h, z29.h, z28.h");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "add z30.s, z29.s, z28.s");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "add z30.d, z29.d, z28.d");
  //TEST_SINGLE(add(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "add z30.q, z29.q, z28.q");

  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "sub z30.b, z29.b, z28.b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sub z30.h, z29.h, z28.h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sub z30.s, z29.s, z28.s");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sub z30.d, z29.d, z28.d");
  //TEST_SINGLE(sub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sub z30.q, z29.q, z28.q");

  TEST_SINGLE(sqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "sqadd z30.b, z29.b, z28.b");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqadd z30.h, z29.h, z28.h");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqadd z30.s, z29.s, z28.s");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqadd z30.d, z29.d, z28.d");
  //TEST_SINGLE(sqadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqadd z30.q, z29.q, z28.q");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "uqadd z30.b, z29.b, z28.b");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqadd z30.h, z29.h, z28.h");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqadd z30.s, z29.s, z28.s");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqadd z30.d, z29.d, z28.d");
  //TEST_SINGLE(uqadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqadd z30.q, z29.q, z28.q");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "sqsub z30.b, z29.b, z28.b");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqsub z30.h, z29.h, z28.h");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqsub z30.s, z29.s, z28.s");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "sqsub z30.d, z29.d, z28.d");
  //TEST_SINGLE(sqsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqsub z30.q, z29.q, z28.q");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "uqsub z30.b, z29.b, z28.b");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqsub z30.h, z29.h, z28.h");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqsub z30.s, z29.s, z28.s");
  TEST_SINGLE(uqsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uqsub z30.d, z29.d, z28.d");
  //TEST_SINGLE(uqsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uqsub z30.q, z29.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE address generation") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE table lookup (three sources)") {
  TEST_SINGLE(tbl(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "tbl z30.b, {z29.b}, z28.b");
  TEST_SINGLE(tbl(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbl z30.h, {z29.h}, z28.h");
  TEST_SINGLE(tbl(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbl z30.s, {z29.s}, z28.s");
  TEST_SINGLE(tbl(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbl z30.d, {z29.d}, z28.d");
  //TEST_SINGLE(tbl(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbl z30.q, {z29.q}, z28.q");

  TEST_SINGLE(tbx(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "tbx z30.b, z29.b, z28.b");
  TEST_SINGLE(tbx(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbx z30.h, z29.h, z28.h");
  TEST_SINGLE(tbx(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbx z30.s, z29.s, z28.s");
  TEST_SINGLE(tbx(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "tbx z30.d, z29.d, z28.d");
  //TEST_SINGLE(tbx(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "tbx z30.q, z29.q, z28.q");

}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE permute vector elements") {
  TEST_SINGLE(zip1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "zip1 z30.b, z29.b, z28.b");
  TEST_SINGLE(zip1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.h, z29.h, z28.h");
  TEST_SINGLE(zip1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.s, z29.s, z28.s");
  TEST_SINGLE(zip1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip1 z30.d, z29.d, z28.d");

  TEST_SINGLE(zip2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "zip2 z30.b, z29.b, z28.b");
  TEST_SINGLE(zip2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.h, z29.h, z28.h");
  TEST_SINGLE(zip2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.s, z29.s, z28.s");
  TEST_SINGLE(zip2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "zip2 z30.d, z29.d, z28.d");

  TEST_SINGLE(uzp1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uzp1 z30.b, z29.b, z28.b");
  TEST_SINGLE(uzp1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.h, z29.h, z28.h");
  TEST_SINGLE(uzp1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.s, z29.s, z28.s");
  TEST_SINGLE(uzp1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp1 z30.d, z29.d, z28.d");

  TEST_SINGLE(uzp2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "uzp2 z30.b, z29.b, z28.b");
  TEST_SINGLE(uzp2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.h, z29.h, z28.h");
  TEST_SINGLE(uzp2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.s, z29.s, z28.s");
  TEST_SINGLE(uzp2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uzp2 z30.d, z29.d, z28.d");

  TEST_SINGLE(trn1(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "trn1 z30.b, z29.b, z28.b");
  TEST_SINGLE(trn1(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.h, z29.h, z28.h");
  TEST_SINGLE(trn1(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.s, z29.s, z28.s");
  TEST_SINGLE(trn1(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn1 z30.d, z29.d, z28.d");

  TEST_SINGLE(trn2(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "trn2 z30.b, z29.b, z28.b");
  TEST_SINGLE(trn2(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.h, z29.h, z28.h");
  TEST_SINGLE(trn2(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.s, z29.s, z28.s");
  TEST_SINGLE(trn2(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "trn2 z30.d, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with unsigned immediate") {
  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0),  "cmphi p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphi p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127),  "cmphi p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphi p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0),  "cmphs p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmphs p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127),  "cmphs p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmphs p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmplo(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0),  "cmplo p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmplo(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmplo(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmplo(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmplo p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmplo(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127),  "cmplo p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmplo(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmplo(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmplo(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmplo p6.d, p5/z, z30.d, #127");

  TEST_SINGLE(cmpls(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0),  "cmpls p6.b, p5/z, z30.b, #0");
  TEST_SINGLE(cmpls(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.h, p5/z, z30.h, #0");
  TEST_SINGLE(cmpls(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.s, p5/z, z30.s, #0");
  TEST_SINGLE(cmpls(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 0), "cmpls p6.d, p5/z, z30.d, #0");
  TEST_SINGLE(cmpls(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127),  "cmpls p6.b, p5/z, z30.b, #127");
  TEST_SINGLE(cmpls(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.h, p5/z, z30.h, #127");
  TEST_SINGLE(cmpls(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.s, p5/z, z30.s, #127");
  TEST_SINGLE(cmpls(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 127), "cmpls p6.d, p5/z, z30.d, #127");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with signed immediate") {
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16),  "cmpeq p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpeq p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),   "cmpeq p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpeq p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpeq p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpeq p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16),  "cmpgt p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpgt p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),   "cmpgt p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpgt p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpgt p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpgt p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16),  "cmpge p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmpge p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),   "cmpge p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpge p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpge p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmpge p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmplt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16),  "cmplt p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmplt p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmplt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),   "cmplt p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmplt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmplt p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmplt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmplt p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmplt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmplt p6.d, p5/z, z30.d, #15");

  TEST_SINGLE(cmple(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16),  "cmple p6.b, p5/z, z30.b, #-16");
  TEST_SINGLE(cmple(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.h, p5/z, z30.h, #-16");
  TEST_SINGLE(cmple(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.s, p5/z, z30.s, #-16");
  TEST_SINGLE(cmple(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, -16), "cmple p6.d, p5/z, z30.d, #-16");
  TEST_SINGLE(cmple(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),   "cmple p6.b, p5/z, z30.b, #15");
  TEST_SINGLE(cmple(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmple p6.h, p5/z, z30.h, #15");
  TEST_SINGLE(cmple(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmple p6.s, p5/z, z30.s, #15");
  TEST_SINGLE(cmple(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, 15),  "cmple p6.d, p5/z, z30.d, #15");
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
  TEST_SINGLE(match(SubRegSize::i8Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29),
              "match p8.b, p6/z, z30.b, z29.b");
  TEST_SINGLE(match(SubRegSize::i16Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29),
              "match p8.h, p6/z, z30.h, z29.h");

  TEST_SINGLE(nmatch(SubRegSize::i8Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29),
              "nmatch p8.b, p6/z, z30.b, z29.b");
  TEST_SINGLE(nmatch(SubRegSize::i16Bit, PReg::p8, PReg::p6.Zeroing(), ZReg::z30, ZReg::z29),
              "nmatch p8.h, p6/z, z30.h, z29.h");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point convert precision odd elements") {
  TEST_SINGLE(fcvtxnt(ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtxnt z30.s, p6/m, z29.d");
  TEST_SINGLE(fcvtnt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.h, p6/m, z29.s");
  TEST_SINGLE(fcvtnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.s, p6/m, z29.d");
  //TEST_SINGLE(fcvtnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtnt z30.d, p6/m, z29.d");

  //TEST_SINGLE(fcvtlt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.h, p6/m, z29.b");
  TEST_SINGLE(fcvtlt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtlt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fcvtlt z30.d, p6/m, z29.s");


  //void fcvtxnt(FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
  /////< Size is destination size
  //void fcvtnt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {
  /////< Size is destination size
  //void fcvtlt(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::ZRegister zd, FEXCore::ARMEmitter::PRegister pg, FEXCore::ARMEmitter::ZRegister zn) {

  // XXX: BFCVTNT
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 floating-point pairwise operations") {
  //TEST_SINGLE(faddp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "faddp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(faddp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "faddp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(faddp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "faddp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(faddp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "faddp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(faddp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "faddp z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmaxnmp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmaxnmp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmaxnmp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnmp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmaxnmp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnmp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmaxnmp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnmp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmaxnmp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnmp z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fminnmp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fminnmp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fminnmp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnmp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fminnmp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnmp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fminnmp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnmp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fminnmp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnmp z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmax z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmax(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmin z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmin(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point complex add") {
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90),
              "fcadd z30.h, p6/m, z30.h, z28.h, #90");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90),
              "fcadd z30.s, p6/m, z30.s, z28.s, #90");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_90),
              "fcadd z30.d, p6/m, z30.d, z28.d, #90");

  TEST_SINGLE(fcadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270),
              "fcadd z30.h, p6/m, z30.h, z28.h, #270");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270),
              "fcadd z30.s, p6/m, z30.s, z28.s, #270");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28, Rotation::ROTATE_270),
              "fcadd z30.d, p6/m, z30.d, z28.d, #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add (vector)") {
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0),
              "fcmla z30.h, p6/m, z10.h, z28.h, #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0),
              "fcmla z30.s, p6/m, z10.s, z28.s, #0");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_0),
              "fcmla z30.d, p6/m, z10.d, z28.d, #0");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90),
              "fcmla z30.h, p6/m, z10.h, z28.h, #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90),
              "fcmla z30.s, p6/m, z10.s, z28.s, #90");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_90),
              "fcmla z30.d, p6/m, z10.d, z28.d, #90");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180),
              "fcmla z30.h, p6/m, z10.h, z28.h, #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180),
              "fcmla z30.s, p6/m, z10.s, z28.s, #180");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_180),
              "fcmla z30.d, p6/m, z10.d, z28.d, #180");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270),
              "fcmla z30.h, p6/m, z10.h, z28.h, #270");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270),
              "fcmla z30.s, p6/m, z10.s, z28.s, #270");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z10, ZReg::z28, Rotation::ROTATE_270),
              "fcmla z30.d, p6/m, z10.d, z28.d, #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add (indexed)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point complex multiply-add (indexed)") {
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 0, Rotation::ROTATE_0),
              "fcmla z30.h, z10.h, z7.h[0], #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 0, Rotation::ROTATE_0),
              "fcmla z30.s, z10.s, z15.s[0], #0");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 1, Rotation::ROTATE_90),
              "fcmla z30.h, z10.h, z7.h[1], #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_90),
              "fcmla z30.s, z10.s, z15.s[1], #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_180),
              "fcmla z30.s, z10.s, z15.s[1], #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, ZReg::z30, ZReg::z10, ZReg::z15, 1, Rotation::ROTATE_270),
              "fcmla z30.s, z10.s, z15.s[1], #270");

  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 2, Rotation::ROTATE_180),
              "fcmla z30.h, z10.h, z7.h[2], #180");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, ZReg::z30, ZReg::z10, ZReg::z7, 3, Rotation::ROTATE_270),
              "fcmla z30.h, z10.h, z7.h[3], #270");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply (indexed)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating point matrix multiply accumulate") {
  // TODO: Implement in emitter.
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
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic (unpredicated)") {
  //TEST_SINGLE(fadd(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fadd z30.b, z29.b, z28.b");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fadd z30.h, z29.h, z28.h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fadd z30.s, z29.s, z28.s");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fadd z30.d, z29.d, z28.d");
  //TEST_SINGLE(fadd(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fadd z30.q, z29.q, z28.q");

  //TEST_SINGLE(fsub(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fsub z30.b, z29.b, z28.b");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fsub z30.h, z29.h, z28.h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fsub z30.s, z29.s, z28.s");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fsub z30.d, z29.d, z28.d");
  //TEST_SINGLE(fsub(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fsub z30.q, z29.q, z28.q");

  //TEST_SINGLE(fmul(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "fmul z30.b, z29.b, z28.b");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fmul z30.h, z29.h, z28.h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fmul z30.s, z29.s, z28.s");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "fmul z30.d, z29.d, z28.d");
  //TEST_SINGLE(fmul(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "fmul z30.q, z29.q, z28.q");

  //TEST_SINGLE(ftsmul(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "ftsmul z30.b, z29.b, z28.b");
  TEST_SINGLE(ftsmul(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "ftsmul z30.h, z29.h, z28.h");
  TEST_SINGLE(ftsmul(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "ftsmul z30.s, z29.s, z28.s");
  TEST_SINGLE(ftsmul(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "ftsmul z30.d, z29.d, z28.d");
  //TEST_SINGLE(ftsmul(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ftsmul z30.q, z29.q, z28.q");

  //TEST_SINGLE(frecps(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "frecps z30.b, z29.b, z28.b");
  TEST_SINGLE(frecps(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frecps z30.h, z29.h, z28.h");
  TEST_SINGLE(frecps(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frecps z30.s, z29.s, z28.s");
  TEST_SINGLE(frecps(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frecps z30.d, z29.d, z28.d");
  //TEST_SINGLE(frecps(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frecps z30.q, z29.q, z28.q");

  //TEST_SINGLE(frsqrts(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28),   "frsqrts z30.b, z29.b, z28.b");
  TEST_SINGLE(frsqrts(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frsqrts z30.h, z29.h, z28.h");
  TEST_SINGLE(frsqrts(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frsqrts z30.s, z29.s, z28.s");
  TEST_SINGLE(frsqrts(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28),  "frsqrts z30.d, z29.d, z28.d");
  //TEST_SINGLE(frsqrts(SubRegSize::i128Bit, ZReg::z30, ZReg::z29, ZReg::z28), "frsqrts z30.q, z29.q, z28.q");
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
  TEST_SINGLE(mla(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29),  "mla z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(mla(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(mla(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(mla(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mla z30.d, p7/m, z28.d, z29.d");

  TEST_SINGLE(mls(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29),  "mls z30.b, p7/m, z28.b, z29.b");
  TEST_SINGLE(mls(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.h, p7/m, z28.h, z29.h");
  TEST_SINGLE(mls(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.s, p7/m, z28.s, z29.s");
  TEST_SINGLE(mls(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z28, ZReg::z29), "mls z30.d, p7/m, z28.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply-add writing multiplicand (predicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract vectors (predicated)") {
  TEST_SINGLE(add(SubRegSize::i8Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28),  "add z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(add(SubRegSize::i16Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "add z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(add(SubRegSize::i32Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "add z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(add(SubRegSize::i64Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "add z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(sub(SubRegSize::i8Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28),  "sub z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "sub z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "sub z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(sub(SubRegSize::i64Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "sub z30.d, p7/m, z30.d, z28.d");

  TEST_SINGLE(subr(SubRegSize::i8Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28),  "subr z30.b, p7/m, z30.b, z28.b");
  TEST_SINGLE(subr(SubRegSize::i16Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "subr z30.h, p7/m, z30.h, z28.h");
  TEST_SINGLE(subr(SubRegSize::i32Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "subr z30.s, p7/m, z30.s, z28.s");
  TEST_SINGLE(subr(SubRegSize::i64Bit, ZReg::z30, PReg::p7, ZReg::z30, ZReg::z28), "subr z30.d, p7/m, z30.d, z28.d");
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
  TEST_SINGLE(mul(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "mul z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(mul(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "mul z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(mul(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "mul z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(mul(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "mul z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(smulh(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "smulh z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(smulh(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "smulh z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(smulh(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "smulh z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(smulh(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "smulh z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(umulh(SubRegSize::i8Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "umulh z30.b, p7/m, z30.b, z29.b");
  TEST_SINGLE(umulh(SubRegSize::i16Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "umulh z30.h, p7/m, z30.h, z29.h");
  TEST_SINGLE(umulh(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "umulh z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(umulh(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "umulh z30.d, p7/m, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer divide vectors (predicated)") {
  TEST_SINGLE(sdiv(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "sdiv z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(sdiv(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "sdiv z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(udiv(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "udiv z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(udiv(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "udiv z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(sdivr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "sdivr z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(sdivr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "sdivr z30.d, p7/m, z30.d, z29.d");

  TEST_SINGLE(udivr(SubRegSize::i32Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "udivr z30.s, p7/m, z30.s, z29.s");
  TEST_SINGLE(udivr(SubRegSize::i64Bit, ZReg::z30, PReg::p7.Merging(), ZReg::z30, ZReg::z29),
              "udivr z30.d, p7/m, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical operations (predicated)") {
  TEST_SINGLE(orr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),   "orr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(orr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "orr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(orr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "orr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(orr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "orr z30.d, p6/m, z30.d, z29.d");
  //TEST_SINGLE(orr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "orr z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(eor(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),   "eor z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(eor(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "eor z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(eor(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "eor z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(eor(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "eor z30.d, p6/m, z30.d, z29.d");
  //TEST_SINGLE(eor(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "eor z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(and_(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),   "and z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(and_(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "and z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(and_(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "and z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(and_(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "and z30.d, p6/m, z30.d, z29.d");
  //TEST_SINGLE(and_(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "and z30.q, p6/m, z30.q, z29.q");

  TEST_SINGLE(bic(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),   "bic z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(bic(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "bic z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(bic(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "bic z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(bic(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "bic z30.d, p6/m, z30.d, z29.d");
  //TEST_SINGLE(bic(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "bic z30.q, p6/m, z30.q, z29.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add reduction (predicated)") {
  TEST_SINGLE(saddv(SubRegSize::i8Bit, DReg::d30, PReg::p7, ZReg::z29),  "saddv d30, p7, z29.b");
  TEST_SINGLE(saddv(SubRegSize::i16Bit, DReg::d30, PReg::p7, ZReg::z29), "saddv d30, p7, z29.h");
  TEST_SINGLE(saddv(SubRegSize::i32Bit, DReg::d30, PReg::p7, ZReg::z29), "saddv d30, p7, z29.s");

  TEST_SINGLE(uaddv(SubRegSize::i8Bit, DReg::d30, PReg::p7, ZReg::z29),  "uaddv d30, p7, z29.b");
  TEST_SINGLE(uaddv(SubRegSize::i16Bit, DReg::d30, PReg::p7, ZReg::z29), "uaddv d30, p7, z29.h");
  TEST_SINGLE(uaddv(SubRegSize::i32Bit, DReg::d30, PReg::p7, ZReg::z29), "uaddv d30, p7, z29.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer min/max reduction (predicated)") {
  TEST_SINGLE(smaxv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29),   "smaxv b30, p6, z29.b");
  TEST_SINGLE(smaxv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29),  "smaxv h30, p6, z29.h");
  TEST_SINGLE(smaxv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29),  "smaxv s30, p6, z29.s");
  TEST_SINGLE(smaxv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29),  "smaxv d30, p6, z29.d");

  TEST_SINGLE(umaxv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29),   "umaxv b30, p6, z29.b");
  TEST_SINGLE(umaxv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29),  "umaxv h30, p6, z29.h");
  TEST_SINGLE(umaxv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29),  "umaxv s30, p6, z29.s");
  TEST_SINGLE(umaxv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29),  "umaxv d30, p6, z29.d");

  TEST_SINGLE(sminv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29),   "sminv b30, p6, z29.b");
  TEST_SINGLE(sminv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29),  "sminv h30, p6, z29.h");
  TEST_SINGLE(sminv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29),  "sminv s30, p6, z29.s");
  TEST_SINGLE(sminv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29),  "sminv d30, p6, z29.d");

  TEST_SINGLE(uminv(SubRegSize::i8Bit, VReg::v30, PReg::p6, ZReg::z29),   "uminv b30, p6, z29.b");
  TEST_SINGLE(uminv(SubRegSize::i16Bit, VReg::v30, PReg::p6, ZReg::z29),  "uminv h30, p6, z29.h");
  TEST_SINGLE(uminv(SubRegSize::i32Bit, VReg::v30, PReg::p6, ZReg::z29),  "uminv s30, p6, z29.s");
  TEST_SINGLE(uminv(SubRegSize::i64Bit, VReg::v30, PReg::p6, ZReg::z29),  "uminv d30, p6, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE constructive prefix (predicated)") {
  TEST_SINGLE(movprfx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "movprfx z30.b, p6/m, z29.b");
  TEST_SINGLE(movprfx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "movprfx z30.h, p6/m, z29.h");
  TEST_SINGLE(movprfx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "movprfx z30.s, p6/m, z29.s");
  TEST_SINGLE(movprfx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "movprfx z30.d, p6/m, z29.d");
  //TEST_SINGLE(movprfx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "movprfx z30.q, p6/m, z29.q");
  TEST_SINGLE(movprfx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29),   "movprfx z30.b, p6/z, z29.b");
  TEST_SINGLE(movprfx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29),  "movprfx z30.h, p6/z, z29.h");
  TEST_SINGLE(movprfx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29),  "movprfx z30.s, p6/z, z29.s");
  TEST_SINGLE(movprfx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29),  "movprfx z30.d, p6/z, z29.d");
  //TEST_SINGLE(movprfx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Zeroing(), ZReg::z29), "movprfx z30.q, p6/z, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical reduction (predicated)") {
  TEST_SINGLE(orv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29),  "orv b30, p7, z29.b");
  TEST_SINGLE(orv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z29), "orv h30, p7, z29.h");
  TEST_SINGLE(orv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z29), "orv s30, p7, z29.s");
  TEST_SINGLE(orv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z29), "orv d30, p7, z29.d");

  TEST_SINGLE(eorv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29),  "eorv b30, p7, z29.b");
  TEST_SINGLE(eorv(SubRegSize::i16Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv h30, p7, z29.h");
  TEST_SINGLE(eorv(SubRegSize::i32Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv s30, p7, z29.s");
  TEST_SINGLE(eorv(SubRegSize::i64Bit, VReg::v30, PReg::p7, ZReg::z29), "eorv d30, p7, z29.d");

  TEST_SINGLE(andv(SubRegSize::i8Bit, VReg::v30, PReg::p7, ZReg::z29),  "andv b30, p7, z29.b");
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
  TEST_SINGLE(asr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "asr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(asr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(asr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(asr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "lsr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsl(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "lsl z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsl(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsl(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsl(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsl z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(asrr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "asrr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(asrr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(asrr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(asrr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "asrr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lsrr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "lsrr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lsrr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lsrr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lsrr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lsrr z30.d, p6/m, z30.d, z29.d");

  TEST_SINGLE(lslr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29),  "lslr z30.b, p6/m, z30.b, z29.b");
  TEST_SINGLE(lslr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.h, p6/m, z30.h, z29.h");
  TEST_SINGLE(lslr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.s, p6/m, z30.s, z29.s");
  TEST_SINGLE(lslr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z29), "lslr z30.d, p6/m, z30.d, z29.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by wide elements (predicated)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer unary operations (predicated)") {
  //TEST_SINGLE(sxtb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxtb z30.b, p6/m, z29.b");
  TEST_SINGLE(sxtb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtb z30.h, p6/m, z29.h");
  TEST_SINGLE(sxtb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtb z30.s, p6/m, z29.s");
  TEST_SINGLE(sxtb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtb z30.d, p6/m, z29.d");
  //TEST_SINGLE(sxtb(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtb z30.q, p6/m, z29.q");

  //TEST_SINGLE(uxtb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxtb z30.b, p6/m, z29.b");
  TEST_SINGLE(uxtb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtb z30.h, p6/m, z29.h");
  TEST_SINGLE(uxtb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtb z30.s, p6/m, z29.s");
  TEST_SINGLE(uxtb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtb z30.d, p6/m, z29.d");
  //TEST_SINGLE(uxtb(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtb z30.q, p6/m, z29.q");

  //TEST_SINGLE(sxth(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxth z30.b, p6/m, z29.b");
  //TEST_SINGLE(sxth(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxth z30.h, p6/m, z29.h");
  TEST_SINGLE(sxth(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxth z30.s, p6/m, z29.s");
  TEST_SINGLE(sxth(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxth z30.d, p6/m, z29.d");
  //TEST_SINGLE(sxth(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxth z30.q, p6/m, z29.q");

  //TEST_SINGLE(uxth(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxth z30.b, p6/m, z29.b");
  //TEST_SINGLE(uxth(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxth z30.h, p6/m, z29.h");
  TEST_SINGLE(uxth(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxth z30.s, p6/m, z29.s");
  TEST_SINGLE(uxth(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxth z30.d, p6/m, z29.d");
  //TEST_SINGLE(uxth(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxth z30.q, p6/m, z29.q");

  //TEST_SINGLE(sxtw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "sxtw z30.b, p6/m, z29.b");
  //TEST_SINGLE(sxtw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtw z30.h, p6/m, z29.h");
  //TEST_SINGLE(sxtw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtw z30.s, p6/m, z29.s");
  TEST_SINGLE(sxtw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "sxtw z30.d, p6/m, z29.d");
  //TEST_SINGLE(sxtw(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "sxtw z30.q, p6/m, z29.q");

  //TEST_SINGLE(uxtw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "uxtw z30.b, p6/m, z29.b");
  //TEST_SINGLE(uxtw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtw z30.h, p6/m, z29.h");
  //TEST_SINGLE(uxtw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtw z30.s, p6/m, z29.s");
  TEST_SINGLE(uxtw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "uxtw z30.d, p6/m, z29.d");
  //TEST_SINGLE(uxtw(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "uxtw z30.q, p6/m, z29.q");

  TEST_SINGLE(abs(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "abs z30.b, p6/m, z29.b");
  TEST_SINGLE(abs(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "abs z30.h, p6/m, z29.h");
  TEST_SINGLE(abs(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "abs z30.s, p6/m, z29.s");
  TEST_SINGLE(abs(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "abs z30.d, p6/m, z29.d");
  //TEST_SINGLE(abs(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "abs z30.q, p6/m, z29.q");

  TEST_SINGLE(neg(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "neg z30.b, p6/m, z29.b");
  TEST_SINGLE(neg(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "neg z30.h, p6/m, z29.h");
  TEST_SINGLE(neg(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "neg z30.s, p6/m, z29.s");
  TEST_SINGLE(neg(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "neg z30.d, p6/m, z29.d");
  //TEST_SINGLE(neg(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "neg z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise unary operations (predicated)") {
  TEST_SINGLE(cls(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "cls z30.b, p6/m, z29.b");
  TEST_SINGLE(cls(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cls z30.h, p6/m, z29.h");
  TEST_SINGLE(cls(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cls z30.s, p6/m, z29.s");
  TEST_SINGLE(cls(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cls z30.d, p6/m, z29.d");
  //TEST_SINGLE(cls(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cls z30.q, p6/m, z29.q");

  TEST_SINGLE(clz(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "clz z30.b, p6/m, z29.b");
  TEST_SINGLE(clz(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "clz z30.h, p6/m, z29.h");
  TEST_SINGLE(clz(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "clz z30.s, p6/m, z29.s");
  TEST_SINGLE(clz(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "clz z30.d, p6/m, z29.d");
  //TEST_SINGLE(clz(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "clz z30.q, p6/m, z29.q");

  TEST_SINGLE(cnt(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "cnt z30.b, p6/m, z29.b");
  TEST_SINGLE(cnt(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnt z30.h, p6/m, z29.h");
  TEST_SINGLE(cnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnt z30.s, p6/m, z29.s");
  TEST_SINGLE(cnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnt z30.d, p6/m, z29.d");
  //TEST_SINGLE(cnt(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnt z30.q, p6/m, z29.q");

  TEST_SINGLE(cnot(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "cnot z30.b, p6/m, z29.b");
  TEST_SINGLE(cnot(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnot z30.h, p6/m, z29.h");
  TEST_SINGLE(cnot(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnot z30.s, p6/m, z29.s");
  TEST_SINGLE(cnot(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "cnot z30.d, p6/m, z29.d");
  //TEST_SINGLE(cnot(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "cnot z30.q, p6/m, z29.q");

  //TEST_SINGLE(fabs(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "fabs z30.b, p6/m, z29.b");
  TEST_SINGLE(fabs(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fabs z30.h, p6/m, z29.h");
  TEST_SINGLE(fabs(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fabs z30.s, p6/m, z29.s");
  TEST_SINGLE(fabs(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fabs z30.d, p6/m, z29.d");
  //TEST_SINGLE(fabs(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fabs z30.q, p6/m, z29.q");

  //TEST_SINGLE(fneg(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "fneg z30.b, p6/m, z29.b");
  TEST_SINGLE(fneg(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fneg z30.h, p6/m, z29.h");
  TEST_SINGLE(fneg(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fneg z30.s, p6/m, z29.s");
  TEST_SINGLE(fneg(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "fneg z30.d, p6/m, z29.d");
  //TEST_SINGLE(fneg(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "fneg z30.q, p6/m, z29.q");

  TEST_SINGLE(not_(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),   "not z30.b, p6/m, z29.b");
  TEST_SINGLE(not_(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "not z30.h, p6/m, z29.h");
  TEST_SINGLE(not_(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "not z30.s, p6/m, z29.s");
  TEST_SINGLE(not_(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29),  "not z30.d, p6/m, z29.d");
  //TEST_SINGLE(not_(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "not z30.q, p6/m, z29.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise logical operations (unpredicated)") {
  TEST_SINGLE(and_(ZReg::z30, ZReg::z29, ZReg::z28), "and z30.d, z29.d, z28.d");
  TEST_SINGLE(orr(ZReg::z30, ZReg::z29, ZReg::z28), "orr z30.d, z29.d, z28.d");
  TEST_SINGLE(mov(ZReg::z30, ZReg::z29), "mov z30.d, z29.d");
  TEST_SINGLE(eor(ZReg::z30, ZReg::z29, ZReg::z28), "eor z30.d, z29.d, z28.d");
  TEST_SINGLE(bic(ZReg::z30, ZReg::z29, ZReg::z28), "bic z30.d, z29.d, z28.d");
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
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE stack frame adjustment") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: Streaming SVE stack frame adjustment") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE stack frame size") {
  // TODO: Implement in emitter.
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
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by wide elements (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE bitwise shift by immediate (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point trig select coefficient") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point exponential accelerator") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE constructive prefix (unpredicated)") {
  TEST_SINGLE(movprfx(ZReg::z30, ZReg::z29), "movprfx z30, z29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec vector by element count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE element count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec vector by element count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec register by element count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec register by element count") {
  // TODO: Implement in emitter.
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
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Unpredicated") {
  TEST_SINGLE(dup(SubRegSize::i8Bit, ZReg::z30, Reg::r29),   "mov z30.b, w29");
  TEST_SINGLE(dup(SubRegSize::i16Bit, ZReg::z30, Reg::r29),  "mov z30.h, w29");
  TEST_SINGLE(dup(SubRegSize::i32Bit, ZReg::z30, Reg::r29),  "mov z30.s, w29");
  TEST_SINGLE(dup(SubRegSize::i64Bit, ZReg::z30, Reg::r29),  "mov z30.d, x29");
  //TEST_SINGLE(dup(SubRegSize::i128Bit, ZReg::z30, Reg::r29), "mov z30.q, x29");
  TEST_SINGLE(mov(SubRegSize::i8Bit, ZReg::z30, Reg::r29),   "mov z30.b, w29");
  TEST_SINGLE(mov(SubRegSize::i16Bit, ZReg::z30, Reg::r29),  "mov z30.h, w29");
  TEST_SINGLE(mov(SubRegSize::i32Bit, ZReg::z30, Reg::r29),  "mov z30.s, w29");
  TEST_SINGLE(mov(SubRegSize::i64Bit, ZReg::z30, Reg::r29),  "mov z30.d, x29");
  //TEST_SINGLE(mov(SubRegSize::i128Bit, ZReg::z30, Reg::r29), "mov z30.q, x29");
  // XXX: INSR
  // XXX: INSR SIMD
  // XXX: REV
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE unpack vector elements") {
  //TEST_SINGLE(sunpklo(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "sunpklo z30.b, z29.b");
  TEST_SINGLE(sunpklo(SubRegSize::i16Bit, ZReg::z30, ZReg::z29),  "sunpklo z30.h, z29.b");
  TEST_SINGLE(sunpklo(SubRegSize::i32Bit, ZReg::z30, ZReg::z29),  "sunpklo z30.s, z29.h");
  TEST_SINGLE(sunpklo(SubRegSize::i64Bit, ZReg::z30, ZReg::z29),  "sunpklo z30.d, z29.s");
  //TEST_SINGLE(sunpklo(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "sunpklo z30.q, z29.q");

  //TEST_SINGLE(sunpkhi(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "sunpkhi z30.b, z29.b");
  TEST_SINGLE(sunpkhi(SubRegSize::i16Bit, ZReg::z30, ZReg::z29),  "sunpkhi z30.h, z29.b");
  TEST_SINGLE(sunpkhi(SubRegSize::i32Bit, ZReg::z30, ZReg::z29),  "sunpkhi z30.s, z29.h");
  TEST_SINGLE(sunpkhi(SubRegSize::i64Bit, ZReg::z30, ZReg::z29),  "sunpkhi z30.d, z29.s");
  //TEST_SINGLE(sunpkhi(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "sunpkhi z30.q, z29.q");

  //TEST_SINGLE(uunpklo(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "uunpklo z30.b, z29.b");
  TEST_SINGLE(uunpklo(SubRegSize::i16Bit, ZReg::z30, ZReg::z29),  "uunpklo z30.h, z29.b");
  TEST_SINGLE(uunpklo(SubRegSize::i32Bit, ZReg::z30, ZReg::z29),  "uunpklo z30.s, z29.h");
  TEST_SINGLE(uunpklo(SubRegSize::i64Bit, ZReg::z30, ZReg::z29),  "uunpklo z30.d, z29.s");
  //TEST_SINGLE(uunpklo(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "uunpklo z30.q, z29.q");

  //TEST_SINGLE(uunpkhi(SubRegSize::i8Bit, ZReg::z30, ZReg::z29),   "uunpkhi z30.b, z29.b");
  TEST_SINGLE(uunpkhi(SubRegSize::i16Bit, ZReg::z30, ZReg::z29),  "uunpkhi z30.h, z29.b");
  TEST_SINGLE(uunpkhi(SubRegSize::i32Bit, ZReg::z30, ZReg::z29),  "uunpkhi z30.s, z29.h");
  TEST_SINGLE(uunpkhi(SubRegSize::i64Bit, ZReg::z30, ZReg::z29),  "uunpkhi z30.d, z29.s");
  //TEST_SINGLE(uunpkhi(SubRegSize::i128Bit, ZReg::z30, ZReg::z29), "uunpkhi z30.q, z29.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Predicate - Base") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Predicate") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE permute predicate elements") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Predicated - Base") {
  // XXX: CPY (SIMD&FP scalar)
  //TEST_SINGLE(compact(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z29),   "compact z30.b, p6, z29.b");
  //TEST_SINGLE(compact(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z29),  "compact z30.h, p6, z29.h");
  TEST_SINGLE(compact(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z29),  "compact z30.s, p6, z29.s");
  TEST_SINGLE(compact(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z29),  "compact z30.d, p6, z29.d");
  //TEST_SINGLE(compact(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z29), "compact z30.q, p6, z29.q");
  // XXX: CPY (scalar)

  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29),   "splice z30.b, p6, {z28.b, z29.b}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29),  "splice z30.h, p6, {z28.h, z29.h}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29),  "splice z30.s, p6, {z28.s, z29.s}");
  TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29),  "splice z30.d, p6, {z28.d, z29.d}");
  //TEST_SINGLE(splice<OpType::Constructive>(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z28, ZReg::z29), "splice z30.q, p6, {z28.q, z29.q}");

  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i8Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28),   "splice z30.b, p6, z30.b, z28.b");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i16Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28),  "splice z30.h, p6, z30.h, z28.h");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i32Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28),  "splice z30.s, p6, z30.s, z28.s");
  TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i64Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28),  "splice z30.d, p6, z30.d, z28.d");
  //TEST_SINGLE(splice<OpType::Destructive>(SubRegSize::i128Bit, ZReg::z30, PReg::p6, ZReg::z30, ZReg::z28), "splice z30.q, p6, z30.q, z28.q");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Predicated") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE extract element to SIMD&FP scalar register") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE reverse within elements") {
  //TEST_SINGLE(revb(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.b, p6/m, z29.b");
  TEST_SINGLE(revb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.h, p6/m, z29.h");
  TEST_SINGLE(revb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.s, p6/m, z29.s");
  TEST_SINGLE(revb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revb z30.d, p6/m, z29.d");

  //TEST_SINGLE(revh(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.b, p6/m, z29.b");
  //TEST_SINGLE(revh(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.h, p6/m, z29.h");
  TEST_SINGLE(revh(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.s, p6/m, z29.s");
  TEST_SINGLE(revh(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revh z30.d, p6/m, z29.d");

  //TEST_SINGLE(revw(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.b, p6/m, z29.b");
  //TEST_SINGLE(revw(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.h, p6/m, z29.h");
  //TEST_SINGLE(revw(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.s, p6/m, z29.s");
  TEST_SINGLE(revw(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "revw z30.d, p6/m, z29.d");

  TEST_SINGLE(rbit(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.b, p6/m, z29.b");
  TEST_SINGLE(rbit(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.h, p6/m, z29.h");
  TEST_SINGLE(rbit(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.s, p6/m, z29.s");
  TEST_SINGLE(rbit(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "rbit z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally broadcast element to vector") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally extract element to SIMD&FP scalar") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE reverse doublewords") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally extract element to general register") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Permute Vector - Extract") {
  TEST_SINGLE(ext<FEXCore::ARMEmitter::OpType::Destructive>(ZReg::z30, ZReg::z30, ZReg::z29, 0), "ext z30.b, z30.b, z29.b, #0");
  TEST_SINGLE(ext<FEXCore::ARMEmitter::OpType::Destructive>(ZReg::z30, ZReg::z30, ZReg::z29, 255), "ext z30.b, z30.b, z29.b, #255");

  TEST_SINGLE(ext<FEXCore::ARMEmitter::OpType::Constructive>(ZReg::z30, ZReg::z28, ZReg::z29, 0), "ext z30.b, {z28.b, z29.b}, #0");
  TEST_SINGLE(ext<FEXCore::ARMEmitter::OpType::Constructive>(ZReg::z30, ZReg::z28, ZReg::z29, 255), "ext z30.b, {z28.b, z29.b}, #255");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE permute vector segments") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare vectors") {
  TEST_SINGLE(cmpeq(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmpeq p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpeq(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpeq(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpeq(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpeq p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpge(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmpge p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpge(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpge(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpge(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpge p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpgt(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmpgt p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpgt(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpgt(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpgt(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpgt p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmphi(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmphi p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmphi(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmphi(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmphi(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphi p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmphs(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmphs p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmphs(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmphs(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmphs(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmphs p6.d, p5/z, z30.d, z29.d");

  TEST_SINGLE(cmpne(SubRegSize::i8Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29),  "cmpne p6.b, p5/z, z30.b, z29.b");
  TEST_SINGLE(cmpne(SubRegSize::i16Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.h, p5/z, z30.h, z29.h");
  TEST_SINGLE(cmpne(SubRegSize::i32Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.s, p5/z, z30.s, z29.s");
  TEST_SINGLE(cmpne(SubRegSize::i64Bit, PReg::p6, PReg::p5.Zeroing(), ZReg::z30, ZReg::z29), "cmpne p6.d, p5/z, z30.d, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare with wide elements") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE propagate break from previous partition") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Predicate Misc") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate test") {
  TEST_SINGLE(ptest(PReg::p6, PReg::p5), "ptest p6, p5.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate first active") {
  TEST_SINGLE(pfirst(PReg::p6, PReg::p5, PReg::p6), "pfirst p6.b, p5, p6.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate zero") {
  TEST_SINGLE(pfalse(PReg::p6), "pfalse p6.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate read from FFR (predicated)") {
  TEST_SINGLE(rdffr(PReg::p6, PReg::p5), "rdffr p6.b, p5/z");
  TEST_SINGLE(rdffrs(PReg::p6, PReg::p5), "rdffrs p6.b, p5/z");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate read from FFR (unpredicated)") {
  TEST_SINGLE(rdffr(PReg::p6), "rdffr p6.b");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate initialize") {
  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.b, pow2");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.h, pow2");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.s, pow2");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrue p6.d, pow2");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.b, pow2");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.h, pow2");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.s, pow2");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_POW2), "ptrues p6.d, pow2");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.b, vl1");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.h, vl1");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.s, vl1");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrue p6.d, vl1");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.b, vl1");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.h, vl1");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.s, vl1");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL1), "ptrues p6.d, vl1");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.b, vl2");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.h, vl2");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.s, vl2");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrue p6.d, vl2");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.b, vl2");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.h, vl2");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.s, vl2");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL2), "ptrues p6.d, vl2");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.b, vl3");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.h, vl3");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.s, vl3");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrue p6.d, vl3");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.b, vl3");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.h, vl3");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.s, vl3");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL3), "ptrues p6.d, vl3");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.b, vl4");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.h, vl4");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.s, vl4");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrue p6.d, vl4");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.b, vl4");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.h, vl4");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.s, vl4");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL4), "ptrues p6.d, vl4");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.b, vl5");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.h, vl5");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.s, vl5");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrue p6.d, vl5");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.b, vl5");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.h, vl5");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.s, vl5");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL5), "ptrues p6.d, vl5");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.b, vl6");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.h, vl6");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.s, vl6");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrue p6.d, vl6");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.b, vl6");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.h, vl6");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.s, vl6");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL6), "ptrues p6.d, vl6");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.b, vl7");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.h, vl7");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.s, vl7");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrue p6.d, vl7");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.b, vl7");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.h, vl7");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.s, vl7");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL7), "ptrues p6.d, vl7");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.b, vl8");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.h, vl8");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.s, vl8");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrue p6.d, vl8");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.b, vl8");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.h, vl8");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.s, vl8");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL8), "ptrues p6.d, vl8");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.b, vl16");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.h, vl16");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.s, vl16");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrue p6.d, vl16");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.b, vl16");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.h, vl16");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.s, vl16");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL16), "ptrues p6.d, vl16");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.b, vl32");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.h, vl32");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.s, vl32");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrue p6.d, vl32");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.b, vl32");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.h, vl32");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.s, vl32");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL32), "ptrues p6.d, vl32");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.b, vl64");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.h, vl64");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.s, vl64");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrue p6.d, vl64");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.b, vl64");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.h, vl64");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.s, vl64");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL64), "ptrues p6.d, vl64");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.b, vl128");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.h, vl128");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.s, vl128");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrue p6.d, vl128");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.b, vl128");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.h, vl128");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.s, vl128");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL128), "ptrues p6.d, vl128");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.b, vl256");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.h, vl256");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.s, vl256");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrue p6.d, vl256");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.b, vl256");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.h, vl256");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.s, vl256");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_VL256), "ptrues p6.d, vl256");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.b, mul4");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.h, mul4");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.s, mul4");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrue p6.d, mul4");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.b, mul4");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.h, mul4");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.s, mul4");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_MUL4), "ptrues p6.d, mul4");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.b, mul3");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.h, mul3");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.s, mul3");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrue p6.d, mul3");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.b, mul3");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.h, mul3");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.s, mul3");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_MUL3), "ptrues p6.d, mul3");

  TEST_SINGLE(ptrue<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.b");
  TEST_SINGLE(ptrue<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.h");
  TEST_SINGLE(ptrue<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.s");
  TEST_SINGLE(ptrue<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrue p6.d");

  TEST_SINGLE(ptrues<SubRegSize::i8Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.b");
  TEST_SINGLE(ptrues<SubRegSize::i16Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.h");
  TEST_SINGLE(ptrues<SubRegSize::i32Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.s");
  TEST_SINGLE(ptrues<SubRegSize::i64Bit>(PReg::p6, PredicatePattern::SVE_ALL), "ptrues p6.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer compare scalar count and limit") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE conditionally terminate scalars") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE pointer conflict compare") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer add/subtract immediate (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer min/max immediate (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer multiply immediate (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast integer immediate (unpredicated)") {
  TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, -128, false), "mov z30.b, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, -128, false), "mov z30.h, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, -128, false), "mov z30.s, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, -128, false), "mov z30.d, #-128");

  TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, 127, false), "mov z30.b, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, 127, false), "mov z30.h, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, 127, false), "mov z30.s, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, 127, false), "mov z30.d, #127");

  //TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, -128, true), "mov z30.b, #-128");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, -128, true), "mov z30.h, #-128, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, -128, true), "mov z30.s, #-128, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, -128, true), "mov z30.d, #-128, lsl #8");

  //TEST_SINGLE(dup_imm(SubRegSize::i8Bit, ZReg::z30, 127, true), "mov z30.b, #127");
  TEST_SINGLE(dup_imm(SubRegSize::i16Bit, ZReg::z30, 127, true), "mov z30.h, #127, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i32Bit, ZReg::z30, 127, true), "mov z30.s, #127, lsl #8");
  TEST_SINGLE(dup_imm(SubRegSize::i64Bit, ZReg::z30, 127, true), "mov z30.d, #127, lsl #8");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, -128, false), "mov z30.b, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, -128, false), "mov z30.h, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, -128, false), "mov z30.s, #-128");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, -128, false), "mov z30.d, #-128");

  TEST_SINGLE(mov_imm(SubRegSize::i8Bit, ZReg::z30, 127, false), "mov z30.b, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i16Bit, ZReg::z30, 127, false), "mov z30.h, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i32Bit, ZReg::z30, 127, false), "mov z30.s, #127");
  TEST_SINGLE(mov_imm(SubRegSize::i64Bit, ZReg::z30, 127, false), "mov z30.d, #127");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE broadcast floating-point immediate (unpredicated)") {
  TEST_SINGLE(fdup(SubRegSize::i16Bit, ZReg::z30, 1.0), "fmov z30.h, #0x70 (1.0000)");
  TEST_SINGLE(fdup(SubRegSize::i32Bit, ZReg::z30, 1.0), "fmov z30.s, #0x70 (1.0000)");
  TEST_SINGLE(fdup(SubRegSize::i64Bit, ZReg::z30, 1.0), "fmov z30.d, #0x70 (1.0000)");

  TEST_SINGLE(fmov(SubRegSize::i16Bit, ZReg::z30, 1.0), "fmov z30.h, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i32Bit, ZReg::z30, 1.0), "fmov z30.s, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, ZReg::z30, 1.0), "fmov z30.d, #0x70 (1.0000)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE predicate count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec vector by predicate count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE saturating inc/dec register by predicate count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec vector by predicate count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE inc/dec register by predicate count") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE FFR write from predicate") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE FFR initialise") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Integer Multiply-Add - Unpredicated") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer dot product (unpredicated)") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add interleaved long") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 complex integer multiply-add") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer multiply-add long") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add long") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating multiply-add high") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE mixed sign dot product") {
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer pairwise add and accumulate long") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer unary operations (predicated)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating/rounding bitwise shift left (predicated)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer halving add/subtract (predicated)") {
  TEST_SINGLE(shadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "shadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shadd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(shadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "uhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhadd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(uhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhadd z30.q, p6/m, z30.q, z28.q");
  TEST_SINGLE(shsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "shsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsub z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(shsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsub z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "uhsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsub z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(uhsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsub z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(srhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "srhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(srhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "srhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(srhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "srhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(srhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "srhadd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(srhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "srhadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(urhadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "urhadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(urhadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "urhadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(urhadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "urhadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(urhadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "urhadd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(urhadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "urhadd z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(shsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "shsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(shsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(shsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(shsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "shsubr z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(shsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "shsubr z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(uhsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "uhsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uhsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uhsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uhsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uhsubr z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(uhsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uhsubr z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer pairwise arithmetic") {
  TEST_SINGLE(addp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "addp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(addp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "addp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(addp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "addp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(addp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "addp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(addp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "addp z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(smaxp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "smaxp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(smaxp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "smaxp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(smaxp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "smaxp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(smaxp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "smaxp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(smaxp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "smaxp z30.q, p6/m, z30.q, z28.q");

  TEST_SINGLE(umaxp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "umaxp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(umaxp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "umaxp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(umaxp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "umaxp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(umaxp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "umaxp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(umaxp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "umaxp z30.q, p6/m, z30.q, z28.q");


  TEST_SINGLE(sminp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "sminp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(sminp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "sminp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(sminp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "sminp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(sminp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "sminp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(sminp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "sminp z30.q, p6/m, z30.q, z28.q");


  TEST_SINGLE(uminp(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "uminp z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(uminp(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uminp z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(uminp(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uminp z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(uminp(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "uminp z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(uminp(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "uminp z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating add/subtract") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract long") {
  //TEST_SINGLE(saddlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.b, z29.b, z28.b");
  TEST_SINGLE(saddlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.h, z29.b, z28.b");
  TEST_SINGLE(saddlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.s, z29.h, z28.h");
  TEST_SINGLE(saddlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlb z30.d, z29.s, z28.s");

  //TEST_SINGLE(saddlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.b, z29.b, z28.b");
  TEST_SINGLE(saddlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.h, z29.b, z28.b");
  TEST_SINGLE(saddlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.s, z29.h, z28.h");
  TEST_SINGLE(saddlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "saddlt z30.d, z29.s, z28.s");

  //TEST_SINGLE(uaddlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.b, z29.b, z28.b");
  TEST_SINGLE(uaddlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.h, z29.b, z28.b");
  TEST_SINGLE(uaddlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.s, z29.h, z28.h");
  TEST_SINGLE(uaddlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlb z30.d, z29.s, z28.s");

  //TEST_SINGLE(uaddlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.b, z29.b, z28.b");
  TEST_SINGLE(uaddlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.h, z29.b, z28.b");
  TEST_SINGLE(uaddlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.s, z29.h, z28.h");
  TEST_SINGLE(uaddlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uaddlt z30.d, z29.s, z28.s");

  //TEST_SINGLE(ssublb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.b, z29.b, z28.b");
  TEST_SINGLE(ssublb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.h, z29.b, z28.b");
  TEST_SINGLE(ssublb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.s, z29.h, z28.h");
  TEST_SINGLE(ssublb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublb z30.d, z29.s, z28.s");

  //TEST_SINGLE(ssublt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.b, z29.b, z28.b");
  TEST_SINGLE(ssublt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.h, z29.b, z28.b");
  TEST_SINGLE(ssublt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.s, z29.h, z28.h");
  TEST_SINGLE(ssublt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "ssublt z30.d, z29.s, z28.s");

  //TEST_SINGLE(usublb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.b, z29.b, z28.b");
  TEST_SINGLE(usublb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.h, z29.b, z28.b");
  TEST_SINGLE(usublb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.s, z29.h, z28.h");
  TEST_SINGLE(usublb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublb z30.d, z29.s, z28.s");

  //TEST_SINGLE(usublt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.b, z29.b, z28.b");
  TEST_SINGLE(usublt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.h, z29.b, z28.b");
  TEST_SINGLE(usublt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.s, z29.h, z28.h");
  TEST_SINGLE(usublt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "usublt z30.d, z29.s, z28.s");

  //TEST_SINGLE(sabdlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.b, z29.b, z28.b");
  TEST_SINGLE(sabdlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.h, z29.b, z28.b");
  TEST_SINGLE(sabdlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.s, z29.h, z28.h");
  TEST_SINGLE(sabdlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlb z30.d, z29.s, z28.s");

  //TEST_SINGLE(sabdlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.b, z29.b, z28.b");
  TEST_SINGLE(sabdlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.h, z29.b, z28.b");
  TEST_SINGLE(sabdlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.s, z29.h, z28.h");
  TEST_SINGLE(sabdlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sabdlt z30.d, z29.s, z28.s");

  //TEST_SINGLE(uabdlb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.b, z29.b, z28.b");
  TEST_SINGLE(uabdlb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.h, z29.b, z28.b");
  TEST_SINGLE(uabdlb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.s, z29.h, z28.h");
  TEST_SINGLE(uabdlb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlb z30.d, z29.s, z28.s");

  //TEST_SINGLE(uabdlt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.b, z29.b, z28.b");
  TEST_SINGLE(uabdlt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.h, z29.b, z28.b");
  TEST_SINGLE(uabdlt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.s, z29.h, z28.h");
  TEST_SINGLE(uabdlt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "uabdlt z30.d, z29.s, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract wide") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer multiply long") {
  //TEST_SINGLE(sqdmullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.b, z29.b, z28.b");
  TEST_SINGLE(sqdmullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullb z30.d, z29.s, z28.s");

  //TEST_SINGLE(sqdmullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.b, z29.b, z28.b");
  TEST_SINGLE(sqdmullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.h, z29.b, z28.b");
  TEST_SINGLE(sqdmullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.s, z29.h, z28.h");
  TEST_SINGLE(sqdmullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "sqdmullt z30.d, z29.s, z28.s");

  //TEST_SINGLE(pmullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.b, z29.b, z28.b");
  TEST_SINGLE(pmullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.h, z29.b, z28.b");
  TEST_SINGLE(pmullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.s, z29.h, z28.h");
  TEST_SINGLE(pmullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullb z30.d, z29.s, z28.s");

  //TEST_SINGLE(pmullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.b, z29.b, z28.b");
  TEST_SINGLE(pmullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.h, z29.b, z28.b");
  TEST_SINGLE(pmullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.s, z29.h, z28.h");
  TEST_SINGLE(pmullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "pmullt z30.d, z29.s, z28.s");

  //TEST_SINGLE(smullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.b, z29.b, z28.b");
  TEST_SINGLE(smullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.h, z29.b, z28.b");
  TEST_SINGLE(smullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.s, z29.h, z28.h");
  TEST_SINGLE(smullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullb z30.d, z29.s, z28.s");

  //TEST_SINGLE(smullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.b, z29.b, z28.b");
  TEST_SINGLE(smullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.h, z29.b, z28.b");
  TEST_SINGLE(smullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.s, z29.h, z28.h");
  TEST_SINGLE(smullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "smullt z30.d, z29.s, z28.s");

  //TEST_SINGLE(umullb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.b, z29.b, z28.b");
  TEST_SINGLE(umullb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.h, z29.b, z28.b");
  TEST_SINGLE(umullb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.s, z29.h, z28.h");
  TEST_SINGLE(umullb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullb z30.d, z29.s, z28.s");

  //TEST_SINGLE(umullt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.b, z29.b, z28.b");
  TEST_SINGLE(umullt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.h, z29.b, z28.b");
  TEST_SINGLE(umullt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.s, z29.h, z28.h");
  TEST_SINGLE(umullt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29, ZReg::z28), "umullt z30.d, z29.s, z28.s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift left long") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract interleaved long") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise exclusive-or interleaved") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE integer matrix multiply accumulate") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise permute") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 complex integer add") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer absolute difference and accumulate long") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer add/subtract long with carry") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift right and accumulate") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 bitwise shift and insert") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 integer absolute difference and accumulate") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 saturating extract narrow") {
  TEST_SINGLE(sqxtnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.b, z29.h");
  TEST_SINGLE(sqxtnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.h, z29.s");
  TEST_SINGLE(sqxtnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.s, z29.d");
  //TEST_SINGLE(sqxtnb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtnb z30.d, z29.q");

  TEST_SINGLE(sqxtnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.b, z29.h");
  TEST_SINGLE(sqxtnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.h, z29.s");
  TEST_SINGLE(sqxtnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.s, z29.d");
  //TEST_SINGLE(sqxtnt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtnt z30.d, z29.q");

  TEST_SINGLE(uqxtnb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.b, z29.h");
  TEST_SINGLE(uqxtnb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.h, z29.s");
  TEST_SINGLE(uqxtnb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.s, z29.d");
  //TEST_SINGLE(uqxtnb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uqxtnb z30.d, z29.q");

  TEST_SINGLE(uqxtnt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.b, z29.h");
  TEST_SINGLE(uqxtnt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.h, z29.s");
  TEST_SINGLE(uqxtnt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.s, z29.d");
  //TEST_SINGLE(uqxtnt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "uqxtnt z30.d, z29.q");

  TEST_SINGLE(sqxtunb(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.b, z29.h");
  TEST_SINGLE(sqxtunb(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.h, z29.s");
  TEST_SINGLE(sqxtunb(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.s, z29.d");
  //TEST_SINGLE(sqxtunb(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtunb z30.d, z29.q");

  TEST_SINGLE(sqxtunt(SubRegSize::i8Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.b, z29.h");
  TEST_SINGLE(sqxtunt(SubRegSize::i16Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.h, z29.s");
  TEST_SINGLE(sqxtunt(SubRegSize::i32Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.s, z29.d");
  //TEST_SINGLE(sqxtunt(SubRegSize::i64Bit, ZReg::z30, ZReg::z29), "sqxtunt z30.d, z29.q");
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
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 Histogram Computation") {
  TEST_SINGLE(histcnt(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29, ZReg::z28),  "histcnt z30.s, p6/z, z29.s, z28.s");
  TEST_SINGLE(histcnt(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29, ZReg::z28),  "histcnt z30.d, p6/z, z29.d, z28.d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 Histogram Computation - Segment") {
  TEST_SINGLE(histseg(ZReg::z30, ZReg::z29, ZReg::z28), "histseg z30.b, z29.b, z28.b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto unary operations") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto destructive binary operations") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE2 crypto constructive binary operations") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE BFloat16 floating-point dot product (indexed)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add long (indexed)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE BFloat16 floating-point dot product") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point multiply-add long") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic (predicated)") {
  //TEST_SINGLE(fadd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fadd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fadd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fadd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fadd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fadd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fadd z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fsub(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fsub z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsub z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsub z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsub z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fsub(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsub z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmul(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmul z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmul z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmul z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmul z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmul(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmul z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fsubr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fsubr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fsubr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsubr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fsubr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsubr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fsubr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fsubr z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fsubr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fsubr z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmaxnm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmaxnm z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnm z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnm z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmaxnm z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmaxnm(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmaxnm z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fminnm(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fminnm z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fminnm(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnm z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnm z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fminnm(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fminnm z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fminnm(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fminnm z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmax(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmax z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmax z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmax(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmax z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmin(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmin z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmin z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmin(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmin z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fabd(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fabd z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fabd(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fabd z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fabd(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fabd z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fabd(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fabd z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fabd(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fabd z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fscale(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fscale z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fscale(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fscale z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fscale(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fscale z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fscale(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fscale z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fscale(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fscale z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fmulx(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fmulx z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fmulx(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmulx z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fmulx(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmulx z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fmulx(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fmulx z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fmulx(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fmulx z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fdiv(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fdiv z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fdiv(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdiv z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fdiv(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdiv z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fdiv(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdiv z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fdiv(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdiv z30.q, p6/m, z30.q, z28.q");

  //TEST_SINGLE(fdivr(SubRegSize::i8Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),   "fdivr z30.b, p6/m, z30.b, z28.b");
  TEST_SINGLE(fdivr(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdivr z30.h, p6/m, z30.h, z28.h");
  TEST_SINGLE(fdivr(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdivr z30.s, p6/m, z30.s, z28.s");
  TEST_SINGLE(fdivr(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28),  "fdivr z30.d, p6/m, z30.d, z28.d");
  //TEST_SINGLE(fdivr(SubRegSize::i128Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z30, ZReg::z28), "fdivr z30.q, p6/m, z30.q, z28.q");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point arithmetic with immediate (predicated)") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE Memory - 32-bit Gather and Unsized Contiguous") {
  TEST_SINGLE(ldr(PReg::p6, Reg::r29, 0), "ldr p6, [x29]");
  TEST_SINGLE(ldr(PReg::p6, Reg::r29, -256), "ldr p6, [x29, #-256, mul vl]");
  TEST_SINGLE(ldr(PReg::p6, Reg::r29, 255), "ldr p6, [x29, #255, mul vl]");
  // XXX: LDR (vector)
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE load multiple structures (scalar plus immediate)") {
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2b {z26.b, z27.b}, p6/z, [x29]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2b {z26.b, z27.b}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2b(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2b {z26.b, z27.b}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2h {z26.h, z27.h}, p6/z, [x29]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2h {z26.h, z27.h}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2h(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2h {z26.h, z27.h}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2w {z26.s, z27.s}, p6/z, [x29]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2w {z26.s, z27.s}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2w(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2w {z26.s, z27.s}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 0), "ld2d {z26.d, z27.d}, p6/z, [x29]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, -16), "ld2d {z26.d, z27.d}, p6/z, [x29, #-16, mul vl]");
  TEST_SINGLE(ld2d(ZReg::z26, ZReg::z27, PReg::p6.Zeroing(), Reg::r29, 14), "ld2d {z26.d, z27.d}, p6/z, [x29, #14, mul vl]");

  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29, #-24, mul vl]");
  TEST_SINGLE(ld3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3b {z26.b, z27.b, z28.b}, p6/z, [x29, #21, mul vl]");

  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29, #-24, mul vl]");
  TEST_SINGLE(ld3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3h {z26.h, z27.h, z28.h}, p6/z, [x29, #21, mul vl]");

  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29, #-24, mul vl]");
  TEST_SINGLE(ld3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3w {z26.s, z27.s, z28.s}, p6/z, [x29, #21, mul vl]");

  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 0), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, -24), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29, #-24, mul vl]");
  TEST_SINGLE(ld3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6.Zeroing(), Reg::r29, 21), "ld3d {z26.d, z27.d, z28.d}, p6/z, [x29, #21, mul vl]");

  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4b {z26.b, z27.b, z28.b, z29.b}, p6/z, [x29]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4b {z26.b, z27.b, z28.b, z29.b}, p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4b {z26.b, z27.b, z28.b, z29.b}, p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4h {z26.h, z27.h, z28.h, z29.h}, p6/z, [x29]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4h {z26.h, z27.h, z28.h, z29.h}, p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4h {z26.h, z27.h, z28.h, z29.h}, p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4w {z26.s, z27.s, z28.s, z29.s}, p6/z, [x29]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4w {z26.s, z27.s, z28.s, z29.s}, p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4w {z26.s, z27.s, z28.s, z29.s}, p6/z, [x29, #28, mul vl]");

  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 0), "ld4d {z26.d, z27.d, z28.d, z29.d}, p6/z, [x29]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, -32), "ld4d {z26.d, z27.d, z28.d, z29.d}, p6/z, [x29, #-32, mul vl]");
  TEST_SINGLE(ld4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6.Zeroing(), Reg::r29, 28), "ld4d {z26.d, z27.d, z28.d, z29.d}, p6/z, [x29, #28, mul vl]");
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

  //TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.b}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1h {z26.d}, p6/z, [x29]");

  //TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1h {z26.d}, p6/z, [x29, #-8, mul vl]");

  //TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.b}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1h {z26.d}, p6/z, [x29, #7, mul vl]");

  //TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.b}, p6/z, [x29]");
  //TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sh {z26.d}, p6/z, [x29]");

  //TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.b}, p6/z, [x29, #-8, mul vl]");
  //TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sh {z26.d}, p6/z, [x29, #-8, mul vl]");

  //TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.b}, p6/z, [x29, #7, mul vl]");
  //TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.h}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.s}, p6/z, [x29, #7, mul vl]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sh {z26.d}, p6/z, [x29, #7, mul vl]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sw {z26.d}, p6/z, [x29]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sw {z26.d}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sw {z26.d}, p6/z, [x29, #7, mul vl]");

  //TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.b}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.h}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.s}, p6/z, [x29]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 0), "ld1sb {z26.d}, p6/z, [x29]");

  //TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.b}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.h}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.s}, p6/z, [x29, #-8, mul vl]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, -8), "ld1sb {z26.d}, p6/z, [x29, #-8, mul vl]");

  //TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, 7), "ld1sb {z26.b}, p6/z, [x29, #7, mul vl]");
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

  //TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.b}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.h}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.s}, p6, [x29, x28, lsl #1]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1h {z26.d}, p6, [x29, x28, lsl #1]");

  //TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.b}, p6, [x29, x28, lsl #2]");
  //TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.h}, p6, [x29, x28, lsl #2]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.s}, p6, [x29, x28, lsl #2]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1w {z26.d}, p6, [x29, x28, lsl #2]");

  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, Reg::r28), "st1d {z26.d}, p6, [x29, x28, lsl #3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE contiguous load (scalar plus scalar)") {
  TEST_SINGLE(ld1b<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1b<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1b {z26.d}, p6/z, [x29, x30]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sw {z26.d}, p6/z, [x29, x30, lsl #2]");

  //TEST_SINGLE(ld1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.b}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1h {z26.d}, p6/z, [x29, x30, lsl #1]");

  //TEST_SINGLE(ld1sh<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.b}, p6/z, [x29, x30, lsl #1]");
  //TEST_SINGLE(ld1sh<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.h}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.s}, p6/z, [x29, x30, lsl #1]");
  TEST_SINGLE(ld1sh<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sh {z26.d}, p6/z, [x29, x30, lsl #1]");

  TEST_SINGLE(ld1sw(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sw {z26.d}, p6/z, [x29, x30, lsl #2]");

  //TEST_SINGLE(ld1sb<SubRegSize::i8Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.b}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i16Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.h}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i32Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.s}, p6/z, [x29, x30]");
  TEST_SINGLE(ld1sb<SubRegSize::i64Bit>(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1sb {z26.d}, p6/z, [x29, x30]");

  TEST_SINGLE(ld1d(ZReg::z26, PReg::p6.Zeroing(), Reg::r29, Reg::r30), "ld1d {z26.d}, p6/z, [x29, x30, lsl #3]");
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
  // TODO: Implement in emitter.
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
  // TODO: Implement in emitter.
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE floating-point convert to integer") {
  TEST_SINGLE(flogb(SubRegSize::i16Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.h, p6/m, z29.h");
  TEST_SINGLE(flogb(SubRegSize::i32Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.s, p6/m, z29.s");
  TEST_SINGLE(flogb(SubRegSize::i64Bit, ZReg::z30, PReg::p6.Merging(), ZReg::z29), "flogb z30.d, p6/m, z29.d");

  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.h, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.h, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.h, p6/m, z29.d");

  //TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.s, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.s, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.s, p6/m, z29.d");

  //TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "scvtf z30.d, p6/m, z29.h");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "scvtf z30.d, p6/m, z29.s");
  TEST_SINGLE(scvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "scvtf z30.d, p6/m, z29.d");

  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.h, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.h, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.h, p6/m, z29.d");

  //TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.s, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.s, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.s, p6/m, z29.d");

  //TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "ucvtf z30.d, p6/m, z29.h");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "ucvtf z30.d, p6/m, z29.s");
  TEST_SINGLE(ucvtf(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "ucvtf z30.d, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.h, p6/m, z29.h");
  //TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.h, p6/m, z29.s");
  //TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.h, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.s, p6/m, z29.s");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.s, p6/m, z29.d");

  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzs z30.d, p6/m, z29.h");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzs z30.d, p6/m, z29.s");
  TEST_SINGLE(fcvtzs(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzs z30.d, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.h, p6/m, z29.h");
  //TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.h, p6/m, z29.s");
  //TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i16Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.h, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.s, p6/m, z29.h");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.s, p6/m, z29.s");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i32Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.s, p6/m, z29.d");

  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i16Bit), "fcvtzu z30.d, p6/m, z29.h");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i32Bit), "fcvtzu z30.d, p6/m, z29.s");
  TEST_SINGLE(fcvtzu(ZReg::z30, SubRegSize::i64Bit, PReg::p6.Merging(), ZReg::z29, SubRegSize::i64Bit), "fcvtzu z30.d, p6/m, z29.d");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: SVE: SVE store multiple structures (scalar plus immediate)") {
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2b {z26.b, z27.b}, p6, [x29]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2b {z26.b, z27.b}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2b(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2b {z26.b, z27.b}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2h {z26.h, z27.h}, p6, [x29]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2h {z26.h, z27.h}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2h(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2h {z26.h, z27.h}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2w {z26.s, z27.s}, p6, [x29]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2w {z26.s, z27.s}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2w(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2w {z26.s, z27.s}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 0), "st2d {z26.d, z27.d}, p6, [x29]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, -16), "st2d {z26.d, z27.d}, p6, [x29, #-16, mul vl]");
  TEST_SINGLE(st2d(ZReg::z26, ZReg::z27, PReg::p6, Reg::r29, 14), "st2d {z26.d, z27.d}, p6, [x29, #14, mul vl]");

  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3b {z26.b, z27.b, z28.b}, p6, [x29]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3b {z26.b, z27.b, z28.b}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3b(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3b {z26.b, z27.b, z28.b}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3h {z26.h, z27.h, z28.h}, p6, [x29]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3h {z26.h, z27.h, z28.h}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3h(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3h {z26.h, z27.h, z28.h}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3w {z26.s, z27.s, z28.s}, p6, [x29]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3w {z26.s, z27.s, z28.s}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3w(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3w {z26.s, z27.s, z28.s}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 0), "st3d {z26.d, z27.d, z28.d}, p6, [x29]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, -24), "st3d {z26.d, z27.d, z28.d}, p6, [x29, #-24, mul vl]");
  TEST_SINGLE(st3d(ZReg::z26, ZReg::z27, ZReg::z28, PReg::p6, Reg::r29, 21), "st3d {z26.d, z27.d, z28.d}, p6, [x29, #21, mul vl]");

  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29, #-32, mul vl]");
  TEST_SINGLE(st4b(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4b {z26.b, z27.b, z28.b, z29.b}, p6, [x29, #28, mul vl]");

  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29, #-32, mul vl]");
  TEST_SINGLE(st4h(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4h {z26.h, z27.h, z28.h, z29.h}, p6, [x29, #28, mul vl]");

  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29, #-32, mul vl]");
  TEST_SINGLE(st4w(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4w {z26.s, z27.s, z28.s, z29.s}, p6, [x29, #28, mul vl]");

  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 0), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, -32), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29, #-32, mul vl]");
  TEST_SINGLE(st4d(ZReg::z26, ZReg::z27, ZReg::z28, ZReg::z29, PReg::p6, Reg::r29, 28), "st4d {z26.d, z27.d, z28.d, z29.d}, p6, [x29, #28, mul vl]");
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

  //TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.b}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.h}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.s}, p6, [x29]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1h {z26.d}, p6, [x29]");

  //TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.b}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1h {z26.d}, p6, [x29, #-8, mul vl]");

  //TEST_SINGLE(st1h<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.b}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.h}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.s}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1h<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1h {z26.d}, p6, [x29, #7, mul vl]");

  //TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.b}, p6, [x29]");
  //TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.h}, p6, [x29]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.s}, p6, [x29]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 0), "st1w {z26.d}, p6, [x29]");

  //TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.b}, p6, [x29, #-8, mul vl]");
  //TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.h}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.s}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, -8), "st1w {z26.d}, p6, [x29, #-8, mul vl]");

  //TEST_SINGLE(st1w<SubRegSize::i8Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.b}, p6, [x29, #7, mul vl]");
  //TEST_SINGLE(st1w<SubRegSize::i16Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.h}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i32Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.s}, p6, [x29, #7, mul vl]");
  TEST_SINGLE(st1w<SubRegSize::i64Bit>(ZReg::z26, PReg::p6, Reg::r29, 7), "st1w {z26.d}, p6, [x29, #7, mul vl]");

  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, 0), "st1d {z26.d}, p6, [x29]");
  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, -8), "st1d {z26.d}, p6, [x29, #-8, mul vl]");
  TEST_SINGLE(st1d(ZReg::z26, PReg::p6, Reg::r29, 7), "st1d {z26.d}, p6, [x29, #7, mul vl]");
}
