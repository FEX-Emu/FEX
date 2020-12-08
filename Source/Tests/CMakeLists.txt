# Boost with minimum version of 1.50, not exact
find_package(Boost 1.50 REQUIRED)

enable_language(ASM_NASM)
if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
  error("Failed to find NASM compatible assembler!")
endif()


set(LIBS FEXCore Common CommonCore pthread)
set(NAME FEXLoader)
set(SRCS ELFLoader.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

install(TARGETS ${NAME}
  RUNTIME
    DESTINATION bin
    COMPONENT runtime)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  set(FEX_INTERP FEXInterpreter)
  install(CODE "
    EXECUTE_PROCESS(COMMAND ln -sf ${NAME} ${FEX_INTERP}
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
    )
  ")

  add_custom_target(binfmt_misc_32
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to remove FEX-x86 misc prior to install. Ignore permission denied"
    COMMAND ${CMAKE_COMMAND} -E
    echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86 || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to install FEX-x86 misc now."
    COMMAND ${CMAKE_COMMAND} -E
      echo
      ':FEX-x86:M:0:\\x7fELF\\x01\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x03\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/${FEX_INTERP}:CF' > /proc/sys/fs/binfmt_misc/register
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
      ':FEX-x86_64:M:0:\\x7fELF\\x02\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/${FEX_INTERP}:CF' > /proc/sys/fs/binfmt_misc/register
    COMMAND ${CMAKE_COMMAND} -E
    echo "binfmt_misc FEX-x86_64 installed"
  )
  add_custom_target(binfmt_misc
    DEPENDS binfmt_misc_32
    DEPENDS binfmt_misc_64
  )

endif()

set(NAME TestHarness)
set(SRCS TestHarness.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME TestHarnessRunner)
set(SRCS TestHarnessRunner.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME UnitTestGenerator)
set(SRCS UnitTestGenerator.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME PTrace)
set(SRCS TestSingleStepHardware.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME IRLoader)
set(SRCS
  IRLoader.cpp
  IRLoader/Loader.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

