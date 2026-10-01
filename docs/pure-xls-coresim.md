# XLS simulation and replay

The hosted XLS model runs through native LLVM JIT/AOT without generated Verilog
or Verilator. The RTL replay is a separate correctness check. Neither simulation
throughput nor behavioral tick counts establish live-board FPS.

## DSLX and native replay

```sh
nix develop --command make -C hardware/renderer test
nix develop --command workflows/run/replay-pure-xls-proc-trace \
  verification/fixtures/traces/test04.bin
```

The replay workflow caches proc IR and AOT artifacts under `build/generated/xls/work/`.
Use `--jit` for in-process compilation, or `--interpreter` for the closed test-proc
path. To check a reference image, add `--framebuffer-reference path/to/reference.png`.

The hosted top is `hardware/renderer/dslx/core_sim.x::core_sim_hosted`.
Mutable native arrays service its memory channels. Register operations stay ordered;
pixel retirement prevents later commands from passing unfinished rendering.

## Behavioral DE10 replay

This path includes DE10 MMIO address conversion, byte enables and command ordering.
It does not model physical Avalon wait states, HDMI or board retrace cadence.

```sh
nix develop --command make native/xls-de10sim/check/test04 \
  XLS_DE10SIM_REFERENCE=path/to/test04_ref.png
nix develop --command make native/xls-de10sim/run/screamer2
nix develop --command make native/xls-de10sim/compare-all
```

The comparison gallery is
`build/test-results/replay/emulation-xls-comparison/index.html`.
It includes reference, XLS and difference images, plus mismatch counts in `summary.csv`.
A trace directory's sibling `state.bin` supplies initial registers and memory;
`--state=FILE` selects an explicit snapshot. `--framebuffer_output=FILE` writes
packed little-endian RGB565 pixels from the native runtime.

## Hosted DOSBox-X

```sh
nix develop --command make dos/xls/run/df00sdk
nix develop --command make dos/xls/dosbox
nix develop --command make tomb/xls/run
```

The launcher owns a private runtime/socket and cleans up both when DOSBox exits.
The 32-bit Glide frontend sends ordered register, texture and LFB operations over
that socket. Headless targets are listed by `make dos/help` and `make tomb/help`.
The native server log is `build/generated/xls/work/dosbox-xls-server.log`.

## RTL and FPGA

```sh
nix develop --command make de10/xls/rtl
nix develop --command make de10/xls/check
nix develop --command make de10/xls/replay
nix develop --command devenv shell -- make de10/xls/bitstream
```

Generation emits `build/generated/xls/work/core_sim_de10_rtl/CoreSimDe10.sv`.
The replay harness checks pixels, hashes and completion separately from timing.
It selects refill-size-specific cache counter fixtures; do not use historical
full-line counters to validate a sectored fill.
Complete Quartus compilation plus required timing signoff must fit within 900 seconds.

See [the architecture](../ARCHITECTURE.md) for renderer/platform boundaries,
[profiling](PROFILING.md) for measurement rules, and
[the experiment journal](SCREAMER2_EXPERIMENT_JOURNAL.md) for retained results.
