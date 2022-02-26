set(NAME FEXMountDaemon)
set(SRCS Main.cpp)

add_executable(${NAME} ${SRCS})

if (STRIP_BUILD)
  target_link_options(${NAME}
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)

target_link_libraries(${NAME} PRIVATE ${STATIC_PIE_OPTIONS} ${PTHREAD_LIB})
