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
          xlsRelease = pkgs.callPackage ./nix/xls-release.nix { };
          xlsRuntime = pkgs.callPackage ./nix/xls.nix { };
          xls = pkgs.symlinkJoin {
            name = "google-xls-0.0.0-10106-gff755d2e5";
            paths = [ xlsRelease xlsRuntime ];
          };
        in {
          inherit xls xlsRelease xlsRuntime;
          default = xls;
        });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; config.allowUnfree = true; };
          xls = self.packages.${system}.xls;
        in {
          default = pkgs.mkShell {
            packages = with pkgs; [
              xls
              git
              curl
              gnumake
              gcc_multi
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
