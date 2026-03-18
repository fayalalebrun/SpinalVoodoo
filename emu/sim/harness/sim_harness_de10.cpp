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

static VCoreDe10* top = nullptr;
#ifdef VM_TRACE_FST
static VerilatedFstC* tfp = nullptr;
#endif
static uint64_t sim_time = 0;
static uint64_t cycle_limit = 0;
static volatile sig_atomic_t quit_requested = 0;

static constexpr uint32_t FB_WORD_COUNT = (4u * 1024u * 1024u) / 4u;
static constexpr uint32_t TEX_WORD_COUNT = (8u * 1024u * 1024u) / 4u;
static constexpr uint32_t TEX_BASE_BYTES = 4u * 1024u * 1024u;

static std::vector<uint32_t> fb_mem;
static std::vector<uint32_t> tex_mem;

struct AvalonMemState {
    uint8_t read_valid_cycles;
    uint32_t read_data;
};

static AvalonMemState fb_state = {0, 0};
static AvalonMemState tex_state = {0, 0};

static uint64_t total_read_ticks = 0;
static uint64_t total_read_count = 0;
static uint64_t total_write_ticks = 0;
static uint64_t total_write_count = 0;
static uint64_t last_tex_read_cycle = 0;
static uint32_t last_tex_read_addr = 0;
static uint64_t last_tex_rvalid_cycle = 0;
static uint32_t last_tex_rvalid_data = 0;
static int mmio_debug = 0;
static int tex_trace = 0;
static uint32_t tex_trace_count = 0;
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
    top->io_memFb_waitRequestn = 1;
    top->io_memFb_readDataValid = fb_state.read_valid_cycles ? 1 : 0;
    top->io_memFb_readData = fb_state.read_data;

    top->io_memTex_waitRequestn = 1;
    top->io_memTex_readDataValid = tex_state.read_valid_cycles ? 1 : 0;
    top->io_memTex_readData = tex_state.read_data;
}

static void capture_memory_outputs(void) {
    if (fb_state.read_valid_cycles) fb_state.read_valid_cycles--;
    if (top->io_memFb_write) {
        const uint32_t idx = fb_index_from_addr(top->io_memFb_address);
        fb_mem[idx] = apply_byteenable(fb_mem[idx], top->io_memFb_writeData,
                                       top->io_memFb_byteEnable);
    }
    if (top->io_memFb_read) {
        const uint32_t idx = fb_index_from_addr(top->io_memFb_address);
        fb_state.read_data = fb_mem[idx];
        fb_state.read_valid_cycles = 64;
    }

    if (tex_state.read_valid_cycles) tex_state.read_valid_cycles--;
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
        tex_state.read_valid_cycles = 64;
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

    if (cycle_limit && (sim_time / 2) >= cycle_limit) {
        fprintf(stderr, "[sim_harness_de10] Cycle limit reached (%lu ticks), shutting down.\n",
                (unsigned long)cycle_limit);
        sim_shutdown();
        _exit(1);
    }

    if (quit_requested) {
        fprintf(stderr, "[sim_harness_de10] Signal %d, shutting down (sim_time=%lu)...\n",
                (int)quit_requested, (unsigned long)sim_time);
        sim_shutdown();
        _exit(128 + quit_requested);
    }
}

static void mmio_begin(uint32_t addr, bool is_write, uint32_t data) {
    top->io_h2fLw_address = (addr >> 2) & 0x3FFFFFu;
    top->io_h2fLw_byteenable = 0xFu;
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
            "[sim_harness_de10] %s %s stall addr=0x%06x cycle=%lu iter=%lu h2f(wait=%u rvalid=%u rdata=0x%08x cmdInFlight=%u readRspPending=%u readDataPending=%u readDataValid=%u cpuCmdReady=%u fifoFree=%u queuedWriteReady=%u pipeBusy=%u swapWait=%u busyDebug=0x%08x) fb(r=%u w=%u addr=0x%08x rv=%u rdata=0x%08x bridge(readOutstanding=%u rspPending=%u rspValid=%u rspData=0x%08x) arbRsp=%u/%u) lfb(state=%u rspPending=%u read=%u fbCmdPending=%u fbCmd=%u/%u fbRsp=%u/%u cap1=0x%08x cap2=0x%08x) tex(r=%u w=%u addr=0x%08x rv=%u rdata=0x%08x bridge(readOutstanding=%u rspPending=%u rspData=0x%08x lastRead=0x%08x@%lu lastRvalid=0x%08x@%lu) cpuTex(cmd=%u/%u rsp=%u/%u) arbIn2(rsp=%u cmdReady=%u))\n",
            kind, phase, addr, (unsigned long)(sim_time / 2),
            (unsigned long)iter,
            (unsigned)top->io_h2fLw_waitrequest,
            (unsigned)top->io_h2fLw_readdatavalid,
            (unsigned)top->io_h2fLw_readdata,
            (unsigned)root->CoreDe10__DOT__cmdInFlight,
            (unsigned)root->CoreDe10__DOT__readRspPending,
            (unsigned)root->CoreDe10__DOT__readDataPending,
            (unsigned)root->CoreDe10__DOT__readDataValid,
            (unsigned)root->CoreDe10__DOT__core_1_io_cpuBus_cmd_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pciFifo_1_io_pciFifoFree,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pciFifo_1__DOT__queuedWriteTx_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__pipelineBusySignal,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__swapBuffer_1_io_waiting,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__busyDebugSignal,
            (unsigned)top->io_memFb_read,
            (unsigned)top->io_memFb_write,
            (unsigned)top->io_memFb_address,
            (unsigned)top->io_memFb_readDataValid,
            (unsigned)top->io_memFb_readData,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbBridge__DOT__readOutstanding,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbBridge__DOT__rspPending,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbBridge_io_bmb_rsp_valid,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__fbBridge__DOT__rspData,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__fbArbiter_io_inputs_4_rsp_s2mPipe_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__fbArbiter_io_inputs_4_rsp_s2mPipe_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__state,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__rspPending,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__capturedIsRead,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__fbReadCmdPending,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__io_fbReadBus_cmd_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__io_fbReadBus_cmd_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__io_fbReadBus_rsp_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__io_fbReadBus_rsp_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__capturedRead1,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__lfb_1__DOT__capturedRead2,
            (unsigned)top->io_memTex_read,
            (unsigned)top->io_memTex_write,
            (unsigned)top->io_memTex_address,
            (unsigned)top->io_memTex_readDataValid,
            (unsigned)top->io_memTex_readData,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__readOutstanding,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__rspPending,
            (unsigned)root->CoreDe10__DOT__memBackend__DOT__texBridge__DOT__rspData,
            (unsigned)last_tex_read_addr,
            (unsigned long)last_tex_read_cycle,
            (unsigned)last_tex_rvalid_data,
            (unsigned long)last_tex_rvalid_cycle,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__cpuTexBus_cmd_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__cpuTexBus_cmd_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__cpuTexBus_rsp_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__cpuTexBus_rsp_ready,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__texArbiter_io_inputs_2_rsp_valid,
            (unsigned)root->CoreDe10__DOT__core_1__DOT__texArbiter_io_inputs_2_cmd_ready);
    fflush(stderr);
}

static void bus_write(uint32_t addr, uint32_t data) {
    const uint64_t t0 = sim_time;
    mmio_begin(addr, true, data);
    maybe_log_mmio("WRITE", "begin", addr, data, 0);

    int timeout = 5000000;
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
        if (iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("WRITE", "cmd", addr, iter);
        }
        tick_one();
        timeout--;
        iter++;
    }
    if (timeout == 0) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_write(0x%06x) cmd timeout after 5M cycles @ cycle %lu\n",
                addr, (unsigned long)(sim_time / 2));
    }

    mmio_end();
    tick_one();
    maybe_log_mmio("WRITE", "done", addr, data, iter);
    total_write_ticks += (sim_time - t0) / 2;
    total_write_count++;
}

static uint32_t bus_read(uint32_t addr) {
    const uint64_t t0 = sim_time;
    mmio_begin(addr, false, 0);
    maybe_log_mmio("READ", "begin", addr, 0, 0);

    int timeout = 1000000;
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
        if (iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("READ", "cmd", addr, iter);
        }
        tick_one();
        timeout--;
        iter++;
    }
    if (timeout == 0) {
        fprintf(stderr, "[sim_harness_de10] ERROR: bus_read(0x%06x) cmd timeout after 5M cycles @ cycle %lu\n",
                addr, (unsigned long)(sim_time / 2));
    }

    mmio_end();
    tick_one();

    timeout = 5000000;
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
        if (iter > 0 && (iter % stall_log_period) == 0) {
            log_stalled_bus_state("READ", "rsp", addr, iter);
        }
        tick_one();
        timeout--;
        iter++;
    }

    const uint64_t ticks = (sim_time - t0) / 2;
    fprintf(stderr, "[sim_harness_de10] ERROR: bus_read(0x%06x) rsp timeout after 5M cycles @ cycle %lu\n",
            addr, (unsigned long)(sim_time / 2));
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
    fb_state = {0, 0};
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

    const char* mmio_debug_str = getenv("SIM_DE10_MMIO_DEBUG");
    if (mmio_debug_str) {
        mmio_debug = atoi(mmio_debug_str);
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
    bus_write(addr, data);
}

uint32_t sim_read(uint32_t addr) {
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
    (void)count;
}
