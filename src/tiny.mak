# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_taxidrvr"
COREDEFS += -DTINY_POINTER="&driver_taxidrvr"

# uses these CPUs
CPUS+=Z80@

# uses these SOUNDs
SOUNDS+=AY8910@

OBJS = $(OBJ)/drivers/taxidrvr.o $(OBJ)/vidhrdw/taxidrvr.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
