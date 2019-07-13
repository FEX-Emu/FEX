
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB ASM_SOURCES CONFIGURE_DEPENDS *.asm)

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

  set(TEST_NAME "Test_${ASM_NAME}")
  add_test(NAME ${TEST_NAME}
    COMMAND "${CMAKE_BINARY_DIR}/Bin/TestHarness" "${OUTPUT_NAME}" "${OUTPUT_CONFIG_NAME}")
  # This will cause the ASM tests to fail if it can't find the TestHarness or ASMN files
  # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
  # Just make sure to independently run `make all` then `make test`
  set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarness")
  set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_NAME}")
  set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")

endforeach()

add_custom_target(asm_files ALL
  DEPENDS "${ASM_DEPENDS}")
