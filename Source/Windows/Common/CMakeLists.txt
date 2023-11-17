add_library(CommonWindows STATIC CPUFeatures.cpp)
target_link_libraries(CommonWindows FEXCore_Base)

target_include_directories(CommonWindows PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
)
