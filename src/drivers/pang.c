/***************************************************************************

	Pang (Mitchell Corp) / Buster Bros (Capcom)
	Super Pang (Mitchell Corp)
	Block Block (Capcom)

	Driver submitted by Paul Leaman

	Processor:
	1xZ80 (40006psc)

	Sound:
	1xOKIM6295 (9Z002 VI)   ADPCM chip
	1xYM2413                FM Chip

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* in machine/kabuki.c */
void bbros_decode(void);
void spang_decode(void);
void sbbros_decode(void);
void qtono1_decode(void);
void block_decode(void);


int  pang_vh_start(void);
void pang_vh_stop(void);
void pang_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void pang_video_bank_w(int offset, int data);
void pang_videoram_w(int offset, int data);
int  pang_videoram_r(int offset);
void pang_colorram_w(int offset, int data);
int  pang_colorram_r(int offset);
void pang_palette_bank_w(int offset, int data);
void pang_paletteram_w(int offset, int data);
int  pang_paletteram_r(int offset);

extern unsigned char *pang_videoram;
extern unsigned char *pang_colorram;

extern int pang_videoram_size;
extern int pang_colorram_size;



static int dial[2],dial_selected;

static void init_machine(void)
{
	/* start with the dial disabled, it will be enabled only for Block Block */
	dial_selected = 0;
}

int block_input_r(int offset)
{
	if (dial_selected)
	{
		static int dir[2];

		if (dial_selected == 1)
		{
			int delta;

			delta = (readinputport(4 + offset) - dial[offset]) & 0xff;
			if (delta & 0x80)
			{
				delta = (-delta) & 0xff;
				if (dir[offset])
				{
				/* don't report movement on a direction change,otherwise it will stutter */
					dir[offset] = 0;
					delta = 0;
				}
			}
			else if (delta > 0)
			{
				if (dir[offset] == 0)
				{
				/* don't report movement on a direction change,otherwise it will stutter */
					dir[offset] = 1;
					delta = 0;
				}
			}
			if (delta > 0x3f) delta = 0x3f;
			return delta << 2;
		}
		else
		{
			int res;

			res = readinputport(1 + offset) & 0xf7;
			if (dir[offset]) res |= 0x08;

			return res;
		}
	}
	else
		return readinputport(1 + offset);
}

void block_dial_control_w(int offset,int data)
{
	if (data == 0x08)
	{
		/* reset the dial counters */
		dial[0] = readinputport(4);
		dial[1] = readinputport(5);
	}
	else if (data == 0x80)
		dial_selected = -1;
	else
		dial_selected = 1;
}



/***************************************************************************

  EEPROM

***************************************************************************/

static int eeprom_serial_count,eeprom_serial_buffer[26];
static UINT16 eeprom_data[0x40],eeprom_data_bits;

static int pang_port_5_r(int offset)
{
	int bit;

	bit = (eeprom_data_bits & 0x8000) >> 8;
	eeprom_data_bits = (eeprom_data_bits << 1) | 1;
	return (input_port_3_r(0) & 0x7f) | bit;
}

static void pang_eeprom_start_w(int offset,int data)
{
	static int last;

	if (!data && last) eeprom_serial_count = 0;

	last = data;
}

static void pang_eeprom_serial_w(int offset,int data)
{
	if (eeprom_serial_count < 26)
	{
		eeprom_serial_buffer[eeprom_serial_count++] = data;
		if (eeprom_serial_count == 10)
		{
			if (eeprom_serial_buffer[0] == 0x00 &&
					eeprom_serial_buffer[1] == 0xff &&
					eeprom_serial_buffer[2] == 0xff &&
					eeprom_serial_buffer[3] == 0x00)
			{
				int i,address;

				address = 0;
				for (i = 4;i < 10;i++)
				{
					address <<= 1;
					if (eeprom_serial_buffer[i]) address |= 1;
				}
if (errorlog) fprintf(errorlog,"EEPROM read %04x from address %02x\n",eeprom_data[address],address);
				eeprom_data_bits = eeprom_data[address];
			}
		}
		else if (eeprom_serial_count == 26)
		{
			if (eeprom_serial_buffer[0] == 0x00 &&
					eeprom_serial_buffer[1] == 0xff &&
					eeprom_serial_buffer[2] == 0x00 &&
					eeprom_serial_buffer[3] == 0xff)
			{
				int i,address,edata;

				address = 0;
				for (i = 4;i < 10;i++)
				{
					address <<= 1;
					if (eeprom_serial_buffer[i]) address |= 1;
				}
				edata = 0;
				for (i = 10;i < 26;i++)
				{
					edata <<= 1;
					if (eeprom_serial_buffer[i]) edata |= 1;
				}
if (errorlog) fprintf(errorlog,"EEPROM write %04x to address %02x\n",edata,address);
				eeprom_data[address] = edata;
			}
		}
	}
	else
if (errorlog) fprintf(errorlog,"EEPROM serial buffer overflow!\n");
}

static int nvram_load(void)
{
	void *f;


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(eeprom_data,0xff,0x80);

	return 1;
}

static void nvram_save(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fclose(f);
	}
}

static int spang_nvram_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fread(f,&RAM[0xe000],0x80);			/* NVRAM */
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(eeprom_data,0xff,0x80);

	return 1;
}

static void spang_nvram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fwrite(f,&RAM[0xe000],0x80);			/* NVRAM */
		osd_fclose(f);
	}
}

static int block_nvram_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fread(f,&RAM[0xff80],0x80);			/* NVRAM */
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(eeprom_data,0xff,0x80);

	return 1;
}

static void block_nvram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,eeprom_data,0x80);	/* EEPROM */
		osd_fwrite(f,&RAM[0xff80],0x80);			/* NVRAM */
		osd_fclose(f);
	}
}




/***************************************************************************

 Horrible bankswitch routine. Need to be able to read decrypted opcodes
 from ROM. There must be a better way than this.

***************************************************************************/

static void pang_bankswitch_w(int offset,int data)
{
	static int olddata=-1;

	if (data != olddata)
	{
		int bankaddress;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		bankaddress = 0x10000 + (data & 0x0f) * 0x4000;

		memcpy(ROM+0x8000, ROM+bankaddress, 0x4000);
		memcpy(RAM+0x8000, RAM+bankaddress, 0x4000);
		olddata=data;
	}
}



/***************************************************************************

  Memory handlers

***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, pang_paletteram_r },	/* Banked palette RAM */
	{ 0xc800, 0xcfff, pang_colorram_r },	/* Attribute RAM */
	{ 0xd000, 0xdfff, pang_videoram_r },	/* Banked char / OBJ RAM */
	{ 0xe000, 0xffff, MRA_RAM },	/* Work RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, pang_paletteram_w },
	{ 0xc800, 0xcfff, pang_colorram_w, &pang_colorram, &pang_colorram_size },
	{ 0xd000, 0xdfff, pang_videoram_w, &pang_videoram, &pang_videoram_size },
	{ 0xe000, 0xffff, MWA_RAMROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x02, block_input_r },
	{ 0x05, 0x05, pang_port_5_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, pang_palette_bank_w },    /* Palette bank */
	{ 0x01, 0x01, block_dial_control_w },
	{ 0x02, 0x02, pang_bankswitch_w },      /* Code bank register */
	{ 0x03, 0x03, YM2413_data_port_0_w },
	{ 0x04, 0x04, YM2413_register_port_0_w },
	{ 0x05, 0x05, OKIM6295_data_0_w },
	{ 0x06, 0x06, MWA_NOP },                /* Watchdog ? */
	{ 0x07, 0x07, pang_video_bank_w },      /* Video RAM bank register */
	{ 0x08, 0x08, pang_eeprom_start_w },
	{ 0x10, 0x10, MWA_NOP },	/* clocks the EEPROM, I think */
	{ 0x18, 0x18, pang_eeprom_serial_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( pang_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */
INPUT_PORTS_END

INPUT_PORTS_START( qtono1_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	/* same as the service mode farther down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */
INPUT_PORTS_END

INPUT_PORTS_START( block_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* dial direction */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* dial direction */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* DIAL1 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL, 100, 0, 0, 0)

	PORT_START      /* DIAL2 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 0, 0, 0)
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	32768,	/* 32768 characters */
	4,		/* 4 bits per pixel */
	{ 32768*16*8+4, 32768*16*8+0,4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,     0, 128 }, /* colors  */
	{ 1, 0x100000, &spritelayout,   0, 128 }, /* colors  */
	{ -1 } /* end of array */
};



static struct YM2413interface ym2413_interface=
{
    1,	/* 1 chip */
    8000000,	/* 8MHz ??? (hand tuned) */
    { 255 },	/* Volume */
    NULL,	/* IRQ handler */
};

static struct OKIM6295interface okim6295_interface =
{
	1,	/* 1 chip */
	8000,	/* 8000Hz ??? */
	{ 2 },	/* memory region 2 */
	{ 100 }
};



static struct MachineDriver machine_driver =
{
	{
		{
			CPU_Z80,
			8000000,	/* Super Pang says 8MHZ ORIGINAL BOARD */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_machine,

	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	128*16, 128*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,	/* jerky otherwise */
	0,
	pang_vh_start,
	pang_vh_stop,
	pang_vh_screenrefresh,
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		},
		{
			SOUND_YM2413,
			&ym2413_interface
		},
	}
};




ROM_START( pang_rom )
	ROM_REGION(0x30000)     /* 64k for code */
	ROM_LOAD( "pang6.bin",    0x00000, 0x08000, 0x68be52cd )
	ROM_LOAD( "pang7.bin",    0x10000, 0x20000, 0x4a2e70f6 )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pang_09.bin",  0x000000, 0x20000, 0x3a5883f5 )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, 0x79a8ed08 )
	/* 40000-7ffff empty */
	ROM_LOAD( "pang_11.bin",  0x080000, 0x20000, 0x166a16ae )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, 0x2fb3db6c )
	/* c0000-fffff empty */
	ROM_LOAD( "bb10.bin",     0x100000, 0x20000, 0xfdba4f6e )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x120000, 0x20000, 0x39f47a63 )

	ROM_REGION(0x10000)     /* OKIM */
	ROM_LOAD( "pang_01.bin",  0x00000, 0x10000, 0xb6463907 )
ROM_END

ROM_START( pangb_rom )
	ROM_REGION(0x60000)     /* 64k for code */
	ROM_LOAD( "pang_03.bin",  0x10000, 0x20000, 0x0c8477ae )   /* Decrypted data */
	ROM_LOAD( "pang_02.bin",  0x30000, 0x20000, 0x3f15bb61 )   /* Decrypted op codes */
	ROM_LOAD( "pang_04.bin",  0x50000, 0x10000, 0xf68f88a5 )   /* Decrypted opcode + data */

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pang_09.bin",  0x000000, 0x20000, 0x3a5883f5 )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, 0x79a8ed08 )
	/* 40000-7ffff empty */
	ROM_LOAD( "pang_11.bin",  0x080000, 0x20000, 0x166a16ae )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, 0x2fb3db6c )
	/* c0000-fffff empty */
	ROM_LOAD( "bb10.bin",     0x100000, 0x20000, 0xfdba4f6e )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x120000, 0x20000, 0x39f47a63 )

	ROM_REGION(0x10000)     /* OKIM */
	ROM_LOAD( "pang_01.bin",  0x00000, 0x10000, 0xb6463907 )
ROM_END

static void pangb_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* this is a bootleg, the ROMs contain decrypted opcodes and data separately */
	memcpy(ROM, RAM+0x50000, 0x8000);   /* OP codes */
	memcpy(ROM+0x10000, RAM+0x30000, 0x20000);   /* OP codes */
	memcpy(RAM, RAM+0x58000, 0x8000);   /* Data */
}

ROM_START( bbros_rom )
	ROM_REGION(0x30000)     /* 64k for code */
	ROM_LOAD( "bb6.bin",      0x00000, 0x08000, 0xa3041ca4 )
	ROM_LOAD( "bb7.bin",      0x10000, 0x20000, 0x09231c68 )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bb2.bin",      0x000000, 0x20000, 0x62f29992 )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, 0x79a8ed08 )
	/* 40000-7ffff empty */
	ROM_LOAD( "bb4.bin",      0x080000, 0x20000, 0xf705aa89 )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, 0x2fb3db6c )
	/* c0000-fffff empty */
	ROM_LOAD( "bb10.bin",     0x100000, 0x20000, 0xfdba4f6e )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x120000, 0x20000, 0x39f47a63 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "bb1.bin",      0x00000, 0x20000, 0xc52e5b8e )
ROM_END

ROM_START( spang_rom )
	ROM_REGION(0x50000)     /* 64k for code */
	ROM_LOAD( "spe_06.rom",   0x00000, 0x08000, 0x1af106fb )
	ROM_LOAD( "spe_07.rom",   0x10000, 0x20000, 0x208b5f54 )
	ROM_LOAD( "spe_08.rom",   0x30000, 0x20000, 0x2bc03ade )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spe_02.rom",   0x000000, 0x20000, 0x63c9dfd2 )	/* chars */
	ROM_LOAD( "03.f2",        0x020000, 0x20000, 0x3ae28bc1 )
	/* 40000-7ffff empty */
	ROM_LOAD( "spe_04.rom",   0x080000, 0x20000, 0x9d7b225b )
	ROM_LOAD( "05.g2",        0x0a0000, 0x20000, 0x4a060884 )
	/* c0000-fffff empty */
	ROM_LOAD( "spe_10.rom",   0x100000, 0x20000, 0xeedd0ade )	/* sprites */
	ROM_LOAD( "spe_09.rom",   0x120000, 0x20000, 0x04b41b75 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "spe_01.rom",   0x00000, 0x20000, 0x2d19c133 )
ROM_END

ROM_START( sbbros_rom )
	ROM_REGION(0x50000)     /* 64k for code */
	ROM_LOAD( "06.j12",       0x00000, 0x08000, 0x292eee6a )
	ROM_LOAD( "07.j13",       0x10000, 0x20000, 0xf46b698d )
	ROM_LOAD( "08.j14",       0x30000, 0x20000, 0xa75e7fbe )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "02.f1",        0x000000, 0x20000, 0x0c22ffc6 )	/* chars */
	ROM_LOAD( "03.f2",        0x020000, 0x20000, 0x3ae28bc1 )
	/* 40000-7ffff empty */
	ROM_LOAD( "04.g2",        0x080000, 0x20000, 0xbb3dee5b )
	ROM_LOAD( "05.g2",        0x0a0000, 0x20000, 0x4a060884 )
	/* c0000-fffff empty */
	ROM_LOAD( "10.l2",        0x100000, 0x20000, 0xd6675d8f )	/* sprites */
	ROM_LOAD( "09.l1",        0x120000, 0x20000, 0x8f678bc8 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "01.d1",        0x00000, 0x20000, 0xb96ea126 )
ROM_END

ROM_START( qtono1_rom )
	ROM_REGION(0x50000)
	ROM_LOAD( "q3-05.rom",    0x00000, 0x08000, 0x1dd0a344 )
	ROM_LOAD( "q3-06.rom",    0x10000, 0x20000, 0x00000000 )
	ROM_LOAD( "q3-07.rom",    0x30000, 0x20000, 0x00000000 )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "q3-08.rom",    0x000000, 0x20000, 0x1533b978 )	/* chars */
	ROM_LOAD( "q3-09.rom",    0x020000, 0x20000, 0xa32db2f2 )
	ROM_LOAD( "q3-10.rom",    0x040000, 0x20000, 0xed681aa8 )
	ROM_LOAD( "q3-11.rom",    0x060000, 0x20000, 0x38b2fd10 )
	ROM_LOAD( "q3-18.rom",    0x080000, 0x20000, 0x9e4292ac )
	ROM_LOAD( "q3-19.rom",    0x0a0000, 0x20000, 0xb7f6d40f )
	ROM_LOAD( "q3-20.rom",    0x0c0000, 0x20000, 0x6cd7f38d )
	ROM_LOAD( "q3-21.rom",    0x0e0000, 0x20000, 0xb4aa6b4b )
	ROM_LOAD( "q3-16.rom",    0x100000, 0x20000, 0x863d6836 )	/* sprites */
	ROM_LOAD( "q3-17.rom",    0x120000, 0x20000, 0x459bf59c )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "q3-01.rom",    0x00000, 0x20000, 0x6c1be591 )
ROM_END

ROM_START( block_rom )
	ROM_REGION(0x50000)
	ROM_LOAD( "ble_05.rom",   0x00000, 0x08000, 0xc12e7f4c )
	ROM_LOAD( "ble_06.rom",   0x10000, 0x20000, 0xcdb13d55 )
	ROM_LOAD( "ble_07.rom",   0x30000, 0x20000, 0x1d114f13 )

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bl_08.rom",    0x000000, 0x20000, 0x00000000 )	/* chars */
	ROM_LOAD( "bl_09.rom",    0x020000, 0x20000, 0x6fa8c186 )
	/* 40000-7ffff empty */
	ROM_LOAD( "bl_18.rom",    0x080000, 0x20000, 0xc0acafaf )
	ROM_LOAD( "bl_19.rom",    0x0a0000, 0x20000, 0x1ae942f5 )
	/* c0000-fffff empty */
	ROM_LOAD( "bl_16.rom",    0x100000, 0x20000, 0xfadcaff7 )	/* sprites */
	ROM_LOAD( "bl_17.rom",    0x120000, 0x20000, 0x5f8cab42 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "bl_01.rom",    0x00000, 0x20000, 0xc2ec2abb )
ROM_END

ROM_START( blockbl_rom )
	ROM_REGION(0xa0000)
	ROM_LOAD( "m6.l5",        0x30000, 0x20000, 0x5768d8eb )   /* Decrypted data */
	ROM_LOAD( "m7.l6",        0x50000, 0x10000, 0x3b576fd9 )   /* Decrypted opcode + data */
	ROM_LOAD( "m5.l3",        0x60000, 0x40000, 0x7c988bb7 )   /* Decrypted opcode + data */

	ROM_REGION_DISPOSE(0x140000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "m12.o10",      0x000000, 0x20000, 0x963154d9 )	/* chars */
	ROM_LOAD( "m13.o14",      0x020000, 0x20000, 0x069480bb )
	/* 40000-7ffff empty */
	ROM_LOAD( "m4.j17",       0x080000, 0x20000, 0x9e3b6f4f )
	ROM_LOAD( "m3.j20",       0x0a0000, 0x20000, 0x629d58fe )
	/* c0000-fffff empty */
	ROM_LOAD( "m11.o7",       0x100000, 0x10000, 0x255180a5 )	/* sprites */
	ROM_LOAD( "m10.o5",       0x110000, 0x10000, 0x3201c088 )
	ROM_LOAD( "m9.o3",        0x120000, 0x10000, 0x29357fe4 )
	ROM_LOAD( "m8.o2",        0x130000, 0x10000, 0xabd665d1 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "m1.a1",        0x00000, 0x10000, 0xd826c105 )
	ROM_LOAD( "m2.c1",        0x10000, 0x10000, 0xe03a07ea )
ROM_END

static void blockbl_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* this is a bootleg, the ROMs contain decrypted opcodes and data separately */
	memcpy(ROM, RAM+0x50000, 0x8000);   /* OP codes */
	memcpy(RAM, RAM+0x58000, 0x8000);   /* Data */
	memcpy(ROM+0x10000, RAM+0x60000, 0x20000);   /* OP codes */
	memcpy(RAM+0x10000, RAM+0x80000, 0x20000);   /* Data */
}



struct GameDriver pang_driver =
{
	__FILE__,
	0,
	"pang",
	"Pang (World)",
	"1989",
	"Mitchell",
	"Paul Leaman\nMario Silva",
	0,
	&machine_driver,
	0,

	pang_rom,
	0, bbros_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	nvram_load, nvram_save
};

struct GameDriver pangb_driver =
{
	__FILE__,
	&pang_driver,
	"pangb",
	"Pang (bootleg)",
	"1989",
	"bootleg",
	"Paul Leaman\nMario Silva",
	0,
	&machine_driver,
	0,

	pangb_rom,
	0, pangb_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	nvram_load, nvram_save
};

struct GameDriver bbros_driver =
{
	__FILE__,
	&pang_driver,
	"bbros",
	"Buster Bros (US)",
	"1989",
	"Capcom",
	"Paul Leaman\nMario Silva",
	0,
	&machine_driver,
	0,

	bbros_rom,
	0, bbros_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	nvram_load, nvram_save
};

struct GameDriver spang_driver =
{
	__FILE__,
	0,
	"spang",
	"Super Pang (World)",
	"1990",
	"Mitchell",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	spang_rom,
	0, spang_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	spang_nvram_load, spang_nvram_save
};

struct GameDriver sbbros_driver =
{
	__FILE__,
	&spang_driver,
	"sbbros",
	"Super Buster Bros (US)",
	"1990",
	"Mitchell + Capcom",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	sbbros_rom,
	0, sbbros_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	spang_nvram_load, spang_nvram_save
};

struct GameDriver qtono1_driver =
{
	__FILE__,
	0,
	"qtono1",
	"Quiz Tonosama no Yabou",
	"1991",
	"Capcom",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	qtono1_rom,
	0, qtono1_decode,
	0,
	0,      /* sound_prom */

	qtono1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	nvram_load, nvram_save
};

struct GameDriver block_driver =
{
	__FILE__,
	0,
	"block",
	"Block Block (World)",
	"1991",
	"Capcom",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	block_rom,
	0, block_decode,
	0,
	0,      /* sound_prom */

	block_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	block_nvram_load, block_nvram_save
};

struct GameDriver blockbl_driver =
{
	__FILE__,
	&block_driver,
	"blockbl",
	"Block Block (bootleg)",
	"1991",
	"bootleg",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	blockbl_rom,
	0, blockbl_decode,
	0,
	0,      /* sound_prom */

	block_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	block_nvram_load, block_nvram_save
};
