# Voodoo Rendering Pipeline: Register Flow

## **1. Geometry Setup Phase**

### Vertex Input (0x008-0x080 or 0x088-0x100)
- **vertexAx/Ay, vertexBx/By, vertexCx/Cy** (0x008-0x01c): Triangle vertex coordinates (12.4 fixed-point)
- **fvertexAx/Ay, etc.** (0x088-0x09c): Float versions (converted to 12.4 internally)
- **Affects**: Triangle rasterization starting positions

**Coordinate System:**
- Vertices have **sub-pixel precision** (1/16th pixel)
- Rasterization samples pixels at **integer coordinates** (top-left corner)
- Vertex coordinates are rounded to nearest integer: `(vertex + 7) >> 4`

### Interpolation Parameters (0x020-0x07c or 0x0a0-0x0fc)
- **startR/G/B/A/Z** (0x020-0x030): Starting color & depth values at vertex A
- **startS/T/W** (0x034-0x03c): Starting texture coordinates & perspective
- **dRdX, dGdX, dBdX, dAdX, dZdX** (0x040-0x050): Color/depth gradients in X
- **dSdX, dTdX, dWdX** (0x054-0x05c): Texture gradients in X
- **dRdY, dGdY, dBdY, dAdY, dZdY** (0x060-0x070): Color/depth gradients in Y
- **dSdY, dTdY, dWdY** (0x074-0x07c): Texture gradients in Y
- **Affects**: Per-pixel interpolation during rasterization

**FBZ_PARAM_ADJUST (fbzMode bit 26):**
When enabled, adjusts interpolation starting values to account for sub-pixel vertex positions:
```c
base_r += (dx * dRdX + dy * dRdY) >> 4  // dx, dy = vertex fractional parts
```
This ensures correct interpolation when vertices have sub-pixel coordinates, compensating for the fact that rasterization snaps to integer pixel positions.

### Trigger Rendering
- **triangleCMD/ftriangleCMD** (0x080/0x100): Starts triangle rasterization with all setup parameters

---

## **2. Rasterization Phase**

### Clipping (0x118-0x11c)
- **clipLeftRight** (0x118): X min/max bounds
- **clipLowYHighY** (0x11c): Y min/max bounds
- **clipLeftRight1/clipTopBottom1** [Banshee+] (0x200/0x204): Secondary clip region
- **Affects**: Determines which pixels to process

### Stippling (0x140)
- **stipple** (0x140): 32-bit pattern for stipple masking
- **Affects**: Per-pixel enable mask during rasterization

---

## **3. Texture Sampling Phase** (TREX chips)

### TMU Selection via Address Bits [13:10]
Triangle texture parameters (startS/T/W, dSdX/dTdX/dWdX, dSdY/dTdY/dWdY) are **per-TMU**:
- **Bit 11 set (0x0800)**: Write to TMU0 (CHIP_TREX0)
- **Bit 12 set (0x1000)**: Write to TMU1 (CHIP_TREX1)
- **Both bits set**: Write to both TMUs with same value
- **Affects**: Each TMU can have independent texture coordinates on the same triangle

### Texture Configuration (0x300-0x318) - Per-TMU
Selected by chip bits [13:10]:
- **textureMode** (0x300): Format, filtering, clamp/wrap modes, **and combine operations**
- **tLOD** (0x304): LOD selection, mipmapping control
- **tDetail** (0x308): Detail texture scale/bias
- **texBaseAddr/1/2/38** (0x30c-0x318): Memory addresses for each LOD level
- **Affects**: Which texture data is fetched and how it's filtered

### Texture Data Tables (0x324-0x380) - Per-TMU
- **nccTable0/1_Y/I/Q** (0x324-0x380): YIQ color compression tables
- **Affects**: Color decompression for NCC texture formats
- **Also**: Palette data writes (bit 31 set)

### Multi-Texture Pipeline Order

**Three operating modes:**

**1. Single TMU Mode** (`textureMode[0] & TEXTUREMODE_LOCAL` or single TMU):
- Only TMU0 sampled
- Output goes directly to FBI color combiner

**2. Pass-Through Mode** (`textureMode[0] & TEXTUREMODE_PASSTHROUGH`):
- TMU0 bypassed
- Only TMU1 sampled
- TMU1 result used as texture input to FBI

**3. Multi-Texture Mode** (dual TMUs, neither LOCAL nor PASSTHROUGH):
- **TMU1 sampled first** → internal combine using textureMode[1]
- **TMU0 sampled second** → combines with TMU1 result using textureMode[0]
- **Final result** → to FBI color combiner

**Pipeline flow: TMU1 → TMU0 → FBI**

### Texture Combine Operations (textureMode bits 12-29)

**Per-TMU combine equation** (separate for RGB and Alpha):

```
// Color combine (bits 12-20):
other = TC_ZERO_OTHER ? 0 : input_from_previous_stage
if (TC_SUB_CLOCAL) other -= texture_color

factor = TC_MSELECT {
  0: ZERO
  1: CLOCAL (texture color)
  2: AOTHER (previous stage alpha)
  3: ALOCAL (texture alpha)
  4: DETAIL (detail texture modulation)
  5: LOD_FRAC (for trilinear filtering)
}

if (TC_REVERSE_BLEND) factor = 255 - factor

result = other * factor / 255

if (TC_ADD_CLOCAL) result += texture_color
else if (TC_ADD_ALOCAL) result += texture_alpha

if (TC_INVERT_OUTPUT) result = 255 - result
```

**Common multi-texture uses:**
- **Detail texturing**: TMU1=base color, TMU0=detail (multiply)
- **Light mapping**: TMU1=diffuse, TMU0=lightmap (multiply)
- **Decals**: TMU1=surface, TMU0=decal (alpha blend)
- **Environment mapping**: TMU1=base, TMU0=reflection (add/blend)

**Affects**: How textures combine before reaching FBI color path

---

## **4. Color Combine Phase** (FBI chip)

### Color Path (0x104)
- **fbzColorPath** (0x104):
  - Bits [1:0]: rgb_sel - RGB "other" source selection
  - Bits [3:2]: a_sel - Alpha "other" source selection
  - Bit 4: cc_localselect - RGB "local" source (0=iterated, 1=color0)
  - Bits [6:5]: cca_localselect - Alpha "local" source
  - Bit 7: cc_localselect_override - Use texture alpha to select local
  - Bit 8: cc_zero_other - Force c_other to zero
  - Bit 9: cc_sub_clocal - Subtract c_local from c_other
  - Bits [12:10]: cc_mselect - Multiply factor selection
  - Bit 13: cc_reverse_blend - Invert multiply factor (1-x)
  - Bits [15:14]: cc_add - Addition control
  - Bit 16: cc_invert_output - XOR output with 0xFF
  - Bits [17:25]: Alpha combine unit controls (mirror of RGB controls)
  - Bit 26: FBZ_PARAM_ADJUST - Enable subpixel parameter correction
  - Bit 27: TEXTURE_ENABLE - Enable texture data from TREX
  - **Affects**: How interpolated color, texture, and constants combine

### Constant Colors (0x144-0x148)
- **color0** (0x144): Constant color source 0 (used as c_local option)
- **color1** (0x148): Constant color source 1 (used as c_other option)
- **Affects**: Available as color combine inputs

### Color Combine Unit (CCU) Pipeline Stages

**1. Input Selection**
- `c_other`: Selected by rgb_sel[1:0]:
  - 0: Iterated RGB (Gouraud shading)
  - 1: Texture RGB (from TREX)
  - 2: color1 RGB
  - 3: LFB RGB (framebuffer read)
- `c_local`: Selected by cc_localselect:
  - 0: Iterated RGB
  - 1: color0 RGB

**2. Zero Other** (cc_zero_other, bit 8)
- If set, forces c_other to 0

**3. Subtract Local** (cc_sub_clocal, bit 9)
- If set, computes `c_other - c_local`

**4. Multiply** (cc_mselect[2:0], bits 12:10)
- Multiplies result by selected factor:
  - 0: Zero
  - 1: c_local
  - 2: a_other (other alpha)
  - 3: a_local (local alpha)
  - 4: Texture alpha
  - 5: Texture RGB (as alpha)

**5. Reverse Blend** (cc_reverse_blend, bit 13)
- Inverts multiply factor: uses (255-factor) instead of factor

**6. Add** (cc_add[1:0], bits 15:14)
- 0: Add nothing
- 1: Add c_local
- 2: Add a_local (alpha broadcast to RGB)

**7. Clamp**
- Clamps result to 0-255

**8. Invert Output** (cc_invert_output, bit 16)
- XORs final result with 0xFF

### Color Combine Equation

```
result = clamp((c_other [- c_local]) * factor + add_term)
if (invert) result = ~result
```

### TEXTURE_ENABLE (bit 27)

Controls data flow from TREX to FBI:
- **0**: No texture data transferred; use Gouraud/flat shading only
- **1**: Texture data available as input to color combiner

**NOTE**: Must write nopCMD before changing this bit.

### Common Rendering Modes

| Mode | rgb_sel | cc_mselect | Description |
|------|---------|------------|-------------|
| Gouraud only | 0 | - | Iterated vertex colors |
| Texture only | 1 | 0 | Pure texture, no modulation |
| Texture * Gouraud | 1 | 1 | Texture modulated by vertex color |
| Texture + Gouraud | 1 | 0, cc_add=1 | Additive blend |

---

## **5. Fog Phase**

### Fog Configuration (0x108, 0x160-0x1dc)
- **fogMode** (0x108):
  - Bit 0: Fog enable
  - Bit 1: fogadd control (0=fogColor, 1=zero)
  - Bit 2: fogmult control (0=Color Combine Unit RGB, 1=zero)
  - Bit 3: fogalpha control (0=fog table alpha, 1=iterated alpha)
  - Bit 4: fogz control (0=fogalpha mux, 1=iterated z[27:20])
  - Bit 5: fogconstant control (0=fog multiplier output, 1=fogColor)
  - **Affects**: Whether/how fog is applied
- **fogTable00-1f** (0x160-0x1dc): 64-entry lookup table for fog density
- **fogColor** (0x12c): RGB fog color
- **Affects**: Final color after fog blend

### Fog Alpha Source Selection

The fog alpha (Afog) determines the blend factor between input color and fog color:

| Bits [4:3] | Source | Description |
|------------|--------|-------------|
| 00 | W-based table | Index fog table using 1/W, interpolate between entries |
| 01 | Iterated alpha | Use vertex alpha directly as fog factor |
| 10 | Iterated Z | Use upper 8 bits of Z as fog factor |
| 11 | Iterated Z | Z takes precedence over alpha |

**W-based fog table lookup**:
- Upper 6 bits of 1/W index the 64-entry fog table
- Lower 8 bits interpolate between adjacent entries (reduces banding)
- Each table entry contains: base fog value + delta for interpolation

### Fog Equations

| fogadd (bit 1) | fogmult (bit 2) | Equation | Use Case |
|----------------|-----------------|----------|----------|
| 0 | 0 | `Cout = Afog*Cfog + (1-Afog)*Cin` | Standard distance fog |
| 0 | 1 | `Cout = Afog*Cfog` | Fog color only |
| 1 | 0 | `Cout = (1-Afog)*Cin` | Fade to black |
| 1 | 1 | `Cout = 0` | Full blackout |

**Constant fog** (bit 5): Bypasses blend math, adds fogColor directly to input color.

### Fog Behavior

- Afog=0 (near camera): Output = input color (no fog)
- Afog=255 (far away): Output = fog color (full fog)
- Output is clamped to 0-255 after fog application

---

## **6. Alpha Test Phase**

### Alpha Testing (0x10c)
- **alphaMode** (0x10c):
  - Bits [3:1]: Comparison function (never, <, ==, <=, >, !=, >=, always)
  - Bits [31:24]: Reference alpha value
  - **Affects**: Pixel discard based on alpha
  - Updates **fbiAFuncFail** counter (0x158)

---

## **7. Depth Test Phase**

### Z-Buffer Testing (0x110)
- **fbzMode** (0x110):
  - Bit 4: Depth test enable
  - Bits [7:5]: Depth comparison function
  - Bit 10: Depth write enable
  - Bit 3: W-buffer mode (perspective-correct depth)
  - Bit 16: Depth bias enable
  - Bit 26: **FBZ_PARAM_ADJUST** - Adjust interpolation for sub-pixel vertices
  - **Affects**: Pixel discard based on depth
  - Updates **fbiZFuncFail** counter (0x154)

---

## **8. Alpha Blending Phase**

### Blending Configuration (0x10c)
- **alphaMode** (0x10c):
  - Bits [11:8]: Source blend factor
  - Bits [15:12]: Destination blend factor
  - **Affects**: How source and destination colors mix

---

## **9. Chroma Keying Phase**

### Chroma Key (0x110, 0x134)
- **fbzMode** bit 1: Chroma key enable
- **chromaKey** (0x134): RGB key color
- **Affects**: Pixel discard if color matches key
- Updates **fbiChromaFail** counter (0x150)

---

## **10. Dithering & Write Phase**

### Framebuffer Control (0x110, 0x1ec-0x1f8)
- **fbzMode** (0x110):
  - Bit 8: Dithering enable
  - Bit 11: 2x2 dither (vs 4x4)
  - Bit 19: Dither subtract mode
  - Bit 9: RGB write enable
  - Bits [15:14]: Draw to front/back buffer
  - **Affects**: Final pixel processing and buffer selection

- **colBufferAddr/Stride** [Banshee+] (0x1ec/0x1f0): Color buffer location
- **auxBufferAddr/Stride** [Banshee+] (0x1f4/0x1f8): Depth buffer location

### Statistics (0x14c-0x15c)
- **fbiPixelsIn** (0x14c): Pixels entering pipeline
- **fbiPixelsOut** (0x15c): Pixels written to framebuffer

---

## **11. Display & Swap**

### Buffer Management (0x128)
- **swapbufferCMD** (0x128): Swaps front/back buffers for double-buffering

---

## **Alternative Paths**

### Setup Mode [Banshee+] (0x260-0x2a4)
Higher-level vertex input that automatically calculates gradients:
- **sSetupMode** (0x260): Enables which vertex attributes to process
- **sVx/Vy, sRed/Green/Blue/Alpha, sVz, sW0/S0/T0, sW1/S1/T1** (0x264-0x29c)
- **sDrawTriCMD** (0x2a0): Triggers automatic gradient calculation + rasterization

### Fast Fill (0x124)
- **fastfillCMD** (0x124): Bypasses pipeline, fills buffer with **zaColor** (0x130)

### Linear Framebuffer (LFB) Writes (0x114, 0x400000-0x7FFFFF)
CPU direct memory access path:
- **lfbMode** (0x114): Configures pixel format, buffer select, and optional pipeline routing
- **Address range 0x400000-0x7FFFFF**: Maps to framebuffer with 1024-pixel stride
- **Pipeline routing** (lfbMode bit 8):
  - **0 (default)**: Direct framebuffer write, bypasses all pipeline stages
  - **1**: Routes through pixel pipeline (alpha test, depth test, blending, fog, etc.)
- **Use cases**:
  - Fast CPU uploads (textures, UI elements)
  - Software rendering fallback
  - Debug visualization
  - Multi-pass effects when pipeline routing enabled

### 2D Blitter [V2+] (0x2c0-0x2fc)
- Separate pipeline for 2D operations, bypasses 3D rasterization entirely
