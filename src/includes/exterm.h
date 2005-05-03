/*************************************************************************

    Gottlieb Exterminator hardware

*************************************************************************/

/*----------- defined in vidhrdw/exterm.c -----------*/

extern data16_t *exterm_master_videoram;
extern data16_t *exterm_slave_videoram;

PALETTE_INIT( exterm );
VIDEO_UPDATE( exterm );

void exterm_to_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_to_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
