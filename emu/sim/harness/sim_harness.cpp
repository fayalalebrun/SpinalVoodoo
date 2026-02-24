/*
 * sim_harness.cpp — Verilator driver for CoreSim.
 *
 * Provides the C API defined in sim_harness.h. Manages clock toggling,
 * BMB bus transactions, vsync generation, and idle-wait polling.
 */

#include "sim_harness.h"
#include "VCoreSim.h"
#include "VCoreSim___024root.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>

/* Verilator boilerplate */
#include "verilated.h"
#ifdef VM_TRACE_FST
#include "verilated_fst_c.h"
#endif

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static VCoreSim *top = nullptr;
#ifdef VM_TRACE_FST
static VerilatedFstC *tfp = nullptr;
#endif
static uint64_t sim_time = 0;
static uint64_t cycle_limit = 0;  /* 0 = no limit; set via SIM_CYCLE_LIMIT */

/* Vsync generation: toggle vRetrace every VSYNC_HALF_PERIOD ticks */
#define VSYNC_PERIOD     5000
#define VSYNC_HIGH_TICKS 200
static uint32_t vsync_counter = 0;

/* fbWrite logging: log addresses written to framebuffer */
static FILE *fbwrite_log = nullptr;
static int fbwrite_phase = 0; /* 0=clear, 1=idle_after_clear, 2=text_render */

/* TMU logging: log texture lookup details for cutoff analysis */
static FILE *tmu_log = nullptr;

/* Signal flag — set asynchronously, polled in tick_one() */
static volatile sig_atomic_t quit_requested = 0;

static void signal_handler(int sig) {
    quit_requested = sig;  /* async-signal-safe */
}

/* ------------------------------------------------------------------ */
/* Clock / eval helpers                                                */
/* ------------------------------------------------------------------ */

static void tick_one(void) {
    /* Rising edge */
    top->clk = 1;
    top->eval();
#ifdef VM_TRACE_FST
    if (tfp) tfp->dump(sim_time);
#endif
    sim_time++;

    /* fbWrite logging: capture on rising edge */
    if (fbwrite_log) {
        auto r = top->rootp;
        uint8_t valid = r->CoreSim__DOT__core_1_io_fbWrite_cmd_valid;
        if (valid) {
            uint32_t addr = r->CoreSim__DOT__core_1_io_fbWrite_cmd_payload_fragment_address;
            uint32_t data = r->CoreSim__DOT__core_1__DOT__io_fbWrite_cmd_payload_fragment_data;
            /* Decode pixel coords: addr = (y*1024 + x)*4 */
            uint32_t pixel = addr >> 2;
            uint32_t y = pixel >> 10;
            uint32_t x = pixel & 0x3FF;
            /* Log text area pixels (y=0-19, x=0-220) during text rendering */
            if (fbwrite_phase == 2 || (fbwrite_phase == 0 && y < 20 && x < 220)) {
                fprintf(fbwrite_log, "%lu %u %u 0x%08x\n",
                        (unsigned long)(sim_time/2), x, y, data);
            }
        }
    }

    /* TMU pipeline logging: capture rasterizer output for cutoff analysis */
    if (tmu_log) {
        auto r = top->rootp;
        /* Log rasterizer output - only for cutoff rows (fb y=158-166, screen y=8-16) */
        uint8_t rast_valid = r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_valid;
        if (rast_valid) {
            int16_t rx = (int16_t)(r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_payload_coords_0 << 4) >> 4;
            int16_t ry = (int16_t)(r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_payload_coords_1 << 4) >> 4;
            if (ry >= 8 && ry <= 16) {
                int32_t sGrad = (int32_t)r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_payload_grads_sGrad;
                int32_t tGrad = (int32_t)r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_payload_grads_tGrad;
                int32_t wGrad = (int32_t)r->CoreSim__DOT__core_1__DOT__rasterizer_1_o_payload_grads_wGrad;
                fprintf(tmu_log, "R %lu %d %d s=%d t=%d w=%d\n",
                        (unsigned long)(sim_time/2), rx, ry, sGrad, tGrad, wGrad);
            }
        }
    }

    /* Check cycle limit (sim_time/2 = tick count) */
    if (cycle_limit && (sim_time / 2) >= cycle_limit) {
        fprintf(stderr, "[sim_harness] Cycle limit reached (%lu ticks), shutting down.\n",
                (unsigned long)cycle_limit);
        sim_shutdown();
        _exit(1);
    }

    /* Falling edge */
    top->clk = 0;
    top->eval();
#ifdef VM_TRACE_FST
    if (tfp) tfp->dump(sim_time);
#endif
    sim_time++;

    /* Check for signal (Ctrl-C / SIGTERM) — safe to clean up here */
    if (quit_requested) {
        fprintf(stderr, "[sim_harness] Signal %d, shutting down (sim_time=%lu)...\n",
                (int)quit_requested, (unsigned long)sim_time);
        sim_shutdown();
        _exit(128 + quit_requested);
    }

    /* Vsync generation */
    vsync_counter++;
    if (vsync_counter >= VSYNC_PERIOD) {
        vsync_counter = 0;
    }
    top->io_vRetrace = (vsync_counter < VSYNC_HIGH_TICKS) ? 1 : 0;

#ifdef VM_TRACE_FST
    /* Periodic FST flush (every 500K sim_time units = 250K ticks) */
    if (tfp && (sim_time % 500000) == 0) {
        tfp->flush();
    }
#endif
}

/* ------------------------------------------------------------------ */
/* BMB transaction helpers                                             */
/* ------------------------------------------------------------------ */

/* BMB opcodes matching SpinalHDL Bmb.Cmd.Opcode */
#define BMB_OPCODE_READ  0
#define BMB_OPCODE_WRITE 1

/* Bus transaction stats */
static uint64_t total_read_ticks = 0;
static uint64_t total_read_count = 0;
static uint64_t total_write_ticks = 0;
static uint64_t total_write_count = 0;
#define STATS_INTERVAL 10000

static void print_periodic_stats(void) {
    uint64_t total_ops = total_read_count + total_write_count;
    if (total_ops > 0 && (total_ops % STATS_INTERVAL) == 0) {
        fprintf(stderr, "[sim_harness] %lu ops (%lu R, %lu W) cycle=%lu\n",
                (unsigned long)total_ops,
                (unsigned long)total_read_count,
                (unsigned long)total_write_count,
                (unsigned long)(sim_time / 2));
    }
}

/* Drive a write command on the unified CPU bus, tick until accepted.
 *
 * IMPORTANT: We check cmd_ready BEFORE the rising edge by calling eval()
 * to settle combinational logic, then tick_one() to apply the clock edge.
 * This is necessary because some slaves (e.g., Lfb) deassert cmd_ready
 * on the same posedge that fires the cmd (state changes immediately).
 * Checking cmd_ready AFTER tick_one() would miss the acceptance. */
static void bus_write(uint32_t addr, uint32_t data) {
    uint64_t t0 = sim_time;
    top->io_cpuBus_cmd_valid = 1;
    top->io_cpuBus_cmd_payload_fragment_opcode = BMB_OPCODE_WRITE;
    top->io_cpuBus_cmd_payload_fragment_address = addr & 0xFFFFFF;
    top->io_cpuBus_cmd_payload_fragment_data = data;
    top->io_cpuBus_cmd_payload_fragment_mask = 0xF;
    top->io_cpuBus_cmd_payload_fragment_length = 3;  /* 4 bytes - 1 */
    top->io_cpuBus_cmd_payload_last = 1;
    top->io_cpuBus_cmd_payload_fragment_source = 0;

    /* Always accept responses */
    top->io_cpuBus_rsp_ready = 1;

    /* Tick until cmd accepted: check ready BEFORE each rising edge.
     * Timeout must be large enough to handle FIFO backpressure — the PCI
     * FIFO can take 500K+ cycles to drain when filled with triangle data. */
    int timeout = 5000000;
    while (timeout > 0) {
        top->eval();  /* Settle combinational logic with current inputs */
        if (top->io_cpuBus_cmd_ready) {
            tick_one();  /* This rising edge fires the cmd */
            break;
        }
        tick_one();
        timeout--;
    }
    if (timeout == 0) {
        fprintf(stderr, "[sim_harness] ERROR: bus_write(0x%06x) cmd timeout after 5M cycles @ cycle %lu\n",
                addr, (unsigned long)(sim_time / 2));
    }

    /* Deassert cmd */
    top->io_cpuBus_cmd_valid = 0;

    /* Wait for response: check rsp_valid BEFORE each rising edge */
    timeout = 5000000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_rsp_valid) {
            tick_one();  /* Consume response */
            break;
        }
        tick_one();
        timeout--;
    }
    if (timeout == 0) {
        fprintf(stderr, "[sim_harness] ERROR: bus_write(0x%06x) rsp timeout after 5M cycles @ cycle %lu\n",
                addr, (unsigned long)(sim_time / 2));
    }

    total_write_ticks += (sim_time - t0) / 2;
    total_write_count++;
    print_periodic_stats();
}

/* Drive a read command on the unified CPU bus, return data.
 * Same handshake approach as bus_write: check signals BEFORE rising edge. */
static uint32_t bus_read(uint32_t addr) {
    uint64_t t0 = sim_time;

    top->io_cpuBus_cmd_valid = 1;
    top->io_cpuBus_cmd_payload_fragment_opcode = BMB_OPCODE_READ;
    top->io_cpuBus_cmd_payload_fragment_address = addr & 0xFFFFFF;
    top->io_cpuBus_cmd_payload_fragment_data = 0;
    top->io_cpuBus_cmd_payload_fragment_mask = 0xF;
    top->io_cpuBus_cmd_payload_fragment_length = 3;
    top->io_cpuBus_cmd_payload_last = 1;
    top->io_cpuBus_cmd_payload_fragment_source = 0;

    top->io_cpuBus_rsp_ready = 1;

    /* Tick until cmd accepted: check ready BEFORE each rising edge */
    int timeout = 5000000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_cmd_ready) {
            tick_one();
            break;
        }
        tick_one();
        timeout--;
    }
    if (timeout == 0) {
        fprintf(stderr, "[sim_harness] ERROR: bus_read(0x%06x) cmd timeout after 5M cycles @ cycle %lu\n",
                addr, (unsigned long)(sim_time / 2));
    }

    /* Deassert cmd */
    top->io_cpuBus_cmd_valid = 0;

    /* Wait for response: check rsp_valid BEFORE each rising edge */
    timeout = 5000000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_rsp_valid) {
            uint32_t data = top->io_cpuBus_rsp_payload_fragment_data;
            tick_one();  /* Consume response */

            uint64_t ticks = (sim_time - t0) / 2;
            total_read_ticks += ticks;
            total_read_count++;
            print_periodic_stats();
            return data;
        }
        tick_one();
        timeout--;
    }

    /* Timeout */
    uint64_t ticks = (sim_time - t0) / 2;
    fprintf(stderr, "[bus_read] #%lu addr=0x%06x TIMEOUT ticks=%lu\n",
            (unsigned long)total_read_count, addr & 0xFFFFFF, (unsigned long)ticks);
    total_read_ticks += ticks;
    total_read_count++;
    print_periodic_stats();
    return 0xDEAD0000;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int sim_init(void) {
    /* Verilator context setup */
    Verilated::commandArgs(0, (const char **)nullptr);

#ifdef VM_TRACE_FST
    /* Must call traceEverOn before model construction if tracing */
    const char *fst_path = getenv("SIM_FST");
    if (fst_path) {
        Verilated::traceEverOn(true);
    }
#endif

    top = new VCoreSim;
    if (!top) return -1;

    /* Reset sequence */
    top->clk = 0;
    top->reset = 1;
    top->io_vRetrace = 0;
    top->io_cpuBus_cmd_valid = 0;
    top->io_cpuBus_rsp_ready = 1;
    /* Hold reset for 10 cycles */
    for (int i = 0; i < 10; i++) {
        tick_one();
    }
    top->reset = 0;

    /* Run a few more cycles to let things settle */
    for (int i = 0; i < 10; i++) {
        tick_one();
    }

#ifdef VM_TRACE_FST
    /* Enable FST tracing after reset */
    if (fst_path) {
        tfp = new VerilatedFstC;
        top->trace(tfp, 99);
        tfp->open(fst_path);
        fprintf(stderr, "[sim_harness] FST tracing to %s\n", fst_path);
    }
#endif

    /* Install signal handlers for clean FST flush on kill */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    /* Enable fbWrite logging if requested */
    const char *fblog_path = getenv("SIM_FBWRITE_LOG");
    if (fblog_path) {
        fbwrite_log = fopen(fblog_path, "w");
        if (fbwrite_log) {
            fprintf(stderr, "[sim_harness] fbWrite logging to %s\n", fblog_path);
            fprintf(fbwrite_log, "# tick x y data\n");
        }
    }

    /* Enable TMU logging if requested */
    const char *tmulog_path = getenv("SIM_TMU_LOG");
    if (tmulog_path) {
        tmu_log = fopen(tmulog_path, "w");
        if (tmu_log) {
            fprintf(stderr, "[sim_harness] TMU logging to %s\n", tmulog_path);
            fprintf(tmu_log, "# R tick rx ry sGrad tGrad  (rasterizer output)\n");
            fprintf(tmu_log, "# T tick sP tP texX texY rgb lod  (TMU output)\n");
        }
    }

    /* Optional cycle timeout */
    const char *cycle_limit_str = getenv("SIM_CYCLE_LIMIT");
    if (cycle_limit_str) {
        cycle_limit = strtoull(cycle_limit_str, nullptr, 0);
        if (cycle_limit)
            fprintf(stderr, "[sim_harness] Cycle limit set to %lu ticks\n",
                    (unsigned long)cycle_limit);
    }

    fprintf(stderr, "[sim_harness] Initialized CoreSim Verilator model\n");
    return 0;
}

void sim_shutdown(void) {
    if (tmu_log) {
        fclose(tmu_log);
        tmu_log = nullptr;
    }
    if (fbwrite_log) {
        fclose(fbwrite_log);
        fbwrite_log = nullptr;
    }
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
    fprintf(stderr, "[sim_harness] Shutdown after %lu cycles (%lu R, %lu W)\n",
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
    /* Poll status register until FIFO has space and pipeline not busy,
     * matching sst1InitIdleLoop's approach of reading through the bus.
     * Status register bits: [5:0]=pciFifoFree, [7]=fbiBusy, [9]=sstBusy */
    #define SST_BUSY       (1u << 9)
    #define SST_FIFOFREE_MASK 0x3Fu

    int timeout = 5000000;  /* 5M reads — generous for slow pipelines */
    uint64_t t0 = sim_time;
    int idle_count = 0;
    while (timeout > 0) {
        uint32_t status = bus_read(0x000000);
        int busy = (status & SST_BUSY) != 0;
        int fifo_free = status & SST_FIFOFREE_MASK;

        if (!busy && fifo_free == 0x3F) {
            if (++idle_count >= 3) {  /* Match sst1InitIdleLoop: 3 consecutive idle reads */
                return status;
            }
        } else {
            idle_count = 0;
        }

        timeout--;
    }

    uint32_t status = bus_read(0x000000);
    fprintf(stderr, "[sim_harness] WARNING: idle_wait timeout after %lu ticks! status=0x%08x\n",
            (unsigned long)((sim_time - t0) / 2), status);
    return status;
}

void sim_tick(int n) {
    for (int i = 0; i < n; i++) {
        tick_one();
    }
}
