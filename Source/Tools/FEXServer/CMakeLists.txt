
set (fex_unwind_x86_64_src
  Unwind/Unwind_x86_64.cpp)

add_library(fex_unwind_x86_64 STATIC ${fex_unwind_x86_64_src})
target_compile_definitions(fex_unwind_x86_64 PRIVATE -DHAVE_LINK_H)
target_include_directories(fex_unwind_x86_64 PRIVATE
  ${CMAKE_SOURCE_DIR}/External/libunwind/include/
  ${CMAKE_SOURCE_DIR}/External/libunwind/include/tdep-x86_64/
  ${CMAKE_BINARY_DIR}/unwind_x86_64/include/
  )

target_link_libraries(fex_unwind_x86_64
  unwind-x86_64
  FEXCore
  fmt)
add_dependencies(fex_unwind_x86_64 unwind_x86_64)

set (fex_unwind_x86_src
  Unwind/Unwind_x86.cpp)

add_library(fex_unwind_x86 STATIC ${fex_unwind_x86_src})
target_compile_definitions(fex_unwind_x86 PRIVATE -DHAVE_LINK_H)
target_include_directories(fex_unwind_x86 PRIVATE
  ${CMAKE_SOURCE_DIR}/External/libunwind/include/
  ${CMAKE_SOURCE_DIR}/External/libunwind/include/tdep-x86/
  ${CMAKE_BINARY_DIR}/unwind_x86/include/
  )
target_link_libraries(fex_unwind_x86
  unwind-x86
  FEXCore
  fmt)
add_dependencies(fex_unwind_x86 unwind_x86)

set(NAME FEXServer)
set(SRCS Main.cpp
  ArgumentLoader.cpp
  CoreDumpService.cpp
  FDCountWatch.cpp
  Logger.cpp
  PipeScanner.cpp
  ProcessPipe.cpp
  Unwind/ELFMapping.cpp
  SquashFS.cpp)

add_executable(${NAME} ${SRCS})
add_dependencies(${NAME} unwind_x86 unwind_x86_64)

target_include_directories(${NAME} PRIVATE
  ${CMAKE_BINARY_DIR}/generated
  ${CMAKE_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} PRIVATE FEXCore Common ${PTHREAD_LIB} fex_unwind_x86_64 fex_unwind_x86 unwind)
target_link_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/unwind_x86/src/)
target_link_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/unwind_x86_64/src/)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(${NAME}
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)
