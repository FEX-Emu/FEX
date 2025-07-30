set(NAME FEXGDBReader)
set(SRCS FEXGDBReader.cpp)

add_library(${NAME} SHARED ${SRCS})

install(TARGETS ${NAME}
  RUNTIME
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gdb
  COMPONENT LIBRARY)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)

# We don't actually link, but this is a nice way to get the include dirs
target_link_libraries(${NAME} PRIVATE Common)
