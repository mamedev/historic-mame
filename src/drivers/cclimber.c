/***************************************************************************

Crazy Climber memory map (preliminary)
as described by Lionel Theunissen (lionelth@ozemail.com.au)

0000h-4fffh ;20k program ROMs. ROM11=0000h
                               ROM10=1000h
                               ROM09=2000h
                               ROM08=3000h
                               ROM07=4000h

8000h-83ffh ;1k scratchpad RAM.
8800h-88ffh ;256 bytes Bigsprite RAM.
9000h-93ffh ;1k screen RAM.
9800h-981fh ;Column smooth scroll position. Corresponds to each char
             column.

9880h-989fh ;Sprite controls. 8 groups of 4 bytes:
  1st byte; code/attribute.
            Bits 0-5: sprite code.
            Bit    6: x invert.
            Bit    7: y invert.
  2nd byte ;color.
            Bits 0-3: colour. (palette scheme 0-15)
            Bit    4: 0=charset1, 1 =charset 2.
  3rd byte ;y position
  4th byte ;x position

98ddh ;Bigsprite colour/attribute.
            Bit 0-2: Big sprite colour.
            Bit   4: x invert.
            Bit   5: y invert. (not used by CC)
98deh ;Bigsprite y position.
98dfh ;Bigsprite x position.

9c00h-9fffh ;1/2k colour RAM: Bits 0-3: colour. (palette scheme 0-15)
                              Bit    4: 0=charset1, 1=charset2.
                              Bit    5: (not used by CC)
                              Bit    6: x invert.
                              Bit    7: y invert. (not used by CC)

a000h ;RD: Player 1 controls.
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right

a000h ;WR: Non Maskable interrupt.
            Bit 0: 0=NMI disable, 1=NMI enable.

a001h ;WR: Horizontal video direction (for table model).
            Bit 0: 0=Normal, 1=invert.

a002h ;WR: Vertical video direction (for table model).
            Bit 0: 0=Normal, 1=invert.

a004h ;WR: Sample trigger.
            Bit 0: 0=Trigger.

a800h ;RD: Player 2 controls (table model only).
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right


a800h ;WR: Sample rate speed.
              Full byte value (0-255).

b000h ;RD: DIP switches.
            Bit 1,0: Number of climbers.
                     00=3, 01=4, 10=5, 11=6.
            Bit   2: Extra climber bonus.
                     0=30000, 1=50000.
            Bit   3: 1=Test Pattern
            Bit 5,4: Coins per credit.
                     00=1, 01=2, 10=3 11=4.
            Bit 7,6: Plays per credit.
                     00=1, 01=2, 10=3, 11=Freeplay.

b000h ;WR: Sample volume.
            Bits 0-5: Volume (0-31).

b800h ;RD: Machine switches.
            Bit 0: Coin 1.
            Bit 1: Coin 2.
            Bit 2: 1 Player start.
            Bit 3: 2 Player start.
            Bit 4: Upright/table select.
                   0=table, 1=upright.


I/O 8  ;AY-3-8910 Control Reg.
I/O 9  ;AY-3-8910 Data Write Reg.
I/O C  ;AY-3-8910 Data Read Reg.
        Port A of the 8910 selects the digital sample to play

***************************************************************************/

#include "driver.h"
#include "machine.h"
#include "common.h"


extern unsigned char *cclimber_videoram;
extern unsigned char *cclimber_colorram;
extern unsigned char *cclimber_bsvideoram;
extern unsigned char *cclimber_spriteram;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;
extern void cclimber_videoram_w(int offset,int data);
extern void cclimber_colorram_w(int offset,int data);
extern void cclimber_bigsprite_videoram_w(int offset,int data);
extern void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int cclimber_vh_start(void);
extern void cclimber_vh_stop(void);
extern void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int cclimber_sh_read_port_r(int offset);
extern void cclimber_sh_control_port_w(int offset,int data);
extern void cclimber_sh_write_port_w(int offset,int data);
extern void cclimber_sample_trigger_w(int offset,int data);
extern void cclimber_sample_rate_w(int offset,int data);
extern void cclimber_sample_volume_w(int offset,int data);
extern int cclimber_sh_start(void);
extern void cclimber_sh_stop(void);
extern void cclimber_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x9880, 0x989f, MRA_RAM },	/* sprite registers */
	{ 0x98dc, 0x98df, MRA_RAM },	/* bigsprite registers */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW1 */
	{ 0xb800, 0xb800, input_port_3_r },	/* IN2 */
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0x9800, 0x981f, MRA_RAM },	/* column scroll registers */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9880, 0x989f, MWA_RAM, &cclimber_spriteram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0x9000, 0x93ff, cclimber_videoram_w, &cclimber_videoram },
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &cclimber_colorram },
	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram },
	{ 0x9400, 0x97ff, cclimber_videoram_w },	/* mirror address, used to draw windows */
	{ 0x98dc, 0x98df, MWA_RAM, &cclimber_bigspriteram },
	{ 0xa004, 0xa004, cclimber_sample_trigger_w },
	{ 0xb000, 0xb000, cclimber_sample_volume_w },
	{ 0xa800, 0xa800, cclimber_sample_rate_w },
	{ 0xa001, 0xa002, MWA_NOP },
	{ 0x0000, 0x4fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0c, 0x0c, cclimber_sh_read_port_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x08, 0x08, cclimber_sh_control_port_w },
	{ 0x09, 0x09, cclimber_sh_write_port_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F,
				OSD_KEY_I, OSD_KEY_K, OSD_KEY_J, OSD_KEY_L },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE2, OSD_JOY_FIRE3, OSD_JOY_FIRE1, OSD_JOY_FIRE4 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x00,
		{ 0, 0, 0, OSD_KEY_F1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x10,	/* standup cabinet */
		{ 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x04, "BONUS", { "30000", "50000" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,     0, 16 },	/* char set #1 */
	{ 0x11000, &charlayout,     0, 16 },	/* char set #2 */
	{ 0x12000, &charlayout,  4*16,  8 },	/* big sprite char set */
	{ 0x10000, &spritelayout,   0, 16 },	/* sprite set #1 */
	{ 0x11000, &spritelayout,   0, 16 },	/* sprite set #2 */
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* char palette */
	0x00,0x79,0x04,0x87,0x00,0xb7,0xff,0x5f,0x00,0xc0,0xe8,0xf4,0x00,0x3f,0x04,0x38,
	0x00,0x0d,0x7a,0xb7,0x00,0x07,0x26,0x02,0x00,0x27,0x16,0x30,0x00,0xb7,0xf4,0x0c,
	0x00,0x4f,0xf6,0x24,0x00,0xb6,0xff,0x5f,0x00,0x33,0x00,0xb7,0x00,0x66,0x00,0x3a,
	0x00,0xc0,0x3f,0xb7,0x00,0x20,0xf4,0x16,0x00,0xff,0x7f,0x87,0x00,0xb6,0xf4,0xc0,
	/* bigsprite palette */
	0x00,0xff,0x18,0xc0,0x00,0xff,0xc6,0x8f,0x00,0x0f,0xff,0x1e,0x00,0xff,0xc0,0x67,
	0x00,0x47,0x7f,0x80,0x00,0x88,0x47,0x7f,0x00,0x7f,0x88,0x47,0x00,0x40,0x08,0xff
};



const struct MachineDriver cclimber_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	readmem,writemem,readport,writeport,
	input_ports,dsw,
	0,
	nmi_interrupt,

	/* video hardware */
	256,256,
	gfxdecodeinfo,
	96,4*24,
	color_prom,cclimber_vh_convert_color_prom,0,0,
	0,10,
	0x0a,0x0b,
	8*13,8*16,0x0a,
	0,
	cclimber_vh_start,
	cclimber_vh_stop,
	cclimber_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	cclimber_sh_start,
	cclimber_sh_stop,
	cclimber_sh_update
};
