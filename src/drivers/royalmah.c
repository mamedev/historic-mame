/****************************************************************************

Royal Mahjong (V1.01  03/15/1982)
driver by Zsolt Vasvari

Location     Device      File ID
--------------------------------
O1            2732         ROM1
O1/2          2732         ROM2
O2/3          2732         ROM3
O4/5          2732         ROM4
O4            2732         ROM5
O4/5          2732         ROM6
K6       TBP18S030    F-ROM.BPR

Notes:    Falcon PCB No. FRM-03


Stephh's notes (based on the games Z80 code and some tests) :

1) 'royalmah'

  - COIN1 has no effect (imperfect royalmah_player_?_port_r read handler ?)
  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode").
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

2) 'tontonb'

  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode")
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

  - I've DELIBERATELY mapped DSW3 before DSW2 to try to spot the common
    things with the other Dynax mahjong games ! Please don't change this !

  - When "Special Combinaisons" Dip Switch is ON, there is a marker in
    front of a random combinaison. It's value is *2 then.

3) 'mjdiplob'

  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode")
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

  - When "Special Combinaisons" Dip Switch is ON, there is a marker in
    front of a random combinaison. It's value remains *1 though.
    Could it be a leftover from another game ('tontonb' for exemple) ?

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


PALETTE_INIT( royalmah )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}


WRITE_HANDLER( royalmah_videoram_w )
{
	int i;
	UINT8 x, y;
	UINT8 col1, col2;


	videoram[offset] = data;

	col1 = videoram[offset & 0x3fff];
	col2 = videoram[offset | 0x4000];

	y = (offset >> 6);
	x = (offset & 0x3f) << 2;

	for (i = 0; i < 4; i++)
	{
		int col = ((col1 & 0x01) >> 0) | ((col1 & 0x10) >> 3) | ((col2 & 0x01) << 2) | ((col2 & 0x10) >> 1);

		plot_pixel(tmpbitmap, x+i, y, Machine->pens[col]);

		col1 >>= 1;
		col2 >>= 1;
	}
}


VIDEO_UPDATE( royalmah )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
		{
			royalmah_videoram_w(offs, videoram[offs]);
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}




static WRITE_HANDLER( royalmah_rom_w )
{
	/* using this handler will avoid all the entries in the error log that are the result of
	   the RLD and RRD instructions this games uses to print text on the screen */
}


static int royalmah_input_port_select;
static int majs101b_dsw_select;

static WRITE_HANDLER( royalmah_input_port_select_w )
{
	royalmah_input_port_select = data;
}

static READ_HANDLER( royalmah_player_1_port_r )
{
	int ret = (input_port_0_r(offset) & 0xc0) | 0x3f;

	if ((royalmah_input_port_select & 0x01) == 0)  ret &= input_port_0_r(offset);
	if ((royalmah_input_port_select & 0x02) == 0)  ret &= input_port_1_r(offset);
	if ((royalmah_input_port_select & 0x04) == 0)  ret &= input_port_2_r(offset);
	if ((royalmah_input_port_select & 0x08) == 0)  ret &= input_port_3_r(offset);
	if ((royalmah_input_port_select & 0x10) == 0)  ret &= input_port_4_r(offset);

	return ret;
}

static READ_HANDLER( royalmah_player_2_port_r )
{
	int ret = (input_port_5_r(offset) & 0xc0) | 0x3f;

	if ((royalmah_input_port_select & 0x01) == 0)  ret &= input_port_5_r(offset);
	if ((royalmah_input_port_select & 0x02) == 0)  ret &= input_port_6_r(offset);
	if ((royalmah_input_port_select & 0x04) == 0)  ret &= input_port_7_r(offset);
	if ((royalmah_input_port_select & 0x08) == 0)  ret &= input_port_8_r(offset);
	if ((royalmah_input_port_select & 0x10) == 0)  ret &= input_port_9_r(offset);

	return ret;
}

/* Royal Mahjong */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, royalmah_rom_w },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0x8000, 0xffff, royalmah_videoram_w, &videoram, &videoram_size },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
PORT_END

/* Dynax Mahjong Games */

static MEMORY_READ_START( dynax_readmem )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( dynax_writemem )
	{ 0x0000, 0x6fff, royalmah_rom_w },
	{ 0x7000, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, royalmah_videoram_w, &videoram, &videoram_size },
MEMORY_END


/* Dynax Ports */

static READ_HANDLER ( majs101b_dsw_r )
{
	switch (majs101b_dsw_select)
	{
		case 0x00: return readinputport(13);	/* DSW3 */
		case 0x20: return readinputport(14);	/* DSW4 */
		case 0x40: return readinputport(12);	/* DSW2 */
	}
	return 0;
}


static data8_t suzume_bank;

static READ_HANDLER ( suzume_bank_r )
{
	return suzume_bank;
}

static WRITE_HANDLER ( suzume_bank_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int address;

	suzume_bank = data;

logerror("%04x: bank %02x\n",activecpu_get_pc(),data);

	/* bits 6, 4 and 3 used for something input related? */

	address = 0x10000 + (data & 0x07) * 0x8000;
	cpu_setbank(1,&rom[address]);
}


/* bits 5 and 6 seem to affect which Dip Switch to read in 'majs101b' */
static WRITE_HANDLER ( dynax_bank_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int address;

logerror("%04x: bank %02x\n",activecpu_get_pc(),data);

	majs101b_dsw_select = data & 0x60;

	if (data == 0) return;	// tontonb fix?

	data &= 0x1f;			// 0x0f might be enough

	address = 0x10000 + data * 0x8000;

	cpu_setbank(1,&rom[address]);
}




static PORT_READ_START( dondenmj_readport )
//	{ 0x00, 0x00, majs101b_dsw_r },
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
PORT_END

static PORT_WRITE_START( dondenmj_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x87, 0x87, dynax_bank_w },
PORT_END

static PORT_READ_START( suzume_readport )
//	{ 0x00, 0x00, majs101b_dsw_r },
	{ 0x01, 0x01, AY8910_read_port_0_r },
//	{ 0x10, 0x10, input_port_11_r },
//	{ 0x11, 0x11, input_port_10_r },
	{ 0x80, 0x80, suzume_bank_r },
PORT_END

static PORT_WRITE_START( suzume_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x81, 0x81, suzume_bank_w },
PORT_END

static PORT_READ_START( tontonb_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x46, 0x46, input_port_13_r },
	{ 0x47, 0x47, input_port_12_r },
PORT_END

static PORT_WRITE_START( tontonb_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x44, 0x44, dynax_bank_w },
PORT_END

static PORT_READ_START( mjdiplob_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x62, 0x62, input_port_12_r },
	{ 0x63, 0x63, input_port_13_r },
PORT_END

static PORT_WRITE_START( mjdiplob_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x61, 0x61, dynax_bank_w },
PORT_END

static PORT_READ_START( majs101b_readport )
	{ 0x00, 0x00, majs101b_dsw_r },
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
PORT_END

static PORT_WRITE_START( majs101b_writeport )
	{ 0x00, 0x00, dynax_bank_w },
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
PORT_END



INPUT_PORTS_START( royalmah )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 Credit Clear", KEYCODE_7,  IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 Credit Clear", KEYCODE_8,  IP_JOY_NONE )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW  (inport $10) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tontonb )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x73b0) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// affects videoram - flip screen ?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 (inport $47 -> 0x73b1) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			// check code at 0x0e6d
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	// table at 0x4e7d
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	// table at 0x4e4d
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	// table at 0x4e5d
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	// table at 0x4e6d
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		// check code at 0x5184
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// stores something at 0x76ff
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		// check code at 0x1482, 0x18c2, 0x1a1d, 0x1a83, 0x2d2f and 0x2d85
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Maximum Payout ?" )		// check code at 0x1ab7
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x40, "300" )
	PORT_DIPSETTING(    0x60, "500" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		// check code at 0x18c2, 0x1a1d, 0x2d2f and 0x2d85
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $46 -> 0x73b2) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	// see notes
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		// check code at 0x07c5
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		// check code at 0x5375
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// check code at 0x5241
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		// untested ?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		// check code at 0x13aa
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Full Tests" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mjdiplob )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x76fa) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// affects videoram - flip screen ?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )		// check code at 0x0b94 and 0x0de2
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $62 -> 0x76fb) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			// check code at 0x09cd
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	// table at 0x4b82
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	// table at 0x4b52
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	// table at 0x4b62
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	// table at 0x4b72
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Payout ?" )		// check code at 0x166c
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPSETTING(    0x10, "200" )
	PORT_DIPSETTING(    0x20, "300" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// check code at 0x2c64
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		// check code at 0x2c64
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW3 (inport $63 -> 0x76fc) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	// see notes
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		// check code at 0x531f and 0x5375
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// check code at 0x5240
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		// check code at 0x2411
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		// check code at 0x2411 and 0x4beb
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// check code at 0x24ff, 0x25f2, 0x3fcf and 0x45d7
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Full Tests" )			// seems to hang after the last animation
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( majs101b )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x76fd) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )		// check code at 0x1635
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $00 (after out 0,$40) -> 0x76fa) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			// check code at 0x14e4
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	// table at 0x1539
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	// table at 0x1509
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	// table at 0x1519
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	// table at 0x1529
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		// check code at 0x1220, 0x128d, 0x13b1, 0x13cb and 0x2692
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x00, "Maximum Payout ?" )		// check code at 0x12c1
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x10, "300" )
	PORT_DIPSETTING(    0x30, "400" )
	PORT_DIPSETTING(    0x08, "500" )
	PORT_DIPSETTING(    0x28, "600" )
	PORT_DIPSETTING(    0x18, "700" )
	PORT_DIPSETTING(    0x00, "1000" )
//	PORT_DIPSETTING(    0x38, "1000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// check code at 0x1333
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		// check code at 0x076d and 0x43c8
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW3 (inport $00 (after out 0,$00) -> 0x76fc) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	// see notes
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		// check code at 0x1cf9
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		// check code at 0x21a9, 0x21dc and 0x2244
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// check code at 0x2b7f
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		// check code at 0x50ba
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		// check code at 0x1f65
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// check code at 0x6412
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )		// check code at 0x2cb2 and 0x2d02
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW4 (inport $00 (after out 0,$20) -> 0x76fb) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Unknown ) )		// stored at 0x702f - check code at 0x1713,
	PORT_DIPSETTING(    0x00, "0" )				// 0x33d1, 0x3408, 0x3415, 0x347c, 0x3492, 0x350d,
	PORT_DIPSETTING(    0x01, "1" )				// 0x4af9, 0x4b1f and 0x61f6
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty ?" )		// check code at 0x4b5c and 0x6d72
	PORT_DIPSETTING(    0x00, "Easy" )				// 0x05 - 0x03, 0x02, 0x02, 0x01
	PORT_DIPSETTING(    0x04, "Normal" )			// 0x0a - 0x05, 0x02, 0x02, 0x01
	PORT_DIPSETTING(    0x08, "Hard" )				// 0x0f - 0x06, 0x03, 0x02, 0x01
	PORT_DIPSETTING(    0x0c, "Hardest" )			// 0x14 - 0x0a, 0x06, 0x02, 0x01
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Unknown ) )		// check code at 0x228e
	PORT_DIPSETTING(    0x00, "0x00" )
	PORT_DIPSETTING(    0x10, "0x10" )
	PORT_DIPSETTING(    0x20, "0x20" )
	PORT_DIPSETTING(    0x30, "0x30" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		// check code at 0x11e4
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Full Tests" )			// check code at 0x006d
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	18432000/12,	/* 1.5 MHz ? */
	{ 33 },
	{ royalmah_player_1_port_r },
	{ royalmah_player_2_port_r },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( royalmah )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000)        /* 3.00 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(royalmah)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(royalmah)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mjdiplob )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000/2)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(dynax_readmem,dynax_writemem)
	MDRV_CPU_PORTS(mjdiplob_readport,mjdiplob_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(royalmah)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(royalmah)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dondenmj )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(mjdiplob)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(dondenmj_readport,dondenmj_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( suzume )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(mjdiplob)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(suzume_readport,suzume_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tontonb )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(mjdiplob)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(tontonb_readport,tontonb_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( majs101b )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(mjdiplob)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(majs101b_readport,majs101b_writeport)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( royalmah )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1",   	0x0000, 0x1000, 0x69b37a62 )
	ROM_LOAD( "rom2",   	0x1000, 0x1000, 0x0c8351b6 )
	ROM_LOAD( "rom3",   	0x2000, 0x1000, 0xb7736596 )
	ROM_LOAD( "rom4",   	0x3000, 0x1000, 0xe3c7c15c )
	ROM_LOAD( "rom5",   	0x4000, 0x1000, 0x16c09c73 )
	ROM_LOAD( "rom6",   	0x5000, 0x1000, 0x92687327 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "f-rom.bpr",  0x0000, 0x0020, 0xd3007282 )
ROM_END


ROM_START( dondenmj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "dn5.1h",   	0x00000, 0x08000, 0x3080252e )
	/* for the banks */
	ROM_LOAD( "dn1.1e",   	0x18000, 0x08000, 0x1cd9c48a )	// 1
	ROM_LOAD( "dn2.1d",   	0x20000, 0x04000, 0x7a72929d )	// 2
	ROM_LOAD( "dn3.2h",   	0x30000, 0x08000, 0xb09d2897 )	// 4
	ROM_LOAD( "dn4.2e",   	0x50000, 0x08000, 0x67d7dcd6 )	// 8

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ic6k.bin",  0x0000, 0x0020, 0x97e1defe )
ROM_END

ROM_START( suzume )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "p1.bin",   	0x0000, 0x1000, 0xe9706967 )
	ROM_LOAD( "p2.bin",   	0x1000, 0x1000, 0xdd48cd62 )
	ROM_LOAD( "p3.bin",   	0x2000, 0x1000, 0x10a05c23 )
	ROM_LOAD( "p4.bin",   	0x3000, 0x1000, 0x267eaf52 )
	ROM_LOAD( "p5.bin",   	0x4000, 0x1000, 0x2fde346b )
	ROM_LOAD( "p6.bin",   	0x5000, 0x1000, 0x57f42ac7 )
	/* for the banks */
	ROM_LOAD( "1.1a",   	0x10000, 0x08000, 0xf670dd47 )	// 0
	ROM_LOAD( "2.1c",   	0x18000, 0x08000, 0x140b11aa )	// 1
	ROM_LOAD( "3.1d",   	0x20000, 0x08000, 0x3d437b61 )	// 2
	ROM_LOAD( "4.1e",   	0x28000, 0x08000, 0x9da8952e )	// 3
	ROM_LOAD( "5.1h",   	0x30000, 0x08000, 0x04a6f41a )	// 4

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ic6k.bin",  0x0000, 0x0020, 0x97e1defe  )
ROM_END

ROM_START( tontonb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "091.5e",   	0x00000, 0x10000, 0xd8d67b59 ) /* 0x0000 - 0x6fff fixed code? */
	/* banks */
	ROM_RELOAD(             0x10000, 0x10000 )				// banks 0,1
															// banks 2,3 empty?
	ROM_LOAD( "093.5b",   	0x30000, 0x10000, 0x24b6be55 )	// banks 4,5
															// banks 6,7 empty?
	ROM_LOAD( "092.5c",   	0x50000, 0x10000, 0x7ff2738b )	// banks 8,9

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ic6k.bin",  0x0000, 0x0020, 0x97e1defe )
ROM_END

ROM_START( mjdiplob )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "071.4l",   	0x00000, 0x10000, 0x81a6d6b0  )
	/* banks */
	ROM_RELOAD(             0x10000, 0x10000 )				// banks 0,1
	ROM_LOAD( "072.4k",   	0x20000, 0x10000, 0xa992bb85  )	// banks 2,3
	ROM_LOAD( "073.4j",   	0x30000, 0x10000, 0x562ed64f  )	// banks 4,5
	ROM_LOAD( "074.4h",   	0x40000, 0x10000, 0x1eba0140  )	// banks 6,7

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ic6k.bin",  0x0000, 0x0020, 0xc1e427df  )
ROM_END

ROM_START( majs101b )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "171.3e",   	0x00000, 0x10000, 0xfa3c553b )
	/* for the banks */
															// 0-1 unused?
	ROM_RELOAD(             0x20000, 0x10000 )				// 2-3
	ROM_LOAD( "172.3f",   	0x30000, 0x20000, 0x7da39a63 )	// 4-5-6-7
	ROM_LOAD( "173.3h",   	0x50000, 0x20000, 0x7a9e71ae )	// 8-9-a-b
	ROM_LOAD( "174.3j",   	0x70000, 0x10000, 0x972c2cc9 )	// c-d

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ic6k.bin",  0x0000, 0x0020, 0xc1e427df ) // wrong prom? colours are a bit off
ROM_END



GAME( 1982, royalmah, 0, royalmah, royalmah, 0, ROT180, "Falcon", "Royal Mahjong" )
GAMEX(1986, dondenmj, 0, dondenmj, royalmah, 0, ROT180, "Dyna Electronics", "Don Den Mahjong [BET]", GAME_NOT_WORKING  )
GAMEX(1986, suzume,   0, suzume,   royalmah, 0, ROT180, "Dyna Electronics", "Watashiha Suzumechan", GAME_NOT_WORKING  )
GAME( 1987, tontonb,  0, tontonb,  tontonb,  0, ROT180, "Dynax", "Tonton [BET]" )
GAME( 1987, mjdiplob, 0, mjdiplob, mjdiplob, 0, ROT180, "Dynax", "Mahjong Diplomat [BET]" )
GAMEX(1987, majs101b, 0, majs101b, majs101b, 0, ROT180, "Dynax", "Mahjong Studio 101 [BET]", GAME_NOT_WORKING )
