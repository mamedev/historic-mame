/***************************************************************************

Homedata Games

driver by Phil Stroffolino and Nicola Salmoria


*1987 X77 Mahjong Hourouki Part1 -Seisyun Hen-
*1987 X72 Mahjong Hourouki Gaiden
 1988     Mahjong Joshi Pro-wres -Give up 5 byou mae-
*1988 A74 Mahjong Hourouki Okite
*1988 X80 Mahjong Clinic
*1988 M81 Mahjong Rokumeikan
*1988 J82 Reikai Doushi / Chinese Exorcist
*1989 X83 Mahjong Kojin Kyouju (Private Teacher)
 1989     Battle Cry (not released in Japan)
*1989 X90 Mahjong Vitamin C
*1989 X91 Mahjong Yougo no Kiso Tairyoku
*1990 X02 Mahjong Lemon Angel
*1991 X07 Mahjong Kinjirareta Asobi -Ike Ike Kyoushi no Yokubou-

Games from other companies:

*1991 M15 Mahjong Ikagadesuka     (c) Mitchell
*19??     Mahjong Jogakuen        (c) Windom


These games use only tilemaps for graphics.  These tilemaps are organized into
two pages (a visible page and a backbuffer) which are automatically swapped by the
hardware at vblank.

Some of the tiles are written directly by the CPU, others are written by a "blitter"
which unpacks RLE data from a ROM.


In games using the uPD7807CW, the coprocessor manages input ports and sound/music.


Notes:

- To access service mode in the older mahjong games, keep F2 pressed during boot.
  Service mode doesn't work in hourouki because it needs an additional "check" ROM.

- The "help" button some games ask you to press is the start button.

- The games can change visible area at runtime. The meaning of the control registers
  isn't understood, but it supported enough to give the correct visible area to all
  games.
  mjkinjas sets the registers to values different from all the other games; it also
  has a 11MHz xtal instead of the 9MHz of all the others, so the two things are
  probably related.

- the ROM for the uPD7807 is divided in 4 0x10000 banks. The first three just
  contain a simple sample player, followed by the PCM data. The fourth bank contains
  the main program. The program literally changes banks under its own feet. However,
  the portions of code executed while the bank switch happens are identical in all
  banks, so it still works with MAME's standard MRA_BANK handling.


TODO:
- Dip switches! They might be right for mjhokite, but I haven't verified the other
  games.

- I'm not sure service mode in the newer mahjong games is working as it's supposed to.
  dip switch changes are not reported, and keypresses only work after you insert a coin.

- Sound and inputs don't work in mjikaga. CPU communication issue?
  Coins and dip switches *do* work however.
  Also note that bit 2 of bankswitch_w() and bit 7 of pteacher_blitter_bank_w() might
  have some other function, since the ROMs are smaller.

- wrong gfx in mrokumei at the beginning of a game. It is selecting the wrong gfx bank;
  the bank handling seems correct in all other games, so I don't know what's wrong here.

- mjikaga shows some imperfections with horizontal placement of the screen.

- in attract mode, hourouki draws a horizontal black bar on the bottom right side of
  the display.


----------------------------------------------------------------------------
Mahjong Hourouki
(c)1987 Home Data

Board:  A77-PWB-A-(A)

CPU:    68B09E Z80-A
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom:	GX61A01

----------------------------------------------------------------------------
Mahjong Hourouki Gaiden
(c)1987 Home Data

Board:  A77-PWB-A-(A)

CPU:    68B09E Z80-A
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom:	GX61A01

----------------------------------------------------------------------------
Mahjong Hourouki Okite
(c)1988 Homedata

Almost same board as "Mahjong Clinic"

Board:  X77-PWB-A-(A) A74 PWB-B

CPU:    Hitachi HD68B09EP (location 14G), Sharp LH0080A (Z80A, location 10K)
Sound:  SN76489 DAC?
OSC:    16.000MHz (OSC1) 9.000MHz (OSC2)
Custom: HOMEDATA GX61A01 102 8728KK (100pin PQFP, location 8C)

----------------------------------------------------------------------------
Mahjong Rokumeikan
(c)1988 Home Data

Board:  A74-PWB-A-(A) (main) A74 PWB-B     (sub)

CPU:    68B09E Z80-A
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom: GX61A01

----------------------------------------------------------------------------
----------------------------------------------------------------------------
Reikai Doushi (Chinese Exorcist)
aka Last Apostle Puppet Show (US)
(c)1988 HOME DATA

CPU   : 68B09E
SOUND : YM2203
OSC.  : 16.000MHz 9.000MHz

----------------------------------------------------------------------------
----------------------------------------------------------------------------
Mahjong Kojinkyouju (Private Teacher)
(c)1989 HOME DATA

Board:  X73-PWB-A(C)

CPU:    6809 uPC324C
Sound:  SN76489
OSC:    16.000MHz 9.000MHz

----------------------------------------------------------------------------
Mahjong Vitamin C
(c)1989 Home Data

Board:  X73-PWB-A(A)

CPU:    68B09E uPD7807CW(?)
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom: GX61A01

----------------------------------------------------------------------------
Mahjong-yougo no Kisotairyoku
(c)1989 Home Data

Board:  X83-PWB-A(A)

CPU:    68B09E uPD7807CW(?)
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom: GX61A01

----------------------------------------------------------------------------
Mahjong Kinjirareta Asobi
(c)1990 Home Data

Board:  X83-PWB-A(A)

CPU:    68B09E uPD7807CW
Sound:  SN76489AN
        DAC
OSC:    11.000MHz 16.000MHz
Custom: GX61A01

----------------------------------------------------------------------------
Mahjong Jogakuen
(c)19?? Windom

Board:  X83-PWB-A(A)

CPU:    68B09E uPD7807CW(?)
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom: GX61A01

----------------------------------------------------------------------------
----------------------------------------------------------------------------
Mahjong Lemon Angel
(c)1990 Homedata

Board:  X83-PWB-A(A)

CPU:    Fujitsu MBL68B09E (16G)
        (surface scratched 64pin DIP device on location 12G) [probably uPD7807CW]
Sound:  SN76489
OSC:    16.0000MHz (XTAL1) 9.000MHz (XTAL2)
Custom: HOMEDATA GX61A01 102 8842KK

----------------------------------------------------------------------------
Mahjong Ikagadesuka
(c)1991 Mitchell

Board:  X83-PWB-A(A)

CPU:    68B09E uPD7807CW
Sound:  SN76489AN DAC
OSC:    9.000MHz 16.000MHz
Custom: GX61A01

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/upd7810/upd7810.h"
#include "homedata.h"



/********************************************************************************/


static int vblank;

static INTERRUPT_GEN( homedata_irq )
{
	vblank = 1;
	cpu_set_irq_line(0,M6809_FIRQ_LINE,HOLD_LINE);
}

static INTERRUPT_GEN( upd7807_irq )
{
	cpu_set_irq_line(1,UPD7810_INTF1,HOLD_LINE);
}


/********************************************************************************

  Older Mahjong games:

  single CPU, handling SN76489 + DAC and inputs

 ********************************************************************************/

static int keyb;

static READ_HANDLER( mrokumei_keyboard_r )
{
	int res = 0x3f,i;

	/* offset 0 is player 1, offset 1 player 2 (not supported) */
	if (offset == 0)
	{
		for (i = 0;i < 5;i++)
		{
			if (keyb & (1 << i))
			{
				res = readinputport(3+i) & 0x3f;
				break;
			}
		}
	}

	if (offset == 0)
	{
		/* bit 7: visible page
		 * bit 6: vblank
		 * other bits are inputs
		 */
		res |= homedata_visible_page << 7;

		if (vblank) res |= 0x40;

		vblank = 0;
	}

	return res;
}

static WRITE_HANDLER( mrokumei_keyboard_select_w )
{
	keyb = data;
}



static int sndbank;

static READ_HANDLER( mrokumei_sound_io_r )
{
	if (sndbank & 4)
		return(soundlatch_r(0));
	else
		return memory_region(REGION_CPU2)[0x10000 + offset + (sndbank & 1) * 0x10000];
}

static WRITE_HANDLER( mrokumei_sound_bank_w )
{
	/* bit 0 = ROM bank
	   bit 2 = ROM or soundlatch
	 */
	sndbank = data;
}

static WRITE_HANDLER( mrokumei_sound_io_w )
{
	switch (offset & 0xff)
	{
		case 0x40:
			DAC_signed_data_w(0,data);
			break;
		default:
			logerror("%04x: I/O write to port %04x\n",activecpu_get_pc(),offset);
			break;
	}
}

static WRITE_HANDLER( mrokumei_sound_cmd_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(1,0,HOLD_LINE);
}


/********************************************************************************

  Reikai Doushi:

  slave uPD7807, handling YM2203 + DAC and inputs

 ********************************************************************************/

static int upd7807_porta,upd7807_portc;

static READ_HANDLER( reikaids_upd7807_porta_r )
{
	return upd7807_porta;
}

static WRITE_HANDLER( reikaids_upd7807_porta_w )
{
	upd7807_porta = data;
}

static WRITE_HANDLER( reikaids_upd7807_portc_w )
{
	/* port C layout:
	   7 coin counter
	   6 to main CPU (data)
	   5 YM2203 write clock
	   4 YM2203 read clock
	   3 YM2203 address (0 = register select 1 = data)
	   2 to main CPU (status)
	   1 \ ROM bank
	   0 /
	  */
//	logerror("%04x: port C wr %02x (STATUS %d DATA %d)\n",activecpu_get_pc(),data,BIT(data,2),BIT(data,6));


	cpu_setbank(2,memory_region(REGION_CPU2) + 0x10000 * (data & 0x03));

	coin_counter_w(0,~data & 0x80);

	if (BIT(upd7807_portc,5) && !BIT(data,5))	/* write clock 1->0 */
	{
		if (BIT(data,3))
			YM2203_write_port_0_w(0,upd7807_porta);
		else
			YM2203_control_port_0_w(0,upd7807_porta);
	}

	if (BIT(upd7807_portc,4) && !BIT(data,4))	/* read clock 1->0 */
	{
		if (BIT(data,3))
			upd7807_porta = YM2203_read_port_0_r(0);
		else
			upd7807_porta = YM2203_status_port_0_r(0);
	}

	upd7807_portc = data;
}

static MACHINE_INIT( reikaids_upd7807 )
{
	/* on reset, ports are set as input (high impedance), therefore 0xff output */
	reikaids_upd7807_portc_w(0,0xff);
}

READ_HANDLER( reikaids_io_r )
{
	int res = readinputport(2);	// bit 4 = coin, bit 5 = service

	res |= BIT(upd7807_portc,2) * 0x01;		// bit 0 = upd7807 status
	res |= BIT(upd7807_portc,6) * 0x02;		// bit 1 = upd7807 data
	if (vblank) res |= 0x04;				// bit 2 = vblank
	res |= homedata_visible_page * 0x08;	// bit 3 = visible page

	vblank = 0;

//logerror("%04x: io_r %02x\n",activecpu_get_pc(),res);

	return res;
}

static int snd_command;

static READ_HANDLER( reikaids_snd_command_r )
{
//logerror("%04x: sndmcd_r (%02x)\n",activecpu_get_pc(),snd_command);
	return snd_command;
}

static WRITE_HANDLER( reikaids_snd_command_w )
{
	snd_command = data;
//logerror("%04x: coprocessor_command_w %02x\n",activecpu_get_pc(),data);
}



/********************************************************************************

  Newer Mahjong games:

  slave uPD7807, handling SN76489 + DAC and inputs

 ********************************************************************************/

static int to_cpu,from_cpu;

static WRITE_HANDLER( pteacher_snd_command_w )
{
//logerror("%04x: snd_command_w %02x\n",activecpu_get_pc(),data);
	from_cpu = data;
}

static READ_HANDLER( pteacher_snd_r )
{
//logerror("%04x: pteacher_snd_r %02x\n",activecpu_get_pc(),to_cpu);
	return to_cpu;
}

static READ_HANDLER( pteacher_io_r )
{
	/* bit 6: !vblank
	 * bit 7: visible page
	 * other bits seem unused
	 */

	int res = (homedata_visible_page ^ 1) << 7;

	if (!vblank) res |= 0x40;

	vblank = 0;

	return res;
}

static READ_HANDLER( pteacher_keyboard_r )
{
	int dips = readinputport(0);

//	logerror("%04x: keyboard_r with port A = %02x\n",activecpu_get_pc(),upd7807_porta);

	if (upd7807_porta & 0x80)
	{
		/* player 1 + dip switches */
		int row = (upd7807_porta & 0x07);
		return readinputport(2 + row) | (((dips >> row) & 1) << 5);	// 0-5
	}
	if (upd7807_porta & 0x08)
	{
		/* player 2 (not supported) + dip switches */
		int row = ((upd7807_porta >> 4) & 0x07);
		return 0xdf | (((dips >> (row+5)) & 1) << 5);	// 6-11
	}

	return 0xff;
}

static READ_HANDLER( pteacher_upd7807_porta_r )
{
	if (!BIT(upd7807_portc,6))
		upd7807_porta = from_cpu;
	else
logerror("%04x: read PA with PC *not* clear\n",activecpu_get_pc());

	return upd7807_porta;
}

static WRITE_HANDLER( pteacher_snd_answer_w )
{
	to_cpu = data;
//logerror("%04x: to_cpu = %02x\n",activecpu_get_pc(),to_cpu);
}

static WRITE_HANDLER( pteacher_upd7807_porta_w )
{
	upd7807_porta = data;
}

static WRITE_HANDLER( pteacher_upd7807_portc_w )
{
	/* port C layout:
	   7 coin counter
	   6 enable message from main CPU on port A
	   5 clock latch for 76489 command
	   4
	   3 \ ROM bank
	   2 /
	   1 input (service)
	   0 input (coin)
	  */

//	logerror("%04x: port C wr %02x\n",activecpu_get_pc(),data);

	cpu_setbank(2,memory_region(REGION_CPU2) + 0x10000 * ((data & 0x0c) >> 2));

	coin_counter_w(0,~data & 0x80);

	if (BIT(upd7807_portc,5) && !BIT(data,5))	/* clock 1->0 */
		SN76496_0_w(0,upd7807_porta);

	upd7807_portc = data;
}

static MACHINE_INIT( pteacher_upd7807 )
{
	/* on reset, ports are set as input (high impedance), therefore 0xff output */
	pteacher_upd7807_portc_w(0,0xff);
}


/********************************************************************************/


static WRITE_HANDLER( bankswitch_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int len = memory_region_length(REGION_CPU1) - 0x10000+0x4000;
	int offs = (data * 0x4000) & (len-1);

	/* last bank is fixed */
	if (offs < len - 0x4000)
	{
		cpu_setbank(1, &rom[offs + 0x10000]);
	}
	else
	{
		cpu_setbank(1, &rom[0xc000]);
	}
}


/********************************************************************************/


MEMORY_READ_START( mrokumei_readmem )
	{ 0x0000, 0x3fff, MRA_RAM }, /* videoram */
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM }, /* work ram */
	{ 0x7800, 0x7800, MRA_RAM }, /* only used to store the result of the ROM check */
	{ 0x7801, 0x7802, mrokumei_keyboard_r },	// also vblank and active page
	{ 0x7803, 0x7803, input_port_2_r },	// coin, service
	{ 0x7804, 0x7804, input_port_0_r },	// DSW1
	{ 0x7805, 0x7805, input_port_1_r },	// DSW2
	{ 0x7ffe, 0x7ffe, MRA_NOP },	// ??? read every vblank, value discarded
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( mrokumei_writemem )
	{ 0x0000, 0x3fff, mrokumei_videoram_w, &videoram },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7800, 0x7800, MWA_RAM }, /* only used to store the result of the ROM check */
	{ 0x7ff0, 0x7ffd, MWA_RAM, &homedata_vreg },
	{ 0x8000, 0x8000, mrokumei_blitter_start_w },	// in some games also ROM bank switch to access service ROM
	{ 0x8001, 0x8001, mrokumei_keyboard_select_w },
	{ 0x8002, 0x8002, mrokumei_sound_cmd_w },
	{ 0x8003, 0x8003, SN76496_0_w },
	{ 0x8006, 0x8006, homedata_blitter_param_w },
	{ 0x8007, 0x8007, mrokumei_blitter_bank_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MEMORY_READ_START( mrokumei_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( mrokumei_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xfffc, 0xfffd, MWA_NOP },	/* stack writes happen here, but there's no RAM */
	{ 0x8080, 0x8080, mrokumei_sound_bank_w },
MEMORY_END

static PORT_READ_START( mrokumei_sound_readport )
	{ 0x0000, 0xffff, mrokumei_sound_io_r },
MEMORY_END

static PORT_WRITE_START( mrokumei_sound_writeport )
	{ 0x0000, 0xffff, mrokumei_sound_io_w },	/* read address is 16-bit, write address is only 8-bit */
MEMORY_END

/********************************************************************************/

MEMORY_READ_START( reikaids_readmem )
	{ 0x0000, 0x3fff, MRA_RAM }, /* videoram */
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM }, /* work ram */
	{ 0x7800, 0x7800, MRA_RAM },
	{ 0x7801, 0x7801, input_port_0_r },
	{ 0x7802, 0x7802, input_port_1_r },
	{ 0x7803, 0x7803, reikaids_io_r },	// coin, blitter, upd7807
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( reikaids_writemem )
	{ 0x0000, 0x3fff, reikaids_videoram_w, &videoram },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7800, 0x7800, MWA_RAM },	/* behaves as normal RAM */
	{ 0x7ff0, 0x7ffd, MWA_RAM, &homedata_vreg },
	{ 0x7ffe, 0x7ffe, reikaids_blitter_bank_w },
	{ 0x7fff, 0x7fff, reikaids_blitter_start_w },
	{ 0x8000, 0x8000, bankswitch_w },
	{ 0x8002, 0x8002, reikaids_snd_command_w },
	{ 0x8005, 0x8005, reikaids_gfx_bank_w },
	{ 0x8006, 0x8006, homedata_blitter_param_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( reikaids_upd7807_readmem )
	{ 0x0000, 0xfeff, MRA_BANK2 },	/* External ROM (Banked) */
	{ 0xff00, 0xffff, MRA_RAM },	/* Internal RAM */
MEMORY_END

static MEMORY_WRITE_START( reikaids_upd7807_writemem )
	{ 0x0000, 0xfeff, MWA_ROM },
	{ 0xff00, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( reikaids_upd7807_readport )
	{ UPD7807_PORTA, UPD7807_PORTA, reikaids_upd7807_porta_r },
	{ UPD7807_PORTT, UPD7807_PORTT, reikaids_snd_command_r },
PORT_END

static PORT_WRITE_START( reikaids_upd7807_writeport )
	{ UPD7807_PORTA, UPD7807_PORTA, reikaids_upd7807_porta_w },
	{ UPD7807_PORTB, UPD7807_PORTB, DAC_0_signed_data_w },
	{ UPD7807_PORTC, UPD7807_PORTC, reikaids_upd7807_portc_w },
PORT_END


/**************************************************************************/


MEMORY_READ_START( pteacher_readmem )
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM }, /* work ram */
	{ 0x7800, 0x7800, MRA_RAM },
	{ 0x7801, 0x7801, pteacher_io_r },	// vblank, visible page
	{ 0x7ff2, 0x7ff2, pteacher_snd_r },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( pteacher_writemem )
	{ 0x0000, 0x3fff, pteacher_videoram_w, &videoram },
	{ 0x4000, 0x5eff, MWA_RAM },
	{ 0x5f00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7800, 0x7800, MWA_RAM },	/* behaves as normal RAM */
	{ 0x7ff0, 0x7ffd, MWA_RAM, &homedata_vreg },
	{ 0x7fff, 0x7fff, pteacher_blitter_start_w },
	{ 0x8000, 0x8000, bankswitch_w },
	{ 0x8002, 0x8002, pteacher_snd_command_w },
	{ 0x8005, 0x8005, pteacher_blitter_bank_w },
	{ 0x8006, 0x8006, homedata_blitter_param_w },
	{ 0x8007, 0x8007, pteacher_gfx_bank_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pteacher_upd7807_readmem )
	{ 0x0000, 0xfeff, MRA_BANK2 },	/* External ROM (Banked) */
	{ 0xff00, 0xffff, MRA_RAM },	/* Internal RAM */
MEMORY_END

static MEMORY_WRITE_START( pteacher_upd7807_writemem )
	{ 0x0000, 0x0000, pteacher_snd_answer_w },
	{ 0x0000, 0xfeff, MWA_ROM },
	{ 0xff00, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( pteacher_upd7807_readport )
	{ UPD7807_PORTA, UPD7807_PORTA, pteacher_upd7807_porta_r },
	{ UPD7807_PORTT, UPD7807_PORTT, pteacher_keyboard_r },
	{ UPD7807_PORTC, UPD7807_PORTC, input_port_1_r },
PORT_END

static PORT_WRITE_START( pteacher_upd7807_writeport )
	{ UPD7807_PORTA, UPD7807_PORTA, pteacher_upd7807_porta_w },
	{ UPD7807_PORTB, UPD7807_PORTB, DAC_0_signed_data_w },
	{ UPD7807_PORTC, UPD7807_PORTC, pteacher_upd7807_portc_w },
PORT_END


/**************************************************************************/


INPUT_PORTS_START( mjhokite )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Initial Score" )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (easiest)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (hardest)" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Girl Voice" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x02, 0x02, "Freeze?" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )	// doesn't work in all games
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( reikaids )
	PORT_START	// IN0  - 0x7801
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* punch */
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) /* kick */
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 ) /* jump */
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	// IN1 - 0x7802
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* punch */
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* kick */
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) /* jump */
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - 0x7803
	PORT_BIT(  0x01, IP_ACTIVE_HIGH,IPT_SPECIAL ) /* coprocessor status */
	PORT_BIT(  0x02, IP_ACTIVE_HIGH,IPT_SPECIAL ) /* coprocessor data */
	PORT_BIT(  0x04, IP_ACTIVE_HIGH,IPT_SPECIAL ) /* vblank */
	PORT_BIT(  0x08, IP_ACTIVE_HIGH,IPT_SPECIAL ) /* visible page */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,	IPT_COIN1    )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,	IPT_SERVICE1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW,	IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW,	IPT_UNKNOWN  )

	PORT_START	// DSW1
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20k then every 60k" )
	PORT_DIPSETTING(    0x04, "30k then every 80k" )
	PORT_DIPSETTING(    0x02, "20k" )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x20, "60" )
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// DSW2
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
INPUT_PORTS_END


#define MJ_KEYBOARD																				\
	PORT_START																					\
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )					\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "B",   KEYCODE_B,        IP_JOY_NONE )					\
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,        IP_JOY_NONE )					\
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,        IP_JOY_NONE )					\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )													\
																								\
	PORT_START																					\
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )					\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",   KEYCODE_F,        IP_JOY_NONE )					\
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,        IP_JOY_NONE )					\
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,        IP_JOY_NONE )					\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )													\
																								\
	PORT_START																					\
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )					\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "J",   KEYCODE_J,        IP_JOY_NONE )					\
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,        IP_JOY_NONE )					\
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,        IP_JOY_NONE )					\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )													\
																								\
	PORT_START																					\
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "M",    KEYCODE_M,        IP_JOY_NONE )					\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "N",    KEYCODE_N,        IP_JOY_NONE )					\
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Chi",  KEYCODE_SPACE, IP_JOY_NONE )						\
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon",  KEYCODE_LALT,  IP_JOY_NONE )						\
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Flip", KEYCODE_X,        IP_JOY_NONE )					\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )													\
																								\
	PORT_START																					\
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )					\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )					\
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )						\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )													\
																								\
	PORT_START																					\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )													\
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )					\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )												\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dip switch (handled separately */		\
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )


INPUT_PORTS_START( pteacher )
	PORT_START	/* dip switches (handled by pteacher_keyboard_r) */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	MJ_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START( jogakuen )
	PORT_START	/* dip switches (handled by pteacher_keyboard_r) */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	MJ_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START( mjikaga )
	PORT_START	/* dip switches (handled by pteacher_keyboard_r) */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	MJ_KEYBOARD
INPUT_PORTS_END


/**************************************************************************/


static struct GfxLayout char_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo mrokumei_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout, 0x6000, 0x100 },
	{ REGION_GFX2, 0, &char_layout, 0x7000, 0x100 },
	{ -1 }
};

static struct GfxLayout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static struct GfxDecodeInfo reikaids_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0x6000, 0x20 },
	{ REGION_GFX2, 0, &tile_layout, 0x4000, 0x20 },
	{ REGION_GFX3, 0, &tile_layout, 0x2000, 0x20 },
	{ REGION_GFX4, 0, &tile_layout, 0x0000, 0x20 },
	{ -1 }
};

static struct GfxDecodeInfo pteacher_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0x0000, 0x40 },
	{ REGION_GFX2, 0, &tile_layout, 0x4000, 0x40 },
	{ -1 }
};

static struct GfxLayout tile_layout_4bpp_hi =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static struct GfxLayout tile_layout_4bpp_lo =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static struct GfxDecodeInfo lemnangl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout_4bpp_hi, 0x0000, 0x200 },
	{ REGION_GFX1, 0, &tile_layout_4bpp_lo, 0x2000, 0x200 },
	{ REGION_GFX2, 0, &tile_layout_4bpp_lo, 0x4000, 0x200 },
	{ REGION_GFX2, 0, &tile_layout_4bpp_hi, 0x6000, 0x200 },
	{ -1 }
};



static struct SN76496interface sn76496_interface =
{
	1,
	{ 16000000/4 },	 /* 4MHz ? */
	{ 50 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};


static MACHINE_DRIVER_START( mrokumei )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 16000000/4)	/* 4MHz ? */
	MDRV_CPU_MEMORY(mrokumei_readmem,mrokumei_writemem)
	MDRV_CPU_VBLANK_INT(homedata_irq,1)	/* also triggered by the blitter */

	MDRV_CPU_ADD(Z80, 16000000/4)	/* 4MHz ? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU | CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(mrokumei_sound_readmem,mrokumei_sound_writemem)
	MDRV_CPU_PORTS(mrokumei_sound_readport,mrokumei_sound_writeport)

	MDRV_FRAMES_PER_SECOND(59)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	// visible area can be changed at runtime
	MDRV_VISIBLE_AREA(0*8, 54*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(mrokumei_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_PALETTE_INIT(mrokumei)
	MDRV_VIDEO_START(mrokumei)
	MDRV_VIDEO_UPDATE(mrokumei)
	MDRV_VIDEO_EOF(homedata)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)	// SN76489 actually
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


/**************************************************************************/


static struct YM2203interface ym2203_interface =
{
	1,
	3000000,	/* ? */
	{ YM2203_VOL(100,25) },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0	},
	{ 0 },
	{ NULL }
};


static struct DACinterface reikaids_dac_interface =
{
	1,
	{ 40 }
};


UPD7810_CONFIG cpu_config =
{
	TYPE_7810,
	NULL	/* io_callback */
};


static MACHINE_DRIVER_START( reikaids )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 16000000/4)	/* 4MHz ? */
	MDRV_CPU_MEMORY(reikaids_readmem,reikaids_writemem)
	MDRV_CPU_VBLANK_INT(homedata_irq,1)	/* also triggered by the blitter */

	MDRV_CPU_ADD(UPD7807, 8000000)	/* ??? MHz (max speed for the 7807 is 12MHz) */
	MDRV_CPU_CONFIG(cpu_config)
	MDRV_CPU_MEMORY(reikaids_upd7807_readmem,reikaids_upd7807_writemem)
	MDRV_CPU_PORTS(reikaids_upd7807_readport,reikaids_upd7807_writeport)
	MDRV_CPU_VBLANK_INT(upd7807_irq,1)

	MDRV_INTERLEAVE(500)	// very high interleave required to sync for startup tests

	MDRV_FRAMES_PER_SECOND(59)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(reikaids_upd7807)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 256-1-16)
	MDRV_GFXDECODE(reikaids_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_PALETTE_INIT(reikaids)
	MDRV_VIDEO_START(reikaids)
	MDRV_VIDEO_UPDATE(reikaids)
	MDRV_VIDEO_EOF(homedata)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(DAC, reikaids_dac_interface)
MACHINE_DRIVER_END


/**************************************************************************/

static MACHINE_DRIVER_START( pteacher )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 16000000/4)	/* 4MHz ? */
	MDRV_CPU_MEMORY(pteacher_readmem,pteacher_writemem)
	MDRV_CPU_VBLANK_INT(homedata_irq,1)	/* also triggered by the blitter */

	MDRV_CPU_ADD_TAG("sound", UPD7807, 9000000)	/* 9MHz ? */
	MDRV_CPU_CONFIG(cpu_config)
	MDRV_CPU_MEMORY(pteacher_upd7807_readmem,pteacher_upd7807_writemem)
	MDRV_CPU_PORTS(pteacher_upd7807_readport,pteacher_upd7807_writeport)
	MDRV_CPU_VBLANK_INT(upd7807_irq,1)

	MDRV_INTERLEAVE(100)	// should be enough

	MDRV_FRAMES_PER_SECOND(59)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pteacher_upd7807)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	// visible area can be changed at runtime
	MDRV_VISIBLE_AREA(0*8, 54*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(pteacher_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_PALETTE_INIT(pteacher)
	MDRV_VIDEO_START(pteacher)
	MDRV_VIDEO_UPDATE(pteacher)
	MDRV_VIDEO_EOF(homedata)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)	// SN76489 actually
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mjkinjas )

	MDRV_IMPORT_FROM(pteacher)

	MDRV_CPU_REPLACE("sound", UPD7807, 11000000)	/* 11MHz ? */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( lemnangl )

	MDRV_IMPORT_FROM(pteacher)

	/* video hardware */
	MDRV_GFXDECODE(lemnangl_gfxdecodeinfo)

	MDRV_VIDEO_START(lemnangl)
MACHINE_DRIVER_END



/**************************************************************************/


ROM_START( hourouki )
	ROM_REGION( 0x010000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x77f01.bin", 0x08000, 0x8000, 0xcd3197b8 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "x77a10.bin", 0x00000, 0x20000, 0xdc1d616b )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "x77a03.bin", 0, 0x20000, 0x5960cde8 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "x77a04.bin", 0x00000, 0x20000, 0xfd348e59 )
	ROM_LOAD( "x77a05.bin", 0x20000, 0x20000, 0x3f76c8af )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x77e06.bin", 0x00000, 0x8000, 0x63607fe5 )
	ROM_LOAD16_BYTE( "x77e07.bin", 0x00001, 0x8000, 0x79fcfc57 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x77a08.bin", 0x0000, 0x20000, 0x22bde229 )
ROM_END

ROM_START( mhgaiden )
	ROM_REGION( 0x010000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x72e01.bin", 0x08000, 0x8000, 0x98cfa53e )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "x72b10.bin", 0x00000, 0x20000, 0x00ebbc45 )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "x72b03.bin", 0, 0x20000, 0x9019936f )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "x72b04.bin", 0x00000, 0x20000, 0x37e3e779 )
	ROM_LOAD( "x72b05.bin", 0x20000, 0x20000, 0xaa5ce6f6 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x72c06.bin", 0x00000, 0x8000, 0xb57fb589 )
	ROM_LOAD16_BYTE( "x72c07.bin", 0x00001, 0x8000, 0x2aadb285 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x72b08.bin", 0x0000, 0x20000, 0xbe312d23 )
ROM_END

ROM_START( mjhokite )
	ROM_REGION( 0x010000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "a74_g01.6g", 0x08000, 0x8000, 0x409cc501 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "a74_a10.11k", 0x00000, 0x20000, 0x2252f3ec )
	ROM_RELOAD(              0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a74_a03.1g", 0, 0x20000, 0xbf801b74 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a74_a040.bin", 0x00000, 0x20000, 0xb7a4ddbd )
	ROM_LOAD( "a74_a050.bin", 0x20000, 0x20000, 0xc1718d39 )
	ROM_LOAD( "a74_a041.bin", 0x40000, 0x20000, 0xc6a6407d )
	ROM_LOAD( "a74_a051.bin", 0x60000, 0x20000, 0x74522b81 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "a74_a06.1l", 0x00000, 0x8000, 0xdf057dd3 )
	ROM_LOAD16_BYTE( "a74_a07.1m", 0x00001, 0x8000, 0x3c230167 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "a74_a08.13a", 0x0000, 0x20000, 0xdffdd855 )
ROM_END

ROM_START( mjclinic )
	ROM_REGION( 0x010000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x80_g01.6g", 0x08000, 0x8000, 0x787b4fb5 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "x80_a10.11k", 0x00000, 0x20000, 0xafedbadf )
	ROM_RELOAD(              0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "x80_a03.1g", 0, 0x20000, 0x34b63c89 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "x80_a040.bin", 0x00000, 0x20000, 0x6f18a8cf )
	ROM_LOAD( "x80_a050.bin", 0x20000, 0x20000, 0x6b1ec3a9 )
	ROM_LOAD( "x80_a041.bin", 0x40000, 0x20000, 0xf70bb001 )
	ROM_LOAD( "x80_a051.bin", 0x60000, 0x20000, 0xc7469cb8 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x80_a06.1l", 0x00000, 0x8000, 0xc1f9b2fb )
	ROM_LOAD16_BYTE( "x80_a07.1m", 0x00001, 0x8000, 0xe3120152 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x80_a08.13a", 0x0000, 0x20000, 0x174e8ec0 )
ROM_END

ROM_START( mrokumei )
	ROM_REGION( 0x010000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "m81d01.bin", 0x08000, 0x8000, 0x6f81a78a )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "m81a10.bin", 0x00000, 0x20000, 0x0866b2d3 )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m81a03.bin", 0, 0x20000, 0x4f96e6d2 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "m81a40.bin", 0x00000, 0x20000, 0xf07c6a91 )
	ROM_LOAD( "m81a50.bin", 0x20000, 0x20000, 0x5ef0d7f2 )
	ROM_LOAD( "m81a41.bin", 0x40000, 0x20000, 0x9332b879 )
	ROM_LOAD( "m81a51.bin", 0x60000, 0x20000, 0xdda3ae30 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "m81b06.bin", 0x00000, 0x8000, 0x96665d39 )
	ROM_LOAD16_BYTE( "m81b07.bin", 0x00001, 0x8000, 0x14f39690 )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "m81a08.bin", 0x0000, 0x20000, 0xdba706b9 )
ROM_END


ROM_START( reikaids )
	ROM_REGION( 0x02c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "j82c01.bin", 0x010000, 0x01c000, 0x50fcc451 )
	ROM_CONTINUE(           0x00c000, 0x004000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x82a04.bin", 0x000000, 0x040000, 0x52c9028a )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a13.bin",  0x000000, 0x80000, 0x954c8844 )
	ROM_LOAD( "x82a14.bin",  0x080000, 0x80000, 0xa748305e )
	ROM_LOAD( "x82a15.bin",  0x100000, 0x80000, 0xc50f7047 )
	ROM_LOAD( "x82a16.bin",  0x180000, 0x80000, 0xb270094a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a09.bin",  0x000000, 0x80000, 0xc496b187 )
	ROM_LOAD( "x82a10.bin",  0x080000, 0x80000, 0x4243fe28 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a08.bin",  0x000000, 0x80000, 0x51cfd790 )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a05.bin",  0x000000, 0x80000, 0xfb65e0e0 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "e82a18.bin", 0x00000, 0x8000, 0x1f52a7aa )
	ROM_LOAD16_BYTE( "e82a17.bin", 0x00001, 0x8000, 0xf91d77a1 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x82a02.bin", 0x00000, 0x040000, 0x90fe700f )

	ROM_REGION( 0x0100, REGION_USER2, 0 )
	ROM_LOAD( "x82a19.bin", 0x0000, 0x0100, 0x7ed947b4 )	// priority (not used)
ROM_END


ROM_START( mjkojink )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x83j01.16e", 0x010000, 0xc000, 0x91f90376 )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x83b02.9g",  0x00000, 0x40000, 0x46a11578 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x83b14.1f",  0, 0x40000, 0x2bcd7557 )
	ROM_LOAD32_BYTE( "x83b15.3f",  1, 0x40000, 0x7d780e22 )
	ROM_LOAD32_BYTE( "x83b16.4f",  2, 0x40000, 0x5420a3f2 )
	ROM_LOAD32_BYTE( "x83b17.6f",  3, 0x40000, 0x96bcdf83 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x83b10.1c",  0, 0x40000, 0x500bfeea )
	ROM_LOAD32_BYTE( "x83b11.3c",  1, 0x40000, 0x2ef77717 )
	ROM_LOAD32_BYTE( "x83b12.4c",  2, 0x40000, 0x2035009d )
	ROM_LOAD32_BYTE( "x83b13.6c",  3, 0x40000, 0x53800df2 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x83a19.4k", 0x00000, 0x8000, 0xd29c9ef0 )
	ROM_LOAD16_BYTE( "x83a18.3k", 0x00001, 0x8000, 0xc3351952 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x83b03.12e", 0x0000, 0x40000, 0x4ba8b5ec )
ROM_END

ROM_START( vitaminc )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x90e01.bin", 0x010000, 0xc000, 0xbc982525 )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x90a02.bin", 0x00000, 0x40000, 0x811f540a )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x90a14.bin", 0, 0x40000, 0x4b49d182 )
	ROM_LOAD32_BYTE( "x90a15.bin", 1, 0x40000, 0x5e9016c2 )
	ROM_LOAD32_BYTE( "x90a16.bin", 2, 0x40000, 0xb8843000 )
	ROM_LOAD32_BYTE( "x90a17.bin", 3, 0x40000, 0xd74a843c )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x90a10.bin", 0, 0x40000, 0xee9fa36f )
	ROM_LOAD32_BYTE( "x90a11.bin", 1, 0x40000, 0xb77d9ef4 )
	ROM_LOAD32_BYTE( "x90a12.bin", 2, 0x40000, 0xda6a65d1 )
	ROM_LOAD32_BYTE( "x90a13.bin", 3, 0x40000, 0x4da4553b )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x90b19.bin", 0x00000, 0x8000, 0xd0022cfb )
	ROM_LOAD16_BYTE( "x90b18.bin", 0x00001, 0x8000, 0xfe1de95d )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x90a03.bin", 0x0000, 0x40000, 0x35d5b4e6 )
ROM_END

ROM_START( mjyougo )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x91c01.bin", 0x010000, 0xc000, 0xe28e8c21 )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x91a02.bin", 0x00000, 0x40000, 0x995b1399 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x91a14.bin", 0, 0x40000, 0xb165fbe8 )
	ROM_LOAD32_BYTE( "x91a15.bin", 1, 0x40000, 0x9b60bf2e )
	ROM_LOAD32_BYTE( "x91a16.bin", 2, 0x40000, 0xdb4a1655 )
	ROM_LOAD32_BYTE( "x91a17.bin", 3, 0x40000, 0x4f35ec3b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x91a10.bin", 0, 0x40000, 0xcb364158 )
	ROM_LOAD32_BYTE( "x91a11.bin", 1, 0x40000, 0xf3655577 )
	ROM_LOAD32_BYTE( "x91a12.bin", 2, 0x40000, 0x149e8f86 )
	ROM_LOAD32_BYTE( "x91a13.bin", 3, 0x40000, 0x59f7a140 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x91a19.bin", 0x00000, 0x8000, 0xf63493df )
	ROM_LOAD16_BYTE( "x91a18.bin", 0x00001, 0x8000, 0xb3541265 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x91a03.bin", 0x0000, 0x40000, 0x4863caa2 )
ROM_END

ROM_START( mjkinjas )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x07c01.bin", 0x010000, 0xc000, 0xe6534904 )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x07a02.bin", 0x00000, 0x40000, 0x31396a5b )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x07a14.bin", 0, 0x80000, 0x02829ede )
	ROM_LOAD32_BYTE( "x07a15.bin", 1, 0x80000, 0x9c8b55db )
	ROM_LOAD32_BYTE( "x07a16.bin", 2, 0x80000, 0x7898a340 )
	ROM_LOAD32_BYTE( "x07a17.bin", 3, 0x80000, 0xbf1f6540 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x07a10.bin", 0, 0x80000, 0x3bfab66e )
	ROM_LOAD32_BYTE( "x07a11.bin", 1, 0x80000, 0xe8f610e3 )
	ROM_LOAD32_BYTE( "x07a12.bin", 2, 0x80000, 0x911f0972 )
	ROM_LOAD32_BYTE( "x07a13.bin", 3, 0x80000, 0x59be4c77 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x07a19.bin", 0x00000, 0x8000, 0x7acabdf8 )
	ROM_LOAD16_BYTE( "x07a18.bin", 0x00001, 0x8000, 0xd247bd5a )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x07a03.bin", 0x0000, 0x80000, 0xf5ff3e72 )
ROM_END

ROM_START( jogakuen )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "a01.bin",    0x010000, 0xc000, 0xa189490a )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "a02.bin",    0x00000, 0x40000, 0x033add6c )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a14.bin",    0, 0x80000, 0x27ad91d7 )
	ROM_LOAD32_BYTE( "a15.bin",    1, 0x80000, 0xe3b2753b )
	ROM_LOAD32_BYTE( "a16.bin",    2, 0x80000, 0x6e2c61fc )
	ROM_LOAD32_BYTE( "a17.bin",    3, 0x80000, 0x2f79d467 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a10.bin",    0, 0x80000, 0xa453759a )
	ROM_LOAD32_BYTE( "a11.bin",    1, 0x80000, 0x252cf007 )
	ROM_LOAD32_BYTE( "a12.bin",    2, 0x80000, 0x5db85eb5 )
	ROM_LOAD32_BYTE( "a13.bin",    3, 0x80000, 0xfe04d5b7 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "a19.bin",    0x00000, 0x8000, 0x9a3d9d5e )
	ROM_LOAD16_BYTE( "a18.bin",    0x00001, 0x8000, 0x3289edd4 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "a03.bin",    0x0000, 0x40000, 0xbb1507ab )
ROM_END


ROM_START( lemnangl )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x02_d01.16e", 0x010000, 0xc000, 0x4c2fae05 )
	ROM_CONTINUE(            0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "x02a02.9g",  0x00000, 0x40000, 0xe9aa8c80 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x02a14.1f",  0, 0x40000, 0x4aa2397b )
	ROM_LOAD32_BYTE( "x02a15.3f",  1, 0x40000, 0xd01986e2 )
	ROM_LOAD32_BYTE( "x02a16.4f",  2, 0x40000, 0x16fca216 )
	ROM_LOAD32_BYTE( "x02a17.6f",  3, 0x40000, 0x7a6a96e7 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x02a10.1c",  0, 0x40000, 0xe7164f57 )
	ROM_LOAD32_BYTE( "x02a11.3c",  1, 0x40000, 0x73fb5d3d )
	ROM_LOAD32_BYTE( "x02a12.4c",  2, 0x40000, 0xfc3a254a )
	ROM_LOAD32_BYTE( "x02a13.6c",  3, 0x40000, 0x9f63e7e0 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x02_b19.5k", 0x00000, 0x8000, 0xf75959bc )
	ROM_LOAD16_BYTE( "x02_b18.3k", 0x00001, 0x8000, 0x3f1510b1 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x02a03.12e", 0x0000, 0x40000, 0x02ef0378 )
ROM_END

ROM_START( mjikaga )
	ROM_REGION( 0x01c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "m15a01.bin", 0x010000, 0xc000, 0x938cc4fb )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x40000, REGION_CPU2, 0) /* uPD7807 code */
	ROM_LOAD( "m15a02.bin", 0x00000, 0x40000, 0x375933dd )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "m15a14.bin", 0, 0x40000, 0xa685c452 )
	ROM_LOAD32_BYTE( "m15a15.bin", 1, 0x40000, 0x44153914 )
	ROM_LOAD32_BYTE( "m15a16.bin", 2, 0x40000, 0xa4b0b8ac )
	ROM_LOAD32_BYTE( "m15a17.bin", 3, 0x40000, 0xbb9cb2ef )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "m15a10.bin", 0, 0x40000, 0x0aeed38e )
	ROM_LOAD32_BYTE( "m15a11.bin", 1, 0x40000, 0xa305e6e6 )
	ROM_LOAD32_BYTE( "m15a12.bin", 2, 0x40000, 0x946b3f55 )
	ROM_LOAD32_BYTE( "m15a13.bin", 3, 0x40000, 0xd9196955 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "m15a19.bin", 0x00000, 0x8000, 0x2f247acf )
	ROM_LOAD16_BYTE( "m15a18.bin", 0x00001, 0x8000, 0x2648ca07 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "m15a03.bin", 0x0000, 0x40000, 0x07e2e8f8 )
ROM_END



static DRIVER_INIT( jogakuen )
{
	/* it seems that Mahjong Jogakuen runs on the same board as the others,
	   but with just these two addresses swapped. Instead of creating a new
	   MachineDriver, I just fix them here. */
	install_mem_write_handler(0, 0x8007, 0x8007, pteacher_blitter_bank_w);
	install_mem_write_handler(0, 0x8005, 0x8005, pteacher_gfx_bank_w);
}

static DRIVER_INIT( mjikaga )
{
	/* Mahjong Ikagadesuka is different as well. */
	install_mem_read_handler(0, 0x7802, 0x7802, pteacher_snd_r);
	install_mem_write_handler(1, 0x0123, 0x0123, pteacher_snd_answer_w);
}


GAMEX(1987, hourouki, 0, mrokumei, mjhokite, 0,        ROT0, "Home Data", "Mahjong Hourouki Part 1 - Seisyun Hen (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1987, mhgaiden, 0, mrokumei, mjhokite, 0,        ROT0, "Home Data", "Mahjong Hourouki Gaiden (Japan)" )
GAME( 1988, mjhokite, 0, mrokumei, mjhokite, 0,        ROT0, "Home Data", "Mahjong Hourouki Okite (Japan)" )
GAME( 1988, mjclinic, 0, mrokumei, mjhokite, 0,        ROT0, "Home Data", "Mahjong Clinic (Japan)" )
GAMEX(1988, mrokumei, 0, mrokumei, mjhokite, 0,        ROT0, "Home Data", "Mahjong Rokumeikan (Japan)", GAME_IMPERFECT_GRAPHICS )

GAME( 1988, reikaids, 0, reikaids, reikaids, 0,        ROT0, "Home Data", "Reikai Doushi (Japan)" )

GAME( 1989, mjkojink, 0, pteacher, pteacher, 0,        ROT0, "Home Data", "Mahjong Kojinkyouju (Private Teacher) (Japan)" )
GAME( 1989, vitaminc, 0, pteacher, pteacher, 0,        ROT0, "Home Data", "Mahjong Vitamin C (Japan)" )
GAME( 1989, mjyougo,  0, pteacher, pteacher, 0,        ROT0, "Home Data", "Mahjong-yougo no Kisotairyoku (Japan)" )
GAME( 1991, mjkinjas, 0, mjkinjas, pteacher, 0,        ROT0, "Home Data", "Mahjong Kinjirareta Asobi (Japan)" )
GAME( 1992?,jogakuen, 0, pteacher, jogakuen, jogakuen, ROT0, "Windom",    "Mahjong Jogakuen (Japan)" )

GAME( 1990, lemnangl, 0, lemnangl, pteacher, 0,        ROT0, "Home Data", "Mahjong Lemon Angel (Japan)" )

GAMEX(1991?,mjikaga,  0, lemnangl, mjikaga,  mjikaga,  ROT0, "Mitchell",  "Mahjong Ikaga Desu ka (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND )
