/***************************************************************************

						-= Jaleco Mega System 1 =-

				driver by	Luca Elia (eliavit@unina.it)


Game						Year	System	Dumped by		Supported ?
---------------------------------------------------------------------------
64th Street	(Japan)			1991	C		J-Rom	 		Yes
64th Street	(World)			1991	C		AraCORN 		Yes
Astyanax (World) /			1989	A		?				Yes (encrypted)
The Lord of King (Japan)	1989	A	 	J-Rom			Yes (encrypted)
Avenging Spirit	(World) /	1991	B		AraCORN 		Yes
Phantasm (Japan)			1990	A		J-Rom 			Yes (encrypted)
Chimera Beast				1993	C		J-Rom	 		Yes
Cybattler					1993	C		AraCORN			Yes
Earth Defense Force			1991	B		AraCORN			Yes
Hachoo						1989	A		AraCORN 		Yes (encrypted)
Iga Ninjyutsuden (Japan)	1989	A		J-Rom			Yes (encrypted)
Legend of Makai	(World) /	1988	Z		AraCORN			Yes
Makai Densetsu  (Japan)		?		?		-				-
P-47 (Japan) /				1988	A		J-rom			Yes
P-47 (World)				1988	A	 	?				Yes
Peek-a-Boo!					1993	C		Bart			Yes
Plus Alpha					1989	A		J-Rom 			Yes (encrypted)
RodLand	(Japan) /			1990	A		?				Yes
RodLand (World)				1990	A		AraCORN 		Yes (encrypted)
Saint Dragon				1989	A		J-Rom 			Yes (encrypted)
Soldam						?		?		-				-
---------------------------------------------------------------------------



Hardware	Main CPU	Sound CPU	Sound Chips
----------------------------------------------------------------
MS1 - Z		68000		Z80			YM2203c
MS1 - A		68000		68000		YM2151	OKI-M6295 x 2
MS1 - B		68000		68000		YM2151	OKI-M6295 x 2
MS1 - C		68000		68000		YM2151	OKI-M6295 x 2 YM3012
----------------------------------------------------------------


Main CPU		RW		MS1-A			MS1-B			MS1-C
-------------------------------------------------------------------
ROM				R	000000-03ffff	000000-03ffff	000000-07ffff
									080000-0bffff
Video Regs		 W	084000-0843ff	044000-0443ff	0c0000-0cffff
Palette			RW	088000-0887ff	048000-0487ff	0f8000-0f87ff
Object RAM		RW	08e000-08ffff	04e000-04ffff	0d2000-0d3fff
Scroll 0		RW	090000-093fff	050000-053fff	0e0000-0e3fff
Scroll 1		RW	094000-097fff	054000-057fff	0e8000-0ebfff
Scroll 2		RW	098000-09bfff	058000-05bfff	0f0000-0f3fff
Work RAM		RW	0f0000-0fffff*	060000-07ffff*	ff0000-ffffff*
Input Ports 	R	080000-080009	0e0000-0e0001**	0d8000-d80001**
-------------------------------------------------------------------
* Can vary 				** Through protection.


Sound CPU		RW		MS1-A			MS1-B			MS1-C
-----------------------------------------------------------------
ROM				R	000000-01ffff	000000-01ffff	000000-01ffff
Latch #1		R	040000-040001	<				060000-060001
Latch #2		 W	060000-060001	<				<
2151 reg		 W	080000-080001	<				<
2151 data		 W	080002-080003	<				<
2151 status		R 	080002-080003	<				<
6295 #1 data	 W 	0a0000-0a0003	<				<
6295 #1 status	R 	0a0000-0a0001	<				<
6295 #2 data	 W 	0c0000-0c0003	<				<
6295 #2 status	R 	0c0000-0c0001	<				<
RAM				RW	0f0000-0f3fff	0e0000-0effff?	<
-----------------------------------------------------------------


Sound interrupts:
	A:
		p47 & p47j				all rte (all equal)
		phantasm				all rte (4 is different, but rte)
		rodland	& rodlandj		all rte (4 is different, but rte)
	B:
		avspirit				all rte (4 is different, but rte)
		edf						all rte (4 is different, but rte)
	C:
		64street				all rte (4 is different, but rte)
		cybattlr
			1;3;5-7]400	busy loop
			2]40c	read & store sound command and echo to main cpu
			4]446	rte






---------------------------- << Priorities >> ---------------------

p0 = scroll 0	p1 = scroll 1	p2 = scroll 2	s = all sprites
s0 = sprites with color 0-3		s1 = sprites with color 4-7
s2 = sprites with color 8-b		s3 = sprites with color c-f

order is : below -> over


|  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
        p0    p1 l  p2 l  p1 h  p2 h

p0h p1h p0l s p1l s p2		0
p0h p1h p0l s p2l s p2h		1

p2 tiles: 005c(void)+(5-6)(120-1cf)(names-your name)


Priorities for:				64th Street (MS1-C)
Flags: p2 always 0011, p1 0001, others 0 (unless otherwise stated).

        enab bg   notes/not empty screens/what's the correct sprites-fg pos
-.intro 000f 0002 - no p1	bg 0123---- sx++, sy=0

0.lev 1 010f 0001	p0 s  p1	p0: 01234--- max sx 3f0	p1: 01234567 max sx 5e8
					p0 tiles:	(0-f)	(002-30f)
					p1 tiles:	0		(0-f)
					p2 tiles:	(1-5)	(000-155)


1.lev 2 010f 0001 p0 s  p1	bg 012----- max sx 1e2	fg 012----- max sx 1e2
2.train 030f 0000 p0 p1 s 	bg 01234567 01234567 sx++ sy = 0 fg 01------ 01------ 01------ 01------ max sx 100. sy 0/100
3.port  050f 0001 2200=0100 bg: 0123---- 0123---- 0123---- 0123---- sx ? sy 0/1/2/300	fg: 012345-- 012345--sx ? sy 0/100
				  p0 s2/3 p1 s0/1	(player=s0, men(ground)=s0&1, men(water)=s2&3, splash=s3)
4.ship1	010f 0001 p0 s  p1
5.ship2 030f 0001 p0 p1 s
6.ship3	010f 0001 - no p1
7.rail  050f 0001 2200=0100
				  p0 s2/3 p1 s0/1	(player=s0, men=s0&1, box+pieces=s1, brokenbox=s3, fallen=s3)
8.tunne	030f 0001 p0 p1 s
9.facto 010f 0001 p0 s  p1
a.fact2 020f 0001 p1 p0 s p2	(p0 = room , p1 = wall (hidden?), p2 = text)
				  p0 = p1 = 0001 *p1 must go below p0, totally obscured*
				  p0 tiles: (0-8)(a70-cef)
				  p1 tiles: 2da8-2dac(wall)+	(<- only pens 0-7 used !)
				  			4dad(floor)+		(<- only pens 0-7 used !)
				  			3000(rest)			(<- only pen pen 15, RGB: 0)

p0 p1-l p2-l p1-h p2-h
     |    |____|____|_____split by 0x0100
     |         |
     |_________|__________split by 0x0100

b.fact3 020f 0001 p1 p0 s	*bg&fg reversed*
c.fact4 030f 0001 p0 p1 s
d.stree 030f 0001 ""
e.palac 030f 0001 ""
f.pal<- 030f 0001 ""
0.palac 030f 0001 ""
1.pal<- 030f 0001 ""
[Continues to 0x15, I'm bored!]


Priorities for:					Avenging Spirit (MS1-B)
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

1.snakes		0c0f	44100=100
						s2 p1 s0/3 p0	(player 3, fire 2, snake+laser 0)
2.factory		020f	p1 s  p0
3.lift boss		010f	p0 p1 s p2	(p0 wall, p1 lift, p2 text)
						p0 tiles: 3		(c50-c75)
						p1 tiles: 1/3-6/a/c	(c1d-c89)
						p2 tiles:


4.city+lifts	010f	p0 p1 s
5.snake boss	000f	p1 p0 s
6.sewers		030f	p0 s  p1
7.end of sewers	030f	p0 s  p1
8.boss			0e0f	44100=100
						p1 s2/3 p0 s0  (player 3, boss+expl+fire 2, splash 0)
9.road			000f	p1 p0 s
a.factory		020f	p1 s  p0
b.boss(flying)	000f	-- p0 s
c.airport 		000f	p1 p0 s
  door			030f	p0 s  p1 (ghost=s1 above all?)
d.final level	000f	p1 p0 s



Priorities for:					Chimera Beast (MS1-C)

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



Priorities for:					Cybattler (MS1-C)
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




Priorities for:					edf (MS1-B)
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




Priorities for:					P47j (MS1-A)
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




Priorities for:					Phantasm (MS1-A)
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


------------------------------------------------------------------------

								To Do
								-----

- Support more encrypted games
- Understand how priorities *really* work
- Understand an handful of unknown bits in video regs
- Flip Screen support

***************************************************************************/

#include "driver.h"
//#include "vidhrdw/generic.h"

/* Variables only used here: */
//	unsigned char *inputram;
static int ip_select;
static int ip_select_values[5];

/* Variables that vidhrdw has access to: */
int hardware_type;

/* Variables defined in vidhrdw: */
extern unsigned char *megasys1_scrollram_0, *megasys1_scrollram_1, *megasys1_scrollram_2;
extern unsigned char *megasys1_objectram, *megasys1_vregs, *megasys1_ram;

/* Functions defined in vidhrdw: */
int  megasys1_vh_start(void);
void megasys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void megasys1_vregs_A_w(int offset, int data);
int  megasys1_vregs_C_r(int offset);
void megasys1_vregs_C_w(int offset, int data);
void paletteram_RRRRGGGGBBBBRGBx_word_w(int offset, int data);
int  megasys1_scrollram_0_r(int offset);
int  megasys1_scrollram_1_r(int offset);
int  megasys1_scrollram_2_r(int offset);
void megasys1_scrollram_0_w(int offset, int data);
void megasys1_scrollram_1_w(int offset, int data);
void megasys1_scrollram_2_w(int offset, int data);



void megasys1_init_machine(void)
{
	ip_select = 0;	/* reset protection */
}



/*
 This macro is used to decrypt the code roms:
 the first parameter is the encrypted word, the other parameters specify
 the bits layout to build the word in clear from the encrypted one
*/
#define BITSWAP(_x,_f,_e,_d,_c,_b,_a,_9,_8,_7,_6,_5,_4,_3,_2,_1,_0)\
		(((_x & (1 << _0))?(1<<0x0):0) + \
		 ((_x & (1 << _1))?(1<<0x1):0) + \
		 ((_x & (1 << _2))?(1<<0x2):0) + \
		 ((_x & (1 << _3))?(1<<0x3):0) + \
		 ((_x & (1 << _4))?(1<<0x4):0) + \
		 ((_x & (1 << _5))?(1<<0x5):0) + \
		 ((_x & (1 << _6))?(1<<0x6):0) + \
		 ((_x & (1 << _7))?(1<<0x7):0) + \
		 ((_x & (1 << _8))?(1<<0x8):0) + \
		 ((_x & (1 << _9))?(1<<0x9):0) + \
		 ((_x & (1 << _a))?(1<<0xa):0) + \
		 ((_x & (1 << _b))?(1<<0xb):0) + \
		 ((_x & (1 << _c))?(1<<0xc):0) + \
		 ((_x & (1 << _d))?(1<<0xd):0) + \
		 ((_x & (1 << _e))?(1<<0xe):0) + \
		 ((_x & (1 << _f))?(1<<0xf):0))



/*
**
**	Main cpu data
**
**
*/


/***************************************************************************
							[ System A ]
***************************************************************************/

static int coins_r(int offset)   {return input_port_0_r(0);}	// < 00 | Coins >
static int player1_r(int offset) {return input_port_1_r(0);}	// < 00 | Player 1 >
static int player2_r(int offset) {return (input_port_2_r(0)<<8)+input_port_3_r(0);}		// < Reserve | Player 2 >
static int dsw1_r(int offset)	 {return input_port_4_r(0);}							//   DSW 1
static int dsw2_r(int offset)	 {return input_port_5_r(0);}							//   DSW 2
static int dsw_r(int offset)	 {return (input_port_4_r(0)<<8)+input_port_5_r(0);}		// < DSW 1 | DSW 2 >


#define INTERRUPT_NUM_A		3
int interrupt_A(void)
{
	switch ( cpu_getiloops() % 3 )
	{
		case 0:		return 3;
		case 1:		return 2;
		case 2:		return 1;
		default:	return 0;
	}
}

//	#define INTERRUPT_NUM_A		1
//	int interrupt_A(void)		{ return 2; }
//	#define INTERRUPT_NUM_A		50
//	static int interrupt_A(void)
//{
//	if (cpu_getiloops()==0)	return 2;
//	else					return 1;
//}


#define MEMORYMAP_A(_shortname_,_ram_start_,_ram_end_) \
static struct MemoryReadAddress _shortname_##_readmem[] = \
{ \
	{ 0x000000, 0x05ffff, MRA_ROM }, 		/* P47 rom is 0x40000 bytes */ \
	{ _ram_start_, _ram_end_, MRA_BANK1,0,0,"ram" }, \
	{ 0x080000, 0x080001, coins_r }, \
	{ 0x080002, 0x080003, player1_r }, \
	{ 0x080004, 0x080005, player2_r }, \
	{ 0x080006, 0x080007, dsw_r }, \
	{ 0x080008, 0x080009, soundlatch2_r },	/* Echo from sound cpu */ \
	{ 0x084000, 0x084fff, MRA_BANK2,0,0,"vregs" }, \
	{ 0x088000, 0x0887ff, paletteram_word_r,0,0,"palette ram" }, \
	{ 0x08e000, 0x08ffff, MRA_BANK3,0,0,"object ram" }, \
	{ 0x090000, 0x093fff, megasys1_scrollram_0_r,0,0,"scroll ram 0" }, \
	{ 0x094000, 0x097fff, megasys1_scrollram_1_r,0,0,"scroll ram 1" }, \
	{ 0x098000, 0x09bfff, megasys1_scrollram_2_r,0,0,"scroll ram 2" }, \
	{ -1 } \
}; \
static struct MemoryWriteAddress _shortname_##_writemem[] = \
{ \
 	{ 0x000000, 0x05ffff, MWA_ROM }, \
	{ _ram_start_, _ram_end_, MWA_BANK1, &megasys1_ram,0,"ram" }, \
	{ 0x084000, 0x0843ff, megasys1_vregs_A_w, &megasys1_vregs,0,"vregs" }, \
	{ 0x088000, 0x0887ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram,0,"palette ram" }, \
	{ 0x08e000, 0x08ffff, MWA_BANK3, &megasys1_objectram, 0, "object ram" }, \
	{ 0x090000, 0x093fff, megasys1_scrollram_0_w, &megasys1_scrollram_0,0,"scroll ram 0"}, \
	{ 0x094000, 0x097fff, megasys1_scrollram_1_w, &megasys1_scrollram_1,0,"scroll ram 1"}, \
	{ 0x098000, 0x09bfff, megasys1_scrollram_2_w, &megasys1_scrollram_2,0,"scroll ram 2"}, \
	{ -1 } \
};





/***************************************************************************
							[ System C ]
***************************************************************************/


#define INTERRUPT_NUM_C		30
int interrupt_C(void)
{
#if 1
	if (cpu_getiloops()==0)	return 4; /* Once */
	else
	{
		if (cpu_getiloops()%2)	return 1;
		else 					return 2;
	}
#else
	if (cpu_getiloops()%2)	return 2;
	else 					return 4;
#endif
}




/*			 Read the input ports, through a protection device:

 ip_select_values must contains the 5 codes sent to the protection device
 in order to obtain the status of the following 5 input ports:

		 Coins	Player1		Player2		DSW-1		DSW-2

 in that order.			*/

static int ip_select_r(int offset)
{
int i;

//	Coins	P1		P2		DSW1	DSW2
//	57		53		54		55		56		< 64street
//	37		35		36		33		34		< avspirit
// 	20		21		22		23		24		< edf
//	56		52		53		54		55		< cybattlr

	/* f(x) = ((x*x)>>4)&0xFF ; f(f($D)) == 6 */
	if ((ip_select & 0xF0) == 0xF0) return 0x0D;

	for (i = 0; i < 5; i++)	if (ip_select == ip_select_values[i]) break;

	switch (i)
	{
			case 0x0 :	return coins_r(0);		break;
			case 0x1 :	return player1_r(0);	break;
			case 0x2 :	return player2_r(0);	break;
			case 0x3 :	return dsw1_r(0);		break;
			case 0x4 :	return dsw2_r(0);		break;
			default	 :	return 0x06;
	}
}

static void ip_select_w(int offset,int data)
{
	ip_select = COMBINE_WORD(ip_select,data);

	cpu_cause_interrupt(0,3);	/* EDF needs it */
}

#define MEMORYMAP_C(_shortname_,_ram_start_,_ram_end_) \
static struct MemoryReadAddress _shortname_##_readmem[] = \
{\
	{ 0x000000, 0x07ffff, MRA_ROM }, \
	{ _ram_start_, _ram_end_, MRA_BANK1,0,0,"ram" }, \
	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_r,0,0,"vregs" }, \
	{ 0x0d2000, 0x0d3fff, MRA_BANK3,0,0,"object ram" }, \
	{ 0x0e0000, 0x0e3fff, megasys1_scrollram_0_r,0,0,"scroll ram 0" }, \
	{ 0x0e8000, 0x0ebfff, megasys1_scrollram_1_r,0,0,"scroll ram 1" }, \
	{ 0x0f0000, 0x0f3fff, megasys1_scrollram_2_r,0,0,"scroll ram 2" }, \
	{ 0x0f8000, 0x0f87ff, paletteram_word_r,0,0,"palette ram" }, \
	{ 0x0d8000, 0x0d8001, ip_select_r }, \
	{ -1 } \
}; \
static struct MemoryWriteAddress _shortname_##_writemem[] = \
{ \
 	{ 0x000000, 0x07ffff, MWA_ROM }, \
	{ _ram_start_, _ram_end_, MWA_BANK1, &megasys1_ram, 0 , "ram" }, \
	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_w, &megasys1_vregs,0,"vregs" }, \
	{ 0x0d2000, 0x0d3fff, MWA_BANK3, &megasys1_objectram,0, "object ram" }, \
	{ 0x0e0000, 0x0e3fff, megasys1_scrollram_0_w, &megasys1_scrollram_0, 0, "scroll ram 0" }, \
	{ 0x0e8000, 0x0ebfff, megasys1_scrollram_1_w, &megasys1_scrollram_1, 0, "scroll ram 1" }, \
	{ 0x0f0000, 0x0f3fff, megasys1_scrollram_2_w, &megasys1_scrollram_2, 0, "scroll ram 2" }, \
	{ 0x0f8000, 0x0f87ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram,0,"palette ram" }, \
	{ 0x0d8000, 0x0d8001, ip_select_w }, \
	{ -1 } \
};






/***************************************************************************
							[ System B ]
***************************************************************************/


#define INTERRUPT_NUM_B	INTERRUPT_NUM_C
#define interrupt_B		interrupt_C


#define MEMORYMAP_B(_shortname_,_ram_start_,_ram_end_) \
static struct MemoryReadAddress _shortname_##_readmem[] =\
{\
	{ 0x000000, 0x03ffff, MRA_ROM },\
	{ 0x080000, 0x0bffff, MRA_ROM },\
	{ _ram_start_, _ram_end_, MRA_BANK1,0,0,"ram" }, \
	{ 0x044000, 0x044fff, MRA_BANK2,0,0,"vregs" },\
	{ 0x048000, 0x0487ff, paletteram_word_r,0,0,"palette ram" },\
	{ 0x04e000, 0x04ffff, MRA_BANK3,0,0,"object ram" },\
	{ 0x050000, 0x053fff, megasys1_scrollram_0_r,0,0,"scroll ram 0" },\
	{ 0x054000, 0x057fff, megasys1_scrollram_1_r,0,0,"scroll ram 1" },\
	{ 0x058000, 0x05bfff, megasys1_scrollram_2_r,0,0,"scroll ram 2" },\
	{ 0x0e0000, 0x0e0001, ip_select_r },\
	{ -1 }\
};\
static struct MemoryWriteAddress _shortname_##_writemem[] =\
{\
 	{ 0x000000, 0x03ffff, MWA_ROM },\
	{ 0x080000, 0x0bffff, MWA_ROM },\
	{ _ram_start_, _ram_end_, MWA_BANK1, &megasys1_ram, 0 , "ram" }, \
	{ 0x044000, 0x0443ff, megasys1_vregs_A_w, &megasys1_vregs,0,"vregs" },\
	{ 0x048000, 0x0487ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram,0,"palette ram" },\
	{ 0x04e000, 0x04ffff, MWA_BANK3, &megasys1_objectram,0, "object ram" },\
	{ 0x050000, 0x053fff, megasys1_scrollram_0_w , &megasys1_scrollram_0,0,"scroll ram 0" },\
	{ 0x054000, 0x057fff, megasys1_scrollram_1_w , &megasys1_scrollram_1,0,"scroll ram 1" },\
	{ 0x058000, 0x05bfff, megasys1_scrollram_2_w , &megasys1_scrollram_2,0,"scroll ram 2" },\
	{ 0x0e0000, 0x0e0001, ip_select_w },\
	{ -1 }\
};\






/*
**
**	Sound cpu data
**
**
*/

/*

 Interrupts for the sound cpu are the same for every system. Note that
 irq 2 are caused by the main cpu writing the sound latch.

 Music tempo is usually driven by the 2151 timers (status polled), but
 may be interrupt driven or even clock driven.

*/

#define SOUND_INTERRUPT_NUM 1
static int sound_interrupt(void)
{
	return 4;
}



/***************************************************************************
								[ System A ]
***************************************************************************/


static struct MemoryReadAddress sound_readmem_A[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x040000, 0x040001, soundlatch_r },
	{ 0x080002, 0x080003, YM2151_status_port_0_r },
	{ 0x0a0000, 0x0a0001, MRA_NOP /*OKIM6295_status_0_r*/ }, /* temporary - to fix sound */
	{ 0x0c0000, 0x0c0001, MRA_NOP /*OKIM6295_status_1_r*/ }, /* temporary - to fix sound */
	{ 0x0e0000, 0x0fffff, MRA_BANK8 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem_A[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x060000, 0x060001, soundlatch2_w },				// to main cpu
	{ 0x080000, 0x080001, YM2151_register_port_0_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_w},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_w },
	{ 0x0e0000, 0x0fffff, MWA_BANK8 },
	{ -1 }
};







/***************************************************************************
						[ System B / C ]
***************************************************************************/


static struct MemoryReadAddress sound_readmem_C[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x040000, 0x040001, soundlatch_r },	/* from main cpu */
	{ 0x060000, 0x060001, soundlatch_r },	/* from main cpu */
	{ 0x080002, 0x080003, YM2151_status_port_0_r },
	{ 0x0a0000, 0x0a0001, MRA_NOP /*OKIM6295_status_0_r*/ }, /* temporary - to fix sound */
	{ 0x0c0000, 0x0c0001, MRA_NOP /*OKIM6295_status_1_r*/ }, /* temporary - to fix sound */
	{ 0x0e0000, 0x0effff, MRA_BANK8 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem_C[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x040000, 0x040001, soundlatch2_w },	/* to main cpu */
	{ 0x060000, 0x060001, soundlatch2_w },	/* to main cpu */
	{ 0x080000, 0x080001, YM2151_register_port_0_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_w},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_w },
	{ 0x0e0000, 0x0effff, MWA_BANK8 },
	{ -1 }
};

#define sound_writemem_B	sound_writemem_C
#define sound_readmem_B		sound_readmem_C
#define sound_interrupt_B	sound_interrupt_C






/***************************************************************************
						[ System Z (Z80) ]
***************************************************************************/


#if 0
static struct MemoryReadAddress sound_readmem_z80[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem_z80[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
//	{ 0xf000, 0xf000, MWA_NOP }, /* ?? */
	{ -1 }
};


static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ -1 }
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ -1 }
};
#endif



/***************************************************************************

							Input Ports Macros

***************************************************************************/


/* IN0 - COINS */
#define COINS \
	PORT_START\
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_COIN3 )\
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2 )

/* IN1/3 - PLAYER 1/2 */
#define JOY(_flag_) \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | _flag_ )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | _flag_ )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | _flag_ )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | _flag_ )

/* IN2 - RESERVE */
#define RESERVE \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reserve 1P */\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reserve 2P */\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

/* IN4 - DSW1 MS1-A */
#define COINAGE_A \
	PORT_START\
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )\
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\

#define COINAGE_A_2 \
	PORT_START \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )\
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )\
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )


/* IN4 - DSW1 MS1-C */
#define COINAGE_C \
	PORT_START\
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )\
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )\
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )



/*
**
** 				Gfx data
**
*/

#define layout8x8(_name_,_romsize_)\
static struct GfxLayout _name_ =\
{\
	8,8,\
	(_romsize_)*8/(8*8*4),\
	4,\
	{0, 1, 2, 3},\
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4},\
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32},\
	8*8*4\
};\

#define layout16x16(_name_,_romsize_)\
static struct GfxLayout _name_ =\
{\
	16,16,\
	(_romsize_)*8/(16*16*4),\
	4,\
	{0, 1, 2, 3},\
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,\
	 0*4+32*16,1*4+32*16,2*4+32*16,3*4+32*16,4*4+32*16,5*4+32*16,6*4+32*16,7*4+32*16},\
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,\
	 8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},\
	16*16*4\
};\

layout8x8(  charlayout,	 0x20000*1)	/* P-47 has half that chars */
layout8x8(  tilelayout,	 0x20000*4)
layout16x16(spritelayout_A,0x20000*4)
layout16x16(spritelayout_C,0x20000*8)

#if 0
layout8x8(  tilelayout1, 0x10000*1)
layout16x16(spritelayout_Z,0x20000*1)
static struct GfxDecodeInfo gfxdecodeinfo_Z[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] scroll 0
	{ 1, 0x020000, &tilelayout1,	256*2, 16 },	// [1] scroll 1
	{ 1, 0x030000, &spritelayout_Z,	256*1, 16 },	// [2] sprites
	{ -1 }
};
#endif

static struct GfxDecodeInfo gfxdecodeinfo_A[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] scroll 0
	{ 1, 0x080000, &tilelayout,		256*1, 16 },	// [1] scroll 1
	{ 1, 0x100000, &charlayout,		256*2, 16 },	// [2] scroll 2
	{ 1, 0x120000, &spritelayout_A,	256*3, 16 },	// [3] sprites
	{ -1 }
};

#define gfxdecodeinfo_B		gfxdecodeinfo_A

static struct GfxDecodeInfo gfxdecodeinfo_C[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] scroll 0
	{ 1, 0x080000, &tilelayout,		256*1, 16 },	// [1] scroll 1
	{ 1, 0x100000, &charlayout,		256*2, 16 },	// [2] scroll 2
	{ 1, 0x120000, &spritelayout_C,	256*3, 16 },	// [3] sprites
	{ -1 }
};



#if 0
static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,
	1200000,	/* ?? */
	{ YM2203_VOL(25,25) },
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ 0	},
	{ 0 },
	{ irq_handler }
};

static struct MachineDriver machine_driver_Z = \
{\
	{\
		{\
			CPU_M68000,\
			6000000,	/* ?? */ \
			0,\
			readmem_A,writemem_A,0,0,\
			interrupt_A,interrupt_num_A \
		},\
		{\
			CPU_Z80 | CPU_AUDIO_CPU,\
			3000000,	/* ?? */ \
			2,\
			sound_readmem_z80,sound_writemem_z80,sound_readport,sound_writeport,\
			ignore_interrupt,1	/* irq generated by YM2203 */	\
		},\
	},\
	60,DEFAULT_60HZ_VBLANK_DURATION,\
	1,\
	0,\
	/* video hardware */ \
	256, 256,{ 0, 256-1, 0+16, 256-16-1 },\
	gfxdecodeinfo_Z,\
	256*3, 256*3,\
	0,\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\
	0,\
	megasys1_vh_start,\
	0,\
	megasys1_vh_screenrefresh,\
	/* sound hardware */ \
	0,0,0,0,\
	{\
		{\
			SOUND_YM2203,\
			&ym2203_interface\
		},\
	}\
};
#endif

void driver_init_Z(void) {hardware_type = 'Z'; }
void driver_init_A(void) {hardware_type = 'A'; }

void driver_init_B(void)
{
unsigned char *RAM;
int i;

	hardware_type = 'B' ;

	/* The ROM area is split in two parts: 000000-03ffff & 080000-0bffff */
	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	for (i = 0 ; i < 0x40000 ; i++)
	{
		RAM[i+0x80000] = RAM[i+0x40000];
		RAM[i+0x40000] = 0;
	}
}

void driver_init_C(void) {hardware_type = 'C'; }



#define MEGASYS1_CREDITS "Luca Elia\n"

/*

 This is a general porpouse macro to define a game and its hardware.

 The dirname can differ from the shortname:
 eg. shortname is street64 but dirname is 64street (<- can't be used as
 shortname since it starts with a digit).

 There is a different macro for clones too (defined below), but if the
 parent and clone game run on different hardwares, you can use this macro
 for the clone (to describe its hardware) and specify the parent's driver
 as a parameter

 Most games use the MEGASYS_GAME macro though (defined below) which is
 like this macro but with shortname equal to dirname and no parent
 (average situation)

*/
#define MEGASYS1_GAME_EXT(_shortname_,_dirname_,_parent_driver_,_fullname_,_year_, \
						  _orientation_, \
						  _type_,_ram_start_, _ram_end_, \
						  _main_clock_,_sound_clock_, \
						  _fm_clock_,_oki1_clock_,_oki2_clock_, \
						  _rom_decode_, _flags_) \
 \
MEMORYMAP_##_type_(_shortname_,_ram_start_,_ram_end_) \
 \
static struct YM2151interface _shortname_##_ym2151_interface = \
{ \
	1, \
	_fm_clock_, \
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) }, \
	{ 0 } \
}; \
 \
static struct OKIM6295interface _shortname_##_okim6295_interface = \
{ \
	2, \
	{_oki1_clock_, _oki2_clock_},\
	{3,4}, \
	{ 50, 50 } \
}; \
 \
static struct MachineDriver _shortname_##_machine_driver_ = \
{ \
	{ \
		{ \
			CPU_M68000, \
			_main_clock_, \
			0, \
			_shortname_##_readmem,_shortname_##_writemem,0,0, \
			interrupt_##_type_,INTERRUPT_NUM_##_type_ \
		}, \
		{ \
			CPU_M68000 | CPU_AUDIO_CPU, \
			_sound_clock_, \
			2, \
			sound_readmem_##_type_,sound_writemem_##_type_,0,0, \
			sound_interrupt,SOUND_INTERRUPT_NUM, \
		}, \
	}, \
	60,DEFAULT_60HZ_VBLANK_DURATION, \
	1, \
	megasys1_init_machine, \
	/* video hardware */ \
	256, 256,{ 0, 256-1, 0+16, 256-16-1 }, \
	gfxdecodeinfo_##_type_, \
	1024, 1024, \
	0, \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE, \
	0, \
	megasys1_vh_start, \
	0, \
	megasys1_vh_screenrefresh, \
	/* sound hardware */ \
	0,0,0,0, \
	{ \
		{ \
			SOUND_YM2151, \
			&_shortname_##_ym2151_interface \
		},\
		{\
			SOUND_OKIM6295, \
			&_shortname_##_okim6295_interface \
		} \
	} \
}; \
 \
struct GameDriver _shortname_##_driver = \
{ \
	__FILE__, \
	_parent_driver_, \
	#_dirname_, \
	#_fullname_, \
	#_year_, \
	"Jaleco", \
	MEGASYS1_CREDITS, \
	_flags_, \
	&_shortname_##_machine_driver_, \
	&driver_init_##_type_, \
	_shortname_##_rom, \
	_rom_decode_, 0, \
	0, \
	0, \
	input_ports_##_shortname_, \
	0, 0, 0, \
	_orientation_, \
	0,0 \
};




/*
   Clone game definition:
   this is for clones that run on the same hardware (average situation) so
   input ports and machine driver will point to those of the parent game
*/

#define MEGASYS1_GAME_CLONE(_shortname_,_parentname_,_fullname_,_year_, \
							_orientation_, _type_,  \
							_rom_decode_, _flags_ ) \
struct GameDriver _shortname_##_driver = \
{ \
	__FILE__, \
	&_parentname_##_driver, \
	#_shortname_, \
	#_fullname_, \
	#_year_, \
	"Jaleco", \
	MEGASYS1_CREDITS, \
	_flags_, \
	&_parentname_##_machine_driver_, \
	&driver_init_##_type_, \
	_shortname_##_rom, \
	_rom_decode_, 0, \
	0, \
	0, \
	input_ports_##_parentname_, \
	0, 0, 0, \
	_orientation_, \
	0,0 \
};



/*
   Standard game definition:
   shortname is equal to dirname, no parent
*/

#define MEGASYS1_GAME(_shortname_,_fullname_,_year_, \
					  _orientation_, \
					  _type_,_ram_start_, _ram_end_, \
					  _main_clock_,_sound_clock_, \
					  _fm_clock_,_oki1_clock_,_oki2_clock_, \
					  _rom_decode_, _flags_) \
MEGASYS1_GAME_EXT(_shortname_,_shortname_,0,_fullname_,_year_, \
				  _orientation_, \
				  _type_,_ram_start_, _ram_end_, \
				  _main_clock_,_sound_clock_, \
				  _fm_clock_,_oki1_clock_,_oki2_clock_, \
				  _rom_decode_, _flags_)







/* Provided by Jim Hernandez */
#define STD_FM_CLOCK	3500000
#define STD_OKI_CLOCK	  30000


/***************************************************************************

  Game driver(s)

***************************************************************************/


/***************************************************************************

							[ 64th Street ]

(World version)
interrupts:	1] 10eac:	disabled while b6c4=6 (10fb6 test)
						if (8b1c)	8b1c<-0
							color cycle
							copies 800 bytes 98da->8008

			2] 10f28:	switch b6c4
						0	RTE
						2	10f44:	M[b6c2]<-d8000; b6c4<-4
						4	10f6c:	next b6c2 & d8000.	if (b6c2>A)	b6c2,4<-0
														else		b6c4  <-2
						6	10f82: b6c6<-(d8001) b6c7<-FF (test)

			4] 10ed0:	disabled while b6c4=6 (10fb6 test)
						watchdog 8b1e
						many routines...
						b6c2<-0

13ca	print a string: a7->screen disp.l(base=f0004),src.l
13ea	print a string: a1->(chars)*
1253c	hw test (table of tests at 125c6)		*TRAP#D*
125f8	mem test (table of mem tests at 126d4)
1278e	input test (table of tests at 12808)
128a8	sound test	12a08	crt test
12aca	dsw test (b68e.w = dswa.b|dswb.b)

ff8b1e.w	incremented by int4, when >= b TRAP#E (software watchdog error)
ff9df8.w	*** level ***

***************************************************************************/

ROM_START( street64_rom )

	ROM_REGION(0x80000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "64th_03.rom", 0x000000, 0x040000, 0xed6c6942 )
	ROM_LOAD_ODD(  "64th_02.rom", 0x000000, 0x040000, 0x0621ed1d )

	ROM_REGION(0x220000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "64th_01.rom", 0x000000, 0x080000, 0x06222f90 )
	ROM_LOAD( "64th_06.rom", 0x080000, 0x080000, 0x2bfcdc75 )
	ROM_LOAD( "64th_09.rom", 0x100000, 0x020000, 0xa4a97db4 ) /* Text */
	ROM_LOAD( "64th_05.rom", 0x120000, 0x080000, 0xa89a7020 ) /* similar */
	ROM_LOAD( "64th_04.rom", 0x1a0000, 0x080000, 0x98f83ef6 ) /* train/boat boss*/

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "64th_08.rom", 0x000000, 0x010000, 0x632be0c1 )
	ROM_LOAD_ODD(  "64th_07.rom", 0x000000, 0x010000, 0x13595d01 )

	ROM_REGION(0x20000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "64th_11.rom", 0x000000, 0x020000, 0xb0b8a65c )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "64th_10.rom", 0x000000, 0x040000, 0xa3390561 )

ROM_END


ROM_START( streej64_rom )

	ROM_REGION(0x80000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "91105-3.bin", 0x000000, 0x040000, 0xa211a83b )
	ROM_LOAD_ODD(  "91105-2.bin", 0x000000, 0x040000, 0x27c1f436 )

	ROM_REGION(0x220000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "64th_01.rom", 0x000000, 0x080000, 0x06222f90 )
	ROM_LOAD( "64th_06.rom", 0x080000, 0x080000, 0x2bfcdc75 )
	ROM_LOAD( "64th_09.rom", 0x100000, 0x020000, 0xa4a97db4 ) /* Text */
	ROM_LOAD( "64th_05.rom", 0x120000, 0x080000, 0xa89a7020 ) /* similar */
	ROM_LOAD( "64th_04.rom", 0x1a0000, 0x080000, 0x98f83ef6 ) /* train/boat boss*/

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "64th_08.rom", 0x000000, 0x010000, 0x632be0c1 )
	ROM_LOAD_ODD(  "64th_07.rom", 0x000000, 0x010000, 0x13595d01 )

	ROM_REGION(0x20000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "64th_11.rom", 0x000000, 0x020000, 0xb0b8a65c )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "64th_10.rom", 0x000000, 0x040000, 0xa3390561 )

ROM_END

INPUT_PORTS_START( input_ports_street64 )
	COINS
	JOY(IPF_PLAYER1)
	RESERVE				// Unused
	JOY(IPF_PLAYER2)
	COINAGE_C
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

INPUT_PORTS_END

void street64_init(void)
{
//	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
//	WRITE_WORD (&RAM[0x006b8],0x6004);		// d8001 test
//	WRITE_WORD (&RAM[0x10EDE],0x6012);		// watchdog

	ip_select_values[0] = 0x57;
	ip_select_values[1] = 0x53;
	ip_select_values[2] = 0x54;
	ip_select_values[3] = 0x55;
	ip_select_values[4] = 0x56;
}

/* OSC:	? (Main 12, Sound 10 MHz according to KLOV) */
MEGASYS1_GAME_EXT(	street64, 64street, 0, 64th. Street - A Detective Story (World),1991, ORIENTATION_DEFAULT,
					C,0xff0000,0xffffff,
					12000000,10000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
					street64_init, 0 )

MEGASYS1_GAME_CLONE( streej64, street64, 64th. Street - A Detective Story (Japan),1991,ORIENTATION_DEFAULT, C,street64_init, 0 )




/***************************************************************************

								[ Astyanax ]

interrupts:	1] 1aa	2] 1b4

***************************************************************************/


#define ASTYANAX_ROM_LOAD \
	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */ \
	ROM_LOAD( "astyan11.bin", 0x000000, 0x020000, 0x5593fec9 ) /* scroll 0 */ \
	ROM_LOAD( "astyan12.bin", 0x020000, 0x020000, 0xe8b313ec ) \
	ROM_LOAD( "astyan13.bin", 0x040000, 0x020000, 0x5f3496c6 ) \
	ROM_LOAD( "astyan14.bin", 0x060000, 0x020000, 0x29a09ec2 ) \
	ROM_LOAD( "astyan15.bin", 0x080000, 0x020000, 0x0d316615 ) /* scroll 1 */ \
	ROM_LOAD( "astyan16.bin", 0x0a0000, 0x020000, 0xba96e8d9 ) \
	ROM_LOAD( "astyan17.bin", 0x0c0000, 0x020000, 0xbe60ba06 ) \
	ROM_LOAD( "astyan18.bin", 0x0e0000, 0x020000, 0x3668da3d ) \
	ROM_LOAD( "astyan19.bin", 0x100000, 0x020000, 0x98158623 ) /* scroll 2 */ \
	ROM_LOAD( "astyan20.bin", 0x120000, 0x020000, 0xc1ad9aa0 ) /* sprites  */ \
	ROM_LOAD( "astyan21.bin", 0x140000, 0x020000, 0x0bf498ee ) \
	ROM_LOAD( "astyan22.bin", 0x160000, 0x020000, 0x5f04d9b1 ) \
	ROM_LOAD( "astyan23.bin", 0x180000, 0x020000, 0x7bd4d1e7 ) \
\
	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */ \
	ROM_LOAD_EVEN( "astyan5.bin",  0x000000, 0x010000, 0x11c74045 ) \
	ROM_LOAD_ODD(  "astyan6.bin",  0x000000, 0x010000, 0xeecd4b16 ) \
\
	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */ \
	ROM_LOAD( "astyan9.bin",  0x000000, 0x020000, 0xa10b3f17 ) \
	ROM_LOAD( "astyan10.bin", 0x000000, 0x020000, 0x4f704e7a ) \
\
	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */ \
	ROM_LOAD( "astyan7.bin",  0x000000, 0x020000, 0x319418cc ) \
	ROM_LOAD( "astyan8.bin",  0x020000, 0x020000, 0x5e5d2a22 )



ROM_START( astyanax_rom )
	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "astyan2.bin", 0x00000, 0x20000, 0x1b598dcc )
	ROM_LOAD_ODD(  "astyan1.bin", 0x00000, 0x20000, 0x1a1ad3cf )
	ROM_LOAD_EVEN( "astyan3.bin", 0x40000, 0x10000, 0x097b53a6 )
	ROM_LOAD_ODD(  "astyan4.bin", 0x40000, 0x10000, 0x1e1cbdb2 )
	ASTYANAX_ROM_LOAD
ROM_END

ROM_START( lordofk_rom )
	ROM_REGION(0x80000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "lokj02.bin", 0x00000, 0x20000, 0x0d7f9b4a )
	ROM_LOAD_ODD(  "lokj01.bin", 0x00000, 0x20000, 0xbed3cb93 )
	ROM_LOAD_EVEN( "lokj03.bin", 0x40000, 0x20000, 0xd8702c91 )
	ROM_LOAD_ODD(  "lokj04.bin", 0x40000, 0x20000, 0xeccbf8c9 )
	ASTYANAX_ROM_LOAD
ROM_END


INPUT_PORTS_START( input_ports_astyanax )
	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80007.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )	// 1_2 shown in test mode
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_1C ) )	// 1_3
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )	// 1_4
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )	// 1_5
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )	// 1_2 shown in test mode
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_1C ) )	// 1_3
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )	// 1_4
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )	// 1_5
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	// according to manual
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 0x80006.b */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) // according to manual
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) // according to manual
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "30K 70K 110K then every 30K" )
	PORT_DIPSETTING(    0x00, "50K 100K then every 40K" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x40, 0x40, "Swap 1P/2P Controls" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/*
**********1
ast:	4934 5103 4A2A-41BD 4101
64:		4255 5320 4552 524f 5200
		B U  S    E R  R O  R .

ast:	47BC 4026 4A28 4A3A 5103 4A2A 41BD 4101 47BC 44A6
64:		4e4e 4144 4452 4553 5320 4552 524f 5200
		N N  A D  D R  E S  S    E R  R O  R .

fedc ba98 7654 3210     <- Encrypted Word
def0 a981 65cb 7234		<- Clear Word


**********2
????
ast:	              -6406 2452 3080 !0E9D
64:		5052 4f43 2e20-4154 4143 4820 !3a20 4e4f
   		P R  O C  .    A T  A C  H    :    N O
????

ast:
0036E0: 5F34 47BC 472A 5500-24E1 24F9 0E8C 6622 _4G.G*U.$.$...f"
                                 ? ?  ? D  U P
                                 	       5550
6622	5550
3e46	7165
3e17	716e
3e06	7164?
3e07	716c?
3e47	716d?
2464	4445

0036F0: 46B0 4132 4235 18FC-1801 0000 9101 0004 F.A2B5..........
ast_1:
0036E0: 4E75 4E4F 4F42 4A20-98C8 98CB 0C1E CD40 NuNOOBJ .......@
0036F0: 4C49 4341 5445 00FF-1030 0000 3220 0004 LICATE...0..2 ..

fedc ba98 7654 3210     <- Encrypted Word
---- ---- ---- 0-46     <- Clear Word
---- -5-- ---e 0246     <- Clear Word


@10ed0
ast:	4226 4C26 FF13 8038 B&L&...8
ast:	4544 4954 FF31 2043 EDIT.1 C





**********3

lok:
007FF0: C380 78A4 0008 C382-2805 F405 0040 283C ..x.....(....@(<
008000: C7CB 6000 4300 5727-C7CB 2000 6100 E624 ..`.C.W'.. .a..$
008010: 8100 C7CB 1000 A100-E6A4 4300 5727 0068 ..........C.W'.h
008020: 66FB A36E C7CB 0000-0400 C6CB 2000 E200 f..n........ ...
008030: C6CB 4000 2400 C6CB-6000 6400 C2C0 A000 ..@.$...`.d.....
lok_2
007FF8: 600C                     bra     #$c; 8006
007FFA: C07C 0001                and.w   #$1, D0
007FFE: 6606                     bne     #$6; 8006
008000: D37C                     dc.w $d37c; ILLEGAL
008002: 0006 0034                ori.b   #$34, D6
008006: E475                     roxr.w  D2, D5
008008: D37C                     dc.w $d37c; ILLEGAL
00800A: 0002 0016                ori.b   #$16, D2
00800E: 246E 0018                movea.l ($18,A6), A2
008012: D37C                     dc.w $d37c; ILLEGAL
Ast_2
007FCE: 600C                     bra     #$c; 7fdc
007FD0: C07C 0001                and.w   #$1, D0
007FD4: 6606                     bne     #$6; 7fdc
>
007FD6: 3D7C 0006 0034           move.w  #$6, ($34,A6)
007FDC: 4E75                     rts
007FDE: 3D7C 0002 0016           move.w  #$2, ($16,A6)
007FE4: 426E 0018                clr.w   ($18,A6)
007FE8: 3D7C 0001 001A           move.w  #$1, ($1a,A6)
007FEE: 526E 0034                addq.w  #1, ($34,A6)
007FF2: 4E75                     rts
007FF4: 6100 FD66                bsr     #-$29a; 7d5c

lok:
008000: C7CB 6000 4300 5727-C7CB 2000 6100 E624 ..`.C.W'.. .a..$
008010: 8100 C7CB 1000 A100-E6A4 4300 5727 0068 ..........C.W'.h
ast_2:
007FD6: 3D7C 0006 0034 4E75-3D7C 0002 0016 426E
007FE6: 0018 3D7C 0001 001A 526E 0034 4E75 6100

fedc ba98 7654 3210     <- Encrypted Word
-5-7 --2- -a98 fedc     <- Clear Word

*/


void astyanax_rom_decode(void)
{
unsigned char *RAM;
int i,size;

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	size = Machine->memory_region_length[Machine->drv->cpu[0].memory_region];
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size ; i+=2)
	{
	int x,y;

		x = READ_WORD(&RAM[i]);

// [0] def0 a981 65cb 7234
// [1] fdb9 7531 8ace 0246
// [2] 4567 0123 ba98 fedc

#define BITSWAP_0	BITSWAP(x,0xd,0xe,0xf,0x0,0xa,0x9,0x8,0x1,0x6,0x5,0xc,0xb,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0x8,0xa,0xc,0xe,0x0,0x2,0x4,0x6)
#define BITSWAP_2	BITSWAP(x,0x4,0x5,0x6,0x7,0x0,0x1,0x2,0x3,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000)	{ if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000)	{ y = BITSWAP_2; }
		else if	(i < 0x18000)	{ if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		WRITE_WORD(&RAM[i],y);
	}
}


void astyanax_init(void)
{
	unsigned char *RAM;

	astyanax_rom_decode();

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	WRITE_WORD(&RAM[0x0004e6],0x6040);	// protection
}


/* OSC:	? */
MEGASYS1_GAME(	astyanax,The Astyanax,1989,ORIENTATION_DEFAULT,
				A,0xff0000,0xffffff,
					4000000,4000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				astyanax_init, 0 )

MEGASYS1_GAME_CLONE( lordofk, astyanax, The Lord of King (Japan),1989,ORIENTATION_DEFAULT,A,astyanax_init, 0 )


/***************************************************************************

							[ Avenging Spirit ]

interrupts:	2,3, 5,6,7]		move.w  $e0000.l, $78e9e.l
							andi.w  #$ff, $78e9e.l
			4] 78b20 software watchdog (78ea0 enables it)

fd6		reads e0000 (values FF,06,34,35,36,37)
ffa		e0000<-6 test

79584.w *** level ***

***************************************************************************/

ROM_START( avspirit_rom )

	ROM_REGION(0xc0000)		/* Region 0 - main cpu code - 00000-3ffff & 80000-bffff */
	ROM_LOAD_EVEN( "spirit05.rom",  0x000000, 0x040000, 0xb26a341a )
	ROM_LOAD_ODD(  "spirit06.rom",  0x000000, 0x040000, 0x609f71fe )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "spirit12.rom",  0x000000, 0x080000, 0x728335d4 )
	ROM_LOAD( "spirit11.rom",  0x080000, 0x080000, 0x7896f6b0 )
	ROM_LOAD( "spirit09.rom",  0x100000, 0x020000, 0x0c37edf7 )
	ROM_LOAD( "spirit10.rom",  0x120000, 0x080000, 0x2b1180b3 )

	ROM_REGION(0x40000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "spirit01.rom",  0x000000, 0x020000, 0xd02ec045 )
	ROM_LOAD_ODD(  "spirit02.rom",  0x000000, 0x020000, 0x30213390 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "spirit14.rom",  0x000000, 0x040000, 0x13be9979 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "spirit13.rom",  0x000000, 0x040000, 0x05bc04d9 )
ROM_END

#define INPUT_PORTS_AVSPIRIT \
	COINS \
	JOY(IPF_PLAYER1) \
	RESERVE \
	JOY(IPF_PLAYER2) \
	COINAGE_C \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x08, "Easy" ) \
	PORT_DIPSETTING(    0x18, "Normal" ) \
	PORT_DIPSETTING(    0x10, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )


INPUT_PORTS_START( input_ports_avspirit )
	INPUT_PORTS_AVSPIRIT
INPUT_PORTS_END

void avspirit_init(void)
{
	ip_select_values[0] = 0x37;
	ip_select_values[1] = 0x35;
	ip_select_values[2] = 0x36;
	ip_select_values[3] = 0x33;
	ip_select_values[4] = 0x34;
}

/* OSC:	8,12,7 MHz */
MEGASYS1_GAME(	avspirit, Avenging Spirit,1991,ORIENTATION_DEFAULT,
				B,0x070000,0x07ffff,
				12000000,8000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				avspirit_init, 0 )



/***************************************************************************

							[ Chimera Beast ]

interrupts:	1,3]
			2, 5,6]
			4]

***************************************************************************/

ROM_START( chimerab_rom )

	ROM_REGION(0x80000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "prg3.bin", 0x000000, 0x040000, 0x70f1448f )
	ROM_LOAD_ODD(  "prg2.bin", 0x000000, 0x040000, 0x821dbb85 )

	ROM_REGION(0x220000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "s1.bin",   0x000000, 0x080000, 0xe4c2ac77 )	// scroll 0
	ROM_LOAD( "s2.bin",   0x080000, 0x080000, 0xfafb37a5 )	// scroll 1
	ROM_LOAD( "scr3.bin", 0x100000, 0x020000, 0x5fe38a83 )	// scroll 2
	ROM_LOAD( "b2.bin",   0x120000, 0x080000, 0x6e7f1778 )	// sprites
	ROM_LOAD( "b1.bin",   0x1a0000, 0x080000, 0x29c0385e )	//

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "prg8.bin", 0x000000, 0x010000, 0xa682b1ca )
	ROM_LOAD_ODD(  "prg7.bin", 0x000000, 0x010000, 0x83b9982d )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "voi11.bin", 0x000000, 0x040000, 0x14b3afe6 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "voi10.bin", 0x000000, 0x040000, 0x67498914 )

ROM_END

INPUT_PORTS_START( input_ports_chimerab )

	COINS
	JOY(IPF_PLAYER1)
	RESERVE				// Unused
	JOY(IPF_PLAYER2)

	PORT_START			/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	COINAGE_C			/* DSW B */

INPUT_PORTS_END

void chimerab_init(void)
{
	/* same as cybattlr */
	ip_select_values[0] = 0x56;
	ip_select_values[1] = 0x52;
	ip_select_values[2] = 0x53;
	ip_select_values[3] = 0x54;
	ip_select_values[4] = 0x55;
}


/* OSC:	? */
MEGASYS1_GAME(	chimerab,Chimera Beast,1993,ORIENTATION_DEFAULT,
				C,0xff0000,0xffffff,
				4000000,4000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				chimerab_init, 0 )


/***************************************************************************

								[ Cybattler ]

interrupts:	1,3]	408
			2, 5,6]	498
					1fd2c2.w routine index:
					0:	4be>	1fd2c0.w <- d8000
					2:	4ca>	1fd2d0+(1fd2c4.w) <- d8000.	next
					4:	4ee>	1fd2c4.w += 2.
											S	P1	P2	DB	DA
								d8000 <-	56	52	53	55	54
								1fd000+ 	00	02	04	06	08
								depending on 1fd2c4.		previous
					6:	4be again

			4]		452

c2208 <- 1fd040	(layers enable)
c2200 <- 1fd042	(sprite control)
c2308 <- 1fd046	(screen control)
c2004 <- 1fd054 (scroll 0 ctrl)	c2000 <- 1fd220 (scroll 0 x)	c2002 <- 1fd222 (scroll 1 y)
c200c <- 1fd05a (scroll 1 ctrl)	c2008 <- 1fd224 (scroll 1 x)	c200a <- 1fd226 (scroll 2 y)
c2104 <- 1fd060 (scroll 2 ctrl)	c2100 <- 1fd228 (scroll 2 x)	c2102 <- 1fd22a (scroll 3 y)

1f0010.w	*** level (0,1,..) ***
1fb044.l	*** score / 10 ***

***************************************************************************/

ROM_START( cybattlr_rom )

	ROM_REGION(0x80000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "cb_03.rom", 0x000000, 0x040000, 0xbee20587 )
	ROM_LOAD_ODD(  "cb_02.rom", 0x000000, 0x040000, 0x2ed14c50 )

	ROM_REGION(0x220000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "cb_m01.rom", 0x000000, 0x080000, 0x1109337f )
	ROM_LOAD( "cb_m04.rom", 0x080000, 0x080000, 0x0c91798e )
	ROM_LOAD( "cb_09.rom",  0x100000, 0x020000, 0x37b1f195 ) // charset
	ROM_LOAD( "cb_m03.rom", 0x120000, 0x080000, 0x4cd49f58 )
	ROM_LOAD( "cb_m02.rom", 0x1a0000, 0x080000, 0x882825db )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "cb_08.rom", 0x000000, 0x010000, 0xbf7b3558 )
	ROM_LOAD_ODD(  "cb_07.rom", 0x000000, 0x010000, 0x85d219d7 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "cb_11.rom", 0x000000, 0x040000, 0x59d62d1f )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "cb_10.rom", 0x000000, 0x040000, 0x8af95eed )

ROM_END

INPUT_PORTS_START( input_ports_cybattlr )

	COINS
	JOY(IPF_PLAYER1)
	RESERVE				// Unused
	JOY(IPF_PLAYER2)

	PORT_START			/* IN - DSW 1 - 1fd2d9.b, !1fd009.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START			/* IN - DSW 2 - 1fd2d7.b, !1fd007.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )	// 1 bit
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )	// 1 bit
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/*
int sound_interrupt_cybattlr(void)
{
	if (cpu_getiloops() % 2)	return 2;
	else 						return 4;
}
*/



void cybattlr_init(void)
{
	ip_select_values[0] = 0x56;
	ip_select_values[1] = 0x52;
	ip_select_values[2] = 0x53;
	ip_select_values[3] = 0x54;
	ip_select_values[4] = 0x55;
}


/* OSC:	? */
MEGASYS1_GAME(	cybattlr,Cybattler,1993,ORIENTATION_ROTATE_90,
				C,0x1f0000,0x01fffff,
					9000000,8000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				cybattlr_init, 0 )



/***************************************************************************

						 [ Earth Defense Force ]

interrupts:	2,3]	543C>	move.w  $e0000.l,	$60da6.l
							move.w  #$ffff,		$60da8.l
			4,5,6]	5928 +	move.w	#$ffff,		$60010.l

89e			(a7)+ -> 44000.w & 6000e.w
8cc			(a7)+ -> 44204.w ; 4420c.w ; 4400c.w
fc0			(a7)+ -> 58000 (string)

616f4.w		*** lives ***
60d8a.w		*** level(1..) ***

***************************************************************************/

ROM_START( edf_rom )

	ROM_REGION(0xc0000)		/* Region 0 - main cpu code - 00000-3ffff & 80000-bffff */
	ROM_LOAD_EVEN( "edf_05.rom",  0x000000, 0x040000, 0x105094d1 )
	ROM_LOAD_ODD(  "edf_06.rom",  0x000000, 0x040000, 0x94da2f0c )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "edf_m04.rom",  0x000000, 0x080000, 0x6744f406 )
	ROM_LOAD( "edf_m05.rom",  0x080000, 0x080000, 0x6f47e456 )
	ROM_LOAD( "edf_09.rom",   0x100000, 0x020000, 0x96e38983 )
	ROM_LOAD( "edf_m03.rom",  0x120000, 0x080000, 0xef469449 )

	ROM_REGION(0x40000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "edf_01.rom",  0x000000, 0x020000, 0x2290ea19 )
	ROM_LOAD_ODD(  "edf_02.rom",  0x000000, 0x020000, 0xce93643e )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "edf_m02.rom",  0x000000, 0x040000, 0xfc4281d2 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "edf_m01.rom",  0x000000, 0x040000, 0x9149286b )

ROM_END

INPUT_PORTS_START( input_ports_edf )
	COINS
	JOY(IPF_PLAYER1)
	RESERVE
	JOY(IPF_PLAYER2)
	COINAGE_A_2
	PORT_START			/* IN5 0x66007.b */
	PORT_DIPNAME( 0x07, 0x07, "DSW-B bits 2-0" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x40, "DSW-B bit 6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

void edf_init(void)
{
	ip_select_values[0] = 0x20;
	ip_select_values[1] = 0x21;
	ip_select_values[2] = 0x22;
	ip_select_values[3] = 0x23;
	ip_select_values[4] = 0x24;
}


/* OSC:	8,12,7 MHz */
MEGASYS1_GAME(	edf,Earth Defense Force,1991,ORIENTATION_DEFAULT,
				B,0x060000,0x07ffff,
				12000000,8000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				edf_init, 0 )


/***************************************************************************

								[ Hachoo ]

***************************************************************************/

ROM_START( hachoo_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "hacho02.rom", 0x000000, 0x020000, 0x49489c27 )
	ROM_LOAD_ODD(  "hacho01.rom", 0x000000, 0x020000, 0x97fc9515 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "hacho14.rom", 0x000000, 0x080000, 0x10188483 )	// scroll 0
	ROM_LOAD( "hacho15.rom", 0x080000, 0x020000, 0xe559347e )	// scroll 1
	ROM_LOAD( "hacho16.rom", 0x0a0000, 0x020000, 0x105fd8b5 )
	ROM_LOAD( "hacho17.rom", 0x0c0000, 0x020000, 0x77f46174 )
	ROM_LOAD( "hacho18.rom", 0x0e0000, 0x020000, 0x0be21111 )
	ROM_LOAD( "hacho19.rom", 0x100000, 0x020000, 0x33bc9de3 )	// scroll 2
	ROM_LOAD( "hacho20.rom", 0x120000, 0x020000, 0x2ae2011e )	// sprites
	ROM_LOAD( "hacho21.rom", 0x140000, 0x020000, 0x6dcfb8d5 )
	ROM_LOAD( "hacho22.rom", 0x160000, 0x020000, 0xccabf0e0 )
	ROM_LOAD( "hacho23.rom", 0x180000, 0x020000, 0xff5f77aa )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "hacho05.rom", 0x000000, 0x010000, 0x6271f74f )
	ROM_LOAD_ODD(  "hacho06.rom", 0x000000, 0x010000, 0xdb9e743c )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "hacho09.rom", 0x000000, 0x020000, 0xe9f35c90 )
	ROM_LOAD( "hacho10.rom", 0x020000, 0x020000, 0x1aeaa188 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "hacho07.rom", 0x000000, 0x020000, 0x06e6ca7f )
	ROM_LOAD( "hacho08.rom", 0x020000, 0x020000, 0x888a6df1 )

ROM_END

INPUT_PORTS_START( input_ports_hachoo )
	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80007.b */
	PORT_START			/* IN5 0x80006.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

void hachoo_init(void)
{
	unsigned char *RAM;

	astyanax_rom_decode();

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	WRITE_WORD(&RAM[0x0006da],0x6000);	// protection
}

/* OSC:	4,7,12 MHz */
MEGASYS1_GAME(	hachoo,Hachoo!,1989,ORIENTATION_DEFAULT,
				A,0x0f0000,0x0fffff,
					12000000,12000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				hachoo_init, 0 )




/***************************************************************************

							[ Iga Ninjyutsuden ]

interrupts:	1] 420		2] 500		3] 410

f010c.w		credits

***************************************************************************/


ROM_START( iganinju_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "iga_02.bin", 0x000000, 0x020000, 0xbd00c280 )
	ROM_LOAD_ODD(  "iga_01.bin", 0x000000, 0x020000, 0xfa416a9e )
	ROM_LOAD_EVEN( "iga_03.bin", 0x040000, 0x010000, 0xde5937ad )
	ROM_LOAD_ODD(  "iga_04.bin", 0x040000, 0x010000, 0xafaf0480 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "iga_14.bin", 0x000000, 0x040000, 0xc707d513 )	// scroll 0
	ROM_LOAD( "iga_18.bin", 0x080000, 0x080000, 0x6c727519 )	// scroll 1
	ROM_LOAD( "iga_19.bin", 0x100000, 0x020000, 0x98a7e998 )	// scroll 2
	ROM_LOAD( "iga_23.bin", 0x120000, 0x080000, 0xfb58c5f4 )	// sprites

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "iga_05.bin", 0x000000, 0x010000, 0x13580868 )
	ROM_LOAD_ODD(  "iga_06.bin", 0x000000, 0x010000, 0x7904d5dd )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "iga_08.bin", 0x000000, 0x040000, 0x857dbf60 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "iga_10.bin", 0x000000, 0x040000, 0x67a89e0d )

ROM_END


INPUT_PORTS_START( input_ports_iganinju )

	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	PORT_START 			/* IN4 0x80007.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze Screen", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/*
lives	---- --10:	3->2	2->4	1->3	0->$1000
		---- -2--:	1->5	0->2
cont	---- 3---:	1->-1	0->0
		--10 ----:	3->3	2->2	1->1	0->0
		-2-- ----:	1->0	0->-1
		3--- ----:	1->0	0->-1
*/
	PORT_START			/* IN5 - 0x80006.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

static void phantasm_rom_decode(void);

static void iganinju_init(void)
{
	unsigned char *RAM;

	phantasm_rom_decode();

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	WRITE_WORD(&RAM[0x02f000],0x835d);	// protection
}

/* OSC:	? */
MEGASYS1_GAME(	iganinju,Iga Ninjyutsuden,1989,ORIENTATION_DEFAULT,
				A,0x0f0000,0x0fffff,
					7000000,7000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				iganinju_init, 0 )



/***************************************************************************

							[ Legend of Makai ]

***************************************************************************/

ROM_START( lomakaj_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "lom_30.rom", 0x000000, 0x020000, 0xba6d65b8 )
	ROM_LOAD_ODD(  "lom_20.rom", 0x000000, 0x020000, 0x56a00dc2 )

	ROM_REGION_DISPOSE(0x50000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "lom_05.rom", 0x000000, 0x020000, 0xd04fc713 )
	ROM_LOAD( "lom_08.rom", 0x020000, 0x010000, 0xbdb15e67 )
	ROM_LOAD( "lom_06.rom", 0x030000, 0x020000, 0xf33b6eed )

	ROM_REGION(0x10000)		/* Region 2 - sound cpu code */
	ROM_LOAD( "lom_01.rom",  0x0000, 0x10000, 0x46e85e90 )

ROM_END

INPUT_PORTS_START( input_ports_lomakaj )
	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x30, 0x30, "?Difficulty?" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Hardest" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/* OSC:	5,12 MHz */
MEGASYS1_GAME(	lomakaj,Legend of Makai,1988,ORIENTATION_DEFAULT,
							A,0x0f0000,0x0fffff,
				6000000,3000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				0, 0 )



/***************************************************************************

							 [ P - 47 ]

(Japan version)
interrupts:	1]	53e		2] 540

517a		print word string: (a6)+,(a5)+$40. FFFF ends
5dbc		print string(s) to (a1)+$40: a6-> len.b,x.b,y.b,(chars.b)*
726a		prints screen
7300		ram test
7558		ip test
75e6(7638 loop)	sound test
	84300.w		<-f1002.w	?portrait F/F on(0x0100)/off(0x0000)
	84308.w		<-f1004.w	sound code

7736(7eb4 loop)	scroll 0 test
	9809c		color
	980a0		hscroll
	980a4		vscroll
	980a8		charsize

	7e1e		prepare screen
	7e84		get user input
	7faa		vhscroll
	80ce		print value.l from a0

785c(78b8 loop)	obj check 1		84000.w	<-0x0E	84100.w	<-0x101
	9804c	size
	98050	number		(0e.w bit 11-0)
	98054	color code	(08.w bit 2-0)
	98058	H flip		(08.w bit 6)
	9805c	V flip		(08.w bit 7)
	98060	priority	(08.w bit 3)
	98064	mosaic		(08.w bit 11-8)
	98068	mosaic sol.	(08.w bit 12)

7afe(7cfe loop)	obj check 2		84000.w	<-0x0f	84100.w	<-0x00
	9804a	obj num	(a4-8e000)/8
	9804e	H-rev	a4+02.w
	98052	V-rev	a4+04.w
	98056	CG-rev	a4+06.w
	9805a	Rem.Eff bit   4 of 84100
	98060	Rem.Num bit 3-0 of 84100 (see 7dd4)

TRAP#2		pause?
f0104.w		*** initial lives ***
f002a/116.w	<-!80000
f0810.w		<-!80002
f0c00.w		<-!80004
f0018.w		*** level ***


***************************************************************************/

ROM_START( p47_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "p47us3.bin", 0x000000, 0x020000, 0x022e58b8 )
	ROM_LOAD_ODD(  "p47us1.bin", 0x000000, 0x020000, 0xed926bd8 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c ) /* Tiles (scroll 0) */
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )
	ROM_LOAD( "p47j_23.bin", 0x080000, 0x020000, 0x6e9bc864 ) /* Tiles (scroll 1) */
	ROM_RELOAD(              0x0a0000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x0c0000, 0x020000, 0x5268395f )
	ROM_LOAD( "p47us16.bin", 0x100000, 0x010000, 0x5a682c8f ) /* Text  (scroll 2) */
	ROM_LOAD( "p47j_27.bin", 0x120000, 0x020000, 0x9e2bde8e ) /* Sprites 16x16 */
	ROM_LOAD( "p47j_18.bin", 0x140000, 0x020000, 0x29d8f676 )
	ROM_LOAD( "p47j_26.bin", 0x160000, 0x020000, 0x4d07581a )
	ROM_RELOAD(              0x180000, 0x020000 )	/* why? */

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "p47j_9.bin",  0x000000, 0x010000, 0xffcf318e )
	ROM_LOAD_ODD(  "p47j_19.bin", 0x000000, 0x010000, 0xadb8c12e )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "p47j_20.bin", 0x000000, 0x020000, 0x2ed53624 )
	ROM_LOAD( "p47j_21.bin", 0x020000, 0x020000, 0x6f56b56d )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "p47j_10.bin", 0x000000, 0x020000, 0xb9d79c1e )
	ROM_LOAD( "p47j_11.bin", 0x020000, 0x020000, 0xfa0d1887 )

ROM_END


ROM_START( p47j_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "p47j_3.bin", 0x000000, 0x020000, 0x11c655e5 )
	ROM_LOAD_ODD(  "p47j_1.bin", 0x000000, 0x020000, 0x0a5998de )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c ) /* Tiles (scroll 0) */
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )
	ROM_LOAD( "p47j_23.bin", 0x080000, 0x020000, 0x6e9bc864 ) /* Tiles (scroll 1) */
	ROM_RELOAD(              0x0a0000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x0c0000, 0x020000, 0x5268395f )
	ROM_LOAD( "p47j_16.bin", 0x100000, 0x010000, 0x30e44375 ) /* Text  (scroll 2) */
	ROM_LOAD( "p47j_27.bin", 0x120000, 0x020000, 0x9e2bde8e ) /* Sprites 16x16 */
	ROM_LOAD( "p47j_18.bin", 0x140000, 0x020000, 0x29d8f676 )
	ROM_LOAD( "p47j_26.bin", 0x160000, 0x020000, 0x4d07581a )
	ROM_RELOAD(              0x180000, 0x020000 )	/* why? */

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "p47j_9.bin",  0x000000, 0x010000, 0xffcf318e )
	ROM_LOAD_ODD(  "p47j_19.bin", 0x000000, 0x010000, 0xadb8c12e )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "p47j_20.bin", 0x000000, 0x020000, 0x2ed53624 )
	ROM_LOAD( "p47j_21.bin", 0x020000, 0x020000, 0x6f56b56d )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "p47j_10.bin", 0x000000, 0x020000, 0xb9d79c1e )
	ROM_LOAD( "p47j_11.bin", 0x020000, 0x020000, 0xfa0d1887 )

ROM_END

INPUT_PORTS_START( input_ports_p47 )

	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Hardest" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/* OSC:	? (Sound CPU clock drives the music tempo!)*/
MEGASYS1_GAME(	p47,P-47 - The Phantom Fighter (World),1988,ORIENTATION_DEFAULT,
				A,0x0f0000,0x0fffff,
				7000000,7000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				0, 0 )

MEGASYS1_GAME_CLONE( p47j, p47, P-47 - The Freedom Fighter (Japan), 1988, ORIENTATION_DEFAULT, A, 0, 0 )



/***************************************************************************

								[ Phantasm ]
		(Japanese version of Avenging Spirit on different hardware)

                        spirit02.rom                   1xxxxxxxxxxxxxxxx = 0xFF
                        spirit01.rom                   1xxxxxxxxxxxxxxxx = 0xFF
phntsm08.bin            spirit13.rom            IDENTICAL
phntsm10.bin            spirit14.rom            IDENTICAL
phntsm14.bin            spirit12.rom            IDENTICAL
phntsm18.bin            spirit11.rom            IDENTICAL
phntsm19.bin            spirit09.rom            IDENTICAL
phntsm23.bin            spirit10.rom            IDENTICAL
phntsm05.bin            spirit01.rom [1st half] IDENTICAL
phntsm06.bin            spirit02.rom [1st half] IDENTICAL
phntsm01.bin            spirit05.rom [1st half] 38.631%
phntsm02.bin            spirit06.rom [1st half] 22.478%
phntsm03.bin            spirit02.rom [2nd half] 14.528%
phntsm03.bin            spirit01.rom [2nd half] 14.528%
phntsm04.bin                                    NO MATCH


1] E9C
2] ED4
3] F4C		rte
4-7] ED2	rte

***************************************************************************/

ROM_START( phantasm_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "phntsm02.bin", 0x000000, 0x020000, 0xd96a3584 )
	ROM_LOAD_ODD(  "phntsm01.bin", 0x000000, 0x020000, 0xa54b4b87 )
	ROM_LOAD_EVEN( "phntsm03.bin", 0x040000, 0x010000, 0x1d96ce20 )
	ROM_LOAD_ODD(  "phntsm04.bin", 0x040000, 0x010000, 0xdc0c4994 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "spirit12.rom",  0x000000, 0x080000, 0x728335d4 )
	ROM_LOAD( "spirit11.rom",  0x080000, 0x080000, 0x7896f6b0 )
	ROM_LOAD( "spirit09.rom",  0x100000, 0x020000, 0x0c37edf7 )
	ROM_LOAD( "spirit10.rom",  0x120000, 0x080000, 0x2b1180b3 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "phntsm05.bin", 0x000000, 0x010000, 0x3b169b4a )
	ROM_LOAD_ODD(  "phntsm06.bin", 0x000000, 0x010000, 0xdf2dfb2e )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "spirit14.rom",  0x000000, 0x040000, 0x13be9979 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "spirit13.rom",  0x000000, 0x040000, 0x05bc04d9 )
ROM_END

INPUT_PORTS_START( input_ports_phantasm )
	INPUT_PORTS_AVSPIRIT
INPUT_PORTS_END


static void phantasm_rom_decode(void)
{
unsigned char *RAM;
int i,size;

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	size = Machine->memory_region_length[Machine->drv->cpu[0].memory_region];
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size ; i+=2)
	{
	int x,y;

		x = READ_WORD(&RAM[i]);

// [0] def0 189a bc56 7234
// [1] fdb9 7531 eca8 6420
// [2] 0123 4567 ba98 fedc
#define BITSWAP_0	BITSWAP(x,0xd,0xe,0xf,0x0,0x1,0x8,0x9,0xa,0xb,0xc,0x5,0x6,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0xe,0xc,0xa,0x8,0x6,0x4,0x2,0x0)
#define BITSWAP_2	BITSWAP(x,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000)	{ if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000)	{ y = BITSWAP_2; }
		else if	(i < 0x18000)	{ if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		WRITE_WORD(&RAM[i],y);
	}

}


/* OSC: same as avspirit(8,12,7 MHz)? (Music tempo driven by timers of the 2151) */
MEGASYS1_GAME_EXT(	phantasm, phantasm, &avspirit_driver, Phantasm (Japan), 1990, ORIENTATION_DEFAULT,
					A,0xff0000,0xffffff,
						8000000,4000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
					phantasm_rom_decode, 0)



/***************************************************************************

							[ Plus Alpha ]
						  (aka Flight Alpha)

***************************************************************************/

ROM_START( plusalph_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "pa-rom2.bin", 0x000000, 0x020000, 0x33244799 )
	ROM_LOAD_ODD(  "pa-rom1.bin", 0x000000, 0x020000, 0xa32fdcae )
	ROM_LOAD_EVEN( "pa-rom3.bin", 0x040000, 0x010000, 0x1b739835 )
	ROM_LOAD_ODD(  "pa-rom4.bin", 0x040000, 0x010000, 0xff760e80 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "pa-rom11.bin", 0x000000, 0x020000, 0xeb709ae7 )	//* scroll 0
	ROM_LOAD( "pa-rom12.bin", 0x020000, 0x020000, 0xcacbc350 )
	ROM_LOAD( "pa-rom13.bin", 0x040000, 0x020000, 0xfad093dd )

	// scroll 1
	ROM_LOAD( "pa-rom15.bin", 0x080000, 0x020000, 0x8787735b )
	ROM_LOAD( "pa-rom17.bin", 0x0a0000, 0x020000, 0xc6b38a4b )
	ROM_LOAD( "pa-rom14.bin", 0x0c0000, 0x020000, 0xd3676cd1 )
	ROM_LOAD( "pa-rom16.bin", 0x0e0000, 0x020000, 0xa06b813b )

	ROM_LOAD( "pa-rom19.bin", 0x100000, 0x010000, 0x39ef193c )	// scroll 2
	ROM_LOAD( "pa-rom20.bin", 0x120000, 0x020000, 0x86c557a8 )	// sprites
	ROM_LOAD( "pa-rom21.bin", 0x140000, 0x020000, 0x81140a88 )
	ROM_LOAD( "pa-rom22.bin", 0x160000, 0x020000, 0x97e39886 )
	ROM_LOAD( "pa-rom23.bin", 0x180000, 0x020000, 0x0383fb65 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "pa-rom5.bin", 0x000000, 0x010000, 0xddc2739b )
	ROM_LOAD_ODD(  "pa-rom6.bin", 0x000000, 0x010000, 0xf6f8a167 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "pa-rom9.bin",  0x000000, 0x020000, 0x065364bd )
	ROM_LOAD( "pa-rom10.bin", 0x020000, 0x020000, 0x395df3b2 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "pa-rom7.bin",  0x000000, 0x020000, 0x9f5d800e )
	ROM_LOAD( "pa-rom8.bin",  0x020000, 0x020000, 0xae007750 )

ROM_END

INPUT_PORTS_START( input_ports_plusalph )
	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80007.b */

	PORT_START			/* IN5 0x80006.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

void plusalph_init(void)
{
	astyanax_rom_decode();
}

/* OSC:	? */
MEGASYS1_GAME(	plusalph,Plus Alpha,1989,ORIENTATION_ROTATE_270,
				A,0x0f0000,0x0fffff,
					4000000,4000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				plusalph_init, 0 )



/***************************************************************************

							[ RodLand ]

(World version)
interrupts:	1] 418->3864: rts	2] 420: move.w #-1,f0010; jsr 3866	3] rte

213da	print test error (20c12 = string address 0-4)

f0018->84200	f0020->84208	f0028->84008
f001c->84202	f0024->8420a	f002c->8400a
f0012->84204	f0014->8420c	f0016->8400c

7fe		d0.w -> 84000.w & f000e.w
81a		d0/d1/d2 & $D -> 84204 / 8420c /8400c

***************************************************************************/

ROM_START( rodland_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "rl_02.rom", 0x000000, 0x020000, 0xc7e00593 )
	ROM_LOAD_ODD(  "rl_01.rom", 0x000000, 0x020000, 0x2e748ca1 )
	ROM_LOAD_EVEN( "rl_03.rom", 0x040000, 0x010000, 0x62fdf6d7 )
	ROM_LOAD_ODD(  "rl_04.rom", 0x040000, 0x010000, 0x44163c86 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "rl_23.rom", 0x000000, 0x080000, 0xac60e771 )
	ROM_LOAD( "rl_18.rom", 0x080000, 0x080000, 0xf3b30ca6 )
	ROM_LOAD( "rl_19.rom", 0x100000, 0x020000, 0x1b718e2a )
	ROM_LOAD( "rl_14.rom", 0x120000, 0x080000, 0x08d01bf4 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "rl_05.rom", 0x000000, 0x010000, 0xc1617c28 )
	ROM_LOAD_ODD(  "rl_06.rom", 0x000000, 0x010000, 0x663392b2 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "rl_08.rom", 0x000000, 0x040000, 0x8a49d3a7 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "rl_10.rom", 0x000000, 0x040000, 0xe1d1cd99 )

ROM_END


ROM_START( rodlandj_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "rl19.bin", 0x000000, 0x010000, 0x028de21f )
	ROM_LOAD_ODD(  "rl17.bin", 0x000000, 0x010000, 0x9c720046 )
	ROM_LOAD_EVEN( "rl20.bin", 0x020000, 0x010000, 0x3f536d07 )
	ROM_LOAD_ODD(  "rl18.bin", 0x020000, 0x010000, 0x5aa61717 )
	ROM_LOAD_EVEN( "rl12.bin", 0x040000, 0x010000, 0xc5b1075f )	// ~ rl_03.rom
	ROM_LOAD_ODD(  "rl11.bin", 0x040000, 0x010000, 0x9ec61048 )	// ~ rl_04.rom

	// bg RL_23 = 27, 28, 29b, 29a, 31a?, 30?, ?, 31b?
	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "rl27.bin",  0x000000, 0x010000, 0x673a5986 )	// bg
	ROM_LOAD( "rl28.bin",  0x010000, 0x010000, 0x523a731d )
	ROM_LOAD( "rl26.bin",  0x020000, 0x010000, 0x4d0a5c97 )
	ROM_LOAD( "rl29a.bin", 0x030000, 0x010000, 0x9fd628f1 )
	ROM_LOAD( "rl29b.bin", 0x040000, 0x010000, 0x2279cb76 )
	ROM_LOAD( "rl30.bin",  0x050000, 0x010000, 0xb155f39e )
	ROM_LOAD( "rl31a.bin", 0x060000, 0x010000, 0xa9bc5b84 )
	ROM_LOAD( "rl31b.bin", 0x070000, 0x010000, 0xfb2faa69 )
	ROM_LOAD( "rl21.bin",  0x080000, 0x010000, 0x32fc0bc6 )	// fg = RL_18
	ROM_LOAD( "rl22.bin",  0x090000, 0x010000, 0x0969daa9 )
	ROM_LOAD( "rl13.bin",  0x0a0000, 0x010000, 0x1203cdf6 )
	ROM_LOAD( "rl14.bin",  0x0b0000, 0x010000, 0xd53e094b )
	ROM_LOAD( "rl24.bin",  0x0c0000, 0x010000, 0xb04343e6 )
	ROM_LOAD( "rl23.bin",  0x0d0000, 0x010000, 0x70aa7e2c )
	ROM_LOAD( "rl15.bin",  0x0e0000, 0x010000, 0x38ac846e )
	ROM_LOAD( "rl16.bin",  0x0f0000, 0x010000, 0x5e31f0b2 )
	ROM_LOAD( "rl25.bin",  0x100000, 0x010000, 0x4ca57cb6 )	// txt = RL_19
	// Filled with color 15 at startup
	ROM_LOAD( "rl04.bin",  0x120000, 0x010000, 0xcfcf9f97 )	// sprites = RL_14
	ROM_LOAD( "rl05.bin",  0x130000, 0x010000, 0x38c05d15 )
	ROM_LOAD( "rl07.bin",  0x140000, 0x010000, 0xe117cb72 )
	ROM_LOAD( "rl08.bin",  0x150000, 0x010000, 0x2f9b40c3 )
	ROM_LOAD( "rl03.bin",  0x160000, 0x010000, 0xf6a88efd )
	ROM_LOAD( "rl06.bin",  0x170000, 0x010000, 0x90a78af1 )
	ROM_LOAD( "rl09.bin",  0x180000, 0x010000, 0x427a0908 )
	ROM_LOAD( "rl10.bin",  0x190000, 0x010000, 0x53cc2c11 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "rl02.bin", 0x000000, 0x010000, 0xd26eae8f )
	ROM_LOAD_ODD(  "rl01.bin", 0x000000, 0x010000, 0x04cf24bc )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "rl_08.rom", 0x000000, 0x040000, 0x8a49d3a7 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "rl_10.rom", 0x000000, 0x040000, 0xe1d1cd99 )

ROM_END


INPUT_PORTS_START( input_ports_rodland )

	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A_2			/* IN4 0x80006.b */
	PORT_START			/* IN5 0x80007.b */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) /* according to manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x10, 0x10, "Alternative Graphics" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy?" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

static void rodland_rom_decode(void)
{
unsigned char *RAM;
int i,size;

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	size = Machine->memory_region_length[Machine->drv->cpu[0].memory_region];
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size ; i+=2)
	{
	int x,y;

		x = READ_WORD(&RAM[i]);

// [0] d0a9 6ebf 5c72 3814	[1] 4567 0123 ba98 fedc
// [2] fdb9 ce07 5318 a246	[3] 4512 ed3b a967 08fc
#define BITSWAP_0	BITSWAP(x,0xd,0x0,0xa,0x9,0x6,0xe,0xb,0xf,0x5,0xc,0x7,0x2,0x3,0x8,0x1,0x4);
#define BITSWAP_1	BITSWAP(x,0x4,0x5,0x6,0x7,0x0,0x1,0x2,0x3,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc);
#define	BITSWAP_2	BITSWAP(x,0xf,0xd,0xb,0x9,0xc,0xe,0x0,0x7,0x5,0x3,0x1,0x8,0xa,0x2,0x4,0x6);
#define	BITSWAP_3	BITSWAP(x,0x4,0x5,0x1,0x2,0xe,0xd,0x3,0xb,0xa,0x9,0x6,0x7,0x0,0x8,0xf,0xc);

		if		(i < 0x08000)	{	if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000)	{	if ( (i | 0x248) != i ) {y = BITSWAP_2;} else {y = BITSWAP_3;} }
		else if	(i < 0x18000)	{	if ( (i | 0x248) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_3; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2
#undef	BITSWAP_3

		WRITE_WORD(&RAM[i],y);
	}
}


static void rodland_init(void)
{
	int i;
	unsigned char *RAM = Machine->memory_region[1];

	/* Second half of the text gfx should be all FF's, not 0's
	   (is this right ? Otherwise the subtitle is wrong) */
	for (i = 0x110000; i < 0x120000; i++)	RAM[i] = 0xFF;

	rodland_rom_decode();
}




void rodlandj_init(void)
{
	unsigned char *RAM = Machine->memory_region[1];
	int i;

/* Second half of the text gfx should be all FF's, not 0's
   (is this right ? Otherwise the subtitle is wrong) */
	for (i = 0x110000; i < 0x120000; i++)	RAM[i] = 0xFF;
}


/* OSC:	4,12,7 MHz (Sound CPU clock drives the music tempo!) */
MEGASYS1_GAME(	rodland,RodLand (World),1990,ORIENTATION_DEFAULT,
				A,0x0f0000,0x0fffff,
				12000000,7000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				rodland_init, 0 )

MEGASYS1_GAME_CLONE( rodlandj, rodland, RodLand (Japan), 1990, ORIENTATION_DEFAULT, A, rodlandj_init, 0 )




/***************************************************************************

							[ Saint Dragon ]

interrupts:	1] rte	2] 620	3] 5e6

***************************************************************************/

ROM_START( stdragon_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "jsd-02.bin", 0x000000, 0x020000, 0xcc29ab19 )
	ROM_LOAD_ODD(  "jsd-01.bin", 0x000000, 0x020000, 0x67429a57 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "jsd-11.bin", 0x000000, 0x020000, 0x2783b7b1 ) /* Tiles (scroll 0) */
	ROM_LOAD( "jsd-12.bin", 0x020000, 0x020000, 0x89466ab7 )
	ROM_LOAD( "jsd-13.bin", 0x040000, 0x020000, 0x9896ae82 )
	ROM_LOAD( "jsd-14.bin", 0x060000, 0x020000, 0x7e8da371 )
	ROM_LOAD( "jsd-15.bin", 0x080000, 0x020000, 0xe296bf59 ) /* Tiles (scroll 1) */
	ROM_LOAD( "jsd-16.bin", 0x0a0000, 0x020000, 0xd8919c06 )
	ROM_LOAD( "jsd-17.bin", 0x0c0000, 0x020000, 0x4f7ad563 )
	ROM_LOAD( "jsd-18.bin", 0x0e0000, 0x020000, 0x1f4da822 )
	ROM_LOAD( "jsd-19.bin", 0x100000, 0x010000, 0x25ce807d ) /* Text  (scroll 2) */
	ROM_LOAD( "jsd-20.bin", 0x120000, 0x020000, 0x2c6e93bb ) /* Sprites 16x16    */
	ROM_LOAD( "jsd-21.bin", 0x140000, 0x020000, 0x864bcc61 )
	ROM_LOAD( "jsd-22.bin", 0x160000, 0x020000, 0x44fe2547 )
	ROM_LOAD( "jsd-23.bin", 0x180000, 0x020000, 0x6b010e1a )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "jsd-05.bin", 0x000000, 0x010000, 0x8c04feaa )
	ROM_LOAD_ODD(  "jsd-06.bin", 0x000000, 0x010000, 0x0bb62f3a )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "jsd-07.bin", 0x000000, 0x020000, 0x6a48e979 )
	ROM_LOAD( "jsd-08.bin", 0x020000, 0x020000, 0x40704962 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "jsd-09.bin", 0x000000, 0x020000, 0xe366bc5a )
	ROM_LOAD( "jsd-10.bin", 0x020000, 0x020000, 0x4a8f4fe6 )

ROM_END

INPUT_PORTS_START( input_ports_stdragon )
	COINS				/* IN0 0x80001.b */
	JOY(IPF_PLAYER1)	/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */

	PORT_START			/* IN4 0x80007.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


	PORT_START			/* IN5 0x80006.b */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

void stdragon_init(void)
{
	unsigned char *RAM;

	phantasm_rom_decode();

	RAM  = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	WRITE_WORD(&RAM[0x00045e],0x0098);	// protection
}

/* OSC:	? */
MEGASYS1_GAME(	stdragon,Saint Dragon,1989,ORIENTATION_DEFAULT,
				A,0x0f0000,0x0fffff,
					10000000,4000000,STD_FM_CLOCK,STD_OKI_CLOCK,STD_OKI_CLOCK,
				stdragon_init, 0 )


#if 0
/***************************************************************************

							[ Peek-a-Boo! ]

interrupt:
	1] 		506>	rte
	2] 		50a>	move.w  #$ffff, $1f0006.l
					jsr     $46e0.l				rte
	3] 		51c>	rte
	4] 		520>	move.w  #$ffff, $1f000a.l	rte
	5-7]	53c>	rte

3832	d7 = ram segment where error occurred. Show error.
	1 after d8000 ok. 3 after e0000&d0000 ok. 4 after ram&rom ok

003E5E: 0000 3E72	[0]	Color Ram
003E62: 0000 3E86	[1]	Video Ram
003E66: 0000 3E9A	[2]	Sprite Ram
003E6A: 0000 3EB0	[3]	Work Ram
003E6E: 0000 3EC4	[4]	ROM


000000-03ffff	rom (3f760 chksum)
1f0000-1fffff	ram

0d0000-0d3fff	text
0d8000-0d87ff	palette (+200 = text palette)
0e8000-0ebfff	layer

0e0000-0e0001	2 dips, 1f003a<-!
	bit 2=service?
	bit 31=flip

0f0000-0f0001	2 controls
0f8000-0f8001	???



010000-010001	protection\watchdog;
	fb -> fb
	9x ->	0		watchdog reset?
			else	samples bank? ($1ff014 = sample - $22 = index:
					0033DC: 0001 0001 0002 0003 0004 0005 0006 0006 0006 0006

	51 -> paddle p1
	52 -> paddle p2
	4bba waits for 1f000a to go !0, then clears 1f000a (int 4)
	4bca waits (100000) & FF == 3
	sequence $81, $71, $67 written



Scroll x,y,ctrl:
c2000<-1f0010
c2002<-1f0014
c2004<-1f000c

Scroll x,y,ctrl:
c2008<-1f0018
c200a<-1f001c
c200c<-1f000e

Layers ctrl:
c2208<-1f0024<<8 + 1f0026
c2308<-1f0022 | 1f002c

Sprite bank + ??
c2108<-1f005a + 1f0060 + 1f0062 + 1f0068

Sprite ctrl:
c2200<-0

1f0000.w	routine index, table at $fae:
	0: 4E40
	1: 4EC2
	2: 4F2C
	3: 4F70
	4: 4FBC
	5: 533A
	6: 5382
	7: 556E

1f003c/40	paddle p1/p2
1f0260/4	p1/p2 score/10 (BCD)
1f02e6/8	p1/p2 current lives
1f02ee		current player (0/1)
1f0380		hi score

20000	100000	250000	500000	100000

***************************************************************************/

ROM_START( peekaboo_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "j3", 0x000000, 0x020000, 0xf5f4cf33 )
	ROM_LOAD_ODD(  "j2", 0x000000, 0x020000, 0x7b3d430d )

	ROM_REGION_DISPOSE(0x180000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "5",       0x000000, 0x080000, 0x34fa07bb )	// Background
	ROM_LOAD( "4",       0x080000, 0x020000, 0xf037794b )	// Text
	ROM_LOAD( "1",       0x100000, 0x080000, 0x5a444ecf )	// Sprites

	/* Samples here - not dumped yet */

ROM_END

INPUT_PORTS_START( input_ports_peekaboo )

	PORT_START		/* IN0 - COINS + P1&P2 Buttons - .b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN3 )		// called "service"
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN4 )		// called "test"
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )		// called "stage clear"
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 )		// called "option"
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define PEEKABOO_PADDLE(_FLAG_)	\
 PORT_ANALOG( 0x00ff, 0x0080, IPT_PADDLE | _FLAG_, 100, 25, 1, 0x0008, 0x00f8 )

	PORT_START      	/* IN1 - paddle p1 */
	PEEKABOO_PADDLE(IPF_PLAYER1)

	RESERVE				/* IN2 - fake port */
	PORT_START      	/* IN3 - paddle p2 */
	PEEKABOO_PADDLE(/*IPF_PLAYER2*/ IPF_COCKTAIL)

	PORT_START			/* IN4 - DSW 1 - 1f003a.b<-e0000.b */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )		// 1f0354<-
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )		// 1f0022/6e<-!
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START			/* IN5 - DSW 2 - 1f003b.b<-e0001.b */
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1" )				// 1f0358<-!
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Movement?" )					// 1f0392<-!
	PORT_DIPSETTING(    0x08, "Paddles" )
	PORT_DIPSETTING(    0x00, "Buttons" )
	PORT_DIPNAME( 0x30, 0x30, "Nudity" )					// 1f0356<-!
	PORT_DIPSETTING(    0x30, "Female and Male (Full)" )
	PORT_DIPSETTING(    0x20, "Female (Full)" )
	PORT_DIPSETTING(    0x10, "Female (Partial)" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )			// 1f006a<-!
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "(controls?)Unknown 2-7" )				// 1f0074<-!
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )	// num of controls?
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

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

void videoregs_peekaboo_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&videoregs[offset]);
	COMBINE_WORD_MEM(&videoregs[offset],data);
	new_data  = READ_WORD(&videoregs[offset]);

	switch (offset)
	{
			case 0x2208   : {active_layers = (new_data & 0xfffb)^0x300;} break;
//		case 0x2100+0 : videoreg_sx(2)
//		case 0x2100+2 : videoreg_sy(2)
//		case 0x2100+4 : videoreg_flag(2)
		case 0x2000+0 : videoreg_sx(0)
		case 0x2000+2 : videoreg_sy(0)
		case 0x2000+4 : videoreg_flag(0)
		case 0x2008+0 : videoreg_sx(1)
		case 0x2008+2 : videoreg_sy(1)
		case 0x2008+4 : videoreg_flag(1)
		case 0x2108   : {spritebank = new_data;} break;
		case 0x2200   : {spriteflag = new_data;} break;
		case 0x2308   : {screenflag = new_data;} break;
//		case 0x8000   : {soundlatch_w(0,new_data);} break;
		default: {if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, videoreg %04X <- %04X\n",cpu_get_pc(),offset,data);}
	}
}

int peekaboo_10k;

/* Read the input ports, through a protection device */
int  peekaboo_10k_r(int offset)
{
	switch (peekaboo_10k)
	{
		case 0x02:	return 0x03;
		case 0x51:	return player1_r(0);
		case 0x52:	return player2_r(0);
		default:	return peekaboo_10k;
	}
}
void peekaboo_10k_w(int offset,int data)
{
	peekaboo_10k = data;
	cpu_cause_interrupt(0,4);
}

static struct MemoryReadAddress readmem_peekaboo[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x1f0000, 0x1fffff, MRA_BANK1 },		/* RAM */
	{ 0x0c0000, 0x0c9fff, MRA_BANK2 },		/* Video Regs */
	{ 0x0ca000, 0x0cbfff, MRA_BANK3 },		/* Object  RAM   */
	{ 0x0d0000, 0x0d3fff, MRA_BANK4 },		/* Scroll RAM 1  */
		{ 0x0d4000, 0x0d7fff, MRA_BANK5 },		/* FAKE Scroll RAM 2, is it at 98000?  */
//	{ 0x0d8000, 0x0d87ff, MRA_BANK6 },		/* ?palette ? */
		{ 0x0d8000, 0x0d87ff, paletteram_word_r },		/* ?palette ? */
	{ 0x0db000, 0x0db7ff, paletteram_word_r },		/* ?palette ? */
	{ 0x0e0000, 0x0e0001, dsw_r },
	{ 0x0e8000, 0x0ebfff, MRA_BANK7 },		/* Scroll RAM 0  */
	{ 0x0f0000, 0x0f0001, coins_r },		/* coins + p1&p2 buttons */
	{ 0x0f8000, 0x0f8001, MRA_NOP },		// OKI M6295
	{ 0x100000, 0x100001, peekaboo_10k_r },
	{ -1 }
};
static struct MemoryWriteAddress writemem_peekaboo[] =
{
 	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x1f0000, 0x1fffff, MWA_BANK1, &ram },
	{ 0x0c0000, 0x0c9fff, videoregs_peekaboo_w, &videoregs },
	{ 0x0ca000, 0x0cbfff, MWA_BANK3, &megasys1_objectram },
	{ 0x0d0000, 0x0d3fff, scrollram_1_w, &scrollram[1] },
	/* Fake */	{ 0x0d4000, 0x0d7fff, scrollram_2_w, &scrollram[2] },
//	{ 0x0d8000, 0x0d87ff, MWA_BANK6 },
		{ 0x0d8000, 0x0d87ff, paletteram_RRRRRGGGGGBBBBBx_word_w },
	{ 0x0db000, 0x0db7ff, paletteram_RRRRRGGGGGBBBBBx_word_w, &paletteram },
	{ 0x0e8000, 0x0ebfff, scrollram_0_w, &scrollram[0] },
	{ 0x0f8000, 0x0f8001, MWA_NOP },		// OKI M6295
	{ 0x100000, 0x100001, peekaboo_10k_w },
	{ -1 }
};

static struct GfxLayout spritelayout_peekaboo =\
{\
	16,16,\
	8*0x80000/(16*16*8),\
	8,\
	{0, 1, 2, 3, 4, 5, 6, 7},\
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,\
	 0*8+64*16,1*8+64*16,2*8+64*16,3*8+64*16,4*8+64*16,5*4+64*16,6*4+64*16,7*4+64*16},\
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64,\
	 8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64},\
	16*16*8\
};\


static struct GfxDecodeInfo gfxdecodeinfo_peekaboo[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] Background
	{ 1, 0x080000, &tilelayout,		256*1, 16 },	// [1] Text
	{ 1, 0x000000, &tilelayout,		256*2, 16 },	// [2] *FAKE*
	{ 1, 0x100000, &spritelayout_A,	256*3, 16 },	// [3] sprites
	{ -1 }
};

int interrupt_peekaboo(void)	{ return 2; }

void vh_screenrefresh_peekaboo(struct osd_bitmap *bitmap,int full_refresh)
{
}

static struct MachineDriver machine_driver_peekaboo = \
{\
	{\
		{\
			CPU_M68000,\
			10000000,	/* ?10? */ \
			0,\
			readmem_peekaboo,writemem_peekaboo,0,0,\
			interrupt_peekaboo,1 \
		},\
	},\
	60,DEFAULT_60HZ_VBLANK_DURATION,\
	1,\
	megasys1_init_machine,\
	/* video hardware */ \
	256, 256,{ 0, 256-1, 0+16, 256-16-1 },\
	gfxdecodeinfo_peekaboo,\
	1024, 1024,\
	0,\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\
	0,\
	megasys1_vh_start,\
	0,\
		megasys1_vh_screenrefresh	/*_peekaboo*/,\
	/* sound hardware */ \
	0,0,0,0,\
	{
		{\
			SOUND_YM2151,\
			&ym2151_interface\
		},\
	}\
};

struct GameDriver peekaboo_driver =\
{\
	__FILE__,\
	0,\
	"peekaboo",\
	"Peek-a-Boo!",\
	"1993",\
	"Jaleco",\
	MEGASYS1_CREDITS,\
	0,\
	&machine_driver_peekaboo,\
	&driver_init_C,\
	peekaboo_rom,\
	0, 0,\
	0,\
	0,\
	input_ports_peekaboo,\
	0, 0, 0,\
	ORIENTATION_DEFAULT,\
	0,0\
};
#endif
