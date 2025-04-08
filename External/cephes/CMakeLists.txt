set (SRCS
  src/Impl.cpp
  )

if (_M_X86_64 OR MINGW_BUILD)
  set (LOW_PRECISION TRUE)
endif()

list(APPEND SRCS
  src/atanll.c
  src/constll.c
  src/exp2ll.c
  src/floorll.c
  src/log2ll.c
  src/mtherr.c
  src/polevll.c
  src/sinll.c
  src/tanll.c)

add_library(cephes_128bit STATIC ${SRCS})
target_link_libraries(cephes_128bit softfloat_3e)
target_include_directories(cephes_128bit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
if (NOT LOW_PRECISION)
  target_compile_options(cephes_128bit PRIVATE -fno-builtin)
  target_compile_definitions(cephes_128bit PRIVATE -DLOW_PRECISION=0)
else()
  target_compile_definitions(cephes_128bit PRIVATE -DLOW_PRECISION=1)
endif()
