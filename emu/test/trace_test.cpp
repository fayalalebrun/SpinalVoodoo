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
#include <cerrno>
#include <algorithm>
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

#include "../../emu/trace/trace_replay_backend.h"
#include "../../emu/trace/trace_replay_image.h"
#include "../../emu/trace/trace_replay_io.h"

/* Sim harness (optional — omitted in --ref-only mode) */
#include "sim_harness.h"

/* -------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------- */

static bool ensure_directory(const char *path) {
    if (!path || !path[0]) return false;
    std::string cur;
    if (path[0] == '/') cur = "/";
    for (const char *p = path; *p; ++p) {
        if (*p == '/') {
            if (!cur.empty() && !traceReplayIsDirectory(cur.c_str()) && mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
                return false;
            }
        }
        cur.push_back(*p);
    }
    if (!traceReplayIsDirectory(cur.c_str()) && mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

static uint8_t srgb_lut[256];

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

static void json_write_string(FILE *f, const std::string &s) {
    fputc('"', f);
    for (char c : s) {
        switch (c) {
            case '\\': fputs("\\\\", f); break;
            case '"': fputs("\\\"", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if ((unsigned char)c < 0x20) fprintf(f, "\\u%04x", (unsigned char)c);
                else fputc(c, f);
                break;
        }
    }
    fputc('"', f);
}

struct ProgressSnapshot {
    uint32_t entry = 0;
    uint64_t logical = 0;
    uint32_t cmd_type = 0;
    uint32_t addr = 0;
    uint32_t data = 0;
    uint32_t count = 0;
    uint64_t sim_cycle = 0;
    uint64_t sim_reads = 0;
    uint64_t sim_writes = 0;
    uint32_t sim_pixels_in = 0;
    uint32_t sim_pixels_out = 0;
};

struct RegHotspot {
    uint32_t reg = 0;
    uint64_t cycles = 0;
    uint32_t count = 0;
};

class SimReplayBackend : public TraceReplayBackend {
public:
    bool useDe10RegisterMap() const override {
#ifdef SIM_INTERFACE_DE10
        return true;
#else
        return false;
#endif
    }

    bool writeReg32(uint32_t mappedAddr, uint32_t, uint32_t value) override {
        sim_write(mappedAddr, value);
        return !sim_stalled();
    }
    bool writeReg16(uint32_t mappedAddr, uint32_t, uint16_t value) override {
        sim_write16(mappedAddr, value);
        return !sim_stalled();
    }
    bool writeFb32(uint32_t byteOffset, uint32_t value) override {
        sim_write(0x400000 | byteOffset, value);
        return !sim_stalled();
    }
    bool writeFb16(uint32_t byteOffset, uint16_t value) override {
        sim_write16(0x400000 | byteOffset, value);
        return !sim_stalled();
    }
    bool writeTex32(uint32_t byteOffset, uint32_t value) override {
        sim_write(0x800000 | byteOffset, value);
        return !sim_stalled();
    }
    bool writeFbBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
        std::vector<uint32_t> words((size + 3) / 4, 0);
        memcpy(words.data(), data, size);
        sim_write_fb_bulk(byteOffset, words.data(), words.size());
        return !sim_stalled();
    }
    bool writeTexBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
        std::vector<uint32_t> words((size + 3) / 4, 0);
        memcpy(words.data(), data, size);
        sim_write_tex_bulk(byteOffset, words.data(), words.size());
        return !sim_stalled();
    }
    bool readFbWords(uint32_t byteOffset, uint32_t *dst, uint32_t wordCount) override {
        sim_read_fb(byteOffset, dst, wordCount);
        return true;
    }
    bool idleWait() override {
        sim_idle_wait();
        return !sim_stalled();
    }
    bool invalidateFbCache() override {
        sim_invalidate_fb_cache();
        return !sim_stalled();
    }
    bool flushFbCache() override {
        sim_flush_fb_cache();
        return !sim_stalled();
    }
    bool setSwapCount(uint32_t count) override {
        sim_set_swap_count(count);
        return !sim_stalled();
    }
};

static uint32_t remap_backend_reg_addr(uint32_t addr) {
    return traceReplayRemapBackendRegAddr(
#ifdef SIM_INTERFACE_DE10
        true,
#else
        false,
#endif
        addr);
}

/* -------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------- */

int main(int argc, char **argv) {
    traceReplayInitSrgbLut(srgb_lut);

    int watch_x = -1;
    int watch_y = -1;
    if (const char *wx = getenv("SIM_WATCH_X")) watch_x = atoi(wx);
    if (const char *wy = getenv("SIM_WATCH_Y")) watch_y = atoi(wy);
    const bool sim_skip_final_idle = getenv("SIM_SKIP_FINAL_IDLE") != nullptr;
    const bool skip_sim_fb_state = getenv("TRACE_TEST_SKIP_SIM_FB_STATE") != nullptr;
    const bool dump_ctrl_regs = getenv("TRACE_TEST_DUMP_CTRL_REGS") != nullptr;
    const uint64_t sim_cycle_limit = getenv("SIM_CYCLE_LIMIT") ? strtoull(getenv("SIM_CYCLE_LIMIT"), nullptr, 0) : 0ull;
    uint32_t sim_pixels_in_final = 0;
    uint32_t sim_pixels_out_final = 0;
    uint64_t sim_cycles_final = 0;
    uint64_t sim_read_ticks_final = 0;
    uint64_t sim_read_count_final = 0;
    uint64_t sim_write_ticks_final = 0;
    uint64_t sim_write_count_final = 0;
    uint32_t sim_fb_fill_hits_final = 0;
    uint32_t sim_fb_fill_misses_final = 0;
    uint32_t sim_fb_fill_burst_count_final = 0;
    uint32_t sim_fb_fill_burst_beats_final = 0;
    uint32_t sim_fb_fill_stall_cycles_final = 0;
    uint32_t sim_fb_write_stall_cycles_final = 0;
    uint32_t sim_fb_write_drain_count_final = 0;
    uint32_t sim_fb_write_full_drain_count_final = 0;
    uint32_t sim_fb_write_partial_drain_count_final = 0;
    uint32_t sim_fb_write_drain_reason_full_count_final = 0;
    uint32_t sim_fb_write_drain_reason_rotate_count_final = 0;
    uint32_t sim_fb_write_drain_reason_flush_count_final = 0;
    uint32_t sim_fb_write_drain_dirty_word_total_final = 0;
    uint32_t sim_fb_write_rotate_blocked_cycles_final = 0;
    uint32_t sim_fb_write_single_word_drain_count_final = 0;
    uint32_t sim_fb_write_single_word_drain_start_at_zero_count_final = 0;
    uint32_t sim_fb_write_single_word_drain_start_at_last_count_final = 0;
    uint32_t sim_fb_write_rotate_adjacent_line_count_final = 0;
    uint32_t sim_fb_write_rotate_same_line_gap_count_final = 0;
    uint32_t sim_fb_write_rotate_other_line_count_final = 0;
    uint32_t sim_fb_mem_color_write_cmd_count_final = 0;
    uint32_t sim_fb_mem_aux_write_cmd_count_final = 0;
    uint32_t sim_fb_mem_color_read_cmd_count_final = 0;
    uint32_t sim_fb_mem_aux_read_cmd_count_final = 0;
    uint32_t sim_fb_mem_lfb_read_cmd_count_final = 0;
    uint32_t sim_fb_mem_color_write_blocked_cycles_final = 0;
    uint32_t sim_fb_mem_aux_write_blocked_cycles_final = 0;
    uint32_t sim_fb_mem_color_read_blocked_cycles_final = 0;
    uint32_t sim_fb_mem_aux_read_blocked_cycles_final = 0;
    uint32_t sim_fb_mem_lfb_read_blocked_cycles_final = 0;
    uint32_t sim_fb_read_req_count_final = 0;
    uint32_t sim_fb_read_req_forward_step_count_final = 0;
    uint32_t sim_fb_read_req_backward_step_count_final = 0;
    uint32_t sim_fb_read_req_same_word_count_final = 0;
    uint32_t sim_fb_read_req_same_line_count_final = 0;
    uint32_t sim_fb_read_req_other_count_final = 0;
    uint32_t sim_fb_read_single_beat_burst_count_final = 0;
    uint32_t sim_fb_read_multi_beat_burst_count_final = 0;
    uint32_t sim_fb_read_max_queue_occupancy_final = 0;
    uint64_t worst_write_cycles = 0;
    uint32_t worst_write_entry = 0;
    uint32_t worst_write_addr = 0;
    uint32_t worst_write_data = 0;
    std::vector<ProgressSnapshot> progress_snapshots;
    std::vector<RegHotspot> sim_write_hotspots;
#ifdef SIM_INTERFACE_DE10
    const uint64_t sim_profile_writes = getenv("SIM_PROFILE_WRITES") ? strtoull(getenv("SIM_PROFILE_WRITES"), nullptr, 0) : 0ull;
    uint32_t slow_write_logs = 0;
    std::vector<uint64_t> sim_write_cycles_by_reg(0x400 / 4, 0);
    std::vector<uint32_t> sim_write_count_by_reg(0x400 / 4, 0);
#endif

    /* Parse arguments */
    const char *trace_path = nullptr;
    const char *output_dir = nullptr;
    const char *profile_json = nullptr;
    const char *ref_trace_jsonl = nullptr;
    bool ref_only = false;
    bool sim_replay_only = false;
    int max_mismatches = 0;
    int max_sim_black_mismatches = -1;
    int max_parity_mismatch_skew = -1;
    int color_tolerance = 0;
    int disp_width  = 640;
    int disp_height = 480;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ref-only") == 0) {
            ref_only = true;
        } else if (strcmp(argv[i], "--sim-replay-only") == 0) {
            sim_replay_only = true;
        } else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (strcmp(argv[i], "--profile-json") == 0 && i + 1 < argc) {
            profile_json = argv[++i];
        } else if (strcmp(argv[i], "--ref-trace-jsonl") == 0 && i + 1 < argc) {
            ref_trace_jsonl = argv[++i];
        } else if (strcmp(argv[i], "--max-mismatches") == 0 && i + 1 < argc) {
            max_mismatches = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max-sim-black-mismatches") == 0 && i + 1 < argc) {
            max_sim_black_mismatches = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max-parity-mismatch-skew") == 0 && i + 1 < argc) {
            max_parity_mismatch_skew = atoi(argv[++i]);
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
        fprintf(stderr, "Usage: trace_test <path> [--ref-only] [--sim-replay-only] [--output-dir DIR] [--profile-json PATH] [--ref-trace-jsonl PATH] "
                "[--max-mismatches N] [--max-sim-black-mismatches N] [--max-parity-mismatch-skew N] [--color-tolerance N] [--width W] [--height H]\n");
        return 1;
    }

    /* Resolve paths */
    TraceReplayFiles replay_files;
    traceReplayResolveFiles(trace_path, &replay_files);
    std::string trace_file = replay_files.traceFile;
    std::string state_file = replay_files.stateFile;

    std::string name = basename_no_ext(trace_path);
    std::string profile_json_path;
    if (profile_json) {
        profile_json_path = profile_json;
    } else if (output_dir) {
        profile_json_path = std::string(output_dir) + "/" + name + "_profile.json";
    }

    /* Read trace file */
    TraceReplayLoadedTrace loaded_trace;
    if (!traceReplayLoadTraceFile(trace_file, &loaded_trace)) {
        fprintf(stderr, "ERROR: Cannot read trace file: %s\n", trace_file.c_str());
        return 1;
    }

    const voodoo_trace_header_t *hdr = loaded_trace.header;

    int fb_mb  = hdr->fb_size_mb ? hdr->fb_size_mb : 4;
    int tex_mb = hdr->tex_size_mb ? hdr->tex_size_mb : 4;

    uint32_t num_entries = loaded_trace.entryCount;

    fprintf(stderr, "[trace_test] Trace: %s (%u entries, fb=%dMB tex=%dMB)\n",
            trace_file.c_str(), num_entries, fb_mb, tex_mb);

    const voodoo_trace_entry_t *entries = loaded_trace.entries;

    /* Read state file if present */
    uint8_t *state_data = nullptr;
    uint32_t state_size = 0;
    uint32_t state_row_width = 0;
    uint32_t state_draw_offset = 0;
    uint32_t state_aux_offset = 0;
    std::vector<uint16_t> ref_fb_initial;
    if (!state_file.empty()) {
        state_data = traceReplayReadFile(state_file.c_str(), &state_size);
        if (state_data) {
            if (state_size < sizeof(voodoo_state_header_t)) {
                fprintf(stderr, "ERROR: State file too small: %s\n", state_file.c_str());
                free(state_data);
                free(loaded_trace.traceData);
                return 1;
            }
            const voodoo_state_header_t *shdr = (const voodoo_state_header_t *)state_data;
            if (shdr->magic != VOODOO_STATE_MAGIC) {
                fprintf(stderr, "ERROR: Bad state magic: 0x%08x (expected 0x%08x)\n",
                        shdr->magic, VOODOO_STATE_MAGIC);
                free(state_data);
                free(loaded_trace.traceData);
                return 1;
            }
            if (shdr->version != VOODOO_STATE_VERSION) {
                fprintf(stderr,
                        "ERROR: Unsupported state version %u (expected %u). v1 is not supported.\n",
                        shdr->version, VOODOO_STATE_VERSION);
                free(state_data);
                free(loaded_trace.traceData);
                return 1;
            }
            state_row_width = shdr->row_width;
            state_draw_offset = shdr->draw_offset;
            state_aux_offset = shdr->aux_offset;
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
            SimReplayBackend simBackend;
            TraceReplayStateInfo simStateInfo;
            if (!traceReplayLoadState(simBackend, state_data, state_size, skip_sim_fb_state, &simStateInfo)) {
                fprintf(stderr, "ERROR: traceReplayLoadState failed\n");
                return 1;
            }
            if (!skip_sim_fb_state) {
                fprintf(stderr, "[trace_test] FB state loaded into sim: raw copy (%u bytes, aux_off=0x%x)\n",
                        shdr->fb_size, simStateInfo.auxOffset);
            } else {
                fprintf(stderr, "[trace_test] FB state load into sim skipped by TRACE_TEST_SKIP_SIM_FB_STATE\n");
            }
            const uint32_t buffer_offset = ((simStateInfo.fbiInit2 >> 11) & 0x1FFu) * 4096u;
            if (buffer_offset > 0) {
                const uint32_t draw_buffer = simStateInfo.drawOffset / buffer_offset;
                const uint32_t swap_count = 1u - (draw_buffer & 0x1u);
                fprintf(stderr, "[trace_test] Buffer alignment: draw_offset=0x%x buffer_offset=0x%x draw_buffer=%u swap_count=%u\n",
                        simStateInfo.drawOffset, buffer_offset, draw_buffer, swap_count);
            }
            fprintf(stderr, "[trace_test] State loaded into sim\n");
            if (dump_ctrl_regs) {
                const uint32_t regs_to_dump[] = {0x110, 0x118, 0x11c, 0x21c};
                for (uint32_t reg : regs_to_dump) {
                    uint32_t sim_val = sim_read(remap_backend_reg_addr(reg));
                    fprintf(stderr,
                            "[trace_test] Ctrl reg 0x%03x sim=0x%08x remap=0x%08x\n",
                            reg, sim_val, remap_backend_reg_addr(reg));
                }
            }
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
    bool replay_truncated = false;
    uint32_t last_entry_replayed = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        last_entry_replayed = i;
        const voodoo_trace_entry_t *e = &entries[i];
        uint32_t repeat = e->count ? e->count : 1;
        logical_cmds += repeat;
        if ((i % 5000) == 0) {
#ifdef SIM_INTERFACE_DE10
            uint64_t sim_cycle = ref_only ? 0 : sim_get_cycle();
            uint64_t sim_reads = ref_only ? 0 : sim_get_total_read_count();
            uint64_t sim_writes = ref_only ? 0 : sim_get_total_write_count();
            uint32_t sim_px_in = ref_only ? 0 : sim_read(0x14c);
            uint32_t sim_px_out = ref_only ? 0 : sim_read(0x15c);
            if (!ref_only) {
                ProgressSnapshot snap;
                snap.entry = i;
                snap.logical = logical_cmds;
                snap.cmd_type = e->cmd_type;
                snap.addr = e->addr;
                snap.data = e->data;
                snap.count = repeat;
                snap.sim_cycle = sim_cycle;
                snap.sim_reads = sim_reads;
                snap.sim_writes = sim_writes;
                snap.sim_pixels_in = sim_px_in;
                snap.sim_pixels_out = sim_px_out;
                progress_snapshots.push_back(snap);
            }
            fprintf(stderr,
                    "[trace_test] Progress: entry=%u/%u logical=%llu type=%u addr=0x%08x data=0x%08x count=%u simCycle=%llu simReads=%llu simWrites=%llu simPixelsIn=%u simPixelsOut=%u\n",
                    i, num_entries, (unsigned long long)logical_cmds,
                    (unsigned)e->cmd_type, (unsigned)e->addr,
                    (unsigned)e->data, (unsigned)repeat,
                    (unsigned long long)sim_cycle,
                    (unsigned long long)sim_reads,
                    (unsigned long long)sim_writes,
                    sim_px_in,
                    sim_px_out);
#else
            fprintf(stderr,
                    "[trace_test] Progress: entry=%u/%u logical=%llu type=%u addr=0x%08x data=0x%08x count=%u\n",
                    i, num_entries, (unsigned long long)logical_cmds,
                    (unsigned)e->cmd_type, (unsigned)e->addr,
                    (unsigned)e->data, (unsigned)repeat);
#endif
        }
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

                        const uint32_t sim_addr = remap_backend_reg_addr(addr);
#ifdef SIM_INTERFACE_DE10
                        uint64_t write_start = sim_get_cycle();
#endif
                        sim_write(sim_addr, e->data);
#ifdef SIM_INTERFACE_DE10
                        uint64_t write_cycles = sim_get_cycle() - write_start;
                        if (write_cycles > worst_write_cycles) {
                            worst_write_cycles = write_cycles;
                            worst_write_entry = i;
                            worst_write_addr = addr;
                            worst_write_data = e->data;
                        }
                        sim_write_cycles_by_reg[(sim_addr & 0x3fc) >> 2] += write_cycles;
                        sim_write_count_by_reg[(sim_addr & 0x3fc) >> 2] += 1;
                        if (sim_profile_writes > 0 && write_cycles >= sim_profile_writes && slow_write_logs < 64) {
                            fprintf(stderr,
                                    "[trace_test] Slow sim_write: entry=%u logical=%llu addr=0x%08x remap=0x%08x data=0x%08x cycles=%llu\n",
                                    i,
                                    (unsigned long long)logical_cmds,
                                    (unsigned)addr,
                                    (unsigned)sim_addr,
                                    (unsigned)e->data,
                                    (unsigned long long)write_cycles);
                            slow_write_logs++;
                        }
#endif

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
                        sim_write16(remap_backend_reg_addr(e->addr), (uint16_t)e->data);
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

            if (!ref_only && sim_stalled()) {
                replay_truncated = true;
                break;
            }

            if (!ref_only && sim_cycle_limit > 0 && sim_get_cycle() >= sim_cycle_limit) {
                replay_truncated = true;
                break;
            }
        }

        if (replay_truncated)
            break;
    }

    fprintf(stderr, "[trace_test] Replay %s: %u/%u packed entries (%llu logical commands), %d triangles, %d tex writes, %d fb writes, %d swaps\n",
            replay_truncated ? "truncated" : "complete",
            replay_truncated ? (last_entry_replayed + 1) : num_entries,
            num_entries,
            (unsigned long long)logical_cmds, tri_count, tex_write_count, fb_write_count, swap_cmd_count);
    if (replay_truncated) {
        fprintf(stderr, "[trace_test] Replay stopped at cycle limit %llu (entry=%u simCycle=%llu)\n",
                (unsigned long long)sim_cycle_limit,
                last_entry_replayed,
                (unsigned long long)(ref_only ? 0 : sim_get_cycle()));
    }
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
    if (!ref_only && !sim_skip_final_idle)
        sim_idle_wait();

    if (!ref_only && !sim_skip_final_idle)
        sim_flush_fb_cache();

    if (!ref_only) {
        fprintf(stderr, "[trace_test] Sim swap count after replay: %u\n", sim_get_swap_count());
#ifdef SIM_INTERFACE_DE10
        sim_cycles_final = sim_get_cycle();
        sim_read_ticks_final = sim_get_total_read_ticks();
        sim_read_count_final = sim_get_total_read_count();
        sim_write_ticks_final = sim_get_total_write_ticks();
        sim_write_count_final = sim_get_total_write_count();
        sim_fb_fill_hits_final = sim_get_fb_fill_hits();
        sim_fb_fill_misses_final = sim_get_fb_fill_misses();
        sim_fb_fill_burst_count_final = sim_get_fb_fill_burst_count();
        sim_fb_fill_burst_beats_final = sim_get_fb_fill_burst_beats();
        sim_fb_fill_stall_cycles_final = sim_get_fb_fill_stall_cycles();
        sim_fb_write_stall_cycles_final = sim_get_fb_write_stall_cycles();
        sim_fb_write_drain_count_final = sim_get_fb_write_drain_count();
        sim_fb_write_full_drain_count_final = sim_get_fb_write_full_drain_count();
        sim_fb_write_partial_drain_count_final = sim_get_fb_write_partial_drain_count();
        sim_fb_write_drain_reason_full_count_final = sim_get_fb_write_drain_reason_full_count();
        sim_fb_write_drain_reason_rotate_count_final = sim_get_fb_write_drain_reason_rotate_count();
        sim_fb_write_drain_reason_flush_count_final = sim_get_fb_write_drain_reason_flush_count();
        sim_fb_write_drain_dirty_word_total_final = sim_get_fb_write_drain_dirty_word_total();
        sim_fb_write_rotate_blocked_cycles_final = sim_get_fb_write_rotate_blocked_cycles();
        sim_fb_write_single_word_drain_count_final = sim_get_fb_write_single_word_drain_count();
        sim_fb_write_single_word_drain_start_at_zero_count_final = sim_get_fb_write_single_word_drain_start_at_zero_count();
        sim_fb_write_single_word_drain_start_at_last_count_final = sim_get_fb_write_single_word_drain_start_at_last_count();
        sim_fb_write_rotate_adjacent_line_count_final = sim_get_fb_write_rotate_adjacent_line_count();
        sim_fb_write_rotate_same_line_gap_count_final = sim_get_fb_write_rotate_same_line_gap_count();
        sim_fb_write_rotate_other_line_count_final = sim_get_fb_write_rotate_other_line_count();
        sim_fb_mem_color_write_cmd_count_final = sim_get_fb_mem_color_write_cmd_count();
        sim_fb_mem_aux_write_cmd_count_final = sim_get_fb_mem_aux_write_cmd_count();
        sim_fb_mem_color_read_cmd_count_final = sim_get_fb_mem_color_read_cmd_count();
        sim_fb_mem_aux_read_cmd_count_final = sim_get_fb_mem_aux_read_cmd_count();
        sim_fb_mem_lfb_read_cmd_count_final = sim_get_fb_mem_lfb_read_cmd_count();
        sim_fb_mem_color_write_blocked_cycles_final = sim_get_fb_mem_color_write_blocked_cycles();
        sim_fb_mem_aux_write_blocked_cycles_final = sim_get_fb_mem_aux_write_blocked_cycles();
        sim_fb_mem_color_read_blocked_cycles_final = sim_get_fb_mem_color_read_blocked_cycles();
        sim_fb_mem_aux_read_blocked_cycles_final = sim_get_fb_mem_aux_read_blocked_cycles();
        sim_fb_mem_lfb_read_blocked_cycles_final = sim_get_fb_mem_lfb_read_blocked_cycles();
        sim_fb_read_req_count_final = sim_get_fb_read_req_count();
        sim_fb_read_req_forward_step_count_final = sim_get_fb_read_req_forward_step_count();
        sim_fb_read_req_backward_step_count_final = sim_get_fb_read_req_backward_step_count();
        sim_fb_read_req_same_word_count_final = sim_get_fb_read_req_same_word_count();
        sim_fb_read_req_same_line_count_final = sim_get_fb_read_req_same_line_count();
        sim_fb_read_req_other_count_final = sim_get_fb_read_req_other_count();
        sim_fb_read_single_beat_burst_count_final = sim_get_fb_read_single_beat_burst_count();
        sim_fb_read_multi_beat_burst_count_final = sim_get_fb_read_multi_beat_burst_count();
        sim_fb_read_max_queue_occupancy_final = sim_get_fb_read_max_queue_occupancy();
        fprintf(stderr,
                "[trace_test] Worst sim_write: entry=%u addr=0x%08x data=0x%08x cycles=%llu\n",
                worst_write_entry,
                worst_write_addr,
                worst_write_data,
                (unsigned long long)worst_write_cycles);
        auto hotspot_cycles = sim_write_cycles_by_reg;
        auto hotspot_counts = sim_write_count_by_reg;
        for (int rank = 0; rank < 8; ++rank) {
            int best_idx = -1;
            for (int idx = 0; idx < (int)hotspot_cycles.size(); ++idx) {
                if (hotspot_counts[idx] == 0) continue;
                if (best_idx < 0 || hotspot_cycles[idx] > hotspot_cycles[best_idx]) {
                    best_idx = idx;
                }
            }
            if (best_idx < 0) break;
            RegHotspot hotspot;
            hotspot.reg = (uint32_t)(best_idx << 2);
            hotspot.cycles = hotspot_cycles[best_idx];
            hotspot.count = hotspot_counts[best_idx];
            sim_write_hotspots.push_back(hotspot);
            fprintf(stderr,
                    "[trace_test] Sim write hotspot[%d]: reg=0x%03x cycles=%llu count=%u avg=%.2f\n",
                    rank,
                    hotspot.reg,
                    (unsigned long long)hotspot.cycles,
                    hotspot.count,
                    (double)hotspot.cycles / (double)hotspot.count);
            hotspot_cycles[best_idx] = 0;
            hotspot_counts[best_idx] = 0;
        }
        sim_pixels_in_final = sim_get_pixels_in();
        sim_pixels_out_final = sim_get_pixels_out();
        fprintf(stderr,
                "[trace_test] Sim pixel stats: pixelsIn=%u pixelsOut=%u\n",
                sim_pixels_in_final, sim_pixels_out_final);
        fprintf(stderr,
                "[trace_test] Sim bus stats: cycles=%llu readTicks=%llu readCount=%llu writeTicks=%llu writeCount=%llu avgRead=%.2f avgWrite=%.2f\n",
                (unsigned long long)sim_cycles_final,
                (unsigned long long)sim_read_ticks_final,
                (unsigned long long)sim_read_count_final,
                (unsigned long long)sim_write_ticks_final,
                (unsigned long long)sim_write_count_final,
                sim_read_count_final ? ((double)sim_read_ticks_final / (double)sim_read_count_final) : 0.0,
                sim_write_count_final ? ((double)sim_write_ticks_final / (double)sim_write_count_final) : 0.0);
        fprintf(stderr,
                "[trace_test] Sim fill stats: hits=%u misses=%u hitRate=%.4f burstCount=%u burstBeats=%u fillStall=%u writeStall=%u\n",
                sim_fb_fill_hits_final,
                sim_fb_fill_misses_final,
                (sim_fb_fill_hits_final + sim_fb_fill_misses_final)
                    ? ((double)sim_fb_fill_hits_final / (double)(sim_fb_fill_hits_final + sim_fb_fill_misses_final))
                    : 0.0,
                sim_fb_fill_burst_count_final,
                sim_fb_fill_burst_beats_final,
                sim_fb_fill_stall_cycles_final,
                sim_fb_write_stall_cycles_final);
        fprintf(stderr,
                "[trace_test] Sim write-buffer stats: drains=%u full=%u partial=%u reason(full=%u rotate=%u flush=%u) avgDirtyWords=%.2f rotateBlocked=%u singleWord=%u singleWord(start0=%u startLast=%u) rotateKinds(adj=%u sameLineGap=%u other=%u)\n",
                sim_fb_write_drain_count_final,
                sim_fb_write_full_drain_count_final,
                sim_fb_write_partial_drain_count_final,
                sim_fb_write_drain_reason_full_count_final,
                sim_fb_write_drain_reason_rotate_count_final,
                sim_fb_write_drain_reason_flush_count_final,
                sim_fb_write_drain_count_final ? ((double)sim_fb_write_drain_dirty_word_total_final / (double)sim_fb_write_drain_count_final) : 0.0,
                sim_fb_write_rotate_blocked_cycles_final,
                sim_fb_write_single_word_drain_count_final,
                sim_fb_write_single_word_drain_start_at_zero_count_final,
                sim_fb_write_single_word_drain_start_at_last_count_final,
                sim_fb_write_rotate_adjacent_line_count_final,
                sim_fb_write_rotate_same_line_gap_count_final,
                sim_fb_write_rotate_other_line_count_final);
        fprintf(stderr,
                "[trace_test] Sim fb-mem contention: cmdCount(cw=%u aw=%u cr=%u ar=%u lfb=%u) blocked(cw=%u aw=%u cr=%u ar=%u lfb=%u)\n",
                sim_fb_mem_color_write_cmd_count_final,
                sim_fb_mem_aux_write_cmd_count_final,
                sim_fb_mem_color_read_cmd_count_final,
                sim_fb_mem_aux_read_cmd_count_final,
                sim_fb_mem_lfb_read_cmd_count_final,
                sim_fb_mem_color_write_blocked_cycles_final,
                sim_fb_mem_aux_write_blocked_cycles_final,
                sim_fb_mem_color_read_blocked_cycles_final,
                sim_fb_mem_aux_read_blocked_cycles_final,
                sim_fb_mem_lfb_read_blocked_cycles_final);
        fprintf(stderr,
                "[trace_test] Sim fb-read pattern: req=%u fwd=%u back=%u sameWord=%u sameLine=%u other=%u bursts(single=%u multi=%u) maxQ=%u\n",
                sim_fb_read_req_count_final,
                sim_fb_read_req_forward_step_count_final,
                sim_fb_read_req_backward_step_count_final,
                sim_fb_read_req_same_word_count_final,
                sim_fb_read_req_same_line_count_final,
                sim_fb_read_req_other_count_final,
                sim_fb_read_single_beat_burst_count_final,
                sim_fb_read_multi_beat_burst_count_final,
                sim_fb_read_max_queue_occupancy_final);
        if (progress_snapshots.empty() || progress_snapshots.back().entry != num_entries) {
            ProgressSnapshot snap;
            snap.entry = num_entries;
            snap.logical = logical_cmds;
            snap.cmd_type = 0xffffffffu;
            snap.addr = 0;
            snap.data = 0;
            snap.count = 0;
            snap.sim_cycle = sim_cycles_final;
            snap.sim_reads = sim_read_count_final;
            snap.sim_writes = sim_write_count_final;
            snap.sim_pixels_in = sim_pixels_in_final;
            snap.sim_pixels_out = sim_pixels_out_final;
            progress_snapshots.push_back(snap);
        }
#endif
    }

    if (!profile_json_path.empty()) {
        size_t slash = profile_json_path.find_last_of('/');
        if (slash != std::string::npos) {
            std::string parent = profile_json_path.substr(0, slash);
            if (!parent.empty() && !ensure_directory(parent.c_str())) {
                fprintf(stderr, "[trace_test] ERROR: Failed to create profile directory %s\n", parent.c_str());
                return 1;
            }
        }

        FILE *profile_fp = fopen(profile_json_path.c_str(), "w");
        if (!profile_fp) {
            fprintf(stderr, "[trace_test] ERROR: Failed to open profile JSON %s\n", profile_json_path.c_str());
            return 1;
        }

        const uint32_t ref_pixels_in = ref_get_fbi_pixels_in();
        const uint32_t ref_pixels_out = ref_get_fbi_pixels_out();
        const uint32_t ref_z_fail = ref_get_fbi_zfunc_fail();
        const uint32_t ref_a_fail = ref_get_fbi_afunc_fail();
        const uint32_t ref_chroma_fail = ref_get_fbi_chroma_fail();
        const uint32_t ref_cov = ref_get_triangle_coverage_pixels();
        const uint32_t ref_cov_total = ref_get_triangle_coverage_total();
        const uint32_t ref_rgb_writes = ref_get_triangle_color_writes();
        const uint32_t ref_black_writes = ref_get_triangle_black_writes();
        const uint32_t ref_depth_only = ref_get_triangle_depth_only_updates();
        const uint32_t ref_tri_tex = ref_get_triangles_textured();
        const uint32_t ref_tri_flat = ref_get_triangles_untextured();
        const uint32_t ref_tri_all_black = ref_get_triangles_all_black_writes();
        const uint32_t ref_pal0_nz = ref_get_palette_nonzero_count(0);
        const uint32_t ref_pal1_nz = ref_get_palette_nonzero_count(1);
        const double fpga_time_seconds = (!ref_only && sim_cycles_final) ? ((double)sim_cycles_final / 50000000.0) : 0.0;
        const double sim_pixels_out_mpps = (fpga_time_seconds > 0.0) ? ((double)sim_pixels_out_final / fpga_time_seconds / 1000000.0) : 0.0;
        const double sim_pixels_in_mpps = (fpga_time_seconds > 0.0) ? ((double)sim_pixels_in_final / fpga_time_seconds / 1000000.0) : 0.0;

        fprintf(profile_fp, "{\n");
        fprintf(profile_fp, "  \"trace\": ");
        json_write_string(profile_fp, trace_file);
        fprintf(profile_fp, ",\n  \"state\": ");
        if (!state_file.empty()) json_write_string(profile_fp, state_file); else fprintf(profile_fp, "null");
        fprintf(profile_fp, ",\n  \"name\": ");
        json_write_string(profile_fp, name);
        fprintf(profile_fp, ",\n  \"sim_interface\": ");
#ifdef SIM_INTERFACE_DE10
        json_write_string(profile_fp, "de10");
#else
        json_write_string(profile_fp, "default");
#endif
        fprintf(profile_fp, ",\n  \"num_entries\": %u,\n", num_entries);
        fprintf(profile_fp, "  \"logical_commands\": %llu,\n", (unsigned long long)logical_cmds);
        fprintf(profile_fp, "  \"triangles\": %d,\n", tri_count);
        fprintf(profile_fp, "  \"tex_writes\": %d,\n", tex_write_count);
        fprintf(profile_fp, "  \"fb_writes\": %d,\n", fb_write_count);
        fprintf(profile_fp, "  \"swap_commands\": %d,\n", swap_cmd_count);
        fprintf(profile_fp, "  \"cmd_histogram\": {\n");
        fprintf(profile_fp, "    \"write_reg\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_REG_L]);
        fprintf(profile_fp, "    \"write_fb_l\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_FB_L]);
        fprintf(profile_fp, "    \"write_fb_w\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_FB_W]);
        fprintf(profile_fp, "    \"write_tex\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_TEX_L]);
        fprintf(profile_fp, "    \"write_cmdfifo\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_WRITE_CMDFIFO]);
        fprintf(profile_fp, "    \"read_reg\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_REG_L]);
        fprintf(profile_fp, "    \"read_fb_l\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_FB_L]);
        fprintf(profile_fp, "    \"read_fb_w\": %llu,\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_READ_FB_W]);
        fprintf(profile_fp, "    \"swap\": %llu\n", (unsigned long long)cmd_type_counts[VOODOO_TRACE_SWAP]);
        fprintf(profile_fp, "  },\n");
        fprintf(profile_fp, "  \"ref_stats\": {\n");
        fprintf(profile_fp, "    \"pixels_in\": %u,\n", ref_pixels_in);
        fprintf(profile_fp, "    \"pixels_out\": %u,\n", ref_pixels_out);
        fprintf(profile_fp, "    \"z_fail\": %u,\n", ref_z_fail);
        fprintf(profile_fp, "    \"a_fail\": %u,\n", ref_a_fail);
        fprintf(profile_fp, "    \"chroma_fail\": %u,\n", ref_chroma_fail);
        fprintf(profile_fp, "    \"triangle_coverage_pixels\": %u,\n", ref_cov);
        fprintf(profile_fp, "    \"triangle_coverage_total\": %u,\n", ref_cov_total);
        fprintf(profile_fp, "    \"rgb_writes\": %u,\n", ref_rgb_writes);
        fprintf(profile_fp, "    \"black_writes\": %u,\n", ref_black_writes);
        fprintf(profile_fp, "    \"depth_only_updates\": %u,\n", ref_depth_only);
        fprintf(profile_fp, "    \"triangles_textured\": %u,\n", ref_tri_tex);
        fprintf(profile_fp, "    \"triangles_untextured\": %u,\n", ref_tri_flat);
        fprintf(profile_fp, "    \"triangles_all_black\": %u,\n", ref_tri_all_black);
        fprintf(profile_fp, "    \"palette_nonzero_tmu0\": %u,\n", ref_pal0_nz);
        fprintf(profile_fp, "    \"palette_nonzero_tmu1\": %u\n", ref_pal1_nz);
        fprintf(profile_fp, "  }");

        if (!ref_only) {
            fprintf(profile_fp, ",\n  \"sim_stats\": {\n");
            fprintf(profile_fp, "    \"cycles\": %llu,\n", (unsigned long long)sim_cycles_final);
            fprintf(profile_fp, "    \"fpga_clock_hz\": 50000000,\n");
            fprintf(profile_fp, "    \"fpga_time_seconds\": %.9f,\n", fpga_time_seconds);
            fprintf(profile_fp, "    \"pixels_in\": %u,\n", sim_pixels_in_final);
            fprintf(profile_fp, "    \"pixels_out\": %u,\n", sim_pixels_out_final);
            fprintf(profile_fp, "    \"pixels_in_mpps\": %.6f,\n", sim_pixels_in_mpps);
            fprintf(profile_fp, "    \"pixels_out_mpps\": %.6f,\n", sim_pixels_out_mpps);
            fprintf(profile_fp, "    \"read_ticks\": %llu,\n", (unsigned long long)sim_read_ticks_final);
            fprintf(profile_fp, "    \"read_count\": %llu,\n", (unsigned long long)sim_read_count_final);
            fprintf(profile_fp, "    \"write_ticks\": %llu,\n", (unsigned long long)sim_write_ticks_final);
            fprintf(profile_fp, "    \"write_count\": %llu,\n", (unsigned long long)sim_write_count_final);
            fprintf(profile_fp, "    \"avg_read_cycles\": %.6f,\n", sim_read_count_final ? ((double)sim_read_ticks_final / (double)sim_read_count_final) : 0.0);
            fprintf(profile_fp, "    \"avg_write_cycles\": %.6f,\n", sim_write_count_final ? ((double)sim_write_ticks_final / (double)sim_write_count_final) : 0.0);
            fprintf(profile_fp, "    \"fb_fill_hits\": %u,\n", sim_fb_fill_hits_final);
            fprintf(profile_fp, "    \"fb_fill_misses\": %u,\n", sim_fb_fill_misses_final);
            fprintf(profile_fp, "    \"fb_fill_hit_rate\": %.6f,\n",
                    (sim_fb_fill_hits_final + sim_fb_fill_misses_final)
                        ? ((double)sim_fb_fill_hits_final / (double)(sim_fb_fill_hits_final + sim_fb_fill_misses_final))
                        : 0.0);
            fprintf(profile_fp, "    \"fb_fill_burst_count\": %u,\n", sim_fb_fill_burst_count_final);
            fprintf(profile_fp, "    \"fb_fill_burst_beats\": %u,\n", sim_fb_fill_burst_beats_final);
            fprintf(profile_fp, "    \"fb_fill_stall_cycles\": %u,\n", sim_fb_fill_stall_cycles_final);
            fprintf(profile_fp, "    \"fb_write_stall_cycles\": %u,\n", sim_fb_write_stall_cycles_final);
            fprintf(profile_fp, "    \"fb_write_drain_count\": %u,\n", sim_fb_write_drain_count_final);
            fprintf(profile_fp, "    \"fb_write_full_drain_count\": %u,\n", sim_fb_write_full_drain_count_final);
            fprintf(profile_fp, "    \"fb_write_partial_drain_count\": %u,\n", sim_fb_write_partial_drain_count_final);
            fprintf(profile_fp, "    \"fb_write_drain_reason_full_count\": %u,\n", sim_fb_write_drain_reason_full_count_final);
            fprintf(profile_fp, "    \"fb_write_drain_reason_rotate_count\": %u,\n", sim_fb_write_drain_reason_rotate_count_final);
            fprintf(profile_fp, "    \"fb_write_drain_reason_flush_count\": %u,\n", sim_fb_write_drain_reason_flush_count_final);
            fprintf(profile_fp, "    \"fb_write_drain_dirty_word_total\": %u,\n", sim_fb_write_drain_dirty_word_total_final);
            fprintf(profile_fp, "    \"fb_write_avg_drain_dirty_words\": %.6f,\n",
                    sim_fb_write_drain_count_final ? ((double)sim_fb_write_drain_dirty_word_total_final / (double)sim_fb_write_drain_count_final) : 0.0);
            fprintf(profile_fp, "    \"fb_write_rotate_blocked_cycles\": %u,\n", sim_fb_write_rotate_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_write_single_word_drain_count\": %u,\n", sim_fb_write_single_word_drain_count_final);
            fprintf(profile_fp, "    \"fb_write_single_word_drain_start_at_zero_count\": %u,\n", sim_fb_write_single_word_drain_start_at_zero_count_final);
            fprintf(profile_fp, "    \"fb_write_single_word_drain_start_at_last_count\": %u,\n", sim_fb_write_single_word_drain_start_at_last_count_final);
            fprintf(profile_fp, "    \"fb_write_rotate_adjacent_line_count\": %u,\n", sim_fb_write_rotate_adjacent_line_count_final);
            fprintf(profile_fp, "    \"fb_write_rotate_same_line_gap_count\": %u,\n", sim_fb_write_rotate_same_line_gap_count_final);
            fprintf(profile_fp, "    \"fb_write_rotate_other_line_count\": %u,\n", sim_fb_write_rotate_other_line_count_final);
            fprintf(profile_fp, "    \"fb_mem_color_write_cmd_count\": %u,\n", sim_fb_mem_color_write_cmd_count_final);
            fprintf(profile_fp, "    \"fb_mem_aux_write_cmd_count\": %u,\n", sim_fb_mem_aux_write_cmd_count_final);
            fprintf(profile_fp, "    \"fb_mem_color_read_cmd_count\": %u,\n", sim_fb_mem_color_read_cmd_count_final);
            fprintf(profile_fp, "    \"fb_mem_aux_read_cmd_count\": %u,\n", sim_fb_mem_aux_read_cmd_count_final);
            fprintf(profile_fp, "    \"fb_mem_lfb_read_cmd_count\": %u,\n", sim_fb_mem_lfb_read_cmd_count_final);
            fprintf(profile_fp, "    \"fb_mem_color_write_blocked_cycles\": %u,\n", sim_fb_mem_color_write_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_mem_aux_write_blocked_cycles\": %u,\n", sim_fb_mem_aux_write_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_mem_color_read_blocked_cycles\": %u,\n", sim_fb_mem_color_read_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_mem_aux_read_blocked_cycles\": %u,\n", sim_fb_mem_aux_read_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_mem_lfb_read_blocked_cycles\": %u,\n", sim_fb_mem_lfb_read_blocked_cycles_final);
            fprintf(profile_fp, "    \"fb_read_req_count\": %u,\n", sim_fb_read_req_count_final);
            fprintf(profile_fp, "    \"fb_read_req_forward_step_count\": %u,\n", sim_fb_read_req_forward_step_count_final);
            fprintf(profile_fp, "    \"fb_read_req_backward_step_count\": %u,\n", sim_fb_read_req_backward_step_count_final);
            fprintf(profile_fp, "    \"fb_read_req_same_word_count\": %u,\n", sim_fb_read_req_same_word_count_final);
            fprintf(profile_fp, "    \"fb_read_req_same_line_count\": %u,\n", sim_fb_read_req_same_line_count_final);
            fprintf(profile_fp, "    \"fb_read_req_other_count\": %u,\n", sim_fb_read_req_other_count_final);
            fprintf(profile_fp, "    \"fb_read_single_beat_burst_count\": %u,\n", sim_fb_read_single_beat_burst_count_final);
            fprintf(profile_fp, "    \"fb_read_multi_beat_burst_count\": %u,\n", sim_fb_read_multi_beat_burst_count_final);
            fprintf(profile_fp, "    \"fb_read_max_queue_occupancy\": %u,\n", sim_fb_read_max_queue_occupancy_final);
            fprintf(profile_fp, "    \"worst_write\": {\n");
            fprintf(profile_fp, "      \"entry\": %u,\n", worst_write_entry);
            fprintf(profile_fp, "      \"addr\": %u,\n", worst_write_addr);
            fprintf(profile_fp, "      \"data\": %u,\n", worst_write_data);
            fprintf(profile_fp, "      \"cycles\": %llu\n", (unsigned long long)worst_write_cycles);
            fprintf(profile_fp, "    },\n");
            fprintf(profile_fp, "    \"write_hotspots\": [\n");
            for (size_t idx = 0; idx < sim_write_hotspots.size(); ++idx) {
                const RegHotspot &hotspot = sim_write_hotspots[idx];
                fprintf(profile_fp,
                        "      {\"reg\": %u, \"cycles\": %llu, \"count\": %u, \"avg_cycles\": %.6f}%s\n",
                        hotspot.reg,
                        (unsigned long long)hotspot.cycles,
                        hotspot.count,
                        hotspot.count ? ((double)hotspot.cycles / (double)hotspot.count) : 0.0,
                        (idx + 1 == sim_write_hotspots.size()) ? "" : ",");
            }
            fprintf(profile_fp, "    ],\n");
            fprintf(profile_fp, "    \"progress\": [\n");
            for (size_t idx = 0; idx < progress_snapshots.size(); ++idx) {
                const ProgressSnapshot &snap = progress_snapshots[idx];
                fprintf(profile_fp,
                        "      {\"entry\": %u, \"logical\": %llu, \"cmd_type\": %u, \"addr\": %u, \"data\": %u, \"count\": %u, \"sim_cycle\": %llu, \"sim_reads\": %llu, \"sim_writes\": %llu, \"sim_pixels_in\": %u, \"sim_pixels_out\": %u}%s\n",
                        snap.entry,
                        (unsigned long long)snap.logical,
                        snap.cmd_type,
                        snap.addr,
                        snap.data,
                        snap.count,
                        (unsigned long long)snap.sim_cycle,
                        (unsigned long long)snap.sim_reads,
                        (unsigned long long)snap.sim_writes,
                        snap.sim_pixels_in,
                        snap.sim_pixels_out,
                        (idx + 1 == progress_snapshots.size()) ? "" : ",");
            }
            fprintf(profile_fp, "    ]\n");
            fprintf(profile_fp, "  }\n");
        } else {
            fprintf(profile_fp, "\n");
        }

        fprintf(profile_fp, "}\n");
        fclose(profile_fp);
        fprintf(stderr, "[trace_test] Wrote %s\n", profile_json_path.c_str());
    }

    if (sim_replay_only) {
        if (!ref_only) sim_shutdown();
        ref_shutdown();
        return 0;
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
    ref_dump_layout_debug();
    fprintf(stderr,
            "[trace_test] State header layout: row_width=%u draw=0x%x aux=0x%x presented=0x%x\n",
            state_row_width, state_draw_offset, state_aux_offset, presented_offset);
    /* Strict frame selection: use the last buffer explicitly presented by
     * swapbufferCMD (sampled pre-swap), no heuristic fallbacks. */
    uint32_t fb_offset = presented_offset;
    /* Sim color plane now matches the 16-bit RGB565 layout: one pixel = 2 bytes. */
    uint32_t sim_fb_offset = presented_offset;

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

    if (row_width == 0) {
        row_width = state_row_width ? state_row_width : (uint32_t)sim_pixel_stride * 2;
        fprintf(stderr,
                "[trace_test] WARNING: Reference row_width is zero; falling back to %u bytes\n",
                row_width);
    }
    row_stride = row_width / 2;              /* pixels (16-bit) */

    fprintf(stderr, "[trace_test] FB layout: front=0x%x draw=0x%x presented=0x%x row_width=%u (%d pixels)\n",
            front_offset, draw_offset, fb_offset, row_width, row_stride);

    std::vector<uint16_t> sim_fb_extracted;
    uint16_t *sim_fb = nullptr;

    if (!ref_only) {
        SimReplayBackend simBackend;
        traceReplayCaptureRgb565(simBackend, sim_fb_offset, (uint32_t)sim_pixel_stride,
                                 (uint32_t)disp_width, (uint32_t)disp_height, &sim_fb_extracted);
        sim_fb = sim_fb_extracted.data();

        uint32_t sim_black = 0;
        for (int y = 0; y < disp_height; y++)
            for (int x = 0; x < disp_width; x++)
                if (sim_fb[y * row_stride + x] == 0)
                    sim_black++;
        fprintf(stderr, "[trace_test] Sim selected buffer blackness: %u/%d at offset 0x%x\n",
                sim_black, disp_width * disp_height, sim_fb_offset);
    }

    /* RGB24 buffers for PNG output */
    std::vector<uint8_t> ref_rgb(disp_width * disp_height * 3);
    std::vector<uint8_t> sim_rgb;
    std::vector<uint8_t> diff_rgb;
    int sim_black_mismatches = 0;
    int parity_mismatches[2] = {0, 0};
    std::vector<int> mismatch_by_column(disp_width, 0);
    std::vector<int> sim_black_mismatch_by_column(disp_width, 0);

    if (!ref_only) {
        sim_rgb.resize(disp_width * disp_height * 3);
        diff_rgb.resize(disp_width * disp_height * 3, 0);
    }

    /* Compare and convert */
    for (int y = 0; y < disp_height; y++) {
        /* Convert reference row */
        traceReplayRgb565ToRgb24(&ref_fb[y * row_stride], &ref_rgb[y * disp_width * 3], disp_width, srgb_lut);

        if (!ref_only) {
            /* Convert sim row */
            traceReplayRgb565ToRgb24(&sim_fb[y * row_stride], &sim_rgb[y * disp_width * 3], disp_width, srgb_lut);

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
                        mismatch_by_column[x]++;
                        parity_mismatches[x & 1]++;
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
                            sim_black_mismatches++;
                            sim_black_mismatch_by_column[x]++;
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
        std::vector<uint16_t> alt_extracted(row_stride * disp_height, 0);
        SimReplayBackend simBackend;
        traceReplayCaptureRgb565(simBackend, alt_sim_fb_offset, (uint32_t)sim_pixel_stride,
                                 (uint32_t)disp_width, (uint32_t)disp_height, &alt_extracted);

        int alt_mismatches = 0;
        uint32_t alt_black = 0;
        for (int y = 0; y < disp_height; y++) {
            for (int x = 0; x < disp_width; x++) {
                uint16_t rp = ref_fb[y * row_stride + x];
                uint16_t sp = alt_extracted[y * row_stride + x];
                if (sp == 0)
                    alt_black++;
                if (rp == sp)
                    continue;
                int rr = (rp >> 11) & 0x1f, rg = (rp >> 5) & 0x3f, rb = rp & 0x1f;
                int sr = (sp >> 11) & 0x1f, sg = (sp >> 5) & 0x3f, sb = sp & 0x1f;
                int dr = abs(rr - sr), dg = abs(rg - sg), db = abs(rb - sb);
                if (dr > color_tolerance || dg > color_tolerance || db > color_tolerance)
                    alt_mismatches++;
            }
        }

        fprintf(stderr, "[trace_test] Alt buffer check: reading=0x%x mismatches=%d black=%u/%d\n",
                alt_sim_fb_offset, alt_mismatches, alt_black, disp_width * disp_height);
        if (alt_mismatches + 64 < mismatches && (mismatches - alt_mismatches) > 256) {
            stale_frame_risk = true;
            fprintf(stderr, "[trace_test] ERROR: alternate framebuffer matches better than selected framebuffer\n");
        }
    }

    /* Write PNG output */
    if (output_dir) {
        char path[512];

        if (!ensure_directory(output_dir)) {
            fprintf(stderr, "[trace_test] ERROR: Failed to create output directory %s\n", output_dir);
            return 1;
        }

        snprintf(path, sizeof(path), "%s/%s_ref.png", output_dir, name.c_str());
        if (!stbi_write_png(path, disp_width, disp_height, 3, ref_rgb.data(), disp_width * 3)) {
            fprintf(stderr, "[trace_test] ERROR: Failed to write %s\n", path);
            return 1;
        }
        fprintf(stderr, "[trace_test] Wrote %s\n", path);

        if (!ref_only) {
            snprintf(path, sizeof(path), "%s/%s_sim.png", output_dir, name.c_str());
            if (!stbi_write_png(path, disp_width, disp_height, 3, sim_rgb.data(), disp_width * 3)) {
                fprintf(stderr, "[trace_test] ERROR: Failed to write %s\n", path);
                return 1;
            }
            fprintf(stderr, "[trace_test] Wrote %s\n", path);

            snprintf(path, sizeof(path), "%s/%s_diff.png", output_dir, name.c_str());
            if (!stbi_write_png(path, disp_width, disp_height, 3, diff_rgb.data(), disp_width * 3)) {
                fprintf(stderr, "[trace_test] ERROR: Failed to write %s\n", path);
                return 1;
            }
            fprintf(stderr, "[trace_test] Wrote %s\n", path);
        }
    }

    /* Summary */
    if (ref_only) {
        fprintf(stderr, "[trace_test] Reference-only mode: %d pixels rendered\n", total_pixels);
    } else {
        int max_column_mismatches = 0;
        int max_column_x = 0;
        int max_black_column_mismatches = 0;
        int max_black_column_x = 0;
        for (int x = 0; x < disp_width; x++) {
            if (mismatch_by_column[x] > max_column_mismatches) {
                max_column_mismatches = mismatch_by_column[x];
                max_column_x = x;
            }
            if (sim_black_mismatch_by_column[x] > max_black_column_mismatches) {
                max_black_column_mismatches = sim_black_mismatch_by_column[x];
                max_black_column_x = x;
            }
        }
        int parity_mismatch_skew = abs(parity_mismatches[0] - parity_mismatches[1]);
        fprintf(stderr, "[trace_test] %d/%d pixels mismatched", mismatches, total_pixels);
        if (color_tolerance > 0)
            fprintf(stderr, " (tolerance=%d)", color_tolerance);
        fprintf(stderr, "\n");
        fprintf(stderr,
                "[trace_test] Artifact stats: simBlackMismatches=%d parity(even=%d odd=%d skew=%d) maxMismatchColumn=x%d count=%d maxBlackColumn=x%d count=%d\n",
                sim_black_mismatches,
                parity_mismatches[0],
                parity_mismatches[1],
                parity_mismatch_skew,
                max_column_x,
                max_column_mismatches,
                max_black_column_x,
                max_black_column_mismatches);
        if (max_sim_black_mismatches >= 0 && sim_black_mismatches > max_sim_black_mismatches) {
            fprintf(stderr,
                    "[trace_test] ERROR: simBlackMismatches=%d exceeded threshold %d\n",
                    sim_black_mismatches,
                    max_sim_black_mismatches);
        }
        if (max_parity_mismatch_skew >= 0 && parity_mismatch_skew > max_parity_mismatch_skew) {
            fprintf(stderr,
                    "[trace_test] ERROR: parity mismatch skew=%d exceeded threshold %d\n",
                    parity_mismatch_skew,
                    max_parity_mismatch_skew);
        }
        if (stale_frame_risk) {
            fprintf(stderr, "[trace_test] ERROR: selected framebuffer appears worse than alternate framebuffer\n");
        }
    }

    /* Cleanup */
    free(loaded_trace.traceData);
    ref_trace_close();
    ref_shutdown();
    if (!ref_only) sim_shutdown();

    /* Use _exit to avoid hanging in Verilator's static destructors */
    int parity_mismatch_skew = abs(parity_mismatches[0] - parity_mismatches[1]);
    int ok = (ref_only || mismatches <= max_mismatches) ? 1 : 0;
    if (!ref_only && max_sim_black_mismatches >= 0 && sim_black_mismatches > max_sim_black_mismatches)
        ok = 0;
    if (!ref_only && max_parity_mismatch_skew >= 0 && parity_mismatch_skew > max_parity_mismatch_skew)
        ok = 0;
    if (!ref_only && stale_frame_risk)
        ok = 0;
    _exit(ok ? 0 : 1);
}
