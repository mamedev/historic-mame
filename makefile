CC      = gcc
LD	= gcc

DEFS   = -DX86_ASM -DLSB_FIRST
CFLAGS = -Isrc -Isrc/Z80 -Isrc/M6502 -Isrc/I86 -Isrc/M6809 -Isrc/M68000 \
         -fstrength-reduce -fomit-frame-pointer -O3 -m486 -Wall -Werror
LIBS   = -lalleg
OBJS   = obj/mame.o obj/common.o obj/usrintrf.o obj/driver.o obj/cpuintrf.o obj/inptport.o \
         obj/osdepend.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/psg.o obj/sndhrdw/8910intf.o \
		 obj/sndhrdw/pokey.o obj/sndhrdw/pokyintf.o obj/sndhrdw/sn76496.o \
		 obj/sndhrdw/votrax.o \
         obj/machine/z80fmly.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/vidhrdw/vector.o obj/machine/mathbox.o \
         obj/machine/pacman.o obj/drivers/pacman.o \
         obj/machine/jrpacman.o obj/drivers/jrpacman.o obj/vidhrdw/jrpacman.o \
         obj/vidhrdw/pengo.o obj/sndhrdw/pengo.o obj/drivers/pengo.o \
         obj/vidhrdw/ladybug.o obj/sndhrdw/ladybug.o obj/drivers/ladybug.o \
         obj/machine/mrdo.o obj/vidhrdw/mrdo.o obj/drivers/mrdo.o \
         obj/machine/docastle.o obj/vidhrdw/docastle.o obj/sndhrdw/docastle.o obj/drivers/docastle.o \
         obj/drivers/dowild.o \
         obj/vidhrdw/cclimber.o obj/sndhrdw/cclimber.o obj/drivers/cclimber.o \
         obj/drivers/ckongs.o \
         obj/machine/seicross.o obj/sndhrdw/seicross.o obj/vidhrdw/seicross.o obj/drivers/seicross.o \
         obj/vidhrdw/dkong.o obj/sndhrdw/dkong.o obj/drivers/dkong.o \
         obj/machine/bagman.o obj/vidhrdw/bagman.o obj/sndhrdw/bagman.o obj/drivers/bagman.o \
         obj/machine/wow.o obj/vidhrdw/wow.o obj/sndhrdw/wow.o obj/drivers/wow.o \
         obj/vidhrdw/galaxian.o obj/drivers/galaxian.o \
         obj/sndhrdw/mooncrst.o obj/drivers/mooncrst.o \
         obj/drivers/moonqsr.o \
         obj/vidhrdw/frogger.o obj/sndhrdw/frogger.o obj/drivers/frogger.o \
         obj/machine/scramble.o obj/sndhrdw/scramble.o obj/drivers/scramble.o \
         obj/drivers/scobra.o \
         obj/machine/warpwarp.o obj/vidhrdw/warpwarp.o obj/drivers/warpwarp.o \
         obj/vidhrdw/popeye.o obj/sndhrdw/popeye.o obj/drivers/popeye.o \
         obj/vidhrdw/amidar.o obj/sndhrdw/amidar.o obj/drivers/amidar.o \
         obj/vidhrdw/rallyx.o obj/drivers/rallyx.o \
         obj/vidhrdw/pooyan.o obj/sndhrdw/pooyan.o obj/drivers/pooyan.o \
         obj/sndhrdw/timeplt.o obj/vidhrdw/timeplt.o obj/drivers/timeplt.o \
         obj/vidhrdw/locomotn.o obj/drivers/locomotn.o \
         obj/machine/phoenix.o obj/vidhrdw/phoenix.o obj/sndhrdw/phoenix.o obj/drivers/phoenix.o \
         obj/sndhrdw/pleiads.o \
         obj/machine/carnival.o obj/vidhrdw/carnival.o obj/sndhrdw/carnival.o obj/drivers/carnival.o \
         obj/machine/invaders.o obj/vidhrdw/invaders.o obj/sndhrdw/invaders.o obj/drivers/invaders.o \
         obj/vidhrdw/mario.o obj/drivers/mario.o \
         obj/vidhrdw/zaxxon.o obj/drivers/zaxxon.o \
         obj/vidhrdw/congo.o obj/drivers/congo.o \
         obj/vidhrdw/bombjack.o obj/sndhrdw/bombjack.o obj/drivers/bombjack.o \
         obj/machine/centiped.o obj/vidhrdw/centiped.o obj/drivers/centiped.o \
         obj/machine/milliped.o obj/vidhrdw/milliped.o obj/drivers/milliped.o \
         obj/machine/warlord.o obj/vidhrdw/warlord.o obj/drivers/warlord.o \
         obj/machine/vanguard.o obj/vidhrdw/vanguard.o obj/sndhrdw/vanguard.o obj/drivers/vanguard.o \
         obj/vidhrdw/nibbler.o obj/drivers/nibbler.o \
         obj/machine/mpatrol.o obj/vidhrdw/mpatrol.o obj/drivers/mpatrol.o \
         obj/vidhrdw/btime.o obj/sndhrdw/btime.o obj/drivers/btime.o \
         obj/sndhrdw/jumpbug.o obj/vidhrdw/jumpbug.o obj/drivers/jumpbug.o \
         obj/machine/gberet.o obj/vidhrdw/gberet.o obj/sndhrdw/gberet.o obj/drivers/gberet.o \
         obj/vidhrdw/exidy.o obj/drivers/venture.o obj/drivers/pepper2.o obj/drivers/mtrap.o \
         obj/vidhrdw/gottlieb.o obj/drivers/reactor.o \
         obj/sndhrdw/gottlieb.o obj/drivers/qbert.o \
         obj/machine/gottlieb.o obj/drivers/krull.o \
         obj/drivers/qbertqub.o \
         obj/drivers/mplanets.o \
         obj/drivers/3stooges.o \
         obj/vidhrdw/taito.o obj/sndhrdw/junglek.o obj/drivers/junglek.o \
         obj/machine/elevator.o obj/sndhrdw/elevator.o obj/drivers/elevator.o \
         obj/drivers/wwestern.o \
         obj/machine/panic.o obj/vidhrdw/panic.o obj/drivers/panic.o \
         obj/machine/arabian.o obj/vidhrdw/arabian.o obj/sndhrdw/arabian.o obj/drivers/arabian.o \
         obj/machine/1942.o obj/vidhrdw/1942.o obj/drivers/1942.o obj/sndhrdw/capcom.o \
         obj/machine/vulgus.o obj/sndhrdw/vulgus.o obj/vidhrdw/vulgus.o obj/drivers/vulgus.o \
         obj/vidhrdw/commando.o obj/drivers/commando.o \
         obj/machine/gng.o obj/vidhrdw/gng.o obj/drivers/gng.o \
         obj/vidhrdw/sonson.o obj/drivers/sonson.o \
         obj/vidhrdw/exedexes.o obj/drivers/exedexes.o \
         obj/sndhrdw/gyruss.o obj/vidhrdw/gyruss.o obj/drivers/gyruss.o \
         obj/machine/superpac.o obj/vidhrdw/superpac.o obj/drivers/superpac.o \
         obj/machine/galaga.o obj/vidhrdw/galaga.o obj/drivers/galaga.o \
         obj/machine/kangaroo.o obj/sndhrdw/kangaroo.o obj/vidhrdw/kangaroo.o obj/drivers/kangaroo.o \
         obj/sndhrdw/kungfum.o obj/vidhrdw/kungfum.o obj/drivers/kungfum.o \
         obj/machine/qix.o obj/vidhrdw/qix.o obj/drivers/qix.o \
         obj/machine/williams.o obj/sndhrdw/williams.o obj/vidhrdw/williams.o obj/drivers/williams.o \
         obj/machine/starforc.o obj/sndhrdw/starforc.o obj/vidhrdw/starforc.o obj/drivers/starforc.o \
         obj/machine/naughtyb.o obj/vidhrdw/naughtyb.o obj/drivers/naughtyb.o \
         obj/machine/mystston.o obj/vidhrdw/mystston.o obj/drivers/mystston.o \
         obj/machine/tutankhm.o obj/sndhrdw/tutankhm.o obj/vidhrdw/tutankhm.o obj/drivers/tutankhm.o \
         obj/machine/spacefb.o obj/vidhrdw/spacefb.o obj/drivers/spacefb.o \
         obj/machine/mappy.o obj/sndhrdw/mappy.o obj/vidhrdw/mappy.o obj/drivers/mappy.o \
         obj/machine/ccastles.o obj/vidhrdw/ccastles.o obj/drivers/ccastles.o \
         obj/machine/yiear.o obj/vidhrdw/yiear.o obj/drivers/yiear.o \
         obj/machine/digdug.o obj/vidhrdw/digdug.o obj/drivers/digdug.o \
         obj/machine/asteroid.o obj/sndhrdw/asteroid.o \
		 obj/machine/atari_vg.o obj/vidhrdw/atari_vg.o obj/drivers/asteroid.o \
         obj/drivers/bwidow.o \
         obj/machine/bzone.o obj/drivers/bzone.o \
         obj/drivers/llander.o obj/machine/llander.o \
         obj/drivers/redbaron.o obj/machine/redbaron.o \
         obj/machine/spacduel.o \
         obj/drivers/tempest.o obj/machine/tempest.o \
         obj/machine/starwars.o obj/machine/swmathbx.o obj/drivers/starwars.o obj/sndhrdw/starwars.o \
         obj/machine/missile.o obj/vidhrdw/missile.o obj/drivers/missile.o \
         obj/machine/bublbobl.o obj/vidhrdw/bublbobl.o obj/drivers/bublbobl.o \
         obj/vidhrdw/eggs.o obj/drivers/eggs.o \
         obj/machine/bosco.o obj/sndhrdw/bosco.o obj/vidhrdw/bosco.o obj/drivers/bosco.o \
         obj/vidhrdw/yard.o obj/drivers/yard.o \
         obj/vidhrdw/blueprnt.o obj/sndhrdw/blueprnt.o obj/drivers/blueprnt.o \
         obj/vidhrdw/sega.o obj/sndhrdw/sega.o obj/machine/sega.o obj/drivers/sega.o \
         obj/sndhrdw/omegrace.o obj/machine/omegrace.o obj/drivers/omegrace.o \
         obj/vidhrdw/xevious.o obj/machine/xevious.o obj/drivers/xevious.o \
         obj/vidhrdw/bankp.o obj/sndhrdw/bankp.o obj/drivers/bankp.o \
         obj/vidhrdw/sbasketb.o obj/drivers/sbasketb.o \
         obj/machine/mcr.o obj/sndhrdw/mcr.o \
         obj/vidhrdw/mcr1.o obj/vidhrdw/mcr2.o obj/vidhrdw/mcr3.o \
         obj/drivers/mcr1.o obj/drivers/mcr2.o obj/drivers/mcr3.o \
         obj/machine/espial.o obj/vidhrdw/espial.o obj/drivers/espial.o \
         obj/machine/tp84.o obj/vidhrdw/tp84.o obj/sndhrdw/tp84.o obj/drivers/tp84.o \
         obj/vidhrdw/rastan.o obj/sndhrdw/rastan.o obj/drivers/rastan.o \
         obj/Z80/Z80.o obj/M6502/M6502.o obj/I86/I86.o obj/M6809/m6809.o \
         obj/M68000/opcode0.o obj/M68000/opcode1.o obj/M68000/opcode2.o obj/M68000/opcode3.o obj/M68000/opcode4.o obj/M68000/opcode5.o \
         obj/M68000/opcode6.o obj/M68000/opcode7.o obj/M68000/opcode8.o obj/M68000/opcode9.o obj/M68000/opcodeb.o \
         obj/M68000/opcodec.o obj/M68000/opcoded.o obj/M68000/opcodee.o obj/M68000/mc68kmem.o \
         obj/M68000/cpufunc.o

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
obj/M6809/m6809.o:  m6809.c m6809.h 6809ops.c


clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\I86\*.o
	del obj\M6809\*.o
	del obj\M68000\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del mame.exe
