/*

macs.c - Multi Amenity Cassette System

processor seems to be ST0016 (z80 based) from SETA

around 0x3700 of the bios (when interleaved) contains the ram test text

TODO:
(general)
-Hook-Up bios.
-Is the NMI triggering really needed?The NMI vectors points to a retn in all the games...
(yuka)
-Crashes because of the first time that executes ldir at 8c0,it call sprite data with
ASCII character texts.
(yujan)
-Girls disappears when you win.
-Some trasparency issues.
-Some gfx are offset.


----- Game Notes -----

Kisekae Mahjong  (c)1995 I'MAX
Kisekae Hanafuda (c)1995 I'MAX
Seimei-Kantei-Meimei-Ki Cult Name (c)1996 I'MAX

KISEKAE -- info

* DIP SWITCH *

                      | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
-------------------------------------------------------
 P2 Level |  Normal   |off|off|                       |
          |   Weak    |on |off|                       |
          |  Strong   |off|on |                       |
          |Very strong|on |on |                       |
-------------------------------------------------------
 P2 Points|  Normal   |       |off|off|               |
          |  Easy     |       |on |off|               |
          |  Hard     |       |off|on |               |
          | Very hard |       |on |on |               |
-------------------------------------------------------
 P1       |  1000pts  |               |off|           |
 points   |  2000pts  |               |on |           |
-------------------------------------------------------
  Auto    |   Yes     |                   |off|       |
  tumo    |   No      |                   |on |       |
-------------------------------------------------------
  Not     |           |                       |off|   |
  Used    |           |                       |on |   |
-------------------------------------------------------
  Tumo    |   Long    |                           |off|
  time    |   Short   |                           |on |
-------------------------------------------------------

* at slotA -> DIP SW3
     slotB -> DIP SW4


*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/st0016.h"

WRITE8_HANDLER (st0016_sprite_bank_w);
WRITE8_HANDLER (st0016_palette_bank_w);
WRITE8_HANDLER (st0016_character_bank_w);
READ8_HANDLER  (st0016_sprite_ram_r);
WRITE8_HANDLER (st0016_sprite_ram_w);
READ8_HANDLER  (st0016_sprite2_ram_r);
WRITE8_HANDLER (st0016_sprite2_ram_w);
READ8_HANDLER  (st0016_palette_ram_r);
WRITE8_HANDLER (st0016_palette_ram_w);
READ8_HANDLER  (st0016_character_ram_r);
WRITE8_HANDLER (st0016_character_ram_w);
READ8_HANDLER(st0016_vregs_r);
WRITE8_HANDLER(st0016_vregs_w);

VIDEO_START(st0016);
VIDEO_UPDATE(st0016);
extern int st0016_game;
void st0016_save_init(void);
//static int mux_port;
extern int st0016_rom_bank;
//static int rambank=0;

static int extraram[0x2000];

static ADDRESS_MAP_START( st0016_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_READ(st0016_sprite_ram_r) AM_WRITE(st0016_sprite_ram_w)
	AM_RANGE(0xd000, 0xdfff) AM_READ(st0016_sprite2_ram_r) AM_WRITE(st0016_sprite2_ram_w)
	AM_RANGE(0xe000, 0xe7ff) AM_RAM /* work ram ? */
	AM_RANGE(0xe800, 0xe87f) AM_RAM
	AM_RANGE(0xe900, 0xe9ff) AM_RAM AM_WRITE(st0016_snd_w) AM_BASE(&st0016_sound_regs) /* sound regs 8 x $20 bytes, see notes */
	AM_RANGE(0xea00, 0xebff) AM_READ(st0016_palette_ram_r) AM_WRITE(st0016_palette_ram_w)
	AM_RANGE(0xec00, 0xec1f) AM_READ(st0016_character_ram_r) AM_WRITE(st0016_character_ram_w)
	//AM_RANGE(0xf800, 0xffff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK(2)//AD(rbank_r) AM_WRITE(rbank_w) /* common /backup ram ?*/
ADDRESS_MAP_END

/*
static READ8_HANDLER(mux_r)
{
//
//  76543210
//      xxxx - input port #2
//  xxxx     - dip switches (2x8 bits) (multiplexed)

    int retval=input_port_2_r(0)&0x0f;
    switch(mux_port&0x30)
    {
        case 0x00: retval|=((input_port_4_r(0)&1)<<4)|((input_port_4_r(0)&0x10)<<1)|((input_port_5_r(0)&1)<<6)|((input_port_5_r(0)&0x10)<<3);break;
        case 0x10: retval|=((input_port_4_r(0)&2)<<3)|((input_port_4_r(0)&0x20)   )|((input_port_5_r(0)&2)<<5)|((input_port_5_r(0)&0x20)<<2);break;
        case 0x20: retval|=((input_port_4_r(0)&4)<<2)|((input_port_4_r(0)&0x40)>>1)|((input_port_5_r(0)&4)<<4)|((input_port_5_r(0)&0x40)<<1);break;
        case 0x30: retval|=((input_port_4_r(0)&8)<<1)|((input_port_4_r(0)&0x80)>>2)|((input_port_5_r(0)&8)<<3)|((input_port_5_r(0)&0x80)   );break;
    }
    return retval;
}

static WRITE8_HANDLER(mux_select_w)
{
    mux_port=data;
}
*/
static WRITE8_HANDLER(st0016_rom_bank_w)
{
	memory_set_bankptr( 1, memory_region(REGION_CPU1) + (data* 0x4000) + 0x10000 );
	st0016_rom_bank=data;
}

READ8_HANDLER(st0016_dma_r);

static READ8_HANDLER(macs_dips)
{
	//printf("[%x] %x\n",offset,activecpu_get_previouspc());
	return 0x00;//xff;
}

static WRITE8_HANDLER(rambank_w)
{
	//printf("RAMBANK [%x] %x\n",data,activecpu_get_previouspc());
	memory_set_bankptr( 2, &extraram[(data&1)*0x1000] );
	//rambank=data;
}

static WRITE8_HANDLER(rambank2_w)
{
	//printf("RAMBANK2 [%x] %x\n",data,activecpu_get_previouspc());
	//memory_set_bankptr( 2, &extraram[((data>>5)&1)*0x1000] );
	//rambank=data;
}


//2d4 !!!

static READ8_HANDLER(macs_rand_r)
{
	//printf("[%x] %x\n",offset,activecpu_get_previouspc());
	return rand();
}

static UINT8 mux_data;

static READ8_HANDLER( macs_input_r )
{
 	//logerror("I/O read at PC = %06x offset = %02x\n",activecpu_get_pc(),offset+0xc0);

	switch(offset)
	{
		case 0:
		{
			/*It's bit-wise*/
			switch(mux_data)
			{
				case 0x00: return readinputportbytag("IN0");
				case 0x01: return readinputportbytag("IN1");
				case 0x02: return readinputportbytag("IN2");
				case 0x04: return readinputportbytag("IN3");
				case 0x08: return readinputportbytag("IN4");
				default:
				logerror("Unmapped mahjong panel mux data %02x\n",mux_data);
				return 0xff;
			}
		}
		case 1: return readinputportbytag("SYS0");
		case 2: return readinputportbytag("DSW0");
		case 3: return readinputportbytag("DSW1");
		case 4: return readinputportbytag("DSW2");
		case 5: return readinputportbytag("DSW3");
		case 6: return readinputportbytag("DSW4");
		case 7: return readinputportbytag("SYS1");
		default: 	usrintf_showmessage("Unmapped I/O read at PC = %06x offset = %02x",activecpu_get_pc(),offset+0xc0);
	}

	return 0xff;
}

static WRITE8_HANDLER( macs_output_w )
{
	//logerror("I/O write at PC = %06x offset = %02x data = %02x\n",activecpu_get_pc(),offset+0xc0,data);

	switch(offset)
	{
		case 2: mux_data = data; break;
		//logerror("Unmapped I/O write at PC = %06x offset = %02x data = %02x\n",activecpu_get_pc(),offset+0xc0,data);
		//default: usrintf_showmessage("Unmapped I/O write at PC = %06x offset = %02x data = %02x",activecpu_get_pc(),offset+0xc0,data);
	}
}

static ADDRESS_MAP_START( st0016_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0xbf) AM_READ(st0016_vregs_r) AM_WRITE(st0016_vregs_w) /* video/crt regs ? */
	AM_RANGE(0xc0, 0xc7) AM_READWRITE(macs_input_r,macs_output_w)
	AM_RANGE(0xe0, 0xe0) AM_WRITENOP /* renju = $40, neratte = 0 */
	AM_RANGE(0xe1, 0xe1) AM_WRITE(st0016_rom_bank_w)
	AM_RANGE(0xe2, 0xe2) AM_WRITE(st0016_sprite_bank_w)
	AM_RANGE(0xe3, 0xe4) AM_WRITE(st0016_character_bank_w)
	AM_RANGE(0xe5, 0xe5) AM_WRITE(st0016_palette_bank_w)
	AM_RANGE(0xe6, 0xe6) AM_WRITE(rambank_w) /* banking ? ram bank ? shared rambank ? */
	AM_RANGE(0xe7, 0xe7) AM_WRITENOP /* watchdog */
	AM_RANGE(0xf0, 0xf0) AM_READ(st0016_dma_r)
ADDRESS_MAP_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
//  { 0, 0, &charlayout,      0, 16*4  },
	{ -1 }	/* end of array */
};

static INTERRUPT_GEN(st0016_int)
{
	if(!cpu_getiloops())
		cpunum_set_input_line(0,0,HOLD_LINE);
	else
		if(activecpu_get_reg(Z80_IFF1)) /* dirty hack ... */
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE );
}

INPUT_PORTS_START( macs )
	/*0*/
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, "DSW0 - BIT 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "BIT 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "BIT 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "BIT 8" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "BIT 10" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "BIT 20" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "BIT 40" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "BIT 80" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/*1*/
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1 - BIT 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "BIT 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "BIT 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "BIT 8" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "BIT 10" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "BIT 20" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "BIT 40" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "BIT 80" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/*2*/
	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "DSW 2 - BIT 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "BIT 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "BIT 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "BIT 8" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "BIT 10" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "BIT 20" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "BIT 40" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "BIT 80" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/*3*/
	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "DSW3 - BIT 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "BIT 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Game" )
	PORT_DIPSETTING(    0x08, "Bet Type" )
	PORT_DIPSETTING(    0x00, "Normal Type" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Level_Select ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Memory Reset" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Analyzer" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Test Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/*4 - unused*/
	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "DSW4 - BIT 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "BIT 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "BIT 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "BIT 8" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "BIT 10" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "BIT 20" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "BIT 40" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "BIT 80" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/*(COMMON) MAHJONG PANEL*/
	PORT_START_TAG("IN0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	/*
    Note: These could likely to be switches that are on the game board and not Dip Switches
    */
	PORT_START_TAG("SYS0")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Note In") PORT_CODE(KEYCODE_4_PAD)

	PORT_START_TAG("SYS1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Clear Coin Counter") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Memory Reset") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Analyzer") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


extern data8_t *st0016_charram;
static struct ST0016interface st0016_interface =
{
	&st0016_charram
};

static MACHINE_DRIVER_START( macs )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,8000000) /* 8 MHz ? */
	MDRV_CPU_PROGRAM_MAP(st0016_mem,0)
	MDRV_CPU_IO_MAP(st0016_io,0)

	MDRV_CPU_VBLANK_INT(st0016_int,5) /*  4*nmi + int0 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 128*8)
	MDRV_VISIBLE_AREA(0*8, 128*8-1, 0*8, 128*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*16*4+1)

	MDRV_VIDEO_START(st0016)
	MDRV_VIDEO_UPDATE(st0016)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(ST0016, 0)
	MDRV_SOUND_CONFIG(st0016_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



#define MACS_BIOS \
	ROM_REGION( 0x1000000, REGION_USER1, 0 ) \
	ROM_LOAD16_BYTE( "macsos_l.u43", 0x00000, 0x80000, CRC(0b5aed5e) SHA1(042e705017ee34656e2c6af45825bb2dd3447747) ) \
	ROM_LOAD16_BYTE( "macsos_h.u44", 0x00001, 0x80000, CRC(538b68e4) SHA1(a0534147791e94e726f49451d0e95671ae0a87d5) ) \

ROM_START( macsbios )
	MACS_BIOS
	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

ROM_START( kisekaem )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "am-mj.u8", 0x000000, 0x100000, CRC(3cf85151) SHA1(e05400065c384730f04ef565db5ba27eb3973d15) )
	ROM_LOAD16_BYTE( "am-mj.u7", 0x000001, 0x100000, CRC(4b645354) SHA1(1dbf9141c3724e5dff2cd8066117fb1b94671a80) )
	ROM_LOAD16_BYTE( "am-mj.u6", 0x200000, 0x100000, CRC(23b3aa24) SHA1(bfabdb16f9b1b60230bb636a944ab46fdfda49d7) )
	ROM_LOAD16_BYTE( "am-mj.u5", 0x200001, 0x100000, CRC(b4d53e29) SHA1(d7683fdd5531bf1aa0ef1e4e6f517b31e2d5829e) )


	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B

	ROM_REGION( 0x1000000, REGION_CPU1, 0 )
	ROM_COPY( REGION_USER2,   0x00000, 0x000000, 0x08000 )
	ROM_COPY( REGION_USER2,   0x00000, 0x010000, 0x400000 )
ROM_END

ROM_START( kisekaeh )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "kh-u8.bin", 0x000000, 0x100000, CRC(601b9e6a) SHA1(54508a6db3928f78897df64ce400791e4789d0f6) )
	ROM_LOAD16_BYTE( "kh-u7.bin", 0x000001, 0x100000, CRC(8f6e4bb3) SHA1(361545189feeda0887f930727d25655309b84629) )
	ROM_LOAD16_BYTE( "kh-u6.bin", 0x200000, 0x100000, CRC(8e700204) SHA1(876e5530d749828de077293cb109a71b67cef140) )
	ROM_LOAD16_BYTE( "kh-u5.bin", 0x200001, 0x100000, CRC(709bf7c8) SHA1(0a93e0c4f9be22a3302a1c5d2a6ec4739b202ea8) )


	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B

	ROM_REGION( 0x1000000, REGION_CPU1, 0 )
	ROM_COPY( REGION_USER2,   0x00000, 0x000000, 0x08000 )
	ROM_COPY( REGION_USER2,   0x00000, 0x010000, 0x400000 )
ROM_END

ROM_START( cultname ) // uses printer
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "cult-d0.u8", 0x000000, 0x100000, CRC(394bc1a6) SHA1(98df5406862234815b46c7b0ac0b19e4b597d1b6) )
	ROM_LOAD16_BYTE( "cult-d1.u7", 0x000001, 0x100000, CRC(f628133b) SHA1(f06e20212074e5d95cc7d419ac8ce98fb9be3b62) )
	ROM_LOAD16_BYTE( "cult-d2.u6", 0x200000, 0x100000, CRC(c5521bc6) SHA1(7554b56b0201b7d81754defa2244fb7ff7452bf6) )
	ROM_LOAD16_BYTE( "cult-d3.u5", 0x200001, 0x100000, CRC(4325b09b) SHA1(45699a0444a221f893724754c917d33041cabcb9) )


	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
	ROM_LOAD16_BYTE( "cult-g0.u8", 0x000000, 0x100000, CRC(f5ab977b) SHA1(e7ee758cc2864500b339e236b944f98df9a1c10e) )
	ROM_LOAD16_BYTE( "cult-g1.u7", 0x000001, 0x100000, CRC(32ae15a4) SHA1(061992efec1ed5527f200bf4c111344b156e759d) )
	ROM_LOAD16_BYTE( "cult-g2.u6", 0x200000, 0x100000, CRC(30ed056d) SHA1(71735339bb501b94402ef403b5a2a60effa39c36) )
	ROM_LOAD16_BYTE( "cult-g3.u5", 0x200001, 0x100000, CRC(fe58b418) SHA1(512f5c544cfafaa98bd2b3791ff1cf67adecec8d) )


	ROM_REGION( 0x1000000, REGION_CPU1, 0 )
	ROM_COPY( REGION_USER3,   0x00000, 0x000000, 0x08000 )
	ROM_COPY( REGION_USER3,   0x00000, 0x010000, 0x400000 )
ROM_END

/* these are listed as MACS2 sub-boards, is it the same? */

ROM_START( yuka )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "yu-ka_2.u6", 0x000001, 0x100000, CRC(c3c5728b) SHA1(e53cdcae556f34bab45d9342fd78ec29b6543c46) )
	ROM_LOAD16_BYTE( "yu-ka_4.u5", 0x000000, 0x100000, CRC(7e391ee6) SHA1(3a0c122c9d0e2a91df6d8039fb958b6d00997747) )
	ROM_LOAD16_BYTE( "yu-ka_1.u8", 0x200001, 0x100000, CRC(bccd1b15) SHA1(02511f3be60c53b5f5d90f12f0648f6e184ca667) )
	ROM_LOAD16_BYTE( "yu-ka_3.u7", 0x200000, 0x100000, CRC(45b8263e) SHA1(59e1846c91dc39a086e8306260506673eb91de0b) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B

	ROM_REGION( 0x1000000, REGION_CPU1, 0 )
	ROM_COPY( REGION_USER2,   0x00000, 0x000000, 0x08000 )
	ROM_COPY( REGION_USER2,   0x00000, 0x010000, 0x400000 )
ROM_END

ROM_START( yujan )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "yu-jan_2.u6", 0x000001, 0x100000, CRC(2f4a8d4b) SHA1(4b328a253b1980a76f46a9a98a7f486813894a33) )
	ROM_LOAD16_BYTE( "yu-jan_4.u5", 0x000000, 0x100000, CRC(226df87b) SHA1(a887728f1ea2ef5f6b4dcd6b5b61586f5e8f267d) )
	ROM_LOAD16_BYTE( "yu-jan_1.u8", 0x200001, 0x100000, CRC(feeeee6a) SHA1(e9613f50d6d2e62fac6b529f81486250cfe83819) )
	ROM_LOAD16_BYTE( "yu-jan_3.u7", 0x200000, 0x100000, CRC(1c1d6997) SHA1(9b07ae6b9ef1c0b57fbaa5fd0bcf1d2d7f17351f) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B

	ROM_REGION( 0x1000000, REGION_CPU1, 0 )
	ROM_COPY( REGION_USER2,   0x00000, 0x000000, 0x08000 )
	ROM_COPY( REGION_USER2,   0x00000, 0x010000, 0x400000 )
ROM_END

static DRIVER_INIT(macs)
{
	st0016_game=10|0x80;
}




GAMEX( 1995, macsbios, 0,        macs, macs, macs, ROT0, "I'Max", "Multi Amenity Cassette System BIOS", NOT_A_DRIVER | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, kisekaem, macsbios, macs, macs, macs, ROT0, "I'Max", "Kisekae Mahjong",  GAME_NOT_WORKING|GAME_IMPERFECT_SOUND )
GAMEX( 1995, kisekaeh, macsbios, macs, macs, macs, ROT0, "I'Max", "Kisekae Hanafuda",  GAME_NOT_WORKING |GAME_IMPERFECT_SOUND)
GAMEX( 1996, cultname, macsbios, macs, macs, macs, ROT0, "I'Max", "Seimei-Kantei-Meimei-Ki Cult Name",  GAME_NOT_WORKING |GAME_IMPERFECT_SOUND)
GAMEX( 1999, yuka,     macsbios, macs, macs, macs, ROT0, "Yubis / T.System", "Yu-Ka",  GAME_NOT_WORKING|GAME_IMPERFECT_SOUND )
GAMEX( 1999, yujan,    macsbios, macs, macs, macs, ROT0, "Yubis / T.System", "Yu-Jan",  GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) // shows *something* with hack
