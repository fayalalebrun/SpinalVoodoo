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
** $Log: 
**  5    3dfx      1.2.1.0.1.0 10/11/00 Brent           Forced check in to enforce
**       branching.
**  4    3dfx      1.2.1.0     06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  3    3dfx      1.2         02/14/00 Kenneth Dyke    Added macro to grab slave
**       IO registers.
**  2    3dfx      1.1         02/07/00 Kenneth Dyke    MacOS & PC fixes for
**       multi-chip anti-aliasing.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 3     6/03/99 4:53p Kcd
** hwcio.h is now endian-independant.
** 
** 2     3/11/98 8:27p Dow
** WinGlide
** 
** 1     3/04/98 4:13p Dow
**
*/
#ifndef HWCIO_H
#define HWCIO_H

extern const char *ioRegNames[];
extern const char *cmdAGPRegNames[];
extern const char *waxRegNames[];
extern const char *sstRegNames[];

#ifndef GET
#  define GET(s) s
#  define SET(d, s)    d = s
#endif

#if SET_BSWAP
#undef GET
#undef SET

#if __POWERPC__ && (defined(__MWERKS__) || defined(__MRC__))
#define GET(s)               __lwbrx( (void*)&(s), 0 )
#define SET(d, s)            __stwbrx((s), (void*)&(d), 0)
#else
#error Need GET/SET Macros for this architecture/compiler!
#endif
#endif

/*
**  I/O Macros
*/
#define HWC_IO_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);\
SET(((SstIORegs *) regInfo.ioMemBase)->reg,val)

#define HWC_IO_STORE_SLAVE(chipNumber, regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);\
SET(((SstIORegs *) regInfo.slaveIOBase[chipNumber-1])->reg,val)

#define HWC_IO_LOAD(regInfo, reg, val)\
val = GET(((SstIORegs *) regInfo.ioMemBase)->reg);\
GDBG_INFO(120, "Loaded 0x%x from IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);

#define HWC_IO_LOAD_SLAVE(chipNumber, regInfo, reg, val)\
val = GET(((SstIORegs *) regInfo.slaveIOBase[chipNumber-1])->reg);\
GDBG_INFO(120, "Loaded 0x%x from IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);

#define HWC_CAGP_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to CAGP Register %s\n", val,\
  cmdAGPRegNames[(offsetof(SstCRegs, reg)) >> 2]);\
SET(((SstCRegs *) (regInfo.cmdAGPBase))->reg,val)

#define HWC_CAGP_LOAD(regInfo, reg, val)\
val = GET(((SstCRegs *) (regInfo).cmdAGPBase)->reg);\
GDBG_INFO(120, "Loaded 0x%x from CAGP Register %s\n", val,\
  cmdAGPRegNames[(offsetof(SstCRegs, reg)) >> 2]);
  
#define HWC_WAX_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to WAX Register %s\n", val,\
  waxRegNames[(offsetof(SstGRegs, reg)) >> 2]);\
SET(((SstGRegs *) regInfo.waxBase)->reg,val)

#define HWC_WAX_LOAD(regInfo, reg, val)\
val = GET(((SstGRegs *) regInfo.waxBase)->reg);\
GDBG_INFO(120, "Loaded 0x%x from WAX Register %s\n", val,\
  waxRegNames[(offsetof(SstGRegs, reg)) >> 2]);

#define HWC_SST_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to 3D Register %s\n", val,\
 sstRegNames[(offsetof(SstRegs, reg)) >> 2]);\
SET(((SstRegs *) regInfo.sstBase)->reg,val)

#define HWC_SST_STORE_SLAVE(chipNumber, regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to 3D Register %s\n", val,\
 sstRegNames[(offsetof(SstRegs, reg)) >> 2]);\
SET(((SstRegs *) regInfo.slaveSstBase[chipNumber-1])->reg,val)

#define HWC_SST_LOAD(regInfo, reg, val)\
val = GET(((SstRegs *) regInfo.sstBase)->reg);\
GDBG_INFO(120, "Loaded 0x%x from WAX Register %s\n", val,\
 sstRegNames[(offsetof(SstRegs, reg)) >> 2]);

#endif /* HWCIO_H */
