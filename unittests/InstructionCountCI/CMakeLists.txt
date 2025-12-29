# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE JSON_SOURCES CONFIGURE_DEPENDS *.json)

set(JSON_DEPENDS "")
set(JSON_UPDATE_DEPENDS "")

foreach(JSON_SRC ${JSON_SOURCES})
  file(RELATIVE_PATH REL_JSON ${CMAKE_SOURCE_DIR} ${JSON_SRC})
  file(RELATIVE_PATH REL_TEST_JSON ${CMAKE_CURRENT_SOURCE_DIR} ${JSON_SRC})
  get_filename_component(JSON_NAME ${JSON_SRC} NAME)
  get_filename_component(JSON_DIR "${REL_JSON}" DIRECTORY)
  set(OUTPUT_JSON_FOLDER "${CMAKE_BINARY_DIR}/${JSON_DIR}")

  # Generate build directory
  file(MAKE_DIRECTORY "${OUTPUT_JSON_FOLDER}")
  set(OUTPUT_JSON_NAME "${OUTPUT_JSON_FOLDER}/${JSON_NAME}.instcountci")
  set(OUTPUT_JSON_NEWNUMBERS_NAME "${OUTPUT_JSON_FOLDER}/${JSON_NAME}.instcountci.json")

  add_custom_command(OUTPUT ${OUTPUT_JSON_NAME}
    DEPENDS "${JSON_SRC}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Scripts/InstructionCountParser.py"
    COMMAND "python3" ARGS "${CMAKE_SOURCE_DIR}/Scripts/InstructionCountParser.py" "${JSON_SRC}" "${OUTPUT_JSON_NAME}")

  list(APPEND JSON_DEPENDS "${OUTPUT_JSON_NAME}")

  if (NOT MINGW_BUILD)
    set(LAUNCH_PROGRAM "${CMAKE_BINARY_DIR}/Bin/CodeSizeValidation")
  else()
    set(LAUNCH_PROGRAM "wine" "${CMAKE_BINARY_DIR}/Bin/CodeSizeValidation.exe")
  endif()

  file(RELATIVE_PATH JSON_PATH_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${JSON_SRC})
  set(TEST_NAME "InstCountCI/Test_${JSON_PATH_RELATIVE}.instcountci")
  set(TEST_NAME_UPDATE_NUMBERS "InstCountCI/Test_${JSON_PATH_RELATIVE}.new_numbers")

  add_test(NAME ${TEST_NAME}
    COMMAND ${LAUNCH_PROGRAM} "${OUTPUT_JSON_NAME}" "${OUTPUT_JSON_NEWNUMBERS_NAME}")

  add_test(NAME ${TEST_NAME_UPDATE_NUMBERS}
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/UpdateInstructionCountJson.py" "${JSON_SRC}" "${OUTPUT_JSON_NEWNUMBERS_NAME}")

  set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/CodeSizeValidation")
  set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_JSON_NAME}")

  set_property(TEST ${TEST_NAME_UPDATE_NUMBERS} APPEND PROPERTY DEPENDS "${TEST_NAME}")
endforeach()

add_custom_target(instcountci_test_files ALL
  DEPENDS "${JSON_DEPENDS}")

add_custom_target(instcountci_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  DEPENDS instcountci_test_files
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "InstCountCI/\.*.instcountci$$")

add_custom_target(instcountci_update_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "InstCountCI/\.*new_numbers$$")
