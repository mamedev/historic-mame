#include "driver.h"
#include "M68000/M68000.h"
#include "vidhrdw/generic.h"



int ssi_input_r (int offset)
{
    switch (offset)
    {

         case 0x04:
              return readinputport(0); /* IN0 */

         case 0x06:
              return readinputport(1); /* IN1 */

         case 0x0e:
              return readinputport(2); /* IN2 */

         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */
    }

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x100000+offset);

	return 0xff;
}


int ssi_videoram_r(int offset);

void ssi_videoram_w(int offset,int data);

int ssi_vh_start(void);
void ssi_vh_stop (void);
int  ssi_interrupt(void) ;


void ssi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
static struct MemoryReadAddress ssi_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
        { 0x100000, 0x10000f, ssi_input_r },
        { 0x200000, 0x20ffff, MRA_BANK1 },
        { 0x300000, 0x301fff, paletteram_word_r },
        { 0x400002, 0x400003, MRA_NOP },
        { 0x800000, 0x80ffff, ssi_videoram_r}, //MRA_BANK3 },

        { -1 }  /* end of table */
};

static struct MemoryWriteAddress ssi_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
        { 0x100000, 0x10000f, MWA_NOP },  /* input and dsw */
        { 0x200000, 0x20ffff, MWA_BANK1 }, /* 68000 ram */
        { 0x300000, 0x301fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram },
        { 0x400000, 0x4000ff, MWA_NOP }, /* ??  */
        { 0x500000, 0x500001, MWA_NOP },
        { 0x800000, 0x80ffff, ssi_videoram_w, &videoram, &videoram_size }, /* sprite ram */

        { -1 }  /* end of table */
};

/*
static struct MemoryReadAddress sound_readmem[] =
{
        {0x0000, 0x7fff, MRA_ROM },

        { -1 }

};

static struct MemoryWriteAddress sound_writemem[] =
{
        {0x0000, 0x7fff, MWA_ROM },
        {0xeffe, 0xefff, MWA_NOP },
        { -1 }
};
*/

INPUT_PORTS_START( ssi_input_ports )

        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START /* DSW A */
        PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE)
        PORT_DIPSETTING(    0x01, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE)
        PORT_DIPSETTING(    0x02, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x04, 0x04, "Test Mode", IP_KEY_NONE)
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE)
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE)
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
        PORT_DIPSETTING(    0x10, "3 Coins/1 Credit")
        PORT_DIPSETTING(    0x20, "2 Coins/1 Credit")
        PORT_DIPSETTING(    0x30, "1 Coin/1 Credit")
        PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE)
        PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits")
        PORT_DIPSETTING(    0x80, "1 Coin/3 Credits")
        PORT_DIPSETTING(    0x40, "1 Coin/4 Credits")
        PORT_DIPSETTING(    0x00, "1 Coin/6 Credits")

        PORT_START /* DSW B */
        PORT_DIPNAME( 0x01, 0x01, "Unknown",IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x02, 0x02, "Unknown",IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "Off")
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPNAME( 0x0c, 0x0c, "Shields",IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "None")
        PORT_DIPSETTING(    0x0c, "1")
        PORT_DIPSETTING(    0x04, "2")
        PORT_DIPSETTING(    0x08, "3")
        PORT_DIPNAME( 0x10, 0x10, "Lives",IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "2")
        PORT_DIPSETTING(    0x10, "3")
        PORT_DIPNAME( 0x20, 0x20, "2 Players Mode", IP_KEY_NONE)
        PORT_DIPSETTING(    0x00, "Alternate")
        PORT_DIPSETTING(    0x20, "Simultaneous")
        PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE)
        PORT_DIPSETTING(    0x40, "Yes")
        PORT_DIPSETTING(    0x00, "No")
        PORT_DIPNAME( 0x80, 0x80, "Allow Simultaneous Game", IP_KEY_NONE)
        PORT_DIPSETTING(    0x80, "Yes")
        PORT_DIPSETTING(    0x00, "No")
     /* I think the cabinet for this game should have two joysticks even
        in upright mode. Maybe this dip actually set cocktail mode where
        simultaneous game is now allowed of course.
        Or maybe it's just what I described... Sand666 21/5 */

INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
        16,16,  /* 16*16 sprites */
          8192,        /* 256 of them */
        4,              /* 4 bits per pixel */
        { 0, 1, 2, 3 },
        { 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
//        { 15*64, 14*64, 13*64, 12*64, 11*64, 10*64, 9*64, 8*64, 7*64, 6*64, 5*64, 4*64, 3*64, 2*64, 1*64, 0*64 },
        { 14*4, 15*4, 12*4, 13*4, 10*4, 11*4, 8*4, 9*4, 6*4, 7*4, 4*4, 5*4, 2*4, 3*4, 0*4, 1*4 },
//        { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },

        128*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ssi_gfxdecodeinfo[] =
{
        { 1, 0x000000, &tilelayout,    0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			6000000,	/* 6 Mhz */
			0,
                        ssi_readmem,ssi_writemem,0,0,
                        ssi_interrupt,1
		},
/*
                {
                        CPU_Z80 |CPU_AUDIO_CPU,
                        3579545,
                        3,
                        sound_readmem, sound_writemem,0,0,
                        ignore_interrupt,0
                }

*/
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
        30*8, 42*8, { 2*8, 30*8-1, 2*8, 42*8-1 },

	ssi_gfxdecodeinfo,
	256*16,256*16,
        0,

        VIDEO_TYPE_RASTER| VIDEO_MODIFIES_PALETTE,
	0,
	ssi_vh_start,
	ssi_vh_stop,
        ssi_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};



/***************************************************************************

  High score save/load

***************************************************************************/


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ssi_rom )
	ROM_REGION(0x80000)     /* 524k for 68000 code */

	ROM_LOAD_EVEN( "ssi_15-1.rom",  0x00000, 0x40000, 0xa67c15aa )
	ROM_LOAD_ODD ( "ssi_16-1.rom",   0x00000, 0x40000, 0x7224f9bc )

	ROM_REGION(0x100000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ssi_m01.rom", 0x000000, 0x100000, 0x83fc9e0e )

//	ROM_REGION(0x10000)      /* sound cpu */
//	ROM_LOAD( "ssi_09.rom",  0x0000, 0x10000, 0x87997597 )
ROM_END



struct GameDriver ssi_driver =
{
	__FILE__,
	0,
	"ssi",
	"Super Space Invaders '91",
	"1990",
	"Taito",
	"Howie Cohen \nAlex Pasadyn \nBill Boyle (graphics info) \nRichard Bush (technical information)",
	0,
	&machine_driver,

	ssi_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ssi_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


