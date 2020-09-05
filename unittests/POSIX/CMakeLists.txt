
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE POSIX_TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/External/fex-posixtest-bins/conformance/*.test)
 
foreach(POSIX_TEST ${POSIX_TESTS})

  string(REPLACE "/fex-posixtest-bins/" ";" TEST_NAME_LIST ${POSIX_TEST})
  list(GET TEST_NAME_LIST 1 TEST_NAME)
  string(REPLACE "/" "-" TEST_NAME ${TEST_NAME})

  add_test(NAME "${TEST_NAME}.int.posix"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/posix_test_runner.py"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Known_Failures"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Expected_Output"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Disabled_Tests"
    "${TEST_NAME}"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "-U" "-c" "irint" "-n" "500" "-R" $ENV{ROOTFS} "--"
    "${POSIX_TEST}")
  
  add_test(NAME "${TEST_NAME}.jit.posix"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/posix_test_runner.py"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Known_Failures"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Expected_Output"
    "${CMAKE_SOURCE_DIR}/unittests/POSIX/Disabled_Tests"
    "${TEST_NAME}"
    "${CMAKE_BINARY_DIR}/Bin/FEXLoader"
    "-U" "-c" "irjit" "-n" "500" "-R" $ENV{ROOTFS} "--"
    "${POSIX_TEST}")


  # This will cause the ASM tests to fail if it can't find the TestHarness or ASMN files
  # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
  # Just make sure to independently run `make all` then `make test`
  #set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner")
  #set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_NAME}")
  #set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")

endforeach()

#add_custom_target(asm_files ALL
#  DEPENDS "${ASM_DEPENDS}")

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  posix_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "-j${CORES}" "-VV" "-R" "\.*.posix")
