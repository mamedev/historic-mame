/***************************************************************************
Note: This driver has been revised to use the sound driver.
Sound ROMS sk4_ic51.bin and sk4_ic52.bin are loaded...but don't have
the right checksums.
Andrew Scott (ascott@utkux.utcc.utk.edu)

Vanguard memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM (3 bits for video RAM 1 and 3 bits for video RAM 2)
1000-1fff Character generator RAM
4000-bfff ROM

read:
3104      IN0
3105      IN1
3106      DSW ??
3107      IN2

write
3100      Sound Port 0
3101      Sound Port 1
3103      bit 7 = flip screen
          bit 4-6 = coin counter?
3200      y scroll register
3300      x scroll register

Fantasy and Nibbler memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM (3 bits for video RAM 1 and 3 bits for video RAM 2)
1000-1fff Character generator RAM
3000-bfff ROM

read:
2104      IN0
2105      IN1
2106      DSW
2107      IN2

write
2100      Sound Port 0
2101      Sound Port 1
2103      bit 7 = flip screen
          bit 4-6 = coin counter?
          bit 3 = char bank selector (Fantasy and Nibbler only, since
                                       Vanguard has only 256 characters)
2200      y scroll register
2300      x scroll register

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

Pioneer Balloon memory map (preliminary)

0000-03ff RAM		   IC13 cpu
0400-07ff Video RAM 1  IC67 video
0800-0bff Video RAM 2  ???? video
0c00-0fff Color RAM    IC68 (3 bits for VRAM 1 and 3 bits for VRAM 2)
1000-1fff RAM		   ???? Character generator
3000-3fff ROM 4/5	   IC12
4000-4fff ROM 1 	   IC07
5000-5fff ROM 2 	   IC08
6000-6fff ROM 3 	   IC09
7000-7fff ROM 4 	   IC10
8000-8fff ROM 5 	   IC14
9000-9fff ROM 6 	   IC15
read:
b104	  IN0
b105	  IN1
b106	  DSW
b107	  IN2

write
b000	  Sound Port 0
b001	  Sound Port 1
b100	  ????
b103	  bit 7 = flip screen
          bit 4-6 = coin counter?
		  bit 3 = char bank selector
b106	  ????
b200	  y scroll register
b300	  x scroll register

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *rockola_videoram2;
extern unsigned char *rockola_characterram;
extern unsigned char *rockola_scrollx,*rockola_scrolly;

void satansat_b002_w(int offset,int data);
void satansat_backcolor_w(int offset, int data);
void satansat_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void satansat_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void satansat_characterram_w(int offset,int data);

void rockola_characterram_w(int offset,int data);
void rockola_flipscreen_w(int offset,int data);
void rockola_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void rockola_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void rockola_sound0_w(int offset,int data);
void rockola_sound1_w(int offset,int data);
int rockola_sh_start(void);
void rockola_sh_update(void);



static struct MemoryReadAddress satansat_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x4000, 0x97ff, MRA_ROM },
	{ 0xb004, 0xb004, input_port_0_r }, /* IN0 */
	{ 0xb005, 0xb005, input_port_1_r }, /* IN1 */
	{ 0xb006, 0xb006, input_port_2_r }, /* DSW */
	{ 0xb007, 0xb007, input_port_3_r }, /* IN2 */
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress satansat_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &rockola_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, rockola_characterram_w, &rockola_characterram },
	{ 0x4000, 0x97ff, MWA_ROM },
//	{ 0xb000, 0xb000, satansat_sound0_w },
//	{ 0xb001, 0xb001, satansat_sound1_w },
	{ 0xb002, 0xb002, satansat_b002_w },	/* flip screen & irq enable */
	{ 0xb003, 0xb003, satansat_backcolor_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress vanguard_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3104, 0x3104, input_port_0_r },	/* IN0 */
	{ 0x3105, 0x3105, input_port_1_r },	/* IN1 */
	{ 0x3106, 0x3106, input_port_2_r },	/* DSW */
	{ 0x3107, 0x3107, input_port_3_r },	/* IN2 */
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress vanguard_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &rockola_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, rockola_characterram_w, &rockola_characterram },
	{ 0x3100, 0x3100, rockola_sound0_w },
	{ 0x3101, 0x3101, rockola_sound1_w },
	{ 0x3103, 0x3103, rockola_flipscreen_w },
	{ 0x3200, 0x3200, MWA_RAM, &rockola_scrolly },
	{ 0x3300, 0x3300, MWA_RAM, &rockola_scrollx },
	{ 0x4000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress fantasy_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2104, 0x2104, input_port_0_r },	/* IN0 */
	{ 0x2105, 0x2105, input_port_1_r },	/* IN1 */
	{ 0x2106, 0x2106, input_port_2_r },	/* DSW */
	{ 0x2107, 0x2107, input_port_3_r },	/* IN2 */
	{ 0x3000, 0xbfff, MRA_ROM },
	{ 0xfffa, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress fantasy_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &rockola_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, rockola_characterram_w, &rockola_characterram },
	{ 0x2100, 0x2100, rockola_sound0_w },
	{ 0x2101, 0x2101, rockola_sound1_w },
	{ 0x2103, 0x2103, rockola_flipscreen_w },
	{ 0x2200, 0x2200, MWA_RAM, &rockola_scrolly },
	{ 0x2300, 0x2300, MWA_RAM, &rockola_scrollx },
	{ 0x3000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress pballoon_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0x9fff, MRA_ROM },
	{ 0xb104, 0xb104, input_port_0_r },	/* IN0 */
	{ 0xb105, 0xb105, input_port_1_r },	/* IN1 */
	{ 0xb106, 0xb106, input_port_2_r },	/* DSW */
	{ 0xb107, 0xb107, input_port_3_r },	/* IN2 */
	{ 0xfffa, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pballoon_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &rockola_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, rockola_characterram_w, &rockola_characterram },
	{ 0x3000, 0x9fff, MWA_ROM },
	{ 0xb100, 0xb100, rockola_sound0_w },
	{ 0xb101, 0xb101, rockola_sound1_w },
	{ 0xb103, 0xb103, rockola_flipscreen_w },
	{ 0xb200, 0xb200, MWA_RAM, &rockola_scrolly },
	{ 0xb300, 0xb300, MWA_RAM, &rockola_scrollx },
	{ -1 }	/* end of table */
};



static int satansat_interrupt(void)
{
	if (cpu_getiloops() != 0)
	{
		/* user asks to insert coin: generate a NMI interrupt. */
		if (readinputport(3) & 1)
			return nmi_interrupt();
		else return ignore_interrupt();
	}
	else return interrupt();	/* one IRQ per frame */
}

static int rockola_interrupt(void)
{
	if (cpu_getiloops() != 0)
	{
		/* user asks to insert coin: generate a NMI interrupt. */
		if (readinputport(4) & 1)
			return nmi_interrupt();
		else return ignore_interrupt();
	}
	else return interrupt();	/* one IRQ per frame */
}



INPUT_PORTS_START( satansat_input_ports )
    PORT_START  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
    PORT_BIT( 0x7C, IP_ACTIVE_HIGH, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

    PORT_START  /* DSW */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright")
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME (0x0a, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x02, "1 Coin/2 Credits" )
	/* 0x0a gives 2/1 again */
	PORT_DIPNAME (0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "5000" )
	PORT_DIPSETTING (   0x04, "10000" )
	PORT_DIPNAME (0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x10, "4" )
	PORT_DIPSETTING (   0x20, "5" )
	/* 0x30 gives 3 again */
	PORT_DIPNAME (0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x40, "On" )
	PORT_DIPNAME (0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x80, "On" )

    PORT_START  /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
    PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* connected to a counter - random number generator? */
INPUT_PORTS_END

INPUT_PORTS_START( vanguard_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x4e, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x42, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits + Bonus" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits + Bonus" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits + Bonus" )
	PORT_DIPSETTING(    0x0c, "1 Coin/7 Credits" )
/*
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*	PORT_DIPSETTING(    0x30, "3" ) */
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( fantasy_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x4e, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x42, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits + Bonus" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits + Bonus" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits + Bonus" )
	PORT_DIPSETTING(    0x0c, "1 Coin/7 Credits" )
/*
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*	PORT_DIPSETTING(    0x30, "3" ) */
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( pballoon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x4e, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x42, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x48, "1 Coin/2 Credits + Bonus" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x44, "1 Coin/3 Credits + Bonus" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x4c, "1 Coin/6 Credits + Bonus" )
	PORT_DIPSETTING(    0x0c, "1 Coin/7 Credits" )
/*
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x46, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4a, "1 Coin/1 Credit + Bonus" )
	PORT_DIPSETTING(    0x4e, "1 Coin/1 Credit + Bonus" )
*/
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*	PORT_DIPSETTING(    0x30, "3" ) */
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( nibbler_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Slow down */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* debug command? */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* debug command */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* debug command */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Pause */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unpause */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* End game */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* debug command */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x80, 0x00, "Bonus Every 2 Credits", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout256 =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 256*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout512 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo satansat_gfxdecodeinfo[] =
{
    { 0, 0x1000, &charlayout256,   0, 4 },	/* the game dynamically modifies this */
    { 1, 0x0000, &charlayout256, 4*4, 4 },
	{ -1 }
};

static struct GfxDecodeInfo vanguard_gfxdecodeinfo[] =
{
	{ 0, 0x1000, &charlayout256,   0, 8 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &charlayout256, 8*4, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo fantasy_gfxdecodeinfo[] =
{
	{ 0, 0x1000, &charlayout256,   0, 8 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &charlayout512, 8*4, 8 },
	{ -1 } /* end of array */
};



static unsigned char satansat_color_prom[] =
{
	0x00,0xF8,0x02,0xFF,0xF8,0x27,0xC0,0xF8,0xC0,0x80,0x07,0x07,0xFF,0xF8,0x3F,0xFF,
	0x00,0xF8,0x02,0xFF,0xC0,0xC0,0x80,0x26,0x38,0xA0,0xA0,0x04,0x07,0xC6,0xC5,0x27
};

static unsigned char vanguard_color_prom[] =
{
	/* foreground colors */
	0x00,0x2F,0xF4,0xFF,0xEF,0xF8,0xFF,0x07,0xFE,0xC0,0x07,0x3F,0xFF,0x3F,0xC6,0xC0,
	0x00,0x38,0xE7,0x07,0xEF,0xC0,0xF4,0xFF,0xFE,0xFF,0xF8,0xC0,0xFF,0xC6,0xE7,0xC0,
	/* background colors */
	0x00,0x80,0x3F,0xC6,0xEF,0xC6,0x2F,0xF8,0xFE,0xC6,0xE7,0xC0,0xFF,0x2F,0x38,0xC6,
	0x00,0x07,0x80,0x2F,0xEF,0x07,0xF8,0xFF,0xFE,0xFF,0xF8,0xC0,0xFF,0xE7,0xC6,0xF4
};

static unsigned char fantasy_color_prom[] =
{
	/* foreground colors */
	0x00,0x07,0x38,0xF8,0x00,0x05,0x2F,0xF8,0x00,0xC0,0x2F,0xF8,0x00,0xF8,0x07,0xFF,
	0x00,0x07,0xF4,0xC0,0x00,0x38,0xC5,0x07,0x00,0xFF,0x2F,0x3F,0x00,0xC5,0xC5,0xC0,
	/* background colors */
	0x00,0x06,0x27,0x05,0x7F,0x2F,0x38,0x05,0x00,0x38,0x05,0x28,0x00,0x07,0xFF,0xF4,
	0x00,0x07,0x38,0x2F,0x00,0xF8,0xFF,0xC5,0x00,0xC0,0x38,0xF4,0x00,0x27,0xFF,0x2F
};

static unsigned char pballoon_color_prom[] =
{
	/* sk8_ic7.bin - foreground colors */
	0x00,0x07,0x38,0xF8,0x00,0x2F,0x38,0x05,0x00,0xC0,0x2F,0xF8,0x00,0xF8,0x07,0xFF,
	0x00,0x07,0xF4,0xC0,0x00,0x38,0xC5,0x07,0x00,0xFF,0x2F,0x3F,0x00,0xF8,0xAD,0x07,
	/* sk8_ic6.bin - background colors */
	0x00,0x07,0xC5,0x2F,0xFF,0x2F,0x38,0x05,0x7E,0x38,0x05,0x28,0xA5,0x07,0xFF,0xF4,
	0xFF,0x07,0x38,0x2F,0xFE,0xF8,0xFF,0xC5,0xA5,0xC0,0x38,0xF4,0xFF,0x25,0x37,0x2F,
};

static unsigned char nibbler_color_prom[] =
{
	/* IC7 - foreground palette */
	0x00,0x07,0xFF,0xC5,0x00,0x38,0xAD,0xA8,0x00,0xAD,0x3F,0xC0,0x00,0xFF,0x07,0xFF,
	0x00,0x3F,0xC0,0xAD,0x00,0xFF,0xC5,0x3F,0x00,0xFF,0x3F,0x07,0x00,0x07,0xC5,0x3F,
	/* IC6 - background palette */
	0x00,0x3F,0xFF,0xC0,0x00,0xC7,0x38,0x05,0x00,0x07,0xC0,0x3F,0x00,0x3F,0xE0,0x05,
	0x00,0x07,0xAC,0xC0,0x00,0xFF,0xC5,0x2F,0x00,0xC0,0x05,0x2F,0x00,0x3F,0x05,0xC7,
};



static struct MachineDriver satansat_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,    /* 700 kHz */
			0,
			satansat_readmem,satansat_writemem,0,0,
			satansat_interrupt,2
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	satansat_gfxdecodeinfo,
	32,4*4 + 4*4,
	satansat_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	satansat_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};

static struct MachineDriver vanguard_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,    /* 1 MHz??? */
			0,
			vanguard_readmem,vanguard_writemem,0,0,
			rockola_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	vanguard_gfxdecodeinfo,
	64,16*4,
	rockola_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	rockola_vh_screenrefresh,

	/* sound hardware */
	0,
	rockola_sh_start,
	0,
	rockola_sh_update
};

static struct MachineDriver fantasy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,    /* 1 MHz??? */
			0,
			fantasy_readmem,fantasy_writemem,0,0,
			rockola_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	fantasy_gfxdecodeinfo,
	64,16*4,
	rockola_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	rockola_vh_screenrefresh,

	/* sound hardware */
	0,
	rockola_sh_start,
	0,
	rockola_sh_update
};

/* note that in this driver the visible area is different!!! */
static struct MachineDriver pballoon_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,    /* 1 MHz??? */
			0,
			pballoon_readmem,pballoon_writemem,0,0,
			rockola_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },	/* different from the others! */
	fantasy_gfxdecodeinfo,
	64,16*4,
	rockola_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	rockola_vh_screenrefresh,

	/* sound hardware */
	0,
	rockola_sh_start,
	0,
	rockola_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( satansat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "SS1",  0x4000, 0x0800, 0x48ea6052 )
	ROM_LOAD( "SS2",  0x4800, 0x0800, 0x15c02424 )
	ROM_LOAD( "SS3",  0x5000, 0x0800, 0xd6a4739a )
	ROM_LOAD( "SS4",  0x5800, 0x0800, 0x6de8e7f2 )
	ROM_LOAD( "SS5",  0x6000, 0x0800, 0x2b0d3f3b )
	ROM_LOAD( "SS6",  0x6800, 0x0800, 0x1ca8acee )
	ROM_LOAD( "SS7",  0x7000, 0x0800, 0xfbc89252 )
	ROM_LOAD( "SS8",  0x7800, 0x0800, 0xc7440c84 )
	ROM_RELOAD(       0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "SS9",  0x8000, 0x0800, 0x78362c82 )
	ROM_LOAD( "SS10", 0x8800, 0x0800, 0xa6dd58a9 )
	ROM_LOAD( "SS11", 0x9000, 0x0800, 0xa9c2a90a )

	ROM_REGION(0x1000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "SS14", 0x0000, 0x0800, 0xbc67fa61 )
	ROM_LOAD( "SS15", 0x0800, 0x0800, 0x2364fe46 )

    ROM_REGION(0x1000)  /* sound data for Vanguard-style audio section */
	ROM_LOAD( "SS12", 0x0000, 0x0800, 0xc7de2350 )
	ROM_LOAD( "SS13", 0x0800, 0x0800, 0x01380400 )
ROM_END

ROM_START( zarzon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ZARZ122.07", 0x4000, 0x0800, 0xa260b9f8 )
	ROM_LOAD( "ZARZ123.08", 0x4800, 0x0800, 0xf2b8072c )
	ROM_LOAD( "ZARZ124.09", 0x5000, 0x0800, 0xdea47b9a )
	ROM_LOAD( "ZARZ125.10", 0x5800, 0x0800, 0xa30532d5 )
	ROM_LOAD( "ZARZ126.13", 0x6000, 0x0800, 0x043c84ba )
	ROM_LOAD( "ZARZ127.14", 0x6800, 0x0800, 0xa3f1286b )
	ROM_LOAD( "ZARZ128.15", 0x7000, 0x0800, 0xfbc89252 )
	ROM_LOAD( "ZARZ129.16", 0x7800, 0x0800, 0xc7440c84 )
	ROM_RELOAD(             0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "ZARZ130.22", 0x8000, 0x0800, 0x78362c82 )
	ROM_LOAD( "ZARZ131.23", 0x8800, 0x0800, 0x566914b5 )
	ROM_LOAD( "ZARZ132.24", 0x9000, 0x0800, 0x7c4f3143 )

	ROM_REGION(0x1000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ZARZ135.73", 0x0000, 0x0800, 0xbc67fa61 )
	ROM_LOAD( "ZARZ136.75", 0x0800, 0x0800, 0x2364fe46 )

    ROM_REGION(0x1000)  /* sound data for Vanguard-style audio section */
	ROM_LOAD( "ZARZ133.53", 0x0000, 0x0800, 0x4b404b14 )
	ROM_LOAD( "ZARZ134.54", 0x0800, 0x0800, 0x01380400 )
ROM_END

ROM_START( vanguard_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000, 0x8dd6cf9a )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000, 0xa3740a46 )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000, 0x27c429cc )
	ROM_LOAD( "sk4_ic10.bin", 0x7000, 0x1000, 0x1e153237 )
	ROM_LOAD( "sk4_ic13.bin", 0x8000, 0x1000, 0x32820c48 )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000, 0x4b34aaea )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000, 0x64432d1d )
	ROM_LOAD( "sk4_ic16.bin", 0xb000, 0x1000, 0xb4d9810f )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800, 0x6a6bc3f7 )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800, 0x7eb71bd1 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "sk4_ic51.bin", 0x0000, 0x0800, 0x7f305f4c )  /* sound ROM 1 */
	ROM_LOAD( "sk4_ic52.bin", 0x0800, 0x0800, 0xe6fb89fb )  /* sound ROM 2 */
ROM_END

static const char *vanguard_sample_names[]={
        "fire.sam",
        "explsion.sam",
        0
};

ROM_START( fantasy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic12.cpu", 0x3000, 0x1000, 0xbe01e827 )
	ROM_LOAD( "ic07.cpu", 0x4000, 0x1000, 0x66f038dc )
	ROM_LOAD( "ic08.cpu", 0x5000, 0x1000, 0x89f3afb1 )
	ROM_LOAD( "ic09.cpu", 0x6000, 0x1000, 0x3cc9498f )
	ROM_LOAD( "ic10.cpu", 0x7000, 0x1000, 0x120a97b6 )
	ROM_LOAD( "ic14.cpu", 0x8000, 0x1000, 0xf3845850 )
	ROM_RELOAD(           0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "ic15.cpu", 0x9000, 0x1000, 0xfa21e08f )
	ROM_LOAD( "ic16.cpu", 0xa000, 0x1000, 0x757af434 )
	ROM_LOAD( "ic17.cpu", 0xb000, 0x1000, 0xb5417a91 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic50.vid", 0x0000, 0x1000, 0xcfa87668 )
	ROM_LOAD( "ic51.vid", 0x1000, 0x1000, 0xf9730c57 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "ic51.cpu", 0x0000, 0x0800, 0xf433dd21 )
	ROM_LOAD( "ic52.cpu", 0x0800, 0x0800, 0xd968fa48 )

/*	ROM_LOAD( "ic53.cpu", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic07.dau", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic08.dau", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic11.dau", 0x????, 0x0800 ) ?? */
ROM_END

ROM_START( pballoon_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "sk7_ic12.bin", 0x3000, 0x1000, 0x9fef177d )
	ROM_LOAD( "sk7_ic07.bin", 0x4000, 0x1000, 0x04216b23 )
	ROM_LOAD( "sk7_ic08.bin", 0x5000, 0x1000, 0xc186fcb8 )
	ROM_LOAD( "sk7_ic09.bin", 0x6000, 0x1000, 0x45265972 )
	ROM_LOAD( "sk7_ic10.bin", 0x7000, 0x1000, 0xf7c9c595 )
	ROM_LOAD( "sk7_ic14.bin", 0x8000, 0x1000, 0x1862375a )
	ROM_RELOAD(               0xf000, 0x1000 )  /* for the reset and interrupt vectors */
	ROM_LOAD( "sk7_ic15.bin", 0x9000, 0x1000, 0x7ed30423 )

	ROM_REGION(0x2000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sk8_ic50.bin", 0x0000, 0x1000, 0xda255685 )
	ROM_LOAD( "sk8_ic51.bin", 0x1000, 0x1000, 0xd40a2fd2 )

	ROM_REGION(0x1800)  /* space for the sound ROMs */
	ROM_LOAD( "sk7_ic51.bin", 0x0000, 0x0800, 0xecf5012f )  /* sound ROM 1 */
	ROM_LOAD( "sk7_ic52.bin", 0x0800, 0x0800, 0xfe490c2f )  /* sound ROM 2 */
	ROM_LOAD( "sk7_ic53.bin", 0x1000, 0x0800, 0x1dee0008 )  /* sound ROM 3 */
ROM_END

ROM_START( nibbler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "IC12", 0x3000, 0x1000, 0x63b52ccf )
	ROM_LOAD( "IC07", 0x4000, 0x1000, 0x5de3931d )
	ROM_LOAD( "IC08", 0x5000, 0x1000, 0x4f2b5fc9 )
	ROM_LOAD( "IC09", 0x6000, 0x1000, 0x157dbec5 )
	ROM_LOAD( "IC10", 0x7000, 0x1000, 0xa04d2ef5 )
	ROM_LOAD( "IC14", 0x8000, 0x1000, 0x4d9217c2 )
	ROM_RELOAD(       0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "IC15", 0x9000, 0x1000, 0x88b3edc7 )
	ROM_LOAD( "IC16", 0xa000, 0x1000, 0x7a1d25a9 )
	ROM_LOAD( "IC17", 0xb000, 0x1000, 0xd000eafc )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC50", 0x0000, 0x1000, 0x2bc0b3f0 )
	ROM_LOAD( "IC51", 0x1000, 0x1000, 0x27bd8de9 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "G960-45.53", 0x0000, 0x0800, 0xaafe2944 )
	ROM_LOAD( "IC52", 0x0800, 0x0800, 0x9ad746f9 )
ROM_END

ROM_START( nibblera_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "G960-52.12", 0x3000, 0x1000, 0xdb018bff )
	ROM_LOAD( "G960-48.07", 0x4000, 0x1000, 0xe07e2a60 )
	ROM_LOAD( "G960-49.08", 0x5000, 0x1000, 0x9499252b )
	ROM_LOAD( "G960-50.09", 0x6000, 0x1000, 0xa06e7e18 )
	ROM_LOAD( "G960-51.10", 0x7000, 0x1000, 0x5fddf069 )
	ROM_LOAD( "G960-53.14", 0x8000, 0x1000, 0xb4cd5f77 )
	ROM_RELOAD(       0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "G960-54.15", 0x9000, 0x1000, 0x88b3edc7 )
	ROM_LOAD( "G960-55.16", 0xa000, 0x1000, 0x7a1d25a9 )
	ROM_LOAD( "G960-56.17", 0xb000, 0x1000, 0xd000eafc )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "G960-57.50", 0x0000, 0x1000, 0x2bc0b3f0 )
	ROM_LOAD( "G960-58.51", 0x1000, 0x1000, 0xab79f1ad )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "G960-45.53", 0x0000, 0x0800, 0xaafe2944 )
	ROM_LOAD( "G960-44.52", 0x0800, 0x0800, 0x9ad646f8 )
ROM_END



static int vanguard_hiload(void)     /* V.V */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0025],"\x00\x10",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0025],3);
			osd_fread(f,&RAM[0x0220],112);
			osd_fread(f,&RAM[0x02a0],16);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */

}

static void vanguard_hisave(void)    /* V.V */
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0025],3);
		osd_fwrite(f,&RAM[0x0220],112);
		osd_fwrite(f,&RAM[0x02a0],16);
		osd_fclose(f);
	}
}

static int fantasy_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0025],"\x00\x20\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0025],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void fantasy_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0025],3);
		osd_fclose(f);
	}
}

static int nibbler_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0290],"\x00\x50\x00\x00",4) == 0 &&
			memcmp(&RAM[0x02b4],"\x00\x05\x00\x00",4) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0290],4*10);
			osd_fread(f,&RAM[0x02d0],3*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void nibbler_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0290],4*10);
		osd_fwrite(f,&RAM[0x02d0],3*10);
		osd_fclose(f);
	}
}



struct GameDriver satansat_driver =
{
	__FILE__,
	0,
    "satansat",
    "Satan of Saturn",
	"1981",
	"SNK",
    "Dan Boris\nTheo Philips",
	0,
	&satansat_machine_driver,

    satansat_rom,
	0, 0,
	0,
	0,	/* sound_prom */

    satansat_input_ports,

    satansat_color_prom,0,0,
    ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver zarzon_driver =
{
	__FILE__,
	&satansat_driver,
    "zarzon",
    "Zarzon",
	"1981",
	"[SNK] (Taito America license)",
    "Dan Boris\nTheo Philips",
	0,
	&satansat_machine_driver,

    zarzon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

    satansat_input_ports,

    satansat_color_prom,0,0,
    ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver vanguard_driver =
{
	__FILE__,
	0,
	"vanguard",
	"Vanguard",
	"1981",
	"SNK",
	"Brian Levine (Vanguard emulator)\nBrad Oliver (MAME driver)\nMirko Buffoni (MAME driver)\nAndrew Scott\nValerio Verrando (high score save)",
	0,
	&vanguard_machine_driver,

	vanguard_rom,
	0, 0,
	vanguard_sample_names,
	0,	/* sound_prom */

	vanguard_input_ports,

	vanguard_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	vanguard_hiload, vanguard_hisave
};

struct GameDriver fantasy_driver =
{
	__FILE__,
	0,
	"fantasy",
	"Fantasy",
	"1981",
	"Rock-ola",
	"Nicola Salmoria\nBrian Levine\nMirko Buffoni\nDani Portillo (high score save)",
	0,
	&fantasy_machine_driver,

	fantasy_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	fantasy_input_ports,

	fantasy_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	fantasy_hiload, fantasy_hisave
};

struct GameDriver pballoon_driver =
{
	__FILE__,
	0,
	"pballoon",
	"Pioner Balloon",
	"1982",
	"SNK",
	"Nicola Salmoria\nBrian Levine\nMirko Buffoni",
	0,
	&pballoon_machine_driver,

	pballoon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pballoon_input_ports,

	pballoon_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver nibbler_driver =
{
	__FILE__,
	0,
	"nibbler",
	"Nibbler (set 1)",
	"1982",
	"Rock-ola",
	"Nicola Salmoria\nBrian Levine\nMirko Buffoni\nMarco Cassili",
	0,
	&fantasy_machine_driver,

	nibbler_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	nibbler_input_ports,

	nibbler_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	nibbler_hiload, nibbler_hisave
};

struct GameDriver nibblera_driver =
{
	__FILE__,
	&nibbler_driver,
	"nibblera",
	"Nibbler (set 2)",
	"1982",
	"Rock-ola",
	"Nicola Salmoria\nBrian Levine\nMirko Buffoni\nMarco Cassili",
	0,
	&fantasy_machine_driver,

	nibblera_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	nibbler_input_ports,

	nibbler_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	nibbler_hiload, nibbler_hisave
};
