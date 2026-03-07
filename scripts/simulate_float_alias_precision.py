#!/usr/bin/env python3

import argparse
import math
import sqlite3
from dataclasses import replace

from query_shared_texture_edges import (
    Triangle,
    canonical_edge,
    compute_ref_texel_address,
    load_ref_triangles,
    sample_edge_pixels,
)


def trunc_toward_zero_div_pow2(value: int, shift: int) -> int:
    if value >= 0:
        return value >> shift
    return -((-value) >> shift)


def quantize_float_path_to_dut_regs(tri: Triangle) -> Triangle:
    start_s = trunc_toward_zero_div_pow2(tri.start_s, 14)
    start_t = trunc_toward_zero_div_pow2(tri.start_t, 14)
    start_w = trunc_toward_zero_div_pow2(tri.start_w, 2)
    dsdx = trunc_toward_zero_div_pow2(tri.dsdx, 14)
    dtdx = trunc_toward_zero_div_pow2(tri.dtdx, 14)
    dwdx = trunc_toward_zero_div_pow2(tri.dwdx, 2)
    dsdy = trunc_toward_zero_div_pow2(tri.dsdy, 14)
    dtdy = trunc_toward_zero_div_pow2(tri.dtdy, 14)
    dwdy = trunc_toward_zero_div_pow2(tri.dwdy, 2)
    return replace(
        tri,
        start_s=start_s << 14,
        start_t=start_t << 14,
        start_w=start_w << 2,
        dsdx=dsdx << 14,
        dtdx=dtdx << 14,
        dwdx=dwdx << 2,
        dsdy=dsdy << 14,
        dtdy=dtdy << 14,
        dwdy=dwdy << 2,
    )


def linear_addr(tri: Triangle, x: int, y: int) -> tuple[int, int, int]:
    s, t, lod = compute_ref_texel_address(tri, x, y)
    lod_dim_bits = max(0, 8 - lod)
    aspect = (tri.tlod_0 >> 21) & 0x3
    narrow_dim_bits = max(0, lod_dim_bits - aspect)
    row_shift = lod_dim_bits if (tri.tlod_0 & (1 << 20)) else narrow_dim_bits if aspect else lod_dim_bits
    return s + (t << row_shift), s, t


def shared_edge(tri_a: Triangle, tri_b: Triangle):
    edges_a = {canonical_edge(*edge): edge for edge in ((tri_a.vertices[0], tri_a.vertices[1]), (tri_a.vertices[1], tri_a.vertices[2]), (tri_a.vertices[2], tri_a.vertices[0]))}
    edges_b = {canonical_edge(*edge): edge for edge in ((tri_b.vertices[0], tri_b.vertices[1]), (tri_b.vertices[1], tri_b.vertices[2]), (tri_b.vertices[2], tri_b.vertices[0]))}
    common = set(edges_a) & set(edges_b)
    if not common:
        raise ValueError("triangles do not share an exact edge")
    return next(iter(common))


def analyze_pair(tri_a: Triangle, tri_b: Triangle) -> None:
    q_a = quantize_float_path_to_dut_regs(tri_a)
    q_b = quantize_float_path_to_dut_regs(tri_b)
    edge = shared_edge(tri_a, tri_b)
    points = sample_edge_pixels(*edge)

    ref_dists = []
    q_dists = []
    max_tri_a_drift = (0, None, None)
    max_tri_b_drift = (0, None, None)

    for p in points:
        ref_a = linear_addr(tri_a, *p)
        ref_b = linear_addr(tri_b, *p)
        q_a_addr = linear_addr(q_a, *p)
        q_b_addr = linear_addr(q_b, *p)
        ref_dists.append(abs(ref_a[0] - ref_b[0]))
        q_dists.append(abs(q_a_addr[0] - q_b_addr[0]))

        drift_a = abs(q_a_addr[0] - ref_a[0])
        drift_b = abs(q_b_addr[0] - ref_b[0])
        if drift_a > max_tri_a_drift[0]:
            max_tri_a_drift = (drift_a, p, (ref_a, q_a_addr))
        if drift_b > max_tri_b_drift[0]:
            max_tri_b_drift = (drift_b, p, (ref_b, q_b_addr))

    print(f"pair draw={tri_a.draw_id} prims=({tri_a.primitive_id},{tri_b.primitive_id})")
    print(f"edge=(({edge[0][0]/16:.2f},{edge[0][1]/16:.2f}) -> ({edge[1][0]/16:.2f},{edge[1][1]/16:.2f})) samples={len(points)}")
    print(f"ref shared-edge max_addr={max(ref_dists)} avg_addr={sum(ref_dists)/len(ref_dists):.3f}")
    print(f"prequantized shared-edge max_addr={max(q_dists)} avg_addr={sum(q_dists)/len(q_dists):.3f}")

    def format_drift(prim_id: int, drift):
        if drift[2] is None:
            return f"tri {prim_id} drift max_addr=0"
        return (
            f"tri {prim_id} drift max_addr={drift[0]} at {drift[1]} "
            f"ref={drift[2][0]} quantized={drift[2][1]}"
        )

    print(format_drift(tri_a.primitive_id, max_tri_a_drift))
    print(format_drift(tri_b.primitive_id, max_tri_b_drift))
    print("raw param deltas after pre-quantization:")
    for label in ("start_s", "start_t", "start_w", "dsdx", "dtdx", "dwdx", "dsdy", "dtdy", "dwdy"):
        print(f"  {label}: A {getattr(q_a, label)-getattr(tri_a, label)}  B {getattr(q_b, label)-getattr(tri_b, label)}")
    print()


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare high-precision float triangle params against pre-quantized DUT-style fixed params.")
    parser.add_argument("--db", required=True, help="SQLite DB with ref_triangles")
    parser.add_argument(
        "--pairs",
        default="0:231:232,0:306:307,1:726:727",
        help="Comma-separated draw:primA:primB list",
    )
    args = parser.parse_args()

    conn = sqlite3.connect(args.db)
    triangles = {(t.draw_id, t.primitive_id): t for t in load_ref_triangles(conn)}
    conn.close()

    for spec in args.pairs.split(","):
        draw, prim_a, prim_b = (int(x) for x in spec.split(":"))
        analyze_pair(triangles[(draw, prim_a)], triangles[(draw, prim_b)])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
