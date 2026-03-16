{ pkgs, lib, config, inputs, ... }:

{
  env = {
    LD_LIBRARY_PATH = lib.makeLibraryPath [
      pkgs.boost
      pkgs.glibc.dev
      # X11 and graphics libraries for JavaFX
      pkgs.xorg.libX11
      pkgs.xorg.libXext
      pkgs.xorg.libXtst
      pkgs.xorg.libXi
      pkgs.xorg.libXrender
      pkgs.xorg.libXxf86vm
      pkgs.libGL
      pkgs.freetype
      pkgs.fontconfig
      pkgs.gtk3
      pkgs.cairo
      pkgs.pango
      pkgs.gdk-pixbuf
      pkgs.atk
      pkgs.glib
    ];
    CPATH = lib.makeIncludePath [
      pkgs.boost
      pkgs.glibc.dev
      pkgs.xorg.libX11
      pkgs.xorg.libXext
      pkgs.xorg.xorgproto
    ];
    LIBRARY_PATH = lib.makeLibraryPath [
      pkgs.pkgsi686Linux.zlib
      pkgs.pkgsi686Linux.xorg.libX11
      pkgs.pkgsi686Linux.xorg.libXext
    ];
    YOSYS_GHDL_EXTENSION = pkgs.yosys-ghdl.outPath + "/share/yosys/plugins/ghdl.so";
    # 32-bit toolchain for Glide builds
    CC32 = pkgs.pkgsi686Linux.stdenv.cc.outPath + "/bin/gcc";
    CXX32 = pkgs.pkgsi686Linux.stdenv.cc.outPath + "/bin/g++";
    DOSBOXX_LOGFILE = "${config.env.DEVENV_ROOT}/output/dosbox-x-console.log";
    DOSBOXX_LOG_CONSOLE = "quiet";
    DOSBOX_TOMB_SRC = "/tmp/tr1-3dfx";
    DOSBOX_TOMB_STAGE_ROOT = "/tmp/tr1-run";
    DOSBOX_TOMB_GAME_DIR = "TOMBRAID";
    DOSBOX_TOMB_ISO = "tr1disc01.iso";
    DOSBOX_TOMB_EXE = "TOMB.EXE";
    DOSBOX_TOMB_WAIT_SECONDS = "180";
    DOSBOX_TOMB_SCREENSHOT = "${config.env.DEVENV_ROOT}/output/tomb/tombraider.png";
    DOSBOX_TOMB_LOG = "${config.env.DEVENV_ROOT}/output/tomb/tomb-screenshot.log";
  };

  packages = [
    pkgs.git
    pkgs.git-lfs
    pkgs.curl
    pkgs.stdenv.cc
    pkgs.boost
    pkgs.genimage
    pkgs.bmaptool
    pkgs.util-linux
    pkgs.parted
    pkgs.dosfstools
    pkgs.e2fsprogs
    pkgs.gnutar
    pkgs.gzip
    pkgs.mtools
    pkgs.dtc
    pkgs.iverilog
    pkgs.ghdl
    pkgs.yosys
    pkgs.yosys-ghdl
    pkgs.sby
    pkgs.yices
    pkgs.z3
    pkgs.verilator
    pkgs.surfer
    pkgs.metals
    pkgs.scalafmt
    pkgs.python3Packages.jupytext
    pkgs.hdf5
    pkgs.gdb
    # FPGA synthesis tooling
    pkgs.quartus-prime-lite
    # Glide build dependencies (32-bit C toolchain for Voodoo1)
    pkgs.nasm
    pkgs.pkgsi686Linux.stdenv.cc
    pkgs.pkgsi686Linux.dosbox-x
    pkgs.open-watcom-bin
    pkgs.djgpp
    # Verilator build acceleration
    pkgs.ccache
    pkgs.llvmPackages.clang
  ];

  languages.java = {
    jdk.package = pkgs.jdk17;
  };

  languages.scala = {
    enable = true;
    package = pkgs.scala_2_13;
    sbt.enable = true;
  };

  scripts.sim-test = {
    exec = ''exec bash ./emu/sim/run_tests.sh "$@"'';
    description = "Run Glide SDK tests against SpinalVoodoo Verilator sim";
  };

  scripts.sim-build = {
    exec = ''
      set -e
      echo "Building sim-enabled Glide library..."
      make -C emu/glide/glide2x/sst1/glide/src -f Makefile.sim all
      echo "Building test programs..."
      make -C emu/glide/glide2x/sst1/glide/tests -f Makefile.sim
      echo "Done."
    '';
    description = "Build sim-enabled Glide library and test programs";
  };

  scripts.de10-plan = {
    exec = ''
      echo "DE10 goals: docs/DE10_MILESTONES.md"
      echo "Bring-up plan: docs/DE10_BRINGUP_PLAN.md"
      echo "Deployment plan: docs/DE10_DEPLOYMENT.md"
    '';
    description = "Print DE10 planning docs";
  };

  scripts.de10-build-bitstream = {
    exec = ''exec bash ./scripts/build-bitstream-de10 "$@"'';
    description = "Build DE10 bitstream (Quartus required)";
  };

  scripts.de10-program = {
    exec = ''exec bash ./scripts/program-de10 "$@"'';
    description = "Program DE10 FPGA image remotely";
  };

  scripts.de10-deploy = {
    exec = ''exec bash ./scripts/deploy-de10.sh "$@"'';
    description = "Deploy runtime bundle to a DE10";
  };

  scripts.dosboxx-console = {
    exec = ''
      set -euo pipefail
      mkdir -p "$(dirname "$DOSBOXX_LOGFILE")"
      exec dosbox-x \
        -set "log logfile=$DOSBOXX_LOGFILE" \
        -set "dos log console=$DOSBOXX_LOG_CONSOLE" \
        "$@"
    '';
    description = "Run DOSBox-X with guest console logging enabled";
  };

  scripts.tomb-run-headless = {
    exec = ''exec bash ./scripts/run-tomb-glide-headless "$@"'';
    description = "Run Tomb Raider through DOSBox-X Glide headlessly";
  };

  scripts.tomb-run-live = {
    exec = ''exec bash ./scripts/run-tomb-glide-live "$@"'';
    description = "Run Tomb Raider through DOSBox-X Glide with a window";
  };

  scripts.tomb-screenshot = {
    exec = ''exec bash ./scripts/capture-tomb-screenshot "$@"'';
    description = "Run Tomb Raider and capture a live screenshot";
  };

    pre-commit = {
    default_stages = [ "commit" ];
    hooks = {
      scalafmt = {
        enable = true;
        description = "Format Scala code";
        entry = "${pkgs.scalafmt}/bin/scalafmt";
        files = "\\.scala$";
        language = "system";
      };
    };
  };
}
