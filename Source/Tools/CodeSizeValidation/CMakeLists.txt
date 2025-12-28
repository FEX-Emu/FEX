list(APPEND LIBS FEXCore Common CommonTools JemallocLibs)

add_executable(CodeSizeValidation Main.cpp)
target_include_directories(CodeSizeValidation
  PRIVATE
    ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(CodeSizeValidation
  PRIVATE
    ${LIBS}
    ${PTHREAD_LIB})
