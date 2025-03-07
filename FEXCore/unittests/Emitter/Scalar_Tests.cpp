// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar copy") {
  TEST_SINGLE(dup(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 0), "mov b30, v29.b[0]");
  TEST_SINGLE(dup(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 15), "mov b30, v29.b[15]");
  TEST_SINGLE(mov(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 0), "mov b30, v29.b[0]");
  TEST_SINGLE(mov(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 15), "mov b30, v29.b[15]");

  TEST_SINGLE(dup(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 0), "mov h30, v29.h[0]");
  TEST_SINGLE(dup(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 7), "mov h30, v29.h[7]");
  TEST_SINGLE(mov(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 0), "mov h30, v29.h[0]");
  TEST_SINGLE(mov(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 7), "mov h30, v29.h[7]");

  TEST_SINGLE(dup(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 0), "mov s30, v29.s[0]");
  TEST_SINGLE(dup(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 3), "mov s30, v29.s[3]");
  TEST_SINGLE(mov(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 0), "mov s30, v29.s[0]");
  TEST_SINGLE(mov(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 3), "mov s30, v29.s[3]");

  TEST_SINGLE(dup(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 0), "mov d30, v29.d[0]");
  TEST_SINGLE(dup(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "mov d30, v29.d[1]");
  TEST_SINGLE(mov(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 0), "mov d30, v29.d[0]");
  TEST_SINGLE(mov(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "mov d30, v29.d[1]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar three same FP16") {
  TEST_SINGLE(fmulx(HReg::h30, HReg::h29, HReg::h28), "fmulx h30, h29, h28");
  TEST_SINGLE(fcmeq(HReg::h30, HReg::h29, HReg::h28), "fcmeq h30, h29, h28");
  TEST_SINGLE(frecps(HReg::h30, HReg::h29, HReg::h28), "frecps h30, h29, h28");
  TEST_SINGLE(frsqrts(HReg::h30, HReg::h29, HReg::h28), "frsqrts h30, h29, h28");
  TEST_SINGLE(fcmge(HReg::h30, HReg::h29, HReg::h28), "fcmge h30, h29, h28");
  TEST_SINGLE(facge(HReg::h30, HReg::h29, HReg::h28), "facge h30, h29, h28");
  TEST_SINGLE(fabd(HReg::h30, HReg::h29, HReg::h28), "fabd h30, h29, h28");
  TEST_SINGLE(fcmgt(HReg::h30, HReg::h29, HReg::h28), "fcmgt h30, h29, h28");
  TEST_SINGLE(facgt(HReg::h30, HReg::h29, HReg::h28), "facgt h30, h29, h28");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar two-register miscellaneous FP16") {
  TEST_SINGLE(fcvtns(HReg::h30, HReg::h29), "fcvtns h30, h29");
  TEST_SINGLE(fcvtms(HReg::h30, HReg::h29), "fcvtms h30, h29");
  TEST_SINGLE(fcvtas(HReg::h30, HReg::h29), "fcvtas h30, h29");
  TEST_SINGLE(scvtf(HReg::h30, HReg::h29), "scvtf h30, h29");
  TEST_SINGLE(fcmgt(HReg::h30, HReg::h29), "fcmgt h30, h29, #0.0");
  TEST_SINGLE(fcmeq(HReg::h30, HReg::h29), "fcmeq h30, h29, #0.0");
  TEST_SINGLE(fcmlt(HReg::h30, HReg::h29), "fcmlt h30, h29, #0.0");
  TEST_SINGLE(fcvtps(HReg::h30, HReg::h29), "fcvtps h30, h29");
  TEST_SINGLE(fcvtzs(HReg::h30, HReg::h29), "fcvtzs h30, h29");
  TEST_SINGLE(frecpe(HReg::h30, HReg::h29), "frecpe h30, h29");
  TEST_SINGLE(frecpx(HReg::h30, HReg::h29), "frecpx h30, h29");
  TEST_SINGLE(fcvtnu(HReg::h30, HReg::h29), "fcvtnu h30, h29");
  TEST_SINGLE(fcvtmu(HReg::h30, HReg::h29), "fcvtmu h30, h29");
  TEST_SINGLE(fcvtau(HReg::h30, HReg::h29), "fcvtau h30, h29");
  TEST_SINGLE(ucvtf(HReg::h30, HReg::h29), "ucvtf h30, h29");
  TEST_SINGLE(fcmge(HReg::h30, HReg::h29), "fcmge h30, h29, #0.0");
  TEST_SINGLE(fcmle(HReg::h30, HReg::h29), "fcmle h30, h29, #0.0");
  TEST_SINGLE(fcvtpu(HReg::h30, HReg::h29), "fcvtpu h30, h29");
  TEST_SINGLE(fcvtzu(HReg::h30, HReg::h29), "fcvtzu h30, h29");
  TEST_SINGLE(frsqrte(HReg::h30, HReg::h29), "frsqrte h30, h29");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar three same extra") {
  // TODO: Implement in emitter
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar two-register miscellaneous") {
  // Commented out lines showcase unallocated encodings.
  TEST_SINGLE(suqadd(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "suqadd b30, b29");
  TEST_SINGLE(suqadd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "suqadd h30, h29");
  TEST_SINGLE(suqadd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "suqadd s30, s29");
  TEST_SINGLE(suqadd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "suqadd d30, d29");

  TEST_SINGLE(sqabs(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "sqabs b30, b29");
  TEST_SINGLE(sqabs(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "sqabs h30, h29");
  TEST_SINGLE(sqabs(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "sqabs s30, s29");
  TEST_SINGLE(sqabs(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "sqabs d30, d29");

  // TEST_SINGLE(cmgt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "cmgt b30, b29, #0");
  // TEST_SINGLE(cmgt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "cmgt h30, h29, #0");
  // TEST_SINGLE(cmgt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "cmgt s30, s29, #0");
  TEST_SINGLE(cmgt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "cmgt d30, d29, #0");

  // TEST_SINGLE(cmeq(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "cmeq b30, b29, #0");
  // TEST_SINGLE(cmeq(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "cmeq h30, h29, #0");
  // TEST_SINGLE(cmeq(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "cmeq s30, s29, #0");
  TEST_SINGLE(cmeq(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "cmeq d30, d29, #0");

  // TEST_SINGLE(cmlt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "cmlt b30, b29, #0");
  // TEST_SINGLE(cmlt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "cmlt h30, h29, #0");
  // TEST_SINGLE(cmlt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "cmlt s30, s29, #0");
  TEST_SINGLE(cmlt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "cmlt d30, d29, #0");

  // TEST_SINGLE(abs(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "abs b30, b29");
  // TEST_SINGLE(abs(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "abs h30, h29");
  // TEST_SINGLE(abs(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "abs s30, s29");
  TEST_SINGLE(abs(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "abs d30, d29");

  TEST_SINGLE(sqxtn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "sqxtn b30, h29");
  TEST_SINGLE(sqxtn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "sqxtn h30, s29");
  TEST_SINGLE(sqxtn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "sqxtn s30, d29");
  // TEST_SINGLE(sqxtn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "sqxtn d30, d29");

  // TEST_SINGLE(fcvtns(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtns b30, b29");
  // TEST_SINGLE(fcvtns(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtns h30, h29");
  TEST_SINGLE(fcvtns(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtns s30, s29");
  TEST_SINGLE(fcvtns(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtns d30, d29");

  // TEST_SINGLE(fcvtms(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtms b30, b29");
  // TEST_SINGLE(fcvtms(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtms h30, h29");
  TEST_SINGLE(fcvtms(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtms s30, s29");
  TEST_SINGLE(fcvtms(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtms d30, d29");

  // TEST_SINGLE(fcvtas(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtas b30, b29");
  // TEST_SINGLE(fcvtas(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtas h30, h29");
  TEST_SINGLE(fcvtas(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtas s30, s29");
  TEST_SINGLE(fcvtas(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtas d30, d29");

  // TEST_SINGLE(scvtf(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "scvtf b30, b29");
  // TEST_SINGLE(scvtf(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "scvtf h30, h29");
  TEST_SINGLE(scvtf(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "scvtf s30, s29");
  TEST_SINGLE(scvtf(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "scvtf d30, d29");

  // TEST_SINGLE(fcmeq(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcmeq b30, b29");
  // TEST_SINGLE(fcmeq(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcmeq h30, h29");
  TEST_SINGLE(fcmeq(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcmeq s30, s29, #0.0");
  TEST_SINGLE(fcmeq(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcmeq d30, d29, #0.0");

  // TEST_SINGLE(fcmlt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcmlt b30, b29");
  // TEST_SINGLE(fcmlt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcmlt h30, h29");
  TEST_SINGLE(fcmlt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcmlt s30, s29, #0.0");
  TEST_SINGLE(fcmlt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcmlt d30, d29, #0.0");

  // TEST_SINGLE(fcvtps(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtps b30, b29");
  // TEST_SINGLE(fcvtps(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtps h30, h29");
  TEST_SINGLE(fcvtps(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtps s30, s29");
  TEST_SINGLE(fcvtps(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtps d30, d29");

  // TEST_SINGLE(fcvtzs(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtzs b30, b29");
  // TEST_SINGLE(fcvtzs(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtzs h30, h29");
  TEST_SINGLE(fcvtzs(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtzs s30, s29");
  TEST_SINGLE(fcvtzs(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtzs d30, d29");

  // TEST_SINGLE(frecpe(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "frecpe b30, b29");
  // TEST_SINGLE(frecpe(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frecpe h30, h29");
  TEST_SINGLE(frecpe(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frecpe s30, s29");
  TEST_SINGLE(frecpe(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frecpe d30, d29");

  // TEST_SINGLE(frecpx(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "frecpx b30, b29");
  // TEST_SINGLE(frecpx(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frecpx h30, h29");
  TEST_SINGLE(frecpx(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frecpx s30, s29");
  TEST_SINGLE(frecpx(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frecpx d30, d29");

  TEST_SINGLE(usqadd(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "usqadd b30, b29");
  TEST_SINGLE(usqadd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "usqadd h30, h29");
  TEST_SINGLE(usqadd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "usqadd s30, s29");
  TEST_SINGLE(usqadd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "usqadd d30, d29");

  TEST_SINGLE(sqneg(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "sqneg b30, b29");
  TEST_SINGLE(sqneg(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "sqneg h30, h29");
  TEST_SINGLE(sqneg(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "sqneg s30, s29");
  TEST_SINGLE(sqneg(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "sqneg d30, d29");

  // TEST_SINGLE(cmge(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "cmge b30, b29");
  // TEST_SINGLE(cmge(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "cmge h30, h29");
  // TEST_SINGLE(cmge(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "cmge s30, s29");
  TEST_SINGLE(cmge(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "cmge d30, d29, #0");

  // TEST_SINGLE(cmle(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "cmle b30, b29");
  // TEST_SINGLE(cmle(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "cmle h30, h29");
  // TEST_SINGLE(cmle(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "cmle s30, s29");
  TEST_SINGLE(cmle(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "cmle d30, d29, #0");

  // TEST_SINGLE(neg(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "neg b30, b29");
  // TEST_SINGLE(neg(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "neg h30, h29");
  // TEST_SINGLE(neg(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "neg s30, s29");
  TEST_SINGLE(neg(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "neg d30, d29");

  TEST_SINGLE(sqxtun(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "sqxtun b30, h29");
  TEST_SINGLE(sqxtun(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "sqxtun h30, s29");
  TEST_SINGLE(sqxtun(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "sqxtun s30, d29");
  // TEST_SINGLE(sqxtun(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "sqxtun d30, d29");

  TEST_SINGLE(uqxtn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "uqxtn b30, h29");
  TEST_SINGLE(uqxtn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "uqxtn h30, s29");
  TEST_SINGLE(uqxtn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "uqxtn s30, d29");
  // TEST_SINGLE(uqxtn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "uqxtn d30, d29");

  // TEST_SINGLE(fcvtxn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtxn b30, b29");
  // TEST_SINGLE(fcvtxn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtxn h30, h29");
  TEST_SINGLE(fcvtxn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtxn s30, d29");
  // TEST_SINGLE(fcvtxn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtxn d30, d29");

  // TEST_SINGLE(fcvtnu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtnu b30, b29");
  // TEST_SINGLE(fcvtnu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtnu h30, h29");
  TEST_SINGLE(fcvtnu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtnu s30, s29");
  TEST_SINGLE(fcvtnu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtnu d30, d29");

  // TEST_SINGLE(fcvtmu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtmu b30, b29");
  // TEST_SINGLE(fcvtmu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtmu h30, h29");
  TEST_SINGLE(fcvtmu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtmu s30, s29");
  TEST_SINGLE(fcvtmu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtmu d30, d29");

  // TEST_SINGLE(fcvtau(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtau b30, b29");
  // TEST_SINGLE(fcvtau(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtau h30, h29");
  TEST_SINGLE(fcvtau(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtau s30, s29");
  TEST_SINGLE(fcvtau(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtau d30, d29");

  // TEST_SINGLE(ucvtf(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "ucvtf b30, b29");
  // TEST_SINGLE(ucvtf(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "ucvtf h30, h29");
  TEST_SINGLE(ucvtf(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "ucvtf s30, s29");
  TEST_SINGLE(ucvtf(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "ucvtf d30, d29");

  // TEST_SINGLE(fcmge(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcmge b30, b29");
  // TEST_SINGLE(fcmge(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcmge h30, h29");
  TEST_SINGLE(fcmge(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcmge s30, s29, #0.0");
  TEST_SINGLE(fcmge(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcmge d30, d29, #0.0");

  // TEST_SINGLE(fcmle(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcmle b30, b29");
  // TEST_SINGLE(fcmle(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcmle h30, h29");
  TEST_SINGLE(fcmle(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcmle s30, s29, #0.0");
  TEST_SINGLE(fcmle(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcmle d30, d29, #0.0");

  // TEST_SINGLE(fcvtpu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtpu b30, b29");
  // TEST_SINGLE(fcvtpu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtpu h30, h29");
  TEST_SINGLE(fcvtpu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtpu s30, s29");
  TEST_SINGLE(fcvtpu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtpu d30, d29");

  // TEST_SINGLE(fcvtzu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fcvtzu b30, b29");
  // TEST_SINGLE(fcvtzu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcvtzu h30, h29");
  TEST_SINGLE(fcvtzu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcvtzu s30, s29");
  TEST_SINGLE(fcvtzu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcvtzu d30, d29");

  // TEST_SINGLE(frsqrte(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "frsqrte b30, b29");
  // TEST_SINGLE(frsqrte(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frsqrte h30, h29");
  TEST_SINGLE(frsqrte(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frsqrte s30, s29");
  TEST_SINGLE(frsqrte(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frsqrte d30, d29");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar pairwise") {
  // Commented out lines showcase unallocated encodings.
  // TEST_SINGLE(addp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "addp b30, b29");
  // TEST_SINGLE(addp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "addp h30, h29");
  // TEST_SINGLE(addp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "addp s30, s29");
  TEST_SINGLE(addp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "addp d30, v29.2d");

  TEST_SINGLE(fmaxnmp(HReg::h30, HReg::h29), "fmaxnmp h30, v29.2h");
  // TEST_SINGLE(fmaxnmp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fmaxnmp b30, b29");
  // TEST_SINGLE(fmaxnmp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fmaxnmp h30, h29");
  TEST_SINGLE(fmaxnmp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fmaxnmp s30, v29.2s");
  TEST_SINGLE(fmaxnmp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fmaxnmp d30, v29.2d");

  TEST_SINGLE(faddp(HReg::h30, HReg::h29), "faddp h30, v29.2h");
  // TEST_SINGLE(faddp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "faddp b30, b29");
  // TEST_SINGLE(faddp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "faddp h30, h29");
  TEST_SINGLE(faddp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "faddp s30, v29.2s");
  TEST_SINGLE(faddp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "faddp d30, v29.2d");

  TEST_SINGLE(fmaxp(HReg::h30, HReg::h29), "fmaxp h30, v29.2h");
  // TEST_SINGLE(fmaxp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fmaxp b30, b29");
  // TEST_SINGLE(fmaxp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fmaxp h30, h29");
  TEST_SINGLE(fmaxp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fmaxp s30, v29.2s");
  TEST_SINGLE(fmaxp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fmaxp d30, v29.2d");

  TEST_SINGLE(fminnmp(HReg::h30, HReg::h29), "fminnmp h30, v29.2h");
  // TEST_SINGLE(fminnmp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fminnmp b30, b29");
  // TEST_SINGLE(fminnmp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fminnmp h30, h29");
  TEST_SINGLE(fminnmp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fminnmp s30, v29.2s");
  TEST_SINGLE(fminnmp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fminnmp d30, v29.2d");

  TEST_SINGLE(fminp(HReg::h30, HReg::h29), "fminp h30, v29.2h");
  // TEST_SINGLE(fminp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29),  "fminp b30, b29");
  // TEST_SINGLE(fminp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fminp h30, h29");
  TEST_SINGLE(fminp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fminp s30, v29.2s");
  TEST_SINGLE(fminp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fminp d30, v29.2d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar three different") {
  // Commented out lines showcase unallocated encodings.
  // TEST_SINGLE(sqdmlal(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28),  "sqdmlal v30.16b, v29.16b, v28.v16b");
  // TEST_SINGLE(sqdmlal(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlal v30.16b, v29.16b, v28.v16b");
  TEST_SINGLE(sqdmlal(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlal s30, h29, h28");
  TEST_SINGLE(sqdmlal(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlal d30, s29, s28");

  // TEST_SINGLE(sqdmlsl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28),  "sqdmlsl v30.16b, v29.16b, v28.v16b");
  // TEST_SINGLE(sqdmlsl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlsl v30.16b, v29.16b, v28.v16b");
  TEST_SINGLE(sqdmlsl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlsl s30, h29, h28");
  TEST_SINGLE(sqdmlsl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmlsl d30, s29, s28");

  // TEST_SINGLE(sqdmull(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28),  "sqdmull v30.16b, v29.16b, v28.v16b");
  // TEST_SINGLE(sqdmull(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmull v30.16b, v29.16b, v28.v16b");
  TEST_SINGLE(sqdmull(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmull s30, h29, h28");
  TEST_SINGLE(sqdmull(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmull d30, s29, s28");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar three same") {
  TEST_SINGLE(sqadd(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqadd b30, b29, b28");
  TEST_SINGLE(sqadd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqadd h30, h29, h28");
  TEST_SINGLE(sqadd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqadd s30, s29, s28");
  TEST_SINGLE(sqadd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqadd d30, d29, d28");

  TEST_SINGLE(sqsub(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqsub b30, b29, b28");
  TEST_SINGLE(sqsub(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqsub h30, h29, h28");
  TEST_SINGLE(sqsub(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqsub s30, s29, s28");
  TEST_SINGLE(sqsub(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqsub d30, d29, d28");

  // TEST_SINGLE(cmgt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmgt b30, b29, b28");
  // TEST_SINGLE(cmgt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmgt h30, h29, h28");
  // TEST_SINGLE(cmgt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmgt s30, s29, s28");
  TEST_SINGLE(cmgt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmgt d30, d29, d28");

  // TEST_SINGLE(cmge(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmge b30, b29, b28");
  // TEST_SINGLE(cmge(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmge h30, h29, h28");
  // TEST_SINGLE(cmge(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmge s30, s29, s28");
  TEST_SINGLE(cmge(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmge d30, d29, d28");

  // TEST_SINGLE(sshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sshl b30, b29, b28");
  // TEST_SINGLE(sshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sshl h30, h29, h28");
  // TEST_SINGLE(sshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sshl s30, s29, s28");
  TEST_SINGLE(sshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sshl d30, d29, d28");

  TEST_SINGLE(sqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqshl b30, b29, b28");
  TEST_SINGLE(sqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqshl h30, h29, h28");
  TEST_SINGLE(sqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqshl s30, s29, s28");
  TEST_SINGLE(sqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqshl d30, d29, d28");

  // TEST_SINGLE(srshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "srshl b30, b29, b28");
  // TEST_SINGLE(srshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "srshl h30, h29, h28");
  // TEST_SINGLE(srshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "srshl s30, s29, s28");
  TEST_SINGLE(srshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "srshl d30, d29, d28");

  TEST_SINGLE(sqrshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqrshl b30, b29, b28");
  TEST_SINGLE(sqrshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqrshl h30, h29, h28");
  TEST_SINGLE(sqrshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqrshl s30, s29, s28");
  TEST_SINGLE(sqrshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqrshl d30, d29, d28");

  // TEST_SINGLE(add(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "add b30, b29, b28");
  // TEST_SINGLE(add(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "add h30, h29, h28");
  // TEST_SINGLE(add(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "add s30, s29, s28");
  TEST_SINGLE(add(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "add d30, d29, d28");

  // TEST_SINGLE(cmtst(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmtst b30, b29, b28");
  // TEST_SINGLE(cmtst(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmtst h30, h29, h28");
  // TEST_SINGLE(cmtst(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmtst s30, s29, s28");
  TEST_SINGLE(cmtst(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmtst d30, d29, d28");

  // TEST_SINGLE(sqdmulh(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmulh b30, b29, b28");
  TEST_SINGLE(sqdmulh(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmulh h30, h29, h28");
  TEST_SINGLE(sqdmulh(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmulh s30, s29, s28");
  // TEST_SINGLE(sqdmulh(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqdmulh d30, d29, d28");

  // TEST_SINGLE(fmulx(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "fmulx b30, b29, b28");
  // TEST_SINGLE(fmulx(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fmulx h30, h29, h28");
  TEST_SINGLE(fmulx(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fmulx s30, s29, s28");
  TEST_SINGLE(fmulx(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fmulx d30, d29, d28");

  // TEST_SINGLE(fcmeq(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "fcmeq b30, b29, b28");
  // TEST_SINGLE(fcmeq(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fcmeq h30, h29, h28");
  TEST_SINGLE(fcmeq(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fcmeq s30, s29, s28");
  TEST_SINGLE(fcmeq(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fcmeq d30, d29, d28");

  // TEST_SINGLE(frecps(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "frecps b30, b29, b28");
  // TEST_SINGLE(frecps(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "frecps h30, h29, h28");
  TEST_SINGLE(frecps(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "frecps s30, s29, s28");
  TEST_SINGLE(frecps(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "frecps d30, d29, d28");

  // TEST_SINGLE(frsqrts(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "frsqrts b30, b29, b28");
  // TEST_SINGLE(frsqrts(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "frsqrts h30, h29, h28");
  TEST_SINGLE(frsqrts(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "frsqrts s30, s29, s28");
  TEST_SINGLE(frsqrts(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "frsqrts d30, d29, d28");

  TEST_SINGLE(uqadd(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "uqadd b30, b29, b28");
  TEST_SINGLE(uqadd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "uqadd h30, h29, h28");
  TEST_SINGLE(uqadd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "uqadd s30, s29, s28");
  TEST_SINGLE(uqadd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "uqadd d30, d29, d28");

  TEST_SINGLE(uqsub(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "uqsub b30, b29, b28");
  TEST_SINGLE(uqsub(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "uqsub h30, h29, h28");
  TEST_SINGLE(uqsub(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "uqsub s30, s29, s28");
  TEST_SINGLE(uqsub(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "uqsub d30, d29, d28");

  // TEST_SINGLE(cmhi(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmhi b30, b29, b28");
  // TEST_SINGLE(cmhi(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmhi h30, h29, h28");
  // TEST_SINGLE(cmhi(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmhi s30, s29, s28");
  TEST_SINGLE(cmhi(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmhi d30, d29, d28");

  // TEST_SINGLE(cmhs(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmhs b30, b29, b28");
  // TEST_SINGLE(cmhs(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmhs h30, h29, h28");
  // TEST_SINGLE(cmhs(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmhs s30, s29, s28");
  TEST_SINGLE(cmhs(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmhs d30, d29, d28");

  // TEST_SINGLE(ushl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "ushl b30, b29, b28");
  // TEST_SINGLE(ushl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "ushl h30, h29, h28");
  // TEST_SINGLE(ushl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "ushl s30, s29, s28");
  TEST_SINGLE(ushl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "ushl d30, d29, d28");

  TEST_SINGLE(uqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "uqshl b30, b29, b28");
  TEST_SINGLE(uqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "uqshl h30, h29, h28");
  TEST_SINGLE(uqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "uqshl s30, s29, s28");
  TEST_SINGLE(uqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "uqshl d30, d29, d28");

  // TEST_SINGLE(urshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "urshl b30, b29, b28");
  // TEST_SINGLE(urshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "urshl h30, h29, h28");
  // TEST_SINGLE(urshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "urshl s30, s29, s28");
  TEST_SINGLE(urshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "urshl d30, d29, d28");

  TEST_SINGLE(uqrshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "uqrshl b30, b29, b28");
  TEST_SINGLE(uqrshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "uqrshl h30, h29, h28");
  TEST_SINGLE(uqrshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "uqrshl s30, s29, s28");
  TEST_SINGLE(uqrshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "uqrshl d30, d29, d28");

  // TEST_SINGLE(sub(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sub b30, b29, b28");
  // TEST_SINGLE(sub(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sub h30, h29, h28");
  // TEST_SINGLE(sub(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sub s30, s29, s28");
  TEST_SINGLE(sub(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sub d30, d29, d28");

  // TEST_SINGLE(cmeq(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "cmeq b30, b29, b28");
  // TEST_SINGLE(cmeq(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "cmeq h30, h29, h28");
  // TEST_SINGLE(cmeq(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "cmeq s30, s29, s28");
  TEST_SINGLE(cmeq(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "cmeq d30, d29, d28");

  // TEST_SINGLE(sqrdmulh(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "sqrdmulh b30, b29, b28");
  TEST_SINGLE(sqrdmulh(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "sqrdmulh h30, h29, h28");
  TEST_SINGLE(sqrdmulh(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "sqrdmulh s30, s29, s28");
  // TEST_SINGLE(sqrdmulh(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "sqrdmulh d30, d29, d28");

  // TEST_SINGLE(fcmge(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "fcmge b30, b29, b28");
  // TEST_SINGLE(fcmge(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fcmge h30, h29, h28");
  TEST_SINGLE(fcmge(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fcmge s30, s29, s28");
  TEST_SINGLE(fcmge(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fcmge d30, d29, d28");

  // TEST_SINGLE(facge(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "facge b30, b29, b28");
  // TEST_SINGLE(facge(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "facge h30, h29, h28");
  TEST_SINGLE(facge(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "facge s30, s29, s28");
  TEST_SINGLE(facge(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "facge d30, d29, d28");

  // TEST_SINGLE(fabd(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "fabd b30, b29, b28");
  // TEST_SINGLE(fabd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fabd h30, h29, h28");
  TEST_SINGLE(fabd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fabd s30, s29, s28");
  TEST_SINGLE(fabd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fabd d30, d29, d28");

  // TEST_SINGLE(fcmgt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "fcmgt b30, b29, b28");
  // TEST_SINGLE(fcmgt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fcmgt h30, h29, h28");
  TEST_SINGLE(fcmgt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fcmgt s30, s29, s28");
  TEST_SINGLE(fcmgt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fcmgt d30, d29, d28");

  // TEST_SINGLE(facgt(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, VReg::v28), "facgt b30, b29, b28");
  // TEST_SINGLE(facgt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "facgt h30, h29, h28");
  TEST_SINGLE(facgt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "facgt s30, s29, s28");
  TEST_SINGLE(facgt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "facgt d30, d29, d28");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar shift by immediate") {
  // TODO: Implement `UCVTF, FCVTZU' in emitter
  // TEST_SINGLE(sshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "sshr b30, b29, #1");
  // TEST_SINGLE(sshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "sshr b30, b29, #7");
  // TEST_SINGLE(sshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "sshr h30, h29, #1");
  // TEST_SINGLE(sshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sshr h30, h29, #15");
  // TEST_SINGLE(sshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "sshr s30, s29, #1");
  // TEST_SINGLE(sshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sshr s30, s29, #31");
  TEST_SINGLE(sshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "sshr d30, d29, #1");
  TEST_SINGLE(sshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sshr d30, d29, #63");

  // TEST_SINGLE(ssra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "ssra b30, b29, #1");
  // TEST_SINGLE(ssra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "ssra b30, b29, #7");
  // TEST_SINGLE(ssra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "ssra h30, h29, #1");
  // TEST_SINGLE(ssra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "ssra h30, h29, #15");
  // TEST_SINGLE(ssra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "ssra s30, s29, #1");
  // TEST_SINGLE(ssra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "ssra s30, s29, #31");
  TEST_SINGLE(ssra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "ssra d30, d29, #1");
  TEST_SINGLE(ssra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "ssra d30, d29, #63");

  // TEST_SINGLE(srshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "srshr b30, b29, #1");
  // TEST_SINGLE(srshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "srshr b30, b29, #7");
  // TEST_SINGLE(srshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "srshr h30, h29, #1");
  // TEST_SINGLE(srshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "srshr h30, h29, #15");
  // TEST_SINGLE(srshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "srshr s30, s29, #1");
  // TEST_SINGLE(srshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "srshr s30, s29, #31");
  TEST_SINGLE(srshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "srshr d30, d29, #1");
  TEST_SINGLE(srshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "srshr d30, d29, #63");

  // TEST_SINGLE(srsra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "srsra b30, b29, #1");
  // TEST_SINGLE(srsra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "srsra b30, b29, #7");
  // TEST_SINGLE(srsra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "srsra h30, h29, #1");
  // TEST_SINGLE(srsra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "srsra h30, h29, #15");
  // TEST_SINGLE(srsra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "srsra s30, s29, #1");
  // TEST_SINGLE(srsra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "srsra s30, s29, #31");
  TEST_SINGLE(srsra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "srsra d30, d29, #1");
  TEST_SINGLE(srsra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "srsra d30, d29, #63");

  // TEST_SINGLE(shl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "shl b30, b29, #1");
  // TEST_SINGLE(shl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "shl b30, b29, #7");
  // TEST_SINGLE(shl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "shl h30, h29, #1");
  // TEST_SINGLE(shl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "shl h30, h29, #15");
  // TEST_SINGLE(shl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "shl s30, s29, #1");
  // TEST_SINGLE(shl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "shl s30, s29, #31");
  TEST_SINGLE(shl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "shl d30, d29, #1");
  TEST_SINGLE(shl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "shl d30, d29, #63");

  TEST_SINGLE(sqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqshl b30, b29, #1");
  TEST_SINGLE(sqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqshl b30, b29, #7");
  TEST_SINGLE(sqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqshl h30, h29, #1");
  TEST_SINGLE(sqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqshl h30, h29, #15");
  TEST_SINGLE(sqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqshl s30, s29, #1");
  TEST_SINGLE(sqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqshl s30, s29, #31");
  TEST_SINGLE(sqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "sqshl d30, d29, #1");
  TEST_SINGLE(sqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqshl d30, d29, #63");

  TEST_SINGLE(sqshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqshrn b30, h29, #1");
  TEST_SINGLE(sqshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqshrn b30, h29, #7");
  TEST_SINGLE(sqshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqshrn h30, s29, #1");
  TEST_SINGLE(sqshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqshrn h30, s29, #15");
  TEST_SINGLE(sqshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqshrn s30, d29, #1");
  TEST_SINGLE(sqshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqshrn s30, d29, #31");
  // TEST_SINGLE(sqshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "sqshrn d30, d29, #1");
  // TEST_SINGLE(sqshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqshrn d30, d29, #63");

  TEST_SINGLE(sqrshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqrshrn b30, h29, #1");
  TEST_SINGLE(sqrshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqrshrn b30, h29, #7");
  TEST_SINGLE(sqrshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqrshrn h30, s29, #1");
  TEST_SINGLE(sqrshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqrshrn h30, s29, #15");
  TEST_SINGLE(sqrshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqrshrn s30, d29, #1");
  TEST_SINGLE(sqrshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqrshrn s30, d29, #31");
  // TEST_SINGLE(sqrshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "sqrshrn d30, d29, #1");
  // TEST_SINGLE(sqrshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqrshrn d30, d29, #63");

  // TODO: Implement `SCVTF, FCVTZS` in emitter
  // TEST_SINGLE(ushr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "ushr b30, b29, #1");
  // TEST_SINGLE(ushr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "ushr b30, b29, #7");
  // TEST_SINGLE(ushr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "ushr h30, h29, #1");
  // TEST_SINGLE(ushr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "ushr h30, h29, #15");
  // TEST_SINGLE(ushr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "ushr s30, s29, #1");
  // TEST_SINGLE(ushr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "ushr s30, s29, #31");
  TEST_SINGLE(ushr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "ushr d30, d29, #1");
  TEST_SINGLE(ushr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "ushr d30, d29, #63");

  // TEST_SINGLE(usra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "usra b30, b29, #1");
  // TEST_SINGLE(usra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "usra b30, b29, #7");
  // TEST_SINGLE(usra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "usra h30, h29, #1");
  // TEST_SINGLE(usra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "usra h30, h29, #15");
  // TEST_SINGLE(usra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "usra s30, s29, #1");
  // TEST_SINGLE(usra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "usra s30, s29, #31");
  TEST_SINGLE(usra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "usra d30, d29, #1");
  TEST_SINGLE(usra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "usra d30, d29, #63");

  // TEST_SINGLE(urshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "urshr b30, b29, #1");
  // TEST_SINGLE(urshr(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "urshr b30, b29, #7");
  // TEST_SINGLE(urshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "urshr h30, h29, #1");
  // TEST_SINGLE(urshr(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "urshr h30, h29, #15");
  // TEST_SINGLE(urshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "urshr s30, s29, #1");
  // TEST_SINGLE(urshr(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "urshr s30, s29, #31");
  TEST_SINGLE(urshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "urshr d30, d29, #1");
  TEST_SINGLE(urshr(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "urshr d30, d29, #63");

  // TEST_SINGLE(ursra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "ursra b30, b29, #1");
  // TEST_SINGLE(ursra(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "ursra b30, b29, #7");
  // TEST_SINGLE(ursra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "ursra h30, h29, #1");
  // TEST_SINGLE(ursra(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "ursra h30, h29, #15");
  // TEST_SINGLE(ursra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "ursra s30, s29, #1");
  // TEST_SINGLE(ursra(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "ursra s30, s29, #31");
  TEST_SINGLE(ursra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "ursra d30, d29, #1");
  TEST_SINGLE(ursra(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "ursra d30, d29, #63");

  // TEST_SINGLE(sri(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "sri b30, b29, #1");
  // TEST_SINGLE(sri(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "sri b30, b29, #7");
  // TEST_SINGLE(sri(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "sri h30, h29, #1");
  // TEST_SINGLE(sri(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sri h30, h29, #15");
  // TEST_SINGLE(sri(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "sri s30, s29, #1");
  // TEST_SINGLE(sri(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sri s30, s29, #31");
  TEST_SINGLE(sri(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "sri d30, d29, #1");
  TEST_SINGLE(sri(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sri d30, d29, #63");

  // TEST_SINGLE(sli(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1),   "sli b30, b29, #1");
  // TEST_SINGLE(sli(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7),   "sli b30, b29, #7");
  // TEST_SINGLE(sli(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1),  "sli h30, h29, #1");
  // TEST_SINGLE(sli(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sli h30, h29, #15");
  // TEST_SINGLE(sli(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1),  "sli s30, s29, #1");
  // TEST_SINGLE(sli(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sli s30, s29, #31");
  TEST_SINGLE(sli(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "sli d30, d29, #1");
  TEST_SINGLE(sli(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sli d30, d29, #63");

  TEST_SINGLE(sqshlu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqshlu b30, b29, #1");
  TEST_SINGLE(sqshlu(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqshlu b30, b29, #7");
  TEST_SINGLE(sqshlu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqshlu h30, h29, #1");
  TEST_SINGLE(sqshlu(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqshlu h30, h29, #15");
  TEST_SINGLE(sqshlu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqshlu s30, s29, #1");
  TEST_SINGLE(sqshlu(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqshlu s30, s29, #31");
  TEST_SINGLE(sqshlu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "sqshlu d30, d29, #1");
  TEST_SINGLE(sqshlu(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqshlu d30, d29, #63");

  TEST_SINGLE(uqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "uqshl b30, b29, #1");
  TEST_SINGLE(uqshl(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "uqshl b30, b29, #7");
  TEST_SINGLE(uqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "uqshl h30, h29, #1");
  TEST_SINGLE(uqshl(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "uqshl h30, h29, #15");
  TEST_SINGLE(uqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "uqshl s30, s29, #1");
  TEST_SINGLE(uqshl(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "uqshl s30, s29, #31");
  TEST_SINGLE(uqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1), "uqshl d30, d29, #1");
  TEST_SINGLE(uqshl(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "uqshl d30, d29, #63");

  TEST_SINGLE(sqshrun(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqshrun b30, h29, #1");
  TEST_SINGLE(sqshrun(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqshrun b30, h29, #7");
  TEST_SINGLE(sqshrun(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqshrun h30, s29, #1");
  TEST_SINGLE(sqshrun(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqshrun h30, s29, #15");
  TEST_SINGLE(sqshrun(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqshrun s30, d29, #1");
  TEST_SINGLE(sqshrun(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqshrun s30, d29, #31");
  // TEST_SINGLE(sqshrun(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "sqshrun d30, d29, #1");
  // TEST_SINGLE(sqshrun(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqshrun d30, d29, #63");

  TEST_SINGLE(sqrshrun(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "sqrshrun b30, h29, #1");
  TEST_SINGLE(sqrshrun(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "sqrshrun b30, h29, #7");
  TEST_SINGLE(sqrshrun(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "sqrshrun h30, s29, #1");
  TEST_SINGLE(sqrshrun(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "sqrshrun h30, s29, #15");
  TEST_SINGLE(sqrshrun(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "sqrshrun s30, d29, #1");
  TEST_SINGLE(sqrshrun(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "sqrshrun s30, d29, #31");
  // TEST_SINGLE(sqrshrun(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "sqrshrun d30, d29, #1");
  // TEST_SINGLE(sqrshrun(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "sqrshrun d30, d29, #63");

  TEST_SINGLE(uqshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "uqshrn b30, h29, #1");
  TEST_SINGLE(uqshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "uqshrn b30, h29, #7");
  TEST_SINGLE(uqshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "uqshrn h30, s29, #1");
  TEST_SINGLE(uqshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "uqshrn h30, s29, #15");
  TEST_SINGLE(uqshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "uqshrn s30, d29, #1");
  TEST_SINGLE(uqshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "uqshrn s30, d29, #31");
  // TEST_SINGLE(uqshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "uqshrn d30, d29, #1");
  // TEST_SINGLE(uqshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "uqshrn d30, d29, #63");

  TEST_SINGLE(uqrshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 1), "uqrshrn b30, h29, #1");
  TEST_SINGLE(uqrshrn(ScalarRegSize::i8Bit, VReg::v30, VReg::v29, 7), "uqrshrn b30, h29, #7");
  TEST_SINGLE(uqrshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 1), "uqrshrn h30, s29, #1");
  TEST_SINGLE(uqrshrn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, 15), "uqrshrn h30, s29, #15");
  TEST_SINGLE(uqrshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 1), "uqrshrn s30, d29, #1");
  TEST_SINGLE(uqrshrn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, 31), "uqrshrn s30, d29, #31");
  // TEST_SINGLE(uqrshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 1),  "uqrshrn d30, d29, #1");
  // TEST_SINGLE(uqrshrn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, 63), "uqrshrn d30, d29, #63");

  // TODO: Implement `UCVTF, FCVTZU' in emitter
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Advanced SIMD scalar x indexed element") {
  // TODO: Implement in emitter
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point data-processing (1 source)") {
  TEST_SINGLE(fmov(SReg::s30, SReg::s29), "fmov s30, s29");
  TEST_SINGLE(fabs(SReg::s30, SReg::s29), "fabs s30, s29");
  TEST_SINGLE(fneg(SReg::s30, SReg::s29), "fneg s30, s29");
  TEST_SINGLE(fsqrt(SReg::s30, SReg::s29), "fsqrt s30, s29");
  TEST_SINGLE(fcvt(DReg::d30, SReg::s29), "fcvt d30, s29");
  TEST_SINGLE(fcvt(HReg::h30, SReg::s29), "fcvt h30, s29");
  TEST_SINGLE(frintn(SReg::s30, SReg::s29), "frintn s30, s29");
  TEST_SINGLE(frintp(SReg::s30, SReg::s29), "frintp s30, s29");
  TEST_SINGLE(frintm(SReg::s30, SReg::s29), "frintm s30, s29");
  TEST_SINGLE(frintz(SReg::s30, SReg::s29), "frintz s30, s29");
  TEST_SINGLE(frinta(SReg::s30, SReg::s29), "frinta s30, s29");
  TEST_SINGLE(frintx(SReg::s30, SReg::s29), "frintx s30, s29");
  TEST_SINGLE(frinti(SReg::s30, SReg::s29), "frinti s30, s29");
  TEST_SINGLE(frint32z(SReg::s30, SReg::s29), "frint32z s30, s29");
  TEST_SINGLE(frint32x(SReg::s30, SReg::s29), "frint32x s30, s29");
  TEST_SINGLE(frint64z(SReg::s30, SReg::s29), "frint64z s30, s29");
  TEST_SINGLE(frint64x(SReg::s30, SReg::s29), "frint64x s30, s29");

  TEST_SINGLE(fmov(DReg::d30, DReg::d29), "fmov d30, d29");
  TEST_SINGLE(fabs(DReg::d30, DReg::d29), "fabs d30, d29");
  TEST_SINGLE(fneg(DReg::d30, DReg::d29), "fneg d30, d29");
  TEST_SINGLE(fsqrt(DReg::d30, DReg::d29), "fsqrt d30, d29");
  TEST_SINGLE(fcvt(SReg::s30, DReg::d29), "fcvt s30, d29");
  if (false) {
    // vixl doesn't support this instruction.
    TEST_SINGLE(bfcvt(HReg::h30, SReg::s29), "bfcvt h30, s29");
  }
  TEST_SINGLE(fcvt(HReg::h30, DReg::d29), "fcvt h30, d29");
  TEST_SINGLE(frintn(DReg::d30, DReg::d29), "frintn d30, d29");
  TEST_SINGLE(frintp(DReg::d30, DReg::d29), "frintp d30, d29");
  TEST_SINGLE(frintm(DReg::d30, DReg::d29), "frintm d30, d29");
  TEST_SINGLE(frintz(DReg::d30, DReg::d29), "frintz d30, d29");
  TEST_SINGLE(frinta(DReg::d30, DReg::d29), "frinta d30, d29");
  TEST_SINGLE(frintx(DReg::d30, DReg::d29), "frintx d30, d29");
  TEST_SINGLE(frinti(DReg::d30, DReg::d29), "frinti d30, d29");
  TEST_SINGLE(frint32z(DReg::d30, DReg::d29), "frint32z d30, d29");
  TEST_SINGLE(frint32x(DReg::d30, DReg::d29), "frint32x d30, d29");
  TEST_SINGLE(frint64z(DReg::d30, DReg::d29), "frint64z d30, d29");
  TEST_SINGLE(frint64x(DReg::d30, DReg::d29), "frint64x d30, d29");

  TEST_SINGLE(fmov(HReg::h30, HReg::h29), "fmov h30, h29");
  TEST_SINGLE(fabs(HReg::h30, HReg::h29), "fabs h30, h29");
  TEST_SINGLE(fneg(HReg::h30, HReg::h29), "fneg h30, h29");
  TEST_SINGLE(fsqrt(HReg::h30, HReg::h29), "fsqrt h30, h29");
  TEST_SINGLE(fcvt(SReg::s30, HReg::h29), "fcvt s30, h29");
  TEST_SINGLE(fcvt(DReg::d30, HReg::h29), "fcvt d30, h29");
  TEST_SINGLE(frintn(HReg::h30, HReg::h29), "frintn h30, h29");
  TEST_SINGLE(frintp(HReg::h30, HReg::h29), "frintp h30, h29");
  TEST_SINGLE(frintm(HReg::h30, HReg::h29), "frintm h30, h29");
  TEST_SINGLE(frintz(HReg::h30, HReg::h29), "frintz h30, h29");
  TEST_SINGLE(frinta(HReg::h30, HReg::h29), "frinta h30, h29");
  TEST_SINGLE(frintx(HReg::h30, HReg::h29), "frintx h30, h29");
  TEST_SINGLE(frinti(HReg::h30, HReg::h29), "frinti h30, h29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point data-processing (1 source sized)") {
  TEST_SINGLE(fmov(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fmov s30, s29");
  TEST_SINGLE(fabs(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fabs s30, s29");
  TEST_SINGLE(fneg(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fneg s30, s29");
  TEST_SINGLE(fsqrt(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fsqrt s30, s29");
  TEST_SINGLE(frintn(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frintn s30, s29");
  TEST_SINGLE(frintp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frintp s30, s29");
  TEST_SINGLE(frintm(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frintm s30, s29");
  TEST_SINGLE(frintz(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frintz s30, s29");
  TEST_SINGLE(frinta(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frinta s30, s29");
  TEST_SINGLE(frintx(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frintx s30, s29");
  TEST_SINGLE(frinti(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frinti s30, s29");
  TEST_SINGLE(frint32z(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frint32z s30, s29");
  TEST_SINGLE(frint32x(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frint32x s30, s29");
  TEST_SINGLE(frint64z(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frint64z s30, s29");
  TEST_SINGLE(frint64x(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "frint64x s30, s29");

  TEST_SINGLE(fmov(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fmov d30, d29");
  TEST_SINGLE(fabs(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fabs d30, d29");
  TEST_SINGLE(fneg(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fneg d30, d29");
  TEST_SINGLE(fsqrt(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fsqrt d30, d29");
  TEST_SINGLE(frintn(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frintn d30, d29");
  TEST_SINGLE(frintp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frintp d30, d29");
  TEST_SINGLE(frintm(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frintm d30, d29");
  TEST_SINGLE(frintz(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frintz d30, d29");
  TEST_SINGLE(frinta(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frinta d30, d29");
  TEST_SINGLE(frintx(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frintx d30, d29");
  TEST_SINGLE(frinti(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frinti d30, d29");
  TEST_SINGLE(frint32z(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frint32z d30, d29");
  TEST_SINGLE(frint32x(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frint32x d30, d29");
  TEST_SINGLE(frint64z(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frint64z d30, d29");
  TEST_SINGLE(frint64x(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "frint64x d30, d29");

  TEST_SINGLE(fmov(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fmov h30, h29");
  TEST_SINGLE(fabs(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fabs h30, h29");
  TEST_SINGLE(fneg(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fneg h30, h29");
  TEST_SINGLE(fsqrt(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fsqrt h30, h29");
  TEST_SINGLE(frintn(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frintn h30, h29");
  TEST_SINGLE(frintp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frintp h30, h29");
  TEST_SINGLE(frintm(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frintm h30, h29");
  TEST_SINGLE(frintz(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frintz h30, h29");
  TEST_SINGLE(frinta(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frinta h30, h29");
  TEST_SINGLE(frintx(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frintx h30, h29");
  TEST_SINGLE(frinti(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "frinti h30, h29");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point compare") {
  // Commented out lines showcase unallocated encodings.
  // TEST_SINGLE(fcmp(ScalarRegSize::i8Bit, VReg::v30, VReg::v29), "fcmp b30, b29");
  TEST_SINGLE(fcmp(ScalarRegSize::i16Bit, VReg::v30, VReg::v29), "fcmp h30, h29");
  TEST_SINGLE(fcmp(ScalarRegSize::i32Bit, VReg::v30, VReg::v29), "fcmp s30, s29");
  TEST_SINGLE(fcmp(ScalarRegSize::i64Bit, VReg::v30, VReg::v29), "fcmp d30, d29");

  TEST_SINGLE(fcmp(SReg::s30, SReg::s29), "fcmp s30, s29");
  TEST_SINGLE(fcmp(SReg::s30), "fcmp s30, #0.0");
  TEST_SINGLE(fcmpe(SReg::s30, SReg::s29), "fcmpe s30, s29");
  TEST_SINGLE(fcmpe(SReg::s30), "fcmpe s30, #0.0");

  TEST_SINGLE(fcmp(DReg::d30, DReg::d29), "fcmp d30, d29");
  TEST_SINGLE(fcmp(DReg::d30), "fcmp d30, #0.0");
  TEST_SINGLE(fcmpe(DReg::d30, DReg::d29), "fcmpe d30, d29");
  TEST_SINGLE(fcmpe(DReg::d30), "fcmpe d30, #0.0");

  TEST_SINGLE(fcmp(HReg::h30, HReg::h29), "fcmp h30, h29");
  TEST_SINGLE(fcmp(HReg::h30), "fcmp h30, #0.0");
  TEST_SINGLE(fcmpe(HReg::h30, HReg::h29), "fcmpe h30, h29");
  TEST_SINGLE(fcmpe(HReg::h30), "fcmpe h30, #0.0");
}

#if TEST_FP16
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point immediate : fp16") {
  TEST_SINGLE(fmov(ScalarRegSize::i16Bit, VReg::v30, 1.0), "fmov h30, #0x70 (1.0000)");
  float Decoding[] = {
    2.000000, 4.000000, 8.000000,  16.000000, 0.125000, 0.250000, 0.500000,  1.000000,  2.125000, 4.250000, 8.500000,  17.000000,
    0.132812, 0.265625, 0.531250,  1.062500,  2.250000, 4.500000, 9.000000,  18.000000, 0.140625, 0.281250, 0.562500,  1.125000,
    2.375000, 4.750000, 9.500000,  19.000000, 0.148438, 0.296875, 0.593750,  1.187500,  2.500000, 5.000000, 10.000000, 20.000000,
    0.156250, 0.312500, 0.625000,  1.250000,  2.625000, 5.250000, 10.500000, 21.000000, 0.164062, 0.328125, 0.656250,  1.312500,
    2.750000, 5.500000, 11.000000, 22.000000, 0.171875, 0.343750, 0.687500,  1.375000,  2.875000, 5.750000, 11.500000, 23.000000,
    0.179688, 0.359375, 0.718750,  1.437500,  3.000000, 6.000000, 12.000000, 24.000000, 0.187500, 0.375000, 0.750000,  1.500000,
    3.125000, 6.250000, 12.500000, 25.000000, 0.195312, 0.390625, 0.781250,  1.562500,  3.250000, 6.500000, 13.000000, 26.000000,
    0.203125, 0.406250, 0.812500,  1.625000,  3.375000, 6.750000, 13.500000, 27.000000, 0.210938, 0.421875, 0.843750,  1.687500,
    3.500000, 7.000000, 14.000000, 28.000000, 0.218750, 0.437500, 0.875000,  1.750000,  3.625000, 7.250000, 14.500000, 29.000000,
    0.226562, 0.453125, 0.906250,  1.812500,  3.750000, 7.500000, 15.000000, 30.000000, 0.234375, 0.468750, 0.937500,  1.875000,
    3.875000, 7.750000, 15.500000, 31.000000, 0.242188, 0.484375, 0.968750,  1.937500,
  };

  const char* DecodingString[] = {
    "fmov h30, #0x0 (2.0000)",  "fmov h30, #0x10 (4.0000)", "fmov h30, #0x20 (8.0000)",  "fmov h30, #0x30 (16.0000)",
    "fmov h30, #0x40 (0.1250)", "fmov h30, #0x50 (0.2500)", "fmov h30, #0x60 (0.5000)",  "fmov h30, #0x70 (1.0000)",
    "fmov h30, #0x1 (2.1250)",  "fmov h30, #0x11 (4.2500)", "fmov h30, #0x21 (8.5000)",  "fmov h30, #0x31 (17.0000)",
    "fmov h30, #0x41 (0.1328)", "fmov h30, #0x51 (0.2656)", "fmov h30, #0x61 (0.5312)",  "fmov h30, #0x71 (1.0625)",
    "fmov h30, #0x2 (2.2500)",  "fmov h30, #0x12 (4.5000)", "fmov h30, #0x22 (9.0000)",  "fmov h30, #0x32 (18.0000)",
    "fmov h30, #0x42 (0.1406)", "fmov h30, #0x52 (0.2812)", "fmov h30, #0x62 (0.5625)",  "fmov h30, #0x72 (1.1250)",
    "fmov h30, #0x3 (2.3750)",  "fmov h30, #0x13 (4.7500)", "fmov h30, #0x23 (9.5000)",  "fmov h30, #0x33 (19.0000)",
    "fmov h30, #0x43 (0.1484)", "fmov h30, #0x53 (0.2969)", "fmov h30, #0x63 (0.5938)",  "fmov h30, #0x73 (1.1875)",
    "fmov h30, #0x4 (2.5000)",  "fmov h30, #0x14 (5.0000)", "fmov h30, #0x24 (10.0000)", "fmov h30, #0x34 (20.0000)",
    "fmov h30, #0x44 (0.1562)", "fmov h30, #0x54 (0.3125)", "fmov h30, #0x64 (0.6250)",  "fmov h30, #0x74 (1.2500)",
    "fmov h30, #0x5 (2.6250)",  "fmov h30, #0x15 (5.2500)", "fmov h30, #0x25 (10.5000)", "fmov h30, #0x35 (21.0000)",
    "fmov h30, #0x45 (0.1641)", "fmov h30, #0x55 (0.3281)", "fmov h30, #0x65 (0.6562)",  "fmov h30, #0x75 (1.3125)",
    "fmov h30, #0x6 (2.7500)",  "fmov h30, #0x16 (5.5000)", "fmov h30, #0x26 (11.0000)", "fmov h30, #0x36 (22.0000)",
    "fmov h30, #0x46 (0.1719)", "fmov h30, #0x56 (0.3438)", "fmov h30, #0x66 (0.6875)",  "fmov h30, #0x76 (1.3750)",
    "fmov h30, #0x7 (2.8750)",  "fmov h30, #0x17 (5.7500)", "fmov h30, #0x27 (11.5000)", "fmov h30, #0x37 (23.0000)",
    "fmov h30, #0x47 (0.1797)", "fmov h30, #0x57 (0.3594)", "fmov h30, #0x67 (0.7188)",  "fmov h30, #0x77 (1.4375)",
    "fmov h30, #0x8 (3.0000)",  "fmov h30, #0x18 (6.0000)", "fmov h30, #0x28 (12.0000)", "fmov h30, #0x38 (24.0000)",
    "fmov h30, #0x48 (0.1875)", "fmov h30, #0x58 (0.3750)", "fmov h30, #0x68 (0.7500)",  "fmov h30, #0x78 (1.5000)",
    "fmov h30, #0x9 (3.1250)",  "fmov h30, #0x19 (6.2500)", "fmov h30, #0x29 (12.5000)", "fmov h30, #0x39 (25.0000)",
    "fmov h30, #0x49 (0.1953)", "fmov h30, #0x59 (0.3906)", "fmov h30, #0x69 (0.7812)",  "fmov h30, #0x79 (1.5625)",
    "fmov h30, #0xa (3.2500)",  "fmov h30, #0x1a (6.5000)", "fmov h30, #0x2a (13.0000)", "fmov h30, #0x3a (26.0000)",
    "fmov h30, #0x4a (0.2031)", "fmov h30, #0x5a (0.4062)", "fmov h30, #0x6a (0.8125)",  "fmov h30, #0x7a (1.6250)",
    "fmov h30, #0xb (3.3750)",  "fmov h30, #0x1b (6.7500)", "fmov h30, #0x2b (13.5000)", "fmov h30, #0x3b (27.0000)",
    "fmov h30, #0x4b (0.2109)", "fmov h30, #0x5b (0.4219)", "fmov h30, #0x6b (0.8438)",  "fmov h30, #0x7b (1.6875)",
    "fmov h30, #0xc (3.5000)",  "fmov h30, #0x1c (7.0000)", "fmov h30, #0x2c (14.0000)", "fmov h30, #0x3c (28.0000)",
    "fmov h30, #0x4c (0.2188)", "fmov h30, #0x5c (0.4375)", "fmov h30, #0x6c (0.8750)",  "fmov h30, #0x7c (1.7500)",
    "fmov h30, #0xd (3.6250)",  "fmov h30, #0x1d (7.2500)", "fmov h30, #0x2d (14.5000)", "fmov h30, #0x3d (29.0000)",
    "fmov h30, #0x4d (0.2266)", "fmov h30, #0x5d (0.4531)", "fmov h30, #0x6d (0.9062)",  "fmov h30, #0x7d (1.8125)",
    "fmov h30, #0xe (3.7500)",  "fmov h30, #0x1e (7.5000)", "fmov h30, #0x2e (15.0000)", "fmov h30, #0x3e (30.0000)",
    "fmov h30, #0x4e (0.2344)", "fmov h30, #0x5e (0.4688)", "fmov h30, #0x6e (0.9375)",  "fmov h30, #0x7e (1.8750)",
    "fmov h30, #0xf (3.8750)",  "fmov h30, #0x1f (7.7500)", "fmov h30, #0x2f (15.5000)", "fmov h30, #0x3f (31.0000)",
    "fmov h30, #0x4f (0.2422)", "fmov h30, #0x5f (0.4844)", "fmov h30, #0x6f (0.9688)",  "fmov h30, #0x7f (1.9375)",
  };

  for (size_t i = 0; i < (sizeof(Decoding) / sizeof(Decoding[0])); ++i) {
    TEST_SINGLE(fmov(ScalarRegSize::i16Bit, VReg::v30, Decoding[i]), DecodingString[i]);
  }
}
#endif

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point immediate") {
  TEST_SINGLE(fmov(ScalarRegSize::i32Bit, VReg::v30, 1.0), "fmov s30, #0x70 (1.0000)");
  TEST_SINGLE(fmov(ScalarRegSize::i64Bit, VReg::v30, 1.0), "fmov d30, #0x70 (1.0000)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point conditional compare") {
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::None, Condition::CC_AL), "fccmp s30, s29, #nzcv, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_N, Condition::CC_AL), "fccmp s30, s29, #Nzcv, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmp s30, s29, #nZcv, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_C, Condition::CC_AL), "fccmp s30, s29, #nzCv, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_V, Condition::CC_AL), "fccmp s30, s29, #nzcV, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmp s30, s29, #NZCV, al");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::None, Condition::CC_EQ), "fccmp s30, s29, #nzcv, eq");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmp s30, s29, #Nzcv, eq");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmp s30, s29, #nZcv, eq");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmp s30, s29, #nzCv, eq");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmp s30, s29, #nzcV, eq");
  TEST_SINGLE(fccmp(SReg::s30, SReg::s29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmp s30, s29, #NZCV, eq");

  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::None, Condition::CC_AL), "fccmpe s30, s29, #nzcv, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_N, Condition::CC_AL), "fccmpe s30, s29, #Nzcv, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmpe s30, s29, #nZcv, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_C, Condition::CC_AL), "fccmpe s30, s29, #nzCv, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_V, Condition::CC_AL), "fccmpe s30, s29, #nzcV, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmpe s30, s29, #NZCV, al");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::None, Condition::CC_EQ), "fccmpe s30, s29, #nzcv, eq");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmpe s30, s29, #Nzcv, eq");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmpe s30, s29, #nZcv, eq");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmpe s30, s29, #nzCv, eq");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmpe s30, s29, #nzcV, eq");
  TEST_SINGLE(fccmpe(SReg::s30, SReg::s29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmpe s30, s29, #NZCV, eq");

  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::None, Condition::CC_AL), "fccmp d30, d29, #nzcv, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_N, Condition::CC_AL), "fccmp d30, d29, #Nzcv, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmp d30, d29, #nZcv, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_C, Condition::CC_AL), "fccmp d30, d29, #nzCv, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_V, Condition::CC_AL), "fccmp d30, d29, #nzcV, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmp d30, d29, #NZCV, al");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::None, Condition::CC_EQ), "fccmp d30, d29, #nzcv, eq");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmp d30, d29, #Nzcv, eq");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmp d30, d29, #nZcv, eq");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmp d30, d29, #nzCv, eq");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmp d30, d29, #nzcV, eq");
  TEST_SINGLE(fccmp(DReg::d30, DReg::d29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmp d30, d29, #NZCV, eq");

  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::None, Condition::CC_AL), "fccmpe d30, d29, #nzcv, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_N, Condition::CC_AL), "fccmpe d30, d29, #Nzcv, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmpe d30, d29, #nZcv, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_C, Condition::CC_AL), "fccmpe d30, d29, #nzCv, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_V, Condition::CC_AL), "fccmpe d30, d29, #nzcV, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmpe d30, d29, #NZCV, al");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::None, Condition::CC_EQ), "fccmpe d30, d29, #nzcv, eq");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmpe d30, d29, #Nzcv, eq");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmpe d30, d29, #nZcv, eq");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmpe d30, d29, #nzCv, eq");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmpe d30, d29, #nzcV, eq");
  TEST_SINGLE(fccmpe(DReg::d30, DReg::d29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmpe d30, d29, #NZCV, eq");

  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::None, Condition::CC_AL), "fccmp h30, h29, #nzcv, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_N, Condition::CC_AL), "fccmp h30, h29, #Nzcv, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmp h30, h29, #nZcv, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_C, Condition::CC_AL), "fccmp h30, h29, #nzCv, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_V, Condition::CC_AL), "fccmp h30, h29, #nzcV, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmp h30, h29, #NZCV, al");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::None, Condition::CC_EQ), "fccmp h30, h29, #nzcv, eq");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmp h30, h29, #Nzcv, eq");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmp h30, h29, #nZcv, eq");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmp h30, h29, #nzCv, eq");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmp h30, h29, #nzcV, eq");
  TEST_SINGLE(fccmp(HReg::h30, HReg::h29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmp h30, h29, #NZCV, eq");

  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::None, Condition::CC_AL), "fccmpe h30, h29, #nzcv, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_N, Condition::CC_AL), "fccmpe h30, h29, #Nzcv, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_Z, Condition::CC_AL), "fccmpe h30, h29, #nZcv, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_C, Condition::CC_AL), "fccmpe h30, h29, #nzCv, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_V, Condition::CC_AL), "fccmpe h30, h29, #nzcV, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_NZCV, Condition::CC_AL), "fccmpe h30, h29, #NZCV, al");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::None, Condition::CC_EQ), "fccmpe h30, h29, #nzcv, eq");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_N, Condition::CC_EQ), "fccmpe h30, h29, #Nzcv, eq");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_Z, Condition::CC_EQ), "fccmpe h30, h29, #nZcv, eq");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_C, Condition::CC_EQ), "fccmpe h30, h29, #nzCv, eq");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_V, Condition::CC_EQ), "fccmpe h30, h29, #nzcV, eq");
  TEST_SINGLE(fccmpe(HReg::h30, HReg::h29, StatusFlags::Flag_NZCV, Condition::CC_EQ), "fccmpe h30, h29, #NZCV, eq");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point data-processing (2 source)") {
  TEST_SINGLE(fmul(SReg::s30, SReg::s29, SReg::s28), "fmul s30, s29, s28");
  TEST_SINGLE(fdiv(SReg::s30, SReg::s29, SReg::s28), "fdiv s30, s29, s28");
  TEST_SINGLE(fadd(SReg::s30, SReg::s29, SReg::s28), "fadd s30, s29, s28");
  TEST_SINGLE(fsub(SReg::s30, SReg::s29, SReg::s28), "fsub s30, s29, s28");
  TEST_SINGLE(fmax(SReg::s30, SReg::s29, SReg::s28), "fmax s30, s29, s28");
  TEST_SINGLE(fmin(SReg::s30, SReg::s29, SReg::s28), "fmin s30, s29, s28");
  TEST_SINGLE(fmaxnm(SReg::s30, SReg::s29, SReg::s28), "fmaxnm s30, s29, s28");
  TEST_SINGLE(fminnm(SReg::s30, SReg::s29, SReg::s28), "fminnm s30, s29, s28");
  TEST_SINGLE(fnmul(SReg::s30, SReg::s29, SReg::s28), "fnmul s30, s29, s28");

  TEST_SINGLE(fmul(DReg::d30, DReg::d29, DReg::d28), "fmul d30, d29, d28");
  TEST_SINGLE(fdiv(DReg::d30, DReg::d29, DReg::d28), "fdiv d30, d29, d28");
  TEST_SINGLE(fadd(DReg::d30, DReg::d29, DReg::d28), "fadd d30, d29, d28");
  TEST_SINGLE(fsub(DReg::d30, DReg::d29, DReg::d28), "fsub d30, d29, d28");
  TEST_SINGLE(fmax(DReg::d30, DReg::d29, DReg::d28), "fmax d30, d29, d28");
  TEST_SINGLE(fmin(DReg::d30, DReg::d29, DReg::d28), "fmin d30, d29, d28");
  TEST_SINGLE(fmaxnm(DReg::d30, DReg::d29, DReg::d28), "fmaxnm d30, d29, d28");
  TEST_SINGLE(fminnm(DReg::d30, DReg::d29, DReg::d28), "fminnm d30, d29, d28");
  TEST_SINGLE(fnmul(DReg::d30, DReg::d29, DReg::d28), "fnmul d30, d29, d28");

  TEST_SINGLE(fmul(HReg::h30, HReg::h29, HReg::h28), "fmul h30, h29, h28");
  TEST_SINGLE(fdiv(HReg::h30, HReg::h29, HReg::h28), "fdiv h30, h29, h28");
  TEST_SINGLE(fadd(HReg::h30, HReg::h29, HReg::h28), "fadd h30, h29, h28");
  TEST_SINGLE(fsub(HReg::h30, HReg::h29, HReg::h28), "fsub h30, h29, h28");
  TEST_SINGLE(fmax(HReg::h30, HReg::h29, HReg::h28), "fmax h30, h29, h28");
  TEST_SINGLE(fmin(HReg::h30, HReg::h29, HReg::h28), "fmin h30, h29, h28");
  TEST_SINGLE(fmaxnm(HReg::h30, HReg::h29, HReg::h28), "fmaxnm h30, h29, h28");
  TEST_SINGLE(fminnm(HReg::h30, HReg::h29, HReg::h28), "fminnm h30, h29, h28");
  TEST_SINGLE(fnmul(HReg::h30, HReg::h29, HReg::h28), "fnmul h30, h29, h28");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point data-processing (2 source sized)") {
  TEST_SINGLE(fmul(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fmul s30, s29, s28");
  TEST_SINGLE(fdiv(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fdiv s30, s29, s28");
  TEST_SINGLE(fadd(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fadd s30, s29, s28");
  TEST_SINGLE(fsub(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fsub s30, s29, s28");
  TEST_SINGLE(fmax(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fmax s30, s29, s28");
  TEST_SINGLE(fmin(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fmin s30, s29, s28");
  TEST_SINGLE(fmaxnm(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fmaxnm s30, s29, s28");
  TEST_SINGLE(fminnm(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fminnm s30, s29, s28");
  TEST_SINGLE(fnmul(ScalarRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v28), "fnmul s30, s29, s28");

  TEST_SINGLE(fmul(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fmul d30, d29, d28");
  TEST_SINGLE(fdiv(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fdiv d30, d29, d28");
  TEST_SINGLE(fadd(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fadd d30, d29, d28");
  TEST_SINGLE(fsub(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fsub d30, d29, d28");
  TEST_SINGLE(fmax(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fmax d30, d29, d28");
  TEST_SINGLE(fmin(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fmin d30, d29, d28");
  TEST_SINGLE(fmaxnm(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fmaxnm d30, d29, d28");
  TEST_SINGLE(fminnm(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fminnm d30, d29, d28");
  TEST_SINGLE(fnmul(ScalarRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28), "fnmul d30, d29, d28");

  TEST_SINGLE(fmul(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fmul h30, h29, h28");
  TEST_SINGLE(fdiv(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fdiv h30, h29, h28");
  TEST_SINGLE(fadd(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fadd h30, h29, h28");
  TEST_SINGLE(fsub(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fsub h30, h29, h28");
  TEST_SINGLE(fmax(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fmax h30, h29, h28");
  TEST_SINGLE(fmin(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fmin h30, h29, h28");
  TEST_SINGLE(fmaxnm(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fmaxnm h30, h29, h28");
  TEST_SINGLE(fminnm(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fminnm h30, h29, h28");
  TEST_SINGLE(fnmul(ScalarRegSize::i16Bit, VReg::v30, VReg::v29, VReg::v28), "fnmul h30, h29, h28");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point conditional select") {
  TEST_SINGLE(fcsel(SReg::s30, SReg::s29, SReg::s28, Condition::CC_AL), "fcsel s30, s29, s28, al");
  TEST_SINGLE(fcsel(SReg::s30, SReg::s29, SReg::s28, Condition::CC_EQ), "fcsel s30, s29, s28, eq");

  TEST_SINGLE(fcsel(DReg::d30, DReg::d29, DReg::d28, Condition::CC_AL), "fcsel d30, d29, d28, al");
  TEST_SINGLE(fcsel(DReg::d30, DReg::d29, DReg::d28, Condition::CC_EQ), "fcsel d30, d29, d28, eq");

  TEST_SINGLE(fcsel(HReg::h30, HReg::h29, HReg::h28, Condition::CC_AL), "fcsel h30, h29, h28, al");
  TEST_SINGLE(fcsel(HReg::h30, HReg::h29, HReg::h28, Condition::CC_EQ), "fcsel h30, h29, h28, eq");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Scalar: Floating-point data-processing (3 source)") {
  TEST_SINGLE(fmadd(SReg::s30, SReg::s29, SReg::s28, SReg::s27), "fmadd s30, s29, s28, s27");
  TEST_SINGLE(fmsub(SReg::s30, SReg::s29, SReg::s28, SReg::s27), "fmsub s30, s29, s28, s27");
  TEST_SINGLE(fnmadd(SReg::s30, SReg::s29, SReg::s28, SReg::s27), "fnmadd s30, s29, s28, s27");
  TEST_SINGLE(fnmsub(SReg::s30, SReg::s29, SReg::s28, SReg::s27), "fnmsub s30, s29, s28, s27");

  TEST_SINGLE(fmadd(DReg::d30, DReg::d29, DReg::d28, DReg::d27), "fmadd d30, d29, d28, d27");
  TEST_SINGLE(fmsub(DReg::d30, DReg::d29, DReg::d28, DReg::d27), "fmsub d30, d29, d28, d27");
  TEST_SINGLE(fnmadd(DReg::d30, DReg::d29, DReg::d28, DReg::d27), "fnmadd d30, d29, d28, d27");
  TEST_SINGLE(fnmsub(DReg::d30, DReg::d29, DReg::d28, DReg::d27), "fnmsub d30, d29, d28, d27");

  TEST_SINGLE(fmadd(HReg::h30, HReg::h29, HReg::h28, HReg::h27), "fmadd h30, h29, h28, h27");
  TEST_SINGLE(fmsub(HReg::h30, HReg::h29, HReg::h28, HReg::h27), "fmsub h30, h29, h28, h27");
  TEST_SINGLE(fnmadd(HReg::h30, HReg::h29, HReg::h28, HReg::h27), "fnmadd h30, h29, h28, h27");
  TEST_SINGLE(fnmsub(HReg::h30, HReg::h29, HReg::h28, HReg::h27), "fnmsub h30, h29, h28, h27");
}
