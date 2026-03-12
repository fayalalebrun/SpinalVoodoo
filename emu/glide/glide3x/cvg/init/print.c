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
** Print functions for SST-1 Initialization routines
*/
#ifdef _MSC_VER
#pragma optimize ("",off)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef BUILD_FOR_SST1
#include <sst.h>
#else
#include <3dfx.h>
#include <cvgregs.h>
#include <cvgdefs.h>
#endif
#include <sst1vid.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1init.h>
#include <gdebug.h>

#ifdef INIT_OUTPUT

FX_ENTRY void FX_CALL sst1InitVPrintf(const char *, va_list);

FX_ENTRY void FX_CALL sst1InitPrintf(const char *format, ...)
{
    va_list args;
    static FxBool firstPass = FXTRUE;
    static FxBool printIt = FXFALSE;

    if(firstPass == FXTRUE) {
        const char *envf;
        sst1InitMsgFile = stdout;
        firstPass = FXFALSE;
        if(GETENV(("SSTV2_INITDEBUG")) || GDBG_GET_DEBUGLEVEL(5))
            printIt = FXTRUE;
        envf = GETENV(("SSTV2_INITDEBUG_FILE"));
        if(envf) {
          printIt = ((sst1InitMsgFile = fopen(envf, "w")) != NULL);
          if (!printIt) {
            fprintf(stderr, "sst1InitPrintf(): Could not open file '%s' for logging...\n", 
                    envf);
          }
        }
    }

    if(printIt == FXFALSE)
        return;

    va_start(args, format);
    sst1InitVPrintf(format, args);
    va_end(args);
}

FX_ENTRY void FX_CALL sst1InitVPrintf(const char *format, va_list args)
{
    vfprintf(sst1InitMsgFile, format, args);
    fflush(sst1InitMsgFile);
}

#endif

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
