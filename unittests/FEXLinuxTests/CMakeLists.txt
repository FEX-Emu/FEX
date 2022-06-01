include(ExternalProject)
ExternalProject_Add(FEXLinuxTests
  PREFIX FEXLinuxTests
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests"
  CMAKE_ARGS
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DX86_C_COMPILER:STRING=${X86_C_COMPILER}"
    "-DX86_CXX_COMPILER:STRING=${X86_CXX_COMPILER}"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON
)

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)
foreach(TEST ${TESTS})
  
  get_filename_component(TEST_NAME ${TEST} NAME_WLE)

  file(READ ${TEST} TEST_CODE)
  
  # Used to insert a configuration dependency to the test file
  CONFIGURE_FILE(${TEST} ${CMAKE_BINARY_DIR}/junk.file)

  set(ARGS_REGEX "auto args = \"([^\"]+)\";")
  string(REGEX MATCH ${ARGS_REGEX} TEST_ARGS ${TEST_CODE})
  # if cannot handle multiline variables, so we have to match the line first
  if(${TEST_ARGS} MATCHES ${ARGS_REGEX})
    string(REGEX REPLACE " |," ";" ARGS "${CMAKE_MATCH_1}")
    set(VARIATIONS "")
    foreach(ARG ${ARGS})
      list(APPEND VARIATIONS "${TEST_NAME}-${ARG}:${ARG}")
    endforeach()
  else()
    set(VARIATIONS "${TEST_NAME}:")
  endif()
  
  set(ALL_BITNESS 32 64)
  foreach(VARIATION ${VARIATIONS})
    foreach(BITNESS ${ALL_BITNESS})
      string(REGEX REPLACE ":" ";" VARIATION "${VARIATION}")
      list(GET VARIATION 0 VARIATION_NAME)
      list(GET VARIATION 1 VARIATION_ARG)
      set(BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests/${TEST_NAME}.${BITNESS}")
      
      set(TEST_CASE "${VARIATION_NAME}.${BITNESS}")
      
      # Add jit test case
      add_test(NAME "${TEST_CASE}.jit.flt"
        COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
        "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
        "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
        "${TEST_CASE}"
        "guest"
        "$<TARGET_FILE:FEXLoader>"
        "--no-silent" "-c" "irjit" "-n" "500" "--"
        "${BIN_PATH}"
        "${VARIATION_ARG}")
      if (_M_X86_64)
        # Add host test case
        add_test(NAME "${TEST_CASE}.host.flt"
          COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures_Host"
          "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
          "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests_Host"
          "${TEST_CASE}"
          "host"
          "${BIN_PATH}"
          "${VARIATION_ARG}")
      endif()
    endforeach()
  endforeach()
endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

# Only emulated
add_custom_target(
  fex_linux_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.jit\.flt$$" "--output-on-failure"
  DEPENDS FEXLinuxTests FEXLoader
)

# Only host
add_custom_target(
  fex_linux_tests_host
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.host\.flt$$" "--output-on-failure"
  DEPENDS FEXLinuxTests
)

# Both host and emulated
add_custom_target(
  fex_linux_tests_all
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.flt$$" "--output-on-failure"
  DEPENDS FEXLinuxTests FEXLoader
)