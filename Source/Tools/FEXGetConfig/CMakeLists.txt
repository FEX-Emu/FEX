set(NAME FEXGetConfig)
set(SRCS Main.cpp)

add_executable(${NAME} ${SRCS})

list(APPEND LIBS Common CommonCore)

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)

target_link_libraries(${NAME} PRIVATE ${LIBS} ${STATIC_PIE_OPTIONS})

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
