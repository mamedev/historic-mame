CC	= gcc
LD	= gcc

DEFS   = -DX86_ASM -DLSB_FIRST
CFLAGS = -Isrc -Isrc/z80 -fstrength-reduce -funroll-loops -fomit-frame-pointer -O3 -m486 -Wall
#CFLAGS = -pg -Isrc -Isrc/z80 -fstrength-reduce -funroll-loops -O3 -m486 -Wall
LIBS   = -lalleg
OBJS   = obj/mame.o obj/common.o obj/machine.o obj/driver.o obj/osdepend.o \
         obj/machine/pacman.o obj/vidhrdw/pacman.o obj/drivers/pacman.o \
		 obj/drivers/crush.o \
         obj/vidhrdw/pengo.o obj/sndhrdw/pengo.o obj/drivers/pengo.o \
         obj/machine/ladybug.o obj/vidhrdw/ladybug.o obj/sndhrdw/ladybug.o obj/drivers/ladybug.o \
         obj/machine/mrdo.o obj/vidhrdw/mrdo.o obj/drivers/mrdo.o \
         obj/vidhrdw/cclimber.o obj/sndhrdw/cclimber.o obj/drivers/cclimber.o \
         obj/vidhrdw/ckong.o obj/drivers/ckong.o \
         obj/vidhrdw/dkong.o obj/drivers/dkong.o \
         obj/machine/bagman.o obj/vidhrdw/bagman.o obj/drivers/bagman.o \
         obj/vidhrdw/wow.o obj/drivers/wow.o \
         obj/drivers/galaxian.o \
         obj/vidhrdw/mooncrst.o obj/sndhrdw/mooncrst.o obj/drivers/mooncrst.o \
         obj/drivers/theend.o \
         obj/vidhrdw/frogger.o obj/drivers/frogger.o \
         obj/machine/scramble.o obj/vidhrdw/scramble.o obj/drivers/scramble.o \
         obj/drivers/scobra.o \
         obj/vidhrdw/amidar.o obj/drivers/amidar.o \
         obj/vidhrdw/rallyx.o obj/drivers/rallyx.o \
         obj/vidhrdw/pooyan.o obj/drivers/pooyan.o \
         obj/machine/phoenix.o obj/vidhrdw/phoenix.o obj/drivers/phoenix.o \
         obj/Z80/Z80.o

VPATH = src src/z80

all: mame.exe

mame.exe:  $(OBJS)
	$(LD) -s -o mame.exe $(OBJS) $(DJDIR)/lib/audiodjf.a $(LIBS)
#	$(LD) -pg -o mame.exe $(OBJS) $(DJDIR)/lib/audiodjf.a $(LIBS)

obj/osdepend.o: src/msdos/msdos.c
	 $(CC) $(DEFS) $(CFLAGS) -Isrc/msdos -o $@ -c $<

obj/%.o: src/%.c common.h machine.h driver.h
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/sndhrdw/cclimber.o:  src/sndhrdw/psg.c src/sndhrdw/psg.h
obj/Z80/Z80.o:  Z80.c Z80.h Z80Codes.h Z80IO.h Z80DAA.h


clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del mame.exe
