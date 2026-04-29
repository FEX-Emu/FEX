cmake_minimum_required(VERSION 3.14)
project(FEXUnixlib CXX)

include(GNUInstallDirs)

add_library(fex_unixlib_obj OBJECT FEXUnixlib.cpp)
set_target_properties(fex_unixlib_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(wow64fex_unixlib SHARED $<TARGET_OBJECTS:fex_unixlib_obj>)
set_target_properties(wow64fex_unixlib PROPERTIES OUTPUT_NAME "wow64fex")
target_link_libraries(wow64fex_unixlib PRIVATE rt)
install(TARGETS wow64fex_unixlib LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime)

add_library(arm64ecfex_unixlib SHARED $<TARGET_OBJECTS:fex_unixlib_obj>)
set_target_properties(arm64ecfex_unixlib PROPERTIES OUTPUT_NAME "arm64ecfex")
target_link_libraries(arm64ecfex_unixlib PRIVATE rt)
install(TARGETS arm64ecfex_unixlib LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime)
