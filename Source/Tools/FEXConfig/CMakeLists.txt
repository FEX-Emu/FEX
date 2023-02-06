set(NAME FEXConfig)
set(SRCS Main.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_sdl.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_opengl3.cpp)

find_library(EPOXY epoxy REQUIRED)
find_package(SDL2 REQUIRED)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>)
add_executable(${NAME} ${SRCS})

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/imgui/examples/)

# Fix for SDL2 includes under Alpine Linux.
target_include_directories(${NAME} PRIVATE /usr/include/directfb/)

if (TARGET SDL2::SDL2)
  target_link_libraries(${NAME} PRIVATE SDL2::SDL2)
else()
  target_include_directories(${NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(${NAME} PRIVATE ${SDL2_LIBRARIES})
endif()

target_link_libraries(${NAME} PRIVATE Common pthread epoxy X11 EGL imgui json-maker)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(${NAME}
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)
