# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
#COREDEFS += -DTINY_NAME="driver_coolpool,driver_amerdart,driver_9ballsht,driver_9ballsh2,driver_9ballsh3"
#COREDEFS += -DTINY_POINTER="&driver_coolpool,&driver_amerdart,&driver_9ballsht,&driver_9ballsh2,&driver_9ballsh3"
COREDEFS += -DTINY_NAME="driver_cheesech,driver_ultennis,driver_stonebal,driver_stoneba2"
COREDEFS += -DTINY_POINTER="&driver_cheesech,&driver_ultennis,&driver_stonebal,&driver_stoneba2"

# uses these CPUs
CPUS+=M68000@
CPUS+=M68010@
CPUS+=M68EC020@
CPUS+=M68020@
CPUS+=Z80@
CPUS+=M6502@
CPUS+=M6809@
CPUS+=M6800@
CPUS+=TMS34010@
CPUS+=TMS34020@
CPUS+=TMS32010@
CPUS+=TMS32025@
CPUS+=UPD7810@

# uses these SOUNDs
SOUNDS+=DAC@
SOUNDS+=AY8910@
SOUNDS+=YM2203@
SOUNDS+=YM2151_ALT@
SOUNDS+=YM2608@
SOUNDS+=YM2610@
SOUNDS+=YM2610B@
SOUNDS+=YM2612@
SOUNDS+=YM3438@
SOUNDS+=YM2413@
SOUNDS+=YM3812@
SOUNDS+=YMZ280B@
SOUNDS+=YM3526@
SOUNDS+=Y8950@
SOUNDS+=SN76477@
SOUNDS+=SN76496@
SOUNDS+=ADPCM@
SOUNDS+=OKIM6295@
SOUNDS+=MSM5205@
SOUNDS+=CUSTOM@

OBJS = $(OBJ)/drivers/artmagic.o $(OBJ)/vidhrdw/artmagic.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
