#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"




unsigned char *paletteram;
int paletteram_size;
static int pal_div;
static unsigned char *dirtypal;
static unsigned int col_test;


void ssi_paletteram_w(int offset, int data)
{
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&paletteram[offset],newword);
		dirtypal[offset / pal_div] = 1;
	}
}

int ssi_paletteram_r(int offset)
{
return READ_WORD(&paletteram[offset]);
}

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
        pal_div=32;
	if ((dirtypal = malloc(paletteram_size / pal_div)) == 0)
	{
		free(dirtybuffer);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}
	memset(dirtypal,1,paletteram_size / pal_div);

	if (Machine->scrbitmap->depth == 8) col_test = 32; else col_test = 8;

	return 0;
}

void ssi_vh_stop (void)
{
	free(dirtypal);
}

void ssi_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,x,y,offs,code,color,spritecont,pal,flipx,flipy,r,g,b;
	int xcurrent,ycurrent;

	for (offs = 0; offs < paletteram_size / pal_div; offs++)
	{
		if (dirtypal[offs])
		{
			for (i = 0;i < 16;i++)
			{
			        pal=READ_WORD(&paletteram[i+i+(offs*pal_div)]);
				r=(pal&0xf000)>>8;
				g=(pal&0x0f00)>>4;
				b=(pal&0x00f0)>>0;
				/* avoid undesired transparency */
				if (i % 16 != 0 && r <= col_test && g <= col_test && b <= (col_test<<1))
					r = col_test;
				setgfxcolorentry(Machine->gfx[0],
						 i+(offs<<4),r,g,b);
			}
		}
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
					TRANSPARENCY_COLOR,0);

			}

		}

	}

}




int ssi_interrupt(void)
{
return MC68000_IRQ_5;
}


