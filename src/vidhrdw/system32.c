/* System 32 Video Hardware */

/* tx tilemap was annoying so i'm not using the tilemap system for now, i'll probably convert it back later once all the details of the
   vidhrdw are working and i know what i can do */
/* bg tilemaps aren't done */
/* are colours 100% in svf? crowds seem a bit odd, also colours in f1en, darkedge */
/* there might be more to the sprite clipping than i emulate */
/* shadow sprites etc. */
/* support changing to the higher resolutions at runtime sonic warning screen, rad rally course select etc.*/
/* optimize */
/* some sprites on the 3rd attract level of ga2 still have bad colours .. my problem or more protection? */

#include "driver.h"

extern int system32_temp_kludge;
extern data16_t *sys32_spriteram16;
data8_t  *sys32_spriteram8; /* I maintain this to make drawing ram based sprites easier */
data16_t *system32_mixerregs;		// mixer registers
data16_t *sys32_videoram;
data8_t sys32_ramtile_dirty[0x1000];
extern data16_t *sys32_displayenable;

extern int system32_palMask;
extern int system32_mixerShift;
int system32_draw_bgs;

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
			drawypos = sys32sprite_ypos+ydst; /* no flip */
			if (drawypos > cliprect->max_y) ysrc = sys32sprite_rom_height<<16; // quit drawing if we've gone off the right
		}
		else
		{
			drawypos = sys32sprite_ypos+((sys32sprite_screen_height-1)-ydst); /* y flip */
			if (drawypos < cliprect->min_y) ysrc = sys32sprite_rom_height<<16; // quit drawing if we've gone off the left on a flipped sprite
		}

		if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y)) {
			UINT16 *destline = (UINT16 *)(bitmap->line[drawypos]);

			while ( xsrc < (sys32sprite_rom_width<<16) ) {

				int drawxpos;

				if (!sys32sprite_xflip)
				{
					drawxpos = sys32sprite_xpos+xdst; /* no flip */
					if (drawxpos > cliprect->max_x) xsrc = sys32sprite_rom_width<<16; // quit drawing if we've gone off the right
				}
				else
				{
					drawxpos = sys32sprite_xpos+((sys32sprite_screen_width-1)-xdst); /* x flip */
					if (drawxpos < cliprect->min_x) xsrc = sys32sprite_rom_width<<16; // quit drawing if we've gone off the left on a flipped sprite
				}

				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x)) {

					int gfxdata;

					if (!sys32sprite_8bpp) { // 4bpp

						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+((xsrc>>16)/2)+(ysrc>>16)*(sys32sprite_rom_width/2)]);

						if (xsrc & 0x10000) gfxdata = gfxdata & 0x0f;
						else gfxdata = (gfxdata & 0xf0) >> 4;

						if ( (!sys32sprite_draw_colour_f) && (gfxdata == 0x0f) ) gfxdata = 0;

						if (!sys32sprite_indirect_palette)
						{
							if (gfxdata) destline[drawxpos] =  gfxdata + (sys32sprite_palette * 16);
						}
						else
						{
							if (gfxdata) destline[drawxpos] =  sys32sprite_table[gfxdata] & 0x1fff;;
						}

					} else { // 8bpp

						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+(xsrc>>16)+(ysrc>>16)*(sys32sprite_rom_width)]);

						if ( (!sys32sprite_draw_colour_f) && (gfxdata == 0xff) ) gfxdata = 0;

						/* can 8bpp sprites have indirect palettes? */
						if (gfxdata) destline[drawxpos] =  gfxdata + (sys32sprite_palette * 16);

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
	/* draw data on sprites for debugging */
	{	struct DisplayText dt[2];	char buf[10];
		sprintf(buf, "%04X",(spritedata_source[7]));
		dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
		dt[0].x = sys32sprite_xpos;		dt[0].y = sys32sprite_ypos;
		dt[1].text = 0;
		displaytext(Machine->scrbitmap,dt);		}
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

	h = height to draw on SCREEN + extra attributes
		xxxx---- -------- (0xf000) :  Bits 0-3 of Sprite ROM Bank (TRUSTED)
		----x--- -------- (0x0800) :  unknown
		-----x-- -------- (0x0400) :  unknown
		------x- -------- (0x0200) :  colour 0 = solid?  doesn't seem to hold true in ga2? (it seems to be 0x200 of the screen height, check sonicp when you start the game
		-------x xxxxxxxx (0x01ff) :  Height to draw on screen (TRUSTED)

	w = width to draw on SCREEN + extra attributes *note the extra attributes are different on Multi-System 32, normal are listed*
		x------- -------- (0x8000) :  unknown
		-x------ -------- (0x4000) :  Bit 5 of Sprite ROM Bank (TRUSTED)
		--x----- -------- (0x2000) :  unknown
		---x---- -------- (0x1000) :  unknown
		----x--- -------- (0x0800) :  Bit 4 of Sprite ROM Bank (TRUSTED)
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
	unsigned char *mixer_regs8 = (unsigned char *)system32_mixerregs;

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
//	sys32sprite_solid					= (spritedata_source[2]&0x0200) >> 9;
	sys32sprite_screen_height			= (spritedata_source[2]&0x03ff) >> 0;

	sys32sprite_unknown_3				= (spritedata_source[3]&0x8000) >> 15;
	sys32sprite_rom_bank_high			= (spritedata_source[3]&0x4000) >> 14;
	sys32sprite_unknown_4				= (spritedata_source[3]&0x2000) >> 13;
	sys32sprite_unknown_5				= (spritedata_source[3]&0x1000) >> 12;
	sys32sprite_rom_bank_mid			= (spritedata_source[3]&0x0800) >> 11;
	sys32sprite_screen_width			= (spritedata_source[3]&0x07ff) >> 0;

	sys32sprite_ypos					= (spritedata_source[4]&0xffff) >> 0;

	sys32sprite_xpos					= (spritedata_source[5]&0xffff) >> 0;

	sys32sprite_rom_offset				= (spritedata_source[6]&0xffff) >> 0;

//	sys32sprite_palette					= (spritedata_source[7]&system32_palMask) >> system32_mixerShift; /* direct mode */
	px = spritedata_source[7];
	mixerinput = (px >> (system32_mixerShift + 8)) & 0xf;
	sys32sprite_palette = (px >> 4) & system32_palMask;
	sys32sprite_palette += (mixer_regs8[mixerinput*2] & 0x30)<<2;

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
			sys32sprite_table = sys32_spriteram16 + (spritedata_source[7] & 0x1fff) * 0x8;
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


	/* game specific hack, palette handling must be wrong */
	if (system32_temp_kludge == 1) /* holloseum */
	{
		sys32sprite_palette = (sys32sprite_palette & 0x3f) | 0x40;
		sys32sprite_palette |= ((spritedata_source[7]&0x0800) >> 4);
	}


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
	00 |
	02 |
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


/* background layers will be handled with tilemaps later, this is just a temporary measure for getting a rough idea of what will need to be implemented */

/* yes i know its ugly and sub-optimal ;-) */

void system32_draw_bg_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer )
{
	int x,y;
	int base[4];
	int basetouse = 0;

	int tileno;
	int scrollx, scrolly;
	int drawx, drawy;
	int yflip, xflip;

	base[1] = (sys32_videoram[(0x01FF40+4*layer)/2] & 0x7f00)>>8;
	base[0] = (sys32_videoram[(0x01FF40+4*layer)/2] & 0x007f);
	base[3] = (sys32_videoram[(0x01FF42+4*layer)/2] & 0x7f00)>>8;
	base[2] = (sys32_videoram[(0x01FF42+4*layer)/2] & 0x007f);

	scrollx = (sys32_videoram[(0x01FF12+8*layer)/2]) & 0x3ff;
	scrolly = (sys32_videoram[(0x01FF16+8*layer)/2]) & 0x3ff;

	for (y = 0; y < 64 ; y++)
	{
		drawy = y*16-scrolly;


		for (x = 0; x < 64 ; x++)
		{
			drawx = x*16-scrollx;
			basetouse = 0;
			if (x > 31) basetouse++;
			if (y > 31) basetouse+=2;

			tileno = sys32_videoram[(base[basetouse]*0x200) +   (x & 31) + (y & 31) * 32]; /* mask is .. ? */
			yflip = tileno & 0x8000;
			xflip = tileno & 0x4000;
			tileno &= 0x3fff;

			if ((drawy+16 > 0) && (drawy < 224))
			{
				if ((drawx+16 > 0) && (drawx < 448))
					drawgfx(bitmap,Machine->gfx[0],tileno,0x100,xflip,yflip,drawx,drawy,cliprect,TRANSPARENCY_PEN,0);

				if ((drawx+16+0x400 > 0) && (drawx+0x400 < 448))
					drawgfx(bitmap,Machine->gfx[0],tileno,0x100,xflip,yflip,drawx+0x400,drawy,cliprect,TRANSPARENCY_PEN,0);
			}

			if ((drawy+16+0x400 > 0) && (drawy+0x400 < 224))
			{

			if ((drawx+16 > 0) && (drawx < 448))
				drawgfx(bitmap,Machine->gfx[0],tileno,0x100,xflip,yflip,drawx,drawy+0x400,cliprect,TRANSPARENCY_PEN,0);

			if ((drawx+16+0x400 > 0) && (drawx+0x400 < 448))
				drawgfx(bitmap,Machine->gfx[0],tileno,0x100,xflip,yflip,drawx+0x400,drawy+0x400,cliprect,TRANSPARENCY_PEN,0);
			}
		}

	}
}

void system32_draw_text_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) /* using this for now to save me tilemap system related headaches */
{
	int x,y;
	unsigned char *mixer_regs8 = (unsigned char *)system32_mixerregs;
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

			pal += ((mixer_regs8[0x20]>>4) * 0x40);
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


VIDEO_START( system32 )
{
	sys32_spriteram8 = auto_malloc ( 0x20000 ); // for ram sprites
	sys32_videoram = auto_malloc ( 0x20000 );

//	system32_draw_bgs = 0; /* disable for now until they work, just for development atm */

	return 0;
}


VIDEO_UPDATE( system32 )
{
	fillbitmap(bitmap, 0, 0);

	if (sys32_displayenable[0] & 0x0002) {

		if (system32_draw_bgs)
		{
			system32_draw_bg_layer ( bitmap,cliprect, 3 );
			system32_draw_bg_layer ( bitmap,cliprect, 2 );
			system32_draw_bg_layer ( bitmap,cliprect, 1 );
			system32_draw_bg_layer ( bitmap,cliprect, 0 );
		}

		system32_process_spritelist (bitmap, cliprect);
		system32_draw_text_layer (bitmap, cliprect);

	}
}
