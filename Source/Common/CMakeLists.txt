add_subdirectory(cpp-optparse/)

set(NAME Common)
set(SRCS
  ArgumentLoader.cpp
  Config.cpp
  EnvironmentLoader.cpp
  FEXServerClient.cpp
  FileFormatCheck.cpp
  StringUtil.cpp)

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base cpp-optparse json-maker FEXHeaderUtils)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/cpp-optparse/)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
