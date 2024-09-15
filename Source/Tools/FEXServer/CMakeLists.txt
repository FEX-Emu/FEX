set(NAME FEXServer)
set(SRCS Main.cpp
  ArgumentLoader.cpp
  Logger.cpp
  PipeScanner.cpp
  ProcessPipe.cpp
  SquashFS.cpp)

add_executable(${NAME} ${SRCS})

target_include_directories(${NAME} PRIVATE
  ${CMAKE_BINARY_DIR}/generated
  ${CMAKE_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} PRIVATE FEXCore Common cpp-optparse ${PTHREAD_LIB})

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
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
