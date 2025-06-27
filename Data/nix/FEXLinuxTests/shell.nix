{ pkgs ? import <nixpkgs> { } }:

let
  pkgsCross32 = pkgs.pkgsCross.gnu32;
  pkgsCross64 = pkgs.pkgsCross.gnu64;

  gcc32 = pkgs.writeText "toolchain_nix_gcc_x86_32.txt" ''
    set(CMAKE_SYSTEM_PROCESSOR i686)
    set(CMAKE_C_COMPILER ${pkgsCross32.buildPackages.gcc}/bin/i686-unknown-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER ${pkgsCross32.buildPackages.gcc}/bin/i686-unknown-linux-gnu-g++)
  '';

  gcc64 = pkgs.writeText "toolchain_nix_gcc_x86_64.txt" ''
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(CMAKE_C_COMPILER ${pkgsCross64.buildPackages.gcc}/bin/x86_64-unknown-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER ${pkgsCross64.buildPackages.gcc}/bin/x86_64-unknown-linux-gnu-g++)
  '';
in
pkgs.mkShell {
  buildInputs = [
    pkgsCross64.buildPackages.clang
    pkgsCross32.buildPackages.clang
  ];

  shellHook = ''
    if [[ $- == *i* ]]; then
      echo "toolchain32: ${gcc32}"
      echo "toolchain64: ${gcc64}"
      echo ""
      echo "Use \$FEX_CMAKE_TOOLCHAINS to configure CMake."
    fi
  '';

  FEX_CMAKE_TOOLCHAINS = "-DX86_32_TOOLCHAIN_FILE=${gcc32} -DX86_64_TOOLCHAIN_FILE=${gcc64}";
}
