/***************************************************************************

Kick & Run - (c) 1987 Taito

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


unsigned char *mexico86_objectram;
int mexico86_objectram_size;

void mexico86_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}

void mexico86_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy,xc,yc;
	int gfx_num,gfx_attr,gfx_offs;


	/* recalc the palette if necessary */
	palette_recalc();


	/* Bubble Bobble doesn't have a real video RAM. All graphics (characters */
	/* and sprites) are stored in the same memory region, and information on */
	/* the background character columns is stored inthe area dd00-dd3f */

	/* This clears & redraws the entire screen each pass */
	fillbitmap(bitmap,Machine->gfx[0]->colortable[0],&Machine->drv->visible_area);

	sx = 0;
	for (offs = 0;offs < mexico86_objectram_size;offs += 4)
    {
		int height;


		/* skip empty sprites */
		/* this is dword aligned so the UINT32 * cast shouldn't give problems */
		/* on any architecture */
		if (*(UINT32 *)(&mexico86_objectram[offs]) == 0)
			continue;

		gfx_num = mexico86_objectram[offs + 1];
		gfx_attr = mexico86_objectram[offs + 3];

		if ((gfx_num & 0x80) == 0)	/* 16x16 sprites */
		{
			gfx_offs = ((gfx_num & 0x1f) * 0x80) + ((gfx_num & 0x60) >> 1) + 12;
			height = 2;
		}
		else	/* tilemaps (each sprite is a 16x256 column) */
		{
			gfx_offs = ((gfx_num & 0x3f) * 0x80);
			height = 32;
		}

		if ((gfx_num & 0xc0) == 0xc0)	/* next column */
			sx += 16;
		else
		{
			sx = mexico86_objectram[offs + 2];
			if (gfx_attr & 0x40) sx -= 256;
		}
		sy = 256 - height*8 - (mexico86_objectram[offs + 0]);

		for (xc = 0;xc < 2;xc++)
		{
			for (yc = 0;yc < height;yc++)
			{
				int goffs,code,color,flipx,flipy,x,y;

				goffs = gfx_offs + xc * 0x40 + yc * 0x02;
				code = videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_attr & 0x0f);
				color = (videoram[goffs + 1] & 0x3c) >> 2;
				flipx = videoram[goffs + 1] & 0x40;
//				flipy = videoram[goffs + 1] & 0x80;
flipy=0;
				x = sx + xc * 8;
				y = (sy + yc * 8) & 0xff;

				drawgfx(bitmap,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						x,y,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}




static unsigned char *shared;

static int shared_r(int offset)
{
	return shared[offset];
}

static void shared_w(int offset,int data)
{
	shared[offset] = data;
}


static void bankswitch_w(int offs,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

if ( errorlog && ( data > 5 ) )
	fprintf( errorlog, "Switching to invalid bank!\n" );

	cpu_setbank(1, &RAM[0x10000 + 0x4000 * (data & 7)]);
}



/***************************************************************************

 Mexico 86 68705 protection interface

 The following is ENTIRELY GUESSWORK!!!

***************************************************************************/
int mexico86_m68705_interrupt(void)
{
	/* I don't know how to handle the interrupt line so I just toggle it every time. */
	if (cpu_getiloops() & 1)
		cpu_set_irq_line(2,0,ASSERT_LINE);
	else
		cpu_set_irq_line(2,0,CLEAR_LINE);

    return 0;
}



static unsigned char portA_in,portA_out,ddrA;

int mexico86_68705_portA_r(int offset)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port A read %02x\n",cpu_get_pc(),portA_in);
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

void mexico86_68705_portA_w(int offset,int data)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port A write %02x\n",cpu_get_pc(),data);
	portA_out = data;
}

void mexico86_68705_ddrA_w(int offset,int data)
{
	ddrA = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  0   W  enables latch which holds data from main Z80 memory
 *  1   W  loads the latch which holds the low 8 bits of the address of
 *               the main Z80 memory location to access
 *  2   W  0 = read input ports, 1 = access Z80 memory
 *  3   W  clocks main Z80 memory access
 *  4   W  selects Z80 memory access direction (0 = write 1 = read)
 *  5   W  clocks a flip-flop which causes IRQ on the main Z80
 *  6   W  not used?
 *  7   W  not used?
 */

static unsigned char portB_in,portB_out,ddrB;

int mexico86_68705_portB_r(int offset)
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

static int address,latch;

void mexico86_68705_portB_w(int offset,int data)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port B write %02x\n",cpu_get_pc(),data);

	if ((ddrB & 0x01) && (~data & 0x01) && (portB_out & 0x01))
	{
		portA_in = latch;
	}
	if ((ddrB & 0x02) && (data & 0x02) && (~portB_out & 0x02)) /* positive edge trigger */
	{
		address = portA_out;
//if (errorlog) fprintf(errorlog,"%04x: 68705 address %02x\n",cpu_get_pc(),portA_out);
	}
	if ((ddrB & 0x08) && (~data & 0x08) && (portB_out & 0x08))
	{
		if (data & 0x10)	/* read */
		{
			if (data & 0x04)
			{
//if (errorlog) fprintf(errorlog,"%04x: 68705 read %02x from address %04x\n",cpu_get_pc(),shared[0x800+address],address);
				latch = shared[0x800+address];
			}
			else
			{
//if (errorlog) fprintf(errorlog,"%04x: 68705 read input port %04x\n",cpu_get_pc(),address);
				latch = readinputport((address & 1) + 1);
			}
		}
		else	/* write */
		{
//if (errorlog) fprintf(errorlog,"%04x: 68705 write %02x to address %04x\n",cpu_get_pc(),portA_out,address);
				shared[0x800+address] = portA_out;
		}
	}
	if ((ddrB & 0x20) && (~data & 0x20) && (portB_out & 0x20))
	{
		cpu_irq_line_vector_w(0,0,shared[0x800]);
		cpu_set_irq_line(0,0,HOLD_LINE);
	}
	if ((ddrB & 0x40) && (~data & 0x40) && (portB_out & 0x40))
	{
//if (errorlog) fprintf(errorlog,"%04x: 68705 unknown port B bit %02x\n",cpu_get_pc(),data);
	}
	if ((ddrB & 0x80) && (~data & 0x80) && (portB_out & 0x80))
	{
//if (errorlog) fprintf(errorlog,"%04x: 68705 unknown port B bit %02x\n",cpu_get_pc(),data);
	}

	portB_out = data;
}

void mexico86_68705_ddrB_w(int offset,int data)
{
	ddrB = data;
}




static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },	/* banked roms */
	{ 0xc000, 0xdfff, MRA_RAM },	/* working & object ram */
	{ 0xe000, 0xefff, shared_r },
	{ 0xf010, 0xf010, input_port_5_r },
	{ 0xf800, 0xffff, MRA_RAM }, /* communication ram - to connect 2 boards for 4 players */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xd4ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xd500, 0xd7ff, MWA_RAM, &mexico86_objectram, &mexico86_objectram_size },
	{ 0xd800, 0xdfff, MWA_RAM },	/* work ram */
	{ 0xe000, 0xefff, shared_w, &shared },
	{ 0xf000, 0xf000, bankswitch_w },
//	{ 0xf018, 0xf018,  },	/* maybe flip screen + other things */
	{ 0xf800, 0xffff, MWA_RAM }, /* communication ram */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x9800, 0x9aff, MRA_RAM },	/* shared? */
	{ 0xa000, 0xa7ff, shared_r },	/* the game doesn't work if a800-afff is shared */
	{ 0xa800, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc000, YM2203_status_port_0_r  },
	{ 0xc001, 0xc001, YM2203_read_port_0_r  },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x9800, 0x9aff, MWA_RAM },	/* shared? */
	{ 0xa000, 0xa7ff, shared_w },
	{ 0xa800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, YM2203_control_port_0_w  },
	{ 0xc001, 0xc001, YM2203_write_port_0_w  },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress m68705_readmem[] =
{
	{ 0x0000, 0x0000, mexico86_68705_portA_r },
	{ 0x0001, 0x0001, mexico86_68705_portB_r },
	{ 0x0002, 0x0002, input_port_0_r },	/* COIN */
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress m68705_writemem[] =
{
	{ 0x0000, 0x0000, mexico86_68705_portA_w },
	{ 0x0001, 0x0001, mexico86_68705_portB_w },
	{ 0x0004, 0x0004, mexico86_68705_ddrA_w },
	{ 0x0005, 0x0005, mexico86_68705_ddrB_w },
	{ 0x000a, 0x000a, MWA_NOP },	/* looks like a bug in the code, writes to */
									/* 0x0a (=10dec) instead of 0x10 */
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Max Players" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	3*2048,   /* 2048 characters */
	2,	  /* 2 bits per pixel */
	{ 0, 4 },	 /* the bitplanes are separated */
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	 /* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 64 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(25,25) },
	AY8910_DEFAULT_GAIN,
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			0,
			readmem,writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the 68705 */
		},
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			3,
			sound_readmem,sound_writemem,0,0,
			interrupt,6	/* ??? */
		},
		{
			CPU_M68705,
			4000000/2,	/* xtal is 4MHz (????) I think it's divided by 2 internally */
			4,
			m68705_readmem,m68705_writemem,0,0,
			mexico86_m68705_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 256,
	mexico86_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	mexico86_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mexico86_rom )
	ROM_REGION(0x28000)	 /* 196k for code */
	ROM_LOAD( "2_g.bin",	0x00000, 0x08000, 0x2bbfe0fb ) /* 1st half, main code		 */
	ROM_CONTINUE(			0x20000, 0x08000 )			   /* 2nd half, banked at 0x8000 */
	ROM_LOAD( "1_f.bin",	0x10000, 0x10000, 0x0b93e68e ) /* banked at 0x8000			 */

	ROM_REGION_DISPOSE(0x30000)	 /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7_a.bin",	0x00000, 0x08000, 0x245036b1 )
	ROM_LOAD( "6_b.bin",	0x08000, 0x10000, 0xa4607989 )
	ROM_LOAD( "5_c.bin",	0x18000, 0x08000, 0xe42fa143 )
	ROM_LOAD( "4_d.bin",	0x20000, 0x10000, 0x57cfdbca )

	ROM_REGION(0x0300)	/* color PROMs */
	ROM_LOAD( "prom2.bin",  0x0000, 0x0100, 0x14f6c28d )
	ROM_LOAD( "prom3.bin",  0x0100, 0x0100, 0x3e953444 )
	ROM_LOAD( "prom1.bin",  0x0200, 0x0100, 0xbe6eb1f0 )

	ROM_REGION(0x10000)	 /* 64k for the audio cpu */
	ROM_LOAD( "3_e.bin",	0x0000, 0x8000, 0x1625b587 )

	ROM_REGION(0x0800)	/* 2k for the microcontroller */
	ROM_LOAD( "68_h.bin",   0x0000, 0x0800, 0xff92f816 )
ROM_END

ROM_START( kicknrun_rom )
	ROM_REGION(0x28000)	 /* 196k for code */
	ROM_LOAD( "a87-08.bin", 0x00000, 0x08000, 0x715e1b04 ) /* 1st half, main code		 */
	ROM_CONTINUE(			0x20000, 0x08000 )			   /* 2nd half, banked at 0x8000 */
	ROM_LOAD( "a87-07.bin", 0x10000, 0x10000, 0x6cb6ebfe ) /* banked at 0x8000			 */

	ROM_REGION_DISPOSE(0x30000)	 /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a87-02.bin", 0x00000, 0x08000, 0x64f1a85f )
	ROM_LOAD( "a87-03.bin", 0x08000, 0x10000, 0xf42e8a88 )
	ROM_LOAD( "a87-04.bin", 0x18000, 0x08000, 0x8b438d20 )
	ROM_LOAD( "a87-05.bin", 0x20000, 0x10000, 0x4eee3a8a )

	ROM_REGION(0x0300)	/* color PROMs */
	ROM_LOAD( "prom2.bin",  0x0000, 0x0100, 0x14f6c28d )
	ROM_LOAD( "prom3.bin",  0x0100, 0x0100, 0x3e953444 )
	ROM_LOAD( "prom1.bin",  0x0200, 0x0100, 0xbe6eb1f0 )

	ROM_REGION(0x10000)	 /* 64k for the audio cpu */
	ROM_LOAD( "3_e.bin",	0x0000, 0x8000, 0x1625b587 )

	ROM_REGION(0x0800)	/* 2k for the microcontroller */
	ROM_LOAD( "68_h.bin",   0x0000, 0x0800, 0xff92f816 )	/* from mexico '86, likely wrong */
ROM_END



struct GameDriver mexico86_driver =
{
	__FILE__,
	0,
	"mexico86",
	"Mexico '86",
	"1987",
	"Taito",
	"Ernesto Corvi",
	0,
	&machine_driver,
	0,

	mexico86_rom,
	0, 0,
	0,
	0,	  /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver kicknrun_driver =
{
	__FILE__,
	&mexico86_driver,
	"kicknrun",
	"Kick & Run",
	"1987",
	"Taito",
	"Ernesto Corvi",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	kicknrun_rom,
	0, 0,
	0,
	0,	  /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
