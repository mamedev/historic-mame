/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

#define VRAM_HIGH 0x08000

//#define NOISY_VIDEO
/* Filter by video port */
//#define NOISE_FILTER (offset==0x0b)
/* Filter by video address */
#define NOISE_FILTER (leland_video_addr[num] < 0x7800)

//#define VIDEO_FIDDLE


#define VIDEO_WIDTH 0x28
#define VIDEO_HEIGHT 0x1e

static int leland_video_addr[2];
static int leland_video_plane_count[2];

/* Callback bank handler */
extern void (*leland_update_master_bank)(void);

/* Palette RAM */
unsigned char *leland_palette_ram;
const int     leland_palette_ram_size=0x0400;

/* Video RAM (internal to gfx chip ???) */
unsigned char *leland_video_ram;
const int leland_video_ram_size=0x10000;
static struct osd_bitmap *screen_bitmap;

/* Scroll background registers */
static int leland_bk_xlow;
static int leland_bk_xhigh;
static int leland_bk_ylow;
static int leland_bk_yhigh;

/* Data latches */
static int leland_vram_high_data;
static int leland_vram_low_data;

/* Background graphics */
static struct osd_bitmap *background_bitmap;

void leland_draw_bkchar(int x, int y, int ch)
{
    unsigned char *bm;
    unsigned char *sd;
    int ny;
    sd= Machine->gfx[0]->gfxdata->line[ch*8];
    for (ny = 0;ny < 8;ny++)
    {
        bm=background_bitmap->line[8*y+ny] + 8*x;
        bm[0]=sd[0];
        bm[1]=sd[1];
        bm[2]=sd[2];
        bm[3]=sd[3];
        bm[4]=sd[4];
        bm[5]=sd[5];
        bm[6]=sd[6];
        bm[7]=sd[7];
        sd+=8;
    }
}

/* Graphics control register (driven by AY8910 port A) */
static int leland_gfx_control;
int leland_sound_port_r(int offset, int data)
{
    return leland_gfx_control;
}

extern void leland_sound_port_w(int offset, int data)
{
    leland_gfx_control=data;
    if (leland_update_master_bank)
    {
        /* Update master bank */
        leland_update_master_bank();
    }
}

/* Scroll ports */
void leland_bk_xlow_w(int offset, int data)
{
        leland_bk_xlow=data;
}

void leland_bk_xhigh_w(int offset, int data)
{
        leland_bk_xhigh=data;
}

void leland_bk_ylow_w(int offset, int data)
{
        leland_bk_ylow=data;
}

void leland_bk_yhigh_w(int offset, int data)
{
        leland_bk_yhigh=data;
}

/* Palette RAM */
void leland_palette_ram_w(int offset, int data)
{
        leland_palette_ram[offset]=data;
}

int leland_palette_ram_r(int offset)
{
        return leland_palette_ram[offset];
}

/* Word video address */

void leland_video_addr_w(int offset, int data, int num)
{
	if (!offset)
    {
        leland_video_addr[num]=(leland_video_addr[num]&0xff00)|data;
    }
	else
	{
        leland_video_addr[num]=(leland_video_addr[num]&0x00ff)|data<<8;
	}

    /* Reset the video plane count */
    leland_video_plane_count[num]=0;
}


int leland_vram_low_r(int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    return leland_video_ram[addr];
}

int leland_vram_high_r(int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    return leland_video_ram[addr+VRAM_HIGH];
}

void leland_vram_low_w(int data, int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    leland_video_ram[addr]=data;
    leland_vram_low_data=data;  /* Set data latch */
}

void leland_vram_high_w(int data, int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    leland_video_ram[addr+VRAM_HIGH]=data;
    leland_vram_high_data=data; /* Set data latch */
}

void leland_vram_all_w(int data, int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    leland_video_ram[addr+0x00000]=data;
    leland_video_ram[addr+0x00001]=data;
    leland_video_ram[addr+VRAM_HIGH]=data;
    leland_video_ram[addr+VRAM_HIGH+1]=data;
}

void leland_vram_fill_w(int data, int num)
{
    int i;
    for (i=leland_video_addr[num]&0x7f; i<0x80; i++)
    {
        leland_vram_high_w(data, num);
        leland_vram_low_w(leland_vram_low_data, num);
        leland_video_addr[num]++;
    }
}

void leland_vram_planar_w(int data, int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    addr+=(leland_video_plane_count[num]&0x01)*VRAM_HIGH;
    addr+=(leland_video_plane_count[num])>>1;
    leland_video_ram[addr]=data;
    leland_video_plane_count[num]++;
}

void leland_vram_planar_masked_w(int data, int num)
{
    int addr=leland_video_addr[num]&0x7fff;
    addr+=(leland_video_plane_count[num]&0x01)*VRAM_HIGH;
    addr+=(leland_video_plane_count[num])>>1;

#if 0
    if (data&0xf0)
    {
        leland_video_ram[addr] &= 0x0f;
    }
    if (data&0x0f)
    {
        leland_video_ram[addr] &= 0xf0;
    }
    leland_video_ram[addr]|=data;
#else
    leland_video_ram[addr]=data;
#endif

    leland_video_plane_count[num]++;
}


int leland_vram_planar_r(int num)
{
    int ret;
    int addr=leland_video_addr[num]&0x7fff;
    addr+=(leland_video_plane_count[num]&0x01)*VRAM_HIGH;
    addr+=(leland_video_plane_count[num])>>1;
    ret= leland_video_ram[addr];
    leland_video_plane_count[num]++;
    return ret;
}

int leland_vram_plane_r(int num)
{
    int ret;
    int addr=leland_video_addr[num]&0x7fff;
    addr+=(leland_video_plane_count[num]&0x01)*VRAM_HIGH;
    addr+=(leland_video_plane_count[num])>>1;
    ret= leland_video_ram[addr];
    return ret;
}


int leland_vram_port_r(int offset, int num)
{
    int ret;

    switch (offset)
    {
        case 0x03:
            ret=leland_vram_plane_r(num);
            break;

        case 0x05:
            ret=leland_vram_high_r(num);
            break;

        case 0x06:
            ret=leland_vram_low_r(num);
            break;

        case 0x0b:
            ret=leland_vram_planar_r(num);
            break;

        case 0x0d:
            ret=leland_vram_high_r(num);
            leland_video_addr[num]++;
            break;

        case 0x0e:
            ret=leland_vram_low_r(num);
            leland_video_addr[num]++;
            break;

        default:
            if (errorlog)
            {
                fprintf(errorlog, "CPU #%d %04x Warning: Unknown video port %02x read (address=%04x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset,
                        leland_video_addr[num]);
            }
            return 0;
    }

#ifdef NOISY_VIDEO
    if (errorlog && NOISE_FILTER)
    {
        fprintf(errorlog, "CPU #%d %04x video port %02x read (address=%04x value=%02x)\n",
                   cpu_getactivecpu(),cpu_get_pc(), offset,
                   leland_video_addr[num], ret);
    }
#endif

    return ret;
}

void leland_vram_port_w(int offset, int data, int num)
{
    int ret, i;


    switch (offset)
    {
        case 0x01:
            leland_vram_fill_w(data, num);
            break;

        case 0x05:
            leland_vram_high_w(data, num);
            break;

        case 0x06:
            leland_vram_low_w(data, num);
            break;

        case 0x09:
            leland_vram_high_w(data, num);
            leland_vram_low_w(leland_vram_low_data, num);
            leland_video_addr[num]++;
            break;

        case 0x0a:
            leland_vram_low_w(data, num);
            leland_vram_high_w(leland_vram_high_data, num);
            leland_video_addr[num]++;
            break;

        case 0x0b:
            leland_vram_planar_w(data,num);
            break;

        case 0x0e: /* Low write with latch */
            leland_vram_low_w(data, num);
            leland_video_addr[num]++;
            break;

        case 0x0d: /* High write with latch */
            leland_vram_high_w(data, num);
            leland_video_addr[num]++;
            break;

        case 0x1b:
            leland_vram_planar_masked_w(data, num);
            break;

        default:
            if (errorlog)
            {
                fprintf(errorlog, "CPU #%d %04x Warning: Unknown video port %02x write (address=%04x value=%02x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset,
                        leland_video_addr[num], data);
            }
            return;
    }


#ifdef NOISY_VIDEO
    if (errorlog && NOISE_FILTER)
    {
        fprintf(errorlog, "CPU #%d %04x video port %02x write (address=%04x value=%02x)\n",
                   cpu_getactivecpu(),cpu_get_pc(), offset,
                   leland_video_addr[num], data);
    }
#endif

}


void leland_mvram_port_w(int offset, int data)
{
    leland_vram_port_w(offset, data, 0);
}

void leland_svram_port_w(int offset, int data)
{
    leland_vram_port_w(offset, data, 1);
}

int  leland_mvram_port_r(int offset)
{
    return leland_vram_port_r(offset, 0);
}

int  leland_svram_port_r(int offset)
{
    return leland_vram_port_r(offset, 1);
}

void leland_master_video_addr_w(int offset, int data)
{
    leland_video_addr_w(offset, data, 0);
}

void leland_slave_video_addr_w(int offset, int data)
{
    leland_video_addr_w(offset, data, 1);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int leland_vh_start(void)
{
    screen_bitmap = osd_create_bitmap(VIDEO_WIDTH*8, VIDEO_HEIGHT*8);
	if (!screen_bitmap)
	{
		return 1;
	}
    fillbitmap(screen_bitmap,palette_transparent_pen,NULL);

    leland_video_ram=malloc(leland_video_ram_size);
    if (!leland_video_ram)
	{
		return 1;
	}
    memset(leland_video_ram, 0, leland_video_ram_size);

	return 0;
}

void leland_onboard_dac(void)
{
    int data, i, offset;
    int dac1on, dac2on;
    static int counter;

    dac1on=(~leland_gfx_control)&0x01;
    dac2on=(~leland_gfx_control)&0x02;
    if (dac1on || dac2on)
    {
        /*
        Either DAC1 or DAC2 are on.
        Output sound. Since both DACs are triggered by the video
        refresh at the same time, we combine the two values and
        use 1 MAME DAC
        */

        /* The timing is completely wrong. */
        for (i=0; i<0x100; i++)
        {
            offset=((counter/0x30)*0x080)+counter%0x30+0x50;
            data=0;
            if (dac1on)
            {
                data =(char)leland_video_ram[offset];
            }
            if (dac2on)
            {
                data+=(char)leland_video_ram[offset+VRAM_HIGH];
            }
            DAC_signed_data_w(0, data);
            counter++;
            if (offset == 0x77ff)
            {
                counter=0;
            }
        }
    }
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void leland_vh_stop(void)
{
	if (screen_bitmap)
	{
		osd_free_bitmap(screen_bitmap);
	}
    if (leland_video_ram)
	{
        free(leland_video_ram);
	}
}

INLINE void leland_build_palette(void)
{
	int offset;

    for (offset = 0; offset < 0x0400; offset++)
	{
		int red, green, blue;
        red   = leland_palette_ram[offset]&0x07;
        green = (leland_palette_ram[offset]&0x38)>>3;
        blue  = (leland_palette_ram[offset]&0xc0)>>6;
        palette_change_color (offset,
                        red*17*2+red*2,
                        green*17*2+green*2,
                        blue*20*2+blue*2);
        palette_used_colors[offset] = PALETTE_COLOR_USED;
	}
	palette_recalc();
}

/***************************************************************************

    Merge background and foreground bitmaps.

***************************************************************************/



void leland_draw_bitmap(struct osd_bitmap *bitmap, struct osd_bitmap *bkbitmap)
{
    int x,y, offs, data, j, value;
    /* Render the bitmap. */
    for (y=0; y<VIDEO_HEIGHT*8; y++)
    {
        for (x=0; x<VIDEO_WIDTH; x++)
        {
            int r=0;
            int bitmapvalue, bkvalue;
            offs=y*0x80+x*2;

            data =((unsigned long)leland_video_ram[0x00000+offs])<<(3*8);
            data|=((unsigned long)leland_video_ram[VRAM_HIGH+offs])<<(2*8);
            data|=((unsigned long)leland_video_ram[0x00001+offs])<<(1*8);
            data|=((unsigned long)leland_video_ram[VRAM_HIGH+1+offs]);

            for (j=32-4; j>=0; j-=4)
            {
                value=0;
                value=(data>>j)&0x0f;

                if (bkbitmap)
                {
                    /*
                    Merge the two bitmaps. The palette index is obtained
                    by combining the foreground and background graphics.
                    TODO:
                    - Support scrolling
                    */
                    if (Machine->orientation & ORIENTATION_SWAP_XY)
                    {
                        int line=VIDEO_WIDTH*8-1-(x*8+r);
                        bkvalue=bkbitmap->line[line][y];
                        bitmapvalue=Machine->pens[value*0x40+bkvalue];
                        screen_bitmap->line[line][y]=bitmapvalue;
                    }
                    else
                    {
                        bkvalue=bkbitmap->line[y][x*8+r];
                        bitmapvalue=Machine->pens[value*0x40+bkvalue];
                        screen_bitmap->line[y][x*8+r]=bitmapvalue;
                    }
                }
                else
                {
                    /* Legacy code */
                    if (!value)
                    {
                        bitmapvalue=palette_transparent_pen;
                    }
                    else
                    {
                        bitmapvalue=Machine->pens[value*0x40];
                    }
                    if (Machine->orientation & ORIENTATION_SWAP_XY)
                    {
                        int line=VIDEO_WIDTH*8-1-(x*8+r);
                        screen_bitmap->line[line][y]=bitmapvalue;
                    }
                    else
                    {
                        screen_bitmap->line[y][x*8+r]=bitmapvalue;
                    }
                }
                r++;
            }
        }
    }
    copybitmap(bitmap,screen_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

#ifdef MAME_DEBUG
	if (osd_key_pressed(OSD_KEY_F))
	{
		FILE *fp=fopen("VIDEOR.DMP", "w+b");
		if (fp)
		{
            fwrite(leland_video_ram, leland_video_ram_size, 1, fp);
			fclose(fp);
		}
        fp=fopen("PALETTE.DMP", "w+b");
		if (fp)
		{
            fwrite(leland_palette_ram, leland_palette_ram_size, 1, fp);
			fclose(fp);
		}
	}
#endif
}


/***************************************************************************

    Shared video refresh funciton

***************************************************************************/

void leland_screenrefresh(struct osd_bitmap *bitmap,int full_refresh, int bkcharbank)
{
#define Z 1
	int j,k,x,y,value, offs, colour;
	unsigned long data;
	long l;
    int scrollx, scrolly, xfine, yfine;
    int bkprombank;
    int ypos;
    int sx, sy;
    unsigned char *BKGND = Machine->memory_region[4];

    /* PROM high bank */
    bkprombank=((leland_gfx_control>>3)&0x01)*0x2000;

    scrollx=leland_bk_xlow+256*leland_bk_xhigh;
    xfine=scrollx&0x07;
    sx=scrollx/8;
    scrolly=leland_bk_ylow+(256*leland_bk_yhigh);
    yfine=scrolly&0x07;
    ypos=scrolly/8;

    /* Build the palette */
    leland_build_palette();

    /*
    Render the background graphics
    TODO: rewrite this completely using the proper video combining technique.
    */
    for (y=0; y<VIDEO_HEIGHT+1; y++)
    {
                for (x=0; x<VIDEO_WIDTH+1; x++)
                {
                        int code;
                        offs =(ypos&0x1f)*0x100;
                        offs+=((ypos>>5) & 0x07)*0x4000;
                        offs+=bkprombank;
                        offs+=((sx+x)&0xff);
                        offs&=0x1ffff;
                        code=BKGND[offs];

                        code+=(((ypos>>6)&0x03)*0x100);
                        code+=bkcharbank;

                        colour=(code&0xe0)>>5;
                        drawgfx(bitmap,Machine->gfx[0],
                                        code,
                                        colour,
                                        0,0,
                                        8*x-xfine,8*y-yfine,
                                        &Machine->drv->visible_area,
                                        TRANSPARENCY_NONE ,0);
                }
                ypos++;
    }

    /* Merge the two bitmaps together */
    leland_draw_bitmap(bitmap, 0);

    leland_onboard_dac();
}

void leland_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    int bkcharbank=((leland_gfx_control>>4)&0x01)*0x0400;
    leland_screenrefresh(bitmap, full_refresh, bkcharbank);
}

void pigout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    /*
    Some games have more background graphics and employ an
    extra bank bit. These are the games with 0x8000 sized character
    sets.
    */
    int bkcharbank=((leland_gfx_control>>4)&0x03)*0x0400;
    leland_screenrefresh(bitmap, full_refresh, bkcharbank);
}

/***************************************************************************

  Ataxx

  Different from other Leland games.
  1) 12 bit palette instead of 8 bit palette.
  2) RAM based background (called QRAM in the self test)
  3) Background characters are 6 bits per pixel with no funny banking.

***************************************************************************/

unsigned char * ataxx_tram;         /* Unknown TRAM */
int ataxx_tram_size;
const int ataxx_qram_size=0x4000;   /* Really 0x2000, but bank is 0x4000 */
unsigned char * ataxx_qram1;        /* Background RAM # 1 */
unsigned char * ataxx_qram2;        /* Background RAM # 2 */

int ataxx_vram_port_r(int offset, int num)
{
    int ret;
    switch (offset)
    {
        case 0x06:
            ret=leland_vram_plane_r(num);
            break;

        case 0x07:
            ret=leland_vram_planar_r(num);
            break;

        case 0x0a:
            ret=leland_vram_high_r(num);
            break;

        case 0x0b:
            ret=leland_vram_high_r(num);
            leland_video_addr[num]++;
            break;

        case 0x0c:
            ret=leland_vram_low_r(num);
            break;

        default:
            if (errorlog)
            {
                fprintf(errorlog, "CPU #%d %04x Warning: Unknown video port %02x read (address=%04x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset,
                        leland_video_addr[num]);
            }
            return 0;
    }

#ifdef NOISY_VIDEO
    if (errorlog && offset != 0x07)
    {
        fprintf(errorlog, "CPU #%d %04x video port %02x read (address=%04x value=%02x)\n",
                   cpu_getactivecpu(),cpu_get_pc(), offset,
                   leland_video_addr[num], ret);
    }
#endif

    return ret;
}

void ataxx_vram_port_w(int offset, int data, int num)
{
    int ret, i;
    switch (offset)
    {
        case 0x03:
            leland_vram_high_w(data, num);
            leland_vram_low_w(leland_vram_low_data, num);
            leland_video_addr[num]++;
            break;

        case 0x05:
            leland_vram_low_w(data, num);
            leland_vram_high_w(leland_vram_high_data, num);
            leland_video_addr[num]++;
            break;

        case 0x07:
            leland_vram_planar_w(data, num);
            break;

        case 0x0a:
            leland_vram_high_w(data, num);
            break;

        case 0x0b: /* High write with latch */
            leland_vram_high_w(data, num);
            leland_video_addr[num]++;
            break;

        case 0x0c:
            leland_vram_low_w(data, num);
            break;

        case 0x17:
            leland_vram_planar_masked_w(data, num);
            break;

        default:
            if (errorlog)
            {
                fprintf(errorlog, "CPU #%d %04x Warning: Unknown video port %02x write (address=%04x value=%02x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset,
                        leland_video_addr[num], data);
            }
            return;
    }

#ifdef NOISY_VIDEO
    if (errorlog && offset != 0x07)
    {
        fprintf(errorlog, "CPU #%d %04x video port %02x write (address=%04x value=%02x)\n",
                   cpu_getactivecpu(),cpu_get_pc(), offset,
                   leland_video_addr[num], data);
    }
#endif
}

void ataxx_mvram_port_w(int offset, int data)
{
    ataxx_vram_port_w(offset, data, 0);
}

void ataxx_svram_port_w(int offset, int data)
{
    ataxx_vram_port_w(offset, data, 1);
}

int  ataxx_mvram_port_r(int offset)
{
    return ataxx_vram_port_r(offset, 0);
}

int  ataxx_svram_port_r(int offset)
{
    return ataxx_vram_port_r(offset, 1);
}

INLINE void ataxx_build_palette(void)
{
    int offset, pen;
    pen=0;
    for (offset = 0; offset < leland_palette_ram_size; offset+=2)
	{
		int red, green, blue;
        blue  = leland_palette_ram[offset]&0x0f;
        green = (leland_palette_ram[offset]&0xf0)>>4;
        red  = (leland_palette_ram[offset+1] & 0x0f);
        palette_change_color (pen,
                        red*16+red,
                        green*16+green,
                        blue*16+blue);
        palette_used_colors[pen] = PALETTE_COLOR_USED;
        pen++;
	}
	palette_recalc();
}

int ataxx_vh_start(void)
{
    if (leland_vh_start())
    {
        return 1;
    }

    /* This background bitmap technique needs to be moved to
    the generic leland driver (when the bitmap renderer supports
    scrolling)
    */
    background_bitmap = osd_create_bitmap(0x100*8, VIDEO_HEIGHT*8);
    if (!background_bitmap)
	{
		return 1;
	}
    fillbitmap(background_bitmap,0, NULL);

    ataxx_qram1=malloc(ataxx_qram_size);
    if (!ataxx_qram1)
    {
        return 1;
    }
    memset(ataxx_qram1, 0, ataxx_qram_size);

    ataxx_qram2=malloc(ataxx_qram_size);
    if (!ataxx_qram2)
    {
        return 1;
    }
    memset(ataxx_qram2, 0, ataxx_qram_size);
	return 0;
}

void ataxx_vh_stop(void)
{
    leland_vh_stop();

    if (background_bitmap)
    {
        osd_free_bitmap(background_bitmap);
    }
    if (ataxx_qram1)
	{
        free(ataxx_qram1);
	}
    if (ataxx_qram2)
	{
        free(ataxx_qram2);
	}
}

void ataxx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    int x, y, offs;

    /* Build the palette */
    ataxx_build_palette();

    /*
    Rebuild the background bitmap.
    TODO:
    - Support scrolling (if it is used)
    - Dirty tile optimization
    */
    for (y=0; y<VIDEO_HEIGHT; y++)
    {
        for (x=0; x<0x28; x++) /* Should be  */
        {
            int code;
            offs=(y*0x0100)+x;
            offs&=0x1fff;
            code=ataxx_qram1[offs]+(ataxx_qram2[offs]&0x3f)*256;
            leland_draw_bkchar(x, y, code);
        }
    }
    /* Merge the foreground and background layers */
    leland_draw_bitmap(bitmap, background_bitmap);

#if 0
    if (osd_key_pressed(OSD_KEY_B))
    {
        FILE *fp;
        fp=fopen("TRAM.DMP", "w+b");
        if (fp)
        {
            fwrite(ataxx_tram, ataxx_tram_size, 1, fp);
            fclose(fp);
        }
        fp=fopen("QRAM1.DMP", "w+b");
        if (fp)
        {
            fwrite(ataxx_qram1, ataxx_qram_size, 1, fp);
            fclose(fp);
        }
        fp=fopen("QRAM2.DMP", "w+b");
        if (fp)
        {
            fwrite(ataxx_qram2, ataxx_qram_size, 1, fp);
            fclose(fp);
        }
    }
#endif
}










