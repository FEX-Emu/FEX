file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)

set (LIBS fmt::fmt vixl Catch2::Catch2WithMain FEXCore_Base)
foreach(TEST ${TESTS})
  get_filename_component(TEST_NAME ${TEST} NAME_WLE)
  add_executable(FEXCore_Tests_${TEST_NAME} ${TEST})
  target_link_libraries(FEXCore_Tests_${TEST_NAME} PRIVATE ${LIBS})
  target_include_directories(FEXCore_Tests_${TEST_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/")
  set_target_properties(FEXCore_Tests_${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/FEXCore_Tests")
  catch_discover_tests(FEXCore_Tests_${TEST_NAME} TEST_SUFFIX ".${TEST_NAME}.FEXCore_Tests")
endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  fexcore_apitests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "\.*.FEXCore_Tests$$")

