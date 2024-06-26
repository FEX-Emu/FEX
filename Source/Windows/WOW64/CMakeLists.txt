add_library(wow64fex SHARED
  Module.cpp
  libwow64fex.def
)
patch_library_wine(wow64fex)

target_include_directories(wow64fex PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
  "${CMAKE_SOURCE_DIR}/Source/Windows/"
  "${CMAKE_SOURCE_DIR}/Source/"
)

target_link_libraries(wow64fex
  PRIVATE
  FEXCore
  Common
  CommonTools
  CommonWindows
  wow64_ex
  ntdll_ex
  ntdll
)

install(TARGETS wow64fex
  RUNTIME
  DESTINATION lib
  COMPONENT runtime)
