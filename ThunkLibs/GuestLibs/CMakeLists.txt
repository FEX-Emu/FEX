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

generate(libasound thunks function_packs function_packs_public)
add_guest_lib(asound)

generate(libEGL thunks function_packs function_packs_public)
add_guest_lib(EGL)

generate(libGL thunks function_packs function_packs_public tab_function_packs)
add_guest_lib(GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 thunks function_packs function_packs_public)
# add_guest_lib(SDL2)
# target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(SDL2-guest PRIVATE GL)
# target_link_libraries(SDL2-guest PRIVATE dl)

generate(libX11 thunks function_packs function_packs_public callback_structs callback_unpacks_header callback_unpacks_header_init callback_unpacks callback_typedefs)
add_guest_lib(X11)

generate(libXext thunks function_packs function_packs_public)
add_guest_lib(Xext)

generate(libXrender thunks function_packs function_packs_public)
add_guest_lib(Xrender)

generate(libXfixes thunks function_packs function_packs_public)
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
  set(WITH_ARGS "lib${LIB}" "${VULKAN_XML}" "--print-header")
  generate_with_name(libvulkan_device lib${LIB} Header)
  set(WITH_ARGS "")

  set(WITH_ARGS "lib${LIB}" "${VULKAN_XML}" "--print-thunks")
  generate_with_name(libvulkan_device lib${LIB} thunks function_packs function_packs_public function_pack_pair)
  add_guest_lib_with_name(vulkan_device ${LIB})
  set(WITH_ARGS "")
endforeach()

# Struct verification on generated headers

if (BUILD_TESTS)
  include(CTest)
  enable_testing()
  message(STATUS "Unit tests are enabled")

  set(HEADERS_TO_VERIFY
    "${CMAKE_CURRENT_BINARY_DIR}/gen/libvulkan_radeon/Header.inl" x86_64 # This needs to match structs to 64bit structs
  )

  list(LENGTH HEADERS_TO_VERIFY ARG_COUNT)
  math(EXPR ARG_COUNT "${ARG_COUNT}-1")
  message(STATUS "Number of tests: ${ARG_COUNT}")

  set (ARGS
    "-x" "c++"
    "-std=c++20"
    "-fno-operator-names")
# Global include directories
  get_directory_property (INC_DIRS INCLUDE_DIRECTORIES)
  list(TRANSFORM INC_DIRS PREPEND "-I")
  list(APPEND ARGS ${INC_DIRS})

  list(APPEND ARGS "-I${CMAKE_CURRENT_BINARY_DIR}/gen/vulkan_radeon/")

  foreach(Index RANGE 0 ${ARG_COUNT} 2)
    math(EXPR TEST_TYPE_INDEX "${Index}+1")

    list(GET HEADERS_TO_VERIFY ${Index} HEADER)
    list(GET HEADERS_TO_VERIFY ${TEST_TYPE_INDEX} TEST_TYPE)

    file(RELATIVE_PATH REL_HEADER ${CMAKE_BINARY_DIR} "${HEADER}")
    set(TEST_NAME "${TEST_DESC}/Guest_Test_verify_${HEADER}")
    set(TEST_NAME_ARCH "${TEST_DESC}/Guest_Test_verify_arch_${HEADER}")

    add_test(
      NAME ${TEST_NAME}_x86_64
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMAND "python3" "${STRUCT_VERIFIER}" "-c1" "x86_64" "${REL_HEADER}" ${ARGS})

    add_test(
      NAME ${TEST_NAME}_aarch64
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMAND "python3" "${STRUCT_VERIFIER}" "-c1" "aarch64" "${REL_HEADER}" ${ARGS})

    add_test(
      NAME ${TEST_NAME_ARCH}_x86_64
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMAND "python3" "${STRUCT_VERIFIER}" "-c1" "x86_64" "-c2" "${TEST_TYPE}" "${REL_HEADER}" ${ARGS})

    add_test(
      NAME ${TEST_NAME_ARCH}_aarch64
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMAND "python3" "${STRUCT_VERIFIER}" "-c1" "aarch64" "-c2" "${TEST_TYPE}" "${REL_HEADER}" ${ARGS})

    set_property(TEST ${TEST_NAME}_x86_64 APPEND PROPERTY DEPENDS "${HEADER}")
    set_property(TEST ${TEST_NAME}_aarch64 APPEND PROPERTY DEPENDS "${HEADER}")
    set_property(TEST ${TEST_NAME_ARCH}_x86_64 APPEND PROPERTY DEPENDS "${HEADER}")
    set_property(TEST ${TEST_NAME_ARCH}_aarch64 APPEND PROPERTY DEPENDS "${HEADER}")
  endforeach()

  execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
  string(STRIP ${CORES} CORES)

  add_custom_target(
    struct_verifier_guest
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "Guest_Test_verify*")
endif()

generate(libxcb thunks function_packs function_packs_public callback_structs callback_unpacks_header callback_unpacks_header_init callback_unpacks callback_typedefs)
add_guest_lib(xcb)

generate(libxcb_dri2 thunks function_packs function_packs_public)
add_guest_lib(xcb_dri2)

generate(libxcb_dri3 thunks function_packs function_packs_public)
add_guest_lib(xcb_dri3)

generate(libxcb_xfixes thunks function_packs function_packs_public)
add_guest_lib(xcb_xfixes)

generate(libxcb_shm thunks function_packs function_packs_public)
add_guest_lib(xcb_shm)

generate(libxcb_sync thunks function_packs function_packs_public)
add_guest_lib(xcb_sync)

generate(libxcb_present thunks function_packs function_packs_public)
add_guest_lib(xcb_present)

generate(libxcb_randr thunks function_packs function_packs_public)
add_guest_lib(xcb_randr)

generate(libxcb_glx thunks function_packs function_packs_public)
add_guest_lib(xcb_glx)

generate(libxshmfence thunks function_packs function_packs_public)
add_guest_lib(xshmfence)

generate(libdrm thunks function_packs function_packs_public)
add_guest_lib(drm)
target_include_directories(drm-guest PRIVATE /usr/include/drm/)
target_include_directories(drm-guest PRIVATE /usr/include/libdrm/)
