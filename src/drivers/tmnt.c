/***************************************************************************

TODO:

- detatwin:
  - sprites are left on screen during attract mode
  - sprite priorities are wrong
  - the "telescopic tower" in the next to last level is wrong
  - end sequence has one sprite with wrong colors and priority
  - collision detection is handled by a protection chip. Its emulation might
    not be 100% accurate.
- sprite priorities in ssriders (protection)
- sprite colors / zoomed placement in tmnt2 (protection)
- is IPT_VBLANK really vblank or something else? Investigate.
- shadows: they should not be drawn as opaque sprites, instead they should
  make the background darker
- wrong sprites in ssriders at the end of the saloon level. They have the
  "shadow" bit on.
- sprite/sprite priority has to be orthogonal to sprite/tile priority. Examples:
  - woman coming from behind corner in punkshot DownTown level
  - ship coming out of a hangar in lgtnfght ice level
- sprite/tilemap priority in lgtnfght is not 100%
  see smoke under space ship in attract animation. This is the only place where
    priority is wrong so I don't know what happens.
- there seems to be a priority problem in the ending sequence of TMNT2 (clouds
  appearing as solid rectangles in front of sprites). I'm certainly not going to
  fix this unless someone finds a way to jump straight to that point wihout
  playing the full game!
- sprite lag, quite evident in lgtnfght and mia but also in the others. Also
  see the left corner in punkshot DownTown level
- some slowdowns in lgtnfght when there are many sprites on screen - vblank issue?
- 053260 sound emulation for
Thunder Cross II
Asterix

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/eeprom.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"


void tmnt_paletteram_w(int offset,int data);
void tmnt_0a0000_w(int offset,int data);
void punkshot_0a0020_w(int offset,int data);
void lgtnfght_0a0018_w(int offset,int data);
void detatwin_700300_w(int offset,int data);
void glfgreat_122000_w(int offset,int data);
void ssriders_1c0300_w(int offset,int data);
void tmnt_priority_w(int offset,int data);
int mia_vh_start(void);
int tmnt_vh_start(void);
int punkshot_vh_start(void);
void punkshot_vh_stop(void);
int lgtnfght_vh_start(void);
void lgtnfght_vh_stop(void);
int detatwin_vh_start(void);
void detatwin_vh_stop(void);
int glfgreat_vh_start(void);
void glfgreat_vh_stop(void);
int xmen_vh_start(void);
void xmen_vh_stop(void);
void mia_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void tmnt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void punkshot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void lgtnfght_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void glfgreat_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void ssriders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void xmen_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static int tmnt_soundlatch;


static int K052109_word_r(int offset)
{
	offset >>= 1;
	return K052109_r(offset + 0x2000) | (K052109_r(offset) << 8);
}

static void K052109_word_w(int offset,int data)
{
	offset >>= 1;
	if ((data & 0xff000000) == 0)
		K052109_w(offset,(data >> 8) & 0xff);
	if ((data & 0x00ff0000) == 0)
		K052109_w(offset + 0x2000,data & 0xff);
}

static int K052109_halfword_r(int offset)
{
	return K052109_r(offset >> 1);
}

static void K052109_halfword_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
		K052109_w(offset >> 1,data & 0xff);
}

static int K052109_word_noA12_r(int offset)
{
	/* some games have the A12 line not connected, so the chip spans */
	/* twice the memory range, with mirroring */
	offset = ((offset & 0x6000) >> 1) | (offset & 0x0fff);
	return K052109_word_r(offset);
}

static void K052109_word_noA12_w(int offset,int data)
{
	/* some games have the A12 line not connected, so the chip spans */
	/* twice the memory range, with mirroring */
	offset = ((offset & 0x6000) >> 1) | (offset & 0x0fff);
	K052109_word_w(offset,data);
}

/* the interface with the 053245 is weird. The chip can address only 0x800 bytes */
/* of RAM, but they put 0x4000 there. The CPU can access them all. Address lines */
/* A1, A5 and A6 don't go to the 053245. */
static int K053245_scattered_word_r(int offset)
{
	if (offset & 0x0062)
		return READ_WORD(&spriteram[offset]);
	else
	{
		offset = ((offset & 0x001c) >> 1) | ((offset & 0x3f80) >> 3);
		return K053245_word_r(offset);
	}
}

static void K053245_scattered_word_w(int offset,int data)
{
	if (offset & 0x0062)
		COMBINE_WORD_MEM(&spriteram[offset],data);
	else
	{
		offset = ((offset & 0x001c) >> 1) | ((offset & 0x3f80) >> 3);
//if (errorlog && (offset&0xf) == 0)
//	fprintf(errorlog,"%04x: write %02x to spriteram %04x\n",cpu_get_pc(),data,offset);
		K053245_word_w(offset,data);
	}
}

static int K053244_halfword_r(int offset)
{
	return K053244_r(offset >> 1);
}

static void K053244_halfword_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
		K053244_w(offset >> 1,data & 0xff);
}

static int K053244_word_noA1_r(int offset)
{
	offset &= ~2;	/* handle mirror address */

	return K053244_r(offset/2 + 1) | (K053244_r(offset/2) << 8);
}

static void K053244_word_noA1_w(int offset,int data)
{
	offset &= ~2;	/* handle mirror address */

	if ((data & 0xff000000) == 0)
		K053244_w(offset/2,(data >> 8) & 0xff);
	if ((data & 0x00ff0000) == 0)
		K053244_w(offset/2 + 1,data & 0xff);
}

static void K053251_halfword_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
		K053251_w(offset >> 1,data & 0xff);
}

static void K053251_halfword_swap_w(int offset,int data)
{
	if ((data & 0xff000000) == 0)
		K053251_w(offset >> 1,(data >> 8) & 0xff);
}

static int K054000_halfword_r(int offset)
{
	return K054000_r(offset >> 1);
}

static void K054000_halfword_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
		K054000_w(offset >> 1,data & 0xff);
}




static int punkshot_interrupt(void)
{
	if (K052109_is_IRQ_enabled()) return m68_level4_irq();
	else return ignore_interrupt();

}

static int lgtnfght_interrupt(void)
{
	if (K052109_is_IRQ_enabled()) return m68_level5_irq();
	else return ignore_interrupt();

}



void tmnt_sound_command_w(int offset,int data)
{
	soundlatch_w(0,data & 0xff);
}

static int punkshot_sound_r(int offset)
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_ReadReg(2 + offset/2);
	else return 0x80;
}

static int detatwin_sound_r(int offset)
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_ReadReg(2 + offset/2);
	else return offset ? 0xfe : 0x00;
}

static int glfgreat_sound_r(int offset)
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_ReadReg(2 + offset/2) << 8;
	else return 0;
}

static void glfgreat_sound_w(int offset,int data)
{
	if ((data & 0xff000000) == 0)
		K053260_WriteReg(offset >> 1,(data >> 8) & 0xff);

	if (offset == 2) cpu_cause_interrupt(1,0xff);
}

static int tmnt2_sound_r(int offset)
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_ReadReg(2 + offset/2);
	else return offset ? 0x00 : 0x80;
}


int tmnt_sres_r(int offset)
{
	return tmnt_soundlatch;
}

void tmnt_sres_w(int offset,int data)
{
	/* bit 1 resets the UPD7795C sound chip */
	if ((data & 0x02) == 0)
	{
		UPD7759_reset_w(0,(data & 0x02) >> 1);
	}

	/* bit 2 plays the title music */
	if (data & 0x04)
	{
		if (!sample_playing(0))	sample_start(0,0,0);
	}
	else sample_stop(0);
	tmnt_soundlatch = data;
}


static int tmnt_decode_sample(void)
{
	int i;
	signed short *dest;
	unsigned char *source = Machine->memory_region[6];
	struct GameSamples *samples;


	if ((Machine->samples = malloc(sizeof(struct GameSamples))) == NULL)
		return 1;

	samples = Machine->samples;

	if ((samples->sample[0] = malloc(sizeof(struct GameSample) + (0x40000)*sizeof(short))) == NULL)
		return 1;

	samples->sample[0]->length = 0x40000*2;
	samples->sample[0]->smpfreq = 20000;	/* 20 kHz */
	samples->sample[0]->resolution = 16;
	dest = (signed short *)samples->sample[0]->data;
	samples->total = 1;

	/*	Sound sample for TMNT.D05 is stored in the following mode:
	 *
	 *	Bit 15-13:	Exponent (2 ^ x)
	 *	Bit 12-4 :	Sound data (9 bit)
	 *
	 *	(Sound info courtesy of Dave <dayvee@rocketmail.com>)
	 */

	for (i = 0;i < 0x40000;i++)
	{
		int val = source[2*i] + source[2*i+1] * 256;
		int exp = val >> 13;

	  	val = (val >> 4) & (0x1ff);	/* 9 bit, Max Amplitude 0x200 */
		val -= 0x100;					/* Centralize value	*/

		val <<= exp;

		dest[i] = val;
	}

	/*	The sample is now ready to be used.  It's a 16 bit, 22khz sample.
	 */

	return 0;
}

static int sound_nmi_enabled;

static void sound_nmi_callback( int param )
{
	cpu_set_nmi_line( 1, ( sound_nmi_enabled ) ? CLEAR_LINE : ASSERT_LINE );

	sound_nmi_enabled = 0;
}

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static void sound_arm_nmi(int offset,int data)
{
//	sound_nmi_enabled = 1;
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}



static int punkshot_kludge(int offset)
{
	/* I don't know what's going on here; at one point, the code reads location */
	/* 0xffffff, and returning 0 causes the game to mess up - locking up in a */
	/* loop where the ball is continuously bouncing from the basket. Returning */
	/* a random number seems to prevent that. */
	return rand();
}

static int ssriders_kludge(int offset)
{
    int data = cpu_readmem24_word(0x105a0a);

    if (errorlog) fprintf(errorlog,"%06x: read 1c0800 (D7=%02x 105a0a=%02x)\n",cpu_get_pc(),cpu_get_reg(M68K_D7),data);

    if (data == 0x075c) data = 0x0064;

    if ( cpu_readmem24_word(cpu_get_pc()) == 0x45f9 )
	{
        data = -( ( cpu_get_reg(M68K_D7) & 0xff ) + 32 );
        data = ( ( data / 8 ) & 0x1f ) * 0x40;
        data += ( ( ( cpu_get_reg(M68K_D6) & 0xffff ) + ( K052109_r(0x1a01) * 256 )
				+ K052109_r(0x1a00) + 96 ) / 8 ) & 0x3f;
    }

    return data;
}



/***************************************************************************

  EEPROM

***************************************************************************/

static int init_eeprom_count;


static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static void ssriders_eeprom_init(void)
{
	EEPROM_init(&eeprom_interface);
	init_eeprom_count = 0;
}

static int detatwin_coin_r(int offset)
{
	int res;
	static int toggle;

	/* bit 3 is service button */
	/* bit 6 is ??? VBLANK? OBJMPX? */
	res = input_port_2_r(0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7;
	}
	toggle ^= 0x40;
	return res ^ toggle;
}

static int detatwin_eeprom_r(int offset)
{
	int res;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	res = EEPROM_read_bit() | input_port_3_r(0);
	return res;
}

static int ssriders_eeprom_r(int offset)
{
	int res;
	static int toggle;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	/* bit 2 is VBLANK (???) */
	/* bit 7 is service button */
	res = EEPROM_read_bit() | input_port_3_r(0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0x7f;
	}
	toggle ^= 0x04;
	return res ^ toggle;
}

static void ssriders_eeprom_w(int offset,int data)
{
	/* bit 0 is data */
	/* bit 1 is cs (active low) */
	/* bit 2 is clock (active high) */
	EEPROM_write_bit(data & 0x01);
	EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 5 selects sprite ROM for testing in TMNT2 */
	K053244_bankselect((data & 0x20) >> 5);
}

static int xmen_eeprom_r(int offset)
{
	int res;

if (errorlog) fprintf(errorlog,"%06x eeprom_r\n",cpu_get_pc());
	/* bit 6 is EEPROM data */
	/* bit 7 is EEPROM ready */
	/* bit 14 is service button */
	res = (EEPROM_read_bit() << 6) | input_port_2_r(0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xbfff;
	}
	return res;
}

static void xmen_eeprom_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%06x: write %04x to 108000\n",cpu_get_pc(),data);
	if ((data & 0x00ff0000) == 0)
	{
		/* bit 0 = coin counter */
		coin_counter_w(0,data & 0x01);

		/* bit 2 is data */
		/* bit 3 is clock (active high) */
		/* bit 4 is cs (active low) */
		EEPROM_write_bit(data & 0x04);
		EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
	else
	{
		/* bit 8 = enable sprite ROM reading */
		K053246_set_OBJCHA_line((data & 0x0100) ? ASSERT_LINE : CLEAR_LINE);
		/* bit 9 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x0200) ? ASSERT_LINE : CLEAR_LINE);
	}
}



static struct MemoryReadAddress mia_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x040000, 0x043fff, MRA_BANK3 },	/* main RAM */
	{ 0x060000, 0x063fff, MRA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, paletteram_word_r },
	{ 0x0a0000, 0x0a0001, input_port_0_r },
	{ 0x0a0002, 0x0a0003, input_port_1_r },
	{ 0x0a0004, 0x0a0005, input_port_2_r },
	{ 0x0a0010, 0x0a0011, input_port_3_r },
	{ 0x0a0012, 0x0a0013, input_port_4_r },
	{ 0x0a0018, 0x0a0019, input_port_5_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x140000, 0x140007, K051937_word_r },
	{ 0x140400, 0x1407ff, K051960_word_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mia_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x040000, 0x043fff, MWA_BANK3 },	/* main RAM */
	{ 0x060000, 0x063fff, MWA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_w, &paletteram },
	{ 0x0a0000, 0x0a0001, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0011, watchdog_reset_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
	{ 0x140000, 0x140007, K051937_word_w },
	{ 0x140400, 0x1407ff, K051960_word_w },
//	{ 0x10e800, 0x10e801, MWA_NOP }, ???
#if 0
	{ 0x0c0000, 0x0c0001, tmnt_priority_w },
#endif
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress tmnt_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x063fff, MRA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, paletteram_word_r },
	{ 0x0a0000, 0x0a0001, input_port_0_r },
	{ 0x0a0002, 0x0a0003, input_port_1_r },
	{ 0x0a0004, 0x0a0005, input_port_2_r },
	{ 0x0a0006, 0x0a0007, input_port_3_r },
	{ 0x0a0010, 0x0a0011, input_port_4_r },
	{ 0x0a0012, 0x0a0013, input_port_5_r },
	{ 0x0a0014, 0x0a0015, input_port_6_r },
	{ 0x0a0018, 0x0a0019, input_port_7_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x140000, 0x140007, K051937_word_r },
	{ 0x140400, 0x1407ff, K051960_word_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress tmnt_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x063fff, MWA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_w, &paletteram },
	{ 0x0a0000, 0x0a0001, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0011, watchdog_reset_w },
	{ 0x0c0000, 0x0c0001, tmnt_priority_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
//	{ 0x10e800, 0x10e801, MWA_NOP }, ???
	{ 0x140000, 0x140007, K051937_word_w },
	{ 0x140400, 0x1407ff, K051960_word_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress punkshot_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x083fff, MRA_BANK1 },	/* main RAM */
	{ 0x090000, 0x090fff, paletteram_word_r },
	{ 0x0a0000, 0x0a0001, input_port_0_r },
	{ 0x0a0002, 0x0a0003, input_port_1_r },
	{ 0x0a0004, 0x0a0005, input_port_3_r },
	{ 0x0a0006, 0x0a0007, input_port_2_r },
	{ 0x0a0040, 0x0a0043, punkshot_sound_r },	/* K053260 */
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x110000, 0x110007, K051937_word_r },
	{ 0x110400, 0x1107ff, K051960_word_r },
	{ 0xfffffc, 0xffffff, punkshot_kludge },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress punkshot_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x083fff, MWA_BANK1 },	/* main RAM */
	{ 0x090000, 0x090fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x0a0020, 0x0a0021, punkshot_0a0020_w },
	{ 0x0a0040, 0x0a0041, K053260_WriteReg },
	{ 0x0a0060, 0x0a007f, K053251_halfword_w },
	{ 0x0a0080, 0x0a0081, watchdog_reset_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
	{ 0x110000, 0x110007, K051937_word_w },
	{ 0x110400, 0x1107ff, K051960_word_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress lgtnfght_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x080fff, paletteram_word_r },
	{ 0x090000, 0x093fff, MRA_BANK1 },	/* main RAM */
	{ 0x0a0000, 0x0a0001, input_port_0_r },
	{ 0x0a0002, 0x0a0003, input_port_1_r },
	{ 0x0a0004, 0x0a0005, input_port_2_r },
	{ 0x0a0006, 0x0a0007, input_port_3_r },
	{ 0x0a0008, 0x0a0009, input_port_4_r },
	{ 0x0a0010, 0x0a0011, input_port_5_r },
	{ 0x0a0020, 0x0a0023, punkshot_sound_r },	/* K053260 */
	{ 0x0b0000, 0x0b3fff, K053245_scattered_word_r },
	{ 0x0c0000, 0x0c001f, K053244_word_noA1_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress lgtnfght_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x080fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x090000, 0x093fff, MWA_BANK1 },	/* main RAM */
	{ 0x0a0018, 0x0a0019, lgtnfght_0a0018_w },
	{ 0x0a0020, 0x0a0021, K053260_WriteReg },
	{ 0x0a0028, 0x0a0029, watchdog_reset_w },
	{ 0x0b0000, 0x0b3fff, K053245_scattered_word_w, &spriteram },
	{ 0x0c0000, 0x0c001f, K053244_word_noA1_w },
	{ 0x0e0000, 0x0e001f, K053251_halfword_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
	{ -1 }	/* end of table */
};


static void ssriders_soundkludge_w(int offset,int data)
{
	/* I think this is more than just a trigger */
	cpu_cause_interrupt(1,0xff);
}

static struct MemoryReadAddress detatwin_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x180000, 0x183fff, K052109_word_r },
	{ 0x204000, 0x207fff, MRA_BANK1 },	/* main RAM */
	{ 0x300000, 0x303fff, K053245_scattered_word_r },
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ 0x500000, 0x50003f, K054000_halfword_r },
	{ 0x680000, 0x68001f, K053244_word_noA1_r },
	{ 0x700000, 0x700001, input_port_0_r },
	{ 0x700002, 0x700003, input_port_1_r },
	{ 0x700004, 0x700005, detatwin_coin_r },
	{ 0x700006, 0x700007, detatwin_eeprom_r },
	{ 0x780600, 0x780603, detatwin_sound_r },	/* K053260 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress detatwin_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x180000, 0x183fff, K052109_word_w },
	{ 0x204000, 0x207fff, MWA_BANK1 },	/* main RAM */
	{ 0x300000, 0x303fff, K053245_scattered_word_w, &spriteram },
	{ 0x500000, 0x50003f, K054000_halfword_w },
	{ 0x400000, 0x400fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x680000, 0x68001f, K053244_word_noA1_w },
	{ 0x700200, 0x700201, ssriders_eeprom_w },
	{ 0x700400, 0x700401, watchdog_reset_w },
	{ 0x700300, 0x700301, detatwin_700300_w },
	{ 0x780600, 0x780601, K053260_WriteReg },
	{ 0x780604, 0x780605, ssriders_soundkludge_w },
	{ 0x780700, 0x78071f, K053251_halfword_w },
	{ -1 }	/* end of table */
};


static int ball(int offset)
{
	return 0x11;
}

static struct MemoryReadAddress glfgreat_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_r },
	{ 0x108000, 0x108fff, paletteram_word_r },
	{ 0x10c000, 0x10cfff, MRA_BANK2 },	/* 053936? */
	{ 0x114000, 0x11401f, K053244_halfword_r },
	{ 0x120000, 0x120001, input_port_0_r },
	{ 0x120002, 0x120003, input_port_1_r },
	{ 0x120004, 0x120005, input_port_3_r },
	{ 0x120006, 0x120007, input_port_2_r },
	{ 0x121000, 0x121001, ball },	/* protection? returning 0, every shot is a "water" */
	{ 0x125000, 0x125003, glfgreat_sound_r },	/* K053260 */
	{ 0x200000, 0x207fff, K052109_word_noA12_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress glfgreat_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_w, &spriteram },
	{ 0x108000, 0x108fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x10c000, 0x10cfff, MWA_BANK2 },	/* 053936? */
	{ 0x110000, 0x11001f, K053244_word_noA1_w },	/* duplicate! */
	{ 0x114000, 0x11401f, K053244_halfword_w },		/* duplicate! */
	{ 0x118000, 0x11801f, MWA_NOP },	/* 053936 control? */
	{ 0x11c000, 0x11c01f, K053251_halfword_swap_w },
	{ 0x122000, 0x122001, glfgreat_122000_w },
	{ 0x124000, 0x124001, watchdog_reset_w },
	{ 0x125000, 0x125003, glfgreat_sound_w },	/* K053260 */
	{ 0x200000, 0x207fff, K052109_word_noA12_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress tmnt2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x104000, 0x107fff, MRA_BANK1 },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram_word_r },
	{ 0x180000, 0x183fff, K053245_scattered_word_r },
	{ 0x1c0000, 0x1c0001, input_port_0_r },
	{ 0x1c0002, 0x1c0003, input_port_1_r },
	{ 0x1c0004, 0x1c0005, input_port_4_r },
	{ 0x1c0006, 0x1c0007, input_port_5_r },
	{ 0x1c0100, 0x1c0101, input_port_2_r },
	{ 0x1c0102, 0x1c0103, ssriders_eeprom_r },
	{ 0x1c0400, 0x1c0401, watchdog_reset_r },
	{ 0x1c0500, 0x1c057f, MRA_BANK3 },	/* TMNT2 only (1J); unknown */
//	{ 0x1c0800, 0x1c0801, ssriders_kludge },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_r },
	{ 0x5c0600, 0x5c0603, tmnt2_sound_r },	/* K053260 */
	{ 0x600000, 0x603fff, K052109_word_r },
	{ -1 }	/* end of table */
};

static unsigned char *tmnt2_1c0800,*sunset_104000;

void tmnt2_1c0800_w( int offset, int data )
{
    COMBINE_WORD_MEM( &tmnt2_1c0800[offset], data );
    if ( offset == 0x0010 && ( ( READ_WORD( &tmnt2_1c0800[0x10] ) & 0xff00 ) == 0x8200 ) )
	{
		unsigned int CellSrc;
		unsigned int CellVar;
		unsigned char *src;
		int dst;
		int x,y;

		CellVar = ( READ_WORD( &tmnt2_1c0800[0x08] ) | ( READ_WORD( &tmnt2_1c0800[0x0A] ) << 16 ) );
		dst = ( READ_WORD( &tmnt2_1c0800[0x04] ) | ( READ_WORD( &tmnt2_1c0800[0x06] ) << 16 ) );
		CellSrc = ( READ_WORD( &tmnt2_1c0800[0x00] ) | ( READ_WORD( &tmnt2_1c0800[0x02] ) << 16 ) );
//        if ( CellDest >= 0x180000 && CellDest < 0x183fe0 ) {
        CellVar -= 0x104000;
		src = &Machine->memory_region[0][CellSrc];

		cpu_writemem24_word(dst+0x00,0x8000 | ((READ_WORD(src+2) & 0xfc00) >> 2));	/* size, flip xy */
        cpu_writemem24_word(dst+0x04,READ_WORD(src+0));	/* code */
        cpu_writemem24_word(dst+0x18,(READ_WORD(src+2) & 0x3ff) ^		/* color, mirror, priority */
				(READ_WORD( &sunset_104000[CellVar + 0x00] ) & 0x0060));

		/* base color modifier */
		/* TODO: this is wrong, e.g. it breaks the explosions when you kill an */
		/* enemy, or surfs in the sewer level (must be blue for all enemies). */
		/* It fixes the enemies, though, they are not all purple when you throw them around. */
		/* Also, the bosses don't blink when they are about to die - don't know */
		/* if this is correct or not. */
//		if (READ_WORD( &sunset_104000[CellVar + 0x2a] ) & 0x001f)
//			cpu_writemem24_word(dst+0x18,(cpu_readmem24_word(dst+0x18) & 0xffe0) |
//					(READ_WORD( &sunset_104000[CellVar + 0x2a] ) & 0x001f));

		x = READ_WORD(src+4);
		if (READ_WORD( &sunset_104000[CellVar + 0x00] ) & 0x4000)
		{
			/* flip x */
			cpu_writemem24_word(dst+0x00,cpu_readmem24_word(dst+0x00) ^ 0x1000);
			x = -x;
		}
		x += READ_WORD( &sunset_104000[CellVar + 0x0C] );
		cpu_writemem24_word(dst+0x0c,x);
		y = READ_WORD(src+6);
		y += READ_WORD( &sunset_104000[CellVar + 0x0E] );
		/* don't do second offset for shadows */
		if ((READ_WORD(&tmnt2_1c0800[0x10]) & 0x00ff) != 0x01)
			y += READ_WORD( &sunset_104000[CellVar + 0x10] );
		cpu_writemem24_word(dst+0x08,y);
#if 0
if (errorlog) fprintf(errorlog,"copy command %04x sprite %08x data %08x: %04x%04x %04x%04x  modifiers %08x:%04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x\n",
	READ_WORD( &tmnt2_1c0800[0x10] ),
	CellDest,CellSrc,
	READ_WORD(src+0),READ_WORD(src+2),READ_WORD(src+4),READ_WORD(src+6),
	CellVar,
	READ_WORD( &sunset_104000[CellVar + 0x00]),
	READ_WORD( &sunset_104000[CellVar + 0x02]),
	READ_WORD( &sunset_104000[CellVar + 0x04]),
	READ_WORD( &sunset_104000[CellVar + 0x06]),
	READ_WORD( &sunset_104000[CellVar + 0x08]),
	READ_WORD( &sunset_104000[CellVar + 0x0a]),
	READ_WORD( &sunset_104000[CellVar + 0x0c]),
	READ_WORD( &sunset_104000[CellVar + 0x0e]),
	READ_WORD( &sunset_104000[CellVar + 0x10]),
	READ_WORD( &sunset_104000[CellVar + 0x12]),
	READ_WORD( &sunset_104000[CellVar + 0x14]),
	READ_WORD( &sunset_104000[CellVar + 0x16]),
	READ_WORD( &sunset_104000[CellVar + 0x18]),
	READ_WORD( &sunset_104000[CellVar + 0x1a]),
	READ_WORD( &sunset_104000[CellVar + 0x1c]),
	READ_WORD( &sunset_104000[CellVar + 0x1e]),
	READ_WORD( &sunset_104000[CellVar + 0x20]),
	READ_WORD( &sunset_104000[CellVar + 0x22]),
	READ_WORD( &sunset_104000[CellVar + 0x24]),
	READ_WORD( &sunset_104000[CellVar + 0x26]),
	READ_WORD( &sunset_104000[CellVar + 0x28]),
	READ_WORD( &sunset_104000[CellVar + 0x2a]),
	READ_WORD( &sunset_104000[CellVar + 0x2c]),
	READ_WORD( &sunset_104000[CellVar + 0x2e])
	);
#endif
//        }
    }
}

static struct MemoryWriteAddress tmnt2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x104000, 0x107fff, MWA_BANK1, &sunset_104000 },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x180000, 0x183fff, K053245_scattered_word_w, &spriteram },
	{ 0x1c0200, 0x1c0201, ssriders_eeprom_w },
	{ 0x1c0300, 0x1c0301, ssriders_1c0300_w },
	{ 0x1c0400, 0x1c0401, watchdog_reset_w },
	{ 0x1c0500, 0x1c057f, MWA_BANK3 },	/* unknown: TMNT2 only (1J) */
	{ 0x1c0800, 0x1c081f, tmnt2_1c0800_w, &tmnt2_1c0800 },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_w },
	{ 0x5c0600, 0x5c0601, K053260_WriteReg },
	{ 0x5c0604, 0x5c0605, ssriders_soundkludge_w },
	{ 0x5c0700, 0x5c071f, K053251_halfword_w },
	{ 0x600000, 0x603fff, K052109_word_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress ssriders_readmem[] =
{
	{ 0x000000, 0x0bffff, MRA_ROM },
	{ 0x104000, 0x107fff, MRA_BANK1 },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram_word_r },
	{ 0x180000, 0x183fff, K053245_scattered_word_r },
	{ 0x1c0000, 0x1c0001, input_port_0_r },
	{ 0x1c0002, 0x1c0003, input_port_1_r },
	{ 0x1c0004, 0x1c0005, input_port_4_r },
	{ 0x1c0006, 0x1c0007, input_port_5_r },
	{ 0x1c0100, 0x1c0101, input_port_2_r },
	{ 0x1c0102, 0x1c0103, ssriders_eeprom_r },
	{ 0x1c0400, 0x1c0401, watchdog_reset_r },
	{ 0x1c0500, 0x1c057f, MRA_BANK3 },	/* TMNT2 only (1J); unknown */
	{ 0x1c0800, 0x1c0801, ssriders_kludge },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_r },
	{ 0x5c0600, 0x5c0603, punkshot_sound_r },	/* K053260 */
	{ 0x600000, 0x603fff, K052109_word_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ssriders_writemem[] =
{
	{ 0x000000, 0x0bffff, MWA_ROM },
	{ 0x104000, 0x107fff, MWA_BANK1 },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x180000, 0x183fff, K053245_scattered_word_w, &spriteram },
	{ 0x1c0200, 0x1c0201, ssriders_eeprom_w },
	{ 0x1c0300, 0x1c0301, ssriders_1c0300_w },
	{ 0x1c0400, 0x1c0401, watchdog_reset_w },
	{ 0x1c0500, 0x1c057f, MWA_BANK3 },	/* TMNT2 only (1J); unknown */
//	{ 0x1c0800, 0x1c081f,  },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_w },
	{ 0x5c0600, 0x5c0601, K053260_WriteReg },
	{ 0x5c0604, 0x5c0605, ssriders_soundkludge_w },
	{ 0x5c0700, 0x5c071f, K053251_halfword_w },
	{ 0x600000, 0x603fff, K052109_word_w },
	{ -1 }	/* end of table */
};

static int xmen_sound_r(int offset)
{
	/* fake self test pass until we emulate the sound chip */
	return 0x000f;
}

static struct MemoryReadAddress xmen_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x100fff, K053247_word_r },
	{ 0x101000, 0x101fff, MRA_BANK2 },
	{ 0x104000, 0x104fff, paletteram_word_r },
	{ 0x108054, 0x108055, xmen_sound_r },
	{ 0x10a000, 0x10a001, input_port_0_r },
	{ 0x10a002, 0x10a003, input_port_1_r },
	{ 0x10a004, 0x10a005, xmen_eeprom_r },
	{ 0x10a00c, 0x10a00d, K053246_word_r },
	{ 0x110000, 0x113fff, MRA_BANK1 },	/* main RAM */
	{ 0x18c000, 0x197fff, K052109_halfword_r },
	{ -1 }	/* end of table */
};

static void xmen_18fa00_w(int offset,int data)
{
	/* bit 2 is interrupt enable */
	interrupt_enable_w(0,data & 0x04);
}

static struct MemoryWriteAddress xmen_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x100fff, K053247_word_w },
	{ 0x101000, 0x101fff, MWA_BANK2 },
	{ 0x104000, 0x104fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x108000, 0x108001, xmen_eeprom_w },
	{ 0x108020, 0x108027, K053246_word_w },
	{ 0x108060, 0x10807f, K053251_halfword_w },
	{ 0x10a000, 0x10a001, watchdog_reset_w },
	{ 0x110000, 0x113fff, MWA_BANK1 },	/* main RAM */
	{ 0x18fa00, 0x18fa01, xmen_18fa00_w },
	{ 0x18c000, 0x197fff, K052109_halfword_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress mia_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mia_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress tmnt_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_r },	/* title music & UPD7759C reset */
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ 0xf000, 0xf000, UPD7759_busy_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress tmnt_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_w },	/* title music & UPD7759C reset */
	{ 0xb000, 0xb00d, K007232_write_port_0_w  },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd000, UPD7759_message_w },
	{ 0xe000, 0xe000, UPD7759_start_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress punkshot_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress punkshot_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, sound_arm_nmi },
	{ 0xfc00, 0xfc2f, K053260_WriteReg },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress lgtnfght_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa001, 0xa001, YM2151_status_port_0_r },
	{ 0xc000, 0xc02f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress lgtnfght_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM2151_register_port_0_w },
	{ 0xa001, 0xa001, YM2151_data_port_0_w },
	{ 0xc000, 0xc02f, K053260_WriteReg },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress glfgreat_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf82f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress glfgreat_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf82f, K053260_WriteReg },
	{ 0xfa00, 0xfa00, sound_arm_nmi },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ssriders_s_readmem[] =
{
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfa00, 0xfa2f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ssriders_s_writemem[] =
{
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa2f, K053260_WriteReg },
	{ 0xfc00, 0xfc00, sound_arm_nmi },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( mia_input_ports )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30000 80000" )
	PORT_DIPSETTING(    0x10, "50000 100000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Character Test" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tmnt_input_ports )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )	/* actually service 1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )	/* actually service 2 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )	/* actually service 3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )	/* actually service 4 */

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* PLAYER 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* PLAYER 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tmnt2p_input_ports )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* PLAYER 3 */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* PLAYER 4 */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( punkshot_input_ports )
	PORT_START	/* DSW1/DSW2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
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
	PORT_DIPNAME( 0x0010, 0x0010, "Continue" )
	PORT_DIPSETTING(      0x0010, "Normal" )
	PORT_DIPSETTING(      0x0000, "1 Coin" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Energy" )
	PORT_DIPSETTING(      0x0300, "30" )
	PORT_DIPSETTING(      0x0200, "40" )
	PORT_DIPSETTING(      0x0100, "50" )
	PORT_DIPSETTING(      0x0000, "60" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Period Length" )
	PORT_DIPSETTING(      0x0c00, "2 Minutes" )
	PORT_DIPSETTING(      0x0800, "3 Minutes" )
	PORT_DIPSETTING(      0x0400, "4 Minutes" )
	PORT_DIPSETTING(      0x0000, "5 Minutes" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Medium" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* COIN/DSW3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )	/* actually service 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )	/* actually service 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START3 )	/* actually service 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )	/* actually service 4 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN0/IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2/IN3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( punksht2_input_ports )
	PORT_START	/* DSW1/DSW2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
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
	PORT_DIPNAME( 0x0010, 0x0010, "Continue" )
	PORT_DIPSETTING(      0x0010, "Normal" )
	PORT_DIPSETTING(      0x0000, "1 Coin" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Energy" )
	PORT_DIPSETTING(      0x0300, "40" )
	PORT_DIPSETTING(      0x0200, "50" )
	PORT_DIPSETTING(      0x0100, "60" )
	PORT_DIPSETTING(      0x0000, "70" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Period Length" )
	PORT_DIPSETTING(      0x0c00, "3 Minutes" )
	PORT_DIPSETTING(      0x0800, "4 Minutes" )
	PORT_DIPSETTING(      0x0400, "5 Minutes" )
	PORT_DIPSETTING(      0x0000, "6 Minutes" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Medium" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* COIN/DSW3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN0/IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( lgtnfght_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* vblank? checked during boot */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "100000 400000" )
	PORT_DIPSETTING(    0x10, "150000 500000" )
	PORT_DIPSETTING(    0x08, "200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
//	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Sound" )
	PORT_DIPSETTING(    0x02, "Mono" )
	PORT_DIPSETTING(    0x00, "Stereo" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( detatwin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VBLANK? OBJMPX? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status? - always 1 */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( glfgreat_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

	PORT_START
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
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 1C_7C ) )
//	PORT_DIPSETTING(      0x0000, "Invalid" )
	PORT_DIPNAME( 0x0300, 0x0100, "Players/Controllers" )
	PORT_DIPSETTING(      0x0300, "4/1" )
	PORT_DIPSETTING(      0x0200, "4/2" )
	PORT_DIPSETTING(      0x0100, "4/4" )
	PORT_DIPSETTING(      0x0000, "3/3" )
	PORT_DIPNAME( 0x0400, 0x0000, "Sound" )
	PORT_DIPSETTING(      0x0400, "Mono" )
	PORT_DIPSETTING(      0x0000, "Stereo" )
	PORT_DIPNAME( 0x1800, 0x1800, "Initial/Maximum Credit" )
	PORT_DIPSETTING(      0x1800, "2/3" )
	PORT_DIPSETTING(      0x1000, "2/4" )
	PORT_DIPSETTING(      0x0800, "2/5" )
	PORT_DIPSETTING(      0x0000, "3/5" )
	PORT_DIPNAME( 0x6000, 0x4000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE )	/* service coin */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x8000, 0x0000, "Freeze" )	/* ?? VBLANK ?? */
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
//	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ssriders4p_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )	/* actually service 1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )	/* actually service 2 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )	/* actually service 3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )	/* actually service 4 */

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: NVBLK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ssriders_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: NVBLK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( xmen_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* COIN  EEPROM and service */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status - always 1 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x3000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x4000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
INPUT_PORTS_END



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ 0 }
};

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	{ 4 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct UPD7759_interface upd7759_interface =
{
	1,		/* number of chips */
	UPD7759_STANDARD_CLOCK,
	{ 60 }, /* volume */
	{ 5 },		/* memory region */
	UPD7759_STANDALONE_MODE,		/* chip mode */
	{0}
};

static struct Samplesinterface samples_interface =
{
	1,	/* 1 channel for the title music */
	25	/* volume */
};

static struct K053260_interface k053260_interface_nmi =
{
	3579545,
	4, /* memory region */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },
//	sound_nmi_callback,
};

static struct K053260_interface k053260_interface =
{
	3579545,
	4, /* memory region */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },
	0
};

static struct K053260_interface glfgreat_k053260_interface =
{
	3579545,
	4, /* memory region */
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) },
//	sound_nmi_callback,
};


static struct MachineDriver mia_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz */
			0,
			mia_readmem,mia_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			mia_s_readmem,mia_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	mia_vh_start,
	punkshot_vh_stop,
	mia_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K007232,
			&k007232_interface,
		}
	}
};

static struct MachineDriver tmnt_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz */
			0,
			tmnt_readmem,tmnt_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			tmnt_s_readmem,tmnt_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tmnt_vh_start,
	punkshot_vh_stop,
	tmnt_vh_screenrefresh,

	/* sound hardware */
	0,
	tmnt_decode_sample,
	0,
	0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K007232,
			&k007232_interface,
		},
		{
			SOUND_UPD7759,
			&upd7759_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static struct MachineDriver punkshot_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* CPU is 68000/12, but this doesn't necessarily mean it's */
						/* running at 12MHz. TMNT uses 8MHz */
			0,
			punkshot_readmem,punkshot_writemem,0,0,
			punkshot_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			punkshot_s_readmem,punkshot_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	punkshot_vh_start,
	punkshot_vh_stop,
	punkshot_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface_nmi
		}
	}
};

static struct MachineDriver lgtnfght_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			0,
			lgtnfght_readmem,lgtnfght_writemem,0,0,
			lgtnfght_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			lgtnfght_s_readmem,lgtnfght_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	lgtnfght_vh_start,
	lgtnfght_vh_stop,
	lgtnfght_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface
		}
	}
};

static struct MachineDriver detatwin_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz */
			0,
			detatwin_readmem,detatwin_writemem,0,0,
			punkshot_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ????? */
			3,
			ssriders_s_readmem,ssriders_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ssriders_eeprom_init,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	detatwin_vh_start,
	detatwin_vh_stop,
	lgtnfght_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface_nmi
		}
	}
};

static struct MachineDriver glfgreat_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* ? */
			0,
			glfgreat_readmem,glfgreat_writemem,0,0,
			lgtnfght_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? */
			3,
			glfgreat_s_readmem,glfgreat_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	glfgreat_vh_start,
	glfgreat_vh_stop,
	glfgreat_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_K053260,
			&glfgreat_k053260_interface
		}
	}
};

static struct MachineDriver tmnt2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz */
			0,
			tmnt2_readmem,tmnt2_writemem,0,0,
			punkshot_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2*3579545,	/* makes the ROM test sync */
			3,
			ssriders_s_readmem,ssriders_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ssriders_eeprom_init,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	lgtnfght_vh_start,
	lgtnfght_vh_stop,
	lgtnfght_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface_nmi
		}
	}
};

static struct MachineDriver ssriders_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz */
			0,
			ssriders_readmem,ssriders_writemem,0,0,
			punkshot_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ????? makes the ROM test sync */
			3,
			ssriders_s_readmem,ssriders_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ssriders_eeprom_init,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	lgtnfght_vh_start,
	lgtnfght_vh_stop,
	ssriders_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface_nmi
		}
	}
};

static int xmen_interrupt(void)
{
	if (cpu_getiloops() == 0) return m68_level5_irq();
	else return m68_level3_irq();
}

static struct MachineDriver xmen_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* ? */
			0,
			xmen_readmem,xmen_writemem,0,0,
			xmen_interrupt,2
		},
#if 0
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2*3579545,	/* ????? */
			3,
			ssriders_s_readmem,ssriders_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ssriders_eeprom_init,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	0,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	xmen_vh_start,
	xmen_vh_stop,
	xmen_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};


/***************************************************************************

  High score save/load

***************************************************************************/

static int tmnt_hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */


if ((READ_WORD(&(cpu_bankbase[1][0x03500])) == 0x0312) &&
		(READ_WORD(&(cpu_bankbase[1][0x03502])) == 0x0257) &&
		(READ_WORD(&(cpu_bankbase[1][0x03512])) == 0x0101) &&
		(READ_WORD(&(cpu_bankbase[1][0x035C8])) == 0x4849) &&
		(READ_WORD(&(cpu_bankbase[1][0x035CA])) == 0x4400) &&
		(READ_WORD(&(cpu_bankbase[1][0x035EC])) == 0x4D49))

{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x03500]),0x14);
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x035C8]),0x27);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void tmnt_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x03500]),0x14);
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x035C8]),0x27);


		osd_fclose(f);
	}
}

static int punkshot_hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */


if ((READ_WORD(&(cpu_bankbase[1][0x00708])) == 0x0007) &&
		(READ_WORD(&(cpu_bankbase[1][0x0070A])) == 0x2020) &&
		(READ_WORD(&(cpu_bankbase[1][0x0072C])) == 0x464C))

{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x00708]),0x28);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void punkshot_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x00708]),0x28);
		osd_fclose(f);
	}
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mia_rom )
	ROM_REGION(0x40000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "808t20.h17",   0x00000, 0x20000, 0x6f0acb1d )
	ROM_LOAD_ODD ( "808t21.j17",   0x00000, 0x20000, 0x42a30416 )

	ROM_REGION(0x40000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD_GFX_EVEN( "808e12.f28",   0x000000, 0x10000, 0xd62f1fde )        /* 8x8 tiles */
	ROM_LOAD_GFX_ODD ( "808e13.h28",   0x000000, 0x10000, 0x1fa708f4 )        /* 8x8 tiles */
	ROM_LOAD_GFX_EVEN( "808e22.i28",   0x020000, 0x10000, 0x73d758f6 )        /* 8x8 tiles */
	ROM_LOAD_GFX_ODD ( "808e23.k28",   0x020000, 0x10000, 0x8ff08b21 )        /* 8x8 tiles */

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "808d17.j4",    0x00000, 0x80000, 0xd1299082 )	/* sprites */
	ROM_LOAD( "808d15.h4",    0x80000, 0x80000, 0x2b22a6b6 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "808e03.f4",    0x00000, 0x08000, 0x3d93a7cd )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "808d01.d4",    0x00000, 0x20000, 0xfd4d37c0 ) /* samples for 007232 */

	ROM_REGION(0x0100)	/* PROMs */
	ROM_LOAD( "808a18.f16",   0x0000, 0x0100, 0xeb95aede )	/* priority encoder (not used) */
ROM_END

ROM_START( mia2_rom )
	ROM_REGION(0x40000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "808s20.h17",   0x00000, 0x20000, 0xcaa2897f )
	ROM_LOAD_ODD ( "808s21.j17",   0x00000, 0x20000, 0x3d892ffb )

	ROM_REGION(0x40000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD_GFX_EVEN( "808e12.f28",   0x000000, 0x10000, 0xd62f1fde )        /* 8x8 tiles */
	ROM_LOAD_GFX_ODD ( "808e13.h28",   0x000000, 0x10000, 0x1fa708f4 )        /* 8x8 tiles */
	ROM_LOAD_GFX_EVEN( "808e22.i28",   0x020000, 0x10000, 0x73d758f6 )        /* 8x8 tiles */
	ROM_LOAD_GFX_ODD ( "808e23.k28",   0x020000, 0x10000, 0x8ff08b21 )        /* 8x8 tiles */

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "808d17.j4",    0x00000, 0x80000, 0xd1299082 )	/* sprites */
	ROM_LOAD( "808d15.h4",    0x80000, 0x80000, 0x2b22a6b6 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "808e03.f4",    0x00000, 0x08000, 0x3d93a7cd )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "808d01.d4",    0x00000, 0x20000, 0xfd4d37c0 ) /* samples for 007232 */

	ROM_REGION(0x0100)	/* PROMs */
	ROM_LOAD( "808a18.f16",   0x0000, 0x0100, 0xeb95aede )	/* priority encoder (not used) */
ROM_END

ROM_START( tmnt_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-r23",      0x00000, 0x20000, 0xa7f61195 )
	ROM_LOAD_ODD ( "963-r24",      0x00000, 0x20000, 0x661e056a )
	ROM_LOAD_EVEN( "963-r21",      0x40000, 0x10000, 0xde047bb6 )
	ROM_LOAD_ODD ( "963-r22",      0x40000, 0x10000, 0xd86a0888 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x0200)	/* PROMs */
	ROM_LOAD( "tmnt.g7",      0x0000, 0x0100, 0xabd82680 )	/* sprite address decoder */
	ROM_LOAD( "tmnt.g19",     0x0100, 0x0100, 0xf8004a1c )	/* priority encoder (not used) */
ROM_END

ROM_START( tmht_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963f23.j17",   0x00000, 0x20000, 0x9cb5e461 )
	ROM_LOAD_ODD ( "963f24.k17",   0x00000, 0x20000, 0x2d902fab )
	ROM_LOAD_EVEN( "963f21.j15",   0x40000, 0x10000, 0x9fa25378 )
	ROM_LOAD_ODD ( "963f22.k15",   0x40000, 0x10000, 0x2127ee53 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x0200)	/* PROMs */
	ROM_LOAD( "tmnt.g7",      0x0000, 0x0100, 0xabd82680 )	/* sprite address decoder */
	ROM_LOAD( "tmnt.g19",     0x0100, 0x0100, 0xf8004a1c )	/* priority encoder (not used) */
ROM_END

ROM_START( tmntj_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-x23",      0x00000, 0x20000, 0xa9549004 )
	ROM_LOAD_ODD ( "963-x24",      0x00000, 0x20000, 0xe5cc9067 )
	ROM_LOAD_EVEN( "963-x21",      0x40000, 0x10000, 0x5789cf92 )
	ROM_LOAD_ODD ( "963-x22",      0x40000, 0x10000, 0x0a74e277 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x0200)	/* PROMs */
	ROM_LOAD( "tmnt.g7",      0x0000, 0x0100, 0xabd82680 )	/* sprite address decoder */
	ROM_LOAD( "tmnt.g19",     0x0100, 0x0100, 0xf8004a1c )	/* priority encoder (not used) */
ROM_END

ROM_START( tmht2p_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-u23",      0x00000, 0x20000, 0x58bec748 )
	ROM_LOAD_ODD ( "963-u24",      0x00000, 0x20000, 0xdce87c8d )
	ROM_LOAD_EVEN( "963-u21",      0x40000, 0x10000, 0xabce5ead )
	ROM_LOAD_ODD ( "963-u22",      0x40000, 0x10000, 0x4ecc8d6b )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x0200)	/* PROMs */
	ROM_LOAD( "tmnt.g7",      0x0000, 0x0100, 0xabd82680 )	/* sprite address decoder */
	ROM_LOAD( "tmnt.g19",     0x0100, 0x0100, 0xf8004a1c )	/* priority encoder (not used) */
ROM_END

ROM_START( tmnt2pj_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-123",      0x00000, 0x20000, 0x6a3527c9 )
	ROM_LOAD_ODD ( "963-124",      0x00000, 0x20000, 0x2c4bfa15 )
	ROM_LOAD_EVEN( "963-121",      0x40000, 0x10000, 0x4181b733 )
	ROM_LOAD_ODD ( "963-122",      0x40000, 0x10000, 0xc64eb5ff )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */

	ROM_REGION(0x20000)	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x0200)	/* PROMs */
	ROM_LOAD( "tmnt.g7",      0x0000, 0x0100, 0xabd82680 )	/* sprite address decoder */
	ROM_LOAD( "tmnt.g19",     0x0100, 0x0100, 0xf8004a1c )	/* priority encoder (not used) */
ROM_END

ROM_START( punkshot_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "907-j02.i7",   0x00000, 0x20000, 0xdbb3a23b )
	ROM_LOAD_ODD ( "907-j03.i10",  0x00000, 0x20000, 0x2151d1ab )

	ROM_REGION(0x80000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, 0xf5cc38f4 )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, 0xe25774c1 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d07l.k2",   0x000000, 0x80000, 0xfeeb345a )
	ROM_LOAD( "907d07h.k2",   0x080000, 0x80000, 0x0bff4383 )
	ROM_LOAD( "907d08l.k7",   0x100000, 0x80000, 0x05f3d196 )
	ROM_LOAD( "907d08h.k7",   0x180000, 0x80000, 0xeaf18c22 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, 0xf040c484 )

	ROM_REGION(0x80000)	/* samples for 053260 */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, 0x090feb5e )
ROM_END

ROM_START( punksht2_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "907m02.i7",    0x00000, 0x20000, 0x59e14575 )
	ROM_LOAD_ODD ( "907m03.i10",   0x00000, 0x20000, 0xadb14b1e )

	ROM_REGION(0x80000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, 0xf5cc38f4 )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, 0xe25774c1 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d07l.k2",   0x000000, 0x80000, 0xfeeb345a )
	ROM_LOAD( "907d07h.k2",   0x080000, 0x80000, 0x0bff4383 )
	ROM_LOAD( "907d08l.k7",   0x100000, 0x80000, 0x05f3d196 )
	ROM_LOAD( "907d08h.k7",   0x180000, 0x80000, 0xeaf18c22 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, 0xf040c484 )

	ROM_REGION(0x80000)	/* samples for the 053260 */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, 0x090feb5e )
ROM_END

ROM_START( lgtnfght_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "939m02.e11",   0x00000, 0x20000, 0x61a12184 )
	ROM_LOAD_ODD ( "939m03.e15",   0x00000, 0x20000, 0x6db6659d )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a07.k14",   0x000000, 0x80000, 0x7955dfcf )
	ROM_LOAD( "939a08.k19",   0x080000, 0x80000, 0xed95b385 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a06.k8",    0x000000, 0x80000, 0xe393c206 )
	ROM_LOAD( "939a05.k2",    0x080000, 0x80000, 0x3662d47a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "939e01.d7",    0x0000, 0x8000, 0x4a5fc848 )

	ROM_REGION(0x80000)	/* samples for the 053260 */
	ROM_LOAD( "939a04.c5",    0x0000, 0x80000, 0xc24e2b6e )
ROM_END

ROM_START( trigon_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "939j02.bin",   0x00000, 0x20000, 0x38381d1b )
	ROM_LOAD_ODD ( "939j03.bin",   0x00000, 0x20000, 0xb5beddcd )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a07.k14",   0x000000, 0x80000, 0x7955dfcf )
	ROM_LOAD( "939a08.k19",   0x080000, 0x80000, 0xed95b385 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a06.k8",    0x000000, 0x80000, 0xe393c206 )
	ROM_LOAD( "939a05.k2",    0x080000, 0x80000, 0x3662d47a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "939e01.d7",    0x0000, 0x8000, 0x4a5fc848 )

	ROM_REGION(0x80000)	/* samples for the 053260 */
	ROM_LOAD( "939a04.c5",    0x0000, 0x80000, 0xc24e2b6e )
ROM_END

ROM_START( detatwin_rom )
	ROM_REGION(0x80000)
	ROM_LOAD_EVEN( "060_j02.rom", 0x000000, 0x20000, 0x11b761ac )
	ROM_LOAD_ODD ( "060_j03.rom", 0x000000, 0x20000, 0x8d0b588c )
	ROM_LOAD_EVEN( "060_j09.rom", 0x040000, 0x20000, 0xf2a5f15f )
	ROM_LOAD_ODD ( "060_j10.rom", 0x040000, 0x20000, 0x36eefdbc )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD_GFX_SWAP( "060_e07.r16",  0x000000, 0x080000, 0xc400edf3 )	/* tiles */
	ROM_LOAD_GFX_SWAP( "060_e08.r16",  0x080000, 0x080000, 0x70dddba1 )

	ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD_GFX_SWAP( "060_e06.r16",  0x000000, 0x080000, 0x09381492 )	/* sprites */
	ROM_LOAD_GFX_SWAP( "060_e05.r16",  0x080000, 0x080000, 0x32454241 )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "060_j01.rom",  0x0000, 0x10000, 0xf9d9a673 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "060_e04.r16",  0x0000, 0x100000, 0xc680395d )
ROM_END

ROM_START( glfgreat_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "061l02.1h",   0x000000, 0x20000, 0xac7399f4 )
	ROM_LOAD_ODD ( "061l03.4h",   0x000000, 0x20000, 0x77b0ff5c )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d14.12l",   0x000000, 0x080000, 0xb9440924 )	/* tiles */
	ROM_LOAD( "061d13.12k",   0x080000, 0x080000, 0x9f999f0b )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d11.3k",    0x000000, 0x100000, 0xc45b66a3 )	/* sprites */
	ROM_LOAD( "061d12.8k",    0x100000, 0x100000, 0xd305ecd1 )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "061f01.4e",    0x0000, 0x8000, 0xab9a2a57 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "061e04.1d",    0x0000, 0x100000, 0x7921d8df )

	ROM_REGION(0x300000)	/* unknown (data for the 053936?) */
	ROM_LOAD( "061b05.15d",   0x000000, 0x020000, 0x2456fb11 )	/* gfx */
	ROM_LOAD( "061b06.16d",   0x080000, 0x080000, 0x41ada2ad )
	ROM_LOAD( "061b07.18d",   0x100000, 0x080000, 0x517887e2 )
	ROM_LOAD( "061b08.14g",   0x180000, 0x080000, 0x6ab739c3 )
	ROM_LOAD( "061b09.15g",   0x200000, 0x080000, 0x42c7a603 )
	ROM_LOAD( "061b10.17g",   0x280000, 0x080000, 0x10f89ce7 )
ROM_END

ROM_START( tmnt2_rom )
	ROM_REGION(0x80000)
	ROM_LOAD_EVEN( "uaa02", 0x000000, 0x20000, 0x58d5c93d )
	ROM_LOAD_ODD ( "uaa03", 0x000000, 0x20000, 0x0541fec9 )
	ROM_LOAD_EVEN( "uaa04", 0x040000, 0x20000, 0x1d441a7d )
	ROM_LOAD_ODD ( "uaa05", 0x040000, 0x20000, 0x9c428273 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, 0xd3283d19 )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, 0x6ebc0c15 )

	ROM_REGION(0x400000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, 0x2d7a9d2a )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, 0xf2dd296e )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, 0xd9bee7bf )
	ROM_LOAD( "b08",          0x300000, 0x080000, 0x3b1ae36f )
	/* second half empty */

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, 0x364f548a )

	ROM_REGION(0x200000)	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, 0x1e510aa5 )
ROM_END

ROM_START( tmnt22p_rom )
	ROM_REGION(0x80000)
	ROM_LOAD_EVEN( "a02",   0x000000, 0x20000, 0xaadffe3a )
	ROM_LOAD_ODD ( "a03",   0x000000, 0x20000, 0x125687a8 )
	ROM_LOAD_EVEN( "a04",   0x040000, 0x20000, 0xfb5c7ded )
	ROM_LOAD_ODD ( "a05",   0x040000, 0x20000, 0x3c40fe66 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, 0xd3283d19 )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, 0x6ebc0c15 )

	ROM_REGION(0x400000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, 0x2d7a9d2a )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, 0xf2dd296e )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, 0xd9bee7bf )
	ROM_LOAD( "b08",          0x300000, 0x080000, 0x3b1ae36f )
	/* second half empty */

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, 0x364f548a )

	ROM_REGION(0x200000)	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, 0x1e510aa5 )
ROM_END

ROM_START( tmnt2a_rom )
	ROM_REGION(0x80000)
	ROM_LOAD_EVEN( "ada02", 0x000000, 0x20000, 0x4f11b587 )
	ROM_LOAD_ODD ( "ada03", 0x000000, 0x20000, 0x82a1b9ac )
	ROM_LOAD_EVEN( "ada04", 0x040000, 0x20000, 0x05ad187a )
	ROM_LOAD_ODD ( "ada05", 0x040000, 0x20000, 0xd4826547 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, 0xd3283d19 )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, 0x6ebc0c15 )

	ROM_REGION(0x400000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, 0x2d7a9d2a )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, 0xf2dd296e )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, 0xd9bee7bf )
	ROM_LOAD( "b08",          0x300000, 0x080000, 0x3b1ae36f )
	/* second half empty */

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, 0x364f548a )

	ROM_REGION(0x200000)	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, 0x1e510aa5 )
ROM_END

ROM_START( ssriders_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064eac02",    0x000000, 0x40000, 0x5a5425f4 )
	ROM_LOAD_ODD ( "064eac03",    0x000000, 0x40000, 0x093c00fb )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdrebd_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064ebd02",    0x000000, 0x40000, 0x8deef9ac )
	ROM_LOAD_ODD ( "064ebd03",    0x000000, 0x40000, 0x2370c107 )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdrebc_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "sr_c02.rom",  0x000000, 0x40000, 0x9bd7d164 )
	ROM_LOAD_ODD ( "sr_c03.rom",  0x000000, 0x40000, 0x40fd4165 )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdruda_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064uda02",    0x000000, 0x40000, 0x5129a6b7 )
	ROM_LOAD_ODD ( "064uda03",    0x000000, 0x40000, 0x9f887214 )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdruac_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064uac02",    0x000000, 0x40000, 0x870473b6 )
	ROM_LOAD_ODD ( "064uac03",    0x000000, 0x40000, 0xeadf289a )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdrubc_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "2pl.8e",      0x000000, 0x40000, 0xaca7fda5 )
	ROM_LOAD_ODD ( "2pl.8g",      0x000000, 0x40000, 0xbb1fdeff )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdrabd_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064abd02.8e", 0x000000, 0x40000, 0x713406cb )
	ROM_LOAD_ODD ( "064abd03.8g", 0x000000, 0x40000, 0x680feb3c )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( ssrdrjbd_rom )
	ROM_REGION(0xc0000)
	ROM_LOAD_EVEN( "064jbd02.8e", 0x000000, 0x40000, 0x7acdc1e3 )
	ROM_LOAD_ODD ( "064jbd03.8g", 0x000000, 0x40000, 0x6a424918 )
	ROM_LOAD_EVEN( "sr_b04.rom",  0x080000, 0x20000, 0xef2315bd )
	ROM_LOAD_ODD ( "sr_b05.rom",  0x080000, 0x20000, 0x51d6fbc4 )

    ROM_REGION(0x100000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, 0xe2bdc619 )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, 0x2d8ca8b0 )

	ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, 0x4160c372 )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, 0x64dd673c )

	ROM_REGION(0x010000) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, 0x44b9bc52 )

	ROM_REGION(0x100000)	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, 0x59810df9 )
ROM_END

ROM_START( xmen_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_EVEN( "065ubb04.10d",  0x00000, 0x20000, 0xf896c93b )
    ROM_LOAD_ODD ( "065ubb05.10f",  0x00000, 0x20000, 0xe02e5d64 )
    ROM_LOAD_EVEN( "xmen17g.bin",   0x80000, 0x40000, 0xb31dc44c )
    ROM_LOAD_ODD ( "xmen17j.bin",   0x80000, 0x40000, 0x13842fe6 )

    ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
    ROM_LOAD( "xmen1l.bin",   0x000000, 0x100000, 0x6b649aca )	/* tiles */
    ROM_LOAD( "xmen1h.bin",   0x100000, 0x100000, 0xc5dc8fc4 )

	ROM_REGION(0x400000)	/* graphics (addressable by the main CPU) */
    ROM_LOAD( "xmen12l.bin",  0x000000, 0x100000, 0xea05d52f )	/* sprites */
    ROM_LOAD( "xmen17l.bin",  0x100000, 0x100000, 0x96b91802 )
    ROM_LOAD( "xmen22h.bin",  0x200000, 0x100000, 0x321ed07a )
    ROM_LOAD( "xmen22l.bin",  0x300000, 0x100000, 0x46da948e )

    ROM_REGION(0x20000)
    ROM_LOAD( "065-a01.6f",   0x00000, 0x20000, 0x147d3a4d )

    ROM_REGION(0x200000)	/* samples for the 054544 */
    ROM_LOAD( "xmenc25.bin",  0x000000, 0x200000, 0x5adbcee0 )
ROM_END

ROM_START( xmen6p_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_EVEN( "xmenb04.bin",   0x00000, 0x20000, 0x0f09b8e0 )
    ROM_LOAD_ODD ( "xmenb05.bin",   0x00000, 0x20000, 0x867becbf )
    ROM_LOAD_EVEN( "xmen17g.bin",   0x80000, 0x40000, 0xb31dc44c )
    ROM_LOAD_ODD ( "xmen17j.bin",   0x80000, 0x40000, 0x13842fe6 )

    ROM_REGION(0x200000)	/* graphics (addressable by the main CPU) */
    ROM_LOAD( "xmen1l.bin",   0x000000, 0x100000, 0x6b649aca )	/* tiles */
    ROM_LOAD( "xmen1h.bin",   0x100000, 0x100000, 0xc5dc8fc4 )

	ROM_REGION(0x400000)	/* graphics (addressable by the main CPU) */
    ROM_LOAD( "xmen12l.bin",  0x000000, 0x100000, 0xea05d52f )	/* sprites */
    ROM_LOAD( "xmen17l.bin",  0x100000, 0x100000, 0x96b91802 )
    ROM_LOAD( "xmen22h.bin",  0x200000, 0x100000, 0x321ed07a )
    ROM_LOAD( "xmen22l.bin",  0x300000, 0x100000, 0x46da948e )

    ROM_REGION(0x20000)
    ROM_LOAD( "065-a01.6f",   0x00000, 0x20000, 0x147d3a4d )

    ROM_REGION(0x200000)	/* samples for the 054544 */
    ROM_LOAD( "xmenc25.bin",  0x000000, 0x200000, 0x5adbcee0 )
ROM_END



static void gfx_untangle(void)
{
	konami_rom_deinterleave_2(1);
	konami_rom_deinterleave_2(2);
}

static void mia_gfx_untangle(void)
{
	unsigned char *gfxdata;
	int len;
	int i,j,k,A,B;
	int bits[32];
	unsigned char *temp;


	gfx_untangle();

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051962 custom IC.
	*/
	gfxdata = Machine->memory_region[1];
	len = Machine->memory_region_length[1];
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	/*
		along with the normal byte reordering, MIA also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051937 custom IC.
	*/
	gfxdata = Machine->memory_region[2];
	len = Machine->memory_region_length[2];
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	temp = malloc(len);
	if (!temp) return;	/* bad thing! */
	memcpy(temp,gfxdata,len);
	for (A = 0;A < len/4;A++)
	{
		/* the bits to scramble are the low 8 ones */
		for (i = 0;i < 8;i++)
			bits[i] = (A >> i) & 0x01;

		B = A & 0x3ff00;

		if ((A & 0x3c000) == 0x3c000)
		{
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[0] << 2;
			B |= bits[1] << 3;
			B |= bits[2] << 4;
			B |= bits[4] << 5;
			B |= bits[6] << 6;
			B |= bits[7] << 7;
		}
		else
		{
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[7] << 2;
			B |= bits[0] << 3;
			B |= bits[1] << 4;
			B |= bits[2] << 5;
			B |= bits[4] << 6;
			B |= bits[6] << 7;
		}

		gfxdata[4*A+0] = temp[4*B+0];
		gfxdata[4*A+1] = temp[4*B+1];
		gfxdata[4*A+2] = temp[4*B+2];
		gfxdata[4*A+3] = temp[4*B+3];
	}
	free(temp);
}


static void tmnt_gfx_untangle(void)
{
	unsigned char *gfxdata;
	int len;
	int i,j,k,A,B,entry;
	int bits[32];
	unsigned char *temp;


	gfx_untangle();

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051962 custom IC.
	*/
	gfxdata = Machine->memory_region[1];
	len = Machine->memory_region_length[1];
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051937 custom IC.
	*/
	gfxdata = Machine->memory_region[2];
	len = Machine->memory_region_length[2];
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	temp = malloc(len);
	if (!temp) return;	/* bad thing! */
	memcpy(temp,gfxdata,len);
	for (A = 0;A < len/4;A++)
	{
		unsigned char *code_conv_table = &Machine->memory_region[7][0x0000];
#define CA0 0
#define CA1 1
#define CA2 2
#define CA3 3
#define CA4 4
#define CA5 5
#define CA6 6
#define CA7 7
#define CA8 8
#define CA9 9

		/* following table derived from the schematics. It indicates, for each of the */
		/* 9 low bits of the sprite line address, which bit to pick it from. */
		/* For example, when the PROM contains 4, which applies to 4x2 sprites, */
		/* bit OA1 comes from CA5, OA2 from CA0, and so on. */
		static unsigned char bit_pick_table[10][8] =
		{
			/*0(1x1) 1(2x1) 2(1x2) 3(2x2) 4(4x2) 5(2x4) 6(4x4) 7(8x8) */
			{ CA3,   CA3,   CA3,   CA3,   CA3,   CA3,   CA3,   CA3 },	/* CA3 */
			{ CA0,   CA0,   CA5,   CA5,   CA5,   CA5,   CA5,   CA5 },	/* OA1 */
			{ CA1,   CA1,   CA0,   CA0,   CA0,   CA7,   CA7,   CA7 },	/* OA2 */
			{ CA2,   CA2,   CA1,   CA1,   CA1,   CA0,   CA0,   CA9 },	/* OA3 */
			{ CA4,   CA4,   CA2,   CA2,   CA2,   CA1,   CA1,   CA0 },	/* OA4 */
			{ CA5,   CA6,   CA4,   CA4,   CA4,   CA2,   CA2,   CA1 },	/* OA5 */
			{ CA6,   CA5,   CA6,   CA6,   CA6,   CA4,   CA4,   CA2 },	/* OA6 */
			{ CA7,   CA7,   CA7,   CA7,   CA8,   CA6,   CA6,   CA4 },	/* OA7 */
			{ CA8,   CA8,   CA8,   CA8,   CA7,   CA8,   CA8,   CA6 },	/* OA8 */
			{ CA9,   CA9,   CA9,   CA9,   CA9,   CA9,   CA9,   CA8 }	/* OA9 */
		};

		/* pick the correct entry in the PROM (top 8 bits of the address) */
		entry = code_conv_table[(A & 0x7f800) >> 11] & 7;

		/* the bits to scramble are the low 10 ones */
		for (i = 0;i < 10;i++)
			bits[i] = (A >> i) & 0x01;

		B = A & 0x7fc00;

		for (i = 0;i < 10;i++)
			B |= bits[bit_pick_table[i][entry]] << i;

		gfxdata[4*A+0] = temp[4*B+0];
		gfxdata[4*A+1] = temp[4*B+1];
		gfxdata[4*A+2] = temp[4*B+2];
		gfxdata[4*A+3] = temp[4*B+3];
	}
	free(temp);
}

static void shuffle(UINT8 *buf,int len)
{
	int i;
	UINT8 t;

	if (len == 2) return;

	if (len % 4) exit(1);	/* must not happen */

	len /= 2;

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	shuffle(buf,len);
	shuffle(buf + len,len);
}

static void glfgreat_gfx_untangle(void)
{
	/* ROMs are interleaved at byte level */
	shuffle(Machine->memory_region[1],Machine->memory_region_length[1]);
	shuffle(Machine->memory_region[2],Machine->memory_region_length[2]);
}

static void xmen_gfx_untangle(void)
{
	konami_rom_deinterleave_2(1);
	konami_rom_deinterleave_4(2);
}



static int nvram_load(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		EEPROM_load(f);
		osd_fclose(f);
	}
	else
		init_eeprom_count = 10;

	return 1;
}

static void nvram_save(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		EEPROM_save(f);
		osd_fclose(f);
	}
}



struct GameDriver mia_driver =
{
	__FILE__,
	0,
	"mia",
	"Missing in Action (version T)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&mia_machine_driver,
	0,

	mia_rom,
	mia_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	mia_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver mia2_driver =
{
	__FILE__,
	&mia_driver,
	"mia2",
	"Missing in Action (version S)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&mia_machine_driver,
	0,

	mia2_rom,
	mia_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	mia_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver tmnt_driver =
{
	__FILE__,
	0,
	"tmnt",
	"Teenage Mutant Ninja Turtles (4 Players US)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmnt_rom,
	tmnt_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	tmnt_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmht_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmht",
	"Teenage Mutant Hero Turtles (4 Players UK)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmht_rom,
	tmnt_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	tmnt_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmntj_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmntj",
	"Teenage Mutant Ninja Turtles (4 Players Japan)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmntj_rom,
	tmnt_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	tmnt_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmht2p_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmht2p",
	"Teenage Mutant Hero Turtles (2 Players UK)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmht2p_rom,
	tmnt_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	tmnt2p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmnt2pj_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmnt2pj",
	"Teenage Mutant Ninja Turtles (2 Players Japan)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmnt2pj_rom,
	tmnt_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	tmnt2p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver punkshot_driver =
{
	__FILE__,
	0,
	"punkshot",
	"Punk Shot (4 Players)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&punkshot_machine_driver,
	0,

	punkshot_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	punkshot_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	punkshot_hiload, punkshot_hisave
};

struct GameDriver punksht2_driver =
{
	__FILE__,
	&punkshot_driver,
	"punksht2",
	"Punk Shot (2 Players)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&punkshot_machine_driver,
	0,

	punksht2_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	punksht2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	punkshot_hiload, punkshot_hisave
};

struct GameDriver lgtnfght_driver =
{
	__FILE__,
	0,
	"lgtnfght",
	"Lightning Fighters (US)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&lgtnfght_machine_driver,
	0,

	lgtnfght_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	lgtnfght_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver trigon_driver =
{
	__FILE__,
	&lgtnfght_driver,
	"trigon",
	"Trigon (Japan)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&lgtnfght_machine_driver,
	0,

	trigon_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	lgtnfght_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver detatwin_driver =
{
	__FILE__,
	0,
	"detatwin",
	"Detana!! Twin Bee (Japan)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&detatwin_machine_driver,
	0,

	detatwin_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	detatwin_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	nvram_load, nvram_save
};

struct GameDriver glfgreat_driver =
{
	__FILE__,
	0,
	"glfgreat",
	"Golfing Greats",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_NOT_WORKING,
	&glfgreat_machine_driver,
	0,

	glfgreat_rom,
	glfgreat_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	glfgreat_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver tmnt2_driver =
{
	__FILE__,
	0,
	"tmnt2",
	"Teenage Mutant Ninja Turtles - Turtles in Time (4 Players US)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_IMPERFECT_COLORS,
	&tmnt2_machine_driver,
	0,

	tmnt2_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders4p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver tmnt22p_driver =
{
	__FILE__,
	&tmnt2_driver,
	"tmnt22p",
	"Teenage Mutant Ninja Turtles - Turtles in Time (2 Players US)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_IMPERFECT_COLORS,
	&tmnt2_machine_driver,
	0,

	tmnt22p_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver tmnt2a_driver =
{
	__FILE__,
	&tmnt2_driver,
	"tmnt2a",
	"Teenage Mutant Ninja Turtles - Turtles in Time (4 Players Asia)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_IMPERFECT_COLORS,
	&tmnt2_machine_driver,
	0,

	tmnt2a_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders4p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssriders_driver =
{
	__FILE__,
	0,
	"ssriders",
	"Sunset Riders (World 4 Players ver. EAC)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssriders_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders4p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdrebd_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdrebd",
	"Sunset Riders (World 2 Players ver. EBD)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdrebd_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdrebc_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdrebc",
	"Sunset Riders (World 2 Players ver. EBC)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdrebc_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdruda_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdruda",
	"Sunset Riders (US 4 Players ver. UDA)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdruda_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdruac_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdruac",
	"Sunset Riders (US 4 Players ver. UAC)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdruac_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdrubc_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdrubc",
	"Sunset Riders (US 2 Players ver. UBC)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdrubc_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdrabd_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdrabd",
	"Sunset Riders (Asia 2 Players ver. ABD)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdrabd_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver ssrdrjbd_driver =
{
	__FILE__,
	&ssriders_driver,
	"ssrdrjbd",
	"Sunset Riders (Japan 2 Players ver. JBD)",
	"1991",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	0,
	&ssriders_machine_driver,
	0,

	ssrdrjbd_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	ssriders_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

struct GameDriver xmen_driver =
{
	__FILE__,
	0,
	"xmen",
	"X-Men (4 Players)",
	"1992",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_NO_SOUND,
	&xmen_machine_driver,
	0,

	xmen_rom,
	xmen_gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	xmen_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};

static void xmen6p_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	WRITE_WORD(&RAM[0x21a6],0x4e71);
	WRITE_WORD(&RAM[0x21a8],0x4e71);
	WRITE_WORD(&RAM[0x21aa],0x4e71);

	xmen_gfx_untangle();
}

struct GameDriver xmen6p_driver =
{
	__FILE__,
	&xmen_driver,
	"xmen6p",
	"X-Men (6 Players)",
	"1992",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)",
	GAME_NOT_WORKING,
	&xmen_machine_driver,
	0,

	xmen6p_rom,
	xmen6p_patch, 0,
	0,
	0,	/* sound_prom */

	xmen_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};
