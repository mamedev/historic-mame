# before compiling for the first time, use "make makedir" to create all the
# necessary subdirectories

AR = ar
CC = gcc
LD = gcc
#ASM = nasm
ASM = nasmw
ASMFLAGS = -f coff
VPATH=src src/z80 src/m6502 src/i86 src/m6809 src/m6808 src/tms34010 src/tms9900

# uncomment next line to use Assembler 6808 engine
# X86_ASM_6808 = 1
# uncomment next line to use Assembler 68k engine
X86_ASM_68K = 1

ifdef X86_ASM_6808
M6808OBJS = obj/m6808/m6808.oa obj/m6808/6808dasm.o
else
M6808OBJS = obj/m6808/m6808.o obj/m6808/6808dasm.o
endif

ifdef X86_ASM_68K
M68KOBJS = obj/m68000/asmintf.o obj/m68000/68kem.oa
M68KDEF  = -DA68KEM
else
M68KOBJS = obj/m68000/opcode0.o obj/m68000/opcode1.o obj/m68000/opcode2.o obj/m68000/opcode3.o obj/m68000/opcode4.o obj/m68000/opcode5.o \
          obj/m68000/opcode6.o obj/m68000/opcode7.o obj/m68000/opcode8.o obj/m68000/opcode9.o obj/m68000/opcodeb.o \
          obj/m68000/opcodec.o obj/m68000/opcoded.o obj/m68000/opcodee.o obj/m68000/mc68kmem.o \
          obj/m68000/cpufunc.o
M68KDEF  =
endif

# add -DMAME_DEBUG to include the debugger
DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -DINLINE="static __inline__" -Dasm=__asm__
#DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -DINLINE="static __inline__" -Dasm=__asm__ \
	-DMAME_DEBUG

CFLAGS = -Isrc -Isrc/msdos -fomit-frame-pointer -O3 -mpentium -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-pedantic \
	-Wshadow \
	-Wstrict-prototypes
#	-Wredundant-decls \
#	-Wlarger-than-27648 \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
#CFLAGS = -Isrc -Isrc/msdos -O -mpentium -Wall -Werror -g
LDFLAGS = -s
#LDFLAGS =
LIBS   = -lalleg $(DJDIR)/lib/libaudio.a \
	 obj/pacman.a obj/galaxian.a obj/scramble.a obj/cclimber.a \
	 obj/phoenix.a obj/namco.a obj/univers.a obj/nintendo.a \
	 obj/midw8080.a obj/midwz80.a obj/meadows.a obj/astrocde.a \
	 obj/mcr.a obj/irem.a obj/gottlieb.a obj/oldtaito.a \
	 obj/qixtaito.a obj/taito.a obj/taito2.a obj/williams.a \
	 obj/capcom.a obj/capbowl.a obj/mitchell.a obj/gremlin.a obj/vicdual.a \
	 obj/segav.a obj/segar.a obj/zaxxon.a obj/system8.a \
	 obj/system16.a obj/btime.a obj/dataeast.a obj/dec8.a \
	 obj/dec0.a obj/tehkan.a obj/konami.a obj/nemesis.a \
	 obj/tmnt.a obj/exidy.a obj/atarivg.a obj/centiped.a \
	 obj/kangaroo.a obj/missile.a obj/ataribw.a obj/atarisy1.a \
	 obj/atarisy2.a obj/atari.a obj/rockola.a obj/technos.a \
	 obj/berzerk.a obj/gameplan.a obj/stratvox.a obj/zaccaria.a \
	 obj/upl.a obj/tms.a obj/cinemar.a obj/thepit.a obj/valadon.a \
	 obj/seibu.a obj/nichibut.a obj/neogeo.a obj/other.a

OBJS   = obj/mame.o obj/common.o obj/usrintrf.o obj/driver.o \
         obj/cpuintrf.o obj/memory.o obj/timer.o obj/palette.o \
         obj/inptport.o obj/cheat.o obj/unzip.o obj/inflate.o \
         obj/audit.o obj/info.o obj/crc32.o obj/png.o obj/artwork.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/ay8910.o obj/sndhrdw/psgintf.o \
         obj/sndhrdw/2151intf.o obj/sndhrdw/fm.o \
         obj/sndhrdw/ym2151.o obj/sndhrdw/ym2413.o \
         obj/sndhrdw/2610intf.o \
         obj/sndhrdw/ym3812.o obj/sndhrdw/3812intf.o \
		 obj/sndhrdw/tms5220.o obj/sndhrdw/5220intf.o obj/sndhrdw/vlm5030.o \
		 obj/sndhrdw/pokey.o obj/sndhrdw/sn76496.o \
		 obj/sndhrdw/nes.o obj/sndhrdw/nesintf.o obj/sndhrdw/astrocde.o \
		 obj/sndhrdw/votrax.o obj/sndhrdw/dac.o obj/sndhrdw/samples.o \
		 obj/sndhrdw/k007232.o \
		 obj/sndhrdw/streams.o \
         obj/machine/z80fmly.o obj/machine/6821pia.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/vidhrdw/vector.o obj/vidhrdw/avgdvg.o obj/machine/mathbox.o \
         obj/sndhrdw/namco.o obj/sndhrdw/namcos1.o \
         obj/machine/segacrpt.o \
         obj/machine/atarigen.o \
         obj/machine/slapstic.o \
         obj/machine/ticket.o \
         obj/z80/z80.o obj/m6502/m6502.o obj/i86/i86.o obj/i8039/i8039.o obj/i8085/i8085.o \
         obj/m6809/m6809.o obj/m6805/m6805.o \
         obj/s2650/s2650.o obj/t11/t11.o \
         obj/tms34010/tms34010.o obj/tms34010/34010fld.o \
         $(M6808OBJS) \
         $(M68KOBJS) \
         obj/tms9900/tms9900.o obj/tms9900/9900dasm.o \
         obj/mamedbg.o obj/asg.o \
         obj/z80/z80dasm.o obj/m6502/6502dasm.o obj/i86/i86dasm.o obj/i8085/8085dasm.o \
         obj/m6809/6809dasm.o obj/m6805/6805dasm.o  obj/I8039/8039dasm.o \
         obj/s2650/2650dasm.o obj/t11/t11dasm.o obj/tms34010/34010dsm.o obj/m68000/m68kdasm.o \
         obj/msdos/msdos.o obj/msdos/video.o obj/msdos/vector.o obj/msdos/sound.o \
         obj/msdos/input.o obj/msdos/fileio.o obj/msdos/config.o obj/msdos/fronthlp.o \
		 obj/msdos/profiler.o


all: mame.exe

mame.exe:  $(OBJS) $(LIBS)
	$(LD) $(LDFLAGS) -o mame.exe $(OBJS) $(LIBS)

obj/%.o: src/%.c mame.h driver.h
	 $(CC) $(DEFS) $(M68KDEF) $(CFLAGS) -o $@ -c $<

obj/m6808/m6808.asm:  src/m6808/make6808.c
	 $(CC) -o obj/m6808/make6808.exe src/m6808/make6808.c
	 obj/m6808/make6808 obj/m6808/m6808.asm -s -m -h

obj/m68000/68kem.asm:  src/m68000/make68k.c
	 $(CC) $(DEFS) $(CFLAGS) -DDOS -o obj/m68000/make68k.exe src/m68000/make68k.c
	 obj/m68000/make68k obj/m68000/68kem.asm

obj/%.oa:  obj/%.asm
	 $(ASM) -o $@ $(ASMFLAGS) $(ASMDEFS) $<

obj/%.a:
	 $(AR) cr $@ $^

obj/pacman.a: \
         obj/machine/pacman.o obj/drivers/pacman.o \
         obj/machine/pacplus.o \
         obj/machine/theglob.o \
         obj/drivers/maketrax.o \
         obj/machine/jrpacman.o obj/drivers/jrpacman.o obj/vidhrdw/jrpacman.o \
         obj/vidhrdw/pengo.o obj/drivers/pengo.o

obj/galaxian.a: \
         obj/vidhrdw/galaxian.o obj/drivers/galaxian.o \
         obj/sndhrdw/mooncrst.o obj/drivers/mooncrst.o \

obj/scramble.a: \
         obj/machine/scramble.o obj/sndhrdw/scramble.o obj/drivers/scramble.o \
         obj/vidhrdw/frogger.o obj/sndhrdw/frogger.o obj/drivers/frogger.o \
         obj/drivers/ckongs.o \
         obj/drivers/scobra.o \
         obj/vidhrdw/amidar.o obj/drivers/amidar.o \
         obj/vidhrdw/jumpbug.o obj/drivers/jumpbug.o \
         obj/vidhrdw/fastfred.o obj/drivers/fastfred.o

obj/cclimber.a: \
         obj/vidhrdw/cclimber.o obj/sndhrdw/cclimber.o obj/drivers/cclimber.o \
         obj/vidhrdw/seicross.o obj/drivers/seicross.o

obj/phoenix.a: \
         obj/vidhrdw/phoenix.o obj/sndhrdw/phoenix.o obj/drivers/phoenix.o \
         obj/sndhrdw/pleiads.o \
         obj/vidhrdw/naughtyb.o obj/drivers/naughtyb.o

obj/namco.a: \
         obj/vidhrdw/rallyx.o obj/drivers/rallyx.o \
         obj/drivers/locomotn.o \
         obj/machine/bosco.o obj/sndhrdw/bosco.o obj/vidhrdw/bosco.o obj/drivers/bosco.o \
         obj/machine/galaga.o obj/vidhrdw/galaga.o obj/drivers/galaga.o \
         obj/machine/digdug.o obj/vidhrdw/digdug.o obj/drivers/digdug.o \
         obj/vidhrdw/xevious.o obj/machine/xevious.o obj/drivers/xevious.o \
         obj/machine/superpac.o obj/vidhrdw/superpac.o obj/drivers/superpac.o \
         obj/machine/mappy.o obj/vidhrdw/mappy.o obj/drivers/mappy.o \
         obj/vidhrdw/pacland.o obj/drivers/pacland.o \
         obj/vidhrdw/rthunder.o obj/drivers/rthunder.o \

obj/univers.a: \
         obj/vidhrdw/cosmica.o obj/drivers/cosmica.o \
         obj/vidhrdw/cheekyms.o obj/drivers/cheekyms.o \
         obj/machine/panic.o obj/vidhrdw/panic.o obj/drivers/panic.o \
         obj/vidhrdw/ladybug.o obj/drivers/ladybug.o \
         obj/vidhrdw/mrdo.o obj/drivers/mrdo.o \
         obj/machine/docastle.o obj/vidhrdw/docastle.o obj/drivers/docastle.o \
         obj/drivers/dowild.o

obj/nintendo.a: \
         obj/vidhrdw/dkong.o obj/sndhrdw/dkong.o obj/drivers/dkong.o \
         obj/vidhrdw/mario.o obj/sndhrdw/mario.o obj/drivers/mario.o \
         obj/vidhrdw/popeye.o obj/drivers/popeye.o \
         obj/vidhrdw/punchout.o obj/sndhrdw/punchout.o obj/drivers/punchout.o

obj/midw8080.a: \
         obj/machine/8080bw.o obj/vidhrdw/8080bw.o obj/sndhrdw/8080bw.o obj/drivers/8080bw.o \
         obj/vidhrdw/m79amb.o obj/drivers/m79amb.o \

obj/midwz80.a: \
         obj/vidhrdw/z80bw.o obj/sndhrdw/z80bw.o obj/drivers/z80bw.o

obj/meadows.a: \
         obj/drivers/lazercmd.o obj/vidhrdw/lazercmd.o \
         obj/drivers/meadows.o obj/sndhrdw/meadows.o obj/vidhrdw/meadows.o \
         obj/drivers/medlanes.o obj/vidhrdw/medlanes.o \

obj/astrocde.a: \
         obj/machine/wow.o obj/vidhrdw/wow.o obj/sndhrdw/wow.o obj/drivers/wow.o \
         obj/sndhrdw/gorf.o \

obj/mcr.a: \
         obj/machine/mcr.o \
         obj/vidhrdw/mcr1.o obj/vidhrdw/mcr2.o obj/vidhrdw/mcr3.o \
         obj/drivers/mcr1.o obj/drivers/mcr2.o obj/drivers/mcr3.o \
         obj/machine/mcr68.o obj/vidhrdw/mcr68.o obj/drivers/mcr68.o

obj/irem.a: \
         obj/sndhrdw/irem.o \
         obj/vidhrdw/mpatrol.o obj/drivers/mpatrol.o \
         obj/vidhrdw/yard.o obj/drivers/yard.o \
         obj/vidhrdw/kungfum.o obj/drivers/kungfum.o \
         obj/vidhrdw/travrusa.o obj/drivers/travrusa.o \
         obj/vidhrdw/ldrun.o obj/drivers/ldrun.o

obj/gottlieb.a: \
         obj/vidhrdw/gottlieb.o obj/sndhrdw/gottlieb.o obj/drivers/gottlieb.o

obj/oldtaito.a: \
         obj/vidhrdw/crbaloon.o obj/drivers/crbaloon.o

obj/qixtaito.a: \
         obj/machine/qix.o obj/vidhrdw/qix.o obj/drivers/qix.o

obj/taito.a: \
         obj/machine/taito.o obj/vidhrdw/taito.o obj/drivers/taito.o

obj/taito2.a: \
         obj/vidhrdw/bking2.o obj/drivers/bking2.o \
         obj/vidhrdw/gsword.o obj/drivers/gsword.o \
         obj/vidhrdw/gladiatr.o obj/drivers/gladiatr.o \
         obj/vidhrdw/tokio.o obj/drivers/tokio.o \
         obj/machine/bublbobl.o obj/vidhrdw/bublbobl.o obj/drivers/bublbobl.o \
         obj/vidhrdw/rastan.o obj/sndhrdw/rastan.o obj/drivers/rastan.o \
         obj/machine/rainbow.o obj/drivers/rainbow.o \
         obj/machine/arkanoid.o obj/vidhrdw/arkanoid.o obj/drivers/arkanoid.o \
         obj/vidhrdw/superqix.o obj/drivers/superqix.o \
         obj/vidhrdw/twincobr.o obj/drivers/twincobr.o \
         obj/machine/tnzs.o obj/vidhrdw/tnzs.o obj/drivers/tnzs.o \
         obj/drivers/arkanoi2.o \
         obj/machine/slapfght.o obj/vidhrdw/slapfght.o obj/drivers/slapfght.o \
         obj/vidhrdw/superman.o obj/drivers/superman.o obj/machine/cchip.o \
         obj/vidhrdw/taitof2.o obj/drivers/taitof2.o \
         obj/vidhrdw/ssi.o obj/drivers/ssi.o \

obj/williams.a: \
         obj/machine/williams.o obj/vidhrdw/williams.o obj/drivers/williams.o

obj/capcom.a: \
         obj/vidhrdw/vulgus.o obj/drivers/vulgus.o \
         obj/vidhrdw/sonson.o obj/drivers/sonson.o \
         obj/vidhrdw/higemaru.o obj/drivers/higemaru.o \
         obj/vidhrdw/1942.o obj/drivers/1942.o \
         obj/vidhrdw/exedexes.o obj/drivers/exedexes.o \
         obj/vidhrdw/commando.o obj/drivers/commando.o \
         obj/vidhrdw/gng.o obj/drivers/gng.o \
         obj/vidhrdw/gunsmoke.o obj/drivers/gunsmoke.o \
         obj/vidhrdw/srumbler.o obj/drivers/srumbler.o \
         obj/machine/lwings.o obj/vidhrdw/lwings.o obj/drivers/lwings.o \
         obj/vidhrdw/sidearms.o obj/drivers/sidearms.o \
         obj/vidhrdw/bionicc.o obj/drivers/bionicc.o \
         obj/vidhrdw/1943.o obj/drivers/1943.o \
         obj/vidhrdw/blktiger.o obj/drivers/blktiger.o \
         obj/vidhrdw/tigeroad.o obj/drivers/tigeroad.o \
         obj/vidhrdw/lastduel.o obj/drivers/lastduel.o \
         obj/machine/cps1.o obj/vidhrdw/cps1.o obj/drivers/cps1.o

obj/capbowl.a: \
         obj/machine/capbowl.o obj/vidhrdw/capbowl.o obj/vidhrdw/tms34061.o obj/drivers/capbowl.o

obj/mitchell.a: \
         obj/vidhrdw/pang.o obj/drivers/pang.o

obj/gremlin.a: \
         obj/vidhrdw/blockade.o obj/drivers/blockade.o

obj/vicdual.a: \
         obj/vidhrdw/vicdual.o obj/sndhrdw/vicdual.o obj/drivers/vicdual.o

obj/segav.a: \
         obj/vidhrdw/sega.o obj/sndhrdw/sega.o obj/machine/sega.o obj/drivers/sega.o

obj/segar.a: \
         obj/vidhrdw/segar.o obj/sndhrdw/segar.o obj/machine/segar.o obj/drivers/segar.o \
         obj/sndhrdw/monsterb.o

obj/zaxxon.a: \
         obj/vidhrdw/zaxxon.o obj/sndhrdw/zaxxon.o obj/drivers/zaxxon.o \
         obj/sndhrdw/congo.o obj/drivers/congo.o

obj/system8.a: \
         obj/vidhrdw/system8.o obj/drivers/system8.o

obj/system16.a: \
         obj/vidhrdw/system16.o obj/drivers/system16.o

obj/btime.a: \
         obj/vidhrdw/btime.o obj/drivers/btime.o \
         obj/vidhrdw/tagteam.o obj/drivers/tagteam.o

obj/dataeast.a: \
         obj/vidhrdw/astrof.o obj/sndhrdw/astrof.o obj/drivers/astrof.o \
         obj/vidhrdw/kchamp.o obj/drivers/kchamp.o \
         obj/vidhrdw/firetrap.o obj/drivers/firetrap.o \
         obj/vidhrdw/brkthru.o obj/drivers/brkthru.o \
         obj/vidhrdw/shootout.o obj/drivers/shootout.o \
         obj/vidhrdw/sidepckt.o obj/drivers/sidepckt.o \
         obj/vidhrdw/exprraid.o obj/drivers/exprraid.o \
         obj/vidhrdw/pcktgal.o obj/drivers/pcktgal.o \

obj/dec8.a: \
         obj/vidhrdw/dec8.o obj/drivers/dec8.o \

obj/dec0.a: \
         obj/vidhrdw/karnov.o obj/drivers/karnov.o \
         obj/machine/dec0.o obj/vidhrdw/dec0.o obj/drivers/dec0.o \
         obj/vidhrdw/darkseal.o obj/drivers/darkseal.o \

obj/tehkan.a: \
         obj/vidhrdw/bombjack.o obj/drivers/bombjack.o \
         obj/sndhrdw/starforc.o obj/vidhrdw/starforc.o obj/drivers/starforc.o \
         obj/vidhrdw/pbaction.o obj/drivers/pbaction.o \
         obj/vidhrdw/tehkanwc.o obj/drivers/tehkanwc.o \
         obj/vidhrdw/solomon.o obj/drivers/solomon.o \
         obj/vidhrdw/tecmo.o obj/drivers/tecmo.o \
         obj/vidhrdw/gaiden.o obj/drivers/gaiden.o \
         obj/vidhrdw/wc90.o obj/drivers/wc90.o \
         obj/vidhrdw/wc90b.o obj/drivers/wc90b.o \

obj/konami.a: \
         obj/vidhrdw/pooyan.o obj/drivers/pooyan.o \
         obj/vidhrdw/timeplt.o obj/drivers/timeplt.o \
         obj/vidhrdw/rocnrope.o obj/drivers/rocnrope.o \
         obj/sndhrdw/gyruss.o obj/vidhrdw/gyruss.o obj/drivers/gyruss.o \
         obj/machine/konami.o obj/vidhrdw/trackfld.o obj/sndhrdw/trackfld.o obj/drivers/trackfld.o \
         obj/vidhrdw/circusc.o obj/drivers/circusc.o \
         obj/machine/tp84.o obj/vidhrdw/tp84.o obj/drivers/tp84.o \
         obj/vidhrdw/hyperspt.o obj/drivers/hyperspt.o \
         obj/vidhrdw/sbasketb.o obj/drivers/sbasketb.o \
         obj/vidhrdw/mikie.o obj/drivers/mikie.o \
         obj/vidhrdw/yiear.o obj/drivers/yiear.o \
         obj/vidhrdw/shaolins.o obj/drivers/shaolins.o \
         obj/vidhrdw/pingpong.o obj/drivers/pingpong.o \
         obj/vidhrdw/gberet.o obj/drivers/gberet.o \
         obj/vidhrdw/jailbrek.o obj/drivers/jailbrek.o \
         obj/vidhrdw/ironhors.o obj/drivers/ironhors.o \
         obj/machine/jackal.o obj/vidhrdw/jackal.o obj/drivers/jackal.o \
         obj/vidhrdw/contra.o obj/drivers/contra.o \
         obj/vidhrdw/mainevt.o obj/drivers/mainevt.o

obj/nemesis.a: \
         obj/vidhrdw/nemesis.o obj/drivers/nemesis.o

obj/tmnt.a: \
         obj/vidhrdw/tmnt.o obj/drivers/tmnt.o

obj/exidy.a: \
         obj/machine/exidy.o obj/vidhrdw/exidy.o obj/sndhrdw/exidy.o obj/drivers/exidy.o \
         obj/sndhrdw/targ.o \
         obj/vidhrdw/circus.o obj/drivers/circus.o \
         obj/machine/starfire.o obj/vidhrdw/starfire.o obj/drivers/starfire.o

obj/atarivg.a: \
         obj/machine/atari_vg.o \
         obj/machine/asteroid.o obj/sndhrdw/asteroid.o \
		 obj/vidhrdw/llander.o obj/drivers/asteroid.o \
         obj/drivers/bwidow.o \
         obj/sndhrdw/bzone.o  obj/drivers/bzone.o \
         obj/sndhrdw/redbaron.o \
         obj/drivers/tempest.o \
         obj/machine/starwars.o obj/machine/swmathbx.o obj/drivers/starwars.o obj/sndhrdw/starwars.o \
         obj/machine/mhavoc.o obj/drivers/mhavoc.o \
         obj/machine/quantum.o obj/drivers/quantum.o

obj/centiped.a: \
         obj/machine/centiped.o obj/vidhrdw/centiped.o obj/drivers/centiped.o \
         obj/machine/milliped.o obj/vidhrdw/milliped.o obj/drivers/milliped.o \
         obj/vidhrdw/qwakprot.o obj/drivers/qwakprot.o \
         obj/vidhrdw/warlord.o obj/drivers/warlord.o

obj/kangaroo.a: \
         obj/machine/kangaroo.o obj/vidhrdw/kangaroo.o obj/drivers/kangaroo.o \
         obj/machine/arabian.o obj/vidhrdw/arabian.o obj/drivers/arabian.o

obj/missile.a: \
         obj/machine/missile.o obj/vidhrdw/missile.o obj/drivers/missile.o

obj/ataribw.a: \
         obj/machine/sprint2.o obj/vidhrdw/sprint2.o obj/drivers/sprint2.o \
         obj/machine/sbrkout.o obj/vidhrdw/sbrkout.o obj/drivers/sbrkout.o \
         obj/machine/dominos.o obj/vidhrdw/dominos.o obj/drivers/dominos.o \
         obj/vidhrdw/nitedrvr.o obj/machine/nitedrvr.o obj/drivers/nitedrvr.o \
         obj/vidhrdw/bsktball.o obj/machine/bsktball.o obj/drivers/bsktball.o \
         obj/vidhrdw/copsnrob.o obj/machine/copsnrob.o obj/drivers/copsnrob.o \
         obj/machine/avalnche.o obj/vidhrdw/avalnche.o obj/drivers/avalnche.o \
         obj/machine/subs.o obj/vidhrdw/subs.o obj/drivers/subs.o \
         obj/machine/atarifb.o obj/vidhrdw/atarifb.o obj/drivers/atarifb.o \

obj/atarisy1.a: \
         obj/machine/atarisy1.o obj/vidhrdw/atarisy1.o obj/drivers/atarisy1.o

obj/atarisy2.a: \
         obj/machine/atarisy2.o obj/vidhrdw/atarisy2.o obj/drivers/atarisy2.o

obj/atari.a: \
         obj/machine/gauntlet.o obj/vidhrdw/gauntlet.o obj/drivers/gauntlet.o \
         obj/vidhrdw/atetris.o obj/drivers/atetris.o \
         obj/machine/toobin.o obj/vidhrdw/toobin.o obj/drivers/toobin.o \
         obj/vidhrdw/vindictr.o obj/drivers/vindictr.o \
         obj/vidhrdw/klax.o obj/drivers/klax.o \
         obj/machine/blstroid.o obj/vidhrdw/blstroid.o obj/drivers/blstroid.o \
         obj/vidhrdw/eprom.o obj/drivers/eprom.o \
         obj/vidhrdw/xybots.o obj/drivers/xybots.o

obj/rockola.a: \
         obj/vidhrdw/rockola.o obj/sndhrdw/rockola.o obj/drivers/rockola.o \
         obj/vidhrdw/warpwarp.o obj/drivers/warpwarp.o

obj/technos.a: \
         obj/vidhrdw/mystston.o obj/drivers/mystston.o \
         obj/vidhrdw/matmania.o obj/drivers/matmania.o \
         obj/vidhrdw/renegade.o obj/drivers/renegade.o \
         obj/vidhrdw/xain.o obj/drivers/xain.o \
         obj/vidhrdw/battlane.o obj/drivers/battlane.o \
         obj/vidhrdw/ddragon.o obj/drivers/ddragon.o \
         obj/vidhrdw/blockout.o obj/drivers/blockout.o

obj/berzerk.a: \
         obj/machine/berzerk.o obj/vidhrdw/berzerk.o obj/sndhrdw/berzerk.o obj/drivers/berzerk.o

obj/gameplan.a: \
         obj/vidhrdw/gameplan.o obj/drivers/gameplan.o

obj/stratvox.a: \
         obj/vidhrdw/route16.o obj/drivers/route16.o

obj/zaccaria.a: \
         obj/vidhrdw/zaccaria.o obj/drivers/zaccaria.o

obj/upl.a: \
         obj/vidhrdw/nova2001.o obj/drivers/nova2001.o \
         obj/vidhrdw/pkunwar.o obj/drivers/pkunwar.o \
         obj/vidhrdw/ninjakd2.o obj/drivers/ninjakd2.o

obj/tms.a: \
         obj/machine/exterm.o obj/vidhrdw/exterm.o obj/drivers/exterm.o \
         obj/machine/smashtv.o obj/vidhrdw/smashtv.o obj/sndhrdw/smashtv.o obj/drivers/smashtv.o \

obj/cinemar.a: \
         obj/vidhrdw/jack.o obj/drivers/jack.o

obj/thepit.a: \
         obj/vidhrdw/thepit.o obj/drivers/thepit.o

obj/valadon.a: \
         obj/machine/bagman.o obj/vidhrdw/bagman.o obj/drivers/bagman.o

obj/seibu.a: \
         obj/vidhrdw/wiz.o obj/drivers/wiz.o

obj/nichibut.a: \
         obj/vidhrdw/cop01.o obj/drivers/cop01.o \
         obj/vidhrdw/terracre.o obj/drivers/terracre.o \
         obj/vidhrdw/galivan.o obj/drivers/galivan.o \

obj/neogeo.a: \
         obj/machine/neogeo.o obj/machine/pd4990a.o obj/vidhrdw/neogeo.o obj/drivers/neogeo.o

obj/other.a: \
         obj/machine/spacefb.o obj/vidhrdw/spacefb.o obj/sndhrdw/spacefb.o obj/drivers/spacefb.o \
         obj/vidhrdw/tutankhm.o obj/drivers/tutankhm.o \
         obj/drivers/junofrst.o \
         obj/vidhrdw/ccastles.o obj/drivers/ccastles.o \
         obj/vidhrdw/blueprnt.o obj/drivers/blueprnt.o \
         obj/drivers/omegrace.o \
         obj/vidhrdw/bankp.o obj/drivers/bankp.o \
         obj/machine/espial.o obj/vidhrdw/espial.o obj/drivers/espial.o \
         obj/machine/cloak.o obj/vidhrdw/cloak.o obj/drivers/cloak.o \
         obj/vidhrdw/champbas.o obj/drivers/champbas.o \
         obj/drivers/sinbadm.o \
         obj/vidhrdw/exerion.o obj/drivers/exerion.o \
         obj/machine/foodf.o obj/vidhrdw/foodf.o obj/drivers/foodf.o \
         obj/vidhrdw/jack.o obj/drivers/jack.o \
         obj/machine/vastar.o obj/vidhrdw/vastar.o obj/drivers/vastar.o \
         obj/vidhrdw/aeroboto.o obj/drivers/aeroboto.o \
         obj/vidhrdw/citycon.o obj/drivers/citycon.o \
         obj/vidhrdw/psychic5.o obj/drivers/psychic5.o \
         obj/machine/jedi.o obj/vidhrdw/jedi.o obj/sndhrdw/jedi.o obj/drivers/jedi.o \
         obj/vidhrdw/tankbatt.o obj/drivers/tankbatt.o \
         obj/vidhrdw/liberatr.o obj/machine/liberatr.o obj/drivers/liberatr.o \
         obj/vidhrdw/dday.o obj/sndhrdw/dday.o obj/drivers/dday.o \
         obj/vidhrdw/toki.o obj/drivers/toki.o \
         obj/vidhrdw/snowbros.o obj/drivers/snowbros.o \
         obj/vidhrdw/gundealr.o obj/drivers/gundealr.o \
         obj/machine/leprechn.o obj/vidhrdw/leprechn.o obj/drivers/leprechn.o \
         obj/vidhrdw/hexa.o obj/drivers/hexa.o \
         obj/vidhrdw/redalert.o obj/sndhrdw/redalert.o obj/drivers/redalert.o \
         obj/machine/irobot.o obj/vidhrdw/irobot.o obj/drivers/irobot.o \
         obj/machine/spiders.o obj/vidhrdw/crtc6845.o obj/vidhrdw/spiders.o obj/drivers/spiders.o \
         obj/machine/stactics.o obj/vidhrdw/stactics.o obj/drivers/stactics.o \
         obj/vidhrdw/goldstar.o obj/drivers/goldstar.o \
         obj/vidhrdw/vigilant.o obj/drivers/vigilant.o \
         obj/vidhrdw/sharkatt.o obj/drivers/sharkatt.o \
         obj/machine/turbo.o obj/vidhrdw/turbo.o obj/drivers/turbo.o \
         obj/vidhrdw/kingobox.o obj/drivers/kingobox.o \
         obj/vidhrdw/zerozone.o obj/drivers/zerozone.o \
         obj/machine/exctsccr.o obj/vidhrdw/exctsccr.o obj/drivers/exctsccr.o \
         obj/vidhrdw/speedbal.o obj/drivers/speedbal.o \
         obj/vidhrdw/sauro.o obj/drivers/sauro.o \
         obj/vidhrdw/pow.o obj/drivers/pow.o \

# dependencies
obj/z80/z80.o:  z80.c z80.h z80daa.h
obj/m6502/m6502.o: m6502.c m6502.h m6502ops.h tbl6502.c tbl65c02.c tbl6510.c
obj/i86/i86.o:  i86.c i86.h i86intrf.h ea.h host.h instr.h modrm.h
obj/m6809/m6809.o:  m6809.c m6809.h 6809ops.c
obj/m6808/m6808.o:  m6808.c m6808.h
obj/tms34010/tms34010.o: tms34010.c tms34010.h 34010ops.c 34010tbl.c
obj/tms9900/tms9900.o: tms9900.h


makedir:
	md obj
	md obj\z80
	md obj\m6502
	md obj\i86
	md obj\i8039
	md obj\i8085
	md obj\m6809
	md obj\m6808
	md obj\m6805
	md obj\m68000
	md obj\s2650
	md obj\t11
	md obj\tms34010
	md obj\tms9900
	md obj\drivers
	md obj\machine
	md obj\vidhrdw
	md obj\sndhrdw
	md obj\msdos

clean:
	del obj\*.o
	del obj\*.a
	del obj\z80\*.o
	del obj\m6502\*.o
	del obj\i86\*.o
	del obj\i8039\*.o
	del obj\i8085\*.o
	del obj\m6809\*.o
	del obj\m6808\*.o
	del obj\m6808\*.oa
	del obj\m6808\*.exe
	del obj\m6805\*.o
	del obj\m68000\*.o
	del obj\m68000\*.oa
	del obj\m68000\*.asm
	del obj\m68000\*.exe
	del obj\s2650\*.o
	del obj\t11\*.o
	del obj\tms34010\*.o
	del obj\tms9900\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del obj\msdos\*.o
	del mame.exe

cleandebug:
	del obj\*.o
	del obj\z80\*.o
	del obj\m6502\*.o
	del obj\i86\*.o
	del obj\i8039\*.o
	del obj\i8085\*.o
	del obj\m6809\*.o
	del obj\m6808\*.o
	del obj\m6808\*.oa
	del obj\m6808\*.exe
	del obj\m6805\*.o
	del obj\m68000\*.o
	del obj\m68000\*.oa
	del obj\m68000\*.asm
	del obj\m68000\*.exe
	del obj\s2650\*.o
	del obj\t11\*.o
	del obj\tms34010\*.o
	del obj\tms9900\*.o
	del mame.exe
