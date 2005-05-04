/*  GTI Club */

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/powerpc/ppc.h"

static data8_t led_reg0;
static data8_t led_reg1;

#define LED_ON		0xff00ff00
#define SHOW_LEDS	0

#if 0
static void draw_led(struct mame_bitmap *bitmap, int x, int y,data8_t value)
{
	plot_box(bitmap, x, y, 5, 9, 0x00000000);

	/* Top */
	if( (value & 0x40) == 0 ) {
		plot_pixel(bitmap, x+1, y+0, LED_ON);
		plot_pixel(bitmap, x+2, y+0, LED_ON);
		plot_pixel(bitmap, x+3, y+0, LED_ON);
	}
	/* Middle */
	if( (value & 0x01) == 0 ) {
		plot_pixel(bitmap, x+1, y+4, LED_ON);
		plot_pixel(bitmap, x+2, y+4, LED_ON);
		plot_pixel(bitmap, x+3, y+4, LED_ON);
	}
	/* Bottom */
	if( (value & 0x08) == 0 ) {
		plot_pixel(bitmap, x+1, y+8, LED_ON);
		plot_pixel(bitmap, x+2, y+8, LED_ON);
		plot_pixel(bitmap, x+3, y+8, LED_ON);
	}
	/* Top Left */
	if( (value & 0x02) == 0 ) {
		plot_pixel(bitmap, x+0, y+1, LED_ON);
		plot_pixel(bitmap, x+0, y+2, LED_ON);
		plot_pixel(bitmap, x+0, y+3, LED_ON);
	}
	/* Top Right */
	if( (value & 0x20) == 0 ) {
		plot_pixel(bitmap, x+4, y+1, LED_ON);
		plot_pixel(bitmap, x+4, y+2, LED_ON);
		plot_pixel(bitmap, x+4, y+3, LED_ON);
	}
	/* Bottom Left */
	if( (value & 0x04) == 0 ) {
		plot_pixel(bitmap, x+0, y+5, LED_ON);
		plot_pixel(bitmap, x+0, y+6, LED_ON);
		plot_pixel(bitmap, x+0, y+7, LED_ON);
	}
	/* Bottom Right */
	if( (value & 0x10) == 0 ) {
		plot_pixel(bitmap, x+4, y+5, LED_ON);
		plot_pixel(bitmap, x+4, y+6, LED_ON);
		plot_pixel(bitmap, x+4, y+7, LED_ON);
	}
}
#endif


int K001604_vh_start(void);
void K001604_tile_update(void);
void K001604_tile_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect);
READ32_HANDLER(K001604_tile_r);
READ32_HANDLER(K001604_char_r);
WRITE32_HANDLER(K001604_tile_w);
WRITE32_HANDLER(K001604_char_w);
WRITE32_HANDLER(K001604_reg_w);
READ32_HANDLER(K001604_reg_r);


static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}




VIDEO_START( gticlub )
{
	return K001604_vh_start();
}

VIDEO_UPDATE( gticlub )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	K001604_tile_update();
	K001604_tile_draw(bitmap, cliprect);

#if SHOW_LEDS
	draw_led(bitmap, 3, 3, led_reg0);
	draw_led(bitmap, 9, 3, led_reg1);
#endif

}

/******************************************************************/

/* 93C56 EEPROM */
static struct EEPROM_interface eeprom_interface =
{
	8,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxxxx",	/* lock command */
	"*10011xxxxxx",	/* unlock command */
	1,				/* enable_multi_read */
	0				/* reset_delay */
};

static void eeprom_handler(mame_file *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)	EEPROM_load(file);
	}
}

//UINT32 eeprom_bit = 0;
static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if (offset == 1) {
		if (!(mem_mask & 0xff000000) )
		{
			UINT32 eeprom_bit = (EEPROM_read_bit() << 1);
			r |= eeprom_bit << 24;
		}
		return r;
	}
	return 0xffffffff;
}

static WRITE32_HANDLER( sysreg_w )
{
	if (offset == 0) {
		if( !(mem_mask & 0xff000000) )
		{
			led_reg0 = (data >> 24) & 0xff;
		}
		if( !(mem_mask & 0x00ff0000) )
		{
			led_reg1 = (data >> 16) & 0xff;
		}
		if( !(mem_mask & 0x000000ff) )
		{
			EEPROM_write_bit((data & 0x01) ? 1 : 0);
			EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
		}
	}
	if (offset == 1)
		return;
}

static data32_t video_reg = 0;

static READ32_HANDLER( video_r )
{
	video_reg ^= 0x80;
	return video_reg;
}

/******************************************************************/

static ADDRESS_MAP_START( gticlub_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x74000000, 0x740000ff) AM_READWRITE(K001604_reg_r, K001604_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_READWRITE(K001604_tile_r, K001604_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_READWRITE(K001604_char_r, K001604_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_RAM
	AM_RANGE(0x78040000, 0x78040007) AM_RAM
	AM_RANGE(0x78080000, 0x78080007) AM_RAM
	AM_RANGE(0x780c0000, 0x780c0003) AM_READ(video_r)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x80000000, 0x800fffff) AM_RAM	AM_SHARE(3)	/* Work RAM */
	AM_RANGE(0xff000000, 0xff3fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0xff800000, 0xff9fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END



/**********************************************************************/

/********************************************************************/

static NVRAM_HANDLER(gticlub)
{
	eeprom_handler(file, read_or_write);
}


INPUT_PORTS_START( gticlub )
INPUT_PORTS_END

static ppc_config gticlub_ppc_cfg =
{
	PPC_MODEL_403GA
};

static MACHINE_DRIVER_START( gticlub )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(gticlub_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(gticlub)
	MDRV_VIDEO_UPDATE(gticlub)
MACHINE_DRIVER_END

/*************************************************************************/

ROM_START(gticlub)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688aaa01.21u", 0x000003, 0x80000, CRC(06a56474) SHA1(3a457b885a35e3ee030fd51d847bcf75fce46208))
	ROM_LOAD32_BYTE("688aaa02.19u", 0x000002, 0x80000, CRC(3c1e714a) SHA1(557f8542b855b2b35f242c8db7396017aca6dbd8))
	ROM_LOAD32_BYTE("688aaa03.21r", 0x000001, 0x80000, CRC(e060580b) SHA1(50242f3f3b949cc03082e4e75d9dcc89e17f0a75))
	ROM_LOAD32_BYTE("688aaa04.19r", 0x000000, 0x80000, CRC(928c23cd) SHA1(cce54398e1e5b98bfb717839cc422f1f60502788))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d))
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423))

	ROM_REGION(0x40000, REGION_CPU2, 0)	/* 68k program */
        ROM_LOAD( "688a07.13k",   0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x1000000, REGION_USER3, 0)	/* other roms */
        ROM_LOAD( "688a09.9s",    0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
        ROM_LOAD( "688a10.7s",    0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
        ROM_LOAD( "688a11.5s",    0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
        ROM_LOAD( "688a12.2s",    0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )
        ROM_LOAD( "688a13.18d",   0x800000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
        ROM_LOAD( "688a14.13d",   0xa00000, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
        ROM_LOAD( "688a15.9d",    0xc00000, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
        ROM_LOAD( "688a16.4d",    0xe00000, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )
ROM_END

/*************************************************************************/

GAMEX( 1996, gticlub,	0,		gticlub,	gticlub,	0,	ROT0,	"Konami",	"GTI Club", GAME_NOT_WORKING|GAME_NO_SOUND )
