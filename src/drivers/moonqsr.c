/***************************************************************************

>
> Dips 1 & 2 control difficulty.
>
> 1 2
> ---
> F F = Easy
> N F = Fairly easy
> F N = Average
> N N = Difficult
>
> 3 4
> ---
> F F = 1C, 1P(lay)
> N F = 2C, 1P
> F N = 3C, 1P
> N N = 4C, 1P
>
> 5 6
> ---
> F F = 1C, 1P(lay)
> N F = 1C, 2P
> F N = 1C, 3P
> N N = Freeplay

***************************************************************************/

#include "driver.h"


extern unsigned char *moonqsr_videoram;
extern unsigned char *moonqsr_attributesram;
extern unsigned char *moonqsr_spriteram;
extern unsigned char *moonqsr_bulletsram;
extern void moonqsr_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void moonqsr_videoram_w(int offset,int data);
extern void moonqsr_attributes_w(int offset,int data);
extern void moonqsr_stars_w(int offset,int data);
extern void moonqsr_gfxextend_w(int offset,int data);
extern int moonqsr_vh_start(void);
extern void moonqsr_vh_stop(void);
extern void moonqsr_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void mooncrst_sound_freq_w(int offset,int data);
extern int mooncrst_sh_start(void);
extern void mooncrst_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW (coins per play) */
	{ 0xb800, 0xb800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x93ff, moonqsr_videoram_w, &moonqsr_videoram },
	{ 0x9800, 0x983f, moonqsr_attributes_w, &moonqsr_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &moonqsr_spriteram },
	{ 0x9860, 0x9880, MWA_RAM, &moonqsr_bulletsram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb800, 0xb800, mooncrst_sound_freq_w },
	{ 0xb004, 0xb004, moonqsr_stars_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_3, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 1, 0xc0, "DIFFICULTY", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the color table */
static struct GfxLayout starslayout =
{
	1,1,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 0, 0,      &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0xf6,0x79,0x4f,0x00,0xc0,0x3f,0x17,0x00,0x87,0xf8,0x7f,0x00,0xc1,0x7f,0x38,
	0x00,0x7f,0xcf,0xf9,0x00,0x57,0xb7,0xc3,0x00,0xff,0x7f,0x87,0x00,0x79,0x4f,0xff
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
};



const struct MachineDriver moonqsr_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	color_prom,moonqsr_vh_convert_color_prom,0,0,
	0,10,
	0x07,0x02,
	8*13,8*16,0x01,
	0,
	moonqsr_vh_start,
	moonqsr_vh_stop,
	moonqsr_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	mooncrst_sh_start,
	0,
	mooncrst_sh_update
};
