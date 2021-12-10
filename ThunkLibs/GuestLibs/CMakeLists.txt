cmake_minimum_required(VERSION 3.14)
project(guest-thunks)

set(CMAKE_CXX_STANDARD 17)

# These get passed in from the main cmake project
set (X86_C_COMPILER "x86_64-linux-gnu-gcc" CACHE STRING "c compiler for compiling x86 guest libs")
set (X86_CXX_COMPILER "x86_64-linux-gnu-g++" CACHE STRING "c++ compiler for compiling x86 guest libs")
set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/share/fex-emu" CACHE PATH "global data directory")

set(CMAKE_C_COMPILER "${X86_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${X86_CXX_COMPILER}")

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

# Syntax: generate(libxyz [LIBNAME libxyz-custom] libxyz-interface.cpp generator-targets...)
# This defines two targets and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-interface: Target for IDE integration (making sure libxyz-interface.cpp shows up as a source file in the project tree)
# - libxyz-deps: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate_new NAME)
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
      DEPENDS "${GENERATOR_EXE}"
      DEPENDS "${OUTFOLDER}"
      DEPENDS "${SOURCE_FILE}"
      COMMAND "${GENERATOR_EXE}" "${SOURCE_FILE}" "${LIBNAME}" "-${WHAT}" "${OUTFILE}" -- -std=c++17
            # Expand include directories to space-separated list of -isystem parameters
           "$<$<BOOL:${prop}>:;-isystem$<JOIN:${prop},;-isystem>>"
      VERBATIM
      COMMAND_EXPAND_LISTS
      )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  set(GEN_${LIBNAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

add_custom_target(ThunkGuestsInstall)

function(add_guest_lib_with_name NAME LIBNAME)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Guest.cpp)
  get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Guest.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Guest lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${LIBNAME}-guest SHARED ${SOURCE_FILE} ${GEN_lib${LIBNAME}})
  target_include_directories(${LIBNAME}-guest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${LIBNAME}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  target_compile_definitions(${LIBNAME}-guest PRIVATE GUEST_THUNK_LIBRARY)
  if (TARGET lib${LIBNAME}-deps)
    target_link_libraries(${LIBNAME}-guest PRIVATE lib${LIBNAME}-deps)
  endif()

  ## Tell GCC to not complain about ignored attributes
  target_compile_options(${LIBNAME}-guest PRIVATE -Wno-attributes)
  target_compile_options(${LIBNAME}-guest PRIVATE -DLIB_NAME=${LIBNAME} -DLIBLIB_NAME=lib${LIBNAME})

  add_custom_target(${LIBNAME}-guest-install
    COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{DESTDIR}/${DATA_DIRECTORY}/GuestThunks/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib${LIBNAME}-guest.so $ENV{DESTDIR}/${DATA_DIRECTORY}/GuestThunks/)
  add_dependencies(ThunkGuestsInstall ${LIBNAME}-guest-install)
endfunction()

function(add_guest_lib NAME)
  add_guest_lib_with_name(${NAME} ${NAME} ${ARGV})
endfunction()

#add_guest_lib(fex_malloc_loader)
#target_link_libraries(fex_malloc_loader-guest PRIVATE dl)

#generate(libfex_malloc thunks function_packs function_packs_public)
#add_guest_lib(fex_malloc)

generate_new(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(asound)

generate_new(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(EGL)

generate_new(libGL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 thunks function_packs function_packs_public)
# add_guest_lib(SDL2)
# target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(SDL2-guest PRIVATE GL)
# target_link_libraries(SDL2-guest PRIVATE dl)

generate_new(libX11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp thunks function_packs function_packs_public callback_structs callback_unpacks_header callback_unpacks_header_init callback_unpacks callback_typedefs)
add_guest_lib(X11)

generate_new(libXext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xext)

generate_new(libXrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xrender)

generate_new(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xfixes)

set (VULKAN_LIBS
  vulkan_radeon
  vulkan_lvp
  vulkan_freedreno
  vulkan_intel
  vulkan_panfrost
  vulkan_virtio
)

foreach (LIB IN LISTS VULKAN_LIBS)
  generate_new(libvulkan_device LIBNAME lib${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan_device/libvulkan_device_interface.cpp thunks function_packs function_packs_public symbol_list)
  add_guest_lib_with_name(vulkan_device ${LIB})
endforeach()

generate_new(libxcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp thunks function_packs function_packs_public callback_structs callback_unpacks_header callback_unpacks_header_init callback_unpacks callback_typedefs)
add_guest_lib(xcb)

generate_new(libxcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-dri2)

generate_new(libxcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-dri3)

generate_new(libxcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-xfixes)

generate_new(libxcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-shm)

generate_new(libxcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-sync)

generate_new(libxcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-present)

generate_new(libxcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-randr)

generate_new(libxcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-glx)

generate_new(libxshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xshmfence)

generate_new(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp thunks function_packs function_packs_public)
target_include_directories(libdrm-deps INTERFACE /usr/include/drm/)
target_include_directories(libdrm-deps INTERFACE /usr/include/libdrm/)
add_guest_lib(drm)
