#ifndef VID_VOODOO_TRACE_H
#define VID_VOODOO_TRACE_H

#include <stdint.h>

#include <86box/vid_voodoo_common.h>
#include "voodoo_trace_format.h"

typedef struct voodoo_trace_t {
    voodoo_t *voodoo;
    int dump_state;
    char trace_dir[4096];
} voodoo_trace_t;

static inline void voodoo_trace_dump_state(voodoo_trace_t *trace, uint32_t frame_num)
{
    (void)trace;
    (void)frame_num;
}

#endif
