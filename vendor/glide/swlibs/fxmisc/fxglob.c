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

/*
	xnglob.c

	The globulate function below takes an NT command line and expands
	wildcards according to POSIX.2 rules. It returns 0 on success, or -1
	on failure. The reason that *argv is always globbed on is to convert
	any backslashes in the program name to slashes. The deglobulate
	function below it merely frees the memory associated with the
	previously run globulate.
*/

#ifdef WIN32

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "glob.h"
#include "fxglob.h"

static signed int globulated_argi;
signed int globulated_argc;
char **globulated_argv;

signed int globulate (signed int argi, signed int argc, char **argv)
{
	glob_t gl;
	signed int ret, a;

	if (argi <= 0 || argc < argi || argv == NULL || (ret = glob(*argv, GLOB_NOCHECK, NULL, &gl)) != 0)
		ret = -1;
	else
	{
		for (a = argi; a < argc; ++a)
		{
			if ((ret = glob(argv[a], GLOB_NOCHECK|GLOB_APPEND|GLOB_SHQUOTE, NULL, &gl)) != 0)
			{
				ret = -1;
				goto globfree_gl;
			}
		}
		if ((globulated_argv = malloc((argi + gl.gl_pathc + 1) * sizeof *globulated_argv)) == NULL)
		{
			ret = -1;
			goto globfree_gl;
		}
		if ((*globulated_argv = malloc(strlen(*(gl.gl_pathv)) + 1)) == NULL)
		{
			ret = -1;
			goto globfree_gl;
		}
		(void) strcpy(*globulated_argv, *(gl.gl_pathv));
		for (a = 1; a < argi; ++a)
			globulated_argv[a] = argv[a];
		globulated_argc = gl.gl_pathc - 1;
		for (a = 0; a < globulated_argc; ++a)
		{
			if ((globulated_argv[a + argi] = malloc(strlen(gl.gl_pathv[a + 1]) + 1)) == NULL)
			{
				while (a--)
					free(globulated_argv[a + argi]);
				free(*globulated_argv);
				ret = -1;
				goto globfree_gl;
			}
			(void) strcpy(globulated_argv[a + argi], gl.gl_pathv[a + 1]);
		}
		globulated_argv[a + argi] = NULL;
		globulated_argc += argi;
		globulated_argi = argi;
		ret = 0;
globfree_gl:
		globfree(&gl);
	}
	return ret;
}

void deglobulate (void)
{
	for (globulated_argi = globulated_argc - globulated_argi; globulated_argi--; )
		free(globulated_argv[--globulated_argc]);
	free(*globulated_argv);
	free(globulated_argv);
}

static WIN32_FIND_DATA fdata;
static int valid;

// GMT: POSIX emulation routines
HANDLE opendir(char *path)
{
    HANDLE h;

    h = FindFirstFile(path,&fdata);
    if (h == INVALID_HANDLE_VALUE) {
        printf("error: FindFirstFile failed - %d\n",GetLastError());
    }
    else valid = 1;
    return h;
}

char *readdir(HANDLE h)
{
    if (!valid) {			// if no valid filename
    	if (!FindNextFile(h,&fdata))	// find the next one
	    return 0;			// if none, return 0
    }
    valid = 0;
	// !!!TURBO HACKLAND
	strlwr( fdata.cFileName );
    return fdata.cFileName;
}

int closedir(HANDLE h)
{
    int rv = FindClose(h);
    if (!rv)
    	printf("error: FindClose failed - %d\n",GetLastError());
    return rv;	
}

void fxGlobify( int *argc, char ***argv )
{
	// There's this upper/lower case pattern matching problem...
	// !!!DANGER! FIRE-BREATHING ROBO-HACK
	int index;
	for( index = 1; index < *argc; index++ )
	{
		strlwr( (*argv)[index] );
	}

	if ( globulate( 1, *argc, *argv ) == 0 )
	{
		*argc = globulated_argc;
		*argv = globulated_argv;
	}
}

#endif
