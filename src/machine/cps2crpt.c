/******************************************************************************

CPS-2 Encryption

All credit goes to Andreas Naive for breaking the encryption algorithm.
Code by Nicola Salmoria.


The encryption only affects opcodes, not data.

It consists of two 4-round Feistel networks (FN) and involves both
the 16-bit opcode and the low 16 bits of the address.

Let be:

E = 16-bit ciphertext
A = 16-bit address
K1 = 96-bit key
K2 = 96-bit key
D = 16-bit plaintext
y = FN1(x,k) = function describing the first Feistel network (x,y = 16 bit, k = 96 bit)
y = FN2(x,k) = function describing the second Feistel network (x,y = 16 bit, k = 96 bit)
y = EX(x) = fixed function that expands the 16-bit x to the 96-bit y

Then the cipher can be described as:

D = FN2( E, K2 XOR EX( FN1(A, K1) ) )


Each round of the Feistel networks consists of four substitution boxes. The boxes
have 6 inputs and 2 outputs. Usually the input is the XOR of a data bit and a key
bit, however in some cases only the key is used.

It is currently not possible to perfectly recontruct the precise contents of the
s-boxes, which means the 192-bit keys we are using are certainly not the same as
the ones actually used by the hardware.

The second FN should be faithful to the real thing, apart from constant XORs and
constant permutations on the contents of each s-box, which would have as effect
a constant XOR and permutation on K2.

The first FN is less accurate, so additionally to the above it's possible that the
s-boxes with only 4 or 5 data inputs could require different XORs and permutations
for the subtables. The effect of this on K1 would be more complex, involving
correlations of the key bits.

The s-boxes were chosen in order to use an empty key (all FF) for the dead board.

The code isn't very fast, it's written to be more easily modified as we discover
new things about the s-boxes and keys.


We are not sure how long the key really is. 192 bits is the maximum that would be
needed to run the algorithm, but it's possible there are correlations between the
bits, e.g. there could be a key scheduling algorithm to extract 24 bits from a
64-bit key at every round.

Also, the hardware has different watchdog opcodes and address range (see below)
which are stored in the battery backed RAM. It's possible that those values would
be derived from the 192-bit key we are using, or are additional to it.



First FN:

 B(0 1 3 5 8 9 11 12)        A(10 4 6 7 2 13 15 14)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (10 4 6 7 2 13 15 14)       (0 1 3 5 8 9 11 12)


Second FN:

 B(3 5 9 10 8 15 12 11)      A(6 0 2 13 1 4 14 7)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (6 0 2 13 1 4 14 7)         (3 5 9 10 8 15 12 11)

******************************************************************************

Some Encryption notes.
----------------------

Address range.

The encryption does _not_ cover the entire address space. The range covered
differs per game.


Encryption Watchdog.

The CPS2 system has a watchdog system that will disable the decryption
of data if the watchdog isn't triggered at least once every few seconds.
The trigger varies from game to game (some games do use the same) and is
basically a 68000 opcode/s instruction. Below is a list of those known;

        cmpi.l  #$7B5D94F1,D6       1944: The Loop Master
        cmpi.l  #$00951101,D1       19xx: The War Against Destiny
        cmpi.l  #$12345678,D0       Alien Vs. Predator
        cmpi.l  #$00970131,D1       Battle Circuit
        cmpi.l  #$00970310,D1       Capcom Sports Club
        cmpi.l  #$4D175B3C,D6       Choko
        cmpi.b  #$FF,$0C38          Cyberbots
        cmpi.w  #$1019,$4000        Dungeons & Dragons: Tower of Doom
        cmpi.l  #$19660419,D1       Dungeons & Dragons: Shadow over Mystara
         unknown                    Galum Pa!
        cmpi.l  #$19721027,D1       Giga Wing

        cmp.w   A4,D7 \
        cmp.w   D4,D1  }-           Great Mahou Daisakusen
        cmpa.w  D5,A3 /

         unknown                    Hyper Street Fighter II
         unknown                    Jyangokushi
         unknown                    Ken Sei Mogura (may not be a CPS2 game)

        cmpa.w  D0,A3 \
        cmp.w   D7,D2  }-           Mars Matrix
        cmpa.w  A7,A5 /

        cmpi.l  #$19660419,D1       Marvel Super Heroes
        cmpi.l  #$19721027,D1       Marvel Super Heroes Vs. Street Fighter
        cmpi.l  #$19720121,D1       Marvel Vs. Capcom
        cmpi.l  #$347D89A3,D4       Mighty! Pang
         unknown                    Pnickies
        cmpi.l  #$1F740D12,D0       Pocket Fighters
        move.w  $00804020,D0        Powered Gear
        cmpi.l  #$63A1B8D3,D1       Progear no Arashi
        cmpi.l  #$9A7315F1,D2       Puzz Loop 2
        cmpi.l  #$19730827,D1       Quiz Nanairo Dreams
        cmpi.l  #$05642194,D0       Rockman                (found in CPS1 version)
        cmpi.l  #$01647101,D0       Rockman2
        cmpi.l  #$05642194,D0       Street Fighter Zero
        cmpi.l  #$30399783,D0       Street Fighter Zero 2
        cmpi.l  #$8E739110,D0       Street Fighter Zero 2 Alpha
        cmpi.l  #$1C62F5A8,D0       Street Fighter Zero 3
        move.w  $00804020,D0        Super Muscle Bomber
        cmpi.l  #$30399819,D0       Super Puzzle Fighter II X
        btst    #7,$2000            Super Street Fighter II
        btst    #7,$2000            Super Street Fighter II X
        btst    #3,$7345            Ultimate Ecology
        btst    #0,$6160            Vampire
        btst    #0,$6160            Vampire Hunter
        cmpi.l  #$06920760,D0       Vampire Hunter 2
        cmpi.l  #$726A4BAF,D0       Vampire Savior
        cmpi.l  #$06920760,D0       Vampire Savior 2
        cmpi.l  #$19720301,D0       X-Men, Children of the Atom
        cmpi.l  #$19720327,D1       X-Men Vs. Street Fighter

        for a dead PCB the encryption watchdog sequence becomes
        0xffff 0xffff 0xffff

*******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68kmame.h"
#include "ui.h"


/******************************************************************************/

static const int fn1_groupA[8] = { 10, 4, 6, 7, 2, 13, 15, 14 };
static const int fn1_groupB[8] = {  0, 1, 3, 5, 8,  9, 11, 12 };

static const int fn2_groupA[8] = { 6, 0, 2, 13, 1,  4, 14,  7 };
static const int fn2_groupB[8] = { 3, 5, 9, 10, 8, 15, 12, 11 };

/******************************************************************************/

// The order of the input and output bits in the s-boxes is arbitrary.
// Each s-box can be XORed with an arbitrary vale in range 0-3 (but the same value
// must be used for the corresponding output bits in f1 and f3 or in f2 and f4)

struct sbox
{
	const int table[64];
	const int inputs[6];		// positions of the inputs bits, -1 means no input except from key
	const int outputs[2];		// positions of the output bits
};


static const struct sbox fn1_r1_boxes[4] =
{
	{	// subkey bits  0- 5
		{
			// 4 subtables, XOR and permutations might not be in sync
			2,3,3,2,0,3,0,0,0,3,0,0,3,3,0,2,1,3,2,3,1,3,2,0,2,1,1,2,2,2,2,3,
			0,1,0,0,1,1,3,0,3,1,3,1,2,2,0,0,3,2,0,3,0,3,1,2,0,2,2,0,1,0,1,1,
		},
		{ 3, 4, 5, 6, -1, -1 },
		{ 3, 6 }
	},
	{	// subkey bits  6-11
		{
			// 2 subtables, XOR and permutations might not be in sync
			2,1,0,1,1,2,1,1,3,1,3,3,3,2,0,1,0,0,2,3,1,3,2,3,2,2,1,3,3,1,3,2,
			3,0,2,2,2,1,1,1,1,2,1,0,0,0,2,3,2,3,1,3,0,0,0,2,1,2,2,3,0,3,3,3,
		},
		{ 0, 1, 2, 4, 7, -1 },
		{ 2, 7 }
	},
	{	// subkey bits 12-17
		{
			0,1,0,1,2,3,0,2,0,2,0,1,0,0,1,0,3,0,3,1,1,0,2,2,3,1,2,0,3,3,2,3,
			3,1,2,3,1,3,1,1,1,2,0,2,2,0,0,0,2,3,1,2,1,0,2,0,2,1,0,1,0,2,1,0,
		},
		{ 0, 1, 2, 3, 6, 7 },
		{ 0, 1 }
	},
	{	// subkey bits 18-23
		{
			2,2,1,3,3,2,1,0,1,0,1,3,0,0,0,2,3,2,0,3,0,2,2,1,1,2,3,2,1,3,2,1,
			3,3,2,0,2,1,3,3,0,0,3,0,1,1,3,3,2,1,0,1,0,1,0,1,3,1,1,2,2,3,2,0,
		},
		{ 0, 1, 3, 5, 6, 7 },
		{ 4, 5 }
	},
};

static const struct sbox fn1_r2_boxes[4] =
{
	{	// subkey bits 24-29
		{
			// 2 subtables, XOR and permutations might not be in sync
			3,0,1,3,3,1,1,0,1,0,2,2,0,3,2,3,0,2,2,1,3,3,1,0,0,3,0,3,2,1,1,0,
			3,3,2,0,3,0,3,1,0,3,0,1,0,2,1,3,1,3,0,3,3,1,3,3,3,2,3,2,2,3,1,2,
		},
		{ 0, 1, 2, 3, 6, -1 },
		{ 1, 6 }
	},
	{	// subkey bits 30-35
		{
			// 2 subtables, XOR and permutations might not be in sync
			0,2,2,2,3,2,0,1,1,2,1,2,3,0,1,2,3,2,1,0,3,1,2,1,2,3,0,2,0,2,0,3,
			1,2,3,2,1,3,0,1,1,0,2,0,0,2,3,2,3,3,0,1,2,2,1,0,1,0,1,2,3,2,1,3,
		},
		{ 2, 4, 5, 6, 7, -1 },
		{ 5, 7 }
	},
	{	// subkey bits 36-41
		{
			0,1,0,2,1,1,0,1,0,2,2,2,1,3,0,0,1,1,3,1,2,2,2,3,1,0,3,3,3,2,2,2,
			1,1,3,0,3,1,3,0,1,3,3,2,1,1,0,0,1,2,2,2,1,1,1,2,2,0,0,3,2,3,1,3,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 3 }
	},
	{	// subkey bits 42-47
		{
			2,1,0,3,3,3,2,0,1,2,1,1,1,0,3,1,1,3,3,0,1,2,1,0,0,0,3,0,3,0,3,0,
			1,3,3,3,0,3,2,0,2,1,2,2,2,1,1,3,0,1,0,1,0,1,1,1,1,3,1,0,1,2,3,3,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 2, 4 }
	},
};

static const struct sbox fn1_r3_boxes[4] =
{
	{	// subkey bits 48-53
		{
			// 2 subtables, XOR and permutations might not be in sync
			0,1,1,3,1,1,3,1,2,0,2,1,2,2,2,2,0,1,1,2,2,2,3,0,3,2,0,1,2,2,0,1,
			2,0,2,0,0,0,3,2,0,0,0,3,3,1,1,0,3,0,0,0,0,0,2,3,0,1,2,3,2,2,1,0,
		},
		{ 0, 1, 5, 6, 7, -1 },
		{ 0, 5 }
	},
	{	// subkey bits 54-59
		{
			// 2 subtables, XOR and permutations might not be in sync
			0,1,2,0,1,3,2,3,0,3,3,2,1,2,0,3,3,1,1,1,2,0,3,1,3,1,2,0,3,2,2,1,
			2,2,3,0,3,2,0,2,2,3,2,3,0,2,3,0,0,1,0,2,1,2,2,1,1,0,2,3,1,1,1,0,
		},
		{ 2, 3, 4, 6, 7, -1 },
		{ 6, 7 }
	},
	{	// subkey bits 60-65
		{
			2,1,1,2,0,0,1,2,3,1,1,2,0,1,3,0,3,1,1,0,0,2,3,0,0,0,0,3,2,0,0,0,
			3,0,2,1,1,3,1,2,2,1,2,2,2,0,0,1,2,3,1,0,2,0,0,2,3,1,2,0,0,0,3,0,
		},
		{ 0, 2, 3, 4, 5, 6 },
		{ 1, 4 }
	},
	{	// subkey bits 66-71
		{
			0,1,0,0,2,1,3,2,3,3,2,1,0,1,1,1,1,1,0,3,3,1,1,0,0,2,2,1,0,3,3,2,
			1,3,3,0,3,0,2,1,1,2,3,2,2,2,1,0,0,3,3,3,2,2,3,1,0,2,3,0,3,1,1,0,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 2, 3 }
	},
};

static const struct sbox fn1_r4_boxes[4] =
{
	{	// subkey bits 72-77
		{
			1,1,1,1,1,0,1,3,3,2,3,0,1,2,0,2,3,3,0,1,2,1,2,3,0,3,2,3,2,0,1,2,
			0,1,0,3,2,1,3,2,3,1,2,3,2,0,1,2,2,0,0,0,2,1,3,0,3,1,3,0,1,3,3,0,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 4 }
	},
	{	// subkey bits 78-83
		{
			3,0,0,0,0,1,0,2,3,3,1,3,0,3,1,2,2,2,3,1,0,0,2,0,1,0,2,2,3,3,0,0,
			1,1,3,0,2,3,0,3,0,3,0,2,0,2,0,1,0,3,0,1,3,1,1,0,0,1,3,3,2,2,1,0,
		},
		{ 0, 1, 2, 3, 5, 6 },
		{ 1, 3 }
	},
	{	// subkey bits 84-89
		{
			0,1,1,2,0,1,3,1,2,0,3,2,0,0,3,0,3,0,1,2,2,3,3,2,3,2,0,1,0,0,1,0,
			3,0,2,3,0,2,2,2,1,1,0,2,2,0,0,1,2,1,1,1,2,3,0,3,1,2,3,3,1,1,3,0,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 2, 6 }
	},
	{	// subkey bits 90-95
		{
			0,1,2,2,0,1,0,3,2,2,1,1,3,2,0,2,0,1,3,3,0,2,2,3,3,2,0,0,2,1,3,3,
			1,1,1,3,1,2,1,1,0,3,3,2,3,2,3,0,3,1,0,0,3,0,0,0,2,2,2,1,2,3,0,0,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 5, 7 }
	},
};

/******************************************************************************/

static const struct sbox fn2_r1_boxes[4] =
{
	{	// subkey bits  0- 5
		{
			// note that subkey bit 5 is 0 for all games, only exception is the dead board
			0,1,0,1,2,2,3,3,0,3,0,2,3,0,1,2,1,1,0,2,0,3,1,1,2,2,1,3,1,1,3,1,
			2,0,2,0,3,0,0,3,1,1,0,1,3,2,0,1,2,0,1,2,0,2,0,2,2,2,3,0,2,1,3,0,
		},
		{ 0, 3, 4, 5, 7, -1 },
		{ 6, 7 }
	},
	{	// subkey bits  6-11
		{
			1,3,1,3,2,2,2,2,2,2,0,1,0,1,1,2,3,1,1,2,0,3,3,3,2,2,3,1,1,1,3,0,
			1,1,0,3,0,2,0,1,3,0,2,0,1,1,0,0,1,3,2,2,0,2,2,2,2,0,1,3,3,3,1,1,
		},
		{ 1, 2, 3, 4, 6, -1 },
		{ 3, 5 }
	},
	{	// subkey bits 12-17
		{
			1,0,2,2,3,3,3,3,1,2,2,1,0,1,2,1,1,2,3,1,2,0,0,1,2,3,1,2,0,0,0,2,
			2,0,1,1,0,0,2,0,0,0,2,3,2,3,0,1,3,0,0,0,2,3,2,0,1,3,2,1,3,1,1,3,
		},
		{ 1, 2, 4, 5, 6, 7 },
		{ 1, 4 }
	},
	{	// subkey bits 18-23
		{
			3,1,0,3,2,3,1,3,2,3,1,1,3,3,1,2,3,2,3,0,0,1,2,0,0,3,0,0,3,3,1,0,
			3,2,0,0,1,0,1,2,0,3,1,0,2,0,2,2,3,3,2,1,3,1,0,0,3,0,1,0,2,3,0,2,
		},
		{ 0, 2, 3, 5, 6, 7 },
		{ 0, 2 }
	},
};

static const struct sbox fn2_r2_boxes[4] =
{
	{	// subkey bits 24-29
		{
			1,1,0,1,2,3,2,3,3,1,2,2,2,0,2,3,3,1,3,0,3,0,3,1,3,0,0,1,1,3,0,3,
			0,3,3,0,0,2,0,0,1,1,2,1,2,1,1,0,2,2,2,1,1,3,3,0,3,1,2,1,1,1,0,2,
		},
		{ 0, 2, 4, 6, -1, -1 },
		{ 4, 6 }
	},
	{	// subkey bits 30-35
		{
			0,3,0,3,3,2,1,2,3,1,1,1,2,0,2,3,0,3,1,2,2,1,3,3,3,2,1,2,2,0,1,0,
			2,3,0,1,2,0,1,1,2,0,2,1,2,0,2,3,3,1,0,2,3,3,0,3,1,1,3,0,0,1,2,0,
		},
		{ 1, 3, 4, 5, 6, 7 },
		{ 0, 3 }
	},
	{	// subkey bits 36-41
		{
			0,0,2,1,3,2,1,0,1,2,2,2,1,1,0,3,1,2,2,3,2,1,1,0,3,0,0,1,1,2,3,1,
			3,3,2,2,1,0,1,1,1,2,0,1,2,3,0,3,3,0,3,2,2,0,2,2,1,2,3,2,1,0,2,1,
		},
		{ 0, 1, 3, 4, 5, 7 },
		{ 1, 7 }
	},
	{	// subkey bits 42-47
		{
			2,0,2,1,2,0,0,2,3,1,0,2,2,3,0,3,3,3,3,2,2,1,1,3,2,2,0,0,2,2,2,1,
			3,2,3,3,1,1,0,0,3,0,0,2,2,3,1,3,1,1,0,1,0,1,3,1,0,0,2,1,3,2,0,2,
		},
		{ 1, 2, 3, 5, 6, 7 },
		{ 2, 5 }
	},
};

static const struct sbox fn2_r3_boxes[4] =
{
	{	// subkey bits 48-53
		{
			0,0,0,0,3,3,3,0,1,2,1,0,2,3,3,1,1,1,2,1,1,3,2,2,0,2,2,3,3,3,2,0,
			0,2,0,3,3,1,0,0,1,1,0,2,3,2,1,2,2,1,2,1,2,3,1,3,2,2,1,3,3,0,0,1,
		},
		{ 2, 3, 4, 6, -1, -1 },
		{ 3, 5 }
	},
	{	// subkey bits 54-59
		{
			0,3,2,0,0,1,2,0,0,2,0,1,2,0,0,0,3,1,3,1,0,2,3,3,0,1,2,2,3,2,0,0,
			2,3,3,3,0,1,0,3,0,2,1,1,0,1,0,3,1,3,1,3,1,0,3,2,2,2,2,3,1,0,2,1,
		},
		{ 0, 1, 3, 5, 7, -1 },
		{ 0, 2 }
	},
	{	// subkey bits 60-65
		{
			2,2,1,0,2,3,3,0,0,0,1,3,1,2,3,2,2,3,1,3,0,3,0,3,3,2,2,1,0,0,0,2,
			1,2,2,2,0,0,1,2,0,1,3,0,2,3,2,1,3,2,2,2,3,1,3,0,2,0,2,1,0,3,3,1,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 1, 6 }
	},
	{	// subkey bits 66-71
		{
			1,2,3,2,0,2,1,3,3,1,0,1,1,2,2,0,0,1,1,1,2,1,1,2,0,1,3,3,1,1,1,2,
			3,3,1,0,2,1,1,1,2,1,0,0,2,2,3,2,3,2,2,0,2,2,3,3,0,2,3,0,2,2,1,1,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 4, 7 }
	},
};

static const struct sbox fn2_r4_boxes[4] =
{
	{	// subkey bits 72-77
		{
			2,3,1,3,3,0,2,0,1,2,3,1,2,0,0,1,2,0,3,3,1,1,0,1,3,1,1,0,1,1,1,3,
			2,0,1,1,2,1,3,3,1,1,1,2,0,1,0,2,0,1,2,0,2,3,0,2,3,3,2,2,3,2,0,1,
		},
		{ 0, 1, 3, 6, 7, -1 },
		{ 0, 3 }
	},
	{	// subkey bits 78-83
		{
			2,2,1,2,2,3,1,3,3,3,0,1,0,1,3,0,0,0,1,2,0,3,3,2,3,2,1,3,2,1,0,2,
			1,2,2,1,0,3,3,1,0,2,2,2,1,0,1,0,1,1,0,1,0,2,1,0,2,1,0,2,3,2,3,3,
		},
		{ 0, 1, 2, 4, 5, 6 },
		{ 4, 7 }
	},
	{	// subkey bits 84-89
		{
			3,1,0,1,2,2,2,1,3,2,2,1,0,2,1,2,2,3,2,1,3,2,3,0,0,2,1,1,0,0,3,2,
			0,0,1,2,1,2,3,2,1,0,0,3,2,1,1,3,0,3,1,0,0,3,1,1,3,3,2,0,1,0,1,3,
		},
		{ 0, 2, 3, 4, 5, 7 },
		{ 1, 2 }
	},
	{	// subkey bits 90-95
		{
			2,0,0,3,2,2,2,1,3,3,1,1,2,0,0,3,1,0,3,2,1,0,2,0,3,2,2,3,2,0,3,0,
			1,3,0,2,2,1,3,3,0,1,0,3,1,1,3,2,0,3,0,2,3,2,1,3,2,3,0,0,1,3,2,1,
		},
		{ 2, 3, 4, 5, 6, 7 },
		{ 5, 6 }
	},
};

/******************************************************************************/

static UINT16 feistel(UINT16 val, int *f1, int* f2, int *f3, int* f4, const int *bitsA, const int *bitsB)
{
	const UINT8 l0 = BITSWAP8(val, bitsB[7],bitsB[6],bitsB[5],bitsB[4],bitsB[3],bitsB[2],bitsB[1],bitsB[0]);
	const UINT8 r0 = BITSWAP8(val, bitsA[7],bitsA[6],bitsA[5],bitsA[4],bitsA[3],bitsA[2],bitsA[1],bitsA[0]);

	const UINT8 l1 = r0;
	const UINT8 r1 = l0 ^ f1[r0];

	const UINT8 l2 = r1;
	const UINT8 r2 = l1 ^ f2[r1];

	const UINT8 l3 = r2;
	const UINT8 r3 = l2 ^ f3[r2];

	const UINT8 l4 = r3;
	const UINT8 r4 = l3 ^ f4[r3];

	return
		(BIT(l4, 0) << bitsA[0]) |
		(BIT(l4, 1) << bitsA[1]) |
		(BIT(l4, 2) << bitsA[2]) |
		(BIT(l4, 3) << bitsA[3]) |
		(BIT(l4, 4) << bitsA[4]) |
		(BIT(l4, 5) << bitsA[5]) |
		(BIT(l4, 6) << bitsA[6]) |
		(BIT(l4, 7) << bitsA[7]) |
		(BIT(r4, 0) << bitsB[0]) |
		(BIT(r4, 1) << bitsB[1]) |
		(BIT(r4, 2) << bitsB[2]) |
		(BIT(r4, 3) << bitsB[3]) |
		(BIT(r4, 4) << bitsB[4]) |
		(BIT(r4, 5) << bitsB[5]) |
		(BIT(r4, 6) << bitsB[6]) |
		(BIT(r4, 7) << bitsB[7]);
}



static int extract_inputs(unsigned int val, const int *inputs)
{
	int i;
	int res = 0;

	for (i = 0; i < 6; ++i)
	{
		if (inputs[i] != -1)
			res |= BIT(val, inputs[i]) << i;
	}

	return res;
}



static void build_ftable(unsigned int *ftable, const struct sbox *sboxes, UINT32 key)
{
	int box;
	int i;

	for (i = 0; i < 256; ++i)
		ftable[i] = 0;

	for (box = 0; box < 4; ++box)
	{
		const struct sbox *sbox = &sboxes[box];
		const unsigned int subkey = (key >> (6*box)) & 0x3f;

		for (i = 0; i < 256; ++i)
		{
			int out = sbox->table[extract_inputs(i, sbox->inputs) ^ subkey];

			if (out & 1)
				ftable[i] |= 1 << sbox->outputs[0];
			if (out & 2)
				ftable[i] |= 1 << sbox->outputs[1];
		}
	}
}



static void expand_subkey(UINT32* subkey, UINT16 seed)
{
	int bit;

	static const int dependencies[96*2] =
	{
		 2, -1,	// bit 0 of the 96-bit subkey depends on bit 2 of the 16-bit seed
		 8, -1,
		 4, -1,
		 4, -1,
		13, -1,
		-1, -1,	// bit 5 of the 96-bit subkey doesn't depend on any bit of the 16-bit seed
		12, -1,
		 6, -1,
		 3, -1,
		 7, -1,
		 8, -1,
		 6, 12,	// bit 11 of the 96-bit subkey depends on bits 6 and 12 of the 16-bit seed
		 0, -1,
		 0, -1,
		 7, -1,
		10, -1,
		13, -1,
		 8, -1,
		11, -1,
		15, -1,
		 2, -1,
		 8, -1,
		 1, -1,
		 2, -1,
		 3, -1,
		14, -1,
		 5, -1,
		13, -1,
		 2, 14,
		 3, 11,
		 3, -1,
		 9, -1,
		 1, -1,
		 6, -1,
		12, -1,
		 1, -1,
		 3, -1,
		12, -1,
		10, -1,
		15, -1,
		 0, -1,
		 4, -1,
		11, -1,
		15, -1,
		 9, -1,
		 6, -1,
		14, -1,
		 2, -1,
		14, -1,
		 9, -1,
		 0, -1,
		 1, -1,
		 1, 12,
		 0, 15,
		11, -1,
		15, -1,
		 3, -1,
		 7, -1,
		 6, -1,
		10, 15,
		 4, -1,
		 6, -1,
		 2, -1,
		 3, -1,
		10, -1,
		 5, -1,
		 1, -1,
		13, -1,
		14, -1,
		 8, -1,
		 9, -1,
		 4, -1,
		12, -1,
		15, -1,
		15, -1,
		14, -1,
		 3, -1,
		 5, 15,
		12, -1,
		 0, -1,
		 9, -1,
		 9, -1,
		13, -1,
		11, -1,
		 7, -1,
		 5, -1,
		13, -1,
		14, -1,
		11, -1,
		10, -1,
		10, -1,
		 4, -1,
		14, -1,
		 8, -1,
		 1, -1,
		 6, -1,
	};


	subkey[0] = 0;
	subkey[1] = 0;
	subkey[2] = 0;
	subkey[3] = 0;

	for (bit = 0; bit < 96; ++bit)
	{
		int k = 0;
		if (dependencies[2*bit + 0] != -1)
			k ^= BIT(seed, dependencies[2*bit + 0]);
		if (dependencies[2*bit + 1] != -1)
			k ^= BIT(seed, dependencies[2*bit + 1]);

		subkey[bit / 24] |= k << (bit % 24);
	}
}



static void cps2_decrypt(const UINT32* keys)
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	int length = memory_region_length(REGION_CPU1);
	UINT16 *dec = auto_malloc(length);
	int i;
	unsigned int fn1_f1[256];
	unsigned int fn1_f2[256];
	unsigned int fn1_f3[256];
	unsigned int fn1_f4[256];

	// build lookup tables for FN1
	// these tables are constant throughout the address range
	build_ftable(fn1_f1, fn1_r1_boxes, keys[0]);
	build_ftable(fn1_f2, fn1_r2_boxes, keys[1]);
	build_ftable(fn1_f3, fn1_r3_boxes, keys[2]);
	build_ftable(fn1_f4, fn1_r4_boxes, keys[3]);

	for (i = 0; i < 0x10000; ++i)
	{
		unsigned int fn2_f1[256];
		unsigned int fn2_f2[256];
		unsigned int fn2_f3[256];
		unsigned int fn2_f4[256];
		int a;
		UINT16 seed;
		UINT32 subkey[4];

		if ((i & 0xff) == 0)
		{
			char loadingMessage[256]; // for displaying with UI
			sprintf(loadingMessage, "Decrypting %d%%", i*100/0x10000);
			ui_set_startup_text(loadingMessage,FALSE);
		}


		// pass the address through FN1
		seed = feistel(i, fn1_f1,fn1_f2,fn1_f3,fn1_f4, fn1_groupA, fn1_groupB);


		// expend the result to 96-bit
		expand_subkey(subkey, seed);

		// XOR with the second key
		subkey[0] ^= keys[4];
		subkey[1] ^= keys[5];
		subkey[2] ^= keys[6];
		subkey[3] ^= keys[7];


		// build lookup tables for FN2
		// these tables change with the address.
		build_ftable(fn2_f1, fn2_r1_boxes, subkey[0]);
		build_ftable(fn2_f2, fn2_r2_boxes, subkey[1]);
		build_ftable(fn2_f3, fn2_r3_boxes, subkey[2]);
		build_ftable(fn2_f4, fn2_r4_boxes, subkey[3]);


		// decrypt the opcodes
		for (a = i; a < length/2; a += 0x10000)
		{
			dec[a] = feistel(rom[a], fn2_f1,fn2_f2,fn2_f3,fn2_f4, fn2_groupA, fn2_groupB);
		}
	}

	memory_set_decrypted_region(0, 0x000000, length - 1, dec);
	m68k_set_encrypted_opcode_range(0,0,length);
}








extern DRIVER_INIT( cps2_video );


struct game_keys
{
	const char *name;             /* game driver name */
	const UINT32 keys[8];
};


static const struct game_keys keys_table[] =
{
	// name         <---------- 1st FN keys ---------->  <---------- 2nd FN keys ---------->           watchdog
	{ "dead",     { 0xffffff,0xffffff,0xffffff,0xffffff, 0xffffff,0xffffff,0xffffff,0xffffff } },	// ffff ffff ffff
	{ "xmcota",   { 0x4077c0,0xab566e,0xad9a4a,0xa31d3c, 0x12bc4c,0x71ad33,0xa5f0d4,0x7b8c34 } },	// 0C80 1972 0301   cmpi.l  #$19720301,D0
	{ "sfzj",     { 0xfc74bc,0x150796,0x1bb62b,0x99b3b4, 0x6d804f,0x0fedcf,0x8e9fb8,0xdc1171 } },	// 0C80 0564 2194   cmpi.l  #$05642194,D0
	{ "sfzjr1",   { 0xfc74bc,0x150796,0x1bb62b,0x99b3b4, 0x6d804f,0x0fedcf,0x8e9fb8,0xdc1171 } },	// 0C80 0564 2194   cmpi.l  #$05642194,D0
	{ "sfzjr2",   { 0xfc74bc,0x150796,0x1bb62b,0x99b3b4, 0x6d804f,0x0fedcf,0x8e9fb8,0xdc1171 } },	// 0C80 0564 2194   cmpi.l  #$05642194,D0
	{ "csclubj",  { 0xcaaeba,0xb91b30,0x991c31,0x4488e4, 0xde6852,0xe5e82c,0x2a4ac8,0x44c822 } },	// 0C81 0097 0310   cmpi.l  #$00970310,D1
	{ "jyangoku", { 0xcc6720,0xb3179c,0xa75027,0x55fae0, 0x7eda56,0x6de913,0xa4eeac,0xdce0c2 } },	// 0C80 3652 1573   cmpi.l  #$36521573,D0
	{ "pzloop2",  { 0x732850,0x049aa3,0x7ee01b,0x4e7654, 0x517a11,0x1a4079,0x19e35e,0x077c69 } },	// 0C82 9A73 15F1   cmpi.l  #$9A7315F1,D2
	{ "pzloop2j", { 0x732850,0x049aa3,0x7ee01b,0x4e7654, 0x517a11,0x1a4079,0x19e35e,0x077c69 } },	// 0C82 9A73 15F1   cmpi.l  #$9A7315F1,D2
	{ "choko",    { 0xbbeac4,0xc5cb1e,0x98846a,0xfdafbb, 0x8fe69f,0xfcc66e,0xea3182,0x74c247 } },	// 0C86 4D17 5B3C   cmpi.l  #$4D175B3C,D6
	{ 0 }	// end of table
};




DRIVER_INIT( cps2 )
{
	const char *gamename = machine->gamedrv->name;
	const struct game_keys *k = &keys_table[0];

	while (k->name)
	{
		if (strcmp(k->name, gamename) == 0)
		{
			break;
		}
		++k;
	}

	if (k->name)
	{
		// we have a proper key so use it to decrypt
		cps2_decrypt(k->keys);
	}
	else
	{
		// we don't have a proper key so use the XOR tables if available

		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		UINT16 *xor = (UINT16 *)memory_region(REGION_USER1);
		int length = memory_region_length(REGION_CPU1);
		int i;

		if (xor)
		{
			for (i = 0; i < length/2; i++)
				xor[i] ^= rom[i];

			memory_set_decrypted_region(0, 0x000000, length - 1, xor);
			m68k_set_encrypted_opcode_range(0,0,length);
		}
	}

	init_cps2_video(machine);
}
