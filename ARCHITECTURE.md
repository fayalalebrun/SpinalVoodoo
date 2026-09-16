# XLS DE10 architecture

## Build and integration

`xls/dslx/core_sim.x` implements the hosted rendering core. `scripts/generate-xls-rtl --de10-proc` generates `xls/build/core_sim_de10_rtl/CoreSimDe10.sv`; `scripts/quartus-compat-xls-rtl` lowers selected FIFOs for the FPGA flow without changing their handshake or capacity.

`hw/de10/xls/XlsDe10Top.sv` connects the generated core to MMIO, texture-tag and palette memories, packed texture banks, framebuffer read/prefetch/write adapters, Avalon memory masters, and scanout. SpinalHDL-generated adapter RTL remains under `hw/de10/rtl/`.

The pixel path separates interpolation, texture sampling/cache lookup, framebuffer reads, pixel processing, posted writes, and retirement with handshaked channels. Cache-fill completion and pixel retirement are distinct acknowledgments.

## Texture cache

The primary cache retains 64 hashed 256-byte line identities; the shadow cache retains 16. Bank read bit 12 selects primary storage. Bank write bit 16 duplicates a write into primary and shadow storage, with shadow-set bits 12–15. Sectoring does not change these address fields or physical bank capacity.

The DE10 specialization enables two 128-byte sectors per line. `texture_cache::SECTOR_WORDS` is 32 and also configures `de10_adapter::block_read_transfer_adapter`, keeping the issued DDR word count and consumed fill length identical. The default non-sector specialization and block-read adapter still use full 64-word fills for existing non-DE10 paths and fixtures.

Sector-valid masks distinguish fetched portions of a line. Four per-request primary-tag snapshots track tag ownership through colliding tap allocations, avoiding a duplicate 64-entry primary-tag register array. Shadow metadata is maintained separately. A replacement tag or invalidated line discards the old sector mask.

Fills encode even/odd texels into both row-bank copies. For eight-bit textures, the two texel bytes in each bank word are expanded to the existing 16-bit texel representation. For sixteen-bit textures, the selected halfword occupies the valid lower half of each corresponding bank word. This representation is independent of texture layout, so layout-only changes need not invalidate cached data. Eight/sixteen-bit format changes, explicit invalidation, texture writes, and existing invalidating narrow requests still invalidate the cache.

A sector may be allocated before all of its words are written. Generation-tagged progress is emitted only after bank-write acknowledgment; the existing fill-prefix checks prevent reads of unwritten words. Prefetched DDR data alone does not establish a completed cache fill.

## Validation and timing

Use `nix develop` for DSLX tests, RTL generation, Verilator, and lint. `make -C xls test`, `scripts/check-de10-xls-rtl`, and `scripts/check-de10-xls-replay` cover the principal XLS paths. The replay harness preserves the original 64-word counter fixtures and selects separate 32-word fixtures only when that refill size is observed; pixel/hash/completion checks remain mandatory.

Presented framebuffer and full 4 MB replay comparisons are separate from timing and throughput measurements. A simulation speedup is not a live-game FPS result. Experimental evidence and immutable signed-candidate manifests are kept under `output/de10/screamer2-17fps/`.

Quartus requires `nix develop --command devenv shell -- ...`. Every complete compile plus required timing signoff is bounded to 900 seconds; a candidate must pass the 50 MHz all-corner and vendor-DDR checks before board programming. Board replay uses `--fb-base 0x3f400000`; RAM-only programming must preserve the protected runtime and persistent firmware policy.
