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
extern void pisces_gfxbank_w(int offset,int data);
extern int mooncrst_vh_start(void);
extern void mooncrst_vh_stop(void);
extern void mooncrst_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x57ff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8100, 0x8100, input_port_0_r },	/* IN0 */
	{ 0x8101, 0x8101, input_port_1_r },	/* IN1 */
	{ 0x8102, 0x8102, input_port_2_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, mooncrst_videoram_w, &mooncrst_videoram },
	{ 0x5000, 0x503f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &mooncrst_spriteram },
	{ 0x5060, 0x5080, MWA_RAM, &mooncrst_bulletsram },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, OSD_KEY_3, OSD_KEY_CONTROL,
				OSD_KEY_LEFT, OSD_KEY_RIGHT, 0, 0 },
		{ 0, 0, 0, OSD_JOY_FIRE,
				OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0 }
	},
	{	/* IN1 */
		0x80,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x01,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x01, "SW1", { "OFF", "ON" } },
	{ 2, 0x02, "SW2", { "OFF", "ON" } },
	{ 2, 0x04, "SW3", { "OFF", "ON" } },
	{ 2, 0x08, "SW4", { "OFF", "ON" } },
	{ 2, 0x10, "SW5", { "OFF", "ON" } },
	{ 2, 0x20, "SW6", { "OFF", "ON" } },
	{ 2, 0x40, "SW7", { "OFF", "ON" } },
	{ 2, 0x80, "SW8", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
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
	0,0,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,     0, 8 },
	{ 0x10000, &spritelayout,   0, 8 },
	{ 0,       &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	/* characters */
    0*4, 0*4, 0*4,                    // TE1
    20*4, 20*4, 20*4,                 // TE2
    40*4, 40*4, 40*4,                 // TE3
    58*4, 58*4, 58*4,                 // TE4
    63*4, 63*4, 63*4,                 // TE5
    10*4, 10*4, 20*4,                 // TE6
    20*4, 20*4, 40*4,                 // TE7
    0*4, 0*4, 63*4,                   // TE8
    63*4, 0*4, 0*4,                   // TE9
    0*4, 63*4, 0*4,                   // TEA
    0*4, 20*4, 0*4,                   // TEB
    0*4, 40*4, 0*4,                   // TEC
    40*4, 63*4, 63*4,                 // TED
    0*4, 0*4, 20*4,                   // TEE
    0*4, 0*4, 40*4,                   // TEF
    63*4, 20*4, 63*4,                 // TEG
    63*4, 0*4, 63*4,                  // TEH
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

enum { TE1,TE2,TE3,TE4,TE5,TE6,TE7,TE8,TE9,TEA,TEB,TEC,TED,TEE,TEF,TEG,TEH };

static unsigned char colortable[] =
{
	/* characters */
    TE1, TE2, TE3, TE5,         // white text
    TE1, TE8, TE9, TEA,         //
    TE1, TEB, TEC, TEA,         // green text
    TE1, TE9, TE8, TED,         //
    TE1, TEE, TEF, TE8,         // bluish text
    TE1, TE6, TE7, TED,         //
    TE1, TE8, TEG, TE4,         //
    TE1, TE5, TEA, TEH,         // explosion

	/* stars */
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f
};



const struct MachineDriver theend_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	readmem,writemem,0,0,
	input_ports,dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,17,
	0x00,0x01,
	8*13,8*16,0x05,
	0,
	mooncrst_vh_start,
	mooncrst_vh_stop,
	mooncrst_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
