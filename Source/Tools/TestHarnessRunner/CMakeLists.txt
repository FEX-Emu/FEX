list(APPEND LIBS FEXCore Common JemallocLibs)

set (SRCS TestHarnessRunner.cpp)
if (NOT MINGW_BUILD)
  list(APPEND SRCS TestHarnessRunner/HostRunner.cpp)
  list(APPEND LIBS LinuxEmulation CommonTools)
endif()

add_executable(TestHarnessRunner ${SRCS})

if (ENABLE_VIXL_SIMULATOR)
  target_compile_definitions(TestHarnessRunner PRIVATE "-DVIXL_SIMULATOR=1")
endif()

target_include_directories(TestHarnessRunner
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(TestHarnessRunner
  PRIVATE
    ${LIBS}
    ${PTHREAD_LIB}
)
target_include_directories(TestHarnessRunner PRIVATE ${CMAKE_SOURCE_DIR}/External/xbyak/)
