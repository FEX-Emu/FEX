if (ENABLE_VISUAL_DEBUGGER)
  add_subdirectory(Debugger/)
endif()

if (NOT TERMUX_BUILD)
  # Termux builds can't rely on X11 packages
  # SDL2 isn't even compiled with GL support so our GUIs wouldn't even work
  add_subdirectory(FEXConfig/)

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
