/***************************************************************************

						-= Jaleco Mega System 1 =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows scroll 0
		W		shows scroll 1
		E		shows scroll 2
		A		shows sprites with attribute 0-3
		S		shows sprites with attribute 4-7
		D		shows sprites with attribute 8-b
		F		shows sprites with attribute c-f

		Keys can be used togheter!


**********  There are 3 scrolling layers, 1 word per tile:

* Note: MS1-Z has 2 layers only.

  A page is 256x256, approximately the visible screen size. Each layer is
  made up of 8 pages (8x8 tiles) or 32 pages (16x16 tiles). The number of
  horizontal  pages and the tiles size  is selectable, using the  layer's
  control register. I think that when tiles are 16x16 a layer can be made
  of 16x2, 8x4, 4x8 or 2x16 pages (see below). When tile size is 8x8 we
  have two examples to guide the choice:

  the copyright screen of p47j (0x12) should be 4x2 (unless it's been hacked :)
  the ending sequence of 64th street (0x13) should be 2x4.

  I don't see a relation.


MS1-A MS1-B MS1-C
-----------------

					Scrolling layers:

90000 50000 e0000	Scroll 0
94000 54000 e8000	Scroll 1
98000 58000 f0000	Scroll 2					* Note: missing on MS1-Z

Tile format:	fedc------------	Palette
				----ba9876543210	Tile Number



84000 44000 c2208	Layers Enable				* Note: missing on MS1-Z?

	fedc ---- ---- ---- unused
	---- ba98 ---- ----	Priority Control
	---- ---- 7654 ----	unused
	---- ---- ---- 3---	Enable Sprites
	---- ---- ---- -210	Enable Layer 210



84200 44200 c2000	Scroll 0 Control
84208 44208 c2008	Scroll 1 Control
84008 44008 c2100	Scroll 2 Control		* Note: missing on MS1-Z

Offset:		00						Scroll X
			02						Scroll Y
			04 fedc ba98 765- ----	? (unused?)
			   ---- ---- ---4 ----	0<->16x16 Tiles	1<->8x8 Tiles
			   ---- ---- ---- 32--	? (used, by p47!)
			   ---- ---- ---- --10	N: Layer H pages = 16 / (2^N)



84300 44300 c2308	Screen Control

	fed- ---- ---- ---- 	? (unused?)
	---c ---- ---- ---- 	? (on, troughout peekaboo)
	---- ba9- ---- ---- 	? (unused?)
	---- ---8 ---- ---- 	Portrait F/F (?FullFill?)
	---- ---- 765- ---- 	? (unused?)
	---- ---- ---4 ---- 	Reset Sound CPU (1->0 Transition)
	---- ---- ---- 321- 	? (unused?)
	---- ---- ---- ---0		Flip Screen



**********  There are 256*4 colors (256*3 for MS1-Z):

Colors		MS1-A/C			MS1-Z

000-0ff		Scroll 0		Scroll 0
100-1ff		Scroll 1		Sprites
200-2ff		Scroll 2		Scroll 1
300-3ff		Sprites			-

88000 48000 f8000	Palette

	fedc--------3---	Red
	----ba98-----2--	Blue
	--------7654--1-	Green
	---------------0	? (used, not RGB! [not changed in fades])


**********  There are 256 sprites (128 for MS1-Z):

&RAM[8000]	Sprite Data	(16 bytes/entry. 128? entries)

Offset:		0-6						? (used, but as normal RAM, I think)
			08 	fed- ---- ---- ----	?
				---c ---- ---- ----	mosaic sol.	(?)
				---- ba98 ---- ----	mosaic		(?)
				---- ---- 7--- ----	y flip
				---- ---- -6-- ----	x flip
				---- ---- --45 ----	?
				---- ---- ---- 3210	color code (* bit 3 = priority *)
			0A						H position
			0C						V position
			0E	fedc ---- ---- ----	? (used by p47j, 0-8!)
				---- ba98 7654 3210	Number



Object RAM tells the hw how to use Sprite Data (missing on MS1-Z).
This makes it possible to group multiple small sprite, up to 256,
into one big virtual sprite (up to a whole screen):

8e000 4e000 d2000	Object RAM (8 bytes/entry. 256*4 entries)

Offset:		00	Index into Sprite Data RAM
			02	H	Displacement
			04	V	Displacement
			06	Number	Displacement

Only one of these four 256 entries is used to see if the sprite is to be
displayed, according to this latter's flipx/y state:

Object RAM entries:		Used by sprites with:

000-0ff					No Flip
100-1ff					Flip X
200-2ff					Flip Y
300-3ff					Flip X & Y




No? No? c2108	Sprite Bank

	fedc ba98 7654 321- ? (unused?)
	---- ---- ---- ---0 Sprite Bank



84100 44100 c2200 Sprite Control

			fedc ba9- ---- ---- ? (unused?)
			---- ---8 ---- ---- Enable Sprite Splitting In 2 Groups:
								Some Sprite Appear Over, Some Below The Layers
			---- ---- 765- ---- ? (unused?)
			---- ---- ---4 ----	Enable Effect (?)
			---- ---- ---- 3210	Effect Number (?)

I think bit 4 enables some sort of color cycling for sprites having priority
bit set. See code of p7j at 6488,  affecting the rotating sprites before the
Jaleco logo is shown: values 11-1f, then 0. I fear the Effect Number is an
offset to be applied over the pens used by those sprites. As for bit 8, it's
not used during game, but it is turned on when sprite/foreground priority is
tested, along with Effect Number being 1, so ...


**********  Priorities (ouch!)

[ Sprite / Sprite order ]

	[MS1-A,B,C]		From first in Object RAM (frontmost) to last.
	[MS1-Z]			From last in Sprite RAM (frontmost) to first.

[ Layer / Layer & Sprite / Layer order ]

Controlled by:

	* bits 7-4 (16 values) of the Layer Control register
	* bit 4 of the Sprite Control register

		Layer Control	Sprite Control
MS1-Z	-
MS1-A	84000			84100
MS1-B	44000			44100
MS1-C	c2208			c2200

When bit 4 of the Sprite Contol register is set, sprites with color
code 0-7 and sprites with color 8-f form two groups. Each group can
appear over or below some layers.

The 16 values in the Layer Control register determine the order of
the layers, and of the groups of sprites.

There is a PROM that translates the values in the register to the
actual code sent to the hardware.


***************************************************************************

							[ Priorities ]

***************************************************************************

p0 = scroll 0	p1 = scroll 1	p2 = scroll 2	s = all sprites
s0 = sprites with color 0-3		s1 = sprites with color 4-7
s2 = sprites with color 8-b		s3 = sprites with color c-f

order is : below -> over

***************************************************************************
						[ 64th Street (MS1-C) ]
***************************************************************************

Flags: p2 always 0011, p1 0001, others 0 (unless otherwise stated).

        enab bg   notes/not empty screens/what's the correct sprites-fg pos
-.intro 000f 0002 - no p1	bg 0123---- sx++, sy=0

0.lev 1 010f 0001	p0 s  p1	p0: 01234--- max sx 3f0	p1: 01234567 max sx 5e8
					p0 tiles:	(0-f)	(002-30f)
					p1 tiles:	0		(0-f)
					p2 tiles:	(1-5)	(000-155)


1.lev 2 010f 0001 p0 s  p1	bg 012----- max sx 1e2	fg 012----- max sx 1e2
2.train 030f 0000 p0 p1 s 	bg 01234567 01234567 sx++ sy = 0 fg 01------ 01------ 01------ 01------ max sx 100. sy 0/100
3.port  050f 0001 *c2200<-0100* bg: 0123---- 0123---- 0123---- 0123---- sx ? sy 0/1/2/300	fg: 012345-- 012345--sx ? sy 0/100
				  p0 s2/3 p1 s0/1	(player=s0, men(ground)=s0&1, men(water)=s2&3, splash=s3)
4.ship1	010f 0001 p0 s  p1
5.ship2 030f 0001 p0 p1 s
6.ship3	010f 0001 - no p1
7.rail  050f 0001 *c2200<-0100*
				  p0 s2/3 p1 s0/1	(player=s0, men=s0&1, box+pieces=s1, brokenbox=s3, fallen=s3)
8.tunne	030f 0001 p0 p1 s
9.facto 010f 0001 p0 s  p1
a.fact2 020f 0001 p1 p0 s p2	(p0 = room , p1 = wall (hidden?), p2 = text)
				  p0 = p1 = 0001 *p1 must go below p0, totally obscured*
				  p0 tiles: (0-8)(a70-cef)
				  p1 tiles: 2da8-2dac(wall)+	(<- only pens 0-7 used !)
				  			4dad(floor)+		(<- only pens 0-7 used !)
				  			3000(rest)			(<- only pen pen 15, RGB: 0)

b.fact3 020f 0001 p1 p0 s	*bg&fg reversed*
c.fact4 030f 0001 p0 p1 s
d.stree 030f 0001 ""
e.palac 030f 0001 ""
f.pal<- 030f 0001 ""
0.palac 030f 0001 ""
1.pal<- 030f 0001 ""
[Continues to 0x15, I'm bored!]


***************************************************************************
						[ Avenging Spirit (MS1-B) ]
***************************************************************************

Flags: txt always 0011, fg&bg 1, others 0 (unless otherwise stated).

				enab	notes/what's the correct sprites-fg pos
-.game logo		000f	p1 p0 p2 -	(p0 black tiles, p1 garbage(stars), p2 logo)
						p0 tiles: 1		(df3-dff) (pen 0 with holes of pen 15)
						p1 tiles: (0-2)	(dd5-ddd)
						p2 tiles: c02-c82 + 942-94a with colors: 3-f
						RGBx have x = 0

-.talking man	030f	p0 s p1 p2	(p0 room, p1 man/glass cage, p2 opaque frame with hole and / or text, s phantasm)
						p0 tiles: (2-6)(e97-ede)
						p1 tiles: (0-1)(ef3-f0e)
						p2 tiles: 0(void)+9488(opaque tile)+d48c(frame)+d680(text)
						RGBx have x = 0

0.city			000f	p1 p0 s p2	(p0 near city, p1 far city, p2 text)
						p0 tiles: (0-a)	(028-3cc)
						p1 tiles: 0		(019-0b8)
						p2 tiles: (0-9)	(030-102)

1.snakes		0c0f	*44100<-100*		(p0 front, p1 back, p2 text,
						s2 p1 s0/3 p0 p2	 player+shot:s3, fire:s2, snake+laser:s0)

2.factory		020f	p1 s  p0

3.lift boss		010f	p0 p1 s p2	(p0 wall, p1 lift, p2 text)
						p0 tiles: 3		(c50-c75)
						p1 tiles: 1/3-6/a/c	(c1d-c89)
						p2 tiles:


4.city+lifts	010f	p0 p1 s
5.snake boss	000f	p1 p0 s
6.sewers		030f	p0 s  p1
7.end of sewers	030f	p0 s  p1

8.boss			0e0f	*44100<-100*
						p1 s2/3 p0 s0 p2 (player:s3, boss+expl+fire:s2, splash:s0)

9.road			000f	p1 p0 s
a.factory		020f	p1 s  p0
b.boss(flying)	000f	-- p0 s
c.airport 		000f	p1 p0 s
  door			030f	p0 s  p1 (ghost=s1 above all?)
d.final level	000f	p1 p0 s



***************************************************************************
						[ Chimera Beast (MS1-C) ]
***************************************************************************

			flags	enab	notes/what's the correct sprites-fg pos
-.copyright	1/0/13	000f	p1 p0 p2	(p0 chimera, p1 wall, p2 text)
							p0 tiles: (0-1)	(c01-c98)
							p1 tiles: 2		(7c9-8c8)
							p2 tiles: f

-.logo		1/0/13	000f	p1 p0 p2	(p0 eyes, p1 opaque black, p2 name)
							p0 tiles: (1-5)	52e-54b & c1a-c7d
							p1 tiles: same as copyright screen
							p2 tiles: 5/f

-.instruct	3/1/13	010f

0.lev1		1/2/13	030f	p0 s? p1 s? p2	(p0 bubbles, p1 plants, p2 text panel)
							p0 tiles: 0		(001-05c)
									  0		(05d-0b8)
									  0		(0b9-114)
							p1 tiles: 2/3	(d00-fdc)
							p2 tiles: 0/1/f



***************************************************************************
							[ Cybattler (MS1-C) ]
***************************************************************************

Flags: txt always 0013, fg&bg 0, others 0 (unless otherwise stated).

				enab	notes/what's the correct sprites-fg pos
-.logo(scroll)	000f	p0 p1 p2 -	(p0 wall, p1 robot, p2 text)
						p0 tiles: 0		(1c0-1ff)
						p1 tiles: 1		(280-31d)
						p2 tiles: (b-c)	(2a0-59e)

  (sprites)		010f	p0 p1 s? p2 s?	(sprites color word: 8000)
  (cybattlr)	000f	p0 p1 p2

-.press start	0f0f	p0 p1 p2 -	(p0/1:3; p2:10)	(p0 wall, p1 robots, p2 text)
						p0 tiles: (b-c)	(1c0-1ff)
						p1 tiles: (9-8)	(605-7dd)
						p2 tiles: (0-3/5/8/f)

-.robot exits	040f	p1 p0 s p2	(p0/1:2; p2:10) (p0 robot, p1 stars, p2 text, s spaceship+arm)
						p0 tiles: 7(3f0-43d)
						p1 tiles: a(681-77f)
						p2 tiles: (c-f)

						sprites 0-7 (8 high bytes): 8002 0000 0000 0FD0

-.robot face	0f0f	p0 p1 p2 -	(p0/1:2; p2:10)
-.robot launch	0f0f	-  p1 s? p2 s?
						  sprites 0-7 (8 high bytes): 8004 0000 00A0 0FC8 semaphore
													  8001 0010 0080 0680 robot

-.high score	050f	p1 p0 p2 s	(p0/1:0; p2:10) (p0 robot, p1 wall, p2 text, s cursor+flame)
						p0 tiles: (4-6)(680-70d)
						p1 tiles: (1-7)(360-37f)
						p2 tiles: (1/3/5/7)(?-cc8?)


0.lev1			0f0f	p0 p1 s p2	(p0/1:0; p2:12) (p0 earth, p1 rocks, p2 text)
						p0 tiles:	(9-a)(001-12f)
						p1 tiles:	(8-a)(000-77f)
						p2 tiles:	(3/5)(ascii)	<- score
									(0)(ascii)		<- "insert coin"
									(6)(700-7c0)	<- energy bar
									(5)(ascii)		<- "charge"
									(2)(728-768)	<- charge bar


(p0 sx = $a00)	040f	p1 p0 s p2					(p0 earth, p1 stars, p2 text)
						p0 tiles:	(0-5)(010-1bb)
						p1 tiles:	(8-a)(004-77f)


1.lev2			0f0f	p0 p1 s p2	(p0:0; p1:2; p2:12)	(p0 land, p1 spaceship, p2 text)




***************************************************************************
							[ edf (MS1-B) ]
***************************************************************************

Flags: txt always 0010, fg 0000, others 0 (unless otherwise stated).
edf does not exploit low and hi sprite priorities at all, during gameplay.

			enab bg   notes/what's the correct sprites-fg pos

-.intro scr	020f b0 f0 t12	p0 s  p1 -notxt

-.edf logo	010f b0 f0 t12	p0 s? p2 s? p1	!!!	(p0 earth, p1 reflex, p2 edf)
							p0 tiles: fd04-fd6e
							p1 tiles: E~290 	(<- only pens 0-7!)
							p2 tiles: 7230-73b0

-.weapon0  	020f b0 f0 t12	p0 s  p1 p2
-.weapon1  	000f b0 f0 t12	p0 p1 s  p2
1.lev1		000f b0 f0 t12	p0 p1 s  p2
			020f b0 f0 t12	p0 s  p1 p2

4.sea		000f b0 f0 t12	p0 p1 s  p2
5.rtype		000f b0 f0 t12	p0 p1 s  p2
6.space		000f b0 f0 t12	p0 p1 s  p2




***************************************************************************
							[ Hachoo (MS1-A) ]
***************************************************************************

Flags: txt , fg&bg , others 0 (unless otherwise stated).

				enab	notes/what's the correct sprites-fg pos
2.lev 2			060f	p2 p1 	(p0 empty, p1 ground+portal, p2 back
								 player:s0, mountains($a)+bridge_mask($8):s2)






***************************************************************************
							[ P47j (MS1-A) ]
***************************************************************************

Flags: p2 always 0010, p1 0000, others 0 (unless otherwise stated).
P47j does not exploit low and hi sprite priorities at all, during gameplay.

				enab p0		notes/what's the correct sprites-fg pos
t.sprite test 1	000e 001c   		p2:0013
t.sprite test 2	000f 001c			p2:0013 p1=column	*84100:0101*
-.copyright scr	000f 0002			p2:0012				*84300:0010*
-.p47 logo 		010c dis. 			p2:0013	during: explosion + your plane goes by
				000c ""				""		after your plane is gone
									p2 tiles: 03ff(void)+(5-6)(200-29f)(subtitle-logo)

-.hi scores		010c dis.	p2 s !	p2:0012 *sprite should go above txt*
									sprite n.$11 (8 high bytes): 0002 0208 0074 0608
									p2 tiles: 005c(void)+(5-6)(120-1cf)(names-your name)

0.lev0			020f 0002	p0 s  p1
							p0 tiles: 33ff(void)+(0-5)(20-4c)
							p1 tiles: 0bff(void)+(0/2)050(ground)+(0-1)(900-980)(trees-cliffs)

1.clouds		000f 0002	p0 p1 s	(p0 = far clouds, p1 = near clouds, p2 = text)
							p0 tiles: f3ff(void)+(0-8)(080-2bf)
							p1 tiles: 0bff(void)+0910-094f


2.north africa	020f 0002 p0 s  p1
3.ship			020f 0000 p0 s  p1
4.ardennes		000f 0001 p0 p1 s
5.desert		020f 0001 p0 s  p1
6.desert+sea	020f 0002 p0 s  p1
7.sea+snow		020f 0000 p0 s  p1
[Repeats?]




***************************************************************************
							[ Phantasm (MS1-A) ]
***************************************************************************

Flags: txt always 0011, fg&bg 1, others 0 (unless otherwise stated).

				enab	notes/what's the correct sprites-fg pos
-.game logo		000f	p1 p0 p2	(p0 black tiles, p1 garbage, p2 logo)
						p0 tiles: 1		(df3-dff)
						p1 tiles: (0-2)	(dd5-ddd)
						p2 tiles: (6-e)	(94f-a7b)
						RGBx have x = 0

0.city 			000f	p1 p0 s p2	(p0 near city, p1 far city, p2 text)
						p0 tiles: (0-a)	(028-3cc)
						p1 tiles: 0		(013-0b8)
						p2 tiles: (0-9)	(030-102)


***************************************************************************/

#include "vidhrdw/generic.h"
#include "drivers/megasys1.h"

/* Variables only used here: */

/* For debug purposes: */
int debugsprites;

/* Variables defined here, that have to be shared: */
struct tilemap *megasys1_tmap_0, *megasys1_tmap_1, *megasys1_tmap_2;
unsigned char *megasys1_scrollram_0, *megasys1_scrollram_1, *megasys1_scrollram_2;
unsigned char *megasys1_objectram, *megasys1_vregs, *megasys1_ram;
int megasys1_scroll_flag[3], megasys1_scrollx[3], megasys1_scrolly[3], megasys1_pages_per_tmap_x[3], megasys1_pages_per_tmap_y[3];
int megasys1_active_layers, megasys1_sprite_bank;
int megasys1_screen_flag, megasys1_sprite_flag;
int megasys1_bits_per_color_code;

/* Variables defined in driver: */
extern int hardware_type;



int megasys1_vh_start(void)
{
	int i;

	spriteram = &megasys1_ram[0x8000];
	megasys1_tmap_0 = megasys1_tmap_1 = megasys1_tmap_2 = NULL;

	megasys1_active_layers = megasys1_sprite_bank = megasys1_screen_flag = megasys1_sprite_flag = 0;

 	for (i = 0; i < 3; i ++)
	{
		megasys1_scroll_flag[i] = megasys1_scrollx[i] = megasys1_scrolly[i] = 0;
		megasys1_pages_per_tmap_x[i] = megasys1_pages_per_tmap_y[i] = 0;
	}

 	megasys1_bits_per_color_code = 4;
 	return 0;
}

/***************************************************************************

							Palette routines

***************************************************************************/


/* MS1-A, B, C, Z */
void paletteram_RRRRGGGGBBBBRGBx_word_w(int offset, int data)
{
	/*	byte 0    byte 1	*/
	/*	RRRR GGGG BBBB RGB?	*/
	/*	4321 4321 4321 000?	*/

	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	int r = ((newword >> 8) & 0xF0 ) | ((newword << 0) & 0x08);
	int g = ((newword >> 4) & 0xF0 ) | ((newword << 1) & 0x08);
	int b = ((newword >> 0) & 0xF0 ) | ((newword << 2) & 0x08);

	palette_change_color( offset/2, r,g,b );

	WRITE_WORD (&paletteram[offset], newword);
}


/* MS1-D */
void paletteram_RRRRRGGGGGBBBBBx_word_w(int offset, int data)
{
	/*	byte 0    byte 1	*/
	/*	RRRR RGGG GGBB BBB?	*/
	/*	4321 0432 1043 210?	*/

	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	int r = ((newword >> 8) & 0xF8 );
	int g = ((newword >> 3) & 0xF8 );
	int b = ((newword << 2) & 0xF8 );

	palette_change_color( offset/2, r,g,b );

	WRITE_WORD (&paletteram[offset], newword);
}


/***************************************************************************

							Layers declarations:

					* Read and write handlers for the layer
					* Callbacks for the TileMap code

***************************************************************************/

#define TILES_PER_PAGE_X (0x20)
#define TILES_PER_PAGE_Y (0x20)
#define TILES_PER_PAGE (TILES_PER_PAGE_X * TILES_PER_PAGE_Y)

#define MEGASYS1_GET_TILE_INFO(_n_) \
void megasys1_get_scroll_##_n_##_tile_info_8x8( int col, int row ) \
{ \
	int tile_index = \
			(col * TILES_PER_PAGE_Y) + \
\
			(row / TILES_PER_PAGE_Y) * TILES_PER_PAGE * megasys1_pages_per_tmap_x[_n_] + \
			(row % TILES_PER_PAGE_Y); \
\
	int code = READ_WORD(&megasys1_scrollram_##_n_[tile_index * 2]); \
	SET_TILE_INFO( _n_ , code & 0xfff, code >> (16 - megasys1_bits_per_color_code) ); \
} \
\
void megasys1_get_scroll_##_n_##_tile_info_16x16( int col, int row ) \
{ \
	int tile_index = \
			((col / 2) * (TILES_PER_PAGE_Y / 2)) + \
\
			((row / 2) / (TILES_PER_PAGE_Y / 2)) * (TILES_PER_PAGE / 4) * megasys1_pages_per_tmap_x[_n_] + \
			((row / 2) % (TILES_PER_PAGE_Y / 2)); \
\
	int code = READ_WORD(&megasys1_scrollram_##_n_[tile_index * 2]); \
	SET_TILE_INFO( _n_ , (code & 0xfff) * 4 + (row & 1) + (col & 1) * 2 , code >> (16-megasys1_bits_per_color_code) ); \
}


#define MEGASYS1_SCROLLRAM_R(_n_) \
int megasys1_scrollram_##_n_##_r(int offset) {return READ_WORD(&megasys1_scrollram_##_n_[offset]);}

#define MEGASYS1_SCROLLRAM_W(_n_) \
void megasys1_scrollram_##_n_##_w(int offset,int data) \
{ \
int old_data, new_data; \
\
	old_data = READ_WORD(&megasys1_scrollram_##_n_[offset]); \
	new_data = COMBINE_WORD(old_data,data); \
	if (old_data != new_data) \
	{ \
		WRITE_WORD(&megasys1_scrollram_##_n_[offset], new_data); \
		if (megasys1_tmap_##_n_) \
		{ \
			int page, tile_index, row, col; \
			if (megasys1_scroll_flag[_n_] & 0x10)	/* tiles are 8x8 */ \
			{ \
				page		=	(offset/2) / TILES_PER_PAGE; \
				tile_index	=	(offset/2) % TILES_PER_PAGE; \
 \
				col	=	tile_index / TILES_PER_PAGE_Y + \
						( page % megasys1_pages_per_tmap_x[_n_] ) * TILES_PER_PAGE_X; \
 \
				row	=	tile_index % TILES_PER_PAGE_Y + \
						( page / megasys1_pages_per_tmap_x[_n_] ) * TILES_PER_PAGE_Y; \
 \
				tilemap_mark_tile_dirty(megasys1_tmap_##_n_, col, row); \
			} \
			else \
			{ \
				page		=	(offset/2) / (TILES_PER_PAGE / 4); \
				tile_index	=	(offset/2) % (TILES_PER_PAGE / 4); \
\
				/* col and row when tiles are 16x16 .. */ \
				col	=	tile_index / (TILES_PER_PAGE_Y / 2) + \
						( page % megasys1_pages_per_tmap_x[_n_] ) * (TILES_PER_PAGE_X / 2); \
 \
				row	=	tile_index % (TILES_PER_PAGE_Y / 2) + \
						( page / megasys1_pages_per_tmap_x[_n_] ) * (TILES_PER_PAGE_Y / 2); \
 \
				/* .. but we draw four 8x8 tiles, so col and row must be scaled */ \
				col *= 2;	row *= 2; \
				tilemap_mark_tile_dirty(megasys1_tmap_##_n_, col + 0, row + 0); \
				tilemap_mark_tile_dirty(megasys1_tmap_##_n_, col + 1, row + 0); \
				tilemap_mark_tile_dirty(megasys1_tmap_##_n_, col + 0, row + 1); \
				tilemap_mark_tile_dirty(megasys1_tmap_##_n_, col + 1, row + 1); \
			} \
		}\
	}\
}

MEGASYS1_GET_TILE_INFO(0)
MEGASYS1_GET_TILE_INFO(1)
MEGASYS1_GET_TILE_INFO(2)

MEGASYS1_SCROLLRAM_R(0)
MEGASYS1_SCROLLRAM_R(1)
MEGASYS1_SCROLLRAM_R(2)

MEGASYS1_SCROLLRAM_W(0)
MEGASYS1_SCROLLRAM_W(1)
MEGASYS1_SCROLLRAM_W(2)


/***************************************************************************

							Video registers access

***************************************************************************/

#define MEGASYS1_SCROLL_FLAG_W(_n_) \
void megasys1_scroll_##_n_##_flag_w(int data) \
{ \
	if ((megasys1_scroll_flag[_n_] != data) || (megasys1_tmap_##_n_ == 0) ) \
	{ \
		megasys1_scroll_flag[_n_] = data; \
\
		if (megasys1_tmap_##_n_) \
			tilemap_dispose(megasys1_tmap_##_n_); \
\
		/* number of pages when tiles are 16x16 */ \
		megasys1_pages_per_tmap_x[_n_] = 16 / ( 1 << (megasys1_scroll_flag[_n_] & 0x3) ); \
		megasys1_pages_per_tmap_y[_n_] = 32 / megasys1_pages_per_tmap_x[_n_]; \
\
		/* when tiles are 8x8, divide the number of total pages by 4 */ \
		if (megasys1_scroll_flag[_n_] & 0x10) \
		{ \
			if (megasys1_pages_per_tmap_y[_n_] > 4)	{ megasys1_pages_per_tmap_x[_n_] /= 1;	megasys1_pages_per_tmap_y[_n_] /= 4;} \
			else									{ megasys1_pages_per_tmap_x[_n_] /= 2;	megasys1_pages_per_tmap_y[_n_] /= 2;} \
		} \
\
		if ((megasys1_tmap_##_n_ = \
				tilemap_create \
					(	(megasys1_scroll_flag[_n_] & 0x10) ? \
							megasys1_get_scroll_##_n_##_tile_info_8x8 : \
							megasys1_get_scroll_##_n_##_tile_info_16x16, \
						TILEMAP_TRANSPARENT, \
						8,8, \
						TILES_PER_PAGE_X * megasys1_pages_per_tmap_x[_n_], \
						TILES_PER_PAGE_Y * megasys1_pages_per_tmap_y[_n_]  \
					) \
			)) megasys1_tmap_##_n_->transparent_pen = 15; \
	} \
}

MEGASYS1_SCROLL_FLAG_W(0)
MEGASYS1_SCROLL_FLAG_W(1)
MEGASYS1_SCROLL_FLAG_W(2)


/* Used by MS1-A/Z, B */
void megasys1_vregs_A_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&megasys1_vregs[offset]);
	COMBINE_WORD_MEM(&megasys1_vregs[offset],data);
	new_data  = READ_WORD(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x000   :	megasys1_active_layers = new_data;	break;

		case 0x008+0 :	MEGASYS1_VREG_SCROLL(2,x)	break;
		case 0x008+2 :	MEGASYS1_VREG_SCROLL(2,y)	break;
		case 0x008+4 :	MEGASYS1_VREG_FLAG(2)		break;

		case 0x200+0 :	MEGASYS1_VREG_SCROLL(0,x)	break;
		case 0x200+2 :	MEGASYS1_VREG_SCROLL(0,y)	break;
		case 0x200+4 :	MEGASYS1_VREG_FLAG(0)		break;

		case 0x208+0 :	MEGASYS1_VREG_SCROLL(1,x)	break;
		case 0x208+2 :	MEGASYS1_VREG_SCROLL(1,y)	break;
		case 0x208+4 :	MEGASYS1_VREG_FLAG(1)		break;

		case 0x100   :	megasys1_sprite_flag = new_data;		break;

		case 0x300   :	megasys1_screen_flag = new_data;
						if (new_data & 0x10)
							cpu_set_reset_line(1,ASSERT_LINE);
						else
							cpu_set_reset_line(1,CLEAR_LINE);
						break;

		case 0x308   :	soundlatch_w(0,new_data);	break;
						/* no interrupt ? */

		default		 :	SHOW_WRITE_ERROR("vreg %04X <- %04X",offset,data);
	}

}




/* Used by MS1-C only */
int megasys1_vregs_C_r(int offset)
{
	switch (offset)
	{
		case 0x8000:	return soundlatch2_r(0);
		default:		return READ_WORD(&megasys1_vregs[offset]);
	}
}

void megasys1_vregs_C_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&megasys1_vregs[offset]);
	COMBINE_WORD_MEM(&megasys1_vregs[offset],data);
	new_data  = READ_WORD(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x2000+0 :	MEGASYS1_VREG_SCROLL(0,x)	break;
		case 0x2000+2 :	MEGASYS1_VREG_SCROLL(0,y)	break;
		case 0x2000+4 :	MEGASYS1_VREG_FLAG(0)		break;

		case 0x2008+0 :	MEGASYS1_VREG_SCROLL(1,x)	break;
		case 0x2008+2 :	MEGASYS1_VREG_SCROLL(1,y)	break;
		case 0x2008+4 :	MEGASYS1_VREG_FLAG(1)		break;

		case 0x2100+0 :	MEGASYS1_VREG_SCROLL(2,x)	break;
		case 0x2100+2 :	MEGASYS1_VREG_SCROLL(2,y)	break;
		case 0x2100+4 :	MEGASYS1_VREG_FLAG(2)		break;

		case 0x2108   :	megasys1_sprite_bank   = new_data;	break;
		case 0x2200   :	megasys1_sprite_flag   = new_data;	break;
		case 0x2208   : megasys1_active_layers = new_data;	break;

		case 0x2308   :	megasys1_screen_flag = new_data;
						if (new_data & 0x10)
							cpu_set_reset_line(1,ASSERT_LINE);
						else
							cpu_set_reset_line(1,CLEAR_LINE);
						break;

		case 0x8000   :	/* Cybattler reads sound latch on irq 2 */
						soundlatch_w(0,new_data);
						cpu_cause_interrupt(1,2);
						break;

		default:		SHOW_WRITE_ERROR("vreg %04X <- %04X",offset,data);
	}
}



/* Used by MS1-D only */
void megasys1_vregs_D_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&megasys1_vregs[offset]);
	COMBINE_WORD_MEM(&megasys1_vregs[offset],data);
	new_data  = READ_WORD(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x2000+0 :	MEGASYS1_VREG_SCROLL(0,x)	break;
		case 0x2000+2 :	MEGASYS1_VREG_SCROLL(0,y)	break;
		case 0x2000+4 :	MEGASYS1_VREG_FLAG(0)		break;
		case 0x2008+0 :	MEGASYS1_VREG_SCROLL(1,x)	break;
		case 0x2008+2 :	MEGASYS1_VREG_SCROLL(1,y)	break;
		case 0x2008+4 :	MEGASYS1_VREG_FLAG(1)		break;
//		case 0x2100+0 :	MEGASYS1_VREG_SCROLL(2,x)	break;
//		case 0x2100+2 :	MEGASYS1_VREG_SCROLL(2,y)	break;
//		case 0x2100+4 :	MEGASYS1_VREG_FLAG(2)		break;

		case 0x2108   :	megasys1_sprite_bank	=	new_data;		break;
		case 0x2200   :	megasys1_sprite_flag	=	new_data;		break;
		case 0x2208   : megasys1_active_layers	=	new_data;		break;
		case 0x2308   :	megasys1_screen_flag	=	new_data;		break;

		default:		SHOW_WRITE_ERROR("vreg %04X <- %04X",offset,data);
	}
}



/***************************************************************************

							Sprites Drawing

***************************************************************************/


/*	 Draw sprites in the given bitmap. Priority may be:

 0	Draw sprites whose priority bit is 0
 1	Draw sprites whose priority bit is 1
-1	Draw sprites regardless of their priority

 Sprite Data:

	Offset		Data

 	00-07						?
	08 		fed- ---- ---- ----	?
			---c ---- ---- ----	mosaic sol.	(?)
			---- ba98 ---- ----	mosaic		(?)
			---- ---- 7--- ----	y flip
			---- ---- -6-- ----	x flip
			---- ---- --45 ----	?
			---- ---- ---- 3210	color code (bit 3 = priority)
	0A		X position
	0C		Y position
	0E		Code											*/

static void draw_sprites(struct osd_bitmap *bitmap, int priority)
{
int color,code,sx,sy,flipx,flipy,attr,sprite,offs,color_mask;

/* objram: 0x100*4 entries		spritedata: 0x80 entries */

	/* move the bit to the relevant position (and invert it) */
	if (priority != -1)	priority = (priority ^ 1) << 3;

	/* sprite order is from first in Sprite Data RAM (frontmost) to last */

	if (hardware_type != 'Z')	/* standard sprite hardware */
	{
		color_mask = (megasys1_sprite_flag & 0x100) ? 0x07 : 0x0f;

		for (offs = 0x000; offs < 0x800 ; offs += 8)
		{
			for (sprite = 0; sprite < 4 ; sprite ++)
			{
				unsigned char *objectdata = &megasys1_objectram[offs + 0x800 * sprite];
				unsigned char *spritedata = &spriteram[(READ_WORD(&objectdata[0x00])&0x7f)*0x10];

				attr = READ_WORD(&spritedata[0x08]);
				if ( (attr & 0x08) == priority ) continue;

				if (((attr & 0xc0)>>6) != sprite)	continue;

#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) != (debugsprites-1)) ) continue;
#endif

				/* apply the position displacements */
				sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&objectdata[0x02]) ) % 512;
				sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&objectdata[0x04]) ) % 512;

				if (sx > 256-1) sx -= 512;
				if (sy > 256-1) sy -= 512;

				flipx = attr & 0x40;
				flipy = attr & 0x80;

				if (megasys1_screen_flag & 1)
				{
					flipx = !flipx;		flipy = !flipy;
					sx = 240-sx;		sy = 240-sy;
				}

				/* sprite code is displaced as well */
				code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&objectdata[0x06]);
				color = (attr & color_mask);

				drawgfx(bitmap,Machine->gfx[3],
						(code & 0xfff ) + ((megasys1_sprite_bank & 1) << 12),
						color,
						flipx, flipy,
						sx, sy,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,15);

			}	/* sprite */
		}	/* offs */
	}	/* non Z hw */
	else
	{

		/* MS1-Z just draws Sprite Data, and in reverse order */

		for (sprite = 0; sprite < 0x80 ; sprite ++)
		{
			unsigned char *spritedata = &spriteram[sprite*0x10];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0x08) == priority ) continue;

#ifdef MAME_DEBUG
if ( (debugsprites) && (((attr & 0x0f)/4) == (debugsprites-1)) ) continue;
#endif

			sx = READ_WORD(&spritedata[0x0A]) % 512;
			sy = READ_WORD(&spritedata[0x0C]) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			code  = READ_WORD(&spritedata[0x0E]);
			color = (attr & 0x0F);

			flipx = attr & 0x40;
			flipy = attr & 0x80;

			if (megasys1_screen_flag & 1)
			{
				flipx = !flipx;		flipy = !flipy;
				sx = 240-sx;		sy = 240-sy;
			}

			drawgfx(bitmap,Machine->gfx[2],
				code,
				color,
				flipx, flipy,
				sx, sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,15);
		}	/* sprite */
	}	/* Z hw */

}







/* Mark colors used by visible sprites */

static void mark_sprite_colors(void)
{
int color_codes_start, color, penmask[16];
int offs, sx, sy, code, attr, i;
int color_mask;

	int xmin = Machine->drv->visible_area.min_x - (16 - 1);
	int xmax = Machine->drv->visible_area.max_x;
	int ymin = Machine->drv->visible_area.min_y - (16 - 1);
	int ymax = Machine->drv->visible_area.max_y;

	for (color = 0 ; color < 16 ; color++) penmask[color] = 0;

	color_mask = (megasys1_sprite_flag & 0x100) ? 0x07 : 0x0f;

	if (hardware_type != 'Z')		/* standard sprite hardware */
	{
		unsigned int *pen_usage	=	Machine->gfx[3]->pen_usage;
		int total_elements		=	Machine->gfx[3]->total_elements;
		color_codes_start		=	Machine->drv->gfxdecodeinfo[3].color_codes_start;

		for (offs = 0; offs < 0x2000 ; offs += 8)
		{
			int sprite = READ_WORD(&megasys1_objectram[offs+0x00]);
			unsigned char *spritedata = &spriteram[(sprite&0x7F)*16];

			attr = READ_WORD(&spritedata[0x08]);
			if ( (attr & 0xc0) != ((offs/0x800)<<6) ) continue;

			sx = ( READ_WORD(&spritedata[0x0A]) + READ_WORD(&megasys1_objectram[offs+0x02]) ) % 512;
			sy = ( READ_WORD(&spritedata[0x0C]) + READ_WORD(&megasys1_objectram[offs+0x04]) ) % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > xmax) ||(sy > ymax) ||(sx < xmin) ||(sy < ymin)) continue;

			code  = READ_WORD(&spritedata[0x0E]) + READ_WORD(&megasys1_objectram[offs+0x06]);
			code  =	(code & 0xfff );
			color = (attr & color_mask);

			penmask[color] |= pen_usage[code % total_elements];
		}
	}
	else
	{
		int sprite;
		unsigned int *pen_usage	=	Machine->gfx[2]->pen_usage;
		int total_elements		=	Machine->gfx[2]->total_elements;
		color_codes_start		=	Machine->drv->gfxdecodeinfo[2].color_codes_start;

		for (sprite = 0; sprite < 0x80 ; sprite++)
		{
			unsigned char* spritedata = &spriteram[sprite*16];

			sx		=	READ_WORD(&spritedata[0x0A]) % 512;
			sy		=	READ_WORD(&spritedata[0x0C]) % 512;
			code	=	READ_WORD(&spritedata[0x0E]);
			color	=	READ_WORD(&spritedata[0x08]) & 0x0F;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			if ((sx > xmax) ||(sy > ymax) ||(sx < xmin) ||(sy < ymin)) continue;

			penmask[color] |= pen_usage[code % total_elements];
		}
	}


	for (color = 0; color < 16; color++)
	 for (i = 0; i < 16; i++)
	  if (penmask[color] & (1 << i)) palette_used_colors[16 * color + i + color_codes_start] = PALETTE_COLOR_USED;
}




/***************************************************************************
			  Draw the game screen in the given osd_bitmap.
***************************************************************************/

struct priority
{
	struct GameDriver *driver;
	int priorities[16];
};

extern struct GameDriver avspirit_driver;
extern struct GameDriver bigstrik_driver;
extern struct GameDriver chimerab_driver;
extern struct GameDriver cybattlr_driver;
extern struct GameDriver hachoo_driver;
extern struct GameDriver edf_driver;
extern struct GameDriver p47_driver;
extern struct GameDriver peekaboo_driver;
extern struct GameDriver plusalph_driver;
extern struct GameDriver stdragon_driver;
extern struct GameDriver street64_driver;

/*
	0:	Scroll 0
	1:	Scroll 1
	2:	Scroll 2
	3:	Sprites with color 0-7
		(*every sprite*, if sprite splitting is not active)
	4:	Sprites with color 8-f

	0xffff: use the standard value 0x04132
*/

static struct priority priorities[] =
{
	{	&avspirit_driver,
		{ 0x14032,0x04132,0x13042,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0x14302,0xfffff,0x14032,0xfffff }
	},
	{	&bigstrik_driver,	/* like 64street */
		{ 0xfffff,0x03142,0xfffff,0x04132,0xfffff,0x04132,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&chimerab_driver,
		{ 0x14032,0x10324,0x14032,0x04132,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0x01324,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&cybattlr_driver,
		{ 0x04132,0xfffff,0xfffff,0xfffff,0x14032,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x04132 }
	},
	{	&hachoo_driver,
		{ 0x24130,0xfffff,0xfffff,0xfffff,0x04132,0xfffff,0x24130,0xfffff,
		  0x24103,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&p47_driver,	/* verified with PROM */
		{ 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&peekaboo_driver, /* verified with PROM */
		{ 0x0134f,0x034ff,0x0341f,0x3401f,0x1340f,0x3410f,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&plusalph_driver,	/* verified with PROM (same as p47) */
		{ 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&stdragon_driver,	/* verified with PROM (same as p47) */
		{ 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	&street64_driver,
		{ 0xfffff,0x03142,0xfffff,0x04132,0xfffff,0x04132,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	},
	{	0,	/* default: edf .. */
		{ 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,
		  0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff }
	}
};

void megasys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,flag,pri;
	int megasys1_active_layers1 = megasys1_active_layers;

	if (hardware_type == 'Z') megasys1_active_layers = 0x020b;

	tilemap_set_flip( ALL_TILEMAPS, (megasys1_screen_flag & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0 );

#ifdef MAME_DEBUG
debugsprites = 0;
if (keyboard_pressed(KEYCODE_Z))
{
int msk = 0;

	if (keyboard_pressed(KEYCODE_Q))	{ msk |= 0xfff1;}
	if (keyboard_pressed(KEYCODE_W))	{ msk |= 0xfff2;}
	if (keyboard_pressed(KEYCODE_E))	{ msk |= 0xfff4;}
	if (keyboard_pressed(KEYCODE_A))	{ msk |= 0xfff8; debugsprites = 1;}
	if (keyboard_pressed(KEYCODE_S))	{ msk |= 0xfff8; debugsprites = 2;}
	if (keyboard_pressed(KEYCODE_D))	{ msk |= 0xfff8; debugsprites = 3;}
	if (keyboard_pressed(KEYCODE_F))	{ msk |= 0xfff8; debugsprites = 4;}

	if (msk != 0) megasys1_active_layers &= msk;
}
#endif

	MEGASYS1_TMAP_SET_SCROLL(0)
	MEGASYS1_TMAP_SET_SCROLL(1)
	MEGASYS1_TMAP_SET_SCROLL(2)

	MEGASYS1_TMAP_UPDATE(0)
	MEGASYS1_TMAP_UPDATE(1)
	MEGASYS1_TMAP_UPDATE(2)

	palette_init_used_colors();

	mark_sprite_colors();

	if (palette_recalc())	tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	MEGASYS1_TMAP_RENDER(0)
	MEGASYS1_TMAP_RENDER(1)
	MEGASYS1_TMAP_RENDER(2)

	for (	i = 0;
			 priorities[i].driver &&
			(priorities[i].driver != Machine->gamedrv) &&
			(priorities[i].driver != Machine->gamedrv->clone_of);
			i++ );

	pri = priorities[i].priorities[(megasys1_active_layers & 0x0f00) >> 8];

#ifdef MAME_DEBUG
	if (pri == 0xfffff || keyboard_pressed(KEYCODE_Z))
	{
		char buf[40];
		sprintf(buf,"Pri: %04X - Flag: %04X", megasys1_active_layers1, megasys1_sprite_flag);
//			sprintf(buf,"%04X", input_port_1_r(0));
		usrintf_showmessage(buf);
	}
#endif

	if (pri == 0xfffff) pri = 0x04132;

	flag = TILEMAP_IGNORE_TRANSPARENCY;

	for (i = 0;i < 5;i++)
	{
		int layer = (pri & 0xf0000) >> 16;
		pri <<= 4;

		switch (layer)
		{
			case 0:	MEGASYS1_TMAP_DRAW(0)	break;
			case 1:	MEGASYS1_TMAP_DRAW(1)	break;
			case 2:	MEGASYS1_TMAP_DRAW(2)	break;
			case 3:
			case 4:
				if (flag != 0)
				{
					flag = 0;
					osd_clearbitmap(bitmap);	/* should use fillbitmap */
				}

				if (megasys1_active_layers & 0x08)
				{
					if (megasys1_sprite_flag & 0x100)	/* sprites are split */
						draw_sprites(bitmap, (layer-3) & 1 );
					else
						if (layer == 3)	draw_sprites(bitmap, -1 );
				}

				break;
		}

		if (flag != 0)
		{
			flag = 0;
			osd_clearbitmap(bitmap);	/* should use fillbitmap */
		}

	}

	megasys1_active_layers = megasys1_active_layers1;
}
