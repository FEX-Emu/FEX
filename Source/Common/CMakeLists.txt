add_subdirectory(cpp-optparse/)

set(NAME Common)
set(SRCS
  Config.cpp
  CPUInfo.cpp
  ArgumentLoader.cpp
  EnvironmentLoader.cpp
  HostFeatures.cpp
  JSONPool.cpp
  StringUtil.cpp
  SHMStats.cpp)

if (NOT MINGW_BUILD)
  list (APPEND SRCS
    FEXServerClient.cpp
    FileFormatCheck.cpp)
endif()

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base cpp-optparse tiny-json FEXHeaderUtils)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
