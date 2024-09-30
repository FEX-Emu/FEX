list(APPEND LIBS FEXCore Common CommonTools JemallocLibs)

set (SRCS Main.cpp)
add_executable(CodeSizeValidation ${SRCS})
target_include_directories(CodeSizeValidation
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(CodeSizeValidation
  PRIVATE
    ${LIBS}
    ${PTHREAD_LIB}
)
