set(NAME FEXRootFSFetcher)
set(SRCS Main.cpp
  XXFileHash.cpp)

add_executable(${NAME} ${SRCS})
list(APPEND LIBS FEXCore Common CommonCore)

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/)

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

target_link_libraries(${NAME} PRIVATE ${LIBS} ${STATIC_PIE_OPTIONS} ${PTHREAD_LIB})
