list(APPEND LIBS FEXCore Common)

add_executable(IRLoader
  IRLoader.cpp
)

if (ENABLE_VIXL_SIMULATOR)
  target_compile_definitions(IRLoader PRIVATE "-DVIXL_SIMULATOR=1")
endif()

target_include_directories(IRLoader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(IRLoader
  PRIVATE
    ${LIBS}
    LinuxEmulation
    CommonTools
    ${PTHREAD_LIB}
    fmt::fmt
)
