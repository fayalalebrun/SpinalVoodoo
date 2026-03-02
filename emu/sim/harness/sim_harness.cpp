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

/* Promote Verilator warnings to fatal errors.
 * verilated.cpp is compiled with -DVL_USER_WARN so its default vl_warn is
 * excluded; we provide this replacement.  This catches $readmemb file-not-found
 * (and any future warning) at model construction time instead of silently
 * zeroing memories. */
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

/* Pixel pipeline trace: env-var-gated per-pixel debug logging.
 * Set SIM_WATCH_X and SIM_WATCH_Y to trace a pixel through every stage.
 * Coords are post-yOriginSwap (screen space). */
static int watch_x = -1;
static int watch_y = -1;

/* Helper: sign-extend 12-bit SData coord to int16_t */
static inline int16_t sext12(uint16_t v) {
    return (int16_t)(v << 4) >> 4;
}

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
        uint8_t valid = r->CoreSim__DOT__core_1__DOT__write_1_o_fbWrite_cmd_valid;
        if (valid) {
            uint32_t addr = r->CoreSim__DOT__core_1__DOT__write_1_o_fbWrite_cmd_payload_fragment_address;
            uint32_t data = r->CoreSim__DOT__core_1__DOT__write_1_o_fbWrite_cmd_payload_fragment_data;
            /* Decode pixel coords: addr = (y*stride + x)*4, stride from fbiInit1[7:4]*128 */
            uint32_t pixel = addr >> 2;
            /* Approximate decode for logging (stride may vary) */
            uint32_t y = pixel / 640;
            uint32_t x = pixel % 640;
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

    /* Pixel pipeline trace: log every pipeline stage for the watched pixel.
     * Gated by watch_x/watch_y (set from SIM_WATCH_X/Y env vars). */
    if (watch_x >= 0) {
        auto r = top->rootp;

        /* --- TMU input (rasterizer fork to TMU: S/T/W before lookup) --- */
        if (r->CoreSim__DOT__core_1__DOT__rasterFork_0_valid &&
            r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__rasterFork_0_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__rasterFork_0_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                int32_t s = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_s;
                int32_t t = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_t;
                int32_t w = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_w;
                int32_t texS = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__texS;
                int32_t texT = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__texT;
                uint8_t lod = r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__lodLevel;
                uint32_t pointAddr = r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__pointAddr;
                fprintf(stderr, "[PIXEL %d,%d] TMU_IN: s=0x%08x t=0x%08x w=0x%08x texS=%d texT=%d lod=%d pointAddr=0x%06x\n",
                        px, py, (uint32_t)s, (uint32_t)t, (uint32_t)w, texS, texT, lod, pointAddr);
            }
        }

        /* --- TMU join (texture result + rasterizer coords reunited) --- */
        if (r->CoreSim__DOT__core_1__DOT__tmuJoined_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__tmuJoined_payload_2_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__tmuJoined_payload_2_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] TMU: tex=(%d,%d,%d) alpha=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__tmu_1_io_output_payload_texture_r,
                        r->CoreSim__DOT__core_1__DOT__tmu_1_io_output_payload_texture_g,
                        r->CoreSim__DOT__core_1__DOT__tmu_1_io_output_payload_texture_b,
                        r->CoreSim__DOT__core_1__DOT__tmu_1_io_output_payload_textureAlpha);
            }
        }

        /* --- ColorCombine output --- */
        if (r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_valid &&
            r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] CC:  rgb=(%d,%d,%d) alpha=%d ccMode=rgbSel%d,msel%d,texEn%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_color_r,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_color_g,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_color_b,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1_io_output_payload_alpha,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1__DOT__io_input_payload_config_rgbSel,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1__DOT__io_input_payload_config_mselect,
                        r->CoreSim__DOT__core_1__DOT__colorCombine_1__DOT__io_input_payload_config_textureEnable);
            }
        }

        /* --- Fog output --- */
        if (r->CoreSim__DOT__core_1__DOT__fog_1_io_output_valid &&
            r->CoreSim__DOT__core_1__DOT__fog_1_io_output_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] FOG: rgb=(%d,%d,%d) alpha=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_color_r,
                        r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_color_g,
                        r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_color_b,
                        r->CoreSim__DOT__core_1__DOT__fog_1_io_output_payload_alpha);
            }
        }

        /* --- FramebufferAccess output (after depth test + alpha blend) --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                uint32_t existingPixel = r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_1;
                fprintf(stderr, "[PIXEL %d,%d] FBA: rgb=(%d,%d,%d) alpha=%d blended=(%d,%d,%d) existing=0x%08x blend=%d srcF=%d dstF=%d depthF=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_b,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_alpha,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_b,
                        existingPixel,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_alphaBlendEnable,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_rgbSrcFact,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_rgbDstFact,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_depthFunction);
            }
        }

        /* --- Pre-dither (RGB888 entering dither) --- */
        if (r->CoreSim__DOT__core_1__DOT__ditherJoined_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] DIT: rgb888=(%d,%d,%d)\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_r,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_g,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_b);
            }
        }

        /* --- Write stage (final RGB565 + FB address) --- */
        if (r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] WR:  rgb565=(%d,%d,%d) depth=0x%04x\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_toFb_color_r,
                        r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_toFb_color_g,
                        r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_toFb_color_b,
                        r->CoreSim__DOT__core_1__DOT__write_1__DOT__i_fromPipeline_payload_toFb_depthAlpha);
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

    /* Pixel pipeline trace: SIM_WATCH_X + SIM_WATCH_Y */
    const char *wx_str = getenv("SIM_WATCH_X");
    const char *wy_str = getenv("SIM_WATCH_Y");
    if (wx_str && wy_str) {
        watch_x = atoi(wx_str);
        watch_y = atoi(wy_str);
        fprintf(stderr, "[sim_harness] Pixel trace enabled for (%d, %d)\n",
                watch_x, watch_y);
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

/* ------------------------------------------------------------------ */
/* Bulk direct RAM access (bypasses bus protocol)                       */
/* ------------------------------------------------------------------ */

/* fbRam: 4MB = 1048576 words, stored in 4 byte-lane arrays (ram_symbol0..3).
 * Each ram_symbolN[i] holds byte N of 32-bit word i. */
#define FB_WORD_COUNT   (4 * 1024 * 1024 / 4)
#define TEX_WORD_COUNT  (8 * 1024 * 1024 / 4)

void sim_read_fb(uint32_t byte_offset, uint32_t *dst, uint32_t word_count) {
    auto r = top->rootp;
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < FB_WORD_COUNT; i++) {
        dst[i] = (uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol0[idx + i]
               | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol1[idx + i] << 8)
               | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol2[idx + i] << 16)
               | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol3[idx + i] << 24);
    }
}

void sim_write_fb_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count) {
    auto r = top->rootp;
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < FB_WORD_COUNT; i++) {
        r->CoreSim__DOT__fbRam__DOT__ram_symbol0[idx + i] = src[i] & 0xff;
        r->CoreSim__DOT__fbRam__DOT__ram_symbol1[idx + i] = (src[i] >> 8) & 0xff;
        r->CoreSim__DOT__fbRam__DOT__ram_symbol2[idx + i] = (src[i] >> 16) & 0xff;
        r->CoreSim__DOT__fbRam__DOT__ram_symbol3[idx + i] = (src[i] >> 24) & 0xff;
    }
}

void sim_write_tex_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count) {
    auto r = top->rootp;
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < TEX_WORD_COUNT; i++) {
        r->CoreSim__DOT__texRam__DOT__ram_symbol0[idx + i] = src[i] & 0xff;
        r->CoreSim__DOT__texRam__DOT__ram_symbol1[idx + i] = (src[i] >> 8) & 0xff;
        r->CoreSim__DOT__texRam__DOT__ram_symbol2[idx + i] = (src[i] >> 16) & 0xff;
        r->CoreSim__DOT__texRam__DOT__ram_symbol3[idx + i] = (src[i] >> 24) & 0xff;
    }
}

void sim_read_tex(uint32_t byte_offset, uint32_t *dst, uint32_t word_count) {
    auto r = top->rootp;
    uint32_t idx = byte_offset / 4;
    for (uint32_t i = 0; i < word_count && (idx + i) < TEX_WORD_COUNT; i++) {
        dst[i] = (uint32_t)r->CoreSim__DOT__texRam__DOT__ram_symbol0[idx + i]
               | ((uint32_t)r->CoreSim__DOT__texRam__DOT__ram_symbol1[idx + i] << 8)
               | ((uint32_t)r->CoreSim__DOT__texRam__DOT__ram_symbol2[idx + i] << 16)
               | ((uint32_t)r->CoreSim__DOT__texRam__DOT__ram_symbol3[idx + i] << 24);
    }
}

void sim_set_swap_count(uint32_t count) {
    top->rootp->CoreSim__DOT__core_1__DOT__swapBuffer_1__DOT__swapCountReg = count & 0x3;
}
