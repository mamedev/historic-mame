/***************************************************************************


***************************************************************************/

#include "driver.h"


extern unsigned char *mooncrst_videoram;
extern unsigned char *mooncrst_attributesram;
extern unsigned char *mooncrst_spriteram;
extern unsigned char *mooncrst_bulletsram;
extern void mooncrst_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void mooncrst_videoram_w(int offset,int data);
extern void mooncrst_attributes_w(int offset,int data);
extern void mooncrst_stars_w(int offset,int data);
extern void mooncrst_gfxextend_w(int offset,int data);
extern int mooncrst_vh_start(void);
extern void mooncrst_vh_stop(void);
extern void mooncrst_vh_screenrefresh(struct osd_bitmap *bitmap);

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
	{ 0x9000, 0x93ff, mooncrst_videoram_w, &mooncrst_videoram },
	{ 0x9800, 0x983f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &mooncrst_spriteram },
	{ 0x9860, 0x9880, MWA_RAM, &mooncrst_bulletsram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb800, 0xb800, mooncrst_sound_freq_w },
	{ 0xa000, 0xa002, mooncrst_gfxextend_w },
	{ 0xb004, 0xb004, mooncrst_stars_w },
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
		0x80,
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
	{ 1, 0x80, "LANGUAGE", { "JAPANESE", "ENGLISH" } },
	{ 1, 0x40, "SW1", { "OFF", "ON" } },
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



static unsigned char palette[] =
{
	/* characters */
    0, 0, 0,
    0, 0, 255,
    255, 0, 0,
    255, 255, 0,
    128, 0, 255,
    255, 128, 0,
    255, 0, 128,
    28*4, 20*4, 0,
    0, 28*4, 20*4,
    23*4, 30*4, 23*4,
    0, 200, 128,
    200, 128, 200,
    128, 255, 128,
    200, 200, 64,
    255, 0, 200,
    200, 64, 255,
    200, 255, 0,
    0, 255, 200,
    255, 0, 255,
    255, 255, 255,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,

	/* stars */
	0x00,0x00,0x00,
	0x88,0x00,0x00,
	0xcc,0x00,0x00,
	0xff,0x00,0x00,
	0x00,0x88,0x00,
	0x88,0x88,0x00,
	0xcc,0x88,0x00,
	0xff,0x88,0x00,
	0x00,0xcc,0x00,
	0x88,0xcc,0x00,
	0xcc,0xcc,0x00,
	0xff,0xcc,0x00,
	0x00,0xff,0x00,
	0x88,0xff,0x00,
	0xcc,0xff,0x00,
	0xff,0xff,0x00,
	0x00,0x00,0x88,
	0x88,0x00,0x88,
	0xcc,0x00,0x88,
	0xff,0x00,0x88,
	0x00,0x88,0x88,
	0x88,0x88,0x88,
	0xcc,0x88,0x88,
	0xff,0x88,0x88,
	0x00,0xcc,0x88,
	0x88,0xcc,0x88,
	0xcc,0xcc,0x88,
	0xff,0xcc,0x88,
	0x00,0xff,0x88,
	0x88,0xff,0x88,
	0xcc,0xff,0x88,
	0xff,0xff,0x88,
	0x00,0x00,0xcc,
	0x88,0x00,0xcc,
	0xcc,0x00,0xcc,
	0xff,0x00,0xcc,
	0x00,0x88,0xcc,
	0x88,0x88,0xcc,
	0xcc,0x88,0xcc,
	0xff,0x88,0xcc,
	0x00,0xcc,0xcc,
	0x88,0xcc,0xcc,
	0xcc,0xcc,0xcc,
	0xff,0xcc,0xcc,
	0x00,0xff,0xcc,
	0x88,0xff,0xcc,
	0xcc,0xff,0xcc,
	0xff,0xff,0xcc,
	0x00,0x00,0xff,
	0x88,0x00,0xff,
	0xcc,0x00,0xff,
	0xff,0x00,0xff,
	0x00,0x88,0xff,
	0x88,0x88,0xff,
	0xcc,0x88,0xff,
	0xff,0x88,0xff,
	0x00,0xcc,0xff,
	0x88,0xcc,0xff,
	0xcc,0xcc,0xff,
	0xff,0xcc,0xff,
	0x00,0xff,0xff,
	0x88,0xff,0xff,
	0xcc,0xff,0xff,
	0xff,0xff,0xff
};

enum
{ MC1, MC2, MC3, MC4, MC5, MC6, MC7, MC8, MC9, MCA, MCB,
		MCC, MCD, MCE, MCF, MCG, MCH, MCI, MCJ, MCK };

static unsigned char colortable[] =
{
	/* characters */
    MC1, MC3, MCK, MC2,
    MC1, MC2, MC3, MC4,
    MC1, MC5, MC6, MC7,
    MC1, MC8, MC9, MCA,
    MC1, MCB, MCC, MCD,
    MC1, MCE, MCF, MCG,
    MC1, MC2, MC4, MC3,
    MC1, MCH, MCI, MCK,

	/* stars */
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
};



const struct MachineDriver mooncrst_driver =
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
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,10,
	0x07,0x01,
	8*13,8*16,0x06,
	0,
	mooncrst_vh_start,
	mooncrst_vh_stop,
	mooncrst_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	mooncrst_sh_start,
	0,
	mooncrst_sh_update
};
