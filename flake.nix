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
          mister-keyboard = let
            receiver = pkgs.pkgsCross.armv7l-hf-multiplatform.pkgsStatic.stdenv.mkDerivation {
              pname = "mister-keyboard-receiver";
              version = "1";
              src = ./diagnostics/board/mister-keyboard-receiver.c;
              dontUnpack = true;
              buildPhase = "$CC -O2 -Wall -Wextra -Werror -std=gnu11 $src -o receiver";
              installPhase = "mkdir -p $out/bin; cp receiver $out/bin/mister-keyboard-receiver";
            };
          in pkgs.writeShellApplication {
            name = "mister-keyboard";
            excludeShellChecks = [ "SC2029" ];
            runtimeInputs = [ pkgs.openssh pkgs.python3 pkgs.ffmpeg-headless ];
            text = ''
              target="''${MISTER_HOST:-root@192.168.2.86}"
              ssh_options=(-o BatchMode=yes -o ConnectTimeout=10)
              identity="''${MISTER_SSH_KEY:-$HOME/.ssh/monumental}"
              if [[ -f "$identity" ]]; then ssh_options+=(-i "$identity"); fi
              if [[ -f "$PWD/output/mister/sd-image/known_hosts" ]]; then
                ssh_options+=(-o "UserKnownHostsFile=$PWD/output/mister/sd-image/known_hosts")
              fi
              remote=$(ssh "''${ssh_options[@]}" "$target" 'mktemp -d /tmp/spinal-keyboard.XXXXXXXX')
              if [[ ! "$remote" =~ ^/tmp/spinal-keyboard\.[a-zA-Z0-9]+$ ]]; then exit 1; fi
              cleanup() {
                ssh "''${ssh_options[@]}" "$target" "rm -f '$remote/receiver'; rmdir '$remote'" || true
              }
              trap cleanup EXIT
              scp "''${ssh_options[@]}" ${receiver}/bin/mister-keyboard-receiver "$target:$remote/receiver"
              ssh "''${ssh_options[@]}" "$target" "chmod 700 '$remote/receiver'"
              export SDL2_LIBRARY=${pkgs.SDL2}/lib/libSDL2.so
              export MISTER_SSH_OPTIONS="''${ssh_options[*]}"
              ${pkgs.python3}/bin/python3 ${./diagnostics/board/mister-keyboard} "$target" \
                --remote-command "$remote/receiver" "$@"
            '';
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
