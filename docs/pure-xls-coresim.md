# Pure XLS CoreSim

## Objective

Replace the SpinalHDL simulation hierarchy with a stateful Google XLS implementation that can replay existing Voodoo traces through XLS's native LLVM JIT/AOT runtime. The pure-XLS CoreSim and behavioral DE10Sim paths must not depend on generated Verilog or Verilator. DE10 synthesis remains a separate concern.

## Architecture

The hosted top is `xls/dslx/core_sim.x::core_sim_hosted`. It spawns `core_sim_hosted_controller` for MMIO, register, raster, and fast-fill state; `hosted_pixel_pipeline` for per-pixel texture, depth, blend, and commit work; and the real `texture_cache::texture_cache` proc. Pixel jobs, cache requests, and results cross internal streaming channels. Paired framebuffer read, write, and fast-fill channels provide two memory operations per activation. The controller's `pixels_pending` barrier prevents later commands from passing a raster until the final `PixelResult` confirms that all pixel writes have drained. `core_sim` remains the closed pure-DSLX wrapper around `core_sim_impl` and the banked XLS RAM hierarchy.

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
- a spawned DE10-compatible `texture_cache::texture_cache` with 64-word lines, 64 two-way slots, XOR indexing, rotating victims, write invalidation, four packed banks, and stale-bank preservation
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

The native runtime lowers the complete `core_sim_hosted` proc network to proc-scoped IR. `core_sim` remains available for closed-network DSLX tests and RTL-oriented flows.

Trace replay uses `scripts/replay-pure-xls-proc-trace`. By default it caches proc-scoped hosted IR, runs a CPU-specific `opt_main` pipeline, compiles the optimized IR with `aot_compiler_main` at LLVM optimization level 3, links the position-independent object, and runs `voodoo_xls_runtime`. The runtime parses the trace, injects requests through native XLS channel queues, and backs scalar framebuffer traffic, paired pixel read/write/fast-fill traffic, texture block reads/writes, and four packed cache-bank channels with mutable arrays. Raw generators use XLS `TypeLayout` offsets rather than recursive `xls::Value` conversion. Register boundaries remain scalar and ordered. The final `PixelResult` is the render-drain barrier before a framebuffer read and hash. `--interpreter` retains the generated closed `#[test_proc]` path and its row/block diagnostics.

The complete `test04.bin` presented framebuffer matches the reference at 640x480 with raw RGB565 FNV-1a hash `0x790a92b8`. A burst advances the command, inactive raster, and inactive fast-fill substates for the same additional 31 cycles as 32 scalar writes, but without copying the full `CoreState` on every iteration; a DSLX regression test compares this reduced accumulator with 31 complete scalar ticks. Burst replay reduces `qatest00_filter.bin` from 44,319 individual activations to 2,004 ordered operations. With the production cache geometry, the DE10 path completes it in about 3.1 seconds; its current framebuffer hash is `0xf98a1435`, so DE10 packed-cache artifact parity still differs. The remaining rendering gaps are palette and NCC table state, packed-cache artifact parity, fog, stipple, complete swap/retrace timing, LFB pixel-pipeline writes, statistics, and parity for the filter, mipmap, and multibase traces.

## Native XLS Runtime

`nix/xls.nix` builds Google XLS revision `ff755d2e5` from source with the pinned Bazel dependency closure. It exports `eval_proc_main`, `aot_compiler_main`, and `voodoo_xls_runtime`; `flake.nix` combines those binaries with the standard tools from the matching upstream release. `nix/xls-tools.patch` uses the Nix Clang 19 toolchain instead of downloading an LLVM wrapper that cannot survive the sandboxed Bazel repository archive.

Build and replay:

```sh
nix build "path:$PWD#xls"
nix develop "path:$PWD" -c scripts/replay-pure-xls-proc-trace traces/test04.bin
```

Pass `--jit` to compile the cached CPU-optimized IR in-process instead of loading the cached AOT object, or `--interpreter` to use the legacy closed test proc. `--hardware-opt --generate-only` validates the same cache-enabled proc graph with the full default XLS hardware optimization pipeline.

## Live DOSBox-X bridge

The 32-bit Glide bridge can drive a persistent pure-XLS AOT runtime over a private Unix socket. Register, LFB, and texture operations remain synchronous and ordered; texture downloads are sent in bulk, and swap presentation reads packed RGB565 directly from the runtime's framebuffer array. The X11 presenter displays the resulting frames inside the DOSBox-X window.

```sh
nix develop "path:$PWD" -c make dos/xls/run/df00sdk
nix develop "path:$PWD" -c make dos/xls/dosbox ARGS='-c "mount c /path/to/demo" -c "c:" -c "demo.exe"'
nix develop "path:$PWD" -c make tomb/xls/run
```

Headless equivalents are `dos/xls/headless/<name>` and `tomb/xls/headless`. The launcher prepares cached proc IR/AOT artifacts, starts `voodoo_xls_runtime --server=SOCKET`, exports `SIM_XLS_SOCKET` to the Glide library, and cleans up both the server and socket when DOSBox-X exits. The server log defaults to `xls/build/dosbox-xls-server.log`.

## Pure XLS DE10Sim

`--de10` injects each scalar operation through the `De10MmioRequest` channel. XLS performs the DE10 H2F word-address to byte-address conversion, read/write validation, byte-enable propagation, command ordering, and response handling before the shared accelerator state sees the request. Texture bursts remain an explicitly ordered simulation acceleration path. Native arrays service the accelerator's explicit framebuffer and texture memory channels, so this behavioral DE10Sim has no SpinalHDL, generated Verilog, or Verilator dependency.

```sh
nix develop "path:$PWD" -c make native/xls-de10sim/check/test04 \
  XLS_DE10SIM_REFERENCE=path/to/test04_ref.png
nix develop "path:$PWD" -c make native/xls-de10sim/run/screamer2
```

Generate 86Box-reference and pure-XLS framebuffer screenshots for every trace with:

```sh
make native/xls-de10sim/compare-all
```

The report is written to `test-output/emulation-xls-comparison/index.html`. Each trace has full-resolution 86Box, XLS, and highlighted-difference panels; `summary.csv` records exact mismatch counts, RMSE, and maximum channel error. The comparison script can resume existing captures with `--reuse`. The native runtime's `--framebuffer_output=FILE` option writes a tightly packed little-endian RGB565 image for conversion or comparison.

When a trace is in a directory, the native runtime automatically loads its sibling `state.bin`. It replays non-command registers in snapshot order with the DE10 high-register remap, initializes native framebuffer and TMU0 texture memory, restores swap parity, and starts with an empty texture cache before replaying `trace.bin`. `--state=FILE` selects an explicit snapshot.

### DE10 RTL and Quartus

The hardware path emits `xls/build/core_sim_de10_rtl/CoreSimDe10.sv` as an 8-stage, WCT/II-1 proc network. `XlsDe10Top.sv` connects its MMIO, four sequential DDR transfer streams, and four packed texture-cache banks to the DE10 platform. GPU and scanout selection state remain in XLS; Avalon, Qsys, PLL, physical HDMI, and the current generated SpinalHDL scanout datapath remain thin platform plumbing.

```sh
nix develop "path:$PWD" -c make de10/xls/rtl
nix develop "path:$PWD" -c make de10/xls/check
nix develop "path:$PWD" -c make de10/xls/replay
nix develop "path:$PWD" -c make de10/xls/bitstream
```

The replay builds `VCoreSimDe10` from the generated RTL and requires `traces/test28.bin` to produce framebuffer hash `0xad4f2b82` with cache counters 10,208 hits, 32 misses, 2,546 all-hit requests, 14 missed requests, and 14 line fills. The fit-oriented 8-stage run takes 8,829,847 cycles.

Quartus analysis and synthesis succeed for revision `SpinalVoodoo_de10_xls`, including HPS/Qsys and HDMI PLL integration. Current synthesis reports 59,342 registers, 1,201,344 block-memory bits, and 54 DSP blocks. The last completed Cyclone V fit used 37,190 / 41,910 ALMs (89%), 60,001 registers, 332 / 553 RAM blocks, and 54 / 112 DSP blocks. Persisted summaries are under `artifacts/quartus/`.

This path currently validates accelerator-visible DE10 MMIO behavior and framebuffer results. Its texture cache matches the SpinalHDL CoreDe10Sim and FPGA geometry and traffic policy: 64 slots, two ways, 64 32-bit words per line, a 32-set three-fold XOR index, rotating victims, serialized lookup of up to four taps, one 64-word backing transaction per distinct missing line, serialized byte formatting into four packed banks, and invalidation on texture writes. Taps covered by an in-flight line are captured directly from its block response, including decode-edge taps outside a packed-layout segment. A complete state-backed Screamer 2 replay produces hash `0x698a7c90`, 2,488,149 tap hits, 43,219 misses, 618,389 all-hit requests, 14,453 missed requests, and 19,673 line fills. `make native/xls-de10sim/run/screamer2` requires both hit rates to remain at least 90%. Cycle-accurate Avalon wait states, bridge wedge diagnostics, HDMI scanout, and retrace cadence remain outside the behavioral model and require separate interface tests until represented as XLS procs.

### Hosted proc scheduling

The split network gives each steady-state hosted proc at most one operation on each channel per activation. `proven_mutually_exclusive` channel strictness is therefore valid without a global ordered-token recurrence. Cache misses hold the cache response and naturally backpressure the pixel producer; an all-hit sample can launch every clock. The cache miss FSM still performs at most one backing-memory block read and one whole-line operation per packed bank in each fill activation.

Generate the scheduled network with:

```sh
nix develop "path:$PWD" -c scripts/generate-xls-rtl --hosted-proc
```

This emits `xls/build/core_sim_hosted_rtl/CoreSimHosted.sv`, block IR, signature, and schedule using 15 pipeline stages, explicit depth-one internal FIFOs, and `--worst_case_throughput=1`. `scripts/generate-xls-rtl --de10-proc` emits the board-facing `CoreSimDe10.sv` network with fixed transfer and packed-bank channels. A Verilator replay of `test28.bin` completes in 592,559 cycles through the hosted interface and 8,829,847 cycles through the fit-oriented DE10 transfer interface, with hash `0xad4f2b82` and exact cache counters.

The CPU pipeline runs five controlled rounds of `pre-inlining` and `one-leaf-inlining`, then a final `pre-inlining` and `fixedpoint_simp(3)`. It keeps the expensive tick, raster, TMU, cache, and fast-fill boundaries available to LLVM's CPU cost model rather than flattening the complete proc graph.

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
| Warm cache-enabled DE10Sim AOT replay plus direct 640x480 hash | 0.87 s |
| Warm hardware-optimized DE10Sim AOT replay plus direct 640x480 hash | 1.28 s |
| Warm `qatest00_filter` DE10Sim AOT replay | 2.55 s |
| State-backed Screamer 2 DE10Sim AOT replay | 12.49 s |
| Optimized Verilator DE10Sim `qatest00_filter` validation | 22.3 s |

The cache-enabled AOT run executes 3,037 `test04` commands as 1,084 ordered operations over 141,533 behavioral DE10Sim ticks and produces `0x790a92b8`. `qatest00_filter` executes 44,319 commands as 2,004 operations over 287,392 ticks. Shell creation through `nix develop` is excluded because flake evaluation is independent of simulation.
