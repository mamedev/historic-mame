/******************************************************************
Terra Cresta (preliminary)
Nichibutsu 1985
68000 + Z80

TODO: I'm playing samples with a DAC, but they could be ADPCM

Carlos A. Lozano (calb@gsyc.inf.uc3m.es)

MEMORY MAP
0x000000 - 0x01ffff   ROM
0x020000 - 0x02006f   VRAM (Sprites)???
0x020070 - 0x021fff   RAM
0x022000 - 0x022fff   VRAM (Background)???
0x024000 - 0x24000f   Input Ports
0x026000 - 0x26000f   Output Ports
0x028000 - 0x287fff   VRAM (Tiles)

VRAM(Background)
0x22000 - 32 bytes (16 tiles)
0x22040 - 32 bytes (16 tiles)
0x22080 - 32 bytes (16 tiles)
0x220c0 - 32 bytes (16 tiles)
0x22100 - 32 bytes (16 tiles)
...
0x22fc0 - 32 bytes (16 tiles)

VRAM(Tiles)
0x28000-0x287ff (1024 tiles 8x8 tiles, 2 bytes every tile)

VRAM(Sprites)
0x20000-0x201ff

******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"
#include "Z80/Z80.h"

void terrac_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void terracre_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
//void terracre_vh_screenrefresh(struct osd_bitmap *bitmap);
int terrac_vh_start(void);
void terrac_vh_stop(void);
void terrac_videoram2_w(int offset,int data);
int terrac_videoram2_r(int offset);

extern unsigned char *terrac_videoram;
extern int terrac_videoram_size;
extern unsigned char terrac_scrolly[];


void terracre_r_write (int offset,int data)
{
	switch (offset)
	{
		case 0: /* ??? */
			break;
		case 2: /* Scroll Y */
			COMBINE_WORD_MEM(terrac_scrolly,data);
			break;
		case 4: /* ??? */
			break;
		case 0xa: /* ??? */
			break;
		case 0xc: /* sound command */
			soundlatch_w(offset,((data & 0x7f) << 1) | 1);
			break;
		case 0xe: /* ??? */
			break;
	}

	if( errorlog ) fprintf( errorlog, "OUTPUT [%x] <- %08x\n", offset,data );
}

int terracre_r_read(int offset)
{
	int msb,lsb;

	switch (offset)
	{
		case 0: /* Player controls */
			return (input_port_0_r (offset));

		case 2: /* Dipswitch 0xf000*/
			lsb = input_port_1_r (offset);
			msb = input_port_3_r (offset);
			return (lsb + (msb<<8));

		case 4: /* Start buttons & Dipswitch */
			lsb=input_port_4_r (offset);
			msb=input_port_2_r (offset);
			return (lsb + (msb<<8));

		case 6: /* Dipswitch???? */
			return (0xffff);

		default:
			if( errorlog ) fprintf( errorlog, "INPUT: [%x] \n", offset );
			return (0xffff);
	}
	return 0xffff;
}


int soundlatch_clear(int offset)
{
	soundlatch_clear_w(0,0);
	return 0;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x020000, 0x0201ff, MRA_BANK1 },
	{ 0x020200, 0x021fff, MRA_BANK2 },
	{ 0x022000, 0x022fff, terrac_videoram2_r },
	{ 0x023000, 0x023fff, MRA_BANK3 },
	{ 0x024000, 0x02400f, terracre_r_read },
	{ 0x028000, 0x0287ff, MRA_BANK4 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x020000, 0x0201ff, MWA_BANK1, &spriteram, &spriteram_size },
	{ 0x020200, 0x021fff, MWA_BANK2 },
	{ 0x022000, 0x022fff, terrac_videoram2_w, &terrac_videoram, &terrac_videoram_size },
	{ 0x023000, 0x023fff, MWA_BANK3 },
	{ 0x026000, 0x02600f, terracre_r_write },
	{ 0x028000, 0x0287ff, MWA_BANK4, &videoram, &videoram_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort sound_readport[] =
{
	{ 0x04, 0x04, soundlatch_clear },
	{ 0x06, 0x06, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport_3526[] =
{
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x03, DAC_data_w },	/* 2 channels */
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport_2203[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x02, 0x03, DAC_data_w },	/* 2 channels */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Coin, Start, Test, Dipswitch */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Test Mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dipswitch */
	PORT_DIPNAME( 0xff, 0xff, "--", IP_KEY_NONE )

	PORT_START	/* Dipswitch */
	PORT_DIPNAME( 0xff, 0xff, "--", IP_KEY_NONE )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout backlayout =
{
	16,16,	/* 16*16 chars */
	512,	/* 512 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* plane offset */
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8   /* every char takes 128 consecutive bytes  */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 characters */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 4+0x8000*8, 0+0x8000*8, 12, 8, 12+0x8000*8, 8+0x8000*8,
		20, 16, 20+0x8000*8, 16+0x8000*8, 28, 24, 28+0x8000*8, 24+0x8000*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
          8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every char takes 64 consecutive bytes  */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,            0,  1 },
	{ 1, 0x02000, &backlayout,         1*16, 16 },
	{ 1, 0x12000, &spritelayout, 1*16+16*16, 64 },
	{ -1 } /* end of array */
};




static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ???? */
	{ YM2203_VOL(255,100), YM2203_VOL(255,100) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	2,	/* 2 channels */
	441000,
	{ 255,255 },
	{  1,  1 }
};

int terracre_interrupt(void)
{
	return (1);
}


static struct MachineDriver ym3526_machine_driver =
{
	{
		{
			CPU_M68000,
			8000000, /* 8 Mhz?? */
			0,
			readmem,writemem,0,0,
			terracre_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz???? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport_3526,
			interrupt,128	/* ??? */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 1*16+16*16+16*64,
	terrac_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	terrac_vh_start,
	terrac_vh_stop,
	terracre_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
		   SOUND_YM3526,
		   &ym3526_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver ym2203_machine_driver =
{
	{
		{
			CPU_M68000,
			8000000, /* 8 Mhz?? */
			0,
			readmem,writemem,0,0,
			terracre_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz???? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport_2203,
			interrupt,128	/* ??? */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 1*16+16*16+16*64,
	terrac_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	terrac_vh_start,
	terrac_vh_stop,
	terracre_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
		   SOUND_YM2203,
		   &ym2203_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



ROM_START( terracre_rom )
	ROM_REGION(0x20000)	/* 128K for 68000 code */
	ROM_LOAD_ODD(  "1a_4b.rom", 0x00000, 0x04000, 0xc914c1b4 , 0x76f17479 )
	ROM_LOAD_EVEN( "1a_4d.rom", 0x00000, 0x04000, 0xf0d45a26 , 0x8119f06e )
	ROM_LOAD_ODD(  "1a_6b.rom", 0x08000, 0x04000, 0xf0defdc0 , 0xba4b5822 )
	ROM_LOAD_EVEN( "1a_6d.rom", 0x08000, 0x04000, 0x74bd81df , 0xca4852f6 )
	ROM_LOAD_ODD(  "1a_7b.rom", 0x10000, 0x04000, 0x33ac902e , 0xd0771bba )
	ROM_LOAD_EVEN( "1a_7d.rom", 0x10000, 0x04000, 0x9c89955b , 0x029d59d9 )
	ROM_LOAD_ODD(  "1a_9b.rom", 0x18000, 0x04000, 0x65b2ba66 , 0x69227b56 )
	ROM_LOAD_EVEN( "1a_9d.rom", 0x18000, 0x04000, 0x69a74071 , 0x5a672942 )

	ROM_REGION_DISPOSE(0x28000)
	ROM_LOAD( "2a_16b.rom", 0x000000, 0x02000, 0x55aee9e6 , 0x591a3804 ) /* tiles */
	ROM_LOAD( "1a_15f.rom", 0x002000, 0x08000, 0xe4f660ae , 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom", 0x00a000, 0x08000, 0x89aa59d8 , 0x30e297ff )
	ROM_LOAD( "2a_6e.rom", 0x012000, 0x04000, 0x59c55bcb , 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom", 0x016000, 0x04000, 0x1b423bc4 , 0xa70b565c )
	ROM_LOAD( "2a_6g.rom", 0x01a000, 0x04000, 0x4c9162ab , 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom", 0x01e000, 0x04000, 0xe8557425 , 0x450749fc )

	ROM_REGION(0x0500)	/* color PROMs */
	ROM_LOAD( "tc1a_10f.bin", 0x00000, 0x0100, 0x7a970207 , 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x00100, 0x0100, 0x7c4d0305 , 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x00200, 0x0100, 0xae890e03 , 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin", 0x00300, 0x0100, 0x2de00108 , 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin", 0x00400, 0x0100, 0x5a38000a , 0x2c43991f )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "2a_15b.rom", 0x000000, 0x04000, 0x47e6c77c , 0x604c3b11 )
	ROM_LOAD( "2a_17b.rom", 0x004000, 0x04000, 0xcb214a93 , 0xaffc898d )
	ROM_LOAD( "2a_18b.rom", 0x008000, 0x04000, 0xcdeefba6 , 0x302dc0ab )
ROM_END

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

ROM_START( terracra_rom )
	ROM_REGION(0x20000)	/* 128K for 68000 code */
	ROM_LOAD_ODD(  "1a_4b.rom", 0x00000, 0x04000, 0xc914c1b4 , 0x76f17479 )
	ROM_LOAD_EVEN( "1a_4d.rom", 0x00000, 0x04000, 0xf0d45a26 , 0x8119f06e )
	ROM_LOAD_ODD(  "1a_6b.rom", 0x08000, 0x04000, 0xf0defdc0 , 0xba4b5822 )
	ROM_LOAD_EVEN( "1a_6d.rom", 0x08000, 0x04000, 0x74bd81df , 0xca4852f6 )
	ROM_LOAD_ODD(  "1a_7b.rom", 0x10000, 0x04000, 0x33ac902e , 0xd0771bba )
	ROM_LOAD_EVEN( "1a_7d.rom", 0x10000, 0x04000, 0x9c89955b , 0x029d59d9 )
	ROM_LOAD_ODD(  "1a_9b.rom", 0x18000, 0x04000, 0x65b2ba66 , 0x69227b56 )
	ROM_LOAD_EVEN( "1a_9d.rom", 0x18000, 0x04000, 0x69a74071 , 0x5a672942 )

	ROM_REGION_DISPOSE(0x28000)
	ROM_LOAD( "2a_16b.rom", 0x000000, 0x02000, 0x55aee9e6 , 0x591a3804 ) /* tiles */
	ROM_LOAD( "1a_15f.rom", 0x002000, 0x08000, 0xe4f660ae , 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom", 0x00a000, 0x08000, 0x89aa59d8 , 0x30e297ff )
	ROM_LOAD( "2a_6e.rom", 0x012000, 0x04000, 0x59c55bcb , 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom", 0x016000, 0x04000, 0x1b423bc4 , 0xa70b565c )
	ROM_LOAD( "2a_6g.rom", 0x01a000, 0x04000, 0x4c9162ab , 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom", 0x01e000, 0x04000, 0xe8557425 , 0x450749fc )

	ROM_REGION(0x0500)	/* color PROMs */
	ROM_LOAD( "tc1a_10f.bin", 0x00000, 0x0100, 0x7a970207 , 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x00100, 0x0100, 0x7c4d0305 , 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x00200, 0x0100, 0xae890e03 , 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin", 0x00300, 0x0100, 0x2de00108 , 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin", 0x00400, 0x0100, 0x5a38000a , 0x2c43991f )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k to sound cpu */
	ROM_LOAD( "tc2a_15b.bin", 0x000000, 0x04000, 0x4ca15657 , 0x790ddfa9 )
	ROM_LOAD( "tc2a_17b.bin", 0x004000, 0x04000, 0xa62a50f0 , 0xd4531113 )
ROM_END



struct GameDriver terracre_driver =
{
	__FILE__,
	0,
	"terracre",
	"Terra Cresta (YM3526)",
	"1985",
	"Nichibutsu",
	"Carlos A. Lozano\nMirko Buffoni\nNicola Salmoria",
	0,
	&ym3526_machine_driver,

	terracre_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

struct GameDriver terracra_driver =
{
	__FILE__,
	&terracre_driver,
	"terracra",
	"Terra Cresta (YM2203)",
	"1985",
	"Nichibutsu",
	"Carlos A. Lozano\nMirko Buffoni\nNicola Salmoria",
	0,
	&ym2203_machine_driver,

	terracra_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
