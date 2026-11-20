{
  description = "FEX: A fast usermode x86 and x86-64 emulator for Arm64 Linux";

  inputs.self.submodules = true;

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  inputs.flake-compat = {
    url = "github:NixOS/flake-compat";
    flake = false;
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      system = "aarch64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvmPackages = pkgs.llvmPackages;

      # Headers/libs the thunk libraries forward to
      libForwardingInputs = pkgs.lib.map pkgs.lib.getInclude [
        pkgs.alsa-lib
        pkgs.libdrm
        pkgs.libGL
        pkgs.wayland
        pkgs.libx11
        pkgs.libxcb
        pkgs.libxrandr
        pkgs.libxrender
        pkgs.xorgproto
      ];

      pkgsCross32 = pkgs.pkgsCross.gnu32;
      pkgsCross64 = pkgs.pkgsCross.gnu64;

      # Sysroot the guest (x86/x86_64) thunk libraries compile against
      devRootFS = pkgs.buildEnv {
        name = "fex-dev-rootfs";
        paths = [
          pkgsCross64.stdenv.cc.libc_dev
          pkgsCross32.stdenv.cc.libc_dev
          pkgsCross64.stdenv.cc.cc
          pkgsCross32.stdenv.cc.cc
        ]
        ++ libForwardingInputs;
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

      # Cross toolchain files for the guest thunk libraries.
      mkToolchain =
        bitness: target: extraFlags:
        let
          cross = if bitness == 32 then pkgsCross32 else pkgsCross64;
        in
        pkgs.writeText "toolchain_nix_x86_${toString bitness}.txt" ''
          set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
          set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
          set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
          set(CMAKE_SYSTEM_PROCESSOR ${target})
          set(CMAKE_C_COMPILER ${cross.buildPackages.clang}/bin/${target}-unknown-linux-gnu-clang)
          set(CMAKE_CXX_COMPILER ${cross.buildPackages.clang}/bin/${target}-unknown-linux-gnu-clang++)
          set(CLANG_FLAGS "-nodefaultlibs -nostartfiles -target ${target}-linux-gnu ${extraFlags} --sysroot=${devRootFS} -iwithsysroot/usr/include")
          set(CMAKE_C_FLAGS "''${CMAKE_C_FLAGS} ''${CLANG_FLAGS}")
          set(CMAKE_CXX_FLAGS "''${CMAKE_CXX_FLAGS} ''${CLANG_FLAGS}")
        '';

      toolchain32 = mkToolchain 32 "i686" "-msse2 -mfpmath=sse";
      toolchain64 = mkToolchain 64 "x86_64" "";

      armTuneArches = {
        armv8_0 = "armv8-a";
        armv8_2 = "armv8.2-a";
        armv8_4 = "armv8.4-a";
      };

      mkFex =
        { pname, tuneArch }:
        llvmPackages.stdenv.mkDerivation (finalAttrs: {
          inherit pname;

          # Short rev + -dirty when the worktree is dirty
          version =
            let
              si = self.sourceInfo or { };
              dirtyRev = si.dirtyRev or null;
              baseRev = si.rev or (if dirtyRev != null then pkgs.lib.removeSuffix "-dirty" dirtyRev else null);
            in
            if baseRev == null then
              "FEX"
            else
              "${pkgs.lib.substring 0 7 baseRev}${pkgs.lib.optionalString (dirtyRev != null) "-dirty"}";

          src = self;

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
            llvmPackages.bintools
            pkgs.qt6.wrapQtAppsHook
            pkgs.nasm
            (pkgs.python3.withPackages (ps: [
              ps.packaging
              ps.libclang
            ]))
          ];

          buildInputs = [
            pkgs.xxhash
            pkgs.fmt
            pkgs.libxml2
            pkgs.openssl
            pkgs.range-v3
            pkgsCross64.buildPackages.clang
            pkgsCross32.buildPackages.clang
            pkgs.libclang
            llvmPackages.libllvm
            pkgs.qt6.qtbase
            pkgs.qt6.qtdeclarative
          ]
          ++ libForwardingInputs;

          cmakeFlags = [
            (pkgs.lib.cmakeFeature "USE_LINKER" "lld")
            (pkgs.lib.cmakeFeature "OVERRIDE_VERSION" finalAttrs.version)
            (pkgs.lib.cmakeFeature "OVERRIDE_HASH" "00")
            (pkgs.lib.cmakeBool "BUILD_THUNKS" true)
            (pkgs.lib.cmakeFeature "X86_32_TOOLCHAIN_FILE" "${toolchain32}")
            (pkgs.lib.cmakeFeature "X86_64_TOOLCHAIN_FILE" "${toolchain64}")
            (pkgs.lib.cmakeFeature "X86_DEV_ROOTFS" "${devRootFS}")
            (pkgs.lib.cmakeFeature "TUNE_ARCH" tuneArch)
            (pkgs.lib.cmakeFeature "TUNE_CPU" "none")
          ];

          strictDeps = true;

          # thunkgen's libclang can't discover system headers on Nix
          THUNKGEN_EXTRA_FLAGS = pkgs.lib.concatStringsSep ";" (
            [
              "-idirafter ${llvmPackages.stdenv.cc.libc_dev}/include"
              "-cxx-isystem ${pkgs.gcc.cc}/include/c++/${pkgs.gcc.version}"
              "-cxx-isystem ${pkgs.gcc.cc}/include/c++/${pkgs.gcc.version}/aarch64-unknown-linux-gnu"
            ]
            ++ pkgs.lib.map (x: "-isystem ${x}/include") libForwardingInputs
          );

          dontWrapQtApps = true;
          preFixup = ''
            wrapQtApp $out/bin/FEXConfig
          '';

          meta = with pkgs.lib; {
            description = "Fast usermode x86 and x86-64 emulator for Arm64 Linux (tuned for ${tuneArch})";
            homepage = "https://fex-emu.com/";
            license = licenses.mit;
            platforms = [ "aarch64-linux" ];
            mainProgram = "FEXBash";
          };
        });

      fex-armv8_0 = mkFex {
        pname = "fex-emu-armv8.0";
        tuneArch = armTuneArches.armv8_0;
      };
      fex-armv8_2 = mkFex {
        pname = "fex-emu-armv8.2";
        tuneArch = armTuneArches.armv8_2;
      };
      fex-armv8_4 = mkFex {
        pname = "fex-emu-armv8.4";
        tuneArch = armTuneArches.armv8_4;
      };
    in
    {
      packages.${system} = {
        fex-emu-armv8_0 = fex-armv8_0;
        fex-emu-armv8_2 = fex-armv8_2;
        fex-emu-armv8_4 = fex-armv8_4;
        default = fex-armv8_2;
        fex = fex-armv8_2;
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ fex-armv8_2 ];
        packages = [
          pkgs.ccache
          pkgs.git
        ];
        inherit (fex-armv8_2) THUNKGEN_EXTRA_FLAGS;
      };

      # NixOS module: registers the x86/x86_64 binfmt_misc entries
      nixosModules.fex =
        {
          config,
          lib,
          utils,
          ...
        }:
        let
          cfg = config.services.fex;
        in
        {
          options.services.fex = {
            enable = lib.mkEnableOption "FEX x86/x86-64 binfmt_misc registration";
            package = lib.mkOption {
              type = lib.types.package;
              default = self.packages.${system}.default;
              description = "FEX package whose bin/FEX the binfmt entries invoke";
            };
          };

          config = lib.mkIf cfg.enable {
            boot.binfmt.registrations = {
              FEX-x86_64 = utils.binfmtMagics.x86_64-linux // {
                interpreter = "${cfg.package}/bin/FEX";
                preserveArgvZero = true;
                openBinary = true;
                matchCredentials = true;
                fixBinary = true;
              };
              FEX-x86 = utils.binfmtMagics.i386-linux // {
                interpreter = "${cfg.package}/bin/FEX";
                preserveArgvZero = true;
                openBinary = true;
                matchCredentials = true;
                fixBinary = true;
              };
            };
          };
        };

      formatter.${system} = nixpkgs.legacyPackages.${system}.nixfmt-tree;
    };
}
