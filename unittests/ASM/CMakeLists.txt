
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS *.asm)

set(ASM_DEPENDS "")
foreach(ASM_SRC ${ASM_SOURCES})
  get_filename_component(ASM_NAME ${ASM_SRC} NAME)

  # Generate a temporary file
 set(ASM_TMP "${ASM_NAME}_TMP.asm")
 set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ASM_TMP}")
  add_custom_command(OUTPUT ${TMP_FILE}
    DEPENDS "${ASM_SRC}"
    COMMAND "cp" ARGS "${ASM_SRC}" "${TMP_FILE}"
    COMMAND "sed" ARGS "-i" "-e" "\'1s;^;BITS 64\\n;\'" "-e" "\'\$\$a\\ret\\n\'" "${TMP_FILE}"
    )

  set(OUTPUT_NAME "${ASM_NAME}.bin")
  set(OUTPUT_CONFIG_NAME "${ASM_NAME}.config.bin")

  add_custom_command(OUTPUT ${OUTPUT_NAME}
    DEPENDS "${TMP_FILE}"
    COMMAND "nasm" ARGS "${TMP_FILE}" "-o" "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")

  add_custom_command(OUTPUT ${OUTPUT_CONFIG_NAME}
    DEPENDS "${ASM_SRC}"
    COMMAND "python3" ARGS "${CMAKE_SOURCE_DIR}/Scripts/json_asm_parse.py" "${ASM_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_NAME}")

  list(APPEND ASM_DEPENDS "${OUTPUT_NAME};${OUTPUT_CONFIG_NAME}")

  set(TEST_ARGS
    "-c irint -n 1"
    "-c irint -n 500"
    "-c irint -n 500 -m"
    "-c irjit -n 1"
    "-c irjit -n 500"
    "-c irjit -n 500 -m"
    "-c llvm -n 1"
    "-c llvm -n 500"
    "-c llvm -n 500 -m")

  set(TEST_NUM 0)
  foreach(ARGS ${TEST_ARGS})
    set(TEST_NAME "${TEST_NUM}/Test_${ASM_NAME}")
    string(REPLACE " " ";" ARGS_LIST ${ARGS})
    add_test(NAME ${TEST_NAME}
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/testharness_runner.py"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Known_Failures"
      "Test_${ASM_NAME}"
      "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner"
      ${ARGS_LIST} "${OUTPUT_NAME}" "${OUTPUT_CONFIG_NAME}")
    # This will cause the ASM tests to fail if it can't find the TestHarness or ASMN files
    # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
    # Just make sure to independently run `make all` then `make test`
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_NAME}")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")

    math(EXPR TEST_NUM "${TEST_NUM} + 1")
  endforeach()

endforeach()

add_custom_target(asm_files ALL
  DEPENDS "${ASM_DEPENDS}")
