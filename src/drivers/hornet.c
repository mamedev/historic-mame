/*	Konami Hornet */

#include "driver.h"

static data8_t led_reg0;
static data8_t led_reg1;

#define SHOW_LEDS	1
#define LED_ON		0xff00ff00

static void draw_7segment_led(struct mame_bitmap *bitmap, int x, int y, data8_t value)
{
	// Gradius 4 seems to not have LEDs, so don't bother
	if (!strcmp(Machine->gamedrv->name, "gradius4"))
	{
		return;
	}

	plot_box(bitmap, x-1, y-1, 7, 11, 0x00000000);

	/* Top */
	if( (value & 0x40) == 0 ) {
		plot_box(bitmap, x+1, y+0, 3, 1, LED_ON);
	}
	/* Middle */
	if( (value & 0x01) == 0 ) {
		plot_box(bitmap, x+1, y+4, 3, 1, LED_ON);
	}
	/* Bottom */
	if( (value & 0x08) == 0 ) {
		plot_box(bitmap, x+1, y+8, 3, 1, LED_ON);
	}
	/* Top Left */
	if( (value & 0x02) == 0 ) {
		plot_box(bitmap, x+0, y+1, 1, 3, LED_ON);
	}
	/* Top Right */
	if( (value & 0x20) == 0 ) {
		plot_box(bitmap, x+4, y+1, 1, 3, LED_ON);
	}
	/* Bottom Left */
	if( (value & 0x04) == 0 ) {
		plot_box(bitmap, x+0, y+5, 1, 3, LED_ON);
	}
	/* Bottom Right */
	if( (value & 0x10) == 0 ) {
		plot_box(bitmap, x+4, y+5, 1, 3, LED_ON);
	}
}

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	r = ((data >> 6) & 0x1f);
	g = ((data >> 0) & 0x3f);
	b = ((data >> 11) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 2) | (g >> 3);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}


/* K037122 Tilemap chip (move to konamiic.c ?) */

static UINT32 *K037122_tile_ram;
static UINT32 *K037122_char_ram;
static UINT8 *K037122_dirty_map;
static int K037122_gfx_index, K037122_char_dirty;
static struct tilemap *K037122_layer[1];

#define K037122_NUM_TILES		2048

static struct GfxLayout K037122_char_layout =
{
	8, 8,
	K037122_NUM_TILES,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 1*16, 0*16, 3*16, 2*16, 5*16, 4*16, 7*16, 6*16 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128 },
	8*128
};

static void K037122_tile_info(int tile_index)
{
	UINT32 val = K037122_tile_ram[tile_index];
	int color = val >> 16;
	int tile = val & 0xffff;
	SET_TILE_INFO(K037122_gfx_index, tile, color/2, 0);
}

int K037122_vh_start(void)
{
	for(K037122_gfx_index = 0; K037122_gfx_index < MAX_GFX_ELEMENTS; K037122_gfx_index++)
		if (Machine->gfx[K037122_gfx_index] == 0)
			break;
	if(K037122_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	K037122_char_ram = auto_malloc(0x80000);
	if( !K037122_char_ram )
		return 1;

	K037122_tile_ram = auto_malloc(0x20000);
	if( !K037122_tile_ram )
		return 1;

	K037122_dirty_map = auto_malloc(K037122_NUM_TILES);
	if( !K037122_dirty_map ) {
		free(K037122_char_ram);
		free(K037122_tile_ram);
		return 1;
	}

	K037122_layer[0] = tilemap_create(K037122_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 256, 64);

	if( !K037122_layer[0] ) {
		free(K037122_dirty_map);
		free(K037122_tile_ram);
		free(K037122_char_ram);
	}

	tilemap_set_transparent_pen(K037122_layer[0], 0);

	memset(K037122_char_ram, 0, 0x80000);
	memset(K037122_tile_ram, 0, 0x10000);
	memset(K037122_dirty_map, 0, K037122_NUM_TILES);

	Machine->gfx[K037122_gfx_index] = decodegfx((UINT8*)K037122_char_ram, &K037122_char_layout);
	if( !Machine->gfx[K037122_gfx_index] ) {
		free(K037122_dirty_map);
		free(K037122_tile_ram);
		free(K037122_char_ram);
	}

	if (Machine->drv->color_table_len)
	{
		Machine->gfx[K037122_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[K037122_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[K037122_gfx_index]->colortable = Machine->pens;
		Machine->gfx[K037122_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	return 0;
}

void K037122_tile_update(void)
{
	if(K037122_char_dirty) {
		int i;
		for(i=0; i<K037122_NUM_TILES; i++) {
			if(K037122_dirty_map[i]) {
				K037122_dirty_map[i] = 0;
				decodechar(Machine->gfx[K037122_gfx_index], i, (UINT8 *)K037122_char_ram, &K037122_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(K037122_layer[0]);
		K037122_char_dirty = 0;
	}
}

void K037122_tile_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	tilemap_draw(bitmap, cliprect, K037122_layer[0], 0,0);
}

READ32_HANDLER(K037122_tile_r)
{
	return K037122_tile_ram[offset];
}

READ32_HANDLER(K037122_char_r)
{
	return K037122_char_ram[offset];
}

WRITE32_HANDLER(K037122_tile_w)
{
	COMBINE_DATA(K037122_tile_ram + offset);
	tilemap_mark_tile_dirty(K037122_layer[0], offset);
}

WRITE32_HANDLER(K037122_char_w)
{
	COMBINE_DATA(K037122_char_ram + offset);
	K037122_dirty_map[offset / 32] = 1;
	K037122_char_dirty = 1;
}



VIDEO_START( hornet )
{
	return K037122_vh_start();
}

VIDEO_UPDATE( hornet )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	K037122_tile_update();
	K037122_tile_draw(bitmap, cliprect);

#if SHOW_LEDS
	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
#endif
}

/******************************************************************/

static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if( offset == 0 ) {
		if( mem_mask == 0x00ffffff )
			r |= 0 << 24;
		if( mem_mask == 0xffff00ff )
			r |= 0 << 8;
		return r;
	}
	if( offset == 1 ) {
		if( mem_mask == 0x00ffffff )	/* Dip switches */
			r |= 0x80 << 24;			/* 0x80 = test mode */
		return r;
	}
	return 0;
}

static WRITE32_HANDLER( sysreg_w )
{
	if( offset == 0 ) {
		if( mem_mask == 0x00ffffff )
			led_reg0 = (data >> 24) & 0xff;
		if( mem_mask == 0xff00ffff )
			led_reg1 = (data >> 16) & 0xff;
		return;
	}
	if( offset == 1 )
		return;
}

static data8_t sndto68k[16], sndtoppc[16];	/* read/write split mapping */

static READ32_HANDLER( ppc_sound_r )
{
	/* Hack to get past the sound IC test */
	return 0x005f0000;
}

INLINE void write_snd_ppc(int reg, int val)
{
	sndto68k[reg] = val;

	if (reg == 7)
	{
		cpunum_set_input_line(1, 1, HOLD_LINE);
	}
}

#if 0
static WRITE32_HANDLER( ppc_sound_w )
{
	int reg=0, val=0;

	if (!(mem_mask & 0xff000000))
	{
		reg = offset * 4;
		val = data >> 24;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		reg = (offset * 4) + 1;
		val = (data >> 16) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x0000ff00))
	{
		reg = (offset * 4) + 2;
		val = (data >> 8) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x000000ff))
	{
		reg = (offset * 4) + 3;
		val = (data >> 0) & 0xff;
		write_snd_ppc(reg, val);
	}
}
#endif

static int comm_rombank = 0;

static WRITE32_HANDLER( comm1_w )
{
	printf("comm1_w: %08X, %08X, %08X\n", offset, data, mem_mask);
}

static WRITE32_HANDLER( comm_rombank_w )
{
	int bank = data >> 24;
	if( bank != comm_rombank ) {
		comm_rombank = bank;
		cpu_setbank(1, memory_region(REGION_USER3) + (comm_rombank * 0x10000));
	}
}

static READ32_HANDLER( comm0_unk_r )
{
	printf("comm0_unk_r: %08X, %08X\n", offset, mem_mask);
	return 0xffffffff;
}

static data32_t video_reg = 0;

static READ32_HANDLER( video_r )
{
	video_reg ^= 0x80;
	return video_reg;
}

/******************************************************************/

static ADDRESS_MAP_START( hornet_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x74020000, 0x74027fff) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74028000, 0x7403ffff) AM_READWRITE(&K037122_tile_r, &K037122_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_READWRITE(&K037122_char_r, &K037122_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_RAM
	AM_RANGE(0x780c0000, 0x780c0003) AM_READ(video_r)
	AM_RANGE(0x7d000000, 0x7d00ffff) AM_READ(sysreg_r)
	AM_RANGE(0x7d010000, 0x7d01ffff) AM_WRITE(sysreg_w)
	AM_RANGE(0x7d020000, 0x7d021fff) AM_RAM				/* M48T58Y RTC/NVRAM */
	AM_RANGE(0x7d030000, 0x7d030007) AM_READ(ppc_sound_r)
	AM_RANGE(0x7d042000, 0x7d043fff) AM_RAM				/* COMM BOARD 0 */
	AM_RANGE(0x7d044000, 0x7d044007) AM_READ(comm0_unk_r)
	AM_RANGE(0x7d048000, 0x7d048003) AM_WRITE(comm1_w)
	AM_RANGE(0x7d04a000, 0x7d04a003) AM_WRITE(comm_rombank_w)
	AM_RANGE(0x7d050000, 0x7d05ffff) AM_ROMBANK(1)		/* COMM BOARD 1 */
	AM_RANGE(0x7f000000, 0x7f1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM	AM_SHARE(3)	/* Work RAM */
	AM_RANGE(0xfe000000, 0xfe3fffff) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0xff000000, 0xff1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gradius4_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x74000014, 0x74000017) AM_NOP
	AM_RANGE(0x74020000, 0x74027fff) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74028000, 0x7403ffff) AM_READWRITE(&K037122_tile_r, &K037122_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_READWRITE(&K037122_char_r, &K037122_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_RAM
	AM_RANGE(0x780c0000, 0x780c0003) AM_READ(video_r)
	AM_RANGE(0x7d000000, 0x7d00ffff) AM_READ(sysreg_r)
	AM_RANGE(0x7d010000, 0x7d01ffff) AM_WRITE(sysreg_w)
	AM_RANGE(0x7d020000, 0x7d021fff) AM_RAM				/* M48T58Y RTC/NVRAM */
	AM_RANGE(0x7d030000, 0x7d030007) AM_READ(ppc_sound_r)
	AM_RANGE(0x7d042000, 0x7d043fff) AM_RAM				/* COMM BOARD 0 */
	AM_RANGE(0x7d044000, 0x7d044007) AM_READ(comm0_unk_r)
	AM_RANGE(0x7d048000, 0x7d048003) AM_WRITE(comm1_w)
	AM_RANGE(0x7d04a000, 0x7d04a003) AM_WRITE(comm_rombank_w)
	AM_RANGE(0x7d050000, 0x7d05ffff) AM_ROMBANK(1)		/* COMM BOARD 1 */
	AM_RANGE(0x7f000000, 0x7f1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM	AM_SHARE(3)	/* Work RAM */
	AM_RANGE(0x7e000000, 0x7e7fffff) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0xff000000, 0xff1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END


/**********************************************************************/

#if 0
static READ16_HANDLER( sndcomm68k_r )
{
	return sndto68k[offset];
}
#endif

static WRITE16_HANDLER( sndcomm68k_w )
{
	sndtoppc[offset] = data;
}

static READ16_HANDLER( rf5c400_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( rf5c400_w )
{

}

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		/* Work RAM */
	AM_RANGE(0x200000, 0x200fff) AM_READ(rf5c400_r)		/* Ricoh RF5C400 */
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(rf5c400_w)
	//AM_RANGE(0x300000, 0x30000f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x300000, 0x30000f) AM_WRITE(sndcomm68k_w)
ADDRESS_MAP_END

/********************************************************************/

INPUT_PORTS_START( hornet )
INPUT_PORTS_END

static MACHINE_INIT( hornet )
{
	cpu_setbank(1, memory_region(REGION_USER3));
}

static MACHINE_DRIVER_START( hornet )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_PROGRAM_MAP(hornet_map, 0)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( hornet )

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(hornet)
	MDRV_VIDEO_UPDATE(hornet)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gradius4 )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_PROGRAM_MAP(gradius4_map, 0)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( hornet )

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(hornet)
	MDRV_VIDEO_UPDATE(hornet)

MACHINE_DRIVER_END

/*************************************************************************/

ROM_START(sscope)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
	ROM_LOAD16_WORD_SWAP("ss1-1.27p", 0x000000, 0x200000, CRC(3b6bb075) SHA1(babc134c3a20c7cdcaa735d5f1fd5cab38667a14))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
	ROM_LOAD16_WORD_SWAP("ss1-1.7s", 0x000000, 0x80000, CRC(2805ea1d) SHA1(2556a51ee98cb8f59bf081e916c69a24532196f1))

	ROM_REGION(0x1000000, REGION_USER4, 0)		/* other roms (samples? textures?) */
        ROM_LOAD( "ss1-1.16p",    0x000000, 0x200000, CRC(4503ff1e) SHA1(2c208a1e9a5633c97e8a8387b7fcc7460013bc2c) )
        ROM_LOAD( "ss1-1.14p",    0x200000, 0x200000, CRC(a5bd9a93) SHA1(c789a272b9f2b449b07fff1c04b6c9ef3ca6bfe0) )
        ROM_LOAD( "ss1-3.u32",    0x400000, 0x400000, CRC(335793e1) SHA1(d582b53c3853abd59bc728f619a30c27cfc9497c) )
        ROM_LOAD( "ss1-3.u24",    0x800000, 0x400000, CRC(d6e7877e) SHA1(b4d0e17ada7dd126ec564a20e7140775b4b3fdb7) )
        ROM_LOAD( "ss830a13.bin", 0xc00000, 0x200000, CRC(707f4b83) SHA1(a0ebe1de32a807e99e77144f2fc6de455ee9e42a) )
        ROM_LOAD( "ss830a14.bin", 0xe00000, 0x200000, CRC(ba338856) SHA1(0cab0bb5a8164256cbe98a4b08dcdaf877c1cfdc) )
ROM_END

ROM_START(sscope2)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
	ROM_LOAD16_WORD_SWAP("931d01.bin", 0x000000, 0x200000, CRC(4065fde6) SHA1(84f2dedc3e8f61651b22c0a21433a64993e1b9e2))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* Data ROM */
	ROM_LOAD32_WORD("931a04.bin", 0x000000, 0x200000, CRC(4f5917e6) SHA1(a63a107f1d6d9756e4ab0965d72ea446f0692814))

	ROM_REGION(0x800000, REGION_USER3, 0)		/* Comm board ROMs */
	ROM_LOAD("931a19.bin", 0x000000, 0x400000, CRC(8e8bb6af) SHA1(1bb399f7897fbcbe6852fda3215052b2810437d8))
	ROM_LOAD("931a20.bin", 0x400000, 0x400000, CRC(a14a7887) SHA1(daf0cbaf83e59680a0d3c4d66fcc48d02c9723d1))
	
	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
	ROM_LOAD16_WORD_SWAP("931a08.bin", 0x000000, 0x80000, CRC(1597d604) SHA1(a1eab4d25907930b59ea558b484c3b6ddcb9303c))

	ROM_REGION(0xc00000, REGION_USER4, 0)		/* other roms (samples? textures?) */
        ROM_LOAD( "931a09.bin",   0x000000, 0x400000, CRC(694c354c) SHA1(42f54254a5959e1b341f2801f1ad032c4ed6f329) )
        ROM_LOAD( "931a10.bin",   0x400000, 0x400000, CRC(78ceb519) SHA1(e61c0d21b6dc37a9293e72814474f5aee59115ad) )
        ROM_LOAD( "931a11.bin",   0x800000, 0x400000, CRC(9c8362b2) SHA1(a8158c4db386e2bbd61dc9a600720f07a1eba294) )
ROM_END

ROM_START(gradius4)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program */
        ROM_LOAD16_WORD_SWAP( "837c01.27p",   0x000000, 0x200000, CRC(ce003123) SHA1(15e33997be2c1b3f71998627c540db378680a7a1) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* Data ROM */
        ROM_LOAD32_WORD( "837a04.16t",   0x000000, 0x200000, CRC(18453b59) SHA1(3c75a54d8c09c0796223b42d30fb3867a911a074) )
        ROM_LOAD32_WORD( "837a05.14t",   0x000002, 0x200000, CRC(77178633) SHA1(ececdd501d0692390325c8dad6dbb068808a8b26) )

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68K Program */
        ROM_LOAD16_WORD_SWAP( "837a08.7s",    0x000000, 0x080000, CRC(c3a7ff56) SHA1(9d8d033277d560b58da151338d14b4758a9235ea) )

	ROM_REGION(0x1800000, REGION_USER4, 0)		/* other roms (samples? textures?) */
        ROM_LOAD( "837a09.16p",   0x0000000, 0x400000, CRC(fb8f3dc2) SHA1(69e314ac06308c5a24309abc3d7b05af6c0302a8) )
        ROM_LOAD( "837a10.14p",   0x0400000, 0x400000, CRC(1419cad2) SHA1(a6369a5c29813fa51e8246d0c091736f32994f3d) )
        ROM_LOAD( "837a13.24u",   0x0800000, 0x400000, CRC(d86e10ff) SHA1(6de1179d7081d9a93ab6df47692d3efc190c38ba) )
        ROM_LOAD( "837a14.32u",   0x0c00000, 0x400000, CRC(ff1b5d18) SHA1(7a38362170133dcc6ea01eb62981845917b85c36) )
        ROM_LOAD( "837a15.24v",   0x1000000, 0x400000, CRC(e0620737) SHA1(c14078cdb44f75c7c956b3627045d8494941d6b4) )
        ROM_LOAD( "837a16.32v",   0x1400000, 0x400000, CRC(bb7a7558) SHA1(8c8cc062793c2dcfa72657b6ea0813d7223a0b87) )
ROM_END

/*************************************************************************/

GAMEX( 1999, gradius4,	0,		gradius4, hornet,	0,		ROT0,	"Konami",	"Gradius 4: Fukkatsu", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1999, sscope,	0,		hornet,	hornet,	0,		ROT0,	"Konami",	"Silent Scope", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 2000, sscope2,	0,		hornet,	hornet,	0,		ROT0,	"Konami",	"Silent Scope 2", GAME_NOT_WORKING|GAME_NO_SOUND )
