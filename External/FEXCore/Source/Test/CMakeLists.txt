set (NAME IRTest)
set (SRCS IRTest.cpp)

add_executable(${NAME} ${SRCS})
add_dependencies(${NAME} IR_INC)

target_link_libraries(${NAME} ${PROJECT_NAME} SonicUtils)

set (NAME LLVMIRTest)
set (SRCS LLVMIRTest.cpp)

add_executable(${NAME} ${SRCS})
add_dependencies(${NAME} IR_INC)

target_link_libraries(${NAME} ${PROJECT_NAME} SonicUtils)

