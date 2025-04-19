// SPDX-License-Identifier: MIT
#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Compare and swap pair") {
  TEST_SINGLE(casp(Size::i32Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "casp w28, w29, w26, w27, [x30]");
  TEST_SINGLE(casp(Size::i64Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "casp x28, x29, x26, x27, [x30]");

  TEST_SINGLE(caspa(Size::i32Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspa w28, w29, w26, w27, [x30]");
  TEST_SINGLE(caspa(Size::i64Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspa x28, x29, x26, x27, [x30]");

  TEST_SINGLE(caspl(Size::i32Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspl w28, w29, w26, w27, [x30]");
  TEST_SINGLE(caspl(Size::i64Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspl x28, x29, x26, x27, [x30]");

  TEST_SINGLE(caspal(Size::i32Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspal w28, w29, w26, w27, [x30]");
  TEST_SINGLE(caspal(Size::i64Bit, Reg::r28, Reg::r29, Reg::r26, Reg::r27, Reg::r30), "caspal x28, x29, x26, x27, [x30]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Advanced SIMD load/store multiple structures") {
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, Reg::r30), "ld1 {v26.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, Reg::r30), "ld1 {v26.8b}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, Reg::r30), "ld1 {v26.8h}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, Reg::r30), "ld1 {v26.4h}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, Reg::r30), "ld1 {v26.4s}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, Reg::r30), "ld1 {v26.2s}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, Reg::r30), "ld1 {v26.2d}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, Reg::r30), "ld1 {v26.1d}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30), "ld1 {v31.16b, v0.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30), "ld1 {v31.8b, v0.8b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30), "ld1 {v26.16b, v27.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30), "ld1 {v26.8b, v27.8b}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30), "ld1 {v26.8h, v27.8h}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30), "ld1 {v26.4h, v27.4h}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30), "ld1 {v26.4s, v27.4s}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30), "ld1 {v26.2s, v27.2s}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30), "ld1 {v26.2d, v27.2d}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30), "ld1 {v26.1d, v27.1d}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30), "ld1 {v31.16b, v0.16b, v1.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30), "ld1 {v31.8b, v0.8b, v1.8b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld1 {v26.16b, v27.16b, v28.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld1 {v26.8b, v27.8b, v28.8b}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld1 {v26.8h, v27.8h, v28.8h}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld1 {v26.4h, v27.4h, v28.4h}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld1 {v26.4s, v27.4s, v28.4s}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld1 {v26.2s, v27.2s, v28.2s}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld1 {v26.2d, v27.2d, v28.2d}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld1 {v26.1d, v27.1d, v28.1d}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30), "ld1 {v31.16b, v0.16b, v1.16b, v2.16b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30), "ld1 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld1 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                            "[x30]");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld1 {v26.8b, v27.8b, v28.8b, v29.8b}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld1 {v26.8h, v27.8h, v28.8h, v29.8h}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld1 {v26.4h, v27.4h, v28.4h, v29.4h}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld1 {v26.4s, v27.4s, v28.4s, v29.4s}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld1 {v26.2s, v27.2s, v28.2s, v29.2s}, [x30]");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld1 {v26.2d, v27.2d, v28.2d, v29.2d}, [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld1 {v26.1d, v27.1d, v28.1d, v29.1d}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, Reg::r30), "st1 {v26.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, Reg::r30), "st1 {v26.8b}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, Reg::r30), "st1 {v26.8h}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, Reg::r30), "st1 {v26.4h}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, Reg::r30), "st1 {v26.4s}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, Reg::r30), "st1 {v26.2s}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, Reg::r30), "st1 {v26.2d}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, Reg::r30), "st1 {v26.1d}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30), "st1 {v31.16b, v0.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30), "st1 {v31.8b, v0.8b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30), "st1 {v26.16b, v27.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30), "st1 {v26.8b, v27.8b}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30), "st1 {v26.8h, v27.8h}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30), "st1 {v26.4h, v27.4h}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30), "st1 {v26.4s, v27.4s}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30), "st1 {v26.2s, v27.2s}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30), "st1 {v26.2d, v27.2d}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30), "st1 {v26.1d, v27.1d}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30), "st1 {v31.16b, v0.16b, v1.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30), "st1 {v31.8b, v0.8b, v1.8b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st1 {v26.16b, v27.16b, v28.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st1 {v26.8b, v27.8b, v28.8b}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st1 {v26.8h, v27.8h, v28.8h}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st1 {v26.4h, v27.4h, v28.4h}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st1 {v26.4s, v27.4s, v28.4s}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st1 {v26.2s, v27.2s, v28.2s}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st1 {v26.2d, v27.2d, v28.2d}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st1 {v26.1d, v27.1d, v28.1d}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30), "st1 {v31.16b, v0.16b, v1.16b, v2.16b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30), "st1 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st1 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                            "[x30]");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st1 {v26.8b, v27.8b, v28.8b, v29.8b}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st1 {v26.8h, v27.8h, v28.8h, v29.8h}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st1 {v26.4h, v27.4h, v28.4h, v29.4h}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st1 {v26.4s, v27.4s, v28.4s, v29.4s}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st1 {v26.2s, v27.2s, v28.2s, v29.2s}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st1 {v26.2d, v27.2d, v28.2d, v29.2d}, [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st1 {v26.1d, v27.1d, v28.1d, v29.1d}, [x30]");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30), "ld2 {v31.16b, v0.16b}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30), "ld2 {v31.8b, v0.8b}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2 {v26.16b, v27.16b}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2 {v26.8b, v27.8b}, [x30]");

  TEST_SINGLE(ld2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2 {v26.8h, v27.8h}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2 {v26.4h, v27.4h}, [x30]");

  TEST_SINGLE(ld2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2 {v26.4s, v27.4s}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2 {v26.2s, v27.2s}, [x30]");

  TEST_SINGLE(ld2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2 {v26.2d, v27.2d}, [x30]");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30), "st2 {v31.16b, v0.16b}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30), "st2 {v31.8b, v0.8b}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30), "st2 {v26.16b, v27.16b}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30), "st2 {v26.8b, v27.8b}, [x30]");

  TEST_SINGLE(st2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30), "st2 {v26.8h, v27.8h}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30), "st2 {v26.4h, v27.4h}, [x30]");

  TEST_SINGLE(st2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30), "st2 {v26.4s, v27.4s}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30), "st2 {v26.2s, v27.2s}, [x30]");

  TEST_SINGLE(st2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30), "st2 {v26.2d, v27.2d}, [x30]");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30), "ld3 {v31.16b, v0.16b, v1.16b}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30), "ld3 {v31.8b, v0.8b, v1.8b}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3 {v26.16b, v27.16b, v28.16b}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3 {v26.8b, v27.8b, v28.8b}, [x30]");

  TEST_SINGLE(ld3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3 {v26.8h, v27.8h, v28.8h}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3 {v26.4h, v27.4h, v28.4h}, [x30]");

  TEST_SINGLE(ld3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3 {v26.4s, v27.4s, v28.4s}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3 {v26.2s, v27.2s, v28.2s}, [x30]");

  TEST_SINGLE(ld3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3 {v26.2d, v27.2d, v28.2d}, [x30]");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30), "st3 {v31.16b, v0.16b, v1.16b}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30), "st3 {v31.8b, v0.8b, v1.8b}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st3 {v26.16b, v27.16b, v28.16b}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st3 {v26.8b, v27.8b, v28.8b}, [x30]");

  TEST_SINGLE(st3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st3 {v26.8h, v27.8h, v28.8h}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st3 {v26.4h, v27.4h, v28.4h}, [x30]");

  TEST_SINGLE(st3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st3 {v26.4s, v27.4s, v28.4s}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "st3 {v26.2s, v27.2s, v28.2s}, [x30]");

  TEST_SINGLE(st3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "st3 {v26.2d, v27.2d, v28.2d}, [x30]");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30), "ld4 {v31.16b, v0.16b, v1.16b, v2.16b}, [x30]");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30), "ld4 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30]");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                            "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4 {v26.8b, v27.8b, v28.8b, v29.8b}, [x30]");

  TEST_SINGLE(ld4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4 {v26.8h, v27.8h, v28.8h, v29.8h}, [x30]");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4 {v26.4h, v27.4h, v28.4h, v29.4h}, [x30]");

  TEST_SINGLE(ld4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4 {v26.4s, v27.4s, v28.4s, v29.4s}, [x30]");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4 {v26.2s, v27.2s, v28.2s, v29.2s}, [x30]");

  TEST_SINGLE(ld4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4 {v26.2d, v27.2d, v28.2d, v29.2d}, [x30]");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30), "st4 {v31.16b, v0.16b, v1.16b, v2.16b}, [x30]");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30), "st4 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30]");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st4 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                            "[x30]");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st4 {v26.8b, v27.8b, v28.8b, v29.8b}, [x30]");

  TEST_SINGLE(st4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st4 {v26.8h, v27.8h, v28.8h, v29.8h}, [x30]");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st4 {v26.4h, v27.4h, v28.4h, v29.4h}, [x30]");

  TEST_SINGLE(st4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st4 {v26.4s, v27.4s, v28.4s, v29.4s}, [x30]");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "st4 {v26.2s, v27.2s, v28.2s, v29.2s}, [x30]");

  TEST_SINGLE(st4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "st4 {v26.2d, v27.2d, v28.2d, v29.2d}, [x30]");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "unallocated (NEONLoadStoreMultiStruct)");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Advanced SIMD load/store multiple structures (post-indexed)") {
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1 {v26.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1 {v26.8b}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1 {v26.8h}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1 {v26.4h}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1 {v26.4s}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1 {v26.2s}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1 {v26.2d}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1 {v26.1d}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, Reg::r30, 16), "ld1 {v26.16b}, [x30], #16");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, Reg::r30, 8), "ld1 {v26.8b}, [x30], #8");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, Reg::r30, 16), "ld1 {v26.8h}, [x30], #16");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, Reg::r30, 8), "ld1 {v26.4h}, [x30], #8");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, Reg::r30, 16), "ld1 {v26.4s}, [x30], #16");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, Reg::r30, 8), "ld1 {v26.2s}, [x30], #8");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, Reg::r30, 16), "ld1 {v26.2d}, [x30], #16");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, Reg::r30, 8), "ld1 {v26.1d}, [x30], #8");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, Reg::r29), "ld1 {v31.16b, v0.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, Reg::r29), "ld1 {v31.8b, v0.8b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld1 {v26.16b, v27.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld1 {v26.8b, v27.8b}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld1 {v26.8h, v27.8h}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld1 {v26.4h, v27.4h}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld1 {v26.4s, v27.4s}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld1 {v26.2s, v27.2s}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld1 {v26.2d, v27.2d}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld1 {v26.1d, v27.1d}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, 32), "ld1 {v31.16b, v0.16b}, [x30], #32");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, 16), "ld1 {v31.8b, v0.8b}, [x30], #16");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld1 {v26.16b, v27.16b}, [x30], #32");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld1 {v26.8b, v27.8b}, [x30], #16");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld1 {v26.8h, v27.8h}, [x30], #32");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld1 {v26.4h, v27.4h}, [x30], #16");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld1 {v26.4s, v27.4s}, [x30], #32");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld1 {v26.2s, v27.2s}, [x30], #16");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld1 {v26.2d, v27.2d}, [x30], #32");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld1 {v26.1d, v27.1d}, [x30], #16");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, Reg::r29), "ld1 {v31.16b, v0.16b, v1.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, Reg::r29), "ld1 {v31.8b, v0.8b, v1.8b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld1 {v26.16b, v27.16b, v28.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld1 {v26.8b, v27.8b, v28.8b}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld1 {v26.8h, v27.8h, v28.8h}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld1 {v26.4h, v27.4h, v28.4h}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld1 {v26.4s, v27.4s, v28.4s}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld1 {v26.2s, v27.2s, v28.2s}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld1 {v26.2d, v27.2d, v28.2d}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld1 {v26.1d, v27.1d, v28.1d}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, 48), "ld1 {v31.16b, v0.16b, v1.16b}, [x30], #48");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, 24), "ld1 {v31.8b, v0.8b, v1.8b}, [x30], #24");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld1 {v26.16b, v27.16b, v28.16b}, [x30], #48");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld1 {v26.8b, v27.8b, v28.8b}, [x30], #24");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld1 {v26.8h, v27.8h, v28.8h}, [x30], #48");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld1 {v26.4h, v27.4h, v28.4h}, [x30], #24");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld1 {v26.4s, v27.4s, v28.4s}, [x30], #48");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld1 {v26.2s, v27.2s, v28.2s}, [x30], #24");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld1 {v26.2d, v27.2d, v28.2d}, [x30], #48");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld1 {v26.1d, v27.1d, v28.1d}, [x30], #24");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, Reg::r29), "ld1 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, Reg::r29), "ld1 {v31.8b, v0.8b, v1.8b, v2.8b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld1 {v26.16b, v27.16b, v28.16b, "
                                                                                                      "v29.16b}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld1 {v26.8b, v27.8b, v28.8b, "
                                                                                                      "v29.8b}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld1 {v26.8h, v27.8h, v28.8h, "
                                                                                                       "v29.8h}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld1 {v26.4h, v27.4h, v28.4h, "
                                                                                                       "v29.4h}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld1 {v26.4s, v27.4s, v28.4s, "
                                                                                                       "v29.4s}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld1 {v26.2s, v27.2s, v28.2s, "
                                                                                                       "v29.2s}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld1 {v26.2d, v27.2d, v28.2d, "
                                                                                                       "v29.2d}, [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld1 {v26.1d, v27.1d, v28.1d, "
                                                                                                       "v29.1d}, [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, 64), "ld1 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                             "[x30], #64");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, 32), "ld1 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30], "
                                                                                             "#32");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld1 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                                "[x30], #64");
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld1 {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                                "[x30], #32");

  TEST_SINGLE(ld1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld1 {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld1 {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(ld1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld1 {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld1 {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(ld1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld1 {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld1 {v26.1d, v27.1d, v28.1d, v29.1d}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, Reg::r30, Reg::r29), "st1 {v26.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, Reg::r30, Reg::r29), "st1 {v26.8b}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, Reg::r30, Reg::r29), "st1 {v26.8h}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, Reg::r30, Reg::r29), "st1 {v26.4h}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, Reg::r30, Reg::r29), "st1 {v26.4s}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, Reg::r30, Reg::r29), "st1 {v26.2s}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, Reg::r30, Reg::r29), "st1 {v26.2d}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, Reg::r30, Reg::r29), "st1 {v26.1d}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, Reg::r30, 16), "st1 {v26.16b}, [x30], #16");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, Reg::r30, 8), "st1 {v26.8b}, [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, Reg::r30, 16), "st1 {v26.8h}, [x30], #16");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, Reg::r30, 8), "st1 {v26.4h}, [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, Reg::r30, 16), "st1 {v26.4s}, [x30], #16");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, Reg::r30, 8), "st1 {v26.2s}, [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, Reg::r30, 16), "st1 {v26.2d}, [x30], #16");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, Reg::r30, 8), "st1 {v26.1d}, [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, Reg::r29), "st1 {v31.16b, v0.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, Reg::r29), "st1 {v31.8b, v0.8b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st1 {v26.16b, v27.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st1 {v26.8b, v27.8b}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st1 {v26.8h, v27.8h}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st1 {v26.4h, v27.4h}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st1 {v26.4s, v27.4s}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st1 {v26.2s, v27.2s}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st1 {v26.2d, v27.2d}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st1 {v26.1d, v27.1d}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, 32), "st1 {v31.16b, v0.16b}, [x30], #32");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, 16), "st1 {v31.8b, v0.8b}, [x30], #16");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st1 {v26.16b, v27.16b}, [x30], #32");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st1 {v26.8b, v27.8b}, [x30], #16");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st1 {v26.8h, v27.8h}, [x30], #32");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st1 {v26.4h, v27.4h}, [x30], #16");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st1 {v26.4s, v27.4s}, [x30], #32");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st1 {v26.2s, v27.2s}, [x30], #16");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st1 {v26.2d, v27.2d}, [x30], #32");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st1 {v26.1d, v27.1d}, [x30], #16");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, Reg::r29), "st1 {v31.16b, v0.16b, v1.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, Reg::r29), "st1 {v31.8b, v0.8b, v1.8b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st1 {v26.16b, v27.16b, v28.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st1 {v26.8b, v27.8b, v28.8b}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st1 {v26.8h, v27.8h, v28.8h}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st1 {v26.4h, v27.4h, v28.4h}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st1 {v26.4s, v27.4s, v28.4s}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st1 {v26.2s, v27.2s, v28.2s}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st1 {v26.2d, v27.2d, v28.2d}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st1 {v26.1d, v27.1d, v28.1d}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, 48), "st1 {v31.16b, v0.16b, v1.16b}, [x30], #48");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, 24), "st1 {v31.8b, v0.8b, v1.8b}, [x30], #24");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st1 {v26.16b, v27.16b, v28.16b}, [x30], #48");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st1 {v26.8b, v27.8b, v28.8b}, [x30], #24");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st1 {v26.8h, v27.8h, v28.8h}, [x30], #48");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st1 {v26.4h, v27.4h, v28.4h}, [x30], #24");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st1 {v26.4s, v27.4s, v28.4s}, [x30], #48");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st1 {v26.2s, v27.2s, v28.2s}, [x30], #24");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st1 {v26.2d, v27.2d, v28.2d}, [x30], #48");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st1 {v26.1d, v27.1d, v28.1d}, [x30], #24");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, Reg::r29), "st1 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, Reg::r29), "st1 {v31.8b, v0.8b, v1.8b, v2.8b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st1 {v26.16b, v27.16b, v28.16b, "
                                                                                                      "v29.16b}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st1 {v26.8b, v27.8b, v28.8b, "
                                                                                                      "v29.8b}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st1 {v26.8h, v27.8h, v28.8h, "
                                                                                                       "v29.8h}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st1 {v26.4h, v27.4h, v28.4h, "
                                                                                                       "v29.4h}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st1 {v26.4s, v27.4s, v28.4s, "
                                                                                                       "v29.4s}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st1 {v26.2s, v27.2s, v28.2s, "
                                                                                                       "v29.2s}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st1 {v26.2d, v27.2d, v28.2d, "
                                                                                                       "v29.2d}, [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st1 {v26.1d, v27.1d, v28.1d, "
                                                                                                       "v29.1d}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, 64), "st1 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                             "[x30], #64");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, 32), "st1 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30], "
                                                                                             "#32");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st1 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                                "[x30], #64");
  TEST_SINGLE(st1<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st1 {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                                "[x30], #32");

  TEST_SINGLE(st1<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st1 {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st1 {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(st1<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st1 {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st1 {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(st1<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st1 {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st1 {v26.1d, v27.1d, v28.1d, v29.1d}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, Reg::r29), "ld2 {v31.16b, v0.16b}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, Reg::r29), "ld2 {v31.8b, v0.8b}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2 {v26.16b, v27.16b}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2 {v26.8b, v27.8b}, [x30], x29");

  TEST_SINGLE(ld2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2 {v26.8h, v27.8h}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2 {v26.4h, v27.4h}, [x30], x29");

  TEST_SINGLE(ld2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2 {v26.4s, v27.4s}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2 {v26.2s, v27.2s}, [x30], x29");

  TEST_SINGLE(ld2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2 {v26.2d, v27.2d}, [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, 32), "ld2 {v31.16b, v0.16b}, [x30], #32");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, 16), "ld2 {v31.8b, v0.8b}, [x30], #16");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld2 {v26.16b, v27.16b}, [x30], #32");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld2 {v26.8b, v27.8b}, [x30], #16");

  TEST_SINGLE(ld2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld2 {v26.8h, v27.8h}, [x30], #32");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld2 {v26.4h, v27.4h}, [x30], #16");

  TEST_SINGLE(ld2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld2 {v26.4s, v27.4s}, [x30], #32");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld2 {v26.2s, v27.2s}, [x30], #16");

  TEST_SINGLE(ld2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "ld2 {v26.2d, v27.2d}, [x30], #32");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, Reg::r29), "st2 {v31.16b, v0.16b}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, Reg::r29), "st2 {v31.8b, v0.8b}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st2 {v26.16b, v27.16b}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st2 {v26.8b, v27.8b}, [x30], x29");

  TEST_SINGLE(st2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st2 {v26.8h, v27.8h}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st2 {v26.4h, v27.4h}, [x30], x29");

  TEST_SINGLE(st2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st2 {v26.4s, v27.4s}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "st2 {v26.2s, v27.2s}, [x30], x29");

  TEST_SINGLE(st2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "st2 {v26.2d, v27.2d}, [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, 32), "st2 {v31.16b, v0.16b}, [x30], #32");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, 16), "st2 {v31.8b, v0.8b}, [x30], #16");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st2 {v26.16b, v27.16b}, [x30], #32");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st2 {v26.8b, v27.8b}, [x30], #16");

  TEST_SINGLE(st2<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st2 {v26.8h, v27.8h}, [x30], #32");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st2 {v26.4h, v27.4h}, [x30], #16");

  TEST_SINGLE(st2<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st2 {v26.4s, v27.4s}, [x30], #32");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "st2 {v26.2s, v27.2s}, [x30], #16");

  TEST_SINGLE(st2<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, 32), "st2 {v26.2d, v27.2d}, [x30], #32");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, Reg::r29), "ld3 {v31.16b, v0.16b, v1.16b}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, Reg::r29), "ld3 {v31.8b, v0.8b, v1.8b}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3 {v26.16b, v27.16b, v28.16b}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3 {v26.8b, v27.8b, v28.8b}, [x30], x29");

  TEST_SINGLE(ld3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3 {v26.8h, v27.8h, v28.8h}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3 {v26.4h, v27.4h, v28.4h}, [x30], x29");

  TEST_SINGLE(ld3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3 {v26.4s, v27.4s, v28.4s}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3 {v26.2s, v27.2s, v28.2s}, [x30], x29");

  TEST_SINGLE(ld3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3 {v26.2d, v27.2d, v28.2d}, [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "unallocated "
                                                                                            "(NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, 48), "ld3 {v31.16b, v0.16b, v1.16b}, [x30], #48");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, 24), "ld3 {v31.8b, v0.8b, v1.8b}, [x30], #24");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld3 {v26.16b, v27.16b, v28.16b}, [x30], #48");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld3 {v26.8b, v27.8b, v28.8b}, [x30], #24");

  TEST_SINGLE(ld3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld3 {v26.8h, v27.8h, v28.8h}, [x30], #48");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld3 {v26.4h, v27.4h, v28.4h}, [x30], #24");

  TEST_SINGLE(ld3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld3 {v26.4s, v27.4s, v28.4s}, [x30], #48");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld3 {v26.2s, v27.2s, v28.2s}, [x30], #24");

  TEST_SINGLE(ld3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "ld3 {v26.2d, v27.2d, v28.2d}, [x30], #48");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, Reg::r29), "st3 {v31.16b, v0.16b, v1.16b}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, Reg::r29), "st3 {v31.8b, v0.8b, v1.8b}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st3 {v26.16b, v27.16b, v28.16b}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st3 {v26.8b, v27.8b, v28.8b}, [x30], x29");

  TEST_SINGLE(st3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st3 {v26.8h, v27.8h, v28.8h}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st3 {v26.4h, v27.4h, v28.4h}, [x30], x29");

  TEST_SINGLE(st3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st3 {v26.4s, v27.4s, v28.4s}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "st3 {v26.2s, v27.2s, v28.2s}, [x30], x29");

  TEST_SINGLE(st3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "st3 {v26.2d, v27.2d, v28.2d}, [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "unallocated "
                                                                                            "(NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, 48), "st3 {v31.16b, v0.16b, v1.16b}, [x30], #48");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, 24), "st3 {v31.8b, v0.8b, v1.8b}, [x30], #24");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st3 {v26.16b, v27.16b, v28.16b}, [x30], #48");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st3 {v26.8b, v27.8b, v28.8b}, [x30], #24");

  TEST_SINGLE(st3<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st3 {v26.8h, v27.8h, v28.8h}, [x30], #48");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st3 {v26.4h, v27.4h, v28.4h}, [x30], #24");

  TEST_SINGLE(st3<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st3 {v26.4s, v27.4s, v28.4s}, [x30], #48");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "st3 {v26.2s, v27.2s, v28.2s}, [x30], #24");

  TEST_SINGLE(st3<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 48), "st3 {v26.2d, v27.2d, v28.2d}, [x30], #48");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "unallocated (NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, Reg::r29), "ld4 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, Reg::r29), "ld4 {v31.8b, v0.8b, v1.8b, v2.8b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4 {v26.16b, v27.16b, v28.16b, "
                                                                                                      "v29.16b}, [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4 {v26.8b, v27.8b, v28.8b, "
                                                                                                      "v29.8b}, [x30], x29");

  TEST_SINGLE(ld4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4 {v26.8h, v27.8h, v28.8h, "
                                                                                                       "v29.8h}, [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4 {v26.4h, v27.4h, v28.4h, "
                                                                                                       "v29.4h}, [x30], x29");

  TEST_SINGLE(ld4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4 {v26.4s, v27.4s, v28.4s, "
                                                                                                       "v29.4s}, [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4 {v26.2s, v27.2s, v28.2s, "
                                                                                                       "v29.2s}, [x30], x29");

  TEST_SINGLE(ld4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4 {v26.2d, v27.2d, v28.2d, "
                                                                                                       "v29.2d}, [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "unallocated "
                                                                                                       "(NEONLoadStoreMultiStructPostIndex"
                                                                                                       ")");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, 64), "ld4 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                             "[x30], #64");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, 32), "ld4 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30], "
                                                                                             "#32");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld4 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                                "[x30], #64");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld4 {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                                "[x30], #32");

  TEST_SINGLE(ld4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld4 {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld4 {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(ld4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld4 {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld4 {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(ld4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "ld4 {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "unallocated "
                                                                                                 "(NEONLoadStoreMultiStructPostIndex)");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, Reg::r29), "st4 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, Reg::r29), "st4 {v31.8b, v0.8b, v1.8b, v2.8b}, "
                                                                                                   "[x30], x29");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st4 {v26.16b, v27.16b, v28.16b, "
                                                                                                      "v29.16b}, [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st4 {v26.8b, v27.8b, v28.8b, "
                                                                                                      "v29.8b}, [x30], x29");

  TEST_SINGLE(st4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st4 {v26.8h, v27.8h, v28.8h, "
                                                                                                       "v29.8h}, [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st4 {v26.4h, v27.4h, v28.4h, "
                                                                                                       "v29.4h}, [x30], x29");

  TEST_SINGLE(st4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st4 {v26.4s, v27.4s, v28.4s, "
                                                                                                       "v29.4s}, [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "st4 {v26.2s, v27.2s, v28.2s, "
                                                                                                       "v29.2s}, [x30], x29");

  TEST_SINGLE(st4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "st4 {v26.2d, v27.2d, v28.2d, "
                                                                                                       "v29.2d}, [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "unallocated "
                                                                                                       "(NEONLoadStoreMultiStructPostIndex"
                                                                                                       ")");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, 64), "st4 {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                             "[x30], #64");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, 32), "st4 {v31.8b, v0.8b, v1.8b, v2.8b}, [x30], "
                                                                                             "#32");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st4 {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                                "[x30], #64");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st4 {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                                "[x30], #32");

  TEST_SINGLE(st4<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st4 {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st4 {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(st4<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st4 {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "st4 {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                                 "[x30], #32");

  TEST_SINGLE(st4<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 64), "st4 {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                                 "[x30], #64");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "unallocated "
                                                                                                 "(NEONLoadStoreMultiStructPostIndex)");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: ASIMD loadstore single") {
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30), "ld1 {v26.b}[0], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30), "ld1 {v26.h}[0], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30), "ld1 {v26.s}[0], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30), "ld1 {v26.d}[0], [x30]");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30), "ld1 {v26.b}[15], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30), "ld1 {v26.h}[7], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30), "ld1 {v26.s}[3], [x30]");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30), "ld1 {v26.d}[1], [x30]");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(DReg::d26, Reg::r30), "ld1r {v26.8b}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(DReg::d26, Reg::r30), "ld1r {v26.4h}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(DReg::d26, Reg::r30), "ld1r {v26.2s}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(DReg::d26, Reg::r30), "ld1r {v26.1d}, [x30]");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(QReg::q26, Reg::r30), "ld1r {v26.16b}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(QReg::q26, Reg::r30), "ld1r {v26.8h}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(QReg::q26, Reg::r30), "ld1r {v26.4s}, [x30]");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(QReg::q26, Reg::r30), "ld1r {v26.2d}, [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30), "st1 {v26.b}[0], [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30), "st1 {v26.h}[0], [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30), "st1 {v26.s}[0], [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30), "st1 {v26.d}[0], [x30]");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30), "st1 {v26.b}[15], [x30]");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30), "st1 {v26.h}[7], [x30]");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30), "st1 {v26.s}[3], [x30]");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30), "st1 {v26.d}[1], [x30]");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30), "ld2 {v31.b, v0.b}[0], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "ld2 {v26.b, v27.b}[0], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "ld2 {v26.h, v27.h}[0], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "ld2 {v26.s, v27.s}[0], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "ld2 {v26.d, v27.d}[0], [x30]");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30), "ld2 {v26.b, v27.b}[15], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30), "ld2 {v26.h, v27.h}[7], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30), "ld2 {v26.s, v27.s}[3], [x30]");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30), "ld2 {v26.d, v27.d}[1], [x30]");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30), "ld2r {v31.8b, v0.8b}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2r {v26.8b, v27.8b}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2r {v26.4h, v27.4h}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2r {v26.2s, v27.2s}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30), "ld2r {v26.1d, v27.1d}, [x30]");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30), "ld2r {v31.16b, v0.16b}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2r {v26.16b, v27.16b}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2r {v26.8h, v27.8h}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2r {v26.4s, v27.4s}, [x30]");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30), "ld2r {v26.2d, v27.2d}, [x30]");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30), "st2 {v31.b, v0.b}[0], [x30]");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "st2 {v26.b, v27.b}[0], [x30]");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "st2 {v26.h, v27.h}[0], [x30]");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "st2 {v26.s, v27.s}[0], [x30]");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30), "st2 {v26.d, v27.d}[0], [x30]");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30), "st2 {v26.b, v27.b}[15], [x30]");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30), "st2 {v26.h, v27.h}[7], [x30]");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30), "st2 {v26.s, v27.s}[3], [x30]");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30), "st2 {v26.d, v27.d}[1], [x30]");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30), "ld3 {v31.b, v0.b, v1.b}[0], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "ld3 {v26.b, v27.b, v28.b}[0], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "ld3 {v26.h, v27.h, v28.h}[0], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "ld3 {v26.s, v27.s, v28.s}[0], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "ld3 {v26.d, v27.d, v28.d}[0], [x30]");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30), "ld3 {v26.b, v27.b, v28.b}[15], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30), "ld3 {v26.h, v27.h, v28.h}[7], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30), "ld3 {v26.s, v27.s, v28.s}[3], [x30]");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30), "ld3 {v26.d, v27.d, v28.d}[1], [x30]");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30), "ld3r {v31.8b, v0.8b, v1.8b}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3r {v26.8b, v27.8b, v28.8b}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3r {v26.4h, v27.4h, v28.4h}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3r {v26.2s, v27.2s, v28.2s}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30), "ld3r {v26.1d, v27.1d, v28.1d}, [x30]");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30), "ld3r {v31.16b, v0.16b, v1.16b}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3r {v26.16b, v27.16b, v28.16b}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3r {v26.8h, v27.8h, v28.8h}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3r {v26.4s, v27.4s, v28.4s}, [x30]");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30), "ld3r {v26.2d, v27.2d, v28.2d}, [x30]");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30), "st3 {v31.b, v0.b, v1.b}[0], [x30]");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "st3 {v26.b, v27.b, v28.b}[0], [x30]");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "st3 {v26.h, v27.h, v28.h}[0], [x30]");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "st3 {v26.s, v27.s, v28.s}[0], [x30]");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30), "st3 {v26.d, v27.d, v28.d}[0], [x30]");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30), "st3 {v26.b, v27.b, v28.b}[15], [x30]");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30), "st3 {v26.h, v27.h, v28.h}[7], [x30]");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30), "st3 {v26.s, v27.s, v28.s}[3], [x30]");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30), "st3 {v26.d, v27.d, v28.d}[1], [x30]");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30), "ld4 {v31.b, v0.b, v1.b, v2.b}[0], [x30]");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "ld4 {v26.b, v27.b, v28.b, v29.b}[0], "
                                                                                               "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "ld4 {v26.h, v27.h, v28.h, v29.h}[0], "
                                                                                                "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "ld4 {v26.s, v27.s, v28.s, v29.s}[0], "
                                                                                                "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "ld4 {v26.d, v27.d, v28.d, v29.d}[0], "
                                                                                                "[x30]");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30), "ld4 {v26.b, v27.b, v28.b, v29.b}[15], "
                                                                                                "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30), "ld4 {v26.h, v27.h, v28.h, v29.h}[7], "
                                                                                                "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30), "ld4 {v26.s, v27.s, v28.s, v29.s}[3], "
                                                                                                "[x30]");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30), "ld4 {v26.d, v27.d, v28.d, v29.d}[1], "
                                                                                                "[x30]");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30), "ld4r {v31.8b, v0.8b, v1.8b, v2.8b}, [x30]");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4r {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                             "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4r {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                              "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4r {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                              "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30), "ld4r {v26.1d, v27.1d, v28.1d, v29.1d}, "
                                                                                              "[x30]");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30), "ld4r {v31.16b, v0.16b, v1.16b, v2.16b}, [x30]");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4r {v26.16b, v27.16b, v28.16b, v29.16b}, "
                                                                                             "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4r {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                              "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4r {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                              "[x30]");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30), "ld4r {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                              "[x30]");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30), "st4 {v31.b, v0.b, v1.b, v2.b}[0], [x30]");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "st4 {v26.b, v27.b, v28.b, v29.b}[0], "
                                                                                               "[x30]");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "st4 {v26.h, v27.h, v28.h, v29.h}[0], "
                                                                                                "[x30]");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "st4 {v26.s, v27.s, v28.s, v29.s}[0], "
                                                                                                "[x30]");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30), "st4 {v26.d, v27.d, v28.d, v29.d}[0], "
                                                                                                "[x30]");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30), "st4 {v26.b, v27.b, v28.b, v29.b}[15], "
                                                                                                "[x30]");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30), "st4 {v26.h, v27.h, v28.h, v29.h}[7], "
                                                                                                "[x30]");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30), "st4 {v26.s, v27.s, v28.s, v29.s}[3], "
                                                                                                "[x30]");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30), "st4 {v26.d, v27.d, v28.d, v29.d}[1], "
                                                                                                "[x30]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Advanced SIMD load/store single structure (post-indexed)") {
  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30, 1), "ld1 {v26.b}[0], [x30], #1");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30, 2), "ld1 {v26.h}[0], [x30], #2");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30, 4), "ld1 {v26.s}[0], [x30], #4");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30, 8), "ld1 {v26.d}[0], [x30], #8");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30, 1), "ld1 {v26.b}[15], [x30], #1");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30, 2), "ld1 {v26.h}[7], [x30], #2");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30, 4), "ld1 {v26.s}[3], [x30], #4");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30, 8), "ld1 {v26.d}[1], [x30], #8");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(DReg::d26, Reg::r30, 1), "ld1r {v26.8b}, [x30], #1");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(DReg::d26, Reg::r30, 2), "ld1r {v26.4h}, [x30], #2");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(DReg::d26, Reg::r30, 4), "ld1r {v26.2s}, [x30], #4");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(DReg::d26, Reg::r30, 8), "ld1r {v26.1d}, [x30], #8");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(QReg::q26, Reg::r30, 1), "ld1r {v26.16b}, [x30], #1");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(QReg::q26, Reg::r30, 2), "ld1r {v26.8h}, [x30], #2");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(QReg::q26, Reg::r30, 4), "ld1r {v26.4s}, [x30], #4");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(QReg::q26, Reg::r30, 8), "ld1r {v26.2d}, [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30, 1), "st1 {v26.b}[0], [x30], #1");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30, 2), "st1 {v26.h}[0], [x30], #2");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30, 4), "st1 {v26.s}[0], [x30], #4");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30, 8), "st1 {v26.d}[0], [x30], #8");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30, 1), "st1 {v26.b}[15], [x30], #1");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30, 2), "st1 {v26.h}[7], [x30], #2");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30, 4), "st1 {v26.s}[3], [x30], #4");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30, 8), "st1 {v26.d}[1], [x30], #8");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30, 2), "ld2 {v31.b, v0.b}[0], [x30], #2");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 2), "ld2 {v26.b, v27.b}[0], [x30], #2");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 4), "ld2 {v26.h, v27.h}[0], [x30], #4");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 8), "ld2 {v26.s, v27.s}[0], [x30], #8");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 16), "ld2 {v26.d, v27.d}[0], [x30], #16");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30, 2), "ld2 {v26.b, v27.b}[15], [x30], #2");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30, 4), "ld2 {v26.h, v27.h}[7], [x30], #4");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30, 8), "ld2 {v26.s, v27.s}[3], [x30], #8");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30, 16), "ld2 {v26.d, v27.d}[1], [x30], #16");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, 2), "ld2r {v31.8b, v0.8b}, [x30], #2");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, 2), "ld2r {v26.8b, v27.8b}, [x30], #2");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, 4), "ld2r {v26.4h, v27.4h}, [x30], #4");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, 8), "ld2r {v26.2s, v27.2s}, [x30], #8");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, 16), "ld2r {v26.1d, v27.1d}, [x30], #16");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, 2), "ld2r {v31.16b, v0.16b}, [x30], #2");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, 2), "ld2r {v26.16b, v27.16b}, [x30], #2");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, 4), "ld2r {v26.8h, v27.8h}, [x30], #4");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, 8), "ld2r {v26.4s, v27.4s}, [x30], #8");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, 16), "ld2r {v26.2d, v27.2d}, [x30], #16");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30, 2), "st2 {v31.b, v0.b}[0], [x30], #2");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 2), "st2 {v26.b, v27.b}[0], [x30], #2");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 4), "st2 {v26.h, v27.h}[0], [x30], #4");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 8), "st2 {v26.s, v27.s}[0], [x30], #8");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30, 16), "st2 {v26.d, v27.d}[0], [x30], #16");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30, 2), "st2 {v26.b, v27.b}[15], [x30], #2");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30, 4), "st2 {v26.h, v27.h}[7], [x30], #4");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30, 8), "st2 {v26.s, v27.s}[3], [x30], #8");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30, 16), "st2 {v26.d, v27.d}[1], [x30], #16");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30, 3), "ld3 {v31.b, v0.b, v1.b}[0], [x30], #3");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 3), "ld3 {v26.b, v27.b, v28.b}[0], [x30], #3");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 6), "ld3 {v26.h, v27.h, v28.h}[0], [x30], #6");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 12), "ld3 {v26.s, v27.s, v28.s}[0], [x30], #12");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 24), "ld3 {v26.d, v27.d, v28.d}[0], [x30], #24");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30, 3), "ld3 {v26.b, v27.b, v28.b}[15], [x30], #3");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30, 6), "ld3 {v26.h, v27.h, v28.h}[7], [x30], #6");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30, 12), "ld3 {v26.s, v27.s, v28.s}[3], [x30], #12");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30, 24), "ld3 {v26.d, v27.d, v28.d}[1], [x30], #24");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, 3), "ld3r {v31.8b, v0.8b, v1.8b}, [x30], #3");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 3), "ld3r {v26.8b, v27.8b, v28.8b}, [x30], #3");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 6), "ld3r {v26.4h, v27.4h, v28.4h}, [x30], #6");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 12), "ld3r {v26.2s, v27.2s, v28.2s}, [x30], #12");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, 24), "ld3r {v26.1d, v27.1d, v28.1d}, [x30], #24");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, 3), "ld3r {v31.16b, v0.16b, v1.16b}, [x30], #3");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 3), "ld3r {v26.16b, v27.16b, v28.16b}, [x30], #3");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 6), "ld3r {v26.8h, v27.8h, v28.8h}, [x30], #6");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 12), "ld3r {v26.4s, v27.4s, v28.4s}, [x30], #12");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, 24), "ld3r {v26.2d, v27.2d, v28.2d}, [x30], #24");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30, 3), "st3 {v31.b, v0.b, v1.b}[0], [x30], #3");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 3), "st3 {v26.b, v27.b, v28.b}[0], [x30], #3");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 6), "st3 {v26.h, v27.h, v28.h}[0], [x30], #6");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 12), "st3 {v26.s, v27.s, v28.s}[0], [x30], #12");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, 24), "st3 {v26.d, v27.d, v28.d}[0], [x30], #24");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30, 3), "st3 {v26.b, v27.b, v28.b}[15], [x30], #3");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30, 6), "st3 {v26.h, v27.h, v28.h}[7], [x30], #6");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30, 12), "st3 {v26.s, v27.s, v28.s}[3], [x30], #12");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30, 24), "st3 {v26.d, v27.d, v28.d}[1], [x30], #24");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30, 4), "ld4 {v31.b, v0.b, v1.b, v2.b}[0], [x30], "
                                                                                               "#4");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 4), "ld4 {v26.b, v27.b, v28.b, v29.b}[0], "
                                                                                                  "[x30], #4");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 8), "ld4 {v26.h, v27.h, v28.h, v29.h}[0], "
                                                                                                   "[x30], #8");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 16), "ld4 {v26.s, v27.s, v28.s, v29.s}[0], "
                                                                                                    "[x30], #16");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 32), "ld4 {v26.d, v27.d, v28.d, v29.d}[0], "
                                                                                                    "[x30], #32");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30, 4), "ld4 {v26.b, v27.b, v28.b, v29.b}[15], "
                                                                                                   "[x30], #4");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30, 8), "ld4 {v26.h, v27.h, v28.h, v29.h}[7], "
                                                                                                   "[x30], #8");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30, 16), "ld4 {v26.s, v27.s, v28.s, v29.s}[3], "
                                                                                                    "[x30], #16");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30, 32), "ld4 {v26.d, v27.d, v28.d, v29.d}[1], "
                                                                                                    "[x30], #32");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, 4), "ld4r {v31.8b, v0.8b, v1.8b, v2.8b}, [x30], "
                                                                                             "#4");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 4), "ld4r {v26.8b, v27.8b, v28.8b, v29.8b}, "
                                                                                                "[x30], #4");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 8), "ld4r {v26.4h, v27.4h, v28.4h, v29.4h}, "
                                                                                                 "[x30], #8");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 16), "ld4r {v26.2s, v27.2s, v28.2s, v29.2s}, "
                                                                                                  "[x30], #16");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, 32), "ld4r {v26.1d, v27.1d, v28.1d, v29.1d}, "
                                                                                                  "[x30], #32");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, 4), "ld4r {v31.16b, v0.16b, v1.16b, v2.16b}, "
                                                                                             "[x30], #4");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 4), "ld4r {v26.16b, v27.16b, v28.16b, "
                                                                                                "v29.16b}, [x30], #4");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 8), "ld4r {v26.8h, v27.8h, v28.8h, v29.8h}, "
                                                                                                 "[x30], #8");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 16), "ld4r {v26.4s, v27.4s, v28.4s, v29.4s}, "
                                                                                                  "[x30], #16");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, 32), "ld4r {v26.2d, v27.2d, v28.2d, v29.2d}, "
                                                                                                  "[x30], #32");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30, 4), "st4 {v31.b, v0.b, v1.b, v2.b}[0], [x30], "
                                                                                               "#4");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 4), "st4 {v26.b, v27.b, v28.b, v29.b}[0], "
                                                                                                  "[x30], #4");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 8), "st4 {v26.h, v27.h, v28.h, v29.h}[0], "
                                                                                                   "[x30], #8");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 16), "st4 {v26.s, v27.s, v28.s, v29.s}[0], "
                                                                                                    "[x30], #16");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, 32), "st4 {v26.d, v27.d, v28.d, v29.d}[0], "
                                                                                                    "[x30], #32");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30, 4), "st4 {v26.b, v27.b, v28.b, v29.b}[15], "
                                                                                                   "[x30], #4");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30, 8), "st4 {v26.h, v27.h, v28.h, v29.h}[7], "
                                                                                                   "[x30], #8");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30, 16), "st4 {v26.s, v27.s, v28.s, v29.s}[3], "
                                                                                                    "[x30], #16");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30, 32), "st4 {v26.d, v27.d, v28.d, v29.d}[1], "
                                                                                                    "[x30], #32");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "ld1 {v26.b}[0], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "ld1 {v26.h}[0], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "ld1 {v26.s}[0], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "ld1 {v26.d}[0], [x30], x29");

  TEST_SINGLE(ld1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30, Reg::r29), "ld1 {v26.b}[15], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30, Reg::r29), "ld1 {v26.h}[7], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30, Reg::r29), "ld1 {v26.s}[3], [x30], x29");
  TEST_SINGLE(ld1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30, Reg::r29), "ld1 {v26.d}[1], [x30], x29");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1r {v26.8b}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1r {v26.4h}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1r {v26.2s}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(DReg::d26, Reg::r30, Reg::r29), "ld1r {v26.1d}, [x30], x29");

  TEST_SINGLE(ld1r<SubRegSize::i8Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1r {v26.16b}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i16Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1r {v26.8h}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i32Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1r {v26.4s}, [x30], x29");
  TEST_SINGLE(ld1r<SubRegSize::i64Bit>(QReg::q26, Reg::r30, Reg::r29), "ld1r {v26.2d}, [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "st1 {v26.b}[0], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "st1 {v26.h}[0], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "st1 {v26.s}[0], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 0, Reg::r30, Reg::r29), "st1 {v26.d}[0], [x30], x29");

  TEST_SINGLE(st1<SubRegSize::i8Bit>(VReg::v26, 15, Reg::r30, Reg::r29), "st1 {v26.b}[15], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i16Bit>(VReg::v26, 7, Reg::r30, Reg::r29), "st1 {v26.h}[7], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i32Bit>(VReg::v26, 3, Reg::r30, Reg::r29), "st1 {v26.s}[3], [x30], x29");
  TEST_SINGLE(st1<SubRegSize::i64Bit>(VReg::v26, 1, Reg::r30, Reg::r29), "st1 {v26.d}[1], [x30], x29");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30, Reg::r29), "ld2 {v31.b, v0.b}[0], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "ld2 {v26.b, v27.b}[0], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "ld2 {v26.h, v27.h}[0], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "ld2 {v26.s, v27.s}[0], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "ld2 {v26.d, v27.d}[0], [x30], x29");

  TEST_SINGLE(ld2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30, Reg::r29), "ld2 {v26.b, v27.b}[15], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30, Reg::r29), "ld2 {v26.h, v27.h}[7], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30, Reg::r29), "ld2 {v26.s, v27.s}[3], [x30], x29");
  TEST_SINGLE(ld2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30, Reg::r29), "ld2 {v26.d, v27.d}[1], [x30], x29");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, Reg::r30, Reg::r29), "ld2r {v31.8b, v0.8b}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2r {v26.8b, v27.8b}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2r {v26.4h, v27.4h}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2r {v26.2s, v27.2s}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, Reg::r30, Reg::r29), "ld2r {v26.1d, v27.1d}, [x30], x29");

  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, Reg::r30, Reg::r29), "ld2r {v31.16b, v0.16b}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2r {v26.16b, v27.16b}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2r {v26.8h, v27.8h}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2r {v26.4s, v27.4s}, [x30], x29");
  TEST_SINGLE(ld2r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, Reg::r30, Reg::r29), "ld2r {v26.2d, v27.2d}, [x30], x29");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v31, VReg::v0, 0, Reg::r30, Reg::r29), "st2 {v31.b, v0.b}[0], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "st2 {v26.b, v27.b}[0], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "st2 {v26.h, v27.h}[0], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "st2 {v26.s, v27.s}[0], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 0, Reg::r30, Reg::r29), "st2 {v26.d, v27.d}[0], [x30], x29");

  TEST_SINGLE(st2<SubRegSize::i8Bit>(VReg::v26, VReg::v27, 15, Reg::r30, Reg::r29), "st2 {v26.b, v27.b}[15], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i16Bit>(VReg::v26, VReg::v27, 7, Reg::r30, Reg::r29), "st2 {v26.h, v27.h}[7], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i32Bit>(VReg::v26, VReg::v27, 3, Reg::r30, Reg::r29), "st2 {v26.s, v27.s}[3], [x30], x29");
  TEST_SINGLE(st2<SubRegSize::i64Bit>(VReg::v26, VReg::v27, 1, Reg::r30, Reg::r29), "st2 {v26.d, v27.d}[1], [x30], x29");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30, Reg::r29), "ld3 {v31.b, v0.b, v1.b}[0], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "ld3 {v26.b, v27.b, v28.b}[0], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "ld3 {v26.h, v27.h, v28.h}[0], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "ld3 {v26.s, v27.s, v28.s}[0], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "ld3 {v26.d, v27.d, v28.d}[0], [x30], x29");

  TEST_SINGLE(ld3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30, Reg::r29), "ld3 {v26.b, v27.b, v28.b}[15], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30, Reg::r29), "ld3 {v26.h, v27.h, v28.h}[7], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30, Reg::r29), "ld3 {v26.s, v27.s, v28.s}[3], [x30], x29");
  TEST_SINGLE(ld3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30, Reg::r29), "ld3 {v26.d, v27.d, v28.d}[1], [x30], x29");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, Reg::r30, Reg::r29), "ld3r {v31.8b, v0.8b, v1.8b}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3r {v26.8b, v27.8b, v28.8b}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3r {v26.4h, v27.4h, v28.4h}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3r {v26.2s, v27.2s, v28.2s}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, Reg::r30, Reg::r29), "ld3r {v26.1d, v27.1d, v28.1d}, [x30], x29");

  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, Reg::r30, Reg::r29), "ld3r {v31.16b, v0.16b, v1.16b}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3r {v26.16b, v27.16b, v28.16b}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3r {v26.8h, v27.8h, v28.8h}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3r {v26.4s, v27.4s, v28.4s}, [x30], x29");
  TEST_SINGLE(ld3r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, Reg::r30, Reg::r29), "ld3r {v26.2d, v27.2d, v28.2d}, [x30], x29");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, 0, Reg::r30, Reg::r29), "st3 {v31.b, v0.b, v1.b}[0], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "st3 {v26.b, v27.b, v28.b}[0], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "st3 {v26.h, v27.h, v28.h}[0], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "st3 {v26.s, v27.s, v28.s}[0], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 0, Reg::r30, Reg::r29), "st3 {v26.d, v27.d, v28.d}[0], [x30], x29");

  TEST_SINGLE(st3<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, 15, Reg::r30, Reg::r29), "st3 {v26.b, v27.b, v28.b}[15], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, 7, Reg::r30, Reg::r29), "st3 {v26.h, v27.h, v28.h}[7], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, 3, Reg::r30, Reg::r29), "st3 {v26.s, v27.s, v28.s}[3], [x30], x29");
  TEST_SINGLE(st3<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, 1, Reg::r30, Reg::r29), "st3 {v26.d, v27.d, v28.d}[1], [x30], x29");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30, Reg::r29), "ld4 {v31.b, v0.b, v1.b, v2.b}[0], "
                                                                                                      "[x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "ld4 {v26.b, v27.b, v28.b, "
                                                                                                         "v29.b}[0], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "ld4 {v26.h, v27.h, v28.h, "
                                                                                                          "v29.h}[0], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "ld4 {v26.s, v27.s, v28.s, "
                                                                                                          "v29.s}[0], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "ld4 {v26.d, v27.d, v28.d, "
                                                                                                          "v29.d}[0], [x30], x29");

  TEST_SINGLE(ld4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30, Reg::r29), "ld4 {v26.b, v27.b, v28.b, "
                                                                                                          "v29.b}[15], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30, Reg::r29), "ld4 {v26.h, v27.h, v28.h, "
                                                                                                          "v29.h}[7], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30, Reg::r29), "ld4 {v26.s, v27.s, v28.s, "
                                                                                                          "v29.s}[3], [x30], x29");
  TEST_SINGLE(ld4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30, Reg::r29), "ld4 {v26.d, v27.d, v28.d, "
                                                                                                          "v29.d}[1], [x30], x29");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d31, DReg::d0, DReg::d1, DReg::d2, Reg::r30, Reg::r29), "ld4r {v31.8b, v0.8b, v1.8b, v2.8b}, "
                                                                                                    "[x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4r {v26.8b, v27.8b, v28.8b, "
                                                                                                       "v29.8b}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4r {v26.4h, v27.4h, v28.4h, "
                                                                                                        "v29.4h}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4r {v26.2s, v27.2s, v28.2s, "
                                                                                                        "v29.2s}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(DReg::d26, DReg::d27, DReg::d28, DReg::d29, Reg::r30, Reg::r29), "ld4r {v26.1d, v27.1d, v28.1d, "
                                                                                                        "v29.1d}, [x30], x29");

  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q31, QReg::q0, QReg::q1, QReg::q2, Reg::r30, Reg::r29), "ld4r {v31.16b, v0.16b, v1.16b, "
                                                                                                    "v2.16b}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i8Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4r {v26.16b, v27.16b, v28.16b, "
                                                                                                       "v29.16b}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i16Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4r {v26.8h, v27.8h, v28.8h, "
                                                                                                        "v29.8h}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i32Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4r {v26.4s, v27.4s, v28.4s, "
                                                                                                        "v29.4s}, [x30], x29");
  TEST_SINGLE(ld4r<SubRegSize::i64Bit>(QReg::q26, QReg::q27, QReg::q28, QReg::q29, Reg::r30, Reg::r29), "ld4r {v26.2d, v27.2d, v28.2d, "
                                                                                                        "v29.2d}, [x30], x29");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v31, VReg::v0, VReg::v1, VReg::v2, 0, Reg::r30, Reg::r29), "st4 {v31.b, v0.b, v1.b, v2.b}[0], "
                                                                                                      "[x30], x29");
  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "st4 {v26.b, v27.b, v28.b, "
                                                                                                         "v29.b}[0], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "st4 {v26.h, v27.h, v28.h, "
                                                                                                          "v29.h}[0], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "st4 {v26.s, v27.s, v28.s, "
                                                                                                          "v29.s}[0], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 0, Reg::r30, Reg::r29), "st4 {v26.d, v27.d, v28.d, "
                                                                                                          "v29.d}[0], [x30], x29");

  TEST_SINGLE(st4<SubRegSize::i8Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 15, Reg::r30, Reg::r29), "st4 {v26.b, v27.b, v28.b, "
                                                                                                          "v29.b}[15], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i16Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 7, Reg::r30, Reg::r29), "st4 {v26.h, v27.h, v28.h, "
                                                                                                          "v29.h}[7], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i32Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 3, Reg::r30, Reg::r29), "st4 {v26.s, v27.s, v28.s, "
                                                                                                          "v29.s}[3], [x30], x29");
  TEST_SINGLE(st4<SubRegSize::i64Bit>(VReg::v26, VReg::v27, VReg::v28, VReg::v29, 1, Reg::r30, Reg::r29), "st4 {v26.d, v27.d, v28.d, "
                                                                                                          "v29.d}[1], [x30], x29");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore exclusive pair") {
  TEST_SINGLE(stxp(Size::i32Bit, Reg::r28, Reg::r29, Reg::r30, Reg::r28), "stxp w28, w29, w30, [x28]");
  TEST_SINGLE(stxp(Size::i64Bit, Reg::r28, Reg::r29, Reg::r30, Reg::r28), "stxp w28, x29, x30, [x28]");

  TEST_SINGLE(stlxp(Size::i32Bit, Reg::r28, Reg::r29, Reg::r30, Reg::r28), "stlxp w28, w29, w30, [x28]");
  TEST_SINGLE(stlxp(Size::i64Bit, Reg::r28, Reg::r29, Reg::r30, Reg::r28), "stlxp w28, x29, x30, [x28]");

  TEST_SINGLE(ldxp(Size::i32Bit, Reg::r29, Reg::r30, Reg::r28), "ldxp w29, w30, [x28]");
  TEST_SINGLE(ldxp(Size::i64Bit, Reg::r29, Reg::r30, Reg::r28), "ldxp x29, x30, [x28]");

  TEST_SINGLE(ldaxp(Size::i32Bit, Reg::r29, Reg::r30, Reg::r28), "ldaxp w29, w30, [x28]");
  TEST_SINGLE(ldaxp(Size::i64Bit, Reg::r29, Reg::r30, Reg::r28), "ldaxp x29, x30, [x28]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore exclusive register") {
  TEST_SINGLE(stxrb(Reg::r30, Reg::r29, Reg::r28), "stxrb w30, w29, [x28]");
  TEST_SINGLE(stlxrb(Reg::r30, Reg::r29, Reg::r28), "stlxrb w30, w29, [x28]");

  TEST_SINGLE(ldxrb(Reg::r30, Reg::r29), "ldxrb w30, [x29]");
  TEST_SINGLE(ldaxrb(Reg::r30, Reg::r29), "ldaxrb w30, [x29]");

  TEST_SINGLE(stxrh(Reg::r30, Reg::r29, Reg::r28), "stxrh w30, w29, [x28]");
  TEST_SINGLE(stlxrh(Reg::r30, Reg::r29, Reg::r28), "stlxrh w30, w29, [x28]");

  TEST_SINGLE(ldxrh(Reg::r30, Reg::r29), "ldxrh w30, [x29]");
  TEST_SINGLE(ldaxrh(Reg::r30, Reg::r29), "ldaxrh w30, [x29]");

  TEST_SINGLE(stxr(WReg::w30, WReg::w29, Reg::r28), "stxr w30, w29, [x28]");
  TEST_SINGLE(stlxr(WReg::w30, WReg::w29, Reg::r28), "stlxr w30, w29, [x28]");

  TEST_SINGLE(ldxr(WReg::w30, Reg::r29), "ldxr w30, [x29]");
  TEST_SINGLE(ldaxr(WReg::w30, Reg::r29), "ldaxr w30, [x29]");

  TEST_SINGLE(stxr(XReg::x30, XReg::x29, Reg::r28), "stxr w30, x29, [x28]");
  TEST_SINGLE(stlxr(WReg::w30, XReg::x29, Reg::r28), "stlxr w30, x29, [x28]");

  TEST_SINGLE(ldxr(XReg::x30, Reg::r29), "ldxr x30, [x29]");
  TEST_SINGLE(ldaxr(XReg::x30, Reg::r29), "ldaxr x30, [x29]");

  TEST_SINGLE(stxr(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "stxrb w30, w29, [x28]");
  TEST_SINGLE(stlxr(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "stlxrb w30, w29, [x28]");
  TEST_SINGLE(stxr(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "stxrh w30, w29, [x28]");
  TEST_SINGLE(stlxr(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "stlxrh w30, w29, [x28]");
  TEST_SINGLE(stxr(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "stxr w30, w29, [x28]");
  TEST_SINGLE(stlxr(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "stlxr w30, w29, [x28]");
  TEST_SINGLE(stxr(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "stxr w30, x29, [x28]");
  TEST_SINGLE(stlxr(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "stlxr w30, x29, [x28]");

  TEST_SINGLE(ldxr(SubRegSize::i8Bit, Reg::r30, Reg::r29), "ldxrb w30, [x29]");
  TEST_SINGLE(ldaxr(SubRegSize::i8Bit, Reg::r30, Reg::r29), "ldaxrb w30, [x29]");
  TEST_SINGLE(ldxr(SubRegSize::i16Bit, Reg::r30, Reg::r29), "ldxrh w30, [x29]");
  TEST_SINGLE(ldaxr(SubRegSize::i16Bit, Reg::r30, Reg::r29), "ldaxrh w30, [x29]");
  TEST_SINGLE(ldxr(SubRegSize::i32Bit, Reg::r30, Reg::r29), "ldxr w30, [x29]");
  TEST_SINGLE(ldaxr(SubRegSize::i32Bit, Reg::r30, Reg::r29), "ldaxr w30, [x29]");
  TEST_SINGLE(ldxr(SubRegSize::i64Bit, Reg::r30, Reg::r29), "ldxr x30, [x29]");
  TEST_SINGLE(ldaxr(SubRegSize::i64Bit, Reg::r30, Reg::r29), "ldaxr x30, [x29]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Load/store ordered") {
  TEST_SINGLE(stllrb(Reg::r30, Reg::r29), "stllrb w30, [x29]");
  TEST_SINGLE(stlrb(Reg::r30, Reg::r29), "stlrb w30, [x29]");
  TEST_SINGLE(ldlarb(Reg::r30, Reg::r29), "ldlarb w30, [x29]");
  TEST_SINGLE(ldarb(Reg::r30, Reg::r29), "ldarb w30, [x29]");

  TEST_SINGLE(stllrh(Reg::r30, Reg::r29), "stllrh w30, [x29]");
  TEST_SINGLE(stlrh(Reg::r30, Reg::r29), "stlrh w30, [x29]");
  TEST_SINGLE(ldlarh(Reg::r30, Reg::r29), "ldlarh w30, [x29]");
  TEST_SINGLE(ldarh(Reg::r30, Reg::r29), "ldarh w30, [x29]");

  TEST_SINGLE(stllr(WReg::w30, Reg::r29), "stllr w30, [x29]");
  TEST_SINGLE(stlr(WReg::w30, Reg::r29), "stlr w30, [x29]");
  TEST_SINGLE(ldlar(WReg::w30, Reg::r29), "ldlar w30, [x29]");
  TEST_SINGLE(ldar(WReg::w30, Reg::r29), "ldar w30, [x29]");

  TEST_SINGLE(stllr(XReg::x30, Reg::r29), "stllr x30, [x29]");
  TEST_SINGLE(stlr(XReg::x30, Reg::r29), "stlr x30, [x29]");
  TEST_SINGLE(ldlar(XReg::x30, Reg::r29), "ldlar x30, [x29]");
  TEST_SINGLE(ldar(XReg::x30, Reg::r29), "ldar x30, [x29]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Compare and swap") {
  TEST_SINGLE(casb(Reg::r30, Reg::r29, Reg::r28), "casb w30, w29, [x28]");
  TEST_SINGLE(caslb(Reg::r30, Reg::r29, Reg::r28), "caslb w30, w29, [x28]");
  TEST_SINGLE(casab(Reg::r30, Reg::r29, Reg::r28), "casab w30, w29, [x28]");
  TEST_SINGLE(casalb(Reg::r30, Reg::r29, Reg::r28), "casalb w30, w29, [x28]");

  TEST_SINGLE(cash(Reg::r30, Reg::r29, Reg::r28), "cash w30, w29, [x28]");
  TEST_SINGLE(caslh(Reg::r30, Reg::r29, Reg::r28), "caslh w30, w29, [x28]");
  TEST_SINGLE(casah(Reg::r30, Reg::r29, Reg::r28), "casah w30, w29, [x28]");
  TEST_SINGLE(casalh(Reg::r30, Reg::r29, Reg::r28), "casalh w30, w29, [x28]");

  TEST_SINGLE(cas(WReg::w30, WReg::w29, Reg::r28), "cas w30, w29, [x28]");
  TEST_SINGLE(casl(WReg::w30, WReg::w29, Reg::r28), "casl w30, w29, [x28]");
  TEST_SINGLE(casa(WReg::w30, WReg::w29, Reg::r28), "casa w30, w29, [x28]");
  TEST_SINGLE(casal(WReg::w30, WReg::w29, Reg::r28), "casal w30, w29, [x28]");

  TEST_SINGLE(cas(XReg::x30, XReg::x29, Reg::r28), "cas x30, x29, [x28]");
  TEST_SINGLE(casl(XReg::x30, XReg::x29, Reg::r28), "casl x30, x29, [x28]");
  TEST_SINGLE(casa(XReg::x30, XReg::x29, Reg::r28), "casa x30, x29, [x28]");
  TEST_SINGLE(casal(XReg::x30, XReg::x29, Reg::r28), "casal x30, x29, [x28]");

  TEST_SINGLE(cas(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "casb w30, w29, [x28]");
  TEST_SINGLE(cas(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "cash w30, w29, [x28]");
  TEST_SINGLE(cas(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "cas w30, w29, [x28]");
  TEST_SINGLE(cas(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "cas x30, x29, [x28]");

  TEST_SINGLE(casl(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "caslb w30, w29, [x28]");
  TEST_SINGLE(casl(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "caslh w30, w29, [x28]");
  TEST_SINGLE(casl(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "casl w30, w29, [x28]");
  TEST_SINGLE(casl(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "casl x30, x29, [x28]");

  TEST_SINGLE(casa(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "casab w30, w29, [x28]");
  TEST_SINGLE(casa(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "casah w30, w29, [x28]");
  TEST_SINGLE(casa(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "casa w30, w29, [x28]");
  TEST_SINGLE(casa(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "casa x30, x29, [x28]");

  TEST_SINGLE(casal(SubRegSize::i8Bit, Reg::r30, Reg::r29, Reg::r28), "casalb w30, w29, [x28]");
  TEST_SINGLE(casal(SubRegSize::i16Bit, Reg::r30, Reg::r29, Reg::r28), "casalh w30, w29, [x28]");
  TEST_SINGLE(casal(SubRegSize::i32Bit, Reg::r30, Reg::r29, Reg::r28), "casal w30, w29, [x28]");
  TEST_SINGLE(casal(SubRegSize::i64Bit, Reg::r30, Reg::r29, Reg::r28), "casal x30, x29, [x28]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: LDAPR/STLR unscaled immediate") {
  TEST_SINGLE(stlurb(Reg::r30, Reg::r29, -256), "stlurb w30, [x29, #-256]");
  TEST_SINGLE(stlurb(Reg::r30, Reg::r29, 255), "stlurb w30, [x29, #255]");

  TEST_SINGLE(ldapurb(Reg::r30, Reg::r29, -256), "ldapurb w30, [x29, #-256]");
  TEST_SINGLE(ldapurb(Reg::r30, Reg::r29, 255), "ldapurb w30, [x29, #255]");

  TEST_SINGLE(ldapursb(WReg::w30, Reg::r29, -256), "ldapursb w30, [x29, #-256]");
  TEST_SINGLE(ldapursb(WReg::w30, Reg::r29, 255), "ldapursb w30, [x29, #255]");
  TEST_SINGLE(ldapursb(XReg::x30, Reg::r29, -256), "ldapursb x30, [x29, #-256]");
  TEST_SINGLE(ldapursb(XReg::x30, Reg::r29, 255), "ldapursb x30, [x29, #255]");

  TEST_SINGLE(stlurh(Reg::r30, Reg::r29, -256), "stlurh w30, [x29, #-256]");
  TEST_SINGLE(stlurh(Reg::r30, Reg::r29, 255), "stlurh w30, [x29, #255]");

  TEST_SINGLE(ldapurh(Reg::r30, Reg::r29, -256), "ldapurh w30, [x29, #-256]");
  TEST_SINGLE(ldapurh(Reg::r30, Reg::r29, 255), "ldapurh w30, [x29, #255]");

  TEST_SINGLE(ldapursh(WReg::w30, Reg::r29, -256), "ldapursh w30, [x29, #-256]");
  TEST_SINGLE(ldapursh(WReg::w30, Reg::r29, 255), "ldapursh w30, [x29, #255]");
  TEST_SINGLE(ldapursh(XReg::x30, Reg::r29, -256), "ldapursh x30, [x29, #-256]");
  TEST_SINGLE(ldapursh(XReg::x30, Reg::r29, 255), "ldapursh x30, [x29, #255]");

  TEST_SINGLE(stlur(WReg::w30, Reg::r29, -256), "stlur w30, [x29, #-256]");
  TEST_SINGLE(stlur(WReg::w30, Reg::r29, 255), "stlur w30, [x29, #255]");

  TEST_SINGLE(ldapur(WReg::w30, Reg::r29, -256), "ldapur w30, [x29, #-256]");
  TEST_SINGLE(ldapur(WReg::w30, Reg::r29, 255), "ldapur w30, [x29, #255]");

  TEST_SINGLE(ldapursw(XReg::x30, Reg::r29, -256), "ldapursw x30, [x29, #-256]");
  TEST_SINGLE(ldapursw(XReg::x30, Reg::r29, 255), "ldapursw x30, [x29, #255]");

  TEST_SINGLE(stlur(XReg::x30, Reg::r29, -256), "stlur x30, [x29, #-256]");
  TEST_SINGLE(stlur(XReg::x30, Reg::r29, 255), "stlur x30, [x29, #255]");

  TEST_SINGLE(ldapur(XReg::x30, Reg::r29, -256), "ldapur x30, [x29, #-256]");
  TEST_SINGLE(ldapur(XReg::x30, Reg::r29, 255), "ldapur x30, [x29, #255]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Load register literal") {
  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldr(WReg::w30, &Label);

    CHECK(DisassembleEncoding(1) == 0x18fffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldr(SReg::s30, &Label);

    CHECK(DisassembleEncoding(1) == 0x1cfffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldr(XReg::x30, &Label);

    CHECK(DisassembleEncoding(1) == 0x58fffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldr(DReg::d30, &Label);

    CHECK(DisassembleEncoding(1) == 0x5cfffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldrsw(XReg::x30, &Label);

    CHECK(DisassembleEncoding(1) == 0x98fffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    ldr(QReg::q30, &Label);

    CHECK(DisassembleEncoding(1) == 0x9cfffffe);
  }

  {
    BackwardLabel Label;
    Bind(&Label);
    dc32(0);
    prfm(Prefetch::PLDL1KEEP, &Label);

    CHECK(DisassembleEncoding(1) == 0xd8ffffe0);
  }

  {
    ForwardLabel Label;
    ldr(WReg::w30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x1800003e);
  }

  {
    ForwardLabel Label;
    ldr(SReg::s30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x1c00003e);
  }

  {
    ForwardLabel Label;
    ldr(XReg::x30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x5800003e);
  }

  {
    ForwardLabel Label;
    ldr(DReg::d30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x5c00003e);
  }

  {
    ForwardLabel Label;
    ldrsw(XReg::x30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x9800003e);
  }

  {
    ForwardLabel Label;
    ldr(QReg::q30, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0x9c00003e);
  }

  {
    ForwardLabel Label;
    prfm(Prefetch::PLDL1KEEP, &Label);
    Bind(&Label);
    dc32(0);

    CHECK(DisassembleEncoding(0) == 0xd8000020);
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Memory copy/set") {
  // Note: Some of these aren't implemented in vixl at the moment, however
  //       we supply the cases to change over to once they are. This is good,
  //       because when we update, the unimplemented cases will naturally fail,
  //       facilitating the switch.

  TEST_SINGLE(cpyfp(XReg::x30, XReg::x28, XReg::x29), "cpyfp [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfp(XReg::x17, XReg::x20, XReg::x19), "cpyfp [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfm(XReg::x30, XReg::x28, XReg::x29), "cpyfm [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfm(XReg::x17, XReg::x20, XReg::x19), "cpyfm [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfe(XReg::x30, XReg::x28, XReg::x29), "cpyfe [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfe(XReg::x17, XReg::x20, XReg::x19), "cpyfe [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpyfpwt(XReg::x30, XReg::x28, XReg::x29), "cpyfpwt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfpwt(XReg::x17, XReg::x20, XReg::x19), "cpyfpwt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfpwt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfpwt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmwt(XReg::x30, XReg::x28, XReg::x29), "cpyfmwt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmwt(XReg::x17, XReg::x20, XReg::x19), "cpyfmwt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmwt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmwt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfewt(XReg::x30, XReg::x28, XReg::x29), "cpyfewt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfewt(XReg::x17, XReg::x20, XReg::x19), "cpyfewt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfewt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfewt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfprt(XReg::x30, XReg::x28, XReg::x29), "cpyfprt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfprt(XReg::x17, XReg::x20, XReg::x19), "cpyfprt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfprt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfprt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmrt(XReg::x30, XReg::x28, XReg::x29), "cpyfmrt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmrt(XReg::x17, XReg::x20, XReg::x19), "cpyfmrt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmrt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmrt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfert(XReg::x30, XReg::x28, XReg::x29), "cpyfert [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfert(XReg::x17, XReg::x20, XReg::x19), "cpyfert [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfert(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfert(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfpt(XReg::x30, XReg::x28, XReg::x29), "cpyfpt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfpt(XReg::x17, XReg::x20, XReg::x19), "cpyfpt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfpt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfpt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmt(XReg::x30, XReg::x28, XReg::x29), "cpyfmt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmt(XReg::x17, XReg::x20, XReg::x19), "cpyfmt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfet(XReg::x30, XReg::x28, XReg::x29), "cpyfet [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfet(XReg::x17, XReg::x20, XReg::x19), "cpyfet [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfet(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfet(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpyfpwn(XReg::x30, XReg::x28, XReg::x29), "cpyfpwn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfpwn(XReg::x17, XReg::x20, XReg::x19), "cpyfpwn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfmwn(XReg::x30, XReg::x28, XReg::x29), "cpyfmwn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfmwn(XReg::x17, XReg::x20, XReg::x19), "cpyfmwn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfewn(XReg::x30, XReg::x28, XReg::x29), "cpyfewn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfewn(XReg::x17, XReg::x20, XReg::x19), "cpyfewn [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpyfpwtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfpwtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfpwtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfpwtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfpwtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfpwtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmwtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfmwtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmwtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfmwtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmwtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmwtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfewtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfewtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfewtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfewtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfewtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfewtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfprtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfprtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfprtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfprtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfprtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfprtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmrtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfmrtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmrtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfmrtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmrtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmrtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfertwn(XReg::x30, XReg::x28, XReg::x29), "cpyfertwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfertwn(XReg::x17, XReg::x20, XReg::x19), "cpyfertwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfertwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfertwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfptwn(XReg::x30, XReg::x28, XReg::x29), "cpyfptwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfptwn(XReg::x17, XReg::x20, XReg::x19), "cpyfptwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfptwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfptwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmtwn(XReg::x30, XReg::x28, XReg::x29), "cpyfmtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmtwn(XReg::x17, XReg::x20, XReg::x19), "cpyfmtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfetwn(XReg::x30, XReg::x28, XReg::x29), "cpyfetwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfetwn(XReg::x17, XReg::x20, XReg::x19), "cpyfetwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfetwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfetwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpyfprn(XReg::x30, XReg::x28, XReg::x29), "cpyfprn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfprn(XReg::x17, XReg::x20, XReg::x19), "cpyfprn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfmrn(XReg::x30, XReg::x28, XReg::x29), "cpyfmrn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfmrn(XReg::x17, XReg::x20, XReg::x19), "cpyfmrn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfern(XReg::x30, XReg::x28, XReg::x29), "cpyfern [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfern(XReg::x17, XReg::x20, XReg::x19), "cpyfern [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpyfpwtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfpwtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfpwtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfpwtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfpwtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfpwtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmwtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfmwtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmwtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfmwtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmwtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmwtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfewtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfewtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfewtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfewtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfewtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfewtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfprtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfprtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfprtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfprtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfprtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfprtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmrtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfmrtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmrtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfmrtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmrtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmrtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfertrn(XReg::x30, XReg::x28, XReg::x29), "cpyfertrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfertrn(XReg::x17, XReg::x20, XReg::x19), "cpyfertrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfertrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfertrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfptrn(XReg::x30, XReg::x28, XReg::x29), "cpyfptrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfptrn(XReg::x17, XReg::x20, XReg::x19), "cpyfptrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfptrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfptrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmtrn(XReg::x30, XReg::x28, XReg::x29), "cpyfmtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmtrn(XReg::x17, XReg::x20, XReg::x19), "cpyfmtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfetrn(XReg::x30, XReg::x28, XReg::x29), "cpyfetrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfetrn(XReg::x17, XReg::x20, XReg::x19), "cpyfetrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfetrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfetrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpyfpn(XReg::x30, XReg::x28, XReg::x29), "cpyfpn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfpn(XReg::x17, XReg::x20, XReg::x19), "cpyfpn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfmn(XReg::x30, XReg::x28, XReg::x29), "cpyfmn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfmn(XReg::x17, XReg::x20, XReg::x19), "cpyfmn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyfen(XReg::x30, XReg::x28, XReg::x29), "cpyfen [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyfen(XReg::x17, XReg::x20, XReg::x19), "cpyfen [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpyfpwtn(XReg::x30, XReg::x28, XReg::x29), "cpyfpwtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfpwtn(XReg::x17, XReg::x20, XReg::x19), "cpyfpwtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfpwtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfpwtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmwtn(XReg::x30, XReg::x28, XReg::x29), "cpyfmwtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmwtn(XReg::x17, XReg::x20, XReg::x19), "cpyfmwtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmwtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmwtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfewtn(XReg::x30, XReg::x28, XReg::x29), "cpyfewtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfewtn(XReg::x17, XReg::x20, XReg::x19), "cpyfewtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfewtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfewtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfprtn(XReg::x30, XReg::x28, XReg::x29), "cpyfprtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfprtn(XReg::x17, XReg::x20, XReg::x19), "cpyfprtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfprtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfprtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmrtn(XReg::x30, XReg::x28, XReg::x29), "cpyfmrtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmrtn(XReg::x17, XReg::x20, XReg::x19), "cpyfmrtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmrtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmrtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfertn(XReg::x30, XReg::x28, XReg::x29), "cpyfertn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfertn(XReg::x17, XReg::x20, XReg::x19), "cpyfertn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfertn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfertn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfptn(XReg::x30, XReg::x28, XReg::x29), "cpyfptn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfptn(XReg::x17, XReg::x20, XReg::x19), "cpyfptn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfptn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfptn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfmtn(XReg::x30, XReg::x28, XReg::x29), "cpyfmtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfmtn(XReg::x17, XReg::x20, XReg::x19), "cpyfmtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfmtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfmtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyfetn(XReg::x30, XReg::x28, XReg::x29), "cpyfetn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyfetn(XReg::x17, XReg::x20, XReg::x19), "cpyfetn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyfetn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyfetn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(setp(XReg::x30, XReg::x28, XReg::x29), "setp [x30]!, x28!, x29");
  TEST_SINGLE(setp(XReg::x17, XReg::x20, XReg::x19), "setp [x17]!, x20!, x19");

  TEST_SINGLE(setm(XReg::x30, XReg::x28, XReg::x29), "setm [x30]!, x28!, x29");
  TEST_SINGLE(setm(XReg::x17, XReg::x20, XReg::x19), "setm [x17]!, x20!, x19");

  TEST_SINGLE(sete(XReg::x30, XReg::x28, XReg::x29), "sete [x30]!, x28!, x29");
  TEST_SINGLE(sete(XReg::x17, XReg::x20, XReg::x19), "sete [x17]!, x20!, x19");

  // TEST_SINGLE(setpt(XReg::x30, XReg::x28, XReg::x29), "setpt [x30]!, x28!, x29");
  // TEST_SINGLE(setpt(XReg::x17, XReg::x20, XReg::x19), "setpt [x17]!, x20!, x19");
  TEST_SINGLE(setpt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setpt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setmt(XReg::x30, XReg::x28, XReg::x29), "setmt [x30]!, x28!, x29");
  // TEST_SINGLE(setmt(XReg::x17, XReg::x20, XReg::x19), "setmt [x17]!, x20!, x19");
  TEST_SINGLE(setmt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setmt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setet(XReg::x30, XReg::x28, XReg::x29), "setet [x30]!, x28!, x29");
  // TEST_SINGLE(setet(XReg::x17, XReg::x20, XReg::x19), "setet [x17]!, x20!, x19");
  TEST_SINGLE(setet(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setet(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(setpn(XReg::x30, XReg::x28, XReg::x29), "setpn [x30]!, x28!, x29");
  TEST_SINGLE(setpn(XReg::x17, XReg::x20, XReg::x19), "setpn [x17]!, x20!, x19");

  TEST_SINGLE(setmn(XReg::x30, XReg::x28, XReg::x29), "setmn [x30]!, x28!, x29");
  TEST_SINGLE(setmn(XReg::x17, XReg::x20, XReg::x19), "setmn [x17]!, x20!, x19");

  TEST_SINGLE(seten(XReg::x30, XReg::x28, XReg::x29), "seten [x30]!, x28!, x29");
  TEST_SINGLE(seten(XReg::x17, XReg::x20, XReg::x19), "seten [x17]!, x20!, x19");

  // TEST_SINGLE(setptn(XReg::x30, XReg::x28, XReg::x29), "setptn [x30]!, x28!, x29");
  // TEST_SINGLE(setptn(XReg::x17, XReg::x20, XReg::x19), "setptn [x17]!, x20!, x19");
  TEST_SINGLE(setptn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setptn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setmtn(XReg::x30, XReg::x28, XReg::x29), "setmtn [x30]!, x28!, x29");
  // TEST_SINGLE(setmtn(XReg::x17, XReg::x20, XReg::x19), "setmtn [x17]!, x20!, x19");
  TEST_SINGLE(setmtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setmtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setetn(XReg::x30, XReg::x28, XReg::x29), "setetn [x30]!, x28!, x29");
  // TEST_SINGLE(setetn(XReg::x17, XReg::x20, XReg::x19), "setetn [x17]!, x20!, x19");
  TEST_SINGLE(setetn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setetn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpyp(XReg::x30, XReg::x28, XReg::x29), "cpyp [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyp(XReg::x17, XReg::x20, XReg::x19), "cpyp [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpym(XReg::x30, XReg::x28, XReg::x29), "cpym [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpym(XReg::x17, XReg::x20, XReg::x19), "cpym [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpye(XReg::x30, XReg::x28, XReg::x29), "cpye [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpye(XReg::x17, XReg::x20, XReg::x19), "cpye [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpypwt(XReg::x30, XReg::x28, XReg::x29), "cpypwt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpypwt(XReg::x17, XReg::x20, XReg::x19), "cpypwt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpypwt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpypwt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymwt(XReg::x30, XReg::x28, XReg::x29), "cpymwt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymwt(XReg::x17, XReg::x20, XReg::x19), "cpymwt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymwt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymwt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyewt(XReg::x30, XReg::x28, XReg::x29), "cpyewt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyewt(XReg::x17, XReg::x20, XReg::x19), "cpyewt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyewt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyewt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyprt(XReg::x30, XReg::x28, XReg::x29), "cpyprt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyprt(XReg::x17, XReg::x20, XReg::x19), "cpyprt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyprt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyprt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymrt(XReg::x30, XReg::x28, XReg::x29), "cpymrt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymrt(XReg::x17, XReg::x20, XReg::x19), "cpymrt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymrt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymrt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyert(XReg::x30, XReg::x28, XReg::x29), "cpyert [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyert(XReg::x17, XReg::x20, XReg::x19), "cpyert [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyert(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyert(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpypt(XReg::x30, XReg::x28, XReg::x29), "cpypt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpypt(XReg::x17, XReg::x20, XReg::x19), "cpypt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpypt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpypt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymt(XReg::x30, XReg::x28, XReg::x29), "cpymt [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymt(XReg::x17, XReg::x20, XReg::x19), "cpymt [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyet(XReg::x30, XReg::x28, XReg::x29), "cpyet [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyet(XReg::x17, XReg::x20, XReg::x19), "cpyet [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyet(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyet(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpypwn(XReg::x30, XReg::x28, XReg::x29), "cpypwn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpypwn(XReg::x17, XReg::x20, XReg::x19), "cpypwn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpymwn(XReg::x30, XReg::x28, XReg::x29), "cpymwn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpymwn(XReg::x17, XReg::x20, XReg::x19), "cpymwn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyewn(XReg::x30, XReg::x28, XReg::x29), "cpyewn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyewn(XReg::x17, XReg::x20, XReg::x19), "cpyewn [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpypwtwn(XReg::x30, XReg::x28, XReg::x29), "cpypwtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpypwtwn(XReg::x17, XReg::x20, XReg::x19), "cpypwtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpypwtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpypwtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymwtwn(XReg::x30, XReg::x28, XReg::x29), "cpymwtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymwtwn(XReg::x17, XReg::x20, XReg::x19), "cpymwtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymwtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymwtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyewtwn(XReg::x30, XReg::x28, XReg::x29), "cpyewtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyewtwn(XReg::x17, XReg::x20, XReg::x19), "cpyewtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyewtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyewtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyprtwn(XReg::x30, XReg::x28, XReg::x29), "cpyprtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyprtwn(XReg::x17, XReg::x20, XReg::x19), "cpyprtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyprtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyprtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymrtwn(XReg::x30, XReg::x28, XReg::x29), "cpymrtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymrtwn(XReg::x17, XReg::x20, XReg::x19), "cpymrtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymrtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymrtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyertwn(XReg::x30, XReg::x28, XReg::x29), "cpyertwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyertwn(XReg::x17, XReg::x20, XReg::x19), "cpyertwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyertwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyertwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyptwn(XReg::x30, XReg::x28, XReg::x29), "cpyptwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyptwn(XReg::x17, XReg::x20, XReg::x19), "cpyptwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyptwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyptwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymtwn(XReg::x30, XReg::x28, XReg::x29), "cpymtwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymtwn(XReg::x17, XReg::x20, XReg::x19), "cpymtwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymtwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymtwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyetwn(XReg::x30, XReg::x28, XReg::x29), "cpyetwn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyetwn(XReg::x17, XReg::x20, XReg::x19), "cpyetwn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyetwn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyetwn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpyprn(XReg::x30, XReg::x28, XReg::x29), "cpyprn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyprn(XReg::x17, XReg::x20, XReg::x19), "cpyprn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpymrn(XReg::x30, XReg::x28, XReg::x29), "cpymrn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpymrn(XReg::x17, XReg::x20, XReg::x19), "cpymrn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyern(XReg::x30, XReg::x28, XReg::x29), "cpyern [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyern(XReg::x17, XReg::x20, XReg::x19), "cpyern [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpypwtrn(XReg::x30, XReg::x28, XReg::x29), "cpypwtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpypwtrn(XReg::x17, XReg::x20, XReg::x19), "cpypwtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpypwtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpypwtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymwtrn(XReg::x30, XReg::x28, XReg::x29), "cpymwtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymwtrn(XReg::x17, XReg::x20, XReg::x19), "cpymwtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymwtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymwtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyewtrn(XReg::x30, XReg::x28, XReg::x29), "cpyewtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyewtrn(XReg::x17, XReg::x20, XReg::x19), "cpyewtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyewtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyewtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyprtrn(XReg::x30, XReg::x28, XReg::x29), "cpyprtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyprtrn(XReg::x17, XReg::x20, XReg::x19), "cpyprtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyprtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyprtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymrtrn(XReg::x30, XReg::x28, XReg::x29), "cpymrtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymrtrn(XReg::x17, XReg::x20, XReg::x19), "cpymrtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymrtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymrtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyertrn(XReg::x30, XReg::x28, XReg::x29), "cpyertrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyertrn(XReg::x17, XReg::x20, XReg::x19), "cpyertrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyertrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyertrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyptrn(XReg::x30, XReg::x28, XReg::x29), "cpyptrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyptrn(XReg::x17, XReg::x20, XReg::x19), "cpyptrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyptrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyptrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymtrn(XReg::x30, XReg::x28, XReg::x29), "cpymtrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymtrn(XReg::x17, XReg::x20, XReg::x19), "cpymtrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymtrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymtrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyetrn(XReg::x30, XReg::x28, XReg::x29), "cpyetrn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyetrn(XReg::x17, XReg::x20, XReg::x19), "cpyetrn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyetrn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyetrn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(cpypn(XReg::x30, XReg::x28, XReg::x29), "cpypn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpypn(XReg::x17, XReg::x20, XReg::x19), "cpypn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpymn(XReg::x30, XReg::x28, XReg::x29), "cpymn [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpymn(XReg::x17, XReg::x20, XReg::x19), "cpymn [x17]!, [x20]!, x19!");

  TEST_SINGLE(cpyen(XReg::x30, XReg::x28, XReg::x29), "cpyen [x30]!, [x28]!, x29!");
  TEST_SINGLE(cpyen(XReg::x17, XReg::x20, XReg::x19), "cpyen [x17]!, [x20]!, x19!");

  // TEST_SINGLE(cpypwtn(XReg::x30, XReg::x28, XReg::x29), "cpypwtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpypwtn(XReg::x17, XReg::x20, XReg::x19), "cpypwtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpypwtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpypwtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymwtn(XReg::x30, XReg::x28, XReg::x29), "cpymwtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymwtn(XReg::x17, XReg::x20, XReg::x19), "cpymwtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymwtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymwtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyewtn(XReg::x30, XReg::x28, XReg::x29), "cpyewtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyewtn(XReg::x17, XReg::x20, XReg::x19), "cpyewtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyewtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyewtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyprtn(XReg::x30, XReg::x28, XReg::x29), "cpyprtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyprtn(XReg::x17, XReg::x20, XReg::x19), "cpyprtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyprtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyprtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymrtn(XReg::x30, XReg::x28, XReg::x29), "cpymrtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymrtn(XReg::x17, XReg::x20, XReg::x19), "cpymrtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymrtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymrtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyertn(XReg::x30, XReg::x28, XReg::x29), "cpyertn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyertn(XReg::x17, XReg::x20, XReg::x19), "cpyertn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyertn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyertn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyptn(XReg::x30, XReg::x28, XReg::x29), "cpyptn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyptn(XReg::x17, XReg::x20, XReg::x19), "cpyptn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyptn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyptn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpymtn(XReg::x30, XReg::x28, XReg::x29), "cpymtn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpymtn(XReg::x17, XReg::x20, XReg::x19), "cpymtn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpymtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpymtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(cpyetn(XReg::x30, XReg::x28, XReg::x29), "cpyetn [x30]!, [x28]!, x29!");
  // TEST_SINGLE(cpyetn(XReg::x17, XReg::x20, XReg::x19), "cpyetn [x17]!, [x20]!, x19!");
  TEST_SINGLE(cpyetn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(cpyetn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(setgp(XReg::x30, XReg::x28, XReg::x29), "setgp [x30]!, x28!, x29");
  TEST_SINGLE(setgp(XReg::x17, XReg::x20, XReg::x19), "setgp [x17]!, x20!, x19");

  TEST_SINGLE(setgm(XReg::x30, XReg::x28, XReg::x29), "setgm [x30]!, x28!, x29");
  TEST_SINGLE(setgm(XReg::x17, XReg::x20, XReg::x19), "setgm [x17]!, x20!, x19");

  TEST_SINGLE(setge(XReg::x30, XReg::x28, XReg::x29), "setge [x30]!, x28!, x29");
  TEST_SINGLE(setge(XReg::x17, XReg::x20, XReg::x19), "setge [x17]!, x20!, x19");

  // TEST_SINGLE(setgpt(XReg::x30, XReg::x28, XReg::x29), "setgpt [x30]!, x28!, x29");
  // TEST_SINGLE(setgpt(XReg::x17, XReg::x20, XReg::x19), "setgpt [x17]!, x20!, x19");
  TEST_SINGLE(setgpt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setgpt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setgmt(XReg::x30, XReg::x28, XReg::x29), "setgmt [x30]!, x28!, x29");
  // TEST_SINGLE(setgmt(XReg::x17, XReg::x20, XReg::x19), "setgmt [x17]!, x20!, x19");
  TEST_SINGLE(setgmt(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setgmt(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setget(XReg::x30, XReg::x28, XReg::x29), "setget [x30]!, x28!, x29");
  // TEST_SINGLE(setget(XReg::x17, XReg::x20, XReg::x19), "setget [x17]!, x20!, x19");
  TEST_SINGLE(setget(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setget(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  TEST_SINGLE(setgpn(XReg::x30, XReg::x28, XReg::x29), "setgpn [x30]!, x28!, x29");
  TEST_SINGLE(setgpn(XReg::x17, XReg::x20, XReg::x19), "setgpn [x17]!, x20!, x19");

  TEST_SINGLE(setgmn(XReg::x30, XReg::x28, XReg::x29), "setgmn [x30]!, x28!, x29");
  TEST_SINGLE(setgmn(XReg::x17, XReg::x20, XReg::x19), "setgmn [x17]!, x20!, x19");

  TEST_SINGLE(setgen(XReg::x30, XReg::x28, XReg::x29), "setgen [x30]!, x28!, x29");
  TEST_SINGLE(setgen(XReg::x17, XReg::x20, XReg::x19), "setgen [x17]!, x20!, x19");

  // TEST_SINGLE(setgptn(XReg::x30, XReg::x28, XReg::x29), "setgptn [x30]!, x28!, x29");
  // TEST_SINGLE(setgptn(XReg::x17, XReg::x20, XReg::x19), "setgptn [x17]!, x20!, x19");
  TEST_SINGLE(setgptn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setgptn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setgmtn(XReg::x30, XReg::x28, XReg::x29), "setgmtn [x30]!, x28!, x29");
  // TEST_SINGLE(setgmtn(XReg::x17, XReg::x20, XReg::x19), "setgmtn [x17]!, x20!, x19");
  TEST_SINGLE(setgmtn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setgmtn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");

  // TEST_SINGLE(setgetn(XReg::x30, XReg::x28, XReg::x29), "setgetn [x30]!, x28!, x29");
  // TEST_SINGLE(setgetn(XReg::x17, XReg::x20, XReg::x19), "setgetn [x17]!, x20!, x19");
  TEST_SINGLE(setgetn(XReg::x30, XReg::x28, XReg::x29), "unimplemented (Unimplemented)");
  TEST_SINGLE(setgetn(XReg::x17, XReg::x20, XReg::x19), "unimplemented (Unimplemented)");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore no-allocate pair") {
  TEST_SINGLE(stnp(WReg::w30, WReg::w28, Reg::r29, -256), "stnp w30, w28, [x29, #-256]");
  TEST_SINGLE(stnp(WReg::w30, WReg::w28, Reg::r29, 252), "stnp w30, w28, [x29, #252]");

  TEST_SINGLE(ldnp(WReg::w30, WReg::w28, Reg::r29, -256), "ldnp w30, w28, [x29, #-256]");
  TEST_SINGLE(ldnp(WReg::w30, WReg::w28, Reg::r29, 252), "ldnp w30, w28, [x29, #252]");

  TEST_SINGLE(stnp(SReg::s30, SReg::s28, Reg::r29, -256), "stnp s30, s28, [x29, #-256]");
  TEST_SINGLE(stnp(SReg::s30, SReg::s28, Reg::r29, 252), "stnp s30, s28, [x29, #252]");

  TEST_SINGLE(ldnp(SReg::s30, SReg::s28, Reg::r29, -256), "ldnp s30, s28, [x29, #-256]");
  TEST_SINGLE(ldnp(SReg::s30, SReg::s28, Reg::r29, 252), "ldnp s30, s28, [x29, #252]");

  TEST_SINGLE(stnp(XReg::x30, XReg::x28, Reg::r29, -512), "stnp x30, x28, [x29, #-512]");
  TEST_SINGLE(stnp(XReg::x30, XReg::x28, Reg::r29, 504), "stnp x30, x28, [x29, #504]");

  TEST_SINGLE(ldnp(XReg::x30, XReg::x28, Reg::r29, -512), "ldnp x30, x28, [x29, #-512]");
  TEST_SINGLE(ldnp(XReg::x30, XReg::x28, Reg::r29, 504), "ldnp x30, x28, [x29, #504]");

  TEST_SINGLE(stnp(DReg::d30, DReg::d28, Reg::r29, -512), "stnp d30, d28, [x29, #-512]");
  TEST_SINGLE(stnp(DReg::d30, DReg::d28, Reg::r29, 504), "stnp d30, d28, [x29, #504]");

  TEST_SINGLE(ldnp(DReg::d30, DReg::d28, Reg::r29, -512), "ldnp d30, d28, [x29, #-512]");
  TEST_SINGLE(ldnp(DReg::d30, DReg::d28, Reg::r29, 504), "ldnp d30, d28, [x29, #504]");

  TEST_SINGLE(stnp(QReg::q30, QReg::q28, Reg::r29, -1024), "stnp q30, q28, [x29, #-1024]");
  TEST_SINGLE(stnp(QReg::q30, QReg::q28, Reg::r29, 1008), "stnp q30, q28, [x29, #1008]");

  TEST_SINGLE(ldnp(QReg::q30, QReg::q28, Reg::r29, -1024), "ldnp q30, q28, [x29, #-1024]");
  TEST_SINGLE(ldnp(QReg::q30, QReg::q28, Reg::r29, 1008), "ldnp q30, q28, [x29, #1008]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register pair post-indexed") {
  TEST_SINGLE(stp<IndexType::POST>(WReg::w30, WReg::w28, Reg::r29, -256), "stp w30, w28, [x29], #-256");
  TEST_SINGLE(stp<IndexType::POST>(WReg::w30, WReg::w28, Reg::r29, 252), "stp w30, w28, [x29], #252");

  TEST_SINGLE(ldp<IndexType::POST>(WReg::w30, WReg::w28, Reg::r29, -256), "ldp w30, w28, [x29], #-256");
  TEST_SINGLE(ldp<IndexType::POST>(WReg::w30, WReg::w28, Reg::r29, 252), "ldp w30, w28, [x29], #252");

  TEST_SINGLE(ldpsw<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, -256), "ldpsw x30, x28, [x29], #-256");
  TEST_SINGLE(ldpsw<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, 252), "ldpsw x30, x28, [x29], #252");

  TEST_SINGLE(stp<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, -512), "stp x30, x28, [x29], #-512");
  TEST_SINGLE(stp<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, 504), "stp x30, x28, [x29], #504");

  TEST_SINGLE(ldp<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, -512), "ldp x30, x28, [x29], #-512");
  TEST_SINGLE(ldp<IndexType::POST>(XReg::x30, XReg::x28, Reg::r29, 504), "ldp x30, x28, [x29], #504");

  TEST_SINGLE(stp<IndexType::POST>(SReg::s30, SReg::s28, Reg::r29, -256), "stp s30, s28, [x29], #-256");
  TEST_SINGLE(stp<IndexType::POST>(SReg::s30, SReg::s28, Reg::r29, 252), "stp s30, s28, [x29], #252");

  TEST_SINGLE(ldp<IndexType::POST>(SReg::s30, SReg::s28, Reg::r29, -256), "ldp s30, s28, [x29], #-256");
  TEST_SINGLE(ldp<IndexType::POST>(SReg::s30, SReg::s28, Reg::r29, 252), "ldp s30, s28, [x29], #252");

  TEST_SINGLE(stp<IndexType::POST>(DReg::d30, DReg::d28, Reg::r29, -512), "stp d30, d28, [x29], #-512");
  TEST_SINGLE(stp<IndexType::POST>(DReg::d30, DReg::d28, Reg::r29, 504), "stp d30, d28, [x29], #504");

  TEST_SINGLE(ldp<IndexType::POST>(DReg::d30, DReg::d28, Reg::r29, -512), "ldp d30, d28, [x29], #-512");
  TEST_SINGLE(ldp<IndexType::POST>(DReg::d30, DReg::d28, Reg::r29, 504), "ldp d30, d28, [x29], #504");

  TEST_SINGLE(stp<IndexType::POST>(QReg::q30, QReg::q28, Reg::r29, -1024), "stp q30, q28, [x29], #-1024");
  TEST_SINGLE(stp<IndexType::POST>(QReg::q30, QReg::q28, Reg::r29, 1008), "stp q30, q28, [x29], #1008");

  TEST_SINGLE(ldp<IndexType::POST>(QReg::q30, QReg::q28, Reg::r29, -1024), "ldp q30, q28, [x29], #-1024");
  TEST_SINGLE(ldp<IndexType::POST>(QReg::q30, QReg::q28, Reg::r29, 1008), "ldp q30, q28, [x29], #1008");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register pair offset") {
  TEST_SINGLE(stp<IndexType::OFFSET>(WReg::w30, WReg::w28, Reg::r29, -256), "stp w30, w28, [x29, #-256]");
  TEST_SINGLE(stp<IndexType::OFFSET>(WReg::w30, WReg::w28, Reg::r29, 252), "stp w30, w28, [x29, #252]");

  TEST_SINGLE(ldp<IndexType::OFFSET>(WReg::w30, WReg::w28, Reg::r29, -256), "ldp w30, w28, [x29, #-256]");
  TEST_SINGLE(ldp<IndexType::OFFSET>(WReg::w30, WReg::w28, Reg::r29, 252), "ldp w30, w28, [x29, #252]");

  TEST_SINGLE(ldpsw<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, -256), "ldpsw x30, x28, [x29, #-256]");
  TEST_SINGLE(ldpsw<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, 252), "ldpsw x30, x28, [x29, #252]");

  TEST_SINGLE(stp<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, -512), "stp x30, x28, [x29, #-512]");
  TEST_SINGLE(stp<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, 504), "stp x30, x28, [x29, #504]");

  TEST_SINGLE(ldp<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, -512), "ldp x30, x28, [x29, #-512]");
  TEST_SINGLE(ldp<IndexType::OFFSET>(XReg::x30, XReg::x28, Reg::r29, 504), "ldp x30, x28, [x29, #504]");

  TEST_SINGLE(stp<IndexType::OFFSET>(SReg::s30, SReg::s28, Reg::r29, -256), "stp s30, s28, [x29, #-256]");
  TEST_SINGLE(stp<IndexType::OFFSET>(SReg::s30, SReg::s28, Reg::r29, 252), "stp s30, s28, [x29, #252]");

  TEST_SINGLE(ldp<IndexType::OFFSET>(SReg::s30, SReg::s28, Reg::r29, -256), "ldp s30, s28, [x29, #-256]");
  TEST_SINGLE(ldp<IndexType::OFFSET>(SReg::s30, SReg::s28, Reg::r29, 252), "ldp s30, s28, [x29, #252]");

  TEST_SINGLE(stp<IndexType::OFFSET>(DReg::d30, DReg::d28, Reg::r29, -512), "stp d30, d28, [x29, #-512]");
  TEST_SINGLE(stp<IndexType::OFFSET>(DReg::d30, DReg::d28, Reg::r29, 504), "stp d30, d28, [x29, #504]");

  TEST_SINGLE(ldp<IndexType::OFFSET>(DReg::d30, DReg::d28, Reg::r29, -512), "ldp d30, d28, [x29, #-512]");
  TEST_SINGLE(ldp<IndexType::OFFSET>(DReg::d30, DReg::d28, Reg::r29, 504), "ldp d30, d28, [x29, #504]");

  TEST_SINGLE(stp<IndexType::OFFSET>(QReg::q30, QReg::q28, Reg::r29, -1024), "stp q30, q28, [x29, #-1024]");
  TEST_SINGLE(stp<IndexType::OFFSET>(QReg::q30, QReg::q28, Reg::r29, 1008), "stp q30, q28, [x29, #1008]");

  TEST_SINGLE(ldp<IndexType::OFFSET>(QReg::q30, QReg::q28, Reg::r29, -1024), "ldp q30, q28, [x29, #-1024]");
  TEST_SINGLE(ldp<IndexType::OFFSET>(QReg::q30, QReg::q28, Reg::r29, 1008), "ldp q30, q28, [x29, #1008]");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register pair pre-indexed") {
  TEST_SINGLE(stp<IndexType::PRE>(WReg::w30, WReg::w28, Reg::r29, -256), "stp w30, w28, [x29, #-256]!");
  TEST_SINGLE(stp<IndexType::PRE>(WReg::w30, WReg::w28, Reg::r29, 252), "stp w30, w28, [x29, #252]!");

  TEST_SINGLE(ldp<IndexType::PRE>(WReg::w30, WReg::w28, Reg::r29, -256), "ldp w30, w28, [x29, #-256]!");
  TEST_SINGLE(ldp<IndexType::PRE>(WReg::w30, WReg::w28, Reg::r29, 252), "ldp w30, w28, [x29, #252]!");

  TEST_SINGLE(ldpsw<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, -256), "ldpsw x30, x28, [x29, #-256]!");
  TEST_SINGLE(ldpsw<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, 252), "ldpsw x30, x28, [x29, #252]!");

  TEST_SINGLE(stp<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, -512), "stp x30, x28, [x29, #-512]!");
  TEST_SINGLE(stp<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, 504), "stp x30, x28, [x29, #504]!");

  TEST_SINGLE(ldp<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, -512), "ldp x30, x28, [x29, #-512]!");
  TEST_SINGLE(ldp<IndexType::PRE>(XReg::x30, XReg::x28, Reg::r29, 504), "ldp x30, x28, [x29, #504]!");

  TEST_SINGLE(stp<IndexType::PRE>(SReg::s30, SReg::s28, Reg::r29, -256), "stp s30, s28, [x29, #-256]!");
  TEST_SINGLE(stp<IndexType::PRE>(SReg::s30, SReg::s28, Reg::r29, 252), "stp s30, s28, [x29, #252]!");

  TEST_SINGLE(ldp<IndexType::PRE>(SReg::s30, SReg::s28, Reg::r29, -256), "ldp s30, s28, [x29, #-256]!");
  TEST_SINGLE(ldp<IndexType::PRE>(SReg::s30, SReg::s28, Reg::r29, 252), "ldp s30, s28, [x29, #252]!");

  TEST_SINGLE(stp<IndexType::PRE>(DReg::d30, DReg::d28, Reg::r29, -512), "stp d30, d28, [x29, #-512]!");
  TEST_SINGLE(stp<IndexType::PRE>(DReg::d30, DReg::d28, Reg::r29, 504), "stp d30, d28, [x29, #504]!");

  TEST_SINGLE(ldp<IndexType::PRE>(DReg::d30, DReg::d28, Reg::r29, -512), "ldp d30, d28, [x29, #-512]!");
  TEST_SINGLE(ldp<IndexType::PRE>(DReg::d30, DReg::d28, Reg::r29, 504), "ldp d30, d28, [x29, #504]!");

  TEST_SINGLE(stp<IndexType::PRE>(QReg::q30, QReg::q28, Reg::r29, -1024), "stp q30, q28, [x29, #-1024]!");
  TEST_SINGLE(stp<IndexType::PRE>(QReg::q30, QReg::q28, Reg::r29, 1008), "stp q30, q28, [x29, #1008]!");

  TEST_SINGLE(ldp<IndexType::PRE>(QReg::q30, QReg::q28, Reg::r29, -1024), "ldp q30, q28, [x29, #-1024]!");
  TEST_SINGLE(ldp<IndexType::PRE>(QReg::q30, QReg::q28, Reg::r29, 1008), "ldp q30, q28, [x29, #1008]!");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register immediate post-indexed") {
  TEST_SINGLE(strb<IndexType::POST>(Reg::r30, Reg::r29, -256), "strb w30, [x29], #-256");
  TEST_SINGLE(strb<IndexType::POST>(Reg::r30, Reg::r29, 255), "strb w30, [x29], #255");
  TEST_SINGLE(ldrb<IndexType::POST>(Reg::r30, Reg::r29, -256), "ldrb w30, [x29], #-256");
  TEST_SINGLE(ldrb<IndexType::POST>(Reg::r30, Reg::r29, 255), "ldrb w30, [x29], #255");

  TEST_SINGLE(strb<IndexType::POST>(VReg::v30, Reg::r29, -256), "str b30, [x29], #-256");
  TEST_SINGLE(strb<IndexType::POST>(VReg::v30, Reg::r29, 255), "str b30, [x29], #255");
  TEST_SINGLE(ldrb<IndexType::POST>(VReg::v30, Reg::r29, -256), "ldr b30, [x29], #-256");
  TEST_SINGLE(ldrb<IndexType::POST>(VReg::v30, Reg::r29, 255), "ldr b30, [x29], #255");

  TEST_SINGLE(ldrsb<IndexType::POST>(WReg::w30, Reg::r29, -256), "ldrsb w30, [x29], #-256");
  TEST_SINGLE(ldrsb<IndexType::POST>(WReg::w30, Reg::r29, 255), "ldrsb w30, [x29], #255");
  TEST_SINGLE(ldrsb<IndexType::POST>(XReg::x30, Reg::r29, -256), "ldrsb x30, [x29], #-256");
  TEST_SINGLE(ldrsb<IndexType::POST>(XReg::x30, Reg::r29, 255), "ldrsb x30, [x29], #255");

  TEST_SINGLE(strh<IndexType::POST>(Reg::r30, Reg::r29, -256), "strh w30, [x29], #-256");
  TEST_SINGLE(strh<IndexType::POST>(Reg::r30, Reg::r29, 255), "strh w30, [x29], #255");
  TEST_SINGLE(ldrh<IndexType::POST>(Reg::r30, Reg::r29, -256), "ldrh w30, [x29], #-256");
  TEST_SINGLE(ldrh<IndexType::POST>(Reg::r30, Reg::r29, 255), "ldrh w30, [x29], #255");

  TEST_SINGLE(strh<IndexType::POST>(VReg::v30, Reg::r29, -256), "str h30, [x29], #-256");
  TEST_SINGLE(strh<IndexType::POST>(VReg::v30, Reg::r29, 255), "str h30, [x29], #255");
  TEST_SINGLE(ldrh<IndexType::POST>(VReg::v30, Reg::r29, -256), "ldr h30, [x29], #-256");
  TEST_SINGLE(ldrh<IndexType::POST>(VReg::v30, Reg::r29, 255), "ldr h30, [x29], #255");

  TEST_SINGLE(ldrsh<IndexType::POST>(WReg::w30, Reg::r29, -256), "ldrsh w30, [x29], #-256");
  TEST_SINGLE(ldrsh<IndexType::POST>(WReg::w30, Reg::r29, 255), "ldrsh w30, [x29], #255");
  TEST_SINGLE(ldrsh<IndexType::POST>(XReg::x30, Reg::r29, -256), "ldrsh x30, [x29], #-256");
  TEST_SINGLE(ldrsh<IndexType::POST>(XReg::x30, Reg::r29, 255), "ldrsh x30, [x29], #255");

  TEST_SINGLE(str<IndexType::POST>(WReg::w30, Reg::r29, -256), "str w30, [x29], #-256");
  TEST_SINGLE(str<IndexType::POST>(WReg::w30, Reg::r29, 255), "str w30, [x29], #255");
  TEST_SINGLE(ldr<IndexType::POST>(WReg::w30, Reg::r29, -256), "ldr w30, [x29], #-256");
  TEST_SINGLE(ldr<IndexType::POST>(WReg::w30, Reg::r29, 255), "ldr w30, [x29], #255");

  TEST_SINGLE(str<IndexType::POST>(SReg::s30, Reg::r29, -256), "str s30, [x29], #-256");
  TEST_SINGLE(str<IndexType::POST>(SReg::s30, Reg::r29, 255), "str s30, [x29], #255");
  TEST_SINGLE(ldr<IndexType::POST>(SReg::s30, Reg::r29, -256), "ldr s30, [x29], #-256");
  TEST_SINGLE(ldr<IndexType::POST>(SReg::s30, Reg::r29, 255), "ldr s30, [x29], #255");

  TEST_SINGLE(ldrsw<IndexType::POST>(XReg::x30, Reg::r29, -256), "ldrsw x30, [x29], #-256");
  TEST_SINGLE(ldrsw<IndexType::POST>(XReg::x30, Reg::r29, 255), "ldrsw x30, [x29], #255");

  TEST_SINGLE(str<IndexType::POST>(XReg::x30, Reg::r29, -256), "str x30, [x29], #-256");
  TEST_SINGLE(str<IndexType::POST>(XReg::x30, Reg::r29, 255), "str x30, [x29], #255");
  TEST_SINGLE(ldr<IndexType::POST>(XReg::x30, Reg::r29, -256), "ldr x30, [x29], #-256");
  TEST_SINGLE(ldr<IndexType::POST>(XReg::x30, Reg::r29, 255), "ldr x30, [x29], #255");

  TEST_SINGLE(str<IndexType::POST>(DReg::d30, Reg::r29, -256), "str d30, [x29], #-256");
  TEST_SINGLE(str<IndexType::POST>(DReg::d30, Reg::r29, 255), "str d30, [x29], #255");
  TEST_SINGLE(ldr<IndexType::POST>(DReg::d30, Reg::r29, -256), "ldr d30, [x29], #-256");
  TEST_SINGLE(ldr<IndexType::POST>(DReg::d30, Reg::r29, 255), "ldr d30, [x29], #255");

  TEST_SINGLE(str<IndexType::POST>(QReg::q30, Reg::r29, -256), "str q30, [x29], #-256");
  TEST_SINGLE(str<IndexType::POST>(QReg::q30, Reg::r29, 255), "str q30, [x29], #255");
  TEST_SINGLE(ldr<IndexType::POST>(QReg::q30, Reg::r29, -256), "ldr q30, [x29], #-256");
  TEST_SINGLE(ldr<IndexType::POST>(QReg::q30, Reg::r29, 255), "ldr q30, [x29], #255");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register immediate pre-indexed") {
  TEST_SINGLE(strb<IndexType::PRE>(Reg::r30, Reg::r29, -256), "strb w30, [x29, #-256]!");
  TEST_SINGLE(strb<IndexType::PRE>(Reg::r30, Reg::r29, 255), "strb w30, [x29, #255]!");
  TEST_SINGLE(ldrb<IndexType::PRE>(Reg::r30, Reg::r29, -256), "ldrb w30, [x29, #-256]!");
  TEST_SINGLE(ldrb<IndexType::PRE>(Reg::r30, Reg::r29, 255), "ldrb w30, [x29, #255]!");

  TEST_SINGLE(strb<IndexType::PRE>(VReg::v30, Reg::r29, -256), "str b30, [x29, #-256]!");
  TEST_SINGLE(strb<IndexType::PRE>(VReg::v30, Reg::r29, 255), "str b30, [x29, #255]!");
  TEST_SINGLE(ldrb<IndexType::PRE>(VReg::v30, Reg::r29, -256), "ldr b30, [x29, #-256]!");
  TEST_SINGLE(ldrb<IndexType::PRE>(VReg::v30, Reg::r29, 255), "ldr b30, [x29, #255]!");

  TEST_SINGLE(ldrsb<IndexType::PRE>(WReg::w30, Reg::r29, -256), "ldrsb w30, [x29, #-256]!");
  TEST_SINGLE(ldrsb<IndexType::PRE>(WReg::w30, Reg::r29, 255), "ldrsb w30, [x29, #255]!");
  TEST_SINGLE(ldrsb<IndexType::PRE>(XReg::x30, Reg::r29, -256), "ldrsb x30, [x29, #-256]!");
  TEST_SINGLE(ldrsb<IndexType::PRE>(XReg::x30, Reg::r29, 255), "ldrsb x30, [x29, #255]!");

  TEST_SINGLE(strh<IndexType::PRE>(Reg::r30, Reg::r29, -256), "strh w30, [x29, #-256]!");
  TEST_SINGLE(strh<IndexType::PRE>(Reg::r30, Reg::r29, 255), "strh w30, [x29, #255]!");
  TEST_SINGLE(ldrh<IndexType::PRE>(Reg::r30, Reg::r29, -256), "ldrh w30, [x29, #-256]!");
  TEST_SINGLE(ldrh<IndexType::PRE>(Reg::r30, Reg::r29, 255), "ldrh w30, [x29, #255]!");

  TEST_SINGLE(strh<IndexType::PRE>(VReg::v30, Reg::r29, -256), "str h30, [x29, #-256]!");
  TEST_SINGLE(strh<IndexType::PRE>(VReg::v30, Reg::r29, 255), "str h30, [x29, #255]!");
  TEST_SINGLE(ldrh<IndexType::PRE>(VReg::v30, Reg::r29, -256), "ldr h30, [x29, #-256]!");
  TEST_SINGLE(ldrh<IndexType::PRE>(VReg::v30, Reg::r29, 255), "ldr h30, [x29, #255]!");

  TEST_SINGLE(ldrsh<IndexType::PRE>(WReg::w30, Reg::r29, -256), "ldrsh w30, [x29, #-256]!");
  TEST_SINGLE(ldrsh<IndexType::PRE>(WReg::w30, Reg::r29, 255), "ldrsh w30, [x29, #255]!");
  TEST_SINGLE(ldrsh<IndexType::PRE>(XReg::x30, Reg::r29, -256), "ldrsh x30, [x29, #-256]!");
  TEST_SINGLE(ldrsh<IndexType::PRE>(XReg::x30, Reg::r29, 255), "ldrsh x30, [x29, #255]!");

  TEST_SINGLE(str<IndexType::PRE>(WReg::w30, Reg::r29, -256), "str w30, [x29, #-256]!");
  TEST_SINGLE(str<IndexType::PRE>(WReg::w30, Reg::r29, 255), "str w30, [x29, #255]!");
  TEST_SINGLE(ldr<IndexType::PRE>(WReg::w30, Reg::r29, -256), "ldr w30, [x29, #-256]!");
  TEST_SINGLE(ldr<IndexType::PRE>(WReg::w30, Reg::r29, 255), "ldr w30, [x29, #255]!");

  TEST_SINGLE(str<IndexType::PRE>(SReg::s30, Reg::r29, -256), "str s30, [x29, #-256]!");
  TEST_SINGLE(str<IndexType::PRE>(SReg::s30, Reg::r29, 255), "str s30, [x29, #255]!");
  TEST_SINGLE(ldr<IndexType::PRE>(SReg::s30, Reg::r29, -256), "ldr s30, [x29, #-256]!");
  TEST_SINGLE(ldr<IndexType::PRE>(SReg::s30, Reg::r29, 255), "ldr s30, [x29, #255]!");

  TEST_SINGLE(ldrsw<IndexType::PRE>(XReg::x30, Reg::r29, -256), "ldrsw x30, [x29, #-256]!");
  TEST_SINGLE(ldrsw<IndexType::PRE>(XReg::x30, Reg::r29, 255), "ldrsw x30, [x29, #255]!");

  TEST_SINGLE(str<IndexType::PRE>(XReg::x30, Reg::r29, -256), "str x30, [x29, #-256]!");
  TEST_SINGLE(str<IndexType::PRE>(XReg::x30, Reg::r29, 255), "str x30, [x29, #255]!");
  TEST_SINGLE(ldr<IndexType::PRE>(XReg::x30, Reg::r29, -256), "ldr x30, [x29, #-256]!");
  TEST_SINGLE(ldr<IndexType::PRE>(XReg::x30, Reg::r29, 255), "ldr x30, [x29, #255]!");

  TEST_SINGLE(str<IndexType::PRE>(DReg::d30, Reg::r29, -256), "str d30, [x29, #-256]!");
  TEST_SINGLE(str<IndexType::PRE>(DReg::d30, Reg::r29, 255), "str d30, [x29, #255]!");
  TEST_SINGLE(ldr<IndexType::PRE>(DReg::d30, Reg::r29, -256), "ldr d30, [x29, #-256]!");
  TEST_SINGLE(ldr<IndexType::PRE>(DReg::d30, Reg::r29, 255), "ldr d30, [x29, #255]!");

  TEST_SINGLE(str<IndexType::PRE>(QReg::q30, Reg::r29, -256), "str q30, [x29, #-256]!");
  TEST_SINGLE(str<IndexType::PRE>(QReg::q30, Reg::r29, 255), "str q30, [x29, #255]!");
  TEST_SINGLE(ldr<IndexType::PRE>(QReg::q30, Reg::r29, -256), "ldr q30, [x29, #-256]!");
  TEST_SINGLE(ldr<IndexType::PRE>(QReg::q30, Reg::r29, 255), "ldr q30, [x29, #255]!");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register unprivileged") {
  if (false) {
    // vixl can't disassemble this class of instructions.
    TEST_SINGLE(sttrb(Reg::r30, Reg::r29, -256), "sttrb w30, [x29, #-256]");
    TEST_SINGLE(sttrb(Reg::r30, Reg::r29, 255), "sttrb w30, [x29, #255]");

    TEST_SINGLE(ldtrb(Reg::r30, Reg::r29, -256), "ldtrb w30, [x29, #-256]");
    TEST_SINGLE(ldtrb(Reg::r30, Reg::r29, 255), "ldtrb w30, [x29, #255]");

    TEST_SINGLE(ldtrsb(WReg::w30, Reg::r29, -256), "ldtrsb w30, [x29, #-256]");
    TEST_SINGLE(ldtrsb(WReg::w30, Reg::r29, 255), "ldtrsb w30, [x29, #255]");
    TEST_SINGLE(ldtrsb(XReg::x30, Reg::r29, -256), "ldtrsb x30, [x29, #-256]");
    TEST_SINGLE(ldtrsb(XReg::x30, Reg::r29, 255), "ldtrsb x30, [x29, #255]");

    TEST_SINGLE(sttrh(Reg::r30, Reg::r29, -256), "sttrh w30, [x29, #-256]");
    TEST_SINGLE(sttrh(Reg::r30, Reg::r29, 255), "sttrh w30, [x29, #255]");

    TEST_SINGLE(ldtrh(Reg::r30, Reg::r29, -256), "ldtrh w30, [x29, #-256]");
    TEST_SINGLE(ldtrh(Reg::r30, Reg::r29, 255), "ldtrh w30, [x29, #255]");

    TEST_SINGLE(ldtrsh(WReg::w30, Reg::r29, -256), "ldtrsh w30, [x29, #-256]");
    TEST_SINGLE(ldtrsh(WReg::w30, Reg::r29, 255), "ldtrsh w30, [x29, #255]");
    TEST_SINGLE(ldtrsh(XReg::x30, Reg::r29, -256), "ldtrsh x30, [x29, #-256]");
    TEST_SINGLE(ldtrsh(XReg::x30, Reg::r29, 255), "ldtrsh x30, [x29, #255]");

    TEST_SINGLE(sttr(WReg::w30, Reg::r29, -256), "sttr w30, [x29, #-256]");
    TEST_SINGLE(sttr(WReg::w30, Reg::r29, 255), "sttr w30, [x29, #255]");

    TEST_SINGLE(ldtr(WReg::w30, Reg::r29, -256), "ldtr w30, [x29, #-256]");
    TEST_SINGLE(ldtr(WReg::w30, Reg::r29, 255), "ldtr w30, [x29, #255]");

    TEST_SINGLE(ldtrsw(XReg::x30, Reg::r29, -256), "ldtrsw x30, [x29, #-256]");
    TEST_SINGLE(ldtrsw(XReg::x30, Reg::r29, 255), "ldtrsw x30, [x29, #255]");

    TEST_SINGLE(sttr(XReg::x30, Reg::r29, -256), "sttr x30, [x29, #-256]");
    TEST_SINGLE(sttr(XReg::x30, Reg::r29, 255), "sttr x30, [x29, #255]");

    TEST_SINGLE(ldtr(XReg::x30, Reg::r29, -256), "ldtr x30, [x29, #-256]");
    TEST_SINGLE(ldtr(XReg::x30, Reg::r29, 255), "ldtr x30, [x29, #255]");
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Atomic memory operations") {
  TEST_SINGLE(stadd(SubRegSize::i8Bit, Reg::r30, Reg::r29), "staddb w30, [x29]");
  TEST_SINGLE(stadd(SubRegSize::i16Bit, Reg::r30, Reg::r29), "staddh w30, [x29]");
  TEST_SINGLE(stadd(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stadd w30, [x29]");
  TEST_SINGLE(stadd(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stadd x30, [x29]");

  TEST_SINGLE(staddl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "staddlb w30, [x29]");
  TEST_SINGLE(staddl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "staddlh w30, [x29]");
  TEST_SINGLE(staddl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "staddl w30, [x29]");
  TEST_SINGLE(staddl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "staddl x30, [x29]");

  TEST_SINGLE(stadda(SubRegSize::i8Bit, Reg::r30, Reg::r29), "staddab w30, [x29]");
  TEST_SINGLE(stadda(SubRegSize::i16Bit, Reg::r30, Reg::r29), "staddah w30, [x29]");
  TEST_SINGLE(stadda(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stadda w30, [x29]");
  TEST_SINGLE(stadda(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stadda x30, [x29]");

  TEST_SINGLE(staddal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "staddalb w30, [x29]");
  TEST_SINGLE(staddal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "staddalh w30, [x29]");
  TEST_SINGLE(staddal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "staddal w30, [x29]");
  TEST_SINGLE(staddal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "staddal x30, [x29]");

  TEST_SINGLE(stclr(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stclrb w30, [x29]");
  TEST_SINGLE(stclr(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stclrh w30, [x29]");
  TEST_SINGLE(stclr(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stclr w30, [x29]");
  TEST_SINGLE(stclr(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stclr x30, [x29]");

  TEST_SINGLE(stclrl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stclrlb w30, [x29]");
  TEST_SINGLE(stclrl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stclrlh w30, [x29]");
  TEST_SINGLE(stclrl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stclrl w30, [x29]");
  TEST_SINGLE(stclrl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stclrl x30, [x29]");

  TEST_SINGLE(stclra(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stclrab w30, [x29]");
  TEST_SINGLE(stclra(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stclrah w30, [x29]");
  TEST_SINGLE(stclra(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stclra w30, [x29]");
  TEST_SINGLE(stclra(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stclra x30, [x29]");

  TEST_SINGLE(stclral(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stclralb w30, [x29]");
  TEST_SINGLE(stclral(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stclralh w30, [x29]");
  TEST_SINGLE(stclral(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stclral w30, [x29]");
  TEST_SINGLE(stclral(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stclral x30, [x29]");

  TEST_SINGLE(stset(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsetb w30, [x29]");
  TEST_SINGLE(stset(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stseth w30, [x29]");
  TEST_SINGLE(stset(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stset w30, [x29]");
  TEST_SINGLE(stset(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stset x30, [x29]");

  TEST_SINGLE(stsetl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsetlb w30, [x29]");
  TEST_SINGLE(stsetl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsetlh w30, [x29]");
  TEST_SINGLE(stsetl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsetl w30, [x29]");
  TEST_SINGLE(stsetl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsetl x30, [x29]");

  TEST_SINGLE(stseta(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsetab w30, [x29]");
  TEST_SINGLE(stseta(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsetah w30, [x29]");
  TEST_SINGLE(stseta(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stseta w30, [x29]");
  TEST_SINGLE(stseta(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stseta x30, [x29]");

  TEST_SINGLE(stsetal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsetalb w30, [x29]");
  TEST_SINGLE(stsetal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsetalh w30, [x29]");
  TEST_SINGLE(stsetal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsetal w30, [x29]");
  TEST_SINGLE(stsetal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsetal x30, [x29]");

  TEST_SINGLE(steor(SubRegSize::i8Bit, Reg::r30, Reg::r29), "steorb w30, [x29]");
  TEST_SINGLE(steor(SubRegSize::i16Bit, Reg::r30, Reg::r29), "steorh w30, [x29]");
  TEST_SINGLE(steor(SubRegSize::i32Bit, Reg::r30, Reg::r29), "steor w30, [x29]");
  TEST_SINGLE(steor(SubRegSize::i64Bit, Reg::r30, Reg::r29), "steor x30, [x29]");

  TEST_SINGLE(steorl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "steorlb w30, [x29]");
  TEST_SINGLE(steorl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "steorlh w30, [x29]");
  TEST_SINGLE(steorl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "steorl w30, [x29]");
  TEST_SINGLE(steorl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "steorl x30, [x29]");

  TEST_SINGLE(steora(SubRegSize::i8Bit, Reg::r30, Reg::r29), "steorab w30, [x29]");
  TEST_SINGLE(steora(SubRegSize::i16Bit, Reg::r30, Reg::r29), "steorah w30, [x29]");
  TEST_SINGLE(steora(SubRegSize::i32Bit, Reg::r30, Reg::r29), "steora w30, [x29]");
  TEST_SINGLE(steora(SubRegSize::i64Bit, Reg::r30, Reg::r29), "steora x30, [x29]");

  TEST_SINGLE(steoral(SubRegSize::i8Bit, Reg::r30, Reg::r29), "steoralb w30, [x29]");
  TEST_SINGLE(steoral(SubRegSize::i16Bit, Reg::r30, Reg::r29), "steoralh w30, [x29]");
  TEST_SINGLE(steoral(SubRegSize::i32Bit, Reg::r30, Reg::r29), "steoral w30, [x29]");
  TEST_SINGLE(steoral(SubRegSize::i64Bit, Reg::r30, Reg::r29), "steoral x30, [x29]");

  TEST_SINGLE(stsmax(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsmaxb w30, [x29]");
  TEST_SINGLE(stsmax(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsmaxh w30, [x29]");
  TEST_SINGLE(stsmax(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmax w30, [x29]");
  TEST_SINGLE(stsmax(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmax x30, [x29]");

  TEST_SINGLE(stsmaxl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsmaxlb w30, [x29]");
  TEST_SINGLE(stsmaxl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsmaxlh w30, [x29]");
  TEST_SINGLE(stsmaxl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmaxl w30, [x29]");
  TEST_SINGLE(stsmaxl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmaxl x30, [x29]");

  TEST_SINGLE(stsmaxa(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsmaxab w30, [x29]");
  TEST_SINGLE(stsmaxa(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsmaxah w30, [x29]");
  TEST_SINGLE(stsmaxa(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmaxa w30, [x29]");
  TEST_SINGLE(stsmaxa(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmaxa x30, [x29]");

  TEST_SINGLE(stsmaxal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsmaxalb w30, [x29]");
  TEST_SINGLE(stsmaxal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsmaxalh w30, [x29]");
  TEST_SINGLE(stsmaxal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmaxal w30, [x29]");
  TEST_SINGLE(stsmaxal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmaxal x30, [x29]");

  TEST_SINGLE(stsmin(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsminb w30, [x29]");
  TEST_SINGLE(stsmin(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsminh w30, [x29]");
  TEST_SINGLE(stsmin(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmin w30, [x29]");
  TEST_SINGLE(stsmin(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmin x30, [x29]");

  TEST_SINGLE(stsminl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsminlb w30, [x29]");
  TEST_SINGLE(stsminl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsminlh w30, [x29]");
  TEST_SINGLE(stsminl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsminl w30, [x29]");
  TEST_SINGLE(stsminl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsminl x30, [x29]");

  TEST_SINGLE(stsmina(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsminab w30, [x29]");
  TEST_SINGLE(stsmina(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsminah w30, [x29]");
  TEST_SINGLE(stsmina(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsmina w30, [x29]");
  TEST_SINGLE(stsmina(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsmina x30, [x29]");

  TEST_SINGLE(stsminal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stsminalb w30, [x29]");
  TEST_SINGLE(stsminal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stsminalh w30, [x29]");
  TEST_SINGLE(stsminal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stsminal w30, [x29]");
  TEST_SINGLE(stsminal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stsminal x30, [x29]");

  TEST_SINGLE(stumax(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stumaxb w30, [x29]");
  TEST_SINGLE(stumax(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stumaxh w30, [x29]");
  TEST_SINGLE(stumax(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumax w30, [x29]");
  TEST_SINGLE(stumax(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumax x30, [x29]");

  TEST_SINGLE(stumaxl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stumaxlb w30, [x29]");
  TEST_SINGLE(stumaxl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stumaxlh w30, [x29]");
  TEST_SINGLE(stumaxl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumaxl w30, [x29]");
  TEST_SINGLE(stumaxl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumaxl x30, [x29]");

  TEST_SINGLE(stumaxa(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stumaxab w30, [x29]");
  TEST_SINGLE(stumaxa(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stumaxah w30, [x29]");
  TEST_SINGLE(stumaxa(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumaxa w30, [x29]");
  TEST_SINGLE(stumaxa(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumaxa x30, [x29]");

  TEST_SINGLE(stumaxal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stumaxalb w30, [x29]");
  TEST_SINGLE(stumaxal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stumaxalh w30, [x29]");
  TEST_SINGLE(stumaxal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumaxal w30, [x29]");
  TEST_SINGLE(stumaxal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumaxal x30, [x29]");

  TEST_SINGLE(stumin(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stuminb w30, [x29]");
  TEST_SINGLE(stumin(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stuminh w30, [x29]");
  TEST_SINGLE(stumin(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumin w30, [x29]");
  TEST_SINGLE(stumin(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumin x30, [x29]");

  TEST_SINGLE(stuminl(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stuminlb w30, [x29]");
  TEST_SINGLE(stuminl(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stuminlh w30, [x29]");
  TEST_SINGLE(stuminl(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stuminl w30, [x29]");
  TEST_SINGLE(stuminl(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stuminl x30, [x29]");

  TEST_SINGLE(stumina(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stuminab w30, [x29]");
  TEST_SINGLE(stumina(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stuminah w30, [x29]");
  TEST_SINGLE(stumina(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stumina w30, [x29]");
  TEST_SINGLE(stumina(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stumina x30, [x29]");

  TEST_SINGLE(stuminal(SubRegSize::i8Bit, Reg::r30, Reg::r29), "stuminalb w30, [x29]");
  TEST_SINGLE(stuminal(SubRegSize::i16Bit, Reg::r30, Reg::r29), "stuminalh w30, [x29]");
  TEST_SINGLE(stuminal(SubRegSize::i32Bit, Reg::r30, Reg::r29), "stuminal w30, [x29]");
  TEST_SINGLE(stuminal(SubRegSize::i64Bit, Reg::r30, Reg::r29), "stuminal x30, [x29]");

  TEST_SINGLE(ldswp(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "swpb w30, w28, [x29]");
  TEST_SINGLE(ldswp(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "swph w30, w28, [x29]");
  TEST_SINGLE(ldswp(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "swp w30, w28, [x29]");
  TEST_SINGLE(ldswp(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "swp x30, x28, [x29]");

  TEST_SINGLE(ldswpl(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "swplb w30, w28, [x29]");
  TEST_SINGLE(ldswpl(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "swplh w30, w28, [x29]");
  TEST_SINGLE(ldswpl(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "swpl w30, w28, [x29]");
  TEST_SINGLE(ldswpl(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "swpl x30, x28, [x29]");

  TEST_SINGLE(ldswpa(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "swpab w30, w28, [x29]");
  TEST_SINGLE(ldswpa(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "swpah w30, w28, [x29]");
  TEST_SINGLE(ldswpa(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "swpa w30, w28, [x29]");
  TEST_SINGLE(ldswpa(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "swpa x30, x28, [x29]");

  TEST_SINGLE(ldswpal(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "swpalb w30, w28, [x29]");
  TEST_SINGLE(ldswpal(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "swpalh w30, w28, [x29]");
  TEST_SINGLE(ldswpal(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "swpal w30, w28, [x29]");
  TEST_SINGLE(ldswpal(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "swpal x30, x28, [x29]");

  TEST_SINGLE(ldadd(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddb w30, w28, [x29]");
  TEST_SINGLE(ldadd(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddh w30, w28, [x29]");
  TEST_SINGLE(ldadd(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldadd w30, w28, [x29]");
  TEST_SINGLE(ldadd(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldadd x30, x28, [x29]");

  TEST_SINGLE(ldaddl(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddlb w30, w28, [x29]");
  TEST_SINGLE(ldaddl(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddlh w30, w28, [x29]");
  TEST_SINGLE(ldaddl(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddl w30, w28, [x29]");
  TEST_SINGLE(ldaddl(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddl x30, x28, [x29]");

  TEST_SINGLE(ldadda(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddab w30, w28, [x29]");
  TEST_SINGLE(ldadda(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddah w30, w28, [x29]");
  TEST_SINGLE(ldadda(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldadda w30, w28, [x29]");
  TEST_SINGLE(ldadda(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldadda x30, x28, [x29]");

  TEST_SINGLE(ldaddal(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddalb w30, w28, [x29]");
  TEST_SINGLE(ldaddal(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddalh w30, w28, [x29]");
  TEST_SINGLE(ldaddal(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddal w30, w28, [x29]");
  TEST_SINGLE(ldaddal(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldaddal x30, x28, [x29]");

  TEST_SINGLE(ldclr(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrb w30, w28, [x29]");
  TEST_SINGLE(ldclr(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrh w30, w28, [x29]");
  TEST_SINGLE(ldclr(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldclr w30, w28, [x29]");
  TEST_SINGLE(ldclr(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldclr x30, x28, [x29]");

  TEST_SINGLE(ldclrl(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrlb w30, w28, [x29]");
  TEST_SINGLE(ldclrl(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrlh w30, w28, [x29]");
  TEST_SINGLE(ldclrl(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrl w30, w28, [x29]");
  TEST_SINGLE(ldclrl(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrl x30, x28, [x29]");

  TEST_SINGLE(ldclra(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrab w30, w28, [x29]");
  TEST_SINGLE(ldclra(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldclrah w30, w28, [x29]");
  TEST_SINGLE(ldclra(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldclra w30, w28, [x29]");
  TEST_SINGLE(ldclra(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldclra x30, x28, [x29]");

  TEST_SINGLE(ldclral(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldclralb w30, w28, [x29]");
  TEST_SINGLE(ldclral(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldclralh w30, w28, [x29]");
  TEST_SINGLE(ldclral(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldclral w30, w28, [x29]");
  TEST_SINGLE(ldclral(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldclral x30, x28, [x29]");

  TEST_SINGLE(ldset(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetb w30, w28, [x29]");
  TEST_SINGLE(ldset(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldseth w30, w28, [x29]");
  TEST_SINGLE(ldset(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldset w30, w28, [x29]");
  TEST_SINGLE(ldset(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldset x30, x28, [x29]");

  TEST_SINGLE(ldsetl(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetlb w30, w28, [x29]");
  TEST_SINGLE(ldsetl(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetlh w30, w28, [x29]");
  TEST_SINGLE(ldsetl(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetl w30, w28, [x29]");
  TEST_SINGLE(ldsetl(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetl x30, x28, [x29]");

  TEST_SINGLE(ldseta(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetab w30, w28, [x29]");
  TEST_SINGLE(ldseta(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetah w30, w28, [x29]");
  TEST_SINGLE(ldseta(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldseta w30, w28, [x29]");
  TEST_SINGLE(ldseta(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldseta x30, x28, [x29]");

  TEST_SINGLE(ldsetal(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetalb w30, w28, [x29]");
  TEST_SINGLE(ldsetal(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetalh w30, w28, [x29]");
  TEST_SINGLE(ldsetal(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetal w30, w28, [x29]");
  TEST_SINGLE(ldsetal(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldsetal x30, x28, [x29]");

  TEST_SINGLE(ldeor(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorb w30, w28, [x29]");
  TEST_SINGLE(ldeor(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorh w30, w28, [x29]");
  TEST_SINGLE(ldeor(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldeor w30, w28, [x29]");
  TEST_SINGLE(ldeor(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldeor x30, x28, [x29]");

  TEST_SINGLE(ldeorl(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorlb w30, w28, [x29]");
  TEST_SINGLE(ldeorl(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorlh w30, w28, [x29]");
  TEST_SINGLE(ldeorl(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorl w30, w28, [x29]");
  TEST_SINGLE(ldeorl(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorl x30, x28, [x29]");

  TEST_SINGLE(ldeora(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorab w30, w28, [x29]");
  TEST_SINGLE(ldeora(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldeorah w30, w28, [x29]");
  TEST_SINGLE(ldeora(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldeora w30, w28, [x29]");
  TEST_SINGLE(ldeora(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldeora x30, x28, [x29]");

  TEST_SINGLE(ldeoral(SubRegSize::i8Bit, Reg::r30, Reg::r28, Reg::r29), "ldeoralb w30, w28, [x29]");
  TEST_SINGLE(ldeoral(SubRegSize::i16Bit, Reg::r30, Reg::r28, Reg::r29), "ldeoralh w30, w28, [x29]");
  TEST_SINGLE(ldeoral(SubRegSize::i32Bit, Reg::r30, Reg::r28, Reg::r29), "ldeoral w30, w28, [x29]");
  TEST_SINGLE(ldeoral(SubRegSize::i64Bit, Reg::r30, Reg::r28, Reg::r29), "ldeoral x30, x28, [x29]");

  TEST_SINGLE(ldaddb(Reg::r30, Reg::r28, Reg::r29), "ldaddb w30, w28, [x29]");
  TEST_SINGLE(ldclrb(Reg::r30, Reg::r28, Reg::r29), "ldclrb w30, w28, [x29]");
  TEST_SINGLE(ldeorb(Reg::r30, Reg::r28, Reg::r29), "ldeorb w30, w28, [x29]");
  TEST_SINGLE(ldsetb(Reg::r30, Reg::r28, Reg::r29), "ldsetb w30, w28, [x29]");
  TEST_SINGLE(ldsmaxb(Reg::r30, Reg::r28, Reg::r29), "ldsmaxb w30, w28, [x29]");
  TEST_SINGLE(ldsminb(Reg::r30, Reg::r28, Reg::r29), "ldsminb w30, w28, [x29]");
  TEST_SINGLE(ldumaxb(Reg::r30, Reg::r28, Reg::r29), "ldumaxb w30, w28, [x29]");
  TEST_SINGLE(lduminb(Reg::r30, Reg::r28, Reg::r29), "lduminb w30, w28, [x29]");
  TEST_SINGLE(ldswpb(Reg::r30, Reg::r28, Reg::r29), "swpb w30, w28, [x29]");
  TEST_SINGLE(ldaddlb(Reg::r30, Reg::r28, Reg::r29), "ldaddlb w30, w28, [x29]");
  TEST_SINGLE(ldclrlb(Reg::r30, Reg::r28, Reg::r29), "ldclrlb w30, w28, [x29]");
  TEST_SINGLE(ldeorlb(Reg::r30, Reg::r28, Reg::r29), "ldeorlb w30, w28, [x29]");
  TEST_SINGLE(ldsetlb(Reg::r30, Reg::r28, Reg::r29), "ldsetlb w30, w28, [x29]");
  TEST_SINGLE(ldsmaxlb(Reg::r30, Reg::r28, Reg::r29), "ldsmaxlb w30, w28, [x29]");
  TEST_SINGLE(ldsminlb(Reg::r30, Reg::r28, Reg::r29), "ldsminlb w30, w28, [x29]");
  TEST_SINGLE(ldumaxlb(Reg::r30, Reg::r28, Reg::r29), "ldumaxlb w30, w28, [x29]");
  TEST_SINGLE(lduminlb(Reg::r30, Reg::r28, Reg::r29), "lduminlb w30, w28, [x29]");
  TEST_SINGLE(ldswplb(Reg::r30, Reg::r28, Reg::r29), "swplb w30, w28, [x29]");
  TEST_SINGLE(ldaddab(Reg::r30, Reg::r28, Reg::r29), "ldaddab w30, w28, [x29]");
  TEST_SINGLE(ldclrab(Reg::r30, Reg::r28, Reg::r29), "ldclrab w30, w28, [x29]");
  TEST_SINGLE(ldeorab(Reg::r30, Reg::r28, Reg::r29), "ldeorab w30, w28, [x29]");
  TEST_SINGLE(ldsetab(Reg::r30, Reg::r28, Reg::r29), "ldsetab w30, w28, [x29]");
  TEST_SINGLE(ldsmaxab(Reg::r30, Reg::r28, Reg::r29), "ldsmaxab w30, w28, [x29]");
  TEST_SINGLE(ldsminab(Reg::r30, Reg::r28, Reg::r29), "ldsminab w30, w28, [x29]");
  TEST_SINGLE(ldumaxab(Reg::r30, Reg::r28, Reg::r29), "ldumaxab w30, w28, [x29]");
  TEST_SINGLE(lduminab(Reg::r30, Reg::r28, Reg::r29), "lduminab w30, w28, [x29]");
  TEST_SINGLE(ldswpab(Reg::r30, Reg::r28, Reg::r29), "swpab w30, w28, [x29]");
  TEST_SINGLE(ldaddalb(Reg::r30, Reg::r28, Reg::r29), "ldaddalb w30, w28, [x29]");
  TEST_SINGLE(ldclralb(Reg::r30, Reg::r28, Reg::r29), "ldclralb w30, w28, [x29]");
  TEST_SINGLE(ldeoralb(Reg::r30, Reg::r28, Reg::r29), "ldeoralb w30, w28, [x29]");
  TEST_SINGLE(ldsetalb(Reg::r30, Reg::r28, Reg::r29), "ldsetalb w30, w28, [x29]");
  TEST_SINGLE(ldsmaxalb(Reg::r30, Reg::r28, Reg::r29), "ldsmaxalb w30, w28, [x29]");
  TEST_SINGLE(ldsminalb(Reg::r30, Reg::r28, Reg::r29), "ldsminalb w30, w28, [x29]");
  TEST_SINGLE(ldumaxalb(Reg::r30, Reg::r28, Reg::r29), "ldumaxalb w30, w28, [x29]");
  TEST_SINGLE(lduminalb(Reg::r30, Reg::r28, Reg::r29), "lduminalb w30, w28, [x29]");
  TEST_SINGLE(ldswpalb(Reg::r30, Reg::r28, Reg::r29), "swpalb w30, w28, [x29]");

  TEST_SINGLE(ldaddh(Reg::r30, Reg::r28, Reg::r29), "ldaddh w30, w28, [x29]");
  TEST_SINGLE(ldclrh(Reg::r30, Reg::r28, Reg::r29), "ldclrh w30, w28, [x29]");
  TEST_SINGLE(ldeorh(Reg::r30, Reg::r28, Reg::r29), "ldeorh w30, w28, [x29]");
  TEST_SINGLE(ldseth(Reg::r30, Reg::r28, Reg::r29), "ldseth w30, w28, [x29]");
  TEST_SINGLE(ldsmaxh(Reg::r30, Reg::r28, Reg::r29), "ldsmaxh w30, w28, [x29]");
  TEST_SINGLE(ldsminh(Reg::r30, Reg::r28, Reg::r29), "ldsminh w30, w28, [x29]");
  TEST_SINGLE(ldumaxh(Reg::r30, Reg::r28, Reg::r29), "ldumaxh w30, w28, [x29]");
  TEST_SINGLE(lduminh(Reg::r30, Reg::r28, Reg::r29), "lduminh w30, w28, [x29]");
  TEST_SINGLE(ldswph(Reg::r30, Reg::r28, Reg::r29), "swph w30, w28, [x29]");
  TEST_SINGLE(ldaddlh(Reg::r30, Reg::r28, Reg::r29), "ldaddlh w30, w28, [x29]");
  TEST_SINGLE(ldclrlh(Reg::r30, Reg::r28, Reg::r29), "ldclrlh w30, w28, [x29]");
  TEST_SINGLE(ldeorlh(Reg::r30, Reg::r28, Reg::r29), "ldeorlh w30, w28, [x29]");
  TEST_SINGLE(ldsetlh(Reg::r30, Reg::r28, Reg::r29), "ldsetlh w30, w28, [x29]");
  TEST_SINGLE(ldsmaxlh(Reg::r30, Reg::r28, Reg::r29), "ldsmaxlh w30, w28, [x29]");
  TEST_SINGLE(ldsminlh(Reg::r30, Reg::r28, Reg::r29), "ldsminlh w30, w28, [x29]");
  TEST_SINGLE(ldumaxlh(Reg::r30, Reg::r28, Reg::r29), "ldumaxlh w30, w28, [x29]");
  TEST_SINGLE(lduminlh(Reg::r30, Reg::r28, Reg::r29), "lduminlh w30, w28, [x29]");
  TEST_SINGLE(ldswplh(Reg::r30, Reg::r28, Reg::r29), "swplh w30, w28, [x29]");
  TEST_SINGLE(ldaddah(Reg::r30, Reg::r28, Reg::r29), "ldaddah w30, w28, [x29]");
  TEST_SINGLE(ldclrah(Reg::r30, Reg::r28, Reg::r29), "ldclrah w30, w28, [x29]");
  TEST_SINGLE(ldeorah(Reg::r30, Reg::r28, Reg::r29), "ldeorah w30, w28, [x29]");
  TEST_SINGLE(ldsetah(Reg::r30, Reg::r28, Reg::r29), "ldsetah w30, w28, [x29]");
  TEST_SINGLE(ldsmaxah(Reg::r30, Reg::r28, Reg::r29), "ldsmaxah w30, w28, [x29]");
  TEST_SINGLE(ldsminah(Reg::r30, Reg::r28, Reg::r29), "ldsminah w30, w28, [x29]");
  TEST_SINGLE(ldumaxah(Reg::r30, Reg::r28, Reg::r29), "ldumaxah w30, w28, [x29]");
  TEST_SINGLE(lduminah(Reg::r30, Reg::r28, Reg::r29), "lduminah w30, w28, [x29]");
  TEST_SINGLE(ldswpah(Reg::r30, Reg::r28, Reg::r29), "swpah w30, w28, [x29]");
  TEST_SINGLE(ldaddalh(Reg::r30, Reg::r28, Reg::r29), "ldaddalh w30, w28, [x29]");
  TEST_SINGLE(ldclralh(Reg::r30, Reg::r28, Reg::r29), "ldclralh w30, w28, [x29]");
  TEST_SINGLE(ldeoralh(Reg::r30, Reg::r28, Reg::r29), "ldeoralh w30, w28, [x29]");
  TEST_SINGLE(ldsetalh(Reg::r30, Reg::r28, Reg::r29), "ldsetalh w30, w28, [x29]");
  TEST_SINGLE(ldsmaxalh(Reg::r30, Reg::r28, Reg::r29), "ldsmaxalh w30, w28, [x29]");
  TEST_SINGLE(ldsminalh(Reg::r30, Reg::r28, Reg::r29), "ldsminalh w30, w28, [x29]");
  TEST_SINGLE(ldumaxalh(Reg::r30, Reg::r28, Reg::r29), "ldumaxalh w30, w28, [x29]");
  TEST_SINGLE(lduminalh(Reg::r30, Reg::r28, Reg::r29), "lduminalh w30, w28, [x29]");
  TEST_SINGLE(ldswpalh(Reg::r30, Reg::r28, Reg::r29), "swpalh w30, w28, [x29]");

  TEST_SINGLE(ldadd(WReg::w30, WReg::w28, Reg::r29), "ldadd w30, w28, [x29]");
  TEST_SINGLE(ldclr(WReg::w30, WReg::w28, Reg::r29), "ldclr w30, w28, [x29]");
  TEST_SINGLE(ldeor(WReg::w30, WReg::w28, Reg::r29), "ldeor w30, w28, [x29]");
  TEST_SINGLE(ldset(WReg::w30, WReg::w28, Reg::r29), "ldset w30, w28, [x29]");
  TEST_SINGLE(ldsmax(WReg::w30, WReg::w28, Reg::r29), "ldsmax w30, w28, [x29]");
  TEST_SINGLE(ldsmin(WReg::w30, WReg::w28, Reg::r29), "ldsmin w30, w28, [x29]");
  TEST_SINGLE(ldumax(WReg::w30, WReg::w28, Reg::r29), "ldumax w30, w28, [x29]");
  TEST_SINGLE(ldumin(WReg::w30, WReg::w28, Reg::r29), "ldumin w30, w28, [x29]");
  TEST_SINGLE(ldswp(WReg::w30, WReg::w28, Reg::r29), "swp w30, w28, [x29]");
  TEST_SINGLE(ldaddl(WReg::w30, WReg::w28, Reg::r29), "ldaddl w30, w28, [x29]");
  TEST_SINGLE(ldclrl(WReg::w30, WReg::w28, Reg::r29), "ldclrl w30, w28, [x29]");
  TEST_SINGLE(ldeorl(WReg::w30, WReg::w28, Reg::r29), "ldeorl w30, w28, [x29]");
  TEST_SINGLE(ldsetl(WReg::w30, WReg::w28, Reg::r29), "ldsetl w30, w28, [x29]");
  TEST_SINGLE(ldsmaxl(WReg::w30, WReg::w28, Reg::r29), "ldsmaxl w30, w28, [x29]");
  TEST_SINGLE(ldsminl(WReg::w30, WReg::w28, Reg::r29), "ldsminl w30, w28, [x29]");
  TEST_SINGLE(ldumaxl(WReg::w30, WReg::w28, Reg::r29), "ldumaxl w30, w28, [x29]");
  TEST_SINGLE(lduminl(WReg::w30, WReg::w28, Reg::r29), "lduminl w30, w28, [x29]");
  TEST_SINGLE(ldswpl(WReg::w30, WReg::w28, Reg::r29), "swpl w30, w28, [x29]");
  TEST_SINGLE(ldadda(WReg::w30, WReg::w28, Reg::r29), "ldadda w30, w28, [x29]");
  TEST_SINGLE(ldclra(WReg::w30, WReg::w28, Reg::r29), "ldclra w30, w28, [x29]");
  TEST_SINGLE(ldeora(WReg::w30, WReg::w28, Reg::r29), "ldeora w30, w28, [x29]");
  TEST_SINGLE(ldseta(WReg::w30, WReg::w28, Reg::r29), "ldseta w30, w28, [x29]");
  TEST_SINGLE(ldsmaxa(WReg::w30, WReg::w28, Reg::r29), "ldsmaxa w30, w28, [x29]");
  TEST_SINGLE(ldsmina(WReg::w30, WReg::w28, Reg::r29), "ldsmina w30, w28, [x29]");
  TEST_SINGLE(ldumaxa(WReg::w30, WReg::w28, Reg::r29), "ldumaxa w30, w28, [x29]");
  TEST_SINGLE(ldumina(WReg::w30, WReg::w28, Reg::r29), "ldumina w30, w28, [x29]");
  TEST_SINGLE(ldswpa(WReg::w30, WReg::w28, Reg::r29), "swpa w30, w28, [x29]");
  TEST_SINGLE(ldaddal(WReg::w30, WReg::w28, Reg::r29), "ldaddal w30, w28, [x29]");
  TEST_SINGLE(ldclral(WReg::w30, WReg::w28, Reg::r29), "ldclral w30, w28, [x29]");
  TEST_SINGLE(ldeoral(WReg::w30, WReg::w28, Reg::r29), "ldeoral w30, w28, [x29]");
  TEST_SINGLE(ldsetal(WReg::w30, WReg::w28, Reg::r29), "ldsetal w30, w28, [x29]");
  TEST_SINGLE(ldsmaxal(WReg::w30, WReg::w28, Reg::r29), "ldsmaxal w30, w28, [x29]");
  TEST_SINGLE(ldsminal(WReg::w30, WReg::w28, Reg::r29), "ldsminal w30, w28, [x29]");
  TEST_SINGLE(ldumaxal(WReg::w30, WReg::w28, Reg::r29), "ldumaxal w30, w28, [x29]");
  TEST_SINGLE(lduminal(WReg::w30, WReg::w28, Reg::r29), "lduminal w30, w28, [x29]");
  TEST_SINGLE(ldswpal(WReg::w30, WReg::w28, Reg::r29), "swpal w30, w28, [x29]");

  TEST_SINGLE(ldadd(XReg::x30, XReg::x28, Reg::r29), "ldadd x30, x28, [x29]");
  TEST_SINGLE(ldclr(XReg::x30, XReg::x28, Reg::r29), "ldclr x30, x28, [x29]");
  TEST_SINGLE(ldeor(XReg::x30, XReg::x28, Reg::r29), "ldeor x30, x28, [x29]");
  TEST_SINGLE(ldset(XReg::x30, XReg::x28, Reg::r29), "ldset x30, x28, [x29]");
  TEST_SINGLE(ldsmax(XReg::x30, XReg::x28, Reg::r29), "ldsmax x30, x28, [x29]");
  TEST_SINGLE(ldsmin(XReg::x30, XReg::x28, Reg::r29), "ldsmin x30, x28, [x29]");
  TEST_SINGLE(ldumax(XReg::x30, XReg::x28, Reg::r29), "ldumax x30, x28, [x29]");
  TEST_SINGLE(ldumin(XReg::x30, XReg::x28, Reg::r29), "ldumin x30, x28, [x29]");
  TEST_SINGLE(ldswp(XReg::x30, XReg::x28, Reg::r29), "swp x30, x28, [x29]");
  TEST_SINGLE(ldaddl(XReg::x30, XReg::x28, Reg::r29), "ldaddl x30, x28, [x29]");
  TEST_SINGLE(ldclrl(XReg::x30, XReg::x28, Reg::r29), "ldclrl x30, x28, [x29]");
  TEST_SINGLE(ldeorl(XReg::x30, XReg::x28, Reg::r29), "ldeorl x30, x28, [x29]");
  TEST_SINGLE(ldsetl(XReg::x30, XReg::x28, Reg::r29), "ldsetl x30, x28, [x29]");
  TEST_SINGLE(ldsmaxl(XReg::x30, XReg::x28, Reg::r29), "ldsmaxl x30, x28, [x29]");
  TEST_SINGLE(ldsminl(XReg::x30, XReg::x28, Reg::r29), "ldsminl x30, x28, [x29]");
  TEST_SINGLE(ldumaxl(XReg::x30, XReg::x28, Reg::r29), "ldumaxl x30, x28, [x29]");
  TEST_SINGLE(lduminl(XReg::x30, XReg::x28, Reg::r29), "lduminl x30, x28, [x29]");
  TEST_SINGLE(ldswpl(XReg::x30, XReg::x28, Reg::r29), "swpl x30, x28, [x29]");
  TEST_SINGLE(ldadda(XReg::x30, XReg::x28, Reg::r29), "ldadda x30, x28, [x29]");
  TEST_SINGLE(ldclra(XReg::x30, XReg::x28, Reg::r29), "ldclra x30, x28, [x29]");
  TEST_SINGLE(ldeora(XReg::x30, XReg::x28, Reg::r29), "ldeora x30, x28, [x29]");
  TEST_SINGLE(ldseta(XReg::x30, XReg::x28, Reg::r29), "ldseta x30, x28, [x29]");
  TEST_SINGLE(ldsmaxa(XReg::x30, XReg::x28, Reg::r29), "ldsmaxa x30, x28, [x29]");
  TEST_SINGLE(ldsmina(XReg::x30, XReg::x28, Reg::r29), "ldsmina x30, x28, [x29]");
  TEST_SINGLE(ldumaxa(XReg::x30, XReg::x28, Reg::r29), "ldumaxa x30, x28, [x29]");
  TEST_SINGLE(ldumina(XReg::x30, XReg::x28, Reg::r29), "ldumina x30, x28, [x29]");
  TEST_SINGLE(ldswpa(XReg::x30, XReg::x28, Reg::r29), "swpa x30, x28, [x29]");
  TEST_SINGLE(ldaddal(XReg::x30, XReg::x28, Reg::r29), "ldaddal x30, x28, [x29]");
  TEST_SINGLE(ldclral(XReg::x30, XReg::x28, Reg::r29), "ldclral x30, x28, [x29]");
  TEST_SINGLE(ldeoral(XReg::x30, XReg::x28, Reg::r29), "ldeoral x30, x28, [x29]");
  TEST_SINGLE(ldsetal(XReg::x30, XReg::x28, Reg::r29), "ldsetal x30, x28, [x29]");
  TEST_SINGLE(ldsmaxal(XReg::x30, XReg::x28, Reg::r29), "ldsmaxal x30, x28, [x29]");
  TEST_SINGLE(ldsminal(XReg::x30, XReg::x28, Reg::r29), "ldsminal x30, x28, [x29]");
  TEST_SINGLE(ldumaxal(XReg::x30, XReg::x28, Reg::r29), "ldumaxal x30, x28, [x29]");
  TEST_SINGLE(lduminal(XReg::x30, XReg::x28, Reg::r29), "lduminal x30, x28, [x29]");
  TEST_SINGLE(ldswpal(XReg::x30, XReg::x28, Reg::r29), "swpal x30, x28, [x29]");

  TEST_SINGLE(ldaprb(WReg::w30, Reg::r29), "ldaprb w30, [x29]");
  TEST_SINGLE(ldaprh(WReg::w30, Reg::r29), "ldaprh w30, [x29]");
  TEST_SINGLE(ldapr(WReg::w30, Reg::r29), "ldapr w30, [x29]");
  TEST_SINGLE(ldapr(XReg::x30, Reg::r29), "ldapr x30, [x29]");

  if (false) {
    // vixl can't disassemble this class of instructions.
    TEST_SINGLE(st64bv0(XReg::x30, XReg::x28, Reg::r29), "st64bv0 x30, x28, [x29]");
    TEST_SINGLE(st64bv(XReg::x30, XReg::x28, Reg::r29), "st64bv x30, x28, [x29]");
    TEST_SINGLE(st64b(XReg::x30, Reg::r29), "st64bv x30, [x29]");
    TEST_SINGLE(ld64b(XReg::x30, Reg::r29), "ld64b x30, [x29]");
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore register-register offset") {
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, false), "strb w30, [x28, x29]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, true), "strb w30, [x28, x29, lsl #0]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, false), "strb w30, [x28, w29, uxtw]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, true), "strb w30, [x28, w29, uxtw #0]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, false), "strb w30, [x28, w29, sxtw]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, true), "strb w30, [x28, w29, sxtw #0]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, false), "strb w30, [x28, x29, sxtx]");
  TEST_SINGLE(strb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, true), "strb w30, [x28, x29, sxtx #0]");

  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, false), "ldrb w30, [x28, x29]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, true), "ldrb w30, [x28, x29, lsl #0]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, false), "ldrb w30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, true), "ldrb w30, [x28, w29, uxtw #0]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, false), "ldrb w30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, true), "ldrb w30, [x28, w29, sxtw #0]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, false), "ldrb w30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, true), "ldrb w30, [x28, x29, sxtx #0]");

  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, false), "ldrsb w30, [x28, x29]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, true), "ldrsb w30, [x28, x29, lsl #0]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, false), "ldrsb w30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, true), "ldrsb w30, [x28, w29, uxtw #0]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, false), "ldrsb w30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, true), "ldrsb w30, [x28, w29, sxtw #0]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, false), "ldrsb w30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, true), "ldrsb w30, [x28, x29, sxtx #0]");

  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, false), "ldrsb x30, [x28, x29]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, true), "ldrsb x30, [x28, x29, lsl #0]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, false), "ldrsb x30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, true), "ldrsb x30, [x28, w29, uxtw #0]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, false), "ldrsb x30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, true), "ldrsb x30, [x28, w29, sxtw #0]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, false), "ldrsb x30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, true), "ldrsb x30, [x28, x29, sxtx #0]");

  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "strh w30, [x28, x29]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "strh w30, [x28, x29, lsl #1]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "strh w30, [x28, w29, uxtw]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "strh w30, [x28, w29, uxtw #1]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "strh w30, [x28, w29, sxtw]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "strh w30, [x28, w29, sxtw #1]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "strh w30, [x28, x29, sxtx]");
  TEST_SINGLE(strh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "strh w30, [x28, x29, sxtx #1]");

  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldrh w30, [x28, x29]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "ldrh w30, [x28, x29, lsl #1]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldrh w30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "ldrh w30, [x28, w29, uxtw #1]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldrh w30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "ldrh w30, [x28, w29, sxtw #1]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldrh w30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "ldrh w30, [x28, x29, sxtx #1]");

  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldrsh w30, [x28, x29]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "ldrsh w30, [x28, x29, lsl #1]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldrsh w30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "ldrsh w30, [x28, w29, uxtw #1]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldrsh w30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "ldrsh w30, [x28, w29, sxtw #1]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldrsh w30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "ldrsh w30, [x28, x29, sxtx #1]");

  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldrsh x30, [x28, x29]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "ldrsh x30, [x28, x29, lsl #1]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldrsh x30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "ldrsh x30, [x28, w29, uxtw #1]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldrsh x30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "ldrsh x30, [x28, w29, sxtw #1]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldrsh x30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "ldrsh x30, [x28, x29, sxtx #1]");

  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str w30, [x28, x29]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 2), "str w30, [x28, x29, lsl #2]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str w30, [x28, w29, uxtw]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 2), "str w30, [x28, w29, uxtw #2]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str w30, [x28, w29, sxtw]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 2), "str w30, [x28, w29, sxtw #2]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str w30, [x28, x29, sxtx]");
  TEST_SINGLE(str(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 2), "str w30, [x28, x29, sxtx #2]");

  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr w30, [x28, x29]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 2), "ldr w30, [x28, x29, lsl #2]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr w30, [x28, w29, uxtw]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::UXTW, 2), "ldr w30, [x28, w29, uxtw #2]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr w30, [x28, w29, sxtw]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTW, 2), "ldr w30, [x28, w29, sxtw #2]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr w30, [x28, x29, sxtx]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r28, Reg::r29, ExtendedType::SXTX, 2), "ldr w30, [x28, x29, sxtx #2]");

  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldrsw x30, [x28, x29]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 2), "ldrsw x30, [x28, x29, lsl #2]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldrsw x30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 2), "ldrsw x30, [x28, w29, uxtw #2]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldrsw x30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 2), "ldrsw x30, [x28, w29, sxtw #2]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldrsw x30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 2), "ldrsw x30, [x28, x29, sxtx #2]");

  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str x30, [x28, x29]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 3), "str x30, [x28, x29, lsl #3]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str x30, [x28, w29, uxtw]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 3), "str x30, [x28, w29, uxtw #3]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str x30, [x28, w29, sxtw]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 3), "str x30, [x28, w29, sxtw #3]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str x30, [x28, x29, sxtx]");
  TEST_SINGLE(str(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 3), "str x30, [x28, x29, sxtx #3]");

  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr x30, [x28, x29]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 3), "ldr x30, [x28, x29, lsl #3]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr x30, [x28, w29, uxtw]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::UXTW, 3), "ldr x30, [x28, w29, uxtw #3]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr x30, [x28, w29, sxtw]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTW, 3), "ldr x30, [x28, w29, sxtw #3]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr x30, [x28, x29, sxtx]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r28, Reg::r29, ExtendedType::SXTX, 3), "ldr x30, [x28, x29, sxtx #3]");

  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "prfm pldl1keep, [x28, x29]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::LSL_64, 3), "prfm pldl1keep, [x28, x29, lsl #3]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "prfm pldl1keep, [x28, w29, uxtw]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::UXTW, 3), "prfm pldl1keep, [x28, w29, uxtw #3]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "prfm pldl1keep, [x28, w29, sxtw]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::SXTW, 3), "prfm pldl1keep, [x28, w29, sxtw #3]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "prfm pldl1keep, [x28, x29, sxtx]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r28, Reg::r29, ExtendedType::SXTX, 3), "prfm pldl1keep, [x28, x29, sxtx #3]");

  TEST_SINGLE(strb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64), "str b30, [x28, x29]");
  TEST_SINGLE(strb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW), "str b30, [x28, w29, uxtw]");
  TEST_SINGLE(strb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW), "str b30, [x28, w29, sxtw]");
  TEST_SINGLE(strb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX), "str b30, [x28, x29, sxtx]");

  TEST_SINGLE(ldrb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64), "ldr b30, [x28, x29]");
  TEST_SINGLE(ldrb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW), "ldr b30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW), "ldr b30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrb(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX), "ldr b30, [x28, x29, sxtx]");

  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str h30, [x28, x29]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "str h30, [x28, x29, lsl #1]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str h30, [x28, w29, uxtw]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "str h30, [x28, w29, uxtw #1]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str h30, [x28, w29, sxtw]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "str h30, [x28, w29, sxtw #1]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str h30, [x28, x29, sxtx]");
  TEST_SINGLE(strh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "str h30, [x28, x29, sxtx #1]");

  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr h30, [x28, x29]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 1), "ldr h30, [x28, x29, lsl #1]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr h30, [x28, w29, uxtw]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::UXTW, 1), "ldr h30, [x28, w29, uxtw #1]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr h30, [x28, w29, sxtw]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTW, 1), "ldr h30, [x28, w29, sxtw #1]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr h30, [x28, x29, sxtx]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r28, Reg::r29, ExtendedType::SXTX, 1), "ldr h30, [x28, x29, sxtx #1]");

  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str s30, [x28, x29]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 2), "str s30, [x28, x29, lsl #2]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str s30, [x28, w29, uxtw]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::UXTW, 2), "str s30, [x28, w29, uxtw #2]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str s30, [x28, w29, sxtw]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTW, 2), "str s30, [x28, w29, sxtw #2]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str s30, [x28, x29, sxtx]");
  TEST_SINGLE(str(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTX, 2), "str s30, [x28, x29, sxtx #2]");

  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr s30, [x28, x29]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 2), "ldr s30, [x28, x29, lsl #2]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr s30, [x28, w29, uxtw]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::UXTW, 2), "ldr s30, [x28, w29, uxtw #2]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr s30, [x28, w29, sxtw]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTW, 2), "ldr s30, [x28, w29, sxtw #2]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr s30, [x28, x29, sxtx]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r28, Reg::r29, ExtendedType::SXTX, 2), "ldr s30, [x28, x29, sxtx #2]");

  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str d30, [x28, x29]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 3), "str d30, [x28, x29, lsl #3]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str d30, [x28, w29, uxtw]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::UXTW, 3), "str d30, [x28, w29, uxtw #3]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str d30, [x28, w29, sxtw]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTW, 3), "str d30, [x28, w29, sxtw #3]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str d30, [x28, x29, sxtx]");
  TEST_SINGLE(str(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTX, 3), "str d30, [x28, x29, sxtx #3]");

  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr d30, [x28, x29]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 3), "ldr d30, [x28, x29, lsl #3]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr d30, [x28, w29, uxtw]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::UXTW, 3), "ldr d30, [x28, w29, uxtw #3]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr d30, [x28, w29, sxtw]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTW, 3), "ldr d30, [x28, w29, sxtw #3]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr d30, [x28, x29, sxtx]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r28, Reg::r29, ExtendedType::SXTX, 3), "ldr d30, [x28, x29, sxtx #3]");

  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "str q30, [x28, x29]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 4), "str q30, [x28, x29, lsl #4]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "str q30, [x28, w29, uxtw]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::UXTW, 4), "str q30, [x28, w29, uxtw #4]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "str q30, [x28, w29, sxtw]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTW, 4), "str q30, [x28, w29, sxtw #4]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "str q30, [x28, x29, sxtx]");
  TEST_SINGLE(str(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTX, 4), "str q30, [x28, x29, sxtx #4]");

  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 0), "ldr q30, [x28, x29]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::LSL_64, 4), "ldr q30, [x28, x29, lsl #4]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::UXTW, 0), "ldr q30, [x28, w29, uxtw]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::UXTW, 4), "ldr q30, [x28, w29, uxtw #4]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTW, 0), "ldr q30, [x28, w29, sxtw]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTW, 4), "ldr q30, [x28, w29, sxtw #4]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTX, 0), "ldr q30, [x28, x29, sxtx]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r28, Reg::r29, ExtendedType::SXTX, 4), "ldr q30, [x28, x29, sxtx #4]");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore PAC") {
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 0), "ldraa x30, [x29]");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, -4096), "ldraa x30, [x29, #-4096]");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 512), "ldraa x30, [x29, #512]");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 4088), "ldraa x30, [x29, #4088]");

  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 0), "ldraa x30, [x29]!");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, -4096), "ldraa x30, [x29, #-4096]!");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 512), "ldraa x30, [x29, #512]!");
  TEST_SINGLE(ldraa(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 4088), "ldraa x30, [x29, #4088]!");

  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 0), "ldrab x30, [x29]");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, -4096), "ldrab x30, [x29, #-4096]");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 512), "ldrab x30, [x29, #512]");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::OFFSET, 4088), "ldrab x30, [x29, #4088]");

  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 0), "ldrab x30, [x29]!");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, -4096), "ldrab x30, [x29, #-4096]!");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 512), "ldrab x30, [x29, #512]!");
  TEST_SINGLE(ldrab(XReg::x30, XReg::x29, ARMEmitter::IndexType::PRE, 4088), "ldrab x30, [x29, #4088]!");
}

TEST_CASE_METHOD(TestDisassembler, "Emitter: Loadstore: Loadstore unsigned immediate") {
  TEST_SINGLE(strb(Reg::r30, Reg::r29, 0), "strb w30, [x29]");
  TEST_SINGLE(strb(Reg::r30, Reg::r29, 4095), "strb w30, [x29, #4095]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r29, 0), "ldrb w30, [x29]");
  TEST_SINGLE(ldrb(Reg::r30, Reg::r29, 4095), "ldrb w30, [x29, #4095]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r29, 0), "ldrsb w30, [x29]");
  TEST_SINGLE(ldrsb(WReg::w30, Reg::r29, 4095), "ldrsb w30, [x29, #4095]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r29, 0), "ldrsb x30, [x29]");
  TEST_SINGLE(ldrsb(XReg::x30, Reg::r29, 4095), "ldrsb x30, [x29, #4095]");
  TEST_SINGLE(ldrb(VReg::v30, Reg::r29, 0), "ldr b30, [x29]");
  TEST_SINGLE(ldrb(VReg::v30, Reg::r29, 4095), "ldr b30, [x29, #4095]");
  TEST_SINGLE(strb(VReg::v30, Reg::r29, 0), "str b30, [x29]");
  TEST_SINGLE(strb(VReg::v30, Reg::r29, 4095), "str b30, [x29, #4095]");

  TEST_SINGLE(strh(Reg::r30, Reg::r29, 0), "strh w30, [x29]");
  TEST_SINGLE(strh(Reg::r30, Reg::r29, 8190), "strh w30, [x29, #8190]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r29, 0), "ldrh w30, [x29]");
  TEST_SINGLE(ldrh(Reg::r30, Reg::r29, 8190), "ldrh w30, [x29, #8190]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r29, 0), "ldrsh w30, [x29]");
  TEST_SINGLE(ldrsh(WReg::w30, Reg::r29, 8190), "ldrsh w30, [x29, #8190]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r29, 0), "ldrsh x30, [x29]");
  TEST_SINGLE(ldrsh(XReg::x30, Reg::r29, 8190), "ldrsh x30, [x29, #8190]");

  TEST_SINGLE(ldrh(VReg::v30, Reg::r29, 0), "ldr h30, [x29]");
  TEST_SINGLE(ldrh(VReg::v30, Reg::r29, 8190), "ldr h30, [x29, #8190]");
  TEST_SINGLE(strh(VReg::v30, Reg::r29, 0), "str h30, [x29]");
  TEST_SINGLE(strh(VReg::v30, Reg::r29, 8190), "str h30, [x29, #8190]");

  TEST_SINGLE(str(WReg::w30, Reg::r29, 0), "str w30, [x29]");
  TEST_SINGLE(str(WReg::w30, Reg::r29, 16380), "str w30, [x29, #16380]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r29, 0), "ldr w30, [x29]");
  TEST_SINGLE(ldr(WReg::w30, Reg::r29, 16380), "ldr w30, [x29, #16380]");

  TEST_SINGLE(ldrsw(XReg::x30, Reg::r29, 0), "ldrsw x30, [x29]");
  TEST_SINGLE(ldrsw(XReg::x30, Reg::r29, 16380), "ldrsw x30, [x29, #16380]");

  TEST_SINGLE(ldr(SReg::s30, Reg::r29, 0), "ldr s30, [x29]");
  TEST_SINGLE(ldr(SReg::s30, Reg::r29, 16380), "ldr s30, [x29, #16380]");
  TEST_SINGLE(str(SReg::s30, Reg::r29, 0), "str s30, [x29]");
  TEST_SINGLE(str(SReg::s30, Reg::r29, 16380), "str s30, [x29, #16380]");

  TEST_SINGLE(str(XReg::x30, Reg::r29, 0), "str x30, [x29]");
  TEST_SINGLE(str(XReg::x30, Reg::r29, 32760), "str x30, [x29, #32760]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r29, 0), "ldr x30, [x29]");
  TEST_SINGLE(ldr(XReg::x30, Reg::r29, 32760), "ldr x30, [x29, #32760]");

  TEST_SINGLE(ldr(SubRegSize::i8Bit, Reg::r30, Reg::r29, 0), "ldrb w30, [x29]");
  TEST_SINGLE(ldr(SubRegSize::i8Bit, Reg::r30, Reg::r29, 4095), "ldrb w30, [x29, #4095]");
  TEST_SINGLE(ldr(SubRegSize::i16Bit, Reg::r30, Reg::r29, 0), "ldrh w30, [x29]");
  TEST_SINGLE(ldr(SubRegSize::i16Bit, Reg::r30, Reg::r29, 8190), "ldrh w30, [x29, #8190]");
  TEST_SINGLE(ldr(SubRegSize::i32Bit, Reg::r30, Reg::r29, 0), "ldr w30, [x29]");
  TEST_SINGLE(ldr(SubRegSize::i32Bit, Reg::r30, Reg::r29, 16380), "ldr w30, [x29, #16380]");
  TEST_SINGLE(ldr(SubRegSize::i64Bit, Reg::r30, Reg::r29, 0), "ldr x30, [x29]");
  TEST_SINGLE(ldr(SubRegSize::i64Bit, Reg::r30, Reg::r29, 32760), "ldr x30, [x29, #32760]");

  TEST_SINGLE(str(SubRegSize::i8Bit, Reg::r30, Reg::r29, 0), "strb w30, [x29]");
  TEST_SINGLE(str(SubRegSize::i8Bit, Reg::r30, Reg::r29, 4095), "strb w30, [x29, #4095]");
  TEST_SINGLE(str(SubRegSize::i16Bit, Reg::r30, Reg::r29, 0), "strh w30, [x29]");
  TEST_SINGLE(str(SubRegSize::i16Bit, Reg::r30, Reg::r29, 8190), "strh w30, [x29, #8190]");
  TEST_SINGLE(str(SubRegSize::i32Bit, Reg::r30, Reg::r29, 0), "str w30, [x29]");
  TEST_SINGLE(str(SubRegSize::i32Bit, Reg::r30, Reg::r29, 16380), "str w30, [x29, #16380]");
  TEST_SINGLE(str(SubRegSize::i64Bit, Reg::r30, Reg::r29, 0), "str x30, [x29]");
  TEST_SINGLE(str(SubRegSize::i64Bit, Reg::r30, Reg::r29, 32760), "str x30, [x29, #32760]");

  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r29, 0), "prfm pldl1keep, [x29]");
  TEST_SINGLE(prfm(Prefetch::PLDL1KEEP, Reg::r29, 32760), "prfm pldl1keep, [x29, #32760]");

  TEST_SINGLE(ldr(DReg::d30, Reg::r29, 0), "ldr d30, [x29]");
  TEST_SINGLE(ldr(DReg::d30, Reg::r29, 32760), "ldr d30, [x29, #32760]");
  TEST_SINGLE(str(DReg::d30, Reg::r29, 0), "str d30, [x29]");
  TEST_SINGLE(str(DReg::d30, Reg::r29, 32760), "str d30, [x29, #32760]");

  TEST_SINGLE(ldr(QReg::q30, Reg::r29, 0), "ldr q30, [x29]");
  TEST_SINGLE(ldr(QReg::q30, Reg::r29, 65520), "ldr q30, [x29, #65520]");
  TEST_SINGLE(str(QReg::q30, Reg::r29, 0), "str q30, [x29]");
  TEST_SINGLE(str(QReg::q30, Reg::r29, 65520), "str q30, [x29, #65520]");
}
