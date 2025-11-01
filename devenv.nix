{ pkgs, lib, config, inputs, ... }:

{
  env = {
    LD_LIBRARY_PATH = lib.makeLibraryPath [
      pkgs.boost
      pkgs.glibc.dev
    ];
    CPATH = lib.makeIncludePath [
      pkgs.boost
      pkgs.glibc.dev
    ];
    YOSYS_GHDL_EXTENSION = pkgs.yosys-ghdl.outPath + "/share/yosys/plugins/ghdl.so";
  };

  packages = [
    pkgs.git
    pkgs.git-lfs
    pkgs.stdenv.cc
    pkgs.boost
    pkgs.iverilog
    pkgs.ghdl
    pkgs.yosys
    pkgs.yosys-ghdl
    pkgs.symbiyosys
    pkgs.yices
    pkgs.z3
    pkgs.verilator
    pkgs.surfer
    pkgs.metals
    pkgs.scalafmt
    pkgs.python3Packages.jupytext
    pkgs.hdf5
  ];

  languages.java = {
    jdk.package = pkgs.jdk17;
  };

  languages.scala = {
    enable = true;
    package = pkgs.scala_2_13;
    sbt.enable = true;
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
