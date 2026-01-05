add_executable(FEXServer
  Main.cpp
  ArgumentLoader.cpp
  Logger.cpp
  PipeScanner.cpp
  ProcessPipe.cpp
  SquashFS.cpp)

target_include_directories(FEXServer PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(FEXServer PRIVATE FEXCore Common CommonTools JemallocDummy ${PTHREAD_LIB})

LinkerGC(FEXServer)

install(TARGETS FEXServer RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
