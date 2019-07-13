# FEX - Fast x86 emulation frontend
This is the frontend application and tooling used for development and debugging of the FEXCore library.

### Dependencies
* [SonicUtils](https://github.com/Sonicadvance1/SonicUtils)
* FEXCore
* cpp-optparse
* imgui
* json-maker
* tiny-json
* boost interprocess (sadly)
* A C++17 compliant compiler (There are assumptions made about using Clang and LTO)
* clang-tidy if you want the code cleaned up
* cmake

![FEX diagram](docs/Diagram.svg)
