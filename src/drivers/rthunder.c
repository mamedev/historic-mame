/*******************************************************************
Rolling Thunder
(C) 1986 Namco

To Do:
-----
Fix PSG sound (some effect don't play, some where working in previous versions)
Further optimize video (skip transparent/obscured tiles)
Add voice (see note below)
Add flipped and cocktail cabinet mode
Remove sprite lag (watch the "bullets" signs on the walls during scrlling).
  Increasing vblank_duration does it but some sprites flicker.


Notes:
-----
PCM roms sample tables:
At the beggining of each PCM sound ROM you can find a 2 byte
offset to the beggining of each sample in the rom. Since the
table is not in sequential order, it is possible that the order
of the table is actually the sound number. Each sample ends in
a 0xff mark.

*******************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6808/m6808.h"

/* This is the clock speed in Hz divided by 64 that is connected to the E pin on the MCU */
/* Each pulse on the E pin increments timer A, wich is used for music tempo. */
/* I've made the decision to downgrade the timer accuracy to allow better speed. */
/* Looking at the code tho, its more than accurate for its purpose. */
#define MCU_E_CLK (1500000/64) /* Hz */

extern unsigned char *rthunder_videoram, *spriteram, *dirtybuffer;

/*******************************************************************/

extern void rthunder_vh_convert_color_prom(	unsigned char *palette, unsigned short *colortable,
	const unsigned char *color_prom);
extern int rthunder_vh_start( void );
extern void rthunder_vh_stop( void );
extern void rthunder_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern int rthunder_videoram_r(int offset);
extern void rthunder_videoram_w(int offset,int data);
extern void rthunder_scroll_w( int layer, int offset, int data );
extern void rthunder_tilebank_w( int offset, int data );

/*******************************************************************/

#define MEM_CPU1		0
#define MEM_DATA1		1

#define MEM_CPU2		2
#define MEM_DATA2		3

#define MEM_GFX_TILES	4
#define MEM_GFX_SPRITES	5
#define MEM_COLOR		6
#define MEM_MCU			7

/*******************************************************************/

/* You can turn on/off cycle skipping in the next line */
#define CYCLE_SKIP 1

#if CYCLE_SKIP
static int cpu0_skip_r( int offs ) {

	if ( spriteram[0x126f] == spriteram[0x126e] && cpu_getpc() == 0xd64c ) {
		if ( spriteram[0x12b4] == spriteram[0x12b3] ) {
			cpu_spinuntil_int();
		}
	}

	return spriteram[0x126f];
}

static int cpu1_skip_r( int offs ) {

	if ( spriteram[0x1268] == spriteram[0x1267] && cpu_getpc() == 0xb0c1 )
		cpu_spinuntil_int();

	return spriteram[0x1268];
}
#endif

/*******************************************************************/

/* play voice sample */
static int voice[2];

static void namco_voice0_play_w( int offset, int data ) {
	if ( errorlog )
		fprintf( errorlog, "Voice 0 triggered\n" );
}

static void namco_voice0_select_w( int offset, int data ) {
	if ( errorlog )
		fprintf( errorlog, "Voice 0 select: %02x\n", data );
}

static void namco_voice1_play_w( int offset, int data ) {
	if ( errorlog )
		fprintf( errorlog, "Voice 1 triggered\n" );
}

static void namco_voice1_select_w( int offset, int data ) {
	if ( errorlog )
		fprintf( errorlog, "Voice 1 select: %02x\n", data );
}

/*******************************************************************/

/* shared memory area with the mcu */
static unsigned char *shared;
static int shared_r( int offs ) { return shared[offs]; }
static void shared_w( int offs, int data ) { shared[offs] = data; }

/*
	each layer has its own pair of scroll registers, but in practice
	layers 0,1,2 scroll together, while layer3 is fixed
*/
static void scroll0_w( int offset, int data ){ rthunder_scroll_w( 0, offset, data ); }
static void scroll1_w( int offset, int data ){ rthunder_scroll_w( 1, offset, data ); }
static void scroll2_w( int offset, int data ){ rthunder_scroll_w( 2, offset, data ); }
static void scroll3_w( int offset, int data ){ rthunder_scroll_w( 3, offset, data ); }

static void spriteram_w( int offset, int data ){
	spriteram[offset] = data;
}
static int spriteram_r( int offset ){
	return spriteram[offset];
}

static void bankswitch1_w( int offset, int data ){
	unsigned char *base = Machine->memory_region[MEM_DATA1];
	cpu_setbank( 1, base + ((data&0x1f)*0x2000) );
	if( data&0xe0 && errorlog ) fprintf( errorlog, "big bank\n" );
}

static void bankswitch2_w( int offset, int data ){
	unsigned char *base = Machine->memory_region[MEM_DATA2];
	cpu_setbank( 2, base + ((data&0x03)*0x2000) );
}

/* Stubs to pass the correct Dip Switch setup to the MCU */
static int dsw_r0( int offset )
{
	int rhi, rlo;

	rhi = ( readinputport( 2 ) & 0x01 ) << 4;
	rhi |= ( readinputport( 2 ) & 0x04 ) << 3;
	rhi |= ( readinputport( 2 ) & 0x10 ) << 2;
	rhi |= ( readinputport( 2 ) & 0x40 ) << 1;

	rlo = ( readinputport( 3 ) & 0x01 );
	rlo |= ( readinputport( 3 ) & 0x04 ) >> 1;
	rlo |= ( readinputport( 3 ) & 0x10 ) >> 2;
	rlo |= ( readinputport( 3 ) & 0x40 ) >> 3;

	return ~( rhi | rlo ) & 0xff; /* Active Low */
}

static int dsw_r1( int offset )
{
	int rhi, rlo;

	rhi = ( readinputport( 2 ) & 0x02 ) << 3;
	rhi |= ( readinputport( 2 ) & 0x08 ) << 2;
	rhi |= ( readinputport( 2 ) & 0x20 ) << 1;
	rhi |= ( readinputport( 2 ) & 0x80 );

	rlo = ( readinputport( 3 ) & 0x02 ) >> 1;
	rlo |= ( readinputport( 3 ) & 0x08 ) >> 2;
	rlo |= ( readinputport( 3 ) & 0x20 ) >> 3;
	rlo |= ( readinputport( 3 ) & 0x80 ) >> 4;

	return ~( rhi | rlo ) & 0xff; /* Active Low */
}

static int int_enabled[2];

static void int_enable_w( int offs, int data ) {
	int cpu = cpu_getactivecpu();

	int_enabled[cpu] = 1;
}

static void *rt_timer = 0;
static unsigned char *hd_regs;


static void rt_timer_proc( int param )
{
	int current_time, total_time, old_time;

	current_time = ( hd_regs[0x09] << 8 ) | hd_regs[0x0a];
	total_time = ( hd_regs[0x0b] << 8 ) | hd_regs[0x0c];

	old_time = current_time;

	/* increment timer A */
	current_time += 64;

	/* update time */
	hd_regs[0x09] = ( current_time >> 8 ) & 0xff;
	hd_regs[0x0a] = current_time & 0xff;

	if ( total_time >= old_time && total_time <= current_time ) {
		if ( hd_regs[0x08] & 8 )
			cpu_cause_interrupt( 2, M6808_INT_OCI );
	}
}

void rt_stop_mcu_timer( void ) {
	if ( rt_timer ) {
		timer_remove( rt_timer );
		rt_timer = 0;
	}
}

static int hd_regs_r( int offset )
{
	return hd_regs[offset+8];
}

static void hd_regs_w( int offset, int data )
{
	hd_regs[offset+8] = data;
}

static void mcu_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(2,M6808_INT_IRQ);
}


/*******************************************************************/

static struct MemoryReadAddress readmem1[] =
{
	{ 0x0000, 0x3fff, rthunder_videoram_r },
	{ 0x4000, 0x40ff, namcos1_wavedata_r }, /* PSG device, shared RAM */
	{ 0x4100, 0x413f, namcos1_sound_r }, /* PSG device, shared RAM */
	{ 0x4000, 0x43ff, shared_r },

#if CYCLE_SKIP
	{ 0x566f, 0x566f, cpu0_skip_r },
#endif

	{ 0x4400, 0x5fff, spriteram_r },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem1[] =
{
	{ 0x0000, 0x3fff, rthunder_videoram_w, &rthunder_videoram },
	{ 0x4000, 0x40ff, namcos1_wavedata_w, &namco_wavedata }, /* PSG device, shared RAM */
	{ 0x4100, 0x413f, namcos1_sound_w, &namco_soundregs }, /* PSG device, shared RAM */
	{ 0x4000, 0x43ff, shared_w, &shared },
	{ 0x4400, 0x5fff, spriteram_w, &spriteram },

	{ 0x6000, 0x6000, namco_voice0_play_w },
	{ 0x6200, 0x6200, namco_voice0_select_w },
	{ 0x6400, 0x6400, namco_voice1_play_w },
	{ 0x6600, 0x6600, namco_voice1_select_w },
	{ 0x6800, 0x6800, bankswitch1_w },

//	{ 0x6c00, 0x6c00, MWA_NOP }, /* written once on startup - background color? */
//	{ 0x6e00, 0x6e00, MWA_NOP }, /* written once on startup - background color? */

	{ 0x8000, 0x8000, watchdog_reset_w }, /* watchdog? */
	{ 0x8400, 0x8400, int_enable_w }, /* written when CPU1 is done its interrupt routine */

	{ 0x8800, 0x8800, rthunder_tilebank_w },
	{ 0x8c00, 0x8c00, rthunder_tilebank_w },

	{ 0x9000, 0x9002, scroll0_w },
	{ 0x9004, 0x9006, scroll1_w },
	{ 0x9400, 0x9402, scroll2_w },
	{ 0x9404, 0x9406, scroll3_w },

	{ 0xa000, 0xa000, mcu_irqtrigger_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};

static struct MemoryReadAddress readmem2[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },

#if CYCLE_SKIP
	{ 0x1668, 0x1668, cpu1_skip_r },
#endif

	{ 0x0400, 0x1fff, spriteram_r },
	{ 0x2000, 0x5fff, rthunder_videoram_r },
	{ 0x6000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x1fff, spriteram_w },
	{ 0x2000, 0x5fff, rthunder_videoram_w },
	{ 0xd803, 0xd803, bankswitch2_w },
	{ 0x8000, 0x8000, watchdog_reset_w }, /* (rthunder) watchdog? */
	{ 0x8800, 0x8800, int_enable_w },	/* (rthunder) written when CPU2 is done its interrupt */
	{ 0xc000, 0xc000, watchdog_reset_w }, /* (wndrmomo) watchdog? */
	{ 0xc800, 0xc800, int_enable_w },	/* (wndrmomo) written when CPU2 is done its interrupt */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};

static struct MemoryReadAddress mcu_readmem[] =
{
	{ 0x0002, 0x0002, input_port_4_r },
	{ 0x0008, 0x000c, hd_regs_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_r }, /* PSG device, shared RAM */
	{ 0x1100, 0x113f, namcos1_sound_r }, /* PSG device, shared RAM */
	{ 0x1000, 0x13ff, shared_r },
	{ 0x1400, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2020, 0x2020, input_port_0_r },
	{ 0x2021, 0x2021, input_port_1_r },
	{ 0x2030, 0x2030, dsw_r0 },
	{ 0x2031, 0x2031, dsw_r1 },
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mcu_writemem[] =
{
	{ 0x0000, 0x0003, MWA_NOP },	/* port 1 & 2 data & ddr TODO: support correctly */
	{ 0x0008, 0x000c, hd_regs_w, &hd_regs },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_w }, /* PSG device, shared RAM */
	{ 0x1100, 0x113f, namcos1_sound_w }, /* PSG device, shared RAM */
	{ 0x1000, 0x13ff, shared_w },
	{ 0x1400, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP }, /* ??? */
	{ 0xb800, 0xb800, MWA_NOP }, /* ??? */
	{ 0x4000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/*******************************************************************/

INPUT_PORTS_START( rthunder_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", OSD_KEY_F1, IP_JOY_NONE, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPNAME( 0x04, 0x00, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x60, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x60, "3 Coins/1 Credit" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, "Continues", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x06, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
/*	PORT_DIPSETTING(    0x04, "Upright 1 Player" ) */
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x06, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Level Select", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPNAME( 0x20, 0x20, "Timer value", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "120 secs" )
	PORT_DIPSETTING(    0x20, "150 secs" )
	PORT_DIPNAME( 0x40, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "70K, 200K" )
	PORT_DIPSETTING(    0x40, "100K, 300K" )
	PORT_DIPNAME( 0x80, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( rthundr2_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", OSD_KEY_F1, IP_JOY_NONE, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPNAME( 0x04, 0x00, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x60, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x60, "3 Coins/1 Credit" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0xc0, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
INPUT_PORTS_END

/*******************************************************************/

static struct GfxLayout tile_layout1 =
{
	8,8,
	2048*2,
	3,
	{ 0xc000*8*2, 0, 0xc000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tile_layout2 =
{
	8,8,
	2048*1,
	3,
	{ 0xc000*8*2, 0, 0xc000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout sprite_layout =
{
	16,16,
	4096,
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*64
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ MEM_GFX_TILES,   0x00000, &tile_layout1,  2048*0, 256 },
	{ MEM_GFX_TILES,   0x08000, &tile_layout2,  2048*0, 256 },
	{ MEM_GFX_SPRITES, 0x00000, &sprite_layout, 2048*1, 128 },
	{ -1 }
};

/*******************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579580,                /* 3.579580 MHz ? */
	{ YM3012_VOL(0,OSD_PAN_CENTER,30,OSD_PAN_CENTER) },	/* only right channel is connected */
	{ 0 },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	24000,  /* sample rate (hand tuned) */
	8,		/* number of voices */
	16,		/* gain adjustment */
	50,     /* playback volume */
	-1,		/* memory region */
	0		/* stereo */
};


static void rt_init_machine( void ) {
	int_enabled[0] = int_enabled[1] = 1;

	/* stop timer if running */
	rt_stop_mcu_timer();

	/* make sure we dont trigger interrupts */
	hd_regs[0x08] = 0;

	/* Init timer A */
	hd_regs[0x09] = 0;
	hd_regs[0x0a] = 0;

	/* start up Timer A */
	rt_timer = timer_pulse( TIME_IN_HZ( MCU_E_CLK ), 0,rt_timer_proc );
}

static int rt_interrupt( void ) {
	int cpu = cpu_getactivecpu();

	if ( int_enabled[cpu] ) {
		int_enabled[cpu] = 0;
		return interrupt();
	}

	return ignore_interrupt();
}

static struct MachineDriver machine_driver =
{
	{
		{
			CPU_M6809,
			1500000, 		/* ??? */
			MEM_CPU1,
			readmem1,writemem1,0,0,
			rt_interrupt,1
		},
		{
			CPU_M6809,
			1500000, 		/* ??? */
			MEM_CPU2,
			readmem2,writemem2,0,0,
			rt_interrupt,1
		},
		{
			CPU_HD63701,	/* or compatible 6808 with extra instructions */
			3000000,		/* ??? */
			MEM_MCU,
			mcu_readmem,mcu_writemem,0,0,
			ignore_interrupt, 1 /* irq's triggered by the main cpu */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	100, /* cpu slices */
	&rt_init_machine, /* init machine */

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	256,4096,
	rthunder_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	rthunder_vh_start,
	rthunder_vh_stop,
	rthunder_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rthunder_rom )
	ROM_REGION( 0x10000 ) /* 6809 code for CPU1 */
	ROM_LOAD( "rt3-1b.9c",  0x8000, 0x8000, 0x7d252a1b )

	ROM_REGION( 0x40000 ) /* bank switched data for CPU1 */
	ROM_LOAD( "rt1-17.f1",  0x00000, 0x10000, 0x766af455 )
	ROM_LOAD( "rt1-18.h1",  0x10000, 0x10000, 0x3f9f2f5d )
	ROM_LOAD( "rt1-19.k1",  0x20000, 0x10000, 0xc16675e9 )
	ROM_LOAD( "rt1-20.m1",  0x30000, 0x10000, 0xc470681b )

	ROM_REGION( 0x10000 ) /* 6809 code for CPU2 */
	ROM_LOAD( "rt3-2b.12c", 0x8000, 0x8000, 0xa7ea46ee )

	ROM_REGION( 0x8000 ) /* bank switched data for CPU2 */
	ROM_LOAD( "rt3-3.12d", 0x0000, 0x8000, 0xa13f601c )

	ROM_REGION( 0x24000 ) /* tiles */
	ROM_LOAD( "rt1-7.7r",  0x00000, 0x10000, 0xa85efa39 )	/* plane 1,2 */
	ROM_LOAD( "rt1-5.4r",  0x10000, 0x08000, 0xd0fc470b )
	ROM_LOAD( "rt1-8.7s",  0x18000, 0x08000, 0xf7a95820 )	/* plane 3 */
	ROM_LOAD( "rt1-6.4s",  0x20000, 0x04000, 0x6b57edb2 )

	ROM_REGION( 0x80000 ) /* sprites */
 	ROM_LOAD( "rt1-9.12h",  0x00000, 0x10000, 0x8e070561 )
 	ROM_LOAD( "rt1-10.12k", 0x10000, 0x10000, 0xcb8fb607 )
 	ROM_LOAD( "rt1-11.12l", 0x20000, 0x10000, 0x2bdf5ed9 )
 	ROM_LOAD( "rt1-12.12m", 0x30000, 0x10000, 0xe6c6c7dc )
 	ROM_LOAD( "rt1-13.12p", 0x40000, 0x10000, 0x489686d7 )
 	ROM_LOAD( "rt1-14.12r", 0x50000, 0x10000, 0x689e56a8 )
 	ROM_LOAD( "rt1-15.12t", 0x60000, 0x10000, 0x1d8bf2ca )
 	ROM_LOAD( "rt1-16.12u", 0x70000, 0x10000, 0x1bbcf37b )

	ROM_REGION( 4096+2*512 ) /* color proms */
	ROM_LOAD( "mb7124e.3r", 0x0000, 0x0200, 0x8ef3bb9d )	/* red & green components */
	ROM_LOAD( "mb7116e.3s", 0x0200, 0x0200, 0x6510a8f2 )	/* blue component */
	ROM_LOAD( "mb7138h.4v", 0x0400, 0x0800, 0x95c7d944 )	/* tiles colortable */
	ROM_LOAD( "mb7138h.6v", 0x0c00, 0x0800, 0x1391fec9 )	/* sprites colortable */

	ROM_REGION( 0x10000 ) /* MCU data */
	ROM_LOAD( "rt1-4.6b",    0x04000, 0x8000, 0x00cf293f )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, 0x6ef08fb3 )

	ROM_REGION( 0x40000 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "rt1-21.f3",  0x00000, 0x10000, 0x454968f3 )
	ROM_LOAD( "rt1-22.h3",  0x10000, 0x10000, 0xfe963e72 )
	/* k3 empty */
	/* m3 empty */

	ROM_REGION( 32 ) /* ??? */
	ROM_LOAD( "mb7112e.7u",	0x0000, 0x0020, 0xe4130804 )
ROM_END

ROM_START( rthundr2_rom )
	ROM_REGION( 0x10000 ) /* 6809 code for CPU1 */
	ROM_LOAD( "r1",         0x8000, 0x8000, 0x6f8c1252 )

	ROM_REGION( 0x40000 ) /* bank switched data for CPU1 */
	ROM_LOAD( "rt1-17.f1",  0x00000, 0x10000, 0x766af455 )
	ROM_LOAD( "rt1-18.h1",  0x10000, 0x10000, 0x3f9f2f5d )
	ROM_LOAD( "r19",        0x20000, 0x10000, 0xfe9343b0 )
	ROM_LOAD( "r20",        0x30000, 0x10000, 0xf8518d4f )

	ROM_REGION( 0x10000 ) /* 6809 code for CPU2 */
	ROM_LOAD( "r2",        0x8000, 0x8000, 0xf22a03d8 )

	ROM_REGION( 0x8000 ) /* bank switched data for CPU2 */
	ROM_LOAD( "r3",        0x0000, 0x8000, 0xaaa82885 )

	ROM_REGION( 0x24000 ) /* tiles */
	ROM_LOAD( "rt1-7.7r",  0x00000, 0x10000, 0xa85efa39 )	/* plane 1,2 */
	ROM_LOAD( "rt1-5.4r",  0x10000, 0x08000, 0xd0fc470b )
	ROM_LOAD( "rt1-8.7s",  0x18000, 0x08000, 0xf7a95820 )	/* plane 3 */
	ROM_LOAD( "rt1-6.4s",  0x20000, 0x04000, 0x6b57edb2 )

	ROM_REGION( 0x80000 ) /* sprites */
 	ROM_LOAD( "rt1-9.12h",  0x00000, 0x10000, 0x8e070561 )
 	ROM_LOAD( "rt1-10.12k", 0x10000, 0x10000, 0xcb8fb607 )
 	ROM_LOAD( "rt1-11.12l", 0x20000, 0x10000, 0x2bdf5ed9 )
 	ROM_LOAD( "rt1-12.12m", 0x30000, 0x10000, 0xe6c6c7dc )
 	ROM_LOAD( "rt1-13.12p", 0x40000, 0x10000, 0x489686d7 )
 	ROM_LOAD( "rt1-14.12r", 0x50000, 0x10000, 0x689e56a8 )
 	ROM_LOAD( "rt1-15.12t", 0x60000, 0x10000, 0x1d8bf2ca )
 	ROM_LOAD( "rt1-16.12u", 0x70000, 0x10000, 0x1bbcf37b )

	ROM_REGION( 4096+2*512 ) /* color proms */
	ROM_LOAD( "mb7124e.3r", 0x0000, 0x0200, 0x8ef3bb9d )	/* red & green components */
	ROM_LOAD( "mb7116e.3s", 0x0200, 0x0200, 0x6510a8f2 )	/* blue component */
	ROM_LOAD( "mb7138h.4v", 0x0400, 0x0800, 0x95c7d944 )	/* tiles colortable */
	ROM_LOAD( "mb7138h.6v", 0x0c00, 0x0800, 0x1391fec9 )	/* sprites colortable */

	ROM_REGION( 0x10000 ) /* MCU data */
	ROM_LOAD( "r4",          0x04000, 0x8000, 0x0387464f )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, 0x6ef08fb3 )

	ROM_REGION( 0x40000 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "rt1-21.f3",  0x00000, 0x10000, 0x454968f3 )
	ROM_LOAD( "rt1-22.h3",  0x10000, 0x10000, 0xfe963e72 )
	/* k3 empty */
	/* m3 empty */

	ROM_REGION( 32 ) /* ??? */
	ROM_LOAD( "mb7112e.7u",	0x0000, 0x0020, 0xe4130804 )
ROM_END

ROM_START( wndrmomo_rom )
	ROM_REGION( 0x10000 ) /* 6809 code for CPU1 */
	ROM_LOAD( "wm1-1.9c", 0x8000, 0x8000, 0x34b50bf0 )

	ROM_REGION( 0x40000 ) /* bank switched data for CPU1 */
 	ROM_LOAD( "wm1-16.f1", 0x00000, 0x10000, 0xe565f8f3 )
	/* h1 empty */
	/* k1 empty */
	/* m1 empty */

	ROM_REGION( 0x10000 ) /* 6809 code for CPU2 */
	ROM_LOAD( "wm1-2.12c", 0x8000, 0x8000, 0x3181efd0 )

	ROM_REGION( 0x8000 ) /* bank switched data for CPU2 */
	ROM_LOAD( "wm1-3.12d", 0x0000, 0x8000, 0x55f01df7 )

	ROM_REGION( 0x24000 ) /* tiles */
	ROM_LOAD( "wm1-8.7s", 0x00000, 0x10000, 0x14f52e72 )	/* plane 1,2 */
	ROM_LOAD( "wm1-6.4s", 0x10000, 0x08000, 0x42d0b513 )
	ROM_LOAD( "wm1-7.7r", 0x18000, 0x04000, 0x7d662527 )	/* plane 3 */
	ROM_LOAD( "wm1-5.4r", 0x20000, 0x04000, 0xa81b481f )

	ROM_REGION( 0x80000 ) /* sprites */
 	ROM_LOAD( "wm1-9.12h",  0x00000, 0x10000, 0x16f8cdae )
 	ROM_LOAD( "wm1-10.12k", 0x10000, 0x10000, 0xbfbc1896 )
 	ROM_LOAD( "wm1-11.12l", 0x20000, 0x10000, 0xd775ddb2 )
 	ROM_LOAD( "wm1-12.12m", 0x30000, 0x10000, 0xde64c12f )
 	ROM_LOAD( "wm1-13.12p", 0x40000, 0x10000, 0xcfe589ad )
 	ROM_LOAD( "wm1-14.12r", 0x50000, 0x10000, 0x2ae21a53 )
 	ROM_LOAD( "wm1-15.12t", 0x60000, 0x10000, 0xb5c98be0 )

	ROM_REGION( 0x1400 ) /* color proms */
	ROM_LOAD( "wm1-1.3r", 0x0000, 0x0200, 0x1af8ade8 )	/* red & green components */
	ROM_LOAD( "wm1-2.3s", 0x0200, 0x0200, 0x8694e213 )	/* blue component */
	ROM_LOAD( "wm1-3.4v", 0x0400, 0x0800, 0x2ffaf9a4 )	/* tiles colortable */
	ROM_LOAD( "wm1-4.5v", 0x0c00, 0x0800, 0xf4e83e0b )	/* sprites colortable */

	ROM_REGION( 0x10000 ) /* MCU data */
	ROM_LOAD( "wm1-4.6b",    0x04000, 0x8000, 0xbbe67836 )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, 0x6ef08fb3 )

	ROM_REGION( 0x40000 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "wm1-17.f3", 0x00000, 0x10000, 0xbea3c318 )
	ROM_LOAD( "wm1-18.h3", 0x10000, 0x10000, 0x6d73bcc5 )
	ROM_LOAD( "wm1-19.k3", 0x20000, 0x10000, 0xd288e912 )
	ROM_LOAD( "wm1-20.m3", 0x30000, 0x10000, 0x076a72cb )

	ROM_REGION( 32 ) /* ??? */
	ROM_LOAD( "wm1-5.6u",	0x0000, 0x0020, 0xe4130804 )
ROM_END


static void rthunder_gfx_untangle( void ) {
	/* shuffle tile ROMs so regular gfx unpack routines can be used */
	int size = 0x18000;
	unsigned char *gfx = Machine->memory_region[MEM_GFX_TILES];
	unsigned char *buffer = malloc( size );

	if ( buffer ) {
		unsigned char *dest1 = gfx;
		unsigned char *dest2 = gfx + ( size / 2 );
		unsigned char *mono = gfx + size;
		int i;

		memcpy( buffer, gfx, size );

		for ( i = 0; i < size; i += 2 ) {
			unsigned char data1 = buffer[i];
			unsigned char data2 = buffer[i+1];
			*dest1++ = ( data1 & 0xf0 ) | ( data2 >> 4 );
			*dest2++ = ( data1 << 4 ) | ( data2 & 0xf );

			*mono ^= 0xff; mono++;
		}

		free( buffer );
	}
}

/***************************************************************************

  High score save - DW 1/17/99

***************************************************************************/


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if  (memcmp(&RAM[0x5400],"\x00\x30\x00",3) == 0 &&
			memcmp(&RAM[0x5420],"\x16\x19\x12",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x5400],35);
			RAM[0x5450] = RAM[0x5400];
			RAM[0x5451] = RAM[0x5401];
			RAM[0x5452] = RAM[0x5402];

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x5400],35);
		osd_fclose(f);
	}
}


struct GameDriver rthunder_driver =
{
	__FILE__,
	0,
	"rthunder",
	"Rolling Thunder (set 1)",
	"1986",
	"Namco",
	"Jimmy Hamm\nPhil Stroffolino\nErnesto Corvi",
	0,
	&machine_driver,
	0,

	rthunder_rom,
	rthunder_gfx_untangle, 0,
	0,
	0, /* sound prom */
	rthunder_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver rthundr2_driver =
{
	__FILE__,
	&rthunder_driver,
	"rthundr2",
	"Rolling Thunder (set 2)",
	"1986",
	"Namco",
	"Jimmy Hamm\nPhil Stroffolino\nErnesto Corvi",
	0,
	&machine_driver,
	0,

	rthundr2_rom,
	rthunder_gfx_untangle, 0,
	0,
	0, /* sound prom */
	rthundr2_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver wndrmomo_driver =
{
	__FILE__,
	0,
	"wndrmomo",
	"Wonder Momo",
	"1987",
	"Namco",
	"Jimmy Hamm\nPhil Stroffolino\nErnesto Corvi",
	0,
	&machine_driver,
	0,

	wndrmomo_rom,
	rthunder_gfx_untangle, 0,
	0,
	0, /* sound prom */
	rthunder_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};
