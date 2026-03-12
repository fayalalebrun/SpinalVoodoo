/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if USE_XDGA_SWITCH
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
static Display *dpy       = 0;
static XDGADevice *dev    = 0;
static XDGAMode *vidModes = 0;
#endif

#include <minihwc.h>
#include <hwcio.h>
#include "h3cinit.h"
#include "glide.h"
#include "fxglide.h"


#define CFG_FILE "/etc/conf.3dfx/voodoo3"

#if 1
#define LOG(x)
#else
#define LOG(x) \
    do { \
	FILE *dbg = fopen("debug.txt","a"); \
	fprintf x ; \
	fclose(dbg); \
    } while(0);
#endif

static struct envitem_t {
       char *env;
       char *val;
       struct envitem_t *next;
} *first = NULL;


static FxU32 saveVidProcCfg;
static FxU32 saveLfbMemoryConfig;
static FxU32 saveMiscInit0;


FxBool setVideoMode(void *hwnd,
                    int  xRes,
                    int  yRes,
                    int  h3pixelSize,
                    int  refresh,
                    void *hmon,
                    char *regpath,
                    char *devicename)
{
 GR_DCL_GC;
 hwcBoardInfo *bInfo = gc->bInfo;

#if USE_XDGA_SWITCH
 int eventbase, errorbase;
 int i, numModes;

 dpy=XOpenDisplay("");
 if (!dpy) {
    GDBG_INFO(80, "Failed to open X display\n");
    return FXFALSE;
 }

 if (XDGAQueryExtension(dpy, &eventbase, &errorbase)) {
    int major, minor;
    XDGAQueryVersion(dpy, &major, &minor);
    if (major>1 && minor<0) {
       XCloseDisplay(dpy);
       GDBG_INFO(80, "Banshee/V3 requires DGA version 1.x\n");
       return FXFALSE;
    }
 }

 vidModes = XDGAQueryModes(dpy, DefaultScreen(dpy), &numModes);
  
 for (i=0; i<numModes; i++)
     if ((vidModes[i].viewportWidth==xRes) &&
         (vidModes[i].viewportHeight==yRes))
        break;
 if (i==numModes) {
    GDBG_INFO(80, "Server doesn't support requested resolution\n");
    vidModes=0;
    return FXFALSE;
 }
 if ((dev = XDGASetMode(dpy, DefaultScreen(dpy), vidModes[i].num)) == NULL) {
    GDBG_INFO(80, "Failed to set required video mode\n");
    return FXFALSE;
 }
 XDGASync(dpy, False);
#else
 if (!h3InitSetVideoMode(bInfo->regInfo.ioPortBase, xRes, yRes, refresh, FXTRUE)) {
    GDBG_INFO(80, "Setmode failed --  unimplemented resolution\n");
    return FXFALSE;
 }
#endif

 HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
 HWC_IO_LOAD(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
 HWC_IO_LOAD(bInfo->regInfo, miscInit0, saveMiscInit0);

 return FXTRUE;
} /* setVideoMode */


void resetVideo (void)
{
 GR_DCL_GC;
 hwcBoardInfo *bInfo = gc->bInfo;

 HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
 HWC_IO_STORE(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
 HWC_IO_STORE(bInfo->regInfo, miscInit0, saveMiscInit0);

#if USE_XDGA_SWITCH
  if (dpy) {
     if (!XDGASetMode(dpy, DefaultScreen(dpy), 0)) {
        GDBG_INFO(80, "Failed to return to previous video mode\n");
     }
     vidModes = NULL;
     XCloseDisplay(dpy);
     dpy = NULL;
     XFree(dev);
  }
#else
 /* XXX to do */
#endif

 GDBG_INFO(80, "resetVideo(): \n");
} /* resetVideo */


FxBool checkResolutions (FxBool *reslist, void *hmon)
{
  /* [dBorca] this should be tied to cinit code (or at least DGA)
   * FxU16 *h3InitFindVideoMode (FxU32 xRes, FxU32 yRes, FxU32 refresh)
   */
  int res;

  for (res = GR_MIN_RESOLUTION; res <= GR_MAX_RESOLUTION; res++) {
    reslist[res] = FXTRUE;
  }

  return FXTRUE;
}


static void loadEnvFile (void)
{
 FILE *file;
 char data[256];
 char *env, *val;
 struct envitem_t *item;
 FxBool sawError = FXFALSE;
 static FxBool envinit = FXFALSE;

 if (envinit) {
    return;
 }
 envinit = FXTRUE;
 if ((file = fopen(CFG_FILE, "r")) == NULL) {
    return;
 }
 while (fgets(data, 255, file) != NULL) {
       if ((*data=='#') || (*data=='\n')) {
          continue;
       }
       if ((val = strchr(data, '=')) == NULL) {
          if (!sawError) {
             sawError = FXTRUE;
             fprintf(stderr, "In config file " CFG_FILE ":\n");
          }
          fprintf(stderr, "Malformed line: %s\n", data);
          continue;
       }
       *val++ = '\0';
       env = data;
       item = malloc(sizeof(struct envitem_t));
       item->env = malloc(strlen(env)+1);
       strcpy(item->env, env);
       item->val = malloc(strlen(val)+1);
       strcpy(item->val, val);
       item->next = first;
       first = item;
 }
 fclose(file);
}


char *file_getenv (const char *a)
{
 struct envitem_t *ptr;
 char *result;

 if ((result = getenv(a)) != NULL) {
    return result;
 }
 loadEnvFile();
 for (ptr = first; ptr != NULL; ptr = ptr->next) {
     if (!strcmp(ptr->env, a)) {
        return ptr->val;
     }
 }

 return NULL;
}

