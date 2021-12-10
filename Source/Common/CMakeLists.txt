set(NAME Common)
set(SRCS
  ArgumentLoader.cpp
  Config.cpp
  EnvironmentLoader.cpp
  FileFormatCheck.cpp
  RootFSSetup.cpp
  StringUtil.cpp
  SocketLogging.cpp)

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base cpp-optparse json-maker)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/cpp-optparse/)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
