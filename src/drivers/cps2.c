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
struct GameDriver NAME##_driver =  \
{                                  \
	__FILE__,                      \
    0,                             \
	#NAME,                         \
	DESCRIPTION,                   \
	YEAR,                          \
	MANUFACTURER,                  \
    CPS2_CREDITS,                  \
	0,                             \
	&NAME##_machine_driver,        \
	0,                             \
	NAME##_rom,                    \
    cps2_decode, 0,                \
	0,                             \
	0,                             \
	NAME##_input_ports,            \
	0, 0, 0,                       \
	ORIENTATION,                   \
	0, 0                           \
};

/* Maximum size of Q Sound Z80 region */
#define QSOUND_SIZE 0x50000

/* Maximum 680000 code size */
#undef  CODE_SIZE
#define CODE_SIZE   0x0800000

#define CPS2_CREDITS            CPS1_CREDITS
#define CPS2_DEFAULT_CPU_SPEED  CPS1_DEFAULT_CPU_SPEED

void cps2_decode(void)
{
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    FILE *fp;
    int i;
    const int decode=CODE_SIZE/2;

    fp = fopen ("ROM.DMP", "w+b");
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
static struct MachineDriver CPS1_DRVNAME##_machine_driver =            \
{                                                                        \
	/* basic machine hardware */                                     \
	{                                                                \
		{                                                        \
			CPU_M68000,                                      \
			CPS1_CPU_FRQ,                                    \
			0,                                               \
			cps1_readmem,cps1_writemem,0,0,                  \
			cps1_qsound_interrupt, 1  /* ??? interrupts per frame */   \
		},                                                       \
		{                                                        \
			CPU_Z80 | CPU_AUDIO_CPU,                         \
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */    \
			2,      /* memory region #2 */                   \
			qsound_readmem,qsound_writemem,0,0,              \
			interrupt,1                               \
	}                                                        \
	},                                                               \
    60, 0,                     \
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
	cps2_vh_start,                                                   \
	cps2_vh_stop,                                                    \
	cps2_vh_screenrefresh,                                           \
									 \
	/* sound hardware */                                             \
	SOUND_SUPPORTS_STEREO,0,0,0,   \
	{ { SOUND_CUSTOM, &custom_interface } } \
};

INPUT_PORTS_START( sfa_input_ports )
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

#define ssf2j_input_ports       sfa_input_ports
#define sfz_input_ports         sfa_input_ports
#define ssf2_input_ports        sfa_input_ports
#define dadtod_input_ports      sfa_input_ports
#define avsp_input_ports        sfa_input_ports
#define vmj_input_ports         sfa_input_ports
#define vphj_input_ports        sfa_input_ports
#define msh_input_ports         sfa_input_ports
#define xmencota_input_ports    sfa_input_ports
#define vsavior_input_ports     sfa_input_ports
#define xmnvssf_input_ports     sfa_input_ports


CPS2_MACHINE_DRIVER( ssf2j,     CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( sfa,       CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( sfz,       CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( dadtod,    CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( avsp,      CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vmj,       CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vphj,      CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( msh,       CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( xmencota,  CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( vsavior,   CPS2_DEFAULT_CPU_SPEED )
CPS2_MACHINE_DRIVER( xmnvssf,   CPS2_DEFAULT_CPU_SPEED )

ROM_START( ssf2j_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("ssfj03.bin", 0x000000, 0x80000, 0x7eb0efed )
    ROM_LOAD_WIDE_SWAP("ssfj04.bin", 0x080000, 0x80000, 0xd7322164 )
    ROM_LOAD_WIDE_SWAP("ssfj05.bin", 0x100000, 0x80000, 0x0918d19a )
    ROM_LOAD_WIDE_SWAP("ssfj06.bin", 0x180000, 0x80000, 0xd808a6cd )
    ROM_LOAD_WIDE_SWAP("ssfj07.bin", 0x200000, 0x80000, 0xeb6a9b1b )

    ROM_REGION_DISPOSE(0xc00000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ssf_c18.bin",   0x000000, 0x100000, 0xf5b1b336 )
    ROM_LOAD( "ssf_c17.bin",   0x100000, 0x200000, 0x59f5267b )
    ROM_LOAD( "ssf_c14.bin",   0x300000, 0x100000, 0xb7cc32e7 )
    ROM_LOAD( "ssf_c13.bin",   0x400000, 0x200000, 0xcf94d275 )
    ROM_LOAD( "ssf_c20.bin",   0x600000, 0x100000, 0x459d5c6b )
    ROM_LOAD( "ssf_c19.bin",   0x700000, 0x200000, 0x8dc0d86e )
    ROM_LOAD( "ssf_c16.bin",   0x900000, 0x100000, 0x8376ad18 )
    ROM_LOAD( "ssf_c15.bin",   0xa00000, 0x200000, 0x9073a0d4 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "ssfj01.bin",    0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "ssf-q1.bin",   0x000000, 0x080000, 0xa6f9da5c )
    ROM_LOAD( "ssf-q2.bin",   0x080000, 0x080000, 0x8c66ae26 )
    ROM_LOAD( "ssf-q3.bin",   0x100000, 0x080000, 0x695cc2ca )
    ROM_LOAD( "ssf-q4.bin",   0x180000, 0x080000, 0x9d9ebe32 )
    ROM_LOAD( "ssf-q5.bin",   0x200000, 0x080000, 0x4770e7b7 )
    ROM_LOAD( "ssf-q6.bin",   0x280000, 0x080000, 0x4e79c951 )
    ROM_LOAD( "ssf-q7.bin",   0x300000, 0x080000, 0xcdd14313 )
    ROM_LOAD( "ssf-q8.bin",   0x380000, 0x080000, 0x6f5a088c )
ROM_END


ROM_START( sfa_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfze_03d.rom", 0x000000, 0x80000, 0xebf2054d )
    ROM_LOAD_WIDE_SWAP("sfze_04c.rom", 0x080000, 0x80000, 0x8b73b0e5 )
    ROM_LOAD_WIDE_SWAP("sfze_05b.rom", 0x100000, 0x80000, 0x0810544d )
    ROM_LOAD_WIDE_SWAP("sfze_06.rom",  0x180000, 0x80000, 0x806e8f38 )

    ROM_REGION_DISPOSE(0x800000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sfz_18-a.rom",   0x000000, 0x80000, 0xf6a58dba )
    ROM_LOAD( "sfz_18-b.rom",   0x080000, 0x80000, 0x6d3593f3 )
    ROM_LOAD( "sfz_18-c.rom",   0x100000, 0x80000, 0x02c4634e )
    ROM_LOAD( "sfz_18-d.rom",   0x180000, 0x80000, 0x853dcd76 )
    ROM_LOAD( "sfz_14-a.rom",   0x200000, 0x80000, 0xa0f938dd )
    ROM_LOAD( "sfz_14-b.rom",   0x280000, 0x80000, 0x8ac60d6b )
    ROM_LOAD( "sfz_14-c.rom",   0x300000, 0x80000, 0xd0e7d272 )
    ROM_LOAD( "sfz_14-d.rom",   0x380000, 0x80000, 0xd744f977 )
    ROM_LOAD( "sfz_20-a.rom",   0x400000, 0x80000, 0x9ea0bba5 )
    ROM_LOAD( "sfz_20-b.rom",   0x480000, 0x80000, 0x1d6eb513 )
    ROM_LOAD( "sfz_20-c.rom",   0x500000, 0x80000, 0xcdf299c1 )
    ROM_LOAD( "sfz_20-d.rom",   0x580000, 0x80000, 0x720567c9 )
    ROM_LOAD( "sfz_16-a.rom",   0x600000, 0x80000, 0x2576b7fb )
    ROM_LOAD( "sfz_16-b.rom",   0x680000, 0x80000, 0xf89961f1 )
    ROM_LOAD( "sfz_16-c.rom",   0x700000, 0x80000, 0xd37ae970 )
    ROM_LOAD( "sfz_16-d.rom",   0x780000, 0x80000, 0xf866764d )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz_01.rom",    0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "sfz_02.rom",    0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfz_11-a.rom",   0x000000, 0x80000, 0x5aef5091 )
    ROM_LOAD( "sfz_11-b.rom",   0x080000, 0x80000, 0x1a7b31a3 )
    ROM_LOAD( "sfz_11-c.rom",   0x100000, 0x80000, 0xf4540c35 )
    ROM_LOAD( "sfz_11-d.rom",   0x180000, 0x80000, 0x3a88e8da )
    ROM_LOAD( "sfz_12-a.rom",   0x200000, 0x80000, 0x207b8d90 )
    ROM_LOAD( "sfz_12-b.rom",   0x280000, 0x80000, 0xd2958f9b )
    ROM_LOAD( "sfz_12-c.rom",   0x300000, 0x80000, 0x8673691e )
    ROM_LOAD( "sfz_12-d.rom",   0x380000, 0x80000, 0x545f3cc3 )
ROM_END

ROM_START( sfz_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("sfzjp03d.bin", 0x000000, 0x80000, 0xf5444120 )
    ROM_LOAD_WIDE_SWAP("sfzjp04c.bin", 0x080000, 0x80000, 0x8b73b0e5 )
    ROM_LOAD_WIDE_SWAP("sfzjp05b.bin", 0x100000, 0x80000, 0x0810544d )
    ROM_LOAD_WIDE_SWAP("sfzjp06.bin",  0x180000, 0x80000, 0x806e8f38 )

    ROM_REGION_DISPOSE(0x800000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sfzj_c18.bin",   0x000000, 0x200000, 0x41a1e790 )
    ROM_LOAD( "sfzj_c14.bin",   0x200000, 0x200000, 0x90fefdb3 )
    ROM_LOAD( "sfzj_c20.bin",   0x400000, 0x200000, 0xa549df98 )
    ROM_LOAD( "sfzj_c16.bin",   0x600000, 0x200000, 0x5354c948 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfz_01.bin",    0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "sfz_02.bin",    0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfzj_s11.bin",   0x000000, 0x200000, 0xc4b093cd )
    ROM_LOAD( "sfzj_s12.bin",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( dadtod_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("dadu-03b.bin", 0x000000, 0x80000, 0xa519905f )
    ROM_LOAD_WIDE_SWAP("dadu-04b.bin", 0x080000, 0x80000, 0x52562d38 )
    ROM_LOAD_WIDE_SWAP("dadu-05b.bin", 0x100000, 0x80000, 0xee1cfbfe )
    ROM_LOAD_WIDE_SWAP("dadu-06.bin",  0x180000, 0x80000, 0x13aa3e56 )
    ROM_LOAD_WIDE_SWAP("dadu-07.bin",  0x200000, 0x80000, 0x431cb6dd )

    ROM_REGION_DISPOSE(0x1000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "dad-18m.bin",    0x000000, 0x200000, 0x12261f05 )
    ROM_LOAD( "dad-17m.bin",    0x200000, 0x200000, 0xb98757f5 )
    ROM_LOAD( "dad-14m.bin",    0x400000, 0x200000, 0x53901447 )
    ROM_LOAD( "dad-13m.bin",    0x600000, 0x200000, 0xda3cb7d6 )
    ROM_LOAD( "dad-20m.bin",    0x800000, 0x200000, 0x4d170077 )
    ROM_LOAD( "dad-19m.bin",    0xa00000, 0x200000, 0x8121ce46 )
    ROM_LOAD( "dad-16m.bin",    0xc00000, 0x200000, 0x86e149fd )
    ROM_LOAD( "dad-15m.bin",    0xe00000, 0x200000, 0x92b63172 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "dad-01.bin",    0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "dad-11m.bin",   0x000000, 0x200000, 0x0c499b67 )
    ROM_LOAD( "dad-12m.bin",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( avsp_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("avsp.3", 0x000000, 0x80000, 0x42757950 )
    ROM_LOAD_WIDE_SWAP("avsp.4", 0x080000, 0x80000, 0x5abcdee6 )
    ROM_LOAD_WIDE_SWAP("avsp.5", 0x100000, 0x80000, 0xfbfb5d7a )
    ROM_LOAD_WIDE_SWAP("avsp.6", 0x180000, 0x80000, 0x190b817f )

    ROM_REGION_DISPOSE(0x1000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "avsp.18",    0x000000, 0x200000, 0xd92b6fc0 )
    ROM_LOAD( "avsp.17",    0x200000, 0x200000, 0x94403195 )
    ROM_LOAD( "avsp.14",    0x400000, 0x200000, 0xebba093e )
    ROM_LOAD( "avsp.13",    0x600000, 0x200000, 0x8f8b5ae4 )
    ROM_LOAD( "avsp.20",    0x800000, 0x200000, 0xf90baa21 )
    ROM_LOAD( "avsp.19",    0xa00000, 0x200000, 0xe1981245 )
    ROM_LOAD( "avsp.16",    0xc00000, 0x200000, 0xfb228297 )
    ROM_LOAD( "avsp.15",    0xe00000, 0x200000, 0xb00280df )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "avsp.1",         0x00000, 0x08000, 0x2d3b4220 )
    ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "avsp.11",   0x000000, 0x200000, 0x83499817 )
    ROM_LOAD( "avsp.12",   0x200000, 0x200000, 0xf4110d49 )
ROM_END

ROM_START( vmj_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vmj_03a.bin", 0x000000, 0x80000, 0xf55d3722 )
    ROM_LOAD_WIDE_SWAP("vmj_04b.bin", 0x080000, 0x80000, 0x4d9c43c4 )
    ROM_LOAD_WIDE_SWAP("vmj_05a.bin", 0x100000, 0x80000, 0x6c497e92 )
    ROM_LOAD_WIDE_SWAP("vmj_06a.bin", 0x180000, 0x80000, 0xf1bbecb6 )
    ROM_LOAD_WIDE_SWAP("vmj_07a.bin", 0x200000, 0x80000, 0x1067ad84 )
    ROM_LOAD_WIDE_SWAP("vmj_08a.bin", 0x280000, 0x80000, 0x4b89f41f )
    ROM_LOAD_WIDE_SWAP("vmj_09a.bin", 0x300000, 0x80000, 0xfc0a4aac )
    ROM_LOAD_WIDE_SWAP("vmj_10a.bin", 0x380000, 0x80000, 0x9270c26b )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vm_c18.bin",   0x0000000, 0x100000, 0x3a033625 ) /* Suspect ! */
    ROM_LOAD( "vm_c17.bin",   0x0100000, 0x400000, 0x4f2408e0 )
    ROM_LOAD( "vm_c14.bin",   0x0800000, 0x100000, 0xbd87243c ) /* Suspect ! */
    ROM_LOAD( "vm_c13.bin",   0x0900000, 0x400000, 0xc51baf99 )
    ROM_LOAD( "vm_c20.bin",   0x1000000, 0x100000, 0x2bff6a89 ) /* Suspect ! */
    ROM_LOAD( "vm_c19.bin",   0x1100000, 0x400000, 0x9ff60250 )
    ROM_LOAD( "vm_c16.bin",   0x1800000, 0x100000, 0xafec855f ) /* Suspect ! */
    ROM_LOAD( "vm_c15.bin",   0x1900000, 0x400000, 0xe87837ad )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vm_01.bin",         0x00000, 0x08000, 0x64b685d5 )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vm_02.bin",         0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "vm_s11.bin",   0x000000, 0x200000, 0x4a39deb2 )
    ROM_LOAD( "vm_s12.bin",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END


ROM_START( vphj_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vphjp03b.bin", 0x000000, 0x80000, 0x679c3fa9 )
    ROM_LOAD_WIDE_SWAP("vphjp04a.bin", 0x080000, 0x80000, 0xeb6e71e4 )
    ROM_LOAD_WIDE_SWAP("vphjp05a.bin", 0x100000, 0x80000, 0xeaf634ea )
    ROM_LOAD_WIDE_SWAP("vphjp06a.bin", 0x180000, 0x80000, 0xb70cc6be )
    ROM_LOAD_WIDE_SWAP("vphjp07a.bin", 0x200000, 0x80000, 0x46ab907d )
    ROM_LOAD_WIDE_SWAP("vphjp08a.bin", 0x280000, 0x80000, 0x1c00355e )
    ROM_LOAD_WIDE_SWAP("vphjp09a.bin", 0x300000, 0x80000, 0x026e6f82 )
    ROM_LOAD_WIDE_SWAP("vphjp10a.bin", 0x380000, 0x80000, 0xaadfb3ea )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vphj_c18.bin",   0x0000000, 0x400000, 0x64498eed )
    ROM_LOAD( "vphj_c17.bin",   0x0400000, 0x400000, 0x4f2408e0 )
    ROM_LOAD( "vphj_c14.bin",   0x0800000, 0x400000, 0x7a0e1add )
    ROM_LOAD( "vphj_c13.bin",   0x0c00000, 0x400000, 0xc51baf99 )
    ROM_LOAD( "vphj_c20.bin",   0x1000000, 0x400000, 0x17f2433f )
    ROM_LOAD( "vphj_c19.bin",   0x1400000, 0x400000, 0x9ff60250 )
    ROM_LOAD( "vphj_c16.bin",   0x1800000, 0x400000, 0x2f41ca75 )
    ROM_LOAD( "vphj_c15.bin",   0x1c00000, 0x400000, 0x3ce83c77 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vph_01.bin",         0x00000, 0x08000, 0x5045dcac )
    ROM_CONTINUE(                  0x10000, 0x18000 )
    ROM_LOAD( "vph_02.bin",         0x28000, 0x20000, 0x86b60e59 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "vphj_s11.bin",   0x000000, 0x200000, 0xe1837d33 )
    ROM_LOAD( "vphj_s12.bin",   0x200000, 0x200000, 0xfbd3cd90 )
ROM_END

ROM_START( msh_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("msh.3", 0x000000, 0x80000, 0xd2805bdd )
    ROM_LOAD_WIDE_SWAP("msh.4", 0x080000, 0x80000, 0x743f96ff )
    ROM_LOAD_WIDE_SWAP("msh.5", 0x100000, 0x80000, 0x6a091b9e )
    ROM_LOAD_WIDE_SWAP("msh.6", 0x180000, 0x80000, 0x803e3fa4 )
    ROM_LOAD_WIDE_SWAP("msh.7", 0x200000, 0x80000, 0xc45f8e27 )
    ROM_LOAD_WIDE_SWAP("msh.8", 0x280000, 0x80000, 0x9ca6f12c )
    ROM_LOAD_WIDE_SWAP("msh.9", 0x300000, 0x80000, 0x82ec27af )
    ROM_LOAD_WIDE_SWAP("msh.10",0x380000, 0x80000, 0x8d931196 )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "msh.18a",   0x0000000, 0x200000, 0xb51babdb )
    ROM_LOAD( "msh.18b",   0x0200000, 0x200000, 0xaff590b2 )
    ROM_LOAD( "msh.17a",   0x0400000, 0x200000, 0x244ffd8a )
    ROM_LOAD( "msh.17b",   0x0600000, 0x200000, 0x86043e72 )
    ROM_LOAD( "msh.14a",   0x0800000, 0x200000, 0x386d82ef )
    ROM_LOAD( "msh.14b",   0x0a00000, 0x200000, 0xf667ac27 )
    ROM_LOAD( "msh.13a",   0x0c00000, 0x200000, 0xfccb4ab6 )
    ROM_LOAD( "msh.13b",   0x0e00000, 0x200000, 0xd152f470 )
    ROM_LOAD( "msh.20a",   0x1000000, 0x200000, 0x00799354 )
    ROM_LOAD( "msh.20b",   0x1200000, 0x200000, 0xd6162c92 )
    ROM_LOAD( "msh.19a",   0x1400000, 0x200000, 0xbf22b269 )
    ROM_LOAD( "msh.19b",   0x1600000, 0x200000, 0x607698e8 )
    ROM_LOAD( "msh.16a",   0x1800000, 0x200000, 0xd3dde9fb )
    ROM_LOAD( "msh.16b",   0x1a00000, 0x200000, 0x6af59ee6 )
    ROM_LOAD( "msh.15a",   0x1c00000, 0x200000, 0x6abc0066 )
    ROM_LOAD( "msh.15b",   0x1e00000, 0x200000, 0xd6e19b82 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "msh.1",         0x00000, 0x08000, 0xc976e6f9 )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "msh.2",         0x28000, 0x20000, 0xce67d0d9 )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "msh.11",   0x000000, 0x200000, 0x37ac6d30 )
    ROM_LOAD( "msh.12",   0x200000, 0x200000, 0xde092570 )
ROM_END


ROM_START( xmencota_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("xcota.3", 0x000000, 0x80000, 0x0bafeb0e )
    ROM_LOAD_WIDE_SWAP("xcota.4", 0x080000, 0x80000, 0xc29bdae3 )
    ROM_LOAD_WIDE_SWAP("xcota.5", 0x100000, 0x80000, 0xac0d7759 )
    ROM_LOAD_WIDE_SWAP("xcota.6", 0x180000, 0x80000, 0x6a3f0924 )
    ROM_LOAD_WIDE_SWAP("xcota.7", 0x200000, 0x80000, 0x2c142a44 )
    ROM_LOAD_WIDE_SWAP("xcota.8", 0x280000, 0x80000, 0xf712d44f )
    ROM_LOAD_WIDE_SWAP("xcota.9", 0x300000, 0x80000, 0xc24db29a )
    ROM_LOAD_WIDE_SWAP("xcota.10",0x380000, 0x80000, 0x53c0eab9 )
    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "xcota.18a",   0x0000000, 0x200000, 0x588c8430 )
    ROM_LOAD( "xcota.18b",   0x0200000, 0x200000, 0xb2b83706 )
    ROM_LOAD( "xcota.17a",   0x0400000, 0x200000, 0x1588c5a6 )
    ROM_LOAD( "xcota.17b",   0x0600000, 0x200000, 0x3120e24b )
    ROM_LOAD( "xcota.14a",   0x0800000, 0x200000, 0x386d82ef )
    ROM_LOAD( "xcota.14b",   0x0a00000, 0x200000, 0x08f5fad7 )
    ROM_LOAD( "xcota.13a",   0x0c00000, 0x200000, 0xaea3ca33 )
    ROM_LOAD( "xcota.13b",   0x0e00000, 0x200000, 0xf55c06bc )
    ROM_LOAD( "xcota.20a",   0x1000000, 0x200000, 0x3aaa3e35 )
    ROM_LOAD( "xcota.20b",   0x1200000, 0x200000, 0xde7d63aa )
    ROM_LOAD( "xcota.19a",   0x1400000, 0x200000, 0x6a2947c4 )
    ROM_LOAD( "xcota.19b",   0x1600000, 0x200000, 0xc72530f2 )
    ROM_LOAD( "xcota.16a",   0x1800000, 0x200000, 0x717e1946 )
    ROM_LOAD( "xcota.16b",   0x1a00000, 0x200000, 0xd7dc9f6d )
    ROM_LOAD( "xcota.15a",   0x1c00000, 0x200000, 0xa7522a23 )
    ROM_LOAD( "xcota.15b",   0x1e00000, 0x200000, 0xea850e16 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "xcota.1",         0x00000, 0x08000, 0x40f479ea )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "xcota.2",         0x28000, 0x20000, 0x39d9b5ad )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "xcota.11",   0x000000, 0x200000, 0xc848a6bc )
    ROM_LOAD( "xcota.12",   0x200000, 0x200000, 0x729c188f )
ROM_END

ROM_START( vsavior_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("vsavior.3", 0x000000, 0x80000, 0x1f295274 )
    ROM_LOAD_WIDE_SWAP("vsavior.4", 0x080000, 0x80000, 0xc46adf81 )
    ROM_LOAD_WIDE_SWAP("vsavior.5", 0x100000, 0x80000, 0x4118e00f )
    ROM_LOAD_WIDE_SWAP("vsavior.6", 0x180000, 0x80000, 0x2f4fd3a9 )
    ROM_LOAD_WIDE_SWAP("vsavior.7", 0x200000, 0x80000, 0xcbda91b8 )
    ROM_LOAD_WIDE_SWAP("vsavior.8", 0x280000, 0x80000, 0x6ca47259 )
    ROM_LOAD_WIDE_SWAP("vsavior.9", 0x300000, 0x80000, 0xf4a339e3 )
    ROM_LOAD_WIDE_SWAP("vsavior.10",0x380000, 0x80000, 0xfffbb5b8 )



    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "vsavior.18a",   0x0000000, 0x200000, 0xc8ff7412 )
    ROM_LOAD( "vsavior.18b",   0x0200000, 0x200000, 0x9c52cd35 )
    ROM_LOAD( "vsavior.17a",   0x0400000, 0x200000, 0x3b087120 )
    ROM_LOAD( "vsavior.17b",   0x0600000, 0x200000, 0x5660f246 )
    ROM_LOAD( "vsavior.14a",   0x0800000, 0x200000, 0x168ac792 )
    ROM_LOAD( "vsavior.14b",   0x0a00000, 0x200000, 0xeff8982e )
    ROM_LOAD( "vsavior.13a",   0x0c00000, 0x200000, 0x481706f7 )
    ROM_LOAD( "vsavior.13b",   0x0e00000, 0x200000, 0x1b308c3d )
    ROM_LOAD( "vsavior.20a",   0x1000000, 0x200000, 0x2bbf79df )
    ROM_LOAD( "vsavior.20b",   0x1200000, 0x200000, 0xdc9e9825 )
    ROM_LOAD( "vsavior.19a",   0x1400000, 0x200000, 0x81272672 )
    ROM_LOAD( "vsavior.19b",   0x1600000, 0x200000, 0x392303ce )
    ROM_LOAD( "vsavior.16a",   0x1800000, 0x200000, 0xcf1cb09f )
    ROM_LOAD( "vsavior.16b",   0x1a00000, 0x200000, 0xa3114989 )
    ROM_LOAD( "vsavior.15a",   0x1c00000, 0x200000, 0x11445de5 )
    ROM_LOAD( "vsavior.15b",   0x1e00000, 0x200000, 0x32c3be67 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "vsavior.1",         0x00000, 0x08000, 0xf778769b )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "vsavior.2",         0x28000, 0x20000, 0xcc09faa1 )

	ROM_REGION(0x800000) /* Q Sound Samples */
    ROM_LOAD( "vsavior.11a",   0x000000, 0x200000, 0xf2b92cff )
	ROM_LOAD( "vsavior.11b",   0x200000, 0x200000, 0x7927e272 )
    ROM_LOAD( "vsavior.12a",   0x400000, 0x200000, 0x51564e82 )
	ROM_LOAD( "vsavior.12b",   0x600000, 0x200000, 0xc80293e6 )
ROM_END

ROM_START( xmnvssf_rom )
    ROM_REGION(CODE_SIZE)      /* 68000 code */
    ROM_LOAD_WIDE_SWAP("xmnvssf.3", 0x000000, 0x80000, 0x5481155a )
    ROM_LOAD_WIDE_SWAP("xmnvssf.4", 0x080000, 0x80000, 0x1e236388 )
    ROM_LOAD_WIDE_SWAP("xmnvssf.5", 0x100000, 0x80000, 0x7db6025d )
    ROM_LOAD_WIDE_SWAP("xmnvssf.6", 0x180000, 0x80000, 0xe8e2c75c )
    ROM_LOAD_WIDE_SWAP("xmnvssf.7", 0x200000, 0x80000, 0x08f0abed )
    ROM_LOAD_WIDE_SWAP("xmnvssf.8", 0x280000, 0x80000, 0x81929675 )
    ROM_LOAD_WIDE_SWAP("xmnvssf.9", 0x300000, 0x80000, 0x9641f36b )

    ROM_REGION_DISPOSE(0x2000000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "xmnvssf.18a",   0x0000000, 0x200000, 0x22d21e78 )
    ROM_LOAD( "xmnvssf.18b",   0x0200000, 0x200000, 0x3182444a )
    ROM_LOAD( "xmnvssf.17a",   0x0400000, 0x200000, 0x8739c14e )
    ROM_LOAD( "xmnvssf.17b",   0x0600000, 0x200000, 0xcbd07bdc )
    ROM_LOAD( "xmnvssf.14a",   0x0800000, 0x200000, 0xf24e050e )
    ROM_LOAD( "xmnvssf.14b",   0x0a00000, 0x200000, 0xf1599e96 )
    ROM_LOAD( "xmnvssf.13a",   0x0c00000, 0x200000, 0x2c5c9cfe )
    ROM_LOAD( "xmnvssf.13b",   0x0e00000, 0x200000, 0xbec956c2 )
    ROM_LOAD( "xmnvssf.20a",   0x1000000, 0x200000, 0x2a383279 )
    ROM_LOAD( "xmnvssf.20b",   0x1200000, 0x200000, 0xa21ae637 )
    ROM_LOAD( "xmnvssf.19a",   0x1400000, 0x200000, 0x981a7186 )
    ROM_LOAD( "xmnvssf.19b",   0x1600000, 0x200000, 0xbd48dd36 )
    ROM_LOAD( "xmnvssf.16a",   0x1800000, 0x200000, 0xcad6a62f )
    ROM_LOAD( "xmnvssf.16b",   0x1a00000, 0x200000, 0xfd415cab )
    ROM_LOAD( "xmnvssf.15a",   0x1c00000, 0x200000, 0x9b6dd145 )
    ROM_LOAD( "xmnvssf.15b",   0x1e00000, 0x200000, 0xe33cc525 )

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "xmnvssf.1",     0x00000, 0x08000, 0x3999e93a )
    ROM_CONTINUE(              0x10000, 0x18000 )
    ROM_LOAD( "xmnvssf.2",     0x28000, 0x20000, 0x19272e4c )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "xmnvssf.11",   0x000000, 0x200000, 0x9cadcdbc )
    ROM_LOAD( "xmnvssf.12",   0x200000, 0x200000, 0x7b11e460 )
ROM_END


CPS2_GAME_DRIVER(ssf2j,   "Super Street Fighter 2 (Japan)",     "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(sfa,     "Street Fighter Alpha",               "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(sfz,     "Street Fighter Zero",                "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(dadtod,  "Dungeons and Dragons: Tower of Doom","199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(avsp,    "Aliens Vs. Predator",                "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(vmj,     "Vampire?????",                       "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(vphj,    "Vampire Hunter (Japan)",             "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(msh,     "Marvel Super Heroes",                "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(xmencota,"X-Men Children of the Atom",         "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(vsavior, "Vampire Savior",                     "199?","Capcom",ORIENTATION_DEFAULT)
CPS2_GAME_DRIVER(xmnvssf, "X-Men Vs. Street Fighter",           "199?","Capcom",ORIENTATION_DEFAULT)


/***************************************************************************

  Capcom System PSX?
  ==================

    Playstation Hardware with Q sound.

    Why bother adding this? Well, we might be able to get it to play
    the Q sound.

***************************************************************************/

#define CPSX_GAME_DRIVER(NAME,DESCRIPTION,YEAR,MANUFACTURER,ORIENTATION) \
struct GameDriver NAME##_driver =  \
{                                  \
	__FILE__,                      \
	0,                             \
	#NAME,                         \
	DESCRIPTION,                   \
	YEAR,                          \
	MANUFACTURER,                  \
    CPS2_CREDITS,                  \
	0,                             \
	&NAME##_machine_driver,        \
	0,                             \
	NAME##_rom,                    \
    0, 0,                          \
	0,                             \
	0,                             \
	NAME##_input_ports,            \
	0, 0, 0,                       \
	ORIENTATION,                   \
	0, 0                           \
};


#define CPSX_MACHINE_DRIVER(CPS1_DRVNAME, CPS1_CPU_FRQ) \
static struct MachineDriver CPS1_DRVNAME##_machine_driver =            \
{                                                                        \
	/* basic machine hardware */                                     \
	{                                                                \
		{                                                        \
			CPU_Z80 | CPU_AUDIO_CPU,                         \
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */    \
			2,      /* memory region #2 */                   \
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
	{ { SOUND_CUSTOM, &custom_interface } } \
};

#define sfex_input_ports    sfa_input_ports

CPSX_MACHINE_DRIVER( sfex,       CPS2_DEFAULT_CPU_SPEED )

ROM_START( sfex_rom )
    ROM_REGION(CODE_SIZE)      /* PSX code */
    ROM_LOAD("sfe-04",  0x000000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-05m", 0x080000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-06m", 0x100000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-07m", 0x180000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-08m", 0x200000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-09m", 0x280000, 0x80000, 0xffffffff )
    ROM_LOAD("sfe-10m", 0x300000, 0x80000, 0xffffffff )

    ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */

    ROM_REGION(QSOUND_SIZE) /* 64k for the audio CPU (+banks) */
    ROM_LOAD( "sfe-02",         0x00000, 0x08000, 0xffffffff )
    ROM_CONTINUE(               0x10000, 0x18000 )
    ROM_LOAD( "sfe-03",         0x28000, 0x20000, 0xffffffff )

	ROM_REGION(0x400000) /* Q Sound Samples */
    ROM_LOAD( "sfe-01m",        0x000000, 0x400000, 0xffffffff )
ROM_END

CPSX_GAME_DRIVER(sfex,    "Street Fighter EX",                "199?","Capcom",ORIENTATION_DEFAULT)





