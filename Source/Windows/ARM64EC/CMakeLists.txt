add_library(arm64ecfex SHARED
  Module.cpp
  Module.S
  libarm64ecfex.def
  $<TARGET_OBJECTS:FEXCore_object>
)
patch_library_wine(arm64ecfex)

target_include_directories(arm64ecfex PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/Windows/include/"
  "${CMAKE_SOURCE_DIR}/Source/"
  "${CMAKE_SOURCE_DIR}/Source/Windows/"
)

target_link_libraries(arm64ecfex
  PRIVATE
  FEXCore_Base
  Common
  CommonTools
  CommonWindows
  ntdll_ex
)

target_link_options(arm64ecfex PRIVATE -static -nostdlib -nostartfiles -nodefaultlibs -lc++ -lc++abi -lunwind)
target_link_libraries(arm64ecfex PRIVATE ${LIBGCC_PATH})
install(TARGETS arm64ecfex
  RUNTIME
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT Runtime)
