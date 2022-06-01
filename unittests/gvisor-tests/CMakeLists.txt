
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-gvisor-tests-bins/*_test)

foreach(TEST ${TESTS})

  string(REPLACE "/fex-gvisor-tests-bins/" ";" TEST_NAME_LIST ${TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  # Interpreter is too slow to run these tests, only generate for jit

  add_test(NAME "${TEST_NAME}.jit.gvisor"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
    "${CMAKE_SOURCE_DIR}/unittests/gvisor-tests/Known_Failures"
    "${CMAKE_SOURCE_DIR}/unittests/gvisor-tests/Expected_Output"
    "${CMAKE_SOURCE_DIR}/unittests/gvisor-tests/Disabled_Tests"
    "${TEST_NAME}"
    "guest"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "--no-silent" "-c" "irjit" "-n" "500" "--"
    "${TEST}")

endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)
set(RM_DIR_COMMAND "rm $ENV{ROOTFS}/tmp 2> /dev/null || true")

add_custom_target(
  gvisor_tests
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "sh" "-c" "${RM_DIR_COMMAND}"
  COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "\.*.gvisor$$")
