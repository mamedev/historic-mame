/* System 32 Video Hardware */

/* todo:

fix alphablending enable, amount etc. (sonic almost certainly shouldn't have it enabled ?)
fix / add row-select, linescroll
add linezoom, clipping window effects on bg tilemaps
fix / improve alphablending
fix priorities properly (will need vmixer)
fix sprite clipping effect? (outside area clip)
find rad rally title screen background
remaining colour problems (sonic?)
solid flag on tiles? (rad rally..)
background colour
any remaining glitches

*/

#include "driver.h"

extern int system32_temp_kludge;
int sys32_sprite_priority_kludge;

extern data16_t *sys32_spriteram16;
data8_t  *sys32_spriteram8; /* I maintain this to make drawing ram based sprites easier */
extern data16_t *system32_mixerregs_monitor_a;		// mixer registers
data16_t *sys32_videoram;
data8_t sys32_ramtile_dirty[0x1000];
extern data16_t *sys32_displayenable;
extern data16_t *sys32_tilebank_external;
data16_t sys32_old_tilebank_external;

extern int multi32;

int sys32_tilebank_internal;
int sys32_old_tilebank_internal;

int sys32_paletteshift_monitor_a[4];
int sys32_palettebank_monitor_a[4];
int sys32_old_paletteshift_monitor_a[4];
int sys32_old_palettebank_monitor_a[4];

extern int system32_palMask;
extern int system32_mixerShift;
int system32_screen_mode;
int system32_screen_old_mode;
int system32_allow_high_resolution;
static int sys32_old_brightness[3];
int sys32_brightness_monitor_a[3];

data8_t system32_dirty_window[0x100];
data8_t system32_windows[4][4];
data8_t system32_old_windows[4][4];

/* these are the various attributes a sprite can have, will decide which need to be global later, maybe put them in a struct */

static int sys32sprite_indirect_palette;
static int sys32sprite_indirect_interleave;
static int sys32sprite_mystery;
static int sys32sprite_rambasedgfx;
static int sys32sprite_8bpp;
static int sys32sprite_draw_colour_f;
static int sys32sprite_yflip;
static int sys32sprite_xflip;
static int sys32sprite_use_yoffset;
static int sys32sprite_use_xoffset;
static int sys32sprite_yalign;
static int sys32sprite_xalign;
static int sys32sprite_rom_height;
static int sys32sprite_rom_width;
static int sys32sprite_rom_bank_low;
static int sys32sprite_unknown_1;
static int sys32sprite_unknown_2;
//static int sys32sprite_solid;
static int sys32sprite_screen_height;
static int sys32sprite_unknown_3;
static int sys32sprite_rom_bank_high;
static int sys32sprite_unknown_4;
static int sys32sprite_unknown_5;
static int sys32sprite_rom_bank_mid;
static int sys32sprite_screen_width;
static int sys32sprite_ypos;
static int sys32sprite_xpos;
static int sys32sprite_rom_offset;
static int sys32sprite_palette;
static int sys32sprite_monitor_select; // multi32

static data16_t *sys32sprite_table;

static int spritenum; /* used to go through the sprite list */
static int jump_x, jump_y; /* these are set during a jump command and sometimes used by the sprites afterwards */
static data16_t *spritedata_source; /* a pointer into spriteram */

static UINT32 sys32sprite_x_zoom;
static UINT32 sys32sprite_y_zoom;



/* system32_get_sprite_info

this actually draws the sprite, and could probably be optimized quite a bit ;-)
currently zooming isn't supported, shadows aren't supported etc.

*/

void system32_draw_sprite ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	data8_t *sprite_gfxdata = memory_region ( REGION_GFX2 );
	UINT32 xsrc,ysrc;
	UINT32 xdst,ydst;
	/* um .. probably a better way to do this */
	struct GfxElement *gfx=Machine->gfx[0];
	const pen_t *paldata = &gfx->colortable[0];

	/* if the gfx data is coming from RAM instead of ROM change the pointer */
	if ( sys32sprite_rambasedgfx )
	{
		sprite_gfxdata = sys32_spriteram8;
		sys32sprite_rom_offset &= 0x1ffff; /* right mask? */
	}

	ysrc = 0;
	ydst = 0;

	while ( ysrc < (sys32sprite_rom_height<<16) ) {
		int drawypos;
		xsrc = 0;
		xdst = 0;

		if (!sys32sprite_yflip)
		{
			drawypos = sys32sprite_ypos+ydst; // no flip
			if (drawypos > cliprect->max_y) ysrc = sys32sprite_rom_height<<16; // quit drawing if we've gone off the right
		}
		else
		{
			drawypos = sys32sprite_ypos+((sys32sprite_screen_height-1)-ydst); // y flip
			if (drawypos < cliprect->min_y) ysrc = sys32sprite_rom_height<<16; // quit drawing if we've gone off the left on a flipped sprite
		}

		if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y))
		{
			UINT32 *destline = (bitmap->line[drawypos]);

			while ( xsrc < (sys32sprite_rom_width<<16) ) {

				int drawxpos;

				if (!sys32sprite_xflip)
				{
					drawxpos = sys32sprite_xpos+xdst; // no flip
					if (drawxpos > cliprect->max_x) xsrc = sys32sprite_rom_width<<16; // quit drawing if we've gone off the right
				}
				else
				{
					drawxpos = sys32sprite_xpos+((sys32sprite_screen_width-1)-xdst); // x flip
					if (drawxpos < cliprect->min_x) xsrc = sys32sprite_rom_width<<16; // quit drawing if we've gone off the left on a flipped sprite
				}

				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x))
				{
					int gfxdata;
					if (sys32sprite_monitor_select) break; // drawxpos+=system32_screen_mode?52*8:40*8; (don't draw monitor 2 for now)
					if (!sys32sprite_8bpp) { // 4bpp
						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+((xsrc>>16)/2)+(ysrc>>16)*(sys32sprite_rom_width/2)]);

						if (xsrc & 0x10000) gfxdata = gfxdata & 0x0f;
						else gfxdata = (gfxdata & 0xf0) >> 4;

						if ( (!sys32sprite_draw_colour_f) && (gfxdata == 0x0f) ) gfxdata = 0;
// 						if ( (gfxdata == 0x0e) ) gfxdata = 0; // Transparency
//						if ( (gfxdata == 0x0f) ) gfxdata = 0; // Shadow

						if (sys32sprite_indirect_palette)
						{
							/* only for indirect? */
				//			if ( (gfxdata == 0x0e) ) gfxdata = 0; // Transparency
				//			if ( (gfxdata == 0x0f) ) gfxdata = 0; // Shadow
							if (gfxdata) destline[drawxpos] =  paldata[(sys32sprite_table[gfxdata] & 0xfff)];
						}
						else
						{
							if (gfxdata) destline[drawxpos] =  paldata[(gfxdata + (sys32sprite_palette * 16))];
						}

					} else { // 8bpp
						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+(xsrc>>16)+(ysrc>>16)*(sys32sprite_rom_width)]);

						if ( (!sys32sprite_draw_colour_f) && (gfxdata == 0xff) ) gfxdata = 0;

						if (sys32sprite_indirect_palette)
						{
							/* only for indirect? */
							if ( (gfxdata == 0xe0) ) gfxdata = 0; // Transparency
							if ( (gfxdata == 0xf0) ) gfxdata = 0; // Shadow

							if (gfxdata) destline[drawxpos] =  paldata[(gfxdata+(sys32sprite_table[0] & 0xfff))];
						}
						else
						{
							if (gfxdata) destline[drawxpos] =  paldata[(gfxdata + (sys32sprite_palette * 16))];
						}
					} /* bpp */

				} /* xcliping */

				xsrc+=sys32sprite_x_zoom;
				xdst++;

			}

		}

		ysrc+=sys32sprite_y_zoom;
		ydst++;

	}
#if 0
	if (spriteinfo) {
		struct DisplayText dt[3];	char buf[10];	 char buf1[10]; int x=sys32sprite_xpos;

		sprintf(buf, "%01x%01x%01x%01x",sys32sprite_indirect_palette,sys32sprite_indirect_interleave,sys32sprite_mystery,sys32sprite_8bpp);
	//		sprintf(buf, "%04x",(sys32sprite_table[1] & 0x1fff));
		dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
		if (sys32sprite_monitor_select) x+=system32_screen_mode?52*8:40*8;
		dt[0].x = x;
		dt[0].y = sys32sprite_ypos;

		sprintf(buf1, "%06x", sys32sprite_rom_offset);
		dt[1].text = buf1;	dt[1].color = UI_COLOR_NORMAL;
		dt[1].x = x;
		dt[1].y = sys32sprite_ypos+8;

		dt[2].text=0;
		displaytext(Machine->scrbitmap,dt);
	}
#endif
}

/* system32_get_sprite_info

this function is used to get information on the sprite from spriteram and call the
drawing functions

	spriteram Sprite Entry layout

	0:  ffffffff ffffffff  1:  HHHHHHHH WWWWWWWW  2:  hhhhhhhh hhhhhhhh  3:  wwwwwwww wwwwwwww
	4:  yyyyyyyy yyyyyyyy  5:  xxxxxxxx xxxxxxxx  6:  rrrrrrrr rrrrrrrr  7:  pppppppp pppppppp

	f = various flags
		xx------ -------- (0xc000) :  Command (00 for a sprite, other values would mean this isn't a sprite)
		--x----- -------- (0x2000) :  Sprite uses Indirect Palette (TRUSTED)
		---x---- -------- (0x1000) :  Sprite uses Indirect Palette which is Interleaved in Spritelist (GA2?)
		----x--- -------- (0x0800) :  Mystery, used over Shadows in some games
		-----x-- -------- (0x0400) :  Sprite GFX data comes from Spriteram, not ROM (TRUSTED)
		------x- -------- (0x0200) :  Sprite is 8bpp not 4bpp (TRUSTED)
		-------x -------- (0x0100) :  If NOT set colour in palette 0x0f is transparent (TRUSTED)
		-------- x------- (0x0080) :  Sprite Y-Flip (TRUSTED)
		-------- -x------ (0x0040) :  Sprite X-Flip (TRUSTED)
		-------- --x----- (0x0020) :  Use Y offset (offset set in last jump) (not trusted)
		-------- ---x---- (0x0010) :  Use X offset (offset set in last jump) (TRUSTED)
		-------- ----xx-- (0x000c) :  Y alignment. 00=Center, 10=Start, 01=End (TRUSTED)
		-------- ------xx (0x0003) :  X alignment. 00=Center, 10=Start, 01=End (TRUSTED)

	H = height of sprite in ROM
	W = width  of sprite in ROM (multiply by 4 to get screen width)

	System32:
	w = width to draw on SCREEN + extra attributes
		x------- -------- (0x8000) :  unknown
		-x------ -------- (0x4000) :  Bit 5 of Sprite ROM Bank (TRUSTED)
		--x----- -------- (0x2000) :  unknown
		---x---- -------- (0x1000) :  unknown
		----x--- -------- (0x0800) :  Bit 4 of Sprite ROM Bank (TRUSTED)
		-----xxx xxxxxxxx (0x07ff) :  Width to draw on screen (TRUSTED)

	Multi32:
	w = width to draw on SCREEN + extra attributes
		x------- -------- (0x8000) :  bit 5 of the sprite bank (TRUSTED)
		-x------ -------- (0x4000) :  unknown
		--x----- -------- (0x2000) :  Bit 4 of the sprite bank (TRUSTED)
		---x---- -------- (0x1000) :  unknown
		----x--- -------- (0x0800) :  Monitor selection for this sprite (TRUSTED)
		-----xxx xxxxxxxx (0x07ff) :  Width to draw on screen (TRUSTED)
	y = y-position (12-bit?, high bit = sign bit?)

	x = x-position (12-bit, high bit = sign bit)

	r = ROM Offset of GFX data (multiply by 4 to get real offset)

	p = Palette & Priority bits, I think these change depending on the mode (Direct or Indirect)
		DIRECT MODE *probably wrong, holoseum needed a kludge to work
		xxxxx--- -------- (0xf800) :  unknown
		-----xxx xxxx---- (0x07f0) :  palette #
		-------- ----xxxx (0x000f) :  unknown

*/

void system32_get_sprite_info ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/* get attributes */
	int disabled = 0;
	int px, mixerinput;

	sys32sprite_indirect_palette		= (spritedata_source[0]&0x2000) >> 13;
	sys32sprite_indirect_interleave		= (spritedata_source[0]&0x1000) >> 12;
	sys32sprite_mystery					= (spritedata_source[0]&0x0800) >> 11;
	sys32sprite_rambasedgfx				= (spritedata_source[0]&0x0400) >> 10;
	sys32sprite_8bpp					= (spritedata_source[0]&0x0200) >> 9;
	sys32sprite_draw_colour_f			= (spritedata_source[0]&0x0100) >> 8;
	sys32sprite_yflip					= (spritedata_source[0]&0x0080) >> 7;
	sys32sprite_xflip					= (spritedata_source[0]&0x0040) >> 6;
	sys32sprite_use_yoffset				= (spritedata_source[0]&0x0020) >> 5;
	sys32sprite_use_xoffset				= (spritedata_source[0]&0x0010) >> 4;
	sys32sprite_yalign					= (spritedata_source[0]&0x000c) >> 2;
	sys32sprite_xalign					= (spritedata_source[0]&0x0003) >> 0;

	sys32sprite_rom_height				= (spritedata_source[1]&0xff00) >> 8;
	sys32sprite_rom_width				= (spritedata_source[1]&0x00ff) >> 0;

	sys32sprite_rom_bank_low			= (spritedata_source[2]&0xf000) >> 12;
	sys32sprite_unknown_1				= (spritedata_source[2]&0x0800) >> 11;
	sys32sprite_unknown_2				= (spritedata_source[2]&0x0400) >> 10;
	sys32sprite_screen_height			= (spritedata_source[2]&0x03ff) >> 0;

	if (multi32)
	{
		sys32sprite_rom_bank_high			= (spritedata_source[3]&0x8000) >> 15;
		sys32sprite_unknown_3				= (spritedata_source[3]&0x4000) >> 14;
		sys32sprite_rom_bank_mid			= (spritedata_source[3]&0x2000) >> 13;
		sys32sprite_unknown_4				= (spritedata_source[3]&0x1000) >> 12;
		sys32sprite_monitor_select			= (spritedata_source[3]&0x0800) >> 11;
	}
	else
	{
		sys32sprite_unknown_3				= (spritedata_source[3]&0x8000) >> 15;
		sys32sprite_rom_bank_high			= (spritedata_source[3]&0x4000) >> 14;
		sys32sprite_unknown_4				= (spritedata_source[3]&0x2000) >> 13;
		sys32sprite_unknown_5				= (spritedata_source[3]&0x1000) >> 12;
		sys32sprite_rom_bank_mid			= (spritedata_source[3]&0x0800) >> 11;
	}
		sys32sprite_screen_width			= (spritedata_source[3]&0x07ff) >> 0;

	sys32sprite_ypos					= (spritedata_source[4]&0xffff) >> 0;

	sys32sprite_xpos					= (spritedata_source[5]&0xffff) >> 0;

	sys32sprite_rom_offset				= (spritedata_source[6]&0xffff) >> 0;

//	sys32sprite_palette					= (spritedata_source[7]&system32_palMask) >> system32_mixerShift; /* direct mode */
	px = spritedata_source[7];
	mixerinput = (px >> (system32_mixerShift + 8)) & 0xf;
	sys32sprite_palette = (px >> 4) & system32_palMask;
	sys32sprite_palette += (system32_mixerregs_monitor_a[mixerinput] & 0x30)<<2;

	/* process attributes */

	sys32sprite_rom_width = sys32sprite_rom_width << 2;
	sys32sprite_rom_offset = sys32sprite_rom_offset | (sys32sprite_rom_bank_low << 16) | (sys32sprite_rom_bank_mid << 20) | (sys32sprite_rom_bank_high << 21);
	sys32sprite_rom_offset = sys32sprite_rom_offset << 2;


	if (sys32sprite_screen_width == 0) disabled = 1;
	if (sys32sprite_screen_height == 0) disabled = 1;
	if (sys32sprite_rom_height == 0) disabled = 1;
	if (sys32sprite_rom_width == 0) disabled = 1;

	if (sys32sprite_indirect_palette)
	{
		if (sys32sprite_indirect_interleave) /* indirect mode where the table is included in the display list */
		{
			sys32sprite_table = spritedata_source+8;
			spritenum+=2;
		}
		else /* indirect mode where the display list contains an offset to the table */
		{
			sys32sprite_table = sys32_spriteram16 + ((spritedata_source[7] & 0x1fff)*8);
/*
			if (spritedata_source[7]==0x226) {
				for (y=0;y<255;y++) {
					logerror("mixerreg[%02x]: %04x\n",y,mixer_regs8[y]);
				}
			}
//			logerror("indirect palette sprite %04d: %04x\n",spritenum, spritedata_source[7]);
*/
		}
	}

	//usrintf_showmessage	("stuff");

	if (!disabled)
	{

	sys32sprite_y_zoom = (sys32sprite_rom_height << 16) / (sys32sprite_screen_height);
	sys32sprite_x_zoom = (sys32sprite_rom_width << 16) / (sys32sprite_screen_width);

	if (sys32sprite_use_yoffset) sys32sprite_ypos += jump_y;
	if (sys32sprite_use_xoffset) sys32sprite_xpos += jump_x;

	/* adjust positions according to offsets if used (radm, radr, alien3, darkedge etc.) */

	/* adjust sprite positions based on alignment, pretty much straight from modeler */
	switch (sys32sprite_xalign)
	{
		case 0: // centerX
		case 3:
			sys32sprite_xpos -= (sys32sprite_screen_width-1) / 2; // this is trusted again spiderman truck door
			break;
		case 1: // rightX
			sys32sprite_xpos -= sys32sprite_screen_width - 1;
			break;
		case 2: // leftX
			break;
	}

	switch (sys32sprite_yalign)
	{
		case 0: // centerY
		case 3:
			sys32sprite_ypos -= (sys32sprite_screen_height-1) / 2; // this is trusted against alien3 energy bars
			break;
		case 1: // bottomY
			sys32sprite_ypos -= sys32sprite_screen_height - 1;
			break;
		case 2: // topY
			break;
	}

	sys32sprite_xpos &= 0x0fff;
	sys32sprite_ypos &= 0x0fff;

	/* sprite positions are signed */
	if (sys32sprite_ypos & 0x0800) sys32sprite_ypos -= 0x1000;
	if (sys32sprite_xpos & 0x0800) sys32sprite_xpos -= 0x1000;

	system32_draw_sprite ( bitmap, cliprect );


	}

}


/* Sprite RAM

each entry in the sprite list is 16 bytes (8 words)
the sprite list itself consists of 4 main different types of entry
 a normal sprite

	0:  00------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

		(See Above for bit usage)

 a command to set the clipping area

 	0:  01------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
 	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

 		(to be filled in later)

 a jump command

  	0:  10ujjjjj jjjjjjjj  1:  yyyyyyyy yyyyyyyy  2:  xxxxxxxx xxxxxxxx  3:  -------- --------
  	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

  		u = set sprite offset positions with this jump (alien3 proves this test is needed)
		j = sprite number to jump to
		y = sprite y offset to use (? bits) (only set if u = 1)
		x = sprite x offset to use (? bits) (only set if u = 1)

		other bits unused / unknown

 a terminate list command

  	0:  11------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
  	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

  		(other bits unused, list is terminated)

sprite ram can also contain palette look up data for the special indirect
palette modes, as well as sprite gfx data which is used instead of the gfx
in the roms if a bit in the sprite entry is set.

*/

void system32_process_spritelist ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int processed;
	int command;
	struct rectangle clip;

	/* set clipping defaults */
	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	clip.min_y = Machine->visible_area.min_y;
	clip.max_y = Machine->visible_area.max_y;

	processed = 0;
	spritenum = 0;

	while (spritenum < 0x20000/16)
	{
		spritedata_source = sys32_spriteram16 + 8 * spritenum;

		command = (spritedata_source[0] & 0xc000) >> 14;

		switch (command)
		{
			case 0x3: /* end of sprite list */
//				logerror ("SPRITELIST: terminated at sprite %06x\n", spritenum*16);
				spritenum = 60000; /* just set a high sprite number so we stop processing */
				break;
			case 0x2: /* jump to position in sprite list*/
//				logerror ("SPRITELIST: jump at sprite %06x to %06x extra data 0 %04x 1 %04x, 2 %04x 3 %04x 4 %04x 5 %04x 6 %04x 7 %04x\n", spritenum*16, (spritedata_source[0] & 0x1fff)*16, spritedata_source[0] & 0x2000, spritedata_source[1], spritedata_source[2], spritedata_source[3] , spritedata_source[4] , spritedata_source[5] ,spritedata_source[6] , spritedata_source[7] );
				spritenum = spritedata_source[0] & 0x1fff;
				if (spritedata_source[0] & 0x2000)
				{
					jump_y = spritedata_source[1];
					jump_x = spritedata_source[2];
				}
				break;
			case 0x1: /* set clipping registers */
//				logerror ("SPRITELIST: set clip regs at %06x extra data 0 %04x 1 %04x 2 %04x 3 %04x 4 %04x 5 %04x 6 %04x 7 %04x\n", spritenum*16, spritedata_source[0], spritedata_source[1],spritedata_source[2],spritedata_source[3],spritedata_source[4],spritedata_source[5],spritedata_source[6],spritedata_source[7]  );
				{

					if (spritedata_source[0] & 0x3000) /* alien 3 needs something like this ... */
					{
						clip.min_y = spritedata_source[0]& 0x0fff;
						clip.max_y = spritedata_source[1]& 0x0fff;
						clip.min_x = spritedata_source[2]& 0x0fff;
						clip.max_x = spritedata_source[3]& 0x0fff;

						if  (clip.max_y > Machine->visible_area.max_y) clip.max_y = Machine->visible_area.max_y;
						if  (clip.max_x > Machine->visible_area.max_x) clip.max_x = Machine->visible_area.max_x;
					}
					else
					{
						clip.min_x = Machine->visible_area.min_x;
						clip.max_x = Machine->visible_area.max_x;
						clip.min_y = Machine->visible_area.min_y;
						clip.max_y = Machine->visible_area.max_y;
					}

				}

				spritenum ++;
				break;
			case 0x0: /* draw sprite */
//				logerror ("SPRITELIST: draw sprite at %06x\n", spritenum*16 );
				system32_get_sprite_info (bitmap, &clip);
				spritenum ++;
				break;
		}

		processed++;
		if (processed > 0x20000/16) /* its dead ;-) */
		{
//			logerror ("SPRITELIST: terminated due to infinite loop\n");
			spritenum = 16384;
		};

	}


}

/* 0x31ff00 - 0x31ffff are video registers */

/*

tile banking is controlled by a register in here as well as a register external to the tilemap chip
which is mapped at 0xc0000e
*/
/*
	00 | rR-- -b--  ---- ----    |  b = tile bank low bit ( | 0x2000 ), not multi-32  r = screen resolution R also resolution?
	02 | ---- ----  ---- dddd    |  d = tilemap disable registers
	04 |
	06 |
	08 |
	0a |
	0c |
	0e |
	10 |
	12 | scroll x for tilemap 0
	14 |
	16 | scroll y for tilemap 0
	18 |
	1a | scroll x for tilemap 1
	1c |
	1e | scroll y for tilemap 1
	20 |
	22 | scroll x for tilemap 2
	24 |
	26 | scroll y for tilemap 2
	28 |
	2a | scroll x for tilemap 3
	2c |
	2e | scroll y for tilemap 3
	30 |
	32 |
	34 |
	36 |
	38 |
	3a |
	3c |
	3e |
	40 | pages 0 + 1 of tilemap 0
	42 | pages 2 + 3 of tilemap 0
	44 | pages 0 + 1 of tilemap 1
	46 | pages 2 + 3 of tilemap 1
	48 | pages 0 + 1 of tilemap 2
	4a | pages 2 + 3 of tilemap 2
	4c | pages 0 + 1 of tilemap 3
	4e | pages 2 + 3 of tilemap 4
	50 |
	52 |
	54 |
	56 |
	58 |
	5a |
	5c |
	5e |
	60 |
	62 |
	64 |
	66 |
	.... etc.. fill the rest in later

*/


/* mixer regs

00
02
04
06
08
0a
0c
0e
10
12
14
16
18
1a
1c
1e
20 ---- ---- ---- pppp  p = priority text?
22 ---- ssss bbbb pppp  (Tilemap Palette Base + Shifting, b = bank, s = shift p = priority 0)
24 ---- ssss bbbb pppp  p = priority 1
26 ---- ssss bbbb pppp  p = priority 2
28 ---- ssss bbbb pppp  p = priority 3
2a
2c
2e
30
32 ---e ---- ---e ----  e = alpha enable 0
34 ---e ---- ---e ----  e = alpha enable 1
36 ---e ---- ---e ----  e = alpha enable 2
38 ---e ---- ---e ----  e = alpha enable 3
3a
3c
3e
40 bbbb bbbb bbbb bbbb  b = brightness (layer?) text?  or r ?
42 bbbb bbbb bbbb bbbb  b = brightness (layer?) 0?     or g ?
44 bbbb bbbb bbbb bbbb  b = brightness (layer?) 1?     or b ?
46 bbbb bbbb bbbb bbbb  b = brightness? (layer?) 2?     or r ? (jpark)
48 bbbb bbbb bbbb bbbb  b = brightness? (layer?) 3?     or g ? (jpark)
4a bbbb bbbb bbbb bbbb  b = brightness? (layer?)        or b ? (jpark)
4c
4e bbbb bbbb ---- ----   b = alpha blend amount?
50
52
54
56
58
5a
5c
5e

....
*/


void system32_draw_text_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) /* using this for now to save me tilemap system related headaches */
{
	int x,y;
	int textbank = sys32_videoram[0x01ff5c/2] & 0x0007;
	int tmaddress = (sys32_videoram[0x01ff5c/2] & 0x00f0) >> 4;
	/* this register is like this

	 ---- ----  tttt -bbb

	 t = address of tilemap data (used by dbzvrvs)
	 b = address of tile gfx data (used by radmobile / radrally ingame, jpark)

	 */

	data8_t *txtile_gfxregion = memory_region(REGION_GFX3);
	data16_t* tx_tilemapbase = sys32_videoram + ((0x10000+tmaddress*0x1000) /2);

	for (y = 0; y < 32 ; y++)
	{
		for (x = 0; x < 64 ; x++)
		{
			int data=tx_tilemapbase[x+y*64];
			int code = data & 0x01ff;
			int pal = (data>>9) & 0x7f;
			int drawypos, flip;

			pal += (((system32_mixerregs_monitor_a[0x10] & 0xf0) >> 4) * 0x40);

			code += textbank * 0x200;

			if (sys32_ramtile_dirty[code])
			{
				decodechar(Machine->gfx[1], code, (data8_t*)txtile_gfxregion, Machine->drv->gfxdecodeinfo[1].gfxlayout);
				sys32_ramtile_dirty[code] = 0;
			}

			if (system32_temp_kludge != 1)
			{
				drawypos = y*8;
				flip = 0;
			}
			else /* holoseum, actually probably requires the sprites to be globally flipped + game ROT180 not the tilemap */
			{
				drawypos = 215-y*8;
				flip = 1;
			}

			drawgfx(bitmap,Machine->gfx[1],code,pal,0,flip,x*8,drawypos,cliprect,TRANSPARENCY_PEN,0);

		}
	}
}

READ16_HANDLER ( sys32_videoram_r )
{
	return sys32_videoram[offset];
}

WRITE16_HANDLER ( sys32_videoram_w )
{
	data8_t *txtile_gfxregion = memory_region(REGION_GFX3);

	COMBINE_DATA(&sys32_videoram[offset]);


	/* also write it to another region so its easier (imo) to work with the ram based tiles */
	if (ACCESSING_MSB)
	txtile_gfxregion[offset*2+1] = (data & 0xff00) >> 8;

	if (ACCESSING_LSB)
	txtile_gfxregion[offset*2] = (data & 0x00ff );

	/* each tile is 0x10 words */
	sys32_ramtile_dirty[offset / 0x10] = 1;

	system32_dirty_window[offset>>9]=1;

}

WRITE16_HANDLER ( sys32_spriteram_w )
{

	COMBINE_DATA(&sys32_spriteram16[offset]);

	/* also write it to another region so its easier to work with when drawing sprites with RAM based gfx */
	if (ACCESSING_MSB)
	sys32_spriteram8[offset*2+1] = (data & 0xff00) >> 8;

	if (ACCESSING_LSB)
	sys32_spriteram8[offset*2] = (data & 0x00ff );
}

/*

Tilemaps are made of 4 windows

each window is 32x16 in size

*/

UINT32 sys32_bg_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	int page = 0;
	if( row<16 ){ /* top */
		if( col<32 ) page = 0; else page = 1;
	}
	else { /* bottom */
		if( col<32 ) page = 2; else page = 3;
	}

	return ((col & 31) + (row & 15) * 32) + page * 0x200;

}


static struct tilemap *system32_layer_tilemap[4];

static void get_system32_tile_info ( int tile_index, int layer )
{
	int tileno, s32palette;
	int page;
	int yxflip;

	page = tile_index >> 9;

	tileno = sys32_videoram[(tile_index&0x1ff)+system32_windows[layer][page]*0x200];
	s32palette = (tileno & 0x1ff0) >> (sys32_paletteshift_monitor_a[layer]+4);
	yxflip = (tileno & 0xc000)>>14;

	tileno &= 0x1fff;

	if (multi32) {
	//	tileno|=(sys32_tilebank_external[0]<<(13-layer*2))&0x6000;
		tileno|=(sys32_tilebank_external[0]>>(layer*2)&3)*0x2000;
	}
	else {
		if (sys32_tilebank_internal) tileno |= 0x2000;
		if (sys32_tilebank_external[0]&1) tileno |= 0x4000;
	}

//	if (tilebank_internal_toggle) tileno |= 0x2000;
//	if (tilebank_external_toggle) tileno |= 0x4000;
/*
	if (tile_index==0) temp_tileno[layer]=tileno;

	if (log_tileno) {
		tileno |= 0x4000;
		logerror("tileno: %04x\n",tileno);
	}
*/
	SET_TILE_INFO(0,tileno,sys32_palettebank_monitor_a[layer]+s32palette,TILE_FLIPYX(yxflip))
}
static void get_system32_layer0_tile_info(int tile_index) {	get_system32_tile_info(tile_index,0); }
static void get_system32_layer1_tile_info(int tile_index) {	get_system32_tile_info(tile_index,1); }
static void get_system32_layer2_tile_info(int tile_index) {	get_system32_tile_info(tile_index,2); }
static void get_system32_layer3_tile_info(int tile_index) {	get_system32_tile_info(tile_index,3); }

VIDEO_START( system32 )
{
	int i;

	system32_layer_tilemap[0] = tilemap_create(get_system32_layer0_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[0],0);
	system32_layer_tilemap[1] = tilemap_create(get_system32_layer1_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[1],0);
	system32_layer_tilemap[2] = tilemap_create(get_system32_layer2_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[2],0);
	system32_layer_tilemap[3] = tilemap_create(get_system32_layer3_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[3],0);

	sys32_spriteram8 = auto_malloc ( 0x20000 ); // for ram sprites
	sys32_videoram = auto_malloc ( 0x20000 );
	sys32_old_brightness[0] = 0; sys32_old_brightness[1]=0; sys32_old_brightness[2] = 0;
	sys32_brightness_monitor_a[0] = 0xff;	sys32_brightness_monitor_a[1] = 0xff;
	sys32_brightness_monitor_a[2] = 0xff;

	for (i = 0; i < 0x100; i++)
		system32_dirty_window[i] = 1;

	return 0;
}

void system32_set_colour (int offset);

static void system32_recalc_palette( void )
{
	int i;
	for (i = 0; i < 0x4000; i++)
		system32_set_colour (i);
}



void system32_draw_bg_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer )
{
	int trans = 0;
	int alphaamount = 0;

	if ((system32_mixerregs_monitor_a[(0x32+2*layer)/2] & 0x1010) == 0x1010)
	{
		trans = TILEMAP_ALPHA;
		alphaamount = 255-((((system32_mixerregs_monitor_a[0x4e/2])>>8) & 7) <<5); //umm this is almost certainly wrong
		alpha_set_level(alphaamount);
	}

	/* test Rowscroll / Rowselect with outrunners road ONLY for now */
	if ((!strcmp(Machine->gamedrv->name,"orunners")) && (layer == 2))
	{
/* modeler code (offsets are byte not word..)
	unsigned int tableWindow = MemRead8(0x31FF05);
	int num = currentLayerNum - 2;

	short* rowScrollTable = (short*)MemGetPointer(0x300000 + (tableWindow + num) * 0x200);
	short* rowSelectTable = (short*)MemGetPointer(0x300000 + (tableWindow + num + 2) * 0x200);

	if (currentGameID == GA2 && num == 1)
	{
		if (numlayersux == 0)
			rowScrollTable = (short*)MemGetPointer(0x300000 + (tableWindow + 0) * 0x200);
		else
			rowScrollTable = (short*)MemGetPointer(0x300000 + (tableWindow + 2) * 0x200);
		//rowSelectTable = (short*)MemGetPointer(0x300000 + (tableWindow + 0) * 0x200);
		rowSelectTable=NULL;
	}

	if (currentGameID == RADRALLY)
	{
		rowScrollTable = (short*)MemGetPointer(0x300000 + (tableWindow + 1) * 0x200);
		rowSelectTable = (short*)MemGetPointer(0x300000 + (tableWindow + 3) * 0x200);
	}
*/

		/* This is working for the ingame gfx but not for the title screen, investigate */

		int line;
		struct rectangle clip;

		int tableaddress = sys32_videoram[0x01FF04/2]>>8;

		tableaddress = (tableaddress * 0x200);

	//	int num = layer - 1;

	//	int rowscrolltable = 0x100*(tableaddress+num);
	//	int rowselecttable = 0x200*(tableaddress+num+1);
/*
		if ( keyboard_pressed_memory(KEYCODE_W) )
		{
			FILE *fp;
			fp=fopen("videoram.dmp", "w+b");
			if (fp)
			{
				fwrite(sys32_videoram, 0x01ffff, 1, fp);
				fclose(fp);
			}
		}
*/



//		usrintf_showmessage	("table %04x",tableaddress);

		clip.min_x = Machine->visible_area.min_x;
		clip.max_x = Machine->visible_area.max_x;
		clip.min_y = Machine->visible_area.min_y;
		clip.max_y = Machine->visible_area.max_y;

		for (line = 0; line < 224;line++)
		{
			clip.min_y = clip.max_y = line;

			tilemap_set_scrollx(system32_layer_tilemap[layer],0,((sys32_videoram[(0x01FF12+8*layer)/2]) + (sys32_videoram[(tableaddress/2)+line])) & 0x3ff);
			tilemap_set_scrolly(system32_layer_tilemap[layer],0,((sys32_videoram[(0x01FF16+8*layer)/2]) + (sys32_videoram[((tableaddress+0x400)/2)+line])-line) & 0x1ff);
			tilemap_draw(bitmap,&clip,system32_layer_tilemap[layer],trans,0);

		}

	}
	else
	{
		tilemap_set_scrollx(system32_layer_tilemap[layer],0,(sys32_videoram[(0x01FF12+8*layer)/2]) & 0x3ff);
		tilemap_set_scrolly(system32_layer_tilemap[layer],0,(sys32_videoram[(0x01FF16+8*layer)/2]) & 0x1ff);
		tilemap_draw(bitmap,cliprect,system32_layer_tilemap[layer],trans,0);
	}
}


VIDEO_UPDATE( system32 )
{
	int sys32_tmap_disabled = sys32_videoram[0x1FF02/2] & 0x000f;

	int priority0 = (system32_mixerregs_monitor_a[0x22/2] & 0x000f);
	int priority1 = (system32_mixerregs_monitor_a[0x24/2] & 0x000f);
	int priority2 = (system32_mixerregs_monitor_a[0x26/2] & 0x000f);
	int priority3 = (system32_mixerregs_monitor_a[0x28/2] & 0x000f);
	int priloop;
	int sys32_palette_dirty = 0;

	/* experimental wip code */
	int tm,ii;

	/* if the windows number used by a tilemap use change then that window of the tilemap needs to be considered dirty */
	for (tm = 0; tm < 4; tm++)
	{
		system32_windows[tm][1] = (sys32_videoram[(0x01FF40+4*tm)/2] & 0x7f00)>>8;
		system32_windows[tm][0] = (sys32_videoram[(0x01FF40+4*tm)/2] & 0x007f);
		system32_windows[tm][3] = (sys32_videoram[(0x01FF42+4*tm)/2] & 0x7f00)>>8;
		system32_windows[tm][2] = (sys32_videoram[(0x01FF42+4*tm)/2] & 0x007f);

		if (system32_windows[tm][0] != system32_old_windows[tm][0]) { for (ii = 0x000 ; ii < 0x200 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_windows[tm][1] != system32_old_windows[tm][1]) { for (ii = 0x200 ; ii < 0x400 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_windows[tm][2] != system32_old_windows[tm][2]) { for (ii = 0x400 ; ii < 0x600 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_windows[tm][3] != system32_old_windows[tm][3]) { for (ii = 0x600 ; ii < 0x800 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }

		/* if the actual windows are dirty we also need to mark them dirty in the tilemap */
		if (system32_dirty_window [ system32_windows[tm][0] ]) { for (ii = 0x000 ; ii < 0x200 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_dirty_window [ system32_windows[tm][1] ]) { for (ii = 0x200 ; ii < 0x400 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_dirty_window [ system32_windows[tm][2] ]) { for (ii = 0x400 ; ii < 0x600 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }
		if (system32_dirty_window [ system32_windows[tm][3] ]) { for (ii = 0x600 ; ii < 0x800 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii); }

		system32_old_windows[tm][0] = system32_windows[tm][0];
		system32_old_windows[tm][1] = system32_windows[tm][1];
		system32_old_windows[tm][2] = system32_windows[tm][2];
		system32_old_windows[tm][3] = system32_windows[tm][3];
	}

	/* we can clean the dirty window markers now */
	for (ii = 0; ii < 0x100; ii++)
		system32_dirty_window[ii] = 0;

	/* if the internal tilebank changed everything is dirty */
	sys32_tilebank_internal = sys32_videoram[0x01FF00/2] & 0x0400;
	if (sys32_tilebank_internal != sys32_old_tilebank_internal)
	{
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[0]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[1]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[2]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[3]);
	}
	sys32_old_tilebank_internal = sys32_tilebank_internal;

	/* if the external tilebank changed everything is dirty */
	if  ( (sys32_tilebank_external[0]) != sys32_old_tilebank_external )
	{
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[0]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[1]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[2]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[3]);
	}
	sys32_old_tilebank_external = sys32_tilebank_external[0];

	/* if the palette shift /bank registers changed the tilemap is dirty, not sure these are regs 100% correct some odd colours in sonic / jpark */
	for (tm = 0; tm < 4; tm++)
	{
		sys32_paletteshift_monitor_a[tm] = (system32_mixerregs_monitor_a[(0x22+tm*2)/2] & 0x0f00)>>8;
		if (sys32_paletteshift_monitor_a[tm] != sys32_old_paletteshift_monitor_a[tm]) tilemap_mark_all_tiles_dirty(system32_layer_tilemap[tm]);
		sys32_old_paletteshift_monitor_a[tm] = sys32_paletteshift_monitor_a[tm];

		sys32_palettebank_monitor_a[tm] = ((system32_mixerregs_monitor_a[(0x22+tm*2)/2] & 0x00f0)>>4)*0x40;
		if (sys32_palettebank_monitor_a[tm] != sys32_old_palettebank_monitor_a[tm]) tilemap_mark_all_tiles_dirty(system32_layer_tilemap[tm]);
		sys32_old_palettebank_monitor_a[tm] = sys32_palettebank_monitor_a[tm];

	}
	/* end wip code */

	/* palette dirty check */
	sys32_brightness_monitor_a[0] = (system32_mixerregs_monitor_a[0x40/2]);
	sys32_brightness_monitor_a[1] = (system32_mixerregs_monitor_a[0x42/2]);
	sys32_brightness_monitor_a[2] = (system32_mixerregs_monitor_a[0x44/2]);

	if (sys32_brightness_monitor_a[0] != sys32_old_brightness[0]) { sys32_old_brightness[0] = sys32_brightness_monitor_a[0]; sys32_palette_dirty = 1; }
	if (sys32_brightness_monitor_a[1] != sys32_old_brightness[1]) { sys32_old_brightness[1] = sys32_brightness_monitor_a[1]; sys32_palette_dirty = 1; }
	if (sys32_brightness_monitor_a[2] != sys32_old_brightness[2]) { sys32_old_brightness[2] = sys32_brightness_monitor_a[2]; sys32_palette_dirty = 1; }

	if (sys32_palette_dirty)
	{
		sys32_palette_dirty = 0;
		system32_recalc_palette();
	}
	/* end palette dirty */

#if 0
	if ( keyboard_pressed_memory(KEYCODE_E) )
	{
	tilemap_mark_all_tiles_dirty(system32_layer_tilemap[0]);
	tilemap_mark_all_tiles_dirty(system32_layer_tilemap[1]);
	tilemap_mark_all_tiles_dirty(system32_layer_tilemap[2]);
	tilemap_mark_all_tiles_dirty(system32_layer_tilemap[3]);
	}
#endif

	system32_screen_mode = sys32_videoram[0x01FF00/2] & 0xc000;  // this should be 0x8000 according to modeler but then brival is broken?  this way alien3 and arabfgt try to change when they shouldn't .. wrong register?

//  usrintf_showmessage ( "sys32_videoram[0x01FF00/2] %04x",sys32_videoram[0x01FF00/2]);

	if (system32_screen_mode!=system32_screen_old_mode)
	{
		system32_screen_old_mode = system32_screen_mode;
		if (system32_screen_mode)
		{
			if (system32_allow_high_resolution)
			{
				fillbitmap(bitmap, 0, 0);
				set_visible_area(0*8, 52*8-1, 0*8, 28*8-1);
			}
		//	else usrintf_showmessage ("attempted to switch to hi-resolution mode!");
		}
		else
		{
			fillbitmap(bitmap, 0, 0);
			set_visible_area(0*8, 40*8-1, 0*8, 28*8-1);
		}
	}


	fillbitmap(bitmap, 0, 0);

	if (!multi32)
	{
		if (sys32_displayenable[0] & 0x0002) {

			for (priloop = 0; priloop < 0x10; priloop++)
			{
				if (priloop == priority0) {	if (!(sys32_tmap_disabled & 0x1)) system32_draw_bg_layer (bitmap,cliprect,0); }
				if (priloop == priority1) {	if (!(sys32_tmap_disabled & 0x2)) system32_draw_bg_layer (bitmap,cliprect,1); }
				if (priloop == priority2) {	if (!(sys32_tmap_disabled & 0x4)) system32_draw_bg_layer (bitmap,cliprect,2); }
				if (priloop == priority3) {	if (!(sys32_tmap_disabled & 0x8)) system32_draw_bg_layer (bitmap,cliprect,3); }

				/* handle real sprite priorities once we have the vmixer, this is a kludge to make radm/radr/jpark ingame look a bit better until then */
				if (priloop == sys32_sprite_priority_kludge) system32_process_spritelist (bitmap, cliprect);
			}
		}
	} else { // multi 32 has 2 tilemaps for each monitor we only draw monitor 1 for now ..

		if (sys32_displayenable[0] & 0x0002) {

			for (priloop = 0; priloop < 0x10; priloop++)
			{
				if (priloop == priority0) {	if (!(sys32_tmap_disabled & 0x1)) system32_draw_bg_layer (bitmap,cliprect,0); }
			//	if (priloop == priority1) {	if (!(sys32_tmap_disabled & 0x2)) system32_draw_bg_layer (bitmap,cliprect,1); }
				if (priloop == priority2) { if (!(sys32_tmap_disabled & 0x4)) system32_draw_bg_layer (bitmap,cliprect,2); }
			//	if (priloop == priority3) {	if (!(sys32_tmap_disabled & 0x8)) system32_draw_bg_layer (bitmap,cliprect,3); }

				/* handle real sprite priorities once we have the vmixer, this is a kludge to make radm/radr/jpark ingame look a bit better until then */
				if (priloop == sys32_sprite_priority_kludge) system32_process_spritelist (bitmap, cliprect);
			}
		}

 	}

	system32_draw_text_layer (bitmap, cliprect);

}
