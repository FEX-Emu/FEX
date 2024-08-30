add_subdirectory(CommonTools)

if (NOT MINGW_BUILD)
  if (USE_FEXCONFIG_TOOLKIT STREQUAL "imgui")
    add_subdirectory(FEXConfig/)
  elseif (USE_FEXCONFIG_TOOLKIT STREQUAL "qt")
    find_package(Qt6 COMPONENTS Qml Quick Widgets)
    if (NOT Qt6_FOUND)
      find_package(Qt5 COMPONENTS Qml Quick Widgets REQUIRED)
    endif()

    add_subdirectory(FEXQonfig/)
  endif()

  if (ENABLE_GDB_SYMBOLS)
    add_subdirectory(FEXGDBReader/)
  endif()

  add_subdirectory(FEXRootFSFetcher/)
  add_subdirectory(FEXGetConfig/)
  add_subdirectory(FEXServer/)
  add_subdirectory(FEXBash/)
  add_subdirectory(CodeSizeValidation/)
  add_subdirectory(LinuxEmulation/)

  add_subdirectory(FEXLoader/)
  add_subdirectory(pidof/)
endif()

if (BUILD_TESTS)
  add_subdirectory(TestHarnessRunner/)
endif()
