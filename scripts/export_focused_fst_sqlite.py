#!/usr/bin/env python3

import argparse
import sqlite3
import subprocess
from pathlib import Path


SIGNALS = {
    "triangle_valid": "streamArbiter_5_io_output_valid",
    "triangle_origin": "streamArbiter_5_io_output_payload_trace_origin",
    "triangle_draw": "streamArbiter_5_io_output_payload_trace_drawId",
    "triangle_prim": "streamArbiter_5_io_output_payload_trace_primitiveId",
    "tmu_in_valid": "tmu_1_io_input_valid",
    "tmu_in_origin": "tmu_1_io_input_payload_trace_primitive_origin",
    "tmu_in_draw": "tmu_1_io_input_payload_trace_primitive_drawId",
    "tmu_in_prim": "tmu_1_io_input_payload_trace_primitive_primitiveId",
    "tmu_out_valid": "tmu_1_io_output_valid",
    "tmu_out_origin": "tmu_1_io_output_payload_trace_primitive_origin",
    "tmu_out_draw": "tmu_1_io_output_payload_trace_primitive_drawId",
    "tmu_out_prim": "tmu_1_io_output_payload_trace_primitive_primitiveId",
    "fb_out_valid": "fbAccess_io_output_valid",
    "fb_out_origin": "fbAccess_io_output_payload_trace_primitive_origin",
    "fb_out_draw": "fbAccess_io_output_payload_trace_primitive_drawId",
    "fb_out_prim": "fbAccess_io_output_payload_trace_primitive_primitiveId",
}


def parse_args():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fst", required=True)
    ap.add_argument("--db", required=True)
    ap.add_argument("--fst2vcd", default="fst2vcd")
    return ap.parse_args()


def wanted_suffix(name: str):
    for key, suffix in SIGNALS.items():
        if name.endswith(suffix):
            return key
    return None


def main():
    args = parse_args()
    db = sqlite3.connect(args.db)
    cur = db.cursor()
    cur.execute("drop table if exists signal_changes")
    cur.execute("create table signal_changes(signal text, time integer, value text)")

    proc = subprocess.Popen(
        [args.fst2vcd, args.fst], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1
    )
    id_to_signal = {}
    in_defs = True
    current_time = 0
    batch = []

    for raw_line in proc.stdout:
        line = raw_line.strip()
        if not line:
            continue
        if in_defs:
            if line.startswith("$var "):
                parts = line.split()
                if len(parts) >= 5:
                    code = parts[3]
                    name = parts[4]
                    sig = wanted_suffix(name)
                    if sig is not None:
                        id_to_signal[code] = sig
            elif line.startswith("$enddefinitions"):
                in_defs = False
            continue

        if line[0] == "#":
            current_time = int(line[1:])
            continue

        if line[0] in "01xXzZ":
            code = line[1:]
            sig = id_to_signal.get(code)
            if sig is not None:
                batch.append((sig, current_time, line[0]))
        elif line[0] == "b":
            bits, code = line[1:].split()
            sig = id_to_signal.get(code)
            if sig is not None:
                batch.append((sig, current_time, bits))

        if len(batch) >= 10000:
            cur.executemany("insert into signal_changes values (?, ?, ?)", batch)
            batch.clear()

    if batch:
        cur.executemany("insert into signal_changes values (?, ?, ?)", batch)
    db.commit()
    db.close()
    stdout, stderr = proc.communicate()
    if proc.returncode != 0:
        raise SystemExit(stderr or stdout or proc.returncode)


if __name__ == "__main__":
    main()
