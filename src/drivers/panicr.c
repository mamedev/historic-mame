/*
Similar hw to Dead Angle.
GFX is encrypted (or rather bitswaped)



Panic Road (JPN Ver.)
(c)1986 Taito / Seibu
SEI-8611M (M6100219A)

CPU  : Sony CXQ70116D-8 (8086)
Sound: YM2151,
OSC  : 14.31818MHz,12.0000MHz,16.0000MHz
Other:
	Toshiba T5182
	SEI0010BU(TC17G005AN-0025) x3
	SEI0021BU(TC17G008AN-0022)
	Toshiba(TC17G008AN-0024)
	SEI0030BU(TC17G005AN-0026)
	SEI0050BU(MA640 00)
	SEI0040BU(TC15G008AN-0048)

13F.BIN      [4e6b3c04]
15F.BIN      [d735b572]

22D.BIN      [eb1a46e1]

5D.BIN       [f3466906]
7D.BIN       [8032c1e9]

2A.BIN       [3ac0e5b4]
2B.BIN       [567d327b]
2C.BIN       [cd77ec79]
2D.BIN       [218d2c3e]

2J.BIN       [80f05923]
2K.BIN       [35f07bca]

1.19N        [674131b9]
2.19M        [3d48b0b5]

A.15C        [c75772bc]	82s129
B.14C        [145d1e0d]	 |
C.13C        [11c11bbd]	 |
D.9B         [f99cac4b]	/

8A.BPR       [908684a6]	63s281
10J.BPR      [1dd80ee1]	 |
10L.BPR      [f3f29695]	 |
12D.BPR      [0df8aa3c]	/

*/

#include "driver.h"
#include "vidhrdw/generic.h" 


static READ8_HANDLER(f_r)
{
		return 0xff;	
}

static struct tilemap *tilemap;

static WRITE8_HANDLER (vram_w)
{
	videoram[offset]=data;
	tilemap_mark_all_tiles_dirty( tilemap );
}

/*
static void get_tile_info(int tile_index)
{
	int code;
	
	code=videoram[tile_index/2];

	SET_TILE_INFO(
		0,
		code,
		0,
		0)
}
*/

static ADDRESS_MAP_START( panicr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0x0c000, 0x0cfff) AM_WRITE(vram_w) AM_BASE(&videoram)
	

	AM_RANGE(0x0d000, 0x0d008) AM_NOP
	AM_RANGE(0x0d200, 0x0d2ff) AM_NOP
	AM_RANGE(0x0d800, 0x0d81f) AM_NOP
	AM_RANGE(0x0d400, 0x0d40f) AM_READ(f_r)
	

	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

VIDEO_START( panicr )
{
	//tilemap = tilemap_create( get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,64 );	
	return 0;
}

VIDEO_UPDATE( panicr)
{
//tilemap_draw(bitmap,cliprect,tilemap,0,0);	

	const data8_t *pSource;
	int sx,sy;
	data8_t tile,attr;
	
	pSource = videoram;
	for( sy=0; sy<256; sy+=8 )
	{
		for( sx=0; sx<512; sx+=8 )
		{
			tile = pSource[0];
			attr = pSource[1];
			if(tile<32 || tile >90)tile=32;
			
			pSource += 2;
			drawgfx(
				bitmap,Machine->uirotfont,
				tile,
				attr, 
				0,0, 
				sx,sy,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
		}
	}
}

static INTERRUPT_GEN( panicr_interrupt )
{
	if (cpu_getiloops())
		cpunum_set_input_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc8/4);	/* VBL */
	else
		cpunum_set_input_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc4/4);	/* VBL */
}

INPUT_PORTS_START( panicr )

INPUT_PORTS_END

static MACHINE_DRIVER_START( panicr )
	MDRV_CPU_ADD(V20,16000000/2) /* Sony 8623h9 CXQ70116D-8 (V20 compatible) */
	MDRV_CPU_PROGRAM_MAP(panicr_map,0)
	
	MDRV_CPU_VBLANK_INT(panicr_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(panicr)
	MDRV_VIDEO_UPDATE(panicr)

MACHINE_DRIVER_END

ROM_START( panicr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v20 main cpu */
	ROM_LOAD16_BYTE("2.19m",   0x0f0000, 0x08000, CRC(3d48b0b5) SHA1(a6e8b38971a8964af463c16f32bb7dbd301dd314) )
	ROM_LOAD16_BYTE("1.19n",   0x0f0001, 0x08000, CRC(674131b9) SHA1(63499cd5ad39e79e70f3ba7060680f0aa133f095) )
	
	ROM_REGION( 0x18000, REGION_CPU3, 0 )	//seibu sound system
	ROM_LOAD( "t5182.bin", 0x0000, 0x2000, NO_DUMP )  
	ROM_LOAD( "22d.bin",   0x010000, 0x08000, CRC(eb1a46e1) SHA1(278859ae4bca9f421247e646d789fa1206dcd8fc) ) //banked?
		
	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE |ROMREGION_INVERT)
	ROM_LOAD( "13f.bin", 0x000000, 0x2000, CRC(4e6b3c04) SHA1(f388969d5d822df0eaa4d8300cbf9cee47468360) )
	ROM_LOAD( "15f.bin", 0x002000, 0x2000, CRC(d735b572) SHA1(edcdb6daec97ac01a73c5010727b1694f512be71) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2a.bin", 0x000000, 0x20000, CRC(3ac0e5b4) SHA1(96b8bdf02002ec8ce87fd47fd21f7797a79d79c9) )
	ROM_LOAD( "2b.bin", 0x020000, 0x20000, CRC(567d327b) SHA1(762b18ef1627d71074ba02b0eb270bd9a01ac0d8) )
	ROM_LOAD( "2c.bin", 0x040000, 0x20000, CRC(cd77ec79) SHA1(94b61b7d77c016ae274eddbb1e66e755f312e11d) )
	ROM_LOAD( "2d.bin", 0x060000, 0x20000, CRC(218d2c3e) SHA1(9503b3b67e71dc63448aed7815845b844e240afe) )
		
	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2j.bin", 0x000000, 0x20000, CRC(80f05923) SHA1(5c886446fd77d3c39cb4fa43ea4beb8c89d20636) )
	ROM_LOAD( "2k.bin", 0x020000, 0x20000, CRC(35f07bca) SHA1(54e6f82c2e6e1373c3ac1c6138ef738e5a0be6d0) )
		
	ROM_REGION( 0x08000, REGION_USER1, 0 ) 
	ROM_LOAD( "5d.bin", 0x00000, 0x4000, CRC(f3466906) SHA1(42b512ba93ba7ac958402d1871c5ae015def3501) ) //tilemaps ?
	ROM_LOAD( "7d.bin", 0x04000, 0x4000, CRC(8032c1e9) SHA1(fcc8579c0117ebe9271cff31e14a30f61a9cf031) ) //sttribute maps ?
	
	ROM_REGION( 0x0800,  REGION_PROMS, 0 )
	ROM_LOAD( "10j.bpr", 0x00000, 0x100, CRC(1dd80ee1) SHA1(2d634e75666b919446e76fd35a06af27a1a89707) )
	ROM_LOAD( "10l.bpr", 0x00100, 0x100, CRC(f3f29695) SHA1(2607e96564a5e6e9a542377a01f399ea86a36c48) )
	ROM_LOAD( "12d.bpr", 0x00200, 0x100, CRC(0df8aa3c) SHA1(5149265d788ea4885793b0786f765524b4745f04) )
	ROM_LOAD( "8a.bpr",  0x00300, 0x100, CRC(908684a6) SHA1(82d9cb8aed576d1132615b5341c36ef51856b3a6) )
	ROM_LOAD( "a.15c",   0x00400, 0x100, CRC(c75772bc) SHA1(ec84052aedc1d53f9caba3232ffff17de69561b2) )
	ROM_LOAD( "b.14c",   0x00500, 0x100, CRC(145d1e0d) SHA1(8073fd176a1805552a5ac00ca0d9189e6e8936b1) )
	ROM_LOAD( "c.13c",   0x00600, 0x100, CRC(11c11bbd) SHA1(73663b2cf7269a62011ee067a026269ce0c15a7c) )
	ROM_LOAD( "d.9b",    0x00700, 0x100, CRC(f99cac4b) SHA1(b4e6d0e0186fe186e747a9f6857b97591948c682) )
ROM_END

GAMEX( 1986, panicr,  0,       panicr,  panicr,  0, ROT270, "Seibu", "Panic Road", GAME_NO_SOUND|GAME_NOT_WORKING )
