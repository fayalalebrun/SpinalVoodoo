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
** 3     4/06/99 3:36p Dow
** Alt tab stuff
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

/*
**  I/O Macros
*/
#define HWC_IO_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);\
((SstIORegs *) regInfo.ioMemBase)->reg = val

#define HWC_IO_LOAD(regInfo, reg, val)\
val = ((SstIORegs *) regInfo.ioMemBase)->reg;\
GDBG_INFO(120, "Loaded 0x%x from IO Register %s\n", val,\
  ioRegNames[(offsetof(SstIORegs, reg)) >> 2]);

#define HWC_CAGP_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to CAGP Register %s\n", val,\
  cmdAGPRegNames[(offsetof(SstCRegs, reg)) >> 2]);\
((SstCRegs *) (regInfo.cmdAGPBase))->reg = val

#define HWC_CAGP_LOAD(regInfo, reg, val)\
val = ((SstCRegs *) (regInfo).cmdAGPBase)->reg;\
GDBG_INFO(120, "Loaded 0x%x from CAGP Register %s\n", val,\
  cmdAGPRegNames[(offsetof(SstCRegs, reg)) >> 2]);
  
#define HWC_WAX_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to WAX Register %s\n", val,\
  waxRegNames[(offsetof(SstGRegs, reg)) >> 2]);\
((SstGRegs *) regInfo.waxBase)->reg = val

#define HWC_WAX_LOAD(regInfo, reg, val)\
val = ((SstGRegs *) regInfo.waxBase)->reg;\
GDBG_INFO(120, "Loaded 0x%x from WAX Register %s\n", val,\
  waxRegNames[(offsetof(SstGRegs, reg)) >> 2]);

#define HWC_SST_STORE(regInfo, reg, val)\
GDBG_INFO(120, "Storing 0x%x to 3D Register %s\n", val,\
 sstRegNames[(offsetof(SstRegs, reg)) >> 2]);\
((SstRegs *) regInfo.sstBase)->reg = val

#define HWC_SST_LOAD(regInfo, reg, val)\
val = ((SstRegs *) regInfo.sstBase)->reg;\
GDBG_INFO(120, "Loaded 0x%x from WAX Register %s\n", val,\
 sstRegNames[(offsetof(SstRegs, reg)) >> 2]);

#endif /* HWCIO_H */
