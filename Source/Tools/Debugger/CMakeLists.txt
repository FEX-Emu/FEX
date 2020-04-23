set(NAME Debugger)
set(SRCS Main.cpp
  DebuggerState.cpp
  Context.cpp
  FEXImGui.cpp
  IMGui_I.cpp
  IRLexer.cpp
  GLUtils.cpp
  MainWindow.cpp
  Disassembler.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_opengl3.cpp
  )

find_library(EPOXY_LIBRARY epoxy)
find_library(GLFW_LIBRARY glfw3)
find_package(LLVM CONFIG QUIET)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>)
add_executable(${NAME} ${SRCS})

target_link_libraries(${NAME} PRIVATE LLVM)
target_include_directories(${NAME} PRIVATE ${LLVM_INCLUDE_DIRS})

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/SonicUtils/)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/imgui/examples/)

target_link_libraries(${NAME} PRIVATE FEXCore Common CommonCore SonicUtils pthread LLVM epoxy glfw X11 EGL imgui tiny-json json-maker)
