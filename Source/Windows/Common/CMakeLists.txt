add_library(CommonWindowsRuntime STATIC LoadConfig.S)
add_subdirectory(CRT)
add_subdirectory(WinAPI)

target_link_libraries(CommonWindowsRuntime FEXCore_Base JemallocLibs)
target_compile_options(CommonWindowsRuntime PRIVATE -Wno-inconsistent-dllimport)
target_include_directories(CommonWindowsRuntime PRIVATE "${CMAKE_SOURCE_DIR}/Source/Windows/include/")

add_library(CommonWindows STATIC
  Allocator.cpp
  CPUFeatures.cpp
  EnvironmentVariablesHandling.cpp
  FEXUnixLib.cpp
  SHMStats.cpp
  InvalidationTracker.cpp
  ImageTracker.cpp
  Logging.cpp
  Threads.cpp
  Horizon.cpp)

target_link_libraries(CommonWindows FEXCore_Base)
target_include_directories(CommonWindows PRIVATE "${CMAKE_SOURCE_DIR}/Source/Windows/include/")

# Apply Tegra X1 / Cortex-A57 compile flags to Windows modules (FEX-2610)
if (FEX_TUNE_COMPILE_FLAGS)
  target_compile_options(CommonWindows PRIVATE ${FEX_TUNE_COMPILE_FLAGS})
  target_compile_options(CommonWindowsRuntime PRIVATE ${FEX_TUNE_COMPILE_FLAGS})
endif()

# Add FEX_HORIZON compile definition when building for Horizon
if (FEX_HORIZON)
  target_compile_definitions(CommonWindows PRIVATE FEX_HORIZON)
  target_compile_definitions(CommonWindowsRuntime PRIVATE FEX_HORIZON)
  target_include_directories(CommonWindows PRIVATE "${CMAKE_SOURCE_DIR}/Source/Windows/include/")
  target_include_directories(CommonWindowsRuntime PRIVATE "${CMAKE_SOURCE_DIR}/Source/Windows/include/")
endif()
