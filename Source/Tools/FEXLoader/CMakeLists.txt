add_subdirectory(LinuxSyscalls)

list(APPEND LIBS FEXCore Common)

if (TERMUX_BUILD)
  # Termux needs android-shmem to get the shm emulation library.
  list(APPEND LIBS android-shmem)
endif()

function(GenerateInterpreter NAME AsInterpreter)
  add_executable(${NAME}
    FEXLoader.cpp
    VDSO_Emulation.cpp
    AOT/AOTGenerator.cpp)

  # Enable FEX APIs to be used by targets that use target_link_libraries on FEXLoader
  set_target_properties(${NAME} PROPERTIES ENABLE_EXPORTS 1)

  target_include_directories(${NAME}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/
      ${CMAKE_BINARY_DIR}/generated
  )
  target_link_libraries(${NAME}
    PRIVATE
      ${LIBS}
      LinuxEmulation
      ${PTHREAD_LIB}
      fmt::fmt
  )
  target_compile_definitions(${NAME} PRIVATE -DFEXLOADER_AS_INTERPRETER=${AsInterpreter})

  if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    target_link_options(${NAME}
      PRIVATE
        "LINKER:--gc-sections"
        "LINKER:--strip-all"
        "LINKER:--as-needed"
    )
  endif()

  if (NOT MINGW_BUILD)
    install(TARGETS ${NAME}
      RUNTIME
        DESTINATION bin
        COMPONENT runtime
    )
  endif()
endfunction()

GenerateInterpreter(FEXLoader 0)
GenerateInterpreter(FEXInterpreter 1)

install(PROGRAMS "${PROJECT_SOURCE_DIR}/Scripts/FEXUpdateAOTIRCache.sh" DESTINATION bin RENAME FEXUpdateAOTIRCache)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  # Check for conflicting binfmt before installing
  set (CONFLICTING_BINFMTS_32
    ${CMAKE_INSTALL_PREFIX}/share/binfmts/qemu-i386
    ${CMAKE_INSTALL_PREFIX}/share/binfmts/box86)
  set (CONFLICTING_BINFMTS_64
    ${CMAKE_INSTALL_PREFIX}/share/binfmts/qemu-x86_64
    ${CMAKE_INSTALL_PREFIX}/share/binfmts/box64)

  find_program(UPDATE_BINFMTS_PROGRAM update-binfmts)
  if (UPDATE_BINFMTS_PROGRAM)
    add_custom_target(binfmt_misc_32
      echo "Attempting to install FEX-x86 misc now."
      COMMAND "${CMAKE_SOURCE_DIR}/Scripts/CheckBinfmtNotInstall.sh" ${CONFLICTING_BINFMTS_32}
      COMMAND "update-binfmts" "--importdir=${CMAKE_INSTALL_PREFIX}/share/binfmts/" "--import" "FEX-x86"
      COMMAND ${CMAKE_COMMAND} -E
      echo "binfmt_misc FEX-x86 installed"
    )

    add_custom_target(binfmt_misc_64
      COMMAND ${CMAKE_COMMAND} -E
      echo "Attempting to install FEX-x86_64 misc now."
      COMMAND "${CMAKE_SOURCE_DIR}/Scripts/CheckBinfmtNotInstall.sh" ${CONFLICTING_BINFMTS_64}
      COMMAND "update-binfmts" "--importdir=${CMAKE_INSTALL_PREFIX}/share/binfmts/" "--import" "FEX-x86_64"
      COMMAND ${CMAKE_COMMAND} -E
      echo "binfmt_misc FEX-x86_64 installed"
    )
    if(TARGET uninstall)
      add_custom_target(uninstall_binfmt_misc_32
        COMMAND update-binfmts --unimport FEX-x86 || (exit 0)
      )
      add_custom_target(uninstall_binfmt_misc_64
        COMMAND update-binfmts --unimport FEX-x86_64 || (exit 0)
      )

      add_dependencies(uninstall uninstall_binfmt_misc_32)
      add_dependencies(uninstall uninstall_binfmt_misc_64)
    endif()
  else()
    # In the case of update-binfmts not being available (Arch for example) then we need to install manually
    add_custom_target(binfmt_misc_32
      COMMAND ${CMAKE_COMMAND} -E
        echo "Attempting to remove FEX-x86 misc prior to install. Ignore permission denied"
      COMMAND ${CMAKE_COMMAND} -E
        echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86 || (exit 0)
      COMMAND ${CMAKE_COMMAND} -E
        echo "Attempting to install FEX-x86 misc now."
      COMMAND ${CMAKE_COMMAND} -E
        echo
        ':FEX-x86:M:0:\\x7fELF\\x01\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x03\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\x00\\x00\\x00\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/FEXInterpreter:POCF' > /proc/sys/fs/binfmt_misc/register
      COMMAND ${CMAKE_COMMAND} -E
        echo "binfmt_misc FEX-x86 installed"
      )
    add_custom_target(binfmt_misc_64
      COMMAND ${CMAKE_COMMAND} -E
        echo "Attempting to remove FEX-x86_64 misc prior to install. Ignore permission denied"
      COMMAND ${CMAKE_COMMAND} -E
        echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86_64 || (exit 0)
      COMMAND ${CMAKE_COMMAND} -E
        echo "Attempting to install FEX-x86_64 misc now."
      COMMAND ${CMAKE_COMMAND} -E
        echo
        ':FEX-x86_64:M:0:\\x7fELF\\x02\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\x00\\x00\\x00\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/FEXInterpreter:POCF' > /proc/sys/fs/binfmt_misc/register
      COMMAND ${CMAKE_COMMAND} -E
        echo "binfmt_misc FEX-x86_64 installed"
      )

    if(TARGET uninstall)
      add_custom_target(uninstall_binfmt_misc_32
        COMMAND ${CMAKE_COMMAND} -E
          echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86 || (exit 0)
      )
      add_custom_target(uninstall_binfmt_misc_64
        COMMAND ${CMAKE_COMMAND} -E
          echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86_64 || (exit 0)
      )

      add_dependencies(uninstall uninstall_binfmt_misc_32)
      add_dependencies(uninstall uninstall_binfmt_misc_64)
    endif()
  endif()

  add_custom_target(binfmt_misc
    DEPENDS binfmt_misc_32
    DEPENDS binfmt_misc_64
  )
endif()

if (BUILD_TESTS)
  add_executable(TestHarnessRunner TestHarnessRunner/HostRunner.cpp TestHarnessRunner.cpp)
  target_include_directories(TestHarnessRunner
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/Source/
      ${CMAKE_BINARY_DIR}/generated
  )
  target_link_libraries(TestHarnessRunner
    PRIVATE
      ${LIBS}
      LinuxEmulation
      ${PTHREAD_LIB}
  )

# add_executable(UnitTestGenerator UnitTestGenerator.cpp)
# target_include_directories(UnitTestGenerator
#   PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/Source/
# )
# target_link_libraries(UnitTestGenerator
#   PRIVATE
#     ${LIBS}
#     ${PTHREAD_LIB}
# )
#

  add_executable(IRLoader
    IRLoader.cpp
  )
  target_include_directories(IRLoader
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/Source/
      ${CMAKE_BINARY_DIR}/generated
  )
  target_link_libraries(IRLoader
    PRIVATE
      ${LIBS}
      LinuxEmulation
      ${PTHREAD_LIB}
      fmt::fmt
  )
endif()
