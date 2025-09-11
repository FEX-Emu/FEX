#! /usr/bin/env nix-shell
#! nix-shell -i bash FEXLinuxTests/shell.nix

# Helper script to configure CMake for building FEXLinuxTests.
# Nix is used to install and manage the required cross-toolchains.

if [ ! -f CMakeCache.txt ]
then
  echo "Must be run from a pre-configured CMake build folder"
  exit 1
fi

# Remove previous build to ensure the new toolchain is applied
rm -rf unittests/FEXLinuxTests

set -o xtrace
cmake . $FEX_CMAKE_TOOLCHAINS -DBUILD_TESTING=ON -DBUILD_FEX_LINUX_TESTS=ON
