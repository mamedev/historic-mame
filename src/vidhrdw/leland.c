/***************************************************************************

	Cinemat/Leland driver

	Leland video hardware
	driver by Aaron Giles and Paul Leaman

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"


/* constants */
#define VRAM_SIZE	0x10000
#define QRAM_SIZE	0x10000

#define VIDEO_WIDTH  0x28
#define VIDEO_HEIGHT 0x1e


/* debugging */
#define LOG_COMM	0



struct vram_state_data
{
	UINT16	addr;
	UINT8	latch[2];
};

struct scroll_position
{
	UINT16 	scanline;
	UINT16 	x, y;
	UINT8 	gfxbank;
};


/* video RAM */
static struct osd_bitmap *fgbitmap;
static UINT8 *leland_video_ram;
UINT8 *ataxx_qram;
UINT8 leland_last_scanline_int;

/* video RAM bitmap drawing */
static struct vram_state_data vram_state[2];
static UINT8 sync_next_write;

/* partial screen updating */
static int next_update_scanline;

/* scroll background registers */
static UINT16 xscroll;
static UINT16 yscroll;
static UINT8 gfxbank;
static UINT8 scroll_index;
static struct scroll_position scroll_pos[VIDEO_HEIGHT];


/* sound routines */
extern UINT8 leland_dac_control;

extern void leland_dac_update(int dacnum, UINT8 sample);



/*************************************
 *
 *	Start video hardware
 *
 *************************************/

int leland_vh_start(void)
{
	void leland_vh_stop(void);

	/* allocate memory */
    leland_video_ram = malloc(VRAM_SIZE);
    fgbitmap = bitmap_alloc(VIDEO_WIDTH * 8, VIDEO_HEIGHT * 8);

	/* error cases */
    if (!leland_video_ram || !fgbitmap)
    {
    	leland_vh_stop();
		return 1;
	}

	/* reset videoram */
    memset(leland_video_ram, 0, VRAM_SIZE);

	/* reset scrolling */
	scroll_index = 0;
	memset(scroll_pos, 0, sizeof(scroll_pos));

	return 0;
}


int ataxx_vh_start(void)
{
	void ataxx_vh_stop(void);

	/* first do the standard stuff */
	if (leland_vh_start())
		return 1;

	/* allocate memory */
	ataxx_qram = malloc(QRAM_SIZE);

	/* error cases */
    if (!ataxx_qram)
    {
    	ataxx_vh_stop();
		return 1;
	}

	/* reset QRAM */
	memset(ataxx_qram, 0, QRAM_SIZE);
	return 0;
}



/*************************************
 *
 *	Stop video hardware
 *
 *************************************/

void leland_vh_stop(void)
{
	if (leland_video_ram)
		free(leland_video_ram);
	leland_video_ram = NULL;

	if (fgbitmap)
		bitmap_free(fgbitmap);
	fgbitmap = NULL;
}


void ataxx_vh_stop(void)
{
	leland_vh_stop();

	if (ataxx_qram)
		free(ataxx_qram);
	ataxx_qram = NULL;
}



/*************************************
 *
 *	Scrolling and banking
 *
 *************************************/

WRITE_HANDLER( leland_gfx_port_w )
{
	int scanline = leland_last_scanline_int;
	struct scroll_position *scroll;

	/* treat anything during the VBLANK as scanline 0 */
	if (scanline > Machine->visible_area.max_y)
		scanline = 0;

	/* adjust the proper scroll value */
    switch (offset)
    {
    	case -1:
    		gfxbank = data;
    		break;
		case 0:
			xscroll = (xscroll & 0xff00) | (data & 0x00ff);
			break;
		case 1:
			xscroll = (xscroll & 0x00ff) | ((data << 8) & 0xff00);
			break;
		case 2:
			yscroll = (yscroll & 0xff00) | (data & 0x00ff);
			break;
		case 3:
			yscroll = (yscroll & 0x00ff) | ((data << 8) & 0xff00);
			break;
	}

	/* update if necessary */
	scroll = &scroll_pos[scroll_index];
	if (xscroll != scroll->x || yscroll != scroll->y || gfxbank != scroll->gfxbank)
	{
		/* determine which entry to use */
		if (scroll->scanline != scanline && scroll_index < VIDEO_HEIGHT - 1)
			scroll++, scroll_index++;

		/* fill in the data */
		scroll->scanline = scanline;
		scroll->x = xscroll;
		scroll->y = yscroll;
		scroll->gfxbank = gfxbank;
	}
}



/*************************************
 *
 *	Video address setting
 *
 *************************************/

void leland_video_addr_w(int offset, int data, int num)
{
	struct vram_state_data *state = vram_state + num;

	if (!offset)
		state->addr = (state->addr & 0xfe00) | ((data << 1) & 0x01fe);
	else
		state->addr = ((data << 9) & 0xfe00) | (state->addr & 0x01fe);

	if (num == 0)
		sync_next_write = (state->addr >= 0xf000);
}



/*************************************
 *
 *	Flush data from VRAM into our copy
 *
 *************************************/

static void update_for_scanline(int scanline)
{
	int i, j;

	/* skip if we're behind the times */
	if (scanline <= next_update_scanline)
		return;

	/* update all scanlines */
	for (i = next_update_scanline; i < scanline; i++)
		if (i < VIDEO_HEIGHT * 8)
		{
			UINT8 scandata[VIDEO_WIDTH * 8];
			UINT8 *dst = scandata;
			UINT8 *src = &leland_video_ram[i * 256];

			for (j = 0; j < VIDEO_WIDTH * 8 / 2; j++)
			{
				UINT8 pix = *src++;
				*dst++ = pix >> 4;
				*dst++ = pix & 15;
			}
			draw_scanline8(fgbitmap, 0, i, VIDEO_WIDTH * 8, scandata, NULL, -1);
		}

	/* also update the DACs */
	if (scanline >= VIDEO_HEIGHT * 8)
		scanline = 256;
	for (i = next_update_scanline; i < scanline; i++)
	{
		if (!(leland_dac_control & 0x01))
			leland_dac_update(0, leland_video_ram[i * 256 + 160]);
		if (!(leland_dac_control & 0x02))
			leland_dac_update(1, leland_video_ram[i * 256 + 161]);
	}

	/* set the new last update */
	next_update_scanline = scanline;
}



/*************************************
 *
 *	Common video RAM read
 *
 *************************************/

int leland_vram_port_r(int offset, int num)
{
	struct vram_state_data *state = vram_state + num;
	int addr = state->addr;
	int inc = (offset >> 2) & 2;
    int ret;

    switch (offset & 7)
    {
        case 3:	/* read hi/lo (alternating) */
        	ret = leland_video_ram[addr];
        	addr += inc & (addr << 1);
        	addr ^= 1;
            break;

        case 5:	/* read hi */
		    ret = leland_video_ram[addr | 1];
		    addr += inc;
            break;

        case 6:	/* read lo */
		    ret = leland_video_ram[addr & ~1];
		    addr += inc;
            break;

        default:
            logerror("CPU #%d %04x Warning: Unknown video port %02x read (address=%04x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset, addr);
            ret = 0;
            break;
    }
    state->addr = addr;

	if (LOG_COMM && addr >= 0xf000)
		logerror("%04X:%s comm read %04X = %02X\n", cpu_getpreviouspc(), num ? "slave" : "master", addr, ret);

    return ret;
}



/*************************************
 *
 *	Common video RAM write
 *
 *************************************/

void leland_vram_port_w(int offset, int data, int num)
{
	struct vram_state_data *state = vram_state + num;
	int addr = state->addr;
	int inc = (offset >> 2) & 2;
	int trans = (offset >> 4) & num;

	/* if we're writing "behind the beam", make sure we've cached what was there */
	if (addr < 0xf000)
	{
		int cur_scanline = cpu_getscanline();
		int mod_scanline = addr / 256;

		if (cur_scanline != next_update_scanline && mod_scanline < cur_scanline)
			update_for_scanline(cur_scanline);
	}

	if (LOG_COMM && addr >= 0xf000)
		logerror("%04X:%s comm write %04X = %02X\n", cpu_getpreviouspc(), num ? "slave" : "master", addr, data);

	/* based on the low 3 bits of the offset, update the destination */
    switch (offset & 7)
    {
        case 1:	/* write hi = data, lo = latch */
        	leland_video_ram[addr & ~1] = state->latch[0];
        	leland_video_ram[addr |  1] = data;
        	addr += inc;
        	break;

        case 2:	/* write hi = latch, lo = data */
        	leland_video_ram[addr & ~1] = data;
        	leland_video_ram[addr |  1] = state->latch[1];
        	addr += inc;
        	break;

        case 3:	/* write hi/lo = data (alternating) */
        	if (trans)
        	{
        		if (!(data & 0xf0)) data |= leland_video_ram[addr] & 0xf0;
        		if (!(data & 0x0f)) data |= leland_video_ram[addr] & 0x0f;
        	}
       		leland_video_ram[addr] = data;
        	addr += inc & (addr << 1);
        	addr ^= 1;
            break;

        case 5:	/* write hi = data */
        	state->latch[1] = data;
        	if (trans)
        	{
        		if (!(data & 0xf0)) data |= leland_video_ram[addr | 1] & 0xf0;
        		if (!(data & 0x0f)) data |= leland_video_ram[addr | 1] & 0x0f;
        	}
		    leland_video_ram[addr | 1] = data;
		    addr += inc;
            break;

        case 6:	/* write lo = data */
        	state->latch[0] = data;
        	if (trans)
        	{
        		if (!(data & 0xf0)) data |= leland_video_ram[addr & ~1] & 0xf0;
        		if (!(data & 0x0f)) data |= leland_video_ram[addr & ~1] & 0x0f;
        	}
		    leland_video_ram[addr & ~1] = data;
		    addr += inc;
            break;

        default:
            logerror("CPU #%d %04x Warning: Unknown video port %02x write (address=%04x value=%02x)\n",
                        cpu_getactivecpu(),cpu_get_pc(), offset, addr);
            break;
    }

    /* update the address and plane */
    state->addr = addr;
}



/*************************************
 *
 *	Master video RAM read/write
 *
 *************************************/

WRITE_HANDLER( leland_master_video_addr_w )
{
    leland_video_addr_w(offset, data, 0);
}


static void leland_delayed_mvram_w(int param)
{
	int num = (param >> 16) & 1;
	int offset = (param >> 8) & 0xff;
	int data = param & 0xff;
	leland_vram_port_w(offset, data, num);
}


WRITE_HANDLER( leland_mvram_port_w )
{
	if (sync_next_write)
	{
		timer_set(TIME_NOW, 0x00000 | (offset << 8) | data, leland_delayed_mvram_w);
		sync_next_write = 0;
	}
	else
	    leland_vram_port_w(offset, data, 0);
}


READ_HANDLER( leland_mvram_port_r )
{
    return leland_vram_port_r(offset, 0);
}



/*************************************
 *
 *	Slave video RAM read/write
 *
 *************************************/

WRITE_HANDLER( leland_slave_video_addr_w )
{
    leland_video_addr_w(offset, data, 1);
}

WRITE_HANDLER( leland_svram_port_w )
{
    leland_vram_port_w(offset, data, 1);
}

READ_HANDLER( leland_svram_port_r )
{
    return leland_vram_port_r(offset, 1);
}



/*************************************
 *
 *	Ataxx master video RAM read/write
 *
 *************************************/

WRITE_HANDLER( ataxx_mvram_port_w )
{
	offset = ((offset >> 1) & 0x07) | ((offset << 3) & 0x08) | (offset & 0x10);
	if (sync_next_write)
	{
		timer_set(TIME_NOW, 0x00000 | (offset << 8) | data, leland_delayed_mvram_w);
		sync_next_write = 0;
	}
	else
		leland_vram_port_w(offset, data, 0);
}


WRITE_HANDLER( ataxx_svram_port_w )
{
	offset = ((offset >> 1) & 0x07) | ((offset << 3) & 0x08) | (offset & 0x10);
	leland_vram_port_w(offset, data, 1);
}



/*************************************
 *
 *	Ataxx slave video RAM read/write
 *
 *************************************/

READ_HANDLER( ataxx_mvram_port_r )
{
	offset = ((offset >> 1) & 0x07) | ((offset << 3) & 0x08) | (offset & 0x10);
    return leland_vram_port_r(offset, 0);
}


READ_HANDLER( ataxx_svram_port_r )
{
	offset = ((offset >> 1) & 0x07) | ((offset << 3) & 0x08) | (offset & 0x10);
    return leland_vram_port_r(offset, 1);
}



/*************************************
 *
 *	End-of-frame routine
 *
 *************************************/

static void scanline_reset(int param)
{
	/* flush the remaining scanlines */
	next_update_scanline = 0;

	/* turn off the DACs at the start of the frame */
	leland_dac_control = 3;
}


void leland_vh_eof(void)
{
	/* reset scrolling */
	scroll_index = 0;
	scroll_pos[0].scanline = 0;
	scroll_pos[0].x = xscroll;
	scroll_pos[0].y = yscroll;
	scroll_pos[0].gfxbank = gfxbank;

	/* update anything remaining */
	update_for_scanline(VIDEO_HEIGHT * 8);

	/* set a timer to go off at the top of the frame */
	timer_set(cpu_getscanlinetime(0), 0, scanline_reset);
}



/*************************************
 *
 *	ROM-based refresh routine
 *
 *************************************/

void leland_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	const UINT8 *background_prom = memory_region(REGION_USER1);
	const struct GfxElement *gfx = Machine->gfx[0];
	int x, y, chunk;

	/* update anything remaining */
	update_for_scanline(VIDEO_HEIGHT * 8);

	/* loop over scrolling chunks */
	/* it's okay to do this before the palette calc because */
	/* these values are raw indexes, not pens */
	for (chunk = 0; chunk <= scroll_index; chunk++)
	{
		int char_bank = ((scroll_pos[chunk].gfxbank >> 4) & 0x03) * 0x0400;
		int prom_bank = ((scroll_pos[chunk].gfxbank >> 3) & 0x01) * 0x2000;

		/* determine scrolling parameters */
		int xfine = scroll_pos[chunk].x % 8;
		int yfine = scroll_pos[chunk].y % 8;
		int xcoarse = scroll_pos[chunk].x / 8;
		int ycoarse = scroll_pos[chunk].y / 8;
		struct rectangle clip;

		/* make a clipper */
		clip = Machine->visible_area;
		if (chunk != 0)
			clip.min_y = scroll_pos[chunk].scanline;
		if (chunk != scroll_index)
			clip.max_y = scroll_pos[chunk + 1].scanline - 1;

		/* draw what's visible to the main bitmap */
		for (y = clip.min_y / 8; y < clip.max_y / 8 + 2; y++)
		{
			int ysum = ycoarse + y;
			for (x = 0; x < VIDEO_WIDTH + 1; x++)
			{
				int xsum = xcoarse + x;
				int offs = ((xsum << 0) & 0x000ff) |
				           ((ysum << 8) & 0x01f00) |
				           prom_bank |
				           ((ysum << 9) & 0x1c000);
				int code = background_prom[offs] |
				           ((ysum << 2) & 0x300) |
				           char_bank;
				int color = (code >> 5) & 7;

				/* draw to the bitmap */
				drawgfx(bitmap, gfx,
						code, 8 * color, 0, 0,
						8 * x - xfine, 8 * y - yfine,
						&clip, TRANSPARENCY_NONE_RAW, 0);
			}
		}
	}

	/* Merge the two bitmaps together */
	copybitmap(bitmap, fgbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_BLEND, 6);
}



/*************************************
 *
 *	RAM-based refresh routine
 *
 *************************************/

void ataxx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	int x, y, chunk;

	/* update anything remaining */
	update_for_scanline(VIDEO_HEIGHT * 8);

	/* loop over scrolling chunks */
	/* it's okay to do this before the palette calc because */
	/* these values are raw indexes, not pens */
	for (chunk = 0; chunk <= scroll_index; chunk++)
	{
		/* determine scrolling parameters */
		int xfine = scroll_pos[chunk].x % 8;
		int yfine = scroll_pos[chunk].y % 8;
		int xcoarse = scroll_pos[chunk].x / 8;
		int ycoarse = scroll_pos[chunk].y / 8;
		struct rectangle clip;

		/* make a clipper */
		clip = Machine->visible_area;
		if (chunk != 0)
			clip.min_y = scroll_pos[chunk].scanline;
		if (chunk != scroll_index)
			clip.max_y = scroll_pos[chunk + 1].scanline - 1;

		/* draw what's visible to the main bitmap */
		for (y = clip.min_y / 8; y < clip.max_y / 8 + 2; y++)
		{
			int ysum = ycoarse + y;
			for (x = 0; x < VIDEO_WIDTH + 1; x++)
			{
				int xsum = xcoarse + x;
				int offs = ((ysum & 0x40) << 9) + ((ysum & 0x3f) << 8) + (xsum & 0xff);
				int code = ataxx_qram[offs] | ((ataxx_qram[offs + 0x4000] & 0x7f) << 8);

				/* draw to the bitmap */
				drawgfx(bitmap, gfx,
						code, 0, 0, 0,
						8 * x - xfine, 8 * y - yfine,
						&clip, TRANSPARENCY_NONE_RAW, 0);
			}
		}
	}

	/* Merge the two bitmaps together */
	copybitmap(bitmap, fgbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_BLEND, 6);
}
