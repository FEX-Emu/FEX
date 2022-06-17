set(NAME FEXGDBReader)
set(SRCS FEXGDBReader.cpp)

add_library(${NAME} SHARED ${SRCS})

install(TARGETS ${NAME}
  RUNTIME
  LIBRARY DESTINATION /usr/lib/gdb
  COMPONENT LIBRARY)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
