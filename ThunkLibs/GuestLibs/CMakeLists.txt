cmake_minimum_required(VERSION 3.14)

set(THUNKS_TARGET Guest)

set(TARGET_TYPE SHARED)
set(GENERATE_INSTALL_TARGETS TRUE)

set(CMAKE_CXX_STANDARD 17)
set(DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/fex-emu" CACHE PATH "global data directory")

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # We've been included using add_subdirectory, so set up targets for IDE integration using the host toolchain
  set(GUEST_ARCH "none")
  set(GENERATOR_TARGET "thunkgenlib")
  set(GENERATOR_EXE "$<TARGET_FILE:${GENERATOR_TARGET}>")

  set(TARGET_TYPE OBJECT)
  set(GENERATE_INSTALL_TARGETS FALSE)

  set(THUNKS_ARCH_TARGET "guest-thunks-${GUEST_ARCH}")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../include/common/helpers.cmake")

project(${THUNKS_ARCH_TARGET})
add_custom_target(${THUNKS_ARCH_TARGET})

#add_thunk_lib(fex_malloc_loader)
#target_link_libraries(fex_malloc_loader-guest PRIVATE dl)

#generate(fex_malloc thunks function_packs function_packs_public)
#add_thunk_lib(fex_malloc)

generate(asound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(asound)


# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(SDL2 thunks function_packs function_packs_public)
# add_thunk_lib(SDL2)
# target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(SDL2-guest PRIVATE GL)
# target_link_libraries(SDL2-guest PRIVATE dl)

find_package(PkgConfig)
pkg_search_module(X11 REQUIRED x11)

string(REGEX MATCH "([0-9]*)\.([0-9]*)\.([0-9]*)" _ "${X11_VERSION}")
set(X11_VERSION_MAJOR ${CMAKE_MATCH_1})
set(X11_VERSION_MINOR ${CMAKE_MATCH_2})
set(X11_VERSION_PATCH ${CMAKE_MATCH_3})


generate(X11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp thunks function_packs function_packs_public)
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})
add_thunk_lib(X11)


generate(GL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp thunks function_packs function_packs_public symbol_list)
add_thunk_lib(GL)
# libGL must pull in libX11.so. This is normally done indirectly via libGLX.
target_link_libraries(${TARGET_NAME} PRIVATE ${X11_TARGET_NAME})

generate(EGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(EGL)
# ELG uses glxGetProcAddress for now
target_link_libraries(${TARGET_NAME} PRIVATE ${GL_TARGET_NAME})

generate(Xext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp thunks function_packs function_packs_public)
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
target_compile_definitions(${TARGET_DEPS} INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})
add_thunk_lib(Xext)

generate(Xrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(Xrender)

generate(Xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(Xfixes)

generate(vulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp thunks function_packs function_packs_public symbol_list)
target_include_directories(${TARGET_DEPS} INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/Vulkan-Headers/include/)
add_thunk_lib(vulkan)

generate(xcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb)

generate(xcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-dri2)

generate(xcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-dri3)

generate(xcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-xfixes)

generate(xcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-shm)

generate(xcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-sync)

generate(xcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-present)

generate(xcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-randr)

generate(xcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xcb-glx)

generate(xshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp thunks function_packs function_packs_public)
add_thunk_lib(xshmfence)

generate(drm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp thunks function_packs function_packs_public)
target_include_directories(${TARGET_DEPS} INTERFACE /usr/include/drm/)
target_include_directories(${TARGET_DEPS} INTERFACE /usr/include/libdrm/)
add_thunk_lib(drm)
