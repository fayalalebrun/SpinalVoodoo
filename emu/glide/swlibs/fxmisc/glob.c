/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)glob.c	5.12 (Berkeley) 6/24/91";
#endif /* LIBC_SCCS and not lint */

/*
 * glob(3) -- a superset of the one defined in POSIX 1003.2.
 *
 * The [!...] convention to negate a range is supported (SysV, Posix, ksh).
 *
 * Optional extra services, controlled by flags not defined by POSIX:
 *
 * GLOB_MAGCHAR:
 *	Set in gl_flags if pattern contained a globbing character.
 *
 * gl_matchc:
 *	Number of matches in the current invocation of glob.
 */

#ifndef __WATCOMC__

 // GMT: hack
#if defined(WIN32) && !defined(KERNEL)
#define WIN_NT 1
#ifndef __STDC__
#  define __STDC__ 1
#endif
#define _POSIX_
typedef void DIR;
extern DIR *opendir(const char *);
extern char *readdir(DIR *);
extern int closedir(DIR *);
#endif

#if WIN_NT
#include <sys/stat.h>
//#include <dirent.h>
#include <limits.h>
//#include <unistd.h>
#include "glob.h"
#else
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glob.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if WIN_NT
# define MAXPATHLEN	PATH_MAX
#endif
#define	DOLLAR		'$'
#define	DOT		'.'
#define	EOS		'\0'
#define	LBRACKET	'['
#define	NOT		'!'
#define	QUESTION	'?'
#define	QUOTE		'\\'
#define	RANGE		'-'
#define	RBRACKET	']'
#define	SEP		'/'
#define	STAR		'*'
#define	TILDE		'~'
#define	UNDERSCORE	'_'
#if WIN_NT
# define PROT1		'\''
#endif

#define	M_QUOTE		0x8000
#define	M_PROTECT	0x4000
#define	M_MASK		0xFFFF
#define	M_ASCII		0x00FF

#define	CHAR(c)		((c)&M_ASCII)
#define	META(c)		((c)|M_QUOTE)
#define	M_ALL		META('*')
#define	M_END		META(']')
#define	M_NOT		META('!')
#define	M_ONE		META('?')
#define	M_RNG		META('-')
#define	M_SET		META('[')
#define	ismeta(c)	(((c)&M_QUOTE) != 0)

#if WIN_NT
typedef unsigned short Char;
typedef unsigned char u_char;
typedef unsigned int u_int;

static int	 compare (const void *, const void *);
static void	 g_Ctoc (Char *, char *);
# if 0
static int	 g_lstat (Char *, struct stat *);
# endif
static DIR	*g_opendir (Char *);
static Char	*g_strchr (Char *, int);
static int	 g_stat (Char *, struct _stat *);
static int	 glob1 (Char *, glob_t *);
static int	 glob2 (Char *, Char *, Char *, glob_t *);
static int	 glob3 (Char *, Char *, Char *, Char *, glob_t *);
static int	 globextend (Char *, glob_t *);
static int	 match (Char *, Char *, Char *);
#ifdef DEBUG
static void	 qprintf (Char *);
#endif
#else
typedef u_short Char;

static int	 compare __P((const void *, const void *));
static void	 g_Ctoc __P((Char *, char *));
# if 0
static int	 g_lstat __P((Char *, struct stat *));
# endif
static DIR	*g_opendir __P((Char *));
static Char	*g_strchr __P((Char *, int));
static int	 g_stat __P((Char *, struct stat *));
static int	 glob1 __P((Char *, glob_t *));
static int	 glob2 __P((Char *, Char *, Char *, glob_t *));
static int	 glob3 __P((Char *, Char *, Char *, Char *, glob_t *));
static int	 globextend __P((Char *, glob_t *));
static int	 match __P((Char *, Char *, Char *));
#ifdef DEBUG
static void	 qprintf __P((Char *));
#endif
#endif
#if WIN_NT

static int	isquotesep;	/* Should SEP be QUOTE? */
#endif

/*
 * The main glob() routine: compiles the pattern (optionally processing
 * quotes), calls glob1() to do the real pattern matching, and finally
 * sorts the list (unless unsorted operation is requested).  Returns 0
 * if things went well, nonzero if errors occurred.  It is not an error
 * to find no matches.
 */
int
#if __STDC__
glob (const char *pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob)
#else
glob(pattern, flags, errfunc, pglob)
	const char *pattern;
	int flags, (*errfunc) __P((char *, int));
	glob_t *pglob;
#endif
{
	const u_char *compilepat, *patnext;
#if WIN_NT
	int c, err, oldpathc, prot1;
#else
	int c, err, oldpathc;
#endif
	Char *bufnext, *bufend, *compilebuf, *qpatnext, patbuf[MAXPATHLEN+1];

#if WIN_NT
	isquotesep = 1; // GMT: (getppid() == (pid_t) 1); /* is parent CMD.EXE? */
	prot1 = 0;
#endif
	patnext = (u_char *) pattern;
	if (!(flags & GLOB_APPEND)) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
		if (!(flags & GLOB_DOOFFS))
			pglob->gl_offs = 0;
	}
	pglob->gl_flags = flags & ~GLOB_MAGCHAR;
	pglob->gl_errfunc = errfunc;
	oldpathc = (int) pglob->gl_pathc;
	pglob->gl_matchc = 0;

	bufnext = patbuf;
	bufend = bufnext + MAXPATHLEN;
	compilebuf = bufnext;
	compilepat = patnext;
	if (!(flags & GLOB_NOESCAPE)) {
		/* Protect the quoted characters. */
		while (bufnext < bufend && (c = *patnext++) != EOS) 
#if WIN_NT
			if (c == PROT1 && (flags & GLOB_SHQUOTE))
				prot1 ^= 1;
			else if (prot1)
				*bufnext++ = (Char) (c | M_PROTECT);
			else if (c == QUOTE) {
				if (isquotesep)
					*bufnext++ = (Char) SEP;
				else {
					if ((c = *patnext++) == EOS) {
						c = QUOTE;
						--patnext;
					}
					*bufnext++ = (Char) (c | M_PROTECT);
				}
			}
#else
			if (c == QUOTE) {
				if ((c = *patnext++) == EOS) {
					c = QUOTE;
					--patnext;
				}
				*bufnext++ = (Char) (c | M_PROTECT);
			}
#endif
			else
				*bufnext++ = (Char) c;
	}
	else 
	    while (bufnext < bufend && (c = *patnext++) != EOS) 
		    *bufnext++ = (Char) c;
	*bufnext = EOS;

	bufnext = patbuf;
	qpatnext = patbuf;
	/* We don't need to check for buffer overflow any more. */
	while ((c = *qpatnext++) != EOS) {
		switch (c) {
		case LBRACKET:
			pglob->gl_flags |= GLOB_MAGCHAR;
			c = *qpatnext;
			if (c == NOT)
				++qpatnext;
			if (*qpatnext == EOS ||
			    g_strchr(qpatnext+1, RBRACKET) == NULL) {
				*bufnext++ = LBRACKET;
				if (c == NOT)
					--qpatnext;
				break;
			}
			*bufnext++ = M_SET;
			if (c == NOT)
				*bufnext++ = M_NOT;
			c = *qpatnext++;
			do {
				*bufnext++ = (Char) CHAR(c);
				if (*qpatnext == RANGE &&
				    (c = qpatnext[1]) != RBRACKET) {
					*bufnext++ = M_RNG;
					*bufnext++ = (Char) CHAR(c);
					qpatnext += 2;
				}
			} while ((c = *qpatnext++) != RBRACKET);
			*bufnext++ = M_END;
			break;
		case QUESTION:
			pglob->gl_flags |= GLOB_MAGCHAR;
			*bufnext++ = M_ONE;
			break;
		case STAR:
			pglob->gl_flags |= GLOB_MAGCHAR;
			*bufnext++ = M_ALL;
			break;
		default:
			*bufnext++ = (Char) CHAR(c);
			break;
		}
	}
	*bufnext = EOS;
#ifdef DEBUG
	qprintf(patbuf);
#endif

	if ((err = glob1(patbuf, pglob)) != 0)
		return(err);

	if (pglob->gl_matchc == 0 && !(flags & GLOB_NOCHECK))
		return(GLOB_NOMATCH);

	if (pglob->gl_pathc == (size_t) oldpathc && (flags & GLOB_NOCHECK)) {
#if WIN_NT
		if (isquotesep || (flags & GLOB_NOESCAPE)) {
#else
		if (flags & GLOB_NOESCAPE) {
#endif
			Char *dp = compilebuf;
			const u_char *sp = compilepat;
#if WIN_NT
			while (*sp != '\0')
				if (*sp == PROT1 && (flags & GLOB_SHQUOTE))
					++sp;
				else
					*dp++ = *sp++;
#else
			while ((*dp++ = *sp++) != '\0')
				;
#endif
#if WIN_NT && CONVERT_TO_SLASHES
			for (dp = compilebuf; *dp != '\0'; ++dp)
				if (*dp == QUOTE)
					*dp = SEP;
#endif
		}
		else {
			/*
			 * Copy pattern, interpreting quotes; this is slightly
			 * different than the interpretation of quotes above
			 * -- which should prevail?
			 */
			while (*compilepat != EOS) {
#if WIN_NT
				if (*compilepat == PROT1 && (flags & GLOB_SHQUOTE)) {
					++compilepat;
					continue;
				} else
#endif
				if (*compilepat == QUOTE) {
					if (*++compilepat == EOS)
						--compilepat;
				}
				*compilebuf++ = (u_char)*compilepat++;
			}
			*compilebuf = EOS;
		}
		return(globextend(patbuf, pglob));
	} else if (!(flags & GLOB_NOSORT)) 
		qsort(pglob->gl_pathv + pglob->gl_offs + oldpathc,
		    pglob->gl_pathc - oldpathc, sizeof(char *), compare);
	return(0);
}

static int
#if __STDC__
compare (const void *p, const void *q)
#else
compare(p, q)
	const void *p, *q;
#endif
{
	return(strcmp(*(char **)p, *(char **)q));
}

static int
#if __STDC__
glob1 (Char *pattern, glob_t *pglob)
#else
glob1(pattern, pglob)
	Char *pattern;
	glob_t *pglob;
#endif
{
	Char pathbuf[MAXPATHLEN+1];

	/* A null pathname is invalid -- POSIX 1003.1 sect. 2.4. */
	if (*pattern == EOS)
		return(0);
	return(glob2(pathbuf, pathbuf, pattern, pglob));
}

/*
 * The functions glob2 and glob3 are mutually recursive; there is one level
 * of recursion for each segment in the pattern that contains one or more
 * meta characters.
 */
static int
#if __STDC__
glob2 (Char *pathbuf, Char *pathend, Char *pattern, glob_t *pglob)
#else
glob2(pathbuf, pathend, pattern, pglob)
	Char *pathbuf, *pathend, *pattern;
	glob_t *pglob;
#endif
{
	struct _stat sb;
	Char *p, *q;
	int anymeta;

	/*
	 * Loop over pattern segments until end of pattern or until
	 * segment with meta character found.
	 */
	for (anymeta = 0;;) {
		if (*pattern == EOS) {		/* End of pattern? */
			*pathend = EOS;
			if (g_stat(pathbuf, &sb))
				return(0);
		
			if (((pglob->gl_flags & GLOB_MARK) &&
#if WIN_NT
#define S_ISDIR(m) ( ((m) & _S_IFMT) == _S_IFDIR )
			    pathend[-1] != SEP) && S_ISDIR(sb.st_mode)) {
				*pathend++ = SEP;
#else
			    pathend[-1] != SEP) && (S_ISDIR(sb.st_mode)
			    || (S_ISLNK(sb.st_mode) &&
			    (g_stat(pathbuf, &sb) == 0) &&
			    S_ISDIR(sb.st_mode)))) {
				*pathend++ = SEP;
#endif
				*pathend = EOS;
			}
			++pglob->gl_matchc;
			return(globextend(pathbuf, pglob));
		}

		/* Find end of next segment, copy tentatively to pathend. */
		q = pathend;
		p = pattern;
		while (*p != EOS && *p != SEP) {
			if (ismeta(*p))
				anymeta = 1;
			*q++ = *p++;
		}

		if (!anymeta) {		/* No expansion, do next segment. */
			pathend = q;
			pattern = p;
			while (*pattern == SEP)
				*pathend++ = *pattern++;
		} else			/* Need expansion, recurse. */
			return(glob3(pathbuf, pathend, pattern, p, pglob));
	}
	/* NOTREACHED */
}

static int
#if __STDC__
glob3 (Char *pathbuf, Char *pathend, Char *pattern, Char *restpattern, glob_t *pglob)
#else
glob3(pathbuf, pathend, pattern, restpattern, pglob)
	Char *pathbuf, *pathend, *pattern, *restpattern;
	glob_t *pglob;
#endif
{
	char *dp;
	DIR *dirp;
	int err;
static int errno;	// GMT: this keeps the linker from going into LIBC and
			// generating multiply defined warnings for _exit
	*pathend = EOS;
	errno = 0;
	    
	if (!(dirp = g_opendir(pathbuf)))
		/* TODO: don't call for ENOENT or ENOTDIR? */
		if (pglob->gl_errfunc &&
		    (*pglob->gl_errfunc)((char *) pathbuf, errno) ||
		    (pglob->gl_flags & GLOB_ERR))
			return(GLOB_ABORTED);
		else
			return(0);

	err = 0;

	/* Search directory for matching names. */
	while ((dp = readdir(dirp)) != NULL) {
		register u_char *sc;
		register Char *dc;

		/* Initial DOT must be matched literally. */
		if (dp[0] == DOT && *pattern != DOT)
			continue;
		for (sc = (u_char *) dp, dc = pathend; 
		     (*dc++ = *sc++) != '\0';)
			;
		if (!match(pathend, pattern, restpattern)) {
			*pathend = EOS;
			continue;
		}
		err = glob2(pathbuf, --dc, restpattern, pglob);
		if (err)
			break;
	}

	/* TODO: check error from readdir? */
	(void)closedir(dirp);
	return(err);
}


/*
 * Extend the gl_pathv member of a glob_t structure to accomodate a new item,
 * add the new item, and update gl_pathc.
 *
 * This assumes the BSD realloc, which only copies the block when its size
 * crosses a power-of-two boundary; for v7 realloc, this would cause quadratic
 * behavior.
 *
 * Return 0 if new item added, error code if memory couldn't be allocated.
 *
 * Invariant of the glob_t structure:
 *	Either gl_pathc is zero and gl_pathv is NULL; or gl_pathc > 0 and
 *	gl_pathv points to (gl_offs + gl_pathc + 1) items.
 */
static int
#if __STDC__
globextend (Char *path, glob_t *pglob)
#else
globextend(path, pglob)
	Char *path;
	glob_t *pglob;
#endif
{
	register char **pathv;
	register int i;
	u_int newsize;
	char *copy;
	Char *p;

	newsize = sizeof(*pathv) * (2 + pglob->gl_pathc + pglob->gl_offs);
	pathv = (char **)realloc((char *)pglob->gl_pathv, newsize);
	if (pathv == NULL)
		return(GLOB_NOSPACE);

	if (pglob->gl_pathv == NULL && pglob->gl_offs > 0) {
		/* first time around -- clear initial gl_offs items */
		pathv += pglob->gl_offs;
		for (i = pglob->gl_offs; --i >= 0; )
			*--pathv = NULL;
	}
	pglob->gl_pathv = pathv;

	for (p = path; *p++;);
	if ((copy = malloc(p - path)) != NULL) {
#if WIN_NT && CONVERT_TO_BACKSLASHES
		char *c;

		g_Ctoc(path, copy);
		for (c = copy; *c++;)
			if (isquotesep && *c == SEP)
				*c = QUOTE;
#else
		g_Ctoc(path, copy);
#endif
		pathv[pglob->gl_offs + pglob->gl_pathc++] = copy;
	}
	pathv[pglob->gl_offs + pglob->gl_pathc] = NULL;
	return(copy == NULL ? GLOB_NOSPACE : 0);
}


/*
 * pattern matching function for filenames.  Each occurrence of the *
 * pattern causes a recursion level.
 */
static int
#if __STDC__
match (register Char *name, register Char *pat, register Char *patend)
#else
match(name, pat, patend)
	register Char *name, *pat, *patend;
#endif
{
	int ok, negate_range;
	Char c, k;

	while (pat < patend) {
		c = *pat++;
		switch (c & M_MASK) {
		case M_ALL:
			if (pat == patend)
				return(1);
			for (; *name != EOS; ++name)
				if (match(name, pat, patend))
					return(1);
			return(0);
		case M_ONE:
			if (*name++ == EOS)
				return(0);
			break;
		case M_SET:
			ok = 0;
			k = *name++;
			if ((negate_range = ((*pat & M_MASK) == M_NOT)) != 0)
				++pat;
			while (((c = *pat++) & M_MASK) != M_END)
				if ((*pat & M_MASK) == M_RNG) {
					if (c <= k && k <= pat[1])
						ok = 1;
					pat += 2;
				} else if (c == k)
					ok = 1;
			if (ok == negate_range)
				return(0);
			break;
		default:
			if (*name++ != c)
				return(0);
			break;
		}
	}
	return(*name == EOS);
}

/* Free allocated data belonging to a glob_t structure. */
void
#if __STDC__
globfree (glob_t *pglob)
#else
globfree(pglob)
	glob_t *pglob;
#endif
{
	register int i;
	register char **pp;

	if (pglob->gl_pathv != NULL) {
		pp = pglob->gl_pathv + pglob->gl_offs;
		for (i = pglob->gl_pathc; i--; ++pp)
			if (*pp)
				free(*pp);
		free(pglob->gl_pathv);
	}
}

static DIR *
#if __STDC__
g_opendir (register Char *str)
#else
g_opendir(str)
	register Char *str;
#endif
{
	char buf[MAXPATHLEN];

	if (!*str)
		return(opendir("./*"));
	g_Ctoc(str, buf);
strcat(buf,"/*");
	return(opendir(buf));
}
#if 0

static int
#if __STDC__
g_lstat (register Char *fn, struct stat *sb)
#else
g_lstat(fn, sb)
	register Char *fn;
	struct stat *sb;
#endif
{
	char buf[MAXPATHLEN];

	g_Ctoc(fn, buf);
	return(lstat(buf, sb));
}
#endif

static int
#if __STDC__
g_stat (register Char *fn, struct _stat *sb)
#else
g_stat(fn, sb)
	register Char *fn;
	struct stat *sb;
#endif
{
	char buf[MAXPATHLEN];

	g_Ctoc(fn, buf);
	return(_stat(buf, sb));
}

static Char *
#if __STDC__
g_strchr (Char *str, int ch)
#else
g_strchr(str, ch)
	Char *str;
	int ch;
#endif
{
	do {
		if (*str == (Char) ch)
			return (str);
	} while (*str++);
	return (NULL);
}

static void
#if __STDC__
g_Ctoc (register Char *str, char *buf)
#else
g_Ctoc(str, buf)
	register Char *str;
	char *buf;
#endif
{
	register char *dc;

	for (dc = buf; (*dc++ = (char) *str++) != '\0';)
		;
}

#ifdef DEBUG
static void 
#if __STDC__
qprintf (register Char *s)
#else
qprintf(s)
	register Char *s;
#endif
{
	register Char *p;

	for (p = s; *p; p++)
		(void)printf("%c", *p & 0xff);
	(void)printf("\n");
	for (p = s; *p; p++)
		(void)printf("%c", *p & M_PROTECT ? '"' : ' ');
	(void)printf("\n");
	for (p = s; *p; p++)
		(void)printf("%c", *p & M_META ? '_' : ' ');
	(void)printf("\n");
}
#endif
#endif

