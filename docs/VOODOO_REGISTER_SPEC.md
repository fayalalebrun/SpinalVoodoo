# Voodoo Graphics Hardware Register Specification
## Host/Software Perspective

**Document Version:** 1.0
**Date:** 2025-10-22
**Target Hardware:** 3dfx Voodoo Graphics, Voodoo 2, Voodoo Banshee, Voodoo 3
**Purpose:** FPGA/Verilog implementation reference

---

## Version Support Legend

This document uses the following notation to indicate feature availability across Voodoo hardware generations:

- **[V1]** - Available in Voodoo 1 (and all later versions unless otherwise noted)
- **[V2+]** - Only in Voodoo 2 and later (not in Voodoo 1)
- **[Banshee+]** - Only in Voodoo Banshee and Voodoo 3
- **[V3+]** - Only in Voodoo 3

### Voodoo 1 Core Features
- Basic 3D triangle rasterization (0x008-0x080, 0x088-0x100)
- Color/alpha/Z buffering and blending (0x104-0x114)
- Texture mapping (single or dual TMU)
- Fog effects (0x108, 0x160-0x1DC)
- Linear framebuffer access (0x400000-0x7FFFFF)
- fbiInit0-6 registers (0x210-0x248)
- Register remapping (fbiInit3 bit 0)
- Statistics counters (0x14C-0x15C)

### Voodoo 2 Additions
- **Command FIFO** (0x1E0-0x1F8, 0x200000-0x3FFFFF memory range)
- **fbiInit7** register (0x24C) for FIFO enable
- **Screen filter** (0x230) for edge anti-aliasing
- **SLI support** (FBIINIT1_SLI_ENABLE bit 23)
- **2D blitter** registers (0x2C0-0x2FC)
- 12-bit clip coordinates (vs 10-bit in V1)

### Voodoo Banshee Additions
- **Setup mode/vertex buffer** (0x260-0x2A4)
- **Banshee-specific 2D blitter** with enhanced features
- Banshee buffer registers (0x1EC-0x1F8, overlapping with V2 CMDFIFO space)
- Secondary clip registers (0x200, 0x204)
- Overlay buffer support (0x250)

### Voodoo 3 Additions
- Dual TMU architecture (always enabled)
- Enhanced texture cache sharing between TMUs
- All Banshee features included

---

## Table of Contents

1. [Version Support Legend](#version-support-legend)
2. [Overview](#overview)
3. [Register Address Map](#register-address-map)
4. [Status and Control Registers](#status-and-control-registers)
5. [Triangle Rendering Registers](#triangle-rendering-registers)
6. [Rendering Mode Registers](#rendering-mode-registers)
7. [Texture Registers](#texture-registers)
8. [Framebuffer Registers](#framebuffer-registers)
9. [Initialization Registers](#initialization-registers)
10. [Display and Video Registers](#display-and-video-registers)
11. [2D Blitter Registers (Voodoo 2/Banshee)](#2d-blitter-registers)
12. [Command FIFO Registers (Voodoo 2)](#command-fifo-registers)
13. [Setup Mode Registers (Vertex Buffer)](#setup-mode-registers)

---

## Overview

This document describes the Voodoo Graphics register interface from the **host software perspective**. It documents:
- What happens when the host **writes** to each register
- What value is **returned** when the host reads each register
- Functional behavior and side effects
- Bit field definitions and their meanings

All registers are **32-bit** and accessed via MMIO (Memory-Mapped I/O). Base address is configured via PCI BAR0.

### Memory Map Overview

- **0x000000 - 0x0003FF**: **[V1]** Register space (remappable via fbiInit3)
- **0x000400 - 0x0007FF**: **[V1]** Remapped register space (when fbiInit3 bit 0 enabled)
- **0x200000 - 0x3FFFFF**: **[V2+]** Command FIFO space (Voodoo 2 only, when fbiInit7 bit 8 enabled)
- **0x400000 - 0x7FFFFF**: **[V1]** Linear Framebuffer (LFB) access
- **0x800000 - 0xBFFFFF**: **[V1]** Texture memory access

### Chip Select Encoding

Many registers are replicated across multiple chips. The chip select is encoded in bits [13:10] of the address:
- **Bit 10 (0x400)**: FBI (Frame Buffer Interface) chip
- **Bit 11 (0x800)**: TREX0 (Texture Engine 0) chip
- **Bit 12 (0x1000)**: TREX1 (Texture Engine 1) chip - dual TMU only
- **Bit 13 (0x2000)**: Reserved

If bits [13:10] are all zero, the write applies to **all chips**.

### Pipeline Synchronization Behavior

SST-1 registers fall into two categories based on whether they require pipeline flushing. The **Sync** column in the official datasheet specifies this behavior:

**"The sync column indicates whether the graphics processor must wait for the current command to finish before loading a particular register from the FIFO. A 'yes' in the sync column means the graphics processor will flush the data pipeline before loading the register."**

#### Sync=No Registers (No Pipeline Flush - Changes Take Effect for Next Triangle)

| Address Range | Registers | Category |
|---------------|-----------|----------|
| 0x000 | status | Status (read-only operation) |
| 0x008-0x01C | vertexAx, vertexAy, vertexBx, vertexBy, vertexCx, vertexCy | Triangle vertices (fixed-point) |
| 0x020-0x03C | startR, startG, startB, startZ, startA, startS, startT, startW | Triangle start parameters |
| 0x040-0x07C | dRdX, dGdX, dBdX, dZdX, dAdX, dSdX, dTdX, dWdX, dRdY, dGdY, dBdY, dZdY, dAdY, dSdY, dTdY, dWdY | Triangle gradients |
| 0x080 | triangleCMD | Triangle render command (fixed-point) |
| 0x088-0x0FC | fvertexAx/Ay/Bx/By/Cx/Cy, fstartR/G/B/Z/A/S/T/W, fdRdX/dY, fdGdX/dY, fdBdX/dY, fdZdX/dY, fdAdX/dY, fdSdX/dY, fdTdX/dY, fdWdX/dY | Triangle parameters (floating-point) |
| 0x100 | ftriangleCMD | Triangle render command (floating-point) |
| 0x104 | fbzColorPath | Color combine path control |
| 0x108 | fogMode | Fog mode control |
| 0x10C | alphaMode | Alpha test and blend control |
| 0x300 | textureMode | Texture format and filtering (per-TMU) |
| 0x304 | tLOD | Texture LOD control (per-TMU) |
| 0x308 | tDetail | Texture detail control (per-TMU) |
| 0x30C-0x318 | texBaseAddr, texBaseAddr_1, texBaseAddr_2, texBaseAddr_3_8 | Texture memory addresses (per-TMU) |

#### Sync=Yes Registers (Automatic Pipeline Flush - Hardware Waits for In-Flight Triangles)

| Address Range | Registers | Category |
|---------------|-----------|----------|
| 0x110 | fbzMode | RGB/depth buffer control |
| 0x114 | lfbMode | Linear framebuffer mode |
| 0x118 | clipLeftRight | Horizontal clip bounds |
| 0x11C | clipLowYHighY | Vertical clip bounds |
| 0x120 | nopCMD | Explicit pipeline flush command |
| 0x124 | fastfillCMD | Fast fill command |
| 0x128 | swapbufferCMD | Buffer swap command |
| 0x12C | fogColor | Fog color constant |
| 0x130 | zaColor | Depth/alpha constant |
| 0x134 | chromaKey | Chroma key color |
| 0x140 | stipple | Stipple pattern |
| 0x144 | color0 | Constant color 0 |
| 0x148 | color1 | Constant color 1 |
| 0x160-0x1DC | fogTable (32 entries) | Fog lookup table |
| 0x31C | trexInit0 | TREX initialization 0 (per-TMU) |
| 0x320 | trexInit1 | TREX initialization 1 (per-TMU) |
| 0x324-0x350 | nccTable0 (12 entries) | NCC table 0 (per-TMU) |
| 0x354-0x380 | nccTable1 (12 entries) | NCC table 1 (per-TMU) |

#### FIFO=No Registers (Bypass PCI FIFO - Immediate Write, Not Queued)

These registers write **directly to hardware**, bypassing the PCI FIFO. They cross clock domains using synchronizers.

| Address Range | Registers | Category |
|---------------|-----------|----------|
| 0x200 | fbiInit4 | Hardware initialization |
| 0x204 | vRetrace | Vertical retrace counter (read-only) |
| 0x208 | backPorch | Video timing |
| 0x20C | videoDimensions | Video configuration |
| 0x210-0x21C | fbiInit0, fbiInit1, fbiInit2, fbiInit3 | FBI hardware initialization |
| 0x220 | hSync | Horizontal sync timing |
| 0x224 | vSync | Vertical sync timing |
| 0x228 | clutData | Color lookup table |
| 0x22C | dacData | DAC configuration |
| 0x230 | maxRgbDelta | Video filtering (V2+) |
| 0x24C | fbiInit7 | Command FIFO enable (V2+) |
| 0x14C-0x15C | fbiPixelsIn, fbiChromaFail, fbiZfuncFail, fbiAfuncFail, fbiPixelsOut | Statistics counters (read-only) |

**Important**: Writing to FIFO=No registers while the FIFO is non-empty or graphics engine is busy can cause race conditions. Software must ensure the pipeline is idle before writing these registers.

#### FIFO=Yes Registers (Queued in PCI FIFO - Asynchronous Write)

**All other registers** (0x000-0x1DC, 0x300-0x380) go into the PCI FIFO and are processed asynchronously by the graphics engine. This includes:
- Triangle parameters and commands
- Rendering state (color path, fog, alpha, texture modes)
- Clipping, constants, fog tables
- Per-TMU texture configuration

**When FIFO is full**: CPU write stalls with PCI wait states until space becomes available. Commands are **never dropped**.

#### Key Points

- **FIFO=Yes registers** (most registers): Queued in 64-entry PCI FIFO, processed asynchronously
- **FIFO=No registers** (init/video): Write directly to hardware, bypass FIFO entirely
- **Sync=Yes registers**: Hardware **automatically flushes pipeline** when you write to them. No manual action needed.
- **Sync=No registers**: Changes take effect for the next rendering command. No pipeline flush occurs.
- **Performance**: Sync=Yes registers cause small stalls. Change them infrequently for best performance.
- **FIFO full behavior**: CPU stalls with PCI wait states (not dropped)
- **Special exception**: fbzColorPath bit 27 (texture enable) requires **manual nopCMD** before changing, even though fbzColorPath is normally Sync=No. This is because enabling/disabling texture mapping changes the TREX→FBI data flow.

#### Examples

**Changing clip bounds (Sync=Yes, automatic flush):**
```c
voodoo_write(clipLeftRight, new_clip);  // Hardware automatically waits for triangles to finish
```

**Changing texture enable (Sync=No, manual flush required):**
```c
voodoo_write(nopCMD, 0);                     // Manually flush pipeline
voodoo_write(fbzColorPath, value_with_bit27_changed);  // Now safe to change
```

---

## Register Address Map

### Core Rendering Registers (0x000 - 0x1FF)

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x000 | status | R/W | **[V1]** | Hardware status and FIFO state |
| 0x004 | (reserved) | - | **[V1]** | Reserved - do not access |
| 0x008 | vertexAx | W | **[V1]** | Triangle vertex A X coordinate (12.4 fixed) |
| 0x00C | vertexAy | W | **[V1]** | Triangle vertex A Y coordinate (12.4 fixed) |
| 0x010 | vertexBx | W | **[V1]** | Triangle vertex B X coordinate (12.4 fixed) |
| 0x014 | vertexBy | W | **[V1]** | Triangle vertex B Y coordinate (12.4 fixed) |
| 0x018 | vertexCx | W | **[V1]** | Triangle vertex C X coordinate (12.4 fixed) |
| 0x01C | vertexCy | W | **[V1]** | Triangle vertex C Y coordinate (12.4 fixed) |
| 0x020 | startR | W | **[V1]** | Starting red value (12.12 fixed) |
| 0x024 | startG | W | **[V1]** | Starting green value (12.12 fixed) |
| 0x028 | startB | W | **[V1]** | Starting blue value (12.12 fixed) |
| 0x02C | startZ | W | **[V1]** | Starting Z/depth value (20.12 fixed) |
| 0x030 | startA | W | **[V1]** | Starting alpha value (12.12 fixed) |
| 0x034 | startS | W | **[V1]** | Starting S texture coordinate (14.18 fixed) |
| 0x038 | startT | W | **[V1]** | Starting T texture coordinate (14.18 fixed) |
| 0x03C | startW | W | **[V1]** | Starting W value (2.30 fixed) |
| 0x040 | dRdX | W | **[V1]** | Red gradient dR/dX (12.12 fixed) |
| 0x044 | dGdX | W | **[V1]** | Green gradient dG/dX (12.12 fixed) |
| 0x048 | dBdX | W | **[V1]** | Blue gradient dB/dX (12.12 fixed) |
| 0x04C | dZdX | W | **[V1]** | Z gradient dZ/dX (20.12 fixed) |
| 0x050 | dAdX | W | **[V1]** | Alpha gradient dA/dX (12.12 fixed) |
| 0x054 | dSdX | W | **[V1]** | S texture gradient dS/dX (14.18 fixed) |
| 0x058 | dTdX | W | **[V1]** | T texture gradient dT/dX (14.18 fixed) |
| 0x05C | dWdX | W | **[V1]** | W gradient dW/dX (2.30 fixed) |
| 0x060 | dRdY | W | **[V1]** | Red gradient dR/dY (12.12 fixed) |
| 0x064 | dGdY | W | **[V1]** | Green gradient dG/dY (12.12 fixed) |
| 0x068 | dBdY | W | **[V1]** | Blue gradient dB/dY (12.12 fixed) |
| 0x06C | dZdY | W | **[V1]** | Z gradient dZ/dY (20.12 fixed) |
| 0x070 | dAdY | W | **[V1]** | Alpha gradient dA/dY (12.12 fixed) |
| 0x074 | dSdY | W | **[V1]** | S texture gradient dS/dY (14.18 fixed) |
| 0x078 | dTdY | W | **[V1]** | T texture gradient dT/dY (14.18 fixed) |
| 0x07C | dWdY | W | **[V1]** | W gradient dW/dY (2.30 fixed) |
| 0x080 | triangleCMD | W | **[V1]** | Triangle command - triggers rendering |
| 0x088-0x0FC | fvertex*, fstart*, fd*d* | W | **[V1]** | Floating-point triangle parameters |
| 0x100 | ftriangleCMD | W | **[V1]** | Floating-point triangle command |
| 0x104 | fbzColorPath | R/W | **[V1]** | Color path configuration |
| 0x108 | fogMode | R/W | **[V1]** | Fog mode configuration |
| 0x10C | alphaMode | R/W | **[V1]** | Alpha blending configuration |
| 0x110 | fbzMode | R/W | **[V1]** | Framebuffer and Z-buffer mode |
| 0x114 | lfbMode | R/W | **[V1]** | Linear framebuffer mode |
| 0x118 | clipLeftRight | R/W | **[V1]** | Clip window left/right (10-bit V1, 12-bit V2+) |
| 0x11C | clipLowYHighY | R/W | **[V1]** | Clip window top/bottom (10-bit V1, 12-bit V2+) |
| 0x120 | nopCMD | W | **[V1]** | No-operation command (resets counters) |
| 0x124 | fastfillCMD | W | **[V1]** | Fast fill command |
| 0x128 | swapbufferCMD | W | **[V1]** | Buffer swap command |
| 0x12C | fogColor | W | **[V1]** | Fog color RGB |
| 0x130 | zaColor | W | **[V1]** | Z/Alpha color for fills |
| 0x134 | chromaKey | W | **[V1]** | Chroma key color |
| 0x138 | (reserved) | - | **[V1]** | Reserved - do not access |
| 0x13C | (reserved) | - | **[V1]** | Reserved - do not access |
| 0x140 | stipple | R/W | **[V1]** | Stipple pattern |
| 0x144 | color0 | R/W | **[V1]** | Constant color 0 |
| 0x148 | color1 | R/W | **[V1]** | Constant color 1 |
| 0x14C | fbiPixelsIn | R | **[V1]** | Statistics: pixels input to FBI |
| 0x150 | fbiChromaFail | R | **[V1]** | Statistics: chroma key test failures |
| 0x154 | fbiZFuncFail | R | **[V1]** | Statistics: Z test failures |
| 0x158 | fbiAFuncFail | R | **[V1]** | Statistics: alpha test failures |
| 0x15C | fbiPixelsOut | R | **[V1]** | Statistics: pixels output from FBI |
| 0x160-0x1DC | fogTable00-1f | W | **[V1]** | Fog lookup table (32 entries, 2 per register) |

### Command FIFO Registers (0x1E0 - 0x1FC) - Voodoo 2

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x1E0 | cmdFifoBaseAddr | R/W | **[V2+]** | Command FIFO base and end address |
| 0x1E4 | cmdFifoBump | W | **[V2+]** | Bump FIFO write pointer |
| 0x1E8 | cmdFifoRdPtr | R/W | **[V2+]** | Command FIFO read pointer |
| 0x1EC | cmdFifoAMin | R/W | **[V2+]** | FIFO AGP minimum address |
| 0x1F0 | cmdFifoAMax | R/W | **[V2+]** | FIFO AGP maximum address |
| 0x1F4 | cmdFifoDepth | R/W | **[V2+]** | Command FIFO depth counter |
| 0x1F8 | cmdFifoHoles | R | **[V2+]** | Command FIFO hole count |

### Banshee Buffer Registers (0x1EC - 0x1FC)

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x1EC | colBufferAddr | W | **[Banshee+]** | Color buffer base address |
| 0x1F0 | colBufferStride | W | **[Banshee+]** | Color buffer stride/tiling |
| 0x1F4 | auxBufferAddr | W | **[Banshee+]** | Auxiliary buffer base address |
| 0x1F8 | auxBufferStride | W | **[Banshee+]** | Auxiliary buffer stride/tiling |

### Initialization and Display Registers (0x200 - 0x24C)

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x200 | fbiInit4 | R/W | **[V1]** | FBI initialization 4 (timing) |
| 0x200 | clipLeftRight1 | W | **[Banshee+]** | Banshee: Secondary clip left/right |
| 0x204 | vRetrace | R | **[V1]** | Vertical retrace line counter |
| 0x204 | clipTopBottom1 | W | **[Banshee+]** | Banshee: Secondary clip top/bottom |
| 0x208 | backPorch | W | **[V1]** | Display back porch timing |
| 0x20C | videoDimensions | W | **[V1]** | Display width and height |
| 0x210 | fbiInit0 | R/W | **[V1]** | FBI initialization 0 (VGA pass, reset) |
| 0x214 | fbiInit1 | R/W | **[V1]** | FBI initialization 1 (PCI timing, SLI) |
| 0x218 | fbiInit2 | R/W | **[V1]** | FBI initialization 2 (buffer config) |
| 0x21C | fbiInit3 | R/W | **[V1]** | FBI initialization 3 (remap enable) |
| 0x220 | hSync | W | **[V1]** | Horizontal sync timing |
| 0x224 | vSync | W | **[V1]** | Vertical sync timing |
| 0x228 | clutData | W | **[V1]** | Color lookup table data |
| 0x22C | dacData | W | **[V1]** | DAC register access |
| 0x230 | maxRgbDelta | W | **[V1]** | Max RGB difference for video filtering |
| 0x234-0x23C | (reserved) | - | **[V1]** | Reserved - do not access |
| 0x240-0x24C | (reserved) | - | **[V1]** | Reserved - do not access |
| 0x250-0x25C | (reserved) | - | **[V1]** | Reserved - do not access |

### Setup Mode Registers (0x260 - 0x2A4)

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x260 | sSetupMode | W | **[Banshee+]** | Setup mode control flags |
| 0x264 | sVx | W | **[Banshee+]** | Vertex X coordinate (float) |
| 0x268 | sVy | W | **[Banshee+]** | Vertex Y coordinate (float) |
| 0x26C | sARGB | W | **[Banshee+]** | Vertex packed ARGB color |
| 0x270 | sRed | W | **[Banshee+]** | Vertex red component (float) |
| 0x274 | sGreen | W | **[Banshee+]** | Vertex green component (float) |
| 0x278 | sBlue | W | **[Banshee+]** | Vertex blue component (float) |
| 0x27C | sAlpha | W | **[Banshee+]** | Vertex alpha component (float) |
| 0x280 | sVz | W | **[Banshee+]** | Vertex Z coordinate (float) |
| 0x284 | sWb | W | **[Banshee+]** | Vertex W billboard (float) |
| 0x288 | sW0 | W | **[Banshee+]** | Vertex W TMU0 (float) |
| 0x28C | sS0 | W | **[Banshee+]** | Vertex S TMU0 (float) |
| 0x290 | sT0 | W | **[Banshee+]** | Vertex T TMU0 (float) |
| 0x294 | sW1 | W | **[Banshee+]** | Vertex W TMU1 (float) |
| 0x298 | sS1 | W | **[Banshee+]** | Vertex S TMU1 (float) |
| 0x29C | sT1 | W | **[Banshee+]** | Vertex T TMU1 (float) |
| 0x2A0 | sDrawTriCMD | W | **[Banshee+]** | Draw triangle command |
| 0x2A4 | sBeginTriCMD | W | **[Banshee+]** | Begin triangle strip/fan |

### 2D Blitter Registers (0x2C0 - 0x2FC) - Voodoo 2/Banshee

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x2C0 | bltSrcBaseAddr | W | **[V2+]** | Blit source base address |
| 0x2C4 | bltDstBaseAddr | W | **[V2+]** | Blit destination base address |
| 0x2C8 | bltXYStrides | W | **[V2+]** | Source and dest X/Y strides |
| 0x2CC | bltSrcChromaRange | W | **[V2+]** | Source chroma key range |
| 0x2D0 | bltDstChromaRange | W | **[V2+]** | Destination chroma key range |
| 0x2D4 | bltClipX | W | **[V2+]** | Blit clip X bounds |
| 0x2D8 | bltClipY | W | **[V2+]** | Blit clip Y bounds |
| 0x2E0 | bltSrcXY | W | **[V2+]** | Blit source X,Y position |
| 0x2E4 | bltDstXY | W | **[V2+]** | Blit dest X,Y position (can trigger) |
| 0x2E8 | bltSize | W | **[V2+]** | Blit width/height (can trigger) |
| 0x2EC | bltRop | W | **[V2+]** | Raster operation codes |
| 0x2F0 | bltColor | W | **[V2+]** | Foreground and background colors |
| 0x2F8 | bltCommand | W | **[V2+]** | Blit command (triggers operation) |
| 0x2FC | bltData | W | **[V2+]** | Host data for CPU-to-screen blits |

### Texture Registers (0x300 - 0x380)

| Address | Name | R/W | Version | Description |
|---------|------|-----|---------|-------------|
| 0x300 | textureMode | W | **[V1]** | Texture mode and format |
| 0x304 | tLOD | W | **[V1]** | Texture LOD control |
| 0x308 | tDetail | W | **[V1]** | Detail texture parameters |
| 0x30C | texBaseAddr | W | **[V1]** | Texture base address (LOD 0) |
| 0x310 | texBaseAddr_1 | W | **[V1]** | Texture base address (LOD 1) |
| 0x314 | texBaseAddr_2 | W | **[V1]** | Texture base address (LOD 2) |
| 0x318 | texBaseAddr_3_8 | W | **[V1]** | Texture base address (LOD 3-8) |
| 0x31C | trexInit0 | W | **[V1]** | TREX initialization register 0 |
| 0x320 | trexInit1 | W | **[V1]** | TREX initialization register 1 |
| 0x324-0x350 | nccTable0_* | W | **[V1]** | NCC table 0 (Y, I, Q components) |
| 0x354-0x380 | nccTable1_* | W | **[V1]** | NCC table 1 (Y, I, Q components) |

### Remapped Registers (0x400 - 0x4FF) **[V1]**

When `fbiInit3` bit 0 (FBIINIT3_REMAP) is set and address bit 21 is set, triangle registers at 0x008-0x0FF are remapped to interleaved addresses 0x400-0x4FF. This allows optimized sequential writes. This feature is available in **Voodoo 1** and all later versions.

---

## Status and Control Registers

### status (0x000) - Read/Write **[V1]**

**Function**: Returns current hardware status, FIFO state, and swap buffer status. Writing clears PCI interrupts.

**Read Behavior**:
```
Bits [5:0]   : PCI FIFO freespace (0x3F=FIFO empty). Default 0x3F.
Bit  6       : Vertical retrace (0=retrace active, 1=retrace inactive). Default 1.
Bit  7       : FBI graphics engine busy (0=idle, 1=busy). Default 0.
Bit  8       : TREX busy (0=idle, 1=busy). Default 0.
Bit  9       : SST-1 busy (0=idle, 1=busy). Default 0.
Bits [11:10] : Displayed buffer (0=buffer 0, 1=buffer 1, 2=aux buffer, 3=reserved). Default 0.
Bits [27:12] : Memory FIFO freespace (0xFFFF=FIFO empty). Default 0xFFFF.
Bits [30:28] : Swap buffers pending. Default 0.
Bit  31      : PCI interrupt generated. Default 0. (Not implemented in V1 hardware)
```

**Bit Details**:
- **Bits [5:0]**: Internal host FIFO (64 entries deep) available space
- **Bit 6**: Monitor vertical retrace signal state
- **Bit 7**: FBI graphics engine only (not including FIFOs)
- **Bit 8**: Any TREX unit busy (graphics engine or FIFOs)
- **Bit 9**: Any SST-1 unit active (graphics engines, FIFOs, etc.)
- **Bits [11:10]**: Which RGB buffer is sent to monitor
- **Bits [27:12]**: Memory FIFO space (up to 65,536 entries depending on frame buffer memory)
- **Bits [30:28]**: Incremented on SWAPBUFFER command, decremented on completion
- **Bit 31**: Set when vertical retrace interrupt generated (cleared by writing to status)

**Write Behavior**: Clears any SST-1 generated PCI interrupts (data value is "don't care")

**Implementation Notes**:
- Software polls this register to check if hardware is ready
- Use bit 9 (SST-1 busy) to determine if all units are idle
- VRETRACE (bit 6) used for timing buffer swaps
- **Note**: Bit 6 is **inverse logic** - 0 means retrace active, 1 means inactive

---

## Triangle Rendering Registers

### Overview

Triangle rendering uses two interfaces:
1. **Fixed-point registers** (0x008-0x080): Integer coordinates and parameters
2. **Floating-point registers** (0x088-0x100): IEEE 754 float parameters (converted internally)

Both interfaces write to the same internal parameter registers. Software chooses based on convenience.

### vertexAx, vertexAy, vertexBx, vertexBy, vertexCx, vertexCy (0x008-0x01C)

**Function**: Define the three triangle vertices in screen coordinates.

**Format**: 12.4 signed fixed-point (16-bit value in 32-bit register)
- Bits [15:0]: Coordinate value
  - Bits [15:4]: Integer part (signed, -2048 to +2047)
  - Bits [3:0]: Fractional part (1/16 pixel precision)
- Bits [31:16]: Ignored

**Write Behavior**:
- Stores vertex coordinate
- Value is masked to 16 bits: `value & 0xFFFF`
- Applied to chip(s) selected by address bits [13:10]

**Coordinate System**:
- Origin (0,0) is top-left of screen
- X increases to the right
- Y increases downward
- Subpixel precision allows smooth anti-aliasing

**Example**:
```c
// Position vertex A at (320.5, 240.25)
write32(0x008, (320 << 4) | 8);     // X = 320.5 (320*16 + 8)
write32(0x00C, (240 << 4) | 4);     // Y = 240.25 (240*16 + 4)
```

---

### fvertexAx, fvertexAy, fvertexBx, fvertexBy, fvertexCx, fvertexCy (0x088-0x09C)

**Function**: Define triangle vertices using IEEE 754 floating-point coordinates.

**Format**: 32-bit IEEE 754 single-precision float

**Write Behavior**:
- Float value is converted to 12.4 fixed-point internally
- Conversion: `fixed = (int16_t)(float_value * 16.0f) & 0xFFFF`
- Stored in same internal registers as fixed-point vertex registers

**Example**:
```c
float ax = 320.5f;
write32(0x088, *(uint32_t*)&ax);    // Write as raw IEEE 754 bits
```

**Implementation Notes**:
- Floating-point interface is more convenient for software
- Hardware always operates on fixed-point internally
- Conversion may introduce small rounding errors

---

### startR, startG, startB, startZ, startA (0x020-0x030)

**Function**: Starting color, depth, and alpha values at vertex A.

**Format**: 12.12 unsigned fixed-point (24-bit value in 32-bit register)
- Bits [23:0]: Parameter value
  - Bits [23:12]: Integer part (0-4095)
  - Bits [11:0]: Fractional part (1/4096 precision)
- Bits [31:24]: Ignored

**Write Behavior**:
- Stores initial parameter value
- Value masked to 24 bits: `value & 0xFFFFFF`
- RGB values represent 0.0-255.0 range (multiply by 16)
- Z values represent full depth range
- Alpha represents 0.0-255.0 range

**Color Encoding**:
- 0x000000 = 0.0 (black or transparent)
- 0xFF000 = 255.0 (full intensity/opaque)
- 0xFFFFFF = Maximum representable value

**Example**:
```c
// Set starting red to 128.5
uint32_t red = (uint32_t)(128.5 * 4096.0);
write32(0x020, red & 0xFFFFFF);
```

---

### fstartR, fstartG, fstartB, fstartZ, fstartA (0x0A0-0x0B0)

**Function**: Starting values using floating-point format.

**Format**: 32-bit IEEE 754 single-precision float

**Write Behavior**:
- Float converted to 12.12 fixed-point: `fixed = (int32_t)(float_value * 4096.0f)`
- Stored in same internal registers as fixed-point start registers

---

### startS, startT, startW (0x034-0x03C)

**Function**: Starting texture coordinates and W value.

**Format**: 32-bit signed integer (converted to 14.18 fixed or 2.30 fixed internally)

**Write Behavior**:
- **startS, startT**: Converted to 14.18 fixed: `value << 14`
- **startW**: Converted to 2.30 fixed: `value << 2`
- Applied to selected TMU chip(s) via chip select bits
- If chip select is FBI (bit 10), startW is stored for perspective correction
- If chip select includes TREX chips (bits 11-12), coordinates stored in TMU

**Chip Select Behavior**:
```c
if (chip & CHIP_FBI)     // Bit 10 set
    params.startW = value << 2;
if (chip & CHIP_TREX0)   // Bit 11 set
    params.tmu[0].startS/T/W = ...;
if (chip & CHIP_TREX1)   // Bit 12 set
    params.tmu[1].startS/T/W = ...;
```

---

### fstartS, fstartT, fstartW (0x0B4-0x0BC)

**Function**: Floating-point texture coordinates and W.

**Format**: 32-bit IEEE 754 single-precision float

**Write Behavior**:
- Float converted to 64-bit fixed: `fixed = (int64_t)(float_value * 4294967296.0f)`
- Provides full 32.32 fixed-point precision internally
- Chip select determines which TMU(s) receive the value

---

### dRdX, dGdX, dBdX, dZdX, dAdX (0x040-0x050)

**Function**: Color, depth, and alpha gradients in X direction.

**Format**: 12.12 signed fixed-point (24-bit value in 32-bit register)

**Write Behavior**:
- Value masked to 24 bits
- Sign-extended from bit 23: `value = (val & 0xFFFFFF) | ((val & 0x800000) ? 0xFF000000 : 0)`
- Represents change per pixel in X direction

**Gradient Calculation**:
```
new_value = start_value + (dRdX * x_offset) + (dRdY * y_offset)
```

---

### dSdX, dTdX, dWdX (0x054-0x05C)

**Function**: Texture coordinate and W gradients in X direction.

**Format**: 32-bit signed integer (converted internally)

**Write Behavior**:
- dSdX, dTdX converted to 14.18 fixed: `value << 14`
- dWdX converted to 2.30 fixed: `value << 2`
- Applied per chip select

---

### dRdY, dGdY, dBdY, dZdY, dAdY (0x060-0x070)

**Function**: Color, depth, and alpha gradients in Y direction (same as dXdX but for Y).

---

### dSdY, dTdY, dWdY (0x074-0x07C)

**Function**: Texture coordinate and W gradients in Y direction (same as dXdX but for Y).

---

### triangleCMD (0x080) - Write Only, Command Trigger

**Function**: Triggers triangle rasterization with current parameters.

**Format**:
```
Bit  31     : SIGN - Triangle area sign (0=positive/CCW, 1=negative/CW)
Bits [30:0] : Reserved/ignored (only bit 31 is used)
```

**Triangle Area Calculation**:
The sign bit must match the actual triangle orientation. To calculate:
1. Sort vertices by Y coordinate: A.y ≤ B.y ≤ C.y
2. Calculate: AREA = ((dxAB × dyBC) - (dxBC × dyAB)) / 2
   - Where: dxAB = A.x - B.x, dyBC = B.y - C.y
   - dxBC = B.x - C.x, dyAB = A.y - B.y
3. Write sign(AREA) to bit 31

**Write Behavior**:
1. Stores sign bit: `params.sign = value & (1 << 31)`
2. Updates NCC lookup tables if dirty (texture color conversion)
3. Queues triangle for rendering with all current parameters
4. Increments command read counter
5. Wakes FIFO processing thread

**CRITICAL CAVEATS**:
- ⚠️ **If sign bit doesn't match actual triangle orientation, FBI enters infinite rendering loop!**
- ⚠️ **Write combining**: Must fence before and after this register write in out-of-order I/O environments
- ⚠️ **Chip field transitions**: Under certain circumstances, FBI can lose chip field changes. Workaround: write to TMU3 texture address (which doesn't exist) to flush

**Implementation Notes**:
- All parameter registers (vertex, start, gradients) must be written before this command
- Graphics engine processes triangle asynchronously
- Status register indicates completion

---

### ftriangleCMD (0x100) - Write Only, Command Trigger

**Function**: Same as triangleCMD but for floating-point triangle interface.

**Behavior**: Identical to triangleCMD (0x080).

---

## Rendering Mode Registers

### fbzColorPath (0x104) - Read/Write

**Function**: Configures the color combine pipeline.

**Bit Fields**:
```
Bits [1:0]   : RGB_SEL - RGB source select
               0 = Iterator RGB
               1 = Texture color
               2 = Color1 register
               3 = LFB (linear framebuffer)

Bits [3:2]   : ALPHA_SEL - Alpha source select
               0 = Iterator alpha
               1 = Texture alpha
               2 = Color1 alpha
               3 = LFB alpha

Bit  4       : CC_LOCALSELECT - Color combiner local select
               0 = Iterator RGB
               1 = Color from texture/color path

Bits [6:5]   : CCA_LOCALSELECT - Alpha combiner local select
               0 = Iterator alpha
               1 = Color0 alpha
               2 = Iterator Z

Bit  7       : CC_LOCALSELECT_OVERRIDE - Override local select
Bit  8       : CC_ZERO_OTHER - Force other color to zero
Bit  9       : CC_SUB_CLOCAL - Subtract local color
Bits [12:10] : CC_MSELECT - Color combiner multiply select
               0 = Zero
               1 = Local color
               2 = Other alpha
               3 = Local alpha
               4 = Texture color
               5 = Texture RGB

Bit  13      : CC_REVERSE_BLEND - Reverse blend direction
Bits [15:14] : CC_ADD - Color combiner add mode
               Bit 14: Add local color
               Bit 15: Add local alpha

Bit  16      : CC_INVERT_OUTPUT - Invert color output
Bit  17      : CCA_ZERO_OTHER - Force other alpha to zero
Bit  18      : CCA_SUB_CLOCAL - Subtract local alpha
Bits [21:19] : CCA_MSELECT - Alpha combiner multiply select
               0 = Zero
               1 = Local alpha
               2 = Other alpha
               3 = Local alpha (duplicate)
               4 = Texture alpha

Bit  22      : CCA_REVERSE_BLEND - Reverse alpha blend
Bits [24:23] : CCA_ADD - Alpha combiner add mode
Bit  25      : CCA_INVERT_OUTPUT - Invert alpha output
Bit  26      : FBZ_PARAM_ADJUST - Subpixel correction (1=enable)
Bit  27      : TEXTURE_ENABLE - Enable texture mapping (1=enable)
Bits [31:28] : Reserved
```

**Read Behavior**: Returns current fbzColorPath value

**Write Behavior**:
- Stores new color path configuration
- Updates internal rgb_sel cache: `voodoo->rgb_sel = value & 3`
- Takes effect immediately for new rendering

**Bit 26 - FBZ_PARAM_ADJUST**:
When enabled, adjusts interpolation starting values to account for sub-pixel vertex positions:
- Automatically corrects start parameters for triangles not aligned on integer boundaries
- Compensation: `base_r += (dx * dRdX + dy * dRdY) >> 4` (dx, dy = vertex fractional parts)
- Decreases triangle setup from 7 to 16 clocks, but minimal impact due to pipelining
- **IMPORTANT**: Must pass new start parameters for each triangle when enabled, or parameters will be double-corrected

**Bit 27 - TEXTURE_ENABLE**:
- Controls data transfer from TREX to FBI
- When 0: Gouraud/flat shading only (no TREX data transferred)
- When 1: Texture mapping enabled
- **CRITICAL**: Must write nopCMD before changing this bit!

**Color Combine Equation**:
```
color_out = (clocal * mselect) + add_terms
if (INVERT_OUTPUT)
    color_out = ~color_out
```

**Implementation Notes**:
- Extremely flexible color combining
- Allows multitexture effects, detail textures, light maps
- Most complex register in Voodoo architecture

---

### fogMode (0x108) - Read/Write

**Function**: Configure fog blending parameters.

**Bit Fields**:
```
Bit  0       : FOG_ENABLE - Enable fog effect
Bit  1       : FOG_ADD - Add fog color (vs blend)
Bit  2       : FOG_MULT - Multiply fog
Bit  3       : FOG_ALPHA - Fog affects alpha channel
Bits [4:3]   : FOG_SOURCE
               00 = Constant fog (no depth)
               01 = Z-based fog
               10 = Reserved
               11 = W-based fog (perspective correct)
Bit  5       : FOG_CONSTANT - Use constant fog value
Bits [31:6]  : Reserved
```

**Read Behavior**: Returns current fogMode value

**Write Behavior**: Stores fog configuration, takes effect immediately

**Fog Application**:
```
fog_value = fogTable[fog_index]  // From depth/W
color_out = lerp(color_in, fogColor, fog_value)
```

**Implementation Notes**:
- Fog table loaded via fogTable registers
- 64-entry table for smooth gradients
- W-based fog provides perspective-correct depth fog

---

### alphaMode (0x10C) - Read/Write

**Function**: Configure alpha testing and blending.

**Bit Fields**:
```
Bits [0]     : ALPHA_TEST_ENABLE - Enable alpha testing
Bits [3:1]   : ALPHA_FUNC - Alpha comparison function
               0 = Never pass
               1 = Less than
               2 = Equal
               3 = Less or equal
               4 = Greater than
               5 = Not equal
               6 = Greater or equal
               7 = Always pass

Bits [7:4]   : Reserved
Bits [11:8]  : SRC_ALPHA_FUNC - Source blend function
               0 = Zero
               1 = Source alpha
               2 = Color
               3 = Destination alpha
               4 = One
               5 = One minus source alpha
               6 = One minus color
               7 = One minus destination alpha
               15 = Source alpha saturate

Bits [15:12] : DST_ALPHA_FUNC - Destination blend function
               (Same encoding as SRC_ALPHA_FUNC)

Bits [23:16] : Reserved
Bits [31:24] : ALPHA_REF - Alpha reference value for testing
```

**Read Behavior**: Returns current alphaMode value

**Write Behavior**: Stores alpha test and blend configuration

**Alpha Test**:
```
if (ALPHA_TEST_ENABLE) {
    if (!compare(pixel_alpha, ALPHA_REF, ALPHA_FUNC))
        discard pixel;
}
```

**Alpha Blend**:
```
src_factor = lookup(SRC_ALPHA_FUNC, src_alpha, dst_alpha);
dst_factor = lookup(DST_ALPHA_FUNC, src_alpha, dst_alpha);
color_out = src_color * src_factor + dst_color * dst_factor;
```

---

### fbzMode (0x110) - Read/Write

**Function**: Framebuffer and Z-buffer operation control.

**Bit Fields**:
```
Bit  0       : ENABLE_CLIPPING - Enable scissor clipping
Bit  1       : CHROMAKEY - Enable chroma key test
Bit  2       : Reserved
Bit  3       : WBUFFER_SELECT - Use W-buffer instead of Z-buffer
Bit  4       : DEPTH_ENABLE - Enable depth testing
Bits [7:5]   : DEPTH_FUNC - Depth comparison function
               0 = Never pass
               1 = Less than
               2 = Equal
               3 = Less or equal
               4 = Greater than
               5 = Not equal
               6 = Greater or equal
               7 = Always pass

Bit  8       : DITHER_ENABLE - Enable dithering
Bit  9       : RGB_WRITE_ENABLE - Enable RGB write to framebuffer
Bit  10      : DEPTH_WRITE_ENABLE - Enable depth write
Bit  11      : DITHER_2x2 - Use 2x2 dither (vs 4x4)
Bits [13:12] : Reserved
Bits [15:14] : DRAW_BUFFER - Select draw buffer
               00 = Front buffer
               01 = Back buffer
               10,11 = Reserved

Bit  16      : DEPTH_BIAS - Enable depth bias
Bits [18:17] : Reserved
Bit  19      : DITHER_SUB - Dither subtraction mode
Bit  20      : DEPTH_SOURCE_COMPARE - Compare using depth from iterator
Bits [25:21] : Reserved
Bit  26      : PARAM_ADJUST - Enable parameter adjustment
Bits [31:27] : Reserved
```

**Read Behavior**: Returns current fbzMode value

**Write Behavior**:
- Stores framebuffer configuration
- Triggers voodoo_recalc() to update buffer offsets
- Draw buffer selection takes effect immediately

**Depth Test**:
```
if (DEPTH_ENABLE) {
    depth_in = DEPTH_SOURCE_COMPARE ? iterator_z : pixel_z;
    if (!compare(depth_in, buffer_depth, DEPTH_FUNC))
        discard pixel;
    if (DEPTH_WRITE_ENABLE)
        buffer_depth = depth_in + (DEPTH_BIAS ? bias : 0);
}
```

---

### lfbMode (0x114) - Read/Write

**Function**: Linear framebuffer access mode control.

**Bit Fields**:
```
Bits [1:0]   : LFB_FORMAT - Framebuffer pixel format for writes
               0  = RGB565
               1  = RGB555
               2  = ARGB1555
               4  = XRGB8888
               5  = ARGB8888
               12 = Depth + RGB565
               13 = Depth + RGB555
               14 = Depth + ARGB1555
               15 = Depth only

Bits [3:2]   : Reserved
Bits [5:4]   : LFB_WRITE_BUFFER - Buffer select for writes
               00 = Front buffer
               01 = Back buffer
               10,11 = Reserved

Bits [7:6]   : LFB_READ_BUFFER - Buffer select for reads
               00 = Front buffer
               01 = Back buffer
               10 = Auxiliary buffer (depth/alpha)
               11 = Reserved

Bit  8       : PIXEL_PIPELINE - Route LFB writes through pixel pipeline
Bit  9       : BYTE_SWIZZLE - Enable byte swizzling
Bit  10      : WORD_SWIZZLE - Enable word swizzling
Bit  11      : WRITE_W - Write W value in depth formats
Bits [15:12] : Reserved
Bit  16      : WRITE_BOTH - Write both color and depth
Bits [31:17] : Reserved
```

**Read Behavior**: Returns current lfbMode value

**Write Behavior**:
- Stores LFB configuration
- Triggers voodoo_recalc() to update read/write offsets
- Format changes take effect for next LFB access

**LFB Access**:
- Address range 0x400000-0x7FFFFF maps to framebuffer
- Format determines pixel size and layout
- Pipeline routing enables effects on LFB writes

---

### clipLeftRight (0x118) - Read/Write **[V1]**

**Function**: Define horizontal clip window boundaries.

**Bit Fields**:
```
Bits [11:0]  : **[V2+]** CLIP_RIGHT - Right clip boundary (12 bits, max 4095)
Bits [9:0]   : **[V1]** CLIP_RIGHT - Right clip boundary (10 bits, max 1023)
Bits [15:12] : Reserved (V1) / Extended bits (V2+)
Bits [27:16] : **[V2+]** CLIP_LEFT - Left clip boundary (12 bits, max 4095)
Bits [25:16] : **[V1]** CLIP_LEFT - Left clip boundary (10 bits, max 1023)
Bits [31:28] : Reserved (V1) / Extended bits (V2+)
```

**Read Behavior**: Returns packed clip boundaries

**Write Behavior**:
```c
if (voodoo->type >= VOODOO_2) {
    params.clipRight = value & 0xFFF;
    params.clipLeft = (value >> 16) & 0xFFF;
} else {  // Voodoo 1
    params.clipRight = value & 0x3FF;
    params.clipLeft = (value >> 16) & 0x3FF;
}
```

**Clipping**:
- Pixels outside [clipLeft, clipRight] are discarded
- Inclusive range (both boundaries are inside clip region)
- Applied before rasterization

---

### clipLowYHighY (0x11C) - Read/Write **[V1]**

**Function**: Define vertical clip window boundaries.

**Bit Fields**: Same layout as clipLeftRight but for Y coordinates
```
Bits [11:0]  : **[V2+]** CLIP_HIGH_Y - Bottom clip boundary (12 bits, max 4095)
Bits [9:0]   : **[V1]** CLIP_HIGH_Y - Bottom clip boundary (10 bits, max 1023)
Bits [27:16] : **[V2+]** CLIP_LOW_Y - Top clip boundary (12 bits, max 4095)
Bits [25:16] : **[V1]** CLIP_LOW_Y - Top clip boundary (10 bits, max 1023)
```

**Behavior**: Same as clipLeftRight but for vertical clipping. **Note:** Voodoo 1 supports up to 1024x1024, Voodoo 2+ supports up to 4096x4096.

---

## Command Registers

### nopCMD (0x120) - Write Only

**Function**: No-operation command that flushes the graphics pipeline and optionally resets statistics counters.

**Bit Fields**:
```
Bit 0: Clear statistics counters (1=clear, 0=preserve)
```

**Write Behavior**:
1. **Flushes the graphics pipeline** - waits for all pending rendering to complete
2. If bit 0 is set to 1, resets all statistics registers to zero:
   - fbiPixelsIn = 0
   - fbiChromaFail = 0
   - fbiZFuncFail = 0
   - fbiAFuncFail = 0
   - fbiPixelsOut = 0
3. If bit 0 is set to 0, statistics counters are NOT modified

**Important Notes**:
- **Sync column: Yes** - This register causes pipeline synchronization
- **Must be written before changing fbzColorPath bit 27** (texture mapping enable)
- Used for ensuring pipeline idle state before critical configuration changes
- The SST_IDLE() function in official examples uses `nopCMD` followed by polling STATUS register

**Usage**: Pipeline flush and optional statistics reset

---

### fastfillCMD (0x124) - Write Only

**Function**: Fast clear of RGB and/or depth buffers (screen clear operation).

**Bit Fields**:
```
Bits [31:0]  : Reserved (value ignored)
```

**Write Behavior**:
1. Waits for rendering thread to be idle
2. Clears rectangular region defined by clipLeftRight/clipLowYHighY registers
3. Increments command counter

**FASTFILL Operation**:
- **Bypasses all pixel pipeline stages** - depth test, alpha test, alpha blending, fog all disabled
- **2 pixels/clock performance** (100 Mpixels/sec vs 50 Mpixels/sec for normal rendering)
- Clears **RGB buffer** with color from **color1** register (bits 23:0)
  - Applies dithering if fbzMode bit 8 is set
  - 24-bit color1 value converted to 16-bit with optional dithering
- Clears **depth buffer** with depth from **zaColor** register (bits 15:0)
  - 16-bit depth value (format matches current Z/W buffer mode)

**Control Registers**:
- **clipLeftRight/clipLowYHighY**: Define clear rectangle (inclusive of left/lowY, exclusive of right/highY)
- **fbzMode bit 9**: RGB write mask (0=disable RGB clear, 1=enable)
- **fbzMode bit 10**: Depth write mask (0=disable depth clear, 1=enable)
- **fbzMode bits 15:14**: Buffer select (0=front, 1=back)
- **fbzMode bit 17**: Y origin (0=top of screen, 1=bottom)

**Usage**: Efficiently clear color and/or depth buffers at frame start

**Implementation**:
```c
voodoo_wait_for_render_thread_idle(voodoo);
voodoo_fastfill(voodoo, &voodoo->params);
voodoo->cmd_read++;
```

---

### swapbufferCMD (0x128) - Write Only

**Function**: Swap front and back framebuffers.

**Bit Fields**:
```
Bit  0       : SWAP_MODE
               0 = Immediate swap (no vsync wait)
               1 = Wait for vsync
Bits [8:1]   : SWAP_INTERVAL - Vsync intervals to wait (0-255)
Bits [31:9]  : Reserved
```

**Write Behavior**:

**Immediate Mode (bit 0 = 0)**:
1. Swaps display buffer pointers immediately
2. Decrements swap counter
3. Increments frame counter
4. Updates front_offset for next display

**Vsync Mode (bit 0 = 1)**:
- Standard (double-buffered):
  1. Sets swap_pending flag
  2. Stores swap_interval and target buffer offset
  3. Waits for completion (blocks if not triple-buffered)
  4. Actual swap occurs at next vsync in display callback

- Triple-buffered:
  1. If previous swap pending, wait for it
  2. Set new swap pending
  3. Return immediately (non-blocking)

**Triple vs Double Buffering**:
```c
if (TRIPLE_BUFFER) {
    if (swap_pending)
        wait_for_swap_complete();
    swap_pending = 1;
    swap_offset = front_offset;
    // Return immediately
} else {
    swap_pending = 1;
    swap_offset = front_offset;
    wait_for_swap_complete();  // Block until vsync
}
```

**Banshee Behavior**:
- Updates overlay buffer address
- Uses leftOverlayBuf register value

**SLI Mode**:
- Only swaps when both cards are ready
- Synchronized via swap_mutex

---

### fogColor (0x12C) - Write Only

**Function**: Set RGB fog color.

**Bit Fields**:
```
Bits [7:0]   : BLUE - Fog blue component (0-255)
Bits [15:8]  : GREEN - Fog green component (0-255)
Bits [23:16] : RED - Fog red component (0-255)
Bits [31:24] : Reserved
```

**Write Behavior**:
```c
params.fogColor.r = (value >> 16) & 0xFF;
params.fogColor.g = (value >> 8) & 0xFF;
params.fogColor.b = value & 0xFF;
```

**Usage**: Target color for fog blending (distance haze color)

---

### zaColor (0x130) - Write Only

**Function**: Combined Z and Alpha color for fast fills.

**Bit Fields**:
```
Bits [15:0]  : Depth value (16-bit Z)
Bits [23:16] : Alpha value (8-bit)
Bits [31:24] : Reserved
```

**Write Behavior**: Stores value used by fastfillCMD

**Usage**:
- Screen clears (color + depth)
- Depth buffer initialization

---

### chromaKey (0x134) - Write Only

**Function**: Set chroma key color for transparency effects.

**Bit Fields**:
```
Bits [7:0]   : BLUE - Chroma key blue (0-255)
Bits [15:8]  : GREEN - Chroma key green (0-255)
Bits [23:16] : RED - Chroma key red (0-255)
Bits [31:24] : Reserved
```

**Write Behavior**:
```c
params.chromaKey_r = (value >> 16) & 0xFF;
params.chromaKey_g = (value >> 8) & 0xFF;
params.chromaKey_b = value & 0xFF;
params.chromaKey = value & 0xFFFFFF;
```

**Usage**: Pixels matching this color are rejected (see fbzMode CHROMAKEY bit)

---

### userIntrCMD (0x13C) - Write Only

**Function**: Trigger user interrupt.

**Write Behavior**: Currently triggers fatal error (not implemented)

**Intended Use**: Software-triggered interrupt for synchronization

---

### stipple (0x140) - Read/Write

**Function**: 32-bit stipple pattern for line/polygon rendering.

**Bit Fields**: Each bit represents enable/disable for corresponding pixel

**Read Behavior**: Returns current stipple pattern

**Write Behavior**: Stores 32-bit pattern

**Usage**: Dashed lines, screen-door transparency

---

### color0, color1 (0x144, 0x148) - Read/Write

**Function**: Constant color registers for color combine operations.

**Bit Fields**:
```
Bits [7:0]   : BLUE (0-255)
Bits [15:8]  : GREEN (0-255)
Bits [23:16] : RED (0-255)
Bits [31:24] : ALPHA (0-255)
```

**Read/Write Behavior**: Stores/returns packed ARGB color

**Usage**:
- Constant colors in fbzColorPath
- Blending operations
- Flat shading

---

## Statistics Registers (Read Only)

### fbiPixelsIn (0x14C)

**Read Behavior**: Returns count of pixels input to FBI (lower 24 bits)

**Write Behavior**: Ignored

**Reset**: nopCMD clears to zero

---

### fbiChromaFail (0x150)

**Read Behavior**: Returns count of pixels rejected by chroma key test

---

### fbiZFuncFail (0x154)

**Read Behavior**: Returns count of pixels rejected by depth test

---

### fbiAFuncFail (0x158)

**Read Behavior**: Returns count of pixels rejected by alpha test

---

### fbiPixelsOut (0x15C)

**Read Behavior**: Returns count of pixels written to framebuffer

**Usage**: Performance monitoring, optimization

---

## Fog Table Registers

### fogTable00 - fogTable1f (0x160-0x1DC) - Write Only

**Function**: Define 64-entry fog lookup table.

**Format**: Each register contains 2 entries (32 bits total)
```
Bits [7:0]   : Entry N delta fog (dfog)
Bits [15:8]  : Entry N fog value
Bits [23:16] : Entry N+1 delta fog (dfog)
Bits [31:24] : Entry N+1 fog value
```

**Write Behavior**:
```c
addr = (register_address - 0x160) >> 1;  // Entry pair index
params.fogTable[addr].dfog = value & 0xFF;
params.fogTable[addr].fog = (value >> 8) & 0xFF;
params.fogTable[addr+1].dfog = (value >> 16) & 0xFF;
params.fogTable[addr+1].fog = (value >> 24) & 0xFF;
```

**Table Layout**:
- 64 entries total, 2 per register (32 registers)
- Entry index computed from depth/W value
- Linear interpolation between entries using dfog

**Fog Computation**:
```
fog_index = compute_from_depth(z_or_w);
fog_entry = fogTable[fog_index >> 10];  // Upper bits select entry
fog_frac = fog_index & 0x3FF;           // Lower bits for interpolation
fog_value = fog_entry.fog + (fog_entry.dfog * fog_frac) / 1024;
```

---

## Initialization Registers

### fbiInit0 (0x210) - Read/Write **[V1]**

**Function**: FBI chip initialization and VGA passthrough control.

**Bit Fields**:
```
Bit  0       : VGA_PASSTHROUGH - 0=VGA blocked, 1=VGA passed to monitor
Bit  1       : GRAPHICS_RESET - Reset graphics engine
Bits [31:2]  : Reserved
```

**Read Behavior**: Returns current fbiInit0 value

**Write Behavior** (only when initEnable bit 0 is set):
1. Stores new value
2. Updates VGA passthrough state:
   ```c
   can_blit = (value & VGA_PASSTHROUGH) ? 1 : 0;
   svga_set_override(svga, value & 1);
   ```
3. If GRAPHICS_RESET set:
   - Resets display buffer to 0
   - Resets draw buffer to 1
   - Calls voodoo_recalc() to update buffer offsets

**VGA Passthrough**:
- When 0: Voodoo output to monitor (3D mode)
- When 1: VGA output to monitor (2D mode)
- Allows seamless switching between 2D and 3D

**Graphics Reset**:
- Reinitializes rendering state
- Does NOT clear framebuffers (use fastfillCMD)

---

### fbiInit1 (0x214) - Read/Write **[V1]**

**Function**: PCI timing configuration and SLI enable.

**Bit Fields**:
```
Bit  0       : Reserved
Bit  1       : PCI_WRITE_WAIT_STATES - 0=fast writes, 1=slow writes
Bit  2       : MULTI_SST - Enable multi-SST (SLI) mode **[V1 only]**
Bits [7:3]   : Reserved
Bit  8       : VIDEO_RESET - Reset video timing
Bits [22:9]  : Reserved
Bit  23      : SLI_ENABLE - Enable SLI mode **[V2+ only]**
Bits [31:24] : Reserved
```

**Read Behavior**: Returns current value (bits [1:0] forced from stored value)

**Write Behavior** (only when initEnable bit 0 is set):
1. Stores value with special handling:
   ```c
   fbiInit1 = (value & ~5) | (fbiInit1 & 5);  // Preserve bits 0,2
   ```
2. Updates PCI timing:
   ```c
   write_time = pci_nonburst + pci_burst * ((value & 2) ? 1 : 0);
   burst_time = pci_burst * ((value & 2) ? 2 : 1);
   ```
3. If VIDEO_RESET transitions 1→0:
   - Resets line counter to 0
   - Clears swap_count
   - Resets retrace_count

**PCI Timing**:
- Bit 1 controls write wait states for PCI bus
- Affects overall 3D performance
- Typical: 0 for modern systems, 1 for slow PCI buses

**SLI Configuration**:
- **[V1]** Voodoo 1: Uses MULTI_SST (bit 2)
- **[V2+]** Voodoo 2: Uses SLI_ENABLE (bit 23)
- Enables scan-line interleave rendering (splits rendering between two cards)

---

### fbiInit2 (0x218) - Read/Write **[V1]**

**Function**: Buffer configuration and swap algorithms.

**Bit Fields**:
```
Bits [8:0]   : Reserved
Bits [10:9]  : SWAP_ALGORITHM
               00 = DAC vsync
               01 = DAC data
               10 = PCI FIFO stall
               11 = SLI sync
Bits [20:11] : BUFFER_OFFSET - Buffer offset in 4KB units (0-511)
Bit  21      : Reserved
Bits [31:22] : Reserved
```

**Read Behavior**:
- If initEnable bit 2 set: Returns dac_readdata (DAC access mode)
- Otherwise: Returns fbiInit2 value

**Write Behavior** (only when initEnable bit 0 is set):
1. Stores new value
2. Calls voodoo_recalc() to update buffer offsets:
   ```c
   buffer_offset = ((fbiInit2 >> 11) & 511) * 4096;
   front_offset = disp_buffer * buffer_offset;
   back_offset = draw_buffer * buffer_offset;
   ```

**Buffer Offset**:
- Defines size of each framebuffer in memory
- Typical values: 307200 bytes for 640x480 @ 16bpp = 75 * 4KB
- Front and back buffers allocated sequentially

**Swap Algorithms**:
- DAC_VSYNC: Wait for vertical retrace (normal)
- DAC_DATA: Swap on DAC data write (immediate)
- PCI_FIFO_STALL: Swap when PCI FIFO stalls
- SLI_SYNC: Synchronized swap for SLI mode

---

### fbiInit3 (0x21C) - Read/Write **[V1]**

**Function**: Register remapping and extended features.

**Bit Fields**:
```
Bit  0       : REMAP_ENABLE - Enable register address remapping
Bits [9:1]   : Reserved
Bit  10      : Always reads as 1
Bits [9:8]   : Always read as 2
Bits [31:11] : Reserved
```

**Read Behavior**:
```c
return fbiInit3 | (1 << 10) | (2 << 8);
```

**Write Behavior** (only when initEnable bit 0 is set):
- Stores new remap enable state

**Register Remapping**:
When REMAP_ENABLE=1 and address bit 21 is set:
- Addresses 0x008-0x0FF are remapped to interleaved addresses at 0x400-0x4FF
- Allows optimized burst writes of triangle parameters
- Mapping defined in vid_voodoo_regs.h (SST_remap_* constants)

**Example Remap**:
```
Normal:  0x020 (startR)  → Remapped: 0x420 (0x020 | 0x400)
Normal:  0x024 (dRdX)    → Remapped: 0x424 (0x024 | 0x400)
```

---

### fbiInit4 (0x200) - Read/Write **[V1]**

**Function**: PCI read timing configuration.

**Bit Fields**:
```
Bit  0       : PCI_READ_WAIT_STATES - 0=1 wait state, 1=2 wait states
Bits [31:1]  : Reserved
```

**Read Behavior**: Returns current fbiInit4 value

**Write Behavior** (only when initEnable bit 0 is set):
1. Stores new value
2. Updates PCI read timing:
   ```c
   read_time = pci_nonburst + pci_burst * ((value & 1) ? 2 : 1);
   ```

**Usage**: Tuning PCI read performance for texture fetches

---

### fbiInit5 (0x244) - Read/Write **[V1]**

**Function**: Multi-chip and extended configuration.

**Bit Fields**:
```
Bits [13:0]  : Reserved
Bit  14      : MULTI_CVG - Multi-chip coverage (SLI mode for Voodoo 2)
Bits [31:15] : Reserved
```

**Read Behavior**: Returns fbiInit5 & ~0x1FF (lower 9 bits forced to 0)

**Write Behavior** (only when initEnable bit 0 is set):
```c
fbiInit5 = (value & ~0x41E6) | (fbiInit5 & 0x41E6);
```

**Multi-CVG**: Enables multi-chip rendering for Voodoo 2 SLI

---

### fbiInit6 (0x248) - Read/Write **[V1]**

**Function**: Extended initialization control.

**Bit Fields**:
```
Bits [7:0]   : Reserved
Bits [31:8]  : Reserved
Bit  30      : BLOCK_WIDTH_EXTEND - Extends block width calculation
```

**Read/Write Behavior**: Simple storage register

**Block Width**: Affects memory tile width calculation:
```c
block_width = ((fbiInit1 >> 4) & 15) * 2;
if (fbiInit6 & (1 << 30))
    block_width += 1;
```

---

### fbiInit7 (0x24C) - Read/Write **[V2+]**

**Function**: Command FIFO enable and configuration. **NOT AVAILABLE IN VOODOO 1.**

**Bit Fields**:
```
Bits [7:0]   : Reserved
Bit  8       : CMDFIFO_ENABLE - Enable command FIFO mode
Bits [31:9]  : Reserved
```

**Read Behavior**: Returns fbiInit7 & ~0xFF (lower 8 bits forced to 0)

**Write Behavior** (only when initEnable bit 0 is set):
1. Stores new value
2. Updates FIFO enabled state:
   ```c
   cmdfifo_enabled = value & 0x100;
   ```

**Command FIFO Mode**:
- When enabled, register writes go to FIFO at 0x200000-0x3FFFFF
- Allows asynchronous command buffering
- AGP-style DMA transfers
- See Command FIFO Registers section

---

### initEnable (PCI Config 0x40) - Read/Write

**Function**: Enable writes to initialization registers.

**Bit Fields**:
```
Bit  0       : ENABLE_INIT_WRITES - Allow writes to fbiInit* registers
Bit  1       : Reserved
Bit  2       : DAC_REGISTER_ACCESS - Enable DAC register read mode
Bits [31:3]  : Reserved
```

**Access**: Via PCI configuration space, not MMIO

**Behavior**:
- Acts as write-protect for critical initialization registers
- Software must set bit 0 before writing fbiInit*
- Prevents accidental reconfiguration during operation

**PCI Read** (addresses 0x40-0x43):
```c
case 0x40: return initEnable & 0xFF;
case 0x41: return (type == VOODOO_2) ?
                  0x50 | ((initEnable >> 8) & 0x0F) :
                  ((initEnable >> 8) & 0x0F);
case 0x42: return (initEnable >> 16) & 0xFF;
case 0x43: return (initEnable >> 24) & 0xFF;
```

**PCI Write**: Directly stores to initEnable register

---

## Display and Video Registers

### vRetrace (0x204) - Read Only

**Function**: Returns current video scan line number.

**Read Behavior**:
```c
return line & 0x1FFF;  // 13-bit line counter (0-8191)
```

**Write Behavior**: Ignored

**Usage**: Synchronizing software to display timing

---

### hvRetrace (0x240) - Read Only

**Function**: Returns both horizontal and vertical retrace positions.

**Read Behavior**:
```
Bits [12:0]  : VRETRACE - Vertical scan line (same as vRetrace)
Bits [15:13] : Reserved
Bits [31:16] : HRETRACE - Horizontal pixel position within scan line
```

**Horizontal Position Calculation**:
```c
line_time = configured_line_time_ticks;
current_time = timer_current_ticks;
time_since_line_start = current_time - line_start_time;
h_pos = h_total - 1 - (time_since_line_start * h_total / line_time);
if (h_pos >= h_total)
    h_pos = 0;
return (line & 0x1FFF) | (h_pos << 16);
```

**Usage**:
- Precise timing for raster effects
- Horizontal position allows cycle-accurate synchronization

---

### backPorch (0x208) - Write Only

**Function**: Configure display back porch timing.

**Bit Fields**: Implementation-specific timing values

**Write Behavior**: Stores back porch timing for video generation

---

### videoDimensions (0x20C) - Write Only

**Function**: Set display resolution.

**Bit Fields**:
```
Bits [11:0]  : H_DISP - Horizontal display width in pixels (minus 1)
Bits [15:12] : Reserved
Bits [27:16] : V_DISP - Vertical display height in lines
Bits [31:28] : Reserved
```

**Write Behavior**:
```c
voodoo->videoDimensions = value;
voodoo->h_disp = (value & 0xFFF) + 1;  // Actual width
voodoo->v_disp = (value >> 16) & 0xFFF;

// Quirk: Adjust certain resolutions
if (v_disp == 386 || v_disp == 402 ||
    v_disp == 482 || v_disp == 602)
    v_disp -= 2;
```

**Common Resolutions**:
- 640x480: H_DISP=639 (0x27F), V_DISP=480 (0x1E0)
- 800x600: H_DISP=799 (0x31F), V_DISP=600 (0x258)
- 1024x768: H_DISP=1023 (0x3FF), V_DISP=768 (0x300)

---

### hSync (0x220) - Write Only

**Function**: Configure horizontal sync timing.

**Bit Fields**:
```
Bits [7:0]   : H_SYNC_ON - Horizontal sync start
Bits [15:8]  : Reserved
Bits [25:16] : H_SYNC_OFF - Horizontal sync end
Bits [31:26] : Reserved
```

**Write Behavior**:
```c
voodoo->hSync = value;
voodoo->h_total = (value & 0xFF) + ((value >> 16) & 0x3FF);
voodoo_pixelclock_update(voodoo);
```

**Total Line Length**: Sum of sync_on and sync_off values

---

### vSync (0x224) - Write Only

**Function**: Configure vertical sync timing.

**Bit Fields**:
```
Bits [15:0]  : V_SYNC_ON - Vertical sync start (lines)
Bits [31:16] : V_SYNC_OFF - Vertical sync end (lines)
```

**Write Behavior**:
```c
voodoo->vSync = value;
voodoo->v_total = (value & 0xFFFF) + (value >> 16);
```

**Total Frame Height**: Sum of sync_on and sync_off values

---

### clutData (0x228) - Write Only

**Function**: Load color lookup table (CLUT) for gamma/color correction.

**Bit Fields**:
```
Bits [7:0]   : BLUE - Blue component (0-255)
Bits [15:8]  : GREEN - Green component (0-255)
Bits [23:16] : RED - Red component (0-255)
Bits [29:24] : INDEX - CLUT entry index (0-63)
Bit  30      : FORCE_WHITE - Force entry to white (255,255,255)
Bit  31      : Reserved
```

**Write Behavior**:
```c
index = (value >> 24) & 0x3F;
clutData[index].b = value & 0xFF;
clutData[index].g = (value >> 8) & 0xFF;
clutData[index].r = (value >> 16) & 0xFF;

if (value & 0x20000000) {  // FORCE_WHITE
    clutData[index].r = 255;
    clutData[index].g = 255;
    clutData[index].b = 255;
}
clutData_dirty = 1;  // Trigger CLUT interpolation
```

**CLUT Operation**:
- 33 entries (0-32) define control points
- Interpolated to 256-entry table for 8-bit RGB lookup
- Applied to final output for gamma correction
- Entry 0 = black, Entry 32 = white (typical)

**Interpolation** (done when clutData_dirty flag is set):
```c
for (c = 0; c < 256; c++) {
    clutData256[c].r = lerp(clutData[c/8], clutData[c/8+1], c%8);
    clutData256[c].g = lerp(clutData[c/8], clutData[c/8+1], c%8);
    clutData256[c].b = lerp(clutData[c/8], clutData[c/8+1], c%8);
}
```

---

### dacData (0x22C) - Write Only

**Function**: Access DAC (Digital-to-Analog Converter) registers and PLL.

**Bit Fields**:
```
Bits [7:0]   : DATA - Data value
Bits [10:8]  : REGISTER - DAC register select (0-7)
Bit  11      : READ_MODE - 1=read, 0=write
Bits [31:12] : Reserved
```

**Write Behavior**:

**Read Mode** (bit 11 = 1):
```c
dac_reg = (value >> 8) & 7;
dac_readdata = 0xFF;  // Default

if (dac_reg == 5) {  // PLL ID register
    switch (dac_data[7]) {  // Address register
        case 0x01: dac_readdata = 0x55; break;  // Manufacturer ID
        case 0x07: dac_readdata = 0x71; break;  // Device ID
        case 0x0B: dac_readdata = 0x79; break;  // Revision
    }
} else {
    dac_readdata = dac_data[dac_reg];
}
```

**Write Mode** (bit 11 = 0):
```c
dac_reg = (value >> 8) & 7;

if (dac_reg == 5) {  // PLL register access
    if (!dac_reg_ff) {
        // Low byte
        dac_pll_regs[dac_data[4] & 0xF] =
            (dac_pll_regs[dac_data[4] & 0xF] & 0xFF00) | (value & 0xFF);
    } else {
        // High byte
        dac_pll_regs[dac_data[4] & 0xF] =
            (dac_pll_regs[dac_data[4] & 0xF] & 0xFF) | ((value & 0xFF) << 8);
    }
    dac_reg_ff = !dac_reg_ff;  // Toggle byte select
    if (!dac_reg_ff)
        dac_data[4]++;  // Auto-increment address
} else {
    dac_data[dac_reg] = value & 0xFF;
    dac_reg_ff = 0;
}

voodoo_pixelclock_update(voodoo);  // Recalculate pixel clock
```

**DAC Register Map**:
- Register 4: PLL address
- Register 5: PLL data (16-bit, accessed as 2 bytes)
- Register 6: Output mode
- Register 7: PLL control address

**PLL Programming**:
```c
// Set pixel clock to ~25MHz for 640x480
dac_data[4] = 0;  // PLL register 0
write_dac(5, M_value & 0xFF);       // Low byte
write_dac(5, M_value >> 8);         // High byte
```

**Pixel Clock Calculation**:
```c
m = (dac_pll_regs[0] & 0x7F) + 2;         // Multiplier
n1 = ((dac_pll_regs[0] >> 8) & 0x1F) + 2; // Divider 1
n2 = ((dac_pll_regs[0] >> 13) & 0x07);    // Divider 2 (power of 2)

freq = (14.318MHz * m / n1) / (1 << n2);

// Divide by 2 for certain modes
if ((dac_data[6] & 0xF0) == 0x20 ||
    (dac_data[6] & 0xF0) == 0x60 ||
    (dac_data[6] & 0xF0) == 0x70)
    freq /= 2;
```

---

### scrFilter (0x230) - Write Only **[V2+]**

**Function**: Configure screen filter (edge anti-aliasing) thresholds. **NOT AVAILABLE IN VOODOO 1.**

**Bit Fields**:
```
Bits [7:0]   : BLUE_THRESHOLD - Blue edge threshold
Bits [15:8]  : GREEN_THRESHOLD - Green edge threshold
Bits [23:16] : RED_THRESHOLD - Red edge threshold
Bits [31:24] : Reserved
```

**Write Behavior**:
```c
if (initEnable & 0x01) {
    scrfilterEnabled = 1;
    scrfilterThreshold = value;

    if (value < 1)
        scrfilterEnabled = 0;

    voodoo_threshold_check(voodoo);  // Regenerate filter table
}
```

**Screen Filter**:
- Post-processing edge smoothing
- Blends adjacent pixels based on color difference
- Separate thresholds for R, G, B channels
- Higher threshold = more aggressive filtering
- Reduces jagged edges ("jaggies") in 3D graphics

**Filter Table Generation**:
- Creates lookup tables for pixel blending
- Based on color difference thresholds
- Different algorithms for Voodoo 1 vs Voodoo 2

---

## 2D Blitter Registers **[V2+]**

### Overview (Voodoo 2 and Banshee)

**NOT AVAILABLE IN VOODOO 1.** The 2D blitter provides hardware-accelerated screen-to-screen and CPU-to-screen copies with optional:
- Raster operations (ROP)
- Chroma keying (transparency)
- Color expansion (monochrome to color)
- Clipping

### bltSrcBaseAddr (0x2C0) - Write Only

**Function**: Set source memory base address for blits.

**Bit Fields**:
```
Bits [21:0]  : SRC_ADDR - Source address in framebuffer (byte address)
Bits [31:22] : Reserved
```

**Write Behavior**:
```c
bltSrcBaseAddr = value & 0x3FFFFF;
```

**Usage**: Starting address for source rectangle in framebuffer memory

---

### bltDstBaseAddr (0x2C4) - Write Only

**Function**: Set destination memory base address for blits.

**Bit Fields**:
```
Bits [21:0]  : DST_ADDR - Destination address (byte address)
Bits [31:22] : Reserved
```

**Write Behavior**:
```c
bltDstBaseAddr = value & 0x3FFFFF;
```

---

### bltXYStrides (0x2C8) - Write Only

**Function**: Set source and destination stride (bytes per scanline).

**Bit Fields**:
```
Bits [11:0]  : SRC_STRIDE - Source stride in bytes
Bits [15:12] : Reserved
Bits [27:16] : DST_STRIDE - Destination stride in bytes
Bits [31:28] : Reserved
```

**Write Behavior**:
```c
bltSrcXYStride = value & 0xFFF;
bltDstXYStride = (value >> 16) & 0xFFF;
```

**Usage**:
- Stride = bytes per line (typically width * bytes_per_pixel)
- Allows blitting from/to non-contiguous memory regions

---

### bltSrcChromaRange (0x2CC) - Write Only

**Function**: Define source chroma key (transparency) color range.

**Bit Fields**: RGB565 format, min and max colors
```
Bits [4:0]   : MIN_BLUE - Minimum blue (5 bits)
Bits [10:5]  : MIN_GREEN - Minimum green (6 bits)
Bits [15:11] : MIN_RED - Minimum red (5 bits)
Bits [20:16] : MAX_BLUE - Maximum blue (5 bits)
Bits [26:21] : MAX_GREEN - Maximum green (6 bits)
Bits [31:27] : MAX_RED - Maximum red (5 bits)
```

**Write Behavior**:
```c
bltSrcChromaRange = value;
bltSrcChromaMinB = value & 0x1F;
bltSrcChromaMinG = (value >> 5) & 0x3F;
bltSrcChromaMinR = (value >> 11) & 0x1F;
bltSrcChromaMaxB = (value >> 16) & 0x1F;
bltSrcChromaMaxG = (value >> 21) & 0x3F;
bltSrcChromaMaxR = (value >> 27) & 0x1F;
```

**Chroma Test**:
```c
if (src_color >= min_color && src_color <= max_color)
    skip_pixel;  // Transparent
```

---

### bltDstChromaRange (0x2D0) - Write Only

**Function**: Define destination chroma key range (same format as source).

---

### bltClipX (0x2D4) - Write Only

**Function**: Set horizontal clip window for blits.

**Bit Fields**:
```
Bits [11:0]  : CLIP_RIGHT - Right boundary
Bits [15:12] : Reserved
Bits [27:16] : CLIP_LEFT - Left boundary
Bits [31:28] : Reserved
```

**Write Behavior**:
```c
bltClipRight = value & 0xFFF;
bltClipLeft = (value >> 16) & 0xFFF;
```

---

### bltClipY (0x2D8) - Write Only

**Function**: Set vertical clip window for blits.

**Bit Fields**:
```
Bits [11:0]  : CLIP_BOTTOM - Bottom boundary
Bits [15:12] : Reserved
Bits [27:16] : CLIP_TOP - Top boundary
Bits [31:28] : Reserved
```

---

### bltSrcXY (0x2E0) - Write Only

**Function**: Set source rectangle top-left position.

**Bit Fields**:
```
Bits [10:0]  : SRC_X - Source X coordinate
Bits [15:11] : Reserved
Bits [26:16] : SRC_Y - Source Y coordinate
Bits [31:27] : Reserved
```

**Write Behavior**:
```c
bltSrcX = value & 0x7FF;
bltSrcY = (value >> 16) & 0x7FF;
```

---

### bltDstXY (0x2E4) - Write Only

**Function**: Set destination rectangle top-left position.

**Bit Fields**:
```
Bits [10:0]  : DST_X - Destination X coordinate
Bits [15:11] : Reserved
Bits [26:16] : DST_Y - Destination Y coordinate
Bits [30:27] : Reserved
Bit  31      : LAUNCH - Trigger blit operation
```

**Write Behavior**:
```c
bltDstX = value & 0x7FF;
bltDstY = (value >> 16) & 0x7FF;
if (value & (1 << 31))
    voodoo_v2_blit_start(voodoo);  // Start blit
```

**Auto-Launch**: Setting bit 31 immediately starts the blit

---

### bltSize (0x2E8) - Write Only

**Function**: Set blit rectangle width and height.

**Bit Fields**:
```
Bits [11:0]  : WIDTH - Blit width in pixels (signed, can be negative)
Bits [15:12] : Reserved
Bits [27:16] : HEIGHT - Blit height in lines (signed, can be negative)
Bits [30:28] : Reserved
Bit  31      : LAUNCH - Trigger blit operation
```

**Write Behavior**:
```c
bltSizeX = value & 0xFFF;
if (bltSizeX & 0x800)  // Sign extend
    bltSizeX |= 0xFFFFF000;

bltSizeY = (value >> 16) & 0xFFF;
if (bltSizeY & 0x800)  // Sign extend
    bltSizeY |= 0xFFFFF000;

if (value & (1 << 31))
    voodoo_v2_blit_start(voodoo);
```

**Negative Sizes**: Negative width/height flips blit direction

---

### bltRop (0x2EC) - Write Only

**Function**: Configure raster operations (pixel combining logic).

**Bit Fields**:
```
Bits [3:0]   : ROP0 - ROP for pattern=0, source=0
Bits [7:4]   : ROP1 - ROP for pattern=0, source=1
Bits [11:8]  : ROP2 - ROP for pattern=1, source=0
Bits [15:12] : ROP3 - ROP for pattern=1, source=1
Bits [31:16] : Reserved
```

**Write Behavior**:
```c
bltRop[0] = value & 0xF;
bltRop[1] = (value >> 4) & 0xF;
bltRop[2] = (value >> 8) & 0xF;
bltRop[3] = (value >> 12) & 0xF;
```

**ROP Operations**:
- 0x0 = 0 (black)
- 0x5 = NOT dest
- 0xA = source
- 0xC = source AND dest
- 0xE = source OR dest
- 0xF = 1 (white)

**Common ROPs**:
- SRCCOPY: 0xCCCC (copy source to dest)
- SRCPAINT: 0xEEEE (source OR dest)
- SRCAND: 0x8888 (source AND dest)
- SRCINVERT: 0x6666 (source XOR dest)

---

### bltColor (0x2F0) - Write Only

**Function**: Foreground and background colors for color expansion.

**Bit Fields**:
```
Bits [15:0]  : FG_COLOR - Foreground color (RGB565)
Bits [31:16] : BG_COLOR - Background color (RGB565)
```

**Write Behavior**:
```c
bltColorFg = value & 0xFFFF;
bltColorBg = (value >> 16) & 0xFFFF;
```

**Usage**:
- Color expansion (monochrome source to color dest)
- Bit 0 in source → BG_COLOR
- Bit 1 in source → FG_COLOR

---

### bltCommand (0x2F8) - Write Only

**Function**: Configure and launch blit operation.

**Bit Fields**:
```
Bits [13:0]  : Operation-specific flags
Bit  14      : SRC_TILED - Source uses tiled memory layout
Bit  15      : DST_TILED - Destination uses tiled memory layout
Bits [30:16] : Operation-specific flags
Bit  31      : LAUNCH - Trigger blit operation
```

**Write Behavior**:
```c
bltCommand = value;
if (value & (1 << 31))
    voodoo_v2_blit_start(voodoo);
```

**Tiled Memory**:
- Alternative to linear layout
- Improves cache locality for 2D operations
- Voodoo Banshee feature

---

### bltData (0x2FC) - Write Only

**Function**: Host data port for CPU-to-screen blits.

**Write Behavior**:
```c
voodoo_v2_blit_data(voodoo, value);
```

**Usage**:
1. Configure blit parameters (size, dest, ROP, etc.)
2. Write source pixel data to this register
3. Hardware performs blit as data arrives
4. Continues until bltSizeX * bltSizeY pixels written

**Example CPU-to-Screen Blit**:
```c
// Setup
write32(bltDstXY, (100 << 16) | 100);  // Dest (100,100)
write32(bltSize, (64 << 16) | 64);     // 64x64 pixels
write32(bltRop, 0xCCCC);               // SRCCOPY
write32(bltCommand, 0x80000000);       // Launch

// Stream pixel data
for (int i = 0; i < 64*64; i++)
    write32(bltData, pixel_data[i]);
```

---

## Command FIFO Registers **[V2+]**

### Overview (Voodoo 2 only)

**NOT AVAILABLE IN VOODOO 1.** Command FIFO allows asynchronous command buffering in main memory, similar to AGP. Instead of writing directly to registers, software:
1. Writes commands to memory FIFO
2. Hardware fetches and executes commands
3. Enables AGP-style DMA

**Enable**: Set fbiInit7 bit 8 (CMDFIFO_ENABLE)

**Memory Region**: 0x200000-0x3FFFFF (2MB FIFO space)

### cmdFifoBaseAddr (0x1E0) - Read/Write

**Function**: Configure FIFO base and end addresses.

**Bit Fields**:
```
Bits [9:0]   : BASE_ADDR - FIFO base address (in 4KB units)
Bits [15:10] : Reserved
Bits [25:16] : END_ADDR - FIFO end address (in 4KB units)
Bits [31:26] : Reserved
```

**Read Behavior**:
```c
return (cmdfifo_base >> 12) | ((cmdfifo_end >> 12) << 16);
```

**Write Behavior**:
```c
cmdfifo_base = (value & 0x3FF) << 12;      // Base in bytes
cmdfifo_end = ((value >> 16) & 0x3FF) << 12;  // End in bytes
```

**FIFO Size**: `cmdfifo_size = cmdfifo_end - cmdfifo_base`

---

### cmdFifoBump (0x1E4) - Write Only

**Function**: Bump FIFO write pointer (notify hardware of new commands).

**Write Behavior**: Implementation specific (advances write pointer)

**Usage**: After writing commands to FIFO memory, bump pointer to start execution

---

### cmdFifoRdPtr (0x1E8) - Read/Write

**Function**: FIFO read pointer (current execution position).

**Read Behavior**: Returns current read pointer offset

**Write Behavior**: Sets read pointer (typically for reset)

---

### cmdFifoAMin, cmdFifoAMax (0x1EC, 0x1F0) - Read/Write

**Function**: AGP aperture bounds for FIFO.

**Read/Write Behavior**: Stores AGP memory range for FIFO commands

---

### cmdFifoDepth (0x1F4) - Read/Write

**Function**: FIFO command depth counter.

**Read Behavior**: Returns pending command count
```c
return cmdfifo_depth_wr - cmdfifo_depth_rd;
```

**Write Behavior**: Reset counters
```c
cmdfifo_depth_rd = 0;
cmdfifo_depth_wr = value & 0xFFFF;
```

**Usage**: Software polls to check FIFO space available

---

### cmdFifoHoles (0x1F8) - Read Only

**Function**: Count of holes in FIFO (fragmentation).

**Read Behavior**: Returns hole count (implementation specific)

**Usage**: Diagnostics for FIFO management

---

## Texture Registers

### textureMode (0x300) - Write Only

**Function**: Configure texture filtering, format, and combine mode.

**Bit Fields**:
```
Bits [3:0]   : Reserved
Bits [7:4]   : Reserved
Bits [11:8]  : TEX_FORMAT - Texture format
               0x0 = RGB332 (8-bit)
               0x1 = Y4I2Q2 (8-bit YIQ)
               0x2 = Alpha 8
               0x3 = Intensity 8
               0x4 = Alpha + Intensity 8
               0x5 = Palette 8
               0x6 = Alpha palette 8
               0x8 = ARGB8332
               0x9 = A8 Y4I2Q2
               0xA = RGB565 (16-bit)
               0xB = ARGB1555 (16-bit)
               0xC = ARGB4444 (16-bit)
               0xD = Alpha + Intensity 16
               0xE = Alpha palette 88

Bit  12      : TC_ZERO_OTHER - Texture color zero other
Bit  13      : TC_SUB_CLOCAL - Texture color subtract local
Bits [16:14] : TC_MSELECT - Texture color multiply select
Bit  17      : TC_REVERSE_BLEND - Reverse texture blend
Bit  18      : TC_ADD_CLOCAL - Add local color
Bit  19      : TC_ADD_ALOCAL - Add local alpha
Bit  20      : TC_INVERT_OUTPUT - Invert texture color output
Bit  21      : TCA_ZERO_OTHER - Texture alpha zero other
Bit  22      : TCA_SUB_CLOCAL - Texture alpha subtract local
Bits [25:23] : TCA_MSELECT - Texture alpha multiply select
Bit  26      : TCA_REVERSE_BLEND - Reverse texture alpha blend
Bit  27      : TCA_ADD_CLOCAL - Add local alpha to result
Bit  28      : TCA_ADD_ALOCAL - Add local alpha
Bit  29      : TCA_INVERT_OUTPUT - Invert texture alpha output
Bit  30      : TRILINEAR - Enable trilinear filtering
Bits [4:0]   : NCC_SEL - NCC table select (bit 5)
Bit  6       : TCLAMPS - Clamp S coordinate
Bit  7       : TCLAMPT - Clamp T coordinate
```

**Write Behavior**:
```c
if (chip & CHIP_TREX0) {
    params.textureMode[0] = value;
    params.tformat[0] = (value >> 8) & 0xF;
    voodoo_recalc_tex(voodoo, 0);
}
if (chip & CHIP_TREX1) {
    params.textureMode[1] = value;
    params.tformat[1] = (value >> 8) & 0xF;
    voodoo_recalc_tex(voodoo, 1);
}
```

**Texture Combine**:
- Similar to fbzColorPath but for texture operations
- Allows detail textures, multi-texturing
- Separate color and alpha combiners

---

### tLOD (0x304) - Write Only

**Function**: Texture LOD (Level of Detail) configuration.

**Bit Fields**:
```
Bits [5:0]   : LOD_MIN - Minimum LOD level (0-8)
Bits [11:6]  : LOD_MAX - Maximum LOD level (0-8)
Bits [17:12] : LOD_BIAS - LOD bias (signed 6-bit)
Bit  18      : LOD_ODD - Odd LOD first
Bit  19      : LOD_SPLIT - Split LOD addressing
Bit  20      : LOD_S_IS_WIDER - S dimension is wider
Bits [23:21] : Reserved
Bit  24      : TMULTIBASEADDR - Multiple base addresses
Bits [27:25] : Reserved
Bit  28      : TMIRROR_S - Mirror S coordinate
Bit  29      : TMIRROR_T - Mirror T coordinate
Bits [31:30] : Reserved
```

**Write Behavior**:
```c
if (chip & CHIP_TREX0) {
    params.tLOD[0] = value;
    voodoo_recalc_tex(voodoo, 0);
}
if (chip & CHIP_TREX1) {
    params.tLOD[1] = value;
    voodoo_recalc_tex(voodoo, 1);
}
```

**LOD Selection**:
- Hardware computes LOD from texture coordinate gradients
- LOD_MIN/MAX clamp to prevent excessive blurring or aliasing
- LOD_BIAS shifts LOD calculation (sharpen/blur control)

**Mirroring**:
- TMIRROR_S: Reflects texture in S direction
- TMIRROR_T: Reflects texture in T direction
- Enables tiling without visible seams

---

### tDetail (0x308) - Write Only

**Function**: Detail texture parameters.

**Bit Fields**:
```
Bits [7:0]   : DETAIL_MAX - Maximum detail level
Bits [13:8]  : DETAIL_BIAS - Detail bias
Bits [16:14] : DETAIL_SCALE - Detail scale factor
Bits [31:17] : Reserved
```

**Write Behavior**:
```c
if (chip & CHIP_TREX0) {
    params.detail_max[0] = value & 0xFF;
    params.detail_bias[0] = (value >> 8) & 0x3F;
    params.detail_scale[0] = (value >> 14) & 7;
}
```

**Detail Texturing**:
- Adds high-frequency detail to base texture
- Detail texture modulates base texture
- Enhances perceived resolution

---

### texBaseAddr (0x30C) - Write Only

**Function**: Base address of LOD level 0 texture.

**Bit Fields**:
```
Bits [18:0]  : BASE_ADDR - Address in texture memory (Voodoo 1/2: 8-byte units)
Bits [23:0]  : BASE_ADDR - Address in texture memory (Banshee: 16-byte units)
```

**Write Behavior**:
```c
if (chip & CHIP_TREX0) {
    if (type >= VOODOO_BANSHEE)
        params.texBaseAddr[0] = value & 0xFFFFF0;
    else
        params.texBaseAddr[0] = (value & 0x7FFFF) << 3;  // Convert to bytes
    voodoo_recalc_tex(voodoo, 0);
}
```

**Texture Memory Layout**:
- Base address for largest MIP level (LOD 0)
- Smaller LOD levels stored sequentially after LOD 0
- Address calculation includes texture size and format

---

### texBaseAddr1, texBaseAddr2, texBaseAddr38 (0x310, 0x314, 0x318) - Write Only

**Function**: Base addresses for other LOD levels.

**LOD Mapping**:
- texBaseAddr1: LOD levels 1-2
- texBaseAddr2: LOD levels 3-4
- texBaseAddr38: LOD levels 5-8

**Usage**: When TMULTIBASEADDR is set, allows non-contiguous LOD storage

---

### trexInit1 (0x320) - Write Only

**Function**: TREX chip initialization.

**Write Behavior**: Stores initialization value per TMU

---

### NCC Table Registers

**Overview**: NCC (New Color Compression) provides YIQ color space compression for textures.

**Tables**: Two NCC tables (0 and 1), selected by textureMode NCC_SEL bit

**Components**:
- **Y (Luminance)**: 4 registers, each with 4 8-bit Y values
- **I (In-phase chroma)**: 4 registers, each with 9-bit signed I values
- **Q (Quadrature chroma)**: 4 registers, each with 9-bit signed Q values

### nccTable0_Y0-Y3 (0x324-0x330) - Write Only

**Function**: Luminance values for NCC table 0.

**Format**: Each register contains 4 luminance values
```
Bits [7:0]   : Y[0] for this register
Bits [15:8]  : Y[1] for this register
Bits [23:16] : Y[2] for this register
Bits [31:24] : Y[3] for this register
```

**Write Behavior**:
```c
if (chip & CHIP_TREX0) {
    voodoo->nccTable[0][0].y[register_index] = value;
    voodoo->ncc_dirty[0] = 1;  // Mark for update
}
```

**NCC Update**: When ncc_dirty flag is set, hardware regenerates lookup table:
```c
for (col = 0; col < 256; col++) {
    int y = (col >> 4);       // Upper 4 bits select Y
    int i = (col >> 2) & 3;   // Middle 2 bits select I
    int q = col & 3;          // Lower 2 bits select Q

    y_val = nccTable.y[y>>2][y&3];
    i_val = sign_extend_9bit(nccTable.i[i]);
    q_val = sign_extend_9bit(nccTable.q[q]);

    ncc_lookup[col].r = CLAMP(y_val + i_val + q_val);
    ncc_lookup[col].g = CLAMP(y_val + i_val_g + q_val_g);
    ncc_lookup[col].b = CLAMP(y_val + i_val_b + q_val_b);
}
```

---

### nccTable0_I0-I3 (0x334-0x340) - Write Only

**Function**: I (chroma) values for NCC table 0.

**Format**: Each register contains three 9-bit signed I components (RGB)
```
Bits [8:0]   : I_BLUE (9-bit signed)
Bits [17:9]  : I_GREEN (9-bit signed)
Bits [26:18] : I_RED (9-bit signed)
Bit  31      : PALETTE_MODE - If set, this write loads palette instead
```

**Write Behavior**:
```c
if (!(value & (1 << 31))) {  // Normal NCC mode
    if (chip & CHIP_TREX0) {
        nccTable[0][0].i[register_index] = value;
        ncc_dirty[0] = 1;
    }
} else {  // Palette load mode
    int palette_index = (value >> 23) & 0xFE;  // Even entries
    if (chip & CHIP_TREX0) {
        palette[0][palette_index].u = value | 0xFF000000;
        palette_dirty[0] = 1;
    }
}
```

**Dual Mode**: I/Q registers can also load palette data when bit 31 is set

---

### nccTable0_Q0-Q3 (0x344-0x350) - Write Only

**Function**: Q (chroma) values for NCC table 0 (same format as I registers).

**Palette Mode**: When bit 31 set, loads odd palette entries
```c
int palette_index = ((value >> 23) & 0xFE) | 0x01;  // Odd entries
```

---

### nccTable1_* (0x354-0x380) - Write Only

**Function**: NCC table 1 (same format as table 0).

**Usage**:
- Two tables allow fast switching between texture sets
- Select via textureMode NCC_SEL bit

---

## Setup Mode Registers **[Banshee+]**

### Overview

**NOT AVAILABLE IN VOODOO 1 OR VOODOO 2.** Setup mode provides a **vertex buffer interface** for triangle rendering. Instead of calculating and writing gradients manually, software writes vertex attributes and hardware computes gradients automatically.

**Modes**:
- Triangle strip
- Triangle fan
- Independent triangles

**Advantages**:
- Less CPU calculation
- Smaller command stream
- Hardware-optimized gradient computation

### sSetupMode (0x260) - Write Only **[Banshee+]**

**Function**: Configure setup mode operation flags. **NOT AVAILABLE IN VOODOO 1 OR VOODOO 2.**

**Bit Fields**:
```
Bit  0       : SETUP_RGB - Enable RGB interpolation
Bit  1       : SETUP_ALPHA - Enable alpha interpolation
Bit  2       : SETUP_Z - Enable Z interpolation
Bit  3       : SETUP_Wb - Enable W billboard interpolation
Bit  4       : SETUP_W0 - Enable W for TMU0
Bit  5       : SETUP_S0_T0 - Enable S,T for TMU0
Bit  6       : SETUP_W1 - Enable W for TMU1
Bit  7       : SETUP_S1_T1 - Enable S,T for TMU1
Bits [15:8]  : Reserved
Bit  16      : STRIP_MODE - 0=strip, 1=fan
Bit  17      : CULLING_ENABLE - Enable backface culling
Bit  18      : CULLING_SIGN - Cull sign (0=CCW, 1=CW)
Bit  19      : DISABLE_PINGPONG - Disable pingpong culling
Bits [31:20] : Reserved
```

**Write Behavior**: Stores setup flags for gradient computation

**Enabled Components**: Only components with corresponding bits set are interpolated

---

### sVx, sVy (0x264, 0x268) - Write Only

**Function**: Vertex X,Y screen coordinates (IEEE 754 float).

**Write Behavior**: Stores floating-point coordinate in vertex[3]
```c
tempif.i = value;
verts[3].sVx = tempif.f;  // Convert uint32 to float bits
```

**Coordinate System**: Same as fixed-point vertices (screen space, top-left origin)

---

### sARGB (0x26C) - Write Only

**Function**: Packed vertex color (8-bit components).

**Bit Fields**:
```
Bits [7:0]   : BLUE (0-255)
Bits [15:8]  : GREEN (0-255)
Bits [23:16] : RED (0-255)
Bits [31:24] : ALPHA (0-255)
```

**Write Behavior**:
```c
verts[3].sBlue = (float)(value & 0xFF);
verts[3].sGreen = (float)((value >> 8) & 0xFF);
verts[3].sRed = (float)((value >> 16) & 0xFF);
verts[3].sAlpha = (float)((value >> 24) & 0xFF);
```

**Usage**: Compact color specification (vs separate R,G,B,A registers)

---

### sRed, sGreen, sBlue, sAlpha (0x270-0x27C) - Write Only

**Function**: Individual vertex color components (IEEE 754 float).

**Write Behavior**: Stores float value (0.0-255.0 range)

**Usage**: High-precision color when needed

---

### sVz (0x280) - Write Only

**Function**: Vertex Z depth coordinate (IEEE 754 float).

**Range**: 0.0 (near) to 65535.0 (far) for 16-bit Z-buffer

---

### sWb (0x284) - Write Only

**Function**: Vertex W billboard value for perspective correction (IEEE 754 float).

**Usage**: 1/W for perspective-correct interpolation

---

### sW0, sS0, sT0 (0x288-0x290) - Write Only

**Function**: Texture coordinates and W for TMU 0.

**Format**: IEEE 754 float

**S,T Range**: Typically 0.0-256.0 for 256x256 texture

**W**: 1/W for perspective-correct texture mapping

---

### sW1, sS1, sT1 (0x294-0x29C) - Write Only

**Function**: Texture coordinates and W for TMU 1 (dual-TMU cards).

---

### sBeginTriCMD (0x2A4) - Write Only

**Function**: Begin triangle strip or fan.

**Write Behavior**:
1. Copies current vertex to vertex[0], [1], [2]
2. Resets vertex age tracking
3. Sets vertex count to 1
4. Resets pingpong culling state

**Usage**: Call once to start strip/fan, then write vertices and call sDrawTriCMD

---

### sDrawTriCMD (0x2A0) - Write Only

**Function**: Draw triangle with current and previous vertices.

**Write Behavior**:

**Vertex Management**:
```c
if (num_vertices < 3) {
    // Store vertex in next slot
    verts[vertex_num] = verts[3];
    vertex_ages[vertex_num] = vertex_next_age++;
    num_vertices++;
} else {
    // Replace oldest (strip) or second-oldest (fan)
    if (STRIP_MODE) {
        vertex_nr = find_oldest(vertex_ages);
    } else {  // Fan mode
        vertex_nr = find_second_oldest(vertex_ages);
    }
    verts[vertex_nr] = verts[3];
    vertex_ages[vertex_nr] = vertex_next_age++;
}

if (num_vertices >= 3) {
    voodoo_triangle_setup(voodoo);  // Compute gradients and render
    cull_pingpong = !cull_pingpong;
}
```

**Strip vs Fan**:
- **Strip**: Each triangle shares edge with previous (oldest vertex replaced)
- **Fan**: All triangles share first vertex (second-oldest replaced)

**Vertex Aging**: Tracks which vertices to replace using age stamps

**Triangle Setup**:
1. Sorts vertices by Y coordinate (top to bottom)
2. Computes gradients for all enabled attributes
3. Converts to fixed-point triangle registers
4. Calls voodoo_queue_triangle()

**Gradient Computation** (for RGB example):
```c
// Compute area
dxAB = verts[A].sVx - verts[B].sVx;
dxBC = verts[B].sVx - verts[C].sVx;
dyAB = verts[A].sVy - verts[B].sVy;
dyBC = verts[B].sVy - verts[C].sVy;
area = dxAB * dyBC - dxBC * dyAB;

// Normalize for 1/area
dxAB /= area;
dxBC /= area;
dyAB /= area;
dyBC /= area;

// Compute gradients
dRdX = ((vA.sRed - vB.sRed) * dyBC - (vB.sRed - vC.sRed) * dyAB) * 4096.0;
dRdY = ((vB.sRed - vC.sRed) * dxAB - (vA.sRed - vB.sRed) * dxBC) * 4096.0;
```

**Example Strip Rendering**:
```c
write32(sSetupMode, SETUPMODE_RGB | SETUPMODE_Z);  // Enable RGB, Z
write32(sVx, float_to_bits(100.0f));  // Vertex 0
write32(sVy, float_to_bits(100.0f));
write32(sRed, float_to_bits(255.0f));
write32(sVz, float_to_bits(0.0f));
write32(sBeginTriCMD, 0);  // Start strip

write32(sVx, float_to_bits(200.0f));  // Vertex 1
write32(sVy, float_to_bits(100.0f));
write32(sRed, float_to_bits(0.0f));
write32(sDrawTriCMD, 0);  // Not enough vertices yet

write32(sVx, float_to_bits(150.0f));  // Vertex 2
write32(sVy, float_to_bits(200.0f));
write32(sRed, float_to_bits(0.0f));
write32(sDrawTriCMD, 0);  // Triangle 0 rendered: (V0,V1,V2)

write32(sVx, float_to_bits(250.0f));  // Vertex 3
write32(sVy, float_to_bits(200.0f));
write32(sRed, float_to_bits(255.0f));
write32(sDrawTriCMD, 0);  // Triangle 1 rendered: (V1,V2,V3)
```

---

## Banshee-Specific Registers

### colBufferAddr (0x1EC) - Write Only (Banshee)

**Function**: Set color buffer base address.

**Bit Fields**:
```
Bits [23:4]  : COLOR_ADDR - Address aligned to 16 bytes
Bits [31:24] : Reserved
```

**Write Behavior**:
```c
params.draw_offset = value & 0xFFFFF0;
fb_write_offset = params.draw_offset;
```

---

### colBufferStride (0x1F0) - Write Only (Banshee)

**Function**: Color buffer stride and tiling mode.

**Bit Fields**:
```
Bits [13:0]  : STRIDE - Linear stride in bytes
Bits [6:0]   : STRIDE - Tiled stride in 128*32-byte units (when tiled)
Bit  14      : Reserved
Bit  15      : TILED - 1=tiled layout, 0=linear
Bits [31:16] : Reserved
```

**Write Behavior**:
```c
col_tiled = value & (1 << 15);
params.col_tiled = col_tiled;

if (col_tiled) {
    row_width = (value & 0x7F) * 128 * 32;  // Tiled
} else {
    row_width = value & 0x3FFF;  // Linear
}
params.row_width = row_width;
```

**Tiled Mode**: Memory organized in tiles for better cache coherency

---

### auxBufferAddr, auxBufferStride (0x1F4, 0x1F8) - Write Only (Banshee)

**Function**: Auxiliary buffer (Z/stencil) configuration (same format as color buffer).

---

### clipLeftRight1, clipTopBottom1 (0x200, 0x204) - Write Only (Banshee)

**Function**: Secondary clip window (for dual-window rendering).

**Format**: Same as clipLeftRight, clipLowYHighY

---

### swapPending (0x24C) - Read Only (Banshee)

**Function**: Returns 1 if buffer swap is pending, 0 if complete.

---

### leftOverlayBuf (0x250) - Write Only (Banshee)

**Function**: Overlay buffer address for video overlay.

**Write Behavior**: Stores address used by swapbufferCMD

---

## Summary Tables

### Register Access Summary

| Address Range | Category | Read | Write | Chips |
|---------------|----------|------|-------|-------|
| 0x000-0x004 | Status | Y | N | All |
| 0x008-0x0FC | Triangle Fixed | N | Y | All |
| 0x088-0x100 | Triangle Float | N | Y | All |
| 0x104-0x11C | Rendering Mode | Y | Y | All |
| 0x120-0x13C | Commands | N | Y | All |
| 0x140-0x148 | Constant Color | Y | Y | All |
| 0x14C-0x15C | Statistics | Y | N | All |
| 0x160-0x1DC | Fog Table | N | Y | All |
| 0x1E0-0x1FC | FIFO/Buffer | Y | Y | V2/Banshee |
| 0x200-0x24C | Init/Display | Mixed | Y | All |
| 0x260-0x2A4 | Setup Mode | N | Y | All |
| 0x2C0-0x2FC | 2D Blitter | N | Y | V2/Banshee |
| 0x300-0x380 | Texture/NCC | N | Y | All |

### Command Trigger Registers

| Register | Address | Function | Side Effects |
|----------|---------|----------|--------------|
| triangleCMD | 0x080 | Render triangle | Queue triangle, wake FIFO |
| ftriangleCMD | 0x100 | Render triangle (float) | Same as triangleCMD |
| nopCMD | 0x120 | No-op | Reset statistics |
| fastfillCMD | 0x124 | Fast rectangle fill | Fill framebuffer region |
| swapbufferCMD | 0x128 | Swap buffers | Swap front/back, sync vsync |
| sDrawTriCMD | 0x2A0 | Draw setup triangle | Compute gradients, render |
| sBeginTriCMD | 0x2A4 | Begin strip/fan | Reset vertex buffer |
| bltCommand | 0x2F8 | 2D blit | Start blit operation |

### Read-Only Registers

| Register | Address | Returns |
|----------|---------|---------|
| status | 0x000 | FIFO state, busy flags, vsync |
| fbiPixelsIn | 0x14C | Pixel input counter |
| fbiChromaFail | 0x150 | Chroma key reject counter |
| fbiZFuncFail | 0x154 | Depth test fail counter |
| fbiAFuncFail | 0x158 | Alpha test fail counter |
| fbiPixelsOut | 0x15C | Pixel output counter |
| vRetrace | 0x204 | Vertical scan line |
| hvRetrace | 0x240 | H and V position |
| cmdFifoDepth | 0x1F4 | Command FIFO depth |

---

## Implementation Notes

### Fixed-Point Formats

| Format | Integer Bits | Fractional Bits | Range | Precision |
|--------|--------------|-----------------|-------|-----------|
| 12.4 | 12 | 4 | ±2048 | 1/16 pixel |
| 12.12 | 12 | 12 | 0-4095 | 1/4096 |
| 14.18 | 14 | 18 | Large | 1/262144 |
| 2.30 | 2 | 30 | 0-4 | 1/1073741824 |

### Coordinate Systems

- **Screen Space**: 12.4 fixed-point, origin top-left
- **Texture Space**: S,T coordinates, typically 0.0-256.0 for 256x256 texture
- **Color Space**: 0.0-255.0 for components
- **Depth Space**: 0-65535 for 16-bit Z-buffer

### Performance Considerations

- **Register Write Order**: Write all parameters before command trigger
- **FIFO Usage**: Check status register before writing to avoid stalls
- **Batch Commands**: Use setup mode for vertex-heavy scenes
- **Buffer Swaps**: Use vsync mode for tear-free display
- **Texture Management**: Update texture parameters together, call recalc

### Common Operation Sequences

**Drawing a Triangle** (Fixed-Point):
```c
// 1. Set rendering mode
write32(0x104, fbzColorPath_value);
write32(0x110, fbzMode_value);

// 2. Write vertices
write32(0x008, vertex_Ax);
write32(0x00C, vertex_Ay);
write32(0x010, vertex_Bx);
write32(0x014, vertex_By);
write32(0x018, vertex_Cx);
write32(0x01C, vertex_Cy);

// 3. Write color parameters
write32(0x020, startR);
write32(0x024, startG);
write32(0x028, startB);
write32(0x040, dRdX);
write32(0x044, dGdX);
write32(0x048, dBdX);
write32(0x060, dRdY);
write32(0x064, dGdY);
write32(0x068, dBdY);

// 4. Trigger rendering
write32(0x080, 0);  // triangleCMD
```

**Buffer Swap**:
```c
// Wait for vsync
write32(0x128, (1 << 0) | (swap_interval << 1));

// Or immediate
write32(0x128, 0);
```

**Screen Clear**:
```c
write32(0x130, clear_color_and_depth);  // zaColor
write32(0x118, (0 << 16) | width);      // clipLeftRight
write32(0x11C, (0 << 16) | height);     // clipLowYHighY
write32(0x124, 0);                      // fastfillCMD
```

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-10-22 | Initial release - comprehensive register specification |

---

**End of Document**
