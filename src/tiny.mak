# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_kengo"
COREDEFS += -DTINY_POINTER="&driver_kengo"

# uses these CPUs
CPUS+=CPU_Z80@
CPUS+=CPU_V30@

# uses these SOUNDs
SOUNDS+=SOUND_YM2151@
SOUNDS+=SOUND_DAC@

OBJS = $(OBJ)/drivers/m72.o $(OBJ)/vidhrdw/m72.o $(OBJ)/sndhrdw/m72.o $(OBJ)/machine/irem_cpu.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
