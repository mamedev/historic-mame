# a tiny compile is without Neogeo games
COREDEFS += -DNEOFREE=1 -DTINY_COMPILE=1 -DTINY_NAME=driver_flstory

# uses these CPUs
CPUS += Z80@
CPUS += M68705@

# uses these SOUNDs
SOUNDS += AY8910@

OBJS =	$(OBJ)/drivers/flstory.o \
	$(OBJ)/vidhrdw/flstory.o \
	$(OBJ)/machine/flstory.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o

