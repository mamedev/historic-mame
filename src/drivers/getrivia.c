/*

Trivia By Greyhound Electronics

  driver by Pierpaolo Prazzoli and graphic fixes by Tomasz Slanina
  based on Find Out driver

ROM BOARD (2764 ROMs)

FILE            CS
TRIVB1.BIN      D900
TRIVB2.BIN      4600

ROM BOARD (27128 ROMs)

FILE            CS
SPRT1_9.BIN     8600
GENERAL5.BIN    A900
ENTR2_9.BIN     B300
COMC2_9.BIN     2800
HCKY5_9.BIN     3700

ROM board has a part # UVM10B  1984
Main board has a part # UV-1B Rev 5 1982-83-84

Processor: Z80 
Support Chips:(2) 8255s
RAM: 6117on ROM board and (24) MCM4517s on main board

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"

static data8_t *drawctrl;

static WRITE8_HANDLER( getrivia_bitmap_w )
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

static WRITE8_HANDLER( lamps_w )
{
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);
}

static WRITE8_HANDLER( sound_w )
{
	/* bit 3 used but unknown */

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);

//	logerror("%04x: sound_w %02x\n",activecpu_get_pc(),data);
//	usrintf_showmessage("%02x",data);
}

static WRITE8_HANDLER( banksel_1_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE8_HANDLER( banksel_2_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x14000);
}
static WRITE8_HANDLER( banksel_3_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE8_HANDLER( banksel_4_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x1c000);
}
static WRITE8_HANDLER( banksel_5_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE8_HANDLER( banksel_1_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x12000);
}
static WRITE8_HANDLER( banksel_2_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x16000);
}
static WRITE8_HANDLER( banksel_3_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x1a000);
}
static WRITE8_HANDLER( banksel_4_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x1e000);
}
static WRITE8_HANDLER( banksel_5_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x22000);
}

static ADDRESS_MAP_START( getrivia_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)	
	AM_RANGE(0x600f, 0x600f) AM_WRITE(banksel_5_1_w)
	AM_RANGE(0x6017, 0x6017) AM_WRITE(banksel_4_1_w)
	AM_RANGE(0x601b, 0x601b) AM_WRITE(banksel_3_1_w)
	AM_RANGE(0x601d, 0x601d) AM_WRITE(banksel_2_1_w)
	AM_RANGE(0x601e, 0x601e) AM_WRITE(banksel_1_1_w)
	AM_RANGE(0x608f, 0x608f) AM_WRITE(banksel_5_2_w)
	AM_RANGE(0x6097, 0x6097) AM_WRITE(banksel_4_2_w)
	AM_RANGE(0x609b, 0x609b) AM_WRITE(banksel_3_2_w)
	AM_RANGE(0x609d, 0x609d) AM_WRITE(banksel_2_2_w)
	AM_RANGE(0x609e, 0x609e) AM_WRITE(banksel_1_2_w)
	AM_RANGE(0x8000, 0x8002) AM_WRITE(MWA8_RAM) AM_BASE(&drawctrl)
	AM_RANGE(0x8000, 0x9fff) AM_ROM /* space for diagnostic ROM? */
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_RAM, getrivia_bitmap_w) AM_BASE(&videoram)
ADDRESS_MAP_END

INPUT_PORTS_START( getrivia )
	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Questions" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x01, "5" )
//	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Show Answer" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Max Coins" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPNAME( 0x10, 0x10, "Timeout" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Tickets" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "No Coins" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

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
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1, 2 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2, 2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4, 2 )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5, 2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static ppi8255_interface ppi8255_intf =
{
	2, 									/* 2 chips */
	{ input_port_0_r, input_port_2_r },	/* Port A read */
	{ input_port_1_r, NULL },			/* Port B read */
	{ NULL,           NULL },			/* Port C read */
	{ NULL,           NULL },			/* Port A write */
	{ NULL,           lamps_w },		/* Port B write */
	{ sound_w,        NULL },			/* Port C write */
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static MACHINE_INIT( getrivia )
{
	ppi8255_init(&ppi8255_intf);
}

static MACHINE_DRIVER_START( getrivia )
	MDRV_CPU_ADD(Z80,4000000) /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(getrivia_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(256)

	MDRV_MACHINE_INIT(getrivia)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

ROM_START( gt102c )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog2_right", 0x00000, 0x2000, CRC(76fdc3a3) SHA1(212e09644b9cab334aad22ec5860e8638c6ba3fa) )
	ROM_LOAD( "prog2_left",  0x0a000, 0x2000, CRC(901fb2f9) SHA1(98e49c74d89c4911a1f4d5ccf3e6cf3226c6a178) )
	/* Question roms */
	ROM_LOAD( "star_trek",   0x10000, 0x4000, CRC(19764e00) SHA1(d7ed577dba02776ac58e8f34b833ed07679c0af1) )
	ROM_LOAD( "television",  0x14000, 0x4000, CRC(413f34c8) SHA1(318f6b464449bf3f0c43c4210a667190c774eb67) )
	ROM_LOAD( "tv_soaps",    0x18000, 0x4000, CRC(26914f3a) SHA1(aec380cea14d6acb71986f3d65c7620b16c174ae) )
	ROM_LOAD( "tv_mash",     0x1c000, 0x4000, CRC(a86990fc) SHA1(6a11b038d48bb97feb4857546349ed93ea1f9273) )
	ROM_LOAD( "movies-tv",   0x20000, 0x4000, CRC(e9a55dad) SHA1(c87682e72bad3507b24eb6a52b4e430e0bfcdab6) )
ROM_END

ROM_START( gt102b )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "trivb2.bin",   0x00000, 0x2000, CRC(e8391f71) SHA1(a955eff87d622d4fcfd25f6d888c48ff82556879) )
	ROM_LOAD( "trivb1.bin",   0x0a000, 0x2000, CRC(cc7b45a7) SHA1(c708f56feb36c1241358a42bb7dce25b799f1f0b) )
	/* Question roms */
	ROM_LOAD( "comics_#1",     0x10000, 0x4000, CRC(8c5cd561) SHA1(1ca566acf72ce636b1b34ee6b7cafb9584340bcc) )
	ROM_LOAD( "entertainment", 0x14000, 0x4000, CRC(b670b9e8) SHA1(0d2246fcc6c753694bc9bd1fc05ac439f24059ef) )
	ROM_LOAD( "general_5",     0x18000, 0x4000, CRC(81bf07c7) SHA1(a53f050b4ef8ffc0499b50224d4bbed4af0ca09c) )
	ROM_LOAD( "hockey",        0x1c000, 0x4000, CRC(4874a431) SHA1(f3c11dfbf71d101aa1a6cd3622b282a4ebe4664b) )
	ROM_LOAD( "sports",        0x20000, 0x4000, CRC(9b4a17b6) SHA1(1b5358b5bc83c2817ecfa4e277fa351a679d5023) )
ROM_END

ROM_START( gt102c1 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog2_right", 0x00000, 0x2000, CRC(76fdc3a3) SHA1(212e09644b9cab334aad22ec5860e8638c6ba3fa) )
	ROM_LOAD( "prog2_left",  0x0a000, 0x2000, CRC(901fb2f9) SHA1(98e49c74d89c4911a1f4d5ccf3e6cf3226c6a178) )
	/* Question roms */
	ROM_LOAD( "entertainment_#1", 0x10000, 0x4000, CRC(cd3ce4c7) SHA1(4bd121fa5899a96b015605f84179ed82be0a25f3) )
	ROM_LOAD( "sports_2",         0x14000, 0x4000, CRC(e8f8e168) SHA1(d2bc57dc0799dd8817b15857f17c4d7ee4d9f932) )
	ROM_LOAD( "sports_2_#2",      0x18000, 0x4000, CRC(fb632622) SHA1(c14d8178f5cfc5994e2ab4f829e353fa75b57304) )
	ROM_LOAD( "sports_3",         0x1c000, 0x4000, CRC(5986996c) SHA1(56432c15a3b0204ed527c18e24716f17bb52dc4e) )
	ROM_LOAD( "sports_#1",        0x20000, 0x4000, CRC(cb1744f5) SHA1(ea3f7bfcecf5c58c26aa0f34908ba5d54f7279ec) )
ROM_END

ROM_START( gt102c2 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog2_right", 0x00000, 0x2000, CRC(76fdc3a3) SHA1(212e09644b9cab334aad22ec5860e8638c6ba3fa) )
	ROM_LOAD( "prog2_left",  0x0a000, 0x2000, CRC(901fb2f9) SHA1(98e49c74d89c4911a1f4d5ccf3e6cf3226c6a178) )
	/* Question roms */
	ROM_LOAD( "comics",       0x10000, 0x4000, CRC(7efdfe8f) SHA1(ec255777c61677ca32c49b9da5e85e07c0647e5f) )
	ROM_LOAD( "history-geog", 0x14000, 0x4000, CRC(76d6b026) SHA1(613809b247cb27773631a1bb34af485c2b1bd486) )
	ROM_LOAD( "science",      0x18000, 0x4000, CRC(68259e09) SHA1(29e848b4744b767c51ff81a756fba7bf96daefec) ) 
	ROM_LOAD( "music_#1",     0x1c000, 0x4000, CRC(1b546857) SHA1(31e04bb5016e8ef6dc48f9b3ddaeab5fe04f91c2) )
ROM_END

ROM_START( gt102c3 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog2_right", 0x00000, 0x2000, CRC(76fdc3a3) SHA1(212e09644b9cab334aad22ec5860e8638c6ba3fa) )
	ROM_LOAD( "prog2_left",  0x0a000, 0x2000, CRC(901fb2f9) SHA1(98e49c74d89c4911a1f4d5ccf3e6cf3226c6a178) )
	/* Question roms */
	ROM_LOAD( "facts_of_life", 0x10000, 0x4000, CRC(1668c7bf) SHA1(6bf43de26f8a626560579ab75fd0890fe00f99dd) )
	ROM_LOAD( "general_#1",    0x14000, 0x4000, CRC(25a0ef9d) SHA1(793abd779cc237e14933933747bbf27bbcbfcd32) )
	ROM_LOAD( "general_2",     0x18000, 0x4000, CRC(5798f2b3) SHA1(0636017969d9b1eac5d33cfb18cb36f7cf4cba88) )
	ROM_LOAD( "general_3",     0x1c000, 0x4000, CRC(a60f17a4) SHA1(0d79be9e2e49b9817e94d410e25bb6dcda10aa9e) )
ROM_END

ROM_START( sextriv1 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog1_right",   0x00000, 0x2000, CRC(73abcd12) SHA1(3b985f25a11507878cef6d11420e215065fb0906) )
	ROM_LOAD( "prog1_left",    0x0a000, 0x2000, CRC(04ee6ecd) SHA1(28342fcdcf36b34fa93f1a985163ca5aab03defe) )
	/* Question roms */
	ROM_LOAD( "adult_sex",    0x10000, 0x4000, CRC(509a8183) SHA1(635c784860e423b22aaea94abc53c1d9477cb1df) )
	ROM_LOAD( "arousing_sex", 0x14000, 0x4000, CRC(1dbf4578) SHA1(51a548d5fe59739e62b5f0e9e6ebc7deb8656210) )
	ROM_LOAD( "intimate_sex", 0x18000, 0x4000, CRC(1f46b626) SHA1(04aa5306c69d130e0f84fa390a773e73c06e5e9b) )
	ROM_LOAD( "sizzling_sex", 0x1c000, 0x4000, CRC(c718833d) SHA1(02ea341e56554dd9302fe95f45dcf446a2978917) )
ROM_END

ROM_START( sextriv2 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "prog1_right",     0x00000, 0x2000, CRC(73abcd12) SHA1(3b985f25a11507878cef6d11420e215065fb0906) )
	ROM_LOAD( "prog1_left",      0x0a000, 0x2000, CRC(04ee6ecd) SHA1(28342fcdcf36b34fa93f1a985163ca5aab03defe) )
	/* Question roms */
	ROM_LOAD( "general_sex",     0x10000, 0x4000, CRC(36fed946) SHA1(25d445ab6cb4b6f41a1dd7104ee1141e597b2e9e) )
	ROM_LOAD( "educational_sex", 0x14000, 0x4000, CRC(281cbe03) SHA1(9c3900cd2535f942a5cbae7edd46ac0170e04c52) )
	ROM_LOAD( "novelty_sex",     0x18000, 0x4000, CRC(26603979) SHA1(78061741e5224b3162be51e637a2fbb9a48962a3) )
ROM_END

GAMEX( 1984, gt102c,   0,        getrivia, getrivia, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.02C)",                 GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1984, gt102b,   0,        getrivia, getrivia, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.02B)",                 GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1985, gt102c1,  gt102c, getrivia, getrivia, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.02C Alt questions 1)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1985, gt102c2,  gt102c, getrivia, getrivia, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.02C Alt questions 2)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1985, gt102c3,  gt102c, getrivia, getrivia, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.02C Alt questions 3)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAMEX( 1985, sextriv1, 0,        getrivia, getrivia, 0, ROT0, "Kinky Kit and Game Co.", "Sexual Trivia (Version 1.02SB set 1)",  GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAMEX( 1985, sextriv2, sextriv1, getrivia, getrivia, 0, ROT0, "Kinky Kit and Game Co.", "Sexual Trivia (Version 1.02SB set 2)",  GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
