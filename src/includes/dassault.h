extern data16_t *dassault_pf1_data,*dassault_pf2_data;
extern data16_t *dassault_pf3_data,*dassault_pf4_data;
extern data16_t *dassault_pf4_rowscroll,*dassault_pf2_rowscroll;

VIDEO_START( dassault );
VIDEO_UPDATE( dassault );

WRITE16_HANDLER( dassault_pf1_data_w );
WRITE16_HANDLER( dassault_pf2_data_w );
WRITE16_HANDLER( dassault_pf3_data_w );
WRITE16_HANDLER( dassault_pf4_data_w );
WRITE16_HANDLER( dassault_control_0_w );
WRITE16_HANDLER( dassault_control_1_w );
WRITE16_HANDLER( dassault_update_sprites_0_w );
WRITE16_HANDLER( dassault_update_sprites_1_w );
WRITE16_HANDLER( dassault_palette_24bit_w );
WRITE16_HANDLER( dassault_priority_w );
