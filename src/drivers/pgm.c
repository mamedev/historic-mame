
/* PGM System (c)1997 IGS

Based on Information from ElSemi

A flexible cartridge based platform some would say was designed to compete with
SNK's NeoGeo and Capcom's CPS Hardware systems, despite its age it only uses a
68000 for the main processor and a Z80 to drive the sound, just like the two
previously mentioned systems in that respect..

Resolution is 448x224, 15 bit colour

Sound system is ICS WaveFront 2115 Wavetable midi syntesizer, used in some
actual sound cards (Turtle Beach)

The later games would appear to be Encrypted

Roms Contain the Following Data

Pxxxx - 68k Program
Txxxx - TX & BG Graphics (2 formats within the same rom)
Mxxxx - Music samples (8 bit mono 11025Hz)
Axxxx - Colour Data (for sprites)
Bxxxx - Masks & A Rom Colour Indexes (for sprites)

There is no rom for the Z80, the program is uploaded by the 68k

Known Games on this Platform
----------------------------

Oriental Legend
Oriental Legend Super
Sengoku Senki / Knights of Valour Series
-
Sangoku Senki (c)1999 IGS
Sangoku Senki Super Heroes (c)1999 IGS
Sangoku Senki 2 Knights of Valour (c)2000 IGS
Sangoku Senki Busyou Souha (c)2001 IGS
-
DoDonPachi II (Bee Storm)
Photo Y2K? (and its sequel?)
Material Artist
Dragon World 2+3
.. lots more ..


To Do / Notes:

sprite zooming (zoom table is contained in vidram)
sprite masking (lower priority sprites under bg layers can mask higher ones) -? no?
calender
better protection emulation in orlegend
hook up the z80 + emulate sound chip (Farfetch'd is working on this)
optimize?
layer enables?
sprites use dma?
verify some things
other 2 interrupts - one is sound rleated *dragon world 2 uses one of them for inputs
the 'encryption' info came from unknown 3rd party
see notes at bottom of driver, protection emulation by ElSemi

Sango / Kov level 2 'bridge' is drawn with bg sprites but needs to appear OVER the bg layer,
ElSemi believes that bg sprites are drawn as fg sprites if the first sprite in the sprite
list doesn't have the bg bit set. -fixed-

the protection determines the region in both the supported games, however the 'china' board
of orlegend is the same revision one of the other sets, but of an earlier build date.

the current 'kov' sets were from 'sango' boards but the protection determines the region so
it makes more sense to name them kov since the roms are probably the same on the various
boards.  The current sets were taken from taiwan boards incase somebody finds
it not to be the case however due to the previous note.

the dragon world 2 set is china only (whats the real chinese name?) there is an english
version, however this set doesn't allow for that without hacks at least.

dragon world 2 still has strange protection issues, we have to patch the code for now, what
should really happen, it jumps to invalid code, should the protection device cause the 68k
to see valid code there or something?

kov superheroes uses a different protection chip / different protection commands and doesn't
work, some of the gfx also need redumping to check they're the same as kov, its using invalid
codes for the ones we have (could just be protection tho)

*/

#include "driver.h"

data16_t *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs, *pgm_rowscrollram;
WRITE16_HANDLER( pgm_tx_videoram_w );
WRITE16_HANDLER( pgm_bg_videoram_w );
VIDEO_START( pgm );
VIDEO_UPDATE( pgm );
void pgm_kov_decrypt(void);
void pgm_kovsh_decrypt(void);
void pgm_dw2_decrypt(void);

READ16_HANDLER ( orlegend_kludge_r )
{
	/* the game reads from within this address range, starting at 0x820000,
	and increasing to 0x??????, and seems to corrupt ram based on the result? */

	return 0x2;
}

/*** Memory Maps *************************************************************/

static MEMORY_READ16_START( pgm_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },   /* BIOS ROM */
	{ 0x100000, 0x5fffff, MRA16_BANK1 }, /* Game ROM */

	{ 0x800000, 0x81ffff, MRA16_RAM }, /* Main Ram, Sprites, SRAM? */

	{ 0x820000, 0x82ffff, orlegend_kludge_r }, /* protection ??? */

	{ 0x900000, 0x903fff, MRA16_RAM }, /* Backgrounds */
	{ 0x904000, 0x905fff, MRA16_RAM }, /* Text Layer */
	{ 0x907000, 0x9077ff, MRA16_RAM },
	{ 0xa00000, 0xa011ff, MRA16_RAM }, /* Palette */
	{ 0xb00000, 0xb0ffff, MRA16_RAM }, /* Video Regs inc. Zoom Table */
	{ 0xc10000, 0xC1ffff, MRA16_RAM }, /* Z80 Program? */

//	{ 0xc00004, 0xc00005, input_port_4_word_r }, // calender stuff?
//	{ 0xc00006, 0xc00007, input_port_5_word_r }, // calender stuff?

	{ 0xC08000, 0xC08001, input_port_0_word_r }, // p1+p2 controls
	{ 0xC08002, 0xC08003, input_port_1_word_r }, // p3+p4 controls
	{ 0xC08004, 0xC08005, input_port_2_word_r }, // extra controls
	{ 0xC08006, 0xC08007, input_port_3_word_r }, // dipswitches
MEMORY_END

static MEMORY_WRITE16_START( pgm_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },   /* BIOS ROM */
	{ 0x100000, 0x5fffff, MWA16_BANK1 }, /* Game ROM */

	{ 0x800000, 0x81ffff, MWA16_RAM, &pgm_mainram },
	{ 0x900000, 0x903fff, pgm_bg_videoram_w, &pgm_bg_videoram },
	{ 0x904000, 0x905fff, pgm_tx_videoram_w, &pgm_tx_videoram },
	{ 0x907000, 0x9077ff, MWA16_RAM, &pgm_rowscrollram },

	{ 0xa00000, 0xa011ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },

	{ 0xb00000, 0xb0ffff, MWA16_RAM, &pgm_videoregs },
	/* 0xb01000 sprite zoom table */
		/*
		32 bit per level
		if bit is 1 and zooming duplicate pixel/line
		if bit is 1 and shrinking, skip pixel/line
		*/
	/* 0xb02000 bg y scroll */
	/* 0xb03000 bg x scroll */
	/* 0xb05000 tx y scroll */
	/* 0xb06000 tx x scroll */
	{ 0xc10000, 0xC1ffff, MWA16_RAM }, // z80
MEMORY_END

/*** ASIC 3 - Protection? ****************************************************/

/* this isn't right, we patch the branch instructions for now */

static READ16_HANDLER( orlegnde_prot_r )
{
	int res;
	res = -1;

	/* Start of Boot Up */
	if (activecpu_get_pc() == 0x145bda) res = 0x80;
	if (activecpu_get_pc() == 0x145c00) res = 0x00; // NOT 0x80 set
	if (activecpu_get_pc() == 0x145c28) res = 0x84;
	if (activecpu_get_pc() == 0x145c54) res = 0x80;
	if (activecpu_get_pc() == 0x145c80) res = 0x04;
	if (activecpu_get_pc() == 0x145caa) res = 0x00; // NOT 0x84 set
	if (activecpu_get_pc() == 0x145cd2) res = 0x20;
	if (activecpu_get_pc() == 0x145cfc) res = 0x00; // NOT 0x20 set

	/* region is supplied by the protection device */
	if (activecpu_get_pc() == 0x145D8E) res = (readinputport(4) & 1) << 3;
	if (activecpu_get_pc() == 0x145D72) res = (readinputport(4) & 2) << 6;


	if (res != -1) logerror("%06x: prot_r %04x\n",activecpu_get_pc(),res);
	else logerror("%06x: prot_r unknown\n",activecpu_get_pc());

	if (res == -1) res = 0x00;

	return res;
}

void remove_orlegend_commonprot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x145D48/2]=0x4e71; mem16[0x145D4a/2]=0x4e71; /* dif to others. */

	mem16[0x145D02/2]=0x4e71; mem16[0x145D04/2]=0x4e71;
	mem16[0x145CDC/2]=0x4e71; mem16[0x145CDE/2]=0x4e71;
	mem16[0x145CB0/2]=0x4e71; mem16[0x145CB2/2]=0x4e71;
	mem16[0x145C8A/2]=0x4e71; mem16[0x145C8C/2]=0x4e71;
	mem16[0x145C5E/2]=0x4e71; mem16[0x145C60/2]=0x4e71;
	mem16[0x145C32/2]=0x4e71; mem16[0x145C34/2]=0x4e71;
	mem16[0x145C06/2]=0x4e71; mem16[0x145C08/2]=0x4e71;
	mem16[0x145BE4/2]=0x4e71; mem16[0x145BE6/2]=0x4e71;
}

void remove_orlegend_prot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x146AE4/2]=0x4e71; mem16[0x146AE6/2]=0x4e71;
	remove_orlegend_commonprot();
}

void remove_orlegnde_prot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x146AF4/2]=0x4e71; mem16[0x146AF6/2]=0x4e71;
	remove_orlegend_commonprot();
}

/*** Knights of Valour / Sango Protection (from ElSemi) (ASIC28) ***/

static unsigned short ASIC28KEY;
static unsigned short ASIC28REGS[10];
static unsigned short ASICPARAMS[256];
static unsigned short ASIC28RCNT=0;
static unsigned int B0TABLE[16]={2,0,1,4,3}; //maps char portraits to tables

//Not sure if BATABLE is complete
static unsigned int BATABLE[0x40]=
	{0x00,0x29,0x2c,0x35,0x3a,0x41,0x4a,0x4e,  //0x00
     0x57,0x5e,0x77,0x79,0x7a,0x7b,0x7c,0x7d, //0x08
     0x7e,0x7f,0x80,0x81,0x82,0x85,0x86,0x87, //0x10
     0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x90,  //0x18
     0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,
     0x9e,0xa3,0xd4,0xa9,0xaf,0xb5,0xbb,0xc1};

static unsigned int E0REGS[16];


READ16_HANDLER (sango_protram_r)
{


	// at offset == 4 is region (supplied by device)
	// 0 = china
	// 1 = taiwan
	// 2 = japan
	// 3 = korea
	// 4 = hong kong
	// 5 = world

	if (offset == 4)	return readinputport(4);

	// otherwise it doesn't seem to use the ram for anything important, we return 0 to avoid test mode corruption
	// kovplus reads from offset 000e a lot ... why?
#ifdef MAME_DEBUG
	usrintf_showmessage ("protection ram r %04x",offset);
#endif
	return 0x0000;
}

READ16_HANDLER (ASIC28_r16)
//unsigned short ASIC28_r16(unsigned int addr)
{

	unsigned int val=(ASIC28REGS[1]<<16)|(ASIC28REGS[0]);

//logerror("Asic28 Read PC = %06x Command = %02x ??\n",activecpu_get_pc(), ASIC28REGS[1]);

	switch(ASIC28REGS[1]&0xff)
	{
		case 0x99:
			val=0x880000;
			break;
		case 0xd0:	//txt palette
			val=0xa01000+(ASIC28REGS[0]<<5);
			break;
		case 0xdc:	//bg palette
			val=0xa00800+(ASIC28REGS[0]<<6);
			break;
		case 0x9d:
		case 0xe0:	//spr palette
			val=0xa00000+((ASIC28REGS[0]&0x1f)<<6);
			break;
		case 0xc0:
			val=0x880000;
			break;
		case 0xc3:	//TXT tile position Uses C0 to select column
			{
				val=0x904000+(ASICPARAMS[0xc0]+ASICPARAMS[0xc3]*64)*4;
			}
			break;
		case 0xcb:
			val=0x880000;
			break;
		case 0xe7:
			val=0x880000;
			break;
		case 0xe5:
			val=0x880000;
			break;
		case 0xB0:
			val=B0TABLE[ASIC28REGS[0]&0xf];
			break;
		case 0xBA:
			val=BATABLE[ASIC28REGS[0]&0x3f];
			if(ASIC28REGS[0]>0x2f)
			{
//				PutMessage("Unmapped BA com, report ElSemi",60);
				usrintf_showmessage	("Unmapped BA com %02x, contact ElSemi / MameDev", ASIC28REGS[0]);
			}
			break;
		case 0xfe:	//todo
			val=0x880000;
			break;
		case 0xfc:	//Adjust damage level to char experience level
			{
			val=(ASICPARAMS[0xfc]*ASICPARAMS[0xfe])>>6;
			break;
			}
		case 0xf8:
			val=E0REGS[ASIC28REGS[0]&0xf]&0xffffff;
			break;
		case 0xcc: //BG
   			{
   	 		int y=ASICPARAMS[0xcc];
    		if(y&0x400)    //y is signed (probably x too and it also applies to TXT, but I've never seen it used)
     			y=-(0x400-(y&0x3ff));
    		val=0x900000+(((ASICPARAMS[0xcb]+(y)*64)*4)/*&0x1fff*/);
   			}
   			break;
		case 0xf0:
			{
				val=0x00C000;
			}
			break;
		case 0xb4:
			{
				int v2=ASIC28REGS[0]&0x0f;
				int v1=(ASIC28REGS[0]&0x0f00)>>8;
//				unsigned short tmp=E0REGS[v2];
				//E0REGS[v2]=E0REGS[v1];
				//E0REGS[v1]=tmp;
				if(ASIC28REGS[0]==0x102)
					E0REGS[1]=E0REGS[0];
				else
					E0REGS[v1]=E0REGS[v2];

				val=0x880000;
			}
			break;
		case 0xd6:	//???? check it
			{
				int v2=ASIC28REGS[0]&0xf;
//				int v1=(ASIC28REGS[0]&0xf0)>>4;
				E0REGS[0]=E0REGS[v2];
				//E0REGS[v2]=0;
				val=0x880000;
			}
			break;
		default:
			{
				val=0x880000;
			}
	}

//	if(addr==0x500000)
	if(offset==0)
	{
		unsigned short d=val&0xffff;
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		d^=realkey;
		return d;
	}
//	else if(addr==0x500002)
	else if(offset==1)
	{
		unsigned short d=val>>16;
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		d^=realkey;
		ASIC28RCNT++;
		ASIC28RCNT&=0xf;	//16 busy states
		if(ASIC28RCNT==0)
		{
			ASIC28KEY+=0x100;
			ASIC28KEY&=0xFF00;
		}
		return d;
	}
	return 0xff;
}

WRITE16_HANDLER (ASIC28_w16)
//void ASIC28_w16(unsigned int addr,unsigned short data)
{
//	if(addr==0x500000)
	if(offset==0)
	{
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		data^=realkey;
		ASIC28REGS[0]=data;
		return;
	}
//	if(addr==0x500002)
	if(offset==1)
	{
		unsigned short realkey;

		ASIC28KEY=data&0xff00;

		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		data^=realkey;
		ASIC28REGS[1]=data;
//		ErrorLogMessage("ASIC28 CMD %X  PARAM %X",ASIC28REGS[1],ASIC28REGS[0]);
		logerror("ASIC28 CMD %04x  PARAM %04x\n",ASIC28REGS[1],ASIC28REGS[0]);

		ASICPARAMS[ASIC28REGS[1]&0xff]=ASIC28REGS[0];
		if(ASIC28REGS[1]==0xE7)
		{
			unsigned int E0R=(ASICPARAMS[0xE7]>>12)&0xf;
			E0REGS[E0R]&=0xffff;
			E0REGS[E0R]|=ASIC28REGS[0]<<16;
		}
		if(ASIC28REGS[1]==0xE5)
		{
			unsigned int E0R=(ASICPARAMS[0xE7]>>12)&0xf;
			E0REGS[E0R]&=0xff0000;
			E0REGS[E0R]|=ASIC28REGS[0];
		}
		ASIC28RCNT=0;
	}
}

/* Dragon World 2 */

#define DW2BITSWAP(s,d,bs,bd)  d=((d&(~(1<<bd)))|(((s>>bs)&1)<<bd))
//Use this handler for reading from 0xd80000-0xd80002
READ16_HANDLER (dw2_d80000_r )
{
//addr&=0xff;
// if(dw2reg<0x20) //NOT SURE!!
	{
		//The value at 0x80EECE is computed in the routine at 0x107c18
		data16_t d=pgm_mainram[0xEECE/2];
		data16_t d2=0;
		d=(d>>8)|(d<<8);
		DW2BITSWAP(d,d2,7 ,0);
		DW2BITSWAP(d,d2,4 ,1);
		DW2BITSWAP(d,d2,5 ,2);
		DW2BITSWAP(d,d2,2 ,3);
		DW2BITSWAP(d,d2,15,4);
		DW2BITSWAP(d,d2,1 ,5);
		DW2BITSWAP(d,d2,10,6);
		DW2BITSWAP(d,d2,13,7);
		// ... missing bitswaps here (8-15) there is not enough data to know them
		// the code only checks the lowest 8 bytes
		return d2;
	}
}


/*** Input Ports *************************************************************/

/* enough for 4 players, the basic dips mapped are listed in the test mode */

INPUT_PORTS_START( pgm )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

/* probably not dsw related anyway
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "4" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )  // Freezes if off?
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "5" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
*/
	PORT_START	/* Region */
	PORT_DIPNAME( 0x0003, 0x0000, "Region" )
	PORT_DIPSETTING(      0x0000, "World" )
//	PORT_DIPSETTING(      0x0001, "World" ) // again?
	PORT_DIPSETTING(      0x0002, "Korea" )
	PORT_DIPSETTING(      0x0003, "China" )
INPUT_PORTS_END

INPUT_PORTS_START( sango )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

/* probably not dsw related anyway
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "4" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )  // Freezes if off?
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "5" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
*/
	PORT_START	/* Region */
	PORT_DIPNAME( 0x000f, 0x0005, "Region" )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan (Alta License)" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, "World" )
INPUT_PORTS_END

/*** GFX Decodes *************************************************************/

/* we can't decode the sprite data like this because it isn't tile based.  the
   data decoded by pgm32_charlayout was rearranged at start-up */

static struct GfxLayout pgm8_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20,16,  28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout pgm32_charlayout =
{
	32,32,
	RGN_FRAC(1,1),
	5,
	{ 3,4,5,6,7 },
	{ 0  , 8 ,16 ,24 ,32 ,40 ,48 ,56 ,
	  64 ,72 ,80 ,88 ,96 ,104,112,120,
	  128,136,144,152,160,168,176,184,
	  192,200,208,216,224,232,240,248 },
	{ 0*256, 1*256, 2*256, 3*256, 4*256, 5*256, 6*256, 7*256,
	  8*256, 9*256,10*256,11*256,12*256,13*256,14*256,15*256,
	 16*256,17*256,18*256,19*256,20*256,21*256,22*256,23*256,
	 24*256,25*256,26*256,27*256,28*256,29*256,30*256,31*256 },
	 32*256
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pgm8_charlayout,    0x800, 32  }, /* 8x8x4 Tiles */
	{ REGION_GFX2, 0, &pgm32_charlayout,   0x400, 32  }, /* 32x32x5 Tiles */
	{ -1 } /* end of array */
};

/*** Machine Driver **********************************************************/

static INTERRUPT_GEN( pgm_interrupt ) {
	if( cpu_getiloops() == 0 )
		cpu_set_irq_line(0, 6, HOLD_LINE);
	else
		cpu_set_irq_line(0, 4, HOLD_LINE);
}

static MACHINE_DRIVER_START( pgm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000) /* 20 mhz! verified on real board */
	MDRV_CPU_MEMORY(pgm_readmem,pgm_writemem)
	MDRV_CPU_VBLANK_INT(pgm_interrupt,2)

	/* theres also a z80, program is uploaded by the 68k */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 56*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1200/2)

	MDRV_VIDEO_START(pgm)
	MDRV_VIDEO_UPDATE(pgm)
MACHINE_DRIVER_END

/*** Init Stuff **************************************************************/

/* This function expands the 32x32 5-bit data into a format which is easier to
   decode in MAME */

static void expand_32x32x5bpp(void)
{
	data8_t *src    = memory_region       ( REGION_GFX1 );
	data8_t *dst    = memory_region       ( REGION_GFX2 );
	size_t  srcsize = memory_region_length( REGION_GFX1 );
	int cnt, pix;

	for (cnt = 0; cnt < srcsize/5 ; cnt ++)
	{
		pix =  ((src[0+5*cnt] >> 0)& 0x1f );							  dst[0+8*cnt]=pix;
		pix =  ((src[0+5*cnt] >> 5)& 0x07) | ((src[1+5*cnt] << 3) & 0x18);dst[1+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 2)& 0x1f );		 					  dst[2+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 7)& 0x01) | ((src[2+5*cnt] << 1) & 0x1e);dst[3+8*cnt]=pix;
		pix =  ((src[2+5*cnt] >> 4)& 0x0f) | ((src[3+5*cnt] << 4) & 0x10);dst[4+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 1)& 0x1f );							  dst[5+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 6)& 0x03) | ((src[4+5*cnt] << 2) & 0x1c);dst[6+8*cnt]=pix;
		pix =  ((src[4+5*cnt] >> 3)& 0x1f );							  dst[7+8*cnt]=pix;
	}
}

/* This function expands the sprite colour data (in the A Roms) from 3 pixels
   in each word to a byte per pixel making it easier to use */

static void expand_colourdata(void)
{
	data8_t *src    = memory_region       ( REGION_GFX3 );
	data8_t *dst    = memory_region       ( REGION_GFX4 );
	size_t  srcsize = memory_region_length( REGION_GFX3 );
	int cnt;

		for (cnt = 0 ; cnt < srcsize/2 ; cnt++)
		{
			data16_t colpack;

			colpack = ((src[cnt*2]) | (src[cnt*2+1] << 8));

			dst[cnt*3+0] = (colpack >> 0 ) & 0x1f;
			dst[cnt*3+1] = (colpack >> 5 ) & 0x1f;
			dst[cnt*3+2] = (colpack >> 10) & 0x1f;
		}
}

/* Oriental Legend INIT */

READ16_HANDLER ( orlegend_speedup )
{
	if (activecpu_get_pc()==0x104DD2) cpu_spinuntil_int();
	return pgm_mainram[0x00a70e/2];
}

static DRIVER_INIT( orlegend )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

	install_mem_read16_handler(0, 0xC0400e, 0xC0400f, orlegnde_prot_r);
	install_mem_read16_handler(0, 0x80a70e, 0x80a70f, orlegend_speedup);

	remove_orlegend_prot(); /* removes  bsr 145b66 */
}

static DRIVER_INIT( orlegnde )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

	install_mem_read16_handler(0, 0xC0400e, 0xC0400f, orlegnde_prot_r);
	install_mem_read16_handler(0, 0x80a70e, 0x80a70f, orlegend_speedup);

	remove_orlegnde_prot(); /* removes  bsr 145b66 */
}

static DRIVER_INIT( dragwld2 )
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

	expand_32x32x5bpp();
	expand_colourdata();

 	pgm_dw2_decrypt();

/* this seems to be some strange protection, its not right yet ..

<ElSemi> patch the jmp (a0) to jmp(a3) (otherwise they jump to illegal code)
<ElSemi> there are 3 (consecutive functions)
<ElSemi> 303bc
<ElSemi> 30462
<ElSemi> 304f2

*/
	mem16[0x1303bc/2]=0x4e93;
	mem16[0x130462/2]=0x4e93;
	mem16[0x1304F2/2]=0x4e93;

/*

<ElSemi> Here is how to "bypass" the dw2 hang protection, it fixes the mode
select and after failing in the 2nd stage (probably there are other checks
out there).

*/
	install_mem_read16_handler(0, 0xd80000, 0xd80003, dw2_d80000_r);

}

static DRIVER_INIT( kov )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

	install_mem_read16_handler(0, 0x500000, 0x500003, ASIC28_r16);
	install_mem_write16_handler(0, 0x500000, 0x500003, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	install_mem_read16_handler(0, 0x4f0000, 0x4fffff, sango_protram_r);

 	pgm_kov_decrypt();
}

static DRIVER_INIT( kovsh )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

	install_mem_read16_handler(0, 0x500000, 0x500003, ASIC28_r16);
	install_mem_write16_handler(0, 0x500000, 0x500003, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
	  the protection device provides the region code */
	install_mem_read16_handler(0, 0x4f0000, 0x4fffff, sango_protram_r);

 	pgm_kovsh_decrypt();
}

/*** Rom Loading *************************************************************/

/* take note of REGION_GFX2 needed for expanding the 32x32x5bpp data and
   REGION_GFX4 needed for expanding the Sprite Colour Data */

/* The Bios - NOT A GAME */
ROM_START( pgm )
	ROM_REGION( 0x520000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x00000, 0x20000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* 8x8 Text Layer Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) )
ROM_END

ROM_START( orlegend )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0103.rom",    0x100000, 0x200000, CRC(d5e93543) SHA1(f081edc26514ca8354c13c7f6f89aba8e4d3e7d2) )

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	/* 0x1800000/2*3 = 0x2400000
		round this up to 0x4000000 so we can mask .. waste of ram?  */
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x200000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegnde )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0102.rom",    0x100000, 0x200000, CRC(4d0f6cc5) SHA1(8d41f0a712fb11a1da865f5159e5e27447b4388a) )

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	/* 0x1800000/2*3 = 0x2400000
		round this up to 0x4000000 so we can mask .. waste of ram?  */
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x200000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegndc )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0101.160",    0x100000, 0x200000, CRC(b24f0c1e) SHA1(a2cf75d739681f091c24ef78ed6fc13aa8cfe0c6) )

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	/* 0x1800000/2*3 = 0x2400000
		round this up to 0x4000000 so we can mask .. waste of ram?  */
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x200000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( dragwld2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-100c.u2",    0x100000, 0x080000, CRC(67467981) SHA1(58af01a3871b6179fe42ff471cc39a2161940043) )

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x800000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END

ROM_START( kov )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.117",    0x100000, 0x400000, CRC(c4d19fe6) SHA1(14ef31539bfbc665e76c9703ee01b12228344052) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kov115 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.115",    0x100000, 0x400000, CRC(527a2924) SHA1(7e3b166dddc5245d7b408e78437c16fd2986d1d9) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kovplus )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.119",    0x100000, 0x400000, CRC(e4b0875d) SHA1(e8382e131b0e431406dc2a05cc1ef128302d987c) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kovsh )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.322",    0x100000, 0x400000, CRC(7c78e5f3) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.320",    0x400000, 0x400000, CRC(164b3c94) ) // bad? its half the size of the kov one

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/*** GAME ********************************************************************/

GAMEX( 1997, pgm,      0,          pgm, pgm,   0,          ROT0, "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAMEX( 1997, orlegend, pgm,        pgm, pgm,   orlegend,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 126)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orlegnde, orlegend,   pgm, pgm,   orlegnde,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, orlegndc, orlegend,   pgm, pgm,   orlegnde,   ROT0, "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112, Chinese Board)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1997, dragwld2, pgm,        pgm, pgm,   dragwld2,   ROT0, "IGS", "Dragon World II (ver. 100C, China)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, kov,      pgm,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 117)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kov115,   kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour / Sangoku Senki (ver. 115)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) /* ver # provided by protection? */
GAMEX( 1999, kovplus,  kov,        pgm, sango, kov, 	   ROT0, "IGS", "Knights of Valour Plus / Sangoku Senki Plus (ver. 119)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, kovsh,    kov,        pgm, sango, kovsh,	   ROT0, "IGS", "Knights of Valour Superheroes / Sangoku Senki Superheroes (ver. 322)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

/*

ASIC3 is the orlegend protection, its not perfect as it still needs 1 of the
patches.

ASIC3 emulation:

unsigned short BACKREGS1[]={'I','G', 'S', 'A', 'A', 0x7f,'A', 'A', 0x3e,'A',
'I', 0xf9,0xa, '&', 'I','I', 'I',  '2'};
unsigned short REGS[]=
{0x20,0x21,0x22,0x24,0x25,0x26,0x27,0x28,0x2a,0x2b,0x2c,0x2d,0x2e,0x30,0x31,
0x32,0x33,0x34};
unsigned short Backup[0x100];
unsigned short BLastReg=0;


ASIC3Init()
{
  Backup[0]=0xff;
  for(int i=0;i<0x12;++i)
      Backup[REGS[i]]=BACKREGS1[i];
}


unsigned shortASIC3Read16(unsigned int addr)
{
  addr&=0xff;
  if(addr==0xe)
  {
//   if(BLastReg==3)
//    return 0xfc;
//   else

   if(BLastReg<0x20 || BLastReg>0x34
     )
     return Backup[BLastReg]*2;
   else
    return Backup[BLastReg];
}

void ASIC3Write16(unsigned int addr,unsigned short val)
{
  addr&=0xff;
  if(addr==0)
   BLastReg=data;
  else if(addr==0xe)
  {
   if(BLastReg!=3 &&
      (BLastReg<0x20 || BLastReg>0x34)
     )
    Backup[BLastReg]=data;
  }
}

Calendar Emulation:  W: 0xc08006   R: 0xc08006

static unsigned char CalVal,CalMask,CalCom=0,CalCnt=0;

static unsigned char bcd(unsigned char v)
{
 unsigned int d,u;
 d=v/10;
 u=v%10;
 return (d<<4)|u;
}

void CalWrite(char data)
{
 static SYSTEMTIME curdate;
 CalCom<<=1;
 CalCom|=data&1;
 ++CalCnt;
 if(CalCnt==4)
 {
  CalMask=0x1;
  CalVal=1;
  CalCnt=0;
  switch(CalCom&0xf)
  {
   case 0:
    CalVal=bcd(curdate.wDayOfWeek); //??
    break;
   case 1:
    CalVal=2;
    break;
   case 2:  //Hours
    CalVal=bcd(curdate.wHour);
    break;
   case 3:
    CalVal=4;
    break;
   case 4:  //seconds
    CalVal=bcd(curdate.wSecond);
    break;
   case 5:
    CalVal=6;
    break;
   case 6:  //Month
    CalVal=bcd(curdate.wMonth); //?? not bcd in MVS
    break;
   case 7:
    CalVal=8;
    break;
   case 8:
    //CalVal=bcd(curdate.wDayOfWeek);
    CalVal=0; //Controls blinking speed, maybe milliseconds
    break;
   case 9:
    CalVal=10;
    break;
   case 0xa: //Day
    CalVal=bcd(curdate.wDay);
    break;
   case 0xb:
    CalVal=12;
    break;
   case 0xc: //Minute
    CalVal=bcd(curdate.wMinute);
    break;
   case 0xd:
    CalVal=14;
    break;
   case 0xe:  //Year
    CalVal=bcd(curdate.wYear%100);
    break;
   case 0xf:  //Load Date
    GetLocalTime(&curdate);
    break;
  }
 }
}

char CalRead()
{
 unsigned char v;
 v=(CalVal&CalMask)?1:0;
 CalMask<<=1;
 return v;
}


*/


