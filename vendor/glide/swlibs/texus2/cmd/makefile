#
# Copyright (c) 1995, 3Dfx Interactive, Inc.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
# the contents of this file may not be disclosed to third parties, copied or
# duplicated in any form, in whole or in part, without the prior written
# permission of 3Dfx Interactive, Inc.
#
# RESTRICTED RIGHTS LEGEND:
# Use, duplication or disclosure by the Government is subject to restrictions
# as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
# and Computer Software clause at DFARS 252.227-7013, and/or in similar or
# successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
# rights reserved under the Copyright Laws of the United States.

# local defines, options, includes
LCDEFS  = 
LCOPTS  =
LCINCS  = -I$(BUILD_ROOT_SWLIBS)\include -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include

!if "$(FX_GLIDE3)" != ""
LCDEFS = $(LCDEFS) -DGLIDE3
!endif

# sources
HFILES = texusint.h
CFILES = cmd.c

INSTALL_DESTINATION = $(BUILD_ROOT_SWLIBS)

PROGRAM = texus2.exe

LLDLIBS = $(BUILD_ROOT_SWLIBS)\lib\fxmisc.lib \
          $(BUILD_ROOT_SWLIBS)\lib\texus.lib

!include $(BUILD_ROOT_SWLIBS)/include/nmake/3dfx.mak
