{ pkgs ? import <nixpkgs> { } }:

let
  toolchain = pkgs.fetchzip {
    url = "https://github.com/bylaws/llvm-mingw/releases/download/20250305/llvm-mingw-20250305-ucrt-ubuntu-20.04-aarch64.tar.xz";
    sha256 = "sha256-cA03/ab9O61eO9+S2JzIXD4V0HzTXK5/AYyxW2d73Po=";
  };

  cmakeToolchainFile = pkgs.substitute {
    # Use absolute paths that are discoverable outside of the nix shell
    src = ../../CMake/toolchain_mingw.cmake;
    substitutions = ["--replace-fail" "\${MINGW_TRIPLE}-" "${toolchain}/bin/\${MINGW_TRIPLE}-"];
  };

  mesonCrossFile = pkgs.writeText "crossfile_llvm_mingw.txt" ''
    [binaries]
    ar = '${toolchain}/bin/arm64ec-w64-mingw32-ar'
    c = '${toolchain}/bin/arm64ec-w64-mingw32-gcc'
    cpp = '${toolchain}/bin/arm64ec-w64-mingw32-g++'
    ld = '${toolchain}/bin/arm64ec-w64-mingw32-ld'
    windres = '${toolchain}/bin/arm64ec-w64-mingw32-windres'
    strip = '${toolchain}/bin/strip'
    widl = '${toolchain}/bin/arm64ec-w64-mingw32-widl'
    pkgconfig = 'aarch64-linux-gnu-pkg-config'
    [host_machine]
    system = 'windows'
    cpu_family = 'aarch64'
    cpu = 'aarch64'
    endian = 'little'
  '';
in
pkgs.mkShell {
  buildInputs = [
    toolchain
  ];

  shellHook = ''
    if [[ $- == *i* ]]; then
      echo "llvm-mingw set up at ${toolchain}."
      echo ""
      echo "To configure DXVK/vkd3d-proton: meson setup \$FEX_MESON_CROSSFILE"
      echo ""
      echo "To configure 32-bit FEX build: cmake \$FEX_CMAKE_TOOLCHAIN_WOW64"
      echo "To configure 64-bit FEX build: cmake \$FEX_CMAKE_TOOLCHAIN_ARM64EC"
    fi
  '';

  # E.g. cmake $FEX_CMAKE_TOOLCHAIN_ARM64EC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_LTO=False -DBUILD_TESTS=False
  FEX_CMAKE_TOOLCHAIN_ARM64EC = "--toolchain ${cmakeToolchainFile} -DMINGW_TRIPLE=arm64ec-w64-mingw32 -DCMAKE_INSTALL_LIBDIR=/usr/lib/wine/aarch64-windows";
  FEX_CMAKE_TOOLCHAIN_WOW64 = "--toolchain ${cmakeToolchainFile} -DMINGW_TRIPLE=aarch64-w64-mingw32 -DCMAKE_INSTALL_LIBDIR=/usr/lib/wine/aarch64-windows";
  FEX_MESON_CROSSFILE = "--cross-file ${mesonCrossFile}";
}
