/***************************************************************************

 COSMIC.C

 emulation of video hardware of cosmic machines of 1979-1980(ish)

 (256 * 192)

 1. Cosmic Alien,
 2. Cosmic Guerilla
 3. Space Panic
 4. Magic Spot II
 5. Devil Zone
 6. No Man's Land

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *cosmic_videoram;

/**************************************************/
/* Colour Decode variables and Functions          */
/**************************************************/

const unsigned char *colourrom;

int ColourRegisters[3];
int ColourMap=0;
int BackGround=0;
int CosmicFlipX;
int CosmicFlipY;
int	MachineID;

int ColourRomLookup(int ScreenX,int ScreenY)
{
	int ColourByte;

    switch(MachineID)
    {
    	case 2 :
            {
    	        /* Cosmic Guerilla = 16 x 16 */

		        if ((ScreenY > 23) && (ScreenY < 32))
			        ColourByte = colourrom[ColourMap + (ScreenX / 16) * 16 + 9];
                else
			        ColourByte = colourrom[ColourMap + (ScreenX / 16) * 16 + (15 - ((ScreenY) / 16))];

    	        return Machine->pens[ColourByte & 0x0F];
            }

    	case 4 :
            {
		        /* Magic Spot = 16 x 8 colouring */

  	            if ((ScreenY > 31) && (ScreenY < 40))
			        ColourByte = colourrom[ColourMap + 26 * 16 + (ScreenX / 16)];
                else
			        ColourByte = colourrom[ColourMap + (31-(ScreenY / 8)) * 16 + (ScreenX / 16)];

	            if (ColourRegisters[1] != 0) return Machine->pens[ColourByte >> 4];
	            else return Machine->pens[ColourByte & 0x0F];
            }

        case 5 :
        case 6 :
            {
		        /* No Mans Land, Devil Zone = 16 x 8 colouring */

		        ColourByte = colourrom[ColourMap + (31-(ScreenY / 8)) * 16 + (ScreenX / 16)];

	            if (ColourRegisters[1] != 0) return Machine->pens[ColourByte >> 4];
	            else return Machine->pens[ColourByte & 0x0F];
            }

        case 1 :
        case 3 :
            {
		        /* 8 x 16 colouring */

		        ColourByte = colourrom[ColourMap + (15 - (ScreenY / 16)) * 32 + (ScreenX / 8)];

  	            if (ColourRegisters[1] != 0) return Machine->pens[ColourByte >> 4];
  	            else return Machine->pens[ColourByte & 0x0F];
            }
    }

    return 0; /* Should NEVER get here */
}

void ColourBankSwitch(void)
{
	/* Need to re-colour existing screen */

	int x,y;
    int ex,ey;

    if (!(Machine->orientation & ORIENTATION_SWAP_XY))
    {
        for(x=0;x<192;x++)
		{
			if (CosmicFlipX)
				ex = 191 - x;
            else
                ex = x;

            for(y=0;y<255;y++)
            {
				if (CosmicFlipY)
                    ey = y;
                else
                    ey = 255 - y;

                if(tmpbitmap->line[ex][ey] != Machine->pens[0])
                {
                    tmpbitmap->line[ex][ey] = ColourRomLookup(x,y);
                }
            }
        }

 		osd_mark_dirty(0,0,255,191,0);	/* ASG 971015 */
    }
    else
    {
        /* Normal */

        for(x=0;x<192;x++)
		{
			if (CosmicFlipX)
				ex = 191 - x;
            else
                ex = x;

            for(y=0;y<255;y++)
            {
				if (CosmicFlipY)
                    ey = y;
                else
                    ey = 255 - y;

                if(tmpbitmap->line[ey][ex] != Machine->pens[0])
                {
                    tmpbitmap->line[ey][ex] = ColourRomLookup(x,y);
                }
            }
        }

		osd_mark_dirty(0,0,191,255,0);	/* ASG 971015 */
    }
}



/**************************************************/
/* Space Panic specific routines                  */
/**************************************************/

static const unsigned char Remap[64][2] = {
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 00 */
{0x00,0},{0x26,0},{0x25,0},{0x24,0},{0x23,0},{0x22,0},{0x21,0},{0x20,0}, /* 08 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 10 */
{0x00,0},{0x16,0},{0x15,0},{0x14,0},{0x13,0},{0x12,0},{0x11,0},{0x10,0}, /* 18 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 20 */
{0x00,0},{0x06,0},{0x05,0},{0x04,0},{0x03,0},{0x02,0},{0x01,0},{0x00,0}, /* 28 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 30 */
{0x07,2},{0x06,2},{0x05,2},{0x04,2},{0x03,2},{0x02,2},{0x01,2},{0x00,2}, /* 38 */
};

/*
 * Panic Colour table setup
 *
 * Bit 0 = RED, Bit 1 = GREEN, Bit 2 = BLUE
 *
 * First 8 colours are normal intensities
 *
 * But, bit 3 can be used to pull Blue via a 2k resistor to 5v
 * (1k to ground) so second version of table has blue set to 2/3
 *
 */

void panic_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    colourrom = color_prom + 0x24;
	MachineID = 3;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = 0xff * ((i >> 0) & 1);
		*(palette++) = 0xff * ((i >> 1) & 1);
		if ((i & 0x0c) == 0x08)
			*(palette++) = 0xaa;
		else
			*(palette++) = 0xff * ((i >> 2) & 1);
	}


	/* characters and sprites use the same palette */

	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0x0f;
}

/***************************************************************************
 * Space Panic Colour Selectors
 *
 * 7c0c & 7c0e = Rom Address Offset
 * 7c0d        = high / low nibble
 **************************************************************************/

void panic_colourmap_select(int offset,int data)
{
	if (ColourRegisters[offset] != (data & 0x80))
    {
    	ColourRegisters[offset] = data & 0x80;
    	ColourMap = (ColourRegisters[0] << 2) + (ColourRegisters[2] << 3);
		ColourBankSwitch();
    }
}

/**************************************************/
/* Cosmic Guerilla specific routines              */
/**************************************************/

/*
 * Cosmic guerilla table setup
 *
 * Use AA for normal, FF for Full Red
 * Bit 0 = R, bit 1 = G, bit 2 = B, bit 4 = High Red
 *
 * It's possible that the background is dark gray and not black, as the
 * resistor chain would never drop to zero, Anybody know ?
 *
 */

void cosmicguerilla_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    colourrom = color_prom+32;
	MachineID = 2;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
    	if (i > 8) *(palette++) = 0xff;
        else *(palette++) = 0xaa * ((i >> 0) & 1);

		*(palette++) = 0xaa * ((i >> 1) & 1);
		*(palette++) = 0xaa * ((i >> 2) & 1);
	}
}

void cosmicguerilla_colourmap_select(int offset,int data)
{
	if (ColourRegisters[offset] != data)
    {
    	ColourRegisters[offset] = data;

        ColourMap = 0;

        if (ColourRegisters[0] == 1) ColourMap += 0x100;
        if (ColourRegisters[1] == 1) ColourMap += 0x200;

		ColourBankSwitch();
    }
}

/**************************************************/
/* Cosmic Alien specific routines                 */
/**************************************************/

/*
 * Cosmic Alien Colour table setup
 *
 * Bit 0 = RED, Bit 1 = GREEN, Bit 2 = BLUE
 *
 */

void cosmicalien_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    colourrom = color_prom + 0x24;
	MachineID = 1;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = 0xff * ((i >> 0) & 1);
		*(palette++) = 0xff * ((i >> 1) & 1);
		*(palette++) = 0xff * ((i >> 2) & 1);
	}

	/* characters and sprites use the same palette */

	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0x0f;
}

void cosmicalien_colourmap_select(int offset,int data)
{
	if (ColourRegisters[1] != data)
    {
        ColourRegisters[1] = data;
		ColourBankSwitch();
    }
}

/**************************************************/
/* Magical Spot                                   */
/**************************************************/

void magspot2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    colourrom  = color_prom + 0x22;
	MachineID  = 4;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if ((i & 0x09) == 0x08)
			*(palette++) = 0xaa;
	 	else
			*(palette++) = 0xff * ((i >> 0) & 1);

		*(palette++) = 0xff * ((i >> 1) & 1);
		*(palette++) = 0xff * ((i >> 2) & 1);
	}

	/* characters and sprites use the same palette */

	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0xf;

}

void magspot2_colourmap_w(int offset, int data)
{
	ColourRegisters[1] = data;
}

/**************************************************/
/* Devil Zone                                     */
/**************************************************/

void devzone_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    colourrom  = color_prom + 0x2;
	MachineID  = 5;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if ((i & 0x09) == 0x08)
			*(palette++) = 0xaa;
	 	else
			*(palette++) = 0xff * ((i >> 0) & 1);

		*(palette++) = 0xff * ((i >> 1) & 1);
		*(palette++) = 0xff * ((i >> 2) & 1);
	}

	/* characters and sprites use the same palette */

}

/**************************************************/
/* No Man's Land                                  */
/**************************************************/

/* I don't know if there are more screen layouts than this */
/* this one seems to be OK for the start of the game       */

static const signed short TreePositions[2][2] = {
	{66,31},{66,127}
};

static const signed short WaterPositions[4][2] = {
	{160,0},{160,64},{160,96},{160,160}
};

void nomanland_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	magspot2_vh_convert_color_prom(palette, colortable, color_prom);

	MachineID = 6;
}

void nomanland_background_w(int offset, int data)
{
	BackGround = data;
}

/**************************************************/
/* Common Routines                                */
/**************************************************/

void cosmic_flipscreen_w(int offset, int data)
{
 static int LastMode=0;
 int x,y,Safe;

 	if (data != LastMode)
	{
     	LastMode = data;

     	if (data)
     	{
     		CosmicFlipX = ((Machine->orientation & ORIENTATION_FLIP_X) == 0);
	     	CosmicFlipY = ((Machine->orientation & ORIENTATION_FLIP_Y) == 0);
    	}
     	else
     	{
      		CosmicFlipX = (Machine->orientation & ORIENTATION_FLIP_X);
      		CosmicFlipY = (Machine->orientation & ORIENTATION_FLIP_Y);
     	}

     	/* Flip Bitmap */

     	if (!(Machine->orientation & ORIENTATION_SWAP_XY))
     	{
     		for(x=0;x<96;x++)
      		{
            	for(y=0;y<255;y++)
            	{
            		Safe = tmpbitmap->line[x][y];
                	tmpbitmap->line[x][y] = tmpbitmap->line[191-x][255-y];
	                tmpbitmap->line[191-x][255-y] = Safe;
    	        }
        	}

	       	osd_mark_dirty(0,0,255,191,0);
	    }
     	else
     	{
     		for(x=0;x<96;x++)
      		{
        		for(y=0;y<255;y++)
            	{
            		Safe = tmpbitmap->line[y][x];
	                tmpbitmap->line[y][x] = tmpbitmap->line[255-y][191-x];
    	            tmpbitmap->line[255-y][191-x] = Safe;
            	}
        	}

      		osd_mark_dirty(0,0,191,255,0);
     	}
 	}
}

void cosmic_videoram_w(int offset,int data)
{
    int i,x,y;
    int col;

	if ((cosmic_videoram[offset] != data))
	{
	    cosmic_videoram[offset] = data;

		x = offset / 32;
		y = 256-8 - 8 * (offset % 32);

        col = ColourRomLookup(x,y);

        /* Allow Rotate */

        if (!(Machine->orientation & ORIENTATION_SWAP_XY))
        {
			if (CosmicFlipX)
				x = 191 - x;

			if (CosmicFlipY)
            {
				osd_mark_dirty(y,x,y+7,x,0);

			    for (i = 0;i < 8;i++)
			    {
				    if (data & 0x01) tmpbitmap->line[x][y] = col;
				    else tmpbitmap->line[x][y] = Machine->pens[0]; /* black */

				    y++;
				    data >>= 1;
                }
            }
            else
            {
				y = 255 - y;
				osd_mark_dirty(y-7,x,y,x,0);

			    for (i = 0;i < 8;i++)
			    {
				    if (data & 0x01) tmpbitmap->line[x][y] = col;
				    else tmpbitmap->line[x][y] = Machine->pens[0]; /* black */

				    y--;
				    data >>= 1;
                }
            }
        }
        else
        {
            /* Normal */

			if (CosmicFlipX)
				x = 191 - x;

			if (CosmicFlipY)
            {
				osd_mark_dirty(x,y,x,y+7,0);

			    for (i = 0;i < 8;i++)
			    {
				    if (data & 0x01) tmpbitmap->line[y][x] = col;
				    else tmpbitmap->line[y][x] = Machine->pens[0]; /* black */

				    y++;
				    data >>= 1;
                }
            }
            else
            {
				y = 255 - y;
				osd_mark_dirty(x,y-7,x,y,0);

			    for (i = 0;i < 8;i++)
			    {
				    if (data & 0x01) tmpbitmap->line[y][x] = col;
				    else tmpbitmap->line[y][x] = Machine->pens[0]; /* black */

				    y--;
				    data >>= 1;
                }
            }
        }
	}
}


int cosmic_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	/* initialize the bitmap to our background color */

	fillbitmap(tmpbitmap,Machine->pens[0],&Machine->drv->visible_area);

    /* Initialise Bitmap orientation */

	CosmicFlipX = (Machine->orientation & ORIENTATION_FLIP_X);
	CosmicFlipY = (Machine->orientation & ORIENTATION_FLIP_Y);

	return 0;
}


void cosmic_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}


void cosmic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, Sprite=0;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


    /* Draw the sprites () */

    if (MachineID == 3)
    {
		int Bank, Rotate;

	    for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	    {
		    if (spriteram[offs] != 0)
            {
        	    /* Remap sprite number to my layout */

                Sprite = Remap[(spriteram[offs] & 0x3F)][0];
                Bank   = Remap[(spriteram[offs] & 0x3F)][1];
                Rotate = spriteram[offs] & 0x40;

                /* Switch Bank */

                if(spriteram[offs+3] & 0x08) Bank=1;

		        drawgfx(bitmap,Machine->gfx[Bank],
				        Sprite,
				        7 - (spriteram[offs+3] & 0x07),
				        0,Rotate,
				        256-spriteram[offs+2],spriteram[offs+1]-32,
				        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
            }
	    }
    }
}


void cosmic_vh_screenrefresh_sprites(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, Sprite=0;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw the sprites () */

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs] != 0)
        {
			Sprite = ~spriteram[offs] & 0x3f;

            if (spriteram[offs] & 0x80)
            {
                /* 16x16 sprite */

			    drawgfx(bitmap,Machine->gfx[0],
					    Sprite,
					    7 - (spriteram[offs+3] & 0x07),
					    0,0,
				    	256-spriteram[offs+2],(spriteram[offs+1]-32),
				        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
            }
            else
            {
                /* 32x32 sprite */

			    drawgfx(bitmap,Machine->gfx[1],
					    Sprite >> 2,
					    7 - (spriteram[offs+3] & 0x07),
					    0,0,
				    	256-spriteram[offs+2],(spriteram[offs+1]-32),
				        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
            }
        }
	}

    /* Background for No Mans Land */

    if ((MachineID == 6) && (BackGround))
    {
        static int Animate=0;
    	int y;

        Animate = (++Animate & 255);

        if (CosmicFlipY == (Machine->orientation & ORIENTATION_FLIP_Y))
        	Sprite = 1;
        else
        	Sprite = 3;

    	for(offs=0;offs<2;offs++)
        {
            if (CosmicFlipY == (Machine->orientation & ORIENTATION_FLIP_Y))
        	    y = TreePositions[offs][0];
            else
        	    y = 225 - TreePositions[offs][0];

    		drawgfx(bitmap,Machine->gfx[1],
					Sprite,
					8,
					0,0,
					y,TreePositions[offs][1],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }

    	for(offs=0;offs<4;offs++)
        {
            if (CosmicFlipY == (Machine->orientation & ORIENTATION_FLIP_Y))
        	    y = WaterPositions[offs][0];
            else
        	    y = 241 - WaterPositions[offs][0];

    		drawgfx(bitmap,Machine->gfx[2],
					(Animate >> 3),
					9,
					0,0,
					y,WaterPositions[offs][1],
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }
    }
}

