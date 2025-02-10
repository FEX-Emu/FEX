set (TESTS
  Allocator
  ArgumentParser
  InterruptableConditionVariable
  Filesystem
  StringUtils
  fextl_function
  )

list(APPEND LIBS Common FEXCore JemallocLibs)

foreach(API_TEST ${TESTS})
  add_executable(${API_TEST} ${API_TEST}.cpp)
  target_link_libraries(${API_TEST} PRIVATE ${LIBS} Catch2::Catch2WithMain)

  catch_discover_tests(${API_TEST}
    TEST_SUFFIX ".${API_TEST}.APITest")
endforeach()

add_custom_target(
  api_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*.APITest")

foreach(API_TEST ${TESTS})
  add_dependencies(api_tests ${API_TEST})
endforeach()
