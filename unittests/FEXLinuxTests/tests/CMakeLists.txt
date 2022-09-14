cmake_minimum_required(VERSION 3.14)
project(FEXLinuxTests)

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

  add_executable(${TEST_NAME}.64 ${TEST})
endforeach()

target_link_libraries(pthread_cancel.32 PRIVATE pthread)
target_link_libraries(pthread_cancel.64 PRIVATE pthread)

target_link_options(smc-1-dynamic.32 PRIVATE -z execstack)
target_link_options(smc-1-dynamic.64 PRIVATE -z execstack)

target_link_libraries(smc-mt-1.32 PRIVATE pthread)
target_link_libraries(smc-mt-1.64 PRIVATE pthread)

target_link_libraries(smc-mt-2.32 PRIVATE pthread)
target_link_libraries(smc-mt-2.64 PRIVATE pthread)

target_link_libraries(smc-shared-1.32 PRIVATE rt pthread)
target_link_libraries(smc-shared-1.64 PRIVATE rt pthread)

target_link_libraries(smc-shared-2.32 PRIVATE rt pthread)
target_link_libraries(smc-shared-2.64 PRIVATE rt pthread)

target_link_libraries(timer-sigev-thread.32 PRIVATE rt pthread)
target_link_libraries(timer-sigev-thread.64 PRIVATE rt pthread)
