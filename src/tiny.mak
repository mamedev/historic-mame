# a tiny compile is without Neogeo games
COREDEFS += -DTINY_NAME="driver_puckman,driver_galaxian,driver_phoenix"
COREDEFS += -DTINY_POINTER="&driver_puckman,&driver_galaxian,&driver_phoenix"


# uses these CPUs
CPUS+=Z80@
CPUS+=Z180@
CPUS+=8080@
CPUS+=8085A@
CPUS+=M6502@
CPUS+=M65C02@
#CPUS+=M65SC02@
#CPUS+=M65CE02@
#CPUS+=M6509@
CPUS+=M6510@
#CPUS+=M6510T@
#CPUS+=M7501@
#CPUS+=M8502@
CPUS+=N2A03@

# uses these SOUNDs
SOUNDS+=NAMCO@
SOUNDS+=AY8910@
SOUNDS+=SN76496@
SOUNDS+=DAC@
SOUNDS+=NES@
SOUNDS+=SAMPLES@
SOUNDS+=TMS5110@
SOUNDS+=CUSTOM@
SOUNDS+=TMS36XX@

OBJS = \
	$(OBJ)/vidhrdw/pacman.o $(OBJ)/drivers/pacman.o \
	$(OBJ)/machine/mspacman.o $(OBJ)/machine/pacplus.o \
	$(OBJ)/machine/jumpshot.o $(OBJ)/machine/theglobp.o \
	$(OBJ)/machine/acitya.o \
	$(OBJ)/drivers/epos.o $(OBJ)/vidhrdw/epos.o \
	$(OBJ)/vidhrdw/dkong.o $(OBJ)/sndhrdw/dkong.o $(OBJ)/drivers/dkong.o \
	$(OBJ)/machine/strtheat.o $(OBJ)/machine/drakton.o \
	$(OBJ)/machine/scramble.o $(OBJ)/sndhrdw/scramble.o $(OBJ)/drivers/scramble.o \
	$(OBJ)/drivers/frogger.o \
	$(OBJ)/drivers/scobra.o \
	$(OBJ)/drivers/amidar.o \
	$(OBJ)/vidhrdw/galaxian.o $(OBJ)/sndhrdw/galaxian.o $(OBJ)/drivers/galaxian.o \
	$(OBJ)/vidhrdw/cclimber.o $(OBJ)/sndhrdw/cclimber.o $(OBJ)/drivers/cclimber.o \
	$(OBJ)/drivers/cvs.o $(OBJ)/vidhrdw/cvs.o $(OBJ)/vidhrdw/s2636.o \
	$(OBJ)/vidhrdw/mario.o $(OBJ)/sndhrdw/mario.o $(OBJ)/drivers/mario.o \
	$(OBJ)/machine/bagman.o $(OBJ)/vidhrdw/bagman.o $(OBJ)/drivers/bagman.o \
	$(OBJ)/vidhrdw/phoenix.o $(OBJ)/sndhrdw/phoenix.o $(OBJ)/drivers/phoenix.o \
	$(OBJ)/sndhrdw/pleiads.o \
	$(OBJ)/vidhrdw/ladybug.o $(OBJ)/drivers/ladybug.o \
	$(OBJ)/machine/8255ppi.o $(OBJ)/machine/7474.o \
	$(OBJ)/vidhrdw/res_net.o \

# MAME specific core objs
COREOBJS += $(OBJ)/tiny.o $(OBJ)/cheat.o
