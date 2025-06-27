{ pkgs ? import <nixpkgs> { } }:

let
  pkgsCross32 = pkgs.pkgsCross.gnu32;
  pkgsCross64 = pkgs.pkgsCross.gnu64;

  devRootFS = pkgs.buildEnv {
    name = "fex-dev-rootfs";
    paths = [
      pkgsCross64.stdenv.cc.libc_dev
      pkgsCross32.stdenv.cc.libc_dev
      pkgsCross64.stdenv.cc.cc
      pkgsCross32.stdenv.cc.cc

      pkgs.alsa-lib.dev
      pkgs.libdrm.dev
      pkgs.libGL.dev
      pkgs.wayland.dev
      pkgs.xorg.libX11.dev
      pkgs.xorg.libxcb.dev
      pkgs.xorg.libXrandr.dev
      pkgs.xorg.libXrender.dev
      pkgs.xorg.xorgproto
    ];
    ignoreCollisions = true;
    pathsToLink = [
      "/include"
      "/lib"
    ];

    postBuild = ''
      mkdir -p $out/usr
      ln -s $out/include $out/usr/
    '';
  };

  toolchain32 = pkgs.writeText "toolchain_nix_x86_32.txt" ''
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SYSTEM_PROCESSOR i686)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER ${pkgsCross32.buildPackages.clang}/bin/i686-unknown-linux-gnu-clang)
    set(CMAKE_CXX_COMPILER ${pkgsCross32.buildPackages.clang}/bin/i686-unknown-linux-gnu-clang++)
    set(CLANG_FLAGS "-nodefaultlibs -nostartfiles -lstdc++ -target i686-linux-gnu -msse2 -mfpmath=sse --sysroot=${devRootFS} -iwithsysroot/include")
    set(CMAKE_C_FLAGS "''${CMAKE_C_FLAGS} ''${CLANG_FLAGS}")
    set(CMAKE_CXX_FLAGS "''${CMAKE_CXX_FLAGS} ''${CLANG_FLAGS}")
  '';

  toolchain64 = pkgs.writeText "toolchain_nix_x86_64.txt" ''
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER ${pkgsCross64.buildPackages.clang}/bin/x86_64-unknown-linux-gnu-clang)
    set(CMAKE_CXX_COMPILER ${pkgsCross64.buildPackages.clang}/bin/x86_64-unknown-linux-gnu-clang++)
    set(CLANG_FLAGS "-nodefaultlibs -nostartfiles -lstdc++ -target x86_64-linux-gnu --sysroot=${devRootFS} -iwithsysroot/usr/include")
    set(CMAKE_C_FLAGS "''${CMAKE_C_FLAGS} ''${CLANG_FLAGS}")
    set(CMAKE_CXX_FLAGS "''${CMAKE_CXX_FLAGS} ''${CLANG_FLAGS}")
  '';
in
pkgs.mkShell {
  buildInputs = [
    pkgsCross64.buildPackages.clang
    pkgsCross32.buildPackages.clang
  ];

  shellHook = ''
    if [[ $- == *i* ]]; then
      echo "Set up dev RootFS at ${devRootFS}"
      echo "toolchain32: ${toolchain32}"
      echo "toolchain64: ${toolchain64}"
      echo ""
      echo "Use \$FEX_CMAKE_TOOLCHAINS to configure CMake."
    fi
  '';

  FEX_CMAKE_TOOLCHAINS = "-DX86_32_TOOLCHAIN_FILE=${toolchain32} -DX86_64_TOOLCHAIN_FILE=${toolchain64} -DX86_DEV_ROOTFS=${devRootFS}";
  ROOTFS = "${devRootFS}";
}
