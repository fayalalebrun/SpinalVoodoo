# DOS / OpenWatcom makefile for Glide3 and Texus2
# This makefile MUST be processed by GNU make!!!
# Building under native DOS is not supported:
#		only tested under Win32 or Linux
#
#  Copyright (c) 2003 - Daniel Borca
#  Email : dborca@users.sourceforge.net
#  Web   : http://www.geocities.com/dborca
#

#
#  Available options:
#
#    Environment variables:
#	FX_GLIDE_HW	build for the given ASIC (sst1, sst96, cvg, h3, h5).
#			default = h5
#	H4=1		High speed Avenger/Napalm.
#			target  = h3, h5
#			default = no
#	OPTFLAGS	pass given optimization flags to compiler
#			target  = sst1, sst96, cvg, h3, h5
#			default = -ox -5s (Pentium, stack)
#	DEBUG=1		enable debugging checks and messages
#			target  = sst1, sst96, cvg, h3, h5
#			default = no
#	USE_X86=1	use assembler triangle specializations; req by CVG
#			target  = sst1, sst96, cvg, h3, h5
#			default = no
#	USE_3DNOW=1	allow 3DNow! specializations. However, the true CPU
#			capabilities are still checked at run-time to avoid
#			crashes.
#			target  = cvg, h3, h5
#			default = no
#	USE_MMX=1	(see USE_3DNOW)
#			target  = h5
#			default = no
#	USE_SSE=1	(see USE_3DNOW)
#			target  = h5
#			default = no
#	USE_SSE2=1	(see USE_3DNOW)
#			target  = h5
#			default = no
#	TEXUS2=1	embed Texus2 functions into Glide3.
#			target  = sst1, sst96, cvg, h3, h5
#			default = no
#
#    Targets:
#	all:		build everything
#	clean:		remove object files
#	realclean:	remove all generated files
#

.PHONY: all clean realclean

export BUILD_NUMBER = 40404
export FX_GLIDE_HW ?= h5

ifeq ($(FX_GLIDE_HW),sst96)
G3_DIR = sst1/glide3/src
else
G3_DIR = $(FX_GLIDE_HW)/glide3/src
endif

all:
	make -f Makefile.wat -C $(G3_DIR)
# USE_X86=1 USE_3DNOW=1 USE_MMX=1 USE_SSE=1 USE_SSE2=1

clean:
	make -f Makefile.wat -C $(G3_DIR) clean

realclean:
	make -f Makefile.wat -C $(G3_DIR) realclean
