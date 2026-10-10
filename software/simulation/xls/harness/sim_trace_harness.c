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
#include <signal.h>
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
static uint32_t swap_count;
static int shutdown_registered;
static int shutdown_done;

#define FB_WORD_COUNT   (4 * 1024 * 1024 / 4)
#define TEX_WORD_COUNT  (8 * 1024 * 1024 / 4)

static uint32_t *fb_shadow;
static uint32_t *tex_shadow;

static uint32_t shadow_read32(const uint32_t *shadow, uint32_t word_count, uint32_t addr)
{
    uint32_t idx = (addr >> 2);
    if (idx >= word_count) return 0;

    if (addr & 0x2u)
        return (shadow[idx] >> 16) & 0xFFFFu;
    return shadow[idx];
}

static void shadow_write32(uint32_t *shadow, uint32_t word_count, uint32_t addr, uint32_t data)
{
    uint32_t idx = (addr >> 2);
    if (idx >= word_count) return;

    if (addr & 0x2u) {
        shadow[idx] = (shadow[idx] & 0x0000FFFFu) | ((data & 0xFFFFu) << 16);
    } else {
        shadow[idx] = data;
    }
}

static void shadow_write16(uint32_t *shadow, uint32_t word_count, uint32_t addr, uint16_t data)
{
    uint32_t idx = (addr >> 2);
    if (idx >= word_count) return;

    if (addr & 0x2u) {
        shadow[idx] = (shadow[idx] & 0x0000FFFFu) | ((uint32_t)data << 16);
    } else {
        shadow[idx] = (shadow[idx] & 0xFFFF0000u) | (uint32_t)data;
    }
}

static void sim_trace_shutdown_atexit(void)
{
    sim_shutdown();
}

static void sim_trace_signal_handler(int sig)
{
    trace_writer_finalize_signal_safe(&writer);
    signal(sig, SIG_DFL);
    raise(sig);
}

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
    swap_count = 0;
    shutdown_done = 0;

    if (!shutdown_registered) {
        atexit(sim_trace_shutdown_atexit);
        signal(SIGINT, sim_trace_signal_handler);
        signal(SIGTERM, sim_trace_signal_handler);
        signal(SIGHUP, sim_trace_signal_handler);
        signal(SIGQUIT, sim_trace_signal_handler);
        shutdown_registered = 1;
    }

    if (!fb_shadow) {
        fb_shadow = (uint32_t *)calloc(FB_WORD_COUNT, sizeof(uint32_t));
        if (!fb_shadow) {
            fprintf(stderr, "[sim_trace] Failed to allocate framebuffer shadow\n");
            return -1;
        }
    } else {
        memset(fb_shadow, 0, FB_WORD_COUNT * sizeof(uint32_t));
    }

    if (!tex_shadow) {
        tex_shadow = (uint32_t *)calloc(TEX_WORD_COUNT, sizeof(uint32_t));
        if (!tex_shadow) {
            fprintf(stderr, "[sim_trace] Failed to allocate texture shadow\n");
            free(fb_shadow);
            fb_shadow = NULL;
            return -1;
        }
    } else {
        memset(tex_shadow, 0, TEX_WORD_COUNT * sizeof(uint32_t));
    }

    if (trace_writer_open(&writer, path, &hdr) != 0) {
        fprintf(stderr, "[sim_trace] Failed to open trace file: %s\n", path);
        return -1;
    }

    fprintf(stderr, "[sim_trace] Trace capture to %s\n", path);
    return 0;
}

void sim_shutdown(void)
{
    if (shutdown_done)
        return;
    shutdown_done = 1;

    trace_writer_close(&writer);
    if (writer.entry_count || writer.file == NULL) {
        fprintf(stderr, "[sim_trace] Closed (%lu entries)\n",
                (unsigned long)writer.entry_count);
    }

    free(fb_shadow);
    fb_shadow = NULL;
    free(tex_shadow);
    tex_shadow = NULL;
}

void sim_write(uint32_t addr, uint32_t data)
{
    uint8_t cmd_type;

    if (addr < 0x400000) {
        cmd_type = VOODOO_TRACE_WRITE_REG_L;
        /* Shadow register value for later reads */
        uint32_t idx = (addr >> 2) & (REG_SHADOW_SIZE - 1);
        reg_shadow[idx] = data;
        if ((addr & 0x3FCu) == 0x128u)
            trace_writer_record(&writer, VOODOO_TRACE_SWAP, swap_count, data);
    } else if (addr < 0x800000) {
        cmd_type = VOODOO_TRACE_WRITE_FB_L;
        shadow_write32(fb_shadow, FB_WORD_COUNT, addr - 0x400000u, data);
    } else {
        cmd_type = VOODOO_TRACE_WRITE_TEX_L;
        shadow_write32(tex_shadow, TEX_WORD_COUNT, addr - 0x800000u, data);
    }

    trace_writer_record(&writer, cmd_type, addr, data);
}

void sim_write16(uint32_t addr, uint16_t data)
{
    uint8_t cmd_type;

    if (addr < 0x400000) {
        uint32_t idx = (addr >> 2) & (REG_SHADOW_SIZE - 1);
        if (addr & 0x2u)
            reg_shadow[idx] = (reg_shadow[idx] & 0x0000FFFFu) | ((uint32_t)data << 16);
        else
            reg_shadow[idx] = (reg_shadow[idx] & 0xFFFF0000u) | (uint32_t)data;
        cmd_type = VOODOO_TRACE_WRITE_REG_W;
        if ((addr & 0x3FCu) == 0x128u)
            trace_writer_record(&writer, VOODOO_TRACE_SWAP, swap_count, data);
    } else if (addr < 0x800000) {
        shadow_write16(fb_shadow, FB_WORD_COUNT, addr - 0x400000u, data);
        cmd_type = VOODOO_TRACE_WRITE_FB_W;
    } else {
        shadow_write16(tex_shadow, TEX_WORD_COUNT, addr - 0x800000u, data);
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

    if (addr < 0x800000) {
        uint32_t value = shadow_read32(fb_shadow, FB_WORD_COUNT, addr - 0x400000u);
        trace_writer_record(&writer,
                            (addr & 0x2u) ? VOODOO_TRACE_READ_FB_W : VOODOO_TRACE_READ_FB_L,
                            addr,
                            value);
        return value;
    }

    {
        uint32_t value = shadow_read32(tex_shadow, TEX_WORD_COUNT, addr - 0x800000u);
        trace_writer_record(&writer, VOODOO_TRACE_READ_REG_L, addr, value);
        return value;
    }
}

uint32_t sim_idle_wait(void)
{
    return STATUS_IDLE;
}

void sim_tick(int n)
{
    (void)n; /* no-op */
}

void sim_flush_fb_cache(void)
{
}

void sim_read_fb(uint32_t byte_offset, uint32_t *dst, uint32_t word_count)
{
    uint32_t idx = byte_offset / 4;
    uint32_t i;

    if (!dst) return;
    for (i = 0; i < word_count; i++) {
        if ((idx + i) < FB_WORD_COUNT)
            dst[i] = fb_shadow[idx + i];
        else
            dst[i] = 0;
    }
}

void sim_write_fb_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count)
{
    uint32_t i;
    if (!src) return;
    for (i = 0; i < word_count; i++) {
        if ((byte_offset / 4u + i) < FB_WORD_COUNT)
            fb_shadow[(byte_offset / 4u) + i] = src[i];
        sim_write(byte_offset + (i * 4u), src[i]);
    }
}

void sim_write_tex_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count)
{
    uint32_t i;
    if (!src) return;
    for (i = 0; i < word_count; i++) {
        if ((byte_offset / 4u + i) < TEX_WORD_COUNT)
            tex_shadow[(byte_offset / 4u) + i] = src[i];
        sim_write(0x800000u + byte_offset + (i * 4u), src[i]);
    }
}

void sim_read_tex(uint32_t byte_offset, uint32_t *dst, uint32_t word_count)
{
    uint32_t idx = byte_offset / 4;
    uint32_t i;

    if (!dst) return;
    for (i = 0; i < word_count; i++) {
        if ((idx + i) < TEX_WORD_COUNT)
            dst[i] = tex_shadow[idx + i];
        else
            dst[i] = 0;
    }
}

void sim_set_swap_count(uint32_t count)
{
    swap_count = count & 0x3u;
}

uint32_t sim_get_swap_count(void)
{
    return swap_count;
}
