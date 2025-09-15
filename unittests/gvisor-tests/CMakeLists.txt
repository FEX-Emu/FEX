
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-gvisor-tests-bins/*_test)

foreach(TEST ${TESTS})

  string(REPLACE "/fex-gvisor-tests-bins/" ";" TEST_NAME_LIST ${TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  add_test(NAME "${TEST_NAME}.jit.gvisor"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
    "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
    "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
    "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
    "${TEST_NAME}"
    "guest"
    "$<TARGET_FILE:FEX>"
    "${TEST}")
  set_property(TEST "${TEST_NAME}.jit.gvisor" APPEND PROPERTY SKIP_RETURN_CODE 125)
  set_property(TEST "${TEST_NAME}.jit.gvisor" APPEND PROPERTY ENVIRONMENT "FEX_OUTPUTLOG=stderr;FEX_SILENTLOG=0;FEX_MAXINST=500")
endforeach()

set(RM_DIR_COMMAND "rm $ENV{FEX_ROOTFS}/tmp 2> /dev/null || true")

add_custom_target(
  gvisor_tests
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "sh" "-c" "${RM_DIR_COMMAND}"
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*.gvisor$$")
