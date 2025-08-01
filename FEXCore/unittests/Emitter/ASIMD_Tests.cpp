// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic AES") {
  TEST_SINGLE(aese(VReg::v30, VReg::v29), "aese v30.16b, v29.16b");
  TEST_SINGLE(aesd(VReg::v30, VReg::v29), "aesd v30.16b, v29.16b");
  TEST_SINGLE(aesmc(VReg::v30, VReg::v29), "aesmc v30.16b, v29.16b");
  TEST_SINGLE(aesimc(VReg::v30, VReg::v29), "aesimc v30.16b, v29.16b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic three-register SHA") {
  TEST_SINGLE(sha1c(VReg::v30, SReg::s29, VReg::v28), "sha1c q30, s29, v28.4s");
  TEST_SINGLE(sha1p(VReg::v30, SReg::s29, VReg::v28), "sha1p q30, s29, v28.4s");
  TEST_SINGLE(sha1m(VReg::v30, SReg::s29, VReg::v28), "sha1m q30, s29, v28.4s");
  TEST_SINGLE(sha1su0(VReg::v30, VReg::v29, VReg::v28), "sha1su0 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sha256h(VReg::v30, VReg::v29, VReg::v28), "sha256h q30, q29, v28.4s");
  TEST_SINGLE(sha256h2(VReg::v30, VReg::v29, VReg::v28), "sha256h2 q30, q29, v28.4s");
  TEST_SINGLE(sha256su1(VReg::v30, VReg::v29, VReg::v28), "sha256su1 v30.4s, v29.4s, v28.4s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic two-register SHA") {
  TEST_SINGLE(sha1h(SReg::s30, SReg::s29), "sha1h s30, s29");
  TEST_SINGLE(sha1su1(VReg::v30, VReg::v29), "sha1su1 v30.4s, v29.4s");
  TEST_SINGLE(sha256su0(VReg::v30, VReg::v29), "sha256su0 v30.4s, v29.4s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD table lookup") {
  TEST_SINGLE(tbl(QReg::q30, QReg::q26, QReg::q25), "tbl v30.16b, {v26.16b}, v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q26, DReg::d25), "tbl v30.8b, {v26.16b}, v25.8b");
  TEST_SINGLE(tbx(QReg::q30, QReg::q26, QReg::q25), "tbx v30.16b, {v26.16b}, v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q26, DReg::d25), "tbx v30.8b, {v26.16b}, v25.8b");

  TEST_SINGLE(tbl(QReg::q30, QReg::q31, QReg::q0, QReg::q25), "tbl v30.16b, {v31.16b, v0.16b}, v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q31, QReg::q0, DReg::d25), "tbl v30.8b, {v31.16b, v0.16b}, v25.8b");
  TEST_SINGLE(tbl(QReg::q30, QReg::q26, QReg::q27, QReg::q25), "tbl v30.16b, {v26.16b, v27.16b}, v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q26, QReg::q27, DReg::d25), "tbl v30.8b, {v26.16b, v27.16b}, v25.8b");

  TEST_SINGLE(tbx(QReg::q30, QReg::q31, QReg::q0, QReg::q25), "tbx v30.16b, {v31.16b, v0.16b}, v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q31, QReg::q0, DReg::d25), "tbx v30.8b, {v31.16b, v0.16b}, v25.8b");
  TEST_SINGLE(tbx(QReg::q30, QReg::q26, QReg::q27, QReg::q25), "tbx v30.16b, {v26.16b, v27.16b}, v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q26, QReg::q27, DReg::d25), "tbx v30.8b, {v26.16b, v27.16b}, v25.8b");

  TEST_SINGLE(tbl(QReg::q30, QReg::q31, QReg::q0, QReg::q1, QReg::q25), "tbl v30.16b, {v31.16b, v0.16b, v1.16b}, v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q31, QReg::q0, QReg::q1, DReg::d25), "tbl v30.8b, {v31.16b, v0.16b, v1.16b}, v25.8b");
  TEST_SINGLE(tbl(QReg::q30, QReg::q26, QReg::q27, QReg::q28, QReg::q25), "tbl v30.16b, {v26.16b, v27.16b, v28.16b}, v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q26, QReg::q27, QReg::q28, DReg::d25), "tbl v30.8b, {v26.16b, v27.16b, v28.16b}, v25.8b");

  TEST_SINGLE(tbx(QReg::q30, QReg::q31, QReg::q0, QReg::q1, QReg::q25), "tbx v30.16b, {v31.16b, v0.16b, v1.16b}, v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q31, QReg::q0, QReg::q1, DReg::d25), "tbx v30.8b, {v31.16b, v0.16b, v1.16b}, v25.8b");
  TEST_SINGLE(tbx(QReg::q30, QReg::q26, QReg::q27, QReg::q28, QReg::q25), "tbx v30.16b, {v26.16b, v27.16b, v28.16b}, v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q26, QReg::q27, QReg::q28, DReg::d25), "tbx v30.8b, {v26.16b, v27.16b, v28.16b}, v25.8b");

  TEST_SINGLE(tbl(QReg::q30, QReg::q31, QReg::q0, QReg::q1, QReg::q2, QReg::q25), "tbl v30.16b, {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                  "v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q31, QReg::q0, QReg::q1, QReg::q2, DReg::d25), "tbl v30.8b, {v31.16b, v0.16b, v1.16b, v2.16b}, v25.8b");
  TEST_SINGLE(tbl(QReg::q30, QReg::q26, QReg::q27, QReg::q28, QReg::q29, QReg::q25), "tbl v30.16b, {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                     "v25.16b");
  TEST_SINGLE(tbl(DReg::d30, QReg::q26, QReg::q27, QReg::q28, QReg::q29, DReg::d25), "tbl v30.8b, {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                     "v25.8b");

  TEST_SINGLE(tbx(QReg::q30, QReg::q31, QReg::q0, QReg::q1, QReg::q2, QReg::q25), "tbx v30.16b, {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                  "v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q31, QReg::q0, QReg::q1, QReg::q2, DReg::d25), "tbx v30.8b, {v31.16b, v0.16b, v1.16b, v2.16b}, v25.8b");
  TEST_SINGLE(tbx(QReg::q30, QReg::q26, QReg::q27, QReg::q28, QReg::q29, QReg::q25), "tbx v30.16b, {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                     "v25.16b");
  TEST_SINGLE(tbx(DReg::d30, QReg::q26, QReg::q27, QReg::q28, QReg::q29, DReg::d25), "tbx v30.8b, {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                     "v25.8b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD permute") {
  // Commented out lines showcase unallocated encodings.
  TEST_SINGLE(uzp1(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uzp1 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uzp1(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uzp1 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uzp1(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uzp1 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(uzp1(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uzp1 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uzp1(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uzp1 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uzp1(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uzp1 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uzp1(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uzp1 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uzp1(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uzp1 v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(trn1(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "trn1 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(trn1(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "trn1 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(trn1(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "trn1 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(trn1(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "trn1 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(trn1(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "trn1 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(trn1(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "trn1 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(trn1(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "trn1 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(trn1(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "trn1 v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(zip1(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "zip1 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(zip1(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "zip1 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(zip1(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "zip1 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(zip1(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "zip1 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(zip1(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "zip1 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(zip1(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "zip1 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(zip1(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "zip1 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(zip1(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "zip1 v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uzp2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uzp2 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uzp2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uzp2 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uzp2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uzp2 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(uzp2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uzp2 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uzp2(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uzp2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uzp2(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uzp2 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uzp2(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uzp2 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uzp2(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uzp2 v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(trn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "trn2 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(trn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "trn2 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(trn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "trn2 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(trn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "trn2 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(trn2(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "trn2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(trn2(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "trn2 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(trn2(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "trn2 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(trn2(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "trn2 v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(zip2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "zip2 v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(zip2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "zip2 v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(zip2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "zip2 v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(zip2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "zip2 v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(zip2(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "zip2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(zip2(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "zip2 v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(zip2(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "zip2 v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(zip2(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "zip2 v30.1d, v29.1d, v28.1d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD extract") {
  TEST_SINGLE(ext(QReg::q30, QReg::q29, QReg::q28, 0), "ext v30.16b, v29.16b, v28.16b, #0");
  TEST_SINGLE(ext(QReg::q30, QReg::q29, QReg::q28, 15), "ext v30.16b, v29.16b, v28.16b, #15");
  TEST_SINGLE(ext(DReg::d30, DReg::d29, DReg::d28, 0), "ext v30.8b, v29.8b, v28.8b, #0");
  TEST_SINGLE(ext(DReg::d30, DReg::d29, DReg::d28, 7), "ext v30.8b, v29.8b, v28.8b, #7");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD copy") {
  // Commented out lines showcase unallocated encodings.
  TEST_SINGLE(dup(SubRegSize::i8Bit, QReg::q30, QReg::q29, 0), "dup v30.16b, v29.b[0]");
  TEST_SINGLE(dup(SubRegSize::i16Bit, QReg::q30, QReg::q29, 0), "dup v30.8h, v29.h[0]");
  TEST_SINGLE(dup(SubRegSize::i32Bit, QReg::q30, QReg::q29, 0), "dup v30.4s, v29.s[0]");
  TEST_SINGLE(dup(SubRegSize::i64Bit, QReg::q30, QReg::q29, 0), "dup v30.2d, v29.d[0]");
  TEST_SINGLE(dup(SubRegSize::i8Bit, QReg::q30, QReg::q29, 15), "dup v30.16b, v29.b[15]");
  TEST_SINGLE(dup(SubRegSize::i16Bit, QReg::q30, QReg::q29, 7), "dup v30.8h, v29.h[7]");
  TEST_SINGLE(dup(SubRegSize::i32Bit, QReg::q30, QReg::q29, 3), "dup v30.4s, v29.s[3]");
  TEST_SINGLE(dup(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "dup v30.2d, v29.d[1]");

  TEST_SINGLE(dup(SubRegSize::i8Bit, DReg::d30, DReg::d29, 0), "dup v30.8b, v29.b[0]");
  TEST_SINGLE(dup(SubRegSize::i16Bit, DReg::d30, DReg::d29, 0), "dup v30.4h, v29.h[0]");
  TEST_SINGLE(dup(SubRegSize::i32Bit, DReg::d30, DReg::d29, 0), "dup v30.2s, v29.s[0]");
  // TEST_SINGLE(dup(SubRegSize::i64Bit, DReg::d30, DReg::d29, 0), "dup v30.1d, v29.d[0]");
  TEST_SINGLE(dup(SubRegSize::i8Bit, DReg::d30, DReg::d29, 15), "dup v30.8b, v29.b[15]");
  TEST_SINGLE(dup(SubRegSize::i16Bit, DReg::d30, DReg::d29, 7), "dup v30.4h, v29.h[7]");
  TEST_SINGLE(dup(SubRegSize::i32Bit, DReg::d30, DReg::d29, 3), "dup v30.2s, v29.s[3]");
  // TEST_SINGLE(dup(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1), "dup v30.1d, v29.d[1]");

  TEST_SINGLE(dup(SubRegSize::i8Bit, QReg::q30, Reg::r29), "dup v30.16b, w29");
  TEST_SINGLE(dup(SubRegSize::i16Bit, QReg::q30, Reg::r29), "dup v30.8h, w29");
  TEST_SINGLE(dup(SubRegSize::i32Bit, QReg::q30, Reg::r29), "dup v30.4s, w29");
  TEST_SINGLE(dup(SubRegSize::i64Bit, QReg::q30, Reg::r29), "dup v30.2d, x29");

  TEST_SINGLE(dup(SubRegSize::i8Bit, DReg::d30, Reg::r29), "dup v30.8b, w29");
  TEST_SINGLE(dup(SubRegSize::i16Bit, DReg::d30, Reg::r29), "dup v30.4h, w29");
  TEST_SINGLE(dup(SubRegSize::i32Bit, DReg::d30, Reg::r29), "dup v30.2s, w29");
  // TEST_SINGLE(dup(SubRegSize::i64Bit, DReg::d30, Reg::r29), "dup v30.1d, x29");
  TEST_SINGLE(smov<SubRegSize::i8Bit>(XReg::x29, VReg::v30, 0), "smov x29, v30.b[0]");
  TEST_SINGLE(smov<SubRegSize::i8Bit>(XReg::x29, VReg::v30, 15), "smov x29, v30.b[15]");
  TEST_SINGLE(smov<SubRegSize::i16Bit>(XReg::x29, VReg::v30, 0), "smov x29, v30.h[0]");
  TEST_SINGLE(smov<SubRegSize::i16Bit>(XReg::x29, VReg::v30, 7), "smov x29, v30.h[7]");
  TEST_SINGLE(smov<SubRegSize::i32Bit>(XReg::x29, VReg::v30, 0), "smov x29, v30.s[0]");
  TEST_SINGLE(smov<SubRegSize::i32Bit>(XReg::x29, VReg::v30, 3), "smov x29, v30.s[3]");

  TEST_SINGLE(smov<SubRegSize::i8Bit>(WReg::w29, VReg::v30, 0), "smov w29, v30.b[0]");
  TEST_SINGLE(smov<SubRegSize::i8Bit>(WReg::w29, VReg::v30, 15), "smov w29, v30.b[15]");
  TEST_SINGLE(smov<SubRegSize::i16Bit>(WReg::w29, VReg::v30, 0), "smov w29, v30.h[0]");
  TEST_SINGLE(smov<SubRegSize::i16Bit>(WReg::w29, VReg::v30, 7), "smov w29, v30.h[7]");

  TEST_SINGLE(umov<SubRegSize::i8Bit>(Reg::r29, VReg::v30, 0), "umov w29, v30.b[0]");
  TEST_SINGLE(umov<SubRegSize::i8Bit>(Reg::r29, VReg::v30, 15), "umov w29, v30.b[15]");
  TEST_SINGLE(umov<SubRegSize::i16Bit>(Reg::r29, VReg::v30, 0), "umov w29, v30.h[0]");
  TEST_SINGLE(umov<SubRegSize::i16Bit>(Reg::r29, VReg::v30, 7), "umov w29, v30.h[7]");
  TEST_SINGLE(umov<SubRegSize::i32Bit>(Reg::r29, VReg::v30, 0), "mov w29, v30.s[0]");
  TEST_SINGLE(umov<SubRegSize::i32Bit>(Reg::r29, VReg::v30, 3), "mov w29, v30.s[3]");
  TEST_SINGLE(umov<SubRegSize::i64Bit>(Reg::r29, VReg::v30, 0), "mov x29, v30.d[0]");
  TEST_SINGLE(umov<SubRegSize::i64Bit>(Reg::r29, VReg::v30, 1), "mov x29, v30.d[1]");

  TEST_SINGLE(ins(SubRegSize::i8Bit, VReg::v30, 0, Reg::r29), "mov v30.b[0], w29");
  TEST_SINGLE(ins(SubRegSize::i16Bit, VReg::v30, 0, Reg::r29), "mov v30.h[0], w29");
  TEST_SINGLE(ins(SubRegSize::i32Bit, VReg::v30, 0, Reg::r29), "mov v30.s[0], w29");
  TEST_SINGLE(ins(SubRegSize::i64Bit, VReg::v30, 0, Reg::r29), "mov v30.d[0], x29");
  TEST_SINGLE(ins(SubRegSize::i8Bit, VReg::v30, 15, Reg::r29), "mov v30.b[15], w29");
  TEST_SINGLE(ins(SubRegSize::i16Bit, VReg::v30, 7, Reg::r29), "mov v30.h[7], w29");
  TEST_SINGLE(ins(SubRegSize::i32Bit, VReg::v30, 3, Reg::r29), "mov v30.s[3], w29");
  TEST_SINGLE(ins(SubRegSize::i64Bit, VReg::v30, 1, Reg::r29), "mov v30.d[1], x29");

  TEST_SINGLE(ins(SubRegSize::i8Bit, VReg::v30, 0, VReg::v29, 15), "mov v30.b[0], v29.b[15]");
  TEST_SINGLE(ins(SubRegSize::i16Bit, VReg::v30, 0, VReg::v29, 7), "mov v30.h[0], v29.h[7]");
  TEST_SINGLE(ins(SubRegSize::i32Bit, VReg::v30, 0, VReg::v29, 3), "mov v30.s[0], v29.s[3]");
  TEST_SINGLE(ins(SubRegSize::i64Bit, VReg::v30, 0, VReg::v29, 1), "mov v30.d[0], v29.d[1]");
  TEST_SINGLE(ins(SubRegSize::i8Bit, VReg::v30, 15, VReg::v29, 0), "mov v30.b[15], v29.b[0]");
  TEST_SINGLE(ins(SubRegSize::i16Bit, VReg::v30, 7, VReg::v29, 0), "mov v30.h[7], v29.h[0]");
  TEST_SINGLE(ins(SubRegSize::i32Bit, VReg::v30, 3, VReg::v29, 0), "mov v30.s[3], v29.s[0]");
  TEST_SINGLE(ins(SubRegSize::i64Bit, VReg::v30, 1, VReg::v29, 0), "mov v30.d[1], v29.d[0]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD three same (FP16)") {
  TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnm v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmla v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmulx(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmulx v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmeq(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmeq v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmax(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmax v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(frecps(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "frecps v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminnm(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminnm v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmls v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmin(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmin v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(frsqrts(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "frsqrts v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmaxnmp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnmp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(faddp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "faddp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmul v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmge(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmge v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(facge(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "facge v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmaxp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fdiv(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fdiv v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminnmp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminnmp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fabd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fabd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmgt(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmgt v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(facgt(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "facgt v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminp v30.8h, v29.8h, v28.8h");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD two-register miscellaneous (FP16)") {
  TEST_SINGLE(frintn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frintn v30.8h, v29.8h");
  TEST_SINGLE(frintm(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frintm v30.8h, v29.8h");
  TEST_SINGLE(fcvtns(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtns v30.8h, v29.8h");
  TEST_SINGLE(fcvtms(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtms v30.8h, v29.8h");
  TEST_SINGLE(fcvtas(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtas v30.8h, v29.8h");
  TEST_SINGLE(scvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29), "scvtf v30.8h, v29.8h");
  TEST_SINGLE(fcmgt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcmgt v30.8h, v29.8h, #0.0");
  TEST_SINGLE(fcmeq(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcmeq v30.8h, v29.8h, #0.0");
  TEST_SINGLE(fcmlt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcmlt v30.8h, v29.8h, #0.0");
  TEST_SINGLE(fabs(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fabs v30.8h, v29.8h");
  TEST_SINGLE(frintp(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frintp v30.8h, v29.8h");
  TEST_SINGLE(frintz(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frintz v30.8h, v29.8h");
  TEST_SINGLE(fcvtps(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtps v30.8h, v29.8h");
  TEST_SINGLE(fcvtzs(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtzs v30.8h, v29.8h");
  TEST_SINGLE(frecpe(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frecpe v30.8h, v29.8h");
  TEST_SINGLE(frinta(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frinta v30.8h, v29.8h");
  TEST_SINGLE(frintx(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frintx v30.8h, v29.8h");
  TEST_SINGLE(fcvtnu(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtnu v30.8h, v29.8h");
  TEST_SINGLE(fcvtmu(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtmu v30.8h, v29.8h");
  TEST_SINGLE(fcvtau(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtau v30.8h, v29.8h");
  TEST_SINGLE(ucvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29), "ucvtf v30.8h, v29.8h");
  TEST_SINGLE(fcmge(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcmge v30.8h, v29.8h, #0.0");
  TEST_SINGLE(fcmle(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcmle v30.8h, v29.8h, #0.0");
  TEST_SINGLE(fneg(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fneg v30.8h, v29.8h");
  TEST_SINGLE(frinti(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frinti v30.8h, v29.8h");
  TEST_SINGLE(fcvtpu(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtpu v30.8h, v29.8h");
  TEST_SINGLE(fcvtzu(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtzu v30.8h, v29.8h");
  TEST_SINGLE(frsqrte(SubRegSize::i16Bit, QReg::q30, QReg::q29), "frsqrte v30.8h, v29.8h");
  TEST_SINGLE(fsqrt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fsqrt v30.8h, v29.8h");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD three-register extension") {
  TEST_SINGLE(sdot(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sdot v30.4s, v29.16b, v28.16b");
  TEST_SINGLE(sdot(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sdot v30.2s, v29.8b, v28.8b");

  TEST_SINGLE(usdot(QReg::q30, QReg::q29, QReg::q28), "usdot v30.4s, v29.16b, v28.16b");
  TEST_SINGLE(usdot(DReg::d30, DReg::d29, DReg::d28), "usdot v30.2s, v29.8b, v28.8b");

  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmlah v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmlah v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlah v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlah v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqrdmlah(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlah v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmlsh v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmlsh v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlsh v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlsh v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqrdmlsh(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmlsh v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(udot(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "udot v30.4s, v29.16b, v28.16b");
  TEST_SINGLE(udot(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "udot v30.2s, v29.8b, v28.8b");
  // TEST_SINGLE(udot(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "udot v30.1d, v29.8b, v28.8b");

  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_0), "fcmla v30.16b, v29.16b, v28.16b, #0");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_0), "fcmla v30.8h, v29.8h, v28.8h, #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_0), "fcmla v30.4s, v29.4s, v28.4s, #0");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_0), "fcmla v30.2d, v29.2d, v28.2d, #0");
  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_0), "fcmla v30.8b, v29.8b, v28.8b, #0");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_0), "fcmla v30.4h, v29.4h, v28.4h, #0");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_0), "fcmla v30.2s, v29.2s, v28.2s, #0");
  // TEST_SINGLE(fcmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_0), "fcmla v30.1d, v29.1d, v28.1d, #0");

  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcmla v30.16b, v29.16b, v28.16b, #90");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcmla v30.8h, v29.8h, v28.8h, #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcmla v30.4s, v29.4s, v28.4s, #90");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcmla v30.2d, v29.2d, v28.2d, #90");
  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcmla v30.8b, v29.8b, v28.8b, #90");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcmla v30.4h, v29.4h, v28.4h, #90");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcmla v30.2s, v29.2s, v28.2s, #90");
  // TEST_SINGLE(fcmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcmla v30.1d, v29.1d, v28.1d, #90");

  // Vixl disassembler has a bug that claims 8-bit fcmla exists
  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_180), "fcmla v30.16b, v29.16b, v28.16b, #180");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_180), "fcmla v30.8h, v29.8h, v28.8h, #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_180), "fcmla v30.4s, v29.4s, v28.4s, #180");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_180), "fcmla v30.2d, v29.2d, v28.2d, #180");
  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_180), "fcmla v30.8b, v29.8b, v28.8b, #180");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_180), "fcmla v30.4h, v29.4h, v28.4h, #180");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_180), "fcmla v30.2s, v29.2s, v28.2s, #180");
  // TEST_SINGLE(fcmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_180), "fcmla v30.1d, v29.1d, v28.1d, #180");

  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcmla v30.16b, v29.16b, v28.16b, #270");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcmla v30.8h, v29.8h, v28.8h, #270");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcmla v30.4s, v29.4s, v28.4s, #270");
  TEST_SINGLE(fcmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcmla v30.2d, v29.2d, v28.2d, #270");
  // TEST_SINGLE(fcmla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcmla v30.8b, v29.8b, v28.8b, #270");
  TEST_SINGLE(fcmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcmla v30.4h, v29.4h, v28.4h, #270");
  TEST_SINGLE(fcmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcmla v30.2s, v29.2s, v28.2s, #270");
  // TEST_SINGLE(fcmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcmla v30.1d, v29.1d, v28.1d, #270");

  // Vixl disassembler has a bug that claims 8-bit fcadd exists
  // TEST_SINGLE(fcadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcadd v30.16b, v29.16b, v28.16b, #90");
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcadd v30.8h, v29.8h, v28.8h, #90");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcadd v30.4s, v29.4s, v28.4s, #90");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_90), "fcadd v30.2d, v29.2d, v28.2d, #90");
  // TEST_SINGLE(fcadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcadd v30.8b, v29.8b, v28.8b, #90");
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcadd v30.4h, v29.4h, v28.4h, #90");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcadd v30.2s, v29.2s, v28.2s, #90");
  // TEST_SINGLE(fcadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_90), "fcadd v30.1d, v29.1d, v28.1d, #90");

  // TEST_SINGLE(fcadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcadd v30.16b, v29.16b, v28.16b, #270");
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcadd v30.8h, v29.8h, v28.8h, #270");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcadd v30.4s, v29.4s, v28.4s, #270");
  TEST_SINGLE(fcadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28, Rotation::ROTATE_270), "fcadd v30.2d, v29.2d, v28.2d, #270");
  // TEST_SINGLE(fcadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcadd v30.8b, v29.8b, v28.8b, #270");
  TEST_SINGLE(fcadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcadd v30.4h, v29.4h, v28.4h, #270");
  TEST_SINGLE(fcadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcadd v30.2s, v29.2s, v28.2s, #270");
  // TEST_SINGLE(fcadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28, Rotation::ROTATE_270), "fcadd v30.1d, v29.1d, v28.1d, #270");

  // TODO: Enable once vixl disassembler supports these instructions
  // TEST_SINGLE(bfdot(QReg::q30, QReg::q29, QReg::q28), "bfdot v30.4s, v29.8h, v28.8h");
  // TEST_SINGLE(bfdot(DReg::d30, DReg::d29, DReg::d28), "bfdot v30.2s, v29.4h, v28.4h");
  // TEST_SINGLE(bfmlalb(VReg::v30, VReg::v29, VReg::v28), "bfmlalb v30.4s, v29.8h, v28.8h");
  // TEST_SINGLE(bfmlalt(VReg::v30, VReg::v29, VReg::v28), "bfmlalt v30.4s, v29.8h, v28.8h");

  TEST_SINGLE(smmla(VReg::v30, VReg::v29, VReg::v28), "smmla v30.4s, v29.16b, v28.16b");
  TEST_SINGLE(usmmla(VReg::v30, VReg::v29, VReg::v28), "usmmla v30.4s, v29.16b, v28.16b");
  // TODO: Enable once vixl disassembler supports these instructions
  // TEST_SINGLE(bfmmla(VReg::v30, VReg::v29, VReg::v28), "bfmmla v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(ummla(VReg::v30, VReg::v29, VReg::v28), "ummla v30.4s, v29.16b, v28.16b");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD two-register miscellaneous") {
  // Commented out lines showcase unallocated encodings.
  TEST_SINGLE(rev64(SubRegSize::i8Bit, QReg::q30, QReg::q29), "rev64 v30.16b, v29.16b");
  TEST_SINGLE(rev64(SubRegSize::i16Bit, QReg::q30, QReg::q29), "rev64 v30.8h, v29.8h");
  TEST_SINGLE(rev64(SubRegSize::i32Bit, QReg::q30, QReg::q29), "rev64 v30.4s, v29.4s");
  // TEST_SINGLE(rev64(SubRegSize::i64Bit, QReg::q30, QReg::q29), "rev64 v30.2d, v29.2d");

  TEST_SINGLE(rev64(SubRegSize::i8Bit, DReg::d30, DReg::d29), "rev64 v30.8b, v29.8b");
  TEST_SINGLE(rev64(SubRegSize::i16Bit, DReg::d30, DReg::d29), "rev64 v30.4h, v29.4h");
  TEST_SINGLE(rev64(SubRegSize::i32Bit, DReg::d30, DReg::d29), "rev64 v30.2s, v29.2s");
  // TEST_SINGLE(rev64(SubRegSize::i64Bit, DReg::d30, DReg::d29), "rev64 v30.1d, v29.1d");

  TEST_SINGLE(rev16(SubRegSize::i8Bit, QReg::q30, QReg::q29), "rev16 v30.16b, v29.16b");
  // TEST_SINGLE(rev16(SubRegSize::i16Bit, QReg::q30, QReg::q29), "rev16 v30.8h, v29.8h");
  // TEST_SINGLE(rev16(SubRegSize::i32Bit, QReg::q30, QReg::q29), "rev16 v30.4s, v29.4s");
  // TEST_SINGLE(rev16(SubRegSize::i64Bit, QReg::q30, QReg::q29), "rev16 v30.2d, v29.2d");

  TEST_SINGLE(rev16(SubRegSize::i8Bit, DReg::d30, DReg::d29), "rev16 v30.8b, v29.8b");
  // TEST_SINGLE(rev16(SubRegSize::i16Bit, DReg::d30, DReg::d29), "rev16 v30.4h, v29.4h");
  // TEST_SINGLE(rev16(SubRegSize::i32Bit, DReg::d30, DReg::d29), "rev16 v30.2s, v29.2s");
  // TEST_SINGLE(rev16(SubRegSize::i64Bit, DReg::d30, DReg::d29), "rev16 v30.1d, v29.1d");

  // TEST_SINGLE(saddlp(SubRegSize::i8Bit, QReg::q30, QReg::q29), "saddlp v30.16b, v29.16b");
  TEST_SINGLE(saddlp(SubRegSize::i16Bit, QReg::q30, QReg::q29), "saddlp v30.8h, v29.16b");
  TEST_SINGLE(saddlp(SubRegSize::i32Bit, QReg::q30, QReg::q29), "saddlp v30.4s, v29.8h");
  TEST_SINGLE(saddlp(SubRegSize::i64Bit, QReg::q30, QReg::q29), "saddlp v30.2d, v29.4s");

  // TEST_SINGLE(saddlp(SubRegSize::i8Bit, DReg::d30, DReg::d29), "saddlp v30.8b, v29.8b");
  TEST_SINGLE(saddlp(SubRegSize::i16Bit, DReg::d30, DReg::d29), "saddlp v30.4h, v29.8b");
  TEST_SINGLE(saddlp(SubRegSize::i32Bit, DReg::d30, DReg::d29), "saddlp v30.2s, v29.4h");
  TEST_SINGLE(saddlp(SubRegSize::i64Bit, DReg::d30, DReg::d29), "saddlp v30.1d, v29.2s");

  TEST_SINGLE(suqadd(SubRegSize::i8Bit, QReg::q30, QReg::q29), "suqadd v30.16b, v29.16b");
  TEST_SINGLE(suqadd(SubRegSize::i16Bit, QReg::q30, QReg::q29), "suqadd v30.8h, v29.8h");
  TEST_SINGLE(suqadd(SubRegSize::i32Bit, QReg::q30, QReg::q29), "suqadd v30.4s, v29.4s");
  TEST_SINGLE(suqadd(SubRegSize::i64Bit, QReg::q30, QReg::q29), "suqadd v30.2d, v29.2d");

  TEST_SINGLE(suqadd(SubRegSize::i8Bit, DReg::d30, DReg::d29), "suqadd v30.8b, v29.8b");
  TEST_SINGLE(suqadd(SubRegSize::i16Bit, DReg::d30, DReg::d29), "suqadd v30.4h, v29.4h");
  TEST_SINGLE(suqadd(SubRegSize::i32Bit, DReg::d30, DReg::d29), "suqadd v30.2s, v29.2s");
  // TEST_SINGLE(suqadd(SubRegSize::i64Bit, DReg::d30, DReg::d29), "suqadd v30.1d, v29.1d");

  TEST_SINGLE(cls(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cls v30.16b, v29.16b");
  TEST_SINGLE(cls(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cls v30.8h, v29.8h");
  TEST_SINGLE(cls(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cls v30.4s, v29.4s");
  // TEST_SINGLE(cls(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cls v30.2d, v29.2d");

  TEST_SINGLE(cls(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cls v30.8b, v29.8b");
  TEST_SINGLE(cls(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cls v30.4h, v29.4h");
  TEST_SINGLE(cls(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cls v30.2s, v29.2s");
  // TEST_SINGLE(cls(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cls v30.1d, v29.1d");

  TEST_SINGLE(cnt(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cnt v30.16b, v29.16b");
  // TEST_SINGLE(cnt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cnt v30.8h, v29.8h");
  // TEST_SINGLE(cnt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cnt v30.4s, v29.4s");
  // TEST_SINGLE(cnt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cnt v30.2d, v29.2d");

  TEST_SINGLE(cnt(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cnt v30.8b, v29.8b");
  // TEST_SINGLE(cnt(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cnt v30.4h, v29.4h");
  // TEST_SINGLE(cnt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cnt v30.2s, v29.2s");
  // TEST_SINGLE(cnt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cnt v30.1d, v29.1d");

  // TEST_SINGLE(sadalp(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sadalp v30.16b, v29.16b");
  TEST_SINGLE(sadalp(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sadalp v30.8h, v29.16b");
  TEST_SINGLE(sadalp(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sadalp v30.4s, v29.8h");
  TEST_SINGLE(sadalp(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sadalp v30.2d, v29.4s");

  // TEST_SINGLE(sadalp(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sadalp v30.8b, v29.8b");
  TEST_SINGLE(sadalp(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sadalp v30.4h, v29.8b");
  TEST_SINGLE(sadalp(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sadalp v30.2s, v29.4h");
  TEST_SINGLE(sadalp(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sadalp v30.1d, v29.2s");

  TEST_SINGLE(sqabs(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqabs v30.16b, v29.16b");
  TEST_SINGLE(sqabs(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqabs v30.8h, v29.8h");
  TEST_SINGLE(sqabs(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqabs v30.4s, v29.4s");
  TEST_SINGLE(sqabs(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqabs v30.2d, v29.2d");

  TEST_SINGLE(sqabs(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqabs v30.8b, v29.8b");
  TEST_SINGLE(sqabs(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqabs v30.4h, v29.4h");
  TEST_SINGLE(sqabs(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqabs v30.2s, v29.2s");
  // TEST_SINGLE(sqabs(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqabs v30.1d, v29.1d");

  TEST_SINGLE(cmgt(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cmgt v30.16b, v29.16b, #0");
  TEST_SINGLE(cmgt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cmgt v30.8h, v29.8h, #0");
  TEST_SINGLE(cmgt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cmgt v30.4s, v29.4s, #0");
  TEST_SINGLE(cmgt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cmgt v30.2d, v29.2d, #0");

  TEST_SINGLE(cmgt(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cmgt v30.8b, v29.8b, #0");
  TEST_SINGLE(cmgt(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cmgt v30.4h, v29.4h, #0");
  TEST_SINGLE(cmgt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cmgt v30.2s, v29.2s, #0");
  // TEST_SINGLE(cmgt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cmgt v30.1d, v29.1d, #0");

  TEST_SINGLE(cmeq(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cmeq v30.16b, v29.16b, #0");
  TEST_SINGLE(cmeq(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cmeq v30.8h, v29.8h, #0");
  TEST_SINGLE(cmeq(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cmeq v30.4s, v29.4s, #0");
  TEST_SINGLE(cmeq(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cmeq v30.2d, v29.2d, #0");

  TEST_SINGLE(cmeq(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cmeq v30.8b, v29.8b, #0");
  TEST_SINGLE(cmeq(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cmeq v30.4h, v29.4h, #0");
  TEST_SINGLE(cmeq(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cmeq v30.2s, v29.2s, #0");
  // TEST_SINGLE(cmeq(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cmeq v30.1d, v29.1d, #0");

  TEST_SINGLE(cmlt(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cmlt v30.16b, v29.16b, #0");
  TEST_SINGLE(cmlt(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cmlt v30.8h, v29.8h, #0");
  TEST_SINGLE(cmlt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cmlt v30.4s, v29.4s, #0");
  TEST_SINGLE(cmlt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cmlt v30.2d, v29.2d, #0");

  TEST_SINGLE(cmlt(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cmlt v30.8b, v29.8b, #0");
  TEST_SINGLE(cmlt(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cmlt v30.4h, v29.4h, #0");
  TEST_SINGLE(cmlt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cmlt v30.2s, v29.2s, #0");
  // TEST_SINGLE(cmlt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cmlt v30.1d, v29.1d, #0");

  TEST_SINGLE(abs(SubRegSize::i8Bit, QReg::q30, QReg::q29), "abs v30.16b, v29.16b");
  TEST_SINGLE(abs(SubRegSize::i16Bit, QReg::q30, QReg::q29), "abs v30.8h, v29.8h");
  TEST_SINGLE(abs(SubRegSize::i32Bit, QReg::q30, QReg::q29), "abs v30.4s, v29.4s");
  TEST_SINGLE(abs(SubRegSize::i64Bit, QReg::q30, QReg::q29), "abs v30.2d, v29.2d");

  TEST_SINGLE(abs(SubRegSize::i8Bit, DReg::d30, DReg::d29), "abs v30.8b, v29.8b");
  TEST_SINGLE(abs(SubRegSize::i16Bit, DReg::d30, DReg::d29), "abs v30.4h, v29.4h");
  TEST_SINGLE(abs(SubRegSize::i32Bit, DReg::d30, DReg::d29), "abs v30.2s, v29.2s");
  // TEST_SINGLE(abs(SubRegSize::i64Bit, DReg::d30, DReg::d29), "abs v30.1d, v29.1d");

  TEST_SINGLE(xtn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "xtn v30.8b, v29.8h");
  TEST_SINGLE(xtn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "xtn v30.4h, v29.4s");
  TEST_SINGLE(xtn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "xtn v30.2s, v29.2d");
  // TEST_SINGLE(xtn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "xtn v30.2d, v29.1d");

  TEST_SINGLE(xtn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "xtn v30.8b, v29.8h");
  TEST_SINGLE(xtn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "xtn v30.4h, v29.4s");
  TEST_SINGLE(xtn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "xtn v30.2s, v29.2d");
  // TEST_SINGLE(xtn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "xtn v30.1d, v29.1d");

  TEST_SINGLE(xtn2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "xtn2 v30.16b, v29.8h");
  TEST_SINGLE(xtn2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "xtn2 v30.8h, v29.4s");
  TEST_SINGLE(xtn2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "xtn2 v30.4s, v29.2d");
  // TEST_SINGLE(xtn2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "xtn2 v30.2d, v29.1d");

  TEST_SINGLE(xtn2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "xtn2 v30.16b, v29.8h");
  TEST_SINGLE(xtn2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "xtn2 v30.8h, v29.4s");
  TEST_SINGLE(xtn2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "xtn2 v30.4s, v29.2d");
  // TEST_SINGLE(xtn2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "xtn2 v30.2d, v29.1d");

  TEST_SINGLE(sqxtn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqxtn v30.8b, v29.8h");
  TEST_SINGLE(sqxtn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqxtn v30.4h, v29.4s");
  TEST_SINGLE(sqxtn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqxtn v30.2s, v29.2d");
  // TEST_SINGLE(sqxtn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqxtn v30.2d, v29.1d");

  TEST_SINGLE(sqxtn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqxtn v30.8b, v29.8h");
  TEST_SINGLE(sqxtn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqxtn v30.4h, v29.4s");
  TEST_SINGLE(sqxtn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqxtn v30.2s, v29.2d");
  // TEST_SINGLE(sqxtn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqxtn v30.1d, v29.1d");

  TEST_SINGLE(sqxtn2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqxtn2 v30.16b, v29.8h");
  TEST_SINGLE(sqxtn2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqxtn2 v30.8h, v29.4s");
  TEST_SINGLE(sqxtn2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqxtn2 v30.4s, v29.2d");
  // TEST_SINGLE(sqxtn2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqxtn2 v30.2d, v29.1d");

  TEST_SINGLE(sqxtn2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqxtn2 v30.16b, v29.8h");
  TEST_SINGLE(sqxtn2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqxtn2 v30.8h, v29.4s");
  TEST_SINGLE(sqxtn2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqxtn2 v30.4s, v29.2d");
  // TEST_SINGLE(sqxtn2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqxtn2 v30.2d, v29.1d");

  // TEST_SINGLE(fcvtn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtn v30.8b, v29.8h");
  TEST_SINGLE(fcvtn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtn v30.4h, v29.4s");
  TEST_SINGLE(fcvtn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtn v30.2s, v29.2d");
  // TEST_SINGLE(fcvtn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtn v30.2d, v29.1d");

  // TEST_SINGLE(fcvtn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtn v30.8b, v29.8h");
  TEST_SINGLE(fcvtn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtn v30.4h, v29.4s");
  TEST_SINGLE(fcvtn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtn v30.2s, v29.2d");
  // TEST_SINGLE(fcvtn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtn v30.1d, v29.1d");

  // TEST_SINGLE(fcvtn2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtn2 v30.16b, v29.8h");
  TEST_SINGLE(fcvtn2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtn2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtn2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtn2 v30.4s, v29.2d");
  // TEST_SINGLE(fcvtn2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtn2 v30.2d, v29.1d");

  // TEST_SINGLE(fcvtn2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtn2 v30.16b, v29.8h");
  TEST_SINGLE(fcvtn2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtn2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtn2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtn2 v30.4s, v29.2d");
  // TEST_SINGLE(fcvtn2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtn2 v30.2d, v29.1d");

  // TEST_SINGLE(fcvtl(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtl v30.8b, v29.8h");
  // TEST_SINGLE(fcvtl(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtl v30.4h, v29.4s");
  TEST_SINGLE(fcvtl(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtl v30.4s, v29.4h");
  TEST_SINGLE(fcvtl(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtl v30.2d, v29.2s");

  // TEST_SINGLE(fcvtl(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtl v30.8b, v29.8h");
  // TEST_SINGLE(fcvtl(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtl v30.4h, v29.4s");
  TEST_SINGLE(fcvtl(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtl v30.4s, v29.4h");
  TEST_SINGLE(fcvtl(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtl v30.2d, v29.2s");

  // TEST_SINGLE(fcvtl2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtl2 v30.16b, v29.8h");
  // TEST_SINGLE(fcvtl2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtl2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtl2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtl2 v30.4s, v29.8h");
  TEST_SINGLE(fcvtl2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtl2 v30.2d, v29.4s");

  // TEST_SINGLE(fcvtl2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtl2 v30.16b, v29.8h");
  // TEST_SINGLE(fcvtl2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtl2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtl2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtl2 v30.4s, v29.8h");
  TEST_SINGLE(fcvtl2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtl2 v30.2d, v29.4s");

  TEST_SINGLE(frintn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frintn v30.4s, v29.4s");
  TEST_SINGLE(frintn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frintn v30.2d, v29.2d");
  TEST_SINGLE(frintn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frintn v30.2s, v29.2s");
  // TEST_SINGLE(frintn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frintn v30.1d, v29.1d");

  TEST_SINGLE(frintm(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frintm v30.4s, v29.4s");
  TEST_SINGLE(frintm(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frintm v30.2d, v29.2d");
  TEST_SINGLE(frintm(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frintm v30.2s, v29.2s");
  // TEST_SINGLE(frintm(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frintm v30.1d, v29.1d");

  TEST_SINGLE(fcvtns(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtns v30.4s, v29.4s");
  TEST_SINGLE(fcvtns(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtns v30.2d, v29.2d");
  TEST_SINGLE(fcvtns(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtns v30.2s, v29.2s");
  // TEST_SINGLE(fcvtns(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtns v30.1d, v29.1d");

  TEST_SINGLE(fcvtms(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtms v30.4s, v29.4s");
  TEST_SINGLE(fcvtms(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtms v30.2d, v29.2d");
  TEST_SINGLE(fcvtms(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtms v30.2s, v29.2s");
  // TEST_SINGLE(fcvtms(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtms v30.1d, v29.1d");

  TEST_SINGLE(fcvtas(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtas v30.4s, v29.4s");
  TEST_SINGLE(fcvtas(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtas v30.2d, v29.2d");
  TEST_SINGLE(fcvtas(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtas v30.2s, v29.2s");
  // TEST_SINGLE(fcvtas(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtas v30.1d, v29.1d");

  TEST_SINGLE(scvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29), "scvtf v30.4s, v29.4s");
  TEST_SINGLE(scvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29), "scvtf v30.2d, v29.2d");
  TEST_SINGLE(scvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29), "scvtf v30.2s, v29.2s");
  // TEST_SINGLE(scvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29), "scvtf v30.1d, v29.1d");

  TEST_SINGLE(frint32z(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frint32z v30.4s, v29.4s");
  TEST_SINGLE(frint32z(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frint32z v30.2d, v29.2d");
  TEST_SINGLE(frint32z(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frint32z v30.2s, v29.2s");
  // TEST_SINGLE(frint32z(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frint32z v30.1d, v29.1d");

  TEST_SINGLE(frint64z(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frint64z v30.4s, v29.4s");
  TEST_SINGLE(frint64z(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frint64z v30.2d, v29.2d");
  TEST_SINGLE(frint64z(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frint64z v30.2s, v29.2s");
  // TEST_SINGLE(frint64z(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frint64z v30.1d, v29.1d");

  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcmgt v30.4s, v29.4s, #0.0");
  TEST_SINGLE(fcmgt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcmgt v30.2d, v29.2d, #0.0");
  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcmgt v30.2s, v29.2s, #0.0");
  // TEST_SINGLE(fcmgt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcmgt v30.1d, v29.1d, #0.0");

  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcmeq v30.4s, v29.4s, #0.0");
  TEST_SINGLE(fcmeq(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcmeq v30.2d, v29.2d, #0.0");
  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcmeq v30.2s, v29.2s, #0.0");
  // TEST_SINGLE(fcmeq(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcmeq v30.1d, v29.1d, #0.0");

  TEST_SINGLE(fcmlt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcmlt v30.4s, v29.4s, #0.0");
  TEST_SINGLE(fcmlt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcmlt v30.2d, v29.2d, #0.0");
  TEST_SINGLE(fcmlt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcmlt v30.2s, v29.2s, #0.0");
  // TEST_SINGLE(fcmlt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcmlt v30.1d, v29.1d, #0.0");

  TEST_SINGLE(fabs(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fabs v30.4s, v29.4s");
  TEST_SINGLE(fabs(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fabs v30.2d, v29.2d");
  TEST_SINGLE(fabs(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fabs v30.2s, v29.2s");
  // TEST_SINGLE(fabs(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fabs v30.1d, v29.1d");

  TEST_SINGLE(frintp(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frintp v30.4s, v29.4s");
  TEST_SINGLE(frintp(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frintp v30.2d, v29.2d");
  TEST_SINGLE(frintp(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frintp v30.2s, v29.2s");
  // TEST_SINGLE(frintp(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frintp v30.1d, v29.1d");

  TEST_SINGLE(frintz(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frintz v30.4s, v29.4s");
  TEST_SINGLE(frintz(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frintz v30.2d, v29.2d");
  TEST_SINGLE(frintz(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frintz v30.2s, v29.2s");
  // TEST_SINGLE(frintz(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frintz v30.1d, v29.1d");

  TEST_SINGLE(fcvtps(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtps v30.4s, v29.4s");
  TEST_SINGLE(fcvtps(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtps v30.2d, v29.2d");
  TEST_SINGLE(fcvtps(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtps v30.2s, v29.2s");
  // TEST_SINGLE(fcvtps(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtps v30.1d, v29.1d");

  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtzs v30.4s, v29.4s");
  TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtzs v30.2d, v29.2d");
  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtzs v30.2s, v29.2s");
  // TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtzs v30.1d, v29.1d");

  TEST_SINGLE(urecpe(SubRegSize::i32Bit, QReg::q30, QReg::q29), "urecpe v30.4s, v29.4s");
  // TEST_SINGLE(urecpe(SubRegSize::i64Bit, QReg::q30, QReg::q29), "urecpe v30.2d, v29.2d");
  TEST_SINGLE(urecpe(SubRegSize::i32Bit, DReg::d30, DReg::d29), "urecpe v30.2s, v29.2s");
  // TEST_SINGLE(urecpe(SubRegSize::i64Bit, DReg::d30, DReg::d29), "urecpe v30.1d, v29.1d");

  TEST_SINGLE(frecpe(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frecpe v30.4s, v29.4s");
  TEST_SINGLE(frecpe(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frecpe v30.2d, v29.2d");
  TEST_SINGLE(frecpe(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frecpe v30.2s, v29.2s");
  // TEST_SINGLE(frecpe(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frecpe v30.1d, v29.1d");

  TEST_SINGLE(rev32(SubRegSize::i8Bit, QReg::q30, QReg::q29), "rev32 v30.16b, v29.16b");
  TEST_SINGLE(rev32(SubRegSize::i16Bit, QReg::q30, QReg::q29), "rev32 v30.8h, v29.8h");
  // TEST_SINGLE(rev32(SubRegSize::i32Bit, QReg::q30, QReg::q29), "rev32 v30.4s, v29.4s");
  // TEST_SINGLE(rev32(SubRegSize::i64Bit, QReg::q30, QReg::q29), "rev32 v30.2d, v29.2d");

  TEST_SINGLE(rev32(SubRegSize::i8Bit, DReg::d30, DReg::d29), "rev32 v30.8b, v29.8b");
  TEST_SINGLE(rev32(SubRegSize::i16Bit, DReg::d30, DReg::d29), "rev32 v30.4h, v29.4h");
  // TEST_SINGLE(rev32(SubRegSize::i32Bit, DReg::d30, DReg::d29), "rev32 v30.2s, v29.2s");
  // TEST_SINGLE(rev32(SubRegSize::i64Bit, DReg::d30, DReg::d29), "rev32 v30.1d, v29.1d");

  // TEST_SINGLE(uaddlp(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uaddlp v30.16b, v29.16b");
  TEST_SINGLE(uaddlp(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uaddlp v30.8h, v29.16b");
  TEST_SINGLE(uaddlp(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uaddlp v30.4s, v29.8h");
  TEST_SINGLE(uaddlp(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uaddlp v30.2d, v29.4s");

  // TEST_SINGLE(uaddlp(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uaddlp v30.8b, v29.8b");
  TEST_SINGLE(uaddlp(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uaddlp v30.4h, v29.8b");
  TEST_SINGLE(uaddlp(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uaddlp v30.2s, v29.4h");
  TEST_SINGLE(uaddlp(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uaddlp v30.1d, v29.2s");

  TEST_SINGLE(usqadd(SubRegSize::i8Bit, QReg::q30, QReg::q29), "usqadd v30.16b, v29.16b");
  TEST_SINGLE(usqadd(SubRegSize::i16Bit, QReg::q30, QReg::q29), "usqadd v30.8h, v29.8h");
  TEST_SINGLE(usqadd(SubRegSize::i32Bit, QReg::q30, QReg::q29), "usqadd v30.4s, v29.4s");
  TEST_SINGLE(usqadd(SubRegSize::i64Bit, QReg::q30, QReg::q29), "usqadd v30.2d, v29.2d");

  TEST_SINGLE(usqadd(SubRegSize::i8Bit, DReg::d30, DReg::d29), "usqadd v30.8b, v29.8b");
  TEST_SINGLE(usqadd(SubRegSize::i16Bit, DReg::d30, DReg::d29), "usqadd v30.4h, v29.4h");
  TEST_SINGLE(usqadd(SubRegSize::i32Bit, DReg::d30, DReg::d29), "usqadd v30.2s, v29.2s");
  // TEST_SINGLE(usqadd(SubRegSize::i64Bit, DReg::d30, DReg::d29), "usqadd v30.1d, v29.1d");

  TEST_SINGLE(clz(SubRegSize::i8Bit, QReg::q30, QReg::q29), "clz v30.16b, v29.16b");
  TEST_SINGLE(clz(SubRegSize::i16Bit, QReg::q30, QReg::q29), "clz v30.8h, v29.8h");
  TEST_SINGLE(clz(SubRegSize::i32Bit, QReg::q30, QReg::q29), "clz v30.4s, v29.4s");
  // TEST_SINGLE(clz(SubRegSize::i64Bit, QReg::q30, QReg::q29), "clz v30.2d, v29.2d");

  TEST_SINGLE(clz(SubRegSize::i8Bit, DReg::d30, DReg::d29), "clz v30.8b, v29.8b");
  TEST_SINGLE(clz(SubRegSize::i16Bit, DReg::d30, DReg::d29), "clz v30.4h, v29.4h");
  TEST_SINGLE(clz(SubRegSize::i32Bit, DReg::d30, DReg::d29), "clz v30.2s, v29.2s");
  // TEST_SINGLE(clz(SubRegSize::i64Bit, DReg::d30, DReg::d29), "clz v30.1d, v29.1d");

  // TEST_SINGLE(uadalp(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uadalp v30.16b, v29.16b");
  TEST_SINGLE(uadalp(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uadalp v30.8h, v29.16b");
  TEST_SINGLE(uadalp(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uadalp v30.4s, v29.8h");
  TEST_SINGLE(uadalp(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uadalp v30.2d, v29.4s");

  // TEST_SINGLE(uadalp(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uadalp v30.8b, v29.8b");
  TEST_SINGLE(uadalp(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uadalp v30.4h, v29.8b");
  TEST_SINGLE(uadalp(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uadalp v30.2s, v29.4h");
  TEST_SINGLE(uadalp(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uadalp v30.1d, v29.2s");

  TEST_SINGLE(sqneg(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqneg v30.16b, v29.16b");
  TEST_SINGLE(sqneg(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqneg v30.8h, v29.8h");
  TEST_SINGLE(sqneg(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqneg v30.4s, v29.4s");
  TEST_SINGLE(sqneg(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqneg v30.2d, v29.2d");

  TEST_SINGLE(sqneg(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqneg v30.8b, v29.8b");
  TEST_SINGLE(sqneg(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqneg v30.4h, v29.4h");
  TEST_SINGLE(sqneg(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqneg v30.2s, v29.2s");
  // TEST_SINGLE(sqneg(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqneg v30.1d, v29.1d");

  TEST_SINGLE(cmge(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cmge v30.16b, v29.16b, #0");
  TEST_SINGLE(cmge(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cmge v30.8h, v29.8h, #0");
  TEST_SINGLE(cmge(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cmge v30.4s, v29.4s, #0");
  TEST_SINGLE(cmge(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cmge v30.2d, v29.2d, #0");

  TEST_SINGLE(cmge(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cmge v30.8b, v29.8b, #0");
  TEST_SINGLE(cmge(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cmge v30.4h, v29.4h, #0");
  TEST_SINGLE(cmge(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cmge v30.2s, v29.2s, #0");
  // TEST_SINGLE(cmge(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cmge v30.1d, v29.1d, #0");
  //
  TEST_SINGLE(cmle(SubRegSize::i8Bit, QReg::q30, QReg::q29), "cmle v30.16b, v29.16b, #0");
  TEST_SINGLE(cmle(SubRegSize::i16Bit, QReg::q30, QReg::q29), "cmle v30.8h, v29.8h, #0");
  TEST_SINGLE(cmle(SubRegSize::i32Bit, QReg::q30, QReg::q29), "cmle v30.4s, v29.4s, #0");
  TEST_SINGLE(cmle(SubRegSize::i64Bit, QReg::q30, QReg::q29), "cmle v30.2d, v29.2d, #0");

  TEST_SINGLE(cmle(SubRegSize::i8Bit, DReg::d30, DReg::d29), "cmle v30.8b, v29.8b, #0");
  TEST_SINGLE(cmle(SubRegSize::i16Bit, DReg::d30, DReg::d29), "cmle v30.4h, v29.4h, #0");
  TEST_SINGLE(cmle(SubRegSize::i32Bit, DReg::d30, DReg::d29), "cmle v30.2s, v29.2s, #0");
  // TEST_SINGLE(cmle(SubRegSize::i64Bit, DReg::d30, DReg::d29), "cmle v30.1d, v29.1d, #0");

  TEST_SINGLE(neg(SubRegSize::i8Bit, QReg::q30, QReg::q29), "neg v30.16b, v29.16b");
  TEST_SINGLE(neg(SubRegSize::i16Bit, QReg::q30, QReg::q29), "neg v30.8h, v29.8h");
  TEST_SINGLE(neg(SubRegSize::i32Bit, QReg::q30, QReg::q29), "neg v30.4s, v29.4s");
  TEST_SINGLE(neg(SubRegSize::i64Bit, QReg::q30, QReg::q29), "neg v30.2d, v29.2d");

  TEST_SINGLE(neg(SubRegSize::i8Bit, DReg::d30, DReg::d29), "neg v30.8b, v29.8b");
  TEST_SINGLE(neg(SubRegSize::i16Bit, DReg::d30, DReg::d29), "neg v30.4h, v29.4h");
  TEST_SINGLE(neg(SubRegSize::i32Bit, DReg::d30, DReg::d29), "neg v30.2s, v29.2s");
  // TEST_SINGLE(neg(SubRegSize::i64Bit, DReg::d30, DReg::d29), "neg v30.1d, v29.1d");

  TEST_SINGLE(sqxtun(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqxtun v30.8b, v29.8h");
  TEST_SINGLE(sqxtun(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqxtun v30.4h, v29.4s");
  TEST_SINGLE(sqxtun(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqxtun v30.2s, v29.2d");
  // TEST_SINGLE(sqxtun(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqxtun v30.2d, v29.1d");

  TEST_SINGLE(sqxtun(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqxtun v30.8b, v29.8h");
  TEST_SINGLE(sqxtun(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqxtun v30.4h, v29.4s");
  TEST_SINGLE(sqxtun(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqxtun v30.2s, v29.2d");
  // TEST_SINGLE(sqxtun(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqxtun v30.1d, v29.1d");

  TEST_SINGLE(sqxtun2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sqxtun2 v30.16b, v29.8h");
  TEST_SINGLE(sqxtun2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sqxtun2 v30.8h, v29.4s");
  TEST_SINGLE(sqxtun2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sqxtun2 v30.4s, v29.2d");
  // TEST_SINGLE(sqxtun2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sqxtun2 v30.2d, v29.1d");

  TEST_SINGLE(sqxtun2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sqxtun2 v30.16b, v29.8h");
  TEST_SINGLE(sqxtun2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sqxtun2 v30.8h, v29.4s");
  TEST_SINGLE(sqxtun2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sqxtun2 v30.4s, v29.2d");
  // TEST_SINGLE(sqxtun2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sqxtun2 v30.2d, v29.1d");

  // TEST_SINGLE(shll(SubRegSize::i8Bit, DReg::d30, DReg::d29), "shll v30.8b, v29.8b, #0");
  TEST_SINGLE(shll(SubRegSize::i16Bit, DReg::d30, DReg::d29), "shll v30.8h, v29.8b, #8");
  TEST_SINGLE(shll(SubRegSize::i32Bit, DReg::d30, DReg::d29), "shll v30.4s, v29.4h, #16");
  TEST_SINGLE(shll(SubRegSize::i64Bit, DReg::d30, DReg::d29), "shll v30.2d, v29.2s, #32");

  // TEST_SINGLE(shll2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "shll2 v30.16b, v29.16b, #0");
  TEST_SINGLE(shll2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "shll2 v30.8h, v29.16b, #8");
  TEST_SINGLE(shll2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "shll2 v30.4s, v29.8h, #16");
  TEST_SINGLE(shll2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "shll2 v30.2d, v29.4s, #32");

  TEST_SINGLE(uqxtn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uqxtn v30.8b, v29.8h");
  TEST_SINGLE(uqxtn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uqxtn v30.4h, v29.4s");
  TEST_SINGLE(uqxtn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uqxtn v30.2s, v29.2d");
  // TEST_SINGLE(uqxtn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uqxtn v30.2d, v29.1d");

  TEST_SINGLE(uqxtn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uqxtn v30.8b, v29.8h");
  TEST_SINGLE(uqxtn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uqxtn v30.4h, v29.4s");
  TEST_SINGLE(uqxtn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uqxtn v30.2s, v29.2d");
  // TEST_SINGLE(uqxtn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uqxtn v30.1d, v29.1d");

  TEST_SINGLE(uqxtn2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uqxtn2 v30.16b, v29.8h");
  TEST_SINGLE(uqxtn2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uqxtn2 v30.8h, v29.4s");
  TEST_SINGLE(uqxtn2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uqxtn2 v30.4s, v29.2d");
  // TEST_SINGLE(uqxtn2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uqxtn2 v30.2d, v29.1d");

  TEST_SINGLE(uqxtn2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uqxtn2 v30.16b, v29.8h");
  TEST_SINGLE(uqxtn2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uqxtn2 v30.8h, v29.4s");
  TEST_SINGLE(uqxtn2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uqxtn2 v30.4s, v29.2d");
  // TEST_SINGLE(uqxtn2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uqxtn2 v30.2d, v29.1d");
  //
  // TEST_SINGLE(fcvtxn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtxn v30.8b, v29.8h");
  // TEST_SINGLE(fcvtxn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtxn v30.4h, v29.4s");
  TEST_SINGLE(fcvtxn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtxn v30.2s, v29.2d");
  // TEST_SINGLE(fcvtxn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtxn v30.2d, v29.1d");

  // TEST_SINGLE(fcvtxn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtxn v30.8b, v29.8h");
  // TEST_SINGLE(fcvtxn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtxn v30.4h, v29.4s");
  TEST_SINGLE(fcvtxn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtxn v30.2s, v29.2d");
  // TEST_SINGLE(fcvtxn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtxn v30.1d, v29.1d");

  // TEST_SINGLE(fcvtxn2(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fcvtxn2 v30.16b, v29.8h");
  // TEST_SINGLE(fcvtxn2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fcvtxn2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtxn2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtxn2 v30.4s, v29.2d");
  // TEST_SINGLE(fcvtxn2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtxn2 v30.2d, v29.1d");

  // TEST_SINGLE(fcvtxn2(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fcvtxn2 v30.16b, v29.8h");
  // TEST_SINGLE(fcvtxn2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fcvtxn2 v30.8h, v29.4s");
  TEST_SINGLE(fcvtxn2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtxn2 v30.4s, v29.2d");
  // TEST_SINGLE(fcvtxn2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtxn2 v30.2d, v29.1d");


  TEST_SINGLE(frinta(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frinta v30.4s, v29.4s");
  TEST_SINGLE(frinta(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frinta v30.2d, v29.2d");
  TEST_SINGLE(frinta(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frinta v30.2s, v29.2s");
  // TEST_SINGLE(frinta(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frinta v30.1d, v29.1d");

  TEST_SINGLE(frintx(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frintx v30.4s, v29.4s");
  TEST_SINGLE(frintx(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frintx v30.2d, v29.2d");
  TEST_SINGLE(frintx(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frintx v30.2s, v29.2s");
  // TEST_SINGLE(frintx(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frintx v30.1d, v29.1d");

  TEST_SINGLE(fcvtnu(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtnu v30.4s, v29.4s");
  TEST_SINGLE(fcvtnu(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtnu v30.2d, v29.2d");
  TEST_SINGLE(fcvtnu(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtnu v30.2s, v29.2s");
  // TEST_SINGLE(fcvtnu(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtnu v30.1d, v29.1d");

  TEST_SINGLE(fcvtmu(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtmu v30.4s, v29.4s");
  TEST_SINGLE(fcvtmu(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtmu v30.2d, v29.2d");
  TEST_SINGLE(fcvtmu(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtmu v30.2s, v29.2s");
  // TEST_SINGLE(fcvtmu(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtmu v30.1d, v29.1d");

  TEST_SINGLE(fcvtau(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtau v30.4s, v29.4s");
  TEST_SINGLE(fcvtau(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtau v30.2d, v29.2d");
  TEST_SINGLE(fcvtau(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtau v30.2s, v29.2s");
  // TEST_SINGLE(fcvtau(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtau v30.1d, v29.1d");

  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29), "ucvtf v30.4s, v29.4s");
  TEST_SINGLE(ucvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29), "ucvtf v30.2d, v29.2d");
  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29), "ucvtf v30.2s, v29.2s");
  // TEST_SINGLE(ucvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29), "ucvtf v30.1d, v29.1d");

  TEST_SINGLE(frint32x(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frint32x v30.4s, v29.4s");
  TEST_SINGLE(frint32x(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frint32x v30.2d, v29.2d");
  TEST_SINGLE(frint32x(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frint32x v30.2s, v29.2s");
  // TEST_SINGLE(frint32x(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frint32x v30.1d, v29.1d");

  TEST_SINGLE(frint64x(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frint64x v30.4s, v29.4s");
  TEST_SINGLE(frint64x(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frint64x v30.2d, v29.2d");
  TEST_SINGLE(frint64x(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frint64x v30.2s, v29.2s");
  // TEST_SINGLE(frint64x(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frint64x v30.1d, v29.1d");

  TEST_SINGLE(not_(SubRegSize::i8Bit, QReg::q30, QReg::q29), "mvn v30.16b, v29.16b");
  // TEST_SINGLE(not_(SubRegSize::i16Bit, QReg::q30, QReg::q29), "not v30.8h, v29.8h");
  // TEST_SINGLE(not_(SubRegSize::i32Bit, QReg::q30, QReg::q29), "not v30.4s, v29.4s");
  // TEST_SINGLE(not_(SubRegSize::i64Bit, QReg::q30, QReg::q29), "not v30.2d, v29.2d");

  TEST_SINGLE(not_(SubRegSize::i8Bit, DReg::d30, DReg::d29), "mvn v30.8b, v29.8b");
  // TEST_SINGLE(not_(SubRegSize::i16Bit, DReg::d30, DReg::d29), "not v30.4h, v29.4h");
  // TEST_SINGLE(not_(SubRegSize::i32Bit, DReg::d30, DReg::d29), "not v30.2s, v29.2s");
  // TEST_SINGLE(not_(SubRegSize::i64Bit, DReg::d30, DReg::d29), "not v30.1d, v29.1d");

  TEST_SINGLE(mvn(SubRegSize::i8Bit, QReg::q30, QReg::q29), "mvn v30.16b, v29.16b");
  // TEST_SINGLE(mvn(SubRegSize::i16Bit, QReg::q30, QReg::q29), "mvn v30.8h, v29.8h");
  // TEST_SINGLE(mvn(SubRegSize::i32Bit, QReg::q30, QReg::q29), "mvn v30.4s, v29.4s");
  // TEST_SINGLE(mvn(SubRegSize::i64Bit, QReg::q30, QReg::q29), "mvn v30.2d, v29.2d");

  TEST_SINGLE(mvn(SubRegSize::i8Bit, DReg::d30, DReg::d29), "mvn v30.8b, v29.8b");
  // TEST_SINGLE(mvn(SubRegSize::i16Bit, DReg::d30, DReg::d29), "mvn v30.4h, v29.4h");
  // TEST_SINGLE(mvn(SubRegSize::i32Bit, DReg::d30, DReg::d29), "mvn v30.2s, v29.2s");
  // TEST_SINGLE(mvn(SubRegSize::i64Bit, DReg::d30, DReg::d29), "mvn v30.1d, v29.1d");

  TEST_SINGLE(rbit(SubRegSize::i8Bit, QReg::q30, QReg::q29), "rbit v30.16b, v29.16b");
  // TEST_SINGLE(rbit(SubRegSize::i16Bit, QReg::q30, QReg::q29), "rbit v30.8h, v29.8h");
  // TEST_SINGLE(rbit(SubRegSize::i32Bit, QReg::q30, QReg::q29), "rbit v30.4s, v29.4s");
  // TEST_SINGLE(rbit(SubRegSize::i64Bit, QReg::q30, QReg::q29), "rbit v30.2d, v29.2d");

  TEST_SINGLE(rbit(SubRegSize::i8Bit, DReg::d30, DReg::d29), "rbit v30.8b, v29.8b");
  // TEST_SINGLE(rbit(SubRegSize::i16Bit, DReg::d30, DReg::d29), "rbit v30.4h, v29.4h");
  // TEST_SINGLE(rbit(SubRegSize::i32Bit, DReg::d30, DReg::d29), "rbit v30.2s, v29.2s");
  // TEST_SINGLE(rbit(SubRegSize::i64Bit, DReg::d30, DReg::d29), "rbit v30.1d, v29.1d");

  TEST_SINGLE(fcmge(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcmge v30.4s, v29.4s, #0.0");
  TEST_SINGLE(fcmge(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcmge v30.2d, v29.2d, #0.0");
  TEST_SINGLE(fcmge(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcmge v30.2s, v29.2s, #0.0");
  // TEST_SINGLE(fcmge(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcmge v30.1d, v29.1d, #0.0");

  TEST_SINGLE(fcmle(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcmle v30.4s, v29.4s, #0.0");
  TEST_SINGLE(fcmle(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcmle v30.2d, v29.2d, #0.0");
  TEST_SINGLE(fcmle(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcmle v30.2s, v29.2s, #0.0");
  // TEST_SINGLE(fcmle(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcmle v30.1d, v29.1d, #0.0");

  TEST_SINGLE(fneg(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fneg v30.4s, v29.4s");
  TEST_SINGLE(fneg(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fneg v30.2d, v29.2d");
  TEST_SINGLE(fneg(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fneg v30.2s, v29.2s");
  // TEST_SINGLE(fneg(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fneg v30.1d, v29.1d");

  TEST_SINGLE(frinti(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frinti v30.4s, v29.4s");
  TEST_SINGLE(frinti(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frinti v30.2d, v29.2d");
  TEST_SINGLE(frinti(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frinti v30.2s, v29.2s");
  // TEST_SINGLE(frinti(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frinti v30.1d, v29.1d");

  TEST_SINGLE(fcvtpu(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtpu v30.4s, v29.4s");
  TEST_SINGLE(fcvtpu(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtpu v30.2d, v29.2d");
  TEST_SINGLE(fcvtpu(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtpu v30.2s, v29.2s");
  // TEST_SINGLE(fcvtpu(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtpu v30.1d, v29.1d");

  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fcvtzu v30.4s, v29.4s");
  TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fcvtzu v30.2d, v29.2d");
  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fcvtzu v30.2s, v29.2s");
  // TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fcvtzu v30.1d, v29.1d");

  TEST_SINGLE(ursqrte(SubRegSize::i32Bit, QReg::q30, QReg::q29), "ursqrte v30.4s, v29.4s");
  // TEST_SINGLE(ursqrte(SubRegSize::i64Bit, QReg::q30, QReg::q29), "ursqrte v30.2d, v29.2d");
  TEST_SINGLE(ursqrte(SubRegSize::i32Bit, DReg::d30, DReg::d29), "ursqrte v30.2s, v29.2s");
  // TEST_SINGLE(ursqrte(SubRegSize::i64Bit, DReg::d30, DReg::d29), "ursqrte v30.1d, v29.1d");

  TEST_SINGLE(frsqrte(SubRegSize::i32Bit, QReg::q30, QReg::q29), "frsqrte v30.4s, v29.4s");
  TEST_SINGLE(frsqrte(SubRegSize::i64Bit, QReg::q30, QReg::q29), "frsqrte v30.2d, v29.2d");
  TEST_SINGLE(frsqrte(SubRegSize::i32Bit, DReg::d30, DReg::d29), "frsqrte v30.2s, v29.2s");
  // TEST_SINGLE(frsqrte(SubRegSize::i64Bit, DReg::d30, DReg::d29), "frsqrte v30.1d, v29.1d");

  TEST_SINGLE(fsqrt(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fsqrt v30.4s, v29.4s");
  TEST_SINGLE(fsqrt(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fsqrt v30.2d, v29.2d");
  TEST_SINGLE(fsqrt(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fsqrt v30.2s, v29.2s");
  // TEST_SINGLE(fsqrt(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fsqrt v30.1d, v29.1d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD across lanes") {
  // TEST_SINGLE(saddlv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "saddlv v30.16b, v29.16b");
  TEST_SINGLE(saddlv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "saddlv h30, v29.16b");
  TEST_SINGLE(saddlv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "saddlv s30, v29.8h");
  TEST_SINGLE(saddlv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "saddlv d30, v29.4s");

  // TEST_SINGLE(saddlv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "saddlv v30.8b, v29.8b");
  TEST_SINGLE(saddlv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "saddlv h30, v29.8b");
  TEST_SINGLE(saddlv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "saddlv s30, v29.4h");
  // TEST_SINGLE(saddlv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "saddlv d30, v29.1d");

  TEST_SINGLE(smaxv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "smaxv b30, v29.16b");
  TEST_SINGLE(smaxv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "smaxv h30, v29.8h");
  TEST_SINGLE(smaxv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "smaxv s30, v29.4s");
  // TEST_SINGLE(smaxv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "smaxv d30, v29.4s");

  TEST_SINGLE(smaxv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "smaxv b30, v29.8b");
  TEST_SINGLE(smaxv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "smaxv h30, v29.4h");
  // TEST_SINGLE(smaxv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "smaxv s30, v29.2s");
  // TEST_SINGLE(smaxv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "smaxv d30, v29.1d");

  TEST_SINGLE(sminv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "sminv b30, v29.16b");
  TEST_SINGLE(sminv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sminv h30, v29.8h");
  TEST_SINGLE(sminv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sminv s30, v29.4s");
  // TEST_SINGLE(sminv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sminv d30, v29.4s");

  TEST_SINGLE(sminv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "sminv b30, v29.8b");
  TEST_SINGLE(sminv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sminv h30, v29.4h");
  // TEST_SINGLE(sminv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sminv s30, v29.2s");
  // TEST_SINGLE(sminv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sminv d30, v29.1d");

  TEST_SINGLE(addv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "addv b30, v29.16b");
  TEST_SINGLE(addv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "addv h30, v29.8h");
  TEST_SINGLE(addv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "addv s30, v29.4s");
  // TEST_SINGLE(addv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "addv d30, v29.4s");

  TEST_SINGLE(addv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "addv b30, v29.8b");
  TEST_SINGLE(addv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "addv h30, v29.4h");
  // TEST_SINGLE(addv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "addv s30, v29.2s");
  // TEST_SINGLE(addv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "addv d30, v29.1d");

  // TEST_SINGLE(uaddlv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uaddlv v30.16b, v29.16b");
  TEST_SINGLE(uaddlv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uaddlv h30, v29.16b");
  TEST_SINGLE(uaddlv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uaddlv s30, v29.8h");
  TEST_SINGLE(uaddlv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uaddlv d30, v29.4s");

  // TEST_SINGLE(uaddlv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uaddlv v30.8b, v29.8b");
  TEST_SINGLE(uaddlv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uaddlv h30, v29.8b");
  TEST_SINGLE(uaddlv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uaddlv s30, v29.4h");
  // TEST_SINGLE(uaddlv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uaddlv d30, v29.1d");

  TEST_SINGLE(umaxv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "umaxv b30, v29.16b");
  TEST_SINGLE(umaxv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "umaxv h30, v29.8h");
  TEST_SINGLE(umaxv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "umaxv s30, v29.4s");
  // TEST_SINGLE(umaxv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "umaxv d30, v29.4s");

  TEST_SINGLE(umaxv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "umaxv b30, v29.8b");
  TEST_SINGLE(umaxv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "umaxv h30, v29.4h");
  // TEST_SINGLE(umaxv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "umaxv s30, v29.2s");
  // TEST_SINGLE(umaxv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "umaxv d30, v29.1d");

  TEST_SINGLE(uminv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "uminv b30, v29.16b");
  TEST_SINGLE(uminv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uminv h30, v29.8h");
  TEST_SINGLE(uminv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uminv s30, v29.4s");
  // TEST_SINGLE(uminv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uminv d30, v29.4s");

  TEST_SINGLE(uminv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "uminv b30, v29.8b");
  TEST_SINGLE(uminv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uminv h30, v29.4h");
  // TEST_SINGLE(uminv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uminv s30, v29.2s");
  // TEST_SINGLE(uminv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uminv d30, v29.1d");

  // TEST_SINGLE(fmaxnmv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fmaxnmv b30, v29.16b");
  TEST_SINGLE(fmaxnmv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fmaxnmv h30, v29.8h");
  TEST_SINGLE(fmaxnmv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fmaxnmv s30, v29.4s");
  // TEST_SINGLE(fmaxnmv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fmaxnmv d30, v29.4s");

  // TEST_SINGLE(fmaxnmv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fmaxnmv b30, v29.8b");
  TEST_SINGLE(fmaxnmv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fmaxnmv h30, v29.4h");
  // TEST_SINGLE(fmaxnmv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fmaxnmv s30, v29.2s");
  // TEST_SINGLE(fmaxnmv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fmaxnmv d30, v29.1d");

  // TEST_SINGLE(fmaxv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fmaxv b30, v29.16b");
  TEST_SINGLE(fmaxv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fmaxv h30, v29.8h");
  TEST_SINGLE(fmaxv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fmaxv s30, v29.4s");
  // TEST_SINGLE(fmaxv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fmaxv d30, v29.4s");

  // TEST_SINGLE(fmaxv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fmaxv b30, v29.8b");
  TEST_SINGLE(fmaxv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fmaxv h30, v29.4h");
  // TEST_SINGLE(fmaxv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fmaxv s30, v29.2s");
  // TEST_SINGLE(fmaxv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fmaxv d30, v29.1d");

  // TEST_SINGLE(fminnmv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fminnmv b30, v29.16b");
  TEST_SINGLE(fminnmv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fminnmv h30, v29.8h");
  TEST_SINGLE(fminnmv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fminnmv s30, v29.4s");
  // TEST_SINGLE(fminnmv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fminnmv d30, v29.4s");

  // TEST_SINGLE(fminnmv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fminnmv b30, v29.8b");
  TEST_SINGLE(fminnmv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fminnmv h30, v29.4h");
  // TEST_SINGLE(fminnmv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fminnmv s30, v29.2s");
  // TEST_SINGLE(fminnmv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fminnmv d30, v29.1d");

  // TEST_SINGLE(fminv(SubRegSize::i8Bit, QReg::q30, QReg::q29), "fminv b30, v29.16b");
  TEST_SINGLE(fminv(SubRegSize::i16Bit, QReg::q30, QReg::q29), "fminv h30, v29.8h");
  TEST_SINGLE(fminv(SubRegSize::i32Bit, QReg::q30, QReg::q29), "fminv s30, v29.4s");
  // TEST_SINGLE(fminv(SubRegSize::i64Bit, QReg::q30, QReg::q29), "fminv d30, v29.4s");

  // TEST_SINGLE(fminv(SubRegSize::i8Bit, DReg::d30, DReg::d29), "fminv b30, v29.8b");
  TEST_SINGLE(fminv(SubRegSize::i16Bit, DReg::d30, DReg::d29), "fminv h30, v29.4h");
  // TEST_SINGLE(fminv(SubRegSize::i32Bit, DReg::d30, DReg::d29), "fminv s30, v29.2s");
  // TEST_SINGLE(fminv(SubRegSize::i64Bit, DReg::d30, DReg::d29), "fminv d30, v29.1d");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD three different") {
  // TEST_SINGLE(saddl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "saddl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(saddl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "saddl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(saddl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "saddl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(saddl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "saddl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(saddl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "saddl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(saddl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "saddl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(saddl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "saddl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(saddl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "saddl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(saddw(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "saddw v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(saddw(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "saddw v30.8h, v29.8h, v28.8b");
  TEST_SINGLE(saddw(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "saddw v30.4s, v29.4s, v28.4h");
  TEST_SINGLE(saddw(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "saddw v30.2d, v29.2d, v28.2s");

  // TEST_SINGLE(saddw2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "saddw2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(saddw2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "saddw2 v30.8h, v29.8h, v28.16b");
  TEST_SINGLE(saddw2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "saddw2 v30.4s, v29.4s, v28.8h");
  TEST_SINGLE(saddw2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "saddw2 v30.2d, v29.2d, v28.4s");

  // TEST_SINGLE(ssubl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "ssubl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(ssubl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "ssubl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(ssubl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "ssubl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(ssubl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "ssubl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(ssubl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "ssubl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(ssubl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "ssubl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(ssubl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "ssubl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(ssubl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "ssubl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(ssubw(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "ssubw v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(ssubw(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "ssubw v30.8h, v29.8h, v28.8b");
  TEST_SINGLE(ssubw(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "ssubw v30.4s, v29.4s, v28.4h");
  TEST_SINGLE(ssubw(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "ssubw v30.2d, v29.2d, v28.2s");

  // TEST_SINGLE(ssubw2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "ssubw2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(ssubw2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "ssubw2 v30.8h, v29.8h, v28.16b");
  TEST_SINGLE(ssubw2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "ssubw2 v30.4s, v29.4s, v28.8h");
  TEST_SINGLE(ssubw2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "ssubw2 v30.2d, v29.2d, v28.4s");

  TEST_SINGLE(addhn(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "addhn v30.8b, v29.8h, v28.8h");
  TEST_SINGLE(addhn(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "addhn v30.4h, v29.4s, v28.4s");
  TEST_SINGLE(addhn(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "addhn v30.2s, v29.2d, v28.2d");
  // TEST_SINGLE(addhn(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "addhn v30.2d, v29.2d, v28.2s");

  TEST_SINGLE(addhn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "addhn2 v30.16b, v29.8h, v28.8h");
  TEST_SINGLE(addhn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "addhn2 v30.8h, v29.4s, v28.4s");
  TEST_SINGLE(addhn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "addhn2 v30.4s, v29.2d, v28.2d");
  // TEST_SINGLE(addhn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "addhn2 v30.2d, v29.2d, v28.4s");

  // TEST_SINGLE(sabal(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sabal v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sabal(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sabal v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(sabal(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sabal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(sabal(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sabal v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(sabal2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sabal2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sabal2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sabal2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(sabal2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sabal2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(sabal2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sabal2 v30.2d, v29.4s, v28.4s");

  TEST_SINGLE(subhn(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "subhn v30.8b, v29.8h, v28.8h");
  TEST_SINGLE(subhn(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "subhn v30.4h, v29.4s, v28.4s");
  TEST_SINGLE(subhn(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "subhn v30.2s, v29.2d, v28.2d");
  // TEST_SINGLE(subhn(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "subhn v30.2d, v29.2d, v28.2s");

  TEST_SINGLE(subhn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "subhn2 v30.16b, v29.8h, v28.8h");
  TEST_SINGLE(subhn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "subhn2 v30.8h, v29.4s, v28.4s");
  TEST_SINGLE(subhn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "subhn2 v30.4s, v29.2d, v28.2d");
  // TEST_SINGLE(subhn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "subhn2 v30.2d, v29.2d, v28.4s");

  // TEST_SINGLE(sabdl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sabdl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sabdl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sabdl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(sabdl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sabdl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(sabdl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sabdl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(sabdl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sabdl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sabdl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sabdl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(sabdl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sabdl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(sabdl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sabdl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(smlal(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smlal v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smlal(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smlal v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(smlal(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smlal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(smlal(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smlal v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(smlal2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smlal2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smlal2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smlal2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(smlal2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smlal2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(smlal2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smlal2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(sqdmlal(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlal v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmlal(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlal v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(sqdmlal(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(sqdmlal(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlal v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(sqdmlal2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlal2 v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmlal2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlal2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(sqdmlal2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlal2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(sqdmlal2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlal2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(smlsl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smlsl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smlsl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smlsl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(smlsl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smlsl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(smlsl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smlsl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(smlsl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smlsl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smlsl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smlsl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(smlsl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smlsl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(smlsl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smlsl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(sqdmlsl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlsl v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmlsl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlsl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(sqdmlsl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlsl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(sqdmlsl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmlsl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(sqdmlsl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlsl2 v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmlsl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlsl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(sqdmlsl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlsl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(sqdmlsl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmlsl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(smull(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smull v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smull(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smull v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(smull(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smull v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(smull(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smull v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(smull2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smull2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smull2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smull2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(smull2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smull2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(smull2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smull2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(sqdmull(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmull v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmull(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmull v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(sqdmull(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmull v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(sqdmull(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmull v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(sqdmull2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmull2 v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(sqdmull2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmull2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(sqdmull2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmull2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(sqdmull2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmull2 v30.2d, v29.4s, v28.4s");

  TEST_SINGLE(pmull(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "pmull v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(pmull(SubRegSize::i128Bit, DReg::d30, DReg::d29, DReg::d28), "pmull v30.1q, v29.1d, v28.1d");

  TEST_SINGLE(pmull2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "pmull2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(pmull2(SubRegSize::i128Bit, QReg::q30, QReg::q29, QReg::q28), "pmull2 v30.1q, v29.2d, v28.2d");

  // TEST_SINGLE(uaddl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uaddl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uaddl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uaddl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(uaddl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uaddl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(uaddl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uaddl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(uaddl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uaddl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uaddl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uaddl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(uaddl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uaddl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(uaddl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uaddl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(uaddw(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uaddw v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uaddw(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uaddw v30.8h, v29.8h, v28.8b");
  TEST_SINGLE(uaddw(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uaddw v30.4s, v29.4s, v28.4h");
  TEST_SINGLE(uaddw(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uaddw v30.2d, v29.2d, v28.2s");

  // TEST_SINGLE(uaddw2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uaddw2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uaddw2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uaddw2 v30.8h, v29.8h, v28.16b");
  TEST_SINGLE(uaddw2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uaddw2 v30.4s, v29.4s, v28.8h");
  TEST_SINGLE(uaddw2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uaddw2 v30.2d, v29.2d, v28.4s");

  // TEST_SINGLE(usubl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "usubl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(usubl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "usubl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(usubl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "usubl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(usubl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "usubl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(usubl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "usubl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(usubl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "usubl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(usubl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "usubl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(usubl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "usubl2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(usubw(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "usubw v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(usubw(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "usubw v30.8h, v29.8h, v28.8b");
  TEST_SINGLE(usubw(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "usubw v30.4s, v29.4s, v28.4h");
  TEST_SINGLE(usubw(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "usubw v30.2d, v29.2d, v28.2s");

  // TEST_SINGLE(usubw2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "usubw2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(usubw2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "usubw2 v30.8h, v29.8h, v28.16b");
  TEST_SINGLE(usubw2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "usubw2 v30.4s, v29.4s, v28.8h");
  TEST_SINGLE(usubw2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "usubw2 v30.2d, v29.2d, v28.4s");

  TEST_SINGLE(raddhn(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "raddhn v30.8b, v29.8h, v28.8h");
  TEST_SINGLE(raddhn(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "raddhn v30.4h, v29.4s, v28.4s");
  TEST_SINGLE(raddhn(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "raddhn v30.2s, v29.2d, v28.2d");

  TEST_SINGLE(raddhn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "raddhn2 v30.16b, v29.8h, v28.8h");
  TEST_SINGLE(raddhn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "raddhn2 v30.8h, v29.4s, v28.4s");
  TEST_SINGLE(raddhn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "raddhn2 v30.4s, v29.2d, v28.2d");

  // TEST_SINGLE(uabal(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uabal v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uabal(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uabal v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(uabal(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uabal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(uabal(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uabal v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(uabal2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uabal2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uabal2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uabal2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(uabal2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uabal2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(uabal2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uabal2 v30.2d, v29.4s, v28.4s");

  TEST_SINGLE(rsubhn(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "rsubhn v30.8b, v29.8h, v28.8h");
  TEST_SINGLE(rsubhn(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "rsubhn v30.4h, v29.4s, v28.4s");
  TEST_SINGLE(rsubhn(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "rsubhn v30.2s, v29.2d, v28.2d");

  TEST_SINGLE(rsubhn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "rsubhn2 v30.16b, v29.8h, v28.8h");
  TEST_SINGLE(rsubhn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "rsubhn2 v30.8h, v29.4s, v28.4s");
  TEST_SINGLE(rsubhn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "rsubhn2 v30.4s, v29.2d, v28.2d");

  // TEST_SINGLE(uabdl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uabdl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uabdl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uabdl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(uabdl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uabdl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(uabdl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uabdl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(uabdl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uabdl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uabdl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uabdl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(uabdl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uabdl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(uabdl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uabdl2 v30.2d, v29.4s, v28.4s");


  // TEST_SINGLE(umlal(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umlal v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umlal(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umlal v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(umlal(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umlal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(umlal(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umlal v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(umlal2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umlal2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umlal2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umlal2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(umlal2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umlal2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(umlal2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umlal2 v30.2d, v29.4s, v28.4s");

  // TEST_SINGLE(umlsl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umlsl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umlsl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umlsl v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(umlsl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umlsl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(umlsl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umlsl v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(umlsl2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umlsl2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umlsl2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umlsl2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(umlsl2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umlsl2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(umlsl2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umlsl2 v30.2d, v29.4s, v28.4s");


  // TEST_SINGLE(umull(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umull v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umull(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umull v30.8h, v29.8b, v28.8b");
  TEST_SINGLE(umull(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umull v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(umull(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umull v30.2d, v29.2s, v28.2s");

  // TEST_SINGLE(umull2(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umull2 v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umull2(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umull2 v30.8h, v29.16b, v28.16b");
  TEST_SINGLE(umull2(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umull2 v30.4s, v29.8h, v28.8h");
  TEST_SINGLE(umull2(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umull2 v30.2d, v29.4s, v28.4s");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD three same") {
  TEST_SINGLE(shadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "shadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(shadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "shadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(shadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "shadd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(shadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "shadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(shadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "shadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(shadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "shadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(shadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "shadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(shadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "shadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sqadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqadd v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sqadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(srhadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "srhadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(srhadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "srhadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(srhadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "srhadd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(srhadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "srhadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(srhadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "srhadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(srhadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "srhadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(srhadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "srhadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(srhadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "srhadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(shsub(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "shsub v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(shsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "shsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(shsub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "shsub v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(shsub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "shsub v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(shsub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "shsub v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(shsub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "shsub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(shsub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "shsub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(shsub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "shsub v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqsub v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqsub v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqsub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqsub v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sqsub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqsub v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqsub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqsub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqsub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqsub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqsub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqsub v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmgt(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmgt v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmgt(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmgt v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmgt(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmgt v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmgt(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmgt v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmgt(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmgt v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmgt(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmgt v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmgt(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmgt v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmgt(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmgt v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmge(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmge v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmge(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmge v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmge(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmge v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmge(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmge v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmge(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmge v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmge(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmge v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmge(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmge v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmge(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmge v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sshl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqshl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(srshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "srshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(srshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "srshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(srshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "srshl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(srshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "srshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(srshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "srshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(srshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "srshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(srshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "srshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(srshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "srshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sqrshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sqrshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqrshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqrshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqrshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqrshl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sqrshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqrshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sqrshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqrshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqrshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqrshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqrshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqrshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqrshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqrshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(smax(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smax v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(smax(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smax v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(smax(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smax v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(smax(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smax v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(smax(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smax v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smax(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smax v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(smax(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smax v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(smax(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smax v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(smin(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smin v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(smin(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smin v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(smin(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smin v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(smin(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smin v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(smin(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smin v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smin(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smin v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(smin(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smin v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(smin(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smin v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sabd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sabd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sabd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sabd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sabd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sabd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(sabd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sabd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sabd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sabd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sabd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sabd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sabd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sabd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sabd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sabd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(saba(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "saba v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(saba(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "saba v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(saba(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "saba v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(saba(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "saba v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(saba(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "saba v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(saba(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "saba v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(saba(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "saba v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(saba(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "saba v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(add(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "add v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(add(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "add v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(add(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "add v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(add(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "add v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(add(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "add v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(add(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "add v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(add(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "add v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(add(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "add v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmtst(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmtst v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmtst(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmtst v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmtst(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmtst v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmtst(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmtst v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmtst(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmtst v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmtst(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmtst v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmtst(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmtst v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmtst(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmtst v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(mla(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "mla v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(mla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "mla v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(mla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "mla v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(mla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "mla v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(mla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "mla v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(mla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "mla v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(mla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "mla v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(mla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "mla v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(mul(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "mul v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(mul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "mul v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(mul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "mul v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(mul(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "mul v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(mul(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "mul v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(mul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "mul v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(mul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "mul v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(mul(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "mul v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(smaxp(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "smaxp v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(smaxp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "smaxp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(smaxp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "smaxp v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(smaxp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "smaxp v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(smaxp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "smaxp v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(smaxp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "smaxp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(smaxp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "smaxp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(smaxp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "smaxp v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sminp(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sminp v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sminp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sminp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sminp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sminp v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(sminp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sminp v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sminp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sminp v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sminp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sminp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sminp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sminp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sminp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sminp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(sqdmulh(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "sqdmulh v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmulh v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmulh v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(sqdmulh(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqdmulh v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(sqdmulh(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmulh v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmulh v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmulh v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqdmulh(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqdmulh v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(addp(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "addp v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(addp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "addp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(addp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "addp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(addp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "addp v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(addp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "addp v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(addp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "addp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(addp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "addp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(addp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "addp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmaxnm(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmaxnm v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnm v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnm v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnm v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmaxnm(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnm v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmaxnm(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnm v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmaxnm(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnm v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmaxnm(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnm v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmla(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmla v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmla v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmla v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmla v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmla(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmla v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmla v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmla v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmla v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fadd(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fadd v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fadd v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fadd v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fadd v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fadd v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmulx(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmulx v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmulx(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmulx v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmulx(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmulx v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmulx(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmulx v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmulx(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmulx v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmulx(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmulx v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmulx(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmulx v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmulx(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmulx v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fcmeq(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fcmeq v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fcmeq(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmeq v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fcmeq v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fcmeq(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fcmeq v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fcmeq(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fcmeq v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fcmeq(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fcmeq v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fcmeq(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fcmeq v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fcmeq(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fcmeq v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmax(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmax v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmax(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmax v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmax v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmax(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmax v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmax(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmax v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmax(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmax v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmax(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmax v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmax(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmax v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(frecps(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "frecps v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(frecps(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "frecps v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(frecps(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "frecps v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(frecps(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "frecps v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(frecps(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "frecps v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(frecps(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "frecps v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(frecps(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "frecps v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(frecps(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "frecps v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(and_(QReg::q30, QReg::q29, QReg::q28), "and v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(and_(DReg::d30, DReg::d29, DReg::d28), "and v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(fmlal(QReg::q30, QReg::q29, QReg::q28), "fmlal v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(fmlal(DReg::d30, DReg::d29, DReg::d28), "fmlal v30.2s, v29.2h, v28.2h");

  TEST_SINGLE(fmlal2(QReg::q30, QReg::q29, QReg::q28), "fmlal2 v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(fmlal2(DReg::d30, DReg::d29, DReg::d28), "fmlal2 v30.2s, v29.2h, v28.2h");

  TEST_SINGLE(bic(QReg::q30, QReg::q29, QReg::q28), "bic v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(bic(DReg::d30, DReg::d29, DReg::d28), "bic v30.8b, v29.8b, v28.8b");

  // TEST_SINGLE(fminnm(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fminnm v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fminnm(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminnm v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fminnm v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fminnm(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fminnm v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fminnm(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fminnm v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fminnm(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fminnm v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fminnm(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fminnm v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fminnm(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fminnm v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmls(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmls v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmls v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmls v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmls(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmls v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmls(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmls v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmls v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmls v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmls(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmls v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fsub(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fsub v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fsub v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fsub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fsub v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fsub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fsub v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fsub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fsub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fsub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fsub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fsub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fsub v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmin(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmin v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmin(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmin v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmin v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmin(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmin v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmin(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmin v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmin(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmin v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmin(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmin v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmin(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmin v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(frsqrts(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "frsqrts v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(frsqrts(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "frsqrts v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(frsqrts(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "frsqrts v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(frsqrts(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "frsqrts v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(frsqrts(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "frsqrts v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(frsqrts(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "frsqrts v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(frsqrts(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "frsqrts v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(frsqrts(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "frsqrts v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(orr(QReg::q30, QReg::q29, QReg::q28), "orr v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(orr(DReg::d30, DReg::d29, DReg::d28), "orr v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(mov(QReg::q30, QReg::q29), "mov v30.16b, v29.16b");
  TEST_SINGLE(mov(DReg::d30, DReg::d29), "mov v30.8b, v29.8b");

  TEST_SINGLE(fmlsl(QReg::q30, QReg::q29, QReg::q28), "fmlsl v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(fmlsl(DReg::d30, DReg::d29, DReg::d28), "fmlsl v30.2s, v29.2h, v28.2h");

  TEST_SINGLE(fmlsl2(QReg::q30, QReg::q29, QReg::q28), "fmlsl2 v30.4s, v29.4h, v28.4h");
  TEST_SINGLE(fmlsl2(DReg::d30, DReg::d29, DReg::d28), "fmlsl2 v30.2s, v29.2h, v28.2h");

  TEST_SINGLE(orn(QReg::q30, QReg::q29, QReg::q28), "orn v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(orn(DReg::d30, DReg::d29, DReg::d28), "orn v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(uhadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uhadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uhadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uhadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uhadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uhadd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uhadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uhadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uhadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uhadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uhadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uhadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uhadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uhadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uhadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uhadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uqadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uqadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uqadd v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(uqadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uqadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uqadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uqadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uqadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uqadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uqadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uqadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uqadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uqadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(urhadd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "urhadd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(urhadd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "urhadd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(urhadd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "urhadd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(urhadd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "urhadd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(urhadd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "urhadd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(urhadd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "urhadd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(urhadd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "urhadd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(urhadd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "urhadd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uhsub(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uhsub v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uhsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uhsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uhsub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uhsub v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uhsub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uhsub v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uhsub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uhsub v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uhsub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uhsub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uhsub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uhsub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uhsub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uhsub v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uqsub v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uqsub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uqsub v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uqsub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uqsub v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uqsub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uqsub v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uqsub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uqsub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uqsub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uqsub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uqsub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uqsub v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmhi(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmhi v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmhi(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmhi v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmhi(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmhi v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmhi(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmhi v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmhi(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmhi v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmhi(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmhi v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmhi(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmhi v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmhi(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmhi v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmhs(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmhs v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmhs(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmhs v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmhs(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmhs v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmhs(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmhs v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmhs(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmhs v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmhs(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmhs v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmhs(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmhs v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmhs(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmhs v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(ushl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "ushl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(ushl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "ushl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(ushl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "ushl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(ushl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "ushl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(ushl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "ushl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(ushl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "ushl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(ushl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "ushl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(ushl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "ushl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uqshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uqshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uqshl v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uqshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uqshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uqshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uqshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uqshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(urshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "urshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(urshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "urshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(urshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "urshl v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(urshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "urshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(urshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "urshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(urshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "urshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(urshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "urshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(urshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "urshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uqrshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uqrshl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uqrshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uqrshl v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uqrshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uqrshl v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uqrshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uqrshl v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uqrshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uqrshl v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uqrshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uqrshl v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uqrshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uqrshl v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uqrshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uqrshl v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(umax(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umax v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(umax(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umax v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(umax(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umax v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(umax(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umax v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(umax(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umax v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umax(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umax v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(umax(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umax v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(umax(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umax v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(umin(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umin v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(umin(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umin v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(umin(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umin v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(umin(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umin v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(umin(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umin v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umin(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umin v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(umin(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umin v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(umin(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umin v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uabd(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uabd v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uabd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uabd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uabd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uabd v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uabd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uabd v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uabd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uabd v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uabd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uabd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uabd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uabd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uabd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uabd v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uaba(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uaba v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uaba(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uaba v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uaba(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uaba v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uaba(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uaba v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uaba(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uaba v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uaba(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uaba v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uaba(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uaba v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uaba(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uaba v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(sub(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "sub v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sub v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sub v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(sub(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sub v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(sub(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sub v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sub(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sub v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sub(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sub v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sub(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sub v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(cmeq(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "cmeq v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(cmeq(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "cmeq v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(cmeq(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "cmeq v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(cmeq(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "cmeq v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(cmeq(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "cmeq v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(cmeq(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "cmeq v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(cmeq(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "cmeq v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(cmeq(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "cmeq v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(mls(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "mls v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(mls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "mls v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(mls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "mls v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(mls(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "mls v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(mls(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "mls v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(mls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "mls v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(mls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "mls v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(mls(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "mls v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(pmul(QReg::q30, QReg::q29, QReg::q28), "pmul v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(pmul(DReg::d30, DReg::d29, DReg::d28), "pmul v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(umaxp(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "umaxp v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(umaxp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "umaxp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(umaxp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "umaxp v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(umaxp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "umaxp v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(umaxp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "umaxp v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(umaxp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "umaxp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(umaxp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "umaxp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(umaxp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "umaxp v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(uminp(SubRegSize::i8Bit, QReg::q30, QReg::q29, QReg::q28), "uminp v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(uminp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "uminp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(uminp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "uminp v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(uminp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "uminp v30.2d, v29.2d, v28.2d");

  TEST_SINGLE(uminp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "uminp v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(uminp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "uminp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(uminp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "uminp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(uminp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "uminp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(sqrdmulh(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "sqrdmulh v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmulh v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmulh v30.4s, v29.4s, v28.4s");
  // TEST_SINGLE(sqrdmulh(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "sqrdmulh v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(sqrdmulh(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmulh v30.8b, v29.8b, v28.8b");
  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmulh v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmulh v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(sqrdmulh(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "sqrdmulh v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmaxnmp(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmaxnmp v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmaxnmp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnmp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmaxnmp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnmp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmaxnmp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxnmp v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmaxnmp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnmp v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmaxnmp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnmp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmaxnmp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnmp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmaxnmp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxnmp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(faddp(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "faddp v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(faddp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "faddp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(faddp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "faddp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(faddp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "faddp v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(faddp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "faddp v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(faddp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "faddp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(faddp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "faddp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(faddp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "faddp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmul(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmul v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmul v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmul v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmul v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmul(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmul v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmul v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmul v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmul(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmul v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fcmge(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fcmge v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fcmge(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmge v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmge(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fcmge v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fcmge(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fcmge v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fcmge(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fcmge v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fcmge(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fcmge v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fcmge(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fcmge v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fcmge(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fcmge v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(facge(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "facge v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(facge(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "facge v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(facge(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "facge v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(facge(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "facge v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(facge(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "facge v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(facge(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "facge v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(facge(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "facge v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(facge(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "facge v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fmaxp(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fmaxp v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fmaxp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fmaxp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fmaxp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fmaxp v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fmaxp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxp v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fmaxp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fmaxp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fmaxp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fmaxp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fdiv(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fdiv v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fdiv(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fdiv v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fdiv(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fdiv v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fdiv(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fdiv v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fdiv(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fdiv v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fdiv(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fdiv v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fdiv(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fdiv v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fdiv(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fdiv v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(eor(QReg::q30, QReg::q29, QReg::q28), "eor v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(eor(DReg::d30, DReg::d29, DReg::d28), "eor v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(bsl(QReg::q30, QReg::q29, QReg::q28), "bsl v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(bsl(DReg::d30, DReg::d29, DReg::d28), "bsl v30.8b, v29.8b, v28.8b");

  // TEST_SINGLE(fminnmp(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fminnmp v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fminnmp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminnmp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminnmp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fminnmp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fminnmp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fminnmp v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fminnmp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fminnmp v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fminnmp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fminnmp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fminnmp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fminnmp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fminnmp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fminnmp v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fabd(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fabd v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fabd(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fabd v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fabd(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fabd v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fabd(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fabd v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fabd(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fabd v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fabd(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fabd v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fabd(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fabd v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fabd(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fabd v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fcmgt(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fcmgt v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fcmgt(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fcmgt v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fcmgt v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fcmgt(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fcmgt v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fcmgt(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fcmgt v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fcmgt(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fcmgt v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fcmgt(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fcmgt v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fcmgt(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fcmgt v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(facgt(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "facgt v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(facgt(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "facgt v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(facgt(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "facgt v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(facgt(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "facgt v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(facgt(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "facgt v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(facgt(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "facgt v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(facgt(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "facgt v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(facgt(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "facgt v30.1d, v29.1d, v28.1d");

  // TEST_SINGLE(fminp(SubRegSize::i8Bit,  QReg::q30, QReg::q29, QReg::q28), "fminp v30.16b, v29.16b, v28.16b");
  // TEST_SINGLE(fminp(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q28), "fminp v30.8h, v29.8h, v28.8h");
  TEST_SINGLE(fminp(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28), "fminp v30.4s, v29.4s, v28.4s");
  TEST_SINGLE(fminp(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q28), "fminp v30.2d, v29.2d, v28.2d");

  // TEST_SINGLE(fminp(SubRegSize::i8Bit, DReg::d30, DReg::d29, DReg::d28), "fminp v30.8b, v29.8b, v28.8b");
  // TEST_SINGLE(fminp(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d28), "fminp v30.4h, v29.4h, v28.4h");
  TEST_SINGLE(fminp(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28), "fminp v30.2s, v29.2s, v28.2s");
  // TEST_SINGLE(fminp(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d28), "fminp v30.1d, v29.1d, v28.1d");

  TEST_SINGLE(bit(QReg::q30, QReg::q29, QReg::q28), "bit v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(bit(DReg::d30, DReg::d29, DReg::d28), "bit v30.8b, v29.8b, v28.8b");

  TEST_SINGLE(bif(QReg::q30, QReg::q29, QReg::q28), "bif v30.16b, v29.16b, v28.16b");
  TEST_SINGLE(bif(DReg::d30, DReg::d29, DReg::d28), "bif v30.8b, v29.8b, v28.8b");
}

#if TEST_FP16
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD modified immediate : fp16") {
  TEST_SINGLE(fmov(SubRegSize::i16Bit, QReg::q30, 1.0), "fmov v30.8h, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i16Bit, DReg::d30, 1.0), "fmov v30.4h, #0x70 (1.0000)");
}
#endif

TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD modified immediate") {
  // XXX: ORR - 32-bit/16-bit
  // XXX: MOVI - Shifting ones
  TEST_SINGLE(fmov(SubRegSize::i32Bit, QReg::q30, 1.0), "fmov v30.4s, #0x70 (1.0000)");
  TEST_SINGLE(fmov(SubRegSize::i64Bit, QReg::q30, 1.0), "fmov v30.2d, #0x70 (1.0000)");

  TEST_SINGLE(fmov(SubRegSize::i32Bit, DReg::d30, 1.0), "fmov v30.2s, #0x70 (1.0000)");
  // TEST_SINGLE(fmov(SubRegSize::i64Bit, DReg::d30, 1.0), "fmov v30.1d, #0x70 (1.0000)");

  // XXX: MVNI - Shifted immediate
  // XXX: BIC
  TEST_SINGLE(movi(SubRegSize::i8Bit, QReg::q30, 0xFE), "movi v30.16b, #0xfe");
  TEST_SINGLE(movi(SubRegSize::i16Bit, QReg::q30, 0xFE, 0), "movi v30.8h, #0xfe, lsl #0");
  TEST_SINGLE(movi(SubRegSize::i16Bit, QReg::q30, 0xFE, 8), "movi v30.8h, #0xfe, lsl #8");
  TEST_SINGLE(movi(SubRegSize::i32Bit, QReg::q30, 0xFE, 0), "movi v30.4s, #0xfe, lsl #0");
  TEST_SINGLE(movi(SubRegSize::i32Bit, QReg::q30, 0xFE, 8), "movi v30.4s, #0xfe, lsl #8");
  TEST_SINGLE(movi(SubRegSize::i32Bit, QReg::q30, 0xFE, 16), "movi v30.4s, #0xfe, lsl #16");
  TEST_SINGLE(movi(SubRegSize::i32Bit, QReg::q30, 0xFE, 24), "movi v30.4s, #0xfe, lsl #24");
  TEST_SINGLE(movi(SubRegSize::i64Bit, QReg::q30, 0xFF00FF), "movi v30.2d, #0xff00ff");

  TEST_SINGLE(movi(SubRegSize::i8Bit, DReg::d30, 0xFE), "movi v30.8b, #0xfe");
  TEST_SINGLE(movi(SubRegSize::i16Bit, DReg::d30, 0xFE, 0), "movi v30.4h, #0xfe, lsl #0");
  TEST_SINGLE(movi(SubRegSize::i16Bit, DReg::d30, 0xFE, 8), "movi v30.4h, #0xfe, lsl #8");
  TEST_SINGLE(movi(SubRegSize::i32Bit, DReg::d30, 0xFE, 0), "movi v30.2s, #0xfe, lsl #0");
  TEST_SINGLE(movi(SubRegSize::i32Bit, DReg::d30, 0xFE, 8), "movi v30.2s, #0xfe, lsl #8");
  TEST_SINGLE(movi(SubRegSize::i32Bit, DReg::d30, 0xFE, 16), "movi v30.2s, #0xfe, lsl #16");
  TEST_SINGLE(movi(SubRegSize::i32Bit, DReg::d30, 0xFE, 24), "movi v30.2s, #0xfe, lsl #24");
  TEST_SINGLE(movi(SubRegSize::i64Bit, DReg::d30, 0xFF00000000000000ULL), "movi d30, #0xff00000000000000");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD shift by immediate") {
  TEST_SINGLE(sshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sshr v30.16b, v29.16b, #1");
  TEST_SINGLE(sshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sshr v30.16b, v29.16b, #7");
  TEST_SINGLE(sshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sshr v30.8h, v29.8h, #1");
  TEST_SINGLE(sshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sshr v30.8h, v29.8h, #15");
  TEST_SINGLE(sshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sshr v30.4s, v29.4s, #1");
  TEST_SINGLE(sshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sshr v30.4s, v29.4s, #31");
  TEST_SINGLE(sshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sshr v30.2d, v29.2d, #1");
  TEST_SINGLE(sshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sshr v30.2d, v29.2d, #63");

  TEST_SINGLE(sshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sshr v30.8b, v29.8b, #1");
  TEST_SINGLE(sshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sshr v30.8b, v29.8b, #7");
  TEST_SINGLE(sshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sshr v30.4h, v29.4h, #1");
  TEST_SINGLE(sshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sshr v30.4h, v29.4h, #15");
  TEST_SINGLE(sshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sshr v30.2s, v29.2s, #1");
  TEST_SINGLE(sshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sshr v30.2s, v29.2s, #31");
  // TEST_SINGLE(sshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sshr v30.1d, v29.1d, #1");
  // TEST_SINGLE(sshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sshr v30.1d, v29.1d, #63");

  TEST_SINGLE(ssra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "ssra v30.16b, v29.16b, #1");
  TEST_SINGLE(ssra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "ssra v30.16b, v29.16b, #7");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "ssra v30.8h, v29.8h, #1");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "ssra v30.8h, v29.8h, #15");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "ssra v30.4s, v29.4s, #1");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "ssra v30.4s, v29.4s, #31");
  TEST_SINGLE(ssra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "ssra v30.2d, v29.2d, #1");
  TEST_SINGLE(ssra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "ssra v30.2d, v29.2d, #63");

  TEST_SINGLE(ssra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "ssra v30.8b, v29.8b, #1");
  TEST_SINGLE(ssra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "ssra v30.8b, v29.8b, #7");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "ssra v30.4h, v29.4h, #1");
  TEST_SINGLE(ssra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "ssra v30.4h, v29.4h, #15");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "ssra v30.2s, v29.2s, #1");
  TEST_SINGLE(ssra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "ssra v30.2s, v29.2s, #31");
  // TEST_SINGLE(ssra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "ssra v30.1d, v29.1d, #1");
  // TEST_SINGLE(ssra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "ssra v30.1d, v29.1d, #63");

  TEST_SINGLE(srshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "srshr v30.16b, v29.16b, #1");
  TEST_SINGLE(srshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "srshr v30.16b, v29.16b, #7");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "srshr v30.8h, v29.8h, #1");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "srshr v30.8h, v29.8h, #15");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "srshr v30.4s, v29.4s, #1");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "srshr v30.4s, v29.4s, #31");
  TEST_SINGLE(srshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "srshr v30.2d, v29.2d, #1");
  TEST_SINGLE(srshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "srshr v30.2d, v29.2d, #63");

  TEST_SINGLE(srshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "srshr v30.8b, v29.8b, #1");
  TEST_SINGLE(srshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "srshr v30.8b, v29.8b, #7");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "srshr v30.4h, v29.4h, #1");
  TEST_SINGLE(srshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "srshr v30.4h, v29.4h, #15");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "srshr v30.2s, v29.2s, #1");
  TEST_SINGLE(srshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "srshr v30.2s, v29.2s, #31");
  // TEST_SINGLE(srshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "srshr v30.1d, v29.1d, #1");
  // TEST_SINGLE(srshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "srshr v30.1d, v29.1d, #63");

  TEST_SINGLE(srsra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "srsra v30.16b, v29.16b, #1");
  TEST_SINGLE(srsra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "srsra v30.16b, v29.16b, #7");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "srsra v30.8h, v29.8h, #1");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "srsra v30.8h, v29.8h, #15");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "srsra v30.4s, v29.4s, #1");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "srsra v30.4s, v29.4s, #31");
  TEST_SINGLE(srsra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "srsra v30.2d, v29.2d, #1");
  TEST_SINGLE(srsra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "srsra v30.2d, v29.2d, #63");

  TEST_SINGLE(srsra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "srsra v30.8b, v29.8b, #1");
  TEST_SINGLE(srsra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "srsra v30.8b, v29.8b, #7");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "srsra v30.4h, v29.4h, #1");
  TEST_SINGLE(srsra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "srsra v30.4h, v29.4h, #15");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "srsra v30.2s, v29.2s, #1");
  TEST_SINGLE(srsra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "srsra v30.2s, v29.2s, #31");
  // TEST_SINGLE(srsra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "srsra v30.1d, v29.1d, #1");
  // TEST_SINGLE(srsra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "srsra v30.1d, v29.1d, #63");

  TEST_SINGLE(shl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "shl v30.16b, v29.16b, #1");
  TEST_SINGLE(shl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "shl v30.16b, v29.16b, #7");
  TEST_SINGLE(shl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "shl v30.8h, v29.8h, #1");
  TEST_SINGLE(shl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "shl v30.8h, v29.8h, #15");
  TEST_SINGLE(shl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "shl v30.4s, v29.4s, #1");
  TEST_SINGLE(shl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "shl v30.4s, v29.4s, #31");
  TEST_SINGLE(shl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "shl v30.2d, v29.2d, #1");
  TEST_SINGLE(shl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "shl v30.2d, v29.2d, #63");

  TEST_SINGLE(shl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "shl v30.8b, v29.8b, #1");
  TEST_SINGLE(shl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "shl v30.8b, v29.8b, #7");
  TEST_SINGLE(shl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "shl v30.4h, v29.4h, #1");
  TEST_SINGLE(shl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "shl v30.4h, v29.4h, #15");
  TEST_SINGLE(shl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "shl v30.2s, v29.2s, #1");
  TEST_SINGLE(shl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "shl v30.2s, v29.2s, #31");
  // TEST_SINGLE(shl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "shl v30.1d, v29.1d, #1");
  // TEST_SINGLE(shl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "shl v30.1d, v29.1d, #63");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqshl v30.16b, v29.16b, #1");
  TEST_SINGLE(sqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqshl v30.16b, v29.16b, #7");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqshl v30.8h, v29.8h, #1");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqshl v30.8h, v29.8h, #15");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqshl v30.4s, v29.4s, #1");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqshl v30.4s, v29.4s, #31");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sqshl v30.2d, v29.2d, #1");
  TEST_SINGLE(sqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqshl v30.2d, v29.2d, #63");

  TEST_SINGLE(sqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqshl v30.8b, v29.8b, #1");
  TEST_SINGLE(sqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqshl v30.8b, v29.8b, #7");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqshl v30.4h, v29.4h, #1");
  TEST_SINGLE(sqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqshl v30.4h, v29.4h, #15");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqshl v30.2s, v29.2s, #1");
  TEST_SINGLE(sqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqshl v30.2s, v29.2s, #31");
  // TEST_SINGLE(sqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqshl v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqshl v30.1d, v29.1d, #63");

  TEST_SINGLE(shrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "shrn v30.8b, v29.8h, #1");
  TEST_SINGLE(shrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "shrn v30.8b, v29.8h, #7");
  TEST_SINGLE(shrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "shrn v30.4h, v29.4s, #1");
  TEST_SINGLE(shrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "shrn v30.4h, v29.4s, #15");
  TEST_SINGLE(shrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "shrn v30.2s, v29.2d, #1");
  TEST_SINGLE(shrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "shrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(shrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "shrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(shrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "shrn v30.1d, v29.1d, #63");

  TEST_SINGLE(shrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "shrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(shrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "shrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(shrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "shrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(shrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "shrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(shrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "shrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(shrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "shrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(shrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "shrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(shrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "shrn2 v30.2d, v29.2d, #63");

  TEST_SINGLE(rshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "rshrn v30.8b, v29.8h, #1");
  TEST_SINGLE(rshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "rshrn v30.8b, v29.8h, #7");
  TEST_SINGLE(rshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "rshrn v30.4h, v29.4s, #1");
  TEST_SINGLE(rshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "rshrn v30.4h, v29.4s, #15");
  TEST_SINGLE(rshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "rshrn v30.2s, v29.2d, #1");
  TEST_SINGLE(rshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "rshrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(rshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "rshrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(rshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "rshrn v30.1d, v29.1d, #63");

  TEST_SINGLE(rshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "rshrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(rshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "rshrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(rshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "rshrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(rshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "rshrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(rshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "rshrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(rshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "rshrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(rshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "rshrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(rshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "rshrn2 v30.2d, v29.2d, #63");

  TEST_SINGLE(sqshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqshrn v30.8b, v29.8h, #1");
  TEST_SINGLE(sqshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqshrn v30.8b, v29.8h, #7");
  TEST_SINGLE(sqshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqshrn v30.4h, v29.4s, #1");
  TEST_SINGLE(sqshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqshrn v30.4h, v29.4s, #15");
  TEST_SINGLE(sqshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqshrn v30.2s, v29.2d, #1");
  TEST_SINGLE(sqshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqshrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(sqshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqshrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqshrn v30.1d, v29.1d, #63");

  TEST_SINGLE(sqshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqshrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(sqshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqshrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(sqshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqshrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(sqshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqshrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(sqshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqshrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(sqshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqshrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(sqshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "sqshrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(sqshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqshrn2 v30.2d, v29.2d, #63");

  TEST_SINGLE(sqrshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqrshrn v30.8b, v29.8h, #1");
  TEST_SINGLE(sqrshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqrshrn v30.8b, v29.8h, #7");
  TEST_SINGLE(sqrshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqrshrn v30.4h, v29.4s, #1");
  TEST_SINGLE(sqrshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqrshrn v30.4h, v29.4s, #15");
  TEST_SINGLE(sqrshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqrshrn v30.2s, v29.2d, #1");
  TEST_SINGLE(sqrshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqrshrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(sqrshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqrshrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqrshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqrshrn v30.1d, v29.1d, #63");

  TEST_SINGLE(sqrshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqrshrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(sqrshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqrshrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(sqrshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqrshrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(sqrshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqrshrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(sqrshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqrshrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(sqrshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqrshrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(sqrshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "sqrshrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(sqrshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqrshrn2 v30.2d, v29.2d, #63");

  // TEST_SINGLE(sshll(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "sshll v30.8b, v29.8h, #1");
  // TEST_SINGLE(sshll(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "sshll v30.8b, v29.8h, #7");
  TEST_SINGLE(sshll(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sshll v30.8h, v29.8b, #1");
  TEST_SINGLE(sshll(SubRegSize::i16Bit, DReg::d30, DReg::d29, 7), "sshll v30.8h, v29.8b, #7");
  TEST_SINGLE(sshll(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sshll v30.4s, v29.4h, #1");
  TEST_SINGLE(sshll(SubRegSize::i32Bit, DReg::d30, DReg::d29, 15), "sshll v30.4s, v29.4h, #15");
  TEST_SINGLE(sshll(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1), "sshll v30.2d, v29.2s, #1");
  TEST_SINGLE(sshll(SubRegSize::i64Bit, DReg::d30, DReg::d29, 31), "sshll v30.2d, v29.2s, #31");

  // TEST_SINGLE(sshll2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "sshll2 v30.16b, v29.8h, #1");
  // TEST_SINGLE(sshll2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "sshll2 v30.16b, v29.8h, #7");
  TEST_SINGLE(sshll2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sshll2 v30.8h, v29.16b, #1");
  TEST_SINGLE(sshll2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 7), "sshll2 v30.8h, v29.16b, #7");
  TEST_SINGLE(sshll2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sshll2 v30.4s, v29.8h, #1");
  TEST_SINGLE(sshll2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 15), "sshll2 v30.4s, v29.8h, #15");
  TEST_SINGLE(sshll2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sshll2 v30.2d, v29.4s, #1");
  TEST_SINGLE(sshll2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 31), "sshll2 v30.2d, v29.4s, #31");

  // TEST_SINGLE(sxtl(SubRegSize::i8Bit, QReg::q30, QReg::q29),   "sxtl v30.8b, v29.8h");
  TEST_SINGLE(sxtl(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sxtl v30.8h, v29.8b");
  TEST_SINGLE(sxtl(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sxtl v30.4s, v29.4h");
  TEST_SINGLE(sxtl(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sxtl v30.2d, v29.2s");

  // TEST_SINGLE(sxtl(SubRegSize::i8Bit, DReg::d30, DReg::d29),   "sxtl v30.8b, v29.8h");
  TEST_SINGLE(sxtl(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sxtl v30.8h, v29.8b");
  TEST_SINGLE(sxtl(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sxtl v30.4s, v29.4h");
  TEST_SINGLE(sxtl(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sxtl v30.2d, v29.2s");

  // TEST_SINGLE(sxtl2(SubRegSize::i8Bit, QReg::q30, QReg::q29),   "sxtl2 v30.16b, v29.8h");
  TEST_SINGLE(sxtl2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "sxtl2 v30.8h, v29.16b");
  TEST_SINGLE(sxtl2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "sxtl2 v30.4s, v29.8h");
  TEST_SINGLE(sxtl2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "sxtl2 v30.2d, v29.4s");

  // TEST_SINGLE(sxtl2(SubRegSize::i8Bit, DReg::d30, DReg::d29),   "sxtl2 v30.16b, v29.8h");
  TEST_SINGLE(sxtl2(SubRegSize::i16Bit, DReg::d30, DReg::d29), "sxtl2 v30.8h, v29.16b");
  TEST_SINGLE(sxtl2(SubRegSize::i32Bit, DReg::d30, DReg::d29), "sxtl2 v30.4s, v29.8h");
  TEST_SINGLE(sxtl2(SubRegSize::i64Bit, DReg::d30, DReg::d29), "sxtl2 v30.2d, v29.4s");

  // TEST_SINGLE(scvtf(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "scvtf v30.16b, v29.16b, #1");
  // TEST_SINGLE(scvtf(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "scvtf v30.16b, v29.16b, #7");
  TEST_SINGLE(scvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "scvtf v30.8h, v29.8h, #1");
  TEST_SINGLE(scvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "scvtf v30.8h, v29.8h, #15");
  TEST_SINGLE(scvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "scvtf v30.4s, v29.4s, #1");
  TEST_SINGLE(scvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "scvtf v30.4s, v29.4s, #31");
  TEST_SINGLE(scvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "scvtf v30.2d, v29.2d, #1");
  TEST_SINGLE(scvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "scvtf v30.2d, v29.2d, #63");

  // TEST_SINGLE(scvtf(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "scvtf v30.8b, v29.8b, #1");
  // TEST_SINGLE(scvtf(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "scvtf v30.8b, v29.8b, #7");
  TEST_SINGLE(scvtf(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "scvtf v30.4h, v29.4h, #1");
  TEST_SINGLE(scvtf(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "scvtf v30.4h, v29.4h, #15");
  TEST_SINGLE(scvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "scvtf v30.2s, v29.2s, #1");
  TEST_SINGLE(scvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "scvtf v30.2s, v29.2s, #31");
  // TEST_SINGLE(scvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "scvtf v30.1d, v29.1d, #1");
  // TEST_SINGLE(scvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "scvtf v30.1d, v29.1d, #63");

  // TEST_SINGLE(fcvtzs(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "fcvtzs v30.16b, v29.16b, #1");
  // TEST_SINGLE(fcvtzs(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "fcvtzs v30.16b, v29.16b, #7");
  TEST_SINGLE(fcvtzs(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "fcvtzs v30.8h, v29.8h, #1");
  TEST_SINGLE(fcvtzs(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "fcvtzs v30.8h, v29.8h, #15");
  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "fcvtzs v30.4s, v29.4s, #1");
  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "fcvtzs v30.4s, v29.4s, #31");
  TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "fcvtzs v30.2d, v29.2d, #1");
  TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "fcvtzs v30.2d, v29.2d, #63");

  // TEST_SINGLE(fcvtzs(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "fcvtzs v30.8b, v29.8b, #1");
  // TEST_SINGLE(fcvtzs(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "fcvtzs v30.8b, v29.8b, #7");
  TEST_SINGLE(fcvtzs(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "fcvtzs v30.4h, v29.4h, #1");
  TEST_SINGLE(fcvtzs(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "fcvtzs v30.4h, v29.4h, #15");
  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "fcvtzs v30.2s, v29.2s, #1");
  TEST_SINGLE(fcvtzs(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "fcvtzs v30.2s, v29.2s, #31");
  // TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "fcvtzs v30.1d, v29.1d, #1");
  // TEST_SINGLE(fcvtzs(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "fcvtzs v30.1d, v29.1d, #63");

  TEST_SINGLE(ushr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "ushr v30.16b, v29.16b, #1");
  TEST_SINGLE(ushr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "ushr v30.16b, v29.16b, #7");
  TEST_SINGLE(ushr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "ushr v30.8h, v29.8h, #1");
  TEST_SINGLE(ushr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "ushr v30.8h, v29.8h, #15");
  TEST_SINGLE(ushr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "ushr v30.4s, v29.4s, #1");
  TEST_SINGLE(ushr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "ushr v30.4s, v29.4s, #31");
  TEST_SINGLE(ushr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "ushr v30.2d, v29.2d, #1");
  TEST_SINGLE(ushr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "ushr v30.2d, v29.2d, #63");

  TEST_SINGLE(ushr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "ushr v30.8b, v29.8b, #1");
  TEST_SINGLE(ushr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "ushr v30.8b, v29.8b, #7");
  TEST_SINGLE(ushr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "ushr v30.4h, v29.4h, #1");
  TEST_SINGLE(ushr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "ushr v30.4h, v29.4h, #15");
  TEST_SINGLE(ushr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "ushr v30.2s, v29.2s, #1");
  TEST_SINGLE(ushr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "ushr v30.2s, v29.2s, #31");
  // TEST_SINGLE(ushr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "ushr v30.1d, v29.1d, #1");
  // TEST_SINGLE(ushr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "ushr v30.1d, v29.1d, #63");

  TEST_SINGLE(usra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "usra v30.16b, v29.16b, #1");
  TEST_SINGLE(usra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "usra v30.16b, v29.16b, #7");
  TEST_SINGLE(usra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "usra v30.8h, v29.8h, #1");
  TEST_SINGLE(usra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "usra v30.8h, v29.8h, #15");
  TEST_SINGLE(usra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "usra v30.4s, v29.4s, #1");
  TEST_SINGLE(usra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "usra v30.4s, v29.4s, #31");
  TEST_SINGLE(usra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "usra v30.2d, v29.2d, #1");
  TEST_SINGLE(usra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "usra v30.2d, v29.2d, #63");

  TEST_SINGLE(usra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "usra v30.8b, v29.8b, #1");
  TEST_SINGLE(usra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "usra v30.8b, v29.8b, #7");
  TEST_SINGLE(usra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "usra v30.4h, v29.4h, #1");
  TEST_SINGLE(usra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "usra v30.4h, v29.4h, #15");
  TEST_SINGLE(usra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "usra v30.2s, v29.2s, #1");
  TEST_SINGLE(usra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "usra v30.2s, v29.2s, #31");
  // TEST_SINGLE(usra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "usra v30.1d, v29.1d, #1");
  // TEST_SINGLE(usra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "usra v30.1d, v29.1d, #63");

  TEST_SINGLE(urshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "urshr v30.16b, v29.16b, #1");
  TEST_SINGLE(urshr(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "urshr v30.16b, v29.16b, #7");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "urshr v30.8h, v29.8h, #1");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "urshr v30.8h, v29.8h, #15");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "urshr v30.4s, v29.4s, #1");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "urshr v30.4s, v29.4s, #31");
  TEST_SINGLE(urshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "urshr v30.2d, v29.2d, #1");
  TEST_SINGLE(urshr(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "urshr v30.2d, v29.2d, #63");

  TEST_SINGLE(urshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "urshr v30.8b, v29.8b, #1");
  TEST_SINGLE(urshr(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "urshr v30.8b, v29.8b, #7");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "urshr v30.4h, v29.4h, #1");
  TEST_SINGLE(urshr(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "urshr v30.4h, v29.4h, #15");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "urshr v30.2s, v29.2s, #1");
  TEST_SINGLE(urshr(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "urshr v30.2s, v29.2s, #31");
  // TEST_SINGLE(urshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "urshr v30.1d, v29.1d, #1");
  // TEST_SINGLE(urshr(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "urshr v30.1d, v29.1d, #63");

  TEST_SINGLE(ursra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "ursra v30.16b, v29.16b, #1");
  TEST_SINGLE(ursra(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "ursra v30.16b, v29.16b, #7");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "ursra v30.8h, v29.8h, #1");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "ursra v30.8h, v29.8h, #15");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "ursra v30.4s, v29.4s, #1");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "ursra v30.4s, v29.4s, #31");
  TEST_SINGLE(ursra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "ursra v30.2d, v29.2d, #1");
  TEST_SINGLE(ursra(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "ursra v30.2d, v29.2d, #63");

  TEST_SINGLE(ursra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "ursra v30.8b, v29.8b, #1");
  TEST_SINGLE(ursra(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "ursra v30.8b, v29.8b, #7");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "ursra v30.4h, v29.4h, #1");
  TEST_SINGLE(ursra(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "ursra v30.4h, v29.4h, #15");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "ursra v30.2s, v29.2s, #1");
  TEST_SINGLE(ursra(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "ursra v30.2s, v29.2s, #31");
  // TEST_SINGLE(ursra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "ursra v30.1d, v29.1d, #1");
  // TEST_SINGLE(ursra(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "ursra v30.1d, v29.1d, #63");

  TEST_SINGLE(sri(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sri v30.16b, v29.16b, #1");
  TEST_SINGLE(sri(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sri v30.16b, v29.16b, #7");
  TEST_SINGLE(sri(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sri v30.8h, v29.8h, #1");
  TEST_SINGLE(sri(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sri v30.8h, v29.8h, #15");
  TEST_SINGLE(sri(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sri v30.4s, v29.4s, #1");
  TEST_SINGLE(sri(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sri v30.4s, v29.4s, #31");
  TEST_SINGLE(sri(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sri v30.2d, v29.2d, #1");
  TEST_SINGLE(sri(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sri v30.2d, v29.2d, #63");

  TEST_SINGLE(sri(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sri v30.8b, v29.8b, #1");
  TEST_SINGLE(sri(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sri v30.8b, v29.8b, #7");
  TEST_SINGLE(sri(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sri v30.4h, v29.4h, #1");
  TEST_SINGLE(sri(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sri v30.4h, v29.4h, #15");
  TEST_SINGLE(sri(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sri v30.2s, v29.2s, #1");
  TEST_SINGLE(sri(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sri v30.2s, v29.2s, #31");
  // TEST_SINGLE(sri(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sri v30.1d, v29.1d, #1");
  // TEST_SINGLE(sri(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sri v30.1d, v29.1d, #63");

  TEST_SINGLE(sli(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sli v30.16b, v29.16b, #1");
  TEST_SINGLE(sli(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sli v30.16b, v29.16b, #7");
  TEST_SINGLE(sli(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sli v30.8h, v29.8h, #1");
  TEST_SINGLE(sli(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sli v30.8h, v29.8h, #15");
  TEST_SINGLE(sli(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sli v30.4s, v29.4s, #1");
  TEST_SINGLE(sli(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sli v30.4s, v29.4s, #31");
  TEST_SINGLE(sli(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sli v30.2d, v29.2d, #1");
  TEST_SINGLE(sli(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sli v30.2d, v29.2d, #63");

  TEST_SINGLE(sli(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sli v30.8b, v29.8b, #1");
  TEST_SINGLE(sli(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sli v30.8b, v29.8b, #7");
  TEST_SINGLE(sli(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sli v30.4h, v29.4h, #1");
  TEST_SINGLE(sli(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sli v30.4h, v29.4h, #15");
  TEST_SINGLE(sli(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sli v30.2s, v29.2s, #1");
  TEST_SINGLE(sli(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sli v30.2s, v29.2s, #31");
  // TEST_SINGLE(sli(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sli v30.1d, v29.1d, #1");
  // TEST_SINGLE(sli(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sli v30.1d, v29.1d, #63");

  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqshlu v30.16b, v29.16b, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqshlu v30.16b, v29.16b, #7");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqshlu v30.8h, v29.8h, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqshlu v30.8h, v29.8h, #15");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqshlu v30.4s, v29.4s, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqshlu v30.4s, v29.4s, #31");
  TEST_SINGLE(sqshlu(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "sqshlu v30.2d, v29.2d, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqshlu v30.2d, v29.2d, #63");

  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqshlu v30.8b, v29.8b, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqshlu v30.8b, v29.8b, #7");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqshlu v30.4h, v29.4h, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqshlu v30.4h, v29.4h, #15");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqshlu v30.2s, v29.2s, #1");
  TEST_SINGLE(sqshlu(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqshlu v30.2s, v29.2s, #31");
  // TEST_SINGLE(sqshlu(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqshlu v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqshlu(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqshlu v30.1d, v29.1d, #63");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "uqshl v30.16b, v29.16b, #1");
  TEST_SINGLE(uqshl(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "uqshl v30.16b, v29.16b, #7");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "uqshl v30.8h, v29.8h, #1");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "uqshl v30.8h, v29.8h, #15");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "uqshl v30.4s, v29.4s, #1");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "uqshl v30.4s, v29.4s, #31");
  TEST_SINGLE(uqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "uqshl v30.2d, v29.2d, #1");
  TEST_SINGLE(uqshl(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "uqshl v30.2d, v29.2d, #63");

  TEST_SINGLE(uqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "uqshl v30.8b, v29.8b, #1");
  TEST_SINGLE(uqshl(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "uqshl v30.8b, v29.8b, #7");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "uqshl v30.4h, v29.4h, #1");
  TEST_SINGLE(uqshl(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "uqshl v30.4h, v29.4h, #15");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "uqshl v30.2s, v29.2s, #1");
  TEST_SINGLE(uqshl(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "uqshl v30.2s, v29.2s, #31");
  // TEST_SINGLE(uqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "uqshl v30.1d, v29.1d, #1");
  // TEST_SINGLE(uqshl(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "uqshl v30.1d, v29.1d, #63");

  TEST_SINGLE(sqshrun(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqshrun v30.8b, v29.8h, #1");
  TEST_SINGLE(sqshrun(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqshrun v30.8b, v29.8h, #7");
  TEST_SINGLE(sqshrun(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqshrun v30.4h, v29.4s, #1");
  TEST_SINGLE(sqshrun(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqshrun v30.4h, v29.4s, #15");
  TEST_SINGLE(sqshrun(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqshrun v30.2s, v29.2d, #1");
  TEST_SINGLE(sqshrun(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqshrun v30.2s, v29.2d, #31");
  // TEST_SINGLE(sqshrun(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqshrun v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqshrun(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqshrun v30.1d, v29.1d, #63");

  TEST_SINGLE(sqshrun2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqshrun2 v30.16b, v29.8h, #1");
  TEST_SINGLE(sqshrun2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqshrun2 v30.16b, v29.8h, #7");
  TEST_SINGLE(sqshrun2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqshrun2 v30.8h, v29.4s, #1");
  TEST_SINGLE(sqshrun2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqshrun2 v30.8h, v29.4s, #15");
  TEST_SINGLE(sqshrun2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqshrun2 v30.4s, v29.2d, #1");
  TEST_SINGLE(sqshrun2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqshrun2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(sqshrun2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "sqshrun2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(sqshrun2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqshrun2 v30.2d, v29.2d, #63");

  TEST_SINGLE(sqrshrun(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "sqrshrun v30.8b, v29.8h, #1");
  TEST_SINGLE(sqrshrun(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "sqrshrun v30.8b, v29.8h, #7");
  TEST_SINGLE(sqrshrun(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "sqrshrun v30.4h, v29.4s, #1");
  TEST_SINGLE(sqrshrun(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "sqrshrun v30.4h, v29.4s, #15");
  TEST_SINGLE(sqrshrun(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "sqrshrun v30.2s, v29.2d, #1");
  TEST_SINGLE(sqrshrun(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "sqrshrun v30.2s, v29.2d, #31");
  // TEST_SINGLE(sqrshrun(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "sqrshrun v30.1d, v29.1d, #1");
  // TEST_SINGLE(sqrshrun(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "sqrshrun v30.1d, v29.1d, #63");

  TEST_SINGLE(sqrshrun2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "sqrshrun2 v30.16b, v29.8h, #1");
  TEST_SINGLE(sqrshrun2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "sqrshrun2 v30.16b, v29.8h, #7");
  TEST_SINGLE(sqrshrun2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "sqrshrun2 v30.8h, v29.4s, #1");
  TEST_SINGLE(sqrshrun2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "sqrshrun2 v30.8h, v29.4s, #15");
  TEST_SINGLE(sqrshrun2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "sqrshrun2 v30.4s, v29.2d, #1");
  TEST_SINGLE(sqrshrun2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "sqrshrun2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(sqrshrun2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "sqrshrun2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(sqrshrun2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "sqrshrun2 v30.2d, v29.2d, #63");

  TEST_SINGLE(uqshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "uqshrn v30.8b, v29.8h, #1");
  TEST_SINGLE(uqshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "uqshrn v30.8b, v29.8h, #7");
  TEST_SINGLE(uqshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "uqshrn v30.4h, v29.4s, #1");
  TEST_SINGLE(uqshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "uqshrn v30.4h, v29.4s, #15");
  TEST_SINGLE(uqshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "uqshrn v30.2s, v29.2d, #1");
  TEST_SINGLE(uqshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "uqshrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(uqshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "uqshrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(uqshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "uqshrn v30.1d, v29.1d, #63");

  TEST_SINGLE(uqshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "uqshrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(uqshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "uqshrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(uqshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "uqshrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(uqshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "uqshrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(uqshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "uqshrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(uqshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "uqshrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(uqshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "uqshrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(uqshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "uqshrn2 v30.2d, v29.2d, #63");

  TEST_SINGLE(uqrshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1), "uqrshrn v30.8b, v29.8h, #1");
  TEST_SINGLE(uqrshrn(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7), "uqrshrn v30.8b, v29.8h, #7");
  TEST_SINGLE(uqrshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "uqrshrn v30.4h, v29.4s, #1");
  TEST_SINGLE(uqrshrn(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "uqrshrn v30.4h, v29.4s, #15");
  TEST_SINGLE(uqrshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "uqrshrn v30.2s, v29.2d, #1");
  TEST_SINGLE(uqrshrn(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "uqrshrn v30.2s, v29.2d, #31");
  // TEST_SINGLE(uqrshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "uqrshrn v30.1d, v29.1d, #1");
  // TEST_SINGLE(uqrshrn(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "uqrshrn v30.1d, v29.1d, #63");

  TEST_SINGLE(uqrshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1), "uqrshrn2 v30.16b, v29.8h, #1");
  TEST_SINGLE(uqrshrn2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7), "uqrshrn2 v30.16b, v29.8h, #7");
  TEST_SINGLE(uqrshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "uqrshrn2 v30.8h, v29.4s, #1");
  TEST_SINGLE(uqrshrn2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "uqrshrn2 v30.8h, v29.4s, #15");
  TEST_SINGLE(uqrshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "uqrshrn2 v30.4s, v29.2d, #1");
  TEST_SINGLE(uqrshrn2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "uqrshrn2 v30.4s, v29.2d, #31");
  // TEST_SINGLE(uqrshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1),  "uqrshrn2 v30.2d, v29.2d, #1");
  // TEST_SINGLE(uqrshrn2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "uqrshrn2 v30.2d, v29.2d, #63");

  // TEST_SINGLE(ushll(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "ushll v30.8b, v29.8h, #1");
  // TEST_SINGLE(ushll(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "ushll v30.8b, v29.8h, #7");
  TEST_SINGLE(ushll(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "ushll v30.8h, v29.8b, #1");
  TEST_SINGLE(ushll(SubRegSize::i16Bit, DReg::d30, DReg::d29, 7), "ushll v30.8h, v29.8b, #7");
  TEST_SINGLE(ushll(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "ushll v30.4s, v29.4h, #1");
  TEST_SINGLE(ushll(SubRegSize::i32Bit, DReg::d30, DReg::d29, 15), "ushll v30.4s, v29.4h, #15");
  TEST_SINGLE(ushll(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1), "ushll v30.2d, v29.2s, #1");
  TEST_SINGLE(ushll(SubRegSize::i64Bit, DReg::d30, DReg::d29, 31), "ushll v30.2d, v29.2s, #31");

  // TEST_SINGLE(ushll2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "ushll2 v30.16b, v29.8h, #1");
  // TEST_SINGLE(ushll2(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "ushll2 v30.16b, v29.8h, #7");
  TEST_SINGLE(ushll2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "ushll2 v30.8h, v29.16b, #1");
  TEST_SINGLE(ushll2(SubRegSize::i16Bit, QReg::q30, QReg::q29, 7), "ushll2 v30.8h, v29.16b, #7");
  TEST_SINGLE(ushll2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "ushll2 v30.4s, v29.8h, #1");
  TEST_SINGLE(ushll2(SubRegSize::i32Bit, QReg::q30, QReg::q29, 15), "ushll2 v30.4s, v29.8h, #15");
  TEST_SINGLE(ushll2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "ushll2 v30.2d, v29.4s, #1");
  TEST_SINGLE(ushll2(SubRegSize::i64Bit, QReg::q30, QReg::q29, 31), "ushll2 v30.2d, v29.4s, #31");

  // TEST_SINGLE(uxtl(SubRegSize::i8Bit, DReg::d30, DReg::d29),   "uxtl v30.8b, v29.8h");
  TEST_SINGLE(uxtl(SubRegSize::i16Bit, DReg::d30, DReg::d29), "uxtl v30.8h, v29.8b");
  TEST_SINGLE(uxtl(SubRegSize::i32Bit, DReg::d30, DReg::d29), "uxtl v30.4s, v29.4h");
  TEST_SINGLE(uxtl(SubRegSize::i64Bit, DReg::d30, DReg::d29), "uxtl v30.2d, v29.2s");

  // TEST_SINGLE(uxtl2(SubRegSize::i8Bit, QReg::q30, QReg::q29),   "uxtl2 v30.16b, v29.8h");
  TEST_SINGLE(uxtl2(SubRegSize::i16Bit, QReg::q30, QReg::q29), "uxtl2 v30.8h, v29.16b");
  TEST_SINGLE(uxtl2(SubRegSize::i32Bit, QReg::q30, QReg::q29), "uxtl2 v30.4s, v29.8h");
  TEST_SINGLE(uxtl2(SubRegSize::i64Bit, QReg::q30, QReg::q29), "uxtl2 v30.2d, v29.4s");

  // TEST_SINGLE(ucvtf(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "ucvtf v30.16b, v29.16b, #1");
  // TEST_SINGLE(ucvtf(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "ucvtf v30.16b, v29.16b, #7");
  TEST_SINGLE(ucvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "ucvtf v30.8h, v29.8h, #1");
  TEST_SINGLE(ucvtf(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "ucvtf v30.8h, v29.8h, #15");
  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "ucvtf v30.4s, v29.4s, #1");
  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "ucvtf v30.4s, v29.4s, #31");
  TEST_SINGLE(ucvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "ucvtf v30.2d, v29.2d, #1");
  TEST_SINGLE(ucvtf(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "ucvtf v30.2d, v29.2d, #63");

  // TEST_SINGLE(ucvtf(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "ucvtf v30.8b, v29.8b, #1");
  // TEST_SINGLE(ucvtf(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "ucvtf v30.8b, v29.8b, #7");
  TEST_SINGLE(ucvtf(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "ucvtf v30.4h, v29.4h, #1");
  TEST_SINGLE(ucvtf(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "ucvtf v30.4h, v29.4h, #15");
  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "ucvtf v30.2s, v29.2s, #1");
  TEST_SINGLE(ucvtf(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "ucvtf v30.2s, v29.2s, #31");
  // TEST_SINGLE(ucvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "ucvtf v30.1d, v29.1d, #1");
  // TEST_SINGLE(ucvtf(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "ucvtf v30.1d, v29.1d, #63");

  // TEST_SINGLE(fcvtzu(SubRegSize::i8Bit, QReg::q30, QReg::q29, 1),   "fcvtzu v30.16b, v29.16b, #1");
  // TEST_SINGLE(fcvtzu(SubRegSize::i8Bit, QReg::q30, QReg::q29, 7),   "fcvtzu v30.16b, v29.16b, #7");
  TEST_SINGLE(fcvtzu(SubRegSize::i16Bit, QReg::q30, QReg::q29, 1), "fcvtzu v30.8h, v29.8h, #1");
  TEST_SINGLE(fcvtzu(SubRegSize::i16Bit, QReg::q30, QReg::q29, 15), "fcvtzu v30.8h, v29.8h, #15");
  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, QReg::q30, QReg::q29, 1), "fcvtzu v30.4s, v29.4s, #1");
  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, QReg::q30, QReg::q29, 31), "fcvtzu v30.4s, v29.4s, #31");
  TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, QReg::q30, QReg::q29, 1), "fcvtzu v30.2d, v29.2d, #1");
  TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, QReg::q30, QReg::q29, 63), "fcvtzu v30.2d, v29.2d, #63");

  // TEST_SINGLE(fcvtzu(SubRegSize::i8Bit, DReg::d30, DReg::d29, 1),   "fcvtzu v30.8b, v29.8b, #1");
  // TEST_SINGLE(fcvtzu(SubRegSize::i8Bit, DReg::d30, DReg::d29, 7),   "fcvtzu v30.8b, v29.8b, #7");
  TEST_SINGLE(fcvtzu(SubRegSize::i16Bit, DReg::d30, DReg::d29, 1), "fcvtzu v30.4h, v29.4h, #1");
  TEST_SINGLE(fcvtzu(SubRegSize::i16Bit, DReg::d30, DReg::d29, 15), "fcvtzu v30.4h, v29.4h, #15");
  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, DReg::d30, DReg::d29, 1), "fcvtzu v30.2s, v29.2s, #1");
  TEST_SINGLE(fcvtzu(SubRegSize::i32Bit, DReg::d30, DReg::d29, 31), "fcvtzu v30.2s, v29.2s, #31");
  // TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, DReg::d30, DReg::d29, 1),  "fcvtzu v30.1d, v29.1d, #1");
  // TEST_SINGLE(fcvtzu(SubRegSize::i64Bit, DReg::d30, DReg::d29, 63), "fcvtzu v30.1d, v29.1d, #63");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Advanced SIMD vector x indexed element") {
  TEST_SINGLE(smlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlal v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(smlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smlal v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(smlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smlal v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(smlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smlal v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(smlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlal v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(smlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smlal v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(smlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlal2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(smlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smlal2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(smlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smlal2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(smlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smlal2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(smlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlal2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(smlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smlal2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(sqdmlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlal v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(sqdmlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmlal v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(sqdmlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmlal v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(sqdmlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmlal v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(sqdmlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlal v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(sqdmlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmlal v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(sqdmlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlal2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(sqdmlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmlal2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(sqdmlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmlal2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(sqdmlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmlal2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(sqdmlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlal2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(sqdmlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmlal2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(smlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlsl v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(smlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smlsl v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(smlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smlsl v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(smlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smlsl v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(smlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlsl v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(smlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smlsl v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(smlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlsl2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(smlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smlsl2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(smlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smlsl2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(smlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smlsl2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(smlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smlsl2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(smlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smlsl2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(sqdmlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlsl v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(sqdmlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmlsl v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(sqdmlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmlsl v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(sqdmlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmlsl v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(sqdmlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlsl v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(sqdmlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmlsl v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(sqdmlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlsl2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(sqdmlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmlsl2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(sqdmlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmlsl2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(sqdmlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmlsl2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(sqdmlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmlsl2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(sqdmlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmlsl2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(mul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mul v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(mul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "mul v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(mul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "mul v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(mul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "mul v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(mul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mul v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(mul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "mul v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(mul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mul v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(mul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "mul v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(mul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "mul v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(mul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "mul v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(mul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mul v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(mul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "mul v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(smull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smull v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(smull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smull v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(smull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smull v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(smull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smull v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(smull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smull v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(smull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smull v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(smull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smull2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(smull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "smull2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(smull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "smull2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(smull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "smull2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(smull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "smull2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(smull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "smull2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(sqdmull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmull v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(sqdmull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmull v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(sqdmull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmull v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(sqdmull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmull v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(sqdmull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmull v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(sqdmull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmull v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(sqdmull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmull2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(sqdmull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "sqdmull2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(sqdmull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "sqdmull2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(sqdmull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "sqdmull2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(sqdmull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "sqdmull2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(sqdmull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "sqdmull2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqdmulh v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "sqdmulh v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "sqdmulh v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "sqdmulh v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqdmulh v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "sqdmulh v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqdmulh v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "sqdmulh v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "sqdmulh v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "sqdmulh v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqdmulh v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(sqdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "sqdmulh v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmulh v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "sqrdmulh v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "sqrdmulh v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "sqrdmulh v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmulh v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "sqrdmulh v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmulh v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "sqrdmulh v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "sqrdmulh v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "sqrdmulh v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmulh v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(sqrdmulh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "sqrdmulh v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(sdot(QReg::q30, QReg::q29, QReg::q28, 0), "sdot v30.4s, v29.16b, v28.4b[0]");
  TEST_SINGLE(sdot(QReg::q30, QReg::q29, QReg::q28, 3), "sdot v30.4s, v29.16b, v28.4b[3]");

  TEST_SINGLE(sdot(QReg::q30, QReg::q29, QReg::q15, 0), "sdot v30.4s, v29.16b, v15.4b[0]");
  TEST_SINGLE(sdot(QReg::q30, QReg::q29, QReg::q15, 3), "sdot v30.4s, v29.16b, v15.4b[3]");

  TEST_SINGLE(sdot(DReg::d30, DReg::d29, DReg::d28, 0), "sdot v30.2s, v29.8b, v28.4b[0]");
  TEST_SINGLE(sdot(DReg::d30, DReg::d29, DReg::d28, 3), "sdot v30.2s, v29.8b, v28.4b[3]");

  TEST_SINGLE(sdot(DReg::d30, DReg::d29, DReg::d15, 0), "sdot v30.2s, v29.8b, v15.4b[0]");
  TEST_SINGLE(sdot(DReg::d30, DReg::d29, DReg::d15, 3), "sdot v30.2s, v29.8b, v15.4b[3]");

  TEST_SINGLE(fmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmla v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(fmla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "fmla v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(fmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmla v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(fmla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "fmla v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(fmls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmls v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(fmls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "fmls v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(fmls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmls v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(fmls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "fmls v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(fmul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmul v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "fmul v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(fmul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmul v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(fmul(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "fmul v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(sudot(QReg::q30, QReg::q29, QReg::q28, 0), "sudot v30.4s, v29.16b, v28.4b[0]");
  TEST_SINGLE(sudot(QReg::q30, QReg::q29, QReg::q28, 3), "sudot v30.4s, v29.16b, v28.4b[3]");

  TEST_SINGLE(sudot(QReg::q30, QReg::q29, QReg::q15, 0), "sudot v30.4s, v29.16b, v15.4b[0]");
  TEST_SINGLE(sudot(QReg::q30, QReg::q29, QReg::q15, 3), "sudot v30.4s, v29.16b, v15.4b[3]");

  TEST_SINGLE(sudot(DReg::d30, DReg::d29, DReg::d28, 0), "sudot v30.2s, v29.8b, v28.4b[0]");
  TEST_SINGLE(sudot(DReg::d30, DReg::d29, DReg::d28, 3), "sudot v30.2s, v29.8b, v28.4b[3]");

  TEST_SINGLE(sudot(DReg::d30, DReg::d29, DReg::d15, 0), "sudot v30.2s, v29.8b, v15.4b[0]");
  TEST_SINGLE(sudot(DReg::d30, DReg::d29, DReg::d15, 3), "sudot v30.2s, v29.8b, v15.4b[3]");

  // Unimplemented in vixl disassembler
  // TEST_SINGLE(bfdot(QReg::q30, QReg::q29, QReg::q28, 0), "bfdot v30.4s, v29.8h, v28.2h[0]");
  // TEST_SINGLE(bfdot(QReg::q30, QReg::q29, QReg::q28, 3), "bfdot v30.4s, v29.8h, v28.2h[3]");

  // TEST_SINGLE(bfdot(QReg::q30, QReg::q29, QReg::q15, 0), "bfdot v30.4s, v29.8h, v15.2h[0]");
  // TEST_SINGLE(bfdot(QReg::q30, QReg::q29, QReg::q15, 3), "bfdot v30.4s, v29.8h, v15.2h[3]");

  // TEST_SINGLE(bfdot(DReg::d30, DReg::d29, DReg::d28, 0), "bfdot v30.2s, v29.4h, v28.2h[0]");
  // TEST_SINGLE(bfdot(DReg::d30, DReg::d29, DReg::d28, 3), "bfdot v30.2s, v29.4h, v28.2h[3]");

  // TEST_SINGLE(bfdot(DReg::d30, DReg::d29, DReg::d15, 0), "bfdot v30.2s, v29.4h, v15.2h[0]");
  // TEST_SINGLE(bfdot(DReg::d30, DReg::d29, DReg::d15, 3), "bfdot v30.2s, v29.4h, v15.2h[3]");

  TEST_SINGLE(fmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmla v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "fmla v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(fmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmla v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(fmla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "fmla v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(fmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmla v30.2d, v29.2d, v15.d[0]");
  TEST_SINGLE(fmla(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 1), "fmla v30.2d, v29.2d, v15.d[1]");

  // TEST_SINGLE(fmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmla v30.1d, v29.1d, v15.d[0]");
  // TEST_SINGLE(fmla(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 1), "fmla v30.1d, v29.1d, v15.d[1]");

  TEST_SINGLE(fmls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmls v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "fmls v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(fmls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmls v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(fmls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "fmls v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(fmls(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmls v30.2d, v29.2d, v15.d[0]");
  TEST_SINGLE(fmls(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 1), "fmls v30.2d, v29.2d, v15.d[1]");

  // TEST_SINGLE(fmls(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmls v30.1d, v29.1d, v15.d[0]");
  // TEST_SINGLE(fmls(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 1), "fmls v30.1d, v29.1d, v15.d[1]");

  TEST_SINGLE(fmul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmul v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "fmul v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(fmul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmul v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(fmul(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "fmul v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(fmul(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 0), "fmul v30.2d, v29.2d, v15.d[0]");
  TEST_SINGLE(fmul(SubRegSize::i64Bit, QReg::q30, QReg::q29, QReg::q15, 1), "fmul v30.2d, v29.2d, v15.d[1]");

  // TEST_SINGLE(fmul(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 0), "fmul v30.1d, v29.1d, v15.d[0]");
  // TEST_SINGLE(fmul(SubRegSize::i64Bit, DReg::d30, DReg::d29, DReg::d15, 1), "fmul v30.1d, v29.1d, v15.d[1]");

  TEST_SINGLE(fmlal(QReg::q30, QReg::q29, QReg::q15, 0), "fmlal v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(fmlal(QReg::q30, QReg::q29, QReg::q15, 7), "fmlal v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(fmlal(DReg::d30, DReg::d29, DReg::d15, 0), "fmlal v30.2s, v29.2h, v15.h[0]");
  TEST_SINGLE(fmlal(DReg::d30, DReg::d29, DReg::d15, 7), "fmlal v30.2s, v29.2h, v15.h[7]");

  TEST_SINGLE(fmlal2(QReg::q30, QReg::q29, QReg::q15, 0), "fmlal2 v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(fmlal2(QReg::q30, QReg::q29, QReg::q15, 7), "fmlal2 v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(fmlal2(DReg::d30, DReg::d29, DReg::d15, 0), "fmlal2 v30.2s, v29.2h, v15.h[0]");
  TEST_SINGLE(fmlal2(DReg::d30, DReg::d29, DReg::d15, 7), "fmlal2 v30.2s, v29.2h, v15.h[7]");

  TEST_SINGLE(fmlsl(QReg::q30, QReg::q29, QReg::q15, 0), "fmlsl v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(fmlsl(QReg::q30, QReg::q29, QReg::q15, 7), "fmlsl v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(fmlsl(DReg::d30, DReg::d29, DReg::d15, 0), "fmlsl v30.2s, v29.2h, v15.h[0]");
  TEST_SINGLE(fmlsl(DReg::d30, DReg::d29, DReg::d15, 7), "fmlsl v30.2s, v29.2h, v15.h[7]");

  TEST_SINGLE(fmlsl2(QReg::q30, QReg::q29, QReg::q15, 0), "fmlsl2 v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(fmlsl2(QReg::q30, QReg::q29, QReg::q15, 7), "fmlsl2 v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(fmlsl2(DReg::d30, DReg::d29, DReg::d15, 0), "fmlsl2 v30.2s, v29.2h, v15.h[0]");
  TEST_SINGLE(fmlsl2(DReg::d30, DReg::d29, DReg::d15, 7), "fmlsl2 v30.2s, v29.2h, v15.h[7]");

  TEST_SINGLE(usdot(QReg::q30, QReg::q29, QReg::q28, 0), "usdot v30.4s, v29.16b, v28.4b[0]");
  TEST_SINGLE(usdot(QReg::q30, QReg::q29, QReg::q28, 3), "usdot v30.4s, v29.16b, v28.4b[3]");

  TEST_SINGLE(usdot(QReg::q30, QReg::q29, QReg::q15, 0), "usdot v30.4s, v29.16b, v15.4b[0]");
  TEST_SINGLE(usdot(QReg::q30, QReg::q29, QReg::q15, 3), "usdot v30.4s, v29.16b, v15.4b[3]");

  TEST_SINGLE(usdot(DReg::d30, DReg::d29, DReg::d28, 0), "usdot v30.2s, v29.8b, v28.4b[0]");
  TEST_SINGLE(usdot(DReg::d30, DReg::d29, DReg::d28, 3), "usdot v30.2s, v29.8b, v28.4b[3]");

  TEST_SINGLE(usdot(DReg::d30, DReg::d29, DReg::d15, 0), "usdot v30.2s, v29.8b, v15.4b[0]");
  TEST_SINGLE(usdot(DReg::d30, DReg::d29, DReg::d15, 3), "usdot v30.2s, v29.8b, v15.4b[3]");

  // Unimplemented in vixl disassembler
  // TEST_SINGLE(bfmlalb(VReg::v30, VReg::v29, VReg::v15, 0), "bfmlalb v30.4s, v29.8h, v15.h[0]");
  // TEST_SINGLE(bfmlalb(VReg::v30, VReg::v29, VReg::v15, 7), "bfmlalb v30.4s, v29.8h, v15.h[7]");

  // TEST_SINGLE(bfmlalt(VReg::v30, VReg::v29, VReg::v15, 0), "bfmlalt v30.4s, v29.8h, v15.h[0]");
  // TEST_SINGLE(bfmlalt(VReg::v30, VReg::v29, VReg::v15, 7), "bfmlalt v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(mla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mla v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(mla(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "mla v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(mla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "mla v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(mla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "mla v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(mla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mla v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(mla(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "mla v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(mla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mla v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(mla(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "mla v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(mla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "mla v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(mla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "mla v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(mla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mla v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(mla(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "mla v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(umlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlal v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(umlal(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umlal v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(umlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umlal v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(umlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umlal v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(umlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlal v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(umlal(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umlal v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(umlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlal2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(umlal2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umlal2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(umlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umlal2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(umlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umlal2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(umlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlal2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(umlal2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umlal2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(mls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mls v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(mls(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "mls v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(mls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "mls v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(mls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "mls v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(mls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "mls v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(mls(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "mls v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(mls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mls v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(mls(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "mls v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(mls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "mls v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(mls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "mls v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(mls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "mls v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(mls(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "mls v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(umlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlsl v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(umlsl(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umlsl v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(umlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umlsl v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(umlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umlsl v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(umlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlsl v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(umlsl(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umlsl v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(umlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlsl2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(umlsl2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umlsl2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(umlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umlsl2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(umlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umlsl2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(umlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umlsl2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(umlsl2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umlsl2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(umull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umull v30.4s, v29.4h, v15.h[0]");
  TEST_SINGLE(umull(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umull v30.4s, v29.4h, v15.h[7]");

  TEST_SINGLE(umull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umull v30.2d, v29.2s, v28.s[0]");
  TEST_SINGLE(umull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umull v30.2d, v29.2s, v28.s[3]");

  TEST_SINGLE(umull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umull v30.2d, v29.2s, v15.s[0]");
  TEST_SINGLE(umull(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umull v30.2d, v29.2s, v15.s[3]");

  TEST_SINGLE(umull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umull2 v30.4s, v29.8h, v15.h[0]");
  TEST_SINGLE(umull2(SubRegSize::i32Bit, VReg::v30, VReg::v29, VReg::v15, 7), "umull2 v30.4s, v29.8h, v15.h[7]");

  TEST_SINGLE(umull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 0), "umull2 v30.2d, v29.4s, v28.s[0]");
  TEST_SINGLE(umull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v28, 3), "umull2 v30.2d, v29.4s, v28.s[3]");

  TEST_SINGLE(umull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 0), "umull2 v30.2d, v29.4s, v15.s[0]");
  TEST_SINGLE(umull2(SubRegSize::i64Bit, VReg::v30, VReg::v29, VReg::v15, 3), "umull2 v30.2d, v29.4s, v15.s[3]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmlah v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "sqrdmlah v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "sqrdmlah v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "sqrdmlah v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmlah v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "sqrdmlah v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmlah v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "sqrdmlah v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "sqrdmlah v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "sqrdmlah v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmlah v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(sqrdmlah(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "sqrdmlah v30.2s, v29.2s, v15.s[3]");

  TEST_SINGLE(udot(QReg::q30, QReg::q29, QReg::q28, 0), "udot v30.4s, v29.16b, v28.4b[0]");
  TEST_SINGLE(udot(QReg::q30, QReg::q29, QReg::q28, 3), "udot v30.4s, v29.16b, v28.4b[3]");

  TEST_SINGLE(udot(QReg::q30, QReg::q29, QReg::q15, 0), "udot v30.4s, v29.16b, v15.4b[0]");
  TEST_SINGLE(udot(QReg::q30, QReg::q29, QReg::q15, 3), "udot v30.4s, v29.16b, v15.4b[3]");

  TEST_SINGLE(udot(DReg::d30, DReg::d29, DReg::d28, 0), "udot v30.2s, v29.8b, v28.4b[0]");
  TEST_SINGLE(udot(DReg::d30, DReg::d29, DReg::d28, 3), "udot v30.2s, v29.8b, v28.4b[3]");

  TEST_SINGLE(udot(DReg::d30, DReg::d29, DReg::d15, 0), "udot v30.2s, v29.8b, v15.4b[0]");
  TEST_SINGLE(udot(DReg::d30, DReg::d29, DReg::d15, 3), "udot v30.2s, v29.8b, v15.4b[3]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmlsh v30.8h, v29.8h, v15.h[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, QReg::q30, QReg::q29, QReg::q15, 7), "sqrdmlsh v30.8h, v29.8h, v15.h[7]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 0), "sqrdmlsh v30.4s, v29.4s, v28.s[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q28, 3), "sqrdmlsh v30.4s, v29.4s, v28.s[3]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 0), "sqrdmlsh v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, QReg::q30, QReg::q29, QReg::q15, 3), "sqrdmlsh v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmlsh v30.4h, v29.4h, v15.h[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i16Bit, DReg::d30, DReg::d29, DReg::d15, 7), "sqrdmlsh v30.4h, v29.4h, v15.h[7]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 0), "sqrdmlsh v30.2s, v29.2s, v28.s[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d28, 3), "sqrdmlsh v30.2s, v29.2s, v28.s[3]");

  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 0), "sqrdmlsh v30.2s, v29.2s, v15.s[0]");
  TEST_SINGLE(sqrdmlsh(SubRegSize::i32Bit, DReg::d30, DReg::d29, DReg::d15, 3), "sqrdmlsh v30.2s, v29.2s, v15.s[3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic three-register, imm2") {
  TEST_SINGLE(sm3tt1a(VReg::v30, VReg::v29, VReg::v15, 0), "sm3tt1a v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sm3tt1a(VReg::v30, VReg::v29, VReg::v15, 1), "sm3tt1a v30.4s, v29.4s, v15.s[1]");
  TEST_SINGLE(sm3tt1a(VReg::v30, VReg::v29, VReg::v15, 2), "sm3tt1a v30.4s, v29.4s, v15.s[2]");
  TEST_SINGLE(sm3tt1a(VReg::v30, VReg::v29, VReg::v15, 3), "sm3tt1a v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sm3tt1b(VReg::v30, VReg::v29, VReg::v15, 0), "sm3tt1b v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sm3tt1b(VReg::v30, VReg::v29, VReg::v15, 1), "sm3tt1b v30.4s, v29.4s, v15.s[1]");
  TEST_SINGLE(sm3tt1b(VReg::v30, VReg::v29, VReg::v15, 2), "sm3tt1b v30.4s, v29.4s, v15.s[2]");
  TEST_SINGLE(sm3tt1b(VReg::v30, VReg::v29, VReg::v15, 3), "sm3tt1b v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sm3tt2a(VReg::v30, VReg::v29, VReg::v15, 0), "sm3tt2a v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sm3tt2a(VReg::v30, VReg::v29, VReg::v15, 1), "sm3tt2a v30.4s, v29.4s, v15.s[1]");
  TEST_SINGLE(sm3tt2a(VReg::v30, VReg::v29, VReg::v15, 2), "sm3tt2a v30.4s, v29.4s, v15.s[2]");
  TEST_SINGLE(sm3tt2a(VReg::v30, VReg::v29, VReg::v15, 3), "sm3tt2a v30.4s, v29.4s, v15.s[3]");

  TEST_SINGLE(sm3tt2b(VReg::v30, VReg::v29, VReg::v15, 0), "sm3tt2b v30.4s, v29.4s, v15.s[0]");
  TEST_SINGLE(sm3tt2b(VReg::v30, VReg::v29, VReg::v15, 1), "sm3tt2b v30.4s, v29.4s, v15.s[1]");
  TEST_SINGLE(sm3tt2b(VReg::v30, VReg::v29, VReg::v15, 2), "sm3tt2b v30.4s, v29.4s, v15.s[2]");
  TEST_SINGLE(sm3tt2b(VReg::v30, VReg::v29, VReg::v15, 3), "sm3tt2b v30.4s, v29.4s, v15.s[3]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic three-register SHA 512") {
  TEST_SINGLE(sha512h(VReg::v30, VReg::v29, VReg::v15), "sha512h q30, q29, v15.2d");
  TEST_SINGLE(sha512h2(VReg::v30, VReg::v29, VReg::v15), "sha512h2 q30, q29, v15.2d");
  TEST_SINGLE(sha512su1(VReg::v30, VReg::v29, VReg::v15), "sha512su1 v30.2d, v29.2d, v15.2d");
  TEST_SINGLE(rax1(VReg::v30, VReg::v29, VReg::v15), "rax1 v30.2d, v29.2d, v15.2d");
  TEST_SINGLE(sm3partw1(VReg::v30, VReg::v29, VReg::v15), "sm3partw1 v30.4s, v29.4s, v15.4s");
  TEST_SINGLE(sm3partw2(VReg::v30, VReg::v29, VReg::v15), "sm3partw2 v30.4s, v29.4s, v15.4s");
  TEST_SINGLE(sm4ekey(VReg::v30, VReg::v29, VReg::v15), "sm4ekey v30.4s, v29.4s, v15.4s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic four-register") {
  TEST_SINGLE(eor3(VReg::v30, VReg::v29, VReg::v15, VReg::v7), "eor3 v30.16b, v29.16b, v15.16b, v7.16b");
  TEST_SINGLE(bcax(VReg::v30, VReg::v29, VReg::v15, VReg::v7), "bcax v30.16b, v29.16b, v15.16b, v7.16b");
  TEST_SINGLE(sm3ss1(VReg::v30, VReg::v29, VReg::v15, VReg::v7), "sm3ss1 v30.4s, v29.4s, v15.4s, v7.4s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Cryptographic two-register SHA 512") {
  TEST_SINGLE(sha512su0(VReg::v30, VReg::v29), "sha512su0 v30.2d, v29.2d");
  TEST_SINGLE(sm4e(VReg::v30, VReg::v29), "sm4e v30.4s, v29.4s");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Conversion between floating-point and fixed-point") {
  TEST_SINGLE(scvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "scvtf h29, w30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "scvtf h29, w30, #32");
  TEST_SINGLE(scvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "scvtf s29, w30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "scvtf s29, w30, #32");
  TEST_SINGLE(scvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "scvtf d29, w30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "scvtf d29, w30, #32");

  TEST_SINGLE(scvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "scvtf h29, x30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "scvtf h29, x30, #64");
  TEST_SINGLE(scvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "scvtf s29, x30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "scvtf s29, x30, #64");
  TEST_SINGLE(scvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "scvtf d29, x30, #1");
  TEST_SINGLE(scvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "scvtf d29, x30, #64");

  TEST_SINGLE(ucvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "ucvtf h29, w30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "ucvtf h29, w30, #32");
  TEST_SINGLE(ucvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "ucvtf s29, w30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "ucvtf s29, w30, #32");
  TEST_SINGLE(ucvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i32Bit, Reg::r30, 1), "ucvtf d29, w30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i32Bit, Reg::r30, 32), "ucvtf d29, w30, #32");

  TEST_SINGLE(ucvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "ucvtf h29, x30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i16Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "ucvtf h29, x30, #64");
  TEST_SINGLE(ucvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "ucvtf s29, x30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i32Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "ucvtf s29, x30, #64");
  TEST_SINGLE(ucvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i64Bit, Reg::r30, 1), "ucvtf d29, x30, #1");
  TEST_SINGLE(ucvtf(ScalarRegSize::i64Bit, VReg::v29, Size::i64Bit, Reg::r30, 64), "ucvtf d29, x30, #64");

  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 1), "fcvtzs w30, h29, #1");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 32), "fcvtzs w30, h29, #32");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 1), "fcvtzs w30, s29, #1");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 32), "fcvtzs w30, s29, #32");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 1), "fcvtzs w30, d29, #1");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 32), "fcvtzs w30, d29, #32");

  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 1), "fcvtzs x30, h29, #1");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 64), "fcvtzs x30, h29, #64");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 1), "fcvtzs x30, s29, #1");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 64), "fcvtzs x30, s29, #64");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 1), "fcvtzs x30, d29, #1");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 64), "fcvtzs x30, d29, #64");

  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 1), "fcvtzu w30, h29, #1");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 32), "fcvtzu w30, h29, #32");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 1), "fcvtzu w30, s29, #1");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 32), "fcvtzu w30, s29, #32");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 1), "fcvtzu w30, d29, #1");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 32), "fcvtzu w30, d29, #32");

  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 1), "fcvtzu x30, h29, #1");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i16Bit, VReg::v29, 64), "fcvtzu x30, h29, #64");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 1), "fcvtzu x30, s29, #1");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i32Bit, VReg::v29, 64), "fcvtzu x30, s29, #64");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 1), "fcvtzu x30, d29, #1");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r30, ScalarRegSize::i64Bit, VReg::v29, 64), "fcvtzu x30, d29, #64");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: ASIMD: Conversion between floating-point and integer") {
  TEST_SINGLE(fcvtns(Size::i32Bit, Reg::r29, HReg::h30), "fcvtns w29, h30");
  TEST_SINGLE(fcvtns(Size::i64Bit, Reg::r29, HReg::h30), "fcvtns x29, h30");
  TEST_SINGLE(fcvtns(Size::i32Bit, Reg::r29, SReg::s30), "fcvtns w29, s30");
  TEST_SINGLE(fcvtns(Size::i64Bit, Reg::r29, SReg::s30), "fcvtns x29, s30");
  TEST_SINGLE(fcvtns(Size::i32Bit, Reg::r29, DReg::d30), "fcvtns w29, d30");
  TEST_SINGLE(fcvtns(Size::i64Bit, Reg::r29, DReg::d30), "fcvtns x29, d30");

  TEST_SINGLE(fcvtnu(Size::i32Bit, Reg::r29, HReg::h30), "fcvtnu w29, h30");
  TEST_SINGLE(fcvtnu(Size::i64Bit, Reg::r29, HReg::h30), "fcvtnu x29, h30");
  TEST_SINGLE(fcvtnu(Size::i32Bit, Reg::r29, SReg::s30), "fcvtnu w29, s30");
  TEST_SINGLE(fcvtnu(Size::i64Bit, Reg::r29, SReg::s30), "fcvtnu x29, s30");
  TEST_SINGLE(fcvtnu(Size::i32Bit, Reg::r29, DReg::d30), "fcvtnu w29, d30");
  TEST_SINGLE(fcvtnu(Size::i64Bit, Reg::r29, DReg::d30), "fcvtnu x29, d30");

  TEST_SINGLE(scvtf(Size::i32Bit, HReg::h30, Reg::r29), "scvtf h30, w29");
  TEST_SINGLE(scvtf(Size::i64Bit, HReg::h30, Reg::r29), "scvtf h30, x29");
  TEST_SINGLE(scvtf(Size::i32Bit, SReg::s30, Reg::r29), "scvtf s30, w29");
  TEST_SINGLE(scvtf(Size::i64Bit, SReg::s30, Reg::r29), "scvtf s30, x29");
  TEST_SINGLE(scvtf(Size::i32Bit, DReg::d30, Reg::r29), "scvtf d30, w29");
  TEST_SINGLE(scvtf(Size::i64Bit, DReg::d30, Reg::r29), "scvtf d30, x29");

  TEST_SINGLE(ucvtf(Size::i32Bit, HReg::h30, Reg::r29), "ucvtf h30, w29");
  TEST_SINGLE(ucvtf(Size::i64Bit, HReg::h30, Reg::r29), "ucvtf h30, x29");
  TEST_SINGLE(ucvtf(Size::i32Bit, SReg::s30, Reg::r29), "ucvtf s30, w29");
  TEST_SINGLE(ucvtf(Size::i64Bit, SReg::s30, Reg::r29), "ucvtf s30, x29");
  TEST_SINGLE(ucvtf(Size::i32Bit, DReg::d30, Reg::r29), "ucvtf d30, w29");
  TEST_SINGLE(ucvtf(Size::i64Bit, DReg::d30, Reg::r29), "ucvtf d30, x29");

  TEST_SINGLE(fcvtas(Size::i32Bit, Reg::r29, HReg::h30), "fcvtas w29, h30");
  TEST_SINGLE(fcvtas(Size::i64Bit, Reg::r29, HReg::h30), "fcvtas x29, h30");
  TEST_SINGLE(fcvtas(Size::i32Bit, Reg::r29, SReg::s30), "fcvtas w29, s30");
  TEST_SINGLE(fcvtas(Size::i64Bit, Reg::r29, SReg::s30), "fcvtas x29, s30");
  TEST_SINGLE(fcvtas(Size::i32Bit, Reg::r29, DReg::d30), "fcvtas w29, d30");
  TEST_SINGLE(fcvtas(Size::i64Bit, Reg::r29, DReg::d30), "fcvtas x29, d30");

  TEST_SINGLE(fcvtau(Size::i32Bit, Reg::r29, HReg::h30), "fcvtau w29, h30");
  TEST_SINGLE(fcvtau(Size::i64Bit, Reg::r29, HReg::h30), "fcvtau x29, h30");
  TEST_SINGLE(fcvtau(Size::i32Bit, Reg::r29, SReg::s30), "fcvtau w29, s30");
  TEST_SINGLE(fcvtau(Size::i64Bit, Reg::r29, SReg::s30), "fcvtau x29, s30");
  TEST_SINGLE(fcvtau(Size::i32Bit, Reg::r29, DReg::d30), "fcvtau w29, d30");
  TEST_SINGLE(fcvtau(Size::i64Bit, Reg::r29, DReg::d30), "fcvtau x29, d30");

  TEST_SINGLE(fmov(Size::i32Bit, Reg::r29, HReg::h30), "fmov w29, h30");
  TEST_SINGLE(fmov(Size::i64Bit, Reg::r29, HReg::h30), "fmov x29, h30");
  TEST_SINGLE(fmov(Size::i32Bit, Reg::r29, SReg::s30), "fmov w29, s30");
  // TEST_SINGLE(fmov(Size::i64Bit, Reg::r29, SReg::s30), "fmov x29, s30");
  // TEST_SINGLE(fmov(Size::i32Bit, Reg::r29, DReg::d30), "fmov w29, d30");
  TEST_SINGLE(fmov(Size::i64Bit, Reg::r29, DReg::d30), "fmov x29, d30");

  // TEST_SINGLE(fmov(Size::i32Bit, Reg::r29, VReg::v30, false), "fmov w29, s30");
  TEST_SINGLE(fmov(Size::i64Bit, Reg::r29, VReg::v30, false), "fmov x29, d30");

  // TEST_SINGLE(fmov(Size::i32Bit, Reg::r29, VReg::v30, true), "fmov w29, s30");
  TEST_SINGLE(fmov(Size::i64Bit, Reg::r29, VReg::v30, true), "fmov x29, v30.D[1]");

  TEST_SINGLE(fmov(Size::i32Bit, HReg::h30, Reg::r29), "fmov h30, w29");
  TEST_SINGLE(fmov(Size::i64Bit, HReg::h30, Reg::r29), "fmov h30, x29");
  TEST_SINGLE(fmov(Size::i32Bit, SReg::s30, Reg::r29), "fmov s30, w29");
  // TEST_SINGLE(fmov(Size::i64Bit, SReg::s30, Reg::r29), "fmov s30, x29");
  // TEST_SINGLE(fmov(Size::i32Bit, DReg::d30, Reg::r29), "fmov d30, w29");
  TEST_SINGLE(fmov(Size::i64Bit, DReg::d30, Reg::r29), "fmov d30, x29");

  // TEST_SINGLE(fmov(Size::i32Bit, VReg::v30, Reg::r29, false), "fmov s30, w29");
  TEST_SINGLE(fmov(Size::i64Bit, VReg::v30, Reg::r29, false), "fmov d30, x29");

  // TEST_SINGLE(fmov(Size::i32Bit, VReg::v30, Reg::r29, true), "fmov d30, x29");
  TEST_SINGLE(fmov(Size::i64Bit, VReg::v30, Reg::r29, true), "fmov v30.D[1], x29");

  TEST_SINGLE(fcvtps(Size::i32Bit, Reg::r29, HReg::h30), "fcvtps w29, h30");
  TEST_SINGLE(fcvtps(Size::i64Bit, Reg::r29, HReg::h30), "fcvtps x29, h30");
  TEST_SINGLE(fcvtps(Size::i32Bit, Reg::r29, SReg::s30), "fcvtps w29, s30");
  TEST_SINGLE(fcvtps(Size::i64Bit, Reg::r29, SReg::s30), "fcvtps x29, s30");
  TEST_SINGLE(fcvtps(Size::i32Bit, Reg::r29, DReg::d30), "fcvtps w29, d30");
  TEST_SINGLE(fcvtps(Size::i64Bit, Reg::r29, DReg::d30), "fcvtps x29, d30");

  TEST_SINGLE(fcvtpu(Size::i32Bit, Reg::r29, HReg::h30), "fcvtpu w29, h30");
  TEST_SINGLE(fcvtpu(Size::i64Bit, Reg::r29, HReg::h30), "fcvtpu x29, h30");
  TEST_SINGLE(fcvtpu(Size::i32Bit, Reg::r29, SReg::s30), "fcvtpu w29, s30");
  TEST_SINGLE(fcvtpu(Size::i64Bit, Reg::r29, SReg::s30), "fcvtpu x29, s30");
  TEST_SINGLE(fcvtpu(Size::i32Bit, Reg::r29, DReg::d30), "fcvtpu w29, d30");
  TEST_SINGLE(fcvtpu(Size::i64Bit, Reg::r29, DReg::d30), "fcvtpu x29, d30");

  TEST_SINGLE(fcvtms(Size::i32Bit, Reg::r29, HReg::h30), "fcvtms w29, h30");
  TEST_SINGLE(fcvtms(Size::i64Bit, Reg::r29, HReg::h30), "fcvtms x29, h30");
  TEST_SINGLE(fcvtms(Size::i32Bit, Reg::r29, SReg::s30), "fcvtms w29, s30");
  TEST_SINGLE(fcvtms(Size::i64Bit, Reg::r29, SReg::s30), "fcvtms x29, s30");
  TEST_SINGLE(fcvtms(Size::i32Bit, Reg::r29, DReg::d30), "fcvtms w29, d30");
  TEST_SINGLE(fcvtms(Size::i64Bit, Reg::r29, DReg::d30), "fcvtms x29, d30");

  TEST_SINGLE(fcvtmu(Size::i32Bit, Reg::r29, HReg::h30), "fcvtmu w29, h30");
  TEST_SINGLE(fcvtmu(Size::i64Bit, Reg::r29, HReg::h30), "fcvtmu x29, h30");
  TEST_SINGLE(fcvtmu(Size::i32Bit, Reg::r29, SReg::s30), "fcvtmu w29, s30");
  TEST_SINGLE(fcvtmu(Size::i64Bit, Reg::r29, SReg::s30), "fcvtmu x29, s30");
  TEST_SINGLE(fcvtmu(Size::i32Bit, Reg::r29, DReg::d30), "fcvtmu w29, d30");
  TEST_SINGLE(fcvtmu(Size::i64Bit, Reg::r29, DReg::d30), "fcvtmu x29, d30");

  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r29, HReg::h30), "fcvtzs w29, h30");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r29, HReg::h30), "fcvtzs x29, h30");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r29, SReg::s30), "fcvtzs w29, s30");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r29, SReg::s30), "fcvtzs x29, s30");
  TEST_SINGLE(fcvtzs(Size::i32Bit, Reg::r29, DReg::d30), "fcvtzs w29, d30");
  TEST_SINGLE(fcvtzs(Size::i64Bit, Reg::r29, DReg::d30), "fcvtzs x29, d30");

  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r29, HReg::h30), "fcvtzu w29, h30");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r29, HReg::h30), "fcvtzu x29, h30");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r29, SReg::s30), "fcvtzu w29, s30");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r29, SReg::s30), "fcvtzu x29, s30");
  TEST_SINGLE(fcvtzu(Size::i32Bit, Reg::r29, DReg::d30), "fcvtzu w29, d30");
  TEST_SINGLE(fcvtzu(Size::i64Bit, Reg::r29, DReg::d30), "fcvtzu x29, d30");
}
