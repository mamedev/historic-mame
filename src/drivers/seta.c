/***************************************************************************

							-= Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


CPU    :	68000 + 65C02 [Optional]
Other  :	NEC D4701 [?]
Custom :	X1-001A  X1-002A			Sprites
			X1-001
			X1-002
			X1-003
			X1-004
			X1-005   X0-005
			X1-006   X0-006
			X1-007
			X1-010						Sound: 16 Bit PCM
			X1-011 x2  X1-012 x2		Tilemaps
			X1-014						Sprites?

---------------------------------------------------------------------------
Ordered by Board		Year + Game						Licensed To
---------------------------------------------------------------------------
?						88 Caliber 50					Taito / RomStar
?        (M6100326A)	88 Twin Eagle(1)				Taito
P0-029-A (M6100287A)	88 Thundercade / Twin Formation	Taito
?						89 Arbalester					Taito / RomStar
?						89 DownTown						Taito / RomStar
?        (M6100430A)	89 U.S. Classic(2)				Taito / RomStar
P1-036-A + P0-045-A +
P1-049-A				89 Meta Fox						Taito / RomStar
P0-053-1				89 Castle of Dragon/Dragon Unit	Taito / RomStar / Athena
P0-053-A				91 Strike Gunner S.T.G			Athena / Tecmo
P0-053-A				92 Quiz Kokology				Tecmo
P0-055-B				89 Wit's						Athena
P0-055-D				90 Thunder & Lightning			Romstar / Visco
P0-063-A				91 Rezon						Allumer
P0-068-B (M6100723A)	92 Block Carnival				Visco
P0-072-2 (prototype)	92 Blandia (prototype)			Allumer
P0-077-A (BP922)		92 Ultraman Club				Banpresto
PO-078-A				92 Blandia						Allumer
P0-079-A				92 Zing Zing Zip				Allumer + Tecmo
P0-079-A				94 Eight Forces					Tecmo
?						93 Athena no Hatena?			Athena
?						93 J.J.Squawkers				Athena / Able
?        (93111A)		93 War Of Aero					Yang Cheng
P0-081-A				93 Mobile Suit Gundam(3)		Banpresto
PO-092-A                93 Daioh                        Athena
PO-096-A ( BP934KA )    93 Kamen Rider (5)              Banpresto
P0-097-A				93 Oishii Puzzle ..				Sunsoft + Atlus
bootleg					9? Triple Fun (4)				bootleg (Comad?)
P0-100-A				93 Quiz Kokology 2				Tecmo
P0-101-1				94 Pro Mahjong Kiwame			Athena
P0-114-A (SKB-001)		94 Krazy Bowl					American Sammy
P0-117-A (DH-01)		95 Extreme Downhill				Sammy Japan
P0-117-A?				95 Sokonuke Taisen Game			Sammy Industries
P0-120-A (BP954KA)		95 Gundhara						Banpresto
---------------------------------------------------------------------------
(1)	some wrong tiles	(2) wrong colors
(3) not working: if the demo modes runs long enough, the colors will screw up.
(4) this is a bootleg of Oishii Puzzle, in english, is there an official
    version?  the sound system has been replaced with an OKI M6295
    hardware is definitely bootleg. standard simple layout board with no
    custom chips and no manufacturer on the pcb. yep bootleg for sure
(5) Alignment Problems

To Do:

- All games : better sound
- Some games: battery backed portion of RAM
- Some games: programmable timer that generates IRQ. See e.g. gundhara:
  lev 4 is triggerd by writes at d00000-6 and drives the sound.
  See also msgundam.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "seta.h"

/* Variables and functions only used here */

static unsigned char *sharedram;


#if __uPD71054_TIMER

#define	USED_TIMER_NUM	1
/*------------------------------
	timer(uPD71054) struct
------------------------------*/
static struct st_chip {
	void			*timer[3];			// Timer
	unsigned short	max[3];				// Max counter
	unsigned short	write_select;		// Max counter write select
	unsigned char	reg[4];				//
} uPD71054;

/*------------------------------
	uppdate timer
------------------------------*/
void uPD71054_update_timer( int no )
{
	int		max = 0;
	double	duration = 0;

	timer_adjust( uPD71054.timer[no], TIME_NEVER, no, 0 );

	max = (int)uPD71054.max[no]&0xffff;

	if( max != 0 ) {
		duration = (double)Machine->drv->cpu[0].cpu_clock/16/max;
	}
	if( duration != 0 ) {
		timer_adjust( uPD71054.timer[no], TIME_IN_HZ(duration), no, 0 );
	} else {
		logerror( "CPU #0 PC %06X: uPD71054 error, timer %d duration is 0\n",
				activecpu_get_pc(), no );
	}
}



/*------------------------------
	callback
------------------------------*/
void uPD71054_timer_callback( int no )
{
	cpu_set_irq_line( 0, 4, HOLD_LINE );
	uPD71054_update_timer( no );
}



/*------------------------------
	initialize
------------------------------*/
void uPD71054_timer_init( void )
{
	int no;

	uPD71054.write_select = 0;

	for( no = 0; no < USED_TIMER_NUM; no++ ) {
		uPD71054.max[no] = 0xffff;
	}
	for( no = 0; no < USED_TIMER_NUM; no++ ) {
		uPD71054.timer[no] = timer_alloc( uPD71054_timer_callback );
	}
}



/*------------------------------
	exit
------------------------------*/
void uPD71054_timer_stop( void )
{
	timer_free();
}



/*------------------------------
	timer write handler
------------------------------*/
WRITE16_HANDLER( timer_regs_w )
{
	data &= 0xff;

	uPD71054.reg[offset] = data;

	switch( offset ) {
	  case 0x0000:
	  case 0x0001:
	  case 0x0002:
		if( uPD71054.write_select == 0 ) {
			uPD71054.max[offset] = (uPD71054.max[offset]&0xff00)+data;
			if( ((uPD71054.reg[3]>>4)&3) == 3 ) {
				uPD71054.write_select = 1;
			}
		} else {
			uPD71054.max[offset] = (uPD71054.max[offset]&0x00ff)+(data<<8);
		}
		if( uPD71054.max[offset] != 0 ) {
			uPD71054_update_timer( offset );
		}
		break;
	  case 0x0003:
		switch( (data>>4)&3 ) {
		  case 2: uPD71054.write_select = 1; break;
		  case 1:
		  case 3: uPD71054.write_select = 0; break;
		}
		break;
	}
}
#endif	// __uPD71054_TIMER




/***************************************************************************


									Sound


***************************************************************************/

static int seta_sh_start_8MHz(const struct MachineSound *msound)
{
#if	__X1_010_V2
	return seta_sh_start( msound, 8000000, 0x0000 );
#else
	return seta_sh_start(msound, 8000000);
#endif	//	__X1_010_V2
}

static int seta_sh_start_16MHz(const struct MachineSound *msound)
{
#if	__X1_010_V2
	return seta_sh_start( msound, 16000000, 0x0000 );
#else
	return seta_sh_start(msound, 16000000);
#endif	//	__X1_010_V2
}

static struct CustomSound_interface seta_sound_intf_8MHz =
{
	seta_sh_start_8MHz,
	0,
	0,
};
static struct CustomSound_interface seta_sound_intf_16MHz =
{
	seta_sh_start_16MHz,
	0,
	0,
};
#if	__X1_010_V2
static int seta_sh_start_16MHz2(const struct MachineSound *msound)
{
	return seta_sh_start( msound, 16000000, 0x1000 );
}
static struct CustomSound_interface seta_sound_intf_16MHz2 =
{
	seta_sh_start_16MHz2,
	0,
	0,
};
#endif	//	__X1_010_V2

/***************************************************************************


								Common Routines


***************************************************************************/

/*

 Mirror RAM

*/
static data16_t *mirror_ram;

READ16_HANDLER( mirror_ram_r )
{
	return mirror_ram[offset];
}

WRITE16_HANDLER( mirror_ram_w )
{
	COMBINE_DATA(&mirror_ram[offset]);
//	logerror("PC %06X - Mirror RAM Written: %04X <- %04X\n", activecpu_get_pc(), offset*2, data);
}



/*

 Shared RAM:

 The 65c02 sees a linear array of bytes that is mapped, for the 68000,
 to a linear array of words whose low order bytes hold the data

*/

static READ16_HANDLER( sharedram_68000_r )
{
	return ((data16_t) sharedram[offset]) & 0xff;
}

static WRITE16_HANDLER( sharedram_68000_w )
{
	if (ACCESSING_LSB)	sharedram[offset] = data & 0xff;
}




/*

 Sub CPU Control

*/

static WRITE16_HANDLER( sub_ctrl_w )
{
	static int old_data = 0;

	switch(offset)
	{
		case 0/2:	// bit 0: reset sub cpu?
			if (ACCESSING_LSB)
			{
				if ( !(old_data&1) && (data&1) )
					cpu_set_reset_line(1,PULSE_LINE);
				old_data = data;
			}
			break;

		case 2/2:	// ?
			break;

		case 4/2:	// not sure
			if (ACCESSING_LSB)	soundlatch_w(0, data & 0xff);
			break;

		case 6/2:	// not sure
			if (ACCESSING_LSB)	soundlatch2_w(0, data & 0xff);
			break;
	}

}


const struct GameDriver driver_blandia;
const struct GameDriver driver_gundhara;
const struct GameDriver driver_kamenrid;
const struct GameDriver driver_zingzip;

/*	---- 3---		Coin #1 Lock Out
	---- -2--		Coin #0 Lock Out
	---- --1-		Coin #1 Counter
	---- ---0		Coin #0 Counter		*/

void seta_coin_lockout_w(int data)
{
	coin_counter_w		(0, (( data) >> 0) & 1 );
	coin_counter_w		(1, (( data) >> 1) & 1 );

	/* blandia, gundhara, kamenrid & zingzip haven't the coin lockout device */
	if (Machine->gamedrv			==	&driver_blandia  ||
		Machine->gamedrv->clone_of	==	&driver_blandia  ||

		Machine->gamedrv			==	&driver_gundhara ||
		Machine->gamedrv->clone_of	==	&driver_gundhara ||

		Machine->gamedrv			==	&driver_kamenrid ||
		Machine->gamedrv->clone_of	==	&driver_kamenrid ||

		Machine->gamedrv			==	&driver_zingzip  ||
		Machine->gamedrv->clone_of	==	&driver_zingzip     )
		return;
	coin_lockout_w		(0, ((~data) >> 2) & 1 );
	coin_lockout_w		(1, ((~data) >> 3) & 1 );
}


/* DSW reading for 16 bit CPUs */
static READ16_HANDLER( seta_dsw_r )
{
	data16_t dsw = readinputport(3);
	if (offset == 0)	return (dsw >> 8) & 0xff;
	else				return (dsw >> 0) & 0xff;
}


/* DSW reading for 8 bit CPUs */

static READ_HANDLER( dsw1_r )
{
	return (readinputport(3) >> 8) & 0xff;
}

static READ_HANDLER( dsw2_r )
{
	return (readinputport(3) >> 0) & 0xff;
}


/*

 Sprites Buffering

*/
VIDEO_EOF( seta_buffer_sprites )
{
	int ctrl2	=	spriteram16[ 0x602/2 ];
	if (~ctrl2 & 0x20)
	{
		if (ctrl2 & 0x40)
			memcpy(&spriteram16_2[0x0000/2],&spriteram16_2[0x2000/2],0x2000/2);
		else
			memcpy(&spriteram16_2[0x2000/2],&spriteram16_2[0x0000/2],0x2000/2);
	}
}


/***************************************************************************


									Main CPU

(for debugging it is useful to be able to peek at some memory regions that
 the game writes to but never reads from. I marked this regions with an empty
 comment to distinguish them, since there's always the possibility that some
 games actually read from this kind of regions, expecting some hardware
 register's value there, instead of the data they wrote)

***************************************************************************/




/***************************************************************************
								Athena no Hatena?
***************************************************************************/

static MEMORY_READ16_START( atehate_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x900000, 0x9fffff, MRA16_RAM				},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_r		},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_r		},	// Sound
	{ 0x100100, 0x103fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
	{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
	{ 0xb00000, 0xb00001, input_port_0_word_r	},	// P1
	{ 0xb00002, 0xb00003, input_port_1_word_r	},	// P2
	{ 0xb00004, 0xb00005, input_port_2_word_r	},	// Coins
/**/{ 0xc00000, 0xc00001, MRA16_RAM				},	// ? 0x4000
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// Sprites Code + X + Attr
MEMORY_END

static MEMORY_WRITE16_START( atehate_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM					},	// ROM
	{ 0x900000, 0x9fffff, MWA16_RAM					},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_w			},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_w			},	// Sound
	{ 0x100100, 0x103fff, MWA16_RAM					},	//
#endif	// __X1_010_V2
	{ 0x200000, 0x200001, MWA16_NOP					},	// ? watchdog ?
	{ 0x300000, 0x300001, MWA16_NOP					},	// ? 0 (irq ack lev 2?)
	{ 0x500000, 0x500001, MWA16_NOP					},	// ? (end of lev 1: bit 4 goes 1,0,1)
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16	},	// Sprites Y
	{ 0xc00000, 0xc00001, MWA16_RAM					},	// ? 0x4000
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2	},	// Sprites Code + X + Attr
MEMORY_END

/***************************************************************************
						Blandia
***************************************************************************/

static MEMORY_READ16_START( blandia_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM				},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM (main ram for zingzip, wrofaero writes to 20f000-20ffff)
	{ 0x210000, 0x21ffff, MRA16_RAM				},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MRA16_RAM				},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, MRA16_RAM				},	// (gundhara)
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, MRA16_RAM				},	// Palette
	{ 0x701000, 0x70ffff, MRA16_RAM				},	//
/**/{ 0x800000, 0x800607, MRA16_RAM				},	// Sprites Y
	{ 0x880000, 0x880001, MRA16_RAM				},	// ? 0xc000
	{ 0x900000, 0x903fff, MRA16_RAM				},	// Sprites Code + X + Attr
/**/{ 0xa00000, 0xa00005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0xa80000, 0xa80005, MRA16_RAM				},	// VRAM 2&3 Ctrl
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// VRAM 0&1
	{ 0xb04000, 0xb0ffff, MRA16_RAM				},	// (jjsquawk)
	{ 0xb80000, 0xb83fff, MRA16_RAM				},	// VRAM 2&3
	{ 0xb84000, 0xb8ffff, MRA16_RAM				},	// (jjsquawk)
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
MEMORY_END

static MEMORY_WRITE16_START( blandia_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x210000, 0x21ffff, MWA16_RAM						},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MWA16_RAM						},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, MWA16_RAM						},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701000, 0x70ffff, MWA16_RAM						},	//
	{ 0x800000, 0x800607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0x880000, 0x880001, MWA16_RAM						},	// ? 0xc000
	{ 0x900000, 0x903fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xa00000, 0xa00005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0xa80000, 0xa80005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xb00000, 0xb01fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0xb02000, 0xb03fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0xb04000, 0xb0ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0xb80000, 0xb81fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0xb82000, 0xb83fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0xb84000, 0xb8ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
	{ 0xd00000, 0xd00007, MWA16_NOP						},	// ?
	{ 0xe00000, 0xe00001, MWA16_NOP						},	// ? VBlank IRQ Ack
	{ 0xf00000, 0xf00001, MWA16_NOP						},	// ? Sound  IRQ Ack
MEMORY_END

#if __X1_010_V2
/***************************************************************************
	Blandia (proto), Gundhara, J.J.Squawkers, Rezon, War of Aero, Zing Zing Zip
						(with slight variations)
***************************************************************************/

static MEMORY_READ16_START( blandiap_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM				},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM (main ram for zingzip, wrofaero writes to 20f000-20ffff)
	{ 0x210000, 0x21ffff, MRA16_RAM				},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MRA16_RAM				},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, MRA16_RAM				},	// (gundhara)
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, MRA16_RAM				},	// Palette
	{ 0x701000, 0x70ffff, MRA16_RAM				},	//
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x804000, 0x80ffff, MRA16_RAM				},	// (jjsquawk)
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
	{ 0x884000, 0x88ffff, MRA16_RAM				},	// (jjsquawk)
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
MEMORY_END

static MEMORY_WRITE16_START( blandiap_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x210000, 0x21ffff, MWA16_RAM						},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MWA16_RAM						},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, MWA16_RAM						},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701000, 0x70ffff, MWA16_RAM						},	//
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x804000, 0x80ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x884000, 0x88ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
	{ 0xd00000, 0xd00007, MWA16_NOP						},	// ?
	{ 0xe00000, 0xe00001, MWA16_NOP						},	// ? VBlank IRQ Ack
	{ 0xf00000, 0xf00001, MWA16_NOP						},	// ? Sound  IRQ Ack
MEMORY_END
#endif	// __X1_010_V2


/***************************************************************************
	Blandia, Gundhara, J.J.Squawkers, Rezon, War of Aero, Zing Zing Zip
						(with slight variations)
***************************************************************************/

static MEMORY_READ16_START( wrofaero_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM				},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM (main ram for zingzip, wrofaero writes to 20f000-20ffff)
	{ 0x210000, 0x21ffff, MRA16_RAM				},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MRA16_RAM				},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, MRA16_RAM				},	// (gundhara)
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, MRA16_RAM				},	// Palette
	{ 0x701000, 0x70ffff, MRA16_RAM				},	//
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x804000, 0x80ffff, MRA16_RAM				},	// (jjsquawk)
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
	{ 0x884000, 0x88ffff, MRA16_RAM				},	// (jjsquawk)
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_r		},	// Sound
	{ 0xc00100, 0xc03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( wrofaero_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM (up to 2MB)
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x210000, 0x21ffff, MWA16_RAM						},	// RAM (gundhara)
	{ 0x300000, 0x30ffff, MWA16_RAM						},	// RAM (wrofaero only?)
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, MWA16_RAM						},	// (rezon,jjsquawk)
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701000, 0x70ffff, MWA16_RAM						},	//
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x804000, 0x80ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x884000, 0x88ffff, MWA16_RAM						},	// (jjsquawk)
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_w				},	// Sound
	{ 0xc00100, 0xc03fff, MWA16_RAM						},	//
#endif	//	__X1_010_V2
#if __uPD71054_TIMER
	{ 0xd00000, 0xd00007, timer_regs_w					},	// ?
#else
	{ 0xd00000, 0xd00007, MWA16_NOP						},	// ?
#endif
	{ 0xe00000, 0xe00001, MWA16_NOP						},	// ? VBlank IRQ Ack
	{ 0xf00000, 0xf00001, MWA16_NOP						},	// ? Sound  IRQ Ack
MEMORY_END



/***************************************************************************
								Block Carnival
***************************************************************************/

/* similar to krzybowl */
static MEMORY_READ16_START( blockcar_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0xf00000, 0xf03fff, MRA16_RAM				},	// RAM
	{ 0xf04000, 0xf041ff, MRA16_RAM				},	// Backup RAM?
	{ 0xf05000, 0xf050ff, MRA16_RAM				},	// Backup RAM?
	{ 0x300000, 0x300003, seta_dsw_r			},	// DSW
	{ 0x500000, 0x500001, input_port_0_word_r	},	// P1
	{ 0x500002, 0x500003, input_port_1_word_r	},	// P2
	{ 0x500004, 0x500005, input_port_2_word_r	},	// Coins
#if	__X1_010_V2
	{ 0xa00000, 0xa03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xa00000, 0xa000ff, seta_sound_word_r		},	// Sound
	{ 0xa00100, 0xa03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0xb00000, 0xb003ff, MRA16_RAM				},	// Palette
	{ 0xc00000, 0xc03fff, MRA16_RAM				},	// Sprites Code + X + Attr
/**/{ 0xd00000, 0xd00001, MRA16_RAM				},	// ? 0x4000
/**/{ 0xe00000, 0xe00607, MRA16_RAM				},	// Sprites Y
MEMORY_END

static MEMORY_WRITE16_START( blockcar_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM					},	// ROM
	{ 0xf00000, 0xf03fff, MWA16_RAM					},	// RAM
	{ 0xf04000, 0xf041ff, MWA16_RAM					},	// Backup RAM?
	{ 0xf05000, 0xf050ff, MWA16_RAM					},	// Backup RAM?
	{ 0x100000, 0x100001, MWA16_NOP					},	// ? 1 (start of interrupts, main loop: watchdog?)
	{ 0x200000, 0x200001, MWA16_NOP					},	// ? 0/1 (IRQ acknowledge?)
	{ 0x400000, 0x400001, seta_vregs_w, &seta_vregs	},	// Coin Lockout + Sound Enable (bit 4?)
#if	__X1_010_V2
	{ 0xa00000, 0xa03fff, seta_sound_word_w			},	// Sound
#else
	{ 0xa00000, 0xa000ff, seta_sound_word_w			},	// Sound
	{ 0xa00100, 0xa03fff, MWA16_RAM					},	//
#endif	// __X1_010_V2
	{ 0xb00000, 0xb003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xc00000, 0xc03fff, MWA16_RAM, &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xd00000, 0xd00001, MWA16_RAM					},	// ? 0x4000
	{ 0xe00000, 0xe00607, MWA16_RAM, &spriteram16	},	// Sprites Y
MEMORY_END



/***************************************************************************
								Caliber 50
***************************************************************************/

READ16_HANDLER ( calibr50_ip_r )
{
	int dir1 = readinputport(4) & 0xfff;	// analog port
	int dir2 = readinputport(5) & 0xfff;	// analog port

	switch (offset)
	{
		case 0x00/2:	return readinputport(0);	// p1
		case 0x02/2:	return readinputport(1);	// p2

		case 0x08/2:	return readinputport(2);	// Coins

		case 0x10/2:	return (dir1&0xff);			// lower 8 bits of p1 rotation
		case 0x12/2:	return (dir1>>8);			// upper 4 bits of p1 rotation
		case 0x14/2:	return (dir2&0xff);			// lower 8 bits of p2 rotation
		case 0x16/2:	return (dir2>>8);			// upper 4 bits of p2 rotation
		case 0x18/2:	return 0xffff;				// ? (value's read but not used)
		default:
			logerror("PC %06X - Read input %02X !\n", activecpu_get_pc(), offset*2);
			return 0;
	}
}

WRITE16_HANDLER( calibr50_soundlatch_w )
{
	soundlatch_word_w(0,data,mem_mask);
	cpu_set_nmi_line(1,PULSE_LINE);
	cpu_spinuntil_time(TIME_IN_USEC(50));	// Allow the other cpu to reply
}

static MEMORY_READ16_START( calibr50_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM				},	// ROM
	{ 0xff0000, 0xffffff, MRA16_RAM				},	// RAM
	{ 0x100000, 0x100007, MRA16_NOP				},	// ? (same as a00010-a00017?)
	{ 0x200000, 0x200fff, MRA16_RAM				},	// NVRAM
	{ 0x300000, 0x300001, MRA16_NOP				},	// ? (value's read but not used)
	{ 0x400000, 0x400001, watchdog_reset16_r	},	// Watchdog
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
/**/{ 0x800000, 0x800005, MRA16_RAM				},	// VRAM Ctrl
	{ 0x900000, 0x901fff, MRA16_RAM				},	// VRAM
	{ 0x902000, 0x903fff, MRA16_RAM				},	// VRAM
	{ 0x904000, 0x904fff, MRA16_RAM				},	//
	{ 0xa00000, 0xa00019, calibr50_ip_r			},	// Input Ports
/**/{ 0xd00000, 0xd00607, MRA16_RAM				},	// Sprites Y
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xb00000, 0xb00001, soundlatch2_word_r	},	// From Sub CPU
/**/{ 0xc00000, 0xc00001, MRA16_RAM				},	// ? $4000
MEMORY_END

static MEMORY_WRITE16_START( calibr50_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM						},	// ROM
	{ 0xff0000, 0xffffff, MWA16_RAM						},	// RAM
	{ 0x200000, 0x200fff, MWA16_RAM						},	// NVRAM
	{ 0x300000, 0x300001, MWA16_NOP						},	// ? (random value)
	{ 0x500000, 0x500001, MWA16_NOP						},	// ?
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x800000, 0x800005, MWA16_RAM, &seta_vctrl_0		},	// VRAM Ctrl
	{ 0x900000, 0x901fff, seta_vram_0_w, &seta_vram_0	},	// VRAM
	{ 0x902000, 0x903fff, seta_vram_1_w, &seta_vram_1	},	// VRAM
	{ 0x904000, 0x904fff, MWA16_RAM						},	//
	{ 0xd00000, 0xd00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xb00000, 0xb00001, calibr50_soundlatch_w			},	// To Sub CPU
	{ 0xc00000, 0xc00001, MWA16_RAM						},	// ? $4000
MEMORY_END

/***************************************************************************
								Daioh
***************************************************************************/

static MEMORY_READ16_START( daioh_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x10ffff, MRA16_RAM				},	// RAM
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x500006, 0x500007, input_port_4_word_r	},	// Buttons 4,5,6
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},
	{ 0x700400, 0x700fff, MRA16_RAM				},  // Palette
	{ 0x701000, 0x70ffff, MRA16_RAM				},
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x804000, 0x80ffff, MRA16_RAM				},	//
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
	{ 0x884000, 0x88ffff, MRA16_RAM				},	//
	{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
	{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb13fff, MRA16_RAM				},	//
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_r		},	// Sound
	{ 0xc00100, 0xc03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( daioh_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x10ffff, MWA16_RAM						},	// RAM
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, MWA16_RAM						},
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701000, 0x70ffff, MWA16_RAM						},	//
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x804000, 0x80ffff, MWA16_RAM						},	//
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x884000, 0x88ffff, MWA16_RAM						},	//
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb13fff, MWA16_RAM						},	//
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_w				},	// Sound
	{ 0xc00100, 0xc03fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
	{ 0xe00000, 0xe00001, MWA16_NOP						},	//
MEMORY_END

/***************************************************************************
				DownTown, Meta Fox, Twin Eagle, Arbalester
			(with slight variations, and protections hooked in)
***************************************************************************/

static MEMORY_READ16_START( downtown_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM				},	// ROM
	{ 0xf00000, 0xffffff, MRA16_RAM				},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_r		},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_r		},	// Sound
	{ 0x100100, 0x103fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
/**/{ 0x800000, 0x800005, MRA16_RAM				},	// VRAM Ctrl
	{ 0x900000, 0x901fff, MRA16_RAM				},	// VRAM
	{ 0x902000, 0x903fff, MRA16_RAM				},	// VRAM
	{ 0xb00000, 0xb00fff, sharedram_68000_r		},	// Shared RAM
/**/{ 0xc00000, 0xc00001, MRA16_RAM				},	// ? $4000
/**/{ 0xd00000, 0xd00607, MRA16_RAM				},	// Sprites Y
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// Sprites Code + X + Attr
MEMORY_END

static MEMORY_WRITE16_START( downtown_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM						},	// ROM
	{ 0xf00000, 0xffffff, MWA16_RAM						},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_w				},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_w				},	// Sound
	{ 0x100100, 0x103fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
	{ 0x500000, 0x500001, MWA16_NOP						},	// ?
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x800000, 0x800005, MWA16_RAM, &seta_vctrl_0		},	// VRAM Ctrl
	{ 0x900000, 0x901fff, seta_vram_0_w, &seta_vram_0	},	// VRAM
	{ 0x902000, 0x903fff, seta_vram_1_w, &seta_vram_1	},	// VRAM
	{ 0xa00000, 0xa00007, sub_ctrl_w					},	// Sub CPU Control?
	{ 0xb00000, 0xb00fff, sharedram_68000_w				},	// Shared RAM
	{ 0xc00000, 0xc00001, MWA16_RAM						},	// ? $4000
	{ 0xd00000, 0xd00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
MEMORY_END


/***************************************************************************
		Dragon Unit, Quiz Kokology, Quiz Kokology 2, Strike Gunner
***************************************************************************/

static MEMORY_READ16_START( drgnunit_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x080000, 0x0bffff, MRA16_RAM				},	// ROM (qzkklgy2)
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	// RAM (qzkklogy)
	{ 0xffc000, 0xffffff, MRA16_RAM				},	// RAM (drgnunit,stg)
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_r		},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_r		},	// Sound
	{ 0x100100, 0x103fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
/**/{ 0x800000, 0x800005, MRA16_RAM				},	// VRAM Ctrl
	{ 0x900000, 0x901fff, MRA16_RAM				},	// VRAM
	{ 0x902000, 0x903fff, MRA16_RAM				},	// VRAM
	{ 0xb00000, 0xb00001, input_port_0_word_r	},	// P1
	{ 0xb00002, 0xb00003, input_port_1_word_r	},	// P2
	{ 0xb00004, 0xb00005, input_port_2_word_r	},	// Coins
	{ 0xb00006, 0xb00007, MRA16_NOP				},	// unused (qzkklogy)
/**/{ 0xc00000, 0xc00001, MRA16_RAM				},	// ? $4000
/**/{ 0xd00000, 0xd00607, MRA16_RAM				},	// Sprites Y
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// Sprites Code + X + Attr
MEMORY_END

static MEMORY_WRITE16_START( drgnunit_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x080000, 0x0bffff, MWA16_RAM						},	// ROM (qzkklgy2)
	{ 0xf00000, 0xf0ffff, MWA16_RAM						},	// RAM (qzkklogy)
	{ 0xffc000, 0xffffff, MWA16_RAM						},	// RAM (drgnunit,stg)
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_w				},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_w				},	// Sound
	{ 0x100100, 0x103fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
	{ 0x200000, 0x200001, MWA16_NOP						},	// Watchdog
	{ 0x300000, 0x300001, MWA16_NOP						},	// ? IRQ Ack
	{ 0x500000, 0x500001, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x800000, 0x800005, MWA16_RAM, &seta_vctrl_0		},	// VRAM Ctrl
	{ 0x900000, 0x901fff, seta_vram_0_w, &seta_vram_0	},	// VRAM
	{ 0x902000, 0x903fff, seta_vram_1_w, &seta_vram_1	},	// VRAM
	{ 0x904000, 0x90ffff, MWA16_NOP						},	// unused (qzkklogy)
	{ 0xc00000, 0xc00001, MWA16_RAM						},	// ? $4000
	{ 0xd00000, 0xd00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
MEMORY_END


/***************************************************************************
						Extreme Downhill / Sokonuke
***************************************************************************/

static MEMORY_READ16_START( extdwnhl_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x210000, 0x21ffff, MRA16_RAM				},	// RAM
	{ 0x220000, 0x23ffff, MRA16_RAM				},	// RAM (sokonuke)
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x400008, 0x40000b, seta_dsw_r			},	// DSW
	{ 0x40000c, 0x40000d, watchdog_reset16_r	},	// Watchdog (extdwnhl, MUST RETURN $FFFF)
	{ 0x500004, 0x500007, MRA16_NOP				},	// IRQ Ack  (extdwnhl)
	{ 0x600400, 0x600fff, MRA16_RAM				},	// Palette
	{ 0x601000, 0x610bff, MRA16_RAM				},	//
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x804000, 0x80ffff, MRA16_RAM				},	//
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
	{ 0x884000, 0x88ffff, MRA16_RAM				},	//
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb13fff, MRA16_RAM				},	//
#if	__X1_010_V2
	{ 0xe00000, 0xe03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xe00000, 0xe000ff, seta_sound_word_r		},	// Sound
	{ 0xe00100, 0xe03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( extdwnhl_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x210000, 0x21ffff, MWA16_RAM						},	// RAM
	{ 0x220000, 0x23ffff, MWA16_RAM						},	// RAM (sokonuke)
	{ 0x40000c, 0x40000d, watchdog_reset16_w			},	// Watchdog (sokonuke)
	{ 0x500000, 0x500003, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x500004, 0x500007, MWA16_NOP						},	// IRQ Ack (sokonuke)
	{ 0x600400, 0x600fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x601000, 0x610bff, MWA16_RAM						},	//
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x804000, 0x80ffff, MWA16_RAM						},	//
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x884000, 0x88ffff, MWA16_RAM						},	//
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb13fff, MWA16_RAM						},	//
#if	__X1_010_V2
	{ 0xe00000, 0xe000ff, seta_sound_word_w				},	// Sound
#else
	{ 0xe00000, 0xe000ff, seta_sound_word_w				},	// Sound
	{ 0xe00100, 0xe03fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
MEMORY_END

/***************************************************************************
				(Kamen) Masked Riders Club Battle Race
***************************************************************************/

static MEMORY_READ16_START( kamenrid_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// ROM
	{ 0x500000, 0x500001, input_port_0_word_r	},	// P1
	{ 0x500002, 0x500003, input_port_1_word_r	},	// P2
	{ 0x500004, 0x500007, seta_dsw_r		},	// DSW
	{ 0x500008, 0x500009, input_port_2_word_r	},	// Coins
	{ 0x50000c, 0x50000d, watchdog_reset16_r	},	// xx Watchdog?
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
	{ 0x700400, 0x700fff, MRA16_RAM				},	// Palette
	{ 0x701000, 0x703fff, MRA16_RAM				},	// Palette
	{ 0x800000, 0x801fff, MRA16_RAM				},	// VRAM 0
	{ 0x802000, 0x803fff, MRA16_RAM				},	// VRAM 1
	{ 0x804000, 0x807fff, MRA16_RAM				},	// tested
	{ 0x880000, 0x881fff, MRA16_RAM				},	// VRAM 2
	{ 0x882000, 0x883fff, MRA16_RAM				},	// VRAM 3
	{ 0x884000, 0x887fff, MRA16_RAM				},	// tested
	{ 0xa00000, 0xa00607, MRA16_RAM			 	},	// Sprites Y
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb07fff, MRA16_RAM				},	// tested
#if	__X1_010_V2
	{ 0xd00000, 0xd03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xd00000, 0xd000ff, seta_sound_word_r		},	// Sound
	{ 0xd00100, 0xd03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( kamenrid_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// ROM
	{ 0x50000c, 0x50000d, watchdog_reset16_w			},	// Watchdog (sokonuke)
	{ 0x600000, 0x600005, seta_vregs_w, &seta_vregs		},	// ? Coin Lockout + Video Registers
	{ 0x700000, 0x7003ff, MWA16_RAM						},	// Palette RAM (tested)
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x701000, 0x703fff, MWA16_RAM	},	// Palette
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x804000, 0x807fff, MWA16_RAM	},	// tested
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x884000, 0x887fff, MWA16_RAM	},	// tested
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM , &spriteram16		},	// Sprites Y
	{ 0xb00000, 0xb03fff, MWA16_RAM , &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xb04000, 0xb07fff, MWA16_RAM },	// tested
#if	__X1_010_V2
	{ 0xd00000, 0xd000ff, seta_sound_word_w				},	// Sound
#else
	{ 0xd00000, 0xd000ff, seta_sound_word_w				},	// Sound
	{ 0xd00100, 0xd03fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
MEMORY_END



/***************************************************************************
								Krazy Bowl
***************************************************************************/

static READ16_HANDLER( krzybowl_input_r )
{
	// analog ports
	int dir1x = readinputport(4) & 0xfff;
	int dir1y = readinputport(5) & 0xfff;
	int dir2x = readinputport(6) & 0xfff;
	int dir2y = readinputport(7) & 0xfff;

	switch (offset)
	{
		case 0x0/2:	return dir1x & 0xff;
		case 0x2/2:	return dir1x >> 8;
		case 0x4/2:	return dir1y & 0xff;
		case 0x6/2:	return dir1y >> 8;
		case 0x8/2:	return dir2x & 0xff;
		case 0xa/2:	return dir2x >> 8;
		case 0xc/2:	return dir2y & 0xff;
		case 0xe/2:	return dir2y >> 8;
		default:
			logerror("PC %06X - Read input %02X !\n", activecpu_get_pc(), offset*2);
			return 0;
	}
}

static MEMORY_READ16_START( krzybowl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	// RAM
	{ 0x100000, 0x100001, MRA16_NOP				},	// ?
	{ 0x200000, 0x200001, MRA16_NOP				},	// ?
	{ 0x300000, 0x300003, seta_dsw_r			},	// DSW
	{ 0x500000, 0x500001, input_port_0_word_r	},	// P1
	{ 0x500002, 0x500003, input_port_1_word_r	},	// P2
	{ 0x500004, 0x500005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x60000f, krzybowl_input_r		},	// P1
	{ 0x8000f0, 0x8000f1, MRA16_RAM				},	// NVRAM
	{ 0x800100, 0x8001ff, MRA16_RAM				},	// NVRAM
#if	__X1_010_V2
	{ 0xa00000, 0xa03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xa00000, 0xa000ff, seta_sound_word_r		},	// Sound
	{ 0xa00100, 0xa03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0xb00000, 0xb003ff, MRA16_RAM				},	// Palette
	{ 0xc00000, 0xc03fff, MRA16_RAM				},	// Sprites Code + X + Attr
/**/{ 0xd00000, 0xd00001, MRA16_RAM				},	// ? 0x4000
/**/{ 0xe00000, 0xe00607, MRA16_RAM				},	// Sprites Y
MEMORY_END

static MEMORY_WRITE16_START( krzybowl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM					},	// ROM
	{ 0xf00000, 0xf0ffff, MWA16_RAM					},	// RAM
	{ 0x400000, 0x400001, MWA16_NOP					},	// ?
	{ 0x8000f0, 0x8000f1, MWA16_RAM					},	// NVRAM
	{ 0x800100, 0x8001ff, MWA16_RAM					},	// NVRAM
#if	__X1_010_V2
	{ 0xa00000, 0xa03fff, seta_sound_word_w			},	// Sound
#else
	{ 0xa00000, 0xa000ff, seta_sound_word_w			},	// Sound
	{ 0xa00100, 0xa03fff, MWA16_RAM					},	//
#endif	// __X1_010_V2
	{ 0xb00000, 0xb003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xc00000, 0xc03fff, MWA16_RAM, &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xd00000, 0xd00001, MWA16_RAM					},	// ? 0x4000
	{ 0xe00000, 0xe00607, MWA16_RAM, &spriteram16	},	// Sprites Y
MEMORY_END


/***************************************************************************
							Mobile Suit Gundam
***************************************************************************/

WRITE16_HANDLER( msgundam_vregs_w )
{
	// swap $500002 with $500004
	switch( offset )
	{
		case 1:	offset = 2;	break;
		case 2:	offset = 1;	break;
	}
	seta_vregs_w(offset,data,mem_mask);
}

/* Mirror RAM is necessary or startup, to clear Work RAM after the test */

static MEMORY_READ16_START( msgundam_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x1fffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700400, 0x700fff, MRA16_RAM				},	// Palette
	{ 0x800000, 0x800607, MRA16_RAM				},	// Sprites Y
	{ 0x900000, 0x903fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xa00000, 0xa03fff, MRA16_RAM				},	// VRAM 0&1
	{ 0xa80000, 0xa83fff, MRA16_RAM				},	// VRAM 2&3
	{ 0xb00000, 0xb00005, MRA16_RAM				},	// VRAM 0&1 Ctrl
	{ 0xb80000, 0xb80005, MRA16_RAM				},	// VRAM 1&2 Ctrl
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_r		},	// Sound
	{ 0xc00100, 0xc03fff, MRA16_RAM				},	//
#endif	//	__X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( msgundam_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x1fffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM, &mirror_ram		},	// RAM
	{ 0x210000, 0x21ffff, mirror_ram_w					},	// Mirrored RAM
	{ 0x220000, 0x22ffff, mirror_ram_w					},
	{ 0x230000, 0x23ffff, mirror_ram_w					},
	{ 0x240000, 0x24ffff, mirror_ram_w					},
	{ 0x400000, 0x400001, MWA16_NOP						},	// Lev 2 IRQ Ack
	{ 0x400004, 0x400005, MWA16_NOP						},	// Lev 4 IRQ Ack
	{ 0x500000, 0x500005, msgundam_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x700400, 0x700fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x800000, 0x800607, MWA16_RAM , &spriteram16		},	// Sprites Y
	{ 0x880000, 0x880001, MWA16_RAM						},	// ? 0x4000
	{ 0x900000, 0x903fff, MWA16_RAM , &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xa00000, 0xa01fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0xa02000, 0xa03fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0xa80000, 0xa81fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0xa82000, 0xa83fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0xb00000, 0xb00005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0xb80000, 0xb80005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_w				},	// Sound
	{ 0xc00100, 0xc03fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
#if __uPD71054_TIMER
	{ 0xd00000, 0xd00007, timer_regs_w					},	// ?
#else
	{ 0xd00000, 0xd00007, MWA16_NOP						},	// ?
#endif
MEMORY_END




/***************************************************************************
								Oishii Puzzle
***************************************************************************/

/* similar to wrofaero */

static MEMORY_READ16_START( oisipuzl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x17ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x300000, 0x300003, seta_dsw_r			},	// DSW
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
#if	__X1_010_V2
	{ 0x700000, 0x703fff, seta_sound_word_r		},	// Sound
#else
	{ 0x700000, 0x7000ff, seta_sound_word_r		},	// Sound
	{ 0x700100, 0x703fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xc00400, 0xc00fff, MRA16_RAM				},	// Palette
MEMORY_END

static MEMORY_WRITE16_START( oisipuzl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x17ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x400000, 0x400001, MWA16_NOP						},	// ? IRQ Ack
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
#if	__X1_010_V2
	{ 0x700000, 0x703fff, seta_sound_word_w				},	// Sound
#else
	{ 0x700000, 0x7000ff, seta_sound_word_w				},	// Sound
	{ 0x700100, 0x703fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xc00400, 0xc00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
MEMORY_END

/***************************************************************************
								Triple Fun
***************************************************************************/

/* the same as oisipuzl with the sound system replaced */

static MEMORY_READ16_START( triplfun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x17ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x300000, 0x300003, seta_dsw_r			},	// DSW
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x500006, 0x500007, OKIM6295_status_0_lsb_r }, // tfun sound
	{ 0x700000, 0x703fff, MRA16_RAM				},	// old sound
	{ 0x800000, 0x803fff, MRA16_RAM				},	// VRAM 0&1
	{ 0x880000, 0x883fff, MRA16_RAM				},	// VRAM 2&3
/**/{ 0x900000, 0x900005, MRA16_RAM				},	// VRAM 0&1 Ctrl
/**/{ 0x980000, 0x980005, MRA16_RAM				},	// VRAM 2&3 Ctrl
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xc00400, 0xc00fff, MRA16_RAM				},	// Palette
MEMORY_END

static MEMORY_WRITE16_START( triplfun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x100000, 0x17ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0x400000, 0x400001, MWA16_NOP						},	// ? IRQ Ack
	{ 0x500000, 0x500005, seta_vregs_w, &seta_vregs		},	// Coin Lockout + Video Registers
	{ 0x500006, 0x500007, OKIM6295_data_0_lsb_w         },  // tfun sound
	{ 0x700000, 0x703fff, MWA16_RAM						},	// old sound
	{ 0x800000, 0x801fff, seta_vram_0_w, &seta_vram_0	},	// VRAM 0
	{ 0x802000, 0x803fff, seta_vram_1_w, &seta_vram_1	},	// VRAM 1
	{ 0x880000, 0x881fff, seta_vram_2_w, &seta_vram_2	},	// VRAM 2
	{ 0x882000, 0x883fff, seta_vram_3_w, &seta_vram_3	},	// VRAM 3
	{ 0x900000, 0x900005, MWA16_RAM, &seta_vctrl_0		},	// VRAM 0&1 Ctrl
	{ 0x980000, 0x980005, MWA16_RAM, &seta_vctrl_2		},	// VRAM 2&3 Ctrl
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM						},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xc00400, 0xc00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
MEMORY_END

/***************************************************************************
							Pro Mahjong Kiwame
***************************************************************************/

data16_t *kiwame_nvram;

READ16_HANDLER( kiwame_nvram_r )
{
	return kiwame_nvram[offset] & 0xff;
}

WRITE16_HANDLER( kiwame_nvram_w )
{
	if (ACCESSING_LSB)	COMBINE_DATA( &kiwame_nvram[offset] );
}

READ16_HANDLER( kiwame_input_r )
{
	int row_select = kiwame_nvram_r( 0x10a/2,0 ) & 0x1f;
	int i;

	for(i = 0; i < 5; i++)
		if (row_select & (1<<i))	break;

	i = 4 + (i % 5);

	switch( offset )
	{
		case 0x00/2:	return readinputport( i );
		case 0x02/2:	return 0xffff;
		case 0x04/2:	return readinputport( 2 );
//		case 0x06/2:
		case 0x08/2:	return 0xffff;

		default:
			logerror("PC %06X - Read input %02X !\n", activecpu_get_pc(), offset*2);
			return 0x0000;
	}
}

static MEMORY_READ16_START( kiwame_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0xfffc00, 0xffffff, kiwame_nvram_r		},	// NVRAM + Regs ?
	{ 0x800000, 0x803fff, MRA16_RAM				},	// Sprites Code + X + Attr
/**/{ 0x900000, 0x900001, MRA16_RAM				},	// ? 0x4000
/**/{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
	{ 0xb00000, 0xb003ff, MRA16_RAM				},	// Palette
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_r		},	// Sound
	{ 0xc00100, 0xc03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0xd00000, 0xd00009, kiwame_input_r		},
	{ 0xe00000, 0xe00003, seta_dsw_r			},	// DSW
MEMORY_END

static MEMORY_WRITE16_START( kiwame_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM						},	// RAM
	{ 0xfffc00, 0xffffff, kiwame_nvram_w, &kiwame_nvram	},	// NVRAM + Regs ?
	{ 0x800000, 0x803fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0x900000, 0x900001, MWA16_RAM						},	// ? 0x4000
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0xb00000, 0xb003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_w				},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_w				},	// Sound
	{ 0xc00100, 0xc03fff, MWA16_RAM						},	//
#endif	// __X1_010_V2
MEMORY_END


/***************************************************************************
						Thunder & Lightning / Wit's
***************************************************************************/

static READ16_HANDLER( thunderl_protection_r )
{
//	logerror("PC %06X - Protection Read\n", activecpu_get_pc());
	return 0x00dd;
}
static WRITE16_HANDLER( thunderl_protection_w )
{
//	logerror("PC %06X - Protection Written: %04X <- %04X\n", activecpu_get_pc(), offset*2, data);
}

/* Similar to downtown etc. */

static MEMORY_READ16_START( thunderl_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM				},	// ROM
	{ 0xffc000, 0xffffff, MRA16_RAM				},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_r		},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_r		},	// Sound
	{ 0x100100, 0x10ffff, MRA16_RAM				},	//
#endif	// __X1_010_V2
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0x700000, 0x7003ff, MRA16_RAM				},	// Palette
	{ 0xb00000, 0xb00001, input_port_0_word_r	},	// P1
	{ 0xb00002, 0xb00003, input_port_1_word_r	},	// P2
	{ 0xb00004, 0xb00005, input_port_2_word_r	},	// Coins
	{ 0xb0000c, 0xb0000d, thunderl_protection_r	},	// Protection (not in wits)
	{ 0xb00008, 0xb00009, input_port_4_word_r	},	// P3 (wits)
	{ 0xb0000a, 0xb0000b, input_port_5_word_r	},	// P4 (wits)
/**/{ 0xc00000, 0xc00001, MRA16_RAM				},	// ? 0x4000
/**/{ 0xd00000, 0xd00607, MRA16_RAM				},	// Sprites Y
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xe04000, 0xe07fff, MRA16_RAM				},	// (wits)
MEMORY_END

static MEMORY_WRITE16_START( thunderl_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM					},	// ROM
	{ 0xffc000, 0xffffff, MWA16_RAM					},	// RAM
#if	__X1_010_V2
	{ 0x100000, 0x103fff, seta_sound_word_w			},	// Sound
#else
	{ 0x100000, 0x1000ff, seta_sound_word_w			},	// Sound
	{ 0x100100, 0x10ffff, MWA16_RAM					},	//
#endif	// __X1_010_V2
	{ 0x200000, 0x200001, MWA16_NOP					},	// ?
	{ 0x300000, 0x300001, MWA16_NOP					},	// ?
	{ 0x400000, 0x40ffff, thunderl_protection_w		},	// Protection (not in wits)
	{ 0x500000, 0x500001, seta_vregs_w, &seta_vregs	},	// Coin Lockout
	{ 0x700000, 0x7003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xc00000, 0xc00001, MWA16_RAM					},	// ? 0x4000
	{ 0xd00000, 0xd00607, MWA16_RAM, &spriteram16	},	// Sprites Y
	{ 0xe00000, 0xe03fff, MWA16_RAM, &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xe04000, 0xe07fff, MWA16_RAM					},	// (wits)
MEMORY_END


/***************************************************************************
								Thundercade
***************************************************************************/

/* Mirror RAM seems necessary since the e00000-e03fff area is not cleared
   on startup. Level 2 int uses $e0000a as a counter that controls when
   to write a value to the sub cpu, and when to read the result back.
   If the check fails "error x0-006" is displayed. Hence if the counter
   is not cleared at startup the game could check for the result before
   writing to sharedram! */


static MEMORY_READ16_START( tndrcade_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x380000, 0x3803ff, MRA16_RAM				},	// Palette
/**/{ 0x400000, 0x400001, MRA16_RAM				},	// ? $4000
/**/{ 0x600000, 0x600607, MRA16_RAM				},	// Sprites Y
	{ 0xa00000, 0xa00fff, sharedram_68000_r		},	// Shared RAM
	{ 0xc00000, 0xc03fff, MRA16_RAM				},	// Sprites Code + X + Attr
	{ 0xe00000, 0xe03fff, MRA16_RAM				},	// RAM (Mirrored?)
	{ 0xffc000, 0xffffff, mirror_ram_r			},	// RAM (Mirrored?)
MEMORY_END

static MEMORY_WRITE16_START( tndrcade_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x200000, 0x200001, MWA16_NOP						},	// ? 0
	{ 0x280000, 0x280001, MWA16_NOP						},	// ? 0 / 1 (sub cpu related?)
	{ 0x300000, 0x300001, MWA16_NOP						},	// ? 0 / 1
	{ 0x380000, 0x3803ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x400000, 0x400001, MWA16_RAM						},	// ? $4000
	{ 0x600000, 0x600607, MWA16_RAM, &spriteram16		},	// Sprites Y
	{ 0x800000, 0x800007, sub_ctrl_w					},	// Sub CPU Control?
	{ 0xa00000, 0xa00fff, sharedram_68000_w				},	// Shared RAM
	{ 0xc00000, 0xc03fff, MWA16_RAM, &spriteram16_2		},	// Sprites Code + X + Attr
	{ 0xe00000, 0xe03fff, MWA16_RAM, &mirror_ram		},	// RAM (Mirrored?)
	{ 0xffc000, 0xffffff, mirror_ram_w					},	// RAM (Mirrored?)
MEMORY_END




/***************************************************************************
								Ultraman Club
***************************************************************************/

static MEMORY_READ16_START( umanclub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x200000, 0x20ffff, MRA16_RAM				},	// RAM
	{ 0x300000, 0x3003ff, MRA16_RAM				},	// Palette
	{ 0x300400, 0x300fff, MRA16_RAM				},	//
	{ 0x400000, 0x400001, input_port_0_word_r	},	// P1
	{ 0x400002, 0x400003, input_port_1_word_r	},	// P2
	{ 0x400004, 0x400005, input_port_2_word_r	},	// Coins
	{ 0x600000, 0x600003, seta_dsw_r			},	// DSW
	{ 0xa00000, 0xa00607, MRA16_RAM				},	// Sprites Y
/**/{ 0xa80000, 0xa80001, MRA16_RAM				},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MRA16_RAM				},	// Sprites Code + X + Attr
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_r		},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_r		},	// Sound
	{ 0xc00100, 0xc03fff, MRA16_RAM				},	//
#endif	// __X1_010_V2
MEMORY_END

static MEMORY_WRITE16_START( umanclub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM					},	// ROM
	{ 0x200000, 0x20ffff, MWA16_RAM					},	// RAM
	{ 0x300000, 0x3003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0x300400, 0x300fff, MWA16_RAM					},	//
	{ 0x400000, 0x400001, MWA16_NOP					},	// ? (end of lev 2)
	{ 0x400004, 0x400005, MWA16_NOP					},	// ? (end of lev 2)
	{ 0x500000, 0x500001, seta_vregs_w, &seta_vregs	},	// Coin Lockout + Video Registers
	{ 0xa00000, 0xa00607, MWA16_RAM, &spriteram16	},	// Sprites Y
	{ 0xa80000, 0xa80001, MWA16_RAM					},	// ? 0x4000
	{ 0xb00000, 0xb03fff, MWA16_RAM, &spriteram16_2	},	// Sprites Code + X + Attr
#if	__X1_010_V2
	{ 0xc00000, 0xc03fff, seta_sound_word_w			},	// Sound
#else
	{ 0xc00000, 0xc000ff, seta_sound_word_w			},	// Sound
	{ 0xc00100, 0xc03fff, MWA16_RAM					},	//
#endif	// __X1_010_V2
MEMORY_END




/***************************************************************************
								U.S. Classic
***************************************************************************/

READ16_HANDLER( usclssic_dsw_r )
{
	switch (offset)
	{
		case 0/2:	return (readinputport(3) >>  8) & 0xf;
		case 2/2:	return (readinputport(3) >> 12) & 0xf;
		case 4/2:	return (readinputport(3) >>  0) & 0xf;
		case 6/2:	return (readinputport(3) >>  4) & 0xf;
	}
	return 0;
}

READ16_HANDLER( usclssic_trackball_x_r )
{
	switch (offset)
	{
		case 0/2:	return (readinputport(0) >> 0) & 0xff;
		case 2/2:	return (readinputport(0) >> 8) & 0xff;
	}
	return 0;
}

READ16_HANDLER( usclssic_trackball_y_r )
{
	switch (offset)
	{
		case 0/2:	return (readinputport(1) >> 0) & 0xff;
		case 2/2:	return (readinputport(1) >> 8) & 0xff;
	}
	return 0;
}


WRITE16_HANDLER( usclssic_lockout_w )
{
	static int old_tiles_offset = 0;

	if (ACCESSING_LSB)
	{
		seta_tiles_offset = (data & 0x10) ? 0x4000: 0;
		if (old_tiles_offset != seta_tiles_offset)	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		old_tiles_offset = seta_tiles_offset;

		seta_coin_lockout_w(data);
	}
}


static MEMORY_READ16_START( usclssic_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	// ROM
	{ 0xff0000, 0xffffff, MRA16_RAM					},	// RAM
	{ 0x800000, 0x800607, MRA16_RAM					},	// Sprites Y
/**/{ 0x900000, 0x900001, MRA16_RAM					},	// ?
	{ 0xa00000, 0xa00005, MRA16_RAM					},	// VRAM Ctrl
/**/{ 0xb00000, 0xb003ff, MRA16_RAM					},	// Palette
	{ 0xb40000, 0xb40003, usclssic_trackball_x_r	},	// TrackBall X
	{ 0xb40004, 0xb40007, usclssic_trackball_y_r	},	// TrackBall Y + Buttons
	{ 0xb40010, 0xb40011, input_port_2_word_r		},	// Coins
	{ 0xb40018, 0xb4001f, usclssic_dsw_r			},	// 2 DSWs
	{ 0xb80000, 0xb80001, MRA16_NOP					},	// watchdog (value is discarded)?
	{ 0xc00000, 0xc03fff, MRA16_RAM					},	// Sprites Code + X + Attr
	{ 0xd00000, 0xd01fff, MRA16_RAM					},	// VRAM
	{ 0xd02000, 0xd03fff, MRA16_RAM					},	// VRAM
	{ 0xd04000, 0xd04fff, MRA16_RAM					},	//
	{ 0xe00000, 0xe00fff, MRA16_RAM					},	// NVRAM? (odd bytes)
MEMORY_END

static MEMORY_WRITE16_START( usclssic_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0xff0000, 0xffffff, MWA16_RAM						},	// RAM
	{ 0x800000, 0x800607, MWA16_RAM , &spriteram16		},	// Sprites Y
	{ 0x900000, 0x900001, MWA16_RAM						},	// ? $4000
	{ 0xa00000, 0xa00005, MWA16_RAM, &seta_vctrl_0		},	// VRAM Ctrl
	{ 0xb00000, 0xb003ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	// Palette
	{ 0xb40000, 0xb40001, usclssic_lockout_w			},	// Coin Lockout + Tiles Banking
	{ 0xb40010, 0xb40011, calibr50_soundlatch_w			},	// To Sub CPU
	{ 0xb40018, 0xb40019, watchdog_reset16_w			},	// Watchdog
	{ 0xb4000a, 0xb4000b, MWA16_NOP						},	// ? (value's not important. In lev2&6)
	{ 0xc00000, 0xc03fff, MWA16_RAM , &spriteram16_2	},	// Sprites Code + X + Attr
	{ 0xd00000, 0xd01fff, seta_vram_0_w, &seta_vram_0	},	// VRAM
	{ 0xd02000, 0xd03fff, seta_vram_1_w, &seta_vram_1	},	// VRAM
	{ 0xd04000, 0xd04fff, MWA16_RAM						},	//
	{ 0xe00000, 0xe00fff, MWA16_RAM						},	// NVRAM? (odd bytes)
MEMORY_END




/***************************************************************************


									Sub CPU


***************************************************************************/

static WRITE_HANDLER( sub_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bank = data >> 4;

	seta_coin_lockout_w(data);

	cpu_setbank(1, &RAM[ bank * 0x4000 + 0xc000 ]);
}


/***************************************************************************
								Caliber 50
***************************************************************************/

WRITE_HANDLER( calibr50_soundlatch2_w )
{
	soundlatch2_w(0,data);
	cpu_spinuntil_time(TIME_IN_USEC(50));	// Allow the other cpu to reply
}

static MEMORY_READ_START( calibr50_sub_readmem )
#if	__X1_010_V2
	{ 0x0000, 0x1fff, seta_sound_r		},	// Sound
#else
	{ 0x0000, 0x0fff, MRA_RAM			},	// RAM
	{ 0x1000, 0x107f, seta_sound_r		},	// Sound
	{ 0x1080, 0x1fff, MRA_RAM			},	// RAM
#endif	// __X1_010_V2
	{ 0x4000, 0x4000, soundlatch_r		},	// From Main CPU
	{ 0x8000, 0xbfff, MRA_BANK1			},	// Banked ROM
	{ 0xc000, 0xffff, MRA_ROM			},	// ROM
MEMORY_END

static MEMORY_WRITE_START( calibr50_sub_writemem )
#if	__X1_010_V2
	{ 0x0000, 0x1fff, seta_sound_w				},	// Sound
#else
	{ 0x0000, 0x0fff, MWA_RAM					},	// RAM
	{ 0x1000, 0x107f, seta_sound_w				},	// Sound
	{ 0x1080, 0x1fff, MWA_RAM					},	//
#endif	// __X1_010_V2
	{ 0x4000, 0x4000, sub_bankswitch_w			},	// Bankswitching
	{ 0x8000, 0xbfff, MWA_ROM					},	// Banked ROM
	{ 0xc000, 0xc000, calibr50_soundlatch2_w	},	// To Main CPU
	{ 0xc001, 0xffff, MWA_ROM					},	// ROM
MEMORY_END



/***************************************************************************
								DownTown
***************************************************************************/

READ_HANDLER( downtown_ip_r )
{
	int dir1 = readinputport(4);	// analog port
	int dir2 = readinputport(5);	// analog port

	dir1 = (~ (0x800 >> ((dir1 * 12)/0x100)) ) & 0xfff;
	dir2 = (~ (0x800 >> ((dir2 * 12)/0x100)) ) & 0xfff;

	switch (offset)
	{
		case 0:	return (readinputport(2) & 0xf0) + (dir1>>8);	// upper 4 bits of p1 rotation + coins
		case 1:	return (dir1&0xff);			// lower 8 bits of p1 rotation
		case 2:	return readinputport(0);	// p1
		case 3:	return 0xff;				// ?
		case 4:	return (dir2>>8);			// upper 4 bits of p2 rotation + ?
		case 5:	return (dir2&0xff);			// lower 8 bits of p2 rotation
		case 6:	return readinputport(1);	// p2
		case 7:	return 0xff;				// ?
	}

	return 0;
}

static MEMORY_READ_START( downtown_sub_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// RAM
	{ 0x0800, 0x0800, soundlatch_r		},	//
	{ 0x0801, 0x0801, soundlatch2_r		},	//
	{ 0x1000, 0x1007, downtown_ip_r		},	// Input Ports
	{ 0x5000, 0x57ff, MRA_RAM			},	// Shared RAM
	{ 0x7000, 0x7fff, MRA_ROM			},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1			},	// Banked ROM
	{ 0xc000, 0xffff, MRA_ROM			},	// ROM
MEMORY_END

static MEMORY_WRITE_START( downtown_sub_writemem )
	{ 0x0000, 0x01ff, MWA_RAM				},	// RAM
	{ 0x1000, 0x1000, sub_bankswitch_w		},	// ROM Bank + Coin Lockout
	{ 0x5000, 0x57ff, MWA_RAM, &sharedram	},	// Shared RAM
	{ 0x7000, 0xffff, MWA_ROM				},	// ROM
MEMORY_END




/***************************************************************************
								Meta Fox
***************************************************************************/

static MEMORY_READ_START( metafox_sub_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// RAM
	{ 0x0800, 0x0800, soundlatch_r		},	//
	{ 0x0801, 0x0801, soundlatch2_r		},	//
	{ 0x1000, 0x1000, input_port_2_r	},	// Coins
	{ 0x1002, 0x1002, input_port_0_r	},	// P1
//	{ 0x1004, 0x1004, MRA_NOP			},	// ?
	{ 0x1006, 0x1006, input_port_1_r	},	// P2
	{ 0x5000, 0x57ff, MRA_RAM			},	// Shared RAM
	{ 0x7000, 0x7fff, MRA_ROM			},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1			},	// Banked ROM
	{ 0xc000, 0xffff, MRA_ROM			},	// ROM
MEMORY_END

static MEMORY_WRITE_START( metafox_sub_writemem )
	{ 0x0000, 0x01ff, MWA_RAM				},	// RAM
	{ 0x1000, 0x1000, sub_bankswitch_w		},	// ROM Bank + Coin Lockout
	{ 0x5000, 0x57ff, MWA_RAM, &sharedram	},	// Shared RAM
	{ 0x7000, 0x7fff, MWA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM				},	// ROM
	{ 0xc000, 0xffff, MWA_ROM				},	// ROM
MEMORY_END


/***************************************************************************
								Twin Eagle
***************************************************************************/

static MEMORY_READ_START( twineagl_sub_readmem )
	{ 0x0000, 0x01ff, MRA_RAM			},	// RAM
	{ 0x0800, 0x0800, soundlatch_r		},	//
	{ 0x0801, 0x0801, soundlatch2_r		},	//
	{ 0x1000, 0x1000, input_port_0_r	},	// P1
	{ 0x1001, 0x1001, input_port_1_r	},	// P2
	{ 0x1002, 0x1002, input_port_2_r	},	// Coins
	{ 0x5000, 0x57ff, MRA_RAM			},	// Shared RAM
	{ 0x7000, 0x7fff, MRA_ROM			},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1			},	// Banked ROM
	{ 0xc000, 0xffff, MRA_ROM			},	// ROM
MEMORY_END

static MEMORY_WRITE_START( twineagl_sub_writemem )
	{ 0x0000, 0x01ff, MWA_RAM				},	// RAM
	{ 0x1000, 0x1000, sub_bankswitch_w		},	// ROM Bank + Coin Lockout
	{ 0x5000, 0x57ff, MWA_RAM, &sharedram	},	// Shared RAM
	{ 0x7000, 0x7fff, MWA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM				},	// ROM
	{ 0xc000, 0xffff, MWA_ROM				},	// ROM
MEMORY_END



/***************************************************************************
								Thundercade
***************************************************************************/

static READ_HANDLER( ff_r )	{return 0xff;}

static MEMORY_READ_START( tndrcade_sub_readmem )
	{ 0x0000, 0x01ff, MRA_RAM				},	// RAM
	{ 0x0800, 0x0800, ff_r					},	// ? (bits 0/1/2/3: 1 -> do test 0-ff/100-1e0/5001-57ff/banked rom)
//	{ 0x0800, 0x0800, soundlatch_r			},	//
//	{ 0x0801, 0x0801, soundlatch2_r			},	//
	{ 0x1000, 0x1000, input_port_0_r		},	// P1
	{ 0x1001, 0x1001, input_port_1_r		},	// P2
	{ 0x1002, 0x1002, input_port_2_r		},	// Coins
	{ 0x2001, 0x2001, YM2203_read_port_0_r	},
	{ 0x5000, 0x57ff, MRA_RAM				},	// Shared RAM
	{ 0x6000, 0x7fff, MRA_ROM				},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1				},	// Banked ROM
	{ 0xc000, 0xffff, MRA_ROM				},	// ROM
MEMORY_END

static MEMORY_WRITE_START( tndrcade_sub_writemem )
	{ 0x0000, 0x01ff, MWA_RAM					},	// RAM
	{ 0x1000, 0x1000, sub_bankswitch_w			},	// ROM Bank + Coin Lockout
	{ 0x2000, 0x2000, YM2203_control_port_0_w	},
	{ 0x2001, 0x2001, YM2203_write_port_0_w		},
	{ 0x3000, 0x3000, YM3812_control_port_0_w	},
	{ 0x3001, 0x3001, YM3812_write_port_0_w		},
	{ 0x5000, 0x57ff, MWA_RAM, &sharedram		},	// Shared RAM
	{ 0x6000, 0xffff, MWA_ROM					},	// ROM
MEMORY_END







/***************************************************************************


								Input Ports


***************************************************************************/

#define	JOY_TYPE1_1BUTTON(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)

#define	JOY_TYPE1_2BUTTONS(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)

#define	JOY_TYPE1_3BUTTONS(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)



#define	JOY_TYPE2_1BUTTON(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)

#define	JOY_TYPE2_2BUTTONS(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN						) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)

#define	JOY_TYPE2_3BUTTONS(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3		|	IPF_PLAYER##_n_	) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_					)


#define JOY_ROTATION(_n_, _left_, _right_ ) \
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_PLAYER##_n_, 15, 15, 0, 0, KEYCODE_##_left_, KEYCODE_##_right_, 0, 0 )



/***************************************************************************
								Arbalester
***************************************************************************/

INPUT_PORTS_START( arbalest )
	PORT_START	// IN0 - Player 1
	JOY_TYPE2_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE2_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0080, IP_ACTIVE_LOW, IPT_COIN1, 5 )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x4001, 0x4001, "Licensed To" )
	PORT_DIPSETTING(      0x0001, "Jordan" )
	PORT_DIPSETTING(      0x4000, "Romstar" )
	PORT_DIPSETTING(      0x4001, "Romstar" )
	PORT_DIPSETTING(      0x0000, "Taito" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-4" )	// not used, according to manual
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Easy" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "Never" )
	PORT_DIPSETTING(      0x0800, "300k Only" )
	PORT_DIPSETTING(      0x0400, "600k Only" )
	PORT_DIPSETTING(      0x0000, "300k & 600k" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "5" )
//                        0x4000  License (see first dsw)
	PORT_DIPNAME( 0x8000, 0x8000, "Coinage Type" )	// not supported
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
INPUT_PORTS_END


/***************************************************************************
								Athena no Hatena?
***************************************************************************/

INPUT_PORTS_START( atehate )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3  |  IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4  |  IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1  |  IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2  |  IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3  |  IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4  |  IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1  |  IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2  |  IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1, 5 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN ) // 4 Bits Called "Cut DSW"
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $e00001 & 3.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )	// manual: unused
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0400, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x1000, "4" )
	PORT_DIPSETTING(      0x2000, "5" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0xc000, "None" )
	PORT_DIPSETTING(      0x0000, "20K Only" )
	PORT_DIPSETTING(      0x8000, "20K, Every 30K" )
	PORT_DIPSETTING(      0x4000, "30K, Every 40K" )
INPUT_PORTS_END


/***************************************************************************
								Blandia
***************************************************************************/

INPUT_PORTS_START( blandia )
	PORT_START	// IN0 - Player 1 - $400000.w
	JOY_TYPE1_3BUTTONS(1)

	PORT_START	// IN1 - Player 2 - $400002.w
	JOY_TYPE1_3BUTTONS(2)

	PORT_START	// IN2 - Coins - $400004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Coinage Type" )	// not supported
	PORT_DIPSETTING(      0x0002, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, "3 Coins/7 Credit" )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0400, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "2 Player Game" )
	PORT_DIPSETTING(      0x1000, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit"  )
	PORT_DIPNAME( 0x2000, 0x2000, "Continue" )
	PORT_DIPSETTING(      0x2000, "1 Credit" )
	PORT_DIPSETTING(      0x0000, "1 Coin"   )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END



/***************************************************************************
								Block Carnival
***************************************************************************/

INPUT_PORTS_START( blockcar )
	PORT_START	// IN0 - Player 1 - $500001.b
	JOY_TYPE1_2BUTTONS(1)	// button2 = speed up

	PORT_START	// IN1 - Player 2 - $500003.b
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins + DSW - $500005.b
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_DIPNAME( 0x0010, 0x0000, "Title" )
	PORT_DIPSETTING(      0x0010, "Thunder & Lightning 2" )
	PORT_DIPSETTING(      0x0000, "Block Carnival" )

	PORT_START	// IN3 - 2 DSWs - $300003 & 1.b
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"    )
	PORT_DIPSETTING(      0x0003, "Normal"  )
	PORT_DIPSETTING(      0x0001, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "20K, Every 50K" )
	PORT_DIPSETTING(      0x0004, "20K, Every 70K" )
	PORT_DIPSETTING(      0x0008, "30K, Every 60K" )
	PORT_DIPSETTING(      0x0000, "30K, Every 90K" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 1-3" )	// service mode, according to a file in the archive
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
INPUT_PORTS_END



/***************************************************************************
								Caliber 50
***************************************************************************/

INPUT_PORTS_START( calibr50 )
	PORT_START	// IN0 - Player 1
	JOY_TYPE2_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE2_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0080, IP_ACTIVE_LOW, IPT_COIN1, 5 )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x4001, 0x4000, "Licensed To" )
	PORT_DIPSETTING(      0x0001, "Romstar"       )
	PORT_DIPSETTING(      0x4001, "Taito America" )
	PORT_DIPSETTING(      0x4000, "Taito"         )
	PORT_DIPSETTING(      0x0000, "None (Japan)"  )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Easiest" )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0100, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x0400, 0x0400, "Score Digits" )
	PORT_DIPSETTING(      0x0400, "7" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0800, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x1000, 0x1000, "Display Score" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Erase Backup Ram" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	//                    0x4000  Country / License (see first dsw)
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )	/* manual: "Don't Touch" */
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN4 - Rotation Player 1
	JOY_ROTATION(1, Z, X)

	PORT_START	// IN5 - Rotation Player 2
	JOY_ROTATION(2, N, M)
INPUT_PORTS_END

/***************************************************************************
								Daioh
***************************************************************************/

INPUT_PORTS_START( daioh )
	PORT_START
	JOY_TYPE1_3BUTTONS(1)

	PORT_START
	JOY_TYPE1_3BUTTONS(2)

	PORT_START
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Auto Shot" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x1000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "5" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x8000, "300k and every 800k" )
	PORT_DIPSETTING(      0xc000, "500k and every 1000k" )
	PORT_DIPSETTING(      0x4000, "800k and 2000k only" )
	PORT_DIPSETTING(      0x0000, "1000k Only" )

	PORT_START
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************
								Dragon Unit
***************************************************************************/

INPUT_PORTS_START( drgnunit )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_3BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_3BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_DIPNAME( 0x0010, 0x0010, "Coinage Type" ) // not supported
	PORT_DIPSETTING(      0x0010, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Title" )
	PORT_DIPSETTING(      0x0020, "Dragon Unit" )
	PORT_DIPSETTING(      0x0000, "Castle of Dragon" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "(C) / License" )
	PORT_DIPSETTING(      0x00c0, "Athena (Japan)" )
	PORT_DIPSETTING(      0x0080, "Athena / Taito (Japan)" )
	PORT_DIPSETTING(      0x0040, "Seta USA / Taito America" )
	PORT_DIPSETTING(      0x0000, "Seta USA / Romstar" )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 1-0&1" )
	PORT_DIPSETTING(      0x0002, "00" )
	PORT_DIPSETTING(      0x0003, "08" )
	PORT_DIPSETTING(      0x0001, "10" )
	PORT_DIPSETTING(      0x0000, "18" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0008, "150K, Every 300K" )
	PORT_DIPSETTING(      0x000c, "200K, Every 400K" )
	PORT_DIPSETTING(      0x0004, "300K, Every 500K" )
	PORT_DIPSETTING(      0x0000, "400K Only" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7*" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2*" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
INPUT_PORTS_END



/***************************************************************************
								DownTown
***************************************************************************/

INPUT_PORTS_START( downtown )
	PORT_START	// IN0 - Player 1
	JOY_TYPE2_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE2_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0080, IP_ACTIVE_LOW, IPT_COIN1, 5 )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0000, "Sales" )
	PORT_DIPSETTING(      0x0001, "Japan Only" )
	PORT_DIPSETTING(      0x0000, "World" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
// other coinage
#if 0
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
#endif

	PORT_DIPNAME( 0x0300, 0x0300, "Unknown 2-0&1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "Never" )
	PORT_DIPSETTING(      0x0800, "50K Only" )
	PORT_DIPSETTING(      0x0400, "100K Only" )
	PORT_DIPSETTING(      0x0000, "50K, Every 150K" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x1000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPSETTING(      0x2000, "5" )
	PORT_DIPNAME( 0x4000, 0x4000, "World License" )
	PORT_DIPSETTING(      0x4000, "Romstar" )
	PORT_DIPSETTING(      0x0000, "Taito" )
	PORT_DIPNAME( 0x8000, 0x8000, "Coinage Type" )	// not supported
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )

	PORT_START	// IN4 - Rotation Player 1
	JOY_ROTATION(1, Z, X)

	PORT_START	// IN5 - Rotation Player 2
	JOY_ROTATION(1, N, M)
INPUT_PORTS_END



/***************************************************************************
								Eight Force
***************************************************************************/

INPUT_PORTS_START( eightfrc )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Shared Credits" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Credits To Start" )
	PORT_DIPSETTING(      0x0080, "1" )
	PORT_DIPSETTING(      0x0000, "2" )

	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0x6000, "3" )
	PORT_DIPSETTING(      0x2000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x8000, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPSETTING(      0x8000, "Japanese" )
INPUT_PORTS_END



/***************************************************************************
								Extreme Downhill
***************************************************************************/

INPUT_PORTS_START( extdwnhl )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )	// Country Code
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )	//
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $400009 & b.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Controls" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Cheap Continue" )
	PORT_DIPSETTING(      0x4000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Game Mode" )
	PORT_DIPSETTING(      0x8000, "Finals Only" )
	PORT_DIPSETTING(      0x0000, "Semi-Finals & Finals" )
INPUT_PORTS_END



/***************************************************************************
								Gundhara
***************************************************************************/

INPUT_PORTS_START( gundhara )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_3BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_3BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Country" )
	PORT_DIPSETTING(      0x00c0, "Japan" )
	PORT_DIPSETTING(      0x0000, "World" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0800, "1" )
	PORT_DIPSETTING(      0x0c00, "2" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x3000, "200K" )
	PORT_DIPSETTING(      0x2000, "200K, Every 200K" )
	PORT_DIPSETTING(      0x1000, "400K" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END



/***************************************************************************
								J.J.Squawkers
***************************************************************************/

INPUT_PORTS_START( jjsquawk )
	PORT_START	// IN0 - Player 1 - $400000.w
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2 - $400002.w
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins - $400004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )	// ?? screen related
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0400, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, "Energy" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x1000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x8000, "20K, Every 100K" )
	PORT_DIPSETTING(      0xc000, "50K, Every 200K" )
	PORT_DIPSETTING(      0x4000, "70K, 200K Only" )
	PORT_DIPSETTING(      0x0000, "100K Only" )
INPUT_PORTS_END

/***************************************************************************
				(Kamen) Masked Riders Club Battle Race
***************************************************************************/

INPUT_PORTS_START( kamenrid )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)	// BUTTON3 in "test mode" only

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)	// BUTTON3 in "test mode" only

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $500005 & 7.b
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )		// masked at 0x001682
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )		// masked at 0x001682
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )			// (displays debug infos)
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )		// masked at 0x001682
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )			// (unknown effect at 0x00606a, 0x0060de, 0x00650a)
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )		// check code at 0x001792
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0f00, 0x0f00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0d00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0b00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0f00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0900, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
								Krazy Bowl
***************************************************************************/

#define KRZYBOWL_TRACKBALL(_dir_, _n_ ) \
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_##_dir_ | IPF_PLAYER##_n_ | IPF_REVERSE, 70, 30, 0, 0 )

INPUT_PORTS_START( krzybowl )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_3BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_3BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1, 5 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, "Frames" )
	PORT_DIPSETTING(      0x0010, "10" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Controls" )
	PORT_DIPSETTING(      0x0040, "Trackball" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Force Coinage" )
	PORT_DIPSETTING(      0x4000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN4 - Rotation X Player 1
	KRZYBOWL_TRACKBALL(X,1)

	PORT_START	// IN5 - Rotation Y Player 1
	KRZYBOWL_TRACKBALL(Y,1)

	PORT_START	// IN6 - Rotation X Player 2
	KRZYBOWL_TRACKBALL(X|IPF_REVERSE,2)

	PORT_START	// IN7 - Rotation Y Player 2
	KRZYBOWL_TRACKBALL(Y,2)
INPUT_PORTS_END



/***************************************************************************
								Meta Fox
***************************************************************************/

INPUT_PORTS_START( metafox )
	PORT_START	// IN0
	JOY_TYPE2_2BUTTONS(1)

	PORT_START	// IN1
	JOY_TYPE2_2BUTTONS(2)

	PORT_START	// IN2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x0040, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0080, IP_ACTIVE_LOW, IPT_COIN1, 5 )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x4001, 0x4001, "Licensed To"    )
	PORT_DIPSETTING(      0x0001, "Jordan"        )
	PORT_DIPSETTING(      0x4001, "Romstar"       )
	PORT_DIPSETTING(      0x4000, "Taito"         )
	PORT_DIPSETTING(      0x0000, "Taito America" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Normal"  )
	PORT_DIPSETTING(      0x0200, "Easy"    )
	PORT_DIPSETTING(      0x0100, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "5" )
//	PORT_DIPNAME( 0x4000, 0x4000, "License" )
	PORT_DIPNAME( 0x8000, 0x8000, "Coinage Type" )	// not supported
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
INPUT_PORTS_END



/***************************************************************************
							Mobile Suit Gundam
***************************************************************************/

#define	__msgundam_LANGUEGE	1		/* Add select languege dip */

INPUT_PORTS_START( msgundam )
	PORT_START	// IN0 - Player 1 - $400000.w
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2 - $400002.w
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins - $400004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
#if	__msgundam_LANGUEGE
	PORT_DIPNAME( 0x0080, 0x0080, "Language" )
	PORT_DIPSETTING(      0x0080, "English" )
	PORT_DIPSETTING(      0x0000, "Japanese" )
#else
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
#endif
	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0600, 0x0600, DEF_STR( Difficulty ) )	// unverified, from the manual
	PORT_DIPSETTING(      0x0400, "Easy"    )
	PORT_DIPSETTING(      0x0600, "Normal"  )
	PORT_DIPSETTING(      0x0200, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Memory Check" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown 1-6" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END



/***************************************************************************
							Oishii Puzzle
***************************************************************************/

INPUT_PORTS_START( oisipuzl )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )	// these seem unused
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )		// this seems unused
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************
							Pro Mahjong Kiwame
***************************************************************************/

INPUT_PORTS_START( kiwame )
	PORT_START	// IN0 - Unused
	PORT_START	// IN1 - Unused

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT_IMPULSE( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1, 5 )

	PORT_START	// IN3 - 2 DSWs - $e00001 & 3.b
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Player's TSUMO" )
	PORT_DIPSETTING(      0x0080, "Manual" )
	PORT_DIPSETTING(      0x0000, "Auto"   )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1c00, "None" )
	PORT_DIPSETTING(      0x1800, "Prelim  1" )
	PORT_DIPSETTING(      0x1400, "Prelim  2" )
	PORT_DIPSETTING(      0x1000, "Final   1" )
	PORT_DIPSETTING(      0x0c00, "Final   2" )
	PORT_DIPSETTING(      0x0800, "Final   3" )
	PORT_DIPSETTING(      0x0400, "Qrt Final" )
	PORT_DIPSETTING(      0x0000, "SemiFinal" )
	PORT_DIPNAME( 0xe000, 0xe000, "Points Gap" )
	PORT_DIPSETTING(      0xe000, "None" )
	PORT_DIPSETTING(      0xc000, "+6000" )
	PORT_DIPSETTING(      0xa000, "+4000" )
	PORT_DIPSETTING(      0x8000, "+2000" )
	PORT_DIPSETTING(      0x6000, "-2000" )
	PORT_DIPSETTING(      0x4000, "-4000" )
	PORT_DIPSETTING(      0x2000, "-6000" )
	PORT_DIPSETTING(      0x0000, "-8000" )

/*
		row 0	1	2	3	4
bit	0		a	b	c	d	lc
	1		e	f	g	h
	2		i	j	k	l
	3		m	n	ch	po	ff
	4		ka	re	ro
	5		st	bt
*/

	PORT_START	// IN4
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 LC",  KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT (0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT (0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 FF",  KEYCODE_RALT,     IP_JOY_NONE )
	PORT_BIT (0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN5
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,      IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,      IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,      IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,      IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "P1 BT",    KEYCODE_X,      IP_JOY_NONE )
	PORT_BIT (0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN6
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT (0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN7
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT (0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN8
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT (0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************
								Quiz Kokology
***************************************************************************/

INPUT_PORTS_START( qzkklogy )
	PORT_START	// IN0 - Player 1 - $b00001.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )	// pause (cheat)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN1 - Player 2 - $b00003.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )	// pause (cheat)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN2 - Coins - $b00005.b
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 1-0&1*" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Highlight Right Answer", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x8000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x4000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
INPUT_PORTS_END



/***************************************************************************
								Quiz Kokology 2
***************************************************************************/

INPUT_PORTS_START( qzkklgy2 )
	PORT_START	// IN0 - Player 1 - $b00001.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	// IN1 - Player 2 - $b00003.b
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	// IN2 - Coins - $b00005.b
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 1-0&1*" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Highlight Right Answer", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3000, "Easy" )
	PORT_DIPSETTING(      0x2000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x8000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x4000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
INPUT_PORTS_END


/***************************************************************************
									Rezon
***************************************************************************/

INPUT_PORTS_START( rezon )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_3BUTTONS(1)	// 1 used??

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_3BUTTONS(2)	// 1 used ??

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0018, 0x0018, "Unknown 1-3&4*" )
	PORT_DIPSETTING(      0x0018, "11" )
	PORT_DIPSETTING(      0x0010, "10" )
	PORT_DIPSETTING(      0x0008, "01" )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Difficulty?" )
	PORT_DIPSETTING(      0x0c00, "11" )
	PORT_DIPSETTING(      0x0800, "10" )
	PORT_DIPSETTING(      0x0400, "01" )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END



/***************************************************************************
								Sokonuke
***************************************************************************/

INPUT_PORTS_START( sokonuke )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_1BUTTON(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_1BUTTON(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - 2 DSWs - $400009 & b.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5*" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	// unused?
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Cheap Continue" )
	PORT_DIPSETTING(      0x4000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )	// unused?
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************
								Strike Gunner
***************************************************************************/

INPUT_PORTS_START( stg )
	PORT_START	// IN0 - Player 1 - $b00001.b
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2 - $b00003.b
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins - $b00005.b
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
//	PORT_DIPNAME( 0x00f0, 0x00f0, "Title" )
	/* This is the index in a table with pointers to the
	   title logo, but the table is filled with just 1 value */

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"    ) // 0
	PORT_DIPSETTING(      0x0003, "Normal"  ) // 4
	PORT_DIPSETTING(      0x0001, "Hard"    ) // 8
	PORT_DIPSETTING(      0x0000, "Hardest" ) // b
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************
							Thunder & Lightning
***************************************************************************/

INPUT_PORTS_START( thunderl )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)	// button2 = speed up

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins + DSW
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_DIPNAME( 0x0010, 0x0000, "Force 1 Life" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, "Copyright" )
	PORT_DIPSETTING(      0x0080, "Romstar" )
	PORT_DIPSETTING(      0x00c0, "Seta (Romstar License)" )
	PORT_DIPSETTING(      0x00e0, "Seta (Visco License)" )
	PORT_DIPSETTING(      0x00a0, "Visco" )
	PORT_DIPSETTING(      0x0060, "None" )
//	PORT_DIPSETTING(      0x0040, "None" )
//	PORT_DIPSETTING(      0x0020, "None" )
//	PORT_DIPSETTING(      0x0000, "None" )

	PORT_START	// IN3 - 2 DSWs - $600003 & 1.b
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_4C ) )

	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )		// WEIRD!
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Controls" )
	PORT_DIPSETTING(      0x0800, "2" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x2000, "3" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x8000, "Easy" )
	PORT_DIPSETTING(      0xc000, "Normal" )
	PORT_DIPSETTING(      0x4000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
INPUT_PORTS_END



/***************************************************************************
								Thundercade (US)
***************************************************************************/

INPUT_PORTS_START( tndrcade )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, "Difficulty?" )
	PORT_DIPSETTING(      0x0002, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "50K  Only" )
	PORT_DIPSETTING(      0x0004, "50K, Every 150K" )
	PORT_DIPSETTING(      0x0004, "70K, Every 200K" )
	PORT_DIPSETTING(      0x0008, "100K Only" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Licensed To" )	// + coin mode (not supported)
	PORT_DIPSETTING(      0x0080, "Taito America Corp." )
	PORT_DIPSETTING(      0x0000, "Taito Corp. Japan" )

	PORT_DIPNAME( 0x0100, 0x0100, "Title" )
	PORT_DIPSETTING(      0x0100, "Thundercade" )
	PORT_DIPSETTING(      0x0000, "Twin Formation" )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
INPUT_PORTS_END


/***************************************************************************
								Thundercade (Japan)
***************************************************************************/

INPUT_PORTS_START( tndrcadj )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0003, 0x0003, "Difficulty?" )
	PORT_DIPSETTING(      0x0002, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "50K  Only" )
	PORT_DIPSETTING(      0x0004, "50K, Every 150K" )
	PORT_DIPSETTING(      0x0004, "70K, Every 200K" )
	PORT_DIPSETTING(      0x0008, "100K Only" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BITX(    0x0100, 0x0100, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
INPUT_PORTS_END



/***************************************************************************
								Twin Eagle
***************************************************************************/

INPUT_PORTS_START( twineagl )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x4001, 0x4001, "Copyright" )		// Always "Seta" if sim. players = 1
	PORT_DIPSETTING(      0x4001, "Seta (Taito license)" )
	PORT_DIPSETTING(      0x0001, "Taito" )
	PORT_DIPSETTING(      0x4000, "Taito America" )
	PORT_DIPSETTING(      0x0000, "Taito America (Romstar license)" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Cabinet ) )	// Only if simultaneous players = 1
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Normal"  )
	PORT_DIPSETTING(      0x0200, "Easy"    )
	PORT_DIPSETTING(      0x0100, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "Never" )
	PORT_DIPSETTING(      0x0800, "500K Only" )
	PORT_DIPSETTING(      0x0400, "1000K Only" )
	PORT_DIPSETTING(      0x0000, "500K, Every 1500K" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "5" )
	PORT_DIPNAME( 0x8000, 0x8000, "Coinage Type" )	// not supported
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )

	/* Fake Dip Switch to determine maximum numbers of simultaneous players */
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Max. simultaneous players" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x01, "2" )
INPUT_PORTS_END



/***************************************************************************
								Ultraman Club
***************************************************************************/

INPUT_PORTS_START( umanclub )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2*" )	//?
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stage Select", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0400, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END



/***************************************************************************
								U.S. Classic
***************************************************************************/

#define TRACKBALL(_dir_) \
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_##_dir_ | IPF_CENTER, 70, 30, 0, 0 )

INPUT_PORTS_START( usclssic )
	PORT_START	// IN0
	TRACKBALL(X)
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT   ( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT   ( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1
	TRACKBALL(Y)
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT   ( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT   ( 0x4000, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	// IN2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN  )	// tested (sound related?)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT_IMPULSE( 0x0010, IP_ACTIVE_HIGH, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0020, IP_ACTIVE_HIGH, IPT_COIN2, 5 )
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_TILT     )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, "Credits For 9-Hole" )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0002, 0x0002, "Game Type" )
	PORT_DIPSETTING(      0x0002, "Domestic" )
	PORT_DIPSETTING(      0x0000, "Foreign" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0004, "1" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x3800, 0x3800, "Flight Distance" )
	PORT_DIPSETTING(      0x3800, "Normal" )
	PORT_DIPSETTING(      0x3000, "-30 Yards" )
	PORT_DIPSETTING(      0x2800, "+10 Yards" )
	PORT_DIPSETTING(      0x2000, "+20 Yards" )
	PORT_DIPSETTING(      0x1800, "+30 Yards" )
	PORT_DIPSETTING(      0x1000, "+40 Yards" )
	PORT_DIPSETTING(      0x0800, "+50 Yards" )
	PORT_DIPSETTING(      0x0000, "+60 Yards" )
	PORT_DIPNAME( 0xc000, 0xc000, "Licensed To"     )
	PORT_DIPSETTING(      0xc000, "Romstar"       )
	PORT_DIPSETTING(      0x8000, "None (Japan)"  )
	PORT_DIPSETTING(      0x4000, "Taito"         )
	PORT_DIPSETTING(      0x0000, "Taito America" )
INPUT_PORTS_END



/***************************************************************************
								War of Aero
***************************************************************************/

INPUT_PORTS_START( wrofaero )
	PORT_START	// IN0 - Player 1 - $400000.w
	JOY_TYPE1_3BUTTONS(1)	// 3rd button selects the weapon
							// when the dsw for cheating is on

	PORT_START	// IN1 - Player 2 - $400002.w
	JOY_TYPE1_3BUTTONS(2)

	PORT_START	// IN2 - Coins - $400004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1*" )	// tested
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2*" )	// tested
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stage & Weapon Select", IP_KEY_NONE, IP_JOY_NONE )	// P2 Start Is Freeze Screen...
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Unknown 2-2&3" )
	PORT_DIPSETTING(      0x0800, "0" )
	PORT_DIPSETTING(      0x0c00, "1" )
	PORT_DIPSETTING(      0x0400, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END



/***************************************************************************
									Wit's
***************************************************************************/

INPUT_PORTS_START( wits )
	PORT_START	// IN0 - Player 1
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins + DSW
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 3-4*" )	// Jumpers, I guess
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 3-5*" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x0040, "License" )
	PORT_DIPSETTING(      0x00c0, "Romstar" )
	PORT_DIPSETTING(      0x0080, "Seta U.S.A" )
	PORT_DIPSETTING(      0x0040, "Visco (Japan Only)" )
	PORT_DIPSETTING(      0x0000, "Athena (Japan Only)" )

	PORT_START	// IN3 - 2 DSWs - $600003 & 1.b
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0008, "150k, 350k" )
	PORT_DIPSETTING(      0x000c, "200k, 500k" )
	PORT_DIPSETTING(      0x0004, "300k, 600k" )
	PORT_DIPSETTING(      0x0000, "400k" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Max Players" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7*" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2*" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )

	PORT_START	// IN4 - Player 3
	JOY_TYPE1_2BUTTONS(3)

	PORT_START	// IN5 - Player 4
	JOY_TYPE1_2BUTTONS(4)
INPUT_PORTS_END


/***************************************************************************
								Zing Zing Zip
***************************************************************************/

INPUT_PORTS_START( zingzip )
	PORT_START	// IN0 - Player 1 - $400000.w
	JOY_TYPE1_2BUTTONS(1)

	PORT_START	// IN1 - Player 2 - $400002.w
	JOY_TYPE1_2BUTTONS(2)

	PORT_START	// IN2 - Coins - $400004.w
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  ) // no coin 2
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - 2 DSWs - $600001 & 3.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-3" )	// remaining switches seem unused
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0400, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END


/***************************************************************************


								Graphics Layouts

Sprites and layers use 16x16 tile, made of four 8x8 tiles. They can be 4
or 6 planes deep and are stored in a wealth of formats.

***************************************************************************/

						/* First the 4 bit tiles */


/* The bitplanes are packed togheter */
static struct GfxLayout layout_packed =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{2*4,3*4,0*4,1*4},
	{256+128,256+129,256+130,256+131, 256+0,256+1,256+2,256+3,
	 128,129,130,131, 0,1,2,3},
	{0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
	 32*16,33*16,34*16,35*16,36*16,37*16,38*16,39*16},
	16*16*4
};


/* The bitplanes are separated (but there are 2 per rom) */
static struct GfxLayout layout_planes_2roms =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0},
	{0,1,2,3,4,5,6,7, 128,129,130,131,132,133,134,135},
	{0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
	 16*16,17*16,18*16,19*16,20*16,21*16,22*16,23*16 },
	16*16*2
};


/* The bitplanes are separated (but there are 2 per rom).
   Each 8x8 tile is additionally split in 2 vertical halves four bits wide,
   stored one after the other */
static struct GfxLayout layout_planes_2roms_split =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{0,4, RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+4},
	{128+64,128+65,128+66,128+67, 128+0,128+1,128+2,128+3,
	 8*8+0,8*8+1,8*8+2,8*8+3, 0,1,2,3},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
	 32*8,33*8,34*8,35*8,36*8,37*8,38*8,39*8},
	16*16*2
};




						/* Then the 6 bit tiles */


/* The bitplanes are packed together: 3 roms with 2 bits in each */
static struct GfxLayout layout_packed_6bits_3roms =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{RGN_FRAC(0,3)+0,RGN_FRAC(0,3)+4,  RGN_FRAC(1,3)+0,RGN_FRAC(1,3)+4,  RGN_FRAC(2,3)+0,RGN_FRAC(2,3)+4},
	{128+64,128+65,128+66,128+67, 128+0,128+1,128+2,128+3,
	 64,65,66,67, 0,1,2,3},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
	 32*8,33*8,34*8,35*8,36*8,37*8,38*8,39*8},
	16*16*2
};


/* The bitplanes are packed togheter: 4 bits in one rom, 2 bits in another.
   Since there isn't simmetry between the two roms, we load the latter with
   ROM_LOAD16_BYTE. This way we can think of it as a 4 planes rom, with the
   upper 2 planes unused.	 */

static struct GfxLayout layout_packed_6bits_2roms =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4, 2*4,3*4,0*4,1*4},
	{256+128,256+129,256+130,256+131, 256+0,256+1,256+2,256+3,
	 128,129,130,131, 0,1,2,3},
	{0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
	 32*16,33*16,34*16,35*16,36*16,37*16,38*16,39*16},
	16*16*4
};



/***************************************************************************
								Blandia
***************************************************************************/

static struct GfxDecodeInfo blandia_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,       0,           32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed_6bits_3roms, 16*32+64*32, 32 }, // [1] Layer 1
	{ REGION_GFX3, 0, &layout_packed_6bits_3roms, 16*32,       32 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								DownTown
***************************************************************************/

static struct GfxDecodeInfo downtown_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,       512*0, 32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_planes_2roms_split, 512*0, 32 }, // [1] Layer 1
	{ -1 }
};

/***************************************************************************
								J.J.Squawkers
***************************************************************************/

static struct GfxDecodeInfo jjsquawk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,       0,             32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed_6bits_2roms, 512 + 64*32*0, 32 }, // [1] Layer 1
	{ REGION_GFX3, 0, &layout_packed_6bits_2roms, 512 + 64*32*1, 32 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
							Mobile Suit Gundam
***************************************************************************/

static struct GfxDecodeInfo msgundam_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms, 512*0, 32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed,       512*2, 32 }, // [1] Layer 1
	{ REGION_GFX3, 0, &layout_packed,       512*1, 32 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
								Quiz Kokology 2
***************************************************************************/

static struct GfxDecodeInfo qzkklgy2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,	512*0, 32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed, 		512*0, 32 }, // [1] Layer 1
	{ -1 }
};

/***************************************************************************
								Thundercade
***************************************************************************/

static struct GfxDecodeInfo tndrcade_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms, 512*0, 32 }, // [0] Sprites
	{ -1 }
};

/***************************************************************************
								U.S. Classic
***************************************************************************/

/* 6 bit layer. The colors are still WRONG.
   Remember there's a vh_init_palette function */

static struct GfxDecodeInfo usclssic_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,       512*0+256, 32/2 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed_6bits_3roms, 512*1, 32 }, // [1] Layer 1
	{ -1 }
};

/***************************************************************************
								Zing Zing Zip
***************************************************************************/

static struct GfxDecodeInfo zingzip_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_planes_2roms,       512*0, 32 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_packed_6bits_2roms, 512*2, 32 }, // [1] Layer 1
	{ REGION_GFX3, 0, &layout_packed,             512*1, 32 }, // [2] Layer 2
	{ -1 }
};










/***************************************************************************

								Machine drivers

***************************************************************************/

#define SETA_INTERRUPTS_NUM 2

static INTERRUPT_GEN( seta_interrupt_1_and_2 )
{
	switch (cpu_getiloops())
	{
		case 0:		cpu_set_irq_line(0, 1, HOLD_LINE);	break;
		case 1:		cpu_set_irq_line(0, 2, HOLD_LINE);	break;
	}
}

static INTERRUPT_GEN( seta_interrupt_2_and_4 )
{
	switch (cpu_getiloops())
	{
		case 0:		cpu_set_irq_line(0, 2, HOLD_LINE);	break;
		case 1:		cpu_set_irq_line(0, 4, HOLD_LINE);	break;
	}
}


#define SETA_SUB_INTERRUPTS_NUM 2

static INTERRUPT_GEN( seta_sub_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);	break;
		case 1:		cpu_set_irq_line(1, 0, HOLD_LINE);				break;
	}
}


/***************************************************************************
								Athena no Hatena?
***************************************************************************/

static MACHINE_DRIVER_START( atehate )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(atehate_readmem,atehate_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Blandia
***************************************************************************/

/*
	Similar to wrofaero, but the layers are 6 planes deep (and
	the pens are strangely mapped to palette entries) + the
	samples are bankswitched
*/

MACHINE_INIT( blandia )
{
	seta_samples_bank = -1;	// set the samples bank to an out of range value at start-up
}

static MACHINE_DRIVER_START( blandia )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(blandia_readmem,blandia_writemem)

	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(blandia)	// Bankswitched Samples

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(blandia_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+64*32+64*32)	/* sprites, layer1, layer2 */

	MDRV_PALETTE_INIT(blandia)				/* layers 1&2 are 6 planes deep */
	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_EOF(seta_buffer_sprites)		/* Blandia uses sprite buffering */
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( blandiap )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
#if	__X1_010_V2
	MDRV_CPU_MEMORY(blandiap_readmem,blandiap_writemem)
#else
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
#endif
	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(blandia)	// Bankswitched Samples

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(blandia_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+64*32+64*32)	/* sprites, layer1, layer2 */

	MDRV_PALETTE_INIT(blandia)				/* layers 1&2 are 6 planes deep */
	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_EOF(seta_buffer_sprites)		/* Blandia uses sprite buffering */
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Block Carnival
***************************************************************************/

static MACHINE_DRIVER_START( blockcar )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(blockcar_readmem,blockcar_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Caliber 50
***************************************************************************/

/*	calibr50 lev 6 = lev 2 + lev 4 !
	Test mode shows a 16ms and 4ms counters. I wonder if every game has
	5 ints per frame */

#define calibr50_INTERRUPTS_NUM (4+1)
INTERRUPT_GEN( calibr50_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:
		case 1:
		case 2:
		case 3:		cpu_set_irq_line(0, 4, HOLD_LINE);	break;
		case 4:		cpu_set_irq_line(0, 2, HOLD_LINE);	break;
	}
}


static MACHINE_DRIVER_START( calibr50 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(calibr50_readmem,calibr50_writemem)
	MDRV_CPU_VBLANK_INT(calibr50_interrupt,calibr50_INTERRUPTS_NUM)

	MDRV_CPU_ADD(M65C02, 2000000)	/* ?? */
	MDRV_CPU_MEMORY(calibr50_sub_readmem,calibr50_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)	/* IRQ: 4/frame
							   NMI: when the 68k writes the sound latch */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(downtown_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer_offset_0x02)	// a little offset
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

/***************************************************************************
								Daioh
***************************************************************************/

static MACHINE_DRIVER_START( daioh )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(daioh_readmem,daioh_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256)
	MDRV_VISIBLE_AREA(16, 400-1, 16, 256-1)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer1, layer2 */

	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

/***************************************************************************
								DownTown
***************************************************************************/

/* downtown lev 3 = lev 2 + lev 1 ! */

static MACHINE_DRIVER_START( downtown )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(downtown_readmem,downtown_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_CPU_ADD(M65C02, 1000000)	/* ?? */
	MDRV_CPU_MEMORY(downtown_sub_readmem,downtown_sub_writemem)
	MDRV_CPU_VBLANK_INT(seta_sub_interrupt,SETA_SUB_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(downtown_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_8MHz)
MACHINE_DRIVER_END



/***************************************************************************
				Dragon Unit, Quiz Kokology, Strike Gunner
***************************************************************************/

/*
	drgnunit,qzkklogy,stg:
	lev 1 == lev 3 (writes to $500000, bit 4 -> 1 then 0)
	lev 2 drives the game
*/

static MACHINE_DRIVER_START( drgnunit )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(drgnunit_readmem,drgnunit_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(downtown_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer_offset_0x02)	// a little offset
	MDRV_VIDEO_EOF(seta_buffer_sprites)	/* qzkklogy uses sprite buffering */
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

/*	Same as qzkklogy, but with a 16MHz CPU and different
	layout for the layer's tiles	*/

static MACHINE_DRIVER_START( qzkklgy2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(drgnunit_readmem,drgnunit_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16+1, 400-1+1, 0, 256-1 -16)	// a little offset
	MDRV_GFXDECODE(qzkklgy2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer)
	MDRV_VIDEO_EOF(seta_buffer_sprites)	/* qzkklogy uses sprite buffering */
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Eight Force
***************************************************************************/

static MACHINE_DRIVER_START( eightfrc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(blandia)	// Bankswitched Samples

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256-16-8)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16-16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer1, layer2 */

	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
						Extreme Downhill / Sokonuke
***************************************************************************/

/*
	extdwnhl:
	lev 1 == lev 3 (writes to $500000, bit 4 -> 1 then 0)
	lev 2 drives the game
*/
static MACHINE_DRIVER_START( extdwnhl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(extdwnhl_readmem,extdwnhl_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256-16)
	MDRV_VISIBLE_AREA(16, 320+16-1, 0, 256-16-1)
	MDRV_GFXDECODE(zingzip_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+16*32+64*32)	/* sprites, layer2, layer1 */

	MDRV_PALETTE_INIT(zingzip)			/* layer 1 gfx is 6 planes deep */
	MDRV_VIDEO_START(seta_2_layers_offset_0x02)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Gundhara
***************************************************************************/
#if __uPD71054_TIMER
static INTERRUPT_GEN( wrofaero_interrupt )
{
	switch( cpu_getiloops() ) {
		case 0: cpu_set_irq_line( 0, 2, HOLD_LINE ); break;
	}
}

MACHINE_INIT( wrofaero ) { uPD71054_timer_init(); }
MACHINE_STOP( wrofaero ) { uPD71054_timer_stop(); }
#endif	// __uPD71054_TIMER



/*
	lev 2: Sound (generated by a timer mapped at $d00000-6 ?)
	lev 4: VBlank
*/
static MACHINE_DRIVER_START( gundhara )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
#if	__uPD71054_TIMER
	MDRV_CPU_VBLANK_INT( wrofaero_interrupt, 1 )
#else
	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)
#endif	// __uPD71054_TIMER

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

#if	__uPD71054_TIMER
	MDRV_MACHINE_INIT( wrofaero )
	MDRV_MACHINE_STOP( wrofaero )
#endif	// __uPD71054_TIMER

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256-16)
	MDRV_VISIBLE_AREA(16+8, 320+32+16+8+16-1, 0, 256-16-1)
	MDRV_GFXDECODE(jjsquawk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+64*32+64*32)	/* sprites, layer2, layer1 */

	MDRV_PALETTE_INIT(gundhara)				/* layers are 6 planes deep (but have only 4 palettes) */
	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								J.J.Squawkers
***************************************************************************/

/*
	lev 1 == lev 3 (writes to $500000, bit 4 -> 1 then 0)
	lev 2 drives the game
*/
static MACHINE_DRIVER_START( jjsquawk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(jjsquawk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+64*32+64*32)	/* sprites, layer2, layer1 */

	MDRV_PALETTE_INIT(jjsquawk)				/* layers are 6 planes deep */
	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

/***************************************************************************
				(Kamen) Masked Riders Club Battle Race
***************************************************************************/

static MACHINE_DRIVER_START( kamenrid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(kamenrid_readmem,kamenrid_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256) // ?
	MDRV_VISIBLE_AREA(16, 400-1, 16, 256-1) // ?
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer2, layer1 */

	MDRV_VIDEO_START(seta_2_layers_y_offset_0x10) // there are still offset problems
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz) // ?
MACHINE_DRIVER_END

/***************************************************************************
								Krazy Bowl
***************************************************************************/

static MACHINE_DRIVER_START( krzybowl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(krzybowl_readmem,krzybowl_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16-8)
	MDRV_VISIBLE_AREA(16+8, 320+8-1, 0, 256-1 -16-16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Meta Fox
***************************************************************************/

/* metafox lev 3 = lev 2 + lev 1 ! */

static MACHINE_DRIVER_START( metafox )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(downtown_readmem,downtown_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_CPU_ADD(M65C02, 1000000)	/* ?? */
	MDRV_CPU_MEMORY(metafox_sub_readmem,metafox_sub_writemem)
	MDRV_CPU_VBLANK_INT(seta_sub_interrupt,SETA_SUB_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16-8)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16-16 +2)
	MDRV_GFXDECODE(downtown_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END



/***************************************************************************
							Mobile Suit Gundam
***************************************************************************/

/* msgundam lev 2 == lev 6 ! */

static MACHINE_DRIVER_START( msgundam )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(msgundam_readmem,msgundam_writemem)
#if	__uPD71054_TIMER
	MDRV_CPU_VBLANK_INT( wrofaero_interrupt, 1 )
#else
	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)
#endif	// __uPD71054_TIMER

	MDRV_FRAMES_PER_SECOND(56.66)	/* between 56 and 57 to match a real PCB's game speed */
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

#if	__uPD71054_TIMER
	MDRV_MACHINE_INIT( wrofaero )
	MDRV_MACHINE_STOP( wrofaero )
#endif	// __uPD71054_TIMER

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer2, layer1 */

	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_EOF(seta_buffer_sprites)	/* msgundam uses sprite buffering */
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END



/***************************************************************************
							Oishii Puzzle
***************************************************************************/

static MACHINE_DRIVER_START( oisipuzl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(oisipuzl_readmem,oisipuzl_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16-8)
	MDRV_VISIBLE_AREA(16, 320+16-1, 0, 256-1 -16-16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer2, layer1 */

	MDRV_VIDEO_START(oisipuzl_2_layers)	// flip is inverted for the tilemaps
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

/***************************************************************************
							Triple Fun
***************************************************************************/

/* same as oisipuzl but with different interrupts and sound */

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

static MACHINE_DRIVER_START( triplfun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(triplfun_readmem,triplfun_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16-8)
	MDRV_VISIBLE_AREA(16, 320+16-1, 0, 256-1 -16-16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer2, layer1 */

	MDRV_VIDEO_START(oisipuzl_2_layers)	// flip is inverted for the tilemaps
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/***************************************************************************
							Pro Mahjong Kiwame
***************************************************************************/

static MACHINE_DRIVER_START( kiwame )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(kiwame_readmem,kiwame_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* lev 1-7 are the same. WARNING:
								   the interrupt table is written to. */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(464, 256 -16)
	MDRV_VISIBLE_AREA(16, 464-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END



/***************************************************************************
									Rezon
***************************************************************************/

/* pretty much like wrofaero, but ints are 1&2, not 2&4 */

static MACHINE_DRIVER_START( rezon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
	MDRV_CPU_VBLANK_INT(seta_interrupt_1_and_2,SETA_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer1, layer2 */

	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END



/***************************************************************************
						Thunder & Lightning / Wit's
***************************************************************************/

/*	thunderl lev 2 = lev 3 - other levels lead to an error */

static MACHINE_DRIVER_START( thunderl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(thunderl_readmem,thunderl_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wits )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(thunderl_readmem,thunderl_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END



/***************************************************************************
								Thundercade
***************************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface tndrcade_ym2203_interface =
{
	1,
	2000000,		/* ? */
	{ YM2203_VOL(35,35) },
	{ dsw1_r },		/* input A: DSW 1 */
	{ dsw2_r },		/* input B: DSW 2 */
	{ 0 },
	{ 0 },
//	{ irq_handler }
};

static struct YM3812interface ym3812_interface =
{
	1,
	4000000,	/* ? */
	{ 100,100 },	/* mixing level */
//	{ irq_handler },
};


#define TNDRCADE_SUB_INTERRUPTS_NUM 			1+16
static INTERRUPT_GEN( tndrcade_sub_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);	break;
		default:	cpu_set_irq_line(1, 0, HOLD_LINE);				break;
	}
}

static MACHINE_DRIVER_START( tndrcade )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(tndrcade_readmem,tndrcade_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(M65C02, 2000000)	/* ?? */
	MDRV_CPU_MEMORY(tndrcade_sub_readmem,tndrcade_sub_writemem)
	MDRV_CPU_VBLANK_INT(tndrcade_sub_interrupt,TNDRCADE_SUB_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16-8)
	MDRV_VISIBLE_AREA(16, 400-1, 0+8, 256-1 -16-8)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, tndrcade_ym2203_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END



/***************************************************************************
								Twin Eagle
***************************************************************************/

/* Just like metafox, but:
   the sub cpu reads the ip at different locations,
   the visible area seems different. */

static MACHINE_DRIVER_START( twineagl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(downtown_readmem,downtown_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_CPU_ADD(M65C02, 1000000)	/* ?? */
	MDRV_CPU_MEMORY(twineagl_sub_readmem,twineagl_sub_writemem)
	MDRV_CPU_VBLANK_INT(seta_sub_interrupt,SETA_SUB_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(downtown_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_1_layer)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								Ultraman Club
***************************************************************************/

static MACHINE_DRIVER_START( umanclub )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(umanclub_readmem,umanclub_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(tndrcade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(seta_no_layers)
	MDRV_VIDEO_UPDATE(seta_no_layers) /* just draw the sprites */

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END


/***************************************************************************
								U.S. Classic
***************************************************************************/


/*	usclssic lev 6 = lev 2+4 !
	Test mode shows a 16ms and 4ms counters. I wonder if every game has
	5 ints per frame
*/

static MACHINE_DRIVER_START( usclssic )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(usclssic_readmem,usclssic_writemem)
	MDRV_CPU_VBLANK_INT(calibr50_interrupt,calibr50_INTERRUPTS_NUM)

	MDRV_CPU_ADD(M65C02, 1000000)	/* ?? */
	MDRV_CPU_MEMORY(calibr50_sub_readmem,calibr50_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* NMI caused by main cpu when writing to the sound latch */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(usclssic_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32)
	MDRV_COLORTABLE_LENGTH(16*32 + 64*32)		/* sprites, layer */

	MDRV_PALETTE_INIT(usclssic)	/* layer is 6 planes deep */
	MDRV_VIDEO_START(seta_1_layer)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
#if	__X1_010_V2
	MDRV_SOUND_ADD( CUSTOM, seta_sound_intf_16MHz2 )
#else
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
#endif	//	__X1_010_V2
MACHINE_DRIVER_END


/***************************************************************************
								War of Aero
***************************************************************************/

/* wrofaero lev 2 almost identical to lev 6, but there's an additional
   call in the latter (sound related?) */

#if	!__X1_010_V2
static INTERRUPT_GEN( wrofaero_interrupt )
{
	static int enab = 0;
	switch (cpu_getiloops())
	{
		case 0:		cpu_set_irq_line(0, 2, HOLD_LINE);	break;
case 1:
	enab++;
	enab %= 2;
	if 	(enab == 0)	cpu_set_irq_line(0, 4, HOLD_LINE);
	break;

#if 0
		case 1:
		{
			int i;
			int new_enab = 0;
			int old_enab = enab;

			for(i=0;i<16;i++)
				if (seta_sound_r(i*8)&1)
					new_enab |= 1 << i;

			enab = new_enab;

			for(i=0;i<16;i++)
				if ( ((new_enab&(1<<i))==0) && (old_enab&(1<<i)) )
					cpu_set_irq_line(0, 4, HOLD_LINE);
			break;
		}

#endif
	}
}
#endif

static MACHINE_DRIVER_START( wrofaero )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
#if	__uPD71054_TIMER
	MDRV_CPU_VBLANK_INT( wrofaero_interrupt, 1 )
#else
	MDRV_CPU_VBLANK_INT(seta_interrupt_2_and_4,SETA_INTERRUPTS_NUM)
#endif	// __uPD71054_TIMER
//	MDRV_CPU_VBLANK_INT(wrofaero_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

#if	__uPD71054_TIMER
	MDRV_MACHINE_INIT( wrofaero )
	MDRV_MACHINE_STOP( wrofaero )
#endif	// __uPD71054_TIMER

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(msgundam_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512 * 3)	/* sprites, layer1, layer2 */

	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END




/***************************************************************************
								Zing Zing Zip
***************************************************************************/

/* zingzip lev 3 = lev 2 + lev 1 !
   SR = 2100 -> lev1 is ignored so we must supply int 3, since the routine
   at int 1 is necessary: it plays the background music.
*/

static MACHINE_DRIVER_START( zingzip )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(wrofaero_readmem,wrofaero_writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256 -16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1 -16)
	MDRV_GFXDECODE(zingzip_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*32+16*32+16*32)
	MDRV_COLORTABLE_LENGTH(16*32+16*32+64*32)	/* sprites, layer2, layer1 */

	MDRV_PALETTE_INIT(zingzip)				/* layer 1 gfx is 6 planes deep */
	MDRV_VIDEO_START(seta_2_layers)
	MDRV_VIDEO_UPDATE(seta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, seta_sound_intf_16MHz)
MACHINE_DRIVER_END




/***************************************************************************


								ROMs Loading


***************************************************************************/




/***************************************************************************

								Arbalester

***************************************************************************/

ROM_START( arbalest )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "uk001.03",  0x000000, 0x040000, 0xee878a2c )
	ROM_LOAD16_BYTE( "uk001.04",  0x000001, 0x040000, 0x902bb4e3 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "uk001.05", 0x006000, 0x002000, 0x0339fc53 )
	ROM_RELOAD(           0x008000, 0x002000  )
	ROM_RELOAD(           0x00a000, 0x002000  )
	ROM_RELOAD(           0x00c000, 0x002000  )
	ROM_RELOAD(           0x00e000, 0x002000  )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "uk001.06", 0x000000, 0x040000, 0x11c75746 )
	ROM_LOAD16_BYTE( "uk001.07", 0x000001, 0x040000, 0x01b166c7 )
	ROM_LOAD16_BYTE( "uk001.08", 0x080000, 0x040000, 0x78d60ba3 )
	ROM_LOAD16_BYTE( "uk001.09", 0x080001, 0x040000, 0xb4748ae0 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "uk001.10", 0x000000, 0x080000, 0xc1e2f823 )
	ROM_LOAD( "uk001.11", 0x080000, 0x080000, 0x09dfe56a )
	ROM_LOAD( "uk001.12", 0x100000, 0x080000, 0x818a4085 )
	ROM_LOAD( "uk001.13", 0x180000, 0x080000, 0x771fa164 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "uk001.15", 0x000000, 0x080000, 0xce9df5dd )
	ROM_LOAD( "uk001.14", 0x080000, 0x080000, 0x016b844a )
ROM_END

READ16_HANDLER( arbalest_protection_r )
{
	return 0;
}

DRIVER_INIT( arbalest )
{
	install_mem_read16_handler(0, 0x80000, 0x8000f, arbalest_protection_r);
}



/***************************************************************************

								Athena no Hatena?

CPU  : 68000-16
Sound: X1-010
OSC  : 16.0000MHz

ROMs:
fs001001.evn - Main programs (27c4001)
fs001002.odd /

fs001004.pcm - Samples (8M mask - read as 27c800)
fs001003.gfx - Graphics (16M mask - read as 27c160)

Chips:	X1-001A	X1-002A
		X1-004
		X1-006
		X1-007
		X1-010

***************************************************************************/

ROM_START( atehate )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fs001001.evn", 0x000000, 0x080000, 0x4af1f273 )
	ROM_LOAD16_BYTE( "fs001002.odd", 0x000001, 0x080000, 0xc7ca7a85 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fs001003.gfx", 0x000000, 0x200000, 0x8b17e431 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fs001004.pcm", 0x000000, 0x100000, 0xf9344ce5 )
ROM_END

/***************************************************************************

								Blandia

Blandia by Allumer

This set is coming from an original Blandia PCB ref : PO-078A

As usually, it use a lot of customs allumer chips !

***************************************************************************/

DRIVER_INIT ( blandia )
{
	/* rearrange the gfx data so it can be decoded in the same way as the other set */

	int rom_size;
	UINT8 *buf;
	UINT8 *rom;
	int rpos;

	rom_size = 0x80000;
	buf = malloc(rom_size);

	if (!buf) return;

	rom = memory_region(REGION_GFX2) + 0x40000;

	for (rpos = 0; rpos < rom_size/2; rpos++) {
		buf[rpos+0x40000] = rom[rpos*2];
		buf[rpos] = rom[rpos*2+1];
	}

	memcpy( rom, buf, rom_size );

	rom = memory_region(REGION_GFX3) + 0x40000;

	for (rpos = 0; rpos < rom_size/2; rpos++) {
		buf[rpos+0x40000] = rom[rpos*2];
		buf[rpos] = rom[rpos*2+1];
	}

	memcpy( rom, buf, rom_size );

	free(buf);
}

ROM_START( blandia )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ux001001.003",  0x000000, 0x040000, 0x2376a1f3 )
	ROM_LOAD16_BYTE( "ux001002.004",  0x000001, 0x040000, 0xb915e172 )
	ROM_LOAD16_WORD_SWAP( "ux001003.202",     0x100000, 0x100000, 0x98052c63 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ux001005.200",  0x300000, 0x100000, 0xbea0c4a5 )
	ROM_LOAD( "ux001007.201",  0x100000, 0x100000, 0x4440fdd1 )
	ROM_LOAD( "ux001006.063",  0x200000, 0x100000, 0xabc01cf7 )
	ROM_LOAD( "ux001008.064",  0x000000, 0x100000, 0x413647b6 )

	ROM_REGION( 0x0c0000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "ux001009.065",  0x000000, 0x080000, 0xbc6f6aea )
	ROM_LOAD( "ux001010.066",  0x040000, 0x080000, 0xbd7f7614 )

	ROM_REGION( 0x0c0000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "ux001011.067",  0x000000, 0x080000, 0x5efe0397 )
	ROM_LOAD( "ux001012.068",  0x040000, 0x080000, 0xf29959f6 )

	/* The c0000-fffff region is bankswitched */
	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* Samples */
	/* not sure this is right due to the bankswitching, will need checking */
	ROM_LOAD( "ux001013.069",  0x000000, 0x100000, 0x5cd273cd )
	ROM_LOAD( "ux001014.070",  0x100000, 0x080000, 0x86b49b4e )
ROM_END

/***************************************************************************

								Blandia (prototype)

PCB:	P0-072-2
CPU:	68000-16
Sound:	X1-010
OSC:	16.0000MHz

Chips:	X1-001A		X1-002A
		X1-004
		X1-007
		X1-010
		X1-011 x2	X1-012 x2

***************************************************************************/

ROM_START( blandiap )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg-even.bin", 0x000000, 0x040000, 0x7ecd30e8 )
	ROM_LOAD16_BYTE( "prg-odd.bin",  0x000001, 0x040000, 0x42b86c15 )
	ROM_LOAD16_BYTE( "tbl0.bin",     0x100000, 0x080000, 0x69b79eb8 )
	ROM_LOAD16_BYTE( "tbl1.bin",     0x100001, 0x080000, 0xcf2fd350 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "o-1.bin",  0x000000, 0x080000, 0x4c67b7f0 )
	ROM_LOAD16_BYTE( "o-0.bin",  0x000001, 0x080000, 0x5e7b8555 )
	ROM_LOAD16_BYTE( "o-5.bin",  0x100000, 0x080000, 0x40bee78b )
	ROM_LOAD16_BYTE( "o-4.bin",  0x100001, 0x080000, 0x7c634784 )
	ROM_LOAD16_BYTE( "o-3.bin",  0x200000, 0x080000, 0x387fc7c4 )
	ROM_LOAD16_BYTE( "o-2.bin",  0x200001, 0x080000, 0xc669bb49 )
	ROM_LOAD16_BYTE( "o-7.bin",  0x300000, 0x080000, 0xfc77b04a )
	ROM_LOAD16_BYTE( "o-6.bin",  0x300001, 0x080000, 0x92882943 )

	ROM_REGION( 0x0c0000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "v1-2.bin",  0x000000, 0x020000, 0xd524735e )
	ROM_LOAD( "v1-5.bin",  0x020000, 0x020000, 0xeb440cdb )
	ROM_LOAD( "v1-1.bin",  0x040000, 0x020000, 0x09bdf75f )
	ROM_LOAD( "v1-4.bin",  0x060000, 0x020000, 0x803911e5 )
	ROM_LOAD( "v1-0.bin",  0x080000, 0x020000, 0x73617548 )
	ROM_LOAD( "v1-3.bin",  0x0a0000, 0x020000, 0x7f18e4fb )

	ROM_REGION( 0x0c0000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "v2-2.bin",  0x000000, 0x020000, 0xc4f15638 )	// identical to v2-1
	ROM_LOAD( "v2-5.bin",  0x020000, 0x020000, 0xc2e57622 )
	ROM_LOAD( "v2-1.bin",  0x040000, 0x020000, 0xc4f15638 )
	ROM_LOAD( "v2-4.bin",  0x060000, 0x020000, 0x16ec2130 )
	ROM_LOAD( "v2-0.bin",  0x080000, 0x020000, 0x5b05eba9 )
	ROM_LOAD( "v2-3.bin",  0x0a0000, 0x020000, 0x80ad0c3b )

	/* The c0000-fffff region is bankswitched */
	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s-0.bin",  0x000000, 0x020000, 0xa5fde408 )
	ROM_CONTINUE(         0x140000, 0x020000             )
	ROM_LOAD( "s-1.bin",  0x020000, 0x020000, 0x3083f9c4 )
	ROM_CONTINUE(         0x160000, 0x020000             )
	ROM_LOAD( "s-2.bin",  0x040000, 0x020000, 0xa591c9ef )
	ROM_CONTINUE(         0x180000, 0x020000             )
	ROM_LOAD( "s-3.bin",  0x060000, 0x020000, 0x68826c9d )
	ROM_CONTINUE(         0x1a0000, 0x020000             )
	ROM_LOAD( "s-4.bin",  0x080000, 0x020000, 0x1c7dc8c2 )
	ROM_CONTINUE(         0x1c0000, 0x020000             )
	ROM_LOAD( "s-5.bin",  0x0a0000, 0x020000, 0x4bb0146a )
	ROM_CONTINUE(         0x1e0000, 0x020000             )
	ROM_LOAD( "s-6.bin",  0x100000, 0x020000, 0x9f8f34ee )	// skip c0000-fffff (banked region)
	ROM_CONTINUE(         0x200000, 0x020000             )	// this half is 0
	ROM_LOAD( "s-7.bin",  0x120000, 0x020000, 0xe077dd39 )
	ROM_CONTINUE(         0x220000, 0x020000             )	// this half is 0
ROM_END



/***************************************************************************

					Block Carnival / Thunder & Lightning 2

P0-068B, M6100723A

CPU  : MC68000B8
Sound: X1-010
OSC  : 16.000MHz

ROMs:
u1.a1 - Main programs (27c010)
u4.a3 /

bl-chr-0.j3 - Graphics (4M mask)
bl-chr-1.l3 /

bl-snd-0.a13 - Sound (4M mask)

Custom chips:	X1-001A	X1-002A
				X1-004
				X1-006
				X1-007
				X1-009
				X1-010

Other:
Lithium battery x1

***************************************************************************/

ROM_START( blockcar )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u1.a1",  0x000000, 0x020000, 0x4313fb00 )
	ROM_LOAD16_BYTE( "u4.a3",  0x000001, 0x020000, 0x2237196d )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bl-chr-0.j3",  0x000000, 0x080000, 0xa33300ca )
	ROM_LOAD( "bl-chr-1.l3",  0x080000, 0x080000, 0x563de808 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bl-snd-0.a13",  0x000000, 0x080000, 0xa92dabaf )
	ROM_RELOAD(                0x080000, 0x080000             )
ROM_END



/***************************************************************************

								Caliber 50

CPU:   TMP 68000N-8, 65C02
Other: NEC D4701

UH-001-006        SW2  SW1
UH-001-007
UH-001-008                    8464         68000-8
UH-001-009  X1-002A X1-001A   8464         Uh-002-001=T01
UH-001-010                    8464            51832
UH-001-011                    8464            51832
                                           UH-001-002
UH-001-012            X1-012               UH-001-003
UH-001-013                               UH-002-004-T02
                      X1-011               5116-10
                                           BAT
                         16MHz
             X1-010   65C02      X1-006
                      UH-001-005 X1-007
                      4701       X1-004

***************************************************************************/

ROM_START( calibr50 )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "uh002001.u45", 0x000000, 0x040000, 0xeb92e7ed )
	ROM_LOAD16_BYTE( "uh002004.u41", 0x000001, 0x040000, 0x5a0ed31e )
	ROM_LOAD16_BYTE( "uh001003.9a",  0x080000, 0x010000, 0x0d30d09f )
	ROM_LOAD16_BYTE( "uh001002.7a",  0x080001, 0x010000, 0x7aecc3f9 )

	ROM_REGION( 0x04c000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "uh001005.u61", 0x004000, 0x040000, 0x4a54c085 )
	ROM_RELOAD(               0x00c000, 0x040000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "uh001006.ux2", 0x000000, 0x080000, 0xfff52f91 )
	ROM_LOAD16_BYTE( "uh001007.ux1", 0x000001, 0x080000, 0xb6c19f71 )
	ROM_LOAD16_BYTE( "uh001008.ux6", 0x100000, 0x080000, 0x7aae07ef )
	ROM_LOAD16_BYTE( "uh001009.ux0", 0x100001, 0x080000, 0xf85da2c5 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "uh001010.u3x", 0x000000, 0x080000, 0xf986577a )
	ROM_LOAD( "uh001011.u50", 0x080000, 0x080000, 0x08620052 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "uh001013.u60", 0x000000, 0x080000, 0x09ec0df6 )
	ROM_LOAD( "uh001012.u46", 0x080000, 0x080000, 0xbb996547 )
ROM_END

/***************************************************************************

							Daioh

DAIOH
Alumer 1993, Sammy license
PO-092A


FG-001-003
FG-001-004  X1-002A X1-001A             FG-001-001
                                        FG-001-002
FG-001-005   X1-11 X1-12
FG-001-006   X1-11 X1-12
                                       68000-16
FG-001-007

   X1-10                           16MHz

                            X1-007  X1-004

***************************************************************************/

ROM_START( daioh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fg1-001",  0x000000, 0x080000, 0x104ae74a )
	ROM_LOAD16_BYTE( "fg1-002",  0x000001, 0x080000, 0xe39a4e67 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fg1-004", 0x000000, 0x100000, 0x9ab0533e )
	ROM_LOAD( "fg1-003", 0x100000, 0x100000, 0x1c9d51e2 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE ) /* Layer 1 */
	ROM_LOAD( "fg1-005",  0x000000, 0x200000, 0xc25159b9 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE ) /* Layer 2 */
	ROM_LOAD( "fg1-006",  0x000000, 0x200000, 0x2052c39a )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fg1-007",  0x000000, 0x100000, 0x4a2fe9e0 )
ROM_END

/***************************************************************************

								DownTown

***************************************************************************/

ROM_START( downtown )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ud2001.000", 0x000000, 0x040000, 0xf1965260 )
	ROM_LOAD16_BYTE( "ud2001.003", 0x000001, 0x040000, 0xe7d5fa5f )
	ROM_LOAD16_BYTE( "ud2000.002", 0x080000, 0x010000, 0xca976b24 )
	ROM_LOAD16_BYTE( "ud2000.001", 0x080001, 0x010000, 0x1708aebd )

	ROM_REGION( 0x04c000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "ud2002.004", 0x004000, 0x040000, 0xbbd538b1 )
	ROM_RELOAD(             0x00c000, 0x040000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "ud2005.t01", 0x000000, 0x080000, 0x77e6d249 )
	ROM_LOAD16_BYTE( "ud2006.t02", 0x000001, 0x080000, 0x6e381bf2 )
	ROM_LOAD16_BYTE( "ud2007.t03", 0x100000, 0x080000, 0x737b4971 )
	ROM_LOAD16_BYTE( "ud2008.t04", 0x100001, 0x080000, 0x99b9d757 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "ud2009.t05", 0x000000, 0x080000, 0xaee6c581 )
	ROM_LOAD( "ud2010.t06", 0x080000, 0x080000, 0x3d399d54 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ud2011.t07", 0x000000, 0x080000, 0x9c9ff69f )
ROM_END


/* Protection. NVRAM is handled writing commands here */
data16_t downtown_protection[0x200/2];
static READ16_HANDLER( downtown_protection_r )
{
	int job = downtown_protection[0xf8/2] & 0xff;

	switch (job)
	{
		case 0xa3:
		{
			unsigned char word[] = "WALTZ0";
			if (offset >= 0x100/2 && offset <= 0x10a/2)	return word[offset-0x100/2];
			else										return 0;
		}
		default:
			return downtown_protection[offset] & 0xff;
	}
}
static WRITE16_HANDLER( downtown_protection_w )
{
	COMBINE_DATA(&downtown_protection[offset]);
}

DRIVER_INIT( downtown )
{
	install_mem_read16_handler (0, 0x200000, 0x2001ff, downtown_protection_r);
	install_mem_write16_handler(0, 0x200000, 0x2001ff, downtown_protection_w);

	install_mem_write16_handler(0, 0x300000, 0x300001, MWA16_NOP);	// IRQ ACK?
}




/***************************************************************************

								Dragon Unit
					 [Prototype of "Castle Of Dragon"]

PCB:	P0-053-1
CPU:	68000-8
Sound:	X1-010
OSC:	16.0000MHz

Chips:	X1-001A, X1-002A, X1-004, X1-006, X1-007, X1-010, X1-011, X1-012

***************************************************************************/

ROM_START( drgnunit )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg-e.bin", 0x000000, 0x020000, 0x728447df )
	ROM_LOAD16_BYTE( "prg-o.bin", 0x000001, 0x020000, 0xb2f58ecf )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "obj-2.bin", 0x000000, 0x020000, 0xd7f6ab5a )
	ROM_LOAD16_BYTE( "obj-1.bin", 0x000001, 0x020000, 0x53a95b13 )
	ROM_LOAD16_BYTE( "obj-6.bin", 0x040000, 0x020000, 0x80b801f7 )
	ROM_LOAD16_BYTE( "obj-5.bin", 0x040001, 0x020000, 0x6b87bc20 )
	ROM_LOAD16_BYTE( "obj-4.bin", 0x080000, 0x020000, 0x60d17771 )
	ROM_LOAD16_BYTE( "obj-3.bin", 0x080001, 0x020000, 0x0bccd4d5 )
	ROM_LOAD16_BYTE( "obj-8.bin", 0x0c0000, 0x020000, 0x826c1543 )
	ROM_LOAD16_BYTE( "obj-7.bin", 0x0c0001, 0x020000, 0xcbaa7f6a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "scr-1o.bin",  0x000000, 0x020000, 0x671525db )
	ROM_LOAD( "scr-2o.bin",  0x020000, 0x020000, 0x2a3f2ed8 )
	ROM_LOAD( "scr-3o.bin",  0x040000, 0x020000, 0x4d33a92d )
	ROM_LOAD( "scr-4o.bin",  0x060000, 0x020000, 0x79a0aa61 )
	ROM_LOAD( "scr-1e.bin",  0x080000, 0x020000, 0xdc9cd8c9 )
	ROM_LOAD( "scr-2e.bin",  0x0a0000, 0x020000, 0xb6126b41 )
	ROM_LOAD( "scr-3e.bin",  0x0c0000, 0x020000, 0x1592b8c2 )
	ROM_LOAD( "scr-4e.bin",  0x0e0000, 0x020000, 0x8201681c )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd-1.bin", 0x000000, 0x020000, 0x8f47bd0d )
	ROM_LOAD( "snd-2.bin", 0x020000, 0x020000, 0x65c40ef5 )
	ROM_LOAD( "snd-3.bin", 0x040000, 0x020000, 0x71fbd54e )
	ROM_LOAD( "snd-4.bin", 0x060000, 0x020000, 0xac50133f )
	ROM_LOAD( "snd-5.bin", 0x080000, 0x020000, 0x70652f2c )
	ROM_LOAD( "snd-6.bin", 0x0a0000, 0x020000, 0x10a1039d )
	ROM_LOAD( "snd-7.bin", 0x0c0000, 0x020000, 0xdecbc8b0 )
	ROM_LOAD( "snd-8.bin", 0x0e0000, 0x020000, 0x3ac51bee )
ROM_END



/***************************************************************************

								Eight Forces

PO-079A (Same board as ZingZingZip)

CPU  : MC68000B16
Sound: X1-010
OSC  : 16.000MHz

ROMs:
uy2-u4.u3 - Main program (even)(27c2001)
uy2-u3.u4 - Main program (odd) (27c2001)

u63.bin - Sprites (HN62434, read as 27c4200)
u64.bin /

u69.bin - Samples (HN62318, read as 27c8001)
u70.bin /

u66.bin - Layer 1 (HN62418, read as 27c800)
u68.bin - Layer 2 (HN62418, read as 27c800)

PALs (not dumped):
uy-012.206 (PAL16L8A)
uy-013.14  (PAL16L8A)
uy-014.35  (PAL16L8A)
uy-015.36  (PALCE16V8)
uy-016.76  (PAL16L8A)
uy-017.116 (PAL16L8A)

Custom:		X1-001A	X1-002A
			X1-004
			X1-007
			X1-010
			X1-011 (x2)		X1-012 (x2)

***************************************************************************/

ROM_START( eightfrc )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "uy2-u4.u3",  0x000000, 0x040000, 0xf1f249c5 )
	ROM_LOAD16_BYTE( "uy2-u3.u4",  0x000001, 0x040000, 0x6f2d8618 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "u64.bin",  0x000000, 0x080000, 0xf561ff2e )
	ROM_LOAD( "u63.bin",  0x080000, 0x080000, 0x4c3f8366 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u66.bin",  0x000000, 0x100000, 0x6fad2b7f )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u68.bin",  0x000000, 0x100000, 0xc17aad22 )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u70.bin",  0x000000, 0x0c0000, 0xdfdb67a3 )
	// skip c0000-fffff (banked region)
	ROM_CONTINUE(         0x100000, 0x040000             )
	ROM_LOAD( "u69.bin",  0x140000, 0x100000, 0x82ec08f1 )
ROM_END

DRIVER_INIT( eightfrc )
{
	install_mem_read16_handler(0, 0x500004, 0x500005, MRA16_NOP);	// watchdog??
}



/***************************************************************************

								Extreme Downhill

(c)1995 Sammy
DH-01
PO-117A (board is made by Seta/Allumer)

CPU  : MC68HC000B16
Sound: X1-010
OSC: 16.0000MHz (X1), 14.3180MHz (X2)

ROMs:
fw001002.201 - Main program (even) (Macronics 27c4000)
fw001001.200 - Main program (odd)  (Macronics 27c4000)

fw001005.205 - (32pin mask, read as 27c8001)
fw001007.026 /

fw001003.202 - (42pin mask, read as 27c160)
fw001004.206 |
fw001006.152 /

PALs (16L8ACN, not dumped):
FW-001
FW-002
FW-003
FW-005

Custom chips:	X1-001A		X1-002A
				X1-004
				X1-007
				X1-010
				X1-011 (x2)	X1-012 (x2)

***************************************************************************/

ROM_START( extdwnhl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fw001002.201",  0x000000, 0x080000, 0x24d21924 )
	ROM_LOAD16_BYTE( "fw001001.200",  0x000001, 0x080000, 0xfb12a28b )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fw001003.202", 0x000000, 0x200000, 0xac9b31d5 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD       ( "fw001004.206", 0x000000, 0x200000, 0x0dcb1d72 )
	ROM_LOAD16_BYTE( "fw001005.205", 0x200000, 0x100000, 0x5c33b2f1 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "fw001006.152",  0x000000, 0x200000, 0xd00e8ddd )	// FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fw001007.026",  0x080000, 0x080000, 0x16d84d7a )	// swapped halves
	ROM_CONTINUE(              0x000000, 0x080000             )
ROM_END


/***************************************************************************

									GundHara

(C) 1995 Banpresto
Seta/Allumer Hardware

PCB: BP954KA
PCB: PO-120A
CPU: TMP68HC000N16 (68000, 64 pin DIP)
SND: ?
OSC: 16.000MHz
RAM: 6264 x 8, 62256 x 4
DIPS: 2 x 8 position
Other Chips:	PALs x 6 (not dumped)
				NEC 71054C
				X1-004
				X1-007
				X1-010
				X1-011 x2	X1-012 x2
				X1-001A		X1-002A

On PCB near JAMMA connector is a small push button to reset the PCB.

ROMS:
BPGH-001.102	27C040
BPGH-002.103	27C4000
BPGH-003.U3		27C4000
BPGH-004.U4		23C4000
BPGH-005.200	23C16000
BPGH-006.201	23C16000
BPGH-007.U63	23C16000
BPGH-008.U64	23C16000
BPGH-009.U65	27C4000
BPGH-010.U66	TC538200
BPGH-011.U67	TC538000
BPGH-012.U68	TC5316200
BPGH-013.U70	TC538000

***************************************************************************/

ROM_START( gundhara )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bpgh-003.u3",  0x000000, 0x080000, 0x14e9970a )
	ROM_LOAD16_BYTE( "bpgh-004.u4",  0x000001, 0x080000, 0x96dfc658 )
	ROM_LOAD16_BYTE( "bpgh-002.103", 0x100000, 0x080000, 0x312f58e2 )
	ROM_LOAD16_BYTE( "bpgh-001.102", 0x100001, 0x080000, 0x8d23a23c )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bpgh-008.u64", 0x000000, 0x200000, 0x7ed9d272 )
	ROM_LOAD( "bpgh-006.201", 0x200000, 0x200000, 0x5a81411d )
	ROM_LOAD( "bpgh-007.u63", 0x400000, 0x200000, 0xaa49ce7b )
	ROM_LOAD( "bpgh-005.200", 0x600000, 0x200000, 0x74138266 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD       ( "bpgh-010.u66", 0x000000, 0x100000, 0xb742f0b8 )
	ROM_LOAD16_BYTE( "bpgh-009.u65", 0x100000, 0x080000, 0xb768e666 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD       ( "bpgh-012.u68", 0x000000, 0x200000, 0xedfda595 )
	ROM_LOAD16_BYTE( "bpgh-011.u67", 0x200000, 0x100000, 0x49aff270 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bpgh-013.u70",  0x080000, 0x080000, 0x0fa5d503 )	// swapped halves
	ROM_CONTINUE(              0x000000, 0x080000             )
ROM_END


/***************************************************************************

								J.J. Squawkers

68HC000N -16N

2)   Alumer  X1-012
2)   Alumer  X1-011
2)   Alumer  X1-014

X1-010
X1-007
X1-004
16.000MHz

NEC 71054C  ----???

***************************************************************************/

ROM_START( jjsquawk )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "jj-rom1.040", 0x000000, 0x040000, 0x7b9af960 )
	ROM_CONTINUE   (                0x100000, 0x040000             )
	ROM_LOAD16_BYTE( "jj-rom2.040", 0x000001, 0x040000, 0x47dd71a3 )
	ROM_CONTINUE   (                0x100001, 0x040000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "jj-rom9",  0x000000, 0x080000, 0x27441cd3 )
	ROM_LOAD( "jj-rom10", 0x080000, 0x080000, 0xca2b42c4 )
	ROM_LOAD( "jj-rom7",  0x100000, 0x080000, 0x62c45658 )
	ROM_LOAD( "jj-rom8",  0x180000, 0x080000, 0x2690c57b )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD       ( "jj-rom11",    0x000000, 0x080000, 0x98b9f4b4 )
	ROM_LOAD       ( "jj-rom12",    0x080000, 0x080000, 0xd4aa916c )
	ROM_LOAD16_BYTE( "jj-rom3.040", 0x100000, 0x080000, 0xa5a35caf )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD       ( "jj-rom14",    0x000000, 0x080000, 0x274bbb48 )
	ROM_LOAD       ( "jj-rom13",    0x080000, 0x080000, 0x51e29871 )
	ROM_LOAD16_BYTE( "jj-rom4.040", 0x100000, 0x080000, 0xa235488e )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "jj-rom5.040", 0x000000, 0x080000, 0xd99f2879 )
	ROM_LOAD( "jj-rom6.040", 0x080000, 0x080000, 0x9df1e478 )
ROM_END

/***************************************************************************

								Kamen Rider
Kamen Riderclub Battleracer
Banpresto, 1993

Runs on Seta/Allumer hardware

PCB No: BP934KA   PO-096A
CPU   : MC68HC000B16
OSC   : 16.000MHz
RAM   : LH5160D-10L (x9), CXK58257AP-10L (x2)
DIPSW : 8 position (x2)
CUSTOM: X1-010
        X1-007
        X1-004
        X1-011 (x2)
        X1-012 (x2)
        X1-002A
        X1-001A
OTHER : NEC71054C, some PALs

ROMs  :
        FJ001007.152	27c4096     near X1-011 & X1-010 (sound program?)
        FJ001008.26     8M Mask     connected to X1-010, near FJ001007
        FJ001003.25     27c4096     main program for 68k
        FJ001006.22     16M Mask    gfx
        FJ001005.21     16M Mask    gfx

***************************************************************************/

ROM_START( kamenrid )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "fj001003.25", 0x000000, 0x080000, 0x9b65d1b9 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fj001005.21", 0x000000, 0x100000, 0x5d031333 )
	ROM_LOAD( "fj001006.22", 0x100000, 0x100000, 0xcf28eb78 )

	ROM_REGION( 0x80000, REGION_USER1, ROMREGION_DISPOSE )	/* Layers 1+2 */
	ROM_LOAD( "fj001007.152", 0x000000, 0x080000, 0xd9ffe80b )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x040000 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_COPY( REGION_USER1, 0x040000, 0x000000, 0x040000 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fj001008.26", 0x000000, 0x100000, 0x45e2b329 )
ROM_END

/***************************************************************************

								Krazy Bowl

PCB:	SKB-001
		PO-114A

FV   FV                           2465
001  001                          2465           X1-005
004  003      X1-002A  X1-001A
                                       58257     FV
                                                 001
                                                 002 (even)
                                       58257
                  14.318MHz                      FV
                                                 001
FV 001 005                                       001 (odd)
FV 001 006
  2465                                      68HC000B16
                 NEC4701  NEC4701

X1-010           X1-006
                 X1-007      X1-004

***************************************************************************/

ROM_START( krzybowl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fv001.002", 0x000000, 0x040000, 0x8c03c75f )
	ROM_LOAD16_BYTE( "fv001.001", 0x000001, 0x040000, 0xf0630beb )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fv001.003", 0x000000, 0x080000, 0x7de22749 )
	ROM_LOAD( "fv001.004", 0x080000, 0x080000, 0xc7d2fe32 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fv001.005", 0x000000, 0x080000, 0x5e206062 )
	ROM_LOAD( "fv001.006", 0x080000, 0x080000, 0x572a15e7 )
ROM_END




/***************************************************************************

									Meta Fox

(Seta 1990)

P0-045A

P1-006-163                    8464   68000-8
P1-007-164    X1-002A X1-001A 8464
P1-008-165                    8464
P1-009-166                    8464     256K-12
                                       256K-12

                 X1-012
                 X1-011


   2063    X1-010     X1-006     X0-006
                      X1-007
                      X1-004     X1-004

----------------------
P1-036-A

UP-001-010
UP-001-011
UP-001-012
UP-001-013


UP-001-014
UP-001-015

-----------------------
P1-049-A

              UP-001-001
              UP-001-002
              P1-003-161
              P1-004-162


              UP-001-005
              x

***************************************************************************/

ROM_START( metafox )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "p1003161", 0x000000, 0x040000, 0x4fd6e6a1 )
	ROM_LOAD16_BYTE( "p1004162", 0x000001, 0x040000, 0xb6356c9a )
	ROM_LOAD16_BYTE( "up001002", 0x080000, 0x010000, 0xce91c987 )
	ROM_LOAD16_BYTE( "up001001", 0x080001, 0x010000, 0x0db7a505 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "up001005", 0x006000, 0x002000, 0x2ac5e3e3 )
	ROM_RELOAD(           0x008000, 0x002000  )
	ROM_RELOAD(           0x00a000, 0x002000  )
	ROM_RELOAD(           0x00c000, 0x002000  )
	ROM_RELOAD(           0x00e000, 0x002000  )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "p1006163", 0x000000, 0x040000, 0x80f69c7c )
	ROM_LOAD16_BYTE( "p1007164", 0x000001, 0x040000, 0xd137e1a3 )
	ROM_LOAD16_BYTE( "p1008165", 0x080000, 0x040000, 0x57494f2b )
	ROM_LOAD16_BYTE( "p1009166", 0x080001, 0x040000, 0x8344afd2 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "up001010", 0x000000, 0x080000, 0xbfbab472 )
	ROM_LOAD( "up001011", 0x080000, 0x080000, 0x26cea381 )
	ROM_LOAD( "up001012", 0x100000, 0x080000, 0xfed2c5f9 )
	ROM_LOAD( "up001013", 0x180000, 0x080000, 0xadabf9ea )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "up001015", 0x000000, 0x080000, 0x2e20e39f )
	ROM_LOAD( "up001014", 0x080000, 0x080000, 0xfca6315e )
ROM_END


DRIVER_INIT( metafox )
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* This game uses the 21c000-21cfff area for protecton? */
	install_mem_read16_handler (0, 0x200000, 0x2001ff, MRA16_NOP);
	install_mem_write16_handler(0, 0x200000, 0x2001ff, MWA16_NOP);

	RAM[0x8ab1c/2] = 0x0000;	// patch protection test: "cp error"
	RAM[0x8ab1e/2] = 0x0000;
	RAM[0x8ab20/2] = 0x0000;
}



/***************************************************************************

							Mobile Suit Gundam

Banpresto 1993
P0-081A
                               SW2  SW1

FA-001-008                          FA-001-001
FA-001-007    X1-002A X1-001A       FA-002-002
                              5160
                              5160
                                        71054
FA-001-006                    5160     62256
FA-001-005    X1-011  X1-012  5160     62256

FA-001-004    X1-011  X1-012  5160
5160                          5160

                                68000-16

                                         16MHz
  X1-010
                    X1-007   X1-004     X1-005

***************************************************************************/

ROM_START( msgundam )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "fa003002.u25",  0x000000, 0x080000, 0x1cc72d4c )
	ROM_LOAD16_WORD_SWAP( "fa001001.u20",  0x100000, 0x100000, 0xfca139d0 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fa001008.u21",  0x000000, 0x200000, 0xe7accf48 )
	ROM_LOAD( "fa001007.u22",  0x200000, 0x200000, 0x793198a6 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "fa001006.u23",  0x000000, 0x100000, 0x3b60365c )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "fa001005.u24",  0x000000, 0x080000, 0x8cd7ff86 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fa001004.u26",  0x000000, 0x100000, 0xb965f07c )
ROM_END


ROM_START( msgunda1 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "fa002002.u25",  0x000000, 0x080000, 0xdee3b083 )
	ROM_LOAD16_WORD_SWAP( "fa001001.u20",  0x100000, 0x100000, 0xfca139d0 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fa001008.u21",  0x000000, 0x200000, 0xe7accf48 )
	ROM_LOAD( "fa001007.u22",  0x200000, 0x200000, 0x793198a6 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "fa001006.u23",  0x000000, 0x100000, 0x3b60365c )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "fa001005.u24",  0x000000, 0x080000, 0x8cd7ff86 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fa001004.u26",  0x000000, 0x100000, 0xb965f07c )
ROM_END




/***************************************************************************

							Oishii Puzzle Ha Irimasenka

PCB  : PO-097A
CPU  : 68000
Sound: X1-010
OSC  : 14.31818MHz

All ROMs are 23c4000

Custom chips:	X1-001A	X1-002A
				X1-004
				X1-007
				X1-010
				X1-011 (x2)	X1-012 (x2)

***************************************************************************/

ROM_START( oisipuzl )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "ss1u200.v10", 0x000000, 0x080000, 0xf5e53baf )
	/* Gap of 0x80000 bytes */
	ROM_LOAD16_WORD_SWAP( "ss1u201.v10", 0x100000, 0x080000, 0x7a7ff5ae )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "ss1u306.v10", 0x000000, 0x080000, 0xce43a754 )
	ROM_LOAD( "ss1u307.v10", 0x080000, 0x080000, 0x2170b7ec )
	ROM_LOAD( "ss1u304.v10", 0x100000, 0x080000, 0x546ab541 )
	ROM_LOAD( "ss1u305.v10", 0x180000, 0x080000, 0x2a33e08b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "ss1u23.v10",  0x000000, 0x080000, 0x9fa60901 )
	ROM_LOAD( "ss1u24.v10",  0x080000, 0x080000, 0xc10eb4b3 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "ss1u25.v10",  0x000000, 0x080000, 0x56840728 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ss1u26.v10", 0x000000, 0x080000, 0xd452336b )
	ROM_LOAD( "ss1u27.v10", 0x080000, 0x080000, 0x17fe921d )
ROM_END

/***************************************************************************

							Triple Fun

Triple Fun
??, 19??


CPU   : TMP68HC000P-16 (68000)
SOUND : OKI M6295
DIPSW : 8 position (x2)
XTAL  : 16.000 MHz (8MHz written on PCB, located near OKI chip)
        14.31818MHz (near 68000)
RAM   : 62256 (x2), 6264 (x8), 2018 (x14)
PROMs : None
PALs  : PALCE16V8H (x13)
OTHER : TPC1020AFN-084C (84 pin PLCC)

ROMs  :

04.bin + 05.bin    Main Program
01.bin             Sound Program
02.bin + 03.bin    OKI Samples
06.bin to 11.bin   GFX


Developers:
           More info reqd? Redump needed? Email me....
           theguru@emuunlim.com

***************************************************************************/

ROM_START( triplfun )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	/* the program fails its self-check but thats probably because
	   its a bootleg, it does the same on the real board */
	ROM_LOAD16_BYTE( "05.bin", 0x000000, 0x40000, 0x06eb3821 )
	ROM_CONTINUE(0x100000,0x40000)
	ROM_LOAD16_BYTE( "04.bin", 0x000001, 0x40000, 0x37a5c46e )
	ROM_CONTINUE(0x100001,0x40000)

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "08.bin", 0x000001, 0x80000, 0x63a8f10f )
	ROM_LOAD16_BYTE( "09.bin", 0x000000, 0x80000, 0x98cc8ca5 )
	ROM_LOAD16_BYTE( "10.bin", 0x100001, 0x80000, 0x20b0f282 )
	ROM_LOAD16_BYTE( "11.bin", 0x100000, 0x80000, 0x276ef724 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "02.bin", 0x000000, 0x80000, 0x4c0d1068 )
	ROM_LOAD16_BYTE( "03.bin", 0x000001, 0x80000, 0xdba94e18 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "06.bin", 0x000000, 0x40000, 0x8944bb72 )
	ROM_LOAD16_BYTE( "07.bin", 0x000001, 0x40000, 0x934a5d91 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "01.bin", 0x000000, 0x40000, 0xc186a930 )
ROM_END

/***************************************************************************

							Pro Mahjong Kiwame

PCB  : PO-101-1 (the board is made by Allumer/Seta)
CPU  : TMP68301AF-16 (68000 core)
Sound: X1-010
OSC  : 20.0000MHz

ROMs:
fp001001.bin - Main program (27c2001, even)
fp001002.bin - Main program (27c2001, odd)
fp001003.bin - Graphics (23c4000)
fp001005.bin - Samples (27c4000, high)
fp001006.bin - Samples (27c4000, low)

Chips:	X1-001A
		X1-002A
		X1-004
		X1-006
		X1-007
		X1-010

- To initialize high scores, power-on holding start button in service mode

***************************************************************************/

ROM_START( kiwame )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fp001001.bin", 0x000000, 0x040000, 0x31b17e39 )
	ROM_LOAD16_BYTE( "fp001002.bin", 0x000001, 0x040000, 0x5a6e2efb )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fp001003.bin", 0x000000, 0x080000, 0x0f904421 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fp001006.bin", 0x000000, 0x080000, 0x96cf395d )
	ROM_LOAD( "fp001005.bin", 0x080000, 0x080000, 0x65b5fe9a )
ROM_END

DRIVER_INIT( kiwame )
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* WARNING: This game writes to the interrupt vector
	   table. Lev 1 routine address is stored at $100 */

	RAM[0x64/2] = 0x0000;
	RAM[0x66/2] = 0x0dca;
}


/***************************************************************************

								Quiz Kokology

(c)1992 Tecmo

PO-053A

CPU  : MC68000B8
Sound: X1-010
OSC  : 16.000MHz

Custom chips:	X1-001A	X1-002A
				X1-004
				X1-006	X1-007
				X1-010
				X1-011	X1-012

***************************************************************************/

ROM_START( qzkklogy )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "3.u27", 0x000000, 0x020000, 0xb8c27cde )
	ROM_LOAD16_BYTE( "1.u9",  0x000001, 0x020000, 0xce01cd54 )
	ROM_LOAD16_BYTE( "4.u33", 0x040000, 0x020000, 0x4f5c554c )
	ROM_LOAD16_BYTE( "2.u17", 0x040001, 0x020000, 0x65fa1b8d )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "t2709u32.u32", 0x000000, 0x080000, 0x900f196c ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD( "t2709u26.u26", 0x080000, 0x080000, 0x416ac849 ) // FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "t2709u42.u39", 0x000000, 0x080000, 0x194d5704 )
	ROM_LOAD( "t2709u39.u42", 0x080000, 0x080000, 0x6f95a76d )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "t2709u47.u47", 0x000000, 0x080000, 0x0ebdad40 )
	ROM_LOAD( "t2709u55.u55", 0x080000, 0x080000, 0x43960c68 )
ROM_END


/***************************************************************************

								Quiz Koko-logy 2

(c)1992 Tecmo

P0-100A

CPU  : MC68HC000B16
Sound: X1-010
OSC  : 16.000MHz

FN001001.106 - Main program (27C4096)
FN001003.107 / (40pin 2M mask)

FN001004.100 - OBJ chr. (42pin mask)
FN001005.104 - BG chr. (42pin mask)
FN001006.105 - Samples (32pin mask)

Custom chips:	X1-001A		X1-002A
				X1-004
				X1-006
				X1-007
				X1-010
				X1-011		X1-012

***************************************************************************/

ROM_START( qzkklgy2 )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "fn001001.106", 0x000000, 0x080000, 0x7bf8eb17 )
	ROM_LOAD16_WORD_SWAP( "fn001003.107", 0x080000, 0x040000, 0xee6ef111 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "fn001004.100", 0x000000, 0x100000, 0x5ba139a2 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "fn001005.104", 0x000000, 0x200000, 0x95726a63 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "fn001006.105", 0x000000, 0x100000, 0x83f201e6 )
ROM_END


/***************************************************************************

								Rezon (Japan)

PCB 	: PO-063A
CPU 	: TOSHIBA TMP68HC000N-16
Sound	: X1-010
OSC 	: 16.000MHz
Other	: Allumer
			X1-001A			X1-002A
			X1-004
			X1-007
			X1-011 x 2		X1-012 x 2

***************************************************************************/

ROM_START( rezon )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "us001001.u3",  0x000000, 0x020000, 0xab923052 )
	ROM_LOAD16_BYTE( "us001002.u4",  0x000001, 0x020000, 0x3dafa0d5 )
	/* empty gap */
	ROM_LOAD16_BYTE( "us001004.103", 0x100000, 0x020000, 0x54871c7c ) // 1xxxxxxxxxxxxxxxx = 0x00
	ROM_LOAD16_BYTE( "us001003.102", 0x100001, 0x020000, 0x1ac3d272 ) // 1xxxxxxxxxxxxxxxx = 0x00

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "us001006.u64",  0x000000, 0x080000, 0xa4916e96 )
	ROM_LOAD( "us001005.u63",  0x080000, 0x080000, 0xe6251ebc )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "us001007.u66",  0x000000, 0x080000, 0x3760b935 ) // 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "us001008.u68",  0x000000, 0x080000, 0x0ab73910 ) // 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "us001009.u70",  0x000000, 0x100000, 0x0d7d2e2b )
ROM_END

DRIVER_INIT( rezon )
{
	install_mem_read16_handler(0, 0x500006, 0x500007, MRA16_NOP);	// irq ack?
}


/***************************************************************************

							Sokonuke Taisen Game (Japan)

(c)1995 Sammy

CPU:	68HC000
Sound:	All PCM ?
OSC:	16MHz

***************************************************************************/

ROM_START( sokonuke )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "001-001.bin",  0x000000, 0x080000, 0x9d0aa3ca )
	ROM_LOAD16_BYTE( "001-002.bin",  0x000001, 0x080000, 0x96f2ef5f )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "001-003.bin", 0x000000, 0x200000, 0xab9ba897 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD       ( "001-004.bin", 0x000000, 0x100000, 0x34ca3540 )
	ROM_LOAD16_BYTE( "001-005.bin", 0x100000, 0x080000, 0x2b95d68d )

	ROM_REGION( 0x100, REGION_GFX3, ROMREGION_ERASE | ROMREGION_DISPOSE )	/* Layer 2 */
	/* Unused */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "001-006.bin", 0x000000, 0x100000, 0xecfac767 )
ROM_END


/***************************************************************************

								Strike Gunner

(c)1991 Athena (distributed by Tecmo)

PO-053A

CPU  : TMP68000N-8
Sound: X1-010
OSC  : 16.000MHz

Custom chips:	X1-001A	X1-002A
				X1-004
				X1-006	X1-007
				X1-010
				X1-011	X1-012

***************************************************************************/

ROM_START( stg )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "att01003.u27", 0x000000, 0x020000, 0x7a640a93 )
	ROM_LOAD16_BYTE( "att01001.u9",  0x000001, 0x020000, 0x4fa88ad3 )
	ROM_LOAD16_BYTE( "att01004.u33", 0x040000, 0x020000, 0xbbd45ca1 ) // 1xxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "att01002.u17", 0x040001, 0x020000, 0x2f8fd80c ) // 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "att01006.u32", 0x000000, 0x080000, 0x6ad78ea2 )
	ROM_LOAD( "att01005.u26", 0x080000, 0x080000, 0xa347ff00 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "att01008.u39", 0x000000, 0x080000, 0x20c47457 ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD( "att01007.u42", 0x080000, 0x080000, 0xac975544 ) // FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "att01009.u47", 0x000000, 0x080000, 0x4276b58d )
	ROM_LOAD( "att01010.u55", 0x080000, 0x080000, 0xfffb2f53 )
ROM_END



/***************************************************************************

							Thunder & Lightning

Location      Device      File ID      Checksum
-----------------------------------------------
U1  1A        27C256        M4           C18C   [ MAIN PROG ] [ EVEN ]
U4  3A        27C256        M5           12E1   [ MAIN PROG ] [ ODD  ]
U29 10A      23C4001        R27          37F2   [   HIGH    ]
U39 12A      23C4001        R28          0070   [   LOW     ]
U6  2K       23C1000        T14          1F7D   [   C40     ]
U9  4K       23C1000        T15          7A15   [   C30     ]
U14 5K       23C1000        T16          BFFD   [   C20     ]
U20 7K       23C1000        T17          7AE7   [   C10     ]

PCB: PO055D

CPU: 68000 8MHz

Custom:	X1-001A		X1-002A
		X1-004
		X1-006
		X1-007
		X1-010

***************************************************************************/

ROM_START( thunderl )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "m4", 0x000000, 0x008000, 0x1e6b9462 )
	ROM_LOAD16_BYTE( "m5", 0x000001, 0x008000, 0x7e82793e )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "t17", 0x000000, 0x020000, 0x599a632a )
	ROM_LOAD16_BYTE( "t16", 0x000001, 0x020000, 0x3aeef91c )
	ROM_LOAD16_BYTE( "t15", 0x040000, 0x020000, 0xb97a7b56 )
	ROM_LOAD16_BYTE( "t14", 0x040001, 0x020000, 0x79c707be )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "r28", 0x000000, 0x080000, 0xa043615d )
	ROM_LOAD( "r27", 0x080000, 0x080000, 0xcb8425a3 )
ROM_END



/***************************************************************************

						Thundercade / Twin Formation

CPU: HD68000PS8
SND: YM3812, YM2203C
OSC: 16MHz

This PCB is loaded with custom SETA chips as follows
X1-001 (also has written YM3906)
X1-002 (also has written YM3909)
X1-003
X1-004
X1-006

Rom code is UAO, M/B code is M6100287A (the TAITO logo is written also)

P0-029-A

  UA0-4 UA0-3 4364 UA0-2 UA0-1 4364  X1-001  16MHz  X1-002
  68000-8
                         4364 4364   UA0-9  UA0-8  UA0-7  UA0-6
                                     UA0-13 UA0-12 UA0-11 UA0-10
     X0-006
  UA10-5 2016 YM3812 YM2203  SW1
                             SW2                   X1-006
                                     X1-004
                                                 X1-003

***************************************************************************/

ROM_START( tndrcade )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ua0-4.1l", 0x000000, 0x020000, 0x73bd63eb )
	ROM_LOAD16_BYTE( "ua0-2.1h", 0x000001, 0x020000, 0xe96194b1 )
	ROM_LOAD16_BYTE( "ua0-3.1k", 0x040000, 0x020000, 0x0a7b1c41 )
	ROM_LOAD16_BYTE( "ua0-1.1g", 0x040001, 0x020000, 0xfa906626 )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "ua10-5.8m", 0x004000, 0x020000, 0x8eff6122 )	// $1fffd=2 (country code)
	ROM_RELOAD(            0x00c000, 0x020000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ua0-10", 0x000000, 0x040000, 0xaa7b6757 )
	ROM_LOAD( "ua0-11", 0x040000, 0x040000, 0x11eaf931 )
	ROM_LOAD( "ua0-12", 0x080000, 0x040000, 0x00b5381c )
	ROM_LOAD( "ua0-13", 0x0c0000, 0x040000, 0x8f9a0ed3 )
	ROM_LOAD( "ua0-6",  0x100000, 0x040000, 0x14ecc7bb )
	ROM_LOAD( "ua0-7",  0x140000, 0x040000, 0xff1a4e68 )
	ROM_LOAD( "ua0-8",  0x180000, 0x040000, 0x936e1884 )
	ROM_LOAD( "ua0-9",  0x1c0000, 0x040000, 0xe812371c )
ROM_END


ROM_START( tndrcadj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ua0-4.1l", 0x000000, 0x020000, 0x73bd63eb )
	ROM_LOAD16_BYTE( "ua0-2.1h", 0x000001, 0x020000, 0xe96194b1 )
	ROM_LOAD16_BYTE( "ua0-3.1k", 0x040000, 0x020000, 0x0a7b1c41 )
	ROM_LOAD16_BYTE( "ua0-1.1g", 0x040001, 0x020000, 0xfa906626 )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "thcade5.bin", 0x004000, 0x020000, 0x8cb9df7b )	// $1fffd=1 (country code jp)
	ROM_RELOAD(              0x00c000, 0x020000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ua0-10", 0x000000, 0x040000, 0xaa7b6757 )
	ROM_LOAD( "ua0-11", 0x040000, 0x040000, 0x11eaf931 )
	ROM_LOAD( "ua0-12", 0x080000, 0x040000, 0x00b5381c )
	ROM_LOAD( "ua0-13", 0x0c0000, 0x040000, 0x8f9a0ed3 )
	ROM_LOAD( "ua0-6",  0x100000, 0x040000, 0x14ecc7bb )
	ROM_LOAD( "ua0-7",  0x140000, 0x040000, 0xff1a4e68 )
	ROM_LOAD( "ua0-8",  0x180000, 0x040000, 0x936e1884 )
	ROM_LOAD( "ua0-9",  0x1c0000, 0x040000, 0xe812371c )
ROM_END


/***************************************************************************

								Twin Eagle

M6100326A	Taito (Seta)

ua2-4              68000
ua2-3
ua2-6
ua2-5
ua2-8
ua2-10
ua2-7               ua2-1
ua2-9
ua2-12
ua2-11              ua2-2

***************************************************************************/

ROM_START( twineagl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "ua2-1", 0x000000, 0x080000, 0x5c3fe531 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "ua2-2", 0x006000, 0x002000, 0x783ca84e )
	ROM_RELOAD(        0x008000, 0x002000  )
	ROM_RELOAD(        0x00a000, 0x002000  )
	ROM_RELOAD(        0x00c000, 0x002000  )
	ROM_RELOAD(        0x00e000, 0x002000  )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "ua2-4",  0x000000, 0x040000, 0x8b7532d6 )
	ROM_LOAD16_BYTE( "ua2-3",  0x000001, 0x040000, 0x1124417a )
	ROM_LOAD16_BYTE( "ua2-6",  0x080000, 0x040000, 0x99d8dbba )
	ROM_LOAD16_BYTE( "ua2-5",  0x080001, 0x040000, 0x6e450d28 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "ua2-8",  0x000000, 0x080000, 0x7d3a8d73 )
	ROM_LOAD( "ua2-10", 0x080000, 0x080000, 0x5bbe1f56 )
	ROM_LOAD( "ua2-7",  0x100000, 0x080000, 0xfce56907 )
	ROM_LOAD( "ua2-9",  0x180000, 0x080000, 0xa451eae9 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ua2-11", 0x000000, 0x080000, 0x624e6057 )
	ROM_LOAD( "ua2-12", 0x080000, 0x080000, 0x3068ff64 )
ROM_END


READ16_HANDLER( twineagl_protection_r )
{
	return 0;
}

/* Extra RAM ? Check code at 0x00ba90 */
READ16_HANDLER( twineagl_200100_r )
{
	const data16_t data[8] =
	{
		0x004d,0x0054,0x0058,0x0044,0x0041,0x0059,0x004f,0x004e
	};

	if (readinputport(4) & 1)
		return (data[offset]);

	return 0;
}

DRIVER_INIT( twineagl )
{
	int i;
	unsigned char *RAM = memory_region(REGION_GFX2);	// Layer

	/* Protection? */
	install_mem_read16_handler (0, 0x800000, 0x8000ff, twineagl_protection_r);

	/* This allows 2 simultaneous players and the use of the "Copyright" Dip Switch. */
	install_mem_read16_handler (0, 0x200100, 0x20010f, twineagl_200100_r);


#if 1
	// waterfalls: tiles 3e00-3fff must be a copy of 2e00-2fff ??
	for (i = 0x3e00 * (16*16*2/8); i < 0x3f00 * (16*16*2/8); i++)
	{
		RAM[i+0x000000] = RAM[i+0x000000 - 0x700*(16*16*2/8)];
		RAM[i+0x100000] = RAM[i+0x100000 - 0x700*(16*16*2/8)];
	}


	// Sea level:  tiles 3e00-3fff must be a copy of 3700-38ff ??
	for (i = 0x3f00 * (16*16*2/8); i < 0x4000 * (16*16*2/8); i++)
	{
		RAM[i+0x000000] = RAM[i+0x000000 - 0x1000*(16*16*2/8)];
		RAM[i+0x100000] = RAM[i+0x100000 - 0x1000*(16*16*2/8)];
	}

#endif
}




/***************************************************************************

								Ultraman Club

Banpresto, 1992
Board looks similar to Castle of Dragon PCB.

PCB No: PO-077A (Seta Number)
        BP922   (Banpresto Number)

CPU: MC68HC000B16
OSC: 16.000MHz
DIP SW x 2 (8 position)

RAM: Sharp LH5160D-10L x 3, Hitachi S256KLP-12 x 2
PALs (2 x PAL16L8, not dumped)
SETA Chips:	X1-010
			X1-004
			X1-007
			X1-006
			X1-002A
			X1-001A

Controls are 8 way Joystick and 2 buttons.

ROMs:

UW001006.U48      27C010                                               \  Main Program
UW001007.U49      27C010                                               /

BP-U-001.U1       4M mask (40 pin, 512k x 8), read as MX27C4100        \  GFX
BP-U-002.U2       4M mask (40 pin, 512k x 8), read as MX27C4100        /

BP-U-003.U13      8M mask (32 pin, 1M x 8),   read as MX27C8000           Sound


***************************************************************************/

ROM_START( umanclub )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "uw001006.u48", 0x000000, 0x020000, 0x3dae1e9d )
	ROM_LOAD16_BYTE( "uw001007.u49", 0x000001, 0x020000, 0x5c21e702 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bp-u-002.u2", 0x000000, 0x080000, 0x936cbaaa )
	ROM_LOAD( "bp-u-001.u1", 0x080000, 0x080000, 0x87813c48 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "uw003.u13", 0x000000, 0x100000, 0xe2f718eb )
ROM_END




/***************************************************************************

								U.S. Classic

M6100430A (Taito 1989)

       u7 119  u6 118   u5 117   u4 116
                                         68000-8
u13  120                                 000
u19  121                                 001
u21  122                                 002
u29  123                                 003
u33  124
u40  125
u44  126
u51  127
u58  128
u60  129                                 65c02
u68  130
u75  131                                 u61 004

                                         u83 132

***************************************************************************/

ROM_START( usclssic )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ue2001.u20", 0x000000, 0x020000, 0x18b41421 )
	ROM_LOAD16_BYTE( "ue2000.u14", 0x000001, 0x020000, 0x69454bc2 )
	ROM_LOAD16_BYTE( "ue2002.u22", 0x040000, 0x020000, 0xa7bbe248 )
	ROM_LOAD16_BYTE( "ue2003.u30", 0x040001, 0x020000, 0x29601906 )

	ROM_REGION( 0x04c000, REGION_CPU2, 0 )		/* 65c02 Code */
	ROM_LOAD( "ue002u61.004", 0x004000, 0x040000, 0x476e9f60 )
	ROM_RELOAD(               0x00c000, 0x040000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "ue001009.119", 0x000000, 0x080000, 0xdc065204 )
	ROM_LOAD16_BYTE( "ue001008.118", 0x000001, 0x080000, 0x5947d9b5 )
	ROM_LOAD16_BYTE( "ue001007.117", 0x100000, 0x080000, 0xb48a885c )
	ROM_LOAD16_BYTE( "ue001006.116", 0x100001, 0x080000, 0xa6ab6ef4 )

	ROM_REGION( 0x600000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "ue001010.120", 0x000000, 0x080000, 0xdd683031 )	// planes 01
	ROM_LOAD( "ue001011.121", 0x080000, 0x080000, 0x0e27bc49 )
	ROM_LOAD( "ue001012.122", 0x100000, 0x080000, 0x961dfcdc )
	ROM_LOAD( "ue001013.123", 0x180000, 0x080000, 0x03e9eb79 )

	ROM_LOAD( "ue001014.124", 0x200000, 0x080000, 0x9576ace7 )	// planes 23
	ROM_LOAD( "ue001015.125", 0x280000, 0x080000, 0x631d6eb1 )
	ROM_LOAD( "ue001016.126", 0x300000, 0x080000, 0xf44a8686 )
	ROM_LOAD( "ue001017.127", 0x380000, 0x080000, 0x7f568258 )

	ROM_LOAD( "ue001018.128", 0x400000, 0x080000, 0x4bd98f23 )	// planes 45
	ROM_LOAD( "ue001019.129", 0x480000, 0x080000, 0x6d9f5a33 )
	ROM_LOAD( "ue001020.130", 0x500000, 0x080000, 0xbc07403f )
	ROM_LOAD( "ue001021.131", 0x580000, 0x080000, 0x98c03efd )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ue001005.132", 0x000000, 0x080000, 0xc5fea37c )
ROM_END
/***************************************************************************

							   War of Aero
							Project M E I O U

93111A	YANG CHENG

CPU   : TOSHIBA TMP68HC000N-16
Sound : Allumer X1-010
OSC   : 16.000000MHz
Other : Allumer
			X1-001A  X1-002A
			X1-004
			X1-007
			X1-011 x 2
			X1-012 x 2
		NEC
			C324C
			D71054C

***************************************************************************/

ROM_START( wrofaero )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u3.bin",  0x000000, 0x040000, 0x9b896a97 )
	ROM_LOAD16_BYTE( "u4.bin",  0x000001, 0x040000, 0xdda84846 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "u64.bin",  0x000000, 0x080000, 0xf06ccd78 )
	ROM_LOAD( "u63.bin",  0x080000, 0x080000, 0x2a602a1b )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u66.bin",  0x000000, 0x080000, 0xc9fc6a0c )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u68.bin",  0x000000, 0x080000, 0x25c0c483 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u69.bin",  0x000000, 0x080000, 0x957ecd41 )
	ROM_LOAD( "u70.bin",  0x080000, 0x080000, 0x8d756fdf )
ROM_END




/***************************************************************************

									Wit's

(c)1989 Athena (distributed by Visco)
P0-055B (board is made by Seta)

CPU  : TMP68000N-8
Sound: X1-010
OSC  : 16.000MHz

ROMs:
UN001001.U1 - Main program (27256)
UN001002.U4 - Main program (27256)

UN001003.10A - Samples (28pin mask)
UN001004.12A /

UN001005.2L - Graphics (28pin mask)
UN001006.4L |
UN001007.5L |
UN001008.7L /

Custom chips:	X1-001A		X1-002A
				X1-004 (x2)
				X1-006
				X1-007
				X1-010

***************************************************************************/

ROM_START( wits )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "un001001.u1", 0x000000, 0x008000, 0x416c567e )
	ROM_LOAD16_BYTE( "un001002.u4", 0x000001, 0x008000, 0x497a3fa6 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "un001008.7l", 0x000000, 0x020000, 0x1d5d0b2b )
	ROM_LOAD16_BYTE( "un001007.5l", 0x000001, 0x020000, 0x9e1e6d51 )
	ROM_LOAD16_BYTE( "un001006.4l", 0x040000, 0x020000, 0x98a980d4 )
	ROM_LOAD16_BYTE( "un001005.2l", 0x040001, 0x020000, 0x6f2ce3c0 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "un001004.12a", 0x000000, 0x020000, 0xa15ff938 )
	ROM_LOAD( "un001003.10a", 0x020000, 0x020000, 0x3f4b9e55 )
ROM_END



/***************************************************************************

								Zing Zing Zip

P0-079A

UY-001-005   X1-002A   X1-001A   5168-10      256k-12
UY-001-006                       5168-10      UY-001-001
UY-001-007                                    UY-001-002
UY-001-008   X1-011 X1-012                    58257-12
                                 5168-10
UY-001-010   X1-011 X1-012       5168-10
UY-001-017
UY-001-018
                                 5168-10
X1-010                           5168-10       68000-16


                           8464-80
                           8464-80       16MHz


                             X1-007    X1-004

***************************************************************************/

ROM_START( zingzip )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "uy001001.3",	0x000000, 0x040000, 0x1a1687ec )
	ROM_LOAD16_BYTE( "uy001002.4",	0x000001, 0x040000, 0x62e3b0c4 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "uy001006.64",		0x000000, 0x080000, 0x46e4a7d8 )
	ROM_LOAD( "uy001005.63",		0x080000, 0x080000, 0x4aac128e )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "uy001008.66",		0x000000, 0x100000, 0x1dff7c4b ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD16_BYTE( "uy001007.65",	0x100000, 0x080000, 0xec5b3ab9 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "uy001010.68",		0x000000, 0x100000, 0xbdbcdf03 ) // FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "uy001011.70",		0x000000, 0x100000, 0xbd845f55 ) // uy001017 + uy001018
ROM_END



/***************************************************************************

								Game Drivers

***************************************************************************/

/* Working Games: */

GAMEX( 1987, tndrcade, 0,        tndrcade, tndrcade, 0,        ROT270, "[Seta] (Taito license)", "Thundercade / Twin Formation",   GAME_IMPERFECT_SOUND ) // Title/License: DSW
GAMEX( 1987, tndrcadj, tndrcade, tndrcade, tndrcadj, 0,        ROT270, "[Seta] (Taito license)", "Tokusyu Butai UAG (Japan)",      GAME_IMPERFECT_SOUND ) // License: DSW
GAMEX( 1988, twineagl, 0,        twineagl, twineagl, twineagl, ROT270, "Seta / Taito",   "Twin Eagle - Revenge Joe's Brother",     GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1989, calibr50, 0,        calibr50, calibr50, 0,        ROT270, "Athena / Seta",          "Caliber 50",                     GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1989, drgnunit, 0,        drgnunit, drgnunit, 0,        ROT0,   "Seta",                   "Dragon Unit / Castle of Dragon", GAME_IMPERFECT_SOUND )
GAMEX( 1989, downtown, 0,        downtown, downtown, downtown, ROT270, "Seta",                   "DownTown",                       GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1989, usclssic, 0,        usclssic, usclssic, 0,        ROT270, "Seta",                   "U.S. Classic",                   GAME_IMPERFECT_SOUND | GAME_WRONG_COLORS ) // Country/License: DSW
GAMEX( 1989, arbalest, 0,        metafox,  arbalest, arbalest, ROT270, "Seta",                   "Arbalester",                     GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1989, metafox,  0,        metafox,  metafox,  metafox,  ROT270, "Seta",                   "Meta Fox",                       GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1989, wits,     0,        wits,     wits,     0,        ROT0,   "Athena (Visco license)", "Wit's (Japan)",                  GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1990, thunderl, 0,        thunderl, thunderl, 0,        ROT270, "Seta",                   "Thunder & Lightning",            GAME_IMPERFECT_SOUND ) // Country/License: DSW
GAMEX( 1991, rezon,    0,        rezon,    rezon,    rezon,    ROT0,   "Allumer",                "Rezon",                          GAME_IMPERFECT_SOUND )
GAMEX( 1991, stg,      0,        drgnunit, stg,      0,        ROT270, "Athena / Tecmo",         "Strike Gunner S.T.G",            GAME_IMPERFECT_SOUND )
GAMEX( 1992, blandia,  0,        blandia,  blandia,  blandia,  ROT0,   "Allumer",                "Blandia",          			   GAME_IMPERFECT_SOUND )
GAMEX( 1992, blandiap, blandia,  blandiap, blandia,  0,        ROT0,   "Allumer",                "Blandia (prototype)",            GAME_IMPERFECT_SOUND )
GAMEX( 1992, blockcar, 0,        blockcar, blockcar, 0,        ROT90,  "Visco",                  "Block Carnival / Thunder & Lightning 2", GAME_IMPERFECT_SOUND ) // Title: DSW
GAMEX( 1992, qzkklogy, 0,        drgnunit, qzkklogy, 0,        ROT0,   "Tecmo",                  "Quiz Kokology",                  GAME_IMPERFECT_SOUND )
GAMEX( 1992, umanclub, 0,        umanclub, umanclub, 0,        ROT0,   "Tsuburaya Prod. / Banpresto", "Ultraman Club - Tatakae! Ultraman Kyoudai!!", GAME_IMPERFECT_SOUND )
GAMEX( 1992, zingzip,  0,        zingzip,  zingzip,  0,        ROT270, "Allumer + Tecmo",        "Zing Zing Zip",                  GAME_IMPERFECT_SOUND )
GAMEX( 1993, atehate,  0,        atehate,  atehate,  0,        ROT0,   "Athena",                 "Athena no Hatena ?",             GAME_IMPERFECT_SOUND )
GAMEX( 1993, daioh,    0,        daioh,    daioh,    0,        ROT270, "Athena",                 "Daioh",                          GAME_IMPERFECT_SOUND )
GAMEX( 1993, msgundam, 0,        msgundam, msgundam, 0,        ROT0,   "Banpresto",              "Mobile Suit Gundam",             GAME_IMPERFECT_SOUND )
GAMEX( 1993, oisipuzl, 0,        oisipuzl, oisipuzl, 0,        ROT0,   "Sunsoft + Atlus",        "Oishii Puzzle Ha Irimasenka",    GAME_IMPERFECT_SOUND )
GAME ( 1993, triplfun, oisipuzl, triplfun, oisipuzl, 0,        ROT0,   "bootleg",                "Triple Fun"                                           )
GAMEX( 1993, qzkklgy2, 0,        qzkklgy2, qzkklgy2, 0,        ROT0,   "Tecmo",                  "Quiz Kokology 2",                GAME_IMPERFECT_SOUND )
GAMEX( 1993, wrofaero, 0,        wrofaero, wrofaero, 0,        ROT270, "Yang Cheng",             "War of Aero - Project MEIOU",    GAME_IMPERFECT_SOUND )
GAMEX( 1993, jjsquawk, 0,        jjsquawk, jjsquawk, 0,        ROT0,   "Athena / Able",          "J. J. Squawkers",                GAME_IMPERFECT_SOUND )
GAMEX( 1993, kamenrid, 0,        kamenrid, kamenrid, 0,        ROT0,   "Toei / Banpresto",       "Masked Riders Club Battle Race", GAME_IMPERFECT_SOUND )
GAMEX( 1994, eightfrc, 0,        eightfrc, eightfrc, eightfrc, ROT90,  "Tecmo",                  "Eight Forces",                   GAME_IMPERFECT_SOUND )
GAMEX( 1994, kiwame,   0,        kiwame,   kiwame,   kiwame,   ROT0,   "Athena",                 "Pro Mahjong Kiwame",             GAME_IMPERFECT_SOUND )
GAMEX( 1994, krzybowl, 0,        krzybowl, krzybowl, 0,        ROT270, "American Sammy Corp.",   "Krazy Bowl",                     GAME_IMPERFECT_SOUND )
GAMEX( 1995, extdwnhl, 0,        extdwnhl, extdwnhl, 0,        ROT0,   "Sammy Industries Japan", "Extreme Downhill (v1.5)",        GAME_IMPERFECT_SOUND )
GAMEX( 1995, gundhara, 0,        gundhara, gundhara, 0,        ROT270, "Banpresto",              "Gundhara",                       GAME_IMPERFECT_SOUND )
GAMEX( 1995, sokonuke, 0,        extdwnhl, sokonuke, 0,        ROT0,   "Sammy Industries",       "Sokonuke Taisen Game (Japan)",   GAME_IMPERFECT_SOUND )

/* Nearly Working Games: */

GAMEX( 1993, msgunda1, msgundam, msgundam, msgundam, 0,        ROT0,   "Banpresto",              "Mobile Suit Gundam (alternate)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
