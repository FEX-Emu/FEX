if (ENABLE_VISUAL_DEBUGGER)
  add_subdirectory(Debugger/)
endif()

add_subdirectory(FEXConfig/)
add_subdirectory(FEXGetConfig/)
add_subdirectory(FEXMountDaemon/)

set(NAME Opt)
set(SRCS Opt.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} FEXCore Common CommonCore pthread)
