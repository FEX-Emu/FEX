add_executable(FEXOfflineCompiler Main.cpp)

target_link_libraries(FEXOfflineCompiler PRIVATE
  Common
  CommonTools
  cpp-optparse
  FEXCore
  JemallocLibs
  fmt::fmt)

if (MINGW)
  patch_library_wine(FEXOfflineCompiler)
  target_include_directories(FEXOfflineCompiler PRIVATE
    "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
    "${CMAKE_SOURCE_DIR}/Source/"
    "${CMAKE_SOURCE_DIR}/Source/Windows/"
  )
  target_link_libraries(FEXOfflineCompiler PRIVATE CommonWindows ntdll_ex)
else()
  target_link_libraries(FEXOfflineCompiler PRIVATE ${PTHREAD_LIB} LinuxEmulation)
endif()

LinkerGC(FEXOfflineCompiler)

install(TARGETS FEXOfflineCompiler RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
