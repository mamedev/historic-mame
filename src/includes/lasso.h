/***************************************************************************


***************************************************************************/

/* defined in vidhrdw/ */
extern data8_t *lasso_vram;
extern data8_t *wwjgtin_scroll;

WRITE_HANDLER( lasso_videoram_w );
WRITE_HANDLER( lasso_backcolor_w );
WRITE_HANDLER( lasso_gfxbank_w );
WRITE_HANDLER( wwjgtin_gfxbank_w );
WRITE_HANDLER( wwjgtin_lastcolor_w );

PALETTE_INIT( lasso );
PALETTE_INIT( wwjgtin );

VIDEO_START( lasso );
VIDEO_START( wwjgtin );

VIDEO_UPDATE( lasso );
VIDEO_UPDATE( chameleo );
VIDEO_UPDATE( wwjgtin );
