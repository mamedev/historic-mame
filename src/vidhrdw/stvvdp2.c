/* Sega Saturn VDP2 */

/*

the dirty marking stuff and tile decoding will probably be removed in the end anyway as we'll need custom
rendering code since mame's drawgfx / tilesytem don't offer everything st-v needs

this system seems far too complex to use Mame's tilemap system

4 'scroll' planes (scroll screens)

the scroll planes have slightly different capabilities

NBG0
NBG1
NBG2
NBG3

2 'rotate' planes

RBG0
RBG1

-- other crap
EXBG (external)

------------------------------------------------------------------------------------------

Notes of Interest:

-the test mode / bios is drawn with layer NBG3

-hanagumi Puts a 'RED' dragon logo in tileram (base 0x64000, 4bpp, 8x8 tiles) but
its not displayed in gurus video.Update:It's actually not drawn because its
priority value is 0.

-scrolling is screen display wise,meaning that a scrolling value is masked with the
screen resolution size values.

-VDP1 priorities aren't taken into account yet,for now we fix the priority value to six...

-AFAIK Suikoenbu & Winter Heat are the only ST-V games that use Mode 2 as the Color RAM
format.

-Bitmaps USES transparency pens,examples are:
elandore's energy bars;
mausuke's foreground(the one used on the playfield)(I guess that this is also
alpha-blended);
shanhigw's tile-based sprites;

for now I've removed black pixels,it isn't 100% right so there MUST BE a better way
for this...
Update: some games uses transparent windows,others uses a transparency pen table like this:

|------------------|---------------------|
| Character count  | Transparency code   |
|------------------|---------------------|
| 16 colors        |=0x0 (4 bits)        |
| 256 colors       |=0x00 (8 bits)       |
| 2048 colors      |=0x000 (11 bits)     |
| 32,768 colors    |MSB=0 (bit 15)       |
| 16,770,000 colors|MSB=0 (bit 31)       |
|------------------|---------------------|

In other words,the first three types uses the offset and not the color allocated...

-Debug key list(only on a debug build):
	\-T: NBG3 layer toggle
	\-Y: NBG2 layer toggle
	\-U: NBG1 layer toggle
	\-I: NBG0 layer toggle
	\-O: SPRITE toggle
	\-K: RBG0 layer toggle
	\-A: SPRITE priority decrease
	\-Q: SPRITE priority increase
	\-Z: Enables Window processing debug screen
		\-C: Window 0 toggle
		\-V: Window 1 toggle
	\-W Decodes the graphics for F4 menu.
	\-M Stores VDP1 ram contents from a file.
	\-N Stores VDP1 ram contents into a file.
*/

#include "driver.h"

data32_t* stv_vdp2_regs;
data32_t* stv_vdp2_vram;
/* this won't be used in the end .. */
data8_t*  stv_vdp2_vram_dirty_8x8x4;
data8_t*  stv_vdp2_vram_dirty_8x8x8;

static int stv_vdp2_render_rbg0;

data32_t* stv_vdp2_cram;
extern void video_update_vdp1(struct mame_bitmap *bitmap, const struct rectangle *cliprect);
extern int stv_vdp1_start ( void );
static void stv_vdp2_dynamic_res_change(void);
static void stv_vdp2_fade_effects(void);
static int stv_vdp2_window_process(int x,int y);

/*

-------------------------------------------------|-----------------------------|------------------------------
|  Function        |  Normal Scroll Screen                                     |  Rotation Scroll Screen     |
|                  |-----------------------------|-----------------------------|------------------------------
|                  | NBG0         | NBG1         | NBG2         | NBG3         | RBG0         | RBG1         |
-------------------------------------------------|-----------------------------|------------------------------
| Character Colour | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   |
| Count            | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      |
|                  | 2048 " "     | 2048 " "     |              |              | 2048 " "     | 2048 " "     |
|                  | 32768 " "    | 32768 " "    |              |              | 32768 " "    | 32768 " "    |
|                  | 16770000 " " |              |              |              | 16770000 " " | 16770000 " " |
-------------------------------------------------|-----------------------------|------------------------------
| Character Size   | 1x1 Cells , 2x2 Cells                                                                   |
-------------------------------------------------|-----------------------------|------------------------------
| Pattern Name     | 1 word , 2 words                                                                        |
| Data Size        |                                                                                         |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Size       | 1 H x 1 V 1 Pages ; 2 H x 1 V 1 Pages ; 2 H x 2 V Pages (I don't understand ... )       |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Count      | 4                                                         | 16                          |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Possible  | Yes                         | No                          | Yes          | No           |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Size      | 512 x 256                   | N/A                         | 512x256      | N/A          |
|                  | 512 x 512                   |                             | 512x512      |              |
|                  | 1024 x 256                  |                             |              |              |
|                  | 1024 x 512                  |                             |              |              |
-------------------------------------------------|-----------------------------|------------------------------
| Scale            | 0.25 x - 256 x              | None                        | Any ?                       |
-------------------------------------------------|-----------------------------|------------------------------
| Rotation         | No                                                        | Yes                         |
-------------------------------------------------|-----------------------------|-----------------------------|
| Linescroll       | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Column Scroll    | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Mosaic           | Yes                                                       | Horizontal Only             |
-------------------------------------------------|-----------------------------|------------------------------

*/

/* 180000 - r/w - TVMD - TV Screen Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | DISP     |    --    |    --    |    --    |    --    |    --    |    --    | BDCLMD   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | LSMD1    | LSMD0    | VRESO1   | VRESO0   |    --    | HRESO2   | HRESO1   | HRESO0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_TVMD 	((stv_vdp2_regs[0x000/4] >> 16)&0x0000ffff)

	#define STV_VDP2_BDCLMD	((STV_VDP2_TVMD & 0x0100) >> 8)
	#define STV_VDP2_LSMD 	((STV_VDP2_TVMD & 0x00c0) >> 6)
	#define STV_VDP2_VRES 	((STV_VDP2_TVMD & 0x0030) >> 4)
	#define STV_VDP2_HRES 	((STV_VDP2_TVMD & 0x0007) >> 0)

/* 180002 - r/w - EXTEN - External Signal Enable Register
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTEN   | EXSYEN   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    | DASEL    | EXBGEN   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180004 - r/o - TVSTAT - Screen Status
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTFG   | EXSYFG   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    | VBLANK   | HBLANK   | ODD      | EVEN     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180006 - r/w - VRSIZE - VRAM Size
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VRAMSZ   |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    | VER3     | VER2     | VER1     | VER0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_VRSIZE ((stv_vdp2_regs[0x004/4] >> 0)&0x0000ffff)

	#define STV_VDP2_VRAMSZ ((STV_VDP2_VRSIZE & 0x8000) >> 15)

/* 180008 - r/o - HCNT - H-Counter
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | HCT9     | HCT8     |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | HCT7     | HCT6     | HCT5     | HCT4     | HCT3     | HCT2     | HCT1     | HCT0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_HCNT ((stv_vdp2_regs[0x008/4] >> 16)&0x000003ff)

/* 18000A - r/o - VCNT - V-Counter
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | VCT9     | VCT8     |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCT7     | VCT6     | VCT5     | VCT4     | VCT3     | VCT2     | VCT1     | VCT0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_VCNT ((stv_vdp2_regs[0x008/4] >> 0)&0x000003ff)

/* 18000C - RESERVED
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18000E - r/w - RAMCTL - RAM Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | CRMD1    | CRMD0    |    --    |    --    | VRBMD    | VRAMD    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | RDBSB11  | RDBSB10  | RDBSB01  | RDBSB00  | RDBSA11  | RDBSA10  | RDBSA01  | RDBSA00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_RAMCTL ((stv_vdp2_regs[0x00c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_CRMD ((STV_VDP2_RAMCTL & 0x3000) >> 12)

/* 180010 - r/w - -CYCA0L - VRAM CYCLE PATTERN (BANK A0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0A03  | VCP0A02  | VCP0A01  | VCP0A00  | VCP1A03  | VCP1A02  | VCP1A01  | VCP1A00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2A03  | VCP2A02  | VCP2A01  | VCP2A00  | VCP3A03  | VCP3A02  | VCP3A01  | VCP3A00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA0L	((stv_vdp2_regs[0x010/4] >> 16)&0x0000ffff)

/* 180012 - r/w - -CYCA0U - VRAM CYCLE PATTERN (BANK A0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4A03  | VCP4A02  | VCP4A01  | VCP4A00  | VCP5A03  | VCP5A02  | VCP5A01  | VCP5A00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6A03  | VCP6A02  | VCP6A01  | VCP6A00  | VCP7A03  | VCP7A02  | VCP7A01  | VCP7A00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA0U	((stv_vdp2_regs[0x10/4] >> 0)&0x0000ffff)

/* 180014 - r/w - -CYCA1L - VRAM CYCLE PATTERN (BANK A1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0A13  | VCP0A12  | VCP0A11  | VCP0A10  | VCP1A13  | VCP1A12  | VCP1A11  | VCP1A10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2A13  | VCP2A12  | VCP2A11  | VCP2A10  | VCP3A13  | VCP3A12  | VCP3A11  | VCP3A10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA1L	((stv_vdp2_regs[0x014/4] >> 16)&0x0000ffff)

/* 180016 - r/w - -CYCA1U - VRAM CYCLE PATTERN (BANK A1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4A13  | VCP4A12  | VCP4A11  | VCP4A10  | VCP5A13  | VCP5A12  | VCP5A11  | VCP5A10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6A13  | VCP6A12  | VCP6A11  | VCP6A10  | VCP7A13  | VCP7A12  | VCP7A11  | VCP7A10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA1U	((stv_vdp2_regs[0x14/4] >> 0)&0x0000ffff)

/* 180018 - r/w - -CYCB0L - VRAM CYCLE PATTERN (BANK B0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0B03  | VCP0B02  | VCP0B01  | VCP0B00  | VCP1B03  | VCP1B02  | VCP1B01  | VCP1B00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2B03  | VCP2B02  | VCP2B01  | VCP2B00  | VCP3B03  | VCP3B02  | VCP3B01  | VCP3B00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA2L	((stv_vdp2_regs[0x018/4] >> 16)&0x0000ffff)

/* 18001A - r/w - -CYCB0U - VRAM CYCLE PATTERN (BANK B0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4B03  | VCP4B02  | VCP4B01  | VCP4B00  | VCP5B03  | VCP5B02  | VCP5B01  | VCP5B00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6B03  | VCP6B02  | VCP6B01  | VCP6B00  | VCP7B03  | VCP7B02  | VCP7B01  | VCP7B00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA2U	((stv_vdp2_regs[0x18/4] >> 0)&0x0000ffff)

/* 18001C - r/w - -CYCB1L - VRAM CYCLE PATTERN (BANK B1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0B13  | VCP0B12  | VCP0B11  | VCP0B10  | VCP1B13  | VCP1B12  | VCP1B11  | VCP1B10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2B13  | VCP2B12  | VCP2B11  | VCP2B10  | VCP3B13  | VCP3B12  | VCP3B11  | VCP3B10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA3L	((stv_vdp2_regs[0x01c/4] >> 16)&0x0000ffff)

/* 18001E - r/w - -CYCB1U - VRAM CYCLE PATTERN (BANK B1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4B13  | VCP4B12  | VCP4B11  | VCP4B10  | VCP5B13  | VCP5B12  | VCP5B11  | VCP5B10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6B13  | VCP6B12  | VCP6B11  | VCP6B10  | VCP7B13  | VCP7B12  | VCP7B11  | VCP7B10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CYCA3U	((stv_vdp2_regs[0x1c/4] >> 0)&0x0000ffff)

/* 180020 - r/w - BGON - SCREEN DISPLAY ENABLE

 this register allows each tilemap to be enabled or disabled and also which layers are solid

 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    | R0TPON   | N3TPON   | N2TPON   | N1TPON   | N0TPON   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | R1ON     | R0ON     | N3ON     | N2ON     | N1ON     | N0ON     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BGON ((stv_vdp2_regs[0x020/4] >> 16)&0x0000ffff)

	// NxOn - Layer Enable Register
	#define STV_VDP2_xxON ((STV_VDP2_BGON & 0x001f) >> 0) /* to see if anything is enabled */

	#define STV_VDP2_N0ON ((STV_VDP2_BGON & 0x0001) >> 0) /* N0On = NBG0 Enable */
	#define STV_VDP2_N1ON ((STV_VDP2_BGON & 0x0002) >> 1) /* N1On = NBG1 Enable */
	#define STV_VDP2_N2ON ((STV_VDP2_BGON & 0x0004) >> 2) /* N2On = NBG2 Enable */
	#define STV_VDP2_N3ON ((STV_VDP2_BGON & 0x0008) >> 3) /* N3On = NBG3 Enable */
	#define STV_VDP2_R0ON ((STV_VDP2_BGON & 0x0010) >> 4) /* R0On = RBG0 Enable */
	#define STV_VDP2_R1ON ((STV_VDP2_BGON & 0x0020) >> 5) /* R1On = RBG1 Enable */

	// NxTPON - Transparency Pen Enable Registers
	#define STV_VDP2_N0TPON ((STV_VDP2_BGON & 0x0100) >> 8) /* 	N0TPON = NBG0 Draw Transparent Pen (as solid) /or/ RBG1 Draw Transparent Pen */
	#define STV_VDP2_N1TPON ((STV_VDP2_BGON & 0x0200) >> 9) /* 	N1TPON = NBG1 Draw Transparent Pen (as solid) /or/ EXBG Draw Transparent Pen */
	#define STV_VDP2_N2TPON ((STV_VDP2_BGON & 0x0400) >> 10)/*  N2TPON = NBG2 Draw Transparent Pen (as solid) */
	#define STV_VDP2_N3TPON ((STV_VDP2_BGON & 0x0800) >> 11)/*  N3TPON = NBG3 Draw Transparent Pen (as solid) */
	#define STV_VDP2_R0TPON ((STV_VDP2_BGON & 0x1000) >> 12)/*  R0TPON = RBG0 Draw Transparent Pen (as solid) */

/*
180022 - MZCTL - Mosaic Control
bit->  /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/*180024 - Special Function Code Select

180026 - Special Function Code

180028 - CHCTLA - Character Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N1CHCN1  | N1CHCN0  | N1BMSZ1  | N1BMSZ0  | N1BMEN   | N1CHSZ   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N0CHCN2  | N0CHCN1  | N0CHCN0  | N0BMSZ1  | N0BMSZ0  | N0BMEN   | N0CHSZ   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CHCTLA ((stv_vdp2_regs[0x028/4] >> 16)&0x0000ffff)

/* -------------------------- NBG0 Character Control Registers -------------------------- */

/*	N0CHCNx  NBG0 (or RGB1) Colour Depth
	000 - 16 Colours
	001 - 256 Colours
	010 - 2048 Colours
	011 - 32768 Colours (RGB5)
	100 - 16770000 Colours (RGB8)
	101 - invalid
	110 - invalid
	111 - invalid   */
	#define STV_VDP2_N0CHCN ((STV_VDP2_CHCTLA & 0x0070) >> 4)

/*	N0BMSZx - NBG0 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512
	10 - 1024 x 256
	11 - 1024 x 512   */
	#define STV_VDP2_N0BMSZ ((STV_VDP2_CHCTLA & 0x000c) >> 2)

/*	N0BMEN - NBG0 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_N0BMEN ((STV_VDP2_CHCTLA & 0x0002) >> 1)

/*	N0CHSZ - NBG0 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_N0CHSZ ((STV_VDP2_CHCTLA & 0x0001) >> 0)

/* -------------------------- NBG1 Character Control Registers -------------------------- */

/*	N1CHCNx - NBG1 (or EXB1) Colour Depth
	00 - 16 Colours
	01 - 256 Colours
	10 - 2048 Colours
	11 - 32768 Colours (RGB5)  */
	#define STV_VDP2_N1CHCN ((STV_VDP2_CHCTLA & 0x3000) >> 12)

/*	N1BMSZx - NBG1 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512
	10 - 1024 x 256
	11 - 1024 x 512   */
	#define STV_VDP2_N1BMSZ ((STV_VDP2_CHCTLA & 0x0c00) >> 10)

/*	N1BMEN - NBG1 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_N1BMEN ((STV_VDP2_CHCTLA & 0x0200) >> 9)

/*	N1CHSZ - NBG1 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_N1CHSZ ((STV_VDP2_CHCTLA & 0x0100) >> 8)

/*
18002A - CHCTLB - Character Control (NBG2, NBG1, RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | R0CHCN2  | R0CHCN1  | R0CHCN0  |    --    | R0BMSZ   | R0BMEN   | R0CHSZ   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N3CHCN   | N3CHSZ   |    --    |    --    | N2CHCN   | N2CHSZ   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CHCTLB ((stv_vdp2_regs[0x028/4] >> 0)&0x0000ffff)

/* -------------------------- RBG0 Character Control Registers -------------------------- */


/*	R0CHCNx  RBG0  Colour Depth
	000 - 16 Colours
	001 - 256 Colours
	010 - 2048 Colours
	011 - 32768 Colours (RGB5)
	100 - 16770000 Colours (RGB8)
	101 - invalid
	110 - invalid
	111 - invalid   */
	#define STV_VDP2_R0CHCN ((STV_VDP2_CHCTLB & 0x7000) >> 12)

/*	R0BMSZx - RBG0 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512  */
	#define STV_VDP2_R0BMSZ ((STV_VDP2_CHCTLB & 0x0400) >> 10)

/*	R0BMEN - RBG0 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_R0BMEN ((STV_VDP2_CHCTLB & 0x0200) >> 9)

/*	R0CHSZ - NBG0 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_R0CHSZ ((STV_VDP2_CHCTLB & 0x0100) >> 8)

	#define STV_VDP2_N3CHCN ((STV_VDP2_CHCTLB & 0x0020) >> 5)
	#define STV_VDP2_N3CHSZ ((STV_VDP2_CHCTLB & 0x0010) >> 4)
	#define STV_VDP2_N2CHCN ((STV_VDP2_CHCTLB & 0x0002) >> 1)
	#define STV_VDP2_N2CHSZ ((STV_VDP2_CHCTLB & 0x0001) >> 0)


/*
18002C - BMPNA - Bitmap Palette Number (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BMPNA ((stv_vdp2_regs[0x02c/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1BMP ((STV_VDP2_BMPNA & 0x0700) >> 8)
	#define STV_VDP2_N0BMP ((STV_VDP2_BMPNA & 0x0007) >> 0)

/* 18002E - Bitmap Palette Number (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BMPNB ((stv_vdp2_regs[0x02c/4] >> 16)&0x0000ffff)

	#define STV_VDP2_R0BMP ((STV_VDP2_BMPNB & 0x0007) >> 0)

/* 180030 - PNCN0 - Pattern Name Control (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | N0PNB    | N0CNSM   |    --    |    --    |    --    |    --    | N0SPR    | N0SCC    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N0SPLT6  | N0SPLT5  | N0SPLT4  | N0SPCN4  | N0SPCN3  | N0SPCN2  | N0SPCN1  | N0SPCN0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN0 ((stv_vdp2_regs[0x030/4] >> 16)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N0PNB  ((STV_VDP2_PNCN0 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N0CNSM ((STV_VDP2_PNCN0 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N0SPR ((STV_VDP2_PNCN0 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N0SCC ((STV_VDP2_PNCN0 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N0SPLT ((STV_VDP2_PNCN0 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N0SPCN ((STV_VDP2_PNCN0 & 0x001f) >> 0)

/* 180032 - Pattern Name Control (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN1 ((stv_vdp2_regs[0x030/4] >> 0)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N1PNB  ((STV_VDP2_PNCN1 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N1CNSM ((STV_VDP2_PNCN1 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N1SPR ((STV_VDP2_PNCN1 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N1SCC ((STV_VDP2_PNCN1 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N1SPLT ((STV_VDP2_PNCN1 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N1SPCN ((STV_VDP2_PNCN1 & 0x001f) >> 0)


/* 180034 - Pattern Name Control (NBG2)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN2 ((stv_vdp2_regs[0x034/4] >> 16)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N2PNB  ((STV_VDP2_PNCN2 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N2CNSM ((STV_VDP2_PNCN2 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N2SPR ((STV_VDP2_PNCN2 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N2SCC ((STV_VDP2_PNCN2 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N2SPLT ((STV_VDP2_PNCN2 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N2SPCN ((STV_VDP2_PNCN2 & 0x001f) >> 0)


/* 180036 - Pattern Name Control (NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | N3PNB    | N3CNSM   |    --    |    --    |    --    |    --    | N3SPR    | N3SCC    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N3SPLT6  | N3SPLT5  | N3SPLT4  | N3SPCN4  | N3SPCN3  | N3SPCN2  | N3SPCN1  | N3SPCN0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN3 ((stv_vdp2_regs[0x034/4] >> 0)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N3PNB  ((STV_VDP2_PNCN3 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N3CNSM ((STV_VDP2_PNCN3 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N3SPR ((STV_VDP2_PNCN3 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N3SCC ((STV_VDP2_PNCN3 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N3SPLT ((STV_VDP2_PNCN3 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N3SPCN ((STV_VDP2_PNCN3 & 0x001f) >> 0)


/* 180038 - Pattern Name Control (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCR ((stv_vdp2_regs[0x038/4] >> 16)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_R0PNB  ((STV_VDP2_PNCR & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_R0CNSM ((STV_VDP2_PNCR & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_R0SPR ((STV_VDP2_PNCR & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_R0SCC ((STV_VDP2_PNCR & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_R0SPLT ((STV_VDP2_PNCR & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_R0SPCN ((STV_VDP2_PNCR & 0x001f) >> 0)

/* 18003A - PLSZ - Plane Size (incomplete)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N3PLSZ1  | N3PLSZ0  |    --    |    --    | N1PLSZ1  | N1PLSZ0  | N0PLSZ1  | N0PLSZ0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PLSZ ((stv_vdp2_regs[0x038/4] >> 0)&0x0000ffff)

	/* NBG0 Plane Size
	00 1H Page x 1V Page
	01 2H Pages x 1V Page
	10 invalid
	11 2H Pages x 2V Pages  */
	#define STV_VDP2_N0PLSZ ((STV_VDP2_PLSZ & 0x0003) >> 0)
	#define STV_VDP2_N1PLSZ ((STV_VDP2_PLSZ & 0x000c) >> 2)
	#define STV_VDP2_N2PLSZ ((STV_VDP2_PLSZ & 0x0030) >> 4)
	#define STV_VDP2_N3PLSZ ((STV_VDP2_PLSZ & 0x00c0) >> 6)
	#define STV_VDP2_RAPLSZ ((STV_VDP2_PLSZ & 0x0300) >> 8)
	#define STV_VDP2_RBPLSZ ((STV_VDP2_PLSZ & 0x3000) >> 12)

/* 18003C - MPOFN - Map Offset (NBG0, NBG1, NBG2, NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | N3MP8    | N3MP7    | N3MP6    |    --    | N2MP8    | N2MP7    | N2MP6    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N1MP8    | N1MP7    | N1MP6    |    --    | N0MP8    | N0MP7    | N0MP6    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPOFN_ ((stv_vdp2_regs[0x03c/4] >> 16)&0x0000ffff)

	/* Higher 3 bits of the map offset for each layer */
	#define STV_VDP2_N0MP_ ((STV_VDP2_MPOFN_ & 0x0007) >> 0)
	#define STV_VDP2_N1MP_ ((STV_VDP2_MPOFN_ & 0x0070) >> 4)
	#define STV_VDP2_N2MP_ ((STV_VDP2_MPOFN_ & 0x0700) >> 8)
	#define STV_VDP2_N3MP_ ((STV_VDP2_MPOFN_ & 0x7000) >> 12)




/* 18003E - Map Offset (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPOFR_ ((stv_vdp2_regs[0x03c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_RAMP_ ((STV_VDP2_MPOFR_ & 0x0007) >> 0)
	#define STV_VDP2_RBMP_ ((STV_VDP2_MPOFR_ & 0x0070) >> 4)

/* 180040 - MPABN0 - Map (NBG0, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N0MPB5   | N0MPB4   | N0MPB3   | N0MPB2   | N0MPB1   | N0MPB0   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N0MPA5   | N0MPA4   | N0MPA3   | N0MPA2   | N0MPA1   | N0MPA0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN0 ((stv_vdp2_regs[0x040/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG0 */
	#define STV_VDP2_N0MPB ((STV_VDP2_MPABN0 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG0 */
	#define STV_VDP2_N0MPA ((STV_VDP2_MPABN0 & 0x003f) >> 0)


/* 180042 - MPCDN0 - (NBG0, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N0MPD5   | N0MPD4   | N0MPD3   | N0MPD2   | N0MPD1   | N0MPD0   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N0MPC5   | N0MPC4   | N0MPC3   | N0MPC2   | N0MPC1   | N0MPC0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN0 ((stv_vdp2_regs[0x040/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG0 */
	#define STV_VDP2_N0MPD ((STV_VDP2_MPCDN0 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG0 */
	#define STV_VDP2_N0MPC ((STV_VDP2_MPCDN0 & 0x003f) >> 0)


/* 180044 - Map (NBG1, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN1 ((stv_vdp2_regs[0x044/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG1 */
	#define STV_VDP2_N1MPB ((STV_VDP2_MPABN1 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG1 */
	#define STV_VDP2_N1MPA ((STV_VDP2_MPABN1 & 0x003f) >> 0)

/* 180046 - Map (NBG1, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN1 ((stv_vdp2_regs[0x044/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG0 */
	#define STV_VDP2_N1MPD ((STV_VDP2_MPCDN1 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG0 */
	#define STV_VDP2_N1MPC ((STV_VDP2_MPCDN1 & 0x003f) >> 0)


/* 180048 - Map (NBG2, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN2 ((stv_vdp2_regs[0x048/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG2 */
	#define STV_VDP2_N2MPB ((STV_VDP2_MPABN2 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG2 */
	#define STV_VDP2_N2MPA ((STV_VDP2_MPABN2 & 0x003f) >> 0)

/* 18004a - Map (NBG2, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN2 ((stv_vdp2_regs[0x048/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG2 */
	#define STV_VDP2_N2MPD ((STV_VDP2_MPCDN2 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG2 */
	#define STV_VDP2_N2MPC ((STV_VDP2_MPCDN2 & 0x003f) >> 0)

/* 18004c - Map (NBG3, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN3 ((stv_vdp2_regs[0x04c/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG1 */
	#define STV_VDP2_N3MPB ((STV_VDP2_MPABN3 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG1 */
	#define STV_VDP2_N3MPA ((STV_VDP2_MPABN3 & 0x003f) >> 0)


/* 18004e - Map (NBG3, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN3 ((stv_vdp2_regs[0x04c/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG0 */
	#define STV_VDP2_N3MPD ((STV_VDP2_MPCDN3 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG0 */
	#define STV_VDP2_N3MPC ((STV_VDP2_MPCDN3 & 0x003f) >> 0)

/* 180050 - Map (Rotation Parameter A, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABRA ((stv_vdp2_regs[0x050/4] >> 16)&0x0000ffff)

	/* R0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap RBG3 */
	#define STV_VDP2_RAMPB ((STV_VDP2_MPABRA & 0x3f00) >> 8)

	/* R0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap RBG3 */
	#define STV_VDP2_RAMPA ((STV_VDP2_MPABRA & 0x003f) >> 0)



/* 180052 - Map (Rotation Parameter A, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_MPCDRA ((stv_vdp2_regs[0x050/4] >> 0)&0x0000ffff)

	/* R0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap RBG0 */
	#define STV_VDP2_RAMPD ((STV_VDP2_MPCDRA & 0x3f00) >> 8)

	/* R0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap RBG0 */
	#define STV_VDP2_RAMPC ((STV_VDP2_MPCDRA & 0x003f) >> 0)

/* 180054 - Map (Rotation Parameter A, Plane E,F)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180056 - Map (Rotation Parameter A, Plane G,H)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180058 - Map (Rotation Parameter A, Plane I,J)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005a - Map (Rotation Parameter A, Plane K,L)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005c - Map (Rotation Parameter A, Plane M,N)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005e - Map (Rotation Parameter A, Plane O,P)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180060 - Map (Rotation Parameter B, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180062 - Map (Rotation Parameter B, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180064 - Map (Rotation Parameter B, Plane E,F)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180066 - Map (Rotation Parameter B, Plane G,H)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180068 - Map (Rotation Parameter B, Plane I,J)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006a - Map (Rotation Parameter B, Plane K,L)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006c - Map (Rotation Parameter B, Plane M,N)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006e - Map (Rotation Parameter B, Plane O,P)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180070 - SCXIN0 - Screen Scroll (NBG0, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXIN0 ((stv_vdp2_regs[0x070/4] >> 16)&0x0000ffff)


/* 180072 - Screen Scroll (NBG0, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180074 - SCYIN0 - Screen Scroll (NBG0, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_SCYIN0 ((stv_vdp2_regs[0x074/4] >> 16)&0x0000ffff)


/* 180076 - Screen Scroll (NBG0, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180078 - Coordinate Inc (NBG0, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXIN0 ((stv_vdp2_regs[0x078/4] >> 16)&0x0000ffff)
	#define STV_VDP2_ZMXN0	(stv_vdp2_regs[0x078/4] & 0x007ff00)

	#define STV_VDP2_N0ZMXI ((STV_VDP2_ZMXIN0 & 0x0007) >> 0)

/* 18007a - Coordinate Inc (NBG0, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXDN0 ((stv_vdp2_regs[0x078/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N0ZMXD ((STV_VDP2_ZMXDN0 >> 8)& 0xff)

/* 18007c - Coordinate Inc (NBG0, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYIN0 ((stv_vdp2_regs[0x07c/4] >> 16)&0x0000ffff)
	#define STV_VDP2_ZMYN0	(stv_vdp2_regs[0x07c/4] & 0x007ff00)

	#define STV_VDP2_N0ZMYI ((STV_VDP2_ZMYIN0 & 0x0007) >> 0)

/* 18007e - Coordinate Inc (NBG0, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYDN0 ((stv_vdp2_regs[0x07c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N0ZMYD ((STV_VDP2_ZMYDN0 >> 8)& 0xff)

/* 180080 - SCXIN1 - Screen Scroll (NBG1, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXIN1 ((stv_vdp2_regs[0x080/4] >> 16)&0x0000ffff)

/* 180082 - Screen Scroll (NBG1, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180084 - SCYIN1 - Screen Scroll (NBG1, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYIN1 ((stv_vdp2_regs[0x084/4] >> 16)&0x0000ffff)

/* 180086 - Screen Scroll (NBG1, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180088 - Coordinate Inc (NBG1, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXIN1 ((stv_vdp2_regs[0x088/4] >> 16)&0x0000ffff)
	#define STV_VDP2_ZMXN1	(stv_vdp2_regs[0x088/4] & 0x007ff00)

	#define STV_VDP2_N1ZMXI ((STV_VDP2_ZMXIN1 & 0x0007) >> 0)

/* 18008a - Coordinate Inc (NBG1, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXDN1 ((stv_vdp2_regs[0x088/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N1ZMXD ((STV_VDP2_ZMXDN1 >> 8)& 0xff)

/* 18008c - Coordinate Inc (NBG1, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYIN1 ((stv_vdp2_regs[0x08c/4] >> 16)&0x0000ffff)
	#define STV_VDP2_ZMYN1	(stv_vdp2_regs[0x08c/4] & 0x007ff00)

	#define STV_VDP2_N1ZMYI ((STV_VDP2_ZMYIN1 & 0x0007) >> 0)

/* 18008e - Coordinate Inc (NBG1, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYDN1 ((stv_vdp2_regs[0x08c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N1ZMYD ((STV_VDP2_ZMYDN1 >> 8)& 0xff)

/* 180090 - SCXN2 - Screen Scroll (NBG2, Horizontal)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXN2 ((stv_vdp2_regs[0x090/4] >> 16)&0x0000ffff)

/* 180092 - SCYN2 - Screen Scroll (NBG2, Vertical)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYN2 ((stv_vdp2_regs[0x090/4] >> 0)&0x0000ffff)

/* 180094 - SCXN3 - Screen Scroll (NBG3, Horizontal)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXN3 ((stv_vdp2_regs[0x094/4] >> 16)&0x0000ffff)

/* 180096 - SCYN3 - Screen Scroll (NBG3, Vertical)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYN3 ((stv_vdp2_regs[0x094/4] >> 0)&0x0000ffff)

/* 180098 - Reduction Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009a - Line and Vertical Cell Scroll Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009c - Vertical Cell Table Address (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009e - Vertical Cell Table Address (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a0 - Line Scroll Table Address (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a2 - Line Scroll Table Address (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a4 - Line Scroll Table Address (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a6 - Line Scroll Table Address (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a8 - Line Colour Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800aa - Line Colour Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ac - Back Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  BKCLMD  |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  BKTA18  |  BKTA17  |  BKTA16  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ae - Back Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  BKTA15  |  BKTA14  |  BKTA13  |  BKTA12  |  BKTA11  |  BKTA10  |  BKTA9   |  BKTA8   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |  BKTA7   |  BKTA7   |  BKTA6   |  BKTA5   |  BKTA4   |  BKTA3   |  BKTA2   |  BKTA0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BKTA_UL (stv_vdp2_regs[0x0ac/4])

	#define STV_VDP2_BKCLMD ((STV_VDP2_BKTA_UL & 0x80000000) >> 31)
	#define STV_VDP2_BKTA   ((STV_VDP2_BKTA_UL & 0x0003ffff) >> 0)
	/*MSB of this register is used when the extra RAM cart is used,ignore it for now.*/
	//	#define STV_VDP2_BKTA   ((STV_VDP2_BKTA_UL & 0x0007ffff) >> 0)

/* 1800b0 - Rotation Parameter Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b2 - Rotation Parameter Read Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b4 - Coefficient Table Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b6 - Coefficient Table Address Offset (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b8 - Screen Over Pattern Name (Rotation Parameter A)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ba - Screen Over Pattern Name (Rotation Parameter B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800bc - RPTAU - Rotation Parameter Table Address (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  RPTA18  |  RPTA17  |  RPTA16  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_RPTAU	((stv_vdp2_regs[0x0bc/4] >> 16) & 0x00000007)

/* 1800be - RPTAL - Rotation Parameter Table Address (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  RPTA15  |  RPTA14  |  RPTA13  |  RPTA12  |  RPTA11  |  RPTA10  |   RPTA9  |   RPTA8  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |   RPTA7  |   RPTA6  |   RPTA5  |   RPTA4  |   RPTA3  |   RPTA2  |   RPTA1  |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_RPTAL	((stv_vdp2_regs[0x0bc/4] >> 0) & 0x0000fffe)

/* 1800c0 - Window Position (W0, Horizontal Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPSX0 ((stv_vdp2_regs[0x0c0/4] >> 16)&0x0000ffff)

	#define STV_VDP2_W0SX ((STV_VDP2_WPSX0 & 0x03ff) >> 0)

/* 1800c2 - Window Position (W0, Vertical Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPSY0 ((stv_vdp2_regs[0x0c0/4] >> 0)&0x0000ffff)

	#define STV_VDP2_W0SY ((STV_VDP2_WPSY0 & 0x03ff) >> 0)

/* 1800c4 - Window Position (W0, Horizontal End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPEX0 ((stv_vdp2_regs[0x0c4/4] >> 16)&0x0000ffff)

	#define STV_VDP2_W0EX ((STV_VDP2_WPEX0 & 0x03ff) >> 0)

/* 1800c6 - Window Position (W0, Vertical End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPEY0 ((stv_vdp2_regs[0x0c4/4] >> 0)&0x0000ffff)

	#define STV_VDP2_W0EY ((STV_VDP2_WPEY0 & 0x03ff) >> 0)

/* 1800c8 - Window Position (W1, Horizontal Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPSX1 ((stv_vdp2_regs[0x0c8/4] >> 16)&0x0000ffff)

	#define STV_VDP2_W1SX ((STV_VDP2_WPSX1 & 0x03ff) >> 0)

/* 1800ca - Window Position (W1, Vertical Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPSY1 ((stv_vdp2_regs[0x0c8/4] >> 0)&0x0000ffff)

	#define STV_VDP2_W1SY ((STV_VDP2_WPSY1 & 0x03ff) >> 0)

/* 1800cc - Window Position (W1, Horizontal End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPEX1 ((stv_vdp2_regs[0x0cc/4] >> 16)&0x0000ffff)

	#define STV_VDP2_W1EX ((STV_VDP2_WPEX1 & 0x03ff) >> 0)

/* 1800ce - Window Position (W1, Vertical End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WPEY1 ((stv_vdp2_regs[0x0cc/4] >> 0)&0x0000ffff)

	#define STV_VDP2_W1EY ((STV_VDP2_WPEY1 & 0x03ff) >> 0)

/* 1800d0 - Window Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WCTLA ((stv_vdp2_regs[0x0d0/4] >> 16)&0x0000ffff)
	#define STV_VDP2_N1LOG ((STV_VDP2_WCTLA & 0x8000) >> 15)
	#define STV_VDP2_N1SWE ((STV_VDP2_WCTLA & 0x2000) >> 13)
	#define STV_VDP2_N1SWA ((STV_VDP2_WCTLA & 0x1000) >> 12)
	#define STV_VDP2_N1W1E ((STV_VDP2_WCTLA & 0x0800) >> 11)
	#define STV_VDP2_N1W1A ((STV_VDP2_WCTLA & 0x0400) >> 10)
	#define STV_VDP2_N1W0E ((STV_VDP2_WCTLA & 0x0200) >> 9)
	#define STV_VDP2_N1W0A ((STV_VDP2_WCTLA & 0x0100) >> 8)
	#define STV_VDP2_N0LOG ((STV_VDP2_WCTLA & 0x0080) >> 7)
	#define STV_VDP2_N0SWE ((STV_VDP2_WCTLA & 0x0020) >> 5)
	#define STV_VDP2_N0SWA ((STV_VDP2_WCTLA & 0x0010) >> 4)
	#define STV_VDP2_N0W1E ((STV_VDP2_WCTLA & 0x0008) >> 3)
	#define STV_VDP2_N0W1A ((STV_VDP2_WCTLA & 0x0004) >> 2)
	#define STV_VDP2_N0W0E ((STV_VDP2_WCTLA & 0x0002) >> 1)
	#define STV_VDP2_N0W0A ((STV_VDP2_WCTLA & 0x0001) >> 0)

/* 1800d2 - Window Control (NBG2, NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WCTLB ((stv_vdp2_regs[0x0d0/4] >> 0)&0x0000ffff)
	#define STV_VDP2_N3LOG ((STV_VDP2_WCTLB & 0x8000) >> 15)
	#define STV_VDP2_N3SWE ((STV_VDP2_WCTLB & 0x2000) >> 13)
	#define STV_VDP2_N3SWA ((STV_VDP2_WCTLB & 0x1000) >> 12)
	#define STV_VDP2_N3W1E ((STV_VDP2_WCTLB & 0x0800) >> 11)
	#define STV_VDP2_N3W1A ((STV_VDP2_WCTLB & 0x0400) >> 10)
	#define STV_VDP2_N3W0E ((STV_VDP2_WCTLB & 0x0200) >> 9)
	#define STV_VDP2_N3W0A ((STV_VDP2_WCTLB & 0x0100) >> 8)
	#define STV_VDP2_N2LOG ((STV_VDP2_WCTLB & 0x0080) >> 7)
	#define STV_VDP2_N2SWE ((STV_VDP2_WCTLB & 0x0020) >> 5)
	#define STV_VDP2_N2SWA ((STV_VDP2_WCTLB & 0x0010) >> 4)
	#define STV_VDP2_N2W1E ((STV_VDP2_WCTLB & 0x0008) >> 3)
	#define STV_VDP2_N2W1A ((STV_VDP2_WCTLB & 0x0004) >> 2)
	#define STV_VDP2_N2W0E ((STV_VDP2_WCTLB & 0x0002) >> 1)
	#define STV_VDP2_N2W0A ((STV_VDP2_WCTLB & 0x0001) >> 0)

/* 1800d4 - Window Control (RBG0, Sprite)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_WCTLC ((stv_vdp2_regs[0x0d4/4] >> 16)&0x0000ffff)
	#define STV_VDP2_SPLOG ((STV_VDP2_WCTLC & 0x8000) >> 15)
	#define STV_VDP2_SPSWE ((STV_VDP2_WCTLC & 0x2000) >> 13)
	#define STV_VDP2_SPSWA ((STV_VDP2_WCTLC & 0x1000) >> 12)
	#define STV_VDP2_SPW1E ((STV_VDP2_WCTLC & 0x0800) >> 11)
	#define STV_VDP2_SPW1A ((STV_VDP2_WCTLC & 0x0400) >> 10)
	#define STV_VDP2_SPW0E ((STV_VDP2_WCTLC & 0x0200) >> 9)
	#define STV_VDP2_SPW0A ((STV_VDP2_WCTLC & 0x0100) >> 8)
	#define STV_VDP2_R0LOG ((STV_VDP2_WCTLC & 0x0080) >> 7)
	#define STV_VDP2_R0SWE ((STV_VDP2_WCTLC & 0x0020) >> 5)
	#define STV_VDP2_R0SWA ((STV_VDP2_WCTLC & 0x0010) >> 4)
	#define STV_VDP2_R0W1E ((STV_VDP2_WCTLC & 0x0008) >> 3)
	#define STV_VDP2_R0W1A ((STV_VDP2_WCTLC & 0x0004) >> 2)
	#define STV_VDP2_R0W0E ((STV_VDP2_WCTLC & 0x0002) >> 1)
	#define STV_VDP2_R0W0A ((STV_VDP2_WCTLC & 0x0001) >> 0)

/* 1800d6 - Window Control (Parameter Window, Colour Calc. Window)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d8 - Line Window Table Address (W0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800da - Line Window Table Address (W0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800dc - Line Window Table Address (W1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800de - Line Window Table Address (W1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800e0 - Sprite Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | SPCCCS1  | SPCCCS0  |    --    |  SPCCN2  |  SPCCN1  |  SPCCN0  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |  SPCLMD  | SPWINEN  |  SPTYPE3 |  SPTYPE2 |  SPTYPE1 |  SPTYPE0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SPCTL		((stv_vdp2_regs[0xe0/4] >> 16)&0x0000ffff)
	#define STV_VDP2_SPCCCS		((STV_VDP2_SPCTL & 0x3000) >> 12)
	#define STV_VDP2_SPCCN		((STV_VDP2_SPCTL & 0x700) >> 8)
	#define STV_VDP2_SPCMLD		((STV_VDP2_SPCTL & 0x20) >> 5)
	#define STV_VDP2_SPWINEN	((STV_VDP2_SPCTL & 0x10) >> 4)
	#define STV_VDP2_SPTYPE		(STV_VDP2_SPCTL & 0xf)

/* 1800e2 - Shadow Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SDCTL		((stv_vdp2_regs[0x0e0/4] >> 0) & 0x0000ffff)

/* 1800e4 - CRAOFA - Colour Ram Address Offset (NBG0 - NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | N0CAOS2  | N3CAOS1  | N3CAOS0  |    --    | N2CAOS2  | N2CAOS1  | N2CAOS0  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N1CAOS2  | N1CAOS1  | N1CAOS0  |    --    | N0CAOS2  | N0CAOS1  | N0CAOS0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CRAOFA ((stv_vdp2_regs[0x0e4/4] >> 16)&0x0000ffff)

	/* NxCAOS =  */
	#define STV_VDP2_N0CAOS ((STV_VDP2_CRAOFA & 0x0007) >> 0)
	#define STV_VDP2_N1CAOS ((STV_VDP2_CRAOFA & 0x0070) >> 4)
	#define STV_VDP2_N2CAOS ((STV_VDP2_CRAOFA & 0x0700) >> 8)
	#define STV_VDP2_N3CAOS ((STV_VDP2_CRAOFA & 0x7000) >> 12)


/* 1800e6 - Colour Ram Address Offset (RBG0, SPRITE)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_CRAOFB ((stv_vdp2_regs[0x0e4/4] >> 0)&0x0000ffff)
	#define STV_VDP2_R0CAOS ((STV_VDP2_CRAOFB & 0x0007) >> 0)
	#define STV_VDP2_SPCAOS ((STV_VDP2_CRAOFB & 0x0070) >> 4)

/* 1800e8 - Line Colour Screen Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ea - Special Priority Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ec - Colour Calculation Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  BOKEN   |  BOKN2   |  BOKN1   |   BOKN0  |    --    |  EXCCEN  |  CCRTMD  |  CCMD    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |  SPCCEN  |  LCCCEN  |  R0CCEN  |  N3CCEN  |  N2CCEN  |  N1CCEN  |  N0CCEN  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCCR		((stv_vdp2_regs[0xec/4]>>16)&0x0000ffff)
	#define STV_VDP2_SPCCEN		((STV_VDP2_CCCR & 0x40) >> 6)
	#define STV_VDP2_R0CCEN		((STV_VDP2_CCCR & 0x10) >> 3)
	#define STV_VDP2_N3CCEN		((STV_VDP2_CCCR & 0x8) >> 3)
	#define STV_VDP2_N2CCEN		((STV_VDP2_CCCR & 0x4) >> 2)
	#define STV_VDP2_N1CCEN		((STV_VDP2_CCCR & 0x2) >> 1)
	#define STV_VDP2_N0CCEN		((STV_VDP2_CCCR & 0x1) >> 0)


/* 1800ee - Special Colour Calculation Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f0 - Priority Number (Sprite 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |  S1PRIN2 |  S1PRIN1 |  S1PRIN0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  S0PRIN2 |  S0PRIN1 |  S0PRIN0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRISA		((stv_vdp2_regs[0xf0/4] >> 16) & 0x0000ffff)
	#define STV_VDP2_S1PRIN		((STV_VDP2_PRISA & 0x0700) >> 8)
	#define STV_VDP2_S0PRIN		((STV_VDP2_PRISA & 0x0007) >> 0)

/* 1800f2 - Priority Number (Sprite 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |  S3PRIN2 |  S3PRIN1 |  S3PRIN0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  S2PRIN2 |  S2PRIN1 |  S2PRIN0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRISB		((stv_vdp2_regs[0xf0/4] >> 0) & 0x0000ffff)
	#define STV_VDP2_S3PRIN		((STV_VDP2_PRISB & 0x0700) >> 8)
	#define STV_VDP2_S2PRIN		((STV_VDP2_PRISB & 0x0007) >> 0)

/* 1800f4 - Priority Number (Sprite 4,5)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |  S5PRIN2 |  S5PRIN1 |  S5PRIN0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  S4PRIN2 |  S4PRIN1 |  S4PRIN0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRISC		((stv_vdp2_regs[0xf4/4] >> 16) & 0x0000ffff)
	#define STV_VDP2_S5PRIN		((STV_VDP2_PRISC & 0x0700) >> 8)
	#define STV_VDP2_S4PRIN		((STV_VDP2_PRISC & 0x0007) >> 0)

/* 1800f6 - Priority Number (Sprite 6,7)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |  S7PRIN2 |  S7PRIN1 |  S7PRIN0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  S6PRIN2 |  S6PRIN1 |  S6PRIN0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRISD		((stv_vdp2_regs[0xf4/4] >> 0) & 0x0000ffff)
	#define STV_VDP2_S7PRIN		((STV_VDP2_PRISD & 0x0700) >> 8)
	#define STV_VDP2_S6PRIN		((STV_VDP2_PRISD & 0x0007) >> 0)


/* 1800f8 - PRINA - Priority Number (NBG 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRINA ((stv_vdp2_regs[0x0f8/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1PRIN ((STV_VDP2_PRINA & 0x0700) >> 8)
	#define STV_VDP2_N0PRIN ((STV_VDP2_PRINA & 0x0007) >> 0)

/* 1800fa - PRINB - Priority Number (NBG 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRINB ((stv_vdp2_regs[0x0f8/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N3PRIN ((STV_VDP2_PRINB & 0x0700) >> 8)
	#define STV_VDP2_N2PRIN ((STV_VDP2_PRINB & 0x0007) >> 0)

/* 1800fc - Priority Number (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_PRIR ((stv_vdp2_regs[0x0fc/4] >> 16)&0x0000ffff)

	#define STV_VDP2_R0PRIN ((STV_VDP2_PRIR & 0x0007) >> 0)

/* 1800fe - Reserved
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180100 - Colour Calculation Ratio (Sprite 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |  S1CCRT4 |  S1CCRT3 |  S1CCRT2 |  S1CCRT1 |  S1CCRT0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |  S0CCRT4 |  S0CCRT3 |  S0CCRT2 |  S0CCRT1 |  S0CCRT0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRSA		((stv_vdp2_regs[0x100/4] >> 16) & 0x0000ffff)
	#define STV_VDP2_S1CCRT		((STV_VDP2_CCRSA & 0x1f00) >> 8)
	#define STV_VDP2_S0CCRT		((STV_VDP2_CCRSA & 0x001f) >> 0)

/* 180102 - Colour Calculation Ratio (Sprite 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |  S3CCRT4 |  S3CCRT3 |  S3CCRT2 |  S3CCRT1 |  S3CCRT0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |  S2CCRT4 |  S2CCRT3 |  S2CCRT2 |  S2CCRT1 |  S2CCRT0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRSB		((stv_vdp2_regs[0x100/4] >> 0) & 0x0000ffff)
	#define STV_VDP2_S3CCRT		((STV_VDP2_CCRSB & 0x1f00) >> 8)
	#define STV_VDP2_S2CCRT		((STV_VDP2_CCRSB & 0x001f) >> 0)

/* 180104 - Colour Calculation Ratio (Sprite 4,5)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |  S5CCRT4 |  S5CCRT3 |  S5CCRT2 |  S5CCRT1 |  S5CCRT0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |  S4CCRT4 |  S4CCRT3 |  S4CCRT2 |  S4CCRT1 |  S4CCRT0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRSC		((stv_vdp2_regs[0x104/4 ]>> 16) & 0x0000ffff)
	#define STV_VDP2_S5CCRT		((STV_VDP2_CCRSC & 0x1f00) >> 8)
	#define STV_VDP2_S4CCRT		((STV_VDP2_CCRSC & 0x001f) >> 0)

/* 180106 - Colour Calculation Ratio (Sprite 6,7)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |  S7CCRT4 |  S7CCRT3 |  S7CCRT2 |  S7CCRT1 |  S7CCRT0 |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |  S6CCRT4 |  S6CCRT3 |  S6CCRT2 |  S6CCRT1 |  S6CCRT0 |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRSD		((stv_vdp2_regs[0x104/4 ]>> 0) & 0x0000ffff)
	#define STV_VDP2_S7CCRT		((STV_VDP2_CCRSD & 0x1f00) >> 8)
	#define STV_VDP2_S6CCRT		((STV_VDP2_CCRSD & 0x001f) >> 0)

/* 180108 - Colour Calculation Ratio (NBG 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    | N1CCRT4  | N1CCRT3  | N1CCRT2  | N1CCRT1  | N1CCRT0  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    | N0CCRT4  | N0CCRT3  | N0CCRT2  | N0CCRT1  | N0CCRT0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRNA	((stv_vdp2_regs[0x108/4] >> 16)&0x0000ffff)
	#define STV_VDP2_N1CCRT	((STV_VDP2_CCRNA & 0x1f00) >> 8)
	#define STV_VDP2_N0CCRT (STV_VDP2_CCRNA & 0x1f)

/* 18010a - Colour Calculation Ratio (NBG 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    | N3CCRT4  | N3CCRT3  | N3CCRT2  | N3CCRT1  | N3CCRT0  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    | N2CCRT4  | N2CCRT3  | N2CCRT2  | N2CCRT1  | N2CCRT0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRNB	((stv_vdp2_regs[0x108/4] >> 0)&0x0000ffff)
	#define STV_VDP2_N3CCRT	((STV_VDP2_CCRNA & 0x1f00) >> 8)
	#define STV_VDP2_N2CCRT (STV_VDP2_CCRNA & 0x1f)

/* 18010c - Colour Calculation Ratio (RBG 0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CCRR	((stv_vdp2_regs[0x10c/4] >> 16)&0x0000ffff)
	#define STV_VDP2_R0CCRT (STV_VDP2_CCRR & 0x1f)

/* 18010e - Colour Calculation Ratio (Line Colour Screen, Back Colour Screen)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180110 - Colour Offset Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CLOFEN ((stv_vdp2_regs[0x110/4] >> 16)&0x0000ffff)
	#define STV_VDP2_N0COEN ((STV_VDP2_CLOFEN & 0x01) >> 0)
	#define STV_VDP2_N1COEN ((STV_VDP2_CLOFEN & 0x02) >> 1)
	#define STV_VDP2_N2COEN ((STV_VDP2_CLOFEN & 0x04) >> 2)
	#define STV_VDP2_N3COEN ((STV_VDP2_CLOFEN & 0x08) >> 3)
	#define STV_VDP2_R0COEN ((STV_VDP2_CLOFEN & 0x10) >> 4)
	#define STV_VDP2_BKCOEN ((STV_VDP2_CLOFEN & 0x20) >> 5)
	#define STV_VDP2_SPCOEN ((STV_VDP2_CLOFEN & 0x40) >> 6)

/* 180112 - Colour Offset Select
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CLOFSL ((stv_vdp2_regs[0x110/4] >> 0)&0x0000ffff)
	#define STV_VDP2_N0COSL ((STV_VDP2_CLOFSL & 0x01) >> 0)
	#define STV_VDP2_N1COSL ((STV_VDP2_CLOFSL & 0x02) >> 1)
	#define STV_VDP2_N2COSL ((STV_VDP2_CLOFSL & 0x04) >> 2)
	#define STV_VDP2_N3COSL ((STV_VDP2_CLOFSL & 0x08) >> 3)
	#define STV_VDP2_R0COSL ((STV_VDP2_CLOFSL & 0x10) >> 4)
	#define STV_VDP2_BKCOSL ((STV_VDP2_CLOFSL & 0x20) >> 5)
	#define STV_VDP2_SPCOSL ((STV_VDP2_CLOFSL & 0x40) >> 6)

/* 180114 - Colour Offset A (Red)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_COAR ((stv_vdp2_regs[0x114/4] >> 16)&0x0000ffff)

/* 180116 - Colour Offset A (Green)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_COAG ((stv_vdp2_regs[0x114/4] >> 0)&0x0000ffff)

/* 180118 - Colour Offset A (Blue)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_COAB ((stv_vdp2_regs[0x118/4] >> 16)&0x0000ffff)

/* 18011a - Colour Offset B (Red)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_COBR ((stv_vdp2_regs[0x118/4] >> 0)&0x0000ffff)

/* 18011b - Colour Offset B (Green)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_COBG ((stv_vdp2_regs[0x11c/4] >> 16)&0x0000ffff)

/* 18011c - Colour Offset B (Blue)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
	#define STV_VDP2_COBB ((stv_vdp2_regs[0x11c/4] >> 0)&0x0000ffff)

/*For Debug purposes only*/
static struct stv_vdp2_debugging
{
	UINT8 l_en;	 /*For Layer enable/disable*/
	UINT8 s_pri; /*Sprite Priority Number*/
	UINT8 win;	 /*Enters into Window effect debug menu*/
} debug;

/* Not sure if to use this for the rotating tilemaps as well or just use different draw functions, might add too much bloat */
static struct stv_vdp2_tilemap_capabilities
{
	UINT8  enabled;
	UINT8  transparency;
	UINT8  colour_depth;
	UINT8  tile_size;
	UINT8  bitmap_enable;
	UINT8  bitmap_size;
	UINT8  bitmap_palette_number;
	UINT8  bitmap_map;
	UINT16 map_offset[16];

	UINT8  pattern_data_size;
	UINT8  character_number_supplement;
	UINT8  special_priority_register;
	UINT8  special_colour_control_register;
	UINT8  supplementary_palette_bits;
	UINT8  supplementary_character_bits;

	INT16 scrollx;
	INT16 scrolly;
	UINT32 incx, incy;

	UINT8  plane_size;
	UINT8  colour_ram_address_offset;
	UINT8  fade_control;
	UINT8  window_control;

	UINT8  real_map_offset[16];

	int layer_name; /* just to keep track */
} stv2_current_tilemap;

static struct rotation_table
{
	INT32	xst;
	INT32	yst;
	INT32	zst;
	INT32	dxst;
	INT32	dyst;
	INT32	dx;
	INT32	dy;
	INT32	A;
	INT32	B;
	INT32	C;
	INT32	D;
	INT32	E;
	INT32	F;
	INT32	px;
	INT32	py;
	INT32	pz;
	INT32	cx;
	INT32	cy;
	INT32	cz;
	INT32	mx;
	INT32	my;
	INT32	kx;
	INT32	ky;
	UINT32	kast;
	INT32	dkast;
	INT32	dkax;

} stv_current_rotation_parameter_table;

static void stv_vdp2_fill_rotation_parameter_table( UINT8 rot_parameter )
{
	UINT32 address;

	address = (((STV_VDP2_RPTAU << 16) | STV_VDP2_RPTAL) << 1);
	if ( rot_parameter == 1 )
	{
		address &= ~0x00000080;
	}
	else if ( rot_parameter == 2 )
	{
		address |= 0x00000080;
	}

	stv_current_rotation_parameter_table.xst  = (stv_vdp2_vram[address/4] & 0x1fffffc0) | ((stv_vdp2_vram[address/4] & 0x10000000) ? 0xe0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.yst  = (stv_vdp2_vram[address/4 + 1] & 0x1fffffc0) | ((stv_vdp2_vram[address/4 + 1] & 0x10000000) ? 0xe0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.zst  = (stv_vdp2_vram[address/4 + 2] & 0x1fffffc0) | ((stv_vdp2_vram[address/4 + 2] & 0x10000000) ? 0xe0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.dxst = (stv_vdp2_vram[address/4 + 3] & 0x0007ffc0) | ((stv_vdp2_vram[address/4 + 3] & 0x00040000) ? 0xfff80000 : 0x00000000 );
	stv_current_rotation_parameter_table.dyst = (stv_vdp2_vram[address/4 + 4] & 0x0007ffc0) | ((stv_vdp2_vram[address/4 + 4] & 0x00040000) ? 0xfff80000 : 0x00000000 );
	stv_current_rotation_parameter_table.dx   = (stv_vdp2_vram[address/4 + 5] & 0x0007ffc0) | ((stv_vdp2_vram[address/4 + 5] & 0x00040000) ? 0xfff80000 : 0x00000000 );
	stv_current_rotation_parameter_table.dy   = (stv_vdp2_vram[address/4 + 6] & 0x0007ffc0) | ((stv_vdp2_vram[address/4 + 6] & 0x00040000) ? 0xfff80000 : 0x00000000 );
	stv_current_rotation_parameter_table.A	  = (stv_vdp2_vram[address/4 + 7] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 7] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.B    = (stv_vdp2_vram[address/4 + 8] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 8] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.C    = (stv_vdp2_vram[address/4 + 9] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 9] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.D    = (stv_vdp2_vram[address/4 + 10] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 10] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.E    = (stv_vdp2_vram[address/4 + 11] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 11] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.F    = (stv_vdp2_vram[address/4 + 12] & 0x000fffc0) | ((stv_vdp2_vram[address/4 + 12] & 0x00080000) ? 0xfff00000 : 0x00000000 );
	stv_current_rotation_parameter_table.px	  = (stv_vdp2_vram[address/4 + 13] & 0x3fff0000) | ((stv_vdp2_vram[address/4 + 13] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.py	  = (stv_vdp2_vram[address/4 + 13] & 0x00003fff) << 16;
	stv_current_rotation_parameter_table.pz   = (stv_vdp2_vram[address/4 + 14] & 0x3fff0000) | ((stv_vdp2_vram[address/4 + 14] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.cx   = (stv_vdp2_vram[address/4 + 15] & 0x3fff0000) | ((stv_vdp2_vram[address/4 + 15] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.cy   = (stv_vdp2_vram[address/4 + 15] & 0x00003fff) << 16;
	stv_current_rotation_parameter_table.cz   = (stv_vdp2_vram[address/4 + 16] & 0x3fff0000) | ((stv_vdp2_vram[address/4 + 16] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.mx   = (stv_vdp2_vram[address/4 + 17] & 0x3fffffc0) | ((stv_vdp2_vram[address/4 + 17] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.my   = (stv_vdp2_vram[address/4 + 18] & 0x3fffffc0) | ((stv_vdp2_vram[address/4 + 18] & 0x30000000) ? 0xc0000000 : 0x00000000 );
	stv_current_rotation_parameter_table.kx   = (stv_vdp2_vram[address/4 + 19] & 0x00ffffff) | ((stv_vdp2_vram[address/4 + 19] & 0x00800000) ? 0xff000000 : 0x00000000 );
	stv_current_rotation_parameter_table.ky   = (stv_vdp2_vram[address/4 + 20] & 0x00ffffff) | ((stv_vdp2_vram[address/4 + 20] & 0x00800000) ? 0xff000000 : 0x00000000 );
	stv_current_rotation_parameter_table.kast = (stv_vdp2_vram[address/4 + 21] & 0xffffffc0);
	stv_current_rotation_parameter_table.dkast= (stv_vdp2_vram[address/4 + 22] & 0x03ffffc0) | ((stv_vdp2_vram[address/4 + 22] & 0x02000000) ? 0xfc000000 : 0x00000000 );
	stv_current_rotation_parameter_table.dkax = (stv_vdp2_vram[address/4 + 23] & 0x03ffffc0) | ((stv_vdp2_vram[address/4 + 23] & 0x02000000) ? 0xfc000000 : 0x00000000 );

#define RP	stv_current_rotation_parameter_table

	logerror( "Rotation parameter table (%d)\n", rot_parameter );
	logerror( "xst = %x, yst = %x, zst = %x\n", RP.xst, RP.yst, RP.zst );
	logerror( "dxst = %x, dyst = %x\n", RP.dxst, RP.dyst );
	logerror( "dx = %x, dy = %x\n", RP.dx, RP.dy );
	logerror( "A = %x, B = %x, C = %x, D = %x, E = %x, F = %x\n", RP.A, RP.B, RP.C, RP.D, RP.E, RP.F );
	logerror( "px = %x, py = %x, pz = %x\n", RP.px, RP.py, RP.pz );
	logerror( "cx = %x, cy = %x, cz = %x\n", RP.cx, RP.cy, RP.cz );
	logerror( "mx = %x, my = %x\n", RP.mx, RP.my );
	logerror( "kx = %x, ky = %x\n", RP.kx, RP.ky );
	logerror( "kast = %x, dkast = %x, dkax = %x\n", RP.kast, RP.dkast, RP.dkax );
}

#define STV_VDP2_CP_NBG0_PNMDR		0x0
#define STV_VDP2_CP_NBG1_PNMDR		0x1
#define STV_VDP2_CP_NBG2_PNMDR		0x2
#define STV_VDP2_CP_NBG3_PNMDR		0x3
#define STV_VDP2_CP_NBG0_CPDR		0x4
#define STV_VDP2_CP_NBG1_CPDR		0x5
#define STV_VDP2_CP_NBG2_CPDR		0x6
#define STV_VDP2_CP_NBG3_CPDR		0x7

static UINT8 stv_vdp2_check_vram_cycle_pattern_registers( 
								UINT8 access_command_pnmdr, 
								UINT8 access_command_cpdr,
								UINT8 bitmap_enable )
{
	int i;
	UINT8  access_command_ok = 0;
	UINT16 cp_regs[8]; 
	cp_regs[0] = STV_VDP2_CYCA0L;
	cp_regs[1] = STV_VDP2_CYCA0U;
	cp_regs[2] = STV_VDP2_CYCA1L;
	cp_regs[3] = STV_VDP2_CYCA1U;
	cp_regs[4] = STV_VDP2_CYCA2L;
	cp_regs[5] = STV_VDP2_CYCA2U;
	cp_regs[6] = STV_VDP2_CYCA3L;
	cp_regs[7] = STV_VDP2_CYCA3U;

	if ( bitmap_enable ) access_command_ok = 1;

	for ( i = 0; i < 8; i++ )
	{
		if ( ((cp_regs[i] >> 12) & 0xf) == access_command_pnmdr )
		{
			access_command_ok |= 1;
		}
		if ( ((cp_regs[i] >> 12) & 0xf) == access_command_cpdr )
		{
			access_command_ok |= 2;
		}
		if ( ((cp_regs[i] >> 8) & 0xf) == access_command_pnmdr )
		{
			access_command_ok |= 1;
		}
		if ( ((cp_regs[i] >> 8) & 0xf) == access_command_cpdr )
		{
			access_command_ok |= 2;
		}
		if ( ((cp_regs[i] >> 4) & 0xf) == access_command_pnmdr )
		{
			access_command_ok |= 1;
		}
		if ( ((cp_regs[i] >> 4) & 0xf) == access_command_cpdr )
		{
			access_command_ok |= 2;
		}
		if ( ((cp_regs[i] >> 0) & 0xf) == access_command_pnmdr )
		{
			access_command_ok |= 1;
		}
		if ( ((cp_regs[i] >> 0) & 0xf) == access_command_cpdr )
		{
			access_command_ok |= 2;
		}
	}
	return access_command_ok == 3 ? 1 : 0;
}

static void stv_vdp2_drawgfxzoom( struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scalex, int scaley,
		int sprite_screen_width, int sprite_screen_height)
{
	struct rectangle myclip;

	if (!scalex || !scaley) return;

	/*
	scalex and scaley are 16.16 fixed point numbers
	1<<15 : shrink to 50%
	1<<16 : uniform scale
	1<<17 : double to 200%
	*/


	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}

	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
		UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;

		//int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
		//int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;

		if (sprite_screen_width && sprite_screen_height)
		{
			/* compute sprite increment per screen pixel */
			//int dx = (gfx->width<<16)/sprite_screen_width;
			//int dy = (gfx->height<<16)/sprite_screen_height;
			int dx = stv2_current_tilemap.incx;
			int dy = stv2_current_tilemap.incy;

			int ex = sx+sprite_screen_width;
			int ey = sy+sprite_screen_height;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = (sprite_screen_width-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = (sprite_screen_height-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
				int y;

				/* case 0: TRANSPARENCY_NONE */
				if (transparency == TRANSPARENCY_NONE)
				{
					if (gfx->flags & GFX_PACKED)
					{
						for( y=sy; y<ey; y++ )
						{
							UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
							UINT16 *dest = (UINT16 *)dest_bmp->line[y];

							int x, x_index = x_index_base;
							for( x=sx; x<ex; x++ )
							{
								dest[x] = pal[(source[x_index>>17] >> ((x_index & 0x10000) >> 14)) & 0x0f];
								x_index += dx;
							}

							y_index += dy;
						}
					}
					else
					{
						for( y=sy; y<ey; y++ )
						{
							UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
							UINT16 *dest = (UINT16 *)dest_bmp->line[y];

							int x, x_index = x_index_base;
							for( x=sx; x<ex; x++ )
							{
								dest[x] = pal[source[x_index>>16]];
								x_index += dx;
							}

							y_index += dy;
						}
					}
				}

				/* case 1: TRANSPARENCY_PEN */
				if (transparency == TRANSPARENCY_PEN)
				{
					if (gfx->flags & GFX_PACKED)
					{
						for( y=sy; y<ey; y++ )
						{
							UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
							UINT16 *dest = (UINT16 *)dest_bmp->line[y];

							int x, x_index = x_index_base;
							for( x=sx; x<ex; x++ )
							{
								int c = (source[x_index>>17] >> ((x_index & 0x10000) >> 14)) & 0x0f;
								if( c != transparent_color ) dest[x] = pal[c];
								x_index += dx;
							}

							y_index += dy;
						}
					}
					else
					{
						for( y=sy; y<ey; y++ )
						{
							UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
							UINT16 *dest = (UINT16 *)dest_bmp->line[y];

							int x, x_index = x_index_base;
							for( x=sx; x<ex; x++ )
							{
								int c = source[x_index>>16];
								if( c != transparent_color ) dest[x] = pal[c];
								x_index += dx;
							}

							y_index += dy;
						}
					}
				}

				/* case 6: TRANSPARENCY_ALPHA */
				if (transparency == TRANSPARENCY_ALPHA)
				{
					for( y=sy; y<ey; y++ )
					{
						UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
						UINT16 *dest = (UINT16 *)dest_bmp->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if( c != transparent_color ) dest[x] = alpha_blend16(dest[x], pal[c]);
							x_index += dx;
						}

						y_index += dy;
					}
				}
			}
		}
	}

}

static void stv_vdp2_compute_color_offset_RGB555( int *r, int *g, int *b, int cor )
{
	*r <<= 3;
	*g <<= 3;
	*b <<= 3;
	if ( cor == 0 )
	{
		*r = (STV_VDP2_COAR & 0x100) ? (*r - (0x100 - (STV_VDP2_COAR & 0xff))) : ((STV_VDP2_COAR & 0xff) + *r);
		*g = (STV_VDP2_COAG & 0x100) ? (*g - (0x100 - (STV_VDP2_COAG & 0xff))) : ((STV_VDP2_COAG & 0xff) + *g);
		*b = (STV_VDP2_COAB & 0x100) ? (*b - (0x100 - (STV_VDP2_COAB & 0xff))) : ((STV_VDP2_COAB & 0xff) + *b);
	}
	else
	{
		*r = (STV_VDP2_COBR & 0x100) ? (*r - (0xff - (STV_VDP2_COBR & 0xff))) : ((STV_VDP2_COBR & 0xff) + *r);
		*g = (STV_VDP2_COBG & 0x100) ? (*g - (0xff - (STV_VDP2_COBG & 0xff))) : ((STV_VDP2_COBG & 0xff) + *g);
		*b = (STV_VDP2_COBB & 0x100) ? (*b - (0xff - (STV_VDP2_COBB & 0xff))) : ((STV_VDP2_COBB & 0xff) + *b);
	}
	if(*r < 0) 		{ *r = 0; }
	if(*r > 0xff) 	{ *r = 0xff; }
	if(*g < 0) 		{ *g = 0; }
	if(*g > 0xff) 	{ *g = 0xff; }
	if(*b < 0) 		{ *b = 0; }
	if(*b > 0xff) 	{ *b = 0xff; }
	*r >>= 3;
	*g >>= 3;
	*b >>= 3;

}

static void stv_vdp2_draw_basic_bitmap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
//	logerror ("bitmap enable %02x size %08x depth %08x\n",	stv2_current_tilemap.layer_name, stv2_current_tilemap.bitmap_size, stv2_current_tilemap.colour_depth);
//	usrintf_showmessage ("bitmap enable %02x size %08x depth %08x number %02x",	stv2_current_tilemap.layer_name, stv2_current_tilemap.bitmap_size, stv2_current_tilemap.colour_depth,stv2_current_tilemap.bitmap_palette_number);

	int xsize = 0;
	int ysize = 0;
	int xlinesize = 0, xpixelsize = 0;
	int xcnt,ycnt;
	data8_t* gfxdata = memory_region(REGION_GFX1);
	static UINT16 *destline;
	UINT16 pal_color_offset = 0;
	data8_t* gfxdatalow, *gfxdatahigh;


	int tw;
	/*Transparency code 1=opaque,0=transparent*/
	int t_pen;
	if (!stv2_current_tilemap.enabled) return;

	/* size for n0 / n1 */
	switch (stv2_current_tilemap.bitmap_size)
	{
		case 0: xsize=512; ysize=256; break;
		case 1: xsize=512; ysize=512; break;
		case 2: xsize=1024; ysize=256; break;
		case 3: xsize=1024; ysize=512; break;
	}

	switch( stv2_current_tilemap.colour_depth )
	{
		case 0: xlinesize = xsize / 2; xpixelsize = 0; break;
		case 1: xlinesize = xsize; xpixelsize = 1; break;
		case 2: xlinesize = xsize * 2; xpixelsize = 2; break;
		case 3: xlinesize = xsize * 4; xpixelsize = 4; break;
	}

	if(stv2_current_tilemap.colour_depth == 0)
		stv2_current_tilemap.scrollx /= 2;
	if(stv2_current_tilemap.colour_depth == 2)
		stv2_current_tilemap.scrollx*=2;
	if(stv2_current_tilemap.colour_depth == 3)
		stv2_current_tilemap.scrollx*=4;

	gfxdatalow = gfxdata + stv2_current_tilemap.bitmap_map * 0x20000;
	gfxdata+=(
	(stv2_current_tilemap.scrollx & (xlinesize-1)) +
	((stv2_current_tilemap.scrolly & (ysize-1)) * (xlinesize)) +
	(stv2_current_tilemap.bitmap_map * 0x20000)
	);
	gfxdatahigh = gfxdatalow + xlinesize*ysize;

//	usrintf_showmessage("%04x %04x",stv2_current_tilemap.scrollx,stv2_current_tilemap.scrolly);

	/*Enable fading bit*/
	if(stv2_current_tilemap.fade_control & 1)
	{
				 /*Select fading bit*/
		pal_color_offset += ((stv2_current_tilemap.fade_control & 2) ? (2*2048) : (2048));
	}

	stv2_current_tilemap.bitmap_palette_number+=stv2_current_tilemap.colour_ram_address_offset;
	stv2_current_tilemap.bitmap_palette_number&=7;//safety check
	switch(stv2_current_tilemap.colour_depth)
	{
		/*Palette Format*/
		case 0:
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt+=2)
				{
					tw = stv_vdp2_window_process(xcnt,ycnt);
					if(tw == 0)
					{
						t_pen = (((gfxdata[0] & 0x0f) >> 0) != 0) ? (1) : (0);
						if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;
						if(t_pen) plot_pixel(bitmap,xcnt+1  ,ycnt,Machine->pens[((gfxdata[0] & 0x0f) >> 0) | (stv2_current_tilemap.bitmap_palette_number * 0x100) | pal_color_offset]);
					}
					tw = stv_vdp2_window_process(xcnt+1,ycnt);
					if(tw == 0)
					{
						t_pen = (((gfxdata[0] & 0xf0) >> 4) != 0) ? (1) : (0);
						if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;
						if(t_pen) plot_pixel(bitmap,xcnt,ycnt,Machine->pens[((gfxdata[0] & 0xf0) >> 4) | (stv2_current_tilemap.bitmap_palette_number * 0x100) | pal_color_offset]);
					}
					gfxdata++;
					if ( gfxdata >= gfxdatahigh ) gfxdata = gfxdatalow;
				}
			}
			break;
		case 1:
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					tw = stv_vdp2_window_process(xcnt,ycnt);
					if(tw == 0)
					{
						t_pen = ((gfxdata[0] & 0xff) != 0) ? (1) : (0);
						if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;
						if(t_pen) plot_pixel(bitmap,xcnt,ycnt,Machine->pens[(gfxdata[0] & 0xff) | (stv2_current_tilemap.bitmap_palette_number * 0x100) | pal_color_offset]);
					}
					gfxdata++;
					if ( gfxdata >= gfxdatahigh ) gfxdata = gfxdatalow;
				}
			}
			break;
		case 2:
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					tw = stv_vdp2_window_process(xcnt,ycnt);
					if(tw == 0)
					{
						t_pen = ((((gfxdata[0] & 0x07) * 0x100) | (gfxdata[1] & 0xff)) != 0) ? (1) : (0);
						if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;
						if(t_pen) plot_pixel(bitmap,xcnt,ycnt,Machine->pens[((gfxdata[0] & 0x07) * 0x100) | (gfxdata[1] & 0xff) | pal_color_offset]);
					}

					gfxdata+=2;
					if ( gfxdata >= gfxdatahigh ) gfxdata = gfxdatalow;
				}
			}
			break;
		/*RGB format*/
		/*
		M                     L
		S                     S
		B                     B
		--------BBBBBGGGGGRRRRR
		*/
		case 3:
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				destline = (UINT16 *)(bitmap->line[ycnt]);

				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					int r,g,b;

					t_pen = ((gfxdata[0] & 0x80) >> 7);
					if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;
					b = ((gfxdata[0] & 0x7c) >> 2);
					g = ((gfxdata[0] & 0x03) << 3) | ((gfxdata[1] & 0xe0) >> 5);
					r = ((gfxdata[1] & 0x1f));
					if(stv2_current_tilemap.fade_control & 1)
						stv_vdp2_compute_color_offset_RGB555(&r,&g,&b,stv2_current_tilemap.fade_control & 2);
					tw = stv_vdp2_window_process(xcnt,ycnt);
					if(tw == 0)
					{
						if(t_pen)
						{
							if ( stv2_current_tilemap.transparency == TRANSPARENCY_ALPHA )
								destline[xcnt] = alpha_blend16( destline[xcnt], b | g << 5 | r << 10 );
							else
								destline[xcnt] = b | g << 5 | r << 10;
						}
					}

					gfxdata+=2;
					if ( gfxdata >= gfxdatahigh ) gfxdata = gfxdatalow;
				}
			}
			break;
		/*
		M                              L
		S                              S
		B                              B
		--------BBBBBBBBGGGGGGGGRRRRRRRR
		*/
		case 4:
			usrintf_showmessage("BITMAP type 4 enabled"); // shanhigw 'sunsoft' after gameover
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				destline = (UINT16 *)(bitmap->line[ycnt]);

				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					int r,g,b;

					t_pen = ((gfxdata[0] & 0x80) >> 7);
					if(stv2_current_tilemap.transparency == TRANSPARENCY_NONE) t_pen = 1;

					b = (gfxdata[1] & 0xf8) >> 3;
					g = (gfxdata[2] & 0xf8) >> 3;
					r = (gfxdata[3] & 0xf8) >> 3;

					tw = stv_vdp2_window_process(xcnt,ycnt);
					if(tw == 0)
					{
						if(t_pen)
							destline[xcnt] = b | g << 5 | r << 10;
					}
					gfxdata+=4;
					if ( gfxdata >= gfxdatahigh ) gfxdata = gfxdatalow;
				}
			}
			break;
	}
}

  /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   ----------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 6-0 * 0x02000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-0 * 0x00800 |
   | 1 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-0 * 0x04000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-0 * 0x01000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 * 0x04000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 * 0x01000 |
   | 2 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 * 0x08000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 * 0x02000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-2 * 0x08000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-2 * 0x02000 |
   | 2 H x 2 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-2 * 0x10000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-2 * 0x04000 |
   --the-highest-bit-is-ignored-if-vram-is-only-4mbits------------------------*/


/*
4.2 Sega's Cell / Character Pattern / Page / Plane / Map system, aka a rather annoying thing that makes optimizations hard
 (this is only for the normal tilemaps at the moment, i haven't even thought about the ROZ ones)

Tiles:

Cells are 8x8 gfx stored in video ram, they can be of various colour depths

Character Patterns can be 8x8 or 16x16 (1 hcell x 1 vcell or 2 hcell x 2 vcell)
  (a 16x16 character pattern is 4 8x8 cells put together)

A page is made up of 64x64 cells, thats 64x64 character patterns in 8x8 mode or 32x32 character patterns in 16x16 mode.
  64 * 8  = 512 (0x200)
  32 * 16 = 512 (0x200)
A page is _always_ 512 (0x200) pixels in each direction

in 1 word mode a 32*16 x 32*16 page is 0x0800 bytes
in 1 word mode a 64*8  x 64*8  page is 0x2000 bytes
in 2 word mode a 32*16 x 32*16 page is 0x1000 bytes
in 2 word mode a 64*8  x 64*8  page is 0x4000 bytes

either 1, 2 or 4 pages make each plane depending on the plane size register (per tilemap)
  therefore each plane is either
  64 * 8 * 1 x 64 * 8 * 1 (512 x 512)
  64 * 8 * 2 x 64 * 8 * 1 (1024 x 512)
  64 * 8 * 2 x 64 * 8 * 2 (1024 x 1024)

  32 * 16 * 1 x 32 * 16 * 1 (512 x 512)
  32 * 16 * 2 x 32 * 16 * 1 (1024 x 512)
  32 * 16 * 2 x 32 * 16 * 2 (1024 x 1024)

map is always enabled?
  map is a 2x2 arrangement of planes, all 4 of the planes can be the same.

*/

static void stv_vdp2_draw_basic_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/* hopefully this is easier to follow than it is efficient .. */

	/* I call character patterns tiles .. even if they represent up to 4 tiles */

	/* Page variables */
	int pgtiles_x, pgpixels_x;
	int pgtiles_y, pgpixels_y;
	int pgsize_bytes, pgsize_dwords;

	/* Plane Variables */
	int pltiles_x, plpixels_x;
	int pltiles_y, plpixels_y;
	int plsize_bytes, plsize_dwords;

	/* Map Variables */
	int mptiles_x, mppixels_x;
	int mptiles_y, mppixels_y;
	int mpsize_bytes, mpsize_dwords;

	/* work Variables */
	int i, x, y;
	int base[4];

	int scalex,scaley;
	int tilesizex, tilesizey;
	int drawypos, drawxpos;

	if ( stv2_current_tilemap.incx == 0 || stv2_current_tilemap.incy == 0 ) return;

	scalex = (INT32)((INT64)S64(0x100000000) / (INT64)stv2_current_tilemap.incx);
	scaley = (INT32)((INT64)S64(0x100000000) / (INT64)stv2_current_tilemap.incy);
	tilesizex = scalex * 8;
	tilesizey = scaley * 8;
	drawypos = drawxpos = 0;

	/* Calculate the Number of tiles for x / y directions of each page (actually these will be the same */
	/* (2-stv2_current_tilemap.tile_size) << 5) */
	pgtiles_x = ((2-stv2_current_tilemap.tile_size) << 5); // 64 (8x8 mode) or 32 (16x16 mode)
	pgtiles_y = ((2-stv2_current_tilemap.tile_size) << 5); // 64 (8x8 mode) or 32 (16x16 mode)

	/* Calculate the Page Size in BYTES */
	/* 64 * 64 * (1 * 2) = 0x2000 bytes
	   32 * 32 * (1 * 2) = 0x0800 bytes
	   64 * 64 * (2 * 2) = 0x4000 bytes
	   32 * 32 * (2 * 2) = 0x1000 bytes */

	pgsize_bytes = (pgtiles_x * pgtiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);

   /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   ----------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 6-0 * 0x02000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-0 * 0x00800 |
   | 1 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-0 * 0x04000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-0 * 0x01000 |
   ---------------------------------------------------------------------------*/


	/* Page Dimensions are always 0x200 pixes (512x512) */
	pgpixels_x = 0x200;
	pgpixels_y = 0x200;

	/* Work out the Plane Size in tiles and Plane Dimensions (pixels) */
	switch (stv2_current_tilemap.plane_size & 3)
	{
		case 0: // 1 page * 1 page
			pltiles_x  = pgtiles_x;
			plpixels_x = pgpixels_x;
			pltiles_y  = pgtiles_y;
			plpixels_y = pgpixels_y;
			break;

		case 1: // 2 pages * 1 page
			pltiles_x  = pgtiles_x * 2;
			plpixels_x = pgpixels_x * 2;
			pltiles_y  = pgtiles_y;
			plpixels_y = pgpixels_y;
			break;

		case 3: // 2 pages * 2 pages
			pltiles_x  = pgtiles_x * 2;
			plpixels_x = pgpixels_x * 2;
			pltiles_y  = pgtiles_y * 2;
			plpixels_y = pgpixels_y * 2;
			break;

		default:
			// illegal
			pltiles_x  = pgtiles_x;
			plpixels_x = pgpixels_x;
			pltiles_y  = pgtiles_y * 2;
			plpixels_y = pgpixels_y * 2;
		break;
	}

	/* Plane Size in BYTES */
	/* still the same as before
	   (64 * 1) * (64 * 1) * (1 * 2) = 0x02000 bytes
	   (32 * 1) * (32 * 1) * (1 * 2) = 0x00800 bytes
	   (64 * 1) * (64 * 1) * (2 * 2) = 0x04000 bytes
	   (32 * 1) * (32 * 1) * (2 * 2) = 0x01000 bytes
	   changed
	   (64 * 2) * (64 * 1) * (1 * 2) = 0x04000 bytes
	   (32 * 2) * (32 * 1) * (1 * 2) = 0x01000 bytes
	   (64 * 2) * (64 * 1) * (2 * 2) = 0x08000 bytes
	   (32 * 2) * (32 * 1) * (2 * 2) = 0x02000 bytes
	   changed
	   (64 * 2) * (64 * 1) * (1 * 2) = 0x08000 bytes
	   (32 * 2) * (32 * 1) * (1 * 2) = 0x02000 bytes
	   (64 * 2) * (64 * 1) * (2 * 2) = 0x10000 bytes
	   (32 * 2) * (32 * 1) * (2 * 2) = 0x04000 bytes
	*/

	plsize_bytes = (pltiles_x * pltiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);

   /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   -----------------------------------------------------------------------------
   | 1 H x 1 V   see above, nothing has changed                                |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 * 0x04000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 * 0x01000 |
   | 2 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 * 0x08000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 * 0x02000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-2 * 0x08000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-2 * 0x02000 |
   | 2 H x 2 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-2 * 0x10000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-2 * 0x04000 |
   --the-highest-bit-is-ignored-if-vram-is-only-4mbits------------------------*/


	/* Work out the Map Sizes in tiles, Map Dimensions */
	/* maps are always enabled? */
	mptiles_x = pltiles_x * 2;
	mptiles_y = pltiles_y * 2;
	mppixels_x = plpixels_x * 2;
	mppixels_y = plpixels_y * 2;

	/* Map Size in BYTES */
	mpsize_bytes = (mptiles_x * mptiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);


   /*-----------------------------------------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 (upper mask 0x07f) (0x1ff >> 2) * 0x04000 |
   |            | 1 word                 |---------------------------------------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 (upper mask 0x1ff) (0x1ff >> 0) * 0x01000 |
   | 2 H x 1 V  -----------------------------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 (upper mask 0x03f) (0x1ff >> 3) * 0x08000 |
   |            | 2 words                |---------------------------------------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 (upper mask 0x0ff) (0x1ff >> 1) * 0x02000 |
   -------------------------------------------------------------------------------------------------------------
    lower mask = ~stv2_current_tilemap.plane_size
   -----------------------------------------------------------------------------------------------------------*/

	/* Precalculate bases from MAP registers */
	for (i = 0; i < 4; i++)
	{
		int shifttable[4] = {0,1,2,2};

		int uppermask, uppermaskshift;

		uppermaskshift = (1-stv2_current_tilemap.pattern_data_size) | ((1-stv2_current_tilemap.tile_size)<<1);
		uppermask = 0x1ff >> uppermaskshift;

		base[i] = ((stv2_current_tilemap.map_offset[i] & uppermask) >> shifttable[stv2_current_tilemap.plane_size]) * plsize_bytes;

		base[i] &= 0x7ffff; /* shienryu needs this for the text layer, is there a problem elsewhere or is it just right without the ram cart */

		base[i] = base[i] / 4; // convert bytes to DWORDS
	}

	/* other bits */
	//stv2_current_tilemap.trans_enabled = stv2_current_tilemap.trans_enabled ? TRANSPARENCY_NONE : TRANSPARENCY_PEN;
	stv2_current_tilemap.scrollx &= mppixels_x-1;
	stv2_current_tilemap.scrolly &= mppixels_y-1;

	pgsize_dwords = pgsize_bytes /4;
	plsize_dwords = plsize_bytes /4;
	mpsize_dwords = mpsize_bytes /4;

//	if (stv2_current_tilemap.layer_name==3) usrintf_showmessage ("well this is a bit  %08x", stv2_current_tilemap.map_offset[0]);
//	if (stv2_current_tilemap.layer_name==3) usrintf_showmessage ("well this is a bit  %08x %08x %08x %08x", stv2_current_tilemap.plane_size, pgtiles_x, pltiles_x, mptiles_x);

	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	/* most things we need (or don't need) to work out are now worked out */

	for (y = 0; y<mptiles_y; y++) {
		int ypageoffs, yplaneoffs;
		int page, map, newbase, offs, data;
		int tilecode, flipyx, pal, gfx;

		map = 0 ; page = 0 ;
		if ( y == 0 )
			drawypos = -(stv2_current_tilemap.scrolly*scaley);
		else
			drawypos += tilesizey*(stv2_current_tilemap.tile_size ? 2 : 1);
		if ((drawypos >> 16) > Machine->visible_area.max_y) continue;

		ypageoffs = y & (pgtiles_y-1);
		yplaneoffs = y & (pltiles_y-1);

		if (yplaneoffs > ypageoffs) page |= 2;
		if (y > yplaneoffs) map |= 2;

		for (x = 0; x<mptiles_x; x++) {
			int xpageoffs, xplaneoffs;
			if ( x == 0 )
				drawxpos = -(stv2_current_tilemap.scrollx*scalex);
			else
				drawxpos+=tilesizex*(stv2_current_tilemap.tile_size ? 2 : 1);
			if ( (drawxpos >> 16) > Machine->visible_area.max_x ) continue;

			xpageoffs = x & (pgtiles_x-1);
			xplaneoffs = x & (pltiles_x-1);

			if (xplaneoffs > xpageoffs) page |= 1;
			if (x > xplaneoffs) map |= 1;

			newbase = base[map] + page * pgsize_dwords;
			offs = (ypageoffs * pgtiles_x) + xpageoffs;

/* GET THE TILE INFO ... */
			/* 1 word per tile mode with supplement bits */
			if (stv2_current_tilemap.pattern_data_size ==1)
			{

				data = stv_vdp2_vram[newbase + offs/2];
				data = (offs&1) ? (data & 0x0000ffff) : ((data & 0xffff0000) >> 16);

				/* Supplement Mode 12 bits, no flip */
				if (stv2_current_tilemap.character_number_supplement == 1)
				{
/* no flip */		flipyx   = 0;
/* 8x8 */			if (stv2_current_tilemap.tile_size==0) tilecode = (data & 0x0fff) + ( (stv2_current_tilemap.supplementary_character_bits&0x1c) << 10);
/* 16x16 */ 		else tilecode = ((data & 0x0fff) << 2) + (stv2_current_tilemap.supplementary_character_bits&0x03) + ((stv2_current_tilemap.supplementary_character_bits&0x10) << 10);
				}
				/* Supplement Mode 10 bits, with flip */
				else
				{
/* flip bits */ 	flipyx   = (data & 0x0c00) >> 10;
/* 8x8 */			if (stv2_current_tilemap.tile_size==0) tilecode = (data & 0x03ff) +  ( (stv2_current_tilemap.supplementary_character_bits) << 10);
/* 16x16 */			else tilecode = ((data & 0x03ff) <<2) +  (stv2_current_tilemap.supplementary_character_bits&0x03) + ((stv2_current_tilemap.supplementary_character_bits&0x1c) << 10);
				}

/*>16cols*/		if (stv2_current_tilemap.colour_depth != 0) pal = ((data & 0x7000)>>8);
/*16 cols*/		else pal = ((data & 0xf000)>>12) +( (stv2_current_tilemap.supplementary_palette_bits) << 4);

			}
			/* 2 words per tile, no supplement bits */
			else
			{

				data = stv_vdp2_vram[newbase + offs];
				tilecode = (data & 0x00007fff);
				pal   = (data &    0x007f0000)>>16;
	//			specialc = (data & 0x10000000)>>28;;
				flipyx   = (data & 0xc0000000)>>30;
			}
/* WE'VE GOT THE TILE INFO ... */

/* DECODE ANY TILES WE NEED TO DECODE */

			pal += stv2_current_tilemap.colour_ram_address_offset<< 4; // bios uses this ..

			/*Enable fading bit*/
			if(stv2_current_tilemap.fade_control & 1)
			{
				/*Select fading bit*/
				pal += ((stv2_current_tilemap.fade_control & 2) ? (0x100) : (0x80));
			}

			if (stv2_current_tilemap.colour_depth != 0)
			{
				tilecode = tilecode >> 1;
				gfx = 2;
				pal = pal >>4;

				/* do a bit of tile decoding */
				tilecode &=0x3fff;
				if (stv2_current_tilemap.tile_size==1)
				{ /* we're treating 16x16 tiles as 4 8x8's atm */
					if (stv_vdp2_vram_dirty_8x8x8[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode] = 0; decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+1] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+1] = 0; decodechar(Machine->gfx[2], tilecode+1,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+2] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+2] = 0; decodechar(Machine->gfx[2], tilecode+2,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+3] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+3] = 0; decodechar(Machine->gfx[2], tilecode+3,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
				}
				else
				{
					if (stv_vdp2_vram_dirty_8x8x8[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode] = 0; decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
				}

			}
			else
			{
				gfx = 0;

				/* do a bit of tile decoding */
				tilecode &=0x7fff;
				if (stv2_current_tilemap.tile_size==1)
				{ /* we're treating 16x16 tiles as 4 8x8's atm */
					if (stv_vdp2_vram_dirty_8x8x4[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode] = 0; decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+1] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+1] = 0; decodechar(Machine->gfx[0], tilecode+1,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+2] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+2] = 0; decodechar(Machine->gfx[0], tilecode+2,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+3] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+3] = 0; decodechar(Machine->gfx[0], tilecode+3,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
				}
				else
				{
					if (stv_vdp2_vram_dirty_8x8x4[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode] = 0; decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
				}
			}
/* TILES ARE NOW DECODED */

/* DRAW! */
			if(stv2_current_tilemap.incx != 0x10000 || stv2_current_tilemap.incy != 0x10000)
			{
#define SCR_TILESIZE_X			(((drawxpos + tilesizex) >> 16) - (drawxpos >> 16))
#define SCR_TILESIZE_X1(startx)	(((drawxpos + (startx) + tilesizex) >> 16) - ((drawxpos + (startx))>>16))
#define SCR_TILESIZE_Y			(((drawypos + tilesizey) >> 16) - (drawypos >> 16))
#define SCR_TILESIZE_Y1(starty)	(((drawypos + (starty) + tilesizey) >> 16) - ((drawypos + (starty))>>16))
				if (stv2_current_tilemap.tile_size==1)
				{
					/* normal */
					stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos >> 16, drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X, SCR_TILESIZE_Y);
					stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex) >> 16,drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex), SCR_TILESIZE_Y);
					stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos >> 16,(drawypos+tilesizey) >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X, SCR_TILESIZE_Y1(tilesizey));
					stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex)>> 16,(drawypos+tilesizey) >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex), SCR_TILESIZE_Y1(tilesizey));

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
					{
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+mppixels_x*scalex)>>16, drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex),SCR_TILESIZE_Y);
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex+mppixels_x*scalex)>>16,drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex + tilesizex),SCR_TILESIZE_Y);
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+mppixels_x*scalex)>>16,(drawypos+tilesizey)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_Y1(mppixels_x*scalex),SCR_TILESIZE_Y1(tilesizey));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex+mppixels_x*scalex)>>16,(drawypos+tilesizey)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex+mppixels_x*scalex),SCR_TILESIZE_Y1(tilesizey));
					}
					if (stv2_current_tilemap.scrolly) /* wraparound y */
					{
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos >> 16, (drawypos+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X,SCR_TILESIZE_Y1(mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex)>>16,(drawypos+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex),SCR_TILESIZE_Y1(mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos >> 16,(drawypos+tilesizey+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley, SCR_TILESIZE_X, SCR_TILESIZE_Y1(tilesizey+mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex)>>16,(drawypos+tilesizey+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley, SCR_TILESIZE_X1(tilesizex), SCR_TILESIZE_Y1(tilesizey+mppixels_y*scaley));
					}
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
					{
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+mppixels_x*scalex)>>16, (drawypos+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex), SCR_TILESIZE_Y1(mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex+mppixels_x*scalex)>>16,(drawypos+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex+mppixels_x*scalex), SCR_TILESIZE_Y1(mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+mppixels_x*scalex)>>16,(drawypos+tilesizey+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex),SCR_TILESIZE_Y1(tilesizey+mppixels_y*scaley));
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,(drawxpos+tilesizex+mppixels_x*scalex)>>16,(drawypos+tilesizey+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(tilesizex+mppixels_x*scalex),SCR_TILESIZE_Y1(tilesizey+mppixels_y*scaley));
					}

				}
				else
				{
					stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos >> 16, drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X,SCR_TILESIZE_Y);
					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, (drawxpos + mppixels_x*scalex) >> 16, drawypos >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex), SCR_TILESIZE_Y);
					if (stv2_current_tilemap.scrolly) /* wraparound y */
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos >> 16, (drawypos+mppixels_y*scaley) >> 16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X, SCR_TILESIZE_Y1(mppixels_y*scaley));
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
						stv_vdp2_drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, (drawxpos+mppixels_x*scalex)>>16, (drawypos+mppixels_y*scaley)>>16,cliprect,stv2_current_tilemap.transparency,0,scalex,scaley,SCR_TILESIZE_X1(mppixels_x*scalex), SCR_TILESIZE_Y1(mppixels_y*scaley));
				}
			}
			else
			{
				int olddrawxpos, olddrawypos;
				olddrawxpos = drawxpos; drawxpos >>= 16;
				olddrawypos = drawypos; drawypos >>= 16;
				if (stv2_current_tilemap.tile_size==1)
				{
					/* normal */
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos,cliprect,stv2_current_tilemap.transparency,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos,cliprect,stv2_current_tilemap.transparency,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8,cliprect,stv2_current_tilemap.transparency,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8,cliprect,stv2_current_tilemap.transparency,0);

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.transparency,0);
					}
					if (stv2_current_tilemap.scrolly) /* wraparound y */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
					}
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
					}
				}
				else
				{
					drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos,cliprect,stv2_current_tilemap.transparency,0);

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.transparency,0);
					if (stv2_current_tilemap.scrolly) /* wraparound y */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.transparency,0);
				}
				drawxpos = olddrawxpos;
				drawypos = olddrawypos;
			}
/* DRAWN?! */

		}
	}
}

static void stv_vdp2_check_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/* the idea is here we check the tilemap capabilities / whats enabled and call an appropriate tilemap drawing routine, or
	  at the very list throw up a few errors if the tilemaps want to do something we don't support yet */

	if (stv2_current_tilemap.bitmap_enable) // this layer is a bitmap
	{
		stv_vdp2_draw_basic_bitmap(bitmap, cliprect);
	}
	else
	{
		stv_vdp2_draw_basic_tilemap(bitmap, cliprect);
	}
}


static void stv_vdp2_draw_NBG0(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   Colours           : 16, 256, 2048, 32768, 16770000
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : Possible
	   Bitmap Sizes      : 512 x 256, 512 x 512, 1024 x 256, 1024 x 512
	   Scale             : 0.25 x - 256 x
	   Rotation          : No
	   Linescroll        : Yes
	   Column Scroll     : Yes
	   Mosaic            : Yes
	*/
	stv2_current_tilemap.enabled = STV_VDP2_N0ON;

//	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	//stv2_current_tilemap.trans_enabled = STV_VDP2_N0TPON;
	if ( STV_VDP2_N0CCEN )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_ALPHA;
		alpha_set_level( ((UINT16)(0x1f-STV_VDP2_N0CCRT)*0xff)/0x1f);
	}
	else if ( STV_VDP2_N0TPON == 0 )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_PEN;
	}
	else
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_NONE;
	}
	stv2_current_tilemap.colour_depth = STV_VDP2_N0CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N0CHSZ;
	stv2_current_tilemap.bitmap_enable = STV_VDP2_N0BMEN;
	stv2_current_tilemap.bitmap_size = STV_VDP2_N0BMSZ;
	stv2_current_tilemap.bitmap_palette_number = STV_VDP2_N0BMP;
	stv2_current_tilemap.bitmap_map = STV_VDP2_N0MP_;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N0MPA | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N0MPB | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N0MPC | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N0MPD | (STV_VDP2_N0MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N0PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N0CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N0SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN0;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N0SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N0SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXIN0;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYIN0;
	stv2_current_tilemap.incx = STV_VDP2_ZMXN0;
	stv2_current_tilemap.incy = STV_VDP2_ZMYN0;

	stv2_current_tilemap.plane_size = STV_VDP2_N0PLSZ;
	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N0CAOS;
	stv2_current_tilemap.fade_control = (STV_VDP2_N0COEN * 1) | (STV_VDP2_N0COSL * 2);
	stv2_current_tilemap.window_control = (STV_VDP2_N0LOG * 0x01) |
										  (STV_VDP2_N0W0E * 0x02) |
										  (STV_VDP2_N0W1E * 0x04) |
										  (STV_VDP2_N0SWE * 0x08) |
										  (STV_VDP2_N0W0A * 0x10) |
										  (STV_VDP2_N0W1A * 0x20) |
										  (STV_VDP2_N0SWA * 0x40);

	stv2_current_tilemap.layer_name=0;

	if ( stv2_current_tilemap.enabled )
	{
		stv2_current_tilemap.enabled = stv_vdp2_check_vram_cycle_pattern_registers( STV_VDP2_CP_NBG0_PNMDR, STV_VDP2_CP_NBG0_CPDR, stv2_current_tilemap.bitmap_enable );
	}

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG1(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   Colours           : 16, 256, 2048, 32768
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : Possible
	   Bitmap Sizes      : 512 x 256, 512 x 512, 1024 x 256, 1024 x 512
	   Scale             : 0.25 x - 256 x
	   Rotation          : No
	   Linescroll        : Yes
	   Column Scroll     : Yes
	   Mosaic            : Yes
	*/
	stv2_current_tilemap.enabled = STV_VDP2_N1ON;

//	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	//stv2_current_tilemap.trans_enabled = STV_VDP2_N1TPON;
	if ( STV_VDP2_N1CCEN )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_ALPHA;
		alpha_set_level( ((UINT16)(0x1f-STV_VDP2_N1CCRT)*0xff)/0x1f);
	}
	else if ( STV_VDP2_N1TPON == 0 )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_PEN;
	}
	else
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_NONE;
	}
	stv2_current_tilemap.colour_depth = STV_VDP2_N1CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N1CHSZ;
	stv2_current_tilemap.bitmap_enable = STV_VDP2_N1BMEN;
	stv2_current_tilemap.bitmap_size = STV_VDP2_N1BMSZ;
	stv2_current_tilemap.bitmap_palette_number = STV_VDP2_N1BMP;
	stv2_current_tilemap.bitmap_map = STV_VDP2_N1MP_;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N1MPA | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N1MPB | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N1MPC | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N1MPD | (STV_VDP2_N1MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N1PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N1CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N1SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN1;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N1SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N1SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXIN1;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYIN1;
	stv2_current_tilemap.incx = STV_VDP2_ZMXN1;
	stv2_current_tilemap.incy = STV_VDP2_ZMYN1;

	stv2_current_tilemap.plane_size = STV_VDP2_N1PLSZ;
	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N1CAOS;
	stv2_current_tilemap.fade_control = (STV_VDP2_N1COEN * 1) | (STV_VDP2_N1COSL * 2);
	stv2_current_tilemap.window_control = (STV_VDP2_N1LOG * 0x01) |
										  (STV_VDP2_N1W0E * 0x02) |
										  (STV_VDP2_N1W1E * 0x04) |
										  (STV_VDP2_N1SWE * 0x08) |
										  (STV_VDP2_N1W0A * 0x10) |
										  (STV_VDP2_N1W1A * 0x20) |
										  (STV_VDP2_N1SWA * 0x40);

	stv2_current_tilemap.layer_name=1;

	if ( stv2_current_tilemap.enabled )
	{
		stv2_current_tilemap.enabled = stv_vdp2_check_vram_cycle_pattern_registers( STV_VDP2_CP_NBG1_PNMDR, STV_VDP2_CP_NBG1_CPDR, stv2_current_tilemap.bitmap_enable );
	}

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG2(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   NBG2 is the first of the 2 more basic tilemaps, it has exactly the same capabilities as NBG3

	   Colours           : 16, 256
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : No
	   Bitmap Sizes      : N/A
	   Scale             : No
	   Rotation          : No
	   Linescroll        : No
	   Column Scroll     : No
	   Mosaic            : Yes
	*/

	stv2_current_tilemap.enabled = STV_VDP2_N2ON;

	/* these modes for N0 disable this layer */
	if (STV_VDP2_N0CHCN == 0x03) stv2_current_tilemap.enabled = 0;
	if (STV_VDP2_N0CHCN == 0x04) stv2_current_tilemap.enabled = 0;

//	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	//stv2_current_tilemap.trans_enabled = STV_VDP2_N2TPON;
	if ( STV_VDP2_N2CCEN )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_ALPHA;
		alpha_set_level( ((UINT16)(0x1f-STV_VDP2_N2CCRT)*0xff)/0x1f);
	}
	else if ( STV_VDP2_N2TPON == 0 )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_PEN;
	}
	else
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_NONE;
	}
	stv2_current_tilemap.colour_depth = STV_VDP2_N2CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N2CHSZ;
	/* this layer can't be a bitmap,so ignore these registers*/
	stv2_current_tilemap.bitmap_enable = 0;
	stv2_current_tilemap.bitmap_size = 0;
	stv2_current_tilemap.bitmap_palette_number = 0;
	stv2_current_tilemap.bitmap_map = 0;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N2MPA | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N2MPB | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N2MPC | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N2MPD | (STV_VDP2_N2MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N2PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N2CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N2SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN2;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N2SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N2SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXN2;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYN2;
	/*This layer can't be scaled*/
	stv2_current_tilemap.incx = 0x10000;
	stv2_current_tilemap.incy = 0x10000;

	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N2CAOS;
	stv2_current_tilemap.fade_control = (STV_VDP2_N2COEN * 1) | (STV_VDP2_N2COSL * 2);
	stv2_current_tilemap.window_control = (STV_VDP2_N2LOG * 0x01) |
										  (STV_VDP2_N2W0E * 0x02) |
										  (STV_VDP2_N2W1E * 0x04) |
										  (STV_VDP2_N2SWE * 0x08) |
										  (STV_VDP2_N2W0A * 0x10) |
										  (STV_VDP2_N2W1A * 0x20) |
										  (STV_VDP2_N2SWA * 0x40);

	stv2_current_tilemap.layer_name=2;

	stv2_current_tilemap.plane_size = STV_VDP2_N2PLSZ;

	if ( stv2_current_tilemap.enabled )
	{
		stv2_current_tilemap.enabled = stv_vdp2_check_vram_cycle_pattern_registers( STV_VDP2_CP_NBG2_PNMDR, STV_VDP2_CP_NBG2_CPDR, stv2_current_tilemap.bitmap_enable );
	}

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG3(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   NBG3 is the second of the 2 more basic tilemaps, it has exactly the same capabilities as NBG2

	   Colours           : 16, 256
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : No
	   Bitmap Sizes      : N/A
	   Scale             : No
	   Rotation          : No
	   Linescroll        : No
	   Column Scroll     : No
	   Mosaic            : Yes
	*/

	stv2_current_tilemap.enabled = STV_VDP2_N3ON;

//	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	/* these modes for N1 disable this layer */
	if (STV_VDP2_N1CHCN == 0x03) stv2_current_tilemap.enabled = 0;
	if (STV_VDP2_N1CHCN == 0x04) stv2_current_tilemap.enabled = 0;

	//stv2_current_tilemap.trans_enabled = STV_VDP2_N3TPON;
	if ( STV_VDP2_N3CCEN )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_ALPHA;
		alpha_set_level( ((UINT16)(0x1f-STV_VDP2_N3CCRT)*0xff)/0x1f);
	}
	else if ( STV_VDP2_N3TPON == 0 )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_PEN;
	}
	else
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_NONE;
	}
	stv2_current_tilemap.colour_depth = STV_VDP2_N3CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N3CHSZ;
	/* this layer can't be a bitmap,so ignore these registers*/
	stv2_current_tilemap.bitmap_enable = 0;
	stv2_current_tilemap.bitmap_size = 0;
	stv2_current_tilemap.bitmap_palette_number = 0;
	stv2_current_tilemap.bitmap_map = 0;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N3MPA | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N3MPB | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N3MPC | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N3MPD | (STV_VDP2_N3MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N3PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N3CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N3SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_N3SCC;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N3SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N3SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXN3;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYN3;
	/*This layer can't be scaled*/
	stv2_current_tilemap.incx = 0x10000;
	stv2_current_tilemap.incy = 0x10000;

	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N3CAOS;
	stv2_current_tilemap.fade_control = (STV_VDP2_N3COEN * 1) | (STV_VDP2_N3COSL * 2);
	stv2_current_tilemap.window_control = (STV_VDP2_N3LOG * 0x01) |
										  (STV_VDP2_N3W0E * 0x02) |
										  (STV_VDP2_N3W1E * 0x04) |
										  (STV_VDP2_N3SWE * 0x08) |
										  (STV_VDP2_N3W0A * 0x10) |
										  (STV_VDP2_N3W1A * 0x20) |
										  (STV_VDP2_N3SWA * 0x40);

	stv2_current_tilemap.layer_name=3;

	stv2_current_tilemap.plane_size = STV_VDP2_N3PLSZ;

	if ( stv2_current_tilemap.enabled )
	{
		stv2_current_tilemap.enabled = stv_vdp2_check_vram_cycle_pattern_registers( STV_VDP2_CP_NBG3_PNMDR, STV_VDP2_CP_NBG3_CPDR, stv2_current_tilemap.bitmap_enable );
	}

	stv_vdp2_check_tilemap(bitmap, cliprect);
}


static void stv_vdp2_draw_RBG0(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   Colours           : 16, 256, 2048, 32768, 16770000
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : Possible
	   Bitmap Sizes      : 512 x 256, 512 x 512, 1024 x 256, 1024 x 512
	   Scale             : 0.25 x - 256 x
	   Rotation          : No
	   Linescroll        : Yes
	   Column Scroll     : Yes
	   Mosaic            : Yes
	*/
	stv2_current_tilemap.enabled = STV_VDP2_R0ON;

//	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...

	//stv2_current_tilemap.trans_enabled = STV_VDP2_R0TPON;
	if ( STV_VDP2_R0CCEN )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_ALPHA;
		alpha_set_level( ((UINT16)(0x1f-STV_VDP2_R0CCRT)*0xff)/0x1f);
	}
	else if ( STV_VDP2_R0TPON == 0 )
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_PEN;
	}
	else
	{
		stv2_current_tilemap.transparency = TRANSPARENCY_NONE;
	}
	stv2_current_tilemap.colour_depth = STV_VDP2_R0CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_R0CHSZ;
	stv2_current_tilemap.bitmap_enable = STV_VDP2_R0BMEN;
	stv2_current_tilemap.bitmap_size = STV_VDP2_R0BMSZ;
	stv2_current_tilemap.bitmap_palette_number = STV_VDP2_R0BMP;
	stv2_current_tilemap.bitmap_map = STV_VDP2_RAMP_;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_RAMPA | (STV_VDP2_RAMP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_RAMPB | (STV_VDP2_RAMP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_RAMPC | (STV_VDP2_RAMP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_RAMPD | (STV_VDP2_RAMP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_R0PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_R0CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_R0SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_R0SCC;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_R0SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_R0SPCN;

	stv_vdp2_fill_rotation_parameter_table(1);

//	stv2_current_tilemap.scrollx = STV_VDP2_SCXIR0;
//	stv2_current_tilemap.scrolly = STV_VDP2_SCYIR0;
//	stv2_current_tilemap.incx = STV_VDP2_ZMXR0;
//	stv2_current_tilemap.incy = STV_VDP2_ZMYR0;
	stv2_current_tilemap.scrollx = stv_current_rotation_parameter_table.mx >> 16;
	stv2_current_tilemap.scrolly = stv_current_rotation_parameter_table.my >> 16;
	stv2_current_tilemap.incx = 0x10000;
	stv2_current_tilemap.incy = 0x10000;

	stv2_current_tilemap.plane_size = STV_VDP2_RAPLSZ;
	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_R0CAOS;
	stv2_current_tilemap.fade_control = (STV_VDP2_R0COEN * 1) | (STV_VDP2_R0COSL * 2);
	stv2_current_tilemap.window_control = (STV_VDP2_R0LOG * 0x01) |
										  (STV_VDP2_R0W0E * 0x02) |
										  (STV_VDP2_R0W1E * 0x04) |
										  (STV_VDP2_R0SWE * 0x08) |
										  (STV_VDP2_R0W0A * 0x10) |
										  (STV_VDP2_R0W1A * 0x20) |
										  (STV_VDP2_R0SWA * 0x40);

	/*Use 0x80 as a normal/rotate switch*/
	stv2_current_tilemap.layer_name=0x80;

	if ( !stv_vdp2_render_rbg0 ) return;

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_back(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int xcnt,ycnt;
	data8_t* gfxdata = memory_region(REGION_GFX1);
	static UINT16 *destline;

	if(!(STV_VDP2_BDCLMD & 1))
		fillbitmap(bitmap, get_black_pen(), cliprect);
	else
	{
		#ifdef MAME_DEBUG
		//usrintf_showmessage("Back screen enabled %08x",STV_VDP2_BKTA);
		#endif
		gfxdata+=(STV_VDP2_BKTA);

		for (ycnt = 0; ycnt <1024;ycnt++)
		{
			destline = (UINT16 *)(bitmap->line[ycnt]);

			for (xcnt = 0; xcnt <1024;xcnt++)
			{
				int r,g,b;

				b = ((gfxdata[0] & 0x7c) >> 2);
				g = ((gfxdata[0] & 0x03) << 3) | ((gfxdata[1] & 0xe0) >> 5);
				r = ((gfxdata[1] & 0x1f));

				destline[xcnt] = b | g << 5 | r << 10;
			}
			if(STV_VDP2_BKCLMD)
				gfxdata+=2;
		}
	}
}


WRITE32_HANDLER ( stv_vdp2_vram_w )
{
	data8_t *stv_vdp2_vram_decode = memory_region(REGION_GFX1);

	COMBINE_DATA(&stv_vdp2_vram[offset]);

	data = stv_vdp2_vram[offset];
	/* put in gfx region for easy decoding */
	stv_vdp2_vram_decode[offset*4+0] = (data & 0xff000000) >> 24;
	stv_vdp2_vram_decode[offset*4+1] = (data & 0x00ff0000) >> 16;
	stv_vdp2_vram_decode[offset*4+2] = (data & 0x0000ff00) >> 8;
	stv_vdp2_vram_decode[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_vdp2_vram_dirty_8x8x4[offset/8] = 1;
	stv_vdp2_vram_dirty_8x8x8[offset/16] = 1;
}

READ32_HANDLER ( stv_vdp2_vram_r )
{
	return stv_vdp2_vram[offset];
}


WRITE32_HANDLER ( stv_vdp2_cram_w )
{
	int r,g,b;
	COMBINE_DATA(&stv_vdp2_cram[offset]);

//	usrintf_showmessage("%01x",STV_VDP2_CRMD);

	switch( STV_VDP2_CRMD )
	{
		/*Mode 2/3*/
		case 2:
		case 3:
		{
			b = ((stv_vdp2_cram[offset] & 0x00ff0000) >> 16);
			g = ((stv_vdp2_cram[offset] & 0x0000ff00) >> 8);
			r = ((stv_vdp2_cram[offset] & 0x000000ff) >> 0);
			palette_set_color(offset,r,g,b);
		}
		break;
		/*Mode 0*/
		case 0:
		{
			offset &= 0x3ff;

			b = ((stv_vdp2_cram[offset] & 0x00007c00) >> 10);
			g = ((stv_vdp2_cram[offset] & 0x000003e0) >> 5);
			r = ((stv_vdp2_cram[offset] & 0x0000001f) >> 0);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color((offset*2)+1,r,g,b);
			b = ((stv_vdp2_cram[offset] & 0x7c000000) >> 26);
			g = ((stv_vdp2_cram[offset] & 0x03e00000) >> 21);
			r = ((stv_vdp2_cram[offset] & 0x001f0000) >> 16);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color(offset*2,r,g,b);
		}
		break;
		/*Mode 1*/
		case 1:
		{
			offset &= 0x7ff;

			b = ((stv_vdp2_cram[offset] & 0x00007c00) >> 10);
			g = ((stv_vdp2_cram[offset] & 0x000003e0) >> 5);
			r = ((stv_vdp2_cram[offset] & 0x0000001f) >> 0);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color((offset*2)+1,r,g,b);
			b = ((stv_vdp2_cram[offset] & 0x7c000000) >> 26);
			g = ((stv_vdp2_cram[offset] & 0x03e00000) >> 21);
			r = ((stv_vdp2_cram[offset] & 0x001f0000) >> 16);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color(offset*2,r,g,b);
		}
		break;
	}
}

READ32_HANDLER ( stv_vdp2_cram_r )
{
	return stv_vdp2_cram[offset];
}

WRITE32_HANDLER ( stv_vdp2_regs_w )
{
	COMBINE_DATA(&stv_vdp2_regs[offset]);
}

extern int stv_vblank,stv_hblank;
READ32_HANDLER ( stv_vdp2_regs_r )
{
//	if (offset!=1) logerror ("VDP2: Read from Registers, Offset %04x\n",offset);

	switch(offset)
	{
		case 0x4/4:
		/*Screen Status Register*/
								   /*VBLANK              HBLANK            ODD         PAL    */
			stv_vdp2_regs[offset] = (stv_vblank<<19) | (stv_hblank<<18) | (1 << 17) | (0 << 16);
		break;
		case 0x8/4:
		/*H/V Counter Register*/
								     /*H-Counter                               V-Counter                                         */
			stv_vdp2_regs[offset] = (((Machine->visible_area.max_x - 1)<<16)&0x3ff0000)|(((Machine->visible_area.max_y - 1)<<0)&0x3ff);
			logerror("CPU #%d PC(%08x) = VDP2: H/V counter read : %08x\n",cpu_getactivecpu(),activecpu_get_pc(),stv_vdp2_regs[offset]);
		break;
	}
	return stv_vdp2_regs[offset];
}

int stv_vdp2_start ( void )
{
	stv_vdp2_regs = auto_malloc ( 0x040000 );
	stv_vdp2_vram = auto_malloc ( 0x100000 ); // actually we only need half of it since we don't emulate extra 4mbit ram cart.
	stv_vdp2_cram = auto_malloc ( 0x080000 );
	stv_vdp2_vram_dirty_8x8x4 = auto_malloc ( 0x100000 );
	stv_vdp2_vram_dirty_8x8x8 = auto_malloc ( 0x100000 );

	memset(stv_vdp2_regs, 0, 0x040000);
	memset(stv_vdp2_vram, 0, 0x100000);
	memset(stv_vdp2_cram, 0, 0x080000);

	stv_vdp2_render_rbg0 = 1;
	if ( !strcmp(Machine->gamedrv->name, "vfremix") ||
		 !strcmp(Machine->gamedrv->name, "vfkids"))
	{
		stv_vdp2_render_rbg0 = 0;
	}
//	Machine->gfx[0]->color_granularity=4;
//	Machine->gfx[1]->color_granularity=4;

	return 0;
}

/* maybe we should move this to vidhrdw/stv.c */
VIDEO_START( stv_vdp2 )
{
	stv_vdp2_start();
	stv_vdp1_start();
	debug.l_en = 0xff;
	debug.s_pri = 6;

	return 0;
}

static void stv_vdp2_dynamic_res_change()
{
	static UINT16 horz,vert;

	switch( STV_VDP2_VRES & 3 )
	{
		case 0: vert = 224; break;
		case 1: vert = 240; break;
		case 2: vert = 256; break;
		case 3:
			logerror("WARNING: V Res setting (3) not allowed!\n");
			vert = 256;
			break;
	}

	/*Double-density interlace mode,doubles the vertical res*/
	if((STV_VDP2_LSMD & 3) == 3) { vert*=2;  }

	switch( STV_VDP2_HRES & 7 )
	{
		case 0: horz = 320; break;
		case 1: horz = 352; break;
		case 2: horz = 640; break;
		case 3: horz = 704; break;
		/*Exclusive modes,they sets the Vertical Resolution without considering the
			VRES register.*/
		case 4: horz = 320; vert = 480; break;
		case 5: horz = 352; vert = 480; break;
		case 6: horz = 640; vert = 480; break;
		case 7: horz = 704; vert = 480; break;
	}

	set_visible_area(0*8, horz-1,0*8, vert-1);
}

/*This is for calculating the rgb brightness*/
/*TODO: Optimize this...*/
static void	stv_vdp2_fade_effects()
{
	/*
	Note:We have to use temporary storages because palette_get_color must use
	variables setted with unsigned int8
	*/
	INT16 t_r,t_g,t_b;
	UINT8 r,g,b;
	int i;
	//usrintf_showmessage("%04x %04x",STV_VDP2_CLOFEN,STV_VDP2_CLOFSL);
	for(i=0;i<2048;i++)
	{
		/*Fade A*/
		palette_get_color(i, &r, &g, &b);
		t_r = (STV_VDP2_COAR & 0x100) ? (r - (0x100 - (STV_VDP2_COAR & 0xff))) : ((STV_VDP2_COAR & 0xff) + r);
		t_g = (STV_VDP2_COAG & 0x100) ? (g - (0x100 - (STV_VDP2_COAG & 0xff))) : ((STV_VDP2_COAG & 0xff) + g);
		t_b = (STV_VDP2_COAB & 0x100) ? (b - (0x100 - (STV_VDP2_COAB & 0xff))) : ((STV_VDP2_COAB & 0xff) + b);
		if(t_r < 0) 	{ t_r = 0; }
		if(t_r > 0xff) 	{ t_r = 0xff; }
		if(t_g < 0) 	{ t_g = 0; }
		if(t_g > 0xff) 	{ t_g = 0xff; }
		if(t_b < 0) 	{ t_b = 0; }
		if(t_b > 0xff) 	{ t_b = 0xff; }
		r = t_r;
		g = t_g;
		b = t_b;
		palette_set_color(i+(2048*1),r,g,b);

		/*Fade B*/
		palette_get_color(i, &r, &g, &b);
		t_r = (STV_VDP2_COBR & 0x100) ? (r - (0xff - (STV_VDP2_COBR & 0xff))) : ((STV_VDP2_COBR & 0xff) + r);
		t_g = (STV_VDP2_COBG & 0x100) ? (g - (0xff - (STV_VDP2_COBG & 0xff))) : ((STV_VDP2_COBG & 0xff) + g);
		t_b = (STV_VDP2_COBB & 0x100) ? (b - (0xff - (STV_VDP2_COBB & 0xff))) : ((STV_VDP2_COBB & 0xff) + b);
		if(t_r < 0) 	{ t_r = 0; }
		if(t_r > 0xff) 	{ t_r = 0xff; }
		if(t_g < 0) 	{ t_g = 0; }
		if(t_g > 0xff) 	{ t_g = 0xff; }
		if(t_b < 0) 	{ t_b = 0; }
		if(t_b > 0xff) 	{ t_b = 0xff; }
		r = t_r;
		g = t_g;
		b = t_b;
		palette_set_color(i+(2048*2),r,g,b);
	}
	//usrintf_showmessage("%04x %04x %04x %04x %04x %04x",STV_VDP2_COAR,STV_VDP2_COAG,STV_VDP2_COAB,STV_VDP2_COBR,STV_VDP2_COBG,STV_VDP2_COBB);
}

/******************************************************************************************

ST-V VDP2 window effect function version 0.02

How it works: returns 0 if the requested pixel is drawnable,1 if it isn't.

Done:
-Basic support(w0 or w1),bitmaps only.

Not Done:
-Windows on cells.
-w0 & w1 at the same time.
-Window logic.
-Line window.
-Color Calculation.
-Rotation parameter Window.

Window Registers are hooked up like this ATM:
	x--- ---- UNUSED
	-x-- ---- Sprite Window Area
	--x- ---- Window 1 Area
	---x ---- Window 0 Area
				  (0 = Inside,1 = Outside)
	---- x--- Sprite Window Enable
	---- -x-- Window 1 Enable
	---- --x- Window 0 Enable
				  (0 = Disabled,1 = Enabled)
	---- ---x Window Logic
				  (0 = OR,1 = AND)
******************************************************************************************/
static int stv_vdp2_window_process(int x,int y)
{
	static UINT16 s_x,e_x,s_y,e_y;
	/*W0*/
 	switch(STV_VDP2_LSMD & 3)
	{
		case 0:
		case 1:
		case 2:
			s_y = ((STV_VDP2_W0SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W0EY & 0x3ff) >> 0);
			break;
		case 3:
			s_y = ((STV_VDP2_W0SY & 0x3fe) >> 1);
			e_y = ((STV_VDP2_W0EY & 0x3fe) >> 1);
			break;
	}
	switch(STV_VDP2_HRES & 6)
	{
		/*Normal*/
		case 0:
			s_x = ((STV_VDP2_W0SX & 0x3fe) >> 1);
			e_x = ((STV_VDP2_W0EX & 0x3fe) >> 1);
			break;
		/*Hi-Res*/
		case 2:
			s_x = ((STV_VDP2_W0SX & 0x3ff) >> 0);
			e_x = ((STV_VDP2_W0EX & 0x3ff) >> 0);
			break;
		/*Exclusive Normal*/
		case 4:
			s_x = ((STV_VDP2_W0SX & 0x1ff) >> 0);
			e_x = ((STV_VDP2_W0EX & 0x1ff) >> 0);
			s_y = ((STV_VDP2_W0SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W0EY & 0x3ff) >> 0);
			break;
		/*Exclusive Hi-Res*/
		case 6:
			s_x = ((STV_VDP2_W0SX & 0x1ff) << 1);
			e_x = ((STV_VDP2_W0EX & 0x1ff) << 1);
			s_y = ((STV_VDP2_W0SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W0EY & 0x3ff) >> 0);
			break;
	}

	if(stv2_current_tilemap.window_control & 2)
	{
		/*Outside Area*/
		if(stv2_current_tilemap.window_control & 0x10)
		{
			if(y < s_y || y > e_y)
				return 1;
			else
			{
				if(x < s_x || x > e_x)
					return 1;
				//else
				//	return 0;
			}
		}
		/*Inside Area*/
		else
		{
			if(y > s_y && y < e_y)
			{
				if(x > s_x && x < e_x)
					return 1;
			}
			//else
			//	return 0;
		}
	}
	/*W1*/
	switch(STV_VDP2_LSMD & 3)
	{
		case 0:
		case 1:
		case 2:
			s_y = ((STV_VDP2_W1SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W1EY & 0x3ff) >> 0);
			break;
		case 3:
			s_y = ((STV_VDP2_W1SY & 0x3fe) >> 1);
			e_y = ((STV_VDP2_W1EY & 0x3fe) >> 1);
			break;
	}
	switch(STV_VDP2_HRES & 6)
	{
		/*Normal*/
		case 0:
			s_x = ((STV_VDP2_W1SX & 0x3fe) >> 1);
			e_x = ((STV_VDP2_W1EX & 0x3fe) >> 1);
			break;
		/*Hi-Res*/
		case 2:
			s_x = ((STV_VDP2_W1SX & 0x3ff) >> 0);
			e_x = ((STV_VDP2_W1EX & 0x3ff) >> 0);
			break;
		/*Exclusive Normal*/
		case 4:
			s_x = ((STV_VDP2_W1SX & 0x1ff) >> 0);
			e_x = ((STV_VDP2_W1EX & 0x1ff) >> 0);
			s_y = ((STV_VDP2_W1SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W1EY & 0x3ff) >> 0);
			break;
		/*Exclusive Hi-Res*/
		case 6:
			s_x = ((STV_VDP2_W1SX & 0x1ff) << 1);
			e_x = ((STV_VDP2_W1EX & 0x1ff) << 1);
			s_y = ((STV_VDP2_W1SY & 0x3ff) >> 0);
			e_y = ((STV_VDP2_W1EY & 0x3ff) >> 0);
			break;
	}
	if(stv2_current_tilemap.window_control & 4)
	{
		/*Outside Area*/
		if(stv2_current_tilemap.window_control & 0x20)
		{
			if(y < s_y || y > e_y)
				return 1;
			else
			{
				if(x < s_x || x > e_x)
					return 1;
				//else
				//	return 0;
			}
		}
		/*Inside Area*/
		else
		{
			if(y > s_y && y < e_y)
			{
				if(x > s_x && x < e_x)
					return 1;
			}
			//else
			//	return 0;
		}
	}
	return 0;
//	return 1;
}

/* VDP1 Framebuffer handling */
extern UINT16	 *stv_framebuffer;
extern UINT16	 **stv_framebuffer_lines;
extern int		 stv_framebuffer_width;
extern int		 stv_framebuffer_height;
extern int		 stv_framebuffer_double_interlace;

void stv_vdp2_drawsprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT8 pri)
{
	int x,y,r,g,b;
	int i;
	UINT16 pix;
	UINT16 *framebuffer_line;
	UINT16 *bitmap_line, *bitmap_line2 = NULL;
	UINT8  interlace_framebuffer;
	static const UINT16 sprite_colormask_table[] = { 0x07ff, 0x07ff, 0x07ff, 0x07ff, 0x03ff, 0x07ff, 0x03ff, 0x01ff,
										0x007f, 0x003f, 0x003f, 0x003f, 0x0ff, 0x0ff, 0x0ff, 0x0ff };
	static const UINT16 priority_shift_table[] = { 14, 13, 14, 13, 13, 12, 12, 12, 7, 7, 6, 0, 7, 7, 6, 0 };
	static const UINT16 priority_mask_table[]  = {  3,  7,  1,  3,  3,  7,  7,  7, 1, 1, 3, 0, 1, 1, 3, 0 };
	static const UINT16 ccrr_shift_table[] =	 { 11, 11, 11, 11, 10, 11, 10,  9, 0, 6, 0, 6, 0, 6, 0, 6 };
	static const UINT16 ccrr_mask_table[] =	     {  7,  3,  7,  3,  7,  1,  3,  7, 0, 1, 0, 3, 0, 1, 0, 3 };
	UINT16 alpha_enabled;

	int sprite_type;
	int sprite_colormask;
	int color_offset_pal;
	UINT16 sprite_priority_shift, sprite_priority_mask, sprite_ccrr_shift, sprite_ccrr_mask;
	UINT8	priority;
	UINT8	ccr = 0;
	UINT8 sprite_priorities[8];
	UINT8 sprite_ccr[8];

	sprite_priorities[0] = STV_VDP2_S0PRIN;
	sprite_priorities[1] = STV_VDP2_S1PRIN;
	sprite_priorities[2] = STV_VDP2_S2PRIN;
	sprite_priorities[3] = STV_VDP2_S3PRIN;
	sprite_priorities[4] = STV_VDP2_S4PRIN;
	sprite_priorities[5] = STV_VDP2_S5PRIN;
	sprite_priorities[6] = STV_VDP2_S6PRIN;
	sprite_priorities[7] = STV_VDP2_S7PRIN;

	sprite_ccr[0] = STV_VDP2_S0CCRT;
	sprite_ccr[1] = STV_VDP2_S1CCRT;
	sprite_ccr[2] = STV_VDP2_S2CCRT;
	sprite_ccr[3] = STV_VDP2_S3CCRT;
	sprite_ccr[4] = STV_VDP2_S4CCRT;
	sprite_ccr[5] = STV_VDP2_S5CCRT;
	sprite_ccr[6] = STV_VDP2_S6CCRT;
	sprite_ccr[7] = STV_VDP2_S7CCRT;

	for ( i = 0; i < 8; i++ ) if ( sprite_priorities[i] == pri ) break;
	if ( i == 8 ) return;

	sprite_type = STV_VDP2_SPTYPE;
	sprite_colormask = sprite_colormask_table[sprite_type];
	sprite_priority_shift = priority_shift_table[sprite_type];
	sprite_priority_mask = priority_mask_table[sprite_type];
	sprite_ccrr_shift = ccrr_shift_table[sprite_type];
	sprite_ccrr_mask = ccrr_mask_table[sprite_type];

	/* color offset (RGB brightness) */
	color_offset_pal = 0;
	if ( STV_VDP2_SPCOEN )
	{
		if ( STV_VDP2_SPCOSL == 0 )
		{ color_offset_pal = 2048; }
		else
		{ color_offset_pal = 2048*2; }
	}

	/* color calculation (alpha blending)*/
	if ( STV_VDP2_SPCCEN )
	{
		alpha_enabled = 0;
		switch( STV_VDP2_SPCCCS )
		{
			case 0x0: if ( pri <= STV_VDP2_SPCCN ) alpha_enabled = 1; break;
			case 0x1: if ( pri == STV_VDP2_SPCCN ) alpha_enabled = 1; break;
			case 0x2: if ( pri >= STV_VDP2_SPCCN ) alpha_enabled = 1; break;
			case 0x3: /* MSBON */ break;
		}
	}
	else
	{
		alpha_enabled = 0;
	}

	/* framebuffer interlace */
	if ( (STV_VDP2_LSMD == 2 || STV_VDP2_LSMD == 3) && stv_framebuffer_double_interlace == 0 )
		interlace_framebuffer = 1;
	else
		interlace_framebuffer = 0;

	for ( y = cliprect->min_y; y <= cliprect->max_y; y++ )
	{
		framebuffer_line = stv_framebuffer_lines[y];
		if ( interlace_framebuffer == 0 )
		{
			bitmap_line = (UINT16*)bitmap->line[y];
		}
		else
		{
			bitmap_line = (UINT16*)bitmap->line[2*y];
			bitmap_line2 = (UINT16*)bitmap->line[2*y + 1];
		}

		for ( x = cliprect->min_x; x <= cliprect->max_x; x++ )
		{
			pix = framebuffer_line[x];
			if ( pix & 0x8000 )
			{
				if ( sprite_priorities[0] != pri ) continue;
				b = (pix & 0x7c00) >> 10;
				g = (pix & 0x03e0) >> 5;
				r = (pix & 0x1f);
				if ( color_offset_pal )
				{
					stv_vdp2_compute_color_offset_RGB555( &r, &g, &b, STV_VDP2_SPCOSL );
				}
				bitmap_line[x] = b | g << 5 | r << 10;
				if ( interlace_framebuffer == 1 ) bitmap_line2[x] = b | g << 5 | r << 10;
			}
			else
			{
				priority = sprite_priorities[(pix >> sprite_priority_shift) & sprite_priority_mask];
				if ( priority != pri ) continue;

				if ( alpha_enabled ) 						
					ccr = sprite_ccr[ (pix >> sprite_ccrr_shift) & sprite_ccrr_mask ];

				pix &= sprite_colormask;
				if ( pix == (sprite_colormask - 1) )
				{
					/*shadow - in reality, we should check from what layer pixel beneath comes...*/
					if ( STV_VDP2_SDCTL & 0x3f )
					{
						bitmap_line[x] = (bitmap_line[x] & ~0x421) >> 1;
					}
					/* note that when shadows are disabled, "shadow" palette entries are not drawn */
				} else if ( pix )
				{
					pix += (STV_VDP2_SPCAOS << 8);
					pix &= 0x7ff;
					pix += color_offset_pal;
					if ( alpha_enabled == 0 )
					{
						bitmap_line[x] = Machine->pens[ pix ];
						if ( interlace_framebuffer == 1 ) bitmap_line2[x] = Machine->pens[ pix ];
					}
					else
					{
						bitmap_line[x] = alpha_blend_r16( bitmap_line[x], Machine->pens[pix], ((UINT16)(0x1f-ccr)*0xff)/0x1f );
						if ( interlace_framebuffer == 1 ) bitmap_line2[x] = alpha_blend_r16( bitmap_line2[x], Machine->pens[pix], ((UINT16)(0x1f-ccr)*0xff)/0x1f );
					}
				}
			}
		}
	}
}

extern data32_t *stv_vdp1_vram;

VIDEO_UPDATE( stv_vdp2 )
{
	static UINT8 pri;

//#ifndef MAME_DEBUG
	stv_vdp2_dynamic_res_change();
//#endif

	video_update_vdp1(bitmap,cliprect);

	stv_vdp2_fade_effects();

	stv_vdp2_draw_back(bitmap,cliprect);

	/*If a plane has a priority value of zero it isn't shown at all.*/
	for(pri=1;pri<8;pri++)
	{
		if (!(code_pressed(KEYCODE_T))) {if(pri==STV_VDP2_N3PRIN) stv_vdp2_draw_NBG3(bitmap,cliprect);}
		if (!(code_pressed(KEYCODE_Y))) {if(pri==STV_VDP2_N2PRIN) stv_vdp2_draw_NBG2(bitmap,cliprect);}
		if (!(code_pressed(KEYCODE_U))) {if(pri==STV_VDP2_N1PRIN) stv_vdp2_draw_NBG1(bitmap,cliprect);}
		if (!(code_pressed(KEYCODE_I))) {if(pri==STV_VDP2_N0PRIN) stv_vdp2_draw_NBG0(bitmap,cliprect);}
		if (!(code_pressed(KEYCODE_K))) {if(pri==STV_VDP2_R0PRIN) stv_vdp2_draw_RBG0(bitmap,cliprect);}
		if (!(code_pressed(KEYCODE_O))) {stv_vdp2_drawsprites(bitmap,cliprect,pri);}
	}

#ifdef MAME_DEBUG
	if(STV_VDP2_VRAMSZ)
		usrintf_showmessage("Warning: VRAM Size = 8 MBit!");

	/*usrintf_showmessage("N0 %02x %04x %02x %04x N1 %02x %04x %02x %04x"
	,STV_VDP2_N0ZMXI,STV_VDP2_N0ZMXD
	,STV_VDP2_N0ZMYI,STV_VDP2_N0ZMYD
	,STV_VDP2_N1ZMXI,STV_VDP2_N1ZMXD
	,STV_VDP2_N1ZMYI,STV_VDP2_N1ZMYD);*/

	if ( code_pressed_memory(KEYCODE_W) )
	{
		int tilecode;

		for (tilecode = 0;tilecode<0x8000;tilecode++)
		{
			decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout);
		}

		for (tilecode = 0;tilecode<0x2000;tilecode++)
		{
			decodechar(Machine->gfx[1], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[1].gfxlayout);
		}

		for (tilecode = 0;tilecode<0x4000;tilecode++)
		{
			decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout);
		}

		for (tilecode = 0;tilecode<0x1000;tilecode++)
		{
			decodechar(Machine->gfx[3], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[3].gfxlayout);
		}

		/* vdp 1 ... doesn't have to be tile based */

		for (tilecode = 0;tilecode<0x8000;tilecode++)
		{
			decodechar(Machine->gfx[4], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[4].gfxlayout);
		}
		for (tilecode = 0;tilecode<0x2000;tilecode++)
		{
			decodechar(Machine->gfx[5], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[5].gfxlayout);
		}
		for (tilecode = 0;tilecode<0x4000;tilecode++)
		{
			decodechar(Machine->gfx[6], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[6].gfxlayout);
		}
		for (tilecode = 0;tilecode<0x1000;tilecode++)
		{
			decodechar(Machine->gfx[7], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[7].gfxlayout);
		}
	}

	if ( code_pressed_memory(KEYCODE_N) )
	{
		FILE *fp;

		fp=fopen("mamevdp1", "w+b");
		if (fp)
		{
			fwrite(stv_vdp1_vram, 0x80000, 1, fp);
			fclose(fp);
		}
	}

	if ( code_pressed_memory(KEYCODE_M) )
	{
		FILE *fp;

		fp=fopen("vdp1_vram.bin", "r+b");
		if (fp)
		{
			fread(stv_vdp1_vram, 0x80000, 1, fp);
			fclose(fp);
		}
	}

#endif



}

/* below is some old code we might use .. */

#if 0

static void stv_dump_ram()
{
	FILE *fp;

	fp=fopen("workraml.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_l, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("workramh.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_h, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("scu.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_scu, 0xd0, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("cram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_cram, 0x00080000, 1, fp);
		fclose(fp);
	}
	fp=fopen("68k.dmp", "w+b");
	if (fp)
	{
		fwrite(memory_region(REGION_CPU3), 0x100000, 1, fp);
		fclose(fp);
	}
}


#endif
