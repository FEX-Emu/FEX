add_subdirectory(Common/)
add_subdirectory(Tools/)

if (MINGW_BUILD)
  add_subdirectory(Windows/)
endif()
