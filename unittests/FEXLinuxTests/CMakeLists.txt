include(ExternalProject)

set(GuestArchList "x86_32;x86_64")

foreach(GUEST_ARCH ${GuestArchList})
  set(TOOLCHAIN_FILE ${TOOLCHAIN_FILE_${GUEST_ARCH}})
  message("FEXLinuxTests: toolchain for |${GUEST_ARCH}| is \"${TOOLCHAIN_FILE}\"")

  ExternalProject_Add(FEXLinuxTests-${GUEST_ARCH}
    PREFIX FEXLinuxTests-${GUEST_ARCH}
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests-${GUEST_ARCH}"
    CMAKE_ARGS
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${TOOLCHAIN_FILE}"
    "-DGUEST_ARCH=${GUEST_ARCH}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DLINKER_OVERRIDE=${LINKER_OVERRIDE}"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
    )
endforeach()

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)

function(AddTests Tests ArchList)
  foreach(TEST ${Tests})
    get_filename_component(TEST_NAME ${TEST} NAME_WLE)

    file(READ ${TEST} TEST_CODE)

    # Used to insert a configuration dependency to the test file
    CONFIGURE_FILE(${TEST} ${CMAKE_BINARY_DIR}/junk.file)

    # Get test variations
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

    # Get test arch overrides
    set(ARCH_REGEX "//[ ]*arch: ([^\n]+)")

    string(REGEX MATCH ${ARCH_REGEX} ARCHES ${TEST_CODE})
    # if cannot handle multiline variables, so we have to match the line first
    if(${ARCHES} MATCHES ${ARCH_REGEX})
      set(ArchList "${CMAKE_MATCH_1}" )
      string(REGEX REPLACE " |," ";" ArchList "${ArchList}")
    endif()

    foreach(VARIATION ${VARIATIONS})
      foreach(ARCH ${ArchList})
        string(REGEX REPLACE ":" ";" VARIATION "${VARIATION}")
        list(GET VARIATION 0 VARIATION_NAME)
        list(GET VARIATION 1 VARIATION_ARG)
        set(BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests-${ARCH}/${TEST_NAME}.${ARCH}")

        set(TEST_CASE "${VARIATION_NAME}.${ARCH}")

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
endfunction()

AddTests("${TESTS}" ${GuestArchList})

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

set(TestTargets ${GuestArchList})
list(TRANSFORM TestTargets PREPEND "FEXLinuxTests-")

# Only emulated
add_custom_target(
  fex_linux_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.jit\.flt$$" "--output-on-failure"
  DEPENDS ${TestTargets} FEXLoader
  )

# Only host
add_custom_target(
  fex_linux_tests_host
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.host\.flt$$" "--output-on-failure"
  DEPENDS ${TestTargets}
  )

# Both host and emulated
add_custom_target(
  fex_linux_tests_all
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "30" "-j${CORES}" "-R" "\.*\.flt$$" "--output-on-failure"
  DEPENDS ${TestTargets} FEXLoader
  )
