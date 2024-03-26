include(ExternalProject)
ExternalProject_Add(FEXLinuxTests
  PREFIX FEXLinuxTests
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests_64"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_64_TOOLCHAIN_FILE}"
  "-DBITNESS=64"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON
  )

ExternalProject_Add(FEXLinuxTests_32
  PREFIX FEXLinuxTests_32
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests_32"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_32_TOOLCHAIN_FILE}"
  "-DBITNESS=32"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON
  )

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)
file(GLOB_RECURSE TESTS_64_ONLY CONFIGURE_DEPENDS tests/*.64.cpp)
file(GLOB_RECURSE TESTS_32_ONLY CONFIGURE_DEPENDS tests/*.32.cpp)

# Apply bitness-specific exclude lists
list(REMOVE_ITEM TESTS ${TESTS_64_ONLY})
list(REMOVE_ITEM TESTS ${TESTS_32_ONLY})

function(AddTests Tests BinDirectory Bitness)
  foreach(TEST ${Tests})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    set(BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BinDirectory}/${TEST_NAME}.${Bitness}")
    set(TEST_CASE "${TEST_NAME}.${Bitness}")

    set(THUNK_ARGS "")
    if(TEST_NAME STREQUAL "thunk_testlib")
      # Test thunking only if thunks are enabled and supported
      if(NOT BUILD_THUNKS OR ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT)
        continue()
      endif()

      set(THUNK_ARGS "-k" "${CMAKE_SOURCE_DIR}/CI/FEXLinuxTestsThunks.json")
    endif()

    # Add jit test case
    add_test(NAME "${TEST_CASE}.jit.flt"
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
      "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
      "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
      "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
      "${TEST_CASE}"
      "guest"
      "$<TARGET_FILE:FEXLoader>"
      ${THUNK_ARGS}
      "-o" "stderr" "--no-silent" "-c" "irjit" "-n" "500" "--"
      "${BIN_PATH}")

    if (_M_X86_64 AND NOT TEST_NAME STREQUAL "thunk_testlib")
      # Add host test case
      add_test(NAME "${TEST_CASE}.host.flt"
        COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures_Host"
        "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
        "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests_Host"
        "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
        "${TEST_CASE}"
        "host"
        "${BIN_PATH}")
    endif()
  endforeach()
endfunction()

# Execute combined 32-bit and 64-bit tests.
AddTests("${TESTS}" "FEXLinuxTests_64" 64)
AddTests("${TESTS}" "FEXLinuxTests_32" 32)
# Execute tests that are only 64-bit.
AddTests("${TESTS_64_ONLY}" "FEXLinuxTests_64" 64)
# Execute tests that are only 32-bit.
AddTests("${TESTS_32_ONLY}" "FEXLinuxTests_32" 32)

if(TEST thunk_testlib.64.jit.flt)
  # Ensure libfex_thunk_test is found even when using an uncommon install prefix
  set_property(TEST "thunk_testlib.32.jit.flt" PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib")
  set_property(TEST "thunk_testlib.64.jit.flt" PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib")
endif()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

# Only emulated
add_custom_target(
  fex_linux_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "30" "-j${CORES}" "-R" "\.*\.jit\.flt$$"
  DEPENDS FEXLinuxTests FEXLinuxTests_32 FEXLoader
  )

# Only host
add_custom_target(
  fex_linux_tests_host
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "30" "-j${CORES}" "-R" "\.*\.host\.flt$$"
  DEPENDS FEXLinuxTests FEXLinuxTests_32
  )

# Both host and emulated
add_custom_target(
  fex_linux_tests_all
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "30" "-j${CORES}" "-R" "\.*\.flt$$"
  DEPENDS FEXLinuxTests FEXLinuxTests_32 FEXLoader
  )
