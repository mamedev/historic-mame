/***************************************************************************

	Badlands

    driver by Aaron Giles

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "vidhrdw/generic.h"


void badlands_playfieldram_w(int offset, int data);
void badlands_pf_bank_w(int offset, int data);

int badlands_vh_start(void);
void badlands_vh_stop(void);
void badlands_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void badlands_scanline_update(int scanline);


static UINT8 pedal_value[2];



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 1;
	if (atarigen_sound_int_state)
		newstate = 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	pedal_value[0] = pedal_value[1] = 0x80;

	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(badlands_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static int vblank_int(void)
{
	int pedal_state = input_port_4_r(0);
	int i;

	/* update the pedals once per frame */
    for (i = 0; i < 2; i++)
	{
		pedal_value[i]--;
		if (pedal_state & (1 << i))
			pedal_value[i]++;
	}

	return atarigen_video_int_gen();
}



/*************************************
 *
 *	I/O read dispatch
 *
 *************************************/

static int sound_busy_r(int offset)
{
	int temp = 0xfeff;

	(void)offset;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0100;
	return temp;
}


static int pedal_0_r(int offset)
{
	(void)offset;
	return pedal_value[0];
}


static int pedal_1_r(int offset)
{
	(void)offset;
	return pedal_value[1];
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress main_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0xfc0000, 0xfc1fff, sound_busy_r },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_r },
	{ 0xfe4000, 0xfe5fff, input_port_0_r },
	{ 0xfe6000, 0xfe6001, input_port_1_r },
	{ 0xfe6002, 0xfe6003, input_port_2_r },
	{ 0xfe6004, 0xfe6005, pedal_0_r },
	{ 0xfe6006, 0xfe6007, pedal_1_r },
	{ 0xfea000, 0xfebfff, atarigen_sound_upper_r },
	{ 0xffc000, 0xffc3ff, paletteram_word_r },
	{ 0xffe000, 0xffefff, MRA_BANK1 },
	{ 0xfff000, 0xffffff, MRA_BANK2 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress main_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfc0000, 0xfc1fff, atarigen_sound_reset_w },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfe0000, 0xfe1fff, watchdog_reset_w },
	{ 0xfe2000, 0xfe3fff, atarigen_video_int_ack_w },
	{ 0xfe8000, 0xfe9fff, atarigen_sound_upper_w },
	{ 0xfec000, 0xfedfff, badlands_pf_bank_w },
	{ 0xfee000, 0xfeffff, atarigen_eeprom_enable_w },
	{ 0xffc000, 0xffc3ff, atarigen_expanded_666_paletteram_w, &paletteram },
	{ 0xffe000, 0xffefff, badlands_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xfff000, 0xffffff, MWA_BANK2, &atarigen_spriteram, &atarigen_spriteram_size },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( badlands )
	PORT_START		/* fe4000 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fe6000 */
	PORT_ANALOG ( 0x00ff, 0, IPT_DIAL | IPF_PLAYER1, 100, 10, 0xff, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fe6002 */
	PORT_ANALOG ( 0x00ff, 0, IPT_DIAL | IPF_PLAYER2, 100, 10, 0xff, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT		/* audio board port */

	PORT_START      /* fake for pedals */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,	/* 8*8 chars */
	12288,	/* 12288 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxLayout molayout =
{
	16,8,	/* 16*8 chars */
	3072,	/* 3072 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*8, 8*8, 16*8, 24*8, 32*8, 40*8, 48*8, 56*8 },
	64*8	/* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout,    0, 8 },
	{ REGION_GFX2, 0, &molayout,  128, 8 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_badlands =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			7159160,		/* 7.159 Mhz */
			main_readmem,main_writemem,0,0,
			vblank_int,1
		},
		JSA_I_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_SUPPORTS_DIRTY,
	0,
	badlands_vh_start,
	badlands_vh_stop,
	badlands_vh_screenrefresh,

	/* sound hardware */
	JSA_I_STEREO,

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM decoding
 *
 *************************************/

static void rom_decode(void)
{
	int i;

	for (i = 0; i < memory_region_length(REGION_GFX1); i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
	for (i = 0; i < memory_region_length(REGION_GFX2); i++)
		memory_region(REGION_GFX2)[i] ^= 0xff;
}



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( badlands )
	ROM_REGIONX( 0x40000, REGION_CPU1 )	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "1008.20f",  0x00000, 0x10000, 0xa3da5774 )
	ROM_LOAD_ODD ( "1006.27f",  0x00000, 0x10000, 0xaa03b4f3 )
	ROM_LOAD_EVEN( "1009.17f",  0x20000, 0x10000, 0x0e2e807f )
	ROM_LOAD_ODD ( "1007.24f",  0x20000, 0x10000, 0x99a20c2c )

	ROM_REGIONX( 0x14000, REGION_CPU2 )	/* 64k for 6502 code */
	ROM_LOAD( "1018.9c", 0x10000, 0x4000, 0xa05fd146 )
	ROM_CONTINUE(        0x04000, 0xc000 )

	ROM_REGIONX( 0x60000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1012.4n",  0x000000, 0x10000, 0x5d124c6c )	/* playfield */
	ROM_LOAD( "1013.2n",  0x010000, 0x10000, 0xb1ec90d6 )
	ROM_LOAD( "1014.4s",  0x020000, 0x10000, 0x248a6845 )
	ROM_LOAD( "1015.2s",  0x030000, 0x10000, 0x792296d8 )
	ROM_LOAD( "1016.4u",  0x040000, 0x10000, 0x878f7c66 )
	ROM_LOAD( "1017.2u",  0x050000, 0x10000, 0xad0071a3 )

	ROM_REGIONX( 0x30000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1010.14r", 0x000000, 0x10000, 0xc15f629e )	/* mo */
	ROM_LOAD( "1011.10r", 0x010000, 0x10000, 0xfb0b6717 )
	ROM_LOAD( "1019.14t", 0x020000, 0x10000, 0x0e26bff6 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_badlands(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 3, 0, 0x0080);

	/* speed up the 6502 */
	atarigen_init_6502_speedup(1, 0x4155, 0x416d);

	/* display messages */
	atarigen_show_sound_message();

	rom_decode();
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1989, badlands, , badlands, badlands, badlands, ROT0, "Atari Games", "Bad Lands" )
