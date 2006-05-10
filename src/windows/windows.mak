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


###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################


#-------------------------------------------------
# specify build options; see each option below
# for details
#-------------------------------------------------

# uncomment next line to enable a build using Microsoft tools
# MSVC_BUILD = 1

# uncomment next line to enable multi-monitor stubs on Windows 95/NT
# you will need to find multimon.h and put it into your include
# path in order to make this work
# WIN95_MULTIMON = 1



###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################


#-------------------------------------------------
# configure the resource compiler
#-------------------------------------------------

RC = @windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/$(MAMEOS)



#-------------------------------------------------
# overrides for the MSVC compiler
#-------------------------------------------------

ifdef MSVC_BUILD

# replace the various compilers with vconv.exe prefixes
CC = @$(OBJ)/vconv.exe gcc /wd4025 /I.
LD = @$(OBJ)/vconv.exe ld /subsystem:console /debug
AR = @$(OBJ)/vconv.exe ar
RC = @$(OBJ)/vconv.exe windres

# turn on link-time codegen if the MAXOPT flag is also set
ifdef MAXOPT
CC += /GL
LD += /LTCG
endif

# filter X86_ASM define
DEFS := $(filter-out -DX86_ASM,$(DEFS))

# add some VC++-specific defines
DEFS += -DNONAMELESSUNION -D_CRT_SECURE_NO_DEPRECATE -DXML_STATIC -D__inline__=__inline -Dsnprintf=_snprintf -Dvsnprintf=_vsnprintf

# make msvcprep into a pre-build step
OSPREBUILD = msvcprep

# rules for building vconv using the mingw tools for bootstrapping
msvcprep: $(OBJ)/vconv.exe

$(OBJ)/vconv.exe: $(OBJ)/windows/vconv.o
	@gcc $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@

$(OBJ)/windows/vconv.o: src/windows/vconv.c
	@gcc $(CDEFS) $(CFLAGSOSDEPEND) -c $< -o $@

endif



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

ifdef WIN95_MULTIMON
CFLAGS += -DWIN95_MULTIMON
endif

# add the windows libaries
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm -ladvapi32 -lshell32



#-------------------------------------------------
# Windows-specific objects
#-------------------------------------------------

OSOBJS = \
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

OSTOOLOBJS = \
	$(OBJ)/$(MAMEOS)/osd_tool.o

# add 32-bit optimized blitters
ifndef PTR64
OSOBJS += \
	$(OBJ)/$(MAMEOS)/asmblit.o \
	$(OBJ)/$(MAMEOS)/asmtile.o
endif

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
# generic rule for the resource compiler
#-------------------------------------------------

$(OBJ)/$(MAMEOS)/%.res: src/$(MAMEOS)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<
