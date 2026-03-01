/*
 * trace_test.cpp — Replay Voodoo traces through 86Box reference model
 * and Verilator CoreSim, comparing framebuffer output pixel-by-pixel.
 *
 * Usage:
 *   trace_test <path> [--ref-only] [--output-dir DIR] [--max-mismatches N]
 *                     [--color-tolerance N] [--width W] [--height H]
 *
 *   path: .bin trace file, or directory containing trace.bin + optional state.bin
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
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

/* Convert RGB565 to 24-bit RGB for PNG output */
static void rgb565_to_rgb24(const uint16_t *src, uint8_t *dst, int count) {
    for (int i = 0; i < count; i++) {
        uint16_t px = src[i];
        uint8_t r = (px >> 8) & 0xf8; r |= r >> 5;
        uint8_t g = (px >> 3) & 0xfc; g |= g >> 6;
        uint8_t b = (px << 3) & 0xf8; b |= b >> 5;
        dst[i * 3 + 0] = r;
        dst[i * 3 + 1] = g;
        dst[i * 3 + 2] = b;
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
    /* Parse arguments */
    const char *trace_path = nullptr;
    const char *output_dir = nullptr;
    bool ref_only = false;
    int max_mismatches = 100;
    int color_tolerance = 0;
    int disp_width  = 640;
    int disp_height = 480;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ref-only") == 0) {
            ref_only = true;
        } else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
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
        fprintf(stderr, "Usage: trace_test <path> [--ref-only] [--output-dir DIR] "
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
    if (!state_file.empty()) {
        state_data = read_file(state_file.c_str(), &state_size);
        if (state_data) {
            const voodoo_state_header_t *shdr = (const voodoo_state_header_t *)state_data;
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
                    reg_addr == 0x120)                          /* nopCMD */
                    continue;
                sim_write(regs[i].addr, regs[i].value);
            }
            ptr += shdr->reg_count * sizeof(voodoo_state_reg_t);

            /* Write framebuffer to sim */
            sim_write_fb_bulk(0, (const uint32_t *)ptr, shdr->fb_size / 4);
            ptr += shdr->fb_size;

            /* Write texture memory to sim */
            sim_write_tex_bulk(0, (const uint32_t *)ptr, shdr->tex_size / 4);

            fprintf(stderr, "[trace_test] State loaded into sim\n");
        }

        free(state_data);
        state_data = nullptr;
    }

    /* Replay trace entries */
    int tri_count = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        const voodoo_trace_entry_t *e = &entries[i];

        switch (e->cmd_type) {
            case VOODOO_TRACE_WRITE_REG_L: {
                uint32_t addr = e->addr & 0x3FFFFF;
                ref_write_reg(addr, e->data);

                if (!ref_only) {
                    sim_write(addr, e->data);

                    /* On triangle commands, wait for pipeline to drain */
                    uint32_t reg = addr & 0x3fc;
                    if (reg == 0x080 || reg == 0x100 || reg == 0x124) {
                        sim_idle_wait();
                        tri_count++;
                    }
                }
                break;
            }

            case VOODOO_TRACE_WRITE_TEX_L: {
                uint32_t offset = e->addr & 0x7FFFFF;
                ref_write_tex(offset, e->data);
                if (!ref_only)
                    sim_write(0x800000 | offset, e->data);
                break;
            }

            case VOODOO_TRACE_WRITE_FB_L: {
                uint32_t offset = e->addr & 0x3FFFFF;
                ref_write_fb(offset, e->data);
                if (!ref_only)
                    sim_write(0x400000 | offset, e->data);
                break;
            }

            case VOODOO_TRACE_WRITE_REG_W: {
                /* 16-bit register writes — sim only (ref model doesn't need them) */
                if (!ref_only)
                    sim_write(e->addr & 0x3FFFFF, e->data);
                break;
            }

            case VOODOO_TRACE_WRITE_FB_W: {
                uint32_t offset = e->addr & 0x3FFFFF;
                ref_write_fb_w(offset, (uint16_t)e->data);
                if (!ref_only)
                    sim_write(0x400000 | offset, e->data);
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

    fprintf(stderr, "[trace_test] Replay complete: %u entries, %d triangles\n",
            num_entries, tri_count);

    /* ---------------------------------------------------------------
     * Comparison
     * --------------------------------------------------------------- */

    uint32_t draw_offset = ref_get_draw_offset();
    uint32_t front_offset = ref_get_front_offset();
    uint32_t row_width   = ref_get_row_width();  /* bytes */
    int row_stride = row_width / 2;              /* pixels (16-bit) */

    /* Use front buffer for screenshot — contains the last completed frame.
     * After swapbufferCMD, draw_offset points to the next render target,
     * while front_offset points to the just-rendered content. */
    uint32_t fb_offset = front_offset;

    fprintf(stderr, "[trace_test] FB layout: front=0x%x draw=0x%x row_width=%u (%d pixels)\n",
            front_offset, draw_offset, row_width, row_stride);

    int mismatches = 0;
    int total_pixels = disp_width * disp_height;

    /* Get reference framebuffer (offset to front/display buffer) */
    uint16_t *ref_fb_base = ref_get_fb();
    uint16_t *ref_fb = (uint16_t *)((uint8_t *)ref_fb_base + fb_offset);

    /* Get sim framebuffer.
     * CoreSim stores pixels as 32-bit words: [depth:16][rgb565:16]
     * with a fixed stride of 1024 pixels (4096 bytes) per row.
     * We extract just the RGB565 color from each word. */
    static const int SIM_PIXEL_STRIDE = 1024;
    std::vector<uint16_t> sim_fb_extracted;
    uint16_t *sim_fb = nullptr;

    if (!ref_only) {
        /* Read the sim framebuffer region covering all display rows */
        uint32_t sim_row_bytes = SIM_PIXEL_STRIDE * 4;  /* 4 bytes per pixel */
        uint32_t sim_total_words = SIM_PIXEL_STRIDE * disp_height;
        std::vector<uint32_t> sim_fb_raw(sim_total_words);
        sim_read_fb(fb_offset, sim_fb_raw.data(), sim_total_words);

        /* Extract RGB565 color from each 32-bit word into a flat buffer
         * with the same row_stride as the ref, for easy comparison. */
        sim_fb_extracted.resize(row_stride * disp_height, 0);
        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint32_t word = sim_fb_raw[y * SIM_PIXEL_STRIDE + x];
                sim_fb_extracted[y * row_stride + x] = (uint16_t)(word & 0xFFFF);
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
                    }
                }
            }
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
    }

    /* Cleanup */
    free(trace_data);
    ref_shutdown();
    if (!ref_only) sim_shutdown();

    /* Use _exit to avoid hanging in Verilator's static destructors */
    _exit((ref_only || mismatches == 0) ? 0 : 1);
}
