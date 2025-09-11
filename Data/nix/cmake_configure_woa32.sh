#! /usr/bin/env nix-shell
#! nix-shell -i bash WineOnArm/shell.nix

# Helper script to configure CMake for building FEX as library for emulation
# of 32-bit applications in Wine/Proton.
# The required cross-toolchains will be set up and managed by nix.

if [ $# -eq 0 ]
then
  echo "Expected CMake argument list"
  exit 1
fi

if [ -f CMakeCache.txt ]
then
  echo "Expected empty build folder"
  exit 1
fi

set -o xtrace
cmake $FEX_CMAKE_TOOLCHAIN_WOW64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_LTO=False -DBUILD_TESTING=False $@
