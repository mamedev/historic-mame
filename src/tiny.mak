# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DTINY_NAME=driver_mazinger

# uses these CPUs
CPUS+=CPU_M68000@
CPUS+=CPU_Z80@

# uses these SOUNDs
SOUNDS+=SOUND_OKIM6295@
SOUNDS+=SOUND_YM2203@

OBJS = $(OBJ)/drivers/mazinger.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o
