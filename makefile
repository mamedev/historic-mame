# set this to mame, mess or the destination you want to build
# TARGET = mame
# TARGET = mess
# example for a tiny compile
# TARGET = tiny
ifeq ($(TARGET),)
TARGET = mame
endif

# uncomment one of the next lines to build a target-optimized build
# ATHLON = 1
# I686 = 1
# P4 = 1
# PM = 1

# uncomment next line to include the symbols for symify
# SYMBOLS = 1

# uncomment next line to generate a link map for exception handling in windows
# MAP = 1

# uncomment next line to include the debugger
# DEBUG = 1

# uncomment next line to use the new multiwindow debugger
NEW_DEBUGGER = 1

# uncomment next line to use DRC MIPS3 engine
X86_MIPS3_DRC = 1

# uncomment next line to use DRC PowerPC engine
X86_PPC_DRC = 1

# uncomment next line to use cygwin compiler
# COMPILESYSTEM_CYGWIN	= 1

# uncomment next line to build expat as part of MAME build
BUILD_EXPAT = 1

# uncomment next line to build zlib as part of MAME build
BUILD_ZLIB = 1


# set this the operating system you're building for
# MAMEOS = msdos
# MAMEOS = windows
ifeq ($(MAMEOS),)
MAMEOS = windows
endif

# extension for executables
EXE = .exe

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
AR = @ar
CC = @gcc
LD = @gcc
ASM = @nasm
ASMFLAGS = -f coff
MD = -mkdir.exe
RM = @rm -f


ifeq ($(MAMEOS),msdos)
PREFIX = d
else
PREFIX =
endif

# by default, compile for Pentium target and add no suffix
NAME = $(PREFIX)$(TARGET)$(SUFFIX)
ARCH = -march=pentium

# architecture-specific builds get extra options and a suffix
ifdef ATHLON
NAME = $(PREFIX)$(TARGET)$(SUFFIX)at
ARCH = -march=athlon
endif

ifdef I686
NAME = $(PREFIX)$(TARGET)$(SUFFIX)pp
ARCH = -march=pentiumpro
endif

ifdef P4
NAME = $(PREFIX)$(TARGET)$(SUFFIX)p4
ARCH = -march=pentium4
endif

ifdef PM
NAME = $(PREFIX)$(TARGET)$(SUFFIX)pm
ARCH = -march=pentium3 -msse2
endif

# debug builds just get the 'd' suffix and nothing more
ifdef DEBUG
NAME = $(PREFIX)$(TARGET)$(SUFFIX)d
endif


# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
OBJ = obj/$(NAME)

EMULATOR = $(NAME)$(EXE)

DEFS = -DX86_ASM -DLSB_FIRST -DINLINE="static __inline__" -Dasm=__asm__ -DCRLF=3
ifdef NEW_DEBUGGER
DEFS += -DNEW_DEBUGGER
endif

CFLAGS = -std=gnu89 -Isrc -Isrc/includes -Isrc/debug -Isrc/$(MAMEOS) -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000

ifdef SYMBOLS
CFLAGS += -O0 -Wall -Wno-unused -g
else
CFLAGS += -DNDEBUG \
	$(ARCH) -O3 -fno-strict-aliasing \
	-Werror -Wall \
	-Wno-sign-compare \
	-Wno-unused-functions \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wcast-align \
	-Wstrict-prototypes \
	-Wundef \
	-Wformat-security \
	-Wwrite-strings \
	-Wdeclaration-after-statement
endif

# extra options needed *only* for the osd files
CFLAGSOSDEPEND = $(CFLAGS)

ifdef SYMBOLS
LDFLAGS =
else
#LDFLAGS = -s -Wl,--warn-common
LDFLAGS = -s
endif

ifdef MAP
MAPFLAGS = -Wl,-Map,$(NAME).map
else
MAPFLAGS =
endif

OBJDIRS = obj $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/$(MAMEOS) \
	$(OBJ)/drivers $(OBJ)/machine $(OBJ)/vidhrdw $(OBJ)/sndhrdw $(OBJ)/debug
ifdef MESS
OBJDIRS += $(OBJ)/mess $(OBJ)/mess/systems $(OBJ)/mess/machine \
	$(OBJ)/mess/vidhrdw $(OBJ)/mess/sndhrdw $(OBJ)/mess/tools
endif

# start with an empty set of libs
LIBS = 

ifdef BUILD_EXPAT
CFLAGS += -Isrc/expat
OBJDIRS += $(OBJ)/expat
EXPAT = $(OBJ)/libexpat.a
else
LIBS += -lexpat
EXPAT =
endif

ifdef BUILD_ZLIB
CFLAGS += -Isrc/zlib
OBJDIRS += $(OBJ)/zlib
ZLIB = $(OBJ)/libz.a
else
LIBS += -lz
ZLIB =
endif

all:	maketree emulator extra

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/$(MAMEOS)/$(MAMEOS).mak

ifdef DEBUG
DBGDEFS = -DMAME_DEBUG
else
DBGDEFS =
DBGOBJS =
endif

ifdef COMPILESYSTEM_CYGWIN
CFLAGS	+= -mno-cygwin
LDFLAGS	+= -mno-cygwin
endif

emulator: maketree $(EMULATOR)

extra:	$(TOOLS) $(TEXTS)

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)

# primary target
$(EMULATOR): $(OBJS) $(COREOBJS) $(OSOBJS) $(DRVLIBS) $(EXPAT) $(ZLIB) $(OSDBGOBJS)
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c src/version.c -o $(OBJ)/version.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@ $(MAPFLAGS)

romcmp$(EXE): $(OBJ)/romcmp.o $(OBJ)/unzip.o $(ZLIB) $(OSDBGOBJS)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@

chdman$(EXE): $(OBJ)/chdman.o $(OBJ)/chd.o $(OBJ)/chdcd.o $(OBJ)/cdrom.o $(OBJ)/md5.o $(OBJ)/sha1.o $(OBJ)/version.o $(ZLIB) $(OSDBGOBJS)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@

xml2info$(EXE): $(OBJ)/xml2info.o $(EXPAT) $(OSDBGOBJS)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@

# secondary libraries
$(OBJ)/libexpat.a: $(OBJ)/expat/xmlparse.o $(OBJ)/expat/xmlrole.o $(OBJ)/expat/xmltok.o

$(OBJ)/libz.a: $(OBJ)/zlib/adler32.o $(OBJ)/zlib/compress.o $(OBJ)/zlib/crc32.o $(OBJ)/zlib/deflate.o \
				$(OBJ)/zlib/gzio.o $(OBJ)/zlib/inffast.o $(OBJ)/zlib/inflate.o $(OBJ)/zlib/infback.o \
				$(OBJ)/zlib/inftrees.o $(OBJ)/zlib/trees.o $(OBJ)/zlib/uncompr.o $(OBJ)/zlib/zutil.o

$(OBJ)/$(MAMEOS)/%.o: src/$(MAMEOS)/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSOSDEPEND) -c $< -o $@

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

# compile generated C files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE)
	@echo Compiling $(subst .o,.c,$@)...
	$(CC) $(CDEFS) $(CFLAGS) -c $*.c -o $@

# additional rule, because m68kcpu.c includes the generated m68kops.h :-/
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE)

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake$(EXE): $(OBJ)/cpu/m68000/m68kmake.o $(OSDBGOBJS)
	@echo M68K make $<...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ -o $@
	@echo Generating M68K source files...
	$(OBJ)/cpu/m68000/m68kmake$(EXE) $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

$(OBJ)/%.a:
	@echo Archiving $@...
	$(RM) $@
	$(AR) cr $@ $^

$(sort $(OBJDIRS)):
	$(MD) $@

maketree: $(sort $(OBJDIRS))

clean:
	@echo Deleting object tree $(OBJ)...
	$(RM) -r $(OBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)
	@echo Deleting $(TOOLS)...
	$(RM) $(TOOLS)

check: $(EMULATOR) xml2info$(EXE)
	./$(EMULATOR) -listxml > $(NAME).xml
	./xml2info < $(NAME).xml > $(NAME).lst
	./xmllint --valid --noout $(NAME).xml
