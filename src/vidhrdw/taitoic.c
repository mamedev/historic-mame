/*******************************************************************************

PC080SN
-------
Tilemap generator. Two tilemaps, with gfx data fetched from ROM.
Darius uses 3xPC080SN and has double width tilemaps. (NB: it
has not been verified that Topspeed uses this chip. Possibly
it had a variant with added rowscroll capability.)

Standard memory layout (two 64x64 tilemaps with 8x8 tiles)

0000-3fff BG
4000-41ff BG rowscroll		(only verified to exist on Topspeed)
4200-7fff unknown/unused?
8000-bfff FG	(FG/BG layer order fixed per game; Topspeed has BG on top)
c000-c1ff FG rowscroll		(only verified to exist on Topspeed)
c200-ffff unknown/unused?

Double width memory layout (two 128x64 tilemaps with 8x8 tiles)

0000-7fff BG
8000-ffff FG
(Tile layout is different; tiles and colors are separated:
0x0000-3fff  color / flip words
0x4000-7fff  tile number words)

Control registers

+0x20000 (on from tilemaps)
000-001 BG scroll Y
002-003 FG scroll Y

+0x40000
000-001 BG scroll X
002-003 FG scroll X

+0x50000 control word (written infrequently, only 2 bits used)
	   ---------------x flip screen
	   ----------x----- 0x20 poked here in Topspeed init, followed
	                    by zero (Darius does the same).


TC0100SCN
---------
Tilemap generator. The front tilemap fetches gfx data from RAM,
the others use ROMs as usual.

Standard memory layout (three 64x64 tilemaps with 8x8 tiles)

0000-3fff BG0
4000-5fff FG0
6000-6fff gfx data for FG0
7000-7fff unused (probably)
8000-bfff BG1
c000-c3ff BG0 rowscroll (second half unused*)
c400-c7ff BG1 rowscroll (second half unused*)
c800-dfff unused (probably)
e000-e0ff BG0 colscroll [see info below]
e100-ffff unused (probably)

Double width tilemaps memory layout (two 128x64 tilemaps, one 128x32 tilemap)

00000-07fff BG0 (128x64)
08000-0ffff BG1 (128x64)
10000-103ff BG0 rowscroll (second half unused*)
10400-107ff BG1 rowscroll (second half unused*)
10800-108ff BG0 colscroll [evidenced by Warriorb inits from $1634]
10900-10fff unused (probably)
11000-11fff gfx data for FG0
12000-13fff FG0 (128x32)

* Perhaps Taito wanted potential for double height tilemaps on the
  TC0100SCN. The inits state the whole area is "linescroll".

Control registers

000-001 BG0 scroll X
002-003 BG1 scroll X
004-005 FG0 scroll X
006-007 BG0 scroll Y
008-009 BG1 scroll Y
00a-00b FG0 scroll Y
00c-00d ---------------x BG0 disable
        --------------x- BG1 disable
        -------------x-- FG0 disable
        ------------x--- change priority order from BG0-BG1-FG0 to BG1-BG0-FG0
        -----------x---- double width tilemaps + different memory map
                              (cameltru and all the multi-screen games)
        ----------x----- unknown (set in most of the TaitoZ games and Cadash)
00e-00f ---------------x flip screen
        ----------x----- this TC0100SCN is subsidiary [= not the main one]
                              (Multi-screen games only. Could it mean: "use
                               main TC0100SCN text layer as my text layer" ?)
        --x------------- unknown (thunderfox)


Colscroll [standard layout]
=========

The e000-ff area is not divided into two halves, it appears to refer only
to bg0 - the bottommost layer unless bg0/1 are flipped. This would work
for Gunfront, which flips bg layers and has bg0 as clouds on top: the
video shows only the clouds affected.

128 words are available in 0xe0?? area. I think every word scrolls 8
pixels - evidenced in Gunfront. [128 words could scroll 128x8 pixels,
adequate for the double width tilemaps which are available on the
TC0100SCN.]

[The reasoning behind colscroll only affecting bg0 may be that it is
only addressable per column of 8 pixels. This is not very fine, and will
tend to look jagged because you can't individually control each pixel
column. Not a problem if:
(i) you only use steps of 1 up or down between neighbouring columns
(ii) you use rowscroll simultaneously to drown out the jaggedness
(iii) it's the background layer and isn't the most visible thing.]

Growl
-----
This uses column scroll in the boat scene [that's just after you have
disposed of the fat men in fezzes] and in the underground lava cavern
scene.

Boat scene: code from $2eb58 appears to be doing see-saw motion for
water layer under boat. $e08c is highest word written, it oscillates
between fffa and 0005. Going back towards $e000 a middle point is reached
which sticks at zero. By $e000 written values oscillate again.

A total of 80 words are being written to [some below 0xe000, I think those
won't do anything, sloppy coding...]

Cavern scene: code from $3178a moves a sequence of 0s, 1s and 0x1ffs
along. These words equate to 0, +1, -1 so will gently ripple bg 0
up and down adding to the shimmering heat effect.

Ninja Kids
----------
This uses column scroll in the fat flame boss scene [that's the end of
round 2] and in the last round in the final confrontation with Satan scene.

Fat flame boss: code at $8eee moves a sequence of 1s and 0s along. This
is similar to the heat shimmer in Growl cavern scene.

Final boss: code at $a024 moves a sine wave of values 0-4 along. When
you are close to getting him the range of values expands to 0-10.

Gunfront
--------
In demo mode when the boss appears with the clouds, a sequence of 40 words
forming sine wave between 0xffd0 and ffe0 is moved along. Bg0 has been
given priority over bg1 so it's the foreground (clouds) affected.

The 40 words will affect 40 8-pixel columns [rows, as this game is
rotated] i.e. what is visible on screen at any point.

Galmedes
--------
Towards end of first round in empty starfield area, about three big ship
sprites cross the screen (scrolling down with the starfield). 16 starfield
columns [rows, as the game is rotated] scroll across with the ship.
$84fc0 and neighbouring routines poke col scroll area.



TC0280GRD
TC0430GRW
---------
These generate a zooming/rotating tilemap. The TC0280GRD has to be used in
pairs, while the TC0430GRW is a newer, single-chip solution.
Regardless of the hardware differences, the two are functionally identical
except for incxx and incxy, which need to be multiplied by 2 in the TC0280GRD
to bring them to the same scale of the other parameters.

control registers:
000-003 start x
004-005 incxx
006-007 incyx
008-00b start y
00c-00d incxy
00e-00f incyy


TC0480SCP
---------
Tilemap generator, has four zoomable tilemaps with 16x16 tiles.
It also has a front tilemap with 8x8 tiles which fetches gfx data
from RAM.

Standard memory layout (four 32x32 bg tilemaps, one 64x64 fg tilemap)

0000-0fff BG0
1000-1fff BG1
2000-2fff BG2
3000-3fff BG3
4000-41ff BG0 row horiz'tl magnification (0 = none) [see notes below]
4400-45ff BG1 row horiz'tl magnification
4800-49ff BG2 row horiz'tl magnification
4c00-4dff BG3 row horiz'tl magnification
6000-61ff BG0 row vertical magnification (0 = none)
6400-65ff BG1 row vertical magnification
6800-69ff BG2 row vertical magnification
6c00-6dff BG3 row vertical magnification
[map for row effects is preliminary: code at $1b4a in Slapshot may disprove it]
7000-bfff unknown/unused?
c000-dfff FG0
e000-ffff gfx data for FG0 (4bpp)

Double width tilemaps memory layout (four 64x32 bg tilemaps, one 64x64 fg tilemap)

0000-1fff BG0
2000-3fff BG1
4000-5fff BG2
6000-7fff BG3
8000-81ff [*] BG0 row horiz'tl magnification? (0 = none)
8400-85ff     BG1 row horiz'tl magnification?
8800-89ff     BG2 row horiz'tl magnification?
8c00-8dff     BG3 row horiz'tl magnification?
9000-91ff     BG0 row vertical magnification? (0 = none)
9400-95ff     BG1 row vertical magnification?
9800-99ff     BG2 row vertical magnification?
9c00-9dff     BG3 row vertical magnification?
a000-a1ff     ??
a400-a5ff     ??
[gaps above seem to be unused]
b000-bfff unknown/unused?
c000-dfff FG0
e000-ffff gfx data for FG0 (4bpp)

(* NB: this row effect map suggested by Slapshot code at $1b4a)

Bg layers tile word layout

+0x00   %yx..bbbb cccccccc      b=control bits(?) c=colour .=unused(?)
+0x02   tilenum
[y=yflip x=xflip b=unknown]

Control registers

000-001 BG0 x scroll     (layer priority order definable)
002-003 BG1 x scroll
004-005 BG2 x scroll
006-007 BG3 x scroll
008-009 BG0 y scroll
00a-00b BG1 y scroll
00c-00d BG2 y scroll
00e-00f BG3 y scroll
010-011 BG0 zoom  <0x7f = shrunk e.g. 0x1a in Footchmp hiscore; 0x7f = normal;
012-013 BG1 zoom   0xffff = max zoom (Slapshot initial attract puck hit)
014-015 BG2 zoom
016-017 BG3 zoom
018-019 Text layer x scroll
01a-01b Text layer y scroll
01c-01d Unused (not written)
01e-01f Layer Control register
		x-------	Double width tilemaps (4 bg tilemaps become 64x32, and the
		            memory layout changes). Slapshot changes this on the fly.
		-x------	Flip screen
		--x-----	unknown

				Set in Metalb init by whether a byte in prg ROM $7fffe is zero.
				Subsequently Metalb changes it for some screen layer layouts.
				Footchmp clears it, Hthero sets it [then both leave it alone].
				Deadconx code at $10e2 is interesting, with possible values of:
				0x0, 0x20, 0x40, 0x60 poked in (via ram buffer) to control reg,
				dependent on byte in prg ROM $7fffd and whether screen is flipped.

		---xxxxx	BG layer priority order (?)

				In Metalb this takes various values [see Raine lookup table in
				vidhrdw\taito_f2.c]. Other games leave it at zero.

020-021 BG0 dx	(provides extra precision to x-scroll)
022-023 BG1 dx
024-025 BG2 dx
026-027 BG3 dx
028-029 BG0 dy	(provides extra precision to y-scroll)
02a-02b BG1 dy
02c-02d BG2 dy
02e-02f BG3 dy

Row magnification
=================
Currently we treat the horizontal row magnification as simple rowscroll.
This is wrong! e.g. MetalB stage1 has parallax scrolling effect for the
background which we completely miss. This is achieved by setting
increasing row horizontal magnification from centre row thru to top row.
These values are unchanged until the round 1 boss.

AFAICS from the MetalB videos there is NO equivalent magnification
effect for individual columns.


TC0110PCR
---------
Interface to palette RAM, and simple tilemap/sprite priority handler. The
priority order seems to be fixed.
The data bus is 16 bits wide.

000  W selects palette RAM address
002 RW read/write palette RAM
004  W unknown, often written to


TC0220IOC
---------
A simple I/O interface with integrated watchdog.
It has four address inputs, which would suggest 16 bytes of addressing space,
but only the first 8 seem to be used.

000 R  IN00-07 (DSA)
000  W watchdog reset
001 R  IN08-15 (DSB)
002 R  IN16-23 (1P)
002  W unknown. Usually written on startup: initialize?
003 R  IN24-31 (2P)
004 RW coin counters and lockout
005  W unknown
006  W unknown
007 R  INB0-7 (coin)


TC0510NIO
---------
Newer version of the I/O chip

000 R  DSWA
000  W watchdog reset
001 R  DSWB
001  W unknown (ssi)
002 R  1P
003 R  2P
003  W unknown (yuyugogo, qzquest and qzchikyu use it a lot)
004 RW coin counters and lockout
005  W unknown
006  W unknown (koshien and pulirula use it a lot)
007 R  coin

***************************************************************************/

#include "driver.h"
#include "taitoic.h"


#define PC080SN_RAM_SIZE 0x10000
#define PC080SN_MAX_CHIPS 2
static int PC080SN_chips;

static data16_t PC080SN_ctrl[PC080SN_MAX_CHIPS][8];

static data16_t *PC080SN_ram[PC080SN_MAX_CHIPS],
				*PC080SN_bg_ram[PC080SN_MAX_CHIPS],
				*PC080SN_fg_ram[PC080SN_MAX_CHIPS],
				*PC080SN_bgscroll_ram[PC080SN_MAX_CHIPS],
				*PC080SN_fgscroll_ram[PC080SN_MAX_CHIPS];

static int PC080SN_bgscrollx[PC080SN_MAX_CHIPS],PC080SN_bgscrolly[PC080SN_MAX_CHIPS],
		PC080SN_fgscrollx[PC080SN_MAX_CHIPS],PC080SN_fgscrolly[PC080SN_MAX_CHIPS];
static struct tilemap *PC080SN_tilemap[PC080SN_MAX_CHIPS][2];
static int PC080SN_bg_gfx[PC080SN_MAX_CHIPS];
static int PC080SN_yinvert,PC080SN_dblwidth;

INLINE void common_get_PC080SN_bg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code,attr;

	if (!PC080SN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x3fff);
		attr = ram[2*tile_index];
	}
	else
	{
		code = (ram[tile_index + 0x2000] & 0x3fff);
		attr = ram[tile_index];
	}

	SET_TILE_INFO(gfxnum,code,(attr & 0x1ff));
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

INLINE void common_get_PC080SN_fg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code,attr;

	if (!PC080SN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x3fff);
		attr = ram[2*tile_index];
	}
	else
	{
		code = (ram[tile_index + 0x2000] & 0x3fff);
		attr = ram[tile_index];
	}

	SET_TILE_INFO(gfxnum,code,(attr & 0x1ff));
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

static void PC080SN_get_bg_tile_info_0(int tile_index)
{
	common_get_PC080SN_bg_tile_info(PC080SN_bg_ram[0],PC080SN_bg_gfx[0],tile_index);
}

static void PC080SN_get_fg_tile_info_0(int tile_index)
{
	common_get_PC080SN_fg_tile_info(PC080SN_fg_ram[0],PC080SN_bg_gfx[0],tile_index);
}

static void PC080SN_get_bg_tile_info_1(int tile_index)
{
	common_get_PC080SN_bg_tile_info(PC080SN_bg_ram[1],PC080SN_bg_gfx[1],tile_index);
}

static void PC080SN_get_fg_tile_info_1(int tile_index)
{
	common_get_PC080SN_fg_tile_info(PC080SN_fg_ram[1],PC080SN_bg_gfx[1],tile_index);
}

void (*PC080SN_get_tile_info[PC080SN_MAX_CHIPS][2])(int tile_index) =
{
	{ PC080SN_get_bg_tile_info_0, PC080SN_get_fg_tile_info_0 },
	{ PC080SN_get_bg_tile_info_1, PC080SN_get_fg_tile_info_1 }
};


int PC080SN_vh_start(int chips,int gfxnum,int x_offset,int y_offset,int y_invert,int opaque,int dblwidth)
{
	int i;

	if (chips > PC080SN_MAX_CHIPS) return 1;
	PC080SN_chips = chips;

	PC080SN_yinvert = y_invert;
	PC080SN_dblwidth = dblwidth;

	for (i = 0;i < chips;i++)
	{
		int a,b,xd,yd;

		/* Rainbow Islands *has* to have an opaque back tilemap, or the
		   background color is always black */

		a = TILEMAP_TRANSPARENT;
		b = TILEMAP_TRANSPARENT;

		if (opaque==1)
			a = TILEMAP_OPAQUE;

		if (opaque==2)
			b = TILEMAP_OPAQUE;

		if (!PC080SN_dblwidth)	/* standard tilemaps */
		{
			PC080SN_tilemap[i][0] = tilemap_create(PC080SN_get_tile_info[i][0],tilemap_scan_rows,a,8,8,64,64);
			PC080SN_tilemap[i][1] = tilemap_create(PC080SN_get_tile_info[i][1],tilemap_scan_rows,b,8,8,64,64);
		}
		else	/* double width tilemaps */
		{
			PC080SN_tilemap[i][0] = tilemap_create(PC080SN_get_tile_info[i][0],tilemap_scan_rows,a,8,8,128,64);
			PC080SN_tilemap[i][1] = tilemap_create(PC080SN_get_tile_info[i][1],tilemap_scan_rows,b,8,8,128,64);
		}

		PC080SN_ram[i] = malloc(PC080SN_RAM_SIZE);

		if (!PC080SN_ram[i] || !PC080SN_tilemap[i][0] ||
				!PC080SN_tilemap[i][1])
		{
			PC080SN_vh_stop();
			return 1;
		}

		PC080SN_bg_ram[i]       = PC080SN_ram[i] + 0x0000;
		PC080SN_bgscroll_ram[i] = PC080SN_ram[i] + 0x2000;
		PC080SN_fg_ram[i]       = PC080SN_ram[i] + 0x4000;
		PC080SN_fgscroll_ram[i] = PC080SN_ram[i] + 0x6000;
		memset(PC080SN_ram[i],0,PC080SN_RAM_SIZE);

		/* use the given gfx set for bg tiles */
		PC080SN_bg_gfx[i] = gfxnum;

		tilemap_set_transparent_pen(PC080SN_tilemap[i][0],0);
		tilemap_set_transparent_pen(PC080SN_tilemap[i][1],0);

		/* I'm setting optional chip #2 with the same offsets (Topspeed) */
		xd = (i == 0) ? -x_offset : -x_offset;
		yd = (i == 0) ? y_offset : y_offset;

		tilemap_set_scrolldx(PC080SN_tilemap[i][0],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(PC080SN_tilemap[i][0],yd,-yd);
		tilemap_set_scrolldx(PC080SN_tilemap[i][1],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(PC080SN_tilemap[i][1],yd,-yd);

		if (!PC080SN_dblwidth)
		{
			tilemap_set_scroll_rows(PC080SN_tilemap[i][0],512);
			tilemap_set_scroll_rows(PC080SN_tilemap[i][1],512);
		}
	}

	return 0;
}

void PC080SN_vh_stop(void)
{
	int i;

	for (i = 0;i < PC080SN_chips;i++)
	{
		free(PC080SN_ram[i]);
		PC080SN_ram[i] = 0;
	}
}

READ16_HANDLER( PC080SN_word_0_r )
{
	return PC080SN_ram[0][offset];
}

READ16_HANDLER( PC080SN_word_1_r )
{
	return PC080SN_ram[1][offset];
}

static void PC080SN_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = PC080SN_ram[chip][offset];

	COMBINE_DATA(&PC080SN_ram[chip][offset]);
	if (oldword != PC080SN_ram[chip][offset])
	{
		if (!PC080SN_dblwidth)
		{
			if (offset < 0x2000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][0],offset / 2);
			else if (offset >= 0x4000 && offset < 0x6000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][1],(offset & 0x1fff) / 2);
		}
		else
		{
			if (offset < 0x4000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][0],(offset & 0x1fff));
			else if (offset >= 0x4000 && offset < 0x8000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][1],(offset & 0x1fff));
		}
	}
}

WRITE16_HANDLER( PC080SN_word_0_w )
{
	PC080SN_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_word_1_w )
{
	PC080SN_word_w(1,offset,data,mem_mask);
}

static void PC080SN_xscroll_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset]);

	data = PC080SN_ctrl[chip][offset];

	switch (offset)
	{
		case 0x00:
			PC080SN_bgscrollx[chip] = -data;
			break;

		case 0x01:
			PC080SN_fgscrollx[chip] = -data;
			break;
	}
}

static void PC080SN_yscroll_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset+2]);

	data = PC080SN_ctrl[chip][offset+2];
	if (PC080SN_yinvert)
		data = -data;

	switch (offset)
	{
		case 0x00:
			PC080SN_bgscrolly[chip] = -data;
			break;

		case 0x01:
			PC080SN_fgscrolly[chip] = -data;
			break;
	}
}

static void PC080SN_ctrl_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset+4]);

	data = PC080SN_ctrl[chip][offset+4];

	switch (offset)
	{
		case 0x00:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			tilemap_set_flip(PC080SN_tilemap[chip][0],flip);
			tilemap_set_flip(PC080SN_tilemap[chip][1],flip);
			break;
		}
	}
#if 0
	usrintf_showmessage("PC080SN ctrl = %4x",data);
#endif
}

WRITE16_HANDLER( PC080SN_xscroll_word_0_w )
{
	PC080SN_xscroll_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_xscroll_word_1_w )
{
	PC080SN_xscroll_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_yscroll_word_0_w )
{
	PC080SN_yscroll_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_yscroll_word_1_w )
{
	PC080SN_yscroll_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_ctrl_word_0_w )
{
	PC080SN_ctrl_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_ctrl_word_1_w )
{
	PC080SN_ctrl_word_w(1,offset,data,mem_mask);
}

/* This routine is needed as an override by Jumping, which
   doesn't set proper scroll values for foreground tilemap */

void PC080SN_set_scroll(int chip,int tilemap_num,int scrollx,int scrolly)
{
	tilemap_set_scrollx(PC080SN_tilemap[chip][tilemap_num],0,scrollx);
	tilemap_set_scrolly(PC080SN_tilemap[chip][tilemap_num],0,scrolly);
}

/* This routine is needed as an override by Jumping */

void PC080SN_set_trans_pen(int chip,int tilemap_num,int pen)
{
	tilemap_set_transparent_pen(PC080SN_tilemap[chip][tilemap_num],pen);
}


void PC080SN_tilemap_update(void)
{
	int chip,j;

	for (chip = 0;chip < PC080SN_chips;chip++)
	{
		tilemap_set_scrolly(PC080SN_tilemap[chip][0],0,PC080SN_bgscrolly[chip]);
		tilemap_set_scrolly(PC080SN_tilemap[chip][1],0,PC080SN_fgscrolly[chip]);

		if (!PC080SN_dblwidth)
		{
			for (j = 0;j < 256;j++)
				tilemap_set_scrollx(PC080SN_tilemap[chip][0],
						(j + PC080SN_bgscrolly[chip]) & 0x1ff,
						PC080SN_bgscrollx[chip] - PC080SN_bgscroll_ram[chip][j]);
			for (j = 0;j < 256;j++)
				tilemap_set_scrollx(PC080SN_tilemap[chip][1],
						(j + PC080SN_fgscrolly[chip]) & 0x1ff,
						PC080SN_fgscrollx[chip] - PC080SN_fgscroll_ram[chip][j]);
		}
		else
		{
			tilemap_set_scrollx(PC080SN_tilemap[chip][0],0,PC080SN_bgscrollx[chip]);
			tilemap_set_scrollx(PC080SN_tilemap[chip][1],0,PC080SN_fgscrollx[chip]);
		}

		tilemap_update(PC080SN_tilemap[chip][0]);
		tilemap_update(PC080SN_tilemap[chip][1]);
	}
}

void PC080SN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority)
{
	switch (layer)
	{
		case 0:
			tilemap_draw(bitmap,PC080SN_tilemap[chip][0],flags,priority);
			break;
		case 1:
			tilemap_draw(bitmap,PC080SN_tilemap[chip][1],flags,priority);
			break;
	}
}


/***************************************************************************/

static unsigned char taitof2_scrbank;
WRITE16_HANDLER( taitof2_scrbank_w )   /* Mjnquest banks its 2 sets of scr tiles */
{
    taitof2_scrbank = (data & 0x1);
}

#define TC0100SCN_RAM_SIZE 0x14000	/* enough for double-width tilemaps */
#define TC0100SCN_TOTAL_CHARS 256
#define TC0100SCN_MAX_CHIPS 3
static int TC0100SCN_chips;

static data16_t TC0100SCN_ctrl[TC0100SCN_MAX_CHIPS][8];

static data16_t *TC0100SCN_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_bg_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_fg_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_tx_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_char_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_bgscroll_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_fgscroll_ram[TC0100SCN_MAX_CHIPS];

static int TC0100SCN_bgscrollx[TC0100SCN_MAX_CHIPS],TC0100SCN_bgscrolly[TC0100SCN_MAX_CHIPS],
		TC0100SCN_fgscrollx[TC0100SCN_MAX_CHIPS],TC0100SCN_fgscrolly[TC0100SCN_MAX_CHIPS];

/* We keep two tilemaps for each of the 3 actual tilemaps: one at standard width, one double */
static struct tilemap *TC0100SCN_tilemap[TC0100SCN_MAX_CHIPS][3][2];

static char *TC0100SCN_char_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_chars_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_bg_gfx[TC0100SCN_MAX_CHIPS],TC0100SCN_tx_gfx[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_bg_col_mult = 0;
static int TC0100SCN_colbank[3],TC0100SCN_dblwidth;


INLINE void common_get_bg0_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int code,attr;

	if (!TC0100SCN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x7fff) + (taitof2_scrbank << 15);
		attr = ram[2*tile_index];
	}
	else
	{
		code = ram[2*tile_index + 1];
		attr = ram[2*tile_index];
	}
	SET_TILE_INFO(gfxnum,code,((attr * TC0100SCN_bg_col_mult) + TC0100SCN_colbank[0]) & 0xff);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

INLINE void common_get_bg1_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int code,attr;

	if (!TC0100SCN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x7fff) + (taitof2_scrbank << 15);
		attr = ram[2*tile_index];
	}
	else
	{
		code = ram[2*tile_index + 1];
		attr = ram[2*tile_index];
	}
	SET_TILE_INFO(gfxnum,code,((attr * TC0100SCN_bg_col_mult) + TC0100SCN_colbank[1]) & 0xff);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

INLINE void common_get_tx_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int attr;

	if (!TC0100SCN_dblwidth)
	{
		attr = ram[tile_index];
	}
	else
	{
		attr = ram[tile_index];
	}
	SET_TILE_INFO(gfxnum,attr & 0xff,(((attr >> 6) &0xfc) + (TC0100SCN_colbank[2] << 2)) &0x3ff);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

static void TC0100SCN_get_bg_tile_info_0(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[0],TC0100SCN_bg_gfx[0],tile_index);
}

static void TC0100SCN_get_fg_tile_info_0(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[0],TC0100SCN_bg_gfx[0],tile_index);
}

static void TC0100SCN_get_tx_tile_info_0(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[0],TC0100SCN_tx_gfx[0],tile_index);
}

static void TC0100SCN_get_bg_tile_info_1(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[1],TC0100SCN_bg_gfx[1],tile_index);
}

static void TC0100SCN_get_fg_tile_info_1(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[1],TC0100SCN_bg_gfx[1],tile_index);
}

static void TC0100SCN_get_tx_tile_info_1(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[1],TC0100SCN_tx_gfx[1],tile_index);
}

static void TC0100SCN_get_bg_tile_info_2(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[2],TC0100SCN_bg_gfx[2],tile_index);
}

static void TC0100SCN_get_fg_tile_info_2(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[2],TC0100SCN_bg_gfx[2],tile_index);
}

static void TC0100SCN_get_tx_tile_info_2(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[2],TC0100SCN_tx_gfx[2],tile_index);
}

/* This array changes if TC0100SCN_MAX_CHIPS is altered */

void (*TC0100SCN_get_tile_info[TC0100SCN_MAX_CHIPS][3])(int tile_index) =
{
	{ TC0100SCN_get_bg_tile_info_0, TC0100SCN_get_fg_tile_info_0 ,TC0100SCN_get_tx_tile_info_0 },
	{ TC0100SCN_get_bg_tile_info_1, TC0100SCN_get_fg_tile_info_1 ,TC0100SCN_get_tx_tile_info_1 },
	{ TC0100SCN_get_bg_tile_info_2, TC0100SCN_get_fg_tile_info_2 ,TC0100SCN_get_tx_tile_info_2 }
};


static struct GfxLayout TC0100SCN_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
#ifdef LSB_FIRST
	{ 8, 0 },
#else
	{ 0, 8 },
#endif
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


void TC0100SCN_set_colbanks(int bg0,int bg1,int fg)
{
	TC0100SCN_colbank[0] = bg0;
	TC0100SCN_colbank[1] = bg1;
	TC0100SCN_colbank[2] = fg;	/* text */
}

void TC0100SCN_set_layer_ptrs(int i)
{
	if (!TC0100SCN_dblwidth)
	{
		TC0100SCN_bg_ram[i]       = TC0100SCN_ram[i] + 0x0000;
		TC0100SCN_tx_ram[i]       = TC0100SCN_ram[i] + 0x2000;
		TC0100SCN_char_ram[i]     = TC0100SCN_ram[i] + 0x3000;
		TC0100SCN_fg_ram[i]       = TC0100SCN_ram[i] + 0x4000;
		TC0100SCN_bgscroll_ram[i] = TC0100SCN_ram[i] + 0x6000;	// c000
		TC0100SCN_fgscroll_ram[i] = TC0100SCN_ram[i] + 0x6200;	// c400
	}
	else
	{
		TC0100SCN_bg_ram[i]       = TC0100SCN_ram[i] + 0x0000;
		TC0100SCN_fg_ram[i]       = TC0100SCN_ram[i] + 0x4000;
		TC0100SCN_bgscroll_ram[i] = TC0100SCN_ram[i] + 0x8000;	// 10000
		TC0100SCN_fgscroll_ram[i] = TC0100SCN_ram[i] + 0x8200;	// 10400
		TC0100SCN_char_ram[i]     = TC0100SCN_ram[i] + 0x8800;	// 11000
		TC0100SCN_tx_ram[i]       = TC0100SCN_ram[i] + 0x9000;	// 12000
	}
}


int TC0100SCN_vh_start(int chips,int gfxnum,int x_offset)
{
	int gfx_index,gfxset_offs,i;

	TC0100SCN_dblwidth=0;

	if (chips > TC0100SCN_MAX_CHIPS) return 1;

	TC0100SCN_chips = chips;

	for (i = 0;i < chips;i++)
	{
		int xd,yd;

		/* Single width versions */
		TC0100SCN_tilemap[i][0][0] = tilemap_create(TC0100SCN_get_tile_info[i][0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][1][0] = tilemap_create(TC0100SCN_get_tile_info[i][1],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][2][0] = tilemap_create(TC0100SCN_get_tile_info[i][2],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		/* Double width versions */
		TC0100SCN_tilemap[i][0][1] = tilemap_create(TC0100SCN_get_tile_info[i][0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
		TC0100SCN_tilemap[i][1][1] = tilemap_create(TC0100SCN_get_tile_info[i][1],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
		TC0100SCN_tilemap[i][2][1] = tilemap_create(TC0100SCN_get_tile_info[i][2],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,32);

		TC0100SCN_ram[i] = malloc(TC0100SCN_RAM_SIZE);
		TC0100SCN_char_dirty[i] = malloc(TC0100SCN_TOTAL_CHARS);

		if (!TC0100SCN_ram[i] || !TC0100SCN_char_dirty[i] ||
				!TC0100SCN_tilemap[i][0][0] || !TC0100SCN_tilemap[i][0][1] ||
				!TC0100SCN_tilemap[i][1][0] || !TC0100SCN_tilemap[i][1][1] ||
				!TC0100SCN_tilemap[i][2][0] || !TC0100SCN_tilemap[i][2][1] )
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		TC0100SCN_set_layer_ptrs(i);

		memset(TC0100SCN_ram[i],0,TC0100SCN_RAM_SIZE);
		memset(TC0100SCN_char_dirty[i],1,TC0100SCN_TOTAL_CHARS);
		TC0100SCN_chars_dirty[i] = 1;

		/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx((UINT8 *)TC0100SCN_char_ram[i],&TC0100SCN_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;

		TC0100SCN_tx_gfx[i] = gfx_index;

		/* use the given gfx set for bg tiles; 2nd/3rd chips will
		   use the same gfx set */
		gfxset_offs = i;
		if (i > 1)
			gfxset_offs = 1;
		TC0100SCN_bg_gfx[i] = gfxnum + gfxset_offs;

		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][0][0],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][1][0],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][2][0],0);

		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][0][1],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][1][1],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][2][1],0);

		/* Standard width tilemaps. I'm setting the optional chip #2
		   7 bits higher and 2 pixels to the left than chip #1 because
		   that's how thundfox wants it. */

		xd = (i == 0) ? -x_offset : (-x_offset-2);
		yd = (i == 0) ? 8 : 1;
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][0][0],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][0][0],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][1][0],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][1][0],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][2][0],-16 + xd,-16 - xd - 7);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][2][0],yd,-yd);

		/* Double width tilemaps */
		xd = -x_offset;
		yd = 8;
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][0][1],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][0][1],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][1][1],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][1][1],yd,-yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][2][1],-16 + xd,-16 - xd - 7);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][2][1],yd,-yd);

		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][0][0],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][1][0],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][0][1],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][1][1],512);
	}

	taitof2_scrbank = 0;

	TC0100SCN_bg_col_mult = 1;	/* multiplier to cope with bg gfx != 4bpp */

	if (Machine->gfx[gfxnum]->color_granularity == 2)	/* Yuyugogo, Yesnoj */
		TC0100SCN_bg_col_mult = 8;

	TC0100SCN_set_colbanks(0,0,0);	/* to use other values, set them after calling TC0100SCN_vh_start */

	return 0;
}

void TC0100SCN_vh_stop(void)
{
	int i;

	for (i = 0;i < TC0100SCN_chips;i++)
	{
		free(TC0100SCN_ram[i]);
		TC0100SCN_ram[i] = 0;
		free(TC0100SCN_char_dirty[i]);
		TC0100SCN_char_dirty[i] = 0;
	}
}


READ16_HANDLER( TC0100SCN_word_0_r )
{
	return TC0100SCN_ram[0][offset];
}

READ16_HANDLER( TC0100SCN_word_1_r )
{
	return TC0100SCN_ram[1][offset];
}

READ16_HANDLER( TC0100SCN_word_2_r )
{
	return TC0100SCN_ram[2][offset];
}

static void TC0100SCN_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = TC0100SCN_ram[chip][offset];

	COMBINE_DATA(&TC0100SCN_ram[chip][offset]);
	if (oldword != TC0100SCN_ram[chip][offset])
	{
		if (!TC0100SCN_dblwidth)
		{
			if (offset < 0x2000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][0][0],offset / 2);
			else if (offset < 0x3000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][0],(offset & 0x0fff));
			else if (offset < 0x3800)
			{
				TC0100SCN_char_dirty[chip][(offset - 0x3000) / 8] = 1;
				TC0100SCN_chars_dirty[chip] = 1;
			}
			else if (offset >= 0x4000 && offset < 0x6000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][1][0],(offset & 0x1fff) / 2);
		}
		else	/* Double-width tilemaps have a different memory map */
		{
			if (offset < 0x4000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][0][1],offset / 2);
			else if (offset >= 0x4000 && offset < 0x8000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][1][1],(offset & 0x3fff) / 2);
			else if (offset >= 0x8800 && offset < 0x9000)
			{
				TC0100SCN_char_dirty[chip][(offset - 0x8800) / 8] = 1;
				TC0100SCN_chars_dirty[chip] = 1;
			}
			else if (offset >= 0x9000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][1],(offset & 0x0fff));
		}
	}
}

WRITE16_HANDLER( TC0100SCN_word_0_w )
{
	TC0100SCN_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_word_1_w )
{
	TC0100SCN_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_word_2_w )
{
	TC0100SCN_word_w(2,offset,data,mem_mask);
}


READ16_HANDLER( TC0100SCN_ctrl_word_0_r )
{
	return TC0100SCN_ctrl[0][offset];
}

READ16_HANDLER( TC0100SCN_ctrl_word_1_r )
{
	return TC0100SCN_ctrl[1][offset];
}

READ16_HANDLER( TC0100SCN_ctrl_word_2_r )
{
	return TC0100SCN_ctrl[2][offset];
}


static void TC0100SCN_ctrl_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&TC0100SCN_ctrl[chip][offset]);

	data = TC0100SCN_ctrl[chip][offset];

	switch (offset)
	{
		case 0x00:
			TC0100SCN_bgscrollx[chip] = -data;
			break;

		case 0x01:
			TC0100SCN_fgscrollx[chip] = -data;
			break;

		case 0x02:
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][0],0,-data);
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][1],0,-data);
			break;

		case 0x03:
			TC0100SCN_bgscrolly[chip] = -data;
			break;

		case 0x04:
			TC0100SCN_fgscrolly[chip] = -data;
			break;

		case 0x05:
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][0],0,-data);
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][1],0,-data);
			break;

		case 0x06:
		{
			int old_width = TC0100SCN_dblwidth;

			TC0100SCN_dblwidth = (data &0x10) >> 4;

			if (TC0100SCN_dblwidth != old_width)	/* tilemap width is changing */
			{
				/* Reinitialise layer pointers */
				TC0100SCN_set_layer_ptrs(chip);

				/* We have neglected these tilemaps, so we make amends now */
				tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth]);
			}

			break;
		}

		case 0x07:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			tilemap_set_flip(TC0100SCN_tilemap[chip][0][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][1][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][2][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][0][1],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][1][1],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][2][1],flip);

			break;
		}
	}
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_0_w )
{
	TC0100SCN_ctrl_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_1_w )
{
	TC0100SCN_ctrl_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_2_w )
{
	TC0100SCN_ctrl_word_w(2,offset,data,mem_mask);
}


void TC0100SCN_tilemap_update(void)
{
	int chip,j;

	for (chip = 0;chip < TC0100SCN_chips;chip++)
	{
		tilemap_set_scrolly(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth],0,TC0100SCN_bgscrolly[chip]);
		tilemap_set_scrolly(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth],0,TC0100SCN_fgscrolly[chip]);

		for (j = 0;j < 256;j++)
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth],
					(j + TC0100SCN_bgscrolly[chip]) & 0x1ff,
					TC0100SCN_bgscrollx[chip] - TC0100SCN_bgscroll_ram[chip][j]);
		for (j = 0;j < 256;j++)
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth],
					(j + TC0100SCN_fgscrolly[chip]) & 0x1ff,
					TC0100SCN_fgscrollx[chip] - TC0100SCN_fgscroll_ram[chip][j]);

		/* Decode any characters that have changed */
		if (TC0100SCN_chars_dirty[chip])
		{
			int tile_index;


			for (tile_index = 0;tile_index < 64*64;tile_index++)
			{
				int attr = TC0100SCN_tx_ram[chip][tile_index];
				if (TC0100SCN_char_dirty[chip][attr & 0xff])
					tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth],tile_index);
			}

			for (j = 0;j < TC0100SCN_TOTAL_CHARS;j++)
			{
				if (TC0100SCN_char_dirty[chip][j])
					decodechar(Machine->gfx[TC0100SCN_tx_gfx[chip]],j,(UINT8 *)TC0100SCN_char_ram[chip],&TC0100SCN_charlayout);
				TC0100SCN_char_dirty[chip][j] = 0;
			}
			TC0100SCN_chars_dirty[chip] = 0;
		}

		tilemap_update(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth]);
		tilemap_update(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth]);
		tilemap_update(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth]);
	}
}

void TC0100SCN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority)
{
	int disable = TC0100SCN_ctrl[chip][6] & 0xf7;

#if 0
if (disable != 0 && disable != 3 && disable != 7)
	usrintf_showmessage("layer disable = %x",disable);
#endif

	switch (layer)
	{
		case 0:
			if (disable & 0x01) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth],flags,priority);
			break;
		case 1:
			if (disable & 0x02) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth],flags,priority);
			break;
		case 2:
			if (disable & 0x04) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth],flags,priority);
			break;
	}
}

int TC0100SCN_bottomlayer(int chip)
{
	return (TC0100SCN_ctrl[chip][6] & 0x8) >> 3;
}


/***************************************************************************/

#define TC0280GRD_RAM_SIZE 0x2000
static data16_t TC0280GRD_ctrl[8];
static data16_t *TC0280GRD_ram;
static struct tilemap *TC0280GRD_tilemap;
static int TC0280GRD_gfxnum,TC0280GRD_base_color;


static void TC0280GRD_get_tile_info(int tile_index)
{
	int attr = TC0280GRD_ram[tile_index];
	SET_TILE_INFO(TC0280GRD_gfxnum,attr & 0x3fff,((attr & 0xc000) >> 14) + TC0280GRD_base_color);
}


int TC0280GRD_vh_start(int gfxnum)
{
	TC0280GRD_ram = malloc(TC0280GRD_RAM_SIZE);
	TC0280GRD_tilemap = tilemap_create(TC0280GRD_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);

	if (!TC0280GRD_ram || !TC0280GRD_tilemap)
	{
		TC0280GRD_vh_stop();
		return 1;
	}

	tilemap_set_clip(TC0280GRD_tilemap,0);

	TC0280GRD_gfxnum = gfxnum;

	return 0;
}

int TC0430GRW_vh_start(int gfxnum)
{
	return TC0280GRD_vh_start(gfxnum);
}

void TC0280GRD_vh_stop(void)
{
	free(TC0280GRD_ram);
	TC0280GRD_ram = 0;
}

void TC0430GRW_vh_stop(void)
{
	TC0280GRD_vh_stop();
}

READ16_HANDLER( TC0280GRD_word_r )
{
	return TC0280GRD_ram[offset];
}

READ16_HANDLER( TC0430GRW_word_r )
{
	return TC0280GRD_word_r(offset);
}

WRITE16_HANDLER( TC0280GRD_word_w )
{
	int oldword = TC0280GRD_ram[offset];

	COMBINE_DATA(&TC0280GRD_ram[offset]);
	if (oldword != TC0280GRD_ram[offset])
	{
		tilemap_mark_tile_dirty(TC0280GRD_tilemap,offset);
	}
}

WRITE16_HANDLER( TC0430GRW_word_w )
{
	TC0280GRD_word_w(offset,data,mem_mask);
}

WRITE16_HANDLER( TC0280GRD_ctrl_word_w )
{
	COMBINE_DATA(&TC0280GRD_ctrl[offset]);
}

WRITE16_HANDLER( TC0430GRW_ctrl_word_w )
{
	TC0280GRD_ctrl_word_w(offset,data,mem_mask);
}

void TC0280GRD_tilemap_update(int base_color)
{
	if (TC0280GRD_base_color != base_color)
	{
		TC0280GRD_base_color = base_color;
		tilemap_mark_all_tiles_dirty(TC0280GRD_tilemap);
	}

	tilemap_update(TC0280GRD_tilemap);
}

void TC0430GRW_tilemap_update(int base_color)
{
	TC0280GRD_tilemap_update(base_color);
}

static void zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority,int xmultiply)
{
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0280GRD_tilemap);

	/* 24-bit signed */
	startx = ((TC0280GRD_ctrl[0] & 0xff) << 16) + TC0280GRD_ctrl[1];
	if (startx & 0x800000) startx -= 0x1000000;
	incxx = (INT16)TC0280GRD_ctrl[2];
	incxx *= xmultiply;
	incyx = (INT16)TC0280GRD_ctrl[3];
	/* 24-bit signed */
	starty = ((TC0280GRD_ctrl[4] & 0xff) << 16) + TC0280GRD_ctrl[5];
	if (starty & 0x800000) starty -= 0x1000000;
	incxy = (INT16)TC0280GRD_ctrl[6];
	incxy *= xmultiply;
	incyy = (INT16)TC0280GRD_ctrl[7];

	startx -= xoffset * incxx + yoffset * incyx;
	starty -= xoffset * incxy + yoffset * incyy;

	copyrozbitmap(bitmap,srcbitmap,startx << 4,starty << 4,
			incxx << 4,incxy << 4,incyx << 4,incyy << 4,
			1,	/* copy with wraparound */
			&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen,priority);
}

void TC0280GRD_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,2);
}

void TC0430GRW_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,1);
}


/***************************************************************************/


#define TC0480SCP_RAM_SIZE 0x10000
#define TC0480SCP_TOTAL_CHARS 256
static data16_t TC0480SCP_ctrl[24];
static data16_t *TC0480SCP_ram,
		*TC0480SCP_bg_ram[4],
		*TC0480SCP_tx_ram,
		*TC0480SCP_char_ram,
		*TC0480SCP_bgscroll_ram[4];
static int TC0480SCP_bgscrollx[4];
static int TC0480SCP_bgscrolly[4];

/* We keep two tilemaps for each of the 5 actual tilemaps: one at standard width, one double */
static struct tilemap *TC0480SCP_tilemap[5][2];
static char *TC0480SCP_char_dirty;
static int TC0480SCP_chars_dirty;
static int TC0480SCP_bg_gfx,TC0480SCP_tx_gfx;
static int TC0480SCP_tile_colbase;
static int TC0480SCP_dblwidth;
static int TC0480SCP_x_offs;
static int TC0480SCP_y_offs;

int TC0480SCP_pri_reg;   // read externally in vidhrdw\taito_f2.c


INLINE void common_get_tc0480bg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int code = ram[2*tile_index + 1] & 0x7fff;
	int attr = ram[2*tile_index];
	SET_TILE_INFO(gfxnum,code,(attr & 0xff) + TC0480SCP_tile_colbase);
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

INLINE void common_get_tc0480tx_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int attr = ram[tile_index];
	SET_TILE_INFO(gfxnum,attr & 0xff,((attr & 0x3f00) >> 8) + TC0480SCP_tile_colbase);   // >> 8 not 6 as 4bpp
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

static void TC0480SCP_get_bg0_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[0],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg1_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[1],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg2_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[2],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg3_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[3],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_tx_tile_info(int tile_index)
{
	common_get_tc0480tx_tile_info(TC0480SCP_tx_ram,TC0480SCP_tx_gfx,tile_index);
}

void (*tc480_get_tile_info[5])(int tile_index) =
{
	TC0480SCP_get_bg0_tile_info, TC0480SCP_get_bg1_tile_info, TC0480SCP_get_bg2_tile_info, TC0480SCP_get_bg3_tile_info, TC0480SCP_get_tx_tile_info
};


static struct GfxLayout TC0480SCP_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
#ifdef LSB_FIRST
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
	{ 3*4, 2*4, 1*4, 0*4, 7*4, 6*4, 5*4, 4*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

void TC0480SCP_set_layer_ptrs(void)
{
	if (!TC0480SCP_dblwidth)
	{
		TC0480SCP_bg_ram[0]	  = TC0480SCP_ram + 0x0000; //0000
		TC0480SCP_bg_ram[1]	  = TC0480SCP_ram + 0x0800; //1000
		TC0480SCP_bg_ram[2]	  = TC0480SCP_ram + 0x1000; //2000
		TC0480SCP_bg_ram[3]	  = TC0480SCP_ram + 0x1800; //3000
		TC0480SCP_bgscroll_ram[0] = TC0480SCP_ram + 0x2000; //4000
		TC0480SCP_bgscroll_ram[1] = TC0480SCP_ram + 0x2200; //4400
		TC0480SCP_bgscroll_ram[2] = TC0480SCP_ram + 0x2400; //4800
		TC0480SCP_bgscroll_ram[3] = TC0480SCP_ram + 0x2600; //4c00
		TC0480SCP_tx_ram		  = TC0480SCP_ram + 0x6000; //c000
		TC0480SCP_char_ram	  = TC0480SCP_ram + 0x7000; //e000
	}
	else
	{
		TC0480SCP_bg_ram[0]	  = TC0480SCP_ram + 0x0000; //0000
		TC0480SCP_bg_ram[1]	  = TC0480SCP_ram + 0x1000; //2000
		TC0480SCP_bg_ram[2]	  = TC0480SCP_ram + 0x2000; //4000
		TC0480SCP_bg_ram[3]	  = TC0480SCP_ram + 0x3000; //6000
		TC0480SCP_bgscroll_ram[0] = TC0480SCP_ram + 0x4000; //8000 ??
		TC0480SCP_bgscroll_ram[1] = TC0480SCP_ram + 0x4200; //8400 ??
		TC0480SCP_bgscroll_ram[2] = TC0480SCP_ram + 0x4400; //8800 ??
		TC0480SCP_bgscroll_ram[3] = TC0480SCP_ram + 0x4600; //8c00 ??
		TC0480SCP_tx_ram		  = TC0480SCP_ram + 0x6000; //c000
		TC0480SCP_char_ram	  = TC0480SCP_ram + 0x7000; //e000
	}
}

int TC0480SCP_vh_start(int gfxnum,int pixels,int x_offset,int y_offset,int col_base)
{
	int gfx_index;

		int i,xd,yd;
		TC0480SCP_tile_colbase = col_base;
		TC0480SCP_dblwidth=0;

		/* Single width versions */
		TC0480SCP_tilemap[0][0] = tilemap_create(tc480_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[1][0] = tilemap_create(tc480_get_tile_info[1],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[2][0] = tilemap_create(tc480_get_tile_info[2],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[3][0] = tilemap_create(tc480_get_tile_info[3],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[4][0] = tilemap_create(tc480_get_tile_info[4],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		/* Double width versions */
		TC0480SCP_tilemap[0][1] = tilemap_create(tc480_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[1][1] = tilemap_create(tc480_get_tile_info[1],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[2][1] = tilemap_create(tc480_get_tile_info[2],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[3][1] = tilemap_create(tc480_get_tile_info[3],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[4][1] = tilemap_create(tc480_get_tile_info[4],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);	/* text layer unchanged */

		TC0480SCP_ram = malloc(TC0480SCP_RAM_SIZE);
		TC0480SCP_char_dirty = malloc(TC0480SCP_TOTAL_CHARS);

		if (!TC0480SCP_ram || !TC0480SCP_tilemap[0][0] || !TC0480SCP_tilemap[0][1] ||
				!TC0480SCP_tilemap[1][0] || !TC0480SCP_tilemap[1][1] ||
				!TC0480SCP_tilemap[2][0] || !TC0480SCP_tilemap[2][1] ||
				!TC0480SCP_tilemap[3][0] || !TC0480SCP_tilemap[3][1] ||
				!TC0480SCP_tilemap[4][0] || !TC0480SCP_tilemap[4][1])
		{
			TC0480SCP_vh_stop();
			return 1;
		}

		TC0480SCP_set_layer_ptrs();

		memset(TC0480SCP_ram,0,TC0480SCP_RAM_SIZE);
		memset(TC0480SCP_char_dirty,1,TC0480SCP_TOTAL_CHARS);
		TC0480SCP_chars_dirty = 1;

		/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0480SCP_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx((UINT8 *)TC0480SCP_char_ram,&TC0480SCP_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;

		TC0480SCP_tx_gfx = gfx_index;

		/* use the given gfx set for bg tiles */
		TC0480SCP_bg_gfx = gfxnum;

		for (i=0;i<2;i++)
		{
			tilemap_set_transparent_pen(TC0480SCP_tilemap[0][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[1][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[2][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[3][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[4][i],0);
		}

		TC0480SCP_x_offs = x_offset + pixels;
		TC0480SCP_y_offs = y_offset;

		xd = -TC0480SCP_x_offs;
		yd =  TC0480SCP_y_offs;


		for (i=0;i<2;i++)
		{
			tilemap_set_scrolldx(TC0480SCP_tilemap[0][i], xd,   319-xd);   // 40*8 = 320 (screen width)
			tilemap_set_scrolldy(TC0480SCP_tilemap[0][i], yd,   256-yd);   // 28*8 = 224 (screen height)
			tilemap_set_scrolldx(TC0480SCP_tilemap[1][i], xd,   319-xd);
			tilemap_set_scrolldy(TC0480SCP_tilemap[1][i], yd,   256-yd);
			tilemap_set_scrolldx(TC0480SCP_tilemap[2][i], xd,   319-xd);
			tilemap_set_scrolldy(TC0480SCP_tilemap[2][i], yd,   256-yd);
			tilemap_set_scrolldx(TC0480SCP_tilemap[3][i], xd,   319-xd);
			tilemap_set_scrolldy(TC0480SCP_tilemap[3][i], yd,   256-yd);
			tilemap_set_scrolldx(TC0480SCP_tilemap[4][i], xd-2, 315-xd);   /* text layer */
			tilemap_set_scrolldy(TC0480SCP_tilemap[4][i], yd,   256-yd);   /* text layer */
		}

		for (i=0;i<2;i++)
		{
			/* Both sets of bg tilemaps scrollable per pixel row */
			tilemap_set_scroll_rows(TC0480SCP_tilemap[0][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[1][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[2][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[3][i],512);
		}

	return 0;
}

void TC0480SCP_vh_stop(void)
{
		free(TC0480SCP_ram);
		TC0480SCP_ram = 0;
		free(TC0480SCP_char_dirty);
		TC0480SCP_char_dirty = 0;
}

READ16_HANDLER( TC0480SCP_word_r )
{
	return TC0480SCP_ram[offset];
}

static void TC0480SCP_word_write(offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = TC0480SCP_ram[offset];
	COMBINE_DATA(&TC0480SCP_ram[offset]);

	if (oldword != TC0480SCP_ram[offset])
	{
		if (!TC0480SCP_dblwidth)
		{
			if (offset < 0x2000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[(offset / 0x800)][TC0480SCP_dblwidth],((offset % 0x800) / 2));
			}
			else if (offset < 0x6000)
			{   /* do nothing */
			}
			else if (offset < 0x7000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],(offset - 0x6000));
			}
			else if (offset <= 0x7fff)
			{
				TC0480SCP_char_dirty[(offset - 0x7000) / 16] = 1;
				TC0480SCP_chars_dirty = 1;
			}
		}
		else
		{
			if (offset < 0x4000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[(offset / 0x1000)][TC0480SCP_dblwidth],((offset % 0x1000) / 2));
			}
			else if (offset < 0x6000)
			{   /* do nothing */
			}
			else if (offset < 0x7000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],(offset - 0x6000));
			}
			else if (offset <= 0x7fff)
			{
				TC0480SCP_char_dirty[(offset - 0x7000) / 16] = 1;
				TC0480SCP_chars_dirty = 1;
			}
		}
	}
}

WRITE16_HANDLER( TC0480SCP_word_w )
{
	TC0480SCP_word_write(offset,data,mem_mask);
}

READ16_HANDLER( TC0480SCP_ctrl_word_r )
{
	return TC0480SCP_ctrl[offset];
}

static void TC0480SCP_ctrl_word_write(offs_t offset,data16_t data,UINT32 mem_mask)
{
	int flip = TC0480SCP_pri_reg & 0x40;

	COMBINE_DATA(&TC0480SCP_ctrl[offset]);
	data = TC0480SCP_ctrl[offset];

	switch( offset )
	{
		/* The x offsets of the four bg layers are staggered by intervals of 4 pixels */
/* NOTE:
Metalb does not always stick with this e.g. it wants bg0&1 further right
(see stick man on stairs and blue planet in attract).
There was a bg3 alignment problem with sprites in the round 4 long
spaceship boss: solved by pushing bg3 two pixels right [there may be a
single pixel left because of sprites being a frame "early"].
New offsets make "film" on skyscraper in round 1 slightly wrong.
*/
		case 0x00:   /* bg0 x */
			if (TC0480SCP_tile_colbase) data += 2;   // improves Metalb attract

			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[0] = data;
			break;

		case 0x01:   /* bg1 x */
			if (TC0480SCP_tile_colbase) data += 2;   // improves Metalb attract

			data += 4;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[1] = data;
			break;

		case 0x02:   /* bg2 x */
			if (TC0480SCP_tile_colbase) data += 2;   // same as other layers

			data += 8;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[2] = data;
			break;

		case 0x03:   /* bg3 x */
			if (TC0480SCP_tile_colbase) data += 2;   // improves Metalb round 4 boss

			data += 12;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[3] = data;
			break;

		case 0x04:   /* bg0 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[0] = data;
			break;

		case 0x05:   /* bg1 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[1] = data;
			break;

		case 0x06:   /* bg2 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[2] = data;
			break;

		case 0x07:   /* bg3 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[3] = data;
			break;

		case 0x08:   /* bg0 zoom */
		case 0x09:   /* bg1 zoom */
		case 0x0a:   /* bg2 zoom */
		case 0x0b:   /* bg3 zoom */
			break;

		case 0x0c:   /* fg (text) x */
			tilemap_set_scrollx(TC0480SCP_tilemap[4][0], 0, -data);
			tilemap_set_scrollx(TC0480SCP_tilemap[4][1], 0, -data);
			break;

		case 0x0d:   /* fg (text) y */
			tilemap_set_scrolly(TC0480SCP_tilemap[4][0], 0, -data);
			tilemap_set_scrolly(TC0480SCP_tilemap[4][1], 0, -data);
			break;

		/* offset 0x0e unused */

		case 0x0f:   /* control register */
		{
			int old_width = (TC0480SCP_pri_reg &0x80) >> 7;
			flip = (data & 0x40) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;
			TC0480SCP_pri_reg = data;

			tilemap_set_flip(TC0480SCP_tilemap[0][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[1][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[2][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[3][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[4][0],flip);

			tilemap_set_flip(TC0480SCP_tilemap[0][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[1][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[2][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[3][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[4][1],flip);

			TC0480SCP_dblwidth = (TC0480SCP_pri_reg &0x80) >> 7;

			if (TC0480SCP_dblwidth != old_width)	/* tilemap width is changing */
			{
				/* Reinitialise layer pointers */
				TC0480SCP_set_layer_ptrs();

				/* We have neglected these tilemaps, so we make amends now */
				tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[0][TC0480SCP_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[1][TC0480SCP_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[2][TC0480SCP_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[3][TC0480SCP_dblwidth]);
				tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth]);
			}

			break;
		}

		/* Rest are layer specific delta x and y, used while scrolling that layer (?) */
	}
}

WRITE16_HANDLER( TC0480SCP_ctrl_word_w )
{
	TC0480SCP_ctrl_word_write(offset,data,mem_mask);
}

void TC0480SCP_tilemap_update(void)
{
	int layer, zoom, i, j;
	int flip = TC0480SCP_pri_reg & 0x40;

	for (layer = 0; layer < 4; layer++)
	{
		tilemap_set_scrolly(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],0,TC0480SCP_bgscrolly[layer]);
		zoom = 0x10000 + 0x7f - TC0480SCP_ctrl[0x08 + layer];

		if (zoom != 0x10000)	/* currently we can't use scroll rows when zooming */
		{
			tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
					0, TC0480SCP_bgscrollx[layer]);
		}
		else
		{
			for (j = 0;j < 256;j++)
			{
				i = TC0480SCP_bgscroll_ram[layer][j];

// DG: possible issues: check yellow bg layer when you kill metalb round 1 boss.
// Top part doesn't behave like rest. But these are right for Footchmp+clones.

				if (!flip)
				tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
						(j + TC0480SCP_bgscrolly[layer] + (16 - TC0480SCP_y_offs)) & 0x1ff,
						TC0480SCP_bgscrollx[layer] - i);
				if (flip)
				tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
						(j + TC0480SCP_bgscrolly[layer] + 0x100 + (16 + TC0480SCP_y_offs)) & 0x1ff,
						TC0480SCP_bgscrollx[layer] + i);
			}
		}
	}

	/* Decode any characters that have changed */
	if (TC0480SCP_chars_dirty)
	{
		int tile_index;

		for (tile_index = 0;tile_index < 64*64;tile_index++)
		{
			int attr = TC0480SCP_tx_ram[tile_index];
			if (TC0480SCP_char_dirty[attr & 0xff])
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],tile_index);
		}

		for (j = 0;j < TC0480SCP_TOTAL_CHARS;j++)
		{
			if (TC0480SCP_char_dirty[j])
				decodechar(Machine->gfx[TC0480SCP_tx_gfx],j,(UINT8 *)TC0480SCP_char_ram,&TC0480SCP_charlayout);
			TC0480SCP_char_dirty[j] = 0;
		}
		TC0480SCP_chars_dirty = 0;
	}

	tilemap_update(TC0480SCP_tilemap[0][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[1][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[2][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[3][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[4][TC0480SCP_dblwidth]);
}


/*********************************************************************
			LAYER ZOOM (still a WIP)

1) bg layers got too far left and down, the greater the magnification.
   Largely fixed by adding offsets (to startx&y) which get bigger as
   we zoom in.

2) Hthero and Footchmp bg layers behaved differently when zoomed.
   Fixed by bringing tc0480scp_x&y_offs into calculations.

3) Metalb "TAITO" text in attract too far to the right.
   Fixed(?) by bringing (layer*4) into offset calculations.
**********************************************************************/

static void zoomtilemap_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{
	/* <0x7f = shrunk e.g. 0x1a in Footchmp hiscore; 0x7f = normal;
		0xfefe = max(?) zoom e.g. start of game in Metalb */

	int zoom = 0x10000 + 0x7f - TC0480SCP_ctrl[0x08 + layer];

	if (zoom == 0x10000)	/* no zoom, so we won't need copyrozbitmap */
	{
		tilemap_set_clip(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],&Machine->visible_area);	// prevent bad things
		tilemap_draw(bitmap,TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],flags,priority);
	}
	else	/* zoom */
	{
		UINT32 startx,starty;
		int incxx,incxy,incyx,incyy;
		struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth]);
		int flip = TC0480SCP_pri_reg & 0x40;

		tilemap_set_clip(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],0);

		if (!flip)
		{
			startx = ((TC0480SCP_bgscrollx[layer] + 16 + layer*4) << 16)
				- ((TC0480SCP_ctrl[0x10 + layer] & 0xff) << 8);	// low order byte

			incxx = zoom;
			incyx = 0;

			starty = (TC0480SCP_bgscrolly[layer] << 16)
				+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);	// low order byte
			incxy = 0;
			incyy = zoom;

			startx += (TC0480SCP_x_offs - 16 - layer*4) * incxx;
			starty -= (TC0480SCP_y_offs) * incyy;
		}
		else
		{
			startx = ((-TC0480SCP_bgscrollx[layer] + 16 + layer*4) << 16)
				- ((TC0480SCP_ctrl[0x10 + layer] & 0xff) << 8);	// low order byte

			incxx = zoom;
			incyx = 0;

			starty = (-TC0480SCP_bgscrolly[layer] << 16)
				+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);	// low order byte
			incxy = 0;
			incyy = zoom;

			startx += (TC0480SCP_x_offs - 16 - layer*4) * incxx;
			starty -= (TC0480SCP_y_offs) * incyy;
		}

		copyrozbitmap(bitmap,srcbitmap,startx,starty,
				incxx,incxy,incyx,incyy,
				1,	/* copy with wraparound */
				&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen,priority);
	}
}

void TC0480SCP_tilemap_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{

/* apparently no layer disable bits: the selectable priority must suffice */

	switch (layer)
	{
		case 0:
			zoomtilemap_draw(bitmap,0,flags,priority);
			break;
		case 1:
			zoomtilemap_draw(bitmap,1,flags,priority);
			break;
		case 2:
			zoomtilemap_draw(bitmap,2,flags,priority);
			break;
		case 3:
			zoomtilemap_draw(bitmap,3,flags,priority);
			break;
		case 4:
			tilemap_draw(bitmap,TC0480SCP_tilemap[4][TC0480SCP_dblwidth],flags,priority);
			break;
	}
}


/***************************************************************************/


static int TC0110PCR_addr;
static int TC0110PCR_1_addr;
static int TC0110PCR_2_addr;
static data16_t *TC0110PCR_ram;
static data16_t *TC0110PCR_1_ram;
static data16_t *TC0110PCR_2_ram;
#define TC0110PCR_RAM_SIZE 0x2000

int TC0110PCR_vh_start(void)
{
	TC0110PCR_ram = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram));

	if (!TC0110PCR_ram) return 1;

	return 0;
}

int TC0110PCR_1_vh_start(void)
{
	TC0110PCR_1_ram = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_1_ram));

	if (!TC0110PCR_1_ram) return 1;

	return 0;
}

int TC0110PCR_2_vh_start(void)
{
	TC0110PCR_2_ram = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_2_ram));

	if (!TC0110PCR_2_ram) return 1;

	return 0;
}

void TC0110PCR_vh_stop(void)
{
	free(TC0110PCR_ram);
	TC0110PCR_ram = 0;
}

void TC0110PCR_1_vh_stop(void)
{
	free(TC0110PCR_1_ram);
	TC0110PCR_1_ram = 0;
}

void TC0110PCR_2_vh_stop(void)
{
	free(TC0110PCR_2_ram);
	TC0110PCR_2_ram = 0;
}

READ16_HANDLER( TC0110PCR_word_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_ram[TC0110PCR_addr];

		default:
logerror("PC %06x: warning - read TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

READ16_HANDLER( TC0110PCR_word_1_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_1_ram[TC0110PCR_1_addr];

		default:
logerror("PC %06x: warning - read second TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

READ16_HANDLER( TC0110PCR_word_2_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_2_ram[TC0110PCR_2_addr];

		default:
logerror("PC %06x: warning - read third TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE16_HANDLER( TC0110PCR_word_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr = (data >> 1) & 0xfff;   /* In test mode game writes to odd register number so it is (data>>1) */
			if (data>0x1fff) logerror ("Write to palette index > 0x1fff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[TC0110PCR_addr] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[TC0110PCR_addr] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_1_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_1_addr = data & 0xfff;
			if (data>0xfff) logerror ("Write to second TC0110PCR palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_1_ram[TC0110PCR_1_addr] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			/* change a color in the second color area (4096-8191) */
			palette_change_color(TC0110PCR_1_addr + 4096,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to second TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_2_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_2_addr = data & 0xfff;
			if (data>0xfff) logerror ("Write to third TC0110PCR palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_2_ram[TC0110PCR_2_addr] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			/* change a color in the second color area (8192-12288) */
			palette_change_color(TC0110PCR_2_addr + 8192,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to third TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_rbswap_word_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_ram[TC0110PCR_addr] = data & 0xffff;

			b = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			r = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_4bpg_word_w )	/* 4 bits per color gun */
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[TC0110PCR_addr] = data & 0xffff;

			r = (data >> 0) & 0xf;
			g = (data >> 4) & 0xf;
			b = (data >> 8) & 0xf;

			r = (r << 4) | r;
			g = (g << 4) | g;
			b = (b << 4) | b;

			palette_change_color(TC0110PCR_addr,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

/***************************************************************************/


static data8_t TC0220IOC_regs[8];
static data8_t TC0220IOC_port;

READ_HANDLER( TC0220IOC_r )
{
	switch (offset)
	{
		case 0x00:	/* IN00-07 (DSA) */
			return input_port_0_r(0);

		case 0x01:	/* IN08-15 (DSB) */
			return input_port_1_r(0);

		case 0x02:	/* IN16-23 (1P) */
			return input_port_2_r(0);

		case 0x03:	/* IN24-31 (2P) */
			return input_port_3_r(0);

		case 0x04:	/* coin counters and lockout */
			return TC0220IOC_regs[4];

		case 0x07:	/* INB0-7 (coin) */
			return input_port_4_r(0);

		default:
logerror("PC %06x: warning - read TC0220IOC address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0220IOC_w )
{
	TC0220IOC_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write to TC0220IOC address %02x\n",cpu_get_pc(),offset);
			break;
	}
}

READ_HANDLER( TC0220IOC_port_r )
{
	return TC0220IOC_port;
}

WRITE_HANDLER( TC0220IOC_port_w )
{
	TC0220IOC_port = data;
}

READ_HANDLER( TC0220IOC_portreg_r )
{
	return TC0220IOC_r(TC0220IOC_port);
}

WRITE_HANDLER( TC0220IOC_portreg_w )
{
	TC0220IOC_w(TC0220IOC_port, data);
}

/***************************************************************************/


static data8_t TC0510NIO_regs[8];

READ_HANDLER( TC0510NIO_r )
{
	switch (offset)
	{
		case 0x00:	/* DSA */
			return input_port_0_r(0);

		case 0x01:	/* DSB */
			return input_port_1_r(0);

		case 0x02:	/* 1P */
			return input_port_2_r(0);

		case 0x03:	/* 2P */
			return input_port_3_r(0);

		case 0x04:	/* coin counters and lockout */
			return TC0510NIO_regs[4];

		case 0x07:	/* coin */
			return input_port_4_r(0);

		default:
logerror("PC %06x: warning - read TC0510NIO address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0510NIO_w )
{
	TC0510NIO_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write %02x to TC0510NIO address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}
