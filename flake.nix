{
  description = " A fast usermode x86 and x86-64 emulator for Arm64 Linux";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });

    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
          deps = with pkgs; [
            git
            cmake
            ninja
            clang
            binutils
            lld
            llvm
            SDL2
            libepoxy
            nasm
            pkg-config
            python3
            gcc-unwrapped
            python311Packages.setuptools
          ];
        in {
          fex = pkgs.stdenv.mkDerivation rec {
            name = "FEXBash";
            src = self;
            nativeBuildInputs = deps ++ [ pkgs.stdenv.cc.cc.lib ];
            dontUseCmakeConfigure = true;
            dontUseCmakeInstall = true;
            configurePhase = ''
              mkdir Build
              cd Build
              CC=clang CXX=clang++ cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DUSE_LINKER=lld -DENABLE_LTO=True -DBUILD_TESTS=False -DENABLE_ASSERTIONS=False -G Ninja ..
            '';
            buildPhase = ''
              ninja -j $(nproc)
            '';
            installPhase = ''
              mkdir -p $out/bin
              mv ./Bin/* $out/bin/
            '';
          };
        });

      defaultPackage = forAllSystems (system: self.packages.${system}.fex);
    };
}
