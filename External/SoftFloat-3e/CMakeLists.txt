
set (SRCS
  src/extF80_add.c
  src/extF80_div.c
  src/extF80_sub.c
  src/extF80_mul.c
  src/extF80_rem.c
  src/extF80_sqrt.c
  src/extF80_to_i32.c
  src/extF80_to_i64.c
  src/extF80_to_ui64.c
  src/extF80_to_f32.c
  src/extF80_to_f64.c
  src/i32_to_extF80.c
  src/ui64_to_extF80.c
  src/extF80_to_f128.c
  src/f128_to_extF80.c
  src/s_roundToUI64.c
  src/s_f128UIToCommonNaN.c
  src/s_commonNaNToF128UI.c
  src/s_normSubnormalF128Sig.c
  src/s_roundToI32.c
  src/s_roundToI64.c
  src/s_roundPackToF32.c
  src/s_addMagsExtF80.c
  src/s_extF80UIToCommonNaN.c
  src/s_commonNaNToF32UI.c
  src/s_commonNaNToF64UI.c
  src/s_roundPackToF64.c
  src/s_propagateNaNExtF80UI.c
  src/s_roundPackToExtF80.c
  src/s_normSubnormalExtF80Sig.c
  src/s_subMagsExtF80.c
  src/s_shiftRightJam128.c
  src/s_shiftRightJam128Extra.c
  src/s_normRoundPackToExtF80.c
  src/s_approxRecip_1Ks.c
  src/s_approxRecipSqrt32_1.c
  src/s_approxRecipSqrt_1Ks.c
  src/softfloat_raiseFlags.c
  src/f64_to_extF80.c
  src/s_commonNaNToExtF80UI.c
  src/s_normSubnormalF64Sig.c
  src/s_f64UIToCommonNaN.c
  src/extF80_roundToInt.c
  src/extF80_eq.c
  src/extF80_lt.c
  src/f32_to_extF80.c
  src/s_normSubnormalF32Sig.c
  src/s_f32UIToCommonNaN.c)

if (_M_ARM_64 AND HAS_CLANG_PRESERVE_ALL)
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=__attribute__((preserve_all));-DFEXCORE_HAS_PRESERVE_ALL_ATTR=1")
else()
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=;-DFEXCORE_HAS_PRESERVE_ALL_ATTR=0")
endif()

list(APPEND DEFINES "-DSOFTFLOAT_BUILTIN_CLZ=1;-DINLINE=static inline;-DINLINE_LEVEL=4;-DSOFTFLOAT_FAST_INT64=1;-DSOFTFLOAT_FAST_DIV32TO16=1;-DSOFTFLOAT_FAST_DIV64TO32=1")

add_library(softfloat_3e STATIC ${SRCS})
target_include_directories(softfloat_3e PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_include_directories(softfloat_3e PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftFloat-3e/)
target_compile_definitions(softfloat_3e PUBLIC ${DEFINES})
