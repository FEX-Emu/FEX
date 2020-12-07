set(NAME FEXConfig)
set(SRCS Main.cpp
  ../Debugger/Context.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_opengl3.cpp
  )

find_library(EPOXY_LIBRARY epoxy)
find_library(GLFW_LIBRARY glfw3)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>)
add_executable(${NAME} ${SRCS})

target_include_directories(${NAME} PRIVATE ${LLVM_INCLUDE_DIRS})

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Tools/Debugger/)

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/imgui/examples/)

target_link_libraries(${NAME} PRIVATE FEXCore Common CommonCore pthread epoxy glfw X11 EGL imgui tiny-json json-maker)

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)
