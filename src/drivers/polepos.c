/***************************************************************************
  Pole Position memory map (preliminary)

Z80
----------------------------------------
0000-2fff (R) ROM
3000-37ff (R/W) Battery Backup RAM
4000-43ff (R/W) Motion Object memory
    (4380-43ff Vertical and Horizontal position)
4400-47ff (R/W) Motion Object memory
    (4780-47ff Vertical and Horizontal position)
4800-4bff (R/W) Road Memory
    (4800-49ff Character)
    (4b80-4bff Horizontal Scroll)
4c00-57ff (R/W) Alphanumeric Memory
    (4c00-4fff) Alphanumeric
    (5000-53ff) View character
8000-83ff (R/W) Sound Memory
    (83c0-83ff Sound)
9000-90ff (R/W) 4 bit CPU data
9100-9100 (R/W) 4 bit CPU controller
a000-a000 (R/W) Input/Output
          on WRITE: IRQ Enable ( 1 = enable, 0 = disable )
          on READ: bit0 = Not Used, bit1 = 128V, bit2 = Power-Line Sense, bit3 = ADC End Flag
a001-a001 (W) 4 bit CPU Enable
a002-a002 (W) Sound enable
a003-a003 (W) ADC Input Select
a004-a004 (W) CPU 1 Enable
a005-a005 (W) CPU 2 Enable
a006-a006 (W) Start Switch
a007-a007 (W) Color Enable
a100-a100 (W) Watchdog reset
a200-a200 (W) Car Sound ( Lower Nibble )
a300-a300 (W) Car Sound ( Upper Nibble )

Z8002 #1 & #2 (they share the ram)
----------------------------------------
0000-3fff ROM
6000-6003 NMI-Enable
	(6000-6001 CPU1 NMI enable)
	(6002-6003 CPU2 NMI enable)
8000-8fff Motion Object Memory
	(8700-87ff Horizontal and Vertical position)
	(8f00-8fff Character, Color, Vertical size, Horizontal size)
9000-97ff Road Memory
	(9000-93ff Character)
	(9700-97ff Horizontal scroll)
9800-9fff Alphanumeric Memory (video RAM #1)
a000-afff View character memory (I think it refers to 'View' as the background)
c000-c000 View horizontal position
c100-c100 Road vertical position

***************************************************************************/

#include "driver.h"
#include "cpu/z8000/z8000.h"
#include "vidhrdw/generic.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) { fprintf x; fclose(errorlog); errorlog = fopen("error.log", "a"); }
#else
#define LOG(x)
#endif

#ifdef  LSB_FIRST
#define BYTE_XOR(a) (a)
#else
#define BYTE_XOR(a) ((a)^1)
#endif

/* from vidhrdw */
extern int polepos_vh_start(void);
extern void polepos_vh_stop(void);
extern void polepos_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void polepos_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* from machine */
void polepos_mcu_enable_w( int offs, int data );
int polepos_mcu_control_r( int offs );
void polepos_mcu_control_w( int offs, int data );
int polepos_mcu_data_r( int offs );
void polepos_mcu_data_w( int offs, int data );
void polepos_init_machine( void );


static int z80_irq_enabled = 0, cpu1_nvi_enabled = 0, cpu2_nvi_enabled = 0;
static int z80_128v = 0, z80_ADC_input = 0;

static void z80_irq_enable_w( int offs, int data ) {
	z80_irq_enabled = data & 1;
}

static void cpu1_nvi_enable_w( int offs, int data ) {
	if ( offs == 0 ) {
		LOG((errorlog,"Z8K#%d cpu1_nvi_enable_w $%02x\n", cpu_getactivecpu(), data));
		cpu1_nvi_enabled = data;
	}
}

static void cpu2_nvi_enable_w( int offs, int data ) {
	if ( offs == 0 ) {
		LOG((errorlog,"Z8K#%d cpu2_nvi_enable_w $%02x\n", cpu_getactivecpu(), data));
		cpu2_nvi_enabled = data;
	}
}

static int z80_interrupt( void ) {
	static int cnt = 0;

	if ( cnt == 1 )
		z80_128v = 1;
	else
		z80_128v = 0;

	if ( ++cnt == 4 )
		cnt = 0;

	if ( z80_irq_enabled )
		return interrupt();

	return ignore_interrupt();
}

static int cpu1_interrupt( void ) {
	if ( cpu1_nvi_enabled )
		cpu_set_irq_line(1,0,PULSE_LINE);

	return ignore_interrupt();
}

static int cpu2_interrupt( void ) {
	if ( cpu2_nvi_enabled )
		cpu_set_irq_line(2,0,PULSE_LINE);

	return ignore_interrupt();
}

static void z80_cpu_enable_w( int offs, int data ) {
    if ( data & 1 )
		cpu_halt( offs + 1, 1 );
	else {
        cpu_halt( offs + 1, 0 );
		cpu_reset( offs + 1 );
	}
}

/* ADC selection and read */
static void z80_ADC_select_w( int offs, int data ) {
	z80_ADC_input = data;
}

static int z80_ADC_r( int offs ) {
	int ret = 0;

	switch( z80_ADC_input ) {
		case 0x00:
			ret = readinputport( 3 );
			break;

		case 0x01:
			ret = readinputport( 4 );
			break;

		default:
			if ( errorlog )
				fprintf( errorlog, "Unknown ADC Input select (%02x)!\n", z80_ADC_input );
		break;
	}

	return ret;
}

/* IO */
static int z80_IO_r( int offs ) {
	int ret = 0xff;

	if ( z80_128v )
		ret ^= 0x02;

	ret ^= 0x08; /* ADC End Flag */

	return ret;
}

/*********************************************************************
 * Motion Object memory
 *********************************************************************/
unsigned char *polepos_motion_object_memory;

static int motion_r( int offs ) {
    return polepos_motion_object_memory[ offs ];
}

static void motion_w( int offs, int data ) {
	polepos_motion_object_memory[ offs ] = data;
}

/* WARNING: May have endian issue */
static int z80_motion_r( int offs ) {
	return polepos_motion_object_memory[ BYTE_XOR(offs << 1) ];
}

/* WARNING: May have endian issue */
static void z80_motion_w( int offs, int data ) {
	polepos_motion_object_memory[ BYTE_XOR(offs << 1) ] = data;
}

/*********************************************************************
 * Road memory
 *********************************************************************/
unsigned char *polepos_road_memory;

static int road_r( int offs ) {
	return polepos_road_memory[ offs ];
}

extern void polepos_road_draw( int offs );
static void road_w( int offs, int data ) {
	if ( polepos_road_memory[ offs ] != data ) {
		polepos_road_memory[ offs ] = data;
		polepos_road_draw( offs );
    }
}

/* WARNING: May have endian issue */
static int z80_road_r( int offs ) {
	return road_r( BYTE_XOR(offs << 1) );
}

/* WARNING: May have endian issue */
static void z80_road_w( int offs, int data ) {
	road_w( BYTE_XOR(offs << 1), data );
}

/*********************************************************************
 * Background Memory
 *********************************************************************/
unsigned char *polepos_back_memory;

static int back_r( int offs ) {
	return polepos_back_memory[ offs ];
}

extern void polepos_back_draw( int offs );
static void back_w( int offs, int data ) {
	if ( polepos_back_memory[ offs ] != data ) {
		polepos_back_memory[ offs ] = data;
		polepos_back_draw( offs );
	}
}

/* WARNING: May have endian issue */
static int z80_back_r( int offs ) {
	return back_r( BYTE_XOR(offs << 1) );
}

/* WARNING: May have endian issue */
static void z80_back_w( int offs, int data ) {
	back_w( BYTE_XOR(offs << 1), data );
}

int polepos_back_hscroll;
static void back_hscroll_w( int offs, int data ) {
    polepos_back_hscroll = data;
}
/*********************************************************************
 * Special Videoram handlers
 *********************************************************************/
/* WARNING: May have endian issue */
static int z80_videoram_r( int offs ) {
	return videoram_r( BYTE_XOR(offs << 1) );
}

/* WARNING: May have endian issue */
static void z80_videoram_w( int offs, int data ) {
	videoram_w( BYTE_XOR(offs << 1), data );
}

static struct MemoryReadAddress z80_readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },				/* ROM */
	{ 0x3000, 0x37ff, MRA_RAM },				/* Battery Backup */
	{ 0x4000, 0x47ff, z80_motion_r },			/* Motion Object */
	{ 0x4800, 0x4bff, z80_road_r },				/* Road Memory */
	{ 0x4c00, 0x4fff, z80_videoram_r },			/* Alphanumeric (char ram) */
	{ 0x5000, 0x53ff, z80_back_r }, 			/* Background Memory */
	{ 0x8000, 0x83ff, MRA_RAM }, 				/* Sound Memory */
	{ 0x9000, 0x90ff, polepos_mcu_data_r },		/* 4 bit CPU data */
	{ 0x9100, 0x9100, polepos_mcu_control_r },	/* 4 bit CPU control */
	{ 0xa000, 0xa000, z80_IO_r }, 				/* IO */
    {-1}                       					/* end of table */
};

static struct MemoryWriteAddress z80_writemem[] =
{
	{ 0x0000, 0x2fff, MWA_ROM }, 						/* ROM */
	{ 0x3000, 0x37ff, MWA_RAM }, 						/* Battery Backup */
	{ 0x4000, 0x47ff, z80_motion_w }, 					/* Motion Object */
	{ 0x4800, 0x4bff, z80_road_w }, 					/* Road Memory */
	{ 0x4c00, 0x4fff, z80_videoram_w }, 				/* Alphanumeric (char ram) */
	{ 0x5000, 0x53ff, z80_back_w }, 					/* Background Memory */
	{ 0x8000, 0x83ff, MWA_RAM }, 						/* Sound Memory */
	{ 0x9000, 0x90ff, polepos_mcu_data_w },				/* 4 bit CPU data */
	{ 0x9100, 0x9100, polepos_mcu_control_w }, 			/* 4 bit CPU control */
	{ 0xa000, 0xa000, z80_irq_enable_w }, 				/* NMI enable */
	{ 0xa001, 0xa001, polepos_mcu_enable_w },			/* 4 bit CPU enable */
	{ 0xa002, 0xa002, MWA_NOP }, 						/* Sound Enable */
	{ 0xa003, 0xa003, z80_ADC_select_w },				/* ADC Input select */
	{ 0xa004, 0xa005, z80_cpu_enable_w }, 				/* CPU 1/2 enable */
	{ 0xa006, 0xa006, osd_led_w }, 						/* Start Switch */
	{ 0xa007, 0xa007, MWA_NOP }, 						/* Color Enable */
	{ 0xa100, 0xa100, watchdog_reset_w }, 				/* Watchdog */
	{ 0xa200, 0xa200, MWA_NOP }, 						/* Car Sound ( Lower Nibble ) */
	{ 0xa300, 0xa300, MWA_NOP }, 						/* Car Sound ( Upper Nibble ) */
    {-1}                       							/* end of table */
};

static struct IOReadPort z80_readport[] =
{
	{ 0x00, 0x00, z80_ADC_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort z80_writeport[] =
{
	{ 0x00, 0x00, IOWP_NOP }, /* ??? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress z8k_1_readmem[] =
{
    { 0x0000, 0x3fff, MRA_ROM },    /* ROM */
    { 0x8000, 0x8fff, motion_r },   /* Motion Object */
    { 0x9000, 0x97ff, road_r },     /* Road Memory */
    { 0x9800, 0x9fff, videoram_r }, /* Alphanumeric (char ram) */
    { 0xa000, 0xafff, back_r },     /* Background memory */
    {-1}                            /* end of table */
};

static struct MemoryWriteAddress z8k_1_writemem[] =
{
    { 0x0000, 0x3fff, MWA_ROM },                                /* ROM */
    { 0x6000, 0x6001, cpu1_nvi_enable_w },                      /* NVI enable */
    { 0x8000, 0x8fff, motion_w, &polepos_motion_object_memory },/* Motion Object */
    { 0x9000, 0x97ff, road_w, &polepos_road_memory },           /* Road Memory */
    { 0x9800, 0x9fff, videoram_w, &videoram, &videoram_size },  /* Alphanumeric (char ram) */
    { 0xa000, 0xafff, back_w, &polepos_back_memory },           /* Background memory */
    { 0xc000, 0xc001, back_hscroll_w },                         /* Background horz scroll position */
    {-1}                                                        /* end of table */
};

static struct MemoryReadAddress z8k_2_readmem[] =
{
    { 0x0000, 0x3fff, MRA_ROM },    /* ROM */
    { 0x8000, 0x8fff, motion_r },   /* Motion Object */
    { 0x9000, 0x97ff, road_r },     /* Road Memory */
    { 0x9800, 0x9fff, videoram_r }, /* Alphanumeric (char ram) */
    { 0xa000, 0xafff, back_r },     /* Background memory */
    {-1}                            /* end of table */
};

static struct MemoryWriteAddress z8k_2_writemem[] =
{
    { 0x0000, 0x3fff, MWA_ROM },            /* ROM */
    { 0x6002, 0x6003, cpu2_nvi_enable_w },  /* NVI enable */
    { 0x8000, 0x8fff, motion_w },           /* Motion Object */
    { 0x9000, 0x97ff, road_w },             /* Road Memory */
    { 0x9800, 0x9fff, videoram_w },         /* Alphanumeric (char ram) */
    { 0xa000, 0xafff, back_w },             /* Background memory */
    { 0xc000, 0xc001, back_hscroll_w },                         /* Background horz scroll position */
    {-1}                                    /* end of table */
};

INPUT_PORTS_START(input_ports)
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Select Button */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_TOGGLE ) /* Gear */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 ) /* Start button */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Unused, according to schematics */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) /* Coin 1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 ) /* Coin 2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) /* Service button */
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Nr. of Laps" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x06, 0x06, "Game Time" )
    PORT_DIPSETTING(    0x06, "120 secs." )
	PORT_DIPSETTING(    0x04, "100 secs." )
	PORT_DIPSETTING(    0x02, "110 secs." )
	PORT_DIPSETTING(    0x00, "90 secs." )
    PORT_DIPNAME( 0x18, 0x00, "Coinage (Coin 2)" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_3C ) )
    PORT_DIPNAME( 0xe0, 0x00, "Coinage (Coin 1)" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x60, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0xa0, DEF_STR( 3C_2C ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0xe0, DEF_STR( Free_Play ) )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0xe0, 0x40, "Practice Rank" )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x80, "B" )
	PORT_DIPSETTING(    0x40, "C" )
	PORT_DIPSETTING(    0xc0, "D" )
	PORT_DIPSETTING(    0x20, "E" )
	PORT_DIPSETTING(    0xa0, "F" )
	PORT_DIPSETTING(    0x60, "G" )
    PORT_DIPSETTING(    0xe0, "H" )
    PORT_DIPNAME( 0x1c, 0x08, "Extended Rank" )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x10, "B" )
	PORT_DIPSETTING(    0x08, "C" )
	PORT_DIPSETTING(    0x18, "D" )
	PORT_DIPSETTING(    0x04, "E" )
	PORT_DIPSETTING(    0x14, "F" )
	PORT_DIPSETTING(    0x0c, "G" )
	PORT_DIPSETTING(    0x1c, "H" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ))

    PORT_START /* IN1 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 10, 0, 0, 0)

	PORT_START /* IN2 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20, 10, 0, 0, 0)
INPUT_PORTS_END

static struct GfxLayout charlayout_2bpp =
{
        8,8,    /* 8*8 characters */
		512,	/* 512 characters */
        2,      /* 2 bits per pixel */
        { 0, 4 }, /* the two bitplanes are packed */
        { 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        8*8*2   /* every char takes 16 consecutive bytes */
};

static struct GfxLayout charlayout_4bpp =
{
        8,8,    /* 8*8 characters */
		256,	/* 256 characters */
		4,		/* 4 bits per pixel */
		{ 0, 4, 8*8*2+0, 8*8*2+4 }, /* the two bitplanes are packed */
        { 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
		8*8*2*2 /* every char takes 32 consecutive bytes */
};

static struct GfxLayout bigspritelayout =
{
        32, 32, /* 32*32 sprites */
		96, 	/* 96 sprites */
		4,		/* 4 bits per pixel */
		{ 0, 4, 0x6000*8+0, 0x6000*8+4}, /* each two of the bitplanes are packed */
        {  0,  1,  2,  3,  8,  9, 10, 11,
          16, 17, 18, 19, 24, 25, 26, 27,
          32, 33, 34, 35, 40, 41, 42, 43,
          48, 49, 50, 51, 56, 57, 58, 59},
        {  0*64,  1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
           8*64,  9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64,
          16*64, 17*64, 18*64, 19*64, 20*64, 21*64, 22*64, 23*64,
          24*64, 25*64, 26*64, 27*64, 28*64, 29*64, 30*64, 31*64 },
		32*32*2  /* each sprite takes 256 consecutive bytes */
};

static struct GfxLayout smallspritelayout =
{
        16, 16, /* 16*16 sprites */
		128,	/* 128 sprites */
		4,		/* 4 bits per pixel */
		{ 0, 4, 0x2000*8+0, 0x2000*8+0 }, /* each two of the bitplanes are packed */
        {  0,  1,  2,  3,  8,  9, 10, 11,
          16, 17, 18, 19, 24, 25, 26, 27 },
        { 0*32, 1*32,  2*32 , 3*32 , 4*32,  5*32,  6*32,  7*32,
          8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
        16*16*2  /* each sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout_2bpp, 		0,	64 },
	{ 1, 0x02000, &charlayout_4bpp, 	 64*4,	64 },
	{ 1, 0x04000, &bigspritelayout,    2*64*4,	64 },
	{ 1, 0x10000, &smallspritelayout,  3*64*4,	64 },
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,		/* 2 MHz? */
			0,
			z80_readmem, z80_writemem, z80_readport, z80_writeport,
			z80_interrupt, 4,
			0, 0
        },
		{
			CPU_Z8000,
			4000000,		/* 4 MHz? */
			2,
			z8k_1_readmem, z8k_1_writemem, 0, 0,
			cpu1_interrupt, 1,
			0, 0
		},
		{
			CPU_Z8000,
			4000000,		/* 4 MHz? */
			3,
			z8k_2_readmem, z8k_2_writemem, 0, 0,
			cpu2_interrupt, 1,
			0, 0
		}
    },
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	 /* frames per second, vblank duration */
	10,	/* some interleaving */
	polepos_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,4*64*4,
	polepos_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	polepos_vh_start,
	polepos_vh_stop,
	polepos_vh_screenrefresh,

/* sound hardware */
	0, 0, 0, 0,
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(polepos_rom)
	ROM_REGION(0x10000) 			/* #0 64K ROM/RAM Z80 */
	ROM_LOAD	 ( "014-105.rom",   0x00000, 0x2000, 0xc918c043 )
	ROM_LOAD	 ( "014-116.rom",   0x02000, 0x1000, 0x7174bcb7 )

	ROM_REGION_DISPOSE(0x18000) 	/* #1 graphics */
	/* 2bpp 8x8 char, missing 2nd half? */
	ROM_LOAD	 ( "014-132.rom",   0x00000, 0x1000, 0xa949aa85 )
	/* 4 bpp 8x8 char?, missing 2nd half? */
	ROM_LOAD	 ( "014-133.rom",   0x02000, 0x1000, 0x3f0eb551 )

	/* 4bpp 32x32 sprites */
	ROM_LOAD	 ( "014-150.rom",   0x04000, 0x2000, 0x2e134b46 )   /* planes 0+1 */
	ROM_LOAD	 ( "014-152.rom",   0x06000, 0x2000, 0xa7e3a1c6 )
	ROM_LOAD	 ( "014-154.rom",   0x08000, 0x2000, 0x8992d381 )
	ROM_LOAD	 ( "014-151.rom",   0x0a000, 0x2000, 0x6f9997d2 )   /* planes 2+3 */
	ROM_LOAD	 ( "014-153.rom",   0x0c000, 0x2000, 0x6c5c6e68 )
	ROM_LOAD	 ( "014-155.rom",   0x0e000, 0x2000, 0x111896ad )

	/* 4bpp 16x16 sprites */
	ROM_LOAD	 ( "014-156.rom",   0x10000, 0x2000, 0xe7a09c93 )   /* planes 0+1 ? */
	ROM_LOAD	 ( "014-157.rom",   0x14000, 0x2000, 0xdee7d687 )   /* planes 2+3 ? */

	ROM_REGION(0x10000) 			/* #2 64K ROM/RAM for Z8002 #1 */
	ROM_LOAD_ODD ( "014-101.rom",   0x00000, 0x2000, 0x8c2cf172 )
	ROM_LOAD_EVEN( "014-102.rom",   0x00000, 0x2000, 0x51018857 )

	ROM_REGION(0x10000) 			/* #3 64K ROM/RAM for Z8002 #2 */
	ROM_LOAD_ODD ( "014-203.rom",   0x00000, 0x2000, 0xeedea6e7 )
	ROM_LOAD_EVEN( "014-204.rom",   0x00000, 0x2000, 0xc52c98ed )

	ROM_REGION(0x10000) 			/* #4 PROM data */
	/* 136014-137, 136014-138, 136014-139 red/green/blue */
	ROM_LOAD	 ( "014-137.bpr",   0x0000, 0x0100, 0xf07ff2ad )
	ROM_LOAD	 ( "014-138.bpr",   0x0100, 0x0100, 0xadbde7d7 )
	ROM_LOAD	 ( "014-139.bpr",   0x0200, 0x0100, 0xddac786a )

	/* 136014-140 alpha character attribute to color mapping ? */
	ROM_LOAD	 ( "014-140.bpr",   0x0300, 0x0100, 0x1e8d0491 )
	/* 136014-141 background attribute to color mapping ? */
	ROM_LOAD     ( "014-141.bpr",   0x0400, 0x0100, 0x0e4fe8a0 )

	ROM_LOAD	 ( "014-117.bpr",   0x0500, 0x0100, 0x2401c817 )
	ROM_LOAD	 ( "014-118.bpr",   0x0600, 0x0100, 0x8568decc )

	/* 136014-142, 136014-143, 136014-144 Vertical position modifiers */
	ROM_LOAD     ( "014-142.bpr",   0x0700, 0x0100, 0x2d502464 )
	ROM_LOAD	 ( "014-143.bpr",   0x0800, 0x0100, 0x027aa62c )
	ROM_LOAD	 ( "014-144.bpr",   0x0900, 0x0100, 0x1f8d0df3 )

	ROM_LOAD	 ( "014-186.bpr",   0x0a00, 0x0100, 0x16d69c31 )
	ROM_LOAD	 ( "014-187.bpr",   0x0b00, 0x0100, 0x07340311 )
	ROM_LOAD	 ( "014-188.bpr",   0x0c00, 0x0100, 0x1efc84d7 )

	ROM_LOAD	 ( "014-145.bpr",   0x1000, 0x0400, 0x7afc7cfc )
	ROM_LOAD	 ( "014-146.bpr",   0x1400, 0x0400, 0xca4ba741 )

	/* 136014-127, 136014-128 ? */
	ROM_LOAD	 ( "014-110.rom",   0x2000, 0x2000, 0xb5ad4d5f )
	ROM_LOAD	 ( "014-111.rom",   0x4000, 0x2000, 0x8fdd2f6f )

	ROM_LOAD	 ( "014-106.rom",   0x6000, 0x2000, 0x5b4cf05e )
	ROM_LOAD	 ( "014-231.rom",   0x8000, 0x1000, 0xa61bff15 )

	/* roadway memory 136014-127, 136014-128, 136014-134 */
	ROM_LOAD	 ( "014-158.rom",   0x9000, 0x2000, 0xee6b3315 )
	ROM_LOAD	 ( "014-159.rom",   0xb000, 0x2000, 0x6d1e7042 )
	ROM_LOAD	 ( "014-134.rom",   0xd000, 0x1000, 0x4e97f101 )
ROM_END

ROM_START(poleposa_rom)
	ROM_REGION(0x10000) 			/* #0 64K ROM/RAM Z80 */
	ROM_LOAD	 ( "7hcpu.rom",     0x00000, 0x2000, 0xf85212c4 )
	ROM_LOAD	 ( "7fcpu.rom",     0x02000, 0x1000, 0xa9d4c380 )

	ROM_REGION_DISPOSE(0x18000) 	/* #1 graphics */
	/* 2bpp 8x8 char */
	ROM_LOAD	 ( "7nvid.rom",     0x00000, 0x2000, 0xfbe5e72f )
	/* 4bpp 8x8 char? */
	ROM_LOAD	 ( "6nvid.rom",     0x02000, 0x2000, 0xec3ec6e6 )

	/* 4bpp 32x32 sprites */
	ROM_LOAD	 ( "13jvid.rom",    0x04000, 0x2000, 0x2e134b46 )   /* planes 0+1 */
	ROM_LOAD	 ( "13kvid.rom",    0x06000, 0x2000, 0x2b0517bd )
	ROM_LOAD	 ( "13lvid.rom",    0x08000, 0x2000, 0x9ab89d7f )
	ROM_LOAD	 ( "12jvid.rom",    0x0a000, 0x2000, 0x6f9997d2 )   /* planes 2+3 */
	ROM_LOAD	 ( "12kvid.rom",    0x0c000, 0x2000, 0xfa131a9b )
	ROM_LOAD	 ( "12lvid.rom",    0x0e000, 0x2000, 0x662ff24b )

	/* 4bpp 16x16 sprites */
	ROM_LOAD	 ( "13nvid.rom",    0x10000, 0x2000, 0x455d79a0 )   /* planes 0+1 */
	ROM_LOAD	 ( "12nvid.rom",    0x14000, 0x2000, 0x78372b81 )   /* planes 2+3 */

	ROM_REGION(0x10000) 			/* #2 64K ROM/RAM for Z8002 #1 */
	ROM_LOAD_ODD ( "014-101.rom",   0x00000, 0x2000, 0x8c2cf172 )
	ROM_LOAD_EVEN( "014-102.rom",   0x00000, 0x2000, 0x51018857 )

	ROM_REGION(0x10000) 			/* #3 64K ROM/RAM for Z8002 #2 */
	ROM_LOAD_ODD ( "014-203.rom",   0x00000, 0x2000, 0xeedea6e7 )
	ROM_LOAD_EVEN( "014-204.rom",   0x00000, 0x2000, 0xc52c98ed )

	ROM_REGION(0x10000) 			/* #4 PROM data */
	/* 136014-137, 136014-138, 136014-139 red/green/blue */
	ROM_LOAD	 ( "014-137.bpr",   0x0000, 0x0100, 0xf07ff2ad )
	ROM_LOAD	 ( "014-138.bpr",   0x0100, 0x0100, 0xadbde7d7 )
	ROM_LOAD	 ( "014-139.bpr",   0x0200, 0x0100, 0xddac786a )

	/* 136014-140 alpha character attribute to color mapping ? */
	ROM_LOAD	 ( "014-140.bpr",   0x0300, 0x0100, 0x1e8d0491 )
	/* 136014-141 background attribute to color mapping ? */
	ROM_LOAD     ( "014-141.bpr",   0x0400, 0x0100, 0x0e4fe8a0 )

	ROM_LOAD	 ( "014-117.bpr",   0x0500, 0x0100, 0x2401c817 )
	ROM_LOAD	 ( "014-118.bpr",   0x0600, 0x0100, 0x8568decc )
	/* 136014-142, 136014-143, 136014-144 Vertical position modifiers */
	ROM_LOAD     ( "014-142.bpr",   0x0700, 0x0100, 0x2d502464 )
	ROM_LOAD	 ( "014-143.bpr",   0x0800, 0x0100, 0x027aa62c )
	ROM_LOAD	 ( "014-144.bpr",   0x0900, 0x0100, 0x1f8d0df3 )

	ROM_LOAD	 ( "014-186.bpr",   0x0a00, 0x0100, 0x16d69c31 )
	ROM_LOAD	 ( "014-187.bpr",   0x0b00, 0x0100, 0x07340311 )
	ROM_LOAD	 ( "014-188.bpr",   0x0c00, 0x0100, 0x1efc84d7 )

	ROM_LOAD	 ( "014-145.bpr",   0x1000, 0x0400, 0x7afc7cfc )
	ROM_LOAD	 ( "014-146.bpr",   0x1400, 0x0400, 0xca4ba741 )

	/* 136014-127, 136014-128 */
	ROM_LOAD	 ( "014-110.rom",   0x2000, 0x2000, 0xb5ad4d5f )
	ROM_LOAD	 ( "014-111.rom",   0x4000, 0x2000, 0x8fdd2f6f )

	ROM_LOAD	 ( "014-106.rom",   0x6000, 0x2000, 0x5b4cf05e )
	ROM_LOAD	 ( "014-231.rom",   0x8000, 0x1000, 0xa61bff15 )

	/* roadway memory 136014-127, 136014-128, 136014-134  */
	ROM_LOAD	 ( "2lvid.rom",     0x9000, 0x2000, 0xee6b3315 )
	ROM_LOAD	 ( "2mvid.rom",     0xb000, 0x2000, 0x6d1e7042 )
	ROM_LOAD	 ( "014-134.rom",   0xd000, 0x1000, 0x4e97f101 )
ROM_END

ROM_START(poleps2b_rom)
	ROM_REGION(0x10000) 			/* #0 64K ROM/RAM Z80 */
	ROM_LOAD	 ( "tr9b.bin",      0x00000, 0x2000, 0x94436b70 )
	ROM_LOAD	 ( "10.bin",        0x02000, 0x1000, 0x7174bcb7 )

	ROM_REGION_DISPOSE(0x14000) 	/* #1 graphics */
	/* 2bpp 8x8 char, missing 2nd half? */
	ROM_LOAD	 ( "tr28.bin",      0x00000, 0x1000, 0xb8217c96 )
	/* 4bpp 8x8 char?, missing 2nd half? */
	ROM_LOAD	 ( "tr29.bin",      0x02000, 0x1000, 0xc6e15c21 )

	/* 4bpp 32x32 sprites */
	ROM_LOAD	 ( "pp17.bin",      0x04000, 0x2000, 0x613ab0df )   /* planes 0+1 */
	ROM_LOAD	 ( "tr19.bin",      0x06000, 0x2000, 0xf8e7f551 )
	ROM_LOAD	 ( "tr21.bin",      0x08000, 0x2000, 0x17c798b0 )
	ROM_LOAD	 ( "pp18.bin",      0x0a000, 0x2000, 0x5fd933e3 )   /* planes 2+3 */
	ROM_LOAD	 ( "tr20.bin",      0x0c000, 0x2000, 0x7053e219 )
	ROM_LOAD	 ( "tr22.bin",      0x0e000, 0x2000, 0xf48917b2 )

	/* 4bpp 16x16 sprites */
	ROM_LOAD	 ( "trus25.bin",    0x10000, 0x2000, 0x9e1a9c3b )   /* planes 0+1 ? */
	ROM_LOAD	 ( "trus26.bin",    0x12000, 0x2000, 0x3b39a176 )   /* planes 2+3 ? */

	ROM_REGION(0x10000) 			/* #2 64K ROM/RAM for Z8002 #1 */
	ROM_LOAD_ODD ( "tr1b.bin",      0x00000, 0x2000, 0x127f0750 )
	ROM_LOAD_EVEN( "tr2b.bin",      0x00000, 0x2000, 0x6bd4ff6b )


	ROM_REGION(0x10000) 			/* #3 64K ROM/RAM for Z8002 #2 */
	ROM_LOAD_ODD ( "tr5b.bin",      0x00000, 0x2000, 0x4e5f7b9c )
	ROM_LOAD_EVEN( "tr6b.bin",      0x00000, 0x2000, 0x9d038ada )

	ROM_REGION(0x10000) 			/* #4 PROM data */
	/* 136014-137, 136014-138, 136014-139 red/green/blue */
	ROM_LOAD	 ( "014-137.bpr",   0x0000, 0x0100, 0xf07ff2ad )    /* 7.bin no good dump? */
	ROM_LOAD	 ( "8.bin",         0x0100, 0x0100, 0xadbde7d7 )
	ROM_LOAD	 ( "9.bin",         0x0200, 0x0100, 0xddac786a )

	/* 136014-140 alpha character attribute to color mapping ? */
	ROM_LOAD     ( "10p.bin",       0x0300, 0x0100, 0x5af3f710 )
	/* 136014-141 background attribute to color mapping ? */
	ROM_LOAD	 ( "014-141.bpr",   0x0400, 0x0100, 0x0e4fe8a0 )

	ROM_LOAD	 ( "4.bin",         0x0500, 0x0100, 0x0e742cb1 )
	ROM_LOAD	 ( "5.bin",         0x0600, 0x0100, 0x8568decc )
	/* 136014-142, 136014-143, 136014-144 Vertical position modifiers */
	ROM_LOAD     ( "15.bin",        0x0700, 0x0100, 0x2d502464 )
	ROM_LOAD	 ( "16.bin",        0x0800, 0x0100, 0x027aa62c )
	ROM_LOAD	 ( "17.bin",        0x0900, 0x0100, 0x1f8d0df3 )

	ROM_LOAD	 ( "014-186.bpr",   0x0a00, 0x0100, 0x16d69c31 )
	ROM_LOAD	 ( "014-187.bpr",   0x0b00, 0x0100, 0x07340311 )
	ROM_LOAD	 ( "014-188.bpr",   0x0c00, 0x0100, 0x1efc84d7 )


	ROM_LOAD	 ( "014-145.bpr",   0x1000, 0x0400, 0x7afc7cfc )
	ROM_LOAD	 ( "014-146.bpr",   0x1400, 0x0400, 0xca4ba741 )

	/* 136014-127, 136014-128 ? */
	ROM_LOAD	 ( "tr15.bin",      0x2000, 0x2000, 0xb5ad4d5f )
	ROM_LOAD	 ( "tr16.bin",      0x4000, 0x2000, 0x8fdd2f6f )

	ROM_LOAD	 ( "tr11b.bin",     0x6000, 0x2000, 0x5b4cf05e )
	ROM_LOAD	 ( "tr27.bin",      0x8000, 0x1000, 0xa61bff15 )

	/* roadway memory 136014-127, 136014-128, 136014-134 */
	ROM_LOAD	 ( "tr30.bin",      0x9000, 0x2000, 0xee6b3315 )
	ROM_LOAD	 ( "tr31.bin",      0xb000, 0x2000, 0x6d1e7042 )
	ROM_LOAD	 ( "tr32.bin",      0xd000, 0x1000, 0x4e97f101 )
ROM_END

static void polepos_init(void)
{
	/* halt the two Z8002 cpus */
	cpu_halt(1, 0);
	cpu_halt(2, 0);
}

struct GameDriver polepos_driver =
{
	__FILE__,
	0,
	"polepos",
	"Pole Position (set 1)",
	"1982",
	"Atari",
	"Ernesto Corvi\nJuergen Buchmueller\nAlex Pasadyn\n",
	0,
	&machine_driver,
	polepos_init,		/* driver init */

	polepos_rom,
	0,		/* rom decode */
	0,
	0,
	0,      /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(4),	/* color_prom */
    0,
	0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver poleposa_driver =
{
	__FILE__,
	&polepos_driver,
	"poleposa",
	"Pole Position (set 2)",
	"1982",
	"Atari",
	"Ernesto Corvi\nJuergen Buchmueller\nAlex Pasadyn\n",
	0,
	&machine_driver,
	polepos_init,		/* driver init */

	poleposa_rom,
	0,		/* rom decode */
	0,
	0,
	0,      /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(4),	/* color_prom */
    0,
	0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver poleps2b_driver =
{
	__FILE__,
	&polepos_driver,
	"poleps2b",
	"Pole Position II (bootleg)",
	"1982",
	"Atari",
	"Ernesto Corvi\nJuergen Buchmueller\nAlex Pasadyn\n",
	0,
	&machine_driver,
	polepos_init,		/* driver init */

	poleps2b_rom,
	0,		/* rom decode */
	0,
	0,
	0,      /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(4),	/* color_prom */
	0,
	0,
	ORIENTATION_DEFAULT,

	0,0
};

