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
#include <sst.h>
#include <sst1vid.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1init.h>

#ifdef INIT_OUTPUT

FX_ENTRY void FX_CALL sst1InitVPrintf(const char *, va_list);

FX_ENTRY void FX_CALL sst1InitPrintf(const char *format, ...)
{
    va_list args;
    static FxBool firstPass = FXTRUE;
    static FxBool printIt = FXFALSE;

    if(firstPass == FXTRUE) {
        const char *envf;
        firstPass = FXFALSE;
	if (sst1InitMsgFile == NULL)
	{
	  /* I couldn't initialize to stdout because stdout is not constant */
	  sst1InitMsgFile = stdout;
	}
        if(GETENV(("SST_INITDEBUG")))
            printIt = FXTRUE;
        envf = GETENV(("SST_INITDEBUG_FILE"));
        if(envf) {
            if((sst1InitMsgFile = fopen(envf, "w")) != NULL)
                printIt = FXTRUE;
            else {
                fprintf(stderr, "sst1InitPrintf(): Could not open file '%s' for logging...\n", envf);
                printIt = FXFALSE;
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

