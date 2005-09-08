/*************************************************************************

    Exidy Vertigo hardware

*************************************************************************/

READ16_HANDLER( vertigo_io_convert );
READ16_HANDLER( vertigo_io_adc );
READ16_HANDLER( vertigo_coin_r );
READ16_HANDLER( vertigo_sio_r );
WRITE16_HANDLER( vertigo_audio_w );
WRITE16_HANDLER( vertigo_motor_w );
WRITE16_HANDLER( vertigo_wsot_w );
READ16_HANDLER( vertigo_8254_r );
WRITE16_HANDLER( vertigo_8254_w );

INTERRUPT_GEN( vertigo_interrupt );
MACHINE_INIT( vertigo );

extern UINT16 *vertigo_vectorram;

void vertigo_vproc_init(void);
void vertigo_vproc(int cycles, int irq4);

