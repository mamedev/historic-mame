/***************************************************************************
                      Emulated
                         |
                  board #|year   CPU      tiles        sprites  priority other (unknown)
                    -----|---- ------ ------------- ------------- ------ ------
BAW                 GX604 1987
Combat School       GX611*1987
Flak Attack         GX669 1987
Devil World         GX687 1987
Kitten Kaboodle     GX712 1988
Chequered Flag      GX717 1988
Fast Lane           GX752 1987
Hot Chase           GX763 1988
Rack 'Em Up         GX765 1987
Haunted Castle      GX768*1988 052001                                    007121(x2)
Ajax / Typhoon      GX770*1987  6309+ 052109 051962 051960 051937        051346
                               052001
Labyrinth Runner    GX771 1987  6309+                                    007121
                               051733
Super Contra        GX775 1987 052001 052109 051962 051960 051937        007327
Battlantis          GX777 1987
Vulcan Venture      GX785 1988
City Bomber         GX787 1987
Over Drive          GX789 1990
Hyper Crash         GX790 1987
Blades of Steel     GX797 1987
The Main Event      GX799*1988   6309 051961 051962 051960 051937  PROM
Missing in Action   GX808*1989  68000 052109 051962 051960 051937  PROM
Crime Fighters      GX821*1989 052526 052109 051962 051960 051967
Special Project Y   GX857 1989
Konami '88          GX861 1988 052001 052109 051962 051960 051937
Final Round         GX870 1988
Thunder Cross       GX873 1988 052001
Aliens              GX875*1990 052526 052109 051962 051960 051937
Gangbusters         GX878 1988
Devastators         GX890*1988  6309+ 052109 051962 051960 051937  PROM
                               051733
Cue Brick           GX903 1989
Punk Shot           GX907*1990  68000 052109 051962 051960 051937 053251?
Surprise Attack     GX911 1990
Lightning Fighters /
  Trigon            GX939*1990  68000 052109 051962 053245 053244 053251
Gradius 3           GX945 1989
Parodius            GX955 1990 053248 052109 051962 053245 053244 053251
TMNT                GX963*1989  68000 052109 051962 051960 051937  PROM
Block Hole          GX973 1989 052526 052109 051962 051960 051937
                            or 052001?
Rollergames         GX999 1991
Bells & Whistles    GX060 1991
Golfing Greats      GX061 1991 68000+ 052109 051962 053245 053244 053251
                               053936
TMNT 2              GX063*1991  68000 052109 051962 053245a053244a053251 053990
Sunset Riders       GX064*1991  68000 052109 051962 053245 053244 053251 054358
X-Men               GX065*1992  68000 052109 051962 053247 053246 053251
XEXEX               GX067 1991        053157 053156 053247 053246a
Asterix             GX068 1992  68000 054157 054156 053245 053244 053251 054358
G.I. Joe            GX069 1992  68000 053157 054156 053247a053246a053251 054539
The Simpsons        GX072*1991 053248 052109 051962 053247 053246 053251
Thunder Cross 2     GX073 1991
Vendetta /
  Crime Fighters 2  GX081*1991 052548 052109 051962 053247 053246 053251
Premier Soccer      GX101 1993
Hexion              GX122 1992
Entapous /
  Gaiapolis         GX123 1993  68000 054157 054156 055673 053246 053252 054000 055555
Mystic Warrior      GX128 1993
Cowboys of Moo Mesa GX151 1993  68000 054157 054156 053247a053246a053252 054338 053990
Violent Storm       GX168 1993  68000 054157 054156 055673 053246a       054338 054539(x2) 055550 055555
Bucky 'O Hare       GX173 1992  68000 054157 054156 053247a053246a053251 054338 054539
Potrio              GX174 1992
Lethal Enforcers    GX191 1992
Metamorphic Force   GX224 1993
Run and Gun         GX247 1993  68000               055673 053246a       053253(x2)
Martial Champion    GX234 1993  68000 053157 053156 055673 053246a       054338 054539 055555 053990
Polynet Comm.       GX305 1993

Notes:
the 051961 is an earlier version of the 052109, functionally equivalent
the 051967 seems to be equivalent to the 051937


Status of the ROM tests in the emulated games:

Ajax / Typhoon      fails F4/H4 (extra graphics, 051346?)
The Main Event      pass
Missing in Action   pass
Crime Fighters      pass
Aliens              pass
Devastators         pass
Punk Shot           pass
Lightning Fighters  fails K2/K8 (053244)
TMNT                pass
TMNT 2              fails 3L/3K/7L/7K (053244) 1D (053260, bad?)
Sunset Riders       fails 3L/7L (053244)
X-Men               fails 2H/2L/1H/1L (053246) 1F (054544)
The Simpsons        fails 3N/8N/12N/16N (053246)
Vendetta            fails 3K/8L/12M/16K (053246)



THE FOLLOWING INFORMATION IS PRELIMINARY AND INACCURATE. DON'T RELY ON IT.


052109/051962
-------------
These work in pair.
The 052109 manages 3 64x32 scrolling tilemaps with 8x8 characters, and
optionally generates timing clocks and interrupt signals. It uses 0x4000
bytes of RAM, and a variable amount of ROM. It cannot read the ROMs:
instead, it exports 21 bits (16 from the tilemap RAM + 3 for the character
raster line + 2 additional ones for ROM banking) and these are externally
used to generate the address of the required data on the ROM; the output of
the ROMs is sent to the 051962, along with a color code. In theory you could
have any combination of bits in the tilemap RAM, as long as they add to 16.
In practice, all the games supported so far standardize on the same format
which uses 3 bits for the color code and 13 bits for the character code.
The 051962 multiplexes the data of the three layers and converts it into
palette indexes and transparency bits which will be mixed later in the video
chain.
Priority is handled externally: these chips only generate the tilemaps, they
don't mix them.
Both chips are interfaced with the main CPU. When the RMRD pin is asserted,
the CPU can read the gfx ROM data. This is done by telling the 052109 which
dword to read (this is a combination of some banking registers, and the CPU
address lines), and then reading it from the 051962.

052109 inputs:
- address lines (AB0-AB15, AB13-AB15 seem to have a different function)
- data lines (DB0-DB7)
- misc interface stuff

052109 outputs:
- address lines for the private RAM (RA0-RA12)
- data lines for the private RAM (VD0-VD15)
- NMI, IRQ, FIRQ for the main CPU
- misc interface stuff
- ROM bank selector (CAB1-CAB2)
- character "code" (VC0-VC10)
- character "color" (COL0-COL7; only COL5-COL7 is really color in the standard setup)
- layer A horizontal scroll (ZA1H-ZA4H range 0..7)
- layer B horizontal scroll (ZB1H-ZB4H range 0..7)
- ????? (BEN)

051962 inputs:
- gfx data from the ROMs (VC0-VC31)
- color code (COL0-COL7; only COL5-COL7 used in the standard setup)
- layer A horizontal scroll (ZA1H-ZA4H range 0..7)
- layer B horizontal scroll (ZB1H-ZB4H range 0..7)
- let main CPU read the gfx ROMs (RMRD)
- address lines to be used with RMRD (AB0-AB1)
- data lines to be used with RMRD (DB0-DB7)
- ????? (BEN)
- misc interface stuff

051962 outputs:
- FIX layer palette index (DFI0-DFI7; DFI4 not used in the standard setup)
- FIX layer transparency (NFIC)
- A layer palette index (DSA0-DSAD; DSA4 and DSAA-DSAD not used in the standard setup)
- A layer transparency (NSAC)
- B layer palette index (DSB0-DSBD; DSB4 and DSBA-DSBD not used in the standard setup)
- B layer transparency (NSBC)
- misc interface stuff


052109 memory layout:
0000-07ff: layer FIX tilemap (attributes)
0800-0fff: layer A tilemap (attributes)
1000-1fff: layer B tilemap (attributes)
180c-1833: A y scroll
1a00-1bff: A x scroll
1c00     : ?
1c80     : row/column scroll control
		   ------xx layer A row scroll
		            00 = disabled
					01 = ?
					10 = 32 lines
					11 = 256 lines
		   -----x-- layer A column scroll
		            0 = disabled
					1 = 64 (actually 40) columns
		   ---xx--- layer B row scroll
		   --x----- layer B column scroll
1d00     : bits 0 & 1 might enable NMI and FIRQ, not sure
         : bit 2 = IRQ enable
1d80     : ROM bank selector bits 0-3 = bank 0 bits 4-7 = bank 1
1e00     : ROM subbank selector for ROM testing
1e80     : bit 0 = flip screen (applies to tilemaps only, not sprites)
         : bit 1 might enable interrupt generation, not sure
1f00     : ROM bank selector bits 0-3 = bank 2 bits 4-7 = bank 3
2000-27ff: layer FIX tilemap (code)
2800-2fff: layer A tilemap (code)
3000-37ff: layer B tilemap (code)
3800-3807: nothing here, so the chip can share address space with a 051937
380c-3833: B y scroll
3a00-3bff: B x scroll
3c00-3fff: nothing here, so the chip can share address space with a 051960
EXTRA ADDRESSING SPACE USED BY X-MEN:
4000-47ff: layer FIX tilemap (code high bits)
4800-4fff: layer A tilemap (code high bits)
5000-57ff: layer B tilemap (code high bits)

The main CPU doesn't have direct acces to the RAM used by the 052109, it has
to through the chip.



051960/051937
-------------

Sprite generators. Designed to work in pair. The 051960 manages the sprite
list and produces and address that is fed to the gfx ROMs. The data from the
ROMs is sent to the 051937, along with color code and other stuff from the
051960. The 051937 outputs up to 12 bits of palette index, plus "shadow" and
transparency information.
Both chips are interfaced to the main CPU, through 8-bit data buses and 11
bits of address space. The 051937 sits in the range 000-007, while the 051960
in the range 400-7ff (all RAM). The main CPU can read the gfx ROM data though
the 051937 data bus, while the 051960 provides the address lines.
The 051960 is designed to directly address 1MB of ROM space, since it produces
18 address lines that go to two 16-bit wide ROMs (the 051937 has a 32-bit data
bus to the ROMs). However, the addressing space can be increased by using one
or more of the "color attribute" bits of the sprites as bank selectors.
Moreover a few games store the gfx data in the ROMs in a format different from
the one expected by the 051960, and use external logic to reorder the address
lines.
The 051960 can also genenrate IRQ, FIRQ and NMI signals.

memory map:
000     W  bit 3 = flip screen (applies to sprites only, not tilemaps)
		   bit 4 used by Devastators and TMNT, unknown
		   bit 5 = enable gfx ROM reading
002-003 W  selects the portion of the gfx ROMs to be read.
004-007 R  reads data from the gfx ROMs (32 bits in total). The address of the
           data is determined by the register above and by the last address
		   accessed on the 051960; plus bank switch bits for larger ROMs.
400-7ff RW sprite RAM, 8 bytes per sprite



053251
------
Priority encoder.

The chip has inputs for 5 layers (CI0-CI4); only 4 are used (CI1-CI4)
CI0-CI2 are 9(=5+4) bits inputs, CI3-CI4 8(=4+4) bits

The input connctions change from game to game. E.g. in Simpsons,
CI0 = grounded (background color)
CI1 = sprites
CI2 = FIX
CI3 = A
CI4 = B

in lgtnfght:
CI0 = grounded
CI1 = sprites
CI2 = FIX
CI3 = B
CI4 = A

there are three 6 bit priority inputs, PR0-PR2

simpsons:
PR0 = 111111
PR1 = xxxxx0 x bits coming from the sprite attributes
PR2 = 111111

lgtnfght:
PR0 = 111111
PR1 = 1xx000 x bits coming from the sprite attributes
PR2 = 111111

also two shadow inputs, SDI0 and SDI1 (from the sprite attributes)

the chip outputs the 11 bit palette index, CO0-CO10, and two shadow bits.

16 internal registers; registers are 6 bits wide (input is D0-D5)
For the most part, their meaning is unknown
All registers are write only.
There must be a way to enable/disable the three external PR inputs.

 0  priority of CI0 (higher = lower priority)
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 3f = 111111
    xmen:     05 = 000101  default value
    xmen:     09 = 001001  used to swap CI0 and CI2
 1  priority of CI1 (higher = lower priority)
    punkshot: 28 = 101000
    lgtnfght: unused?
    simpsons: unused?
    xmen:     02 = 000010
 2  priority of CI2 (higher = lower priority)
    punkshot: 24 = 100100
    lgtnfght: 24 = 100100
    simpsons: 04 = 000100
    xmen:     09 = 001001  default value
    xmen:     05 = 000101  used to swap CI0 and CI2
 3  priority of CI3 (higher = lower priority)
    punkshot: 34 = 110100
    lgtnfght: 34 = 110100
    simpsons: 28 = 101000
    xmen:     00 = 000000
 4  priority of CI4 (higher = lower priority)
    punkshot: 2c = 101100  default value
    punkshot: 3c = 111100  used to swap CI3 and CI4
    punkshot: 26 = 100110  used to swap CI1 and CI4
    lgtnfght: 2c = 101100
    simpsons: 18 = 011000
    xmen:     fe = 111110
 5
    punkshot: unused?
    lgtnfght: 2a = 101010
    simpsons: unused?
    xmen: unused?
 6
    punkshot: 26 = 100110
    lgtnfght: 30 = 110000
    simpsons: 17 = 010111
    xmen:     03 = 000011 (written after initial tests)
 7
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 27 = 100111
    xmen:     07 = 000111 (written after initial tests)
 8
    punkshot: unused?
    lgtnfght: unused?
    simpsons: 37 = 110111
    xmen:     ff = 111111 (written after initial tests)
 9  ----xx CI0 palette index base (CO9-CO10)
	--xx-- CI1 palette index base (CO9-CO10)
	xx---- CI2 palette index base (CO9-CO10)
10  ---xxx CI3 palette index base (CO8-CO10)
	xxx--- CI4 palette index base (CO8-CO10)
11
    punkshot: 00 = 000000
    lgtnfght: 00 = 000000
    simpsons: 00 = 000000
    xmen:     00 = 000000 (written after initial tests)
12
    punkshot: 04 = 000100
    lgtnfght: 04 = 000100
    simpsons: 05 = 000101
    xmen:     05 = 000101
13  unused
14  unused
15  unused

For reference, this is the default priority order for some games,
from back to front (CI1 is always sprites):

punkshot: CI0 CI3 CI4 CI1 CI2 one sprite priority bit brings them on top of CI2
lgtnfght: CI0 CI3 CI4 CI2 two sprite priority bits move them anywhere (11 = low 00 = hi)
simpsons: CI0 CI3 CI4 CI2
xmen:     CI4 CI2 CI0 CI1 CI3

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"


/*
	This recursive function doesn't use additional memory
	(it could be easily converted into an iterative one).
	It's called shuffle because it mimics the shuffling of a deck of cards.
*/
static void shuffle(UINT16 *buf,int len)
{
	int i;
	UINT16 t;

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


/* helper function to join 16-bit ROMs and form a 32-bit data stream */
void konami_rom_deinterleave(int memory_region)
{
	shuffle((UINT16 *)Machine->memory_region[memory_region],Machine->memory_region_length[memory_region]/2);
}



static int K052109_memory_region;
static struct GfxElement *K052109_gfx;
static void (*K052109_callback)(int layer,int bank,int *code,int *color,unsigned char *flags);
static unsigned char *K052109_ram;
static unsigned char *K052109_videoram_F,*K052109_videoram2_F,*K052109_colorram_F;
static unsigned char *K052109_videoram_A,*K052109_videoram2_A,*K052109_colorram_A;
static unsigned char *K052109_videoram_B,*K052109_videoram2_B,*K052109_colorram_B;
static unsigned char K052109_yscroll[2];
static unsigned char K052109_charrombank[4];
static int has_extra_video_ram;

static int RMRD_line;
static unsigned char romsubbank,scrollctrl;
static struct tilemap *tilemap[3];
static int K052109_irq_enabled;




/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/*
  data format:
  video RAM    xxxxxxxx  tile number (low 8 bits)
  color RAM    xxxx----  depends on external connections (usually color and banking)
  color RAM    ----xx--  bank select (0-3): these bits are replaced with the 2
                         bottom bits of the bank register before being placed on
						 the output pins. The other two bits of the bank register are
						 placed on the CAB1 and CAB2 output pins.
  color RAM    ------xx  depends on external connections (usually banking, flip)
*/

static unsigned char *colorram,*videoram1,*videoram2;
static int layer;

static void tilemap0_preupdate(void)
{
	colorram = K052109_colorram_F;
	videoram1 = K052109_videoram_F;
	videoram2 = K052109_videoram2_F;
	layer = 0;
}

static void tilemap1_preupdate(void)
{
	colorram = K052109_colorram_A;
	videoram1 = K052109_videoram_A;
	videoram2 = K052109_videoram2_A;
	layer = 1;
}

static void tilemap2_preupdate(void)
{
	colorram = K052109_colorram_B;
	videoram1 = K052109_videoram_B;
	videoram2 = K052109_videoram2_B;
	layer = 2;
}

static void get_tile_info(int col,int row)
{
	int tile_index = 64*row+col;
	int code = videoram1[tile_index] + 256 * videoram2[tile_index];
	int color = colorram[tile_index];
	int bank = K052109_charrombank[(color & 0x0c) >> 2];
if (has_extra_video_ram) bank = (color & 0x0c) >> 2;	/* kludge for X-Men */
	color = (color & 0xf3) | ((bank & 0x03) << 2);
	bank >>= 2;

	(*K052109_callback)(layer,bank,&code,&color,&tile_info.flags);

	code %= K052109_gfx->total_elements;
	tile_info.pen_data = K052109_gfx->gfxdata->line[code*K052109_gfx->height];
	tile_info.pal_data = &K052109_gfx->colortable[K052109_gfx->color_granularity * color];
	tile_info.pen_usage = K052109_gfx->pen_usage?K052109_gfx->pen_usage[code]:0;
}



int K052109_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int layer,int bank,int *code,int *color,unsigned char *flags))
{
	int gfx_index;
	static struct GfxLayout charlayout =
	{
		8,8,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
		32*8
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	charlayout.total = Machine->memory_region_length[gfx_memory_region] / 32;
	charlayout.planeoffset[0] = plane3 * 8;
	charlayout.planeoffset[1] = plane2 * 8;
	charlayout.planeoffset[2] = plane1 * 8;
	charlayout.planeoffset[3] = plane0 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(Machine->memory_region[gfx_memory_region],&charlayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = Machine->colortable;
	Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;

	K052109_memory_region = gfx_memory_region;
	K052109_gfx = Machine->gfx[gfx_index];
	K052109_callback = callback;

	has_extra_video_ram = 0;

	tilemap[0] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[2] = tilemap_create(get_tile_info,TILEMAP_TRANSPARENT,8,8,64,32);

	K052109_ram = malloc(0x6000);

	if (!K052109_ram || !tilemap[0] || !tilemap[1] || !tilemap[2])
	{
		K052109_vh_stop();
		return 1;
	}

	memset(K052109_ram,0,0x6000);

	K052109_colorram_F = &K052109_ram[0x0000];
	K052109_colorram_A = &K052109_ram[0x0800];
	K052109_colorram_B = &K052109_ram[0x1000];
	K052109_videoram_F = &K052109_ram[0x2000];
	K052109_videoram_A = &K052109_ram[0x2800];
	K052109_videoram_B = &K052109_ram[0x3000];
	K052109_videoram2_F = &K052109_ram[0x4000];
	K052109_videoram2_A = &K052109_ram[0x4800];
	K052109_videoram2_B = &K052109_ram[0x5000];

	tilemap[0]->transparent_pen = 0;
	tilemap[1]->transparent_pen = 0;
	tilemap[2]->transparent_pen = 0;

	return 0;
}

void K052109_vh_stop(void)
{
	free(K052109_ram);
	K052109_ram = 0;
}



int K052109_r(int offset)
{
	if (RMRD_line == CLEAR_LINE)
	{
		if ((offset & 0x1fff) >= 0x1800)
if (errorlog) fprintf(errorlog,"%06x: read from unknown 052109 address %04x\n",cpu_get_pc(),offset);

		return K052109_ram[offset];
	}
	else	/* Punk Shot and TMNT read from 0000-1fff, Aliens from 2000-3fff */
	{
		int code = (offset & 0x1fff) >> 5;
		int color = romsubbank;
		int bank = K052109_charrombank[(color & 0x0c) >> 2] >> 2;	/* discard low bits (TMNT) */
		unsigned char flags;
		int addr;

if (has_extra_video_ram) code |= color << 8;	/* kludge for X-Men */
else
		(*K052109_callback)(0,bank,&code,&color,&flags);

		addr = (code << 5) + (offset & 0x1f);
		addr &= Machine->memory_region_length[K052109_memory_region]-1;

#if 0
{
	char baf[60];
	sprintf(baf,"%04x: off%04x sub%02x (bnk%x) adr%06x",cpu_get_pc(),offset,romsubbank,bank,addr);
	usrintf_showmessage(baf);
}
#endif

		return Machine->memory_region[K052109_memory_region][addr];
	}
}

void K052109_w(int offset,int data)
{
	if ((offset & 0x1fff) < 0x1800)	/* tilemap RAM */
	{
		if (K052109_ram[offset] != data)
		{
			if (offset >= 0x4000) has_extra_video_ram = 1;	/* kludge for X-Men */
			K052109_ram[offset] = data;
			tilemap_mark_tile_dirty(tilemap[(offset&0x1fff)/0x800],offset%64,(offset%0x800)/64);
		}
	}
	else	/* control registers */
	{
		K052109_ram[offset] = data;

		if (offset == 0x180c) K052109_yscroll[0] = data;
		else if (offset >= 0x1a00 && offset < 0x1c00)
		{
			/* A x scroll */
		}
		else if (offset == 0x1c80)
		{
if (scrollctrl != data)
{
#ifdef MAME_DEBUG
char baf[40];
sprintf(baf,"scrollcontrol = %02x",data);
usrintf_showmessage(baf);
#endif
if (errorlog) fprintf(errorlog,"%04x: rowscrollcontrol = %02x\n",cpu_get_pc(),data);
			scrollctrl = data;
}
		}
		else if (offset == 0x1d00)
		{
if (errorlog) fprintf(errorlog,"%04x: 052109 register 1d00 = %02x\n",cpu_get_pc(),data);
			/* bit 2 = irq enable */
			/* the custom chip can also generate NMI and FIRQ, for use with a 6809 */
			K052109_irq_enabled = data & 0x04;
		}
		else if (offset == 0x1d80)
		{
			int dirty = 0;

			if (K052109_charrombank[0] != (data & 0x0f)) dirty |= 1;
			if (K052109_charrombank[1] != ((data >> 4) & 0x0f)) dirty |= 2;
			if (dirty)
			{
				int i;

				K052109_charrombank[0] = data & 0x0f;
				K052109_charrombank[1] = (data >> 4) & 0x0f;

				for (i = 0;i < 0x1800;i++)
				{
					int bank = (K052109_ram[i]&0x0c) >> 2;
					if ((bank == 0 && (dirty & 1)) || (bank == 1 && dirty & 2))
					{
						tilemap_mark_tile_dirty(tilemap[(i&0x1fff)/0x800],i%64,(i%0x800)/64);
					}
				}
			}
		}
		else if (offset == 0x1e00)
		{
if (errorlog) fprintf(errorlog,"%04x: 052109 register 1e00 = %02x\n",cpu_get_pc(),data);
			romsubbank = data;
		}
		else if (offset == 0x1e80)
		{
if (errorlog) fprintf(errorlog,"%04x: 052109 register 1e80 = %02x\n",cpu_get_pc(),data);
			tilemap_set_flip(ALL_TILEMAPS,(data & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
		}
		else if (offset == 0x1f00)
		{
			int dirty = 0;

			if (K052109_charrombank[2] != (data & 0x0f)) dirty |= 1;
			if (K052109_charrombank[3] != ((data >> 4) & 0x0f)) dirty |= 2;
			if (dirty)
			{
				int i;

				K052109_charrombank[2] = data & 0x0f;
				K052109_charrombank[3] = (data >> 4) & 0x0f;

				for (i = 0;i < 0x1800;i++)
				{
					int bank = (K052109_ram[i]&0x0c) >> 2;
					if ((bank == 2 && (dirty & 1)) || (bank == 3 && dirty & 2))
					{
						tilemap_mark_tile_dirty(tilemap[(i&0x1fff)/0x800],i%64,(i%0x800)/64);
					}
				}
			}
		}
		else if (offset == 0x380c) K052109_yscroll[1] = data;
		else if (offset >= 0x3a00 && offset < 0x3c00)
		{
			/* B x scroll */
		}
		else
if (errorlog) fprintf(errorlog,"%06x: write %02x to unknown 052109 address %04x\n",cpu_get_pc(),data,offset);
	}
}

void K052109_set_RMRD_line(int state)
{
	RMRD_line = state;
}


void K052109_tilemap_update(void)
{
#if 0
char baf[40];
sprintf(baf,"%x %x %x %x",
	K052109_charrombank[0],
	K052109_charrombank[1],
	K052109_charrombank[2],
	K052109_charrombank[3]);
usrintf_showmessage(baf);
#endif
	if ((scrollctrl & 0x03) == 0x02)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(tilemap[1],256);
		tilemap_set_scroll_cols(tilemap[1],1);
		yscroll = K052109_yscroll[0];
		tilemap_set_scrolly(tilemap[1],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*(offs&0xfff8)+0] + 256 * scrollram[2*(offs&0xfff8)+1];
			xscroll -= 6;
			tilemap_set_scrollx(tilemap[1],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((scrollctrl & 0x03) == 0x03)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(tilemap[1],256);
		tilemap_set_scroll_cols(tilemap[1],1);
		yscroll = K052109_yscroll[0];
		tilemap_set_scrolly(tilemap[1],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*offs+0] + 256 * scrollram[2*offs+1];
			xscroll -= 6;
			tilemap_set_scrollx(tilemap[1],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((scrollctrl & 0x04) == 0x04)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x1800];


		tilemap_set_scroll_rows(tilemap[1],1);
		tilemap_set_scroll_cols(tilemap[1],512);
		xscroll = K052109_ram[0x1a00] + 256 * K052109_ram[0x1a01];
		xscroll -= 6;
		tilemap_set_scrollx(tilemap[1],0,xscroll);
		for (offs = 0;offs < 512;offs++)
		{
			yscroll = scrollram[offs/8];
			tilemap_set_scrolly(tilemap[1],(offs+xscroll)&0x1ff,yscroll);
		}
	}
	else
	{
		int xscroll,yscroll;
		unsigned char *scrollram = &K052109_ram[0x1a00];


		tilemap_set_scroll_rows(tilemap[1],1);
		tilemap_set_scroll_cols(tilemap[1],1);
		xscroll = scrollram[0] + 256 * scrollram[1];
		xscroll -= 6;
		yscroll = K052109_yscroll[0];
		tilemap_set_scrollx(tilemap[1],0,xscroll);
		tilemap_set_scrolly(tilemap[1],0,yscroll);
	}

	if ((scrollctrl & 0x18) == 0x10)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(tilemap[2],256);
		tilemap_set_scroll_cols(tilemap[2],1);
		yscroll = K052109_yscroll[1];
		tilemap_set_scrolly(tilemap[2],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*(offs&0xfff8)+0] + 256 * scrollram[2*(offs&0xfff8)+1];
			xscroll -= 6;
			tilemap_set_scrollx(tilemap[2],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((scrollctrl & 0x18) == 0x18)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(tilemap[2],256);
		tilemap_set_scroll_cols(tilemap[2],1);
		yscroll = K052109_yscroll[1];
		tilemap_set_scrolly(tilemap[2],0,yscroll);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll = scrollram[2*offs+0] + 256 * scrollram[2*offs+1];
			xscroll -= 6;
			tilemap_set_scrollx(tilemap[2],(offs+yscroll)&0xff,xscroll);
		}
	}
	else if ((scrollctrl & 0x20) == 0x20)
	{
		int xscroll,yscroll,offs;
		unsigned char *scrollram = &K052109_ram[0x3800];


		tilemap_set_scroll_rows(tilemap[2],1);
		tilemap_set_scroll_cols(tilemap[2],512);
		xscroll = K052109_ram[0x3a00] + 256 * K052109_ram[0x3a01];
		xscroll -= 6;
		tilemap_set_scrollx(tilemap[2],0,xscroll);
		for (offs = 0;offs < 512;offs++)
		{
			yscroll = scrollram[offs/8];
			tilemap_set_scrolly(tilemap[2],(offs+xscroll)&0x1ff,yscroll);
		}
	}
	else
	{
		int xscroll,yscroll;
		unsigned char *scrollram = &K052109_ram[0x3a00];


		tilemap_set_scroll_rows(tilemap[2],1);
		tilemap_set_scroll_cols(tilemap[2],1);
		xscroll = scrollram[0] + 256 * scrollram[1];
		xscroll -= 6;
		yscroll = K052109_yscroll[1];
		tilemap_set_scrollx(tilemap[2],0,xscroll);
		tilemap_set_scrolly(tilemap[2],0,yscroll);
	}

	tilemap0_preupdate(); tilemap_update(tilemap[0]);
	tilemap1_preupdate(); tilemap_update(tilemap[1]);
	tilemap2_preupdate(); tilemap_update(tilemap[2]);

#if 1
if ((scrollctrl & 0x03) == 0x01 ||
	(scrollctrl & 0x18) == 0x08 ||
	((scrollctrl & 0x04) && (scrollctrl & 0x03)) ||
	((scrollctrl & 0x20) && (scrollctrl & 0x18)) ||
	(scrollctrl & 0xc0) != 0)
{
char baf[40];
sprintf(baf,"scrollcontrol = %02x",scrollctrl);
usrintf_showmessage(baf);
}
#endif

#if 0
if (keyboard_pressed(KEYCODE_F))
{
	FILE *fp;
	fp=fopen("TILE.DMP", "w+b");
	if (fp)
	{
		fwrite(K052109_ram, 0x6000, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
}

void K052109_tilemap_draw(struct osd_bitmap *bitmap,int num,int flags)
{
	tilemap_draw(bitmap,tilemap[num],flags);
}

int K052109_is_IRQ_enabled(void)
{
	return K052109_irq_enabled;
}







static int K051960_memory_region;
static struct GfxElement *K051960_gfx;
static void (*K051960_callback)(int *code,int *color,int *priority);
static int K051960_romoffset;
static int K051960_spriteflip,K051960_readroms;
static unsigned char K051960_spriterombank[2];
static unsigned char *K051960_ram;
static int K051960_irq_enabled;


int K051960_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int *code,int *color,int *priority))
{
	int gfx_index;
	static struct GfxLayout spritelayout =
	{
		16,16,
		0,				/* filled in later */
		4,
		{ 0, 0, 0, 0 },	/* filled in later */
		{ 0, 1, 2, 3, 4, 5, 6, 7,
				8*32+0, 8*32+1, 8*32+2, 8*32+3, 8*32+4, 8*32+5, 8*32+6, 8*32+7 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
				16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
		128*8	/* every sprite takes 64 consecutive bytes */
	};


	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* tweak the structure for the number of tiles we have */
	spritelayout.total = Machine->memory_region_length[gfx_memory_region] / 128;
	spritelayout.planeoffset[0] = plane0 * 8;
	spritelayout.planeoffset[1] = plane1 * 8;
	spritelayout.planeoffset[2] = plane2 * 8;
	spritelayout.planeoffset[3] = plane3 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(Machine->memory_region[gfx_memory_region],&spritelayout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = Machine->colortable;
	Machine->gfx[gfx_index]->total_colors = Machine->drv->color_table_len / 16;

	K051960_memory_region = gfx_memory_region;
	K051960_gfx = Machine->gfx[gfx_index];
	K051960_callback = callback;
	K051960_ram = malloc(0x400);
	if (!K051960_ram) return 1;

	memset(K051960_ram,0,0x400);

	return 0;
}

void K051960_vh_stop(void)
{
	free(K051960_ram);
	K051960_ram = 0;
}

int K051960_r(int offset)
{
	/* the 051960 remembers the last address read and uses it when reading the sprite ROMs */
	K051960_romoffset = offset & 0x3fc;
	return K051960_ram[offset];
}

void K051960_w(int offset,int data)
{
	K051960_ram[offset] = data;
}

int K051960_word_r(int offset)
{
	return K051960_r(offset + 1) | (K051960_r(offset) << 8);
}

void K051960_word_w(int offset,int data)
{
	if ((data & 0xff000000) == 0)
		K051960_w(offset,(data >> 8) & 0xff);
	if ((data & 0x00ff0000) == 0)
		K051960_w(offset + 1,data & 0xff);
}

static int bankkludge;

int K051937_r(int offset)
{
	if (K051960_readroms && offset >= 4 && offset < 8)
	{
		int base,off1,rom,addr;


		base = K051960_spriterombank[0] + 256 * K051960_spriterombank[1];
		off1 = (base & 0x3ff);
		rom = (base & 0x4000) >> 14;	/* Punk Shot */
		if (bankkludge == 0xff) rom = 1;	/* Aliens */
		addr = 0x100000 * rom + 0x400 * off1 + K051960_romoffset + (offset & 3);
		addr &= Machine->memory_region_length[K051960_memory_region]-1;

#if 0
{
	char baf[60];
	sprintf(baf,"%04x: base %04x offset %02x addr %06x",cpu_get_pc(),base,offset,addr);
	usrintf_showmessage(baf);
}
#endif

		return Machine->memory_region[K051960_memory_region][addr];
	}
	else
	{
if (errorlog) fprintf(errorlog,"%06x: read unknown 051937 address %x\n",cpu_get_pc(),offset);
		return 0;
	}
}

void K051937_w(int offset,int data)
{
	if (offset == 0)
	{
#ifdef MAME_DEBUG
if (data & 0xc6)
{
	char baf[40];
	sprintf(baf,"051937 reg 00 = %02x",data);
	usrintf_showmessage(baf);
}
#endif
		/* bit 0 is IRQ enable */
		K051960_irq_enabled = (data & 0x01);

		/* bit 3 = flip screen */
		K051960_spriteflip = data & 0x08;

		/* bit 4 used by Devastators and TMNT, unknown */

		/* bit 5 = enable gfx ROM reading */
		K051960_readroms = data & 0x20;
	}
	else if (offset >= 2 && offset < 4)
	{
		K051960_spriterombank[offset - 2] = data;
	}
	else
	{
#if 0
{
	char baf[40];
	sprintf(baf,"%06x: write %02x to 051937 address %x",cpu_get_pc(),data,offset);
	usrintf_showmessage(baf);
}
#endif
if (errorlog) fprintf(errorlog,"%06x: write %02x to unknown 051937 address %x\n",cpu_get_pc(),data,offset);
		if (offset == 4) bankkludge = data;	/* Aliens kludge */
	}
}



/*
 * Sprite Format
 * ------------------
 *
 * Byte | Bit(s)   | Use
 * -----+-76543210-+----------------
 *   0  | x------- | active (show this sprite)
 *   0  | -xxxxxxx | priority order
 *   1  | xxx----- | sprite size (see below)
 *   1  | ---xxxxx | sprite code (high 5 bits)
 *   2  | xxxxxxxx | sprite code (low 8 bits)
 *   3  | x------- | I think it's hardcoded as shadow, but can be ignored
 *   3  | xxxxxxxx | "color", but depends on external connections (see below)
 *   4  | xxxxxx-- | zoom y (0 = normal, >0 = shrink)
 *   4  | ------x- | flip y
 *   4  | -------x | y position (high bit)
 *   5  | xxxxxxxx | y position (low 8 bits)
 *   6  | xxxxxx-- | zoom x (0 = normal, >0 = shrink)
 *   6  | ------x- | flip x
 *   6  | -------x | x position (high bit)
 *   7  | xxxxxxxx | x position (low 8 bits)
 *
 * Example of "color" field for Punk Shot:
 *   3  | x------- | shadow
 *   3  | -xx----- | priority
 *   3  | ---x---- | use second gfx ROM bank
 *   3  | ----xxxx | color code
 */

void K051960_draw_sprites(struct osd_bitmap *bitmap,int min_priority,int max_priority)
{
#define NUM_SPRITES 128
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < 0x400;offs += 8)
	{
		if (K051960_ram[offs] & 0x80)
			sortedlist[K051960_ram[offs] & 0x7f] = offs;
	}

	for (pri_code = 0;pri_code < NUM_SPRITES;pri_code++)
	{
		int ox,oy,code,color,pri,size,w,h,x,y,flipx,flipy,zoomx,zoomy;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
		static int width[8] =  { 1, 2, 1, 2, 4, 2, 4, 8 };
		static int height[8] = { 1, 1, 2, 2, 2, 4, 4, 8 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		code = K051960_ram[offs+2] + ((K051960_ram[offs+1] & 0x1f) << 8);
		color = K051960_ram[offs+3];
		pri = 0;

		(*K051960_callback)(&code,&color,&pri);

		if (pri < min_priority || pri > max_priority) continue;

		size = (K051960_ram[offs+1] & 0xe0) >> 5;
		w = width[size];
		h = height[size];

		if (w >= 2) code &= ~0x01;
		if (h >= 2) code &= ~0x02;
		if (w >= 4) code &= ~0x04;
		if (h >= 4) code &= ~0x08;
		if (w >= 8) code &= ~0x10;
		if (h >= 8) code &= ~0x20;

		ox = (256 * K051960_ram[offs+6] + K051960_ram[offs+7]) & 0x01ff;
		oy = 256 - ((256 * K051960_ram[offs+4] + K051960_ram[offs+5]) & 0x01ff);
		flipx = K051960_ram[offs+6] & 0x02;
		flipy = K051960_ram[offs+4] & 0x02;
		zoomx = (K051960_ram[offs+6] & 0xfc) >> 2;
		zoomy = (K051960_ram[offs+4] & 0xfc) >> 2;
		zoomx = 0x10000 / 128 * (128 - zoomx);
		zoomy = 0x10000 / 128 * (128 - zoomy);

		if (K051960_spriteflip)
		{
			ox = 512 - (zoomx * w >> 12) - ox;
			oy = 256 - (zoomy * h >> 12) - oy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (zoomx == 0x10000 && zoomy == 0x10000)
		{
			int sx,sy;

			for (y = 0;y < h;y++)
			{
				sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 16 * x;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfx(bitmap,K051960_gfx,
							c,
							color,
							flipx,flipy,
							sx & 0x1ff,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			int sx,sy,zw,zh;

			for (y = 0;y < h;y++)
			{
				sy = oy + ((zoomy * y + (1<<11)) >> 12);
				zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoomx * x + (1<<11)) >> 12);
					zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfxzoom(bitmap,K051960_gfx,
							c,
							color,
							flipx,flipy,
							sx & 0x1ff,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(K051960_ram, 0x400, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
}
#endif
}

void K051960_mark_sprites_colors(void)
{
	int offs,i;

	unsigned short palette_map[128];

	memset (palette_map, 0, sizeof (palette_map));

	/* sprites */
	for (offs = 0x400-8;offs >= 0;offs -= 8)
	{
		int code,color,pri;

		code = K051960_ram[offs+2] + ((K051960_ram[offs+1] & 0x1f) << 8);
		color = (K051960_ram[offs+3] & 0x0f);
		pri = 0;
		(*K051960_callback)(&code,&color,&pri);
		palette_map[color] |= 0xffff;
	}

	/* now build the final table */
	for (i = 0; i < 128; i++)
	{
		int usage = palette_map[i], j;
		if (usage)
		{
			for (j = 1; j < 16; j++)
				if (usage & (1 << j))
					palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
		}
	}
}

int K051960_is_IRQ_enabled(void)
{
	return K051960_irq_enabled;
}




int K052109_051960_r(int offset)
{
	if (RMRD_line == CLEAR_LINE)
	{
		if (offset >= 0x3800 && offset < 0x3808)
			return K051937_r(offset - 0x3800);
		else if (offset < 0x3c00)
			return K052109_r(offset);
		else
			return K051960_r(offset - 0x3c00);
	}
	else return K052109_r(offset);
}

void K052109_051960_w(int offset,int data)
{
	if (offset >= 0x3800 && offset < 0x3808)
		K051937_w(offset - 0x3800,data);
	else if (offset < 0x3c00)
		K052109_w(offset,data);
	else
		K051960_w(offset - 0x3c00,data);
}




static unsigned char K053251_ram[16];
static int K053251_palette_index[5];

void K053251_w(int offset,int data)
{
	data &= 0x3f;

	if (K053251_ram[offset] != data)
	{
		K053251_ram[offset] = data;
		if (offset == 9)
		{
			/* palette base index */
			K053251_palette_index[0] = 32 * ((data >> 0) & 0x03);
			K053251_palette_index[1] = 32 * ((data >> 2) & 0x03);
			K053251_palette_index[2] = 32 * ((data >> 4) & 0x03);
			tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}
		else if (offset == 10)
		{
			/* palette base index */
			K053251_palette_index[3] = 16 * ((data >> 0) & 0x07);
			K053251_palette_index[4] = 16 * ((data >> 3) & 0x07);
			tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}
#if 0
else
{
char baf[80];

if (errorlog)
fprintf(errorlog,"%04x: write %02x to K053251 register %04x\n",cpu_get_pc(),data&0xff,offset);
sprintf(baf,"pri = %02x%02x%02x%02x %02x%02x%02x%02x %02x----%02x %02x%02x%02x%02x",
	K053251_ram[0],K053251_ram[1],K053251_ram[2],K053251_ram[3],
	K053251_ram[4],K053251_ram[5],K053251_ram[6],K053251_ram[7],
	K053251_ram[8],                              K053251_ram[11],
	K053251_ram[12],K053251_ram[13],K053251_ram[14],K053251_ram[15]
	);
usrintf_showmessage(baf);
}
#endif
	}
}

int K053251_get_priority(int ci)
{
	return K053251_ram[ci];
}

int K053251_get_palette_index(int ci)
{
	return K053251_palette_index[ci];
}
