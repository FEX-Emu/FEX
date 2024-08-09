add_library(wow64fex SHARED
  Module.cpp
  libwow64fex.def
  $<TARGET_OBJECTS:FEXCore_object>
)
patch_library_wine(wow64fex)

target_include_directories(wow64fex PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
  "${CMAKE_SOURCE_DIR}/Source/Windows/"
  "${CMAKE_SOURCE_DIR}/Source/"
)

target_link_libraries(wow64fex
  PRIVATE
  FEXCore_Base
  Common
  CommonTools
  CommonWindows
  wow64_ex
  ntdll_ex
)

target_link_options(wow64fex PRIVATE -static -nostdlib -nostartfiles -nodefaultlibs -lc++ -lc++abi -lunwind -lclang_rt.builtins-aarch64)
install(TARGETS wow64fex
  RUNTIME
  DESTINATION lib
  COMPONENT runtime)
