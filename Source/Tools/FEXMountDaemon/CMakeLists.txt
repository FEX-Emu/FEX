set(NAME FEXMountDaemon)
set(SRCS Main.cpp)

add_executable(${NAME} ${SRCS})

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)

target_link_libraries(${NAME} PRIVATE ${STATIC_PIE_OPTIONS})
