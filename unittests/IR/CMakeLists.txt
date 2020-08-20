
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE IR_SOURCES CONFIGURE_DEPENDS *.ir)

set(IR_DEPENDS "")
foreach(IR_SRC ${IR_SOURCES})
  get_filename_component(IR_NAME ${IR_SRC} NAME)

  # Generate a temporary file
  set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${IR_TMP}")

  set(OUTPUT_CONFIG_NAME "${IR_NAME}.config.bin")

  add_custom_command(OUTPUT ${OUTPUT_CONFIG_NAME}
    DEPENDS "${IR_SRC}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Scripts/json_asm_config_parse.py"
    COMMAND "python3" ARGS "${CMAKE_SOURCE_DIR}/Scripts/json_ir_config_parse.py" "${IR_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_NAME}")

  list(APPEND IR_DEPENDS "${OUTPUT_CONFIG_NAME}")

  set(TEST_ARGS
    "-c irint -n 1"      "ir_int_1"
    "-c irint -n 500"    "ir_int_500"
    "-c irint -n 500 -m" "ir_int_500_m"
    "-c irjit -n 1"      "ir_jit_1"
    "-c irjit -n 500"    "ir_jit_500"
    "-c irjit -n 500 -m" "ir_jit_500_m"
    )

  list(LENGTH TEST_ARGS ARG_COUNT)
  math(EXPR ARG_COUNT "${ARG_COUNT}-1")
  foreach(Index RANGE 0 ${ARG_COUNT} 2)
    math(EXPR TEST_NAME_INDEX "${Index}+1")

    list(GET TEST_ARGS ${Index} ARGS)
    list(GET TEST_ARGS ${TEST_NAME_INDEX} TEST_DESC)

    set(TEST_NAME "${TEST_DESC}/Test_${IR_NAME}")
    string(REPLACE " " ";" ARGS_LIST ${ARGS})

    set(TEST_NAME "${TEST_DESC}/Test_${IR_NAME}")
    string(REPLACE " " ";" ARGS_LIST ${ARGS})
    add_test(NAME ${TEST_NAME}
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/testharness_runner.py"
      "${CMAKE_SOURCE_DIR}/unittests/IR/Known_Failures"
      "${CMAKE_SOURCE_DIR}/unittests/IR/Disabled_Tests"
      "Test_${IR_NAME}"
      "${CMAKE_BINARY_DIR}/Bin/IRLoader"
      ${ARGS_LIST} "${IR_SRC}" "${OUTPUT_CONFIG_NAME}")
    # This will cause the IR tests to fail if it can't find the TestHarness or IRN files
    # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
    # Just make sure to independently run `make all` then `make test`
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/IRLoader")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${IRC_SRC}")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")

  endforeach()
endforeach()

add_custom_target(ir_files ALL
  DEPENDS "${IR_DEPENDS}")

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  ir_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "-j${CORES}" "-R" "\.*.ir")
