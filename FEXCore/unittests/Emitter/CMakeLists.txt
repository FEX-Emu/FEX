file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)

set (LIBS fmt::fmt vixl Catch2::Catch2WithMain FEXCore_Base JemallocLibs)
foreach(TEST ${TESTS})
  get_filename_component(TEST_NAME ${TEST} NAME_WLE)
  add_executable(Emitter_${TEST_NAME} ${TEST})
  target_link_libraries(Emitter_${TEST_NAME} PRIVATE ${LIBS})
  target_include_directories(Emitter_${TEST_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/")
  set_target_properties(Emitter_${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/EmitterTests")
  catch_discover_tests(Emitter_${TEST_NAME} TEST_SUFFIX ".${TEST_NAME}.Emitter")
endforeach()

add_custom_target(
  emitter_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*.Emitter$$")
