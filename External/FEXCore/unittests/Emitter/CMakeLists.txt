if (COMPILE_VIXL_DISASSEMBLER)
  file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)

  set (LIBS fmt::fmt vixl Catch2::Catch2WithMain FEXCore_Base)
  foreach(TEST ${TESTS})
    get_filename_component(TEST_NAME ${TEST} NAME_WLE)
    add_executable(Emitter_${TEST_NAME} ${TEST})
    target_link_libraries(Emitter_${TEST_NAME} PRIVATE ${LIBS})
    target_include_directories(Emitter_${TEST_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/")
    set_target_properties(Emitter_${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/EmitterTests")
    catch_discover_tests(Emitter_${TEST_NAME} TEST_SUFFIX ".${TEST_NAME}.Emitter")
  endforeach()

  execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
  string(STRIP ${CORES} CORES)

  add_custom_target(
    emitter_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/"
    USES_TERMINAL
    COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "\.*.Emitter$$")
else()
  message(AUTHOR_WARNING "Tests are enabled but vixl disassembler is not. Emitter tests won't be built.")
endif()
