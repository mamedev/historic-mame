/*****************************************************************************

	Irem M92 system games:

	Blademaster								(c) 1991 Irem Corp
	Gunforce (World set)			M92-B?	(c) 1991 Irem Corp
	Gunforce (USA set)				M92-B?	(c) 1991 Irem America Corp
	Lethal Thunder							(c) 1991 Irem Corp
	Hook (World set)						(c) 1992 Irem Corp
	Hook (USA set)							(c) 1992 Irem America Corp
	Undercover Cops							(c) 1992 Irem Corp
	Rtype Leo (Japan)						(c) 1992 Irem Corp
	Major Title 2  							(c) 1992 Irem Corp
	The Irem Skins Game (USA Set 1)			(c) 1992 Irem America Corp
	The Irem Skins Game (USA Set 2)			(c) 1992 Irem America Corp
	In The Hunt						M92-E	(c) 1993 Irem America Corp
	Ninja Baseball Batman (Not working)		(c) 1993 Irem America Corp


	Other games thought to be on this hardware:

	Lethal Thunder 2
	Mystic Riders/Gun Hoki

	Once decrypted, Irem M97 games will be very close to this hardware.


System notes:
	Each game has an encrypted sound cpu (see m97.c), the sound cpu and
	the sprite chip are on the game board rather than the main board and
	can differ between games.

	Irem Skins Game has an eeprom and ticket payout(?).
	R-Type Leo & Lethal Thunder have a memory card.

	Many games use raster IRQ's for special video effects, eg,
		* Scrolling water in Undercover Cops
		* Score display in R-Type Leo

	These are slow to emulate, and can be turned on/off by pressing
	F1 - they are on by default.


Glitch list!

	Hook:
		Crashes very soon into the game.

	R-Type Leo:
		Title screen is incorrect, it uses mask sprites but I can't find
		how the effect is turned on/off
		Colour flicker on level 2
		Crashes after continue screen (related to memcard?)
		Crash after level 3?

	In The Hunt:
		Crash in level 3?

	Gunforce:
		Random crashes
		Water doesn't appear on last(?) level.
		Waterfalls on level 3(?) should appear over sprites.
		^- Bit 0x8 in playfield 3 set - new type of split tilemap?

	Lethal Thunder;
		Starts with 99 credits (related to memcard?)

	Irem Skins:
		Sprites & tiles written with bad colour values.
		Eeprom load/save not yet implemented - when done, MT2EEP should
		  be removed from the ROM definition.

	Blade Master:
		Sprite list register isn't updated in service mode.

	Ninja Baseball:
		Very picky about interrupts..
		Doesn't work!

	Emulation by Bryan McPhail, mish@tendril.force9.net
	Thanks to Chris Hardy and Olli Bergmann too!

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m92.h"

static int m92_irq_vectorbase,m92_vblank,raster_enable;
static unsigned char *m92_eeprom,*m92_ram;

#define M92_IRQ_0 ((m92_irq_vectorbase+0)/4)  /* VBL interrupt*/
#define M92_IRQ_1 ((m92_irq_vectorbase+4)/4)  /* End of VBL interrupt?? */
#define M92_IRQ_2 ((m92_irq_vectorbase+8)/4)  /* Raster interrupt */
#define M92_IRQ_3 ((m92_irq_vectorbase+12)/4) /* End of raster interrupt?? */

/* From vidhrdw/m92.c */
void m92_spritecontrol_w(int offset,int data);
void m92_spritebuffer_w(int offset,int data);
int m92_vram_r(int offset);
void m92_vram_w(int offset,int data);
void m92_pf1_control_w(int offset,int data);
void m92_pf2_control_w(int offset,int data);
void m92_pf3_control_w(int offset,int data);
void m92_master_control_w(int offset,int data);
int m92_vh_start(void);
void m92_vh_stop(void);
void m92_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void m92_vh_raster_partial_refresh(struct osd_bitmap *bitmap,int start_line,int end_line);

extern int m92_sprite_list,m92_raster_irq_position,m92_spritechip;
extern unsigned char *m92_vram_data,*m92_spriteram,*m92_spritecontrol;

/*****************************************************************************/

static int status_port_r(int offset)
{
if (errorlog) fprintf(errorlog,"%06x: status %04x\n",cpu_get_pc(),offset);

/*

	Gunforce waits on bit 1 (word) going high on bootup, just after
	setting up interrupt controller.

	Gunforce reads bit 2 (word) on every coin insert, doesn't seem to
	do much though..

	Ninja Batman polls this.

	R-Type Leo reads it now & again..

*/
	return 0xff;
}

static int m92_eeprom_r(int offset)
{
	unsigned char *RAM = Machine->memory_region[4];
//	if (errorlog) fprintf(errorlog,"EEPROM %04x\n",offset);
	return RAM[offset/2];
}

static void m92_coincounter_w(int offset, int data)
{
	if (offset==0) {
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* Bit 0x8 is Motor(?!), used in Hook, In The Hunt, UCops */
		/* Bit 0x8 is Memcard related in RTypeLeo */
		/* Bit 0x40 set in Blade Master test mode input check */
	}
#if 0
	if (offset==0 && data!=0) {
		char t[16];
		sprintf(t,"%02x",data);
		usrintf_showmessage(t);
	}
#endif
}

static void m92_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[0];

//	if (errorlog) fprintf(errorlog,"%04x: Bank %04x\n",cpu_get_pc(),data);
	if (offset==1) return; /* Unused top byte */

	cpu_setbank(1,&RAM[0x100000 + ((data&0x7)*0x10000)]);
}

static int m92_port_4_r(int offset)
{
	if (m92_vblank) return readinputport(4) | 0;
	return readinputport(4) | 0x80;
}

static void m92_soundlatch_w(int offset, int data)
{
	if (offset==0) soundlatch_w(0,data);
	/* Interrupt second V33 */
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x00000, 0x9ffff, MRA_ROM },
	{ 0xa0000, 0xbffff, MRA_BANK1 },
	{ 0xc0000, 0xcffff, MRA_BANK2 }, /* Mirror of rom:  Used by In The Hunt as protection */
	{ 0xd0000, 0xdffff, m92_vram_r },
	{ 0xe0000, 0xeffff, MRA_RAM },
	{ 0xf0000, 0xf3fff, m92_eeprom_r }, /* Eeprom, Major Title 2 only */
	{ 0xf8000, 0xf87ff, MRA_RAM },
	{ 0xf8800, 0xf8fff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x00000, 0xbffff, MWA_ROM },
	{ 0xd0000, 0xdffff, m92_vram_w, &m92_vram_data },
	{ 0xe0000, 0xeffff, MWA_RAM, &m92_ram }, /* System ram */
	{ 0xf0000, 0xf3fff, MWA_RAM, &m92_eeprom }, /* Eeprom, Major Title 2 only */
	{ 0xf8000, 0xf87ff, MWA_RAM, &spriteram },
	{ 0xf8800, 0xf8fff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xf9000, 0xf900f, m92_spritecontrol_w, &m92_spritecontrol },
	{ 0xf9800, 0xf9801, m92_spritebuffer_w },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress lethalth_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0x80000, 0x8ffff, m92_vram_r },
	{ 0xe0000, 0xeffff, MRA_RAM },
	{ 0xf8000, 0xf87ff, MRA_RAM },
	{ 0xf8800, 0xf8fff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress lethalth_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0x80000, 0x8ffff, m92_vram_w, &m92_vram_data },
	{ 0xe0000, 0xeffff, MWA_RAM, &m92_ram }, /* System ram */
	{ 0xf8000, 0xf87ff, MWA_RAM, &spriteram },
	{ 0xf8800, 0xf8fff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xf9000, 0xf900f, m92_spritecontrol_w, &m92_spritecontrol },
	{ 0xf9800, 0xf9801, m92_spritebuffer_w },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r }, /* Player 1 */
	{ 0x01, 0x01, input_port_1_r }, /* Player 2 */
	{ 0x02, 0x02, m92_port_4_r },   /* Coins & VBL */
	{ 0x03, 0x03, input_port_7_r }, /* Dip 3 */
	{ 0x04, 0x04, input_port_6_r }, /* Dip 2 */
	{ 0x05, 0x05, input_port_5_r }, /* Dip 1 */
	{ 0x06, 0x06, input_port_2_r }, /* Player 3 */
	{ 0x07, 0x07, input_port_3_r }, /* Player 4 */
	{ 0x08, 0x09, status_port_r },  /* ? */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x01, m92_soundlatch_w },
	{ 0x02, 0x03, m92_coincounter_w },
	{ 0x20, 0x21, m92_bankswitch_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x87, m92_pf1_control_w },
	{ 0x88, 0x8f, m92_pf2_control_w },
	{ 0x90, 0x97, m92_pf3_control_w },
	{ 0x98, 0x9f, m92_master_control_w },
	{ -1 }	/* end of table */
};

/******************************************************************************/

#if 0
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x00000, 0x1ffff, MRA_ROM },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x00000, 0x1ffff, MWA_ROM },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};
#endif

/******************************************************************************/

INPUT_PORTS_START( bmaster_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( gunforce_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( lethalth_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( hook_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_PLAYER3_2BUTTON_JOYSTICK
	PORT_PLAYER4_2BUTTON_JOYSTICK
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( skingame_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( uccops_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_PLAYER3_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( rtypeleo_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( inthunt_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Very Easy" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Any Button starts game" ) /* ? */
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_UNUSED	/* Game manual only mentions 2 dips */
INPUT_PORTS_END

INPUT_PORTS_START( nbbatman_input_ports )
	PORT_PLAYER1_2BUTTON_JOYSTICK
	PORT_PLAYER2_2BUTTON_JOYSTICK
	PORT_UNUSED
	PORT_UNUSED
	PORT_COINS_VBLANK
	PORT_SYSTEM_DIPSWITCH

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* One of these is continue */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	65536,	/* 65536 characters */
	4,	/* 4 bits per pixel */
	{ 0x180000*8, 0x100000*8, 0x80000*8, 0x000000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	0x8000,
	4,
	{ 0x300000*8, 0x200000*8, 0x100000*8, 0x000000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0, 64 },
	{ 1, 0x200000, &spritelayout, 0, 64 },
	{ -1 } /* end of array */
};

/***************************************************************************/

static int m92_raster_interrupt(void)
{
	static int last_line=0,m92_last_raster_irq_position;
	int line = 256 - cpu_getiloops();

	if (keyboard_pressed_memory(KEYCODE_F1)) {
		raster_enable ^= 1;
		if (raster_enable)
			usrintf_showmessage("Raster IRQ enabled");
		else
			usrintf_showmessage("Raster IRQ disabled");
	}

	/* Raster interrupt */
	if (raster_enable && line==m92_raster_irq_position) {
		if (osd_skip_this_frame()==0)
			m92_vh_raster_partial_refresh(Machine->scrbitmap,last_line,line);
		last_line=line+1;
		m92_last_raster_irq_position=m92_raster_irq_position;

		return M92_IRQ_2;
	}

#if 0
	/* This is not quite accurate, it should be at the end of the above line? */
	if (raster_enable && line==m92_last_raster_irq_position+1) {
		m92_last_raster_irq_position=-2;
		return M92_IRQ_3;
	}
#endif

	/* Redraw screen, we do this *before* the VBL IRQ to ensure no sprite lag or flicker */
	if (line==248) {
		if (osd_skip_this_frame()==0)
			m92_vh_raster_partial_refresh(Machine->scrbitmap,last_line,248);
		last_line=0;//check

		return 0;
	}

	/* Vblank interrupt */
	if (line==249) {
		/* Get length of the sprite list, before taking the interrupt */
		if (m92_spritechip==0)
			m92_sprite_list=(0x10000 - (m92_spritecontrol[0] | (m92_spritecontrol[1]<<8)))&0xff;
		m92_vblank=1;
		return M92_IRQ_0;
	}

	/* End of vblank, allow 2 lines for processing of this irq (guess) */
	if (line==254) {
		m92_vblank=0;
		return 0; //M92_IRQ_1; /* Enables Batman to run, but breaks Skingame */
	}

#if 0 /* Kludge to allow Batman to boot */
	if (line==10)
		return M92_IRQ_1;
	if (line==11)
		return M92_IRQ_3;
#endif

	return 0;
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V33,	/* NEC V33 */
			18000000,	/* 18 MHz */
			0,
			readmem,writemem,readport,writeport,
			m92_raster_interrupt,256 /* 8 prelines, 240 visible lines, 8 for vblank? */
		},
#if 0
		{
			CPU_V33 | CPU_AUDIO_CPU,
			14318180,	/* 14.31818 MHz */
			2,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 512, { 80, 511-112, 128+8, 511-128-8 }, /* 320 x 240 */

	gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m92_vh_start,
	m92_vh_stop,
	m92_vh_screenrefresh,

	/* sound hardware */
#if 0
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
#endif
};

static struct MachineDriver lethalth_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_V33,	/* NEC V33 */
			18000000,	/* 18MHz clock */
			0,
			lethalth_readmem,lethalth_writemem,readport,writeport,
			m92_raster_interrupt,256 /* 8 prelines, 240 visible lines, 8 for vblank? */
		},
#if 0
		{
			CPU_V33 | CPU_AUDIO_CPU,
			14318180,	/* 14.31818 MHz */
			2,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 512, { 80, 511-112, 128+8, 511-128-8 }, /* 320 x 240 */

	gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m92_vh_start,
	m92_vh_stop,
	m92_vh_screenrefresh,

	/* sound hardware */
#if 0
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
#endif
};

/***************************************************************************/

ROM_START( bmaster_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "bm_d-h0.rom",  0x000000, 0x40000, 0x49b257c7 )
	ROM_LOAD_V20_ODD ( "bm_d-l0.rom",  0x000000, 0x40000, 0xa873523e )
	ROM_LOAD_V20_EVEN( "bm_d-h1.rom",  0x080000, 0x10000, 0x082b7158 )
	ROM_LOAD_V20_ODD ( "bm_d-l1.rom",  0x080000, 0x10000, 0x6ff0c04e )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bm_c0.rom",       0x000000, 0x40000, 0x2cc966b8 )
	ROM_LOAD( "bm_c1.rom",       0x080000, 0x40000, 0x46df773e )
	ROM_LOAD( "bm_c2.rom",       0x100000, 0x40000, 0x05b867bd )
	ROM_LOAD( "bm_c3.rom",       0x180000, 0x40000, 0x0a2227a4 )

	ROM_LOAD( "bm_000.rom",      0x200000, 0x80000, 0x339fc9f3 )
	ROM_LOAD( "bm_010.rom",      0x300000, 0x80000, 0x6a14377d )
	ROM_LOAD( "bm_020.rom",      0x400000, 0x80000, 0x31532198 )
	ROM_LOAD( "bm_030.rom",      0x500000, 0x80000, 0xd1a041d3 )

	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "bm_d-sh0.rom",  0x000000, 0x10000, 0x9f7c075b )
	ROM_LOAD_V20_ODD ( "bm_d-sl0.rom",  0x000000, 0x10000, 0x1fa87c89 )

	ROM_REGION(0x80000)
	ROM_LOAD( "bm_da.rom",       0x000000, 0x80000, 0x62ce5798 )
ROM_END

ROM_START( skingame_rom )
	ROM_REGION(0x180000)
	ROM_LOAD_V20_EVEN( "is-h0-d",  0x000000, 0x40000, 0x80940abb )
	ROM_LOAD_V20_ODD ( "is-l0-d",  0x000000, 0x40000, 0xb84beed6 )
	ROM_LOAD_V20_EVEN( "is-h1",    0x100000, 0x40000, 0x9ba8e1f2 )
	ROM_LOAD_V20_ODD ( "is-l1",    0x100000, 0x40000, 0xe4e00626 )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c0",       0x000000, 0x40000, 0x7e61e4b5 )
	ROM_LOAD( "c1",       0x080000, 0x40000, 0x0a667564 )
	ROM_LOAD( "c2",       0x100000, 0x40000, 0x5eb44312 )
	ROM_LOAD( "c3",       0x180000, 0x40000, 0xf2866294 )

	ROM_LOAD( "k30",      0x200000, 0x100000, 0x8c9a2678 )
	ROM_LOAD( "k31",      0x300000, 0x100000, 0x5455df78 )
	ROM_LOAD( "k32",      0x400000, 0x100000, 0x3a258c41 )
	ROM_LOAD( "k33",      0x500000, 0x100000, 0xc1e91a14 )

	ROM_REGION(0x100000)	/* 64k for the audio CPU */
	ROM_LOAD_V20_EVEN( "mt2sh0",  0x000000, 0x10000, 0x1ecbea43 )
	ROM_LOAD_V20_ODD ( "mt2sl0",  0x000000, 0x10000, 0x8fd5b531 )

	ROM_REGION(0x80000)
	ROM_LOAD( "da",        0x000000, 0x80000, 0x713b9e9f )

	ROM_REGION(0x4000)	/* EEPROM */
	ROM_LOAD( "mt2eep",       0x000000, 0x800, 0x208af971 )
ROM_END

ROM_START( majtitl2_rom )
	ROM_REGION(0x180000)
	ROM_LOAD_V20_EVEN( "mt2-ho-b.5m",0x000000, 0x40000, 0xb163b12e )
	ROM_LOAD_V20_ODD ( "mt2-lo-b.5f",0x000000, 0x40000, 0x6f3b5d9d )
	ROM_LOAD_V20_EVEN( "is-h1",      0x100000, 0x40000, 0x9ba8e1f2 )
	ROM_LOAD_V20_ODD ( "is-l1",      0x100000, 0x40000, 0xe4e00626 )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c0",       0x000000, 0x40000, 0x7e61e4b5 )
	ROM_LOAD( "c1",       0x080000, 0x40000, 0x0a667564 )
	ROM_LOAD( "c2",       0x100000, 0x40000, 0x5eb44312 )
	ROM_LOAD( "c3",       0x180000, 0x40000, 0xf2866294 )

	ROM_LOAD( "k30",      0x200000, 0x100000, 0x8c9a2678 )
	ROM_LOAD( "k31",      0x300000, 0x100000, 0x5455df78 )
	ROM_LOAD( "k32",      0x400000, 0x100000, 0x3a258c41 )
	ROM_LOAD( "k33",      0x500000, 0x100000, 0xc1e91a14 )

	ROM_REGION(0x100000)	/* 64k for the audio CPU */
	ROM_LOAD_V20_EVEN( "mt2sh0",  0x000000, 0x10000, 0x1ecbea43 )
	ROM_LOAD_V20_ODD ( "mt2sl0",  0x000000, 0x10000, 0x8fd5b531 )

	ROM_REGION(0x80000)
	ROM_LOAD( "da",        0x000000, 0x80000, 0x713b9e9f )

	ROM_REGION(0x4000)	/* EEPROM */
	ROM_LOAD( "mt2eep",       0x000000, 0x800, 0x208af971 )
ROM_END

ROM_START( skingam2_rom )
	ROM_REGION(0x180000)
	ROM_LOAD_V20_EVEN( "mt2h0a", 0x000000, 0x40000, 0x7c6dbbc7 )
	ROM_LOAD_V20_ODD ( "mt2l0a", 0x000000, 0x40000, 0x9de5f689 )
	ROM_LOAD_V20_EVEN( "is-h1",  0x100000, 0x40000, 0x9ba8e1f2 )
	ROM_LOAD_V20_ODD ( "is-l1",  0x100000, 0x40000, 0xe4e00626 )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c0",       0x000000, 0x40000, 0x7e61e4b5 )
	ROM_LOAD( "c1",       0x080000, 0x40000, 0x0a667564 )
	ROM_LOAD( "c2",       0x100000, 0x40000, 0x5eb44312 )
	ROM_LOAD( "c3",       0x180000, 0x40000, 0xf2866294 )

	ROM_LOAD( "k30",      0x200000, 0x100000, 0x8c9a2678 )
	ROM_LOAD( "k31",      0x300000, 0x100000, 0x5455df78 )
	ROM_LOAD( "k32",      0x400000, 0x100000, 0x3a258c41 )
	ROM_LOAD( "k33",      0x500000, 0x100000, 0xc1e91a14 )

	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "mt2sh0",  0x000000, 0x10000, 0x1ecbea43 )
	ROM_LOAD_V20_ODD ( "mt2sl0",  0x000000, 0x10000, 0x8fd5b531 )

	ROM_REGION(0x80000)
	ROM_LOAD( "da",        0x000000, 0x80000, 0x713b9e9f )

	ROM_REGION(0x4000)	/* EEPROM */
	ROM_LOAD( "mt2eep",       0x000000, 0x800, 0x208af971 )
ROM_END

ROM_START( gunforce_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "gf_h0-c.rom",  0x000000, 0x20000, 0xc09bb634 )
	ROM_LOAD_V20_ODD ( "gf_l0-c.rom",  0x000000, 0x20000, 0x1bef6f7d )
	ROM_LOAD_V20_EVEN( "gf_h1-c.rom",  0x040000, 0x20000, 0xc84188b7 )
	ROM_LOAD_V20_ODD ( "gf_l1-c.rom",  0x040000, 0x20000, 0xb189f72a )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gf_c0.rom",       0x000000, 0x40000, 0xb3b74979 )
	ROM_LOAD( "gf_c1.rom",       0x080000, 0x40000, 0xf5c8590a )
	ROM_LOAD( "gf_c2.rom",       0x100000, 0x40000, 0x30f9fb64 )
	ROM_LOAD( "gf_c3.rom",       0x180000, 0x40000, 0x87b3e621 )

	ROM_LOAD( "gf_000.rom",      0x200000, 0x40000, 0x209e8e8d )
	ROM_LOAD( "gf_010.rom",      0x300000, 0x40000, 0x6e6e7808 )
	ROM_LOAD( "gf_020.rom",      0x400000, 0x40000, 0x6f5c3cb0 )
	ROM_LOAD( "gf_030.rom",      0x500000, 0x40000, 0x18978a9f )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "gf_sh0.rom",0x000000, 0x010000, 0x3f8f16e0 )
	ROM_LOAD_V20_ODD ( "gf_sl0.rom",0x000000, 0x010000, 0xdb0b13a3 )

	ROM_REGION(0x80000)	/* ADPCM samples */
	ROM_LOAD( "gf-da.rom",	 0x000000, 0x020000, 0x933ba935 )
ROM_END

ROM_START( gunforcu_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_V20_EVEN( "gf_h0-d.5m",  0x000000, 0x20000, 0xa6db7b5c )
	ROM_LOAD_V20_ODD ( "gf_l0-d.5f",  0x000000, 0x20000, 0x82cf55f6 )
	ROM_LOAD_V20_EVEN( "gf_h1-d.5l",  0x040000, 0x20000, 0x08a3736c )
	ROM_LOAD_V20_ODD ( "gf_l1-d.5j",  0x040000, 0x20000, 0x435f524f )

	ROM_REGION_DISPOSE(0x600000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gf_c0.rom",       0x000000, 0x40000, 0xb3b74979 )
	ROM_LOAD( "gf_c1.rom",       0x080000, 0x40000, 0xf5c8590a )
	ROM_LOAD( "gf_c2.rom",       0x100000, 0x40000, 0x30f9fb64 )
	ROM_LOAD( "gf_c3.rom",       0x180000, 0x40000, 0x87b3e621 )

	ROM_LOAD( "gf_000.rom",      0x200000, 0x40000, 0x209e8e8d )
	ROM_LOAD( "gf_010.rom",      0x300000, 0x40000, 0x6e6e7808 )
	ROM_LOAD( "gf_020.rom",      0x400000, 0x40000, 0x6f5c3cb0 )
	ROM_LOAD( "gf_030.rom",      0x500000, 0x40000, 0x18978a9f )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "gf_sh0.rom",0x000000, 0x010000, 0x3f8f16e0 )
	ROM_LOAD_V20_ODD ( "gf_sl0.rom",0x000000, 0x010000, 0xdb0b13a3 )

	ROM_REGION(0x80000)	/* ADPCM samples */
	ROM_LOAD( "gf-da.rom",	 0x000000, 0x020000, 0x933ba935 )
ROM_END

ROM_START( inthunt_rom )
	ROM_REGION(0x100000) /* Region 0 - v33 main cpu */
	ROM_LOAD_V20_EVEN( "ith-h0-d.rom",0x000000, 0x040000, 0x52f8e7a6 )
	ROM_LOAD_V20_ODD ( "ith-l0-d.rom",0x000000, 0x040000, 0x5db79eb7 )
	ROM_LOAD_V20_EVEN( "ith-h1-b.rom",0x080000, 0x020000, 0xfc2899df )
	ROM_LOAD_V20_ODD ( "ith-l1-b.rom",0x080000, 0x020000, 0x955a605a )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "ith_ic26.rom",0x000000, 0x080000, 0x4c1818cf )
	ROM_LOAD( "ith_ic25.rom",0x080000, 0x080000, 0x91145bae )
	ROM_LOAD( "ith_ic24.rom",0x100000, 0x080000, 0xfc03fe3b )
	ROM_LOAD( "ith_ic23.rom",0x180000, 0x080000, 0xee156a0a )

	ROM_LOAD( "ith_ic34.rom",0x200000, 0x100000, 0xa019766e )
	ROM_LOAD( "ith_ic35.rom",0x300000, 0x100000, 0x3fca3073 )
	ROM_LOAD( "ith_ic36.rom",0x400000, 0x100000, 0x20d1b28b )
	ROM_LOAD( "ith_ic37.rom",0x500000, 0x100000, 0x90b6fd4b )

	ROM_REGION(0x100000)	/* Irem D8000011A1 */
	ROM_LOAD_V20_EVEN( "ith-sh0.rom",0x000000, 0x010000, 0x209c8b7f )
	ROM_LOAD_V20_ODD ( "ith-sl0.rom",0x000000, 0x010000, 0x18472d65 )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "ith_ic9.rom" ,0x000000, 0x080000, 0x318ee71a )
ROM_END

ROM_START( kaiteids_rom )
	ROM_REGION(0x100000) /* Region 0 - v33 main cpu */
	ROM_LOAD_V20_EVEN( "ith-h0j.bin", 0x000000, 0x040000, 0xdc1dec36 )
	ROM_LOAD_V20_ODD ( "ith-l0j.bin", 0x000000, 0x040000, 0x8835d704 )
	ROM_LOAD_V20_EVEN( "ith-h1j.bin", 0x080000, 0x020000, 0x5a7b212d )
	ROM_LOAD_V20_ODD ( "ith-l1j.bin", 0x080000, 0x020000, 0x4c084494 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "ith_ic26.rom",0x000000, 0x080000, 0x4c1818cf )
	ROM_LOAD( "ith_ic25.rom",0x080000, 0x080000, 0x91145bae )
	ROM_LOAD( "ith_ic24.rom",0x100000, 0x080000, 0xfc03fe3b )
	ROM_LOAD( "ith_ic23.rom",0x180000, 0x080000, 0xee156a0a )

	ROM_LOAD( "ith_ic34.rom",0x200000, 0x100000, 0xa019766e )
	ROM_LOAD( "ith_ic35.rom",0x300000, 0x100000, 0x3fca3073 )
	ROM_LOAD( "ith_ic36.rom",0x400000, 0x100000, 0x20d1b28b )
	ROM_LOAD( "ith_ic37.rom",0x500000, 0x100000, 0x90b6fd4b )

	ROM_REGION(0x100000)	/* Irem D8000011A1 */
	ROM_LOAD_V20_EVEN( "ith-sh0.rom",0x000000, 0x010000, 0x209c8b7f )
	ROM_LOAD_V20_ODD ( "ith-sl0.rom",0x000000, 0x010000, 0x18472d65 )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "ith_ic9.rom" ,0x000000, 0x080000, 0x318ee71a )
ROM_END

ROM_START( hook_rom )
	ROM_REGION(0x100000) /* Region 0 - v33 main cpu */
	ROM_LOAD_V20_EVEN( "h-h0-d.rom",0x000000, 0x040000, 0x40189ff6 )
	ROM_LOAD_V20_ODD ( "h-l0-d.rom",0x000000, 0x040000, 0x14567690 )
	ROM_LOAD_V20_EVEN( "h-h1.rom",  0x080000, 0x020000, 0x264ba1f0 )
	ROM_LOAD_V20_ODD ( "h-l1.rom",  0x080000, 0x020000, 0xf9913731 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "hook-c0.rom",0x000000, 0x040000, 0xdec63dcf )
	ROM_LOAD( "hook-c1.rom",0x080000, 0x040000, 0xe4eb0b92 )
	ROM_LOAD( "hook-c2.rom",0x100000, 0x040000, 0xa52b320b )
	ROM_LOAD( "hook-c3.rom",0x180000, 0x040000, 0x7ef67731 )

	ROM_LOAD( "hook-000.rom",0x200000, 0x100000, 0xccceac30 )
	ROM_LOAD( "hook-010.rom",0x300000, 0x100000, 0x8ac8da67 )
	ROM_LOAD( "hook-020.rom",0x400000, 0x100000, 0x8847af9a )
	ROM_LOAD( "hook-030.rom",0x500000, 0x100000, 0x239e877e )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "h-sh0.rom",0x000000, 0x010000, 0x86a4e56e )
	ROM_LOAD_V20_ODD ( "h-sl0.rom",0x000000, 0x010000, 0x10fd9676 )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "hook-da.rom" ,0x000000, 0x080000, 0x88cd0212 )
ROM_END

ROM_START( hooku_rom )
	ROM_REGION(0x100000) /* Region 0 - v33 main cpu */
	ROM_LOAD_V20_EVEN( "h0-c.3h",0x000000, 0x040000, 0x84cc239e )
	ROM_LOAD_V20_ODD ( "l0-c.5h",0x000000, 0x040000, 0x45e194fe )
	ROM_LOAD_V20_EVEN( "h-h1.rom",  0x080000, 0x020000, 0x264ba1f0 )
	ROM_LOAD_V20_ODD ( "h-l1.rom",  0x080000, 0x020000, 0xf9913731 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "hook-c0.rom",0x000000, 0x040000, 0xdec63dcf )
	ROM_LOAD( "hook-c1.rom",0x080000, 0x040000, 0xe4eb0b92 )
	ROM_LOAD( "hook-c2.rom",0x100000, 0x040000, 0xa52b320b )
	ROM_LOAD( "hook-c3.rom",0x180000, 0x040000, 0x7ef67731 )

	ROM_LOAD( "hook-000.rom",0x200000, 0x100000, 0xccceac30 )
	ROM_LOAD( "hook-010.rom",0x300000, 0x100000, 0x8ac8da67 )
	ROM_LOAD( "hook-020.rom",0x400000, 0x100000, 0x8847af9a )
	ROM_LOAD( "hook-030.rom",0x500000, 0x100000, 0x239e877e )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "h-sh0.rom",0x000000, 0x010000, 0x86a4e56e )
	ROM_LOAD_V20_ODD ( "h-sl0.rom",0x000000, 0x010000, 0x10fd9676 )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "hook-da.rom" ,0x000000, 0x080000, 0x88cd0212 )
ROM_END

ROM_START( rtypeleo_rom )
	ROM_REGION(0x100000) /* Region 0 - v33 main cpu */
	ROM_LOAD_V20_EVEN( "rtl-h0-d.bin", 0x000000, 0x040000, 0x3dbac89f )
	ROM_LOAD_V20_ODD ( "rtl-l0-d.bin", 0x000000, 0x040000, 0xf85a2537 )
	ROM_LOAD_V20_EVEN( "rtl-h1-d.bin", 0x080000, 0x020000, 0x352ff444 )
	ROM_LOAD_V20_ODD ( "rtl-l1-d.bin", 0x080000, 0x020000, 0xfd34ea46 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "rtl-c0.bin", 0x000000, 0x080000, 0xfb588d7c )
	ROM_LOAD( "rtl-c1.bin", 0x080000, 0x080000, 0xe5541bff )
	ROM_LOAD( "rtl-c2.bin", 0x100000, 0x080000, 0xfaa9ae27 )
	ROM_LOAD( "rtl-c3.bin", 0x180000, 0x080000, 0x3a2343f6 )

	ROM_LOAD( "rtl-000.bin",0x200000, 0x100000, 0x82a06870 )
	ROM_LOAD( "rtl-010.bin",0x300000, 0x100000, 0x417e7a56 )
	ROM_LOAD( "rtl-020.bin",0x400000, 0x100000, 0xf9a3f3a1 )
	ROM_LOAD( "rtl-030.bin",0x500000, 0x100000, 0x03528d95 )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "rtl-sh0a.bin",0x000000, 0x010000, 0xe518b4e3 )
	ROM_LOAD_V20_ODD ( "rtl-sl0a.bin",0x000000, 0x010000, 0x896f0d36 )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "rtl-da.bin" ,0x000000, 0x080000, 0xdbebd1ff )
ROM_END

ROM_START( uccops_rom )
	ROM_REGION(0x100000) /* Region 0 - v30 main cpu */
	ROM_LOAD_V20_EVEN( "uc_h0.rom",  0x000000, 0x040000, 0x240aa5f7 )
	ROM_LOAD_V20_ODD ( "uc_l0.rom",  0x000000, 0x040000, 0xdf9a4826 )
	ROM_LOAD_V20_EVEN( "uc_h1.rom",  0x080000, 0x020000, 0x8d29bcd6 )
	ROM_LOAD_V20_ODD ( "uc_l1.rom",  0x080000, 0x020000, 0xa8a402d8 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "uc_w38m.rom", 0x000000, 0x080000, 0x130a40e5 )
	ROM_LOAD( "uc-w39m.rom", 0x080000, 0x080000, 0xe42ca144 )
	ROM_LOAD( "uc-w40m.rom", 0x100000, 0x080000, 0xc2961648 )
	ROM_LOAD( "uc-w41m.rom", 0x180000, 0x080000, 0xf5334b80 )

	ROM_LOAD( "uc_k16m.rom", 0x200000, 0x100000, 0x4A225F09 )
	ROM_LOAD( "uc-k17m.rom", 0x300000, 0x100000, 0xE4ED9A54 )
	ROM_LOAD( "uc-k18m.rom", 0x400000, 0x100000, 0xA626EB12 )
	ROM_LOAD( "uc-k19m.rom", 0x500000, 0x100000, 0x5DF46549 )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU - encrypted V30 = NANAO custom D80001 (?) */
	ROM_LOAD_V20_EVEN( "uc_sh0.rom",0x000000, 0x010000, 0xDF90B198 )
	ROM_LOAD_V20_ODD ( "uc_sl0.rom",0x000000, 0x010000, 0x96C11AAC )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "uc_w42.rom" ,0x000000, 0x080000, 0xD17D3FD6 )
ROM_END

ROM_START( lethalth_rom )
	ROM_REGION(0x100000) /* Region 0 - v30 main cpu */
	ROM_LOAD_V20_EVEN( "lt_d-h0.rom",  0x000000, 0x020000, 0x20c68935 )
	ROM_LOAD_V20_ODD ( "lt_d-l0.rom",  0x000000, 0x020000, 0xe1432fb3 )
	ROM_LOAD_V20_EVEN( "lt_d-h1.rom",  0x040000, 0x020000, 0xd7dd3d48 )
	ROM_LOAD_V20_ODD ( "lt_d-l1.rom",  0x040000, 0x020000, 0xb94b3bd8 )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "lt_7a.rom", 0x000000, 0x040000, 0xada0fd50 )
	ROM_LOAD( "lt_7b.rom", 0x080000, 0x040000, 0xd2596883 )
	ROM_LOAD( "lt_7d.rom", 0x100000, 0x040000, 0x2de637ef )
	ROM_LOAD( "lt_7h.rom", 0x180000, 0x040000, 0x9f6585cd )

	ROM_LOAD( "lt_7j.rom", 0x200000, 0x040000, 0xbaf8863e )
	ROM_LOAD( "lt_7l.rom", 0x300000, 0x040000, 0x40fd50af )
	ROM_LOAD( "lt_7s.rom", 0x400000, 0x040000, 0xc8e970df )
	ROM_LOAD( "lt_7y.rom", 0x500000, 0x040000, 0xf5436708 )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU */
	ROM_LOAD_V20_EVEN( "lt_d-sh0.rom",0x000000, 0x010000, 0xaf5b224f )
	ROM_LOAD_V20_ODD ( "lt_d-sl0.rom",0x000000, 0x010000, 0xcb3faac3 )

	ROM_REGION(0x40000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "lt_8a.rom" ,0x000000, 0x040000, 0x357762a2 )
ROM_END

ROM_START( nbbatman_rom )
	ROM_REGION(0x180000) /* Region 0 - v30 main cpu */
	ROM_LOAD_V20_EVEN( "a1-h0-a.34",  0x000000, 0x040000, 0x24a9b794 )
	ROM_LOAD_V20_ODD ( "a1-l0-a.31",  0x000000, 0x040000, 0x846d7716 )
	ROM_LOAD_V20_EVEN( "a1-h1-.33",   0x100000, 0x040000, 0x3ce2aab5 )
	ROM_LOAD_V20_ODD ( "a1-l1-.32",   0x100000, 0x040000, 0x116d9bcc )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "lh534k0c.9",  0x000000, 0x080000, 0x314a0c6d )
	ROM_LOAD( "lh534k0e.10", 0x080000, 0x080000, 0xdc31675b )
	ROM_LOAD( "lh534k0f.11", 0x100000, 0x080000, 0xe15d8bfb )
	ROM_LOAD( "lh534k0g.12", 0x180000, 0x080000, 0x888d71a3 )

	ROM_LOAD( "lh538393.42", 0x200000, 0x100000, 0x26cdd224 )
	ROM_LOAD( "lh538394.43", 0x300000, 0x100000, 0x4bbe94fa )
	ROM_LOAD( "lh538395.44", 0x400000, 0x100000, 0x2a533b5e )
	ROM_LOAD( "lh538396.45", 0x500000, 0x100000, 0x863a66fa )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU */
	ROM_LOAD_V20_EVEN( "a1-sh0-.14",0x000000, 0x010000, 0xb7fae3e6 )
	ROM_LOAD_V20_ODD ( "a1-sl0-.17",0x000000, 0x010000, 0xb26d54fc )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "lh534k0k.8" ,0x000000, 0x080000, 0x735e6380 )
ROM_END

ROM_START( leaguemn_rom )
	ROM_REGION(0x180000) /* Region 0 - v30 main cpu */
	ROM_LOAD_V20_EVEN( "lma1-h0.34",  0x000000, 0x040000, 0x47c54204 )
	ROM_LOAD_V20_ODD ( "lma1-l0.31",  0x000000, 0x040000, 0x1d062c82 )
	ROM_LOAD_V20_EVEN( "a1-h1-.33",   0x100000, 0x040000, 0x3ce2aab5 )
	ROM_LOAD_V20_ODD ( "a1-l1-.32",   0x100000, 0x040000, 0x116d9bcc )

	ROM_REGION_DISPOSE(0x600000)	/* Region 1 - Graphics */
	ROM_LOAD( "lh534k0c.9",  0x000000, 0x080000, 0x314a0c6d )
	ROM_LOAD( "lh534k0e.10", 0x080000, 0x080000, 0xdc31675b )
	ROM_LOAD( "lh534k0f.11", 0x100000, 0x080000, 0xe15d8bfb )
	ROM_LOAD( "lh534k0g.12", 0x180000, 0x080000, 0x888d71a3 )

	ROM_LOAD( "lh538393.42", 0x200000, 0x100000, 0x26cdd224 )
	ROM_LOAD( "lh538394.43", 0x300000, 0x100000, 0x4bbe94fa )
	ROM_LOAD( "lh538395.44", 0x400000, 0x100000, 0x2a533b5e )
	ROM_LOAD( "lh538396.45", 0x500000, 0x100000, 0x863a66fa )

	ROM_REGION(0x100000)	/* 1MB for the audio CPU */
	ROM_LOAD_V20_EVEN( "a1-sh0-.14",0x000000, 0x010000, 0xb7fae3e6 )
	ROM_LOAD_V20_ODD ( "a1-sl0-.17",0x000000, 0x010000, 0xb26d54fc )

	ROM_REGION(0x80000)	 /* Region 3 ADPCM samples */
	ROM_LOAD( "lh534k0k.8" ,0x000000, 0x080000, 0x735e6380 )
ROM_END

/***************************************************************************/

static int lethalth_cycle_r(int offset)
{
	if (cpu_get_pc()==0x1f4 && (m92_ram[0x1e])==2 && offset==0)
		cpu_spinuntil_int();

//if (errorlog && cpu_get_pc()!=0x28913 ) fprintf(errorlog,"%04x: %04x\n",cpu_get_pc(),m92_ram[0x1e]);
	return m92_ram[0x1e + offset];
}

#if 0
static int hook_cycle_r(int offset)
{
	if (cpu_get_pc()==0x55ba && (m92_ram[0x12])==0 && (m92_ram[0x13])==0 && offset==0)
		cpu_spinuntil_int();

//if (errorlog && cpu_get_pc()!=0x28913 ) fprintf(errorlog,"%04x: %04x\n",cpu_get_pc(),m92_ram[0x1e]);
	return m92_ram[0x12 + offset];
}

static int rtypeleo_cycle_r(int offset)
{
	if (cpu_get_pc()==0x307a3 && m92_ram[0x32]==2)
		cpu_spinuntil_int();

//if (errorlog && cpu_get_pc()!=0x307a3 ) fprintf(errorlog,"%04x: %04x\n",cpu_get_pc(),m92_ram[0x32]);
	return m92_ram[0x32 + offset];
}
#endif

static void memory_hooks(void)
{
	if (!strcmp(Machine->gamedrv->name,"lethalth"))
		install_mem_read_handler(0, 0xe001e, 0xe001f, lethalth_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"hook"))
//		install_mem_read_handler(0, 0xe0012, 0xe0013, hook_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"rtypeleo"))
//		install_mem_read_handler(0, 0xe0032, 0xe0033, rtypeleo_cycle_r);
}

static void m92_startup(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	memcpy(RAM+0xffff0,RAM+0x7fff0,0x10); /* Start vector */
	cpu_setbank(1,&RAM[0xa0000]); /* Initial bank */

	/* Mirror used by In The Hunt for protection */
	memcpy(RAM+0xc0000,RAM+0x00000,0x10000);
	cpu_setbank(2,&RAM[0xc0000]);

	RAM = Machine->memory_region[2];
	memcpy(RAM+0xffff0,RAM+0x1fff0,0x10); /* Sound cpu Start vector */

	/* This must be to do with the IRQ controller.. */
	if (!strcmp(Machine->gamedrv->name,"rtypeleo")
		|| !strcmp(Machine->gamedrv->name,"lethalth"))
		m92_irq_vectorbase=0x20;
	else
		m92_irq_vectorbase=0x80;

	/* This game sets the raster IRQ position, but the interrupt routine
		is just an iret, no need to emulate it */
	if (!strcmp(Machine->gamedrv->name,"lethalth"))
		raster_enable=0;
	else
		raster_enable=1;

	/* These games seem to have a different sprite chip */
	if (!strcmp(Machine->gamedrv->name,"rtypeleo")
		|| !strcmp(Machine->gamedrv->name,"lethalth")
		|| !strcmp(Machine->gamedrv->name,"uccops"))
		m92_spritechip=1;
	else
		m92_spritechip=0;
}

static void m92_sound_decrypt(void)
{
	m92_startup();
	/* Not decrypted yet */
}

/***************************************************************************/

#define M92DRIVER(M92_NAME,M92_REALNAME,M92_YEAR,M92_MANU,M92_PORTS,M92_CLONE,M92_ROTATION) \
struct GameDriver M92_NAME##_driver  = \
{                             		  \
	__FILE__,                		   \
	M92_CLONE,               			\
	#M92_NAME,               		   \
	M92_REALNAME,            		   \
	M92_YEAR,                		   \
	M92_MANU,                   		\
	"Bryan McPhail",          			\
	GAME_NO_SOUND, 			       		\
	&machine_driver,                	\
	memory_hooks,						\
	M92_NAME##_rom,             		\
	m92_sound_decrypt, 0,     			\
	0,                          		\
	0, 	    	                		\
	M92_PORTS,             				\
	0, 0, 0,                    		\
	M92_ROTATION,        				\
	0,0  								\
};

M92DRIVER(bmaster ,"Blade Master","1991","Irem",bmaster_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(gunforce,"Gunforce - Battle Fire Engulfed Terror Island (World)","1991","Irem",gunforce_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(gunforcu,"Gunforce - Battle Fire Engulfed Terror Island (US)","1991","Irem America",gunforce_input_ports,&gunforce_driver,ORIENTATION_DEFAULT)
M92DRIVER(hook,    "Hook (World)","1992","Irem",hook_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(hooku,   "Hook (US)","1992","Irem America",hook_input_ports,&hook_driver,ORIENTATION_DEFAULT)
M92DRIVER(uccops  ,"Undercover Cops","1992","Irem",uccops_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(rtypeleo,"R-Type Leo (Japan)","1992","Irem",rtypeleo_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(majtitl2,"Major Title 2 (World)","1992","Irem",skingame_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(skingame,"The Irem Skins Game (US set 1)","1992","Irem America",skingame_input_ports,&majtitl2_driver,ORIENTATION_DEFAULT)
M92DRIVER(skingam2,"The Irem Skins Game (US set 2)","1992","Irem America",skingame_input_ports,&majtitl2_driver,ORIENTATION_DEFAULT)
M92DRIVER(inthunt ,"In The Hunt","1993","Irem",inthunt_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(kaiteids,"Kaitei Daisensou (Japan)","1993","Irem",inthunt_input_ports,&inthunt_driver,ORIENTATION_DEFAULT)

/* Not working yet */
M92DRIVER(nbbatman,"Ninja Baseball Batman","1993","Irem",nbbatman_input_ports,0,ORIENTATION_DEFAULT)
M92DRIVER(leaguemn,"Yakyuu Kakutou League-Man (Japan)","1993","Irem",nbbatman_input_ports,&nbbatman_driver,ORIENTATION_DEFAULT)

/* Different machine driver for this one, it uses a different memory map */
struct GameDriver lethalth_driver  =
{
	__FILE__,
	0,
	"lethalth",
	"Lethal Thunder",
	"1991",
	"Irem",
	"Bryan McPhail",
	GAME_NO_SOUND,
	&lethalth_machine_driver,
	0,
	lethalth_rom,
	m92_sound_decrypt, 0,
	0,
	0,
	lethalth_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0,0
};


/*

Irem Custom V33 CPU:

Gunforce						Nanao 	08J27261A 011 9106KK701
Quiz F-1 1,2 Finish          	Nanao	08J27291A4 014 9147KK700
Skins Game						Nanao 	08J27291A7
R-Type Leo						Irem 	D800001A1
In The Hunt						Irem 	D8000011A1
Risky Challenge/Gussun Oyoyo 			D8000019A1
Shisensho II                 			D8000020A1 023 9320NK700

Nanao Custom parts:

GA20	Sample player
GA21	Tilemaps } Used in combination
GA22	Tilemaps }
GA23	Sprites (Gun Force)
GA30 	Tilemaps
GA31	Tilemaps
GA32	Sprites (Fire Barrel)

*/
