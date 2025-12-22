add_library(CommonWindowsRuntime STATIC LoadConfig.S)
add_subdirectory(CRT)
add_subdirectory(WinAPI)

target_link_libraries(CommonWindowsRuntime FEXCore_Base JemallocLibs)
target_compile_options(CommonWindowsRuntime PRIVATE -Wno-inconsistent-dllimport)
target_include_directories(CommonWindowsRuntime PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
)

add_library(CommonWindows STATIC CPUFeatures.cpp SHMStats.cpp InvalidationTracker.cpp ImageTracker.cpp Logging.cpp)

target_link_libraries(CommonWindows FEXCore_Base)
target_include_directories(CommonWindows PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
)
