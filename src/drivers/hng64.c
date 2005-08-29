/* Hyper NeoGeo 64

Driver by David Haywood, ElSemi, and Andrew Gardner
Rasterizing code provided in part by Andrew Zaferakis

ToDo  12th Oct 2004:

* find where the remainder of the display list information is 'hiding'
    - complete display lists are not currently found in the dl_w memory handler.
      (this manifests itself as 'flickering' 3d in fatfurwa and missing bodies in buriki.
* The effect of numerous (dozens of) 3d flags needs to be figured out by examining real hardware
* Populate the display buffers with the data based on Elsemi's notes below
    - maybe there are some frame-buffer effects which will then work
* Determine wether or not the hardware does perspective-correct texture mapping - if not,
    disable it in the rasterizer.


ToDo  2nd Sept 2004:

fix / add MMU to Mips core to get rid of the ugly hack thats needed
clean up I/O implementation, currently it only 'works' with Fatal Fury WA, it has
issues with the coins and causes the startup checks to fail
work out the purpose of the interrupts and how many are needed
correct game speed (seems too fast)
figure out what 'network' Road Edge needs to boot, it should run as a standalone
make ss64 boot (io return 3 then 4 not just 4) then work out where the palette is (mmu?)
fix remaining 2d graphic glitches
 -- scroll (base registers?)
 -- roz (4th tilemap in fatal fury should be floor, background should zoom)
 -- find registers to control tilemap mode (4bpp/8bpp, 8x8, 16x16)
 -- fix zooming sprites (zoom registers not understood, differ between games?)
 -- priorities
 -- still some bad sprites (waterfall level 'splash')
 -- 3d ram isn't populated at the moment?

hook up communications CPU (z80 based, we have bios rom)
hook up CPU2 (v30 based?) no rom? (maybe its the 'sound driver' the game uploads?)
add sound
backup ram etc. (why does ff have a corrupt 'easy' string in test mode?)
correct cpu speed and find idle skips
work out what other unknown areas of ram are for and emulate...
cleanups!

-----

Known games on this system

Beast Busters 2nd Nightmare ( http://emustatus.rainemu.com/games/bbustr2nd.htm )
Buriki One ( http://emustatus.rainemu.com/games/buriki1.htm )
Fatal Fury: Wild Ambition ( http://emustatus.rainemu.com/games/ffurywa.htm )
Roads Edge / Round Trip? ( http://emustatus.rainemu.com/games/redge.htm )
Samurai Shodown 64 ( http://emustatus.rainemu.com/games/sams64.htm )
Samurai Shodown: Warrior's Rage ( http://emustatus.rainemu.com/games/samswr.htm )
Xtreme Rally / Offbeat Racer ( http://emustatus.rainemu.com/games/xrally.htm )


pr = program
sc = scroll characters?
sd = sound
tx = textures
sp = sprites?
vt = vertex?
*/

/*

NeoGeo Hyper 64 (Main Board)
SNK, 1997

This is a 3D system comprising one large PCB with many custom smt components
on both sides, one interface PCB with JAMMA connector and sound circuitry, and
one game cartridge. Only the Main PCB and interface PCB are detailed here.

PCB Layout (Top)
----------

LVS-MAC SNK 1997.06.02
|--------------------------------------------------------------|
|              CONN9                                           |
|                                                              |
|   ASIC1           ASIC3            CPU1                      |
|                                                              |
|                                               DPRAM1         |
|                        OSC2        ASIC5                 ROM1|
|   FSRAM1               OSC2                                  |
|   FSRAM2                                             FPGA1   |
|   FSRAM3                           ASIC10       OSC4         |
|                                                              |
|                                                 CPU3  IC4    |
|   PSRAM1  ASIC7   ASIC8            DSP1 OSC3    SRAM5        |
|   PSRAM2                                              FROM1  |
|                                                              |
|                                                              |
|              CONN10                                          |
|--------------------------------------------------------------|

No.  PCB Label  IC Markings               IC Package
----------------------------------------------------
01   ASIC1      NEO64-REN                 QFP304
02   ASIC3      NEO64-GTE                 QFP208
03   ASIC5      NEO64-SYS                 QFP208
04   ASIC7      NEO64-BGC                 QFP240
05   ASIC8      NEO64-SPR                 QFP208
06   ASIC10     NEO64-SCC                 QFP208
07   CPU1       NEC D30200GD-100 VR4300   QFP120
08   CPU3       KL5C80A12CFP              QFP80
09   DPRAM1     DT7133 LA35J              PLCC68
10   DSP1       L7A1045 L6028 DSP-A       QFP120
11   FPGA1      ALTERA EPF10K10QC208-4    QFP208
12   FROM1      MBM29F400B-12             TSOP48 (archived as FROM1.BIN)
13   FSRAM1     TC55V1664AJ-15            SOJ44
14   FSRAM2     TC55V1664AJ-15            SOJ44
15   FSRAM3     TC55V1664AJ-15            SOJ44
16   IC4        SMC COM20020-5ILJ         PLCC28
17   OSC1       M33.333 KDS 7M            -
18   OSC2       M50.113 KDS 7L            -
19   OSC3       A33.868 KDS 7M            -
20   OSC4       A40.000 KDS 7L            -
21   PSRAM1     TC551001BFL-70L           SOP32
22   PSRAM2     TC551001BFL-70L           SOP32
23   ROM1       ALTERA EPC1PC8            DIP8   (130817 bytes, archived as ROM1.BIN)
24   SRAM5      TC55257DFL-85L            SOP28


PCB Layout (Bottom)

|--------------------------------------------------------------|
|             CONN10                                           |
|                                                              |
|                                                              |
|   PSRAM4  ASIC9                   SRAM4     CPU1  Y1         |
|   PSRAM3         SRAM1            SRAM3                      |
|                  SRAM2                                       |
|                                                              |
|   FSRAM6                        DRAM3                        |
|   FSRAM5                                                     |
|   FSRAM4                        DRAM1                        |
|                   BROM1         DRAM2                        |
|                                                              |
|                                                              |
|   ASIC2           ASIC4         ASIC6                        |
|                                                              |
|             CONN9                                            |
|--------------------------------------------------------------|

No.  PCB Label  IC Markings               IC Package
----------------------------------------------------
01   ASIC2      NEO64-REN                 QFP304
02   ASIC4      NEO64-TRI2                QFP208
03   ASIC6      NEO64-CVR                 QFP120
04   ASIC9      NEO64-CAL                 QFP208
05   BROM1      MBM29F400B-12             TSOP48  (archived as BROM1.BIN)
06   CPU2       NEC D70326AGJ-16 V53A     QFP120
07   DRAM1      HY51V18164BJC-60          SOJ42
08   DRAM2      HY51V18164BJC-60          SOJ42
09   DRAM3      HY51V18164BJC-60          SOJ42
10   FSRAM4     TC55V1664AJ-15            SOJ44
11   FSRAM5     TC55V1664AJ-15            SOJ44
12   FSRAM6     TC55V1664AJ-15            SOJ44
13   PSRAM3     TC551001BFL-70L           SOP32
14   PSRAM4     TC551001BFL-70L           SOP32
15   SRAM1      TC55257DFL-85L            SOP28
16   SRAM2      TC55257DFL-85L            SOP28
17   SRAM3      TC551001BFL-70L           SOP32
18   SRAM4      TC551001BFL-70L           SOP32
19   Y1         D320L7                    XTAL


INTERFACE PCB
-------------

LVS-JAM SNK 1999.1.20
|---------------------------------------------|
|                 J A M M A                   |
|                                             |
|                                             |
|                                             |
|     SW3             SW1                     |
|                                             |
| IC6                       IOCTR1            |
|                           BACKUP            |
|                           BKRAM1            |
|     SW2   BT1  DPRAM1              IC1      |
|---------------------------------------------|

No.  PCB Label  IC Markings               IC Package
----------------------------------------------------
01   DPRAM1     DT 71321 LA55PF           QFP64
02   IC1        MC44200FT                 QFP44
03   IOCTR1     TOSHIBA TMP87CH40N-4828   SDIP64
04   BACKUP     EPSON RTC62423            SOP24
05   BKRAM1     W24258S-70LE              SOP28
06   IC6        NEC C1891ACY              DIP20
07   BT1        3V Coin Battery
08   SW1        2 position DIPSW  OFF = JAMMA       ON = MVS
09   SW2        4 position DIPSW
10   SW3        2 position DIPSW  OFF = MONO/JAMMA  ON = 2CH MVS

Notes:
       1. The game cart plugs into the main PCB on the TOP side into CONN9 & CONN10
       2. If the game cart is not plugged in, the hardware shows nothing on screen.




Hyper Neo Geo game cartridges
-----------------------------

The game carts contains nothing except a huge pile of surface mounted ROMs
on both sides of the PCB. On a DG1 cart all the roms are 32Mbits, for the
DG2 cart the SC and SP roms are 64Mbit.
The DG1 cart can accept a maximum of 96 ROMs
The DG2 cart can accept a maximum of 84 ROMs


The actual carts are mostly only about 1/3rd to 1/2 populated.
Some of the IC locations between DG1 and DG2 are different also. See the source code below
for the exact number of ROMs used per game and ROM placements.

Games that use the LVS-DG1 cart: Road's Edge

Games that use the LVS-DG2 cart: Fatal Fury: Wild Ambition, Buriki One, SS 64 II

Other games not dumped yet     : Samurai Spirits 64 / Samurai Shodown 64
                                 Samurai Spirits II: Asura Zanmaden / Samurai Shodown: Warrior's Rage
                                 Off Beat Racer / Xtreme Rally
                                 Beast Busters: Second Nightmare
                                 Garou Densetsu 64: Wild Ambition (=Fatal Fury Wild Ambition)*
                                 Round Trip RV (=Road's Edge)*

                               * Different regions might use the same roms, not known yet

                               There might be Rev.A boards for Buriki and Round Trip
Top
---
LVS-DG1
(C) SNK 1997
1997.5.20
|----------------------------------------------------------------------------|
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
| TX01A.5    TX01A.13        VT03A.19   VT02A.18   VT01A.17   PR01B.81       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX02A.6    TX02A.14        VT06A.22   VT05A.21   VT04A.20   PR03A.83       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX03A.7    TX03A.15        VT09A.25   VT08A.24   VT07A.23   PR05A.85       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX04A.8    TX04A.16        VT12A.28   VT11A.27   VT10A.26   PR07A.87       |
|                                                                            |
|                                                                            |
|                                                                            |
|                            PR15A.95   PR13A.93   PR11A.91   PR09A.89       |
|                                                                            |
|                                                                            |
|                                                                            |
| SC09A.49  SC10A.50   SP09A.61   SP10A.62   SP11A.63   SP12A.64    SD02A.78 |
|                                                                            |
|                                                                            |
|                                                                            |
| SC05A.45  SC06A.46   SP05A.57   SP06A.58   SP07A.59   SP08A.60             |
|                                                                            |
|                                                                            |
|                                                                            |
| SC01A.41  SC02A.42   SP01A.53   SP02A.54   SP03A.55   SP04A.56    SD01A.77 |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|----------------------------------------------------------------------------|

Bottom
------

LVS-DG1
|----------------------------------------------------------------------------|
|                         |----------------------|                           |
|                         |----------------------|                           |
|                                                                            |
|                                                                            |
| SC03A.43  SC04A.44   SP13A.65   SP14A.66   SP15A.67   SP16A.68    SD03A.79 |
|                                                                            |
|                                                                            |
|                                                                            |
| SC07A.47  SC08A.48   SP17A.69   SP18A.70   SP19A.71   SP20A.72             |
|                                                                            |
|                                                                            |
|                                                                            |
| SC11A.51  SC12A.52   SP21A.73   SP22A.74   SP23A.75   SP24A.76    SD04A.80 |
|                                                                            |
|                                                                            |
|                                                                            |
|                            PR16A.96   PR14A.94   PR12A.92   PR10A.90       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX04A.4    TX04A.12        VT24A.40   VT23A.39   VT22A.38   PR08A.88       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX03A.3    TX03A.11        VT21A.37   VT20A.36   VT19A.35   PR06A.86       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX02A.2    TX02A.10        VT18A.34   VT17A.33   VT16A.32   PR04A.84       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX01A.1    TX01A.9         VT15A.31   VT14A.30   VT13A.29   PR02B.82       |
|                                                                            |
|                                                                            |
|                         |----------------------|                           |
|                         |----------------------|                           |
|----------------------------------------------------------------------------|


Top
---
LVS-DG2
(C) SNK 1998
1998.6.5
|----------------------------------------------------------------------------|
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
| TX01A.5    TX01A.13        VT03A.19   VT02A.18   VT01A.17   PR01B.81       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX02A.6    TX02A.14        VT06A.22   VT05A.21   VT04A.20   PR03A.83       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX03A.7    TX03A.15        VT09A.25   VT08A.24   VT07A.23   PR05A.85       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX04A.8    TX04A.16        VT12A.28   VT11A.27   VT10A.26   PR07A.87       |
|                                                                            |
|                                                                            |
|                                                                            |
|                            PR15A.95   PR13A.93   PR11A.91   PR09A.89       |
|                                                                            |
|                                                                            |
|                                                                            |
| SC05A.98  SC06A.100  SP09A.107  SP10A.111  SP11A.115  SP12A.119   SD02A.78 |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
| SC01A.97  SC02A.99   SP01A.105  SP02A.109  SP03A.113  SP04A.117   SD01A.77 |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|----------------------------------------------------------------------------|

Bottom
------
LVS-DG2
|----------------------------------------------------------------------------|
|                         |----------------------|                           |
|                         |----------------------|                           |
|                                                                            |
|                                                                            |
| SC03A.101 SC04A.103  SP13A.108  SP14A.112  SP15A.116  SP16A.120   SD03A.79 |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
| SC07A.102  SC08A.104  SP05A.106  SP06A.110  SP07A.114  SP08A.118  SD04A.80 |
|                                                                            |
|                                                                            |
|                                                                            |
|                            PR16A.96   PR14A.94   PR12A.92   PR10A.90       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX04A.4    TX04A.12        VT24A.40   VT23A.39   VT22A.38   PR08A.88       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX03A.3    TX03A.11        VT21A.37   VT20A.36   VT19A.35   PR06A.86       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX02A.2    TX02A.10        VT18A.34   VT17A.33   VT16A.32   PR04A.84       |
|                                                                            |
|                                                                            |
|                                                                            |
| TX01A.1    TX01A.9         VT15A.31   VT14A.30   VT13A.29   PR02B.82       |
|                                                                            |
|                                                                            |
|                         |----------------------|                           |
|                         |----------------------|                           |
|----------------------------------------------------------------------------|
 Notes:
      Not all ROM positions are populated, check the source for exact ROM usage.
      ROMs are mirrored. i.e. TX/PR/SP/SC etc ROMs line up on both sides of the PCB.
      There are 4 copies of each TX ROM on the PCB.


----

info from Daemon

(MACHINE CODE ERROR):
is given wehn you try to put a "RACING GAME" on a "FIGHTING" board

there are various types of neogeo64 boards
FIGHTING (revision 1 and 2)
RACING
SHOOTING
and
SAMURAI SHODOWN ONLY (Korean)

fighting boards will ONLY play fighting games

racing boards will ONLY play racing games (and you need the extra gimmicks
to connect analog wheel and pedals, otherwise it gives you yet another
error)

shooter boards will only work with beast busters 2

and the korean board only plays samurai shodown games (wont play buriki one
or fatal fury for example)

*/

#define MASTER_CLOCK	50000000
#define LOADHNGTEXS 1
#define DECODETEXS 0
#include "driver.h"
#include "cpu/mips/mips3.h"
#include "machine/random.h"

static UINT32 *hng_mainram;
static UINT32 *hng_cart;
static UINT32 *hng64_dualport;
static UINT32 *hng64_soundram;


// Stuff from over in vidhrdw...
extern tilemap *hng64_tilemap, *hng64_tilemap2, *hng64_tilemap3, *hng64_tilemap4 ;
extern UINT32 *hng64_spriteram, *hng64_videoregs ;
extern UINT32 *hng64_videoram ;
extern UINT32 *hng64_fcram ;

extern UINT32 hng64_dls[2][0x81] ;

VIDEO_START( hng64 ) ;
VIDEO_UPDATE( hng64 ) ;

static UINT32 activeBuffer ;


UINT32 no_machine_error_code;
static int hng64_interrupt_level_request;
WRITE32_HANDLER( hng64_videoram_w );

/* AJG */
static UINT32 *hng64_3d_1 ;
static UINT32 *hng64_3d_2 ;
static UINT32 *hng64_dl ;

static UINT32 *hng64_q2 ;


char writeString[1024] ;

/* DRIVER NOTE!!

Hyper NeoGeo makes use of the MIPS MMU, supporting this is *very* messy at the
moment.

If you want something to run you must use the non-drm MIPs core and use the hack
below...

before the function
INLINE void logonetlbentry(int which)   (src/cpu/mips/mips3.c)
put
extern void hyperneogeo_mmu_hack(UINT64 vaddr,UINT64 paddr);
and in the function (at the end) put
hyperneogeo_mmu_hack(vaddr,paddr);

the same can be done with the drc core and it should work


*/

/*

physical

0x00000000  RAM
0x04000000  Cart
0x20000000  Sprites-tilemaps-palette
0x30000000  3D banks
0x60000000  Sound
0x68000000  Something related to sound
0x6E000000  Something related to sound
0x8     ??  (ffury maps these, access to roms?)
0x9     ??
0xa     ??
0xC0000000  Comm


MMU tables:

Virtual     Physical

bios
0xC0000000  0x20000000
0xD0000000  0x30000000
0xE0000000  0x60000000
0x60000000  0xC0000000


fatfury
0x10000000  0x80000000
0x20000000  0x88000000
0x30000000  0x90000000
0x40000000  0x98000000
0x50000000  0xA0000000
0xC0000000  0x20000000
0xD0000000  0x30000000
0xE0000000  0x60000000
0xE8000000  0x68000000
0xEE000000  0x6E000000
0x60000000  0xC0000000


samsho64
0xC0000000  0x20000000
0xC2000000  0x30000000
0xC4000000  0x60000000
0xC6000000  0x6e000000
0xc8000000  0x68000000
0xD0000000  0x30000000
0xE0000000  0x60000000
0x60000000  0xC0000000

<ElSemi> to translate the addresses, first lookup in that table, if it's not there, then output the address (minus the top 3 bits)
<ElSemi> otherwise output the physical add associated

*/

/*
WRITE32_HANDLER( trap_write )
{
    logerror("Remapped write... %08x %08x\n",offset,data);
}
*/


void hyperneogeo_mmu_hack(UINT64 vaddr,UINT64 paddr)
{
	printf("MMU %08x%08x %08x%08x\n",(UINT32)(vaddr >> 32), (UINT32)vaddr, (UINT32)(paddr >> 32), (UINT32)paddr );


	if ((vaddr==U64(0x0000000000000000)) && (paddr==U64(0x0000004000000000)))
	{
		printf("Remapped 0 to vidram!\n");
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000000, 0x07fffff, 0, 0, hng64_videoram_w);
	//  memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000000, 0x07fffff, 0, 0, hng64_videoram_r);

	}
}

WRITE32_HANDLER( hng64_videoram_w )
{
	int realoff;
	COMBINE_DATA(&hng64_videoram[offset]);

	realoff = offset*4;

	if ((realoff>=0) && (realoff<0x10000))
	{
		tilemap_mark_tile_dirty(hng64_tilemap,offset&0x3fff);
	}
	else if ((realoff>=0x10000) && (realoff<0x20000))
	{
		tilemap_mark_tile_dirty(hng64_tilemap2,offset&0x3fff);
	}
	else if ((realoff>=0x20000) && (realoff<0x30000))
	{
		tilemap_mark_tile_dirty(hng64_tilemap3,offset&0x3fff);
	}
	else if ((realoff>=0x30000) && (realoff<0x40000))
	{
		tilemap_mark_tile_dirty(hng64_tilemap4,offset&0x3fff);
	}

//  if ((realoff>=0x40000) && (realoff<=0x48000)) printf("offsw %08x %08x\n",realoff,data);

	/* 400000 - 7fffff is scroll regs etc. */
}


READ32_HANDLER( hng64_random_reader )
{
	return mame_rand()&0xffffffff;
}


READ32_HANDLER( hng64_comm_r )
{
	printf("commr %08x\n",offset);

	if(offset==0x1)
		return 0xffffffff;
	if(offset==0)
		return 0xAAAA;
	return 0x00000000;
}



static WRITE32_HANDLER( hng64_pal_w )
{
	int r,g,b,a;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x000000ff) >>0);
	g = ((paletteram32[offset] & 0x0000ff00) >>8);
	r = ((paletteram32[offset] & 0x00ff0000) >>16);
	a = ((paletteram32[offset] & 0xff000000) >>24);

	// a sure ain't alpha ???
	// alpha_set_level(mame_rand()) ;
	// printf("Alpha : %d %d %d %d\n", a, b, g, r) ;

	// if (a != 0)
	//  ui_popup("Alpha is not zero!") ;

	palette_set_color(offset,r,g,b);
}


static READ32_HANDLER( hng64_port_read )
{
	if(offset==0x85b)
		return 0x00000010;
	if(offset==0x421)
		return 0x00000002;
 	if(offset==0x441)
 		return hng64_interrupt_level_request;

	return mame_rand();
}


/* preliminary dma code, dma is used to copy program code -> ram */
int hng_dma_start,hng_dma_dst,hng_dma_len;
void hng64_do_dma (void)
{


	printf("Performing DMA Start %08x Len %08x Dst %08x\n",hng_dma_start, hng_dma_len, hng_dma_dst);

	while (hng_dma_len>0)
	{
		UINT32 dat;

		dat = program_read_dword(hng_dma_start);
		program_write_dword(hng_dma_dst,dat);
		hng_dma_start+=4;
		hng_dma_dst+=4;
		hng_dma_len--;
	}

	hng_dma_start+=4;
	hng_dma_dst+=4;
}


WRITE32_HANDLER( hng_dma_start_w )
{
	logerror ("DMA Start Write %08x\n",data);
	hng_dma_start = data;
}

WRITE32_HANDLER( hng_dma_dst_w )
{
	logerror ("DMA Dst Write %08x\n",data);
	hng_dma_dst = data;
}

WRITE32_HANDLER( hng_dma_len_w )
{
	logerror ("DMA Len Write %08x\n",data);
	hng_dma_len = data;
	hng64_do_dma();

}

READ32_HANDLER( hng64_videoram_r )
{
	return hng64_videoram[offset];
}

WRITE32_HANDLER( hng64_mainram_w )
{
	COMBINE_DATA (&hng_mainram[offset]);
}

READ32_HANDLER( hng64_cart_r )
{
	return hng_cart[offset];
}

READ32_HANDLER( no_machine_error )
{
	return no_machine_error_code;
}

WRITE32_HANDLER( hng64_dualport_w )
{
//  printf("dualport W %08x %08x %08x\n", activecpu_get_pc(), offset, hng64_dualport[offset]);

	COMBINE_DATA (&hng64_dualport[offset]);
}

READ32_HANDLER( hng64_dualport_r )
{
//  printf("dualport R %08x %08x %08x\n", activecpu_get_pc(), offset, hng64_dualport[offset]);
//  return mame_rand();

	return hng64_dualport[offset];
}

/* AJG */
WRITE32_HANDLER( hng64_3d_1_w )
{
	// !!! Never does a write to 1 in the tests !!!
	COMBINE_DATA (&hng64_3d_1[offset]) ;
	COMBINE_DATA (&hng64_3d_2[offset]) ;
//  printf("1w : %d %d\n", offset, hng64_3d_1[offset]) ;

	exit(1) ;
}

WRITE32_HANDLER( hng64_3d_2_w )
{
	COMBINE_DATA (&hng64_3d_1[offset]) ;
	COMBINE_DATA (&hng64_3d_2[offset]) ;
//  printf("2w : %d %d\n", offset, hng64_3d_2[offset]) ;
}

READ32_HANDLER( hng64_3d_1_r )
{
//  printf("1r : %d %d\n", offset, hng64_3d_1[offset]) ;
	return hng64_3d_1[offset] ;
}

READ32_HANDLER( hng64_3d_2_r )
{
//  printf("2r : %d %d\n", offset, hng64_3d_2[offset]) ;
	return hng64_3d_2[offset] ;
}


// These are for the 3d 'display list'

WRITE32_HANDLER( dl_w )
{
	COMBINE_DATA (&hng64_dl[offset]) ;

	// !!! There are a few other writes over 0x80 as well - don't forget about those someday !!!

	// This method of finding the 85 writes and switching banks seems to work
	//  the problem is when there are a lot of things to be drawn (more than what can fit in 2*0x80)
	//  the list doesn't fit anymore, and parts aren't drawn every frame.

	if (offset == 0x85)
	{
		// Only if it's VALID
		if ((INT32)hng64_dl[offset] == 1 || (INT32)hng64_dl[offset] == 2)
			activeBuffer = (INT32)hng64_dl[offset] - 1 ;		// Subtract 1 to fit into my array...
	}

	if (offset <= 0x80)
		hng64_dls[activeBuffer][offset] = hng64_dl[offset] ;

//  printf("dl W : %.8x %.8x\n", offset, hng64_dl[offset]) ;

	// Write out some interesting parts of the 3d display list ...
	/*
    if (!flagFlag)
    {
        if (offset >= 0x00000007)
        {
            dataArray[offset-0x00000007] = hng64_dl[offset] ;
        }
    }

    if (offset == 0x00000017)
    {
        flagFlag = 1 ;
        // a test out of curiosity
        hng64_dl[offset] = 1 ;
    }
    */
}

READ32_HANDLER( dl_r )
{
//  printf("dl R : %x %x\n", offset, hng64_dl[offset]) ;
	return hng64_dl[offset] ;
}

/*
WRITE32_HANDLER( activate_3d_buffer )
{
    COMBINE_DATA (&active_3d_buffer[offset]) ;
    printf("COMBINED %d\n", active_3d_buffer[offset]) ;
}
*/

// Transition Control memory...
WRITE32_HANDLER( fcram_w )
{
	COMBINE_DATA (&hng64_fcram[offset]) ;
//  printf("Q1 W : %.8x %.8x\n", offset, hng64_fcram[offset]) ;

	if (offset == 0x00000007)
	{
		sprintf(writeString, "%.8x ", hng64_fcram[offset]) ;
	}

	if (offset == 0x0000000a)
	{
		sprintf(writeString, "%s %.8x ", writeString, hng64_fcram[offset]) ;
	}

	if (offset == 0x0000000b)
	{
		sprintf(writeString, "%s %.8x ", writeString, hng64_fcram[offset]) ;
//      ui_popup("%s", writeString) ;
	}
}

READ32_HANDLER( fcram_r )
{
//  printf("Q1 R : %.8x %.8x\n", offset, hng64_fcram[offset]) ;
	return hng64_fcram[offset] ;
}


// Q2 handler (just after display list in memory)
// Seems to read and write the same thing for every frame in fatfurwa
/*
    Q2 R : 00000000 00311800
    Q2 W : 00000003 03000000
    Q2 W : 00000002 e0001c00
    Q2 W : 00000002 e0001c00
    Q2 W : 00000001 3fe037e0
    Q2 W : 00000001 3fe037e0
    Q2 W : 00000000 00311800

    In the beginning it likes setting 0x04-0x0b to what looks like
    a bunch of bit-flags...
*/
WRITE32_HANDLER( q2_w )
{
	COMBINE_DATA (&hng64_q2[offset]) ;
	// printf("Q2 W : %.8x %.8x\n", offset, hng64_q2[offset]) ;
}

READ32_HANDLER( q2_r )
{
	// printf("Q2 R : %.8x %.8x\n", offset, hng64_q2[offset]) ;
	return hng64_q2[offset] ;
}


/*

<ElSemi> 0xE0000000 sound
<ElSemi> 0xD0100000 3D bank A
<ElSemi> 0xD0200000 3D bank B
<ElSemi> 0xC0000000-0xC000C000 Sprite
<ElSemi> 0xC0200000-0xC0204000 palette
<ElSemi> 0xC0100000-0xC0180000 Tilemap
<ElSemi> 0xBF808000-0xBF808800 Dualport ram
<ElSemi> 0xBF800000-0xBF808000 S-RAM
<ElSemi> 0x60000000-0x60001000 Comm dualport ram

*/

/*
<ElSemi> d0100000-d011ffff is framebuffer A0
<ElSemi> d0120000-d013ffff is framebuffer A1
<ElSemi> d0140000-d015ffff is ZBuffer A
*/

READ32_HANDLER( hng64_inputs_r )
{
//  printf("hng read %08x %08x\n",offset,mem_mask);

//  static int toggi=0;


	switch (offset*4)
	{
//      case 0x00: toggi^=1; if (toggi==1) {return 0x00000400;} else {return 0x00000300;};//otherwise it won't read inputs ..
		case 0x00: return 0x00000400;
		case 0x04: return readinputport(0)|(readinputport(1)<<16);
		case 0x08: return readinputport(3)|(readinputport(2)<<16);
	}
	return 0;

}

READ32_HANDLER ( random_read )
{
	return mame_rand();
}

WRITE32_HANDLER( hng64_soundram_w )
{
	/* swap data around.. keep the v30 happy ;-) */
	data =
	(
	((data & 0xff000000) >> 24) |
	((data & 0x00ff0000) >> 8 ) |
	((data & 0x0000ff00) << 8 ) |
	((data & 0x000000ff) << 24)
	);

	/* mem mask too */
	mem_mask =
	(
	((mem_mask & 0xff000000) >> 24) |
	((mem_mask & 0x00ff0000) >> 8 ) |
	((mem_mask & 0x0000ff00) << 8 ) |
	((mem_mask & 0x000000ff) << 24)
	);

	COMBINE_DATA(&hng64_soundram[offset]);
}

READ32_HANDLER( hng64_soundram_r )
{
	int data = hng64_soundram[offset];

	/* we had to swap it when we wrote it, we have to swap it when we read it */
	data =
	(
	((data & 0xff000000) >> 24) |
	((data & 0x00ff0000) >> 8 ) |
	((data & 0x0000ff00) << 8 ) |
	((data & 0x000000ff) << 24)
	);

	return data;
}

static ADDRESS_MAP_START( hng_map, ADDRESS_SPACE_PROGRAM, 32 )

	AM_RANGE(0x00000000, 0x00ffffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x20000000, 0x20ffffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x40000000, 0x40ffffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x80000000, 0x80ffffff) AM_RAM AM_SHARE(3) AM_BASE(&hng_mainram)
	AM_RANGE(0xa0000000, 0xa0ffffff) AM_RAM AM_SHARE(3)


	AM_RANGE(0xc0000000, 0xc000bfff) AM_RAM AM_BASE(&hng64_spriteram)/* Sprites */
	AM_RANGE(0xc0010000, 0xc0010013) AM_RAM
	AM_RANGE(0xc0100000, 0xc017ffff) AM_READWRITE(MRA32_RAM, hng64_videoram_w) AM_BASE(&hng64_videoram) // tilemap
	AM_RANGE(0xc0190000, 0xc0190037) AM_RAM AM_BASE(&hng64_videoregs)
	AM_RANGE(0xc0200000, 0xc0203fff) AM_READWRITE(MRA32_RAM,hng64_pal_w) AM_BASE(&paletteram32)// palette


	AM_RANGE(0xC0208000, 0xC020805f) AM_READWRITE(fcram_r, fcram_w) AM_BASE(&hng64_fcram)

	AM_RANGE(0xC0300000, 0xC030ffff) AM_READWRITE(dl_r, dl_w) AM_BASE(&hng64_dl)
	AM_RANGE(0xd0000000, 0xd000002f) AM_READWRITE(q2_r, q2_w) AM_BASE(&hng64_q2)


	AM_RANGE(0xd0100000, 0xd015ffff) AM_READWRITE(hng64_3d_1_r,hng64_3d_2_w) AM_BASE(&hng64_3d_1) /* 3D Bank A */
	AM_RANGE(0xd0200000, 0xd025ffff) AM_READWRITE(hng64_3d_2_r,hng64_3d_2_w) AM_BASE(&hng64_3d_2) /* 3D Bank B */

	AM_RANGE(0xe0000000, 0xe01fffff) AM_RAM /* Sound ?? */
	AM_RANGE(0xe0200000, 0xe03fffff) AM_READWRITE(hng64_soundram_r, hng64_soundram_w) /* uploads the v53 sound program here, elsewhere on ss64-2 */

	AM_RANGE(0xE8000000, 0xE8000003) AM_WRITE(MWA32_NOP)//??
	AM_RANGE(0xE8000004, 0xE8000007) AM_READ(MRA32_NOP)//??


	AM_RANGE(0x60000000, 0x60000fff) AM_RAM /* Dualp */

	AM_RANGE(0x60001000, 0x6000ffff) AM_READ(hng64_comm_r)

	AM_RANGE(0x04000000, 0x05ffffff) AM_WRITENOP AM_ROM AM_SHARE(1) AM_REGION(REGION_USER3,0)
	AM_RANGE(0x24000000, 0x25ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)
	AM_RANGE(0x44000000, 0x45ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)
	AM_RANGE(0x64000000, 0x65ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)
	AM_RANGE(0x84000000, 0x85ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)
	AM_RANGE(0xa4000000, 0xa5ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)  AM_BASE(&hng_cart)// reads HYPER NEOGEO 64 string from 0xa4000060 but does less if its present?
	AM_RANGE(0xc4000000, 0xc5ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)
	AM_RANGE(0xe4000000, 0xe5ffffff) AM_WRITENOP AM_ROM AM_SHARE(1)

	AM_RANGE(0xBF700000, 0xBF702fff) AM_READ(hng64_port_read)

	AM_RANGE(0xBF70100C, 0xBF70100F) AM_WRITE(MWA32_NOP)//?? often
	AM_RANGE(0xBF70101C, 0xBF70101F) AM_WRITE(MWA32_NOP)//?? often
	AM_RANGE(0xBF70106C, 0xBF70106F) AM_WRITE(MWA32_NOP)//fatfur,strange
	AM_RANGE(0xBF70111C, 0xBF70111F) AM_WRITE(MWA32_NOP)//irq ack
	AM_RANGE(0xBF701204, 0xBF701207) AM_WRITE(hng_dma_start_w);
	AM_RANGE(0xBF701214, 0xBF701217) AM_WRITE(hng_dma_dst_w);
	AM_RANGE(0xBF701224, 0xBF701227) AM_WRITE(hng_dma_len_w);
	AM_RANGE(0xBF70124C, 0xBF70124F) AM_WRITE(MWA32_NOP)//dma related?
	AM_RANGE(0xBF70125C, 0xBF70125F) AM_WRITE(MWA32_NOP)//dma related?

	AM_RANGE(0xBF7021C4, 0xBF7021C7) AM_WRITE(MWA32_NOP)//?? often

	/* SRAM? -- reads times etc.?  Shared? System Log? */
	AM_RANGE(0xBF800000, 0xBF803fff) AM_RAM
	AM_RANGE(0xBF808000, 0xBF80800b) AM_READ(hng64_inputs_r) /* HACK .. proper method unknown */
	AM_RANGE(0xBF808600, 0xBF808603) AM_READ(no_machine_error) /* HACK .. prevent machine error (02 driving game, 01 fighting game) */
	AM_RANGE(0xBF808000, 0xBF8087ff) AM_RAM AM_READWRITE(hng64_dualport_r, hng64_dualport_w) AM_BASE (&hng64_dualport) /* Dualport RAM */

	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x3fc00000, 0x3fc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0x5fc00000, 0x5fc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0x7fc00000, 0x7fc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0xdfc00000, 0xdfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
	AM_RANGE(0xffc00000, 0xffc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2)	/* bios mirror */
ADDRESS_MAP_END


static ADDRESS_MAP_START( hng_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xe0000, 0xfffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END


INPUT_PORTS_START( hng64 )
	PORT_START	/* 16bit */


	PORT_START	/* 16bit */
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_COIN1)
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_COIN2)
	PORT_BIT(  0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0100, IP_ACTIVE_HIGH, IPT_SERVICE1)
	PORT_BIT(  0x0200, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0400, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_DIPNAME( 0x8000, 0x0000, "TST" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START	/* Player 1 */
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_PLAYER(1)
	PORT_BIT(  0x0100, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0200, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0400, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_START1)
	PORT_BIT(  0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START	/* Player 2  */
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_PLAYER(2)
	PORT_BIT(  0x0100, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0200, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0400, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_START2)
	PORT_BIT(  0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN)
INPUT_PORTS_END



/* the 4bpp gfx encoding is annoying */

static gfx_layout hng64_4_even_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 40, 44, 8,12,  32,36,    0,4},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};

static gfx_layout hng64_4_odd_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{  56,60, 24, 28,  48,52,   16, 20 },


	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};


static gfx_layout hng64_4_16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{  56,60, 24, 28,  48,52,   16, 20,40, 44, 8,12,  32,36,    0,4 },


	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};


static gfx_layout hng64_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 56, 24, 48,16,  40, 8,  32, 0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};


static gfx_layout hng64_16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },

	{ 56, 24, 48,16,  40, 8,  32, 0,
	 1024+56, 1024+24, 1024+48,1024+16,  1024+40, 1024+8,  1024+32, 1024+0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	32*64
};

#if LOADHNGTEXS
#if DECODETEXS
/* not really much point in this, but it allows us to see the 1024x1024 texture pages */
static gfx_layout hng64_tex_layout =
{
	1024,1024,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
	   0*8,    1*8,   2*8,     3*8,    4*8,    5*8,    6*8,    7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	  16*8,   17*8,   18*8,   19*8,   20*8,   21*8,   22*8,   23*8, 24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8,
	  32*8,   33*8,   34*8,   35*8,   36*8,   37*8,   38*8,   39*8, 40*8, 41*8, 42*8, 43*8, 44*8, 45*8, 46*8, 47*8,
	  48*8,   49*8,   50*8,   51*8,   52*8,   53*8,   54*8,   55*8, 56*8, 57*8, 58*8, 59*8, 60*8, 61*8, 62*8, 63*8,
	  64*8,   65*8,   66*8,   67*8,   68*8,   69*8,   70*8,   71*8, 72*8, 73*8, 74*8, 75*8, 76*8, 77*8, 78*8, 79*8,
	  80*8,   81*8,   82*8,   83*8,   84*8,   85*8,   86*8,   87*8, 88*8, 89*8, 90*8, 91*8, 92*8, 93*8, 94*8, 95*8,
	  96*8,   97*8,   98*8,   99*8,  100*8,  101*8,  102*8,  103*8, 104*8, 105*8, 106*8, 107*8, 108*8, 109*8, 110*8, 111*8,
	 112*8,  113*8,  114*8,  115*8,  116*8,  117*8,  118*8,  119*8, 120*8, 121*8, 122*8, 123*8, 124*8, 125*8, 126*8, 127*8,
	 128*8,  129*8,  130*8,  131*8,  132*8,  133*8,  134*8,  135*8, 136*8, 137*8, 138*8, 139*8, 140*8, 141*8, 142*8, 143*8,
	 144*8,  145*8,  146*8,  147*8,  148*8,  149*8,  150*8,  151*8, 152*8, 153*8, 154*8, 155*8, 156*8, 157*8, 158*8, 159*8,
	 160*8,  161*8,  162*8,  163*8,  164*8,  165*8,  166*8,  167*8, 168*8, 169*8, 170*8, 171*8, 172*8, 173*8, 174*8, 175*8,
	 176*8,  177*8,  178*8,  179*8,  180*8,  181*8,  182*8,  183*8, 184*8, 185*8, 186*8, 187*8, 188*8, 189*8, 190*8, 191*8,
	 192*8,  193*8,  194*8,  195*8,  196*8,  197*8,  198*8,  199*8, 200*8, 201*8, 202*8, 203*8, 204*8, 205*8, 206*8, 207*8,
	 208*8,  209*8,  210*8,  211*8,  212*8,  213*8,  214*8,  215*8, 216*8, 217*8, 218*8, 219*8, 220*8, 221*8, 222*8, 223*8,
	 224*8,  225*8,  226*8,  227*8,  228*8,  229*8,  230*8,  231*8, 232*8, 233*8, 234*8, 235*8, 236*8, 237*8, 238*8, 239*8,
	 240*8,  241*8,  242*8,  243*8,  244*8,  245*8,  246*8,  247*8, 248*8, 249*8, 250*8, 251*8, 252*8, 253*8, 254*8, 255*8,
	 256*8,  257*8,  258*8,  259*8,  260*8,  261*8,  262*8,  263*8, 264*8, 265*8, 266*8, 267*8, 268*8, 269*8, 270*8, 271*8,
	 272*8,  273*8,  274*8,  275*8,  276*8,  277*8,  278*8,  279*8, 280*8, 281*8, 282*8, 283*8, 284*8, 285*8, 286*8, 287*8,
	 288*8,  289*8,  290*8,  291*8,  292*8,  293*8,  294*8,  295*8, 296*8, 297*8, 298*8, 299*8, 300*8, 301*8, 302*8, 303*8,
	 304*8,  305*8,  306*8,  307*8,  308*8,  309*8,  310*8,  311*8, 312*8, 313*8, 314*8, 315*8, 316*8, 317*8, 318*8, 319*8,
	 320*8,  321*8,  322*8,  323*8,  324*8,  325*8,  326*8,  327*8, 328*8, 329*8, 330*8, 331*8, 332*8, 333*8, 334*8, 335*8,
	 336*8,  337*8,  338*8,  339*8,  340*8,  341*8,  342*8,  343*8, 344*8, 345*8, 346*8, 347*8, 348*8, 349*8, 350*8, 351*8,
	 352*8,  353*8,  354*8,  355*8,  356*8,  357*8,  358*8,  359*8, 360*8, 361*8, 362*8, 363*8, 364*8, 365*8, 366*8, 367*8,
	 368*8,  369*8,  370*8,  371*8,  372*8,  373*8,  374*8,  375*8, 376*8, 377*8, 378*8, 379*8, 380*8, 381*8, 382*8, 383*8,
	 384*8,  385*8,  386*8,  387*8,  388*8,  389*8,  390*8,  391*8, 392*8, 393*8, 394*8, 395*8, 396*8, 397*8, 398*8, 399*8,
	 400*8,  401*8,  402*8,  403*8,  404*8,  405*8,  406*8,  407*8, 408*8, 409*8, 410*8, 411*8, 412*8, 413*8, 414*8, 415*8,
	 416*8,  417*8,  418*8,  419*8,  420*8,  421*8,  422*8,  423*8, 424*8, 425*8, 426*8, 427*8, 428*8, 429*8, 430*8, 431*8,
	 432*8,  433*8,  434*8,  435*8,  436*8,  437*8,  438*8,  439*8, 440*8, 441*8, 442*8, 443*8, 444*8, 445*8, 446*8, 447*8,
	 448*8,  449*8,  450*8,  451*8,  452*8,  453*8,  454*8,  455*8, 456*8, 457*8, 458*8, 459*8, 460*8, 461*8, 462*8, 463*8,
	 464*8,  465*8,  466*8,  467*8,  468*8,  469*8,  470*8,  471*8, 472*8, 473*8, 474*8, 475*8, 476*8, 477*8, 478*8, 479*8,
	 480*8,  481*8,  482*8,  483*8,  484*8,  485*8,  486*8,  487*8, 488*8, 489*8, 490*8, 491*8, 492*8, 493*8, 494*8, 495*8,
	 496*8,  497*8,  498*8,  499*8,  500*8,  501*8,  502*8,  503*8, 504*8, 505*8, 506*8, 507*8, 508*8, 509*8, 510*8, 511*8,
	 512*8,  513*8,  514*8,  515*8,  516*8,  517*8,  518*8,  519*8, 520*8, 521*8, 522*8, 523*8, 524*8, 525*8, 526*8, 527*8,
	 528*8,  529*8,  530*8,  531*8,  532*8,  533*8,  534*8,  535*8, 536*8, 537*8, 538*8, 539*8, 540*8, 541*8, 542*8, 543*8,
	 544*8,  545*8,  546*8,  547*8,  548*8,  549*8,  550*8,  551*8, 552*8, 553*8, 554*8, 555*8, 556*8, 557*8, 558*8, 559*8,
	 560*8,  561*8,  562*8,  563*8,  564*8,  565*8,  566*8,  567*8, 568*8, 569*8, 570*8, 571*8, 572*8, 573*8, 574*8, 575*8,
	 576*8,  577*8,  578*8,  579*8,  580*8,  581*8,  582*8,  583*8, 584*8, 585*8, 586*8, 587*8, 588*8, 589*8, 590*8, 591*8,
	 592*8,  593*8,  594*8,  595*8,  596*8,  597*8,  598*8,  599*8, 600*8, 601*8, 602*8, 603*8, 604*8, 605*8, 606*8, 607*8,
	 608*8,  609*8,  610*8,  611*8,  612*8,  613*8,  614*8,  615*8, 616*8, 617*8, 618*8, 619*8, 620*8, 621*8, 622*8, 623*8,
	 624*8,  625*8,  626*8,  627*8,  628*8,  629*8,  630*8,  631*8, 632*8, 633*8, 634*8, 635*8, 636*8, 637*8, 638*8, 639*8,
	 640*8,  641*8,  642*8,  643*8,  644*8,  645*8,  646*8,  647*8, 648*8, 649*8, 650*8, 651*8, 652*8, 653*8, 654*8, 655*8,
	 656*8,  657*8,  658*8,  659*8,  660*8,  661*8,  662*8,  663*8, 664*8, 665*8, 666*8, 667*8, 668*8, 669*8, 670*8, 671*8,
	 672*8,  673*8,  674*8,  675*8,  676*8,  677*8,  678*8,  679*8, 680*8, 681*8, 682*8, 683*8, 684*8, 685*8, 686*8, 687*8,
	 688*8,  689*8,  690*8,  691*8,  692*8,  693*8,  694*8,  695*8, 696*8, 697*8, 698*8, 699*8, 700*8, 701*8, 702*8, 703*8,
	 704*8,  705*8,  706*8,  707*8,  708*8,  709*8,  710*8,  711*8, 712*8, 713*8, 714*8, 715*8, 716*8, 717*8, 718*8, 719*8,
	 720*8,  721*8,  722*8,  723*8,  724*8,  725*8,  726*8,  727*8, 728*8, 729*8, 730*8, 731*8, 732*8, 733*8, 734*8, 735*8,
	 736*8,  737*8,  738*8,  739*8,  740*8,  741*8,  742*8,  743*8, 744*8, 745*8, 746*8, 747*8, 748*8, 749*8, 750*8, 751*8,
	 752*8,  753*8,  754*8,  755*8,  756*8,  757*8,  758*8,  759*8, 760*8, 761*8, 762*8, 763*8, 764*8, 765*8, 766*8, 767*8,
	 768*8,  769*8,  770*8,  771*8,  772*8,  773*8,  774*8,  775*8, 776*8, 777*8, 778*8, 779*8, 780*8, 781*8, 782*8, 783*8,
	 784*8,  785*8,  786*8,  787*8,  788*8,  789*8,  790*8,  791*8, 792*8, 793*8, 794*8, 795*8, 796*8, 797*8, 798*8, 799*8,
	 800*8,  801*8,  802*8,  803*8,  804*8,  805*8,  806*8,  807*8, 808*8, 809*8, 810*8, 811*8, 812*8, 813*8, 814*8, 815*8,
	 816*8,  817*8,  818*8,  819*8,  820*8,  821*8,  822*8,  823*8, 824*8, 825*8, 826*8, 827*8, 828*8, 829*8, 830*8, 831*8,
	 832*8,  833*8,  834*8,  835*8,  836*8,  837*8,  838*8,  839*8, 840*8, 841*8, 842*8, 843*8, 844*8, 845*8, 846*8, 847*8,
	 848*8,  849*8,  850*8,  851*8,  852*8,  853*8,  854*8,  855*8, 856*8, 857*8, 858*8, 859*8, 860*8, 861*8, 862*8, 863*8,
	 864*8,  865*8,  866*8,  867*8,  868*8,  869*8,  870*8,  871*8, 872*8, 873*8, 874*8, 875*8, 876*8, 877*8, 878*8, 879*8,
	 880*8,  881*8,  882*8,  883*8,  884*8,  885*8,  886*8,  887*8, 888*8, 889*8, 890*8, 891*8, 892*8, 893*8, 894*8, 895*8,
	 896*8,  897*8,  898*8,  899*8,  900*8,  901*8,  902*8,  903*8, 904*8, 905*8, 906*8, 907*8, 908*8, 909*8, 910*8, 911*8,
	 912*8,  913*8,  914*8,  915*8,  916*8,  917*8,  918*8,  919*8, 920*8, 921*8, 922*8, 923*8, 924*8, 925*8, 926*8, 927*8,
	 928*8,  929*8,  930*8,  931*8,  932*8,  933*8,  934*8,  935*8, 936*8, 937*8, 938*8, 939*8, 940*8, 941*8, 942*8, 943*8,
	 944*8,  945*8,  946*8,  947*8,  948*8,  949*8,  950*8,  951*8, 952*8, 953*8, 954*8, 955*8, 956*8, 957*8, 958*8, 959*8,
	 960*8,  961*8,  962*8,  963*8,  964*8,  965*8,  966*8,  967*8, 968*8, 969*8, 970*8, 971*8, 972*8, 973*8, 974*8, 975*8,
	 976*8,  977*8,  978*8,  979*8,  980*8,  981*8,  982*8,  983*8, 984*8, 985*8, 986*8, 987*8, 988*8, 989*8, 990*8, 991*8,
	 992*8,  993*8,  994*8,  995*8,  996*8,  997*8,  998*8,  999*8, 1000*8, 1001*8, 1002*8, 1003*8, 1004*8, 1005*8, 1006*8, 1007*8,
	1008*8, 1009*8, 1010*8, 1011*8, 1012*8, 1013*8, 1014*8, 1015*8, 1016*8, 1017*8, 1018*8, 1019*8, 1020*8, 1021*8, 1022*8, 1023*8,
	},
	{
	0*8192, 1*8192, 2*8192, 3*8192, 4*8192, 5*8192, 6*8192, 7*8192, 8*8192, 9*8192, 10*8192, 11*8192, 12*8192, 13*8192, 14*8192, 15*8192,
	16*8192, 17*8192, 18*8192, 19*8192, 20*8192, 21*8192, 22*8192, 23*8192, 24*8192, 25*8192, 26*8192, 27*8192, 28*8192, 29*8192, 30*8192, 31*8192,
	32*8192, 33*8192, 34*8192, 35*8192, 36*8192, 37*8192, 38*8192, 39*8192, 40*8192, 41*8192, 42*8192, 43*8192, 44*8192, 45*8192, 46*8192, 47*8192,
	48*8192, 49*8192, 50*8192, 51*8192, 52*8192, 53*8192, 54*8192, 55*8192, 56*8192, 57*8192, 58*8192, 59*8192, 60*8192, 61*8192, 62*8192, 63*8192,
	64*8192, 65*8192, 66*8192, 67*8192, 68*8192, 69*8192, 70*8192, 71*8192, 72*8192, 73*8192, 74*8192, 75*8192, 76*8192, 77*8192, 78*8192, 79*8192,
	80*8192, 81*8192, 82*8192, 83*8192, 84*8192, 85*8192, 86*8192, 87*8192, 88*8192, 89*8192, 90*8192, 91*8192, 92*8192, 93*8192, 94*8192, 95*8192,
	96*8192, 97*8192, 98*8192, 99*8192, 100*8192, 101*8192, 102*8192, 103*8192, 104*8192, 105*8192, 106*8192, 107*8192, 108*8192, 109*8192, 110*8192, 111*8192,
	112*8192, 113*8192, 114*8192, 115*8192, 116*8192, 117*8192, 118*8192, 119*8192, 120*8192, 121*8192, 122*8192, 123*8192, 124*8192, 125*8192, 126*8192, 127*8192,
	128*8192, 129*8192, 130*8192, 131*8192, 132*8192, 133*8192, 134*8192, 135*8192, 136*8192, 137*8192, 138*8192, 139*8192, 140*8192, 141*8192, 142*8192, 143*8192,
	144*8192, 145*8192, 146*8192, 147*8192, 148*8192, 149*8192, 150*8192, 151*8192, 152*8192, 153*8192, 154*8192, 155*8192, 156*8192, 157*8192, 158*8192, 159*8192,
	160*8192, 161*8192, 162*8192, 163*8192, 164*8192, 165*8192, 166*8192, 167*8192, 168*8192, 169*8192, 170*8192, 171*8192, 172*8192, 173*8192, 174*8192, 175*8192,
	176*8192, 177*8192, 178*8192, 179*8192, 180*8192, 181*8192, 182*8192, 183*8192, 184*8192, 185*8192, 186*8192, 187*8192, 188*8192, 189*8192, 190*8192, 191*8192,
	192*8192, 193*8192, 194*8192, 195*8192, 196*8192, 197*8192, 198*8192, 199*8192, 200*8192, 201*8192, 202*8192, 203*8192, 204*8192, 205*8192, 206*8192, 207*8192,
	208*8192, 209*8192, 210*8192, 211*8192, 212*8192, 213*8192, 214*8192, 215*8192, 216*8192, 217*8192, 218*8192, 219*8192, 220*8192, 221*8192, 222*8192, 223*8192,
	224*8192, 225*8192, 226*8192, 227*8192, 228*8192, 229*8192, 230*8192, 231*8192, 232*8192, 233*8192, 234*8192, 235*8192, 236*8192, 237*8192, 238*8192, 239*8192,
	240*8192, 241*8192, 242*8192, 243*8192, 244*8192, 245*8192, 246*8192, 247*8192, 248*8192, 249*8192, 250*8192, 251*8192, 252*8192, 253*8192, 254*8192, 255*8192,
	256*8192, 257*8192, 258*8192, 259*8192, 260*8192, 261*8192, 262*8192, 263*8192, 264*8192, 265*8192, 266*8192, 267*8192, 268*8192, 269*8192, 270*8192, 271*8192,
	272*8192, 273*8192, 274*8192, 275*8192, 276*8192, 277*8192, 278*8192, 279*8192, 280*8192, 281*8192, 282*8192, 283*8192, 284*8192, 285*8192, 286*8192, 287*8192,
	288*8192, 289*8192, 290*8192, 291*8192, 292*8192, 293*8192, 294*8192, 295*8192, 296*8192, 297*8192, 298*8192, 299*8192, 300*8192, 301*8192, 302*8192, 303*8192,
	304*8192, 305*8192, 306*8192, 307*8192, 308*8192, 309*8192, 310*8192, 311*8192, 312*8192, 313*8192, 314*8192, 315*8192, 316*8192, 317*8192, 318*8192, 319*8192,
	320*8192, 321*8192, 322*8192, 323*8192, 324*8192, 325*8192, 326*8192, 327*8192, 328*8192, 329*8192, 330*8192, 331*8192, 332*8192, 333*8192, 334*8192, 335*8192,
	336*8192, 337*8192, 338*8192, 339*8192, 340*8192, 341*8192, 342*8192, 343*8192, 344*8192, 345*8192, 346*8192, 347*8192, 348*8192, 349*8192, 350*8192, 351*8192,
	352*8192, 353*8192, 354*8192, 355*8192, 356*8192, 357*8192, 358*8192, 359*8192, 360*8192, 361*8192, 362*8192, 363*8192, 364*8192, 365*8192, 366*8192, 367*8192,
	368*8192, 369*8192, 370*8192, 371*8192, 372*8192, 373*8192, 374*8192, 375*8192, 376*8192, 377*8192, 378*8192, 379*8192, 380*8192, 381*8192, 382*8192, 383*8192,
	384*8192, 385*8192, 386*8192, 387*8192, 388*8192, 389*8192, 390*8192, 391*8192, 392*8192, 393*8192, 394*8192, 395*8192, 396*8192, 397*8192, 398*8192, 399*8192,
	400*8192, 401*8192, 402*8192, 403*8192, 404*8192, 405*8192, 406*8192, 407*8192, 408*8192, 409*8192, 410*8192, 411*8192, 412*8192, 413*8192, 414*8192, 415*8192,
	416*8192, 417*8192, 418*8192, 419*8192, 420*8192, 421*8192, 422*8192, 423*8192, 424*8192, 425*8192, 426*8192, 427*8192, 428*8192, 429*8192, 430*8192, 431*8192,
	432*8192, 433*8192, 434*8192, 435*8192, 436*8192, 437*8192, 438*8192, 439*8192, 440*8192, 441*8192, 442*8192, 443*8192, 444*8192, 445*8192, 446*8192, 447*8192,
	448*8192, 449*8192, 450*8192, 451*8192, 452*8192, 453*8192, 454*8192, 455*8192, 456*8192, 457*8192, 458*8192, 459*8192, 460*8192, 461*8192, 462*8192, 463*8192,
	464*8192, 465*8192, 466*8192, 467*8192, 468*8192, 469*8192, 470*8192, 471*8192, 472*8192, 473*8192, 474*8192, 475*8192, 476*8192, 477*8192, 478*8192, 479*8192,
	480*8192, 481*8192, 482*8192, 483*8192, 484*8192, 485*8192, 486*8192, 487*8192, 488*8192, 489*8192, 490*8192, 491*8192, 492*8192, 493*8192, 494*8192, 495*8192,
	496*8192, 497*8192, 498*8192, 499*8192, 500*8192, 501*8192, 502*8192, 503*8192, 504*8192, 505*8192, 506*8192, 507*8192, 508*8192, 509*8192, 510*8192, 511*8192,
	512*8192, 513*8192, 514*8192, 515*8192, 516*8192, 517*8192, 518*8192, 519*8192, 520*8192, 521*8192, 522*8192, 523*8192, 524*8192, 525*8192, 526*8192, 527*8192,
	528*8192, 529*8192, 530*8192, 531*8192, 532*8192, 533*8192, 534*8192, 535*8192, 536*8192, 537*8192, 538*8192, 539*8192, 540*8192, 541*8192, 542*8192, 543*8192,
	544*8192, 545*8192, 546*8192, 547*8192, 548*8192, 549*8192, 550*8192, 551*8192, 552*8192, 553*8192, 554*8192, 555*8192, 556*8192, 557*8192, 558*8192, 559*8192,
	560*8192, 561*8192, 562*8192, 563*8192, 564*8192, 565*8192, 566*8192, 567*8192, 568*8192, 569*8192, 570*8192, 571*8192, 572*8192, 573*8192, 574*8192, 575*8192,
	576*8192, 577*8192, 578*8192, 579*8192, 580*8192, 581*8192, 582*8192, 583*8192, 584*8192, 585*8192, 586*8192, 587*8192, 588*8192, 589*8192, 590*8192, 591*8192,
	592*8192, 593*8192, 594*8192, 595*8192, 596*8192, 597*8192, 598*8192, 599*8192, 600*8192, 601*8192, 602*8192, 603*8192, 604*8192, 605*8192, 606*8192, 607*8192,
	608*8192, 609*8192, 610*8192, 611*8192, 612*8192, 613*8192, 614*8192, 615*8192, 616*8192, 617*8192, 618*8192, 619*8192, 620*8192, 621*8192, 622*8192, 623*8192,
	624*8192, 625*8192, 626*8192, 627*8192, 628*8192, 629*8192, 630*8192, 631*8192, 632*8192, 633*8192, 634*8192, 635*8192, 636*8192, 637*8192, 638*8192, 639*8192,
	640*8192, 641*8192, 642*8192, 643*8192, 644*8192, 645*8192, 646*8192, 647*8192, 648*8192, 649*8192, 650*8192, 651*8192, 652*8192, 653*8192, 654*8192, 655*8192,
	656*8192, 657*8192, 658*8192, 659*8192, 660*8192, 661*8192, 662*8192, 663*8192, 664*8192, 665*8192, 666*8192, 667*8192, 668*8192, 669*8192, 670*8192, 671*8192,
	672*8192, 673*8192, 674*8192, 675*8192, 676*8192, 677*8192, 678*8192, 679*8192, 680*8192, 681*8192, 682*8192, 683*8192, 684*8192, 685*8192, 686*8192, 687*8192,
	688*8192, 689*8192, 690*8192, 691*8192, 692*8192, 693*8192, 694*8192, 695*8192, 696*8192, 697*8192, 698*8192, 699*8192, 700*8192, 701*8192, 702*8192, 703*8192,
	704*8192, 705*8192, 706*8192, 707*8192, 708*8192, 709*8192, 710*8192, 711*8192, 712*8192, 713*8192, 714*8192, 715*8192, 716*8192, 717*8192, 718*8192, 719*8192,
	720*8192, 721*8192, 722*8192, 723*8192, 724*8192, 725*8192, 726*8192, 727*8192, 728*8192, 729*8192, 730*8192, 731*8192, 732*8192, 733*8192, 734*8192, 735*8192,
	736*8192, 737*8192, 738*8192, 739*8192, 740*8192, 741*8192, 742*8192, 743*8192, 744*8192, 745*8192, 746*8192, 747*8192, 748*8192, 749*8192, 750*8192, 751*8192,
	752*8192, 753*8192, 754*8192, 755*8192, 756*8192, 757*8192, 758*8192, 759*8192, 760*8192, 761*8192, 762*8192, 763*8192, 764*8192, 765*8192, 766*8192, 767*8192,
	768*8192, 769*8192, 770*8192, 771*8192, 772*8192, 773*8192, 774*8192, 775*8192, 776*8192, 777*8192, 778*8192, 779*8192, 780*8192, 781*8192, 782*8192, 783*8192,
	784*8192, 785*8192, 786*8192, 787*8192, 788*8192, 789*8192, 790*8192, 791*8192, 792*8192, 793*8192, 794*8192, 795*8192, 796*8192, 797*8192, 798*8192, 799*8192,
	800*8192, 801*8192, 802*8192, 803*8192, 804*8192, 805*8192, 806*8192, 807*8192, 808*8192, 809*8192, 810*8192, 811*8192, 812*8192, 813*8192, 814*8192, 815*8192,
	816*8192, 817*8192, 818*8192, 819*8192, 820*8192, 821*8192, 822*8192, 823*8192, 824*8192, 825*8192, 826*8192, 827*8192, 828*8192, 829*8192, 830*8192, 831*8192,
	832*8192, 833*8192, 834*8192, 835*8192, 836*8192, 837*8192, 838*8192, 839*8192, 840*8192, 841*8192, 842*8192, 843*8192, 844*8192, 845*8192, 846*8192, 847*8192,
	848*8192, 849*8192, 850*8192, 851*8192, 852*8192, 853*8192, 854*8192, 855*8192, 856*8192, 857*8192, 858*8192, 859*8192, 860*8192, 861*8192, 862*8192, 863*8192,
	864*8192, 865*8192, 866*8192, 867*8192, 868*8192, 869*8192, 870*8192, 871*8192, 872*8192, 873*8192, 874*8192, 875*8192, 876*8192, 877*8192, 878*8192, 879*8192,
	880*8192, 881*8192, 882*8192, 883*8192, 884*8192, 885*8192, 886*8192, 887*8192, 888*8192, 889*8192, 890*8192, 891*8192, 892*8192, 893*8192, 894*8192, 895*8192,
	896*8192, 897*8192, 898*8192, 899*8192, 900*8192, 901*8192, 902*8192, 903*8192, 904*8192, 905*8192, 906*8192, 907*8192, 908*8192, 909*8192, 910*8192, 911*8192,
	912*8192, 913*8192, 914*8192, 915*8192, 916*8192, 917*8192, 918*8192, 919*8192, 920*8192, 921*8192, 922*8192, 923*8192, 924*8192, 925*8192, 926*8192, 927*8192,
	928*8192, 929*8192, 930*8192, 931*8192, 932*8192, 933*8192, 934*8192, 935*8192, 936*8192, 937*8192, 938*8192, 939*8192, 940*8192, 941*8192, 942*8192, 943*8192,
	944*8192, 945*8192, 946*8192, 947*8192, 948*8192, 949*8192, 950*8192, 951*8192, 952*8192, 953*8192, 954*8192, 955*8192, 956*8192, 957*8192, 958*8192, 959*8192,
	960*8192, 961*8192, 962*8192, 963*8192, 964*8192, 965*8192, 966*8192, 967*8192, 968*8192, 969*8192, 970*8192, 971*8192, 972*8192, 973*8192, 974*8192, 975*8192,
	976*8192, 977*8192, 978*8192, 979*8192, 980*8192, 981*8192, 982*8192, 983*8192, 984*8192, 985*8192, 986*8192, 987*8192, 988*8192, 989*8192, 990*8192, 991*8192,
	992*8192, 993*8192, 994*8192, 995*8192, 996*8192, 997*8192, 998*8192, 999*8192, 1000*8192, 1001*8192, 1002*8192, 1003*8192, 1004*8192, 1005*8192, 1006*8192, 1007*8192,
	1008*8192, 1009*8192, 1010*8192, 1011*8192, 1012*8192, 1013*8192, 1014*8192, 1015*8192, 1016*8192, 1017*8192, 1018*8192, 1019*8192, 1020*8192, 1021*8192, 1022*8192, 1023*8192,
	},
	1024*8192
};
#endif
#endif

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &hng64_4_even_layout,     0x0, 0x100 }, /* scrolltiles */
	{ REGION_GFX1, 0, &hng64_4_odd_layout,     0x0, 0x100 }, /* scrolltiles */
	{ REGION_GFX1, 0, &hng64_layout,     0x0, 0x20 }, /* scrolltiles */
	{ REGION_GFX1, 0, &hng64_16_layout,  0x0, 0x20 }, /* scroll tiles */
	{ REGION_GFX2, 0, &hng64_4_16_layout,     0x0, 0x100 }, /* sprite tiles */
	{ REGION_GFX2, 0, &hng64_16_layout,  0x0, 0x20 }, /* sprite tiles */
#if LOADHNGTEXS
#if DECODETEXS
	{ REGION_GFX3, 0, &hng64_tex_layout, 0x0, 2 }, /* texture pages */
#endif
#endif
	{ -1 } /* end of array */
};

DRIVER_INIT( hng64 )
{
	hng64_soundram=auto_malloc(0x200000);
}

DRIVER_INIT(hng64_fght)
{
	no_machine_error_code=0x01010101;
	init_hng64();
}

DRIVER_INIT(hng64_race)
{
	no_machine_error_code=0x02020202;
	init_hng64();
}



/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	16384				/* data cache size */
};

static void irq_stop(int param)
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static INTERRUPT_GEN( irq_start )
{
	/* the irq 'level' is read in hng64_port_read */
	/* there are more, the sources are unknown at
       the moment */
	switch (cpu_getiloops())
	{
		case 0x00: hng64_interrupt_level_request = 0;
		break;
		case 0x01: hng64_interrupt_level_request = 1;
		break;
		case 0x02: hng64_interrupt_level_request = 2;
		break;
	}

	cpunum_set_input_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_USEC(50), 0, irq_stop);
}




MACHINE_INIT(hyperneo)
{
	FILE *fp;
	UINT8 *RAM = (UINT8*)hng64_soundram;
	memory_set_bankptr(1,&RAM[0x1e0000]);
	memory_set_bankptr(2,&RAM[0x001000]); // where..
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);

	/* HACK .. put ram here on reset .. we end up replacing it with the MMU hack later so the game doesn't clear its own ram with thecode in it!!..... */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000000, 0x0ffffff, 0, 0, hng64_mainram_w);

	// Aaaaaugh !!! AJG ugliness
	activeBuffer = 0 ;

	fp = fopen("dump.bin", "wb") ;

	// fwrite(memory_region(REGION_GFX4), sizeof(UINT8), 0x400000*3, fp) ;

	fclose(fp) ;
}

MACHINE_DRIVER_START( hng64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, MASTER_CLOCK/4)  /* actually R4300 ? */
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(hng_map, 0)
	MDRV_CPU_VBLANK_INT(irq_start,3)

	MDRV_CPU_ADD(V30,8000000)		 /* v53, 16? mhz! */
	MDRV_CPU_PROGRAM_MAP(hng_sound_map,0)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_MACHINE_INIT(hyperneo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(1024, 1024)
//  MDRV_VISIBLE_AREA(0, 1023, 0, 1023) // to see the whole textures
	MDRV_VISIBLE_AREA(0, 511, 16, 447)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(hng64)
	MDRV_VIDEO_UPDATE(hng64)
MACHINE_DRIVER_END

ROM_START( hng64 )
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) /* 512k for R4300 BIOS code */
	ROM_LOAD ( "brom1.bin", 0x000000, 0x080000,  CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) )
	ROM_REGION( 0x0100000, REGION_USER2, 0 ) /* unknown / unused bios roms */
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) )
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) )
ROM_END

/* roads edge might need a different bios (driving board bios?) */
ROM_START( roadedge )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	/* BIOS */
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) /* 512k for R4300 BIOS code */
	ROM_LOAD ( "brom1.bin", 0x000000, 0x080000,  CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) )
	ROM_REGION( 0x0100000, REGION_USER2, ROMREGION_DISPOSE ) /* unknown / unused bios roms */
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) )
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) )
	/* END BIOS */

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* Program Code, mapped at ??? maybe banked?  LE? */
	ROM_LOAD32_WORD( "001pr01b.81", 0x0000000, 0x400000, CRC(effbac30) SHA1(c1bddf3e511a8950f65ac7e452f81dbc4b7fd977) )
	ROM_LOAD32_WORD( "001pr02b.82", 0x0000002, 0x400000, CRC(b9aa4ad3) SHA1(9ab3c896dbdc45560b7127486e2db6ca3b15a057) )

	/* Scroll Characters 8x8x8 / 16x16x8 */
	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "001sc01a.41", 0x0000000, 0x400000, CRC(084395a1) SHA1(8bfea8fd3981fd45dcc04bd74840a5948aaf06a8) )
	ROM_LOAD32_BYTE( "001sc02a.42", 0x0000001, 0x400000, CRC(51dd19e3) SHA1(eeb3634294a049a357a75ee00aa9fce65b737395) )
	ROM_LOAD32_BYTE( "001sc03a.43", 0x0000002, 0x400000, CRC(0b6f3e19) SHA1(3b6dfd0f0633b0d8b629815920edfa39d92336bf) )
	ROM_LOAD32_BYTE( "001sc04a.44", 0x0000003, 0x400000, CRC(256c8c1c) SHA1(85935eea3722ec92f8d922f527c2e049c4185aa3) )

	/* Sprite Characters - 8x8x8 / 16x16x8 */
	ROM_REGION( 0x1000000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "001sp01a.53",0x0000000, 0x400000, CRC(7a469453) SHA1(3738ca76f538243bb23ffd23a42b2a0558882889) )
	ROM_LOAD32_BYTE( "001sp02a.54",0x0000001, 0x400000, CRC(6b9a3de0) SHA1(464c652f7b193326e3a871dfe751dd83c14284eb) )
	ROM_LOAD32_BYTE( "001sp03a.55",0x0000002, 0x400000, CRC(efbbd391) SHA1(7447c481ba6f9ba154d48a4b160dd24157891d35) )
	ROM_LOAD32_BYTE( "001sp04a.56",0x0000003, 0x400000, CRC(1a0eb173) SHA1(a69b786a9957197d1cc950ab046c57c18ca07ea7) )

#if LOADHNGTEXS
	/* Textures - 1024x1024x8 pages */
	ROM_REGION( 0x1000000, REGION_GFX3, ROMREGION_DISPOSE )
	/* note: same roms are at different positions on the board, repeated a total of 4 times*/
	ROM_LOAD( "001tx01a.1", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.2", 0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.3", 0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.4", 0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )
	ROM_LOAD( "001tx01a.5", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.6", 0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.7", 0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.8", 0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )
	ROM_LOAD( "001tx01a.9", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.10",0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.11",0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.12",0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )
	ROM_LOAD( "001tx01a.13",0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.14",0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.15",0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.16",0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )

	/* X,Y,Z Vertex ROMs */
	ROM_REGION( 0x0c00000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "001vt01a.17", 0x0000000, 0x400000, CRC(1a748e1b) SHA1(376d40baa3b94890d4740045d053faf208fe43db) )
	ROM_LOAD( "001vt02a.18", 0x0400000, 0x400000, CRC(449f94d0) SHA1(2228690532d82d2661285aeb4260689b027597cb) )
	ROM_LOAD( "001vt03a.19", 0x0800000, 0x400000, CRC(50ac8639) SHA1(dd2d3689466990a7c479bb8f11bd930ea45e47b5) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_DISPOSE ) /* Sound Samples? */
	ROM_LOAD( "001sd01a.77", 0x0000000, 0x400000, CRC(a851da99) SHA1(2ba24feddafc5fadec155cdb7af305fdffcf6690) )
	ROM_LOAD( "001sd02a.78", 0x0400000, 0x400000, CRC(ca5cec15) SHA1(05e91a602728a048d61bf86aa8d43bb4186aeac1) )
#endif
ROM_END


ROM_START( sams64_2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	/* BIOS */
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) /* 512k for R4300 BIOS code */
	ROM_LOAD ( "brom1.bin", 0x000000, 0x080000,  CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) )
	ROM_REGION( 0x0100000, REGION_USER2, ROMREGION_DISPOSE ) /* unknown / unused bios roms */
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) )
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) )
	/* END BIOS */

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* Program Code, mapped at ??? maybe banked?  LE? */
	ROM_LOAD32_WORD( "005pr01a.81", 0x0000000, 0x400000, CRC(a69d7700) SHA1(a580783a109bc3e24248d70bcd67f62dd7d8a5dd) )
	ROM_LOAD32_WORD( "005pr02a.82", 0x0000002, 0x400000, CRC(38b9e6b3) SHA1(d1dad8247d920cc66854a0096e1c7845842d2e1c) )
	ROM_LOAD32_WORD( "005pr03a.83", 0x0800000, 0x400000, CRC(0bc738a8) SHA1(79893b0e1c4a31e02ab385c4382684245975ae8f) )
	ROM_LOAD32_WORD( "005pr04a.84", 0x0800002, 0x400000, CRC(6b504852) SHA1(fcdcab432162542d249818a6cd15b8f2e8230f97) )
	ROM_LOAD32_WORD( "005pr05a.85", 0x1000000, 0x400000, CRC(32a743d3) SHA1(4088b930a1a4d6224a0939ef3942af1bf605cdb5) )
	ROM_LOAD32_WORD( "005pr06a.86", 0x1000002, 0x400000, CRC(c09fa615) SHA1(697d6769c16b3c8f73a6df4a1e268ec40cb30d51) )
	ROM_LOAD32_WORD( "005pr07a.87", 0x1800000, 0x400000, CRC(44286ad3) SHA1(1f890c74c0da0d34940a880468e68f7fb1417813) )
	ROM_LOAD32_WORD( "005pr08a.88", 0x1800002, 0x400000, CRC(d094eb67) SHA1(3edc8d608c631a05223e1d05157cd3daf2d6597a) )

	/* Scroll Characters 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "005sc01a.97",  0x0000000, 0x800000, CRC(7f11cda9) SHA1(5fbdabd8423e9723a6ec38f8503e6ca7f4f69fdd) )
	ROM_LOAD32_BYTE( "005sc02a.99",  0x0000001, 0x800000, CRC(87d1e1a7) SHA1(00f2ef46ce64ab715add8cd47745c57944286f81) )
	ROM_LOAD32_BYTE( "005sc03a.101", 0x0000002, 0x800000, CRC(a5d4c535) SHA1(089a3cd07701f025024ce73b7b4d38063c33a59f) )
	ROM_LOAD32_BYTE( "005sc04a.103", 0x0000003, 0x800000, CRC(14930d77) SHA1(b4c613a8896e21fe2cac0595dd1ea30dc7fce0bd) )
	ROM_LOAD32_BYTE( "005sc05a.98",  0x2000000, 0x800000, CRC(4475a3f8) SHA1(f099baf766ee00d166cfa8402baa0b6ea25a0010) )
	ROM_LOAD32_BYTE( "005sc06a.100", 0x2000001, 0x800000, CRC(41c0fbbd) SHA1(1d9ac01c9499a6202ee59d15d498ec34edc05888) )
	ROM_LOAD32_BYTE( "005sc07a.102", 0x2000002, 0x800000, CRC(3505b198) SHA1(2fdfdd5a1f6f31f5fb1c0af70047108d1df44af2) )
	ROM_LOAD32_BYTE( "005sc08a.104", 0x2000003, 0x800000, CRC(3139e413) SHA1(38210541379ddeba8c0b9ef8fa5430c0090db7c7) )

	/* Sprite Characters - 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "005sp01a.105",0x0000000, 0x800000, CRC(68eefee5) SHA1(d95bd7b549900500633af07544423b0062ac07ce) )
	ROM_LOAD32_BYTE( "005sp02a.109",0x0000001, 0x800000, CRC(5d9a49b9) SHA1(50768c496a3e0b4379e121349f32edec4f18652f) )
	ROM_LOAD32_BYTE( "005sp03a.113",0x0000002, 0x800000, CRC(9b6530fe) SHA1(398433b98578a6b4b950afc4d6318916376e0760) )
	ROM_LOAD32_BYTE( "005sp04a.117",0x0000003, 0x800000, CRC(d4e422ce) SHA1(9bfaa533ab3d014cdb0c535cf6952e01925cc30b) )
	ROM_LOAD32_BYTE( "005sp05a.106",0x2000000, 0x400000, CRC(d8b1fb26) SHA1(7da767d8e817c52afc416ccfe8caf30f66c233ef) )
	ROM_LOAD32_BYTE( "005sp06a.110",0x2000001, 0x400000, CRC(87ed72a0) SHA1(0d7db4dc9f15a0377a83f020ffbe81621ca77cff) )
	ROM_LOAD32_BYTE( "005sp07a.114",0x2000002, 0x400000, CRC(8eb3c173) SHA1(d5763c19a3e2fd93f7784d957e7401c9152c40de) )
	ROM_LOAD32_BYTE( "005sp08a.118",0x2000003, 0x400000, CRC(05486fbc) SHA1(747d9ae03ce999be4ab697753e93c90ea85b7d44) )

#if LOADHNGTEXS
	/* Textures - 1024x1024x8 pages */
	ROM_REGION( 0x1000000, REGION_GFX3, ROMREGION_DISPOSE )
	/* note: same roms are at different positions on the board, repeated a total of 4 times*/
	ROM_LOAD( "005tx01a.1", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.2", 0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.3", 0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.4", 0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )
	ROM_LOAD( "005tx01a.5", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.6", 0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.7", 0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.8", 0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )
	ROM_LOAD( "005tx01a.9", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.10",0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.11",0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.12",0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )
	ROM_LOAD( "005tx01a.13",0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.14",0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.15",0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.16",0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )

	/* X,Y,Z Vertex ROMs */
	ROM_REGION( 0x1800000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "005vt01a.17", 0x0000000, 0x400000, CRC(48a61479) SHA1(ef982b1ecc6dfca2ad989391afcc1b3d1e7fe652) )
	ROM_LOAD( "005vt02a.18", 0x0400000, 0x400000, CRC(ba9100c8) SHA1(f7704fb8e5310ea7d0e6ae6b8935717ec9119b6d) )
	ROM_LOAD( "005vt03a.19", 0x0800000, 0x400000, CRC(f54a28de) SHA1(c445cf7fee71a516065cf37e05b898208f48b17e) )
	ROM_LOAD( "005vt04a.20", 0x0c00000, 0x400000, CRC(57ad79c7) SHA1(bc382317323c1f8a31b69ae3100d3bba6b5d0838) )
	ROM_LOAD( "005vt05a.21", 0x1000000, 0x400000, CRC(49c82bec) SHA1(09255279edb9a204bbe1cce8cef58d5c81e86d1f) )
	ROM_LOAD( "005vt06a.22", 0x1400000, 0x400000, CRC(7ba05b6c) SHA1(729c1d182d74998dd904b587a2405f55af9825e0) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_DISPOSE ) /* Sound Samples? */
	ROM_LOAD( "005sd01a.77", 0x0000000, 0x400000, CRC(8f68150f) SHA1(a1e5efdfd1ed29f81e25c8da669851ddb7b0c826) )
	ROM_LOAD( "005sd02a.78", 0x0400000, 0x400000, CRC(6b4da6a0) SHA1(8606c413c129635bdaaa37254edbfd19b10426bb) )
	ROM_LOAD( "005sd03a.79", 0x0800000, 0x400000, CRC(a529fab3) SHA1(8559d402c8f66f638590b8b57ec9efa775010c96) )
	ROM_LOAD( "005sd04a.80", 0x0800000, 0x400000, CRC(dca95ead) SHA1(39afdfba0e5262b524f25706a96be00e5d14548e) )
#endif
ROM_END


ROM_START( fatfurwa )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	/* BIOS */
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) /* 512k for R4300 BIOS code */
	ROM_LOAD ( "brom1.bin", 0x000000, 0x080000,  CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) )
	ROM_REGION( 0x0100000, REGION_USER2, ROMREGION_DISPOSE ) /* unknown / unused bios roms */
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) )
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) )
	/* END BIOS */

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* Program Code, mapped at ??? maybe banked?  LE? */
	ROM_LOAD32_WORD( "006pr01a.81", 0x0000000, 0x400000, CRC(3830efa1) SHA1(9d8c941ccb6cbe8d138499cf9d335db4ac7a9ec0) )
	ROM_LOAD32_WORD( "006pr02a.82", 0x0000002, 0x400000, CRC(8d5de84e) SHA1(e3ae014263f370c2836f62ab323f1560cb3a9cf0) )
	ROM_LOAD32_WORD( "006pr03a.83", 0x0800000, 0x400000, CRC(c811b458) SHA1(7d94e0df501fb086b2e5cf08905d7a3adc2c6472) )
	ROM_LOAD32_WORD( "006pr04a.84", 0x0800002, 0x400000, CRC(de708d6c) SHA1(2c9848e7bbf61c574370f9ecab5f5a6ba63339fd) )

	/* Scroll Characters 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "006sc01a.97", 0x0000000, 0x800000, CRC(f13dffad) SHA1(86363aeae176fd4204e446c13a028da919dc2069) )
	ROM_LOAD32_BYTE( "006sc02a.99", 0x0000001, 0x800000, CRC(be79d42a) SHA1(f3eb950a62e2df1de116af9434027439f1305e1f) )
	ROM_LOAD32_BYTE( "006sc03a.101",0x0000002, 0x800000, CRC(16918b73) SHA1(ad0c751a301fe3c95fca19473869dfd55fb6b0de) )
	ROM_LOAD32_BYTE( "006sc04a.103",0x0000003, 0x800000, CRC(9b63cd98) SHA1(62519a3a531c4493a5a85dc01ca69413977120ca) )
	ROM_LOAD32_BYTE( "006sc05a.98", 0x2000000, 0x800000, CRC(0487297b) SHA1(d3fa4d691559327739c96717312faf09b498001d) )
	ROM_LOAD32_BYTE( "006sc06a.100",0x2000001, 0x800000, CRC(34a76c31) SHA1(be05dc75afb7cde65ba5d29c0e66a7b1b62c41cb) )
	ROM_LOAD32_BYTE( "006sc07a.102",0x2000002, 0x800000, CRC(7a1c371e) SHA1(1cd4ad66dd007adc9ab0c29720cbf9955c7337e0) )
	ROM_LOAD32_BYTE( "006sc08a.104",0x2000003, 0x800000, CRC(88232ade) SHA1(4ae2a572c3525087f77c95185e8697a1fc720512) )

	/* Sprite Characters - 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "006sp01a.105",0x0000000, 0x800000, CRC(087b8c49) SHA1(bb1eb2baef7da91f904bf45414f21dd6bac30749) )
	ROM_LOAD32_BYTE( "006sp02a.109",0x0000001, 0x800000, CRC(da28631e) SHA1(ea7e2d9195cfa4f954f4d542296eec1323223653) )
	ROM_LOAD32_BYTE( "006sp03a.113",0x0000002, 0x800000, CRC(bb87b55b) SHA1(8644ebb356ae158244a6e03254b0212cb359e167) )
	ROM_LOAD32_BYTE( "006sp04a.117",0x0000003, 0x800000, CRC(2367a536) SHA1(304b5b7f7e5d41e69fbd4ac2a938c42f3766630e) )
	ROM_LOAD32_BYTE( "006sp05a.106",0x2000000, 0x800000, CRC(0eb8fd06) SHA1(c2b6fab1b0104910d7bb39d0a496ada39c5cc122) )
	ROM_LOAD32_BYTE( "006sp06a.110",0x2000001, 0x800000, CRC(dccc3f75) SHA1(fef8d259c17a78e2266fed965fba1e15f1cd01dd) )
	ROM_LOAD32_BYTE( "006sp07a.114",0x2000002, 0x800000, CRC(cd7baa1b) SHA1(4084f3a73aae623d69bd9de87cecf4a33b628b7f) )
	ROM_LOAD32_BYTE( "006sp08a.118",0x2000003, 0x800000, CRC(9c3044ac) SHA1(24b28bcc6be51ab3ff59c2894094cd03ec377d84) )

#if LOADHNGTEXS
	/* Textures - 1024x1024x8 pages */
	ROM_REGION( 0x1000000, REGION_GFX3, ROMREGION_DISPOSE )
	/* note: same roms are at different positions on the board, repeated a total of 4 times*/
	ROM_LOAD( "006tx01a.1", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.2", 0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.3", 0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.4", 0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )
	ROM_LOAD( "006tx01a.5", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.6", 0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.7", 0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.8", 0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )
	ROM_LOAD( "006tx01a.9", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.10",0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.11",0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.12",0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )
	ROM_LOAD( "006tx01a.13",0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.14",0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.15",0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.16",0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )

	/* X,Y,Z Vertex ROMs */
	ROM_REGION16_BE( 0x0c00000, REGION_GFX4, ROMREGION_NODISPOSE )
	ROMX_LOAD( "006vt01a.17", 0x0000000, 0x400000, CRC(5c20ed4c) SHA1(df679f518292d70b9f23d2bddabf975d56b96910), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "006vt02a.18", 0x0000002, 0x400000, CRC(150eb717) SHA1(9acb067346eb386256047c0f1d24dc8fcc2118ca), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "006vt03a.19", 0x0000004, 0x400000, CRC(021cfcaf) SHA1(fb8b5f50d3490b31f0a4c3e6d3ae1b98bae41c97), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_DISPOSE ) /* Sound Samples? */
	ROM_LOAD( "006sd01a.77", 0x0000000, 0x400000, CRC(790efb6d) SHA1(23ddd3ee8ae808e58cbcaf92a9ef56d3ca6289b5) )
	ROM_LOAD( "006sd02a.78", 0x0400000, 0x400000, CRC(f7f020c7) SHA1(b72fde4ff6384b80166a3cb67d31bf7afda750bc) )
	ROM_LOAD( "006sd03a.79", 0x0800000, 0x400000, CRC(1a678084) SHA1(f52efb6145102d289f332d8341d89a5d231ba003) )
	ROM_LOAD( "006sd04a.80", 0x0800000, 0x400000, CRC(3c280a5c) SHA1(9d3fc78e18de45382878268db47ff9d9716f1505) )
#endif
ROM_END

ROM_START( buriki )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* dummy region for R4300 */

	/* BIOS */
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) /* 512k for R4300 BIOS code */
	ROM_LOAD ( "brom1.bin", 0x000000, 0x080000,  CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) )
	ROM_REGION( 0x0100000, REGION_USER2, ROMREGION_DISPOSE ) /* unknown / unused bios roms */
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) )
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) )
	/* END BIOS */

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* Program Code, mapped at ??? maybe banked?  LE? */
	ROM_LOAD32_WORD( "007pr01b.81", 0x0000000, 0x400000, CRC(a31202f5) SHA1(c657729b292d394ced021a0201a1c5608a7118ba) )
	ROM_LOAD32_WORD( "007pr02b.82", 0x0000002, 0x400000, CRC(a563fed6) SHA1(9af9a021beb814e35df968abe5a99225a124b5eb) )
	ROM_LOAD32_WORD( "007pr03a.83", 0x0800000, 0x400000, CRC(da5f6105) SHA1(5424cf5289cef66e301e968b4394e551918fe99b) )
	ROM_LOAD32_WORD( "007pr04a.84", 0x0800002, 0x400000, CRC(befc7bce) SHA1(83d9ecf944e03a40cf25ee288077c2265d6a588a) )
	ROM_LOAD32_WORD( "007pr05a.85", 0x1000000, 0x400000, CRC(013e28bc) SHA1(45e5ac45b42b26957c2877ac1042472c4b5ec914) )
	ROM_LOAD32_WORD( "007pr06a.86", 0x1000002, 0x400000, CRC(0620fccc) SHA1(e0bffc56b019c79276a4ef5ec7354edda15b0889) )

	/* Scroll Characters 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "007sc01a.97", 0x0000000, 0x800000, CRC(4e8300db) SHA1(f1c9e6fddc10efc8f2a530027cca062f48b8c8d4) )
	ROM_LOAD32_BYTE( "007sc02a.99", 0x0000001, 0x800000, CRC(d5855944) SHA1(019c0bd2f8de7ffddd53df6581b40940262f0053) )
	ROM_LOAD32_BYTE( "007sc03a.101",0x0000002, 0x800000, CRC(ff45c9b5) SHA1(ddcc2a10ccac62eb1f3671172ad1a4d163714fca) )
	ROM_LOAD32_BYTE( "007sc04a.103",0x0000003, 0x800000, CRC(e4cb59e9) SHA1(4e07ff374890217466a53d5bfb1fa99eb7402360) )
	ROM_LOAD32_BYTE( "007sc05a.98", 0x2000000, 0x400000, CRC(27f848c1) SHA1(2ee9cca4e68e56c7c17c8e2d7e0f55a34a5960bd) )
	ROM_LOAD32_BYTE( "007sc06a.100",0x2000001, 0x400000, CRC(c39e9b4c) SHA1(3c8a0494c2a6866ecc0df2c551619c57ee072440) )
	ROM_LOAD32_BYTE( "007sc07a.102",0x2000002, 0x400000, CRC(753e7e3d) SHA1(39b2e9fd23878d8fc4f98fe88b466e963d8fc959) )
	ROM_LOAD32_BYTE( "007sc08a.104",0x2000003, 0x400000, CRC(b605928e) SHA1(558042b84115273fa581606daafba0e9688fa002) )

	/* Sprite Characters - 8x8x8 / 16x16x8 */
	ROM_REGION( 0x4000000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "007sp01a.105",0x0000000, 0x800000, CRC(160acae6) SHA1(37c15e1d2544ec6f3b61d06200345d6abdd28edf) )
	ROM_LOAD32_BYTE( "007sp02a.109",0x0000001, 0x800000, CRC(1a55331d) SHA1(0b03d5c7312e01874365b31f1ff3d9766abd00f1) )
	ROM_LOAD32_BYTE( "007sp03a.113",0x0000002, 0x800000, CRC(3f308444) SHA1(0acd52312c15a2ed3bacf60a2fd820cb09ebbb55) )
	ROM_LOAD32_BYTE( "007sp04a.117",0x0000003, 0x800000, CRC(6b81aa51) SHA1(55f7702e1d7a2bef7f050d0358de9036a0139877) )
	ROM_LOAD32_BYTE( "007sp05a.106",0x2000000, 0x400000, CRC(32d2fa41) SHA1(b16a0bbd397be2a8d532c85951b924e2e086a189) )
	ROM_LOAD32_BYTE( "007sp06a.110",0x2000001, 0x400000, CRC(b6f8d7f3) SHA1(70ce94f2193ee39218022da617413c42f6753574) )
	ROM_LOAD32_BYTE( "007sp07a.114",0x2000002, 0x400000, CRC(5caa1cc9) SHA1(3e40b10ea3bcf1239d0015da4be869632b805ddd) )
	ROM_LOAD32_BYTE( "007sp08a.118",0x2000003, 0x400000, CRC(7a158c67) SHA1(d66f4920a513208d45b908a1934d9afb894debf1) )

#if LOADHNGTEXS
	/* Textures - 1024x1024x8 pages */
	ROM_REGION( 0x1000000, REGION_GFX3, ROMREGION_DISPOSE )
	/* note: same roms are at different positions on the board, repeated a total of 4 times*/
	ROM_LOAD( "007tx01a.1", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.2", 0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.3", 0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.4", 0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )
	ROM_LOAD( "007tx01a.5", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.6", 0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.7", 0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.8", 0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )
	ROM_LOAD( "007tx01a.9", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.10",0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.11",0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.12",0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )
	ROM_LOAD( "007tx01a.13",0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.14",0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.15",0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.16",0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )

	/* X,Y,Z Vertex ROMs */
	ROM_REGION16_BE( 0x0c00000, REGION_GFX4, ROMREGION_NODISPOSE )
	ROMX_LOAD( "007vt01a.17", 0x0000000, 0x400000, CRC(f78a0376) SHA1(fde4ddd4bf326ae5f1ed10311c237b13b62e060c), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "007vt02a.18", 0x0000002, 0x400000, CRC(f365f608) SHA1(035fd9b829b7720c4aee6fdf204c080e6157994f), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "007vt03a.19", 0x0000004, 0x400000, CRC(ba05654d) SHA1(b7fe532732c0af7860c8eded3c5abd304d74e08e), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_DISPOSE ) /* Sound Samples? */
	ROM_LOAD( "007sd01a.77", 0x0000000, 0x400000, CRC(1afb48c6) SHA1(b072d4fe72d6c5267864818d300b32e85b426213) )
	ROM_LOAD( "007sd02a.78", 0x0400000, 0x400000, CRC(c65f1dd5) SHA1(7f504c585a10c1090dbd1ac31a3a0db920c992a0) )
	ROM_LOAD( "007sd03a.79", 0x0800000, 0x400000, CRC(356f25c8) SHA1(5250865900894232960686f40c5da35b3868b78c) )
	ROM_LOAD( "007sd04a.80", 0x0800000, 0x400000, CRC(dabfbbad) SHA1(7d58d5181705618e0e2d69c6fdb81b9b3d2b9e0f) )
#endif
ROM_END

/* Bios */
GAMEX( 1997, hng64,  0,        hng64, hng64, 0,     ROT0, "SNK", "Hyper NeoGeo 64 Bios", GAME_NOT_WORKING|GAME_NO_SOUND|NOT_A_DRIVER )

/* Games */
GAMEX( 1997, roadedge, hng64,  hng64, hng64, hng64_race, ROT0, "SNK", "Roads Edge / Round Trip (rev.B)", GAME_NOT_WORKING|GAME_NO_SOUND ) /* 001 */
/* 002 Samurai Shodown 64 */
/* 003 Xtreme Rally / Offbeat Racer */
/* 004 Beast Busters 2nd Nightmare */
GAMEX( 1998, sams64_2, hng64,  hng64, hng64, hng64_fght, ROT0, "SNK", "Samurai Shodown: Warrior's Rage", GAME_NOT_WORKING|GAME_NO_SOUND ) /* 005 */
GAMEX( 1998, fatfurwa, hng64,  hng64, hng64, hng64_fght, ROT0, "SNK", "Fatal Fury: Wild Ambition (rev.A)", GAME_NOT_WORKING|GAME_NO_SOUND ) /* 006 */
GAMEX( 1999, buriki,   hng64,  hng64, hng64, hng64_fght, ROT0, "SNK", "Buriki One (rev.B)", GAME_NOT_WORKING|GAME_NO_SOUND ) /* 007 */
