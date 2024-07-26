
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-gcc-target-tests-bins/32/*)

foreach(TEST ${TESTS})

  string(REPLACE "/fex-gcc-target-tests-bins/32/" ";" TEST_NAME_LIST ${TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  add_test(NAME "${TEST_NAME}.jit.gcc-target-32"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
    "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
    "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
    "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
    "${TEST_NAME}"
    "guest"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "-o" "stderr" "--no-silent" "-c" "irjit" "-n" "500" "--"
    "${TEST}")
  set_property(TEST "${TEST_NAME}.jit.gcc-target-32" APPEND PROPERTY SKIP_RETURN_CODE 125)
endforeach()

add_custom_target(
  gcc_target_tests_32
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "20" ${TEST_JOB_FLAG} "-R" "\.*.gcc-target-32$$")
