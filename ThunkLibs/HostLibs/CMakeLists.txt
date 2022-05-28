cmake_minimum_required(VERSION 3.14)
project(host-thunks)

set(CMAKE_CXX_STANDARD 17)
set (HOSTLIBS_DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/fex-emu" CACHE PATH "global data directory")

# Syntax: generate(libxyz [LIBNAME libxyz-custom] libxyz-interface.cpp generator-targets...)
# This defines two targets and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-interface: Target for IDE integration (making sure libxyz-interface.cpp shows up as a source file in the project tree)
# - libxyz-deps: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate NAME)
  cmake_parse_arguments(PARSE_ARGV 1 ARGS "" "LIBNAME" "")
  set(ARGN ${ARGS_UNPARSED_ARGUMENTS})
  list(POP_FRONT ARGN SOURCE_FILE)

  if (ARGS_LIBNAME)
    set(LIBNAME ${ARGS_LIBNAME})
  else()
    set(LIBNAME ${NAME})
  endif()

  # Interface target for the user to add include directories
  add_library(${LIBNAME}-deps INTERFACE)
  target_include_directories(${LIBNAME}-deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(prop "$<TARGET_PROPERTY:${LIBNAME}-deps,INTERFACE_INCLUDE_DIRECTORIES>")

  # Target for IDE integration
  add_library(${LIBNAME}-interface EXCLUDE_FROM_ALL ${SOURCE_FILE})
  target_link_libraries(${LIBNAME}-interface PRIVATE ${LIBNAME}-deps)

  # Run thunk generator for each of the given output files
  foreach(WHAT IN LISTS ARGN)
    set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen/${LIBNAME}")
    set(OUTFILE "${OUTFOLDER}/${WHAT}.inl")

    add_custom_command(OUTPUT ${OUTFOLDER}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTFOLDER}")

    add_custom_command(
      OUTPUT "${OUTFILE}"
      DEPENDS "${OUTFOLDER}"
      DEPENDS "${SOURCE_FILE}"
      DEPENDS thunkgen
      COMMAND thunkgen "${SOURCE_FILE}" "${LIBNAME}" "-${WHAT}" "${OUTFILE}" -- -std=c++17
            # Expand include directories to space-separated list of -isystem parameters
           "$<$<BOOL:${prop}>:;-isystem$<JOIN:${prop},;-isystem>>"
      VERBATIM
      COMMAND_EXPAND_LISTS
      )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  set(GEN_${LIBNAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

function(add_host_lib_with_name NAME LIBNAME)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Host lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${LIBNAME}-host SHARED ${SOURCE_FILE} ${GEN_lib${LIBNAME}})
  set_property(TARGET ${LIBNAME}-host PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_include_directories(${LIBNAME}-host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${LIBNAME}")
  target_link_libraries(${LIBNAME}-host PRIVATE dl)
  target_link_libraries(${LIBNAME}-host PRIVATE lib${LIBNAME}-deps)
  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${LIBNAME}-host PRIVATE -fwrapv)
  target_compile_options(${LIBNAME}-host PRIVATE -DLIB_NAME=${LIBNAME} -DLIBLIB_NAME=lib${LIBNAME})

  # generated files forward-declare functions that need to be implemented manually, so pass --no-undefined to make sure errors are detected at compile-time rather than runtime
  target_link_options(${LIBNAME}-host PRIVATE "LINKER:--no-undefined")

  install(TARGETS ${LIBNAME}-host DESTINATION ${HOSTLIBS_DATA_DIRECTORY}/HostThunks/)
endfunction()

function(add_host_lib NAME)
  add_host_lib_with_name(${NAME} ${NAME} ${ARGV})
endfunction()

#add_host_lib(fex_malloc_symbols)

#generate(libfex_malloc function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(fex_malloc)

#generate(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(asound)

#generate(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(EGL)

generate(libFGL ${CMAKE_CURRENT_SOURCE_DIR}/../libFGL/libFGL_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(FGL)
target_link_libraries(FGL-host PRIVATE X11)
target_link_libraries(FGL-host PRIVATE GLX)
target_link_libraries(FGL-host PRIVATE EGL)

find_package(OpenGL REQUIRED)
target_link_libraries(FGL-host PRIVATE OpenGL::GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 function_unpacks tab_function_unpacks ldr ldr_ptrs)
# add_host_lib(SDL2)
# target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

#generate(libX11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs callback_structs callback_unpacks_header callback_typedefs)
#add_host_lib(X11)

#generate(libXext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(Xext)

#generate(libXrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(Xrender)

#generate(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(Xfixes)

set (VULKAN_LIBS
  vulkan_radeon
  vulkan_lvp
  vulkan_freedreno
  vulkan_intel
  vulkan_panfrost
  vulkan_virtio
)

foreach (LIB IN LISTS VULKAN_LIBS)
#  generate(libvulkan_device LIBNAME lib${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan_device/libvulkan_device_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs symbol_list)
#  add_host_lib_with_name(vulkan_device ${LIB})
endforeach()

#generate(libxcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs callback_structs callback_unpacks_header callback_typedefs)
#add_host_lib(xcb)

#generate(libxcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-dri2)

#generate(libxcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-dri3)

#generate(libxcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-xfixes)

#generate(libxcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-shm)

#generate(libxcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-sync)

#generate(libxcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-present)

#generate(libxcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-randr)

#generate(libxcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xcb-glx)

#generate(libxshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(xshmfence)

#generate(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
#target_include_directories(libdrm-deps INTERFACE /usr/include/drm/)
#target_include_directories(libdrm-deps INTERFACE /usr/include/libdrm/)
#add_host_lib(drm)
