/*************************************************************************

    Sega Z80-3D system

*************************************************************************/

#include "sound/discrete.h"

/* sprites are scaled in the analog domain; to give a better */
/* rendition of this, we scale in the X direction by this factor */
#define TURBO_X_SCALE		2


typedef struct _i8279_state i8279_state;
struct _i8279_state
{
	UINT8		command;
	UINT8		mode;
	UINT8		prescale;
	UINT8		inhibit;
	UINT8		clear;
	UINT8		ram[16];
};


typedef struct _turbo_state turbo_state;
struct _turbo_state
{
	/* memory pointers */
	UINT8 *		videoram;
	UINT8 *		spriteram;
	UINT8 *		sprite_position;
	UINT8 *		buckrog_bitmap_ram;

	/* machine states */
	i8279_state	i8279;

	/* video state */
	tilemap *	fg_tilemap;

	/* Turbo-specific states */
	UINT8		turbo_opa, turbo_opb, turbo_opc;
	UINT8		turbo_ipa, turbo_ipb, turbo_ipc;
	UINT8		turbo_fbpla, turbo_fbcol;
	UINT8		turbo_speed;
	UINT8		turbo_collision;
	UINT8		turbo_last_analog;

	/* Subroc-specific states */
	UINT8		subroc3d_col, subroc3d_ply, subroc3d_flip;

	/* Buck Rogers-specific states */
	UINT8		buckrog_fchg, buckrog_mov, buckrog_obch;
	UINT8		buckrog_command;
};


/*----------- defined in drivers/turbo.c -----------*/

void turbo_update_tachometer(void);
void turbo_update_segments(void);



/*----------- defined in sndhrdw/turbo.c -----------*/

extern discrete_sound_block turbo_sound_interface[];

WRITE8_HANDLER( turbo_sound_A_w );
WRITE8_HANDLER( turbo_sound_B_w );
WRITE8_HANDLER( turbo_sound_C_w );

WRITE8_HANDLER( subroc3d_sound_A_w );
WRITE8_HANDLER( subroc3d_sound_B_w );
WRITE8_HANDLER( subroc3d_sound_C_w );

WRITE8_HANDLER( buckrog_sound_A_w );
WRITE8_HANDLER( buckrog_sound_B_w );


/*----------- defined in vidhrdw/turbo.c -----------*/

PALETTE_INIT( turbo );
VIDEO_START( turbo );
VIDEO_UPDATE( turbo );

PALETTE_INIT( subroc3d );
VIDEO_UPDATE( subroc3d );

PALETTE_INIT( buckrog );
VIDEO_START( buckrog );
VIDEO_UPDATE( buckrog );

WRITE8_HANDLER( turbo_videoram_w );
WRITE8_HANDLER( buckrog_bitmap_w );
