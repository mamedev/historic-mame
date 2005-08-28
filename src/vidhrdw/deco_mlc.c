/*
    The MLC graphics hardware is quite complicated - the usual method of having 'object ram' that
    controls sprites is expanded into object ram that controls sprite blocks that may be stored
    in RAM or ROM.  Each tile in a block may be specified explicitly via a display list in ROM or
    calculated as part of a block offset.

    Blocks can be scaled and subpositioned, and are usually 4bpp but blocks can be combined
    into 8bpp with a flag.
*/

#include "driver.h"
#include "vidhrdw/generic.h"

data32_t *avengrgs_vram;

/******************************************************************************/

VIDEO_START( avengrgs )
{
	return 0;
}

static void mlc_drawgfxzoom( struct mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code1,unsigned int code2, unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int use8bpp,
		int scalex, int scaley)
{
	struct rectangle myclip;

	if (!scalex || !scaley) return;

	/*
    scalex and scaley are 16.16 fixed point numbers
    1<<15 : shrink to 50%
    1<<16 : uniform scale
    1<<17 : double to 200%
    */

	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}

	{
		if( gfx && gfx->colortable )
		{
			const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
			int source_base1 = (code1 % gfx->total_elements) * gfx->height;
			int source_base2 = (code2 % gfx->total_elements) * gfx->height;

			int sprite_screen_height = (scaley*gfx->height+(sy&0xffff))>>16;
			int sprite_screen_width = (scalex*gfx->width+(sx&0xffff))>>16;

			sx>>=16;
			sy>>=16;

			if (sprite_screen_width && sprite_screen_height)
			{
				/* compute sprite increment per screen pixel */
				int dx = (gfx->width<<16)/sprite_screen_width;
				int dy = (gfx->height<<16)/sprite_screen_height;

				int ex = sx+sprite_screen_width;
				int ey = sy+sprite_screen_height;

				int x_index_base;
				int y_index;

				if( flipx )
				{
					x_index_base = (sprite_screen_width-1)*dx;
					dx = -dx;
				}
				else
				{
					x_index_base = 0;
				}

				if( flipy )
				{
					y_index = (sprite_screen_height-1)*dy;
					dy = -dy;
				}
				else
				{
					y_index = 0;
				}

				if( clip )
				{
					if( sx < clip->min_x)
					{ /* clip left */
						int pixels = clip->min_x-sx;
						sx += pixels;
						x_index_base += pixels*dx;
					}
					if( sy < clip->min_y )
					{ /* clip top */
						int pixels = clip->min_y-sy;
						sy += pixels;
						y_index += pixels*dy;
					}
					/* NS 980211 - fixed incorrect clipping */
					if( ex > clip->max_x+1 )
					{ /* clip right */
						int pixels = ex-clip->max_x-1;
						ex -= pixels;
					}
					if( ey > clip->max_y+1 )
					{ /* clip bottom */
						int pixels = ey-clip->max_y-1;
						ey -= pixels;
					}
				}

				if( ex>sx )
				{ /* skip if inner loop doesn't draw anything */
					int y;

					/* case 1: TRANSPARENCY_PEN */
					if (transparency == TRANSPARENCY_PEN)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source1 = gfx->gfxdata + (source_base1+(y_index>>16)) * gfx->line_modulo;
								UINT8 *source2 = gfx->gfxdata + (source_base2+(y_index>>16)) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source1[x_index>>16];
									if (use8bpp)
										c=(c<<4)|source2[x_index>>16];

									if( c != transparent_color ) dest[x] = pal[c];
									x_index += dx;
								}

								y_index += dy;
							}
						}
					}

					/* case 6: TRANSPARENCY_ALPHA */
					if (transparency == TRANSPARENCY_ALPHA)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = gfx->gfxdata + (source_base1+(y_index>>16)) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source[x_index>>16];
									if( c != transparent_color ) dest[x] = alpha_blend32(dest[x], pal[c]);
									x_index += dx;
								}

								y_index += dy;
							}
						}
					}
				}
			}
		}
	}
}

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	data32_t *index_ptr;
	int offs,fx=0,fy=0,x,y,color,sprite,indx,h,w,bx,by;
	int xmult,ymult,xoffs,yoffs;
	data8_t *rom = memory_region(REGION_GFX4) + 0x20000, *index_ptr8;
	data8_t *rawrom = memory_region(REGION_GFX4); //fix above
	int blockIsTilemapIndex=0;
	int sprite2=0,indx2=0,use8bppMode=0;
	int yscale,xscale;
	int ybase,yinc;

//  for (offs = 0; offs<0x3000/4; offs+=8)
	for (offs = (0x3000/4)-8; offs>=0x100; offs-=8) // TEST - REMOVE TOP ENTRIES!
	{
		if ((spriteram32[offs+0]&0x8000)==0) {
			//memset((data8_t*)spriteram32,0,8*4);
			continue; //check
		}

		y = spriteram32[offs+2]&0x7ff;
		x = spriteram32[offs+3]&0x7ff; /* Bit 0100 0000 sometimes set?? sh2 bug? */

		if (x&0x400) x=-(0x400-(x&0x3ff));
		if (y&0x400) y=-(0x400-(y&0x3ff));

		fx = spriteram32[offs+1]&0x8000;
		fy = spriteram32[offs+1]&0x4000;
//      fx = spriteram32[offs+2]&0x01000000; // Alpha blend??
//      fy = spriteram32[offs+3]&0x01000000; // Alpha blend??
		color = spriteram32[offs+1]&0x7f;
		indx = spriteram32[offs+0]&0x3fff;
		yscale = spriteram32[offs+4]&0x1ff;
		xscale = spriteram32[offs+5]&0x1ff;

		/* If this bit is set, combine this block with the next one */
		if (spriteram32[offs+1]&0x1000) {
			use8bppMode=1;
			/* In 8bpp the palette base is stored in the next block */
			if (offs-8>=0) {
				color = (spriteram32[offs+1-8]&0x7f);// | 0x80;
				indx2 = spriteram32[offs+0-8]&0x3fff;
			}
		} else
			use8bppMode=0;

		/* Lookup tiles/size in sprite index ram OR in the lookup rom */
		if (spriteram32[offs+0]&0x4000) {
			index_ptr8=rom + indx*8; /* Byte ptr */
			h=(index_ptr8[1]>>0)&0xf;
			w=(index_ptr8[3]>>0)&0xf;

			if (!h) h=16;
			if (!w) w=16; //test above in sprites too

			sprite = (index_ptr8[7]<<8)|index_ptr8[6];
//          bank = index_ptr8[4]&3;
			sprite |= (index_ptr8[4]&3)<<16;

			if (use8bppMode) {
				data8_t* index_ptr28=rom + indx2*8;
				sprite2=(index_ptr28[7]<<8)|index_ptr28[6];
			//  fx=spriteram32[offs+1]&0x10;
			}
			//unused byte 5
			yoffs=index_ptr8[0]&0xff;
			xoffs=index_ptr8[2]&0xff;
			if (index_ptr8[4]&0x40)
				blockIsTilemapIndex=1;
			else
				blockIsTilemapIndex=0;
		} else {
			indx&=0x1fff;
			index_ptr=avengrgs_vram + indx*4;
			h=(index_ptr[0]>>8)&0xf;
			w=(index_ptr[1]>>8)&0xf;

			if (!h) h=16;
			if (!w) w=16; //test above in sprites too

			if (use8bppMode) {
				data32_t* index_ptr2=avengrgs_vram + indx2*4;
				sprite2=((index_ptr2[2]&0x3)<<16) | (index_ptr2[3]&0xffff);
			}

			sprite = ((index_ptr[2]&0x3)<<16) | (index_ptr[3]&0xffff);
			if (index_ptr[2]&0x40)
				blockIsTilemapIndex=1;
			else
				blockIsTilemapIndex=0;

			yoffs=index_ptr[0]&0xff;
			xoffs=index_ptr[1]&0xff;
		}

		if (fx) x+=xoffs-16; else x-=xoffs; //check for signed offsets...
		if (fy) y+=yoffs-16; else y-=yoffs; //check for signed offsets...

		if (fx) xmult=-1; else xmult=1;
		if (fy) ymult=-1; else ymult=1;

		ybase=y<<16;
		yinc=(yscale<<8)*16;

		for (by=0; by<h; by++) {
			int xbase=x<<16;
			int xinc=(xscale<<8)*16;

			if (fx) xinc=-xinc;

			for (bx=0; bx<w; bx++) {
				int tile=sprite;
				int tile2=sprite2;
				int bank=sprite>>16;

				if (blockIsTilemapIndex) {
					const data8_t* ptr=rawrom+(sprite*2);
					tile=(*ptr) + ((*(ptr+1))<<8);
					bank=0;

					if (use8bppMode) {
						const data8_t* ptr2=rawrom+(sprite2*2);
						tile2=(*ptr2) + ((*(ptr2+1))<<8);
					}
				}

				mlc_drawgfxzoom(bitmap,Machine->gfx[bank],
							tile,tile2,
							color,fx,fy,xbase,ybase,
							cliprect,TRANSPARENCY_PEN,0,use8bppMode,(xscale<<8),(yscale<<8));

				sprite++;
				sprite2++;

				xbase+=xinc;
			}
			ybase+=yinc;
		}

		if (use8bppMode)
			offs-=8;
	}
}


/*

    0100 0000 bug...

  4964 - calls function
    6a6c - which sets 3c@SP

    6bf8 - uses 3c@SP

    SHLR8 at 6a4e creates 0100 0000


    other:

    6a90 - unaligned sp access??  72@sp
    6a92 - puts $30 into r0 for sp access and calculates flip


    6b18 - alters r13 - ORs in flip information - uses 30@sp
    6b66 - sets up r13 (palette)
    6d14 - extu r13 into r5, then call function below
    463a - moves R5 into spriteram (contains flip X part for CA)

*/

VIDEO_UPDATE( avengrgs )
{
	int mx,my;
	data32_t *vram_ptr=avengrgs_vram + (0x1dc00/4);

#if 0
//  data8_t *rom = memory_region(REGION_GFX4);

//  static int bank=0;
//  static int base=0x40000;
//  int o=0;

//  if (code_pressed_memory(KEYCODE_X))
//      base+=0x200;
//  if (code_pressed_memory(KEYCODE_Z))
//      base-=0x200;

// 22a65c0 == linescroll

	ui_popup("%08x",base);

	for (my=0; my<16; my++) {
		for (mx=0; mx<16; mx++) {
			int t=rom[base+o]|(rom[base+1+o]<<8);
			drawgfx(bitmap,Machine->gfx[bank],t,5,0,0,mx*16,my*16,0,TRANSPARENCY_PEN,0);
			o+=2;
		}
	}
#endif

	fillbitmap(bitmap,get_black_pen(),cliprect);
	draw_sprites(bitmap,cliprect);

	for (my=0; my<32; my++) {
		for (mx=0; mx<64; mx++) {
			int indx=0;

			if (mx<16)
				indx=mx + (my&0x1f)*0x10;
			else if (mx<32)
				indx=(mx-16) + (my&0x1f)*0x10 + 0x200;
			else
				indx=(mx-32) + (my&0x1f)*0x10 + 0x400;

			if ((vram_ptr[indx])&0xff)
				drawgfx(bitmap,Machine->gfx[3],(vram_ptr[indx])&0xff,15-((vram_ptr[indx]>>12)&0xf),0,0,mx*8,my*8,0,TRANSPARENCY_PEN,0);
		}
	}
}

VIDEO_STOP(avengrgs)
{
#if 0
	FILE *fp;
	int i;
	char a[4];

	fp=fopen("vram.dmp","wb");
	for (i=0; i<0x20000/4; i++) {
		a[0]=avengrgs_vram[i]&0xff;
		a[1]=avengrgs_vram[i]>>8;
		fwrite(a,0x2,1,fp);
	}
	fclose(fp);

	fp=fopen("vram2.dmp","wb");
	fwrite(avengrgs_ram2,0x4000,1,fp);
	fclose(fp);
#endif
}
