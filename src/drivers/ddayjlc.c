/*

    D-DAY   (c)Jaleco 1984
    preliminary driver by Pierpaolo Prazzoli
----------------------------------------------
    Tomasz Slanina 2004.11.12. :

  - correct sprite decoding
  - DMA  - see below
  - sound
  - interrupts
  - controls

 TODO:
  - everything else


Is it 1984 or 1987 game ?
There's text inside rom "1987.07    BY  ELS"
-------------------------------------------------------------

    DMA copy ?

    Writes to adr. in range $e000-$e008 are usually like this:

    ld hl,$e000
    ld bc,XXxx
    ld de,YYyy
    ld (hl),c       ;($e000),xx
    ld (hl),b       ;($e000),XX
    inc hl
    ld (hl),e       ;($e001),yy
    ld (hl),d       ;($e001),YY
    inc hl
    ld bc,ZZzz
    ld de,QQqq
    ld (hl),c       ;($e002),zz
    ld (hl),b       ;($e002),ZZ
    inc hl
    ld (hl),e       ;($e003),qq
    ld (hl),d       ;($e003),QQ
    ld a,WW
    ld ($e008),a
    ld a,1
    ld ($f803),a
    xor a
    ld ($f083),a    ; write latch
    ld a,1
    ld ($f083),a

where (my guess):

XXxx = src address
YYyy = size | $4000
ZZzz = dst address
QQqq = size | $8000
WW = $73 (always)

It's used in (at least) three places :

- $16e6 RAM -> spriteram
    src = $8d6f
    dst = $9000
    size = $200

- $1cb1 RAM -> RAM

    src = $8432
    dst = $807a
    size = $364

- $1cf4     RAM -> RAM

    src = $805e
    dst = $8432
    size = $380

----------------------------------------------


    CPU  : Z80
    Sound: Z80 AY-3-8910(x2)
    OSC  : 12.000MHz

    -------
    DD-8416
    -------
    ROMs:
    1  - (2764)
    2  |
    3  |
    4  |
    5  |
    6  |
    7  |
    8  |
    9  |
    10 |
    11 /

    -------
    DD-8417
    -------
    ROMs:
    12 - (2732)
    13 /

    14 - (2732)
    15 /

    16 - (2764)
    17 |
    18 |
    19 /


*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ay8910.h"

static int char_bank = 0;
static struct tilemap *bg_tilemap, *fg_tilemap;
static data8_t *bgram;
static int bgadr=0;

static WRITE8_HANDLER( char_bank_w )
{
	char_bank = data;
	tilemap_mark_all_tiles_dirty(fg_tilemap);
}

static WRITE8_HANDLER( ddayjlc_bgram_w )
{
	if(!offset)
		tilemap_set_scrollx(bg_tilemap,0,data);

	if( bgram[offset] != data )
	{
		bgram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

static WRITE8_HANDLER( ddayjlc_videoram_w )
{
	if( videoram[offset] != data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}
static int sound_nmi_enable=0;
static int main_nmi_enable=0;

static WRITE8_HANDLER(sound_nmi_w)
{
	sound_nmi_enable=data;
}
static WRITE8_HANDLER(main_nmi_w)
{
	main_nmi_enable=data;
}



static WRITE8_HANDLER(bg2_w)
{
	bgadr=(bgadr&0xfe)|(data&1);
}

static WRITE8_HANDLER(bg1_w)
{
	bgadr=(bgadr&0xfd)|((data&1)<<1);
}

static WRITE8_HANDLER(bg0_w)
{
	bgadr=(bgadr&0xfb)|((data&1)<<1);
}

static READ8_HANDLER(custom_r)
{
	UINT8 *ROM = memory_region(REGION_USER2);
	return ROM[offset|(bgadr<<11)];
}

#if 0
static WRITE8_HANDLER(test_w)
{
		//printf("W %x %x %x\n",offset,data,activecpu_get_previouspc());
}

static READ8_HANDLER(test_r)
{
		//printf("R %x %x\n",offset,activecpu_get_previouspc());
		UINT8 *ROM = memory_region(REGION_USER1);
		return ROM[offset];
}
#endif

static WRITE8_HANDLER( sound_w )
{
	soundlatch_w(offset,data);
	cpunum_set_input_line_and_vector(1,0,HOLD_LINE,0xff);
}


static int e00x_l[4];
static int e00x_d[4][2];
static int e008_d;

static WRITE8_HANDLER( e00x_w )
{
	e00x_d[offset][e00x_l[offset]]=data;
	e00x_l[offset]^=1;
}

static WRITE8_HANDLER( e008_w )
{
	e008_d=data;
}

static WRITE8_HANDLER( f083_w )
{
	if(!data)
	{
		//printf("DMA? %.2x%.2x %.2x%.2x %.2x%.2x %.2x%.2x %.2x @%.4x\n", e00x_d[0][1],e00x_d[0][0],e00x_d[1][1],e00x_d[1][0],e00x_d[2][1],e00x_d[2][0],e00x_d[3][1],e00x_d[3][0],e008_d,activecpu_get_previouspc());
		{
			int src=e00x_d[0][1]*256+e00x_d[0][0];
			int dst=e00x_d[2][1]*256+e00x_d[2][0];
			int size=(e00x_d[1][1]*256+e00x_d[1][0])&0x3ff;

			int i;
			for(i=0;i<size;i++)
			{
				program_write_byte(dst++, program_read_byte(src++));
			}
		}
		e00x_l[0]=0;
		e00x_l[1]=0;
		e00x_l[2]=0;
		e00x_l[3]=0;
	}
}

static ADDRESS_MAP_START( main_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9000, 0x93ff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE(videoram_r, ddayjlc_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9800, 0x9fff) AM_READWRITE(MRA8_RAM, ddayjlc_bgram_w) AM_BASE(&bgram) /* 9800-981f - videoregs */
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(MWA8_RAM) AM_READ(custom_r)

	AM_RANGE(0xa800, 0xdfff) AM_RAM //AM_READ(test_r) AM_WRITE(test_w)  // other ROMs ?

	AM_RANGE(0xe000, 0xe003) AM_WRITE(e00x_w)
	AM_RANGE(0xf083, 0xf083) AM_WRITE(f083_w)
	AM_RANGE(0xe008, 0xe008) AM_WRITE(e008_w)

	AM_RANGE(0xf000, 0xf000) AM_WRITE(sound_w)
	AM_RANGE(0xf100, 0xf100) AM_WRITE(MWA8_NOP) // ff,00,ff,00

	AM_RANGE(0xf080, 0xf080) AM_WRITE(char_bank_w)

	AM_RANGE(0xf081, 0xf081) AM_WRITE(MWA8_NOP) //1 or 0

	AM_RANGE(0xf084, 0xf084) AM_WRITE(bg0_w)
	AM_RANGE(0xf085, 0xf085) AM_WRITE(bg1_w)
	AM_RANGE(0xf086, 0xf086) AM_WRITE(bg2_w)

	AM_RANGE(0xf101, 0xf101) AM_WRITE(main_nmi_w)
	AM_RANGE(0xf102, 0xf105) AM_WRITE(MWA8_NOP) // $7eac 4 bit value (each bit to one adr ($f102,03,04,05)) top bits of *something* ?

	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_0_r)
	AM_RANGE(0xf100, 0xf100) AM_READ(input_port_1_r)//protection ?
	AM_RANGE(0xf180, 0xf180) AM_READ(input_port_2_r)
	AM_RANGE(0xf200, 0xf200) AM_READ(input_port_3_r)
	AM_RANGE(0xff00, 0xffff) AM_RAM /* $49a */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x7000, 0x7000) AM_WRITE(sound_nmi_w)

	AM_RANGE(0x3000, 0x3000) AM_READ(AY8910_read_port_0_r) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x5000, 0x5000) AM_READ(AY8910_read_port_1_r) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_control_port_1_w)

ADDRESS_MAP_END

INPUT_PORTS_START( ddayjlc )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DIPSW-1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, "Extend" )
	PORT_DIPSETTING(    0x00, "30K" )
	PORT_DIPSETTING(    0x20, "50K" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DIPSW-2 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf8, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xf8, DEF_STR( On ) )

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2)},
	{ 0+8*8, 1+8*8, 2+8*8, 3+8*8, 4+8*8, 5+8*8, 6+8*8, 7+8*8,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,0*8+2*8*8, 1*8+2*8*8, 2*8+2*8*8, 3*8+2*8*8, 4*8+2*8*8, 5*8+2*8*8, 6*8+2*8*8, 7*8+2*8*8},
	16*16,
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,   0, 16 },
	{ REGION_GFX2, 0, &charlayout,     0, 16 },
	{ REGION_GFX3, 0, &charlayout,     0, 16 },

	{ -1 }
};

static void get_tile_info_bg(int tile_index)
{
	int code = bgram[tile_index];
	SET_TILE_INFO(2, code, 0, 0)
	//tile_index+0x400 = attributes ?
}

static void get_tile_info_fg(int tile_index)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(1, code + char_bank * 0x100, 0, 0)
}

VIDEO_START( ddayjlc )
{
	bg_tilemap = tilemap_create(get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	fg_tilemap = tilemap_create(get_tile_info_fg,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if( !bg_tilemap || !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_UPDATE( ddayjlc )
{
	int i;
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	for (i = 0; i < 0x400; i += 4)
	{
		int flags = spriteram[i + 2];
		int y = spriteram[i + 0];//-3*8 ;
		int code = spriteram[i + 1];
		int x = spriteram[i + 3];//-8 ;
		int xflip = flags&0x80;
		int yflip = (code&0x80)^0x80;

		code=(code&0x7f)|((flags&0x30)<<3);//0x30 ? 0x10(tested)

		drawgfx(bitmap, Machine->gfx[0], code, 1, xflip, yflip, x, y, cliprect, TRANSPARENCY_PEN, 0);
	}

	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

static struct AY8910interface ay8910_interface =
{
	soundlatch_r
};

static INTERRUPT_GEN( ddayjlc_interrupt )
{
	if(main_nmi_enable)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static INTERRUPT_GEN( ddayjlc_snd_interrupt )
{
	if(sound_nmi_enable)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( ddayjlc )
	MDRV_CPU_ADD(Z80,12000000/3)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(main_cpu,0)
	MDRV_CPU_VBLANK_INT(ddayjlc_interrupt,1)

	MDRV_CPU_ADD(Z80, 12000000/3)     /* ? MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)
	MDRV_CPU_VBLANK_INT(ddayjlc_snd_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(ddayjlc)
	MDRV_VIDEO_UPDATE(ddayjlc)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 12000000/4)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(AY8910, 12000000/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( ddayjlc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",  0x0000, 0x2000, CRC(dbfb8772) SHA1(1fbc9726d0cd1f8781ced2f8233107b65b9bdb1a) )
	ROM_LOAD( "2",  0x2000, 0x2000, CRC(f40ea53e) SHA1(234ef686d3e9fe12aceada7098c4cc53e56eb1a3) )
	ROM_LOAD( "3",  0x4000, 0x2000, CRC(0780ef60) SHA1(9247af38acbaea0f78892fc50081b2400abbdc1f) )
	ROM_LOAD( "4",  0x6000, 0x2000, CRC(75991a24) SHA1(175f505da6eb80479a70181d6aed01130f6a64cc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11", 0x0000, 0x2000, CRC(fe4de019) SHA1(16c5402e1a79756f8227d7e99dd94c5896c57444) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "16", 0x0000, 0x2000, CRC(a167fe9a) SHA1(f2770d93ee5ae4eb9b3bcb052e14e36f53eec707) )
	ROM_LOAD( "17", 0x2000, 0x2000, CRC(13ffe662) SHA1(2ea7855a14a4b8429751bae2e670e77608f93406) )
	ROM_LOAD( "18", 0x4000, 0x2000, CRC(debe6531) SHA1(34b3b70a1872527266c664b2a82014d028a4ff1e) )
	ROM_LOAD( "19", 0x6000, 0x2000, CRC(5816f947) SHA1(2236bed3e82980d3e7de3749aef0fbab042086e6) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14", 0x0000, 0x1000, CRC(2c0e9bbe) SHA1(e34ab774d2eb17ddf51af513dbcaa0c51f8dcbf7) )
	ROM_LOAD( "15", 0x1000, 0x1000, CRC(a6eeaa50) SHA1(052cd3e906ca028e6f55d0caa1e1386482684cbf) )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "12", 0x0000, 0x1000, CRC(7f7afe80) SHA1(e8a549b8a8985c61d3ba452e348414146f2bc77e) )
	ROM_LOAD( "13", 0x1000, 0x1000, CRC(f169b93f) SHA1(fb0617162542d688503fc6618dd430308e259455) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )
	ROM_LOAD( "5",  0x0000, 0x2000, CRC(299b05f2) SHA1(3c1804bccb514bada4bed68a6af08db63a8f1b19) )
	ROM_LOAD( "6",  0x2000, 0x2000, CRC(38ae2616) SHA1(62c96f32532f0d7e2cf1606a303d81ebb4aada7d) ) // 1xxxxxxxxxxxx = 0xFF
	ROM_LOAD( "9",  0x6000, 0x2000, CRC(ccb82f09) SHA1(37c23f13aa0728bae82dba9e2858a8d81fa8afa5) ) // FIXED BITS (xx00xxxx)
	ROM_LOAD( "10", 0x4000, 0x2000, CRC(5452aba1) SHA1(03ef47161d0ab047c8675d6ffd3b7acf81f74721) ) // FIXED BITS (1xxxxxxx)

	ROM_REGION( 0x04000, REGION_USER2, 0 )
	ROM_LOAD( "7",  0x0000, 0x2000, CRC(4210f6ef) SHA1(525d8413afabf97cf1d04ee9a3c3d980b91bde65) )
	ROM_LOAD( "8",  0x2000, 0x2000, CRC(91a32130) SHA1(cbcd673b47b672b9ce78c7354dacb5964a81db6f) )
ROM_END


static DRIVER_INIT( ddayjlc )
{
#define repack(n)\
		dst[newadr+0+n] = src[oldaddr+0+n];\
		dst[newadr+1+n] = src[oldaddr+1+n];\
		dst[newadr+2+n] = src[oldaddr+2+n];\
		dst[newadr+3+n] = src[oldaddr+3+n];\
		dst[newadr+4+n] = src[oldaddr+4+n];\
		dst[newadr+5+n] = src[oldaddr+5+n];\
		dst[newadr+6+n] = src[oldaddr+6+n];\
		dst[newadr+7+n] = src[oldaddr+7+n];\
		dst[newadr+8+n] = src[oldaddr+0+0x2000+n];\
		dst[newadr+9+n] = src[oldaddr+1+0x2000+n];\
		dst[newadr+10+n] = src[oldaddr+2+0x2000+n];\
		dst[newadr+11+n] = src[oldaddr+3+0x2000+n];\
		dst[newadr+12+n] = src[oldaddr+4+0x2000+n];\
		dst[newadr+13+n] = src[oldaddr+5+0x2000+n];\
		dst[newadr+14+n] = src[oldaddr+6+0x2000+n];\
		dst[newadr+15+n] = src[oldaddr+7+0x2000+n];\
		dst[newadr+16+n] = src[oldaddr+0+8+n];\
		dst[newadr+17+n] = src[oldaddr+1+8+n];\
		dst[newadr+18+n] = src[oldaddr+2+8+n];\
		dst[newadr+19+n] = src[oldaddr+3+8+n];\
		dst[newadr+20+n] = src[oldaddr+4+8+n];\
		dst[newadr+21+n] = src[oldaddr+5+8+n];\
		dst[newadr+22+n] = src[oldaddr+6+8+n];\
		dst[newadr+23+n] = src[oldaddr+7+8+n];\
		dst[newadr+24+n] = src[oldaddr+0+0x2008+n];\
		dst[newadr+25+n] = src[oldaddr+1+0x2008+n];\
		dst[newadr+26+n] = src[oldaddr+2+0x2008+n];\
		dst[newadr+27+n] = src[oldaddr+3+0x2008+n];\
		dst[newadr+28+n] = src[oldaddr+4+0x2008+n];\
		dst[newadr+29+n] = src[oldaddr+5+0x2008+n];\
		dst[newadr+30+n] = src[oldaddr+6+0x2008+n];\
		dst[newadr+31+n] = src[oldaddr+7+0x2008+n];

	UINT8 *ROM = memory_region(REGION_CPU1);
	//protection (?) patch
	ROM[0x7e5e] = 0;
	ROM[0x7e5f] = 0;
	ROM[0x7e60] = 0;

	//patched cpir (reads from not mapped adresses , whole adr space (000-ffff), REMOVE it)
	ROM[0x13de] = 0;
	ROM[0x13df] = 0;

	{
		UINT32 oldaddr, newadr, length,j;
		UINT8 *src, *dst, *temp;
		temp = malloc(0x10000);
		src = temp;
		dst = memory_region(REGION_GFX1);
		length = memory_region_length(REGION_GFX1);
		memcpy(src, dst, length);
		newadr=0;
		oldaddr=0;
		for (j = 0; j < length/2; j+=32)
		{
			repack(0);
			repack(0x4000)
			newadr+=32;
			oldaddr+=16;
		}
	}

}

GAMEX( 1984, ddayjlc, 0, ddayjlc, ddayjlc, ddayjlc, ROT90, "Jaleco", "D-Day (Jaleco)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
