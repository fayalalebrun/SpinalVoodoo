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
*/

#ifndef _INITVGA_H_
#define _INITVGA_H_

#if defined(__WATCOMC__)
#define _inp inp
#define _outp outp
#define _outpw outpw
#endif

/* 
** Some Standard VGA Registers 
*/
/* Reading */
#define VGA_MISC_OUTPUT_READ    0x3cc

/* Writing */
#define VGA_MISC_OUTPUT_WRITE    0x3c2

/* Read/Write */
#define VGA_REGISTER_INPUT_STATUS_1_MONO                    0x3BA
#define VGA_REGISTER_INPUT_STATUS_1_COLOR                   0x3DA
#define VGA_INPUT_STATUS_1C                     0x3DA
#define VIS1C_PIXEL_DISPLAY_INACTIVE            BIT(0)
#define VIS1C_VERTICAL_RETRACE_ACTIVE           BIT(3)

#define VGA_REGISTER_CRTC                                   0x3D4
#define VR_CRTC_SERIAL_START_ADDRESS_HI_INDEX               0x0C
#define VR_CRTC_SERIAL_START_ADDRESS_LO_INDEX               0x0D
#define AR_CRTC_SERIAL_OVERFLOW_INDEX                       0x1C

/* General Port I/O */
#ifdef GDBG_INFO_ON

#define OUTP(port,val)\
GDBG_INFO((80, "%s:  Writing 0x%x to port 0x%x\n", FN_NAME, val, port));\
_outp(port, val)

#define OUTPW(port,val)\
GDBG_INFO((80, "%s:  Writing 0x%x to port 0x%x\n", FN_NAME, val, port));\
_outpw(port, val)


#define INP(port, val)\
val = _inp(port);\
GDBG_INFO((80, "%s:  Read 0x%x from port 0x%x\n", FN_NAME, val, port))

#define INPW(port, val)\
val = _inpW(port);\
GDBG_INFO((80, "%s:  Read 0x%x from port 0x%x\n", FN_NAME, val, port))

#else

#define OUTP(port, val) _outp(port, val)
#define INP(port, val)  val = _inp(port)

#define OUTPW(port, val) _outpw(port, val)
#define INPW(port, val)  val = _inpw(port)

#endif

/* Macros for Sequencer registers */
#define SEQU_INDEX               0x3c4
#define SEQU_DATA                0x3c5

/* Macros for CRTC registers */
#define CRTC_INDEX              0x3d4
#define CRTC_DATA               0x3d5

#ifdef GDBG_INFO_ON

#define SEQU_SET(reg,val)\
GDBG_INFO((80, "%s:  Writing 0x%x to Sequencer Index 0x%x\n", FN_NAME, val, reg));\
_outp(SEQU_INDEX, reg); _outp(SEQU_DATA, val)
#define SEQU_GET(reg,val)\
_outp(SEQU_INDEX, reg);val = _inp(SEQU_DATA);\
GDBG_INFO((80, "%s:  Read 0x%x from Sequencer Index 0x%x\n", val, reg));

#define CRTC_SET(reg,val)\
GDBG_INFO((80, "%s:  Writing 0x%x to CRTC Index 0x%x\n", FN_NAME, val, reg));\
_outp(CRTC_INDEX, reg); _outp(CRTC_DATA, val)
#define CRTC_GET(reg,val)\
_outp(CRTC_INDEX, reg); val = _inp(CRTC_DATA);\
GDBG_INFO((80, "%s:  Read 0x%x from CRTC Index 0x%x\n", FN_NAME, val, reg))

#else

#define SEQU_SET(reg,val) _outp(SEQU_INDEX, reg); _outp(SEQU_DATA, val)
#define SEQU_GET(reg,val) _outp(SEQU_INDEX, reg); val = _inp(SEQU_DATA)

#define CRTC_SET(reg,val) _outp(CRTC_INDEX, reg); _outp(CRTC_DATA, val)
#define CRTC_GET(reg,d)   _outp(CRTC_INDEX, reg); d = _inp(CRTC_DATA)

#endif


#endif /* _INITVGA_H_ */
