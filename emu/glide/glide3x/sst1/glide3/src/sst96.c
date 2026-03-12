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
**
** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
** Bastardised Glide3x for SST1
 * 
 * 20    9/19/97 12:38p Peter
 * asm rush trisetup vs alt fifo
 * 
 * 19    9/12/97 12:56p Dow
 * Protected JUJU code
 * 
 * 18    9/11/97 2:22p Atai
 * fixed muckage
 * 
 * 17    9/10/97 10:15p Peter
 * fifo logic from GaryT
 * 
 * 16    8/19/97 8:55p Peter
 * lots of stuff, hopefully no muckage
 * 
 * 15    8/01/97 11:49a Dow
 * Allowed for non4K fifo wraps
 * 
 * 14    7/08/97 1:29p Jdt
 * Fixed watcom muckage
 * 
 * 13    7/04/97 12:08p Dow
 * Modified GWP debugging
 * 
 * 12    6/20/97 5:54p Dow
 * Moved grDebugGroupWriteHeader in here.
 * 
 * 11    6/18/97 5:54p Dow
 * P6 adjustments
 * 
 * 10    5/27/97 2:03p Dow
 * added new assertion
 * 
 * 9     5/04/97 12:49p Dow
 * Moved texture downloads to level 195, like on SST1
 * 
 * 8     4/13/97 2:06p Pgj
 * eliminate all anonymous unions (use hwDep)
 * 
 * 7     3/16/97 12:39a Jdt
 * initWrapFifo only neads fifodata...
 * 
 * 6     3/04/97 9:12p Dow
 * Neutered mutiplatform multiheaded monster.
 * 
 * 5     2/18/97 9:53a Jdt
 * Removed some unecessary declarations.
 * 
 * 4     2/14/97 12:55p Dow
 * moved vg96 fifo wrap into init code
 * 
 * 3     2/11/97 6:59p Dow
 * Changes to support vid tiles and ser status
 * 
 * 2     1/14/97 10:49a Dow
 * Changed all output to use GDBG_INFO
 * Renabled some messages
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/
/* gsst96.c - transport stuff for SST 96 */

#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#include <glidesys.h>
#include <sst1vid.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)

#define GEN_INDEX(a) ((((FxU32) a) - ((FxU32) gc->reg_ptr)) >> 2)

#if GDBG_INFO_ON || defined(GLIDE_DEBUG)
const char
*regNames[] = {
  "status",             /* 0x00 */
  "reserved001",        /* 0x01 */
  "vAx",                /* 0x02 */
  "vAy",                /* 0x03 */
  "vBx",                /* 0x04 */
  "vBy",                /* 0x05 */
  "vCx",                /* 0x06 */
  "vCy",                /* 0x07 */
  "r",                  /* 0x08 */
  "drdx",               /* 0x09 */
  "drdy",               /* 0x0a */
  "g",                  /* 0x0b */
  "dgdx",               /* 0x0c */
  "dgdy",               /* 0x0d */
  "b",                  /* 0x0e */
  "dbdx",               /* 0x0f */
  "dbdy",               /* 0x10 */
  "z",                  /* 0x11 */
  "dzdx",               /* 0x12 */
  "dzdy",               /* 0x13 */
  "a",                  /* 0x14 */
  "dadx",               /* 0x15 */
  "dady",               /* 0x16 */
  "s",                  /* 0x17 */
  "dsdx",               /* 0x18 */
  "dsdy",               /* 0x19 */
  "t",                  /* 0x1a */
  "dtdx",               /* 0x1b */
  "dtdy",               /* 0x1c */
  "w",                  /* 0x1d */
  "dwdx",               /* 0x1e */
  "dwdy",               /* 0x1f */
  "triangleCMD",        /* 0x20 */
  "reserved021",        /* 0x21 */
  "FvAx",               /* 0x22 */
  "FvAy",               /* 0x23 */
  "FvBx",               /* 0x24 */
  "FvBy",               /* 0x25 */
  "FvCx",               /* 0x26 */
  "FvCy",               /* 0x27 */
  "Fr",                 /* 0x28 */
  "Fdrdx",              /* 0x29 */
  "Fdrdy",              /* 0x2a */
  "Fg",                 /* 0x2b */
  "Fdgdx",              /* 0x2c */
  "Fdgdy",              /* 0x2d */
  "Fb",                 /* 0x2e */
  "Fdbdx",              /* 0x2f */
  "Fdbdy",              /* 0x30 */
  "Fz",                 /* 0x31 */
  "Fdzdx",              /* 0x32 */
  "Fdzdy",              /* 0x33 */
  "Fa",                 /* 0x34 */
  "Fdadx",              /* 0x35 */
  "Fdady",              /* 0x36 */
  "Fs",                 /* 0x37 */
  "Fdsdx",              /* 0x38 */
  "Fdsdy",              /* 0x39 */
  "Ft",                 /* 0x3a */
  "Fdtdx",              /* 0x3b */
  "Fdtdy",              /* 0x3c */
  "Fw",                 /* 0x3d */
  "Fdwdx",              /* 0x3e */
  "Fdwdy",              /* 0x3f */
  "FtriangleCMD",       /* 0x40 */
  "reserved041",        /* 0x41 */
  "nopCMD",             /* 0x42 */
  "reserved043",        /* 0x43 */
  "fastfillCMD",        /* 0x44 */
  "reserved045",        /* 0x45 */
  "swapbufferCMD",      /* 0x46 */
  "reserved047",        /* 0x47 */
  "swappendCMD",        /* 0x48 */
  "reserved049",        /* 0x49 */
  "reserved04A",        /* 0x4a */
  "reserved04B",        /* 0x4b */
  "reserved04C",        /* 0x4c */
  "reserved04D",        /* 0x4d */
  "reserved04E",        /* 0x4e */
  "reserved04F",        /* 0x4f */
  "fbzColorPath",       /* 0x50 */
  "fogMode",            /* 0x51 */
  "alphaMode",          /* 0x52 */
  "reserved053",        /* 0x53 */
  "fbzMode",            /* 0x54 */
  "stipple",            /* 0x55 */
  "color0",             /* 0x56 */
  "color1",             /* 0x57 */
  "fogColor",           /* 0x58 */
  "zaColor",            /* 0x59 */
  "chromaKey",          /* 0x5a */
  "chromaRange",        /* 0x5b */
  "reserved05C",        /* 0x5c */
  "reserved05D",        /* 0x5d */
  "reserved05E",        /* 0x5e */
  "reserved05F",        /* 0x5f */
  "colBufferSetup",     /* 0x60 */
  "auxBufferSetup",     /* 0x61 */
  "clipLeftRight0",     /* 0x62 */
  "clipTopBottom0",     /* 0x63 */
  "clipLeftRight1",     /* 0x64 */
  "clipTopBottom1",     /* 0x65 */
  "reserved066",        /* 0x66 */
  "reserved067",        /* 0x67 */
  "reserved068",        /* 0x68 */
  "reserved069",        /* 0x69 */
  "reserved06A",        /* 0x6a */
  "reserved06B",        /* 0x6b */
  "reserved06C",        /* 0x6c */
  "reserved06D",        /* 0x6d */
  "reserved06E",        /* 0x6e */
  "reserved06F",        /* 0x6f */
  "fogTable0001",       /* 0x70 */
  "fogTable0203",       /* 0x71 */
  "fogTable0405",       /* 0x72 */
  "fogTable0607",       /* 0x73 */
  "fogTable0809",       /* 0x74 */
  "fogTable0A0B",       /* 0x75 */
  "fogTable0C0D",       /* 0x76 */
  "fogTable0E0F",       /* 0x77 */
  "fogTable1011",       /* 0x78 */
  "fogTable1213",       /* 0x79 */
  "fogTable1415",       /* 0x7a */
  "fogTable1617",       /* 0x7b */
  "fogTable1819",       /* 0x7c */
  "fogTable1A1B",       /* 0x7d */
  "fogTable1C1D",       /* 0x7e */
  "fogTable1E1F",       /* 0x7f */
  "fogTable2021",       /* 0x80 */
  "fogTable2223",       /* 0x81 */
  "fogTable2425",       /* 0x82 */
  "fogTable2627",       /* 0x83 */
  "fogTable2829",       /* 0x84 */
  "fogTable2A2B",       /* 0x85 */
  "fogTable2C2D",       /* 0x86 */
  "fogTable2E2F",       /* 0x87 */
  "fogTable3031",       /* 0x88 */
  "fogTable3233",       /* 0x89 */
  "fogTable3435",       /* 0x8a */
  "fogTable3637",       /* 0x8b */
  "fogTable3839",       /* 0x8c */
  "fogTable3A3B",       /* 0x8d */
  "fogTable3C3D",       /* 0x8e */
  "fogTable3E3F",       /* 0x8f */
  "fbijrInit0",         /* 0x90 */
  "fbijrInit1",         /* 0x91 */
  "fbijrInit2",         /* 0x92 */
  "fbijrInit3",         /* 0x93 */
  "fbijrInit4",         /* 0x94 */
  "fbijrInit5",         /* 0x95 */
  "reserved096",        /* 0x96 */
  "reserved097",        /* 0x97 */
  "reserved098",        /* 0x98 */
  "reserved099",        /* 0x99 */
  "reserved09A",        /* 0x9a */
  "reserved09B",        /* 0x9b */
  "reserved09C",        /* 0x9c */
  "reserved09D",        /* 0x9d */
  "reserved09E",        /* 0x9e */
  "reserved09F",        /* 0x9f */
  "fbijrVersion",       /* 0xa0 */
  "fbiPixelsIn",        /* 0xa1 */
  "fbiChromaFail",      /* 0xa2 */
  "fbiZfuncFail",       /* 0xa3 */
  "fbiAfuncFail",       /* 0xa4 */
  "fbiPixelsOut",       /* 0xa5 */
  "reserved0A6",        /* 0xa6 */
  "reserved0A7",        /* 0xa7 */
  "reserved0A8",        /* 0xa8 */
  "reserved0A9",        /* 0xa9 */
  "reserved0AA",        /* 0xaa */
  "reserved0AB",        /* 0xab */
  "reserved0AC",        /* 0xac */
  "reserved0AD",        /* 0xad */
  "reserved0AE",        /* 0xae */
  "reserved0AF",        /* 0xaf */
  "reserved0B0",        /* 0xb0 */
  "reserved0B1",        /* 0xb1 */
  "reserved0B2",        /* 0xb2 */
  "reserved0B3",        /* 0xb3 */
  "reserved0B4",        /* 0xb4 */
  "reserved0B5",        /* 0xb5 */
  "reserved0B6",        /* 0xb6 */
  "reserved0B7",        /* 0xb7 */
  "reserved0B8",        /* 0xb8 */
  "reserved0B9",        /* 0xb9 */
  "reserved0BA",        /* 0xba */
  "reserved0BB",        /* 0xbb */
  "reserved0BC",        /* 0xbc */
  "reserved0BD",        /* 0xbd */
  "reserved0BE",        /* 0xbe */
  "texChipSel",         /* 0xbf */
  "textureMode",        /* 0xc0 */
  "tLOD",               /* 0xc1 */
  "tDetail",            /* 0xc2 */
  "texBaseAddr",        /* 0xc3 */
  "texBaseAddr1",       /* 0xc4 */
  "texBaseAddr2",       /* 0xc5 */
  "texBaseAddr38",      /* 0xc6 */
  "trexInit0",          /* 0xc7 */
  "trexInit1",          /* 0xc8 */
  "nccTable0_0",        /* 0xc9 */
  "nccTable0_1",        /* 0xca */
  "nccTable0_2",        /* 0xcb */
  "nccTable0_3",        /* 0xcc */
  "nccTable0_4",        /* 0xcd */
  "nccTable0_5",        /* 0xce */
  "nccTable0_6",        /* 0xcf */
  "nccTable0_7",        /* 0xd0 */
  "nccTable0_8",        /* 0xd1 */
  "nccTable0_9",        /* 0xd2 */
  "nccTable0_A",        /* 0xd3 */
  "nccTable0_B",        /* 0xd4 */
  "nccTable1_0",        /* 0xd5 */
  "nccTable1_1",        /* 0xd6 */
  "nccTable1_2",        /* 0xd7 */
  "nccTable1_3",        /* 0xd8 */
  "nccTable1_4",        /* 0xd9 */
  "nccTable1_5",        /* 0xda */
  "nccTable1_6",        /* 0xdb */
  "nccTable1_7",        /* 0xdc */
  "nccTable1_8",        /* 0xdd */
  "nccTable1_9",        /* 0xde */
  "nccTable1_A",        /* 0xdf */
  "nccTable1_B",        /* 0xe0 */
  "reserved0E1",        /* 0xe1 */
  "reserved0E2",        /* 0xe2 */
  "reserved0E3",        /* 0xe3 */
  "reserved0E4",        /* 0xe4 */
  "reserved0E5",        /* 0xe5 */
  "reserved0E6",        /* 0xe6 */
  "reserved0E7",        /* 0xe7 */
  "reserved0E8",        /* 0xe8 */
  "reserved0E9",        /* 0xe9 */
  "reserved0EA",        /* 0xea */
  "reserved0EB",        /* 0xeb */
  "reserved0EC",        /* 0xec */
  "reserved0ED",        /* 0xed */
  "reserved0EE",        /* 0xee */
  "reserved0EF",        /* 0xef */
  "reserved0F0",        /* 0xf0 */
  "reserved0F1",        /* 0xf1 */
  "reserved0F2",        /* 0xf2 */
  "reserved0F3",        /* 0xf3 */
  "reserved0F4",        /* 0xf4 */
  "reserved0F5",        /* 0xf5 */
  "reserved0F6",        /* 0xf6 */
  "reserved0F7",        /* 0xf7 */
  "reserved0F8",        /* 0xf8 */
  "reserved0F9",        /* 0xf9 */
  "reserved0FA",        /* 0xfa */
  "reserved0FB",        /* 0xfb */
  "reserved0FC",        /* 0xfc */
  "reserved0FD",        /* 0xfd */
  "reserved0FE",        /* 0xfe */
  "reserved0FF"         /* 0xff */
};
#endif /* GDBG_INFO_ON||GLIDE_DEBUG */

/*---------------------------------------------------------------------------
** _grDebugGroupWriteHeader
*/
#if defined(GDBG_INFO_ON) && (GLIDE_PLATFORM & GLIDE_HW_SST96)
void
_grDebugGroupWriteHeader(FxU32 header, FxU32 address) 
{
#define FN_NAME "_grDebugGroupWriteHeader"
  GR_DCL_GC;
  FxU32
    offset,
    /* laddress,
     * chipField, */
    lheader = header;
  int
    index,
    nBits = 0;

  GR_ASSERT(address & GWH_ENABLE_BIT);

  GDBG_INFO((128, "New Group Write Packet ======================\n"));
  GDBG_INFO((128, "Address:      0x%x\n", address));
  GDBG_INFO((128, "Header:       0x%x\n", header));
  GDBG_INFO((128, "PCI Address:  0x%x\n", (address & 0xfffff) << 2));
  GDBG_INFO((128, "Chip Field:   0x%x\n", (address >> 14) & 0xf));

  offset = (address & ~(0xf << 14)) & 0xfffff;
  index = offset;

  if (offset < 0x100 ) { /* It's state or triangle */

    GDBG_INFO((128, "Start:     0x%s (0x%x)\n", regNames[index], index));
    GDBG_INFO((128, "Mask:      0x%x\n", header));

    while (lheader) {
      if (lheader & 0x1) {
        nBits++;
        GDBG_INFO((128, "Includes:\t%s (0x%x)\n",
                   regNames[index], index));
      }
      lheader >>= 1;
      index++;
    }
  } else if (offset >= 0x10000) { /* It's texture */
    GDBG_INFO((128, "Mask:      0x%x\n", header));
    GDBG_INFO((128, "TEXTURE\n"));
    do {
      if (lheader & 0x1)
        nBits++;
    } while (lheader >>=1 );
  } else {                      /* WTF? */
    GDBG_INFO((128, "Group Write Header:\t????\n"));
    nBits = 1;
  }

  GDBG_INFO((128, "%s:  %d bits of mask 0x%x were set\n", FN_NAME,
             nBits, header));

  GR_ASSERT(nBits);

#undef FN_NAME
} /* _grDebugGroupWriteHeader */

#ifdef SST96_FIFO
void
_grFifoWriteDebug(FxU32 addr, FxU32 val, FxU32 fifoPtr)
{
  GR_DCL_GC;
  FxU32
    index = GEN_INDEX(addr);
  const char
    *regName;

  regName = (index <= 0xff) ? regNames[index] : "TRAM";

  if (index <= 0xff) { 
    GDBG_INFO((120, "Storing to FIFO:\n"));
    GDBG_INFO((120, "\tReg Name:    %s\n", regName));
    GDBG_INFO((120, "\tReg Num:     0x%x\n", index));
    GDBG_INFO((120, "\tValue:       0x%x\n", val));
    GDBG_INFO((120, "\tFIFO Ptr:    0x%x\n", fifoPtr));
    GDBG_INFO((120, "\tSince Fence: 0x%x\n", gc->hwDep.sst96Dep.writesSinceFence));
    GDBG_INFO((120, "\tFIFO Test:    0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoSize));
  }
} /* _grFifoWriteDebug */

void
_grFifoFWriteDebug(FxU32 addr, float val, FxU32 fifoPtr)
{
  GR_DCL_GC;
  FxU32
    index = GEN_INDEX(addr);
  const char
    *regName;

  if (index <= 0xff) {

    regName = regNames[index];

    GDBG_INFO((120, "Storing to FIFO:\n"));
    GDBG_INFO((120, "\tReg Name:    %s\n", regName));
    GDBG_INFO((120, "\tReg Num:     0x%x\n", index));
    GDBG_INFO((120, "\tValue:       %4.2f\n", val));
    GDBG_INFO((120, "\tFIFO Ptr:    0x%x\n", fifoPtr));
    GDBG_INFO((120, "\tFIFO Test:    0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoSize));
  } else {
    GDBG_INFO((195, "Storing to FIFO:\n"));
    GDBG_INFO((195, "\tTEX Addr:    0x%x\n", index));
    GDBG_INFO((195, "\tTexel:       %4.2f\n", val));
    GDBG_INFO((195, "\tFIFO Ptr:    0x%x\n", fifoPtr));
    GDBG_INFO((120, "\tFIFO Test:    0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoSize));
  }
} /* _grFifoFWriteDebug */
#endif /* (SST96_FIFO) */
#endif /* (GDBG_INFO_ON & (GLIDE_PLATFORM & GLIDE_HW_SST96)) */


/*--------------------------------------------------------------------------
** _grSst96PCIFifoEmpty 
*/
FxBool
_grSst96PCIFifoEmpty() {
  GR_DCL_GC;
  GR_DCL_HW;

  int
    count = 0;

#define COUNT_PATIENCE_VALUE  100000000
#define SST96_PCI_FIFO_EMPTY(a) ((((a->status) & 0x3f00) >> 8) == 0x3f)
  for (;;) {
    if (SST96_PCI_FIFO_EMPTY(hw))
      return FXTRUE;

    if (!(++count % COUNT_PATIENCE_VALUE)) {
      GDBG_INFO((120, "Losing Patients after %d tries\n", COUNT_PATIENCE_VALUE));
      break;
    }
  }
  return FXFALSE;
} /* _grSst96PCIFifoEmpty */


void GR_CDECL
_grSst96FifoMakeRoom(void) 
{
  GR_DCL_GC;
#ifdef SST96_ALT_FIFO_WRAP
  /* NB: W/ the alternate fifo wrapping stuff on only the asm
   * tri-setup uses this routine.  
   */
  gc->fifoData.hwDep.vg96FIFOData.blockSize = _GlideRoot.curTriSize;
#endif

  /* Wrap Fifo now if triangle is going to incur a wrap */
  initWrapFIFO(&gc->fifoData);
} /* _grSst96FifoMakeRoom */

/*--------------------------------------------------------------------------
** _grSst96Load32
*/
FxU32
_grSst96Load32(FxU32 *s) {
  #if GDBG_INFO_ON
  GR_DCL_GC;
  FxU32 regVal = *s;
  FxU32 index = GEN_INDEX(s);
  if (index <= 0xff) {
    const char *regName = regNames[index];
    GDBG_INFO((120, "Direct Register Read:\n"));
    GDBG_INFO((120, "\tReg Name:        %s\n", regName));
    GDBG_INFO((120, "\tReg Num:         0x%x\n", index));
    GDBG_INFO((120, "\tReg Val:         0x%x\n", regVal));
  }
  return regVal;
  #else
  return *s;
  #endif
} /* _grSst96Load32 */

/*---------------------------------------------------------------------------
** _gr96SstStore32
*/
void
_grSst96Store32(FxU32 *d, FxU32 s) {
  GR_DCL_GC;

#ifdef SST96_FIFO
  GLIDE_FIFO_CHECK();
  {
    FxI32 fifoSize = gc->fifoData.hwDep.vg96FIFOData.fifoSize - 8;
    if (fifoSize < 0) {
#if SST96_ALT_FIFO_WRAP
      gc->fifoData.hwDep.vg96FIFOData.blockSize = (sizeof(FxU32) << 1);
      initWrapFIFO(&gc->fifoData);
#else
      _grSst96FifoMakeRoom();
#endif
    }
  }

  SST96_STORE_FIFO( d, s );
  GLIDE_FIFO_CHECK();
#else /* !SST96_FIFO */

  #if GDBG_INFO_ON
  FxU32 index = GEN_INDEX(d);
  if (index <= 0xff) {
    const char *regName = (index <= 0xff) ? regNames[index] : "TRAM";
    GDBG_INFO((120, "Direct Rester Write:\n"));
    GDBG_INFO((120, "\tReg Name:        %s\n", regName));
    GDBG_INFO((120, "\tReg Num:         0x%x\n", index));
    GDBG_INFO((120, "\tValue:           0x%x\n", s));
  }
  #endif
  if (_grSst96PCIFifoEmpty() == FXFALSE) {
    GDBG_INFO((120, "ERROR:  Fifo didn't empty\n"));
  }

  *d = s;
#endif /* !SST96_FIFO */
} /* _gr96SstStore32 */

/*---------------------------------------------------------------------------
** _grSst96Store32F
*/
void
_grSst96Store32F(float *d, float s) {
#ifdef SST96_FIFO
  GLIDE_FIFO_CHECK();
#else

  #if GDBG_INFO_ON
  FxU32 index = GEN_INDEX(d);/* Offset into reg name array */
  if (index <= 0xff) {
    const char *regName = regNames[index];
    GDBG_INFO((120, "Direct Register Write:\n"));
    GDBG_INFO((120, "\tReg Name:        %s\n", regName));
    GDBG_INFO((120, "\tReg Num:         0x%x\n", index));
    GDBG_INFO((120, "\tValue:           %4.4f\n", s));
  }
  #endif
  *d = s;

  if (_grSst96PCIFifoEmpty() == FXFALSE) {
    GDBG_INFO((120, "ERROR:  Fifo didn't empty\n"));
  }
#endif
} /* _grSst96Store32F */

/*---------------------------------------------------------------------------
** _grSst96Store16
*/
void
_grSst96Store16(FxU16 *d, FxU16 s) {
  FxU32 dummy;

  dummy = (FxU32) s;
  _grSst96Store32((FxU32 *) d, dummy);
} /* _grSst96Store16 */

#undef GETINDEX
#undef GETREADADDR
#undef GR_GET
#define GR_GET(s) *(GETREADADDR(s))
#define GETINDEX(a) ((((FxU32) &a) - (((FxU32) hw) + 0x400000)) >> 2)
#define GETREADADDR(s) ((FxU32 *) (((FxU32) hw) + (GETINDEX(s) <<12)))

#define STATUS_MEMFIFOERR       FXBIT(15)
#define STATUS_CMDFIFOERR       FXBIT(14)

#define FIFO_TOP_BYTES(a) ((((a) >> 11) & 0x3ff) << 12)
#define FIFO_BOT_BYTES(a) ((((a) >> 1) & 0x3ff) << 12)
#define FIFO_RD_BYTES(a) (((a) << 3) & 0x7ffff)

/*---------------------------------------------------------------------------
** _grSst96CheckFifoData
*/
void
_grSst96CheckFifoData()
{
#define FN_NAME "_grSst96CheckFifoData"
  GR_DCL_GC;
  GR_DCL_HW;

  FxU32
    /* Data for error checking */
    fifoStartGC,                /* GC's idea of FIFO bottom */
    fifoStartHW,                /* Jr's idea of FIFO bbottom */
    fifoWrPtrGC,                /* GC's idea of write pointer */
    fifoRdPtrHW,                /* Jr's idea of read pointer */
    fifoTopHW,                  /* Jr's idea of top of FIFO */
    /* Register contents */
    status,
    fbiJrInit3,
    fbiJrInit4,
    fbiJrInit5;

  FxBool
    error = FXFALSE;

  if (!gc->open) return;

  /*
  **  Get the relavent data from the GC and from Jr.
  */

  /* Jr. */
  status = GR_GET(hw->status);
  fbiJrInit3 = GR_GET(hw->fbijrInit3);
  fbiJrInit4 = GR_GET(hw->fbijrInit4);
  fbiJrInit5 = GR_GET(hw->fbijrInit5);

  /* GC */
  fifoStartGC = (FxU32) gc->fifoData.hwDep.vg96FIFOData.fifoVirt - (FxU32) gc->base_ptr;
  fifoWrPtrGC = (FxU32) gc->fifoData.hwDep.vg96FIFOData.fifoPtr - (FxU32) gc->base_ptr;
  
  /* Deduce relevant data from registers */
  fifoStartHW = FIFO_BOT_BYTES(fbiJrInit3);
  fifoTopHW = FIFO_TOP_BYTES(fbiJrInit3) + 0x1000;
  fifoRdPtrHW = FIFO_RD_BYTES(fbiJrInit5);

  /*
  **  Perform Error Checking
  */

  /* Check for errors indicated in status bits */
  if (status & STATUS_MEMFIFOERR) {
    GDBG_PRINTF(("FIFO ERROR:  Status 0x%x indicates MEMFIFO error!\n", status));
    error = FXTRUE;
  }

  if (status & STATUS_CMDFIFOERR) {
    GDBG_PRINTF(("FIFO ERROR:   Status 0x%x indicates CMDFIFO error\n"));
    error = FXTRUE;
  }

  /* Check for proper concept of FIFO bottom */
  if (fifoStartGC != fifoStartHW) {
    GDBG_PRINTF(("FIFO_ERROR:   fbijrInit3 0x%x: FIFO Bottom  0x%x != 0x%x\n",
               fbiJrInit3, fifoStartGC, fifoStartHW));
    error = FXTRUE;
  }

  /* Check for write past end of FIFO */
  if (fifoWrPtrGC > fifoTopHW) {
    GDBG_PRINTF(("FIFO ERROR:  Top of FIFO: 0x%x, Write Ptr: 0x%x\n",
                fifoTopHW, fifoWrPtrGC));
    error = FXTRUE;
  }

  if ((fbiJrInit4 > (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoMax) ||
      (fbiJrInit4 >= 0x10000UL)) {
    GDBG_PRINTF(("FIFO Error: Entry count overflow: 0x%X 0x%X\n",
                 fbiJrInit4, gc->fifoData.hwDep.vg96FIFOData.fifoMax));
    error = FXTRUE;
  }

  if (error) {
    GrErrorCallback("FATAL FIFO ERROR (See above)\n", FXTRUE);
  }

#undef FN_NAME
} /* _grSst96CheckFifoData */

#endif /* (GLIDE_PLATFORM & GLIDE_HW_SST96) */
