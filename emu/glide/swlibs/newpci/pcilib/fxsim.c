/*
** Simulation platform backend for SpinalVoodoo Verilator bridge.
** Replaces fxlinux.c when building with -DSIM_BACKEND.
**
** Direct in-process Verilator model via sim_harness.h.
** PCI config space is stubbed to make Glide's init sequence succeed.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3dfx.h>
#include <fxmemmap.h>
#include "fxpci.h"
#include "pcilib.h"
#include "fxsim.h"
#include "sim_harness.h"

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static int simInitialized = 0;

/* 16 MB local buffer that Glide thinks is the mmap'd SST1 address space.
 * Layout: 0x000000-0x3FFFFF = registers (4MB)
 *         0x400000-0x7FFFFF = LFB (4MB)
 *         0x800000-0xFFFFFF = texture (8MB)
 */
#define SIM_MEMSIZE (16 * 1024 * 1024)
static char simMem[SIM_MEMSIZE] __attribute__((aligned(4096)));
static void *simBasePtr = simMem;

/* PCI config space shadow (256 bytes) */
static FxU32 pciConfigSpace[64];

/* ------------------------------------------------------------------ */
/* PCI config space stub                                               */
/* ------------------------------------------------------------------ */

static void initPciConfigSpace(void) {
    memset(pciConfigSpace, 0, sizeof(pciConfigSpace));

    /* Vendor ID = 3dfx (0x121A), Device ID = Voodoo1 (0x0001) */
    pciConfigSpace[0] = 0x0001121A; /* [15:0]=vendor, [31:16]=device */

    /* Command = memory access enabled */
    pciConfigSpace[1] = 0x00000002; /* [15:0]=command, [31:16]=status */

    /* Revision ID = 2, Class = display controller */
    pciConfigSpace[2] = 0x04000002; /* [7:0]=rev, [31:8]=class */

    /* BAR0 = fake physical address (will be used by pciMapLinearSim) */
    pciConfigSpace[4] = 0xF0000000;

    /* SST1_PCI_INIT_ENABLE (offset 0x40) */
    pciConfigSpace[16] = 0x00000000;

    /* SST1_PCI_BUS_SNOOP_0 (offset 0x44) */
    pciConfigSpace[17] = 0x00000000;

    /* SST1_PCI_BUS_SNOOP_1 (offset 0x48) */
    pciConfigSpace[18] = 0x00000000;
}

/* ------------------------------------------------------------------ */
/* FxPlatformIOProcs implementation                                    */
/* ------------------------------------------------------------------ */

static const char*
pciIdentifySim(void) {
    return "fxPCI for SpinalVoodoo Simulation (in-process Verilator)";
}

static FxBool
pciInitializeSim(void) {
    initPciConfigSpace();
    if (sim_init() < 0) {
        pciErrorCode = PCI_ERR_NO_IO_PERM;
        return FXFALSE;
    }
    simInitialized = 1;
    return FXTRUE;
}

static FxBool
pciShutdownSim(void) {
    if (simInitialized) {
        /* Auto-dump framebuffer if SIM_DUMP_PREFIX is set */
        const char *dumpPrefix = getenv("SIM_DUMP_PREFIX");
        if (dumpPrefix && dumpPrefix[0]) {
            char path[512];
            snprintf(path, sizeof(path), "%s_front.ppm", dumpPrefix);
            simDumpFB(path, 0, 640, 480);
            snprintf(path, sizeof(path), "%s_back.ppm", dumpPrefix);
            simDumpFB(path, 1, 640, 480);
            fprintf(stderr, "[fxsim] Dumped framebuffer to %s_{front,back}.ppm\n", dumpPrefix);
        }

        sim_shutdown();
        simInitialized = 0;
    }
    return FXTRUE;
}

/* Port IO: Stub PCI config space mechanism 1 access */

/* PCI config mechanism 1 state */
static FxU32 configAddress = 0;

static FxU8
pciPortInByteSim(FxU16 port) {
    return 0;
}

static FxU16
pciPortInWordSim(FxU16 port) {
    return 0;
}

static FxU32
pciPortInLongSim(FxU16 port) {
    if (port == 0xCF8) {
        /* CONFIG_ADDRESS_PORT - return last written config address */
        return configAddress;
    } else if (port == 0xCFC) {
        /* CONFIG_DATA_PORT - return PCI config data */
        FxU32 devNum = (configAddress >> 11) & 0x1F;
        FxU32 busNum = (configAddress >> 16) & 0xFF;
        FxU32 funcNum = (configAddress >> 8) & 0x7;
        if (busNum == 0 && devNum == 0 && funcNum == 0) {
            FxU32 regOffset = configAddress & 0xFC;
            FxU32 idx = regOffset >> 2;
            if (idx < 64) {
                return pciConfigSpace[idx];
            }
        }
        return 0xFFFFFFFF; /* No device present */
    }
    return 0xFFFFFFFF;
}

static FxBool
pciPortOutByteSim(FxU16 port, FxU8 data) {
    return FXTRUE;
}

static FxBool
pciPortOutWordSim(FxU16 port, FxU16 data) {
    return FXTRUE;
}

static FxBool
pciPortOutLongSim(FxU16 port, FxU32 data) {
    if (port == 0xCF8) {
        configAddress = data;
    } else if (port == 0xCFC) {
        FxU32 devNum = (configAddress >> 11) & 0x1F;
        FxU32 busNum = (configAddress >> 16) & 0xFF;
        FxU32 funcNum = (configAddress >> 8) & 0x7;
        if (busNum == 0 && devNum == 0 && funcNum == 0) {
            FxU32 regOffset = configAddress & 0xFC;
            FxU32 idx = regOffset >> 2;
            if (idx < 64) {
                pciConfigSpace[idx] = data;
            }
        }
    }
    return FXTRUE;
}

static FxBool
pciMapLinearSim(FxU32 busNumber, FxU32 physAddr,
                unsigned long *linearAddr, FxU32 *length) {
    /* Return pointer to our local simMem buffer */
    *linearAddr = (unsigned long)simMem;
    /* Ensure at least 16MB mapped */
    if (*length < SIM_MEMSIZE) *length = SIM_MEMSIZE;
    fprintf(stderr, "[fxsim] pciMapLinear: physAddr=0x%08x -> linearAddr=%p, length=0x%x\n",
            physAddr, (void *)*linearAddr, *length);
    return FXTRUE;
}

static FxBool
pciUnmapLinearSim(unsigned long linearAddr, FxU32 length) {
    return FXTRUE;
}

static FxBool
pciSetPermissionSim(const unsigned long addrBase, const FxU32 addrLen,
                    const FxBool writePermP) {
    return FXTRUE;
}

static FxBool
pciMsrGetSim(MSRInfo *in, MSRInfo *out) {
    return FXTRUE;
}

static FxBool
pciMsrSetSim(MSRInfo *in, MSRInfo *out) {
    return FXTRUE;
}

static FxBool
pciOutputStringSim(const char *msg) {
    fprintf(stderr, "[fxsim] %s", msg);
    return FXTRUE;
}

static FxBool
pciSetPassThroughBaseSim(FxU32 *baseAddr, FxU32 baseAddrLen) {
    return FXTRUE;
}

const FxPlatformIOProcs ioProcsSim = {
    pciInitializeSim,
    pciShutdownSim,
    pciIdentifySim,

    pciPortInByteSim,
    pciPortInWordSim,
    pciPortInLongSim,

    pciPortOutByteSim,
    pciPortOutWordSim,
    pciPortOutLongSim,

    pciMapLinearSim,
    pciUnmapLinearSim,
    pciSetPermissionSim,

    pciMsrGetSim,
    pciMsrSetSim,

    pciOutputStringSim,
    pciSetPassThroughBaseSim
};

FxBool
pciPlatformInit(void) {
    gCurPlatformIO = &ioProcsSim;
    return FXTRUE;
}

/* ------------------------------------------------------------------ */
/* Also stub the Linux-specific functions that fxpci.c calls           */
/* ------------------------------------------------------------------ */

FxBool hasDev3DfxLinux(void) {
    return FXFALSE; /* Force generic PCI scan path */
}

int getNumDevicesLinux(void) {
    return 0; /* Not used when hasDev3DfxLinux returns false */
}

FxU32 pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device) {
    return 0;
}

int pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device) {
    return 0;
}

/* ------------------------------------------------------------------ */
/* SET/GET/SETF intercept functions                                    */
/* ------------------------------------------------------------------ */

void simSetBasePtr(void *ptr) {
    simBasePtr = ptr;
}

void *simGetBasePtr(void) {
    return simBasePtr;
}

static int simDebugLevel = -1; /* -1 = uninitialized */

static int simGetDebugLevel(void) {
    if (simDebugLevel < 0) {
        const char *env = getenv("SIM_DEBUG");
        simDebugLevel = env ? atoi(env) : 0;
    }
    return simDebugLevel;
}

/* ------------------------------------------------------------------ */
/* Texture write buffering                                             */
/* ------------------------------------------------------------------ */
/* Texture writes are buffered locally and flushed in bulk before any  */
/* command that triggers GPU texture reads (triangleCMD, fastfillCMD). */
/* We track dirty 4KB pages to minimize per-word sim_write calls.      */

#define TEX_MEM_SIZE     (8 * 1024 * 1024)  /* 8MB texture space */
#define TEX_PAGE_SHIFT   12
#define TEX_PAGE_SIZE    (1 << TEX_PAGE_SHIFT) /* 4KB */
#define TEX_NUM_PAGES    (TEX_MEM_SIZE / TEX_PAGE_SIZE) /* 2048 */

static unsigned char texDirtyPages[TEX_NUM_PAGES];
static int texHasDirty = 0;

static void simFlushTexture(void) {
    if (!texHasDirty || !simInitialized) return;

    /* Find contiguous runs of dirty pages and flush each run */
    int totalBytes = 0;
    int numRuns = 0;
    int i = 0;
    while (i < TEX_NUM_PAGES) {
        if (!texDirtyPages[i]) { i++; continue; }

        /* Find end of this dirty run */
        int start = i;
        while (i < TEX_NUM_PAGES && texDirtyPages[i]) {
            texDirtyPages[i] = 0;
            i++;
        }

        /* Write each word in the dirty range using the raw PCI texture aperture
         * layout. Address translation into packed TMU SRAM now lives in shared
         * RTL (Core.scala), so fxsim must not relocate or deinterleave here. */
        FxU32 offset = (FxU32)start << TEX_PAGE_SHIFT;
        FxU32 length = (FxU32)(i - start) << TEX_PAGE_SHIFT;
        FxU32 *data = (FxU32 *)(simMem + 0x800000 + offset);
        FxU32 words = length / 4;
        FxU32 j;
        for (j = 0; j < words; j++) {
            FxU32 texOffset = offset + j * 4;
            sim_write(0x800000 | texOffset, data[j]);
        }

        totalBytes += length;
        numRuns++;
    }

    fprintf(stderr, "[fxsim] FLUSH TEX %d runs, %d bytes total\n",
            numRuns, totalBytes);

    texHasDirty = 0;
}

void simWriteReg(volatile void *addr, FxU32 value) {
    FxU32 offset = (FxU32)((char *)addr - (char *)simBasePtr);
    if (simGetDebugLevel() >= 2) {
        fprintf(stderr, "[fxsim] WRITE offset=0x%08x value=0x%08x\n", offset, value);
    }

    /* Also write to local shadow so subsequent reads see the value */
    *(volatile FxU32 *)addr = value;

    if (!simInitialized) return;

    if (offset < 0x400000) {
        /* Register space: 0x000000-0x3FFFFF
         * Glide's Sstregs struct places TMU0 registers at offset 0x800-0xBFF.
         * SpinalVoodoo maps all registers in a flat 10-bit (0x000-0x3FF) space:
         *   FBI registers: 0x000-0x2FF, TMU registers: 0x300-0x3FF.
         * Folding with & 0x3FF maps TMU0 textureMode (0xB00) to 0x300, etc. */
        FxU32 regOff = offset & 0x3FF;

        /* Flush pending texture data before any command that triggers
         * GPU rendering or texture reads */
        if (regOff == 0x120 || regOff == 0x124 || /* nopCMD, fastfillCMD */
            regOff == 0x080 || regOff == 0x100 ||  /* triangleCMD, ftriangleCMD */
            regOff == 0x128) {                     /* swapbufferCMD */
            simFlushTexture();
        }

        /* Flush pending raw texture aperture writes before texBaseAddr changes so
         * Core sees the command stream in-order relative to later draws/reads. */
        if (regOff == 0x30C) {
            simFlushTexture();
        }

        /* Wait for pipeline drain before swapbuffer */
        if (regOff == 0x128) {
            sim_idle_wait();
        }

        sim_write(regOff, value);
    } else if (offset < 0x800000) {
        /* LFB space: 0x400000-0x7FFFFF */
        FxU32 lfbAddr = offset & 0x3FFFFF;
        sim_write(0x400000 + lfbAddr, value);
    } else {
        /* Texture space: 0x800000-0xFFFFFF
         * Buffer locally, flush before next rendering command.
         */
        static int texWriteCount = 0;
        static int texNonZeroCount = 0;
        static FxU32 texMinOffset = 0xFFFFFFFF;
        static FxU32 texMaxOffset = 0;
        texWriteCount++;
        if (value != 0) texNonZeroCount++;
        FxU32 texOff = offset - 0x800000;
        if (texOff < texMinOffset) texMinOffset = texOff;
        if (texOff > texMaxOffset) texMaxOffset = texOff;
        if (texNonZeroCount <= 5 && value != 0) {
            fprintf(stderr, "[fxsim] TEX WRITE (non-zero #%d) offset=0x%08x texOff=0x%06x value=0x%08x\n",
                    texNonZeroCount, offset, texOff, value);
        }
        if (texWriteCount == 1 || texWriteCount == 100 || texWriteCount == 1000 || texWriteCount == 5000) {
            fprintf(stderr, "[fxsim] TEX WRITE stats at #%d: %d non-zero, range 0x%06x-0x%06x\n",
                    texWriteCount, texNonZeroCount, texMinOffset, texMaxOffset);
        }
        FxU32 texOffset = offset & 0x7FFFFF;
        FxU32 page = texOffset >> TEX_PAGE_SHIFT;
        if (page < TEX_NUM_PAGES) {
            texDirtyPages[page] = 1;
            /* Mark next page too if write crosses page boundary */
            if (((texOffset + 3) >> TEX_PAGE_SHIFT) != page &&
                ((texOffset + 3) >> TEX_PAGE_SHIFT) < TEX_NUM_PAGES) {
                texDirtyPages[(texOffset + 3) >> TEX_PAGE_SHIFT] = 1;
            }
            texHasDirty = 1;
        }
    }
}

FxU32 simReadReg(volatile void *addr) {
    FxU32 offset = (FxU32)((char *)addr - (char *)simBasePtr);

    if (!simInitialized) {
        return *(volatile FxU32 *)addr;
    }

    if (offset < 0x400000) {
        /* Register space - read from simulation (10-bit folded address) */
        FxU32 regOff = offset & 0x3FF;
        FxU32 value;
        if (regOff == 0) {
            /* Status register: just read via BMB — the bus_read
             * advances the clock, no extra ticks needed. */
            value = sim_read(0);
        } else {
            value = sim_read(regOff);
        }
        if (simGetDebugLevel() >= 2) {
            fprintf(stderr, "[fxsim] READ  offset=0x%08x value=0x%08x\n", offset, value);
        }
        /* Update local shadow */
        *(volatile FxU32 *)addr = value;
        return value;
    } else if (offset < 0x800000) {
        /* LFB read */
        FxU32 lfbAddr = offset & 0x3FFFFF;
        FxU32 value = sim_read(0x400000 + lfbAddr);
        *(volatile FxU32 *)addr = value;
        return value;
    } else {
        /* Texture space read - return local shadow */
        return *(volatile FxU32 *)addr;
    }
}

void simWriteRegF(volatile void *addr, float value) {
    /* Reinterpret float bits as FxU32 */
    FxU32 ivalue;
    memcpy(&ivalue, &value, sizeof(ivalue));
    simWriteReg(addr, ivalue);
}

void simWriteTex(FxU32 offset, const void *data, FxU32 length) {
    if (!simInitialized) return;

    /* Also copy to local shadow */
    if (offset + length <= SIM_MEMSIZE - 0x800000) {
        memcpy(simMem + 0x800000 + offset, data, length);
    }

    /* Write each word to texture memory via unified bus */
    const FxU32 *words = (const FxU32 *)data;
    FxU32 numWords = length / 4;
    FxU32 i;
    for (i = 0; i < numWords; i++) {
        sim_write(0x800000 + offset + i * 4, words[i]);
    }
}

void simIdleWait(void) {
    if (!simInitialized) return;
    sim_idle_wait();
}

void simDumpFB(const char *filename, FxU32 buffer, FxU32 width, FxU32 height) {
    if (!simInitialized) return;

    /* Set lfbMode readBufferSelect (bits [7:6]) to select front/back buffer.
     * Register 0x114 = lfbMode. We preserve other fields by reading first. */
    FxU32 lfbMode = sim_read(0x114);
    lfbMode = (lfbMode & ~(0x3u << 6)) | ((buffer & 0x3) << 6);
    sim_write(0x114, lfbMode);

    /* Read framebuffer via LFB read path.
     * LFB read address: 0x400000 | (y << 11) | (x << 1)
     * Each 32-bit read returns two RGB565 pixels: lo16=pixel(x), hi16=pixel(x+1) */
    FILE *f = fopen(filename, "wb");
    if (!f) return;

    fprintf(f, "P6\n%d %d\n255\n", width, height);
    FxU32 y, x;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2) {
            FxU32 addr = 0x400000 | (y << 11) | (x << 1);
            FxU32 pair = sim_read(addr);

            /* Two pixels packed: lo16=pixel(x), hi16=pixel(x+1) */
            FxU16 pixels[2];
            pixels[0] = (FxU16)(pair & 0xFFFF);
            pixels[1] = (FxU16)(pair >> 16);

            int i;
            for (i = 0; i < 2 && (x + i) < width; i++) {
                FxU16 rgb565 = pixels[i];
                unsigned char r = ((rgb565 >> 11) & 0x1F) << 3;
                unsigned char g = ((rgb565 >> 5) & 0x3F) << 2;
                unsigned char b = (rgb565 & 0x1F) << 3;
                r |= r >> 5;
                g |= g >> 6;
                b |= b >> 5;
                fputc(r, f);
                fputc(g, f);
                fputc(b, f);
            }
        }
    }
    fclose(f);

    fprintf(stderr, "[fxsim] Dumped framebuffer to %s (%dx%d, buf%d)\n",
            filename, width, height, buffer);
}
