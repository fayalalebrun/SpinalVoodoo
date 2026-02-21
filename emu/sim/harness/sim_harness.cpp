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
#include "verilated_fst_c.h"

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static VCoreSim *top = nullptr;
static VerilatedFstC *tfp = nullptr;
static uint64_t sim_time = 0;

/* Vsync generation: toggle vRetrace every VSYNC_HALF_PERIOD ticks */
#define VSYNC_PERIOD     5000
#define VSYNC_HIGH_TICKS 200
static uint32_t vsync_counter = 0;

/* fbWrite logging: log addresses written to framebuffer */
static FILE *fbwrite_log = nullptr;
static int fbwrite_phase = 0; /* 0=clear, 1=idle_after_clear, 2=text_render */

/* TMU logging: log texture lookup details for cutoff analysis */
static FILE *tmu_log = nullptr;

/* Signal handler to flush FST on termination */
static void signal_handler(int sig) {
    fprintf(stderr, "[sim_harness] Signal %d, flushing trace (sim_time=%lu)...\n",
            sig, (unsigned long)sim_time);
    if (tfp) {
        tfp->flush();
        tfp->close();
        tfp = nullptr;
    }
    _exit(128 + sig);
}

/* ------------------------------------------------------------------ */
/* Clock / eval helpers                                                */
/* ------------------------------------------------------------------ */

static void tick_one(void) {
    /* Rising edge */
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time);
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
        /* Detect phase transitions based on pipeline status */
        if (fbwrite_phase == 0 && top->io_fifoEmpty && !top->io_pipelineBusy) {
            fbwrite_phase = 1;
            fprintf(fbwrite_log, "# PHASE 1: idle after clear at tick %lu\n", (unsigned long)(sim_time/2));
        }
        if (fbwrite_phase == 1 && (!top->io_fifoEmpty || top->io_pipelineBusy)) {
            fbwrite_phase = 2;
            fprintf(fbwrite_log, "# PHASE 2: text rendering at tick %lu\n", (unsigned long)(sim_time/2));
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

    /* Falling edge */
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    /* Vsync generation */
    vsync_counter++;
    if (vsync_counter >= VSYNC_PERIOD) {
        vsync_counter = 0;
    }
    top->io_vRetrace = (vsync_counter < VSYNC_HIGH_TICKS) ? 1 : 0;

    /* Periodic FST flush (every 500K sim_time units = 250K ticks) */
    if (tfp && (sim_time % 500000) == 0) {
        tfp->flush();
    }
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
        fprintf(stderr, "[sim_harness] %lu ops (%lu R avg %.1f tck, %lu W avg %.1f tck) sim_time=%lu\n",
                (unsigned long)total_ops,
                (unsigned long)total_read_count,
                total_read_count > 0 ? (double)total_read_ticks / total_read_count : 0.0,
                (unsigned long)total_write_count,
                total_write_count > 0 ? (double)total_write_ticks / total_write_count : 0.0,
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

    /* Tick until cmd accepted: check ready BEFORE each rising edge */
    int timeout = 10000;
    while (timeout > 0) {
        top->eval();  /* Settle combinational logic with current inputs */
        if (top->io_cpuBus_cmd_ready) {
            tick_one();  /* This rising edge fires the cmd */
            break;
        }
        tick_one();
        timeout--;
    }

    /* Deassert cmd */
    top->io_cpuBus_cmd_valid = 0;

    /* Wait for response: check rsp_valid BEFORE each rising edge */
    timeout = 10000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_rsp_valid) {
            tick_one();  /* Consume response */
            break;
        }
        tick_one();
        timeout--;
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
    int timeout = 10000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_cmd_ready) {
            tick_one();
            break;
        }
        tick_one();
        timeout--;
    }

    uint64_t t_cmd = sim_time;

    /* Deassert cmd */
    top->io_cpuBus_cmd_valid = 0;

    /* Wait for response: check rsp_valid BEFORE each rising edge */
    timeout = 10000;
    while (timeout > 0) {
        top->eval();
        if (top->io_cpuBus_rsp_valid) {
            uint32_t data = top->io_cpuBus_rsp_payload_fragment_data;
            tick_one();  /* Consume response */

            uint64_t t_rsp = sim_time;
            uint64_t ticks = (sim_time - t0) / 2;
            uint64_t cmd_ticks = (t_cmd - t0) / 2;
            uint64_t rsp_ticks = (t_rsp - t_cmd) / 2;

            if (total_read_count < 20 || ticks > 10) {
                fprintf(stderr, "[bus_read] #%lu addr=0x%06x data=0x%08x ticks=%lu (cmd=%lu rsp=%lu)\n",
                        (unsigned long)total_read_count, addr & 0xFFFFFF, data,
                        (unsigned long)ticks, (unsigned long)cmd_ticks, (unsigned long)rsp_ticks);
            }

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

    /* Must call traceEverOn before model construction if tracing */
    const char *fst_path = getenv("SIM_FST");
    if (fst_path) {
        Verilated::traceEverOn(true);
    }

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

    /* Enable FST tracing after reset */
    if (fst_path) {
        tfp = new VerilatedFstC;
        top->trace(tfp, 99);
        tfp->open(fst_path);
        fprintf(stderr, "[sim_harness] FST tracing to %s\n", fst_path);
    }

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
    if (tfp) {
        tfp->close();
        delete tfp;
        tfp = nullptr;
    }
    if (top) {
        top->final();
        delete top;
        top = nullptr;
    }
    fprintf(stderr, "[sim_harness] Shutdown complete (%lu ticks)\n",
            (unsigned long)sim_time);
    if (total_read_count > 0) {
        fprintf(stderr, "[sim_harness] Reads: %lu ops, avg %.1f ticks/op\n",
                (unsigned long)total_read_count,
                (double)total_read_ticks / total_read_count);
    }
    if (total_write_count > 0) {
        fprintf(stderr, "[sim_harness] Writes: %lu ops, avg %.1f ticks/op\n",
                (unsigned long)total_write_count,
                (double)total_write_ticks / total_write_count);
    }
}

void sim_write(uint32_t addr, uint32_t data) {
    bus_write(addr, data);
}

uint32_t sim_read(uint32_t addr) {
    return bus_read(addr);
}


uint32_t sim_idle_wait(void) {
    /* Spin until FIFO empty and pipeline not busy */
    int timeout = 10000000;  /* 10M ticks — generous for slow pipelines */
    uint64_t t0 = sim_time;
    int last_report = 0;
    while (timeout > 0) {
        if (top->io_fifoEmpty && !top->io_pipelineBusy) {
            fprintf(stderr, "[sim_harness] idle_wait: settled in %lu ticks\n",
                    (unsigned long)((sim_time - t0) / 2));
            break;
        }
        /* Periodic progress report */
        int elapsed = (int)((sim_time - t0) / 2);
        if (elapsed / 1000000 > last_report) {
            last_report = elapsed / 1000000;
            fprintf(stderr, "[sim_harness] idle_wait: %dM ticks, fifoEmpty=%d pipelineBusy=%d\n",
                    last_report, (int)top->io_fifoEmpty, (int)top->io_pipelineBusy);
        }
        /* Tick in batches for efficiency */
        for (int i = 0; i < 100 && timeout > 0; i++, timeout--) {
            tick_one();
        }
    }

    if (timeout <= 0) {
        fprintf(stderr, "[sim_harness] WARNING: idle_wait timeout after %lu ticks! fifoEmpty=%d pipelineBusy=%d\n",
                (unsigned long)((sim_time - t0) / 2),
                (int)top->io_fifoEmpty, (int)top->io_pipelineBusy);
    }

    /* Read status register and return it */
    return bus_read(0x000000);
}

void sim_tick(int n) {
    for (int i = 0; i < n; i++) {
        tick_one();
    }
}
