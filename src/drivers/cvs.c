/***************************************************************************

Century CVS System

MAIN BOARD:

             FLAG LOW               |   FLAG HIGH
------------------------------------+-----------------------------------
1C00-1FFF    SYSTEM RAM             |   SYSTEM RAM
                                    |
                                    |
1800-1BFF    ATTRIBUTE RAM 32X28    |   SCREEN RAM 32X28
1700         2636 1                 |   CHARACTER RAM 1 256BYTES OF 1K*
1600         2636 2                 |   CHARACTER RAM 2 256BYTES OF 1K*
1500         2636 3                 |   CHARACTER RAM 3 256BYTES OF 1K*
1400         BULLET RAM             |   PALETTE RAM 16BYTES
                                    |
                                    |
0000-13FF    PROGRAM ROM'S          |   PROGRAM ROM'S

* Note the upper two address lines are latched using a IO read. The IO map only has
  space for 128 character bit maps

The CPU CANNOT read the character PROM'S
        ------

                         CVS IO MAP
                         ----------
ADR14 ADR13 | READ                                      | WRITE
------------+-------------------------------------------+-----------------------------
  1     0   | COLLISION RESET                           | D0 = STARS ON
            |                                           | D1 = SHADE BRIGHTER TO RIGHT
            |                                           | D2 = SCREEN ROTATE
 read/write |                                           | D3 = SHADE BRIGHTER TO LEFT
   Data     |                                           | D4 = LAMP 1 (CN1 1)
            |                                           | D5 = LAMP 2 (CN1 2)
            |                                           | D6 = SHADE BRIGHTER TO BOTTOM
            |                                           | D7 = SHADE BRIGHTER TO TOP
------------+-------------------------------------------+------------------------------
  X     1   | A0-2: 0 STROBE CN2 1                      | VERTICAL SCROLL OFFSET
            |       1 STROBE CN2 11                     |
            |       2 STROBE CN2 2                      |
            |       3 STROBE CN2 3                      |
            |       4 STROBE CN2 4                      |
            |       5 STROBE CN2 12                     |
            |       6 STROBE DIP SW3                    |
            |       7 STROBE DIP SW2                    |
            |                                           |
 read/write | A4-5: CHARACTER PROM/RAM SELECTION MODE   |
  extended  | There are 256 characters in total. The    |
            | higher ones can be user defined in RAM.   |
            | The split between PROM and RAM characters |
            | is variable.                              |
            |                ROM              RAM       |
            | A4 A5 MODE  CHARACTERS       CHARACTERS   |
            | 0  0   0    224 (0-223)      32  (224-255)|
            | 0  1   1    192 (0-191)      64  (192-255)|
            | 1  0   2    256 (0-255)      0            |
            | 1  1   3    128 (0-127)      128 (128-255)|
            |                                           |
            |                                           |
            | A6-7: SELECT CHARACTER RAM's              |
            |       UPPER ADDRESS BITS A8-9             |
            |       (see memory map)                    |
------------+-------------------------------------------+-------------------------------
  0     0   | COLLISION DATA BYTE:                      | SOUND CONTROL PORT
            | D0 = OBJECT 1 AND 2                       |
            | D1 = OBJECT 2 AND 3                       |
 read/write | D2 = OBJECT 1 AND 3                       |
  control   | D3 = ANY OBJECT AND BULLET                |
            | D4 = OBJECT 1 AND CP1 OR CP2              |
            | D5 = OBJECT 2 AND CP1 OR CP2              |
            | D6 = OBJECT 3 AND CP1 OR CP2              |
            | D7 = BULLET AND CP1 OR CP2                |
------------+-------------------------------------------+-------------------------------

Driver by
	Mike Coates

Hardware Info
 Malcolm & Darren

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/s2650/s2650.h"
#include "sound/tms5110.h"

int  cvs_interrupt(void);
void cvs_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void cvs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  cvs_vh_start(void);
void cvs_vh_stop(void);
int  s2650_get_flag(void);

extern unsigned char *dirty_character;
extern unsigned char *character_1_ram;
extern unsigned char *character_2_ram;
extern unsigned char *character_3_ram;
extern unsigned char *bullet_ram;
extern unsigned char *s2636_1_ram;
extern unsigned char *s2636_2_ram;
extern unsigned char *s2636_3_ram;

WRITE_HANDLER( cvs_videoram_w );
WRITE_HANDLER( cvs_bullet_w );
WRITE_HANDLER( cvs_2636_1_w );
WRITE_HANDLER( cvs_2636_2_w );
WRITE_HANDLER( cvs_2636_3_w );
WRITE_HANDLER( cvs_scroll_w );
WRITE_HANDLER( cvs_video_fx_w );

READ_HANDLER( cvs_collision_r );
READ_HANDLER( cvs_collision_clear );
READ_HANDLER( cvs_videoram_r );
READ_HANDLER( cvs_bullet_r );
READ_HANDLER( cvs_2636_1_r );
READ_HANDLER( cvs_2636_2_r );
READ_HANDLER( cvs_2636_3_r );
READ_HANDLER( cvs_character_mode_r );

/***************************************************************************
	S2650 Memory Mirroring calls
***************************************************************************/

READ_HANDLER( cvs_mirror_r )
{
	return cpu_readmem16(0x1400+offset);
}

WRITE_HANDLER( cvs_mirror_w )
{
	cpu_writemem16(0x1400+offset,data);
}

/***************************************************************************
	Speech Calls
***************************************************************************/

static int speech_rom_address = 0;
static int speech_rom_bit = 0;

static void start_talking (void)
{
	tms5110_CTL_w(0,TMS5110_CMD_SPEAK);
	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);
}

static void reset_talking (void)
{
	tms5110_CTL_w(0,TMS5110_CMD_RESET);
	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	speech_rom_address = 0x0;
    speech_rom_bit     = 0x0;
}

WRITE_HANDLER( control_port_w )
{
	/* Controls both Speech and Effects */

	logerror("%4x : Sound Port = %2x\n",s2650_get_pc(),data);

    /* Sample CPU write - Causes interrupt if bit 7 set */

    soundlatch_w(0,data);
	if(data & 0x80) cpu_cause_interrupt(1,3);


    /* Speech CPU stuff */

    if((!tms5110_status_read()) && ((data & 0x40) == 0))
    {
   	    /* Speech Command */

    	if(data == 0x3f)
        {
            reset_talking();
        }
        else
        {
            speech_rom_address = ((data & 0x3f) * 0x80);
           	speech_rom_bit     = 0;

			logerror("%4x : Speech = %4x\n",s2650_get_pc(),speech_rom_address);

            start_talking();
        }
    }
}

int cvs_speech_rom_read_bit(void)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);
    int bit;

	bit = (ROM[speech_rom_address] >> speech_rom_bit ) & 1;

    speech_rom_bit++;
    if(speech_rom_bit == 8)
    {
		speech_rom_address++;
        speech_rom_bit = 0;
    }

	return bit;
}

WRITE_HANDLER( cvs_DAC2_w )
{
    /* 4 Bit DAC - 4 memory locations used */

	static int DAC_Value=0;

    DAC_Value &= (1 << ~(offset + 4));
    DAC_Value |= ((data & 0x80) >> 7) << (offset + 4);

	DAC_1_data_w(0,DAC_Value);
}

READ_HANDLER( CVS_393hz_Clock_r )
{
  	if(cpu_scalebyfcount(6) & 1) return 0x80;
    else return 0;
}

static struct TMS5110interface tms5110_interface =
{
	640000, /*640 kHz clock*/
	100,	/*100 % mixing level */
	0,		/*irq callback function*/
	cvs_speech_rom_read_bit	/*M0 callback function. Called whenever chip requests a single bit of data*/
};

static struct DACinterface dac_interface =
{
	2,
	{ 100 }
};

static MEMORY_READ_START( cvs_readmem )
	{ 0x0000, 0x13ff, MRA_ROM },
	{ 0x2000, 0x33ff, MRA_ROM },
	{ 0x4000, 0x53ff, MRA_ROM },
	{ 0x6000, 0x73ff, MRA_ROM },
    { 0x1400, 0x14ff, cvs_bullet_r },
    { 0x1500, 0x15ff, cvs_2636_3_r },
    { 0x1600, 0x16ff, cvs_2636_2_r },
    { 0x1700, 0x17ff, cvs_2636_1_r },
	{ 0x1800, 0x1bff, cvs_videoram_r },
    { 0x1c00, 0x1fff, MRA_RAM },
	{ 0x3400, 0x3fff, cvs_mirror_r },
	{ 0x5400, 0x5fff, cvs_mirror_r },
	{ 0x7400, 0x7fff, cvs_mirror_r },
MEMORY_END

static MEMORY_WRITE_START( cvs_writemem )
	{ 0x0000, 0x13ff, MWA_ROM },
	{ 0x2000, 0x33ff, MWA_ROM },
	{ 0x4000, 0x53ff, MWA_ROM },
	{ 0x6000, 0x73ff, MWA_ROM },
    { 0x1400, 0x14ff, cvs_bullet_w, &bullet_ram },
    { 0x1500, 0x15ff, cvs_2636_3_w, &s2636_3_ram },
    { 0x1600, 0x16ff, cvs_2636_2_w, &s2636_2_ram },
    { 0x1700, 0x17ff, cvs_2636_1_w, &s2636_1_ram },
	{ 0x1800, 0x1bff, cvs_videoram_w, &videoram, &videoram_size },
    { 0x1c00, 0x1fff, MWA_RAM },
	{ 0x3400, 0x3fff, cvs_mirror_w },
	{ 0x5400, 0x5fff, cvs_mirror_w },
	{ 0x7400, 0x7fff, cvs_mirror_w },

    /** Not real addresses, just memory blocks **/

    { 0x8000, 0x83ff, MWA_RAM, &character_1_ram },	/* same bitplane */
    { 0x8800, 0x8bff, MWA_RAM, &character_2_ram },	/* separation as */
    { 0x9000, 0x93ff, MWA_RAM, &character_3_ram },	/* rom character */
	{ 0x9400, 0x97ff, MWA_RAM, &colorram },
    { 0x9800, 0x98ff, MWA_RAM, &paletteram },
    { 0x9900, 0x99ff, MWA_RAM, &dirty_character },
MEMORY_END

static PORT_READ_START( cvs_readport )
	{ 0x000, 0x000, input_port_0_r },
    { 0x002, 0x002, input_port_1_r },
	{ 0x003, 0x003, input_port_2_r },
    { 0x004, 0x004, input_port_3_r },
	{ 0x006, 0x006, input_port_4_r },		// Dip 1
	{ 0x007, 0x007, input_port_5_r },		// Dip 2
    { 0x010, 0x0ff, cvs_character_mode_r },	// Programmable Character Settings
	{ S2650_DATA_PORT, S2650_DATA_PORT, cvs_collision_clear },
	{ S2650_CTRL_PORT, S2650_CTRL_PORT, cvs_collision_r },
    { S2650_SENSE_PORT, S2650_SENSE_PORT, input_port_6_r },
PORT_END

static PORT_WRITE_START( cvs_writeport )
	{ 0              , 0xff           , cvs_scroll_w },
	{ S2650_CTRL_PORT, S2650_CTRL_PORT, control_port_w },
	{ S2650_DATA_PORT, S2650_DATA_PORT, cvs_video_fx_w },
PORT_END

static MEMORY_READ_START( cvs_sound_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
    { 0x1000, 0x107f, MRA_RAM },
    { 0x1800, 0x1800, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( cvs_sound_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
    { 0x1000, 0x107f, MWA_RAM },
    { 0x1840, 0x1840, DAC_0_data_w },
    { 0x1880, 0x1883, cvs_DAC2_w },
    { 0x1884, 0x1887, MWA_NOP },		/* Not connected to anything */
MEMORY_END

static PORT_READ_START( cvs_sound_readport )
    { S2650_SENSE_PORT, S2650_SENSE_PORT, CVS_393hz_Clock_r },
PORT_END

INPUT_PORTS_START( cvs )

	PORT_START	/* Matrix 0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )				/* Confirmed */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 ) 		  	/* Confirmed */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )				/* Confirmed */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )				/* Confirmed */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )			/* Confirmed */
    PORT_BIT( 0xC0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dunno */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2  | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )			/* Confirmed */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )		/* Confirmed */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) 		/* Confirmed */
    PORT_BIT( 0xcc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dunno */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )				/* Duplicate? */
    PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dunno */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )		/* Confirmed */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )		/* Confirmed */
    PORT_BIT( 0xcf, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* SW BANK 3 */
	PORT_DIPNAME( 0x01, 0x00, "Colour" )
	PORT_DIPSETTING(    0x00, "option 1" )
	PORT_DIPSETTING(    0x01, "option 2" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
    PORT_DIPNAME( 0x0C, 0x00, "Bonus" )
    PORT_DIPSETTING(    0x00, "10k only" )
    PORT_DIPSETTING(    0x04, "20k only" )
    PORT_DIPSETTING(    0x08, "30k and every 40k" )
    PORT_DIPSETTING(    0x0C, "40k and every 80k" )
	PORT_DIPNAME( 0x10, 0x00, "Registration Length" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPNAME( 0x20, 0x00, "Registration" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

    PORT_START	/* SW BANK 2 */
	PORT_DIPNAME( 0x03, 0x00, "Coins for 1 Play" )			/* Confirmed */
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
    PORT_DIPSETTING(    0x02, "3" )
    PORT_DIPSETTING(    0x03, "4" )
    PORT_DIPNAME( 0x0C, 0x0C, "Plays for 1 Coin" )			/* Confirmed */
    PORT_DIPSETTING(    0x0C, "2" )
    PORT_DIPSETTING(    0x08, "3" )
    PORT_DIPSETTING(    0x04, "4" )
    PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ))				/* Confirmed */
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Meter Pulses" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "5" )

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END

static struct GfxLayout charlayout8colour =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,		/* 3 bits per pixel */
	{ 0, 0x800*8, 0x1000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

/* S2636 Mappings */

static struct GfxLayout s2636_character10 =
{
	8,10,
	5,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
   	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	8*16
};

static struct GfxDecodeInfo cvs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout8colour, 0, 259 },	/* Rom chars */
	{ REGION_CPU1, 0x7c00, &charlayout8colour, 0, 259 },	/* Ram chars */
  	{ REGION_CPU1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #1  */
  	{ REGION_CPU1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #2  */
  	{ REGION_CPU1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #3  */
	{ -1 } /* end of array */
};

static struct MachineDriver machine_driver_cvs =
{
	/* basic machine hardware */
	{
		{
			CPU_S2650,
			894886.25,
			cvs_readmem,cvs_writemem,cvs_readport,cvs_writeport,
			cvs_interrupt,1
		},
		{
			CPU_S2650 | CPU_AUDIO_CPU,
			894886.25/3,
			cvs_sound_readmem,cvs_sound_writemem,cvs_sound_readport,0,
			ignore_interrupt,1
		}
	},
	60, 1000,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 32*8-1 },
	cvs_gfxdecodeinfo,
	16,4096,
	cvs_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	cvs_vh_start,
	cvs_vh_stop,
	cvs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_TMS5110,
			&tms5110_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cvs )
	ROM_REGION( 0x8000, REGION_CPU3, 0 )
	ROM_LOAD( "5b.bin",            0x0000, 0x0800, 0xf055a624)

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
    ROM_LOAD( "82s185.10h",        0x0000, 0x0800, 0xc205bca6)
ROM_END


#define CVS_ROM(name,p1,gp1,p2,gp2,p3,gp3,p4,gp4,p5,gp5,p6,cp1,p7,cp2,p8,cp3,p9,sdp1,size1,p10,sp1,size2) \
ROM_START( name )                                               \
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) 						\
	ROM_LOAD( #p1"-gp1.bin",   0x0000, 0x0400, gp1 )        	\
	ROM_CONTINUE(                  0x2000, 0x0400 )             \
	ROM_CONTINUE(                  0x4000, 0x0400 )             \
	ROM_CONTINUE(                  0x6000, 0x0400 )             \
	ROM_LOAD( #p2"-gp2.bin",   0x0400, 0x0400, gp2 )        	\
	ROM_CONTINUE(                  0x2400, 0x0400 )             \
	ROM_CONTINUE(                  0x4400, 0x0400 )             \
	ROM_CONTINUE(                  0x6400, 0x0400 )             \
	ROM_LOAD( #p3"-gp3.bin",   0x0800, 0x0400, gp3 )        	\
	ROM_CONTINUE(                  0x2800, 0x0400 )             \
	ROM_CONTINUE(                  0x4800, 0x0400 )             \
	ROM_CONTINUE(                  0x6800, 0x0400 )             \
	ROM_LOAD( #p4"-gp4.bin",   0x0C00, 0x0400, gp4 )        	\
	ROM_CONTINUE(                  0x2C00, 0x0400 )             \
	ROM_CONTINUE(                  0x4C00, 0x0400 )             \
	ROM_CONTINUE(                  0x6C00, 0x0400 )             \
	ROM_LOAD( #p5"-gp5.bin",   0x1000, 0x0400, gp5 )        	\
	ROM_CONTINUE(                  0x3000, 0x0400 )             \
	ROM_CONTINUE(                  0x5000, 0x0400 )             \
	ROM_CONTINUE(                  0x7000, 0x0400 )             \
                                                                \
	ROM_REGION( 0x2000, REGION_CPU2, 0 ) 						\
	ROM_LOAD( #p9"-sdp1.bin",  0x0000, size1, sdp1)  			\
                                                                \
	ROM_REGION( 0x0800, REGION_CPU3, 0 ) 						\
	ROM_LOAD( "5b.bin",            0x0000, 0x0800, 0xf055a624)  \
                                                                \
	ROM_REGION( 0x1000, REGION_SOUND1, 0 ) 						\
    ROM_LOAD( #p10"-sp1.bin",   0x0000, size2, sp1)  			\
                                                                \
	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )        \
	ROM_LOAD( #p6"-cp1.bin",   0x0000, 0x0800, cp1 )        	\
	ROM_LOAD( #p7"-cp2.bin",   0x0800, 0x0800, cp2 )        	\
	ROM_LOAD( #p8"-cp3.bin",   0x1000, 0x0800, cp3 )        	\
                                                                \
	ROM_REGION( 0x0800, REGION_PROMS, ROMREGION_DISPOSE  )      \
    ROM_LOAD( "82s185.10h",        0x0000, 0x0800, 0xc205bca6 ) \
ROM_END

CVS_ROM(huncholy,ho,0x4f17cda7,ho,0x70fa52c7,ho,0x931934b1,ho,0xaf5cd501,ho,0x658e8974,ho,0xc6c73d46,ho,0xe596371c,ho,0x11fae1cf,ho,0x3efb3ffd,0x1000,ho,0x3fd39b1e,0x1000)
CVS_ROM(darkwar ,dw,0xf10ccf24,dw,0xb77d0483,dw,0xc01c3281,dw,0x0b0bffaf,dw,0x7fdbcaff,dw,0x7a0f9f3e,dw,0x232e5120,dw,0x573e0a17,dw,0xb385b669,0x0800,dw,0xce815074,0x1000)
CVS_ROM(8ball   ,8b,0x1b4fb37f,8b,0xf193cdb5,8b,0x191989bf,8b,0x9c64519e,8b,0xc50d0f9d,8b,0xc1f68754,8b,0x6ec1d711,8b,0x4a9afce4,8b,0xa571daf4,0x1000,8b,0x1ee167f3,0x0800)
CVS_ROM(8ball1  ,8a,0xb5d3b763,8a,0x5e4aa61a,8a,0x3dc272fe,8a,0x33afedbf,8a,0xb8b3f373,8a,0xd9b36c16,8a,0x6f66f0ff,8a,0xbaee8b17,8b,0xa571daf4,0x1000,8b,0x1ee167f3,0x0800)
CVS_ROM(hunchbak,hb,0xaf801d54,hb,0xb448cc8e,hb,0x57c6ea7b,hb,0x7f91287b,hb,0x1dd5755c,hb,0xf256b047,hb,0xb870c64f,hb,0x9a7dab88,hb,0xf9ba2854,0x1000,hb,0xed1cd201,0x0800)
CVS_ROM(wallst  ,ws,0xbdac81b6,ws,0x9ca67cdd,ws,0xc2f407f2,ws,0x1e4b2fe1,ws,0xeec7bfd0,ws,0x5aca11df,ws,0xca530d85,ws,0x1e0225d6,ws,0xfaed2ac0,0x1000,ws,0x84b72637,0x0800)
CVS_ROM(dazzler ,dz,0x2c5d75de,dz,0xd0db80d6,dz,0xd5f07796,dz,0x84e41a46,dz,0x2ae59c41,dz,0x0a8a9034,dz,0x3868dd82,dz,0x755d9ed2,dz,0x89847352,0x1000,dz,0x25da1fc1,0x0800)
CVS_ROM(radarzon,rd,0x775786ba,rd,0x9f6be426,rd,0x61d11b29,rd,0x2fbc778c,rd,0x692a99d5,rd,0xed601677,rd,0x35e317ff,rd,0x90f2c43f,rd,0xcd5aea6d,0x0800,rd,0x43b17734,0x0800)
CVS_ROM(radarzn1,r1,0x7c73c21f,r1,0xdedbd2ce,r1,0x966a49e7,r1,0xf3175bee,r1,0x7484927b,rd,0xed601677,rd,0x35e317ff,rd,0x90f2c43f,rd,0xcd5aea6d,0x0800,rd,0x43b17734,0x0800)
CVS_ROM(radarznt,rt,0x43573974,rt,0x257a11ce,rt,0xe00f3552,rt,0xd1e824ac,rt,0xbc770af8,rd,0xed601677,rd,0x35e317ff,rd,0x90f2c43f,rd,0xcd5aea6d,0x0800,rd,0x43b17734,0x0800)
CVS_ROM(outline ,rt,0x43573974,rt,0x257a11ce,ot,0x699489e1,ot,0xc94aca17,ot,0x154712f4,rd,0xed601677,rd,0x35e317ff,rd,0x90f2c43f,ot,0x739066a9,0x0800,ot,0xfa21422a,0x1000)
CVS_ROM(goldbug ,gb,0x8deb7761,gb,0x135036c1,gb,0xd48b1090,gb,0xc8053205,gb,0xeca17472,gb,0x80e1ad5a,gb,0x0a288b29,gb,0xe5bcf8cf,gb,0xc8a4b39d,0x1000,gb,0x5d0205c3,0x0800)
CVS_ROM(superbik,sb,0xf0209700,sb,0x1956d687,sb,0xceb27b75,sb,0x430b70b3,sb,0x013615a3,sb,0x03ba7760,sb,0x04de69f2,sb,0xbb7d0b9a,sb,0xe977c090,0x0800,sb,0x0aeb9ccd,0x0800)
CVS_ROM(hero    ,hr,0x82f39788,hr,0x79607812,hr,0x2902715c,hr,0x696d2f8e,hr,0x936a4ba6,hr,0x2d201496,hr,0x21b61fe3,hr,0x9c8e3f9e,hr,0xc34ecf79,0x0800,hr,0xa5c33cb1,0x0800)
CVS_ROM(logger  ,lg,0x0022b9ed,lg,0x23c5c8dc,lg,0xf9288f74,lg,0xe52ef7bf,lg,0x4ee04359,lg,0xe4ede80e,lg,0xd3de8e5b,lg,0x9b8d1031,lg,0x5af8da17,0x1000,lg,0x74f67815,0x0800)
CVS_ROM(cosmos  ,cs,0x7eb96ddf,cs,0x6975a8f7,cs,0x76904b13,cs,0xbdc89719,cs,0x94be44ea,cs,0x6a48c898,cs,0xdb0dfd8c,cs,0x01eee875,cs,0xb385b669,0x0800,cs,0x3c7fe86d,0x1000)
CVS_ROM(heartatk,ha,0xe8297c23,ha,0xf7632afc,ha,0xa9ce3c6a,ha,0x090f30a9,ha,0x163b3d2d,ha,0x2d0f6d13,ha,0x7f5671bd,ha,0x35b05ab4,ha,0xb9c466a0,0x1000,ha,0xfa21422a,0x1000)

static void init_cosmos(void)
{
	/* Patch out 2nd Character Mode Change */

    memory_region(REGION_CPU1)[0x0357] = 0xc0;
    memory_region(REGION_CPU1)[0x0358] = 0xc0;
}

static void init_goldbug(void)
{
	/* Redirect calls to real memory bank */

    memory_region(REGION_CPU1)[0x4347] = 0x1e;
    memory_region(REGION_CPU1)[0x436a] = 0x1e;
}

static void init_huncholy(void)
{
    /* Patch out protection */

    memory_region(REGION_CPU1)[0x0082] = 0xc0;
    memory_region(REGION_CPU1)[0x0083] = 0xc0;
    memory_region(REGION_CPU1)[0x0084] = 0xc0;
    memory_region(REGION_CPU1)[0x00b7] = 0xc0;
    memory_region(REGION_CPU1)[0x00b8] = 0xc0;
    memory_region(REGION_CPU1)[0x00b9] = 0xc0;
    memory_region(REGION_CPU1)[0x00d9] = 0xc0;
    memory_region(REGION_CPU1)[0x00da] = 0xc0;
    memory_region(REGION_CPU1)[0x00db] = 0xc0;
    memory_region(REGION_CPU1)[0x4456] = 0xc0;
    memory_region(REGION_CPU1)[0x4457] = 0xc0;
    memory_region(REGION_CPU1)[0x4458] = 0xc0;
}

static void init_superbik(void)
{
    /* Patch out protection */

    memory_region(REGION_CPU1)[0x0079] = 0xc0;
    memory_region(REGION_CPU1)[0x007a] = 0xc0;
    memory_region(REGION_CPU1)[0x007b] = 0xc0;
    memory_region(REGION_CPU1)[0x0081] = 0xc0;
    memory_region(REGION_CPU1)[0x0082] = 0xc0;
    memory_region(REGION_CPU1)[0x0083] = 0xc0;
    memory_region(REGION_CPU1)[0x00b6] = 0xc0;
    memory_region(REGION_CPU1)[0x00b7] = 0xc0;
    memory_region(REGION_CPU1)[0x00b8] = 0xc0;
    memory_region(REGION_CPU1)[0x0168] = 0xc0;
    memory_region(REGION_CPU1)[0x0169] = 0xc0;
    memory_region(REGION_CPU1)[0x016a] = 0xc0;

    /* and speed up the protection check */

    memory_region(REGION_CPU1)[0x0099] = 0xc0;
    memory_region(REGION_CPU1)[0x009a] = 0xc0;
    memory_region(REGION_CPU1)[0x009b] = 0xc0;
    memory_region(REGION_CPU1)[0x00bb] = 0xc0;
    memory_region(REGION_CPU1)[0x00bc] = 0xc0;
    memory_region(REGION_CPU1)[0x00bd] = 0xc0;
}

static void init_hero(void)
{
    /* Patch out protection */

    memory_region(REGION_CPU1)[0x0087] = 0xc0;
    memory_region(REGION_CPU1)[0x0088] = 0xc0;
    memory_region(REGION_CPU1)[0x0aa1] = 0xc0;
    memory_region(REGION_CPU1)[0x0aa2] = 0xc0;
    memory_region(REGION_CPU1)[0x0aa3] = 0xc0;
    memory_region(REGION_CPU1)[0x0aaf] = 0xc0;
    memory_region(REGION_CPU1)[0x0ab0] = 0xc0;
    memory_region(REGION_CPU1)[0x0ab1] = 0xc0;
    memory_region(REGION_CPU1)[0x0abd] = 0xc0;
    memory_region(REGION_CPU1)[0x0abe] = 0xc0;
    memory_region(REGION_CPU1)[0x0abf] = 0xc0;
    memory_region(REGION_CPU1)[0x4de0] = 0xc0;
    memory_region(REGION_CPU1)[0x4de1] = 0xc0;
    memory_region(REGION_CPU1)[0x4de2] = 0xc0;
}

/******************************************************************************/

GAMEX( 1981, cvs,        0,        cvs,      cvs,    0,          ROT90, "Century Electronics","CVS Bios", NOT_A_DRIVER )

/******************************************************************************/

GAME( 1981, cosmos,      cvs,      cvs,      cvs,    cosmos,     ROT90, "Century Electronics","Cosmos" )
GAME( 1981, darkwar,     cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Dark Warrior" )
GAME( 1982, 8ball,       cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Video Eight Ball" )
GAME( 1982, 8ball1,      8ball,    cvs,      cvs,    0,          ROT90, "Century Electronics","Video Eight Ball - Rev.1" )
GAME( 1982, logger,      cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Logger" )
GAME( 1982, dazzler,     cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Dazzler" )
GAME( 1982, wallst,      cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Wall Street" )
GAME( 1982, radarzon,    cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Radar Zone" )
GAME( 1982, radarzn1,    radarzon, cvs,      cvs,    0,          ROT90, "Century Electronics","Radar Zone - Rev.1" )
GAME( 1982, radarznt,    radarzon, cvs,      cvs,    0,          ROT90, "Century Electronics (Tuni Electro Service Inc)","Radar Zone - Tuni" )
GAME( 1982, outline,     radarzon, cvs,      cvs,    0,          ROT90, "Century Electronics","Outline" )
GAME( 1982, goldbug,     cvs,      cvs,      cvs,    goldbug,    ROT90, "Century Electronics","Gold Bug" )
GAME( 1983, heartatk,    cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Heart Attack" )
GAME( 1983, hunchbak,    cvs,      cvs,      cvs,    0,          ROT90, "Century Electronics","Hunchback" )
GAME( 1983, superbik,    cvs,      cvs,      cvs,    superbik,   ROT90, "Century Electronics","Superbike" )
GAME( 1983, hero,        cvs,      cvs,      cvs,    hero,       ROT90, "Seatongrove Ltd","Hero" )
GAME( 1984, huncholy,    cvs,      cvs,      cvs,    huncholy,   ROT90, "Seatongrove Ltd","Hunchback Olympic" )
