# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_karatour,driver_ladykill,driver_pangpoms,driver_skyalert,driver_poitto,driver_dharma,driver_lastfort,driver_toride2g,driver_daitorid,driver_puzzli,driver_pururun,driver_balcube,driver_mouja,driver_blzntrnd"
COREDEFS += -DTINY_POINTER="&driver_karatour,&driver_ladykill,&driver_pangpoms,&driver_skyalert,&driver_poitto,&driver_dharma,&driver_lastfort,&driver_toride2g,&driver_daitorid,&driver_puzzli,&driver_pururun,&driver_balcube,&driver_mouja,&driver_blzntrnd"

# uses these CPUs
CPUS+=CPU_M68000@
CPUS+=CPU_UPD7810@

# uses these SOUNDs
SOUNDS+=OKIM6295@
SOUNDS+=YM2151@
SOUNDS+=YM2413@

OBJS = $(OBJ)/drivers/metro.o $(OBJ)/vidhrdw/metro.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
