CC	= gcc
LD	= gcc

DEFS   = -DX86_ASM -DLSB_FIRST
CFLAGS = -Isrc -Isrc/Z80 -Isrc/M6502 -Isrc/I86 -Isrc/M6809 \
         -fstrength-reduce -fomit-frame-pointer -O3 -m486 -Wall
LIBS   = -lalleg
OBJS   = obj/mame.o obj/common.o obj/driver.o obj/cpuintrf.o obj/osdepend.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/psg.o obj/sndhrdw/8910intf.o obj/sndhrdw/pokey.o obj/sndhrdw/sn76496.o \
         obj/machine/pacman.o obj/vidhrdw/pacman.o obj/drivers/pacman.o \
         obj/drivers/jrpacman.o obj/vidhrdw/jrpacman.o \
         obj/vidhrdw/pengo.o obj/sndhrdw/pengo.o obj/drivers/pengo.o \
         obj/machine/ladybug.o obj/vidhrdw/ladybug.o obj/sndhrdw/ladybug.o obj/drivers/ladybug.o \
         obj/machine/mrdo.o obj/vidhrdw/mrdo.o obj/drivers/mrdo.o \
         obj/machine/docastle.o obj/vidhrdw/docastle.o obj/drivers/docastle.o \
         obj/vidhrdw/cclimber.o obj/sndhrdw/cclimber.o obj/drivers/cclimber.o \
         obj/machine/seicross.o obj/sndhrdw/seicross.o obj/vidhrdw/seicross.o obj/drivers/seicross.o \
         obj/vidhrdw/ckong.o obj/drivers/ckong.o \
         obj/drivers/ckongs.o \
         obj/vidhrdw/dkong.o obj/sndhrdw/dkong.o obj/drivers/dkong.o \
         obj/vidhrdw/dkong3.o obj/drivers/dkong3.o \
         obj/machine/bagman.o obj/vidhrdw/bagman.o obj/sndhrdw/bagman.o obj/drivers/bagman.o \
         obj/machine/wow.o obj/vidhrdw/wow.o obj/drivers/wow.o \
         obj/drivers/galaxian.o \
         obj/vidhrdw/mooncrst.o obj/sndhrdw/mooncrst.o obj/drivers/mooncrst.o \
         obj/vidhrdw/moonqsr.o obj/drivers/moonqsr.o \
         obj/vidhrdw/frogger.o obj/sndhrdw/frogger.o obj/drivers/frogger.o \
         obj/machine/scramble.o obj/vidhrdw/scramble.o obj/sndhrdw/scramble.o obj/drivers/scramble.o \
         obj/drivers/scobra.o \
         obj/machine/warpwarp.o obj/vidhrdw/warpwarp.o obj/drivers/warpwarp.o \
         obj/vidhrdw/popeye.o obj/sndhrdw/popeye.o obj/drivers/popeye.o \
         obj/vidhrdw/amidar.o obj/sndhrdw/amidar.o obj/drivers/amidar.o \
         obj/vidhrdw/rallyx.o obj/drivers/rallyx.o \
         obj/vidhrdw/pooyan.o obj/sndhrdw/pooyan.o obj/drivers/pooyan.o \
         obj/sndhrdw/timeplt.o obj/vidhrdw/timeplt.o obj/drivers/timeplt.o \
         obj/machine/phoenix.o obj/vidhrdw/phoenix.o obj/sndhrdw/phoenix.o obj/drivers/phoenix.o \
         obj/machine/carnival.o obj/vidhrdw/carnival.o obj/drivers/carnival.o \
         obj/machine/invaders.o obj/vidhrdw/invaders.o obj/sndhrdw/invaders.o obj/drivers/invaders.o \
         obj/vidhrdw/mario.o obj/drivers/mario.o \
         obj/machine/zaxxon.o obj/vidhrdw/zaxxon.o obj/drivers/zaxxon.o \
         obj/vidhrdw/congo.o obj/drivers/congo.o \
         obj/vidhrdw/bombjack.o obj/sndhrdw/bombjack.o obj/drivers/bombjack.o \
         obj/machine/centiped.o obj/vidhrdw/centiped.o obj/drivers/centiped.o \
         obj/vidhrdw/milliped.o obj/sndhrdw/milliped.o obj/drivers/milliped.o \
         obj/machine/nibbler.o obj/vidhrdw/nibbler.o obj/drivers/nibbler.o \
         obj/machine/mpatrol.o obj/vidhrdw/mpatrol.o obj/drivers/mpatrol.o \
         obj/machine/btime.o obj/vidhrdw/btime.o obj/sndhrdw/btime.o obj/drivers/btime.o \
         obj/sndhrdw/jumpbug.o obj/drivers/jumpbug.o \
         obj/machine/vanguard.o obj/vidhrdw/vanguard.o obj/drivers/vanguard.o \
         obj/machine/gberet.o obj/vidhrdw/gberet.o obj/sndhrdw/gberet.o obj/drivers/gberet.o \
         obj/vidhrdw/venture.o obj/drivers/venture.o \
         obj/drivers/pepper2.o \
         obj/vidhrdw/gottlieb.o obj/drivers/reactor.o \
         obj/sndhrdw/gottlieb.o obj/drivers/qbert.o \
         obj/machine/gottlieb.o obj/drivers/krull.o \
         obj/drivers/qbertqub.o \
         obj/drivers/mplanets.o \
         obj/vidhrdw/junglek.o obj/sndhrdw/junglek.o obj/drivers/junglek.o \
         obj/machine/elevator.o obj/sndhrdw/elevator.o obj/vidhrdw/elevator.o obj/drivers/elevator.o \
         obj/machine/panic.o obj/vidhrdw/panic.o obj/drivers/panic.o \
         obj/machine/arabian.o obj/vidhrdw/arabian.o obj/sndhrdw/arabian.o obj/drivers/arabian.o \
         obj/machine/1942.o obj/vidhrdw/1942.o obj/sndhrdw/1942.o obj/drivers/1942.o \
         obj/sndhrdw/gyruss.o obj/vidhrdw/gyruss.o obj/drivers/gyruss.o \
         obj/machine/superpac.o obj/vidhrdw/superpac.o obj/drivers/superpac.o \
         obj/machine/galaga.o obj/vidhrdw/galaga.o obj/drivers/galaga.o \
         obj/machine/kangaroo.o obj/sndhrdw/kangaroo.o obj/vidhrdw/kangaroo.o obj/drivers/kangaroo.o \
         obj/vidhrdw/commando.o obj/drivers/commando.o \
         obj/machine/gng.o obj/vidhrdw/gng.o obj/drivers/gng.o \
         obj/machine/vulgus.o obj/sndhrdw/vulgus.o obj/vidhrdw/vulgus.o obj/drivers/vulgus.o \
         obj/sndhrdw/kungfum.o obj/vidhrdw/kungfum.o obj/drivers/kungfum.o \
         obj/machine/qix.o obj/vidhrdw/qix.o obj/drivers/qix.o \
         obj/machine/williams.o obj/sndhrdw/williams.o obj/vidhrdw/williams.o obj/drivers/williams.o \
         obj/Z80/Z80.o obj/M6502/M6502.o obj/I86/I86.o obj/M6809/M6809.o

VPATH = src src/Z80 src/M6502 src/I86 src/M6809

all: mame.exe

mame.exe:  $(OBJS)
	$(LD) -s -o mame.exe $(OBJS) $(DJDIR)/lib/audiodjf.a $(LIBS)

obj/osdepend.o: src/msdos/msdos.c
	 $(CC) $(DEFS) $(CFLAGS) -Isrc/msdos -o $@ -c $<

obj/%.o: src/%.c mame.h common.h driver.h
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/Z80/Z80.o:  Z80.c Z80.h Z80Codes.h Z80IO.h Z80DAA.h
obj/M6502/M6502.o:	M6502.c M6502.h Tables.h Codes.h
obj/I86/I86.o:  I86.c I86.h I86intrf.h ea.h host.h instr.h modrm.h
obj/M6809/M6809.o:  M6809.c M6809.h


clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\I86\*.o
	del obj\M6809\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del mame.exe
