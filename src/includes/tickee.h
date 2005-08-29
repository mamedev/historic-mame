/*************************************************************************

    Raster Elite Tickee Tickats hardware

**************************************************************************/

/*----------- defined in driver/tickee.c -----------*/

extern UINT16 *tickee_control;


/*----------- defined in vidhrdw/tickee.c -----------*/

extern UINT16 *tickee_vram;

VIDEO_START( tickee );

VIDEO_UPDATE( tickee );
