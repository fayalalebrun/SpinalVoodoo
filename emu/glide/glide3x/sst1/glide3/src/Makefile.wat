# OpenWatcom makefile for Glide3/SST1 and Texus2
# This makefile MUST be processed by GNU make!!!
# Building under native DOS is not supported:
#		only tested under Win32 or Linux
#
#  Copyright (c) 2004 - Daniel Borca
#  Email : dborca@users.sourceforge.net
#  Web   : http://www.geocities.com/dborca
#

#
#  Available options:
#
#    Environment variables:
#	FX_GLIDE_HW	build for the given ASIC (either sst1, or sst96).
#			default = sst1
#	OPTFLAGS	pass given optimization flags to compiler
#			default = -ox -5s (Pentium, stack)
#	DEBUG=1		enable debugging checks and messages
#			default = no
#	USE_X86=1	use assembler triangle specializations!
#			default = no
#	TEXUS2=1	embed Texus2 functions into Glide3.
#			default = no
#
#    Targets:
#	all:		build everything
#	glide3x:	build Glide3x lib
#	clean:		remove object files
#	realclean:	remove all generated files
#

.PHONY: all glide3x clean realclean
.INTERMEDIATE: fxgasm.exe wlib.lbc
.SUFFIXES: .c .obj

###############################################################################
#	general defines (user settable?)
###############################################################################

GLIDE_LIB = glide3x.lib
TEXUS_EXE = texus2.exe

FX_GLIDE_HW ?= sst1
FX_GLIDE_SW = ../../../swlibs
GLIDE_LIBDIR = ../../lib/$(FX_GLIDE_HW)
TEXUS_EXEDIR = $(FX_GLIDE_SW)/bin

###############################################################################
#	tools
###############################################################################

CC = wcl386
AS = nasm
AR = wlib

# detect if running under unix by finding 'rm' in $PATH :
ifeq ($(wildcard $(addsuffix /rm,$(subst :, ,$(PATH)))),)
DOSMODE= 1
UNLINK = del $(subst /,\,$(1))
FIXPATH= $(subst /,\,$1)
else
DOSMODE= 0
UNLINK = $(RM) $(1)
FIXPATH= $1
endif

###############################################################################
#	defines
###############################################################################

# platform
CDEFS = -D__DOS__ -D__DOS32__ -DINIT_DOS

# general
CDEFS += -DGLIDE3 -DGLIDE3_ALPHA -DGLIDE_HARDWARE

# subsystem
ifeq ($(FX_GLIDE_HW),sst1)
CDEFS += -DSST1
else
ifeq ($(FX_GLIDE_HW),sst96)
CDEFS += -DSST96
CDEFS += -DSST96_FIFO
#CDEFS += -DSST96_ALT_FIFO_WRAP
#CDEFS += -DINIT96VGASWAP
CDEFS += -DINIT_ACCESS_DIRECT
CDEFS += -DGLIDE_USE_ALT_REGMAP
else
$(error Invalid FX_GLIDE_HW setting)
endif
endif

# debug
ifdef DEBUG
CDEFS += -DGDBG_INFO_ON -DGLIDE_DEBUG -DGLIDE_SANITY_ASSERT -DGLIDE_SANITY_SIZE
endif

# shameless plug and splash screen
#CDEFS += -DGLIDE_PLUG -DGLIDE_SPLASH

ifeq ($(TEXUS2),1)
CDEFS += -DHAVE_TEXUS2
endif

###############################################################################
#	flags
###############################################################################

# librarian
ARFLAGS = -c -fo -n -t -q

# linker
# pick either of causeway, dos4g, dos32a or stub32a as link target
LDFLAGS = -zq -k16384 -l=dos32a

# assembler
ASFLAGS = -O6 -fobj -D__WATCOMD__ --prefix _
ASFLAGS += $(CDEFS)

# compiler
CFLAGS = -bt=dos -wx -zq
# newer OpenWatcom versions enable W303 by default
CFLAGS += -wcd=303
INCPATH = -I. -I../../incsrc -I../../init
ifeq ($(FX_GLIDE_HW),sst1)
INCPATH += -I../../init/initvg
else
INCPATH += -I../../init/init96
endif
INCPATH += -I$(FX_GLIDE_SW)/fxmisc -I$(FX_GLIDE_SW)/newpci/pcilib -I$(FX_GLIDE_SW)/fxmemmap
INCPATH += -I$(FX_GLIDE_SW)/texus2/lib
OPTFLAGS ?= -ox -5s
CFLAGS += $(CDEFS) $(OPTFLAGS)
CFLAGS += $(call FIXPATH,$(INCPATH))

ifeq ($(USE_X86),1)
CFLAGS += -DGL_X86
else
CFLAGS += -DGLIDE_USE_C_TRISETUP
endif

###############################################################################
#	objects
###############################################################################

GLIDE_OBJECTS = \
	distate.obj \
	diget.obj \
	gstrip.obj \
	distrip.obj \
	diglide.obj \
	disst.obj \
	ditex.obj \
	g3df.obj \
	gaa.obj \
	gbanner.obj \
	gdraw.obj \
	gerror.obj \
	gglide.obj \
	glfb.obj \
	gpci.obj \
	gsplash.obj \
	gsst.obj \
	gtex.obj \
	gtexdl.obj \
	gu.obj \
	gxdraw.obj

ifeq ($(USE_X86),1)
GLIDE_OBJECTS += \
	cpuid.obj
ifeq ($(FX_GLIDE_HW),sst1)
GLIDE_OBJECTS += \
	xdraw.obj
else
GLIDE_OBJECTS += \
	xdraw96.obj
endif
endif

INIT_OBJECTS = \
	../../init/init.obj \
	../../init/gdebug.obj

ifeq ($(FX_GLIDE_HW),sst96)
GLIDE_OBJECTS += \
	sst96.obj
INIT_OBJECTS += \
	../../init/vg96drvr.obj \
	../../init/init96/init96.obj \
	../../init/init96/dxdrvr.obj \
	../../init/init96/initat3d.obj \
	../../init/init96/initmcrx.obj
else
INIT_OBJECTS += \
	../../init/vgdrvr.obj \
	../../init/initvg/gamma.obj \
	../../init/initvg/dac.obj \
	../../init/initvg/video.obj \
	../../init/initvg/parse.obj \
	../../init/initvg/sli.obj \
	../../init/initvg/util.obj \
	../../init/initvg/info.obj \
	../../init/initvg/print.obj \
	../../init/initvg/sst1init.obj
endif

GLIDE_OBJECTS += \
	$(INIT_OBJECTS) \
	$(FX_GLIDE_SW)/newpci/pcilib/sst1_pci.obj \
	$(FX_GLIDE_SW)/newpci/pcilib/fxmsr.obj \
	$(FX_GLIDE_SW)/newpci/pcilib/fxpci.obj \
	$(FX_GLIDE_SW)/newpci/pcilib/fxdpmi2.obj

TEXUS_SOURCES = \
	$(FX_GLIDE_SW)/texus2/lib/texuslib.c \
	$(FX_GLIDE_SW)/texus2/lib/clamp.c \
	$(FX_GLIDE_SW)/texus2/lib/read.c \
	$(FX_GLIDE_SW)/texus2/lib/resample.c \
	$(FX_GLIDE_SW)/texus2/lib/mipmap.c \
	$(FX_GLIDE_SW)/texus2/lib/quantize.c \
	$(FX_GLIDE_SW)/texus2/lib/ncc.c \
	$(FX_GLIDE_SW)/texus2/lib/nccnnet.c \
	$(FX_GLIDE_SW)/texus2/lib/pal256.c \
	$(FX_GLIDE_SW)/texus2/lib/pal6666.c \
	$(FX_GLIDE_SW)/texus2/lib/dequant.c \
	$(FX_GLIDE_SW)/texus2/lib/view.c \
	$(FX_GLIDE_SW)/texus2/lib/util.c \
	$(FX_GLIDE_SW)/texus2/lib/diffuse.c \
	$(FX_GLIDE_SW)/texus2/lib/write.c \
	$(FX_GLIDE_SW)/texus2/lib/tga.c \
	$(FX_GLIDE_SW)/texus2/lib/3df.c \
	$(FX_GLIDE_SW)/texus2/lib/ppm.c \
	$(FX_GLIDE_SW)/texus2/lib/rgt.c \
	$(FX_GLIDE_SW)/texus2/lib/txs.c \
	$(FX_GLIDE_SW)/texus2/lib/codec.c \
	$(FX_GLIDE_SW)/texus2/lib/eigen.c \
	$(FX_GLIDE_SW)/texus2/lib/bitcoder.c

ifeq ($(TEXUS2),1)
GLIDE_OBJECTS += $(TEXUS_SOURCES:.c=.obj)
endif

###############################################################################
#	rules
###############################################################################

.c.obj:
	$(CC) $(CFLAGS) -fo=$@ -c $<

###############################################################################
#	main
###############################################################################
all: glide3x $(TEXUS_EXEDIR)/$(TEXUS_EXE)

glide3x: $(GLIDE_LIBDIR)/$(GLIDE_LIB)

$(GLIDE_LIBDIR)/$(GLIDE_LIB): wlib.lbc
	$(AR) $(ARFLAGS) -o $(call FIXPATH,$@) @wlib.lbc

$(TEXUS_EXEDIR)/$(TEXUS_EXE): $(FX_GLIDE_SW)/texus2/cmd/cmd.c $(GLIDE_LIBDIR)/$(GLIDE_LIB)
ifeq ($(TEXUS2),1)
	$(CC) $(CFLAGS) -fe=$(call FIXPATH,$@) $(LDFLAGS) $(call FIXPATH,$^)
else
	$(warning Texus2 not enabled... Skipping $(TEXUS_EXE))
endif

###############################################################################
#	rules(2)
###############################################################################

cpuid.obj: cpudtect.asm
	$(AS) -o $@ $(ASFLAGS) $<
xdraw.obj: xdraw.asm
	$(AS) -o $@ $(ASFLAGS) $<
xdraw96.obj: xdraw96.asm
	$(AS) -o $@ $(ASFLAGS) $<

$(GLIDE_OBJECTS): fxinline.h fxgasm.h

fxinline.h: fxgasm.exe
	$(call FIXPATH,./$<) -inline > $@

fxgasm.h: fxgasm.exe
	$(call FIXPATH,./$<) -hex > $@

# -bt without args resets build target to host OS.
fxgasm.exe: fxgasm.c
	$(CC) $(CFLAGS) -bt -fe=$@ $<

wlib.lbc: $(call FIXPATH,$(GLIDE_OBJECTS))
	@echo $(addprefix +,$^) > wlib.lbc

###############################################################################
#	clean, realclean
###############################################################################

clean:
	-$(call UNLINK,*.obj)
	-$(call UNLINK,*.o)
	-$(call UNLINK,../../init/*.obj)
	-$(call UNLINK,../../init/initvg/*.obj)
	-$(call UNLINK,../../init/init96/*.obj)
	-$(call UNLINK,$(FX_GLIDE_SW)/newpci/pcilib/*.obj)
	-$(call UNLINK,fxinline.h)
	-$(call UNLINK,fxgasm.h)
	-$(call UNLINK,$(FX_GLIDE_SW)/texus2/lib/*.obj)
	-$(call UNLINK,*.err)

realclean: clean
	-$(call UNLINK,$(GLIDE_LIBDIR)/$(GLIDE_LIB))
	-$(call UNLINK,$(TEXUS_EXEDIR)/$(TEXUS_EXE))
