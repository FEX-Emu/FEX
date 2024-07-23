add_library(CommonWindows STATIC CPUFeatures.cpp InvalidationTracker.cpp Logging.cpp)
add_subdirectory(CRT)
target_link_libraries(CommonWindows FEXCore_Base)
target_compile_options(CommonWindows PRIVATE -Wno-inconsistent-dllimport)
target_include_directories(CommonWindows PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
)
