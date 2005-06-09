# the core object files (without target specific objects;
# those are added in the target.mak files)
COREOBJS = $(OBJ)/version.o $(OBJ)/mame.o \
	$(OBJ)/drawgfx.o $(OBJ)/common.o $(OBJ)/usrintrf.o $(OBJ)/ui_text.o \
	$(OBJ)/cpuintrf.o $(OBJ)/cpuexec.o $(OBJ)/cpuint.o $(OBJ)/memory.o $(OBJ)/timer.o \
	$(OBJ)/palette.o $(OBJ)/input.o $(OBJ)/inptport.o $(OBJ)/config.o $(OBJ)/unzip.o \
	$(OBJ)/audit.o $(OBJ)/info.o $(OBJ)/png.o $(OBJ)/artwork.o \
	$(OBJ)/tilemap.o $(OBJ)/fileio.o \
	$(OBJ)/state.o $(OBJ)/datafile.o $(OBJ)/hiscore.o \
	$(sort $(CPUOBJS)) \
	$(OBJ)/sndintrf.o \
	$(OBJ)/sound/streams.o $(OBJ)/sound/filter.o \
	$(OBJ)/sound/flt_vol.o $(OBJ)/sound/flt_rc.o \
	$(sort $(SOUNDOBJS)) \
	$(OBJ)/vidhrdw/generic.o $(OBJ)/vidhrdw/vector.o \
	$(OBJ)/machine/eeprom.o \
	$(OBJ)/profiler.o \
	$(OBJ)/hash.o $(OBJ)/sha1.o \
	$(OBJ)/chd.o $(OBJ)/harddisk.o $(OBJ)/md5.o \
	$(OBJ)/cdrom.o \
	$(OBJ)/sound/wavwrite.o

ifdef NEW_DEBUGGER
COREOBJS += $(OBJ)/debug/debugcmd.o $(OBJ)/debug/debugcpu.o $(OBJ)/debug/express.o \
			$(OBJ)/debug/debugvw.o $(OBJ)/debug/debughlp.o $(OBJ)/debug/debugcon.o
else
COREOBJS += $(OBJ)/debug/mamedbg.o $(OBJ)/debug/window.o
endif

ifdef X86_MIPS3_DRC
COREOBJS += $(OBJ)/x86drc.o
endif

COREOBJS += $(sort $(DBGOBJS))

TOOLS = romcmp$(EXE) chdman$(EXE) xml2info$(EXE)
