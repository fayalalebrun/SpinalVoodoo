#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uintptr_t FxU32;
typedef struct { float RED, GRN, BLU, Z, ALF, S, T, W, FtriangleCMD; } Sstregs;
typedef struct { FxU32 i; float *addr; } Entry;
typedef struct { struct { FxU32 paramHints, paramIndex; } state; Sstregs *reg_ptr; Entry dataList[32]; } GrGC;
static GrGC context;
static unsigned char registers[65536] __attribute__((aligned(65536)));
static struct { unsigned CPUType; FxU32 packerFixAddress; int curTriSize, curTriSizeNoGradient; } _GlideRoot;
static unsigned writes, fences;
#define GLIDE_PLATFORM 1
#define GLIDE_HW_SST1 1
#define GLIDE_NUM_TMU 2
#define SST_CHIP_MASK 0x3c00
#define SST_CHIP(hw, chip) ((Sstregs *)((unsigned char *)(hw) + ((chip) << 10)))
#define GR_DCL_GC GrGC *gc = &context
#define GR_DCL_HW Sstregs *hw = gc->reg_ptr
#define GDBG_INFO(arguments) ((void)0)
#define GR_SET(destination, value) (++writes)
#define GR_P6FENCE (++fences)
#define STATE_REQUIRES_IT_DRGB 1
#define STATE_REQUIRES_OOZ 2
#define STATE_REQUIRES_IT_ALPHA 4
#define STATE_REQUIRES_ST_TMU0 8
#define STATE_REQUIRES_OOW_FBI 16
#define STATE_REQUIRES_W_TMU0 32
#define STATE_REQUIRES_ST_TMU1 64
#define STATE_REQUIRES_W_TMU1 128
#define GR_VERTEX_R_OFFSET 3
#define GR_VERTEX_G_OFFSET 4
#define GR_VERTEX_B_OFFSET 5
#define GR_VERTEX_OOZ_OFFSET 6
#define GR_VERTEX_A_OFFSET 7
#define GR_VERTEX_OOW_OFFSET 8
#define GR_VERTEX_SOW_TMU0_OFFSET 9
#define GR_VERTEX_TOW_TMU0_OFFSET 10
#define GR_VERTEX_OOW_TMU0_OFFSET 11
#define GR_VERTEX_SOW_TMU1_OFFSET 12
#define GR_VERTEX_TOW_TMU1_OFFSET 13
#define GR_VERTEX_OOW_TMU1_OFFSET 14
#define GR_DDFUNC(name, type, arguments) type legacy_list arguments
#include "packer-header.inc"
#include "packer-list.inc"
static void check_legacy_macro(void) {
    FxU32 lastAddress = 0;
    writes = fences = 0;
    PACKER_WORKAROUND;
    PACKER_BUGCHECK(0x800);
    assert(writes == 2 && fences == (_GlideRoot.CPUType == 6 ? 4u : 0u));
    assert(PACKER_WORKAROUND_SIZE == 4);
    (void)lastAddress;
}
#undef GR_DDFUNC
#undef PACKER_WORKAROUND_SIZE
#undef PACKER_WORKAROUND
#undef PACKER_BUGCHECK
#define MISTER_BACKEND 1
#define GR_DDFUNC(name, type, arguments) type mister_list arguments
#include "packer-header.inc"
#include "packer-list.inc"
static void check_mister_macro(void) {
    FxU32 lastAddress = 0;
    writes = fences = 0;
    PACKER_WORKAROUND;
    PACKER_BUGCHECK(0x800);
    assert(writes == 0 && fences == 0 && PACKER_WORKAROUND_SIZE == 0);
    GR_P6FENCE;
    GR_SET(*(FxU32 *)_GlideRoot.packerFixAddress, 0);
    assert(writes == 1 && fences == 1);
    (void)lastAddress;
}
int main(void) {
    _GlideRoot.packerFixAddress = (FxU32)registers + 0xe20000;
    unsigned checks = 0;
    for (unsigned cpu = 5; cpu <= 6; ++cpu) {
        _GlideRoot.CPUType = cpu;
        check_legacy_macro();
        check_mister_macro();
        for (unsigned mask = 0; mask < 256; ++mask) {
            Entry expected[32];
            unsigned kept = 0, removed = 0;
            int expected_size = 28, expected_no_gradient = 28;
            memset(&context, 0, sizeof(context));
            context.reg_ptr = (Sstregs *)registers;
            context.state.paramIndex = mask;
            if (mask) {
                legacy_list();
                for (unsigned i = 0; context.dataList[i].i; ++i) {
                    assert(i < 30);
                    if (context.dataList[i].i & 1) ++removed;
                    else expected[kept++] = context.dataList[i];
                }
                expected_size = _GlideRoot.curTriSize - (int)removed * 4;
                expected_no_gradient = _GlideRoot.curTriSizeNoGradient - (int)removed * 4;
            }
            memset(context.dataList, 0, sizeof(context.dataList));
            mister_list();
            for (unsigned i = 0; i < kept; ++i) {
                assert(context.dataList[i].i == expected[i].i);
                assert(context.dataList[i].addr == expected[i].addr);
            }
            assert(context.dataList[kept].i == 0);
            assert(context.dataList[kept + 1].addr == &context.reg_ptr->FtriangleCMD);
            assert(context.dataList[kept + 1].i == (cpu == 6 ? 3u : 1u));
            assert(_GlideRoot.curTriSize == expected_size);
            assert(_GlideRoot.curTriSizeNoGradient == expected_no_gradient);
            ++checks;
        }
    }
    printf("MiSTer PCI packer workaround removal: PASS (%u parameter/CPU combinations; legacy writes and real fences preserved)\n", checks);
    return 0;
}
