/*
 * trace_test.cpp — Replay Voodoo traces through 86Box reference model
 * and Verilator CoreSim, comparing framebuffer output pixel-by-pixel.
 *
 * Usage:
 *   trace_test <path> [--ref-only] [--output-dir DIR] [--ref-trace-jsonl PATH] [--max-mismatches N]
 *                     [--color-tolerance N] [--width W] [--height H]
 *
 *   path: .bin trace file, or directory containing trace.bin + optional state.bin
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

/* stb_image_write for PNG output */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/* Trace format */
extern "C" {
#include "../../emu/trace/voodoo_trace_format.h"
#include "ref_model.h"
}

/* Sim harness (optional — omitted in --ref-only mode) */
#include "sim_harness.h"

/* -------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------- */

static bool is_directory(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

static uint8_t *read_file(const char *path, uint32_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(sz);
    if (!buf) { fclose(f); return nullptr; }
    if ((long)fread(buf, 1, sz, f) != sz) { free(buf); fclose(f); return nullptr; }
    fclose(f);
    *out_size = (uint32_t)sz;
    return buf;
}

/* sRGB gamma encoding LUT — built once, maps linear 0-255 to sRGB 0-255.
 * Voodoo1 framebuffer stores linear values meant for a CRT with ~2.2 gamma.
 * PNG viewers assume sRGB, so we apply the sRGB transfer function. */
static uint8_t srgb_lut[256];

static void init_srgb_lut(void) {
    for (int i = 0; i < 256; i++) {
        double v = i / 255.0;
        double s = (v <= 0.0031308)
            ? v * 12.92
            : 1.055 * pow(v, 1.0 / 2.4) - 0.055;
        srgb_lut[i] = (uint8_t)(s * 255.0 + 0.5);
    }
}

/* Convert RGB565 to 24-bit sRGB for PNG output */
static void rgb565_to_rgb24(const uint16_t *src, uint8_t *dst, int count) {
    for (int i = 0; i < count; i++) {
        uint16_t px = src[i];
        uint8_t r = (px >> 8) & 0xf8; r |= r >> 5;
        uint8_t g = (px >> 3) & 0xfc; g |= g >> 6;
        uint8_t b = (px << 3) & 0xf8; b |= b >> 5;
        dst[i * 3 + 0] = srgb_lut[r];
        dst[i * 3 + 1] = srgb_lut[g];
        dst[i * 3 + 2] = srgb_lut[b];
    }
}

/* Extract display name from path */
static std::string basename_no_ext(const char *path) {
    std::string s(path);
    /* Remove trailing slash */
    while (!s.empty() && s.back() == '/') s.pop_back();
    /* Get last component */
    size_t sep = s.rfind('/');
    if (sep != std::string::npos) s = s.substr(sep + 1);
    /* Remove .bin extension */
    size_t dot = s.rfind('.');
    if (dot != std::string::npos && s.substr(dot) == ".bin") s = s.substr(0, dot);
    return s;
}

/* -------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------- */

int main(int argc, char **argv) {
    init_srgb_lut();

    int watch_x = -1;
    int watch_y = -1;
    if (const char *wx = getenv("SIM_WATCH_X")) watch_x = atoi(wx);
    if (const char *wy = getenv("SIM_WATCH_Y")) watch_y = atoi(wy);

    /* Parse arguments */
    const char *trace_path = nullptr;
    const char *output_dir = nullptr;
    const char *ref_trace_jsonl = nullptr;
    bool ref_only = false;
    int max_mismatches = 0;
    int color_tolerance = 0;
    int disp_width  = 640;
    int disp_height = 480;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ref-only") == 0) {
            ref_only = true;
        } else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (strcmp(argv[i], "--ref-trace-jsonl") == 0 && i + 1 < argc) {
            ref_trace_jsonl = argv[++i];
        } else if (strcmp(argv[i], "--max-mismatches") == 0 && i + 1 < argc) {
            max_mismatches = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--color-tolerance") == 0 && i + 1 < argc) {
            color_tolerance = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            disp_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            disp_height = atoi(argv[++i]);
        } else if (!trace_path) {
            trace_path = argv[i];
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (!trace_path) {
        fprintf(stderr, "Usage: trace_test <path> [--ref-only] [--output-dir DIR] [--ref-trace-jsonl PATH] "
                "[--max-mismatches N] [--color-tolerance N] [--width W] [--height H]\n");
        return 1;
    }

    /* Resolve paths */
    std::string trace_file;
    std::string state_file;

    if (is_directory(trace_path)) {
        trace_file = std::string(trace_path) + "/trace.bin";
        state_file = std::string(trace_path) + "/state.bin";
        /* Check if state file exists */
        struct stat st;
        if (stat(state_file.c_str(), &st) != 0)
            state_file.clear();
    } else {
        trace_file = trace_path;
    }

    std::string name = basename_no_ext(trace_path);

    /* Read trace file */
    uint32_t trace_size = 0;
    uint8_t *trace_data = read_file(trace_file.c_str(), &trace_size);
    if (!trace_data) {
        fprintf(stderr, "ERROR: Cannot read trace file: %s\n", trace_file.c_str());
        return 1;
    }

    if (trace_size < sizeof(voodoo_trace_header_t)) {
        fprintf(stderr, "ERROR: Trace file too small\n");
        free(trace_data);
        return 1;
    }

    const voodoo_trace_header_t *hdr = (const voodoo_trace_header_t *)trace_data;
    if (hdr->magic != VOODOO_TRACE_MAGIC) {
        fprintf(stderr, "ERROR: Bad trace magic: 0x%08x (expected 0x%08x)\n",
                hdr->magic, VOODOO_TRACE_MAGIC);
        free(trace_data);
        return 1;
    }

    int fb_mb  = hdr->fb_size_mb ? hdr->fb_size_mb : 4;
    int tex_mb = hdr->tex_size_mb ? hdr->tex_size_mb : 4;

    uint32_t entry_bytes = trace_size - sizeof(voodoo_trace_header_t);
    uint32_t num_entries = entry_bytes / sizeof(voodoo_trace_entry_t);

    fprintf(stderr, "[trace_test] Trace: %s (%u entries, fb=%dMB tex=%dMB)\n",
            trace_file.c_str(), num_entries, fb_mb, tex_mb);

    const voodoo_trace_entry_t *entries =
        (const voodoo_trace_entry_t *)(trace_data + sizeof(voodoo_trace_header_t));

    /* Read state file if present */
    uint8_t *state_data = nullptr;
    uint32_t state_size = 0;
    std::vector<uint16_t> ref_fb_initial;
    if (!state_file.empty()) {
        state_data = read_file(state_file.c_str(), &state_size);
        if (state_data) {
            if (state_size < sizeof(voodoo_state_header_t)) {
                fprintf(stderr, "ERROR: State file too small: %s\n", state_file.c_str());
                free(state_data);
                free(trace_data);
                return 1;
            }
            const voodoo_state_header_t *shdr = (const voodoo_state_header_t *)state_data;
            if (shdr->magic != VOODOO_STATE_MAGIC) {
                fprintf(stderr, "ERROR: Bad state magic: 0x%08x (expected 0x%08x)\n",
                        shdr->magic, VOODOO_STATE_MAGIC);
                free(state_data);
                free(trace_data);
                return 1;
            }
            if (shdr->version != VOODOO_STATE_VERSION) {
                fprintf(stderr,
                        "ERROR: Unsupported state version %u (expected %u). v1 is not supported.\n",
                        shdr->version, VOODOO_STATE_VERSION);
                free(state_data);
                free(trace_data);
                return 1;
            }
            if (shdr->h_disp > 0) disp_width  = shdr->h_disp;
            if (shdr->v_disp > 0) disp_height = shdr->v_disp;
            fprintf(stderr, "[trace_test] State: %s (display %dx%d)\n",
                    state_file.c_str(), disp_width, disp_height);
        }
    }

    /* Initialize reference model */
    if (ref_init(fb_mb, tex_mb) != 0) {
        fprintf(stderr, "ERROR: ref_init failed\n");
        return 1;
    }
    if (ref_trace_open(ref_trace_jsonl) != 0) {
        fprintf(stderr, "ERROR: ref_trace_open failed\n");
        return 1;
    }

    /* Initialize Verilator model (unless --ref-only) */
    if (!ref_only) {
        if (sim_init() != 0) {
            fprintf(stderr, "ERROR: sim_init failed\n");
            return 1;
        }
    }

    /* Load state into both models */
    if (state_data) {
        if (ref_load_state(state_data, state_size) != 0) {
            fprintf(stderr, "ERROR: ref_load_state failed\n");
            return 1;
        }
        {
            uint16_t *fb_ptr = ref_get_fb();
            uint32_t fb_bytes = ref_get_fb_size();
            if (fb_ptr && fb_bytes > 0) {
                ref_fb_initial.resize(fb_bytes / 2);
                memcpy(ref_fb_initial.data(), fb_ptr, fb_bytes);
            }
        }

        if (!ref_only) {
            const voodoo_state_header_t *shdr = (const voodoo_state_header_t *)state_data;
            const uint8_t *ptr = state_data + sizeof(voodoo_state_header_t);

            /* Write registers to sim.
             * Skip command registers (swap, fastfill, triangle) that have
             * side effects — state dump stores values, not triggers. */
            const voodoo_state_reg_t *regs = (const voodoo_state_reg_t *)ptr;
            for (uint32_t i = 0; i < shdr->reg_count; i++) {
                uint32_t reg_addr = regs[i].addr & 0x3fc;
                if (reg_addr == 0x128 || reg_addr == 0x124 ||  /* swapbuffer, fastfill */
                    reg_addr == 0x080 || reg_addr == 0x100 ||  /* triangleCMD, ftriangleCMD */
                    reg_addr == 0x120)                         /* nopCMD */
                    continue;
                sim_write(regs[i].addr, regs[i].value);
            }
            ptr += shdr->reg_count * sizeof(voodoo_state_reg_t);

            /* Extract fbiInit2 for buffer offset calculations below. */
            uint32_t fbiInit2_val = 0;
            for (uint32_t i = 0; i < shdr->reg_count; i++) {
                if ((regs[i].addr & 0x3fc) == 0x218) {
                    fbiInit2_val = regs[i].value;
                    break;
                }
            }

            /* Restore framebuffer state from state.bin. */
            {
                const uint8_t *fb_mem = ptr;
                std::vector<uint32_t> fb_words((shdr->fb_size + 3) / 4, 0);
                memcpy(fb_words.data(), fb_mem, shdr->fb_size);
                sim_write_fb_bulk(0, fb_words.data(), fb_words.size());
                fprintf(stderr, "[trace_test] FB state loaded into sim: raw copy (%u bytes, aux_off=0x%x)\n",
                        shdr->fb_size, shdr->aux_offset);
            }
            ptr += shdr->fb_size;

            /* Write texture memory to sim */
            sim_write_tex_bulk(0, (const uint32_t *)ptr, shdr->tex_size / 4);

            /* Align sim's front/back assignment with 86Box draw_buffer.
             * 86Box stores draw_offset as the currently selected draw buffer.
             * CoreSim derives front/back from swapCount(0):
             *   swapCount(0)=0 -> front=buffer0, back=buffer1
             *   swapCount(0)=1 -> front=buffer1, back=buffer0
             * For the common Voodoo1 double-buffer case, draw_buffer parity is
             * therefore inverted relative to swapCount(0). */
            {
                uint32_t buffer_offset = ((fbiInit2_val >> 11) & 0x1FF) * 4096;
                if (buffer_offset > 0) {
                    uint32_t draw_buffer = shdr->draw_offset / buffer_offset;
                    uint32_t swap_count = 1 - (draw_buffer & 0x1);
                    sim_set_swap_count(swap_count);
                    fprintf(stderr, "[trace_test] Buffer alignment: draw_offset=0x%x buffer_offset=0x%x draw_buffer=%u swap_count=%u\n",
                            shdr->draw_offset, buffer_offset, draw_buffer, swap_count);
                }
            }

            /* Ensure all state registers are drained from PciFifo before
             * reading back or proceeding to trace replay. */
            sim_idle_wait();

            fprintf(stderr, "[trace_test] State loaded into sim\n");
        }

        free(state_data);
        state_data = nullptr;
    }

    /* Replay trace entries */
    int tri_count = 0;
    int tex_write_count = 0;
    int fb_write_count = 0;
    int swap_cmd_count = 0;
    uint64_t cmd_type_counts[32] = {0};
    uint32_t fbzmode_writes = 0, fbzcp_writes = 0, texmode0_writes = 0, texmode1_writes = 0;
    uint32_t fbzmode_last = 0, fbzcp_last = 0, texmode0_last = 0, texmode1_last = 0;
    uint32_t pal_index_writes = 0;
    bool have_presented_offset = false;
    uint32_t presented_offset = 0;
    uint64_t logical_cmds = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        const voodoo_trace_entry_t *e = &entries[i];
        uint32_t repeat = e->count ? e->count : 1;
        logical_cmds += repeat;
        if (e->cmd_type < 32)
            cmd_type_counts[e->cmd_type] += repeat;

        for (uint32_t rep = 0; rep < repeat; rep++) {
            switch (e->cmd_type) {
                case VOODOO_TRACE_WRITE_REG_L: {
                uint32_t addr = e->addr & 0x3FFFFF;
                uint32_t reg = addr & 0x3fc;
                if (reg == 0x110) { fbzmode_writes++; fbzmode_last = e->data; }
                if (reg == 0x104) { fbzcp_writes++; fbzcp_last = e->data; }
                if (reg == 0x300) { texmode0_writes++; texmode0_last = e->data; }
                if (reg == 0x304) { texmode1_writes++; texmode1_last = e->data; }
                if ((reg == 0x34c || reg == 0x350) && (e->data & 0x80000000u))
                    pal_index_writes++;
                if (reg == 0x124 && watch_x >= 0 && watch_y >= 0) {
                    uint16_t *fb = ref_get_fb();
                    uint32_t row_width = ref_get_row_width();
                    uint32_t draw_off = ref_get_draw_offset();
                    uint32_t row_stride = row_width / 2;
                    uint32_t p = (uint32_t)watch_y * row_stride + (uint32_t)watch_x;
                    uint16_t before = *((uint16_t *)((uint8_t *)fb + draw_off) + p);
                    fprintf(stderr,
                            "[trace_test] REF watch before fastfill (%d,%d): draw=0x%x pixel=0x%04x rowWidth=%u\n",
                            watch_x, watch_y, draw_off, before, row_width);
                }
                /* swapbufferCMD presents the buffer that was being drawn
                 * immediately before the command write. */
                uint32_t pre_swap_draw_offset = 0;
                    if (reg == 0x128)
                        pre_swap_draw_offset = ref_get_draw_offset();

                    ref_write_reg(addr, e->data);

                    if (!ref_only) {
                        if (reg == 0x128)
                            sim_idle_wait();

                        sim_write(addr, e->data);

                        if (reg == 0x128)
                            sim_idle_wait();

                        /* Swapbuffer is the only replay synchronization point.
                         * Triangle / fastfill writes rely on normal bus backpressure. */
                    }

                    if (reg == 0x080 || reg == 0x100 || reg == 0x124) {
                        tri_count++;
                        if (reg == 0x124 && watch_x >= 0 && watch_y >= 0) {
                            uint16_t *fb = ref_get_fb();
                            uint32_t row_width = ref_get_row_width();
                            uint32_t draw_off = ref_get_draw_offset();
                            uint32_t row_stride = row_width / 2;
                            uint32_t p = (uint32_t)watch_y * row_stride + (uint32_t)watch_x;
                            uint16_t after = *((uint16_t *)((uint8_t *)fb + draw_off) + p);
                            fprintf(stderr,
                                    "[trace_test] REF watch after fastfill (%d,%d): draw=0x%x pixel=0x%04x rowWidth=%u\n",
                                    watch_x, watch_y, draw_off, after, row_width);
                        }
                    }
                    if (reg == 0x128) {
                        swap_cmd_count++;
                        have_presented_offset = true;
                        presented_offset = pre_swap_draw_offset;
                    }
                    break;
                }

                case VOODOO_TRACE_WRITE_TEX_L: {
                    uint32_t offset = e->addr & 0x7FFFFF;
                    ref_write_tex(offset, e->data);
                    if (!ref_only)
                        sim_write(0x800000 | offset, e->data);
                    tex_write_count++;
                    break;
                }

                case VOODOO_TRACE_WRITE_FB_L: {
                    uint32_t offset = e->addr & 0x3FFFFF;
                    ref_write_fb(offset, e->data);
                    if (!ref_only)
                        sim_write(0x400000 | offset, e->data);
                    fb_write_count++;
                    break;
                }

                case VOODOO_TRACE_WRITE_REG_W: {
                    /* 16-bit register writes — sim only (ref model doesn't need them) */
                    if (!ref_only)
                        sim_write16(e->addr & 0x3FFFFF, (uint16_t)e->data);
                    break;
                }

                case VOODOO_TRACE_WRITE_FB_W: {
                    uint32_t offset = e->addr & 0x3FFFFF;
                    ref_write_fb_w(offset, (uint16_t)e->data);
                    if (!ref_only)
                        sim_write16(0x400000 | offset, (uint16_t)e->data);
                    fb_write_count++;
                    break;
                }

                case VOODOO_TRACE_VSYNC:
                case VOODOO_TRACE_SWAP:
                case VOODOO_TRACE_CONFIG:
                case VOODOO_TRACE_FRAME_END:
                    /* Informational — skip */
                    break;

                default:
                    /* Read operations etc. — skip */
                    break;
            }
        }
    }

    fprintf(stderr, "[trace_test] Replay complete: %u packed entries (%llu logical commands), %d triangles, %d tex writes, %d fb writes, %d swaps\n",
            num_entries, (unsigned long long)logical_cmds, tri_count, tex_write_count, fb_write_count, swap_cmd_count);
    fprintf(stderr,
            "[trace_test] Cmd histogram(logical): WR_REG=%llu WR_FB_L=%llu WR_FB_W=%llu WR_TEX=%llu WR_CMDFIFO=%llu RD_REG=%llu RD_FB_L=%llu RD_FB_W=%llu SWAP=%llu\n",
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_REG_L],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_FB_L],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_FB_W],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_TEX_L],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_CMDFIFO],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_REG_L],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_FB_L],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_FB_W],
            (unsigned long long)cmd_type_counts[VOODOO_TRACE_SWAP]);
    fprintf(stderr,
            "[trace_test] Key reg writes: fbzMode=%u(last=0x%08x) fbzColorPath=%u(last=0x%08x) texMode0=%u(last=0x%08x) texMode1=%u(last=0x%08x) paletteIndexed=%u\n",
            fbzmode_writes, fbzmode_last, fbzcp_writes, fbzcp_last, texmode0_writes, texmode0_last, texmode1_writes, texmode1_last, pal_index_writes);
    {
        uint32_t px_in = ref_get_fbi_pixels_in();
        uint32_t px_out = ref_get_fbi_pixels_out();
        uint32_t z_fail = ref_get_fbi_zfunc_fail();
        uint32_t a_fail = ref_get_fbi_afunc_fail();
        uint32_t ck_fail = ref_get_fbi_chroma_fail();
        uint32_t fbz = ref_get_fbz_mode();
        uint32_t za = ref_get_za_color();
        uint32_t cov = ref_get_triangle_coverage_pixels();
        uint32_t cov_total = ref_get_triangle_coverage_total();
        uint32_t rgb_writes = ref_get_triangle_color_writes();
        uint32_t black_writes = ref_get_triangle_black_writes();
        uint32_t depth_only = ref_get_triangle_depth_only_updates();
        uint32_t tri_tex = ref_get_triangles_textured();
        uint32_t tri_flat = ref_get_triangles_untextured();
        uint32_t tri_all_black = ref_get_triangles_all_black_writes();
        uint32_t pal0_nz = ref_get_palette_nonzero_count(0);
        uint32_t pal1_nz = ref_get_palette_nonzero_count(1);
        fprintf(stderr,
                "[trace_test] Ref reject stats: in=%u out=%u zFail=%u aFail=%u chromaFail=%u fbzMode=0x%08x zaColor=0x%08x\n",
                px_in, px_out, z_fail, a_fail, ck_fail, fbz, za);
        if (cov_total > 0) {
            fprintf(stderr,
                    "[trace_test] Triangle screen coverage: %u/%u pixels (%.2f%%)\n",
                    cov, cov_total, (cov * 100.0) / cov_total);
        }
        if (rgb_writes || depth_only) {
            fprintf(stderr,
                    "[trace_test] Triangle write mix: rgbWrites=%u (black=%u, %.2f%%), depthOnly=%u\n",
                    rgb_writes, black_writes,
                    rgb_writes ? (black_writes * 100.0) / rgb_writes : 0.0,
                    depth_only);
        }
        if (tri_tex || tri_flat) {
            fprintf(stderr,
                    "[trace_test] Triangle mode mix: textured=%u untextured=%u allBlackTriangles=%u\n",
                    tri_tex, tri_flat, tri_all_black);
        }
        fprintf(stderr,
                "[trace_test] Palette nonzero entries: TMU0=%u TMU1=%u\n",
                pal0_nz, pal1_nz);
        if (tri_tex > 100 && pal_index_writes == 0 && pal0_nz <= 8) {
            fprintf(stderr,
                    "[trace_test] WARNING: PAL8 texturing active but TMU0 palette looks mostly empty "
                    "(nonzero=%u, indexed writes in trace=%u). State snapshot is likely missing palette contents.\n",
                    pal0_nz, pal_index_writes);
        }
    }

    if (!have_presented_offset) {
        presented_offset = ref_get_draw_offset();
        fprintf(stderr,
                "[trace_test] WARNING: Trace contains no swapbufferCMD; "
                "falling back to final draw_offset=0x%x\n",
                presented_offset);
    }

    /* Wait for pipeline to drain before reading back */
    if (!ref_only)
        sim_idle_wait();

    if (!ref_only)
        sim_flush_fb_cache();

    if (!ref_only) {
        fprintf(stderr, "[trace_test] Sim swap count after replay: %u\n", sim_get_swap_count());
    }

    /* ---------------------------------------------------------------
     * Texture memory comparison (sim vs ref)
     * --------------------------------------------------------------- */
    if (!ref_only) {
        uint8_t *ref_tex = ref_get_tex();
        uint32_t tex_size = ref_get_tex_size();
        uint32_t tex_words = tex_size / 4;
        if (tex_words > 2 * 1024 * 1024) tex_words = 2 * 1024 * 1024; /* cap at 8MB */

        uint32_t *sim_tex = (uint32_t *)malloc(tex_words * 4);
        sim_read_tex(0, sim_tex, tex_words);

        uint32_t tex_mismatches = 0;
        uint32_t first_mismatch_addr = 0;
        for (uint32_t i = 0; i < tex_words; i++) {
            uint32_t ref_word = *(uint32_t *)(ref_tex + i * 4);
            if (sim_tex[i] != ref_word) {
                if (tex_mismatches < 10) {
                    fprintf(stderr, "[tex_cmp] MISMATCH at 0x%06x: sim=0x%08x ref=0x%08x\n",
                            i * 4, sim_tex[i], ref_word);
                }
                if (tex_mismatches == 0) first_mismatch_addr = i * 4;
                tex_mismatches++;
            }
        }
        fprintf(stderr, "[tex_cmp] Compared %u words (%u bytes): %u mismatches\n",
                tex_words, tex_words * 4, tex_mismatches);
        if (tex_mismatches == 0)
            fprintf(stderr, "[tex_cmp] Texture memory MATCHES between sim and ref\n");
        else
            fprintf(stderr, "[tex_cmp] First mismatch at 0x%06x, total %u/%u words differ\n",
                    first_mismatch_addr, tex_mismatches, tex_words);
        free(sim_tex);
    }

    /* ---------------------------------------------------------------
     * Comparison
     * --------------------------------------------------------------- */

    uint32_t draw_offset = ref_get_draw_offset();
    uint32_t front_offset = ref_get_front_offset();
    uint32_t row_width   = ref_get_row_width();  /* bytes */
    int row_stride = row_width / 2;              /* pixels (16-bit) */
    /* Strict frame selection: use the last buffer explicitly presented by
     * swapbufferCMD (sampled pre-swap), no heuristic fallbacks. */
    uint32_t fb_offset = presented_offset;
    /* Sim color plane now matches the 16-bit RGB565 layout: one pixel = 2 bytes. */
    uint32_t sim_fb_offset = presented_offset;

    fprintf(stderr, "[trace_test] FB layout: front=0x%x draw=0x%x presented=0x%x row_width=%u (%d pixels)\n",
            front_offset, draw_offset, fb_offset, row_width, row_stride);

    int mismatches = 0;
    int total_pixels = disp_width * disp_height;
    bool stale_frame_risk = false;

    /* Get reference framebuffer (offset to front/display buffer) */
    uint16_t *ref_fb_base = ref_get_fb();
    uint16_t *ref_fb = (uint16_t *)((uint8_t *)ref_fb_base + fb_offset);
    if (draw_offset != front_offset) {
        uint32_t other_offset = (fb_offset == draw_offset) ? front_offset : draw_offset;
        uint32_t presented_black = 0;
        uint32_t other_black = 0;
        uint32_t presented_changed = 0;
        uint32_t other_changed = 0;
        uint32_t total = (uint32_t)disp_width * (uint32_t)disp_height;
        uint16_t *other_fb = (uint16_t *)((uint8_t *)ref_fb_base + other_offset);
        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint32_t p = (uint32_t)y * (uint32_t)row_stride + (uint32_t)x;
                uint16_t cur_presented = ref_fb[p];
                uint16_t cur_other = other_fb[p];
                if (cur_presented == 0)
                    presented_black++;
                if (cur_other == 0)
                    other_black++;
                if (!ref_fb_initial.empty()) {
                    uint32_t pi = (fb_offset / 2) + p;
                    uint32_t oi = (other_offset / 2) + p;
                    if (pi < ref_fb_initial.size() && cur_presented != ref_fb_initial[pi])
                        presented_changed++;
                    if (oi < ref_fb_initial.size() && cur_other != ref_fb_initial[oi])
                        other_changed++;
                }
            }
        }
        fprintf(stderr,
                "[trace_test] Ref buffer blackness: presented@0x%x=%u/%u (%.2f%%), other@0x%x=%u/%u (%.2f%%)\n",
                fb_offset, presented_black, total, (presented_black * 100.0) / total,
                other_offset, other_black, total, (other_black * 100.0) / total);
        if (!ref_fb_initial.empty()) {
            fprintf(stderr,
                    "[trace_test] Ref buffer delta vs state: presented@0x%x changed=%u/%u (%.2f%%), other@0x%x changed=%u/%u (%.2f%%)\n",
                    fb_offset, presented_changed, total, (presented_changed * 100.0) / total,
                    other_offset, other_changed, total, (other_changed * 100.0) / total);
        }
    }

    /* Get sim framebuffer.
     * CoreSim now uses split planes (color + aux), with RGB565 packed in the
     * color plane as two 16-bit pixels per 32-bit word. Runtime stride is still
     * derived from fbiInit1[7:4] * 64 pixels. */
    int sim_pixel_stride = 640; /* default for 640x480 */
    uint32_t sim_buffer_offset = 0;
    if (!ref_only) {
        /* Read fbiInit1 from sim — has the final value after state + trace */
        uint32_t fbiInit1_val = sim_read(0x214);
        uint32_t fbiInit2_val = sim_read(0x218);
        sim_buffer_offset = ((fbiInit2_val >> 11) & 0x1FF) * 4096;
        int tiles = (fbiInit1_val >> 4) & 0xF;
        if (tiles > 0) sim_pixel_stride = tiles * 64;
        fprintf(stderr, "[trace_test] Sim pixel stride: %d (fbiInit1=0x%08x)\n",
                sim_pixel_stride, fbiInit1_val);
    }
    std::vector<uint16_t> sim_fb_extracted;
    uint16_t *sim_fb = nullptr;

    if (!ref_only) {
        /* sim_read_fb() is word-addressed. Support 2-byte offsets by aligning
         * down and tracking the initial halfword lane. */
        uint32_t aligned_sim_fb_offset = sim_fb_offset & ~0x3u;
        uint32_t start_halfword = (sim_fb_offset >> 1) & 0x1u;
        uint32_t total_halfwords = (uint32_t)sim_pixel_stride * (uint32_t)disp_height + start_halfword;
        uint32_t sim_total_words = (total_halfwords + 1) / 2;
        std::vector<uint32_t> sim_fb_raw(sim_total_words);
        sim_read_fb(aligned_sim_fb_offset, sim_fb_raw.data(), sim_total_words);

        /* Extract RGB565 pixels from packed 32-bit words into a flat buffer
         * with the same row_stride as the reference. */
        sim_fb_extracted.resize(row_stride * disp_height, 0);
        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint32_t p = start_halfword + (uint32_t)y * (uint32_t)sim_pixel_stride + (uint32_t)x;
                uint32_t word = sim_fb_raw[p >> 1];
                uint16_t pixel = (p & 1) ? (uint16_t)((word >> 16) & 0xFFFF) : (uint16_t)(word & 0xFFFF);
                sim_fb_extracted[y * row_stride + x] = pixel;
            }
        }
        sim_fb = sim_fb_extracted.data();
    }

    /* RGB24 buffers for PNG output */
    std::vector<uint8_t> ref_rgb(disp_width * disp_height * 3);
    std::vector<uint8_t> sim_rgb;
    std::vector<uint8_t> diff_rgb;

    if (!ref_only) {
        sim_rgb.resize(disp_width * disp_height * 3);
        diff_rgb.resize(disp_width * disp_height * 3, 0);
    }

    /* Compare and convert */
    for (int y = 0; y < disp_height; y++) {
        /* Convert reference row */
        rgb565_to_rgb24(&ref_fb[y * row_stride], &ref_rgb[y * disp_width * 3], disp_width);

        if (!ref_only) {
            /* Convert sim row */
            rgb565_to_rgb24(&sim_fb[y * row_stride], &sim_rgb[y * disp_width * 3], disp_width);

            /* Compare */
            for (int x = 0; x < disp_width; x++) {
                uint16_t rp = ref_fb[y * row_stride + x];
                uint16_t sp = sim_fb[y * row_stride + x];

                if (rp != sp) {
                    /* Check if within colour tolerance */
                    int rr = (rp >> 11) & 0x1f, rg = (rp >> 5) & 0x3f, rb = rp & 0x1f;
                    int sr = (sp >> 11) & 0x1f, sg = (sp >> 5) & 0x3f, sb = sp & 0x1f;
                    int dr = abs(rr - sr), dg = abs(rg - sg), db = abs(rb - sb);

                    if (dr > color_tolerance || dg > color_tolerance || db > color_tolerance) {
                        mismatches++;
                        /* Mark in diff image: red channel = ref, green = sim, blue = 0 */
                        int idx = (y * disp_width + x) * 3;
                        diff_rgb[idx + 0] = 255;  /* red = mismatch */
                        diff_rgb[idx + 1] = 0;
                        diff_rgb[idx + 2] = 0;

                        if (mismatches <= 10) {
                            fprintf(stderr, "  MISMATCH at (%d,%d): ref=0x%04x sim=0x%04x\n",
                                    x, y, rp, sp);
                        }
                        /* Count sim-black pixels where ref has visible color */
                        if (sp == 0x0000 && rp > 0x0000) {
                            static int black_count = 0;
                            if (black_count < 20) {
                                fprintf(stderr, "  SIM-BLACK at (%d,%d): ref=0x%04x\n", x, y, rp);
                            }
                            black_count++;
                        }
                    }
                }
            }
        }
    }

    /* Guard against stale-frame selection: if the opposite color buffer
     * matches dramatically better, flag as a frame-slip risk. */
    if (!ref_only && sim_buffer_offset > 0 &&
        (sim_fb_offset == 0 || sim_fb_offset == sim_buffer_offset)) {
        uint32_t alt_sim_fb_offset = (sim_fb_offset == 0) ? sim_buffer_offset : 0;
        uint32_t aligned_alt_offset = alt_sim_fb_offset & ~0x3u;
        uint32_t alt_start_halfword = (alt_sim_fb_offset >> 1) & 0x1u;
        uint32_t alt_total_halfwords =
            (uint32_t)sim_pixel_stride * (uint32_t)disp_height + alt_start_halfword;
        uint32_t alt_total_words = (alt_total_halfwords + 1) / 2;
        std::vector<uint32_t> alt_raw(alt_total_words);
        std::vector<uint16_t> alt_extracted(row_stride * disp_height, 0);
        sim_read_fb(aligned_alt_offset, alt_raw.data(), alt_total_words);

        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint32_t p =
                    alt_start_halfword + (uint32_t)y * (uint32_t)sim_pixel_stride + (uint32_t)x;
                uint32_t word = alt_raw[p >> 1];
                uint16_t pixel = (p & 1) ? (uint16_t)((word >> 16) & 0xFFFF)
                                         : (uint16_t)(word & 0xFFFF);
                alt_extracted[y * row_stride + x] = pixel;
            }
        }

        int alt_mismatches = 0;
        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint16_t rp = ref_fb[y * row_stride + x];
                uint16_t sp = alt_extracted[y * row_stride + x];
                if (rp == sp)
                    continue;
                int rr = (rp >> 11) & 0x1f, rg = (rp >> 5) & 0x3f, rb = rp & 0x1f;
                int sr = (sp >> 11) & 0x1f, sg = (sp >> 5) & 0x3f, sb = sp & 0x1f;
                int dr = abs(rr - sr), dg = abs(rg - sg), db = abs(rb - sb);
                if (dr > color_tolerance || dg > color_tolerance || db > color_tolerance)
                    alt_mismatches++;
            }
        }

        fprintf(stderr, "[trace_test] Alt buffer check: reading=0x%x mismatches=%d\n",
                alt_sim_fb_offset, alt_mismatches);
        if (alt_mismatches + 64 < mismatches && (mismatches - alt_mismatches) > 256) {
            stale_frame_risk = true;
            fprintf(stderr, "[trace_test] ERROR: alternate framebuffer matches better (stale-frame risk)\n");
        }
    }

    /* Write PNG output */
    if (output_dir) {
        char path[512];

        snprintf(path, sizeof(path), "%s/%s_ref.png", output_dir, name.c_str());
        stbi_write_png(path, disp_width, disp_height, 3, ref_rgb.data(), disp_width * 3);
        fprintf(stderr, "[trace_test] Wrote %s\n", path);

        if (!ref_only) {
            snprintf(path, sizeof(path), "%s/%s_sim.png", output_dir, name.c_str());
            stbi_write_png(path, disp_width, disp_height, 3, sim_rgb.data(), disp_width * 3);
            fprintf(stderr, "[trace_test] Wrote %s\n", path);

            snprintf(path, sizeof(path), "%s/%s_diff.png", output_dir, name.c_str());
            stbi_write_png(path, disp_width, disp_height, 3, diff_rgb.data(), disp_width * 3);
            fprintf(stderr, "[trace_test] Wrote %s\n", path);
        }
    }

    /* Summary */
    if (ref_only) {
        fprintf(stderr, "[trace_test] Reference-only mode: %d pixels rendered\n", total_pixels);
    } else {
        fprintf(stderr, "[trace_test] %d/%d pixels mismatched", mismatches, total_pixels);
        if (color_tolerance > 0)
            fprintf(stderr, " (tolerance=%d)", color_tolerance);
        fprintf(stderr, "\n");
        if (stale_frame_risk) {
            fprintf(stderr, "[trace_test] ERROR: stale-frame risk detected by alternate buffer check\n");
        }
    }

    /* Cleanup */
    free(trace_data);
    ref_trace_close();
    ref_shutdown();
    if (!ref_only) sim_shutdown();

    /* Use _exit to avoid hanging in Verilator's static destructors */
    int ok = (ref_only || mismatches <= max_mismatches) ? 1 : 0;
    if (!ref_only && stale_frame_risk)
        ok = 0;
    _exit(ok ? 0 : 1);
}
