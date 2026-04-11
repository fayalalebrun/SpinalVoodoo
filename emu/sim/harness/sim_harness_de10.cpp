/*
 * sim_harness_de10.cpp -- Verilator driver for CoreDe10.
 *
 * This exercises the DE10-facing lightweight MMIO wrapper and the external
 * Avalon-MM memory ports instead of talking directly to Core's BMB bus.
 */

#include "sim_harness.h"
#include "VCoreDe10.h"
#include "VCoreDe10___024root.h"
#include <array>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "verilated.h"
#ifdef VM_TRACE_FST
#include "verilated_fst_c.h"
#endif

#ifdef VL_USER_WARN
void vl_warn(const char* filename, int linenum, const char* hier,
             const char* msg) {
    if (strstr(msg, "$readmem")) {
        vl_fatal(filename, linenum, hier, msg);
    } else {
        if (filename && filename[0])
            VL_PRINTF("%%Warning: %s:%d: %s\n", filename, linenum, msg);
        else
            VL_PRINTF("%%Warning: %s\n", msg);
    }
}
#endif

static VCoreDe10* top = nullptr;
static VerilatedContext* contextp = nullptr;
#ifdef VM_TRACE_FST
static VerilatedFstC* tfp = nullptr;
#endif
static uint64_t sim_time = 0;
static uint64_t cycle_limit = 0;
static bool cycle_limit_hit = false;
static uint32_t mmio_timeout_cycles = 400000;
static bool mmio_timeout_hit = false;
static volatile sig_atomic_t quit_requested = 0;

static constexpr uint32_t FB_WORD_COUNT = (4u * 1024u * 1024u) / 4u;

static FILE *tex_access_file = nullptr;
static bool prev_tex_read_fire = false;
static uint64_t tex_access_dump_writes = 0;
static uint64_t compare_pop_count = 0;
static uint64_t compare_pop_all_hits_count = 0;
static uint64_t compare_pop_any_miss_count = 0;
static uint64_t compare_pop_issue_needed_sum = 0;
static uint64_t compare_pop_tap_hit_sum = 0;
static uint64_t compare_pop_nonfast_zero_issue = 0;

struct TexSampleDumpHeader {
    char magic[4];
    uint32_t version;
};

struct TexSampleDumpRecord {
    uint64_t cycle;
    uint32_t flags;
    uint32_t addrs[5];
    uint32_t banks;
    uint32_t reserved;
};

static void dump_tex_sample(const TexSampleDumpRecord& rec) {
    if (!tex_access_file) return;
    fwrite(&rec, sizeof(rec), 1, tex_access_file);
    tex_access_dump_writes++;
}

static inline void dump_texture_cache_lookup(VCoreDe10___024root* root) {
    (void)root;
}

static void open_tex_access_dump(const char *path) {
    if (!path || !path[0]) return;
    tex_access_file = fopen(path, "wb");
    if (!tex_access_file) fprintf(stderr, "[sim_harness] could not open tex_access_dump %s\n", path);
    if (tex_access_file) {
        TexSampleDumpHeader hdr = {{'T', 'X', 'S', '1'}, 1};
        fwrite(&hdr, sizeof(hdr), 1, tex_access_file);
    }
}

static void close_tex_access_dump(void) {
    if (tex_access_file) { fclose(tex_access_file); tex_access_file = nullptr; }
}

static constexpr uint32_t TEX_WORD_COUNT = (8u * 1024u * 1024u) / 4u;
static constexpr uint32_t TEX_BASE_BYTES = 4u * 1024u * 1024u;

static std::vector<uint32_t> fb_mem;
static std::vector<uint32_t> tex_mem;

struct AvalonMemState {
    uint8_t read_delay;
    uint8_t read_valid;
    uint32_t read_data;
    uint32_t next_addr;
    uint16_t beats_left;
};

struct AvalonMemCmdSample {
    uint8_t read;
    uint8_t write;
    uint32_t address;
    uint32_t write_data;
    uint8_t byte_enable;
    uint16_t burst_count;
};

static AvalonMemState fb_write_state = {0, 0, 0, 0, 0};
static AvalonMemState fb_color_write_state = {0, 0, 0, 0, 0};
static AvalonMemState fb_aux_write_state = {0, 0, 0, 0, 0};
static AvalonMemState fb_color_state = {0, 0, 0, 0, 0};
static AvalonMemState fb_aux_state = {0, 0, 0, 0, 0};
static AvalonMemState tex_state = {0, 0, 0, 0, 0};
static AvalonMemCmdSample fb_write_cmd = {0, 0, 0, 0, 0, 0};
static AvalonMemCmdSample fb_color_write_cmd = {0, 0, 0, 0, 0, 0};
static AvalonMemCmdSample fb_aux_write_cmd = {0, 0, 0, 0, 0, 0};
static AvalonMemCmdSample fb_color_cmd = {0, 0, 0, 0, 0, 0};
static AvalonMemCmdSample fb_aux_cmd = {0, 0, 0, 0, 0, 0};
static AvalonMemCmdSample tex_cmd = {0, 0, 0, 0, 0, 0};

static uint64_t total_read_ticks = 0;
static uint64_t total_read_count = 0;
static uint64_t total_write_ticks = 0;
static uint64_t total_write_count = 0;
static uint64_t snoop_color_write_count = 0;
static uint64_t snoop_aux_write_count = 0;
static uint64_t snoop_color_write_buf0 = 0;
static uint64_t snoop_color_write_buf1 = 0;
static uint64_t tex_model_read_launch_count = 0;
static uint64_t tex_model_read_reject_count = 0;
static uint64_t tex_model_rvalid_count = 0;
static uint32_t tex_model_last_launch_burst = 0;
static uint64_t tex_bridge_launch_count = 0;
static uint32_t prev_tex_outstanding = 0;
static uint64_t last_tex_read_cycle = 0;
static uint32_t last_tex_read_addr = 0;
static uint64_t last_tex_rvalid_cycle = 0;
static uint32_t last_tex_rvalid_data = 0;
static int mmio_debug = 0;
static int stall_debug = 0;
static int tex_trace = 0;
static int fb_trace = 0;
static uint32_t mem_read_delay = 0;
static uint32_t tex_trace_count = 0;
static uint32_t fb_trace_count = 0;

static inline uint32_t effective_mem_read_delay(void) {
    return mem_read_delay + 1;
}
static uint64_t tex_trace_after_cycle = 0;
static uint32_t tex_trace_min_addr = 0;
static uint64_t fb_trace_after_cycle = 0;
static uint32_t fb_trace_min_addr = 0;

struct ProgressSnapshot {
    uint32_t pixels_in;
    uint32_t pixels_out;
    uint64_t color_writes;
    uint64_t aux_writes;
    uint64_t tex_launches;
    uint64_t tex_rvalids;
    uint64_t tex_read_cycle;
    uint64_t tex_rvalid_cycle;
};

static inline ProgressSnapshot capture_progress_snapshot(void) {
    auto* root = top->rootp;
    return {
        (uint32_t)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__io_statistics_pixelsIn,
        (uint32_t)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__io_statistics_pixelsOut,
        snoop_color_write_count,
        snoop_aux_write_count,
        tex_model_read_launch_count,
        tex_model_rvalid_count,
        last_tex_read_cycle,
        last_tex_rvalid_cycle,
    };
}

static inline bool progress_changed(const ProgressSnapshot& a, const ProgressSnapshot& b) {
    return a.pixels_in != b.pixels_in ||
           a.pixels_out != b.pixels_out ||
           a.color_writes != b.color_writes ||
           a.aux_writes != b.aux_writes ||
           a.tex_launches != b.tex_launches ||
           a.tex_rvalids != b.tex_rvalids ||
           a.tex_read_cycle != b.tex_read_cycle ||
           a.tex_rvalid_cycle != b.tex_rvalid_cycle;
}

static void log_stalled_bus_state(const char* kind, const char* phase, uint32_t addr, uint64_t iter);

static inline bool tex_trace_enabled_now(uint32_t addr) {
    return tex_trace && ((sim_time / 2) >= tex_trace_after_cycle) && (addr >= tex_trace_min_addr);
}

static inline bool fb_trace_enabled_now(uint32_t addr) {
    return fb_trace && ((sim_time / 2) >= fb_trace_after_cycle) && (addr >= fb_trace_min_addr);
}

static void dump_timeout_debug(const char* kind, uint32_t addr) {
    auto* r = top->rootp;
    fprintf(stderr,
            "[sim_harness_de10] %s addr=0x%06x pipeBusy=%u tri=%u rast=%u tmuIn=%u tmuBusy=%u fbAcc=%u fbInReady=%u fbOutValid=%u fbFetchedOcc=%u fbColorRspOcc=%u fbAuxRspOcc=%u wColorV=%u wColorR=%u wAuxV=%u wAuxR=%u fbColorBusy=%u fbAuxBusy=%u texRead=%u texRvalid=%u texOut=%u texPending=%u texCanLaunch=%u colorRead=%u auxRead=%u fbWrite=%u fbWriteWaitN=%u colorWaitN=%u auxWaitN=%u\n",
            kind,
            addr,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_pipelineBusy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_triangleSetupValid,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_rasterizerRunning,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_tmuInputValid,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_tmuBusy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_fbAccessBusy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess_io_input_ready,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess_io_output_valid,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__fetchedRaw_fifo_io_occupancy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__io_fbReadColorRsp_fifo_io_occupancy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__io_fbReadAuxRsp_fifo_io_occupancy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_writeColorInputValid,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_writeColorReady,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_writeAuxInputValid,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__pixelPipeline_1_io_debug_busy_writeAuxReady,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_colorBusy,
            (unsigned)r->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_auxBusy,
            (unsigned)top->io_memTex_read,
            (unsigned)top->io_memTex_readDataValid,
            (unsigned)r->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats,
            (unsigned)r->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__queuedReadCmdValid,
            (unsigned)r->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__canLaunchRead,
            (unsigned)top->io_memFbColorRead_read,
            (unsigned)top->io_memFbAuxRead_read,
            (unsigned)top->io_memFbWrite_write,
            (unsigned)top->io_memFbColorWrite_write,
            (unsigned)top->io_memFbAuxWrite_write,
            (unsigned)top->io_memFbWrite_waitRequestn,
            (unsigned)top->io_memFbColorWrite_waitRequestn,
            (unsigned)top->io_memFbAuxWrite_waitRequestn,
            (unsigned)top->io_memFbColorRead_waitRequestn,
            (unsigned)top->io_memFbAuxRead_waitRequestn);
}

static void signal_handler(int sig) {
    quit_requested = sig;
}

static inline uint32_t apply_byteenable(uint32_t old_value, uint32_t new_value,
                                        uint32_t byteenable) {
    uint32_t merged = old_value;
    for (int byte = 0; byte < 4; byte++) {
        if ((byteenable >> byte) & 1u) {
            const uint32_t mask = 0xFFu << (byte * 8);
            merged = (merged & ~mask) | (new_value & mask);
        }
    }
    return merged;
}

static inline uint32_t fb_index_from_addr(uint32_t addr) {
    return (addr >> 2) & (FB_WORD_COUNT - 1u);
}

static inline uint32_t tex_index_from_addr(uint32_t addr) {
    return ((addr - TEX_BASE_BYTES) >> 2) & (TEX_WORD_COUNT - 1u);
}

static inline uint32_t mem_word_from_addr(const std::vector<uint32_t>& mem, uint32_t addr,
                                          bool texture) {
    if (texture) {
        if (addr < TEX_BASE_BYTES) return 0;
        return mem[tex_index_from_addr(addr)];
    }
    return mem[fb_index_from_addr(addr)];
}

static inline void service_read_state(AvalonMemState& state, const std::vector<uint32_t>& mem,
                                      bool texture) {
    if (state.read_valid) {
        if (state.beats_left != 0) {
            state.read_data = mem_word_from_addr(mem, state.next_addr, texture);
            state.next_addr += 4;
            state.beats_left--;
            state.read_valid = 1;
        } else {
            state.read_valid = 0;
        }
    } else if (state.read_delay) {
        state.read_delay--;
        if (state.read_delay != 0) {
            return;
        }
        state.read_data = mem_word_from_addr(mem, state.next_addr, texture);
        state.next_addr += 4;
        state.beats_left--;
        state.read_valid = 1;
    }
}

static inline bool start_read_burst(AvalonMemState& state, uint32_t addr, uint32_t burst_count) {
    if (state.read_valid || state.read_delay || state.beats_left) {
        return false;
    }
    state.next_addr = addr;
    state.beats_left = burst_count ? burst_count : 1;
    state.read_valid = 0;
    state.read_delay = effective_mem_read_delay();
    return true;
}

static inline void clear_mem_cmd_samples(void) {
    fb_write_cmd = {0, 0, 0, 0, 0, 0};
    fb_color_write_cmd = {0, 0, 0, 0, 0, 0};
    fb_aux_write_cmd = {0, 0, 0, 0, 0, 0};
    fb_color_cmd = {0, 0, 0, 0, 0, 0};
    fb_aux_cmd = {0, 0, 0, 0, 0, 0};
    tex_cmd = {0, 0, 0, 0, 0, 0};
    prev_tex_read_fire = top ? (top->io_memTex_read && top->io_memTex_waitRequestn) : false;
}

static inline void latch_mem_cmd_sample(AvalonMemCmdSample& dst, uint8_t read, uint8_t write,
                                        uint32_t address, uint32_t write_data, uint8_t byte_enable,
                                        uint16_t burst_count) {
    dst.read |= read;
    dst.write |= write;
    if (read || write) {
        dst.address = address;
        dst.write_data = write_data;
        dst.byte_enable = byte_enable;
        dst.burst_count = burst_count;
    }
}

static inline void observe_memory_outputs(void) {
    auto *root = top->rootp;
    const bool tex_read_fire = top->io_memTex_read && top->io_memTex_waitRequestn;
    if (tex_read_fire && !prev_tex_read_fire) {
        tex_bridge_launch_count++;
        if (tex_trace && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX bridge launch cycle=%lu addr=0x%08x burst=%u out=%u memRead=%u memBurst=%u\n",
                    (unsigned long)(sim_time / 2),
                    (unsigned)top->io_memTex_address,
                    (unsigned)top->io_memTex_burstCount,
                    (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats,
                    (unsigned)top->io_memTex_read,
                    (unsigned)top->io_memTex_burstCount);
            tex_trace_count++;
        }
    }
    latch_mem_cmd_sample(fb_write_cmd, top->io_memFbWrite_read, top->io_memFbWrite_write,
                         top->io_memFbWrite_address, top->io_memFbWrite_writeData,
                         top->io_memFbWrite_byteEnable, top->io_memFbWrite_burstCount);
    latch_mem_cmd_sample(fb_color_write_cmd, top->io_memFbColorWrite_read, top->io_memFbColorWrite_write,
                         top->io_memFbColorWrite_address, top->io_memFbColorWrite_writeData,
                         top->io_memFbColorWrite_byteEnable, top->io_memFbColorWrite_burstCount);
    latch_mem_cmd_sample(fb_aux_write_cmd, top->io_memFbAuxWrite_read, top->io_memFbAuxWrite_write,
                         top->io_memFbAuxWrite_address, top->io_memFbAuxWrite_writeData,
                         top->io_memFbAuxWrite_byteEnable, top->io_memFbAuxWrite_burstCount);
    latch_mem_cmd_sample(fb_color_cmd, top->io_memFbColorRead_read, top->io_memFbColorRead_write,
                         top->io_memFbColorRead_address, top->io_memFbColorRead_writeData,
                         top->io_memFbColorRead_byteEnable, top->io_memFbColorRead_burstCount);
    latch_mem_cmd_sample(fb_aux_cmd, top->io_memFbAuxRead_read, top->io_memFbAuxRead_write,
                         top->io_memFbAuxRead_address, top->io_memFbAuxRead_writeData,
                         top->io_memFbAuxRead_byteEnable, top->io_memFbAuxRead_burstCount);
    latch_mem_cmd_sample(tex_cmd,
                         tex_read_fire,
                         top->io_memTex_write,
                         top->io_memTex_address,
                         top->io_memTex_writeData,
                         top->io_memTex_byteEnable,
                         top->io_memTex_burstCount);
    const uint32_t tex_outstanding = root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats;
    if (tex_trace && tex_outstanding != prev_tex_outstanding && tex_trace_count < 256) {
        fprintf(stderr,
                "[sim_harness_de10] TEX outstanding cycle=%lu old=%u new=%u memRead=%u rvalid=%u queued=%u cmdBeat=%u burstLeft=%u launchQ=%u canLaunch=%u\n",
                (unsigned long)(sim_time / 2),
                (unsigned)prev_tex_outstanding,
                (unsigned)tex_outstanding,
                (unsigned)top->io_memTex_read,
                (unsigned)top->io_memTex_readDataValid,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__queuedReadCmdValid,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__cmdPayload_beatCount,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readRspBurstBeatsLeft,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__launchQueuedRead,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__canLaunchRead);
        tex_trace_count++;
    }
    prev_tex_outstanding = tex_outstanding;
}

static void drive_memory_inputs(void) {
    top->io_memFbWrite_waitRequestn = 1;
    top->io_memFbWrite_readDataValid = fb_write_state.read_valid;
    top->io_memFbWrite_readData = fb_write_state.read_data;

    top->io_memFbColorWrite_waitRequestn = 1;
    top->io_memFbColorWrite_readDataValid = fb_color_write_state.read_valid;
    top->io_memFbColorWrite_readData = fb_color_write_state.read_data;

    top->io_memFbAuxWrite_waitRequestn = 1;
    top->io_memFbAuxWrite_readDataValid = fb_aux_write_state.read_valid;
    top->io_memFbAuxWrite_readData = fb_aux_write_state.read_data;

    top->io_memFbColorRead_waitRequestn = 1;
    top->io_memFbColorRead_readDataValid = fb_color_state.read_valid;
    top->io_memFbColorRead_readData = fb_color_state.read_data;

    top->io_memFbAuxRead_waitRequestn = 1;
    top->io_memFbAuxRead_readDataValid = fb_aux_state.read_valid;
    top->io_memFbAuxRead_readData = fb_aux_state.read_data;

    top->io_memTex_waitRequestn = 1;
    top->io_memTex_readDataValid = tex_state.read_valid;
    top->io_memTex_readData = tex_state.read_data;
    if (tex_state.read_valid) tex_model_rvalid_count++;
    if (tex_trace && tex_state.read_valid && tex_trace_count < 512) {
        fprintf(stderr,
                "[sim_harness_de10] TEX rvalid data=0x%08x cycle=%lu\n",
                (unsigned)tex_state.read_data,
                (unsigned long)(sim_time / 2));
        tex_trace_count++;
    }
}

static void capture_memory_outputs(void) {
    auto* root = top->rootp;

    dump_texture_cache_lookup(root);

    service_read_state(fb_write_state, fb_mem, false);
    service_read_state(fb_color_write_state, fb_mem, false);
    service_read_state(fb_aux_write_state, fb_mem, false);
    service_read_state(fb_color_state, fb_mem, false);
    service_read_state(fb_aux_state, fb_mem, false);

    if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor_o_fbWrite_valid &&
        root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor__DOT__o_fbWrite_ready) {
        snoop_color_write_count++;
        const uint32_t addr =
            root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor_o_fbWrite_payload_address;
        if (addr < 0x96000) snoop_color_write_buf0++;
        else if (addr < 0x12c000) snoop_color_write_buf1++;
    }
    if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeAux_o_fbWrite_valid &&
        root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeAux__DOT__o_fbWrite_ready) {
        snoop_aux_write_count++;
    }

    if (fb_write_cmd.write) {
        const uint32_t idx = fb_index_from_addr(fb_write_cmd.address);
        fb_mem[idx] = apply_byteenable(fb_mem[idx], fb_write_cmd.write_data,
                                       fb_write_cmd.byte_enable);
    }
    if (fb_color_write_cmd.write) {
        const uint32_t idx = fb_index_from_addr(fb_color_write_cmd.address);
        fb_mem[idx] = apply_byteenable(fb_mem[idx], fb_color_write_cmd.write_data,
                                       fb_color_write_cmd.byte_enable);
    }
    if (fb_aux_write_cmd.write) {
        const uint32_t idx = fb_index_from_addr(fb_aux_write_cmd.address);
        fb_mem[idx] = apply_byteenable(fb_mem[idx], fb_aux_write_cmd.write_data,
                                       fb_aux_write_cmd.byte_enable);
    }
    if (fb_write_cmd.read) {
        start_read_burst(fb_write_state, fb_write_cmd.address, fb_write_cmd.burst_count);
    }
    if (fb_color_write_cmd.read) {
        start_read_burst(fb_color_write_state, fb_color_write_cmd.address, fb_color_write_cmd.burst_count);
    }
    if (fb_aux_write_cmd.read) {
        start_read_burst(fb_aux_write_state, fb_aux_write_cmd.address, fb_aux_write_cmd.burst_count);
    }
    if (fb_color_cmd.read) {
        start_read_burst(fb_color_state, fb_color_cmd.address, fb_color_cmd.burst_count);
    }
    if (fb_aux_cmd.read) {
        start_read_burst(fb_aux_state, fb_aux_cmd.address, fb_aux_cmd.burst_count);
    }

    service_read_state(tex_state, tex_mem, true);
    if (tex_cmd.write) {
        const uint32_t addr = tex_cmd.address;
        if (tex_trace_enabled_now(addr) && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX write addr=0x%08x data=0x%08x be=0x%x cycle=%lu\n",
                    addr, (unsigned)tex_cmd.write_data,
                    (unsigned)tex_cmd.byte_enable,
                    (unsigned long)(sim_time / 2));
            tex_trace_count++;
        }
        if (addr >= TEX_BASE_BYTES) {
            const uint32_t idx = tex_index_from_addr(addr);
            tex_mem[idx] = apply_byteenable(tex_mem[idx], tex_cmd.write_data,
                                            tex_cmd.byte_enable);
        }
    }
    if (tex_cmd.read) {
        const uint32_t addr = tex_cmd.address;
        if (tex_trace_enabled_now(addr) && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX read addr=0x%08x cycle=%lu burst=%u cmdBeat=%u outBeats=%u\n",
                    addr, (unsigned long)(sim_time / 2), (unsigned)tex_cmd.burst_count,
                    (unsigned)top->rootp->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__cmdBeatCount,
                    (unsigned)top->rootp->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats);
            tex_trace_count++;
        }
        last_tex_read_cycle = sim_time / 2;
        last_tex_read_addr = addr;
        if (start_read_burst(tex_state, addr, tex_cmd.burst_count)) {
            tex_model_read_launch_count++;
            tex_model_last_launch_burst = tex_cmd.burst_count;
        } else tex_model_read_reject_count++;
        if (addr >= TEX_BASE_BYTES) {
            last_tex_rvalid_data = tex_mem[tex_index_from_addr(addr)];
        } else {
            last_tex_rvalid_data = 0;
        }
        last_tex_rvalid_cycle = (sim_time / 2) + effective_mem_read_delay();
    }
}

static void eval_inputs(void) {
    drive_memory_inputs();
    top->eval();
}

static void tick_one(void) {
    drive_memory_inputs();
    top->eval();
    observe_memory_outputs();

    top->clk = 1;
    top->eval();
#ifdef VM_TRACE_FST
    if (tfp) tfp->dump(sim_time);
#endif
    sim_time++;

    top->clk = 0;
    top->eval();
#ifdef VM_TRACE_FST
    if (tfp) tfp->dump(sim_time);
    if (tfp && (sim_time % 500000) == 0) tfp->flush();
#endif
    sim_time++;

    capture_memory_outputs();
    clear_mem_cmd_samples();

    if (!cycle_limit_hit && cycle_limit && (sim_time / 2) >= cycle_limit) {
        fprintf(stderr, "[sim_harness_de10] Cycle limit reached (%lu ticks), shutting down.\n",
                (unsigned long)cycle_limit);
        cycle_limit_hit = true;
    }

    if (quit_requested) {
        fprintf(stderr, "[sim_harness_de10] Signal %d, shutting down (sim_time=%lu)...\n",
                (int)quit_requested, (unsigned long)sim_time);
        sim_shutdown();
        _exit(128 + quit_requested);
    }
}

static void mmio_begin(uint32_t addr, bool is_write, uint32_t data, uint8_t byteenable = 0xFu) {
    top->io_h2fLw_address = (addr >> 2) & 0x3FFFFFu;
    top->io_h2fLw_byteenable = byteenable;
    top->io_h2fLw_writedata = data;
    top->io_h2fLw_write = is_write ? 1 : 0;
    top->io_h2fLw_read = is_write ? 0 : 1;
}

static void mmio_end(void) {
    top->io_h2fLw_write = 0;
    top->io_h2fLw_read = 0;
    top->io_h2fLw_writedata = 0;
    top->io_h2fLw_byteenable = 0;
}

static bool wait_h2f_idle(uint32_t addr, const char* kind) {
    auto* root = top->rootp;
    int timeout = 1000000;
    uint64_t iter = 0;
    while (timeout > 0) {
        eval_inputs();
        if (!root->CoreDe10__DOT__h2fBridge__DOT__cmdInFlight &&
            !root->CoreDe10__DOT__h2fBridge__DOT__readRspPending) {
            return true;
        }
        if (stall_debug && iter > 0 && (iter % 100000) == 0) {
            log_stalled_bus_state(kind, "preidle", addr, iter);
        }
        tick_one();
        if (cycle_limit_hit) break;
        timeout--;
        iter++;
    }
    if (!cycle_limit_hit) {
        fprintf(stderr,
                "[sim_harness_de10] ERROR: %s(0x%06x) pre-idle timeout after 1M cycles @ cycle %lu\n",
                kind, addr, (unsigned long)(sim_time / 2));
    }
    return false;
}

static void maybe_log_mmio(const char* kind, const char* phase, uint32_t addr,
                           uint32_t value, uint64_t iter) {
    if (!mmio_debug) return;
    fprintf(stderr,
            "[sim_harness_de10] %s %s addr=0x%06x value=0x%08x cycle=%lu iter=%lu wait=%u rvalid=%u\n",
            kind, phase, addr, value, (unsigned long)(sim_time / 2),
            (unsigned long)iter, (unsigned)top->io_h2fLw_waitrequest,
            (unsigned)top->io_h2fLw_readdatavalid);
}

static void log_stalled_bus_state(const char* kind, const char* phase,
                                  uint32_t addr, uint64_t iter) {
    auto* root = top->rootp;
    fprintf(stderr,
            "[sim_harness_de10] %s %s stall addr=0x%06x cycle=%lu iter=%lu h2f(wait=%u rvalid=%u rdata=0x%08x cmdInFlight=%u readRspPending=%u cpuCmdReady=%u pipeBusy=%u) pci(canDrain=%u cmdBlocked=%u triV=%u triR=%u ftriV=%u ftriR=%u) pipe(ts=%u/%u rsRun=%u rs=%u/%u tmuI=%u/%u tmuO=%u/%u cc=%u/%u fogIn=%u fbIn=%u wcIn=%u tmuState=%u/%u/%u/%u/%u tc=%u/%u or=%u/%u exp=%u/%u tcTex=%u/%u/%u texB=%u/%u/%u/%u/%u texM=%u/%u/%u/0x%08x) mem(fbW=%u/%u 0x%08x bridge=%u/%u/%u/%u/%u/0x%08x fbCR=%u/%u 0x%08x fbAR=%u/%u 0x%08x tex=%u/%u 0x%08x) fb(accessBusy=%u inFlight=%u colorBusy=%u auxBusy=%u cbuf=%u/%u/%u/%u/%u/%u abuf=%u/%u/%u/%u/%u/%u) tex(lastRead=0x%08x@%lu lastRvalid=0x%08x@%lu cpuBus=%u/%u rsp=%u/%u)\n",
            kind, phase, addr, (unsigned long)(sim_time / 2),
            (unsigned long)iter,
            (unsigned)top->io_h2fLw_waitrequest,
            (unsigned)top->io_h2fLw_readdatavalid,
            (unsigned)top->io_h2fLw_readdata,
            (unsigned)root->CoreDe10__DOT__h2fBridge__DOT__cmdInFlight,
            (unsigned)root->CoreDe10__DOT__h2fBridge__DOT__readRspPending,
            (unsigned)root->CoreDe10__DOT__core_1_io_cpuBus_cmd_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__pipelineBusySignal,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pciFifo_1__DOT__drainControl_canDrain,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pciFifo_1__DOT__drainControl_commandStreamBlocked,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__commands_triangleCmdStream_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__commands_triangleCmdStream_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__commands_ftriangleCmdStream_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__regBank__DOT__commands_ftriangleCmdStream_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__triangleSetup_1__DOT__i_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__triangleSetup_1__DOT__i_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__rasterizer_1__DOT__running,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__rasterizer_1__DOT__o_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__rasterizer_1__DOT__o_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_input_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_input_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_output_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_output_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__colorCombine_1__DOT__io_input_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__colorCombine_1__DOT__io_input_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fog_1__DOT__io_input_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__io_input_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor__DOT__i_fromPipeline_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__inFlightCount,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__bypassSafe,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__canAllocate,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__bypassOutput_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__sampleRequest_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__io_sampleRequest_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__io_sampleRequest_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath_io_texRead_cmd_valid,
            0u,
            0u,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath_io_texRead_rsp_ready,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readRspBurstActive,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readRspBurstBeatsLeft,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readRspFifo_io_pop_valid,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_bmb_rsp_ready,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_mem_readDataValid,
            (unsigned)tex_state.read_valid,
            (unsigned)tex_state.read_delay,
            (unsigned)tex_state.beats_left,
            (unsigned)tex_state.next_addr,
            (unsigned)top->io_memFbWrite_read,
            (unsigned)top->io_memFbWrite_write,
            (unsigned)top->io_memFbWrite_address,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_valid,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_ready,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__writeBurstActive,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__writeRspPending,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_mem_write,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_mem_address,
            (unsigned)top->io_memFbColorRead_read,
            (unsigned)top->io_memFbColorRead_readDataValid,
            (unsigned)top->io_memFbColorRead_address,
            (unsigned)top->io_memFbAuxRead_read,
            (unsigned)top->io_memFbAuxRead_readDataValid,
            (unsigned)top->io_memFbAuxRead_address,
            (unsigned)top->io_memTex_read,
            (unsigned)top->io_memTex_readDataValid,
            (unsigned)top->io_memTex_address,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess_io_busy,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__inFlightCount,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_colorBusy,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_auxBusy,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            0u,
            (unsigned)last_tex_read_addr,
            (unsigned long)last_tex_read_cycle,
            (unsigned)last_tex_rvalid_data,
            (unsigned long)last_tex_rvalid_cycle,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__frontdoor__DOT__io_texReadBus_cmd_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__frontdoor__DOT__io_texReadBus_cmd_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__frontdoor__DOT__io_texReadBus_rsp_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__frontdoor__DOT__io_texReadBus_rsp_ready);
    fflush(stderr);
}

static void bus_write(uint32_t addr, uint32_t data) {
    const uint64_t t0 = sim_time;
    mmio_begin(addr, true, data);
    maybe_log_mmio("WRITE", "begin", addr, data, 0);

    uint32_t idle_cycles = 0;
    uint64_t iter = 0;
    ProgressSnapshot last_progress = capture_progress_snapshot();
    uint64_t fb_write_cycles = 0;
    uint64_t fb_cmd_ready_cycles = 0;
    uint64_t fb_cmd_valid_cycles = 0;
    uint64_t fb_waitrequestn_cycles = 0;
    uint64_t color_req_cycles = 0;
    uint64_t aux_req_cycles = 0;
    uint64_t color_pipe_cycles = 0;
    uint64_t aux_pipe_cycles = 0;
    uint64_t color_busy_cycles = 0;
    uint64_t aux_busy_cycles = 0;
    uint64_t rast_out_cycles = 0;
    uint64_t tmu_in_cycles = 0;
    uint64_t tmu_out_cycles = 0;
    uint64_t cc_in_cycles = 0;
    uint64_t fog_in_cycles = 0;
    uint64_t fb_in_cycles = 0;
    uint64_t wc_in_cycles = 0;
    uint64_t tex_cmd_valid_cycles = 0;
    uint64_t tex_cmd_ready_cycles = 0;
    uint64_t tex_accept_read_cycles = 0;
    uint64_t tex_slot_available_cycles = 0;
    uint64_t tex_storage_accept_cycles = 0;
    uint64_t tex_pending_read_cycles = 0;
    uint64_t tex_read_cycles = 0;
    uint64_t tex_rvalid_cycles = 0;
    uint64_t tex_addr_changes = 0;
    uint32_t last_tex_addr = 0;
    bool have_last_tex_addr = false;
    uint64_t fb_addr_changes = 0;
    uint32_t last_fb_addr = 0;
    bool have_last_fb_addr = false;
    const uint64_t stall_log_period = stall_debug ? 1000 : ((addr == 0x000000) ? 10000 : 100000);
    while (idle_cycles < mmio_timeout_cycles) {
        eval_inputs();
        auto *root = top->rootp;
        ProgressSnapshot current_progress = capture_progress_snapshot();
        if (progress_changed(current_progress, last_progress)) {
            last_progress = current_progress;
            idle_cycles = 0;
        }
        if (top->io_memFbWrite_write) {
            fb_write_cycles++;
            uint32_t cur = top->io_memFbWrite_address;
            if (have_last_fb_addr && cur != last_fb_addr) fb_addr_changes++;
            last_fb_addr = cur;
            have_last_fb_addr = true;
        }
        if (root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_valid) fb_cmd_valid_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_ready) fb_cmd_ready_cycles++;
        if (top->io_memFbWrite_waitRequestn) fb_waitrequestn_cycles++;
        color_req_cycles += 0;
        aux_req_cycles += 0;
        color_pipe_cycles += 0;
        aux_pipe_cycles += 0;
        if (root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_colorBusy) color_busy_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_auxBusy) aux_busy_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__rasterizer_1__DOT__o_valid) rast_out_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_input_valid) tmu_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_output_valid) tmu_out_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__colorCombine_1__DOT__io_input_valid) cc_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fog_1__DOT__io_input_valid) fog_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__io_input_valid) fb_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor__DOT__i_fromPipeline_valid) wc_in_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_bmb_cmd_valid) tex_cmd_valid_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_bmb_cmd_ready) tex_cmd_ready_cycles++;
        if (top->io_memTex_read) tex_accept_read_cycles++;
        if (!root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__pendingReadTraffic) tex_slot_available_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readStorageCanAcceptCmd) tex_storage_accept_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__pendingReadTraffic) tex_pending_read_cycles++;
        if (top->io_memTex_read) {
            tex_read_cycles++;
            uint32_t cur = top->io_memTex_address;
            if (have_last_tex_addr && cur != last_tex_addr) tex_addr_changes++;
            last_tex_addr = cur;
            have_last_tex_addr = true;
        }
        if (top->io_memTex_readDataValid) tex_rvalid_cycles++;
        if (!top->io_h2fLw_waitrequest) {
            maybe_log_mmio("WRITE", "accept", addr, data, iter);
            tick_one();
            break;
        }
        if (mmio_debug && (iter < 8 || (iter % 1000000) == 0)) {
            maybe_log_mmio("WRITE", "wait", addr, data, iter);
        }
        if (stall_debug && iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("WRITE", "cmd", addr, iter);
        }
        tick_one();
        if (cycle_limit_hit) break;
        idle_cycles++;
        iter++;
    }
    if (idle_cycles >= mmio_timeout_cycles && !cycle_limit_hit) {
        auto *root = top->rootp;
        if (stall_debug) log_stalled_bus_state("WRITE", "timeout", addr, iter);
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_write(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
        dump_timeout_debug("write-cmd-timeout", addr);
        fprintf(stderr,
                "[sim_harness_de10] WRITE timeout stats: fb_write_cycles=%lu/%lu fb_cmd_valid=%lu fb_cmd_ready=%lu fb_waitrequestn=%lu colorReq=%lu auxReq=%lu colorPipe=%lu auxPipe=%lu colorBusy=%lu auxBusy=%lu rastOut=%lu tmuIn=%lu tmuOut=%lu ccIn=%lu fogIn=%lu fbIn=%lu wcIn=%lu texCmdValid=%lu texCmdReady=%lu texAccept=%lu texSlotAvail=%lu texStorageOk=%lu texPending=%lu texRead=%lu texRvalid=%lu texModel=%lu/%lu/%lu/%u texBridge=%lu texState=%u/%u/%u/0x%08x texWriteActive=%u texWriteRsp=%u texOutReadBeats=%u texQueuedRead=%u texCanLaunch=%u texAddrChanges=%lu lastTex=0x%08x fb_addr_changes=%lu last_fb_addr=0x%08x\n",
                (unsigned long)fb_write_cycles, (unsigned long)iter,
                (unsigned long)fb_cmd_valid_cycles, (unsigned long)fb_cmd_ready_cycles,
                (unsigned long)fb_waitrequestn_cycles, (unsigned long)color_req_cycles,
                (unsigned long)aux_req_cycles, (unsigned long)color_pipe_cycles,
                (unsigned long)aux_pipe_cycles, (unsigned long)color_busy_cycles,
                (unsigned long)aux_busy_cycles, (unsigned long)rast_out_cycles,
                (unsigned long)tmu_in_cycles, (unsigned long)tmu_out_cycles,
                (unsigned long)cc_in_cycles, (unsigned long)fog_in_cycles,
                (unsigned long)fb_in_cycles, (unsigned long)wc_in_cycles,
                (unsigned long)tex_cmd_valid_cycles, (unsigned long)tex_cmd_ready_cycles,
                (unsigned long)tex_accept_read_cycles, (unsigned long)tex_slot_available_cycles,
                (unsigned long)tex_storage_accept_cycles, (unsigned long)tex_pending_read_cycles,
                (unsigned long)tex_read_cycles, (unsigned long)tex_rvalid_cycles,
                (unsigned long)tex_model_read_launch_count,
                (unsigned long)tex_model_read_reject_count,
                (unsigned long)tex_model_rvalid_count,
                (unsigned)tex_model_last_launch_burst,
                (unsigned long)tex_bridge_launch_count,
                (unsigned)tex_state.read_valid, (unsigned)tex_state.read_delay,
                (unsigned)tex_state.beats_left, (unsigned)tex_state.next_addr,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__writeBurstActive,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__writeRspPending,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__queuedReadCmdValid,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__canLaunchRead,
                (unsigned long)tex_addr_changes, (unsigned)last_tex_addr,
                (unsigned long)fb_addr_changes, (unsigned)last_fb_addr);
        cycle_limit_hit = true;
        mmio_timeout_hit = true;
    }

    mmio_end();
    if (!cycle_limit_hit) tick_one();
    maybe_log_mmio("WRITE", "done", addr, data, iter);
    total_write_ticks += (sim_time - t0) / 2;
    total_write_count++;
}

static void bus_write_masked(uint32_t addr, uint32_t data, uint8_t byteenable) {
    const uint64_t t0 = sim_time;
    mmio_begin(addr, true, data, byteenable);
    maybe_log_mmio("WRITE", "begin", addr, data, 0);

    uint32_t idle_cycles = 0;
    uint64_t iter = 0;
    ProgressSnapshot last_progress = capture_progress_snapshot();
    uint64_t fb_write_cycles = 0;
    uint64_t fb_cmd_ready_cycles = 0;
    uint64_t fb_cmd_valid_cycles = 0;
    uint64_t fb_waitrequestn_cycles = 0;
    uint64_t color_req_cycles = 0;
    uint64_t aux_req_cycles = 0;
    uint64_t color_pipe_cycles = 0;
    uint64_t aux_pipe_cycles = 0;
    uint64_t color_busy_cycles = 0;
    uint64_t aux_busy_cycles = 0;
    uint64_t rast_out_cycles = 0;
    uint64_t tmu_in_cycles = 0;
    uint64_t tmu_out_cycles = 0;
    uint64_t cc_in_cycles = 0;
    uint64_t fog_in_cycles = 0;
    uint64_t fb_in_cycles = 0;
    uint64_t wc_in_cycles = 0;
    uint64_t tex_cmd_valid_cycles = 0;
    uint64_t tex_cmd_ready_cycles = 0;
    uint64_t tex_accept_read_cycles = 0;
    uint64_t tex_slot_available_cycles = 0;
    uint64_t tex_storage_accept_cycles = 0;
    uint64_t tex_pending_read_cycles = 0;
    uint64_t tex_read_cycles = 0;
    uint64_t tex_rvalid_cycles = 0;
    uint64_t tex_addr_changes = 0;
    uint32_t last_tex_addr = 0;
    bool have_last_tex_addr = false;
    uint64_t fb_addr_changes = 0;
    uint32_t last_fb_addr = 0;
    bool have_last_fb_addr = false;
    const uint64_t stall_log_period = stall_debug ? 1000 : ((addr == 0x000000) ? 10000 : 100000);
    while (idle_cycles < mmio_timeout_cycles) {
        eval_inputs();
        auto *root = top->rootp;
        ProgressSnapshot current_progress = capture_progress_snapshot();
        if (progress_changed(current_progress, last_progress)) {
            last_progress = current_progress;
            idle_cycles = 0;
        }
        if (top->io_memFbWrite_write) {
            fb_write_cycles++;
            uint32_t cur = top->io_memFbWrite_address;
            if (have_last_fb_addr && cur != last_fb_addr) fb_addr_changes++;
            last_fb_addr = cur;
            have_last_fb_addr = true;
        }
        if (root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_valid) fb_cmd_valid_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__fbWriteBridge__DOT__io_bmb_cmd_ready) fb_cmd_ready_cycles++;
        if (top->io_memFbWrite_waitRequestn) fb_waitrequestn_cycles++;
        color_req_cycles += 0;
        aux_req_cycles += 0;
        color_pipe_cycles += 0;
        aux_pipe_cycles += 0;
        if (root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_colorBusy) color_busy_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__framebufferMem_io_status_auxBusy) aux_busy_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__rasterizer_1__DOT__o_valid) rast_out_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_input_valid) tmu_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__io_output_valid) tmu_out_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__colorCombine_1__DOT__io_input_valid) cc_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fog_1__DOT__io_input_valid) fog_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__fbAccess__DOT__io_input_valid) fb_in_cycles++;
        if (root->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__writeColor__DOT__i_fromPipeline_valid) wc_in_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_bmb_cmd_valid) tex_cmd_valid_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__io_bmb_cmd_ready) tex_cmd_ready_cycles++;
        if (top->io_memTex_read) tex_accept_read_cycles++;
        if (!root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__pendingReadTraffic) tex_slot_available_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__readStorageCanAcceptCmd) tex_storage_accept_cycles++;
        if (root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__pendingReadTraffic) tex_pending_read_cycles++;
        if (top->io_memTex_read) {
            tex_read_cycles++;
            uint32_t cur = top->io_memTex_address;
            if (have_last_tex_addr && cur != last_tex_addr) tex_addr_changes++;
            last_tex_addr = cur;
            have_last_tex_addr = true;
        }
        if (top->io_memTex_readDataValid) tex_rvalid_cycles++;
        if (!top->io_h2fLw_waitrequest) {
            maybe_log_mmio("WRITE", "accept", addr, data, iter);
            tick_one();
            break;
        }
        if (mmio_debug && (iter < 8 || (iter % 1000000) == 0)) {
            maybe_log_mmio("WRITE", "wait", addr, data, iter);
        }
        if (stall_debug && iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("WRITE", "cmd", addr, iter);
        }
        tick_one();
        if (cycle_limit_hit) break;
        idle_cycles++;
        iter++;
    }
    if (idle_cycles >= mmio_timeout_cycles && !cycle_limit_hit) {
        auto *root = top->rootp;
        if (stall_debug) log_stalled_bus_state("WRITE", "timeout", addr, iter);
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_write_masked(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
        dump_timeout_debug("write-masked-cmd-timeout", addr);
        fprintf(stderr,
                "[sim_harness_de10] WRITE timeout stats: fb_write_cycles=%lu/%lu fb_cmd_valid=%lu fb_cmd_ready=%lu fb_waitrequestn=%lu colorReq=%lu auxReq=%lu colorPipe=%lu auxPipe=%lu colorBusy=%lu auxBusy=%lu rastOut=%lu tmuIn=%lu tmuOut=%lu ccIn=%lu fogIn=%lu fbIn=%lu wcIn=%lu texCmdValid=%lu texCmdReady=%lu texAccept=%lu texSlotAvail=%lu texStorageOk=%lu texPending=%lu texRead=%lu texRvalid=%lu texModel=%lu/%lu/%lu/%u texBridge=%lu texState=%u/%u/%u/0x%08x texWriteActive=%u texWriteRsp=%u texOutReadBeats=%u texQueuedRead=%u texCanLaunch=%u texAddrChanges=%lu lastTex=0x%08x fb_addr_changes=%lu last_fb_addr=0x%08x\n",
                (unsigned long)fb_write_cycles, (unsigned long)iter,
                (unsigned long)fb_cmd_valid_cycles, (unsigned long)fb_cmd_ready_cycles,
                (unsigned long)fb_waitrequestn_cycles, (unsigned long)color_req_cycles,
                (unsigned long)aux_req_cycles, (unsigned long)color_pipe_cycles,
                (unsigned long)aux_pipe_cycles, (unsigned long)color_busy_cycles,
                (unsigned long)aux_busy_cycles, (unsigned long)rast_out_cycles,
                (unsigned long)tmu_in_cycles, (unsigned long)tmu_out_cycles,
                (unsigned long)cc_in_cycles, (unsigned long)fog_in_cycles,
                (unsigned long)fb_in_cycles, (unsigned long)wc_in_cycles,
                (unsigned long)tex_cmd_valid_cycles, (unsigned long)tex_cmd_ready_cycles,
                (unsigned long)tex_accept_read_cycles, (unsigned long)tex_slot_available_cycles,
                (unsigned long)tex_storage_accept_cycles, (unsigned long)tex_pending_read_cycles,
                (unsigned long)tex_read_cycles, (unsigned long)tex_rvalid_cycles,
                (unsigned long)tex_model_read_launch_count,
                (unsigned long)tex_model_read_reject_count,
                (unsigned long)tex_model_rvalid_count,
                (unsigned)tex_model_last_launch_burst,
                (unsigned long)tex_bridge_launch_count,
                (unsigned)tex_state.read_valid, (unsigned)tex_state.read_delay,
                (unsigned)tex_state.beats_left, (unsigned)tex_state.next_addr,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__writeBurstActive,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__writeRspPending,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__outstandingReadBeats,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__queuedReadCmdValid,
                (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridgeInst__DOT__canLaunchRead,
                (unsigned long)tex_addr_changes, (unsigned)last_tex_addr,
                (unsigned long)fb_addr_changes, (unsigned)last_fb_addr);
        cycle_limit_hit = true;
        mmio_timeout_hit = true;
    }

    mmio_end();
    if (!cycle_limit_hit) tick_one();
    maybe_log_mmio("WRITE", "done", addr, data, iter);
    total_write_ticks += (sim_time - t0) / 2;
    total_write_count++;
}

static uint32_t bus_read(uint32_t addr) {
    const uint64_t t0 = sim_time;
    mmio_begin(addr, false, 0);
    maybe_log_mmio("READ", "begin", addr, 0, 0);

    uint32_t idle_cycles = 0;
    uint64_t iter = 0;
    ProgressSnapshot last_progress = capture_progress_snapshot();
    const uint64_t stall_log_period = (addr == 0x000000) ? 10000 : 100000;
    while (idle_cycles < mmio_timeout_cycles) {
        eval_inputs();
        ProgressSnapshot current_progress = capture_progress_snapshot();
        if (progress_changed(current_progress, last_progress)) {
            last_progress = current_progress;
            idle_cycles = 0;
        }
        if (!top->io_h2fLw_waitrequest) {
            maybe_log_mmio("READ", "accept", addr, 0, iter);
            tick_one();
            break;
        }
        if (mmio_debug && (iter < 8 || (iter % 1000000) == 0)) {
            maybe_log_mmio("READ", "wait", addr, 0, iter);
        }
        if (stall_debug && iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("READ", "cmd", addr, iter);
        }
        tick_one();
        if (cycle_limit_hit) break;
        idle_cycles++;
        iter++;
    }
    if (idle_cycles >= mmio_timeout_cycles && !cycle_limit_hit) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_read(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
        cycle_limit_hit = true;
        mmio_timeout_hit = true;
    }

    mmio_end();
    if (!cycle_limit_hit) tick_one();

    idle_cycles = 0;
    iter = 0;
    last_progress = capture_progress_snapshot();
    while (idle_cycles < mmio_timeout_cycles) {
        eval_inputs();
        ProgressSnapshot current_progress = capture_progress_snapshot();
        if (progress_changed(current_progress, last_progress)) {
            last_progress = current_progress;
            idle_cycles = 0;
        }
        if (top->io_h2fLw_readdatavalid) {
            const uint32_t data = top->io_h2fLw_readdata;
            maybe_log_mmio("READ", "rsp", addr, data, iter);
            tick_one();
            const uint64_t ticks = (sim_time - t0) / 2;
            total_read_ticks += ticks;
            total_read_count++;
            return data;
        }
        if (mmio_debug && (iter < 8 || (iter % 1000000) == 0)) {
            maybe_log_mmio("READ", "rspwait", addr, 0, iter);
        }
        if (stall_debug && iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("READ", "rsp", addr, iter);
        }
        tick_one();
        if (cycle_limit_hit) break;
        idle_cycles++;
        iter++;
    }

    const uint64_t ticks = (sim_time - t0) / 2;
    if (!cycle_limit_hit && idle_cycles >= mmio_timeout_cycles) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_read(0x%06x) rsp timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
        cycle_limit_hit = true;
        mmio_timeout_hit = true;
    }
    total_read_ticks += ticks;
    total_read_count++;
    return 0xDEAD0000;
}

int sim_init(void) {
    contextp = new VerilatedContext;
    if (!contextp) return -1;
    contextp->commandArgs(0, (const char**)nullptr);
    contextp->threads(1);

#ifdef VM_TRACE_FST
    const char* fst_path = getenv("SIM_FST");
    if (fst_path) {
        contextp->traceEverOn(true);
    }
#endif

    fb_mem.assign(FB_WORD_COUNT, 0);
    tex_mem.assign(TEX_WORD_COUNT, 0);
    fb_write_state = {0, 0, 0, 0, 0};
    fb_color_write_state = {0, 0, 0, 0, 0};
    fb_aux_write_state = {0, 0, 0, 0, 0};
    fb_color_state = {0, 0, 0, 0, 0};
    fb_aux_state = {0, 0, 0, 0, 0};
    tex_state = {0, 0, 0, 0, 0};
    clear_mem_cmd_samples();
    total_read_ticks = 0;
    total_read_count = 0;
    total_write_ticks = 0;
    total_write_count = 0;
    tex_model_read_launch_count = 0;
    tex_model_read_reject_count = 0;
    tex_model_rvalid_count = 0;
    tex_access_dump_writes = 0;
    compare_pop_count = 0;
    compare_pop_all_hits_count = 0;
    compare_pop_any_miss_count = 0;
    compare_pop_issue_needed_sum = 0;
    compare_pop_tap_hit_sum = 0;
    compare_pop_nonfast_zero_issue = 0;
    tex_model_last_launch_burst = 0;
    tex_bridge_launch_count = 0;
    prev_tex_outstanding = 0;
    last_tex_read_cycle = 0;
    last_tex_read_addr = 0;
    last_tex_rvalid_cycle = 0;
    last_tex_rvalid_data = 0;
    sim_time = 0;
    cycle_limit = 0;
    cycle_limit_hit = false;
    mmio_timeout_hit = false;
    quit_requested = 0;

    top = new VCoreDe10{contextp};
    if (!top) return -1;

    top->clk = 0;
    top->reset = 1;
    top->io_h2fLw_address = 0;
    top->io_h2fLw_read = 0;
    top->io_h2fLw_write = 0;
    top->io_h2fLw_byteenable = 0;
    top->io_h2fLw_writedata = 0;
    drive_memory_inputs();

    for (int i = 0; i < 10; i++) tick_one();
    top->reset = 0;
    for (int i = 0; i < 10; i++) tick_one();

#ifdef VM_TRACE_FST
    if (fst_path) {
        tfp = new VerilatedFstC;
        top->trace(tfp, 99);
        tfp->open(fst_path);
        fprintf(stderr, "[sim_harness_de10] FST tracing to %s\n", fst_path);
    }
#endif

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    const char* cycle_limit_str = getenv("SIM_CYCLE_LIMIT");
    if (cycle_limit_str) {
        cycle_limit = strtoull(cycle_limit_str, nullptr, 0);
        if (cycle_limit) {
            fprintf(stderr, "[sim_harness_de10] Cycle limit set to %lu ticks\n",
                    (unsigned long)cycle_limit);
        }
    }

    const char* mmio_timeout_str = getenv("SIM_DE10_MMIO_TIMEOUT");
    if (mmio_timeout_str) {
        mmio_timeout_cycles = (uint32_t)strtoul(mmio_timeout_str, nullptr, 0);
    }

    const char* mmio_debug_str = getenv("SIM_DE10_MMIO_DEBUG");
    if (mmio_debug_str) {
        mmio_debug = atoi(mmio_debug_str);
    }

    const char* stall_debug_str = getenv("SIM_DE10_STALL_DEBUG");
    if (stall_debug_str) {
        stall_debug = atoi(stall_debug_str);
    }

    const char* tex_trace_str = getenv("SIM_DE10_TEX_TRACE");
    if (tex_trace_str) {
        tex_trace = atoi(tex_trace_str);
    }

    const char* tex_trace_after_str = getenv("SIM_DE10_TEX_TRACE_AFTER");
    if (tex_trace_after_str) {
        tex_trace_after_cycle = strtoull(tex_trace_after_str, nullptr, 0);
    }

    const char* tex_trace_min_addr_str = getenv("SIM_DE10_TEX_TRACE_MIN_ADDR");
    if (tex_trace_min_addr_str) {
        tex_trace_min_addr = strtoul(tex_trace_min_addr_str, nullptr, 0);
    }
    const char* fb_trace_str = getenv("SIM_DE10_FB_TRACE");
    if (fb_trace_str) {
        fb_trace = atoi(fb_trace_str);
    }
    const char* fb_trace_after_str = getenv("SIM_DE10_FB_TRACE_AFTER");
    if (fb_trace_after_str) {
        fb_trace_after_cycle = strtoull(fb_trace_after_str, nullptr, 0);
    }
    const char* fb_trace_min_addr_str = getenv("SIM_DE10_FB_TRACE_MIN_ADDR");
    if (fb_trace_min_addr_str) {
        fb_trace_min_addr = strtoul(fb_trace_min_addr_str, nullptr, 0);
    }

    const char* mem_read_delay_str = getenv("SIM_DE10_MEM_READ_DELAY");
    if (mem_read_delay_str) {
        mem_read_delay = strtoul(mem_read_delay_str, nullptr, 0);
    }

    fprintf(stderr, "[sim_harness_de10] Initialized CoreDe10 Verilator model\n");
    return 0;
}

void sim_tex_access_dump(const char *path) {
    close_tex_access_dump();
    open_tex_access_dump(path);
}

void sim_tex_access_close(void) {
    close_tex_access_dump();
}

void sim_shutdown(void) {
    fprintf(stderr,
            "[sim_harness_de10] compare pop stats: count=%lu allHits=%lu anyMiss=%lu issueNeededSum=%lu tapHitSum=%lu nonfastZeroIssue=%lu\n",
            (unsigned long)compare_pop_count,
            (unsigned long)compare_pop_all_hits_count,
            (unsigned long)compare_pop_any_miss_count,
            (unsigned long)compare_pop_issue_needed_sum,
            (unsigned long)compare_pop_tap_hit_sum,
            (unsigned long)compare_pop_nonfast_zero_issue);
    if (tex_access_file) {
        fprintf(stderr, "[sim_harness_de10] tex access dump writes=%lu\n",
                (unsigned long)tex_access_dump_writes);
    }
    close_tex_access_dump();
#ifdef VM_TRACE_FST
    if (tfp) {
        tfp->close();
        delete tfp;
        tfp = nullptr;
    }
#endif
    if (top) {
        top->final();
        delete top;
        top = nullptr;
    }
    fb_mem.clear();
    tex_mem.clear();
    fprintf(stderr, "[sim_harness_de10] Shutdown after %lu cycles (%lu R, %lu W, snoopCW=%lu snoopAW=%lu buf0=%lu buf1=%lu)\n",
            (unsigned long)(sim_time / 2),
            (unsigned long)total_read_count, (unsigned long)total_write_count,
            (unsigned long)snoop_color_write_count, (unsigned long)snoop_aux_write_count,
            (unsigned long)snoop_color_write_buf0, (unsigned long)snoop_color_write_buf1);
}

void sim_write(uint32_t addr, uint32_t data) {
    if (mmio_timeout_hit) return;
    bus_write(addr, data);
}

void sim_write16(uint32_t addr, uint16_t data) {
    if (mmio_timeout_hit) return;
    const int lane_hi = (addr & 0x2) != 0;
    const uint32_t packed_data = lane_hi ? ((uint32_t)data << 16) : (uint32_t)data;
    const uint8_t mask = lane_hi ? 0xC : 0x3;
    bus_write_masked(addr, packed_data, mask);
}

uint32_t sim_read(uint32_t addr) {
    if (mmio_timeout_hit) return 0;
    return bus_read(addr);
}

uint32_t sim_idle_wait(void) {
    #define SST_BUSY (1u << 9)
    #define SST_FIFOFREE_MASK 0x3Fu

    int timeout = 5000000;
    uint64_t t0 = sim_time;
    int idle_count = 0;
    while (timeout > 0) {
        uint32_t status = bus_read(0x000000);
        int busy = (status & SST_BUSY) != 0;
        int fifo_free = status & SST_FIFOFREE_MASK;
        if (!busy && fifo_free == 0x3F) {
            if (++idle_count >= 3) return status;
        } else {
            idle_count = 0;
        }
        if (cycle_limit_hit) return status;
        timeout--;
    }

    uint32_t status = bus_read(0x000000);
    fprintf(stderr, "[sim_harness_de10] WARNING: idle_wait timeout after 1M ticks (%lu elapsed)! status=0x%08x\n",
            (unsigned long)((sim_time - t0) / 2), status);
    return status;
}

void sim_tick(int n) {
    for (int i = 0; i < n; i++) tick_one();
}

void sim_flush_fb_cache(void) {
    if (!top) return;
    top->rootp->CoreDe10__DOT__core_1__DOT__io_flushFbCaches = 1;
    sim_idle_wait();
    top->rootp->CoreDe10__DOT__core_1__DOT__io_flushFbCaches = 0;
    tick_one();
}

void sim_invalidate_fb_cache(void) {
    if (!top) return;
    top->rootp->CoreDe10__DOT__core_1__DOT__io_flushFbCaches = 1;
    sim_idle_wait();
    top->rootp->CoreDe10__DOT__core_1__DOT__io_flushFbCaches = 0;
    fprintf(stderr,
            "[sim_harness_de10] Flushed framebuffer caches\n");
    tick_one();
}

void sim_read_fb(uint32_t byte_offset, uint32_t* dst, uint32_t word_count) {
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < FB_WORD_COUNT; i++) {
        dst[i] = fb_mem[idx + i];
    }
}

void sim_write_fb_bulk(uint32_t byte_offset, const uint32_t* src, uint32_t word_count) {
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < FB_WORD_COUNT; i++) {
        fb_mem[idx + i] = src[i];
    }
}

void sim_write_tex_bulk(uint32_t byte_offset, const uint32_t* src, uint32_t word_count) {
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < TEX_WORD_COUNT; i++) {
        tex_mem[idx + i] = src[i];
    }
}

void sim_read_tex(uint32_t byte_offset, uint32_t* dst, uint32_t word_count) {
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < TEX_WORD_COUNT; i++) {
        dst[i] = tex_mem[idx + i];
    }
}

void sim_set_swap_count(uint32_t count) {
    if (!top) return;
    top->rootp->CoreDe10__DOT__core_1__DOT__controlPlane_swapBuffer__DOT__swapCountReg = count & 0x3u;
}

uint32_t sim_get_swap_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__controlPlane_swapBuffer__DOT__swapCountReg & 0x3u;
}

void sim_dump_first_fb_writes(const char *path, uint32_t limit) {
    (void)path;
    (void)limit;
}

uint64_t sim_get_cycle(void) {
    return sim_time / 2;
}

uint64_t sim_get_total_read_ticks(void) {
    return total_read_ticks;
}

uint64_t sim_get_total_read_count(void) {
    return total_read_count;
}

uint64_t sim_get_total_write_ticks(void) {
    return total_write_ticks;
}

uint64_t sim_get_total_write_count(void) {
    return total_write_count;
}

uint32_t sim_get_pixels_in(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__regBank__DOT__io_statistics_pixelsIn;
}

uint32_t sim_get_pixels_out(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__regBank__DOT__io_statistics_pixelsOut;
}

uint32_t sim_get_fb_fill_hits(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_fillHits;
}

uint32_t sim_get_fb_fill_misses(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_fillMisses;
}

uint32_t sim_get_fb_fill_burst_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_fillBurstCount;
}

uint32_t sim_get_fb_fill_burst_beats(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_fillBurstBeats;
}

uint32_t sim_get_fb_fill_stall_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_fillStallCycles;
}

uint32_t sim_get_fb_write_stall_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeStallCycles;
}

uint32_t sim_get_fb_write_drain_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeDrainCount;
}

uint32_t sim_get_fb_write_full_drain_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeFullDrainCount;
}

uint32_t sim_get_fb_write_partial_drain_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writePartialDrainCount;
}

uint32_t sim_get_fb_write_drain_reason_full_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeDrainReasonFullCount;
}

uint32_t sim_get_fb_write_drain_reason_rotate_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeDrainReasonRotateCount;
}

uint32_t sim_get_fb_write_drain_reason_flush_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeDrainReasonFlushCount;
}

uint32_t sim_get_fb_write_drain_dirty_word_total(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeDrainDirtyWordTotal;
}

uint32_t sim_get_fb_write_rotate_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeRotateBlockedCycles;
}

uint32_t sim_get_fb_write_single_word_drain_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeSingleWordDrainCount;
}

uint32_t sim_get_fb_write_single_word_drain_start_at_zero_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeSingleWordDrainStartAtZeroCount;
}

uint32_t sim_get_fb_write_single_word_drain_start_at_last_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeSingleWordDrainStartAtLastCount;
}

uint32_t sim_get_fb_write_rotate_adjacent_line_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeRotateAdjacentLineCount;
}

uint32_t sim_get_fb_write_rotate_same_line_gap_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeRotateSameLineGapCount;
}

uint32_t sim_get_fb_write_rotate_other_line_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_writeRotateOtherLineCount;
}

uint32_t sim_get_fb_mem_color_write_cmd_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memColorWriteCmdCount;
}

uint32_t sim_get_fb_mem_aux_write_cmd_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memAuxWriteCmdCount;
}

uint32_t sim_get_fb_mem_color_read_cmd_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memColorReadCmdCount;
}

uint32_t sim_get_fb_mem_aux_read_cmd_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memAuxReadCmdCount;
}

uint32_t sim_get_fb_mem_lfb_read_cmd_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memLfbReadCmdCount;
}

uint32_t sim_get_fb_mem_color_write_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memColorWriteBlockedCycles;
}

uint32_t sim_get_fb_mem_aux_write_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memAuxWriteBlockedCycles;
}

uint32_t sim_get_fb_mem_color_read_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memColorReadBlockedCycles;
}

uint32_t sim_get_fb_mem_aux_read_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memAuxReadBlockedCycles;
}

uint32_t sim_get_fb_mem_lfb_read_blocked_cycles(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_memLfbReadBlockedCycles;
}

uint32_t sim_get_fb_read_req_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqCount;
}

uint32_t sim_get_fb_read_req_forward_step_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqForwardStepCount;
}

uint32_t sim_get_fb_read_req_backward_step_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqBackwardStepCount;
}

uint32_t sim_get_fb_read_req_same_word_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqSameWordCount;
}

uint32_t sim_get_fb_read_req_same_line_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqSameLineCount;
}

uint32_t sim_get_fb_read_req_other_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readReqOtherCount;
}

uint32_t sim_get_fb_read_single_beat_burst_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readSingleBeatBurstCount;
}

uint32_t sim_get_fb_read_multi_beat_burst_count(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readMultiBeatBurstCount;
}

uint32_t sim_get_fb_read_max_queue_occupancy(void) {
    if (!top) return 0;
    return top->rootp->CoreDe10__DOT__core_1__DOT__framebufferMem_io_stats_readMaxQueueOccupancy;
}

uint32_t sim_get_tex_fill_hits(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillHits;
}
uint32_t sim_get_tex_fill_misses(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillMisses;
}
uint32_t sim_get_tex_fill_burst_count(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillBurstCount;
}
uint32_t sim_get_tex_fill_burst_beats(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillBurstBeats;
}
uint32_t sim_get_tex_fill_stall_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillStallCycles;
}
uint32_t sim_get_tex_fast_bilinear_hits(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFastBilinearHits;
}
uint32_t sim_get_tex_compare_miss_samples(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texCompareMissSamples;
}
uint32_t sim_get_tex_lookup_blocked_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texLookupBlockedCycles;
}
uint32_t sim_get_tex_lookup_blocked_by_owner_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texLookupBlockedByOwnerCycles;
}
uint32_t sim_get_tex_lookup_blocked_by_fill_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texLookupBlockedByFillCycles;
}
uint32_t sim_get_tex_lookup_blocked_by_hold_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texLookupBlockedByHoldCycles;
}
uint32_t sim_get_tex_lookup_blocked_by_live_cycles(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texLookupBlockedByLiveCycles;
}
uint32_t sim_get_tex_fill_evict_valid(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillEvictValid;
}
uint32_t sim_get_tex_fill_evict_ready(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillEvictReady;
}
uint32_t sim_get_tex_fill_evict_inflight(void) {
    if (!top) return 0;
    return (uint32_t)top->rootp->CoreDe10__DOT__core_1__DOT__pixelPipeline_1__DOT__tmu_1__DOT__texturePath__DOT__texFillEvictInflight;
}

int sim_stalled(void) {
    return mmio_timeout_hit ? 1 : 0;
}
