cmake_minimum_required(VERSION 3.14)
project(host-thunks)

set(CMAKE_CXX_STANDARD 17)
set (HOSTLIBS_DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/fex-emu" CACHE PATH "global data directory")
option(ENABLE_CLANG_THUNKS "Enable building thunks with clang" FALSE)

if (ENABLE_CLANG_THUNKS)
  set (LD_OVERRIDE "-fuse-ld=lld")
  add_link_options(${LD_OVERRIDE})
endif()

# Syntax: generate(libxyz libxyz-interface.cpp)
# This defines two targets and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-interface: Target for IDE integration (making sure libxyz-interface.cpp shows up as a source file in the project tree)
# - libxyz-deps: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate NAME SOURCE_FILE)
  # Interface target for the user to add include directories
  add_library(${NAME}-deps INTERFACE)
  target_include_directories(${NAME}-deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  target_link_libraries(${NAME}-deps INTERFACE FEXLoader)
  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(prop "$<TARGET_PROPERTY:${NAME}-deps,INTERFACE_INCLUDE_DIRECTORIES>")
  set(compile_prop "$<TARGET_PROPERTY:${NAME}-deps,INTERFACE_COMPILE_DEFINITIONS>")
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    list(APPEND compile_prop _M_X86_64=1)
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    list(APPEND compile_prop _M_ARM_64=1)
  endif()

  # Target for IDE integration
  add_library(${NAME}-interface EXCLUDE_FROM_ALL ${SOURCE_FILE})
  target_link_libraries(${NAME}-interface PRIVATE ${NAME}-deps)

  # Run thunk generator for each of the given output files
  set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen")
  set(OUTFILE "${OUTFOLDER}/thunkgen_host_${NAME}.inl")

  file(MAKE_DIRECTORY "${OUTFOLDER}")

  add_custom_command(
    OUTPUT "${OUTFILE}"
    DEPENDS "${SOURCE_FILE}"
    DEPENDS thunkgen
    COMMAND thunkgen "${SOURCE_FILE}" "${NAME}" "-host" "${OUTFILE}" -- -std=c++17
      # Expand compile definitions to space-separated list of -D parameters
      "$<$<BOOL:${compile_prop}>:;-D$<JOIN:${compile_prop},;-D>>"
      # Expand include directories to space-separated list of -isystem parameters
      "$<$<BOOL:${prop}>:;-isystem$<JOIN:${prop},;-isystem>>"
    VERBATIM
    COMMAND_EXPAND_LISTS
    )

  list(APPEND OUTPUTS "${OUTFILE}")
  set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

function(add_host_lib NAME)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Host lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${NAME}-host SHARED ${SOURCE_FILE} ${GEN_lib${NAME}})
  target_include_directories(${NAME}-host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/")
  target_link_libraries(${NAME}-host PRIVATE dl)
  target_link_libraries(${NAME}-host PRIVATE lib${NAME}-deps)
  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${NAME}-host PRIVATE -fwrapv)

  # generated files forward-declare functions that need to be implemented manually, so pass --no-undefined to make sure errors are detected at compile-time rather than runtime
  target_link_options(${NAME}-host PRIVATE "LINKER:--no-undefined")

  install(TARGETS ${NAME}-host DESTINATION ${HOSTLIBS_DATA_DIRECTORY}/HostThunks/)
endfunction()

#add_host_lib(fex_malloc_symbols)

#generate(libfex_malloc)
#add_host_lib(fex_malloc)

generate(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp)
add_host_lib(asound)

generate(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp)
add_host_lib(EGL)

generate(libGL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp)
add_host_lib(GL)

find_package(OpenGL REQUIRED)
target_link_libraries(GL-host PRIVATE OpenGL::GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2)
# add_host_lib(SDL2)
# target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

find_package(PkgConfig)
pkg_search_module(X11 REQUIRED x11)

string(REGEX MATCH "([0-9]*)\.([0-9]*)\.([0-9]*)" _ "${X11_VERSION}")
set(X11_VERSION_MAJOR ${CMAKE_MATCH_1})
set(X11_VERSION_MINOR ${CMAKE_MATCH_2})
set(X11_VERSION_PATCH ${CMAKE_MATCH_3})

generate(libX11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp)
add_host_lib(X11)

target_compile_definitions(libX11-deps INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
target_compile_definitions(libX11-deps INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
target_compile_definitions(libX11-deps INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})

generate(libXext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp)
add_host_lib(Xext)

target_compile_definitions(libXext-deps INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
target_compile_definitions(libXext-deps INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
target_compile_definitions(libXext-deps INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})

generate(libXrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp)
add_host_lib(Xrender)

generate(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp)
add_host_lib(Xfixes)

generate(libvulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp)
target_include_directories(libvulkan-deps INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/Vulkan-Headers/include/)
add_host_lib(vulkan)

generate(libxcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp)
add_host_lib(xcb)

generate(libxcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp)
add_host_lib(xcb-dri2)

generate(libxcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp)
add_host_lib(xcb-dri3)

generate(libxcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp)
add_host_lib(xcb-xfixes)

generate(libxcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp)
add_host_lib(xcb-shm)

generate(libxcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp)
add_host_lib(xcb-sync)

generate(libxcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp)
add_host_lib(xcb-present)

generate(libxcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp)
add_host_lib(xcb-randr)

generate(libxcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp)
add_host_lib(xcb-glx)

generate(libxshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp)
add_host_lib(xshmfence)

generate(libOpenCL ${CMAKE_CURRENT_SOURCE_DIR}/../libOpenCL/libOpenCL_interface.cpp)
target_include_directories(libOpenCL-deps INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/OpenCL-Headers/)
add_host_lib(OpenCL)

generate(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp)
target_include_directories(libdrm-deps INTERFACE /usr/include/drm/)
target_include_directories(libdrm-deps INTERFACE /usr/include/libdrm/)
add_host_lib(drm)
