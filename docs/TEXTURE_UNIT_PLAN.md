# Texture Unit Implementation Plan for Voodoo 1

## Current State
The codebase has the basic triangle pipeline implemented:
- **TriangleSetup**: Edge equation computation, bounding box calculation
- **Rasterizer**: Serpentine scan with S, T, W, RGB, Z, A interpolation
- **RegisterBank**: Contains startS/T/W and gradient registers, but no texture configuration
- **Write**: Framebuffer output stage

## What the Texture Unit Needs to Support

### Texture Formats (16 formats)
- **8-bit**: RGB332, YIQ422, Alpha, Intensity, Alpha-Intensity44, Palette
- **16-bit**: ARGB8332, AYIQ8422, RGB565, ARGB1555, ARGB4444, Alpha-Intensity88, Alpha-Palette

### Filtering Modes
- Point sampling (nearest neighbor)
- Bilinear filtering (4 texel blend)
- Trilinear filtering (8 texel blend across 2 LOD levels - requires dual TMU)

### Address Modes
- Wrap (default)
- Clamp
- Mirror (tLOD bits 28-29)

### Special Features
- Perspective correction (S/W, T/W division per pixel)
- LOD (Level of Detail) calculation from texture gradients
- NCC (Narrow Channel Compression) decompression for YIQ textures
- 256-entry palette lookup
- Texture combine unit (like FBI's color combine)

---

## Implementation Steps

### Phase 1: Infrastructure & Registers
1. **Add texture configuration registers to RegisterBank**
   - textureMode (0x300): Format, filtering, clamp/wrap, combine bits
   - tLOD (0x304): LOD min/max/bias, aspect ratio, mirror modes
   - tDetail (0x308): Detail texture parameters
   - texBaseAddr[0-3] (0x30C-0x318): Texture memory base addresses for LOD levels
   - trexInit0/1 (0x31C-0x320): TMU initialization
   - nccTable0/1 (0x324-0x380): 12 entries each for YIQ decompression

2. **Add texture memory interface**
   - Separate 8MB address space (0x800000-0xFFFFFF in CPU space)
   - 4MB per TMU for Voodoo 1
   - BMB slave interface for texture uploads from CPU

### Phase 2: Core Texture Pipeline
3. **Create Perspective Divider component**
   - Input: interpolated S, T, W from rasterizer
   - Output: S/W, T/W (perspective-corrected texture coordinates)
   - Use iterative divider or reciprocal lookup table
   - This is the most compute-intensive part

4. **Create LOD Calculator component**
   - Input: dSdX, dTdX, dSdY, dTdY, tLOD register
   - Output: LOD value (4.2 fixed-point) clamped to [lodmin, lodmax]
   - Formula: LOD = log2(max(|dU/dx|, |dV/dy|)) + lodbias
   - Can use leading-zero count for approximate log2

5. **Create Texture Address Generator**
   - Input: S/W, T/W (after perspective), LOD, tLOD register, texBaseAddr
   - Output: Texture memory address(es) for fetch
   - Handle: wrap/clamp/mirror, mipmap level selection, aspect ratio

### Phase 3: Memory & Fetch
6. **Create Texture Memory component**
   - On-chip SRAM model for simulation
   - CPU write path for texture uploads
   - Multi-port read for bilinear (4 texels) or trilinear (8 texels)

7. **Create Texture Fetch component**
   - Input: texture address(es), textureMode
   - Output: Up to 4 texels (for bilinear)
   - Pipeline texture memory reads

### Phase 4: Format Decode & Filtering
8. **Create Texture Format Decoder**
   - Input: Raw texel data, format from textureMode
   - Output: 8-bit ARGB per texel
   - Handle all 16 formats (RGB expansion, intensity replication)
   - Defer NCC/palette lookup to separate step

9. **Create NCC Decompressor** (for YIQ formats)
   - Input: 8-bit YIQ index, nccTable selection
   - Output: 8-bit RGB
   - Y lookup + I*R,G,B + Q*R,G,B matrix multiply

10. **Create Palette Lookup** (for palette formats)
    - Input: 8-bit palette index
    - Output: 8-bit RGB from 256-entry palette RAM
    - Palette loaded via nccTable I/Q register writes with bit 31 set

11. **Create Bilinear Filter**
    - Input: 4 decoded ARGB texels, S/T fractional bits
    - Output: Filtered ARGB
    - Weighted average based on fractional position

### Phase 5: Integration
12. **Create Texture Combine Unit (TCU)**
    - Input: Filtered texture ARGB, previous stage output (for multi-TMU)
    - Output: Combined ARGB
    - Programmable combine logic from textureMode bits 12-29
    - Same structure as FBI's color combine unit

13. **Create TMU (Texture Mapping Unit) top-level**
    - Instantiate and connect all texture sub-components
    - Pipeline with appropriate buffering
    - Stream interface: Input from rasterizer, Output to FBI color combine

14. **Integrate TMU into Core**
    - Insert TMU between Rasterizer and Write stages
    - Route textureMode bit 27 of fbzColorPath to bypass TMU when disabled
    - Connect texture coordinate registers (startS/T/W, dS/T/WdX/Y)

### Phase 6: Advanced Features (Optional/Later)
15. **Add detail texture support**
    - Second texture modulates the base texture
    - Controlled by tDetail register

16. **Add projected texture support**
    - For spotlight effects
    - Uses W coordinate specially

17. **Add trilinear filtering support**
    - Requires second TMU or multi-pass
    - Blends between two LOD levels

---

## Key Data Formats

| Parameter | Format | Notes |
|-----------|--------|-------|
| S, T coords | SQ(32, 18) | 14.18 fixed-point |
| W value | SQ(32, 30) | 2.30 fixed-point |
| LOD | UQ(4, 2) | 4.2 unsigned |
| Texel color | 8-bit per channel | After decode |
| Texture address | 19-bit | 8-byte granularity |

## File Structure
```
src/
  texture/
    TextureUnit.scala          # Top-level TMU component
    PerspectiveDivider.scala   # S/W, T/W computation
    LodCalculator.scala        # LOD calculation
    TexAddressGen.scala        # Address generation
    TextureMemory.scala        # Texture RAM
    TexFormatDecode.scala      # Format decoding
    NccDecompress.scala        # YIQ decompression
    BilinearFilter.scala       # Texture filtering
    TextureCombine.scala       # TCU
```

---

## Recommended Implementation Order

1. **Start with point-sampled textures only** (simplest path)
   - Registers, perspective division, address gen, memory, format decode
   - Skip bilinear, LOD, NCC initially

2. **Add bilinear filtering**
   - 4-texel fetch, weighted blend

3. **Add LOD/mipmapping**
   - LOD calculation, mipmap address selection

4. **Add NCC/palette support**
   - For compressed texture formats

5. **Add trilinear** (if dual-TMU support desired)

---

## Register Details

### textureMode (0x300) - 31 bits
```
Bit 0:     tpersp_st      - Enable perspective correction (0=linear, 1=perspective)
Bit 1:     tminfilter     - Minification filter (0=point, 1=bilinear)
Bit 2:     tmagfilter     - Magnification filter (0=point, 1=bilinear)
Bit 3:     tclampw        - Clamp when W negative (force S=T=0)
Bit 4:     tloddither     - Enable LOD dithering
Bit 5:     tnccselect     - NCC table select (0=table0, 1=table1)
Bit 6:     tclamps        - Clamp S coordinate (0=wrap, 1=clamp)
Bit 7:     tclampt        - Clamp T coordinate (0=wrap, 1=clamp)
Bits 11:8: tformat        - Texture format (0-15, see format table)
Bits 20:12: tc_*          - Texture color combine control
Bits 29:21: tca_*         - Texture alpha combine control
Bit 30:    trilinear      - Enable trilinear filtering
Bit 31:    seq_8_downld   - Sequential 8-bit download mode
```

### tLOD (0x304) - 27 bits
```
Bits 5:0:   lodmin         - Minimum LOD (4.2 unsigned)
Bits 11:6:  lodmax         - Maximum LOD (4.2 unsigned)
Bits 17:12: lodbias        - LOD bias (4.2 signed)
Bit 18:     lod_odd        - LOD odd (for split textures)
Bit 19:     lod_split      - Texture is split across TMUs
Bit 20:     lod_s_is_wider - S dimension is wider (for non-square)
Bits 22:21: lod_aspect     - Aspect ratio (0=1:1, 1=2:1, 2=4:1, 3=8:1)
Bit 23:     lod_zerofrac   - Force LOD fraction to 0
Bit 24:     tmultibaseaddr - Use multiple base addresses
Bit 25:     tdata_swizzle  - Byte swap texture data
Bit 26:     tdata_swap     - Word swap texture data
```

### Texture Format Table
```
0:  8-bit RGB (3-3-2)
1:  8-bit YIQ (4-2-2) - NCC compressed
2:  8-bit Alpha
3:  8-bit Intensity
4:  8-bit Alpha-Intensity (4-4)
5:  8-bit Palette
6-7: Reserved
8:  16-bit ARGB (8-3-3-2)
9:  16-bit AYIQ (8-4-2-2) - NCC compressed with alpha
10: 16-bit RGB (5-6-5)
11: 16-bit ARGB (1-5-5-5)
12: 16-bit ARGB (4-4-4-4)
13: 16-bit Alpha-Intensity (8-8)
14: 16-bit Alpha-Palette (8-8)
15: Reserved
```

---

## Pipeline Diagram

```
                    ┌─────────────────┐
                    │   Rasterizer    │
                    │  (S, T, W, RGB) │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   Perspective   │
                    │    Divider      │
                    │  S/W, T/W calc  │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
        ┌──────────┐  ┌──────────┐  ┌──────────┐
        │   LOD    │  │  Address │  │  Address │
        │  Calc    │  │  Gen (0) │  │  Gen (1) │
        └────┬─────┘  └────┬─────┘  └────┬─────┘
             │             │              │
             │             ▼              ▼
             │       ┌──────────┐  ┌──────────┐
             │       │  Texel   │  │  Texel   │
             │       │ Fetch(0) │  │ Fetch(1) │
             │       └────┬─────┘  └────┬─────┘
             │             │              │
             │             ▼              ▼
             │       ┌──────────┐  ┌──────────┐
             │       │  Format  │  │  Format  │
             │       │  Decode  │  │  Decode  │
             │       └────┬─────┘  └────┬─────┘
             │             │              │
             │             └──────┬───────┘
             │                    │
             │                    ▼
             │            ┌──────────────┐
             │            │   Bilinear   │
             │            │    Filter    │
             └───────────►│  (4 texels)  │
                          └──────┬───────┘
                                 │
                                 ▼
                          ┌──────────────┐
                          │   Texture    │
                          │   Combine    │
                          │    Unit      │
                          └──────┬───────┘
                                 │
                                 ▼
                          ┌──────────────┐
                          │    Color     │
                          │   Combine    │
                          │   (FBI)      │
                          └──────┬───────┘
                                 │
                                 ▼
                          ┌──────────────┐
                          │    Write     │
                          │   Stage      │
                          └──────────────┘
```
