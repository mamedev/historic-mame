/***********************************************************************************************

	Sega System C/C2 Driver
	-----------------------
	Version 0.52 - 5 March 2001
	(for changes see drivers\segac2.c)

***********************************************************************************************/

#include "driver.h"
#include "state.h"

/******************************************************************************
	Macros
******************************************************************************/

#define BITMAP_WIDTH		336
#define BITMAP_HEIGHT		240

#define VRAM_SIZE			0x10000
#define VRAM_MASK			(VRAM_SIZE - 1)
#define VSRAM_SIZE			0x80
#define VSRAM_MASK			(VSRAM_SIZE - 1)

#define VDP_VRAM_BYTE(x)	(vdp_vram[(x) & VRAM_MASK])
#define VDP_VSRAM_BYTE(x)	(vdp_vsram[(x) & VSRAM_MASK])
#define VDP_VRAM_WORD(x)	((VDP_VRAM_BYTE(x) << 8) | VDP_VRAM_BYTE((x) + 1))
#define VDP_VSRAM_WORD(x)	((VDP_VSRAM_BYTE(x) << 8) | VDP_VSRAM_BYTE((x) + 1))

#ifdef LSB_FIRST
#define PIXEL_XOR_BE(x)		((x) ^ 6)
#else
#define PIXEL_XOR_BE(x)		(x)
#endif



/******************************************************************************
	Function Prototypes
******************************************************************************/

static int  vdp_data_r(void);
static void vdp_data_w(int data);
static int  vdp_control_r(void);
static void vdp_control_w(int data);
static void vdp_register_w(int data);
static void vdp_control_dma(int data);
static void vdp_dma_68k(void);
static void vdp_dma_fill(int);
static void vdp_dma_copy(void);

static void drawline(UINT16 *bitmap, int line);
static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, UINT8 *offset);
static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles, UINT8 *offset);
static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri);
static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase);



/******************************************************************************
	Global variables
******************************************************************************/

/* EXTERNALLY ACCESSIBLE */
       int			segac2_bg_palbase;			/* base of background palette */
       int			segac2_sp_palbase;			/* base of sprite palette */
       int			segac2_palbank;				/* global palette bank */
       UINT8		segac2_vdp_regs[32];		/* VDP registers */

/* LOCAL */
static UINT8 *		vdp_vram;					/* VDP video RAM */
static UINT8 *		vdp_vsram;					/* VDP vertical scroll RAM */
static UINT8		display_enable;				/* is the display enabled? */

/* updates */
static int			last_update_scanline;		/* last scanline we drew */
static UINT16 *		cache_bitmap;				/* 16bpp bitmap with raw pen values */
static UINT8		internal_vblank;			/* state of the VBLANK line */
static UINT16 *		transparent_lookup;			/* fast transparent mapping table */

/* vram bases */
static UINT32		vdp_scrollabase;			/* base address of scroll A tiles */
static UINT32		vdp_scrollbbase;			/* base address of scroll B tiles */
static UINT32		vdp_windowbase;				/* base address of window tiles */
static UINT32		vdp_spritebase;				/* base address of sprite data */
static UINT32		vdp_hscrollbase;			/* base address of H scroll values */

/* other vdp variables */
static int			vdp_hscrollmask;			/* mask for H scrolling */
static UINT32		vdp_hscrollsize;			/* size of active H scroll values */
static UINT8		vdp_vscrollmode;			/* current V scrolling mode */

static UINT8		vdp_cmdpart;				/* partial command flag */
static UINT8		vdp_code;					/* command code value */
static UINT32		vdp_address;				/* current I/O address */
static UINT8		vdp_dmafill;				/* DMA filling flag */
static UINT8		scrollheight;				/* height of the scroll area in tiles */
static UINT8		scrollwidth;				/* width of the scroll area in tiles */
static UINT8		bgcol;						/* current background color */
static UINT8		window_down;				/* window direction */
static UINT32		window_vpos;				/* window Y position */



/******************************************************************************
	Video Start / Stop Functions
*******************************************************************************

	Here we allocate memory used by the VDP and various other video related parts
	of the System C/C2 hardware such as the Palette RAM. Here is what is needed

	64kb of VRAM (multi-purpose, storing tiles, tilemaps, hscroll data,
					spritelist etc.)

	80bytes of VSRAM (used exclusively for storing Vertical Scroll values)

******************************************************************************/

int segac2_vh_start(void)
{
	static const UINT8 vdp_init[24] =
	{
		0x04, 0x44, 0x30, 0x3C, 0x07, 0x6C, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0x00, 0x01, 0x37, 0x00, 0x02,
		0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x80,
	};
	int i;

	/* allocate memory for the VDP, the lookup table, and the buffer bitmap */
	vdp_vram			= malloc(VRAM_SIZE);
	vdp_vsram			= malloc(VSRAM_SIZE);
	transparent_lookup	= malloc(0x1000 * sizeof(UINT16));
	cache_bitmap		= malloc(BITMAP_WIDTH * BITMAP_HEIGHT * sizeof(UINT16));

	/* check for errors */
	if (!vdp_vram || !vdp_vsram || !transparent_lookup || !cache_bitmap)
		return 1;

	/* clear the VDP memory, prevents corrupt tile in Puyo Puyo 2 */
	memset(vdp_vram, 0, VRAM_SIZE);
	memset(vdp_vsram, 0, VSRAM_SIZE);

	/* init transparency table */
	for (i = 0; i < 0x1000; i++)
	{
		int orig_color = i & 0x7ff;
		int half_bright = i & 0x800;

		if (orig_color & 0x100)
			transparent_lookup[i] = orig_color;
		else if (half_bright)
			transparent_lookup[i] = orig_color | 0x800;
		else
			transparent_lookup[i] = orig_color | 0x1000;
	}

	/* reset the palettes */
	memset(paletteram16, 0, 0x800 * sizeof(data16_t));
	segac2_bg_palbase = 0x000;
	segac2_sp_palbase = 0x100;
	segac2_palbank    = 0x000;

	/* reset VDP */
	internal_vblank = 1;
    for (i = 0; i < 24; i++)
        vdp_register_w(0x8000 | (i << 8) | vdp_init[i]);
	vdp_cmdpart = 0;
	vdp_code    = 0;
	vdp_address = 0;

	/* reset buffer */
	last_update_scanline = 0;

	/* Save State Stuff we could probably do with an init values from vdp registers function or something (todo) */

	state_save_register_UINT8 ("C2_VDP", 0, "VDP Registers", segac2_vdp_regs, 32);
	state_save_register_UINT8 ("C2_VDP", 0, "VDP VRam", vdp_vram, 0x10000);
	state_save_register_UINT8 ("C2_VDP", 0, "VDP VSRam", vdp_vsram, 0x80);
	state_save_register_int("C2_Video", 0, "Palette Bank", &segac2_palbank);
	state_save_register_int("C2_Video", 0, "Background Pal Base",  &segac2_bg_palbase);
	state_save_register_int("C2_Video", 0, "Sprite Pal Base",  &segac2_sp_palbase);
	state_save_register_UINT8("C2_Video", 0, "Display Enabled",  &display_enable, 1);
	state_save_register_UINT32("C2_Video", 0, "Scroll A Base in VRAM",  &vdp_scrollabase, 1);
	state_save_register_UINT32("C2_Video", 0, "Scroll B Base in VRAM",  &vdp_scrollbbase, 1);
	state_save_register_UINT32("C2_Video", 0, "Window Base in VRAM",  &vdp_windowbase, 1);
	state_save_register_UINT32("C2_Video", 0, "Sprite Table Base in VRAM",  &vdp_spritebase, 1);
	state_save_register_UINT32("C2_Video", 0, "HScroll Data Base in VRAM",  &vdp_hscrollbase, 1);
	state_save_register_int("C2_Video", 0, "vdp_hscrollmask",  &vdp_hscrollmask);
	state_save_register_UINT32("C2_Video", 0, "vdp_hscrollsize",  &vdp_hscrollsize, 1);
	state_save_register_UINT8("C2_Video", 0, "vdp_vscrollmode",  &vdp_vscrollmode, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP Command Part",  &vdp_cmdpart, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP Current Code",  &vdp_code, 1);
	state_save_register_UINT32("C2_VDP", 0, "VDP Address",  &vdp_address, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP DMA Mode",  &vdp_dmafill, 1);
	state_save_register_UINT8("C2_Video", 0, "scrollheight",  &scrollheight, 1);
	state_save_register_UINT8("C2_Video", 0, "scrollwidth",  &scrollwidth, 1);
	state_save_register_UINT8("C2_Video", 0, "Background Colour",  &bgcol, 1);
	state_save_register_UINT8("C2_Video", 0, "Window Horz",  &window_down, 1);
	state_save_register_UINT32("C2_Video", 0, "Window Vert",  &window_vpos, 1);

	return 0;

}


void segac2_vh_stop(void)
{
	free(cache_bitmap);
	free(transparent_lookup);
	free(vdp_vram);
	free(vdp_vsram);
}



/******************************************************************************
	VBLANK routines
*******************************************************************************

	These callbacks are used to track the state of VBLANK. At the end of
	VBLANK, all the palette information is reset and updates to the cache
	bitmap are enabled.

******************************************************************************/

/* timer callback for the end of VBLANK */
static void vblank_end(int param)
{
	/* reset update scanline */
	last_update_scanline = 0;

	/* reset VBLANK flag */
	internal_vblank = 0;
}


/* end-of-frame callback to mark the start of VBLANK */
void segac2_vh_eof(void)
{
	/* set VBLANK flag */
	internal_vblank = 1;

	/* set a timer for VBLANK off */
	timer_set(cpu_getscanlinetime(0), 0, vblank_end);
}



/******************************************************************************
	Screen Refresh Functions
*******************************************************************************

	These are responsible for the core drawing. The update_display function
	can be called under several circumstances to cache all the currently
	displayed lines before a significant palette or scrolling change is
	set to occur. The actual refresh routine marks the accumulated palette
	entries and then converts the raw pens in the cache bitmap to their
	final remapped values.

******************************************************************************/

/* update the cached bitmap to the given scanline */
void segac2_update_display(int scanline)
{
	int line;

	/* don't bother if we're skipping */
	if (osd_skip_this_frame())
		return;

	/* clamp to the screen */
	if (scanline < 0)
		scanline = 0;
	if (scanline > 224)
		scanline = 224;
	if (last_update_scanline >= scanline)
		return;

	/* update all relevant lines */
	for (line = last_update_scanline; line < scanline; line++)
		drawline(&cache_bitmap[line * BITMAP_WIDTH], line);
	last_update_scanline = scanline;
}


/* set the display enable bit */
void segac2_enable_display(int enable)
{
	if (!internal_vblank)
		segac2_update_display(cpu_getscanline());
	display_enable = enable;
}


/* core refresh: computes the final screen */
void segac2_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int y;

	/* finish updating the display */
	segac2_update_display(224);

	/* generate the final screen */
	for (y = 0; y < 224; y++)
		draw_scanline16(bitmap, 8, y, 320, &cache_bitmap[y * BITMAP_WIDTH + 8], Machine->pens, -1);
}



/******************************************************************************
	VDP Read & Write Handlers
*******************************************************************************

	The VDP is accessed via 8 & 16 bit reads / writes at the addresses 0xC00000 -
	0xC0001F.  Different locations have different functions in the following
	layout.  (based on Information in Charles MacDonalds Document)

	0xC00000 -   DATA port (8 or 16 bit, Read or Write)
	0xC00002 -   mirror of above
	0xC00004 -   CTRL port (8 or 16 bit, Read or Write)
	0xC00006 -   mirror of above
	0xC00008 -   HV counter (8 or 16 bit Read Only)
	0xC0000A -   mirror of above
	0xC0000C -   mirror of above
	0xC0000E -   mirror of above
	0xC00010 -   SN76489 (8 bit Write Only)
	0xC00012 -   mirror of above
	0xC00014 -   mirror of above
	0xC00016 -   mirror of above

	The SN76489 PSG Writes are intercepted before we get here and handled
	directly.

******************************************************************************/

READ16_HANDLER( segac2_vdp_r )
{
	switch (offset)
	{
		case 0x00:	/* Read Data */
		case 0x01:
			return vdp_data_r();

		case 0x02:	/* Status Register */
		case 0x03:
			return vdp_control_r();

		case 0x04:	/* HV counter */
		case 0x05:
		case 0x06:
		case 0x07:
		{
			int xpos = cpu_gethorzbeampos();
			int ypos = cpu_getscanline();

			/* adjust for the weird counting rules */
			if (xpos > 0xe9) xpos -= (342 - 0x100);
			if (ypos > 0xea) ypos -= (262 - 0x100);

			/* kludge for ichidant: it sets the H counter to 160, then */
			/* expects that a read from here will return 159 */
			if (ypos > 0) ypos -= 2;
			return (ypos << 8) | xpos;
		}
	}
	return 0;
}


WRITE16_HANDLER( segac2_vdp_w )
{
	switch (offset)
	{
		case 0x00:	/* Write data */
		case 0x01:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
				 	data |= data >> 8;
				 else
				 	data |= data << 8;
			}
			vdp_data_w(data);
			break;

		case 0x02:	/* Control Write */
		case 0x03:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
				 	data |= data >> 8;
				 else
				 	data |= data << 8;
			}
			vdp_control_w(data);
			break;
	}
}



/******************************************************************************
	VDP DATA Reads / Writes (Accesses to 0xC00000 - 0xC00003)
*******************************************************************************

	The functions here are called by the read / write handlers for the VDP.
	They deal with Data Reads and Writes to and from the VDP Chip.

	Read / write mode is set by Writes to VDP Control, Can't Read when in
	write mode or vice versa.

******************************************************************************/

/* Games needing Read to Work .. bloxeed (attract) .. puyo puyo .. probably more */
static int vdp_data_r(void)
{
	int read = 0;

	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* which memory is based on the code */
	switch (vdp_code & 0x0f)
	{
		case 0x00:		/* VRAM read */
			read = VDP_VRAM_WORD(vdp_address & ~1);
			break;

		case 0x04:		/* VSRAM read */
			read = VDP_VSRAM_WORD(vdp_address & ~1);
			break;

		default:		/* Illegal read attempt */
			logerror("%06x: VDP illegal read type %02x\n", cpu_getpreviouspc(), vdp_code);
			read = 0x00;
			break;
	}

	/* advance the address */
	vdp_address += segac2_vdp_regs[15];
	return read;
}


static void vdp_data_w(int data)
{
	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* handle the fill case */
	if (vdp_dmafill)
	{
		vdp_dma_fill(data);
		vdp_dmafill = 0;
		return;
	}

	/* which memory is based on the code */
	switch (vdp_code & 0x0f)
	{
		case 0x01:		/* VRAM write */

			/* if the hscroll RAM is changing during screen refresh, force an update */
			if (!internal_vblank &&
				vdp_address >= vdp_hscrollbase &&
				vdp_address < vdp_hscrollbase + vdp_hscrollsize)
				segac2_update_display(cpu_getscanline());

			/* write to VRAM */
			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VRAM_BYTE(vdp_address |  1) = data;
			break;

		case 0x05:		/* VSRAM write */

			/* if the vscroll RAM is changing during screen refresh, force an update */
			if (!internal_vblank)
				segac2_update_display(cpu_getscanline());

			/* write to VSRAM */
			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VSRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VSRAM_BYTE(vdp_address |  1) = data;
			break;

		default:		/* Illegal write attempt */
			logerror("PC:%06x: VDP illegal write type %02x data %04x\n", cpu_getpreviouspc(), vdp_code, data);
			break;
	}

	/* advance the address */
	vdp_address += segac2_vdp_regs[15];
}



/******************************************************************************
	VDP CTRL Reads / Writes (Accesses to 0xC00004 - 0xC00007)
*******************************************************************************

	A Read from the Control Port will return the Status Register Value.
	16-bits are used to report the VDP status
	|  0     |  0     |  1     |  1     |  0     |  1     |  FIFE  |  FIFF  |
	|  VIP   |  SOF   |  SCL   |  ODD   |  VBLK  |  HBLK  |  DMA   |  PAL   |

		0,1 = Set Values
		FIFE = FIFO Empty
		FIFF = FIFO Full
		VIP  = Vertical Interrupt Pending
		SOF  = Sprite Overflow  (Not used in C2 afaik)
		SCL  = Sprite Collision (Not used in C2 afaik)
		ODD  = Odd Frame (Interlace Mode) (Not used in C2 afaik)
		VBLK = In Vertical Blank
		HBLK = In Horizontal Blank
		DMA  = DMA In Progress
		PAL  = Pal Mode Flag

	Control Writes are used for setting VDP Registers, setting up DMA Transfers
	etc.

	A Write to the Control port can consist of 2 16-bit Words.
	When the VDP _isn't_ expecting the 2nd part of a command the highest 2 bits
	of the data written will determine what it is we want to do.
	10xxxxxx xxxxxxxx will cause a register to be set
	anything else will trigger the 1st half of a 2 Part Mode Setting Command
	If the VDP is already expecting the 2nd half of a command the data written
	will be the 2nd half of the Mode setup.

******************************************************************************/

static int vdp_control_r(void)
{
	int beampos = cpu_gethorzbeampos();
	int status = 0x3400;

	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* set the VBLANK bit */
	if (internal_vblank)
		status |= 0x0008;

	/* set the HBLANK bit */
	if (beampos < Machine->visible_area.min_x || beampos > Machine->visible_area.max_x)
		status |= 0x0004;

	return (status);
}


static void vdp_control_w(int data)
{
	/* case 1: we're not expecting the 2nd half of a command */
	if (!vdp_cmdpart)
	{
		/* if 10xxxxxx xxxxxxxx this is a register setting command */
		if ((data & 0xc000) == 0x8000)
			vdp_register_w(data);

		/* otherwise this is the First part of a mode setting command */
		else
		{
			vdp_code    = (vdp_code & 0x3c) | ((data >> 14) & 0x03);
			vdp_address = (vdp_address & 0xc000) | (data & 0x3fff);
			vdp_cmdpart = 1;
		}
	}

	/* case 2: this is the 2nd part of a mode setting command */
	else
	{
		vdp_code    = (vdp_code & 0x03) | ((data >> 2) & 0x3c);
		vdp_address = (vdp_address & 0x3fff) | ((data << 14) & 0xc000);
		vdp_cmdpart = 0;
		vdp_control_dma(data);
	}
}


static void vdp_register_w(int data)
{
	static const UINT8 is_important[32] = { 0,0,1,1,1,1,0,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	UINT8 regnum = (data & 0x1f00) >> 8; /* ---R RRRR ---- ---- */
	UINT8 regdat = (data & 0x00ff);      /* ---- ---- DDDD DDDD */

	segac2_vdp_regs[regnum] = regdat;

	/* these are mostly important writes; force an update if they */
	/* are written during a screen refresh */
	if (!internal_vblank && is_important[regnum])
		segac2_update_display(cpu_getscanline());

	/* For quite a few of the registers its a good idea to set a couple of variable based
	   upon the writes here */
	switch (regnum)
	{
		case 0x01: /* video modes */
			if (regdat & 8)
				usrintf_showmessage("Video height = 240!");
			break;

		case 0x02: /* Scroll A Name Table Base */
			vdp_scrollabase = (regdat & 0x38) << 10;
			break;

		case 0x03: /* Window Name Table Base */
			vdp_windowbase = (regdat & 0x3e) << 10;
			break;

		case 0x04: /* Scroll B Name Table Base */
			vdp_scrollbbase = (regdat & 0x07) << 13;
			break;

		case 0x05: /* Sprite Table Base */
			vdp_spritebase = (regdat & 0x7e) << 9;
			break;

		case 0x07: /* BG Colour */
			bgcol = regdat & 0x0f;
			break;

		case 0x0b: /* Scroll Modes */
		{
			static const UINT16 mask_table[4] = { 0x000, 0x007, 0xff8, 0xfff };
			vdp_vscrollmode = (regdat & 0x04) >> 2;

			vdp_hscrollmask = mask_table[regdat & 3];
			vdp_hscrollsize = 4 * ((vdp_hscrollmask < 224) ? (vdp_hscrollmask + 1) : 224);
			break;
		}

		case 0x0c: /* video modes */
			if (!(regdat & 1))
				usrintf_showmessage("Video width = 256!");
			break;

		case 0x0d: /* HScroll Base */
			vdp_hscrollbase = (regdat & 0x3f) << 10;
			break;

		case 0x10: /* Scroll Size */
		{
			static const UINT8 size_table[4] = { 32, 64, 128, 128 };
			scrollwidth = size_table[regdat & 0x03];
			scrollheight = size_table[(regdat & 0x30) >> 4];
			break;
		}

		case 0x11: /* Window H Position .. Doesn't Matter for any of the C2 Games */
			break;

		case 0x12: /* Window V Position */
			window_down = regdat & 0x80;
			window_vpos = (regdat & 0x1f) << 3;
			break;
	}
}


static void vdp_control_dma(int data)
{
	if ((vdp_code & 0x20) && (segac2_vdp_regs[1] & 0x10))
	{
		switch(segac2_vdp_regs[23] & 0xc0)
		{
			case 0x00:
			case 0x40:		/* 68k -> VRAM Tranfser */
				vdp_dma_68k();
				break;

			case 0x80:		/* VRAM fill, can't be done here, requires data write */
				vdp_dmafill = 1;
				break;

			case 0xC0:		/* VRAM copy */
				vdp_dma_copy();
				break;
		}
	}
}



/******************************************************************************
	DMA handling
*******************************************************************************

	These are currently Pretty much directly from the C2 emu

******************************************************************************/

static void vdp_dma_68k(void)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int source = (segac2_vdp_regs[21] << 1) | (segac2_vdp_regs[22] << 9) | ((segac2_vdp_regs[23] & 0x7f) << 17);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the DMA */
	for (count = 0; count < length; count++)
	{
		vdp_data_w(cpu_readmem24bew_word(source));
		source += 2;
	}
}


static void vdp_dma_copy(void)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int source = segac2_vdp_regs[21] | (segac2_vdp_regs[22] << 8);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the DMA */
	for (count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address) = VDP_VRAM_BYTE(source++);
		vdp_address += segac2_vdp_regs[15];
	}
}


static void vdp_dma_fill(int data)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the fill */
	VDP_VRAM_BYTE(vdp_address) = data;
	data >>= 8;
	for(count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address ^ 1) = data;
		vdp_address += segac2_vdp_regs[15];
	}
}



/******************************************************************************
	Scroll calculations
*******************************************************************************

	These special routines handle the transparency effects in sprites.

******************************************************************************/

/*** Useful Little Functions **************************************************/

/* Note: We Expect plane = 0 for Scroll A, plane = 2 for Scroll B */
INLINE int vdp_gethscroll(int plane, int line)
{
	line &= vdp_hscrollmask;
	return 0x400 - (VDP_VRAM_WORD(vdp_hscrollbase + (4 * line) + plane) & 0x3ff);
}


/* Note: We expect plane = 0 for Scroll A, plane = 2 for Scroll B
   A Column is 8 Pixels Wide                                     */
static int vdp_getvscroll(int plane, int column)
{
	UINT32 vsramoffset;

	switch (vdp_vscrollmode)
	{
		case 0x00: /* Overall Scroll */
			return VDP_VSRAM_WORD(plane) & 0x7ff;

		case 0x01: /* Column Scroll */
			if (column == 40) column = 39; /* Fix Minor Innacuracy Only affects PotoPoto */
			vsramoffset = (4 * (column >> 1)) + plane;
			return VDP_VSRAM_WORD(vsramoffset) & 0x7ff;
	}
	return 0;
}



/******************************************************************************
	Drawing Functions
*******************************************************************************

	These are used by the Screen Refresh functions to do the actual rendering
	of a screenline to Machine->scrbitmap.

	Draw Planes in Order
		Scroll B Low
		Scroll A Low / Window Low
		Sprites Low
		Scroll B High
		Scroll A High / Window High
		Sprites High

	NOTE: Low Sprites _can_ overlap High sprites, however none of the C2
			games do this ever so its safe to draw them in this order.

	NOTE2: C2 Games Only Ever Split the Screen Horizontally with the Window so
			its safe to draw a line entirely of Scroll A or Window without any
			graphical glitches.

******************************************************************************/

static void drawline(UINT16 *bitmap, int line)
{
	int lowsprites, highsprites, link;
	UINT32 scrolla_tiles[41], scrollb_tiles[41];
	UINT8 scrolla_offset, scrollb_offset;
	UINT8 *lowlist[81], *highlist[81];
	int column, sprite;
	int bgcolor = bgcol + segac2_palbank;

	/* clear to the background color */
	for (column = 8; column < 328; column++)
		bitmap[column] = bgcolor;

	/* if display is disabled, stop */
	if (!(segac2_vdp_regs[1] & 0x40) || !display_enable)
		return;

	/* Sprites need to be Drawn in Reverse order .. may as well sort them here */
	link = lowsprites = highsprites = 0;
	for (sprite = 0; sprite < 80; sprite++)
	{
		UINT8 *spritebase = &VDP_VRAM_BYTE(vdp_spritebase + 8 * link);

		/* sort into high/low priorities */
		if (spritebase[4] & 0x0080)
			highlist[++highsprites] = spritebase;
		else
			lowlist[++lowsprites] = spritebase;

		/* get the link; if 0, stop processing */
		link = spritebase[3] & 0x7F;
		if (!link)
			break;
	}

	/* get tiles for the B scroll layer */
	get_scroll_tiles(line, 2, vdp_scrollbbase, scrollb_tiles, &scrollb_offset);

	/* get tiles for the A scroll layer */
	if ((line < window_vpos && window_down == 0x80) || (line >= window_vpos && window_down == 0))
		get_scroll_tiles(line, 0, vdp_scrollabase, scrolla_tiles, &scrolla_offset);
	else
		get_window_tiles(line, vdp_windowbase, scrolla_tiles, &scrolla_offset);

	/* Scroll B Low */
	drawline_tiles(scrollb_tiles, bitmap + scrollb_offset, 0);

	/* Scroll A Low */
	drawline_tiles(scrolla_tiles, bitmap + scrolla_offset, 0);

	/* Sprites Low */
	for (sprite = lowsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 0, lowlist[sprite]);

	/* Scroll B High */
	drawline_tiles(scrollb_tiles, bitmap + scrollb_offset, 1);

	/* Scroll A High */
	drawline_tiles(scrolla_tiles, bitmap + scrolla_offset, 1);

	/* Sprites High */
	for (sprite = highsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 1, highlist[sprite]);
}



/******************************************************************************
	Background rendering
*******************************************************************************

	This code handles all the scroll calculations and rendering of the
	background layers.

******************************************************************************/

/* determine the tiles we will draw on a scrolling layer */
static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, UINT8 *offset)
{
	int linehscroll = vdp_gethscroll(scrollnum, line);
	int column;

	/* adjust for partial tiles and then pre-divide hscroll to get the tile offset */
	*offset = 8 - (linehscroll % 8);
	linehscroll /= 8;

	/* loop over columns */
	for (column = 0; column < 41; column++)
	{
		int columnvscroll = vdp_getvscroll(scrollnum, column) + line;

		/* determine the base of the tilemap row */
		int temp = ((columnvscroll / 8) & (scrollheight - 1)) * scrollwidth;
		int tilebase = scrollbase + 2 * temp;

		/* offset into the tilemap based on the column */
		temp = (linehscroll + column) & (scrollwidth - 1);
		tilebase += 2 * temp;

		/* get the tile info */
		*tiles++ = ((columnvscroll % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}


/* determine the tiles we will draw on a non-scrolling window layer */
static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles, UINT8 *offset)
{
	int column;

	/* set the offset */
	*offset = 8;

	/* loop over columns */
	for (column = 0; column < 40; column++)
	{
		/* determine the base of the tilemap row */
		int temp = (line / 8) * 64 + column;
		int tilebase = scrollbase + 2 * temp;

		/* get the tile info */
		*tiles++ = ((line % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}


/* draw a line of tiles */
static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri)
{
	int column;

	/* loop over columns */
	for (column = 0; column < 41; column++, bmap += 8)
	{
		UINT32 tile = *tiles++;

		/* if the tile is the correct priority, draw it */
		if (((tile >> 15) & 1) == pri)
		{
			int colbase = 16 * ((tile & 0x6000) >> 13) + segac2_bg_palbase + segac2_palbank;
			UINT32 *tp = (UINT32 *)&VDP_VRAM_BYTE((tile & 0x7ff) * 32);
			UINT32 mytile;
			int col;

			/* vertical flipping */
			if (!(tile & 0x1000))
				mytile = tp[tile >> 16];
			else
				mytile = tp[(tile >> 16) ^ 7];

			/* skip if all-transparent */
			if (!mytile)
				continue;

			/* non-flipped */
			if (!(tile & 0x0800))
			{
				col = (mytile >> 28) & 0x0f; if (col) bmap[PIXEL_XOR_BE(0)] = colbase + col;
				col = (mytile >> 24) & 0x0f; if (col) bmap[PIXEL_XOR_BE(1)] = colbase + col;
				col = (mytile >> 20) & 0x0f; if (col) bmap[PIXEL_XOR_BE(2)] = colbase + col;
				col = (mytile >> 16) & 0x0f; if (col) bmap[PIXEL_XOR_BE(3)] = colbase + col;
				col = (mytile >> 12) & 0x0f; if (col) bmap[PIXEL_XOR_BE(4)] = colbase + col;
				col = (mytile >> 8)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(5)] = colbase + col;
				col = (mytile >> 4)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(6)] = colbase + col;
				col = (mytile >> 0)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(7)] = colbase + col;
			}

			/* horizontal flip */
			else
			{
				col = (mytile >> 28) & 0x0f; if (col) bmap[PIXEL_XOR_BE(7)] = colbase + col;
				col = (mytile >> 24) & 0x0f; if (col) bmap[PIXEL_XOR_BE(6)] = colbase + col;
				col = (mytile >> 20) & 0x0f; if (col) bmap[PIXEL_XOR_BE(5)] = colbase + col;
				col = (mytile >> 16) & 0x0f; if (col) bmap[PIXEL_XOR_BE(4)] = colbase + col;
				col = (mytile >> 12) & 0x0f; if (col) bmap[PIXEL_XOR_BE(3)] = colbase + col;
				col = (mytile >> 8)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(2)] = colbase + col;
				col = (mytile >> 4)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(1)] = colbase + col;
				col = (mytile >> 0)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(0)] = colbase + col;
			}
		}
	}
}



/******************************************************************************
	Sprite drawing
*******************************************************************************

	These special routines handle the transparency effects in sprites.

******************************************************************************/

/* draw a non-horizontally-flipped section of a sprite */
INLINE void draw8pixs(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	/* skip if all-transparent */
	if (!tile)
		return;

	/* non-transparent */
	if ((colbase & 0x30) != 0x30 || !(segac2_vdp_regs[12] & 0x08))
	{
		col = (tile >> 28) & 0x0f; if (col) bmap[PIXEL_XOR_BE(0)] = colbase + col;
		col = (tile >> 24) & 0x0f; if (col) bmap[PIXEL_XOR_BE(1)] = colbase + col;
		col = (tile >> 20) & 0x0f; if (col) bmap[PIXEL_XOR_BE(2)] = colbase + col;
		col = (tile >> 16) & 0x0f; if (col) bmap[PIXEL_XOR_BE(3)] = colbase + col;
		col = (tile >> 12) & 0x0f; if (col) bmap[PIXEL_XOR_BE(4)] = colbase + col;
		col = (tile >> 8)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(5)] = colbase + col;
		col = (tile >> 4)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(6)] = colbase + col;
		col = (tile >> 0)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(7)] = colbase + col;
	}

	/* transparent */
	else
	{
		col = (tile >> 28) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(0)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(0)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(0)] & 0x7ff)];
			}
		}
		col = (tile >> 24) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(1)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(1)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(1)] & 0x7ff)];
			}
		}
		col = (tile >> 20) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(2)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(2)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(2)] & 0x7ff)];
			}
		}
		col = (tile >> 16) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(3)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(3)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(3)] & 0x7ff)];
			}
		}
		col = (tile >> 12) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(4)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(4)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(4)] & 0x7ff)];
			}
		}
		col = (tile >>  8) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(5)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(5)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(5)] & 0x7ff)];
			}
		}
		col = (tile >>  4) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(6)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(6)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(6)] & 0x7ff)];
			}
		}
		col = (tile >>  0) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(7)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(7)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(7)] & 0x7ff)];
			}
		}
	}
}


/* draw a horizontally-flipped section of a sprite */
INLINE void draw8pixs_hflip(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	/* skip if all-transparent */
	if (!tile)
		return;

	/* non-transparent */
	if ((colbase & 0x30) != 0x30 || !(segac2_vdp_regs[12] & 0x08))
	{
		col = (tile >> 28) & 0x0f; if (col) bmap[PIXEL_XOR_BE(7)] = colbase + col;
		col = (tile >> 24) & 0x0f; if (col) bmap[PIXEL_XOR_BE(6)] = colbase + col;
		col = (tile >> 20) & 0x0f; if (col) bmap[PIXEL_XOR_BE(5)] = colbase + col;
		col = (tile >> 16) & 0x0f; if (col) bmap[PIXEL_XOR_BE(4)] = colbase + col;
		col = (tile >> 12) & 0x0f; if (col) bmap[PIXEL_XOR_BE(3)] = colbase + col;
		col = (tile >> 8)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(2)] = colbase + col;
		col = (tile >> 4)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(1)] = colbase + col;
		col = (tile >> 0)  & 0x0f; if (col) bmap[PIXEL_XOR_BE(0)] = colbase + col;
	}

	/* transparent */
	else
	{
		col = (tile >> 28) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(7)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(7)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(7)] & 0x7ff)];
			}
		}
		col = (tile >> 24) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(6)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(6)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(6)] & 0x7ff)];
			}
		}
		col = (tile >> 20) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(5)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(5)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(5)] & 0x7ff)];
			}
		}
		col = (tile >> 16) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(4)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(4)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(4)] & 0x7ff)];
			}
		}
		col = (tile >> 12) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(3)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(3)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(3)] & 0x7ff)];
			}
		}
		col = (tile >>  8) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(2)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(2)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(2)] & 0x7ff)];
			}
		}
		col = (tile >>  4) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(1)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(1)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(1)] & 0x7ff)];
			}
		}
		col = (tile >>  0) & 0x0f;
		if (col)
		{
			if (col < 0x0e) bmap[PIXEL_XOR_BE(0)] = colbase + col;
			else
			{
				bmap[PIXEL_XOR_BE(0)] = transparent_lookup[((col & 1) << 11) | (bmap[PIXEL_XOR_BE(0)] & 0x7ff)];
			}
		}
	}
}


static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase)
{
	int spriteypos   = (((spritebase[0] & 0x01) << 8) | spritebase[1]) - 0x80;
	int spritexpos   = (((spritebase[6] & 0x01) << 8) | spritebase[7]) - 0x78;
	int spriteheight = ((spritebase[2] & 0x03) + 1) * 8;
	int spritewidth  = (((spritebase[2] & 0x0c) >> 2) + 1) * 8;
	int spriteattr, patno, patflip, patline, colbase, x;

	/* skip if out of range */
	if (line < spriteypos || line >= spriteypos + spriteheight)
		return;
	if (spritexpos + spritewidth < 8 || spritexpos >= 328)
		return;

	/* extract the remaining data */
	spriteattr = (spritebase[4] << 8) | spritebase[5];
	patno      = spriteattr & 0x07FF;
	patflip    = (spriteattr & 0x1800) >> 11;
	patline    = line - spriteypos;

	/* determine the color base */
	colbase = 16 * ((spriteattr & 0x6000) >> 13) + segac2_sp_palbase + segac2_palbank;

	/* adjust for the X position */
	spritewidth >>= 3;
	spriteheight >>= 3;

	/* switch off the flip mode */
	bmap += spritexpos;
	switch (patflip)
	{
		case 0x00: /* No Flip */
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= 0 && spritexpos < 328)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x01: /* Horizontal Flip */
			patno += spriteheight * (spritewidth - 1);
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= 0 && spritexpos < 328)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;

		case 0x02: /* Vertical Flip */
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= 0 && spritexpos < 328)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x03: /* Both Flip */
			patno += spriteheight * (spritewidth - 1);
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= 0 && spritexpos < 328)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;
	}
}



/******************************************************************************/
/* General Information (Mainly Genesis Related                                */
/******************************************************************************/
/* Genesis VDP Registers (from sega2.doc)

Reg# : |  Bit7  |  Bit6  |  Bit5  |  Bit4  |  Bit3  |  Bit2  |  Bit1  |  Bit0  |    General Function
--------------------------------------------------------------------------------
0x00 : |  0     |  0     |  0     |  IE1   |  0     |  1     |  M3    |  0     |    Mode Set Register #1
IE = Enabled H Interrupt (Lev 4), M3 = Enable HV Counter Read / Stopped
0x01 : |  0     |  DISP  |  IE0   |  M1    |  M2    |  1     |  0     |  0     |    Mode Set Register #2
DISP = Display Enable, IE0 = Enabled V Interrupt (Lev 6), M1 = DMA Enabled, M2 = 30 Cell Mode
0x02 : |  0     |  0     |  SA15  |  SA14  |  SA13  |  0     |  0     |  0     |    Scroll A Base Address
SA13-15 = Bits 13-15 of the Scroll A Name Table Base Address in VRAM
0x03 : |  0     |  0     |  WD15  |  WD14  |  WD13  |  WD12  |  WD11  |  0     |    Window Base Address
WD11-15 = Bits 11-15 of the Window Name Table Base Address in VRAM
0x04 : |  0     |  0     |  0     |   0    |  0     |  SB15  |  SB14  |  SB13  |    Scroll B Base Address
SB13-15 = Bits 13-15 of the Scroll B Name Table Base Address in VRAM
0x05 : |  0     |  AT15  |  AT14  |  AT13  |  AT12  |  AT11  |  AT10  |  AT9   |    Sprite Table Base Address
AT9=15 = Bits 9-15 of the Sprite Name Table Base Address in VRAM
0x06 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x07 : |  0     |  0     |  CPT1  |  CPT0  |  COL3  |  COL2  |  COL1  |  COL0  |    Background Colour Select
CPT0-1 = Palette Number, COL = Colour in Palette
0x08 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x09 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0A : |  BIT7  |  BIT6  |  BIT5  |  BIT4  |  BIT3  |  BIT2  |  BIT1  |  BIT0  |    H-Interrupt Register
BIT0-7 = Controls Level 4 Interrupt Timing
0x0B : |  0     |  0     |  0     |  0     |  IE2   |  VSCR  |  HSCR  |  LSCR  |    Mode Set Register #3
IE2 = Enable E Interrupt (Lev 2), VSCR = Vertical Scroll Mode, HSCR / LSCR = Horizontal Scroll Mode
0x0C : |  RS0   |  0     |  0     |  0     |  S/TE  |  LSM1  |  LSM0  |  RS1   |    Mode Set Register #4
RS0 / RS1 = Cell Mode, S/TE = Shadow/Hilight Enable, LSM0 / LSM1 = Interlace Mode Setting
0x0D : |  0     |  0     |  HS15  |  HS14  |  HS13  |  HS12  |  HS11  |  HS10  |    HScroll Base Address
HS10-15 = Bits 10-15 of the HScroll Name Table Base Address in VRAM
0x0E : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0F : |  INC7  |  INC6  |  INC5  |  INC4  |  INC3  |  INC2  |  INC1  |  INC0  |    Auto-Increment
INC0-7 = Auto Increment Value (after VRam Access)
0x10 : |  0     |  0     |  VSZ1  |  VSZ0  |  0     |  0     |  HSZ1  |  HSZ0  |    Scroll Size
VSZ0-1 = Vertical Plane Size, HSZ0-1 = Horizontal Plane Size
0x11 : |  RIGT  |  0     |  0     |  WHP5  |  WHP4  |  WHP3  |  WHP2  |  WHP1  |    Window H Position
RIGT = Window Right Side of Base Point, WHP1-5 = Bits1-5 of Window H Point
0x12 : |  DOWN  |  0     |  0     |  WVP4  |  WVP3  |  WVP2  |  WVP1  |  WVP0  |    Window V Position
DOWN = Window Below Base Point, WVP0-4 = Bits0-4 of Window V Point
0x13 : |  LG7   |  LG6   |  LG5   |  LG4   |  LG3   |  LG2   |  LG1   |  LG0   |    DMA Length Counter LOW
0x14 : |  LG15  |  LG14  |  LG13  |  LG12  |  LG11  |  LG10  |  LG9   |  LG8   |    DMA Length Counter HIGH
LG0-15 = Bits 0-15 of DMA Length Counter
0x15 : |  SA8   |  SA7   |  SA6   |  SA5   |  SA4   |  SA3   |  SA2   |  SA1   |    DMA Source Address LOW
0x16 : |  SA16  |  SA15  |  SA14  |  SA13  |  SA12  |  SA11  |  SA10  |  SA9   |    DMA Source Address MID
0x17 : |  DMD1  |  DMD0  |  SA22  |  SA21  |  SA20  |  SA19  |  SA18  |  S17   |    DMA Source Address HIGH
LG0-15 = Bits 1-22 of DMA Source Address
DMD0-1 = DMA Mode

Memory Layouts ...

Scroll Name Table
16-bits are used to Define a Tile in the Scroll Plane
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
PRI = Priority, CP = Colour Palette, VF = VFlip, HF = HFlip, PT0-9 = Tile # in VRAM

HScroll Data  (in VRAM)
0x00 |
0x01 / H Scroll of Plane A (Used in Overall, Cell & Line Modes)
0x02 |
0x03 / H Scroll of Plane B (Used in Overall, Cell & Line Modes)
0x04 |
0x05 / H Scroll of Plane A (Used in Line Mode)
0x06 |
0x07 / H Scroll of Plane B (Used in Line Mode)
...
0x20 |
0x21 / H Scroll of Plane A (Used in Cell & Line Mode)
0x22 |
0x23 / H Scroll of Plane B (Used in Cell & Line Mode)
etc.. That kinda thing :)
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  x     |  HS9   |  HS8   |
|  HS7   |  HS6   |  HS5   |  HS4   |  HS3   |  HS2   |  HS1   |  HS0   |
HS = HScroll Amount for Overall / Cell / Line depending on Mode.

VScroll Data (in VSRAM)
0x00 |
0x01 / V Scroll of Plane A (Used in Overall & Cell Modes)
0x02 |
0x03 / V Scroll of Plane B (Used in Overall & Cell Modes)
0x04 |
0x05 / V Scroll of Plane A (Used in Cell Mode)
0x06 |
0x07 / V Scroll of Plane B (Used in Cell Modes)
etc..
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  VS10  |  VS9   |  VS8   |
|  VS7   |  VS6   |  VS5   |  VS4   |  VS3   |  VS2   |  VS1   |  VS0   |
VS = HScroll Amount for Overall / Cell / Line depending on Mode.

Sprite Attributes Table (in VRAM)
Each Sprite uses 64-bits of Information (8 bytes)
|  x     |  x     |  x     |  x     |  x     |  x     |  YP9   |  YP8   |
|  YP7   |  YP6   |  YP5   |  YP4   |  YP3   |  YP2   |  YP1   |  YP0   |
|  x     |  x     |  x     |  x     |  HS1   |  HS0   |  VS1   |  VS0   |
|  x     |  LN6   |  LN5   |  LN4   |  LN3   |  LN2   |  LN1   |  LN0   |
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
|  x     |  x     |  x     |  x     |  x     |  x     |  XP9   |  XP8   |
|  XP7   |  XP6   |  XP5   |  XP4   |  XP3   |  XP2   |  XP1   |  XP0   |
YP = Y Position of Sprite
HS = Horizontal Size (Blocks)
VS = Vertical Size (Blocks)
LN = Link Field
PRI = Priority
CP = Colour Palette
VF = VFlip
HF = HFlip
PT = Pattern Number
XP = X Position of Sprite



*/

