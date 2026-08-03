{ stdenvNoCC, fetchurl }:

stdenvNoCC.mkDerivation {
  pname = "google-xls-release";
  version = "0.0.0-10106-gff755d2e5";
  src = fetchurl {
    url = "https://github.com/google/xls/releases/download/v0.0.0-10106-gff755d2e5/xls-v0.0.0-10106-gff755d2e5-linux-x64.tar.gz";
    hash = "sha256-Vhv0Te3z3agJJNpRQ1OeAS0lAlkIidQdHJro7VmUCJU=";
  };
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin"
    cp -r . "$out/bin"
    runHook postInstall
  '';
}
