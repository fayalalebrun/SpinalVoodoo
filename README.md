# SpinalVoodoo

SpinalHDL implementation of the 3dfx Voodoo Graphics GPU.

## Implementation Status

### FBI (Frame Buffer Interface)

- Rasterization
  - [x] Triangle setup (edge walking)
  - [x] Span generation
  - [x] Clipping (clipLeftRight, clipLowYHighY)
  - [ ] Stipple patterns
- Gradient Interpolation
  - [x] Color (R, G, B, A) - 12.12 fixed-point
  - [x] Depth (Z) - 20.12 fixed-point
  - [x] Texture coordinates (S, T, W) - 14.18 fixed-point
  - [ ] Parameter adjustment for sub-pixel vertices (fbzMode bit 26)
- Color Combine Unit
  - [x] c_other source selection (iterated, texture, color1)
  - [x] c_local source selection (iterated, color0)
  - [x] Basic multiply/add operations
  - [ ] Full mselect modes (texture alpha, etc.)
  - [ ] cc_localselect_override (texture alpha selects local)
  - [ ] Alpha combine unit
- Fog
  - [ ] W-based fog table lookup
  - [ ] Iterated fog (alpha/Z based)
  - [ ] Fog color blending
- Alpha Test
  - [ ] Comparison functions (never, <, ==, <=, >, !=, >=, always)
  - [ ] Reference alpha
- Depth Buffer
  - [ ] Z-buffer read/compare
  - [ ] Depth function selection
  - [ ] Z-buffer write
  - [ ] W-buffer mode
  - [ ] Depth bias
- Alpha Blending
  - [ ] Source blend factors
  - [ ] Destination blend factors
  - [ ] Framebuffer read for blending
- Chroma Key
  - [ ] Color key comparison
- Dithering
  - [ ] 4x4 ordered dither
  - [ ] 2x2 ordered dither
- Framebuffer Write
  - [x] 16-bit RGB565 output
  - [x] Draw buffer selection
  - [ ] RGB write mask
- Linear Frame Buffer (LFB)
  - [ ] Direct CPU writes
  - [ ] Pixel format conversion
  - [ ] Pipeline routing option
- Commands
  - [x] triangleCMD / ftriangleCMD
  - [ ] fastfillCMD
  - [x] nopCMD
  - [ ] swapbufferCMD

### TMU (Texture Mapping Unit)

- Texture Coordinates
  - [x] S/T/W interpolation
  - [x] Perspective correction (S/W, T/W)
- LOD (Level of Detail)
  - [x] LOD calculation from gradients
  - [x] Mipmap base offset calculation
  - [ ] LOD bias
  - [ ] LOD clamping (lodmin, lodmax)
  - [ ] Trilinear blending (lod_frac)
- Texture Address
  - [x] texBaseAddr register
  - [x] Texel address generation
  - [ ] texBaseAddr_1/2/3_8 (per-LOD base addresses)
  - [ ] Clamp/wrap modes
- Texture Filtering
  - [x] Point sampling (nearest)
  - [ ] Bilinear filtering
- Texture Formats
  - [x] ARGB4444
  - [ ] ARGB1555
  - [ ] RGB565
  - [ ] AI88 (alpha + intensity)
  - [ ] YIQ (NCC compressed)
  - [ ] 8-bit palettized
- Texture Combine
  - [x] Basic texture output
  - [ ] tc_mselect modes
  - [ ] Multi-texture (TMU chaining)

## Scala CLI Commands

```bash
# Run tests
scala-cli test .

# Format code
scalafmt

# Compile
scala-cli compile .
```

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
