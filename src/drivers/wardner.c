/***************************************************************************
				Wardners Forest  (Pyros in USA)

		Basically the same video and machine hardware as Flying shark,
		except for the Main CPU which is a Z80 here.


**************************** Memory & I/O Maps *****************************
Z80:(0)  Main CPU
0000-6fff Main ROM
7000-7fff Main RAM
8000-ffff Level and scenery ROMS. This is banked with the following
8000-8fff Sprite RAM
a000-adff Pallette RAM
c000-c7ff Sound RAM - shared with C000-C7FF in Z80(1) RAM

in:
50		DSW 1
52		DSW 2
54		Player 1 controls
56		Player 2 controls
58		VBlank (bit 7) and coin-in/start inputs
60		LSB data from char display layer
61		MSB data from char display layer
62		LSB data from BG   display layer
63		MSB data from BG   display layer
64		LSB data from FG   display layer
65		MSB data from FG   display layer

out:
00		6845 CRTC offset register
02		6845 CRTC register data
10		char scroll LSB   < Y >
11		char scroll MSB   < Y >
12		char scroll LSB     X
13		char scroll MSB     X
14		char LSB RAM offset     20h * 40h  (0-07ff) and (4000-47ff) ???
15		char MSB RAM offset
20		BG   scroll LSB   < Y >
21		BG   scroll MSB   < Y >
22		BG   scroll LSB     X
23		BG   scroll MSB     X
24		BG   LSB RAM offset     40h * 40h  (0-0fff)
25		BG   MSB RAM offset
30		FG   scroll LSB   < Y >
31		FG   scroll MSB   < Y >
32		FG   scroll LSB     X
33		FG   scroll MSB     X
34		FG   LSB RAM offset     40h * 40h  (0-0fff)
35		FG   MSB RAM offset
40		spare scroll LSB  < Y >  (Not used)
41		spare scroll MSB  < Y >  (Not used)
5a-5c	Control registers
		bits 7-4 always 0
		bits 3-1 select the control signal to drive.
		bit   0  is the value passed to the control signal.
5a		data
		00-01	INT line to TMS320C10 DSP (Active low trigger)
		0c-0d	lockout for coin A input (Active low lockout)
		0e-0f	lockout for coin B input (Active low lockout)
5c		data
		00-01	???
		02-03	???
		04-05	Active low INTerrupt to Z80(0) for screen refresh
		06-07	Flip Screen (Active high flips)
		08-09	Background RAM display bank switch
		0a-0b	Foreground ROM display bank switch (not used here)
		0c-0d	??? (what the hell does this do ?)
60		LSB data to char display layer
61		MSB data to char display layer
62		LSB data to BG   display layer
63		MSB data to BG   display layer
64		LSB data to FG   display layer
65		MSB data to FG   display layer
70		ROM bank selector for Z80(0) address 8000-ffff
		data
		00  switch ROM from 8000-ffff out, and put sprite/palette/sound RAM back.
		02  switch lower half of B25-18.ROM  ROM to 8000-ffff
		03  switch upper half of B25-18.ROM  ROM to 8000-ffff
		04  switch lower half of B25-19.ROM  ROM to 8000-ffff
		05  switch upper half of B25-19.ROM  ROM to 8000-ffff
		07  switch               B25-30.ROM  ROM to 8000-ffff



Z80:(1)  Sound CPU
0000-7fff Main ROM
8000-807f RAM ???
c000-cfff Sound RAM - C000-C7FF shared with C000-C7FF in Z80(0) ram



TMS320C10 DSP: Harvard type architecture. RAM and ROM on seperate data buses.
0000-07ff ROM 16-bit opcoodes (word access only)
0000-0090 Internal RAM (words).	Moved to 8000-8120 for MAME compatibility.
								View this memory in the debugger at 4000h
in:
01		data read from addressed Z80:(0) address space (Main RAM/Sprite RAM)

out:
00		address of Z80:(0) to read/write to
01		data to write to addressed Z80:(0) address space (Main RAM/Sprite RAM)
03		bit 15 goes to BIO line of TMS320C10. BIO is a polled input line.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"

extern unsigned char *wardner_mainram;

void wardner_reset(void);

int  twincobr_crtc_r(int offset);
void twincobr_crtc_w(int offset,int data);

void wardner_videoram_w(int offset, int data);
int  wardner_videoram_r(int offset);
void wardner_bglayer_w(int offset, int data);
void wardner_fglayer_w(int offset, int data);
void wardner_txlayer_w(int offset, int data);
void wardner_bgscroll_w(int offset, int data);
void wardner_fgscroll_w(int offset, int data);
void wardner_txscroll_w(int offset, int data);
void twincobr_exscroll_w(int offset,int data);
void wardner_mainram_w(int offset, int data);
int  wardner_mainram_r(int offset);



int  twincobr_vh_start(void);
void twincobr_vh_stop(void);
void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern unsigned char *videoram;
extern unsigned char *twincobr_fgvideoram;
extern unsigned char *twincobr_bgvideoram;
extern int twincobr_display_on;

int  twincobr_dsp_in(int offset);
void twincobr_dsp_out(int fnction,int data);
void twincobr_7800c_w(int offset,int data);
void fshark_coin_dsp_w(int offset,int data);



extern int intenable;
static int wardner_membank = 0;

int wardner_interrupt(void)
{
	if (intenable) {
		intenable = 0;
		return interrupt();
	}
	else return ignore_interrupt();
}


int vblank_wardner_input_r(int offset)
{
	/* VBlank and also coin, start and system switches */
	int read_two_ports = 0;		/* hack coz cant read switches and vblank on same port ? */
	read_two_ports = readinputport(0);
	read_two_ports |= readinputport(3);
	return read_two_ports;
}


void CRTC_add_w(int offset,int data)
{
	crtc6845_address_w(offset, data);
}

void CRTC_data_w(int offset, int data)
{
	crtc6845_register_w(0, data);
	twincobr_display_on = 1;
}

int wardner_sprite_r(int offset)
{
	return spriteram[offset];
}

void wardner_sprite_w(int offset, int data)
{
	spriteram[offset] = data;
}


void wardner_ramrom_banksw(int offset,int data)
{
	if (wardner_membank != data) {
		int bankaddress = 0;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
		wardner_membank = data;

		if (data) {
			install_mem_read_handler (0, 0x8000, 0xffff, MRA_BANK1);
			switch (data) {
				case 2:  bankaddress = 0x10000; break;
				case 3:  bankaddress = 0x18000; break;
				case 4:  bankaddress = 0x20000; break;
				case 5:  bankaddress = 0x28000; break;
				case 7:  bankaddress = 0x38000; break;
				case 1:  bankaddress = 0x08000; break; /* not used */
				case 6:  bankaddress = 0x30000; break; /* not used */
				default: bankaddress = 0x00000; break; /* not used */
			}
			cpu_setbank(1,&RAM[bankaddress]);
		}
		else {
			cpu_setbank(1,&RAM[0x0000]);
			install_mem_read_handler (0, 0x8000, 0x8fff, wardner_sprite_r);
			install_mem_read_handler (0, 0xa000, 0xadff, paletteram_r);
			install_mem_read_handler (0, 0xae00, 0xafff, MRA_BANK2);
			install_mem_read_handler (0, 0xc000, 0xc7ff, MRA_BANK3);
		}
	}
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x6fff, MRA_ROM },			/* Main CPU ROM code */
	{ 0x7000, 0x7fff, wardner_mainram_r },	/* Main RAM */
	{ 0x8000, 0x8fff, wardner_sprite_r },	/* Sprite RAM data */
	{ 0x9000, 0x9fff, MRA_ROM },			/* Banked ROM */
	{ 0xa000, 0xadff, paletteram_r },		/* Palette RAM */
	{ 0xae00, 0xafff, MRA_BANK2 },			/* Unused Palette RAM */
	{ 0xb000, 0xbfff, MRA_ROM },			/* Banked ROM */
	{ 0xc000, 0xc7ff, MRA_BANK3 },			/* Shared RAM with Sound CPU RAM */
	{ 0xc800, 0xffff, MRA_ROM },			/* Banked ROM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, wardner_mainram_w, &wardner_mainram },
	{ 0x8000, 0x8fff, wardner_sprite_w, &spriteram, &spriteram_size },
	{ 0x9000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xadff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xae00, 0xafff, MWA_BANK2 },
	{ 0xb000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_BANK3 },
	{ 0xc800, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x807f, MRA_BANK4 },
	{ 0xc000, 0xc7ff, MRA_BANK3 },
	{ 0xc800, 0xcfff, MRA_BANK5 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x807f, MWA_BANK4 },
	{ 0xc000, 0xc7ff, MWA_BANK3 },
	{ 0xc800, 0xcfff, MWA_BANK5 },
	{ -1 }
};

static struct MemoryReadAddress DSP_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x8000, 0x811F, MRA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }
};

static struct MemoryWriteAddress DSP_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x8000, 0x811F, MWA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }
};


static struct IOReadPort readport[] =
{
	{ 0x50, 0x50, input_port_4_r },
	{ 0x52, 0x52, input_port_5_r },
	{ 0x54, 0x54, input_port_1_r },
	{ 0x56, 0x56, input_port_2_r },
	{ 0x58, 0x58, vblank_wardner_input_r },
	{ 0x60, 0x65, wardner_videoram_r },		/* data from video layer RAM */
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x02, twincobr_crtc_w },
	{ 0x10, 0x13, wardner_txscroll_w },		/* scroll text layer */
	{ 0x14, 0x15, wardner_txlayer_w },		/* offset in text video RAM */
	{ 0x20, 0x23, wardner_bgscroll_w },		/* scroll bg layer */
	{ 0x24, 0x25, wardner_bglayer_w },		/* offset in bg video RAM */
	{ 0x30, 0x33, wardner_fgscroll_w },		/* scroll fg layer */
	{ 0x34, 0x35, wardner_fglayer_w },		/* offset in fg video RAM */
	{ 0x40, 0x43, twincobr_exscroll_w },	/* scroll extra layer (not used) */
	{ 0x60, 0x65, wardner_videoram_w },		/* data for video layer RAM */
	{ 0x5a, 0x5a, fshark_coin_dsp_w },		/* Machine system control */
	{ 0x5c, 0x5c, twincobr_7800c_w },		/* Machine system control */
	{ 0x70, 0x70, wardner_ramrom_banksw },	/* ROM bank select */
	{ -1 }
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ -1 }
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ -1 }
};

static struct IOReadPort DSP_readport[] =
{
	{ 0x01, 0x01, twincobr_dsp_in },
	{ -1 }
};
static struct IOWritePort DSP_writeport[] =
{
	{ 0x00,  0x03, twincobr_dsp_out },
	{ -1 }
};


INPUT_PORTS_START( wardner_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )	/* Skips video RAM tests */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000 & 80000" )
	PORT_DIPSETTING(	0x04, "50000 & 100000" )
	PORT_DIPSETTING(	0x08, "30000" )
	PORT_DIPSETTING(	0x0c, "50000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( pyros_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )	/* Skips video RAM tests */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000 & 80000" )
	PORT_DIPSETTING(	0x04, "50000 & 100000" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(	0x40, DEF_STR( No ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wardnerj_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )	/* Extra Player 1 button */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000 & 80000" )
	PORT_DIPSETTING(	0x04, "50000 & 100000" )
	PORT_DIPSETTING(	0x08, "30000" )
	PORT_DIPSETTING(	0x0c, "50000" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x20, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0*2048*8*8, 1*2048*8*8, 2*2048*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every char takes 8 consecutive bytes */
};

static struct GfxLayout fgtilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout bgtilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,		/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
	//cpu_cause_interrupt(1,0xff);
}
static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	24000000/7,	/* 3.43 MHz ??? */
	{ 255 },	/* (not supported) */
	{ irqhandler },
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1536, 32 },	/* colors 1536-1791 */
	{ 1, 0x0c000, &fgtilelayout, 1280, 16 },	/* colors 1280-1535 */
	{ 1, 0x2c000, &bgtilelayout, 1024, 16 },	/* colors 1024-1079 */
	{ 1, 0x4c000, &spritelayout,    0, 64 },	/* colors    0-1023 */
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			24000000/4,			/* 6 MHz ??? - Real board crystal is 24Mhz */
			0,					/* memory region #0 */
			readmem,writemem,
			readport,
			writeport,
			wardner_interrupt,1
		},
		{
			CPU_Z80,
			24000000/7,			/* 3.43 MHz ??? */
			2,					/* memory region #2 */
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		},
		{
			CPU_TMS320C10,
			24000000/7,			/* 3.43 MHz ??? */
			3,					/* memory region #3 */
			DSP_readmem,DSP_writemem,
			DSP_readport,DSP_writeport,
			ignore_interrupt,0	/* IRQs are caused by Z80(0) */
		},
	},
	56, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	20,										/* 20 CPU slices per frame */
	wardner_reset,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1792, 1792,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	twincobr_vh_start,
	twincobr_vh_stop,
	twincobr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	},
};




/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( wardner_rom )
	ROM_REGION(0x48000)		/* Banked Z80 code */
	ROM_LOAD( "wardner.17", 0x00000, 0x08000, 0xc5dd56fd )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom", 0x18000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom", 0x28000, 0x10000, 0x95b68813 )
	ROM_LOAD( "wardner.20", 0x40000, 0x08000, 0x347f411b )

	ROM_REGION_DISPOSE(0x8c000)		/* temporary space for graphics */
	ROM_LOAD( "wardner.07", 0x00000, 0x04000, 0x1392b60d )	/* chars */
	ROM_LOAD( "wardner.06", 0x04000, 0x04000, 0x0ed848da )
	ROM_LOAD( "wardner.05", 0x08000, 0x04000, 0x79792c86 )
	ROM_LOAD( "b25-12.rom", 0x0c000, 0x08000, 0x15d08848 )	/* fg tiles */
	ROM_LOAD( "b25-15.rom", 0x14000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom", 0x1c000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom", 0x24000, 0x08000, 0xbe21db2b )
	ROM_LOAD( "b25-08.rom", 0x2c000, 0x08000, 0x883ccaa3 )	/* bg tiles */
	ROM_LOAD( "b25-11.rom", 0x34000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom", 0x3c000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom", 0x44000, 0x08000, 0x585411b7 )
	ROM_LOAD( "b25-01.rom", 0x4c000, 0x10000, 0x42ec01fb )	/* sprites */
	ROM_LOAD( "b25-02.rom", 0x5c000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom", 0x6c000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom", 0x7c000, 0x10000, 0x8059573c )

	ROM_REGION(0x10000)		/* 32k for second Z80 CPU */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp.lsb",	0x0000, 0x0800, BADCRC( 0x7bbb405a ) )
	ROM_LOAD_ODD ( "dsp.msb",	0x0000, 0x0800, BADCRC( 0x963ce23d ) )
ROM_END

ROM_START( pyros_rom )
	ROM_REGION(0x48000)		/* Banked Z80 code */
	ROM_LOAD( "b25-29.rom", 0x00000, 0x08000, 0xb568294d )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom", 0x18000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom", 0x28000, 0x10000, 0x95b68813 )
	ROM_LOAD( "b25-30.rom", 0x40000, 0x08000, 0x5056c799 )

	ROM_REGION_DISPOSE(0x8c000)		/* temporary space for graphics */
	ROM_LOAD( "b25-35.rom", 0x00000, 0x04000, 0xfec6f0c0 )	/* chars */
	ROM_LOAD( "b25-34.rom", 0x04000, 0x04000, 0x02505dad )
	ROM_LOAD( "b25-33.rom", 0x08000, 0x04000, 0x9a55fcb9 )
	ROM_LOAD( "b25-12.rom", 0x0c000, 0x08000, 0x15d08848 )	/* fg tiles */
	ROM_LOAD( "b25-15.rom", 0x14000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom", 0x1c000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom", 0x24000, 0x08000, 0xbe21db2b )
	ROM_LOAD( "b25-08.rom", 0x2c000, 0x08000, 0x883ccaa3 )	/* bg tiles */
	ROM_LOAD( "b25-11.rom", 0x34000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom", 0x3c000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom", 0x44000, 0x08000, 0x585411b7 )
	ROM_LOAD( "b25-01.rom", 0x4c000, 0x10000, 0x42ec01fb )	/* sprites */
	ROM_LOAD( "b25-02.rom", 0x5c000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom", 0x6c000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom", 0x7c000, 0x10000, 0x8059573c )

	ROM_REGION(0x10000)		/* 32k for second Z80 CPU */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp.lsb",	0x0000, 0x0800, BADCRC( 0x7bbb405a ) )
	ROM_LOAD_ODD ( "dsp.msb",	0x0000, 0x0800, BADCRC( 0x963ce23d ) )
ROM_END

ROM_START( wardnerj_rom )
	ROM_REGION(0x48000)		/* Banked Z80 code */
	ROM_LOAD( "wardnerj.17", 0x00000, 0x08000, 0xc06804ec )	/* Main Z80 code */
	ROM_LOAD( "b25-18.rom",  0x18000, 0x10000, 0x9aab8ee2 )	/* OBJ ROMs */
	ROM_LOAD( "b25-19.rom",  0x28000, 0x10000, 0x95b68813 )
	ROM_LOAD( "wardner.20",  0x40000, 0x08000, 0x347f411b )

	ROM_REGION_DISPOSE(0x8c000)		/* temporary space for graphics */
	ROM_LOAD( "wardnerj.07", 0x00000, 0x04000, 0x50e329e0 )	/* chars */
	ROM_LOAD( "wardnerj.06", 0x04000, 0x04000, 0x3bfeb6ae )
	ROM_LOAD( "wardnerj.05", 0x08000, 0x04000, 0xbe36a53e )
	ROM_LOAD( "b25-12.rom",  0x0c000, 0x08000, 0x15d08848 )	/* fg tiles */
	ROM_LOAD( "b25-15.rom",  0x14000, 0x08000, 0xcdd2d408 )
	ROM_LOAD( "b25-14.rom",  0x1c000, 0x08000, 0x5a2aef4f )
	ROM_LOAD( "b25-13.rom",  0x24000, 0x08000, 0xbe21db2b )
	ROM_LOAD( "b25-08.rom",  0x2c000, 0x08000, 0x883ccaa3 )	/* bg tiles */
	ROM_LOAD( "b25-11.rom",  0x34000, 0x08000, 0xd6ebd510 )
	ROM_LOAD( "b25-10.rom",  0x3c000, 0x08000, 0xb9a61e81 )
	ROM_LOAD( "b25-09.rom",  0x44000, 0x08000, 0x585411b7 )
	ROM_LOAD( "b25-01.rom",  0x4c000, 0x10000, 0x42ec01fb )	/* sprites */
	ROM_LOAD( "b25-02.rom",  0x5c000, 0x10000, 0x6c0130b7 )
	ROM_LOAD( "b25-03.rom",  0x6c000, 0x10000, 0xb923db99 )
	ROM_LOAD( "b25-04.rom",  0x7c000, 0x10000, 0x8059573c )

	ROM_REGION(0x10000)		/* 32k for second Z80 CPU */
	ROM_LOAD( "b25-16.rom", 0x00000, 0x08000, 0xe5202ff8 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp.lsb",	0x0000, 0x0800, BADCRC( 0x7bbb405a ) )
	ROM_LOAD_ODD ( "dsp.msb",	0x0000, 0x0800, BADCRC( 0x963ce23d ) )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x7117],"\x20\x00\x00\x20",4) == 0) &&
		(memcmp(&RAM[0x7170],"\x02\x01\x01",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x7119], 10*3);	/* score values */
			osd_fread(f,&RAM[0x7137], 10*5);	/* initials */
			osd_fread(f,&RAM[0x7169], 10);		/* levels */
/*			osd_fread(f,&RAM[0x71b2], 3);		 * current players score */
/*			osd_fread(f,&RAM[0x7230], 3);		 * other players score */

			RAM[0x7116] = RAM[0x7119];		/* update high score */
			RAM[0x7117] = RAM[0x711a];		/* on top of screen */
			RAM[0x7118] = RAM[0x711b];
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x7119],30+50+10);		/* HS table */
		osd_fclose(f);
	}
}



struct GameDriver wardner_driver =
{
	__FILE__,
	0,
	"wardner",
	"Wardner (World)",
	"1987",
	"[Toaplan] Taito Corporation Japan",
	"Quench",
	0,
	&machine_driver,
	0,

	wardner_rom,
	0, 0,
	0,
	0,

	wardner_input_ports,

	0, 0, 0,
	0,

	hiload, hisave
};

struct GameDriver pyros_driver =
{
	__FILE__,
	&wardner_driver,
	"pyros",
	"Pyros (US)",
	"1987",
	"[Toaplan] Taito America Corporation",
	"Quench",
	0,
	&machine_driver,
	0,

	pyros_rom,
	0, 0,
	0,
	0,

	pyros_input_ports,

	0, 0, 0,
	0,

	hiload, hisave
};

struct GameDriver wardnerj_driver =
{
	__FILE__,
	&wardner_driver,
	"wardnerj",
	"Wardner no Mori (Japan)",
	"1987",
	"[Toaplan] Taito Corporation Japan",
	"Quench",
	0,
	&machine_driver,
	0,

	wardnerj_rom,
	0, 0,
	0,
	0,

	wardnerj_input_ports,

	0, 0, 0,
	0,

	hiload, hisave
};

