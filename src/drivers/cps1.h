#ifndef _CPS1_H_
#define _CPS1_H_

extern data16_t *cps1_gfxram;     /* Video RAM */
extern data16_t *cps1_output;     /* Output ports */
extern size_t cps1_gfxram_size;
extern size_t cps1_output_size;

extern const struct Memory_ReadAddress qsound_readmem[];
extern const struct Memory_WriteAddress qsound_writemem[];

READ16_HANDLER( qsound_sharedram1_r );
WRITE16_HANDLER( qsound_sharedram1_w );

READ16_HANDLER( cps1_eeprom_port_r );
WRITE16_HANDLER( cps1_eeprom_port_w );

READ16_HANDLER( cps1_output_r );
WRITE16_HANDLER( cps1_output_w );

WRITE16_HANDLER( cps2_objram_bank_w );
READ16_HANDLER( cps2_objram1_r );
READ16_HANDLER( cps2_objram2_r );
WRITE16_HANDLER( cps2_objram1_w );
WRITE16_HANDLER( cps2_objram2_w );

int  cps1_vh_start(void);
void cps1_vh_stop(void);
void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cps1_eof_callback(void);

int cps1_qsound_interrupt(void);

extern struct QSound_interface qsound_interface;

#endif
