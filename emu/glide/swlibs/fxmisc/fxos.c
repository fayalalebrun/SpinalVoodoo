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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <fxos.h>

void sleep(int secs)
{
    GdiFlush();
    Sleep(secs * 1000);
}

int fxGethostname(char *name, unsigned long n)
{
    return (int) GetComputerNameA((LPTSTR)name, &n);
}
#else
#include <fxos.h>
#endif

#if !macintosh && !defined(__FreeBSD__)
/* return current time in seconds (floating point)      */
#if defined ( __sparc__ ) || defined ( __DJGPP__ )
/* times returns 0 in BSD Unix, so we use ftime instead */
#include <sys/types.h>
#include <sys/timeb.h>
float fxTime(void)
{
    struct timeb tb;
    static time_t once;                 // saves first time value

    ftime(&tb);
    if (once == 0)                      // stash away first call
        once = tb.time;                 // as float is not big enough
    return (tb.time - once) + tb.millitm * .001;
}
#elif defined ( WIN32 ) || ( __DOS__ )
#include <time.h>
float fxTime(void)
{
    return clock()/(float)CLOCKS_PER_SEC;
}
#else
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
float fxTime(void)
{
    struct tms foo;
    return times(&foo)/(float)HZ;
}
#endif

/* returns elapsed time in seconds */
float timer(int flag)
{
    static float starttime,endtime;

    if (flag == 0)
        starttime = endtime = fxTime();
    else
        endtime = fxTime();
    return endtime - starttime;
}
#endif

//----------------------------------------------------------------------
// open a file thru a search path, first search in the current directory
// and then go thru the semicolon separated search path
// returns the file handle and the path prefix that was used (if one)
//----------------------------------------------------------------------
FILE *fxFopenPath(const char *filename, const char *mode, const char *path, const char **pprefix)
{
    char *psemi;
    FILE *file;

    // first try and open up the file in the current directory
    if (pprefix) *pprefix = NULL;
    if ((file = fopen(filename,mode)) != NULL)
        return file;
    if (path == NULL)
        return NULL;

    while (*path) {             // for each chunk of the search path
        char nameWithPath[1024];
        psemi = strchr(path,';');               // find the next semicolon
        if (psemi) {                            // if one, copy up to it
            strncpy(nameWithPath,path,psemi-path);
            nameWithPath[psemi-path] = '\0';
        }
        else                                    // else copy entire path
            strcpy(nameWithPath,path);
        strcat(nameWithPath,"/");               // add directory separator
        strcat(nameWithPath,filename);          // add filename
        if (pprefix) *pprefix = path;           // save the prefix
        if ((file = fopen(nameWithPath,mode)) != NULL)
            return file;
        path = psemi;                           // advance to next path element
        if (path)
            path++;                             // if one, then skip semicolon
        else
            break;                              // else all done
    }
    return file;
}
