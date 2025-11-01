add_compile_options(-fno-operator-names)

set (SRCS
  VDSO_Emulation.cpp
  Thunks.cpp
  ArchHelpers/MContext.cpp
  GdbServer/Info.cpp
  LinuxSyscalls/GdbServer.cpp
  LinuxSyscalls/EmulatedFiles/EmulatedFiles.cpp
  LinuxSyscalls/FaultSafeUserMemAccess.cpp
  LinuxSyscalls/FileManagement.cpp
  LinuxSyscalls/LinuxAllocator.cpp
  LinuxSyscalls/Seccomp/SeccompEmulator.cpp
  LinuxSyscalls/Seccomp/BPFEmitter.cpp
  LinuxSyscalls/Seccomp/Dumper.cpp
  LinuxSyscalls/SignalDelegator.cpp
  LinuxSyscalls/Syscalls.cpp
  LinuxSyscalls/SyscallsSMCTracking.cpp
  LinuxSyscalls/SyscallsVMATracking.cpp
  LinuxSyscalls/ThreadManager.cpp
  LinuxSyscalls/SignalDelegator/GuestFramesManagement.cpp
  LinuxSyscalls/Utils/Threads.cpp
  LinuxSyscalls/x32/Syscalls.cpp
  LinuxSyscalls/x32/EPoll.cpp
  LinuxSyscalls/x32/FD.cpp
  LinuxSyscalls/x32/FS.cpp
  LinuxSyscalls/x32/Info.cpp
  LinuxSyscalls/x32/IO.cpp
  LinuxSyscalls/x32/Memory.cpp
  LinuxSyscalls/x32/Msg.cpp
  LinuxSyscalls/x32/NotImplemented.cpp
  LinuxSyscalls/x32/Semaphore.cpp
  LinuxSyscalls/x32/Sched.cpp
  LinuxSyscalls/x32/Signals.cpp
  LinuxSyscalls/x32/Socket.cpp
  LinuxSyscalls/x32/Stubs.cpp
  LinuxSyscalls/x32/Thread.cpp
  LinuxSyscalls/x32/Time.cpp
  LinuxSyscalls/x32/Timer.cpp
  LinuxSyscalls/x32/IoctlEmulation.cpp
  LinuxSyscalls/x64/EPoll.cpp
  LinuxSyscalls/x64/FD.cpp
  LinuxSyscalls/x64/Info.cpp
  LinuxSyscalls/x64/Memory.cpp
  LinuxSyscalls/x64/NotImplemented.cpp
  LinuxSyscalls/x64/Semaphore.cpp
  LinuxSyscalls/x64/Signals.cpp
  LinuxSyscalls/x64/Thread.cpp
  LinuxSyscalls/x64/Syscalls.cpp
  LinuxSyscalls/x64/Time.cpp
  LinuxSyscalls/Syscalls/EPoll.cpp
  LinuxSyscalls/Syscalls/FD.cpp
  LinuxSyscalls/Syscalls/FS.cpp
  LinuxSyscalls/Syscalls/Passthrough.cpp
  LinuxSyscalls/Syscalls/Info.cpp
  LinuxSyscalls/Syscalls/IO.cpp
  LinuxSyscalls/Syscalls/Memory.cpp
  LinuxSyscalls/Syscalls/Signals.cpp
  LinuxSyscalls/Syscalls/Thread.cpp
  LinuxSyscalls/Syscalls/Timer.cpp
  LinuxSyscalls/Syscalls/NotImplemented.cpp
  LinuxSyscalls/Syscalls/Stubs.cpp)

add_library(LinuxEmulation STATIC ${SRCS})

target_compile_options(LinuxEmulation
PRIVATE
  -Wall
  -Werror=cast-qual
  -Werror=ignored-qualifiers
  -Werror=implicit-fallthrough

  -Wno-trigraphs
  -fwrapv
)

set_target_properties(LinuxEmulation PROPERTIES
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN TRUE
)

target_include_directories(LinuxEmulation
PRIVATE
  ${CMAKE_BINARY_DIR}/generated
  ${CMAKE_CURRENT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/External/drm-headers/include/
)

target_include_directories(LinuxEmulation
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(LinuxEmulation
  PRIVATE
  Common
  CommonTools
)

target_link_libraries(LinuxEmulation
  INTERFACE
  FEXCore
)

set(HEADERS_TO_VERIFY
  LinuxSyscalls/x32/Types.h          x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/asound.h   x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/drm.h      x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/streams.h  x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/usbdev.h   x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/input.h    x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/sockios.h  x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/joystick.h x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x32/Ioctl/v4l2.h     x86_32 # This needs to match structs to 32bit structs
  LinuxSyscalls/x64/Types.h          x86_64 # This needs to match structs to 64bit structs
)

list(LENGTH HEADERS_TO_VERIFY ARG_COUNT)
math(EXPR ARG_COUNT "${ARG_COUNT}-1")

set (ARGS
  "-x" "c++"
  "-std=c++20"
  "-fno-operator-names"
  "-I${PROJECT_SOURCE_DIR}/External/drm-headers/include/"
  "-I${CMAKE_CURRENT_SOURCE_DIR}/../")
# Global include directories
get_directory_property (INC_DIRS INCLUDE_DIRECTORIES)
list(TRANSFORM INC_DIRS PREPEND "-I")
list(APPEND ARGS ${INC_DIRS})

# FEXCore directories
get_target_property(INC_DIRS FEXCore INTERFACE_INCLUDE_DIRECTORIES)
list(TRANSFORM INC_DIRS PREPEND "-I")
list(APPEND ARGS ${INC_DIRS})

get_target_property(INC_DIRS LinuxEmulation INTERFACE_INCLUDE_DIRECTORIES)
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

add_custom_target(
  struct_verifier
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
  COMMAND "ctest" "--output-on-failure" "--timeout" "302" ${TEST_JOB_FLAG} "-R" "Test_verify*")
