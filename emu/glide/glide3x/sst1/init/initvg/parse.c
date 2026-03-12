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
** Parsing code for grabbing information from "voodoo.ini" initialization file
**
*/
#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#ifdef _MSC_VER
#pragma optimize ("",off)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>

#if __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* __WIN32__ */

static int sst1InitFgets(char *, FILE *);
static int sst1InitFgetc(FILE *);
static int sst1InitParseFieldCfg(char *);
static void sst1InitToLower(char *string);
#if TEST || (!defined(INIT_LINUX) && defined(__DOS32__))
static int sst1InitParseFieldDac(char *);
static int sst1InitParseDacRdWrString(char *, sst1InitDacStruct *);
static int sst1InitParseDacRdWr(char *, sst1InitDacRdWrStruct *);
static int sst1InitParseSetVideoString(char *, sst1InitDacStruct *);
static int sst1InitParseSetMemClkString(char *, sst1InitDacStruct *);
static int sst1InitParseSetVideoModeString(char *, sst1InitDacStruct *);
#endif
#if !defined(INIT_LINUX) && defined(__DOS32__)
static void sst1InitFixFilename(char *dst, char *src);
#endif
#if TEST
static void sst1InitPrintDacRdWr(sst1InitDacRdWrStruct *, char *);
#endif

#ifndef INIT_LINUX
/*
** sst1InitVoodooFile():
**  Find and setup "voodoo.ini" file if possible
**
**    Returns:
**      FxTRUE if "voodoo.ini" file is found and opened with no errors
**      FXFALSE if cannot find file or error opening file
**
*/
FX_ENTRY FxBool FX_CALL sst1InitVoodooFile()
{
#ifndef __DOS32__
    return(FXFALSE);
#else
    char filename[256], fixedFilename[512], *tmpPtr;
    char path[512];
    FILE *file = NULL;
    char buffer[1024];
    int inCfg, inDac;
    int i;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    if(getenv("VOODOO_FILE")) {
        /* Override voodoo.ini name */
        strcpy(filename, getenv("VOODOO_FILE"));
        if(!(file = fopen(filename, "r")))
            return(FXFALSE);
    } else {
        /* Override path setting */
        if(getenv("VOODOO_PATH"))
          strcpy(path, getenv("VOODOO_PATH"));
        else if(getenv("PATH")) {
            strcpy(path, ".;");
            strcat(path, getenv("PATH"));
        } else
            strcpy(path, ".;");

        i = 0;
        while(1) {
            if(!i) {
                if((tmpPtr = strtok(path, ";")) == NULL)
                    break;
            } else {
                if((tmpPtr = strtok(NULL, ";")) == NULL)
                    break;
            }
            strcpy(filename, tmpPtr);
            sst1InitFixFilename(fixedFilename, filename);
            if(fixedFilename[strlen(fixedFilename)-1] == '\\')
                sprintf(filename, "%svoodoo.ini", filename);
            else
                sprintf(filename, "%s\\voodoo.ini", filename);
            i++;
            if((file = fopen(filename, "r")))
                break;
        }
    }
    if(file == NULL)
        return(FXFALSE);

    inCfg = inDac = 0;
    while(sst1InitFgets(buffer, file)) {
        buffer[strlen(buffer)-1] = 0;
        if(!strcmp(buffer, "[CFG]")) {
            inCfg = 1; inDac = 0;
            continue;
        } else if (!strcmp(buffer, "[DAC]")) {
            inCfg = 0; inDac = 1;
            continue;
        } else if(buffer[0] == '[') {
            inCfg = 0; inDac = 0;
            continue;
        }
        if(inCfg) {
            if(!sst1InitParseFieldCfg(buffer)) {
                tmpPtr = "[CFG]";
                goto _fail;
            }
        } else if(inDac) {
            if(!sst1InitParseFieldDac(buffer)) {
                tmpPtr = "[DAC]";
                goto _fail;
            }
        }
    }
    fclose(file);
    INIT_PRINTF(("INIT: Using .ini file '%s'\n", filename));
    return(FXTRUE);
 _fail:
    if(helper)
        INIT_PRINTF(("ERROR in %s section of .ini file.\n", tmpPtr));
    fclose(file);
    return(FXFALSE);
#endif
}

#else

/*
** sst1InitVoodooFile():
**  Find and setup "voodoo2.ini" file if possible
**
**    Returns:
**      FxTRUE if "voodoo2.ini" file is found, opened with no errors, and
**             constains the dac programming data.
**      FXFALSE if cannot find file, error opening file, or has no dac data in it.
**
*/
FX_ENTRY FxBool FX_CALL sst1InitVoodooFile() {
  static FxBool retVal = FXFALSE;
  FILE *file = 0;
  char buffer[1024], filename[256];
  char *tmpPtr;
  char path[512];
  int i;

  if (getenv("VOODOO_FILE")) {
    /* Override voodoo.ini name */
    strcpy(filename, getenv("VOODOO_FILE"));
    if (!(file = fopen(filename, "r"))) 
      goto __errExit;
  } else {
    /* Override path setting */
    if (getenv("VOODOO_PATH")) {
      strcpy(path, getenv("VOODOO_PATH"));
    } else {
      strcpy(path, "/etc/conf.3dfx");
    }

    i = 0;
    while(1) {
      if (!i) {
	if ((tmpPtr = strtok(path, ":")) == NULL)
	  break;
      } else {
	if ((tmpPtr = strtok(NULL, ":")) == NULL)
	  break;
      }
      strcpy(filename, tmpPtr);
      if (filename[strlen(filename)-1] == '/')
	sprintf(filename, "%svoodoo", filename);
      else
	sprintf(filename, "%s/voodoo", filename);
      i++;
      if ((file = fopen(filename, "r")))
	break;
    }
  }
  if (!file) {
    retVal = FXFALSE;
    goto __errExit;
  }
  while(sst1InitFgets(buffer, file)) {
    buffer[strlen(buffer)-1] = 0;
    if (buffer[0]=='#') continue;
    if (!sst1InitParseFieldCfg(buffer)) {
      retVal = FXFALSE;
      break;
    }
  }
  INIT_PRINTF(("INIT: Using .ini file '%s'\n", filename));

__errExit:
  if (file) fclose(file);

  return retVal;
}
#endif

#if defined(INIT_DOS) || defined(INIT_LINUX)

/* Get the definition of myGetenv for glide builds */
#include <init.h>

#if TEST
void main(int argc, char **argv)
{
    char buffer[2048]; /* buffer for command line inputs */
    int inCfg, inDac;
    sst1InitEnvVarStruct *envVarsPtr;
    sst1InitDacStruct *dacPtr;
    sst1InitDacSetVideoStruct *setVideo;
    sst1InitDacSetMemClkStruct *setMemClk;
    FILE *file = fopen(argv[1], "r");

    inCfg = inDac = 0;
    while(sst1InitFgets(buffer, file)) {
        buffer[strlen(buffer)-1] = 0;
        if(!strcmp(buffer, "[CFG]")) {
            inCfg = 1; inDac = 0;
            continue;
        } else if (!strcmp(buffer, "[DAC]")) {
            inCfg = 0; inDac = 1;
            continue;
        } else if(buffer[0] == '[') {
            inCfg = 0; inDac = 0;
            continue;
        }
        if(inCfg) {
            if(!sst1InitParseFieldCfg(buffer))
                /* Error processing .ini file */
                exit(1);
        } else if(inDac) {
            if(!sst1InitParseFieldDac(buffer))
                /* Error processing .ini file */
                exit(1);
        }
    }

    /* Dump CFG Data... */
    envVarsPtr = envVarsBase;
    while(envVarsPtr) {
        printf("ENV VAR:%s  VALUE:%s\n", envVarsPtr->envVariable,
            envVarsPtr->envValue);
        envVarsPtr = (sst1InitEnvVarStruct *) envVarsPtr->nextVar;
    }

    /* Dump Dac Data... */
    dacPtr = dacStructBase;
    while(dacPtr) {
        printf("DAC MANU:%s  DEVICE:%s\n", dacPtr->dacManufacturer,
            dacPtr->dacDevice);
        if(dacPtr->detect) {
            printf("\tDetect:\n");
            sst1InitPrintDacRdWr(dacPtr->detect, "\t\t");
        }
        if(dacPtr->setVideo) {
            setVideo = dacPtr->setVideo;
            while(1) {
                printf("\tsetVideo (%dx%d @ %d Hz)\n",
                  setVideo->width, setVideo->height, setVideo->refresh);
                sst1InitPrintDacRdWr(setVideo->setVideoRdWr, "\t\t");
                if(!setVideo->nextSetVideo)
                    break;
                else
                    setVideo = setVideo->nextSetVideo;
            }
        }
        if(dacPtr->setMemClk) {
            setMemClk = dacPtr->setMemClk;
            while(1) {
                printf("\tsetMemClk (%d MHz)\n", setMemClk->frequency);
                sst1InitPrintDacRdWr(setMemClk->setMemClkRdWr, "\t\t");
                if(!setMemClk->nextSetMemClk)
                    break;
                else
                    setMemClk = setMemClk->nextSetMemClk;
            }
        }
        dacPtr = dacPtr->nextDac;
    }
    fclose(file);
}
#endif

#if !defined(INIT_LINUX) && defined(__DOS32__)
static void sst1InitFixFilename(char *dst, char *src)
{
    while(*src) {
        *dst++ = *src;
        if(*src == '\\')
            *dst++ = *src;
        src++;
    }
    *dst = 0;
}
#endif

static int sst1InitFgets(char *string, FILE *stream)
{
    int validChars = 0;
    char *ptr = string;
    int charRead;

    while((charRead = sst1InitFgetc(stream))) {
        *ptr++ = (char) charRead;
        validChars++;
        if(charRead == '\n') {
            *ptr++ = 0;
            break;
        }
    }
    return(validChars);
}

static __inline int sst1_isspace (int c)
{
    switch(c) {
    case ' ':  case '\t':
    case '\n': case '\r':
    case '\f': case '\v': return 1;
    }
    return 0;
}

static int sst1InitFgetc(FILE *stream)
{
    static int column = 0;
    static int validChars = 0;
    int charRead, charReadL;
    int inComment;

    inComment = 0;
    while(1) {
        charRead = fgetc(stream);
        if(inComment == 1) {
            if(charRead <= 0)
                return(0);
            else if(charRead == '\n')
                inComment = 0;
            column = 0;
            validChars = 0;
            continue;
        } else if(column == 0 && charRead == '#') {
            /* Comment line */
            inComment = 1;
            column = 0;
            validChars = 0;
        } else if(charRead <= 0) {
                return(0);
        } else {
            if(charRead == '\n') {
                if(validChars > 0) {
                    validChars = 0;
                    column = 0;
                    return(charRead);
                } else
                    continue;
            } else {
                if(sst1_isspace(charRead))
                    continue;
                validChars++;
                column++;
                charReadL = charRead;
                if (charReadL >= 'a' && charReadL <= 'z')
                    charReadL -= ('a'-'A');
                return(charReadL);
            }
        }
    }
}

static int sst1InitParseFieldCfg(char *string)
{
    char *envName, *envVal;
    sst1InitEnvVarStruct *envVarsPtr;

    if((envName = strtok(string, "=")) == NULL)
        return(0);
    if((envVal = strtok(NULL, "=")) == NULL)
        /* Valid environment variable, NULL value */
        return(1);
    sst1InitToLower(envVal);

    if(envVarsBase == NULL) {
        if((envVarsPtr = malloc(sizeof(sst1InitEnvVarStruct))) == NULL)
            return(0);
        envVarsBase = envVarsPtr;
    } else {
        envVarsPtr = envVarsBase;
        while(1) {
            if(envVarsPtr->nextVar == NULL)
                break;
            else
               envVarsPtr = envVarsPtr->nextVar;
        }
        if((envVarsPtr->nextVar = malloc(sizeof(sst1InitEnvVarStruct))) == NULL)
            return(0);
        envVarsPtr = envVarsPtr->nextVar;
    }
    envVarsPtr->nextVar = NULL;
    strcpy(envVarsPtr->envVariable, envName);
    strcpy(envVarsPtr->envValue, envVal);

    return(1);
}

#if TEST || (!defined(INIT_LINUX) && defined(__DOS32__))
static int sst1InitParseFieldDac(char *string)
{
    char *dacFieldReference, *dacFieldValue;
    static sst1InitDacStruct *dacPtr = NULL;

    if((dacFieldReference = strtok(string, "=")) == NULL)
        return(0);
    if(!strcmp(dacFieldReference, "MANUFACTURER")) {
        /* Add new dac device */
        if(dacStructBase == NULL) {
            if((dacPtr = malloc(sizeof(sst1InitDacStruct))) == NULL)
                return(0);
            dacStructBase = dacPtr;
        } else {
            dacPtr = dacStructBase;
            while(1) {
                if(dacPtr->nextDac == NULL)
                    break;
                else
                   dacPtr = dacPtr->nextDac;
            }
            if((dacPtr->nextDac = malloc(sizeof(sst1InitDacStruct))) == NULL)
                return(0);
            dacPtr = dacPtr->nextDac;
        }
        dacPtr->nextDac = NULL;
        dacPtr->dacManufacturer[0] = 0;
        dacPtr->dacDevice[0] = 0;
        dacPtr->detect = NULL;
        dacPtr->setVideo = NULL;
        dacPtr->setMemClk = NULL;
        dacPtr->setVideoMode = NULL;
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        strcpy(dacPtr->dacManufacturer, dacFieldValue);
    } else if(!strcmp(dacFieldReference, "DEVICE")) {
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        strcpy(dacPtr->dacDevice, dacFieldValue);
    } else if(!strcmp(dacFieldReference, "DETECT")) {
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        sst1InitToLower(dacFieldValue);
        if(!sst1InitParseDacRdWrString(dacFieldValue, dacPtr))
            return(0);
    } else if(!strcmp(dacFieldReference, "SETVIDEO")) {
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        sst1InitToLower(dacFieldValue);
        if(!sst1InitParseSetVideoString(dacFieldValue, dacPtr))
            return(0);
    } else if(!strcmp(dacFieldReference, "SETMEMCLK")) {
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        sst1InitToLower(dacFieldValue);
        if(!sst1InitParseSetMemClkString(dacFieldValue, dacPtr))
            return(0);
    } else if(!strcmp(dacFieldReference, "SETVIDEOMODE")) {
        if((dacFieldValue = strtok(NULL, "=")) == NULL)
            return(0);
        sst1InitToLower(dacFieldValue);
        if(!sst1InitParseSetVideoModeString(dacFieldValue, dacPtr))
            return(0);
    } else
        return(0);

    return(1);
}

static int sst1InitParseDacRdWrString(char *string, sst1InitDacStruct *dacBase)
{
    int cntr = 0;
    int i;
    sst1InitDacRdWrStruct *dacRdWrPtr;
    char *dacRdWrCmd;
    char stringCpy[2048];

    while(1) {
        strcpy(stringCpy, string);
        if((dacRdWrCmd = strtok(stringCpy, ";")) == NULL)
            return(0);
        if(cntr == 0) {
            if(!(dacBase->detect = malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacBase->detect;
        } else {
            for(i=0; i<(cntr-1); i++) {
                if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                    return(0);
            }
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL) {
                break;
            }
            if(!(dacRdWrPtr->nextRdWr = malloc(sizeof(sst1InitDacRdWrStruct)))) 
                return(0);

            dacRdWrPtr = dacRdWrPtr->nextRdWr;
        }
        dacRdWrPtr->nextRdWr = NULL;
        if(!sst1InitParseDacRdWr(dacRdWrCmd, dacRdWrPtr))
            return(0);
        cntr++;
    }
    return(1);
}

static int sst1InitParseDacRdWr(char *string, sst1InitDacRdWrStruct *dacRdWrPtr)
{
    char *addr, *data, *mask, *addrDataCmd;
    char stringCpy[2048];
    int i1, i2, i3;

    strcpy(stringCpy, string);

    if(stringCpy[5] == '(') {
        stringCpy[5] = 0;
        addrDataCmd = &stringCpy[6];
    } else if(stringCpy[7] == '(') {
        stringCpy[7] = 0;
        addrDataCmd = &stringCpy[8];
    } else if(stringCpy[8] == '(') {
        stringCpy[8] = 0;
        addrDataCmd = &stringCpy[9];
    } else if(stringCpy[9] == '(') {
        stringCpy[9] = 0;
        addrDataCmd = &stringCpy[10];
    } else
        return(0);

    if(!strcmp(stringCpy, "dacwr")) {
        if((addr = strtok(addrDataCmd, ",")) == NULL)
            return(0);
        if((data = strtok(NULL, ",")) == NULL)
            return(0);
        if(data[strlen(data)-1] != ')')
            return(0);
        data[strlen(data)-1] = '\0';
        dacRdWrPtr->type = DACRDWR_TYPE_WR;
        if (SSCANF(addr, "%i", &i1) != 1)
            return(0);
        if (SSCANF(data, "%i", &i2) != 1)
            return(0);
        dacRdWrPtr->addr = i1;
        dacRdWrPtr->data = i2;
    } else if(!strcmp(stringCpy, "dacwrpop")) {
        if((addr = strtok(addrDataCmd, ",")) == NULL)
            return(0);
        if((mask = strtok(NULL, ",")) == NULL)
            return(0);
        if((data = strtok(NULL, ",")) == NULL)
            return(0);
        if(data[strlen(data)-1] != ')')
            return(0);
        data[strlen(data)-1] = 0;
        dacRdWrPtr->type = DACRDWR_TYPE_WRMOD_POP;
        if (SSCANF(addr, "%i", &i1) != 1)
            return(0);
        if (SSCANF(mask, "%i", &i2) != 1)
            return(0);
        if (SSCANF(data, "%i", &i3) != 1)
            return(0);
        dacRdWrPtr->addr = i1;
        dacRdWrPtr->mask = i2;
        dacRdWrPtr->data = i3;
    } else if(!strcmp(stringCpy, "dacrdwr")) {
        if((addr = strtok(addrDataCmd, ",")) == NULL)
            return(0);
        if((mask = strtok(NULL, ",")) == NULL)
            return(0);
        if((data = strtok(NULL, ",")) == NULL)
            return(0);
        if(data[strlen(data)-1] != ')')
            return(0);
        data[strlen(data)-1] = 0;
        dacRdWrPtr->type = DACRDWR_TYPE_RDMODWR;
        if (SSCANF(addr, "%i", &i1) != 1)
            return(0);
        if (SSCANF(mask, "%i", &i2) != 1)
            return(0);
        if (SSCANF(data, "%i", &i3) != 1)
            return(0);
        dacRdWrPtr->addr = i1;
        dacRdWrPtr->mask = i2;
        dacRdWrPtr->data = i3;
    } else if(!strcmp(stringCpy, "dacrd")) {
        if((addr = strtok(addrDataCmd, ",")) == NULL)
            return(0);
        if(addr[strlen(addr)-1] == ')') {
            dacRdWrPtr->type = DACRDWR_TYPE_RDNOCHECK;
            if (SSCANF(addr, "%i", &i1) != 1)
                return(0);
            dacRdWrPtr->addr = i1;
        } else {
            dacRdWrPtr->type = DACRDWR_TYPE_RDCHECK;
            if((data = strtok(NULL, ",")) == NULL)
                return(0);
            if(data[strlen(data)-1] != ')')
                return(0);
            data[strlen(data)-1] = 0;
            if (SSCANF(addr, "%i", &i1) != 1)
                return(0);
            if (SSCANF(data, "%i", &i2) != 1)
                return(0);
            dacRdWrPtr->addr = i1;
            dacRdWrPtr->data = i2;
        }
    } else if(!strcmp(stringCpy, "dacrdpush")) {
        if((addr = strtok(addrDataCmd, ",")) == NULL)
            return(0);
        if(addr[strlen(addr)-1] == ')') {
            dacRdWrPtr->type = DACRDWR_TYPE_RDPUSH;
            if (SSCANF(addr, "%i", &i1) != 1)
                return(0);
            dacRdWrPtr->addr = i1;
        } else
            return(0);
    } else {
        return(0);
    }

    return(1);
}

#if TEST
static void sst1InitPrintDacRdWr(sst1InitDacRdWrStruct *dacRdWrBase,
  char *prefix)
{
    sst1InitDacRdWrStruct *dacRdWrPtr = dacRdWrBase;

    while(dacRdWrPtr) {
        if(dacRdWrPtr->type == DACRDWR_TYPE_WR) {
            printf("%sDacWR", prefix);
            printf("(0x%x,0x%x)\n", dacRdWrPtr->addr, dacRdWrPtr->data);
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDMODWR) {
            printf("%sDacRD-MOD-WR", prefix);
            printf("(0x%x,0x%x,0x%x)\n", dacRdWrPtr->addr, dacRdWrPtr->mask,
              dacRdWrPtr->data);
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDNOCHECK) {
            printf("%sDacRD-NOCHECK", prefix);
            printf("(0x%x)\n", dacRdWrPtr->addr);
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDCHECK) {
            printf("%sDacRD-CHECK", prefix);
            printf("(0x%x,0x%x)\n", dacRdWrPtr->addr, dacRdWrPtr->data);
        } else
            printf("%sDAC???", prefix);
        dacRdWrPtr = dacRdWrPtr->nextRdWr;
    }
}
#endif /* TEST */

static int sst1InitParseSetVideoString(char *string, sst1InitDacStruct *dacBase)
{
    int cntr = 0;
    int i;
    sst1InitDacRdWrStruct *dacRdWrPtr;
    sst1InitDacSetVideoStruct *dacSetVideoPtr;
    char *dacRdWrCmd;
    char stringCpy[2048];

    while(1) {
        strcpy(stringCpy, string);
        if((dacRdWrCmd = strtok(stringCpy, ";")) == NULL)
            return(0);
        if(cntr == 0) {
            if(!dacBase->setVideo) {
                if(!(dacBase->setVideo =
                  malloc(sizeof(sst1InitDacSetVideoStruct))))
                    return(0);
                dacSetVideoPtr = dacBase->setVideo;
            } else {
                dacSetVideoPtr = dacBase->setVideo;
                while(1) {
                    if(!dacSetVideoPtr->nextSetVideo)
                        break;
                    dacSetVideoPtr = dacSetVideoPtr->nextSetVideo;
                }
                if(!(dacSetVideoPtr->nextSetVideo =
                   malloc(sizeof(sst1InitDacSetVideoStruct))))
                    return(0);
                dacSetVideoPtr = dacSetVideoPtr->nextSetVideo;
            }
            dacSetVideoPtr->nextSetVideo = NULL;
            /* Width */
            if (SSCANF(dacRdWrCmd, "%i", &dacSetVideoPtr->width) != 1)
                return(0);
            /* Height */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if (SSCANF(dacRdWrCmd, "%i", &dacSetVideoPtr->height) != 1)
                return(0);
            /* Refresh */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if (SSCANF(dacRdWrCmd, "%i", &dacSetVideoPtr->refresh) != 1)
                return(0);
            /* video16BPP */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if (SSCANF(dacRdWrCmd, "%i", &dacSetVideoPtr->video16BPP) != 1)
                return(0);
            /* First DacRdWr */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if(!(dacSetVideoPtr->setVideoRdWr =
               malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacSetVideoPtr->setVideoRdWr;
        } else {
            for(i=0; i<(cntr+3); i++) {
                if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                    return(0);
            }
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL) {
                break;
            }
            if(!(dacRdWrPtr->nextRdWr = malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacRdWrPtr->nextRdWr;
        }
        dacRdWrPtr->nextRdWr = NULL;
        if(!sst1InitParseDacRdWr(dacRdWrCmd, dacRdWrPtr))
            return(0);
        cntr++;
    }
    return(1);
}

static int sst1InitParseSetMemClkString(char *string,
  sst1InitDacStruct *dacBase)
{
    int cntr = 0;
    int i;
    sst1InitDacRdWrStruct *dacRdWrPtr;
    sst1InitDacSetMemClkStruct *dacSetMemClkPtr;
    char *dacRdWrCmd;
    char stringCpy[2048];

    while(1) {
        strcpy(stringCpy, string);
        if((dacRdWrCmd = strtok(stringCpy, ";")) == NULL)
            return(0);
        if(cntr == 0) {
            if(!dacBase->setMemClk) {
                if(!(dacBase->setMemClk =
                  malloc(sizeof(sst1InitDacSetMemClkStruct))))
                    return(0);
                dacSetMemClkPtr = dacBase->setMemClk;
            } else {
                dacSetMemClkPtr = dacBase->setMemClk;
                while(1) {
                    if(!dacSetMemClkPtr->nextSetMemClk)
                        break;
                    dacSetMemClkPtr = dacSetMemClkPtr->nextSetMemClk;
                }
                if(!(dacSetMemClkPtr->nextSetMemClk =
                   malloc(sizeof(sst1InitDacSetMemClkStruct))))
                    return(0);
                dacSetMemClkPtr = dacSetMemClkPtr->nextSetMemClk;
            }
            dacSetMemClkPtr->nextSetMemClk = NULL;
            /* Frequency */
            if (SSCANF(dacRdWrCmd, "%i", &dacSetMemClkPtr->frequency) != 1)
              return(0);

            /* First DacRdWr */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if(!(dacSetMemClkPtr->setMemClkRdWr =
               malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacSetMemClkPtr->setMemClkRdWr;
        } else {
            for(i=0; i<(cntr); i++) {
                if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                    return(0);
            }
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL) {
                break;
            }
            if(!(dacRdWrPtr->nextRdWr = malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacRdWrPtr->nextRdWr;
        }
        dacRdWrPtr->nextRdWr = NULL;
        if(!sst1InitParseDacRdWr(dacRdWrCmd, dacRdWrPtr))
            return(0);
        cntr++;
    }
    return(1);
}

static int sst1InitParseSetVideoModeString(char *string,
  sst1InitDacStruct *dacBase)
{
    int cntr = 0;
    int i;
    sst1InitDacRdWrStruct *dacRdWrPtr;
    sst1InitDacSetVideoModeStruct *dacSetVideoModePtr;
    char *dacRdWrCmd;
    char stringCpy[2048];

    while(1) {
        strcpy(stringCpy, string);
        if((dacRdWrCmd = strtok(stringCpy, ";")) == NULL)
            return(0);
        if(cntr == 0) {
            if(!dacBase->setVideoMode) {
                if(!(dacBase->setVideoMode =
                  malloc(sizeof(sst1InitDacSetVideoModeStruct))))
                    return(0);
                dacSetVideoModePtr = dacBase->setVideoMode;
            } else {
                dacSetVideoModePtr = dacBase->setVideoMode;
                while(1) {
                    if(!dacSetVideoModePtr->nextSetVideoMode)
                        break;
                    dacSetVideoModePtr = dacSetVideoModePtr->nextSetVideoMode;
                }
                if(!(dacSetVideoModePtr->nextSetVideoMode =
                   malloc(sizeof(sst1InitDacSetVideoModeStruct))))
                    return(0);
                dacSetVideoModePtr = dacSetVideoModePtr->nextSetVideoMode;
            }
            dacSetVideoModePtr->nextSetVideoMode = NULL;
            /* video16BPP */
            if (SSCANF(dacRdWrCmd, "%i", &dacSetVideoModePtr->video16BPP) != 1)
              return(0);

            /* First DacRdWr */
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                return(0);
            if(!(dacSetVideoModePtr->setVideoModeRdWr =
               malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacSetVideoModePtr->setVideoModeRdWr;
        } else {
            for(i=0; i<(cntr); i++) {
                if((dacRdWrCmd = strtok(NULL, ";")) == NULL)
                    return(0);
            }
            if((dacRdWrCmd = strtok(NULL, ";")) == NULL) {
                break;
            }
            if(!(dacRdWrPtr->nextRdWr = malloc(sizeof(sst1InitDacRdWrStruct))))
                return(0);
            dacRdWrPtr = dacRdWrPtr->nextRdWr;
        }
        dacRdWrPtr->nextRdWr = NULL;
        if(!sst1InitParseDacRdWr(dacRdWrCmd, dacRdWrPtr))
            return(0);
        cntr++;
    }
    return(1);
}
#endif /* TEST || (!defined(INIT_LINUX) && defined(__DOS32__)) */

static void sst1InitToLower(char *string)
{
    char *ptr = string;

    while(*ptr) {
        if (*ptr >= 'A' && *ptr <= 'Z')
            *ptr += ('a'-'A');
        ptr++;
    }
}

/*
** sst1InitGetenv():
**  Getenv() for INIT routines.
**
**  If the actual environment variable exists (determined by a call to
**  the system getenv() routine), then that pointer is returned.  Otherwise,
**  if the variable is defined in the [CFG] section of "voodoo.ini", then
**  a pointer to the value defined in "voodoo.ini" is returned.  Otherwise,
**  NULL is returned
**
*/
FX_ENTRY char * FX_CALL sst1InitGetenv(char *string)
{
    sst1InitEnvVarStruct *envVarsPtr;
    char *retVal = NULL;

    /* Does the real environment variable exist? */
    if(myGetenv(string))
      return((char*)myGetenv(string));

    /* Does the requested environment variable exist in "voodoo.ini"? */
    /* Dump CFG Data... */
    envVarsPtr = envVarsBase;
    while(envVarsPtr) {
        if(!strcmp(string, envVarsPtr->envVariable)) {
            retVal = envVarsPtr->envValue;
            break;
        }
        envVarsPtr = (sst1InitEnvVarStruct *) envVarsPtr->nextVar;
    }
    return(retVal);
}
#endif

const char*
myGetenv(const char* envKey)
{
  FxBool callRealGetenvP = FXTRUE;

#if __WIN32__
  /* NB: If were being called from cya code in
   * DllMain(DLL_PROCESS_DETACH) because the current app has called
   * exit() or dropped off of the end of main the per dll environ
   * string table has been freed by the c runtime but has not been set
   * to NULL. Bad things happen if this memory has been unmapped by
   * the system or if the string cannot be found.  
   */
  {
    HANDLE curProcessHandle = GetCurrentProcess();
    DWORD exitCode = STILL_ACTIVE;

    callRealGetenvP = ((curProcessHandle != NULL) &&
                       GetExitCodeProcess(curProcessHandle, &exitCode) &&
                       (exitCode == STILL_ACTIVE));
  }
#endif /* __WIN32__ */

  return (callRealGetenvP
          ? getenv(envKey)
          : NULL);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
