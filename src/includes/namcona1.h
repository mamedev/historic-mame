/* namcona1.h */

#define NA1_NVRAM_SIZE (0x800)

extern data16_t *namcona1_workram;
extern data16_t *namcona1_vreg;
extern data16_t *namcona1_scroll;
extern data16_t *namcona1_sparevram;

extern WRITE16_HANDLER( namcona1_videoram_w );
extern READ16_HANDLER( namcona1_videoram_r );

extern READ16_HANDLER( namcona1_gfxram_r );
extern WRITE16_HANDLER( namcona1_gfxram_w );

extern READ16_HANDLER( namcona1_paletteram_r );
extern WRITE16_HANDLER( namcona1_paletteram_w );

extern VIDEO_UPDATE( namcona1 );
extern VIDEO_START( namcona1 );
