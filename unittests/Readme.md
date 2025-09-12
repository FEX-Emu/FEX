# FEX Unit tests

FEX has its own test suite for x86-64 emulation, and we also use gcc's target tests, posixtest, and gvisor's tests. We use a combination of CMake/CTest and python runner scripts.

We also regularly run and pass qemu's and valgrind's tests for validation, but those aren't in CI right now.

## x86/64 testing
- A lot of handwritten assembly unit tests in [32Bit_ASM](32Bit_ASM) and [ASM](ASM) folders, run via our TestHarnessHelper
- A few handwritten IR tests in [IR](IR), run via our IRLoader
- gcc-target-tests-32 and gcc-target-tests-64, run via FEX. The tests binaries are in [External/fex-gcc-target-tests-bins](../External/fex-gcc-target-tests-bins)


## Syscall testing
- 64-bit posixtest from http://posixtest.sourceforge.net/, run via FEX. The tests binaries are in [External/fex-posixtest-bins](../External/fex-posixtest-bins)
- 64-bit gvisor tests from https://github.com/google/gvisor, run via FEX. The tests binaries are in [External/fex-gvisor-tests-bins](../External/fex-gvisor-tests-bins)

