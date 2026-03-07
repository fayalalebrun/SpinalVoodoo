#!/usr/bin/env python3
"""Small gradient-walk simulator for suspected interpolation mismatch.

Compares two walkers for one gradient component (e.g. S or T):
1) 86Box-style per-scanline anchor from edge intersections.
2) Serpentine bounding-box walk (RTL style traversal order).

The script reports per-pixel drift on the shared covered pixels.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, List, Tuple

FRAC_BITS = 18  # 14.18 texture coordinate format


@dataclass(frozen=True)
class Tri12p4:
    # Vertices in 12.4 fixed-point integer units, sorted by Y: A(top), B(mid), C(bottom)
    ax: int
    ay: int
    bx: int
    by: int
    cx: int
    cy: int


@dataclass(frozen=True)
class GradScenario:
    name: str
    tri: Tri12p4
    start: int
    d_dx: int
    d_dy: int
    xdir: int = 1  # +1 for sign=0 in 86Box, -1 for sign=1
    wrap_bits: int | None = None


def c_div_toward_zero(a: int, b: int) -> int:
    if b == 0:
        return 0
    return int(a / b)


def wrap_signed(v: int, bits: int) -> int:
    mask = (1 << bits) - 1
    v &= mask
    sign = 1 << (bits - 1)
    return v - (1 << bits) if (v & sign) else v


def round_vertex_x_to_pixel(ax_12p4: int) -> int:
    # Matches ((vertexAx << 12) + 0x7000) >> 16 in 86Box (round-half-down in pixel units)
    return ((ax_12p4 << 12) + 0x7000) >> 16


def y_start(ay_12p4: int) -> int:
    # Matches vertexAy_adjusted = (vertexAy + 7) >> 4 in 86Box
    return (ay_12p4 + 7) >> 4


def y_end(cy_12p4: int) -> int:
    # Matches vertexCy_adjusted = (vertexCy + 7) >> 4 (exclusive loop end)
    return (cy_12p4 + 7) >> 4


def ref_scanline_pixels(sc: GradScenario) -> Dict[Tuple[int, int], int]:
    t = sc.tri
    out: Dict[Tuple[int, int], int] = {}

    dx_ab = c_div_toward_zero((((t.bx << 12) - (t.ax << 12)) << 4), (t.by - t.ay)) if t.by != t.ay else 0
    dx_ac = c_div_toward_zero((((t.cx << 12) - (t.ax << 12)) << 4), (t.cy - t.ay)) if t.cy != t.ay else 0
    dx_bc = c_div_toward_zero((((t.cx << 12) - (t.bx << 12)) << 4), (t.cy - t.by)) if t.cy != t.by else 0

    ys = y_start(t.ay)
    ye = y_end(t.cy)
    ax_pix = round_vertex_x_to_pixel(t.ax)

    for y in range(ys, ye):
        real_y = (y << 4) + 8
        x = (t.ax << 12) + ((dx_ac * (real_y - t.ay)) >> 4)
        if real_y < t.by:
            x2 = (t.ax << 12) + ((dx_ab * (real_y - t.ay)) >> 4)
        else:
            x2 = (t.bx << 12) + ((dx_bc * (real_y - t.by)) >> 4)

        if sc.xdir > 0:
            x2 -= 1 << 16
        else:
            x -= 1 << 16

        x0 = (x + 0x7000) >> 16
        x1 = (x2 + 0x7000) >> 16

        if sc.xdir > 0 and x1 < x0:
            continue
        if sc.xdir < 0 and x1 > x0:
            continue

        base_y = sc.start + sc.d_dy * (y - ys)
        dx = x0 - ax_pix
        cur = base_y + sc.d_dx * dx

        if sc.xdir > 0:
            xs = range(x0, x1 + 1)
            step = sc.d_dx
        else:
            xs = range(x0, x1 - 1, -1)
            step = -sc.d_dx

        for px in xs:
            out[(px, y)] = cur
            cur += step

    return out


def serpentine_bbox_pixels(sc: GradScenario) -> Dict[Tuple[int, int], int]:
    t = sc.tri
    out: Dict[Tuple[int, int], int] = {}

    xmin = min(t.ax, t.bx, t.cx) >> 4
    xmax = (max(t.ax, t.bx, t.cx) + 15) >> 4
    ys = y_start(t.ay)
    ye = y_end(t.cy)
    ax_pix = round_vertex_x_to_pixel(t.ax)

    start = sc.start + sc.d_dx * (xmin - ax_pix)
    if sc.wrap_bits is not None:
        start = wrap_signed(start, sc.wrap_bits)

    cur = start
    dir_sign = 1

    for _row, y in enumerate(range(ys, ye)):
        if dir_sign > 0:
            x_iter = range(xmin, xmax + 1)
            dstep = sc.d_dx
        else:
            x_iter = range(xmax, xmin - 1, -1)
            dstep = -sc.d_dx

        width = xmax - xmin + 1
        for i, x in enumerate(x_iter):
            out[(x, y)] = cur
            if i != width - 1:
                cur = cur + dstep
                if sc.wrap_bits is not None:
                    cur = wrap_signed(cur, sc.wrap_bits)

        cur = cur + sc.d_dy
        if sc.wrap_bits is not None:
            cur = wrap_signed(cur, sc.wrap_bits)

        dir_sign *= -1

    return out


def texels(v: int) -> float:
    return v / float(1 << FRAC_BITS)


def compare(sc: GradScenario) -> None:
    ref = ref_scanline_pixels(sc)
    rtl = serpentine_bbox_pixels(sc)

    shared = sorted(set(ref.keys()) & set(rtl.keys()))
    if not shared:
        print(f"[{sc.name}] No shared pixels to compare")
        return

    diffs: List[Tuple[int, Tuple[int, int], int, int]] = []
    for p in shared:
        rv = ref[p]
        sv = rtl[p]
        d = abs(rv - sv)
        if d:
            diffs.append((d, p, rv, sv))

    print(f"[{sc.name}]")
    print(f"  compared_pixels={len(shared)}")
    print(f"  mismatched_pixels={len(diffs)}")

    if not diffs:
        print("  max_raw_diff=0")
        print("  conclusion: walkers are equivalent for this case")
        return

    diffs.sort(reverse=True)
    max_d, max_p, rv, sv = diffs[0]
    avg_d = sum(d for d, *_ in diffs) / len(diffs)

    print(f"  max_raw_diff={max_d}")
    print(f"  max_texel_diff={texels(max_d):.6f}")
    print(f"  avg_raw_diff={avg_d:.2f}")
    print(f"  worst_pixel={max_p} ref={rv} sim={sv}")
    print("  top_mismatches:")
    for d, p, r, s in diffs[:5]:
        print(
            f"    p={p} raw={d} tex={texels(d):.6f} "
            f"ref={r} ({texels(r):.3f}) sim={s} ({texels(s):.3f})"
        )


def main() -> None:
    tri = Tri12p4(
        ax=int(102.25 * 16),
        ay=int(34.50 * 16),
        bx=int(548.75 * 16),
        by=int(126.25 * 16),
        cx=int(176.50 * 16),
        cy=int(422.75 * 16),
    )

    # Scenario A: realistic in-range 32-bit value domain -> should match exactly.
    nominal = GradScenario(
        name="nominal_32bit_range",
        tri=tri,
        start=0x12345678,
        d_dx=180_000,   # ~0.6866 texels/pixel
        d_dy=-11_000,   # ~-0.0420 texels/pixel
        xdir=1,
        wrap_bits=None,
    )

    # Scenario B: perspective-heavy large start value (fits 64-bit ref, overflows 32-bit RTL path).
    wrapped = GradScenario(
        name="large_start_64v32",
        tri=tri,
        start=(1 << 35) + 1_234_567,
        d_dx=180_000,
        d_dy=-11_000,
        xdir=1,
        wrap_bits=32,
    )

    compare(nominal)
    print()
    compare(wrapped)


if __name__ == "__main__":
    main()
