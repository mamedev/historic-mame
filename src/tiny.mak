# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DTINY_NAME=driver_overdriv

# uses these CPUs
CPUS+=CPU_M68000@
CPUS+=CPU_M6809@

# uses these SOUNDs
SOUNDS+=SOUND_YM2151@
SOUNDS+=K053260@

OBJS = $(OBJ)/drivers/overdriv.o $(OBJ)/vidhrdw/overdriv.o $(OBJ)/vidhrdw/konamiic.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
