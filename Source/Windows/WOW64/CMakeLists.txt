add_library(wow64fex SHARED
  Module.cpp
  libwow64fex.def
)

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

target_link_options(wow64fex PRIVATE "LINKER:--image-base,0x6f100000")

install(TARGETS wow64fex
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)
