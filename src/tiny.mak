# a tiny compile is without Neogeo games
COREDEFS += -DTINY_NAME="driver_robby,driver_gridlee,driver_polyplay"
COREDEFS += -DTINY_POINTER="&driver_robby,&driver_gridlee,&driver_polyplay"

# uses these CPUs
CPUS+=Z80
CPUS+=M6809

# uses these SOUNDs
SOUNDS+=CUSTOM
SOUNDS+=SN76496
SOUNDS+=SAMPLES
SOUNDS+=ASTROCADE

DRVLIBS = \
	$(OBJ)/machine/astrocde.o $(OBJ)/vidhrdw/astrocde.o \
	$(OBJ)/sndhrdw/wow.o $(OBJ)/sndhrdw/gorf.o $(OBJ)/drivers/astrocde.o \
	$(OBJ)/vidhrdw/gridlee.o $(OBJ)/sndhrdw/gridlee.o $(OBJ)/drivers/gridlee.o \
	$(OBJ)/vidhrdw/polyplay.o $(OBJ)/sndhrdw/polyplay.o $(OBJ)/drivers/polyplay.o \

# MAME specific core objs
COREOBJS += $(OBJ)/tiny.o $(OBJ)/cheat.o
