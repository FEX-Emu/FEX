include(ExternalProject)
ExternalProject_Add(FEXLinuxTests
  PREFIX FEXLinuxTests
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests_64"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_64_TOOLCHAIN_FILE}"
  "-DENABLE_CLANG_THUNKS=True"
  "-DBITNESS=64"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

ExternalProject_Add(FEXLinuxTests_32
  PREFIX FEXLinuxTests_32
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXLinuxTests_32"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_32_TOOLCHAIN_FILE}"
  "-DENABLE_CLANG_THUNKS=True"
  "-DBITNESS=32"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)
file(GLOB_RECURSE TESTS_64_ONLY CONFIGURE_DEPENDS tests/*.64.cpp)
file(GLOB_RECURSE TESTS_32_ONLY CONFIGURE_DEPENDS tests/*.32.cpp)

# Apply bitness-specific exclude lists
list(REMOVE_ITEM TESTS ${TESTS_64_ONLY})
list(REMOVE_ITEM TESTS ${TESTS_32_ONLY})

# target directories
set(PORTABLE_INSTALL_DIR "${CMAKE_BINARY_DIR}/install")
set(GLOBAL_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")

function(AddTests Tests BinDirectory Bitness PortableTest)
  if (PortableTest)
    set(TEST_DIR "${PORTABLE_INSTALL_DIR}")
    set(TEST_TYPE "portable")
    set(TEST_LABEL "")
  else()
    set(TEST_DIR "${GLOBAL_INSTALL_DIR}")
    set(TEST_TYPE "global")
    set(TEST_LABEL "integration")
  endif()

  foreach(TEST ${Tests})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    set(BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BinDirectory}/${TEST_NAME}.${Bitness}")
    set(TEST_CASE_BASE "${TEST_NAME}.${Bitness}")
    set(TEST_CASE "${TEST_CASE_BASE}.${TEST_TYPE}")

    if(TEST_NAME STREQUAL "thunk_testlib")
      # Test thunking only if thunks are enabled and supported
      if(NOT BUILD_THUNKS OR ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT)
        continue()
      endif()
    endif()

    # Add jit test case
    add_test(NAME "${TEST_CASE}.jit.flt"
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures"
      "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
      "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests"
      "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
      "${TEST_CASE_BASE}"
      "guest"
      "${TEST_DIR}/bin/FEX"
      "${BIN_PATH}")

    set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY ENVIRONMENT "FEX_OUTPUTLOG=stderr;FEX_SILENTLOG=0;FEX_MAXINST=500")

    if(TEST_NAME STREQUAL "thunk_testlib")
      set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY ENVIRONMENT
        "FEX_THUNKCONFIG=${CMAKE_SOURCE_DIR}/Data/CI/FEXLinuxTestsThunks.json"
        "LD_LIBRARY_PATH=${TEST_DIR}/lib")
      if (PortableTest)
        set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY ENVIRONMENT
          "FEX_PORTABLE=1"
          "FEX_THUNKHOSTLIBS=${TEST_DIR}/${HOSTLIBS_DATA_DIRECTORY}/HostThunks"
          "FEX_THUNKGUESTLIBS=${TEST_DIR}/${DATA_DIRECTORY}/GuestThunks")
      else()     
        set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY ENVIRONMENT
          "FEX_PORTABLE=0")
      endif()
    endif()

    if (ARCHITECTURE_x86_64 AND NOT TEST_NAME STREQUAL "thunk_testlib")
      # Add host test case
      add_test(NAME "${TEST_CASE}.host.flt"
        COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/guest_test_runner.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/Known_Failures_Host"
        "${CMAKE_CURRENT_SOURCE_DIR}/Expected_Output"
        "${CMAKE_CURRENT_SOURCE_DIR}/Disabled_Tests_Host"
        "${CMAKE_CURRENT_SOURCE_DIR}/Flake_Tests"
        "${TEST_CASE_BASE}"
        "host"
        "${BIN_PATH}")
      set_property(TEST "${TEST_CASE}.host.flt" APPEND PROPERTY SKIP_RETURN_CODE 125)
      set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY LABELS "${TEST_LABEL}")
    endif()
    set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY SKIP_RETURN_CODE 125)
    set_property(TEST "${TEST_CASE}.jit.flt" APPEND PROPERTY LABELS "${TEST_LABEL}")
  endforeach()
endfunction()

function(AddTestSuite TestTargetName PortableTest)
  if(PortableTest)
    add_custom_target("${TEST_TARGET}_install"
      COMMAND ${CMAKE_COMMAND} "--install" "." "--prefix" "${PORTABLE_INSTALL_DIR}"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set(INSTALL_TARGET "${TEST_TARGET}_install")
    set(TEST_TYPE "portable")
  else()
    set(INSTALL_TARGET "")
    set(TEST_TYPE "global")
  endif()

  # Execute combined 32-bit and 64-bit tests.
  AddTests("${TESTS}" "FEXLinuxTests_64" 64 "${PortableTest}")
  AddTests("${TESTS}" "FEXLinuxTests_32" 32 "${PortableTest}")
  # Execute tests that are only 64-bit.
  AddTests("${TESTS_64_ONLY}" "FEXLinuxTests_64" 64 "${PortableTest}")
  # Execute tests that are only 32-bit.
  AddTests("${TESTS_32_ONLY}" "FEXLinuxTests_32" 32 "${PortableTest}")

  # Only emulated
  add_custom_target("${TestTargetName}_tests"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMAND "ctest" "--output-on-failure" "--timeout" "30" ${TEST_JOB_FLAG} "-R" "\.*\.${TEST_TYPE}\.jit\.flt$$"
    DEPENDS FEXLinuxTests FEXLinuxTests_32 FEX "${INSTALL_TARGET}")

  # Only host
  add_custom_target("${TestTargetName}_tests_host"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMAND "ctest" "--output-on-failure" "--timeout" "30" ${TEST_JOB_FLAG} "-R" "\.*\.${TEST_TYPE}\.host\.flt$$"
    DEPENDS FEXLinuxTests FEXLinuxTests_32 "${INSTALL_TARGET}")

  # Both host and emulated
  add_custom_target("${TestTargetName}_tests_all"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMAND "ctest" "--output-on-failure" "--timeout" "30" ${TEST_JOB_FLAG} "-R" "\.*\.${TEST_TYPE}\.*\.flt$$"
    DEPENDS FEXLinuxTests FEXLinuxTests_32 FEX "${INSTALL_TARGET}")

endfunction()

AddTestSuite("fex_linux_portable" ON)
AddTestSuite("fex_linux_global" OFF)
