/*
 * sim_harness_de10.cpp -- Verilator driver for CoreDe10.
 *
 * This exercises the DE10-facing lightweight MMIO wrapper and the external
 * Avalon-MM memory ports instead of talking directly to Core's BMB bus.
 */

#include "sim_harness.h"
#include "VCoreDe10.h"
#include "VCoreDe10_FramebufferPlaneBuffer.h"
#include "VCoreDe10_FramebufferPlaneReader.h"
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
static constexpr uint32_t TEX_WORD_COUNT = (8u * 1024u * 1024u) / 4u;
static constexpr uint32_t TEX_BASE_BYTES = 4u * 1024u * 1024u;

static std::vector<uint32_t> fb_mem;
static std::vector<uint32_t> tex_mem;

struct AvalonMemState {
    uint8_t read_delay;
    uint8_t read_valid;
    uint32_t read_data;
};

static AvalonMemState fb_write_state = {0, 0, 0};
static AvalonMemState fb_color_state = {0, 0, 0};
static AvalonMemState fb_aux_state = {0, 0, 0};
static AvalonMemState tex_state = {0, 0, 0};

static uint64_t total_read_ticks = 0;
static uint64_t total_read_count = 0;
static uint64_t total_write_ticks = 0;
static uint64_t total_write_count = 0;
static uint64_t last_tex_read_cycle = 0;
static uint32_t last_tex_read_addr = 0;
static uint64_t last_tex_rvalid_cycle = 0;
static uint32_t last_tex_rvalid_data = 0;
static int mmio_debug = 0;
static int stall_debug = 0;
static int tex_trace = 0;
static uint32_t mem_read_delay = 0;
static uint32_t tex_trace_count = 0;

static inline uint32_t effective_mem_read_delay(void) {
    return mem_read_delay + 1;
}
static uint64_t tex_trace_after_cycle = 0;
static uint32_t tex_trace_min_addr = 0;

static inline bool tex_trace_enabled_now(uint32_t addr) {
    return tex_trace && ((sim_time / 2) >= tex_trace_after_cycle) && (addr >= tex_trace_min_addr);
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

static void drive_memory_inputs(void) {
    top->io_memFbWrite_waitRequestn = 1;
    top->io_memFbWrite_readDataValid = fb_write_state.read_valid;
    top->io_memFbWrite_readData = fb_write_state.read_data;

    top->io_memFbColorRead_waitRequestn = 1;
    top->io_memFbColorRead_readDataValid = fb_color_state.read_valid;
    top->io_memFbColorRead_readData = fb_color_state.read_data;

    top->io_memFbAuxRead_waitRequestn = 1;
    top->io_memFbAuxRead_readDataValid = fb_aux_state.read_valid;
    top->io_memFbAuxRead_readData = fb_aux_state.read_data;

    top->io_memTex_waitRequestn = 1;
    top->io_memTex_readDataValid = tex_state.read_valid;
    top->io_memTex_readData = tex_state.read_data;
    if (tex_trace && tex_state.read_valid && tex_trace_count < 512) {
        fprintf(stderr,
                "[sim_harness_de10] TEX rvalid data=0x%08x cycle=%lu bridge(outstanding=%u beats=%u rspPending=%u)\n",
                (unsigned)tex_state.read_data,
                (unsigned long)(sim_time / 2),
                (unsigned)top->rootp->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__readOutstanding,
                (unsigned)top->rootp->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__readBeatsLeft,
                (unsigned)top->rootp->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__rspPending);
        tex_trace_count++;
    }
}

static void capture_memory_outputs(void) {
    if (fb_write_state.read_valid) {
        fb_write_state.read_valid = 0;
    } else if (fb_write_state.read_delay) {
        fb_write_state.read_delay--;
        if (fb_write_state.read_delay == 0) fb_write_state.read_valid = 1;
    }
    if (fb_color_state.read_valid) {
        fb_color_state.read_valid = 0;
    } else if (fb_color_state.read_delay) {
        fb_color_state.read_delay--;
        if (fb_color_state.read_delay == 0) fb_color_state.read_valid = 1;
    }
    if (fb_aux_state.read_valid) {
        fb_aux_state.read_valid = 0;
    } else if (fb_aux_state.read_delay) {
        fb_aux_state.read_delay--;
        if (fb_aux_state.read_delay == 0) fb_aux_state.read_valid = 1;
    }
    if (top->io_memFbWrite_write) {
        const uint32_t idx = fb_index_from_addr(top->io_memFbWrite_address);
        fb_mem[idx] = apply_byteenable(fb_mem[idx], top->io_memFbWrite_writeData,
                                       top->io_memFbWrite_byteEnable);
    }
    if (top->io_memFbWrite_read) {
        const uint32_t idx = fb_index_from_addr(top->io_memFbWrite_address);
        fb_write_state.read_data = fb_mem[idx];
        fb_write_state.read_delay = effective_mem_read_delay();
        fb_write_state.read_valid = 0;
    }
    if (top->io_memFbColorRead_read) {
        const uint32_t idx = fb_index_from_addr(top->io_memFbColorRead_address);
        fb_color_state.read_data = fb_mem[idx];
        fb_color_state.read_delay = effective_mem_read_delay();
        fb_color_state.read_valid = 0;
    }
    if (top->io_memFbAuxRead_read) {
        const uint32_t idx = fb_index_from_addr(top->io_memFbAuxRead_address);
        fb_aux_state.read_data = fb_mem[idx];
        fb_aux_state.read_delay = effective_mem_read_delay();
        fb_aux_state.read_valid = 0;
    }

    if (tex_state.read_valid) {
        tex_state.read_valid = 0;
    } else if (tex_state.read_delay) {
        tex_state.read_delay--;
        if (tex_state.read_delay == 0) tex_state.read_valid = 1;
    }
    if (top->io_memTex_write) {
        const uint32_t addr = top->io_memTex_address;
        if (tex_trace_enabled_now(addr) && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX write addr=0x%08x data=0x%08x be=0x%x cycle=%lu\n",
                    addr, (unsigned)top->io_memTex_writeData,
                    (unsigned)top->io_memTex_byteEnable,
                    (unsigned long)(sim_time / 2));
            tex_trace_count++;
        }
        if (addr >= TEX_BASE_BYTES) {
            const uint32_t idx = tex_index_from_addr(addr);
            tex_mem[idx] = apply_byteenable(tex_mem[idx], top->io_memTex_writeData,
                                            top->io_memTex_byteEnable);
        }
    }
    if (top->io_memTex_read) {
        const uint32_t addr = top->io_memTex_address;
        if (tex_trace_enabled_now(addr) && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX read addr=0x%08x cycle=%lu\n",
                    addr, (unsigned long)(sim_time / 2));
            tex_trace_count++;
        }
        last_tex_read_cycle = sim_time / 2;
        last_tex_read_addr = addr;
        if (addr >= TEX_BASE_BYTES) {
            const uint32_t idx = tex_index_from_addr(addr);
            tex_state.read_data = tex_mem[idx];
        } else {
            tex_state.read_data = 0;
        }
        if (tex_trace_enabled_now(addr) && tex_trace_count < 256) {
            fprintf(stderr,
                    "[sim_harness_de10] TEX read addr=0x%08x data=0x%08x cycle=%lu\n",
                    addr, (unsigned)tex_state.read_data,
                    (unsigned long)(sim_time / 2));
            tex_trace_count++;
        }
        tex_state.read_delay = effective_mem_read_delay();
        tex_state.read_valid = 0;
        last_tex_rvalid_cycle = (sim_time / 2) + 1;
        last_tex_rvalid_data = tex_state.read_data;
    }
}

static void eval_inputs(void) {
    drive_memory_inputs();
    top->eval();
}

static void tick_one(void) {
    drive_memory_inputs();

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
            "[sim_harness_de10] %s %s stall addr=0x%06x cycle=%lu iter=%lu h2f(wait=%u rvalid=%u rdata=0x%08x cmdInFlight=%u readRspPending=%u cpuCmdReady=%u pipeBusy=%u) pci(canDrain=%u cmdBlocked=%u triV=%u triR=%u ftriV=%u ftriR=%u) mem(fbW=%u/%u 0x%08x fbCR=%u/%u 0x%08x fbAR=%u/%u 0x%08x tex=%u/%u 0x%08x) fb(accessBusy=%u inFlight=%u colorBusy=%u auxBusy=%u) tex(lastRead=0x%08x@%lu lastRvalid=0x%08x@%lu cpuBus=%u/%u rsp=%u/%u)\n",
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
            (unsigned)top->io_memFbWrite_read,
            (unsigned)top->io_memFbWrite_write,
            (unsigned)top->io_memFbWrite_address,
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

    int timeout = (int)mmio_timeout_cycles;
    uint64_t iter = 0;
    const uint64_t stall_log_period = (addr == 0x000000) ? 10000 : 100000;
    while (timeout > 0) {
        eval_inputs();
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
        timeout--;
        iter++;
    }
    if (timeout == 0 && !cycle_limit_hit) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_write(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
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

    int timeout = (int)mmio_timeout_cycles;
    uint64_t iter = 0;
    const uint64_t stall_log_period = (addr == 0x000000) ? 10000 : 100000;
    while (timeout > 0) {
        eval_inputs();
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
        timeout--;
        iter++;
    }
    if (timeout == 0 && !cycle_limit_hit) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_write_masked(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
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

    int timeout = (int)mmio_timeout_cycles;
    uint64_t iter = 0;
    const uint64_t stall_log_period = (addr == 0x000000) ? 10000 : 100000;
    while (timeout > 0) {
        eval_inputs();
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
        timeout--;
        iter++;
    }
    if (timeout == 0 && !cycle_limit_hit) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_read(0x%06x) cmd timeout after %u cycles @ cycle %lu\n",
                addr, mmio_timeout_cycles, (unsigned long)(sim_time / 2));
        cycle_limit_hit = true;
        mmio_timeout_hit = true;
    }

    mmio_end();
    if (!cycle_limit_hit) tick_one();

    timeout = (int)mmio_timeout_cycles;
    iter = 0;
    while (timeout > 0) {
        eval_inputs();
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
        timeout--;
        iter++;
    }

    const uint64_t ticks = (sim_time - t0) / 2;
    if (!cycle_limit_hit) {
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
    Verilated::commandArgs(0, (const char**)nullptr);

#ifdef VM_TRACE_FST
    const char* fst_path = getenv("SIM_FST");
    if (fst_path) {
        Verilated::traceEverOn(true);
    }
#endif

    fb_mem.assign(FB_WORD_COUNT, 0);
    tex_mem.assign(TEX_WORD_COUNT, 0);
    fb_write_state = {0, 0, 0};
    fb_color_state = {0, 0, 0};
    fb_aux_state = {0, 0, 0};
    tex_state = {0, 0};
    total_read_ticks = 0;
    total_read_count = 0;
    total_write_ticks = 0;
    total_write_count = 0;
    last_tex_read_cycle = 0;
    last_tex_read_addr = 0;
    last_tex_rvalid_cycle = 0;
    last_tex_rvalid_data = 0;
    sim_time = 0;
    cycle_limit = 0;
    cycle_limit_hit = false;
    mmio_timeout_hit = false;
    quit_requested = 0;

    top = new VCoreDe10;
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

    const char* mem_read_delay_str = getenv("SIM_DE10_MEM_READ_DELAY");
    if (mem_read_delay_str) {
        mem_read_delay = strtoul(mem_read_delay_str, nullptr, 0);
    }

    fprintf(stderr, "[sim_harness_de10] Initialized CoreDe10 Verilator model\n");
    return 0;
}

void sim_shutdown(void) {
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
    fprintf(stderr, "[sim_harness_de10] Shutdown after %lu cycles (%lu R, %lu W)\n",
            (unsigned long)(sim_time / 2),
            (unsigned long)total_read_count, (unsigned long)total_write_count);
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

    auto clear_buffer = [](VCoreDe10_FramebufferPlaneBuffer* buffer) {
        if (!buffer) return;
        buffer->slotValid_0 = 0;
        buffer->slotValid_1 = 0;
        buffer->slotDraining_0 = 0;
        buffer->slotDraining_1 = 0;
        buffer->slotStartAddr_0 = 0;
        buffer->slotStartAddr_1 = 0;
        buffer->slotWordCount_0 = 0;
        buffer->slotWordCount_1 = 0;
        buffer->activeSlot = 0;
        buffer->directState_1 = 0;
        buffer->drainState_1 = 0;
        buffer->drainSlot = 0;
        buffer->drainIndex = 0;
        buffer->drainLastIndex = 0;
        buffer->drainLength = 3;
        buffer->activeNextAddr = 0;

    };

    fprintf(stderr,
            "[sim_harness_de10] Invalidated host framebuffer image\n");
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

int sim_stalled(void) {
    return mmio_timeout_hit ? 1 : 0;
}
