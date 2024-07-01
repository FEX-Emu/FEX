add_executable(thunkgentest generator.cpp abi.cpp)
target_link_libraries(thunkgentest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(thunkgentest PRIVATE fmt::fmt)
target_link_libraries(thunkgentest PRIVATE thunkgenlib)
catch_discover_tests(thunkgentest TEST_SUFFIX ".ThunkGen")

add_custom_target(
  thunkgen_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "\.*.ThunkGen")
add_dependencies(thunkgen_tests thunkgentest)
