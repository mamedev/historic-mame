/*
	Sega System 32 hardware

	V60 + 4 zooming, source linescrolled, alpha blended tilemap planes +
	zooming sprites + 1 front tilemap w/ram-based characters +
	ram-based zooming alpha-blended sprites	+ global r/g/b brightness controls.

	Z80 + 2xYM3438 + RF5c68 for sound (similar to System 18 with a larger ROM capacity)

	The later System Multi32 (two monitors and two sets of controls from one
	PCB with one main CPU) has mostly the same hardware, with these changes:
	V70 replaces the V60 (faster but 100% software compatible)
	2 of the tilemaps are hardwired to the left monitor, 2 to the right.
	The sprite attribute layout changes, and a bit indicates if each sprite
	should draw on the left or right monitor.
	One YM3438 is removed, and the RF5c68 is replaced by the far more powerful
	Sega "MultiPCM" IC.

	Driver by David Haywood, Olivier Galibert and R. Belmont, based on the
	"Modeler" emulator by Farfetch'd, R. Belmont, and Olivier Galibert.

	why do rad* corrupt the ram based tile memory? (or can it select a bank?) *fixed bank select
	figure out remaining palette issues, indirect modes etc. *done?
	clipping registers for sprites (radr, f1en need this most) *partly done?
	zooming *done
	controls *some done, mainly analog ones need doing
	add multi-32 games (v70 based)
	tilemaps (inc. alpha blending, line zooming + scrolling etc.) *started preliminary


Stephh's notes (based on some tests) :

 0)  all games

  - Here is what the COINn buttons do depending on the settings for games supporting
    up to 4 players ('ga*', 'spidey*' and 'arabfgt') :

      * common coin slots, 2 to 4 players :
          . COIN1 : no effect
          . COIN2 : no effect
          . COIN3 : adds coin(s)/credit(s) depending on "Coin 2" settings
          . COIN4 : adds coin(s)/credit(s) depending on "Coin 1" settings

      * individual coin slots, 2 players :
          . COIN1 : no effect
          . COIN2 : no effect
          . COIN3 : adds coin(s)/credit(s) to player 1
          . COIN4 : adds coin(s)/credit(s) to player 2

      * individual coin slots, 3 players :
          . COIN1 : no effect
          . COIN2 : adds coin(s)/credit(s) to player 1
          . COIN3 : adds coin(s)/credit(s) to player 2
          . COIN4 : adds coin(s)/credit(s) to player 3

      * individual coin slots, 4 players :
          . COIN1 : adds coin(s)/credit(s) to player 1
          . COIN2 : adds coin(s)/credit(s) to player 2
          . COIN3 : adds coin(s)/credit(s) to player 3
          . COIN4 : adds coin(s)/credit(s) to player 4


 1)  'holo'

  - default settings :

      * 1 coin to start
      * Difficulty : 4/8

  - buttons :

      * BUTTON1 : punch
      * BUTTON2 : kick


 2a) 'svf'

  - default settings :

      * 1 coin to start
      * Time (vs CPU)    : 0:45
      * Time (vs player) : 1:00
      * Difficulty : 8/20
      * Initial points (???) : 2

  - buttons :

      * BUTTON1 : ???
      * BUTTON2 : ???
      * BUTTON3 : ???


 2b) 'jleague'

  - default settings :

      * 1 coin to start
      * Time (vs CPU)    : 0:45
      * Time (vs player) : 1:00
      * Difficulty : 8/20
      * Initial points (???) : 2

  - buttons :

      * BUTTON1 : ???
      * BUTTON2 : ???
      * BUTTON3 : ???


 3a) 'ga2'

  - default settings :

      * 2 coins to start
      * individual coin slots
      * Difficulty : 4/8
      * Lives : 1
      * Energy : 40
      * 4 players cabinet

  - buttons :

      * BUTTON1 : attack
      * BUTTON2 : jump
      * BUTTON3 : magic


 3a) 'ga2j'

  - default settings :

      * 1 coin to start
      * common coin slots
      * Difficulty : 4/8
      * Lives : 2
      * Energy : 24
      * 2 players cabinet  (do not change to 3 or 4 players as there are no controls !)

  - buttons :

      * BUTTON1 : attack
      * BUTTON2 : jump
      * BUTTON3 : magic


 4a) 'spidey'

  - default settings :

      * 1 coin to start
      * common coin slots
      * Difficulty : 5/8
      * Energy : 400
      * 4 players cabinet

  - buttons :

      * BUTTON1 : attack
      * BUTTON2 : jump
      * You can use a "special attack" (which costs energy) by pressing BUTTON1 and BUTTON2


 4b) 'spideyj'

  - default settings :

      * 2 coins to start
      * individual coin slots
      * Difficulty : 5/8
      * Energy : 400
      * 4 players cabinet

  - buttons :

      * BUTTON1 : attack
      * BUTTON2 : jump
      * You can use a "special attack" (which costs energy) by pressing BUTTON1 and BUTTON2


 5)  'arabfgt'

  - default settings :

      * 2 coins to start
      * individual coin slots
      * Difficulty : 7/16
      * Lives : 2
      * 4 players cabinet

  - buttons :

      * BUTTON1 : attack
      * BUTTON2 : jump
      * You can use "magic" (if you have collected a lamp) by pressing BUTTON1 and BUTTON2


 6)  'brival'

  - Here is what the COINn buttons do depending on the settings :

      * common coin slots :
          . COIN1 : adds coin(s)/credit(s) depending on "Coin 1" settings
          . COIN2 : adds coin(s)/credit(s) depending on "Coin 2" settings

      * individual coin slots :
          . COIN1 : adds coin(s)/credit(s) to player 2
          . COIN2 : adds coin(s)/credit(s) to player 1

  - default settings :

      * 1 coin to start
      * common coin slots
      * Difficulty : 8/16
      * Damage : 5/16

  - buttons :

      * BUTTON1 : punch 1
      * BUTTON2 : punch 2
      * BUTTON3 : punch 3
      * BUTTON4 : kick 1
      * BUTTON5 : kick 2
      * BUTTON6 : kick 3


 7)  'f1en'

  - default settings :

      * 1 coin to start
      * Difficulty : 8/16

  - controls :

      * steering wheel
      * PEDAL P1 : accel
      * PEDAL P2 : brake

  - buttons :

      * BUTTON2 : gear up
      * BUTTON3 : gear down


 8)  'radm'

  - default settings :

      * 2 coins to start
      * Difficulty : 8/16
      * Time : 45 seconds
      * Cabinet : Deluxe  (changed to "Upright" via predefined EEPROM)
      * Rival  car speed : 8/16
      * Police car speed : 8/16

    Do not manually change the "Cabinet" setting until the motor is emulated !

  - controls :

      * steering wheel
      * PEDAL P1 : accel
      * PEDAL P2 : brake

  - buttons :

      * BUTTON4 : light
      * BUTTON5 : wiper


 9)  'radr'

  - default settings :

      * 1 coin to start
      * Difficulty : 8/16
      * Time : 75 seconds
      * Cabinet : Upright
      * ID : 2  (in case of linked machines - unsupported yet)
      * Coinage : Free Play  (changed to "1 Coin/1 Credit" via predefined EEPROM)

    Do not manually change the "Cabinet" setting until the motor is emulated !

  - controls :

      * steering wheel
      * PEDAL P1 : accel
      * PEDAL P2 : brake

  - buttons :

      * BUTTON2 : gear shift  (acts as a toggle)


10)  'alien3'

  - Here is what the COINn buttons do depending on the settings :

      * common coin slots :
          . COIN1 : adds coin(s)/credit(s) depending on "Coin 2" settings
          . COIN2 : adds coin(s)/credit(s) depending on "Coin 1" settings

      * individual coin slots :
          . COIN1 : adds coin(s)/credit(s) to player 1
          . COIN2 : adds coin(s)/credit(s) to player 2

  - default settings :

      * 1 coin to start
      * individual coin slots
      * Difficulty : Normal
      * Gun vibtation ON  (unsupported yet)

  - controls :

      * 2 lightguns  (already calibrated via predefined EEPROM)

  - buttons :

      * BUTTON1 : trigger (= shoot)  (also acts as a START button)
      * BUTTON2 : bombs


11a) 'sonic'

  - default settings :

      * 1 coin to start
      * common coin slots  (do not change to "individual" as there is no COIN3 !)
      * Difficulty : 2/4
      * Vitality : 2/4
      * 2 players cabinet

  - controls :

      * 2/3 trackballs (one for each player)

  - buttons :

      * BUTTON1 : attack


11b) 'sonicp'

  UNTESTED !

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

#define OSC_A	(32215900)	// System 32 master crystal is 32215900 Hz

enum { EEPROM_SYS32_0=0, EEPROM_ALIEN3, EEPROM_RADM, EEPROM_RADR };

static unsigned char irq_status;
static data16_t *system32_shared_ram;
data16_t *system32_mixerregs;		// mixer registers
static int s32_blo, s32_bhi;		// bank high and low values
static int s32_f1_prot;			// port f1 is used to protect the sound program on some games
static int s32_rand;
static data16_t *sys32_protram;
static int tocab, fromcab;
static data16_t *system32_workram;
extern int system32_draw_bgs;
data16_t *sys32_tilebank_external;

/* Video Hardware */
int system32_temp_kludge;
data16_t *sys32_spriteram16;
data16_t *sys32_txtilemap_ram;
data16_t *sys32_ramtile_ram;
data16_t *scrambled_paletteram16;

int system32_palMask;
int system32_mixerShift;
extern int system32_screen_mode;
extern int system32_screen_old_mode;
extern int system32_allow_high_resolution;

WRITE16_HANDLER( sys32_videoram_w );
WRITE16_HANDLER ( sys32_ramtile_w );
WRITE16_HANDLER ( sys32_spriteram_w );
READ16_HANDLER ( sys32_videoram_r );
VIDEO_START( system32 );
VIDEO_UPDATE( system32 );

int system32_use_default_eeprom;

/* alien 3 with the gun calibrated, it doesn't prompt you if its not */
unsigned char alien3_default_eeprom[128] = {
	0x33, 0x53, 0x41, 0x32, 0x00, 0x35, 0x00, 0x00, 0x8B, 0xE8, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00,
	0x01, 0x01, 0x03, 0x00, 0x01, 0x08, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x8B, 0xE8, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x03, 0x00,
	0x01, 0x08, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* put radmobile in 'upright' mode since we don't emulate the motor */
unsigned char radm_default_eeprom[128] = {
	0x45, 0x53, 0x41, 0x47, 0x83, 0x01, 0x00, 0x01, 0x03, 0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0xFF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x45, 0x53, 0x41, 0x47, 0x83, 0x01, 0x00, 0x01, 0x03, 0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0xFF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

/* set rad rally to 1 coin 1 credit otherwise it'll boot into freeplay by default which isn't ideal ;-) */
unsigned char radr_default_eeprom[128] = {
 0x45, 0x53, 0x41, 0x47, 0x00, 0x03, 0x00, 0x01, 0x04, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x01, 0xFF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
 0x75, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
 0x45, 0x53, 0x41, 0x47, 0x00, 0x03, 0x00, 0x01, 0x04, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x01, 0xFF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
 0x75, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00
};

static void irq_raise(int level)
{
//	logerror("irq: raising %d\n", level);
	irq_status |= (1 << level);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static int irq_callback(int irqline)
{
	int i;
	for(i=7; i>=0; i--)
		if(irq_status & (1 << i)) {
//			logerror("irq: taking irq level %d\n", i);
			return i;
		}
	return 0;
}

static WRITE16_HANDLER(irq_ack_w)
{
	if(ACCESSING_MSB) {
		irq_status &= data >> 8;
//		logerror("irq: clearing %02x -> %02x\n", data >> 8, irq_status);
		if(!irq_status)
			cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

static void irq_init(void)
{
	irq_status = 0;
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}

static NVRAM_HANDLER( system32 )
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&eeprom_interface_93C46);

		if (file)
			EEPROM_load(file);
		else
		{
			if (system32_use_default_eeprom == EEPROM_ALIEN3)
				EEPROM_set_data(alien3_default_eeprom,0x80);

			if (system32_use_default_eeprom == EEPROM_RADM)
				EEPROM_set_data(radm_default_eeprom,0x80);

			if (system32_use_default_eeprom == EEPROM_RADR)
				EEPROM_set_data(radr_default_eeprom,0x80);
		}
	}
}

static READ16_HANDLER(system32_eeprom_r)
{
	return (EEPROM_read_bit() << 7) | input_port_0_r(0);
}

static WRITE16_HANDLER(system32_eeprom_w)
{
	if(ACCESSING_LSB) {
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static READ16_HANDLER(ga2_sprite_protection_r)
{
	static unsigned int prot[16] =
	{
		0x0a, 0,
		0xc5, 0,
		0x11, 0,
		0x11, 0,
		0x18, 0,
		0x18, 0,
		0x1f, 0,
		0xc6, 0,
	};

	return prot[offset];
}

static READ16_HANDLER(ga2_wakeup_protection_r)
{
	static const char *prot =
		"wake up! GOLDEN AXE The Revenge of Death-Adder! ";
//	fprintf(stderr, "Protection read %06x %04x\n", 0xa00100 + offset*2, mem_mask);
	return prot[offset];
}

// the protection board on many system32 games has full dma/bus access
// and can write things into work RAM.  we simulate that here for burning rival.
static READ16_HANDLER(brival_protection_r)
{
	if (!mem_mask)	// only trap on word-wide reads
	{
		switch (offset)
		{
			case 0:
			case 2:
			case 3:
				return 0;
				break;
		}
	}

	return system32_workram[0xba00/2 + offset];
}

static WRITE16_HANDLER(brival_protboard_w)
{
	static const int protAddress[6][2] =
	{
		{ 0x9517, 0x00/2 },
		{ 0x9597, 0x10/2 },
		{ 0x9597, 0x20/2 },
		{ 0x9597, 0x30/2 },
		{ 0x9597, 0x40/2 },
		{ 0x9617, 0x50/2 },
	};
	char ret[32];
	int curProtType;
	unsigned char *ROM = memory_region(REGION_CPU1) + 0x100000;

	switch (offset)
	{
		case 0x800/2:
			curProtType = 0;
			break;
		case 0x802/2:
			curProtType = 1;
			break;
		case 0x804/2:
			curProtType = 2;
			break;
		case 0x806/2:
			curProtType = 3;
			break;
		case 0x808/2:
			curProtType = 4;
			break;
		case 0x80a/2:
			curProtType = 5;
			break;
		default:
			if (offset >= 0xa00/2 && offset < 0xc00/2)
				return;
			logerror("brival_protboard_w: UNKNOWN WRITE: offset %x value %x\n", offset, data);
			return;
			break;
	}

	memcpy(ret, &ROM[protAddress[curProtType][0]], 16);
	ret[16] = '\0';

	memcpy(&sys32_protram[protAddress[curProtType][1]], ret, 16);
}

// protection ram is 8-bits wide and only occupies every other address
static READ16_HANDLER(arabfgt_protboard_r)
{
	int PC = activecpu_get_pc();
	int cmpVal;

	if (PC == 0xfe0325 || PC == 0xfe01e5 || PC == 0xfe035e || PC == 0xfe03cc)
	{
		cmpVal = activecpu_get_reg(1);

		// R0 always contains the value the protection is supposed to return (!)
		return cmpVal;
	}
	else
	{
		usrintf_showmessage("UNKONWN ARF PROTECTION READ PC=%x\n", PC);
	}

	return 0;
}

static WRITE16_HANDLER(arabfgt_protboard_w)
{
}

static READ16_HANDLER(arf_wakeup_protection_r)
{
	static const char *prot =
		"wake up! ARF!                                   ";
//	fprintf(stderr, "Protection read %06x %04x\n", 0xa00100 + offset*2, mem_mask);
	return prot[offset];
}

static READ16_HANDLER(sys32_read_ff)
{
	return 0xffff;
}

static READ16_HANDLER(sys32_read_random)
{
	// some totally bogus "random" algorithm
	s32_rand += 0x10001;

	return s32_rand;
}

extern int sys32_brightness[3];

void system32_set_colour (int offset)
{
	int data;
	int r,g,b;
	int r2,g2,b2;
	UINT16 r_bright, g_bright, b_bright;

	data = paletteram16[offset];


	r = (data >> 0) & 0x0f;
	g = (data >> 4) & 0x0f;
	b = (data >> 8) & 0x0f;

	r2 = (data >> 12) & 0x1;
	g2 = (data >> 13) & 0x1;
	b2 = (data >> 13) & 0x1;

	r = (r << 4) | (r2 << 3);
	g = (g << 4) | (g2 << 3);
	b = (b << 4) | (b2 << 3);

	/* there might be better ways of doing this ... but for now its functional ;-) */
	r_bright = sys32_brightness[0]; r_bright &= 0x3f;
	g_bright = sys32_brightness[1]; b_bright &= 0x3f;
	b_bright = sys32_brightness[2]; b_bright &= 0x3f;

	if ((r_bright & 0x20)) { r = (r * (r_bright&0x1f))>>5; } else { r = r+(((0xf8-r) * (r_bright&0x1f))>>5); }
	if ((g_bright & 0x20)) { g = (g * (g_bright&0x1f))>>5; } else { g = g+(((0xf8-g) * (g_bright&0x1f))>>5); }
	if ((b_bright & 0x20)) { b = (b * (b_bright&0x1f))>>5; } else { b = b+(((0xf8-b) * (b_bright&0x1f))>>5); }

	palette_set_color(offset,r,g,b);
}

static READ16_HANDLER( system32_paletteram16_r )
{
	return paletteram16[offset];
}

static WRITE16_HANDLER( system32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w )
{
	int r,g,b;
	int r2,g2,b2;

	COMBINE_DATA(&scrambled_paletteram16[offset]); // it expects to read back the same values?

	/* rearrange the data to normal format ... */
	r = (data >>1) & 0xf;
	g = (data >>6) & 0xf;
	b = (data >>11) & 0xf;

	r2 = (data >>0) & 0x1;
	g2 = (data >>5) & 0x1;
	b2 = (data >> 10) & 0x1;

	data = (data & 0x8000) | r | g<<4 | b << 8 | r2 << 12 | g2 << 13 | b2 << 14;

	COMBINE_DATA(&paletteram16[offset]);

	system32_set_colour(offset);

}




static WRITE16_HANDLER( system32_paletteram16_xBGRBBBBGGGGRRRR_word_w )
{

	COMBINE_DATA(&paletteram16[offset]);

	// some games use 8-bit writes to some palette regions
	// (especially for the text layer palettes)

	system32_set_colour(offset);
}

static READ16_HANDLER( jp_v60_read_cab )
{
	return fromcab;
}

static WRITE16_HANDLER( jp_v60_write_cab )
{
	tocab = data;
	cpu_set_irq_line(1, 0, HOLD_LINE);
}


static MEMORY_READ16_START( system32_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x23ffff, MRA16_RAM },	// work RAM

	{ 0x300000, 0x31ffff, sys32_videoram_r }, // Tile Ram

	{ 0x400000, 0x41ffff, MRA16_RAM },	// sprite RAM

	{ 0x500002, 0x500003, jp_v60_read_cab },

	{ 0x600000, 0x607fff, MRA16_RAM },	// palette RAM
	{ 0x608000, 0x60ffff, MRA16_RAM },	// palette RAM
/**/{ 0x610000, 0x6100ff, MRA16_RAM }, // mixer chip registers
	{ 0x700000, 0x701fff, MRA16_RAM },	// shared RAM
	{ 0xc00000, 0xc00001, input_port_1_word_r },
	{ 0xc00002, 0xc00003, input_port_2_word_r },
	{ 0xc00004, 0xc00007, sys32_read_ff },
	{ 0xc00008, 0xc00009, input_port_3_word_r },
	{ 0xc0000a, 0xc0000b, system32_eeprom_r },
	{ 0xc0000c, 0xc0005f, sys32_read_ff },
	{ 0xc00060, 0xc00061, input_port_4_word_r },
	{ 0xc00062, 0xc00063, input_port_5_word_r },
	{ 0xc00064, 0xc00065, input_port_6_word_r },
	{ 0xc00066, 0xc000ff, sys32_read_ff },
	{ 0xd80000, 0xd80001, sys32_read_random },
	{ 0xf00000, 0xffffff, MRA16_BANK1 }, // High rom mirror
MEMORY_END

data16_t* sys32_displayenable;

static MEMORY_WRITE16_START( system32_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x23ffff, MWA16_RAM, &system32_workram },

	{ 0x300000, 0x31ffff, sys32_videoram_w },
	/* system 32 video ram is multi-purpose, it contains amongst other things

	multiple 32x32 bg tilemaps which can be arranged to form larger ones
	data for ram based text tiles
	text tilemap
	scroll data
	video registers

	*/

	{ 0x400000, 0x41ffff, sys32_spriteram_w, &sys32_spriteram16 }, // Sprites
	{ 0x600000, 0x607fff, system32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w, &scrambled_paletteram16 },	// magic data-line-scrambled mirror of palette RAM * we need to shuffle data written then?
	{ 0x608000, 0x60ffff, system32_paletteram16_xBGRBBBBGGGGRRRR_word_w, &paletteram16 }, // Palettes
	{ 0x610000, 0x6100ff, MWA16_RAM, &system32_mixerregs }, // mixer chip registers
	{ 0x700000, 0x701fff, MWA16_RAM, &system32_shared_ram }, // Shared ram with the z80
	{ 0xa00000, 0xa00fff, MWA16_RAM, &sys32_protram },	// protection RAM
	{ 0xc00006, 0xc00007, system32_eeprom_w },
//	{ 0xc0000c, 0xc0000d, jp_v60_write_cab },
	{ 0xc0000e, 0xc0000f, MWA16_RAM, &sys32_tilebank_external }, // at least the lowest bit is tilebank
	{ 0xc0001c, 0xc0001d, MWA16_RAM, &sys32_displayenable },
	{ 0xd00006, 0xd00007, irq_ack_w },
	{ 0xf00000, 0xffffff, MWA16_ROM },
MEMORY_END

static UINT8 *sys32_SoundMemBank;

static READ_HANDLER( system32_bank_r )
{
	return sys32_SoundMemBank[offset];
}

// the Z80's work RAM is fully shared with the V60 or V70 and battery backed up.
static READ_HANDLER( sys32_shared_snd_r )
{
	data8_t *RAM = (data8_t *)system32_shared_ram;

	return RAM[offset];
}

static WRITE_HANDLER( sys32_shared_snd_w )
{
	data8_t *RAM = (data8_t *)system32_shared_ram;

	RAM[offset] = data;
}

// some games require that port f1 be a magic echo-back latch.
// thankfully, it's not required to do any math or anything on the values.
static READ_HANDLER( sys32_sound_prot_r )
{
	return s32_f1_prot;
}

static WRITE_HANDLER( sys32_sound_prot_w )
{
	s32_f1_prot = data;
}

static MEMORY_READ_START( sound_readmem_32 )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system32_bank_r },
	{ 0xd000, 0xdfff, RF5C68_r },
	{ 0xe000, 0xffff, sys32_shared_snd_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_32 )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc008, RF5C68_reg_w },
	{ 0xd000, 0xdfff, RF5C68_w },
	{ 0xe000, 0xffff, sys32_shared_snd_w },
MEMORY_END

static void s32_recomp_bank(void)
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int Bank=0;
	static char remapbhi[8] =
	{
		0, 1, 5, 3, 4, 2, 6, 4
	};

	switch (s32_blo & 0xc0)
	{
		case 0x00:	// normal case
			Bank = (((remapbhi[s32_bhi]<<6) + (s32_blo)) << 13);
			break;
		case 0x40:
		case 0xc0:
			// SegaSonic (prototype) needs this alternate interpretation.
//			Bank = (((remapbhi[s32_bhi]<<5) + (s32_blo&0x3f)) << 13);
			// all other s32 games work with this, notably Super Visual Football
			// and Alien3: The Gun
			Bank = (((remapbhi[s32_bhi]<<6) + (s32_blo&0x3f)) << 13);
			break;
	}

	sys32_SoundMemBank = &RAM[Bank+0x100000];
}

static WRITE_HANDLER( sys32_soundbank_lo_w )
{
	s32_blo = data;
	s32_recomp_bank();
}

static WRITE_HANDLER( sys32_soundbank_hi_w )
{
	s32_bhi = data;
	s32_recomp_bank();
}

static PORT_READ_START( sound_readport_32 )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
	{ 0x90, 0x90, YM2612_status_port_1_A_r },
	{ 0xf1, 0xf1, sys32_sound_prot_r },
PORT_END

static PORT_WRITE_START( sound_writeport_32 )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0x90, 0x90, YM2612_control_port_1_A_w },
	{ 0x91, 0x91, YM2612_data_port_1_A_w },
	{ 0x92, 0x92, YM2612_control_port_1_B_w },
	{ 0x93, 0x93, YM2612_data_port_1_B_w },
	{ 0xa0, 0xa0, sys32_soundbank_lo_w },
	{ 0xb0, 0xb0, sys32_soundbank_hi_w },
	{ 0xc1, 0xc1, IOWP_NOP },
	{ 0xf1, 0xf1, sys32_sound_prot_w },
PORT_END

static MACHINE_INIT( system32 )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();

	/* force it to select lo-resolution on reset */
	system32_allow_high_resolution = 0;
	system32_screen_mode = 0;
	system32_screen_old_mode = 1;
}

static MACHINE_INIT( s32hi )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();

	/* force it to select lo-resolution on reset */
	system32_allow_high_resolution = 1;
	system32_screen_mode = 0;
	system32_screen_old_mode = 1;
}


static INTERRUPT_GEN( system32_interrupt )
{
	if(cpu_getiloops())
		irq_raise(1);
	else
		irq_raise(0);
}

/* jurassic park moving cab - not working yet */

static READ_HANDLER( jpcab_z80_read )
{
	return tocab;
}

static MEMORY_READ_START( jpcab_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xc000, 0xc000, jpcab_z80_read },
MEMORY_END

static MEMORY_WRITE_START( jpcab_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
MEMORY_END

static PORT_READ_START( jpcab_readport )
	{ 0x4, 0x4, IORP_NOP },		// interrupt control
PORT_END

static PORT_WRITE_START( jpcab_writeport )
	{ 0x4, 0x4, IOWP_NOP },
PORT_END

/* Analog Input Handlers */
/* analog controls for sonic */
/*

sonic analog trackball inputs
these are relative signed inputs that must be
between -96 and 96 each frame or sonic's code will reject them.

*/

static UINT8 last[6];

static WRITE16_HANDLER( sonic_track_reset_w )
{
	switch (offset)
	{
		case 0x00 >> 1:
			last[0] = readinputport(7);
			last[1] = readinputport(8);
			break;
		case 0x08 >> 1:
			last[2] = readinputport(9);
			last[3] = readinputport(10);
			break;
		case 0x10 >> 1:
			last[4] = readinputport(11);
			last[5] = readinputport(12);
			break;
		default:
			logerror("track_w : warning - read unmapped address %06x - PC = %06x\n",0xc00040 + (offset << 1),activecpu_get_pc());
			break;
	}
}

static READ16_HANDLER( sonic_track_r )
{
	int delta = 0;

	switch (offset)
	{
		case 0x00 >> 1:
			delta = (int)readinputport(7)  - (int)last[0];
			break;
		case 0x04 >> 1:
			delta = (int)readinputport(8)  - (int)last[1];
			break;
		case 0x08 >> 1:
			delta = (int)readinputport(9)  - (int)last[2];
			break;
		case 0x0c >> 1:
			delta = (int)readinputport(10) - (int)last[3];
			break;
		case 0x10 >> 1:
			delta = (int)readinputport(11) - (int)last[4];
			break;
		case 0x14 >> 1:
			delta = (int)readinputport(12) - (int)last[5];
			break;
		default:
			logerror("track_r : warning - read unmapped address %06x - PC = %06x\n",0xc00040 + (offset << 1),activecpu_get_pc());
			break;
	}

	/* handle the case where we wrap around from 0x00 to 0xff, or vice versa */
	if (delta >=  0x80)
		delta -= 0x100;
	if (delta <= -0x80)
		delta += 0x100;

	return delta;
}

/* analog controls for the driving games

they write to an address then read 8 times, expecting a single bit each time

WORKING (seems to be anyway)

*/

static data16_t sys32_driving_steer_c00050_data;
static data16_t sys32_driving_accel_c00052_data;
static data16_t sys32_driving_brake_c00054_data;

static WRITE16_HANDLER ( sys32_driving_steer_c00050_w ) { sys32_driving_steer_c00050_data = readinputport(7); }
static WRITE16_HANDLER ( sys32_driving_accel_c00052_w ) { sys32_driving_accel_c00052_data = readinputport(8); }
static WRITE16_HANDLER ( sys32_driving_brake_c00054_w ) { sys32_driving_brake_c00054_data = readinputport(9); }

static READ16_HANDLER ( sys32_driving_steer_c00050_r ) { int retdata; retdata = sys32_driving_steer_c00050_data & 0x80; sys32_driving_steer_c00050_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_driving_accel_c00052_r ) { int retdata; retdata = sys32_driving_accel_c00052_data & 0x80; sys32_driving_accel_c00052_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_driving_brake_c00054_r ) { int retdata; retdata = sys32_driving_brake_c00054_data & 0x80; sys32_driving_brake_c00054_data <<= 1; return retdata; }


/* lightgun for alien 3

WORKING (seems to be anyway)

*/

static data16_t sys32_gun_p1_x_c00050_data;
static data16_t sys32_gun_p1_y_c00052_data;
static data16_t sys32_gun_p2_x_c00054_data;
static data16_t sys32_gun_p2_y_c00056_data;

static WRITE16_HANDLER ( sys32_gun_p1_x_c00050_w ) { sys32_gun_p1_x_c00050_data = readinputport(7); }
static WRITE16_HANDLER ( sys32_gun_p1_y_c00052_w ) { sys32_gun_p1_y_c00052_data = readinputport(8); sys32_gun_p1_y_c00052_data = (sys32_gun_p1_y_c00052_data+1)&0xff; }
static WRITE16_HANDLER ( sys32_gun_p2_x_c00054_w ) { sys32_gun_p2_x_c00054_data = readinputport(9); }
static WRITE16_HANDLER ( sys32_gun_p2_y_c00056_w ) { sys32_gun_p2_y_c00056_data = readinputport(10); sys32_gun_p2_y_c00056_data = (sys32_gun_p2_y_c00056_data+1)&0xff; }

static READ16_HANDLER ( sys32_gun_p1_x_c00050_r ) { int retdata; retdata = sys32_gun_p1_x_c00050_data & 0x80; sys32_gun_p1_x_c00050_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_gun_p1_y_c00052_r ) { int retdata; retdata = sys32_gun_p1_y_c00052_data & 0x80; sys32_gun_p1_y_c00052_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_gun_p2_x_c00054_r ) { int retdata; retdata = sys32_gun_p2_x_c00054_data & 0x80; sys32_gun_p2_x_c00054_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_gun_p2_y_c00056_r ) { int retdata; retdata = sys32_gun_p2_y_c00056_data & 0x80; sys32_gun_p2_y_c00056_data <<= 1; return retdata; }

/* end analog input bits */



#define SYSTEM32_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ )


/* Generic entry for 2 players games - to be used for games which haven't been tested yet */
INPUT_PORTS_START( system32 )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* Generic entry for 4 players games - to be used for games which haven't been tested yet */
INPUT_PORTS_START( sys32_4p )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	SYSTEM32_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00062 - port 5
	SYSTEM32_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( holo )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( svf )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ga2 )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	SYSTEM32_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00062 - port 5
	SYSTEM32_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ga2j )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( spidey )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	SYSTEM32_PLAYER_INPUTS(3, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00062 - port 5
	SYSTEM32_PLAYER_INPUTS(4, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( spideyj )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	SYSTEM32_PLAYER_INPUTS(3, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00062 - port 5
	SYSTEM32_PLAYER_INPUTS(4, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( brival )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00002 - port 2
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( f1en )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON2, "Gear Up",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3, "Gear Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00050 - port 7 - steering wheel
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00052 - port 8 - accel pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00054 - port 9 - brake pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER2, 30, 10, 0x00, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( radm )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON4, "Light", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON5, "Wiper", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00050 - port 7 - steering wheel
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00052 - port 8 - accel pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00054 - port 9 - brake pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER2, 30, 10, 0x00, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( radr )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Gear Change", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00050 - port 7 - steering wheel
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00052 - port 8 - accel pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	// 0xc00054 - port 9 - brake pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER2, 30, 10, 0x00, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( alien3 )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// gives a credit to player 2 ?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00050 - port 7  - player 1 lightgun X axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START	// 0xc00052 - port 8  - player 1 lightgun Y axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 35, 15, 0, 0xff )

	PORT_START	// 0xc00054 - port 9  - player 2 lightgun X axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 35, 15, 0, 0xff )

	PORT_START	// 0xc00056 - port 10 - player 2 lightgun Y axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 35, 15, 0, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( sonic )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START // 0xc00040 - port 7  - player 1 trackball X axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_REVERSE, 100, 15, 0, 0 )

	PORT_START // 0xc00044 - port 8  - player 1 trackball Y axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 15, 0, 0 )

	PORT_START // 0xc00048 - port 9  - player 2 trackball X axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_REVERSE, 100, 15, 0, 0 )

	PORT_START // 0xc0004c - port 10 - player 2 trackball Y axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 15, 0, 0 )

	PORT_START // 0xc00050 - port 11 - player 3 trackball X axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER3  | IPF_REVERSE, 100, 15, 0, 0 )

	PORT_START // 0xc00054 - port 12 - player 3 trackball Y axis
	PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_PLAYER3, 100, 15, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( jpark )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00002 - port 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00008 - port 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00060 - port 4
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00062 - port 5
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00064 - port 6
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 0xc00050 - port 7  - player 1 lightgun X axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 35, 15, 0x40, 0xc0 )

	PORT_START	// 0xc00052 - port 8  - player 1 lightgun Y axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 35, 15, 0x39, 0xbf )

	PORT_START	// 0xc00054 - port 9  - player 2 lightgun X axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 35, 15, 0x40, 0xc0 )

	PORT_START	// 0xc00056 - port 10 - player 2 lightgun Y axis
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 35, 15, 0x39, 0xbf )
INPUT_PORTS_END

static void irq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

struct RF5C68interface sys32_rf5c68_interface =
{
  9000000,	/* pitch matches real PCB, but this is a weird frequency */
  55
};

struct YM2612interface sys32_ym3438_interface =
{
	2,		/* 2 chips */
	8000000,	/* verified on real PCB */
	{ 40,40 },
	{ 0 },	{ 0 },	{ 0 },	{ 0 },
	{ irq_handler }
};

static struct GfxLayout s32_bgcharlayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 16, 20, 8, 12, 24, 28,
	   32, 36, 48, 52, 40, 44, 56, 60  },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};



static struct GfxLayout s32_fgcharlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	16*16
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &s32_bgcharlayout,   0x00, 0x3ff  },
	{ REGION_GFX3, 0, &s32_fgcharlayout,   0x00, 0x3ff  },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( system32 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V60, OSC_A/2/12) // Reality is 16.somethingMHz, use magic /12 factor to get approximate speed
	MDRV_CPU_MEMORY(system32_readmem,system32_writemem)
	MDRV_CPU_VBLANK_INT(system32_interrupt,2)

	MDRV_CPU_ADD_TAG("sound", Z80, OSC_A/4)	// verified on real PCB
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem_32, sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_32, sound_writeport_32)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100 /*DEFAULT_60HZ_VBLANK_DURATION*/)

	MDRV_MACHINE_INIT(system32)
	MDRV_NVRAM_HANDLER(system32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_AFTER_VBLANK /* | VIDEO_RGB_DIRECT*/) // RGB_DIRECT will be needed for alpha
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16384)

	MDRV_VIDEO_START(system32)
	MDRV_VIDEO_UPDATE(system32)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3438, sys32_ym3438_interface)
	MDRV_SOUND_ADD(RF5C68, sys32_rf5c68_interface)
MACHINE_DRIVER_END

// system 32 hi-res mode is 416x224.  Yes that's TRUSTED.
static MACHINE_DRIVER_START( sys32_hi )
	MDRV_IMPORT_FROM( system32 )

	MDRV_MACHINE_INIT(s32hi)

	MDRV_SCREEN_SIZE(52*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 52*8-1, 0*8, 28*8-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jpark )
	MDRV_IMPORT_FROM( system32 )

	MDRV_CPU_ADD_TAG("cabinet", Z80, OSC_A/8)	// ???
	MDRV_CPU_MEMORY( jpcab_readmem, jpcab_writemem )
	MDRV_CPU_PORTS( jpcab_readport, jpcab_writeport )
//	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)		// CPU has an IRQ handler, it appears to be periodic

MACHINE_DRIVER_END


ROM_START( ga2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD( "epr14960.b", 0x000000, 0x20000, 0x87182fea )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD16_WORD( "epr14957.b", 0x080000, 0x20000, 0xab787cf4 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr15146.b", 0x100000, 0x40000, 0x7293d5c3 )
	ROM_LOAD16_BYTE( "epr15145.b", 0x100001, 0x40000, 0x0da61782 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU + banks */
	ROM_LOAD("epr14945", 0x000000, 0x010000, 0x4781d4cb)
	ROM_RELOAD(          0x100000, 0x010000)
	ROM_LOAD("mpr14944", 0x180000, 0x100000, 0xfd4d4b86)
	ROM_LOAD("mpr14942", 0x280000, 0x100000, 0xa89b0e90)
	ROM_LOAD("mpr14943", 0x380000, 0x100000, 0x24d40333)

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Protection CPU */
	ROM_LOAD( "epr14468", 0x00000, 0x10000, 0x77634daa )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr14948", 0x000000, 0x200000, 0x75050d4a )
	ROM_LOAD16_BYTE( "mpr14947", 0x000001, 0x200000, 0xb53e62f4 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr14949", 0x000000, 0x200000, 0x152c716c, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14951", 0x000002, 0x200000, 0xfdb1a534, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14953", 0x000004, 0x200000, 0x33bd1c15, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14955", 0x000006, 0x200000, 0xe42684aa, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14950", 0x800000, 0x200000, 0x15fd0026, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14952", 0x800002, 0x200000, 0x96f96613, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14954", 0x800004, 0x200000, 0x39b2ac9e, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14956", 0x800006, 0x200000, 0x298fca50, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( ga2j )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "epr14961.b", 0x000000, 0x20000, 0xd9cd8885 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD16_WORD( "epr14958.b", 0x080000, 0x20000, 0x0be324a3 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr15148.b", 0x100000, 0x40000, 0xc477a9fd )
	ROM_LOAD16_BYTE( "epr15147.b", 0x100001, 0x40000, 0x1bb676ea )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU + banks */
	ROM_LOAD("epr14945", 0x000000, 0x010000, 0x4781d4cb)
	ROM_RELOAD(          0x100000, 0x010000)
	ROM_LOAD("mpr14944", 0x180000, 0x100000, 0xfd4d4b86)
	ROM_LOAD("mpr14942", 0x280000, 0x100000, 0xa89b0e90)
	ROM_LOAD("mpr14943", 0x380000, 0x100000, 0x24d40333)

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Protection CPU */
	ROM_LOAD( "epr14468", 0x00000, 0x10000, 0x77634daa )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr14948", 0x000000, 0x200000, 0x75050d4a )
	ROM_LOAD16_BYTE( "mpr14947", 0x000001, 0x200000, 0xb53e62f4 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr14949", 0x000000, 0x200000, 0x152c716c, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14951", 0x000002, 0x200000, 0xfdb1a534, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14953", 0x000004, 0x200000, 0x33bd1c15, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14955", 0x000006, 0x200000, 0xe42684aa, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14950", 0x800000, 0x200000, 0x15fd0026, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14952", 0x800002, 0x200000, 0x96f96613, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14954", 0x800004, 0x200000, 0x39b2ac9e, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr14956", 0x800006, 0x200000, 0x298fca50, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( radm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "epr13690.bin", 0x000000, 0x20000, 0x21637dec )
	ROM_RELOAD     (                 0x020000, 0x20000 )
	ROM_RELOAD     (                 0x040000, 0x20000 )
	ROM_RELOAD     (                 0x060000, 0x20000 )
	ROM_RELOAD     (                 0x080000, 0x20000 )
	ROM_RELOAD     (                 0x0a0000, 0x20000 )
	ROM_RELOAD     (                 0x0c0000, 0x20000 )
	ROM_RELOAD     (                 0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr13525.bin", 0x100000, 0x80000, 0x62ad83a0 )
	ROM_LOAD16_BYTE( "epr13526.bin", 0x100001, 0x80000, 0x59ea372a )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13527.bin", 0x00000, 0x20000, 0xa2e3fbbe )
	ROM_RELOAD(               0x100000, 0x020000             )
	ROM_LOAD( "epr13523.bin", 0x180000, 0x080000, 0xd5563697 )
	ROM_LOAD( "epr13523.bin", 0x280000, 0x080000, 0xd5563697 )
	ROM_LOAD( "epr13699.bin", 0x380000, 0x080000, 0x33fd2913 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "mpr13519.bin", 0x000000, 0x080000, 0xbedc9534 )
	ROM_LOAD32_BYTE( "mpr13520.bin", 0x000002, 0x080000, 0x3532e91a )
	ROM_LOAD32_BYTE( "mpr13521.bin", 0x000001, 0x080000, 0xe9bca903 )
	ROM_LOAD32_BYTE( "mpr13522.bin", 0x000003, 0x080000, 0x25e04648 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr13511.bin", 0x800000, 0x100000, 0xf8f15b11, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13512.bin", 0x800001, 0x100000, 0xd0be34a6, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13513.bin", 0x800002, 0x100000, 0xfeef1982, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13514.bin", 0x800003, 0x100000, 0xd0f9ebd1, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13515.bin", 0x800004, 0x100000, 0x77bf2387, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13516.bin", 0x800005, 0x100000, 0x8c4bc62d, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13517.bin", 0x800006, 0x100000, 0x1d7d84a7, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13518.bin", 0x800007, 0x100000, 0x9ea4b15d, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* unused */
	ROM_LOAD( "epr13686.bin", 0x00000, 0x8000, 0x317a2857 ) /* cabinet movement */
ROM_END

ROM_START( radr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "epr14241.06", 0x000000, 0x20000, 0x59a5f63d )
	ROM_RELOAD     (                0x020000, 0x20000 )
	ROM_RELOAD     (                0x040000, 0x20000 )
	ROM_RELOAD     (                0x060000, 0x20000 )
	ROM_RELOAD     (                0x080000, 0x20000 )
	ROM_RELOAD     (                0x0a0000, 0x20000 )
	ROM_RELOAD     (                0x0c0000, 0x20000 )
	ROM_RELOAD     (                0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr14106.14", 0x100000, 0x80000, 0xe73c63bf )
	ROM_LOAD16_BYTE( "epr14107.07", 0x100001, 0x80000, 0x832f797a )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr14108.35", 0x00000, 0x20000, 0x38a99b4d )
	ROM_RELOAD(              0x100000, 0x20000             )
	ROM_LOAD( "epr14109.31", 0x180000, 0x080000, 0xb42e5833 )
	ROM_LOAD( "epr14110.26", 0x280000, 0x080000, 0xb495e7dc )
	ROM_LOAD( "epr14237.22", 0x380000, 0x080000, 0x0a4b4b29 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "epr14102.38", 0x000000, 0x040000, 0x5626e80f )
	ROM_LOAD32_BYTE( "epr14103.34", 0x000002, 0x040000, 0x08c7e804 )
	ROM_LOAD32_BYTE( "epr14104.29", 0x000001, 0x040000, 0xb0173646 )
	ROM_LOAD32_BYTE( "epr14105.25", 0x000003, 0x040000, 0x614843b6 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr13511.36", 0x800000, 0x100000, 0xf8f15b11, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13512.32", 0x800001, 0x100000, 0xd0be34a6, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13513.27", 0x800002, 0x100000, 0xfeef1982, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13514.23", 0x800003, 0x100000, 0xd0f9ebd1, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13515.37", 0x800004, 0x100000, 0x77bf2387, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13516.33", 0x800005, 0x100000, 0x8c4bc62d, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13517.28", 0x800006, 0x100000, 0x1d7d84a7, ROM_SKIP(7) )
	ROMX_LOAD( "mpr13518.24", 0x800007, 0x100000, 0x9ea4b15d, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* unused */
	ROM_LOAD( "epr14084.17", 0x00000, 0x8000, 0xf14ed074 ) /* cabinet link */
ROM_END

ROM_START( svf )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "ep16872a.17", 0x000000, 0x020000, 0x1f383b00 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD( "ep16871a.8", 0x080000, 0x020000, 0xf7061bd7 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr16865.18", 0x100000, 0x080000, 0x9198ca9f )
	ROM_LOAD16_BYTE( "epr16864.9", 0x100001, 0x080000, 0x201a940e )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr16866.36", 0x00000, 0x20000, 0x74431350 )
	ROM_RELOAD(           0x100000, 0x20000             )
	ROM_LOAD( "mpr16779.35", 0x180000, 0x100000, 0x7055e859 )
	ROM_LOAD( "mpr16777.24", 0x280000, 0x100000, 0x14b5d5df )
	ROM_LOAD( "mpr16778.34", 0x380000, 0x100000, 0xfeedaecf )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr16784.14", 0x000000, 0x100000, 0x4608efe2 )
	ROM_LOAD16_BYTE( "mpr16783.5", 0x000001, 0x100000, 0x042eabe7 )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr16785.32", 0x000000, 0x200000, 0x51f775ce, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16787.30", 0x000002, 0x200000, 0xdee7a204, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16789.28", 0x000004, 0x200000, 0x6b6c8ad3, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16791.26", 0x000006, 0x200000, 0x4f7236da, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16860.31", 0x800000, 0x200000, 0x578a7325, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16861.29", 0x800002, 0x200000, 0xd79c3f73, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16862.27", 0x800004, 0x200000, 0x00793354, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16863.25", 0x800006, 0x200000, 0x42338226, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( svs )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "ep16883a.17", 0x000000, 0x020000, 0xe1c0c3ce )
	ROM_RELOAD	   (			   0x020000, 0x20000 )
	ROM_RELOAD	   (			   0x040000, 0x20000 )
	ROM_RELOAD	   (			   0x060000, 0x20000 )
	ROM_LOAD( "ep16882a.8", 0x080000, 0x020000, 0x1161bbbe )
	ROM_RELOAD	   (			   0x0a0000, 0x20000 )
	ROM_RELOAD	   (			   0x0c0000, 0x20000 )
	ROM_RELOAD	   (			   0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr16865.18", 0x100000, 0x080000, 0x9198ca9f )
	ROM_LOAD16_BYTE( "epr16864.9", 0x100001, 0x080000, 0x201a940e )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr16868.36", 0x00000, 0x40000, 0x47aa4ec7 ) /* same as jleague but with a different part number */
	ROM_RELOAD(           0x100000, 0x20000             )
	ROM_LOAD( "mpr16779.35", 0x180000, 0x100000, 0x7055e859 )
	ROM_LOAD( "mpr16777.24", 0x280000, 0x100000, 0x14b5d5df )
	ROM_LOAD( "mpr16778.34", 0x380000, 0x100000, 0xfeedaecf )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr16784.14", 0x000000, 0x100000, 0x4608efe2 )
	ROM_LOAD16_BYTE( "mpr16783.5", 0x000001, 0x100000, 0x042eabe7 )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr16785.32", 0x000000, 0x200000, 0x51f775ce, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16787.30", 0x000002, 0x200000, 0xdee7a204, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16789.28", 0x000004, 0x200000, 0x6b6c8ad3, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16791.26", 0x000006, 0x200000, 0x4f7236da, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16860.31", 0x800000, 0x200000, 0x578a7325, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16861.29", 0x800002, 0x200000, 0xd79c3f73, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16862.27", 0x800004, 0x200000, 0x00793354, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16863.25", 0x800006, 0x200000, 0x42338226, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( jleague )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "epr16782.17",0x000000, 0x020000, 0xf0278944 )
	ROM_RELOAD     (        0x020000, 0x020000 )
	ROM_RELOAD     (        0x040000, 0x020000 )
	ROM_RELOAD     (        0x060000, 0x020000 )
	ROM_LOAD( "epr16781.8", 0x080000, 0x020000, 0x7df9529b )
	ROM_RELOAD     (        0x0a0000, 0x020000 )
	ROM_RELOAD     (        0x0c0000, 0x020000 )
	ROM_RELOAD     (        0x0e0000, 0x020000 )
	ROM_LOAD16_BYTE( "epr16776.18", 0x100000, 0x080000, 0xe8694626 )
	ROM_LOAD16_BYTE( "epr16775.9",  0x100001, 0x080000, 0xe81e2c3d )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr16780.36", 0x00000, 0x40000, 0x47aa4ec7 ) /* can probably be cut */
	ROM_RELOAD(           0x100000, 0x40000             )
	ROM_LOAD( "mpr16779.35", 0x180000, 0x100000, 0x7055e859 )
	ROM_LOAD( "mpr16777.24", 0x280000, 0x100000, 0x14b5d5df )
	ROM_LOAD( "mpr16778.34", 0x380000, 0x100000, 0xfeedaecf )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr16784.14", 0x000000, 0x100000, 0x4608efe2 )
	ROM_LOAD16_BYTE( "mpr16783.5", 0x000001, 0x100000, 0x042eabe7 )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr16785.32", 0x000000, 0x200000, 0x51f775ce, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16787.30", 0x000002, 0x200000, 0xdee7a204, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16789.28", 0x000004, 0x200000, 0x6b6c8ad3, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr16791.26", 0x000006, 0x200000, 0x4f7236da, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16786.31",  0x800000, 0x200000, 0xd08a2d32, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16788.29",  0x800002, 0x200000, 0xcd9d3605, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16790.27",  0x800004, 0x200000, 0x2ea75746, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16792.25",  0x800006, 0x200000, 0x9f416072, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( spidey )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "14303", 0x000000, 0x020000, 0x7f1bd28f )
	ROM_RELOAD (       0x020000, 0x020000 )
	ROM_RELOAD (       0x040000, 0x020000 )
	ROM_RELOAD (       0x060000, 0x020000 )
	ROM_LOAD( "14302", 0x080000, 0x020000, 0xd954c40a )
	ROM_RELOAD (       0x0a0000, 0x020000 )
	ROM_RELOAD (       0x0c0000, 0x020000 )
	ROM_RELOAD (       0x0e0000, 0x020000 )
	ROM_LOAD16_BYTE( "14281", 0x100000, 0x020000, 0x8a746c42 )
	ROM_LOAD16_BYTE( "14280", 0x100001, 0x020000, 0x3c8148f7 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "14285", 0x00000, 0x40000, 0x25aefad6 )
	ROM_RELOAD(        0x100000, 0x40000             )
	ROM_LOAD( "14284", 0x180000, 0x080000, 0x760542d4 )
	ROM_LOAD( "14282", 0x280000, 0x080000, 0xea20979e )
	ROM_LOAD( "14283", 0x380000, 0x080000, 0xc863a91c )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "14291", 0x000000, 0x100000, 0x490F95A1 )
	ROM_LOAD32_BYTE( "14290", 0x000002, 0x100000, 0xA144162D )
	ROM_LOAD32_BYTE( "14289", 0x000001, 0x100000, 0x38570582 )
	ROM_LOAD32_BYTE( "14288", 0x000003, 0x100000, 0x3188B636 )

	ROM_REGION( 0x0800000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "14299", 0x000000, 0x100000, 0xCE59231B, ROM_SKIP(7) )
	ROMX_LOAD( "14298", 0x000001, 0x100000, 0x2745C84C, ROM_SKIP(7) )
	ROMX_LOAD( "14297", 0x000002, 0x100000, 0x29CB9450, ROM_SKIP(7) )
	ROMX_LOAD( "14296", 0x000003, 0x100000, 0x9D8CBD31, ROM_SKIP(7) )
	ROMX_LOAD( "14295", 0x000004, 0x100000, 0x29591F50, ROM_SKIP(7) )
	ROMX_LOAD( "14294", 0x000005, 0x100000, 0xFA86B794, ROM_SKIP(7) )
	ROMX_LOAD( "14293", 0x000006, 0x100000, 0x52899269, ROM_SKIP(7) )
	ROMX_LOAD( "14292", 0x000007, 0x100000, 0x41F71443, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( spideyj )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "14307", 0x000000, 0x020000, 0xd900219c )
	ROM_RELOAD (       0x020000, 0x020000 )
	ROM_RELOAD (       0x040000, 0x020000 )
	ROM_RELOAD (       0x060000, 0x020000 )
	ROM_LOAD( "14306", 0x080000, 0x020000, 0x64379dc6 )
	ROM_RELOAD (       0x0a0000, 0x020000 )
	ROM_RELOAD (       0x0c0000, 0x020000 )
	ROM_RELOAD (       0x0e0000, 0x020000 )
	ROM_LOAD16_BYTE( "14281", 0x100000, 0x020000, 0x8a746c42 )
	ROM_LOAD16_BYTE( "14280", 0x100001, 0x020000, 0x3c8148f7 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "14285", 0x00000, 0x40000, 0x25aefad6 )
	ROM_RELOAD(        0x100000, 0x40000             )
	ROM_LOAD( "14284", 0x180000, 0x080000, 0x760542d4 )
	ROM_LOAD( "14282", 0x280000, 0x080000, 0xea20979e )
	ROM_LOAD( "14283", 0x380000, 0x080000, 0xc863a91c )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "14291", 0x000000, 0x100000, 0x490F95A1 )
	ROM_LOAD32_BYTE( "14290", 0x000002, 0x100000, 0xA144162D )
	ROM_LOAD32_BYTE( "14289", 0x000001, 0x100000, 0x38570582 )
	ROM_LOAD32_BYTE( "14288", 0x000003, 0x100000, 0x3188B636 )

	ROM_REGION( 0x0800000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "14299", 0x000000, 0x100000, 0xCE59231B, ROM_SKIP(7) )
	ROMX_LOAD( "14298", 0x000001, 0x100000, 0x2745C84C, ROM_SKIP(7) )
	ROMX_LOAD( "14297", 0x000002, 0x100000, 0x29CB9450, ROM_SKIP(7) )
	ROMX_LOAD( "14296", 0x000003, 0x100000, 0x9D8CBD31, ROM_SKIP(7) )
	ROMX_LOAD( "14295", 0x000004, 0x100000, 0x29591F50, ROM_SKIP(7) )
	ROMX_LOAD( "14294", 0x000005, 0x100000, 0xFA86B794, ROM_SKIP(7) )
	ROMX_LOAD( "14293", 0x000006, 0x100000, 0x52899269, ROM_SKIP(7) )
	ROMX_LOAD( "14292", 0x000007, 0x100000, 0x41F71443, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( sonic )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "epr-c-87.17", 0x000000, 0x020000, 0x25e3c27e )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD( "epr-c-86.8", 0x080000, 0x020000, 0xefe9524c )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr-c-81.18", 0x100000, 0x040000, 0x65b06c25 )
	ROM_LOAD16_BYTE( "epr-c-80.9",  0x100001, 0x040000, 0x2db66fd2 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr15785.36", 0x00000, 0x40000, 0x0fe7422e )
	ROM_RELOAD(              0x100000, 0x40000             )
	ROM_LOAD( "mpr15784.35", 0x180000, 0x100000, 0x42f06714 )
	ROM_LOAD( "mpr15783.34", 0x280000, 0x100000, 0xe4220eea )
	ROM_LOAD( "mpr15782.33", 0x380000, 0x100000, 0xcf56b5a0 ) // (this is the only rom unchanged from the prototype)

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr15789.14", 0x000000, 0x100000, 0x4378f12b )
	ROM_LOAD16_BYTE( "mpr15788.5",  0x000001, 0x100000, 0xa6ed5d7a )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr15790.32", 0x000000, 0x200000, 0xc69d51b1, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15792.30", 0x000002, 0x200000, 0x1006bb67, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15794.28", 0x000004, 0x200000, 0x8672b480, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15796.26", 0x000006, 0x200000, 0x95b8011c, ROM_SKIP(6)|ROM_GROUPWORD )

	// NOTE: these last 4 are in fact 16 megabit ROMs,
	// but they were dumped as 8 because the top half
	// is "FF" in all of them.
	ROMX_LOAD( "mpr15791.31", 0x800000, 0x100000, 0x42217066, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15793.29", 0x800002, 0x100000, 0x75bafe55, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15795.27", 0x800004, 0x100000, 0x7f3dad30, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15797.25", 0x800006, 0x100000, 0x013c6cab, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( sonicp )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "sonpg0.bin", 0x000000, 0x020000, 0xDA05DCBB )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD( "sonpg1.bin", 0x080000, 0x020000, 0xC57DC5C5 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "sonpd0.bin", 0x100000, 0x040000, 0xA7DA7546 )
	ROM_LOAD16_BYTE( "sonpd1.bin", 0x100001, 0x040000, 0xC30E4C70 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "sonsnd0.bin", 0x00000, 0x40000, 0x569C8D4B )
	ROM_RELOAD(              0x100000, 0x40000             )
	ROM_LOAD( "sonsnd1.bin", 0x180000, 0x100000, 0xF4FA5A21 )
	ROM_LOAD( "sonsnd2.bin", 0x280000, 0x100000, 0xE1BD45A5 )
	ROM_LOAD( "sonsnd3.bin", 0x380000, 0x100000, 0xCF56B5A0 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "sonscl0.bin", 0x000000, 0x080000, 0x445E31B9 )
	ROM_LOAD32_BYTE( "sonscl1.bin", 0x000002, 0x080000, 0x3D234181 )
	ROM_LOAD32_BYTE( "sonscl2.bin", 0x000001, 0x080000, 0xA5DE28B2 )
	ROM_LOAD32_BYTE( "sonscl3.bin", 0x000003, 0x080000, 0x7CE7554B )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "sonobj0.bin", 0x800000, 0x100000, 0xCEEA18E3, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj1.bin", 0x800001, 0x100000, 0x6BBC226B, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj2.bin", 0x800002, 0x100000, 0xFCD5EF0E, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj3.bin", 0x800003, 0x100000, 0xB99B42AB, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj4.bin", 0x800004, 0x100000, 0xC7EC1456, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj5.bin", 0x800005, 0x100000, 0xBD5DA27F, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj6.bin", 0x800006, 0x100000, 0x313C92D1, ROM_SKIP(7) )
	ROMX_LOAD( "sonobj7.bin", 0x800007, 0x100000, 0x3784C507, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( holo )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "epr14977.a", 0x000000, 0x020000, 0xe0d7e288 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD16_WORD( "epr14976.a", 0x080000, 0x020000, 0xe56f13be )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr15011", 0x100000, 0x020000, 0xb9f59f59 )
	ROM_LOAD16_BYTE( "epr15010", 0x100001, 0x020000, 0x0c09c57b )

        ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr14965", 0x00000, 0x20000, 0x3a918cfe )
	ROM_RELOAD(           0x100000, 0x020000             )
	ROM_LOAD( "mpr14964", 0x180000, 0x100000, 0x7ff581d5 )
	ROM_LOAD( "mpr14962", 0x280000, 0x100000, 0x6b2e694e )
	ROM_LOAD( "mpr14963", 0x380000, 0x100000, 0x0974a60e )

	ROM_REGION( 0x000100, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	/* game doesn't use bg tilemaps */

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr14973", 0x800000, 0x100000, 0xb3c3ff6b, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14972", 0x800001, 0x100000, 0x0c161374, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14971", 0x800002, 0x100000, 0xdfcf6fdf, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14970", 0x800003, 0x100000, 0xcae3a745, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14969", 0x800004, 0x100000, 0xc06b7c15, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14968", 0x800005, 0x100000, 0xf413894a, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14967", 0x800006, 0x100000, 0x5377fce0, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14966", 0x800007, 0x100000, 0xdffba2e9, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( arabfgt )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "mp14608.8",  0x000000, 0x20000, 0xcd5efba9 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_RELOAD     (               0x080000, 0x20000 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "ep14592.18", 0x100000, 0x40000, 0xf7dff316 )
	ROM_LOAD16_BYTE( "ep14591.9",  0x100001, 0x40000, 0xbbd940fb )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU + banks */
	ROM_LOAD( "ep14596.36", 0x000000, 0x20000, 0xbd01faec )
	ROM_RELOAD(             0x100000, 0x20000             )
	ROM_LOAD( "mp14595f.35", 0x180000, 0x100000, 0x5173d1af )
	ROM_LOAD( "mp14596f.24", 0x280000, 0x100000, 0xaa037047 )
	ROM_LOAD( "mp14594f.34", 0x380000, 0x100000, 0x01777645 )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* Protection CPU */
	ROM_LOAD( "144680-1.3", 0x00000, 0x10000, 0xc3c591e4 )
	ROM_RELOAD(             0xf0000, 0x10000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mp14599f.14", 0x000000, 0x200000, 0x94f1cf10 )
	ROM_LOAD16_BYTE( "mp14598f.5",  0x000001, 0x200000, 0x010656f3 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mp14600f.32", 0x000000, 0x200000, 0xe860988a, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14602.30",  0x000002, 0x200000, 0x64524e4d, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14604.28",  0x000004, 0x200000, 0x5f8d5167, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14606.26",  0x000006, 0x200000, 0x7047f437, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14601f.31", 0x800000, 0x200000, 0xa2f3bb32, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14603.29",  0x800002, 0x200000, 0xf6ce494b, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14605.27",  0x800004, 0x200000, 0xaaf52697, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp14607.25",  0x800006, 0x200000, 0xb70b0735, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( brival )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "ep15720.8", 0x000000, 0x020000, 0x0d182d78 )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_RELOAD     (               0x080000, 0x20000 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "ep15723.18", 0x100000, 0x080000, 0x4ff40d39 )
	ROM_LOAD16_BYTE( "ep15724.9",  0x100001, 0x080000, 0x3ff8a052 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ep15725.36", 0x00000, 0x20000, 0xea1407d7 )
	ROM_RELOAD(             0x100000, 0x20000             )
	ROM_LOAD( "mp15627.35", 0x180000, 0x100000, 0x8a8388c5 )
	ROM_LOAD( "mp15625.24", 0x280000, 0x100000, 0x3ce82932 )
	ROM_LOAD( "mp15626.34", 0x380000, 0x100000, 0x83306d1e )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mp14599f.14", 0x000000, 0x200000, 0x1de17e83 )
	ROM_LOAD16_BYTE( "mp14598f.5",  0x000001, 0x200000, 0xcafb0de9 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mp15637.32", 0x000000, 0x200000, 0xf39844c0, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15635.30", 0x000002, 0x200000, 0x263cf6d1, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15633.28", 0x000004, 0x200000, 0x44e9a88b, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15631.26", 0x000006, 0x200000, 0xe93cf9c9, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15636.31", 0x800000, 0x200000, 0x079ff77f, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15634.29", 0x800002, 0x200000, 0x1edc14cd, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15632.27", 0x800004, 0x200000, 0x796215f2, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp15630.25", 0x800006, 0x200000, 0x8dabb501, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( alien3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "15943.bin", 0x000000, 0x040000, 0xac4591aa )
	ROM_RELOAD     (               0x040000, 0x40000 )
	ROM_LOAD( "15942.bin", 0x080000, 0x040000, 0xa1e1d0ec )
	ROM_RELOAD     (               0x0c0000, 0x40000 )
	ROM_LOAD16_BYTE( "15855.bin", 0x100000, 0x080000, 0xa6fadabe )
	ROM_LOAD16_BYTE( "15854.bin", 0x100001, 0x080000, 0xd1aec392 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "15859.bin", 0x00000, 0x40000, 0x91b55bd0 )
	ROM_RELOAD(            0x100000, 0x40000             )
	ROM_LOAD( "15858.bin", 0x180000, 0x100000, 0x2eb64c10 )
	ROM_LOAD( "15856.bin", 0x280000, 0x100000, 0xa5ef4f1f )
	ROM_LOAD( "15857.bin", 0x380000, 0x100000, 0x915c56df )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "15863.bin", 0x000000, 0x200000, 0x9d36b645 )
	ROM_LOAD16_BYTE( "15862.bin", 0x000001, 0x200000, 0x9e277d25 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "15864.bin", 0x000000, 0x200000, 0x58207157, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15866.bin", 0x000002, 0x200000, 0x9c53732c, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15868.bin", 0x000004, 0x200000, 0x62d556e8, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15870.bin", 0x000006, 0x200000, 0xd31c0400, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15865.bin", 0x800000, 0x200000, 0xdd64f87b, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15867.bin", 0x800002, 0x200000, 0x8cf9cb11, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15869.bin", 0x800004, 0x200000, 0xdd4b137f, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15871.bin", 0x800006, 0x200000, 0x58eb10ae, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( f1lap )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "15598", 0x000000, 0x020000, 0x9feab7cd )
	ROM_RELOAD     (               0x020000, 0x20000 )
	ROM_RELOAD     (               0x040000, 0x20000 )
	ROM_RELOAD     (               0x060000, 0x20000 )
	ROM_LOAD( "15599", 0x080000, 0x020000, 0x5c5ac112 )
	ROM_RELOAD     (               0x0a0000, 0x20000 )
	ROM_RELOAD     (               0x0c0000, 0x20000 )
	ROM_RELOAD     (               0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "15596", 0x100000, 0x040000, 0x20e92909 )
	ROM_LOAD16_BYTE( "15597", 0x100001, 0x040000, 0xcd1ccddb )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "15592", 0x00000, 0x20000, 0x7c055cc8 )
	ROM_RELOAD(        0x100000, 0x20000             )
	ROM_LOAD( "15593", 0x180000, 0x100000, 0xe7300441 )
	ROM_LOAD( "15595", 0x280000, 0x100000, 0x3fbdad9a )
	ROM_LOAD( "15594", 0x380000, 0x100000, 0x7f4ca3bb )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "15608", 0x000000, 0x200000, 0x64462c69 )
	ROM_LOAD16_BYTE( "15609", 0x000001, 0x200000, 0xd586e455 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "15600", 0x000000, 0x200000, 0xd2698d23, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15602", 0x000002, 0x200000, 0x1674764d, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15604", 0x000004, 0x200000, 0x1552bbb9, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15606", 0x000006, 0x200000, 0x2b4f5265, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15601", 0x800000, 0x200000, 0x31a8f40a, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15603", 0x800002, 0x200000, 0x3805ecbc, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15605", 0x800004, 0x200000, 0xcbdbf35e, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "15607", 0x800006, 0x200000, 0x6c8817c9, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /*  comms board  */
	ROM_LOAD( "15612", 0x00000, 0x20000, 0x9d204617 )
ROM_END

ROM_START( f1en )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "ep14452a.006", 0x000000, 0x20000, 0xb5b4a9d9 )
	ROM_RELOAD     (                  0x020000, 0x20000 )
	ROM_RELOAD     (                  0x040000, 0x20000 )
	ROM_RELOAD     (                  0x060000, 0x20000 )
	ROM_RELOAD     (                  0x080000, 0x20000 )
	ROM_RELOAD     (                  0x0a0000, 0x20000 )
	ROM_RELOAD     (                  0x0c0000, 0x20000 )
	ROM_RELOAD     (                  0x0e0000, 0x20000 )
	ROM_LOAD16_BYTE( "epr14445.014",  0x100000, 0x040000, 0xd06261ab )
	ROM_LOAD16_BYTE( "epr14444.007",  0x100001, 0x040000, 0x07724354 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr14449.035", 0x00000, 0x20000, 0x2d29699c )
	ROM_RELOAD(               0x100000, 0x20000             )
	ROM_LOAD( "epr14448.031", 0x180000, 0x080000, 0x87ca1e8d )
	ROM_LOAD( "epr14446.022", 0x280000, 0x080000, 0x646ec2cb )
	ROM_LOAD( "epr14447.026", 0x380000, 0x080000, 0xdb1cfcbd )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD32_BYTE( "mpr14362", 0x000000, 0x040000, 0xfb1c4e79 )
	ROM_LOAD32_BYTE( "mpr14361", 0x000002, 0x040000, 0xe3204bda )
	ROM_LOAD32_BYTE( "mpr14360", 0x000001, 0x040000, 0xc5e8da79 )
	ROM_LOAD32_BYTE( "mpr14359", 0x000003, 0x040000, 0x70305c68 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr14370", 0x800000, 0x080000, 0xfda78289, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14369", 0x800001, 0x080000, 0x7765116d, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14368", 0x800002, 0x080000, 0x5744a30e, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14367", 0x800003, 0x080000, 0x77bb9003, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14366", 0x800004, 0x080000, 0x21078e83, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14365", 0x800005, 0x080000, 0x36913790, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14364", 0x800006, 0x080000, 0x0fa12ecd, ROM_SKIP(7) )
	ROMX_LOAD( "mpr14363", 0x800007, 0x080000, 0xf3427a56, ROM_SKIP(7) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( dbzvrvs )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "16543",   0x000000, 0x80000, 0x7b9bc6f5 )
	ROM_LOAD16_WORD( "16542.a", 0x080000, 0x80000, 0x6449ab22 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "16541", 0x00000, 0x40000, 0x1d61d836 )
	ROM_RELOAD(        0x100000, 0x40000             )
	ROM_LOAD( "16540", 0x180000, 0x100000, 0xb6f9bb43 )
	ROM_LOAD( "16539", 0x280000, 0x100000, 0x38c26418 )
	ROM_LOAD( "16538", 0x380000, 0x100000, 0x4d402c31 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "16544", 0x000000, 0x100000, 0xf6c93dfc )
	ROM_LOAD16_BYTE( "16545", 0x000001, 0x100000, 0x51748bac )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "16546", 0x000000, 0x200000, 0x96f4be31, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16548", 0x000002, 0x200000, 0x00377f59, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16550", 0x000004, 0x200000, 0x168e8966, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16553", 0x000006, 0x200000, 0xc0a43009, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16547", 0x800000, 0x200000, 0x50d328ed, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16549", 0x800002, 0x200000, 0xa5802e9f, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16551", 0x800004, 0x200000, 0xdede05fc, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16552", 0x800006, 0x200000, 0xa31dae31, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( darkedge )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_WORD( "epr15244.8", 0x000000, 0x80000, 0x0db138cb )
	ROM_RELOAD     (               0x080000, 0x80000 )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr15243.36", 0x00000, 0x20000, 0x08ca5f11 )
	ROM_RELOAD(              0x100000, 0x20000             )
	ROM_LOAD( "mpr15242.35", 0x180000, 0x100000, 0xffb7d917 )
	ROM_LOAD( "mpr15240.24", 0x280000, 0x100000, 0x867d59e8 )
	ROM_LOAD( "mpr15241.34", 0x380000, 0x100000, 0x8eccc4fe )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr15248", 0x000000, 0x080000, 0x185b308b )
	ROM_LOAD16_BYTE( "mpr15247", 0x000001, 0x080000, 0xbe21548c )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr15249.32", 0x000000, 0x200000, 0x2b4371a8, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15251.30", 0x000002, 0x200000, 0xefe2d689, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15253.28", 0x000004, 0x200000, 0x8356ed01, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15255.26", 0x000006, 0x200000, 0xff04a5b0, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15250.31", 0x800000, 0x200000, 0xc5cab71a, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15252.29", 0x800002, 0x200000, 0xf8885fea, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15254.27", 0x800004, 0x200000, 0x7765424b, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15256.25", 0x800006, 0x200000, 0x44c36b62, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */
ROM_END

ROM_START( jpark )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD( "ep16402a.8", 0x000000, 0x080000, 0xc70db239  )
	ROM_RELOAD     (               0x080000, 0x80000 )
	ROM_LOAD16_BYTE( "ep16395.18", 0x100000, 0x080000, 0xac5a01d6  )
	ROM_LOAD16_BYTE( "ep16394.9",  0x100001, 0x080000, 0xc08c3a8a  )

	ROM_REGION( 0x480000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ep16399.36", 0x00000, 0x40000, 0xb09b2fe3  )
	ROM_RELOAD(             0x100000, 0x40000             )
	ROM_LOAD( "mp16398.35", 0x180000, 0x100000, 0xfa710ca6  )
	ROM_LOAD( "mp16396.24", 0x280000, 0x100000, 0xf69a2dc4  )
	ROM_LOAD( "mp16397.34", 0x380000, 0x100000, 0x6e96e0be  )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mp16404.14", 0x000000, 0x200000, 0x11283807  )
	ROM_LOAD16_BYTE( "mp16403.5",  0x000001, 0x200000, 0x02530a9b  )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mp16405.32", 0x000000, 0x200000, 0xb425f182 , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16407.30", 0x000002, 0x200000, 0xbc49ffd9 , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16409.28", 0x000004, 0x200000, 0xfe73660d , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16411.26", 0x000006, 0x200000, 0x71cabbc5 , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16406.31", 0x800000, 0x200000, 0xb9ed73d6 , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16408.29", 0x800002, 0x200000, 0x7b2f476b , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16410.27", 0x800004, 0x200000, 0x49c8f952 , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mp16412.25", 0x800006, 0x200000, 0x105dc26e , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
	/* populated at runtime */

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* unused */
	ROM_LOAD( "ep13908.xx", 0x00000, 0x8000, 0x6228c1d2  ) /* cabinet movement */
ROM_END

static WRITE16_HANDLER( trap_w )
{
//	printf("Write %x to magic (mask=%x) at PC=%x\n", data, mem_mask, activecpu_get_pc());
}

static DRIVER_INIT ( s32 )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	system32_temp_kludge = 0;
	system32_draw_bgs = 1;

	system32_palMask = 0xff;
	system32_mixerShift = 4;

	install_mem_write16_handler(0, 0x20f4e0, 0x20f4e1, trap_w);
}

static DRIVER_INIT ( alien3 )
{
	system32_use_default_eeprom = EEPROM_ALIEN3;

	system32_temp_kludge = 0;
	system32_draw_bgs = 1;

	system32_palMask = 0xff;
	system32_mixerShift = 4;

	install_mem_read16_handler(0, 0xc00050, 0xc00051, sys32_gun_p1_x_c00050_r);
	install_mem_read16_handler(0, 0xc00052, 0xc00053, sys32_gun_p1_y_c00052_r);
	install_mem_read16_handler(0, 0xc00054, 0xc00055, sys32_gun_p2_x_c00054_r);
	install_mem_read16_handler (0, 0xc00056, 0xc00057, sys32_gun_p2_y_c00056_r);

	install_mem_write16_handler(0, 0xc00050, 0xc00051, sys32_gun_p1_x_c00050_w);
	install_mem_write16_handler(0, 0xc00052, 0xc00053, sys32_gun_p1_y_c00052_w);
	install_mem_write16_handler(0, 0xc00054, 0xc00055, sys32_gun_p2_x_c00054_w);
	install_mem_write16_handler(0, 0xc00056, 0xc00057, sys32_gun_p2_y_c00056_w);
}

static DRIVER_INIT ( brival )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	system32_temp_kludge = 0;
	system32_draw_bgs = 1;

	system32_palMask = 0xff;
	system32_mixerShift = 4;

	install_mem_read16_handler (0, 0x20ba00, 0x20ba07, brival_protection_r);
	install_mem_write16_handler(0, 0xa000000, 0xa00fff, brival_protboard_w);
}

static DRIVER_INIT ( ga2 )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	/* Protection - the game expects a string from a RAM area shared with the protection device */
	install_mem_read16_handler (0, 0xa00000, 0xa0001f, ga2_sprite_protection_r); /* main sprite colours */
	install_mem_read16_handler (0, 0xa00100, 0xa0015f, ga2_wakeup_protection_r);

	system32_temp_kludge = 0;
	system32_draw_bgs = 1;
	system32_palMask = 0x7f;
	system32_mixerShift = 3;

}

static DRIVER_INIT ( spidey )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	system32_palMask = 0x7f;
	system32_mixerShift = 3;
	system32_draw_bgs = 1;
}

static DRIVER_INIT ( f1sl )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	system32_palMask = 0x7f;
	system32_mixerShift = 3;
	system32_draw_bgs = 1;
}

static DRIVER_INIT ( arf )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	install_mem_read16_handler (0, 0xa00000, 0xa000ff, arabfgt_protboard_r);
	install_mem_read16_handler (0, 0xa00100, 0xa0011f, arf_wakeup_protection_r);
	install_mem_write16_handler(0, 0xa00000, 0xa00fff, arabfgt_protboard_w);

	system32_palMask = 0xff;
	system32_mixerShift = 4;

	system32_temp_kludge = 0;

	system32_draw_bgs = 1;
	/* Main Character Colours are also controlled by protection? */
}

static DRIVER_INIT ( holo )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	/* holoseum requires the tx tilemap to be flipped, also doesn't seem to like my palette select yet */
	system32_temp_kludge = 1;

	system32_palMask = 0xff;
	system32_mixerShift = 4;
	system32_draw_bgs = 0; // doesn't have backgrounds
}

static DRIVER_INIT ( sonic )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	system32_palMask = 0x1ff;
	system32_mixerShift = 5;
	system32_draw_bgs = 1;

	install_mem_write16_handler(0, 0xc00040, 0xc00055, sonic_track_reset_w);
	install_mem_read16_handler (0, 0xc00040, 0xc00055, sonic_track_r);
}

static DRIVER_INIT ( driving )
{
	system32_palMask = 0x1ff;
	system32_mixerShift = 5;
	system32_draw_bgs = 1;

	install_mem_read16_handler (0, 0xc00050, 0xc00051, sys32_driving_steer_c00050_r);
	install_mem_read16_handler (0, 0xc00052, 0xc00053, sys32_driving_accel_c00052_r);
	install_mem_read16_handler (0, 0xc00054, 0xc00055, sys32_driving_brake_c00054_r);

	install_mem_write16_handler(0, 0xc00050, 0xc00051, sys32_driving_steer_c00050_w);
	install_mem_write16_handler(0, 0xc00052, 0xc00053, sys32_driving_accel_c00052_w);
	install_mem_write16_handler(0, 0xc00054, 0xc00055, sys32_driving_brake_c00054_w);
}

static DRIVER_INIT ( radm )
{
	system32_use_default_eeprom = EEPROM_RADM;

	init_driving();
}

static DRIVER_INIT ( radr )
{
	system32_use_default_eeprom = EEPROM_RADR;

	init_driving();
}

static DRIVER_INIT ( f1en )
{
	system32_use_default_eeprom = EEPROM_SYS32_0;

	init_driving();
}

static DRIVER_INIT ( jpark )
{

	/* Temp. Patch until we emulate the 'Drive Board', thanks to Malice */
	data16_t *pROM = (data16_t *)memory_region(REGION_CPU1);
	pROM[0xC15A8/2] = 0xCD70;
	pROM[0xC15AA/2] = 0xD8CD;

	system32_draw_bgs = 1;
	system32_palMask = 0xff;
	system32_mixerShift = 4;

	install_mem_read16_handler(0, 0xc00050, 0xc00051, sys32_gun_p1_x_c00050_r);
	install_mem_read16_handler(0, 0xc00052, 0xc00053, sys32_gun_p1_y_c00052_r);
	install_mem_read16_handler(0, 0xc00054, 0xc00055, sys32_gun_p2_x_c00054_r);
	install_mem_read16_handler (0, 0xc00056, 0xc00057, sys32_gun_p2_y_c00056_r);

	install_mem_write16_handler(0, 0xc00050, 0xc00051, sys32_gun_p1_x_c00050_w);
	install_mem_write16_handler(0, 0xc00052, 0xc00053, sys32_gun_p1_y_c00052_w);
	install_mem_write16_handler(0, 0xc00054, 0xc00055, sys32_gun_p2_x_c00054_w);
	install_mem_write16_handler(0, 0xc00056, 0xc00057, sys32_gun_p2_y_c00056_w);
}


/* this one is pretty much ok since it doesn't use backgrounds tilemaps */
GAME( 1992, holo,     0,        system32, holo,     holo,     ROT0, "Sega", "Holosseum" ) /* fine */

/* these have a range of issues, mainly with the backgrounds */
GAMEX(1994, svf,      0,        system32, svf,      s32,      ROT0, "Sega", "Super Visual Football", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing, playable */
GAMEX(1994, svs,	  svf,		system32, svf,		s32,	  ROT0, "Sega", "Super Visual Soccer (US?)", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing, playable */
GAMEX(1994, jleague,  svf,      system32, svf,      s32,      ROT0, "Sega", "The J.League 94 (Japan)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION ) /* like svf but fails when you coin it up */
GAMEX(1992, brival,   0,        sys32_hi, brival,   brival,   ROT0, "Sega", "Burning Rival (Japan)", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up, missing bg's, playable */
GAMEX(1990, radm,     0,        system32, radm,     radm,     ROT0, "Sega", "Rad Mobile", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up, prelim bg, playable */
GAMEX(1991, radr,     0,        sys32_hi, radr,     radr,     ROT0, "Sega", "Rad Rally", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up, prelim bg, playable */
GAMEX(199?, f1en,     0,        system32, f1en,     f1en,     ROT0, "Sega", "F1 Exhaust Note", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up prelim bg, playable  */
GAMEX(1993, alien3,   0,        system32, alien3,   alien3,   ROT0, "Sega", "Alien 3", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up, missing bg layers also most gfx are done with sprites so present */
GAMEX(1992, sonic,    0,        sys32_hi, sonic,    sonic,    ROT0, "Sega", "Sonic (Japan rev. C)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION ) /* boots, coins up, bg's missing, no control, protection */
GAMEX(1992, sonicp,   sonic,    sys32_hi, sonic,    sonic,    ROT0, "Sega", "Sonic (Japan prototype)", GAME_IMPERFECT_GRAPHICS ) /* boots, coins up, missing bg gfx, no controls */
GAMEX(1994, jpark,    0,        jpark,    jpark,    jpark,    ROT0, "Sega", "Jurassic Park", GAME_IMPERFECT_GRAPHICS )  /* drive board test patched, uses hi-res on a test screen */
GAMEX(1992, ga2,      0,        system32, ga2,      ga2,      ROT0, "Sega", "Golden Axe - The Revenge of Death Adder (US)", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing, sprite colour probs */
GAMEX(1992, ga2j,     ga2,      system32, ga2j,     ga2,      ROT0, "Sega", "Golden Axe - The Revenge of Death Adder (Japan)", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing, sprite colour probs  - can be set to 4 player but controls won't work as is, they conflict with the 2 player setup */
GAMEX(1991, spidey,   0,        system32, spidey,   spidey,   ROT0, "Sega", "Spiderman (US)", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing */
GAMEX(1991, spideyj,  spidey,   system32, spideyj,  spidey,   ROT0, "Sega", "Spiderman (Japan)", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing */
GAMEX(1991, arabfgt,  0,        system32, spidey,   arf,      ROT0, "Sega", "Arabian Fight", GAME_IMPERFECT_GRAPHICS ) /* boots, bg's missing */

/* not really working */
GAMEX(199?, f1lap,    0,        system32, system32, f1sl,     ROT0, "Sega", "F1 Super Lap", GAME_NOT_WORKING ) /* blank screen, also requires 2 linked sys32 boards to function */
GAMEX(199?, dbzvrvs,  0,        sys32_hi, system32, s32,      ROT0, "Sega", "Dragon Ball Z VRVS", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION) /* does nothing useful, known to be heavily protected */
GAMEX(1992, darkedge, 0,        sys32_hi, system32, s32,      ROT0, "Sega", "Dark Edge", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION ) /* locks up on some levels, sprites are submerged, protected */
/* Air Rescue */
/* Loony Toons (maybe) */

/* the following are Multi-32 (v70 based, 2 monitors) */
/* Hard Dunk */
/* Outrunners */
/* Stadium Cross */
/* Title Fight */
