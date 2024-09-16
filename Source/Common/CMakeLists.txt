add_subdirectory(cpp-optparse/)

set(NAME Common)
set(SRCS
  Config.cpp
  ArgumentLoader.cpp
  EnvironmentLoader.cpp
  HostFeatures.cpp
  JSONPool.cpp
  MicroArgParser.cpp
  StringUtil.cpp)

if (NOT MINGW_BUILD)
  list (APPEND SRCS
    FEXServerClient.cpp
    FileFormatCheck.cpp)
endif()

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base tiny-json FEXHeaderUtils)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/xbyak/)
