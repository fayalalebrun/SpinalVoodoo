#include "glide.h"
#include "mister_runtime.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned drawn, swapped;
static int fail(const char *phase) {
    const char *selected = getenv("MISTER_DEMO_TEST_FAIL");
    return selected && !strcmp(selected, phase);
}
void grGlideInit(void) {}
void grGlideShutdown(void) { assert(drawn == swapped); }
FxBool grSstQueryHardware(GrHwConfiguration *hardware) { assert(hardware); return !fail("query"); }
void grSstSelect(int index) { assert(index == 0); }
FxBool grSstWinOpen(FxU32 window, GrScreenResolution_t resolution, GrScreenRefresh_t refresh,
                   GrColorFormat_t format, GrOriginLocation_t origin, int colors, int aux) {
    assert(!window && resolution == GR_RESOLUTION_640x480 && refresh == GR_REFRESH_60Hz);
    assert(format == GR_COLORFORMAT_ARGB && origin == GR_ORIGIN_UPPER_LEFT && colors == 2 && aux == 1);
    return !fail("open");
}
void grSstWinClose(void) {}
void grColorCombine(GrCombineFunction_t function, GrCombineFactor_t factor,
                    GrCombineLocal_t local, GrCombineOther_t other, FxBool invert) {
    assert(function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_NONE);
    assert(local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_NONE && !invert);
}
void grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU16 depth) {
    assert(color == 0xff0000ff && alpha == 255 && depth == 0xffff);
}
void grConstantColorValue(GrColor_t color) { assert(color == 0xff00ff00); }
void grDrawTriangle(const GrVertex *a, const GrVertex *b, const GrVertex *c) {
    assert(a->x == 40.0f + (float)((drawn * 7) % 360) && a->y == 100 && a->oow == 1);
    assert(b->x == a->x + 180 && b->y == 100 && b->oow == 1);
    assert(c->x == a->x && c->y == 350 && c->oow == 1);
    ++drawn;
}
void grBufferSwap(int interval) { assert(!interval && drawn == ++swapped); }
FxBool grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t mode,
                 GrOriginLocation_t origin, FxBool pipeline, GrLfbInfo_t *info) {
    assert(type == GR_LFB_READ_ONLY && buffer == GR_BUFFER_FRONTBUFFER);
    assert(mode == GR_LFBWRITEMODE_ANY && origin == GR_ORIGIN_UPPER_LEFT && !pipeline);
    assert(info && info->size == sizeof(*info));
    return !fail("lock");
}
FxBool grLfbUnlock(GrLock_t type, GrBuffer_t buffer) {
    assert(type == GR_LFB_READ_ONLY && buffer == GR_BUFFER_FRONTBUFFER);
    return !fail("unlock");
}
void sim_idle_wait(void) {}
int mister_runtime_exchange(const mister_command *commands, size_t count, uint32_t *results,
                            mister_keys *status) {
    assert(count == 2 && !status && swapped > 0);
    unsigned x = 60 + ((swapped - 1) * 7) % 360;
    assert(commands[0].opcode == 2 && commands[0].address == 0x400000 + 120 * 2048 + (x / 2) * 4);
    assert(commands[1].opcode == 2 && commands[1].address == 0x400000 + 450 * 2048 + 1200);
    results[0] = fail("pixels") ? 0 : 0x07e007e0;
    results[1] = 0x001f001f;
    return fail("exchange") ? -1 : 0;
}
