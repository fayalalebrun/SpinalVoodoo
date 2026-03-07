#!/usr/bin/env python3

import argparse
import json
import sqlite3
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional


@dataclass(frozen=True)
class Signal:
    column: str
    path: str


@dataclass(frozen=True)
class Stage:
    name: str
    signals: List[Signal]


TRACE_SIGNALS = [
    Signal("origin", "trace_primitive_origin"),
    Signal("draw_id", "trace_primitive_drawId"),
    Signal("primitive_id", "trace_primitive_primitiveId"),
    Signal("pixel_seq", "trace_pixelSeq"),
]


STAGES = {
    "rasterizer_out": Stage(
        "rasterizer_out",
        [
            Signal("valid", "core_1.rasterizer_1.o_valid"),
            Signal("ready", "core_1.rasterizer_1.o_ready"),
            Signal("origin", "core_1.rasterizer_1.o_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.rasterizer_1.o_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.rasterizer_1.o_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.rasterizer_1.o_payload_trace_pixelSeq"),
            Signal("x", "core_1.rasterizer_1.o_payload_coords_0"),
            Signal("y", "core_1.rasterizer_1.o_payload_coords_1"),
            Signal("s", "core_1.rasterizer_1.o_payload_grads_sGrad"),
            Signal("t", "core_1.rasterizer_1.o_payload_grads_tGrad"),
            Signal("w", "core_1.rasterizer_1.o_payload_grads_wGrad"),
        ],
    ),
    "tmu_input": Stage(
        "tmu_input",
        [
            Signal("valid", "core_1.tmu_1.io_input_valid"),
            Signal("ready", "core_1.tmu_1.io_input_ready"),
            Signal("origin", "core_1.tmu_1.io_input_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.tmu_1.io_input_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.tmu_1.io_input_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.tmu_1.io_input_payload_trace_pixelSeq"),
            Signal("s", "core_1.tmu_1.io_input_payload_s"),
            Signal("t", "core_1.tmu_1.io_input_payload_t"),
            Signal("w", "core_1.tmu_1.io_input_payload_w"),
            Signal("dsdx", "core_1.tmu_1.io_input_payload_dSdX"),
            Signal("dsdy", "core_1.tmu_1.io_input_payload_dSdY"),
            Signal("dtdx", "core_1.tmu_1.io_input_payload_dTdX"),
            Signal("dtdy", "core_1.tmu_1.io_input_payload_dTdY"),
            Signal("bilinear_enable", "core_1.tmu_1.bilinearEnable"),
            Signal("frac_s", "core_1.tmu_1.finalDs"),
            Signal("frac_t", "core_1.tmu_1.finalDt"),
            Signal("lod_level", "core_1.tmu_1.lodLevel"),
            Signal("tex_s_x4", "core_1.tmu_1.texS"),
            Signal("tex_t_x4", "core_1.tmu_1.texT"),
            Signal("adj_s_x4", "core_1.tmu_1.adjS"),
            Signal("adj_t_x4", "core_1.tmu_1.adjT"),
            Signal("point_texel_x", "core_1.tmu_1.pointTexelX"),
            Signal("point_texel_y", "core_1.tmu_1.pointTexelY"),
            Signal("bi_x0", "core_1.tmu_1.biX0"),
            Signal("bi_x1", "core_1.tmu_1.biX1"),
            Signal("bi_y0", "core_1.tmu_1.biY0"),
            Signal("bi_y1", "core_1.tmu_1.biY1"),
        ],
    ),
    "tmu_output": Stage(
        "tmu_output",
        [
            Signal("valid", "core_1.tmu_1.io_output_valid"),
            Signal("ready", "core_1.tmu_1.io_output_ready"),
            Signal("origin", "core_1.tmu_1.io_output_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.tmu_1.io_output_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.tmu_1.io_output_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.tmu_1.io_output_payload_trace_pixelSeq"),
            Signal("texture_r", "core_1.tmu_1.io_output_payload_texture_r"),
            Signal("texture_g", "core_1.tmu_1.io_output_payload_texture_g"),
            Signal("texture_b", "core_1.tmu_1.io_output_payload_texture_b"),
            Signal("texture_a", "core_1.tmu_1.io_output_payload_textureAlpha"),
        ],
    ),
    "color_output": Stage(
        "color_output",
        [
            Signal("valid", "core_1.colorCombine_1.io_output_valid"),
            Signal("ready", "core_1.colorCombine_1.io_output_ready"),
            Signal("origin", "core_1.colorCombine_1.io_output_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.colorCombine_1.io_output_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.colorCombine_1.io_output_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.colorCombine_1.io_output_payload_trace_pixelSeq"),
            Signal("x", "core_1.colorCombine_1.io_output_payload_coords_0"),
            Signal("y", "core_1.colorCombine_1.io_output_payload_coords_1"),
            Signal("color_r", "core_1.colorCombine_1.io_output_payload_color_r"),
            Signal("color_g", "core_1.colorCombine_1.io_output_payload_color_g"),
            Signal("color_b", "core_1.colorCombine_1.io_output_payload_color_b"),
            Signal("alpha", "core_1.colorCombine_1.io_output_payload_alpha"),
            Signal("depth", "core_1.colorCombine_1.io_output_payload_depth"),
        ],
    ),
    "fog_output": Stage(
        "fog_output",
        [
            Signal("valid", "core_1.fog_1.io_output_valid"),
            Signal("ready", "core_1.fog_1.io_output_ready"),
            Signal("origin", "core_1.fog_1.io_output_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.fog_1.io_output_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.fog_1.io_output_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.fog_1.io_output_payload_trace_pixelSeq"),
            Signal("x", "core_1.fog_1.io_output_payload_coords_0"),
            Signal("y", "core_1.fog_1.io_output_payload_coords_1"),
            Signal("color_before_fog_r", "core_1.fog_1.io_output_payload_colorBeforeFog_r"),
            Signal("color_before_fog_g", "core_1.fog_1.io_output_payload_colorBeforeFog_g"),
            Signal("color_before_fog_b", "core_1.fog_1.io_output_payload_colorBeforeFog_b"),
            Signal("color_r", "core_1.fog_1.io_output_payload_color_r"),
            Signal("color_g", "core_1.fog_1.io_output_payload_color_g"),
            Signal("color_b", "core_1.fog_1.io_output_payload_color_b"),
            Signal("alpha", "core_1.fog_1.io_output_payload_alpha"),
            Signal("w_depth", "core_1.fog_1.io_output_payload_wDepth"),
        ],
    ),
    "fb_output": Stage(
        "fb_output",
        [
            Signal("valid", "core_1.fbAccess.io_output_valid"),
            Signal("ready", "core_1.fbAccess.io_output_ready"),
            Signal("origin", "core_1.fbAccess.io_output_payload_trace_primitive_origin"),
            Signal("draw_id", "core_1.fbAccess.io_output_payload_trace_primitive_drawId"),
            Signal("primitive_id", "core_1.fbAccess.io_output_payload_trace_primitive_primitiveId"),
            Signal("pixel_seq", "core_1.fbAccess.io_output_payload_trace_pixelSeq"),
            Signal("x", "core_1.fbAccess.io_output_payload_coords_0"),
            Signal("y", "core_1.fbAccess.io_output_payload_coords_1"),
            Signal("color_r", "core_1.fbAccess.io_output_payload_color_r"),
            Signal("color_g", "core_1.fbAccess.io_output_payload_color_g"),
            Signal("color_b", "core_1.fbAccess.io_output_payload_color_b"),
            Signal("alpha", "core_1.fbAccess.io_output_payload_alpha"),
            Signal("new_depth", "core_1.fbAccess.io_output_payload_newDepth"),
            Signal("rgb_write", "core_1.fbAccess.io_output_payload_rgbWrite"),
            Signal("aux_write", "core_1.fbAccess.io_output_payload_auxWrite"),
            Signal("enable_dithering", "core_1.fbAccess.io_output_payload_enableDithering"),
            Signal("dither_algorithm", "core_1.fbAccess.io_output_payload_ditherAlgorithm"),
        ],
    ),
}

TRIANGLE_STREAM = Stage(
    "dut_triangle_stream",
    [
        Signal("valid", "core_1.streamArbiter_5_io_output_valid"),
        Signal("ready", "core_1.triangleSetup_1_i_ready"),
        Signal("origin", "core_1.streamArbiter_5_io_output_payload_trace_origin"),
        Signal("draw_id", "core_1.streamArbiter_5_io_output_payload_trace_drawId"),
        Signal("primitive_id", "core_1.streamArbiter_5_io_output_payload_trace_primitiveId"),
        Signal("trace_valid", "core_1.streamArbiter_5_io_output_payload_trace_valid"),
        Signal("sign", "core_1.streamArbiter_5_io_output_payload_triWithSign_signBit"),
        Signal("vertex_ax", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_0_0"),
        Signal("vertex_ay", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_0_1"),
        Signal("vertex_bx", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_1_0"),
        Signal("vertex_by", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_1_1"),
        Signal("vertex_cx", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_2_0"),
        Signal("vertex_cy", "core_1.streamArbiter_5_io_output_payload_triWithSign_tri_2_1"),
        Signal("fbz_color_path_texture_enable", "core_1.triangleSetup_1_o_payload_config_fbzColorPath_textureEnable"),
        Signal("fbz_color_path_param_adjust", "core_1.triangleSetup_1_o_payload_config_fbzColorPath_paramAdjust"),
        Signal("texture_mode_0", "core_1.triangleSetup_1_o_payload_config_tmuTextureMode"),
        Signal("tlod_0", "core_1.triangleSetup_1_o_payload_config_tmuTLOD"),
        Signal("tex_base_addr_0", "core_1.triangleSetup_1_o_payload_config_tmuTexBaseAddr"),
        Signal("start_s", "core_1.triangleSetup_1_o_payload_grads_sGrad_start"),
        Signal("start_t", "core_1.triangleSetup_1_o_payload_grads_tGrad_start"),
        Signal("start_w", "core_1.triangleSetup_1_o_payload_grads_wGrad_start"),
        Signal("hi_start_s", "core_1.triangleSetup_1_o_payload_texHi_sStart"),
        Signal("hi_start_t", "core_1.triangleSetup_1_o_payload_texHi_tStart"),
        Signal("x_start", "core_1.triangleSetup_1_o_payload_xrange_0"),
        Signal("y_start", "core_1.triangleSetup_1_o_payload_yrange_0"),
        Signal("dsdx", "core_1.triangleSetup_1_o_payload_config_tmudSdX"),
        Signal("dtdx", "core_1.triangleSetup_1_o_payload_config_tmudTdX"),
        Signal("dsdy", "core_1.triangleSetup_1_o_payload_config_tmudSdY"),
        Signal("dtdy", "core_1.triangleSetup_1_o_payload_config_tmudTdY"),
        Signal("hi_dsdx", "core_1.triangleSetup_1_o_payload_texHi_dSdX"),
        Signal("hi_dtdx", "core_1.triangleSetup_1_o_payload_texHi_dTdX"),
        Signal("hi_dsdy", "core_1.triangleSetup_1_o_payload_texHi_dSdY"),
        Signal("hi_dtdy", "core_1.triangleSetup_1_o_payload_texHi_dTdY"),
        Signal("dwdx", "core_1.triangleSetup_1_o_payload_grads_wGrad_d_0"),
        Signal("dwdy", "core_1.triangleSetup_1_o_payload_grads_wGrad_d_1"),
    ],
)

SIGNED_COLUMNS = {
    "vertex_ax": 16,
    "vertex_ay": 16,
    "vertex_bx": 16,
    "vertex_by": 16,
    "vertex_cx": 16,
    "vertex_cy": 16,
    "start_s": 48,
    "start_t": 48,
    "start_w": 48,
    "hi_start_s": 60,
    "hi_start_t": 60,
    "x_start": 16,
    "y_start": 16,
    "dsdx": 32,
    "dtdx": 32,
    "dsdy": 32,
    "dtdy": 32,
    "hi_dsdx": 60,
    "hi_dtdx": 60,
    "hi_dsdy": 60,
    "hi_dtdy": 60,
    "dwdx": 48,
    "dwdy": 48,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export DUT trace-tagged stage transfers into SQLite.")
    parser.add_argument("--netlist", help="Path to the Verilog/netlist file for conetrace.")
    parser.add_argument("--trace", help="Path to the waveform trace (.fst/.vcd).")
    parser.add_argument("--db", required=True, help="Output SQLite database path.")
    parser.add_argument(
        "--stages",
        default=",".join(STAGES.keys()),
        help="Comma-separated stage names to export.",
    )
    parser.add_argument(
        "--time-range",
        help="Optional conetrace time range, for example 0..100000 or 8707000..8708000.",
    )
    parser.add_argument(
        "--conetrace",
        default="conetrace",
        help="Conetrace executable name or path.",
    )
    parser.add_argument(
        "--skip-dut",
        action="store_true",
        help="Skip DUT waveform export and only import reference JSONL data.",
    )
    parser.add_argument(
        "--ref-jsonl",
        help="Optional reference trace JSONL file from trace_test --ref-trace-jsonl.",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress per-stage progress messages.",
    )
    return parser.parse_args()


def parse_value(text: Optional[str]) -> Optional[int]:
    if text is None:
        return None
    text = text.strip()
    if text in {"x", "z", "?", ""}:
        return None
    if text.startswith("0x") or text.startswith("-0x"):
        return int(text, 16)
    if text.startswith("0b") or text.startswith("-0b"):
        return int(text, 2)
    return int(text, 10)


def sign_extend(value: Optional[int], width: int) -> Optional[int]:
    if value is None:
        return None
    sign_bit = 1 << (width - 1)
    mask = (1 << width) - 1
    value &= mask
    return value - (1 << width) if value & sign_bit else value


def run_conetrace_changes(
    conetrace: str,
    netlist: str,
    trace: str,
    signal_path: str,
    time_range: Optional[str],
) -> List[dict]:
    query = f"prefix detect; changes {signal_path}"
    if time_range:
        query += f" | where time in {time_range}"
    query += " | json"
    result = subprocess.run(
        [conetrace, netlist, trace, "-e", query],
        check=True,
        capture_output=True,
        text=True,
    )
    stdout = result.stdout
    start = stdout.find("[")
    end = stdout.rfind("]")
    if start == -1 or end == -1 or end < start:
        raise RuntimeError(f"Failed to parse JSON from conetrace for {signal_path}\n{stdout}\n{result.stderr}")
    rows = json.loads(stdout[start : end + 1])
    suffix = f".{signal_path}"
    return [row for row in rows if row.get("signal", "").endswith(suffix)]


def collect_stage_changes(
    conetrace: str,
    netlist: str,
    trace: str,
    stage: Stage,
    time_range: Optional[str],
    quiet: bool,
) -> tuple[Dict[int, List[tuple]], Dict[str, Optional[int]]]:
    by_time: Dict[int, List[tuple]] = defaultdict(list)
    initial_state: Dict[str, Optional[int]] = {}
    for signal in stage.signals:
        if not quiet:
            print(f"[{stage.name}] reading {signal.column}", file=sys.stderr)
        rows = run_conetrace_changes(conetrace, netlist, trace, signal.path, time_range)
        if rows:
            initial_state[signal.column] = parse_value(rows[0].get("from"))
        for row in rows:
            by_time[int(row["time"])].append((signal.column, parse_value(row["to"])))
    return by_time, initial_state


def reconstruct_stage_rows(
    stage: Stage,
    by_time: Dict[int, List[tuple]],
    initial_state: Dict[str, Optional[int]],
) -> List[dict]:
    defaults = {
        "valid": 0,
        "ready": 0,
        "origin": 0,
        "draw_id": 0,
        "primitive_id": 0,
        "pixel_seq": 0,
    }
    state = {
        signal.column: initial_state.get(
            signal.column,
            defaults.get(signal.column),
        )
        for signal in stage.signals
    }
    rows = []
    seen_keys = set()
    for time in sorted(by_time):
        changed = set()
        for column, value in by_time[time]:
            state[column] = value
            changed.add(column)
        if not state.get("valid") or not state.get("ready"):
            continue
        transfer_triggered = (
            "valid" in changed or "ready" in changed or any(column not in {"valid", "ready"} for column in changed)
        )
        if not transfer_triggered:
            continue
        trace_key = (
            state.get("origin"),
            state.get("draw_id"),
            state.get("primitive_id"),
            state.get("pixel_seq"),
        )
        event_identity = (time, trace_key)
        if event_identity in seen_keys:
            continue
        seen_keys.add(event_identity)
        row = {
            "stage": stage.name,
            "time": time,
            "cycle": time // 2,
        }
        row.update(state)
        rows.append(row)
    return rows


def ensure_schema(conn: sqlite3.Connection) -> None:
    conn.executescript(
        """
        PRAGMA journal_mode = WAL;
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS dut_stage_events (
            stage TEXT NOT NULL,
            time INTEGER NOT NULL,
            cycle INTEGER NOT NULL,
            origin INTEGER,
            draw_id INTEGER,
            primitive_id INTEGER,
            pixel_seq INTEGER,
            x INTEGER,
            y INTEGER,
            s INTEGER,
            t INTEGER,
            w INTEGER,
            dsdx INTEGER,
            dsdy INTEGER,
            dtdx INTEGER,
            dtdy INTEGER,
            texture_r INTEGER,
            texture_g INTEGER,
            texture_b INTEGER,
            texture_a INTEGER,
            bilinear_enable INTEGER,
            frac_s INTEGER,
            frac_t INTEGER,
            lod_level INTEGER,
            tex_s_x4 INTEGER,
            tex_t_x4 INTEGER,
            adj_s_x4 INTEGER,
            adj_t_x4 INTEGER,
            point_texel_x INTEGER,
            point_texel_y INTEGER,
            bi_x0 INTEGER,
            bi_x1 INTEGER,
            bi_y0 INTEGER,
            bi_y1 INTEGER,
            color_before_fog_r INTEGER,
            color_before_fog_g INTEGER,
            color_before_fog_b INTEGER,
            color_r INTEGER,
            color_g INTEGER,
            color_b INTEGER,
            alpha INTEGER,
            depth INTEGER,
            w_depth INTEGER,
            new_depth INTEGER,
            rgb_write INTEGER,
            aux_write INTEGER,
            enable_dithering INTEGER,
            dither_algorithm INTEGER,
            PRIMARY KEY (stage, time)
        );

        CREATE INDEX IF NOT EXISTS dut_stage_events_trace_key
            ON dut_stage_events(stage, draw_id, primitive_id, pixel_seq, time);

        CREATE INDEX IF NOT EXISTS dut_stage_events_pixel_lookup
            ON dut_stage_events(draw_id, primitive_id, pixel_seq, stage);

        CREATE TABLE IF NOT EXISTS dut_triangles (
            draw_id INTEGER NOT NULL,
            primitive_id INTEGER NOT NULL,
            origin INTEGER,
            textured INTEGER,
            sign INTEGER,
            vertex_ax INTEGER,
            vertex_ay INTEGER,
            vertex_bx INTEGER,
            vertex_by INTEGER,
            vertex_cx INTEGER,
            vertex_cy INTEGER,
            texture_mode_0 INTEGER,
            tlod_0 INTEGER,
            tex_base_addr_0 INTEGER,
            tex_base_addr_1_0 INTEGER,
            tex_base_addr_2_0 INTEGER,
            tex_base_addr_38_0 INTEGER,
            tformat_0 INTEGER,
            start_s INTEGER,
            start_t INTEGER,
            start_w INTEGER,
            hi_start_s INTEGER,
            hi_start_t INTEGER,
            x_start INTEGER,
            y_start INTEGER,
            dsdx INTEGER,
            dtdx INTEGER,
            dwdx INTEGER,
            dsdy INTEGER,
            dtdy INTEGER,
            hi_dsdx INTEGER,
            hi_dtdx INTEGER,
            hi_dsdy INTEGER,
            hi_dtdy INTEGER,
            dwdy INTEGER,
            PRIMARY KEY (draw_id, primitive_id)
        );

        CREATE INDEX IF NOT EXISTS dut_triangles_texture_lookup
            ON dut_triangles(tex_base_addr_0, texture_mode_0, tlod_0);

        CREATE TABLE IF NOT EXISTS ref_stage_events (
            event_index INTEGER PRIMARY KEY,
            stage TEXT NOT NULL,
            tmu INTEGER,
            origin INTEGER,
            draw_id INTEGER,
            primitive_id INTEGER,
            pixel_seq INTEGER,
            x INTEGER,
            y INTEGER,
            s INTEGER,
            t INTEGER,
            w INTEGER,
            tex_s INTEGER,
            tex_t INTEGER,
            lod INTEGER,
            texture_r INTEGER,
            texture_g INTEGER,
            texture_b INTEGER,
            texture_a INTEGER
        );

        CREATE INDEX IF NOT EXISTS ref_stage_events_pixel_lookup
            ON ref_stage_events(draw_id, primitive_id, pixel_seq, stage);

        CREATE TABLE IF NOT EXISTS ref_triangles (
            draw_id INTEGER NOT NULL,
            primitive_id INTEGER NOT NULL,
            origin INTEGER,
            textured INTEGER,
            sign INTEGER,
            vertex_ax INTEGER,
            vertex_ay INTEGER,
            vertex_bx INTEGER,
            vertex_by INTEGER,
            vertex_cx INTEGER,
            vertex_cy INTEGER,
            fbz_color_path INTEGER,
            texture_mode_0 INTEGER,
            tlod_0 INTEGER,
            tex_base_addr_0 INTEGER,
            tex_base_addr_1_0 INTEGER,
            tex_base_addr_2_0 INTEGER,
            tex_base_addr_38_0 INTEGER,
            tformat_0 INTEGER,
            start_s INTEGER,
            start_t INTEGER,
            start_w INTEGER,
            dsdx INTEGER,
            dtdx INTEGER,
            dwdx INTEGER,
            dsdy INTEGER,
            dtdy INTEGER,
            dwdy INTEGER,
            PRIMARY KEY (draw_id, primitive_id)
        );

        CREATE INDEX IF NOT EXISTS ref_triangles_texture_lookup
            ON ref_triangles(tex_base_addr_0, texture_mode_0, tlod_0);
        """
    )


def write_metadata(conn: sqlite3.Connection, args: argparse.Namespace, stages: Iterable[str]) -> None:
    pairs = {
        "netlist": str(Path(args.netlist).resolve()) if args.netlist else "",
        "trace": str(Path(args.trace).resolve()) if args.trace else "",
        "time_range": args.time_range or "",
        "stages": ",".join(stages),
        "ref_jsonl": str(Path(args.ref_jsonl).resolve()) if args.ref_jsonl else "",
    }
    conn.executemany(
        "INSERT INTO metadata(key, value) VALUES(?, ?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
        pairs.items(),
    )


def insert_stage_rows(conn: sqlite3.Connection, rows: List[dict]) -> None:
    columns = [
        "stage",
        "time",
        "cycle",
        "origin",
        "draw_id",
        "primitive_id",
        "pixel_seq",
        "x",
        "y",
        "s",
        "t",
        "w",
        "dsdx",
        "dsdy",
        "dtdx",
        "dtdy",
        "texture_r",
        "texture_g",
        "texture_b",
        "texture_a",
        "bilinear_enable",
        "frac_s",
        "frac_t",
        "lod_level",
        "tex_s_x4",
        "tex_t_x4",
        "adj_s_x4",
        "adj_t_x4",
        "point_texel_x",
        "point_texel_y",
        "bi_x0",
        "bi_x1",
        "bi_y0",
        "bi_y1",
        "color_before_fog_r",
        "color_before_fog_g",
        "color_before_fog_b",
        "color_r",
        "color_g",
        "color_b",
        "alpha",
        "depth",
        "w_depth",
        "new_depth",
        "rgb_write",
        "aux_write",
        "enable_dithering",
        "dither_algorithm",
    ]
    placeholders = ", ".join("?" for _ in columns)
    sql = f"INSERT OR REPLACE INTO dut_stage_events ({', '.join(columns)}) VALUES ({placeholders})"
    conn.executemany(sql, ([row.get(column) for column in columns] for row in rows))


def reconstruct_triangle_rows(
    by_time: Dict[int, List[tuple]],
    initial_state: Dict[str, Optional[int]],
) -> List[dict]:
    defaults = {
        "valid": 0,
        "ready": 0,
        "trace_valid": 1,
        "origin": 0,
        "draw_id": 0,
        "primitive_id": 0,
        "sign": 0,
        "fbz_color_path_texture_enable": 0,
    }
    state = {
        signal.column: initial_state.get(signal.column, defaults.get(signal.column))
        for signal in TRIANGLE_STREAM.signals
    }
    rows = []
    seen_keys = set()
    for time in sorted(by_time):
        changed = set()
        for column, value in by_time[time]:
            state[column] = value
            changed.add(column)
        if not state.get("valid") or not state.get("ready"):
            continue
        transfer_triggered = (
            "valid" in changed or "ready" in changed or any(column not in {"valid", "ready"} for column in changed)
        )
        if not transfer_triggered:
            continue
        key = (state.get("draw_id"), state.get("primitive_id"))
        event_identity = (time, key)
        if event_identity in seen_keys:
            continue
        seen_keys.add(event_identity)
        row = {
            "draw_id": state.get("draw_id"),
            "primitive_id": state.get("primitive_id"),
            "origin": state.get("origin"),
            "textured": state.get("fbz_color_path_texture_enable"),
            "sign": state.get("sign"),
            "tex_base_addr_1_0": None,
            "tex_base_addr_2_0": None,
            "tex_base_addr_38_0": None,
            "tformat_0": ((state.get("texture_mode_0") or 0) >> 8) & 0xF,
        }
        for column in (
            "vertex_ax",
            "vertex_ay",
            "vertex_bx",
            "vertex_by",
            "vertex_cx",
            "vertex_cy",
            "start_s",
            "start_t",
            "start_w",
            "hi_start_s",
            "hi_start_t",
            "x_start",
            "y_start",
            "dsdx",
            "dtdx",
            "dwdx",
            "dsdy",
            "dtdy",
            "hi_dsdx",
            "hi_dtdx",
            "hi_dsdy",
            "hi_dtdy",
            "dwdy",
        ):
            row[column] = sign_extend(state.get(column), SIGNED_COLUMNS[column])
        for column in (
            "texture_mode_0",
            "tlod_0",
            "tex_base_addr_0",
        ):
            row[column] = state.get(column)
        rows.append(row)
    return rows


def insert_dut_triangles(conn: sqlite3.Connection, rows: List[dict]) -> None:
    columns = [
        "draw_id",
        "primitive_id",
        "origin",
        "textured",
        "sign",
        "vertex_ax",
        "vertex_ay",
        "vertex_bx",
        "vertex_by",
        "vertex_cx",
        "vertex_cy",
        "texture_mode_0",
        "tlod_0",
        "tex_base_addr_0",
        "tex_base_addr_1_0",
        "tex_base_addr_2_0",
        "tex_base_addr_38_0",
        "tformat_0",
        "start_s",
        "start_t",
        "start_w",
        "hi_start_s",
        "hi_start_t",
        "x_start",
        "y_start",
        "dsdx",
        "dtdx",
        "dwdx",
        "dsdy",
        "dtdy",
        "hi_dsdx",
        "hi_dtdx",
        "hi_dsdy",
        "hi_dtdy",
        "dwdy",
    ]
    placeholders = ", ".join("?" for _ in columns)
    sql = f"INSERT OR REPLACE INTO dut_triangles ({', '.join(columns)}) VALUES ({placeholders})"
    conn.executemany(sql, ([row.get(column) for column in columns] for row in rows))


def import_reference_jsonl(conn: sqlite3.Connection, jsonl_path: str, quiet: bool) -> int:
    event_columns = [
        "event_index",
        "stage",
        "tmu",
        "origin",
        "draw_id",
        "primitive_id",
        "pixel_seq",
        "x",
        "y",
        "s",
        "t",
        "w",
        "tex_s",
        "tex_t",
        "lod",
        "texture_r",
        "texture_g",
        "texture_b",
        "texture_a",
    ]
    event_placeholders = ", ".join("?" for _ in event_columns)
    event_sql = f"INSERT OR REPLACE INTO ref_stage_events ({', '.join(event_columns)}) VALUES ({event_placeholders})"

    triangle_columns = [
        "draw_id",
        "primitive_id",
        "origin",
        "textured",
        "sign",
        "vertex_ax",
        "vertex_ay",
        "vertex_bx",
        "vertex_by",
        "vertex_cx",
        "vertex_cy",
        "fbz_color_path",
        "texture_mode_0",
        "tlod_0",
        "tex_base_addr_0",
        "tex_base_addr_1_0",
        "tex_base_addr_2_0",
        "tex_base_addr_38_0",
        "tformat_0",
        "start_s",
        "start_t",
        "start_w",
        "dsdx",
        "dtdx",
        "dwdx",
        "dsdy",
        "dtdy",
        "dwdy",
    ]
    triangle_placeholders = ", ".join("?" for _ in triangle_columns)
    triangle_sql = f"INSERT OR REPLACE INTO ref_triangles ({', '.join(triangle_columns)}) VALUES ({triangle_placeholders})"
    count = 0
    event_batch = []
    triangle_batch = []
    with open(jsonl_path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            if row.get("stage") == "triangle":
                triangle_batch.append([row.get(column) for column in triangle_columns])
            else:
                event_batch.append([row.get(column) for column in event_columns])
            count += 1
            if len(event_batch) >= 5000:
                conn.executemany(event_sql, event_batch)
                event_batch.clear()
            if len(triangle_batch) >= 1000:
                conn.executemany(triangle_sql, triangle_batch)
                triangle_batch.clear()
        if event_batch:
            conn.executemany(event_sql, event_batch)
        if triangle_batch:
            conn.executemany(triangle_sql, triangle_batch)
    if not quiet:
        print(f"[ref] imported {count} rows from {jsonl_path}", file=sys.stderr)
    return count


def main() -> int:
    args = parse_args()
    stage_names = [name.strip() for name in args.stages.split(",") if name.strip()]
    unknown = [name for name in stage_names if name not in STAGES]
    if unknown:
        print(f"Unknown stages: {', '.join(unknown)}", file=sys.stderr)
        print(f"Known stages: {', '.join(sorted(STAGES))}", file=sys.stderr)
        return 2
    if not args.skip_dut and (not args.netlist or not args.trace):
        print("--netlist and --trace are required unless --skip-dut is used", file=sys.stderr)
        return 2
    if args.skip_dut and not args.ref_jsonl:
        print("Nothing to do: --skip-dut requires --ref-jsonl", file=sys.stderr)
        return 2

    db_path = Path(args.db)
    db_path.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(db_path)
    try:
        ensure_schema(conn)
        write_metadata(conn, args, stage_names)
        total_rows = 0
        if not args.skip_dut:
            triangle_changes, triangle_initial_state = collect_stage_changes(
                args.conetrace,
                args.netlist,
                args.trace,
                TRIANGLE_STREAM,
                args.time_range,
                args.quiet,
            )
            triangle_rows = reconstruct_triangle_rows(triangle_changes, triangle_initial_state)
            insert_dut_triangles(conn, triangle_rows)
            conn.commit()
            if not args.quiet:
                print(f"[dut_triangles] wrote {len(triangle_rows)} rows", file=sys.stderr)
            for stage_name in stage_names:
                stage = STAGES[stage_name]
                by_time, initial_state = collect_stage_changes(
                    args.conetrace,
                    args.netlist,
                    args.trace,
                    stage,
                    args.time_range,
                    args.quiet,
                )
                rows = reconstruct_stage_rows(stage, by_time, initial_state)
                insert_stage_rows(conn, rows)
                conn.commit()
                total_rows += len(rows)
                if not args.quiet:
                    print(f"[{stage.name}] wrote {len(rows)} rows", file=sys.stderr)
            if not args.quiet:
                print(f"Wrote {total_rows} DUT stage rows to {db_path}", file=sys.stderr)
        if args.ref_jsonl:
            import_reference_jsonl(conn, args.ref_jsonl, args.quiet)
            conn.commit()
    finally:
        conn.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
