cmake_minimum_required(VERSION 3.14)
project(host-thunks)
include(${FEX_PROJECT_SOURCE_DIR}/Data/CMake/version_to_variables.cmake)

set(CMAKE_CXX_STANDARD 20)
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
function(generate NAME SOURCE_FILE GUEST_BITNESS)
  # Interface target for the user to add include directories
  add_library(${NAME}-${GUEST_BITNESS}-deps INTERFACE)
  target_include_directories(${NAME}-${GUEST_BITNESS}-deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  if (GUEST_BITNESS EQUAL 32)
    target_compile_definitions(${NAME}-${GUEST_BITNESS}-deps INTERFACE IS_32BIT_THUNK)
  endif ()
  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(prop "$<TARGET_PROPERTY:${NAME}-${GUEST_BITNESS}-deps,INTERFACE_INCLUDE_DIRECTORIES>")
  set(compile_prop "$<TARGET_PROPERTY:${NAME}-${GUEST_BITNESS}-deps,INTERFACE_COMPILE_DEFINITIONS>")
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    list(APPEND compile_prop _M_X86_64=1)
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    list(APPEND compile_prop _M_ARM_64=1)
  endif()

  # Target for IDE integration
  add_library(${NAME}-${GUEST_BITNESS}-interface EXCLUDE_FROM_ALL ${SOURCE_FILE})
  target_link_libraries(${NAME}-${GUEST_BITNESS}-interface PRIVATE ${NAME}-${GUEST_BITNESS}-deps)

  # Run thunk generator for each of the given output files
  set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen_${GUEST_BITNESS}")
  set(OUTFILE "${OUTFOLDER}/thunkgen_host_${NAME}.inl")

  file(MAKE_DIRECTORY "${OUTFOLDER}")

  set (BITNESS_FLAGS "")
  if (GUEST_BITNESS EQUAL 32)
    set (BITNESS_FLAGS "-for-32bit-guest")
  endif()

  add_custom_command(
    OUTPUT "${OUTFILE}"
    DEPENDS "${SOURCE_FILE}"
    DEPENDS thunkgen
    COMMAND thunkgen "${SOURCE_FILE}" "${NAME}" "-host" "${OUTFILE}" "${X86_DEV_ROOTFS}" ${BITNESS_FLAGS} -- -std=c++20
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

function(add_host_lib NAME GUEST_BITNESS)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Host.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Host lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${NAME}-host-${GUEST_BITNESS} SHARED ${SOURCE_FILE} ${GEN_lib${NAME}})
  set_target_properties(${NAME}-host-${GUEST_BITNESS} PROPERTIES OUTPUT_NAME "${NAME}-host")
  set_target_properties(${NAME}-host-${GUEST_BITNESS} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/HostLibs_${GUEST_BITNESS}")
  target_include_directories(${NAME}-host-${GUEST_BITNESS} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen_${GUEST_BITNESS}/")
  target_link_libraries(${NAME}-host-${GUEST_BITNESS} PRIVATE dl)
  target_link_libraries(${NAME}-host-${GUEST_BITNESS} PRIVATE lib${NAME}-${GUEST_BITNESS}-deps)
  target_link_libraries(${NAME}-host-${GUEST_BITNESS} PRIVATE FEXLoader)
  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${NAME}-host-${GUEST_BITNESS} PRIVATE -fwrapv)

  if (NOT ENABLE_ASAN)
    # generated files forward-declare functions that need to be implemented manually, so pass --no-undefined to make sure errors are detected at compile-time rather than runtime
    # NOTE: ASan is not compatible with --no-undefined, see https://github.com/google/sanitizers/issues/380 for details
    target_link_options(${NAME}-host-${GUEST_BITNESS} PRIVATE "LINKER:--no-undefined")
  endif()

  if (${GUEST_BITNESS} EQUAL 32)
    install(TARGETS ${NAME}-host-${GUEST_BITNESS} DESTINATION ${HOSTLIBS_DATA_DIRECTORY}/HostThunks_32/)
  else()
    install(TARGETS ${NAME}-host-${GUEST_BITNESS} DESTINATION ${HOSTLIBS_DATA_DIRECTORY}/HostThunks/)
  endif()
endfunction()

set (BITNESS_LIST "64")
foreach(GUEST_BITNESS IN LISTS BITNESS_LIST)
  #add_host_lib(fex_malloc_symbols ${GUEST_BITNESS})

  #generate(libfex_malloc)
  #add_host_lib(fex_malloc ${GUEST_BITNESS})

  generate(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp ${GUEST_BITNESS})
  add_host_lib(asound ${GUEST_BITNESS})

  # disabled for now, headers are platform specific
  # find_package(SDL2 REQUIRED)
  # generate(libSDL2)
  # add_host_lib(SDL2 ${GUEST_BITNESS})
  # target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

  generate(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp ${GUEST_BITNESS})
  target_include_directories(libdrm-${GUEST_BITNESS}-deps INTERFACE /usr/include/drm/)
  target_include_directories(libdrm-${GUEST_BITNESS}-deps INTERFACE /usr/include/libdrm/)
  add_host_lib(drm ${GUEST_BITNESS})
endforeach()

set (BITNESS_LIST "32;64")
foreach(GUEST_BITNESS IN LISTS BITNESS_LIST)
  generate(libfex_thunk_test ${CMAKE_CURRENT_SOURCE_DIR}/../libfex_thunk_test/libfex_thunk_test_interface.cpp ${GUEST_BITNESS})
  add_host_lib(fex_thunk_test ${GUEST_BITNESS})

  generate(libvulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp ${GUEST_BITNESS})
  target_include_directories(libvulkan-${GUEST_BITNESS}-deps INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/Vulkan-Headers/include/)
  add_host_lib(vulkan ${GUEST_BITNESS})

  generate(libwayland-client ${CMAKE_CURRENT_SOURCE_DIR}/../libwayland-client/libwayland-client_interface.cpp ${GUEST_BITNESS})
  add_host_lib(wayland-client ${GUEST_BITNESS})
  target_include_directories(libwayland-client-${GUEST_BITNESS}-deps INTERFACE /usr/include/wayland)

  generate(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp ${GUEST_BITNESS})
  add_host_lib(EGL ${GUEST_BITNESS})

  generate(libGL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp ${GUEST_BITNESS})
  add_host_lib(GL ${GUEST_BITNESS})

  find_package(OpenGL REQUIRED)
  target_link_libraries(GL-host-${GUEST_BITNESS} PRIVATE OpenGL::GL)
endforeach()

add_library(fex_thunk_test SHARED ../libfex_thunk_test/lib.cpp)
install(TARGETS fex_thunk_test LIBRARY DESTINATION lib COMPONENT TestLibraries)
