/***************************************************************************

	Taito Qix hardware

	driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "driver.h"


/*----------- defined in machine/qix.c -----------*/

extern UINT8 *qix_sharedram;
extern UINT8 *qix_68705_port_out;
extern UINT8 *qix_68705_ddr;

void qix_init_machine(void);
void qixmcu_init_machine(void);
void slither_init_machine(void);

READ_HANDLER( qix_sharedram_r );
WRITE_HANDLER( qix_sharedram_w );

WRITE_HANDLER( zoo_bankswitch_w );

READ_HANDLER( qix_data_firq_r );
READ_HANDLER( qix_data_firq_ack_r );
WRITE_HANDLER( qix_data_firq_w );
WRITE_HANDLER( qix_data_firq_ack_w );

READ_HANDLER( qix_video_firq_r );
READ_HANDLER( qix_video_firq_ack_r );
WRITE_HANDLER( qix_video_firq_w );
WRITE_HANDLER( qix_video_firq_ack_w );

READ_HANDLER( qix_68705_portA_r );
READ_HANDLER( qix_68705_portB_r );
READ_HANDLER( qix_68705_portC_r );
WRITE_HANDLER( qix_68705_portA_w );
WRITE_HANDLER( qix_68705_portB_w );
WRITE_HANDLER( qix_68705_portC_w );

WRITE_HANDLER( qix_pia_0_w );
WRITE_HANDLER( zookeep_pia_0_w );
WRITE_HANDLER( zookeep_pia_2_w );


/*----------- defined in vidhrdw/qix.c -----------*/

extern UINT8 *qix_palettebank;
extern UINT8 *qix_videoaddress;
extern UINT8 qix_cocktail_flip;

int qix_vh_start(void);
void qix_vh_stop(void);
void qix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int qix_vblank_start(void);
void qix_scanline_callback(int scanline);

READ_HANDLER( qix_scanline_r );
READ_HANDLER( qix_videoram_r );
WRITE_HANDLER( qix_videoram_w );
READ_HANDLER( qix_addresslatch_r );
WRITE_HANDLER( qix_addresslatch_w );
WRITE_HANDLER( slither_vram_mask_w );
WRITE_HANDLER( qix_paletteram_w );
WRITE_HANDLER( qix_palettebank_w );

READ_HANDLER( qix_data_io_r );
READ_HANDLER( qix_sound_io_r );
WRITE_HANDLER( qix_data_io_w );
WRITE_HANDLER( qix_sound_io_w );
