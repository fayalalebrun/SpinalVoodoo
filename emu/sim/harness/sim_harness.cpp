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

/* Optional warning override.  Only define vl_warn when verilated.cpp is
 * built with -DVL_USER_WARN, otherwise Verilator provides its own symbol. */
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

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static VerilatedContext *contextp = nullptr;
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
static int32_t debug_fb_addr = -1;

/* fbWrite logging: log addresses written to framebuffer */
static FILE *fbwrite_log = nullptr;
static int fbwrite_phase = 0; /* 0=clear, 1=idle_after_clear, 2=text_render */

/* TMU logging: log texture lookup details for cutoff analysis */
static FILE *tmu_log = nullptr;

/* Unified bus logging for trace-vs-live comparisons. */
static FILE *bus_log = nullptr;

static void log_bus_event(char op, uint32_t addr, uint32_t data) {
    if (!bus_log) return;
    fprintf(bus_log, "%c %06x %08x\n", op, addr & 0xFFFFFF, data);
}

/* Pixel pipeline trace: env-var-gated per-pixel debug logging.
 * Set SIM_WATCH_X and SIM_WATCH_Y to trace a pixel through every stage.
 * Coords are post-yOriginSwap (screen space). */
static int watch_x = -1;
static int watch_y = -1;

/* Helper: sign-extend 12-bit SData coord to int16_t */
static inline int16_t sext12(uint16_t v) {
    return (int16_t)(v << 4) >> 4;
}

static inline int64_t sext48(uint64_t v) {
    return (int64_t)(v << 16) >> 16;
}

static const char *predither_source_name(uint8_t chosen) {
    switch (chosen) {
        case 0: return "fastfill";
        case 1: return "triangle";
        case 2: return "lfb";
        default: return "unknown";
    }
}

/* Signal flag — set asynchronously, polled in tick_one() */
static volatile sig_atomic_t quit_requested = 0;

static void dump_fb_word(uint32_t byte_addr, const char *tag) {
    auto r = top->rootp;
    uint32_t idx = byte_addr / 4;
    uint32_t word = (uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol0[idx]
                  | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol1[idx] << 8)
                  | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol2[idx] << 16)
                  | ((uint32_t)r->CoreSim__DOT__fbRam__DOT__ram_symbol3[idx] << 24);
    fprintf(stderr, "[sim_harness] %s fbRam[0x%06x] = 0x%08x\n", tag, byte_addr, word);
}

static void dump_fb_debug(const char *tag) {
    if (debug_fb_addr < 0)
        return;
    fprintf(stderr, "[sim_harness] ==== FB debug %s ====\n", tag);
    dump_fb_word((uint32_t)debug_fb_addr & ~0x3u, tag);
}

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
        uint8_t valid = r->CoreSim__DOT__core_1__DOT__writeColor_o_fbWrite_valid;
        if (valid) {
            uint32_t addr = r->CoreSim__DOT__core_1__DOT__writeColor_o_fbWrite_payload_address;
            uint32_t data = r->CoreSim__DOT__core_1__DOT__writeColor_o_fbWrite_payload_data;
            uint8_t mask = r->CoreSim__DOT__core_1__DOT__writeColor_o_fbWrite_payload_mask;
            /* Split-plane writes are 32-bit aligned, with lane selected by byte mask. */
            uint32_t planeAddr = addr + ((mask & 0xC) ? 2 : 0);
            uint32_t pixel = planeAddr >> 1;
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
                int64_t s = sext48(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_s);
                int64_t t = sext48(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_t);
                int64_t w = sext48(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_w);
                int32_t texS = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__texS;
                int32_t texT = (int32_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__texT;
                int32_t sPoint = (int16_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__sPoint;
                int32_t tPoint = (int16_t)r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__tPoint;
                uint8_t lod = r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__lodLevel;
                uint32_t pointAddr = r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__pointAddr;
                fprintf(stderr, "[PIXEL %d,%d] TMU_IN: s=0x%012llx t=0x%012llx w=0x%012llx sDec=%lld tDec=%lld wDec=%lld texS=%d texT=%d sPoint=%d tPoint=%d lod=%d pointAddr=0x%06x\n",
                        px, py,
                        (unsigned long long)(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_s & 0xffffffffffffULL),
                        (unsigned long long)(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_t & 0xffffffffffffULL),
                        (unsigned long long)(r->CoreSim__DOT__core_1__DOT__tmu_1_io_input_payload_w & 0xffffffffffffULL),
                        (long long)s, (long long)t, (long long)w,
                        texS, texT, sPoint, tPoint, lod, pointAddr);
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

        /* --- FramebufferAccess input --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_valid &&
            r->CoreSim__DOT__core_1__DOT__fbAccess_io_input_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr,
                        "[PIXEL %d,%d] FBA_IN: rgb=(%d,%d,%d) alpha=%d depth=0x%08x wDepth=0x%04x depthFn=%d zbuf=%d wbuf=%d depthSrc=%d depthBias=%d blend=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_color_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_color_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_color_b,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alpha,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_input_payload_depth,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_wDepth,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_depthFunction,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_enableDepthBuffer,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_wBufferSelect,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_depthSourceSelect,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_enableDepthBias,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_alphaBlendEnable);
            }
        }

        /* --- FramebufferAccess request fire --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_fbReadColorReq_valid &&
            r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_fbReadColorReq_ready &&
            r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_fbReadAuxReq_valid &&
            r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_fbReadAuxReq_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__request_passthrough_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__request_passthrough_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr,
                        "[PIXEL %d,%d] FBA_REQ: colorAddr=0x%06x auxAddr=0x%06x newDepth=0x%04x q=%u readJoin=%d fetchedJoin=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__request_colorAddress,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__request_auxAddress,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__request_passthrough_newDepth,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__queuedReqsRaw_fifo_io_occupancy,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__readJoined_valid,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetchedJoined_valid);
            }
        }

        /* --- FramebufferAccess fetched/joined pixel --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_valid &&
            r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr,
                        "[PIXEL %d,%d] FBA_FETCH: colorData=0x%08x auxData=0x%08x newDepth=0x%04x oldDepth=0x%04x depthPassed=%d depthKill=%d laneHi(c=%d a=%d) wbuf=%d depthSrc=%d depthBias=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_colorData,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_auxData,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_newDepth,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__oldDepth,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__depthPassed,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__depthKill,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_colorLaneHi,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_auxLaneHi,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_fbzMode_wBufferSelect,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_fbzMode_depthSourceSelect,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__fetched_payload_passthrough_fbzMode_enableDepthBias);
            }
        }

        /* --- FramebufferAccess after depth test --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_valid &&
            r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_ready) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr,
                        "[PIXEL %d,%d] FBA_PASS: rgb=(%d,%d,%d) alpha=%d depthPassed=%d depthKill=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_color_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_color_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_color_b,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__afterDepthTest_payload_passthrough_alpha,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__depthPassed,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__depthKill);
            }
        }

        /* --- FramebufferAccess output (after depth test + alpha blend) --- */
        if (r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] FBA: rgb=(%d,%d,%d) alpha=%d blended=(%d,%d,%d) existing=0x%08x blend=%d srcF=%d dstF=%d depthF=%d\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_color_b,
                        r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_payload_alpha,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_r,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_g,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__blended_b,
                        0u,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_alphaBlendEnable,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_rgbSrcFact,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_alphaMode_rgbDstFact,
                        r->CoreSim__DOT__core_1__DOT__fbAccess__DOT__io_input_payload_fbzMode_depthFunction);
            }
        }

        /* --- Pre-dither arbiter output --- */
        if (r->CoreSim__DOT__core_1__DOT__preDitherMerged_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                uint8_t chosen = r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_chosen;
                fprintf(stderr,
                        "[PIXEL %d,%d] PRE: src=%s chosen=%u valids=%d%d%d readies=%d%d%d rgb888=(%d,%d,%d) rgbWrite=%d auxWrite=%d fbBase=0x%06x auxBase=0x%06x stride=%u\n",
                        px, py,
                        predither_source_name(chosen), chosen,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_0_valid,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_1_valid,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_2_valid,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_0_ready,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_1_ready,
                        r->CoreSim__DOT__core_1__DOT__streamArbiter_7__DOT__io_inputs_2_ready,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_r,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_g,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_b,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_rgbWrite,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_auxWrite,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_fbBaseAddr,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_auxBaseAddr,
                        r->CoreSim__DOT__core_1__DOT__preDitherMerged_payload_fbPixelStride);
            }
        }

        /* --- Pre-dither (RGB888 entering dither) --- */
        if (r->CoreSim__DOT__core_1__DOT__ditherJoined_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_coords_1);
            if (px == watch_x && py == watch_y) {
                fprintf(stderr, "[PIXEL %d,%d] DIT: rgb888=(%d,%d,%d) fbBase=0x%06x stride=%u\n",
                        px, py,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_r,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_g,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_b,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_fbBaseAddr,
                        r->CoreSim__DOT__core_1__DOT__ditherJoined_payload_2_fbPixelStride);
            }
        }

        /* --- Write stage (final RGB565 on color plane) --- */
        if (r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_valid) {
            int16_t px = sext12(r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_payload_coords_0);
            int16_t py = sext12(r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_payload_coords_1);
            if (px == watch_x && py == watch_y) {
                uint16_t rgb565 = r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_payload_data;
                fprintf(stderr, "[PIXEL %d,%d] WR:  rgb565=0x%04x (r5=%d g6=%d b5=%d) fbBase=0x%06x front=0x%06x back=0x%06x swapCount=%u\n",
                        px, py,
                        rgb565,
                        (rgb565 >> 11) & 0x1f,
                        (rgb565 >> 5) & 0x3f,
                        rgb565 & 0x1f,
                        r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_payload_fbBaseAddr,
                        r->CoreSim__DOT__core_1__DOT__frontBufferBase,
                        r->CoreSim__DOT__core_1__DOT__backBufferBase,
                        r->CoreSim__DOT__core_1__DOT__swapBuffer_1__DOT__swapCountReg);
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
static void bus_write_masked(uint32_t addr, uint32_t data, uint8_t mask, uint8_t length) {
    uint64_t t0 = sim_time;
    top->io_cpuBus_cmd_valid = 1;
    top->io_cpuBus_cmd_payload_fragment_opcode = BMB_OPCODE_WRITE;
    top->io_cpuBus_cmd_payload_fragment_address = addr & 0xFFFFFF;
    top->io_cpuBus_cmd_payload_fragment_data = data;
    top->io_cpuBus_cmd_payload_fragment_mask = mask;
    top->io_cpuBus_cmd_payload_fragment_length = length;
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

static void bus_write(uint32_t addr, uint32_t data) {
    bus_write_masked(addr, data, 0xF, 3);
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
    contextp = new VerilatedContext;
    if (!contextp) return -1;
    contextp->commandArgs(0, (const char **)nullptr);
    contextp->threads(1);

#ifdef VM_TRACE_FST
    /* Must call traceEverOn before model construction if tracing */
    const char *fst_path = getenv("SIM_FST");
    if (fst_path) {
        contextp->traceEverOn(true);
    }
#endif

    top = new VCoreSim{contextp};
    if (!top) return -1;

    /* Reset sequence */
    top->clk = 0;
    top->reset = 1;
    top->io_vRetrace = 0;
    top->io_flushFbCaches = 0;

    const char *debug_fb_addr_env = getenv("SIM_DEBUG_FB_ADDR");
    if (debug_fb_addr_env && debug_fb_addr_env[0]) {
        debug_fb_addr = (int32_t)strtoul(debug_fb_addr_env, nullptr, 0);
        fprintf(stderr, "[sim_harness] SIM_DEBUG_FB_ADDR=0x%06x\n", (uint32_t)debug_fb_addr);
    } else {
        debug_fb_addr = -1;
    }
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

    const char *buslog_path = getenv("SIM_BUS_LOG");
    if (buslog_path) {
        bus_log = fopen(buslog_path, "w");
        if (bus_log) {
            fprintf(stderr, "[sim_harness] Bus logging to %s\n", buslog_path);
            fprintf(bus_log, "# op addr data\n");
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
    if (bus_log) {
        fclose(bus_log);
        bus_log = nullptr;
    }
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
        auto r = top->rootp;
        fprintf(stderr,
                "[sim_harness] Fill stats: tmu hit=%u miss=%u burst=%u beats=%u stall=%u fastBi=%u | fb hit=%u miss=%u burst=%u beats=%u stall=%u\n",
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFillHits,
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFillMisses,
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFillBurstCount,
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFillBurstBeats,
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFillStallCycles,
                r->CoreSim__DOT__core_1__DOT__tmu_1__DOT__textureCache__DOT__texFastBilinearHits,
                r->CoreSim__DOT__core_1__DOT__fbFillHits,
                r->CoreSim__DOT__core_1__DOT__fbFillMisses,
                r->CoreSim__DOT__core_1__DOT__fbFillBurstCount,
                r->CoreSim__DOT__core_1__DOT__fbFillBurstBeats,
                r->CoreSim__DOT__core_1__DOT__fbFillStallCycles);
        top->final();
        delete top;
        top = nullptr;
    }
    if (contextp) {
        delete contextp;
        contextp = nullptr;
    }
    fprintf(stderr, "[sim_harness] Shutdown after %lu cycles (%lu R, %lu W)\n",
            (unsigned long)(sim_time / 2),
            (unsigned long)total_read_count, (unsigned long)total_write_count);
}

void sim_write(uint32_t addr, uint32_t data) {
    bus_write(addr, data);
    log_bus_event('W', addr, data);
}

void sim_write16(uint32_t addr, uint16_t data) {
    const int lane_hi = (addr & 0x2) != 0;
    const uint32_t packed_data = lane_hi ? ((uint32_t)data << 16) : (uint32_t)data;
    const uint8_t mask = lane_hi ? 0xC : 0x3;
    bus_write_masked(addr, packed_data, mask, 1);
}

uint32_t sim_read(uint32_t addr) {
    uint32_t data = bus_read(addr);
    log_bus_event('R', addr, data);
    return data;
}


uint32_t sim_idle_wait(void) {
    /* Poll status register until FIFO has space and pipeline not busy,
     * matching sst1InitIdleLoop's approach of reading through the bus.
     * Also wait for swap queue to drain so final framebuffer selection
     * cannot race a pending swap.
     * Status register bits:
     *   [5:0]  = pciFifoFree
     *   [9]    = sstBusy
     *   [30:28]= swapsPending */
    #define SST_BUSY       (1u << 9)
    #define SST_FIFOFREE_MASK 0x3Fu
    #define SST_SWAPS_PENDING_MASK (7u << 28)

    int timeout = 5000000;
    uint64_t t0 = sim_time;
    int idle_count = 0;
    while (timeout > 0) {
        uint32_t status = bus_read(0x000000);
        int busy = (status & SST_BUSY) != 0;
        int fifo_free = status & SST_FIFOFREE_MASK;
        int swaps_pending = (status & SST_SWAPS_PENDING_MASK) >> 28;

        if (!busy && fifo_free == 0x3F && swaps_pending == 0) {
            if (++idle_count >= 3) {  /* Match sst1InitIdleLoop: 3 consecutive idle reads */
                return status;
            }
        } else {
            idle_count = 0;
        }

        timeout--;
    }

    uint32_t status = bus_read(0x000000);
    auto r = top->rootp;
    fprintf(stderr, "[sim_harness] WARNING: idle_wait timeout after %lu ticks! status=0x%08x fifo=%u busy=%u swaps=%u\n",
            (unsigned long)((sim_time - t0) / 2), status,
            status & SST_FIFOFREE_MASK,
            (status & SST_BUSY) ? 1u : 0u,
            (status & SST_SWAPS_PENDING_MASK) >> 28);
    fprintf(stderr,
            "[sim_harness] Busy detail: pipe=%u regPipe=%u pciPipe=%u rastRun=%u tmuBusy=%u fbBusy=%u fbColorBusy=%u fbAuxBusy=%u lfbBusy=%u swapWait=%u fbOut=%u writeIn=%u\n",
            r->CoreSim__DOT__core_1__DOT__pipelineBusySignal,
            r->CoreSim__DOT__core_1__DOT__regBank__DOT__io_pipelineBusy,
            r->CoreSim__DOT__core_1__DOT__pciFifo_1__DOT__io_pipelineBusy,
            r->CoreSim__DOT__core_1__DOT__rasterizer_1_running,
            r->CoreSim__DOT__core_1__DOT__tmu_1_io_busy,
            r->CoreSim__DOT__core_1__DOT__fbAccess_io_busy,
            r->CoreSim__DOT__core_1__DOT__fbColorBusy,
            r->CoreSim__DOT__core_1__DOT__fbAuxBusy,
            r->CoreSim__DOT__core_1__DOT__lfb_1_io_busy,
            r->CoreSim__DOT__core_1__DOT__swapBuffer_1_io_waiting,
            r->CoreSim__DOT__core_1__DOT__fbAccess_io_output_valid,
            r->CoreSim__DOT__core_1__DOT__writeColor__DOT__i_fromPipeline_valid);
    return status;
}

void sim_tick(int n) {
    for (int i = 0; i < n; i++) {
        tick_one();
    }
}

void sim_flush_fb_cache(void) {
    dump_fb_debug("before_flush");
    top->io_flushFbCaches = 1;
    sim_idle_wait();
    dump_fb_debug("after_idle_wait");
    top->io_flushFbCaches = 0;
    tick_one();
    dump_fb_debug("after_flush_release");
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
