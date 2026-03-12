#
# THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
# PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
# TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
# INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
# DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
# THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
# EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
# FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
# 
# USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
# RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
# TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
# AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
# SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
# THE UNITED STATES.  
# 
# COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED

#
# If we call make recursively, we may want to print the subdirectory to which
# we are changing.  This helps track down make errors.  However, this can also
# make the make output noisy.  If the envariable NOISY_RECURSION is
# set to "YES", we will print the directory when we recurse into it.
# Otherwise we will not.  The default is to not print the directory.
#
ifneq ($(NOISY_RECURSION),YES)
	MAKE_PRINT_DIRECTORY=--no-print-directory
endif
#
# Check to see that certain variables are actually set, and set them
# to reasonable values.
#
ifeq ($(BUILD_ROOT),)
ifeq ($(TOPDIR),)
	export TOPDIR = $(shell pwd)
endif
	export BUILD_ROOT = $(TOPDIR)
endif
ifeq ($(BUILD_ROOT_SWLIBS),)
	export BUILD_ROOT_SWLIBS = $(TOPDIR)/swlibs
endif
ifeq ($(BUILD_ROOT_HW),)
	export BUILD_ROOT_HW = $(TOPDIR)/$(FX_GLIDE_HW)
endif
#
# determine the OS type
#
ifeq ($(SCRIPTDIR),)
SCRIPTDIR=$(TOPDIR)/swlibs/include/make/
endif
OS=$(shell $(SCRIPTDIR)/ostype)
ifeq ($(OS),) 
	echo "$OS not defined"
endif

ifeq ($(GCC_INCLUDE),)
GCC_INCLUDE=/usr/local/include/gcc
endif

#
# Make include file, kept as simple as possible, defines/setups the following
#	1) global CC flags
#	2) global LD flags
#	3) default rule (all)
#	4) make depend rules
#	5) file removal rules
#	6) recursive rules

#--------------------------------------------------------------------------
# configuration variables for the compiler environment
#	FX_COMPILER must be set to either MICROSOFT or WATCOM
#	DEBUG must be set to enable debugging flags for cc and link
#
ifeq ($(CC),)
CC             = gcc
endif
CPP            = $(CC) -E -c
ASM_LIST_FLAGS = -s
CDEBUG         = -g
CNODEBUG       = -O
LDEBUG         = -g
LNODEBUG       =
GLDOPTS	       = -L$(BUILD_ROOT_SWLIBS)/lib -L$(BUILD_ROOT_HW)/lib
LINK           = $(CC)

ifeq "$(OS)" "sunos"
GCINCS         = -I -I- -I$(GCC_INCLUDE) -I/usr/local/include -I$(BUILD_ROOT_SWLIBS)/include
GCOPTS         = -ansi -Wall
GCDEFS         = -DENDB -DX11 -DGDBG_INFO_ON
DEBUGDEFS      = 
endif
ifeq "$(OS)" "solaris"
GCINCS         = -I -I- -I$(GCC_INCLUDE) -I/usr/local/include -I$(BUILD_ROOT_SWLIBS)/include
GCOPTS         = -Wall
GCDEFS         = -DENDB -DX11 -DGDBG_INFO_ON
DEBUGDEFS      = 
endif
ifeq "$(OS)" "hpux"
GCINCS         = -I -I- -I/usr/local/include/gcc -I$(BUILD_ROOT_SWLIBS)/include
GCOPTS         = -Wall
GCDEFS         = -DENDB -DX11 -DGDBG_INFO_ON
DEBUGDEFS      = 
endif

ifeq "$(OS)" "Linux"
DEBUGDEFS      = -DGDBG_INFO_ON -DGLIDE_DEBUG
GCDEFS         = -DENDB -DX11
GCINCS	       = -I. -I$(BUILD_ROOT_SWLIBS)/include -I$(BUILD_ROOT_HW)/include
GCOPTS	       = -Wall
ifeq "$(FX_GLIDE_PIC)" "1"
GCOPTS	       := $(GCOPTS) -fPIC -DPIC
endif

# 
# BIG_OPT Indicates O3(?) or better is being used. It changes the
# assembly language in grDrawTriangle. Larger optimization removes
# an extra push in the calling sequence.
#
CNODEBUG       = -O6 -mcpu=pentium -fomit-frame-pointer -funroll-loops \
	-fexpensive-optimizations -ffast-math -DBIG_OPT

CDEBUG	       = -g -O
GLDOPTS	       = -L$(BUILD_ROOT_SWLIBS)/lib -L/usr/lib
# Profiling
#CDEBUG	       = -pg -g -O
#GCDEFS	       =
endif

ifeq "$(OS)" "FreeBSD"
GCINCS	       = -I. -I$(BUILD_ROOT_SWLIBS)/include -I$(BUILD_ROOT_HW)/include -I/usr/X11R6/include
GCOPTS	       = -Wall
# 
# BIG_OPT Indicates O3(?) or better is being used. It changes the
# assembly language in grDrawTriangle. Larger optimization removes
# an extra push in the calling sequence.
#
CNODEBUG       = -O6 -m486 -fomit-frame-pointer -funroll-loops \
	-fexpensive-optimizations -ffast-math -DBIG_OPT

CDEBUG	       = -g -O
# Profiling
#CDEBUG	       = -pg -g -O
#GCDEFS	       =
endif

# if we are not debugging then replace debug flags with nodebug flags

# DEBUG = xx

ifndef DEBUG
CDEBUG   = $(CNODEBUG)
LDEBUG   = $(LNODEBUG)
else
CDEBUG   += $(DEBUGDEFS)
endif

#--------------------------------------------------------------------------
# build up CFLAGS from global, local, and variable flags
#	each of which has includes, defines, and options
#
GCFLAGS	= $(GCINCS) $(GCDEFS) $(GCOPTS)
LCFLAGS	= $(LCINCS) $(LCDEFS) $(LCOPTS)
VCFLAGS	= $(VCINCS) $(VCDEFS) $(VCOPTS)

CFLAGS	= $(CDEBUG) $(GCFLAGS) $(LCFLAGS) $(VCFLAGS)

#--------------------------------------------------------------------------
# build up global linker flags (LDFLAGS) and libraries (LDLIBS)
#	note that local libs are before global libs
#
GLDLIBS	= -lm
LDFLAGS	= $(LDEBUG) $(GLDOPTS) $(LLDOPTS)
LDLIBS  = $(LLDLIBS) $(GLDLIBS)

#--------------------------------------------------------------------------
# configure OS commands
#

ifeq "$(OS)" "sunos"
AR      = /usr/5bin/ar crsl
ECHO	= /usr/5bin/echo
INSTALL = install
endif
ifeq "$(OS)" "solaris"
AR      = /usr/ccs/bin/ar crsl
ECHO	= /usr/5bin/echo
INSTALL = /usr/ucb/install
endif
ifeq "$(OS)" "hpux"
AR      = /bin/ar crsl
ECHO	= /bin/echo
INSTALL = /usr/local/bin/install
endif
ifeq "$(OS)" "Linux"
AR      = /usr/bin/ar crsl
ECHO	= /bin/echo
INSTALL = /usr/bin/install
endif

DATE	= date
RM      = rm

#--------------------------------------------------------------------------
# TAGS file for emacs

TAGS    = $(BUILD_ROOT)/TAGS

#--------------------------------------------------------------------------
#
# a default rule, serves 2 purposes
#	1) keeps us from typing "make install" all the time
#	2) allows this file to be included first, without clobber becoming
#		the default rule
default: all

all: incs libs bins

OBJECTS	= $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
ifeq ($(OS),Linux)
OBJECTS +=  $(LIBOBJS) $(AFILES:.S=.o)
endif
#--------------------------------------------------------------------------
# rules for INCS, LIBS, and BINS , the three major targets
#

$(THISDIR)incs: $(HEADERS)
ifdef HEADERS
ifdef INSTALL_DESTINATION
	$(INSTALL) -d $(INSTALL_DESTINATION)/include
	$(INSTALL) -m 444 $(HEADERS) $(INSTALL_DESTINATION)/include
else
	@echo INSTALL_DESTINATION not defined, not installing HEADERS
endif
endif

#--------------------------------------------------------------------------
# rules for LIBRARIES
#	NOTE: we supply a default rule for making a library
ifdef LIBRARIES
LIBPARTS = $(OBJECTS) $(SUBLIBRARIES)

$(LIBRARIES): $(LIBPARTS)
ifeq ($(OS),Linux)
	/bin/rm -f $*.a
	$(AR) $*.a $(OBJECTS)
else
	$(AR) $*.a $(LIBPARTS)
endif

# We need to glean the soname from the name of the library, this
# is pretty good as long as shared library nams are reasonable
ifneq "$(SHARED_LIBRARY)" ""
SONAME := $(shell echo $(SHARED_LIBRARY) | cut -d "." -f 1-3)
BASENAME := $(shell echo $(SHARED_LIBRARY) | cut -d "." -f 1-2)
endif

ifneq ($(SHARED_LIBRARY),)
$(SHARED_LIBRARY): $(LIBPARTS) $(SUBLIBRARIES)
	$(LINK) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $(SHARED_LIBRARY) \
		-Xlinker --whole-archive \
		$(LIBRARIES) $(SUBLIBRARIES) \
		-Xlinker --no-whole-archive \
		$(LINKLIBRARIES)
endif

$(THISDIR)libs: $(LIBRARIES) $(SHARED_LIBRARY)
ifdef INSTALL_DESTINATION
	$(INSTALL) -d $(INSTALL_DESTINATION)/lib
	$(INSTALL) -m 444 $(LIBRARIES) $(INSTALL_DESTINATION)/lib
ifneq "$(SHARED_LIBRARY)" ""
	$(INSTALL) -m 444 $(SHARED_LIBRARY) $(INSTALL_DESTINATION)/lib
	ln -sf $(INSTALL_DESTINATION)/lib/$(SHARED_LIBRARY) $(INSTALL_DESTINATION)/lib/$(SONAME)
	ln -sf $(INSTALL_DESTINATION)/lib/$(SHARED_LIBRARY) $(INSTALL_DESTINATION)/lib/$(BASENAME)
endif
else
	@echo INSTALL_DESTINATION not defined, not installing LIBRARIES
endif
else
$(THISDIR)libs:
endif

#--------------------------------------------------------------------------
# rules for BINS
#	NOTE: calling makefile must define rules for making programs
#	or define SIMPLE_EXE or MULTI_EXE
ifeq ($(OS),Linux)
PROGRAM_LLDOBJECTS=
else
PROGRAM_LLDOBJECTS=$(LLDLIBS)
endif

ifdef PROGRAM
$(PROGRAM): $(OBJECTS) $(PROGRAM_LLDOBJECTS)
	$(LINK) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)
endif

ifdef PROGRAMS
$(PROGRAMS): % : %.o $(PROGRAM_LLDOBJECTS)
	$(LINK) -o $@ $@.o $(LDLIBS) $(LDFLAGS)
endif

INSTALL_TARGETS = $(PROGRAM) $(PROGRAMS) $(BATS) $(DIAGS)
$(THISDIR)bins: $(PROGRAM) $(PROGRAMS) $(BATS) $(DIAGS)
ifneq ($(strip $(INSTALL_TARGETS)),)
ifdef INSTALL_DESTINATION
	echo installing $(INSTALL_TARGETS)
	$(INSTALL) -d $(INSTALL_DESTINATION)/bin
	$(INSTALL) -m 555 $(INSTALL_TARGETS) $(INSTALL_DESTINATION)/bin
else
	@echo INSTALL_DESTINATION not defined, not installing PROGRAMS
endif
endif

TARGETS += $(LIBRARIES) $(SHARED_LIBRARY) $(PROGRAM) $(PROGRAMS) $(BATS) $(DIAGS)

#--------------------------------------------------------------------------
# MKDEPFILE is the name of the dependency database
#
MKDEPFILE = makedep

#--------------------------------------------------------------------------
# File removal rules: there are four
#	- neat removes dirt
#	- clean removes intermediates and neat (dirt)
#	- clobber removes targets and clean (intermediates and dirt)
#	- rmtargets removes targets only
#

#--------------------------------------------------------------------------
# DIRT definitions
#
GDIRT	= *.cod *.bak *.pdb *.ilk *.map *.sym *.err *.i stderr.out
ifeq ($(OS),Linux)
GDIRT   += core
endif

DIRT	= $(GDIRT) $(LDIRT)
JUNK	= __junk__

$(THISDIR)clobber: $(THISDIR)clean $(THISDIR)rmtargets
	$(RM) -f $(MKDEPFILE) $(JUNK) $(TAGS)
ifdef INSTALL_DESTINATION
ifdef HEADERS
	$(INSTALL) -d $(INSTALL_DESTINATION)/include
	$(RM) -f $(addprefix $(INSTALL_DESTINATION)/include/, $(HEADERS))
endif
ifdef LIBRARIES
	$(RM) -f $(addprefix $(INSTALL_DESTINATION)/lib/, $(LIBRARIES))
endif
ifneq "$(SHARED_LIBRARY)" ""
	$(RM) -f $(addprefix $(INSTALL_DESTINATION)/lib/, $(SHARED_LIBRARY) $(SONAME) $(BASENAME))
endif
ifneq ($(strip $(INSTALL_TARGETS)),)
	$(RM) -f $(addprefix $(INSTALL_DESTINATION)/bin/, $(INSTALL_TARGETS))
endif
endif

$(THISDIR)clean: $(THISDIR)neat
	$(RM) -f $(OBJECTS) $(JUNK)

$(THISDIR)neat:
	$(RM) -f $(DIRT) $(JUNK)

$(THISDIR)rmtargets:
	$(RM) -f $(TARGETS) $(JUNK)

.SUFFIXES: .cod .i .bat .sh

.c.cod:
	$(CC) $(CFLAGS) $(ASM_LIST_FLAGS) $*.c

.c.i:
	$(CPP) $(CFLAGS) $*.c > $@

.bat.sh:
	awk -f $(BUILD_ROOT_CHIP)/diags/bat2sh.awk $*.bat > $@
	@chmod +x $@

#--------------------------------------------------------------------------
# Include a makedepend file if necessary.  Don't know if this
# works.
#
#if EXISTS ($(MKDEPFILE))
#include $(MKDEPFILE)
#endif

ifndef MAKEFILE
ifeq ($(OS),Linux)
MAKEFILE = makefile.linux
else
MAKEFILE = makefile.unix
endif
endif

ifdef CFILES
$(OBJECTS): $(HEADERS) $(PRIVATE_HEADERS) $(MAKEFILE)
endif

#--------------------------------------------------------------------------
# Rules for emacs TAGS
# The etags command appends to the TAGS file, so the TAGS file will keep growing
# everytime you run add_tags. To start a whole new tags file, make new_tags.
# This will delete the old TAGS file first. Then if you want to add to
# the tags file later, just run add_tags. This is useful when you run make
# from more than one directory (like swlibs and h4), but you want all of it in
# one tags file.

new_tags:
	- rm -f $(TAGS)
	$(MAKE) -f $(MAKEFILE) rtags

add_tags:
	$(MAKE) -f $(MAKEFILE) rtags

$(THISDIR)rtags: $(HEADERS) $(PRIVATE_HEADERS) $(CFILES) $(LIBSRC)
ifdef HEADERS
	etags -t -a -C -o $(TAGS) $(HEADERS)
endif
ifdef PRIVATE_HEADERS
	etags -t -a -C -o $(TAGS) $(PRIVATE_HEADERS)
endif
ifdef CFILES
	etags -t -a -C -o $(TAGS) $(CFILES)
endif
ifdef LIBSRC
	etags -t -a -C -o $(TAGS) $(LIBSRC)
endif

#--------------------------------------------------------------------------
# RECURSIVE rules
#	A recursive makefile should set SUBDIRS and THISDIR
#	setting THISDIR prefixes all the common targets with $(THISDIR)
#	and enables the recursive definitions of them below
#	SUBDIRS1 is used when SUBDIRS exceeds 6 (DOS args only go up to %9)
ifdef THISDIR

# Recursive targets and rules (decend into each subdirectory)
RETARGETS= clobber clean neat rmtargets depend incs libs bins rtags

$(RETARGETS): % : $(THISDIR)%
	@for d in ${SUBDIRS} ;\
	do \
		echo "====recursing into "$$d" (make $@) ============================"; \
		${MAKE} -f $(MAKEFILE) -C $$d $(MAKE_PRINT_DIRECTORY) $@ || exit 1;\
	done
endif
