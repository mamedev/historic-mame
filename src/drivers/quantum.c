/*
  quantum

  Paul Forgey, 1997

  This code is donated to the MAME team, and inherits all copyrights
  and restrictions from MAME
*/

#include "driver.h"
#include "inptport.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"


int quantum_avg_start(void);
int quantum_interrupt(void);
void quantum_led_write(int, int);
void quantum_vram_write(int, int);
int quantum_vram_read(int);
void quantum_nvram_write(int, int);
int quantum_nvram_read(int);
void quantum_snd_write(int, int);
int quantum_snd_read(int);
int quantum_trackball_r(int);
int quantum_nvram_load(void);
void quantum_nvram_save(void);
int quantum_switches_r(int);
int quantum_input_1_r(int);
int quantum_input_2_r(int);

extern unsigned char *quantum_ram;
extern unsigned char *quantum_nvram;


/*
	QUANTUM MEMORY MAP (per schem):

	000000-003FFF	ROM0
	004000-004FFF	ROM1
	008000-00BFFF	ROM2
	00C000-00FFFF	ROM3
	010000-013FFF	ROM4

	018000-01BFFF	RAM0
	01C000-01CFFF	RAM1

	940000			TRACKBALL
	948000			SWITCHES
	950000			COLORRAM
	958000			CONTROL (LED and coin control)
	960000-970000	RECALL (nvram read)
	968000			VGRST (vector reset)
	970000			VGGO (vector go)
	978000			WDCLR (watchdog)
	900000			NVRAM (nvram write)
	840000			I/OS (sound and dip switches)
	800000-801FFF	VMEM (vector display list)
	940000			I/O (shematic label really - covered above)
	900000			DTACK1

*/

struct MemoryReadAddress quantum_read[] =
{
	{ 0x000000, 0x013fff, MRA_ROM },
	{ 0x018000, 0x01cfff, MRA_BANK1, &quantum_ram },
	{ 0x800000, 0x801fff, MRA_BANK2, &vectorram, &vectorram_size },
	{ 0x840000, 0x8400ff, quantum_snd_read },
	{ 0x900000, 0x9001ff, quantum_nvram_read, &quantum_nvram },
	{ 0x940000, 0x947fff, quantum_trackball_r }, /* trackball */
	{ 0x948000, 0x94ffff, quantum_switches_r },
	{ 0x950000, 0x957fff, MRA_NOP }, /* color RAM isn't readable */
	{ 0x958000, 0x95ffff, MRA_NOP }, /* LS273 latch isn't even selected on read */
	{ 0x960000, 0x9601ff, quantum_nvram_read },
	{ -1 }
};

struct MemoryWriteAddress quantum_write[] =
{
	{ 0x000000, 0x013fff, MWA_ROM },
	{ 0x018000, 0x01cfff, MWA_BANK1 },
	{ 0x800000, 0x801fff, MWA_BANK2 },
	{ 0x840000, 0x8400ff, quantum_snd_write },
	{ 0x900000, 0x9001ff, quantum_nvram_write },
	{ 0x948000, 0x94ffff, MWA_NOP }, /* switches */
	{ 0x950000, 0x957fff, quantum_colorram_w },
	{ 0x958000, 0x95ffff, quantum_led_write },
	{ 0x960000, 0x9601ff, quantum_nvram_write },
	{ 0x968000, 0x96ffff, avgdvg_reset },
	{ 0x970000, 0x977fff, avgdvg_go },
	{ 0x978000, 0x9fffff, MWA_NOP },
	{ -1 }
};



INPUT_PORTS_START( quantum_input_ports )
	PORT_START	/* IN0 */

	/* YHALT here MUST BE ALWAYS 0  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_UNKNOWN )	/* vg YHALT */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(	0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_DIPSETTING(      0x00, "On" )
	PORT_DIPSETTING(      0x80, "Off" )

/* first POKEY is SW2, second is SW1 -- more confusion! */
	PORT_START /* DSW0 */
	PORT_DIPNAME(   0xc0, 0x00, "Coinage",             IP_KEY_NONE )
	PORT_DIPSETTING(      0x40, "Free Play"            )
	PORT_DIPSETTING(      0xc0, "1 Coin/2 Credit"      )
	PORT_DIPSETTING(      0x00, "1 Coin/1 Credit"      )
	PORT_DIPSETTING(      0x80, "2 Coin/1 Credit"      )

	PORT_DIPNAME(   0x30, 0x00, "Right Coin mech",     IP_KEY_NONE)
	PORT_DIPSETTING(      0x00, "Right coin mech x1"   )
	PORT_DIPSETTING(      0x20, "Right coin mech x4"   )
	PORT_DIPSETTING(      0x10, "Right coin mech x5"   )
	PORT_DIPSETTING(      0x30, "Right coin mech x6"   )

	PORT_DIPNAME(   0x08, 0x00, "Left Coin mech",      IP_KEY_NONE)
	PORT_DIPSETTING(      0x00, "Left coin mech x1"    )
	PORT_DIPSETTING(      0x08, "Left coin mech x2"    )

	PORT_DIPNAME(   0x07, 0x00, "Bonus Coins",         IP_KEY_NONE)
	PORT_DIPSETTING(      0x00, "No bonus coins"       )
	PORT_DIPSETTING(      0x02, "1 bonus coin for 4"   )
	PORT_DIPSETTING(      0x06, "2 bonus coin for 4"   )
	PORT_DIPSETTING(      0x01, "1 bonus coin for 5"   )
	PORT_DIPSETTING(      0x05, "1 bonus coin for 3"   )

	PORT_START /* DSW1 */
	PORT_DIPNAME(   0xff, 0x00, "Unknown",             IP_KEY_NONE )
	PORT_DIPSETTING(      0x00, "Factory Settings"     )

	PORT_START      /* IN2 */
	PORT_ANALOG ( 0x0f, 0, IPT_TRACKBALL_Y | IPF_REVERSE, 20, 7, 0,0)

	PORT_START      /* IN3 */
	PORT_ANALOG ( 0x0f, 0, IPT_TRACKBALL_X, 20, 7, 0, 0 )
INPUT_PORTS_END


static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0,      &fakelayout,     0, 256 },
	{ -1 } /* end of array */
};


static unsigned char color_prom[] = { VEC_PAL_COLOR };



static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz? */
	255,
	POKEY_DEFAULT_GAIN/2,
	NO_CLIP,
	/* The 8 pot handlers */
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	/* The allpot handler */
	{ 0, 0 },
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,		/* really should be 6MHz, but we use 8 because the 68k isn't properly timed */
			0,
			quantum_read,quantum_write,0,0,
			quantum_interrupt,3	/* IRQ rate = 750kHz/4096 */
		}
	},
	60, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,

	/* video hardware */
	350, 400, { 0, 600, 0, 900 },
	gfxdecodeinfo,
	256, 256,
	avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	avg_start_quantum,
	avg_stop,
	avg_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



ROM_START( quantum1_rom )
	ROM_REGION(0x014000)
    ROM_LOAD_EVEN( "136016.101", 0x000000, 0x002000, 0x6d6c10aa )
    ROM_LOAD_ODD ( "136016.106", 0x000000, 0x002000, 0xf8098bbd )
    ROM_LOAD_EVEN( "136016.102", 0x004000, 0x002000, 0xe248a2b4 )
    ROM_LOAD_ODD ( "136016.107", 0x004000, 0x002000, 0x08a8ebee )
    ROM_LOAD_EVEN( "136016.103", 0x008000, 0x002000, 0x8f3b4f1f )
    ROM_LOAD_ODD ( "136016.108", 0x008000, 0x002000, 0x16aa75d6 )
    ROM_LOAD_EVEN( "136016.104", 0x00C000, 0x002000, 0xbd97cfe7 )
    ROM_LOAD_ODD ( "136016.109", 0x00C000, 0x002000, 0x35fe3930 )
    ROM_LOAD_EVEN( "136016.105", 0x010000, 0x002000, 0xf574385a )
    ROM_LOAD_ODD ( "136016.110", 0x010000, 0x002000, 0x943b9d71 )
ROM_END

ROM_START( quantum_rom )
	ROM_REGION(0x014000)
    ROM_LOAD_EVEN( "136016.201", 0x000000, 0x002000, 0x8cfa0e3a )
    ROM_LOAD_ODD ( "136016.206", 0x000000, 0x002000, 0x75bd063d )
    ROM_LOAD_EVEN( "136016.102", 0x004000, 0x002000, 0xe248a2b4 )
    ROM_LOAD_ODD ( "136016.107", 0x004000, 0x002000, 0x08a8ebee )
    ROM_LOAD_EVEN( "136016.203", 0x008000, 0x002000, 0x87164704 )
    ROM_LOAD_ODD ( "136016.208", 0x008000, 0x002000, 0xa6e3e47d )
    ROM_LOAD_EVEN( "136016.104", 0x00C000, 0x002000, 0xbd97cfe7 )
    ROM_LOAD_ODD ( "136016.109", 0x00C000, 0x002000, 0x35fe3930 )
    ROM_LOAD_EVEN( "136016.105", 0x010000, 0x002000, 0xf574385a )
    ROM_LOAD_ODD ( "136016.110", 0x010000, 0x002000, 0x943b9d71 )
ROM_END


struct GameDriver quantum_driver =
{
	__FILE__,
	0,
	"quantum",
	"Quantum (rev 2)",
	"1982",
	"Atari",
	"Paul Forgey (MAME driver)\nAaron Giles (MAME driver)\n"VECTOR_TEAM,
	0,
	&machine_driver,

	quantum_rom,
	NULL, NULL,

	NULL,
	NULL,

	quantum_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	quantum_nvram_load, quantum_nvram_save
};

struct GameDriver quantum1_driver =
{
	__FILE__,
	&quantum_driver,
	"quantum1",
	"Quantum (rev 1)",
	"1982",
	"Atari",
	"Paul Forgey (MAME driver)\nAaron Giles (MAME driver)\n"VECTOR_TEAM,
	0,
	&machine_driver,

	quantum1_rom,
	NULL, NULL,

	NULL,
	NULL,

	quantum_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	quantum_nvram_load, quantum_nvram_save
};
