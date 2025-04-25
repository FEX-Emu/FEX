set(SRCS_128BIT
  src/128bit/Impl.cpp
  src/128bit/atanll.c
  src/128bit/constll.c
  src/128bit/exp2ll.c
  src/128bit/floorll.c
  src/128bit/log2ll.c
  src/128bit/mtherr.c
  src/128bit/polevll.c
  src/128bit/sinll.c
  src/128bit/tanll.c)

# 128-bit library
add_library(cephes_128bit STATIC ${SRCS_128BIT})
target_link_libraries(cephes_128bit softfloat_3e)
target_include_directories(cephes_128bit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_compile_options(cephes_128bit PRIVATE -fno-builtin)
