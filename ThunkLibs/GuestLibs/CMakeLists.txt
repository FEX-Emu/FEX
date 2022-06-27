cmake_minimum_required(VERSION 3.14)
project(guest-thunks)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # We've been included using ExternalProject_add, so set up the actual thunk libraries to be cross-compiled
  set(CMAKE_CXX_STANDARD 17)

  # This gets passed in from the main cmake project
  set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/share/fex-emu" CACHE PATH "global data directory")

  set(TARGET_TYPE SHARED)
  set(GENERATE_GUEST_INSTALL_TARGETS TRUE)
else()
  # We've been included using add_subdirectory, so set up targets for IDE integration using the host toolchain
  set(GENERATOR_EXE thunkgen)
  set(TARGET_TYPE OBJECT)
  set(GENERATE_GUEST_INSTALL_TARGETS FALSE)
endif()

# Syntax: generate(libxyz libxyz-interface.cpp generator-targets...)
# This defines a target and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-guest-deps: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate NAME SOURCE_FILE)
  # Interface target for the user to add include directories
  add_library(${NAME}-guest-deps INTERFACE)
  target_include_directories(${NAME}-guest-deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(prop "$<TARGET_PROPERTY:${NAME}-guest-deps,INTERFACE_INCLUDE_DIRECTORIES>")

  # Run thunk generator for each of the given output files
  foreach(WHAT IN LISTS ARGN)
    set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen/${NAME}")
    set(OUTFILE "${OUTFOLDER}/${WHAT}.inl")

    file(MAKE_DIRECTORY "${OUTFOLDER}")

    add_custom_command(
      OUTPUT "${OUTFILE}"
      DEPENDS "${GENERATOR_EXE}"
      DEPENDS "${SOURCE_FILE}"
      COMMAND "${GENERATOR_EXE}" "${SOURCE_FILE}" "${NAME}" "-${WHAT}" "${OUTFILE}" -- -std=c++17
            # Expand include directories to space-separated list of -isystem parameters
           "$<$<BOOL:${prop}>:;-isystem$<JOIN:${prop},;-isystem>>"
      VERBATIM
      COMMAND_EXPAND_LISTS
      )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

function(add_guest_lib NAME)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Guest.cpp)
  get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Guest.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Guest lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${NAME}-guest ${TARGET_TYPE} ${SOURCE_FILE} ${GEN_lib${NAME}})
  target_include_directories(${NAME}-guest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${NAME}")
  target_compile_definitions(${NAME}-guest PRIVATE GUEST_THUNK_LIBRARY)
  target_link_libraries(${NAME}-guest PRIVATE lib${NAME}-guest-deps)

  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${NAME}-guest PRIVATE -fwrapv)

  if (GENERATE_GUEST_INSTALL_TARGETS)
    install(TARGETS ${NAME}-guest DESTINATION ${DATA_DIRECTORY}/GuestThunks/)
  endif()
endfunction()

#add_guest_lib(fex_malloc_loader)
#target_link_libraries(fex_malloc_loader-guest PRIVATE dl)

#generate(libfex_malloc thunks function_packs function_packs_public)
#add_guest_lib(fex_malloc)

generate(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(asound)

generate(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(EGL)

generate(libGL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp thunks function_packs function_packs_public symbol_list)
add_guest_lib(GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 thunks function_packs function_packs_public)
# add_guest_lib(SDL2)
# target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(SDL2-guest PRIVATE GL)
# target_link_libraries(SDL2-guest PRIVATE dl)

generate(libX11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp thunks function_packs function_packs_public callback_structs callback_unpacks callback_typedefs)
add_guest_lib(X11)

generate(libXext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xext)

generate(libXrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xrender)

generate(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(Xfixes)

generate(libvulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp thunks function_packs function_packs_public symbol_list)
add_guest_lib(vulkan)

generate(libxcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp thunks function_packs function_packs_public callback_structs callback_unpacks callback_typedefs)
add_guest_lib(xcb)

generate(libxcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-dri2)

generate(libxcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-dri3)

generate(libxcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-xfixes)

generate(libxcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-shm)

generate(libxcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-sync)

generate(libxcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-present)

generate(libxcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-randr)

generate(libxcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xcb-glx)

generate(libxshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp thunks function_packs function_packs_public)
add_guest_lib(xshmfence)

generate(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp thunks function_packs function_packs_public)
target_include_directories(libdrm-guest-deps INTERFACE /usr/include/drm/)
target_include_directories(libdrm-guest-deps INTERFACE /usr/include/libdrm/)
add_guest_lib(drm)
