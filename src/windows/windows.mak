###########################################################################
#
#   windows.mak
#
#   Windows-specific makefile
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# nasm for Windows (but not cygwin) has a "w"
# at the end
#-------------------------------------------------

ifndef COMPILESYSTEM_CYGWIN
ASM = @nasmw
endif



#-------------------------------------------------
# due to quirks of using /bin/sh, we need to
# explicitly specify the current path
#-------------------------------------------------

CURPATH = ./



#-------------------------------------------------
# Windows-specific flags and libararies
#-------------------------------------------------

# add our prefix files to the mix
CFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h

# add the windows libaries
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm



#-------------------------------------------------
# Windows-specific objects
#-------------------------------------------------

OSOBJS = \
	$(OBJ)/$(MAMEOS)/asmblit.o \
	$(OBJ)/$(MAMEOS)/asmtile.o \
	$(OBJ)/$(MAMEOS)/blit.o \
	$(OBJ)/$(MAMEOS)/config.o \
	$(OBJ)/$(MAMEOS)/fileio.o \
	$(OBJ)/$(MAMEOS)/fronthlp.o \
	$(OBJ)/$(MAMEOS)/input.o \
	$(OBJ)/$(MAMEOS)/misc.o \
	$(OBJ)/$(MAMEOS)/rc.o \
	$(OBJ)/$(MAMEOS)/sound.o \
	$(OBJ)/$(MAMEOS)/ticker.o \
	$(OBJ)/$(MAMEOS)/video.o \
	$(OBJ)/$(MAMEOS)/window.o \
	$(OBJ)/$(MAMEOS)/wind3d.o \
	$(OBJ)/$(MAMEOS)/wind3dfx.o \
	$(OBJ)/$(MAMEOS)/winddraw.o \
	$(OBJ)/$(MAMEOS)/winmain.o \

# add debug-specific files
ifdef DEBUG
OSOBJS += \
	$(OBJ)/$(MAMEOS)/debugwin.o
endif

# non-UI builds need a stub resource file
ifeq ($(WINUI),)
OSOBJS += $(OBJ)/$(MAMEOS)/mame.res
endif



#-------------------------------------------------
# Windows-specific debug objects and flags
#-------------------------------------------------

OSDBGOBJS =
OSDBGLDFLAGS =

# debug build: enable guard pages on all memory allocations
ifdef DEBUG
DEFS += -DMALLOC_DEBUG
OSDBGOBJS += $(OBJ)/$(MAMEOS)/winalloc.o
OSDBGLDFLAGS += -Wl,--allow-multiple-definition
endif



#-------------------------------------------------
# rules for assembly targets
#-------------------------------------------------

# video blitting functions
$(OBJ)/$(MAMEOS)/asmblit.o: src/$(MAMEOS)/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# tilemap blitting functions
$(OBJ)/$(MAMEOS)/asmtile.o: src/$(MAMEOS)/asmtile.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<



#-------------------------------------------------
# if building with a UI, set the C flags and
# include the ui.mak
#-------------------------------------------------

ifneq ($(WINUI),)
CFLAGS += -DWINUI=1
include src/ui/ui.mak
endif



#-------------------------------------------------
# configure resources
#-------------------------------------------------

RC = @windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/$(MAMEOS)



#-------------------------------------------------
# generic rule for the resource compiler
#-------------------------------------------------

$(OBJ)/$(MAMEOS)/%.res: src/$(MAMEOS)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<
