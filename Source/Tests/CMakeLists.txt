
add_subdirectory(LinuxSyscalls)

set(LIBS FEXCore Common CommonCore)

add_executable(FEXLoader ELFLoader.cpp)
target_include_directories(FEXLoader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(FEXLoader PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(FEXLoader ${LIBS} LinuxEmulation)

install(TARGETS FEXLoader
  RUNTIME
    DESTINATION bin
    COMPONENT runtime)

add_custom_target(FEXInterpreter ALL
  COMMAND "ln" "-f" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/FEXLoader" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/FEXInterpreter"
  DEPENDS FEXLoader
)

install(
  CODE "MESSAGE(\"-- Installing: ${CMAKE_INSTALL_PREFIX}/bin/FEXInterpreter\")"
  CODE "
  EXECUTE_PROCESS(COMMAND ln -f FEXLoader FEXInterpreter
  WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
  )"
)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  add_custom_target(binfmt_misc_32
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to remove FEX-x86 misc prior to install. Ignore permission denied"
    COMMAND ${CMAKE_COMMAND} -E
    echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86 || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to install FEX-x86 misc now."
    COMMAND ${CMAKE_COMMAND} -E
      echo
      ':FEX-x86:M:0:\\x7fELF\\x01\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x03\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/FEXInterpreter:CF' > /proc/sys/fs/binfmt_misc/register
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
      ':FEX-x86_64:M:0:\\x7fELF\\x02\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/FEXInterpreter:CF' > /proc/sys/fs/binfmt_misc/register
    COMMAND ${CMAKE_COMMAND} -E
    echo "binfmt_misc FEX-x86_64 installed"
  )
  add_custom_target(binfmt_misc
    DEPENDS binfmt_misc_32
    DEPENDS binfmt_misc_64
  )

endif()

add_executable(FEXBash FEXBash.cpp)
target_include_directories(FEXBash PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(FEXBash PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(FEXBash ${LIBS} LinuxEmulation)

install(TARGETS FEXBash
  RUNTIME
    DESTINATION bin
    COMPONENT runtime)

add_executable(TestHarnessRunner TestHarnessRunner.cpp)
target_include_directories(TestHarnessRunner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(TestHarnessRunner PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(TestHarnessRunner ${LIBS} LinuxEmulation)

add_executable(UnitTestGenerator UnitTestGenerator.cpp)
target_include_directories(UnitTestGenerator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(UnitTestGenerator ${LIBS})

add_executable(IRLoader
  IRLoader.cpp
  IRLoader/Loader.cpp
)
target_include_directories(IRLoader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(IRLoader PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(IRLoader ${LIBS} LinuxEmulation)

