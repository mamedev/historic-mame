/*************************************************************************

	Driver for Williams/Midway games using the TMS34010 processor

**************************************************************************/

/* these are accurate for MK Rev 5 according to measurements done by Bryan on a real board */
/* due to the way the TMS34010 core operates, however, we need to use 0 for our VBLANK */
/* duration (263ms is the measured value) */
#define MKLA5_VBLANK_DURATION		0
#define MKLA5_FPS					53.204950


/*----------- defined in machine/wmsyunit.c -----------*/

extern data16_t *wms_code_rom;
extern data16_t *wms_scratch_ram;

extern data16_t *wms_cmos_ram;
extern UINT32 	wms_cmos_page;

extern offs_t 	wms_speedup_pc;
extern offs_t 	wms_speedup_offset;
extern offs_t 	wms_speedup_spin[3];
extern data16_t *wms_speedup_base;

WRITE16_HANDLER( wms_yunit_cmos_w );
READ16_HANDLER( wms_yunit_cmos_r );

WRITE16_HANDLER( wms_yunit_cmos_enable_w );
READ16_HANDLER( wms_yunit_protection_r );

READ16_HANDLER( wms_yunit_input_r );

READ16_HANDLER( wms_generic_speedup_1_16bit );
READ16_HANDLER( wms_generic_speedup_1_mixedbits );
READ16_HANDLER( wms_generic_speedup_1_32bit );
READ16_HANDLER( wms_generic_speedup_3 );

DRIVER_INIT( narc );
DRIVER_INIT( narc3 );

DRIVER_INIT( trog );
DRIVER_INIT( trog3 );
DRIVER_INIT( trogp );

DRIVER_INIT( smashtv );
DRIVER_INIT( smashtv4 );

DRIVER_INIT( hiimpact );
DRIVER_INIT( shimpact );
DRIVER_INIT( strkforc );

DRIVER_INIT( mkprot9 );
DRIVER_INIT( mkla1 );
DRIVER_INIT( mkla2 );
DRIVER_INIT( mkla3 );
DRIVER_INIT( mkla4 );

DRIVER_INIT( term2 );

DRIVER_INIT( totcarn );
DRIVER_INIT( totcarnp );

MACHINE_INIT( wms_yunit );

WRITE16_HANDLER( wms_yunit_sound_w );


/*----------- defined in vidhrdw/wmsyunit.c -----------*/

extern struct rectangle wms_visible_area;
extern UINT8 *	wms_gfx_rom;
extern size_t	wms_gfx_rom_size;
extern UINT8	wms_partial_update_offset;

VIDEO_START( wms_yunit_4bit );
VIDEO_START( wms_yunit_6bit );
VIDEO_START( wms_zunit );

READ16_HANDLER( wms_yunit_gfxrom_r );

WRITE16_HANDLER( wms_yunit_vram_w );
READ16_HANDLER( wms_yunit_vram_r );

void wms_yunit_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void wms_yunit_from_shiftreg(UINT32 address, UINT16 *shiftreg);

WRITE16_HANDLER( wms_yunit_control_w );
WRITE16_HANDLER( wms_yunit_paletteram_w );

READ16_HANDLER( wms_yunit_dma_r );
WRITE16_HANDLER( wms_yunit_dma_w );

void wms_yunit_display_addr_changed(UINT32 offs, int rowbytes, int scanline);
void wms_yunit_display_interrupt(int scanline);

VIDEO_EOF( wms_yunit );
VIDEO_EOF( wms_zunit );

VIDEO_UPDATE( wms_yunit );
