CC	= gcc
LD	= gcc

DEFS   = -DX86_ASM -DLSB_FIRST
CFLAGS = -Isrc -Isrc/z80 -funroll-loops -fomit-frame-pointer -O3 -m486 -Wall
LIBS   = -lalleg
OBJS   = obj/mame.o obj/common.o obj/machine.o obj/driver.o obj/osdepend.o \
         obj/pacman/machine.o \
		 obj/pacman/driver.o obj/crush/machine.o obj/crush/driver.o \
         obj/pengo/machine.o obj/pengo/vidhrdw.o obj/pengo/sndhrdw.o obj/pengo/driver.o \
         obj/ladybug/machine.o obj/ladybug/vidhrdw.o obj/ladybug/sndhrdw.o obj/ladybug/driver.o \
         obj/Z80/Z80.o

VPATH = src src/z80

all: mame.exe

mame.exe:  $(OBJS)
	$(LD) -s -o mame.exe $(OBJS) $(DJDIR)/lib/audiodjf.a $(LIBS)

obj/osdepend.o: src/msdos/msdos.c
	 $(CC) $(DEFS) $(CFLAGS) -Isrc/msdos -o $@ -c $<

obj/%.o: src/%.c
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/%.o:	    common.h machine.h driver.h
obj/Z80.o:  Z80.c Z80.h Z80Codes.h Z80IO.h DAA.h


clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\pacman\*.o
	del obj\crush\*.o
	del obj\pengo\*.o
	del obj\ladybug\*.o
	del mame.exe
