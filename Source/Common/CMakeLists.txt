set(NAME Common)
set(SRCS
  ArgumentLoader.cpp
  EnvironmentLoader.cpp
  Config.cpp
  StringUtil.cpp)

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} cpp-optparse tiny-json json-maker)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/cpp-optparse/)
