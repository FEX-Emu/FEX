cmake_minimum_required(VERSION 3.14)
project(guest-thunks)
include(${FEX_PROJECT_SOURCE_DIR}/CMakeFiles/version_to_variables.cmake)

option(ENABLE_CLANG_THUNKS "Enable building thunks with clang" FALSE)

if (ENABLE_CLANG_THUNKS)
  set (LD_OVERRIDE "-fuse-ld=lld")
  add_link_options(${LD_OVERRIDE})
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "CCache enabled for guest thunks")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # We've been included using ExternalProject_add, so set up the actual thunk libraries to be cross-compiled
  set(CMAKE_CXX_STANDARD 20)

  # This gets passed in from the main cmake project
  set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/share/fex-emu" CACHE PATH "global data directory")

  set(TARGET_TYPE SHARED)
  set(GENERATE_GUEST_INSTALL_TARGETS TRUE)

  # uninstall target
  if(NOT TARGET uninstall)
    configure_file(
      "${FEX_PROJECT_SOURCE_DIR}/CMakeFiles/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cmake_uninstall.cmake"
      IMMEDIATE @ONLY)

    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cmake_uninstall.cmake)
  endif()
else()
  # We've been included using add_subdirectory, so set up targets for IDE integration using the host toolchain
  set(GENERATOR_EXE thunkgen)
  set(TARGET_TYPE OBJECT)
  set(GENERATE_GUEST_INSTALL_TARGETS FALSE)
  set(BITNESS 64)
endif()

# Syntax: generate(libxyz libxyz-interface.cpp)
# This defines a target and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - libxyz-guest-deps: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate NAME SOURCE_FILE)
  # Interface target for the user to add include directories
  add_library(${NAME}-guest-deps INTERFACE)
  target_include_directories(${NAME}-guest-deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  if (BITNESS EQUAL 32)
    target_compile_definitions(${NAME}-guest-deps INTERFACE IS_32BIT_THUNK)
  endif ()
  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(prop "$<TARGET_PROPERTY:${NAME}-guest-deps,INTERFACE_INCLUDE_DIRECTORIES>")
  set(compile_prop "$<TARGET_PROPERTY:${NAME}-guest-deps,INTERFACE_COMPILE_DEFINITIONS>")

  # Run thunk generator for each of the given output files
  set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen")
  set(OUTFILE "${OUTFOLDER}/thunkgen_guest_${NAME}.inl")

  file(MAKE_DIRECTORY "${OUTFOLDER}")

  if (BITNESS EQUAL 32)
    set(BITNESS_FLAGS "-for-32bit-guest")
    set(BITNESS_FLAGS2 "-m32" "--target=i686-linux-unknown" "-isystem" "/usr/i686-linux-gnu/include/")
  else()
    set(BITNESS_FLAGS "")
    set(BITNESS_FLAGS2 "--target=x86_64-linux-unknown" "-isystem" "/usr/x86_64-linux-gnu/include/")
  endif()

  add_custom_command(
    OUTPUT "${OUTFILE}"
    DEPENDS "${GENERATOR_EXE}"
    DEPENDS "${SOURCE_FILE}"
    COMMAND "${GENERATOR_EXE}" "${SOURCE_FILE}" "${NAME}" "-guest" "${OUTFILE}" ${BITNESS_FLAGS} -- -std=c++20 ${BITNESS_FLAGS2}
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

function(add_guest_lib NAME SONAME)
  set (SOURCE_FILE ../lib${NAME}/lib${NAME}_Guest.cpp)
  get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)

  set (SOURCE_LDS_FILE ../lib${NAME}/lib${NAME}_Guest.lds)
  get_filename_component(SOURCE_LDS_FILE_ABS "${SOURCE_LDS_FILE}" ABSOLUTE)

  set (SOURCE_LDS_32_FILE ../lib${NAME}/lib${NAME}_Guest_32.lds)
  get_filename_component(SOURCE_LDS_32_FILE_ABS "${SOURCE_LDS_32_FILE}" ABSOLUTE)

  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE ../lib${NAME}/Guest.cpp)
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for Guest lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${NAME}-guest ${TARGET_TYPE} ${SOURCE_FILE} ${GEN_lib${NAME}})
  target_include_directories(${NAME}-guest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/")
  target_compile_definitions(${NAME}-guest PRIVATE GUEST_THUNK_LIBRARY)
  target_link_libraries(${NAME}-guest PRIVATE lib${NAME}-guest-deps)

  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${NAME}-guest PRIVATE -fwrapv)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    ## Compile for SSE2
    ## Compile with fpmath=sse to remove x87 usage
    target_compile_options(${NAME}-guest PRIVATE -msse2 -mfpmath=sse)
  endif()

  if (BITNESS EQUAL 32)
    # Makes the GOT/PLT lookups slightly less painful
    target_compile_options(${NAME}-guest PRIVATE -fno-plt -fno-stack-protector)
    target_link_options(${NAME}-guest PRIVATE "LINKER:-z,now" "LINKER:-z,relro" "LINKER:-z,notext")
  endif()

  # Add linker script if set
  if (BITNESS EQUAL 64 AND EXISTS "${SOURCE_LDS_FILE_ABS}")
    target_link_options(${NAME}-guest PRIVATE "-T" "${CMAKE_CURRENT_SOURCE_DIR}/../lib${NAME}/lib${NAME}_Guest.lds")
    set_property(TARGET ${NAME}-guest APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../lib${NAME}/lib${NAME}_Guest.lds")
  endif()

  if (BITNESS EQUAL 32 AND EXISTS "${SOURCE_LDS_32_FILE_ABS}")
    target_link_options(${NAME}-guest PRIVATE "-T" "${CMAKE_CURRENT_SOURCE_DIR}/../lib${NAME}/lib${NAME}_Guest_32.lds")
    set_property(TARGET ${NAME}-guest APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../lib${NAME}/lib${NAME}_Guest_32.lds")
  endif()

  # We need to override the soname for the linker.
  # Our guest thunk libraries are named `lib<Thunk>-guest`.
  # Once we override the loaded name, the guest is free to dlopen again by SONAME rather than filepath.
  # eg:
  # dlopen("libGL.so.1", RTLD_GLOBAL | RTLD_NOW); -> We override this `libGL.so.1` to `libGL-guest.so`
  # Later on in the program, it can do:
  # dlopen("libGL.so.1", RTLD_GLOBAL | RTLD_NOLOAD);
  # This second dlopen will only check to see if the previous load has made the library resident
  # Searching for SONAME in the process.
  #
  # Additionally, VDSO can only be opened by SONAME.
  # This means it will only ever open the handle with `dlopen("linux-vdso.so.1", RTLD_GLOBAL | RTLD_NOLOAD);
  # Note that this doesn't have a lib prefix, and also since it doesn't exist on the filesystem, it can never
  # Actually load from a path.
  target_link_options(${NAME}-guest PRIVATE "LINKER:-soname,${SONAME}")
  set_target_properties(${NAME}-guest PROPERTIES NO_SONAME ON)

  if (GENERATE_GUEST_INSTALL_TARGETS)
    if (BITNESS EQUAL 64)
      install(TARGETS ${NAME}-guest DESTINATION ${DATA_DIRECTORY}/GuestThunks/)
    else()
      install(TARGETS ${NAME}-guest DESTINATION ${DATA_DIRECTORY}/GuestThunks_32/)
    endif()
  endif()
endfunction()

# These thunks only support 64-bit
if (BITNESS EQUAL 64)
  #add_guest_lib(fex_malloc_loader)
  #target_link_libraries(fex_malloc_loader-guest PRIVATE dl)

  #generate(libfex_malloc)
  #add_guest_lib(fex_malloc)

  generate(libasound ${CMAKE_CURRENT_SOURCE_DIR}/../libasound/libasound_interface.cpp)
  add_guest_lib(asound "libasound.so.2")

  generate(libEGL ${CMAKE_CURRENT_SOURCE_DIR}/../libEGL/libEGL_interface.cpp)
  add_guest_lib(EGL "libEGL.so.1")

  generate(libGL ${CMAKE_CURRENT_SOURCE_DIR}/../libGL/libGL_interface.cpp)
  add_guest_lib(GL "libGL.so.1")

  # libGL must pull in libX11.so, so generate a placeholder libX11.so to link against
  add_library(X11 SHARED ../libX11/libX11_NativeGuest.cpp)
  target_link_libraries(GL-guest PRIVATE X11)

  # disabled for now, headers are platform specific
  # find_package(SDL2 REQUIRED)
  # generate(libSDL2)
  # add_guest_lib(SDL2)
  # target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
  # target_link_libraries(SDL2-guest PRIVATE GL)
  # target_link_libraries(SDL2-guest PRIVATE dl)

  find_package(PkgConfig)
  pkg_search_module(X11 REQUIRED x11)
  version_to_variables(${X11_VERSION} X11)

  generate(libX11 ${CMAKE_CURRENT_SOURCE_DIR}/../libX11/libX11_interface.cpp)
  add_guest_lib(X11 "libX11.so.6")

  target_compile_definitions(libX11-guest-deps INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
  target_compile_definitions(libX11-guest-deps INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
  target_compile_definitions(libX11-guest-deps INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})

  generate(libXext ${CMAKE_CURRENT_SOURCE_DIR}/../libXext/libXext_interface.cpp)
  add_guest_lib(Xext "libXext.so.6")

  target_compile_definitions(libXext-guest-deps INTERFACE -DX11_VERSION_MAJOR=${X11_VERSION_MAJOR})
  target_compile_definitions(libXext-guest-deps INTERFACE -DX11_VERSION_MINOR=${X11_VERSION_MINOR})
  target_compile_definitions(libXext-guest-deps INTERFACE -DX11_VERSION_PATCH=${X11_VERSION_PATCH})

  generate(libXrender ${CMAKE_CURRENT_SOURCE_DIR}/../libXrender/libXrender_interface.cpp)
  add_guest_lib(Xrender "libXrender.so.1")

  generate(libXfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libXfixes/libXfixes_interface.cpp)
  add_guest_lib(Xfixes "libXfixes.so.3")

  find_package(PkgConfig)
  pkg_search_module(XCB REQUIRED xcb)
  version_to_variables(${XCB_VERSION} XCB)

  generate(libxcb ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb/libxcb_interface.cpp)
  add_guest_lib(xcb "libxcb.so.1")

  target_compile_definitions(libxcb-guest-deps INTERFACE -DXCB_VERSION_MAJOR=${XCB_VERSION_MAJOR})
  target_compile_definitions(libxcb-guest-deps INTERFACE -DXCB_VERSION_MINOR=${XCB_VERSION_MINOR})
  target_compile_definitions(libxcb-guest-deps INTERFACE -DXCB_VERSION_PATCH=${XCB_VERSION_PATCH})

  generate(libxcb-dri2 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri2/libxcb-dri2_interface.cpp)
  add_guest_lib(xcb-dri2 "libxcb-dri2.so.0")

  generate(libxcb-dri3 ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-dri3/libxcb-dri3_interface.cpp)
  add_guest_lib(xcb-dri3 "libxcb-dri3.so.0")

  generate(libxcb-xfixes ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-xfixes/libxcb-xfixes_interface.cpp)
  add_guest_lib(xcb-xfixes "libxcb-xfixes.so.0")

  generate(libxcb-shm ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-shm/libxcb-shm_interface.cpp)
  add_guest_lib(xcb-shm "libxcb-shm.so.0")

  generate(libxcb-sync ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-sync/libxcb-sync_interface.cpp)
  add_guest_lib(xcb-sync "libxcb-sync.so.1")

  generate(libxcb-present ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-present/libxcb-present_interface.cpp)
  add_guest_lib(xcb-present "libxcb-present.so.0")

  generate(libxcb-randr ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-randr/libxcb-randr_interface.cpp)
  add_guest_lib(xcb-randr "libxcb-randr.so.0")

  generate(libxcb-glx ${CMAKE_CURRENT_SOURCE_DIR}/../libxcb-glx/libxcb-glx_interface.cpp)
  add_guest_lib(xcb-glx "libxcb-glx.so.0")

  generate(libxshmfence ${CMAKE_CURRENT_SOURCE_DIR}/../libxshmfence/libxshmfence_interface.cpp)
  add_guest_lib(xshmfence "libxshmfence.so.1")

  generate(libdrm ${CMAKE_CURRENT_SOURCE_DIR}/../libdrm/libdrm_interface.cpp)
  target_include_directories(libdrm-guest-deps INTERFACE /usr/include/drm/)
  target_include_directories(libdrm-guest-deps INTERFACE /usr/include/libdrm/)
  add_guest_lib(drm "libdrm.so.2")
endif()

generate(libvulkan ${CMAKE_CURRENT_SOURCE_DIR}/../libvulkan/libvulkan_interface.cpp)
target_include_directories(libvulkan-guest-deps INTERFACE ${FEX_PROJECT_SOURCE_DIR}/External/Vulkan-Headers/include/)
add_guest_lib(vulkan "libvulkan.so.1")

generate(libwayland-client ${CMAKE_CURRENT_SOURCE_DIR}/../libwayland-client/libwayland-client_interface.cpp)
add_guest_lib(wayland-client "libwayland-client.so.0.20.0")

generate(libVDSO ${CMAKE_CURRENT_SOURCE_DIR}/../libVDSO/libVDSO_interface.cpp)
add_guest_lib(VDSO "linux-vdso.so.1")
# Can't use a stack protector because otherwise cross-compiling fails
# Not necessary anyway because it only trampolines
target_compile_options(VDSO-guest PRIVATE "-fno-stack-protector")
target_link_options(VDSO-guest PRIVATE "-nostdlib" "LINKER:--no-undefined" "LINKER:-z,max-page-size=4096" "LINKER:--hash-style=both")

if (BITNESS EQUAL 32)
  # 32-bit entrypoint points to __kernel_vsyscall and needs to exist
  target_link_options(VDSO-guest PRIVATE "LINKER:-e,__kernel_vsyscall")
  # 32-bit VDSO needs to have PIC disabled.
  # Otherwise GCC/Clang generates GOT prologues on the functions that corrupt vsyscall.
  # Correct:
  # 00000350 <__kernel_vsyscall>:
  #  350:   cd 80                   int    0x80
  #  352:   c3                      ret
  #  353:   0f 0b                   ud2
  # Incorrect:
  # 0000032a <__kernel_vsyscall>:
  #  32a:   e8 0b 00 00 00          call   33a <__x86.get_pc_thunk.ax>
  #  32f:   05 79 03 00 00          add    eax,0x379
  #  334:   cd 80                   int    0x80
  #  336:   c3                      ret
  #  337:   90                      nop
  #  338:   0f 0b                   ud2
  target_compile_options(VDSO-guest PRIVATE "-fno-pic")
endif()

if (BUILD_FEX_LINUX_TESTS)
  generate(libfex_thunk_test ${CMAKE_CURRENT_SOURCE_DIR}/../libfex_thunk_test/libfex_thunk_test_interface.cpp)
  add_guest_lib(fex_thunk_test "libfex_thunk_test.so")
endif()
