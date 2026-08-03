# Pure XLS CoreSim

## Objective

Replace the SpinalHDL `CoreSim` hierarchy with a stateful Google XLS implementation that can replay existing Voodoo traces through XLS's native LLVM JIT/AOT runtime. The pure-XLS CoreSim path must not depend on generated Verilog or Verilator. DE10 synthesis remains a separate concern.

## Architecture

The stateful accelerator proc is `xls/dslx/core_sim.x::core_sim_hosted`. It exposes command, response, framebuffer-memory, texture-memory, and texture-block channels. `core_sim` is the closed pure-DSLX wrapper that connects the same proc to the banked XLS RAM hierarchy. Existing stateless functions in `pixel.x`, `texture.x`, `layout.x`, `tmu.x`, and `raster.x` are called directly from the stateful proc.

The migration order is:

1. MMIO/register state and derived texture-layout state.
2. PCI FIFO, command scheduling, swap state, and status.
3. Triangle setup, raster/span state, and interpolators.
4. TMU scheduling, texture memory, palette/NCC state, and texture cache.
5. Pixel pipeline, framebuffer memory, depth/color caches, and write buffering.
6. Trace loading, direct proc-channel replay, idle detection, and memory extraction.

The closed test configuration uses XLS RAM procs. The optimized replay configuration keeps all accelerator behavior and timing in XLS while servicing the explicit memory channels from mutable native arrays. This avoids copying multi-megabyte proc state and permits direct framebuffer extraction without generated RTL or Verilator.

## Current Vertical Slice

`core_sim.x` currently implements:

- byte-masked MMIO/register state, float register aliases, status, command counters, and swap state
- a PCI FIFO model with rendering-busy command ordering
- 32-bank framebuffer and texture RAM proc hierarchies with first-class 32-word texture-download bursts
- packed texture-download address mapping and texture-layout recomputation
- fast-fill color/aux writes, clip rectangles, Y origin, exact 2x2/4x4 dithering, and dither subtraction
- triangle setup, Voodoo horizontal coverage sampling, clipping, and R/G/B/A/Z/S/T/W interpolation
- ftriangle S/T/W float shadows with reference-compatible subpixel adjustment and fixed-point rounding
- de-duplicated point and bilinear texture reads, perspective coordinates, fixed LOD selection, texel extraction, and format decode
- color/alpha combine, chroma and alpha tests, destination reads, and alpha blending
- RGB565 framebuffer packing and direct BAR read/write access

The RAMs are interleaved across 32 leaf XLS procs behind an eight-way block-hash router. This preserves exact word addressing while reducing the JIT cost of updates to large proc-state arrays.

Verification:

```sh
nix develop "path:$PWD" -c interpreter_main \
  --dslx_path=xls/dslx --evaluator=ir-jit xls/dslx/core_sim.x
```

The core module currently passes twelve tests, including proc-level fast-fill, untextured triangle, textured triangle, float-shadow precision, parameter adjustment, burst mapping, scalar-equivalent burst timing, and memory readback coverage. Proc IR generation also works:

```sh
nix develop "path:$PWD" -c ir_converter_main \
  --dslx_path=xls/dslx --top=core_sim --lower_to_proc_scoped_channels \
  xls/dslx/core_sim.x
```

The native runtime lowers `core_sim_hosted` to proc-scoped IR with `__core_sim__core_sim_hosted_0_next` as its top proc. `core_sim` remains available for closed-network DSLX tests and RTL-oriented flows.

Trace replay uses `scripts/replay-pure-xls-proc-trace`. By default it caches proc-scoped hosted IR, runs a CPU-specific `opt_main` pipeline, compiles the optimized IR with `aot_compiler_main` at LLVM optimization level 3, links the position-independent object, and runs `voodoo_xls_runtime`. The runtime parses the trace, losslessly packets aligned runs of 32 texture writes, injects scalar and burst values through native XLS channel queues, and services framebuffer and texture response queues with raw native-layout generators backed by mutable arrays. These generators read and write JIT buffers directly using XLS `TypeLayout` offsets, avoiding recursive `xls::Value` conversion and per-access allocation. Register boundaries, partial runs, and non-contiguous downloads remain scalar and ordered. A framebuffer read command fences outstanding rendering before the runtime hashes the presented RGB565 pixels directly from native memory. `--interpreter` retains the generated closed `#[test_proc]` path and its row/block diagnostics.

The complete `test04.bin` presented framebuffer matches the reference at 640x480 with raw RGB565 FNV-1a hash `0x790a92b8`. A burst advances the command, inactive raster, and inactive fast-fill substates for the same additional 31 cycles as 32 scalar writes, but without copying the full `CoreState` on every iteration; a DSLX regression test compares this reduced accumulator with 31 complete scalar ticks. Burst replay reduces `qatest00_filter.bin` from 44,319 individual activations to 2,004 ordered operations and completes in about 1.52 seconds; its current framebuffer hash is `0x1053a945`, so reference parity still fails from row 0. The remaining rendering gaps are palette and NCC table state, texture-cache timing, fog, stipple, complete swap/retrace timing, LFB pixel-pipeline writes, statistics, and parity for the filter, mipmap, and multibase traces.

## Native XLS Runtime

`nix/xls.nix` builds Google XLS revision `ff755d2e5` from source with the pinned Bazel dependency closure. It exports `eval_proc_main`, `aot_compiler_main`, and `voodoo_xls_runtime`; `flake.nix` combines those binaries with the standard tools from the matching upstream release. `nix/xls-tools.patch` uses the Nix Clang 19 toolchain instead of downloading an LLVM wrapper that cannot survive the sandboxed Bazel repository archive.

Build and replay:

```sh
nix build "path:$PWD#xls"
nix develop "path:$PWD" -c scripts/replay-pure-xls-proc-trace traces/test04.bin
```

Pass `--jit` to compile the cached CPU-optimized IR in-process instead of loading the cached AOT object, or `--interpreter` to use the legacy closed test proc. `--hardware-opt` builds and runs a separate artifact through the unmodified default XLS pipeline; `--hardware-opt --generate-only` is the full-optimization validation gate.

### Proc optimization root cause

The hosted memory protocol performs several ordered sends and receives on each memory channel during one proc activation. Those channels initially inherited XLS's `proven_mutually_exclusive` default. During pass invocation 931, `channel_legalization` therefore discarded token-order information and asked Z3 to prove all predicates mutually exclusive over the complete inlined CoreSim graph with no solver resource limit. The proof did not finish within 15 minutes; the same pass rejected the closed replay proc when it found multiple non-exclusive sends.

`core_sim_hosted` now enables DSLX channel attributes and declares its boundary channels `total_order`. This matches the existing token chain, lets `channel_legalization` use the projected token DAG instead of an unbounded mutual-exclusion proof, and preserves transaction order across activations. The unmodified default XLS pipeline now completes in about eight seconds; no optimization pass is skipped.

The default pipeline is designed for hardware and fully inlines the proc graph. It remains the synthesis-oriented correctness gate, while native replay uses hierarchical IR so LLVM's CPU cost model can choose helper boundaries. Both products preserve the exact burst-cycle advancement and framebuffer hash.

The CPU pipeline runs five controlled rounds of `pre-inlining` and `one-leaf-inlining`, then a final `pre-inlining` and `fixedpoint_simp(3)`. It currently leaves 20 optimized helper functions and 42 invokes, including the expensive tick, raster, TMU, and fast-fill boundaries, in about 960 KiB of IR. The full hardware pipeline remains independently runnable through `--hardware-opt`; it currently optimizes in about 6.33 seconds and replays `test04` in about 1.01 seconds.

A reference image adds an enforced hash gate:

```sh
nix develop "path:$PWD" -c scripts/replay-pure-xls-proc-trace \
  traces/test04.bin --framebuffer-reference path/to/test04_ref.png
```

## Native profiling

The development shell includes `perf`, Valgrind, and LLVM 19 tools. Profile the cached AOT artifact without charging compilation to the sample:

```sh
nix develop "path:$PWD" -c perf stat -d -- \
  result/bin/voodoo_xls_runtime xls/build/core_sim_hosted.ir traces/test04.bin \
  --aot_proto=xls/build/core_sim_hosted.pb \
  --aot_library=xls/build/core_sim_hosted.so --expected_hash=0x790a92b8
```

The original CPU pipeline retired about 17.8 billion instructions and spent 46.8% of sampled cycles in `FunctionBase::GetNodeById`. Every blocked proc continuation linearly rescanned all IR nodes to recover its send or receive node. `nix/xls-runtime-performance.patch` now builds a continuation-point-to-`Node*` map once in `ProcJit`; that reduced warm replay from 2.80 to about 1.47 seconds without changing ticks or output.

The next profile was dominated by `JitRuntime::UnpackBufferInternal`, `UBitsWithStatus`, `InlineBitmap::FromBytes`, queue callbacks, allocations, and locks in host memory responses. The same patch adds raw JIT queue generators and exposes native `TypeLayout` metadata. `voodoo_xls_runtime` now decodes requests and writes responses directly in JIT buffers, preserving callback behavior when observers are present. Combined with the reduced burst-tick accumulator, final `perf stat` measurements are about 3.39 billion cycles, 7.97 billion instructions, 2.3 IPC, and 204 million L1 data-cache load misses. Generated AOT helpers and state-copy `memmove` calls are now the principal remaining costs.

Measured timings on the current host are:

| Stage | Time |
|---|---:|
| DSLX to hosted proc IR | 2.38 s |
| CPU-specific XLS optimization | 0.89 s |
| CPU-optimized IR to LLVM-O3 AOT object and metadata | 3.02 s |
| AOT shared-object link | 0.25 s |
| Warm CPU-optimized AOT replay plus direct 640x480 hash, median of 7 | 0.68 s |
| Warm CPU-optimized AOT process wall time, median of 7 | 0.83 s |
| In-process CPU JIT compilation | 3.22 s |
| Warm CPU JIT replay | 0.69 s |
| Full hardware XLS optimization | 6.33 s |
| Warm hardware-optimized AOT replay, median of 5 | 1.01 s |
| Verilator full framebuffer validation | 83.91 s |

The CPU-optimized AOT run executes 3,037 `test04` commands as 1,084 ordered operations over 141,515 CoreSim ticks and produces `0x790a92b8`. Its warm replay is about 4.1 times faster than the earlier 2.80-second native path, 1.5 times faster than the hardware-optimized AOT product, and 123 times faster than the measured Verilator full-validation path. Shell creation through `nix develop` is excluded because flake evaluation is independent of simulation.
