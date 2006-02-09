#include "vidhrdw/generic.h"

/*defined in drivers/darkmist.c */

extern unsigned char * darkmist_scroll;
extern int darkmist_hw;

#define DM_HACKS 1


#if DM_HACKS
static int palc=-1;
#endif

/* vis. flags */

#define DISPLAY_SPR		1
#define DISPLAY_FG		2 /* 2 or 8 */
#define DISPLAY_BG		4
#define DISPLAY_TXT		16

static tilemap *bgtilemap, *fgtilemap, *txtilemap;

static void get_bgtile_info(int tile_index)
{
	int code,attr,pal;

	code=memory_region(REGION_USER1)[tile_index]; /* TTTTTTTT */
	attr=memory_region(REGION_USER2)[tile_index]; /* -PPP--TT - FIXED BITS (0xxx00xx) */
	code+=(attr&3)<<8;
	pal=(attr>>4);

#if DM_HACKS
	pal+=1;
#endif

	SET_TILE_INFO(
		1,
        code,
        pal,
        0)
}

static void get_fgtile_info(int tile_index)
{
	int code,attr,pal;

	code=memory_region(REGION_USER3)[tile_index]; /* TTTTTTTT */
	attr=memory_region(REGION_USER4)[tile_index]; /* -PPP--TT - FIXED BITS (0xxx00xx) */
	pal=attr>>4;

	code+=(attr&3)<<8;

	code+=0x400;

#if DM_HACKS
	if(pal==palc)
		pal=rand()&0xf;
#endif

	SET_TILE_INFO(
		1,
        code,
        pal,
        0)
}

static void get_txttile_info(int tile_index)
{
	int code,attr,pal;

	code=videoram[tile_index];
	attr=videoram[tile_index+0x400];
	pal=attr&0xf;

	code+=(attr&1)<<8;


	SET_TILE_INFO(
		0,
        code,
        pal,
        0)
}

READ8_HANDLER(darkmist_palette_r)
{
	return paletteram[offset];
}

WRITE8_HANDLER(darkmist_palette_w)
{
	int r,g,b;
	paletteram[offset]=data;
	offset&=0x1ff;
	r=paletteram[offset+0x200]&0xf;
	g=(paletteram[offset])>>4;
	b=paletteram[offset]&0x0f;

	palette_set_color(offset, r * 0x11, g * 0x11, b * 0x11);
}

VIDEO_START(darkmist)
{
	bgtilemap = tilemap_create( get_bgtile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,512,64 );
	fgtilemap = tilemap_create( get_fgtile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,256 );
	txtilemap = tilemap_create( get_txttile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32 );
	tilemap_set_transparent_pen(fgtilemap, 0);
	tilemap_set_transparent_pen(txtilemap, 0);

	return 0;
}

VIDEO_UPDATE( darkmist)
{


#if DM_HACKS
	if(code_pressed_memory(KEYCODE_Q))
	{
		palc++;
		printf("%x \n",palc);
	}

	if(code_pressed_memory(KEYCODE_W))
	{
		palc--;
		printf("%x \n",palc);
	}
	tilemap_mark_all_tiles_dirty(fgtilemap);
#endif



#define DM_GETSCROLL(n) (((darkmist_scroll[(n)]<<1)&0xff) + ((darkmist_scroll[(n)]&0x80)?1:0) +( ((darkmist_scroll[(n)-1]<<4) | (darkmist_scroll[(n)-1]<<12) )&0xff00))

	tilemap_set_scrollx(bgtilemap, 0, DM_GETSCROLL(0x2));
	tilemap_set_scrolly(bgtilemap, 0, DM_GETSCROLL(0x6));
	tilemap_set_scrollx(fgtilemap, 0, DM_GETSCROLL(0xa));
	tilemap_set_scrolly(fgtilemap, 0, DM_GETSCROLL(0xe));

	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	if(darkmist_hw & DISPLAY_BG)
	{
		tilemap_draw(bitmap,cliprect,bgtilemap, 0,0);
	}


	if(darkmist_hw & DISPLAY_FG)
	{
		tilemap_draw(bitmap,cliprect,fgtilemap, 0,0);
	}

	if(darkmist_hw & DISPLAY_SPR)
	{
/*
    Sprites

    76543210
0 - TTTTTTTT - tile
1 - xy0PPP21 - palette (P), flips (x,y), top bits of tile number (0,1,2)
2 - YYYYYYYY - y coord
3 - XXXXXXXX - x coord

*/

	int i,fx,fy,tile,palette;
	for(i=0;i<spriteram_size;i+=32)
	{
		fy=spriteram[i+1]&0x40;
		fx=spriteram[i+1]&0x80; /* guess ....*/
		tile=(((spriteram[i+1]&3)<<9)|((spriteram[i+1]&0x20)<<3));

#if DM_HACKS
		if(tile==0x4000)
		{
			tile=0x3000;
		}
#endif

		tile+=spriteram[i+0];
		palette=(spriteram[i+1]>>2)&0x7; /* guess  ... */

#if DM_HACKS
		palette+=1;
#endif

		drawgfx(
               bitmap,Machine->gfx[2],
               tile,
               palette,
               fx,fy,
               spriteram[i+3],spriteram[i+2],
               &Machine->visible_area,
               TRANSPARENCY_PEN,0 );
		}
	}

	if(darkmist_hw & DISPLAY_TXT)
	{
		tilemap_mark_all_tiles_dirty(txtilemap);
		tilemap_draw(bitmap,cliprect,txtilemap, 0,0);
	}
}


