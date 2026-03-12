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

#ifdef __DJGPP__
#include <fxdpmi.h>
#endif

#if defined(__WATCOMC__)
#include <conio.h>

#define _inp inp
#define _outp outp

#define _inpw inpw
#define _outpw outpw

#define _inpd inpd
#define _outpd outpd

#endif

#ifdef __linux__
#define _inp pioInByte
#define _outp pioOutByte
#define _inpw pioInWord
#define _outpw pioOutWord
#define _inpd pioInLong
#define _outpd pioOutLong
#endif


#define SSTIOADDR(regName)      ((FxU16)offsetof(SstIORegs, regName))

#define ISET32(addr, value)     _outpd((FxU16) ((FxU16) regBase + (FxU16) (SSTIOADDR(addr))), value)
#define IGET32(addr)            _inpd((FxU16) ((FxU16) regBase + (FxU16) (SSTIOADDR(addr))))

#define ISET8PHYS(a,b) {\
FxU16 port = (FxU16) (regBase) + (FxU16) (a);\
GDBG_INFO(120, "OUT8:  Port 0x%x Value 0x%x\n", port, b);\
_outp(port, (FxU8) (b));}

#define ISET16PHYS(a,b) {\
FxU16 port = (FxU16)(regBase) + (FxU16)(a);\
GDBG_INFO(120, "OUT16:  Port 0x%x Value 0x%x\n", port, b);\
_outpw(port, (FxU16) (b));}

#define IGET8PHYS(a) _inp((FxU16) ((FxU16) (regBase) + (FxU16) (a)))
#define IGET16PHYS(a) _inpw((FxU16) ((FxU16) (regBase) + (FxU16)(a)))

#define CHECKFORROOM while (! (_inp((FxU16) regBase) & (FxU16)(0x3f)))
#define MESSAGE GDBG_PRINTF

#ifdef OLD


#define ISET32(a,b)\
GDBG_INFO(120, "SET32: Register 0x%x Value 0x%x\n", (FxU32) (&((SstIORegs *)regBase)->a) - (FxU32) regBase, b); \
((FxU32) (((SstIORegs *) regBase)->a)) = (FxU32) b

#define IGET32(a) ((FxU32) (((SstIORegs *) regBase)->a))





#endif /* #ifdef OLD */
