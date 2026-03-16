# SpinalVoodoo

SpinalHDL implementation of the 3dfx Voodoo Graphics GPU.

## Simulation Gallery

<table>
  <tr>
    <td align="center"><img src="docs/readme-assets/logo.png" alt="3dfx logo trace" width="320"><br><sub>Logo</sub></td>
    <td align="center"><img src="docs/readme-assets/screamer2.png" alt="Screamer 2 trace" width="320"><br><sub>Screamer 2</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/readme-assets/quake.png" alt="Quake trace" width="320"><br><sub>Quake</sub></td>
    <td align="center"><img src="docs/readme-assets/valley-of-ra.png" alt="Valley of Ra trace" width="320"><br><sub>Valley of Ra</sub></td>
  </tr>
</table>

## Implementation Status

### FBI (Frame Buffer Interface)

- Rasterization
  - [x] Triangle setup (bounding-box scan + edge function testing)
  - [x] Span generation (serpentine scan)
  - [x] Scissor clipping (clipLeftRight, clipLowYHighY)
  - [x] Y-origin transform (fbzMode bit 17, fbiInit3[31:22])
  - [ ] Stipple patterns
- Gradient Interpolation
  - [x] Color (R, G, B, A) - 12.12 fixed-point
  - [x] Depth (Z) - 20.12 fixed-point
  - [x] Texture coordinates (S, T, W) - 14.18 / 2.30 fixed-point
  - [x] Parameter adjustment for sub-pixel vertices (fbzColorPath bit 26)
- Color Combine Unit (CCU)
  - [x] c_other source selection (iterated, texture, color1, LFB)
  - [x] c_local source selection (iterated, color0)
  - [x] cc_localselect_override (texture alpha bit 7 selects color0)
  - [x] zero_other, sub_clocal
  - [x] All mselect modes (ZERO, CLOCAL, AOTHER, ALOCAL, TEXTURE_ALPHA, TEXTURE_RGB)
  - [x] reverse_blend (factor inversion)
  - [x] Add modes (NONE, CLOCAL, ALOCAL)
  - [x] Invert output
- Alpha Combine Unit (ACU)
  - [x] a_other source selection (iterated, texture, color1, LFB)
  - [x] a_local source selection (iterated, color0, iterated_z)
  - [x] alpha_zero_other, alpha_sub_clocal
  - [x] All alpha mselect modes
  - [x] alpha_reverse_blend, alpha_add, alpha_invert_output
- Fog
  - [x] W-based fog table lookup
  - [x] Iterated fog (alpha/Z based)
  - [x] Fog color blending
- Alpha Test
  - [x] Comparison functions (never, <, ==, <=, >, !=, >=, always)
  - [x] Reference alpha
- Depth Buffer
  - [x] Z-buffer read/compare
  - [x] Depth function selection
  - [x] Z-buffer write
  - [x] W-buffer mode
  - [x] Depth bias
  - [x] Depth source select (iterated Z or W)
- Alpha Blending
  - [x] Source blend factors (10 modes)
  - [x] Destination blend factors (10 modes)
  - [x] Framebuffer read for blending (fork-queue-join BMB pattern)
- Chroma Key
  - [x] Color key comparison (post color-combine, matching real hardware)
  - [ ] Chroma range
- Dithering
  - [x] 4x4 ordered dither (86Box ground-truth LUTs)
  - [x] 2x2 ordered dither (86Box ground-truth LUTs)
- Framebuffer Write
  - [x] 16-bit RGB565 output
  - [x] Draw buffer selection
  - [x] Depth/alpha planes selection (fbzMode bit 18)
  - [x] RGB write mask
  - [x] Aux write mask
- Linear Frame Buffer (LFB)
  - [x] Direct CPU writes (bypass mode, all write formats)
  - [x] Pixel format conversion (RGB565, RGB555, ARGB1555, XRGB8888, ARGB8888, Depth+color, Depth-only)
  - [x] Dual-pixel (16-bit formats) and single-pixel (32-bit formats) modes
  - [x] RGBA lane swizzle (ARGB, ABGR, RGBA, BGRA)
  - [x] Word swap and byte swizzle
  - [x] Dithering support
  - [x] Pipeline routing option (pixelPipelineEnable=1)
  - [x] LFB reads
- Commands
  - [x] triangleCMD / ftriangleCMD
  - [x] fastfillCMD (screen clear via clip rectangle, color1/zaColor, with dithering)
  - [x] nopCMD
  - [x] swapbufferCMD (immediate + vsync-synchronized, swapsPending tracking)

### TMU (Texture Mapping Unit)

- Texture Coordinates
  - [x] S/T/W interpolation
  - [x] Perspective correction (S/W, T/W division via 257-entry reciprocal LUT)
- LOD (Level of Detail)
  - [x] LOD calculation from gradients (max gradient MSB position)
  - [x] Mipmap base offset calculation (per-LOD cumulative sizes)
  - [x] LOD bias (tLOD bits 17:12)
  - [x] LOD clamping (lodmin, lodmax)
  - [x] Per-pixel LOD adjustment (perspective-corrected W)
  - [x] Aspect ratio / non-square textures (tLOD lodAspect + lodSIsWider)
  - [ ] Trilinear blending (lod_frac)
- Texture Address
  - [x] texBaseAddr register
  - [x] Texel address generation (row-major, 8/16-bit stride)
  - [x] Clamp and wrap modes (per-axis, textureMode bits 6-7)
  - [x] Clamp W (negative W forces S=T=0, textureMode bit 3)
  - [ ] texBaseAddr_1/2/3_8 (per-LOD base addresses)
- Texture Filtering
  - [x] Point sampling (nearest)
  - [x] Bilinear filtering (min/mag)
- Texture Formats (decode logic)
  - [x] RGB332 (8-bit)
  - [x] A8 (8-bit, alpha only)
  - [x] I8 (8-bit, intensity)
  - [x] AI44 (8-bit, alpha + intensity)
  - [x] ARGB8332 (16-bit)
  - [x] RGB565 (16-bit)
  - [x] ARGB1555 (16-bit)
  - [x] ARGB4444 (16-bit)
  - [x] AI88 (16-bit, alpha + intensity)
  - [x] YIQ422 / AYIQ8422 (NCC compressed)
  - [x] P8 / AP88 (palettized)
- Texture Combine
  - [x] Texture output to color combine unit
  - [ ] Multi-texture (TMU chaining)
  - [x] NCC table decode
  - [x] Palette RAM (256-entry, loaded via NCC table 0 I/Q registers)
  - [ ] LOD dither
  - [ ] Data swizzle/swap

### Bus Interface / Register System

- [x] BMB bus adapter (BmbBusInterface)
- [x] 64-entry PCI FIFO with categorized routing (fifoBypass / syncRequired)
- [x] Pipeline drain blocking for sync registers (triangleCMD, swapbufferCMD, etc.)
- [x] Address remapping (fbiInit3 bit 0, external bit 21)
- [x] CPU bus address decode (PCI BAR regions: registers, LFB, texture)
- [x] texBaseAddr write relocation (SST-1 spec 5.53)
- [ ] PCI configuration space (initEnable, busSnoop)
- [ ] Memory FIFO (off-screen framebuffer extension)

### Display Controller

- [ ] Video timing generator (hSync, vSync, backPorch, videoDimensions)
- [ ] Framebuffer scan-out
- [ ] Gamma correction CLUT (clutData)
- [ ] DAC programming (dacData)
- vRetrace is currently an external input (no internal generation)

## Scala CLI Commands

```bash
# Run tests
scala-cli test .

# Format code
scalafmt

# Compile
scala-cli compile .
```

## Glide Simulation Tests

Run the Glide2x test suite against the Verilator model:

```bash
make run/test00          # single test
make run-all             # all tests
TRACE=1 make run/test00  # with FST waveform dump
```

Screenshots are saved to `output/<test>/screenshot.png`.

**Environment variables** (set at runtime):

| Variable | Description |
|---|---|
| `SIM_FST` | Path for FST waveform output (requires `TRACE=1` build) |
| `SIM_CYCLE_LIMIT` | Max simulation ticks before clean exit (0 = unlimited) |
| `SIM_FBWRITE_LOG` | Path to log framebuffer writes |
| `SIM_TMU_LOG` | Path to log TMU/rasterizer activity |

## DOSBox-X (32-bit Glide Path)

The sim backend builds a **32-bit** `libglide2x`, so DOSBox-X must also be
32-bit to `dlopen()` it.

```bash
# Build sim + Glide backend
make glide

# Build DOS guest binaries that use the original 3dfx DOS SDK import libs
make dos/sdk

# Build one DOS guest binary
make dos/sdk/df00sdk

# Run one DOS guest binary inside DOSBox-X through its built-in GLIDE2X.OVL
make dos/run/df00sdk

# Headless variant
make dos/run/df00sdk-headless

# Launch a raw DOSBox-X session with this repo's Glide library injected
make dosbox
```

Common top-level commands:

- `make dos/help` prints the DOS and Tomb entry points.
- `make dos/sdk` builds all DOS SDK-style test binaries from `emu/glide/glide2x/sst1/glide/tests/Makefile.sdkwat`.
- `make dos/sdk/<name>` builds one DOS SDK-style binary, for example `make dos/sdk/df00sdk`.
- `make dos/run/<name>` mounts `emu/glide/glide2x/sst1/glide/tests` as `C:` in DOSBox-X and runs `<name>.exe`.
- `make dos/run/<name>-headless` does the same with `DOSBOXX32_HEADLESS=1`.
- `make dosbox ARGS='...'` launches DOSBox-X with the repo Glide backend and forwards extra DOSBox-X arguments.
- `make tomb/help` prints the Tomb Raider setup requirements.
- `make tomb/prepare ARGS='--game-dir ... --patch ... --iso ...'` prepares a reusable Tomb Raider source tree from your game files and 3dfx patch assets.
- `make tomb/run`, `make tomb/headless`, and `make tomb/screenshot` expose the existing Tomb helper scripts through the top-level `Makefile`.

The split is intentional:

- `make run/<name>` runs a Linux-hosted test binary linked directly against the sim backend.
- `make dos/run/<name>` runs a DOS guest binary inside DOSBox-X through its built-in `GLIDE2X.OVL` bridge.

### Tomb Raider Setup

The Tomb helper scripts expect a prepared source tree at `DOSBOX_TOMB_SRC`.
By default that is `/tmp/tr1-3dfx`.

The easiest way to create it is:

```bash
make tomb/prepare ARGS='--game-dir /path/to/TOMBRAID --patch /path/to/3dfx-patch.zip --iso /path/to/tr1disc01.iso'
```

That helper:

- copies your source game directory into `DOSBOX_TOMB_SRC/TOMBRAID`
- copies the ISO into `DOSBOX_TOMB_SRC/tr1disc01.iso` if provided
- overlays the supplied 3dfx patch directory or archive onto the game directory

It accepts either a patch directory or a `.zip`/tar archive. Add `--output /path/to/tree`
to prepare somewhere other than `DOSBOX_TOMB_SRC`, and `--force` to replace an existing tree.

Expected layout by default:

```text
/tmp/tr1-3dfx/
  tr1disc01.iso
  TOMBRAID/
    TOMB.EXE
    ... game files ...
```

Typical flow:

- Prepare the tree with `make tomb/prepare ...`, or assemble the layout manually.
- Run `make tomb/help` to print the current assumptions.
- Run `make tomb/run` or `make tomb/headless`.

At launch time the helper scripts copy the source tree into `DOSBOX_TOMB_STAGE_ROOT`
(default `/tmp/tr1-run`) and automatically rename any bundled `glide2x.ovl` so that
DOSBox-X's built-in `GLIDE2X.OVL` guest bridge is used instead.

You can override the defaults with these environment variables:

- `DOSBOX_TOMB_SRC`
- `DOSBOX_TOMB_STAGE_ROOT`
- `DOSBOX_TOMB_GAME_DIR`
- `DOSBOX_TOMB_ISO`
- `DOSBOX_TOMB_EXE`

Notes:

- `scripts/run-dosboxx32-glide` prefers `DOSBOXX32_BIN` if set.
- If `dosbox-x` on `PATH` is already 32-bit, it uses that.
- Otherwise it auto-resolves Nix `pkgsi686Linux.dosbox-x`.
- The script appends `scripts/dosboxx32-glide.conf` (enables `glide=true`, `voodoo_card=false`, `lfb=full_noaux`).
- For low-level manual control you can still use `scripts/run-dosboxx32-glide` directly.

## Trace-Based Testing

Glide trace files (`.bin`) capture PCI bus operations for offline replay against both
the RTL simulation and a software reference model:

```bash
# Capture a trace (rebuilds Glide with trace harness)
make trace/test_alphabet

# Replay a trace through the reference model and RTL simulator
make check/test_alphabet

# Capture and replay in one step
make test/test_alphabet

# Replay all existing traces under traces/
make check-all
```

Replay outputs are written to `test-output/<trace>/` as `<trace>_ref.png`, `<trace>_sim.png`, and `<trace>_diff.png`.

Trace files are stored in `traces/` and compared pixel-by-pixel against a reference model.

## DE10-Nano Bring-Up Flow (Scaffold)

Goal-oriented planning docs:

- `docs/DE10_MILESTONES.md`
- `docs/DE10_BRINGUP_PLAN.md`
- `docs/DE10_DEPLOYMENT.md`

Entry points:

```bash
make de10/help
make de10/plan

# Generate DE10-targeted RTL
make de10/rtl

# Generate DE10 Platform Designer system collateral
make de10/qsys

# Build bitstream (requires Quartus in PATH)
make de10/bitstream

# Program/deploy to a network-accessible board
make de10/program ARGS="--host <de10-host> --user <user>"
make de10/deploy  ARGS="--host <de10-host> --user <user>"

# Build MMIO smoke utility for board-side validation
make de10/mmio-smoke
```

Current scope of this scaffold:

- Establishes scripted DE10 build/program/deploy interfaces.
- Adds DE10 top-level RTL generator entry point (`voodoo.de10.De10TopGen`).
- Adds Platform Designer generation for HPS bridge integration (`scripts/gen-qsys-de10`).
- Adds a board MMIO smoke utility (`tools/de10-mmio-smoke.c`).

The FPGA wrapper and memory integration are initial bring-up placeholders and
are expected to evolve as full HPS hard-IP/Platform Designer integration is
finalized.
