/***************************************************************************

  Capcom System 2
  ===============




***************************************************************************/

#if 1
/* Graphics viewer functions */
extern int  cps2_vh_start(void);
extern void cps2_vh_stop(void);
extern void cps2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
#else
/* Dummy CPS1 functions */
#define cps2_vh_start			cps1_ch_start
#define cps2_vh_stop			cps2_vh_stop
#define cps2_vh_screenrefresh	cps2_vh_screenrefresh
#endif

/* Export this function so that the vidhrdw routine can drive the
Q-Sound hardware
*/
void cps2_qsound_sharedram_w(int offset,int data)
{
    qsound_sharedram_w(offset, data);
}

#define CPS2_GAME_DRIVER(NAME,DESCRIPTION,YEAR,MANUFACTURER,ORIENTATION) \
struct GameDriver driver_##NAME =  \
{                                  \
	__FILE__,                      \
	0,                             \
	#NAME,                         \
	DESCRIPTION,                   \
	YEAR,                          \
	MANUFACTURER,                  \
	CPS2_CREDITS,                  \
	0,                             \
	&machine_driver_##NAME,        \
	cps2_decode,                   \
	rom_##NAME,                    \
	0, 0,                          \
	0,                             \
	0,                             \
	input_ports_##NAME,            \
	0, 0, 0,                       \
	ORIENTATION,                   \
	0, 0                           \
};

#define CLONE_CPS2_GAME_DRIVER(NAME,MAIN,DESCRIPTION,YEAR,MANUFACTURER,ORIENTATION) \
struct GameDriver driver_##NAME =  \
{                                  \
	__FILE__,                      \
	&driver_##MAIN,                \
	#NAME,                         \
	DESCRIPTION,                   \
	YEAR,                          \
	MANUFACTURER,                  \
	CPS2_CREDITS,                  \
	0,                             \
	&machine_driver_##MAIN,        \
	cps2_decode,                   \
	rom_##NAME,                    \
	0, 0,                          \
	0,                             \
	0,                             \
	input_ports_##MAIN,            \
	0, 0, 0,                       \
	ORIENTATION,                   \
	0, 0                           \
};


/* Maximum size of Q Sound Z80 region */
#define QSOUND_SIZE 0x50000

/* Maximum 680000 code size */
#undef  CODE_SIZE
#define CODE_SIZE   0x0800000

#define CPS2_CREDITS            ""
#define CPS2_DEFAULT_CPU_SPEED  10000000

void cps2_decode(void)
{
    unsigned char *RAM = memory_region(REGION_CPU1);
    FILE *fp;
    int i;
    const int decode=CODE_SIZE/2;

    fp = fopen ("ROM.DMP", "w+b");
    if (fp)
    {
        for (i=0; i<decode; i+=2)
        {
            int value=READ_WORD(&RAM[i]);
            fputc(value>>8,   fp);
            fputc(value&0xff, fp);
        }
        fclose(fp);
    }


	/* Decrypt it ! */

    fp=fopen ("ROMD.DMP", "w+b");
    if (fp)
    {
        for (i=0; i<decode; i+=2)
        {
            int value=READ_WORD(&RAM[decode+i]);
            fputc(value>>8,   fp);
            fputc(value&0xff, fp);
        }

        fclose(fp);
    }


    /*
    Poke in a dummy program to stop the 68K core from crashing due
    to silly addresses.
    */
    WRITE_WORD(&RAM[0x0000], 0x00ff);
    WRITE_WORD(&RAM[0x0002], 0x8000);  /* Dummy stack pointer */
    WRITE_WORD(&RAM[0x0004], 0x0000);
    WRITE_WORD(&RAM[0x0006], 0x00c2);  /* Dummy start vector */

    for (i=0x0008; i<0x00c0; i+=4)
    {
        WRITE_WORD(&RAM[i+0], 0x0000);
        WRITE_WORD(&RAM[i+2], 0x00c0);
    }

    WRITE_WORD(&RAM[0x00c0], 0x4e73);   /* RETE */
    WRITE_WORD(&RAM[0x00c2], 0x6000);
    WRITE_WORD(&RAM[0x00c4], 0x00c2);   /* BRA 00c2 */
}

#define CPS2_MACHINE_DRIVER(CPS1_DRVNAME, CPS1_CPU_FRQ) \
static struct MachineDriver machine_driver_##CPS1_DRVNAME =              \
{                                                                        \
    /* basic machine hardware */                                         \
    {                                                                    \
        {                                                                \
            CPU_M68000,                                                  \
            CPS1_CPU_FRQ,                                                \
            cps1_readmem,cps1_writemem,0,0,                              \
            cps1_qsound_interrupt, 1  /* ??? interrupts per frame */     \
        },                                                               \
        {                                                                \
            CPU_Z80 | CPU_AUDIO_CPU,                                     \
            8000000,  /* 4 Mhz ??? TODO: find real FRQ */                \
            qsound_readmem,qsound_writemem,0,0,                          \
            interrupt,4                                                  \
        }                                                                \
    },                                                                   \
    60, 0,                                                               \
    1,                                                                   \
    0,                                                                   \
                                                                         \
    /* video hardware */                                                 \
    0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },\
                                                                         \
    cps1_gfxdecodeinfo,                                                  \
    32*16+32*16+32*16+32*16,   /* lotsa colours */                       \
    32*16+32*16+32*16+32*16,   /* Colour table length */                 \
    0,                                                                   \
                                                                         \
    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                          \
    0,                                                                   \
    cps2_vh_start,                                                       \
    cps2_vh_stop,                                                        \
    cps2_vh_screenrefresh,                                               \
                                                                         \
    /* sound hardware */                                                 \
    SOUND_SUPPORTS_STEREO,0,0,0,                                         \
    { { SOUND_QSOUND, &qsound_interface } }                              \
};

INPUT_PORTS_START( sfa )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE  )	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

#define input_ports_armwara     input_ports_sfa
#define input_ports_avspj       input_ports_sfa
#define input_ports_batcirj     input_ports_sfa
#define input_ports_ddtodu      input_ports_sfa
#define input_ports_mshj        input_ports_sfa
#define input_ports_mshvsfu     input_ports_sfa
#define input_ports_rckman2j    input_ports_sfa
#define input_ports_sfzj        input_ports_sfa
#define input_ports_sfz2j       input_ports_sfa
#define input_ports_ssf2j       input_ports_sfa
#define input_ports_ssf2xj      input_ports_sfa
#define input_ports_vampj       input_ports_sfa
#define input_ports_vhuntj      input_ports_sfa
#define input_ports_vhunt2j     input_ports_sfa
#define input_ports_vsavj       input_ports_sfa
#define input_ports_vsav2j      input_ports_sfa
#define input_ports_xmcotaj     input_ports_sfa
#define input_ports_xmvssfj     input_ports_sfa

CPS2_MACHINE_DRIVER( armwara,  	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( avspj,    	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( batcirj,     CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( ddtodu,  	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( mshj,     	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( mshvsfu, 	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( rckman2j, 	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( sfzj,  	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( sfz2j,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( ssf2j,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( ssf2xj,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vampj,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vhuntj,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vhunt2j,  	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vsavj,    	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vsav2j,   	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( xmcotaj, 	  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( xmvssfj,  	  CPS2_DEFAULT_CPU_SPEED )

ROM_START( armwara )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("armwar_a.c03", 0x000000, 0x80000, 0x8d474ab1 )
    ROM_LOAD_WIDE_SWAP("armwar_a.c04", 0x080000, 0x80000, 0x81b5aec7 )
    ROM_LOAD_WIDE_SWAP("armwar_a.c05", 0x100000, 0x80000, 0x2618e819 )
    ROM_LOAD_WIDE_SWAP("armwar_a.c06", 0x180000, 0x80000, 0x87a60ce8 )
    ROM_LOAD_WIDE_SWAP("armwar_a.c07", 0x200000, 0x80000, 0xf7b148df )
    ROM_LOAD_WIDE_SWAP("armwar_a.c08", 0x280000, 0x80000, 0xcc62823e )
    ROM_LOAD_WIDE_SWAP("armwar_a.c09", 0x300000, 0x80000, 0xddc85ca6 )
    ROM_LOAD_WIDE_SWAP("armwar_a.c10", 0x380000, 0x80000, 0x07c4fb28 )

    ROM_REGION_DISPOSE(0x1400000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "armwar.g18",   0x0000000, 0x100000, 0x0109c71b )
    ROM_LOAD( "armwar.g17",   0x0100000, 0x400000, 0xbc475b94 )
    ROM_LOAD( "armwar.g14",   0x0500000, 0x100000, 0xc3f9ba63 )
    ROM_LOAD( "armwar.g13",   0x0600000, 0x400000, 0xae8fe08e )
    ROM_LOAD( "armwar.g20",   0x0a00000, 0x100000, 0xeb75ffbe )
    ROM_LOAD( "armwar.g19",   0x0b00000, 0x400000, 0x07439ff7 )
    ROM_LOAD( "armwar.g16",   0x0f00000, 0x100000, 0x815b0e7b )
    ROM_LOAD( "armwar.g15",   0x1000000, 0x400000, 0xdb560f58 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "armwar.01",         0x00000, 0x08000, 0x18a5c0e4 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "armwar.02",         0x28000, 0x20000, 0xc9dfffa6 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "armwar.s11",   0x000000, 0x200000, 0xa78f7433 )
    ROM_LOAD( "armwar.s12",   0x200000, 0x200000, 0x77438ed0 )
ROM_END

ROM_START( avspj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("avsp_j.c03", 0x000000, 0x80000, 0x42757950 )
    ROM_LOAD_WIDE_SWAP("avsp_j.c04", 0x080000, 0x80000, 0x5abcdee6 )
    ROM_LOAD_WIDE_SWAP("avsp_j.c05", 0x100000, 0x80000, 0xfbfb5d7a )
    ROM_LOAD_WIDE_SWAP("avsp_j.c06", 0x180000, 0x80000, 0x190b817f )

    ROM_REGION_DISPOSE(0x1000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "avsp.g18",    0x000000, 0x200000, 0xd92b6fc0 )
    ROM_LOAD( "avsp.g17",    0x200000, 0x200000, 0x94403195 )
    ROM_LOAD( "avsp.g14",    0x400000, 0x200000, 0xebba093e )
    ROM_LOAD( "avsp.g13",    0x600000, 0x200000, 0x8f8b5ae4 )
    ROM_LOAD( "avsp.g20",    0x800000, 0x200000, 0xf90baa21 )
    ROM_LOAD( "avsp.g19",    0xa00000, 0x200000, 0xe1981245 )
    ROM_LOAD( "avsp.g16",    0xc00000, 0x200000, 0xfb228297 )
    ROM_LOAD( "avsp.g15",    0xe00000, 0x200000, 0xb00280df )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "avsp.01",         0x00000, 0x08000, 0x2d3b4220 )
    ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "avsp.s11",   0x000000, 0x200000, 0x83499817 )
    ROM_LOAD( "avsp.s12",   0x200000, 0x200000, 0xf4110d49 )
ROM_END

ROM_START( batcirj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("batcir_j.c03", 0x000000, 0x80000, 0x6b7e168d )
    ROM_LOAD_WIDE_SWAP("batcir_j.c04", 0x080000, 0x80000, 0x46ba3467 )
    ROM_LOAD_WIDE_SWAP("batcir_j.c05", 0x100000, 0x80000, 0x0e23a859 )
    ROM_LOAD_WIDE_SWAP("batcir_j.c06", 0x180000, 0x80000, 0xa853b59c )
    ROM_LOAD_WIDE_SWAP("batcir_j.c07", 0x200000, 0x80000, 0x7322d5db )
    ROM_LOAD_WIDE_SWAP("batcir_j.c08", 0x280000, 0x80000, 0x6aac85ab )
    ROM_LOAD_WIDE_SWAP("batcir_j.c09", 0x300000, 0x80000, 0x1203db08 )

    ROM_REGION_DISPOSE(0x1000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "batcir.g17",   0x000000, 0x400000, 0xb33f4112 )
    ROM_LOAD( "batcir.g13",   0x400000, 0x400000, 0xdc705bad )
    ROM_LOAD( "batcir.g19",   0x800000, 0x400000, 0xa6fcdb7e )
    ROM_LOAD( "batcir.g15",   0xC00000, 0x400000, 0xe5779a3c )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "batcir.01",         0x00000, 0x08000, 0x1e194310 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "batcir.02",         0x28000, 0x20000, 0x01aeb8e6 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "batcir.s11",   0x000000, 0x200000, 0xc27f2229 )
    ROM_LOAD( "batcir.s12",   0x200000, 0x200000, 0x418a2e33 )
ROM_END

ROM_START( ddtodu )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("ddtod_u.c03", 0x000000, 0x80000, 0xa519905f )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c04", 0x080000, 0x80000, 0x52562d38 )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c05", 0x100000, 0x80000, 0xee1cfbfe )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c06", 0x180000, 0x80000, 0x13aa3e56 )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c07", 0x200000, 0x80000, 0x431cb6dd )

	ROM_REGION_DISPOSE(0xc00000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ddtod.g18",      0x000000, 0x100000, 0xcef393ef )
	ROM_LOAD( "ddtod.g17",      0x100000, 0x200000, 0xb98757f5 )
	ROM_LOAD( "ddtod.g14",      0x300000, 0x100000, 0x837e6f3f )
	ROM_LOAD( "ddtod.g13",      0x400000, 0x200000, 0xda3cb7d6 )
	ROM_LOAD( "ddtod.g20",      0x600000, 0x100000, 0x8953fe9e )
	ROM_LOAD( "ddtod.g19",      0x700000, 0x200000, 0x8121ce46 )
	ROM_LOAD( "ddtod.g16",      0x900000, 0x100000, 0xf0916bdb )
	ROM_LOAD( "ddtod.g15",      0xa00000, 0x200000, 0x92b63172 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "ddtod.01",    0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "ddtod.s11",   0x000000, 0x200000, 0x0c499b67 )
    ROM_LOAD( "ddtod.s12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( ddtoda )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("ddtod_a.c03", 0x000000, 0x80000, 0xfc6f2dd7 )
    ROM_LOAD_WIDE_SWAP("ddtod_a.c04", 0x080000, 0x80000, 0xd4be4009 )
    ROM_LOAD_WIDE_SWAP("ddtod_a.c05", 0x100000, 0x80000, 0x6712d1cf )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c06", 0x180000, 0x80000, 0x13aa3e56 )
    ROM_LOAD_WIDE_SWAP("ddtod_u.c07", 0x200000, 0x80000, 0x431cb6dd )

	ROM_REGION_DISPOSE(0xc00000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ddtod.g18",      0x000000, 0x100000, 0xcef393ef )
	ROM_LOAD( "ddtod.g17",      0x100000, 0x200000, 0xb98757f5 )
	ROM_LOAD( "ddtod.g14",      0x300000, 0x100000, 0x837e6f3f )
	ROM_LOAD( "ddtod.g13",      0x400000, 0x200000, 0xda3cb7d6 )
	ROM_LOAD( "ddtod.g20",      0x600000, 0x100000, 0x8953fe9e )
	ROM_LOAD( "ddtod.g19",      0x700000, 0x200000, 0x8121ce46 )
	ROM_LOAD( "ddtod.g16",      0x900000, 0x100000, 0xf0916bdb )
	ROM_LOAD( "ddtod.g15",      0xa00000, 0x200000, 0x92b63172 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "ddtod.01",    0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "ddtod.s11",   0x000000, 0x200000, 0x0c499b67 )
    ROM_LOAD( "ddtod.s12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( mshj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("msh_j.c03", 0x000000, 0x80000, 0xd2805bdd )
    ROM_LOAD_WIDE_SWAP("msh_j.c04", 0x080000, 0x80000, 0x743f96ff )
    ROM_LOAD_WIDE_SWAP("msh_j.c05", 0x100000, 0x80000, 0x6a091b9e )
    ROM_LOAD_WIDE_SWAP("msh_j.c06", 0x180000, 0x80000, 0x803e3fa4 )
    ROM_LOAD_WIDE_SWAP("msh_j.c07", 0x200000, 0x80000, 0xc45f8e27 )
    ROM_LOAD_WIDE_SWAP("msh_j.c08", 0x280000, 0x80000, 0x9ca6f12c )
    ROM_LOAD_WIDE_SWAP("msh_j.c09", 0x300000, 0x80000, 0x82ec27af )
    ROM_LOAD_WIDE_SWAP("msh_j.c10", 0x380000, 0x80000, 0x8d931196 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "msh.g18",        0x0000000, 0x400000, 0x4db92d94 )
    ROM_LOAD( "msh.g17",        0x0400000, 0x400000, 0x604ece14 )
    ROM_LOAD( "msh.g14",        0x0800000, 0x400000, 0x4197973e )
    ROM_LOAD( "msh.g13",        0x0c00000, 0x400000, 0x0031a54e )
    ROM_LOAD( "msh.g20",        0x1000000, 0x400000, 0xa2b0c6c0 )
    ROM_LOAD( "msh.g19",        0x1400000, 0x400000, 0x94a731e8 )
    ROM_LOAD( "msh.g16",        0x1800000, 0x400000, 0x438da4a0 )
    ROM_LOAD( "msh.g15",        0x1c00000, 0x400000, 0xee962057 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "msh.01",         0x00000, 0x08000, 0xc976e6f9 )
    ROM_CONTINUE(                0x10000, 0x18000 )
    ROM_LOAD( "msh.02",         0x28000, 0x20000, 0xce67d0d9 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "msh.s11",   0x000000, 0x200000, 0x37ac6d30 )
    ROM_LOAD( "msh.s12",   0x200000, 0x200000, 0xde092570 )
ROM_END

ROM_START( mshvsfu )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c03", 0x000000, 0x80000, 0xae60a66a )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c04", 0x080000, 0x80000, 0x91f67d8a )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c05", 0x100000, 0x80000, 0x1a5de0cb )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c06", 0x180000, 0x80000, 0x959f3030 )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c07", 0x200000, 0x80000, 0x7f915bdb )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c08", 0x280000, 0x80000, 0xc2813884 )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c09", 0x300000, 0x80000, 0x3ba08818 )
    ROM_LOAD_WIDE_SWAP("mshvsf_u.c10", 0x380000, 0x80000, 0xcf0dba98 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "mshvsf.g18",        0x0000000, 0x400000, 0xc1228b35 )
    ROM_LOAD( "mshvsf.g17",        0x0400000, 0x400000, 0x97aaf4c7 )
    ROM_LOAD( "mshvsf.g14",        0x0800000, 0x400000, 0xb3b1972d )
    ROM_LOAD( "mshvsf.g13",        0x0c00000, 0x400000, 0x29b05fd9 )
    ROM_LOAD( "mshvsf.g20",        0x1000000, 0x400000, 0x366cc6c2 )
    ROM_LOAD( "mshvsf.g19",        0x1400000, 0x400000, 0xcb70e915 )
    ROM_LOAD( "mshvsf.g16",        0x1800000, 0x400000, 0x08aadb5d )
    ROM_LOAD( "mshvsf.g15",        0x1c00000, 0x400000, 0xfaddccf1 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "mshvsf.01",         0x00000, 0x08000, 0x68252324 )
    ROM_CONTINUE(               0x10000, 0x18000 )
    ROM_LOAD( "mshvsf.02",         0x28000, 0x20000, 0xb34e773d )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "mshvsf.s11",   0x000000, 0x400000, 0x86219770 )
    ROM_LOAD( "mshvsf.s12",   0x400000, 0x400000, 0xf2fd7f68 )
ROM_END

ROM_START( rckman2j )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("rckmn2_j.c03",  0x000000, 0x80000, 0xdbaa1437 )
    ROM_LOAD_WIDE_SWAP("rckmn2_j.c04",  0x080000, 0x80000, 0xcf5ba612 )
    ROM_LOAD_WIDE_SWAP("rckmn2_j.c05",  0x100000, 0x80000, 0x02ee9efc )

    ROM_REGION_DISPOSE(0x800000) /* load samples as graphics */
    ROM_LOAD( "rckmn2.g18",     0x000000, 0x200000, 0x12257251 )
    ROM_LOAD( "rckmn2.g14",     0x200000, 0x200000, 0x9b1f00b4 )
	ROM_LOAD( "rckmn2.g20",     0x400000, 0x200000, 0xf9b6e786 )
    ROM_LOAD( "rckmn2.g16",     0x600000, 0x200000, 0xc2bb0c24 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "rckmn2.01",     0x00000, 0x08000, 0xd18e7859 )
    ROM_CONTINUE(               0x10000, 0x18000 )
    ROM_LOAD( "rckmn2.02",     0x28000, 0x20000, 0xc463ece0 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "rckmn2.s11",   0x000000, 0x200000, 0x2106174d )
    ROM_LOAD( "rckmn2.s12",   0x200000, 0x200000, 0x546c1636 )
ROM_END

ROM_START( sfzj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfz_j.c03",  0x000000, 0x80000, 0xf5444120 )
    ROM_LOAD_WIDE_SWAP("sfz_j.c04",  0x080000, 0x80000, 0x8b73b0e5 )
    ROM_LOAD_WIDE_SWAP("sfz_j.c05",  0x100000, 0x80000, 0x0810544d )
    ROM_LOAD_WIDE_SWAP("sfz_j.c06",  0x180000, 0x80000, 0x806e8f38 )

    ROM_REGION_DISPOSE(0x800000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sfz.g18",     0x000000, 0x200000, 0x41a1e790 )
    ROM_LOAD( "sfz.g14",     0x200000, 0x200000, 0x90fefdb3 )
    ROM_LOAD( "sfz.g20",     0x400000, 0x200000, 0xa549df98 )
    ROM_LOAD( "sfz.g16",     0x600000, 0x200000, 0x5354c948 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz.01",          0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "sfz.02",          0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfz.s11",         0x000000, 0x200000, 0xc4b093cd )
    ROM_LOAD( "sfz.s12",         0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfau )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfa_u.c03",  0x000000, 0x80000, 0xebf2054d )
    ROM_LOAD_WIDE_SWAP("sfz_j.c04",  0x080000, 0x80000, 0x8b73b0e5 )
    ROM_LOAD_WIDE_SWAP("sfz_j.c05",  0x100000, 0x80000, 0x0810544d )
    ROM_LOAD_WIDE_SWAP("sfz_j.c06",  0x180000, 0x80000, 0x806e8f38 )

    ROM_REGION_DISPOSE(0x800000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sfz.g18",     0x000000, 0x200000, 0x41a1e790 )
    ROM_LOAD( "sfz.g14",     0x200000, 0x200000, 0x90fefdb3 )
    ROM_LOAD( "sfz.g20",     0x400000, 0x200000, 0xa549df98 )
    ROM_LOAD( "sfz.g16",     0x600000, 0x200000, 0x5354c948 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz.01",          0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "sfz.02",          0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfz.s11",         0x000000, 0x200000, 0xc4b093cd )
    ROM_LOAD( "sfz.s12",         0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfzj1 )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfz_j1.c03", 0x000000, 0x80000, 0x844220c2 )
    ROM_LOAD_WIDE_SWAP("sfz_j1.c04", 0x080000, 0x80000, 0x5f99e9a5 )
    ROM_LOAD_WIDE_SWAP("sfz_j.c05",  0x100000, 0x80000, 0x0810544d )
    ROM_LOAD_WIDE_SWAP("sfz_j.c06",  0x180000, 0x80000, 0x806e8f38 )

    ROM_REGION_DISPOSE(0x800000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sfz.g18",     0x000000, 0x200000, 0x41a1e790 )
    ROM_LOAD( "sfz.g14",     0x200000, 0x200000, 0x90fefdb3 )
    ROM_LOAD( "sfz.g20",     0x400000, 0x200000, 0xa549df98 )
    ROM_LOAD( "sfz.g16",     0x600000, 0x200000, 0x5354c948 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz.01",          0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "sfz.02",          0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfz.s11",         0x000000, 0x200000, 0xc4b093cd )
    ROM_LOAD( "sfz.s12",         0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfz2j )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfz2_j.c03", 0x000000, 0x80000, 0x97461e28 )
    ROM_LOAD_WIDE_SWAP("sfz2_j.c04", 0x080000, 0x80000, 0xae4851a9 )
    ROM_LOAD_WIDE_SWAP("sfz2_j.c05", 0x100000, 0x80000, 0x98e8e992 )
    ROM_LOAD_WIDE_SWAP("sfz2_j.c06", 0x180000, 0x80000, 0x5b1d49c0 )
    ROM_LOAD_WIDE_SWAP("sfz2_j.c07", 0x200000, 0x80000, 0xd910b2a2 )
    ROM_LOAD_WIDE_SWAP("sfz2_j.c08", 0x280000, 0x80000, 0x0fe8585d )

    ROM_REGION_DISPOSE(0x1400000)     /* temporary space for graphics (disposed after conversion) */
    /* One of these planes is corrupt */
    ROM_LOAD( "sfz2.g18",    0x0000000, 0x100000, 0x4bc3c8bc )
    ROM_LOAD( "sfz2.g17",    0x0100000, 0x400000, 0xe01b4588 )
    ROM_LOAD( "sfz2.g14",    0x0500000, 0x100000, 0x08c6bb9c )
    ROM_LOAD( "sfz2.g13",    0x0600000, 0x400000, 0x1858afce )
    ROM_LOAD( "sfz2.g20",    0x0a00000, 0x100000, 0x39e674c0 )
    ROM_LOAD( "sfz2.g19",    0x0b00000, 0x400000, 0x0feeda64 )
    ROM_LOAD( "sfz2.g16",    0x0f00000, 0x100000, 0xca161c9c )
    ROM_LOAD( "sfz2.g15",    0x1000000, 0x400000, 0x96fcf35c )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz2.01",         0x00000, 0x08000, 0x1bc323cf )
    ROM_CONTINUE(                0x10000, 0x18000 )
    ROM_LOAD( "sfz2.02",         0x28000, 0x20000, 0xba6a5013 )

    ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfz2.s11",   0x000000, 0x200000, 0xaa47a601 )
    ROM_LOAD( "sfz2.s12",   0x200000, 0x200000, 0x2237bc53 )
ROM_END

ROM_START( ssf2j )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("ssf2_j.c03", 0x000000, 0x80000, 0x7eb0efed )
    ROM_LOAD_WIDE_SWAP("ssf2_j.c04", 0x080000, 0x80000, 0xd7322164 )
    ROM_LOAD_WIDE_SWAP("ssf2_j.c05", 0x100000, 0x80000, 0x0918d19a )
    ROM_LOAD_WIDE_SWAP("ssf2_j.c06", 0x180000, 0x80000, 0xd808a6cd )
    ROM_LOAD_WIDE_SWAP("ssf2_j.c07", 0x200000, 0x80000, 0xeb6a9b1b )

    ROM_REGION_DISPOSE(0xc00000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ssf2.g18",   0x000000, 0x100000, 0xf5b1b336 )
    ROM_LOAD( "ssf2.g17",   0x100000, 0x200000, 0x59f5267b )
    ROM_LOAD( "ssf2.g14",   0x300000, 0x100000, 0xb7cc32e7 )
    ROM_LOAD( "ssf2.g13",   0x400000, 0x200000, 0xcf94d275 )
    ROM_LOAD( "ssf2.g20",   0x600000, 0x100000, 0x459d5c6b )
    ROM_LOAD( "ssf2.g19",   0x700000, 0x200000, 0x8dc0d86e )
    ROM_LOAD( "ssf2.g16",   0x900000, 0x100000, 0x8376ad18 )
    ROM_LOAD( "ssf2.g15",   0xa00000, 0x200000, 0x9073a0d4 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "ssf2.01",         0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "ssf2.s01",   0x000000, 0x080000, 0xa6f9da5c )
    ROM_LOAD( "ssf2.s02",   0x080000, 0x080000, 0x8c66ae26 )
    ROM_LOAD( "ssf2.s03",   0x100000, 0x080000, 0x695cc2ca )
    ROM_LOAD( "ssf2.s04",   0x180000, 0x080000, 0x9d9ebe32 )
    ROM_LOAD( "ssf2.s05",   0x200000, 0x080000, 0x4770e7b7 )
    ROM_LOAD( "ssf2.s06",   0x280000, 0x080000, 0x4e79c951 )
    ROM_LOAD( "ssf2.s07",   0x300000, 0x080000, 0xcdd14313 )
    ROM_LOAD( "ssf2.s08",   0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2xj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c03", 0x000000, 0x80000, 0xa7417b79 )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c04", 0x080000, 0x80000, 0xaf7767b4 )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c05", 0x100000, 0x80000, 0xf4ff18f5 )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c06", 0x180000, 0x80000, 0x260d0370 )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c07", 0x200000, 0x80000, 0x1324d02a )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c08", 0x280000, 0x80000, 0x2de76f10 )
    ROM_LOAD_WIDE_SWAP("ssf2x_j.c09", 0x300000, 0x80000, 0x642fae3f )

    ROM_REGION_DISPOSE(0x1000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ssf2x.g25",    0x0000000, 0x100000, 0x1ee90208 )
    ROM_LOAD( "ssf2x.g18",    0x0100000, 0x100000, 0xf5b1b336 )
    ROM_LOAD( "ssf2x.g17",    0x0200000, 0x200000, 0x59f5267b )
    ROM_LOAD( "ssf2x.g21",    0x0400000, 0x100000, 0xe32854af )
    ROM_LOAD( "ssf2x.g14",    0x0500000, 0x100000, 0xb7cc32e7 )
    ROM_LOAD( "ssf2x.g13",    0x0600000, 0x200000, 0xcf94d275 )
    ROM_LOAD( "ssf2x.g27",    0x0800000, 0x100000, 0xf814400f )
    ROM_LOAD( "ssf2x.g20",    0x0900000, 0x100000, 0x459d5c6b )
    ROM_LOAD( "ssf2x.g19",    0x0a00000, 0x200000, 0x8dc0d86e )
    ROM_LOAD( "ssf2x.g23",    0x0c00000, 0x100000, 0x760f2927 )
    ROM_LOAD( "ssf2x.g16",    0x0d00000, 0x100000, 0x8376ad18 )
    ROM_LOAD( "ssf2x.g15",    0x0e00000, 0x200000, 0x9073a0d4 )


    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "ssf2x.01",         0x00000, 0x08000, 0xb47b8835 )
    ROM_CONTINUE(                0x10000, 0x18000 )
    ROM_LOAD( "ssf2x.02",         0x28000, 0x20000, 0x0022633f )

    ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "ssf2x.s11",   0x000000, 0x200000, 0x9bdbd476 )
    ROM_LOAD( "ssf2x.s12",   0x200000, 0x200000, 0xa05e3aab )
ROM_END

ROM_START( vampj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vamp_j.c03", 0x000000, 0x80000, 0xf55d3722 )
    ROM_LOAD_WIDE_SWAP("vamp_j.c04", 0x080000, 0x80000, 0x4d9c43c4 )
    ROM_LOAD_WIDE_SWAP("vamp_j.c05", 0x100000, 0x80000, 0x6c497e92 )
    ROM_LOAD_WIDE_SWAP("vamp_j.c06", 0x180000, 0x80000, 0xf1bbecb6 )
    ROM_LOAD_WIDE_SWAP("vamp_j.c07", 0x200000, 0x80000, 0x1067ad84 )
    ROM_LOAD_WIDE_SWAP("vamp_j.c08", 0x280000, 0x80000, 0x4b89f41f )
    ROM_LOAD_WIDE_SWAP("vamp_j.c09", 0x300000, 0x80000, 0xfc0a4aac )
    ROM_LOAD_WIDE_SWAP("vamp_j.c10", 0x380000, 0x80000, 0x9270c26b )

    ROM_REGION_DISPOSE(0x1400000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vamp.g18",   0x0000000, 0x100000, 0x3a033625 )
    ROM_LOAD( "vamp.g17",   0x0100000, 0x400000, 0x4f2408e0 )
    ROM_LOAD( "vamp.g14",   0x0500000, 0x100000, 0xbd87243c )
    ROM_LOAD( "vamp.g13",   0x0600000, 0x400000, 0xc51baf99 )
    ROM_LOAD( "vamp.g20",   0x0a00000, 0x100000, 0x2bff6a89 )
    ROM_LOAD( "vamp.g19",   0x0b00000, 0x400000, 0x9ff60250 )
    ROM_LOAD( "vamp.g16",   0x0f00000, 0x100000, 0xafec855f )
    ROM_LOAD( "vamp.g15",   0x1000000, 0x400000, 0xe87837ad )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vamp.01",         0x00000, 0x08000, 0x64b685d5 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vamp.02",         0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "vamp.s11",   0x000000, 0x200000, 0x4a39deb2 )
    ROM_LOAD( "vamp.s12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( dstlka )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("dstlk_a.c03", 0x000000, 0x80000, 0x294e0bec )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c04", 0x080000, 0x80000, 0xbc18e128 )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c05", 0x100000, 0x80000, 0xe709fa59 )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c06", 0x180000, 0x80000, 0x55e4d387 )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c07", 0x200000, 0x80000, 0x24e8f981 )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c08", 0x280000, 0x80000, 0x743f3a8e )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c09", 0x300000, 0x80000, 0x67fa5573 )
    ROM_LOAD_WIDE_SWAP("dstlk_a.c10", 0x380000, 0x80000, 0x5e03d747 )

    ROM_REGION_DISPOSE(0x1400000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vamp.g18",    0x0000000, 0x100000, 0x3a033625 )
    ROM_LOAD( "vamp.g17",    0x0100000, 0x400000, 0x4f2408e0 )
    ROM_LOAD( "vamp.g14",    0x0500000, 0x100000, 0xbd87243c )
    ROM_LOAD( "vamp.g13",    0x0600000, 0x400000, 0xc51baf99 )
    ROM_LOAD( "vamp.g20",    0x0a00000, 0x100000, 0x2bff6a89 )
    ROM_LOAD( "vamp.g19",    0x0b00000, 0x400000, 0x9ff60250 )
    ROM_LOAD( "vamp.g16",    0x0f00000, 0x100000, 0xafec855f )
    ROM_LOAD( "dstlk_a.g15", 0x1000000, 0x400000, 0x3ce83c77 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vamp.01",         0x00000, 0x08000, 0x64b685d5 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vamp.02",         0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "vamp.s11",   0x000000, 0x200000, 0x4a39deb2 )
    ROM_LOAD( "vamp.s12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( vhuntj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vhunt_j.c03", 0x000000, 0x80000, 0x679c3fa9 )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c04", 0x080000, 0x80000, 0xeb6e71e4 )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c05", 0x100000, 0x80000, 0xeaf634ea )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c06", 0x180000, 0x80000, 0xb70cc6be )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c07", 0x200000, 0x80000, 0x46ab907d )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c08", 0x280000, 0x80000, 0x1c00355e )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c09", 0x300000, 0x80000, 0x026e6f82 )
    ROM_LOAD_WIDE_SWAP("vhunt_j.c10", 0x380000, 0x80000, 0xaadfb3ea )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vhunt.g18",   0x0000000, 0x400000, 0x64498eed )
    ROM_LOAD( "vhunt.g17",   0x0400000, 0x400000, 0x4f2408e0 )
    ROM_LOAD( "vhunt.g14",   0x0800000, 0x400000, 0x7a0e1add )
    ROM_LOAD( "vhunt.g13",   0x0c00000, 0x400000, 0xc51baf99 )
    ROM_LOAD( "vhunt.g20",   0x1000000, 0x400000, 0x17f2433f )
    ROM_LOAD( "vhunt.g19",   0x1400000, 0x400000, 0x9ff60250 )
    ROM_LOAD( "vhunt.g16",   0x1800000, 0x400000, 0x2f41ca75 )
    ROM_LOAD( "vhunt.g15",   0x1c00000, 0x400000, 0x3ce83c77 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vhunt.01",         0x00000, 0x08000, 0x5045dcac )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vhunt.02",         0x28000, 0x20000, 0x86b60e59 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "vhunt.s11",   0x000000, 0x200000, 0xe1837d33 )
    ROM_LOAD( "vhunt.s12",   0x200000, 0x200000, 0xfbd3cd90 )
ROM_END

ROM_START( vhunt2j )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c03", 0x000000, 0x80000, 0x1a5feb13 )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c04", 0x080000, 0x80000, 0x434611a5 )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c05", 0x100000, 0x80000, 0xffe3edbc )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c06", 0x180000, 0x80000, 0x6a3b9897 )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c07", 0x200000, 0x80000, 0xb021c029 )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c08", 0x280000, 0x80000, 0xac873dff )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c09", 0x300000, 0x80000, 0xeaefce9c )
    ROM_LOAD_WIDE_SWAP("vhunt2_j.c10", 0x380000, 0x80000, 0x11730952 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vhunt2.g18",   0x0000000, 0x400000, 0xf2f42b38 )
    ROM_LOAD( "vhunt2.g17",   0x0400000, 0x400000, 0x6cfe0141 )
    ROM_LOAD( "vhunt2.g14",   0x0800000, 0x400000, 0xcd09bd63 )
    ROM_LOAD( "vhunt2.g13",   0x0c00000, 0x400000, 0xe3fedff7 )
    ROM_LOAD( "vhunt2.g20",   0x1000000, 0x400000, 0x5b8f22b8 )
    ROM_LOAD( "vhunt2.g19",   0x1400000, 0x400000, 0x30cfe6a9 )
    ROM_LOAD( "vhunt2.g16",   0x1800000, 0x400000, 0x70698843 )
    ROM_LOAD( "vhunt2.g15",   0x1c00000, 0x400000, 0x2dc777f0 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vhunt2.01",         0x00000, 0x08000, 0x67b9f779 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vhunt2.02",         0x28000, 0x20000, 0xaaf15fcb )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "vhunt2.s11",   0x000000, 0x400000, 0x38922efd )
    ROM_LOAD( "vhunt2.s12",   0x400000, 0x400000, 0x6e2430af )
ROM_END


ROM_START( vsavj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vsav_j.c03", 0x000000, 0x80000, 0x2a2e74a4 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c04", 0x080000, 0x80000, 0x1c2427bc )
    ROM_LOAD_WIDE_SWAP("vsav_j.c05", 0x100000, 0x80000, 0x95ce88d5 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c06", 0x180000, 0x80000, 0x2c4297e0 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c07", 0x200000, 0x80000, 0xa38aaae7 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c08", 0x280000, 0x80000, 0x5773e5c9 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c09", 0x300000, 0x80000, 0xd064f8b9 )
    ROM_LOAD_WIDE_SWAP("vsav_j.c10", 0x380000, 0x80000, 0x434518e9 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vsav.g18",   0x0000000, 0x400000, 0xdf9a9f47 )
    ROM_LOAD( "vsav.g17",   0x0400000, 0x400000, 0x6b89445e )
    ROM_LOAD( "vsav.g14",   0x0800000, 0x400000, 0xc1a28e6c )
    ROM_LOAD( "vsav.g13",   0x0c00000, 0x400000, 0xfd8a11eb )
    ROM_LOAD( "vsav.g20",   0x1000000, 0x400000, 0xc22fc3d9 )
    ROM_LOAD( "vsav.g19",   0x1400000, 0x400000, 0x3830fdc7 )
    ROM_LOAD( "vsav.g16",   0x1800000, 0x400000, 0x194a7304 )
    ROM_LOAD( "vsav.g15",   0x1c00000, 0x400000, 0xdd1e7d4e )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vsav.01",         0x00000, 0x08000, 0xf778769b )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "vsav.02",         0x28000, 0x20000, 0xcc09faa1 )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "vsav.s11",   0x000000, 0x400000, 0xe80e956e )
    ROM_LOAD( "vsav.s12",   0x400000, 0x400000, 0x9cd71557 )
ROM_END

ROM_START( vsavu )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
	ROM_LOAD_WIDE_SWAP("vsav_u.c03", 0x000000, 0x80000, 0x1f295274 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c04", 0x080000, 0x80000, 0xc46adf81 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c05", 0x100000, 0x80000, 0x4118e00f )
	ROM_LOAD_WIDE_SWAP("vsav_u.c06", 0x180000, 0x80000, 0x2f4fd3a9 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c07", 0x200000, 0x80000, 0xcbda91b8 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c08", 0x280000, 0x80000, 0x6ca47259 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c09", 0x300000, 0x80000, 0xf4a339e3 )
	ROM_LOAD_WIDE_SWAP("vsav_u.c10", 0x380000, 0x80000, 0xfffbb5b8 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vsav.g18",   0x0000000, 0x400000, 0xdf9a9f47 )
    ROM_LOAD( "vsav.g17",   0x0400000, 0x400000, 0x6b89445e )
    ROM_LOAD( "vsav.g14",   0x0800000, 0x400000, 0xc1a28e6c )
    ROM_LOAD( "vsav.g13",   0x0c00000, 0x400000, 0xfd8a11eb )
    ROM_LOAD( "vsav.g20",   0x1000000, 0x400000, 0xc22fc3d9 )
    ROM_LOAD( "vsav.g19",   0x1400000, 0x400000, 0x3830fdc7 )
    ROM_LOAD( "vsav.g16",   0x1800000, 0x400000, 0x194a7304 )
    ROM_LOAD( "vsav.g15",   0x1c00000, 0x400000, 0xdd1e7d4e )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vsav.01",         0x00000, 0x08000, 0xf778769b )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "vsav.02",         0x28000, 0x20000, 0xcc09faa1 )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "vsav.s11",   0x000000, 0x400000, 0xe80e956e )
    ROM_LOAD( "vsav.s12",   0x400000, 0x400000, 0x9cd71557 )
ROM_END

ROM_START( vsav2j )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
	ROM_LOAD_WIDE_SWAP("vs2j.03", 0x000000, 0x80000, 0x89fd86b4 )
	ROM_LOAD_WIDE_SWAP("vs2j.04", 0x080000, 0x80000, 0x107c091b )
	ROM_LOAD_WIDE_SWAP("vs2j.05", 0x100000, 0x80000, 0x61979638 )
	ROM_LOAD_WIDE_SWAP("vs2j.06", 0x180000, 0x80000, 0xf37c5bc2 )
	ROM_LOAD_WIDE_SWAP("vs2j.07", 0x200000, 0x80000, 0x8f885809 )
	ROM_LOAD_WIDE_SWAP("vs2j.08", 0x280000, 0x80000, 0x2018c120 )
	ROM_LOAD_WIDE_SWAP("vs2j.09", 0x300000, 0x80000, 0xfac3c217 )
	ROM_LOAD_WIDE_SWAP("vs2j.10", 0x380000, 0x80000, 0xeb490213 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vs2.18",   0x0000000, 0x400000, 0x778dc4f6 )
    ROM_LOAD( "vs2.17",   0x0400000, 0x400000, 0x39db59ad )
    ROM_LOAD( "vs2.14",   0x0800000, 0x400000, 0xcd09bd63 )
    ROM_LOAD( "vs2.13",   0x0c00000, 0x400000, 0x5c852f52 )
    ROM_LOAD( "vs2.20",   0x1000000, 0x400000, 0x605d9d1d )
    ROM_LOAD( "vs2.19",   0x1400000, 0x400000, 0x00c763a7 )
    ROM_LOAD( "vs2.16",   0x1800000, 0x400000, 0xe0182c15 )
    ROM_LOAD( "vs2.15",   0x1c00000, 0x400000, 0xa20f58af )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vs2.01",         0x00000, 0x08000, 0x35190139 )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "vs2.02",         0x28000, 0x20000, 0xc32dba09 )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "vs2.11",   0x000000, 0x400000, 0xd67e47b7 )
    ROM_LOAD( "vs2.12",   0x400000, 0x400000, 0x6d020a14 )
ROM_END

ROM_START( xmcotaj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("xmcota_j.c03", 0x000000, 0x80000, 0x0bafeb0e )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c04", 0x080000, 0x80000, 0xc29bdae3 )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c05", 0x100000, 0x80000, 0xac0d7759 )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c06", 0x180000, 0x80000, 0x6a3f0924 )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c07", 0x200000, 0x80000, 0x2c142a44 )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c08", 0x280000, 0x80000, 0xf712d44f )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c09", 0x300000, 0x80000, 0xc24db29a )
    ROM_LOAD_WIDE_SWAP("xmcota_j.c10", 0x380000, 0x80000, 0x53c0eab9 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "xmcota.g18",   0x0000000, 0x400000, 0x015a7c4c )
    ROM_LOAD( "xmcota.g17",   0x0400000, 0x400000, 0x513eea17 )
    ROM_LOAD( "xmcota.g14",   0x0800000, 0x400000, 0x778237b7 )
    ROM_LOAD( "xmcota.g13",   0x0c00000, 0x400000, 0xbf4df073 )
    ROM_LOAD( "xmcota.g20",   0x1000000, 0x400000, 0x9dde2758 )
    ROM_LOAD( "xmcota.g19",   0x1400000, 0x400000, 0xd23897fc )
    ROM_LOAD( "xmcota.g16",   0x1800000, 0x400000, 0x67b36948 )
    ROM_LOAD( "xmcota.g15",   0x1c00000, 0x400000, 0x4d7e4cef )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "xmcota.01",         0x00000, 0x08000, 0x40f479ea )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "xmcota.02",         0x28000, 0x20000, 0x39d9b5ad )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "xmcota.s11",   0x000000, 0x200000, 0xc848a6bc )
    ROM_LOAD( "xmcota.s12",   0x200000, 0x200000, 0x729c188f )
ROM_END

ROM_START( xmvssfj )
    ROM_REGIONX( CODE_SIZE, REGION_CPU1 )      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c03", 0x000000, 0x80000, 0x5481155a )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c04", 0x080000, 0x80000, 0x1e236388 )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c05", 0x100000, 0x80000, 0x7db6025d )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c06", 0x180000, 0x80000, 0xe8e2c75c )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c07", 0x200000, 0x80000, 0x08f0abed )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c08", 0x280000, 0x80000, 0x81929675 )
    ROM_LOAD_WIDE_SWAP("xmvssf_j.c09", 0x300000, 0x80000, 0x9641f36b )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "xmvssf.g18",   0x0000000, 0x400000, 0xb0def86a )
    ROM_LOAD( "xmvssf.g17",   0x0400000, 0x400000, 0x92db3474 )
    ROM_LOAD( "xmvssf.g14",   0x0800000, 0x400000, 0xbcac2e41 )
    ROM_LOAD( "xmvssf.g13",   0x0c00000, 0x400000, 0xf6684efd )
    ROM_LOAD( "xmvssf.g20",   0x1000000, 0x400000, 0x4b40ff9f )
    ROM_LOAD( "xmvssf.g19",   0x1400000, 0x400000, 0x3733473c )
    ROM_LOAD( "xmvssf.g16",   0x1800000, 0x400000, 0xea04a272 )
    ROM_LOAD( "xmvssf.g15",   0x1c00000, 0x400000, 0x29109221 )

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU2 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "xmvssf.01",     0x00000, 0x08000, 0x3999e93a )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "xmvssf.02",     0x28000, 0x20000, 0x19272e4c )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "xmvssf.s11",   0x000000, 0x200000, 0x9cadcdbc )
    ROM_LOAD( "xmvssf.s12",   0x200000, 0x200000, 0x7b11e460 )
ROM_END

CPS2_GAME_DRIVER(armwara,                "Armoured Warriors (Asia 940920)"							, "1994", "Capcom", ROT0)
CPS2_GAME_DRIVER(avspj,                  "Aliens Vs. Predator (Japan 940520)"						, "1994", "Capcom", ROT0)
CPS2_GAME_DRIVER(batcirj,                "Battle Circuit (Japan 970319)"							, "1997", "Capcom", ROT0)
CPS2_GAME_DRIVER(ddtodu,                 "Dungeons & Dragons: Tower of Doom (USA 940125)"				, "1994", "Capcom", ROT0)
CLONE_CPS2_GAME_DRIVER(ddtoda,  ddtodu , "Dungeons & Dragons: Tower of Doom (Asia 940113)"   			, "1994", "Capcom", ROT0)
CPS2_GAME_DRIVER(mshj,                   "Marvel Super Heroes (Japan 951024)"                 			, "1995", "Capcom", ROT0)
CPS2_GAME_DRIVER(mshvsfu,                "Marvel Super Heroes Vs. Street Fighter (USA 970625)"			, "1997", "Capcom", ROT0)
//CPS2_GAME_DRIVER(mvsc,                   "Marvel Super Heroes vs. Capcom: Clash of Super Heroes (??? ??????)"    , "199?", "Capcom", ROT0)
CPS2_GAME_DRIVER(rckman2j,               "Rockman 2: The Power Fighters (Japan 960708)"                 , "1996", "Capcom", ROT0)
CPS2_GAME_DRIVER(sfzj,                   "Street Fighter Zero (Japan, 950727)"						, "1995", "Capcom", ROT0)
CLONE_CPS2_GAME_DRIVER(sfzj1, sfzj,      "Street Fighter Zero (Japan, 950627)"						, "1995", "Capcom", ROT0)
CLONE_CPS2_GAME_DRIVER(sfau, sfzj,       "Street Fighter Alpha: The Warriors Dream (USA, 950727)"			, "1995", "Capcom", ROT0)
CPS2_GAME_DRIVER(sfz2j,                  "Street Fighter Zero 2 (Japan, 960227)"						, "1996", "Capcom", ROT0)
//CPS2_GAME_DRIVER(sfa3,                   "Street Fighter Alpha 3 (??? ??????)"                       , "199?", "Capcom", ROT0)
CPS2_GAME_DRIVER(ssf2j,                  "Super Street Fighter 2: The New Challengers (Japan 930910)"		, "1993", "Capcom", ROT0)
CPS2_GAME_DRIVER(ssf2xj,                 "Super Street Fighter 2X: Grand Master Challenge (Japan 940223)"	, "1994", "Capcom", ROT0)
CPS2_GAME_DRIVER(vampj,                  "Vampire: The Night Warriors (Japan, 940705)"					, "1994", "Capcom", ROT0)
CLONE_CPS2_GAME_DRIVER(dstlka, vampj,    "Dark Stalkers (Asia, 940705)"							, "1994", "Capcom", ROT0)
CPS2_GAME_DRIVER(vhuntj,                 "Vampire Hunter: Darkstalkers 2 (Japan, 950302)"				, "1995", "Capcom", ROT0)
CPS2_GAME_DRIVER(vhunt2j,                "Vampire Hunter 2: Darkstalkers Revenge (Japan, 970828)"			, "1997", "Capcom", ROT0)
CPS2_GAME_DRIVER(vsavj,                  "Vampire Savior: The Lord of Vampire (Japan, 970519)"			, "1997", "Capcom", ROT0)
CLONE_CPS2_GAME_DRIVER(vsavu, vsavj,     "Vampire Savior: Jedah's Damnation (USA 970519)"				, "1997", "Capcom", ROT0)
CPS2_GAME_DRIVER(vsav2j,                 "Vampire Savior 2: The Lord of Vampire (Japan 97????)"            , "1997", "Capcom", ROT0)
CPS2_GAME_DRIVER(xmcotaj,                "X-Men: Children of the Atom (Japan, 950105)"					, "1995", "Capcom", ROT0)
CPS2_GAME_DRIVER(xmvssfj,                "X-Men Vs. Street Fighter (Japan, 961004)"					, "1996", "Capcom", ROT0)

/***************************************************************************

  Capcom System PSX?
  ==================

    Playstation Hardware with Q sound.

    Why bother adding this? Well, we might be able to get it to play
    the Q sound.

***************************************************************************/

#define CPSX_GAME_DRIVER(NAME,DESCRIPTION,YEAR,MANUFACTURER,ORIENTATION) \
struct GameDriver driver_##NAME =  \
{                                  \
	__FILE__,                      \
	0,                             \
	#NAME,                         \
	DESCRIPTION,                   \
	YEAR,                          \
	MANUFACTURER,                  \
    CPS2_CREDITS,                  \
	0,                             \
	&machine_driver_##NAME,        \
	0,                             \
	rom_##NAME,                    \
    0, 0,                          \
	0,                             \
	0,                             \
	input_ports_##NAME,            \
	0, 0, 0,                       \
	ORIENTATION,                   \
	0, 0                           \
};


#define CPSX_MACHINE_DRIVER(CPS1_DRVNAME, CPS1_CPU_FRQ) \
static struct MachineDriver machine_driver_##CPS1_DRVNAME =            \
{                                                                        \
	/* basic machine hardware */                                     \
	{                                                                \
		{                                                        \
			CPU_Z80 | CPU_AUDIO_CPU,                         \
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */    \
			qsound_readmem,qsound_writemem,0,0,              \
			interrupt,1                               \
	}                                                        \
	},                                                               \
    60, 0,                   \
	1,                                                               \
	0,                                                               \
									 \
	/* video hardware */                                             \
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 }, \
									 \
	cps1_gfxdecodeinfo,                                              \
	32*16+32*16+32*16+32*16,   /* lotsa colours */                   \
	32*16+32*16+32*16+32*16,   /* Colour table length */             \
	0,                                                               \
									 \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                      \
	0,                                                               \
	cps1_vh_start,                                                   \
	cps1_vh_stop,                                                    \
	cps1_vh_screenrefresh,                                           \
									 \
	/* sound hardware */                                             \
	SOUND_SUPPORTS_STEREO,0,0,0,   \
    { { SOUND_CUSTOM, &qsound_interface } } \
};

#define input_ports_sfex    input_ports_sfa

CPSX_MACHINE_DRIVER( sfex,       CPS2_DEFAULT_CPU_SPEED )

ROM_START( sfex )
    ROM_REGION( CODE_SIZE )      /* PSX code */
    ROM_LOAD("sfe-04",  0x000000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-05m", 0x080000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-06m", 0x100000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-07m", 0x180000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-08m", 0x200000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-09m", 0x280000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-10m", 0x300000, 0x80000, 0xffffffff )

    ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */

    ROM_REGIONX( QSOUND_SIZE, REGION_CPU1 ) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfe-02",         0x00000, 0x08000, 0xffffffff )
    ROM_CONTINUE(               0x10000, 0x18000 )
    ROM_LOAD( "sfe-03",         0x28000, 0x20000, 0xffffffff )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfe-01m",        0x000000, 0x400000, 0xffffffff )
ROM_END

CPSX_GAME_DRIVER(sfex,    "Street Fighter EX",                "199?","Capcom",ROT0)





