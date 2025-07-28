set(FUNCTIONAL_DEPENDS "")

function(AddThunksTest Bin ThunksFile)
  set (ARGS
    "-t" "${HOSTLIBS_DATA_DIRECTORY}/HostThunks"
    "-j" "${CMAKE_INSTALL_PREFIX}/share/fex-emu/GuestThunks"
    "-o" "stderr" "--no-silentlog" "-n" "500"
    )
  if (NOT ThunksFile)
    set (TEST_NAME ThunkFunctionalTest-NoThunks-${Bin})
  else()
    set (TEST_NAME ThunkFunctionalTest-Thunks-${Bin})
    list (APPEND ARGS
      "-k" "${CMAKE_SOURCE_DIR}/Data/CI/${ThunksFile}")
  endif()

  add_test(NAME ${TEST_NAME}
    COMMAND "$<TARGET_FILE:FEXLoader>"
    ${ARGS}
    "--"
    ${Bin})
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${Bin}")

  list(APPEND FUNCTIONAL_DEPENDS "${TEST_NAME}")
endfunction()

function(AddTest Bin ThunksFile)
  AddThunksTest("${Bin}" "")
  AddThunksTest("${Bin}" "${ThunksFile}")
endfunction()

AddTest("/usr/bin/glxinfo" "GLThunks.json")
AddTest("/usr/bin/vulkaninfo" "VulkanThunks.json")

add_custom_target(
  thunk_functional_tests_nothunks
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "ThunkFunctionalTest-NoThunks-\.*"
  DEPENDS "${FUNCTIONAL_DEPENDS}")

add_custom_target(
  thunk_functional_tests_thunks
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "ThunkFunctionalTest-Thunks-\.*"
  DEPENDS "${FUNCTIONAL_DEPENDS}")

add_custom_target(
  thunk_functional_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "ThunkFunctionalTest\.*"
  DEPENDS "${FUNCTIONAL_DEPENDS}")
