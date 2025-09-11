#! /usr/bin/env nix-shell
#! nix-shell -i bash WineOnArm/shell.nix

# Helper script to configure CMake for building FEX as library for emulation
# of 64-bit applications in Wine/Proton
# Nix is used to install and manage the required cross-toolchains.

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
cmake $FEX_CMAKE_TOOLCHAIN_ARM64EC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_LTO=False -DBUILD_TESTING=False $@
