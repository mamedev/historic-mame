/*************************************************************************

	3dfx Voodoo Graphics SST-1 emulator

	driver by Aaron Giles

**************************************************************************/

VIDEO_START( voodoo_1x4mb );
VIDEO_START( voodoo_2x4mb );
VIDEO_START( voodoo2_1x4mb );
VIDEO_START( voodoo2_2x4mb );
VIDEO_START( voodoo3_1x4mb );
VIDEO_START( voodoo3_2x4mb );
VIDEO_STOP( voodoo );
VIDEO_UPDATE( voodoo );

void voodoo_set_init_enable(data32_t newval);
void voodoo_set_vblank_callback(void (*vblank)(int));
UINT32 voodoo_fifo_words_left(void);

void voodoo_reset(void);
int voodoo_get_type(void);

WRITE32_HANDLER( voodoo_regs_w );
READ32_HANDLER( voodoo_regs_r );
WRITE32_HANDLER( voodoo_regs_alt_w );
READ32_HANDLER( voodoo_regs_alt_r );
WRITE32_HANDLER( voodoo_framebuf_w );
READ32_HANDLER( voodoo_framebuf_r );
WRITE32_HANDLER( voodoo_textureram_w );

WRITE32_HANDLER( voodoo2_regs_w );

READ32_HANDLER( voodoo3_rom_r );
WRITE32_HANDLER( voodoo3_ioreg_w );
READ32_HANDLER( voodoo3_ioreg_r );
WRITE32_HANDLER( voodoo3_cmdagp_w );
READ32_HANDLER( voodoo3_cmdagp_r );
WRITE32_HANDLER( voodoo3_2d_w );
READ32_HANDLER( voodoo3_2d_r );
WRITE32_HANDLER( voodoo3_yuv_w );
READ32_HANDLER( voodoo3_yuv_r );
