cmake_minimum_required(VERSION 3.14)
project(FEXUnixLib CXX)

include(GNUInstallDirs)

function(CreateLib TargetName LibName)
  add_library(${TargetName} SHARED FEXUnixLib.cpp)
  set_target_properties(${TargetName} PROPERTIES OUTPUT_NAME ${LibName} PREFIX "lib" SUFFIX ".so")
  target_link_libraries(${TargetName} PRIVATE rt)
  install(TARGETS ${TargetName} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime)
endfunction()

CreateLib(wow64fex_unixlib wow64fex)
CreateLib(arm64ecfex_unixlib arm64ecfex)
