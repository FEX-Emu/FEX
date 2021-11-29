set(NAME FEXLogServer)
set(SRCS Main.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_sdl.cpp
  ${CMAKE_SOURCE_DIR}/External/imgui/examples/imgui_impl_opengl3.cpp)

find_library(EPOXY_LIBRARY epoxy)
find_package(SDL2 REQUIRED)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>)

add_executable(${NAME} ${SRCS})

list(APPEND LIBS FEXCore Common CommonCore epoxy X11 EGL imgui)

target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/imgui/examples/)

if (TARGET SDL2::SDL2)
  target_link_libraries(${NAME} PRIVATE SDL2::SDL2)
else()
  target_include_directories(${NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(${NAME} PRIVATE ${SDL2_LIBRARIES})
endif()


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

target_link_libraries(${NAME} PRIVATE ${LIBS} ${STATIC_PIE_OPTIONS} ${PTHREAD_LIB})

