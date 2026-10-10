/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVE
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/*----------------------------------------------------------------------
 * macros for creating assembler offset files
 *----------------------------------------------------------------------*/

#define NEWLINE printf("\n")
#define COMMENT printf(";----------------------------------------------------------------------\n")

#define HEADER(str)     NEWLINE; COMMENT; \
                        printf("; Assembler offsets for %s struct\n",str);\
                        COMMENT; NEWLINE

#define OFFSET(p,o,pname) if (hex) \
        printf("%s\tequ %08xh\n",pname,((int)&p.o)-(int)&p); \
    else printf("%s\tequ %10d\n",pname,((int)&p.o)-(int)&p)

#define OFFSET2(p,o,pname) if (hex) \
        printf("%s\tequ %08xh\n",pname,((int)&o)-(int)&p); \
    else printf("%s\tequ %10d\n",pname,((int)&o)-(int)&p)

#define SIZEOF(p,pname) if (hex) \
        printf("SIZEOF_%s\tequ %08lxh\n",pname,(unsigned long)sizeof(p)); \
    else printf("SIZEOF_%s\tequ %10lu\n",pname,(unsigned long)sizeof(p))


int
main (int argc, char **argv)
{
    int hex=0;          /* default is print in decimal  */
    static struct _GlideRoot_s gr;
    static GrGC gc;

#if !GLIDE_HW_TRI_SETUP
    static SstRegs sst;
    static struct dataList_s dl;
#endif /* !GLIDE_HW_TRI_SETUP */

    if (argc > 1) {
      if (strcmp("-inline", argv[1]) == 0) {
        SstRegs dummyRegs = { 0x00UL }; /* silence VC6 */

        printf("#ifndef __FX_INLINE_H__\n");
        printf("#define __FX_INLINE_H__\n");
        printf("\n");

        printf("#define kTriProcOffset 0x%lXUL\n",
               (unsigned long)offsetof(struct GrGC_s, triSetupProc));

        printf("/* The # of 2-byte entries in the hw fog table */\n");
        printf("#define kInternalFogTableEntryCount 0x%X\n",
               (unsigned int)sizeof(dummyRegs.fogTable) >> 1);

        printf("#define kTLSOffset 0x%lXUL\n",
               (unsigned long)offsetof(struct _GlideRoot_s, tlsOffset));

        printf("#define kLostContextOffset 0x%lxUL\n",
               (unsigned long)offsetof(GrGC, lostContext));

        printf("\n");
        printf("#endif /* __FX_INLINE_H__ */\n");

        return 0;
      }

      hex = 1;
    }

#if !GLIDE_HW_TRI_SETUP
    printf("SST_CHIP_MASK = 0%xh\n",SST_CHIP_MASK);
#endif

    HEADER ("SSTREGS");

    NEWLINE;
    HEADER ("GC");
    OFFSET (gc,base_ptr,"base_ptr\t");
    OFFSET (gc,reg_ptr,"reg_ptr\t\t");
    OFFSET (gc,lfb_ptr,"lfb_ptr\t\t");
    OFFSET (gc,state.cull_mode,"cull_mode\t");
#ifndef GLIDE3
    OFFSET (gc, regDataList,"regDataList\t");
#endif
#ifdef GLIDE_DEBUG
    OFFSET (gc,checkPtr,"checkPtr\t\t");
#endif
    OFFSET (gc, tsuDataList,"tsuDataList\t");
    OFFSET (gc, cmdTransportInfo.triPacketHdr, "triPacketHdr");
    OFFSET (gc, cmdTransportInfo.cullStripHdr, "cullStripHdr");
    OFFSET (gc, cmdTransportInfo.paramMask, "paramMask");

    OFFSET (gc, cmdTransportInfo.fifoStart, "fifoStart");
    OFFSET (gc, cmdTransportInfo.fifoEnd, "fifoEnd");
    OFFSET (gc, cmdTransportInfo.fifoOffset, "fifoOffset");
    OFFSET (gc, cmdTransportInfo.fifoSize, "fifoSize");
    OFFSET (gc, cmdTransportInfo.fifoJmpHdr, "fifoJmpHdr");
    OFFSET (gc, cmdTransportInfo.fifoPtr, "fifoPtr");
    OFFSET (gc, cmdTransportInfo.fifoRead, "fifoRead");

    OFFSET (gc, cmdTransportInfo.fifoRoom, "fifoRoom");
    OFFSET (gc, cmdTransportInfo.roomToReadPtr, "roomToReadPtr");
    OFFSET (gc, cmdTransportInfo.roomToEnd, "roomToEnd");

    OFFSET (gc, cmdTransportInfo.lfbLockCount, "lfbLockCount");

    OFFSET (gc, triSetupProc, "kTriProcOffset");
    OFFSET (gc, archDispatchProcs.triSetupProc, "triSetupProc");
    OFFSET (gc, archDispatchProcs.drawTrianglesProc, "drawTrianglesProc");
    OFFSET (gc, archDispatchProcs.triSetupProc, "drawVertexList");

    /* for Glide3 strip routine */
    OFFSET (gc, state.vData.vSize, "vertexSize");
    OFFSET (gc, state.vData.vStride, "vertexStride");
    OFFSET (gc, state.invalid, "invalid");
    OFFSET (gc, state.grCoordinateSpaceArgs.coordinate_space_mode, "CoordinateSpace");
#ifdef GLIDE_VERTEX_TABLE
    OFFSET (gc, state.vTable, "vTable");
#endif
    /* for Glide3 clip coordinate */
    OFFSET (gc, state.paramIndex, "paramIndex");
    OFFSET (gc, state.Viewport.hwidth, "vp_hwidth");
    OFFSET (gc, state.Viewport.hheight, "vp_hheight");
    OFFSET (gc, state.Viewport.hdepth, "vp_hdepth");
    OFFSET (gc, state.Viewport.ox, "vp_ox");
    OFFSET (gc, state.Viewport.oy, "vp_oy");
    OFFSET (gc, state.Viewport.oz, "vp_oz");
    OFFSET (gc, state.vData.colorType, "colorType");
    OFFSET (gc, state.vData.wInfo.offset, "wInfo_offset");
    OFFSET (gc, state.vData.qInfo.mode, "qInfo_mode");
    OFFSET (gc, state.vData.qInfo.offset, "qInfo_offset");
    OFFSET (gc, state.vData.q0Info.offset, "q0Info_offset");
    OFFSET (gc, state.vData.q1Info.offset, "q1Info_offset");
    OFFSET (gc, state.vData.q0Info.mode, "q0Info_mode");
    OFFSET (gc, state.vData.q1Info.mode, "q1Info_mode");
    OFFSET (gc, state.vData.fogInfo.offset, "fogInfo_offset");
    OFFSET (gc, state.vData.fogInfo.mode, "fogInfo_mode");
    OFFSET (gc, state.depth_range, "depth_range");

    OFFSET (gc, state.per_tmu[0].s_scale, "tmu0_s_scale");
    OFFSET (gc, state.per_tmu[0].t_scale, "tmu0_t_scale");
    OFFSET (gc, state.per_tmu[1].s_scale, "tmu1_s_scale");
    OFFSET (gc, state.per_tmu[1].t_scale, "tmu1_t_scale");
    OFFSET (gc, state.shadow.fbzMode, "fbi_fbzMode");

    OFFSET (gc, curTriSize, "curTriSize\t");
    OFFSET (gc, stats.trisProcessed, "trisProcessed\t");
    OFFSET (gc, stats.trisDrawn, "trisDrawn\t");
    OFFSET (gc, lostContext, "lostContext\t");
    OFFSET (gc, windowed, "windowed\t");
#ifdef GLIDE_INIT_HWC
    OFFSET (gc, bInfo, "bInfo\t");
#endif

#if GLIDE_PACKED_RGB
    OFFSET (gc, pool.temp1.f, "fTemp1");
    OFFSET (gc, pool.temp2.f, "fTemp2");
#endif

    HEADER ("GlideRoot");
#if defined(__WATCOMC__) || defined(__MSC__) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
    OFFSET (gr,p6Fencer,"p6Fencer\t");
#endif
    OFFSET (gr,current_sst,"current_sst\t");
#if GL_X86
    OFFSET (gr,CPUType,"CPUType\t\t");
#endif
    OFFSET (gr,tlsOffset,"tlsOffset\t\t");
    OFFSET (gr, pool.f255,"pool_f255");
    OFFSET (gr, pool.f1,"pool_f1");
    SIZEOF (gr.GCs[0].state,"GrState\t");
    SIZEOF (gr.GCs[0],"GC\t");
    NEWLINE;


#if GLIDE_PACKED_RGB
    OFFSET (gr, pool.fBiasHi,"fBiasHi");
    OFFSET (gr, pool.fBiasLo,"fBiasLo");
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_PACKED_RGB */

    SIZEOF (gr,"GlideRoot");
    NEWLINE;

#if !GLIDE_HW_TRI_SETUP
    HEADER ("dataList");
    OFFSET (dl,i,"dl_i\t\t");
    OFFSET (dl,addr,"dl_addr\t\t");
    SIZEOF (dl,"dataList\t");
    NEWLINE;
#endif /* !GLIDE_HW_TRI_SETUP */

    return 0;
}
