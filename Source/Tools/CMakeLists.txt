add_subdirectory(Debugger/)

set(NAME Opt)
set(SRCS Opt.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/External/SonicUtils/)

target_link_libraries(${NAME} FEXCore Common CommonCore SonicUtils pthread LLVM)
