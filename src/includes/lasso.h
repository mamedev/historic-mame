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

void lasso_vh_convert_color_prom  (unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom);
void wwjgtin_vh_convert_color_prom(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom);

int lasso_vh_start  ( void );
int wwjgtin_vh_start( void );

void lasso_vh_screenrefresh   ( struct mame_bitmap *bitmap, int fullrefresh );
void chameleo_vh_screenrefresh( struct mame_bitmap *bitmap, int fullrefresh );
void wwjgtin_vh_screenrefresh ( struct mame_bitmap *bitmap, int fullrefresh );
