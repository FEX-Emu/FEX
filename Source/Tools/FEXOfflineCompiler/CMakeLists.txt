add_executable(FEXOfflineCompiler Main.cpp)

target_link_libraries(FEXOfflineCompiler PRIVATE
  Common
  CommonTools
  cpp-optparse
  FEXCore
  JemallocLibs
  LinuxEmulation
  ${PTHREAD_LIB}
  fmt::fmt
  fex_unwind_x86_64 unwind)

target_link_directories(FEXOfflineCompiler PRIVATE ${CMAKE_BINARY_DIR}/unwind_x86_64/src/)

LinkerGC(FEXOfflineCompiler)

install(TARGETS FEXOfflineCompiler RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
