// This file only exists to create a placeholder library to link against for
// libraries that are supposed to implicitly load libX11. At runtime, the guest
// linker will select the library from the RootFS instead, which is then
// replaced by libX11-guest.so.

// Define some symbol so that the linker doesn't consider this library unused
extern "C" void XSetErrorHandler() {}
