cmake_minimum_required(VERSION 3.14)

set(GENERATOR_EXE "$<TARGET_FILE:thunkgenlib>")
set(THUNKS_TARGET Host)
set(TARGET_TYPE SHARED)

set(CMAKE_CXX_STANDARD 17)
set (HOSTLIBS_DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/fex-emu" CACHE PATH "global data directory")

include("${CMAKE_CURRENT_SOURCE_DIR}/../include/common/helpers.cmake")

foreach(GUEST_ARCH ${GuestArchList})
  set(THUNKS_ARCH_TARGET "host-thunks-${GUEST_ARCH}")

  project(${THUNKS_ARCH_TARGET})
  add_custom_target(${THUNKS_ARCH_TARGET})
  
  add_dependencies(host-thunks ${THUNKS_ARCH_TARGET})

  ##
  ### Host Thunk Libs
  ##


  #add_thunk_lib(fex_malloc_symbols)

  #generate(fex_malloc function_unpacks tab_function_unpacks ldr ldr_ptrs)
  #add_thunk_lib(fex_malloc)

  generate(asound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(asound)

  generate(EGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(EGL)

  find_package(OpenGL REQUIRED)
  generate(GL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(GL)
  target_link_libraries(${TARGET_DEPS} INTERFACE OpenGL::GL)

  # disabled for now, headers are platform specific
  # find_package(SDL2 REQUIRED)
  # generate(SDL2 function_unpacks tab_function_unpacks ldr ldr_ptrs)
  # add_thunk_lib(SDL2)
  # target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

  find_package(PkgConfig)
  pkg_search_module(X11 REQUIRED x11)

  string(REGEX MATCH "([0-9]*)\.([0-9]*)\.([0-9]*)" _ "${X11_VERSION}")
  set(X11_VERSION_MAJOR ${CMAKE_MATCH_1})
  set(X11_VERSION_MINOR ${CMAKE_MATCH_2})
  set(X11_VERSION_PATCH ${CMAKE_MATCH_3})

  generate(X11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})
  add_thunk_lib(X11)

  generate(Xext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
  target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})
  add_thunk_lib(Xext)

  generate(Xrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(Xrender)

  generate(Xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(Xfixes)

  generate(vulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs symbol_list)
  target_include_directories(${TARGET_DEPS} INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/Vulkan-Headers/include/)
  add_thunk_lib(vulkan)

  generate(xcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb)

  generate(xcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-dri2)

  generate(xcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-dri3)

  generate(xcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-xfixes)

  generate(xcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-shm)

  generate(xcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-sync)

  generate(xcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-present)

  generate(xcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-randr)

  generate(xcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xcb-glx)

  generate(xshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  add_thunk_lib(xshmfence)

  generate(drm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
  target_include_directories(${TARGET_DEPS} INTERFACE /usr/include/drm/ /usr/include/libdrm/)
  add_thunk_lib(drm)
endforeach()