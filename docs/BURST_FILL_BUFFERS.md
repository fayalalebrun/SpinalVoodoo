# Burst Fill Buffer Design

## Scope
- Add burst-based, scanline-aware fill buffers in front of framebuffer and texture RAM.
- Keep the existing raster, TMU, and color math behavior unchanged.
- Optimize for sequential raster order and bilinear texture reuse.

## Goals
- Convert per-pixel RAM traffic into aligned burst traffic.
- Hide RAM latency behind scanline-local buffering.
- Reduce arbiter pressure on `fbMem` and `texMem`.
- Allow the TMU to read the four bilinear texels from local banks in one cycle after fill.

## Non-Goals
- No change to texture filtering equations, LOD selection, or framebuffer blending rules.
- No general-purpose cache hierarchy.
- No attempt to reorder visible writes across pipeline boundaries.

## High-Level Structure
- `FramebufferAccess` gets scanline fill buffers for color and aux reads.
- `Tmu` gets a four-bank texture fill buffer in front of `texRead`.
- Existing RAM arbiters remain, but they now see mostly burst traffic instead of isolated reads.

## Framebuffer Fill Buffer

### Organization
- One color fill buffer and one aux fill buffer.
- Each buffer stores one active burst line and one prefetched next line.
- A line is an aligned run of framebuffer words on one screen scanline.

### Addressing
- Tag fields: `y`, aligned word base `xWordBase`, plane select.
- Lookup key is derived from the existing `pixelFlat` address.
- Buffer line size should be a power of two in words; initial target: `8` or `16` 32-bit words.

### Access Model
- On hit, return the requested 16-bit lane from the buffered 32-bit word.
- On first miss in a span, allocate the aligned line containing the request.
- If raster access stays sequential, prefetch the next aligned line ahead of demand.

### Burst Policy
- Reads are issued as aligned incrementing bursts.
- Burst length equals the line size.
- Do not cross a scanline boundary in one burst.
- Color and aux may issue independently, but should use the same line geometry.

### Expected Behavior
- Horizontal spans should become mostly hits after the first access per line.
- Depth/color read traffic becomes bursty and predictable.
- `FramebufferAccess` continues to retire pixels in order.

## Texture Fill Buffer

### Organization
- A four-bank fill buffer sits inside `Tmu` after address generation and before texel decode.
- Each resident texture line is split across four banks by low texel address bits.
- The buffer stores two resident rows per active footprint: current row and adjacent row.
- Each row may also hold one prefetched next line.

### Bank Mapping
- Bank select: low two bits of texel word index within the filled line.
- Data for a filled line is striped across banks `0..3` in round-robin order.
- Each bank provides one read per cycle.
- A bilinear sample may read the four texels in one cycle when all four texels are resident.

### Fill Granularity
- Fill unit is an aligned texture line fragment, not a full mip row.
- Initial target line size: `8` or `16` 32-bit words per burst.
- Line alignment is based on post-layout SRAM address, not logical `S/T`.

### Access Model
- Point sample: one lookup into the resident line.
- Bilinear sample: two adjacent texels from one row and two from the next row.
- If both rows are resident, the sample reads all four banks locally and bypasses RAM.
- On miss, the fill engine fetches the required aligned line for the row, then the paired row if bilinear needs it.

### Burst Policy
- Texture fills are aligned incrementing bursts on `texRead`.
- Burst requests must not cross the chosen fill-line boundary.
- The prefetch target is the next line in the dominant address direction of the current sample stream.
- Prefetch is suppressed when address deltas indicate low locality.

### Locality Model
- Neighboring screen pixels usually reuse nearby texture addresses even with perspective correction.
- Bilinear always has a 2x2 texel footprint; the buffer is optimized for this exact case.
- Trilinear, if added later, can reuse the same design per selected LOD.

## Control Rules

### Allocation
- One miss allocates one line slot.
- If a line is already in-flight, later requests to the same line merge behind it.
- Replacement policy is simple FIFO or two-entry MRU; no associative cache behavior is needed beyond the active span.

### In-Flight Tracking
- Each buffer line has `valid`, `filling`, and `prefetched` state.
- Consumers may stall only when the needed line is neither valid nor already filling.
- Outstanding bursts are tracked per client so responses fill the correct line slot.

### Ordering
- Framebuffer read responses must be associated with the original pixel order already maintained by `FramebufferAccess`.
- Texture fill responses may complete out of order only if the line tracker can route them unambiguously; otherwise keep one outstanding burst per TMU fill stream.

## Bus Requirements
- `fbMem` and `texMem` paths must support multi-beat BMB reads.
- `cmd.length` is set to the burst byte count minus one, matching the chosen line size.
- `last` is asserted only on the final beat of the burst.
- Fill engines consume full burst responses and write them into local line storage.

## Arbitration Expectations
- Existing arbiters remain the top-level merge points.
- Fill buffers reduce request count but each request is longer.
- Priority should remain with latency-sensitive read clients over CPU-side bulk traffic.
- Write traffic is unchanged by this design.

## Parameters
- Framebuffer line size in words.
- Texture line size in words.
- Number of resident lines per client (`2` active + optional `1` prefetched is sufficient to start).
- Maximum outstanding bursts per client.

## Recommended Initial Targets
- Framebuffer: `2` resident lines per plane, `8` words per burst line.
- Texture: `2` resident rows x `2` line slots per row, `4` banks, `8` words per burst line.
- Outstanding bursts: `1` per framebuffer plane, `1` per TMU fill stream initially.

## Resulting Dataflow
- `FramebufferAccess`: pixel request -> line lookup -> hit returns word locally, miss triggers aligned burst fill.
- `Tmu`: texel footprint -> bank lookup -> local one-cycle bilinear read on hit, burst fill on miss.
- RAM sees fewer, longer, aligned reads; compute stages mostly interact with local buffers.

## Verification Plan

### Correctness
- Add a configuration switch to disable fill buffers and compare buffered vs unbuffered runs on the same traces.
- Reuse existing framebuffer compare flow; outputs must remain bit-exact.
- Add assertions for burst assembly: aligned start, expected beat count, correct `last`, no scanline-crossing burst in framebuffer mode.
- Add assertions for fill-buffer state: no duplicate active fill for one line, no response routed to the wrong slot, no hit on invalid data.
- Add assertions for TMU bank access: bilinear hit reads at most one entry per bank per cycle and returns the same four texels as the baseline path.
- Add directed unit tests for edge cases: line boundary, row boundary, bilinear footprint spanning two lines, LOD change, and halfword lane selection on framebuffer reads.

### Trace-Based Performance Checks
- Instrument counters for framebuffer and TMU separately.
- Minimum counters: line hits, line misses, prefetched hits, merged misses, consumer stall cycles, burst count, burst beats, and average beats per burst.
- TMU-specific counters: bilinear full-hit count, partial-hit count, four-bank single-cycle hit count, and prefetch drop count.
- Framebuffer-specific counters: color hit rate, aux hit rate, and cases where color/aux locality diverge.
- Run the existing traces, especially `screamer2`, and dump per-trace counter summaries.

### Success Criteria
- Framebuffer and texture outputs remain bit-exact against the current design.
- RAM request count drops materially while average burst length rises toward the configured line size.
- Consumer stall cycles drop on texture-heavy traces.
- TMU bilinear accesses hit locally often enough to justify the four-bank design; this should be reported directly as a percentage of bilinear samples.
