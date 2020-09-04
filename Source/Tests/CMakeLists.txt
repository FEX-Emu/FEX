# Boost with minimum version of 1.50, not exact
find_package(Boost 1.50 REQUIRED)

enable_language(ASM_NASM)
if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
  error("Failed to find NASM compatible assembler!")
endif()


set(LIBS FEXCore Common CommonCore SonicUtils pthread)
set(NAME FEXLoader)
set(SRCS ELFLoader.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

install(TARGETS ${NAME}
  RUNTIME
    DESTINATION bin
    COMPONENT runtime)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  add_custom_target(binfmt_misc
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to remove FEX-x86_64 misc prior to install. Ignore permission denied"
    COMMAND ${CMAKE_COMMAND} -E
    echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86_64 || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to install FEX-x86_64 misc now."
    COMMAND ${CMAKE_COMMAND} -E
      echo
      ':FEX-x86_64:M:0:\\x7fELF\\x02\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/${NAME}:CF' > /proc/sys/fs/binfmt_misc/register
    COMMAND ${CMAKE_COMMAND} -E
    echo "binfmt_misc FEX installed"
      )
endif()

set(NAME TestHarness)
set(SRCS TestHarness.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

set(NAME TestHarnessRunner)
set(SRCS TestHarnessRunner.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

set(NAME LockstepRunner)
set(SRCS LockstepRunner.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

set(NAME UnitTestGenerator)
set(SRCS UnitTestGenerator.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

set(NAME PTrace)
set(SRCS TestSingleStepHardware.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

set(NAME IRLoader)
set(SRCS
  IRLoader.cpp
  IRLoader/Loader.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} ${LIBS})

