cmake_minimum_required(VERSION 3.14)
project(FEXLibWrappers)

set(CMAKE_CXX_STANDARD 20)
set(FEX_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_subdirectory(Generator)

# Two methods of building guest library wrappers are supported:
# - build as part of the main project (recommended)
# - build in isolation from a dedicated build folder
# The second method is useful when it's difficult to set up a full
# cross-compiling environment targeting x86. In those cases, guest library
# compilation can be moved to a different machine / VM.
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # When included during a regular FEX build, build both guest and host libs
  # unless requested otherwise
  set(BUILD_HOST_LIBS TRUE)
  # BUILD_GUEST_LIBS from parent project

  # However, include targets for IDE integration of guest libs
  add_subdirectory(GuestLibs)
else()
  # When included on our own, only build guest libraries
  set(BUILD_HOST_LIBS FALSE)
  set(BUILD_GUEST_LIBS TRUE)
endif()

if (BUILD_HOST_LIBS)
  # Targets for building host libraries and for IDE integration
  add_subdirectory(HostLibs)
endif()

if (BUILD_GUEST_LIBS)
  # Targets for building guest libraries
  include(ExternalProject)

  ExternalProject_Add(guest-libs
    PREFIX Guest/guest-libs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/GuestLibs"
    BINARY_DIR "../Guest"
    CMAKE_ARGS
      "-DBITNESS=64"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-DBUILD_FEX_LINUX_TESTS=${BUILD_FEX_LINUX_TESTS}"
      "-DENABLE_CLANG_THUNKS=${ENABLE_CLANG_THUNKS}"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${FEX_PROJECT_SOURCE_DIR}/toolchain_x86_64.cmake"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
      "-DFEX_PROJECT_SOURCE_DIR=${FEX_PROJECT_SOURCE_DIR}"
      "-DGENERATOR_EXE=$<TARGET_FILE:thunkgen>"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
    DEPENDS thunkgen
  )

  ExternalProject_Add(guest-libs-32
    PREFIX Guest_32/guest-libs-32
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/GuestLibs"
    BINARY_DIR "../Guest_32"
    CMAKE_ARGS
      "-DBITNESS=32"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-DBUILD_FEX_LINUX_TESTS=${BUILD_FEX_LINUX_TESTS}"
      "-DENABLE_CLANG_THUNKS=${ENABLE_CLANG_THUNKS}"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${FEX_PROJECT_SOURCE_DIR}/toolchain_x86_32.cmake"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
      "-DFEX_PROJECT_SOURCE_DIR=${FEX_PROJECT_SOURCE_DIR}"
      "-DGENERATOR_EXE=$<TARGET_FILE:thunkgen>"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
    DEPENDS thunkgen
  )

  install(
    CODE "MESSAGE(\"-- Installing: guest-libs\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest
    )"
    DEPENDS guest-libs
  )

  install(
    CODE "MESSAGE(\"-- Installing: guest-libs-32\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest_32
    )"
    DEPENDS guest-libs-32
  )

  if (TARGET uninstall)
    add_custom_target(uninstall_guest-libs
      COMMAND ${CMAKE_COMMAND} "--build" "." "--target" "uninstall"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest
    )

    add_custom_target(uninstall_guest-libs-32
      COMMAND ${CMAKE_COMMAND} "--build" "." "--target" "uninstall"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest_32
    )

    add_dependencies(uninstall uninstall_guest-libs)
    add_dependencies(uninstall uninstall_guest-libs-32)
  endif ()
endif()
