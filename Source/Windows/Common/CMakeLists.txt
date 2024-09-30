add_library(CommonWindows STATIC CPUFeatures.cpp InvalidationTracker.cpp Logging.cpp LoadConfig.S)
add_subdirectory(CRT)
add_subdirectory(WinAPI)
target_link_libraries(CommonWindows FEXCore_Base JemallocLibs)
target_compile_options(CommonWindows PRIVATE -Wno-inconsistent-dllimport)
target_include_directories(CommonWindows PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
)
