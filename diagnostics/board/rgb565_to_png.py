#!/usr/bin/env python3
import argparse
import struct
import zlib
from pathlib import Path


def chunk(tag: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)


def expand_rgb565(pixel: int) -> bytes:
    r = (pixel >> 11) & 0x1F
    g = (pixel >> 5) & 0x3F
    b = pixel & 0x1F
    r = (r << 3) | (r >> 2)
    g = (g << 2) | (g >> 4)
    b = (b << 3) | (b >> 2)
    return bytes((r, g, b))


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert RGB565 framebuffer dump to PNG")
    parser.add_argument("raw")
    parser.add_argument("png")
    parser.add_argument("width", type=int)
    parser.add_argument("height", type=int)
    parser.add_argument("row_bytes", type=int)
    args = parser.parse_args()

    raw = Path(args.raw).read_bytes()
    needed = args.row_bytes * args.height
    if len(raw) < needed:
        raise SystemExit(f"raw framebuffer too small: have {len(raw)} need {needed}")

    rows = bytearray()
    for y in range(args.height):
        rows.append(0)
        row_base = y * args.row_bytes
        for x in range(args.width):
            pixel = struct.unpack_from("<H", raw, row_base + x * 2)[0]
            rows.extend(expand_rgb565(pixel))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png.extend(chunk(b"IHDR", struct.pack(">IIBBBBB", args.width, args.height, 8, 2, 0, 0, 0)))
    png.extend(chunk(b"IDAT", zlib.compress(bytes(rows), 9)))
    png.extend(chunk(b"IEND", b""))
    out = Path(args.png)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(png)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
