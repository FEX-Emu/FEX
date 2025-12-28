add_executable(FEXServer
  Main.cpp
  ArgumentLoader.cpp
  Logger.cpp
  PipeScanner.cpp
  ProcessPipe.cpp
  SquashFS.cpp)

target_include_directories(FEXServer PRIVATE
  ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(FEXServer PRIVATE FEXCore Common CommonTools JemallocDummy ${PTHREAD_LIB})

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXServer
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
endif()

install(TARGETS FEXServer
  RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
