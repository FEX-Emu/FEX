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

