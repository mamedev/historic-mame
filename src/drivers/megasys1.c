/***************************************************************************

						-= Jaleco Mega System 1 =-

				driver by	Luca Elia (eliavit@unina.it)


Game				System		Dumped?		Supported?
------------------------------------------------------
64th Street				C		AraCORN 	Yes
64th Street (Japan)		C		J-Rom	 	Yes
Astyanax				?	 	Yes			encrypted
Avenging Spirit			B		AraCORN 	Yes
Cybattler				?		AraCORN 	encrypted
Earth Defense Force		B		AraCORN		Yes
Hachoo					?		AraCORN 	encrypted
Legend of Makai (World)	Z		AraCORN		Yes
Makai Densetsu (Japan)
P-47 (World)			A	 	Yes			Yes
P-47 (Japan)			A		J-rom		Yes
Phantasm*				A?		J-Rom 		encrypted
Plus Alpha				?		J-Rom 		encrypted
RodLand (World)			A?		AraCORN 	encrypted
RodLand (Japan)			A		Yes			Yes
Saint Dragon			?		J-Rom 		encrypted
Soldam					?		No?			-
------------------------------------------------------
* Japanese version of Avenging Spirit


Hardware	Main CPU	Sound CPU	Sound Chips
----------------------------------------------------------------
MS1 - Z		68000		Z80			YM2203c
MS1 - A		68000		68000		YM2151	OKI-M6295 x 2
MS1 - B		68000		68000		YM2151	OKI-M6295 x 2
MS1 - C		68000		68000		YM2151	OKI-M6295 x 2 YM3012
----------------------------------------------------------------


Main CPU	RW		MS1-A			MS1-B			MS1-C
-------------------------------------------------------------
ROM			R	000000-03ffff	000000-03ffff	000000-07ffff
ROM			R	-				080000-0bffff	-
VideoRegs	 W	084000-0843ff	044000-0443ff	0c0000-0cffff
Palette		RW	088000-0887ff	048000-0487ff	0f8000-0f87ff
Object RAM	RW	08e000-08ffff	04e000-04ffff	0d2000-0d3fff
Scroll 1	RW	090000-093fff	050000-053fff	0e0000-0e3fff
Scroll 2	RW	094000-097fff	054000-057fff	0e8000-0ebfff
Scroll 3	RW	098000-09bfff	058000-05bfff	0f0000-0f3fff
Work RAM	RW	0f0000-0fffff	060000-07ffff*	ff0000-ffffff
Input Ports R	080000-080009	0e0000-0e0001**	0d8000-d80001**
-------------------------------------------------------------
*	Avenging spirit ram starts at 070000
**	Through protection


Sound CPU	RW		MS1-A			MS1-B			MS1-C
-------------------------------------------------------------
ROM			R	000000-01ffff	000000-03ffff	000000-01ffff
Soundlatch	R	040000-040001	<				060000-060001
Soundlatch2	 W	060000-060001	<				<
?			 W	-				040000-040001	<
2151 reg	 W	080000-080001	<				<
2151 data	 W	080002-080003	<				<
2151 status	R 	080002-080003	<				<
6295 data 0	 W 	0a0000-0a0003	<				<
6295 stat 0	R 	0a0000-0a0001	<				<
6295 data 1	 W 	0c0000-0c0003	<				<
6295 stat 1	R 	0c0000-0c0001	<				<
RAM			RW	0f0000-0f3fff	0e0000-0effff?	<
-------------------------------------------------------------

----------------------- Alien Command -----------------------

interrupt:	1] 			4f0: rte		2]			44a: rte
			3]			484:			4,5,6,7]	4f0: rte

ram check:	b8000-bffff	objectram?		a0000-a3fff	bg1?
			b0000-b3fff	bg3?			f8000-f8fff	palette?

f0064->82000	f006c->82100
f0068->82002	f0070->82102
f0060->82004	f0062->82104	82208<-0	watchdog

9a8		print string: (a7)->string=x.w,y.w,attr.w,(char.b)*,0
		dest = b0000, line = 40

------------------------- Rodland ---------------------------

interrupt:	1] 			418->3864: rts
			2]			420: move.w #-1,f0010; jsr 3866
			3]			430: rte		4,5,6,7]	434: rte

213da	print test error (20c12 = string address 0-4)

f0018->84200	f0020->84208	f0028->84008
f001c->84202	f0024->8420a	f002c->8400a
f0012->84204	f0014->8420c	f0016->8400c

7fe		d0.w -> 84000.w & f000e.w
81a		d0/d1/d2 & $D -> 84204 / 8420c /8400c

----------------------- Avenging Spirit ---------------------

interrupt:	1] 				rte
			2,3, 5,6,7]		move.w  $e0000.l, $78e9e.l
							andi.w  #$ff, $78e9e.l
			4] 78b20 software watchdog (78ea0 enables it)

fd6		reads e0000 (values FF,06,34,35,36,37)
ffa		e0000<-6 test

******** Level: 79584.w ************

-------------------------- 64th Street ----------------------

interrupt:	1] 10eac:	disabled while b6c4=6 (10fb6 test)
						if (8b1c)	8b1c<-0
							color cycle scroll 4
							copies 800 bytes 98da->8008

			2] 10f28:	switch b6c4
						0	RTE
						2	10f44: M[b6c2]<-d8000. b6c4<-4
						4	10f6c: next b6c2 & d8000. if (b6c2>A)
									b6c2,4<-0	else	b6c4<-2
						6	10f82: b6c6<-(d8001) b6c7<-FF (test)
			3] RTE

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

8b1e.w	incremented by int4, when >= b TRAP#E (software watchdog error)

*******		ff9df8.w	level *******

------------------------------- EDF -------------------------

interrupt:	1] rte			7] rte
			2,3]	543C:	move.w  $e0000.l, $60da6.l
							move.w  #$ffff, $60da8.l
			4,5,6]	5928 + move.w  #$ffff, $60010.l

616f4.w		lives
********* 60d8a.w	level(1..) ********

89e			(a7)+ -> 44000.w & 6000e.w
8cc			(a7)+ -> 44204.w ; 4420c.w ; 4400c.w
fc0			(a7)+ -> 58000 (string)

if t(x) = ((x^2)>>4)&0xff and e0000(x) is the magic function,
test at 554c wants t(t(e0000(0xf0))) == e0000(6)

---------------------------- Priorities ---------------------

p0 = scroll 1	p1 = scroll 2	p2 = scroll3	s = all sprites
s0 = sprites with color 0-3		s1 = sprites with color 4-7
s2 = sprites with color 8-b		s3 = sprites with color c-f

order is : below -> over

Priorities for:					P47j (MS1-A)
Flags: txt always 0010, fg 0000, others 0 (unless otherwise stated).
P47j does not exploit low and hi sprite priorities at all, during gameplay.

				enab bg   notes/what's the correct sprites-fg pos
t.sprite test 1	000e 001c             	txt:0013
t.sprite test 2	000f 001c *84100:0101*	txt:0013 p1=column
-.copyright scr	000f 0002 *84300:0010*	txt:0012
-.game logo		010c dis.				txt:0013
-.hi scores		010c dis. *sprite should go above txt*		txt:0012
0.lev0			020f 0000 p0 s  p1
1.clouds		000f 0002 p0 p1 s
2.north africa	020f 0002 p0 s  p1
3.ship			020f 0000 p0 s  p1
4.ardennes		000f 0001 p0 p1 s
5.desert		020f 0001 p0 s  p1
6.desert+sea	020f 0002 p0 s  p1
7.sea+snow		020f 0000 p0 s  p1
[Repeats?]


Priorities for:				64th Street (MS1-C)
Flags: txt always 0011, fg 0001, others 0 (unless otherwise stated).

        enab bg   notes/not empty screens/what's the correct sprites-fg pos
-.intro 000f 0002 - no p1	bg 0123---- sx++, sy=0
0.lev 1 010f 0001 p0 s  p1	bg 01234--- max sx 3f0	fg 01234567 max sx 5e8
1.lev 2 010f 0001 p0 s  p1	bg 012----- max sx 1e2	fg 012----- max sx 1e2
2.train 030f 0000 p0 p1 s 	bg 01234567 01234567 sx++ sy = 0 fg 01------ 01------ 01------ 01------ max sx 100. sy 0/100
3.port  050f 0001 2200=0100 bg: 0123---- 0123---- 0123---- 0123---- sx ? sy 0/1/2/300	fg: 012345-- 012345--sx ? sy 0/100
				  p0 s2/3 p1 s0/1	(player0, men0&1, splash3, fallen2&3)
4.boat1	010f 0001 p0 s  p1
5.boat2 030f 0001 p0 p1 s
6.boat3	010f 0001 - no p1
7.rail  050f 0001 2200=0100
				  p0 s2/3 p1 s0/1	(player0, men0&1, box+pieces1, brokenbox3, fallen3)
8.tunne	030f 0001 p0 p1 s
9.facto 010f 0001 p0 s  p1
a.fact2 020f 0001 p1 p0 s	*bg&fg reversed*
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
0.city			000f	p1 p0 s
1.snakes		0c0f	44100=100
						s2 p1 s0/3 p0	(player3, fire2, snake+laser0)
2.factory		020f	p1 s  p0
3.lift boss		010f	p0 p1 s
4.city+lifts	010f	p0 p1 s
5.snake boss	000f	p1 p0 s
6.sewers		030f	p0 s  p1
7.end of sewers	030f	p0 s  p1
8.boss			0e0f	44100=100
						p1 s2/3 p0 s0  (player3, boss+expl+fire2, splash0)
9.road			000f	p1 p0 s
a.factory		020f	p1 s  p0
b.boss(flying)	000f	-- p0 s
c.airport win	000f	p1 p0 s
	door		030f	p0 s  p1 (ghost=s1 above all?)
d.final level	000f	p1 p0 s

0000	s3p1s2p0s1p2s0
s0 over 	s1 over p0	s2 over p1	s3 over p0
0			1			0			1			   p0 s2/s3  p1 s0/s1	C
1			1			0			0			s2 p1 s3/s0  p0 s1? 	B
1			1			1			0			   p1 s2/s3  p0 s0 s1?	B
0			0			0			0			   p0? s2/s3 p1 s0/s1	A

Priorities for:					edf (MS1-B)
Flags: txt always 0010, fg 0000, others 0 (unless otherwise stated).
edf does not exploit low and hi sprite priorities at all, during gameplay.

			enab bg   notes/what's the correct sprites-fg pos

-.intro scr	020f b0 f0 t12	p0 s  p1 -notxt
-.edf logo	010f b0 f0 t12	p0 s? p2 s? p1	!!!
-.weapon0  	020f b0 f0 t12	p0 s  p1 p2
-.weapon1  	000f b0 f0 t12	p0 p1 s  p2
1.lev1		020f b0 f0 t12	p0 p1 s  p2
			000f b0 f0 t12	p0 s  p1 p2

4.sea		000f b0 f0 t12	p0 p1 s  p2
5.rtype		000f b0 f0 t12	p0 p1 s  p2
6.space		000f b0 f0 t12	p0 p1 s  p2

------------------------------ P - 47 -----------------------

initial: 	PC = 400	SP = fff00
interrupts:	2] 540		1/3-7] 758 RTE

517a		print word string: (a6)+,(a5)+$40. FFFF ends
5dbc		print string(s) to (a1)+$40: a6-> len.b,x.b,y.b,(chars.b)*
726a		prints screen
7300		ram test
7558		ip test
75e6(7638 loop)	sound test
	84300.w		<-f1002.w	?portrait F/F on(0x0100)/off(0x0000)
	84308.w		<-f1004.w	sound code

7736(7eb4 loop)	scroll 1 test

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

f0104.w		initial lives
f002a/116.w	<-!80000
f0810.w		<-!80002
f0c00.w		<-!80004
*******		f0018.w	level *******

								To Do
								-----

- Support encrypted games
- Understand how priorities *really* work
- Understand an handful of unknown bits in video regs
- Flip Screen support

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* videohrdw has access to these */
unsigned char *scrollram[3],*scrollram_dirty[3];		// memory pointers
unsigned char *objectram, *videoregs, *ram;
int scrollx[3],scrolly[3],scrollflag[3],nx[3],ny[3];	// video registers
int active_layers, spritebank, screenflag, spriteflag, hardware_type;
int bg, fg, txt;

/* videohrdw variables */
extern struct osd_bitmap *scroll_bitmap[3];

/* videohrdw functions */
int  megasys1_vh_start(void);
void megasys1_vh_stop(void);
void megasys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void mark_dirty_all(void);
void mark_dirty(int n);

int ip_select;
struct GameDriver avspirit_driver;


#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

void megasys1_init_machine(void)
{
int i;

			return;

/* reset global variables to zero */

	for (i = 0; i < 3; i++)
	{
		scrollx[i]    = 0;
		scrolly[i]    = 0;
		scrollflag[i] = 0;
	}

	ip_select = 0;
	active_layers = 0;
	spritebank = 0;
	screenflag = 0;
	spriteflag = 0;
}


/*
We have bit scrambling on words:

fedc ba98 7654 3210		encry
d0a9 6ebf 5c72 3814     clear

142a	20ca clear
d011	4541 clear correct
		*2= 8a82 ^ 0248 = 8aca

0248

encry	clear
0008	0008	bit 3(clear) is bit 3(encry)
0002	0002	bit 1(clear) is bit 1(encry)
4000	0400	bit a(clear) is bit e(encry)
ffef	fffe	bit 0(clear) is bit 4(encry)
0400	2000	bit d(clear) is bit a(encry)
0480	2020	bit 5(clear) is bit 7(encry)
6000	8400	bit f(clear) is bit d(encry)
011a	000f	bit 2(clear) is bit 8(encry)
5100	0444	bit 6(clear) is bit c(encry)
4184	0434	bit 4(clear) is bit 2(encry)
7fff	feff	bit 8(clear) is bit f(encry)
b1be	01ff	bit 7(clear) is bit 5(encry)
8204	1110	bit c(clear) is bit 9(encry)
5007	4452	bit e(clear) is bit 0(encry)

But... on addresses t: t|0x0248==t, it's different

*/
static void rom_decode(void)
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
		y = 0;

		y |= (x & (1 << 0x4))?(1<<0x0):0;
		y |= (x & (1 << 0x1))?(1<<0x1):0;
		y |= (x & (1 << 0x8))?(1<<0x2):0;
		y |= (x & (1 << 0x3))?(1<<0x3):0;
		y |= (x & (1 << 0x2))?(1<<0x4):0;
		y |= (x & (1 << 0x7))?(1<<0x5):0;
		y |= (x & (1 << 0xc))?(1<<0x6):0;
		y |= (x & (1 << 0x5))?(1<<0x7):0;
		y |= (x & (1 << 0xf))?(1<<0x8):0;
		y |= (x & (1 << 0xb))?(1<<0x9):0;
		y |= (x & (1 << 0xe))?(1<<0xa):0;
		y |= (x & (1 << 0x6))?(1<<0xb):0;
		y |= (x & (1 << 0x9))?(1<<0xc):0;
		y |= (x & (1 << 0xa))?(1<<0xd):0;
		y |= (x & (1 << 0x0))?(1<<0xe):0;
		y |= (x & (1 << 0xd))?(1<<0xf):0;

		WRITE_WORD(&ROM[i],y);
	}

}





/*
**
**	Main cpu data
**
**
*/


/******************** System A ***********************/


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


#define interrupt_num_A		2
int interrupt_A(void)		{ if (cpu_getiloops()%2) return 1; else return 2; }
int sound_interrupt_A(void)	{return 4;} /* stdragon */

#define videoreg_sx(_n_) 	{scrollx[_n_] = -new_data;} break;
#define videoreg_sy(_n_) 	{scrolly[_n_] = -new_data;} break;
#define videoreg_flag(_n_)\
{\
	scrollflag[_n_] = new_data;\
	if (new_data != old_data)\
	{	if (scroll_bitmap[_n_])\
		{\
			osd_free_bitmap(scroll_bitmap[_n_]);\
			scroll_bitmap[_n_] = 0;\
			mark_dirty(_n_);\
/*			if (errorlog) fprintf(errorlog, "freeing scroll %d\n",_n_);*/ \
		}\
	}\
} break;


void videoregs_A_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&videoregs[offset]);
	COMBINE_WORD_MEM(&videoregs[offset],data);
	new_data  = READ_WORD(&videoregs[offset]);

	switch (offset)
	{
		case 0x000   : {active_layers = new_data;} break;
		case 0x008+0 : videoreg_sx(2)
		case 0x008+2 : videoreg_sy(2)
		case 0x008+4 : videoreg_flag(2)
		case 0x200+0 : videoreg_sx(0)
		case 0x200+2 : videoreg_sy(0)
		case 0x200+4 : videoreg_flag(0)
		case 0x208+0 : videoreg_sx(1)
		case 0x208+2 : videoreg_sy(1)
		case 0x208+4 : videoreg_flag(1)
		case 0x100   : {spriteflag = new_data;} break;
		case 0x300   : {screenflag = new_data;} break;
		case 0x308   : {soundlatch_w(0,new_data);

/* the only game checking if the  sound cpu echoes the sound  command
   is Rodland. If the check fails,  the game is just slowed down.  So
   to allow diabling the sound cpu without the slow down, we echo the
   sound command here */
						soundlatch2_w(0,new_data);} break;

		default: {if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, videoreg %04X <- %04X\n",cpu_get_pc(),offset,data);}
	}
}



#define scrollram_handler(_n_)\
void scrollram_##_n_##_w(int offset, int data)\
{\
int old_data;\
\
	old_data = READ_WORD(&scrollram[_n_][offset]);\
	COMBINE_WORD_MEM(&scrollram[_n_][offset],data);\
\
	if ( old_data != READ_WORD(&scrollram[_n_][offset]) )\
		scrollram_dirty[_n_][offset/2] = 1;\
}\

scrollram_handler(0)
scrollram_handler(1)
scrollram_handler(2)




int coins_r(int offset)   {return input_port_0_r(0);}	// < 00 | Coins >
int player1_r(int offset) {return input_port_1_r(0);}	// < 00 | Player 1 >
int player2_r(int offset) {return (input_port_2_r(0)<<8)+input_port_3_r(0);}	// < Reserve | Player 2 >
int dipsw_r(int offset)	  {return (input_port_4_r(0)<<8)+input_port_5_r(0);}	// < DSW 1 | DSW 2 >




static struct MemoryReadAddress readmem_A[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM }, /* P47 has half that */
	{ 0x0f0000, 0x0fffff, MRA_BANK2 },
	{ 0x080000, 0x080001, coins_r },
	{ 0x080002, 0x080003, player1_r },
	{ 0x080004, 0x080005, player2_r },
	{ 0x080006, 0x080007, dipsw_r },
	{ 0x080008, 0x080009, soundlatch2_r },	/* Echo from sound cpu */
	{ 0x084000, 0x084fff, MRA_BANK3 },		/* Video Regist. */
	{ 0x088000, 0x0887ff, paletteram_word_r },
	{ 0x08e000, 0x08ffff, MRA_BANK4 },		/* Object  RAM   */
	{ 0x090000, 0x093fff, MRA_BANK5 },		/* Scroll RAM 1  */
	{ 0x094000, 0x097fff, MRA_BANK6 },		/* Scroll RAM 2  */
	{ 0x098000, 0x09bfff, MRA_BANK7 },		/* Scroll RAM 3  */
	{ -1 }
};

static struct MemoryWriteAddress writemem_A[] =
{
 	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x0f0000, 0x0fffff, MWA_BANK2, &ram },
	{ 0x084000, 0x0843ff, videoregs_A_w, &videoregs },
	{ 0x088000, 0x0887ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram },
	{ 0x08e000, 0x08ffff, MWA_BANK4, &objectram },
	{ 0x090000, 0x093fff, scrollram_0_w, &scrollram[0] },
	{ 0x094000, 0x097fff, scrollram_1_w, &scrollram[1] },
	{ 0x098000, 0x09bfff, scrollram_2_w, &scrollram[2] },
	{ -1 }
};



/******************** System C ***********************/




#define interrupt_num_C 30
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

int sound_interrupt_C(void)	{return 4;}


void videoregs_C_w(int offset, int data)
{
int old_data, new_data;

	old_data = READ_WORD(&videoregs[offset]);
	COMBINE_WORD_MEM(&videoregs[offset],data);
	new_data  = READ_WORD(&videoregs[offset]);

	switch (offset)
	{
		case 0x2208   : {active_layers = new_data;} break;
		case 0x2100+0 : videoreg_sx(2)
		case 0x2100+2 : videoreg_sy(2)
		case 0x2100+4 : videoreg_flag(2)
		case 0x2000+0 : videoreg_sx(0)
		case 0x2000+2 : videoreg_sy(0)
		case 0x2000+4 : videoreg_flag(0)
		case 0x2008+0 : videoreg_sx(1)
		case 0x2008+2 : videoreg_sy(1)
		case 0x2008+4 : videoreg_flag(1)
		case 0x2108   : {spritebank = new_data;} break;
		case 0x2200   : {spriteflag = new_data;} break;
		case 0x2308   : {screenflag = new_data;} break;
		case 0x8000   : {soundlatch_w(0,new_data);} break;
		default: {if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, videoreg %04X <- %04X\n",cpu_get_pc(),offset,data);}
	}
}




/* Read the input ports, through a protection device */
int ip_select_r(int offset)
{
//	S   P1  P2  DA  DB
// 	20, 21, 22, 23, 24	edf
//	37, 35, 36, 33, 34	avspirit
//	57, 53, 54, 55, 56	64street

	/* f(x) = ((x*x)>>4)&0xFF ; f(f($D)) == 6 */
	if ((ip_select & 0xF0) == 0xF0) return 0x0D;

	switch (ip_select & 0xFF)
	{
			case 0x20 :
			case 0x37 :
			case 0x57 :	return coins_r(0); break;

			case 0x21 :
			case 0x35 :
			case 0x53 : return player1_r(0); break;

			case 0x22 :
			case 0x36 :
			case 0x54 : return player2_r(0); break;

			case 0x23 :
			case 0x33 :
			case 0x55 : return ((dipsw_r(0)&0xFF00)>>8); break;

			case 0x24 :
			case 0x34 :
			case 0x56 : return ((dipsw_r(0)&0x00FF)>>0); break;

			default   : return 0x06;
	}
}

void ip_select_w(int offset,int data)
{
	ip_select = COMBINE_WORD(ip_select,data);

	cpu_cause_interrupt(0,3);	/* EDF needs it */
}





static struct MemoryReadAddress readmem_C[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0xff0000, 0xffffff, MRA_BANK2 },
	{ 0x080000, 0x080001, coins_r },
	{ 0x080002, 0x080003, player1_r },
	{ 0x080004, 0x080005, player2_r },
	{ 0x080006, 0x080007, dipsw_r },
	{ 0x0c0000, 0x0cffff, MRA_BANK3 },		/* Video Regist. */
	{ 0x0d2000, 0x0d3fff, MRA_BANK4 },		/* Object  RAM   */
	{ 0x0e0000, 0x0e3fff, MRA_BANK5 },		/* Scroll RAM 1  */
	{ 0x0e8000, 0x0ebfff, MRA_BANK6 },		/* Scroll RAM 2  */
	{ 0x0f0000, 0x0f3fff, MRA_BANK7 },		/* Scroll RAM 3  */
	{ 0x0f8000, 0x0f87ff, paletteram_word_r },
	{ 0x0d8000, 0x0d8001, ip_select_r },
	{ -1 }
};

static struct MemoryWriteAddress writemem_C[] =
{
 	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0xff0000, 0xffffff, MWA_BANK2, &ram },
	{ 0x0c0000, 0x0cffff, videoregs_C_w, &videoregs },
	{ 0x0d2000, 0x0d3fff, MWA_BANK4, &objectram },
	{ 0x0e0000, 0x0e3fff, scrollram_0_w, &scrollram[0] },
	{ 0x0e8000, 0x0ebfff, scrollram_1_w, &scrollram[1] },
	{ 0x0f0000, 0x0f3fff, scrollram_2_w, &scrollram[2] },
	{ 0x0f8000, 0x0f87ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram },
	{ 0x0d8000, 0x0d8001, ip_select_w },
	{ -1 }
};


/******************** System B ***********************/


#define interrupt_num_B	interrupt_num_C
#define interrupt_B		interrupt_C


static struct MemoryReadAddress readmem_B[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x0bffff, MRA_ROM },
	{ 0x060000, 0x07ffff, MRA_BANK2 },
	{ 0x044000, 0x044fff, MRA_BANK3 },		/* Video Regist. */
	{ 0x048000, 0x0487ff, paletteram_word_r },
	{ 0x04e000, 0x04ffff, MRA_BANK4 },		/* Object  RAM   */
	{ 0x050000, 0x053fff, MRA_BANK5 },		/* Scroll RAM 1  */
	{ 0x054000, 0x057fff, MRA_BANK6 },		/* Scroll RAM 2  */
	{ 0x058000, 0x05bfff, MRA_BANK7 },		/* Scroll RAM 3  */
	{ 0x0e0000, 0x0e0001, ip_select_r },
	{ -1 }
};

static struct MemoryWriteAddress writemem_B[] =
{
 	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x0bffff, MWA_ROM },
	{ 0x060000, 0x07ffff, MWA_BANK2, &ram },
	{ 0x044000, 0x0443ff, videoregs_A_w, &videoregs },
	{ 0x048000, 0x0487ff, paletteram_RRRRGGGGBBBBRGBx_word_w, &paletteram },
	{ 0x04e000, 0x04ffff, MWA_BANK4, &objectram },
	{ 0x050000, 0x053fff, scrollram_0_w	, &scrollram[0] },
	{ 0x054000, 0x057fff, scrollram_1_w	, &scrollram[1] },
	{ 0x058000, 0x05bfff, scrollram_2_w, &scrollram[2] },
	{ 0x0e0000, 0x0e0001, ip_select_w },
	{ -1 }
};






/*
**
**	Sound cpu data
**
**
*/


/******************** System A ***********************/


static struct MemoryReadAddress sound_readmem_A[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x040000, 0x040001, soundlatch_r },
	{ 0x080002, 0x080003, YM2151_status_port_0_r },
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_r },
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_r },
	{ 0x0e0000, 0x0fffff, MRA_BANK1 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem_A[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x060000, 0x060001, soundlatch2_w },	/* Echo to main cpu */
	{ 0x080000, 0x080001, YM2151_register_port_0_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_w},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_w },
	{ 0x0e0000, 0x0fffff, MWA_BANK1 },
	{ -1 }
};





/******************** System B / C ***********************/





static struct MemoryReadAddress sound_readmem_C[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },		/* MS1-C has half that */
	{ 0x040000, 0x040001, soundlatch_r },	/* MS1-B */
	{ 0x060000, 0x060001, soundlatch_r },
	{ 0x080002, 0x080003, YM2151_status_port_0_r },
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_r },
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_r },
	{ 0x0e0000, 0x0effff, MRA_BANK1 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem_C[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },		/* MS1-C has half that */
	{ 0x040000, 0x040001, MWA_NOP },		/* ? */
	{ 0x060000, 0x060001, MWA_NOP },		/* sound command received ? */
	{ 0x080000, 0x080001, YM2151_register_port_0_w },
	{ 0x080002, 0x080003, YM2151_data_port_0_w},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_w },
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_w },
	{ 0x0e0000, 0x0effff, MWA_BANK1 },
	{ -1 }
};

#define sound_writemem_B	sound_writemem_C
#define sound_readmem_B		sound_readmem_C
#define sound_interrupt_B	sound_interrupt_C


/******************** Z80 ***********************/



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



/* IN0 - COINS */
#define SERVICE \
	PORT_START\
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )\
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

/* IN2 - Reserve */
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
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )\
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )\

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
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\

/* IN5 - DSW2 */
#define UNKNOWN \
	PORT_START\
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


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

layout8x8(  tilelayout1, 0x10000*1)
layout8x8(  charlayout,	 0x20000*1)	/* P-47 has half that chars */
layout8x8(  tilelayout,	 0x20000*4)
layout16x16(spritelayout_Z,0x20000*1)
layout16x16(spritelayout_A,0x20000*4)
layout16x16(spritelayout_C,0x20000*8)

static struct GfxDecodeInfo gfxdecodeinfo_Z[] =
{
	{ 1, 0x000000, &charlayout,		256*0, 16 },	// [0] scroll 1
	{ 1, 0x020000, &tilelayout1,	256*2, 16 },	// [2] scroll 2
	{ 1, 0x030000, &spritelayout_Z,	256*1, 16 },	// [3] sprites
	{ -1 }
};
static struct GfxDecodeInfo gfxdecodeinfo_A[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] scroll 1
	{ 1, 0x080000, &tilelayout,		256*1, 16 },	// [1] scroll 2
	{ 1, 0x100000, &charlayout,		256*2, 16 },	// [2] scroll 3
	{ 1, 0x120000, &spritelayout_A,	256*3, 16 },	// [3] sprites
	{ -1 }
};

#define gfxdecodeinfo_B		gfxdecodeinfo_A

static struct GfxDecodeInfo gfxdecodeinfo_C[] =
{
	{ 1, 0x000000, &tilelayout,		256*0, 16 },	// [0] scroll 1
	{ 1, 0x080000, &tilelayout,		256*1, 16 },	// [1] scroll 2
	{ 1, 0x100000, &charlayout,		256*2, 16 },	// [2] scroll 3
	{ 1, 0x120000, &spritelayout_C,	256*3, 16 },	// [3] sprites
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
	1,
	3500000,	/* ?? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 30000, 30000 },	/* ?? */
	{ 3, 4 },
	{ 50, 50 }
};

#define MEGASYS1_MACHINE(_type_) \
static struct MachineDriver machine_driver_##_type_ = \
{\
	{\
		{\
			CPU_M68000,\
			7000000,	/* ?? */ \
			0,\
			readmem_##_type_,writemem_##_type_,0,0,\
			interrupt_##_type_,interrupt_num_##_type_ \
		},\
		{\
			CPU_M68000 | CPU_AUDIO_CPU,\
			7000000,	/* ?? */ \
			2,\
			sound_readmem_##_type_,sound_writemem_##_type_,0,0,\
			sound_interrupt_##_type_,1\
		},\
	},\
	60,DEFAULT_60HZ_VBLANK_DURATION,\
	10,\
	megasys1_init_machine,\
	/* video hardware */ \
	256, 256,{ 0, 256-1, 0+16, 256-16-1 },\
	gfxdecodeinfo_##_type_,\
	1024, 1024,\
	0,\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\
	0,\
	megasys1_vh_start,\
	megasys1_vh_stop,\
	megasys1_vh_screenrefresh,\
	/* sound hardware */ \
	0,0,0,0,\
	{\
		{\
			SOUND_YM2151,\
			&ym2151_interface\
		},\
		{\
			SOUND_OKIM6295,\
			&okim6295_interface\
		}\
	}\
};


MEGASYS1_MACHINE(A)
MEGASYS1_MACHINE(B)
MEGASYS1_MACHINE(C)

void irq_handler(int irq)
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
	10,\
	0,\
	/* video hardware */ \
	256, 256,{ 0, 256-1, 0+16, 256-16-1 },\
	gfxdecodeinfo_Z,\
	256*3, 256*3,\
	0,\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\
	0,\
	megasys1_vh_start,\
	megasys1_vh_stop,\
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


void driver_init_Z(void) {hardware_type = 'Z' ; spriteram = &ram[0x8000];}
void driver_init_A(void) {hardware_type = 'A' ; spriteram = &ram[0x8000];}

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

	/* avspirit memory starts at 70000, not 60000 */
	if (Machine->gamedrv == &avspirit_driver)	spriteram = &ram[0x18000];
	else										spriteram = &ram[0x08000];
}

void driver_init_C(void) {hardware_type = 'C' ; spriteram = &ram[0x8000];}


#define MEGASYS1_CREDITS "Luca Elia\n"

#define MEGASYS1_GAMEDRIVER(_shortname_,_clone_driver_,_fullname_,_year_,_type_,_rom_decode_) \
struct GameDriver _shortname_##_driver =\
{\
	__FILE__,\
	_clone_driver_,\
	#_shortname_,\
	#_fullname_,\
	#_year_,\
	"Jaleco",\
	MEGASYS1_CREDITS,\
	0,\
	&machine_driver_##_type_,\
	&driver_init_##_type_,\
	_shortname_##_rom,\
	_rom_decode_, 0,\
	0,\
	0,\
	input_ports_##_shortname_,\
	0, 0, 0,\
	ORIENTATION_DEFAULT,\
	0,0\
};

/***************************************************************************

  Game driver(s)

***************************************************************************/


/***************************************************************************

  64th Street

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
	SERVICE
	JOY(0)
	RESERVE				// Unused
	JOY(IPF_PLAYER2)
	COINAGE_C
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
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

void street64_patch(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

//	WRITE_WORD (&RAM[0x006b8],0x6004); /* d8001 test */
	WRITE_WORD (&RAM[0x10EDE],0x6012); /* watchdog   */
}


struct GameDriver street64_driver =\
{\
	__FILE__,\
	0,\
	"64street",\
	"64th. Street - A Detective Story (World?)",\
	"1991",\
	"Jaleco",\
	MEGASYS1_CREDITS,\
	0,\
	&machine_driver_C,\
	&driver_init_C,\
	street64_rom,\
	street64_patch, 0,\
	0,\
	0,\
	input_ports_street64,\
	0, 0, 0,\
	ORIENTATION_DEFAULT,\
	0,0\
};

struct GameDriver streej64_driver =\
{\
	__FILE__,\
	&street64_driver,\
	"64streej",\
	"64th. Street - A Detective Story (Japan)",\
	"1991",\
	"Jaleco",\
	MEGASYS1_CREDITS,\
	0,\
	&machine_driver_C,\
	&driver_init_C,\
	streej64_rom,\
	street64_patch, 0,\
	0,\
	0,\
	input_ports_street64,\
	0, 0, 0,\
	ORIENTATION_DEFAULT,\
	0,0\
};


/***************************************************************************

  Astyanax

***************************************************************************/

ROM_START( astyanax_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "astyan2.bin", 0x000000, 0x020000, 0x1b598dcc )
	ROM_LOAD_ODD(  "astyan1.bin", 0x000000, 0x020000, 0x1a1ad3cf )
//	ROM_LOAD( "astyan3.bin", 0x040000, 0x010000, 0x1 )
//	ROM_LOAD( "astyan4.bin", 0x050000, 0x010000, 0x1 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "astyan11.bin", 0x000000, 0x020000, 0x5593fec9 ) /* Tiles (scroll 1) */
	ROM_LOAD( "astyan12.bin", 0x020000, 0x020000, 0xe8b313ec )
	ROM_LOAD( "astyan13.bin", 0x040000, 0x020000, 0x5f3496c6 )
	ROM_LOAD( "astyan14.bin", 0x060000, 0x020000, 0x29a09ec2 )
	ROM_LOAD( "astyan15.bin", 0x080000, 0x020000, 0x0d316615 ) /* Tiles (scroll 2) */
	ROM_LOAD( "astyan16.bin", 0x0a0000, 0x020000, 0xba96e8d9 )
	ROM_LOAD( "astyan17.bin", 0x0c0000, 0x020000, 0xbe60ba06 )
	ROM_LOAD( "astyan18.bin", 0x0e0000, 0x020000, 0x3668da3d )
	ROM_LOAD( "astyan19.bin", 0x100000, 0x020000, 0x98158623 ) /* Text  (scroll 3) */
	ROM_LOAD( "astyan20.bin", 0x120000, 0x020000, 0xc1ad9aa0 ) /* Sprites 16x16    */
	ROM_LOAD( "astyan21.bin", 0x140000, 0x020000, 0x0bf498ee )
	ROM_LOAD( "astyan22.bin", 0x160000, 0x020000, 0x5f04d9b1 )
	ROM_LOAD( "astyan23.bin", 0x180000, 0x020000, 0x7bd4d1e7 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "astyan5.bin",  0x000000, 0x010000, 0x11c74045 )
	ROM_LOAD_ODD(  "astyan6.bin",  0x000000, 0x010000, 0xeecd4b16 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "astyan7.bin",  0x000000, 0x020000, 0x319418cc )
	ROM_LOAD( "astyan8.bin",  0x020000, 0x020000, 0x5e5d2a22 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "astyan9.bin",  0x000000, 0x020000, 0xa10b3f17 )
	ROM_LOAD( "astyan10.bin", 0x000000, 0x020000, 0x4f704e7a )

ROM_END

INPUT_PORTS_START( input_ports_astyanax )
	SERVICE				/* IN0 0x80001.b */
	JOY(0)				/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	UNKNOWN				/* IN5 0x80007.b */
INPUT_PORTS_END


MEGASYS1_GAMEDRIVER(astyanax,0,The Astyanax,1988,A,rom_decode)



/***************************************************************************

  Avenging Spirit

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
	SERVICE \
	JOY(0) \
	RESERVE \
	JOY(IPF_PLAYER2) \
	COINAGE_C \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( No ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) ) \
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x08, "Easy" ) \
	PORT_DIPSETTING(    0x18, "Normal" ) \
	PORT_DIPSETTING(    0x10, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )\

INPUT_PORTS_START( input_ports_avspirit )
	INPUT_PORTS_AVSPIRIT
INPUT_PORTS_END


MEGASYS1_GAMEDRIVER(avspirit,0,Avenging Spirit,1991,B,0)


/***************************************************************************

  Earth Defense Force

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
	SERVICE
	JOY(0)
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


MEGASYS1_GAMEDRIVER(edf,0,Earth Defense Force,1991,B,0)



/***************************************************************************

  Hachoo

***************************************************************************/

ROM_START( hachoo_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "hacho02.rom", 0x000000, 0x020000, 0x49489c27 )
	ROM_LOAD_ODD(  "hacho01.rom", 0x000000, 0x020000, 0x97fc9515 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "hacho14.rom", 0x000000, 0x080000, 0x10188483 ) /* Tiles (scroll 2) */
	ROM_LOAD( "hacho15.rom", 0x080000, 0x020000, 0xe559347e )
	ROM_LOAD( "hacho16.rom", 0x0a0000, 0x020000, 0x105fd8b5 )
	ROM_LOAD( "hacho17.rom", 0x0c0000, 0x020000, 0x77f46174 )
	ROM_LOAD( "hacho19.rom", 0x100000, 0x020000, 0x33bc9de3 ) /* Text  (scroll 3) */
	ROM_LOAD( "hacho20.rom", 0x120000, 0x020000, 0x2ae2011e ) /* Sprites 16x16 */
	ROM_LOAD( "hacho21.rom", 0x140000, 0x020000, 0x6dcfb8d5 )
	ROM_LOAD( "hacho22.rom", 0x160000, 0x020000, 0xccabf0e0 )
	ROM_LOAD( "hacho23.rom", 0x180000, 0x020000, 0xff5f77aa )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "hacho05.rom", 0x000000, 0x010000, 0x6271f74f )
	ROM_LOAD_ODD(  "hacho06.rom", 0x000000, 0x010000, 0xdb9e743c )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "hacho07.rom", 0x000000, 0x020000, 0x06e6ca7f )
	ROM_LOAD( "hacho08.rom", 0x020000, 0x020000, 0x888a6df1 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "hacho09.rom", 0x000000, 0x020000, 0xe9f35c90 )
	ROM_LOAD( "hacho10.rom", 0x020000, 0x020000, 0x1aeaa188 )

ROM_END

INPUT_PORTS_START( input_ports_hachoo )
	SERVICE				/* IN0 0x80001.b */
	JOY(0)				/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	UNKNOWN				/* IN5 0x80007.b */
INPUT_PORTS_END


MEGASYS1_GAMEDRIVER(hachoo,0,Hachoo,1988,A,rom_decode)



/***************************************************************************

  Legend of Makaj

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
	SERVICE				/* IN0 0x80001.b */
	JOY(0)				/* IN1 0x80003.b */
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


MEGASYS1_GAMEDRIVER(lomakaj,0,Legend of Makai,1988,Z,0)

/***************************************************************************

  P - 47 (World)

***************************************************************************/

ROM_START( p47_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "p47us3.bin", 0x000000, 0x020000, 0x022e58b8 )
	ROM_LOAD_ODD(  "p47us1.bin", 0x000000, 0x020000, 0xed926bd8 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c ) /* Tiles (scroll 1) */
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )
	ROM_LOAD( "p47j_23.bin", 0x080000, 0x020000, 0x6e9bc864 ) /* Tiles (scroll 2) */
	ROM_RELOAD(              0x0a0000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x0c0000, 0x020000, 0x5268395f )
	ROM_LOAD( "p47us16.bin", 0x100000, 0x010000, 0x5a682c8f ) /* Text  (scroll 3) */
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

#define INPUT_PORTS_P47 \
	SERVICE				/* IN0 0x80001.b */ \
	JOY(0)				/* IN1 0x80003.b */ \
	RESERVE				/* IN2 0x80004.b */ \
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */ \
	COINAGE_A			/* IN4 0x80006.b */ \
	PORT_START			/* IN5 0x80007.b */ \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) \
	PORT_DIPSETTING(    0x02, "2" ) \
	PORT_DIPSETTING(    0x03, "3" ) \
	PORT_DIPSETTING(    0x01, "4" ) \
	PORT_DIPSETTING(    0x00, "5" ) \
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x00, "Easy" ) \
	PORT_DIPSETTING(    0x30, "Normal" ) \
	PORT_DIPSETTING(    0x20, "Hard" ) \
	PORT_DIPSETTING(    0x10, "Hardest" ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_START( input_ports_p47 )
	INPUT_PORTS_P47
INPUT_PORTS_END


MEGASYS1_GAMEDRIVER(p47,0,P-47 - The Phantom Fighter (World),1988,A,0)

/***************************************************************************

  P - 47 (Japan)

***************************************************************************/

ROM_START( p47j_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "p47j_3.bin", 0x000000, 0x020000, 0x11c655e5 )
	ROM_LOAD_ODD(  "p47j_1.bin", 0x000000, 0x020000, 0x0a5998de )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "p47j_5.bin",  0x000000, 0x020000, 0xfe65b65c ) /* Tiles (scroll 1) */
	ROM_LOAD( "p47j_6.bin",  0x020000, 0x020000, 0xe191d2d2 )
	ROM_LOAD( "p47j_7.bin",  0x040000, 0x020000, 0xf77723b7 )
	ROM_LOAD( "p47j_23.bin", 0x080000, 0x020000, 0x6e9bc864 ) /* Tiles (scroll 2) */
	ROM_RELOAD(              0x0a0000, 0x020000 )	/* why? */
	ROM_LOAD( "p47j_12.bin", 0x0c0000, 0x020000, 0x5268395f )
	ROM_LOAD( "p47j_16.bin", 0x100000, 0x010000, 0x30e44375 ) /* Text  (scroll 3) */
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

INPUT_PORTS_START( input_ports_p47j )
	INPUT_PORTS_P47
INPUT_PORTS_END


MEGASYS1_GAMEDRIVER(p47j,&p47_driver,P-47 - The Freedom Fighter (Japan),1988,A,0)



/***************************************************************************

  Plus Alpha (aka Flight Alpha)

***************************************************************************/

ROM_START( plusalph_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "pa-rom2.bin", 0x000000, 0x020000, 0x33244799 )
	ROM_LOAD_ODD(  "pa-rom1.bin", 0x000000, 0x020000, 0xa32fdcae )
	ROM_LOAD_EVEN( "pa-rom3.bin", 0x040000, 0x010000, 0x1b739835 )
	ROM_LOAD_ODD(  "pa-rom4.bin", 0x040000, 0x010000, 0xff760e80 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "pa-rom11.bin", 0x000000, 0x020000, 0xeb709ae7 ) /* Tiles (scroll 1) */
	ROM_LOAD( "pa-rom12.bin", 0x020000, 0x020000, 0xcacbc350 )
	ROM_LOAD( "pa-rom13.bin", 0x040000, 0x020000, 0xfad093dd )

	ROM_LOAD( "pa-rom14.bin", 0x080000, 0x020000, 0xd3676cd1 ) /* Tiles (scroll 2) */
	ROM_LOAD( "pa-rom15.bin", 0x0a0000, 0x020000, 0x8787735b )
	ROM_LOAD( "pa-rom16.bin", 0x0c0000, 0x020000, 0xa06b813b )
	ROM_LOAD( "pa-rom17.bin", 0x0e0000, 0x020000, 0xc6b38a4b )
	ROM_LOAD( "pa-rom19.bin", 0x100000, 0x010000, 0x39ef193c ) /* Text  (scroll 3) */
	ROM_LOAD( "pa-rom20.bin", 0x120000, 0x020000, 0x86c557a8 ) /* Sprites 16x16 */
	ROM_LOAD( "pa-rom21.bin", 0x140000, 0x020000, 0x81140a88 )
	ROM_LOAD( "pa-rom22.bin", 0x160000, 0x020000, 0x97e39886 )
	ROM_LOAD( "pa-rom23.bin", 0x180000, 0x020000, 0x0383fb65 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "pa-rom5.bin", 0x000000, 0x010000, 0xddc2739b )
	ROM_LOAD_ODD(  "pa-rom6.bin", 0x000000, 0x010000, 0xf6f8a167 )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "pa-rom7.bin",  0x000000, 0x020000, 0x9f5d800e )
	ROM_LOAD( "pa-rom8.bin",  0x020000, 0x020000, 0xae007750 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "pa-rom9.bin",  0x000000, 0x020000, 0x065364bd )
	ROM_LOAD( "pa-rom10.bin", 0x020000, 0x020000, 0x395df3b2 )

ROM_END

INPUT_PORTS_START( input_ports_plusalph )
	SERVICE				/* IN0 0x80001.b */
	JOY(0)				/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	UNKNOWN				/* IN5 0x80007.b */
INPUT_PORTS_END

MEGASYS1_GAMEDRIVER(plusalph,0,Plus Alpha,1988,A,rom_decode)


/***************************************************************************

  Phantasm (Japanese version of Avenging Spirit)

***************************************************************************/

ROM_START( phantasm_rom )

	ROM_REGION(0x60000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "phntsm02.bin", 0x000000, 0x020000, 0xd96a3584 )
	ROM_LOAD_ODD(  "phntsm01.bin", 0x000000, 0x020000, 0xa54b4b87 )
	ROM_LOAD_EVEN( "phntsm04.bin", 0x040000, 0x010000, 0xdc0c4994 )
	ROM_LOAD_ODD(  "phntsm03.bin", 0x040000, 0x010000, 0x1d96ce20 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "phntsm14.bin",  0x000000, 0x080000, 0x728335d4 )
	ROM_LOAD( "phntsm18.bin",  0x080000, 0x080000, 0x7896f6b0 )
	ROM_LOAD( "phntsm19.bin",  0x100000, 0x020000, 0x0c37edf7 )
	ROM_LOAD( "phntsm23.bin",  0x120000, 0x080000, 0x2b1180b3 )

	ROM_REGION(0x20000)		/* Region 2 - sound cpu code */
	ROM_LOAD_EVEN( "phntsm05.bin", 0x000000, 0x010000, 0x3b169b4a )
	ROM_LOAD_ODD(  "phntsm06.bin", 0x000000, 0x010000, 0xdf2dfb2e )

	ROM_REGION(0x40000)		/* Region 3 - ADPCM sound samples */
	ROM_LOAD( "phntsm08.bin", 0x000000, 0x040000, 0x05bc04d9 )

	ROM_REGION(0x40000)		/* Region 4 - ADPCM sound samples */
	ROM_LOAD( "phntsm10.bin", 0x000000, 0x040000, 0x13be9979 )

ROM_END

INPUT_PORTS_START( input_ports_phantasm )
	INPUT_PORTS_AVSPIRIT
INPUT_PORTS_END

MEGASYS1_GAMEDRIVER(phantasm,&avspirit_driver,Phantasm (Japan),1990,A,rom_decode)



/***************************************************************************

  RodLand (World)

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


#define INPUT_PORTS_RODLAND \
	SERVICE				/* IN0 0x80001.b */	\
	JOY(0)				/* IN1 0x80003.b */	\
	RESERVE				/* IN2 0x80004.b */	\
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */	\
	COINAGE_A_2			/* IN4 0x80006.b */	\
	PORT_START			/* IN5 0x80007.b */	\
	PORT_DIPNAME( 0x01, 0x01, "DSW-B 0 (Unused?)" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x02, 0x02, "DSW-B 1 (Unused?)" ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x04, 0x04, "DSW-B 2 (Unused?)" ) \
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x08, 0x08, "DSW-B 3 (Lives?)" ) \
	PORT_DIPSETTING(    0x08, "Off(3?)" ) \
	PORT_DIPSETTING(    0x00, "On(2?)" ) \
	PORT_DIPNAME( 0x10, 0x10, "DSW-B 4 (Other Gfx?)" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x20, 0x20, "DSW-B 5 (5+6?)" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x40, 0x40, "DSW-B 6 (5+6?)" ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_START( input_ports_rodland )
	INPUT_PORTS_RODLAND
INPUT_PORTS_END

MEGASYS1_GAMEDRIVER(rodland,0,RodLand (World),1990,A,rom_decode)



/***************************************************************************

  RodLand (Japan)

***************************************************************************/

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

INPUT_PORTS_START( input_ports_rodlandj )
	INPUT_PORTS_RODLAND
INPUT_PORTS_END

/* second half of the text gfx should be all FF's, not 0's */
void rodlandj_fill(void)
{
	unsigned char *RAM = Machine->memory_region[1];
	int i;

	for (i = 0x110000; i < 0x120000; i++)	RAM[i] = 0xFF;
}

//MEGASYS1_GAMEDRIVER(rodlandj,&rodland_driver,RodLand (Japan),1990,A,rodlandj_fill)
MEGASYS1_GAMEDRIVER(rodlandj,0,RodLand (Japan),1990,A,rodlandj_fill)



/***************************************************************************

  Saint Dragon

***************************************************************************/

ROM_START( stdragon_rom )

	ROM_REGION(0x40000)		/* Region 0 - main cpu code */
	ROM_LOAD_EVEN( "jsd-02.bin", 0x000000, 0x020000, 0xcc29ab19 )
	ROM_LOAD_ODD(  "jsd-01.bin", 0x000000, 0x020000, 0x67429a57 )

	ROM_REGION_DISPOSE(0x1a0000)	/* Region 1 - temporary for gfx roms */
	ROM_LOAD( "jsd-11.bin", 0x000000, 0x020000, 0x2783b7b1 ) /* Tiles (scroll 1) */
	ROM_LOAD( "jsd-12.bin", 0x020000, 0x020000, 0x89466ab7 )
	ROM_LOAD( "jsd-13.bin", 0x040000, 0x020000, 0x9896ae82 )
	ROM_LOAD( "jsd-14.bin", 0x060000, 0x020000, 0x7e8da371 )
	ROM_LOAD( "jsd-15.bin", 0x080000, 0x020000, 0xe296bf59 ) /* Tiles (scroll 2) */
	ROM_LOAD( "jsd-16.bin", 0x0a0000, 0x020000, 0xd8919c06 )
	ROM_LOAD( "jsd-17.bin", 0x0c0000, 0x020000, 0x4f7ad563 )
	ROM_LOAD( "jsd-18.bin", 0x0e0000, 0x020000, 0x1f4da822 )
	ROM_LOAD( "jsd-19.bin", 0x100000, 0x010000, 0x25ce807d ) /* Text  (scroll 3) */
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
	SERVICE				/* IN0 0x80001.b */
	JOY(0)				/* IN1 0x80003.b */
	RESERVE				/* IN2 0x80004.b */
	JOY(IPF_PLAYER2)	/* IN3 0x80005.b */
	COINAGE_A			/* IN4 0x80006.b */
	UNKNOWN				/* IN5 0x80007.b */
INPUT_PORTS_END

MEGASYS1_GAMEDRIVER(stdragon,0,Saint Dragon,1988,A,rom_decode)
