/*************************************************************************

	Cinematronics vector hardware

*************************************************************************/

/*----------- defined in sndhrdw/cinemat.c -----------*/

typedef void (*cinemat_sound_handler_proc)(UINT8, UINT8);
extern cinemat_sound_handler_proc cinemat_sound_handler;

MACHINE_INIT( cinemat_sound );

READ16_HANDLER( cinemat_output_port_r );
WRITE16_HANDLER( cinemat_output_port_w );

extern struct Samplesinterface starcas_samples_interface;
extern struct Samplesinterface warrior_samples_interface;
extern struct Samplesinterface ripoff_samples_interface;
extern struct Samplesinterface solarq_samples_interface;
extern struct Samplesinterface spacewar_samples_interface;

void starcas_sound_w(UINT8 sound_val, UINT8 bits_changed);
void warrior_sound_w(UINT8 sound_val, UINT8 bits_changed);
void armora_sound_w(UINT8 sound_val, UINT8 bits_changed);
void ripoff_sound_w(UINT8 sound_val, UINT8 bits_changed);
void solarq_sound_w(UINT8 sound_val, UINT8 bits_changed);
void spacewar_sound_w(UINT8 sound_val, UINT8 bits_changed);
void demon_sound_w(UINT8 sound_val, UINT8 bits_changed);

MACHINE_DRIVER_EXTERN( demon_sound );


/*----------- defined in vidhrdw/cinemat.c -----------*/

extern struct artwork_element starcas_overlay[];
extern struct artwork_element tailg_overlay[];
extern struct artwork_element sundance_overlay[];
extern struct artwork_element solarq_overlay[];

void CinemaVectorData(int fromx, int fromy, int tox, int toy, int color);

void cinemat_select_artwork(int monitor_type, int overlay_req, int backdrop_req, struct artwork_element *simple_overlay);

PALETTE_INIT( cinemat );
VIDEO_START( cinemat );
VIDEO_EOF( cinemat );

PALETTE_INIT( spacewar );
VIDEO_UPDATE( spacewar );
