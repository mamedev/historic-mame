# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_holeland,driver_crzrally"
COREDEFS += -DTINY_POINTER="&driver_holeland,&driver_crzrally"

# uses these CPUs
CPUS+=CPU_Z80@

# uses these SOUNDs
SOUNDS+=SOUND_AY8910@

OBJS = $(OBJ)/drivers/holeland.o $(OBJ)/vidhrdw/holeland.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
