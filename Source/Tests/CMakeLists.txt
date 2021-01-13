# Boost with minimum version of 1.50, not exact
find_package(Boost 1.50 REQUIRED)

enable_language(ASM_NASM)
if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
  error("Failed to find NASM compatible assembler!")
endif()

set(SYSCALL_SRCS
  LinuxSyscalls/FileManagement.cpp
  LinuxSyscalls/EmulatedFiles/EmulatedFiles.cpp
  LinuxSyscalls/SignalDelegator.cpp
  LinuxSyscalls/Syscalls.cpp
  LinuxSyscalls/x32/Syscalls.cpp
  LinuxSyscalls/x32/EPoll.cpp
  LinuxSyscalls/x32/FD.cpp
  LinuxSyscalls/x32/FS.cpp
  LinuxSyscalls/x32/Info.cpp
  LinuxSyscalls/x32/Memory.cpp
  LinuxSyscalls/x32/NotImplemented.cpp
  LinuxSyscalls/x32/Semaphore.cpp
  LinuxSyscalls/x32/Sched.cpp
  LinuxSyscalls/x32/Signals.cpp
  LinuxSyscalls/x32/Socket.cpp
  LinuxSyscalls/x32/Thread.cpp
  LinuxSyscalls/x32/Time.cpp
  LinuxSyscalls/x32/Timer.cpp
  LinuxSyscalls/x64/EPoll.cpp
  LinuxSyscalls/x64/FD.cpp
  LinuxSyscalls/x64/IO.cpp
  LinuxSyscalls/x64/Ioctl.cpp
  LinuxSyscalls/x64/Info.cpp
  LinuxSyscalls/x64/Memory.cpp
  LinuxSyscalls/x64/Msg.cpp
  LinuxSyscalls/x64/NotImplemented.cpp
  LinuxSyscalls/x64/Semaphore.cpp
  LinuxSyscalls/x64/Sched.cpp
  LinuxSyscalls/x64/Signals.cpp
  LinuxSyscalls/x64/Socket.cpp
  LinuxSyscalls/x64/Thread.cpp
  LinuxSyscalls/x64/Syscalls.cpp
  LinuxSyscalls/x64/Time.cpp
  LinuxSyscalls/Syscalls/EPoll.cpp
  LinuxSyscalls/Syscalls/FD.cpp
  LinuxSyscalls/Syscalls/FS.cpp
  LinuxSyscalls/Syscalls/Info.cpp
  LinuxSyscalls/Syscalls/IO.cpp
  LinuxSyscalls/Syscalls/Key.cpp
  LinuxSyscalls/Syscalls/Memory.cpp
  LinuxSyscalls/Syscalls/Msg.cpp
  LinuxSyscalls/Syscalls/Sched.cpp
  LinuxSyscalls/Syscalls/Semaphore.cpp
  LinuxSyscalls/Syscalls/SHM.cpp
  LinuxSyscalls/Syscalls/Signals.cpp
  LinuxSyscalls/Syscalls/Socket.cpp
  LinuxSyscalls/Syscalls/Thread.cpp
  LinuxSyscalls/Syscalls/Time.cpp
  LinuxSyscalls/Syscalls/Timer.cpp
  LinuxSyscalls/Syscalls/NotImplemented.cpp
  LinuxSyscalls/Syscalls/Stubs.cpp
  )
set(LIBS FEXCore Common CommonCore pthread)
set(NAME FEXLoader)
set(SRCS ELFLoader.cpp ${SYSCALL_SRCS})

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

install(TARGETS ${NAME}
  RUNTIME
    DESTINATION bin
    COMPONENT runtime)

set(FEX_INTERP FEXInterpreter)
install(CODE "
  EXECUTE_PROCESS(COMMAND ln -f ${NAME} ${FEX_INTERP}
  WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
  )
")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  add_custom_target(binfmt_misc_32
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to remove FEX-x86 misc prior to install. Ignore permission denied"
    COMMAND ${CMAKE_COMMAND} -E
    echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86 || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to install FEX-x86 misc now."
    COMMAND ${CMAKE_COMMAND} -E
      echo
      ':FEX-x86:M:0:\\x7fELF\\x01\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x03\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/${FEX_INTERP}:CF' > /proc/sys/fs/binfmt_misc/register
    COMMAND ${CMAKE_COMMAND} -E
    echo "binfmt_misc FEX-x86 installed"
  )

  add_custom_target(binfmt_misc_64
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to remove FEX-x86_64 misc prior to install. Ignore permission denied"
    COMMAND ${CMAKE_COMMAND} -E
    echo -1 > /proc/sys/fs/binfmt_misc/FEX-x86_64 || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E
    echo "Attempting to install FEX-x86_64 misc now."
    COMMAND ${CMAKE_COMMAND} -E
      echo
      ':FEX-x86_64:M:0:\\x7fELF\\x02\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:\\xff\\xff\\xff\\xff\\xff\\xfe\\xfe\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xfe\\xff\\xff\\xff:${CMAKE_INSTALL_PREFIX}/bin/${FEX_INTERP}:CF' > /proc/sys/fs/binfmt_misc/register
    COMMAND ${CMAKE_COMMAND} -E
    echo "binfmt_misc FEX-x86_64 installed"
  )
  add_custom_target(binfmt_misc
    DEPENDS binfmt_misc_32
    DEPENDS binfmt_misc_64
  )

endif()

set(NAME TestHarness)
set(SRCS TestHarness.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME TestHarnessRunner)
set(SRCS TestHarnessRunner.cpp
  ${SYSCALL_SRCS})

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME UnitTestGenerator)
set(SRCS UnitTestGenerator.cpp)

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

set(NAME IRLoader)
set(SRCS
  IRLoader.cpp
  IRLoader/Loader.cpp
  ${SYSCALL_SRCS})

add_executable(${NAME} ${SRCS})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)

target_link_libraries(${NAME} ${LIBS})

