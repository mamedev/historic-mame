#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"




int ssi_videoram_r(int offset)
{
	return READ_WORD(&videoram[offset]);
}

void ssi_videoram_w(int offset,int data)
{
	COMBINE_WORD_MEM(&videoram[offset],data);
}

int ssi_vh_start(void)
{
	return 0;
}

void ssi_vh_stop (void)
{
}

void ssi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,x,y,offs,code,color,spritecont,flipx,flipy;
	int xcurrent,ycurrent;

	/* update the palette usage */
	{
		unsigned short palette_map[128];

		memset (palette_map, 0, sizeof (palette_map));

		x=0;
		y=0;
		xcurrent=0;
		ycurrent=0;
		color=0;

		for(offs=0;offs<0x3400;offs+=16)
		{
	        spritecont = (READ_WORD(&videoram[offs+8]) & 0xff00)>>8;

			if((spritecont&0x10)==0)
			{
				if((spritecont&0x04)==0)
				{
					x = READ_WORD(&videoram[offs+6]) & 0x0fff;
					if(x>0x7FF)
						x=16-((x^0xFFF)+1);
					else
						x=16+x;
					xcurrent=x;
				}
				else
					x=xcurrent;
			}
			else
			{			// ???
				if((spritecont&0x20)!=0)
				{	// OK
					x+=16;
				}
			}

			if((spritecont&0x40)==0)
			{
				if((spritecont&0x04)==0)
				{
					y = READ_WORD(&videoram[offs+4]) & 0x0fff;
					if(y>0x7FF)
						y=(320+((y^0xFFF)+1));
					else
						y=(320-y);
					ycurrent=y;
				}
				else
					y=ycurrent;
			}
			else
			{			// ???
				if((spritecont&0x80)!=0)
				{	// OK
					y-=16;
				}
			}

			if((spritecont&0x04)==0)
		        color=(READ_WORD(&videoram[offs+8])&0x007f);

			if((x>0)&&(y>0)&&(x<224+16)&&(y<320+16))
			{
		        code=((READ_WORD(&videoram[offs])&0x1fff)<<8);
				if (code != 0)
					palette_map[color] |= Machine->gfx[0]->pen_usage[code>>8];
			}
		}

		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
				for (j = 1; j < 16; j++)
					if (palette_map[i] & (1 << j))
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
					else
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
			}
			else
				memset (&palette_used_colors[i * 16], PALETTE_COLOR_UNUSED, 16);
		}

		palette_recalc ();
	}

	osd_clearbitmap(bitmap);
	x=0;
	y=0;
	xcurrent=0;
	ycurrent=0;
	color=0;
	for(offs=0;offs<0x3400;offs+=16)
	{

	        spritecont = (READ_WORD(&videoram[offs+8]) & 0xff00)>>8;

		flipy = spritecont&0x01;
		flipx = (spritecont&0x02)>>1;

		if((spritecont&0x10)==0)
		{
		        if((spritecont&0x04)==0)
			{
			        x = READ_WORD(&videoram[offs+6]) & 0x0fff;
				if(x>0x7FF)
				{
				        x=16-((x^0xFFF)+1);
				}
				else
				{
				        x=16+x;
				}
				xcurrent=x;
			}
			else
			{
			        x=xcurrent;
			}
		}
		else
		{			// ???
		        if((spritecont&0x20)!=0)
			{	// OK
			        x+=16;
			}
		}

		if((spritecont&0x40)==0)
		{
		        if((spritecont&0x04)==0)
			{
			        y = READ_WORD(&videoram[offs+4]) & 0x0fff;
				if(y>0x7FF)
				{
				        y=(320+((y^0xFFF)+1));
				}
				else
				{
				        y=(320-y);
				}
				ycurrent=y;
			}
			else
			{
			        y=ycurrent;
			}
		}
		else
		{			// ???
		        if((spritecont&0x80)!=0)
			{	// OK
			        y-=16;
			}
		}

		if((spritecont&0x04)==0)
		{
		        color=(READ_WORD(&videoram[offs+8])&0x007f);
		}

		if((x>0)&&(y>0)&&(x<224+16)&&(y<320+16))
		{

		if (errorlog&&(flipy||flipy)) fprintf(errorlog,"fx %d; fy %d\n",flipx,flipy);
		        code=((READ_WORD(&videoram[offs])&0x1fff)<<8);
			if (code != 0)
			{
				drawgfx(bitmap,Machine->gfx[0],
					(code>>8),
					color,
					flipx,flipy,
					x,y,
					&Machine->drv->visible_area,
					TRANSPARENCY_PEN,0);

			}

		}

	}

}




int ssi_interrupt(void)
{
return MC68000_IRQ_5;
}


