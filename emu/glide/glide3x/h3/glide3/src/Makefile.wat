# OpenWatcom makefile for Glide3/H3 and Texus2
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
#	H4=1		High speed Avenger.
#			default = no
#	OPTFLAGS	pass given optimization flags to compiler
#			default = -ox -5s (Pentium, stack)
#	DEBUG=1		enable debugging checks and messages
#			default = no
#	USE_X86=1	use assembler triangle specializations!
#			default = no
#	USE_3DNOW=1	allow 3DNow! specializations. However, the true CPU
#			capabilities are still checked at run-time to avoid
#			crashes.
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

FX_GLIDE_SW = ../../../swlibs
GLIDE_LIBDIR = ../../lib
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
CDEFS = -D__DOS__ -D__DOS32__

# general
CDEFS += -DGLIDE3 -DGLIDE3_ALPHA -DGLIDE_HW_TRI_SETUP=1 -DGLIDE_INIT_HWC -DGLIDE_PACKED_RGB=0 -DGLIDE_PACKET3_TRI_SETUP=1 -DGLIDE_TRI_CULLING=1 -DUSE_PACKET_FIFO=1
#CDEFS += -DGLIDE_CHECK_CONTEXT

# subsystem
CDEFS += -DH3
ifdef H4
CDEFS += -DH4
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
INCPATH = -I. -I../../incsrc -I../../minihwc -I../../cinit
INCPATH += -I$(FX_GLIDE_SW)/fxmisc -I$(FX_GLIDE_SW)/newpci/pcilib -I$(FX_GLIDE_SW)/fxmemmap
INCPATH += -I$(FX_GLIDE_SW)/texus2/lib
OPTFLAGS ?= -ox -5s
CFLAGS += $(CDEFS) $(OPTFLAGS)
CFLAGS += $(call FIXPATH,$(INCPATH))

ifeq ($(USE_3DNOW),1)
CFLAGS += -DGL_AMD3D
override USE_X86 = 1
endif

ifeq ($(USE_X86),1)
CFLAGS += -DGL_X86
else
CFLAGS += -DGLIDE_USE_C_TRISETUP
endif

###############################################################################
#	objects
###############################################################################

GLIDE_OBJECTS = \
	fifo.obj \
	distate.obj \
	gstrip.obj \
	distrip.obj \
	diget.obj \
	gsplash.obj \
	g3df.obj \
	gu.obj \
	gpci.obj \
	diglide.obj \
	disst.obj \
	ditex.obj \
	gbanner.obj \
	gerror.obj \
	gaa.obj \
	gdraw.obj \
	gglide.obj \
	glfb.obj \
	gsst.obj \
	gtex.obj \
	gtexdl.obj \
	xtexdl_def.obj

ifeq ($(USE_X86),1)
GLIDE_OBJECTS += \
	cpuid.obj \
	xdraw2_d.obj \
	xdraw3_d.obj
ifeq ($(USE_3DNOW),1)
GLIDE_OBJECTS += \
	xdraw2_3.obj \
	xdraw3_3.obj \
	xtexdl_3.obj
endif
else
GLIDE_OBJECTS += \
	gxdraw.obj
endif

GLIDE_OBJECTS += \
	gthread.obj \
	$(FX_GLIDE_SW)/newpci/pcilib/fxpci.obj \
	$(FX_GLIDE_SW)/newpci/pcilib/fxdpmi2.obj \
	../../minihwc/hwcio.obj \
	../../minihwc/gdebug.obj \
	../../minihwc/minihwc.obj \
	../../minihwc/dos_mode.obj \
	../../cinit/h3cinit.obj

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
xdraw2_d.obj: xdraw2.asm
	$(AS) -o $@ $(ASFLAGS) $<
xdraw3_d.obj: xdraw3.asm
	$(AS) -o $@ $(ASFLAGS) $<
xdraw2_3.obj: xdraw2.asm
	$(AS) -o $@ $(ASFLAGS) -DGL_AMD3D=1 $<
xdraw3_3.obj: xdraw3.asm
	$(AS) -o $@ $(ASFLAGS) -DGL_AMD3D=1 $<
xtexdl_3.obj: xtexdl.asm
	$(AS) -o $@ $(ASFLAGS) -DGL_AMD3D=1 $<

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
	-$(call UNLINK,../../cinit/*.obj)
	-$(call UNLINK,../../minihwc/*.obj)
	-$(call UNLINK,$(FX_GLIDE_SW)/newpci/pcilib/*.obj)
	-$(call UNLINK,fxinline.h)
	-$(call UNLINK,fxgasm.h)
	-$(call UNLINK,$(FX_GLIDE_SW)/texus2/lib/*.obj)
	-$(call UNLINK,*.err)

realclean: clean
	-$(call UNLINK,$(GLIDE_LIBDIR)/$(GLIDE_LIB))
	-$(call UNLINK,$(TEXUS_EXEDIR)/$(TEXUS_EXE))
