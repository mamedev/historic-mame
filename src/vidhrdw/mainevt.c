/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"

static struct osd_bitmap *back_bitmap;
static struct osd_bitmap *front_bitmap;
static char *dirty_f,*dirty_b;
unsigned char *mainevt_attr_ram;
static int dirty_video;



void mainevt_video_w(int offset, int data)
{
	if (videoram[offset]==data)
		return;
	videoram[offset]=data;
	dirty_video=1;

	/* Dirty correct layer */
	if (offset>0xfff) dirty_f[offset-0x1000]=1;
	else if (offset>0x7ff) dirty_b[offset-0x800]=1;
}

void mainevt_attr_w(int offset, int data)
{
	if (mainevt_attr_ram[offset]==data)
  		return;
	mainevt_attr_ram[offset]=data;
	dirty_video=1;

	/* Dirty correct layer */
	if (offset>0xfff) dirty_f[offset-0x1000]=1;
	else if (offset>0x7ff) dirty_b[offset-0x800]=1;
}

void mainevt_control_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	switch (offset) {
  	case 0: /* Rom Bank Switch */
		bankaddress = 0x10000 + (data & 0x03) * 0x2000;
		cpu_setbank(1,&RAM[bankaddress]);
		return;
    case 4:
		/* Sound? */
		soundlatch_w(0,data);
//		cpu_cause_interrupt(1,Z80_NMI_INT);
		return;
    case 8:
		/* Sound? Usually same value as above */
		soundlatch_w(0,data);
//		cpu_cause_interrupt(1,Z80_NMI_INT);
		return;
    case 12:
		/* Voice? */
	//  soundlatch_w(0,data);
	//  cpu_cause_interrupt(1,Z80_NMI_INT);
		return;
    case 16:
		/* Voice? */
		soundlatch_w(0,data);
//		cpu_cause_interrupt(1,Z80_NMI_INT);
		return;
	}
	if (errorlog) fprintf(errorlog,"Control %d %02x\n",offset,data);
}

/*****************************************************************************/

int mainevt_vh_start(void)
{
	if ((back_bitmap = osd_create_bitmap(64*8,32*8)) == 0)
		return 1;
 	if ((front_bitmap = osd_create_bitmap(64*8,32*8)) == 0)
		return 1;

	dirty_b=(char *)malloc(0x800);
	dirty_f=(char *)malloc(0x800);
	memset(dirty_b,1,0x800);
	memset(dirty_f,1,0x800);

	return 0;
}

void mainevt_vh_stop(void)
{
	osd_free_bitmap(back_bitmap);
	osd_free_bitmap(front_bitmap);
	free(dirty_b);
	free(dirty_f);
}

/*****************************************************************************/

/* Draw sprites on given bitmap, if 'behind' specified then sprites are known
to be behind 2nd playfield */
void mainevt_vh_drawsprites(struct osd_bitmap *bitmap, int behind)
{
	int offs;

	/* Sprites */
	for (offs = 0x400-8;offs >=0 ;offs -= 8) {

  	/* Tables from Punkshot driver, I don't think more than 2 by 2 is used */
 		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
		static int width[8] =  { 1, 2, 1, 2, 4, 2, 4, 8 };
		static int height[8] = { 1, 1, 2, 2, 2, 4, 4, 8 };
		int size,w,h,x,y,tile,color,mx,my,fx,fy;

		/* Sprite display enable */
		if (!(spriteram[offs]&0x80)) continue;

		/* If drawing to background and this in front of playfield continue */
		if (behind && (spriteram[offs+3]&0x60)) continue;

 		tile=((spriteram[offs+1]<<8)+spriteram[offs+2])&0x1fff;

		/* And if drawing to front and this is behind continue */
		if ((!behind && ((spriteram[offs+3]&0x60)==0x00))
			&& tile!=0x3f8 && tile!=0x3f9
		) continue;

		/* Inevitable kludge for now...  Fix priority on ropes (3F8 & 3F9) */

		/* bit 0x40 is priority over playfield 2, bit 0x20???? */

		size = (spriteram[offs+1] & 0xe0) >> 5;
		w = width[size];
		h = height[size];

		mx = ((spriteram[offs+6]<<8)+spriteram[offs+7]) & 0x01ff;
		my = 256 - (((spriteram[offs+4]<<8)+spriteram[offs+5]) & 0x01ff);

		fx=spriteram[offs+6]&0x2;
		fy=spriteram[offs+4]&0x2;
		color=spriteram[offs+3]&0x3;

		if (fx) mx += 16 * (w - 1);
		if (fy) my += 16 * (h - 1);

		/* Arrange multi-sprites on multiples of 4..  eg, ref when a round is won */
		if (size) tile=tile&0x1ffc;

		/* Similar to Punk Shot */
		for (y = 0;y < h;y++)
		{
			for (x = 0;x < w;x++)
			{
				drawgfx(bitmap,Machine->gfx[1],
					tile + xoffset[x] + yoffset[y],
					color,
					fx,fy,
					mx + 16 * x * (fx ? -1 : 1),my + 16 * y * (fy ? -1 : 1),
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
    }
  }
}

/*****************************************************************************/

void mainevt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,tile,color,mx,my,fx,bank,scrollx,scrolly;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (palette_recalc())
	{
		memset(dirty_b,1,0x800);
		memset(dirty_f,1,0x800);
		dirty_video = 1;
	}

	/* If whole video layer is unchanged, then don't even go through the control
  	loops below, this gains a 2/3% speed increase */
	if (!dirty_video) goto NO_BACK_DRAW;

	/* Draw character tiles */
	mx=-1; my=0;
	for (offs = 0x0800;offs < 0x1000;offs += 1) {
  	mx++;
  	if (mx==64) {mx=0; my++;}

  	if (!dirty_b[offs-0x800]) continue;
		dirty_b[offs-0x800]=0;

		tile=videoram[offs];
		fx=mainevt_attr_ram[offs]&0x2;
		bank=((((mainevt_attr_ram[offs]>>1)&0xe)+(mainevt_attr_ram[offs]&0x1))*0x100);
		color=mainevt_attr_ram[offs]>>6;

		drawgfx(back_bitmap,Machine->gfx[0],
			tile+bank,color+8,fx,0,8*mx,8*my,
			0,TRANSPARENCY_NONE,0);
	}

NO_BACK_DRAW:

	scrolly=-(RAM[0x180c]);
	scrollx=-((RAM[0x1a01]<<8)+RAM[0x1a00])+6;
	copyscrollbitmap(bitmap,back_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Render sprites that are behind 2nd playfield */
	mainevt_vh_drawsprites(bitmap,1);
	if (!dirty_video) goto NO_FORE_DRAW;

	/* Draw character tiles */
	mx=-1; my=0;
	for (offs = 0x1000;offs < 0x1800;offs += 1) {
		mx++;
		if (mx==64) {mx=0; my++;}

		if (!dirty_f[offs-0x1000]) continue;
		dirty_f[offs-0x1000]=0;

		tile=videoram[offs];
		fx=mainevt_attr_ram[offs]&0x2;
		color=mainevt_attr_ram[offs]>>6;

		bank=((((mainevt_attr_ram[offs]>>1)&0xe)+(mainevt_attr_ram[offs]&0x1))*0x100);

		drawgfx(front_bitmap,Machine->gfx[0],
			tile+bank,color+4,fx,0,8*mx,8*my,
			0,TRANSPARENCY_NONE,0);
	}

	dirty_video=0;

NO_FORE_DRAW:

	scrolly=-(RAM[0x380c]);
	scrollx=-((RAM[0x3a01]<<8)+RAM[0x3a00])+6;
	copyscrollbitmap(bitmap,front_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	/* Render sprites foreground sprites */
	mainevt_vh_drawsprites(bitmap,0);

	/* Draw character tiles */
	mx=-1; my=0;
	for (offs = 0x000;offs < 0x800;offs += 1) {
		mx++;
    if (mx==64) {mx=0; my++;}
    if (mx<14 || mx>50) continue; /* Clipped areas */

    tile=videoram[offs];
    bank=((((mainevt_attr_ram[offs]>>1)&0xe)+(mainevt_attr_ram[offs]&0x1))*0x100);

    /* Simple speedup */
    if (tile==0xfe && bank==0x100) continue;

 		fx=mainevt_attr_ram[offs]&0x2;
    color=mainevt_attr_ram[offs]>>6;

		drawgfx(bitmap,Machine->gfx[0],
    	tile+bank,color,fx,0,mx*8,my*8,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
  	}
}
