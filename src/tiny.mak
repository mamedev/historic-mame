# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_gulfstrm"
COREDEFS += -DTINY_POINTER="&driver_gulfstrm"

# uses these CPUs
CPUS+=CPU_Z80@
CPUS+=CPU_M68000@
CPUS+=CPU_M68020@

# uses these SOUNDs
SOUNDS+=SOUND_YM2203@
SOUNDS+=SOUND_YM2151_ALT@
SOUNDS+=SOUND_OKIM6295@

OBJS = $(OBJ)/drivers/dooyong.o $(OBJ)/vidhrdw/dooyong.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
