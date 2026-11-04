include(ExternalProject)
ExternalProject_Add(FEXMingwTests
  PREFIX FEXMingwTests
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXMingwTests_64"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DLLVM_MINGW=${LLVM_MINGW}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_64_MINGW_TOOLCHAIN_FILE}"
  "-DBITNESS=64"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

ExternalProject_Add(FEXMingwTests_32
  PREFIX FEXMingwTests_32
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXMingwTests_32"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DLLVM_MINGW=${LLVM_MINGW}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_32_MINGW_TOOLCHAIN_FILE}"
  "-DBITNESS=32"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)
file(GLOB_RECURSE TESTS_64_ONLY CONFIGURE_DEPENDS tests/*.64.cpp)
file(GLOB_RECURSE TESTS_32_ONLY CONFIGURE_DEPENDS tests/*.32.cpp)

# Apply bitness-specific exclude lists
list(REMOVE_ITEM TESTS ${TESTS_64_ONLY})
list(REMOVE_ITEM TESTS ${TESTS_32_ONLY})

find_program(WINE_PATH wine)

function(AddTests Tests BinDirectory Bitness)
  foreach(TEST ${Tests})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    set(BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BinDirectory}/${TEST_NAME}.${Bitness}.exe")
    set(TEST_CASE "${TEST_NAME}.${Bitness}")

    # Add jit test case
    add_test(NAME "${TEST_CASE}.mingw.tst"
      COMMAND ${PYTHON_TEST_RUNNER} "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
      "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
      "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
      "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
      "${TEST_CASE}"
      "host"
      "${WINE_PATH}"
      "${BIN_PATH}")
  endforeach()
endfunction()

# Only add the tests if a wine binary is found.
if (NOT WINE_PATH-NOTFOUND)
  # Execute combined 32-bit and 64-bit tests.
  AddTests("${TESTS}" "FEXMingwTests_64" 64)
  AddTests("${TESTS}" "FEXMingwTests_32" 32)
  # Execute tests that are only 64-bit.
  AddTests("${TESTS_64_ONLY}" "FEXMingwTests_64" 64)
  # Execute tests that are only 32-bit.
  AddTests("${TESTS_32_ONLY}" "FEXMingwTests_32" 32)
endif()

add_custom_target(fex_mingw_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "30" ${TEST_JOB_FLAG} "-R" "\.*\.mingw\.tst$$"
  DEPENDS FEXMingwTests FEXMingwTests_32)
