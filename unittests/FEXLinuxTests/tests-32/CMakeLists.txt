cmake_minimum_required(VERSION 3.14)
project(FEXLinuxTests_32)

set(CMAKE_CXX_STANDARD 17)

unset (CMAKE_C_FLAGS)
unset (CMAKE_CXX_FLAGS)

set(GENERATE_GUEST_INSTALL_TARGETS TRUE)

file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)

foreach(TEST ${TESTS})
  get_filename_component(TEST_NAME ${TEST} NAME_WLE)
  add_executable(${TEST_NAME}.32 ${TEST})
  target_compile_options(${TEST_NAME}.32 PRIVATE -m32)
  target_link_options(${TEST_NAME}.32 PRIVATE -m32)
endforeach()
