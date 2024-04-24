  include(ExternalProject)

  set (FEX_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
  add_subdirectory(Generator)

  # Thunk targets for IDE integration of guest code, only
  add_subdirectory(GuestLibs)

  # Thunk targets for both host libraries and IDE integration
  add_subdirectory(HostLibs)

  # Thunk targets for guest libraries
  ExternalProject_Add(guest-libs
    PREFIX Guest/guest-libs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/GuestLibs"
    BINARY_DIR "Guest"
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
    BINARY_DIR "Guest_32"
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
