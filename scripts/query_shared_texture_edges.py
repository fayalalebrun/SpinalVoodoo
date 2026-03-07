#!/usr/bin/env python3

import argparse
import math
import sqlite3
from dataclasses import dataclass
from itertools import combinations
from typing import Dict, List, Optional, Tuple


LOGTABLE = [
    0x00, 0x01, 0x02, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0B, 0x0C, 0x0E, 0x0F, 0x10, 0x12, 0x13, 0x15,
    0x16, 0x17, 0x19, 0x1A, 0x1B, 0x1D, 0x1E, 0x1F, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27, 0x28, 0x2A,
    0x2B, 0x2C, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3A, 0x3B, 0x3D, 0x3E,
    0x3F, 0x40, 0x41, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x50, 0x51,
    0x52, 0x53, 0x54, 0x55, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x60, 0x61, 0x62, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
    0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA2, 0xA3,
    0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2,
    0xB3, 0xB4, 0xB5, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
    0xC1, 0xC2, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCD,
    0xCE, 0xCF, 0xD0, 0xD1, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDA,
    0xDB, 0xDC, 0xDD, 0xDE, 0xDE, 0xDF, 0xE0, 0xE1, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE8, 0xE9, 0xEA, 0xEB, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF2, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF5, 0xF6, 0xF7, 0xF7, 0xF8, 0xF9, 0xFA, 0xFA, 0xFB, 0xFC, 0xFD, 0xFD, 0xFE, 0xFF,
]

TEXTUREMODE_TCLAMPS = 1 << 6
TEXTUREMODE_TCLAMPT = 1 << 7
LOD_S_IS_WIDER = 1 << 20
LOD_TMIRROR_S = 1 << 28
LOD_TMIRROR_T = 1 << 29


@dataclass(frozen=True)
class Triangle:
    source: str
    draw_id: int
    primitive_id: int
    textured: int
    sign: int
    vertex_ax: int
    vertex_ay: int
    vertex_bx: int
    vertex_by: int
    vertex_cx: int
    vertex_cy: int
    fbz_color_path: int
    texture_mode_0: int
    tlod_0: int
    tex_base_addr_0: int
    tex_base_addr_1_0: int
    tex_base_addr_2_0: int
    tex_base_addr_38_0: int
    tformat_0: int
    start_s: int
    start_t: int
    start_w: int
    hi_start_s: Optional[int]
    hi_start_t: Optional[int]
    x_start: int
    y_start: int
    dsdx: int
    dtdx: int
    dwdx: int
    dsdy: int
    dtdy: int
    hi_dsdx: Optional[int]
    hi_dtdx: Optional[int]
    hi_dsdy: Optional[int]
    hi_dtdy: Optional[int]
    dwdy: int

    @property
    def vertices(self) -> Tuple[Tuple[int, int], Tuple[int, int], Tuple[int, int]]:
        return (
            (self.vertex_ax, self.vertex_ay),
            (self.vertex_bx, self.vertex_by),
            (self.vertex_cx, self.vertex_cy),
        )

    @property
    def texture_key(self) -> Tuple[int, ...]:
        return (
            self.tex_base_addr_0,
            self.tex_base_addr_1_0,
            self.tex_base_addr_2_0,
            self.tex_base_addr_38_0,
            self.texture_mode_0,
            self.tlod_0,
            self.tformat_0,
        )

    @property
    def area_pixels(self) -> float:
        ax, ay = self.vertex_ax / 16.0, self.vertex_ay / 16.0
        bx, by = self.vertex_bx / 16.0, self.vertex_by / 16.0
        cx, cy = self.vertex_cx / 16.0, self.vertex_cy / 16.0
        return abs((bx - ax) * (cy - ay) - (by - ay) * (cx - ax)) * 0.5


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Find shared-edge textured triangle pairs.")
    parser.add_argument("--db", required=True, help="SQLite DB produced by export_dut_trace_sqlite.py")
    parser.add_argument("--limit", type=int, default=20, help="Number of pairs to print")
    parser.add_argument(
        "--source",
        choices=("ref", "dut", "both"),
        default="both",
        help="Which triangle source to query.",
    )
    return parser.parse_args()


def round_half_down_12_4(value: int) -> int:
    return (value + 7) >> 4


def fastlog(val: int) -> int:
    if val <= 0 or (val & (1 << 63)):
        return 0x80000000
    oldval = val
    exp = 63
    while exp >= 32 and not (val & 0xFFFFFFFF00000000):
        exp -= 32
        val <<= 32
    if not (val & 0xFFFF000000000000):
        exp -= 16
        val <<= 16
    if not (val & 0xFF00000000000000):
        exp -= 8
        val <<= 8
    if not (val & 0xF000000000000000):
        exp -= 4
        val <<= 4
    if not (val & 0xC000000000000000):
        exp -= 2
        val <<= 2
    if not (val & 0x8000000000000000):
        exp -= 1
        val <<= 1
    if exp >= 8:
        frac = (oldval >> (exp - 8)) & 0xFF
    else:
        frac = (oldval << (8 - exp)) & 0xFF
    return (exp << 8) | LOGTABLE[frac]


def signed_div_floor_pow2(value: int, shift: int) -> int:
    return value >> shift


def triangle_base_values(tri: Triangle) -> Tuple[int, int, int, int, int]:
    dx_sub = 8 - (tri.vertex_ax & 0xF)
    if (tri.vertex_ax & 0xF) > 8:
        dx_sub += 16
    dy_sub = 8 - (tri.vertex_ay & 0xF)
    if (tri.vertex_ay & 0xF) > 8:
        dy_sub += 16
    base_s = tri.start_s
    base_t = tri.start_t
    base_w = tri.start_w
    if tri.fbz_color_path & (1 << 26):
        base_s += (dx_sub * tri.dsdx + dy_sub * tri.dsdy) >> 4
        base_t += (dx_sub * tri.dtdx + dy_sub * tri.dtdy) >> 4
        base_w += (dx_sub * tri.dwdx + dy_sub * tri.dwdy) >> 4
    ax_pix = round_half_down_12_4(tri.vertex_ax)
    ay_pix = round_half_down_12_4(tri.vertex_ay)
    return base_s, base_t, base_w, ax_pix, ay_pix


def base_lod_8_8(tri: Triangle) -> int:
    tempdx = (tri.dsdx >> 14) * (tri.dsdx >> 14) + (tri.dtdx >> 14) * (tri.dtdx >> 14)
    tempdy = (tri.dsdy >> 14) * (tri.dsdy >> 14) + (tri.dtdy >> 14) * (tri.dtdy >> 14)
    temp_lod = max(tempdx, tempdy)
    if temp_lod <= 0:
        lod = 0
    else:
        lod = int(math.log2(temp_lod / float(1 << 36)) * 256)
    lod >>= 2
    lodbias = (tri.tlod_0 >> 12) & 0x3F
    if lodbias & 0x20:
        lodbias |= ~0x3F
    return lod + (lodbias << 6)


def dut_base_lod_8_8(tri: Triangle) -> int:
    tempdx = tri.dsdx * tri.dsdx + tri.dtdx * tri.dtdx
    tempdy = tri.dsdy * tri.dsdy + tri.dtdy * tri.dtdy
    temp_lod = max(tempdx, tempdy)
    if temp_lod <= 0:
        lod = 0
    else:
        lod = int(math.log2(temp_lod) * 256) - (36 * 256)
    lod >>= 2
    lodbias = (tri.tlod_0 >> 12) & 0x3F
    if lodbias & 0x20:
        lodbias |= ~0x3F
    return lod + (lodbias << 6)


def clamp_lod(lod_8_8: int, tri: Triangle) -> int:
    lod_min = (tri.tlod_0 & 0x3F) << 6
    lod_max = ((tri.tlod_0 >> 6) & 0x3F) << 6
    lod_max = min(lod_max, 0x800)
    return min(max(lod_8_8, lod_min), lod_max)


def compute_ref_texel_address(tri: Triangle, x: int, y: int) -> Tuple[int, int, int]:
    base_s, base_t, base_w, ax_pix, ay_pix = triangle_base_values(tri)
    s_raw = base_s + (x - ax_pix) * tri.dsdx + (y - ay_pix) * tri.dsdy
    t_raw = base_t + (x - ax_pix) * tri.dtdx + (y - ay_pix) * tri.dtdy
    w_raw = base_w + (x - ax_pix) * tri.dwdx + (y - ay_pix) * tri.dwdy

    perspective = tri.texture_mode_0 & 1
    lod_8_8 = base_lod_8_8(tri)
    if perspective:
        reciprocal = (1 << 48) // w_raw if w_raw else 0
        tex_s_x4 = (((((s_raw + (1 << 13)) >> 14) * reciprocal) + (1 << 29)) >> 30)
        tex_t_x4 = (((((t_raw + (1 << 13)) >> 14) * reciprocal) + (1 << 29)) >> 30)
        lod_8_8 += fastlog(reciprocal) - (19 << 8)
    else:
        tex_s_x4 = signed_div_floor_pow2(s_raw, 28)
        tex_t_x4 = signed_div_floor_pow2(t_raw, 28)

    lod_8_8 = clamp_lod(lod_8_8, tri)
    lod_level = max(0, min(8, lod_8_8 >> 8))

    if tri.tlod_0 & LOD_TMIRROR_S and (tex_s_x4 & 0x1000):
        tex_s_x4 = ~tex_s_x4
    if tri.tlod_0 & LOD_TMIRROR_T and (tex_t_x4 & 0x1000):
        tex_t_x4 = ~tex_t_x4

    bilinear = (tri.texture_mode_0 & 0x6) != 0
    if bilinear:
        tex_s_x4 -= 1 << (3 + lod_level)
        tex_t_x4 -= 1 << (3 + lod_level)

    s_addr = tex_s_x4 >> (4 + lod_level)
    t_addr = tex_t_x4 >> (4 + lod_level)

    aspect = (tri.tlod_0 >> 21) & 0x3
    width = 256
    height = 256
    if tri.tlod_0 & LOD_S_IS_WIDER:
        height >>= aspect
    else:
        width >>= aspect
    width = max(1, width >> lod_level)
    height = max(1, height >> lod_level)
    w_mask = width - 1
    h_mask = height - 1

    if s_addr & ~w_mask:
        if tri.texture_mode_0 & TEXTUREMODE_TCLAMPS:
            s_addr = max(0, min(s_addr, w_mask))
        else:
            s_addr &= w_mask
    if t_addr & ~h_mask:
        if tri.texture_mode_0 & TEXTUREMODE_TCLAMPT:
            t_addr = max(0, min(t_addr, h_mask))
        else:
            t_addr &= h_mask

    return s_addr, t_addr, lod_level


def dut_recip_interp(abs_oow: int) -> Tuple[int, int]:
    clz = 64 - abs_oow.bit_length()
    msb_pos = 63 - clz
    norm = abs_oow << clz
    index = (norm >> 55) & 0xFF
    frac = (norm >> 47) & 0xFF
    recip_base = round((1 << 24) / (256.0 + index))
    recip_next = round((1 << 24) / (256.0 + index + 1))
    interp = recip_base - (((recip_base - recip_next) * frac) >> 8)
    return msb_pos, interp


def wrap_or_clamp(coord: int, size_bits: int, clamp_enable: bool) -> int:
    size = 1 << size_bits
    max_val = size - 1
    if clamp_enable:
        return max(0, min(coord, max_val))
    return coord & max_val


def compute_dut_texel_address(tri: Triangle, x: int, y: int) -> Tuple[int, int, int]:
    x0 = tri.x_start >> 4
    y0 = tri.y_start >> 4
    if tri.hi_start_s is not None and tri.hi_start_t is not None and tri.hi_dsdx is not None and tri.hi_dtdx is not None and tri.hi_dsdy is not None and tri.hi_dtdy is not None:
        s_raw = (tri.hi_start_s + (x - x0) * tri.hi_dsdx + (y - y0) * tri.hi_dsdy) >> 12
        t_raw = (tri.hi_start_t + (x - x0) * tri.hi_dtdx + (y - y0) * tri.hi_dtdy) >> 12
    else:
        s_raw = tri.start_s + (x - x0) * tri.dsdx + (y - y0) * tri.dsdy
        t_raw = tri.start_t + (x - x0) * tri.dtdx + (y - y0) * tri.dtdy
    w_raw = tri.start_w + (x - x0) * tri.dwdx + (y - y0) * tri.dwdy

    perspective = tri.texture_mode_0 & 1
    persp_lod_adjust = 0
    if not perspective:
        tex_s_x4 = s_raw >> 14
        tex_t_x4 = t_raw >> 14
    else:
        valid_oow = w_raw >= 0 and w_raw != 0
        if valid_oow:
            msb_pos, interp = dut_recip_interp(abs(w_raw))
            tex_s_x4 = (s_raw * interp) >> msb_pos
            tex_t_x4 = (t_raw * interp) >> msb_pos
            persp_lod_adjust = ((27 - msb_pos) << 8) - LOGTABLE[((abs(w_raw) << (64 - abs(w_raw).bit_length())) >> 55) & 0xFF]
        else:
            tex_s_x4 = 0
            tex_t_x4 = 0

    lod_8_8 = clamp_lod(dut_base_lod_8_8(tri) + persp_lod_adjust, tri)
    lod_level = max(0, min(8, lod_8_8 >> 8))

    s_point = tex_s_x4 >> (4 + lod_level)
    t_point = tex_t_x4 >> (4 + lod_level)
    adj_s = tex_s_x4 - (1 << (3 + lod_level))
    adj_t = tex_t_x4 - (1 << (3 + lod_level))
    s_scaled = adj_s >> lod_level
    t_scaled = adj_t >> lod_level
    si = s_scaled >> 4
    ti = t_scaled >> 4
    clamp_to_zero = bool((tri.texture_mode_0 & (1 << 3)) and w_raw < 0)
    if clamp_to_zero:
        s_point = t_point = si = ti = 0
    aspect = (tri.tlod_0 >> 21) & 0x3
    lod_dim_bits = max(0, 8 - lod_level)
    narrow_dim_bits = max(0, lod_dim_bits - aspect)
    tex_width_bits = lod_dim_bits if not (aspect != 0 and not (tri.tlod_0 & LOD_S_IS_WIDER)) else narrow_dim_bits
    tex_height_bits = narrow_dim_bits if (tri.tlod_0 & LOD_S_IS_WIDER) else lod_dim_bits
    clamp_s = bool(tri.texture_mode_0 & TEXTUREMODE_TCLAMPS)
    clamp_t = bool(tri.texture_mode_0 & TEXTUREMODE_TCLAMPT)
    bilinear = bool(tri.texture_mode_0 & 0x6)
    if bilinear:
        tex_x = wrap_or_clamp(si, tex_width_bits, clamp_s)
        tex_y = wrap_or_clamp(ti, tex_height_bits, clamp_t)
    else:
        tex_x = wrap_or_clamp(s_point, tex_width_bits, clamp_s)
        tex_y = wrap_or_clamp(t_point, tex_height_bits, clamp_t)
    return tex_x, tex_y, lod_level


def dut_row_shift_bits(tri: Triangle, lod_level: int) -> int:
    lod_dim_bits = max(0, 8 - lod_level)
    aspect = (tri.tlod_0 >> 21) & 0x3
    narrow_dim_bits = max(0, lod_dim_bits - aspect)
    if tri.tlod_0 & LOD_S_IS_WIDER:
        return lod_dim_bits
    if aspect != 0:
        return narrow_dim_bits
    return lod_dim_bits


def sample_edge_pixels(a: Tuple[int, int], b: Tuple[int, int]) -> List[Tuple[int, int]]:
    ax, ay = a[0] / 16.0, a[1] / 16.0
    bx, by = b[0] / 16.0, b[1] / 16.0
    length = math.hypot(bx - ax, by - ay)
    steps = max(2, int(math.ceil(length)) + 1)
    points = []
    seen = set()
    for i in range(steps):
        t = i / (steps - 1)
        x = int(round(ax + (bx - ax) * t))
        y = int(round(ay + (by - ay) * t))
        if (x, y) not in seen:
            seen.add((x, y))
            points.append((x, y))
    return points


def canonical_edge(v0: Tuple[int, int], v1: Tuple[int, int]) -> Tuple[Tuple[int, int], Tuple[int, int]]:
    return tuple(sorted((v0, v1)))  # type: ignore[return-value]


def load_ref_triangles(conn: sqlite3.Connection) -> List[Triangle]:
    columns = [
        "'ref'",
        "draw_id", "primitive_id", "textured", "sign",
        "vertex_ax", "vertex_ay", "vertex_bx", "vertex_by", "vertex_cx", "vertex_cy",
        "fbz_color_path", "texture_mode_0", "tlod_0", "tex_base_addr_0",
        "tex_base_addr_1_0", "tex_base_addr_2_0", "tex_base_addr_38_0", "tformat_0",
        "start_s", "start_t", "start_w", "NULL as hi_start_s", "NULL as hi_start_t", "0 as x_start", "0 as y_start", "dsdx", "dtdx", "dwdx", "dsdy", "dtdy", "NULL as hi_dsdx", "NULL as hi_dtdx", "NULL as hi_dsdy", "NULL as hi_dtdy", "dwdy",
    ]
    sql = f"select {', '.join(columns)} from ref_triangles where textured = 1"
    return [Triangle(*row) for row in conn.execute(sql)]


def load_dut_triangles(conn: sqlite3.Connection) -> List[Triangle]:
    columns = [
        "'dut'",
        "draw_id", "primitive_id", "textured", "sign",
        "vertex_ax", "vertex_ay", "vertex_bx", "vertex_by", "vertex_cx", "vertex_cy",
        "0 as fbz_color_path", "texture_mode_0", "tlod_0", "tex_base_addr_0",
        "tex_base_addr_1_0", "tex_base_addr_2_0", "tex_base_addr_38_0", "tformat_0",
        "start_s", "start_t", "start_w", "hi_start_s", "hi_start_t", "x_start", "y_start", "dsdx", "dtdx", "dwdx", "dsdy", "dtdy", "hi_dsdx", "hi_dtdx", "hi_dsdy", "hi_dtdy", "dwdy",
    ]
    sql = f"select {', '.join(columns)} from dut_triangles where textured = 1"
    return [Triangle(*row) for row in conn.execute(sql)]


def analyze_pairs_ref(triangles: List[Triangle]) -> List[dict]:
    edge_map: Dict[Tuple[Tuple[int, int], Tuple[int, int]], List[Triangle]] = {}
    for tri in triangles:
        verts = tri.vertices
        for i0, i1 in ((0, 1), (1, 2), (2, 0)):
            edge_map.setdefault(canonical_edge(verts[i0], verts[i1]), []).append(tri)

    results = []
    for edge, tris in edge_map.items():
        if len(tris) < 2:
            continue
        for tri_a, tri_b in combinations(tris, 2):
            if tri_a.draw_id != tri_b.draw_id or tri_a.texture_key != tri_b.texture_key:
                continue
            samples = sample_edge_pixels(*edge)
            distances = []
            lod_mismatches = 0
            for x, y in samples:
                sa, ta, la = compute_ref_texel_address(tri_a, x, y)
                sb, tb, lb = compute_ref_texel_address(tri_b, x, y)
                if la != lb:
                    lod_mismatches += 1
                row_shift_a = dut_row_shift_bits(tri_a, la)
                row_shift_b = dut_row_shift_bits(tri_b, lb)
                addr_a = sa + (ta << row_shift_a)
                addr_b = sb + (tb << row_shift_b)
                distances.append(abs(addr_a - addr_b))
            if not distances:
                continue
            results.append(
                {
                    "source": "ref",
                    "draw_id": tri_a.draw_id,
                    "primitive_a": tri_a.primitive_id,
                    "primitive_b": tri_b.primitive_id,
                    "edge": edge,
                    "area_a": tri_a.area_pixels,
                    "area_b": tri_b.area_pixels,
                    "sort_area": max(tri_a.area_pixels, tri_b.area_pixels),
                    "edge_length": math.hypot((edge[1][0] - edge[0][0]) / 16.0, (edge[1][1] - edge[0][1]) / 16.0),
                    "sample_count": len(samples),
                    "max_addr_distance": max(distances),
                    "avg_addr_distance": sum(distances) / len(distances),
                    "lod_mismatches": lod_mismatches,
                }
            )
    return results


def analyze_pairs_dut(triangles: List[Triangle]) -> List[dict]:
    edge_map: Dict[Tuple[Tuple[int, int], Tuple[int, int]], List[Triangle]] = {}
    for tri in triangles:
        verts = tri.vertices
        for i0, i1 in ((0, 1), (1, 2), (2, 0)):
            edge_map.setdefault(canonical_edge(verts[i0], verts[i1]), []).append(tri)

    results = []
    for edge, tris in edge_map.items():
        if len(tris) < 2:
            continue
        for tri_a, tri_b in combinations(tris, 2):
            if tri_a.draw_id != tri_b.draw_id or tri_a.texture_key != tri_b.texture_key:
                continue
            distances = []
            lod_mismatches = 0
            for x, y in sample_edge_pixels(*edge):
                tex_x_a, tex_y_a, lod_a = compute_dut_texel_address(tri_a, x, y)
                tex_x_b, tex_y_b, lod_b = compute_dut_texel_address(tri_b, x, y)
                if lod_a != lod_b:
                    lod_mismatches += 1
                row_shift_a = dut_row_shift_bits(tri_a, lod_a)
                row_shift_b = dut_row_shift_bits(tri_b, lod_b)
                addr_a = tex_x_a + (tex_y_a << row_shift_a)
                addr_b = tex_x_b + (tex_y_b << row_shift_b)
                distances.append(abs(addr_a - addr_b))
            if not distances:
                continue
            results.append(
                {
                    "source": "dut",
                    "draw_id": tri_a.draw_id,
                    "primitive_a": tri_a.primitive_id,
                    "primitive_b": tri_b.primitive_id,
                    "edge": edge,
                    "area_a": tri_a.area_pixels,
                    "area_b": tri_b.area_pixels,
                    "sort_area": max(tri_a.area_pixels, tri_b.area_pixels),
                    "edge_length": math.hypot((edge[1][0] - edge[0][0]) / 16.0, (edge[1][1] - edge[0][1]) / 16.0),
                    "sample_count": len(distances),
                    "max_addr_distance": max(distances),
                    "avg_addr_distance": sum(distances) / len(distances),
                    "lod_mismatches": lod_mismatches,
                }
            )
    return results


def print_results(title: str, rows: List[dict], limit: int) -> None:
    print(title)
    print("draw  prim_a  prim_b  area_a    area_b    edge_len  samples  max_addr  avg_addr  lod_mis  edge")
    for row in rows[:limit]:
        edge = row["edge"]
        print(
            f"{row['draw_id']:4d}  {row['primitive_a']:6d}  {row['primitive_b']:6d}  "
            f"{row['area_a']:8.1f}  {row['area_b']:8.1f}  {row['edge_length']:8.1f}  "
            f"{row['sample_count']:7d}  {row['max_addr_distance']:8.3f}  {row['avg_addr_distance']:8.3f}  "
            f"{row['lod_mismatches']:7d}  "
            f"(({edge[0][0]/16.0:.2f},{edge[0][1]/16.0:.2f}) -> ({edge[1][0]/16.0:.2f},{edge[1][1]/16.0:.2f}))"
        )
    print(f"Total matching shared-edge pairs: {len(rows)}")


def main() -> int:
    args = parse_args()
    conn = sqlite3.connect(args.db)
    try:
        ref_triangles = load_ref_triangles(conn) if args.source in {"ref", "both"} else []
        dut_triangles = load_dut_triangles(conn) if args.source in {"dut", "both"} else []
    finally:
        conn.close()

    if args.source in {"ref", "both"}:
        ref_results = analyze_pairs_ref(ref_triangles)
        ref_results.sort(key=lambda row: (-row["sort_area"], -row["edge_length"], -row["max_addr_distance"]))
        print_results("REF", ref_results, args.limit)
    if args.source in {"dut", "both"}:
        dut_results = analyze_pairs_dut(dut_triangles)
        dut_results.sort(key=lambda row: (-row["sort_area"], -row["edge_length"], -row["max_addr_distance"]))
        print_results("DUT", dut_results, args.limit)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
