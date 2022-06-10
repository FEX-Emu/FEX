
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-gcc-target-tests-bins/64/*)

foreach(TEST ${TESTS})

  string(REPLACE "/fex-gcc-target-tests-bins/64/" ";" TEST_NAME_LIST ${TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  # Interpreter is too slow to run these tests, only generate for jit

  add_test(NAME "${TEST_NAME}.jit.gcc-target-64"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
    "${CMAKE_SOURCE_DIR}/unittests/gcc-target-tests-64/Known_Failures"
    "${CMAKE_SOURCE_DIR}/unittests/gcc-target-tests-64/Expected_Output"
    "${CMAKE_SOURCE_DIR}/unittests/gcc-target-tests-64/Disabled_Tests"
    "${TEST_NAME}"
    "guest"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "--no-silent" "-c" "irjit" "-n" "500" "--"
    "${TEST}")

endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  gcc_target_tests_64
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "20" "-j${CORES}" "-R" "\.*.gcc-target-64$$")
