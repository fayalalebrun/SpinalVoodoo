{
  description = "SpinalVoodoo XLS development environment";

  inputs.nixpkgs.url = "github:cachix/devenv-nixpkgs/rolling";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; config.allowUnfree = true; };
          xlsRelease = pkgs.callPackage ./environment/xls-release.nix { };
          xlsRuntime = pkgs.callPackage ./environment/xls.nix { };
          xls = pkgs.symlinkJoin {
            name = "google-xls-0.0.0-10106-gff755d2e5";
            paths = [ xlsRelease xlsRuntime ];
          };
        in {
          inherit xls xlsRelease xlsRuntime;
          default = xls;
          mister-thumb-breakpoint-probe = self.packages.${system}.mister-reset-breakpoint-probe.overrideAttrs {
            pname = "mister-thumb-breakpoint-probe";
            buildPhase = "$CC -mthumb -O2 -Wall -Wextra -Werror $src -o probe";
          };
          mister-reset-breakpoint-probe = pkgs.pkgsCross.armv7l-hf-multiplatform.pkgsStatic.stdenv.mkDerivation {
            pname = "mister-reset-breakpoint-probe";
            version = "1";
            src = ./diagnostics/board/arm_reset_breakpoint_probe.c;
            dontUnpack = true;
            buildPhase = "$CC -marm -O2 -Wall -Wextra -Werror $src -o probe";
            installPhase = "mkdir -p $out/bin; cp probe $out/bin/mister-reset-breakpoint-probe";
          };
          mister-core-gate = pkgs.pkgsCross.armv7l-hf-multiplatform.pkgsStatic.stdenv.mkDerivation {
            pname = "mister-core-gate";
            version = "1";
            src = ./software/runtime/mister/core_gate.c;
            dontUnpack = true;
            nativeBuildInputs = [ pkgs.pkg-config ];
            buildInputs = [ pkgs.pkgsCross.armv7l-hf-multiplatform.pkgsStatic.fuse3 ];
            buildPhase = "$CC -D_GNU_SOURCE -O2 -Wall -Wextra -Werror $src $(pkg-config --cflags --libs --static fuse3) -o core-gate";
            installPhase = "mkdir -p $out/bin; cp core-gate $out/bin/mister-core-gate";
          };

        });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; config.allowUnfree = true; };
          xls = self.packages.${system}.xls;
        in {
          mister-arm = let
            cross = pkgs.pkgsCross.armv7l-hf-multiplatform;
            sdl3 = (cross.sdl3.override {
              alsaSupport = true;
              dbusSupport = false;
              drmSupport = false;
              ibusSupport = false;
              jackSupport = false;
              libdecorSupport = false;
              libudevSupport = false;
              libusbSupport = false;
              openglSupport = false;
              pipewireSupport = false;
              pulseaudioSupport = false;
              traySupport = false;
              vulkanSupport = false;
              waylandSupport = false;
              x11Support = false;
            }).overrideAttrs (old: {
              cmakeFlags = old.cmakeFlags ++ [
                "-DSDL_ALSA_SHARED=OFF"
                "-DSDL_HIDAPI=OFF"
                "-DSDL_JOYSTICK=OFF"
                "-DSDL_HAPTIC=OFF"
                "-DSDL_SENSOR=OFF"
                "-DSDL_CAMERA=OFF"
                "-DHAVE_LINUX_INPUT_H=OFF"
                "-DHAVE_INPUT_KD=OFF"
              ];
            });
            sdl2 = (cross.SDL2.override { inherit sdl3; x11Support = false; }).overrideAttrs (old: {
              cmakeFlags = old.cmakeFlags ++ [ "-DSDL2COMPAT_X11=OFF" ];
            });
          in cross.mkShell {
            nativeBuildInputs = with pkgs; [ autoconf automake libtool pkg-config gnumake ];
            buildInputs = [ sdl2 cross.zlib cross.libpng ];
          };
          default = pkgs.mkShell {
            packages = with pkgs; [
              xls
              git
              curl
              file
              gnumake
              gcc_multi
              SDL2
              pkg-config
              fuse3
              patchelf
              nix
              qemu
              tcl
              open-watcom-bin
              pkgsi686Linux.dosbox-x
              pkgsi686Linux.zlib
              pkgsi686Linux.libx11
              pkgsi686Linux.libx11.dev
              pkgsi686Linux.libxext
              pkgsi686Linux.libxext.dev
              jdk17
              scala-cli
              scalafmt
              iverilog
              verilator
              yosys
              sby
              yices
              z3
              ccache
              linuxPackages.perf
              valgrind
              llvmPackages_19.llvm
            ];
          };
        });
    };
}
