set (TESTS
  InterruptableConditionVariable
  LexicallyNormal
  )

list(APPEND LIBS FEXCore)

foreach(API_TEST ${TESTS})
  add_executable(${API_TEST} ${API_TEST}.cpp)
  target_link_libraries(${API_TEST} PRIVATE ${LIBS} Catch2::Catch2WithMain)

  catch_discover_tests(${API_TEST}
    TEST_SUFFIX ".${API_TEST}.APITest")
endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  api_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "\.*.APITest")

foreach(API_TEST ${TESTS})
  add_dependencies(api_tests ${API_TEST})
endforeach()
