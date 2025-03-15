set (SRCS
  # Data symbols
  src/math/signgam.c

  # Doubles
  src/math/__cos.c
  src/math/__math_divzero.c
  src/math/__math_invalid.c
  src/math/__math_oflow.c
  src/math/__math_uflow.c
  src/math/__math_xflow.c
  src/math/__rem_pio2.c
  src/math/__rem_pio2_large.c
  src/math/__sin.c
  src/math/__tan.c
  src/math/atan.c
  src/math/atan2.c
  src/math/cos.c
  src/math/exp2.c
  src/math/exp_data.c
  src/math/fabs.c
  src/math/fmod.c
  src/math/floor.c
  src/math/log2.c
  src/math/log2_data.c
  src/math/remainder.c
  src/math/remquo.c
  src/math/scalbn.c
  src/math/sin.c
  src/math/tan.c

  # Long doubles
  src/math/__cosl.c
  src/math/__fpclassifyl.c
  src/math/__invtrigl.c
  src/math/__polevll.c
  src/math/__rem_pio2l.c
  src/math/__rem_pio2_large.c
  src/math/__sinl.c
  src/math/__tanl.c
  src/math/atanl.c
  src/math/atan2l.c
  src/math/cosl.c
  src/math/exp2l.c
  src/math/fabsl.c
  src/math/frexpl.c
  src/math/ldexpl.c
  # Disabled because it only operates at 64-bit precision in musl v1.2.5. Enable once fixed!
  # src/math/log2l.c
  src/math/scalbnl.c
  src/math/sinl.c
  src/math/tanl.c
)

add_library(musl_static STATIC ${SRCS})

target_compile_options(musl_static
  PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/include/features.h)

target_link_options(musl_static
  PRIVATE
  -ffreestanding
  -nostdlib
  -nostdinc
  -nostdinc++)

target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include/)
target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pre_gen/)
target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/internal/)
target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/arch/generic/)
target_include_directories(musl_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public_include)

if (_M_X86_64)
  target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/)
else()
  target_include_directories(musl_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/arch/aarch64/)
endif()
