/* namconb1.h */

#define NAMCONB1_COLS		36
#define NAMCONB1_ROWS		28
#define NAMCONB1_FG1BASE	(0x4008)
#define NAMCONB1_FG2BASE	(0x4408)

extern enum namconb1_type
{
	key_nebulray,
	key_gunbulet,
	key_gslgr94u,
	key_sws96,
	key_sws97
} namconb1_type;

extern data32_t *namconb1_workram32;
extern data32_t *namconb1_spritelist32;
extern data32_t *namconb1_spriteformat32;
extern data32_t *namconb1_spritetile32;
extern data32_t *namconb1_spritebank32;
extern data32_t *namconb1_scrollram32;
extern data32_t *namconb1_spritepos32;

extern data8_t *namconb1_maskrom;

WRITE32_HANDLER( namconb1_videoram_w );
void namconb1_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh );
int namconb1_vh_start( void );
void namconb1_vh_stop( void );
