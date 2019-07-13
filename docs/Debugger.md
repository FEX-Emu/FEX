# FEX - Debugger GUI
---
FEX has a debugger GUI for debugging the translation of x86 code.
This UI is written in [Dear ImGui](https://github.com/ocornut/imgui) since it is purely for debugging and not user facing.

## Features
* Stopping code execution at any point
* Single stepping code
* ELF code loading
* Test Harness code loading
* CPU State inspection
* IR inspection
* CF inspection of IR
* Application stderr, stdout viewing
* Disassembler for viewing both guest and host code

## Nice to have
* IR writing and direct compiling
  * Requires writing an IR lexer
* Profiling of compiled blocks directly in debugger
  * Save CPU state prior to running, time compiled block for microprofiling
* Save all state to file to allow offline inspection and profiling
  * Save original code
  * Save CPU state on entry
  * Save IR
  * Save compiled host code
  * Profiling data
  * Allow comparison of all state before(What was saved to file) and after (New version compiled in this version of FEX for iterative debugging)
* Single stepping of IR state and inspection of SSA values
* Complete disconnection of debugger from FEXCore to ensure robustness if core crashes.
