/***************************************************************************

Sega G-80 Raster drivers

The G-80 Raster games are Astro Blaster, Monster Bash, 005, and Space Odyssey.

See also sega.c for the Sega G-80 Vector games.

Many thanks go to Dave Fish for the fine detective work he did into the
G-80 security chips (315-0064, 315-0070, 315-0076, 315-0082) which provided
me with enough information to emulate those chips at runtime along with
the 315-0062 Astro Blaster chip and the 315-0063 Space Odyssey chip.

Thanks also go to Paul Tonizzo, Clay Cowgill, John Bowes, and Kevin Klopp
for all the helpful information, samples, and schematics!

- Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* sndhrdw/segar.c */

extern void astrob_speech_port_w(int offset, int data);
extern void astrob_audio_port1_w(int offset, int data);
extern void astrob_audio_port2_w(int offset, int data);
extern void spaceod_audio_port1_w(int offset, int data);
extern void spaceod_audio_port2_w(int offset, int data);
extern void monsterb_audio_port1_w(int offset, int data);
extern void monsterb_audio_port2_w(int offset, int data);
extern void monsterb_audio_port3_w(int offset, int data);
extern void monsterb_audio_port4_w(int offset, int data);
extern int monsterb_audio_port3_r(int offset);

extern int segar_sh_start(void);
extern void segar_sh_update(void);


extern void monsterb_sound_w(int offset, int data);

extern int monsterb_sh_start(void);
extern void monsterb_sh_update(void);
extern void monsterb_sh_stop(void);



extern const char *astrob_sample_names[];
extern const char *s005_sample_names[];
extern const char *monsterb_sample_names[];
extern const char *spaceod_sample_names[];

/* machine/segar.c */

extern void sega_security(int chip);
extern void segar_wr(int offset, int data);

extern unsigned char *segar_mem;

/* vidhrdw/segar.c */

extern unsigned char *segar_characterram;
extern unsigned char *segar_characterram2;
extern unsigned char *segar_mem_colortable;
extern unsigned char *segar_mem_bcolortable;
extern unsigned char *segar_mem_blookup;

extern void segar_init_colors(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern int spaceod_vh_start(void);
extern void spaceod_vh_stop(void);
extern void segar_video_port_w(int offset,int data);
extern void monsterb_back_port_w(int offset,int data);
extern void spaceod_back_port_w(int offset,int data);
extern void spaceod_backshift_w(int offset,int data);
extern void spaceod_backshift_clear_w(int offset,int data);
extern void spaceod_backfill_w(int offset,int data);
extern void spaceod_nobackfill_w(int offset,int data);
extern void segar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void spaceod_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* drivers/segar.c */

static int segar_interrupt(void);
static int segar_read_ports(int offset);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0xC7FF, MRA_ROM },
	{ 0xC800, 0xCFFF, MRA_RAM },	/* Misc RAM */
	{ 0xE000, 0xE3FF, MRA_RAM, &videoram, &videoram_size },
	{ 0xE400, 0xE7FF, MRA_RAM },  /* Used by at least Monster Bash? */
	{ 0xE800, 0xEFFF, MRA_RAM, &segar_characterram },
	{ 0xF000, 0xF03F, MRA_RAM, &segar_mem_colortable },	/* Dynamic color table */
	{ 0xF040, 0xF07F, MRA_RAM, &segar_mem_bcolortable }, 	/* Dynamic color table for background (Monster Bash)*/
        { 0xF080, 0xF7FF, MRA_RAM },
        { 0xF800, 0xFFFF, MRA_RAM, &segar_characterram2 },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress monsterb_readmem[] =
{
    { 0x0000, 0xC7FF, MRA_ROM },
	{ 0xC800, 0xCFFF, MRA_RAM },	/* Misc RAM */
	{ 0xE000, 0xE3FF, MRA_RAM, &videoram, &videoram_size },
	{ 0xE400, 0xE7FF, MRA_RAM },  /* Used by at least Monster Bash? */
	{ 0xE800, 0xEFFF, MRA_RAM, &segar_characterram },
	{ 0xF000, 0xF03F, MRA_RAM, &segar_mem_colortable },	/* Dynamic color table */
	{ 0xF040, 0xF07F, MRA_RAM, &segar_mem_bcolortable }, 	/* Dynamic color table for background (Monster Bash)*/
        { 0xF080, 0xF7FF, MRA_RAM },
        { 0xF800, 0xFFFF, MRA_RAM, &segar_characterram2 },
    {0x10000,0x11FFF, MRA_ROM, &segar_mem_blookup }, /* Background pattern ROM */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress spaceod_readmem[] =
{
    { 0x0000, 0xC7FF, MRA_ROM },
	{ 0xC800, 0xCFFF, MRA_RAM },	/* Misc RAM */
	{ 0xE000, 0xE3FF, MRA_RAM, &videoram, &videoram_size },
	{ 0xE400, 0xE7FF, MRA_RAM },  /* Used by at least Monster Bash? */
	{ 0xE800, 0xEFFF, MRA_RAM, &segar_characterram },
	{ 0xF000, 0xF03F, MRA_RAM, &segar_mem_colortable },	/* Dynamic color table */
	{ 0xF040, 0xF07F, MRA_RAM, &segar_mem_bcolortable }, 	/* Dynamic color table for background (Monster Bash)*/
        { 0xF080, 0xF7FF, MRA_RAM },
        { 0xF800, 0xFFFF, MRA_RAM, &segar_characterram2 },
    {0x10000,0x13FFF, MRA_ROM, &segar_mem_blookup }, /* Background pattern ROMs */
	{ -1 }	/* end of table */
};



static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xFFFF, segar_wr, &segar_mem },
	{ -1 }
};

static struct IOReadPort readport[] =
{
        { 0x0e, 0x0e, monsterb_audio_port3_r },
        { 0xf8, 0xfc, segar_read_ports },
	{ -1 }	/* end of table */
};

static struct IOWritePort astrob_writeport[] =
{

        { 0x38, 0x38, astrob_speech_port_w },
        { 0x3E, 0x3E, astrob_audio_port1_w },
        { 0x3F, 0x3F, astrob_audio_port2_w },

        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */

	{ -1 }	/* end of table */
};

static struct IOWritePort spaceod_writeport[] =
{
        { 0x08, 0x08, spaceod_back_port_w },
        { 0x09, 0x09, spaceod_backshift_clear_w },
        { 0x0A, 0x0A, spaceod_backshift_w },
        { 0x0B, 0x0C, spaceod_nobackfill_w }, /* I'm not sure what these ports really do */
        { 0x0D, 0x0D, spaceod_backfill_w },
        { 0x0E, 0x0E, spaceod_audio_port1_w },
        { 0x0F, 0x0F, spaceod_audio_port2_w },

	{ 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */

	{ -1 }	/* end of table */
};

static struct IOWritePort s005_writeport[] =
{

	{ 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */

	{ -1 }	/* end of table */
};

static struct IOWritePort monsterb_writeport[] =
{
        { 0x0c, 0x0c, monsterb_sound_w },
        { 0x0d, 0x0d, monsterb_audio_port2_w },
        { 0x0e, 0x0e, monsterb_audio_port3_w },
        { 0x0f, 0x0f, monsterb_audio_port4_w },
        { 0xbc, 0xbc, monsterb_back_port_w },
	{ 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */

	{ -1 }	/* end of table */
};




/***************************************************************************

  The Sega games use NMI to trigger the self test. We use a fake input port to
  tie that event to a keypress.

***************************************************************************/
static int segar_interrupt(void)
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */
	else return interrupt();
}

/***************************************************************************

  The Sega games store the DIP switches in a very mangled format that's
  not directly useable by MAME.  This function mangles the DIP switches
  into a format that can be used.

  Original format:
  Port 0 - 2-4, 2-8, 1-4, 1-8
  Port 1 - 2-3, 2-7, 1-3, 1-7
  Port 2 - 2-2, 2-6, 1-2, 1-6
  Port 3 - 2-1, 2-5, 1-1, 1-5
  MAME format:
  Port 6 - 1-1, 1-2, 1-3, 1-4, 1-5, 1-6, 1-7, 1-8
  Port 7 - 2-1, 2-2, 2-3, 2-4, 2-5, 2-6, 2-7, 2-8
***************************************************************************/
static int segar_read_ports(int offset)
{
        int dip1, dip2;

        dip1 = input_port_6_r(offset);
        dip2 = input_port_7_r(offset);

        switch(offset)
        {
                case 0:
                   return ((input_port_0_r(0) & 0xF0) | ((dip2 & 0x08)>>3) | ((dip2 & 0x80)>>6) |
                                                        ((dip1 & 0x08)>>1) | ((dip1 & 0x80)>>4));
                case 1:
                   return ((input_port_1_r(0) & 0xF0) | ((dip2 & 0x04)>>2) | ((dip2 & 0x40)>>5) |
                                                        ((dip1 & 0x04)>>0) | ((dip1 & 0x40)>>3));
                case 2:
                   return ((input_port_2_r(0) & 0xF0) | ((dip2 & 0x02)>>1) | ((dip2 & 0x20)>>4) |
                                                        ((dip1 & 0x02)<<1) | ((dip1 & 0x20)>>2));
                case 3:
                   return ((input_port_3_r(0) & 0xF0) | ((dip2 & 0x01)>>0) | ((dip2 & 0x10)>>3) |
                                                        ((dip1 & 0x01)<<2) | ((dip1 & 0x10)>>1));
                case 4:
                   return input_port_4_r(0);
        }

        return 0;
}
/***************************************************************************
Input Ports
***************************************************************************/

INPUT_PORTS_START( astrob_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0xC0, 0xC0, "Number of Ships", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "5 Ships" )
	PORT_DIPSETTING(    0x40, "4 Ships" )
	PORT_DIPSETTING(    0x80, "3 Ships" )
	PORT_DIPSETTING(    0x00, "2 Ships" )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 2 */
	PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x08, "3 / 1" )
	PORT_DIPSETTING(    0x04, "2 / 1" )
	PORT_DIPSETTING(    0x0C, "1 / 1" )
	PORT_DIPSETTING(    0x02, "1 / 2" )
	PORT_DIPSETTING(    0x0A, "1 / 3" )
	PORT_DIPSETTING(    0x06, "1 / 4" )
	PORT_DIPSETTING(    0x0E, "1 / 5" )
	PORT_DIPSETTING(    0x01, "1 / 6" )
	PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
	PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

	PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x80, "3 / 1" )
	PORT_DIPSETTING(    0x40, "2 / 1" )
	PORT_DIPSETTING(    0xC0, "1 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0xA0, "1 / 3" )
	PORT_DIPSETTING(    0x60, "1 / 4" )
	PORT_DIPSETTING(    0xE0, "1 / 5" )
	PORT_DIPSETTING(    0x10, "1 / 6" )
	PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
	PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( s005_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "6 Men" )
	PORT_DIPSETTING(    0x80, "5 Men" )
	PORT_DIPSETTING(    0x40, "4 Men" )
	PORT_DIPSETTING(    0x00, "3 Men" )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 2 */
	PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x08, "3 / 1" )
	PORT_DIPSETTING(    0x04, "2 / 1" )
	PORT_DIPSETTING(    0x0C, "1 / 1" )
	PORT_DIPSETTING(    0x02, "1 / 2" )
	PORT_DIPSETTING(    0x0A, "1 / 3" )
	PORT_DIPSETTING(    0x06, "1 / 4" )
	PORT_DIPSETTING(    0x0E, "1 / 5" )
	PORT_DIPSETTING(    0x01, "1 / 6" )
	PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
	PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

	PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x80, "3 / 1" )
	PORT_DIPSETTING(    0x40, "2 / 1" )
	PORT_DIPSETTING(    0xC0, "1 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0xA0, "1 / 3" )
	PORT_DIPSETTING(    0x60, "1 / 4" )
	PORT_DIPSETTING(    0xE0, "1 / 5" )
	PORT_DIPSETTING(    0x10, "1 / 6" )
	PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
	PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( monsterb_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL)
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x01, 0x01, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Endless" )
	PORT_DIPNAME( 0x06, 0x02, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x06, "40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x18, 0x08, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x18, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xC0, "6" )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 2 */
	PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x08, "3 / 1" )
	PORT_DIPSETTING(    0x04, "2 / 1" )
	PORT_DIPSETTING(    0x0C, "1 / 1" )
	PORT_DIPSETTING(    0x02, "1 / 2" )
	PORT_DIPSETTING(    0x0A, "1 / 3" )
	PORT_DIPSETTING(    0x06, "1 / 4" )
	PORT_DIPSETTING(    0x0E, "1 / 5" )
	PORT_DIPSETTING(    0x01, "1 / 6" )
	PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
	PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

	PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x80, "3 / 1" )
	PORT_DIPSETTING(    0x40, "2 / 1" )
	PORT_DIPSETTING(    0xC0, "1 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0xA0, "1 / 3" )
	PORT_DIPSETTING(    0x60, "1 / 4" )
	PORT_DIPSETTING(    0xE0, "1 / 5" )
	PORT_DIPSETTING(    0x10, "1 / 6" )
	PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
	PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( spaceod_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
	IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x01, 0x01, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Endless" )
	PORT_DIPNAME( 0x18, 0x08, "Extra Ship @", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "80,000 Points" )
	PORT_DIPSETTING(    0x10, "60,000 Points" )
	PORT_DIPSETTING(    0x08, "40,000 Points" )
	PORT_DIPSETTING(    0x00, "20,000 Points" )
	PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "6 Ships" )
	PORT_DIPSETTING(    0x80, "5 Ships" )
	PORT_DIPSETTING(    0x40, "4 Ships" )
	PORT_DIPSETTING(    0x00, "3 Ships" )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 2 */
	PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x08, "3 / 1" )
	PORT_DIPSETTING(    0x04, "2 / 1" )
	PORT_DIPSETTING(    0x0C, "1 / 1" )
	PORT_DIPSETTING(    0x02, "1 / 2" )
	PORT_DIPSETTING(    0x0A, "1 / 3" )
	PORT_DIPSETTING(    0x06, "1 / 4" )
	PORT_DIPSETTING(    0x0E, "1 / 5" )
	PORT_DIPSETTING(    0x01, "1 / 6" )
	PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
	PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

	PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 / 1" )
	PORT_DIPSETTING(    0x80, "3 / 1" )
	PORT_DIPSETTING(    0x40, "2 / 1" )
	PORT_DIPSETTING(    0xC0, "1 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0xA0, "1 / 3" )
	PORT_DIPSETTING(    0x60, "1 / 4" )
	PORT_DIPSETTING(    0xE0, "1 / 5" )
	PORT_DIPSETTING(    0x10, "1 / 6" )
	PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
	PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
	PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
	PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
	PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
	PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
	PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters? */
	2,	/* 2 bits per pixel */
	{ 0, 0x1000*8 },	/* separated by 0x1000 bytes */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout backlayout =
{
	8,8,	/* 8*8 characters */
        256*4,  /* 256 characters per scene, 4 scenes */
	2,	/* 2 bits per pixel */
        { 0, 0x2000*8 },        /* separated by 0x2000 bytes */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spacelayout =
{
        8,8,   /* 16*8 characters */
        256*2,    /* 256 characters */
        6,      /* 6 bits per pixel */
        { 0, 0x1000*8, 0x2000*8, 0x3000*8, 0x4000*8, 0x5000*8 },        /* separated by 0x1000 bytes (1 EPROM per bit) */
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
        8*8     /* every char takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout, 0x01, 0x10 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo monsterb_gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout, 0x01, 0x10 }, /* offset into colors, # of colors */
        { 1, 0x0000, &backlayout, 0x41, 0x10 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo spaceod_gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout,   0x01, 0x10 }, /* offset into colors, # of colors */
        { 1, 0x0000, &spacelayout,  0x41, 1 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};



/***************************************************************************
  Game ROMs
***************************************************************************/


ROM_START( astrob_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "829b", 0x0000, 0x0800, 0xfaf1807b ) /* U25 */
	ROM_LOAD( "888", 0x0800, 0x0800, 0x76d69726 ) /* U1 */
	ROM_LOAD( "889", 0x1000, 0x0800, 0x0c99ac2d ) /* U2 */
	ROM_LOAD( "890", 0x1800, 0x0800, 0x80a5dbc5 ) /* U3 */
	ROM_LOAD( "891", 0x2000, 0x0800, 0x8de56827 ) /* U4 */
	ROM_LOAD( "892", 0x2800, 0x0800, 0xfb499b4f ) /* U5 */
	ROM_LOAD( "893", 0x3000, 0x0800, 0x5e481b08 ) /* U6 */
	ROM_LOAD( "894", 0x3800, 0x0800, 0x515b704f ) /* U7 */
	ROM_LOAD( "895", 0x4000, 0x0800, 0x2943b91f ) /* U8 */
	ROM_LOAD( "896", 0x4800, 0x0800, 0xc1fee47a ) /* U9 */
	ROM_LOAD( "897", 0x5000, 0x0800, 0xa3a1fbf3 ) /* U10 */
	ROM_LOAD( "898", 0x5800, 0x0800, 0xa88f0d35 ) /* U11 */
	ROM_LOAD( "899", 0x6000, 0x0800, 0xbc63f743 ) /* U12 */
	ROM_LOAD( "900", 0x6800, 0x0800, 0xa4a29056 ) /* U13 */
	ROM_LOAD( "901", 0x7000, 0x0800, 0x0960a790 ) /* U14 */
	ROM_LOAD( "902", 0x7800, 0x0800, 0xaafa19a2 ) /* U15 */
	ROM_LOAD( "903", 0x8000, 0x0800, 0xec35c73b ) /* U16 */
	ROM_LOAD( "904", 0x8800, 0x0800, 0x5c40a316 ) /* U16 */
	ROM_LOAD( "905", 0x9000, 0x0800, 0x0f21a0f1 ) /* U16 */
	ROM_LOAD( "906", 0x9800, 0x0800, 0xd1a84b58 ) /* U16 */

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)	/* 64k for speech code */
	ROM_LOAD( "808b", 0x0000, 0x0800, 0x62a09920 ) /* U7 */
	ROM_LOAD( "809a", 0x0800, 0x0800, 0x88f4cd68 ) /* U6 */
	ROM_LOAD( "810",  0x1000, 0x0800, 0xefc511d1 ) /* U5 */
	ROM_LOAD( "811",  0x1800, 0x0800, 0x17838277 ) /* U4 */
	ROM_LOAD( "812a", 0x2000, 0x0800, 0x8e50ad22 ) /* U3 */
ROM_END

ROM_START( astrob1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "829", 0x0000, 0x0800, 0xa7c2d826 ) /* U25 */
	ROM_LOAD( "837", 0x0800, 0x0800, 0xd0d3b713 ) /* U1 */
	ROM_LOAD( "838", 0x1000, 0x0800, 0xe471b901 ) /* U2 */
	ROM_LOAD( "839", 0x1800, 0x0800, 0xe13d7981 ) /* U3 */
	ROM_LOAD( "840", 0x2000, 0x0800, 0xfac9b2e7 ) /* U4 */
	ROM_LOAD( "841", 0x2800, 0x0800, 0x9b3ce336 ) /* U5 */
	ROM_LOAD( "842", 0x3000, 0x0800, 0xdca661a8 ) /* U6 */
	ROM_LOAD( "843", 0x3800, 0x0800, 0xf2a2461c ) /* U7 */
	ROM_LOAD( "844", 0x4000, 0x0800, 0x710cf11c ) /* U8 */
	ROM_LOAD( "845", 0x4800, 0x0800, 0x0744f834 ) /* U9 */
	ROM_LOAD( "846", 0x5000, 0x0800, 0xe7b9be2d ) /* U10 */
	ROM_LOAD( "847", 0x5800, 0x0800, 0x5ca4c3a2 ) /* U11 */
	ROM_LOAD( "848", 0x6000, 0x0800, 0xd379e407 ) /* U12 */
	ROM_LOAD( "849", 0x6800, 0x0800, 0x61230dc1 ) /* U13 */
	ROM_LOAD( "850", 0x7000, 0x0800, 0x1e1039e8 ) /* U14 */
	ROM_LOAD( "851", 0x7800, 0x0800, 0xf1403e8e ) /* U15 */
	ROM_LOAD( "852", 0x8000, 0x0800, 0x2769427b ) /* U16 */

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)	/* 64k for speech code */
	ROM_LOAD( "808b", 0x0000, 0x0800, 0x62a09920 ) /* U7 */
	ROM_LOAD( "809a", 0x0800, 0x0800, 0x88f4cd68 ) /* U6 */
	ROM_LOAD( "810",  0x1000, 0x0800, 0xefc511d1 ) /* U5 */
	ROM_LOAD( "811",  0x1800, 0x0800, 0x17838277 ) /* U4 */
	ROM_LOAD( "812a", 0x2000, 0x0800, 0x8e50ad22 ) /* U3 */
ROM_END

ROM_START( s005_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1346b.u25",0x0000, 0x0800, 0xd2388c1a ) /* U25 */
	ROM_LOAD( "5092.u1",  0x0800, 0x0800, 0x57630baf ) /* U1 */
	ROM_LOAD( "5093.u2",  0x1000, 0x0800, 0x716cdf04 ) /* U2 */
	ROM_LOAD( "5094.u3",  0x1800, 0x0800, 0x58791775 ) /* U3 */
	ROM_LOAD( "5095.u4",  0x2000, 0x0800, 0x46c98d7b ) /* U4 */
	ROM_LOAD( "5096.u5",  0x2800, 0x0800, 0x68a0bce8 ) /* U5 */
	ROM_LOAD( "5097.u6",  0x3000, 0x0800, 0x715c6f46 ) /* U6 */
	ROM_LOAD( "5098.u7",  0x3800, 0x0800, 0xd85b9a91 ) /* U7 */
	ROM_LOAD( "5099.u8",  0x4000, 0x0800, 0xa0171fb5 ) /* U8 */
	ROM_LOAD( "5100.u9",  0x4800, 0x0800, 0x37ff4483 ) /* U9 */
	ROM_LOAD( "5101.u10", 0x5000, 0x0800, 0x05f7b335 ) /* U10 */
	ROM_LOAD( "5102.u11", 0x5800, 0x0800, 0x1015d135 ) /* U11 */
	ROM_LOAD( "5103.u12", 0x6000, 0x0800, 0x20b78e49 ) /* U12 */
	ROM_LOAD( "5104.u13", 0x6800, 0x0800, 0xddf3520d ) /* U13 */
	ROM_LOAD( "5105.u14", 0x7000, 0x0800, 0x283fd429 ) /* U14 */
	ROM_LOAD( "5106.u15", 0x7800, 0x0800, 0xe6f187a5 ) /* U15 */
	ROM_LOAD( "5107.u16", 0x8000, 0x0800, 0xd08e94ea ) /* U16 */
	ROM_LOAD( "5108.u17", 0x8800, 0x0800, 0x3bc012b8 ) /* U17 */
	ROM_LOAD( "5109.u18", 0x9000, 0x0800, 0xb647bdb1 ) /* U18 */
	ROM_LOAD( "5110.u19", 0x9800, 0x0800, 0x228b1921 ) /* U19 */
	ROM_LOAD( "5111.u20", 0xA000, 0x0800, 0x2d069e9a ) /* U20 */

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x800)	/* 2k for sound */
	ROM_LOAD( "epr-1286.16",0x0000, 0x0800, 0x7b132c1b )
ROM_END

ROM_START( monsterb_rom )
	ROM_REGION(0x12000)	/* 64k for code + 2k for background */
	ROM_LOAD( "1778cpu.bin", 0x0000, 0x0800, 0x8ec4448a ) /* U25 */
	ROM_LOAD( "1779.bin", 0x0800, 0x0800, 0xd50c5ffe ) /* U1 */
	ROM_LOAD( "1780.bin", 0x1000, 0x0800, 0x7b559937 ) /* U2 */
	ROM_LOAD( "1781.bin", 0x1800, 0x0800, 0xe06a9d1a ) /* U3 */
	ROM_LOAD( "1782.bin", 0x2000, 0x0800, 0x0c964894 ) /* U4 */
	ROM_LOAD( "1783.bin", 0x2800, 0x0800, 0x4714b3c8 ) /* U5 */
	ROM_LOAD( "1784.bin", 0x3000, 0x0800, 0xbf1e2448 ) /* U6 */
	ROM_LOAD( "1785.bin", 0x3800, 0x0800, 0x22a48b72 ) /* U7 */
	ROM_LOAD( "1786.bin", 0x4000, 0x0800, 0x51258567 ) /* U8 */
	ROM_LOAD( "1787.bin", 0x4800, 0x0800, 0x7973af07 ) /* U9 */
	ROM_LOAD( "1788.bin", 0x5000, 0x0800, 0x55afea07 ) /* U10 */
	ROM_LOAD( "1789.bin", 0x5800, 0x0800, 0xfa33f265 ) /* U11 */
	ROM_LOAD( "1790.bin", 0x6000, 0x0800, 0xe072ff5e ) /* U12 */
	ROM_LOAD( "1791.bin", 0x6800, 0x0800, 0x90406e64 ) /* U13 */
	ROM_LOAD( "1792.bin", 0x7000, 0x0800, 0x0b8b9061 ) /* U14 */
	ROM_LOAD( "1793.bin", 0x7800, 0x0800, 0x9df16e25 ) /* U15 */
	ROM_LOAD( "1794.bin", 0x8000, 0x0800, 0xe97695ec ) /* U16 */
	ROM_LOAD( "1795.bin", 0x8800, 0x0800, 0x414a37b2 ) /* U17 */
	ROM_LOAD( "1796.bin", 0x9000, 0x0800, 0x685c5d0a ) /* U18 */
	ROM_LOAD( "1797.bin", 0x9800, 0x0800, 0xbd86b83e ) /* U19 */
	ROM_LOAD( "1798.bin", 0xA000, 0x0800, 0x99e2cdb2 ) /* U20 */
	ROM_LOAD( "1799.bin", 0xA800, 0x0800, 0x864cef58 ) /* U21 */
	ROM_LOAD( "1800.bin", 0xB000, 0x0800, 0xf21e93be ) /* U22 */
	ROM_LOAD( "1801.bin", 0xB800, 0x0800, 0x1410ea9c ) /* U23 */

	ROM_LOAD( "1518a.bin",0x10000,0x2000, 0xbbe8139e ) /* U25 */

	ROM_REGION(0x4000)	/* 16k for background graphics */
	ROM_LOAD( "1516.bin", 0x0000, 0x2000, 0xee08373e ) /* U25 */
	ROM_LOAD( "1517.bin", 0x2000, 0x2000, 0xa1a6c650 ) /* U25 */

	ROM_REGION(0x2000)      /* 8k for sound */
	ROM_LOAD( "1543snd.bin", 0x0000, 0x1000, 0x8503b7d3 ) /* U25 */
	ROM_LOAD( "1544snd.bin", 0x1000, 0x1000, 0x3f427e0e ) /* U25 */
ROM_END

ROM_START( spaceod_rom )
	ROM_REGION(0x14000)     /* 64k for code + 4k for background lookup */
	ROM_LOAD( "so-959.dat", 0x0000, 0x0800, 0x3a1abb14 ) /* U25 */
	ROM_LOAD( "so-941.dat", 0x0800, 0x0800, 0x8eb435d6 ) /* U1 */
	ROM_LOAD( "so-942.dat", 0x1000, 0x0800, 0xe3a8b98c ) /* U2 */
	ROM_LOAD( "so-943.dat", 0x1800, 0x0800, 0x2fe83022 ) /* U3 */
	ROM_LOAD( "so-944.dat", 0x2000, 0x0800, 0xa037fb9b ) /* U4 */
	ROM_LOAD( "so-945.dat", 0x2800, 0x0800, 0xdbbb5573 ) /* U5 */
	ROM_LOAD( "so-946.dat", 0x3000, 0x0800, 0xcfad4f97 ) /* U6 */
	ROM_LOAD( "so-947.dat", 0x3800, 0x0800, 0xe22ff995 ) /* U7 */
	ROM_LOAD( "so-948.dat", 0x4000, 0x0800, 0x9815ff77 ) /* U8 */
	ROM_LOAD( "so-949.dat", 0x4800, 0x0800, 0xda3b95e9 ) /* U9 */
	ROM_LOAD( "so-950.dat", 0x5000, 0x0800, 0x18112967 ) /* U10 */
	ROM_LOAD( "so-951.dat", 0x5800, 0x0800, 0xcd0f2c33 ) /* U11 */
	ROM_LOAD( "so-952.dat", 0x6000, 0x0800, 0xdb34988c ) /* U12 */
	ROM_LOAD( "so-953.dat", 0x6800, 0x0800, 0x4b7bbbb3 ) /* U13 */
	ROM_LOAD( "so-954.dat", 0x7000, 0x0800, 0xefb32701 ) /* U14 */
	ROM_LOAD( "so-955.dat", 0x7800, 0x0800, 0x7f5426a8 ) /* U15 */
	ROM_LOAD( "so-956.dat", 0x8000, 0x0800, 0x409090fa ) /* U16 */
	ROM_LOAD( "so-957.dat", 0x8800, 0x0800, 0x13c86eec ) /* U17 */
	ROM_LOAD( "so-958.dat", 0x9000, 0x0800, 0xffd5a7b9 ) /* U18 */

	ROM_LOAD( "epr-09.dat", 0x10000, 0x1000, 0x422dc709 )
	ROM_LOAD( "epr-10.dat", 0x11000, 0x1000, 0xceee55fe )
	ROM_LOAD( "epr-11.dat", 0x12000, 0x1000, 0x615dab29 )
	ROM_LOAD( "epr-12.dat", 0x13000, 0x1000, 0xbf615473 )


	ROM_REGION(0x6000)      /* for backgrounds */
	ROM_LOAD( "epr-13.dat", 0x0000, 0x1000, 0xefcae6a6 )
	ROM_LOAD( "epr-14.dat", 0x1000, 0x1000, 0x839535c3 )
	ROM_LOAD( "epr-15.dat", 0x2000, 0x1000, 0xf372e574 )
	ROM_LOAD( "epr-16.dat", 0x3000, 0x1000, 0xe6565672 )
	ROM_LOAD( "epr-17.dat", 0x4000, 0x1000, 0xf0a1f50d )
	ROM_LOAD( "epr-18.dat", 0x5000, 0x1000, 0x126e945e )
ROM_END

/***************************************************************************
  Security Decode "chips"
***************************************************************************/

void astrob_decode(void)
{
    /* This game uses the 315-0062 security chip */
    sega_security(62);
}

void s005_decode(void)
{
    /* This game uses the 315-0070 security chip */
    sega_security(70);
}

void monsterb_decode(void)
{
    /* This game uses the 315-0082 security chip */
    sega_security(82);
}

void spaceod_decode(void)
{
    /* This game uses the 315-0063 security chip */
    sega_security(63);
}

/***************************************************************************
  Hi Score Routines
***************************************************************************/

static int astrob_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xCC0E],"KUV",3) == 0) &&
		(memcmp(&RAM[0xCC16],"PS\\",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f,&RAM[0xCB3F],0xDA);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void astrob_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xCB3F],"\xFF\xFF\xFF\xFF",4)==0)
                return;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xCB3F],0xDA);
		osd_fclose(f);
	}

}

static int monsterb_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8E8],"\x22\x0e\xd5\x0a",4) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xC913],7);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void monsterb_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC913],"\xFF\xFF\xFF\xFF",4)==0)
                return;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xC913],7);
		osd_fclose(f);
	}

}

static int s005_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8ED],"\x10\x1B\x17",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xC911],8);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void s005_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC911],"\xFF\xFF\xFF\xFF",4)==0)
                return;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xC911],8);
		osd_fclose(f);
	}

}

static int spaceod_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8F1],"\xE2\x00\x04\x03",4) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xC906],4);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void spaceod_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC906],"\xFF\xFF\xFF\xFF",4)==0)
                return;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xC906],4);
		osd_fclose(f);
	}

}

/***************************************************************************
  Game drivers
***************************************************************************/

static struct MachineDriver astrob_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz ??? */
			0,
			readmem,writemem,readport,astrob_writeport,
			segar_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
        16*4+1,16*4+1,
	segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
        generic_vh_start,
        generic_vh_stop,
	segar_vh_screenrefresh,

	/* sound hardware */
	0,
        segar_sh_start,
	0,
        segar_sh_update
};

static struct MachineDriver spaceod_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz ??? */
			0,
			spaceod_readmem,writemem,readport,spaceod_writeport,
			segar_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        spaceod_gfxdecodeinfo,
        16*4*2+1,16*4*2+1,
	segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
        spaceod_vh_start,
        spaceod_vh_stop,
        spaceod_vh_screenrefresh,

	/* sound hardware */
	0,
        segar_sh_start,
	0,
        segar_sh_update
};

static struct MachineDriver s005_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz ??? */
			0,
			readmem,writemem,readport,s005_writeport,
			segar_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
        16*4+1,16*4+1,
	segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
        generic_vh_start,
        generic_vh_stop,
	segar_vh_screenrefresh,

	/* sound hardware */
	0,
        segar_sh_start,
	0,
        segar_sh_update
};


static struct MachineDriver monsterb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz ??? */
			0,
			monsterb_readmem,writemem,readport,monsterb_writeport,
			segar_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
	monsterb_gfxdecodeinfo,
	16*4*2+1,16*4*2+1,
	segar_init_colors,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	segar_vh_screenrefresh,

	/* sound hardware */
	0,
	monsterb_sh_start,
	monsterb_sh_stop,
	monsterb_sh_update
};



struct GameDriver astrob_driver =
{
	__FILE__,
	0,
	"astrob",
	"Astro Blaster (version 2)",
	"1981",
	"Sega",
	"Dave Fish (security consultant)\nMike Balfour (game driver)",
	0,
	&astrob_machine_driver,

	astrob_rom,
	astrob_decode, 0,
	astrob_sample_names,
	0,	/* sound_prom */

	astrob_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	astrob_hiload, astrob_hisave
};

struct GameDriver astrob1_driver =
{
	__FILE__,
	&astrob_driver,
	"astrob1",
	"Astro Blaster (version 1)",
	"1981",
	"Sega",
	"Dave Fish (security consultant)\nMike Balfour (game driver)",
	0,
	&astrob_machine_driver,

	astrob1_rom,
	astrob_decode, 0,
	astrob_sample_names,
	0,	/* sound_prom */

	astrob_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	astrob_hiload, astrob_hisave
};

struct GameDriver s005_driver =
{
	__FILE__,
	0,
	"005",
	"005",
	"1981",
	"Sega",
	"Dave Fish (security consultant)\nMike Balfour (game driver)",
	0,
	&s005_machine_driver,

	s005_rom,
	s005_decode, 0,
	s005_sample_names,
	0,	/* sound_prom */

	s005_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	s005_hiload, s005_hisave
};

struct GameDriver monsterb_driver =
{
	__FILE__,
	0,
	"monsterb",
	"Monster Bash",
	"1982",
	"Sega",
	"Dave Fish (security consultant)\nMike Balfour (game driver)",
	0,
	&monsterb_machine_driver,

	monsterb_rom,
	monsterb_decode, 0,
	monsterb_sample_names,
	0,	/* sound_prom */

	monsterb_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	monsterb_hiload, monsterb_hisave
};

struct GameDriver spaceod_driver =
{
	__FILE__,
	0,
	"spaceod",
	"Space Odyssey",
	"1981",
	"Sega",
	"Dave Fish (security consultant)\nMike Balfour (game driver)",
	0,
	&spaceod_machine_driver,

	spaceod_rom,
	spaceod_decode, 0,
	spaceod_sample_names,
	0,	/* sound_prom */

	spaceod_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	spaceod_hiload, spaceod_hisave
};

