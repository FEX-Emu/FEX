add_subdirectory(CommonTools)

if (NOT MINGW_BUILD)
  if (BUILD_FEXCONFIG)
    add_subdirectory(FEXConfig/)
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
