# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DTINY_NAME=driver_rshark

# uses these CPUs
CPUS+=Z80@
CPUS+=M68000@

# uses these SOUNDs
SOUNDS+=YM2203@
SOUNDS+=YM2151@
SOUNDS+=YM3812@
SOUNDS+=OKIM6295@

OBJS = $(OBJ)/drivers/dooyong.o $(OBJ)/vidhrdw/dooyong.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o
