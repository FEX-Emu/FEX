cmake_minimum_required(VERSION 3.14)
project(host-thunks)

set(CMAKE_CXX_STANDARD 17)
set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/fex-emu" CACHE PATH "global data directory")

function(generate_with_name NAME LIBNAME)
  foreach(WHAT IN LISTS ARGN)
    set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen/${LIBNAME}")
    set(OUTFILE "${OUTFOLDER}/${WHAT}.inl")
    set(GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/../Generators/${NAME}.py")
    set(HELPER "${CMAKE_CURRENT_SOURCE_DIR}/../Generators/ThunkHelpers.py")

    add_custom_command(OUTPUT ${OUTFOLDER}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTFOLDER}")

    add_custom_command(
      OUTPUT "${OUTFILE}"
      DEPENDS "${GENERATOR}"
      DEPENDS "${HELPER}"
      DEPENDS "${OUTFOLDER}"
      COMMAND "python3" "${GENERATOR}" ${WITH_ARGS} "${WHAT}" > "${OUTFILE}"
      )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  if (DEFINED GEN_${LIBNAME})
    list(APPEND OUTPUTS "${GEN_${LIBNAME}}")
    set(GEN_${LIBNAME} ${OUTPUTS} PARENT_SCOPE)
  else()
    set(GEN_${LIBNAME} ${OUTPUTS} PARENT_SCOPE)
  endif()
endfunction()

function(generate NAME)
  generate_with_name(${NAME} ${NAME} ${ARGV})
  # Pass the details up the stack
  if (DEFINED GEN_${NAME})
    list(APPEND OUTPUTS "${GEN_${NAME}}")
    set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
  else()
    set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
  endif()
endfunction()

# Syntax: generate(libxyz libxyz-interface.cpp generator-targets...)
# This defines a target and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-interface: Target for IDE integration (making sure libxyz-interface.cpp shows up as a source file in the project tree)
function(generate_new NAME)
  list(POP_FRONT ARGN SOURCE_FILE)

  # Target for IDE integration
  add_library(${NAME}-interface EXCLUDE_FROM_ALL ${SOURCE_FILE})

  # Run thunk generator for each of the given output files
  foreach(WHAT IN LISTS ARGN)
    set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen/${NAME}")
    set(OUTFILE "${OUTFOLDER}/${WHAT}.inl")

    add_custom_command(OUTPUT ${OUTFOLDER}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTFOLDER}")

    add_custom_command(
      OUTPUT "${OUTFILE}"
      DEPENDS "${GENERATOR_EXE}"
      DEPENDS "${OUTFOLDER}"
      DEPENDS "${SOURCE_FILE}"
      COMMAND "${GENERATOR_EXE}" "${SOURCE_FILE}" "${NAME}" "-${WHAT}" "${OUTFILE}" -- -std=c++17 -I"${CMAKE_CURRENT_SOURCE_DIR}/../include"
      COMMAND_EXPAND_LISTS
      )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
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
  target_include_directories(${LIBNAME}-host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${LIBNAME}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  target_link_libraries(${LIBNAME}-host PRIVATE dl)
  target_compile_options(${LIBNAME}-host PRIVATE -DLIB_NAME=${LIBNAME} -DLIBLIB_NAME=lib${LIBNAME})

  # generated files forward-declare functions that need to be implemented manually, so pass --no-undefined to make sure errors are detected at compile-time rather than runtime
  target_link_options(${LIBNAME}-host PRIVATE "-Wl,--no-undefined")

  add_custom_target(${LIBNAME}-host-install
    COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{DESTDIR}/${DATA_DIRECTORY}/HostThunks/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib${LIBNAME}-host.so $ENV{DESTDIR}/${DATA_DIRECTORY}/HostThunks/)
  add_dependencies(ThunkHostsInstall ${LIBNAME}-host-install)
endfunction()

add_custom_target(ThunkHostsInstall)

function(add_host_lib NAME)
  add_host_lib_with_name(${NAME} ${NAME} ${ARGV})
endfunction()

#add_host_lib(fex_malloc_symbols)

#generate(libfex_malloc function_unpacks tab_function_unpacks ldr ldr_ptrs)
#add_host_lib(fex_malloc)

generate_new(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(asound)

generate(libEGL function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(EGL)

generate(libGL function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(GL)

find_package(OpenGL REQUIRED)
target_link_libraries(GL-host PRIVATE OpenGL::GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 function_unpacks tab_function_unpacks ldr ldr_ptrs)
# add_host_lib(SDL2)
# target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

generate(libX11 function_unpacks tab_function_unpacks ldr ldr_ptrs callback_packs callback_structs callback_unpacks_header callback_typedefs)
add_host_lib(X11)

generate(libXext function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xext)

generate(libXrender function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xrender)

generate_new(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xfixes)

set (VULKAN_LIBS
  vulkan_radeon
  vulkan_lvp
  vulkan_freedreno
  vulkan_intel
  vulkan_panfrost
  vulkan_virtio
)

foreach (LIB IN LISTS VULKAN_LIBS)
  set(WITH_ARGS "lib${LIB}" "${VULKAN_XML}" "--print-header")
  generate_with_name(libvulkan_device lib${LIB} Header)
  set(WITH_ARGS "")

  set(WITH_ARGS "lib${LIB}" "${VULKAN_XML}" "--print-thunks")
  generate_with_name(libvulkan_device lib${LIB} function_unpacks tab_function_unpacks ldr ldr_ptrs ldr_ptrs_pair)
  add_host_lib_with_name(vulkan_device ${LIB})
  set(WITH_ARGS "")
endforeach()

generate(libxcb function_unpacks tab_function_unpacks ldr ldr_ptrs callback_packs callback_structs callback_unpacks_header callback_typedefs)
add_host_lib(xcb)

generate(libxcb_dri2 function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_dri2)

generate(libxcb_dri3 function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_dri3)

generate(libxcb_xfixes function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_xfixes)

generate(libxcb_shm function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_shm)

generate(libxcb_sync function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_sync)

generate(libxcb_present function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_present)

generate(libxcb_randr function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_randr)

generate(libxcb_glx function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xcb_glx)

generate(libxshmfence function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(xshmfence)

generate(libdrm function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(drm)
target_include_directories(drm-host PRIVATE /usr/include/drm/)
target_include_directories(drm-host PRIVATE /usr/include/libdrm/)
