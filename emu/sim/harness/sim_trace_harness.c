/*
 * sim_trace_harness.c — Trace-capture implementation of sim_harness.h.
 *
 * Implements the same API as sim_harness.cpp but writes a binary trace
 * file instead of driving a Verilator model.  No Verilator dependency;
 * pure C, compiles in seconds.
 *
 * Register writes are shadowed so that subsequent reads (e.g. Glide's
 * init reading back fbiInit registers) return the written value.
 * The status register always reports idle / FIFO-free so that
 * sim_idle_wait() returns immediately.
 */

#include "sim_harness.h"
#include "voodoo_trace_format.h"
#include "voodoo_trace_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static voodoo_trace_writer_t writer;

/* Shadow register file — 1 K entries covers the 0x000-0x3FC register space.
 * Index = (addr >> 2) & 0xFF for the basic 256 registers.
 * We keep a generous 1024 entries to handle any stray aliased writes. */
#define REG_SHADOW_SIZE 1024
static uint32_t reg_shadow[REG_SHADOW_SIZE];

/* Status register layout (from sst.h):
 *   [5:0]   PCI FIFO free entries (0x3F = all free)
 *   [6]     vRetrace (toggled so grBufferSwap sees 0→1)
 *   [7]     FBI busy (0 = idle)
 *   [8]     TMU busy (0 = idle)
 *   [9]     Overall busy (0 = idle)
 *   [11:10] displayed buffer
 *   [27:12] Memory FIFO free level (0xFFFF = all free)
 *   [30:28] Swap buffers pending (0 = none)
 *   [31]    PCI interrupted
 */
#define STATUS_PCI_FIFO_FREE  0x3F
#define STATUS_MEMFIFO_FREE   (0xFFFFu << 12)
#define STATUS_VRETRACE       (1u << 6)
#define STATUS_IDLE           (STATUS_PCI_FIFO_FREE | STATUS_MEMFIFO_FREE)
static uint32_t status_read_count;

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int sim_init(void)
{
    const char *path = getenv("SIM_TRACE_FILE");
    if (!path)
        path = "trace.bin";

    voodoo_trace_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic       = VOODOO_TRACE_MAGIC;
    hdr.version     = 1;
    hdr.voodoo_type = 1;   /* VOODOO_1 */
    hdr.fb_size_mb  = 4;
    hdr.tex_size_mb = 4;
    hdr.num_tmus    = 1;

    memset(reg_shadow, 0, sizeof(reg_shadow));
    status_read_count = 0;

    if (trace_writer_open(&writer, path, &hdr) != 0) {
        fprintf(stderr, "[sim_trace] Failed to open trace file: %s\n", path);
        return -1;
    }

    fprintf(stderr, "[sim_trace] Trace capture to %s\n", path);
    return 0;
}

void sim_shutdown(void)
{
    trace_writer_close(&writer);
    fprintf(stderr, "[sim_trace] Closed (%lu entries)\n",
            (unsigned long)writer.entry_count);
}

void sim_write(uint32_t addr, uint32_t data)
{
    uint8_t cmd_type;

    if (addr < 0x400000) {
        cmd_type = VOODOO_TRACE_WRITE_REG_L;
        /* Shadow register value for later reads */
        uint32_t idx = (addr >> 2) & (REG_SHADOW_SIZE - 1);
        reg_shadow[idx] = data;
    } else if (addr < 0x800000) {
        cmd_type = VOODOO_TRACE_WRITE_FB_L;
    } else {
        cmd_type = VOODOO_TRACE_WRITE_TEX_L;
    }

    trace_writer_record(&writer, cmd_type, addr, data);
}

uint32_t sim_read(uint32_t addr)
{
    /* Status register: idle, with vRetrace toggling every 64 reads
     * so that grBufferSwap() detects a 0→1 transition. */
    if (addr == 0) {
        uint32_t v = STATUS_IDLE;
        if ((status_read_count++ >> 6) & 1)
            v |= STATUS_VRETRACE;
        return v;
    }

    /* Register reads: return shadow value */
    if (addr < 0x400000) {
        uint32_t idx = (addr >> 2) & (REG_SHADOW_SIZE - 1);
        return reg_shadow[idx];
    }

    /* LFB / texture reads: record in trace, return 0 */
    uint8_t cmd_type = (addr < 0x800000) ? VOODOO_TRACE_READ_FB_L
                                         : VOODOO_TRACE_READ_REG_L;
    trace_writer_record(&writer, cmd_type, addr, 0);
    return 0;
}

uint32_t sim_idle_wait(void)
{
    return STATUS_IDLE;
}

void sim_tick(int n)
{
    (void)n; /* no-op */
}
