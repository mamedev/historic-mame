# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_spatter"
COREDEFS += -DTINY_POINTER="&driver_spatter"

# uses these CPUs
CPUS+=CPU_Z80@

# uses these SOUNDs
SOUNDS+=SOUND_SN76496@

OBJS = $(OBJ)/drivers/system1.o $(OBJ)/vidhrdw/system1.o $(OBJ)/machine/segacrpt.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
