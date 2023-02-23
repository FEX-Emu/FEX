add_compile_options(-fno-operator-names)

add_library(LinuxEmulation STATIC
    EmulatedFiles/EmulatedFiles.cpp
    FileManagement.cpp
    LinuxAllocator.cpp
    SignalDelegator.cpp
    Syscalls.cpp
    SyscallsSMCTracking.cpp
    SyscallsVMATracking.cpp
    x32/Syscalls.cpp
    x32/EPoll.cpp
    x32/FD.cpp
    x32/FS.cpp
    x32/Info.cpp
    x32/IO.cpp
    x32/Memory.cpp
    x32/Msg.cpp
    x32/NotImplemented.cpp
    x32/Semaphore.cpp
    x32/Sched.cpp
    x32/Signals.cpp
    x32/Socket.cpp
    x32/Stubs.cpp
    x32/Thread.cpp
    x32/Time.cpp
    x32/Timer.cpp
    x32/IoctlEmulation.cpp
    x64/EPoll.cpp
    x64/FD.cpp
    x64/IO.cpp
    x64/Ioctl.cpp
    x64/Info.cpp
    x64/Memory.cpp
    x64/Msg.cpp
    x64/NotImplemented.cpp
    x64/Semaphore.cpp
    x64/Sched.cpp
    x64/Signals.cpp
    x64/Socket.cpp
    x64/Thread.cpp
    x64/Syscalls.cpp
    x64/Time.cpp
    Syscalls/EPoll.cpp
    Syscalls/FD.cpp
    Syscalls/FS.cpp
    Syscalls/Info.cpp
    Syscalls/IO.cpp
    Syscalls/IOUring.cpp
    Syscalls/Key.cpp
    Syscalls/Memory.cpp
    Syscalls/Msg.cpp
    Syscalls/Namespace.cpp
    Syscalls/Sched.cpp
    Syscalls/Semaphore.cpp
    Syscalls/SHM.cpp
    Syscalls/Signals.cpp
    Syscalls/Socket.cpp
    Syscalls/Thread.cpp
    Syscalls/Time.cpp
    Syscalls/Timer.cpp
    Syscalls/NotImplemented.cpp
    Syscalls/Stubs.cpp
)

target_compile_options(LinuxEmulation
PRIVATE
  -Wall
  -Werror=cast-qual
  -Werror=ignored-qualifiers
  -Werror=implicit-fallthrough

  -Wno-trigraphs
  -fwrapv
)

target_include_directories(LinuxEmulation
PRIVATE
  ${CMAKE_BINARY_DIR}/generated
  ${PROJECT_SOURCE_DIR}/External/drm-headers/include/
)

target_link_libraries(LinuxEmulation
PRIVATE
  FEXCore
  Common
  FEX_Utils
)

set(HEADERS_TO_VERIFY
  x32/Types.h          x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/asound.h   x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/drm.h      x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/streams.h  x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/usbdev.h   x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/input.h    x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/sockios.h  x86_32 # This needs to match structs to 32bit structs
  x32/Ioctl/joystick.h x86_32 # This needs to match structs to 32bit structs
  x64/Types.h          x86_64 # This needs to match structs to 64bit structs
)

list(LENGTH HEADERS_TO_VERIFY ARG_COUNT)
math(EXPR ARG_COUNT "${ARG_COUNT}-1")

set (ARGS
  "-x" "c++"
  "-std=c++20"
  "-fno-operator-names"
  "-I${PROJECT_SOURCE_DIR}/External/drm-headers/include/")
# Global include directories
get_directory_property (INC_DIRS INCLUDE_DIRECTORIES)
list(TRANSFORM INC_DIRS PREPEND "-I")
list(APPEND ARGS ${INC_DIRS})

# FEXCore directories
get_target_property(INC_DIRS FEXCore INTERFACE_INCLUDE_DIRECTORIES)
list(TRANSFORM INC_DIRS PREPEND "-I")
list(APPEND ARGS ${INC_DIRS})

foreach(Index RANGE 0 ${ARG_COUNT} 2)
  math(EXPR TEST_TYPE_INDEX "${Index}+1")

  list(GET HEADERS_TO_VERIFY ${Index} HEADER)
  list(GET HEADERS_TO_VERIFY ${TEST_TYPE_INDEX} TEST_TYPE)

  file(RELATIVE_PATH REL_HEADER ${CMAKE_BINARY_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}")
  set(TEST_NAME "${TEST_DESC}/Test_verify_${HEADER}")
  set(TEST_NAME_ARCH "${TEST_DESC}/Test_verify_arch_${HEADER}")

  add_test(
    NAME ${TEST_NAME}_x86_64
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/StructPackVerifier.py" "-c1" "x86_64" "${REL_HEADER}" ${ARGS})

  add_test(
    NAME ${TEST_NAME}_aarch64
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/StructPackVerifier.py" "-c1" "aarch64" "${REL_HEADER}" ${ARGS})

  add_test(
    NAME ${TEST_NAME_ARCH}_x86_64
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/StructPackVerifier.py" "-c1" "x86_64" "-c2" "${TEST_TYPE}" "${REL_HEADER}" ${ARGS})

  add_test(
    NAME ${TEST_NAME_ARCH}_aarch64
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/StructPackVerifier.py" "-c1" "aarch64" "-c2" "${TEST_TYPE}" "${REL_HEADER}" ${ARGS})

  set_property(TEST ${TEST_NAME}_x86_64 APPEND PROPERTY DEPENDS "${HEADER}")
  set_property(TEST ${TEST_NAME}_aarch64 APPEND PROPERTY DEPENDS "${HEADER}")
  set_property(TEST ${TEST_NAME_ARCH}_x86_64 APPEND PROPERTY DEPENDS "${HEADER}")
  set_property(TEST ${TEST_NAME_ARCH}_aarch64 APPEND PROPERTY DEPENDS "${HEADER}")
endforeach()

execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
string(STRIP ${CORES} CORES)

add_custom_target(
  struct_verifier
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--timeout" "302" "-j${CORES}" "-R" "Test_verify*")
