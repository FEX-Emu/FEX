find_package(Clang REQUIRED CONFIG)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

add_library(thunkgenlib gen.cpp)
target_include_directories(thunkgenlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(thunkgenlib SYSTEM PUBLIC ${CLANG_INCLUDE_DIRS})
target_link_libraries(thunkgenlib PUBLIC clangTooling)
target_link_libraries(thunkgenlib PRIVATE OpenSSL::Crypto)

add_executable(thunkgen main.cpp)
target_link_libraries(thunkgen PRIVATE thunkgenlib)
