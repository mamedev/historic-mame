/*

STV - VDP1

crude drawing code to support some of the basic vdp1 features
based on what hanagumi columns needs
-- to be expanded

the vdp1 draws to the FRAMEBUFFER which is mapped in memory

*/


#include "driver.h"

data32_t *stv_vdp1_vram;
data32_t *stv_vdp1_regs;
extern data32_t *stv_scu;

/*
Registers:
00
02
04
06
08 EWLR
0a EWRR
0c
0e
10 EDSR
12
*/
/*Erase/Write Upper-Left register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
 0|X1 register      |Y1 register               |
*/
#define STV_VDP1_EWLR ((stv_vdp1_regs[0x008/4] >> 16)&0x0000ffff)
#define STV_VDP1_EWLR_X1 ((STV_VDP1_EWLR & 0x7e00) >> 9)
#define STV_VDP1_EWLR_Y1 ((STV_VDP1_EWLR & 0x01ff) >> 0)
/*Erase/Write Lower-Right register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
X1 register         |Y1 register               |
*/
#define STV_VDP1_EWRR ((stv_vdp1_regs[0x008/4] >> 16)&0x0000ffff)
#define STV_VDP1_EWRR_X3 ((STV_VDP1_EWLR & 0xfe00) >> 9)
#define STV_VDP1_EWRR_Y3 ((STV_VDP1_EWLR & 0x01ff) >> 0)
/*Transfer End Status Register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02| 01| 00|
0                                        |CEF|BEF|
*/
#define STV_VDP1_EDSR ((stv_vdp1_regs[0x010/4] >> 16)&0x0000ffff)
#define STV_VDP1_CEF  (STV_VDP1_EDSR & 2)
#define STV_VDP1_BEF  (STV_VDP1_EDSR & 1)
#define SET_CEF_FROM_1_TO_0 	if(STV_VDP1_CEF)	 stv_vdp1_regs[0x010/4]^=0x00020000
#define SET_CEF_FROM_0_TO_1     if(!(STV_VDP1_CEF))	 stv_vdp1_regs[0x010/4]^=0x00020000
/**/


READ32_HANDLER( stv_vdp1_regs_r )
{
	logerror ("VDP1: Read from Registers, Offset %04x\n",offset);
	return stv_vdp1_regs[offset];
}


int stv_vdp1_start ( void )
{
	stv_vdp1_regs = auto_malloc ( 0x040000 );
	stv_vdp1_vram = auto_malloc ( 0x100000 );

	memset(stv_vdp1_regs, 0, 0x040000);
	memset(stv_vdp1_vram, 0, 0x100000);

	return 0;
}

WRITE32_HANDLER( stv_vdp1_regs_w )
{
	COMBINE_DATA(&stv_vdp1_regs[offset]);
}

READ32_HANDLER ( stv_vdp1_vram_r )
{
	return stv_vdp1_vram[offset];
}


WRITE32_HANDLER ( stv_vdp1_vram_w )
{
	data8_t *vdp1 = memory_region(REGION_GFX2);

	COMBINE_DATA (&stv_vdp1_vram[offset]);

	data = stv_vdp1_vram[offset];
	/* put in gfx region for easy decoding */
	vdp1[offset*4+0] = (data & 0xff000000) >> 24;
	vdp1[offset*4+1] = (data & 0x00ff0000) >> 16;
	vdp1[offset*4+2] = (data & 0x0000ff00) >> 8;
	vdp1[offset*4+3] = (data & 0x000000ff) >> 0;
}

/*

there is a command every 0x20 bytes
the first word is the control word
the rest are data used by it

---
00 CMDCTRL
   e--- ---- ---- ---- | end bit (15)
   -jjj ---- ---- ---- | jump select bits (12-14)
   ---- zzzz ---- ---- | zoom point / hotspot (8-11)
   ---- ---- 00-- ---- | UNUSED
   ---- ---- --dd ---- | character read direction (4,5)
   ---- ---- ---- cccc | command bits (0-3)

02 CMDLINK
   llll llll llll ll-- | link
   ---- ---- ---- --00 | UNUSED

04 CMDPMOD
   m--- ---- ---- ---- | MON (looks at MSB and apply shadows etc.)
   -00- ---- ---- ---- | UNUSED
   ---h ---- ---- ---- | HSS (High Speed Shrink)
   ---- p--- ---- ---- | PCLIP (Pre Clipping Disable)
   ---- -c-- ---- ---- | CLIP (Clipping Mode Bit)
   ---- --m- ---- ---- | CMOD (User Clipping Enable Bit)
   ---- ---M ---- ---- | MESH (Mesh Enable Bit)
   ---- ---- e--- ---- | ECD (End Code Disable)
   ---- ---- -S-- ---- | SPD (Transparent Pixel Disable)
   ---- ---- --cc c--- | Colour Mode
   ---- ---- ---- -CCC | Colour Calculation bits

06 CMDCOLR
   mmmm mmmm mmmm mmmm | Colour Bank, Colour Lookup /8

08 CMDSRCA (Character Address)
   aaaa aaaa aaaa aa-- | Character Address
   ---- ---- ---- --00 | UNUSED

0a CMDSIZE (Character Size)
   00-- ---- ---- ---- | UNUSED
   --xx xxxx ---- ---- | Character Size (X)
   ---- ---- yyyy yyyy | Character Size (Y)

0c CMDXA (used for normal sprite)
   eeee ee-- ---- ---- | extension bits
   ---- --xx xxxx xxxx | x position

0e CMDYA (used for normal sprite)
   eeee ee-- ---- ---- | extension bits
   ---- --yy yyyy yyyy | y position

10 CMDXB
12 CMDYB
14 CMDXC
16 CMDYC
18 CMDXD
1a CMDYD
1c CMDGRDA (Gouraud Shading Table)
1e UNUSED
---


*/

int vdp1_sprite_log = 0;

static struct stv_vdp2_sprite_list
{

	int CMDCTRL, CMDLINK, CMDPMOD, CMDCOLR, CMDSRCA, CMDSIZE, CMDGRDA;
	int CMDXA, CMDYA;
	int CMDXB, CMDYB;
	int CMDXC, CMDYC;
	int CMDXD, CMDYD;

} stv2_current_sprite;

/* we should actually draw to the framebuffer then process that with vdp.. note that if we're drawing
to the framebuffer we CAN'T frameskip the vdp1 drawing as the hardware can READ the framebuffer
and if we skip the drawing the content could be incorrect when it reads it, although i have no idea
why they would want to */

extern data32_t* stv_vdp2_cram;

void stv_vpd1_draw_normal_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 *destline;

	int pix,mode,transmask;
	int y, ysize, ycnt, drawypos;
	int x, xsize, xcnt, drawxpos;
	int direction, zoompoint;
	int patterndata;
	data8_t* gfxdata = memory_region(REGION_GFX2);

	x = (stv2_current_sprite.CMDXA & 0x03ff);
	y = (stv2_current_sprite.CMDYA & 0x03ff);

	/* sign extend */
	if (x & 0x200) x-=0x400;
	if (y & 0x200) y-=0x400;

	/* shift a bit ..*/
	x+= 128+32;
	y+= 128-16;

	direction = (stv2_current_sprite.CMDCTRL & 0x0030)>>4;

	xsize = (stv2_current_sprite.CMDSIZE & 0x3f00) >> 8;
	xsize = xsize * 8;

	ysize = (stv2_current_sprite.CMDSIZE & 0x00ff);

	zoompoint = (stv2_current_sprite.CMDCTRL & 0x0f00)>>8;

	/* NOTE we should only process zoompoint for SCALED sprites, remove this later or make the function recognise which are scaled and which are normal .. */
	switch (zoompoint)
	{
		case 0x0: // specified co-ordinates
			break;
		case 0x5: // up left
			break;
		case 0x6: // up center
			x -= xsize/2;
			break;
		case 0x7: // up right
			x -= xsize;
			break;

		case 0x9: // center left
			y -= ysize/2;
			break;
		case 0xa: // center center
			y -= ysize/2;
			x -= xsize/2;

			break;
		case 0xb: // center right
			y -= ysize/2;
			x -= xsize;
			break;

		case 0xd: // center left
			y -= ysize;
			break;
		case 0xe: // center center
			y -= ysize;
			x -= xsize/2;
			break;
		case 0xf: // center right
			y -= ysize;
			x -= xsize;
			break;

		default: // illegal
			break;
	}

	patterndata = (stv2_current_sprite.CMDSRCA >> 2) & 0x3fff;
	patterndata = patterndata * 0x20;

	if (vdp1_sprite_log) logerror ("Drawing Normal Sprite x %04x y %04x xsize %04x ysize %04x patterndata %06x\n",x,y,xsize,ysize,patterndata);


	for (ycnt = 0; ycnt != ysize; ycnt++) {


		if (direction & 0x2) // 'yflip' (reverse direction)
		{
			drawypos = y+((ysize-1)-ycnt);
		}
		else
		{
			drawypos = y+ycnt;
		}

		if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y))
		{
			destline = (UINT16 *)(bitmap->line[drawypos]);

			for (xcnt = 0; xcnt != xsize; xcnt ++)
			{
				int offsetcnt;

				offsetcnt = ycnt*xsize+xcnt;
				if (direction & 0x1) // 'xflip' (reverse direction)
				{
					drawxpos = x+((xsize-1)-xcnt);
				}
				else
				{
					drawxpos = x+xcnt;
				}

				switch (stv2_current_sprite.CMDPMOD&0x0038)
				{
					case 0x0000: // mode 0 16 colour bank mode (4bits) (hanagumi blocks)
						pix = gfxdata[patterndata+offsetcnt/2];
						pix = offsetcnt&1 ? (pix & 0x0f):((pix & 0xf0)>>4) ;
						pix = pix+(stv2_current_sprite.CMDCOLR&0x0ff0);
						mode = 0;
						transmask = 0xf;
						break;
					case 0x0008: // mode 1 16 colour lookup table mode (4bits)
						pix = gfxdata[patterndata+offsetcnt/2];
						pix = offsetcnt&1 ?  (pix & 0x0f):((pix & 0xf0)>>4);
						mode = 1;
						transmask = 0x0f;
						break;
					case 0x0010: // mode 2 64 colour bank mode (8bits) (character select portraits on hanagumi)
						pix = gfxdata[patterndata+offsetcnt];
						mode = 2;
						pix = pix+(stv2_current_sprite.CMDCOLR&0x0fc0);
						transmask = 0x3f;
						break;
					case 0x0018: // mode 3 128 colour bank mode (8bits) (little characters on hanagumi use this mode)
						pix = gfxdata[patterndata+offsetcnt];
						pix = pix+(stv2_current_sprite.CMDCOLR&0x0f80);
						transmask = 0x7f;
						mode = 3;
					//	pix = rand();
						break;
					case 0x0020: // mode 4 256 colour bank mode (8bits) (hanagumi title)
						pix = gfxdata[patterndata+offsetcnt];
						pix = pix+(stv2_current_sprite.CMDCOLR&0x0f00);
						transmask = 0xff;
						mode = 4;
						break;
					case 0x0028: // mode 5 32,768 colour RGB mode (16bits)
						pix = gfxdata[patterndata+offsetcnt*2+1] | (gfxdata[patterndata+offsetcnt*2]<<8) ;
						mode = 5;
						transmask = 0x7fff;
						break;
					default: // other settings illegal
						pix = rand();
						mode = 0;
						transmask = 0xff;
				}


				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x))
				{
					if (mode != 5) // mode 0-4 are 'normal'
					{
						if (pix & transmask)
						{
							/* there is probably a better way to do this .. it will probably have to change anyway because we'll be writing to the framebufferi instead */
							int col;
							col = (pix&1)? ((stv_vdp2_cram[(pix&0x0ffe)/2] & 0x00007fff) >>0): ((stv_vdp2_cram[(pix&0x0ffe)/2] & 0x7fff0000) >>16);
							col = ((col & 0x001f)*0x400) + (col & 0x03e0) + ((col & 0x7c00)/0x400);
							destline[drawxpos] = col;
						}
					}
					else // mode 5 is rgb mode
					{
						int col;
						col = pix;
						col = ((col & 0x001f)*0x400) + (col & 0x03e0) + ((col & 0x7c00)/0x400);
						destline[drawxpos] = col & 0x7fff;
					}

				} // drawxpos

			} // xcnt

		} // if drawypos

	} // ycny

}

void stv_vdp1_process_list(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int position;
	int spritecount;

	spritecount = 0;
	position = 0;

	if (vdp1_sprite_log) logerror ("Sprite List Process START\n");

	int vdp1_nest;

	vdp1_nest = -1;

	/*Set CEF bit to 0*/
	SET_CEF_FROM_1_TO_0;

	while (spritecount<10000) // if its drawn this many sprites something is probably wrong or sega were crazy ;-)
	{
		int draw_this_sprite;

		draw_this_sprite = 1;

	//	if (position >= ((0x80000/0x20)/4)) // safety check
	//	{
	//		if (vdp1_sprite_log) logerror ("Sprite List Position Too High!\n");
	//		position = 0;
	//	}

		stv2_current_sprite.CMDCTRL = (stv_vdp1_vram[position * (0x20/4)+0] & 0xffff0000) >> 16;

		if (stv2_current_sprite.CMDCTRL == 0x8000)
		{
			if (vdp1_sprite_log) logerror ("List Terminator (0x8000) Encountered, Sprite List Process END\n");
			goto end; // end of list
		}

		stv2_current_sprite.CMDLINK = (stv_vdp1_vram[position * (0x20/4)+0] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDPMOD = (stv_vdp1_vram[position * (0x20/4)+1] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDCOLR = (stv_vdp1_vram[position * (0x20/4)+1] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDSRCA = (stv_vdp1_vram[position * (0x20/4)+2] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDSIZE = (stv_vdp1_vram[position * (0x20/4)+2] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXA   = (stv_vdp1_vram[position * (0x20/4)+3] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYA   = (stv_vdp1_vram[position * (0x20/4)+3] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXB   = (stv_vdp1_vram[position * (0x20/4)+4] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYB   = (stv_vdp1_vram[position * (0x20/4)+4] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXC   = (stv_vdp1_vram[position * (0x20/4)+5] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYC   = (stv_vdp1_vram[position * (0x20/4)+5] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXD   = (stv_vdp1_vram[position * (0x20/4)+6] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYD   = (stv_vdp1_vram[position * (0x20/4)+6] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDGRDA = (stv_vdp1_vram[position * (0x20/4)+7] & 0xffff0000) >> 16;
//		stv2_current_sprite.UNUSED  = (stv_vdp1_vram[position * (0x20/4)+7] & 0x0000ffff) >> 0;

		/* proecess jump / skip commands, set position for next sprite */
		switch (stv2_current_sprite.CMDCTRL & 0x7000)
		{
			case 0x0000: // jump next
				if (vdp1_sprite_log) logerror ("Sprite List Process + Next (Normal)\n");
				position++;
				break;
			case 0x1000: // jump assign
				if (vdp1_sprite_log) logerror ("Sprite List Process + Jump Old %06x New %06x\n", position, (stv2_current_sprite.CMDLINK>>2));
				position= (stv2_current_sprite.CMDLINK>>2);
				break;
			case 0x2000: // jump call
				if (vdp1_nest == -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Process + Call Old %06x New %06x\n",position, (stv2_current_sprite.CMDLINK>>2));
					vdp1_nest = position+1;
					position = (stv2_current_sprite.CMDLINK>>2);
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Sprite List Nested Call, ignoring\n");
					position++;
				}
				break;
			case 0x3000:
				if (vdp1_nest != -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Process + Return\n");
					position = vdp1_nest;
					vdp1_nest = -1;
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Attempted return from no subroutine, ignoring\n");
					position++;
				}
				break;
			case 0x4000:
				draw_this_sprite = 0;
				position++;
				break;
			case 0x5000:
				if (vdp1_sprite_log) logerror ("Sprite List Skip + Jump Old %06x New %06x\n", position, (stv2_current_sprite.CMDLINK>>2));
				draw_this_sprite = 0;
				position= (stv2_current_sprite.CMDLINK>>2);

				break;
			case 0x6000:
				draw_this_sprite = 0;
				if (vdp1_nest == -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Skip + Call To Subroutine Old %06x New %06x\n",position, (stv2_current_sprite.CMDLINK>>2));

					vdp1_nest = position+1;
					position = (stv2_current_sprite.CMDLINK>>2);
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Sprite List Nested Call, ignoring\n");
					position++;
				}
				break;
			case 0x7000:
				draw_this_sprite = 0;
				if (vdp1_nest != -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Skip + Return from Subroutine\n");

					position = vdp1_nest;
					vdp1_nest = -1;
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Attempted return from no subroutine, ignoring\n");
					position++;
				}
				break;
		}

		/* continue to draw this sprite only if the command wasn't to skip it */
		if (draw_this_sprite ==1)
		{
			switch (stv2_current_sprite.CMDCTRL & 0x000f)
			{
				case 0x0000:
					if (vdp1_sprite_log) logerror ("Sprite List Normal Sprite\n");
					stv_vpd1_draw_normal_sprite(bitmap,cliprect);
					break;

				case 0x0001:
					if (vdp1_sprite_log) logerror ("Sprite List Scaled Sprite\n");
					stv_vpd1_draw_normal_sprite(bitmap,cliprect);
					break;

				case 0x0002:
					if (vdp1_sprite_log) logerror ("Sprite List Distorted Sprite\n");
					stv_vpd1_draw_normal_sprite(bitmap,cliprect);
					break;

				case 0x0004:
					if (vdp1_sprite_log) logerror ("Sprite List Polygon and doesn't want a cracker anymore\n");
					break;

				case 0x0005:
					if (vdp1_sprite_log) logerror ("Sprite List Polyline\n");
					break;

				case 0x0006:
					if (vdp1_sprite_log) logerror ("Sprite List Line\n");
					break;

				case 0x0008:
					if (vdp1_sprite_log) logerror ("Sprite List Set Command for User Clipping\n");
					break;

				case 0x0009:
					if (vdp1_sprite_log) logerror ("Sprite List Set Command for System Clipping\n");
					break;

				case 0x000a:
					if (vdp1_sprite_log) logerror ("Sprite List Local Co-Ordinate Set\n");
					break;

				default:
					if (vdp1_sprite_log) logerror ("Sprite List Illegal!\n");
					break;


			}


		}

		spritecount++;

	}


	end:
	/*set CEF to 1*/
	SET_CEF_FROM_0_TO_1;

	if(!(stv_scu[40] & 0x2000)) /*Sprite draw end irq*/
		cpu_set_irq_line_and_vector(0, 2, HOLD_LINE , 0x4d);

	if (vdp1_sprite_log) logerror ("End of list processing!\n");
}

void video_update_vdp1(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
//	int enable;
//	if (keyboard_pressed (KEYCODE_R)) vdp1_sprite_log = 1;
//	if (keyboard_pressed (KEYCODE_T)) vdp1_sprite_log = 0;

//	if (keyboard_pressed (KEYCODE_Y)) vdp1_sprite_log = 0;
//	{
//		FILE *fp;
//
//		fp=fopen("vdp1_ram.dmp", "w+b");
//		if (fp)
//		{
//			fwrite(stv_vdp1, 0x00100000, 1, fp);
//			fclose(fp);
//		}
//	}
	stv_vdp1_process_list(bitmap,cliprect);
}
