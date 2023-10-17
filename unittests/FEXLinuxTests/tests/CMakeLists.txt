cmake_minimum_required(VERSION 3.14)
project(FEXLinuxTests)

set(CMAKE_CXX_STANDARD 17)

unset(CMAKE_C_FLAGS)
unset(CMAKE_CXX_FLAGS)

set(GENERATE_GUEST_INSTALL_TARGETS TRUE)

file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)
if(BITNESS EQUAL 64)
  file(GLOB_RECURSE TESTS_32_ONLY CONFIGURE_DEPENDS *.32.cpp)
  list(REMOVE_ITEM TESTS ${TESTS_32_ONLY})
else()
  file(GLOB_RECURSE TESTS_64_ONLY CONFIGURE_DEPENDS *.64.cpp)
  list(REMOVE_ITEM TESTS ${TESTS_64_ONLY})
endif()

option(CATCH_BUILD_STATIC_LIBRARY "" ON)
set(CATCH_BUILD_STATIC_LIBRARY ON)
add_subdirectory(../../../External/Catch2/ Catch2)

foreach(TEST ${TESTS})
  get_filename_component(TEST_NAME ${TEST} NAME_WE)

  add_executable(${TEST_NAME}.${BITNESS} ${TEST})
  target_link_libraries(${TEST_NAME}.${BITNESS} PRIVATE Catch2::Catch2WithMain)
endforeach()

target_link_libraries(pthread_cancel.${BITNESS} PRIVATE pthread)

target_link_options(smc-1-dynamic.${BITNESS} PRIVATE -z execstack)

target_link_libraries(smc-mt-1.${BITNESS} PRIVATE pthread)

target_link_libraries(smc-mt-2.${BITNESS} PRIVATE pthread)

target_link_libraries(smc-shared-1.${BITNESS} PRIVATE rt pthread)

target_link_libraries(smc-shared-2.${BITNESS} PRIVATE rt pthread)

target_link_libraries(thunk_testlib.${BITNESS} PRIVATE ${CMAKE_DL_LIBS})

target_link_libraries(timer-sigev-thread.${BITNESS} PRIVATE rt pthread)
