/*
	Sega Multi System 32 hardware

 preliminary support by Jason Lo aka fbff

 based on earlier work by R.Belmont and David Haywood which was
 in turn based on the Modeler emulator

 Main ToDo's:

 I/O bits including controls
 Fix remaining Gfx problems
 convert from using the 16-bit V60 to the 32-bit V70 (I'm doing this
  later as I couldn't get it to boot with the V70 for now and the gfx
  hardware is easier to work with this way)
 Support both monitors


*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "machine/random.h"

#define OSC_A	(32215900)	// System 32 master crystal is 32215900 Hz
#define Z80_CLOCK (OSC_A/4)

int multi32;

static unsigned char irq_status;
static data16_t *system32_shared_ram;
data16_t *system32_mixerregs_monitor_a;		// mixer registers
data16_t *system32_mixerregs_monitor_b;		// mixer registers

static data16_t *sys32_protram;
static data16_t *system32_workram;
data16_t *sys32_tilebank_external;
data16_t* sys32_displayenable;

/* Video Hardware */
int system32_temp_kludge;
data16_t *sys32_spriteram16;
data16_t *sys32_txtilemap_ram;
data16_t *sys32_ramtile_ram;
data16_t *scrambled_paletteram16;

extern int sys32_sprite_priority_kludge;

int system32_palMask;
int system32_mixerShift;
extern int system32_screen_mode;
extern int system32_screen_old_mode;
extern int system32_allow_high_resolution;

WRITE16_HANDLER( sys32_videoram_w );
WRITE16_HANDLER ( sys32_ramtile_w );
WRITE16_HANDLER ( sys32_spriteram_w );
READ16_HANDLER ( sys32_videoram_r );
WRITE32_HANDLER( sys32_videoram_long_w );
READ32_HANDLER ( sys32_videoram_long_r );
VIDEO_START( system32 );
VIDEO_UPDATE( system32 );

int system32_use_default_eeprom;

static data16_t controlB[256];
static data16_t control[256];

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


static READ16_HANDLER(sys32_read_ff)
{
	return 0xffff;
}

static READ16_HANDLER(sys32_read_random)
{
	// some totally bogus "random" algorithm
//	s32_rand += 0x10001;

	return mame_rand(); // new random.c random number code, see clouds in ga2
}

extern int sys32_brightness_monitor_a[3];

void multi32_set_colour (int offset)
{
	int data;
	int r,g,b;
	int r2,g2,b2;
	UINT16 r_bright, g_bright, b_bright;

	data = paletteram16[offset];


	r = (data >> 0) & 0x0f;
	g = (data >> 4) & 0x0f;
	b = (data >> 8) & 0x0f;

//	r2 = (data >> 12) & 0x1;
	r2 = (data >> 13) & 0x1;
	g2 = (data >> 13) & 0x1;
	b2 = (data >> 13) & 0x1;

	r = (r << 4) | (r2 << 3);
	g = (g << 4) | (g2 << 3);
	b = (b << 4) | (b2 << 3);

	/* there might be better ways of doing this ... but for now its functional ;-) */
	r_bright = sys32_brightness_monitor_a[0]; r_bright &= 0x3f;
	g_bright = sys32_brightness_monitor_a[1]; g_bright &= 0x3f;
	b_bright = sys32_brightness_monitor_a[2]; b_bright &= 0x3f;

	if ((r_bright & 0x20)) { r = (r * (r_bright&0x1f))>>5; } else { r = r+(((0xf8-r) * (r_bright&0x1f))>>5); }
	if ((g_bright & 0x20)) { g = (g * (g_bright&0x1f))>>5; } else { g = g+(((0xf8-g) * (g_bright&0x1f))>>5); }
	if ((b_bright & 0x20)) { b = (b * (b_bright&0x1f))>>5; } else { b = b+(((0xf8-b) * (b_bright&0x1f))>>5); }

	palette_set_color(offset,r,g,b);
}

static READ16_HANDLER( multi32_palette_r )
{
	if (offset>0x1000)
		return paletteram16[offset];
	else
		return scrambled_paletteram16[offset];
}

static WRITE16_HANDLER( multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w )
{
	int r,g,b;
	int r2,g2,b2;

//	logerror("Palette Scrambled Write %04d [%d:%06x]: %04x \n", offset, cpu_getactivecpu(), activecpu_get_pc(), data);
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

	multi32_set_colour(offset);

}

static WRITE16_HANDLER( multi32_paletteram16_xBGRBBBBGGGGRRRR_word_w )
{
//	logerror("Palette Write %04d [%d:%06x]: %04x \n", offset, cpu_getactivecpu(), activecpu_get_pc(), data);
	COMBINE_DATA(&paletteram16[offset]);

	// some games use 8-bit writes to some palette regions
	// (especially for the text layer palettes)

	multi32_set_colour(offset);
}



static READ16_HANDLER( multi32_io_r )
{
	switch(offset)
	{
	case 0:	// c00001 / c00003
		return (input_port_1_r(0)<<16) | input_port_2_r(0);
		break;
	case 2:	// c00008 / c0000a
		return (input_port_3_r(0)<<16) | input_port_0_r(0);
		break;
	case 0x18:	// c00060 / c00062
		return (input_port_4_r(0)<<16) | input_port_5_r(0);
		break;
	case 0x19:	// c00064 / c00068
		return (input_port_6_r(0)<<16) | 0xff;
		break;
	default:
		logerror("Port A %d [%d:%06x]: read (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), mem_mask);
		return 0xffff;
		break;
	}
}

static WRITE16_HANDLER( multi32_io_w )
{
	COMBINE_DATA(&control[offset]);
	switch(offset) {
	case 00:
		// value = c8?
		break;
	case 03:
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
		break;
	case 25:
		// value = ff?
		break;
	case 26:
		// value = ff?
		break;
	case 27:
		// value = ff?
		break;
	case 28:
		// value = ff?
		break;
	case 33:
		// value = 00?
		break;
	case 34:
		// dbzvrvs value = 00?
		break;
	case 35:
		// harddunk value = 00?
		break;
	case 36:
		// harddunk value = 93?
		break;
	case 39:
		// dbzvrvs value = f0?
		break;
	case 40:
		// dbzvrvs value = 00?
		break;
	case 43:
		// value = 13?
		break;
	default:
		logerror("Port A %d [%d:%06x]: %02x\n", offset, cpu_getactivecpu(), activecpu_get_pc(), data);
		break;
	}
}

static READ16_HANDLER( multi32_io_B_r )
{
	switch(offset) {
	case 0:
		// orunners (mask ff00)
		return 0xffff;
	case 1:
		// orunners (mask ff00)
		return 0xffff;
	case 2:
		return (EEPROM_read_bit() << 23) | input_port_0_r(0);
	case 4:
		// orunners (mask ff00)
		return 0xffff;
	case 5:
		// orunners (mask ff00)
		return 0xffff;
	case 7:
		// orunners (mask ff00)
		return 0xffff;
	case 14:
		// harddunk (mask ff00)
		return 0xffff;
	default:
		logerror("Port B %d [%d:%06x]: read (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), mem_mask);
		return 0xffff;
	}
}

static WRITE16_HANDLER( multi32_io_B_w )
{
	COMBINE_DATA(&controlB[offset]);
	switch(offset) {
	case 3:
		EEPROM_write_bit(data & 0x800000);
		EEPROM_set_cs_line((data & 0x200000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x400000) ? ASSERT_LINE : CLEAR_LINE);
		break;
	case 06:
		// orunners value=00, 08, 34
		break;
	case 07:
		// orunners value=00, 1f
		break;
	case 14:
		// orunners value=86
		break;
	case 15:
		// orunners value=c8
		break;
	default:
		logerror("Port B %d [%d:%06x]: %02x (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask);
		break;
	}
}

static MEMORY_READ16_START( multi32_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x23ffff, MRA16_RAM }, // work RAM
	{ 0x300000, 0x31ffff, sys32_videoram_r }, // Tile Ram
	{ 0x400000, 0x41ffff, MRA16_RAM }, // sprite RAM
	{ 0x500000, 0x50000d, MRA16_RAM },	// Unknown

	{ 0x600000, 0x60ffff, multi32_palette_r }, // Video A
//	{ 0x600000, 0x607fff, MRA16_RAM }, // palette RAM
//	{ 0x608000, 0x60ffff, MRA16_RAM }, // palette RAM
	{ 0x610000, 0x6100ff, MRA16_RAM }, // mixer chip registers

	{ 0x680000, 0x69004f, MRA16_RAM }, // Video B
//	{ 0x680000, 0x687fff, MRA16_RAM }, // Unknown
//	{ 0x688000, 0x68ffff, MRA16_RAM }, // monitor B palette
//	{ 0x690000, 0x69004f, MRA16_RAM }, // monitor B mixer registers

	{ 0x700000, 0x701fff, MRA16_RAM },	// shared RAM
	{ 0x800000, 0x80000f, MRA16_RAM },	// Unknown
	{ 0x80007e, 0x80007f, MRA16_RAM },	// Unknown f1lap
	{ 0x801000, 0x801003, MRA16_RAM },	// Unknown
	{ 0xa00000, 0xa00001, MRA16_RAM }, // Unknown dbzvrvs
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
	{ 0xc80000, 0xc8007f, multi32_io_B_r },
	{ 0xd80000, 0xd80001, sys32_read_random },
	{ 0xd80002, 0xd80003, MRA16_RAM }, // Unknown harddunk
	{ 0xe00000, 0xe0000f, MRA16_RAM },   // Unknown
	{ 0xe80000, 0xe80003, MRA16_RAM }, // Unknown
	{ 0xf00000, 0xffffff, MRA16_BANK1 }, // High rom mirror
MEMORY_END

static MEMORY_WRITE16_START( multi32_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x23ffff, MWA16_RAM, &system32_workram },
	{ 0x300000, 0x31ffff, sys32_videoram_w },
	{ 0x400000, 0x41ffff, sys32_spriteram_w, &sys32_spriteram16 }, // Sprites
	{ 0x500000, 0x50000d, MWA16_RAM },	// Unknown

	{ 0x600000, 0x607fff, multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w, &scrambled_paletteram16 },	// magic data-line-scrambled mirror of palette RAM * we need to shuffle data written then?
	{ 0x608000, 0x60ffff, multi32_paletteram16_xBGRBBBBGGGGRRRR_word_w, &paletteram16 }, // Palettes
	{ 0x610000, 0x6100ff, MWA16_RAM, &system32_mixerregs_monitor_a }, // mixer chip registers

	{ 0x680000, 0x68ffff, MWA16_RAM }, // Video B
//	{ 0x680000, 0x687fff, MWA16_RAM }, // Unknown
//	{ 0x688000, 0x68ffff, MWA16_RAM }, // monitor B palette
	{ 0x690000, 0x69004f, MWA16_RAM, &system32_mixerregs_monitor_b }, // monitor B mixer registers

	{ 0x700000, 0x701fff, MWA16_RAM, &system32_shared_ram }, // Shared ram with the z80
	{ 0x800000, 0x80000f, MWA16_RAM },	// Unknown
	{ 0x80007e, 0x80007f, MWA16_RAM },	// Unknown f1lap
	{ 0x801000, 0x801003, MWA16_RAM },	// Unknown
	{ 0x81002a, 0x81002b, MWA16_RAM },	// Unknown dbzvrvs
	{ 0x810100, 0x810101, MWA16_RAM },	// Unknown dbzvrvs
	{ 0xa00000, 0xa00fff, MWA16_RAM, &sys32_protram },	// protection RAM
	{ 0xc00006, 0xc00007, system32_eeprom_w },

//	{ 0xc0000c, 0xc0000d, jp_v60_write_cab },

	{ 0xc00008, 0xc0000d, MWA16_RAM }, // Unknown c00008=f1lap
	{ 0xc0000e, 0xc0000f, MWA16_RAM, &sys32_tilebank_external }, // tilebank per layer on multi32
	{ 0xc0001c, 0xc0001d, MWA16_RAM, &sys32_displayenable },
	{ 0xc0001e, 0xc0007f, multi32_io_w },

//	{ 0xc0001e, 0xc0001f, MWA16_RAM }, // Unknown
//	{ 0xc00050, 0xc00057, MWA16_RAM }, // Unknown
//	{ 0xc00060, 0xc00061, MWA16_RAM }, // Unknown
//	{ 0xc00074, 0xc00075, MWA16_RAM }, // Unknown

	{ 0xc80000, 0xc8007f, multi32_io_B_w },
	{ 0xd00000, 0xd00005, MWA16_RAM }, // Unknown
	{ 0xd00006, 0xd00007, irq_ack_w },
	{ 0xd00008, 0xd0000b, MWA16_RAM }, // Unknown
	{ 0xd80000, 0xd80003, MWA16_RAM }, // Unknown titlef / harddunk
	{ 0xe00000, 0xe0000f, MWA16_RAM },   // Unknown
	{ 0xe80000, 0xe80003, MWA16_RAM }, // Unknown
	{ 0xf00000, 0xffffff, MWA16_ROM },
MEMORY_END




static MACHINE_INIT( system32 )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();

	/* force it to select lo-resolution on reset */
	system32_allow_high_resolution = 0;
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



static void irq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}


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

static UINT8 *sys32_SoundMemBank;

static READ_HANDLER( system32_bank_r )
{
	return sys32_SoundMemBank[offset];
}


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

static MEMORY_READ_START( multi32_sound_readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system32_bank_r },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_r },
	{ 0xe000, 0xffff, sys32_shared_snd_r },
MEMORY_END

static MEMORY_WRITE_START( multi32_sound_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_w },
	{ 0xe000, 0xffff, sys32_shared_snd_w },
MEMORY_END

static WRITE_HANDLER( sys32_soundbank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int Bank;

	Bank = data * 0x2000;

	sys32_SoundMemBank = &RAM[Bank+0x10000];
}

static PORT_READ_START( multi32_sound_readport )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
PORT_END

static PORT_WRITE_START( multi32_sound_writeport )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0xa0, 0xa0, sys32_soundbank_w },
	{ 0xb0, 0xb0, MultiPCM_bank_0_w },
	{ 0xc1, 0xc1, IOWP_NOP },
PORT_END

struct YM2612interface mul32_ym3438_interface =
{
	1,
	Z80_CLOCK,
	{ 60,60 },
	{ 0 },	{ 0 },	{ 0 },	{ 0 },
	{ irq_handler }
};

static struct MultiPCM_interface mul32_multipcm_interface =
{
	1,		// 1 chip
	{ Z80_CLOCK },	// clock
	{ MULTIPCM_MODE_MULTI32 },	// banking mode
	{ (512*1024) },	// bank size
	{ REGION_SOUND1 },	// sample region
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};

static MACHINE_DRIVER_START( multi32 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V60, 20000000/10) // Reality is 20mhz but V60/V70 timings are unknown
	MDRV_CPU_MEMORY(multi32_readmem,multi32_writemem)
	MDRV_CPU_VBLANK_INT(system32_interrupt,2)

	MDRV_CPU_ADD(Z80, Z80_CLOCK)
	MDRV_CPU_MEMORY(multi32_sound_readmem, multi32_sound_writemem)
	MDRV_CPU_PORTS(multi32_sound_readport, multi32_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100 /*DEFAULT_60HZ_VBLANK_DURATION*/)

	MDRV_MACHINE_INIT(system32)
	MDRV_NVRAM_HANDLER(system32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT ) // RGB_DIRECT will be needed for alpha
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
//	MDRV_SCREEN_SIZE(1200, 1024)
//	MDRV_VISIBLE_AREA(0*8, 1200-1, 0*8, 1024-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(system32)
	MDRV_VIDEO_UPDATE(system32)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3438, mul32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, mul32_multipcm_interface)
MACHINE_DRIVER_END

static data16_t sys32_driving_steer_c00050_data;
static data16_t sys32_driving_accel_c00052_data;
static data16_t sys32_driving_brake_c00054_data;

static WRITE16_HANDLER ( sys32_driving_steer_c00050_w ) { sys32_driving_steer_c00050_data = readinputport(7); }
static WRITE16_HANDLER ( sys32_driving_accel_c00052_w ) { sys32_driving_accel_c00052_data = readinputport(8); }
static WRITE16_HANDLER ( sys32_driving_brake_c00054_w ) { sys32_driving_brake_c00054_data = readinputport(9); }

static READ16_HANDLER ( sys32_driving_steer_c00050_r ) { int retdata; retdata = sys32_driving_steer_c00050_data & 0x80; sys32_driving_steer_c00050_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_driving_accel_c00052_r ) { int retdata; retdata = sys32_driving_accel_c00052_data & 0x80; sys32_driving_accel_c00052_data <<= 1; return retdata; }
static READ16_HANDLER ( sys32_driving_brake_c00054_r ) { int retdata; retdata = sys32_driving_brake_c00054_data & 0x80; sys32_driving_brake_c00054_data <<= 1; return retdata; }


static DRIVER_INIT ( driving_m32 )
{
	install_mem_read16_handler (0, 0xc00050, 0xc00051, sys32_driving_steer_c00050_r);
	install_mem_read16_handler (0, 0xc00052, 0xc00053, sys32_driving_accel_c00052_r);
	install_mem_read16_handler (0, 0xc00054, 0xc00055, sys32_driving_brake_c00054_r);

	install_mem_write16_handler(0, 0xc00050, 0xc00051, sys32_driving_steer_c00050_w);
	install_mem_write16_handler(0, 0xc00052, 0xc00053, sys32_driving_accel_c00052_w);
	install_mem_write16_handler(0, 0xc00054, 0xc00055, sys32_driving_brake_c00054_w);
}

static DRIVER_INIT(orunners)
{
	multi32=1;
//	m32_vbIn = 0;
//	m32_vbOut = 1;
//	system32_use_default_eeprom = EEPROM_SYS32_0;
	sys32_sprite_priority_kludge = 0xf;
	system32_temp_kludge = 0;
	system32_palMask = 0xff;
	system32_mixerShift = 4;
	init_driving_m32();
}

static DRIVER_INIT(titlef)
{
	multi32=1;
//	m32_vbIn = 1;
//	m32_vbOut = 0;
//	system32_use_default_eeprom = EEPROM_SYS32_0;
	sys32_sprite_priority_kludge = 0xf;
	system32_temp_kludge = 0;
	system32_palMask = 0x1ff;
	system32_mixerShift = 5;

}

static DRIVER_INIT(harddunk)
{
	multi32=1;
//	m32_vbIn = 1;
//	m32_vbOut = 0;
//	system32_use_default_eeprom = EEPROM_SYS32_0;
	sys32_sprite_priority_kludge = 0xf;
	system32_temp_kludge = 0;
	system32_palMask = 0x1ff;
	system32_mixerShift = 5;

}

#if 0
static UINT8 *sys32_SoundMemBank;

// the Z80's work RAM is fully shared with the V60 or V70 and battery backed up.
static READ_HANDLER( sys32_shared_snd_long_r )
{
	data8_t *RAM = (data8_t *)system32_shared_ram_long;

	return RAM[offset];
}

static WRITE_HANDLER( sys32_shared_snd_long_w )
{
	data8_t *RAM = (data8_t *)system32_shared_ram_long;

	RAM[offset] = data;
}


#endif

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
INPUT_PORTS_START( orunners )
	PORT_START	// 0xc0000a - port 0
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	// 0xc00000 - port 1
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON2, "Shift Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3, "Shift Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_START	// 0xc00002 - port 2
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4, "DJ / Music", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON5, "Radio Back", IP_KEY_DEFAULT, IP_JOY_DEFAULT  )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON6, "Radio Forward", IP_KEY_DEFAULT, IP_JOY_DEFAULT  )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )


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
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 50, 20, 0x00, 0xff)

	PORT_START	// 0xc00052 - port 8 - accel pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER1, 50, 20, 0x00, 0xff)

	PORT_START	// 0xc00054 - port 9 - brake pedal
	PORT_ANALOG( 0xff, 0x80, IPT_PEDAL | IPF_PLAYER2, 50, 20, 0x00, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( titlef )
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

INPUT_PORTS_START( harddunk )
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



ROM_START( orunners )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "epr15618.bin", 0x000000, 0x020000, 0x25647f76 )
	ROM_RELOAD(                      0x040000, 0x020000 )
	ROM_RELOAD(                      0x080000, 0x020000 )
	ROM_RELOAD(                      0x0c0000, 0x020000 )
	ROM_LOAD32_WORD( "epr15619.bin", 0x000002, 0x020000, 0x2a558f95 )
	ROM_RELOAD(                      0x040002, 0x020000 )
	ROM_RELOAD(                      0x080002, 0x020000 )
	ROM_RELOAD(                      0x0c0002, 0x020000 )

	/* v60 data */
	ROM_LOAD32_WORD( "mpr15538.bin", 0x100000, 0x080000, 0x93958820 )
	ROM_LOAD32_WORD( "mpr15539.bin", 0x100002, 0x080000, 0x219760fa )

	ROM_REGION( 0x90000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("epr15550.bin", 0x00000, 0x80000, 0x0205d2ed )
	ROM_RELOAD(              0x10000, 0x80000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr15548.bin", 0x000000, 0x200000, 0xb6470a66 )
	ROM_LOAD16_BYTE( "mpr15549.bin", 0x000001, 0x200000, 0x81d12520 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr15540.bin", 0x000000, 0x200000, 0xa10d72b4, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15542.bin", 0x000002, 0x200000, 0x40952374, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15544.bin", 0x000004, 0x200000, 0x39e3df45, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15546.bin", 0x000006, 0x200000, 0xe3fcc12c, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15541.bin", 0x800000, 0x200000, 0xa2003c2d, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15543.bin", 0x800002, 0x200000, 0x933e8e7b, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15545.bin", 0x800004, 0x200000, 0x53dd0235, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15547.bin", 0x800006, 0x200000, 0xedcb2a43, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mpr15551.bin", 0x000000, 0x200000, 0x4894bc73)
	ROM_LOAD("mpr15552.bin", 0x200000, 0x200000, 0x1c4b5e73)

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( harddunk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "16508", 0x000000, 0x40000, 0xb3713be5 )
	ROM_RELOAD(                      0x080000, 0x040000 )
	ROM_LOAD32_WORD( "16509", 0x000002, 0x40000, 0x603dee75 )
	ROM_RELOAD(                      0x080002, 0x040000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("16505",        0x00000, 0x20000, 0xeeb90a07 )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "16503", 0x000000, 0x080000, 0xac1b6f1a )
	ROM_LOAD16_BYTE( "16504", 0x000001, 0x080000, 0x7c61fcd8 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "16495", 0x000000, 0x200000, 0x6e5f26be, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16497", 0x000002, 0x200000, 0x42ab5859, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16499", 0x000004, 0x200000, 0xa290ea36, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16501", 0x000006, 0x200000, 0xf1566620, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16496", 0x800000, 0x200000, 0xd9d27247, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16498", 0x800002, 0x200000, 0xc022a991, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16500", 0x800004, 0x200000, 0x452c0be3, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16502", 0x800006, 0x200000, 0xffc3147e, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mp16506.1", 0x000000, 0x200000, 0xe779f5ed )
	ROM_LOAD("mp16507.2", 0x200000, 0x200000, 0x31e068d3 )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( titlef )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "tf-15386.rom", 0x000000, 0x40000, 0x7ceaf15d )
	ROM_RELOAD(                      0x080000, 0x040000 )
	ROM_LOAD32_WORD( "tf-15387.rom", 0x000002, 0x40000, 0xaaf3cb03 )
	ROM_RELOAD(                      0x080002, 0x040000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("tf-15384.rom", 0x00000, 0x20000, 0x0f7d208d )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "tf-15381.rom", 0x000000, 0x200000, 0x162cc4d6 )
	ROM_LOAD16_BYTE( "tf-15382.rom", 0x000001, 0x200000, 0xfd03a130 )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "tf-15379.rom", 0x000000, 0x200000, 0xe5c74b11, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15375.rom", 0x000002, 0x200000, 0x046a9b50, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15371.rom", 0x000004, 0x200000, 0x999046c6, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15373.rom", 0x000006, 0x200000, 0x9b3294d9, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15380.rom", 0x800000, 0x200000, 0x6ea0e58d, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15376.rom", 0x800002, 0x200000, 0xde3e05c5, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15372.rom", 0x800004, 0x200000, 0xc187c36a, ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "tf-15374.rom", 0x800006, 0x200000, 0xe026aab0, ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("tf-15385.rom", 0x000000, 0x200000, 0x5a9b0aa0 )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

// boot, and are playable, some gfx problems */
GAMEX( 1992, orunners,     0, multi32, orunners, orunners, ROT0, "Sega", "Outrunners (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, harddunk,     0, multi32, harddunk, harddunk, ROT0, "Sega", "Hard Dunk (Japan)", GAME_IMPERFECT_GRAPHICS )

// doesn't boot (needs v70 or something else?)
GAMEX( 199?, titlef,       0, multi32, titlef,   titlef,   ROT0, "Sega", "Title Fight", GAME_NOT_WORKING )


