if (NOT MINGW_BUILD)
  if (BUILD_FEXCONFIG)
    add_subdirectory(FEXConfig/)
  endif()

  if (NOT TERMUX_BUILD)
    # Disable FEXRootFSFetcher on Termux, it doesn't even work there
    add_subdirectory(FEXRootFSFetcher/)
  endif()

  if (ENABLE_GDB_SYMBOLS)
    add_subdirectory(FEXGDBReader/)
  endif()

  add_subdirectory(FEXGetConfig/)
  add_subdirectory(FEXServer/)
  add_subdirectory(FEXBash/)

  set(NAME Opt)
  set(SRCS Opt.cpp)

  add_executable(${NAME} ${SRCS})
  target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

  target_link_libraries(${NAME} FEXCore Common pthread)
endif()

add_subdirectory(FEXLoader/)
