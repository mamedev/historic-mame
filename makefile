# before compiling for the first time, use "make makedir" to create all the
# necessary subdirectories

CC = gcc
LD = gcc

# add -DMAME_DEBUG to include the debugger
DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES
#DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -DMAME_DEBUG
CFLAGS = -Isrc -Isrc/msdos -fomit-frame-pointer -O3 -mpentium -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-Wstrict-prototypes
#	-pedantic \
#	-Wredundant-decls \
#	-Wlarger-than-27648 \
#	-Wshadow \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
#CFLAGS = -Isrc -Isrc/msdos -O -mpentium -Wall -Werror -g
LDFLAGS = -s
#LDFLAGS =
LIBS   = -lalleg $(DJDIR)/lib/audiodjf.a
OBJS   = obj/mame.o obj/common.o obj/usrintrf.o obj/driver.o \
         obj/cpuintrf.o obj/memory.o obj/timer.o obj/gfxlayer.o \
         obj/inptport.o obj/cheat.o obj/unzip.o obj/inflate.o \
         obj/audit.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/psg.o obj/sndhrdw/psgintf.o \
         obj/sndhrdw/2151intf.o obj/sndhrdw/fm.o \
         obj/sndhrdw/ym2151.o obj/sndhrdw/ym3812.o \
		 obj/sndhrdw/tms5220.o obj/sndhrdw/5220intf.o obj/sndhrdw/vlm5030.o \
		 obj/sndhrdw/pokey.o obj/sndhrdw/pokyintf.o obj/sndhrdw/sn76496.o \
		 obj/sndhrdw/nes.o obj/sndhrdw/nesintf.o \
		 obj/sndhrdw/votrax.o obj/sndhrdw/dac.o obj/sndhrdw/samples.o \
         obj/machine/z80fmly.o obj/machine/6821pia.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/vidhrdw/vector.o obj/vidhrdw/avgdvg.o obj/machine/mathbox.o \
         obj/sndhrdw/namco.o \
         obj/machine/pacman.o obj/drivers/pacman.o \
         obj/drivers/maketrax.o \
         obj/machine/jrpacman.o obj/drivers/jrpacman.o obj/vidhrdw/jrpacman.o \
         obj/vidhrdw/pengo.o obj/drivers/pengo.o \
         obj/vidhrdw/ladybug.o obj/drivers/ladybug.o \
         obj/vidhrdw/mrdo.o obj/drivers/mrdo.o \
         obj/machine/docastle.o obj/vidhrdw/docastle.o obj/drivers/docastle.o \
         obj/drivers/dowild.o \
         obj/vidhrdw/cclimber.o obj/sndhrdw/cclimber.o obj/drivers/cclimber.o \
         obj/drivers/ckongs.o \
         obj/vidhrdw/seicross.o obj/drivers/seicross.o \
         obj/vidhrdw/dkong.o obj/sndhrdw/dkong.o obj/drivers/dkong.o \
         obj/machine/bagman.o obj/vidhrdw/bagman.o obj/drivers/bagman.o \
         obj/machine/wow.o obj/vidhrdw/wow.o obj/sndhrdw/wow.o obj/drivers/wow.o \
         obj/sndhrdw/gorf.o \
         obj/vidhrdw/galaxian.o obj/drivers/galaxian.o \
         obj/sndhrdw/mooncrst.o obj/drivers/mooncrst.o \
         obj/vidhrdw/frogger.o obj/sndhrdw/frogger.o obj/drivers/frogger.o \
         obj/machine/scramble.o obj/sndhrdw/scramble.o obj/drivers/scramble.o \
         obj/drivers/scobra.o \
         obj/vidhrdw/amidar.o obj/drivers/amidar.o \
         obj/machine/warpwarp.o obj/vidhrdw/warpwarp.o obj/drivers/warpwarp.o \
         obj/vidhrdw/popeye.o obj/drivers/popeye.o \
         obj/vidhrdw/rallyx.o obj/drivers/rallyx.o \
         obj/drivers/locomotn.o \
         obj/vidhrdw/pooyan.o obj/drivers/pooyan.o \
         obj/vidhrdw/timeplt.o obj/drivers/timeplt.o \
         obj/vidhrdw/phoenix.o obj/sndhrdw/phoenix.o obj/drivers/phoenix.o \
         obj/sndhrdw/pleiads.o \
         obj/machine/carnival.o obj/vidhrdw/carnival.o obj/sndhrdw/carnival.o obj/drivers/carnival.o \
         obj/machine/invaders.o obj/vidhrdw/invaders.o obj/sndhrdw/invaders.o obj/drivers/invaders.o \
         obj/vidhrdw/mario.o obj/sndhrdw/mario.o obj/drivers/mario.o \
         obj/vidhrdw/zaxxon.o obj/sndhrdw/zaxxon.o obj/drivers/zaxxon.o \
         obj/vidhrdw/congo.o obj/sndhrdw/congo.o obj/drivers/congo.o \
         obj/vidhrdw/bombjack.o obj/drivers/bombjack.o \
         obj/machine/centiped.o obj/vidhrdw/centiped.o obj/drivers/centiped.o \
         obj/machine/milliped.o obj/vidhrdw/milliped.o obj/drivers/milliped.o \
         obj/machine/warlord.o obj/vidhrdw/warlord.o obj/drivers/warlord.o \
         obj/vidhrdw/rockola.o obj/sndhrdw/rockola.o obj/drivers/rockola.o \
         obj/vidhrdw/mpatrol.o  obj/sndhrdw/mpatrol.o obj/drivers/mpatrol.o \
         obj/vidhrdw/travrusa.o obj/drivers/travrusa.o \
         obj/vidhrdw/btime.o obj/drivers/btime.o \
         obj/vidhrdw/bnj.o obj/drivers/bnj.o \
         obj/vidhrdw/jumpbug.o obj/drivers/jumpbug.o \
         obj/vidhrdw/gberet.o obj/drivers/gberet.o \
         obj/vidhrdw/exidy.o obj/drivers/exidy.o \
		 obj/sndhrdw/targ.o \
         obj/machine/gottlieb.o obj/vidhrdw/gottlieb.o obj/sndhrdw/gottlieb.o \
         obj/drivers/reactor.o obj/drivers/qbert.o obj/drivers/krull.o \
         obj/drivers/qbertqub.o obj/drivers/mplanets.o obj/drivers/3stooges.o \
         obj/machine/taito.o obj/vidhrdw/taito.o obj/drivers/taito.o \
         obj/machine/panic.o obj/vidhrdw/panic.o obj/drivers/panic.o \
         obj/machine/arabian.o obj/vidhrdw/arabian.o obj/drivers/arabian.o \
         obj/vidhrdw/1942.o obj/drivers/1942.o \
         obj/machine/vulgus.o obj/vidhrdw/vulgus.o obj/drivers/vulgus.o \
         obj/vidhrdw/commando.o obj/drivers/commando.o \
         obj/machine/gng.o obj/vidhrdw/gng.o obj/drivers/gng.o \
         obj/vidhrdw/sonson.o obj/drivers/sonson.o \
         obj/vidhrdw/exedexes.o obj/drivers/exedexes.o \
         obj/sndhrdw/gyruss.o obj/vidhrdw/gyruss.o obj/drivers/gyruss.o \
         obj/machine/superpac.o obj/vidhrdw/superpac.o obj/drivers/superpac.o \
         obj/machine/galaga.o obj/vidhrdw/galaga.o obj/drivers/galaga.o \
         obj/machine/kangaroo.o obj/vidhrdw/kangaroo.o obj/drivers/kangaroo.o \
         obj/vidhrdw/kungfum.o obj/drivers/kungfum.o \
         obj/machine/qix.o obj/vidhrdw/qix.o obj/drivers/qix.o \
         obj/machine/williams.o obj/vidhrdw/williams.o obj/drivers/williams.o \
         obj/machine/ticket.o \
         obj/sndhrdw/starforc.o obj/vidhrdw/starforc.o obj/drivers/starforc.o \
         obj/vidhrdw/naughtyb.o obj/drivers/naughtyb.o \
         obj/machine/mystston.o obj/vidhrdw/mystston.o obj/drivers/mystston.o \
         obj/vidhrdw/matmania.o obj/drivers/matmania.o \
         obj/vidhrdw/tutankhm.o obj/drivers/tutankhm.o \
         obj/machine/spacefb.o obj/vidhrdw/spacefb.o obj/drivers/spacefb.o \
         obj/machine/mappy.o obj/vidhrdw/mappy.o obj/drivers/mappy.o \
         obj/vidhrdw/ccastles.o obj/drivers/ccastles.o \
         obj/vidhrdw/yiear.o obj/sndhrdw/yiear.o obj/drivers/yiear.o \
         obj/machine/digdug.o obj/vidhrdw/digdug.o obj/drivers/digdug.o \
         obj/machine/asteroid.o obj/sndhrdw/asteroid.o \
		 obj/machine/atari_vg.o obj/drivers/asteroid.o \
         obj/drivers/bwidow.o \
         obj/sndhrdw/bzone.o  obj/drivers/bzone.o \
         obj/sndhrdw/redbaron.o \
         obj/drivers/tempest.o \
         obj/machine/starwars.o obj/machine/swmathbx.o obj/drivers/starwars.o obj/sndhrdw/starwars.o \
         obj/machine/mhavoc.o obj/drivers/mhavoc.o \
         obj/machine/quantum.o obj/drivers/quantum.o \
         obj/machine/missile.o obj/vidhrdw/missile.o obj/drivers/missile.o \
         obj/machine/bublbobl.o obj/vidhrdw/bublbobl.o obj/drivers/bublbobl.o \
         obj/vidhrdw/eggs.o obj/drivers/eggs.o \
         obj/machine/bosco.o obj/sndhrdw/bosco.o obj/vidhrdw/bosco.o obj/drivers/bosco.o \
         obj/vidhrdw/yard.o obj/drivers/yard.o \
         obj/vidhrdw/blueprnt.o obj/drivers/blueprnt.o \
         obj/vidhrdw/sega.o obj/sndhrdw/sega.o obj/machine/sega.o obj/drivers/sega.o \
         obj/vidhrdw/segar.o obj/sndhrdw/segar.o obj/machine/segar.o obj/drivers/segar.o \
         obj/sndhrdw/monsterb.o \
         obj/drivers/omegrace.o \
         obj/vidhrdw/xevious.o obj/machine/xevious.o obj/drivers/xevious.o \
         obj/vidhrdw/bankp.o obj/drivers/bankp.o \
         obj/vidhrdw/sbasketb.o obj/drivers/sbasketb.o \
         obj/machine/mcr.o \
         obj/vidhrdw/mcr1.o obj/vidhrdw/mcr2.o obj/vidhrdw/mcr3.o \
         obj/drivers/mcr1.o obj/drivers/mcr2.o obj/drivers/mcr3.o \
         obj/machine/espial.o obj/vidhrdw/espial.o obj/drivers/espial.o \
         obj/machine/tp84.o obj/vidhrdw/tp84.o obj/drivers/tp84.o \
         obj/vidhrdw/mikie.o obj/drivers/mikie.o \
         obj/vidhrdw/ironhors.o obj/drivers/ironhors.o \
         obj/vidhrdw/shaolins.o obj/drivers/shaolins.o \
         obj/machine/rastan.o obj/vidhrdw/rastan.o obj/sndhrdw/rastan.o obj/drivers/rastan.o \
         obj/machine/cloak.o obj/vidhrdw/cloak.o obj/drivers/cloak.o \
         obj/machine/lwings.o obj/vidhrdw/lwings.o obj/drivers/lwings.o \
         obj/machine/berzerk.o obj/vidhrdw/berzerk.o obj/sndhrdw/berzerk.o obj/drivers/berzerk.o \
         obj/machine/capbowl.o obj/vidhrdw/capbowl.o obj/drivers/capbowl.o \
         obj/vidhrdw/1943.o obj/drivers/1943.o \
         obj/vidhrdw/gunsmoke.o obj/drivers/gunsmoke.o \
         obj/vidhrdw/blktiger.o obj/drivers/blktiger.o \
         obj/vidhrdw/tecmo.o obj/drivers/tecmo.o \
         obj/vidhrdw/gaiden.o obj/drivers/gaiden.o \
         obj/vidhrdw/sidearms.o obj/drivers/sidearms.o \
         obj/vidhrdw/srumbler.o obj/drivers/srumbler.o \
         obj/vidhrdw/champbas.o obj/drivers/champbas.o \
         obj/vidhrdw/pbaction.o obj/drivers/pbaction.o \
         obj/vidhrdw/exerion.o obj/drivers/exerion.o \
         obj/machine/arkanoid.o obj/vidhrdw/arkanoid.o obj/drivers/arkanoid.o \
         obj/machine/slapstic.o \
         obj/machine/gauntlet.o obj/vidhrdw/gauntlet.o obj/drivers/gauntlet.o \
         obj/machine/atarisy1.o obj/vidhrdw/atarisy1.o obj/drivers/atarisy1.o \
         obj/machine/foodf.o obj/vidhrdw/foodf.o obj/drivers/foodf.o \
         obj/vidhrdw/circus.o obj/drivers/circus.o \
         obj/machine/konami.o obj/vidhrdw/trackfld.o obj/sndhrdw/trackfld.o obj/drivers/trackfld.o \
         obj/vidhrdw/hyperspt.o obj/drivers/hyperspt.o \
         obj/vidhrdw/rocnrope.o obj/drivers/rocnrope.o \
         obj/vidhrdw/circusc.o obj/drivers/circusc.o \
         obj/vidhrdw/pingpong.o obj/drivers/pingpong.o \
         obj/vidhrdw/astrof.o obj/drivers/astrof.o \
         obj/machine/sprint2.o obj/vidhrdw/sprint2.o obj/drivers/sprint2.o \
         obj/vidhrdw/punchout.o obj/sndhrdw/punchout.o obj/drivers/punchout.o \
         obj/vidhrdw/firetrap.o obj/drivers/firetrap.o \
         obj/vidhrdw/jack.o obj/drivers/jack.o \
         obj/machine/vastar.o obj/vidhrdw/vastar.o obj/drivers/vastar.o \
         obj/vidhrdw/brkthru.o obj/drivers/brkthru.o \
         obj/vidhrdw/citycon.o obj/drivers/citycon.o \
         obj/machine/starfire.o obj/vidhrdw/starfire.o obj/drivers/starfire.o \
         obj/machine/sbrkout.o obj/vidhrdw/sbrkout.o obj/drivers/sbrkout.o \
         obj/vidhrdw/superqix.o obj/drivers/superqix.o \
         obj/machine/jedi.o obj/vidhrdw/jedi.o obj/sndhrdw/jedi.o obj/drivers/jedi.o \
         obj/vidhrdw/gameplan.o obj/drivers/gameplan.o \
         obj/machine/dominos.o obj/vidhrdw/dominos.o obj/drivers/dominos.o \
         obj/vidhrdw/jumpcoas.o obj/drivers/jumpcoas.o \
         obj/vidhrdw/tankbatt.o obj/drivers/tankbatt.o \
         obj/machine/rainbow.o obj/drivers/rainbow.o \
         obj/vidhrdw/nitedrvr.o obj/machine/nitedrvr.o obj/drivers/nitedrvr.o \
         obj/vidhrdw/lrunner.o obj/drivers/lrunner.o \
         obj/vidhrdw/liberatr.o obj/machine/liberatr.o obj/drivers/liberatr.o \
         obj/vidhrdw/wiz.o obj/drivers/wiz.o \
         obj/vidhrdw/blockout.o obj/drivers/blockout.o \
         obj/vidhrdw/fastfred.o obj/drivers/fastfred.o \
         obj/vidhrdw/thepit.o obj/drivers/thepit.o \
         obj/vidhrdw/bsktball.o obj/machine/bsktball.o obj/drivers/bsktball.o \
         obj/vidhrdw/copsnrob.o obj/machine/copsnrob.o obj/drivers/copsnrob.o \
         obj/vidhrdw/toki.o obj/sndhrdw/toki.o obj/drivers/toki.o \
         obj/vidhrdw/snowbros.o obj/drivers/snowbros.o \
         obj/machine/cps1.o obj/vidhrdw/cps1.o obj/drivers/cps1.o \
         obj/vidhrdw/gundealr.o obj/drivers/gundealr.o \
         obj/machine/tnzs.o obj/vidhrdw/tnzs.o obj/drivers/tnzs.o \
         obj/vidhrdw/route16.o obj/drivers/route16.o \
         obj/vidhrdw/wc90.o obj/drivers/wc90.o \
         obj/vidhrdw/wc90b.o obj/drivers/wc90b.o \
         obj/drivers/twincobr.o \
         obj/machine/dec0.o obj/vidhrdw/dec0.o obj/drivers/dec0.o \
         obj/vidhrdw/karnov.o obj/drivers/karnov.o \
         obj/machine/toobin.o obj/vidhrdw/toobin.o obj/drivers/toobin.o \
         obj/vidhrdw/tigeroad.o obj/drivers/tigeroad.o \
         obj/vidhrdw/blockade.o obj/drivers/blockade.o \
         obj/machine/leprechn.o obj/vidhrdw/leprechn.o obj/drivers/leprechn.o \
         obj/vidhrdw/atetris.o obj/drivers/atetris.o \
         obj/vidhrdw/dday.o obj/drivers/dday.o \
         obj/machine/system8.o obj/drivers/system8.o \
         obj/Z80/Z80.o obj/M6502/M6502.o obj/I86/I86.o obj/I8039/I8039.o \
		 obj/M6809/m6809.o obj/M6808/m6808.o obj/M6805/m6805.o \
         obj/M68000/opcode0.o obj/M68000/opcode1.o obj/M68000/opcode2.o obj/M68000/opcode3.o obj/M68000/opcode4.o obj/M68000/opcode5.o \
         obj/M68000/opcode6.o obj/M68000/opcode7.o obj/M68000/opcode8.o obj/M68000/opcode9.o obj/M68000/opcodeb.o \
         obj/M68000/opcodec.o obj/M68000/opcoded.o obj/M68000/opcodee.o obj/M68000/mc68kmem.o \
         obj/M68000/cpufunc.o \
         obj/mamedbg.o obj/asg.o obj/M6502/6502dasm.o \
         obj/M6809/6809dasm.o obj/M6808/6808dasm.o obj/M6805/6805dasm.o \
         obj/M68000/m68kdasm.o \
         obj/msdos/msdos.o obj/msdos/video.o obj/msdos/vector.o obj/msdos/sound.o \
         obj/msdos/input.o obj/msdos/fileio.o obj/msdos/config.o obj/msdos/fronthlp.o

VPATH=src src/Z80 src/M6502 src/I86 src/M6809

all: mame.exe

mame.exe:  $(OBJS)
	$(LD) $(LDFLAGS) -o mame.exe $(OBJS) $(LIBS)

obj/%.o: src/%.c mame.h driver.h
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/Z80/Z80.o:  Z80.c Z80.h Z80Codes.h Z80IO.h Z80DAA.h
obj/M6502/M6502.o:	M6502.c M6502.h Tables.h Codes.h
obj/I86/I86.o:  I86.c I86.h I86intrf.h ea.h host.h instr.h modrm.h
obj/M6809/m6809.o:  m6809.c m6809.h 6809ops.c
obj/M6808/M6808.o:  m6808.c m6808.h


makedir:
	mkdir obj
	mkdir obj\Z80
	mkdir obj\M6502
	mkdir obj\I86
	mkdir obj\I8039
	mkdir obj\M6809
	mkdir obj\M6808
	mkdir obj\M6805
	mkdir obj\M68000
	mkdir obj\drivers
	mkdir obj\machine
	mkdir obj\vidhrdw
	mkdir obj\sndhrdw
	mkdir obj\msdos

clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\I86\*.o
	del obj\I8039\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del obj\msdos\*.o
	del mame.exe

cleandebug:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del mame.exe
