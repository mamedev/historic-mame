/***************************************************************************

	SNK/Alpha 68000 based games:

	(Game)					(PCB Number)     (Manufacturer)

	The Koukouyakyuh        ?                Alpha Denshi 1985
	Super Stingray			? (Early)        Alpha 1986?
	Kyros					? (Early)        World Games Inc 1987
	Paddle Mania			Alpha 68K-96 I   SNK 1988
	Time Soldiers (Ver 3)	Alpha 68K-96 II  SNK/Romstar 1987
	Time Soldiers (Ver 1)	Alpha 68K-96 II  SNK/Romstar 1987
	Battlefield (Ver 1)		Alpha 68K-96 II  SNK 1987
	Sky Soldiers            Alpha 68K-96 II  SNK/Romstar 1988
	Gold Medalist		    Alpha 68K-96 II  SNK 1988
	Gold Medalist           (Bootleg)        SNK 1988
	Sky Adventure			Alpha 68K-96 V   SNK 1989
	Gang Wars				Alpha 68K-96 V   Alpha 1989
	Gang Wars				(Bootleg)        Alpha 1989
	Super Champion Baseball (V board?)       SNK/Alpha/Romstar/Sega 1989

General notes:

	The Koukouyakyuh is different hardware design from the other games.

	All II & V games are 68000, z80 plus a custom Alpha microcontroller,
	the microcontroller is able to write to anywhere within main memory.

	Gold Medalist (bootleg) has a 68705 in place of the Alpha controller.

	V boards have more memory and double the amount of colours as II boards.

	Coinage is controlled by the microcontroller and is not fully supported
	for all games yet.

	Time Soldiers - make the ROM writable and the game will enter a 'debug'
	kind of mode, probably from the development system used.

	Time Soldiers - Title screen is corrupt when set to 'Japanese language',
	the real board does this too!  (Battlefield is corrupt when set to English
	too).

	Paddle Mania is (c) 1988 but is crude hardware and there are probably
	several earlier games running on it.

	Emulation by Bryan McPhail, mish@tendril.co.uk

Revision History:

Acho A. Tang, 3-16-2002

[Super Stingray]
- improved color, added sound, game timer and coin handler, corrected DIP settings
note: CLUT and color assignment PROMs missing

[Kyros]
- fixed color, added sound and coin handler, corrected DIP settings

[Paddle Mania]
- reactivated driver
- improved color, added sound, fixed control and priority, corrected DIP settings

[Time Soldiers]
- fixed priority

[Gold Medalist]
- fixed gameplay, control and priority, corrected DIP settings
note: bad sound ROM

[Sky Adventure]
- fixed sprite position and priority

[Gang Wars]
- fixed color in the 2nd last graphics bank

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

VIDEO_START( kouyakyu );
VIDEO_START( alpha68k );
VIDEO_UPDATE( kyros );
VIDEO_UPDATE( sstingry );
VIDEO_UPDATE( alpha68k_I );
VIDEO_UPDATE( kouyakyu );
PALETTE_INIT( kyros );
PALETTE_INIT( paddlem );
VIDEO_UPDATE( alpha68k_II );
WRITE16_HANDLER( alpha68k_II_video_bank_w );
VIDEO_UPDATE( alpha68k_V );
VIDEO_UPDATE( alpha68k_V_sb );
void alpha68k_V_video_bank_w(int bank);
void alpha68k_flipscreen_w(int flip);
WRITE16_HANDLER( alpha68k_V_video_control_w );
WRITE16_HANDLER( alpha68k_paletteram_w );
WRITE16_HANDLER( alpha68k_videoram_w );
WRITE16_HANDLER( kouyakyu_video_w );

static data16_t *shared_ram;
//static unsigned char *sound_ram; //AT: not needed
static int invert_controls,microcontroller_id,coin_id;

/******************************************************************************/

static READ16_HANDLER( control_1_r )
{
	if (invert_controls)
		return ~(readinputport(0) + (readinputport(1) << 8));

	return (readinputport(0) + (readinputport(1) << 8));
}

static READ16_HANDLER( control_2_r )
{
	if (invert_controls)
		return ~(readinputport(3) + ((~(1 << (readinputport(5) * 12 / 256))) << 8));

	return readinputport(3) + /* Low byte of CN1 */
    	((~(1 << (readinputport(5) * 12 / 256))) << 8);
}

static READ16_HANDLER( control_2_V_r )
{
	return readinputport(3);
}

static READ16_HANDLER( kyros_dip_r )
{
	return readinputport(1)<<8;
}

static READ16_HANDLER( control_3_r )
{
	if (invert_controls)
		return ~((( ~(1 << (readinputport(6) * 12 / 256)) )<<8)&0xff00);

	return (( ~(1 << (readinputport(6) * 12 / 256)) )<<8)&0xff00;
}

/* High 4 bits of CN1 & CN2 */
static READ16_HANDLER( control_4_r )
{
	if (invert_controls)
		return ~(((( ~(1 << (readinputport(6) * 12 / 256))  ) <<4)&0xf000)
    	 + ((( ~(1 << (readinputport(5) * 12 / 256))  )    )&0x0f00));

	return ((( ~(1 << (readinputport(6) * 12 / 256))  ) <<4)&0xf000)
    	 + ((( ~(1 << (readinputport(5) * 12 / 256))  )    )&0x0f00);
}

/******************************************************************************/

static WRITE16_HANDLER( kyros_sound_w )
{
	if(ACCESSING_MSB)
		soundlatch_w(0, (data>>8)&0xff);
}

static WRITE16_HANDLER( alpha68k_II_sound_w )
{
	if(ACCESSING_LSB)
		soundlatch_w(0, data&0xff);
}

static WRITE16_HANDLER( alpha68k_V_sound_w )
{
	/* Sound & fix bank select are in the same word */
	if(ACCESSING_LSB)
		soundlatch_w(0,data&0xff);
	if(ACCESSING_MSB)
		alpha68k_V_video_bank_w((data>>8)&0xff);
}
//AT
static WRITE16_HANDLER( paddlema_soundlatch_w )
{
        if (ACCESSING_LSB)
        {
                soundlatch_w(0, data);
                cpu_set_irq_line(1, 0, HOLD_LINE);
        }
}
//ZT
/******************************************************************************/

/* Time Soldiers, Sky Soldiers, Gold Medalist */
static READ16_HANDLER( alpha_II_trigger_r )
{
	static int latch;
	int source=shared_ram[offset];

	switch (offset) {
		case 0:	/* Dipswitch 2 */
			shared_ram[0] = (source&0xff00)| readinputport(4);
			return 0;

		case 0x22: /* Coin value */
			shared_ram[0x22] = (source&0xff00)|0x1;
			return 0;

		case 0x29: /* Query microcontroller for coin insert */
                if (microcontroller_id != 0x8803) { //AT
			if ((readinputport(2)&0x3)==3) latch=0;
			if ((readinputport(2)&0x1)==0 && !latch) {
				shared_ram[0x29] = (source&0xff00)|0x22;
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;
			} else if ((readinputport(2)&0x2)==0 && !latch) {
				shared_ram[0x29] = (source&0xff00)|0x22;
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;
			}
			else
				shared_ram[0x29] = (source&0xff00)|0x00;
			return 0;
//AT: unidentified jump code: 0x5b
                }
                else
                {
                        if ((readinputport(2)&0x3)==3) latch=0;
                        if ((readinputport(2)&0x1)==0 && !latch) {
                                shared_ram[0x29] = (source&0xff00)|0x23; // coinA
                                latch=1;
                        } else if ((readinputport(2)&0x2)==0 && !latch) {
                                shared_ram[0x29] = (source&0xff00)|0x24; // coinB
                                latch=1;
                        }
                        else
                                shared_ram[0x29] = (source&0xff00)|0x21; // timer
                        return 0;
                }
//ZT
		case 0xfe:	/* Custom ID check, same for all games */
			shared_ram[0xfe] = (source&0xff00)|0x87;
			break;
		case 0xff:	/* Custom ID check, same for all games */
			shared_ram[0xff] = (source&0xff00)|0x13;
			break;
	}

	logerror("%04x:  Alpha read trigger at %04x\n",activecpu_get_pc(),offset);

	return 0; /* Values returned don't matter */
}

/* Sky Adventure & Gang Wars */
static READ16_HANDLER( alpha_V_trigger_r )
{
	static int latch;
	int source=shared_ram[offset];

	switch (offset) {
		case 0:	/* Dipswitch 1 */
			shared_ram[0] = (source&0xff00)| readinputport(4);
			return 0;
		case 0x22: /* Coin value */
			shared_ram[0x22] = (source&0xff00)|0x1;
			return 0;
		case 0x29: /* Query microcontroller for coin insert */
			if ((readinputport(2)&0x3)==3) latch=0;
			if ((readinputport(2)&0x1)==0 && !latch) {
				shared_ram[0x29] = (source&0xff00)|(coin_id&0xff);
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;
			}
			else if ((readinputport(2)&0x2)==0 && !latch) {
				shared_ram[0x29] = (source&0xff00)|(coin_id>>8);
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;
			}
			else
				shared_ram[0x29] = (source&0xff00)|0x00;
			return 0;
		case 0xfe:	/* Custom ID check */
			shared_ram[0xfe] = (source&0xff00)|(microcontroller_id>>8);
			break;
		case 0xff:	/* Custom ID check */
			shared_ram[0xff] = (source&0xff00)|(microcontroller_id&0xff);
			break;

		case 0x1f00: /* Dipswitch 1 */
			shared_ram[0x1f00] = (source&0xff00)| readinputport(4);
			return 0;
		case 0x1f29: /* Gang Wars - Query microcontroller for coin insert */
			if ((readinputport(2)&0x3)==3) latch=0;
			if ((readinputport(2)&0x1)==0 && !latch) {
				shared_ram[0x1f29] = (source&0xff00)|0x23;
				shared_ram[0x1f22] = (source&0xff00)|0x0;
				latch=1;
			}
			else if ((readinputport(2)&0x2)==0 && !latch) {
				shared_ram[0x1f29] = (source&0xff00)|0x24;
				shared_ram[0x1f22] = (source&0xff00)|0x0;
				latch=1;
			}
			else
				shared_ram[0x1f29] = (source&0xff00)|0x00;

			/* The game expects the first dip to appear in RAM at 0x2c6, presumably
				the microcontroller supplies it (it does for all the other games, but
				usually to 0x0 in RAM) */
			source=shared_ram[0x163];
			shared_ram[0x163] = (source&0x00ff)| (readinputport(4)<<8);
			return 0;

		case 0x1ffe: /* Custom ID check, Gang Wars */
			shared_ram[0x1ffe] = (source&0xff00)|(microcontroller_id>>8);
			break;
		case 0x1fff: /* Custom ID check, Gang Wars */
			shared_ram[0x1fff] = (source&0xff00)|(microcontroller_id&0xff);
			break;
	}

	logerror("%04x:  Alpha read trigger at %04x\n",activecpu_get_pc(),offset);

	return 0; /* Values returned don't matter */
}

static WRITE16_HANDLER( alpha_microcontroller_w )
{
	logerror("%04x:  Alpha write trigger at %04x (%04x)\n",activecpu_get_pc(),offset,data);
	/* 0x44 = coin clear signal to microcontroller? */
	if (offset==0x2d && ACCESSING_LSB)
		alpha68k_flipscreen_w(data & 1);
}

static READ16_HANDLER( kyros_alpha_trigger_r )
{
//AT: possible jump codes: 0x21,0x22,0x23,0x24,0x34,0x37,0x3a,0x3d,0x40,0x43,0x46,0x49,0x00
        static unsigned coinvalue, trigstate=0, deposits=0, credits=0;
        static UINT8 coinage[8][2]={{1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{2,3},{3,2}};
//ZT
	static int latch;
	int source=shared_ram[offset];

	switch (offset) {
		case 0x22: /* Coin value */
			//shared_ram[0x22] = (source&0xff00)|0x1;
                        shared_ram[0x22] = (source&0xff00)|(credits&0x00ff); //AT
			return 0;
		case 0x29: /* Query microcontroller for coin insert */
			if ((readinputport(2)&0x1)==1) latch=0;
			if ((readinputport(2)&0x1)==0 && !latch) {
				shared_ram[0x29] = (source&0xff00)|0x22;
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;
//AT
                                coinvalue = readinputport(1)>>1 & 7;
                                if (++deposits==coinage[coinvalue][0]) {
                                        credits = coinage[coinvalue][1];
                                        deposits = 0;
                                } else
                                        credits = 0;
                        }
                        else if (++trigstate>=12) { // sets jump cycle to 12(arbitrary)
                                shared_ram[0x29] = (source&0xff00)|0x21;
                                trigstate = 0;
//ZT
			}
			else
				shared_ram[0x29] = (source&0xff00)|0x00;
			return 0;

		case 0xff:	/* Custom check, only used at bootup */
			shared_ram[0xff] = (source&0xff00)|microcontroller_id;
			break;

	}

	logerror("%04x:  Alpha read trigger at %04x\n",activecpu_get_pc(),offset);

	return 0; /* Values returned don't matter */
}

/******************************************************************************/

static MEMORY_READ16_START( kouyakyu_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x040fff, MRA16_RAM },
	{ 0x080000, 0x081fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, MRA16_RAM },
	{ 0x100000, 0x1007ff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( kouyakyu_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x040000, 0x040fff, MWA16_RAM, &shared_ram },
	{ 0x080000, 0x080fff, kouyakyu_video_w, &videoram16 },
	{ 0x0c0000, 0x0c0fff, MWA16_RAM, &spriteram16 },
	{ 0x100000, 0x1007ff, MWA16_RAM },
	{ 0x140000, 0x1407ff, MWA16_RAM },
	{ 0x780000, 0x780001, MWA16_NOP }, /* Watchdog? */
MEMORY_END
//AT
#if 0
static MEMORY_READ16_START( kyros_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x020000, 0x020fff, MRA16_RAM },
	{ 0x040000, 0x041fff, MRA16_RAM },
	{ 0x060000, 0x060001, MRA16_NOP }, /* Watchdog? */
	{ 0x080000, 0x0801ff, kyros_alpha_trigger_r },
	{ 0x0c0000, 0x0c0001, input_port_0_word_r },
	{ 0x0e0000, 0x0e0001, kyros_dip_r },
MEMORY_END

static MEMORY_WRITE16_START( kyros_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x020000, 0x020fff, MWA16_RAM, &shared_ram },
	{ 0x040000, 0x041fff, MWA16_RAM, &spriteram16 },
	{ 0x060000, 0x060001, alpha68k_II_sound_w }, /* Watchdog? */
	{ 0x0e0000, 0x0e0001, kyros_sound_w },
MEMORY_END

static MEMORY_READ16_START( alpha68k_I_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
//	{ 0x180008, 0x180009, control_2_r }, /* 1 byte */
	{ 0x300000, 0x300001, control_1_r }, /* 2  */
	{ 0x340000, 0x340001, control_1_r }, /* 2  */
	{ 0x100000, 0x103fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( alpha68k_I_writemem )
	{ 0x000000, 0x03ffff, MWA16_NOP },
	{ 0x080000, 0x083fff, MWA16_RAM, &shared_ram },
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16 },
	{ 0x180000, 0x180001, MWA16_NOP }, /* Watchdog */
	{ 0x380000, 0x380001, alpha68k_II_sound_w },
MEMORY_END
#endif
static MEMORY_READ16_START( kyros_readmem )
        { 0x000000, 0x01ffff, MRA16_ROM }, // main program
        { 0x020000, 0x020fff, MRA16_RAM }, // work RAM
        { 0x040000, 0x041fff, MRA16_RAM }, // sprite RAM
        { 0x060000, 0x060001, MRA16_RAM }, // MSB: watchdog
        { 0x080000, 0x0801ff, kyros_alpha_trigger_r },
        { 0x0c0000, 0x0c0001, input_port_0_word_r },
        { 0x0e0000, 0x0e0001, kyros_dip_r },
MEMORY_END

static MEMORY_WRITE16_START( kyros_writemem )
        { 0x000000, 0x01ffff, MWA16_ROM },
        { 0x020000, 0x020fff, MWA16_RAM, &shared_ram },
        { 0x040000, 0x041fff, MWA16_RAM, &spriteram16 },
        { 0x060000, 0x060001, MWA16_RAM, &videoram16 }, // LSB: BGC
        { 0x080000, 0x0801ff, MWA16_NOP },
        { 0x0e0000, 0x0e0001, kyros_sound_w },
MEMORY_END

static MEMORY_READ16_START( alpha68k_I_readmem )
        { 0x000000, 0x03ffff, MRA16_ROM }, // main program
        { 0x080000, 0x083fff, MRA16_RAM }, // work RAM
        { 0x100000, 0x103fff, MRA16_RAM }, // video RAM
        { 0x180000, 0x180001, input_port_3_word_r }, // LSB: DSW0
        { 0x180008, 0x180009, input_port_4_word_r }, // LSB: DSW1
        { 0x300000, 0x300001, input_port_0_word_r }, // joy1, joy2
        { 0x340000, 0x340001, input_port_1_word_r }, // coin, start, service
        { 0x380000, 0x380001, input_port_2_word_r }, // joy3, joy4
MEMORY_END

static MEMORY_WRITE16_START( alpha68k_I_writemem )
        { 0x000000, 0x03ffff, MWA16_NOP },
        { 0x080000, 0x083fff, MWA16_RAM },
        { 0x100000, 0x103fff, MWA16_RAM, &spriteram16 },
        { 0x180000, 0x180001, MWA16_NOP }, // MSB: watchdog(?)
        { 0x380000, 0x380001, paddlema_soundlatch_w }, // LSB: sound latch write and RST38 trigger
MEMORY_END
//ZT
static MEMORY_READ16_START( alpha68k_II_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x040fff, MRA16_RAM },
	{ 0x080000, 0x080001, control_1_r }, /* Joysticks */
	{ 0x0c0000, 0x0c0001, control_2_r }, /* CN1 & Dip 1 */
	{ 0x0c8000, 0x0c8001, control_3_r }, /* Bottom of CN2 */
	{ 0x0d0000, 0x0d0001, control_4_r }, /* Top of CN1 & CN2 */
	{ 0x0d8000, 0x0d8001, MRA16_NOP }, /* IRQ ack? */
	{ 0x0e0000, 0x0e0001, MRA16_NOP }, /* IRQ ack? */
	{ 0x0e8000, 0x0e8001, MRA16_NOP }, /* watchdog? */
	{ 0x100000, 0x100fff, MRA16_RAM },
	{ 0x200000, 0x207fff, MRA16_RAM },
	{ 0x300000, 0x3001ff, alpha_II_trigger_r },
	{ 0x400000, 0x400fff, MRA16_RAM },
	{ 0x800000, 0x83ffff, MRA16_BANK8 }, /* Extra code bank */
MEMORY_END

static MEMORY_WRITE16_START( alpha68k_II_writemem )
	{ 0x000000, 0x03ffff, MWA16_NOP },
	{ 0x040000, 0x040fff, MWA16_RAM, &shared_ram },
	{ 0x080000, 0x080001, alpha68k_II_sound_w },
	{ 0x0c0000, 0x0c00ff, alpha68k_II_video_bank_w },
	{ 0x100000, 0x100fff, alpha68k_videoram_w, &videoram16 },
	{ 0x200000, 0x207fff, MWA16_RAM, &spriteram16 },
	{ 0x300000, 0x3001ff, alpha_microcontroller_w },
	{ 0x400000, 0x400fff, alpha68k_paletteram_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( alpha68k_V_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, MRA16_RAM },
	{ 0x080000, 0x080001, control_1_r }, /* Joysticks */
	{ 0x0c0000, 0x0c0001, control_2_V_r }, /* Dip 2 */
	{ 0x0d8000, 0x0d8001, MRA16_NOP }, /* IRQ ack? */
	{ 0x0e0000, 0x0e0001, MRA16_NOP }, /* IRQ ack? */
	{ 0x0e8000, 0x0e8001, MRA16_NOP }, /* watchdog? */
	{ 0x100000, 0x100fff, MRA16_RAM },
	{ 0x200000, 0x207fff, MRA16_RAM },
	{ 0x300000, 0x303fff, alpha_V_trigger_r },
	{ 0x400000, 0x401fff, MRA16_RAM },
	{ 0x800000, 0x83ffff, MRA16_BANK8 },
MEMORY_END

static MEMORY_WRITE16_START( alpha68k_V_writemem )
	{ 0x000000, 0x03ffff, MWA16_NOP },
	{ 0x040000, 0x043fff, MWA16_RAM, &shared_ram },
	{ 0x080000, 0x080001, alpha68k_V_sound_w },
	{ 0x0c0000, 0x0c00ff, alpha68k_V_video_control_w },
	{ 0x100000, 0x100fff, alpha68k_videoram_w, &videoram16 },
	{ 0x200000, 0x207fff, MWA16_RAM, &spriteram16 },
	{ 0x300000, 0x3001ff, alpha_microcontroller_w },
	{ 0x303e00, 0x303fff, alpha_microcontroller_w }, /* Gang Wars mirror */
	{ 0x400000, 0x401fff, alpha68k_paletteram_w, &paletteram16 },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( sound_bank_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);

	bankaddress = 0x10000 + (data) * 0x4000;
	cpu_setbank(7,&RAM[bankaddress]);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xffff, MRA_BANK7 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END
//AT
/*
static MEMORY_READ_START( kyros_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
//	{ 0xe000, 0xe000, soundlatch_r },
//	{ 0xc000, 0xffff, MRA_BANK7 },
MEMORY_END

static MEMORY_WRITE_START( kyros_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( sstingry_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
//	{ 0xe000, 0xe000, soundlatch_r },
//	{ 0xc000, 0xffff, MRA_BANK7 },
MEMORY_END

static WRITE_HANDLER (soundram_mirror_w)
{
	sound_ram[offset]=data;
}

static MEMORY_WRITE_START( sstingry_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM, &sound_ram },
//	{ 0xc000, 0xc3ff, soundram_mirror_w },
MEMORY_END
*/
static MEMORY_READ_START( kyros_sound_readmem )
        { 0x0000, 0xbfff, MRA_ROM },
        { 0xc000, 0xc7ff, MRA_RAM },
        { 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( kyros_sound_writemem )
        { 0x0000, 0xbfff, MWA_ROM },
        { 0xc000, 0xc7ff, MWA_RAM },
        { 0xe002, 0xe002, soundlatch_clear_w },
        { 0xe004, 0xe004, DAC_0_signed_data_w },
        { 0xe006, 0xe00e, MWA_NOP }, // soundboard I/O's, ignored
/* for reference only
        { 0xe006, 0xe006, MWA_NOP }, // NMI: diminishing saw-tooth
        { 0xe008, 0xe008, MWA_NOP }, // NMI: 00
        { 0xe00a, 0xe00a, MWA_NOP }, // RST38: 20
        { 0xe00c, 0xe00c, MWA_NOP }, // RST30: 00 on entry
        { 0xe00e, 0xe00e, MWA_NOP }, // RST30: 00,02,ff on exit(0x1d88)
*/
MEMORY_END

static MEMORY_READ_START( sstingry_sound_readmem )
        { 0x0000, 0x7fff, MRA_ROM },
        { 0x8000, 0x87ff, MRA_RAM },
        { 0xc100, 0xc100, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sstingry_sound_writemem )
        { 0x0000, 0x7fff, MWA_ROM },
        { 0x8000, 0x87ff, MWA_RAM },
        { 0xc102, 0xc102, soundlatch_clear_w },
        { 0xc104, 0xc104, DAC_0_signed_data_w },
        { 0xc106, 0xc10e, MWA_NOP }, // soundboard I/O's, ignored
MEMORY_END

static MEMORY_READ_START( alpha68k_I_s_readmem )
        { 0x0000, 0x9fff, MRA_ROM }, // sound program
        { 0xe000, 0xe000, soundlatch_r },
        { 0xe800, 0xe800, YM3812_status_port_0_r },
        { 0xf000, 0xf7ff, MRA_RAM }, // work RAM
        { 0xfc00, 0xfc00, MRA_RAM }, // ???
MEMORY_END

static MEMORY_WRITE_START( alpha68k_I_s_writemem )
        { 0x0000, 0x9fff, MWA_ROM },
        { 0xe000, 0xe000, soundlatch_clear_w },
        { 0xe800, 0xe800, YM3812_control_port_0_w },
        { 0xec00, 0xec00, YM3812_write_port_0_w },
        { 0xf000, 0xf7ff, MWA_RAM },
        { 0xfc00, 0xfc00, MWA_RAM }, // ???
MEMORY_END
//ZT
static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x08, 0x08, DAC_0_signed_data_w },
	{ 0x0a, 0x0a, YM2413_register_port_0_w },
	{ 0x0b, 0x0b, YM2413_data_port_0_w },
	{ 0x0c, 0x0c, YM2203_control_port_0_w },
	{ 0x0d, 0x0d, YM2203_write_port_0_w },
	{ 0x0e, 0x0e, sound_bank_w },
PORT_END

static PORT_WRITE_START( kyros_sound_writeport )
//	{ 0x00, 0x00, soundlatch_clear_w },
//	{ 0x08, 0x08, DAC_0_data_w },
	{ 0x10, 0x10, YM2203_control_port_0_w },
	{ 0x11, 0x11, YM2203_write_port_0_w },
//AT
/*
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x81, 0x81, AY8910_control_port_0_w },
	{ 0x90, 0x90, AY8910_write_port_1_w },
	{ 0x91, 0x91, AY8910_control_port_1_w },
*/
        { 0x80, 0x80, YM2203_write_port_1_w },
        { 0x81, 0x81, YM2203_control_port_1_w },
        { 0x90, 0x90, YM2203_write_port_2_w },
        { 0x91, 0x91, YM2203_control_port_2_w },
//ZT
PORT_END

/******************************************************************************/

#define ALPHA68K_PLAYER1_INPUT \
	PORT_START	/* Player 1 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 3 */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) \

#define ALPHA68K_PLAYER2_INPUT \
	PORT_START	/* Player 2 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 3 */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

INPUT_PORTS_START( kyros )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT| IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START	/* dipswitches */
//AT
/*
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) ) // demo sound?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
*/
        PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
        PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x00, DEF_STR( On ) )
        PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
        PORT_DIPSETTING(    0x00, "A 1C/1P B 1C/1P" )
        PORT_DIPSETTING(    0x02, "A 1C/2P B 2C/1P" )
        PORT_DIPSETTING(    0x04, "A 1C/3P B 3C/1P" )
        PORT_DIPSETTING(    0x06, "A 1C/4P B 4C/1P" )
        PORT_DIPSETTING(    0x08, "A 1C/5P B 5C/1P" )
        PORT_DIPSETTING(    0x0a, "A 1C/6P B 6C/1P" )
        PORT_DIPSETTING(    0x0c, "A 2C/3P B 7C/1P" )
        PORT_DIPSETTING(    0x0e, "A 3C/2P B 8C/1P" )
        PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x10, "4" )
        PORT_DIPSETTING(    0x20, "5" )
        PORT_DIPSETTING(    0x30, "6" )
        PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPSETTING(    0x40, "Hard" )
        PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
        PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
//ZT
	PORT_START /* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END

INPUT_PORTS_START( sstingry )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT| IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
//AT
/*
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
*/
        PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x01, DEF_STR( On ) )
        PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
        PORT_DIPSETTING(    0x00, "A 1C/1P B 1C/1P" )
        PORT_DIPSETTING(    0x02, "A 1C/2P B 2C/1P" )
        PORT_DIPSETTING(    0x04, "A 1C/3P B 3C/1P" )
        PORT_DIPSETTING(    0x06, "A 1C/4P B 4C/1P" )
        PORT_DIPSETTING(    0x08, "A 1C/5P B 5C/1P" )
        PORT_DIPSETTING(    0x0a, "A 1C/6P B 6C/1P" )
        PORT_DIPSETTING(    0x0c, "A 2C/3P B 7C/1P" )
        PORT_DIPSETTING(    0x0e, "A 3C/2P B 8C/1P" )
        PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x10, "4" )
        PORT_DIPSETTING(    0x20, "5" )
        PORT_DIPSETTING(    0x30, "6" )
        PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
        PORT_DIPSETTING(    0x40, "Easy" )
        PORT_DIPSETTING(    0x00, "Hard" )
        PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
        PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
//ZT
	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END
//AT
INPUT_PORTS_START( paddlema )
        PORT_START // control port 0
        PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY )
        PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
        PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
        PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
        PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
        PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 )
        PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

        PORT_START // control port 1
        PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
        PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
        PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
        PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
        PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE2 )
        PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE3 )
        PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE4 )

        PORT_START // control port 2
        PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
        PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
        PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
        PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
        PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_PLAYER4 )
        PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
        PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
        PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
        PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
        PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
        PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
        PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

        PORT_START // DSW0
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, "1/1C" )
	PORT_DIPSETTING(    0x08, "2/1C" )
	PORT_DIPSETTING(    0x04, "3/1C" )
	PORT_DIPSETTING(    0x0c, "4/1C" )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, "1/2C" )
	PORT_DIPSETTING(    0x01, "1/3C" )
	PORT_DIPSETTING(    0x02, "1/4C" )
	PORT_DIPSETTING(    0x00, "1/6C" )
        PORT_DIPNAME( 0x30, 0x30, "Game Time" )
        PORT_DIPSETTING(    0x00, "1:00" )
        PORT_DIPSETTING(    0x20, "1:10" )
        PORT_DIPSETTING(    0x10, "1:20" )
        PORT_DIPSETTING(    0x30, "1:30" )
        PORT_DIPNAME( 0xc0, 0x40, "Match Type" )
        PORT_DIPSETTING(    0x80, "A to B" )
        PORT_DIPSETTING(    0x00, "A to C" )
        PORT_DIPSETTING(    0x40, "A to E" )
        //PORT_DIPSETTING(    0xc0, "A to B" )

        PORT_START // DSW1
        PORT_DIPNAME( 0x01, 0x00, DEF_STR( Service_Mode ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x01, DEF_STR( On ) )
        PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x02, DEF_STR( On ) )
        PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x04, DEF_STR( On ) )
        PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x08, DEF_STR( On ) )
        PORT_DIPNAME( 0x30, 0x00, DEF_STR( Demo_Sounds ) )
        PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x00, DEF_STR( On ) )
        PORT_DIPSETTING(    0x30, "Freeze" )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x20, DEF_STR( On ) )
        PORT_DIPNAME( 0x40, 0x00, "Language" )
        PORT_DIPSETTING(    0x00, "English" )
        PORT_DIPSETTING(    0x40, "Japanese" )
        PORT_DIPNAME( 0x80, 0x00, "Continue" )
        PORT_DIPSETTING(    0x80, DEF_STR( No ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END
//ZT
INPUT_PORTS_START( timesold )
	ALPHA68K_PLAYER1_INPUT
	ALPHA68K_PLAYER2_INPUT

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* 2 physical sets of _6_ dip switches */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x01, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x07, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x06, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x05, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x03, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 8, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 8, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( btlfield )
	ALPHA68K_PLAYER1_INPUT
	ALPHA68K_PLAYER2_INPUT

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* 2 physical sets of _6_ dip switches */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPNAME( 0x20, 0x20, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x01, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x07, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x06, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x05, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x03, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 8, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 8, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( skysoldr )
	ALPHA68K_PLAYER1_INPUT
	ALPHA68K_PLAYER2_INPUT

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* 2 physical sets of _6_ dip switches */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" ) /* 18 Normal */
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, "Manufacturer" )
	PORT_DIPSETTING(    0x40, "SNK" )
	PORT_DIPSETTING(    0x00, "Romstar" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x01, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x07, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x06, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x05, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x03, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* player 1 12-way rotary control */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 2 12-way rotary control */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( goldmedl )
	PORT_START	/* 3 buttons per player, no joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 ) /* Doesn't work? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* 3 buttons per player, no joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
//AT
#if 0
	/* 2 physical sets of _6_ dip switches */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, "Manufacturer" )
	PORT_DIPSETTING(    0x40, "SNK" )
	PORT_DIPSETTING(    0x00, "Romstar" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x01, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x07, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x06, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x05, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x03, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
#endif
        PORT_DIPNAME( 0x04, 0x00, "Event Select" )
        PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x00, DEF_STR( On ) )
        PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
        PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
        PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x20, DEF_STR( On ) )
        PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
        PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x40, DEF_STR( On ) )
        PORT_DIPNAME( 0x80, 0x80, "No. of Players" )
        PORT_DIPSETTING(    0x00, "1-2" )
        PORT_DIPSETTING(    0x80, "1-4" )

        PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x1c, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x18, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x14, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x10, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x0c, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x08, "A 1C/6C B 6C/1C" )
	PORT_DIPSETTING(    0x04, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
        PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
        PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
        PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//ZT
	PORT_START	/* player 1 12-way rotary control */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 2 12-way rotary control */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( skyadvnt )
	ALPHA68K_PLAYER1_INPUT
	ALPHA68K_PLAYER2_INPUT

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* Dip 2: 6 way dip switch */
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x09, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x0e, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x0c, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x0a, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x08, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gangwars )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* Dip 2: 6 way dip switch */
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* A 6 way dip switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x09, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x0e, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x0c, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x0a, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x08, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sbasebal )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* Service + dip */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	/* Dip 2: 6 way dip switch */
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

/*

	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, "Manufacturer" )
	PORT_DIPSETTING(    0x40, "SNK" )
	PORT_DIPSETTING(    0x00, "Romstar" )
	PORT_DIPNAME( 0x80, 0x80, "Invincibility" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
*/

	PORT_START /* A 6 way dip switch */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )
	PORT_DIPSETTING(    0x09, "A 2C/3C B 7C/1C" )
	PORT_DIPSETTING(    0x0e, "A 1C/1C B 1C/1C" )
	PORT_DIPSETTING(    0x0c, "A 1C/2C B 2C/1C" )
	PORT_DIPSETTING(    0x0a, "A 1C/3C B 3C/1C" )
	PORT_DIPSETTING(    0x08, "A 1C/4C B 4C/1C" )
	PORT_DIPSETTING(    0x04, "A 1C/5C B 5C/1C" )
	PORT_DIPSETTING(    0x02, "A 1C/6C B 6C/1C" )

	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	2048,
	4,		/* 4 bits per pixel  */
	{ 0, 4, 0x8000*8, (0x8000*8)+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 8 consecutive bytes */
};

/* You wouldn't believe how long it took me to figure this one out.. */
static struct GfxLayout charlayout_V =
{
	8,8,
	2048,
	4,	/* 4 bits per pixel */
	{ 0,1,2,3 },
  	{ 16*8+4, 16*8+0, 24*8+4, 24*8+0, 4, 0, 8*8+4, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	4096*4,
	4,		/* 4 bits per pixel */
	{ 0, 0x80000*8, 0x100000*8, 0x180000*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout_V =
{
	16,16,	/* 16*16 sprites */
	0x5000,
	4,		/* 4 bits per pixel */
	{ 0, 0xa0000*8, 0x140000*8, 0x1e0000*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};
//AT
#if 0
static struct GfxLayout paddle_layout =
{
	8,8,	/* 8*8 chars */
	0x4000,
	4,	/* 4 bits per pixel */
	{
//0x40000*8+4, 0x40000*8+0,4, 0,
//0x40000*8+4, 4,0x40000*8+0, 0,
// 4,0x40000*8+4,0,  0x40000*8+0,
 4,0,0x40000*8+4,  0x40000*8+0,
//0x40000*8+0,0x40000*8+4,    0,4,

   },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout1 =
{
	8,8,	/* 8*8 chars */
	2048,
	3,	/* 3 bits per pixel */
	{ 4, 4+(0x8000*8), 0+(0x10000*4) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout2 =
{
	8,8,	/* 8*8 chars */
	2048,
	3,	/* 3 bits per pixel */
	{ 0, 0+(0x28000*8), 4+(0x28000*8) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout3 =
{
	8,8,	/* 8*8 chars */
	2048,
	3,	/* 3 bits per pixel */
	{ 0, 0+(0x10000*8), 4+(0x10000*8) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};
#endif
static struct GfxLayout paddle_layout =
{
        8,8,    /* 8*8 chars */
        0x4000,
        4,      /* 4 bits per pixel */
        { 0, 4, 0x40000*8, 0x40000*8+4 },
        { 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout1 =
{
        8,8,    /* 8*8 chars */
        1024,
        3,      /* 3 bits per pixel */
        { 4, 4+(0x8000*8), 0+(0x10000*4) },
        { 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout2 =
{
        8,8,    /* 8*8 chars */
        1024,
        3,      /* 3 bits per pixel */
        { 0, 0+(0x28000*8), 4+(0x28000*8) },
        { 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout sting_layout3 =
{
        8,8,    /* 8*8 chars */
        1024,
        3,      /* 3 bits per pixel */
        { 0, 0+(0x10000*8), 4+(0x10000*8) },
        { 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        16*8    /* every char takes 16 consecutive bytes */
};
//ZT
static struct GfxLayout kyros_char_layout1 =
{
	8,8,	/* 8*8 chars */
	0x8000/16,
	3,	/* 3 bits per pixel */
	{ 4,0x8000*8,0x8000*8+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout kyros_char_layout2 =
{
	8,8,	/* 8*8 chars */
	0x8000/16,
	3,	/* 3 bits per pixel */
	{ 0,0x10000*8,0x10000*8+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout kouyakyu_layout0 =
{
	8,8,
	256,
	2,	/* 2 bits per pixel */
	{ 0, 4 },
  	{ 8*8+3, 8*8+2, 8*8+1, 8*8, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout kouyakyu_layout1 =
{
	16,16,
	1024,
	2,
	{ 0, 4, 0x10000*8 },
   	{ 16*8+3, 16*8+2, 16*8+1, 16*8+0, 32*8+3, 32*8+2 ,32*8+1 ,32*8+0,
      48*8+3 ,48*8+2 ,48*8+1 ,48*8+0, 3, 2, 1, 0
    },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8  },
	64*8
};

static struct GfxLayout kouyakyu_layout2 =
{
	16,16,
	1024,
	2,
	{ 0, 4, 0x10000*8+4 },
   	{ 16*8+3, 16*8+2, 16*8+1, 16*8+0, 32*8+3, 32*8+2 ,32*8+1 ,32*8+0,
      48*8+3 ,48*8+2 ,48*8+1 ,48*8+0, 3, 2, 1, 0
    },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8  },
	64*8
};

/******************************************************************************/

static struct GfxDecodeInfo alpha68k_II_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0,  16 },
	{ REGION_GFX2, 0, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo alpha68k_V_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout_V,    0,  16 },
	{ REGION_GFX2, 0, &spritelayout_V,  0, 256 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo paddle_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &paddle_layout,  0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo sstingry_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &sting_layout1,  0, 256 },
 	{ REGION_GFX1, 0x00000, &sting_layout2,  0, 256 },
	{ REGION_GFX1, 0x10000, &sting_layout1,  0, 256 },
	{ REGION_GFX1, 0x10000, &sting_layout3,  0, 256 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo kyros_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x00000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x18000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x18000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x30000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x30000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x48000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x48000, &kyros_char_layout2,  0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo kouyakyu_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &kouyakyu_layout0,  0, 256 },
	{ REGION_GFX2, 0x000000, &kouyakyu_layout1,  0, 256 },
 	{ REGION_GFX2, 0x000000, &kouyakyu_layout2,  0, 256 },
	{ -1 } /* end of array */
};

/******************************************************************************/
//AT
static struct YM2203interface kyros_ym2203_interface =
{
        3,
        3000000,
        { YM2203_VOL(35,35), YM2203_VOL(35,35), YM2203_VOL(90,90) },
        { 0 },
        { 0 },
        { 0 },
        { 0 },
        { 0 }
};

static struct YM2203interface sstingry_ym2203_interface =
{
        3,
        3000000,
        { YM2203_VOL(35,35), YM2203_VOL(35,35), YM2203_VOL(50,50) },
        { 0 },
        { 0 },
        { 0 },
        { 0 },
        { 0 }
};

static void YM3812_irq(int param)
{
        cpu_set_irq_line(1, 0, (param) ? HOLD_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
        1,
        4000000,
        { 100 },
        { YM3812_irq },
};
//ZT
static struct YM2413interface ym2413_interface=
{
    1,
    8000000,	/* ??? */
    { 100 },
};

static struct YM2203interface ym2203_interface =
{
	1,
	3000000,	/* ??? */
	{ YM2203_VOL(65,65) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};
//AT
#if 0
static struct AY8910interface ay8910_interface =
{
	2, /* 2 chips */
	1500000,
	{ 60, 60 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};
#endif

static INTERRUPT_GEN( goldmedl_interrupt )
{
        if (cpu_getiloops() == 0)
                cpu_set_irq_line(0, 1, HOLD_LINE);
        else
                cpu_set_irq_line(0, 2, HOLD_LINE);
}
//ZT
static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static INTERRUPT_GEN( kyros_interrupt )
{
	if (cpu_getiloops() == 0)
		cpu_set_irq_line(0, 1, HOLD_LINE);
	else
		cpu_set_irq_line(0, 2, HOLD_LINE);
}

/******************************************************************************/


static MACHINE_DRIVER_START( kouyakyu )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000) /* 12MHz/2? */
	MDRV_CPU_MEMORY(kouyakyu_readmem,kouyakyu_writemem)
	MDRV_CPU_VBLANK_INT(kyros_interrupt,2)


	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
//	MDRV_SCREEN_SIZE(64*8, 64*8)
//	MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 64*8-1)
	MDRV_GFXDECODE(kouyakyu_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(kouyakyu)
	MDRV_VIDEO_UPDATE(kouyakyu)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kyros )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000) /* 24MHz/4? */
	MDRV_CPU_MEMORY(kyros_readmem,kyros_writemem)
	MDRV_CPU_VBLANK_INT(kyros_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* ? */
	MDRV_CPU_MEMORY(kyros_sound_readmem,kyros_sound_writemem)
	MDRV_CPU_PORTS(0,kyros_sound_writeport)
//AT
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,8)
        MDRV_CPU_VBLANK_INT(irq0_line_hold, 2)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 4000)
//ZT
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(kyros_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(kyros)
	MDRV_VIDEO_UPDATE(kyros)

	/* sound hardware */
//AT
	//MDRV_SOUND_ADD(YM2203, ym2203_interface)
	//MDRV_SOUND_ADD(AY8910, ay8910_interface)
        MDRV_SOUND_ADD(YM2203, kyros_ym2203_interface)
        MDRV_SOUND_ADD(DAC, dac_interface)
//ZT

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sstingry )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000) /* 24MHz/4? */
	MDRV_CPU_MEMORY(kyros_readmem,kyros_writemem)
	MDRV_CPU_VBLANK_INT(kyros_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* ? */
	MDRV_CPU_MEMORY(sstingry_sound_readmem,sstingry_sound_writemem)
	MDRV_CPU_PORTS(0,kyros_sound_writeport)
//AT
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,32)
        MDRV_CPU_VBLANK_INT(irq0_line_hold, 2)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 4000)
//ZT

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(sstingry_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
//AT
	//MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
        MDRV_COLORTABLE_LENGTH(256)
        MDRV_PALETTE_INIT(kyros)
//ZT
	MDRV_VIDEO_UPDATE(sstingry)

	/* sound hardware */
//AT
	//MDRV_SOUND_ADD(YM2203, ym2203_interface)
	//MDRV_SOUND_ADD(AY8910, ay8910_interface)
        MDRV_SOUND_ADD(YM2203, sstingry_ym2203_interface)
        MDRV_SOUND_ADD(DAC, dac_interface)
//ZT
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( alpha68k_I )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000) /* 24MHz/4? */
	MDRV_CPU_MEMORY(alpha68k_I_readmem,alpha68k_I_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* VBL */
//AT
#if 0
	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
#endif
        MDRV_CPU_ADD(Z80, 4000000) //AT: 4Mhz seems to yield the correct tone
        MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
        MDRV_CPU_MEMORY(alpha68k_I_s_readmem, alpha68k_I_s_writemem) //AT
//ZT
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(paddle_gfxdecodeinfo)
//AT
	//MDRV_PALETTE_LENGTH(256)

	//MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
        MDRV_PALETTE_LENGTH(272)
        MDRV_COLORTABLE_LENGTH(1024)
        MDRV_PALETTE_INIT(paddlem)
//ZT
	MDRV_VIDEO_UPDATE(alpha68k_I)

	/* sound hardware */
	//MDRV_SOUND_ADD(YM2203, ym2203_interface)
        MDRV_SOUND_ADD(YM3812, ym3812_interface) //AT
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( alpha68k_II )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000) /* Correct */
	MDRV_CPU_MEMORY(alpha68k_II_readmem,alpha68k_II_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)/* VBL */

	MDRV_CPU_ADD(Z80, /*3579545*/3579545*2)	/* Unlikely but needed to stop nested NMI's */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* Correct?? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,116)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 7500) //AT

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(alpha68k_II_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(alpha68k)
	MDRV_VIDEO_UPDATE(alpha68k_II)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END
//AT
static MACHINE_DRIVER_START( alpha68k_II_gm )

        /* basic machine hardware */
        MDRV_CPU_ADD(M68000, 8000000)
        MDRV_CPU_MEMORY(alpha68k_II_readmem, alpha68k_II_writemem)
        MDRV_CPU_VBLANK_INT(goldmedl_interrupt, 4)

        MDRV_CPU_ADD(Z80, 4000000*2)
        MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
        MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
        MDRV_CPU_PORTS(sound_readport, sound_writeport)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 7500)

        MDRV_FRAMES_PER_SECOND(60)
        MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

        /* video hardware */
        MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
        MDRV_SCREEN_SIZE(32*8, 32*8)
        MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
        MDRV_GFXDECODE(alpha68k_II_gfxdecodeinfo)
        MDRV_PALETTE_LENGTH(2048)

        MDRV_VIDEO_START(alpha68k)
        MDRV_VIDEO_UPDATE(alpha68k_II)

        /* sound hardware */
        MDRV_SOUND_ADD(YM2203, ym2203_interface)
        MDRV_SOUND_ADD(YM2413, ym2413_interface)
        MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END
//ZT
static MACHINE_DRIVER_START( alpha68k_V )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* ? */
	MDRV_CPU_MEMORY(alpha68k_V_readmem,alpha68k_V_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)/* VBL */

	MDRV_CPU_ADD(Z80, /*3579545*/3579545*2)	/* Unlikely but needed to stop nested NMI's */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,148)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 8500) //AT

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(alpha68k_V_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(alpha68k)
	MDRV_VIDEO_UPDATE(alpha68k_V)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( alpha68k_V_sb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* ? */
	MDRV_CPU_MEMORY(alpha68k_V_readmem,alpha68k_V_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)/* VBL */

	MDRV_CPU_ADD(Z80, /*3579545*/3579545*2)	/* Unlikely but needed to stop nested NMI's */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,112)
        MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 8500) //AT

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(alpha68k_V_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(alpha68k)
	MDRV_VIDEO_UPDATE(alpha68k_V_sb)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( kouyakyu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-6704.bin",  0x00001, 0x2000, 0xc7ac2292 )
	ROM_LOAD16_BYTE( "epr-6707.bin",  0x00000, 0x2000, 0x9cb2962e )
	ROM_LOAD16_BYTE( "epr-6705.bin",  0x04001, 0x2000, 0x985327cb )
	ROM_LOAD16_BYTE( "epr-6708.bin",  0x04000, 0x2000, 0xf8863dc5 )
	ROM_LOAD16_BYTE( "epr-6706.bin",  0x08001, 0x2000, 0x053a74f6 )
	ROM_LOAD16_BYTE( "epr-6709.bin",  0x08000, 0x2000, 0xf41cb58c )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6710.bin", 0x000000, 0x1000, 0xaccda190 ) /* Chars */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-6686.bin", 0x000000, 0x2000, 0x3d9e516f ) /* Tiles plane 1 & 2 */
	ROM_LOAD( "epr-6687.bin", 0x002000, 0x2000, 0x9c1f49df )
	ROM_LOAD( "epr-6688.bin", 0x004000, 0x2000, 0xceb76c5b )
	ROM_LOAD( "epr-6689.bin", 0x006000, 0x2000, 0x53bf7587 )
	ROM_LOAD( "epr-6690.bin", 0x008000, 0x2000, 0xa142a11d )
	ROM_LOAD( "epr-6691.bin", 0x00a000, 0x2000, 0xb640568c )
	ROM_LOAD( "epr-6692.bin", 0x00c000, 0x2000, 0xb91d8172 )
	ROM_LOAD( "epr-6693.bin", 0x00e000, 0x2000, 0x874e3acc )

	ROM_LOAD( "epr-6694.bin", 0x010000, 0x2000, 0x51a7345e ) /* Tiles plane 3 */
	ROM_LOAD( "epr-6695.bin", 0x012000, 0x2000, 0x22bea465 )
	ROM_LOAD( "epr-6696.bin", 0x014000, 0x2000, 0x0625f48e )
	ROM_LOAD( "epr-6697.bin", 0x016000, 0x2000, 0xf18afabe )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "epr-6698.bin", 0x000000, 0x2000, 0x7adfd1ff )
	ROM_LOAD( "epr-6699.bin", 0x002000, 0x2000, 0x9bfa4a72 )
	ROM_LOAD( "epr-6700.bin", 0x004000, 0x2000, 0xee579266 )
	ROM_LOAD( "epr-6701.bin", 0x006000, 0x2000, 0x3c83588a )
	ROM_LOAD( "epr-6702.bin", 0x008000, 0x2000, 0x27ddf031 )
	ROM_LOAD( "epr-6703.bin", 0x00a000, 0x2000, 0xfbff3a86 )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr6627.bpr",  0x000000, 0x100, 0x5ec5480d )
	ROM_LOAD( "pr6628.bpr",  0x000100, 0x100, 0x8af247a4 )
	ROM_LOAD( "pr6629.bpr",  0x000200, 0x100, 0x29c7a393 )
	ROM_LOAD( "pr6630a.bpr", 0x000300, 0x100, 0xd6e202da ) /* Same as pr6630b,c,d */
	ROM_LOAD( "pr.bpr",      0x000400, 0x020, 0x33b98466 )
ROM_END

ROM_START( sstingry )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "ss_05.rom",  0x0000,  0x4000, 0xbfb28d53 )
	ROM_LOAD16_BYTE( "ss_07.rom",  0x0001,  0x4000, 0xeb1b65c5 )
	ROM_LOAD16_BYTE( "ss_04.rom",  0x8000,  0x4000, 0x2e477a79 )
	ROM_LOAD16_BYTE( "ss_06.rom",  0x8001,  0x4000, 0x597620cb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "ss_01.rom",       0x0000,  0x4000, 0xfef09a92 )
	ROM_LOAD( "ss_02.rom",       0x4000,  0x4000, 0xab4e8c01 )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_12.rom",       0x00000, 0x4000, 0x74caa9e9 )
	ROM_LOAD( "ss_08.rom",       0x08000, 0x4000, 0x32368925 )
	ROM_LOAD( "ss_13.rom",       0x10000, 0x4000, 0x13da6203 )
	ROM_LOAD( "ss_10.rom",       0x18000, 0x4000, 0x2903234a )
	ROM_LOAD( "ss_11.rom",       0x20000, 0x4000, 0xd134302e )
	ROM_LOAD( "ss_09.rom",       0x28000, 0x4000, 0x6f9d938a )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "ic92",            0x0000, 0x0100, 0xe7ce1179 )
	ROM_LOAD( "ic93",            0x0100, 0x0100, 0x9af8a375 )
	ROM_LOAD( "ic91",            0x0200, 0x0100, 0xc3965079 )
	ROM_LOAD( "clut_hi.rom",     0x0300, 0x0100, 0 ) // missing
	ROM_LOAD( "clut_lo.rom",     0x0400, 0x0100, 0 ) // missing

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "colormap.rom",    0x0000, 0x2000, 0 ) // may or may not exist
ROM_END

ROM_START( kyros )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2.10c", 0x00000,  0x4000, 0x4bd030b1 )
	ROM_CONTINUE   (          0x10000,  0x4000 )
	ROM_LOAD16_BYTE( "1.13c", 0x00001,  0x4000, 0x75cfbc5e )
	ROM_CONTINUE   (          0x10001,  0x4000 )
	ROM_LOAD16_BYTE( "4.10b", 0x08000,  0x4000, 0xbe2626c2 )
	ROM_CONTINUE   (          0x18000,  0x4000 )
	ROM_LOAD16_BYTE( "3.13b", 0x08001,  0x4000, 0xfb25e71a )
	ROM_CONTINUE   (          0x18001,  0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )   /* Sound CPU */
	ROM_LOAD( "2s.1f",      0x00000, 0x4000, 0x800ceb27 )
	ROM_LOAD( "1s.1d",      0x04000, 0x8000, 0x87d3e719 )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.9pr",      0x00000, 0x8000, 0xc5290944 )
	ROM_LOAD( "11.11m",     0x08000, 0x8000, 0xfbd44f1e )
	ROM_LOAD( "12.11n",	0x10000, 0x8000, 0x10fed501 )
	ROM_LOAD( "9.9s",	0x18000, 0x8000, 0xdd40ca33 )
	ROM_LOAD( "13.11p",	0x20000, 0x8000, 0xe6a02030 )
	ROM_LOAD( "14.11r",	0x28000, 0x8000, 0x722ad23a )
	ROM_LOAD( "15.3t",	0x30000, 0x8000, 0x045fdda4 )
	ROM_LOAD( "17.7t",	0x38000, 0x8000, 0x7618ec00 )
	ROM_LOAD( "18.9t",	0x40000, 0x8000, 0x0ee74171 )
	ROM_LOAD( "16.5t",	0x48000, 0x8000, 0x2cf14824 )
	ROM_LOAD( "19.11t",	0x50000, 0x8000, 0x4f336306 )
	ROM_LOAD( "20.13t",	0x58000, 0x8000, 0xa165d06b )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114l.5r", 0x000, 0x100, 0x3628bf36 )
	ROM_LOAD( "mb7114l.4r", 0x100, 0x100, 0x850704e4 )
	ROM_LOAD( "mb7114l.6r", 0x200, 0x100, 0xa54f60d7 )
	ROM_LOAD( "mb7114l.5p", 0x300, 0x100, 0x1cc53765 )
	ROM_LOAD( "mb7114l.6p", 0x400, 0x100, 0xb0d6971f )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "0.1t",      0x0000,0x2000, 0x5d0acb4c )
ROM_END

ROM_START( paddlema )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "padlem.6g",  0x00000, 0x10000, 0xc227a6e8 )
	ROM_LOAD16_BYTE( "padlem.3g",  0x00001, 0x10000, 0xf11a21aa )
	ROM_LOAD16_BYTE( "padlem.6h",  0x20000, 0x10000, 0x8897555f )
	ROM_LOAD16_BYTE( "padlem.3h",  0x20001, 0x10000, 0xf0fe9b9d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "padlem.18c", 0x000000, 0x10000, 0x9269778d )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "padlem.16m",      0x00000, 0x10000, 0x0984fb4d )
	ROM_LOAD( "padlem.16n",      0x10000, 0x10000, 0x4249e047 )
	ROM_LOAD( "padlem.13m",      0x20000, 0x10000, 0xfd9dbc27 )
	ROM_LOAD( "padlem.13n",      0x30000, 0x10000, 0x1d460486 )
	ROM_LOAD( "padlem.9m",       0x40000, 0x10000, 0x4ee4970d )
	ROM_LOAD( "padlem.9n",       0x50000, 0x10000, 0xa1756f15 )
	ROM_LOAD( "padlem.6m",       0x60000, 0x10000, 0x3f47910c )
	ROM_LOAD( "padlem.6n",       0x70000, 0x10000, 0xfe337655 )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "padlem.a",        0x0000,  0x0100,  0xcae6bcd6 ) /* R */
	ROM_LOAD( "padlem.b",        0x0100,  0x0100,  0xb6df8dcb ) /* G */
	ROM_LOAD( "padlem.c",        0x0200,  0x0100,  0x39ca9b86 ) /* B */
	ROM_LOAD( "padlem.17j",      0x0300,  0x0400,  0x86170069 ) /* Clut low nibble */
	ROM_LOAD( "padlem.16j",      0x0700,  0x0400,  0x8da58e2c ) /* Clut high nibble */

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	ROM_LOAD( "padlem.18n",      0x0000,  0x8000,  0x06506200 ) /* Colour lookup */
ROM_END

ROM_START( timesold )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bf.3",       0x00000,  0x10000, 0xa491e533 )
	ROM_LOAD16_BYTE( "bf.4",       0x00001,  0x10000, 0x34ebaccc )
	ROM_LOAD16_BYTE( "bf.1",       0x20000,  0x10000, 0x158f4cb3 )
	ROM_LOAD16_BYTE( "bf.2",       0x20001,  0x10000, 0xaf01a718 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "bf.7",            0x00000,  0x08000, 0xf8b293b5 )
	ROM_CONTINUE(                0x18000,  0x08000 )
	ROM_LOAD( "bf.8",            0x30000,  0x10000, 0x8a43497b )
	ROM_LOAD( "bf.9",            0x50000,  0x10000, 0x1408416f )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "bf.5",            0x00000,  0x08000, 0x3cec2f55 )
	ROM_LOAD( "bf.6",            0x08000,  0x08000, 0x086a364d )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "bf.10",           0x000000, 0x20000, 0x613313ba )
	ROM_LOAD( "bf.14",           0x020000, 0x20000, 0xefda5c45 )
	ROM_LOAD( "bf.18",           0x040000, 0x20000, 0xe886146a )
	ROM_LOAD( "bf.11",           0x080000, 0x20000, 0x92b42eba )
	ROM_LOAD( "bf.15",           0x0a0000, 0x20000, 0xba3b9f5a )
	ROM_LOAD( "bf.19",           0x0c0000, 0x20000, 0x8994bf10 )
	ROM_LOAD( "bf.12",           0x100000, 0x20000, 0x7ca8bb32 )
	ROM_LOAD( "bf.16",           0x120000, 0x20000, 0x2aa74125 )
	ROM_LOAD( "bf.20",           0x140000, 0x20000, 0xbab6a7c5 )
	ROM_LOAD( "bf.13",           0x180000, 0x20000, 0x56a3a26a )
	ROM_LOAD( "bf.17",           0x1a0000, 0x20000, 0x6b37d048 )
	ROM_LOAD( "bf.21",           0x1c0000, 0x20000, 0xbc3b3944 )
ROM_END

ROM_START( timesol1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3",          0x00000,  0x10000, 0xbc069a29 )
	ROM_LOAD16_BYTE( "4",          0x00001,  0x10000, 0xac7dca56 )
	ROM_LOAD16_BYTE( "bf.1",       0x20000,  0x10000, 0x158f4cb3 )
	ROM_LOAD16_BYTE( "bf.2",       0x20001,  0x10000, 0xaf01a718 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "bf.7",            0x00000,  0x08000, 0xf8b293b5 )
	ROM_CONTINUE(                0x18000,  0x08000 )
	ROM_LOAD( "bf.8",            0x30000,  0x10000, 0x8a43497b )
	ROM_LOAD( "bf.9",            0x50000,  0x10000, 0x1408416f )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "bf.5",            0x00000,  0x08000, 0x3cec2f55 )
	ROM_LOAD( "bf.6",            0x08000,  0x08000, 0x086a364d )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "bf.10",           0x000000, 0x20000, 0x613313ba )
	ROM_LOAD( "bf.14",           0x020000, 0x20000, 0xefda5c45 )
	ROM_LOAD( "bf.18",           0x040000, 0x20000, 0xe886146a )
	ROM_LOAD( "bf.11",           0x080000, 0x20000, 0x92b42eba )
	ROM_LOAD( "bf.15",           0x0a0000, 0x20000, 0xba3b9f5a )
	ROM_LOAD( "bf.19",           0x0c0000, 0x20000, 0x8994bf10 )
	ROM_LOAD( "bf.12",           0x100000, 0x20000, 0x7ca8bb32 )
	ROM_LOAD( "bf.16",           0x120000, 0x20000, 0x2aa74125 )
	ROM_LOAD( "bf.20",           0x140000, 0x20000, 0xbab6a7c5 )
	ROM_LOAD( "bf.13",           0x180000, 0x20000, 0x56a3a26a )
	ROM_LOAD( "bf.17",           0x1a0000, 0x20000, 0x6b37d048 )
	ROM_LOAD( "bf.21",           0x1c0000, 0x20000, 0xbc3b3944 )
ROM_END

ROM_START( btlfield )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bfv1_03.bin", 0x00000, 0x10000, 0x8720af0d )
	ROM_LOAD16_BYTE( "bfv1_04.bin", 0x00001, 0x10000, 0x7dcccbe6 )
	ROM_LOAD16_BYTE( "bf.1",        0x20000, 0x10000, 0x158f4cb3 )
	ROM_LOAD16_BYTE( "bf.2",        0x20001, 0x10000, 0xaf01a718 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "bf.7",            0x00000,  0x08000, 0xf8b293b5 )
	ROM_CONTINUE(                0x18000,  0x08000 )
	ROM_LOAD( "bf.8",            0x30000,  0x10000, 0x8a43497b )
	ROM_LOAD( "bf.9",            0x50000,  0x10000, 0x1408416f )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "bfv1_05.bin",     0x00000,  0x08000, 0xbe269dbf )
	ROM_LOAD( "bfv1_06.bin",     0x08000,  0x08000, 0x022b9de9 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "bf.10",           0x000000, 0x20000, 0x613313ba )
	ROM_LOAD( "bf.14",           0x020000, 0x20000, 0xefda5c45 )
	ROM_LOAD( "bf.18",           0x040000, 0x20000, 0xe886146a )
	ROM_LOAD( "bf.11",           0x080000, 0x20000, 0x92b42eba )
	ROM_LOAD( "bf.15",           0x0a0000, 0x20000, 0xba3b9f5a )
	ROM_LOAD( "bf.19",           0x0c0000, 0x20000, 0x8994bf10 )
	ROM_LOAD( "bf.12",           0x100000, 0x20000, 0x7ca8bb32 )
	ROM_LOAD( "bf.16",           0x120000, 0x20000, 0x2aa74125 )
	ROM_LOAD( "bf.20",           0x140000, 0x20000, 0xbab6a7c5 )
	ROM_LOAD( "bf.13",           0x180000, 0x20000, 0x56a3a26a )
	ROM_LOAD( "bf.17",           0x1a0000, 0x20000, 0x6b37d048 )
	ROM_LOAD( "bf.21",           0x1c0000, 0x20000, 0xbc3b3944 )
ROM_END

ROM_START( skysoldr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ss.3",       0x00000, 0x10000, 0x7b88aa2e )
	ROM_CONTINUE ( 0x40000,      0x10000 )
	ROM_LOAD16_BYTE( "ss.4",       0x00001, 0x10000, 0xf0283d43 )
	ROM_CONTINUE ( 0x40001,      0x10000 )
	ROM_LOAD16_BYTE( "ss.1",       0x20000, 0x10000, 0x20e9dbc7 )
	ROM_CONTINUE ( 0x60000,      0x10000 )
	ROM_LOAD16_BYTE( "ss.2",       0x20001, 0x10000, 0x486f3432 )
	ROM_CONTINUE ( 0x60001,      0x10000 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ss.7",            0x00000, 0x08000, 0xb711fad4 )
	ROM_CONTINUE(                0x18000, 0x08000 )
	ROM_LOAD( "ss.8",            0x30000, 0x10000, 0xe5cf7b37 )
	ROM_LOAD( "ss.9",            0x50000, 0x10000, 0x76124ca2 )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "ss.5",            0x00000, 0x08000, 0x928ba287 )
	ROM_LOAD( "ss.6",            0x08000, 0x08000, 0x93b30b55 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ss.10",          0x000000, 0x20000, 0xe48c1623 )
	ROM_LOAD( "ss.14",			0x020000, 0x20000, 0x190c8704 )
	ROM_LOAD( "ss.18",			0x040000, 0x20000, 0xcb6ff33a )
	ROM_LOAD( "ss.22",			0x060000, 0x20000, 0xe69b4485 )
	ROM_LOAD( "ss.11",			0x080000, 0x20000, 0x6c63e9c5 )
	ROM_LOAD( "ss.15",			0x0a0000, 0x20000, 0x55f71ab1 )
	ROM_LOAD( "ss.19",			0x0c0000, 0x20000, 0x312a21f5 )
	ROM_LOAD( "ss.23",			0x0e0000, 0x20000, 0x923c19c2 )
	ROM_LOAD( "ss.12",			0x100000, 0x20000, 0x63bb4e89 )
	ROM_LOAD( "ss.16",			0x120000, 0x20000, 0x138179f7 )
	ROM_LOAD( "ss.20",			0x140000, 0x20000, 0x268cc7b4 )
	ROM_LOAD( "ss.24",			0x160000, 0x20000, 0xf63b8417 )
	ROM_LOAD( "ss.13",			0x180000, 0x20000, 0x3506c06b )
	ROM_LOAD( "ss.17",			0x1a0000, 0x20000, 0xa7f524e0 )
	ROM_LOAD( "ss.21",			0x1c0000, 0x20000, 0xcb7bf5fe )
	ROM_LOAD( "ss.25",			0x1e0000, 0x20000, 0x65138016 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 ) /* Reload the code here for upper bank */
	ROM_LOAD16_BYTE( "ss.3",      0x00000, 0x10000, 0x7b88aa2e )
	ROM_CONTINUE   ( 0x40000,     0x10000 )
	ROM_LOAD16_BYTE( "ss.4",      0x00001, 0x10000, 0xf0283d43 )
	ROM_CONTINUE   ( 0x40001,     0x10000 )
	ROM_LOAD16_BYTE( "ss.1",      0x20000, 0x10000, 0x20e9dbc7 )
	ROM_CONTINUE   ( 0x60000,     0x10000 )
	ROM_LOAD16_BYTE( "ss.2",      0x20001, 0x10000, 0x486f3432 )
	ROM_CONTINUE   ( 0x60001,     0x10000 )
ROM_END

ROM_START( goldmedl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "gm.3",      0x00000,  0x10000, 0xddf0113c )
	ROM_LOAD16_BYTE( "gm.4",      0x00001,  0x10000, 0x16db4326 )
	ROM_LOAD16_BYTE( "gm.1",      0x20000,  0x10000, 0x54a11e28 )
	ROM_LOAD16_BYTE( "gm.2",      0x20001,  0x10000, 0x4b6a13e4 )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "goldsnd0.c47",   0x00000,  0x08000, BADCRC(0x031d27dc) )
	ROM_CONTINUE(               0x10000,  0x78000 )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "gm.5",           0x000000, 0x08000, 0x667f33f1 )
	ROM_LOAD( "gm.6",           0x008000, 0x08000, 0x56020b13 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "goldchr3.c46",   0x000000, 0x80000, 0x6faaa07a )
	ROM_LOAD( "goldchr2.c45",   0x080000, 0x80000, 0xe6b0aa2c )
	ROM_LOAD( "goldchr1.c44",   0x100000, 0x80000, 0x55db41cd )
	ROM_LOAD( "goldchr0.c43",   0x180000, 0x80000, 0x76572c3f )
ROM_END

ROM_START( goldmedb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "l_3.bin",   0x00000,  0x10000, 0x5e106bcf)
	ROM_LOAD16_BYTE( "l_4.bin",   0x00001,  0x10000, 0xe19966af)
	ROM_LOAD16_BYTE( "l_1.bin",   0x20000,  0x08000, 0x7eec7ee5)
	ROM_LOAD16_BYTE( "l_2.bin",   0x20001,  0x08000, 0xbf59e4f9)

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "goldsnd0.c47",   0x00000,  0x08000, 0x031d27dc )
	ROM_CONTINUE(               0x10000,  0x78000 )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "gm.5",           0x000000, 0x08000, 0x667f33f1 )
	ROM_LOAD( "gm.6",           0x008000, 0x08000, 0x56020b13 )
//	ROM_LOAD( "33.bin", 0x000000, 0x10000, 0x5600b13 )

	/* I haven't yet verified if these are the same as the bootleg */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "goldchr3.c46",   0x000000, 0x80000, 0x6faaa07a )
	ROM_LOAD( "goldchr2.c45",   0x080000, 0x80000, 0xe6b0aa2c )
	ROM_LOAD( "goldchr1.c44",   0x100000, 0x80000, 0x55db41cd )
	ROM_LOAD( "goldchr0.c43",   0x180000, 0x80000, 0x76572c3f )

	ROM_REGION16_BE( 0x10000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "l_1.bin",   0x00000,  0x08000, 0x7eec7ee5)
	ROM_LOAD16_BYTE( "l_2.bin",   0x00001,  0x08000, 0xbf59e4f9)
ROM_END

ROM_START( skyadvnt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sa1.bin",   0x00000,  0x20000, 0xc2b23080 )
	ROM_LOAD16_BYTE( "sa2.bin",   0x00001,  0x20000, 0x06074e72 )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "sa.3",           0x00000,  0x08000, 0x3d0b32e0 )
	ROM_CONTINUE(               0x18000,  0x08000 )
	ROM_LOAD( "sa.4",           0x30000,  0x10000, 0xc2e3c30c )
	ROM_LOAD( "sa.5",           0x50000,  0x10000, 0x11cdb868 )
	ROM_LOAD( "sa.6",           0x70000,  0x08000, 0x237d93fd )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "sa.7",           0x000000, 0x08000, 0xea26e9c5 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "sachr3",         0x000000, 0x80000, 0xa986b8d5 )
	ROM_LOAD( "sachr2",         0x0a0000, 0x80000, 0x504b07ae )
	ROM_LOAD( "sachr1",         0x140000, 0x80000, 0xe734dccd )
	ROM_LOAD( "sachr0",         0x1e0000, 0x80000, 0xe281b204 )
ROM_END

ROM_START( skyadvnu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sa_v3.1",   0x00000,  0x20000, 0x862393b5 )
	ROM_LOAD16_BYTE( "sa_v3.2",   0x00001,  0x20000, 0xfa7a14d1 )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "sa.3",           0x00000,  0x08000, 0x3d0b32e0 )
	ROM_CONTINUE(               0x18000,  0x08000 )
	ROM_LOAD( "sa.4",           0x30000,  0x10000, 0xc2e3c30c )
	ROM_LOAD( "sa.5",           0x50000,  0x10000, 0x11cdb868 )
	ROM_LOAD( "sa.6",           0x70000,  0x08000, 0x237d93fd )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "sa.7",           0x000000, 0x08000, 0xea26e9c5 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "sachr3",         0x000000, 0x80000, 0xa986b8d5 )
	ROM_LOAD( "sachr2",         0x0a0000, 0x80000, 0x504b07ae )
	ROM_LOAD( "sachr1",         0x140000, 0x80000, 0xe734dccd )
	ROM_LOAD( "sachr0",         0x1e0000, 0x80000, 0xe281b204 )
ROM_END

ROM_START( skyadvnj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "saj1.c19",  0x00000,  0x20000, 0x662cb4b8 )
	ROM_LOAD16_BYTE( "saj2.e19",  0x00001,  0x20000, 0x06d6130a )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "sa.3",           0x00000,  0x08000, 0x3d0b32e0 )
	ROM_CONTINUE(               0x18000,  0x08000 )
	ROM_LOAD( "sa.4",           0x30000,  0x10000, 0xc2e3c30c )
	ROM_LOAD( "sa.5",           0x50000,  0x10000, 0x11cdb868 )
	ROM_LOAD( "sa.6",           0x70000,  0x08000, 0x237d93fd )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "sa.7",           0x000000, 0x08000, 0xea26e9c5 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "sachr3",         0x000000, 0x80000, 0xa986b8d5 )
	ROM_LOAD( "sachr2",         0x0a0000, 0x80000, 0x504b07ae )
	ROM_LOAD( "sachr1",         0x140000, 0x80000, 0xe734dccd )
	ROM_LOAD( "sachr0",         0x1e0000, 0x80000, 0xe281b204 )
ROM_END

ROM_START( gangwars )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u1",        0x00000, 0x20000, 0x11433507 )
	ROM_LOAD16_BYTE( "u2",        0x00001, 0x20000, 0x44cc375f )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "u12",            0x00000, 0x08000, 0x2620caa1 )
	ROM_CONTINUE(               0x18000, 0x08000 )
	ROM_LOAD( "u9",             0x70000, 0x10000, 0x9136745e )
	ROM_LOAD( "u10",            0x50000, 0x10000, 0x636978ae )
	ROM_LOAD( "u11",            0x30000, 0x10000, 0x2218ceb9 )

/*

These roms are from the bootleg version, the original graphics
should be the same.  The original uses 4 512k mask roms and 4 128k
eeproms to store the same data.  The 512k roms are not dumped but
the 128k ones are and match these ones.

*/

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "gwb_ic.m19",		0x000000, 0x10000, 0xb75bf1d0 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "gwb_ic.308",		0x000000, 0x10000, 0x321a2fdd )
	ROM_LOAD( "gwb_ic.309",		0x010000, 0x10000, 0x4d908f65 )
	ROM_LOAD( "gwb_ic.310",		0x020000, 0x10000, 0xfc888541 )
	ROM_LOAD( "gwb_ic.311",		0x030000, 0x10000, 0x181b128b )
	ROM_LOAD( "gwb_ic.312",		0x040000, 0x10000, 0x930665f3 )
	ROM_LOAD( "gwb_ic.313",		0x050000, 0x10000, 0xc18f4ca8 )
	ROM_LOAD( "gwb_ic.314",		0x060000, 0x10000, 0xdfc44b60 )
	ROM_LOAD( "gwb_ic.307",		0x070000, 0x10000, 0x28082a7f )
	//ROM_LOAD( "gwb_ic.320",		0x080000, 0x10000, 0x9a7b51d8 )
        ROM_LOAD( "gwb_ic.280",         0x080000, 0x10000, 0x222b3dcd ) //AT
	ROM_LOAD( "gwb_ic.321",		0x090000, 0x10000, 0x6b421c7b )
	ROM_LOAD( "gwb_ic.300",		0x0a0000, 0x10000, 0xf3fa0877 )
	ROM_LOAD( "gwb_ic.301",		0x0b0000, 0x10000, 0xf8c866de )
	ROM_LOAD( "gwb_ic.302",		0x0c0000, 0x10000, 0x5b0d587d )
	ROM_LOAD( "gwb_ic.303",		0x0d0000, 0x10000, 0xd8c0e102 )
	ROM_LOAD( "gwb_ic.304",		0x0e0000, 0x10000, 0xb02bc9d8 )
	ROM_LOAD( "gwb_ic.305",		0x0f0000, 0x10000, 0x5e04a9aa )
	ROM_LOAD( "gwb_ic.306",		0x100000, 0x10000, 0xe2172955 )
	ROM_LOAD( "gwb_ic.299",		0x110000, 0x10000, 0xe39f5599 )
	//ROM_LOAD( "gwb_ic.318",		0x120000, 0x10000, 0x9aeaddf9 )
        ROM_LOAD( "gwb_ic.320",         0x120000, 0x10000, 0x9a7b51d8 ) //AT
	ROM_LOAD( "gwb_ic.319",		0x130000, 0x10000, 0xc5b862b7 )
	ROM_LOAD( "gwb_ic.292",		0x140000, 0x10000, 0xc125f7be )
	ROM_LOAD( "gwb_ic.293",		0x150000, 0x10000, 0xc04fce8e )
	ROM_LOAD( "gwb_ic.294",		0x160000, 0x10000, 0x4eda3df5 )
	ROM_LOAD( "gwb_ic.295",		0x170000, 0x10000, 0x6e60c475 )
	ROM_LOAD( "gwb_ic.296",		0x180000, 0x10000, 0x99b2a557 )
	ROM_LOAD( "gwb_ic.297",		0x190000, 0x10000, 0x10373f63 )
	ROM_LOAD( "gwb_ic.298",		0x1a0000, 0x10000, 0xdf37ec4d )
	ROM_LOAD( "gwb_ic.291",		0x1b0000, 0x10000, 0xbeb07a2e )
	//ROM_LOAD( "gwb_ic.316",		0x1c0000, 0x10000, 0x655b1518 )
        ROM_LOAD( "gwb_ic.318",         0x1c0000, 0x10000, 0x9aeaddf9 ) //AT
	ROM_LOAD( "gwb_ic.317",		0x1d0000, 0x10000, 0x1622fadd )
	ROM_LOAD( "gwb_ic.284",		0x1e0000, 0x10000, 0x4aa95d66 )
	ROM_LOAD( "gwb_ic.285",		0x1f0000, 0x10000, 0x3a1f3ce0 )
	ROM_LOAD( "gwb_ic.286",		0x200000, 0x10000, 0x886e298b )
	ROM_LOAD( "gwb_ic.287",		0x210000, 0x10000, 0xb9542e6a )
	ROM_LOAD( "gwb_ic.288",		0x220000, 0x10000, 0x8e620056 )
	ROM_LOAD( "gwb_ic.289",		0x230000, 0x10000, 0xc754d69f )
	ROM_LOAD( "gwb_ic.290",		0x240000, 0x10000, 0x306d1963 )
	ROM_LOAD( "gwb_ic.283",		0x250000, 0x10000, 0xb46e5761 )
	//ROM_LOAD( "gwb_ic.280",		0x260000, 0x10000, 0x222b3dcd )
        ROM_LOAD( "gwb_ic.316",         0x260000, 0x10000, 0x655b1518 ) //AT
	ROM_LOAD( "gwb_ic.315",		0x270000, 0x10000, 0xe7c9b103 )

	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 ) /* Extra code bank */
	ROM_LOAD16_BYTE( "u3",        0x00000,  0x20000, 0xde6fd3c0 )
	ROM_LOAD16_BYTE( "u4",        0x00001,  0x20000, 0x43f7f5d3 )
ROM_END

ROM_START( gangwarb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "gwb_ic.m15", 0x00000, 0x20000, 0x7752478e )
	ROM_LOAD16_BYTE( "gwb_ic.m16", 0x00001, 0x20000, 0xc2f3b85e )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "gwb_ic.380",      0x00000, 0x08000, 0xe6d6c9cf )
	ROM_CONTINUE(                0x18000, 0x08000 )
	ROM_LOAD( "gwb_ic.419",      0x30000, 0x10000, 0x84e5c946 )
	ROM_LOAD( "gwb_ic.420",      0x50000, 0x10000, 0xeb305d42 )
	ROM_LOAD( "gwb_ic.421",      0x70000, 0x10000, 0x7b9f2608 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "gwb_ic.m19",		0x000000, 0x10000, 0xb75bf1d0 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "gwb_ic.308",		0x000000, 0x10000, 0x321a2fdd )
	ROM_LOAD( "gwb_ic.309",		0x010000, 0x10000, 0x4d908f65 )
	ROM_LOAD( "gwb_ic.310",		0x020000, 0x10000, 0xfc888541 )
	ROM_LOAD( "gwb_ic.311",		0x030000, 0x10000, 0x181b128b )
	ROM_LOAD( "gwb_ic.312",		0x040000, 0x10000, 0x930665f3 )
	ROM_LOAD( "gwb_ic.313",		0x050000, 0x10000, 0xc18f4ca8 )
	ROM_LOAD( "gwb_ic.314",		0x060000, 0x10000, 0xdfc44b60 )
	ROM_LOAD( "gwb_ic.307",		0x070000, 0x10000, 0x28082a7f )
	//ROM_LOAD( "gwb_ic.320",		0x080000, 0x10000, 0x9a7b51d8 )
        ROM_LOAD( "gwb_ic.280",         0x080000, 0x10000, 0x222b3dcd ) //AT
	ROM_LOAD( "gwb_ic.321",		0x090000, 0x10000, 0x6b421c7b )
	ROM_LOAD( "gwb_ic.300",		0x0a0000, 0x10000, 0xf3fa0877 )
	ROM_LOAD( "gwb_ic.301",		0x0b0000, 0x10000, 0xf8c866de )
	ROM_LOAD( "gwb_ic.302",		0x0c0000, 0x10000, 0x5b0d587d )
	ROM_LOAD( "gwb_ic.303",		0x0d0000, 0x10000, 0xd8c0e102 )
	ROM_LOAD( "gwb_ic.304",		0x0e0000, 0x10000, 0xb02bc9d8 )
	ROM_LOAD( "gwb_ic.305",		0x0f0000, 0x10000, 0x5e04a9aa )
	ROM_LOAD( "gwb_ic.306",		0x100000, 0x10000, 0xe2172955 )
	ROM_LOAD( "gwb_ic.299",		0x110000, 0x10000, 0xe39f5599 )
	//ROM_LOAD( "gwb_ic.318",		0x120000, 0x10000, 0x9aeaddf9 )
        ROM_LOAD( "gwb_ic.320",         0x120000, 0x10000, 0x9a7b51d8 ) //AT
	ROM_LOAD( "gwb_ic.319",		0x130000, 0x10000, 0xc5b862b7 )
	ROM_LOAD( "gwb_ic.292",		0x140000, 0x10000, 0xc125f7be )
	ROM_LOAD( "gwb_ic.293",		0x150000, 0x10000, 0xc04fce8e )
	ROM_LOAD( "gwb_ic.294",		0x160000, 0x10000, 0x4eda3df5 )
	ROM_LOAD( "gwb_ic.295",		0x170000, 0x10000, 0x6e60c475 )
	ROM_LOAD( "gwb_ic.296",		0x180000, 0x10000, 0x99b2a557 )
	ROM_LOAD( "gwb_ic.297",		0x190000, 0x10000, 0x10373f63 )
	ROM_LOAD( "gwb_ic.298",		0x1a0000, 0x10000, 0xdf37ec4d )
	ROM_LOAD( "gwb_ic.291",		0x1b0000, 0x10000, 0xbeb07a2e )
	//ROM_LOAD( "gwb_ic.316",		0x1c0000, 0x10000, 0x655b1518 )
        ROM_LOAD( "gwb_ic.318",         0x1c0000, 0x10000, 0x9aeaddf9 ) //AT
	ROM_LOAD( "gwb_ic.317",		0x1d0000, 0x10000, 0x1622fadd )
	ROM_LOAD( "gwb_ic.284",		0x1e0000, 0x10000, 0x4aa95d66 )
	ROM_LOAD( "gwb_ic.285",		0x1f0000, 0x10000, 0x3a1f3ce0 )
	ROM_LOAD( "gwb_ic.286",		0x200000, 0x10000, 0x886e298b )
	ROM_LOAD( "gwb_ic.287",		0x210000, 0x10000, 0xb9542e6a )
	ROM_LOAD( "gwb_ic.288",		0x220000, 0x10000, 0x8e620056 )
	ROM_LOAD( "gwb_ic.289",		0x230000, 0x10000, 0xc754d69f )
	ROM_LOAD( "gwb_ic.290",		0x240000, 0x10000, 0x306d1963 )
	ROM_LOAD( "gwb_ic.283",		0x250000, 0x10000, 0xb46e5761 )
	//ROM_LOAD( "gwb_ic.280",		0x260000, 0x10000, 0x222b3dcd )
        ROM_LOAD( "gwb_ic.316",         0x260000, 0x10000, 0x655b1518 ) //AT
	ROM_LOAD( "gwb_ic.315",		0x270000, 0x10000, 0xe7c9b103 )

	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 ) /* Extra code bank */
	ROM_LOAD16_BYTE( "gwb_ic.m17", 0x00000, 0x20000, 0x2a5fe86e )
	ROM_LOAD16_BYTE( "gwb_ic.m18", 0x00001, 0x20000, 0xc8b60c53 )
ROM_END

ROM_START( sbasebal )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "snksb1.bin", 0x00000, 0x20000, 0x304fef2d )
	ROM_LOAD16_BYTE( "snksb2.bin", 0x00001, 0x20000, 0x35821339 )

	ROM_REGION( 0x90000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "snksb3.bin",      0x00000, 0x08000, 0x89e12f25 )
	ROM_CONTINUE(                0x18000, 0x08000 )
	ROM_LOAD( "snksb4.bin",      0x30000, 0x10000, 0xcca2555d )
	ROM_LOAD( "snksb5.bin",      0x50000, 0x10000, 0xf45ee36f )
	ROM_LOAD( "snksb6.bin",      0x70000, 0x10000, 0x651c9472 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "snksb7.bin",      0x000000, 0x10000, 0x8f3c2e25 )

	ROM_REGION( 0x280000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "kcbchr3.bin",     0x000000, 0x80000, 0x719071c7 )
	ROM_LOAD( "kcbchr2.bin",     0x0a0000, 0x80000, 0x014f0f90 )
	ROM_LOAD( "kcbchr1.bin",     0x140000, 0x80000, 0xa5ce1e10 )
	ROM_LOAD( "kcbchr0.bin",     0x1e0000, 0x80000, 0xb8a1a088 )
ROM_END

/******************************************************************************/

static READ16_HANDLER( timesold_cycle_r )
{
	int ret=shared_ram[0x4];

	if (activecpu_get_pc()==0x9ea2 && (ret&0xff00)==0) {
		cpu_spinuntil_int();
		return 0x100 | (ret&0xff);
	}

	return ret;
}

static READ16_HANDLER( skysoldr_cycle_r )
{
	int ret=shared_ram[0x4];

	if (activecpu_get_pc()==0x1f4e && (ret&0xff00)==0) {
		cpu_spinuntil_int();
		return 0x100 | (ret&0xff);
	}

	return ret;
}

static READ16_HANDLER( gangwars_cycle_r )
{
	int ret=shared_ram[0x103];

	if (activecpu_get_pc()==0xbbca || activecpu_get_pc()==0xbbb6) {
		cpu_spinuntil_int();
		return (ret+2)&0xff;
	}

	return ret;
}

static DRIVER_INIT( timesold )
{
	install_mem_read16_handler(0, 0x40008, 0x40009, timesold_cycle_r);
	invert_controls=0;
}

static DRIVER_INIT( skysoldr )
{
	install_mem_read16_handler(0, 0x40008, 0x40009, skysoldr_cycle_r);
	cpu_setbank(8, (memory_region(REGION_USER1))+0x40000);
	invert_controls=0;
}

static DRIVER_INIT( goldmedb )
{
	cpu_setbank(8, memory_region(REGION_USER1));
	invert_controls=0;
        microcontroller_id=0x8803; //AT
}

static DRIVER_INIT( gangwars )
{
	install_mem_read16_handler(0, 0x40206, 0x40207, gangwars_cycle_r);
	cpu_setbank(8, memory_region(REGION_USER1));
	invert_controls=0;
	microcontroller_id=0x8512;
}

static DRIVER_INIT( btlfield )
{
	invert_controls=1;
}

static DRIVER_INIT( sbasebal )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);

	/* Game hangs on divide by zero?!  Patch it */
	rom[0xb672/2] = 0x4e71;

	/* And patch the ROM checksums */
	rom[0x44e/2] = 0x4e71;
	rom[0x450/2] = 0x4e71;
	rom[0x458/2] = 0x4e71;
	rom[0x45a/2] = 0x4e71;

	microcontroller_id=0x8512;
	invert_controls=0;
	coin_id=0x23|(0x24<<8);
}

static DRIVER_INIT( skyadvnt )
{
	microcontroller_id=0x8814;
	invert_controls=0;
	coin_id=0x22|(0x22<<8);
}

static DRIVER_INIT( skyadvnu )
{
	microcontroller_id=0x8814;
	invert_controls=0;
	coin_id=0x23|(0x24<<8);
}

static DRIVER_INIT( kyros )
{
	microcontroller_id=0x12;
}

static DRIVER_INIT( sstingry )
{
	microcontroller_id=0xff;
}

static DRIVER_INIT( goldmedl )
{
	invert_controls=0;
        microcontroller_id=0x8803; //AT
}

/******************************************************************************/

GAMEX(1985, kouyakyu, 0,        kouyakyu,      sstingry, 0,        ROT0,  "Alpha Denshi Co.", "The Koukouyakyuh", GAME_NOT_WORKING )
GAMEX(1986, sstingry, 0,        sstingry,      sstingry, sstingry, ROT90, "Alpha Denshi Co.", "Super Stingray", GAME_NO_COCKTAIL | GAME_WRONG_COLORS )
GAMEX(1987, kyros,    0,        kyros,         kyros,    kyros,    ROT90, "World Games Inc", "Kyros", GAME_NO_COCKTAIL )
GAMEX(1988, paddlema, 0,        alpha68k_I,    paddlema, 0,        ROT90, "SNK", "Paddle Mania", GAME_NO_COCKTAIL )
GAME( 1987, timesold, 0,        alpha68k_II,   timesold, timesold, ROT90, "SNK / Romstar", "Time Soldiers (US Rev 3)" )
GAME( 1987, timesol1, timesold, alpha68k_II,   timesold, btlfield, ROT90, "SNK / Romstar", "Time Soldiers (US Rev 1)" )
GAME( 1987, btlfield, timesold, alpha68k_II,   btlfield, btlfield, ROT90, "SNK", "Battle Field (Japan)" )
GAME( 1988, skysoldr, 0,        alpha68k_II,   skysoldr, skysoldr, ROT90, "SNK / Romstar", "Sky Soldiers (US)" )
GAMEX(1988, goldmedl, 0,        alpha68k_II_gm,goldmedl, goldmedl, ROT0,  "SNK", "Gold Medalist", GAME_IMPERFECT_SOUND ) //AT
GAMEX(1988, goldmedb, goldmedl, alpha68k_II,   goldmedl, goldmedb, ROT0,  "bootleg", "Gold Medalist (bootleg)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAME( 1989, skyadvnt, 0,        alpha68k_V,    skyadvnt, skyadvnt, ROT90, "Alpha Denshi Co.", "Sky Adventure (World)" )
GAME( 1989, skyadvnu, skyadvnt, alpha68k_V,    skyadvnt, skyadvnu, ROT90, "Alpha Denshi Co. (SNK of America license)", "Sky Adventure (US)" )
GAME( 1989, skyadvnj, skyadvnt, alpha68k_V,    skyadvnt, skyadvnt, ROT90, "Alpha Denshi Co.", "Sky Adventure (Japan)" )
GAME( 1989, gangwars, 0,        alpha68k_V,    gangwars, gangwars, ROT0,  "Alpha Denshi Co.", "Gang Wars (US)" )
GAME( 1989, gangwarb, gangwars, alpha68k_V,    gangwars, gangwars, ROT0,  "bootleg", "Gang Wars (bootleg)" )
GAMEX(1989, sbasebal, 0,        alpha68k_V_sb, sbasebal, sbasebal, ROT0,  "Alpha Denshi Co. (SNK of America license)", "Super Champion Baseball", GAME_NO_COCKTAIL )

/*
 Super Baseball and Sky Adventure don't write to their flipscreen ports - I'm not yet sure if this
is a microcontroller protection issue (flipscreen is controlled by the Alpha microcontroller) or these
games simply don't support it.
*/
