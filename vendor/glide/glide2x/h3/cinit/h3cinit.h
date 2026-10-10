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


#ifndef __H3CINIT_H__
#define __H3CINIT_H__

#include <3dfx.h>

#define H3_GRXCLK_SPEED 100
#define H4_GRXCLK_SPEED 143
#define H4_OEM_GRXCLK_SPEED 141
#define H4_BRINGUP_GRXCLK_SPEED 100

#ifdef H4
#define DEFAULT_GRXCLK_SPEED H4_BRINGUP_GRXCLK_SPEED
#else
#define DEFAULT_GRXCLK_SPEED H3_GRXCLK_SPEED
#endif

FxU32                             // return # of MB of memory
h3InitGetMemSize(FxU32 regBase);  // init register base

FxU32                           // return # of MB of memory
h3InitSgram(FxU32 regBase,      // init iegister base
            FxU32 sgramMode,
            FxU32 sgramMask,
            FxU32 sgramColor,
            char *vendorName);  // NULL or name of SGRAM vendor

void
h3InitPlls(FxU32 regBase,               // init iegister base
           FxU32 grxSpeedInMHz,         // desired GRX clock frequency (MHz)
           FxU32 memSpeedInMHz);        // desired MEM clock frequency (MHz)

void
h4InitPlls(FxU32 regBase,               // init register base
           FxU32 deviceID,              // H4 or H4_OEM
           FxU32 grxSpeedInMHz);        // desired clock frequency (MHz)

void 
h3InitVga(
    FxU32 regBase,              // memory base address
    FxU32 legacyDecode);        // 1=enable VGA decode, 0=disable

void
h3InitVideoProc(
    FxU32 regBase,              // memory base address
    FxU32 vidProcCfg);          // vidProcCfg register control bits

FxBool
h3InitSetVideoMode(
    FxU32 regBase,              // memory base address
    FxU32 xRes,                 // x resolution
    FxU32 yRes,                 // y resolution
    FxU32 refresh,              // refresh freq
#if defined(H3VDD) && defined(H3_B0)
  FxU32 loadClut, // really a bool, should we load the lookup table
  FxU32 scanlinedouble); // set scanline double bit and double y?
#else
    FxU32 loadClut) ;           // initialize clut entries?
#endif

void
h3InitVideoDesktopSurface(
    FxU32 regBase,
    FxU32 enable,               // 1=enable desktop surface (DS), 1=disable
    FxU32 tiled,                // 0=DS linear, 1=tiled
    FxU32 pixFmt,               // pixel format of DS
    FxU32 clutBypass,           // bypass clut for DS?
    FxU32 clutSelect,           // 0=lower 256 CLUT entries, 1=upper 256
    FxU32 startAddress,         // board address of beginning of DS
    FxU32 stride);              // distance between scanlines of the DS, in
                                // units of bytes for linear DS's and tiles for
                                // tiled DS's

void
h3InitVideoOverlaySurface(
    FxU32 regBase,
    FxU32 enable,               // 1=enable Overlay surface (OS), 1=disable
    FxU32 stereo,               // 1=enable OS stereo, 0=disable
    FxU32 horizScaling,         // 1=enable horizontal scaling, 0=disable
    FxU32 dudx,                 // horizontal scale factor (ignored if not
                                // scaling)
    FxU32 verticalScaling,      // 1=enable vertical scaling, 0=disable
    FxU32 dvdy,                 // vertical scale factor (ignored if not
                                // scaling)
    FxU32 filterMode,           // duh
    FxU32 tiled,                // 0=OS linear, 1=tiled
    FxU32 pixFmt,               // pixel format of OS
    FxU32 clutBypass,           // bypass clut for OS?
    FxU32 clutSelect,           // 0=lower 256 CLUT entries, 1=upper 256
    FxU32 startAddress,         // board address of beginning of OS
    FxU32 stride);              // distance between scanlines of the OS, in
                                // units of bytes for linear OS's and tiles for
                                // tiled OS's

#ifndef H3VDD
void
h3InitMeasureSiProcess(
    FxU32 regBase);             // init register base
#endif // #ifndef H3VDD

void
h3InitBlockWrite(
    FxU32 regBase,
    FxU32 enable,               // 1=enable block writes, 0=disable
    FxU32 threshhold);          // block write threshhold

void
h3InitResetAll(
    FxU32 regBase);             // init register base

#endif /*  __H3CINIT_H__ */

