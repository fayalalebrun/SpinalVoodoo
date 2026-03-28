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
#include <86box/vid_voodoo_trace.h>

#include "ref_model.h"

#define REF_EXPECTED_STATE_VERSION 2

/* -------------------------------------------------------------------
 * Globals required by 86Box code
 * ------------------------------------------------------------------- */

int tris = 0;
uint64_t tsc = 0;
int voodoo_trace_max_frames = 0;
static uint64_t ref_total_pixels_in = 0;
static uint64_t ref_total_pixels_out = 0;
static uint64_t ref_total_zfunc_fail = 0;
static uint64_t ref_total_afunc_fail = 0;
static uint64_t ref_total_chroma_fail = 0;
static uint8_t *ref_triangle_coverage_bits = NULL;
static uint32_t ref_triangle_coverage_w = 0;
static uint32_t ref_triangle_coverage_h = 0;
static uint32_t ref_triangle_coverage_unique = 0;
static uint64_t ref_triangle_color_writes = 0;
static uint64_t ref_triangle_black_writes = 0;
static uint64_t ref_triangle_depth_only_updates = 0;
static uint64_t ref_triangles_textured = 0;
static uint64_t ref_triangles_untextured = 0;
static uint64_t ref_triangles_all_black_writes = 0;

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
static FILE *ref_trace_file = NULL;
static uint64_t ref_trace_event_index = 0;
static uint32_t ref_trace_draw_id = 0;
static uint32_t ref_trace_next_primitive_id = 0;
static uint32_t ref_trace_active_primitive_id = 0;
static uint32_t ref_trace_pixel_seq = 0;
static int ref_trace_triangle_active = 0;

void ref_trace_close(void);

int ref_trace_open(const char *path)
{
    ref_trace_close();
    ref_trace_event_index = 0;
    ref_trace_draw_id = 0;
    ref_trace_next_primitive_id = 0;
    ref_trace_active_primitive_id = 0;
    ref_trace_pixel_seq = 0;
    ref_trace_triangle_active = 0;

    if (!path || !path[0])
        return 0;

    ref_trace_file = fopen(path, "w");
    if (!ref_trace_file) {
        fprintf(stderr, "[ref_model] ERROR: failed to open ref trace export: %s\n", path);
        return -1;
    }

    setvbuf(ref_trace_file, NULL, _IOFBF, 1 << 20);
    fprintf(stderr, "[ref_model] Reference trace export: %s\n", path);
    return 0;
}

void ref_trace_close(void)
{
    if (!ref_trace_file)
        return;
    fclose(ref_trace_file);
    ref_trace_file = NULL;
}

static void ref_trace_reset_ids(void)
{
    ref_trace_event_index = 0;
    ref_trace_draw_id = 0;
    ref_trace_next_primitive_id = 0;
    ref_trace_active_primitive_id = 0;
    ref_trace_pixel_seq = 0;
    ref_trace_triangle_active = 0;
}

static void ref_trace_begin_triangle(void)
{
    ref_trace_active_primitive_id = ref_trace_next_primitive_id++;
    ref_trace_pixel_seq = 0;
    ref_trace_triangle_active = 1;
}

static void ref_trace_end_triangle(void)
{
    ref_trace_triangle_active = 0;
}

static void ref_trace_note_swapbuffer(void)
{
    ref_trace_draw_id++;
}

static void ref_trace_triangle(const voodoo_params_t *params)
{
    if (!ref_trace_file || !ref_trace_triangle_active)
        return;

    fprintf(ref_trace_file,
            "{\"stage\":\"triangle\",\"origin\":0,\"draw_id\":%u,\"primitive_id\":%u,"
            "\"textured\":%u,\"sign\":%d,"
            "\"vertex_ax\":%d,\"vertex_ay\":%d,\"vertex_bx\":%d,\"vertex_by\":%d,\"vertex_cx\":%d,\"vertex_cy\":%d,"
            "\"fbz_color_path\":%u,\"texture_mode_0\":%u,\"tlod_0\":%u,\"tex_base_addr_0\":%u,"
            "\"tex_base_addr_1_0\":%u,\"tex_base_addr_2_0\":%u,\"tex_base_addr_38_0\":%u,\"tformat_0\":%d,"
            "\"start_s\":%lld,\"start_t\":%lld,\"start_w\":%lld,"
            "\"dsdx\":%lld,\"dtdx\":%lld,\"dwdx\":%lld,"
            "\"dsdy\":%lld,\"dtdy\":%lld,\"dwdy\":%lld}\n",
            ref_trace_draw_id,
            ref_trace_active_primitive_id,
            (params->fbzColorPath & FBZCP_TEXTURE_ENABLED) ? 1u : 0u,
            params->sign,
            params->vertexAx,
            params->vertexAy,
            params->vertexBx,
            params->vertexBy,
            params->vertexCx,
            params->vertexCy,
            params->fbzColorPath,
            params->textureMode[0],
            params->tLOD[0],
            params->texBaseAddr[0],
            params->texBaseAddr1[0],
            params->texBaseAddr2[0],
            params->texBaseAddr38[0],
            params->tformat[0],
            (long long)params->tmu[0].startS,
            (long long)params->tmu[0].startT,
            (long long)params->tmu[0].startW,
            (long long)params->tmu[0].dSdX,
            (long long)params->tmu[0].dTdX,
            (long long)params->tmu[0].dWdX,
            (long long)params->tmu[0].dSdY,
            (long long)params->tmu[0].dTdY,
            (long long)params->tmu[0].dWdY);
}

void ref_trace_tmu_input(int tmu, int x, int y, int64_t s, int64_t t, int64_t w)
{
    if (!ref_trace_file || !ref_trace_triangle_active)
        return;

    int64_t dut_s = s >> 14;
    int64_t dut_t = t >> 14;
    int64_t dut_w = w >> 2;

    fprintf(ref_trace_file,
            "{\"event_index\":%llu,\"stage\":\"tmu_input\",\"tmu\":%d,\"origin\":0,"
            "\"draw_id\":%u,\"primitive_id\":%u,\"pixel_seq\":%u,\"x\":%d,\"y\":%d,"
            "\"s\":%lld,\"t\":%lld,\"w\":%lld}\n",
            (unsigned long long)ref_trace_event_index++,
            tmu,
            ref_trace_draw_id,
            ref_trace_active_primitive_id,
            ref_trace_pixel_seq,
            x,
            y,
            (long long)dut_s,
            (long long)dut_t,
            (long long)dut_w);
}

void ref_trace_tmu_output(int tmu, int x, int y, int tex_s, int tex_t, int lod,
                          int texture_r, int texture_g, int texture_b, int texture_a)
{
    if (!ref_trace_file || !ref_trace_triangle_active)
        return;

    fprintf(ref_trace_file,
            "{\"event_index\":%llu,\"stage\":\"tmu_output\",\"tmu\":%d,\"origin\":0,"
            "\"draw_id\":%u,\"primitive_id\":%u,\"pixel_seq\":%u,\"x\":%d,\"y\":%d,"
            "\"tex_s\":%d,\"tex_t\":%d,\"lod\":%d,\"texture_r\":%d,\"texture_g\":%d,"
            "\"texture_b\":%d,\"texture_a\":%d}\n",
            (unsigned long long)ref_trace_event_index++,
            tmu,
            ref_trace_draw_id,
            ref_trace_active_primitive_id,
            ref_trace_pixel_seq,
            x,
            y,
            tex_s,
            tex_t,
            lod,
            texture_r,
            texture_g,
            texture_b,
            texture_a);

    ref_trace_pixel_seq++;
}

static void ref_coverage_reset(void)
{
    free(ref_triangle_coverage_bits);
    ref_triangle_coverage_bits = NULL;
    ref_triangle_coverage_w = 0;
    ref_triangle_coverage_h = 0;
    ref_triangle_coverage_unique = 0;
    ref_triangle_color_writes = 0;
    ref_triangle_black_writes = 0;
    ref_triangle_depth_only_updates = 0;
    ref_triangles_textured = 0;
    ref_triangles_untextured = 0;
    ref_triangles_all_black_writes = 0;
}

/* Hook called from vid_voodoo_render.c when REF_TRIANGLE_COVERAGE_HOOK is enabled. */
void ref_coverage_mark_pixel(int x, int y)
{
    if (!voodoo)
        return;
    if (x < 0 || y < 0)
        return;
    if (voodoo->h_disp <= 0 || voodoo->v_disp <= 0)
        return;
    if (x >= voodoo->h_disp || y >= voodoo->v_disp)
        return;

    uint32_t w = (uint32_t)voodoo->h_disp;
    uint32_t h = (uint32_t)voodoo->v_disp;
    uint64_t total = (uint64_t)w * (uint64_t)h;
    uint64_t bytes = (total + 7u) / 8u;

    if (!ref_triangle_coverage_bits ||
        ref_triangle_coverage_w != w ||
        ref_triangle_coverage_h != h) {
        uint8_t *new_bits = (uint8_t *)calloc(1, (size_t)bytes);
        if (!new_bits)
            return;
        free(ref_triangle_coverage_bits);
        ref_triangle_coverage_bits = new_bits;
        ref_triangle_coverage_w = w;
        ref_triangle_coverage_h = h;
        ref_triangle_coverage_unique = 0;
    }

    uint64_t idx = (uint64_t)(uint32_t)y * (uint64_t)w + (uint64_t)(uint32_t)x;
    uint8_t mask = (uint8_t)(1u << (idx & 7u));
    uint8_t *cell = &ref_triangle_coverage_bits[idx >> 3];
    if (!(*cell & mask)) {
        *cell |= mask;
        ref_triangle_coverage_unique++;
    }
}

void ref_coverage_note_color_write(int is_black)
{
    ref_triangle_color_writes++;
    if (is_black)
        ref_triangle_black_writes++;
}

void ref_coverage_note_depth_only(void)
{
    ref_triangle_depth_only_updates++;
}

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
    uint64_t before_writes = ref_triangle_color_writes;
    uint64_t before_black = ref_triangle_black_writes;

    ref_trace_begin_triangle();

    voodoo_use_texture(voodoo, params, 0);
    if (voodoo->dual_tmus)
        voodoo_use_texture(voodoo, params, 1);

    ref_trace_triangle(params);

    memcpy(&voodoo->params_buffer[0], params, sizeof(voodoo_params_t));
    voodoo_triangle(voodoo, &voodoo->params_buffer[0], 0);

    if (params->fbzColorPath & FBZCP_TEXTURE_ENABLED)
        ref_triangles_textured++;
    else
        ref_triangles_untextured++;

    {
        uint64_t delta_w = ref_triangle_color_writes - before_writes;
        uint64_t delta_b = ref_triangle_black_writes - before_black;
        if (delta_w > 0 && delta_w == delta_b)
            ref_triangles_all_black_writes++;
    }
    ref_trace_end_triangle();
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
    ref_total_pixels_in = 0;
    ref_total_pixels_out = 0;
    ref_total_zfunc_fail = 0;
    ref_total_afunc_fail = 0;
    ref_total_chroma_fail = 0;
    ref_coverage_reset();
    ref_trace_reset_ids();

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

static void ref_accumulate_fbi_counters(void)
{
    if (!voodoo) return;
    ref_total_pixels_in += voodoo->fbiPixelsIn;
    ref_total_pixels_out += voodoo->fbiPixelsOut;
    ref_total_zfunc_fail += voodoo->fbiZFuncFail;
    ref_total_afunc_fail += voodoo->fbiAFuncFail;
    ref_total_chroma_fail += voodoo->fbiChromaFail;
}

void ref_shutdown(void)
{
    if (!voodoo) return;

    ref_trace_close();

    for (int i = 0; i < TEX_CACHE_MAX; i++) {
        free(voodoo->texture_cache[0][i].data);
        free(voodoo->texture_cache[1][i].data);
    }
    free(voodoo->tex_mem[0]);
    free(voodoo->fb_mem);
    free(voodoo);
    voodoo = NULL;
    ref_coverage_reset();
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
            voodoo_recalc(voodoo);
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
        case SST_nopCMD:
            /* 86Box resets FBI counters on nopCMD; keep cumulative totals too. */
            ref_accumulate_fbi_counters();
            voodoo_reg_writel(addr, val, voodoo);
            return;
        default:
            break;
    }

    /* Everything else: forward to 86Box's register handler */
    voodoo_reg_writel(addr, val, voodoo);

    if (reg == SST_swapbufferCMD)
        ref_trace_note_swapbuffer();
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

uint32_t ref_get_fb_write_offset(void)
{
    if (!voodoo) return 0;
    return voodoo->fb_write_offset;
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

uint32_t ref_get_clip_left_right(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(voodoo->params.clipRight | (voodoo->params.clipLeft << 16));
}

uint32_t ref_get_clip_lowy_highy(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(voodoo->params.clipHighY | (voodoo->params.clipLowY << 16));
}

uint32_t ref_get_swapbuffer_cmd(void)
{
    if (!voodoo) return 0;
    return voodoo->params.swapbufferCMD;
}

uint32_t ref_get_tex_base_addr(int tmu, int which)
{
    if (!voodoo) return 0;
    if (tmu < 0 || tmu > 1) return 0;

    switch (which) {
        case 0: return voodoo->params.texBaseAddr[tmu];
        case 1: return voodoo->params.texBaseAddr1[tmu];
        case 2: return voodoo->params.texBaseAddr2[tmu];
        case 3: return voodoo->params.texBaseAddr38[tmu];
        default: return 0;
    }
}

uint32_t ref_get_clut_rgb(int index)
{
    if (!voodoo) return 0;
    if (index < 0 || index >= 33) return 0;

    return ((uint32_t)voodoo->clutData[index].r << 16) |
           ((uint32_t)voodoo->clutData[index].g << 8) |
           ((uint32_t)voodoo->clutData[index].b);
}

uint32_t ref_get_fbi_pixels_in(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(ref_total_pixels_in + voodoo->fbiPixelsIn);
}

uint32_t ref_get_fbi_pixels_out(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(ref_total_pixels_out + voodoo->fbiPixelsOut);
}

uint32_t ref_get_fbi_zfunc_fail(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(ref_total_zfunc_fail + voodoo->fbiZFuncFail);
}

uint32_t ref_get_fbi_afunc_fail(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(ref_total_afunc_fail + voodoo->fbiAFuncFail);
}

uint32_t ref_get_fbi_chroma_fail(void)
{
    if (!voodoo) return 0;
    return (uint32_t)(ref_total_chroma_fail + voodoo->fbiChromaFail);
}

uint32_t ref_get_fbz_mode(void)
{
    if (!voodoo) return 0;
    return voodoo->params.fbzMode;
}

uint32_t ref_get_za_color(void)
{
    if (!voodoo) return 0;
    return voodoo->params.zaColor;
}

uint32_t ref_get_triangle_coverage_pixels(void)
{
    return ref_triangle_coverage_unique;
}

uint32_t ref_get_triangle_coverage_total(void)
{
    if (ref_triangle_coverage_w == 0 || ref_triangle_coverage_h == 0)
        return 0;
    return ref_triangle_coverage_w * ref_triangle_coverage_h;
}

uint32_t ref_get_triangle_color_writes(void)
{
    return (uint32_t)ref_triangle_color_writes;
}

uint32_t ref_get_triangle_black_writes(void)
{
    return (uint32_t)ref_triangle_black_writes;
}

uint32_t ref_get_triangle_depth_only_updates(void)
{
    return (uint32_t)ref_triangle_depth_only_updates;
}

uint32_t ref_get_triangles_textured(void)
{
    return (uint32_t)ref_triangles_textured;
}

uint32_t ref_get_triangles_untextured(void)
{
    return (uint32_t)ref_triangles_untextured;
}

uint32_t ref_get_triangles_all_black_writes(void)
{
    return (uint32_t)ref_triangles_all_black_writes;
}

uint32_t ref_get_palette_nonzero_count(int tmu)
{
    if (!voodoo)
        return 0;
    if (tmu < 0 || tmu > 1)
        return 0;
    uint32_t nz = 0;
    for (int i = 0; i < 256; i++) {
        if (voodoo->palette[tmu][i].u != 0)
            nz++;
    }
    return nz;
}

void ref_dump_triangle_debug(void)
{
    if (!voodoo)
        return;
    fprintf(stderr,
            "[ref_model] tri dbg: vA=(%d,%d) vB=(%d,%d) vC=(%d,%d) draw=0x%x aux=0x%x row=%d auxRow=%d clip=(%d..%d,%d..%d) sign=%d fbzMode=0x%08x colorPath=0x%08x tex0=0x%08x tex1=0x%08x tLOD0=0x%08x tLOD1=0x%08x base0=0x%08x base1=0x%08x front=0x%x back=0x%x fbMask=0x%x h=%d v=%d\n",
            voodoo->params.vertexAx, voodoo->params.vertexAy,
            voodoo->params.vertexBx, voodoo->params.vertexBy,
            voodoo->params.vertexCx, voodoo->params.vertexCy,
            voodoo->params.draw_offset, voodoo->params.aux_offset,
            voodoo->params.row_width, voodoo->params.aux_row_width,
            voodoo->params.clipLeft, voodoo->params.clipRight,
            voodoo->params.clipLowY, voodoo->params.clipHighY,
            voodoo->params.sign, voodoo->params.fbzMode,
            voodoo->params.fbzColorPath,
            voodoo->params.textureMode[0], voodoo->params.textureMode[1],
            voodoo->params.tLOD[0], voodoo->params.tLOD[1],
            voodoo->params.texBaseAddr[0], voodoo->params.texBaseAddr[1],
            voodoo->params.front_offset, voodoo->back_offset,
            voodoo->fb_mask, voodoo->h_disp, voodoo->v_disp);
}

void ref_dump_layout_debug(void)
{
    if (!voodoo)
        return;
    fprintf(stderr,
            "[ref_model] layout dbg: fbiInit1=0x%08x fbiInit2=0x%08x videoDimensions=0x%08x row=%d auxRow=%d draw=0x%x front=0x%x back=0x%x fbWrite=0x%x fbRead=0x%x h=%d v=%d disp=%d drawbuf=%d\n",
            voodoo->fbiInit1, voodoo->fbiInit2, voodoo->videoDimensions,
            voodoo->row_width, voodoo->aux_row_width,
            voodoo->params.draw_offset, voodoo->params.front_offset,
            voodoo->back_offset, voodoo->fb_write_offset,
            voodoo->fb_read_offset, voodoo->h_disp, voodoo->v_disp,
            voodoo->disp_buffer, voodoo->draw_buffer);
}

int ref_dump_state_to_dir(const char *dir, uint32_t frame_num)
{
    if (!voodoo || !dir || !dir[0])
        return -1;

    uint32_t saved_fb_size = voodoo->fb_size;
    uint32_t saved_tex_size = voodoo->texture_size;

    /* 86Box writer expects these fields in MB; ref model stores bytes. */
    voodoo->fb_size = saved_fb_size / (1024 * 1024);
    voodoo->texture_size = saved_tex_size / (1024 * 1024);

    voodoo_trace_t trace;
    memset(&trace, 0, sizeof(trace));
    trace.voodoo = voodoo;
    trace.dump_state = 1;
    snprintf(trace.trace_dir, sizeof(trace.trace_dir), "%s", dir);
    voodoo_trace_dump_state(&trace, frame_num);

    voodoo->fb_size = saved_fb_size;
    voodoo->texture_size = saved_tex_size;
    return 0;
}

/* -------------------------------------------------------------------
 * State loading
 * ------------------------------------------------------------------- */

int ref_load_state(const uint8_t *data, uint32_t size)
{
    if (size < sizeof(voodoo_state_header_t))
        return -1;

    const voodoo_state_header_t *hdr = (const voodoo_state_header_t *)data;
    ref_total_pixels_in = 0;
    ref_total_pixels_out = 0;
    ref_total_zfunc_fail = 0;
    ref_total_afunc_fail = 0;
    ref_total_chroma_fail = 0;
    ref_coverage_reset();

    if (hdr->magic != VOODOO_STATE_MAGIC)
        return -1;
    if (hdr->version != REF_EXPECTED_STATE_VERSION) {
        fprintf(stderr, "[ref_model] ERROR: Unsupported state version %u (expected %u). v1 is not supported.\n",
                hdr->version, REF_EXPECTED_STATE_VERSION);
        return -1;
    }

    uint32_t reg_data_size = hdr->reg_count * sizeof(voodoo_state_reg_t);
    uint32_t expected = sizeof(voodoo_state_header_t) + reg_data_size +
                        hdr->fb_size + hdr->tex_size;
    if (size < expected)
        return -1;

    const uint8_t *ptr = data + sizeof(voodoo_state_header_t);

    /* Apply register entries.
     * Skip command registers that have side effects (swap, triangle, fastfill)
     * — the state dump stores their values but isn't meant to trigger them. */
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

    /* Restore framebuffer state from state.bin. */
    {
        uint32_t fb_copy = (hdr->fb_size <= (uint32_t)voodoo->fb_size) ?
                            hdr->fb_size : (uint32_t)voodoo->fb_size;
        memcpy(voodoo->fb_mem, ptr, fb_copy);
    }
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
        /* Recompute fb_write_offset, fb_read_offset, etc. from the
         * corrected buffer assignments. */
        voodoo_recalc(voodoo);
    }
    fprintf(stderr, "[ref_model] State loaded: %u regs, fb=%u tex=%u draw=0x%x aux=0x%x row=%u\n",
            hdr->reg_count, hdr->fb_size, hdr->tex_size,
            hdr->draw_offset, hdr->aux_offset, hdr->row_width);
    return 0;
}
