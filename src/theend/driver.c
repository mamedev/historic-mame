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
#include "machine.h"
#include "common.h"


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



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,     0, 8 },
	{ 0x10000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
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
};

enum { TE1,TE2,TE3,TE4,TE5,TE6,TE7,TE8,TE9,TEA,TEB,TEC,TED,TEE,TEF,TEG,TEH };

static unsigned char colortable[] =
{
    TE1, TE2, TE3, TE5,         // white text
    TE1, TE8, TE9, TEA,         //
    TE1, TEB, TEC, TEA,         // green text
    TE1, TE9, TE8, TED,         //
    TE1, TEE, TEF, TE8,         // bluish text
    TE1, TE6, TE7, TED,         //
    TE1, TE8, TEG, TE4,         //
    TE1, TE5, TEA, TEH,         // explosion
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
	256,256,
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
