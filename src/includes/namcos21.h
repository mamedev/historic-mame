/* includes/namcos21.h */

enum
{
	NAMCOS21_AIRCOMBAT,
	NAMCOS21_STARBLADE,
	NAMCOS21_CYBERSLED,
	NAMCOS21_SOLVALOU,
	NAMCOS21_WINRUN91
};

extern int namcos21_gametype;
extern data16_t *namcos21_dspram16;

#define NAMCOS21_NUM_COLORS 0x8000

int namcos21_vh_start( void );
void namcos21_vh_stop( void );
void namcos21_vh_update_default( struct mame_bitmap *bitmap, int fullrefresh );
