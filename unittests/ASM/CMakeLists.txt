enable_language(ASM_NASM)
if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
  error("Failed to find NASM compatible assembler!")
endif()

# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS *.asm)

set(ASM_DEPENDS "")

execute_process(COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/ClassifyCPU.py"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE CPU_CLASS)

foreach(ASM_SRC ${ASM_SOURCES})
  file(RELATIVE_PATH REL_ASM ${CMAKE_SOURCE_DIR} ${ASM_SRC})
  file(RELATIVE_PATH REL_TEST_ASM ${CMAKE_CURRENT_SOURCE_DIR} ${ASM_SRC})
  get_filename_component(ASM_NAME ${ASM_SRC} NAME)
  get_filename_component(ASM_DIR "${REL_ASM}" DIRECTORY)
  set(OUTPUT_ASM_FOLDER "${CMAKE_BINARY_DIR}/${ASM_DIR}")

  # Generate build directory
  file(MAKE_DIRECTORY "${OUTPUT_ASM_FOLDER}")

  # Generate a temporary file
  set(ASM_TMP "${ASM_NAME}_TMP.asm")
  set(TMP_FILE "${OUTPUT_ASM_FOLDER}/${ASM_TMP}")

  add_custom_command(OUTPUT ${TMP_FILE}
    DEPENDS "${ASM_SRC}"
    COMMAND "cp" ARGS "${ASM_SRC}" "${TMP_FILE}"
    COMMAND "sed" ARGS "-i" "-e" "\'1s;^;BITS 64\\n;\'" "-e" "\'\$\$a\\ret\\n\'" "${TMP_FILE}")

  set(OUTPUT_NAME "${OUTPUT_ASM_FOLDER}/${ASM_NAME}.bin")
  set(OUTPUT_CONFIG_NAME "${OUTPUT_ASM_FOLDER}/${ASM_NAME}.config.bin")

  add_custom_command(OUTPUT ${OUTPUT_NAME}
    DEPENDS "${TMP_FILE}"
    COMMAND "nasm" ARGS "-i" "${CMAKE_SOURCE_DIR}/unittests/ASM/Includes/" "${TMP_FILE}" "-o" "${OUTPUT_NAME}")

  add_custom_command(OUTPUT ${OUTPUT_CONFIG_NAME}
    DEPENDS "${ASM_SRC}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Scripts/json_asm_config_parse.py"
    DEPENDS "${CMAKE_SOURCE_DIR}/Scripts/json_config_parse.py"
    COMMAND "python3" ARGS "${CMAKE_SOURCE_DIR}/Scripts/json_asm_config_parse.py" "${ASM_SRC}" "${OUTPUT_CONFIG_NAME}")

  list(APPEND ASM_DEPENDS "${OUTPUT_NAME};${OUTPUT_CONFIG_NAME}")

  # Format is "<Test Arguments>" "<Test Name>" "<Test Type>"

  set(TEST_ARGS)
  if (ARCHITECTURE_arm64 OR ENABLE_VIXL_SIMULATOR)
    list(APPEND TEST_ARGS
      "FEX_SILENTLOG=0 FEX_DUMPGPRS=1 FEX_MAXINST=1 FEX_MULTIBLOCK=0 FEX_TSOENABLED=0"   "jit_1"     "jit"
      "FEX_SILENTLOG=0 FEX_DUMPGPRS=1 FEX_MAXINST=500 FEX_MULTIBLOCK=0 FEX_TSOENABLED=0" "jit_500"   "jit"
      "FEX_SILENTLOG=0 FEX_DUMPGPRS=1 FEX_MAXINST=500 FEX_MULTIBLOCK=1 FEX_TSOENABLED=0" "jit_500_m" "jit")
  endif()

  if (ENABLE_VIXL_SIMULATOR)
    set(CPU_CLASS Simulator)
  elseif (ARCHITECTURE_x86_64)
    list(APPEND TEST_ARGS "FEX_SILENTLOG=0 FEX_DUMPGPRS=1" "host" "host")
  endif()

  if (NOT MINGW)
    set(LAUNCH_PROGRAM "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner")
  else()
    set(LAUNCH_PROGRAM "wine" "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner.exe")
  endif()

  list(LENGTH TEST_ARGS ARG_COUNT)
  math(EXPR ARG_COUNT "${ARG_COUNT}-1")
  foreach(Index RANGE 0 ${ARG_COUNT} 3)
    math(EXPR TEST_NAME_INDEX "${Index}+1")
    math(EXPR TEST_TYPE_INDEX "${Index}+2")

    list(GET TEST_ARGS ${Index} FEX_ARGS)
    list(GET TEST_ARGS ${TEST_NAME_INDEX} TEST_DESC)
    list(GET TEST_ARGS ${TEST_TYPE_INDEX} TEST_TYPE)

    set(TEST_NAME "${TEST_DESC}/Test_64Bit_${REL_TEST_ASM}")
    string(REPLACE " " ";" FEX_ARGS_LIST ${FEX_ARGS})

    if (TEST_NAME MATCHES "SelfModifyingCode")
      list(APPEND FEX_ARGS_LIST "FEX_SMCCHECKS=full")
    endif()

    add_test(NAME ${TEST_NAME}
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/testharness_runner.py"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Known_Failures"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Known_Failures_${TEST_TYPE}"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Disabled_Tests"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Disabled_Tests_${TEST_TYPE}"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Disabled_Tests_${CPU_CLASS}"
      "Test_${REL_TEST_ASM}"
      "${TEST_NAME}"
      ${LAUNCH_PROGRAM}
      "${OUTPUT_NAME}" "${OUTPUT_CONFIG_NAME}")
    # This will cause the ASM tests to fail if it can't find the TestHarness or ASMN files
    # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
    # Just make sure to independently run `make all` then `make test`
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_NAME}")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY SKIP_RETURN_CODE 125)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT ${FEX_ARGS_LIST})
    if (MINGW)
      # Ensure the DOS region can be allocated.
      set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT "WINEPRELOADRESERVE=10000-110000")
    endif()
  endforeach()

endforeach()

add_custom_target(asm_files ALL DEPENDS "${ASM_DEPENDS}")

add_custom_target(64bit_asm_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  DEPENDS asm_files
  DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner"
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*64Bit\.*.asm$$")

add_custom_target(asm_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  DEPENDS asm_files
  DEPENDS 32bit_asm_files
  DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner"
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*.asm$$")
