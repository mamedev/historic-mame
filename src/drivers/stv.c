/* Sega ST-V (Sega Titan Video)

built to run the rom test mode only, don't consider anything here too accurate ;-)
we only run 1 sh2, not both, vidhrdw is just made to display bios text, interrupts
are mostly not done, most smpc commands not done, no scu / dsp stuff, no sound, you
get the idea ;-) 40ghz pc recommended once driver is finished

any rom which has a non-plain loaded rom at 0x2200000 (or 0x2000000, i think it
recognises a cart at either) appears to fail its self check, reason unknown, the roms
are almost certainly not bad its a mystery.

this hardware comes above hell on the great list of hellish things as far as emulation goes anyway ;-)

*/

#include "driver.h"

static data8_t *smpc_ram;
static void stv_dump_ram(void);

static data32_t* stv_workram_l;
static data32_t* stv_workram_h;
static data32_t* stv_scu;
static data32_t* stv_vram;
static data32_t* stv_cram;

static char stv_8x8x4_dirty[0x4000];


VIDEO_START(stv)
{
	return 0;
}


/* a complete hack just to display the text graphics used in the test mode */
VIDEO_UPDATE(stv)
{

//	int i;
	data8_t *txtile_gfxregion = memory_region(REGION_GFX1);


//	for (i = 0; i < 0x2000; i++)
//		decodechar(Machine->gfx[1], i, (data8_t*)txtile_gfxregion, Machine->drv->gfxdecodeinfo[1].gfxlayout);


	if ( keyboard_pressed_memory(KEYCODE_W) ) stv_dump_ram();

	// bios text

	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){

				int address;
				int data1;
				int data2;

				address = 0x62000/4;

				address += (x + y*32);

				data1=(stv_vram[address] & 0x00ff0000) >> 16;
				data2=(stv_vram[address] & 0x000000ff) >> 0;
				data1+= 0x3000;
				data2+= 0x3000;

				if (stv_8x8x4_dirty[data1])
				{
					stv_8x8x4_dirty[data1] = 0;
					decodechar(Machine->gfx[0], data1, (data8_t*)txtile_gfxregion, Machine->drv->gfxdecodeinfo[0].gfxlayout);
				}

				if (stv_8x8x4_dirty[data2])
				{
					stv_8x8x4_dirty[data2] = 0;
					decodechar(Machine->gfx[0], data2, (data8_t*)txtile_gfxregion, Machine->drv->gfxdecodeinfo[0].gfxlayout);
				}

				drawgfx(bitmap,Machine->gfx[0],data1,0x20,0,0,x*16,y*8,cliprect,TRANSPARENCY_NONE,0);
				drawgfx(bitmap,Machine->gfx[0],data2,0x20,0,0,x*16+8,y*8,cliprect,TRANSPARENCY_NONE,0);




			}

		}


	}
/* // stv logo
	{
		int x,y;

		for (y = 0; y < 128; y++){
			for (x = 0; x < 32; x++){

				int address;
				int data1;
				int data2;

				address = 0x40000/4;

				address += (x + y*32);

				data1=(stv_vram[address] & 0x0fff0000) >> 16;
				data2=(stv_vram[address] & 0x00000fff) >> 0;

				drawgfx(bitmap,Machine->gfx[1],data1/2,0,0,0,x*16,y*8,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],data2/2,0,0,0,x*16+8,y*8,cliprect,TRANSPARENCY_PEN,0);




			}

		}


	}
*/
}

/* SMPC
 System Manager and Peripheral Control

*/


	/* 0x00000000, 0x0007ffff  BIOS ROM */
														/* 0x00080000, 0x000fffff  Unused?  */
	/* 0x00100000, 0x0010007f  SMPC     */
														/* 0x00100080, 0x0017ffff  Unused?  */
	/* 0x00180000, 0x0018ffff  Back Up Ram? */
														/* 0x00190000, 0x001fffff  Unused?  */
	/* 0x00200000, 0x002fffff  Work Ram-L */
														/* 0x00300000, 0x00ffffff  Unused?  */
	/* 0x01000000, 0x01000003  MINIT */
														/* 0x01000004, 0x017fffff  Unused?  */
	/* 0x01800000, 0x01800003  SINIT */
														/* 0x01800004, 0x01ffffff  Unused? */
	/* 0x02000000  0x03ffffff  A-BUS CS0 */
	/* 0x04000000, 0x04ffffff  A-BUS CS1 */
	/* 0x05000000, 0x057fffff  A-BUS DUMMY */
	/* 0x05800000, 0x058fffff  A-BUS CS2 */
														/* 0x05900000, 0x059fffff  Unused? */
	/* 0x05a00000, 0x05b00ee3  Sound Region */
														/* 0x05b00ee4  0x05bfffff  Unused? */
	/* 0x05c00000, 0x05cbffff  VDP 1 */
														/* 0x05cc0000, 0x05cfffff  Unused? */
	/* 0x05d00000, 0x05d00017  VDP 1 */
														/* 0x05d00018, 0x05dfffff  Unused? */
	/* 0x05e00000, 0x05e7ffff  VDP2 */
														/* 0x05e80000, 0x05efffff  Unused? */
	/* 0x05f00000, 0x05f00fff  VDP2 */
														/* 0x05f01000, 0x05f7ffff  Unused? */
	/* 0x05f80000  0x05f8011f  VDP2 */
														/* 0x05f80120  0x05fdffff  Unused? */
	/* 0x05fe0000, 0x05fe00cf  SCU */
														/* 0x05fe00d0  0x05ffffff  Unused? */
	/* 0x06000000, 0x060fffff  Work Ram H */
														/* 0x06100000, 0x07ffffff  Unused? */
/* SMPC Addresses

00
01 -w  Input Register 0
02
03 -w  Input Register 1
04
05 -w  Input Register 2
06
07 -w  Input Register 3
08
09 -w  Input Register 4
0a
0b -w  Input Register 5
0c
0d -w  Input Register 6
0e
0f
10
11
12
13
14
15
16
17
18
19
1a
1b
1c
1d
1e
1f -w  Command Register
20
21 r-  Output Register 0
22
23 r-  Output Register 1
24
25 r-  Output Register 2
26
27 r-  Output Register 3
28
29 r-  Output Register 4
2a
2b r-  Output Register 5
2c
2d r-  Output Register 6
2e
2f r-  Output Register 7
30
31 r-  Output Register 8
32
33 r-  Output Register 9
34
35 r-  Output Register 10
36
37 r-  Output Register 11
38
39 r-  Output Register 12
3a
3b r-  Output Register 13
3c
3d r-  Output Register 14
3e
3f r-  Output Register 15
40
41 r-  Output Register 16
42
43 r-  Output Register 17
44
45 r-  Output Register 18
46
47 r-  Output Register 19
48
49 r-  Output Register 20
4a
4b r-  Output Register 21
4c
4d r-  Output Register 22
4e
4f r-  Output Register 23
50
51 r-  Output Register 24
52
53 r-  Output Register 25
54
55 r-  Output Register 26
56
57 r-  Output Register 27
58
59 r-  Output Register 28
5a
5b r-  Output Register 29
5c
5d r-  Output Register 30
5e
5f r-  Output Register 31
60
61 r-  SR
62
63 rw  SF
64
65
66
67
68
69
6a
6b
6c
6d
6e
6f
70
71
72
73
74
75
76
77
78
79
7a
7b
7c
7d
7e
7f

*/


static UINT8 stv_SMPC_r8 (int offset)
{
//	logerror ("8-bit SMPC Read from Offset %02x Returns %02x\n", offset, smpc_ram[offset]);

	if (offset == 0x77)
	{
		return readinputport(0);
	}

	return smpc_ram[offset];
}

static void stv_SMPC_w8 (int offset, UINT8 data)
{
//	logerror ("8-bit SMPC Write to Offset %02x with Data %02x\n", offset, data);
	smpc_ram[offset] = data;

	if (offset == 0x1f)
	{
		switch (data)
		{
			case 0x03:
				logerror ("SMPC: Slave OFF\n");
				smpc_ram[0x5f]=0x03;
				break;
			case 0x0d:
				logerror ("SMPC: System Reset\n");
				smpc_ram[0x5f]=0x0d;
				cpu_set_reset_line(0, PULSE_LINE);
				break;
			case 0x0e:
				logerror ("SMPC: Change Clock to 352\n");
				smpc_ram[0x5f]=0x0e;
				cpu_set_nmi_line(0,PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			case 0x1a:
				logerror ("SMPC: Nmi DISABLE\n");
				smpc_ram[0x5f]=0x1a;
				break;
		//	default:
			//	logerror ("SMPC: Unhandled Command %02x\n",data);
		}

		// we've processed the command, clear status flag
		smpc_ram[0x63] = 0x00;

	}



}


static READ32_HANDLER ( stv_SMPC_r32 )
{
	int byte = 0;
	int readdata = 0;
	/* registers are all byte accesses, convert here */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; readdata = stv_SMPC_r8(offset+byte) << 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; readdata = stv_SMPC_r8(offset+byte) << 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; readdata = stv_SMPC_r8(offset+byte) << 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; readdata = stv_SMPC_r8(offset+byte) << 0;  }

	return readdata;
}


static WRITE32_HANDLER ( stv_SMPC_w32 )
{
	int byte = 0;
	int writedata = 0;
	/* registers are all byte accesses, convert here so we can use the data more easily later */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; writedata = data >> 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; writedata = data >> 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; writedata = data >> 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; writedata = data >> 0;  }

	writedata &= 0xff;

	offset += byte;

	stv_SMPC_w8(offset,writedata);
}

static READ32_HANDLER ( stv_vdp2_regs_r32 )
{
//	if (offset!=1) logerror ("VDP2: Read from Registers, Offset %04x\n",offset);
	// this is vblank status etc.  fake for now
	if (offset == 1){
		static int i = 0x00000000;
		i ^= 0xffffffff;

		return i;
	}
	return 0;
}



static void stv_dump_ram()
{
	FILE *fp;

	fp=fopen("workraml.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_l, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("workramh.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_h, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("scu.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_scu, 0xd0, 1, fp);
		fclose(fp);
	}
	fp=fopen("vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_vram, 0x00080000, 1, fp);
		fclose(fp);
	}
	fp=fopen("cram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_cram, 0x00080000, 1, fp);
		fclose(fp);
	}

}

static WRITE32_HANDLER ( stv_vram_w )
{
	data8_t *btiles = memory_region(REGION_GFX1);

	COMBINE_DATA (&stv_vram[offset]);

	data = stv_vram[offset];
	/* put in gfx region for easy decoding */
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_8x8x4_dirty[offset/2] = 1;

}

static INTERRUPT_GEN(stv_interrupt)
{
	/* i guess this is wrong ;-) */
	if(cpu_getiloops() == 0)	cpu_set_irq_line_and_vector(0, 0xf, HOLD_LINE, 0x40);

	else cpu_set_irq_line_and_vector(0, 0xe, HOLD_LINE, 0x41);
}

READ32_HANDLER ( stv_read_jamma )
{
	UINT32 data;

	data = (readinputport(offset*2+1) << 16) | (readinputport(offset*2+2));

	return data;

}



static MEMORY_READ32_START( stv_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM },


	{ 0x00100000, 0x0010007f, stv_SMPC_r32 },

	{ 0x00180000, 0x0018ffff, MRA32_RAM },


	{ 0x00200000, 0x002fffff, MRA32_RAM },
	{ 0x00400000, 0x0040000f, stv_read_jamma },

	{ 0x02000000, 0x057fffff, MRA32_BANK1 },

	/* Sound */
	{ 0x05a00000, 0x05b00ee3, MRA32_RAM },

	/* VDP1 */
	{ 0x05c00000, 0x05cbffff, MRA32_RAM },
	{ 0x05d00000, 0x05d0001f, MRA32_RAM },

	/* VDP2 */
	{ 0x05e00000, 0x05e7ffff, MRA32_RAM }, /* VRAM */
	{ 0x05f00000, 0x05f0ffff, MRA32_RAM }, /* CRAM */
	{ 0x05f80000, 0x05fbffff, stv_vdp2_regs_r32 }, /* REGS */

	{ 0x06000000, 0x060fffff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( stv_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM },
	{ 0x00100000, 0x0010007f, stv_SMPC_w32 },

	{ 0x00180000, 0x0018ffff, MWA32_RAM },


	{ 0x00200000, 0x002fffff, MWA32_RAM, &stv_workram_l },

	{ 0x02000000, 0x057fffff, MWA32_ROM },

	/* Sound */
	{ 0x05a00000, 0x05b00ee3, MWA32_RAM },

	/* VDP1 */
	{ 0x05c00000, 0x05cbffff, MWA32_RAM },
	{ 0x05d00000, 0x05d0001f, MWA32_RAM },

	/* VDP2 */
	{ 0x05e00000, 0x05e7ffff, stv_vram_w, &stv_vram }, /* VRAM */
	{ 0x05f00000, 0x05f0ffff, MWA32_RAM, &stv_cram }, /* CRAM */
	{ 0x05f80000, 0x05fbffff, MWA32_RAM }, /* REGS */

	{ 0x05fe0000, 0x05fe00cf, MWA32_RAM, &stv_scu },

	{ 0x06000000, 0x060fffff, MWA32_RAM, &stv_workram_h },
MEMORY_END

#define STV_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ )

INPUT_PORTS_START( stv )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "xx" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)


	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



DRIVER_INIT ( stv )
{
	unsigned char *ROM = memory_region(REGION_USER1);
	cpu_setbank(1,&ROM[0x000000]);

	smpc_ram = auto_malloc (0x80);

}

static struct GfxLayout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tiles8x8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4,5,6,7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX1, 0, &tiles8x8x8_layout,   0x00, 0x100  },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( stv )

	/* basic machine hardware */
	MDRV_CPU_ADD(SH2, 28000000) // 28MHz
	MDRV_CPU_MEMORY(stv_readmem,stv_writemem)
	MDRV_CPU_VBLANK_INT(stv_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 352-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x10000/4)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(stv)
	MDRV_VIDEO_UPDATE(stv)

MACHINE_DRIVER_END

ROM_START( stvbios )
	ROM_REGION( 0x1000000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) // jp
	ROM_LOAD16_WORD_SWAP( "mp17951a.s",     0x000000, 0x080000, 0x2672f9d8 ) // jp alt?
	ROM_LOAD16_WORD_SWAP( "mp17952a.s",     0x000000, 0x080000, 0xd1be2adf ) // us?

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* the roms marked as bad almost certainly aren't bad, theres some very weird
   mirroring going on, or maybe its meant to transfer the rom data to the region it
   tests from rearranging it a bit (dma?)

   comments merely indicate the status the rom gets in the rom check at the moment

*/

ROM_START( astrass )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20825.24",    0x0200000, 0x0100000, 0x94a9ad8f ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr20827.12",    0x0400000, 0x0400000, 0x65cabbb3 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20828.13",    0x0800000, 0x0400000, 0x3934d44a ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20829.14",    0x0c00000, 0x0400000, 0x814308c3 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20830.15",    0x1000000, 0x0400000, 0xff97fd19 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20831.16",    0x1400000, 0x0400000, 0x4408e6fb ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20826.17",    0x1800000, 0x0400000, 0xbdc4b941 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20832.18s",   0x1c00000, 0x0400000, 0xaf1b0985 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20833.19s",   0x2000000, 0x0400000, 0xcb6af231 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( bakubaku )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17969.13",    0x0200000, 0x0100000, 0xbee327e5 ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr17970.2",    0x0400000, 0x0400000, 0xbc4d6f91 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17971.3",    0x0800000, 0x0400000, 0xc780a3b3 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17972.4",    0x0c00000, 0x0400000, 0x8f29815a ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17973.5",    0x1000000, 0x0400000, 0x5f6e0e8b ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( colmns97 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0xc00000, REGION_USER1, 0 ) /* SH2 code */
	/* it tests .13 at 0x000000 - 0x1fffff but reports as bad even if we put the rom there */
	ROM_LOAD( "fpr19553.13",                0x200000, 0x100000, 0xd4fb6a5e ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr19554.2",     0x400000, 0x400000, 0x5a3ebcac ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19555.3",     0x800000, 0x400000, 0x74f6e6b8 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( cotton2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20122.7",    0x0200000, 0x0200000, 0xd616f78a ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20117.2",    0x0400000, 0x0400000, 0x893656ea ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20118.3",    0x0800000, 0x0400000, 0x1b6a1d4c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20119.4",    0x0c00000, 0x0400000, 0x5a76e72b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20120.5",    0x1000000, 0x0400000, 0x7113dd7b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20121.6",    0x1400000, 0x0400000, 0x8c8fd521 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20116.1",    0x1800000, 0x0400000, 0xd30b0175 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20123.8",    0x1c00000, 0x0400000, 0x35f1b89f ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( cottonbm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21075.7",    0x0200000, 0x0200000, 0x200b58ba ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21070.2",    0x0400000, 0x0400000, 0x56c0bf1d ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21071.3",    0x0800000, 0x0400000, 0x2bb18df2 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21072.4",    0x0c00000, 0x0400000, 0x7c7cb977 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21073.5",    0x1000000, 0x0400000, 0xf2e5a5b7 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21074.6",    0x1400000, 0x0400000, 0x6a7e7a7b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21069.1",    0x1800000, 0x0400000, 0x6a28e3c5 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( decathlt )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18967.13",    0x0200000, 0x0100000, 0xc0446674 ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18968.1",    0x0400000, 0x0400000, 0x11a891de ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18969.2",    0x0800000, 0x0400000, 0x199cc47d ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18970.3",    0x0c00000, 0x0400000, 0x8b7a509e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18971.4",    0x1000000, 0x0400000, 0xc87c443b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18972.5",    0x1400000, 0x0400000, 0x45c64fca ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( dnmtdeka )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19114.13",    0x0200000, 0x0100000, 0x1fd22a5f ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr19115.2",    0x0400000, 0x0400000, 0x6fe06a30 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19116.3",    0x0800000, 0x0400000, 0xaf9e627b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19117.4",    0x0c00000, 0x0400000, 0x74520ff1 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19118.5",    0x1000000, 0x0400000, 0x2c9702f0 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( ejihon )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18137.13",    0x0200000, 0x0080000, 0x151aa9bc ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18138.2",    0x0400000, 0x0400000, 0xf5567049 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18139.3",    0x0800000, 0x0400000, 0xf36b4878 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18140.4",    0x0c00000, 0x0400000, 0x228850a0 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18141.5",    0x1000000, 0x0400000, 0xb51eef36 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18142.6",    0x1400000, 0x0400000, 0xcf259541 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( elandore )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21307.11s",   0x0200000, 0x0200000, 0x966ad472 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21301.12",    0x0400000, 0x0400000, 0x1a23b0a0 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21302.13",    0x0800000, 0x0400000, 0x1c91ca33 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21303.14",    0x0c00000, 0x0400000, 0x07b2350e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21304.15",    0x1000000, 0x0400000, 0xcfea52ae ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21305.16",    0x1400000, 0x0400000, 0x46cfc2a2 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21306.17",    0x1800000, 0x0400000, 0x87a5929c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21308.18s",   0x1c00000, 0x0400000, 0x336ec1a4 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( ffreveng )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "opr21872.11s",   0x0200000, 0x0200000, 0x32d36fee  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21873.12",    0x0400000, 0x0400000, 0xdac5bd98  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21874.13",    0x0800000, 0x0400000, 0x0a7be2f1  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21875.14",    0x0c00000, 0x0400000, 0xccb75029  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21876.15",    0x1000000, 0x0400000, 0xbb92a7fc  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21877.16",    0x1400000, 0x0400000, 0xc22a4a75  ) // good
	ROM_LOAD16_WORD_SWAP( "opr21878.17",    0x1800000, 0x0200000, 0x2ea4a64d  ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* set system to 1 player to test rom */
ROM_START( fhboxers )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	/* maybe there is some banking on this one, or the roms are in the wrong places */
	ROM_REGION32_BE( 0x2800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fr18541a.13",   0x0200000, 0x0100000, 0x8c61a17c ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18533.2",    0x0400000, 0x0400000, 0x7181fe51 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18534.3",    0x0800000, 0x0400000, 0xc87ef125 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18535.4",    0x0c00000, 0x0400000, 0x929a64cf ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18536.5",    0x1000000, 0x0400000, 0x51b9f64e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18537.6",    0x1400000, 0x0400000, 0xc364f6a7 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18532.1",    0x1800000, 0x0400000, 0x39528643 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18538.7",    0x1c00000, 0x0200000, 0x7b5230c5 ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18539.8",    0x2000000, 0x0400000, 0x62b3908c ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18540.9",    0x2400000, 0x0400000, 0x4c2b59a4 ) // bad

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* set system to 1 player to test rom */
ROM_START( findlove )
	ROM_REGION( 0x340000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	/* maybe these is some banking on this one, or the roms are in the wrong places.. */
	ROM_REGION32_BE( 0x3400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20424.13",   0x0200000, 0x0100000, 0x4e61fa46 ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr20426.2",    0x0400000, 0x0400000, 0x897d1747 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20427.3",    0x0800000, 0x0400000, 0xa488a694 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20428.4",    0x0c00000, 0x0400000, 0x4353b3b6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20429.5",    0x1000000, 0x0400000, 0x4f566486 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20430.6",    0x1400000, 0x0400000, 0xd1e11979 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20431.7",    0x1800000, 0x0200000, 0xea656ced )
	ROM_LOAD16_WORD_SWAP( "mpr20425.1",    0x1c00000, 0x0400000, 0x67f104c4 )
	ROM_LOAD16_WORD_SWAP( "mpr20432.8",    0x2000000, 0x0400000, 0x79fcdecd )
	ROM_LOAD16_WORD_SWAP( "mpr20433.9",    0x2400000, 0x0400000, 0x82289f29 )
	ROM_LOAD16_WORD_SWAP( "mpr20434.10",   0x2800000, 0x0400000, 0x85c94afc )
	ROM_LOAD16_WORD_SWAP( "mpr20435.11",   0x2c00000, 0x0400000, 0x263a2e48 )
	ROM_LOAD16_WORD_SWAP( "mpr20436.12",   0x3000000, 0x0400000, 0xe3823f49 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( finlarch )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "finlarch.13",    0x0200000, 0x0100000, 0x4505fa9e ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr18257.2",    0x0400000, 0x0400000, 0x137fdf55 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18258.3",    0x0800000, 0x0400000, 0xf519c505 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18259.4",    0x0c00000, 0x0400000, 0x5cabc775 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18260.5",    0x1000000, 0x0400000, 0xf5b92082 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END


ROM_START( gaxeduel )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17766.13",    0x0200000, 0x0080000, 0xa83fcd62 ) // ic13 bad?!
	ROM_LOAD16_WORD_SWAP( "mpr17768.2",    0x0400000, 0x0400000, 0xd6808a7d ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17769.3",    0x0800000, 0x0400000, 0x3471dd35 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17770.4",    0x0c00000, 0x0400000, 0x06978a00 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17771.5",    0x1000000, 0x0400000, 0xaea2ea3b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17772.6",    0x1400000, 0x0400000, 0xb3dc0e75 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17767.1",    0x1800000, 0x0400000, 0x9ba1e7b1 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( grdforce )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20844.7",    0x0200000, 0x0200000, 0x283e7587 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20839.2",    0x0400000, 0x0400000, 0xfacd4dd8 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20840.3",    0x0800000, 0x0400000, 0xfe0158e6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20841.4",    0x0c00000, 0x0400000, 0xd87ac873 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20842.5",    0x1000000, 0x0400000, 0xbaebc506 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20843.6",    0x1400000, 0x0400000, 0x263e49cc ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( groovef )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19820.7",    0x0200000, 0x0100000, 0xe93c4513 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19815.2",    0x0400000, 0x0400000, 0x1b9b14e6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19816.3",    0x0800000, 0x0400000, 0x83f5731c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19817.4",    0x0c00000, 0x0400000, 0x525bd6c7 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19818.5",    0x1000000, 0x0400000, 0x66723ba8 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19819.6",    0x1400000, 0x0400000, 0xee8c55f4 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19814.1",    0x1800000, 0x0400000, 0x8f20e9f7 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19821.8",    0x1c00000, 0x0400000, 0xf69a76e6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19822.9",    0x2000000, 0x0200000, 0x5e8c4b5f ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( hanagumi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20143.7",    0x0200000, 0x0100000, 0x7bfc38d0 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20138.2",    0x0400000, 0x0400000, 0xfdcf1046 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20139.3",    0x0800000, 0x0400000, 0x7f0140e5 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20140.4",    0x0c00000, 0x0400000, 0x2fa03852 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20141.5",    0x1000000, 0x0400000, 0x45d6d21b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20142.6",    0x1400000, 0x0400000, 0xe38561ec ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20137.1",    0x1800000, 0x0400000, 0x181d2688 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20144.8",    0x1c00000, 0x0400000, 0x235b43f6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20145.9",    0x2000000, 0x0400000, 0xaeaac7a1 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20146.10",   0x2400000, 0x0400000, 0x39bab9a2 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20147.11",   0x2800000, 0x0400000, 0x294ab997 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20148.12",   0x2c00000, 0x0400000, 0x5337ccb0 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( introdon )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18937.13",    0x0200000, 0x0080000, 0x1f40d766  ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18938.1",    0x1800000, 0x0400000, 0x580ecb83 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18939.2",    0x0400000, 0x0400000, 0xef95a6e6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18940.3",    0x0800000, 0x0400000, 0xcabab4cd ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18941.4",    0x0c00000, 0x0400000, 0xf4a33a20 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18942.5",    0x1000000, 0x0400000, 0x8dd0a446 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18943.6",    0x1400000, 0x0400000, 0xd8702a9e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18944.7",    0x1800000, 0x0100000, 0xf7f75ce5 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* set system to 1 player to test rom */
ROM_START( kiwames )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18737.13",    0x0200000, 0x0080000, 0xcfad6c49 ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18738.2",    0x0400000, 0x0400000, 0x4b3c175a ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18739.3",    0x0800000, 0x0400000, 0xeb41fa67 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18740.4",    0x0c00000, 0x0200000, 0x9ca7962f ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( maruchan )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20416.13",    0x0200000, 0x0100000, 0x8bf0176d ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr20417.2",    0x0400000, 0x0400000, 0x636c2a08 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20418.3",    0x0800000, 0x0400000, 0x3f0d9e34 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20419.4",    0x0c00000, 0x0400000, 0xec969815 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20420.5",    0x1000000, 0x0400000, 0xf2902c88 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20421.6",    0x1400000, 0x0400000, 0xcd0b477c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20422.1",    0x1800000, 0x0400000, 0x66335049 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20423.8",    0x1c00000, 0x0400000, 0x2bd55832 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20443.9",    0x2000000, 0x0400000, 0x8ac288f5 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* set system to 1 player to test rom */
ROM_START( myfairld )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr21000.7",    0x0200000, 0x0200000, 0x2581c560 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20995.2",    0x0400000, 0x0400000, 0x1bb73f24 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20996.3",    0x0800000, 0x0400000, 0x993c3859 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20997.4",    0x0c00000, 0x0400000, 0xf0bf64a4 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20998.5",    0x1000000, 0x0400000, 0xd3b19786 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20999.6",    0x1400000, 0x0400000, 0x82e31f25 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20994.1",    0x1800000, 0x0400000, 0xa69243a0 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21001.8",    0x1c00000, 0x0400000, 0x95fbe549 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( othellos )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20967.7",    0x0200000, 0x0200000, 0xefc05b97 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20963.2",    0x0400000, 0x0400000, 0x2cc4f141 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20964.3",    0x0800000, 0x0400000, 0x5f5cda94 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20965.4",    0x0c00000, 0x0400000, 0x37044f3e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20966.5",    0x1000000, 0x0400000, 0xb94b83de ) // good


	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( pblbeach )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18852.13",    0x0200000, 0x0080000, 0xd12414ec  ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr18853.2",    0x0400000, 0x0400000, 0xb9268c97  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18854.3",    0x0800000, 0x0400000, 0x3113c8bc  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18855.4",    0x0c00000, 0x0400000, 0xdaf6ad0c  ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18856.5",    0x1000000, 0x0400000, 0x214cef24  ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( prikura )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19337.7",    0x0200000, 0x0200000, 0x76f69ff3 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19333.2",    0x0400000, 0x0400000, 0xeb57a6a6 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19334.3",    0x0800000, 0x0400000, 0xc9979981 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19335.4",    0x0c00000, 0x0400000, 0x9e000140 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19336.5",    0x1000000, 0x0400000, 0x2363fa4b ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( puyosun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr19531.13",    0x0200000, 0x0080000, 0xac81024f ) // bad
	ROM_LOAD16_WORD_SWAP( "mpr19533.2",    0x0400000, 0x0400000, 0x17ec54ba ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19534.3",    0x0800000, 0x0400000, 0x820e4781 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19535.4",    0x0c00000, 0x0400000, 0x94fadfa4 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19536.5",    0x1000000, 0x0400000, 0x5765bc9c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19537.6",    0x1400000, 0x0400000, 0x8b736686 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19532.1",    0x1800000, 0x0400000, 0x985f0c9d ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19538.8",    0x1c00000, 0x0400000, 0x915a723e ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19539.9",    0x2000000, 0x0400000, 0x72a297e5 ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( rsgun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr20958.11s",   0x0200000, 0x0200000, 0xcbe5a449 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20959.12",    0x0400000, 0x0400000, 0xa953330b ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20960.13",    0x0800000, 0x0400000, 0xb5ab9053 ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20961.14",    0x0c00000, 0x0400000, 0x0e06295c ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20962.15",    0x1000000, 0x0400000, 0xf1e6c7fc ) // good

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

/* this one must be a strange mapping .. all roms fail */
ROM_START( sandor )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "sando-r.13",    0x0200000, 0x0100000, 0xfe63a239 )
	ROM_LOAD16_WORD_SWAP( "mpr18635.8",   0x0400000, 0x0400000, 0x441e1368 )
	ROM_LOAD16_WORD_SWAP( "mpr18636.9",   0x0800000, 0x0400000, 0xfff1dd80 )
	ROM_LOAD16_WORD_SWAP( "mpr18637.10",  0x0c00000, 0x0400000, 0x83aced0f )
	ROM_LOAD16_WORD_SWAP( "mpr18638.11",  0x1000000, 0x0400000, 0xcaab531b )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END


/************** beyond this point I haven't even tried to check the roms *************/

ROM_START( sassisu )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20542.13",    0x0200000, 0x0100000, 0x0e632db5 )
	ROM_LOAD16_WORD_SWAP( "mpr20544.2",    0x0400000, 0x0400000, 0x661fff5e )
	ROM_LOAD16_WORD_SWAP( "mpr20545.3",    0x0800000, 0x0400000, 0x8e3a37be )
	ROM_LOAD16_WORD_SWAP( "mpr20546.4",    0x0c00000, 0x0400000, 0x72020886 )
	ROM_LOAD16_WORD_SWAP( "mpr20547.5",    0x1000000, 0x0400000, 0x8362e397 )
	ROM_LOAD16_WORD_SWAP( "mpr20548.6",    0x1400000, 0x0400000, 0xe37534d9 )
	ROM_LOAD16_WORD_SWAP( "mpr20543.1",    0x1800000, 0x0400000, 0x1f688cdf )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( seabass )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "seabassf.13",    0x0200000, 0x0100000, 0x6d7c39cc )
	ROM_LOAD16_WORD_SWAP( "mpr20551.2",    0x0400000, 0x0400000, 0x9a0c6dd8 )
	ROM_LOAD16_WORD_SWAP( "mpr20552.3",    0x0800000, 0x0400000, 0x5f46b0aa )
	ROM_LOAD16_WORD_SWAP( "mpr20553.4",    0x0c00000, 0x0400000, 0xc0f8a6b6 )
	ROM_LOAD16_WORD_SWAP( "mpr20554.5",    0x1000000, 0x0400000, 0x215fc1f9 )
	ROM_LOAD16_WORD_SWAP( "mpr20555.6",    0x1400000, 0x0400000, 0x3f5186a9 )
	ROM_LOAD16_WORD_SWAP( "mpr20550.1",    0x1800000, 0x0400000, 0x083e1ca8 )
	ROM_LOAD16_WORD_SWAP( "mpr20556.8",    0x1c00000, 0x0400000, 0x1fd70c6c )
	ROM_LOAD16_WORD_SWAP( "mpr20557.9",    0x2000000, 0x0400000, 0x3c9ba442 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( shanhigw )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x0800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr18341.7",    0x0200000, 0x0200000, 0xcc5e8646 )
	ROM_LOAD16_WORD_SWAP( "mpr18340.2",    0x0400000, 0x0200000, 0x8db23212 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( shienryu )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x0c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19631.7",    0x0200000, 0x0200000, 0x3a4b1abc )
	ROM_LOAD16_WORD_SWAP( "mpr19632.2",    0x0400000, 0x0200000, 0x985fae46 )
	ROM_LOAD16_WORD_SWAP( "mpr19633.3",    0x0800000, 0x0200000, 0xe2f0b037 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( sleague )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18777.13",    0x0200000, 0x0080000, 0x8d180866 )
	ROM_LOAD16_WORD_SWAP( "mpr18778.8",    0x0400000, 0x0400000, 0x25e1300e )
	ROM_LOAD16_WORD_SWAP( "mpr18779.9",    0x0800000, 0x0400000, 0x51e2fabd )
	ROM_LOAD16_WORD_SWAP( "mpr18780.10",   0x0c00000, 0x0400000, 0x8cd4dd74 )
	ROM_LOAD16_WORD_SWAP( "mpr18781.11",   0x1000000, 0x0400000, 0x13ee41ae )
	ROM_LOAD16_WORD_SWAP( "mpr18782.12",   0x1400000, 0x0200000, 0x9be2270a )


	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( sokyugrt )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19188.13",    0x0200000, 0x0080000, 0x45a27e32  )
	ROM_LOAD16_WORD_SWAP( "mpr19189.2",    0x0400000, 0x0400000, 0x0b202a3e )
	ROM_LOAD16_WORD_SWAP( "mpr19190.3",    0x0800000, 0x0400000, 0x1777ded8 )
	ROM_LOAD16_WORD_SWAP( "mpr19191.4",    0x0c00000, 0x0400000, 0xec6eb07b )
	ROM_LOAD16_WORD_SWAP( "mpr19192.5",    0x1000000, 0x0400000, 0xcb544a1e )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( sss )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr21488.24",    0x0200000, 0x0080000, 0x71c9def1 )
	ROM_LOAD16_WORD_SWAP( "mpr21489.12",    0x0400000, 0x0400000, 0x4c85152b )
	ROM_LOAD16_WORD_SWAP( "mpr21490.13",    0x0800000, 0x0400000, 0x03da67f8 )
	ROM_LOAD16_WORD_SWAP( "mpr21491.14",    0x0c00000, 0x0400000, 0xcf7ee784 )
	ROM_LOAD16_WORD_SWAP( "mpr21492.15",    0x1000000, 0x0400000, 0x57753894 )
	ROM_LOAD16_WORD_SWAP( "mpr21493.16",    0x1400000, 0x0400000, 0xefb2d271 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( suikoenb )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17834.13",    0x0200000, 0x0100000, 0x746ef686  )
	ROM_LOAD16_WORD_SWAP( "mpr17836.2",    0x0400000, 0x0400000, 0x55e9642d )
	ROM_LOAD16_WORD_SWAP( "mpr17837.3",    0x0800000, 0x0400000, 0x13d1e667 )
	ROM_LOAD16_WORD_SWAP( "mpr17838.4",    0x0c00000, 0x0400000, 0xf9e70032 )
	ROM_LOAD16_WORD_SWAP( "mpr17839.5",    0x1000000, 0x0400000, 0x1b2762c5 )
	ROM_LOAD16_WORD_SWAP( "mpr17840.6",    0x1400000, 0x0400000, 0x0fd4c857 )
	ROM_LOAD16_WORD_SWAP( "mpr17835.1",    0x1800000, 0x0400000, 0x77f5cb43 )
	ROM_LOAD16_WORD_SWAP( "mpr17841.8",    0x1c00000, 0x0400000, 0xf48beffc )
	ROM_LOAD16_WORD_SWAP( "mpr17842.9",    0x2000000, 0x0400000, 0xac8deed7 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( twcup98 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1400000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20819.24",    0x0200000, 0x0100000, 0xd930dfc8 )
	ROM_LOAD16_WORD_SWAP( "mpr20821.12",    0x0400000, 0x0400000, 0x2d930d23 )
	ROM_LOAD16_WORD_SWAP( "mpr20822.13",    0x0800000, 0x0400000, 0x8b33a5e2 )
	ROM_LOAD16_WORD_SWAP( "mpr20823.14",    0x0c00000, 0x0400000, 0x6e6d4e95 )
	ROM_LOAD16_WORD_SWAP( "mpr20824.15",    0x1000000, 0x0400000, 0x4cf18a25 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( vfkids )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr18914.13",    0x0200000, 0x0100000, 0xcd35730a )
	ROM_LOAD16_WORD_SWAP( "mpr18915.1",    0x0400000, 0x0400000, 0x09cc38e5 )
	ROM_LOAD16_WORD_SWAP( "mpr18916.4",    0x0800000, 0x0400000, 0x4aae3ddb )
	ROM_LOAD16_WORD_SWAP( "mpr18917.5",    0x0c00000, 0x0400000, 0xedf6edc3 )
	ROM_LOAD16_WORD_SWAP( "mpr18918.6",    0x1000000, 0x0400000, 0xd3a95036 )
	ROM_LOAD16_WORD_SWAP( "mpr18919.8",    0x1400000, 0x0400000, 0x4ac700de )
	ROM_LOAD16_WORD_SWAP( "mpr18920.9",    0x1800000, 0x0400000, 0x0106e36c )
	ROM_LOAD16_WORD_SWAP( "mpr18921.10",   0x1c00000, 0x0400000, 0xc23d51ad )
	ROM_LOAD16_WORD_SWAP( "mpr18922.11",   0x2000000, 0x0400000, 0x99d0ab90 )
	ROM_LOAD16_WORD_SWAP( "mpr18923.12",   0x2400000, 0x0400000, 0x30a41ae9 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( vfremix )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x1c00000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17944.13",    0x0200000, 0x0100000, 0xa5bdc560  )
	ROM_LOAD16_WORD_SWAP( "mpr17946.2",    0x0400000, 0x0400000, 0x4cb245f7 )
	ROM_LOAD16_WORD_SWAP( "mpr17947.3",    0x0800000, 0x0400000, 0xfef4a9fb )
	ROM_LOAD16_WORD_SWAP( "mpr17948.4",    0x0c00000, 0x0400000, 0x3e2b251a )
	ROM_LOAD16_WORD_SWAP( "mpr17949.5",    0x1000000, 0x0400000, 0xb2ecea25 )
	ROM_LOAD16_WORD_SWAP( "mpr17950.6",    0x1400000, 0x0400000, 0x5b1f981d )
	ROM_LOAD16_WORD_SWAP( "mpr17945.1",    0x1800000, 0x0200000, 0x03ede188 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( vmahjong )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "mpr19620.7",    0x0200000, 0x0200000, 0xc98de7e5 )
	ROM_LOAD16_WORD_SWAP( "mpr19615.2",    0x0400000, 0x0400000, 0xc62896da )
	ROM_LOAD16_WORD_SWAP( "mpr19616.3",    0x0800000, 0x0400000, 0xf62207c7 )
	ROM_LOAD16_WORD_SWAP( "mpr19617.4",    0x0c00000, 0x0400000, 0xab667e19 )
	ROM_LOAD16_WORD_SWAP( "mpr19618.5",    0x1000000, 0x0400000, 0x9782ceee )
	ROM_LOAD16_WORD_SWAP( "mpr19619.6",    0x1400000, 0x0400000, 0x0b76866c )
	ROM_LOAD16_WORD_SWAP( "mpr19614.1",    0x1800000, 0x0400000, 0xb83b3f03 )
	ROM_LOAD16_WORD_SWAP( "mpr19621.8",    0x1c00000, 0x0400000, 0xf92616b3 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( winterht )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr20108.13",    0x0200000, 0x0100000, 0x1ef9ced0 )
	ROM_LOAD16_WORD_SWAP( "mpr20110.2",    0x0400000, 0x0400000, 0x238ef832 )
	ROM_LOAD16_WORD_SWAP( "mpr20111.3",    0x0800000, 0x0400000, 0xb0a86f69 )
	ROM_LOAD16_WORD_SWAP( "mpr20112.4",    0x0c00000, 0x0400000, 0x3ba2b49b )
	ROM_LOAD16_WORD_SWAP( "mpr20113.5",    0x1000000, 0x0400000, 0x8c858b41 )
	ROM_LOAD16_WORD_SWAP( "mpr20114.6",    0x1400000, 0x0400000, 0xb723862c )
	ROM_LOAD16_WORD_SWAP( "mpr20109.1",    0x1800000, 0x0200000, 0xc1a713b8 )
	ROM_LOAD16_WORD_SWAP( "mpr20115.8",    0x1c00000, 0x0200000, 0xdd01f2ad )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

ROM_START( znpwfv )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr19730.ic8",   0x000000, 0x080000, 0xd0e0889d ) /* bios */

	ROM_REGION32_BE( 0x2800000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20398.13",    0x0200000, 0x0100000, 0x3fb56a0b )
	ROM_LOAD16_WORD_SWAP( "mpr20400.2",    0x0400000, 0x0400000, 0x1edfbe05 )
	ROM_LOAD16_WORD_SWAP( "mpr20401.3",    0x0800000, 0x0400000, 0x99e98937 )
	ROM_LOAD16_WORD_SWAP( "mpr20402.4",    0x0c00000, 0x0400000, 0x4572aa60 )
	ROM_LOAD16_WORD_SWAP( "mpr20403.5",    0x1000000, 0x0400000, 0x26a8e13e )
	ROM_LOAD16_WORD_SWAP( "mpr20404.6",    0x1400000, 0x0400000, 0x0b70275d )
	ROM_LOAD16_WORD_SWAP( "mpr20399.1",    0x1800000, 0x0200000, 0xc178a96e )
	ROM_LOAD16_WORD_SWAP( "mpr20405.8",    0x1c00000, 0x0200000, 0xf53337b7 )
	ROM_LOAD16_WORD_SWAP( "mpr20406.9",    0x2000000, 0x0200000, 0xb677c175 )
	ROM_LOAD16_WORD_SWAP( "mpr20407.10",   0x2400000, 0x0200000, 0x58356050 )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* GFX */
ROM_END

GAMEX( 1996, stvbios,  0,        stv, stv,  stv,  ROT0, "Sega", "ST-V Bios", NOT_A_DRIVER )

GAMEX( 199?, astrass,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Astro SuperStars", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, bakubaku,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Baku Baku Animal", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, colmns97,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Columns 97", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, cotton2,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Cotton 2", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, cottonbm,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Cotton Boomerang", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, decathlt,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Decathlete", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, dnmtdeka,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Dynamite Deka", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, ejihon,    stvbios, stv, stv,  stv,  ROT0, "Sega", "Ejihon", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, elandore,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Elandoree", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, ffreveng,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Final Fight Revenge", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, fhboxers,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Funky Head Boxers", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, findlove,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Find Love", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, finlarch,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Final Arch", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, gaxeduel,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Golden Axe - The Duel", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, grdforce,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Guardian Force", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, groovef,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Power Instinct 3 - Groove On Fight", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, hanagumi,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Hanagumi Taisen Columns - Sakura Wars", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, introdon,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Intro Don Don", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, kiwames,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Pro Mahjong Kiwame S", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, maruchan,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Maru-Chan de Goo!", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, myfairld,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Virtual Mahjong 2 - My Fair Lady", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, othellos,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Othello Shiyouyo", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, pblbeach,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Pebble Beach - The Great Shot", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, prikura,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Purikura Sakusen", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, puyosun,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Puyo Puyo Sun", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, rsgun,     stvbios, stv, stv,  stv,  ROT0, "Sega", "Radiant Silvergun", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, sandor,    stvbios, stv, stv,  stv,  ROT0, "Sega", "Sando-R", GAME_NO_SOUND | GAME_NOT_WORKING )
/* below this point i've not even looked at the rom mappings */
GAMEX( 199?, sassisu,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Taisen Tanto-R 'Sasshissu!'", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, seabass,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Sea Bass Fishing", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, shanhigw,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Shanghai - The Great Wall", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, shienryu,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Shienryu", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, sleague,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Super League ", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, sokyugrt,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Soukyugurentai/Terra Diver", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, sss,       stvbios, stv, stv,  stv,  ROT0, "Sega", "Steep Slope Sliders", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, suikoenb,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Suikoenbu", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, twcup98,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Tecmo World Cup '98", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, vfkids,    stvbios, stv, stv,  stv,  ROT0, "Sega", "Virtua Fighter Kids", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, vfremix,   stvbios, stv, stv,  stv,  ROT0, "Sega", "Virtua Fighter Remix", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, vmahjong,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Virtual Mahjong", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, winterht,  stvbios, stv, stv,  stv,  ROT0, "Sega", "Winter Heat", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, znpwfv,    stvbios, stv, stv,  stv,  ROT0, "Sega", "Zen Nippon Pro-Wrestling Featuring Virtua", GAME_NO_SOUND | GAME_NOT_WORKING )

/* there are probably a bunch of other games (batman forever? with extra sound board, some fishing games with cd-rom etc.) */
