set(LIBS FEXCore Common CommonCore SonicUtils pthread LLVM)
set(NAME ELFLoader)
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

