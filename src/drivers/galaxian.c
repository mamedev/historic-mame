/***************************************************************************

Galaxian memory map

Compiled from information provided by friends and Uncles on RGVAC.

            AAAAAA
            111111AAAAAAAAAA     DDDDDDDD   Schem   function
HEX         5432109876543210 R/W 76543210   name

0000-27FF                                           Game ROM
5000-57FF   01010AAAAAAAAAAA R/W DDDDDDDD   !Vram   Character ram
5800-583F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Screen attributes
5840-585F   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Sprites
5860-5FFF   01011AAAAAAAAAAA R/W DDDDDDDD   !OBJRAM Bullets

6000        0110000000000000 R   -------D   !SW0    coin1
6000        0110000000000000 R   ------D-   !SW0    coin2
6000        0110000000000000 R   -----D--   !SW0    p1 left
6000        0110000000000000 R   ----D---   !SW0    p1 right
6000        0110000000000000 R   ---D----   !SW0    p1shoot
6000        0110000000000000 R   --D-----   !SW0    table ??
6000        0110000000000000 R   -D------   !SW0    test
6000        0110000000000000 R   D-------   !SW0    service

6000        0110000000000001 W   -------D   !DRIVER lamp 1 ??
6001        0110000000000001 W   -------D   !DRIVER lamp 2 ??
6002        0110000000000010 W   -------D   !DRIVER lamp 3 ??
6003        0110000000000011 W   -------D   !DRIVER coin control
6004        0110000000000100 W   -------D   !DRIVER Background lfo freq bit0
6005        0110000000000101 W   -------D   !DRIVER Background lfo freq bit1
6006        0110000000000110 W   -------D   !DRIVER Background lfo freq bit2
6007        0110000000000111 W   -------D   !DRIVER Background lfo freq bit3

6800        0110100000000000 R   -------D   !SW1    1p start
6800        0110100000000000 R   ------D-   !SW1    2p start
6800        0110100000000000 R   -----D--   !SW1    p2 left
6800        0110100000000000 R   ----D---   !SW1    p2 right
6800        0110100000000000 R   ---D----   !SW1    p2 shoot
6800        0110100000000000 R   --D-----   !SW1    no used
6800        0110100000000000 R   -D------   !SW1    dip sw1
6800        0110100000000000 R   D-------   !SW1    dip sw2

6800        0110100000000000 W   -------D   !SOUND  reset background F1
                                                    (1=reset ?)
6801        0110100000000001 W   -------D   !SOUND  reset background F2
6802        0110100000000010 W   -------D   !SOUND  reset background F3
6803        0110100000000011 W   -------D   !SOUND  Noise on/off
6804        0110100000000100 W   -------D   !SOUND  not used
6805        0110100000000101 W   -------D   !SOUND  shoot on/off
6806        0110100000000110 W   -------D   !SOUND  Vol of f1
6807        0110100000000111 W   -------D   !SOUND  Vol of f2

7000        0111000000000000 R   -------D   !DIPSW  dip sw 3
7000        0111000000000000 R   ------D-   !DIPSW  dip sw 4
7000        0111000000000000 R   -----D--   !DIPSW  dip sw 5
7000        0111000000000000 R   ----D---   !DIPSW  dip s2 6

7001        0111000000000001 W   -------D   9Nregen NMIon
7004        0111000000000100 W   -------D   9Nregen stars on
7006        0111000000000110 W   -------D   9Nregen hflip
7007        0111000000000111 W   -------D   9Nregen vflip

Note: 9n reg,other bits  used on moon cresta for extra graphics rom control.

7800        0111100000000000 R   --------   !wdr    watchdog reset
7800        0111100000000000 W   DDDDDDDD   !pitch  Sound Fx base frequency

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
extern void pisces_gfxbank_w(int offset,int data);
extern int mooncrst_vh_start(void);
extern void mooncrst_vh_stop(void);
extern void mooncrst_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void mooncrst_sound_freq_w(int offset,int data);
extern int mooncrst_sh_start(void);
extern void mooncrst_sh_update(void);



static struct MemoryReadAddress galaxian_readmem[] =
{
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x27ff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, MRA_NOP },
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress pisces_readmem[] =
{
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, MRA_NOP },
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress japirem_readmem[] =
{
	{ 0x5000, 0x5fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW */
	{ 0x7800, 0x7800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress galaxian_writemem[] =
{
	{ 0x5000, 0x53ff, mooncrst_videoram_w, &mooncrst_videoram },
	{ 0x5800, 0x583f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &mooncrst_spriteram },
	{ 0x5860, 0x5880, MWA_RAM, &mooncrst_bulletsram },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_sound_freq_w },
	{ 0x7004, 0x7004, mooncrst_stars_w },
	{ 0x0000, 0x27ff, MWA_ROM },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress pisces_writemem[] =
{
	{ 0x5000, 0x53ff, mooncrst_videoram_w, &mooncrst_videoram },
	{ 0x5800, 0x583f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &mooncrst_spriteram },
	{ 0x5860, 0x5880, MWA_RAM, &mooncrst_bulletsram },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x6002, 0x6002, pisces_gfxbank_w },
	{ 0x7800, 0x7800, mooncrst_sound_freq_w },
	{ 0x7004, 0x7004, mooncrst_stars_w },
	{ 0x0000, 0x2fff, MWA_ROM },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress japirem_writemem[] =
{
	{ 0x5000, 0x53ff, mooncrst_videoram_w, &mooncrst_videoram },
	{ 0x5800, 0x583f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x5840, 0x585f, MWA_RAM, &mooncrst_spriteram },
	{ 0x5860, 0x5880, MWA_RAM, &mooncrst_bulletsram },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7800, 0x7800, mooncrst_sound_freq_w },
	{ 0x6002, 0x6002, pisces_gfxbank_w },
	{ 0x7004, 0x7004, mooncrst_stars_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort galaxian_input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, OSD_KEY_3, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, OSD_KEY_F2, 0 },
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



static struct InputPort warofbug_input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_3, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, OSD_KEY_DOWN, OSD_KEY_UP },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE, 0, OSD_JOY_DOWN, OSD_JOY_UP }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW galaxian_dsw[] =
{
	{ 2, 0x04, "LIVES", { "3", "5" } },
	{ 2, 0x03, "BONUS", { "7000", "10000", "12000", "20000" } },
	{ -1 }
};



static struct DSW pisces_dsw[] =
{
	{ 1, 0x40, "LIVES", { "3", "4" } },
	{ 1, 0x80, "SW2", { "OFF", "ON" } },
	{ 2, 0x01, "SW3", { "OFF", "ON" } },
	{ 2, 0x04, "SW5", { "OFF", "ON" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct DSW japirem_dsw[] =
{
	{ 2, 0x04, "LIVES", { "3", "5" } },
	{ 2, 0x03, "BONUS", { "NONE", "4000", "5000", "7000" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct DSW warofbug_dsw[] =
{
	{ 2, 0x03, "LIVES", { "1", "2", "3", "4" } },
	{ 2, 0x04, "SW5", { "OFF", "ON" } },
	{ 2, 0x08, "SW6", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout galaxian_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout galaxian_spritelayout =
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
static struct GfxLayout pisces_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout pisces_spritelayout =
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
	0,0,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo galaxian_gfxdecodeinfo[] =
{
	{ 0x10000, &galaxian_charlayout,    0,  8 },
	{ 0x10000, &galaxian_spritelayout,  0,  8 },
	{ 0,       &starslayout,           32, 64 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo pisces_gfxdecodeinfo[] =
{
	{ 0x10000, &pisces_charlayout,    0,  8 },
	{ 0x10000, &pisces_spritelayout,  0,  8 },
	{ 0,       &starslayout,         32, 64 },
	{ -1 } /* end of array */
};



static unsigned char galaxian_color_prom[] =
{
	/* palette */
	0x00,0x00,0x00,0xF6,0x00,0x16,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC0,0xA0,0x0C,0x00,0x00,0x00,0x07,0x00,0xF6,0x07,0xF0,0x00,0x76,0x07,0xC6
};



static unsigned char japirem_color_prom[] =
{
	/* palette */
	0x00,0x7A,0x36,0x07,0x00,0xF0,0x38,0x1F,0x00,0xC7,0xF0,0x3F,0x00,0xDB,0xC6,0x38,
	0x00,0x36,0x07,0xF0,0x00,0x33,0x3F,0xDB,0x00,0x3F,0x57,0xC6,0x00,0xC6,0x3F,0xFF
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
#if 0
	/* VOL1 = 0  VOL2 = 0 */
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x18,0x18,0x28,0x28,0x18,0x18,0x28,0x28,
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x18,0x18,0x28,0x28,0x18,0x18,0x28,0x28,
	/* VOL1 = 1  VOL2 = 0 */
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x4c,0x4c,0x5c,0x5c,0x4c,0x4c,0x5c,0x5c,
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x4c,0x4c,0x5c,0x5c,0x4c,0x4c,0x5c,0x5c,
	/* VOL1 = 0  VOL2 = 1 */
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x18,0x18,0x28,0x28,0x18,0x18,0x28,0x28,
	0x23,0x23,0x33,0x33,0x23,0x23,0x33,0x33,0x3b,0x3b,0x4b,0x4b,0x3b,0x3b,0x4b,0x4b,
	/* VOL1 = 1  VOL2 = 1 */
	0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x4c,0x4c,0x5c,0x5c,0x4c,0x4c,0x5c,0x5c,
	0x23,0x23,0x33,0x33,0x23,0x23,0x33,0x33,0x6f,0x6f,0x7f,0x7f,0x6f,0x6f,0x7f,0x7f,
#endif
};



const struct MachineDriver galaxian_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	galaxian_readmem,galaxian_writemem,0,0,
	galaxian_input_ports,galaxian_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	galaxian_gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	galaxian_color_prom,mooncrst_vh_convert_color_prom,0,0,
	0,17,
	0x00,0x01,
	8*13,8*16,0x05,
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



const struct MachineDriver pisces_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	pisces_readmem,pisces_writemem,0,0,
	galaxian_input_ports,pisces_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	pisces_gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	galaxian_color_prom,mooncrst_vh_convert_color_prom,0,0,
	0,17,
	0x00,0x01,
	8*13,8*16,0x05,
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



const struct MachineDriver japirem_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	japirem_readmem,japirem_writemem,0,0,
	galaxian_input_ports,japirem_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	pisces_gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	japirem_color_prom,mooncrst_vh_convert_color_prom,0,0,
	0,17,
	0x07,0x02,
	8*13,8*16,0x00,
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



const struct MachineDriver warofbug_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	galaxian_readmem,galaxian_writemem,0,0,
	warofbug_input_ports,warofbug_dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	galaxian_gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	japirem_color_prom,mooncrst_vh_convert_color_prom,0,0,
	0,17,
	0x00,0x01,
	8*13,8*16,0x05,
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
