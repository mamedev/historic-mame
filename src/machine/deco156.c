/* Deco 156 analysis */

/*

interesting locations

block at Ca600 of nslasher_romregion1_and_2_shuffled_and_xor (xor of world/jap prg, uncomment in driver)
this block appears to be half full and should give some indication of the arrangement within a block

the 'blank' block finder finds NOTHING on joemacr, and an awful lot on hoops96/ddream95
(the 'blank' block is only a guess anyway, many of the blocks found won't be blank and many are probably
missed)

several games seem to be using the same, or similar extra encryption.. charlie ninja / magical drop / osman
for example.. areas which are identical between these totally different games are also probably blank data

the encryption is probably a more complex version of that found in deco102.c, despite the large amount of
data available working it out may be difficult.  comparisons between clone sets may be useful for working
out the data swaps as there is a high chance of equal data being shifted across addresses using different
encryption however before that is even attempted the address scrambling needs fully figuring out.

*/

#include "driver.h"

static char filename[256];

static UINT32 reversetable[0x10000];
static data8_t *ROM1_ROM2_shuffled_xored;
static data8_t *ROM1_shuffled;
static data8_t *ROM2_shuffled;
UINT32 romregionsize;
static data8_t *tmp;

/*
magdrop

00041b00 data 2dbb
00043800 data 0b3b
00047100 data ed99
00047e00 data edb9
00048000 data f862
00048f00 data f842
0004a300 data dee2
0004ac00 data dec2
0004e500 data 3860
0004ea00 data 3840
00051000 data 4270
00051f00 data 4250
00053300 data 64f0
00053c00 data 64d0
00055600 data a4f2
00055900 data a4d2
00058b00 data 277d
0005a800 data 01fd
0005c200 data c1df
0005cd00 data c1ff
00061500 data a76e
00061a00 data a74e
00063600 data 81ee
00063900 data 81ce
00065300 data 41ec
00065c00 data 41cc
00066c00 data 1f25
00068100 data c243
00068e00 data c263
0006a200 data e4c3
0006c700 data 24c1
0006c800 data 24e1
0006cd00 data 9892


charlien

00041b00 data 2dbb
00043200 data 8616
00043800 data 0b3b
00045d00 data cb39
00046800 data 18dd
00047100 data ed99
00047d00 data 6348
00047e00 data edb9
00048500 data c5bb
00048f00 data f842
00049600 data bdd2
0004a200 data 0f70
0004aa00 data 9215
0004ac00 data dec2
0004c900 data 1ec0
0004e500 data 3860
0004e600 data 0645
0004ea00 data 3840

00051100 data 93e2
00051f00 data 4250
00052500 data 2140
00053c00 data 64d0
00054f00 data e162
00055600 data a4f2
00055900 data a4d2
00057a00 data 8252
00058700 data a9ac
00058b00 data 277d
0005a800 data 01fd
0005ad00 data 3c24
0005c200 data c1df
0005c700 data fc06
0005cd00 data c1ff
0005d500 data bfb8
0005e000 data 8619
0005ee00 data e77f

0005f900 data 5b18
00061300 data 5b6d
00061500 data a76e
00061b00 data c608
00063600 data 81ee
00063c00 data bc17
00065300 data 41ec
00065c00 data 41cc
00067000 data 676c
00068100 data c243
0006a200 data e4c3
0006c200 data a9cc
0006c700 data 24c1
0006c800 data 24e1
0006d000 data 98bd
0006e400 data 0241
0006ea00 data d3f3

prtytime

00040100 data 1e08
00041b00 data 2dbb
00042d00 data 78a6
00045300 data aa5f
00045500 data a9a3
00045a00 data a983
00047600 data 8f23
00047900 data 8f03
00048700 data 2a0c
00048800 data 2a2c
00048900 data b495
00048f00 data f842
0004a400 data 0c8c
0004ab00 data 0cac
0004c100 data cc8e
0004c700 data cf72
0004ce00 data ccae
0004e200 data ea0e
0004ed00 data ea2e
00050500 data 3876
00051700 data 20ca
00051800 data 20ea
00053300 data 64f0
00053400 data 064a
00053b00 data 066a
00053c00 data 64d0
00054800 data 16c3
00055100 data c648
00055e00 data c668
00057200 data e0c8
00057400 data 53e0
00057d00 data e0e8
00058300 data f533
00058c00 data f513
0005a000 data d3b3
0005a700 data 01dd
0005af00 data d393
0005b200 data 5a51
00060f00 data bc7a
00061200 data 7500
00061d00 data 7520
00063100 data 5380
00063e00 data 53a0
00064200 data e0a4
00065400 data 9382
00065b00 data 93a2
00066d00 data eeb3
00067700 data b502
00067800 data b522
00067e00 data 060a
0006a200 data e4c3
0006b800 data 9765
0006c000 data 467b
0006cf00 data 465b
0006e300 data 60fb
0006ea00 data d3f3
0006ec00 data 60db

osman

00041400 data 2d9b
00041b00 data 2dbb
00045200 data cb19
00045d00 data cb39
00047f00 data 893d
00048000 data f862
0004c600 data 1ee0
0004c900 data 1ec0
0004e000 data 05b9
0004eb00 data 7f73

00053200 data 9d4f
00053300 data 64f0
00053c00 data 64d0
00055300 data 29ff
00055800 data 5d6d
00057300 data ac74
00057500 data 8272
00057a00 data 8252
0005a700 data 01dd
0005a800 data 01fd
0005cc00 data 2bd0
0005e100 data e75f
0005eb00 data d61f
0005ee00 data e77f

00063600 data 81ee
00063800 data 6be1
00063900 data 81ce
00065200 data abc3
00067000 data 676c
00067600 data 6490
00067f00 data 674c
0006a200 data e4c3
0006ad00 data e4e3
0006c600 data dd7e
0006dd00 data 47d7
0006e400 data 0241
0006eb00 data 0261




hvysmash

00070300 data f943
00070c00 data f963
00071800 data cb51
00071e00 data 7879
00073200 data 5ed9
00073400 data edf1
00073d00 data 5ef9
00075100 data 2df3
00075300 data c244
00075700 data 9edb
00075800 data 9efb
00075e00 data 2dd3
00076a00 data 9f71
00077400 data b85b
00077a00 data 69e9
00077b00 data b87b
00078100 data 710f
00078200 data cf1a
00078500 data ada0
00078a00 data ad80
00078c00 data 1ea8
00078d00 data cf3a
0007a600 data 8b20
0007a700 data ea66
0007a800 data ea46
0007a900 data 8b00
0007b700 data ac2a
0007c300 data 4b22
0007c500 data f80a
0007c700 data 17bd
0007ca00 data f82a
0007cc00 data 4b02
0007e000 data 6da2
0007ef00 data 6d82
0007f200 data 748d
0007fb00 data 7751



0007ea00 data e08f


   OSMAN                CHARLIEN              MAGDROP              PRTYTIME           JOEMACRETURNS          NSLASHER           WCVOL95             HVYSMASH
                                                                                                        (note                 (same note
                                                                                                         addr+0x80000 =           as nslsh)
                                                                                                           ^0x0008?)







                                                                                                          00077200 data 0b73
                                          00077400 data b853                                              00077400 data b85b
00077500 data ff60
                                                                                                                               00077700 data 867e
                                                                                                                               00077800 data 865e
                                          00077b00 data b873                                              00077b00 data b87b
                                                                                                          00077d00 data 0b53
00077e00 data 85aa?                       00077e00 data ca5b                         00077e00 data ca5b                        00077e00 data 85a2?
                                                                00078200 data cf12
00078500 data ada8   00078500 data ada8   00078500 data ada8    00078500 data ada8                        00078500 data ada0
                                                                                                                               00078600 data 2351
                                                                                                          00078900 data 2371
00078a00 data ad88                        00078a00 data ad88    00078a00 data ad88
                                                                00078d00 data cf32
                                                                                                          00079700 data b48f
                                                                                                          00079800 data b4af
                                                                00079f00 data 9e1b
                                                                0007a100 data e992
                                                                                                                               0007a500 data 05d1
                     0007a600 data 8b28   0007a600 data 8b28                                              0007a600 data 8b20
0007a900 data 8b08                        0007a900 data 8b08                                              0007a900 data 8b00

0007aa00 data 05f9                                                                                                             0007aa00 data 05f1
                                                                                                                               0007ac00 data 060d
                                                                0007ae00 data e9b2
                                                                0007b300 data 78d7
                                                                                                          0007ba00 data 43bd
                     0007be00 data 7ffe
0007c300 data 4b2a   0007c300 data 4b2a
                                                                0007c400 data 2990
0007c500 data f802?                                             0007c500 data 07fd?
                     0007c600 data c627
                                                                0007cb00 data 29b0
0007cc00 data 4b0a   0007cc00 data 4b0a
                                                                0007cd00 data 2a4c
0007cf00 data e801
                     0007e000 data 6daa   0007e000 data 6daa                                              0007e000 data 6da2
0007e100 data 092e?                                                                                       0007e100 data 0ce4?
0007e300 data e35b                                                                                                              0007e300 data e353
                                                                                                                                0007e500 data e0af
                                                                                                          0007e600 data de8a
                                                                0007e700 data 0f10
                                                                0007e800 data 0f30
                                                                                                          0007e900 data deaa
                                                                                                                                0007ea00 data e08f
                                                                                                                                0007ec00 data e373

                                                                                                          0007ee00 data 0cc4
                     0007ef00 data 6d8a   0007ef00 data 6d8a                                              0007ef00 data 6d82






































*/

static void wordcounter(int offs)
{
	UINT32 counttable[0x10000];

	int y;
	int used;

	static int blankblocks = 0;

	for (y=0;y<0x10000;y++)
		counttable[y]=0;

	for (y=offs;y<offs+0x100;y+=2)
	{
		int word = (ROM1_shuffled[y]<<8) | ROM1_shuffled[y+1];

		counttable[word]++;
	}

	used = 0;

	for(y=0;y<0x10000;y++)
	{
		if ((counttable[y]!=0)&&(counttable[y]!=16))
		{
			used = 1;
		}
	}


	if (used ==0)		printf("%08x , %02x%02x , \n",offs,ROM1_shuffled[offs+0],ROM1_shuffled[offs+1]);
	else  printf("%08x , %02x%02x , \n",offs,0,0);

	if (offs>=0x40000)
	{
		if (used ==0)
		{
			int databooo;
			databooo = (ROM1_shuffled[offs+0] << 8) | ROM1_shuffled[offs+1];

			if (offs & 0x100) databooo ^=0x20;
			if (offs & 0x400) databooo ^=0x40;

	//      printf("offset %08x data %04x\n",offs, databooo);
	//  } else  printf("offset %08x data %02x%02x\n",offs,0,0);
		}
	}


	if (used == 0)
	{
		blankblocks++;

//      printf("%08x , %02x%02x , \n",offs,ROM1_shuffled[offs+0],ROM1_shuffled[offs+1]);
//      printf("offset %08x blanks %d\n",offs,blankblocks);


		for(y=0;y<0x10000;y++)
		{
			if (counttable[y]!=0)
			{
		//      printf("word %04x count %d\n",y,counttable[y]);
			}

		}


	}

	if (used==0)
	{
		for(y=0;y<0x100;y++)
		{
			tmp[offs+y]=ROM1_shuffled[offs+y];
		}
	}
	else
	{
		for(y=0;y<0x100;y++)
		{
			tmp[offs+y]=0;
		}
	}



}

/* this is based on ~256 bytes that change in nslasher compared with nslasherj
   in the 0x80000-0xbffff region */
static void buildreversetable1(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr = BITSWAP24(i,23,22,21,20,
		                   19,18,7, 6,
	  	                   5, 4, 3, 2,
	  	                   11,10,9, 8,
	  	                   17,16,15,14,
						   13,12,1, 0  );


	 	xor = 0x0047c;

	  	if (addr & 0x01000) xor ^= 0x009ac;
	  	if (addr & 0x02000) xor ^= 0x00564;
	  	if (addr & 0x04000) xor ^= 0x00f70;
 	 	if (addr & 0x08000) xor ^= 0x004a8;
	  	if (addr & 0x10000) xor ^= 0x00a00;
	  	if (addr & 0x20000) xor ^= 0x00fe0;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable3(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr = BITSWAP24(i,23,22,21,20,
		                   19,18,17,16,
	  	                   15,14,13,12,
	  	                   8,10,9, 11,
	  	                   7, 6, 5, 4,
						   3, 2, 1, 0  );

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}
static void buildnewreversetable2(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable4(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00400) xor ^= 0x34900;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable5(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr = BITSWAP24(i,23,22,21,20,
		                   19,18,17,16,
	  	                   15,14,13,12,
	  	                   11,9,10, 8,
	  	                   7, 6, 5, 4,
						   3, 2, 1, 0  );

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable6(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00400) xor ^= 0x30800;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable7(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00800) xor ^= 0x12000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}


static void buildnewreversetable8(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x01000) xor ^= 0x18000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}


static void buildnewreversetable9(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x02000) xor ^= 0x30000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable10(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00400) xor ^= 0x3000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable11(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00100) xor ^= 0x4400;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable12(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

//      xor = 0x0047c;
		xor=0;

	  	if (addr & 0x00200) xor ^= 0x4000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable13(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;

	 	xor = 0x00500;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

/* heh.. */
static void buildnewreversetable14(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x00800) xor ^= 0x0400;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable15(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x00800) xor ^= 0x0100;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}


static void buildnewreversetable16(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x00100) xor ^= 0x4000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable17(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x01000) xor ^= 0x0100;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable18(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x01000) xor ^= 0x0f00;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable19(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x01000) xor ^= 0x0100;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable20(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x00100) xor ^= 0x7000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable21(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	xor ^= 0x34000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable22(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x01000) xor ^= 0x2000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable23(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x04000) xor ^= 0x8000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable24(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr = BITSWAP24(i,23,22,21,20,
		                   19,18,17,16,
	  	                   12,13,14,15,
	  	                   11,10,9, 8,
	  	                   7, 6, 5, 4,
						   3, 2, 1, 0  );

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable25(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x01000) xor ^= 0x8000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable26(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	xor ^= 0x4000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable27(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr=i;
		xor = 0;

	  	if (addr & 0x02000) xor ^= 0x1000;


		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable28(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

		addr = BITSWAP24(i,23,22,21,20,
		                   19,18,17,16,
	  	                   15,14,12,13,
	  	                   11,10,9, 8,
	  	                   7, 6, 5, 4,
						   3, 2, 1, 0  );

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable29(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

unsigned char swapdata[0x80] = {

0x00,0x01,0x02,0x03 , 0x80,0x81,0x82,0x83 , 0x10,0x11,0x12,0x13, 0x90,0x91,0x92,0x93,
0x44,0x45,0x46,0x47 , 0xc4,0xc5,0xc6,0xc7 , 0x54,0x55,0x56,0x57, 0xd4,0xd5,0xd6,0xd7,

0x20,0x21,0x22,0x23 , 0xa0,0xa1,0xa2,0xa3 , 0x30,0x31,0x32,0x33, 0xb0,0xb1,0xb2,0xb3,
0x64,0x65,0x66,0x67 , 0xe4,0xe5,0xe6,0xe7 , 0x74,0x75,0x76,0x77, 0xf4,0xf5,0xf6,0xf7,

0xec,0xed,0xee,0xef , 0x6c,0x6d,0x6e,0x6f , 0xfc,0xfd,0xfe,0xff, 0x7c,0x7d,0x7e,0x7f,
0xa8,0xa9,0xaa,0xab , 0x28,0x29,0x2a,0x2b , 0xb8,0xb9,0xba,0xbb, 0x38,0x39,0x3a,0x3b,

0xcc,0xcd,0xce,0xcf , 0x4c,0x4d,0x4e,0x4f , 0xdc,0xdd,0xde,0xdf, 0x5c,0x5d,0x5e,0x5f,
0x88,0x89,0x8a,0x8b , 0x08,0x09,0x0a,0x0b , 0x98,0x99,0x9a,0x9b, 0x18,0x19,0x1a,0x1b,
};

		int newaddr;

		newaddr = swapdata[i&0x7f];
		if (i&0x80) newaddr ^=0x40;

		addr = (i & 0xffff00)|(newaddr&0xfc);

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}


static void buildnewreversetable30(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;


unsigned char swapdata[0x80] = {

  0x04,0x04,0x04,0x04, 0x64,0x64,0x64,0x04, 0x00,0x00,0x00,0x00, 0x60,0x60,0x60,0x00,
  0x0c,0x0c,0x0c,0x0c, 0x6c,0x6c,0x6c,0x6c, 0x08,0x08,0x08,0x08, 0x68,0x68,0x68,0x68,
  0x14,0x14,0x14,0x14, 0x74,0x74,0x74,0x14, 0x10,0x10,0x10,0x10, 0x70,0x70,0x70,0x10,
  0x1c,0x1c,0x1c,0x1c, 0x7c,0x7c,0x7c,0x7c, 0x18,0x18,0x18,0x18, 0x78,0x78,0x78,0x78,

  0x24,0x24,0x24,0x04, 0x44,0x44,0x44,0x04, 0x20,0x20,0x20,0x00, 0x40,0x40,0x40,0x00,
  0x2c,0x2c,0x2c,0x2c, 0x4c,0x4c,0x4c,0x4c, 0x28,0x28,0x28,0x28, 0x48,0x48,0x48,0x48,
  0x34,0x34,0x34,0x14, 0x54,0x54,0x54,0x14, 0x30,0x30,0x30,0x10, 0x50,0x50,0x50,0x10,
  0x3c,0x3c,0x3c,0x3c, 0x5c,0x5c,0x5c,0x5c, 0x38,0x38,0x38,0x38, 0x58,0x58,0x58,0x58,
 };


		int newaddr;

		newaddr = swapdata[i&0x7f];

		addr = (i & 0xffff80)|(newaddr&0x7c);

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}


static void buildnewreversetable31(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;

/*
unsigned char swapdata[0x80] = {

  0x00,0x00,0x00,0x00, 0x04,0x04,0x04,0x04, 0x08,0x08,0x08,0x08, 0x0c,0x0c,0x0c,0x0c,
  0x10,0x10,0x10,0x10, 0x14,0x14,0x14,0x14, 0x18,0x18,0x18,0x18, 0x1c,0x1c,0x1c,0x1c,
  0x20,0x20,0x20,0x20, 0x24,0x24,0x24,0x24, 0x28,0x28,0x28,0x28, 0x2c,0x2c,0x2c,0x2c,
  0x30,0x30,0x30,0x30, 0x34,0x34,0x34,0x34, 0x38,0x38,0x38,0x38, 0x3c,0x3c,0x3c,0x3c,

  0x40,0x40,0x40,0x40, 0x44,0x44,0x44,0x44, 0x48,0x48,0x48,0x48, 0x4c,0x4c,0x4c,0x4c,
  0x50,0x50,0x50,0x50, 0x54,0x54,0x54,0x54, 0x58,0x58,0x58,0x58, 0x5c,0x5c,0x5c,0x5c,
  0x60,0x60,0x60,0x60, 0x64,0x64,0x64,0x64, 0x68,0x68,0x68,0x68, 0x6c,0x6c,0x6c,0x6c,
  0x70,0x70,0x70,0x70, 0x74,0x74,0x74,0x74, 0x78,0x78,0x78,0x78, 0x7c,0x7c,0x7c,0x7c,
 };
*/

unsigned char swapdata[0x80] = {

  0x00,0x00,0x00,0x00, 0x04,0x04,0x04,0x04, 0x08,0x08,0x08,0x08, 0x0c,0x0c,0x0c,0x0c,
  0x10,0x10,0x10,0x10, 0x14,0x14,0x14,0x14, 0x18,0x18,0x18,0x18, 0x1c,0x1c,0x1c,0x1c,
  0x20,0x20,0x20,0x20, 0x24,0x24,0x24,0x24, 0x28,0x28,0x28,0x28, 0x2c,0x2c,0x2c,0x2c,
  0x30,0x30,0x30,0x30, 0x34,0x34,0x34,0x34, 0x38,0x38,0x38,0x38, 0x3c,0x3c,0x3c,0x3c,

  0x44,0x44,0x44,0x44, 0x40,0x40,0x40,0x40, 0x4c,0x4c,0x4c,0x4c, 0x48,0x48,0x48,0x48,
  0x54,0x54,0x54,0x54, 0x50,0x50,0x50,0x50 ,0x5c,0x5c,0x5c,0x5c, 0x58,0x58,0x58,0x58,
  0x64,0x64,0x64,0x64, 0x60,0x60,0x60,0x60, 0x6c,0x6c,0x6c,0x6c, 0x68,0x68,0x68,0x68,
  0x74,0x74,0x74,0x74, 0x70,0x70,0x70,0x70, 0x7c,0x7c,0x7c,0x7c, 0x78,0x78,0x78,0x78,
 };



		int newaddr;

		newaddr = swapdata[i&0x7f];

		addr = (i & 0xffff80)|(newaddr&0x7c);

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable32(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;


unsigned char swapdata[0x100] = {

  0x0c,0x0c,0x0c,0x0c, 0x08,0x08,0x08,0x08, 0x04,0x04,0x04,0x04, 0x00,0x00,0x00,0x00,
  0x1c,0x1c,0x1c,0x1c, 0x18,0x18,0x18,0x18, 0x14,0x14,0x14,0x14, 0x10,0x10,0x10,0x10,
  0x2c,0x2c,0x2c,0x2c, 0x28,0x28,0x28,0x28, 0x24,0x24,0x24,0x24, 0x20,0x20,0x20,0x20,
  0x3c,0x3c,0x3c,0x3c, 0x38,0x38,0x38,0x38, 0x34,0x34,0x34,0x34, 0x30,0x30,0x30,0x30,
  0x4c,0x4c,0x4c,0x4c, 0x48,0x48,0x48,0x48, 0x44,0x44,0x44,0x44, 0x40,0x40,0x40,0x40,
  0x5c,0x5c,0x5c,0x5c, 0x58,0x58,0x58,0x58, 0x54,0x54,0x54,0x54, 0x50,0x50,0x50,0x50,
  0x6c,0x6c,0x6c,0x6c, 0x68,0x68,0x68,0x68, 0x64,0x64,0x64,0x64, 0x60,0x60,0x60,0x60,
  0x7c,0x7c,0x7c,0x7c, 0x78,0x78,0x78,0x78, 0x74,0x74,0x74,0x74, 0x70,0x70,0x70,0x70,
  0x80,0x80,0x80,0x80, 0x84,0x84,0x84,0x84, 0x88,0x88,0x88,0x88, 0x8c,0x8c,0x8c,0x8c,
  0x90,0x90,0x90,0x90, 0x94,0x94,0x94,0x94, 0x98,0x98,0x98,0x98, 0x9c,0x9c,0x9c,0x9c,
  0xa0,0xa0,0xa0,0xa0, 0xa4,0xa4,0xa4,0xa4, 0xa8,0xa8,0xa8,0xa8, 0xac,0xac,0xac,0xac,
  0xb0,0xb0,0xb0,0xb0, 0xb4,0xb4,0xb4,0xb4, 0xb8,0xb8,0xb8,0xb8, 0xbc,0xbc,0xbc,0xbc,
  0xc0,0xc0,0xc0,0xc0, 0xc4,0xc4,0xc4,0xc4, 0xc8,0xc8,0xc8,0xc8, 0xcc,0xcc,0xcc,0xcc,
  0xd0,0xd0,0xd0,0xd0, 0xd4,0xd4,0xd4,0xd4, 0xd8,0xd8,0xd8,0xd8, 0xdc,0xdc,0xdc,0xdc,
  0xe0,0xe0,0xe0,0xe0, 0xe4,0xe4,0xe4,0xe4, 0xe8,0xe8,0xe8,0xe8, 0xec,0xec,0xec,0xec,
  0xf0,0xf0,0xf0,0xf0, 0xf4,0xf4,0xf4,0xf4, 0xf8,0xf8,0xf8,0xf8, 0xfc,0xfc,0xfc,0xfc,


 };



		int newaddr;

		newaddr = swapdata[i&0xff];

		addr = (i & 0xffff00)|(newaddr&0xfc);

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void buildnewreversetable33(void)
{
	static int i;

	for (i=0;i<0x40000;i+=4)
	{
		int addr;
		int xor;


unsigned char swapdata[0x100] = {

  0x20,0x20,0x20,0x20, 0x24,0x24,0x24,0x24, 0x28,0x28,0x28,0x28, 0x2c,0x2c,0x2c,0x2c,
  0x30,0x30,0x30,0x30, 0x34,0x34,0x34,0x34, 0x38,0x38,0x38,0x38, 0x3c,0x3c,0x3c,0x3c,
  0x00,0x00,0x00,0x00, 0x04,0x04,0x04,0x04, 0x08,0x08,0x08,0x08, 0x0c,0x0c,0x0c,0x0c,
  0x10,0x10,0x10,0x10, 0x14,0x14,0x14,0x14, 0x18,0x18,0x18,0x18, 0x1c,0x1c,0x1c,0x1c,

  0x60,0x60,0x60,0x60, 0x64,0x64,0x64,0x64, 0x68,0x68,0x68,0x68, 0x6c,0x6c,0x6c,0x6c,
  0x70,0x70,0x70,0x70, 0x74,0x74,0x74,0x74, 0x78,0x78,0x78,0x78, 0x7c,0x7c,0x7c,0x7c,
  0x40,0x40,0x40,0x40, 0x44,0x44,0x44,0x44, 0x48,0x48,0x48,0x48, 0x4c,0x4c,0x4c,0x4c,
  0x50,0x50,0x50,0x50, 0x54,0x54,0x54,0x54, 0x58,0x58,0x58,0x58, 0x5c,0x5c,0x5c,0x5c,

  0x80,0x80,0x80,0x80, 0x84,0x84,0x84,0x84, 0x88,0x88,0x88,0x88, 0x8c,0x8c,0x8c,0x8c,
  0x90,0x90,0x90,0x90, 0x94,0x94,0x94,0x94, 0x98,0x98,0x98,0x98, 0x9c,0x9c,0x9c,0x9c,
  0xa0,0xa0,0xa0,0xa0, 0xa4,0xa4,0xa4,0xa4, 0xa8,0xa8,0xa8,0xa8, 0xac,0xac,0xac,0xac,
  0xb0,0xb0,0xb0,0xb0, 0xb4,0xb4,0xb4,0xb4, 0xb8,0xb8,0xb8,0xb8, 0xbc,0xbc,0xbc,0xbc,
  0xc0,0xc0,0xc0,0xc0, 0xc4,0xc4,0xc4,0xc4, 0xc8,0xc8,0xc8,0xc8, 0xcc,0xcc,0xcc,0xcc,
  0xd0,0xd0,0xd0,0xd0, 0xd4,0xd4,0xd4,0xd4, 0xd8,0xd8,0xd8,0xd8, 0xdc,0xdc,0xdc,0xdc,
  0xe0,0xe0,0xe0,0xe0, 0xe4,0xe4,0xe4,0xe4, 0xe8,0xe8,0xe8,0xe8, 0xec,0xec,0xec,0xec,
  0xf0,0xf0,0xf0,0xf0, 0xf4,0xf4,0xf4,0xf4, 0xf8,0xf8,0xf8,0xf8, 0xfc,0xfc,0xfc,0xfc,

 };



		int newaddr;

		newaddr = swapdata[i&0xff];

		addr = (i & 0xffff00)|(newaddr&0xfc);

//      xor = 0x0047c;
		xor=0;

//      if (addr & 0x00800) xor ^= 0x00700;

		addr = addr^xor;

		reversetable[i>>2]=addr>>2;
	}
}

static void processregion1_apply_data_xor(void)
{
	int x;
//  tmp = malloc(romregionsize);

	for (x=0;x< romregionsize;x++)
	{
		//int lookupaddr;
		int addres;
		int xor;
	//  lookupaddr = (x&0x3fffc);
	//  lookupaddr = lookupaddr >>2;
	//  addres = reversetable[lookupaddr];
	//  addres = addres << 2;
	//  addres |= (x&0xc0000);
unsigned char xordata[0x100] = {

0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0x04, 0x00,   0x00, 0x80, 0x04, 0x00,  0x00, 0x80, 0x04, 0x00,  0x00, 0x80, 0x04, 0x00,
0x04, 0x80, 0x04, 0x40,   0x04, 0x80, 0x04, 0x40,  0x04, 0x80, 0x04, 0x40,  0x04, 0x80, 0x04, 0x40,
0x04, 0x00, 0x00, 0x40,   0x04, 0x00, 0x00, 0x40,  0x04, 0x00, 0x00, 0x40,  0x04, 0x00, 0x00, 0x40,
0x00, 0x00, 0x40, 0x04,   0x00, 0x00, 0x40, 0x04,  0x00, 0x00, 0x40, 0x04,  0x00, 0x00, 0x40, 0x04,
0x00, 0x80, 0x44, 0x04,   0x00, 0x80, 0x44, 0x04,  0x00, 0x80, 0x44, 0x04,  0x00, 0x80, 0x44, 0x04,
0x04, 0x80, 0x44, 0x44,   0x04, 0x80, 0x44, 0x44,  0x04, 0x80, 0x44, 0x44,  0x04, 0x80, 0x44, 0x44,
0x04, 0x00, 0x40, 0x44,   0x04, 0x00, 0x40, 0x44,  0x04, 0x00, 0x40, 0x44,  0x04, 0x00, 0x40, 0x44,

0x80, 0x02, 0x00, 0x00,   0x80, 0x02, 0x00, 0x00,  0x80, 0x02, 0x00, 0x00,  0x80, 0x02, 0x00, 0x00,
0x80, 0x82, 0x04, 0x00,   0x80, 0x82, 0x04, 0x00,  0x80, 0x82, 0x04, 0x00,  0x80, 0x82, 0x04, 0x00,
0x84, 0x82, 0x04, 0x40,   0x84, 0x82, 0x04, 0x40,  0x84, 0x82, 0x04, 0x40,  0x84, 0x82, 0x04, 0x40,
0x84, 0x02, 0x00, 0x40,   0x84, 0x02, 0x00, 0x40,  0x84, 0x02, 0x00, 0x40,  0x84, 0x02, 0x00, 0x40,
0x80, 0x02, 0x40, 0x04,   0x80, 0x02, 0x40, 0x04,  0x80, 0x02, 0x40, 0x04,  0x80, 0x02, 0x40, 0x04,
0x80, 0x82, 0x44, 0x04,   0x80, 0x82, 0x44, 0x04,  0x80, 0x82, 0x44, 0x04,  0x80, 0x82, 0x44, 0x04,
0x84, 0x82, 0x44, 0x44,   0x84, 0x82, 0x44, 0x44,  0x84, 0x82, 0x44, 0x44,  0x84, 0x82, 0x44, 0x44,
0x84, 0x02, 0x40, 0x44,   0x84, 0x02, 0x40, 0x44,  0x84, 0x02, 0x40, 0x44,  0x84, 0x02, 0x40, 0x44,
};

		addres = x;
		xor = xordata[addres&0xff];

		ROM1_shuffled[addres] = ROM1_shuffled[addres] ^ xor;


	//  tmp[x]  =(ROM1_shuffled[addres+0]);
	//  tmp[x+1]=(ROM1_shuffled[addres+1]);
	//  tmp[x+2]=(ROM1_shuffled[addres+2]);
	//  tmp[x+3]=(ROM1_shuffled[addres+3]);

	}
//  memcpy(ROM1_shuffled,tmp, romregionsize);

	free(tmp);
}

static void processregion1_apply_data_xor2(void)
{
	int x;
//  tmp = malloc(romregionsize);

	for (x=0;x< romregionsize;x++)
	{
		//int lookupaddr;
		int addres;
		int xor;
	//  lookupaddr = (x&0x3fffc);
	//  lookupaddr = lookupaddr >>2;
	//  addres = reversetable[lookupaddr];
	//  addres = addres << 2;
	//  addres |= (x&0xc0000);
unsigned char xordata[0x10] = {

0x00, 0x08, 0x80, 0x00,   0x00, 0x08, 0x80, 0x00,  0x00, 0x08, 0x80, 0x00,  0x00, 0x08, 0x80, 0x00,

};

		addres = x;
		xor = 0;


		if (addres&0x80000) xor = xordata[addres&0x0f];

		ROM1_shuffled[addres] = ROM1_shuffled[addres] ^ xor;


	//  tmp[x]  =(ROM1_shuffled[addres+0]);
	//  tmp[x+1]=(ROM1_shuffled[addres+1]);
	//  tmp[x+2]=(ROM1_shuffled[addres+2]);
	//  tmp[x+3]=(ROM1_shuffled[addres+3]);

	}
//  memcpy(ROM1_shuffled,tmp, romregionsize);

	free(tmp);
}

static void processregion1_apply_data_xor3(void)
{
	int x;
//  tmp = malloc(romregionsize);

	for (x=0;x< romregionsize;x++)
	{
		//int lookupaddr;
		int addres;
		int xor;
	//  lookupaddr = (x&0x3fffc);
	//  lookupaddr = lookupaddr >>2;
	//  addres = reversetable[lookupaddr];
	//  addres = addres << 2;
	//  addres |= (x&0xc0000);
unsigned char xordata[0x10] = {

0x00, 0x00, 0x10, 0x02,  0x00, 0x00, 0x10, 0x02,  0x00, 0x00, 0x10, 0x02,   0x00, 0x00, 0x10, 0x02,

};

		addres = x;
		xor = 0;


		if (addres&0x40000) xor = xordata[addres&0x0f];

		ROM1_shuffled[addres] = ROM1_shuffled[addres] ^ xor;


	//  tmp[x]  =(ROM1_shuffled[addres+0]);
	//  tmp[x+1]=(ROM1_shuffled[addres+1]);
	//  tmp[x+2]=(ROM1_shuffled[addres+2]);
	//  tmp[x+3]=(ROM1_shuffled[addres+3]);

	}
//  memcpy(ROM1_shuffled,tmp, romregionsize);

	free(tmp);
}

static void scanforblankscpu1(void)
{
	int x;
	int blockcount;

	tmp = malloc( romregionsize);
	blockcount=0;

	for(x=0;x< romregionsize;x+=0x100)
	{

		wordcounter(x);
#if 0
		int y;
		int usedcount;

		usedcount = 0;

		for(y=0;y<0x100;y+=8)
		{
			int aa;
			/* not foolproof but common to a fair number of blank blocks*/
			aa = (ROM1_shuffled[x+y+0]&0x0f) ^ (ROM1_shuffled[x+y+4]&0x0f);
			if (aa!=0x0c) usedcount++;

		}

		if (usedcount==0)
		{
			for(y=0;y<0x100;y++)
			{
				tmp[x+y]=ROM1_shuffled[x+y];
			}
			blockcount++;
		}
		else
		{
			for(y=0;y<0x100;y++)
			{
				tmp[x+y]=0;
			}
		}
#endif
	}

	memcpy(ROM1_shuffled,tmp, romregionsize);
	free(tmp);

//  printf("%d suspected blank blocks\n",blockcount);

	{
		FILE *fp;
		sprintf(filename,"%s_suspected_blank_blocks",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		{
			fwrite(ROM1_shuffled,  romregionsize, 1, fp);
			fclose(fp);
		}
	}

}

static void processregion1_again(void)
{
	int x;
	tmp = malloc(romregionsize);

	for (x=0;x< romregionsize;x+=4)
	{
		int lookupaddr;
		int addres;

		lookupaddr = (x&0x3fffc);
		lookupaddr = lookupaddr >>2;
		addres = reversetable[lookupaddr];
		addres = addres << 2;
		addres |= (x&0xc0000);


		tmp[x]  =(ROM1_shuffled[addres+0]);
		tmp[x+1]=(ROM1_shuffled[addres+1]);
		tmp[x+2]=(ROM1_shuffled[addres+2]);
		tmp[x+3]=(ROM1_shuffled[addres+3]);

	}
	memcpy(ROM1_shuffled,tmp, romregionsize);

	free(tmp);
}

static void processregion1(void)
{
	int x;
	data8_t *ROM1, *ROM2;
	ROM1 = memory_region(REGION_CPU1);
	ROM2 = memory_region(REGION_CPU3); /* if we have a rom in region 3 we xor wirh reigon 1 pair for analysis */

	for (x=0;x< romregionsize;x+=4)
	{
		int lookupaddr;
		int addres;

		lookupaddr = (x&0x3fffc);
		lookupaddr = lookupaddr >>2;

		addres = reversetable[lookupaddr];
		addres = addres << 2;

		addres |= (x&0xc0000);

		ROM1_shuffled[x]  =(ROM1[addres+0]);
		ROM1_shuffled[x+1]=(ROM1[addres+1]);
		ROM1_shuffled[x+2]=(ROM1[addres+2]);
		ROM1_shuffled[x+3]=(ROM1[addres+3]);

//          ROM1_shuffled[x]  =(ROM1[addres+0]^ROM2[addres+0]);
//          ROM1_shuffled[x+1]=(ROM1[addres+1]^ROM2[addres+1]);
//          ROM1_shuffled[x+2]=(ROM1[addres+2]^ROM2[addres+2]);
//          ROM1_shuffled[x+3]=(ROM1[addres+3]^ROM2[addres+3]);


		if (ROM2)
		{
			ROM2_shuffled[x]  =(ROM2[addres+0]);
			ROM2_shuffled[x+1]=(ROM2[addres+1]);
			ROM2_shuffled[x+2]=(ROM2[addres+2]);
			ROM2_shuffled[x+3]=(ROM2[addres+3]);

	//      ROM1_shuffled[x]  =(ROM1[addres+0]^ROM2[addres+0]);
	//      ROM1_shuffled[x+1]=(ROM1[addres+1]^ROM2[addres+1]);
	//      ROM1_shuffled[x+2]=(ROM1[addres+2]^ROM2[addres+2]);
	//      ROM1_shuffled[x+3]=(ROM1[addres+3]^ROM2[addres+3]);
		}
	}

	/* dump a couple of things to file */

	{
		FILE *fp;
		sprintf(filename,"%s_romregion1",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(ROM1,  romregionsize, 1, fp);
			fclose(fp);
		}
	}

	if (ROM2)
	{
		FILE *fp;
		sprintf(filename,"%s_romregion2",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(ROM2,  romregionsize, 1, fp);
			fclose(fp);
		}
	}

	{
		FILE *fp;
		sprintf(filename,"%s_romregion1_shuffled",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(ROM1_shuffled,  romregionsize, 1, fp);
			fclose(fp);
		}
	}

	if (ROM2)
	{
		FILE *fp;
		sprintf(filename,"%s_romregion2_shuffled",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		{
			fwrite(ROM2_shuffled,  romregionsize, 1, fp);
			fclose(fp);
		}
	}

	if (ROM2)
	{
		FILE *fp;
		sprintf(filename,"%s_romregion1_and_2_shuffled_and_xor",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		{
			fwrite(ROM1_ROM2_shuffled_xored,  romregionsize, 1, fp);
			fclose(fp);
		}
	}
}

void decrypt156(void)
{
	/* analysis */
	printf("Performing Analysis on CPU roms, decryption still unknown, this does not decrypt anything!\n");
	romregionsize=memory_region_length(REGION_CPU1);
	ROM1_ROM2_shuffled_xored=auto_malloc(romregionsize);
	ROM1_shuffled=auto_malloc(romregionsize);
	ROM2_shuffled=auto_malloc(romregionsize);
	buildreversetable1();
	processregion1();
	buildnewreversetable2();
	processregion1_again();
	buildnewreversetable3();
	processregion1_again();
	buildnewreversetable4();
	processregion1_again();
	buildnewreversetable5();
	processregion1_again();
	buildnewreversetable6();
	processregion1_again();
	buildnewreversetable7();
	processregion1_again();
	buildnewreversetable8();
	processregion1_again();
	buildnewreversetable9();
	processregion1_again();
	buildnewreversetable10();
	processregion1_again();
	buildnewreversetable11();
	processregion1_again();
	buildnewreversetable12();
	processregion1_again();
	buildnewreversetable13();
	processregion1_again();
	buildnewreversetable14();
	processregion1_again();
	buildnewreversetable15();
	processregion1_again();
	buildnewreversetable16();
	processregion1_again();
	buildnewreversetable17();
	processregion1_again();
	buildnewreversetable18();
	processregion1_again();
	buildnewreversetable19();
	processregion1_again();
	buildnewreversetable20();
	processregion1_again();
	buildnewreversetable21();
	processregion1_again();
	buildnewreversetable22();
	processregion1_again();
	buildnewreversetable23();
	processregion1_again();
	buildnewreversetable24();
	processregion1_again();
	buildnewreversetable25();
	processregion1_again();
	buildnewreversetable26();
	processregion1_again();
	buildnewreversetable27();
	processregion1_again();
	buildnewreversetable28();
	processregion1_again();
	buildnewreversetable29();
	processregion1_again();
	buildnewreversetable30();
	processregion1_again();
	buildnewreversetable31();
	processregion1_again();
	buildnewreversetable32();
	processregion1_again();
	buildnewreversetable33();
	processregion1_again();
	processregion1_apply_data_xor();
	processregion1_apply_data_xor2();
	processregion1_apply_data_xor3();
	//  buildnewreversetable34();
//  processregion1_again();

	{
		FILE *fp;
		sprintf(filename,"%s_romregion1_shuffled",Machine->gamedrv->name);
		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(ROM1_shuffled,  romregionsize, 1, fp);
			fclose(fp);
		}
	}


	scanforblankscpu1();
}

