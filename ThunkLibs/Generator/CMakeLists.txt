find_package(Clang REQUIRED CONFIG)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# Query clang's global resource directory for system include directories
if (NOT CLANG_RESOURCE_DIR)
  find_program(CLANG_EXEC_PATH clang REQUIRED)

  execute_process(COMMAND ${CLANG_EXEC_PATH} -print-resource-dir
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

add_library(thunkgenlib STATIC analysis.cpp data_layout.cpp gen.cpp)
target_include_directories(thunkgenlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(thunkgenlib SYSTEM PUBLIC ${CLANG_INCLUDE_DIRS})
target_link_libraries(thunkgenlib PUBLIC clang-cpp LLVM)
target_link_libraries(thunkgenlib PRIVATE OpenSSL::Crypto)
target_link_libraries(thunkgenlib PRIVATE fmt::fmt)
target_compile_definitions(thunkgenlib INTERFACE -DCLANG_RESOURCE_DIR="${CLANG_RESOURCE_DIR}")

# Clang's libtooling won't compile with libstdc++'s debug mode
target_compile_options(thunkgenlib PUBLIC "-U_GLIBCXX_DEBUG")

add_executable(thunkgen main.cpp)
target_link_libraries(thunkgen PRIVATE thunkgenlib)
