/*---

Pirates (c)1994 NIX
preliminary driver

-----

Here's some info about the dump:

Name:            Pirates
Manufacturer:    NIX
Year:            1994
Date Dumped:     14-07-2002 (DD-MM-YYYY)

CPU:             68000, possibly at 12mhz (prototype board does have a 16mhz one)
SOUND:           OKIM6295
GFX:             Unknown

CPU Roms at least are the same on the Prototype board (the rest of the roms probably are too)

-----

Tilemaps look a bit strange, looks like its 3 32x40 tilemaps in a 0x4000 region? not sure,
might be more obvious once I have some valid tiles to play with.

Program Roms are Scrambled (Data + Address Lines)
P Graphic Roms are Scrambled (Address Lines Only?) Not Descrambled
S Graphic Roms are Scrambled (Address Lines Only?) Not Descrambled
OKI Samples Rom is Scrambled (Address Lines Only?) Not Descrambled

68k interrupts
lev 1 : 0x64 : 0000 bf84 - vbl?
lev 2 : 0x68 : 0000 4bc6 -
lev 3 : 0x6c : 0000 3bda -
lev 4 : 0x70 : 0000 3bf0 -
lev 5 : 0x74 : 0000 3c06 -
lev 6 : 0x78 : 0000 3c1c -
lev 7 : 0x7c : 0000 3c32 -

Game Crashes at Game Over or after a while of the attract mode, it simply hangs.
Inputs mapped by Stephh

---*/

#include "driver.h"

data16_t *pirates_tileram, *pirates_spriteram;
static struct tilemap *pirates_tilemap;

/* Video Hardware */

/* sprites */

static void pirates_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *source = pirates_spriteram;
	data16_t *finish = source + 0x800/2;

	while( source<finish )
	{
		int xpos, ypos, number;

		ypos = source[1] & 0x1ff;
		xpos = source[3] & 0x1ff;
		number = source[0];

		xpos = 0xff-xpos;

		drawgfx(bitmap,gfx,number,0,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);

		source+=4;
	}
}

/* tilemaps */

static void get_pirates_tile_info(int tile_index)
{
	int code = pirates_tileram[tile_index*2];
	int colr = pirates_tileram[tile_index*2+1];

	SET_TILE_INFO(3,code,colr,0)
}

WRITE16_HANDLER(  pirates_tileram_w )
{
	pirates_tileram[offset] = data;
	tilemap_mark_tile_dirty(pirates_tilemap,offset/2);
}

/* video start / update */

VIDEO_START(pirates)
{
	pirates_tilemap = tilemap_create(get_pirates_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32, 128);

	if (!pirates_tilemap) return 1;

	return 0;
}

VIDEO_UPDATE(pirates)
{
	tilemap_draw(bitmap,cliprect,pirates_tilemap,0,0);
	pirates_drawsprites(bitmap,cliprect);
}

/* graphic decodes */

/* we don't know what they really are since the gfx roms are currently encrypted,
   going on the content of spriteram sprite tiles are probably 16x16 tho, and
   going on the front layer text tiles are probably 8x8 */

static struct GfxLayout pirates1616_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 	0x080000*3*8, 	0x080000*2*8, 	0x080000*1*8,	0x080000*0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*16
};

static struct GfxLayout pirates88_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 	0x080000*3*8, 	0x080000*2*8, 	0x080000*1*8,	0x080000*0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{

	{ REGION_GFX1, 0, &pirates1616_layout,  0, 256  },
	{ REGION_GFX2, 0, &pirates1616_layout,  0, 256  },
	{ REGION_GFX1, 0, &pirates88_layout,  0, 256  },
	{ REGION_GFX2, 0, &pirates88_layout,  0, 256  },
	{ -1 } /* end of array */
};

/* Input Ports */

INPUT_PORTS_START( pirates )
	PORT_START	// IN0 - 0x300000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN1 - 0x400000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )		// seems checked in "test mode"
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )		// seems checked in "test mode"
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* What do these bits do ? */
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - 0xa00000.w (only the LSB is read)
	/* What do these bits do ? */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/* Memory Maps */

static MEMORY_READ16_START( pirates_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x300000, 0x300001, input_port_0_word_r },
	{ 0x400000, 0x400001, input_port_1_word_r },
	{ 0x500000, 0x5007ff, MRA16_RAM },
	{ 0x800000, 0x800fff, MRA16_RAM },
	{ 0x800000, 0x803fff, MRA16_RAM },
	{ 0x900000, 0x903fff, MRA16_RAM },
	{ 0x904000, 0x907fff, MRA16_RAM },
	{ 0xA00000, 0xA00001, input_port_2_word_r }, // To be confirmed
MEMORY_END

static MEMORY_WRITE16_START( pirates_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM }, // main ram
	{ 0x500000, 0x5007ff, MWA16_RAM, &pirates_spriteram },
	{ 0x500800, 0x50080f, MWA16_RAM },
	{ 0x600000, 0x600001, MWA16_NOP },
	{ 0x700000, 0x700001, MWA16_RAM },
	{ 0x800000, 0x803fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 }, // pal?
	{ 0x900000, 0x903fff, pirates_tileram_w, &pirates_tileram }, // tilemaps ?
	{ 0x904000, 0x907fff, MWA16_RAM },  // more of tilemaps ?
MEMORY_END

/* Machine Driver + Related bits */

static INTERRUPT_GEN( pirates_interrupt ) {

	if( cpu_getiloops() == 0 )
	{
		cpu_set_irq_line(0, 1, HOLD_LINE); // vbl?
	}
	else
	{
//		cpu_set_irq_line(0, 2, HOLD_LINE);
	}
}

static MACHINE_DRIVER_START( pirates )
	MDRV_CPU_ADD(M68000, 16000000) /* either 16mhz or 12mhz */
	MDRV_CPU_MEMORY(pirates_readmem,pirates_writemem)
//	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)
	MDRV_CPU_VBLANK_INT(pirates_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 40*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 40*8-1)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(pirates)
	MDRV_VIDEO_UPDATE(pirates)

//	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/* Init */

void pirates_decrypt_68k(void)
{
    int rom_size;
    UINT16 *buf, *rom;
    int i;

    rom_size = memory_region_length(REGION_CPU1);

    buf = malloc(rom_size);

    if (!buf) return;

    rom = (UINT16 *)memory_region(REGION_CPU1);
    memcpy (buf, rom, rom_size);

    for (i=0; i<rom_size/2; i++)
    {
        int adrl, adrr;
        unsigned char vl, vr;

        adrl = BITSWAP24(i,23,22,21,20,19,18,4,8,3,14,2,15,17,0,9,13,10,5,16,7,12,6,1,11);
        vl = BITSWAP8(buf[adrl],    4,2,7,1,6,5,0,3);

        adrr = BITSWAP24(i,23,22,21,20,19,18,4,10,1,11,12,5,9,17,14,0,13,6,15,8,3,16,7,2);
        vr = BITSWAP8(buf[adrr]>>8, 1,4,7,0,3,5,6,2);

        rom[i] = (vr<<8) | vl;
    }
    free (buf);

/* save to a file */
#if 0
	{
		FILE *fp;
		fp=fopen("pirates.dmp", "w+b");
		if (fp)
		{
			fwrite(rom, rom_size, 1, fp);
			fclose(fp);
		}
	}
#endif

}

static DRIVER_INIT( pirates )
{
	pirates_decrypt_68k();
//	pirates_decrypt_p();
//	pirates_decrypt_s();
//	pirates_decrypt_oki();
}

/* Rom Loading */

ROM_START( pirates )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "r_449b.bin",  0x00000, 0x80000, 0x224aeeda )
	ROM_LOAD16_BYTE( "l_5c1e.bin",  0x00001, 0x80000, 0x46740204 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* GFX */
	ROM_LOAD( "p1_7b30.bin", 0x000000, 0x080000, 0x26d78518 )
	ROM_LOAD( "p2_5d74.bin", 0x080000, 0x080000, 0x40e069b4 )
	ROM_LOAD( "p4_4d48.bin", 0x100000, 0x080000, 0x89fda216 )
	ROM_LOAD( "p8_9f4f.bin", 0x180000, 0x080000, 0xf31696ea )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "s1_6e89.bin", 0x000000, 0x080000, 0xc78a276f )
	ROM_LOAD( "s2_6df3.bin", 0x080000, 0x080000, 0x9f0bad96 )
	ROM_LOAD( "s4_fdcc.bin", 0x100000, 0x080000, 0x8916ddb5 )
	ROM_LOAD( "s8_4b7c.bin", 0x180000, 0x080000, 0x1c41bd2c )

	ROM_REGION( 0x080000, REGION_SOUND1, 0) /* sound? */
	ROM_LOAD( "s89_49d4.bin", 0x000000, 0x080000, 0x63a739ec )
ROM_END

/* GAME */

GAMEX(1994, pirates, 0, pirates, pirates, pirates, ROT90, "NIX", "Pirates", GAME_NOT_WORKING | GAME_NO_SOUND )
