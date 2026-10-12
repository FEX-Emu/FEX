# -S skips importing the "site" package which cuts down on interpreter startup time.
# This means that these scripts can only use the python standard library, since "site"
# is what resolves third-party packages from pip or apt.
set(PYTHON_TEST_RUNNER "python3" "-S")

if (NOT MINGW)
  add_subdirectory(APITests/)
  add_subdirectory(POSIX/)
  add_subdirectory(gvisor-tests/)
  add_subdirectory(gcc-target-tests-32/)
  add_subdirectory(gcc-target-tests-64/)
  add_subdirectory(Utilities/)

  if (BUILD_THUNKS)
    add_subdirectory(ThunkLibs)
    add_subdirectory(ThunkFunctionalTests)
  endif()

  if (BUILD_FEX_LINUX_TESTS)
    add_subdirectory(FEXLinuxTests/)
  endif()
endif()

add_subdirectory(ASM/)
add_subdirectory(32Bit_ASM/)
if (ENABLE_VIXL_DISASSEMBLER)
  # Tests are only valid to run if the vixl disassembler is enabled and the active JIT is the ARM64 JIT.
  add_subdirectory(InstructionCountCI/)
endif()
