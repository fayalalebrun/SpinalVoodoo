{
  lib,
  stdenv,
  buildBazelPackage,
  fetchFromGitHub,
  bazel_7,
  jdk17_headless,
  python3,
  git,
  clang_19,
}:

let
  registry = fetchFromGitHub {
    owner = "bazelbuild";
    repo = "bazel-central-registry";
    rev = "ea29a02395cabdfa4c4aacf109d67de6beb700af";
    hash = "sha256-6AxL0R+hcf+ye3sb8uDn0KcRvvO5Qh8pvD81bVZoU80=";
  };
in
buildBazelPackage rec {
  pname = "google-xls";
  version = "0.0.0-10106-gff755d2e5";

  src = fetchFromGitHub {
    owner = "google";
    repo = "xls";
    rev = "ff755d2e5";
    hash = "sha256-txGqU00lyIv7xoerujmnSr3nv7onAz80ifA9lYdXzcM=";
  };

  patches = [ ../vendor/patches/xls/xls-tools.patch ../vendor/patches/xls/xls-runtime-performance.patch ];
  postPatch = ''
    printf '7.6.0\n' > .bazelversion
    cp ${../software/simulation/xls/voodoo-xls-runtime.cc} xls/tools/voodoo_xls_runtime.cc
    cp ${../software/simulation/xls/xls-bridge-protocol.h} xls/tools/xls-bridge-protocol.h
    cp ${../software/simulation/xls/xls-bridge-server.inc} xls/tools/xls-bridge-server.inc
    patchShebangs xls/build_rules/clang-format.sh
  '';

  bazel = bazel_7;
  bazelFlags = [ "--registry" "file://${registry}" ];
  removeRulesCC = false;
  dontAddBazelOpts = true;
  nativeBuildInputs = [ jdk17_headless python3 git clang_19 ];

  fetchAttrs = {
    __structuredAttrs = true;
    hash = "sha256-0pLx/oRMJ/CuP07wNLyXgDO5FxIbZYW4vhdIUg3FFgc=";
    unsafeDiscardReferences.out = true;
  };

  bazelTargets = [
    "//xls/jit:aot_compiler_main"
    "//xls/tools:eval_proc_main"
    "//xls/tools:voodoo_xls_runtime"
  ];
  bazelBuildFlags = [ "-c" "opt" "--spawn_strategy=local" ];

  buildAttrs.preConfigure = ''
    export CC=${clang_19}/bin/clang
    export CXX=${clang_19}/bin/clang++
  '';

  buildAttrs.preBuild = ''
    patchShebangs "$bazelOut/external"
  '';

  buildAttrs.installPhase = ''
    runHook preInstall
    install -Dm755 bazel-bin/xls/jit/aot_compiler_main "$out/bin/aot_compiler_main"
    install -Dm755 bazel-bin/xls/tools/eval_proc_main "$out/bin/eval_proc_main"
    install -Dm755 bazel-bin/xls/tools/voodoo_xls_runtime "$out/bin/voodoo_xls_runtime"
    runHook postInstall
  '';

  meta = {
    description = "Google XLS tools with proc evaluation and AOT compilation";
    homepage = "https://github.com/google/xls";
    license = lib.licenses.asl20;
    platforms = lib.platforms.linux;
  };
}
