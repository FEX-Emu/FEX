
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-gcc-target-tests-bins/32/*)

foreach(TEST ${TESTS})
  # Extract test file name
  string(REPLACE "/fex-gcc-target-tests-bins/32/" ";" TEST_NAME_LIST ${TEST})
  list(GET TEST_NAME_LIST 1 TEST_FILE_NAME)

  # Loop over block sizes
  foreach(BLOCK_SIZE 500 1)
    # Create TEST_NAME from TEST_FILE_NAME
    string(REPLACE "/" "-" TEST_NAME ${TEST_FILE_NAME})
    string(APPEND TEST_NAME ".n${BLOCK_SIZE}.gcc-target-32")

    add_test(NAME "${TEST_NAME}"
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
      "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
      "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
      "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
      "${TEST_NAME}"
      "guest"
      "$<TARGET_FILE:FEX>"
      "${TEST}")
    set_property(TEST "${TEST_NAME}" APPEND PROPERTY SKIP_RETURN_CODE 125)
    set_property(TEST "${TEST_NAME}" APPEND PROPERTY ENVIRONMENT "FEX_OUTPUTLOG=stderr;FEX_SILENTLOG=0;FEX_MAXINST=${BLOCK_SIZE}")
  endforeach()
endforeach()

add_custom_target(
  gcc_target_tests_32
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "40" ${TEST_JOB_FLAG} "-R" "\.*.gcc-target-32$$")
