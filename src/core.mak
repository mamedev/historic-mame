###########################################################################
#
#   core.mak
#
#   MAME core makefile
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# the core object files (without target specific
# objects; those are added in the target.mak
# files)
#-------------------------------------------------

COREOBJS = \
	$(OBJ)/artwork.o \
	$(OBJ)/audit.o \
	$(OBJ)/cdrom.o \
	$(OBJ)/chd.o \
	$(OBJ)/cheat.o \
	$(OBJ)/common.o \
	$(OBJ)/config.o \
	$(OBJ)/cpuexec.o \
	$(OBJ)/cpuint.o \
	$(OBJ)/cpuintrf.o \
	$(OBJ)/drawgfx.o \
	$(OBJ)/fileio.o \
	$(OBJ)/harddisk.o \
	$(OBJ)/hash.o \
	$(OBJ)/hiscore.o \
	$(OBJ)/info.o \
	$(OBJ)/input.o \
	$(OBJ)/inptport.o \
	$(OBJ)/mame.o \
	$(OBJ)/md5.o \
	$(OBJ)/memory.o \
	$(OBJ)/palette.o \
	$(OBJ)/png.o \
	$(OBJ)/profiler.o \
	$(OBJ)/sha1.o \
	$(OBJ)/sndintrf.o \
	$(OBJ)/state.o \
	$(OBJ)/tilemap.o \
	$(OBJ)/timer.o \
	$(OBJ)/ui_text.o \
	$(OBJ)/unzip.o \
	$(OBJ)/usrintrf.o \
	$(OBJ)/validity.o \
	$(OBJ)/version.o \
	$(OBJ)/x86drc.o \
	$(OBJ)/xmlfile.o \
	$(OBJ)/sound/filter.o \
	$(OBJ)/sound/flt_vol.o \
	$(OBJ)/sound/flt_rc.o \
	$(OBJ)/sound/streams.o \
	$(OBJ)/sound/wavwrite.o \
	$(OBJ)/machine/eeprom.o \
	$(OBJ)/vidhrdw/generic.o \
	$(OBJ)/vidhrdw/vector.o \



#-------------------------------------------------
# additional core files needed for the debugger
#-------------------------------------------------

ifdef DEBUG
ifdef NEW_DEBUGGER
COREOBJS += \
	$(OBJ)/debug/debugcmd.o \
	$(OBJ)/debug/debugcpu.o \
	$(OBJ)/debug/express.o \
	$(OBJ)/debug/debugvw.o \
	$(OBJ)/debug/debughlp.o \
	$(OBJ)/debug/debugcon.o
else
COREOBJS += \
	$(OBJ)/debug/mamedbg.o \
	$(OBJ)/debug/window.o
endif
endif



#-------------------------------------------------
# set of tool targets
#-------------------------------------------------

TOOLS = romcmp$(EXE) chdman$(EXE) xml2info$(EXE)
