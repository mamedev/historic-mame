/*************************************************************************

	Sega vector hardware

*************************************************************************/

/*----------- defined in machine/sega.c -----------*/

extern UINT8 *sega_mem;
extern void sega_security(int chip);

WRITE8_HANDLER( sega_w );

INTERRUPT_GEN( sega_interrupt );

READ8_HANDLER( sega_mult_r );
WRITE8_HANDLER( sega_mult1_w );
WRITE8_HANDLER( sega_mult2_w );
WRITE8_HANDLER( sega_switch_w );
WRITE8_HANDLER( sega_coin_counter_w );

READ8_HANDLER( sega_ports_r );
READ8_HANDLER( sega_IN4_r );
READ8_HANDLER( elim4_IN4_r );


/*----------- defined in sndhrdw/sega.c -----------*/

WRITE8_HANDLER( elim1_sh_w );
WRITE8_HANDLER( elim2_sh_w );
WRITE8_HANDLER( spacfury1_sh_w );
WRITE8_HANDLER( spacfury2_sh_w );
WRITE8_HANDLER( zektor1_sh_w );
WRITE8_HANDLER( zektor2_sh_w );

int tacscan_sh_start(const struct MachineSound *msound);
void tacscan_sh_update(void);

WRITE8_HANDLER( tacscan_sh_w );
WRITE8_HANDLER( startrek_sh_w );


/*----------- defined in vidhrdw/sega.c -----------*/

VIDEO_START( sega );
VIDEO_UPDATE( sega );
