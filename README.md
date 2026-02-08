# SpinalVoodoo

SpinalHDL implementation of the 3dfx Voodoo Graphics GPU.

## Implementation Status

### FBI (Frame Buffer Interface)

- Rasterization
  - [x] Triangle setup (bounding-box scan + edge function testing)
  - [x] Span generation (serpentine scan)
  - [x] Clipping (clipLeftRight, clipLowYHighY)
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
  - [x] Depth/alpha planes selection (fbzMode bit 18)
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
  - [ ] Perspective correction (S/W, T/W division) — parsed but not yet implemented
- LOD (Level of Detail)
  - [x] LOD calculation from gradients (max gradient MSB position)
  - [x] Mipmap base offset calculation (per-LOD cumulative sizes)
  - [x] LOD bias (tLOD bits 17:12)
  - [x] LOD clamping (lodmin, lodmax)
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
  - [ ] Bilinear filtering (min/mag)
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
  - [ ] YIQ422 / AYIQ8422 (NCC compressed)
  - [ ] P8 / AP88 (palettized)
- Texture Combine
  - [x] Texture output to color combine unit
  - [ ] Multi-texture (TMU chaining)
  - [ ] NCC table decode
  - [ ] LOD dither
  - [ ] Data swizzle/swap

### Integration Tests

- [x] Flat-shaded triangle (constant color, 9520 pixels)
- [x] Gouraud-shaded triangle (color gradients, 9520 pixels)
- [x] Textured triangle (ARGB4444, point sample, clamp, non-perspective, LOD 0, 240 pixels)

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
