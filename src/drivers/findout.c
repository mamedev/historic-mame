/***************************************************************************

Find Out    (c) 1987

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"



VIDEO_UPDATE( findout )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}


static data8_t drawctrl[3];

static WRITE_HANDLER( findout_drawctrl_w )
{
	drawctrl[offset] = data;
}

static WRITE_HANDLER( findout_bitmap_w )
{
	int sx,sy;
	int fg,bg,mask,bits;
	static int prevoffset, yadd;

	videoram[offset] = data;

	yadd = (offset==prevoffset) ? (yadd+1):0;
	prevoffset = offset;

	fg = drawctrl[0] & 7;
	bg = 2;
	mask = 0xff;//drawctrl[2];
	bits = drawctrl[1];

	sx = 8 * (offset % 64);
	sy = offset / 64;
	sy = (sy + yadd) & 0xff;

//if (mask != bits)
//	usrintf_showmessage("color %02x bits %02x mask %02x\n",fg,bits,mask);

	if (mask & 0x80) plot_pixel(tmpbitmap,sx+0,sy,(bits & 0x80) ? fg : bg);
	if (mask & 0x40) plot_pixel(tmpbitmap,sx+1,sy,(bits & 0x40) ? fg : bg);
	if (mask & 0x20) plot_pixel(tmpbitmap,sx+2,sy,(bits & 0x20) ? fg : bg);
	if (mask & 0x10) plot_pixel(tmpbitmap,sx+3,sy,(bits & 0x10) ? fg : bg);
	if (mask & 0x08) plot_pixel(tmpbitmap,sx+4,sy,(bits & 0x08) ? fg : bg);
	if (mask & 0x04) plot_pixel(tmpbitmap,sx+5,sy,(bits & 0x04) ? fg : bg);
	if (mask & 0x02) plot_pixel(tmpbitmap,sx+6,sy,(bits & 0x02) ? fg : bg);
	if (mask & 0x01) plot_pixel(tmpbitmap,sx+7,sy,(bits & 0x01) ? fg : bg);
}


static READ_HANDLER( portC_r )
{
	return 4;
//	return (rand()&2);
}

static WRITE_HANDLER( lamps_w )
{
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);
}

static WRITE_HANDLER( sound_w )
{
	/* bit 3 used but unknown */

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);

//	logerror("%04x: sound_w %02x\n",activecpu_get_pc(),data);
//	usrintf_showmessage("%02x",data);
}

static ppi8255_interface ppi8255_intf =
{
	2, 									/* 2 chips */
	{ input_port_0_r, input_port_2_r },	/* Port A read */
	{ input_port_1_r, NULL },			/* Port B read */
	{ NULL,           portC_r },		/* Port C read */
	{ NULL,           NULL },			/* Port A write */
	{ NULL,           lamps_w },		/* Port B write */
	{ sound_w,        NULL },			/* Port C write */
};

MACHINE_INIT( findout )
{
	ppi8255_init(&ppi8255_intf);
}


static READ_HANDLER( catchall )
{
	int pc = activecpu_get_pc();

	if (pc != 0x3c74 && pc != 0x0364 && pc != 0x036d)	/* weed out spurious blit reads */
		logerror("%04x: unmapped memory read from %04x\n",pc,offset);

	return 0xff;
}

static WRITE_HANDLER( banksel_main_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x8000);
}
static WRITE_HANDLER( banksel_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE_HANDLER( banksel_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE_HANDLER( banksel_3_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE_HANDLER( banksel_4_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x28000);
}
static WRITE_HANDLER( banksel_5_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x30000);
}


/* This signature is used to validate the question ROMs. Simple protection check? */
static int signature_answer,signature_pos;

static READ_HANDLER( signature_r )
{
	return signature_answer;
}

static WRITE_HANDLER( signature_w )
{
	if (data == 0) signature_pos = 0;
	else
	{
		static data8_t signature[8] = { 0xff, 0x01, 0xfd, 0x05, 0xf5, 0x15, 0xd5, 0x55 };

		signature_answer = signature[signature_pos++];

		signature_pos &= 7;	/* safety; shouldn't happen */
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4803) AM_READ(ppi8255_0_r)
	AM_RANGE(0x5000, 0x5003) AM_READ(ppi8255_1_r)
	AM_RANGE(0x6400, 0x6400) AM_READ(signature_r)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x0000, 0xffff) AM_READ(catchall)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_WRITE(ppi8255_1_w)
	/* banked ROMs are enabled by low 6 bits of the address */
	AM_RANGE(0x603e, 0x603e) AM_WRITE(banksel_1_w)
	AM_RANGE(0x603d, 0x603d) AM_WRITE(banksel_2_w)
	AM_RANGE(0x603b, 0x603b) AM_WRITE(banksel_3_w)
	AM_RANGE(0x6037, 0x6037) AM_WRITE(banksel_4_w)
	AM_RANGE(0x602f, 0x602f) AM_WRITE(banksel_5_w)
	AM_RANGE(0x601f, 0x601f) AM_WRITE(banksel_main_w)
	AM_RANGE(0x6200, 0x6200) AM_WRITE(signature_w)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_ROM)	/* space for diagnostic ROM? */
	AM_RANGE(0x8000, 0x8002) AM_WRITE(findout_drawctrl_w)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(findout_bitmap_w)  AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)	/* overlapped by the above */
ADDRESS_MAP_END



INPUT_PORTS_START( findout )
	PORT_START
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Game Repetition" )
	PORT_DIPSETTING( 0x08, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Orientation" )
	PORT_DIPSETTING( 0x10, "Horizontal" )
	PORT_DIPSETTING( 0x00, "Vertical" )
	PORT_DIPNAME( 0x20, 0x20, "Buy Letter" )
	PORT_DIPSETTING( 0x20, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Starting Letter" )
	PORT_DIPSETTING( 0x40, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Bonus Letter" )
	PORT_DIPSETTING( 0x80, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static MACHINE_DRIVER_START( findout )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ?????? (affects sound pitch) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(findout)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(findout)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( findout )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "12.bin",       0x00000, 0x4000, CRC(21132d4c) SHA1(e3562ee2f46b3f022a852a0e0b1c8fb8164f64a3) )
	ROM_LOAD( "11.bin",       0x08000, 0x2000, CRC(0014282c) SHA1(c6792f2ff712ba3759ff009950d78750df844d01) )	/* banked */
	ROM_LOAD( "13.bin",       0x10000, 0x8000, CRC(cea91a13) SHA1(ad3b395ab0362f3decf178824b1feb10b6335bb3) )	/* banked ROMs for solution data */
	ROM_LOAD( "14.bin",       0x18000, 0x8000, CRC(2a433a40) SHA1(4132d81256db940789a40aa1162bf1b3997cb23f) )
	ROM_LOAD( "15.bin",       0x20000, 0x8000, CRC(d817b31e) SHA1(11e6e1042ee548ce2080127611ce3516a0528ae0) )
	ROM_LOAD( "16.bin",       0x28000, 0x8000, CRC(143f9ac8) SHA1(4411e8ba853d7d5c032115ce23453362ab82e9bb) )
	ROM_LOAD( "17.bin",       0x30000, 0x8000, CRC(dd743bc7) SHA1(63f7e01ac5cda76a1d3390b6b83f4429b7d3b781) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s147.bin",   0x0000, 0x0200, CRC(f3b663bb) SHA1(5a683951c8d3a2baac4b49e379d6e10e35465c8a) )	/* unknown */
ROM_END

ROM_START( getrv1 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "triv_3_2.bin", 0x00000, 0x4000,  CRC(2d72a081) SHA1(8aa32acf335d027466799b097e0de66bcf13247f) )
	ROM_LOAD( "rom_ad.bin",   0x08000, 0x2000,  CRC(c81cc847) SHA1(057b7b75a2fe1abf88b23e7b2de230d9f96139f5) )
	ROM_LOAD( "aeros836.bin",  0x10000, 0x8000, CRC(cb555d46) SHA1(559ae05160d7893ff96311a2177eba039a4cf186) )
	ROM_LOAD( "engsp838.bin",  0x18000, 0x8000, CRC(6ae8a63d) SHA1(c6018141d8bbe0ed7619980bf7da89dd91d7fcc2) )
	ROM_LOAD( "genft836.bin", 0x20000, 0x8000,  CRC(f921f108) SHA1(fd72282df5cee0e6ab55268b40785b3dc8e3d65b) )
	ROM_LOAD( "horor839.bin",  0x28000, 0x8000, CRC(5f7b262a) SHA1(047480d6bf5c6d0603d538b84c996bd226f07f77) )
	ROM_LOAD( "pop12_57.bin",  0x30000, 0x8000, CRC(884fec7c) SHA1(b389216c17f516df4e15eee46246719dd4acb587) )
ROM_END

ROM_START( getrv2 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin", 0x00000, 0x4000, CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "richfa_1.bin", 0x10000, 0x8000, CRC(39e07e4a) SHA1(6e5a0bcefaa1169f313e8818cf50919108b3e121) )
	ROM_LOAD( "rnr_9.bin",    0x18000, 0x8000, CRC(1be036b1) SHA1(0b262906044950319dd911b956ac2e0b433f6c7f) )
	ROM_LOAD( "sex_3_11.bin", 0x20000, 0x8000, CRC(2c46e355) SHA1(387ab389abaaea8e870b00039dd884237f7dd9c6) )
	ROM_LOAD( "sprt_8.bin",   0x28000, 0x8000, CRC(6bd1ba9a) SHA1(7caac1bd438a9b1d11fb33e11814b5d76951211a) )
	ROM_LOAD( "tv_comd.bin",  0x30000, 0x8000, CRC(992ae38e) SHA1(312780d651a85a1c433f587ff2ede579456d3fd9) )
ROM_END

ROM_START( getrv3 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin", 0x00000, 0x4000,  CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "ent_10.bin",   0x10000, 0x8000,  CRC(07068c9f) SHA1(1aedc78d071281ec8b08488cd82655d41a77cf6b) )
ROM_END

ROM_START( getrv4 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "trvmast3.bin",  0x00000, 0x4000, CRC(9ea277bc) SHA1(f813de572a3b5e2ad601dc6559dc93c30bb7a38c) )
	ROM_LOAD( "comic416.bin",  0x10000, 0x8000, CRC(193c868e) SHA1(75f18eb6cc6467e927961271374bedaf7736e20b) )
	ROM_LOAD( "gnrl3_42.bin",  0x18000, 0x8000, CRC(b0376464) SHA1(d1812d6dd42cd7f3753fcc0d2647b56a91bfcc9d) )
	ROM_LOAD( "scfi2_31.bin",  0x20000, 0x8000, CRC(3c8bc603) SHA1(5e8c1d27fc8ffb9a2a26b749328e09c548da4fca) )
	ROM_LOAD( "soccr_41.bin",  0x28000, 0x8000, CRC(f821f860) SHA1(b0437ef5d31c507c6499c1fb732d2ba3b9beb151) )
	ROM_LOAD( "sprt5_13.bin",  0x30000, 0x8000, CRC(02712bc7) SHA1(a07a87c2c37f34a188d7c00acc0d977acc850f00) )
ROM_END

GAMEX( 1987, findout, 0, findout, findout, 0, ROT0, "Elettronolo", "Find Out", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

/*

When game is first run a RAM error will occur because the nvram needs initialising.

GETriv1	- Press F2 to get into test mode, then press control/fire1 to continue
GETriv2-3 - When ERROR appears, press F2 , then F3 to reset, then F2 again and the game will start
GETriv4 - When RAM Error appears press F3 to reset and the game will start

*/


GAMEX( 1986, getrv1, 0, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia 1 (UK Version 5.07)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1984, getrv2, 0, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia 2 (Version 1.03a)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1984, getrv3, 0, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia 3 (Version 1.03a)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1986, getrv4, 0, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia 4 (Version 1.03)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )


