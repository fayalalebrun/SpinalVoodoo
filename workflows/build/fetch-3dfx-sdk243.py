#!/usr/bin/env python3
import argparse
import pathlib
import sys
import urllib.request
import zipfile


SDK_URL = "https://3dfxarchive.com/downloads/glide_sdk-243.zip"
WRAPPER_NAME = "gwebvgr3.exe"
NEEDED = [
    "Glide/Lib/Dos/Stack/glide2x.lib",
    "Glide/Diags/Dos/DOS4GW.EXE",
    "Glide/Src/Sst1/include/glide.h",
    "Glide/Src/Sst1/include/3DFX.H",
    "Glide/Src/Sst1/include/FXDLL.H",
    "Glide/Src/Sst1/include/FXGLOB.H",
    "Glide/Src/Sst1/include/FXOS.H",
    "Glide/Src/Sst1/include/glidesys.h",
    "Glide/Src/Sst1/include/glideutl.h",
    "Glide/Src/Sst1/include/SST1VID.H",
]


def download(url: str, dest: pathlib.Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as resp, dest.open("wb") as out:
        out.write(resp.read())


def ensure_lowercase_aliases(include_dir: pathlib.Path) -> None:
    aliases = {
        "3DFX.H": "3dfx.h",
        "FXDLL.H": "fxdll.h",
        "FXGLOB.H": "fxglob.h",
        "FXOS.H": "fxos.h",
        "SST1VID.H": "sst1vid.h",
    }
    for src_name, dst_name in aliases.items():
        src = include_dir / src_name
        dst = include_dir / dst_name
        if src.exists() and not dst.exists():
            dst.write_bytes(src.read_bytes())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dest")
    args = parser.parse_args()

    dest = pathlib.Path(args.dest).resolve()
    sdk_zip = dest / "downloads" / "glide_sdk-243.zip"
    wrapper = dest / "downloads" / WRAPPER_NAME
    marker = dest / ".ready"

    wanted_outputs = [dest / path for path in NEEDED]
    if marker.exists() and all(path.exists() for path in wanted_outputs):
        return 0

    download(SDK_URL, sdk_zip)

    with zipfile.ZipFile(sdk_zip) as outer:
        with outer.open(WRAPPER_NAME) as src, wrapper.open("wb") as out:
            out.write(src.read())

    with zipfile.ZipFile(wrapper) as inner:
        for path in NEEDED:
            inner.extract(path, dest)

    ensure_lowercase_aliases(dest / "Glide" / "Src" / "Sst1" / "include")
    marker.write_text("ok\n", encoding="ascii")
    return 0


if __name__ == "__main__":
    sys.exit(main())
