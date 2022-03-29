# FEXCore - Fast x86 Core emulation library
This is the core emulation library that is used for the FEX emulator project.
This project aims to provide a fast and functional x86-64 emulation library that can meet and surpass other x86-64 emulation libraries.
### Goals
* Be as fast as possible, beating and exceeding current options for x86-64 emulation
  * 25% - 50% lower performance than native code would be desired target
  * Use an IR to efficiently translate x86-64 to our host architecture
  * Support a tiered recompiler to allow for fast runtime performance
  * Support offline compilation and offline tooling for inspection and performance analysis
  * Support threaded emulation. Including emulating x86-64's strong memory model on weak memory model architectures
* Support a significant portion of the x86-64 instruction space.
  * Including MMX, SSE, SSE2, SSE3, SSSE3, and SSE4*
* Support fallback routines for uncommonly used x86-64 instructions
  * Including x87 and 3DNow!
* Only support userspace emulation.
  * All x86-64 instructions run as if they are under CPL-3(userland) security layer
* Minimal Linux Syscall emulation for testing purposes
* Portable library implementation in order to support easy integration in to applications
### Target Host Architecture
The target host architecture for this library is AArch64. Specifically the ARMv8.1 version or newer.
The CPU IR is designed with AArch64 in mind but should allow for other architectures as well.
x86-64 host support is available for ease of development, but is not a priority.
### Not desired
* Kernel space emulation
* CPL0-2 emulation
* Real Mode, Protected Mode, Virtual-8086 Mode, System Management Mode
* IRQs
* SVM
* "Cycle Accurate" emulation
