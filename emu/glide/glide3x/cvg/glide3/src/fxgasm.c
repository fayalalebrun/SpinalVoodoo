/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
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

        printf("#define kCurGCOffset   0x%lXUL\n",
               (unsigned long)offsetof(struct _GlideRoot_s, curGC));

#if GLIDE_DISPATCH_SETUP
        printf("#define kTriProcOffset 0x%lXUL\n",
               (unsigned long)offsetof(struct GrGC_s, curArchProcs.triSetupProc));
	printf("#define kGCStateInvalid 0x%lXUL\n",
	       (unsigned long)offsetof(struct GrGC_s, state.invalid));
#endif /* GLIDE_DISPATCH_SETUP */
        
        printf("/* The # of 2-byte entries in the hw fog table */\n");
        printf("#define kInternalFogTableEntryCount 0x%X\n",
               (unsigned int)sizeof(dummyRegs.fogTable) >> 1);

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
    OFFSET (gc,tex_ptr,"tex_ptr\t\t");
    OFFSET (gc,state.cull_mode,"cull_mode\t");
#ifndef GLIDE3
    OFFSET (gc, regDataList,"regDataList\t");
#endif
#ifdef GLIDE_DEBUG
    OFFSET (gc,checkPtr,"checkPtr\t\t");
#endif
    OFFSET (gc, tsuDataList,"tsuDataList\t");
#ifdef GLIDE3_SCALER
    OFFSET (gc, tsuDataListScaler,"tsuDatLstScal\t");
#endif
    OFFSET (gc, cmdTransportInfo.triPacketHdr, "triPacketHdr");
    OFFSET (gc, cmdTransportInfo.cullStripHdr, "cullStripHdr");
    OFFSET (gc, cmdTransportInfo.paramMask, "paramMask");

#if GLIDE_DISPATCH_SETUP
    OFFSET (gc, curArchProcs.triSetupProc, "triSetupProc");
    OFFSET (gc, curArchProcs.drawTrianglesProc, "drawTrianglesProc");
    OFFSET (gc, curArchProcs.drawVertexList, "drawVertexList");
#endif /* GLIDE_DISPATCH_SETUP */

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

    OFFSET (gc, cmdTransportInfo.fifoLfbP, "fifoLfbP");
    OFFSET (gc, cmdTransportInfo.lfbLockCount, "lfbLockCount");

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

    OFFSET (gc, state.tmu_config[0].s_scale, "tmu0_s_scale");
    OFFSET (gc, state.tmu_config[0].t_scale, "tmu0_t_scale");
    OFFSET (gc, state.tmu_config[1].s_scale, "tmu1_s_scale");
    OFFSET (gc, state.tmu_config[1].t_scale, "tmu1_t_scale");

    SIZEOF (gr.GCs[0].state,"GrState\t");
#if 0
    SIZEOF (gr.hwConfig,"GrHwConfiguration");
#endif
    SIZEOF (gr.GCs[0],"GC\t");
    NEWLINE;

    HEADER ("GlideRoot");
#if defined(__WATCOMC__) || defined(__MSC__) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
    OFFSET (gr,p6Fencer,"p6Fencer\t");
#endif
    OFFSET (gr,current_sst,"current_sst\t");
#if GL_X86
    OFFSET (gr,CPUType,"CPUType\t\t");
#endif
    OFFSET (gr,curGC,"curGC\t\t");
    OFFSET (gr,curTriSize,"curTriSize\t");

    OFFSET (gr,stats.linesDrawn,"stats_linesDrawn\t");
    OFFSET (gr,stats.othertrisDrawn,"stats_otherTrisDrawn\t");
    OFFSET (gr,stats.trisProcessed,"trisProcessed\t");
    OFFSET (gr,stats.trisDrawn,"trisDrawn\t");

#if GLIDE_PACKED_RGB
    OFFSET (gr, pool.fBiasHi,"fBiasHi");
    OFFSET (gr, pool.fBiasLo,"fBiasLo");
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_PACKED_RGB */
    OFFSET (gr, pool.f0,    "pool_f0");
    OFFSET (gr, pool.fHalf, "pool_fHalf");
    OFFSET (gr, pool.f1,    "pool_f1");
    OFFSET (gr, pool.f255,  "pool_f255");

    OFFSET (gr, pool.temp1.f, "pool_fTemp1");
    OFFSET (gr, pool.temp2.f, "pool_fTemp2");

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
