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

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define HWOFFSET(p, o, pname) if (hex) \
        printf("%s\tequ %08xh\n",pname,(((int) &p.o)-(int)&p)>>2);\
    else printf("%s\tequ %10d\n",pname,(((int)&p.o)-(int)&p))
#endif /* (GLIDE_PLATFORM & GLIDE_HW_SST96) */

#define OFFSET2(p,o,pname) if (hex) \
        printf("%s\tequ %08xh\n",pname,((int)&o)-(int)&p); \
    else printf("%s\tequ %10d\n",pname,((int)&o)-(int)&p)

#define SIZEOF(p,pname) if (hex) \
        printf("SIZEOF_%s\tequ %08lxh\n",pname,(unsigned long)sizeof(p)); \
    else printf("SIZEOF_%s\tequ %10lu\n",pname,(unsigned long)sizeof(p))

#define CONST(name) if (hex) \
        printf(#name " equ 0%xh\n", name); \
    else printf(#name " equ %d\n", name)


int
main (int argc, char **argv)
{
    int hex=0;          /* default is print in decimal  */
    static Sstregs sst;
    static struct _GlideRoot_s gr;
    static GrGC gc;
    static struct dataList_s dl;

    if (argc > 1) {
      if (strcmp("-inline", argv[1]) == 0) {
        Sstregs dummyRegs = { 0x00UL }; /* silence VC6 */

        printf("#ifndef __FX_INLINE_H__\n");
        printf("#define __FX_INLINE_H__\n");
        printf("\n");

        printf("/* The # of 2-byte entries in the hw fog table */\n");
        printf("#define kInternalFogTableEntryCount 0x%X\n",
               (unsigned int)sizeof(dummyRegs.fogTable) >> 1);

        printf("\n");
        printf("#endif /* __FX_INLINE_H__ */\n");

        return 0;
      }

      hex = 1;
    }

    CONST (SST_CHIP_MASK);

    HEADER ("SSTREGS");
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
    OFFSET (sst,FvA.x,"FVAX\t\t");
    OFFSET (sst,FvA.y,"FVAY\t\t");
    OFFSET (sst,FvB.x,"FVBX\t\t");
    OFFSET (sst,FvB.y,"FVBY\t\t");
    OFFSET (sst,FvC.x,"FVCX\t\t");
    OFFSET (sst,FvC.y,"FVCY\t\t");
    OFFSET (sst,FtriangleCMD,"FTRIANGLECMD\t");
#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
    HWOFFSET (sst,FvA.x,"FVAX\t\t");
    HWOFFSET (sst,FvA.y,"FVAY\t\t");
    HWOFFSET (sst,FvB.x,"FVBX\t\t");
    HWOFFSET (sst,FvB.y,"FVBY\t\t");
    HWOFFSET (sst,FvC.x,"FVCX\t\t");
    HWOFFSET (sst,FvC.y,"FVCY\t\t");
    HWOFFSET (sst,FtriangleCMD,"FTRIANGLECMD\t");
#else
#error "Update fxgasm.c for this chip"
#endif
    NEWLINE;
#ifdef GLIDE_USE_ALT_REGMAP
    OFFSET2(sst.Fr_ALT,sst.Fdrdx_ALT,"FDPDX_OFFSET\t");
    OFFSET2(sst.Fr_ALT,sst.Fdrdy_ALT,"FDPDY_OFFSET\t");
#else
    OFFSET2(sst.Fr,sst.Fdrdx,"FDPDX_OFFSET\t");
    OFFSET2(sst.Fr,sst.Fdrdy,"FDPDY_OFFSET\t");
#endif
    HEADER ("GC");
    OFFSET (gc,base_ptr,"base_ptr\t");
    OFFSET (gc,reg_ptr,"reg_ptr\t\t");
    OFFSET (gc,lfb_ptr,"lfb_ptr\t\t");
    OFFSET (gc,state.cull_mode,"cull_mode\t");
    OFFSET (gc,dataList,"dataList\t");
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
    OFFSET (gc,state.fifoFree, "fifoFree\t");
#endif
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
    OFFSET (gc,hwDep.sst96Dep.writesSinceFence, "writesSinceFence" );
    OFFSET (gc,hwDep.sst96Dep.paramMask, "paramMask\t");
    OFFSET (gc,hwDep.sst96Dep.gwCommand, "gwCommand\t");
    OFFSET (gc,hwDep.sst96Dep.gwHeaders, "gwHeaders\t");
    OFFSET (gc,fifoData.hwDep.vg96FIFOData.fifoPtr, "fifoPtr\t\t");
    OFFSET (gc,fifoData.hwDep.vg96FIFOData.fifoSize, "fifoSize\t");
#endif
    SIZEOF (gr.GCs[0].state,"GrState\t");
    SIZEOF (gr.hwConfig,"GrHwConfiguration");
    SIZEOF (gr.GCs[0],"GC\t");
    NEWLINE;

    HEADER ("GlideRoot");
    OFFSET (gr,p6Fencer,"p6Fencer\t");
    OFFSET (gr,current_sst,"current_sst\t");
    OFFSET (gr,CPUType,"CPUType\t\t");
    OFFSET (gr,curGC,"curGC\t\t");
    OFFSET (gr,curTriSize,"curTriSize\t");
    OFFSET (gr,stats.trisProcessed,"trisProcessed\t");
    OFFSET (gr,stats.trisDrawn,"trisDrawn\t");
    SIZEOF (gr,"GlideRoot");
    NEWLINE;

    HEADER ("dataList");
    OFFSET (dl,i,"dl_i\t\t");
    OFFSET (dl,addr,"dl_addr\t\t");
    SIZEOF (dl,"dataList\t");
    NEWLINE;

    return 0;
}
