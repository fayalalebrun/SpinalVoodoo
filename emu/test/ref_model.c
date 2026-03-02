/*
 * ref_model.c — Golden reference model for Voodoo rendering.
 *
 * Allocates a voodoo_t, initialises lookup tables, and provides register
 * write handling that feeds 86Box's voodoo_triangle() for rendering.
 *
 * Compile 86Box's vid_voodoo_render.c with:
 *   -Dvoodoo_queue_triangle=_voodoo_queue_triangle_86box
 * so we can provide our own synchronous voodoo_queue_triangle() here.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#include <86box/86box.h>
#include "cpu.h"
#include <86box/machine.h>
#include <86box/device.h>
#include <86box/mem.h>
#include <86box/timer.h>
#include <86box/plat.h>
#include <86box/thread.h>
#include <86box/video.h>
#include <86box/vid_svga.h>
#include <86box/vid_voodoo_common.h>
#include <86box/vid_voodoo_regs.h>
#include <86box/vid_voodoo_render.h>
#include <86box/vid_voodoo_texture.h>

#include "ref_model.h"
#include "../../emu/trace/voodoo_trace_format.h"

/* -------------------------------------------------------------------
 * Globals required by 86Box code
 * ------------------------------------------------------------------- */

int tris = 0;

/* Lookup tables — defined here, declared extern in vid_voodoo_common.h */
rgba8_t rgb332[0x100];
rgba8_t ai44[0x100];
rgba8_t rgb565[0x10000];
rgba8_t argb1555[0x10000];
rgba8_t argb4444[0x10000];
rgba8_t ai88[0x10000];

/* -------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------- */

static voodoo_t *voodoo = NULL;

/* -------------------------------------------------------------------
 * voodoo_update_ncc — copied from vid_voodoo_display.c
 * ------------------------------------------------------------------- */

void voodoo_update_ncc(voodoo_t *voodoo, int tmu)
{
    for (uint8_t tbl = 0; tbl < 2; tbl++) {
        for (uint16_t col = 0; col < 256; col++) {
            int y = (col >> 4);
            int i = (col >> 2) & 3;
            int q = col & 3;

            y = (voodoo->nccTable[tmu][tbl].y[y >> 2] >> ((y & 3) * 8)) & 0xff;

            int i_r = (voodoo->nccTable[tmu][tbl].i[i] >> 18) & 0x1ff;
            if (i_r & 0x100) i_r |= 0xfffffe00;
            int i_g = (voodoo->nccTable[tmu][tbl].i[i] >> 9) & 0x1ff;
            if (i_g & 0x100) i_g |= 0xfffffe00;
            int i_b = voodoo->nccTable[tmu][tbl].i[i] & 0x1ff;
            if (i_b & 0x100) i_b |= 0xfffffe00;

            int q_r = (voodoo->nccTable[tmu][tbl].q[q] >> 18) & 0x1ff;
            if (q_r & 0x100) q_r |= 0xfffffe00;
            int q_g = (voodoo->nccTable[tmu][tbl].q[q] >> 9) & 0x1ff;
            if (q_g & 0x100) q_g |= 0xfffffe00;
            int q_b = voodoo->nccTable[tmu][tbl].q[q] & 0x1ff;
            if (q_b & 0x100) q_b |= 0xfffffe00;

            voodoo->ncc_lookup[tmu][tbl][col].rgba.r = CLAMP(y + i_r + q_r);
            voodoo->ncc_lookup[tmu][tbl][col].rgba.g = CLAMP(y + i_g + q_g);
            voodoo->ncc_lookup[tmu][tbl][col].rgba.b = CLAMP(y + i_b + q_b);
            voodoo->ncc_lookup[tmu][tbl][col].rgba.a = 0xff;
        }
    }
}

/* -------------------------------------------------------------------
 * voodoo_recalc — simplified from vid_voodoo.c (Voodoo 1 only)
 * ------------------------------------------------------------------- */

void voodoo_recalc(voodoo_t *voodoo)
{
    uint32_t buffer_offset = ((voodoo->fbiInit2 >> 11) & 511) * 4096;

    if (voodoo->type >= VOODOO_BANSHEE)
        return;

    voodoo->params.front_offset = voodoo->disp_buffer * buffer_offset;
    voodoo->back_offset         = voodoo->draw_buffer * buffer_offset;

    int triple = (voodoo->fbiInit2 & 0x10) || ((voodoo->fbiInit5 & 0x600) == 0x400);
    voodoo->buffer_cutoff = triple ? (buffer_offset * 4) : (buffer_offset * 3);
    if (triple)
        voodoo->params.aux_offset = buffer_offset * 3;
    else
        voodoo->params.aux_offset = buffer_offset * 2;

    switch (voodoo->lfbMode & LFB_WRITE_MASK) {
        case LFB_WRITE_FRONT:
            voodoo->fb_write_offset = voodoo->params.front_offset;
            break;
        case LFB_WRITE_BACK:
            voodoo->fb_write_offset = voodoo->back_offset;
            break;
        default:
            voodoo->fb_write_offset = voodoo->params.front_offset;
            break;
    }

    switch (voodoo->lfbMode & LFB_READ_MASK) {
        case LFB_READ_FRONT:
            voodoo->fb_read_offset = voodoo->params.front_offset;
            break;
        case LFB_READ_BACK:
            voodoo->fb_read_offset = voodoo->back_offset;
            break;
        case LFB_READ_AUX:
            voodoo->fb_read_offset = voodoo->params.aux_offset;
            break;
        default:
            voodoo->fb_read_offset = voodoo->params.front_offset;
            break;
    }

    switch (voodoo->params.fbzMode & FBZ_DRAW_MASK) {
        case FBZ_DRAW_FRONT:
            voodoo->params.draw_offset = voodoo->params.front_offset;
            break;
        case FBZ_DRAW_BACK:
            voodoo->params.draw_offset = voodoo->back_offset;
            break;
        default:
            voodoo->params.draw_offset = voodoo->params.front_offset;
            break;
    }

    /* Row width from fbiInit1/fbiInit6 */
    voodoo->block_width = ((voodoo->fbiInit1 >> 4) & 15) * 2;
    if (voodoo->fbiInit6 & (1 << 30))
        voodoo->block_width += 1;
    if (voodoo->fbiInit1 & (1 << 24))
        voodoo->block_width += 32;
    voodoo->row_width             = voodoo->block_width * 32 * 2;
    voodoo->params.row_width      = voodoo->row_width;
    voodoo->aux_row_width         = voodoo->row_width;
    voodoo->params.aux_row_width  = voodoo->row_width;
}

/* -------------------------------------------------------------------
 * voodoo_queue_triangle — our synchronous override
 *
 * Called when triangleCMD or ftriangleCMD is written.
 * The original voodoo_queue_triangle in vid_voodoo_render.c is renamed
 * to _voodoo_queue_triangle_86box via -D at compile time.
 * ------------------------------------------------------------------- */

/* Declared in vid_voodoo_render.c (not in a header) */
extern void voodoo_triangle(voodoo_t *voodoo, voodoo_params_t *params, int odd_even);

void voodoo_queue_triangle(voodoo_t *voodoo, voodoo_params_t *params)
{
    voodoo_use_texture(voodoo, params, 0);
    if (voodoo->dual_tmus)
        voodoo_use_texture(voodoo, params, 1);

    memcpy(&voodoo->params_buffer[0], params, sizeof(voodoo_params_t));
    voodoo_triangle(voodoo, &voodoo->params_buffer[0], 0);
    tris++;
}

/* -------------------------------------------------------------------
 * Stubs for symbols referenced by 86Box code but not needed here
 * ------------------------------------------------------------------- */

/* Banshee-only, never called for Voodoo 1 */
void banshee_set_overlay_addr(void *priv, uint32_t addr) { (void)priv; (void)addr; }
void banshee_cmd_write(void *priv, uint32_t addr, uint32_t val) { (void)priv; (void)addr; (void)val; }

/* FIFO sync — no-op in single-threaded model */
void voodoo_wait_for_swap_complete(voodoo_t *voodoo) { (void)voodoo; }

/* Voodoo 2+ setup engine — not used by Voodoo 1 traces */
void voodoo_triangle_setup(voodoo_t *voodoo) { (void)voodoo; }

/* -------------------------------------------------------------------
 * voodoo_generate_vb_filters — stub (display-only feature)
 * ------------------------------------------------------------------- */

void voodoo_generate_vb_filters(voodoo_t *v, int fcr, int fcg)
{
    (void)v; (void)fcr; (void)fcg;
}

/* -------------------------------------------------------------------
 * Lookup table initialization — from vid_voodoo.c lines 1300-1345
 * ------------------------------------------------------------------- */

static void init_luts(void)
{
    int c;

    for (c = 0; c < 0x100; c++) {
        rgb332[c].r = c & 0xe0;
        rgb332[c].g = (c << 3) & 0xe0;
        rgb332[c].b = (c << 6) & 0xc0;
        rgb332[c].r = rgb332[c].r | (rgb332[c].r >> 3) | (rgb332[c].r >> 6);
        rgb332[c].g = rgb332[c].g | (rgb332[c].g >> 3) | (rgb332[c].g >> 6);
        rgb332[c].b = rgb332[c].b | (rgb332[c].b >> 2);
        rgb332[c].b = rgb332[c].b | (rgb332[c].b >> 4);
        rgb332[c].a = 0xff;

        ai44[c].a = (c & 0xf0) | ((c & 0xf0) >> 4);
        ai44[c].r = (c & 0x0f) | ((c & 0x0f) << 4);
        ai44[c].g = ai44[c].b = ai44[c].r;
    }

    for (c = 0; c < 0x10000; c++) {
        rgb565[c].r = (c >> 8) & 0xf8;
        rgb565[c].g = (c >> 3) & 0xfc;
        rgb565[c].b = (c << 3) & 0xf8;
        rgb565[c].r |= (rgb565[c].r >> 5);
        rgb565[c].g |= (rgb565[c].g >> 6);
        rgb565[c].b |= (rgb565[c].b >> 5);
        rgb565[c].a = 0xff;

        argb1555[c].r = (c >> 7) & 0xf8;
        argb1555[c].g = (c >> 2) & 0xf8;
        argb1555[c].b = (c << 3) & 0xf8;
        argb1555[c].r |= (argb1555[c].r >> 5);
        argb1555[c].g |= (argb1555[c].g >> 5);
        argb1555[c].b |= (argb1555[c].b >> 5);
        argb1555[c].a = (c & 0x8000) ? 0xff : 0;

        argb4444[c].a = (c >> 8) & 0xf0;
        argb4444[c].r = (c >> 4) & 0xf0;
        argb4444[c].g = c & 0xf0;
        argb4444[c].b = (c << 4) & 0xf0;
        argb4444[c].a |= (argb4444[c].a >> 4);
        argb4444[c].r |= (argb4444[c].r >> 4);
        argb4444[c].g |= (argb4444[c].g >> 4);
        argb4444[c].b |= (argb4444[c].b >> 4);

        ai88[c].a = (c >> 8);
        ai88[c].r = c & 0xff;
        ai88[c].g = c & 0xff;
        ai88[c].b = c & 0xff;
    }
}

/* -------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------- */

int ref_init(int fb_size_mb, int tex_size_mb)
{
    voodoo = (voodoo_t *)calloc(1, sizeof(voodoo_t));
    if (!voodoo) return -1;

    voodoo->type           = VOODOO_1;
    voodoo->dual_tmus      = 0;
    voodoo->render_threads = 1;

    /* Framebuffer */
    voodoo->fb_size    = fb_size_mb * 1024 * 1024;
    voodoo->fb_mask    = voodoo->fb_size - 1;
    voodoo->fb_mem     = (uint8_t *)calloc(1, voodoo->fb_size);
    if (!voodoo->fb_mem) return -1;

    /* Texture memory */
    voodoo->texture_size = tex_size_mb * 1024 * 1024;
    voodoo->texture_mask = voodoo->texture_size - 1;
    voodoo->tex_mem[0]   = (uint8_t *)calloc(1, voodoo->texture_size);
    voodoo->tex_mem_w[0] = (uint16_t *)voodoo->tex_mem[0];
    if (!voodoo->tex_mem[0]) return -1;

    /* Texture cache: allocate data buffers for each cache entry.
     * Size must hold all LOD levels: 256*256 *2 + 128*128 + 64*64 + ... + 2*2 */
    size_t tex_cache_size = (256*256 + 256*256 + 128*128 + 64*64 +
                             32*32 + 16*16 + 8*8 + 4*4 + 2*2) * sizeof(uint32_t);
    for (int i = 0; i < TEX_CACHE_MAX; i++) {
        voodoo->texture_cache[0][i].data = (uint32_t *)calloc(1, tex_cache_size);
        voodoo->texture_cache[1][i].data = (uint32_t *)calloc(1, tex_cache_size);
    }

    voodoo->initEnable = 0x07;  /* Enable fbiInit writes */
    voodoo->bilinear_enabled  = 1;  /* 86Box UI toggle — enable for HW accuracy */
    voodoo->dithersub_enabled = 1;

    /* Default buffer layout: fbiInit2 = 0 gives buffer_offset=0 which is wrong.
     * Set a reasonable default. Glide typically sets fbiInit2 with buffer_offset. */
    voodoo->disp_buffer = 0;
    voodoo->draw_buffer = 1;
    voodoo->row_width             = 1024 * 2;
    voodoo->params.row_width      = 1024 * 2;
    voodoo->aux_row_width         = 1024 * 2;
    voodoo->params.aux_row_width  = 1024 * 2;

    init_luts();

    fprintf(stderr, "[ref_model] Initialized: fb=%dMB tex=%dMB\n", fb_size_mb, tex_size_mb);
    return 0;
}

void ref_shutdown(void)
{
    if (!voodoo) return;

    for (int i = 0; i < TEX_CACHE_MAX; i++) {
        free(voodoo->texture_cache[0][i].data);
        free(voodoo->texture_cache[1][i].data);
    }
    free(voodoo->tex_mem[0]);
    free(voodoo->fb_mem);
    free(voodoo);
    voodoo = NULL;
}

/* -------------------------------------------------------------------
 * Register write handler — wrapper around 86Box's voodoo_reg_writel()
 *
 * fbiInit0-3 and videoDimensions are handled in vid_voodoo.c (the main
 * device file) not in vid_voodoo_reg.c, so we handle them here.
 * ------------------------------------------------------------------- */

/* Declared in vid_voodoo_reg.h */
extern void voodoo_reg_writel(uint32_t addr, uint32_t val, void *priv);

void ref_write_reg(uint32_t addr, uint32_t val)
{
    uint32_t reg = addr & 0x3fc;

    switch (reg) {
        case SST_fbiInit0:
            voodoo->fbiInit0 = val;
            return;
        case SST_fbiInit1:
            voodoo->fbiInit1 = (val & ~5) | (voodoo->fbiInit1 & 5);
            return;
        case SST_fbiInit2:
            voodoo->fbiInit2 = val;
            voodoo_recalc(voodoo);
            return;
        case SST_fbiInit3:
            voodoo->fbiInit3 = val;
            return;
        case SST_videoDimensions:
            voodoo->videoDimensions = val;
            voodoo->h_disp = (val & 0xfff) + 1;
            voodoo->v_disp = (val >> 16) & 0xfff;
            if ((voodoo->v_disp == 386) || (voodoo->v_disp == 402) ||
                (voodoo->v_disp == 482) || (voodoo->v_disp == 602))
                voodoo->v_disp -= 2;
            return;
        default:
            break;
    }

    /* Everything else: forward to 86Box's register handler */
    voodoo_reg_writel(addr, val, voodoo);
}

/* Declared in vid_voodoo_texture.h */
extern void voodoo_tex_writel(uint32_t addr, uint32_t val, void *priv);

void ref_write_tex(uint32_t addr, uint32_t data)
{
    if (!voodoo) return;

    /* Forward to 86Box's texture write handler which remaps PCI-encoded
     * (LOD, T, S) address fields to linear texture memory addresses. */
    voodoo_tex_writel(addr, data, voodoo);
}

/* Declared in vid_voodoo_fb.h */
extern void voodoo_fb_writel(uint32_t addr, uint32_t val, void *priv);
extern void voodoo_fb_writew(uint32_t addr, uint16_t val, void *priv);

void ref_write_fb(uint32_t addr, uint32_t data)
{
    voodoo_fb_writel(addr, data, voodoo);
}

void ref_write_fb_w(uint32_t addr, uint16_t data)
{
    voodoo_fb_writew(addr, data, voodoo);
}

uint16_t *ref_get_fb(void)
{
    if (!voodoo) return NULL;
    return (uint16_t *)voodoo->fb_mem;
}

uint32_t ref_get_fb_size(void)
{
    if (!voodoo) return 0;
    return voodoo->fb_size;
}

uint32_t ref_get_draw_offset(void)
{
    if (!voodoo) return 0;
    return voodoo->params.draw_offset;
}

uint32_t ref_get_front_offset(void)
{
    if (!voodoo) return 0;
    return voodoo->params.front_offset;
}

uint32_t ref_get_row_width(void)
{
    if (!voodoo) return 0;
    return voodoo->row_width;
}

uint8_t *ref_get_tex(void)
{
    if (!voodoo) return NULL;
    return voodoo->tex_mem[0];
}

uint32_t ref_get_tex_size(void)
{
    if (!voodoo) return 0;
    return voodoo->texture_size;
}

/* -------------------------------------------------------------------
 * State loading
 * ------------------------------------------------------------------- */

int ref_load_state(const uint8_t *data, uint32_t size)
{
    if (size < sizeof(voodoo_state_header_t))
        return -1;

    const voodoo_state_header_t *hdr = (const voodoo_state_header_t *)data;

    if (hdr->magic != VOODOO_STATE_MAGIC)
        return -1;

    uint32_t reg_data_size = hdr->reg_count * sizeof(voodoo_state_reg_t);
    uint32_t expected = sizeof(voodoo_state_header_t) + reg_data_size +
                        hdr->fb_size + hdr->tex_size;
    if (size < expected)
        return -1;

    const uint8_t *ptr = data + sizeof(voodoo_state_header_t);

    /* Apply register entries.
     * Skip command registers that have side effects (swap, triangle, fastfill)
     * — the state dump stores their values but isn't meant to trigger them.
     * Also skip fake buffer layout registers (0x300-0x314 for FBI) since they
     * overlap with TMU register space and are just informational. */
    const voodoo_state_reg_t *regs = (const voodoo_state_reg_t *)ptr;
    for (uint32_t i = 0; i < hdr->reg_count; i++) {
        uint32_t reg_addr = regs[i].addr & 0x3fc;
        /* Skip commands — store value only, no side effects */
        if (reg_addr == SST_swapbufferCMD) {
            voodoo->params.swapbufferCMD = regs[i].value;
            continue;
        }
        if (reg_addr == SST_fastfillCMD || reg_addr == SST_triangleCMD ||
            reg_addr == SST_ftriangleCMD || reg_addr == SST_nopCMD)
            continue;
        ref_write_reg(regs[i].addr, regs[i].value);
    }
    ptr += reg_data_size;

    /* Copy framebuffer */
    /* Framebuffer copy disabled — initial FB state not needed for trace replay,
     * and 86Box's 16-bit layout is incompatible with sim's 32-bit interleaved format. */
    // uint32_t fb_copy = (hdr->fb_size <= (uint32_t)voodoo->fb_size) ?
    //                    hdr->fb_size : (uint32_t)voodoo->fb_size;
    // memcpy(voodoo->fb_mem, ptr, fb_copy);
    ptr += hdr->fb_size;

    /* Copy texture memory */
    uint32_t tex_copy = (hdr->tex_size <= (uint32_t)voodoo->texture_size) ?
                        hdr->tex_size : (uint32_t)voodoo->texture_size;
    memcpy(voodoo->tex_mem[0], ptr, tex_copy);

    /* Flush entire texture cache */
    for (int i = 0; i < TEX_CACHE_MAX; i++) {
        voodoo->texture_cache[0][i].refcount = 0;
        voodoo->texture_cache[0][i].base     = ~0u;
    }
    memset(voodoo->texture_present[0], 0, sizeof(voodoo->texture_present[0]));

    /* Apply layout from header */
    voodoo->params.draw_offset    = hdr->draw_offset;
    voodoo->params.aux_offset     = hdr->aux_offset;
    voodoo->params.row_width      = hdr->row_width;
    voodoo->params.aux_row_width  = hdr->row_width;
    voodoo->row_width             = hdr->row_width;
    voodoo->aux_row_width         = hdr->row_width;

    /* Derive draw_buffer/disp_buffer from header's draw_offset so that
     * subsequent voodoo_recalc calls produce consistent offsets. */
    uint32_t buffer_offset = ((voodoo->fbiInit2 >> 11) & 511) * 4096;
    if (buffer_offset > 0) {
        voodoo->draw_buffer = hdr->draw_offset / buffer_offset;
        voodoo->disp_buffer = !voodoo->draw_buffer;
        /* Also set front_offset/back_offset consistently */
        voodoo->params.front_offset = voodoo->disp_buffer * buffer_offset;
        voodoo->back_offset         = voodoo->draw_buffer * buffer_offset;
    }

    fprintf(stderr, "[ref_model] State loaded: %u regs, fb=%u tex=%u draw=0x%x aux=0x%x row=%u\n",
            hdr->reg_count, hdr->fb_size, hdr->tex_size,
            hdr->draw_offset, hdr->aux_offset, hdr->row_width);
    return 0;
}
