#include "driver.h"
#include "TMS34010/tms34010.h"
#include "vidhrdw/generic.h"

static unsigned char *eeprom, *master_speedup, *slave_speedup;
static int eeprom_size;
static unsigned char *code_rom;
static int code_rom_size;
static unsigned char *master_videoram, *slave_videoram;
static struct osd_bitmap *tmpbitmap1, *tmpbitmap2;

void exterm_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	unsigned int i;

	palette += 3*0x1000;

	for (i = 0;i < 0x8000; i++)
	{
		int r = (i >> 10) & 0x1f;
		int g = (i >>  5) & 0x1f;
		int b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		*(palette++) = r;
		*(palette++) = g;
		*(palette++) = b;
	}
}

void exterm_init_machine(void)
{
	static int copied = 0;

	if (!copied)
	{
		bcopy(Machine->memory_region[Machine->drv->cpu[0].memory_region],code_rom,code_rom_size);
		copied = 1;
	}
}

int exterm_vh_start(void)
{
	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	if ((tmpbitmap1 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((tmpbitmap2 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		osd_free_bitmap(tmpbitmap1);
		return 1;
	}

	return 0;
}

void exterm_vh_stop (void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap1);
	osd_free_bitmap(tmpbitmap2);
}

static int exterm_master_videoram_r(int offset)
{
    return READ_WORD(&master_videoram[offset]);
}

static void exterm_master_videoram_w(int offset, int data)
{
	COMBINE_WORD_MEM(&master_videoram[offset], data);

	if (data & 0x8000)
	{
		data &= 0x0fff;
	}
	else
	{
		data += 0x1000;
	}

	if (tmpbitmap->depth == 16)
		((unsigned short *)tmpbitmap->line[offset >> 9])[(offset >> 1) & 0xff] = Machine->pens[data];
	else
		tmpbitmap->line[offset >> 9][(offset >> 1) & 0xff] = Machine->pens[data];
}

static int exterm_slave_videoram_r(int offset)
{
    return READ_WORD(&slave_videoram[offset]);
}

static void exterm_slave_videoram_w(int offset, int data)
{
	int x,y;
	struct osd_bitmap *foreground;

	COMBINE_WORD_MEM(&slave_videoram[offset], data);

	x = offset & 0xff;
	y = (offset >> 8) & 0xff;

	foreground = (offset & 0x10000) ? tmpbitmap2 : tmpbitmap1;

	if (tmpbitmap1->depth == 16)
	{
		((unsigned short *)foreground->line[y])[x  ] = Machine->pens[ (data       & 0xff)];
		((unsigned short *)foreground->line[y])[x+1] = Machine->pens[((data >> 8) & 0xff)];
	}
	else
	{
		foreground->line[y][x  ] = Machine->pens[ (data       & 0xff)];
		foreground->line[y][x+1] = Machine->pens[((data >> 8) & 0xff)];
	}
}


static struct rectangle foregroundvisiblearea =
{
	0, 255, 40, 238
};

void exterm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (TMS34010_io_display_blanked(0))
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

		return;
	}

	if (palette_recalc() != 0)
	{
		/* Redraw screen */

		int offset;

		for (offset = 0; offset < 256*256; offset+=2)
		{
			int data1,data2,data3,data4,x1,y1,x2,y2;

			x1 = offset & 0xff;
			y1 = (offset >> 8) & 0xff;

			x2 = (offset >> 1) & 0xff;
			y2 = offset >> 9;

			data3 = READ_WORD(&master_videoram[offset]);
			if (data3 & 0x8000)
			{
				data3 &= 0x0fff;
			}
			else
			{
				data3 += 0x1000;
			}

			data4 = READ_WORD(&master_videoram[offset+256*256]);
			if (data4 & 0x8000)
			{
				data4 &= 0x0fff;
			}
			else
			{
				data4 += 0x1000;
			}

			data1 = READ_WORD(&slave_videoram[offset]);
			data2 = READ_WORD(&slave_videoram[offset+256*256]);

			if (tmpbitmap1->depth == 16)
			{
				((unsigned short *)tmpbitmap ->line[y2     ])[x2  ] = Machine->pens[data3];
				((unsigned short *)tmpbitmap ->line[y2|0x80])[x2  ] = Machine->pens[data4];
				((unsigned short *)tmpbitmap1->line[y1     ])[x1  ] = Machine->pens[ (data1       & 0xff)];
				((unsigned short *)tmpbitmap1->line[y1     ])[x1+1] = Machine->pens[((data1 >> 8) & 0xff)];
				((unsigned short *)tmpbitmap2->line[y1     ])[x1  ] = Machine->pens[ (data2       & 0xff)];
				((unsigned short *)tmpbitmap2->line[y1     ])[x1+1] = Machine->pens[((data2 >> 8) & 0xff)];
			}
			else
			{
				tmpbitmap ->line[y2     ][x2  ] = Machine->pens[data3];
				tmpbitmap ->line[y2|0x80][x2  ] = Machine->pens[data4];
				tmpbitmap1->line[y1     ][x1  ] = Machine->pens[ (data1       & 0xff)];
				tmpbitmap1->line[y1     ][x1+1] = Machine->pens[((data1 >> 8) & 0xff)];
				tmpbitmap2->line[y1     ][x1  ] = Machine->pens[ (data2       & 0xff)];
				tmpbitmap2->line[y1     ][x1+1] = Machine->pens[((data2 >> 8) & 0xff)];
			}
		}
	}

	copybitmap(bitmap,tmpbitmap, 0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    if (TMS34010_get_DPYSTRT(1) & 0x800)
	{
		copybitmap(bitmap,tmpbitmap2,0,0,0,0,&foregroundvisiblearea,TRANSPARENCY_PEN ,Machine->pens[0]);
	}
	else
	{
		copybitmap(bitmap,tmpbitmap1,0,0,0,0,&foregroundvisiblearea,TRANSPARENCY_PEN ,Machine->pens[0]);
	}
}

static int exterm_coderom_r(int offset)
{
    return READ_WORD(&code_rom[offset]);
}

static int exterm_input_port_0_r(int offset)
{
	//if (errorlog) fprintf(errorlog, "Input Port 0 read %08X\n", cpu_getpc());
	return (input_port_1_r(offset) << 8) | input_port_0_r(offset);
}
static int exterm_input_port_1_r(int offset)
{
	//if (errorlog) fprintf(errorlog, "Input Port 1 read %08X\n", cpu_getpc());
	return input_port_2_r(offset);
}

static void exterm_output_port_0_w(int offset, int data)
{
	//if (errorlog) fprintf(errorlog, "Output Port 0 written %08X=%04X\n", cpu_getpc(), data);

	/* Bit 15=0 resets the slave CPU */
	if (!(data & 0x8000))
	{
		//cpu_reset(1);
	}
}
static void exterm_output_port_1_w(int offset, int data)
{
	//if (errorlog) fprintf(errorlog, "Sound command %08X=%02X\n", cpu_getpc(), data&0xff);
}

static int exterm_master_speedup_r(int offset)
{
	int value = READ_WORD(&master_speedup[offset]);

	// Suspend cpu if it's waiting for an interrupt
	if (cpu_getpc() == 0xfff4d9b0 && !value)
	{
		cpu_spinuntil_int();
	}

	return value;
}

static void exterm_slave_speedup_w(int offset, int data)
{
	// Suspend cpu if it's waiting for an interrupt
	if (cpu_getpc() == 0xfffff050)
	{
		//TMS34010_Regs* state = TMS34010_GetState();
		//state->Aregs[1] = 0x5000;
		//state->pc = 0xfffff0d0;
		//cpu_spinuntil_int();
	}

	WRITE_WORD(&slave_speedup[offset], data);
}

static struct MemoryReadAddress master_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_master_videoram_r },
	{ TOBYTE(0x00c800e0), TOBYTE(0x00c800ef), exterm_master_speedup_r, &master_speedup },
	{ TOBYTE(0x00c00000), TOBYTE(0x00ffffff), MRA_BANK1 },
	{ TOBYTE(0x01000000), TOBYTE(0x0100000f), MRA_NOP }, // Off by one bug in RAM test, prevent log entry
	{ TOBYTE(0x01200000), TOBYTE(0x012fffff), TMS34010_HSTDATA_r },
	{ TOBYTE(0x01400000), TOBYTE(0x0140000f), exterm_input_port_0_r },
	{ TOBYTE(0x01440000), TOBYTE(0x0144000f), exterm_input_port_1_r },
	{ TOBYTE(0x01480000), TOBYTE(0x0148000f), input_port_3_r },
	{ TOBYTE(0x01800000), TOBYTE(0x01807fff), paletteram_word_r },
	{ TOBYTE(0x01808000), TOBYTE(0x0180800f), MRA_NOP }, // Off by one bug in RAM test, prevent log entry
	{ TOBYTE(0x02800000), TOBYTE(0x02807fff), MRA_BANK2, &eeprom, &eeprom_size },
	{ TOBYTE(0x03000000), TOBYTE(0x03ffffff), exterm_coderom_r },
	{ TOBYTE(0x3f000000), TOBYTE(0x3fffffff), exterm_coderom_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MRA_BANK3, &code_rom, &code_rom_size },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress master_writemem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_master_videoram_w, &master_videoram },
	{ TOBYTE(0x00c00000), TOBYTE(0x00ffffff), MWA_BANK1 },
	{ TOBYTE(0x01000000), TOBYTE(0x010fffff), TMS34010_HSTADRL_w },
	{ TOBYTE(0x01100000), TOBYTE(0x011fffff), TMS34010_HSTADRH_w },
	{ TOBYTE(0x01200000), TOBYTE(0x012fffff), TMS34010_HSTDATA_w },
	{ TOBYTE(0x01300000), TOBYTE(0x013fffff), TMS34010_HSTCTLH_w },
	{ TOBYTE(0x01500000), TOBYTE(0x0150000f), exterm_output_port_0_w },
	{ TOBYTE(0x01580000), TOBYTE(0x0158000f), exterm_output_port_1_w },
	{ TOBYTE(0x015c0000), TOBYTE(0x015c000f), watchdog_reset_w },
	{ TOBYTE(0x01800000), TOBYTE(0x01807fff), paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ TOBYTE(0x02800000), TOBYTE(0x02807fff), MWA_BANK2 }, // EEPROM
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress slave_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_slave_videoram_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA_BANK4 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress slave_writemem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_slave_videoram_w, &slave_videoram },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ TOBYTE(0xfffffb90), TOBYTE(0xfffffb90), exterm_slave_speedup_w, &slave_speedup },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA_BANK4 },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( exterm_input_ports )

	PORT_START      /* IN0 LO */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START      /* IN0 HI */
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )	/* According to the test screen */
	PORT_DIPNAME( 0x06, 0x06, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "1 Credit/Coin" )
	PORT_DIPSETTING(    0x02, "2 Credits/Coin" )
	PORT_DIPSETTING(    0x04, "3 Credits/Coin" )
	PORT_DIPSETTING(    0x00, "4 Credits/Coin" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Credit/Coin" )
	PORT_DIPSETTING(    0x18, "2 Credits/Coin" )
	PORT_DIPSETTING(    0x28, "3 Credits/Coin" )
	PORT_DIPSETTING(    0x08, "4 Credits/Coin" )
	PORT_DIPSETTING(    0x30, "5 Credits/Coin" )
	PORT_DIPSETTING(    0x10, "6 Credits/Coin" )
	PORT_DIPSETTING(    0x20, "7 Credits/Coin" )
	PORT_DIPSETTING(    0x00, "8 Credits/Coin" )
	PORT_DIPNAME( 0x40, 0x40, "Memory Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Single" )
	PORT_DIPSETTING(    0x00, "Contionous" )
	PORT_DIPNAME( 0x80, 0x80, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			40000000,	/* 40 Mhz */
			0,
            master_readmem,master_writemem,0,0,
            ignore_interrupt,0
		},
		{
			CPU_TMS34010,
			40000000,	/* 40 Mhz */
			2,
            slave_readmem,slave_writemem,0,0,
            ignore_interrupt,0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	exterm_init_machine,

	/* video hardware */
    256, 256, { 0, 255, 0, 238 },

	0,
	4096+32768,0,
    exterm_vh_convert_color_prom,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	exterm_vh_start,
	exterm_vh_stop,
	exterm_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};



/***************************************************************************

  High score save/load

***************************************************************************/


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( exterm_rom )
	ROM_REGION(0x200000)     /* 2MB for 34010 code */
	ROM_LOAD_ODD(  "v101bg0",  0x000000, 0x10000, 0xc3d768f5 )
	ROM_LOAD_EVEN( "v101bg1",  0x000000, 0x10000, 0x64945e6c )
	ROM_LOAD_ODD(  "v101bg2",  0x020000, 0x10000, 0xa16ea1ac )
	ROM_LOAD_EVEN( "v101bg3",  0x020000, 0x10000, 0xd1647492 )
	ROM_LOAD_ODD(  "v101bg4",  0x040000, 0x10000, 0x4f59ad2f )
	ROM_LOAD_EVEN( "v101bg5",  0x040000, 0x10000, 0x1efdf203 )
	ROM_LOAD_ODD(  "v101bg6",  0x060000, 0x10000, 0x08128942 )
	ROM_LOAD_EVEN( "v101bg7",  0x060000, 0x10000, 0xbb4cc998 )
	ROM_LOAD_ODD(  "v101bg8",  0x080000, 0x10000, 0x37d21406 )
	ROM_LOAD_EVEN( "v101bg9",  0x080000, 0x10000, 0x88be2d1e )
	ROM_LOAD_ODD(  "v101bg10", 0x0a0000, 0x10000, 0xb7c0fd40 )
	ROM_LOAD_EVEN( "v101bg11", 0x0a0000, 0x10000, 0x7e7fb8a3 )
	ROM_LOAD_ODD(  "v101fg0",  0x180000, 0x10000, 0xcafea36e )
	ROM_LOAD_EVEN( "v101fg1",  0x180000, 0x10000, 0xd647a0cf )
	ROM_LOAD_ODD(  "v101fg2",  0x1a0000, 0x10000, 0xcbf2ccd8 )
	ROM_LOAD_EVEN( "v101fg3",  0x1a0000, 0x10000, 0x44e4a5e8 )
	ROM_LOAD_ODD(  "v101fg4",  0x1c0000, 0x10000, 0x1b6f6dff )
	ROM_LOAD_EVEN( "v101fg5",  0x1c0000, 0x10000, 0x48a7184f )
	ROM_LOAD_ODD(  "v101p0",   0x1e0000, 0x10000, 0x07aa7a1a )
	ROM_LOAD_EVEN( "v101p1",   0x1e0000, 0x10000, 0x04680a42 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */

	ROM_REGION(0x1000)	/* Slave CPU memory space. There are no ROMs mapped here */
ROM_END


static int hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, eeprom, eeprom_size);
		osd_fclose (f);
	}

	return 1;
}


static void hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, eeprom, eeprom_size);
		osd_fclose (f);
	}
}


struct GameDriver exterm_driver =
{
	__FILE__,
	0,
	"exterm",
	"Exterminator",
	"1989",
	"Premier Technology",
	"Alex Pasadyn+Zsolt Vasvari",
	0,
	&machine_driver,

	exterm_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	exterm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,

	hiload, hisave
};


