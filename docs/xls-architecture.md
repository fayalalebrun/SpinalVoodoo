# XLS Architecture

SpinalHDL and XLS are independent implementations of the Voodoo core.

## Native SpinalHDL

Production Scala under `src/voodoo/` owns the complete native implementation, including arithmetic, streams, state machines, caches, memories, and platform integration. Native simulation and the `SpinalVoodoo_de10` Quartus revision compile only RTL generated from that implementation.

## Pure XLS

The pure-XLS implementation lives under `xls/dslx/`. `xls/dslx/core_sim.x` composes the hosted command controller, rasterizer, pixel pipeline, texture sampler, texture cache, and memory interfaces as an XLS proc network.

`scripts/generate-xls-rtl --de10-proc` emits `CoreSimDe10.sv` and its schedule artifacts under `xls/build/core_sim_de10_rtl/`. The DE10 adapters under `hw/de10/xls/` connect that generated core to Avalon, Qsys, cache-bank memories, and scanout. The separate Quartus revision is `SpinalVoodoo_de10_xls`.

The pure-XLS replay, simulation, and DE10 targets are documented in `docs/pure-xls-coresim.md`.

### Pipeline FIFO storage

The DE10 RTL compatibility pass maps the registered-output `pixel_work` and `pixel_texture` FIFOs to synchronous M10K storage. The `pixel_cache_request`, `pixel_read_job`, and `palette_pending` FIFOs retain their original first-word timing by reading the next tail synchronously and forwarding same-address writes, without adding a pop stage. Queue capacities, backpressure, and valid-qualified cycle timing remain unchanged. Reset clears queue control, not unused payload memory; consumers must observe `pop_valid`. The unmodified XLS FIFO RTL is retained as `CoreSimDe10.register-fifos.sv`, and the RTL check compares both implementations across fill, drain, wraparound, stalls, and resets.

Depth-one FIFOs without bypass or a registered pop use a single payload register and a full bit instead of the generated two-slot ring. Push readiness remains independent of pop readiness, including a full queue refusing a simultaneous replacement. FIFO equivalence tests cover every matching generated type as well as the RAM-backed queues.

### Texture tags

The cache retains 64 primary lines and 16 shadow lines. `XlsTextureTagMemory.sv` stores tags in four replicated synchronous RAM pairs, providing four tap lookups. Primary fills also update shadow tags. Read/write collisions forward the new tag. The RAM output and a skid register hold two responses under backpressure and sustain one lookup per cycle when the consumer is ready.

The DSLX controller owns validity bits, invalidation, and ordering. It captures a request before reading tags and consumes the registered response in a separate phase; mixed-hit requests retain their hit mask during serial bank reads. Reset clears validity and pending responses without clearing tag RAM. The DSLX `tag_memory` proc models this storage for hosted tests; DE10 exposes separate tag-read, tag-response, and tag-write channels to the RTL memory.

Deferred texture lookups retain the captured request's texture layout until any background fill completes. The layout comparison is captured with the request, and an all-hit completion can capture the next request in the same iteration. The DE10 pixel-texture queue has nine entries to cover lookup latency without reducing hit throughput. The fill-pending and fill-result queues register their outputs to separate fill-completion decoding from cache allocation. Separate two-entry controller and fill bank-response queues decouple bank dispatch from cache response consumption. Lookup invalidation masks the four selected hits rather than the full validity arrays before tap selection; formal equivalence preserves invalidating-request behavior.

Primary-cache hits may read the acknowledged prefix of an active fill. Completion and progress messages carry a two-bit fill generation, and the controller accepts only monotonic progress for its active generation. Shadow fills retain the conservative same-line exclusion. The ordered bank acknowledgments, bounded progress queues, and unchanged invalidation barrier keep unfinished or stale fill data invisible.

### Texture addressing

Cache bank lanes use physical byte-address bits rather than mip-relative offsets, preventing adjacent tiny mip levels from overwriting each other within a filled word. The cache controller resolves narrow paletted requests after the pixel-request queue: safe layout mappings reuse cached samples, while wrapped samples retain the invalidating fallback. Texture writes and layout changes still invalidate cached data. DE10 code generation uses six pipeline stages to separate perspective rounding, coordinate generation, and clamping.

Reciprocal normalization combines leading-zero detection and alignment in a binary shift tree rather than a wide priority encoder followed by a barrel shifter; formal equivalence includes zero and negative W. Coordinate clamping detects overflow with a dimension mask instead of a size comparison. Packed texel addresses use carry-save reduction before their final addition. Both transformations preserve the original fixed-width overflow behavior, with quickchecks formally proving equivalence over all input values.

Fast bank requests pass through a registered request and skid slot before collision checking and bank dispatch. The skid slot separates upstream readiness from dispatch, avoiding a combinational loop through the XLS controller. The DE10 pixel-texture, pixel-work, and pixel-result queues register their outputs, separating queue selection and validity decoding from sampler and command-controller logic.

Texture download addressing applies the byte-width shift before the variable row shift and uses carry-save reduction for base, row, and column addition. Formal equivalence covers all inputs, including invalid LODs and fixed-width overflow.

The primary cache RAMs connect their write address directly to port A because that port never reads. Shared read/write ports select the read address only for an enabled read without a write. This removes a late write-enable gate from the RAM address path while preserving write priority and all enabled read/write addresses; the behavioral model uses the same address selection.

### Command timing

Register-file byte writes decode physical register indexes directly and merge data locally in each selected word, avoiding compact-index re-encoding and a shared read-modify-write mux across the entire register file. A formal equivalence check covers all register indexes, data values, and byte masks. The H2F spill FIFO reads its egress RAM synchronously into a backpressure-held output register. Batch expansion has a separate registered output before core acceptance.

Floating-point register writes capture their command and submit both narrow-register and high-precision shadow conversion to the existing pipelined converter. The controller applies the result and acknowledges completion before accepting another command. DE10 disables the unused hosted-command input explicitly, preventing a producerless channel from generating phantom status responses. MMIO captures response payloads on the valid handshake, not one cycle later; replay rejects unsolicited responses.

The XLS scanout wrapper pipelines framebuffer prefetch and pixel-read requests in both directions. These timing cuts preserve addresses, spans, pixel order, and backpressure without changing the native scanout implementation. Posted framebuffer writes register their busy barrier, asserting on the first accepted write and releasing only after both the ingress queue and write engine drain.

### Prefetched raster spans

The DE10 span prefetcher sends each completed, unclipped row span to the command controller after submitting the framebuffer read spans. The controller emits pixels directly from the inclusive span endpoints instead of repeating the edge-equation search. Empty rows still advance the raster and the last row still retires the triangle. The registered row queue preserves prefetch ordering and backpressure; the non-prefetched hosted controller retains its bounded raster walk.

### Timing analysis

The DE10 timing reports force delay annotation when creating the post-fit netlist. With Quartus 24.1, a cached annotation exposed synthetic `_smrt` nodes instead of HPS hard-PHY registers, preventing the generated memory constraints from matching and causing spurious HPS violations and a failed vendor DDR report. `create_timing_netlist -force_dat` restored the expected nodes and allowed the unchanged vendor constraints and DDR report to run correctly; no timing exceptions were added.

For signoff, analyze all available operating conditions with fresh timing netlists and run the generated `hps_sdram_p0_report_timing.tcl` after loading the SDC. Creating each corner separately avoids the observed Quartus internal error when switching operating conditions on the cached HPS netlist. Passing STA does not replace board replay and gameplay validation.

## Separation Rule

Do not instantiate generated XLS datapaths from production SpinalHDL. Shared behavior must remain independently implemented and tested in both designs. Platform-only wrappers may be shared when they do not move core logic across the implementation boundary.

## Commands

```bash
nix develop "path:$PWD" -c make -C xls test
nix develop "path:$PWD" -c make native/sim/build
nix develop "path:$PWD" -c make de10/rtl
nix develop "path:$PWD" -c make de10/xls/check
nix develop "path:$PWD" -c make de10/xls/replay
```
