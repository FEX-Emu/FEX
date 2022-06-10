
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE POSIX_TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-posixtest-bins/conformance/*.test)

foreach(POSIX_TEST ${POSIX_TESTS})

  string(REPLACE "/fex-posixtest-bins/" ";" TEST_NAME_LIST ${POSIX_TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  if (ENABLE_INTERPRETER)
    add_test(NAME "${TEST_NAME}.int.posix"
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
      "${CMAKE_SOURCE_DIR}/unittests/POSIX/Known_Failures"
      "${CMAKE_SOURCE_DIR}/unittests/POSIX/Expected_Output"
      "${CMAKE_SOURCE_DIR}/unittests/POSIX/Disabled_Tests"
      "${TEST_NAME}"
      "guest"
      "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
      "--no-silent" "-c" "irint" "-n" "500" "--"
      "${POSIX_TEST}")
  endif()

  add_test(NAME "${TEST_NAME}.jit.posix"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Known_Failures"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Expected_Output"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Disabled_Tests"
    "${TEST_NAME}"
    "guest"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "--no-silent" "-c" "irjit" "-n" "500" "--"
    "${POSIX_TEST}")

endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  posix_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "\.*.posix")
