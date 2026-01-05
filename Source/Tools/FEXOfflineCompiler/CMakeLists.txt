add_executable(FEXOfflineCompiler Main.cpp)

target_link_libraries(FEXOfflineCompiler PRIVATE
  Common
  CommonTools
  cpp-optparse
  FEXCore
  JemallocLibs
  LinuxEmulation
  ${PTHREAD_LIB}
  fmt::fmt)

LinkerGC(FEXOfflineCompiler)

install(TARGETS FEXOfflineCompiler RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
