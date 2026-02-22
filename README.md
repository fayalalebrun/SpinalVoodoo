# SpinalVoodoo

SpinalHDL implementation of the 3dfx Voodoo Graphics GPU.

## Implementation Status

### FBI (Frame Buffer Interface)

- Rasterization
  - [x] Triangle setup (bounding-box scan + edge function testing)
  - [x] Span generation (serpentine scan)
  - [x] Scissor clipping (clipLeftRight, clipLowYHighY)
  - [x] Y-origin transform (fbzMode bit 17)
  - [ ] Stipple patterns
- Gradient Interpolation
  - [x] Color (R, G, B, A) - 12.12 fixed-point
  - [x] Depth (Z) - 20.12 fixed-point
  - [x] Texture coordinates (S, T, W) - 14.18 / 2.30 fixed-point
  - [ ] Parameter adjustment for sub-pixel vertices (fbzMode bit 26)
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
  - [x] Color key comparison
- Dithering
  - [x] 4x4 ordered dither
  - [x] 2x2 ordered dither
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

## Trace Player

Replay Voodoo trace files and watch triangles render in real-time:

```bash
scala-cli run . -- \
  --trace /path/to/voodoo_trace.bin \
  [--index /path/to/voodoo_trace.bin.idx] \
  [--frame N] \
  [--timing accurate|freerun] \
  [--resolution WxH]
```

**Options:**
- `--trace` - Binary trace file (required)
- `--index` - Frame index file (auto-detected if `.bin.idx` exists)
- `--frame` - Render specific frame number (requires index)
- `--timing` - `accurate` for cycle-accurate, `freerun` for max speed (default: freerun)
- `--resolution` - Display resolution (default: 640x480)
