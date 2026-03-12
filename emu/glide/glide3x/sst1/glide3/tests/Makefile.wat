# OpenWatcom tests makefile for Glide3
# This makefile MUST be processed by GNU make!!!
# Building under native DOS is not supported:
#		only tested under Win32 or Linux
#
#  Copyright (c) 2004 - Borca Daniel
#  Email : dborca@users.sourceforge.net
#  Web   : http://www.geocities.com/dborca
#

#
#  Available options:
#
#    Environment variables:
#	FX_GLIDE_HW	build for the given ASIC (sst1, sst96).
#			default = sst1
#	CPU		optimize for the given processor.
#			default = 5s (Pentium, stack)
#	DEBUG=1		disable optimizations and build for debug.
#			default = no
#
#    Targets:
#	<file.exe>	build a specific file
#

.PHONY: all
.SUFFIXES: .c .obj .exe
.SECONDARY: tlib.obj fxos.obj

FX_GLIDE_HW ?= sst1
ifeq ($(FX_GLIDE_HW),sst1)
HWDEF = -DSST1
else
ifeq ($(FX_GLIDE_HW),sst96)
HWDEF = -DSST96
endif
endif

override FX_HW_BASE = sst1
TOP = ../../..

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

CC = wcl386
CFLAGS = -bt=dos -wx -zq
INCPATH = -I$(TOP)/$(FX_HW_BASE)/glide3/src -I$(TOP)/$(FX_HW_BASE)/incsrc -I$(TOP)/$(FX_HW_BASE)/init
INCPATH += -I$(TOP)/swlibs/fxmisc
CFLAGS += -D__DOS__ $(HWDEF)
CFLAGS += -D__DOS32__
CFLAGS += $(call FIXPATH,$(INCPATH))

ifdef DEBUG
CFLAGS += -od -d2
else
CPU ?= 5s
CFLAGS += -ox -$(CPU)
endif

# pick either of causeway, dos4g, dos32a or stub32a as link target
LDFLAGS = -zq -k16384 -l=dos32a

LDLIBS = $(TOP)/$(FX_HW_BASE)/lib/$(FX_GLIDE_HW)/glide3x.lib

.c.obj:
	$(CC) $(CFLAGS) -fo=$@ -c $<
%.exe: tlib.obj %.obj
	$(CC) -fe=$@ $(LDFLAGS) $^ $(call FIXPATH,$(LDLIBS))

all:
	$(error Must specify <filename.exe> to build)

sbench.exe: sbench.obj fxos.obj tlib.obj
	$(CC) -fe=$@ $(LDFLAGS) $^ $(call FIXPATH,$(LDLIBS))

fxos.obj: $(call FIXPATH,$(TOP)/swlibs/fxmisc/fxos.c)
	$(CC) $(CFLAGS) -fo=$@ -c $<
