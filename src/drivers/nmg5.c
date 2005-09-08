/*

 Multi 5 / New Multi Game 5     (c) 1998 Yun Sung
 Search Eye                     (c) 1999 Yun Sung
 Puzzle Club                    (c) 2000 Yun Sung

 driver by Pierpaolo Prazzoli

- nmg5:
  Press player 2 button 2 in service mode to enable image test

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

static tilemap *fg_tilemap,*bg_tilemap;
UINT16 *nmg5_bitmap;
static UINT16 *fgvideoram,*bgvideoram,*scroll_ram,*sprite_pri;
static UINT16 gfx_bank = 0;
static UINT16 input_data;

static WRITE16_HANDLER( fgvideoram_w )
{
	int oldword = fgvideoram[offset];
	COMBINE_DATA(&fgvideoram[offset]);
	if (oldword != fgvideoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

static WRITE16_HANDLER( bgvideoram_w )
{
	int oldword = bgvideoram[offset];
	COMBINE_DATA(&bgvideoram[offset]);
	if (oldword != bgvideoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE16_HANDLER( nmg5_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static READ16_HANDLER( prot_r )
{
	return 0x10 | input_data;
}

static WRITE16_HANDLER( prot_w )
{
	input_data = data & 0xf;
}

static WRITE16_HANDLER( gfx_bank_w )
{
	if( gfx_bank != (data & 3) )
	{
		gfx_bank = data & 3;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static WRITE8_HANDLER( oki_banking_w )
{
	OKIM6295_set_bank_base(0, (data & 1) ? 0x40000 : 0 );
}

/*******************************************************************

                            Main Cpu

********************************************************************/

static ADDRESS_MAP_START( nmg5_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x120000, 0x12ffff) AM_RAM
	AM_RANGE(0x140000, 0x1407ff) AM_RAM AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x160000, 0x1607ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x180000, 0x180001) AM_WRITE(nmg5_soundlatch_w)
	AM_RANGE(0x180002, 0x180003) AM_WRITENOP
	AM_RANGE(0x180004, 0x180005) AM_READWRITE(prot_r, prot_w)
	AM_RANGE(0x180006, 0x180007) AM_WRITE(gfx_bank_w)
	AM_RANGE(0x180008, 0x180009) AM_READ(input_port_0_word_r)
	AM_RANGE(0x18000a, 0x18000b) AM_READ(input_port_1_word_r)
	AM_RANGE(0x18000c, 0x18000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x18000e, 0x18000f) AM_WRITE(MWA16_RAM) AM_BASE(&sprite_pri)
	AM_RANGE(0x300002, 0x300009) AM_WRITE(MWA16_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x30000a, 0x30000f) AM_WRITENOP
	AM_RANGE(0x320000, 0x321fff) AM_RAM AM_WRITE(bgvideoram_w) AM_BASE(&bgvideoram)
	AM_RANGE(0x322000, 0x323fff) AM_RAM AM_WRITE(fgvideoram_w) AM_BASE(&fgvideoram)
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE(&nmg5_bitmap)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pclubys_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x440000, 0x4407ff) AM_RAM AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x460000, 0x4607ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x480000, 0x480001) AM_WRITE(nmg5_soundlatch_w)
	AM_RANGE(0x480002, 0x480003) AM_WRITENOP
	AM_RANGE(0x480004, 0x480005) AM_READWRITE(prot_r, prot_w)
	AM_RANGE(0x480006, 0x480007) AM_WRITE(gfx_bank_w)
	AM_RANGE(0x480008, 0x480009) AM_READ(input_port_0_word_r)
	AM_RANGE(0x48000a, 0x48000b) AM_READ(input_port_1_word_r)
	AM_RANGE(0x48000c, 0x48000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x48000e, 0x48000f) AM_WRITE(MWA16_RAM) AM_BASE(&sprite_pri)
	AM_RANGE(0x500002, 0x500009) AM_WRITE(MWA16_RAM) AM_BASE(&scroll_ram)
	AM_RANGE(0x520000, 0x521fff) AM_RAM AM_WRITE(bgvideoram_w) AM_BASE(&bgvideoram)
	AM_RANGE(0x522000, 0x523fff) AM_RAM AM_WRITE(fgvideoram_w) AM_BASE(&fgvideoram)
	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE(&nmg5_bitmap)
ADDRESS_MAP_END

/*******************************************************************

                            Sound Cpu

********************************************************************/

static ADDRESS_MAP_START( nmg5_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xcfff) AM_ROM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pclubys_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xf7ff) AM_ROM
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(oki_banking_w)
	AM_RANGE(0x10, 0x10) AM_READWRITE(YM3812_status_port_0_r, YM3812_control_port_0_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x18, 0x18) AM_READ(soundlatch_r)
	AM_RANGE(0x1c, 0x1c) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( nmg5 )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0001, 0x0001, "Game Title" )
	PORT_DIPSETTING(      0x0001, "Multi 5" )
	PORT_DIPSETTING(      0x0000, "New Multi Game 5" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0018, 0x0018, "License" )
	PORT_DIPSETTING(      0x0000, "New Impeuropex Corp. S.R.L." )
	PORT_DIPSETTING(      0x0008, "BNS Enterprises" )
	PORT_DIPSETTING(      0x0010, "Nova Games" )
	PORT_DIPSETTING(      0x0018, DEF_STR( None ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_3C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_4C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_2C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_1C ) )	PORT_CONDITION("DSW",0x4000,PORTCOND_EQUALS,0x00)
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) ) /* doesn't work */
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Coin Type" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START	/* Coins */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0050, IP_ACTIVE_HIGH, IPT_SPECIAL ) // otherwise it doesn't boot
	PORT_BIT( 0xffac, IP_ACTIVE_LOW,  IPT_UNUSED )  // tested in service mode

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( searchey )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, "Time Speed" )
	PORT_DIPSETTING(      0x0000, "Fastest" )
	PORT_DIPSETTING(      0x0001, "Fast" )
	PORT_DIPSETTING(      0x0002, "Slow" )
	PORT_DIPSETTING(      0x0003, "Slowest" )
	PORT_DIPNAME( 0x000c, 0x000c, "Number of Helps" )
	PORT_DIPSETTING(      0x000c, "1" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0030, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x2000, "Korean" )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Items to Start Finding" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )

	PORT_START	/* Coins */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( pclubys )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Nude Girls" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "4" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


INLINE void get_tile_info(int tile_index,UINT16 *vram,int color)
{
	SET_TILE_INFO(0,vram[tile_index] | (gfx_bank << 16),color,0)
}

static void fg_get_tile_info(int tile_index) { get_tile_info(tile_index,fgvideoram, 0); }
static void bg_get_tile_info(int tile_index) { get_tile_info(tile_index,bgvideoram, 1); }

VIDEO_START( nmg5 )
{
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	fg_tilemap = tilemap_create(fg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,code,color,flipx,flipy,height,y;

		sx = spriteram16[offs + 2];
		sy = spriteram16[offs + 0];
		code = spriteram16[offs + 1];
		color = (spriteram16[offs + 2] >> 9)&0xf;
		height = 1 << ((spriteram16[offs + 0] & 0x0600) >> 9);
		flipx = spriteram16[offs + 0] & 0x2000;
		flipy = spriteram16[offs + 0] & 0x4000;

		for (y = 0;y < height;y++)
		{
			drawgfx(bitmap,Machine->gfx[1],
					code + (flipy ? height-1 - y : y),
					color,
					flipx,flipy,
					sx & 0x1ff,248 - ((sy + 0x10 * (height - y)) & 0x1ff),
					cliprect,TRANSPARENCY_PEN,0);

			/* wrap around */
			drawgfx(bitmap,Machine->gfx[1],
					code + (flipy ? height-1 - y : y),
					color,
					flipx,flipy,
					(sx & 0x1ff) - 512,248 - ((sy + 0x10 * (height - y)) & 0x1ff),
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
}

static void draw_bitmap(mame_bitmap *bitmap)
{
	int yyy=256;
	int xxx=512/4;
	UINT16 x,y,count;
	int xoff=-12;
	int yoff=-9;
	int pix;

	count=0;
	for (y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x++)
		{
			pix = (nmg5_bitmap[count]&0xf000)>>12;
			if (pix) plot_pixel(bitmap, x*4+0+xoff,y+yoff, pix + 0x300);
			pix = (nmg5_bitmap[count]&0x0f00)>>8;
			if (pix) plot_pixel(bitmap, x*4+1+xoff,y+yoff, pix + 0x300);
			pix = (nmg5_bitmap[count]&0x00f0)>>4;
			if (pix) plot_pixel(bitmap, x*4+2+xoff,y+yoff, pix + 0x300);
			pix = (nmg5_bitmap[count]&0x000f)>>0;
			if (pix) plot_pixel(bitmap, x*4+3+xoff,y+yoff, pix + 0x300);

			count++;
		}
	}
}


VIDEO_UPDATE( nmg5 )
{
	tilemap_set_scrolly(bg_tilemap,0,scroll_ram[3]+9);
	tilemap_set_scrollx(bg_tilemap,0,scroll_ram[2]+3);
	tilemap_set_scrolly(fg_tilemap,0,scroll_ram[1]+9);
	tilemap_set_scrollx(fg_tilemap,0,scroll_ram[0]-1);

	fillbitmap(bitmap,Machine->pens[0x100],cliprect);

	if(*sprite_pri == 0)
		draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	if(*sprite_pri == 1)
		draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);

	if(*sprite_pri == 3)
		draw_sprites(bitmap,cliprect);

	draw_bitmap(bitmap);

	if(*sprite_pri == 7)
		draw_sprites(bitmap,cliprect);

}


static gfx_layout nmg5_layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(7,8),RGN_FRAC(6,8),RGN_FRAC(5,8),RGN_FRAC(4,8),RGN_FRAC(3,8),RGN_FRAC(2,8),RGN_FRAC(1,8),RGN_FRAC(0,8) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static gfx_layout pclubys_layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(1,4)+8,RGN_FRAC(2,4)+0,RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0,RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,RGN_FRAC(2,4)+8,RGN_FRAC(1,4)+0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16 },
	8*16
};

static gfx_layout layout_16x16x5 =
{
	16,16,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(2,5),RGN_FRAC(3,5),RGN_FRAC(1,5),RGN_FRAC(4,5),RGN_FRAC(0,5) },
	{ 7,6,5,4,3,2,1,0,135,134,133,132,131,130,129,128 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static gfx_decode nmg5_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &nmg5_layout_8x8x8, 0x000,  2 },
	{ REGION_GFX2, 0, &layout_16x16x5,	  0x200, 16 },
	{ -1 }
};

static gfx_decode pclubys_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pclubys_layout_8x8x8, 0x000,  2 },
	{ REGION_GFX2, 0, &layout_16x16x5,		 0x200, 16 },
	{ -1 }
};


static void soundirq(int state)
{
	cpunum_set_input_line(1, 0, state);
}

static struct YM3812interface ym3812_intf =
{
	soundirq	/* IRQ Line */
};

static MACHINE_DRIVER_START( nmg5 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(nmg5_map,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)		/* 4 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(nmg5_sound_map,0)
	MDRV_CPU_IO_MAP(sound_io_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(nmg5_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(nmg5)
	MDRV_VIDEO_UPDATE(nmg5)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 4000000)
	MDRV_SOUND_CONFIG(ym3812_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pclubys )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nmg5)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pclubys_map,0)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(pclubys_sound_map,0)

	MDRV_GFXDECODE(pclubys_gfxdecodeinfo)
MACHINE_DRIVER_END


ROM_START( nmg5 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ub15.bin", 0x000000, 0x80000, CRC(36af3e2f) SHA1(735aaa901290b1d921242869e81e59649905eb30) )
	ROM_LOAD16_BYTE( "ub16.bin", 0x000001, 0x80000, CRC(2d9923d4) SHA1(e27549da311244db14ae1d3ad5e814a731a0f440) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "xh15.bin", 0x00000, 0x10000, CRC(12d047c4) SHA1(3123b1856219380ff598a2fad97a66863e30d80f) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 */
	ROM_LOAD( "srom1.bin", 0x000000, 0x80000, CRC(6771b694) SHA1(115e5eb45bb05f7a8021b3af3b8e709bbdcae55e) )
	ROM_LOAD( "srom2.bin", 0x080000, 0x80000, CRC(362d33af) SHA1(abf66ab9eabdd40fcd47bc291d60e7e4903cde74) )
	ROM_LOAD( "srom3.bin", 0x100000, 0x80000, CRC(8bad69d1) SHA1(c68d6b318e86b6deb64cc0cd5b51a2ea3ce04fb8) )
	ROM_LOAD( "srom4.bin", 0x180000, 0x80000, CRC(e73a7fcb) SHA1(b6213c0da61ba1c6dbe975365bcde17c71ea3388) )
	ROM_LOAD( "srom5.bin", 0x200000, 0x80000, CRC(7300494e) SHA1(031a74d7a82d23cdd5976b88379b9119322da1a0) )
	ROM_LOAD( "srom6.bin", 0x280000, 0x80000, CRC(74b5fdf9) SHA1(1c0e82a0e3cc1006b902e509076bbea04618320b) )
	ROM_LOAD( "srom7.bin", 0x300000, 0x80000, CRC(bd2b9036) SHA1(28c2d86c9645e6738811f3ece7c2fa02cd6ae4a1) )
	ROM_LOAD( "srom8.bin", 0x380000, 0x80000, CRC(dd38360e) SHA1(be7cb62369513b972c4370adf78df6fcf8caea0a) )

	ROM_REGION( 0x140000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x5 */
	ROM_LOAD( "uf1.bin", 0x000000, 0x40000, CRC(9a9fb6f4) SHA1(4541d33493b9bba11b8e5ed35431271790763db4) )
	ROM_LOAD( "uf2.bin", 0x040000, 0x40000, CRC(66954d63) SHA1(62a315640beb8b063886ea6ed1433a18f75e8d0d) )
	ROM_LOAD( "ufa1.bin",0x080000, 0x40000, CRC(ba73ed2d) SHA1(efd2548fb0ada11ff98b73335689d2394cbf42a4) )
	ROM_LOAD( "uh1.bin", 0x0c0000, 0x40000, CRC(f7726e8e) SHA1(f28669725609ffab7c6c3bfddbe293c55ddd0155) )
	ROM_LOAD( "uj1.bin", 0x100000, 0x40000, CRC(54f7486e) SHA1(88a237a1005b1fd70b6d8544ef60a0d16cb38e6f) )

	ROM_REGION( 0x20000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "xra1.bin", 0x00000, 0x20000, CRC(c74a4f3e) SHA1(2f6165c1d5bdd3e816b95ffd9303dd4bd07f7ac8) )
ROM_END

ROM_START( searchey )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u7.bin", 0x000000, 0x40000, CRC(287ce3dd) SHA1(32305f7b09c58b7f126d41b5b1991e349884cc02) )
	ROM_LOAD16_BYTE( "u2.bin", 0x000001, 0x40000, CRC(b574f033) SHA1(8603926cef9df2495e97a071f08bbf418b9e01a8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "u128.bin", 0x00000, 0x10000, CRC(85bae10c) SHA1(a1e58d8b8c8718cc346aae400bb4eadf6873b86d) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 */
	ROM_LOAD( "u63.bin", 0x000000, 0x80000, CRC(1b0b7b7d) SHA1(092855407fef95da69fcd6e608b8b3aa720d8bcd) )
	ROM_LOAD( "u68.bin", 0x080000, 0x80000, CRC(ae18b2aa) SHA1(f1b2d3c1bafe99ec8b7e8e587ae0a0f9fa410a5a) )
	ROM_LOAD( "u73.bin", 0x100000, 0x80000, CRC(ab7f8716) SHA1(c8cc3c1e9c37add31af28c43130b66f7fdd28042) )
	ROM_LOAD( "u79.bin", 0x180000, 0x80000, CRC(7f2c8b83) SHA1(4f25bf5652ad3327efe63d960b987e581d20afbb) )
	ROM_LOAD( "u64.bin", 0x200000, 0x80000, CRC(322a903c) SHA1(e3b00576edf58c7743854de95102d3d36ce3b775) )
	ROM_LOAD( "u69.bin", 0x280000, 0x80000, CRC(d546eaf8) SHA1(de6c80b733c31ef2c0c64d25d46f1cff9a262c42) )
	ROM_LOAD( "u74.bin", 0x300000, 0x80000, CRC(e6134d84) SHA1(d639f44ef404e206b25a0b4f71ded3854836c60f) )
	ROM_LOAD( "u80.bin", 0x380000, 0x80000, CRC(9a160918) SHA1(aac63dcb6005eaad7088d4e4e584825a6e232764) )

	ROM_REGION( 0x0a0000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x5 */
	ROM_LOAD( "u83.bin", 0x000000, 0x20000, CRC(c5a1c647) SHA1(c8d1cc631b0286a4caa35dce6552c4206e58b620) )
	ROM_LOAD( "u82.bin", 0x020000, 0x20000, CRC(25b2ae62) SHA1(02a1bd8719ca1792c2e4ff52fd5d4845e19fedb7) )
	ROM_LOAD( "u105.bin",0x040000, 0x20000, CRC(b4207ef0) SHA1(e70a73b98e5399221208d81a324fab6b942470c8) )
	ROM_LOAD( "u96.bin", 0x060000, 0x20000, CRC(8c40818a) SHA1(fe2c0da42154261ae1734ddb6cb9ddf33dd58510) )
	ROM_LOAD( "u97.bin", 0x080000, 0x20000, CRC(5dc7f231) SHA1(5e57e436a24dfa14228bac7b8ae5f000393926b9) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u137.bin", 0x00000, 0x40000,  CRC(49105e23) SHA1(99543fbbccf5df5b15a0470eac641b4158024c6a) )
ROM_END

/*

Puzzle Club
Yunsung, 2000

PCB Layout
----------

YS-2113
|----------------------------------------------|
|ROM1.128 ROM2.137 ROM3.7  62256  ROM5.97      |
|  6116 Z80  6295  ROM4.2  62256  ROM6.95      |
|  YMXXXX   16MHz     PAL         ROM7.105     |
|  YM3014             PAL  PAL    ROM8.83      |
|          62256    68000         ROM9.82      |
|          62256           6116     14.38383MHz|
|                          6116   PAL     PAL  |
|J           6116   QL2003        PAL          |
|A           6116                 PAL          |
|M           6116   QL12X16B      PAL          |
|M DIP1      6116                 PAL          |
|A DIP2                                        |
|            62256  QL2003                     |
|            62256                             |
|                                              |
|  ROM11.166  ROM10.167                        |
|                   QL2003        PAL          |
|  ROM13.164  ROM12.165           PAL          |
|----------------------------------------------|
Notes:
      68000 clock : 16.000MHz
      Z80 clock   : 4.000MHz (16/4)
      M6295 clock : 1.000MHz (16/16). Sample Rate = 1000000 / 132
      YMXXXX clock: 4.000MHz (16/4). Chip is either YM2151 or YM3812
      VSync       : 60Hz

*/

ROM_START( pclubys )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom3.7", 0x000000, 0x80000, CRC(62e28e6d) SHA1(30307dfbb6bd02d78fb06d3c3522b41115f1c27a) )
	ROM_LOAD16_BYTE( "rom4.2", 0x000001, 0x80000, CRC(b51dab41) SHA1(2ad3929c8cf2b66c36289c2c851769190916b718) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom1.128", 0x00000, 0x10000, CRC(25cd27f8) SHA1(97af1368381234361bbd97f4552209c435652372) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 */
	ROM_LOAD( "rom10.167", 0x000000, 0x400000, CRC(d67e8e84) SHA1(197d1bdb321cf7ac986b5dbfa061494ffd4db6e4) )
	ROM_LOAD( "rom12.165", 0x400000, 0x400000, CRC(6be8b733) SHA1(bdbbec77938828ac9831537c00abd5c31dc56284) )
	ROM_LOAD( "rom11.166", 0x800000, 0x400000, CRC(672501a4) SHA1(193e1965c2f21f469e5c6c514d3cdcab3ffdf629) )
	ROM_LOAD( "rom13.164", 0xc00000, 0x400000, CRC(fc725ce7) SHA1(d997a31a975ae67efa071720c58235c738ebbbe3) )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x5 */
	ROM_LOAD( "rom8.83", 0x000000, 0x80000, CRC(651af101) SHA1(350698bd7ee65fc1ed084382db7f66ffb83c23c6) )
	ROM_LOAD( "rom9.82", 0x080000, 0x80000, CRC(2535b4d6) SHA1(85af6a042e83f8a7abb78c5edfd4497a9018ed68) )
	ROM_LOAD( "rom7.105",0x100000, 0x80000, CRC(f7536c52) SHA1(546976f52c6f064f5172736988ada053c1c1183f) )
	ROM_LOAD( "rom6.95", 0x180000, 0x80000, CRC(3c078a52) SHA1(be8936bcafbfd77e491c81a3d215a53dad78d652) )
	ROM_LOAD( "rom5.97", 0x200000, 0x80000, CRC(20eae2f8) SHA1(ad9ac6e5e0331fb19652f6578dbb2f532bb22b3d) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom2.137", 0x00000, 0x80000, CRC(4ff97ad1) SHA1(d472c8298e428cb9659ce90a8ce9402c119bdb0f) )
ROM_END

ROM_START( pclubysa )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom3a.7", 0x000000, 0x80000, CRC(885aa07a) SHA1(a0af5b0704f7fb18ed21f42979a40a8b419377b1) )
	ROM_LOAD16_BYTE( "rom4a.2", 0x000001, 0x80000, CRC(9bfbdeac) SHA1(263341b05883d4a9125da69d9d8d6f4d654f3475) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom1.128", 0x00000, 0x10000, CRC(25cd27f8) SHA1(97af1368381234361bbd97f4552209c435652372) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 */
	ROM_LOAD( "rom10.167", 0x000000, 0x400000, CRC(d67e8e84) SHA1(197d1bdb321cf7ac986b5dbfa061494ffd4db6e4) )
	ROM_LOAD( "rom12.165", 0x400000, 0x400000, CRC(6be8b733) SHA1(bdbbec77938828ac9831537c00abd5c31dc56284) )
	ROM_LOAD( "rom11.166", 0x800000, 0x400000, CRC(672501a4) SHA1(193e1965c2f21f469e5c6c514d3cdcab3ffdf629) )
	ROM_LOAD( "rom13.164", 0xc00000, 0x400000, CRC(fc725ce7) SHA1(d997a31a975ae67efa071720c58235c738ebbbe3) )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x5 */
	ROM_LOAD( "rom8.83", 0x000000, 0x80000, CRC(651af101) SHA1(350698bd7ee65fc1ed084382db7f66ffb83c23c6) )
	ROM_LOAD( "rom9.82", 0x080000, 0x80000, CRC(2535b4d6) SHA1(85af6a042e83f8a7abb78c5edfd4497a9018ed68) )
	ROM_LOAD( "rom7.105",0x100000, 0x80000, CRC(f7536c52) SHA1(546976f52c6f064f5172736988ada053c1c1183f) )
	ROM_LOAD( "rom6.95", 0x180000, 0x80000, CRC(3c078a52) SHA1(be8936bcafbfd77e491c81a3d215a53dad78d652) )
	ROM_LOAD( "rom5.97", 0x200000, 0x80000, CRC(20eae2f8) SHA1(ad9ac6e5e0331fb19652f6578dbb2f532bb22b3d) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom2.137", 0x00000, 0x80000, CRC(4ff97ad1) SHA1(d472c8298e428cb9659ce90a8ce9402c119bdb0f) )
ROM_END

DRIVER_INIT( nmg5 )
{
	/* hardcoded sprites priority */
	*sprite_pri = 7;
}


GAME( 1998, nmg5,     0,       nmg5,    nmg5,      nmg5, ROT0, "Yun Sung", "Multi 5 / New Multi Game 5" )
GAME( 1999, searchey, 0,       nmg5,    searchey,  0,    ROT0, "Yun Sung", "Search Eye" )
GAME( 2000, pclubys,  0,       pclubys, pclubys,   0,    ROT0, "Yun Sung", "Puzzle Club (Yun Sung - set 1)" )
GAME( 2000, pclubysa, pclubys, pclubys, pclubys,   0,    ROT0, "Yun Sung", "Puzzle Club (Yun Sung - set 2)" )
