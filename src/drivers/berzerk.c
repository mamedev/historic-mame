/***************************************************************************

    Berzerk Driver by Zsolt Vasvari

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char* magicram;

void berzerk_init_machine();

int  berzerk_interrupt();
void berzerk_interrupt_enable_w(int offset,int data);
void berzerk_enable_nmi_w(int offset,int data);
void berzerk_disable_nmi_w(int offset,int data);
int  berzerk_enable_nmi_r(int offset);
int  berzerk_disable_nmi_r(int offset);
int berzerk_led_on_w(int offset);
int berzerk_led_off_w(int offset);

void berzerk_videoram_w(int offset,int data);

void berzerk_colorram_w(int offset,int data);

void berzerk_magicram_w(int offset,int data);
int  berzerk_magicram_r(int offset);
void berzerk_magicram_control_w(int offset,int data);
int  berzerk_collision_r(int offset);

void berzerk_vh_screenrefresh(struct osd_bitmap *bitmap);

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x07ff, MRA_ROM},
        { 0x0800, 0x09ff, MRA_RAM},
        { 0x1000, 0x3fff, MRA_ROM},
        { 0x4000, 0x5fff, videoram_r},
        { 0x6000, 0x7fff, berzerk_magicram_r},
        { 0x8000, 0x87ff, colorram_r},
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x0800, 0x09ff, MWA_RAM},
        { 0x4000, 0x5fff, berzerk_videoram_w, &videoram, &videoram_size},
        { 0x6000, 0x7fff, berzerk_magicram_w, &magicram},
        { 0x8000, 0x87ff, berzerk_colorram_w, &colorram},
        { -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
        { 0x40, 0x47, IORP_NOP}, /* Sound stuff */
        { 0x48, 0x48, input_port_0_r},
        { 0x49, 0x49, input_port_1_r},
        { 0x4a, 0x4a, input_port_7_r}, /* Same as 48 for Player 2 */
        { 0x4c, 0x4c, berzerk_enable_nmi_r},
        { 0x4d, 0x4d, berzerk_disable_nmi_r},
        { 0x4e, 0x4e, berzerk_collision_r},
        { 0x50, 0x57, IORP_NOP}, /* Sound stuff */
        { 0x60, 0x60, input_port_2_r},
        { 0x61, 0x61, input_port_3_r},
        { 0x62, 0x62, IORP_NOP},   /* I really need 1 more I/O port here */
        { 0x63, 0x63, input_port_4_r},
        { 0x64, 0x64, input_port_5_r},
        { 0x65, 0x65, input_port_6_r},
        { 0x66, 0x66, berzerk_led_off_w},
        { 0x67, 0x67, berzerk_led_on_w},
        { -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
        { 0x40, 0x47, IOWP_NOP}, /* Sound stuff */
        { 0x4b, 0x4b, berzerk_magicram_control_w},
        { 0x4c, 0x4c, berzerk_enable_nmi_w},
        { 0x4d, 0x4d, berzerk_disable_nmi_w},
        { 0x4f, 0x4f, berzerk_interrupt_enable_w},
        { 0x50, 0x57, IOWP_NOP}, /* Sound stuff */
        { -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x7c, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* Has to do with */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_DIPNAME( 0xC0, 0x00, "Language", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "English" )
        PORT_DIPSETTING(    0x40, "German" )
        PORT_DIPSETTING(    0x80, "French" )
        PORT_DIPSETTING(    0xc0, "Spanish" )

        PORT_START      /* IN3 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* Has to do with */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_DIPNAME( 0x40, 0x40, "Extra Man at  5,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Extra Man at 10,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x80, "On" )

        PORT_START      /* IN4 - Coin Chute 2*/
        PORT_BIT( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

        PORT_START      /* IN5 */
        PORT_DIPNAME( 0x0f, 0x00, "Credits/Coins", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, " 1/1" )
        PORT_DIPSETTING(    0x01, " 2/1" )
        PORT_DIPSETTING(    0x02, " 3/1" )
        PORT_DIPSETTING(    0x03, " 4/1" )
        PORT_DIPSETTING(    0x04, " 5/1" )
        PORT_DIPSETTING(    0x05, " 6/1" )
        PORT_DIPSETTING(    0x06, " 7/1" )
        PORT_DIPSETTING(    0x07, "10/1" )
        PORT_DIPSETTING(    0x08, "14/1" )
        PORT_DIPSETTING(    0x09, " 1/2" )
        PORT_DIPSETTING(    0x0a, " 3/2" )
        PORT_DIPSETTING(    0x0b, " 5/2" )
        PORT_DIPSETTING(    0x0c, " 7/2" )
        PORT_DIPSETTING(    0x0d, " 3/4" )
        PORT_DIPSETTING(    0x0e, " 5/4" )
        PORT_DIPSETTING(    0x0f, " 7/4" )
        PORT_BIT( 0xf0, IP_ACTIVE_LOW,  IPT_UNUSED )

        PORT_START      /* IN6 */
        PORT_DIPNAME( 0x01, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_BITX(0x80, IP_ACTIVE_HIGH, 0, "Stats", OSD_KEY_F1, IP_JOY_NONE, 0 )

        PORT_START      /* IN7 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_DIPNAME( 0x80, 0x80, "Cocktail Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_DIPSETTING(    0x00, "On" )

INPUT_PORTS_END


/* Simple 1-bit RGBI palette */
unsigned char berzerk_palette[16 * 3] =
{
        0x00, 0x00, 0x00,
        0xff, 0x00, 0x00,
        0x00, 0xff, 0x00,
        0xff, 0xff, 0x00,
        0x00, 0x00, 0xff,
        0xff, 0x00, 0xff,
        0x00, 0xff, 0xff,
        0xff, 0xff, 0xff,
        0x40, 0x40, 0x40,
        0xff, 0x40, 0x40,
        0x40, 0xff, 0x40,
        0xff, 0xff, 0x40,
        0x40, 0x40, 0xff,
        0xff, 0x40, 0xff,
        0x40, 0xff, 0xff,
        0xff, 0xff, 0xff
};


static struct MachineDriver berzerk_machine_driver =
{
	/* basic machine hardware */
	{
			{
					CPU_Z80,
					4000000,        /* 4 Mhz ??? */
					0,
					readmem,writemem,readport,writeport,
					berzerk_interrupt,8
			},
	},
	60,
	1,	/* single CPU, no need for interleaving */
	berzerk_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	0,
	sizeof(berzerk_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	berzerk_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



static int hiload(void)
{
        unsigned char *RAM = Machine->memory_region[0];

        void *f;


        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
            osd_fread(f,&RAM[0x0800],0x0800);
            osd_fclose(f);
        }

        return 1;
}



static void hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[0];


        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0x0800],0x800);
                osd_fclose(f);
        }
}



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( berzerk1_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "rom0.1c", 0x0000, 0x0800, 0x6a3d0405 )
        ROM_LOAD( "rom1.1d", 0x1000, 0x0800, 0xa014ad50 )
        ROM_LOAD( "rom2.3d", 0x1800, 0x0800, 0x9501b63b )
        ROM_LOAD( "rom3.5d", 0x2000, 0x0800, 0x5c2a2996 )
        ROM_LOAD( "rom4.6d", 0x2800, 0x0800, 0x1558c1da )
        ROM_LOAD( "rom5.5c", 0x3000, 0x0800, 0xd35649e0 )
ROM_END

ROM_START( berzerk_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "1c-0", 0x0000, 0x0800, 0x20874015 )
        ROM_LOAD( "1d-1", 0x1000, 0x0800, 0x98ca9778 )
        ROM_LOAD( "3d-2", 0x1800, 0x0800, 0x75212e73 )
        ROM_LOAD( "5d-3", 0x2000, 0x0800, 0xfd913cd1 )
        ROM_LOAD( "6d-4", 0x2800, 0x0800, 0x255e05f8 )
        ROM_LOAD( "5c-5", 0x3000, 0x0800, 0x5c8d0cf9 )
ROM_END


struct GameDriver berzerk_driver =
{
        "Berzerk",
        "berzerk",
        "Zsolt Vasvari\nChristopher Kirmse\nMirko Buffoni\nValerio Verrando",
        &berzerk_machine_driver,

        berzerk_rom,
        0, 0,
        0,

        0, input_ports, 0, 0, 0,

        0, berzerk_palette, 0,

        ORIENTATION_DEFAULT,

        hiload, hisave
};


struct GameDriver berzerk1_driver =
{
        "Berzerk (version 1)",
        "berzerk1",
        "Zsolt Vasvari\nChristopher Kirmse\nMirko Buffoni",
        &berzerk_machine_driver,

        berzerk1_rom,
        0, 0,
        0,

        0, input_ports, 0, 0, 0,

        0, berzerk_palette, 0,

        ORIENTATION_DEFAULT,

        hiload, hisave
};


