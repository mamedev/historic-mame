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
	$(OBJ)/audit.o \
	$(OBJ)/cdrom.o \
	$(OBJ)/chd.o \
	$(OBJ)/cheat.o \
	$(OBJ)/config.o \
	$(OBJ)/cpuexec.o \
	$(OBJ)/cpuint.o \
	$(OBJ)/cpuintrf.o \
	$(OBJ)/drawgfx.o \
	$(OBJ)/driver.o \
	$(OBJ)/fileio.o \
	$(OBJ)/harddisk.o \
	$(OBJ)/hash.o \
	$(OBJ)/info.o \
	$(OBJ)/input.o \
	$(OBJ)/inptport.o \
	$(OBJ)/jedparse.o \
	$(OBJ)/mame.o \
	$(OBJ)/mamecore.o \
	$(OBJ)/md5.o \
	$(OBJ)/memory.o \
	$(OBJ)/options.o \
	$(OBJ)/output.o \
	$(OBJ)/palette.o \
	$(OBJ)/png.o \
	$(OBJ)/render.o \
	$(OBJ)/rendfont.o \
	$(OBJ)/rendlay.o \
	$(OBJ)/rendutil.o \
	$(OBJ)/restrack.o \
	$(OBJ)/romload.o \
	$(OBJ)/sha1.o \
	$(OBJ)/sound.o \
	$(OBJ)/sndintrf.o \
	$(OBJ)/state.o \
	$(OBJ)/streams.o \
	$(OBJ)/tilemap.o \
	$(OBJ)/timer.o \
	$(OBJ)/ui.o \
	$(OBJ)/uigfx.o \
	$(OBJ)/uimenu.o \
	$(OBJ)/uitext.o \
	$(OBJ)/unzip.o \
	$(OBJ)/validity.o \
	$(OBJ)/version.o \
	$(OBJ)/video.o \
	$(OBJ)/xmlfile.o \
	$(OBJ)/sound/filter.o \
	$(OBJ)/sound/flt_vol.o \
	$(OBJ)/sound/flt_rc.o \
	$(OBJ)/sound/wavwrite.o \
	$(OBJ)/machine/eeprom.o \
	$(OBJ)/machine/generic.o \
	$(OBJ)/sndhrdw/generic.o \
	$(OBJ)/vidhrdw/generic.o \
	$(OBJ)/vidhrdw/vector.o \

ifdef X86_MIPS3_DRC
COREOBJS += $(OBJ)/x86drc.o
else
ifdef X86_PPC_DRC
COREOBJS += $(OBJ)/x86drc.o
endif
endif



#-------------------------------------------------
# additional dependencies
#-------------------------------------------------

$(OBJ)/video.o: rendersw.c



#-------------------------------------------------
# core layouts
#-------------------------------------------------

$(OBJ)/rendlay.o:	$(OBJ)/layout/dualhovu.lh \
					$(OBJ)/layout/dualhsxs.lh \
					$(OBJ)/layout/dualhuov.lh \
					$(OBJ)/layout/horizont.lh \
					$(OBJ)/layout/triphsxs.lh \
					$(OBJ)/layout/vertical.lh \
					$(OBJ)/layout/ho20ffff.lh \
					$(OBJ)/layout/ho2eff2e.lh \
					$(OBJ)/layout/ho4f893d.lh \
					$(OBJ)/layout/ho88ffff.lh \
					$(OBJ)/layout/hoa0a0ff.lh \
					$(OBJ)/layout/hoffe457.lh \
					$(OBJ)/layout/hoffff20.lh \
					$(OBJ)/layout/voffff20.lh \

$(OBJ)/video.o:		$(OBJ)/layout/snap.lh



#-------------------------------------------------
# additional core files needed for the debugger
#-------------------------------------------------

ifdef DEBUG
COREOBJS += \
	$(OBJ)/profiler.o \
	$(OBJ)/debug/debugcmd.o \
	$(OBJ)/debug/debugcmt.o \
	$(OBJ)/debug/debugcon.o \
	$(OBJ)/debug/debugcpu.o \
	$(OBJ)/debug/debughlp.o \
	$(OBJ)/debug/debugvw.o \
	$(OBJ)/debug/express.o \
	$(OBJ)/debug/textbuf.o
endif



#-------------------------------------------------
# set of tool targets
#-------------------------------------------------

TOOLS += romcmp$(EXE) chdman$(EXE) jedutil$(EXE) file2str$(EXE)
