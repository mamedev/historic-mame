/***************************************************************************
				ToaPlan game hardware from 1988-1991
				------------------------------------
****************************************************************************/


/************* Machine stuff ****** machine/toaplan1.c *************/
int toaplan1_interrupt(void);
WRITE16_HANDLER( toaplan1_int_enable_w );
READ16_HANDLER ( toaplan1_shared_r );
WRITE16_HANDLER( toaplan1_shared_w );
WRITE16_HANDLER( toaplan1_reset_sound );
READ16_HANDLER ( demonwld_dsp_r );
WRITE16_HANDLER( demonwld_dsp_w );
WRITE16_HANDLER( demonwld_dsp_ctrl_w );
READ16_HANDLER ( samesame_port_6_word_r );
READ16_HANDLER ( vimana_mcu_r );
WRITE16_HANDLER( vimana_mcu_w );
READ16_HANDLER ( vimana_input_port_5_word_r );

WRITE_HANDLER( rallybik_coin_w );
WRITE_HANDLER( toaplan1_coin_w );
WRITE16_HANDLER( samesame_coin_w );

void toaplan1_init_machine(void);
void demonwld_init_machine(void);
void vimana_init_machine(void);
void zerozone_init_machine(void);	/* hack for ZeroWing/OutZone. See vidhrdw */

int toaplan1_unk_reset_port;

extern data8_t *toaplan1_sharedram;



/************* Video stuff ****** vidhrdw/toaplan1.c *************/

READ16_HANDLER ( toaplan1_frame_done_r );
WRITE16_HANDLER( toaplan1_bcu_control_w );
WRITE16_HANDLER( rallybik_bcu_flipscreen_w );
WRITE16_HANDLER( toaplan1_bcu_flipscreen_w );
WRITE16_HANDLER( toaplan1_fcu_flipscreen_w );

READ16_HANDLER ( rallybik_tileram16_r );
READ16_HANDLER ( toaplan1_tileram16_r );
WRITE16_HANDLER( toaplan1_tileram16_w );
READ16_HANDLER ( toaplan1_spriteram16_r );
WRITE16_HANDLER( toaplan1_spriteram16_w );
READ16_HANDLER ( toaplan1_spritesizeram16_r );
WRITE16_HANDLER( toaplan1_spritesizeram16_w );
READ16_HANDLER ( toaplan1_colorram1_r );
WRITE16_HANDLER( toaplan1_colorram1_w );
READ16_HANDLER ( toaplan1_colorram2_r );
WRITE16_HANDLER( toaplan1_colorram2_w );

READ16_HANDLER ( toaplan1_scroll_regs_r );
WRITE16_HANDLER( toaplan1_scroll_regs_w );
WRITE16_HANDLER( toaplan1_tile_offsets_w );
READ16_HANDLER ( toaplan1_tileram_offs_r );
WRITE16_HANDLER( toaplan1_tileram_offs_w );
READ16_HANDLER ( toaplan1_spriteram_offs_r );
WRITE16_HANDLER( toaplan1_spriteram_offs_w );

void rallybik_eof_callback(void);
void toaplan1_eof_callback(void);
void samesame_eof_callback(void);
int  rallybik_vh_start(void);
void rallybik_vh_stop(void);
int  toaplan1_vh_start(void);
void toaplan1_vh_stop(void);
void rallybik_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void toaplan1_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void zerowing_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void demonwld_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);


extern data16_t *toaplan1_colorram1;
extern data16_t *toaplan1_colorram2;
extern size_t toaplan1_colorram1_size;
extern size_t toaplan1_colorram2_size;
