/* Gals Panic 3 */

/*

Gals Panic 3 (JPN Ver.)
(c)1995 Kaneko

CPU:	68000-16
Sound:	YMZ280B-F
OSC:	28.6363MHz
	33.3333MHz
EEPROM:	93C46
Chips.:	GRAP2 x3
	APRIO-GL
	BABY004
	GCNT2
	TBSOP01
	CG24173 6186
	CG24143 4181


G3P0J1.71     prg.
G3P1J1.102

GP340000.123  chr.
GP340100.122
GP340200.121
GP340300.120
G3G0J0.101
G3G1J0.100

G3D0X0.134

GP320000.1    OBJ chr.

GP310000.41   sound data
GP310100.40


--- Team Japump!!! ---
http://www.rainemu.com/japump/
http://japump.i.am/
Dumped by Uki
10/22/2000

*/

#include "driver.h"

extern data32_t* skns_spc_regs;

/***************************************************************************

 vidhrdw

***************************************************************************/

static data16_t *galpani3_sprregs, *galpani3_spriteram;

#if 0
INTERRUPT_GEN( galpani3_vblank ) // 2, 3, 5 ?
{
	if (!cpu_getiloops())
		cpunum_set_input_line(0, 2, HOLD_LINE);
	else
		cpunum_set_input_line(0, 3, HOLD_LINE);
}
#endif

#include "vidhrdw/generic.h"


VIDEO_START(galpani3)
{
	/* so we can use suprnova.c */
	buffered_spriteram32 = auto_malloc ( 0x4000 );
	spriteram_size = 0x4000;
	skns_spc_regs = auto_malloc (0x40);

	return 0;
}

extern void skns_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect );


VIDEO_UPDATE(galpani3)
{
	fillbitmap(bitmap, get_black_pen(), cliprect);

	skns_drawsprites (bitmap,cliprect);
}

INPUT_PORTS_START( galpani3 ) /* everything is assumption except IN3 */

	PORT_START	// IN0 - Player Controls
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START	// IN1 - Player Controls
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START	// IN2 - Coins
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - DSW provided by the MCU - $200386.b <- $400200
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


WRITE16_HANDLER( galpani3_suprnova_sprite32_w )
{
	COMBINE_DATA(&galpani3_spriteram[offset]);

	/* put in buffered_spriteram32 for suprnova.c */
	offset>>=1;
	buffered_spriteram32[offset]=(galpani3_spriteram[offset*2+1]<<16) | (galpani3_spriteram[offset*2]);
}

WRITE16_HANDLER( galpani3_suprnova_sprite32regs_w )
{
	COMBINE_DATA(&galpani3_sprregs[offset]);

	/* put in skns_spc_regs for suprnova.c */
	offset>>=1;
	skns_spc_regs[offset]=(galpani3_sprregs[offset*2+1]<<16) | (galpani3_sprregs[offset*2]);
}



/***************************************************************************

							MCU Code Simulation
				(follows the implementation of kaneko16.c)

***************************************************************************/
static data16_t *mcu_ram, galpani3_mcu_com[4];

void galpani3_mcu_run(void)
{
	data16_t mcu_command = mcu_ram[0x0010/2];
	data16_t mcu_offset  = mcu_ram[0x0012/2] / 2; // 'param1' where MCU writes result
	data16_t mcu_data    = mcu_ram[0x0014/2];

	logerror("(PC=%06X): MCU executed command : %04X %04X\n",activecpu_get_pc(),mcu_command,mcu_offset*2);

	/* the only MCU commands found in program code are:
		 0x04: protection: provide code/data, that's exactly where we are stuck !!!
		 0x03: read DSW
		 0x02: load NVRAM settings \ ATMEL AT93C46 chip,
		 0x42: save NVRAM settings / 128 bytes serial EEPROM
 	*/
	switch (mcu_command >> 8)
	{
		case 0x03: // $388D2
		{
			/* MCU writes Coins? status */
			mcu_ram[mcu_offset] = readinputport(3);
		}
		break;

		case 0x02: // $38950 - load NVRAM settings
		{
			/* NOTE: code @ $38B46 & $38ab8 does exactly what is checked after MCU command
			         so that's what we'll mimic here... probably the initial NVRAM settings */
			int i;

			/* MCU writes 128 bytes to shared ram: last byte is the byte-sum */
			/* first 32 bytes (header): 0x8BE08E71.L, then the string "95/06/30 Gals Panic3Ver 0.95"; */
			mcu_ram[mcu_offset +  0] = 0x8BE0; mcu_ram[mcu_offset +  1] = 0x8E71;
			mcu_ram[mcu_offset +  2] = 0x3935; mcu_ram[mcu_offset +  3] = 0x2F30;
			mcu_ram[mcu_offset +  4] = 0x362F; mcu_ram[mcu_offset +  5] = 0x3330;
			mcu_ram[mcu_offset +  6] = 0x2047; mcu_ram[mcu_offset +  7] = 0x616C;
			mcu_ram[mcu_offset +  8] = 0x7320; mcu_ram[mcu_offset +  9] = 0x5061;
			mcu_ram[mcu_offset + 10] = 0x6E69; mcu_ram[mcu_offset + 11] = 0x6333;
			mcu_ram[mcu_offset + 12] = 0x5665; mcu_ram[mcu_offset + 13] = 0x7220;
			mcu_ram[mcu_offset + 14] = 0x302E; mcu_ram[mcu_offset + 15] = 0x3935;
			/* next 11 bytes - initial NVRAM settings */
			mcu_ram[mcu_offset + 16] = 0x0001; mcu_ram[mcu_offset + 17] = 0x0101;
			mcu_ram[mcu_offset + 18] = 0x0100; mcu_ram[mcu_offset + 19] = 0x0208;
			mcu_ram[mcu_offset + 20] = 0x02FF; mcu_ram[mcu_offset + 21] = 0x0000;
			/* rest is zeroes */
			for (i=22;i<63;i++)
				mcu_ram[mcu_offset + i] = 0;
			/* and sum is $0c.b */
			mcu_ram[mcu_offset + 63] = 0x000c;
		}
		break;

		case 0x04: // $38842 - provides code/data?
		{
			switch(mcu_data)
			{
				case 0x00: /* $1a9c - provides code @ $40f000 */
					break;
				case 0x01: /* $1aa8 - provides data @ $400400 */
					break;
			}
		}
		break;

		case 0x42: // $389ee - save NVRAM settings
		{
			// found, TODO: trace call in code !!!
		}
		break;

		default:
			logerror("UNKNOWN COMMAND\n");
	}
}

/*
  MCU doesn't execute exactly as it is coded right know (ala jchan):
   * com0=com1=0xFFFF -> command to execute
   * com2=com3=0xFFFF -> status reading only
*/
#define GALPANI3_MCU_COM_W(_n_) \
WRITE16_HANDLER( galpani3_mcu_com##_n_##_w ) \
{ \
	COMBINE_DATA(&galpani3_mcu_com[_n_]); \
	if (galpani3_mcu_com[0] != 0xFFFF)	return; \
	if (galpani3_mcu_com[1] != 0xFFFF)	return; \
	if (galpani3_mcu_com[2] != 0xFFFF)	return; \
	if (galpani3_mcu_com[3] != 0xFFFF)	return; \
\
	memset(galpani3_mcu_com, 0, 4 * sizeof( data16_t ) ); \
	galpani3_mcu_run(); \
}

GALPANI3_MCU_COM_W(0)
GALPANI3_MCU_COM_W(1)
GALPANI3_MCU_COM_W(2)
GALPANI3_MCU_COM_W(3)

READ16_HANDLER( galpani3_mcu_status_r )
{
	logerror("cpu #%d (PC=%06X): read mcu status\n", cpu_getactivecpu(), activecpu_get_previouspc());
	return 0;
}



/* NOTE: $24c8 = 'Gals Panic 3 v0.96 95/08/29(Tue)' */

static ADDRESS_MAP_START( galpani3_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM

	AM_RANGE(0x200000, 0x20ffff) AM_RAM // area [B]
	AM_RANGE(0x280000, 0x287fff) AM_RAM // area [A]

	AM_RANGE(0x300000, 0x303fff) AM_RAM AM_BASE(&galpani3_spriteram) AM_WRITE(galpani3_suprnova_sprite32_w)
	AM_RANGE(0x380000, 0x38003f) AM_RAM AM_BASE(&galpani3_sprregs) AM_WRITE(galpani3_suprnova_sprite32regs_w)

	AM_RANGE(0x400000, 0x40ffff) AM_RAM AM_BASE(&mcu_ram) // area [C]

	AM_RANGE(0x580000, 0x580001) AM_WRITE(galpani3_mcu_com0_w)	// ] see $387e8: these 2 locations are written (w.#$ffff)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(galpani3_mcu_com1_w)	// ] then bit #0 of $780000.l is tested: 0 = OK!
	AM_RANGE(0x680000, 0x680001) AM_WRITE(galpani3_mcu_com2_w)	// ] see $387e8: these 2 locations are written (w.#$ffff)
	AM_RANGE(0x700000, 0x700001) AM_WRITE(galpani3_mcu_com3_w)	// ] then bit #0 of $780000.l is tested: 0 = OK!
	AM_RANGE(0x780000, 0x780003) AM_READ(galpani3_mcu_status_r)

//	AM_RANGE(0x800C02, 0x800C03) AM_RAM // ??? see subroutine $3a03e
//	AM_RANGE(0xa00C02, 0xa00C03) AM_RAM // ??? see subroutine $3a03e
//	AM_RANGE(0xc00C02, 0xc00C03) AM_RAM // ??? see subroutine $3a03e

	AM_RANGE(0x880000, 0x8801ff) AM_RAM // area [G]
	AM_RANGE(0x900000, 0x97ffff) AM_RAM // area [D]
	AM_RANGE(0xa80000, 0xa801ff) AM_RAM // area [H]
	AM_RANGE(0xb00000, 0xb7ffff) AM_RAM // area [E]
	AM_RANGE(0xc80000, 0xc801ff) AM_RAM // area [I]
	AM_RANGE(0xd00000, 0xd7ffff) AM_RAM // area [F]
	AM_RANGE(0xe00000, 0xe7ffff) AM_RAM // area [J]

	AM_RANGE(0xf00040, 0xf00041) AM_RAM // probably watchdog
ADDRESS_MAP_END

/* probably sprite registers (suprnova.c), very similar to jchan (thanks Haze!)

cpu #0 (PC=00001868): unmapped program memory word write to 00380000 = 0040 & FFFF
cpu #0 (PC=0000186E): unmapped program memory word write to 00380004 = 0000 & FFFF
cpu #0 (PC=00001874): unmapped program memory word write to 00380008 = 0000 & FFFF
cpu #0 (PC=0000187A): unmapped program memory word write to 0038000C = 0000 & FFFF
cpu #0 (PC=00001880): unmapped program memory word write to 00380010 = 0000 & FFFF
cpu #0 (PC=00001886): unmapped program memory word write to 00380014 = 0000 & FFFF
cpu #0 (PC=0000188C): unmapped program memory word write to 00380018 = 0000 & FFFF
cpu #0 (PC=00001892): unmapped program memory word write to 0038001C = 0000 & FFFF
cpu #0 (PC=00001898): unmapped program memory word write to 00380020 = 0000 & FFFF
cpu #0 (PC=0000189E): unmapped program memory word write to 00380024 = 0000 & FFFF
cpu #0 (PC=000018A4): unmapped program memory word write to 00380028 = 0000 & FFFF
cpu #0 (PC=000018AA): unmapped program memory word write to 0038002C = 0000 & FFFF
cpu #0 (PC=000018B0): unmapped program memory word write to 00380030 = 0000 & FFFF
cpu #0 (PC=000018B6): unmapped program memory word write to 00380034 = 0000 & FFFF
*/

/* none of the gfx are tile based? */
static struct GfxLayout charlayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 8*32+4, 8*32+0, 8*32+12, 8*32+8, 8*32+20, 8*32+16, 8*32+28, 8*32+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 16*32,17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 1  },
	{ REGION_GFX2, 0, &charlayout,   0, 1  },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( galpani3 )
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)	 // ? (from which clock?)
	MDRV_CPU_PROGRAM_MAP(galpani3_map,0)
//	MDRV_CPU_VBLANK_INT(galpani3_vblank, 2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(galpani3)
	MDRV_VIDEO_UPDATE(galpani3)
MACHINE_DRIVER_END

ROM_START( galpani3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "g3p0j1.71",  0x000000, 0x080000, CRC(52893326) SHA1(78fdbf3436a4ba754d7608fedbbede5c719a4505) )
	ROM_LOAD16_BYTE( "g3p1j1.102", 0x000001, 0x080000, CRC(05f935b4) SHA1(81e78875585bcdadad1c302614b2708e60563662) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* MCU data rom */
	ROM_LOAD( "g3d0x0.134", 0x000000, 0x020000, CRC(4ace10f9) SHA1(d19e4540d535ce10d23cb0844be03a3239b3402e) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gp310100.40", 0x000000, 0x200000, CRC(6a0b1d12) SHA1(11fed80b96d07fddb27599743991c58c12c048e0) )
	ROM_LOAD( "gp310000.41", 0x200000, 0x100000, CRC(641062ef) SHA1(c8902fc46319eac94b3f95d18afa24bd895078d6) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites? */
	ROM_LOAD( "gp320000.1", 0x000000, 0x200000, CRC(a0112827) SHA1(0a6c78d71b75a1d78215aab3104176aa1769b14f) )

	ROM_REGION( 0xa00000, REGION_GFX2, 0 ) /* Tilemaps? */
	ROM_LOAD( "gp340000.123", 0x000000, 0x200000, CRC(a58a26b1) SHA1(832d70cce1b4f04fa50fc221962ff6cc4287cb92) )
	ROM_LOAD( "gp340100.122", 0x200000, 0x200000, CRC(746fe4a8) SHA1(a5126ae9e83d556277d31b166296a708c311a902) )
	ROM_LOAD( "gp340200.121", 0x400000, 0x200000, CRC(e9bc15c8) SHA1(2c6a10e768709d1937d9206970553f4101ce9016) )
	ROM_LOAD( "gp340300.120", 0x600000, 0x200000, CRC(59062eef) SHA1(936977c20d83540c1e0f65d429c7ebea201ef991) )
	ROM_LOAD16_BYTE( "g3g0j0.101", 0x800000, 0x040000, CRC(fbb1e0dc) SHA1(14f6377afd93054aa5dc38af235ae12b932e847f) )
	ROM_LOAD16_BYTE( "g3g1j0.100", 0x800001, 0x040000, CRC(18edb5f0) SHA1(5e2ed0105b3e6037f6116494d3b186a368824171) )
ROM_END

DRIVER_INIT( galpani3 )
{
	memset(galpani3_mcu_com, 0, 4 * sizeof( data16_t) );
}

GAMEX( 1995, galpani3, 0, galpani3, galpani3, 0, ROT90, "Kaneko", "Gals Panic 3", GAME_NOT_WORKING | GAME_NO_SOUND )
