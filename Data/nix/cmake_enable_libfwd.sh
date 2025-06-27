# Helper script to configure CMake for library forwarding in FEX.
# Nix is used to install and manage the required cross-toolchains.

if [ ! -f CMakeCache.txt ]
then
  echo "Must be run from a pre-configured CMake build folder"
  exit 1
fi

# Remove previous build to ensure the new toolchain is applied
rm -rf guest-libs guest-libs-32 Guest Guest_32

# Set clang executable path manually since the one from the nix store
# will be picked up otherwise
CLANG_EXEC_PATH=""
if ! grep -q CLANG_EXEC_PATH CMakeCache.txt
then
  CLANG_EXEC_PATH="-DCLANG_EXEC_PATH=`which clang`"
fi

nix-shell `dirname -- "$0"`/LibraryForwarding/shell.nix \
  --run "set -o xtrace; cmake . \$FEX_CMAKE_TOOLCHAINS -DBUILD_THUNKS=ON $CLANG_EXEC_PATH; set +o xtrace"
