# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_mosaic"
COREDEFS += -DTINY_POINTER="&driver_mosaic"

# uses these CPUs
CPUS+=CPU_Z80@
CPUS+=CPU_Z180@

# uses these SOUNDs
SOUNDS+=SOUND_YM2203@

OBJS = $(OBJ)/drivers/mosaic.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
