# XLS and SpinalHDL Architecture

SpinalVoodoo uses Google XLS for pure combinational production datapaths and SpinalHDL for control and integration. A separate pure-XLS hosted CoreSim/DE10Sim proc network provides behavioral replay and schedulable RTL without replacing the production SpinalHDL integration. An experimental DE10 revision uses that proc network as the accelerator behind thin SystemVerilog Avalon/Qsys adapters. Functional compatibility with the SST-1 behavior is the target; generated XLS blocks are not required to preserve the previous cycle structure.

## Boundary

XLS owns deterministic arithmetic and bit manipulation without internal state:

- framebuffer address and write-lane packing
- depth comparison, alpha blending, fog, and color combine
- chroma/alpha tests and LFB format/swizzle logic
- texture format decode, point/bilinear filtering, and packed layout
- TMU reciprocal metadata/interpolation, perspective coordinates, gradient magnitude, LOD selection, coordinate/address generation, packed cache decode, and texture-download addressing
- triangle bounds, edge setup, interpolation load/step arithmetic, signed magnitude, and span cursor arithmetic

SpinalHDL retains stateful and protocol-oriented logic:

- Stream ready/valid stages, queues, joins, forks, and arbitration
- BMB interfaces and PCI/register handling
- state machines and outstanding-request accounting
- framebuffer and texture memories, caches, palette RAM, NCC tables, and lookup ROMs
- clock-domain crossing, HDMI, CoreSim, and DE10 platform integration

The production Scala boundary is `src/voodoo/xls/XlsBlackBoxes.scala`. Generated modules are combinational Verilog files under `xls/rtl/`; they remain separate from generated `CoreSim.v` and `De10Top.v`.

In the experimental DE10 path, XLS also owns command control, rendering state, texture-cache control, memory arbitration, transfer decomposition, and scanout selection state. `hw/de10/xls/XlsDe10Top.sv` retains Avalon signaling, MMIO response alignment, and inferred cache-bank RAMs. Qsys, PLL/reset, physical HDMI, and the current generated SpinalHDL scanout datapath remain platform plumbing.

## Sources

| DSLX source | Datapaths |
|---|---|
| `xls/dslx/pixel.x` | framebuffer addressing, write packing, depth/alpha/fog/color operations, pixel tests, LFB decode/swizzle |
| `xls/dslx/texture.x` | texture-format decode and point/bilinear filtering |
| `xls/dslx/layout.x` | packed per-LOD texture layout tables |
| `xls/dslx/tmu.x` | reciprocal, perspective, gradients, LOD, coordinates, addresses, packed decode, texture writes |
| `xls/dslx/raster.x` | triangle setup/interpolation and span arithmetic |
| `xls/dslx/core_sim.x` | hosted command/raster controller, pixel pipeline, and closed simulation wrapper |
| `xls/dslx/texture_cache.x` | hosted cache miss FSM, fill formatter, and packed bank memory |
| `xls/dslx/de10_adapter.x` | fixed memory transfers, pair-to-plane routing, and framebuffer arbitration |

`scripts/generate-xls-rtl` formats and tests every combinational DSLX source, converts each selected top to XLS IR, optimizes it, and emits plain Verilog. `xls/Makefile` lists every production generated module and maintains `xls/rtl/.stamp`. `scripts/generate-xls-rtl --hosted-proc` separately emits a 15-stage, WCT/II-1 `CoreSimHosted.sv` under `xls/build/core_sim_hosted_rtl/`. Its `--de10-proc` mode emits `CoreSimDe10.sv` plus block IR, signature, and schedule under `xls/build/core_sim_de10_rtl/` and applies Quartus-compatible generate-loop syntax. Neither proc output is part of `xls/Makefile` or the production `xls/rtl/` set.

## Reproducible Environment

The pinned `flake.lock` supplies Google XLS, Java, Scala CLI, Scalafmt, Icarus Verilog, Verilator, Yosys/SymbiYosys, SMT solvers, ccache, and a multilib GCC toolchain.

Initialize the reference-model submodule once, then enter the shell:

```bash
git submodule update --init emu/86Box
nix develop "path:$PWD"
```

Run commands directly through the shell without entering it interactively:

```bash
nix develop "path:$PWD" -c make xls
nix develop "path:$PWD" -c make -C xls test
nix develop "path:$PWD" -c make -C emu/sim model
nix develop "path:$PWD" -c make -C emu/test
nix develop "path:$PWD" -c make de10/rtl
nix develop "path:$PWD" -c make de10/xls/check
nix develop "path:$PWD" -c make de10/xls/replay
```

Production Scala, CoreSim, trace-test, and DE10 generation targets depend on `xls/rtl/.stamp`. Verilator receives all `xls/rtl/*.v` files explicitly, and the production Quartus QSF includes the same generated RTL. The experimental `SpinalVoodoo_de10_xls` revision instead regenerates `CoreSimDe10.sv`, the DE10 SystemVerilog adapters, and the scanout RTL independently.

## Adding or Changing a Datapath

1. Add the stateless function and DSLX tests to the appropriate `xls/dslx/*.x` file.
2. Add a `generate` entry to `scripts/generate-xls-rtl` and the corresponding output to `xls/Makefile`.
3. Add the exact-width blackbox declaration to `XlsBlackBoxes.scala`.
4. Instantiate the blackbox at the existing SpinalHDL timing boundary; keep handshakes, registers, memories, and control in SpinalHDL.
5. Run DSLX tests, targeted Scala tests, model builds, and a representative baseline trace comparison.

For the hosted proc network, update the spawned stages in `core_sim.x`, run `scripts/generate-xls-rtl --hosted-proc`, and verify its WCT-1 schedule and a cycle-simulated replay. No `xls/Makefile` entry is required.

Avoid reserved Verilog/SystemVerilog identifiers in DSLX port and local names because Verilator parses generated and SpinalHDL RTL together.

## Verification Record

The migration was verified with the pinned Nix shell:

- DSLX: 21 tests passed.
- Targeted Scala suites passed for write packing, color combine, fog, framebuffer access, texture decode/filtering and caching, TMU conversion/addressing, triangle setup, raster/span behavior, PCI FIFO control, SwapBuffer control, and formal invariants.
- Full Scala run: 179 tests passed and 1 was ignored across 35 suites, with no failures.
- `make -C emu/sim model`: passed, including the 32-bit C++ model build.
- `make -C emu/test`: passed after initializing `emu/86Box`.
- `make de10/rtl`: passed.
- `make de10/xls/check`: passed, including transfer and packed-bank tests plus full-wrapper lint.
- `make de10/xls/replay`: passed `test28.bin` with hash `0xad4f2b82` and exact cache counters in 8,829,847 cycles.
- Quartus analysis and synthesis passed for `SpinalVoodoo_de10_xls` at 59,342 registers, 1,201,344 block-memory bits, and 54 DSP blocks.
- The last completed Cyclone V fit succeeded at 37,190 / 41,910 ALMs (89%), 60,001 registers, 332 / 553 RAM blocks, and 54 / 112 DSP blocks; summaries are preserved under `artifacts/quartus/`.
- Scalafmt completed successfully.

Representative CoreSim/reference replays completed with these current results:

| Trace | Result |
|---|---:|
| `test04.bin` | 143 / 307200 mismatched pixels |
| `qatest00_filter.bin` | 161374 / 307200 mismatched pixels |
| `qatest00_mipmap.bin` | 113592 / 307200 pixels and 19630 / 1048576 texture words mismatched |
| `qatest00_multibase.bin` | 109808 / 307200 mismatched pixels |

These nonzero differences mean `make native/sim/check-all` is not currently a green absolute acceptance target. A clean replay build from pre-migration commit `28d1435` (with only the required CoreSim HDMI clock/reset drivers and the independently verified PCI FIFO capacity/throughput repair applied) produced byte-identical simulated framebuffer PNGs for all four traces. Texture comparison counts and reported mismatch samples were also identical. The remaining reference-model differences therefore predate the XLS datapath migration. `test02.bin` still stalls, and `native/sim/check/state-roundtrip` still fails to read its first dumped `state.bin`.
